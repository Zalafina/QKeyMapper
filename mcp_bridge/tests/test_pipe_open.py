"""Connection-slot classification; mocked errors do not prove real pipe concurrency."""

import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch

import pywintypes

from qkeymapper_mcp.protocol import BridgeError
from qkeymapper_mcp.windows import Pipe


class PipeOpenTests(unittest.TestCase):
    def test_observed_busy_deadline_is_specific(self):
        for wait_error in (None, pywintypes.error(121, "WaitNamedPipe", "mock timeout")):
            with self.subTest(wait_error=wait_error), \
                    patch.object(Pipe, "_remaining", side_effect=[1, BridgeError("timeout")]), \
                    patch("qkeymapper_mcp.windows.win32file.CreateFile", side_effect=pywintypes.error(231, "CreateFile", "mock busy")), \
                    patch("qkeymapper_mcp.windows.win32pipe.WaitNamedPipe", side_effect=wait_error), \
                    self.assertRaises(BridgeError) as failure:
                Pipe(SimpleNamespace(pipe="unused"), None, None)
            self.assertEqual(failure.exception.code, "pipe_busy")

    def test_timeout_without_busy_is_not_reclassified(self):
        with patch.object(Pipe, "_remaining", side_effect=BridgeError("timeout")), \
                patch("qkeymapper_mcp.windows.win32file.CreateFile") as create, self.assertRaises(BridgeError) as failure:
            Pipe(SimpleNamespace(pipe="unused"), None, None)
        self.assertEqual(failure.exception.code, "timeout")
        create.assert_not_called()

    def test_cancel_after_busy_stays_cancelled(self):
        with patch.object(Pipe, "_remaining", side_effect=[1, BridgeError("cancelled")]), \
                patch("qkeymapper_mcp.windows.win32file.CreateFile", side_effect=pywintypes.error(231, "CreateFile", "mock busy")), \
                patch("qkeymapper_mcp.windows.win32pipe.WaitNamedPipe"), self.assertRaises(BridgeError) as failure:
            Pipe(SimpleNamespace(pipe="unused"), None, None)
        self.assertEqual(failure.exception.code, "cancelled")

    def test_other_native_failure_stays_unavailable(self):
        with patch.object(Pipe, "_remaining", return_value=1), \
                patch("qkeymapper_mcp.windows.win32file.CreateFile", side_effect=pywintypes.error(5, "CreateFile", "mock denied")), \
                self.assertRaises(BridgeError) as failure:
            Pipe(SimpleNamespace(pipe="unused"), None, None)
        self.assertEqual(failure.exception.code, "pipe_unavailable")

    def test_existing_bounded_wait_can_succeed_without_new_target(self):
        handle = Mock()
        with patch.object(Pipe, "_remaining", return_value=1), \
                patch("qkeymapper_mcp.windows.win32file.CreateFile", side_effect=[pywintypes.error(231, "CreateFile", "mock busy"), handle]) as create, \
                patch("qkeymapper_mcp.windows.win32pipe.WaitNamedPipe") as wait, patch.object(Pipe, "verify") as verify:
            pipe = Pipe(SimpleNamespace(pipe="same-pipe"), None, None)
        self.assertEqual([call.args[0] for call in create.call_args_list], ["same-pipe", "same-pipe"])
        wait.assert_called_once_with("same-pipe", 100)
        verify.assert_called_once_with()
        pipe.close()
        handle.Close.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
