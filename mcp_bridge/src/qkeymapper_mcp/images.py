"""Bounded own-window PNG validation; no filesystem access."""

import base64
import binascii
import hashlib
import math
import struct
import zlib
from datetime import datetime, timedelta

from .protocol import BridgeError, decimal, fields, hex_bytes, require

MAX_PNG_BYTES = 160 * 1024
MAX_PIXELS = 8 * 1024 * 1024


def validate_capture(result: dict, discovery, arguments: dict) -> dict:
    try:
        require(type(result) is dict)
        fields(result, "target role window_ref source_revision source_tab_ref source_context presentation format pixel_width pixel_height "
                       "device_pixel_ratio bytes captured_at_utc png_base64" + (" edit_target" if result.get("role") == "item_setup"
                           else " settings_scope" if result.get("role") == "mapping_advanced" else ""))
        require(all(result[key] == arguments.get(key) for key in ("role", "window_ref", "source_revision", "source_tab_ref")))
        require(result["role"] in ("main", "item_setup", "mapping_advanced") and result["format"] == "image/png")
        if result["role"] == "mapping_advanced":
            require(result["settings_scope"] == "live_loaded_settings_not_committed_by_source_revision")
        hex_bytes(result["window_ref"], 32)
        hex_bytes(result["source_revision"], 32)
        require(type(result["source_tab_ref"]) is str)
        source_parts = result["source_tab_ref"].split(":")
        require(len(source_parts) == 3 and source_parts[:2] == [result["source_revision"], "source"])
        require(decimal(source_parts[2], 32, zero=True) <= 2147483647)
        target = result["target"]
        fields(target, "instance pid created executable desktop")
        require(target["instance"] == discovery.instance
                and decimal(target["pid"], 32) == discovery.server.pid
                and decimal(target["created"], 64) == discovery.server.created
                and target["executable"] == discovery.server.executable)
        require(type(target["desktop"]) is str and 0 < len(target["desktop"]) <= 255)
        context = result["source_context"]
        fields(context, "revision instance reference_scope scope read_basis cleanup_generation configuration_selection "
                        "mapping_load_context common_feature_enabled")
        require(context["revision"] == result["source_revision"] and context["instance"] == discovery.instance
                and context["reference_scope"] == "source_snapshot_only" and context["scope"] == "current_memory_sources"
                and context["read_basis"] in ("pristine_idle", "completed_cleanup"))
        if context["read_basis"] == "completed_cleanup":
            decimal(context["cleanup_generation"], 64)
        else:
            require(context["cleanup_generation"] is None)
        require(result["presentation"] == "live_qt_client_ui_drafts_may_be_visible")
        captured = result["captured_at_utc"]
        require(type(captured) is str and 0 < len(captured) <= 40 and captured.endswith("Z"))
        require(datetime.fromisoformat(captured.replace("Z", "+00:00")).utcoffset() == timedelta(0))
        if result["role"] == "item_setup":
            edit = result["edit_target"]
            fields(edit, "source_tab_ref source_ref source_row")
            require(type(edit["source_tab_ref"]) is str)
            parts = edit["source_tab_ref"].split(":")
            require(parts == source_parts)
            row = edit["source_row"]
            require(type(row) is int and 0 <= row <= 2147483647
                    and edit["source_ref"] == result["source_revision"] + ":row:" + parts[2] + ":" + str(row))
        width, height, size, ratio = (result[key] for key in ("pixel_width", "pixel_height", "bytes", "device_pixel_ratio"))
        require(type(width) is int and type(height) is int and width > 0 and height > 0 and width * height <= MAX_PIXELS)
        require(type(size) is int and 0 < size <= MAX_PNG_BYTES)
        require(type(ratio) in (int, float) and math.isfinite(ratio) and ratio > 0)
        encoded = result["png_base64"]
        require(type(encoded) is str and 0 < len(encoded) <= 4 * ((MAX_PNG_BYTES + 2) // 3))
        png = base64.b64decode(encoded, validate=True)
        require(len(png) == size and png[:8] == b"\x89PNG\r\n\x1a\n")
        position, chunks, idat, ended = 8, 0, False, False
        while position < len(png):
            require(position + 12 <= len(png) and chunks < 128)
            length = int.from_bytes(png[position:position + 4], "big")
            end = position + 12 + length
            require(end <= len(png))
            kind = png[position + 4:position + 8]
            body = png[position + 8:position + 8 + length]
            require(zlib.crc32(png[position + 4:end - 4]) & 0xffffffff == int.from_bytes(png[end - 4:end], "big"))
            if chunks == 0:
                require(kind == b"IHDR" and length == 13)
                require(struct.unpack(">II", body[:8]) == (width, height)
                        and body[8] == 8 and body[9] in (2, 6) and body[10:] == b"\0\0\0")
            else:
                require(kind != b"IHDR")
            idat |= kind == b"IDAT" and length > 0
            position, chunks = end, chunks + 1
            if kind == b"IEND":
                require(length == 0 and idat and end == len(png))
                ended = True
                break
        require(ended)
        return {**result, "sha256": hashlib.sha256(png).hexdigest()}
    except (BridgeError, KeyError, TypeError, ValueError, binascii.Error, struct.error):
        raise BridgeError("invalid_image") from None
