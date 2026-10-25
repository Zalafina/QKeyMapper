"""Workspace-only, ordinary-process gsudo cache controller. No arbitrary command runner."""

import argparse
from collections import OrderedDict
from contextlib import closing
from dataclasses import asdict
from datetime import datetime, timedelta, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import uuid

from cache_core import (CHINA, DEFAULT_SECONDS, CacheError, Window, build_job,
                        parse_command, require, seconds_ok, validate_scope)
from cache_history import HEARTBEAT_SECONDS, History, environment
from cache_actions import Actions, action_command, TICKS_TO_FILETIME, file_hash

SCRIPT = Path(__file__).resolve()
ROOT = next(p for p in SCRIPT.parents if (p / "QKeyMapper/QKeyMapper.pro").is_file())
sys.path.insert(0, str(ROOT / "mcp_bridge/src"))
from qkeymapper_mcp.windows import Identity, identity
from qkeymapper_mcp.protocol import BridgeError
import pywintypes
import win32api
import win32con
import win32event
import win32file
import win32process
import win32security

RUNTIME = ROOT / "out/gsudo_cache"
HEX_ID = re.compile(r"[0-9a-f]{32}\Z")
NO_WINDOW = subprocess.CREATE_NO_WINDOW


def utc_now():
    return datetime.now(timezone.utc)


def token_info(pid=None):
    with closing(win32api.OpenProcess(0x1000, False, pid or os.getpid())) as process:
        with closing(win32security.OpenProcessToken(process, win32con.TOKEN_QUERY)) as token:
            return {"elevated": bool(win32security.GetTokenInformation(token, win32security.TokenElevation)),
                "integrity": int(win32security.ConvertSidToStringSid(win32security.GetTokenInformation(token, 25)[0]).rsplit("-", 1)[1])}


def read_json(path, limit=65536):
    require(path.is_file() and path.stat().st_size <= limit, "运行记录不存在或超限。", "invalid_record")
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path, value):
    temporary = path.with_name(path.name + "." + uuid.uuid4().hex + ".tmp")
    temporary.write_bytes(json.dumps(value, ensure_ascii=False, indent=2).encode("utf-8"))
    temporary.replace(path)


def error_result(error):
    return {"ok": False, "error": error.code if isinstance(error, CacheError) else "internal_error",
        "message": str(error) if isinstance(error, CacheError) else "控制器操作失败；请检查本机运行记录，不自动重试。"}


def runtime_safe():
    require(RUNTIME.resolve().is_relative_to(ROOT / "out")
        and all((RUNTIME / part).resolve().is_relative_to(RUNTIME.resolve()) for part in ("requests", "results", "logs")),
        "运行目录或通信子目录越过本工作区边界。", "unsafe_runtime")


def native_status(gsudo):
    result = subprocess.run([gsudo, "status", "--json"], capture_output=True, timeout=5, creationflags=NO_WINDOW)
    require(result.returncode == 0, "无法读取 gsudo 状态。", "native_status_failed")
    try:
        status = json.loads(result.stdout.decode("utf-8-sig"))
    except (ValueError, UnicodeError) as error:
        raise CacheError("native_status_failed", "gsudo 状态不是有效的 UTF-8 JSON。") from error
    require(type(status) is dict and type(status.get("CacheAvailable")) is bool
        and type(status.get("CacheSessionsCount")) is int and status["CacheSessionsCount"] >= 0
        and type(status.get("CacheSessions")) is list and len(status["CacheSessions"]) == status["CacheSessionsCount"]
        and all(type(pipe) is str and pipe for pipe in status["CacheSessions"]),
        "gsudo 状态字段缺失、类型错误或缓存数量不一致。", "native_status_failed")
    return status


