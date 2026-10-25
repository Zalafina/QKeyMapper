"""Health journal checks use ordinary files, controlled clocks and a fake cache backend."""

from datetime import timedelta
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import cache_history as journal
import cache_control as app
from test_cache_control import ControllerFixture


class HistoryTests(ControllerFixture):
    def rows(self):
        result = []
        for path in sorted((self.controller.runtime / "history").glob("events-*.jsonl")):
            result += [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()]
        return result

    def test_requests_scope_and_results_are_useful_but_not_private(self):
        self.request("authorize", scope={"identity_probe": {"reason": "PRIVATE_APPROVAL_TEXT"}})
        request = self.set_window(300); self.finish_on()
        job = self.request("run", kind="identity_probe", arguments={})
        self.controller.job["process"].code = 0
        self.controller.advance()
        self.request("set", deadline="C:/PRIVATE_CONFIG", token="PRIVATE_TOKEN")
        rows = self.rows()
        text = json.dumps(rows)
        for private in ("PRIVATE_APPROVAL_TEXT", "PRIVATE_CONFIG", "PRIVATE_TOKEN", "test-user", "test-logon", "test-python.exe"):
            self.assertNotIn(private, text)
        result = next(row for row in rows if row.get("request_id") == request["id"] and row["event"] == "request_result")
        self.assertTrue(result["ok"])
        self.assertEqual(result["action"], "created")
        self.assertIn("identity_probe", result["scope"])
        self.assertEqual(result["generation"], 1)
        self.assertTrue(result["deadline"].endswith("+08:00"))
        finished = next(row for row in rows if row.get("request_id") == job["id"] and row["event"] == "request_result")
        self.assertEqual(finished["operation"], "run")
        self.assertEqual(finished["job_kind"], "identity_probe")
        self.assertEqual(finished["exit_code"], 0)

    def test_heartbeat_is_throttled_and_does_not_probe_or_keep_cache_alive(self):
        self.set_window(300); self.finish_on()
        self.controller.native["checked_at"] = self.now.isoformat()
        with patch.object(self.backend, "info", side_effect=AssertionError("heartbeat must not probe native state")), \
                patch.object(self.backend, "on", side_effect=AssertionError("heartbeat must not extend cache")):
            for _ in range(59):
                self.tick += 1
                self.assertTrue(self.controller.advance())
            self.assertFalse([row for row in self.rows() if row["event"] == "heartbeat"])
            self.tick += 1
            self.assertTrue(self.controller.advance())
            self.tick += 2
            self.assertTrue(self.controller.advance())
        heartbeats = [row for row in self.rows() if row["event"] == "heartbeat"]
        self.assertEqual(len(heartbeats), 1)
        self.assertEqual(heartbeats[0]["remaining_seconds"], 240)
        self.assertEqual(heartbeats[0]["native_checked_at"], self.now.isoformat())

    def test_plain_status_does_not_flood_the_history(self):
        self.set_window(300); self.finish_on()
        count = len(self.rows())
        for _ in range(20):
            self.request("status")
        self.assertEqual(len(self.rows()), count)

    def test_resource_failure_is_observation_failure_not_control_failure(self):
        self.set_window(300); self.finish_on()
        with patch.object(self.backend, "resources", create=True, side_effect=OSError("private diagnostic")):
            self.tick += 60
            self.assertTrue(self.controller.advance())
        self.assertEqual(self.rows()[-1]["resource_error"], "resource_unavailable")
        self.assertFalse(self.controller.closing)

    def test_write_failure_does_not_change_deadline_or_prevent_stop(self):
        self.set_window(300); self.finish_on()
        deadline = self.controller.window.deadline
        with patch.object(self.controller.history, "_append", side_effect=OSError("disk full PRIVATE")):
            self.request("status")
            self.assertEqual(self.controller.window.deadline, deadline)
            self.request("off")
            self.assertFalse(self.controller.advance())
        self.assertFalse(self.backend.available)
        self.assertFalse(self.controller.snapshot()["history"]["available"])
        self.assertGreater(self.controller.snapshot()["history"]["missed_events"], 0)

    def test_logger_exception_cannot_escape_into_cache_operations(self):
        with patch.object(self.controller.history, "emit", side_effect=RuntimeError("unexpected observer failure")):
            self.set_window(5); self.finish_on()
            self.tick += 6
            self.assertFalse(self.controller.advance())
        self.assertFalse(self.backend.available)
        self.assertEqual(self.controller.history.status()["last_error"], "history_record_failed")

    def test_recovery_records_gap_and_sequence(self):
        with patch.object(self.controller.history, "_append", side_effect=OSError("failed")):
            self.controller.record("heartbeat")
        self.controller.record("heartbeat")
        row = self.rows()[-1]
        self.assertEqual(row["sequence"], 2)
        self.assertEqual(row["missed_events"], 1)
        self.assertTrue(self.controller.history.status()["available"])

    def test_retention_is_bounded_and_only_removes_managed_files(self):
        directory = self.controller.runtime / "history"
        directory.mkdir()
        old = directory / "events-20260901-0000.jsonl"
        old.write_text("old\n", encoding="utf-8")
        other = directory / "keep.txt"
        other.write_text("do not remove", encoding="utf-8")
        with patch.object(journal, "FILE_BYTES", 2048), patch.object(journal, "TOTAL_BYTES", 6144):
            for _ in range(60):
                self.controller.record("heartbeat")
        files = list(directory.glob("events-*.jsonl"))
        self.assertFalse(old.exists())
        self.assertTrue(other.exists())
        self.assertLessEqual(sum(path.stat().st_size for path in files), 6144)
        self.assertTrue(all(path.stat().st_size <= 2048 for path in files))
        self.assertTrue(self.rows())

    def test_partial_line_stays_visible_and_following_event_is_readable(self):
        directory = self.controller.runtime / "history"
        directory.mkdir()
        path = directory / "events-20261025-0000.jsonl"
        path.write_bytes(b'{"partial":')
        self.controller.record("heartbeat")
        lines = path.read_text(encoding="utf-8").splitlines()
        with self.assertRaises(ValueError): json.loads(lines[0])
        self.assertEqual(json.loads(lines[1])["event"], "heartbeat")

    def test_history_junction_is_rejected_without_blocking_cleanup(self):
        import _winapi
        with tempfile.TemporaryDirectory() as directory:
            outside = Path(directory)
            link = self.controller.runtime / "history"
            _winapi.CreateJunction(str(outside), str(link))
            try:
                self.set_window(5); self.finish_on()
                self.tick += 6
                self.assertFalse(self.controller.advance())
                self.assertFalse(list(outside.iterdir()))
                self.assertFalse(self.backend.available)
                self.assertGreater(self.controller.history.missed, 0)
            finally:
                os.rmdir(link)

    def test_hard_link_log_cannot_modify_an_unrelated_file(self):
        directory = self.controller.runtime / "history"
        directory.mkdir()
        other = self.root / "unrelated.txt"
        other.write_text("keep unchanged", encoding="utf-8")
        os.link(other, directory / "events-20261025-0000.jsonl")
        self.controller.record("heartbeat")
        self.assertEqual(other.read_text(encoding="utf-8"), "keep unchanged")
        self.assertFalse(self.controller.history.status()["available"])

    def test_scope_and_resource_values_are_filtered(self):
        self.controller.history.emit("heartbeat", password="PRIVATE", arguments={"secret": "PRIVATE"},
            operation="C:/PRIVATE", error="C:/PRIVATE", deadline="PRIVATE", cpu_seconds=float("nan"),
            input_flags=["allow_default", "PRIVATE"], scope=["source", "PRIVATE"])
        row = self.rows()[-1]
        self.assertNotIn("PRIVATE", json.dumps(row))
        self.assertNotIn("cpu_seconds", row)
        self.assertEqual(row["input_flags"], ["allow_default"])

    def test_serve_records_only_verified_finish_and_write_loss_is_reported(self):
        self.controller.closing = True
        self.controller.close_reason = "user_off"
        self.controller.serve()
        rows = self.rows()
        self.assertEqual(rows[0]["event"], "controller_start")
        self.assertEqual(rows[-1]["event"], "controller_finish")
        self.assertTrue(rows[-1]["cleanup_complete"])
        self.assertFalse(rows[-1]["cache_available"])
        self.assertTrue(json.loads((self.controller.runtime / "closed.json").read_text())["history"]["available"])

    def test_cleanup_failure_has_no_successful_finish_record(self):
        self.controller.closing = True
        with patch.object(self.backend, "off", side_effect=OSError("PRIVATE_FAILURE")):
            with self.assertRaises(OSError): self.controller.serve()
        self.assertFalse(any(row["event"] == "controller_finish" for row in self.rows()))
        self.assertEqual(self.rows()[-1]["event"], "cleanup_failed")

    def test_final_record_failure_still_closes_and_reports_incomplete_history(self):
        self.controller.closing = True
        with patch.object(self.controller.history, "_append", side_effect=OSError("no space")):
            self.controller.serve()
        closed = app.read_json(self.controller.runtime / "closed.json")
        self.assertFalse(closed["cache_available"])
        self.assertFalse(closed["history"]["available"])
        self.assertGreater(closed["history"]["missed_events"], 0)

    def test_environment_hash_changes_with_the_implementation_without_paths(self):
        directory = self.root / "code"
        directory.mkdir()
        for name in ("cache_control.py", "cache_core.py", "cache_history.py"):
            (directory / name).write_text("first", encoding="utf-8")
        with patch.object(journal.platform, "node", return_value="PRIVATE_HOST"):
            first = journal.environment(directory)
            (directory / "cache_history.py").write_text("changed", encoding="utf-8")
            second = journal.environment(directory)
        self.assertNotEqual(first["code_version"], second["code_version"])
        self.assertEqual(first["machine_id"], second["machine_id"])
        self.assertNotIn("PRIVATE_HOST", json.dumps(first))
        self.assertNotIn(str(directory), json.dumps(first))


if __name__ == "__main__":
    unittest.main()
