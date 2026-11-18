"""Verify real Rust-served images and in-memory cleanup on an approved private product."""

import argparse
import asyncio
import json
import os
import subprocess
import sys
import uuid
from contextlib import closing
from pathlib import Path

from mcp import Client, StdioServerParameters
import win32api
import win32con
import win32event
import win32gui
import win32process
import win32service

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "mcp_bridge/tests"))
from demo_scene_acceptance import exercise
from test_product_interop import connection_pending
from test_product_persistence import filter_keys
from test_sdk_interop import call, close_desktop, connection, integrity, managed_action, parameters
from window_checks import verify_capture_reply
from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.windows import identity

import cache_control as cache
from cache_actions import digest, file_hash, helper_hashes


def desktop_observation():
    handle = win32service.OpenInputDesktop(0, False, win32con.DESKTOP_READOBJECTS)
    try:
        name = win32service.GetUserObjectInformation(handle, win32con.UOI_NAME)
    finally:
        close_desktop(handle)
    foreground = win32gui.GetForegroundWindow()
    return name, win32process.GetWindowThreadProcessId(foreground)[1] if foreground else 0


async def run(args):
    exe, rust, helper, output = args.executable, args.rust_exe, args.gui, args.output
    ini = exe.parent / "keymapdata.ini"
    fixture = ROOT / "QKeyMapper/tests/mcp_control/product_persistence_fixture.ini.example"
    assert ini.read_bytes() == fixture.read_bytes(), "Use the unchanged owned synthetic configuration."
    assert integrity(win32api.GetCurrentProcess()) == 8192
    assert desktop_observation()[0] == "Default"
    output.mkdir(parents=True, exist_ok=False)
    initial_filter = filter_keys()
    controller = cache.verified_controller()
    assert controller is not None
    grants = set()
    job = process = store = None
    pid = None
    sequence = 0
    desk = "QkmProduct_" + uuid.uuid4().hex
    desktop = win32service.CreateDesktop(desk, 0, 511, None)

    def live():
        value = cache.send(controller, {"op":"status"}, timeout=10)
        assert value["ok"] and value["cache_available"] and not value["unresolved_starts"]
        return value

    def register(name, target, arguments, env, actions=()):
        value = {"id":name,"executable":str(target),"sha256":file_hash(target),
            "cases":[{"arguments":arguments,"environment":env}],"desktop":"private",
            "output_root":str(output),"maximum_seconds":420 if actions else 60,
            "gui_actions":list(actions),"client_executable":str(rust) if actions else None,"helpers":helper_hashes(ROOT)}
        scope = dict(live()["scope"])
        fixed = dict(scope.get("test_actions", {}))
        fixed.setdefault("reason","已批准的 RUST-C3/C4 私有演示、固定 GUI 权限及清理。")
        key = digest(value)
        fixed["definitions"] = sorted(set(fixed.get("definitions", [])) | {key})
        scope["test_actions"] = fixed
        result = cache.send(controller, {"op":"authorize","scope":scope}, timeout=10)
        assert result["ok"], result
        grants.add(key)
        result = cache.send(controller, {"op":"register","definition":value}, timeout=10)
        assert result["ok"], result
        cache.write_json(output / f"{name}.json", value)

    async def gui(name, client=None):
        nonlocal sequence
        sequence += 1
        path = output / f"{sequence:02}_{name}.json"
        values = {"Action":"Gui","Name":name,"Executable":str(exe),"ProcessId":pid,
            "StartTicks":job["StartTicks"],"DesktopName":desk,"ResultFile":str(path)}
        if client is not None:
            values["ClientExecutable"] = str(client)
        await asyncio.to_thread(managed_action, values)
        value = json.loads(path.read_text(encoding="utf-8"))
        assert value["success"] and value["action"] == name and value["desktop"] == desk
        return value

    async def native_gui(name, pair_id=None):
        path = exe.parent / f"{output.name}-{name}.json"
        assert not path.exists()
        values = [name, str(pid), str(server.created), str(exe), str(path)]
        if pair_id is not None:
            values.append(pair_id)
        arguments = subprocess.list2cmdline(values)
        matcher = {"PATH":"C:/Qt/6.8.3/msvc2022_64/bin;C:/Windows/System32;C:/Windows"}
        register("rust-c4-" + name.replace("_", "-"), helper, arguments, matcher)
        reply = await asyncio.to_thread(managed_action, {"Action":"Start","Executable":str(helper),
            "Arguments":arguments,"Environment":matcher,"DesktopName":desk})
        with closing(win32api.OpenProcess(0x1000 | win32con.SYNCHRONIZE, False, reply["ProcessId"])) as current:
            try:
                async with asyncio.timeout(22):
                    while not path.exists():
                        assert win32event.WaitForSingleObject(current, 0) == win32event.WAIT_TIMEOUT
                        await asyncio.sleep(.05)
                value = json.loads(path.read_text(encoding="utf-8"))
                assert value["ok"] and value["desktop"] == desk and value["target_pid"] == pid
            finally:
                Path(str(path) + ".done").touch()
                assert await asyncio.to_thread(win32event.WaitForSingleObject, current, 28000) == win32event.WAIT_OBJECT_0
                assert win32process.GetExitCodeProcess(current) == 0

    async def request(client, method, arguments=None, error=None):
        nonlocal sequence
        observed = [desktop_observation()]
        if method in ("qkm_capture_window", "qkm_demo_mapping"):
            reply = await client.call_tool(method, arguments)
            reference = arguments
            if method == "qkm_demo_mapping":
                reference = reply.structured_content["result"]
            result, png = verify_capture_reply(reply, reference, error=error)
            if png is not None:
                sequence += 1
                (output / f"{sequence:02}_{reference['role']}.png").write_bytes(png)
        else:
            result = await call(client, method, arguments, error=error)
        observed.append(desktop_observation())
        with (output / "calls.jsonl").open("a", encoding="utf-8") as log:
            print(json.dumps({"tool":method,"arguments":arguments,"reply":result,
                "desktop_samples":observed}, ensure_ascii=False), file=log)
        assert all(name == "Default" and foreground != pid for name, foreground in observed)
        return result

    try:
        current = live()
        assert current["remaining_seconds"] > 600 and not any(current[key] for key in
            ("current_action","current_job","pending_request","owned_targets"))
        case = "--demo-configuration-only"
        actions = ("OpenAdvanced","OpenMcp","Enable","ApproveConfigure","CloseMcp","Revoke","Disable")
        register("rust-c4-demo-product", exe, case, {}, actions)
        job = await asyncio.to_thread(managed_action, {"Action":"Start","Executable":str(exe),
            "Arguments":case,"Environment":{},"DesktopName":desk})
        pid = job["ProcessId"]
        process = win32api.OpenProcess(0x1000 | win32con.SYNCHRONIZE, False, pid)
        server = identity(pid)
        own = identity(os.getpid())
        assert Path(server.executable) == exe and own.same_login(server) and integrity(process) == 0x3000
        await gui("OpenAdvanced")
        initial = await gui("OpenMcp")
        assert not initial["enabled"] and initial["pair_count"] == 0
        discovery = Path((await gui("Enable"))["discovery"])
        candidate = CredentialStore(discovery, exe, own.session)
        assert candidate.load() is None
        store = candidate
        options = ["--discovery-file", str(discovery), "--executable", str(exe)]
        rust_client = StdioServerParameters(command=str(rust), cwd=str(output),
            args=[*options,"--product-demo-record","--enable-demo-mapping"])
        async with Client(rust_client, cache=None) as client:
            await call(client, "qkm_request_pairing")
            await connection_pending(client)
            await gui("ApproveConfigure", rust)
            info = await connection(client, "connected")
            assert info["scopes"] == 3
            observed = (await request(client, "qkm_get_demo_state"))["result"]
            assert not observed["ready"] and observed["reason"] == "permission_denied"
        await native_gui("grant_both", store.load().pair_id)
        await gui("CloseMcp")
        await native_gui("return_to_mapping")
        creation = discovery.with_name(f"demo-{info['instance']}.json")
        python = parameters(discovery, exe, output)
        python.args.extend(["--demo-record", str(creation)])
        async with Client(python, cache=None) as client:
            await call(client, "qkm_connect")
            assert (await connection(client, "connected"))["scopes"] == 387
            expected = (await request(client, "qkm_get_demo_state"))["result"]
            assert expected["ready"]

        async with Client(rust_client, cache=None) as client:
            await call(client, "qkm_connect")
            assert (await connection(client, "connected"))["scopes"] == 387
            ready = (await request(client, "qkm_get_demo_state"))["result"]
            assert ready["ready"] and ready["target"] == expected["target"] and ready["source_snapshot"] == expected["source_snapshot"]
            assert ready["target"]["desktop"] == desk and ready["target"]["pid"] == str(pid)
            sources = (await request(client, "qkm_query", {"entity":"mapping_sources"}))["result"]
            source = {"source_revision":sources["source_snapshot"]["revision"],"source_tab_ref":sources["items"][0]["source_tab_ref"]}
            windows = (await request(client, "qkm_query", {"entity":"windows"}))["result"]
            await request(client, "qkm_capture_window", dict(source, role="main", window_ref=windows["items"][0]["window_ref"]))
            popup = {"source_tab_ref":source["source_tab_ref"],"presentation":"private_background","window_role":"ibinput_settings"}
            opened = (await request(client, "qkm_open_mapping_settings", popup))["result"]
            assert opened["opened"] and not opened["activation_requested"]
            reference = opened["window"]["window_ref"]
            capture = dict(source, role="ibinput_settings", window_ref=reference)
            assert (await request(client, "qkm_capture_window", capture))["result"]["settings_editor_unchanged"]
            assert (await request(client, "qkm_close_mapping_settings", dict(popup, window_ref=reference)))["result"]["closed"]
            await request(client, "qkm_capture_window", capture, error="stale_window")

            async def bound_request(method, arguments=None, error=None):
                return await request(client, method, arguments, error)

            result = await exercise(client, bound_request, "demo_sequence", output, exe, views=("item_setup","mapping_advanced"))
            assert result["full_scene_cleanup_verified"] and result["scenes"] == 2
        assert ini.read_bytes() == fixture.read_bytes()
        await gui("OpenAdvanced")
        await gui("OpenMcp")
        await gui("Revoke")
        await gui("Disable")
    finally:
        try:
            if job is not None:
                await asyncio.to_thread(managed_action, {"Action":"Close","Executable":str(exe),
                    "ProcessId":pid,"StartTicks":job["StartTicks"],"DesktopName":desk})
                assert process is not None and win32event.WaitForSingleObject(process, 10000) == win32event.WAIT_OBJECT_0
                assert win32process.GetExitCodeProcess(process) == 0
                print("Product exited 0", flush=True)
        finally:
            try:
                if process is not None:
                    process.Close()
                if store is not None:
                    store.forget()
                    assert store.load() is None
            finally:
                close_desktop(desktop)
                if grants:
                    scope = dict(live()["scope"])
                    fixed = dict(scope.get("test_actions", {}))
                    remaining = set(fixed.get("definitions", [])) - grants
                    if remaining:
                        scope["test_actions"] = dict(fixed, definitions=sorted(remaining))
                    else:
                        scope.pop("test_actions", None)
                    assert cache.send(controller, {"op":"authorize","scope":scope}, timeout=10)["ok"]
                assert filter_keys() == initial_filter
    print("PASS four native PNG roles, no stale image, two scenes recovered, private product and credential cleaned", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    for flag in ("--executable","--rust-exe","--gui","--output"):
        parser.add_argument(flag, type=Path, required=True)
    args = parser.parse_args()
    if sys.flags.optimize:
        parser.error("Run with assertions enabled.")
    for key, value in vars(args).items():
        path = value.resolve(strict=key != "output")
        assert path.is_relative_to(ROOT / "out/mcp_product")
        setattr(args, key, path)
    assert args.executable.name == "QKeyMapperMcpAcceptance_ibinput_qt6.exe"
    assert args.gui.name == "QKeyMapperIbInputProductGui.exe"
    assert args.rust_exe.name == "qkeymapper-mcp-rust.exe"
    asyncio.run(run(args))
