"""Opt-in IbInput GUI/MCP acceptance on an owned stock product, without mapping execution."""

import argparse
import asyncio
from contextlib import closing
import ctypes
from ctypes import wintypes
import hashlib
import importlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from unittest.mock import patch
import winreg

from mcp import Client, StdioServerParameters
import win32api
import win32con
import win32event
import win32file
import win32process


def filter_keys():
    class State(ctypes.Structure):
        _fields_ = [(name, wintypes.DWORD) for name in ("size", "flags", "wait", "delay", "repeat", "bounce")]
    value = State()
    value.size = ctypes.sizeof(value)
    user = ctypes.WinDLL("user32", use_last_error=True)
    if not user.SystemParametersInfoW(0x0032, value.size, ctypes.byref(value), 0):
        raise ctypes.WinError(ctypes.get_last_error())
    return {name: getattr(value, name) for name, _ in value._fields_}


async def run(args):
    source = Path(__file__).resolve().parents[2]
    host = args.controller_workspace
    assert args.runtime.is_relative_to(host / "out/mcp_product")
    assert args.gui_probe.is_relative_to(host / "out/mcp_product")
    assert args.output.is_relative_to(host / "out/mcp_product") and not args.output.exists()
    assert args.runtime.name in ("gui_runtime", "mcp_runtime")
    executable = args.runtime / "QKeyMapperMcpAcceptance_ibinput_qt6.exe"
    def sha(path):
        with path.open("rb") as stream:
            return hashlib.file_digest(stream, "sha256").hexdigest()
    assert sha(executable) == sha(source / "out/build_qt6/release/QKeyMapper.exe")
    assert sha(args.gui_probe) == sha(source / "out/ibinput_mcp_integration/gui-qt6/release/QKeyMapperIbInputProductGui.exe")
    for dependency in ("QJoysticks/lib/SDL/lib/x64/SDL2.dll", "ViGEm/lib/x64/ViGEmClient.dll",
                       "Interception/lib/x64/interception.dll", "libusb/lib/x64/libusb-1.0.dll",
                       "FakerInput/lib/x64/FakerInputDll.dll", "IbInputSimulator/lib/x64/IbInputSimulator.dll"):
        path = source / "QKeyMapper" / dependency
        assert sha(args.runtime / path.name) == sha(path), "Incomplete or mismatched stock product dependencies."
    fixture = source / "QKeyMapper/tests/mcp_control" / (
        "product_fixture.ini.example" if args.phase == "gui" else
        "ibinput_sources_fixture.ini.example" if args.phase == "sources" else "product_persistence_fixture.ini.example")
    assert (args.runtime / "keymapdata.ini").read_bytes() == fixture.read_bytes()
    # The original public controller and fixed helpers stay in their own checkout.
    sys.path.insert(0, str(host / "mcp_bridge/tests"))
    from test_product_interop import Product, connection_pending
    from test_sdk_interop import call, connection, integrity, identity, managed_action
    import cache_control as control
    from cache_actions import digest, helper_hashes
    from qkeymapper_mcp.credentials import CredentialStore
    assert control.ROOT == host
    assert (host / "mcp_bridge/tests/test_product_interop.py").read_bytes() == (source / "mcp_bridge/tests/test_product_interop.py").read_bytes()
    assert (host / "scripts/test_process.ps1").read_bytes() == (source / "scripts/test_process.ps1").read_bytes()
    args.output.mkdir(parents=True)
    calls, peers, grants, completed = [], [], set(), []
    store = None
    original_filter = filter_keys()
    own = identity(os.getpid())
    with closing(win32api.OpenProcess(0x1000, False, own.pid)) as process:
        assert integrity(process) == 8192
    (args.output / "filter-before.json").write_text(json.dumps(original_filter), encoding="utf-8")

    def register(identifier, target, cases, gui_actions, client=None):
        value = dict(id=identifier, executable=str(target), sha256=hashlib.file_digest(target.open("rb"), "sha256").hexdigest(),
                     cases=cases, desktop="private", output_root=str(args.output), maximum_seconds=600 if gui_actions else 60,
                     gui_actions=gui_actions, client_executable=client, helpers=helper_hashes(host))
        state = control.verified_controller()
        assert state and state["format_version"] == 2
        live = control.send(state, dict(op="status"))
        assert live["ok"] and not live["unresolved_starts"] and live["status"] == "active"
        scope = dict(live["scope"])
        allowed = set(scope.get("test_actions", {}).get("definitions", []))
        approved = digest(value)
        allowed.add(approved)
        scope["test_actions"] = dict(reason="Approved IB-MCP-138 owned private stock product and fixed native GUI actions; no mapping or system input.", definitions=sorted(allowed))
        assert control.send(state, dict(op="authorize", scope=scope))["ok"]
        grants.add(approved)
        assert control.send(state, dict(op="register", definition=value))["ok"]
        (args.output / (identifier + ".json")).write_text(json.dumps(value, indent=2), encoding="utf-8")

    register("ibmcp138-" + args.phase, executable, [dict(arguments="", environment={})],
             ["OpenAdvanced", "OpenMcp", "Enable", "SaveStartup", "ApproveConfigure", "ApproveRead", "CloseMcp",
              "ReadMapping", "ReadTab1", "ReadCommon", "AddBasicMapping", "Revoke", "Disable"], own.executable)

    async def gui(product, action):
        output = args.runtime / (args.output.name + "-" + product.output.name + "-" + action + ".json")
        assert not output.exists()
        arguments = subprocess.list2cmdline([action, str(product.pid), str(product.started_record["created"]), str(executable), str(output)])
        environment = {"PATH": "C:/Qt/6.8.3/msvc2022_64/bin;C:/Windows/System32;C:/Windows"}
        register("ibmcp138-gui-probe", args.gui_probe, [dict(arguments=arguments, environment=environment)], [])
        response = await asyncio.to_thread(managed_action, dict(Action="Start", Executable=str(args.gui_probe),
            Arguments=arguments, Environment=environment, DesktopName=product.desktop_name))
        pid = response["ProcessId"]
        process = win32api.OpenProcess(0x1000 | win32con.SYNCHRONIZE, False, pid)
        try:
            deadline = time.monotonic() + 20
            while not output.exists():
                assert win32event.WaitForSingleObject(process, 0) == win32event.WAIT_TIMEOUT, "Fixed GUI probe exited before writing evidence."
                assert time.monotonic() < deadline, "Fixed GUI probe did not return evidence."
                await asyncio.sleep(0.03)
            value = json.loads(output.read_text(encoding="utf-8"))
        finally:
            Path(str(output) + ".done").touch()
            assert win32event.WaitForSingleObject(process, 30000) == win32event.WAIT_OBJECT_0
            native_exit = win32process.GetExitCodeProcess(process)
            process.Close()
            (args.output / (product.output.name + "-" + action + "-exit.json")).write_text(
                json.dumps(dict(pid=pid, exit_code=native_exit)), encoding="utf-8")
        assert native_exit == 0 and value["ok"] and value["target_pid"] == product.pid
        assert value["desktop"] == product.desktop_name
        return value

    async def request(client, name, arguments=None, error=None):
        value = await call(client, name, arguments, error=error)
        calls.append(dict(tool=name, arguments=arguments, response=value))
        (args.output / "sdk-calls.json").write_text(json.dumps(calls, ensure_ascii=False, indent=2), encoding="utf-8")
        return value

    def parameters(discovery):
        return StdioServerParameters(command=sys.executable, args=["-B", "-X", "utf8", "-m", "qkeymapper_mcp",
            "--discovery-file", str(discovery), "--executable", str(executable)],
            env={"PYTHONPATH": str(source / "mcp_bridge/src")}, cwd=str(args.output))

    stdio = importlib.import_module("mcp.client.stdio")
    spawn = stdio._create_platform_compatible_process

    async def observed_spawn(*values, **keywords):
        process = await spawn(*values, **keywords)
        try:
            peer = identity(process.pid)
            peers.append(dict(pid=peer.pid, created=str(peer.created), executable=peer.executable))
            (args.output / "bridge-peers.json").write_text(json.dumps(peers, indent=2), encoding="utf-8")
            assert peer.same_login(own) and Path(peer.executable) in {Path(own.executable), Path(sys.executable)}
        except BaseException:
            process.terminate()
            await process.wait()
            raise
        return process

    old = {}

    async def configured(product):
        nonlocal store
        await product.action("OpenAdvanced")
        state = await product.action("OpenMcp")
        assert state["pair_count"] in (0, 1)
        paired = state["pair_count"] == 1
        if state["enabled"]:
            # A failed owned run may already have saved its startup choice.
            await product.action("Disable")
            await product.action("SaveStartup")
        await product.action("Enable")
        discovery = Path((await product.action("SaveStartup"))["discovery"])
        candidate = CredentialStore(discovery, executable, own.session)
        assert (candidate.load() is not None) == paired, "Preserve inconsistent pre-existing pairing evidence."
        store = candidate
        async with Client(parameters(discovery), cache=None) as client:
            await request(client, "qkm_connect")
            if not paired:
                await connection(client, "pairing_required")
                await request(client, "qkm_request_pairing")
                await connection_pending(client)
                await product.action("ApproveRead" if args.phase == "read" else "ApproveConfigure", own.executable)
            await connection(client, "connected")
            await product.action("CloseMcp")
            await gui(product, "return_to_mapping")
            await product.action("ReadMapping")
            if args.phase == "read":
                before = (args.runtime / "keymapdata.ini").read_bytes()
                state = (await request(client, "qkm_get_state"))["result"]
                assert (await request(client, "qkm_get_connection_state"))["scopes"] == 1
                sources = (await request(client, "qkm_query", dict(entity="mapping_sources")))["result"]
                await request(client, "qkm_preview_changes", dict(source_revision=sources["source_snapshot"]["revision"],
                    source_tab_ref=sources["items"][0]["source_tab_ref"], original_key="A", mapping_key="B", send_method=3), error="permission_denied")
                await request(client, "qkm_preview_changes", dict(action="ibinput.preferences.update",
                    preferences_revision=state["ibinput"]["preferences_revision"], preferred_backend="razer"), error="permission_denied")
                await request(client, "qkm_preview_changes", dict(action="configuration.save", setting_name="Persist136A"), error="permission_denied")
                for _ in range(3):
                    observed = (await request(client, "qkm_get_state"))["result"]
                    assert observed["configuration"] == state["configuration"]
                    assert observed["ibinput"]["preferences_revision"] == state["ibinput"]["preferences_revision"]
                    assert not observed["ibinput"]["connected"] and observed["ibinput"]["last_result"] is None
                assert (await request(client, "qkm_query", dict(entity="mapping_sources")))["result"] == sources
                assert (args.runtime / "keymapdata.ini").read_bytes() == before
                await request(client, "qkm_disconnect")
                await product.action("OpenAdvanced")
                await product.action("OpenMcp")
                await product.action("Disable")
                await product.action("SaveStartup")
                await product.action("Revoke")
                return
            if args.phase == "sources":
                await source_changes(product, client)
                return
            if args.phase == "gui":
                await product.action("AddBasicMapping")
                await gui(product, "row_ibinput")
                await gui(product, "preferences_razer_fallback")
                await gui(product, "save")
            else:
                await mcp_changes(product, client)
            state = (await request(client, "qkm_get_state"))["result"]
            assert state["run"]["gui_state"] == "idle"
            assert state["ibinput"]["preferences"] == dict(preferred_backend="razer", unavailable_policy="sendinput_before_start")
            assert state["ibinput"]["connected"] is False and state["ibinput"]["actual_backend"] is None
            assert state["ibinput"]["last_result"] is None
            rows = await source_rows(client)
            assert len(rows["items"]) == 1 and rows["items"][0]["fields"]["KeyMapData_SendMappingKeyMethod"] == 3
            old.update(discovery=str(discovery), instance=state["instance"], source_revision=rows["source_snapshot"]["revision"],
                       source_tab_ref=rows["source"]["source_tab_ref"])

    async def source_rows(client):
        sources = (await request(client, "qkm_query", dict(entity="mapping_sources")))["result"]
        source = sources["items"][0]
        rows = (await request(client, "qkm_query", dict(entity="mappings", source_revision=sources["source_snapshot"]["revision"],
            source_tab_ref=source["source_tab_ref"])))["result"]
        return dict(rows, source=source)

    async def mcp_changes(product, client):
        sources = (await request(client, "qkm_query", dict(entity="mapping_sources")))["result"]
        preview = (await request(client, "qkm_preview_changes", dict(source_revision=sources["source_snapshot"]["revision"],
            source_tab_ref=sources["items"][0]["source_tab_ref"], original_key="A", mapping_key="B", send_method=3)))["result"]
        before = (args.runtime / "keymapdata.ini").read_bytes()
        await request(client, "qkm_get_state")
        await request(client, "qkm_get_state")
        await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"]))
        rows = await source_rows(client)
        row = rows["items"][0]
        preview = (await request(client, "qkm_preview_changes", dict(action="mapping.update", source_ref=row["source_ref"], send_method=0)))["result"]
        await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"]))
        rows = await source_rows(client)
        preview = (await request(client, "qkm_preview_changes", dict(action="mapping.update", source_ref=rows["items"][0]["source_ref"], send_method=3)))["result"]
        await gui(product, "preferences_razer_fallback")
        await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"]), error="stale_preview")
        state = (await request(client, "qkm_get_state"))["result"]
        change = dict(action="ibinput.preferences.update", preferences_revision=state["ibinput"]["preferences_revision"], preferred_backend="auto")
        preview = (await request(client, "qkm_preview_changes", change))["result"]
        await request(client, "qkm_cancel_changes", dict(preview_id=preview["preview_id"]))
        assert (await request(client, "qkm_get_state"))["result"]["ibinput"]["preferences_revision"] == change["preferences_revision"]
        preview = (await request(client, "qkm_preview_changes", change))["result"]
        await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"]))
        await request(client, "qkm_preview_changes", change, error="stale_preview")
        state = (await request(client, "qkm_get_state"))["result"]
        preview = (await request(client, "qkm_preview_changes", dict(action="ibinput.preferences.update",
            preferences_revision=state["ibinput"]["preferences_revision"], preferred_backend="razer", unavailable_policy="sendinput_before_start")))["result"]
        await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"]))
        rows = await source_rows(client)
        preview = (await request(client, "qkm_preview_changes", dict(action="mapping.update", source_ref=rows["items"][0]["source_ref"], send_method=3)))["result"]
        await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"]))
        assert (args.runtime / "keymapdata.ini").read_bytes() == before
        state = (await request(client, "qkm_get_state"))["result"]
        draft_preferences = state["ibinput"]["preferences"]
        revision = state["ibinput"]["preferences_revision"]
        preview = (await request(client, "qkm_preview_changes", dict(action="ibinput.preferences.update",
            preferences_revision=revision, **draft_preferences)))["result"]
        await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"]))
        assert (await request(client, "qkm_get_state"))["result"]["ibinput"]["preferences_revision"] == revision
        preview = (await request(client, "qkm_preview_changes", dict(action="configuration.save", setting_name="Persist136A")))["result"]
        await request(client, "qkm_cancel_changes", dict(preview_id=preview["preview_id"]))
        assert (args.runtime / "keymapdata.ini").read_bytes() == before
        assert (await request(client, "qkm_get_state"))["result"]["configuration"]["has_unsaved_changes"]
        preview = (await request(client, "qkm_preview_changes", dict(action="configuration.save", setting_name="Persist136A")))["result"]
        # Share reads but deny replacement of this owned INI; no ACL or global setting changes.
        with closing(win32file.CreateFile(str(args.runtime / "keymapdata.ini"), win32con.GENERIC_READ,
                                         win32con.FILE_SHARE_READ, None, win32con.OPEN_EXISTING, 0, None)):
            await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"], confirm_scope=True), error="save_write_failed")
            operation = (await request(client, "qkm_get_operation", dict(operation_id=preview["preview_id"])))["result"]
            assert operation["status"] == "failed" and not operation["result"]["saved"] and not operation["result"]["reloaded"]
        assert (args.runtime / "keymapdata.ini").read_bytes() == before
        after_failure = (await request(client, "qkm_get_state"))["result"]
        assert after_failure["ibinput"]["preferences"] == draft_preferences
        assert after_failure["ibinput"]["preferences_revision"] == revision
        assert after_failure["configuration"]["has_unsaved_changes"]
        preview = (await request(client, "qkm_preview_changes", dict(action="configuration.save", setting_name="Persist136A")))["result"]
        assert {"IbInputPreferredBackend", "IbInputUnavailablePolicy"} <= set(preview["description"]["scope"]["shared_write_keys"])
        result = (await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"], confirm_scope=True)))["result"]
        assert result["status"] == "applied" and result["result"]["saved"] and result["result"]["reloaded"]
        old["operation_id"] = preview["preview_id"]
        rows = await source_rows(client)
        pending = (await request(client, "qkm_preview_changes", dict(action="mapping.update",
            source_ref=rows["items"][0]["source_ref"], send_method=3)))["result"]
        old["pending_preview"] = pending["preview_id"]

    async def source_changes(product, client):
        disk = (args.runtime / "keymapdata.ini").read_bytes()
        rows = await source_rows(client)
        assert len(rows["items"]) == 4
        by_key = {row["fields"]["KeyMapData_OriginalKeys"]: row for row in rows["items"]}
        assert by_key["C"]["fields"]["KeyMapData_Disabled"] is True
        assert by_key["F8"]["fields"]["KeyMapData_SendMappingKeyMethod"] == 3
        await request(client, "qkm_preview_changes", dict(action="mapping.update", source_ref=by_key["F8"]["source_ref"], send_method=0), error="invalid_mapping")
        assert (await source_rows(client)) == rows and (args.runtime / "keymapdata.ini").read_bytes() == disk
        preview = (await request(client, "qkm_preview_changes", dict(action="mapping.update", source_ref=by_key["C"]["source_ref"], send_method=3)))["result"]
        await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"]))
        await product.action("ReadCommon")
        listing = (await request(client, "qkm_query", dict(entity="mapping_sources")))["result"]
        common = next(item for item in listing["items"] if item["kind"] == "common")
        common_rows = (await request(client, "qkm_query", dict(entity="mappings", source_revision=listing["source_snapshot"]["revision"], source_tab_ref=common["source_tab_ref"])))["result"]
        assert len(common_rows["items"]) == 2
        preview = (await request(client, "qkm_preview_changes", dict(action="mapping.update", source_ref=common_rows["items"][0]["source_ref"], send_method=0)))["result"]
        await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"]))
        await product.action("ReadTab1")
        rows = await source_rows(client)
        preview = (await request(client, "qkm_preview_changes", dict(source_revision=rows["source_snapshot"]["revision"],
            source_tab_ref=rows["source"]["source_tab_ref"], original_key="H", mapping_key="L", send_method=3)))["result"]
        await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"], confirm_conflicts=True))
        rows = await source_rows(client)
        added = next(row for row in rows["items"] if row["fields"]["KeyMapData_OriginalKeys"] == "H")
        assert added["fields"]["KeyMapData_Disabled"] is True
        assert (args.runtime / "keymapdata.ini").read_bytes() == disk
        preview = (await request(client, "qkm_preview_changes", dict(action="configuration.save", setting_name="Persist136A")))["result"]
        result = (await request(client, "qkm_apply_changes", dict(preview_id=preview["preview_id"], confirm_scope=True)))["result"]
        assert result["status"] == "applied" and result["result"]["saved"] and result["result"]["reloaded"]
        state = (await request(client, "qkm_get_state"))["result"]
        assert not state["ibinput"]["connected"] and state["ibinput"]["last_result"] is None

    async def cold_sources(product):
        await product.action("OpenAdvanced")
        management = await product.action("OpenMcp")
        assert management["enabled"] and management["pair_count"] == 1
        await product.action("CloseMcp")
        await gui(product, "return_to_mapping")
        await product.action("ReadTab1")
        async with Client(parameters(Path(management["discovery"])), cache=None) as client:
            await request(client, "qkm_connect")
            await connection(client, "connected")
            rows = await source_rows(client)
            assert len(rows["items"]) == 5
            row = next(item for item in rows["items"] if item["fields"]["KeyMapData_OriginalKeys"] == "C")
            assert row["fields"]["KeyMapData_Disabled"] is True and row["fields"]["KeyMapData_SendMappingKeyMethod"] == 3
            disk = (args.runtime / "keymapdata.ini").read_bytes()
            await request(client, "qkm_preview_changes", dict(action="mapping.update", source_ref=row["source_ref"], send_method=0), error="invalid_target")
            assert await source_rows(client) == rows and (args.runtime / "keymapdata.ini").read_bytes() == disk
            listing = (await request(client, "qkm_query", dict(entity="mapping_sources")))["result"]
            common = next(item for item in listing["items"] if item["kind"] == "common")
            page = (await request(client, "qkm_query", dict(entity="mappings", source_revision=listing["source_snapshot"]["revision"], source_tab_ref=common["source_tab_ref"])))["result"]
            assert page["items"][0]["fields"]["KeyMapData_SendMappingKeyMethod"] == 0
        await product.action("OpenAdvanced")
        await product.action("OpenMcp")
        await product.action("Disable")
        await product.action("SaveStartup")
        await product.action("Revoke")

    async def cold(product):
        await product.action("OpenAdvanced")
        state = await product.action("OpenMcp")
        assert state["enabled"] and state["pair_count"] == 1
        await product.action("CloseMcp")
        async with Client(parameters(Path(old["discovery"])), cache=None) as client:
            await request(client, "qkm_connect")
            await connection(client, "connected")
            state = (await request(client, "qkm_get_state"))["result"]
            assert state["instance"] != old["instance"] and not state["configuration"]["has_unsaved_changes"]
            assert state["ibinput"]["preferences"] == dict(preferred_backend="razer", unavailable_policy="sendinput_before_start")
            rows = await source_rows(client)
            assert len(rows["items"]) == 1 and rows["items"][0]["fields"]["KeyMapData_SendMappingKeyMethod"] == 3
            assert rows["items"][0]["fields"]["KeyMapData_OriginalKeys"] == "A"
            assert rows["items"][0]["fields"]["KeyMapData_MappingKeys"] == ["B"]
            await request(client, "qkm_query", dict(entity="mappings", source_revision=old["source_revision"], source_tab_ref=old["source_tab_ref"]), error="stale_source")
            if args.phase == "mcp":
                assert (await request(client, "qkm_get_operation", dict(operation_id=old["operation_id"])))["result"]["status"] == "unknown"
                await request(client, "qkm_apply_changes", dict(preview_id=old["pending_preview"]), error="stale_preview")
        await product.action("OpenAdvanced")
        await product.action("OpenMcp")
        await product.action("Disable")
        await product.action("SaveStartup")
        await product.action("Revoke")

    async def phase(name, body):
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Enum\ROOT\FAKERINPUT"):
                raise AssertionError("FakerInput appeared; review ordinary shutdown side effects before startup.")
        except FileNotFoundError:
            pass
        output = args.output / name
        output.mkdir()
        product = Product(executable, output, lifetime_seconds=600)
        try:
            await product.started()
            await body(product)
        finally:
            failure = sys.exc_info()[1]
            try:
                await product.close()
            except Exception as cleanup_error:
                (output / "cleanup-error.txt").write_text(repr(cleanup_error), encoding="utf-8")
                if failure is None:
                    raise
                failure.add_note("Cleanup did not verify a normal exit; see cleanup-error.txt and the native action record.")
            finally:
                after = filter_keys()
                (output / "filter-after.json").write_text(json.dumps(after), encoding="utf-8")
                assert after == original_filter
        completed.append(name)

    try:
        with patch.object(stdio, "_create_platform_compatible_process", observed_spawn):
            await phase("configured", configured)
            if args.phase == "sources":
                path = args.runtime / "keymapdata.ini"
                saved = path.read_bytes()
                (args.output / "saved-before-hidden-fixture.ini").write_bytes(saved)
                assert saved.count(b"HideDisabled=false") == 1
                path.write_bytes(saved.replace(b"HideDisabled=false", b"HideDisabled=true"))
                await phase("hidden-cold", cold_sources)
            elif args.phase != "read":
                await phase("cold", cold)
        assert sum(item["tool"] == "qkm_request_pairing" for item in calls) <= 1 and len(peers) == (1 if args.phase == "read" else 2)
        store.forget()
        assert store.load() is None
        (args.output / "passed.json").write_text(json.dumps(dict(phase=args.phase, completed=completed,
            system_input=False, mapping_started=False, peer_count=len(peers), credential_removed=True), indent=2), encoding="utf-8")
        detail = "Read-only configuration refusals and passive invariance" if args.phase == "read" else "GUI/MCP configuration and independent cold read"
        print("PASS owned stock product", args.phase, detail + "; no mapping started", flush=True)
    finally:
        state = control.verified_controller()
        live = control.send(state, dict(op="status")) if state else None
        if live and live.get("ok") and not live["owned_targets"] and not live["unresolved_starts"]:
            scope = dict(live["scope"])
            allowed = set(scope.get("test_actions", {}).get("definitions", [])) - grants
            if allowed:
                scope["test_actions"] = dict(reason="Retained other approved fixed tests after IB-MCP-138 cleanup.", definitions=sorted(allowed))
            else:
                scope.pop("test_actions", None)
            assert control.send(state, dict(op="authorize", scope=scope))["ok"]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--controller-workspace", type=lambda value: Path(value).resolve(strict=True), required=True)
    parser.add_argument("--runtime", type=lambda value: Path(value).resolve(strict=True), required=True)
    parser.add_argument("--gui-probe", type=lambda value: Path(value).resolve(strict=True), required=True)
    parser.add_argument("--output", type=lambda value: Path(value).resolve(), required=True)
    parser.add_argument("--phase", choices=("gui", "mcp", "read", "sources"), required=True)
    asyncio.run(run(parser.parse_args()))
