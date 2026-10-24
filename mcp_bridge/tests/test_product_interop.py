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
    def __init__(self, executable: Path, output: Path, *, default_desktop=False):
        self.executable, self.output = executable, output
        self.sequence = 0
        self.snapshots_available = True
        self.closed = False
        self.default_desktop = default_desktop
        if default_desktop:
            base = (ROOT.parent / "out/mcp_product/engine61").resolve()
            assert executable.is_relative_to(base) and output.is_relative_to(base)
            assert executable.name in ("QKeyMapperMcpAcceptance_engine61_baseline.exe",
                                       "QKeyMapperMcpAcceptance_engine61_current.exe")
        self.desktop_name = "Default" if default_desktop else "QkmProduct_" + uuid.uuid4().hex
        self.desktop = None if default_desktop else win32service.CreateDesktop(self.desktop_name, 0, 511, None)
        self.log = (output / "lifecycle.log").open("wb")
        self.guardian = subprocess.Popen([shutil.which("pwsh.exe"), "-NoProfile", "-File",
            str(ROOT.parent / "QKeyMapper/tests/mcp_control/product_lifecycle.ps1"),
            "-ExecutablePath", str(executable), "-DesktopName", self.desktop_name, "-OutputDirectory", str(output)] +
            (["-AllowDefaultDesktop"] if default_desktop else []),
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
        if self.default_desktop:
            command += ["-AllowDefaultDesktop"]
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
            if self.desktop is not None:
                close_desktop(self.desktop)


async def warm_loads(product, client, summary, catalog_revision, passed):
    await product.action("CloseMcp")
    stages = (("ReadMapping", "McpSaved", "accepted", "McpSaved", "A", "B"),
        ("SelectSecond", "McpSecond", "accepted", "McpSecond", "C", "D"),
        ("SelectBlank", "", "accepted", "", None, None),
        ("SelectSaved", "McpSaved", "accepted", "McpSaved", "A", "B"),
        ("SelectInvalid", "", "invalid_mapping_data", None, None, None),
        ("SelectSaved", "McpSaved", "accepted", "McpSaved", "A", "B"),
        ("ReloadCurrent", "McpSaved", "accepted", "McpSaved", "A", "B"))
    generation = int(summary["mapping_load_context"]["generation"])
    previous_state = summary
    for index, (action, selected_group, outcome, loaded_group, original, mapping) in enumerate(stages):
        gui = await product.action(action)
        if index:
            generation += 1
        ini = product.executable.parent / "keymapdata.ini"
        before_read = ini.read_bytes()
        state = (await call(client, "qkm_get_state"))["result"]
        tabs = (await call(client, "qkm_query", {"entity": "tabs", "limit": 1}))["result"]
        revision = tabs["catalog"]["revision"]
        configs = (await call(client, "qkm_query", {"entity": "configurations",
            "catalog_revision": revision}))["result"]
        stale = {}
        if index:
            for entity in ("tabs", "configurations"):
                stale[entity] = await call(client, "qkm_query", {"entity": entity,
                    "catalog_revision": catalog_revision, "offset": 1, "limit": 1}, error="stale_catalog")
        last = (await call(client, "qkm_query", {"entity": "tabs", "catalog_revision": revision,
            "offset": 1, "limit": 1}))["result"]
        repeated = (await call(client, "qkm_get_state"))["result"]
        gui_after = await product.action("ReadMapping")
        after_read = ini.read_bytes()
        record = {"gui": gui, "state": state, "tabs": tabs, "configurations": configs,
            "stale_requests": stale, "last_page": last, "repeated_state": repeated,
            "gui_after_reads": gui_after, "ini_unchanged_by_reads": before_read == after_read}
        (product.output / ("load_" + str(index) + "_" + action + ".json")).write_text(
            json.dumps(record, ensure_ascii=False, indent=2), encoding="utf-8")
        assert state["instance"] == summary["instance"] and state["business_revision"] is None
        assert state["mapping_load_context"] == {"generation": str(generation), "scope": "last_mapping_load",
            "outcome": outcome, "group": loaded_group}
        if action == "ReloadCurrent":
            unchanged = dict(state)
            unchanged["mapping_load_context"] = dict(state["mapping_load_context"],
                generation=previous_state["mapping_load_context"]["generation"])
            assert unchanged == previous_state, "Same-content reload changed other business summary fields"
        assert state["configuration"]["group"] == selected_group
        assert state["configuration"]["kind"] == ("saved_profile" if selected_group else "draft")
        assert type(state["configuration"]["has_unsaved_changes"]) is bool
        assert gui["configuration_display"] == selected_group and gui["configuration_read_method"] == "ValuePattern"
        expected_rows = [] if original is None else [{"original": original, "mapping": mapping}]
        assert gui["mapping_rows"] == expected_rows and gui["mapping_row_count"] == len(expected_rows)
        assert gui["mapping_table_read_method"] == "GridPattern.Item.Name"
        assert gui["mapping_button_text"] == "映射开始"
        assert state["run"] == {"gui_state": "idle", "monitoring_enabled": False,
            "worker_state_sample": "stopped", "stop_completion": "unobserved",
            "actual_stop_shortcut": gui["stop_shortcut_edit"]}
        assert state["current_tab"] == {"name": "Tab1", "index": 0, "is_common": False}
        for catalog in (tabs, configs):
            assert catalog["catalog"]["selected_configuration"] == state["configuration"]
            assert catalog["catalog"]["mapping_load_context"] == state["mapping_load_context"]
        assert tabs["total"] == 2 and tabs["next_offset"] == 1
        assert tabs["items"][0]["name"] == "Tab1" and tabs["items"][0]["is_current"]
        assert last["next_offset"] is None and last["items"][0]["is_common"]
        assert not last["items"][0]["attached_to_ui"] and last["items"][0]["index"] is None
        assert [item["group"] for item in configs["items"]] == ["", "QKeyMapperGlobalSetting", "McpInvalid", "McpSaved", "McpSecond"]
        assert [item["display_name"] for item in configs["items"] if item["is_selected"]] == [gui["configuration_display"]]
        if index:
            assert gui["gui_event_method"] in ("owned_UIA_expand_Up_Down_Return", "owned_window_combo_left_click_Up_Down_Return")
            assert gui["requested_configuration"] == ("McpInvalid" if action == "SelectInvalid" else selected_group)
            assert revision != catalog_revision
            assert all(result["ok"] is False and result["error"] == "stale_catalog" for result in stale.values())
        else:
            assert state == summary and revision == catalog_revision
        assert repeated == state and before_read == after_read
        for field in ("configuration_display", "mapping_rows", "mapping_row_count", "mapping_button_text", "stop_shortcut_edit"):
            assert gui_after[field] == gui[field], field + " changed after read-only MCP requests"
        passed("warm GUI " + str(index) + " " + action + ": actual rows, load context, catalog validity and read-only invariance")
        catalog_revision = revision
        previous_state = state
    await product.action("OpenAdvanced")
    await product.action("OpenMcp")


async def run(args):
    assert elevation(win32api.GetCurrentProcess()) == 0 and integrity(win32api.GetCurrentProcess()) == 0x2000
    checks = []
    def passed(name):
        checks.append(name)
        print("PASS", name, flush=True)
    expected_group = "McpSaved" if args.load_fixture in ("saved", "warm") else ""
    expected_outcome = "invalid_mapping_data" if args.load_fixture == "invalid" else "accepted"
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
            await call(client, "qkm_get_capabilities", error="busy_or_closed")
            await product.capture(mcp_window, "pending")
            await product.action("ApproveRead", own.executable)
            state = await connection(client, "connected")
            assert state["scopes"] == 1 and store.load() is not None
            result = await call(client, "qkm_get_capabilities")
            assert result["ok"] and result["result"]["stage"] == "business"
            capabilities = result["result"]["business_capabilities"]
            by_id = {item["id"]: item for item in capabilities}
            assert set(by_id) == {"state.read", "catalog.read", "source.read"}
            assert by_id["state.read"]["method"] == "qkm_get_state" and by_id["state.read"]["read_only"] is True
            assert by_id["state.read"]["arguments_schema"]["additionalProperties"] is False
            assert by_id["catalog.read"]["method"] == "qkm_query" and by_id["catalog.read"]["read_only"] is True
            summary = (await call(client, "qkm_get_state"))["result"]
            (args.output / "state_summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
            assert (await client.call_tool("qkm_get_state", {"unknown": True})).is_error
            assert summary["schema_version"] == 1 and summary["instance"] == state["instance"]
            assert summary["business_revision"] is None
            assert summary["configuration"] == {"group": expected_group,
                "kind": "saved_profile" if expected_group else "draft", "has_unsaved_changes": False}
            assert summary["mapping_load_context"] == {"generation": "1", "scope": "last_mapping_load",
                "outcome": expected_outcome, "group": None if args.load_fixture == "invalid" else expected_group}
            assert summary["current_tab"] == {"name": "Tab1", "index": 0, "is_common": False}
            assert summary["current_tab"]["name"] == baseline["mapping_tab_name"]
            assert summary["run"]["gui_state"] == "idle" and summary["run"]["monitoring_enabled"] is False
            assert summary["run"]["worker_state_sample"] == "stopped"
            assert summary["run"]["stop_completion"] == "unobserved"
            assert summary["run"]["actual_stop_shortcut"] == baseline["stop_shortcut_edit"]
            assert baseline["mapping_button_text"] == "映射开始"
            assert (await call(client, "qkm_get_state"))["result"] == summary
            passed("read-only business state matches independent GUI stop shortcut and idle button; cleanup remains unobserved")
            passed("mapping load context records the resolved load outcome independently of GUI selection: " + args.load_fixture)
            catalog = (await call(client, "qkm_query", {"entity": "tabs", "limit": 1}))["result"]
            assert catalog["schema_version"] == 1 and catalog["entity"] == "tabs"
            assert catalog["catalog"]["reference_scope"] == "catalog_only"
            assert catalog["catalog"]["selected_configuration"] == summary["configuration"]
            assert catalog["catalog"]["mapping_load_context"] == summary["mapping_load_context"]
            assert catalog["total"] == 2 and catalog["next_offset"] == 1 and len(catalog["items"]) == 1
            assert catalog["items"][0]["name"] == baseline["mapping_tab_name"]
            assert catalog["items"][0]["is_current"] and not catalog["items"][0]["is_common"]
            catalog_revision = catalog["catalog"]["revision"]
            last = (await call(client, "qkm_query", {"entity": "tabs", "catalog_revision": catalog_revision,
                "offset": 1, "limit": 1}))["result"]
            assert last["next_offset"] is None and len(last["items"]) == 1
            assert last["items"][0]["is_common"] and not last["items"][0]["attached_to_ui"]
            assert last["items"][0]["index"] is None
            whole = (await call(client, "qkm_query", {"entity": "tabs", "catalog_revision": catalog_revision}))["result"]
            assert whole["items"] == catalog["items"] + last["items"]
            configs = (await call(client, "qkm_query", {"entity": "configurations", "catalog_revision": catalog_revision}))["result"]
            expected_groups = ["", "QKeyMapperGlobalSetting"]
            if args.load_fixture == "warm":
                expected_groups += ["McpInvalid", "McpSaved", "McpSecond"]
            elif args.load_fixture in ("saved", "invalid"):
                expected_groups.append("McpSaved" if args.load_fixture == "saved" else "McpInvalid")
            assert configs["total"] == len(expected_groups) and configs["next_offset"] is None
            assert [item["group"] for item in configs["items"]] == expected_groups
            assert configs["items"][0]["kind"] == "draft"
            selected = [item for item in configs["items"] if item["is_selected"]]
            assert len(selected) == 1 and selected[0]["group"] == expected_group
            assert baseline["configuration_read_method"] == "ValuePattern"
            assert selected[0]["display_name"] == baseline["configuration_display"]
            assert configs["items"][1]["kind"] == "global" and not configs["items"][1]["is_selected"]
            fresh = (await call(client, "qkm_query", {"entity": "tabs"}))["result"]
            assert fresh["catalog"]["revision"] == catalog_revision and fresh["items"] == whole["items"]
            wrong_revision = ("0" if catalog_revision[0] != "0" else "1") + catalog_revision[1:]
            await call(client, "qkm_query", {"entity": "tabs", "catalog_revision": wrong_revision}, error="stale_catalog")
            await call(client, "qkm_query", {"entity": "tabs", "offset": 1}, error="invalid_arguments")
            assert (await call(client, "qkm_get_state"))["result"] == summary
            (args.output / "catalog.json").write_text(json.dumps({"tabs": whole, "configurations": configs}, ensure_ascii=False, indent=2), encoding="utf-8")
            passed("formal catalog pagination matches independent GUI names, separates detached common metadata and validates stable revisions")
            await product.capture(mcp_window, "paired")
            await product.action("CloseMcp")
            assert (await call(client, "qkm_get_capabilities"))["ok"]
            passed("GUI read-only approval, protected credential, formal RPC and close-window keeps interface")
        probe = Bridge(discovery, args.executable)
        try:
            assert probe.connect()["accepted"]
            await product.wait(lambda: probe.state()["status"] == "connected" and not probe.state()["busy"])
            # Negative native requests supplement the formal MCP path; they cannot bypass product validation.
            def invalid_arguments(method, arguments):
                request_id = uuid.uuid4().hex
                deadline = time.monotonic() + 5
                probe._pipe.send({"id": request_id, "method": method, "arguments": arguments}, deadline)
                return request_id, probe._pipe.receive(deadline)
            for method, arguments, expected in (("qkm_get_state", {"unknown": True}, "invalid_arguments"),
                    ("qkm_get_capabilities", {"unknown": True}, "invalid_arguments"),
                    ("qkm_query", {"entity": "tabs", "unknown": True}, "invalid_arguments"),
                    ("qkm_query", {"entity": "tabs", "limit": True}, "invalid_arguments"),
                    ("qkm_query", {"entity": "tabs", "limit": 1.5}, "invalid_arguments"),
                    ("qkm_query", {"entity": "tabs", "offset": 1}, "invalid_arguments"),
                    ("qkm_query", {"entity": "mappings"}, "invalid_arguments"),
                    ("qkm_query", {"entity": "tabs", "catalog_revision": wrong_revision}, "stale_catalog")):
                raw = await asyncio.wrap_future(probe._submit(lambda: invalid_arguments(method, arguments)))
                assert isinstance(raw, tuple), raw
                request_id, response = raw
                assert response["id"] == request_id and response["ok"] is False
                assert response["error"]["code"] == expected
            assert (await probe.get_state())["result"] == summary
            native_catalog = await probe.query("tabs", catalog_revision=catalog_revision)
            assert native_catalog["ok"] and native_catalog["result"]["items"] == whole["items"]
            passed("authenticated product rejects unknown native arguments without changing state or closing the session")
        finally:
            await asyncio.to_thread(probe.close)
        async with Client(config) as client:
            await call(client, "qkm_connect")
            await connection(client, "connected")
            passed("new ordinary stdio bridge reconnects to actual product using its saved credential")
            assert (await call(client, "qkm_query", {"entity": "tabs", "catalog_revision": catalog_revision}))["ok"]
            await product.action("OpenAdvanced")
            await product.action("OpenMcp")
            await product.action("Revoke")
            await call(client, "qkm_get_capabilities", error=("connection_closed", "native_operation_failed", "not_connected"))
            await call(client, "qkm_connect")
            await connection(client, "failed")
            passed("actual GUI revocation disconnects and rejects the old key")
            await call(client, "qkm_request_pairing")
            await connection_pending(client)
            await product.action("ApproveRead", own.executable)
            await connection(client, "connected")
            await call(client, "qkm_query", {"entity": "tabs", "catalog_revision": catalog_revision}, error="stale_catalog")
            new_revision = (await call(client, "qkm_query", {"entity": "tabs"}))["result"]["catalog"]["revision"]
            assert new_revision != catalog_revision
            assert (await call(client, "qkm_get_state"))["result"] == summary
            passed("old catalog revision cannot cross an explicit new GUI pairing approval")
            if args.load_fixture == "warm":
                await warm_loads(product, client, summary, new_revision, passed)
            await product.action("Revoke")
            await call(client, "qkm_connect")
            await connection(client, "failed")
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
        "load_fixture": args.load_fixture,
        "gsudo_cache": args.gsudo_cache,
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
    parser.add_argument("--load-fixture", choices=("blank", "saved", "invalid", "missing", "warm"), default="blank",
                        help="Prepare a synthetic startup configuration before launching the owned product.")
    parser.add_argument("--gsudo-cache", action="store_true",
                        help="Authorized test-controller PID cache only; five-minute idle timeout, closed in finally.")
    args = parser.parse_args()
    args.executable, args.output = args.executable.resolve(), args.output.resolve()
    base = (ROOT.parent / "out/mcp_product").resolve()
    if not __debug__ or not args.executable.is_relative_to(base) or not args.executable.is_file() or not re.fullmatch(
            r"QKeyMapperMcpAcceptance_[a-zA-Z0-9_]+\.exe", args.executable.name):
        parser.error("Select an isolated, deployed stock product under out/mcp_product and run without -O.")
    if not args.output.is_relative_to(base) or args.output == base or not shutil.which("gsudo.exe") or not shutil.which("pwsh.exe"):
        parser.error("Select a new output directory under out/mcp_product; gsudo and PowerShell 7 are required.")
    args.output.mkdir(parents=True, exist_ok=False)
    if args.load_fixture != "blank":
        ini = args.executable.parent / "keymapdata.ini"
        template = (ROOT.parent / "QKeyMapper/tests/mcp_control/product_fixture.ini.example").read_text(encoding="utf-8")
        assert ini.read_text(encoding="utf-8") == template, "load fixtures require a fresh runtime with the unchanged synthetic INI"
        group = {"saved": "McpSaved", "invalid": "McpInvalid", "missing": "McpMissing", "warm": "McpSaved"}[args.load_fixture]
        contents = template.replace("[General]\n", "[General]\nSettingSelect=" + group + "\n", 1)
        if args.load_fixture != "missing":
            original = "A" if args.load_fixture in ("saved", "warm") else "MCP_INVALID_ORIGINAL"
            contents += ("\n[" + group + "]\nMappingTable_TabNameList=Tab1\nKeyMapData_OriginalKeys=" + original
                + "\nKeyMapData_MappingKeys=B\nEnableSystemFilterKeyChecked=false\nDisableFilterKeyClickSoundChecked=false\n")
        if args.load_fixture == "warm":
            for group, original, mapping in (("McpSecond", "C", "D"), ("McpInvalid", "MCP_INVALID_ORIGINAL", "B")):
                contents += ("\n[" + group + "]\nMappingTable_TabNameList=Tab1\nKeyMapData_OriginalKeys=" + original
                    + "\nKeyMapData_MappingKeys=" + mapping + "\nEnableSystemFilterKeyChecked=false\nDisableFilterKeyClickSoundChecked=false\n")
        ini.write_text(contents, encoding="utf-8")
    cache_pid = str(os.getpid())
    gsudo = shutil.which("gsudo.exe")
    try:
        if args.gsudo_cache:
            assert elevation(win32api.GetCurrentProcess()) == 0
            assert subprocess.run([gsudo, "status", "CacheAvailable", "--no-output"]).returncode == 1
            subprocess.run([gsudo, "cache", "on", "-p", cache_pid, "-d", "00:05:00"], check=True)
            subprocess.run([gsudo, "status", "CacheAvailable", "--no-output"], check=True)
            print("PASS gsudo cache available for ordinary test controller PID " + cache_pid, flush=True)
        asyncio.run(run(args))
    finally:
        if args.gsudo_cache:
            subprocess.run([gsudo, "cache", "off", "-p", cache_pid], check=True)
            assert subprocess.run([gsudo, "status", "CacheAvailable", "--no-output"]).returncode == 1
            print("PASS this test controller gsudo cache is closed", flush=True)
