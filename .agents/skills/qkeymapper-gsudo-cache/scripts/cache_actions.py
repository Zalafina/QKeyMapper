"""Registered native test targets and owned actions; never accepts a command runner."""

import hashlib
import json
from pathlib import Path
import re
import uuid

from cache_core import CacheError, require

PRIVATE_ACTIONS = {"OpenAdvanced", "OpenMcp", "Enable", "ApproveRead", "ApproveConfigure", "Revoke", "Disable",
    "CloseMcp", "ReadMapping", "ReadTab1", "ReadTab2", "ReadCommon", "MoveSecondUp", "RestoreFirstRows",
    "SelectSaved", "SelectSecond", "SelectInvalid", "SelectBlank", "ReloadCurrent", "AddBasicMapping"}
DEFAULT_ACTIONS = (PRIVATE_ACTIONS - {"AddBasicMapping"}) | {"ClearBasicMapping", "ShowForCapture",
    "OpenFloatingSetup", "CloseFloatingSetup", "SyncGroupOne", "SyncGroupZero", "StartMapping", "StopMapping"}
HELPERS = ("scripts/test_process.ps1", "QKeyMapper/tests/mcp_control/product_ui_actions.ps1")
TICKS_TO_FILETIME = 504911232000000000
REGISTRATION_LIMIT = 128
TARGET_ID = re.compile(r"[a-zA-Z][a-zA-Z0-9_-]{0,63}")


def helper_hashes(root):
    return {name: file_hash(root / name) for name in HELPERS}


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, ensure_ascii=False,
        separators=(",", ":")).encode("utf-8")).hexdigest()


def file_hash(path):
    with Path(path).open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def under_out(root, value, suffix=None):
    require(type(value) is str and "\0" not in value, "测试路径不合法。")
    path = Path(value).resolve()
    require(path.is_relative_to(root / "out") and path != root / "out"
        and (suffix is None or path.suffix.lower() == suffix), "测试路径必须在本工作区 out 中。", "unsafe_target")
    return path


def definition(root, value):
    require(type(value) is dict and set(value) == {"id", "executable", "sha256", "cases", "desktop",
        "output_root", "maximum_seconds", "gui_actions", "client_executable", "helpers"}, "测试定义字段不合法。")
    require(type(value["id"]) is str and TARGET_ID.fullmatch(value["id"]), "测试定义 ID 不合法。")
    exe = under_out(root, value["executable"], ".exe")
    require(exe.is_file(), "目标文件不存在。", "unsafe_target")
    with exe.open("rb") as source:
        native = source.read(2) == b"MZ"
    require(native, "目标须是本机构建的原生测试 EXE。", "unsafe_target")
    require(type(value["sha256"]) is str and re.fullmatch(r"[0-9a-f]{64}", value["sha256"])
        and file_hash(exe) == value["sha256"], "目标内容与已审核定义不一致。", "artifact_changed")
    require(type(value["cases"]) is list and 1 <= len(value["cases"]) <= 64, "须声明有界测试用例。")
    for case in value["cases"]:
        require(type(case) is dict and set(case) == {"arguments", "environment"}
            and type(case["arguments"]) is str and len(case["arguments"]) <= 8192 and "\0" not in case["arguments"], "用例参数不合法。")
        env = case["environment"]
        require(type(env) is dict and len(env) <= 16 and all(type(k) is str and re.fullmatch(r"[a-zA-Z_][a-zA-Z0-9_]{0,63}", k)
            and type(v) is str and len(v) <= 16384 and "\0" not in v for k, v in env.items()), "环境参数须精确登记。")
    require(value["desktop"] in ("private", "Default"), "须明确测试桌面。")
    require(type(value["maximum_seconds"]) is int and 15 <= value["maximum_seconds"] <= 1800, "测试预算须为15至1800秒并包含清理。")
    under_out(root, value["output_root"])
    actions = value["gui_actions"]
    require(type(actions) is list and len(actions) <= 64 and all(type(x) is str and
        re.fullmatch(r"[a-zA-Z][a-zA-Z0-9_]{0,63}", x) for x in actions)
        and len(actions) == len(set(actions)), "固定 GUI 动作名称不合法。")
    require(type(value["helpers"]) is dict and value["helpers"] == helper_hashes(root),
        "固定助手内容须随测试定义一并审核。", "helper_changed")
    client = value["client_executable"]
    require(client is None or type(client) is str and Path(client).is_absolute() and Path(client).is_file(), "客户端身份不合法。")
    # Approval covers the exact document, including cases, desktop and artifact identity.
    return json.loads(json.dumps(value))


