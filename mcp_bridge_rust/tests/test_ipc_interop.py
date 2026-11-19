"""Compare the Rust and Python clients against the existing private Qt fixture."""

import argparse
import asyncio
import json
import os
import sys
import time
from pathlib import Path

from mcp import Client, StdioServerParameters
import win32api
import win32file
import win32security

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "mcp_bridge/tests"))
from test_sdk_interop import Fixture, call, connection, elevation, identity, pending
from test_tool_contract import TOOLS
from qkeymapper_mcp.credentials import CredentialStore


def rust_parameters(args, discovery, executable):
    return StdioServerParameters(command=str(args.rust_exe),
        args=["--discovery-file", str(discovery), "--executable", str(executable)],
        cwd=str(args.output))


def new_discovery(output, name, user, value, *, private=True):
    path = output / name
    security = win32security.SECURITY_ATTRIBUTES()
    unexpected_ace = "" if private else "(A;OICI;FR;;;SY)"
    security.SECURITY_DESCRIPTOR = win32security.ConvertStringSecurityDescriptorToSecurityDescriptor(
        f"O:{user}D:P(A;OICI;FA;;;{user}){unexpected_ace}", 1)
    win32file.CreateDirectory(str(path), security)
    hint = path / "instance.json"
    hint.write_text(json.dumps(value, ensure_ascii=False), encoding="utf-8")
    return hint


async def read_wait_states(client, fixture, instance):
    async def set_observation(observation):
        fixture.command(observation)
        await fixture.wait(lambda value: value.get("sequence") == fixture.sequence)

    async def wait(condition, timeout_ms):
        value = (await call(client, "qkm_wait_state", {
            "condition": condition, "timeout_ms": timeout_ms
        }))["result"]
        assert value["condition"] == condition and value["state"]["instance"] == instance
        return value

    await set_observation("wait-idle")
    idle = await wait("stopped_and_quiescent", 200)
    assert idle["outcome"] == "timed_out" and idle["state"]["run"]["stop_completion"] == "unobserved"
    assert (await call(client, "qkm_get_connection_state"))["status"] == "connected"

    await set_observation("wait-checking")
    monitoring = await wait("monitoring_enabled", 1000)
    assert monitoring["outcome"] == "satisfied" and monitoring["state"]["run"]["gui_state"] == "checking"
    samples = (await fixture.wait(lambda value: "wait_samples" in value))["wait_samples"]
    waiting = asyncio.create_task(wait("mapping_active", 4000))
    try:
        await fixture.wait(lambda value: value.get("wait_samples", 0) > samples)
        await call(client, "qkm_get_state", error="busy_or_closed")
        await set_observation("wait-active")
        active = await waiting
        assert active["outcome"] == "satisfied" and active["state"]["run"]["gui_state"] == "mapping_matched"
    finally:
        if not waiting.done():
            waiting.cancel()
            await asyncio.gather(waiting, return_exceptions=True)

    await set_observation("wait-cleaned")
    stopped = await wait("stopped_and_quiescent", 1000)
    assert stopped["outcome"] == "satisfied" and stopped["state"]["run"]["stop_completion"] == "observed"

    await set_observation("wait-stall")
    samples = (await fixture.wait(lambda value: "wait_samples" in value))["wait_samples"]
    waiting = asyncio.create_task(call(client, "qkm_wait_state",
        {"condition": "monitoring_enabled", "timeout_ms": 4000}, error="cancelled"))
    try:
        await fixture.wait(lambda value: value.get("wait_samples", 0) > samples)
        await call(client, "qkm_disconnect")
        await waiting
        await connection(client, "disconnected")
    finally:
        if not waiting.done():
            waiting.cancel()
            await asyncio.gather(waiting, return_exceptions=True)

    await call(client, "qkm_connect")
    assert (await connection(client, "connected"))["instance"] == instance
    started = time.monotonic()
    await call(client, "qkm_wait_state", {"condition": "monitoring_enabled", "timeout_ms": 300}, error="timeout")
    assert time.monotonic() - started < 3
    assert (await connection(client, "failed"))["error"] == "timeout"


