"""Bounded cache windows and fixed QKeyMapper test arguments; no native side effects."""

import math
import re
import unicodedata
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from pathlib import Path

CHINA = timezone(timedelta(hours=8), "Asia/Shanghai")
DEFAULT_SECONDS = 300
MAX_SECONDS = 86400
PROFILES = {"product", "source", "input_regression", "identity_probe", "test_actions"}
ENGINE_CASES = {"hold_stop", "burst_release", "burst_stop", "lock_toggle_stop", "sequence_complete",
                "sequence_cancel", "stop_message", "tab_return", "profile_return", "quick_restart", "exit_wait"}


class CacheError(Exception):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code


def require(condition, message, code="invalid_arguments"):
    if not condition:
        raise CacheError(code, message)


def seconds_ok(value):
    require(type(value) in (int, float) and 0 < value <= MAX_SECONDS and math.isfinite(value),
            "窗口须大于0且不超过24小时，不会自动截断。")
    return value


@dataclass(frozen=True)
class Command:
    action: str
    deadline: datetime | None = None


def parse_command(text, now=None):
    require(type(text) is str and len(text) <= 256, "指令须为不超过256字符的文本。")
    now = (now or datetime.now(CHINA)).astimezone(CHINA)
    text = unicodedata.normalize("NFKC", text).strip()
    text = re.sub(r"^[/$]?qkeymapper-gsudo-cache(?:\s+|$)", "", text, flags=re.I).strip()
    aliases = {"": "status", "status": "status", "当前状态": "status", "状态": "status",
        "restore": "restore", "恢复默认": "restore", "恢复默认5分钟": "restore",
        "off": "off", "stop": "off", "关闭": "off", "关闭缓存": "off",
        "help": "help", "帮助": "help"}
    if text.casefold() in aliases:
        return Command(aliases[text.casefold()])
    duration = re.fullmatch(r"([0-9]+)\s*(s|m|h|秒|分钟|小时)?", text, re.I)
    if duration:
        factor = {None: 60, "s": 1, "m": 60, "h": 3600, "秒": 1, "分钟": 60, "小时": 3600}
        unit = duration[2].casefold() if duration[2] else None
        seconds = seconds_ok(int(duration[1]) * factor[unit])
        return Command("set", now + timedelta(seconds=seconds))
    until = re.fullmatch(r"(?:until\s+|直到\s*)(.*)", text, re.I)
    if until:
        parts = re.fullmatch(r"(今天|明天|[0-9]{8}|[0-9]{4}-[0-9]{2}-[0-9]{2})?\s*"
            r"([0-9]{1,2})(?::([0-9]{1,2})|点(?:([0-9]{1,2})(?:分|份)?)?)", until[1])
        require(parts is not None, "请给出明确日期和时间，例如 until 20261025 19:30 或直到明天0点50分。")
        date = parts[1]
        try:
            day = now.date() + timedelta(days=1 if date == "明天" else 0) if date in (None, "今天", "明天") else \
                datetime.strptime(date, "%Y-%m-%d" if "-" in date else "%Y%m%d").date()
            deadline = datetime(day.year, day.month, day.day, int(parts[2]), int(parts[3] or parts[4] or 0), tzinfo=CHINA)
        except ValueError as error:
            raise CacheError("invalid_arguments", "日期或时分不合法。") from error
        seconds_ok((deadline - now).total_seconds())
        return Command("set", deadline)
    raise CacheError("invalid_arguments", "无法确定指令；请使用 status、30m、until 明确时间、restore 或 off。")


class Window:
    def __init__(self):
        self.deadline = None
        self.until = None
        self.generation = 0

    def remaining(self, tick):
        return max(0.0, self.deadline - tick) if self.deadline is not None else 0.0

    def install(self, tick, seconds, until):
        seconds_ok(seconds)
        previous = self.remaining(tick)
        self.deadline, self.until = tick + seconds, until
        self.generation += 1
        return "extended" if previous and seconds > previous else "shortened" if previous else "created"

    def expire(self, tick, generation):
        return generation == self.generation and self.deadline is not None and tick >= self.deadline

    def clear(self):
        self.deadline = self.until = None
        self.generation += 1


def validate_scope(value):
    require(type(value) is dict and set(value) <= PROFILES, "范围只允许三个固定测试入口及只读身份探针。")
    result = {}
    for kind, entry in value.items():
        require(type(entry) is dict and set(entry) <= {"reason", "allow_default", "allow_focus_click", "allow_late_forced_up", "definitions"},
                "范围字段不合法。")
        require(type(entry.get("reason")) is str and 1 <= len(entry["reason"]) <= 1000, "须记录已有授权的来源和具体范围。")
        if kind == "test_actions":
            definitions = entry.get("definitions")
            require(type(definitions) is list and len(definitions) <= 32 and all(type(x) is str
                and re.fullmatch(r"[0-9a-f]{64}", x) for x in definitions), "须明确批准测试定义的完整摘要。")
        else:
            require("definitions" not in entry, "该入口不接受测试定义。")
        for key in ("allow_default", "allow_focus_click", "allow_late_forced_up"):
            require(key not in entry or type(entry[key]) is bool, "输入授权标志须为布尔值。")
            require(kind == "input_regression" or not entry.get(key), "普通产品及源查询范围不得包含真实输入。")
        require(not (entry.get("allow_focus_click") or entry.get("allow_late_forced_up")) or entry.get("allow_default"),
                "额外输入范围依赖明确的 Default 授权。")
        result[kind] = dict(entry)
    return result


