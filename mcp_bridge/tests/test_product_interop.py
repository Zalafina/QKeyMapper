"""Authorized stock-product GUI -> official SDK -> ordinary stdio bridge acceptance."""

import argparse
import asyncio
import hashlib
import json
import os
import re
import shutil
import subprocess
import time
import uuid
from pathlib import Path
from contextlib import closing

import win32api
import win32con
import win32event
import win32file
import win32process
import win32security
import win32service
from mcp import Client

from test_sdk_interop import ROOT, call, close_desktop, connection, elevation, integrity, parameters
from qkeymapper_mcp.bridge import Bridge
from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.windows import identity


class Product:
    def __init__(self, executable: Path, output: Path):
        self.executable, self.output = executable, output
        self.sequence = 0
        self.snapshots_available = True
        self.closed = False
        self.desktop_name = "QkmProduct_" + uuid.uuid4().hex
        self.desktop = win32service.CreateDesktop(self.desktop_name, 0, 511, None)
        self.log = (output / "lifecycle.log").open("wb")
        self.guardian = subprocess.Popen([shutil.which("pwsh.exe"), "-NoProfile", "-File",
            str(ROOT.parent / "QKeyMapper/tests/mcp_control/product_lifecycle.ps1"),
            "-ExecutablePath", str(executable), "-DesktopName", self.desktop_name, "-OutputDirectory", str(output)],
            stdout=self.log, stderr=self.log, creationflags=subprocess.CREATE_NO_WINDOW)
        self.process = None

    async def started(self):
        await self.wait(lambda: (self.output / "started.json").exists(), 85)
        self.started_record = json.loads((self.output / "started.json").read_text(encoding="utf-8"))
        self.pid = self.started_record["pid"]
        self.process = win32api.OpenProcess(0x1000 | win32con.SYNCHRONIZE, False, self.pid)
        server, own = identity(self.pid), identity(os.getpid())
        assert server.created == int(self.started_record["created"]) and Path(server.executable) == self.executable
        assert own.same_login(server) and elevation(self.process) == 1 and integrity(self.process) == 0x3000
        self.window = self.started_record["window"]
        assert self.started_record["desktop"] == self.desktop_name
        (self.output / "topology.json").write_text(json.dumps({"pid": self.pid,
            "created": str(server.created), "executable": str(self.executable), "desktop": self.desktop_name,
            "server_elevated": 1, "server_integrity": 0x3000, "controller_elevated": 0,
            "controller_integrity": integrity(win32api.GetCurrentProcess()), "same_login": True}, indent=2), encoding="utf-8")

    async def wait(self, predicate, timeout=5):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if predicate():
                return
            assert self.guardian.poll() is None, "product lifecycle failed; see lifecycle.log"
            if self.process is not None:
                assert win32event.WaitForSingleObject(self.process, 0) == win32event.WAIT_TIMEOUT, "product exited"
            await asyncio.sleep(0.05)
        raise AssertionError("product condition timed out")

    async def action(self, name, client_executable=None):
        self.sequence += 1
        prefix = str(self.sequence).zfill(2) + "_" + name
        command = [shutil.which("gsudo.exe"), "--direct", shutil.which("pwsh.exe"), "-NoProfile", "-STA", "-File",
            str(ROOT.parent / "QKeyMapper/tests/mcp_control/product_ui_actions.ps1"), "-Action", name,
            "-TargetProcessId", str(self.pid), "-ExecutablePath", str(self.executable),
            "-StartTicks", self.started_record["start_ticks"], "-DesktopName", self.desktop_name,
            "-ResultFile", str(self.output / (prefix + ".json"))]
        if client_executable:
            command += ["-ClientExecutable", client_executable]
        with (self.output / (prefix + ".log")).open("wb") as log:
            process = await asyncio.create_subprocess_exec(*command, stdout=log, stderr=log,
                                                         creationflags=subprocess.CREATE_NO_WINDOW)
            assert await asyncio.wait_for(process.wait(), 60) == 0, name + " failed; see action log"
        data = json.loads((self.output / (prefix + ".json")).read_text(encoding="utf-8"))
        assert data["success"] is True and data["action"] == name
        assert data["desktop"] == self.desktop_name and data["window"] == self.window
        return data

    async def capture(self, window, label):
        if not self.snapshots_available:
            return
        # Capture remains Medium; STARTUPINFO selects a private desktop without changing the input desktop.
        startup = win32process.STARTUPINFO()
        startup.lpDesktop = "winsta0\\" + self.desktop_name
        startup.dwFlags = win32process.STARTF_USESTDHANDLES
        attributes = win32security.SECURITY_ATTRIBUTES()
        attributes.bInheritHandle = True
        command = [shutil.which("pwsh.exe"), "-NoProfile", "-STA", "-File",
            str(ROOT.parent / "scripts/capture_ui_snapshots.ps1"), "-ProcessId", str(self.pid),
            "-WindowHandle", str(window), "-PrintWindow", "-Label", self.desktop_name + "_" + label]
        with closing(win32file.CreateFile(str(self.output / (label + ".log")), win32con.GENERIC_WRITE, 1,
                                         attributes, win32con.CREATE_ALWAYS, 0, None)) as log:
            with closing(win32file.CreateFile("NUL", win32con.GENERIC_READ, 3, attributes, 3, 0, None)) as nul:
                startup.hStdInput, startup.hStdOutput, startup.hStdError = nul, log, log
                process, thread, _, _ = win32process.CreateProcess(None, subprocess.list2cmdline(command), None, None,
                    True, subprocess.CREATE_NO_WINDOW | win32con.CREATE_UNICODE_ENVIRONMENT,
                    dict(os.environ), str(self.output), startup)
                thread.Close()
        try:
            assert elevation(process) == 0
            assert await asyncio.to_thread(win32event.WaitForSingleObject, process, 25000) == win32event.WAIT_OBJECT_0
            code = win32process.GetExitCodeProcess(process)
            if code != 0:
                diagnostic = (self.output / (label + ".log")).read_text(encoding="utf-8-sig", errors="replace")
                assert "PrintWindow failed; no screenshot was saved." in diagnostic, "ordinary snapshot failed; see capture log"
                self.snapshots_available = False
                print("UNVERIFIED: ordinary PrintWindow cannot capture this High product; capture was not elevated.", flush=True)
                return
        finally:
            process.Close()
        source = ROOT.parent / "test_snapshots" / ("snapshot_" + self.desktop_name + "_" + label + ".png")
        shutil.copyfile(source, self.output / (label + ".png"))

    async def close(self):
        if self.closed:
            return
        (self.output / "stop").touch()
        try:
            code = await asyncio.to_thread(self.guardian.wait, 80)
            assert code == 0, "product did not close; see lifecycle.log"
            closed = json.loads((self.output / "closed.json").read_text(encoding="utf-8"))
            assert closed["exit_code"] == 0
        finally:
            self.closed = True
            if self.process is not None:
                self.process.Close()
            self.log.close()
            close_desktop(self.desktop)


