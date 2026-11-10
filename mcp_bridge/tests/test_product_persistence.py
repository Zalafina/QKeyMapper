"""Opt-in stock Release persistence acceptance on owned private desktops; no mapping execution."""

import argparse
import asyncio
from contextlib import closing
import ctypes
from ctypes import wintypes
import importlib
import hashlib
import json
import os
import winreg
from pathlib import Path
from unittest.mock import patch

import win32api
from mcp import Client, StdioServerParameters

from test_product_interop import Product, connection_pending
from test_sdk_interop import call, connection, integrity
from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.windows import identity, persistent_target


def filter_keys():
    class State(ctypes.Structure):
        _fields_ = [(name, wintypes.DWORD) for name in ("size", "flags", "wait", "delay", "repeat", "bounce")]
    state = State()
    state.size = ctypes.sizeof(state)
    user = ctypes.WinDLL("user32", use_last_error=True)
    if not user.SystemParametersInfoW(0x0032, state.size, ctypes.byref(state), 0):
        raise ctypes.WinError(ctypes.get_last_error())
    return {name: getattr(state, name) for name, _ in State._fields_}


async def run(args):
    root = Path(__file__).resolve().parents[2]
    assert args.executable.is_relative_to(root / "out/mcp_product")
    assert args.output.is_relative_to(root / "out/mcp_product") and not args.output.exists()
    fixture = root / "QKeyMapper/tests/mcp_control/product_persistence_fixture.ini.example"
    if args.resume_paired is None:
        assert (args.executable.parent / "keymapdata.ini").read_bytes() == fixture.read_bytes(), "Use only the unchanged synthetic persistence fixture."
    args.output.mkdir(parents=True)
    calls, peers, completed = [], [], []
    store = saved = None
    own = identity(os.getpid())
    system_before = filter_keys()
    (args.output / "filter-keys-before.json").write_text(json.dumps(system_before), encoding="utf-8")
    with closing(win32api.OpenProcess(0x1000, False, own.pid)) as handle:
        assert integrity(handle) == 8192
    stdio = importlib.import_module("mcp.client.stdio")
    spawn = stdio._create_platform_compatible_process

    async def observed_spawn(*values, **keywords):
        process = await spawn(*values, **keywords)
        try:
            peer = identity(process.pid)
            assert own.same_login(peer) and Path(peer.executable) == args.bridge_python
            with closing(win32api.OpenProcess(0x1000, False, peer.pid)) as handle:
                assert integrity(handle) == 8192
            peers.append(dict(pid=peer.pid, created=str(peer.created), executable=peer.executable, integrity=8192))
            (args.output / "bridge-peers.json").write_text(json.dumps(peers, indent=2), encoding="utf-8")
        except BaseException:
            process.terminate()
            await process.wait()
            raise
        return process

    async def request(client, phase, name, arguments=None, *, error=None):
        value = await call(client, name, arguments, error=error)
        calls.append(dict(phase=phase, tool=name, arguments=arguments, value=value))
        (args.output / "sdk-calls.json").write_text(json.dumps(calls, ensure_ascii=False, indent=2), encoding="utf-8")
        return value

    def parameters(discovery, output, *, persistent=None):
        if persistent is None:
            persistent = args.persistent_target
        target = ["--persistent-target"] if persistent else ["--discovery-file", str(discovery)]
        return StdioServerParameters(command=str(args.bridge_python), args=["-I", "-B", "-X", "utf8", "-m", "qkeymapper_mcp",
            *target, "--executable", str(args.executable)], cwd=str(output))

    async def managed(phase, body):
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Enum\ROOT\FAKERINPUT"):
                raise AssertionError("FakerInput is present; ordinary shutdown can send neutral reports. Review scope before startup.")
        except FileNotFoundError:
            pass
        output = args.output / phase
        output.mkdir()
        product = Product(args.executable, output, lifetime_seconds=600)
        try:
            await product.started()
            await body(product)
        finally:
            await product.close()
            system_after = filter_keys()
            (output / "filter-keys-after.json").write_text(json.dumps(system_after), encoding="utf-8")
            assert system_after == system_before, "Global FilterKeys changed; do not restore it automatically."
        completed.append(phase)

    async def management(product):
        baseline = await product.action("OpenAdvanced")
        assert baseline["mapping_button_text"] == "映射开始"
        value = await product.action("OpenMcp")
        return value

    async def first(product):
        value = await management(product)
        assert value["enabled"] is False and value["pair_count"] == 0
        assert (await product.action("Enable"))["enabled"] is True
        value = await product.action("SaveStartup")
        assert value["enabled"] and value["startup_status"] == "已保存的启动选择：启用。"
        await product.action("CloseMcp")

    old = {}

    async def paired(product):
        nonlocal store, saved
        value = await management(product)
        assert value["enabled"] and value["pair_count"] == 0
        discovery = Path(value["discovery"])
        if args.persistent_target:
            assert discovery == persistent_target(args.executable, own.session)[0]
        candidate = CredentialStore(discovery, args.executable, own.session, persistent=args.persistent_target)
        assert candidate.load() is None, "Never delete or replace an existing credential."
        store = candidate
        async with Client(parameters(discovery, product.output), cache=None) as client:
            await request(client, "paired", "qkm_connect")
            await connection(client, "pairing_required")
            await request(client, "paired", "qkm_request_pairing")
            await connection_pending(client)
            await product.action("ApproveConfigure", str(args.bridge_python))
            state = await connection(client, "connected")
            assert state["scopes"] == 3
            (product.output / "authenticated.json").write_text(json.dumps(state), encoding="utf-8")
            saved = store.load()
            assert saved is not None and saved.scopes == 3
            old.update(discovery=str(discovery), instance=state["instance"], pid=product.pid,
                       created=product.started_record["created"])
            sources = (await request(client, "paired", "qkm_query", {"entity": "mapping_sources", "limit": 100}))["result"]
            source = sources["items"][0]
            assert source["attached_to_ui"] and source["row_count"] == 0
            mapping = dict(source_revision=sources["source_snapshot"]["revision"], source_tab_ref=source["source_tab_ref"],
                           original_key="A", mapping_key="B")
            preview = (await request(client, "paired", "qkm_preview_changes", mapping))["result"]
            applied = (await request(client, "paired", "qkm_apply_changes", {"preview_id": preview["preview_id"]}))["result"]
            assert applied["status"] == "applied" and applied["result"]["saved"] is False
            assert applied["result"]["started_by_operation"] is False
            old["add_operation"] = preview["preview_id"]
            preview = (await request(client, "paired", "qkm_preview_changes", {"action": "configuration.save", "setting_name": "Persist136A"}))["result"]
            assert preview["description"]["apply_supported"]
            saved_result = (await request(client, "paired", "qkm_apply_changes", {"preview_id": preview["preview_id"], "confirm_scope": True}))["result"]
            assert saved_result["status"] == "applied" and saved_result["result"]["saved"] and saved_result["result"]["reloaded"]
            assert saved_result["result"]["started_by_operation"] is False
            old["save_operation"] = preview["preview_id"]
            sources = (await request(client, "paired", "qkm_query", {"entity": "mapping_sources", "limit": 100}))["result"]
            source = sources["items"][0]
            old.update(source_revision=sources["source_snapshot"]["revision"], source_tab_ref=source["source_tab_ref"])
            rows = (await request(client, "paired", "qkm_query", dict(entity="mappings", source_revision=old["source_revision"], source_tab_ref=old["source_tab_ref"])))["result"]
            assert len(rows["items"]) == 1
            old["source_ref"] = rows["items"][0]["source_ref"]
            preview = (await request(client, "paired", "qkm_preview_changes", dict(mapping, source_revision=old["source_revision"],
                source_tab_ref=old["source_tab_ref"], original_key="C", mapping_key="D")))["result"]
            old["unconsumed_preview"] = preview["preview_id"]
            gui = await product.action("ReadMapping")
            assert gui["mapping_rows"] == [dict(original="A", mapping="B")] and gui["mapping_button_text"] == "映射开始"

    async def reused(product):
        value = await management(product)
        assert value["enabled"] and value["pair_count"] == 1 and Path(value["discovery"]) == Path(old["discovery"])
        assert (product.pid, product.started_record["created"]) != (old["pid"], old["created"])
        assert store.load() == saved
        async with Client(parameters(Path(old["discovery"]), product.output), cache=None) as client:
            await request(client, "reused", "qkm_connect")
            state = await connection(client, "connected")
            assert state["scopes"] == 3 and state["instance"] != old["instance"] and store.load() == saved
            (product.output / "authenticated.json").write_text(json.dumps(state), encoding="utf-8")
            summary = (await request(client, "reused", "qkm_get_state"))["result"]
            assert summary["instance"] == state["instance"] and summary["configuration"]["group"] == "Persist136A"
            assert summary["run"]["gui_state"] == "idle" and summary["run"]["worker_state_sample"] == "stopped"
            sources = (await request(client, "reused", "qkm_query", {"entity": "mapping_sources", "limit": 100}))["result"]
            source = sources["items"][0]
            rows = (await request(client, "reused", "qkm_query", dict(entity="mappings", source_revision=sources["source_snapshot"]["revision"], source_tab_ref=source["source_tab_ref"])))["result"]
            assert len(rows["items"]) == 1 and rows["items"][0]["source_ref"] != old["source_ref"]
            assert rows["items"][0]["fields"]["KeyMapData_OriginalKeys"] == "A"
            assert rows["items"][0]["fields"]["KeyMapData_MappingKeys"] == ["B"]
            await request(client, "reused", "qkm_query", dict(entity="mappings", source_revision=old["source_revision"], source_tab_ref=old["source_tab_ref"]), error="stale_source")
            await request(client, "reused", "qkm_apply_changes", {"preview_id": old["unconsumed_preview"]}, error="stale_preview")
            for name in ("add_operation", "save_operation"):
                value = (await request(client, "reused", "qkm_get_operation", {"operation_id": old[name]}))["result"]
                assert value["status"] == "unknown"
            gui = await product.action("ReadMapping")
            assert gui["mapping_rows"] == [dict(original="A", mapping="B")] and gui["mapping_button_text"] == "映射开始"
        await product.action("Disable")
        value = await product.action("SaveStartup")
        assert value["enabled"] is False and value["startup_status"] == "已保存的启动选择：关闭。"

    async def disabled(product):
        value = await management(product)
        assert value["enabled"] is False and value["pair_count"] == 1 and not value["discovery"]
        assert value["startup_status"] == "已保存的启动选择：关闭。"
        gui = await product.action("ReadMapping")
        assert gui["mapping_rows"] == [dict(original="A", mapping="B")] and gui["mapping_button_text"] == "映射开始"
        # Revoke only this fixture's single pairing as cleanup, without enabling the interface.
        await product.action("Revoke")
        if args.persistent_target:
            # A separate Read-only pairing proves the old explicit entry, without copying v2 keys.
            discovery = Path((await product.action("Enable"))["discovery"])
            legacy = CredentialStore(discovery, args.executable, own.session)
            assert legacy.load() is None
            async with Client(parameters(discovery, product.output, persistent=False), cache=None) as client:
                await request(client, "legacy_read", "qkm_connect")
                await connection(client, "pairing_required")
                await request(client, "legacy_read", "qkm_request_pairing")
                await connection_pending(client)
                await product.action("ApproveRead", str(args.bridge_python))
                assert (await connection(client, "connected"))["scopes"] == 1
                state = (await request(client, "legacy_read", "qkm_get_state"))["result"]
                assert state["configuration"]["group"] == "Persist136A" and state["run"]["gui_state"] == "idle"
                await request(client, "legacy_read", "qkm_preview_changes", {"action": "configuration.save", "setting_name": "Persist136A"}, error="permission_denied")
                await request(client, "legacy_read", "qkm_disconnect")
                await connection(client, "disconnected")
            await product.action("Disable")
            await product.action("Revoke")
            legacy.forget()

    try:
        with patch.object(stdio, "_create_platform_compatible_process", observed_spawn):
            if args.resume_paired is None:
                await managed("first", first)
                await managed("paired", paired)
                with args.executable.open("rb") as stream:
                    executable_hash = hashlib.file_digest(stream, "sha256").hexdigest()
                (args.output / "paired-receipt.json").write_text(json.dumps(dict(executable=str(args.executable),
                    sha256=executable_hash, evidence=str(args.output), old=old, pair_id=saved.pair_id, epoch=saved.epoch,
                    persistent_target=args.persistent_target)), encoding="utf-8")
            else:
                receipt_path = args.resume_paired.resolve(strict=True)
                assert receipt_path.is_relative_to(root / "out/mcp_product") and receipt_path.stat().st_size <= 8192
                receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
                assert receipt.keys() - {"persistent_target"} == {"executable", "sha256", "evidence", "old", "pair_id", "epoch"}
                assert receipt.get("persistent_target", False) is args.persistent_target
                assert Path(receipt["executable"]) == args.executable
                with args.executable.open("rb") as stream:
                    assert hashlib.file_digest(stream, "sha256").hexdigest() == receipt["sha256"]
                evidence = Path(receipt["evidence"]).resolve(strict=True)
                assert evidence.is_relative_to(root / "out/mcp_product")
                for phase in ("first", "paired"):
                    record = json.loads((evidence / phase / "started.json").read_text(encoding="utf-8"))
                    closed = json.loads((evidence / phase / "closed.json").read_text(encoding="utf-8"))
                    assert Path(record["executable"]) == args.executable and closed["exit_code"] == 0 and closed["pid"] == record["pid"]
                old.update(receipt["old"])
                store = CredentialStore(Path(old["discovery"]), args.executable, own.session, persistent=args.persistent_target)
                saved = store.load()
                assert saved is not None and (saved.pair_id, saved.epoch, saved.scopes) == (receipt["pair_id"], receipt["epoch"], 3)
                completed.extend(["first_reused_evidence", "paired_reused_evidence"])
            await managed("reused", reused)
            await managed("disabled", disabled)
        assert len(peers) == (2 if args.resume_paired is None else 1) + int(args.persistent_target)
        assert len({(peer["pid"], peer["created"]) for peer in peers}) == len(peers)
        assert sum(item["tool"] == "qkm_request_pairing" for item in calls) == (1 if args.resume_paired is None else 0) + int(args.persistent_target)
        (args.output / "passed.json").write_text(json.dumps(dict(completed=completed, peers=peers, old=old,
            prior_paired_receipt=str(args.resume_paired) if args.resume_paired else None,
            new_pairing_requests=sum(item["tool"] == "qkm_request_pairing" for item in calls), granted_scopes=3,
            target_mode="persistent_v2" if args.persistent_target else "explicit_v1",
            legacy_direct_scopes=1 if args.persistent_target else None,
            mapping_started=False, configuration="Persist136A"), ensure_ascii=False, indent=2), encoding="utf-8")
        print("PASS stock Release: startup choice, GUI pairing reuse, A-to-B save/cold read, stale references and saved off; prior evidence reuse is recorded", flush=True)
    finally:
        if store is not None and len(completed) == 4:
            store.forget()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=lambda value: Path(value).resolve(strict=True), required=True)
    parser.add_argument("--bridge-python", type=lambda value: Path(value).resolve(strict=True), required=True)
    parser.add_argument("--output", type=lambda value: Path(value).resolve(), required=True)
    parser.add_argument("--resume-paired", type=Path, help="Reuse this test's explicit non-secret receipt after verifying prior normal exits; never repeat accepted writes.")
    parser.add_argument("--persistent-target", action="store_true", help="Verify v2 fixed target arguments and a separate Read-only legacy entry.")
    asyncio.run(run(parser.parse_args()))