class NativeBackend:
    name = "native"

    def __init__(self, runtime):
        self.runtime = runtime
        self.owner = identity(os.getpid())
        require(token_info() == {"elevated": False, "integrity": 8192}, "控制器及命令客户端必须是普通 Medium 权限。", "integrity_rejected")
        self.gsudo = shutil.which("gsudo.exe")
        require(self.gsudo is not None, "未找到本机 gsudo。", "gsudo_missing")
        require(not self.info()["cache_available"], "控制器意外继承其他祖先进程的缓存，未将其接管。", "unexpected_inherited_cache")
        try:
            self.history_context = {**environment(SCRIPT.parent), "integrity": 8192,
                "windows_session_id": self.owner.session,
                "login_tag": hashlib.sha256((self.owner.user + self.owner.logon + str(self.owner.authentication)).encode()).hexdigest()[:12]}
        except Exception:
            self.history_context = {"integrity": 8192}
        try:
            version = win32api.GetFileVersionInfo(self.gsudo, "\\")
            high, low = version["FileVersionMS"], version["FileVersionLS"]
            self.history_context["gsudo_version"] = ".".join(str(x) for x in (high >> 16, high & 65535, low >> 16, low & 65535))
        except Exception:
            self.history_context["gsudo_version"] = "unavailable"

    def resources(self):
        process = win32api.GetCurrentProcess()
        memory, times = win32process.GetProcessMemoryInfo(process), win32process.GetProcessTimes(process)
        return {"working_set_bytes": memory["WorkingSetSize"],
                "cpu_seconds": (times["KernelTime"] + times["UserTime"]) / 10_000_000}

    def info(self):
        status = native_status(self.gsudo)
        return {"cache_available": status["CacheAvailable"], "native_sessions_observed": status["CacheSessionsCount"],
                "checked_at": utc_now().isoformat()}

    def on(self, seconds, log):
        seconds = math.ceil(seconds_ok(seconds))
        duration = f"{seconds//3600:02}:{seconds//60%60:02}:{seconds%60:02}"
        return subprocess.Popen([self.gsudo, "cache", "on", "-p", str(self.owner.pid), "-d", duration],
            stdout=log, stderr=log, creationflags=NO_WINDOW)

    def off(self):
        result = subprocess.run([self.gsudo, "cache", "off", "-p", str(self.owner.pid)],
            capture_output=True, timeout=5, creationflags=NO_WINDOW)
        require(result.returncode == 0, "关闭本控制器缓存失败。", "cache_close_failed")
        for _ in range(20):
            if not self.info()["cache_available"]:
                return
            time.sleep(.1)
        raise CacheError("cache_close_failed", "缓存仍可用，未确认关闭；未清除其他 PID 的缓存。")

    def spawn(self, command, log):
        return subprocess.Popen(command, cwd=ROOT, stdout=log, stderr=log, creationflags=NO_WINDOW)

    def spawn_job(self, command, log, token):
        env = os.environ.copy()
        env["QKM_CACHE_JOB"] = token
        return subprocess.Popen(command, cwd=ROOT, env=env, stdout=log, stderr=log, creationflags=NO_WINDOW)

    def verify_actor(self, value):
        actual = identity(value["pid"])
        require(asdict(actual) == value and actual.same_login(self.owner)
            and token_info(actual.pid) == {"elevated": False, "integrity": 8192},
            "动作调用方身份或登录会话不符。", "identity_rejected")

    def owned_alive(self, value):
        try:
            with closing(win32api.OpenProcess(0x1000 | win32con.SYNCHRONIZE, False, value["pid"])) as process:
                if win32event.WaitForSingleObject(process, 0) == win32event.WAIT_OBJECT_0:
                    return False
                return asdict(identity(value["pid"])) == value
        except pywintypes.error as error:
            return False if error.winerror in (87, 1168) else None
        except (OSError, BridgeError):
            return None

    def started_identity(self, response):
        actual = identity(response["ProcessId"])
        require(actual.created == int(response["StartTicks"]) - TICKS_TO_FILETIME
            and actual.same_login(self.owner) and token_info(actual.pid) == {"elevated": True, "integrity": 12288},
            "新测试进程身份或实际权限不符。", "identity_rejected")
        return asdict(actual)

    def start_action(self, payload, request_path, log, expected_files):
        shell = shutil.which("pwsh.exe")
        require(shell is not None, "未找到 PowerShell 7。", "shell_missing")
        pins = []
        try:
            for path, expected in expected_files.items():
                pins.append(win32file.CreateFile(str(path), win32con.GENERIC_READ, win32con.FILE_SHARE_READ,
                    None, win32con.OPEN_EXISTING, 0, None))
                require(file_hash(path) == expected, "提权前文件身份已改变。", "artifact_changed")
            require(self.info()["cache_available"], "动作派发前缓存失效，未启动提权。", "needs_uac")
            process = self.spawn([self.gsudo, "--direct", *action_command(ROOT, shell, payload, request_path)], log)
            process.qkm_pins = pins
            return process
        except BaseException:
            for handle in pins:
                handle.Close()
            raise

    def probe_command(self):
        return [self.gsudo, "--direct", sys.executable, "-B", "-X", "utf8", str(SCRIPT), "--identity-probe"]


