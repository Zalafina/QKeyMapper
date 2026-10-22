import gc
import unittest
import uuid

import win32api
import win32service

from test_sdk_interop import close_desktop, elevation


class DesktopLifecycleTests(unittest.TestCase):
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