async def exercise(args):
    assert elevation(win32api.GetCurrentProcess()) == 0
    checks = []

    def passed(name):
        checks.append(name)
        print("PASS", name, flush=True)

    async with Client(rust_parameters(args, args.output / "absent/instance.json", args.test_exe)) as client:
        assert {tool.name for tool in (await client.list_tools()).tools} == TOOLS
        assert (await client.call_tool("qkm_connect", {"unexpected": True})).is_error
        assert (await call(client, "qkm_get_connection_state"))["status"] == "disconnected"
        await call(client, "qkm_connect")
        assert (await connection(client, "failed"))["error"] == "application_unavailable"
    passed("bounded Rust tool catalog, invalid arguments, absent target")

    fixture = Fixture(args.test_exe, args.qt_bin, args.output)
    store = None
    owns_credential = False
    try:
        state = await fixture.wait(lambda value: value.get("enabled"))
        discovery = Path(state["discovery"])
        own = identity(os.getpid())
        assert own.same_login(identity(fixture.pid))
        store = CredentialStore(discovery, args.test_exe, own.session)
        assert store.load() is None, "fixture must start without a stored credential"
        owns_credential = True
        rust = rust_parameters(args, discovery, args.test_exe)
        original = json.loads(discovery.read_text(encoding="utf-8"))

        async with Client(rust_parameters(args, discovery, Path(sys.executable))) as client:
            await call(client, "qkm_request_pairing")
            assert (await connection(client, "failed"))["error"] == "executable_rejected"
        for name, changed, is_private, reason in (
            ("bad-acl", original, False, "unsafe_discovery"),
            ("bad-created", {**original, "created": str(int(original["created"]) + 1)}, True, "identity_rejected"),
            ("bad-exe", {**original, "executable": sys.executable}, True, "executable_rejected"),
            ("bad-pipe", {**original, "pipe": original["pipe"] + "0"}, True, "invalid_discovery"),
        ):
            hint = new_discovery(args.output, name, own.user, changed, private=is_private)
            async with Client(rust_parameters(args, hint, args.test_exe)) as client:
                await call(client, "qkm_request_pairing")
                assert (await connection(client, "failed"))["error"] == reason
        assert not fixture.state()["pending"] and store.load() is None
        passed("actual EXE and creation stamp, private ACL and fixed pipe validation")

        async with Client(rust) as client:
            await call(client, "qkm_connect")
            await connection(client, "pairing_required")
            await pending(client, fixture)
            await call(client, "qkm_connect", error="busy_or_closed")
            await call(client, "qkm_get_capabilities", error="busy_or_closed")
            started = time.monotonic()
            await call(client, "qkm_disconnect")
            await connection(client, "disconnected")
            await fixture.wait(lambda value: not value.get("pending"))
            assert time.monotonic() - started < 3 and store.load() is None
            await asyncio.sleep(1.05)
            await pending(client, fixture)
            fixture.command("reject")
            await connection(client, "failed")
            assert store.load() is None
        passed("busy admission, native cancellation, GUI fixture refusal")

        await asyncio.sleep(1.05)
        async with Client(rust) as client:
            await pending(client, fixture)
            fixture.command("approve")
            bound = await connection(client, "connected")
            assert bound["scopes"] == 1
            cap = (await call(client, "qkm_get_capabilities"))["result"]
            assert cap["instance"] == bound["instance"] and cap["stage"] == "transport_only"
            assert cap["scopes"] == 1 and cap["business_capabilities"] == []
            await call(client, "qkm_get_state", error="unsupported_capability")
            for tool, arguments in (
                ("qkm_wait_state", {"condition": "monitoring_enabled", "timeout_ms": 1000}),
                ("qkm_get_help", {"topic": "basic_mapping"}),
                ("qkm_query", {"entity": "tabs", "offset": 1, "limit": 5}),
                ("qkm_get_operation", {"operation_id": "a" * 32}),
            ):
                denied = await call(client, tool, arguments, error="unsupported_capability")
                assert denied["message"] == "Business capability is not connected."
            for tool, arguments in (
                ("qkm_preview_changes", {"action": "configuration.save", "setting_name": "Synthetic"}),
                ("qkm_apply_changes", {"preview_id": "a" * 32}),
                ("qkm_cancel_changes", {"preview_id": "a" * 32}),
                ("qkm_stop_mapping", {"instance": bound["instance"], "request_id": "b" * 32}),
            ):
                await call(client, tool, arguments, error="permission_denied")
            revision = "a" * 64
            tab = revision + ":source:0"
            row = revision + ":row:0:0"
            editor = {"source_ref": row, "presentation": "private_background"}
            settings = {"source_tab_ref": tab, "presentation": "private_background"}
            window = "b" * 64
            await call(client, "qkm_start_mapping", {"instance": bound["instance"], "request_id": "c" * 32,
                "source_revision": revision, "source_tab_ref": tab, "save_policy": "start_only"}, error="permission_denied")
            for tool, arguments in (
                ("qkm_open_mapping_editor", editor),
                ("qkm_close_mapping_editor", {**editor, "window_ref": window}),
                ("qkm_open_mapping_settings", settings),
                ("qkm_close_mapping_settings", {**settings, "window_ref": window}),
                ("qkm_capture_window", {"role": "main", "window_ref": window,
                    "source_revision": revision, "source_tab_ref": tab}),
            ):
                await call(client, tool, arguments, error="permission_denied")
            await fixture.wait(lambda value: value["authenticated"] == 1)
        assert store.load() is not None
        async with Client(rust) as client:
            await call(client, "qkm_connect")
            assert (await connection(client, "connected"))["instance"] == bound["instance"]
            await fixture.wait(lambda value: value["authenticated"] == 2)
            async with Client(rust) as other:
                await call(other, "qkm_connect")
                status = await connection(other, "failed", timeout=7)
                assert status["error"] in ("pipe_busy", "connection_closed", "identity_rejected"), status
            assert (await call(client, "qkm_get_capabilities"))["result"]["scopes"] == 1
            fixture.command("revoke")
            await fixture.wait(lambda value: value["pair_count"] == 0)
            await call(client, "qkm_get_capabilities", error=("connection_closed", "identity_rejected"))
        async with Client(rust) as client:
            await call(client, "qkm_connect")
            await connection(client, "failed")
            assert not fixture.state()["pending"]
        passed("v1 DPAPI credential, Rust reconnect, busy peer and revocation")

        store.forget()
        await asyncio.sleep(1.05)
        async with Client(rust) as client:
            await pending(client, fixture)
            fixture.command("approve")
            assert (await connection(client, "connected"))["scopes"] == 1
        async with Client(rust, mode="legacy") as client:
            await call(client, "qkm_connect")
            assert (await connection(client, "connected"))["instance"] == bound["instance"]
            assert (await call(client, "qkm_get_capabilities"))["result"]["scopes"] == 1
        await fixture.wait(lambda value: value["authenticated"] == 4)
        passed("fresh v1 credential reused by a new legacy stdio session")

        await asyncio.sleep(1.05)
        started = time.monotonic()
        async with Client(rust) as client:
            await pending(client, fixture)
        await fixture.wait(lambda value: not value.get("pending"))
        assert time.monotonic() - started < 5
        assert fixture.state()["authenticated"] == 4
        passed("closing stdio drains a pending native job")

        for mode in ("legacy", "2026-07-28"):
            async with Client(rust, mode=mode) as client:
                await call(client, "qkm_connect")
                connected = await connection(client, "connected")
                assert connected["scopes"] == 1
                await read_wait_states(client, fixture, connected["instance"])
            passed(f"{mode} bounded wait over private IPC, condition timeout, cancellation and transport timeout")
    finally:
        try:
            if store is not None and owns_credential:
                store.forget()
                assert store.load() is None
        finally:
            fixture.close()
    passed("dedicated credential removed, private fixture exit 0")
    (args.output / "result.json").write_text(
        json.dumps({"checks": checks, "desktop_scope": "private", "server_integrity": "medium",
                    "fixture_exe": str(args.test_exe), "rust_exe": str(args.rust_exe)}, indent=2) + "\n",
        encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust-exe", required=True, type=Path)
    parser.add_argument("--test-exe", required=True, type=Path)
    parser.add_argument("--qt-bin", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if sys.flags.optimize:
        parser.error("Run the fixture with assertions enabled.")
    for key in ("rust_exe", "test_exe", "qt_bin", "output"):
        setattr(args, key, getattr(args, key).resolve())
    out = ROOT / "out"
    if out not in args.output.parents or out not in args.test_exe.parents or out not in args.rust_exe.parents:
        parser.error("Use only repository out/ artifacts and a fresh output directory.")
    if args.test_exe.name != "QKeyMapperMcpControlTest.exe":
        parser.error("The approved asInvoker MCP control test target is required.")
    args.output.mkdir(parents=True, exist_ok=False)
    asyncio.run(exercise(args))