class Controller:
    def __init__(self, root, runtime, backend, tick=time.monotonic, wall=utc_now):
        self.root, self.runtime, self.backend = root, runtime, backend
        self.tick, self.wall = tick, wall
        self.instance = uuid.uuid4().hex
        self.window = Window()
        self.scope = {}
        self.pending = self.job = None
        self.closing = False
        self.close_reason = None
        self.last_error = None
        self.cache_closed = False
        self.started = tick()
        self.seen = set()
        self.results = OrderedDict()
        self.native = {"cache_available": False, "checked_at": None}
        self.runtime.mkdir(parents=True, exist_ok=True)
        for folder in ("requests", "results", "logs"):
            (runtime / folder).mkdir(exist_ok=True)
        self.history = History(runtime, self.instance, backend.owner.pid, backend.owner.created, tick, wall,
            {**getattr(backend, "history_context", {}), "backend": backend.name})
        self.next_heartbeat = tick() + HEARTBEAT_SECONDS
        self.actions = Actions(self)

    def record(self, event, **fields):
        try:
            data = {"generation": self.window.generation, "deadline": self.window.until,
                "remaining_seconds": round(self.window.remaining(self.tick()), 3), "closing": self.closing,
                "close_reason": self.close_reason, "job_kind": self.job["kind"] if self.job else None,
                "pending_request": self.pending["id"] if self.pending else None,
                "cache_available": self.native.get("cache_available") if self.native.get("checked_at") else None,
                "native_checked_at": self.native.get("checked_at"), "scope": list(self.scope),
                "input_flags": [key for key in ("allow_default", "allow_focus_click", "allow_late_forced_up")
                    if self.scope.get("input_regression", {}).get(key) is True]}
            self.history.emit(event, **{**data, **fields})
        except Exception:
            self.history.sequence += 1
            self.history.failed("history_record_failed")

    def snapshot(self, refresh=False):
        if refresh:
            self.native = self.backend.info()
        return {"ok": True, "workspace": str(self.root), "instance": self.instance,
            "format_version": 2,
            "identity": asdict(self.backend.owner), "interpreter": sys.executable, "backend": self.backend.name,
            "status": "closing" if self.closing else "awaiting_cache" if self.pending else "active" if self.window.deadline else "idle",
            "deadline": self.window.until, "remaining_seconds": round(self.window.remaining(self.tick()), 2),
            "generation": self.window.generation, "default_seconds": DEFAULT_SECONDS,
            "scope": self.scope, "registered_targets": sorted(self.actions.definitions),
            "owned_targets": [{"pid": pid, "target_id": item["definition"]["id"]} for pid, item in self.actions.owned.items()],
            "unresolved_starts": sorted(self.actions.unknown_starts),
            "current_action": self.actions.pending["request"]["Action"] if self.actions.pending else None,
            "current_job": self.job["kind"] if self.job else None,
            "pending_request": self.pending["id"] if self.pending else None,
            "close_reason": self.close_reason, "last_error": self.last_error,
            "history": self.history.status(), **self.native}

    def publish(self):
        write_json(self.runtime / "state.json", self.snapshot())

    def answer(self, request_id, value, operation=None):
        value = {**value, "request_id": request_id, "instance": self.instance}
        write_json(self.runtime / "results" / (request_id + ".json"), value)
        self.results[request_id] = value
        while len(self.results) > 64:
            old, _ = self.results.popitem(last=False)
            (self.runtime / "results" / (old + ".json")).unlink(missing_ok=True)
        if operation is not None and (operation != "status" or not value.get("ok")):
            self.record("request_result", request_id=request_id, operation=operation,
                job_kind=value.get("kind", self.job["kind"] if self.job else None),
                **{key: value[key] for key in ("ok", "action", "error", "previous_deadline", "native_exit_code",
                    "requested_window_expired", "deadline_expired", "exit_code") if key in value})

    def set_window(self, request_id, request):
        require(self.pending is None and self.actions.pending is None and not self.closing, "缓存调整／动作正在执行或关闭，请先查询结果。", "busy")
        try:
            until = datetime.fromisoformat(request["deadline"])
            require(until.tzinfo is not None, "截止时间须带时区。")
            until = until.astimezone(CHINA)
        except (ValueError, KeyError, TypeError) as error:
            raise CacheError("invalid_arguments", "截止时间不合法。") from error
        remaining = seconds_ok((until - self.wall()).total_seconds())
        if "remaining_cap" in request:
            remaining = min(remaining, seconds_ok(request["remaining_cap"]))
        require(not self.actions.owned or remaining >= 20, "自有测试尚未清理；先 off 完成清理，再设置短窗口。", "insufficient_window")
        deadline_tick = self.tick() + remaining
        old_remaining, old_until = self.window.remaining(self.tick()), self.window.until
        self.native = self.backend.info()
        action = "restored" if request.get("restore") is True else "extended" if old_remaining and remaining > old_remaining else "shortened" if old_remaining else "created"
        if self.native["cache_available"] and old_remaining and remaining <= old_remaining:
            self.window.install(self.tick(), max(0, deadline_tick - self.tick()), until.isoformat())
            self.answer(request_id, {**self.snapshot(), "action": action, "previous_deadline": old_until}, "set")
            return
        log = (self.runtime / "logs" / (request_id + "-cache.log")).open("wb")
        try:
            process = self.backend.on(remaining, log)
        except BaseException:
            log.close()
            raise
        self.pending = {"id": request_id, "process": process, "log": log, "started": self.tick(),
            "deadline_tick": deadline_tick, "until": until.isoformat(), "old_until": old_until,
            "action": "reenabled" if old_remaining and not self.native["cache_available"] else action}
        self.record("cache_request", request_id=request_id, requested_deadline=until.isoformat(), previous_deadline=old_until)

    def handle(self, request):
        require(type(request) is dict, "请求须为对象。")
        request_id = request.get("id")
        require(type(request_id) is str and HEX_ID.fullmatch(request_id), "请求 ID 不合法。")
        if request_id in self.seen:
            if request_id not in self.results and not (self.pending and self.pending["id"] == request_id) and not (self.job and self.job["id"] == request_id) and not (self.actions.pending and self.actions.pending["id"] == request_id):
                self.answer(request_id, error_result(CacheError("result_evicted", "该请求已经处理，结果已淘汰；未重新执行。")), "duplicate")
            return
        require(len(self.seen) < 4096 or request.get("op") in ("status", "off"), "控制器请求记录已满，请关闭后重新创建。", "request_limit")
        if len(self.seen) < 4096:
            self.seen.add(request_id)
        if self.window.expire(self.tick(), self.window.generation):
            self.closing, self.close_reason = True, self.close_reason or "deadline_expired"
        operation = request.get("op")
        history_op = operation if type(operation) is str and operation in ("status", "off", "set", "authorize", "run", "register", "test_action") else "invalid"
        if history_op != "status":
            self.record("request", request_id=request_id, operation=history_op,
                requested_deadline=request.get("deadline"), restore=request.get("restore") is True)
        try:
            fields = {"status": set(), "off": set(), "set": {"deadline", "restore", "remaining_cap"},
                "authorize": {"scope"}, "run": {"kind", "arguments"}, "register": {"definition"}, "test_action": {"request", "job_token"}}
            require(type(operation) is str and operation in fields and set(request) <= {"id", "instance", "op", "actor"} | fields[operation], "请求字段不合法。")
            require("restore" not in request or type(request["restore"]) is bool, "恢复标志须为布尔值。")
            require(request.get("instance") == self.instance, "请求属于其他或已经结束的控制器。", "stale_controller")
            if operation == "status":
                self.answer(request_id, self.snapshot(refresh=True), "status")
            elif operation == "set":
                self.set_window(request_id, request)
            elif operation == "authorize":
                require(not self.closing, "控制器正在关闭。", "busy")
                self.scope = validate_scope(request.get("scope"))
                self.answer(request_id, {**self.snapshot(), "action": "scope_registered"}, "authorize")
            elif operation == "register":
                require(not self.job and not self.pending and not self.actions.pending and not self.closing, "测试或调整执行中不能变更定义。", "busy")
                self.backend.verify_actor(request.get("actor"))
                target = self.actions.register(request.get("definition"), self.scope.get("test_actions", {}).get("definitions", []))
                self.answer(request_id, {"ok": True, "action": "target_registered", "target_id": target}, "register")
            elif operation == "test_action":
                self.backend.verify_actor(request.get("actor"))
                raw = request.get("request")
                entry = raw.get("Action") if type(raw) is dict else None
                self.record("test_action_request", request_id=request_id,
                    entry_type=entry.lower() if entry in ("Start", "Close", "Gui") else "invalid")
                self.actions.begin(request_id, request.get("request"), request.get("job_token"))
            elif operation == "off":
                self.closing, self.close_reason = True, self.close_reason or "user_off"
                self.answer(request_id, {**self.snapshot(), "action": "closing"}, "off")
            elif operation == "run":
                require(self.pending is None and self.job is None and self.actions.pending is None and not self.actions.owned and not self.closing, "有缓存调整、固定动作或自有测试尚未清理。", "busy")
                command, budget = build_job(self.root, sys.executable, request.get("kind"), request.get("arguments"), self.scope)
                require(self.window.remaining(self.tick()) >= budget, "当前窗口不足以完成该测试及清理；请明确设置更长窗口。", "insufficient_window")
                self.native = self.backend.info()
                require(self.native["cache_available"], "缓存已不可用，请明确重新设置窗口并处理 UAC。", "needs_uac")
                if request["kind"] == "identity_probe":
                    command = self.backend.probe_command()
                log_path = self.runtime / "logs" / (request_id + "-job.log")
                log = log_path.open("wb")
                try:
                    self.actions.register_job(request["kind"], request["arguments"])
                    process = self.backend.spawn_job(command, log, self.actions.job_token) if hasattr(self.backend, "spawn_job") else self.backend.spawn(command, log)
                except BaseException:
                    log.close()
                    raise
                self.job = {"id": request_id, "process": process, "log": log, "kind": request["kind"],
                    "started": self.tick(), "budget": budget, "log_path": str(log_path)}
                self.record("job_start", request_id=request_id, budget_seconds=budget)
            else:
                raise CacheError("invalid_arguments", "不支持该控制操作。")
        except Exception as error:
            self.answer(request_id, error_result(error), history_op)
        self.publish()

    def advance(self):
        changed = False
        if self.window.deadline is not None and self.window.expire(self.tick(), self.window.generation):
            self.closing, self.close_reason = True, self.close_reason or "deadline_expired"
        if self.pending:
            pending = self.pending
            code = pending["process"].poll()
            if code is None and self.tick() - pending["started"] > 90:
                pending["process"].terminate()
                self.closing, self.close_reason = True, "cache_result_unknown"
            if code is not None:
                changed = True
                pending["log"].close()
                self.pending = None
                self.cache_closed = False
                self.native = self.backend.info()
                left = pending["deadline_tick"] - self.tick()
                if code == 0 and self.native["cache_available"] and left > 0 and not self.closing:
                    self.window.install(self.tick(), left, pending["until"])
                    self.answer(pending["id"], {**self.snapshot(), "action": pending["action"], "previous_deadline": pending["old_until"]}, "set")
                else:
                    self.answer(pending["id"], {**error_result(CacheError("needs_uac" if code == 999 else "cache_update_failed",
                        "UAC 已取消，未应用新窗口。" if code == 999 else "缓存调整未确认成功，未扩大原窗口。")),
                        "native_exit_code": code, "requested_window_expired": left <= 0}, "set")
                    if not self.window.deadline:
                        self.closing, self.close_reason = True, self.close_reason or "cache_start_failed"
        if self.actions.pending:
            self.actions.advance()
            changed = True
        self.actions.refresh_owned()
        if self.job:
            job = self.job
            code = job["process"].poll()
            if code is not None:
                changed = True
                job["log"].close()
                self.job = None
                self.answer(job["id"], {"ok": code == 0, "action": "job_finished", "kind": job["kind"],
                    "exit_code": code, "log": job["log_path"],
                    "deadline_expired": self.window.expire(self.tick(), self.window.generation), "close_reason": self.close_reason}, "run")
            elif self.tick() - job["started"] > job["budget"]:
                self.closing, self.close_reason = True, "job_needs_attention"
        if not self.window.deadline and not self.pending and self.tick() - self.started > 30:
            self.closing, self.close_reason = True, self.close_reason or "unused_controller"
        try:
            if self.window.remaining(self.tick()) >= 15:
                self.actions.cleanup()
        except Exception as error:
            self.last_error = error_result(error)
            self.record("owned_cleanup_failed", error=self.last_error["error"], cleanup_complete=False)
        if self.closing:
            # Complete bounded owned cleanup before cache off, without crossing the hard deadline.
            if self.window.remaining(self.tick()) > 0 and self.actions.pending:
                if changed:
                    self.publish()
                return True
            # A late cache-on may complete after expiry; keep the owner alive until it is reaped.
            first_close = not self.cache_closed
            if not self.cache_closed:
                self.backend.off()
                self.cache_closed = True
            self.native = self.backend.info()
            if first_close:
                self.record("cache_off_checked", ok=self.native.get("cache_available") is False)
            if self.pending is None and self.job is None and self.actions.pending is None:
                if self.actions.owned:
                    self.last_error = {"error": "owned_cleanup_pending"}
                self.window.clear()
                self.publish()
                return False
        if changed:
            self.publish()
        if self.tick() >= self.next_heartbeat:
            self.next_heartbeat = self.tick() + HEARTBEAT_SECONDS
            try:
                resources = self.backend.resources() if hasattr(self.backend, "resources") else {}
            except Exception:
                resources = {"resource_error": "resource_unavailable"}
            self.record("heartbeat", **resources)
        return True

    def serve(self):
        self.publish()
        self.record("controller_start")
        try:
            while True:
                paths = sorted((self.runtime / "requests").glob("*.json"), key=lambda p: p.stat().st_mtime_ns)
                require(len(paths) <= 128, "请求队列超限，关闭本控制器缓存。", "request_limit")
                for path in paths:
                    try:
                        require(HEX_ID.fullmatch(path.stem) is not None, "请求文件名不合法。")
                        request = read_json(path, 65536)
                        require(type(request) is dict and request.get("id") == path.stem, "请求记录不一致。")
                        self.handle(request)
                    except Exception as error:
                        if HEX_ID.fullmatch(path.stem):
                            self.answer(path.stem, error_result(error), "invalid")
                    finally:
                        path.unlink(missing_ok=True)
                if not self.advance():
                    break
                time.sleep(.25)
        except BaseException as error:
            self.last_error = error_result(error) if isinstance(error, Exception) else {"error": "interrupted"}
            raise
        finally:
            try:
                if self.pending:
                    self.pending["process"].terminate()
                    self.pending["process"].wait(timeout=5)
                    self.pending["log"].close()
                self.backend.off()
                self.native = self.backend.info()
                self.closing = True
            except Exception as error:
                self.record("cleanup_failed", error=error_result(error)["error"])
                raise
            self.record("controller_finish", cleanup_complete=self.native.get("cache_available") is False
                and self.pending is None and self.job is None and self.actions.pending is None and not self.actions.owned and not self.actions.unknown_starts and self.last_error is None,
                error=self.last_error.get("error") if self.last_error else None)
            write_json(self.runtime / "closed.json", {**self.snapshot(), "status": "closed"})