def build_job(root, python, kind, args, scope):
    require(type(kind) is str and kind in PROFILES - {"test_actions"} and kind in scope, "该固定测试入口尚未登记已有授权。", "scope_not_authorized")
    require(type(args) is dict, "测试参数须为对象。")
    root = Path(root).resolve()

    def path(name, base, exists=True):
        require(type(args.get(name)) is str, "缺少路径参数 " + name)
        p = Path(args[name]).resolve()
        require(p != base and p.is_relative_to(base), "路径必须位于本工作区允许的 out 子目录。")
        require(p.is_file() if exists else not p.exists(), "输入文件不存在或输出目录已存在。")
        return str(p)

    if kind == "identity_probe":
        require(not args, "身份探针不接受命令或路径参数。")
        return [], 15
    if kind in ("product", "source"):
        allowed = {"executable", "output", "load_fixture"} if kind == "product" else {"executable", "output", "test_exe", "qt_bin"}
        require(set(args) <= allowed, "存在未支持的测试参数。")
        base = root / "out/mcp_product"
        exe, output = path("executable", base), path("output", base, False)
        require(re.fullmatch(r"QKeyMapperMcpAcceptance_[a-zA-Z0-9_]+\.exe", Path(exe).name) is not None, "须选择自有隔离正式产品。")
        command = [python, "-B", "-X", "utf8", str(root / "mcp_bridge/tests" / ("test_product_interop.py" if kind == "product" else "test_source_interop.py")),
            "--executable", exe, "--output", output]
        if kind == "product":
            fixture = args.get("load_fixture", "blank")
            require(fixture in ("blank", "saved", "invalid", "missing", "warm"), "配置夹具不合法。")
            command += ["--load-fixture", fixture]
        else:
            test_exe = path("test_exe", root / "out/tests/mcp_control")
            require(re.fullmatch(r"QKeyMapperMcpControlTest\.exe|QkmControl[a-zA-Z0-9_]*\.exe", Path(test_exe).name) is not None,
                    "须选择固定控制维护测试目标。")
            require(type(args.get("qt_bin")) is str and Path(args["qt_bin"]).is_dir(), "须提供本机 Qt bin 目录。")
            command += ["--test-exe", test_exe, "--qt-bin", str(Path(args["qt_bin"]).resolve())]
        return command, 630
    allowed = {"baseline", "current", "output", "states", "pairs", "rounds", "cases", "allow_default", "allow_focus_click", "allow_late_forced_up", "thread_cpu"}
    require(set(args) <= allowed, "存在未支持的输入回归参数。")
    base = root / "out/mcp_product/engine61"
    baseline, current, output = path("baseline", base), path("current", base), path("output", base, False)
    for name, exe in (("baseline", baseline), ("current", current)):
        require(Path(exe).name == "QKeyMapperMcpAcceptance_engine61_" + name + ".exe", "输入回归产品名称不合法。")
    command = [python, "-B", "-X", "utf8", str(root / "QKeyMapper/tests/input_regression/run_acceptance.py"),
        "--baseline", baseline, "--current", current, "--output", output]
    for key in ("allow_default", "allow_focus_click", "allow_late_forced_up", "thread_cpu"):
        require(key not in args or type(args[key]) is bool, "输入选项须为布尔值。")
        if args.get(key):
            require(key == "thread_cpu" or scope[kind].get(key) is True, "该系统输入选项没有已登记授权。", "scope_not_authorized")
            command.append("--" + key.replace("_", "-"))
    require(args.get("allow_default") is True, "输入回归要求显式的 allow_default，不会自动追加。")
    for key, default, low, high in (("pairs", 250, 10, 500), ("rounds", 3, 1, 3)):
        value = args.get(key, default)
        require(type(value) is int and low <= value <= high, "输入采样数超出范围。")
        command += ["--" + key, str(value)]
    states = args.get("states", ["baseline", "off", "idle", "query"])
    require(type(states) is list and states and all(type(x) is str and x in ("baseline", "off", "idle", "query") for x in states)
            and len(set(states)) == len(states), "状态列表不合法。")
    command += ["--states", *states]
    if "cases" in args:
        cases = args["cases"]
        require(type(cases) is list and cases and all(type(x) is str and x in ENGINE_CASES for x in cases)
                and len(set(cases)) == len(cases) and set(states) <= {"baseline", "off"}, "固定输入用例不合法。")
        command += ["--cases", *cases]
    require(not args.get("allow_late_forced_up") or args.get("cases") == ["hold_stop"], "晚到释放仅限独立 hold_stop 用例。")
    return command, 1230
