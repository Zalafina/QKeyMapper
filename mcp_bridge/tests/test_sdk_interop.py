"""Official SDK stdio -> Python bridge -> native C++ session -> real Qt GUI events."""

import argparse
import asyncio
import json
import os
import subprocess
import sys
import time
import uuid
from contextlib import closing
from pathlib import Path

import win32api
import win32con
import win32event
import win32file
import win32process
import win32security
import win32service
from mcp import Client, StdioServerParameters

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))
from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.protocol import BridgeError
from qkeymapper_mcp.windows import identity

TOOLS = {"qkm_get_connection_state", "qkm_connect", "qkm_request_pairing", "qkm_disconnect", "qkm_get_capabilities"}


def elevation(process) -> int:
    with closing(win32security.OpenProcessToken(process, win32con.TOKEN_QUERY)) as token:
        return win32security.GetTokenInformation(token, win32security.TokenElevation)


class Fixture:
    def __init__(self, exe: Path, qt: Path, output: Path):
        self.exe, self.output = exe, output
        self.state_path = output / "fixture.json"
        self.sequence = 0
        self.desktop_name = "QkmSdk_" + uuid.uuid4().hex
        self.desktop = win32service.CreateDesktop(self.desktop_name, 0, 511, None)
        env = os.environ.copy()
        env["PATH"] = str(qt) + os.pathsep + env["PATH"]
        startup = win32process.STARTUPINFO()
        startup.lpDesktop = "winsta0\\" + self.desktop_name
        startup.dwFlags = win32process.STARTF_USESTDHANDLES
        sa = win32security.SECURITY_ATTRIBUTES()
        sa.bInheritHandle = True
        try:
            with closing(win32file.CreateFile(str(output / "fixture.log"), win32con.GENERIC_WRITE, 1, sa, 2, 0, None)) as log:
                with closing(win32file.CreateFile("NUL", win32con.GENERIC_READ, 3, sa, 3, 0, None)) as nul:
                    startup.hStdInput, startup.hStdOutput, startup.hStdError = nul, log, log
                    command = subprocess.list2cmdline([str(exe), "--interop-fixture", str(self.state_path)])
                    self.process, thread, self.pid, _ = win32process.CreateProcess(None, command, None, None, True,
                        subprocess.CREATE_NO_WINDOW | win32con.CREATE_UNICODE_ENVIRONMENT, env, str(output), startup)
                    thread.Close()
            assert elevation(self.process) == 0
        except BaseException:
            self.desktop.CloseDesktop()
            raise

    def state(self) -> dict:
        if self.state_path.is_file():
            state = json.loads(self.state_path.read_text(encoding="utf-8"))
            assert state["desktop"] == self.desktop_name and state["pid"] == self.pid
            return state
        return {}

    def command(self, command: str) -> None:
        self.sequence += 1
        path = self.state_path.with_name(self.state_path.name + ".command")
        temporary = path.with_name(path.name + ".new")
        temporary.write_text(json.dumps({"sequence": self.sequence, "command": command}), encoding="utf-8")
        os.replace(temporary, path)

    async def wait(self, predicate, timeout=5) -> dict:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            state = self.state()
            if predicate(state):
                return state
            assert win32event.WaitForSingleObject(self.process, 0) == win32event.WAIT_TIMEOUT, "fixture exited"
            await asyncio.sleep(0.03)
        raise AssertionError("fixture condition timed out")

    def close(self) -> None:
        self.command("quit")
        try:
            if win32event.WaitForSingleObject(self.process, 5000) != win32event.WAIT_OBJECT_0:
                win32process.TerminateProcess(self.process, 99)
                win32event.WaitForSingleObject(self.process, 5000)
                raise AssertionError("owned fixture did not close")
            assert win32process.GetExitCodeProcess(self.process) == 0, "fixture failed"
        finally:
            self.process.Close()
            self.desktop.CloseDesktop()


def parameters(discovery: Path, executable: Path, output: Path) -> StdioServerParameters:
    return StdioServerParameters(command=sys.executable,
        args=[str(ROOT / "run_bridge.py"), "--discovery-file", str(discovery), "--executable", str(executable)],
        cwd=str(output))


async def call(client, name: str) -> dict:
    result = await client.call_tool(name)
    assert not result.is_error, "SDK tool error: " + name
    assert type(result.structured_content) is dict
    return result.structured_content


async def connection(client, status: str, timeout=5) -> dict:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        state = await call(client, "qkm_get_connection_state")
        if state["status"] == status and not state["busy"]:
            return state
        await asyncio.sleep(0.03)
    raise AssertionError("connection did not become " + status + "; last=" + json.dumps(state))


