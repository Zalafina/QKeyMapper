"""Compare Rust and Python transactions on an explicitly authorized private product."""

import argparse
import asyncio
import json
import os
import sys
import winreg
from contextlib import asynccontextmanager, closing
from pathlib import Path

from mcp import Client, StdioServerParameters
import win32api
import win32con
import win32file

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "mcp_bridge/tests"))
from test_product_interop import Product, connection_pending
from test_product_persistence import filter_keys
from test_sdk_interop import call, connection, integrity, parameters
from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.windows import identity


async def run(args):
    assert integrity(win32api.GetCurrentProcess()) == 8192
    ini = args.executable.parent / "keymapdata.ini"
    fixture = ROOT / "QKeyMapper/tests/mcp_control/product_persistence_fixture.ini.example"
    assert ini.read_bytes() == fixture.read_bytes(), "An unchanged, owned synthetic INI is required."
    args.output.mkdir(parents=True, exist_ok=False)
    filter_before = filter_keys()
    store = None

    async def request(client, tool, arguments=None, *, error=None):
        value = await call(client, tool, arguments, error=error)
        with (args.output / "calls.jsonl").open("a", encoding="utf-8") as log:
            print(json.dumps({"tool": tool, "arguments": arguments, "response": value}, ensure_ascii=False), file=log)
        return value if error else value["result"]

    @asynccontextmanager
    async def product(phase):
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Enum\ROOT\FAKERINPUT"):
                raise AssertionError("FakerInput is present; review ordinary shutdown before startup.")
        except FileNotFoundError:
            pass
        output = args.output / phase
        output.mkdir()
        app = Product(args.executable, output, lifetime_seconds=420)
        try:
            await app.started()
            yield app
        finally:
            await app.close()
            assert filter_keys() == filter_before, "System FilterKeys changed."

    async def enable(app, pairs):
        gui = await app.action("OpenAdvanced")
        assert gui["mapping_button_text"] == "映射开始"
        state = await app.action("OpenMcp")
        assert not state["enabled"] and state["pair_count"] == pairs
        return Path((await app.action("Enable"))["discovery"])

    def rust_config(discovery, app):
        return StdioServerParameters(command=str(args.rust_exe), cwd=str(app.output), args=[
            "--discovery-file", str(discovery), "--executable", str(args.executable),
        ])

    @asynccontextmanager
    async def connected(config, bound):
        async with Client(config, cache=None) as client:
            await call(client, "qkm_connect")
            info = await connection(client, "connected")
            assert info["instance"] == bound and info["scopes"] == 3
            yield client

    async def source(client):
        page = await request(client, "qkm_query", {"entity": "mapping_sources"})
        items = [item for item in page["items"] if item["attached_to_ui"]]
        assert len(items) == 1
        return dict(source_revision=page["source_snapshot"]["revision"], source_tab_ref=items[0]["source_tab_ref"])

    async def preview(client, **arguments):
        return (await request(client, "qkm_preview_changes", arguments))["preview_id"]

    async def apply(client, preview_id, *, error=None, **confirmations):
        return await request(client, "qkm_apply_changes", dict(preview_id=preview_id, **confirmations), error=error)

    async def operation(client, preview_id):
        return await request(client, "qkm_get_operation", {"operation_id": preview_id})

    async def assert_row(client, target, method):
        rows = (await request(client, "qkm_query", dict(target, entity="mappings")))["items"]
        assert len(rows) == 1
        fields = rows[0]["fields"]
        assert fields["KeyMapData_OriginalKeys"] == "A" and fields["KeyMapData_MappingKeys"] == ["B"]
        assert fields["KeyMapData_SendMappingKeyMethod"] == method
        return rows[0]["source_ref"]

    save = {"action": "configuration.save", "setting_name": "Persist136A"}
    try:
        async with product("first") as app:
            discovery = await enable(app, 0)
            candidate = CredentialStore(discovery, args.executable, identity(os.getpid()).session)
            assert candidate.load() is None, "The test must not replace an existing pairing."
            store = candidate
            rust = rust_config(discovery, app)
            python = parameters(discovery, args.executable, app.output)
            async with Client(rust, cache=None) as client:
                await call(client, "qkm_request_pairing")
                await connection_pending(client)
                await app.action("ApproveConfigure", str(args.rust_exe))
                info = await connection(client, "connected")
                assert info["scopes"] == 3 and store.load().scopes == 3
                bound = info["instance"]
                await app.action("CloseMcp")
                before = ini.read_bytes()
                help_arguments = {"topic": "original_keys", "offset": 1, "limit": 4}
                help_page = await request(client, "qkm_get_help", help_arguments)
                initial = await source(client)
                assert not (await request(client, "qkm_query", dict(initial, entity="mappings")))["items"]
                discard = await preview(client, **initial, original_key="C", mapping_key="D")
            async with connected(python, bound) as client:
                assert await request(client, "qkm_get_help", help_arguments) == help_page
                assert await source(client) == initial
                assert (await request(client, "qkm_cancel_changes", {"preview_id": discard}))["status"] == "cancelled"
                add = await preview(client, **initial, original_key="A", mapping_key="B", send_method=0)
            assert ini.read_bytes() == before
            async with connected(rust, bound) as client:
                applied = await apply(client, add)
                assert applied["status"] == "applied" and not applied["result"]["saved"]
                await request(client, "qkm_query", dict(initial, entity="mappings"), error="stale_source")
                target = await source(client)
                row = await assert_row(client, target, 0)
                edit = await preview(client, action="mapping.update", source_ref=row, send_method=3)
                assert (await apply(client, edit))["status"] == "applied"
                denied = await preview(client, **save)
                await apply(client, denied, error="confirmation_required")
                failed = await preview(client, **save)
                with closing(win32file.CreateFile(str(ini), win32con.GENERIC_READ, win32con.FILE_SHARE_READ,
                                                 None, win32con.OPEN_EXISTING, 0, None)):
                    await apply(client, failed, confirm_scope=True, error="save_write_failed")
                assert ini.read_bytes() == before
                result = await operation(client, failed)
                assert result["status"] == "failed" and not result["result"]["saved"] and not result["result"]["reloaded"]
                assert (await request(client, "qkm_get_state"))["configuration"]["has_unsaved_changes"]
                saved = await preview(client, **save)
                result = await apply(client, saved, confirm_scope=True)
                assert result["status"] == "applied" and result["result"]["saved"] and result["result"]["reloaded"]
                assert not result["result"]["started_by_operation"] and ini.read_bytes() != before
                previous = await source(client)
                pending = await preview(client, **previous, original_key="C", mapping_key="D")
            async with connected(python, bound) as client:
                assert (await operation(client, saved))["status"] == "applied"
                assert (await operation(client, failed))["status"] == "failed"
            gui = await app.action("ReadMapping")
            assert gui["mapping_rows"] == [dict(original="A", mapping="B")]
        print("PASS same-pair Python/Rust preview, cancel, commit, save failure and recovery", flush=True)

        async with product("cold") as app:
            assert await enable(app, 1) == discovery
            async with Client(rust_config(discovery, app), cache=None) as client:
                await call(client, "qkm_connect")
                info = await connection(client, "connected")
                assert info["instance"] != bound and info["scopes"] == 3
                current = await source(client)
                await assert_row(client, current, 3)
                await request(client, "qkm_query", dict(previous, entity="mappings"), error="stale_source")
                await apply(client, pending, error="stale_preview")
                assert (await operation(client, saved))["status"] == "unknown"
            gui = await app.action("ReadMapping")
            assert gui["mapping_rows"] == [dict(original="A", mapping="B")]
            assert gui["mapping_button_text"] == "映射开始"
            await app.action("Revoke")
            await app.action("Disable")
        print("PASS new instance cold read, old reference rejection and normal close", flush=True)
    finally:
        if store is not None:
            store.forget()
            assert store.load() is None


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--rust-exe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if sys.flags.optimize:
        parser.error("Run with assertions enabled.")
    args.executable = args.executable.resolve(strict=True)
    args.rust_exe = args.rust_exe.resolve(strict=True)
    args.output = args.output.resolve()
    owned = ROOT / "out/mcp_product"
    for path in vars(args).values():
        assert path.is_relative_to(owned), "Only owned acceptance copies are supported."
    assert args.executable.name == "QKeyMapperMcpAcceptance_RustC.exe"
    asyncio.run(run(args))
