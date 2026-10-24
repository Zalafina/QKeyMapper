import gc
import json
import tempfile
import unittest
import uuid
from pathlib import Path
from unittest.mock import patch

import win32api
import win32service

from test_sdk_interop import Fixture, close_desktop, elevation


class DesktopLifecycleTests(unittest.TestCase):
    def test_fixture_command_retries_sharing_without_changing_sequence(self):
        with tempfile.TemporaryDirectory() as directory:
            fixture = Fixture.__new__(Fixture)
            fixture.state_path, fixture.sequence = Path(directory) / "fixture.json", 0
            locked = PermissionError("fixture sharing")
            locked.winerror = 32
            with patch("test_sdk_interop.os.replace", side_effect=[locked, None]) as replace, \
                 patch("test_sdk_interop.time.sleep") as sleep:
                fixture.command("disable")
            self.assertEqual(replace.call_count, 2)
            sleep.assert_called_once_with(0.01)
            self.assertEqual(json.loads(fixture.state_path.with_name("fixture.json.command.new").read_text()),
                             {"sequence": 1, "command": "disable"})

    def test_fixture_command_does_not_retry_past_deadline_or_other_errors(self):
        for code in (5, 13):
            with self.subTest(code=code), tempfile.TemporaryDirectory() as directory:
                fixture = Fixture.__new__(Fixture)
                fixture.state_path, fixture.sequence = Path(directory) / "fixture.json", 0
                error = PermissionError("fixture failure")
                error.winerror = code
                with patch("test_sdk_interop.os.replace", side_effect=error) as replace, \
                     patch("test_sdk_interop.time.monotonic", side_effect=[0, 1]), \
                     patch("test_sdk_interop.time.sleep") as sleep:
                    with self.assertRaises(PermissionError):
                        fixture.command("quit")
                self.assertEqual(replace.call_count, 1)
                sleep.assert_not_called()

    def test_closed_object_cannot_close_a_replacement_desktop(self):
        self.assertEqual(elevation(win32api.GetCurrentProcess()), 0)
        for _ in range(8):
            desktop = win32service.CreateDesktop("QkmDesktopTest_" + uuid.uuid4().hex, 0, 511, None)
            replacement = None
            try:
                close_desktop(desktop)
                try:
                    self.assertEqual(int(desktop), 0, "A closed object must not retain a reusable native value.")
                except BaseException:
                    desktop.Detach()
                    raise
                name = "QkmDesktopTest_" + uuid.uuid4().hex
                replacement = win32service.CreateDesktop(name, 0, 511, None)
                del desktop
                gc.collect()
                self.assertEqual(win32service.GetUserObjectInformation(replacement, 2), name)
            finally:
                if replacement is not None:
                    close_desktop(replacement)


if __name__ == "__main__":
    unittest.main()
