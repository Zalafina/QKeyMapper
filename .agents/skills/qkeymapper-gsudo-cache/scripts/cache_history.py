"""Bounded local health events; no commands, cache probes or authorization decisions."""

from datetime import datetime, timedelta
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import re

from cache_core import CHINA, PROFILES

HEARTBEAT_SECONDS = 60
KEEP_DAYS = 30
FILE_BYTES = 1024 * 1024
TOTAL_BYTES = 20 * 1024 * 1024
NAME = re.compile(r"events-([0-9]{8})-([0-9]{4,10})\.jsonl\Z")
TOKEN = re.compile(r"[a-zA-Z][a-zA-Z0-9_]{0,63}\Z")


def safe_time(value):
    if type(value) is not str or len(value) > 64:
        return None
    try:
        parsed = datetime.fromisoformat(value)
        return parsed.astimezone(CHINA).isoformat() if parsed.tzinfo else None
    except ValueError:
        return None


def safe_fields(fields):
    result = {}
    for key, value in fields.items():
        if key in {"operation", "action", "error", "close_reason", "job_kind", "resource_error", "backend", "entry_type", "target_id"}:
            if type(value) is str and (re.fullmatch(r"[a-zA-Z][a-zA-Z0-9_-]{0,63}", value) if key == "target_id" else TOKEN.fullmatch(value)):
                result[key] = value
        elif key in {"generation", "remaining_seconds", "budget_seconds", "native_exit_code", "exit_code",
                     "working_set_bytes", "cpu_seconds", "windows_session_id", "integrity", "target_pid"}:
            if type(value) in (int, float) and -2**32 <= value <= 10**18 and math.isfinite(value):
                result[key] = value
        elif key in {"ok", "restore", "cache_available", "deadline_expired", "requested_window_expired", "closing", "cleanup_complete", "cleanup"}:
            if type(value) is bool or value is None:
                result[key] = value
        elif key in {"request_id", "pending_request"}:
            if type(value) is str and re.fullmatch(r"[0-9a-f]{32}", value):
                result[key] = value
        elif key in {"deadline", "previous_deadline", "requested_deadline", "native_checked_at"}:
            result[key] = safe_time(value)
        elif key in {"machine_id", "login_tag", "code_version", "definition_hash"}:
            if type(value) is str and re.fullmatch(r"[0-9a-f]{12}|[0-9a-f]{64}", value):
                result[key] = value
        elif key in {"os_version", "python_version", "gsudo_version"}:
            if type(value) is str and re.fullmatch(r"[a-zA-Z0-9 ._+-]{1,96}", value):
                result[key] = value
        elif key in {"scope", "input_flags"} and type(value) is list:
            allowed = PROFILES if key == "scope" else {"allow_default", "allow_focus_click", "allow_late_forced_up"}
            result[key] = sorted({x for x in value if type(x) is str and x in allowed})
    return result


def environment(directory):
    """Identify only this implementation, rather than hashing the whole workspace."""
    context = {"machine_id": hashlib.sha256(platform.node().encode("utf-8")).hexdigest()[:12],
               "os_version": platform.version(), "python_version": platform.python_version()}
    try:
        digest = hashlib.sha256()
        for name in ("cache_control.py", "cache_core.py", "cache_history.py", "cache_actions.py"):
            digest.update(name.encode("ascii") + b"\0" + (directory / name).read_bytes())
        context["code_version"] = digest.hexdigest()
    except OSError:
        pass
    return context


class History:
    def __init__(self, runtime, instance, pid, created, tick, wall, context):
        self.runtime, self.directory = Path(runtime), Path(runtime) / "history"
        self.instance, self.pid, self.created = instance, pid, created
        self.tick, self.wall, self.started = tick, wall, tick()
        self.context = safe_fields(context)
        self.sequence = self.missed = 0
        self.last_error = None

    def status(self):
        return {"available": self.last_error is None if self.sequence else None,
                "missed_events": self.missed, "last_error": self.last_error}

    def failed(self, code):
        self.missed += 1
        self.last_error = code

    def _safe(self, path, parent):
        if path.resolve().parent != parent.resolve():
            raise OSError("unsafe_history_path")
        try:
            info = path.lstat()
        except FileNotFoundError:
            return
        if info.st_file_attributes & 0x400 or info.st_nlink != 1:
            raise OSError("unsafe_history_path")

    def _files(self):
        files = []
        for path in self.directory.glob("events-*.jsonl"):
            match = NAME.fullmatch(path.name)
            if match:
                self._safe(path, self.directory)
                if not path.is_file():
                    raise OSError("invalid_history_file")
                files.append((match[1], int(match[2]), path, path.stat().st_size))
        return sorted(files)

    def _append(self, day, line):
        self._safe(self.directory, self.runtime)
        self.directory.mkdir(exist_ok=True)
        files = self._files()
        cutoff = (day - timedelta(days=KEEP_DAYS)).strftime("%Y%m%d")
        for date, _, path, _ in files:
            if date < cutoff:
                path.unlink()
        files = [x for x in files if x[0] >= cutoff]
        today = day.strftime("%Y%m%d")
        same_day = [x for x in files if x[0] == today]
        required = len(line) + 1
        if required > FILE_BYTES or required > TOTAL_BYTES:
            raise OSError("history_record_too_large")
        index = same_day[-1][1] if same_day else 0
        if same_day and same_day[-1][3] + required > FILE_BYTES:
            index += 1
        path = self.directory / f"events-{today}-{index:04d}.jsonl"
        total = sum(x[3] for x in files)
        for _, _, old, size in files:
            if total + required <= TOTAL_BYTES:
                break
            old.unlink()
            total -= size
        self._safe(path, self.directory)
        with path.open("ab+") as stream:
            # Preserve a partial old line as a visible gap; keep later records readable.
            stream.seek(0, os.SEEK_END)
            if stream.tell():
                stream.seek(-1, os.SEEK_END)
                if stream.read(1) != b"\n":
                    stream.write(b"\n")
            if stream.write(line) != len(line):
                raise OSError("history_short_write")

    def emit(self, event, **fields):
        self.sequence += 1
        try:
            if not TOKEN.fullmatch(event):
                raise ValueError("invalid_history_event")
            now = self.wall().astimezone(CHINA)
            row = {**self.context, "schema_version": 1, "instance": self.instance,
                   "pid": self.pid, "process_created": self.created, "sequence": self.sequence,
                   "timestamp": now.isoformat(timespec="milliseconds"),
                   "elapsed_seconds": round(max(0, self.tick() - self.started), 3),
                   "missed_events": self.missed, "event": event, **safe_fields(fields)}
            line = json.dumps(row, ensure_ascii=False, allow_nan=False, separators=(",", ":")).encode("utf-8") + b"\n"
            self._append(now.date(), line)
            self.last_error = None
        except Exception:
            self.failed("history_write_failed")