class Actions:
    def __init__(self, controller):
        self.c = controller
        self.definitions = {}
        self.release_marks = {}
        self.owned = {}
        self.pending = None
        self.job_token = None
        self.job_definitions = set()
        self.unknown_starts = {}

    def check_capacity(self, target_ids):
        if len(self.definitions) + len(set(target_ids) - self.definitions.keys()) > REGISTRATION_LIMIT:
            choice = "请从 status 选择并显式注销可释放登记。" if self.releasable_targets() else "当前没有可释放登记。"
            raise CacheError("request_limit", f"测试目标登记已达 {REGISTRATION_LIMIT} 项。{choice}")

    def register(self, value, approved):
        value = definition(self.c.root, value)
        target_id, fingerprint = value["id"], digest(value)
        require(fingerprint in approved, "测试定义未经本次已有授权登记；元数据本身不是授权。", "scope_not_authorized")
        self.check_capacity((target_id,))
        if target_id in self.definitions:
            self.refresh_owned()
            require(not self.in_use(target_id), "该定义仍关联进程、动作、作业或未定启动。", "busy")
        self.definitions[target_id] = value
        self.release_marks.pop(target_id, None)
        self.c.record("target_registered", target_id=target_id, definition_hash=fingerprint)
        return target_id

    def registration_reference(self, target_id):
        return {"instance": self.c.instance, "target_id": target_id, "definition_hash": digest(self.definitions[target_id])}

    def in_use(self, target_id):
        pending = bool(self.pending) and (self.pending.get("definition") or {}).get("id") in (None, target_id)
        job = bool(self.c.job) and (target_id in self.job_definitions
            or not self.job_definitions and self.c.job.get("kind") != "identity_probe")
        return (pending or job or any(item["definition"]["id"] == target_id for item in self.owned.values())
            or any(unknown in (None, target_id) for unknown in self.unknown_starts.values()))

    def approved_definitions(self):
        return self.c.scope.get("test_actions", {}).get("definitions", [])

    def authorization_changed(self):
        approved = set(self.approved_definitions())
        self.release_marks = {key: value for key, value in self.release_marks.items() if value[0] not in approved}

    def releasable_targets(self):
        if self.c.closing:
            return []
        approved = set(self.approved_definitions())
        return sorted(target_id for target_id, (fingerprint, _) in self.release_marks.items()
            if fingerprint not in approved and not self.in_use(target_id))

    def check_reference(self, reference, unregister=False):
        fields = {"instance", "target_id", "definition_hash"}
        if unregister:
            fields.add("release_token")
        require(type(reference) is dict and set(reference) == fields
            and all(type(value) is str for value in reference.values()), "登记引用字段不合法。")
        require(reference["instance"] == self.c.instance, "登记引用属于其他或已结束的控制器。", "stale_controller")
        target_id, fingerprint = reference["target_id"], reference["definition_hash"]
        target = self.definitions.get(target_id)
        require(target is not None and digest(target) == fingerprint, "登记已变化或已注销，请核对最新登记。", "stale_registration")
        return target_id, fingerprint

    def require_releasable(self, target_id, fingerprint):
        require(not self.c.closing, "控制器正在关闭。", "busy")
        require(fingerprint not in self.approved_definitions(), "该定义仍有授权，先撤出本定义的授权摘要。", "target_authorized")
        self.refresh_owned()
        require(not self.in_use(target_id), "该定义仍关联进程、动作、作业或未定启动。", "target_in_use")

    def mark_releasable(self, reference, request_id):
        target_id, fingerprint = self.check_reference(reference)
        self.require_releasable(target_id, fingerprint)
        if target_id not in self.release_marks:
            self.release_marks[target_id] = (fingerprint, uuid.uuid4().hex)
            self.c.record("target_release_marked", request_id=request_id, target_id=target_id, definition_hash=fingerprint)
        return {**reference, "release_token": self.release_marks[target_id][1]}

    def unregister(self, reference, request_id):
        target_id, fingerprint = self.check_reference(reference, unregister=True)
        require(self.release_marks.get(target_id) == (fingerprint, reference["release_token"]),
            "释放标记已失效，请核对当前登记状态。", "stale_release_reference")
        self.require_releasable(target_id, fingerprint)
        del self.release_marks[target_id]
        del self.definitions[target_id]
        self.c.record("target_unregistered", request_id=request_id, target_id=target_id, definition_hash=fingerprint)
        return target_id

    def register_job(self, kind, args):
        self.job_token = uuid.uuid4().hex
        self.job_definitions.clear()
        if kind not in ("product", "source", "input_regression"):
            return
        names = ("baseline", "current") if kind == "input_regression" else ("executable",)
        self.check_capacity("run_" + name for name in names)
        for name in names:
            exe = Path(args[name]).resolve()
            value = {"id": "run_" + name, "executable": str(exe), "sha256": file_hash(exe),
                "cases": [{"arguments": "", "environment": {}}],
                "desktop": "Default" if kind == "input_regression" else "private",
                "output_root": str(self.c.root / "out/mcp_product"), "maximum_seconds": 1200 if kind == "input_regression" else 600,
                "gui_actions": sorted(DEFAULT_ACTIONS if kind == "input_regression" else PRIVATE_ACTIONS),
                "client_executable": self.c.backend.owner.executable, "helpers": helper_hashes(self.c.root)}
            self.register(value, [digest(value)])
            self.job_definitions.add(value["id"])

    def allowed(self, target, job_token):
        c = self.c
        return digest(target) in self.approved_definitions() or bool(c.job
            and c.job["kind"] in c.scope and job_token == self.job_token and target["id"] in self.job_definitions)

    def refresh_owned(self):
        for pid, item in list(self.owned.items()):
            if self.c.backend.owned_alive(item["identity"]) is False:
                del self.owned[pid]
                self.c.record("owned_exit", target_pid=pid, cleanup_complete=True)

    def begin(self, request_id, request, job_token=None, cleanup=False):
        c = self.c
        require(self.pending is None and c.pending is None, "另一个固定动作或缓存调整尚未完成。", "busy")
        require(not c.job or job_token == self.job_token, "整套测试占用中，仅其内部请求可进入。", "busy")
        require(type(request) is dict and type(request.get("Action")) is str, "固定动作请求不合法。")
        action = request["Action"]
        require(action in ("Start", "Close", "Gui"), "不接受任意提权动作。")
        require(not c.closing or action == "Close", "控制器正在关闭，不派发新测试。", "busy")
        self.refresh_owned()
        if action == "Start":
            require(set(request) == {"Action", "Executable", "Arguments", "Environment", "DesktopName"}, "启动字段不合法。")
            matches = [d for d in self.definitions.values() if self.allowed(d, job_token)
                and Path(request["Executable"]).resolve() == Path(d["executable"]).resolve()
                and {"arguments": request["Arguments"], "environment": request["Environment"]} in d["cases"]]
            require(len(matches) == 1, "目标／参数未唯一登记；不会另建缓存或直接申请 UAC。", "target_not_registered")
            target = matches[0]
            require(len(self.owned) < 32, "自有测试进程数量已满。", "request_limit")
            require(c.window.remaining(c.tick()) >= target["maximum_seconds"], "窗口不足以完成测试和清理。", "insufficient_window")
        else:
            required = {"Action", "Executable", "ProcessId", "StartTicks", "DesktopName"}
            require(set(request) <= required | ({"Name", "ResultFile", "ClientExecutable"} if action == "Gui" else set())
                and required <= set(request), "自有进程动作字段不合法。")
            item = self.owned.get(request["ProcessId"])
            require(item is not None and Path(request["Executable"]).resolve() == Path(item["identity"]["executable"]).resolve()
                and int(request["StartTicks"]) - TICKS_TO_FILETIME == item["identity"]["created"]
                and request["DesktopName"] == item["desktop"], "进程不属于本控制器或 PID 身份已变化。", "ownership_rejected")
            target = item["definition"]
        if action != "Close":
            require(self.allowed(target, job_token), "测试定义授权已撤销或不属于当前整套作业。", "scope_not_authorized")
        desktop = request["DesktopName"]
        require(type(desktop) is str and (desktop == "Default" if target["desktop"] == "Default" else
            bool(re.fullmatch(r"Qkm[a-zA-Z0-9_-]{1,120}", desktop)) and desktop != "Default"), "桌面超出已登记范围。", "scope_not_authorized")
        require(file_hash(target["executable"]) == target["sha256"], "目标文件已变化，须重新审核登记。", "artifact_changed")
        require(target["helpers"] == helper_hashes(c.root),
            "固定助手已变化；先清理旧测试，再审核登记新定义。", "helper_changed")
        if action == "Gui":
            require(request.get("Name") in target["gui_actions"], "GUI 动作未登记已有授权。", "scope_not_authorized")
            result = under_out(c.root, request.get("ResultFile"), ".json")
            require(result.is_relative_to(Path(target["output_root"]).resolve()) and not result.exists(), "GUI 结果路径不合法。")
            require(request.get("ClientExecutable") in (None, target["client_executable"]), "客户端身份不匹配。", "identity_rejected")
        require(c.window.remaining(c.tick()) >= (15 if action == "Close" else 60), "已到清理保留时间或截止。", "insufficient_window")
        c.native = c.backend.info()
        require(c.native["cache_available"], "所属控制器缓存不可用；未改用其他入口或自动重试。", "needs_uac")
        payload = dict(request)
        if action == "Close":
            payload["AnyOwnedWindow"] = True
        response = c.runtime / "results" / (request_id + "-native.json")
        payload["Response"] = str(response)
        request_path = c.runtime / "logs" / (request_id + "-action.json")
        from cache_control import write_json
        write_json(request_path, payload)
        log_path = c.runtime / "logs" / (request_id + "-action.log")
        log = log_path.open("wb")
        dispatch_started = c.tick()
        try:
            process = c.backend.start_action(payload, request_path, log,
                {**{c.root / k: v for k, v in target["helpers"].items()}, Path(target["executable"]): target["sha256"], request_path: file_hash(request_path)})
        except BaseException as error:
            log.close()
            c.record("test_action_diagnostic", request_id=request_id, entry_type=action.lower(), target_id=target["id"],
                **self.diagnostics({"process": None, "started": dispatch_started}, None, "dispatch", error=error))
            raise
        self.pending = {"id": request_id, "process": process, "log": log, "request": request,
            "definition": target, "response": response, "started": c.tick(), "cleanup": cleanup, "timed_out": False}
        c.record("test_action_start", request_id=request_id, entry_type=action.lower(), target_id=target["id"], definition_hash=digest(target), cleanup=cleanup)

    def diagnostics(self, pending, code, stage=None, response_state="not_observed", response=None, error=None):
        result = {"native_exit_code": code, "native_pid": None, "reported_target_pid": None,
            "action_seconds": None, "timed_out": pending.get("timed_out", False), "controller_terminated": False,
            "failure_stage": stage, "response_state": response_state, "os_error_code": None, "os_errno": None}
        try:
            result["action_seconds"] = round(max(0, self.c.tick() - pending["started"]), 3)
            pid = getattr(pending["process"], "pid", None)
            result["native_pid"] = pid if type(pid) is int and pid > 0 else None
            pid = response.get("ProcessId") if type(response) is dict else None
            result["reported_target_pid"] = pid if type(pid) is int and pid > 0 else None
            for key, attribute in (("os_error_code", "winerror"), ("os_errno", "errno")):
                value = getattr(error, attribute, None)
                result[key] = value if type(value) is int else None
            if stage == "response_read" and error is not None:
                result["response_state"] = "unavailable"
                if isinstance(error, (ValueError, UnicodeError)):
                    result["response_state"] = "invalid"
                elif getattr(error, "code", None) == "invalid_record":
                    path = Path(pending["request"]["ResultFile"]) if pending["request"]["Action"] == "Gui" else pending["response"]
                    result["response_state"] = "invalid" if path.is_file() else "missing"
        except Exception:
            pass  # Observation failure must not change the action result or cleanup.
        return result

    def advance(self):
        self.refresh_owned()
        p = self.pending
        if not p:
            return
        code = p["process"].poll()
        if code is None:
            if self.c.tick() - p["started"] > 70 and not p["timed_out"]:
                p["timed_out"] = True
                if p["request"]["Action"] == "Start":
                    self.unknown_starts[p["id"]] = p["definition"]["id"]
                self.c.closing, self.c.close_reason = True, "action_result_unknown"
                self.c.answer(p["id"], {"ok": False, "error": "result_unknown", "message": "固定动作超时；未重试或启动替代进程。",
                    **self.diagnostics(p, code, "await_exit")}, "test_action")
            return
        p["log"].close()
        for handle in getattr(p["process"], "qkm_pins", []):
            handle.Close()
        self.pending = None
        from cache_control import read_json
        stage, response_state, observed, failure = "response_read", "not_observed", None, None
        try:
            action = p["request"]["Action"]
            if action == "Gui":
                value = read_json(Path(p["request"]["ResultFile"]))
                observed = value
                response_state = "valid" if type(value) is dict else "invalid"
                stage = "action_result"
                require(code == 0 and value.get("success") is True, "固定 GUI 动作失败，查看本机结果。", "test_action_failed")
                response = {"Success": True, "ExitCode": 0}
            else:
                response = read_json(p["response"])
                observed = response
                response_state = "valid" if type(response) is dict else "invalid"
                stage = "action_result"
                require(code == 0 and response.get("Success") is True, "固定启动／关闭未成功，查看本机动作日志。", "test_action_failed")
            if action == "Start":
                stage = "identity_check"
                actual = self.c.backend.started_identity(response)
                require(Path(actual["executable"]).resolve() == Path(p["definition"]["executable"]).resolve(), "启动结果身份不符。", "identity_rejected")
                self.unknown_starts.pop(p["id"], None)
                self.owned[actual["pid"]] = {"identity": actual, "definition": p["definition"],
                    "desktop": p["request"]["DesktopName"], "cleanup_at": min(self.c.tick() + p["definition"]["maximum_seconds"] - 20,
                        self.c.window.deadline - 20)}
            elif action == "Close":
                self.owned.pop(p["request"]["ProcessId"], None)
            result = {"ok": True, "action": "test_action_finished", "response": response, "exit_code": code}
        except Exception as error:
            failure = error
            from cache_control import error_result
            result = error_result(error)
            self.c.last_error = result
            if p["request"]["Action"] == "Start":
                self.unknown_starts[p["id"]] = None if stage == "identity_check" else p["definition"]["id"]
                self.c.closing, self.c.close_reason = True, "start_result_unknown"
        result.update(self.diagnostics(p, code, stage if failure else None, response_state, observed, failure))
        self.c.answer(p["id"], result, "test_action")

    def cleanup(self):
        self.refresh_owned()
        if self.pending or self.c.pending or not self.owned:
            return
        for pid, item in list(self.owned.items()):
            if not item.get("cleanup_attempted") and (self.c.closing or self.c.tick() >= min(item["cleanup_at"], self.c.window.deadline - 20)):
                item["cleanup_attempted"] = True
                request = {"Action": "Close", "Executable": item["identity"]["executable"], "ProcessId": pid,
                    "StartTicks": str(item["identity"]["created"] + TICKS_TO_FILETIME), "DesktopName": item["desktop"]}
                self.begin(uuid.uuid4().hex, request, self.job_token, cleanup=True)
                return


def action_command(root, shell, payload, request_path):
    if payload["Action"] != "Gui":
        return [shell, "-NoProfile", "-File", str(root / "scripts/test_process.ps1"), "-LaunchRequest", str(request_path)]
    command = [shell, "-NoProfile", "-STA", "-File", str(root / "QKeyMapper/tests/mcp_control/product_ui_actions.ps1"),
        "-Action", payload["Name"], "-TargetProcessId", str(payload["ProcessId"]), "-ExecutablePath", payload["Executable"],
        "-StartTicks", str(payload["StartTicks"]), "-DesktopName", payload["DesktopName"], "-ResultFile", payload["ResultFile"]]
    if payload.get("ClientExecutable"):
        command += ["-ClientExecutable", payload["ClientExecutable"]]
    if payload["DesktopName"] == "Default":
        command += ["-AllowDefaultDesktop"]
    return command
