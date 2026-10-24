"""Harness checks: private-window messages only; no SendInput, mapping or UAC."""

import json
import os
from pathlib import Path
import subprocess
import sys
import threading
import time
import unittest
from unittest.mock import patch
import uuid

import win32api
import win32con
import win32event
import win32gui
import win32process
import win32service

from probe import Sender, focus_click, keyboard_input, plain_latencies, process_sample
from receiver import CLOSE, COUNT, FLUSH, RESET, RESULT_ROOT
from run_acceptance import Receiver


def wait_file(path, process):
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        if path.is_file():
            return json.loads(path.read_text(encoding="utf-8"))
        assert win32event.WaitForSingleObject(process, 0) == win32event.WAIT_TIMEOUT
        time.sleep(.02)
    raise AssertionError("Private receiver did not publish its result")


class ProbeTests(unittest.TestCase):
    def test_input_allowlist_and_layout(self):
        event = keyboard_input(0x41, False)
        self.assertEqual((event.type, event.payload.ki.wVk, event.payload.ki.dwFlags), (1, 0x41, 2))
        for key in (0, 0x42, win32con.VK_CONTROL, win32con.VK_LBUTTON):
            with self.assertRaises(AssertionError):
                keyboard_input(key, True)

    def test_output_counts_order_and_clock(self):
        sent = [{"key": 0x41, "down": True, "qpc_ns": 100},
                {"key": 0x41, "down": False, "qpc_ns": 200}]
        received = [{"key": 0x42, "down": True, "qpc_ns": 1100},
                    {"key": 0x42, "down": False, "qpc_ns": 2200}]
        self.assertEqual(plain_latencies(sent, received, 0x42)["p95"], .002)
        for invalid in (received[:1], received[::-1], [dict(received[0], key=0x41), received[1]],
                        [dict(received[0], qpc_ns=1), received[1]]):
            with self.assertRaises(AssertionError):
                plain_latencies(sent, invalid, 0x42)

    def test_focus_failure_never_injects(self):
        sender = Sender.__new__(Sender)
        sender.window, sender.pid, sender.held = 1, 2, set()
        with patch.object(sender, "check_target", side_effect=AssertionError("focus")), \
                patch("probe.user32.SendInput") as native:
            with self.assertRaises(AssertionError):
                sender.send(0x41, True)
            native.assert_not_called()

    def test_cleanup_only_releases_accepted_source_keys(self):
        sender = Sender.__new__(Sender)
        sender.held = {0x41}
        with patch("probe.user32.SendInput", return_value=1) as native:
            sender.release()
            self.assertEqual(native.call_count, 1)
        self.assertFalse(sender.held)

    def test_timeout_result_uses_message_payload(self):
        receiver = Receiver()
        receiver.window = 123
        with patch("win32gui.SendMessageTimeout", return_value=(1, 22)):
            self.assertEqual(receiver.count(), 22)

    def test_focus_click_needs_its_separate_authorization(self):
        with patch("probe.user32.SendInput") as native, patch("win32api.SetCursorPos") as move:
            with self.assertRaises(AssertionError):
                focus_click(1, 2, 3, allowed=False)
            native.assert_not_called()
            move.assert_not_called()

    def test_obscured_receiver_never_gets_clicked(self):
        with patch("probe.user32.SendInput") as native, patch("win32api.SetCursorPos") as move, \
                patch("win32process.GetWindowThreadProcessId", return_value=(8, 3)), \
                patch("win32api.GetAsyncKeyState", return_value=0), patch("win32gui.IsChild", return_value=True), \
                patch("win32gui.GetWindowRect", return_value=(0, 0, 10, 10)), \
                patch("win32api.GetCursorPos", side_effect=[(20, 20), (5, 5)]), \
                patch("win32gui.SetWindowPos"), patch("win32gui.WindowFromPoint", return_value=999):
            with self.assertRaisesRegex(AssertionError, "obscured"):
                focus_click(1, 2, 3, allowed=True)
            native.assert_not_called()
            self.assertEqual([call.args for call in move.call_args_list], [((5, 5),), ((20, 20),)])

    def test_powershell_default_scope_refused_before_elevation(self):
        root = Path(__file__).resolve().parents[3]
        helper = root / "QKeyMapper/tests/mcp_control/product_ui_actions.ps1"
        common = ["-Action", "StartMapping", "-TargetProcessId", str(os.getpid()), "-StartTicks", "0",
                  "-ExecutablePath", str(root / "out/mcp_product/QKeyMapperMcpAcceptance_guard.exe"),
                  "-ResultFile", str(root / "out/mcp_product/guard.json")]
        for arguments in (["-DesktopName", "Default"],
                          ["-DesktopName", "Default", "-AllowDefaultDesktop"],
                          ["-DesktopName", "QkmProduct_" + "a" * 32, "-AllowDefaultDesktop"]):
            result = subprocess.run(["pwsh", "-NoProfile", "-File", str(helper), *common, *arguments],
                                    capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(b"Select an owned, isolated product fixture.", result.stderr)

    def test_private_native_receiver(self):
        errors = []
        def check():
            try:
                self.check_private_native_receiver()
            except BaseException as error:
                errors.append(error)
        worker = threading.Thread(target=check)
        worker.start()
        worker.join(45)
        self.assertFalse(worker.is_alive(), "Private receiver check exceeded its watchdog")
        if errors:
            raise errors[0]

    def check_private_native_receiver(self):
        desktop_name = "QkmReceiver_" + uuid.uuid4().hex
        desktop = win32service.CreateDesktop(desktop_name, 0, 511, None)
        original = win32service.GetThreadDesktop(win32api.GetCurrentThreadId())
        desktop.SetThreadDesktop()
        output = RESULT_ROOT / ("selftest-" + uuid.uuid4().hex)
        startup = win32process.STARTUPINFO()
        startup.lpDesktop = "winsta0\\" + desktop_name
        command = [sys.executable, "-B", str(Path(__file__).with_name("receiver.py")),
                   "--output", str(output), "--desktop", desktop_name, "--self-test"]
        process = None
        window = None
        try:
            process, thread, pid, _ = win32process.CreateProcess(None, subprocess.list2cmdline(command),
                None, None, False, subprocess.CREATE_NO_WINDOW, dict(os.environ), str(Path.cwd()), startup)
            thread.Close()
            started = wait_file(output / "started.json", process)
            window = started["window"]
            self.assertEqual(started["desktop"], desktop_name)
            self.assertEqual(win32process.GetWindowThreadProcessId(window)[1], started["pid"])
            win32gui.SendMessage(window, RESET, 1, 0)
            # These messages test receiver plumbing, not system input or mapping behavior.
            win32gui.PostMessage(started["focus_window"], win32con.WM_KEYDOWN, 0x42, 0)
            win32gui.PostMessage(started["focus_window"], win32con.WM_KEYUP, 0x42, 0)
            win32gui.PostMessage(window, FLUSH, 1, 0)
            result = wait_file(output / "events-1.json", process)
            self.assertEqual([(e["key"], e["down"]) for e in result["events"]], [(0x42, True), (0x42, False)])
            self.assertEqual(win32gui.SendMessage(window, COUNT, 0, 0), 2)
            sample = process_sample(win32api.GetCurrentProcess(), os.getpid(), window)
            self.assertGreater(sample["threads"], 0)
            self.assertGreater(sample["working_set"], 0)
            win32gui.SendMessage(window, RESET, 2, 0)
            self.assertEqual(win32gui.SendMessage(window, COUNT, 0, 0), 0)
        finally:
            if window and win32gui.IsWindow(window):
                win32gui.PostMessage(window, CLOSE, 0, 0)
            if process is not None:
                try:
                    self.assertEqual(win32event.WaitForSingleObject(process, 10000), win32event.WAIT_OBJECT_0)
                    self.assertEqual(win32process.GetExitCodeProcess(process), 0)
                    self.assertFalse(wait_file(output / "closed.json", process)["aborted"])
                finally:
                    process.Close()
            original.SetThreadDesktop()
            original.Detach()
            desktop.CloseDesktop()
            desktop.Detach()
        print("Private receiver evidence:", output, flush=True)


if __name__ == "__main__":
    unittest.main()