def verified_controller(runtime=RUNTIME):
    try:
        state = read_json(runtime / "state.json")
        require(type(state) is dict and state.get("workspace") == str(ROOT) and state.get("backend") == "native" and state.get("format_version") in (1, 2),
                "记录不属于本工作区或当前版本。")
        require(HEX_ID.fullmatch(state["instance"]) is not None, "记录代次不合法。")
        recorded, actual, own = Identity(**state["identity"]), identity(state["identity"]["pid"]), identity(os.getpid())
        require(actual == recorded and actual.same_login(own), "控制器 PID／身份已经变化。")
        require(actual.executable == own.executable and Path(state["interpreter"]).resolve() == Path(sys.executable).resolve(), "控制器解释器已经变化。")
        require(token_info(actual.pid) == {"elevated": False, "integrity": 8192}, "控制器权限不符合要求。")
        return state
    except (OSError, ValueError, KeyError, TypeError, CacheError, BridgeError, pywintypes.error):
        return None


def serve():
    runtime_safe()
    RUNTIME.mkdir(parents=True, exist_ok=True)
    require(RUNTIME.resolve().is_relative_to(ROOT / "out"), "运行目录必须位于本工作区 out。")
    try:
        lock = win32file.CreateFile(str(RUNTIME / "controller.lock"), win32con.GENERIC_READ | win32con.GENERIC_WRITE,
            0, None, win32con.OPEN_ALWAYS, 0, None)
    except pywintypes.error as error:
        raise CacheError("controller_exists", "另一个控制器已持有本工作区锁。") from error
    try:
        Controller(ROOT, RUNTIME, NativeBackend(RUNTIME)).serve()
    finally:
        lock.Close()


