"""Run the selected B read-only acceptance on a registered, owned private product."""

import argparse
import asyncio
import json
import os
import sys
from pathlib import Path

from mcp import Client, StdioServerParameters
import win32api

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "mcp_bridge/tests"))
from test_product_interop import Product, connection_pending
from test_sdk_interop import business_state, call, connection, elevation, identity, integrity, parameters
from qkeymapper_mcp.credentials import CredentialStore


async def compare_state(client, baseline):
    state = (await call(client, "qkm_get_state"))["result"]
    assert state["schema_version"] == 1
    assert state["run"]["gui_state"] == "idle" and not state["run"]["monitoring_enabled"]
    assert state["run"]["actual_stop_shortcut"] == baseline["stop_shortcut_edit"]
    assert state["current_tab"]["name"] == baseline["mapping_tab_name"]
    observed = (await call(client, "qkm_wait_state",
        {"condition": "stopped_and_quiescent", "timeout_ms": 200}))["result"]
    assert observed["outcome"] == "timed_out" and observed["condition"] == "stopped_and_quiescent"
    assert business_state(observed["state"]) == business_state(state)
    return business_state(state)


async def exercise(args):
    process = win32api.GetCurrentProcess()
    assert elevation(process) == 0 and integrity(process) == 0x2000
    product = Product(args.executable, args.output, lifetime_seconds=180)
    store = None
    owned = False
    try:
        await product.started()
        baseline = await product.action("OpenAdvanced")
        initial = await product.action("OpenMcp")
        assert not initial["enabled"] and initial["pair_count"] == 0
        enabled = await product.action("Enable")
        discovery = Path(enabled["discovery"])
        assert enabled["enabled"] and discovery.name == "instance.json"
        await product.wait(discovery.is_file)
        own = identity(os.getpid())
        store = CredentialStore(discovery, args.executable, own.session)
        assert store.load() is None
        owned = True
        config = StdioServerParameters(command=str(args.rust_exe), cwd=str(args.output),
            args=["--executable", str(args.executable), "--discovery-file", str(discovery)])
        ini = args.executable.parent / "keymapdata.ini"
        before = ini.read_bytes()
        async with Client(config) as client:
            await call(client, "qkm_connect")
            await connection(client, "pairing_required")
            await call(client, "qkm_request_pairing")
            await connection_pending(client)
            await product.action("ApproveRead", str(args.rust_exe))
            binding = await connection(client, "connected")
            assert binding["scopes"] == 1 and store.load() is not None
            caps = (await call(client, "qkm_get_capabilities"))["result"]
            assert caps["instance"] == binding["instance"] and caps["stage"] == "business"
            assert caps["scopes"] == 1 and any(
                item["id"] == "state.read" and item["read_only"] for item in caps["business_capabilities"]
            )
            rust_state = await compare_state(client, baseline)
            assert rust_state == await compare_state(client, baseline)
            assert rust_state["instance"] == binding["instance"]
            await call(client, "qkm_disconnect")
            await connection(client, "disconnected")
        python = parameters(discovery, args.executable, args.output)
        async with Client(python) as client:
            await call(client, "qkm_connect")
            assert (await connection(client, "connected"))["instance"] == binding["instance"]
            assert await compare_state(client, baseline) == rust_state
            await call(client, "qkm_disconnect")
            await connection(client, "disconnected")
        async with Client(config, mode="legacy") as client:
            await call(client, "qkm_connect")
            assert (await connection(client, "connected"))["scopes"] == 1
            assert await compare_state(client, baseline) == rust_state
            assert ini.read_bytes() == before
            await product.action("Revoke")
            await call(client, "qkm_get_state", error=("connection_closed", "identity_rejected"))
            await call(client, "qkm_connect")
            await connection(client, "failed")
        await product.action("Disable")
        print("PASS private stock product, Read only, Rust/Python/new Rust state match, INI unchanged during reads")
        print("PASS revocation rejected stored pairing")
    finally:
        try:
            if store is not None and owned:
                store.forget()
                assert store.load() is None
        finally:
            await product.close()
    print("PASS dedicated credential removed and owned product closed")
    (args.output / "result.json").write_text(json.dumps({
        "passed": True, "product": str(args.executable), "rust": str(args.rust_exe),
        "desktop": product.desktop_name, "permissions": ["Read"],
    }, indent=2) + "\n", encoding="utf-8")


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
    root = ROOT / "out/mcp_product"
    if root not in args.executable.parents or root not in args.output.parents:
        parser.error("Use the owned B runtime and a fresh output below out/mcp_product.")
    if args.executable.name != "QKeyMapperMcpAcceptance_RustB.exe":
        parser.error("The registered ordinary B product copy is required.")
    if ROOT / "out/rust_sidecar" not in args.rust_exe.parents:
        parser.error("Use the owned Rust B candidate.")
    args.output.mkdir(exist_ok=False)
    asyncio.run(exercise(args))