async def run(args):
    assert elevation(win32api.GetCurrentProcess()) == 0 and integrity(win32api.GetCurrentProcess()) == 0x2000
    checks = []
    def passed(name):
        checks.append(name)
        print("PASS", name, flush=True)
    product, store = Product(args.executable, args.output), None
    first_desktop = product.desktop_name
    try:
        await product.started()
        await product.capture(product.window, "main")
        baseline = await product.action("OpenAdvanced")
        initial = await product.action("OpenMcp")
        assert initial["enabled"] is False and initial["discovery"] == "" and initial["pair_count"] == 0, "use an unused product namespace; interface must default to disabled"
        mcp_window = initial["dialog_window"]
        await product.capture(mcp_window, "disabled")
        enabled = await product.action("Enable")
        discovery = Path(enabled["discovery"])
        own = identity(os.getpid())
        binding = hashlib.sha256(("QKeyMapper-MCP-App/v1\0" + args.executable.as_posix() + "\0" + str(own.session)).encode("utf-8")).hexdigest()
        assert enabled["enabled"] is True and discovery.name == "instance.json"
        assert discovery.parent.name == binding and discovery.parent.parent.name == "MCP", "actual product application binding differs"
        await product.wait(discovery.is_file)
        store = CredentialStore(discovery, args.executable, own.session)
        assert store.load() is None, "use an unused, isolated product runtime"
        config = parameters(discovery, args.executable, args.output)
        async with Client(config) as client:
            await call(client, "qkm_connect")
            await connection(client, "pairing_required")
            passed("stock High product General Advanced/MCP entry and ordinary discovery")
            await call(client, "qkm_request_pairing")
            await connection_pending(client)
            assert not (await call(client, "qkm_get_capabilities"))["ok"]
            await product.capture(mcp_window, "pending")
            await product.action("ApproveRead", own.executable)
            state = await connection(client, "connected")
            assert state["scopes"] == 1 and store.load() is not None
            result = await call(client, "qkm_get_capabilities")
            assert result["ok"] and result["result"]["stage"] == "business"
            capabilities = result["result"]["business_capabilities"]
            assert len(capabilities) == 1 and capabilities[0]["id"] == "state.read"
            assert capabilities[0]["method"] == "qkm_get_state" and capabilities[0]["read_only"] is True
            assert capabilities[0]["arguments_schema"]["additionalProperties"] is False
            summary = (await call(client, "qkm_get_state"))["result"]
            (args.output / "state_summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
            assert (await client.call_tool("qkm_get_state", {"unknown": True})).is_error
            assert summary["schema_version"] == 1 and summary["instance"] == state["instance"]
            assert summary["business_revision"] is None
            assert summary["configuration"] == {"group": "", "kind": "draft", "has_unsaved_changes": False}
            assert summary["current_tab"] == {"name": "Tab1", "index": 0, "is_common": False}
            assert summary["run"]["gui_state"] == "idle" and summary["run"]["monitoring_enabled"] is False
            assert summary["run"]["worker_state_sample"] == "stopped"
            assert summary["run"]["stop_completion"] == "unobserved"
            assert summary["run"]["actual_stop_shortcut"] == baseline["stop_shortcut_edit"]
            assert baseline["mapping_button_text"] == "映射开始"
            assert (await call(client, "qkm_get_state"))["result"] == summary
            passed("read-only business state matches independent GUI stop shortcut and idle button; cleanup remains unobserved")
            await product.capture(mcp_window, "paired")
            await product.action("CloseMcp")
            assert (await call(client, "qkm_get_capabilities"))["ok"]
            passed("GUI read-only approval, protected credential, formal RPC and close-window keeps interface")
        probe = Bridge(discovery, args.executable)
        try:
            assert probe.connect()["accepted"]
            await product.wait(lambda: probe.state()["status"] == "connected" and not probe.state()["busy"])
            # Negative native requests supplement the formal MCP path; they cannot bypass product validation.
            def invalid_arguments(method):
                request_id = uuid.uuid4().hex
                deadline = time.monotonic() + 5
                probe._pipe.send({"id": request_id, "method": method, "arguments": {"unknown": True}}, deadline)
                return request_id, probe._pipe.receive(deadline)
            for method in ("qkm_get_state", "qkm_get_capabilities"):
                request_id, response = await asyncio.wrap_future(probe._submit(lambda: invalid_arguments(method)))
                assert response["id"] == request_id and response["ok"] is False
                assert response["error"]["code"] == "invalid_arguments"
            assert (await probe.get_state())["result"] == summary
            passed("authenticated product rejects unknown native arguments without changing state or closing the session")
        finally:
            await asyncio.to_thread(probe.close)
        async with Client(config) as client:
            await call(client, "qkm_connect")
            await connection(client, "connected")
            passed("new ordinary stdio bridge reconnects to actual product using its saved credential")
            await product.action("OpenAdvanced")
            await product.action("OpenMcp")
            await product.action("Revoke")
            assert not (await call(client, "qkm_get_capabilities"))["ok"]
            await call(client, "qkm_connect")
            await connection(client, "failed")
            passed("actual GUI revocation disconnects and rejects the old key")
            await product.action("Disable")
            await product.wait(lambda: not discovery.exists())
            await product.capture(mcp_window, "revoked_disabled")
            passed("actual GUI disable removes discovery")
        snapshots_available = product.snapshots_available
        await product.close()
        restart_output = args.output / "cold_restart"
        restart_output.mkdir()
        product = Product(args.executable, restart_output)
        await product.started()
        await product.action("OpenAdvanced")
        restored = await product.action("OpenMcp")
        assert restored["enabled"] is False and restored["discovery"] == "" and restored["pair_count"] == 0
        assert (await product.action("Enable"))["discovery"] == str(discovery).replace("\\", "/")
        async with Client(config) as client:
            assert store.load() is not None
            await call(client, "qkm_connect")
            await connection(client, "failed")
        await product.action("Disable")
        await product.wait(lambda: not discovery.exists())
        passed("cold product restart stays disabled, restores revocation and rejects the retained old client key")
    finally:
        try:
            if store is not None:
                store.forget()
                assert store.load() is None
        finally:
            await product.close()
    (args.output / "results.json").write_text(json.dumps({"checks": checks, "desktop": first_desktop,
        "restart_desktop": product.desktop_name,
        "executable": str(args.executable), "server_elevated": 1, "controller_elevated": 0,
        "stock_product_snapshots": snapshots_available}, indent=2), encoding="utf-8")


async def connection_pending(client):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        state = await call(client, "qkm_get_connection_state")
        if state["status"] == "awaiting_gui_approval" and state["busy"]:
            return
        await asyncio.sleep(0.05)
    raise AssertionError("pairing did not reach the actual GUI approval state")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.executable, args.output = args.executable.resolve(), args.output.resolve()
    base = (ROOT.parent / "out/mcp_product").resolve()
    if not __debug__ or not args.executable.is_relative_to(base) or not args.executable.is_file() or not re.fullmatch(
            r"QKeyMapperMcpAcceptance_[a-zA-Z0-9_]+\.exe", args.executable.name):
        parser.error("Select an isolated, deployed stock product under out/mcp_product and run without -O.")
    if not args.output.is_relative_to(base) or args.output == base or not shutil.which("gsudo.exe") or not shutil.which("pwsh.exe"):
        parser.error("Select a new output directory under out/mcp_product; gsudo and PowerShell 7 are required.")
    args.output.mkdir(parents=True, exist_ok=False)
    asyncio.run(run(args))