def ensure_controller():
    runtime_safe()
    own = identity(os.getpid())
    name = "Local\\QkmGsudoCache_" + hashlib.sha256((str(ROOT).casefold() + own.user + own.logon + str(own.session)).encode()).hexdigest()[:32]
    mutex = win32event.CreateMutex(None, False, name)
    acquired = False
    try:
        acquired = win32event.WaitForSingleObject(mutex, 10000) in (win32con.WAIT_OBJECT_0, 0x80)
        require(acquired, "其他客户端正在创建控制器，请稍后查询。", "busy")
        current = verified_controller()
        if current:
            return current, False
        RUNTIME.mkdir(parents=True, exist_ok=True)
        require(RUNTIME.resolve().is_relative_to(ROOT / "out"), "运行目录越界。")
        with (RUNTIME / "controller.log").open("ab") as log:
            process = subprocess.Popen([sys.executable, "-B", "-X", "utf8", str(SCRIPT), "--serve"],
                cwd=ROOT, stdout=log, stderr=log, creationflags=NO_WINDOW)
        for _ in range(100):
            current = verified_controller()
            if current:
                return current, True
            if process.poll() is not None:
                break
            time.sleep(.1)
        raise CacheError("controller_start_failed", "普通控制器未能启动；请检查 out/gsudo_cache/controller.log。")
    finally:
        if acquired:
            win32event.ReleaseMutex(mutex)
        mutex.Close()


