"""Ordinary-process deterministic cache tests; never invokes gsudo or system input."""

from dataclasses import asdict
from datetime import datetime, timedelta
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import uuid

import cache_control as app
from cache_core import CHINA, CacheError, Window, build_job, parse_command, validate_scope


class ParserTests(unittest.TestCase):
    now = datetime(2026, 10, 25, 12, 0, tzinfo=CHINA)

    def test_aliases_and_prefixes(self):
        for text, action in (("", "status"), ("status", "status"), ("当前状态", "status"),
                ("恢复默认", "restore"), ("restore", "restore"), ("off", "off"),
                ("关闭缓存", "off"), ("帮助", "help")):
            for prefix in ("", "$qkeymapper-gsudo-cache ", "/qkeymapper-gsudo-cache "):
                with self.subTest(text=text, prefix=prefix):
                    self.assertEqual(parse_command(prefix + text, self.now).action, action)

    def test_duration_units(self):
        for text, seconds in (("5m", 300), ("5h", 18000), ("5分钟", 300), ("5小时", 18000),
                ("5秒", 5), ("10s", 10), ("5", 300), ("５ Ｍ", 300), (" 24H ", 86400)):
            with self.subTest(text=text):
                self.assertEqual((parse_command(text, self.now).deadline - self.now).total_seconds(), seconds)

    def test_until_variants(self):
        for text, target in (("until 20261025 19:30", (25, 19, 30)),
                ("until 2026-10-25 19:30", (25, 19, 30)), ("直到今天19:30", (25, 19, 30)),
                ("直到明天0点50份", (26, 0, 50)), ("until 19:30", (25, 19, 30)),
                ("直到明天0点", (26, 0, 0)), ("直到今天１９：３０", (25, 19, 30))):
            with self.subTest(text=text):
                value = parse_command(text, self.now).deadline
                self.assertEqual((value.day, value.hour, value.minute), target)

    def test_invalid_requests_do_not_roll_or_clamp(self):
        for text in ("0", "-5m", "25h", "86401s", "1.5h", "1d", "一会儿", "until 20261024 19:30",
                "until 20260230 19:30", "直到今天11:59", "until 25:00", "直到明天12点1分", "until 19:60"):
            with self.subTest(text=text), self.assertRaises(CacheError):
                parse_command(text, self.now)

    def test_oversize_and_nonfinite_durations_are_rejected(self):
        for text in ("9" * 250 + "h", "9" * 1000 + "m", None):
            with self.subTest(text=str(text)[:20]), self.assertRaises(CacheError):
                parse_command(text, self.now)


class FakeProcess:
    def __init__(self):
        self.code = None
        self.terminated = False

    def poll(self):
        return self.code

    def terminate(self):
        self.terminated = True
        self.code = 1

    def wait(self, timeout=None):
        return self.code


class FakeBackend:
    name = "fake"
    owner = app.Identity(500, 1234, 1, "test-user", "test-logon", 7, "test-python.exe")

    def __init__(self):
        self.available = False
        self.opens = []
        self.closes = 0
        self.spawned = []

    def info(self):
        return {"cache_available": self.available, "checked_at": "fake", "native_sessions_observed": int(self.available)}

    def on(self, seconds, log):
        self.opens.append(seconds)
        return FakeProcess()

    def off(self):
        self.closes += 1
        self.available = False

    def spawn(self, command, log):
        self.spawned.append(command)
        return FakeProcess()

    def probe_command(self):
        return ["fixed-read-only-identity-probe"]


class ControllerTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.tick = 100.0
        self.now = datetime(2026, 10, 25, 12, 0, tzinfo=CHINA)
        self.backend = FakeBackend()
        self.controller = app.Controller(self.root, self.root / "out/runtime", self.backend,
            tick=lambda: self.tick, wall=lambda: self.now)
        self.addCleanup(self.close_logs)

    def close_logs(self):
        for name in ("pending", "job"):
            value = getattr(self.controller, name)
            if value:
                value["log"].close()

    def request(self, op, **values):
        request = {"id": uuid.uuid4().hex, "instance": self.controller.instance, "op": op, **values}
        self.controller.handle(request)
        return request

    def set_window(self, seconds, restore=False):
        return self.request("set", deadline=(self.now + timedelta(seconds=seconds)).isoformat(), restore=restore)

    def finish_on(self, code=0, available=True):
        self.controller.pending["process"].code = code
        self.backend.available = available
        return self.controller.advance()

    def result(self, request):
        return self.controller.results[request["id"]]

    def test_no_success_before_cache_is_observed(self):
        request = self.set_window(300)
        self.assertNotIn(request["id"], self.controller.results)
        self.assertIsNone(self.controller.window.deadline)
        self.finish_on()
        self.assertTrue(self.result(request)["ok"])
        self.assertEqual(self.result(request)["action"], "created")

    def test_extend_same_owner_survives_old_deadline(self):
        self.set_window(10); self.finish_on()
        old_generation = self.controller.window.generation
        self.tick += 2
        request = self.set_window(100); self.finish_on()
        self.tick += 10
        self.assertFalse(self.controller.window.expire(self.tick, old_generation))
        self.assertTrue(self.controller.advance())
        self.assertTrue(self.backend.available)
        self.assertEqual(self.result(request)["action"], "extended")
        self.assertEqual(self.controller.backend.owner.pid, 500)

    def test_shorten_and_restore_do_not_spawn_new_cache(self):
        self.set_window(18000); self.finish_on()
        request = self.set_window(300, restore=True)
        self.assertEqual(self.result(request)["action"], "restored")
        self.assertEqual(len(self.backend.opens), 1)
        request = self.set_window(10)
        self.assertEqual(self.result(request)["action"], "shortened")
        self.tick += 11
        self.assertFalse(self.controller.advance())
        self.assertFalse(self.backend.available)

    def test_wall_clock_change_does_not_extend_window(self):
        self.set_window(300); self.finish_on()
        self.now -= timedelta(hours=4)
        self.tick += 301
        self.assertFalse(self.controller.advance())

    def test_cancel_creation_closes_without_automatic_retry(self):
        request = self.set_window(300)
        self.assertFalse(self.finish_on(999, False))
        self.assertEqual(self.result(request)["error"], "needs_uac")
        self.assertEqual(len(self.backend.opens), 1)

    def test_failed_extension_keeps_old_deadline(self):
        self.set_window(300); self.finish_on()
        old = self.controller.window.deadline
        request = self.set_window(3600)
        self.finish_on(999, True)
        self.assertEqual(self.controller.window.deadline, old)
        self.assertFalse(self.result(request)["ok"])
        self.assertFalse(self.controller.closing)

    def test_zero_exit_without_cache_is_not_success(self):
        request = self.set_window(300)
        self.finish_on(0, False)
        self.assertFalse(self.result(request)["ok"])

    def test_reenable_lost_cache_is_explicit(self):
        self.set_window(300); self.finish_on()
        self.backend.available = False
        request = self.set_window(200); self.finish_on()
        self.assertEqual(self.result(request)["action"], "reenabled")

    def test_old_instance_and_unknown_fields_are_rejected(self):
        request = self.request("set", deadline=(self.now + timedelta(seconds=300)).isoformat(), scope={})
        self.assertFalse(self.result(request)["ok"])
        request = {"id": uuid.uuid4().hex, "instance": "0" * 32, "op": "set", "deadline": self.now.isoformat()}
        self.controller.handle(request)
        self.assertEqual(self.result(request)["error"], "stale_controller")
        self.assertFalse(self.backend.opens)

    def test_duplicate_and_evicted_request_never_reapply(self):
        request = self.set_window(300); self.finish_on()
        self.controller.handle(request)
        self.assertEqual(len(self.backend.opens), 1)
        for _ in range(65): self.request("status")
        self.controller.handle(request)
        self.assertEqual(self.result(request)["error"], "result_evicted")
        self.assertEqual(len(self.backend.opens), 1)

    def test_late_cache_completion_after_off_is_closed(self):
        request = self.set_window(300)
        self.request("off")
        self.assertTrue(self.controller.advance())
        self.assertFalse(self.finish_on(0, True))
        self.assertFalse(self.backend.available)
        self.assertFalse(self.result(request)["ok"])

    def test_busy_updates_do_not_override_pending(self):
        self.set_window(300)
        request = self.set_window(3600)
        self.assertEqual(self.result(request)["error"], "busy")
        self.assertEqual(self.backend.opens, [300])

    def test_fixed_job_requires_scope_and_cleanup_budget(self):
        self.set_window(10); self.finish_on()
        request = self.request("run", kind="identity_probe", arguments={})
        self.assertEqual(self.result(request)["error"], "scope_not_authorized")
        self.request("authorize", scope={"identity_probe": {"reason": "approved fixed test"}})
        request = self.request("run", kind="identity_probe", arguments={})
        self.assertEqual(self.result(request)["error"], "insufficient_window")
        self.assertFalse(self.backend.spawned)
        self.set_window(300); self.finish_on()
        request = self.request("run", kind="identity_probe", arguments={})
        self.assertEqual(self.backend.spawned, [["fixed-read-only-identity-probe"]])
        self.controller.job["process"].code = 0
        self.controller.advance()
        self.assertTrue(self.result(request)["ok"])

    def test_status_is_observation_not_keepalive(self):
        self.set_window(300); self.finish_on()
        deadline = self.controller.window.deadline
        self.request("status")
        self.assertEqual(self.controller.window.deadline, deadline)
        self.assertEqual(len(self.backend.opens), 1)

    def test_time_change_remains_responsive_during_a_fixed_job(self):
        self.set_window(300); self.finish_on()
        self.request("authorize", scope={"identity_probe": {"reason": "approved fixed test"}})
        job = self.request("run", kind="identity_probe", arguments={})
        request = self.set_window(3600); self.finish_on()
        self.assertEqual(self.result(request)["action"], "extended")
        self.assertEqual(self.controller.job["id"], job["id"])
        request = self.set_window(30)
        self.assertEqual(self.result(request)["action"], "shortened")
        self.assertEqual(self.controller.job["id"], job["id"])


class AdapterTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.base = self.root / "out/mcp_product"
        self.base.mkdir(parents=True)
        self.exe = self.base / "runtime/QKeyMapperMcpAcceptance_fixture.exe"
        self.exe.parent.mkdir(); self.exe.touch()
        self.args = {"executable": str(self.exe), "output": str(self.base / "new-output")}

    def test_product_is_fixed_and_never_adds_nested_cache(self):
        command, budget = build_job(self.root, "python", "product", self.args, {"product": {"reason": "approved"}})
        self.assertEqual(command[4], str(self.root / "mcp_bridge/tests/test_product_interop.py"))
        self.assertNotIn("--gsudo-cache", command)
        self.assertGreater(budget, 0)

    def test_no_generic_runner_or_extra_input(self):
        for kind, args in (("shell", self.args), ([], self.args),
                ("product", {**self.args, "command": "cmd.exe"}),
                ("product", {**self.args, "executable": "C:/Windows/System32/cmd.exe"}),
                ("identity_probe", {"command": "whoami"})):
            with self.subTest(kind=kind), self.assertRaises(CacheError):
                build_job(self.root, "python", kind, args, {"product": {}, "identity_probe": {}})

    def test_scope_is_not_granted_by_time_request(self):
        for scope in ({"anything": {"reason": "x"}}, {"product": {"reason": "x", "allow_default": True}},
                {"input_regression": {"reason": "x", "allow_focus_click": True}}, {"product": {"reason": ""}}):
            with self.subTest(scope=scope), self.assertRaises(CacheError): validate_scope(scope)

    def test_input_flags_need_both_explicit_request_and_scope(self):
        engine = self.base / "engine61"
        engine.mkdir()
        args = {"baseline": str(engine / "QKeyMapperMcpAcceptance_engine61_baseline.exe"),
                "current": str(engine / "QKeyMapperMcpAcceptance_engine61_current.exe"),
                "output": str(engine / "new"), "states": ["baseline", "off"]}
        Path(args["baseline"]).touch(); Path(args["current"]).touch()
        scope = {"input_regression": {"reason": "approved", "allow_default": True}}
        with self.assertRaises(CacheError): build_job(self.root, "python", "input_regression", args, scope)
        args["allow_default"] = True
        command, _ = build_job(self.root, "python", "input_regression", args, scope)
        self.assertIn("--allow-default", command)
        self.assertNotIn("--allow-focus-click", command)
        args["allow_focus_click"] = True
        with self.assertRaises(CacheError): build_job(self.root, "python", "input_regression", args, scope)


class ClientBoundaryTests(unittest.TestCase):
    def test_absent_status_restore_and_off_do_not_create(self):
        with tempfile.TemporaryDirectory() as directory:
            runtime = Path(directory) / "not-created"
            with patch.object(app, "RUNTIME", runtime), patch.object(app, "verified_controller", return_value=None), \
                    patch.object(app, "runtime_safe"), \
                    patch.object(app, "token_info", return_value={"elevated": False, "integrity": 8192}), \
                    patch.object(app, "native_status", return_value={"CacheSessionsCount": 0}):
                for command in ([], ["status"], ["restore"], ["off"]):
                    self.assertIsNone(app.client(command)["controller"])
                self.assertFalse(runtime.exists())

    def test_pid_reuse_and_high_controller_not_accepted(self):
        owner = FakeBackend.owner
        with tempfile.TemporaryDirectory() as directory:
            runtime = Path(directory)
            app.write_json(runtime / "state.json", {"workspace": str(app.ROOT), "backend": "native", "instance": "a" * 32,
                "identity": asdict(owner), "interpreter": app.sys.executable, "format_version": 1})
            changed = app.Identity(owner.pid, owner.created + 1, owner.session, owner.user, owner.logon, owner.authentication, owner.executable)
            with patch.object(app, "identity", side_effect=[changed, owner]):
                self.assertIsNone(app.verified_controller(runtime))
            with patch.object(app, "identity", return_value=owner), patch.object(app, "token_info", return_value={"elevated": True, "integrity": 12288}):
                self.assertIsNone(app.verified_controller(runtime))


if __name__ == "__main__":
    unittest.main()
