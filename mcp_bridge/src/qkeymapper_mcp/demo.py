"""Read-only checks for one explicitly configured, prewarmed private instance."""

import os
import re
from contextlib import ExitStack, closing
from pathlib import Path

import pywintypes
import win32file

from .protocol import BridgeError, decimal, decode, fields, hex_bytes, require
from .windows import _private

DEMO_SCOPES = 1 | 2 | 128 | 256


def read_binding(path: Path, own) -> dict:
    try:
        with ExitStack() as stack:
            stack.enter_context(closing(_private(path.parent, own.user, True)))
            handle = stack.enter_context(closing(_private(path, own.user, False)))
            size = win32file.GetFileSize(handle)
            require(0 < size <= 8192)
            _, raw = win32file.ReadFile(handle, size)
            require(len(raw) == size)
            return decode(raw, 8192)
    except (BridgeError, pywintypes.error, OSError):
        raise BridgeError("demo_record_rejected") from None


def validate_binding(value: dict, target, expected_executable: Path) -> dict:
    try:
        fields(value, "version executable pid created session instance desktop configuration")
        require(type(value["version"]) is int and value["version"] == 1)
        hex_bytes(value["instance"], 16)
        require((value["instance"], decimal(value["pid"], 32), decimal(value["created"], 64),
                 decimal(value["session"], 32, zero=True))
                == (target.instance, target.server.pid, target.server.created, target.server.session))
        path = value["executable"]
        require(type(path) is str and os.path.isabs(path) and "\0" not in path)
        normalized = lambda text: os.path.normcase(os.path.normpath(str(text)))
        require(normalized(path) == normalized(target.server.executable) == normalized(expected_executable))
        desktop = value["desktop"]
        require(type(desktop) is str and re.fullmatch(r"[A-Za-z0-9_.-]{1,120}", desktop) is not None
                and desktop.casefold() != "default")
        require(type(value["configuration"]) is str and 0 < len(value["configuration"]) <= 1024
                and "\0" not in value["configuration"])
        return value
    except (BridgeError, KeyError, TypeError, ValueError):
        raise BridgeError("demo_record_mismatch") from None


def readiness(binding: dict, windows: dict, state: dict, sources: dict, granted: int) -> str:
    """No business mutation or reset; fail closed when the baseline cannot be proved."""
    if not all(type(value) is dict for value in (windows, state, sources)):
        return "invalid_demo_state"
    target = windows.get("target", {})
    if target != {key: binding[key] for key in ("instance", "pid", "created", "executable", "desktop")}:
        return "demo_record_mismatch"
    if (granted & DEMO_SCOPES) != DEMO_SCOPES:
        return "permission_denied"
    if windows.get("private_background_available") is not True:
        return "unsupported_desktop"
    if state.get("instance") != binding["instance"]:
        return "demo_record_mismatch"
    configuration = state.get("configuration", {})
    load = state.get("mapping_load_context", {})
    if type(configuration) is not dict or type(load) is not dict:
        return "invalid_demo_state"
    if (configuration.get("group") != binding["configuration"] or configuration.get("kind") != "saved_profile"
            or load.get("group") != binding["configuration"] or load.get("outcome") != "accepted"):
        return "demo_configuration_mismatch"
    if configuration.get("has_unsaved_changes") is not False:
        return "demo_baseline_dirty"
    run = state.get("run", {})
    snapshot = sources.get("source_snapshot", {})
    if type(run) is not dict or type(snapshot) is not dict:
        return "invalid_demo_state"
    if (run.get("gui_state") != "idle" or run.get("worker_state_sample") != "stopped"
            or snapshot.get("read_basis") not in ("pristine_idle", "completed_cleanup")):
        return "context_busy"
    items = sources.get("items")
    if (snapshot.get("instance") != binding["instance"] or snapshot.get("common_feature_enabled") is not False
            or sources.get("total") != 2 or sources.get("offset") != 0 or sources.get("next_offset") is not None
            or type(items) is not list or len(items) != 2 or not all(type(item) is dict for item in items)):
        return "demo_baseline_not_empty"
    if (snapshot.get("configuration_selection") != configuration or snapshot.get("mapping_load_context") != load):
        return "context_busy"
    source, backup = items
    if (source.get("attached_to_ui") is not True or source.get("kind") != "normal"
            or source.get("ui_index") != 0 or source.get("row_count") != 0
            or source.get("include_common") is not False or state.get("current_tab", {}).get("index") != 0
            or backup.get("attached_to_ui") is not False or backup.get("kind") != "common"
            or backup.get("ui_index") is not None or backup.get("row_count") != 0):
        return "demo_baseline_not_empty"
    roles = windows.get("items", [])
    if (type(roles) is not list or not all(type(item) is dict for item in roles)
            or [item.get("role") for item in roles] != ["main", "item_setup"]):
        return "window_unavailable"
    main, editor = roles
    if (not all(main.get(key) is True for key in ("qt_visible", "native_ready", "native_visible"))
            or main.get("minimized") is not False or editor.get("qt_visible") is not False):
        return "window_unavailable"
    return "ready"