def send(state, arguments, timeout=100, request_id=None):
    request_id = request_id or uuid.uuid4().hex
    require(HEX_ID.fullmatch(request_id) is not None, "请求 ID 不合法。")
    current = verified_controller()
    require(current is not None and current["instance"] == state["instance"], "控制器已退出或变化。", "stale_controller")
    path = RUNTIME / "requests" / (request_id + ".json")
    result_path = RUNTIME / "results" / (request_id + ".json")
    if not result_path.exists() and not path.exists():
        write_json(path, {"id": request_id, "instance": state["instance"], **arguments,
            **({"actor": asdict(identity(os.getpid()))} if state.get("format_version") == 2 else {})})
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if result_path.exists():
            result = read_json(result_path)
            require(result.get("instance") == state["instance"], "结果属于其他控制器。", "stale_controller")
            return result
        time.sleep(.1)
    return {"ok": False, "error": "result_unknown", "request_id": request_id,
        "message": "尚未取得请求结果；先查询状态或同一请求 ID，勿重新提交变更。"}


def public(value):
    value = dict(value)
    owner = value.pop("identity", None)
    if owner:
        value.update(pid=owner["pid"], created=owner["created"], image=owner["executable"], integrity="Medium")
    return value


def upgrade_controller():
    state = verified_controller()
    require(state is not None, "没有可升级的活动控制器。", "controller_absent")
    observed = time.monotonic()
    state = send(state, {"op": "status"}, timeout=10)
    require(state.get("ok") and state.get("status") == "active" and not any(state.get(key) for key in
        ("current_job", "current_action", "pending_request", "owned_targets")), "控制器须先完成作业和自有进程清理。", "busy")
    old_pid, deadline, scope = state["identity"]["pid"], state["deadline"], state["scope"]
    remaining = state["remaining_seconds"]
    require(remaining > 15, "旧窗口即将到期，未自动续期。", "insufficient_window")
    send(state, {"op": "off"}, timeout=10)
    for _ in range(100):
        current = verified_controller()
        if current is None:
            break
        time.sleep(.1)
    else:
        raise CacheError("controller_upgrade_failed", "旧控制器尚未退出，未启动替代控制器。")
    closed = read_json(RUNTIME / "closed.json")
    require(closed.get("instance") == state["instance"] and closed.get("cache_available") is False,
        "旧缓存未核实关闭。", "controller_upgrade_failed")
    current, _ = ensure_controller()
    send(current, {"op": "authorize", "scope": scope})
    cap = remaining - (time.monotonic() - observed)
    require(cap > 0, "升级期间原窗口已到期，未重新开启。", "insufficient_window")
    result = send(current, {"op": "set", "deadline": deadline, "remaining_cap": cap, "restore": False})
    return {**result, "previous_controller_pid": old_pid, "previous_deadline": deadline, "upgrade": True}


