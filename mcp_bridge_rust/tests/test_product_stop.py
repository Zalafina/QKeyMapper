"""Selected real GUI/stdio stop acceptance, using an empty unmatched profile."""

import argparse
import asyncio
import json
import os
import sys
import uuid
from pathlib import Path

from mcp import Client, StdioServerParameters
import win32api

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "mcp_bridge/tests"))
from test_product_interop import Product, connection_pending
from test_sdk_interop import call, connection, elevation, integrity
from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.windows import identity


def fixture_ini():
    template = (ROOT / "QKeyMapper/tests/mcp_control/product_fixture.ini.example").read_text(encoding="utf-8")
    return (template.replace("[General]\n", "[General]\nSettingSelect=Monitoring157\nMappingStartActionMode=0\n", 1)
        + "\n[Monitoring157]\nMappingTable_TabNameList=Tab1\nMappingTable_TabIncludeCommonList=OFF\n"
        "MappingTable_Common_Enabled=false\nKeyMapData_OriginalKeys=\nKeyMapData_MappingKeys=\n"
        "AutoStartMappingChecked=0\nEnableSystemFilterKeyChecked=false\nDisableFilterKeyClickSoundChecked=false\n"
        "ProcessInfo_FileName=QkmAbsentMonitoring157.exe\nProcessInfo_FileNameMatchIndex=1\n"
        "ProcessInfo_WindowTitle=QkmAbsentMonitoring157\nProcessInfo_WindowTitleMatchIndex=1\n"
        "ProcessInfo_ClassNameMatchIndex=0\nProcessInfo_DisplayModeMatchIndex=0\n").encode("utf-8")


async def get_state(client, gui_state):
    result = (await call(client, "qkm_get_state"))["result"]
    run = result["run"]
    assert run["gui_state"] == gui_state and run["worker_state_sample"] == "stopped", run
    assert result["mapping_load_context"]["outcome"] == "accepted", result["mapping_load_context"]
    assert result["configuration"]["group"] == "Monitoring157", result["configuration"]
    return result


async def stopped(client, request):
    wait = (await call(client, "qkm_wait_state",
        {"condition": "stopped_and_quiescent", "timeout_ms": 5000}))["result"]
    assert wait["outcome"] == "satisfied", wait["state"]["run"]
    result = (await call(client, "qkm_get_operation", {"operation_id": request["request_id"]}))["result"]
    assert result["status"] == "completed" and result["stop_completion"] == "observed", result
    assert result["target"]["instance_id"] == request["instance"] and result["stop_initiated"]
    return result


