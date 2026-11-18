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
from test_sdk_interop import Fixture, call, connection, elevation, identity, parameters, pending
from qkeymapper_mcp.credentials import CredentialStore

TOOLS = {
    "qkm_get_connection_state", "qkm_connect", "qkm_request_pairing",
    "qkm_disconnect", "qkm_get_capabilities", "qkm_get_state",
}


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
        python = parameters(discovery, args.test_exe, args.output)
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
            await fixture.wait(lambda value: value["authenticated"] == 1)
        assert store.load() is not None
        async with Client(python) as client:
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
            await call(client, "qkm_get_capabilities", error=("connection_closed", "native_operation_failed"))
        async with Client(rust) as client:
            await call(client, "qkm_connect")
            await connection(client, "failed")
            assert not fixture.state()["pending"]
        passed("Rust writes v1 DPAPI credential, Python reauthenticates, busy peer and revocation")

        store.forget()
        await asyncio.sleep(1.05)
        async with Client(python) as client:
            await pending(client, fixture)
            fixture.command("approve")
            assert (await connection(client, "connected"))["scopes"] == 1
        async with Client(rust, mode="legacy") as client:
            await call(client, "qkm_connect")
            assert (await connection(client, "connected"))["instance"] == bound["instance"]
            assert (await call(client, "qkm_get_capabilities"))["result"]["scopes"] == 1
        await fixture.wait(lambda value: value["authenticated"] == 4)
        passed("Python writes v1 DPAPI credential, new Rust process reauthenticates")

        await asyncio.sleep(1.05)
        started = time.monotonic()
        async with Client(rust) as client:
            await pending(client, fixture)
        await fixture.wait(lambda value: not value.get("pending"))
        assert time.monotonic() - started < 5
        assert fixture.state()["authenticated"] == 4
        passed("closing stdio drains a pending native job")
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