def client(words, scope_file=None, job_file=None, request_id=None, definition_file=None, action_file=None):
    require(token_info() == {"elevated": False, "integrity": 8192}, "客户端必须是普通 Medium 权限。", "integrity_rejected")
    runtime_safe()
    text = " ".join(words)
    if words and words[0] == "result":
        require(len(words) == 2 and HEX_ID.fullmatch(words[1]) is not None, "须提供完整请求 ID。")
        path = RUNTIME / "results" / (words[1] + ".json")
        require(path.is_file(), "结果未知或已经淘汰；先查询当前状态，勿重发变更。", "result_unknown")
        return {**read_json(path), "historical_result": True}
    if text == "upgrade":
        return upgrade_controller()
    if text in ("register", "test-action"):
        state = verified_controller()
        require(state is not None, "没有受管控制器；先设置缓存或明确选择不缓存 UAC 验收。", "controller_absent")
        require(state.get("format_version") == 2, "现有控制器需在空闲时升级；不会另建控制器。", "controller_upgrade_required")
        path = definition_file if text == "register" else action_file
        require(path is not None, "须提供测试定义或固定动作文件。")
        value = read_json(path)
        args = {"op": "register", "definition": value} if text == "register" else {
            "op": "test_action", "request": value, "job_token": os.environ.get("QKM_CACHE_JOB")}
        return send(state, args, timeout=80, request_id=request_id)
    if text in ("authorize", "run"):
        require((scope_file is not None if text == "authorize" else job_file is not None), "须提供对应的范围／固定测试 JSON。")
        state = verified_controller()
        require(state is not None, "尚无活动控制器，请先明确设置窗口。", "controller_absent")
        if text == "authorize":
            return send(state, {"op": "authorize", "scope": validate_scope(read_json(scope_file, 8192))}, request_id=request_id)
        job = read_json(job_file, 8192)
        require(type(job) is dict and set(job) == {"kind", "arguments"}, "固定测试请求不合法。")
        return send(state, {"op": "run", **job}, timeout=1300, request_id=request_id)
    require(scope_file is None and job_file is None, "缓存时间指令不会顺带修改测试授权。")
    command = parse_command(text)
    if command.action == "help":
        return {"ok": True, "default_seconds": 300, "examples": ["status", "30m", "5小时", "until 20261025 19:30", "直到明天0点50份", "restore", "off"]}
    state = verified_controller()
    if command.action in ("status", "off", "restore") and state is None:
        gsudo = shutil.which("gsudo.exe")
        sessions = native_status(gsudo)["CacheSessionsCount"] if gsudo else None
        return {"ok": True, "action": "default_restored" if command.action == "restore" else "already_closed" if command.action == "off" else "status",
            "controller": None, "default_seconds": 300, "native_sessions_observed": sessions,
            "message": "本工作区没有已核实的受管控制器；不会接管其他缓存。"}
    if command.action in ("status", "off"):
        result = send(state, {"op": command.action}, timeout=10, request_id=request_id)
        if command.action == "off" and result.get("ok"):
            for _ in range(50):
                try:
                    closed = read_json(RUNTIME / "closed.json")
                    if closed.get("instance") == state["instance"] and closed.get("cache_available") is False:
                        return {**closed, "action": "closed"}
                except (OSError, ValueError, CacheError):
                    pass
                time.sleep(.1)
        return result
    deadline = utc_now().astimezone(CHINA) + timedelta(seconds=300) if command.action == "restore" else command.deadline
    created = False
    if state is None:
        state, created = ensure_controller()
    result = send(state, {"op": "set", "deadline": deadline.isoformat(), "restore": command.action == "restore"}, request_id=request_id)
    result["controller_created"] = created
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", nargs="*")
    parser.add_argument("--scope-file", type=Path)
    parser.add_argument("--job-file", type=Path)
    parser.add_argument("--request-id")
    parser.add_argument("--definition-file", type=Path)
    parser.add_argument("--action-file", type=Path)
    parser.add_argument("--serve", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--identity-probe", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.identity_probe:
        print(json.dumps({**token_info(), "identity": asdict(identity(os.getpid()))}), flush=True)
        # Keep this fixed read-only fixture observable during cache expiry/off acceptance.
        time.sleep(5)
        return 0
    if args.serve:
        serve()
        return 0
    try:
        value = public(client(args.command, args.scope_file, args.job_file, args.request_id, args.definition_file, args.action_file))
    except Exception as error:
        value = error_result(error)
    print(json.dumps(value, ensure_ascii=False, indent=2))
    return 0 if value.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