async def pending(client, fixture) -> None:
    result = await call(client, "qkm_request_pairing")
    assert result["accepted"]
    await fixture.wait(lambda state: state.get("pending"))
    state = await call(client, "qkm_get_connection_state")
    assert state["status"] == "awaiting_gui_approval" and state["busy"]
    with closing(win32api.OpenProcess(0x1000, False, fixture.state()["client_pid"])) as process:
        assert elevation(process) == 0


async def run(args):
    assert elevation(win32api.GetCurrentProcess()) == 0
    checks = []

    def passed(name):
        checks.append(name)
        print("PASS", name, flush=True)

    async with Client(parameters(args.output / "absent" / "instance.json", args.test_exe, args.output)) as client:
        assert {tool.name for tool in (await client.list_tools()).tools} == TOOLS
        assert (await call(client, "qkm_get_connection_state"))["status"] == "disconnected"
        assert (await call(client, "qkm_connect"))["accepted"]
        assert (await connection(client, "failed"))["error"] == "application_unavailable"
        passed("SDK initialize/list/call with application absent")

    fixture = Fixture(args.test_exe, args.qt_bin, args.output)
    store = None
    try:
        state = await fixture.wait(lambda state: bool(state.get("enabled")))
        discovery = Path(state["discovery"])
        own = identity(os.getpid())
        store = CredentialStore(discovery, args.test_exe, own.session)
        assert store.load() is None, "fixture credential target must be unique"
        config = parameters(discovery, args.test_exe, args.output)
        async with Client(config) as client:
            await call(client, "qkm_connect")
            await connection(client, "pairing_required")
            assert not fixture.state()["pending"]
            await pending(client, fixture)
            assert fixture.state()["authenticated"] == 0 and fixture.state()["pair_count"] == 0
            assert (await call(client, "qkm_get_capabilities"))["error"] == "busy_or_closed"
            fixture.command("approve")
            connected = await connection(client, "connected")
            assert connected["scopes"] == 1
            await fixture.wait(lambda state: state.get("authenticated") == 1 and state.get("pair_count") == 1)
            response = await call(client, "qkm_get_capabilities")
            assert response["ok"] and response["result"]["stage"] == "transport_only"
            assert response["result"]["business_capabilities"] == []
            assert response["result"]["scopes"] == 1 and response["result"]["instance"] == connected["instance"]
            saved = store.load()
            assert saved is not None and saved.secret.hex() not in json.dumps({"state": connected, "response": response})
            wrong_context = CredentialStore(discovery.with_name("different-instance.json"), args.test_exe, own.session)
            wrong_context.target = store.target
            try:
                wrong_context.load()
                raise AssertionError("wrong credential entropy was accepted")
            except BridgeError as error:
                assert error.code == "credential_rejected"
            assert store.load() == saved
            passed("GUI approval, cross-language proofs, credential save, formal capability RPC")
            await call(client, "qkm_disconnect")
            await call(client, "qkm_disconnect")
            await connection(client, "disconnected")
            passed("idempotent disconnect")

        async with Client(config) as client:
            await call(client, "qkm_connect")
            await connection(client, "connected")
            await fixture.wait(lambda state: state.get("authenticated") == 2)
            assert not fixture.state()["pending"]
            passed("new stdio bridge authenticates from protected stored credential")
            fixture.command("revoke")
            await fixture.wait(lambda state: state.get("pair_count") == 0)
            assert not (await call(client, "qkm_get_capabilities"))["ok"]
            await call(client, "qkm_connect")
            await connection(client, "failed")
            assert not fixture.state()["pending"]
            passed("revocation closes session; reconnect never silently re-pairs")
            await pending(client, fixture)
            fixture.command("reject")
            await connection(client, "failed")
            assert fixture.state()["pair_count"] == 0
            passed("GUI rejection never becomes ready")
            await asyncio.sleep(1.05)
            await pending(client, fixture)
            started = time.monotonic()
            await call(client, "qkm_disconnect")
            await connection(client, "disconnected")
            await fixture.wait(lambda state: not state.get("pending"))
            assert time.monotonic() - started < 3
            passed("cancel pending native read drains and disconnects within 3 seconds")

            await asyncio.sleep(1.05)
            await pending(client, fixture)
            fixture.command("approve")
            await connection(client, "connected")
            await fixture.wait(lambda state: state.get("authenticated") == 3)
            fixture.command("disable")
            await fixture.wait(lambda state: state.get("enabled") is False and state.get("pair_count") == 1)
            assert not discovery.exists()
            assert not (await call(client, "qkm_get_capabilities"))["ok"]
            fixture.command("revoke")
            await fixture.wait(lambda state: state.get("enabled") is False and state.get("pair_count") == 0)
            assert not discovery.exists()
            fixture.command("enable")
            await fixture.wait(lambda state: state.get("enabled") is True)
            await call(client, "qkm_connect")
            await connection(client, "failed")
            assert not fixture.state()["pending"]
            passed("GUI revokes while disabled; old key rejected after re-enable without opening access to revoke")

        await asyncio.sleep(1.05)
        started = time.monotonic()
        async with Client(config) as client:
            await pending(client, fixture)
        await fixture.wait(lambda state: not state.get("pending"))
        assert time.monotonic() - started < 5
        passed("stdio shutdown cancels pending GUI approval without leaving a session")

        async with Client(parameters(discovery, Path(sys.executable), args.output)) as client:
            await call(client, "qkm_request_pairing")
            assert (await connection(client, "failed"))["error"] == "executable_rejected"
            assert not fixture.state()["pending"]
            passed("actual process image rejects an explicitly wrong expected EXE")

        private = args.output / "private-hint"
        sa = win32security.SECURITY_ATTRIBUTES()
        sa.SECURITY_DESCRIPTOR = win32security.ConvertStringSecurityDescriptorToSecurityDescriptor(
            "O:" + own.user + "D:P(A;OICI;FA;;;" + own.user + ")", 1)
        win32file.CreateDirectory(str(private), sa)
        fake = private / "instance.json"
        original = json.loads(discovery.read_text(encoding="utf-8"))
        for key, value, expected in (("created", str(int(original["created"]) + 1), "identity_rejected"),
            ("executable", r"\\untrusted-host\share\program.exe", "executable_rejected"),
            ("executable", original["executable"] + "\0", "invalid_discovery"),
            ("unexpected", True, "invalid_message")):
            with closing(win32file.CreateFile(str(fake), win32con.GENERIC_WRITE, 1, sa, 2, 0, None)) as handle:
                win32file.WriteFile(handle, json.dumps({**original, key: value}).encode("utf-8"))
            async with Client(parameters(fake, args.test_exe, args.output)) as client:
                await call(client, "qkm_request_pairing")
                assert (await connection(client, "failed"))["error"] == expected
                assert not fixture.state()["pending"]
        passed("private but stale/tampered discovery is rejected before a pairing request")

        if args.approval_timeout:
            await asyncio.sleep(1.05)
            async with Client(config) as client:
                await pending(client, fixture)
                started = time.monotonic()
                await connection(client, "failed", timeout=65)
                assert 58 <= time.monotonic() - started <= 65
                assert fixture.state()["pair_count"] == 0
                passed("real 60-second GUI approval timeout rejects authorization")

        fixture.command("corrupt-store")
        await fixture.wait(lambda state: state.get("sequence") == fixture.sequence)
        async with Client(config) as client:
            await pending(client, fixture)
            fixture.command("approve")
            await connection(client, "failed")
            assert store.load() is not None
            assert fixture.state()["pair_count"] == 0 and fixture.state()["authenticated"] == 3
            assert discovery.with_name("pairings.dat").read_bytes() == b"invalid"
            passed("server persistence failure cannot become ready despite a saved client key")
    finally:
        try:
            if store is not None:
                store.forget()
                assert store.load() is None
        finally:
            fixture.close()
    (args.output / "results.json").write_text(json.dumps({"checks": checks, "elevated": 0,
        "desktop": fixture.desktop_name, "test_exe": str(args.test_exe), "sdk": "mcp 2.3.0"}, indent=2), encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--test-exe", type=Path, required=True)
    parser.add_argument("--qt-bin", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--approval-timeout", action="store_true")
    args = parser.parse_args()
    if not __debug__:
        parser.error("Run without Python -O.")
    args.test_exe, args.qt_bin, args.output = (p.resolve() for p in (args.test_exe, args.qt_bin, args.output))
    if not args.test_exe.is_file() or args.test_exe.name != "QKeyMapperMcpControlTest.exe":
        parser.error("Select the built maintenance MCP control executable.")
    if not args.qt_bin.is_dir() or not args.output.is_relative_to((ROOT.parent / "out").resolve()):
        parser.error("Select the matching Qt bin and an output directory under this checkout out/.")
    args.output.mkdir(parents=True, exist_ok=False)
    asyncio.run(run(args))
