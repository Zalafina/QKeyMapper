"""Authorized Default-desktop A->B Release comparison; never installs drivers."""

if not __debug__:
    raise RuntimeError("Run without -O; input scope and validation assertions must stay enabled.")

import argparse
import asyncio
from contextlib import AsyncExitStack, closing
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

import win32api
import win32con
import win32gui
import win32process
import pywintypes
from mcp import Client

from probe import Sender, display_sample, distribution, focus_click, plain_latencies, process_sample
from receiver import CLOSE, COUNT, FLUSH, RESET, RESULT_ROOT, input_desktop_name
from features import CASES, configuration, exercise, pressed, wait_for

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / "mcp_bridge/tests"))
from test_product_interop import Product, connection_pending
from managed_test import reject_nested_cache
from test_sdk_interop import business_state, call, connection, elevation, integrity, parameters
from test_source_interop import source_rows
from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.windows import identity


def save(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")


def save_display(path, window=None):
    try:
        save(path, display_sample(window))
    except Exception as error:
        print("UNVERIFIED display observation:", type(error).__name__, flush=True)


async def wait_file(path, child):
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        if path.exists():
            return json.loads(path.read_text(encoding="utf-8"))
        assert child.returncode is None, "Receiver exited; see receiver.log"
        await asyncio.sleep(.02)
    raise AssertionError("Receiver response timed out")


class Receiver:
    async def start(self, output, *, allow_focus_click=False, activate=True):
        self.output, self.sequence, self.allow_focus_click = output, 0, allow_focus_click
        self.clicked = False
        self.log = output.parent.joinpath("receiver.log").open("wb")
        self.child = await asyncio.create_subprocess_exec(sys.executable, "-B", str(Path(__file__).with_name("receiver.py")),
            "--output", str(output), "--desktop", "Default", "--allow-default",
            stdout=self.log, stderr=self.log, creationflags=subprocess.CREATE_NO_WINDOW)
        self.window = None
        data = await wait_file(output / "started.json", self.child)
        self.window, self.pid, self.focus_window = data["window"], data["pid"], data["focus_window"]
        assert data["desktop"] == "Default" and win32process.GetWindowThreadProcessId(self.window)[1] == self.pid
        with closing(win32api.OpenProcess(0x1000, False, self.pid)) as handle:
            assert elevation(handle) == 0 and integrity(handle) == 0x2000
        assert identity(os.getpid()).same_login(identity(self.pid))
        if activate:
            await self.focus()
            await asyncio.sleep(1)
            assert win32gui.GetForegroundWindow() == self.window, "Receiver could not acquire foreground"

    async def focus(self):
        assert win32process.GetWindowThreadProcessId(self.window)[1] == self.pid
        assert win32process.GetWindowThreadProcessId(self.focus_window)[1] == self.pid
        for _ in range(100):
            if input_desktop_name() == "Default":
                break
            await asyncio.sleep(.05)
        assert input_desktop_name() == "Default", "Input desktop is not Default; focus and input refused"
        foreground = win32gui.GetForegroundWindow()
        with (self.output / "focus.jsonl").open("a", encoding="utf-8") as log:
            log.write(json.dumps({"visible": bool(win32gui.IsWindowVisible(self.window)),
                "enabled": bool(win32gui.IsWindowEnabled(self.focus_window)),
                "foreground": foreground, "foreground_pid": win32process.GetWindowThreadProcessId(foreground)[1]
                if foreground else None}) + "\n")
        if not win32gui.IsWindowVisible(self.window):
            win32gui.ShowWindow(self.window, win32con.SW_SHOWNORMAL)
        if win32gui.GetForegroundWindow() != self.window:
            try:
                win32gui.SetForegroundWindow(self.window)
            except pywintypes.error:
                pass
        if win32gui.GetForegroundWindow() != self.window:
            assert not self.clicked, "Only one authorized focus click per receiver is allowed"
            focus_click(self.window, self.focus_window, self.pid, allowed=self.allow_focus_click)
            self.clicked = True
            for _ in range(20):
                if win32gui.GetForegroundWindow() == self.window:
                    break
                await asyncio.sleep(.05)
        assert win32gui.GetForegroundWindow() == self.window, "Receiver could not acquire foreground"

    def reset(self):
        self.sequence += 1
        win32gui.SendMessageTimeout(self.window, RESET, self.sequence, 0, 2, 1000)

    def count(self, key=0):
        return win32gui.SendMessageTimeout(self.window, COUNT, key, 0, 2, 1000)[1]

    async def flush(self):
        win32gui.PostMessage(self.window, FLUSH, self.sequence, 0)
        return (await wait_file(self.output / ("events-" + str(self.sequence) + ".json"), self.child))["events"]

    async def close(self):
        try:
            if self.window and win32gui.IsWindow(self.window):
                win32gui.PostMessage(self.window, CLOSE, 0, 0)
            assert await asyncio.wait_for(self.child.wait(), 10) == 0, "Receiver cleanup failed"
        finally:
            self.log.close()


async def plain(receiver, sender, pairs, expected_key):
    receiver.reset()
    sent = []
    try:
        for index in range(pairs):
            sent.append(sender.send(0x41, True))
            await asyncio.sleep(.02)
            sent.append(sender.send(0x41, False))
            await asyncio.sleep(.02)
            deadline = time.monotonic() + .5
            while receiver.count() < 2 * (index + 1) and time.monotonic() < deadline:
                await asyncio.sleep(.002)
            assert receiver.count() == 2 * (index + 1), "Lost or extra mapped input; stop before the next pair"
    finally:
        sender.release()
    received = await receiver.flush()
    return {"latency_ms": plain_latencies(sent, received, expected_key), "sent": sent, "received": received}


def fixture():
    text = (REPO / "QKeyMapper/tests/mcp_control/product_fixture.ini.example").read_text(encoding="utf-8")
    return text + """SettingSelect=Engine61

[Engine61]
MappingTable_TabNameList=Tab1
MappingTable_LastTabName=Tab1
KeyMapData_OriginalKeys=A
KeyMapData_MappingKeys=B
KeyMapData_SendMappingKeyMethod=0
ProcessInfo_FileName=python.exe
ProcessInfo_FileNameMatchIndex=4
ProcessInfo_WindowTitle=QKeyMapper ENGINE-61 键盘接收窗口
ProcessInfo_WindowTitleMatchIndex=1
ProcessInfo_ClassNameMatchIndex=0
MappingStartKey=F23
MappingStopKey=F24
MappingStartActionMode=0
AutoStartMappingChecked=0
SendToSameWindowsChecked=0
"""


async def measurements(product, stop, samples, *, thread_cpu=False):
    while not stop.is_set():
        samples.append(process_sample(product.process, product.pid, product.window, thread_cpu=thread_cpu))
        try:
            await asyncio.wait_for(stop.wait(), 1)
        except asyncio.TimeoutError:
            pass


async def queries(client, stop, records):
    index = 0
    while not stop.is_set():
        started = time.perf_counter_ns()
        name = "qkm_get_state" if index % 2 == 0 else "qkm_query"
        await call(client, name, None if name == "qkm_get_state" else {"entity": "tabs", "limit": 1})
        records.append({"qpc_ns": started, "method": name, "ms": (time.perf_counter_ns() - started) / 1_000_000})
        index += 1
        try:
            await asyncio.wait_for(stop.wait(), max(0, .05 - (time.perf_counter_ns() - started) / 1e9))
        except asyncio.TimeoutError:
            pass


async def editor_invalidation(product, client, current):
    from mapping_changes_acceptance import contents, operation, preview, snapshot
    pending = await preview(client, current)
    await product.action("OpenFloatingSetup")
    await call(client, "qkm_preview_changes", {"source_revision": current["revision"],
        "source_tab_ref": current["target"]["source_tab_ref"], "original_key": "A", "mapping_key": "B"}, error="context_busy")
    cancelled = (await call(client, "qkm_cancel_changes", {"preview_id": pending["preview_id"]}))["result"]
    assert cancelled["status"] == "cancelled"
    await product.action("CloseFloatingSetup")
    unchanged = await snapshot(client)
    assert contents(unchanged) == contents(current), "Opening/closing an editor changed mapping content"
    pending = await preview(client, unchanged)
    await product.action("OpenFloatingSetup")
    await call(client, "qkm_apply_changes", {"preview_id": pending["preview_id"]}, error="context_busy")
    assert (await operation(client, pending["preview_id"]))["result"]["modified"] is False
    await product.action("CloseFloatingSetup")
    current = await snapshot(client)
    pending = await preview(client, current)
    await product.action("OpenFloatingSetup")
    await product.action("SyncGroupOne")
    await product.action("CloseFloatingSetup")
    group = await snapshot(client)
    group_field = "KeyMapData_FloatingButton_SyncGroupId"
    changed = contents(group)
    expected = contents(current)
    assert expected[0]["fields"][group_field] == 0 and changed[0]["fields"][group_field] == 1
    expected[0]["fields"] = dict(expected[0]["fields"], **{group_field: 1})
    assert changed == expected, "The GUI fixture must change only SyncGroupId"
    await call(client, "qkm_apply_changes", {"preview_id": pending["preview_id"]}, error="stale_source")
    after = await snapshot(client)
    assert after["revision"] == group["revision"] and contents(after) == changed
    rejected = await operation(client, pending["preview_id"])
    assert rejected["status"] == "rejected" and rejected["result"]["modified"] is False
    save(product.output / "sync-group-stale.json", {"before": current, "group": group,
        "after_rejection": after, "operation": rejected, "only_changed_field": group_field})
    await product.action("OpenFloatingSetup")
    await product.action("SyncGroupZero")
    await product.action("CloseFloatingSetup")
    restored = await snapshot(client)
    assert contents(restored) == contents(current)
    return restored


async def prepare_mcp_mapping(product, client):
    from mapping_changes_acceptance import contents, operation, preview, snapshot
    before = await snapshot(client)
    assert len(before["rows"]) == 1 and source_rows({"items": before["rows"]}) == [("A", ["B"])]
    ini = product.executable.parent / "keymapdata.ini"
    disk = ini.read_bytes()
    cleared = await product.action("ClearBasicMapping")
    empty = await snapshot(client)
    assert cleared["mapping_rows"] == [] and contents(empty) == []
    planned = await preview(client, empty)
    applied = (await call(client, "qkm_apply_changes", {"preview_id": planned["preview_id"]}))["result"]
    actual = await snapshot(client)
    save(product.output / "mcp-rebuilt-fields.json", {"before": before, "actual": actual})
    assert applied["status"] == "applied" and applied["result"]["saved"] is False
    assert contents(actual) == contents(before), "Performance comparison requires identical effective mapping fields"
    actual = await editor_invalidation(product, client, actual)
    assert ini.read_bytes() == disk
    gui = await product.action("ShowForCapture")
    assert gui["mapping_rows"] == [{"original": "A", "mapping": "B"}] and gui["add_button_enabled"]
    save_display(product.output / "display-before-capture.json", product.window)
    await product.capture(product.window, "map67_mcp_added")
    pending = await preview(client, actual)
    save(product.output / "mcp-added.json", {"before": before, "empty": empty, "actual": actual,
        "operation": applied, "gui": gui, "ini_unchanged": True,
        "known_operation": await operation(client, planned["preview_id"])})
    return actual, pending["preview_id"]


async def run_state(args, state, executable):
    output = args.output / state
    output.mkdir()
    save_display(output / "display-before.json")
    # A failed earlier run must not leave pairings in this run's application namespace.
    runtime = output / "runtime"
    shutil.copytree(executable.parent, runtime, ignore=shutil.ignore_patterns("keymapdata.ini", "*.log"))
    executable = runtime / executable.name
    ini = executable.parent / "keymapdata.ini"
    ini.write_text(fixture(), encoding="utf-8")
    product = Product(executable, output, default_desktop=True)
    receiver, sender, store = None, None, None
    stop, tasks, samples, requests = asyncio.Event(), [], [], []
    try:
        async with AsyncExitStack() as stack:
            await product.started()
            save_display(output / "display-product.json", product.window)
            gui = await product.action("ReadMapping")
            assert gui["mapping_rows"] == [{"original": "A", "mapping": "B"}]
            assert gui["configuration_display"] == "Engine61" and gui["stop_shortcut_edit"] == "F24"
            await product.action("OpenAdvanced")
            if state in ("idle", "query"):
                await product.action("OpenMcp")
                enabled = await product.action("Enable")
                discovery = Path(enabled["discovery"])
                await product.wait(discovery.is_file)
                own = identity(os.getpid())
                store = CredentialStore(discovery, executable, own.session)
                assert store.load() is None, "Use an unpaired test runtime"
                client = await stack.enter_async_context(Client(parameters(discovery, executable, output)))
                await call(client, "qkm_connect")
                await connection(client, "pairing_required")
                await call(client, "qkm_request_pairing")
                await connection_pending(client)
                await product.action("ApproveConfigure" if state == "query" else "ApproveRead", own.executable)
                connected = await connection(client, "connected")
                assert connected["scopes"] == (3 if state == "query" else 1)
                await product.action("CloseMcp")
            mcp_source, pending_preview = None, None
            if state == "query":
                mcp_source, pending_preview = await prepare_mcp_mapping(product, client)
            started = await product.action("StartMapping")
            if state == "query":
                assert not started["mapping_action"]["add_button_enabled"], "GUI addition was enabled while running"
            receiver = Receiver()
            await receiver.start(output / "receiver", allow_focus_click=args.allow_focus_click)
            sender = Sender(receiver.window, receiver.pid, allow_default=args.allow_default)
            warmup = await plain(receiver, sender, 10, 0x42)
            save(output / "warmup.json", warmup)
            if state == "query":
                running = (await call(client, "qkm_get_state"))["result"]
                assert running["run"]["worker_state_sample"] == "started" and running["run"]["monitoring_enabled"]
                await call(client, "qkm_apply_changes", {"preview_id": pending_preview}, error="context_busy")
                await call(client, "qkm_preview_changes", {"source_revision": mcp_source["revision"],
                    "source_tab_ref": mcp_source["target"]["source_tab_ref"], "original_key": "A", "mapping_key": "B"}, error="context_busy")
                rejected = (await call(client, "qkm_get_operation", {"operation_id": pending_preview}))["result"]
                assert rejected["status"] == "rejected" and rejected["result"]["modified"] is False
                save(output / "running-edit-rejection.json", {"gui": started, "state": running, "operation": rejected})
            tasks.append(asyncio.create_task(measurements(product, stop, samples, thread_cpu=args.thread_cpu)))
            if state == "query":
                tasks.append(asyncio.create_task(queries(client, stop, requests)))
            rounds = []
            for index in range(args.rounds):
                for task in tasks:
                    if task.done():
                        task.result()
                result = await plain(receiver, sender, args.pairs, 0x42)
                save(output / ("round-" + str(index + 1) + ".json"), result)
                rounds.append(result["latency_ms"])
                print("PASS", state, "round", index + 1, result["latency_ms"], flush=True)
            stop.set()
            await asyncio.gather(*tasks)
            tasks.clear()
            samples.append(process_sample(product.process, product.pid, product.window, thread_cpu=args.thread_cpu))
            sender.release()
            receiver.reset()
            try:
                stop_input = [sender.send(0x87, True)]
                await asyncio.sleep(.02)
                stop_input.append(sender.send(0x87, False))
            finally:
                sender.release()
            await asyncio.sleep(1)
            save(output / "stop-shortcut.json", {"sent": stop_input, "received": await receiver.flush()})
            save(output / "stopped.json", await plain(receiver, sender, 10, 0x41))
            stopped = await product.action("StopMapping")
            assert stopped["mapping_action"]["before"] == "映射开始", "F24 did not reach GUI idle"
            if state == "query":
                from mapping_changes_acceptance import contents, snapshot
                after_stop = await snapshot(client)
                assert contents(after_stop) == contents(mcp_source)
                assert after_stop["listing"]["source_snapshot"]["read_basis"] == "completed_cleanup"
                save(output / "mcp-after-stop.json", after_stop)
            assert len(samples) >= 2
            cpu = (samples[-1]["cpu_100ns"] - samples[0]["cpu_100ns"]) / 1e7
            elapsed = (samples[-1]["qpc_ns"] - samples[0]["qpc_ns"]) / 1e9
            result = {"state": state, "rounds": rounds, "samples": samples, "requests": requests,
                "cpu_core_percent": cpu / elapsed * 100, "elapsed_s": elapsed,
                "wakeups": "unmeasured; thread counts are not wakeup counts",
                "query_latency_ms": distribution([r["ms"] for r in requests]) if requests else None,
                "executable": str(executable), "exe_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
                "controller_pid": os.getpid(), "pairs_per_round": args.pairs,
                "thread_cpu_enabled": args.thread_cpu,
                "gui_thread_id": win32process.GetWindowThreadProcessId(product.window)[0],
                "scope": "ordinary Release, real SendInput A->B, owned ordinary receiver, Default; not engine-only latency"}
            save(output / "result.json", result)
            if state in ("idle", "query"):
                await call(client, "qkm_disconnect")
                await product.action("OpenAdvanced")
                await product.action("OpenMcp")
                await product.action("Revoke")
                await product.action("Disable")
            return result
    finally:
        save_display(output / "display-after.json", getattr(product, "window", None))
        stop.set()
        try:
            if tasks:
                await asyncio.gather(*tasks)
        finally:
            try:
                if sender is not None:
                    sender.release()
            finally:
                try:
                    await product.close()
                finally:
                    try:
                        if receiver is not None:
                            await receiver.close()
                    finally:
                        if store is not None:
                            store.forget()


async def sources_after_stop(product):
    own, store, approved = identity(os.getpid()), None, False
    try:
        await product.action("OpenAdvanced")
        await product.action("OpenMcp")
        enabled = await product.action("Enable")
        discovery = Path(enabled["discovery"])
        await product.wait(discovery.is_file)
        store = CredentialStore(discovery, product.executable, own.session)
        assert store.load() is None
        async with Client(parameters(discovery, product.executable, product.output)) as client:
            try:
                await call(client, "qkm_connect")
                await connection(client, "pairing_required")
                await call(client, "qkm_request_pairing")
                await connection_pending(client)
                await product.action("ApproveRead", own.executable)
                approved = True
                await connection(client, "connected")
                await product.action("CloseMcp")
                gui = await product.action("ReadMapping")
                assert gui["mapping_rows"] == [{"original": "A", "mapping": "B"}]
                ini = product.executable.parent / "keymapdata.ini"
                disk = ini.read_bytes()
                before = (await call(client, "qkm_get_state"))["result"]
                first = (await call(client, "qkm_query", {"entity": "mapping_sources"}))["result"]
                assert first["source_snapshot"]["read_basis"] == "completed_cleanup"
                assert before["run"]["stop_completion"] == "observed" and before["business_revision"] is None
                source = next(item for item in first["items"] if item["kind"] == "normal")
                target = {"entity": "mappings", "source_revision": first["source_snapshot"]["revision"],
                          "source_tab_ref": source["source_tab_ref"]}
                rows = (await call(client, "qkm_query", target))["result"]
                assert source_rows(rows) == [("A", ["B"])]
                assert (await call(client, "qkm_query", {"entity": "mapping_sources"}))["result"] == first
                assert business_state((await call(client, "qkm_get_state"))["result"]) == business_state(before) and ini.read_bytes() == disk
                await product.action("StartMapping")
                running = (await call(client, "qkm_get_state"))["result"]
                assert running["run"]["monitoring_enabled"] and running["run"]["stop_completion"] == "unobserved"
                await call(client, "qkm_query", target, error="context_busy")
                assert (await call(client, "qkm_get_state"))["result"]["run"]["monitoring_enabled"], "Read rejection stopped mapping"
                await product.action("StopMapping")
                second_disk = ini.read_bytes()
                second_state = (await call(client, "qkm_get_state"))["result"]
                second = (await call(client, "qkm_query", {"entity": "mapping_sources"}))["result"]
                assert second["source_snapshot"]["read_basis"] == "completed_cleanup"
                assert int(second["source_snapshot"]["cleanup_generation"]) > int(first["source_snapshot"]["cleanup_generation"])
                await call(client, "qkm_query", target, error="stale_source")
                new_source = next(item for item in second["items"] if item["kind"] == "normal")
                new_target = {"entity": "mappings", "source_revision": second["source_snapshot"]["revision"],
                              "source_tab_ref": new_source["source_tab_ref"]}
                fresh = (await call(client, "qkm_query", new_target))["result"]
                assert source_rows(fresh) == source_rows(rows)
                assert fresh["items"][0]["fields"] == rows["items"][0]["fields"]
                assert ini.read_bytes() == second_disk and business_state((await call(client, "qkm_get_state"))["result"]) == business_state(second_state)
                save(product.output / "sources-after-stop.json", {"before": before, "first": first,
                    "rows": rows, "running": running, "second_state": second_state, "second": second, "fresh": fresh,
                    "read_batches_ini_unchanged": True, "gui_cycle_ini_unchanged": second_disk == disk})
            finally:
                if approved:
                    await call(client, "qkm_disconnect")
                    await product.action("OpenAdvanced")
                    await product.action("OpenMcp")
                    await product.action("Revoke")
                    await product.action("Disable")
    finally:
        if store is not None:
            store.forget()


async def late_force_release(product, receiver, sender, warmup, *, allowed=False):
    assert allowed is True, "Tagged late-release input requires its explicit authorization"
    tags = {event["extra_info"] for event in warmup["received"] if event["key"] == 0x42}
    assert len(tags) == 1, "Mapped B did not provide one product tag"
    receiver.reset()
    sent, protected, cleanup = [], False, []
    try:
        sent.append(sender.send(0x41, True))
        await wait_for(receiver, sender, lambda: receiver.count(0x41) == 1 and pressed(0x41))
        sent.append(sender.force_original_up(tags.pop()))
        await asyncio.sleep(.15)
        sender.check_target()
        protected = pressed(0x41) and receiver.count(0x41) == 1
        assert protected, "A late forced release escaped after stop and released the held original A"
        sent.append(sender.send(0x41, False))
        await wait_for(receiver, sender, lambda: receiver.count(0x41) == 2 and not pressed(0x41))
    finally:
        cleanup = sorted(sender.held)
        sender.release()
        await asyncio.sleep(.1)
        events = await receiver.flush()
        save(product.output / "late-force-release.json", {"sent": sent, "received": events,
            "held_original_protected": protected, "released_in_cleanup": cleanup})
    assert [(event["key"], event["down"]) for event in events] == [(0x41, True), (0x41, False)]


async def run_feature(args, state, executable, case):
    output = args.output / (state + "_" + case)
    output.mkdir()
    runtime = output / "runtime"
    shutil.copytree(executable.parent, runtime, ignore=shutil.ignore_patterns("keymapdata.ini", "*.log"))
    executable = runtime / executable.name
    receiver, product, sender = Receiver(), None, None
    try:
        await receiver.start(output / "receiver", allow_focus_click=args.allow_focus_click, activate=False)
        text, rows = configuration(fixture(), case, win32gui.GetClassName(receiver.window))
        (runtime / "keymapdata.ini").write_text(text, encoding="utf-8")
        product = Product(executable, output, default_desktop=True)
        await product.started()
        gui = await product.action("ReadMapping")
        assert gui["mapping_rows"] == rows and gui["stop_shortcut_edit"] == "F24", "Feature fixture did not load"
        await product.action("StartMapping")
        await receiver.focus()
        await asyncio.sleep(1)
        sender = Sender(receiver.window, receiver.pid, allow_default=args.allow_default)
        warmup = await plain(receiver, sender, 10, 0x42)
        save(output / "warmup.json", warmup)
        result = await exercise(case, product, receiver, sender)
        save(output / "stopped.json", await plain(receiver, sender, 10, 0x41))
        if not product.closed:
            observed = await product.action("StopMapping")
            assert observed["mapping_action"]["before"] == "映射开始", "Feature stop did not reach GUI idle"
            if case == "hold_stop" and args.allow_late_forced_up:
                await receiver.focus()
                await late_force_release(product, receiver, sender, warmup, allowed=args.allow_late_forced_up)
            if case == "profile_return":
                restored = await product.action("ReadMapping")
                assert restored["configuration_display"] == "Engine61" and restored["mapping_rows"] == rows, \
                    "Stop did not restore the last matched configuration"
            if case in ("quick_restart", "profile_return") and state == "off":
                await sources_after_stop(product)
        result.update(state=state, executable=str(executable),
                      exe_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(), controller_pid=os.getpid())
        save(output / "result.json", result)
        print("PASS", state, case, "real output, release, stopped A and", "normal exit" if product.closed else "GUI idle", flush=True)
        return result
    finally:
        try:
            if sender is not None:
                sender.release()
        finally:
            try:
                if product is not None:
                    await product.close()
            finally:
                await receiver.close()


async def run(args):
    results = []
    for state in args.states:
        executable = args.baseline if state == "baseline" else args.current
        for case in args.cases or [None]:
            result = await run_feature(args, state, executable, case) if case else await run_state(args, state, executable)
            results.append(result)
            expected = len(args.states) * len(args.cases or [None])
            save(args.output / "results.json", {"results": results, "complete": len(results) == expected})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--current", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--states", nargs="+", choices=("baseline", "off", "idle", "query"),
                        default=["baseline", "off", "idle", "query"])
    parser.add_argument("--pairs", type=int, default=250)
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--cases", nargs="+", choices=CASES,
                        help="Fixed original-function cases; use --states baseline off")
    parser.add_argument("--allow-default", action="store_true")
    parser.add_argument("--allow-focus-click", action="store_true")
    parser.add_argument("--thread-cpu", action="store_true",
                        help="Read per-thread CPU/name with ordinary query rights; does not measure wakeups")
    parser.add_argument("--allow-late-forced-up", action="store_true",
                        help="Explicitly authorized product-tagged A release probe; requires --cases hold_stop")
    parser.add_argument("--gsudo-cache", action="store_true")
    args = parser.parse_args()
    reject_nested_cache(args.gsudo_cache)
    args.baseline, args.current, args.output = args.baseline.resolve(), args.current.resolve(), args.output.resolve()
    for name, executable in (("baseline", args.baseline), ("current", args.current)):
        assert executable.is_relative_to(RESULT_ROOT.resolve()) and executable.is_file()
        assert executable.name == "QKeyMapperMcpAcceptance_engine61_" + name + ".exe"
        assert not args.output.is_relative_to(executable.parent), "Output must be outside the runtime template"
    assert args.output.is_relative_to(RESULT_ROOT.resolve()) and args.output != RESULT_ROOT.resolve()
    assert args.allow_default and 10 <= args.pairs <= 500 and 1 <= args.rounds <= 3
    assert len(set(args.states)) == len(args.states)
    if args.cases:
        assert len(set(args.cases)) == len(args.cases) and set(args.states) <= {"baseline", "off"}
    if args.allow_late_forced_up:
        assert args.cases == ["hold_stop"], "Tagged input must be isolated to hold_stop"
    assert elevation(win32api.GetCurrentProcess()) == 0 and integrity(win32api.GetCurrentProcess()) == 0x2000
    args.output.mkdir(parents=True, exist_ok=False)
    asyncio.run(run(args))
