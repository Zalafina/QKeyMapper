"""Ordinary-process deterministic cache tests; never invokes gsudo or system input."""

from dataclasses import asdict
from datetime import datetime, timedelta
import json
import os
from pathlib import Path
import tempfile
import unittest
from types import SimpleNamespace
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


class ControllerFixture(unittest.TestCase):
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


class ControllerTests(ControllerFixture):
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
        self.assertEqual(self.controller.close_reason, "user_off")
        self.assertEqual(self.result(request)["native_exit_code"], 0)
        self.assertFalse(self.result(request)["requested_window_expired"])

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


class AdapterFixture(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.base = self.root / "out/mcp_product"
        self.base.mkdir(parents=True)
        self.exe = self.base / "runtime/QKeyMapperMcpAcceptance_fixture.exe"
        self.exe.parent.mkdir(); self.exe.touch()
        self.args = {"executable": str(self.exe), "output": str(self.base / "new-output")}


class AdapterTests(AdapterFixture):
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


class FailureTests(ControllerFixture):
    def start_probe(self):
        self.set_window(300); self.finish_on()
        self.request("authorize", scope={"identity_probe": {"reason": "approved fixed test"}})
        return self.request("run", kind="identity_probe", arguments={})

    def test_unreadable_native_state_never_installs_a_window(self):
        with patch.object(self.backend, "info", side_effect=CacheError("native_status_failed", "unreadable")):
            request = self.set_window(300)
        self.assertFalse(self.result(request)["ok"])
        self.assertIsNone(self.controller.window.deadline)
        self.assertFalse(self.backend.opens)

    def test_completion_state_failure_does_not_report_success(self):
        request = self.set_window(300)
        self.controller.pending["process"].code = 0
        with patch.object(self.backend, "info", side_effect=CacheError("native_status_failed", "unreadable")):
            with self.assertRaises(CacheError): self.controller.advance()
        self.assertNotIn(request["id"], self.controller.results)
        self.assertIsNone(self.controller.window.deadline)

    def test_cache_spawn_failure_preserves_previous_deadline(self):
        self.set_window(300); self.finish_on()
        previous = self.controller.window.deadline
        with patch.object(self.backend, "on", side_effect=OSError("spawn failed")):
            request = self.set_window(3600)
        self.assertFalse(self.result(request)["ok"])
        self.assertEqual(self.controller.window.deadline, previous)
        self.assertIsNone(self.controller.pending)

    def test_late_extension_after_old_deadline_is_closed(self):
        self.set_window(5); self.finish_on()
        request = self.set_window(300)
        self.tick += 6
        self.assertTrue(self.controller.advance())
        self.assertFalse(self.backend.available)
        self.assertFalse(self.finish_on(0, True))
        self.assertFalse(self.result(request)["ok"])
        self.assertFalse(self.backend.available)

    def test_expired_window_cannot_be_revived_before_next_poll(self):
        self.set_window(5); self.finish_on()
        self.tick += 6
        request = self.set_window(300)
        self.assertEqual(self.backend.opens, [5])
        self.assertFalse(self.result(request)["ok"])
        self.assertEqual(self.controller.close_reason, "deadline_expired")
        self.request("off")
        self.assertEqual(self.controller.close_reason, "deadline_expired")
        self.assertFalse(self.controller.advance())
        self.assertFalse(self.backend.available)

    def test_approval_after_requested_deadline_is_not_applied(self):
        request = self.set_window(5)
        self.tick += 6
        self.assertFalse(self.finish_on(0, True))
        self.assertFalse(self.result(request)["ok"])
        self.assertEqual(self.result(request)["native_exit_code"], 0)
        self.assertTrue(self.result(request)["requested_window_expired"])
        self.assertFalse(self.backend.available)

    def test_uac_unknown_result_terminates_only_pending_launcher(self):
        request = self.set_window(300)
        launcher = self.controller.pending["process"]
        self.tick += 91
        self.assertTrue(self.controller.advance())
        self.assertTrue(launcher.terminated)
        self.assertEqual(self.controller.close_reason, "cache_result_unknown")
        self.assertFalse(self.controller.advance())
        self.assertFalse(self.result(request)["ok"])
        self.assertFalse(self.backend.available)

    def test_close_failure_is_not_reported_as_closed(self):
        self.set_window(300); self.finish_on()
        self.request("off")
        with patch.object(self.backend, "off", side_effect=CacheError("cache_close_failed", "still active")):
            with self.assertRaises(CacheError): self.controller.advance()
        self.assertFalse(self.controller.cache_closed)
        self.assertTrue(self.backend.available)
        self.assertFalse((self.controller.runtime / "closed.json").exists())
        self.assertFalse(self.controller.advance())

    def test_off_keeps_active_job_for_its_own_cleanup(self):
        request = self.start_probe()
        process = self.controller.job["process"]
        self.request("off")
        self.assertTrue(self.controller.advance())
        self.assertFalse(self.backend.available)
        self.assertFalse(process.terminated)
        rejected = self.request("run", kind="identity_probe", arguments={})
        self.assertEqual(self.result(rejected)["error"], "busy")
        process.code = 0
        self.assertFalse(self.controller.advance())
        self.assertTrue(self.result(request)["ok"])
        self.assertFalse(self.result(request)["deadline_expired"])

    def test_shrink_to_expiry_waits_for_job_and_rejects_extension(self):
        request = self.start_probe()
        process = self.controller.job["process"]
        self.set_window(2)
        self.tick += 3
        self.assertTrue(self.controller.advance())
        self.assertFalse(self.backend.available)
        self.assertFalse(process.terminated)
        rejected = self.set_window(300)
        self.assertEqual(self.result(rejected)["error"], "busy")
        process.code = 1
        self.assertFalse(self.controller.advance())
        self.assertFalse(self.result(request)["ok"])
        self.assertTrue(self.result(request)["deadline_expired"])

    def test_job_timeout_keeps_process_and_reports_attention(self):
        self.start_probe()
        process = self.controller.job["process"]
        self.tick += 16
        self.assertTrue(self.controller.advance())
        self.assertEqual(self.controller.close_reason, "job_needs_attention")
        self.assertFalse(process.terminated)
        self.assertFalse(self.backend.available)
        process.code = 1
        self.assertFalse(self.controller.advance())

    def test_job_timeout_after_expiry_still_records_elapsed_deadline(self):
        request = self.start_probe()
        process = self.controller.job["process"]
        self.set_window(1)
        self.tick += 16
        self.assertTrue(self.controller.advance())
        self.assertEqual(self.controller.close_reason, "job_needs_attention")
        self.assertFalse(process.terminated)
        process.code = 1
        self.assertFalse(self.controller.advance())
        self.assertTrue(self.result(request)["deadline_expired"])

    def test_spawned_job_failure_has_result_and_does_not_close_window(self):
        request = self.start_probe()
        self.controller.job["process"].code = 7
        self.assertTrue(self.controller.advance())
        self.assertFalse(self.result(request)["ok"])
        self.assertEqual(self.result(request)["exit_code"], 7)
        self.assertIsNone(self.controller.job)
        self.assertFalse(self.controller.closing)

    def test_native_job_spawn_failure_leaves_no_active_job(self):
        self.set_window(300); self.finish_on()
        self.request("authorize", scope={"identity_probe": {"reason": "approved fixed test"}})
        with patch.object(self.backend, "spawn", side_effect=OSError("spawn failed")):
            request = self.request("run", kind="identity_probe", arguments={})
        self.assertFalse(self.result(request)["ok"])
        self.assertIsNone(self.controller.job)

    def test_request_limit_still_allows_status_and_off(self):
        self.controller.seen = {str(i) for i in range(4096)}
        with self.assertRaises(CacheError): self.request("authorize", scope={})
        request = self.request("status")
        self.assertTrue(self.result(request)["ok"])
        self.request("off")
        self.assertFalse(self.controller.advance())


class NativeStateTests(unittest.TestCase):
    def test_native_status_rejects_failed_or_malformed_observations(self):
        valid = {"CacheAvailable": False, "CacheSessionsCount": 0, "CacheSessions": []}
        bad = (None, [], {}, {**valid, "CacheAvailable": "false"}, {**valid, "CacheSessionsCount": True},
               {**valid, "CacheSessionsCount": -1}, {**valid, "CacheSessions": "pipe"},
               {**valid, "CacheSessionsCount": 1}, {**valid, "CacheSessions": [17], "CacheSessionsCount": 1})
        with patch.object(app.subprocess, "run", return_value=SimpleNamespace(returncode=1, stdout=b"{}")):
            with self.assertRaises(CacheError): app.native_status("gsudo.exe")
        for value in bad:
            result = SimpleNamespace(returncode=0, stdout=json.dumps(value).encode())
            with self.subTest(value=value), patch.object(app.subprocess, "run", return_value=result):
                with self.assertRaises(CacheError): app.native_status("gsudo.exe")
        with patch.object(app.subprocess, "run", return_value=SimpleNamespace(returncode=0, stdout=json.dumps(valid).encode())):
            self.assertEqual(app.native_status("gsudo.exe"), valid)

    def test_native_off_only_targets_owner_and_rejects_live_cache(self):
        backend = app.NativeBackend.__new__(app.NativeBackend)
        backend.gsudo, backend.owner = "gsudo.exe", FakeBackend.owner
        with patch.object(app.subprocess, "run", return_value=SimpleNamespace(returncode=0)) as command, \
                patch.object(backend, "info", return_value={"cache_available": True}), patch.object(app.time, "sleep"):
            with self.assertRaises(CacheError): backend.off()
            self.assertEqual(command.call_args.args[0], ["gsudo.exe", "cache", "off", "-p", "500"])


class SupplementAdapterTests(AdapterFixture):
    def test_source_positive_and_strict_boundaries(self):
        fixture = self.root / "out/tests/mcp_control/QkmControlSource.exe"
        fixture.parent.mkdir(parents=True); fixture.touch()
        qt = self.root / "qt/bin"; qt.mkdir(parents=True)
        args = {**self.args, "test_exe": str(fixture), "qt_bin": str(qt)}
        command, budget = build_job(self.root, "python", "source", args, {"source": {"reason": "approved"}})
        self.assertEqual(command[4], str(self.root / "mcp_bridge/tests/test_source_interop.py"))
        self.assertIn(str(fixture), command)
        self.assertNotIn("--gsudo-cache", command)
        self.assertEqual(budget, 630)
        for change in ({"test_exe": str(self.exe)}, {"qt_bin": str(qt / "missing")},
                       {"gsudo_cache": True}, {"output": str(self.exe.parent)}):
            with self.subTest(change=change), self.assertRaises(CacheError):
                build_job(self.root, "python", "source", {**args, **change}, {"source": {}})


class SupplementClientTests(unittest.TestCase):
    def test_nonobject_state_record_is_not_adopted(self):
        with tempfile.TemporaryDirectory() as directory:
            runtime = Path(directory)
            for value in (None, [], "old-state", 7):
                with self.subTest(value=value):
                    app.write_json(runtime / "state.json", value)
                    self.assertIsNone(app.verified_controller(runtime))

    def test_timeout_and_historical_lookup_never_replay_mutation(self):
        with tempfile.TemporaryDirectory() as directory:
            runtime = Path(directory)
            (runtime / "requests").mkdir(); (runtime / "results").mkdir()
            state = {"instance": "c" * 32}
            request_id = "a" * 32
            with patch.object(app, "RUNTIME", runtime), patch.object(app, "verified_controller", return_value=state), \
                    patch.object(app, "runtime_safe"), patch.object(app, "token_info", return_value={"elevated": False, "integrity": 8192}):
                result = app.send(state, {"op": "set", "deadline": "later"}, timeout=0, request_id=request_id)
                self.assertEqual(result["error"], "result_unknown")
                files = list((runtime / "requests").glob("*.json"))
                self.assertEqual(len(files), 1)
                before = files[0].read_bytes()
                with self.assertRaises(CacheError): app.client(["result", request_id])
                self.assertEqual(files[0].read_bytes(), before)
                app.write_json(runtime / "results" / (request_id + ".json"), {"ok": True, "instance": state["instance"], "request_id": request_id})
                self.assertTrue(app.client(["result", request_id])["historical_result"])
                self.assertEqual(files[0].read_bytes(), before)
                app.write_json(runtime / "results" / (request_id + ".json"), {"instance": "d" * 32})
                with self.assertRaises(CacheError): app.send(state, {"op": "off"}, request_id=request_id)

    def test_time_commands_cannot_smuggle_scope_or_jobs(self):
        with patch.object(app, "runtime_safe"), patch.object(app, "token_info", return_value={"elevated": False, "integrity": 8192}), \
                patch.object(app, "ensure_controller") as create:
            with self.assertRaises(CacheError): app.client(["30m"], scope_file=Path("scope.json"))
            with self.assertRaises(CacheError): app.client(["30m"], job_file=Path("job.json"))
            create.assert_not_called()

    def test_high_client_is_rejected_before_any_native_operation(self):
        with patch.object(app, "token_info", return_value={"elevated": True, "integrity": 12288}), \
                patch.object(app, "native_status") as native, patch.object(app, "ensure_controller") as create:
            with self.assertRaises(CacheError): app.client(["status"])
            native.assert_not_called(); create.assert_not_called()

    def test_changed_login_executable_interpreter_or_format_not_adopted(self):
        owner = FakeBackend.owner
        with tempfile.TemporaryDirectory() as directory:
            runtime = Path(directory)
            state = {"workspace": str(app.ROOT), "backend": "native", "instance": "a" * 32,
                "identity": asdict(owner), "interpreter": app.sys.executable, "format_version": 1}
            for change in ({"workspace": "another-workspace"}, {"backend": "other"}, {"format_version": 3},
                           {"interpreter": "another-python.exe"}, {"instance": "invalid"}):
                with self.subTest(change=change), patch.object(app, "identity", return_value=owner), \
                        patch.object(app, "token_info", return_value={"elevated": False, "integrity": 8192}):
                    app.write_json(runtime / "state.json", {**state, **change})
                    self.assertIsNone(app.verified_controller(runtime))
            app.write_json(runtime / "state.json", state)
            other = app.Identity(owner.pid + 1, owner.created, owner.session, "other-user", "other-login", 99, "other-python.exe")
            with patch.object(app, "identity", side_effect=[owner, other]):
                self.assertIsNone(app.verified_controller(runtime))

    def test_runtime_junction_cannot_escape_workspace(self):
        import _winapi
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runtime, outside = root / "out/gsudo_cache", root / "outside"
            runtime.mkdir(parents=True); outside.mkdir()
            link = runtime / "requests"
            _winapi.CreateJunction(str(outside), str(link))
            try:
                with patch.object(app, "ROOT", root), patch.object(app, "RUNTIME", runtime):
                    with self.assertRaises(CacheError): app.runtime_safe()
            finally:
                os.rmdir(link)

    def test_status_native_failure_cannot_claim_no_cache(self):
        with patch.object(app, "runtime_safe"), patch.object(app, "verified_controller", return_value=None), \
                patch.object(app, "token_info", return_value={"elevated": False, "integrity": 8192}), \
                patch.object(app, "native_status", side_effect=CacheError("native_status_failed", "unreadable")):
            with self.assertRaises(CacheError): app.client(["status"])


if __name__ == "__main__":
    unittest.main()
