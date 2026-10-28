import asyncio
import base64
import copy
import hashlib
import json
import os
import struct
import unittest
import zlib
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock, patch

from qkeymapper_mcp.images import MAX_PNG_BYTES, validate_capture
from qkeymapper_mcp.protocol import BridgeError


def fixture():
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 2, 1, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(b"\0" + b"\x80\x40\x20" * 2)) + chunk(b"IEND", b""))
    discovery = SimpleNamespace(instance="a" * 32, server=SimpleNamespace(pid=10, created=100, executable="test.exe"))
    arguments = dict(role="main", window_ref="b" * 64, source_revision="c" * 64)
    arguments["source_tab_ref"] = arguments["source_revision"] + ":source:0"
    value = {**arguments, "target": dict(instance=discovery.instance, pid="10", created="100", executable="test.exe", desktop="Private"),
        "source_context": dict(revision=arguments["source_revision"], instance=discovery.instance,
            reference_scope="source_snapshot_only", scope="current_memory_sources", read_basis="pristine_idle",
            cleanup_generation=None, configuration_selection={}, mapping_load_context={}, common_feature_enabled=False),
        "presentation": "live_qt_client_ui_drafts_may_be_visible", "format": "image/png", "pixel_width": 2,
        "pixel_height": 1, "bytes": len(png), "device_pixel_ratio": 1, "captured_at_utc": "2026-10-28T00:00:00.000Z",
        "png_base64": base64.b64encode(png).decode("ascii")}
    return value, discovery, arguments, png


class ImageTests(unittest.TestCase):
    def test_valid_png_and_no_duplicate_base64(self):
        from qkeymapper_mcp.__main__ import image_tool_result
        value, discovery, arguments, png = fixture()
        checked = validate_capture(value, discovery, arguments)
        self.assertEqual(checked["sha256"], hashlib.sha256(png).hexdigest())
        result = image_tool_result({"id": "d" * 32, "ok": True, "result": checked})
        self.assertFalse(result.is_error)
        self.assertEqual(result.content[0].type, "image")
        self.assertEqual(base64.b64decode(result.content[0].data), png)
        self.assertEqual(result.content[0].mime_type, "image/png")
        self.assertNotIn("png_base64", result.structured_content["result"])
        self.assertNotIn(value["png_base64"], result.content[1].text)
        self.assertEqual(json.loads(result.content[1].text), result.structured_content)

    def test_failures_have_no_image(self):
        from qkeymapper_mcp.__main__ import image_tool_result
        for value in ({"ok": False, "error": "permission_denied"}, {"ok": False, "error": "capture_timeout"},
                      {"ok": False, "error": "stale_window"}, {"ok": True, "result": {}}):
            with self.subTest(value=value):
                result = image_tool_result(value)
                self.assertTrue(result.is_error)
                self.assertTrue(all(item.type == "text" for item in result.content))

    def test_identity_request_and_bounds(self):
        original, discovery, arguments, _ = fixture()
        changes = [("role", "item_setup"), ("window_ref", "d" * 64), ("source_revision", "d" * 64),
                   ("pixel_width", True), ("pixel_width", 8 * 1024 * 1024 + 1), ("pixel_height", 0),
                   ("bytes", MAX_PNG_BYTES + 1), ("bytes", True), ("device_pixel_ratio", float("nan")),
                   ("device_pixel_ratio", True), ("captured_at_utc", "2026-10-28"),
                   ("png_base64", "A" * (4 * ((MAX_PNG_BYTES + 2) // 3) + 4)), ("png_base64", "%%%"),
                   ("unknown", 1)]
        for key, value in changes:
            with self.subTest(key=key, value=str(value)[:30]), self.assertRaises(BridgeError) as raised:
                validate_capture({**original, key: value}, discovery, arguments)
            self.assertEqual(raised.exception.code, "invalid_image")
        for key, value in (("pid", "11"), ("created", "101"), ("instance", "d" * 32), ("executable", "other.exe"), ("desktop", "")):
            changed = copy.deepcopy(original)
            changed["target"][key] = value
            with self.subTest(target=key), self.assertRaises(BridgeError):
                validate_capture(changed, discovery, arguments)
        for value in (None, [], {}, {key: value for key, value in original.items() if key != "source_context"}):
            with self.subTest(value=type(value)), self.assertRaises(BridgeError):
                validate_capture(value, discovery, arguments)

    def test_png_damage(self):
        original, discovery, arguments, png = fixture()
        damaged = bytearray(png)
        damaged[-1] ^= 1
        for data in (png[:-1], png + b"trailing", bytes(damaged), b"not png"):
            with self.subTest(size=len(data)), self.assertRaises(BridgeError):
                validate_capture({**original, "png_base64": base64.b64encode(data).decode("ascii"), "bytes": len(data)}, discovery, arguments)

    def test_item_target_matches_source(self):
        original, discovery, arguments, _ = fixture()
        arguments["role"] = original["role"] = "item_setup"
        original["edit_target"] = dict(source_tab_ref=arguments["source_tab_ref"],
            source_ref=arguments["source_revision"] + ":row:0:0", source_row=0)
        self.assertEqual(validate_capture(original, discovery, arguments)["edit_target"], original["edit_target"])
        for key, value in (("source_tab_ref", arguments["source_revision"] + ":source:1"),
                           ("source_tab_ref", 1), ("source_ref", arguments["source_revision"] + ":row:0:1"),
                           ("source_row", True)):
            changed = copy.deepcopy(original)
            changed["edit_target"][key] = value
            with self.subTest(key=key), self.assertRaises(BridgeError):
                validate_capture(changed, discovery, arguments)

    def test_source_context_must_be_current(self):
        original, discovery, arguments, _ = fixture()
        for key, value in (("revision", "d" * 64), ("instance", "d" * 32), ("read_basis", "stopped"),
                           ("cleanup_generation", "1"), ("png_base64", "private")):
            changed = copy.deepcopy(original)
            changed["source_context"][key] = value
            with self.subTest(key=key), self.assertRaises(BridgeError):
                validate_capture(changed, discovery, arguments)

    @unittest.skipUnless(os.name == "nt", "Windows bridge worker")
    def test_invalid_image_disconnects_without_returning_bytes(self):
        from qkeymapper_mcp.bridge import Bridge
        from qkeymapper_mcp.protocol import Credential
        value, discovery, arguments, _ = fixture()
        value["pixel_width"] = 3
        pipe = Mock(discovery=discovery)
        pipe.receive.return_value = {"id": "d" * 32, "ok": True, "result": value}
        bridge = Bridge(Path("unused/instance.json"), Path("test.exe"))
        bridge._pipe = pipe
        bridge._credential = Credential("e" * 32, 1, 257, b"k" * 32)
        try:
            with patch("qkeymapper_mcp.bridge.uuid.uuid4", return_value=SimpleNamespace(hex="d" * 32)):
                self.assertEqual(asyncio.run(bridge.capture_window(**arguments)), {"ok": False, "error": "invalid_image"})
            pipe.close.assert_called_once()
            self.assertEqual(bridge.state()["status"], "failed")
            self.assertIsNone(bridge._pipe)
        finally:
            bridge.close()


if __name__ == "__main__":
    unittest.main()