async def run(args):
    assert elevation(win32api.GetCurrentProcess()) == 0 and integrity(win32api.GetCurrentProcess()) == 0x2000
    ini = args.executable.parent / "keymapdata.ini"
    with ini.open("xb") as output:
        output.write(fixture_ini())
    product = Product(args.executable, args.output)
    store = None
    owned = False
    evidence = {}
    try:
        await product.started()
        await product.action("OpenAdvanced")
        await product.action("OpenMcp")
        enabled = await product.action("Enable")
        assert enabled["enabled"] and enabled["pair_count"] == 0, enabled
        discovery = Path(enabled["discovery"])
        await product.wait(discovery.is_file)
        store = CredentialStore(discovery, args.executable, identity(os.getpid()).session)
        assert store.load() is None
        owned = True
        rust = StdioServerParameters(command=str(args.rust_exe), cwd=str(args.output),
            args=["--executable", str(args.executable), "--discovery-file", str(discovery)])
        async with Client(rust) as client:
            for approval, scopes in (("ApproveRead", 1), ("ApproveRunMapping", 5)):
                await call(client, "qkm_connect")
                await connection(client, "pairing_required")
                await call(client, "qkm_request_pairing")
                await connection_pending(client)
                await product.action(approval, str(args.rust_exe))
                binding = await connection(client, "connected")
                assert binding["scopes"] == scopes, binding
                state = await get_state(client, "idle")
                assert state["instance"] == binding["instance"]
                request = {"instance": state["instance"], "request_id": uuid.uuid4().hex}
                if scopes == 1:
                    await call(client, "qkm_stop_mapping", request, error="permission_denied")
                    await product.action("Revoke")
                    await call(client, "qkm_get_state", error=("connection_closed", "identity_rejected"))
                    store.forget()
            await call(client, "qkm_stop_mapping", request | {"instance": "0" * 32}, error="wrong_instance")
            idle = (await call(client, "qkm_stop_mapping", request))["result"]
            assert idle["status"] == "already_idle" and idle["stop_completion"] == "unobserved", idle
            sources = (await call(client, "qkm_query", {"entity": "mapping_sources", "limit": 100}))["result"]
            assert len(sources["items"]) == sources["total"] and sources["total"] >= 1
            for tab in sources["items"]:
                rows = (await call(client, "qkm_query", {"entity": "mappings",
                    "source_revision": sources["source_snapshot"]["revision"], "source_tab_ref": tab["source_tab_ref"]}))["result"]
                assert rows["total"] == 0, rows
            assert ini.read_bytes() == fixture_ini()
            await product.action("CloseMcp")
            await product.action("StartPrivateMonitoring")
            await get_state(client, "checking")
            first = request | {"request_id": uuid.uuid4().hex}
            evidence["accepted"] = (await call(client, "qkm_stop_mapping", first))["result"]
            assert evidence["accepted"]["operation_id"] == first["request_id"]
            assert evidence["accepted"]["status"] == "waiting_cleanup" and evidence["accepted"]["stop_initiated"]
            await call(client, "qkm_disconnect")
            await connection(client, "disconnected")
        async with Client(rust, mode="legacy") as client:
            await call(client, "qkm_connect")
            assert (await connection(client, "connected"))["instance"] == first["instance"]
            evidence["first"] = await stopped(client, first)
            await product.action("StartPrivateMonitoring")
            await get_state(client, "checking")
            replay = (await call(client, "qkm_stop_mapping", first))["result"]
            assert replay.pop("replayed") is True, replay
            assert replay.pop("query_sampled_at_utc")
            assert replay == {key: value for key, value in evidence["first"].items() if key != "query_sampled_at_utc"}, replay
            await get_state(client, "checking")
            second = first | {"request_id": uuid.uuid4().hex}
            await call(client, "qkm_stop_mapping", second)
            evidence["second"] = await stopped(client, second)
            assert ini.read_bytes() == fixture_ini()
            await product.action("OpenAdvanced")
            await product.action("OpenMcp")
            revoked = await product.action("Revoke")
            assert revoked["pair_count"] == 0, revoked
            await call(client, "qkm_get_state", error=("connection_closed", "identity_rejected"))
        await product.action("Disable")
    finally:
        try:
            if owned:
                store.forget()
                assert store.load() is None
        finally:
            await product.close()
    evidence.update(desktop=product.desktop_name, executable=str(args.executable), bridge=str(args.rust_exe))
    (args.output / "result.json").write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print("PASS private GUI start, scoped MCP stop, disconnect, reauthentication, replay, cleanup and unchanged INI")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--rust-exe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.executable = args.executable.resolve(strict=True)
    args.rust_exe = args.rust_exe.resolve(strict=True)
    args.output = args.output.resolve()
    product_root = ROOT / "out/mcp_product"
    if (sys.flags.optimize or product_root not in args.executable.parents or product_root not in args.output.parents
            or args.executable.name != "QKeyMapperMcpAcceptance_Monitoring.exe"
            or ROOT / "out/rust_sidecar" not in args.rust_exe.parents):
        parser.error("Use the registered isolated monitoring fixture and an owned Sidecar with assertions enabled.")
    args.output.mkdir(parents=True, exist_ok=False)
    asyncio.run(run(args))
