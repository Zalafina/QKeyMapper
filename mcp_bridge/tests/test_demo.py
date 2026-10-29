"""Focused read-only demo binding checks; simulated metadata is not GUI acceptance."""

import copy
import json
import os
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock, patch

import win32file
import win32security

from qkeymapper_mcp.bridge import Bridge
from qkeymapper_mcp.demo import DEMO_POLICY, DEMO_PURPOSE, DEMO_SCOPES, read_binding, readiness, validate_binding
from qkeymapper_mcp.protocol import BridgeError
from qkeymapper_mcp.windows import identity


def fixture():
    record = dict(version=2, startup_purpose=DEMO_PURPOSE, executable=r"C:\QkmDemo\QKeyMapper.exe", pid="100", created="200", session="1",
                  instance="a" * 32, desktop="QkmDemoUnit", configuration="DemoSeed")
    target = {key: record[key] for key in ("instance", "pid", "created", "executable", "desktop")}
    windows = dict(target=target, private_background_available=True, items=[
        dict(role="main", qt_visible=True, native_ready=True, native_visible=True, minimized=False),
        dict(role="item_setup", qt_visible=False)])
    configuration = dict(group="DemoSeed", kind="saved_profile", has_unsaved_changes=False)
    load = dict(group="DemoSeed", outcome="accepted", generation="1", scope="last_mapping_load")
    state = dict(instance=record["instance"], execution_policy=dict(DEMO_POLICY), configuration=configuration, mapping_load_context=load,
                 run=dict(gui_state="idle", worker_state_sample="stopped"), current_tab=dict(index=0))
    sources = dict(offset=0, total=2, next_offset=None, source_snapshot=dict(instance=record["instance"],
        common_feature_enabled=False, read_basis="pristine_idle", configuration_selection=configuration,
        mapping_load_context=load), items=[
            dict(kind="normal", attached_to_ui=True, ui_index=0, row_count=0, include_common=False),
            dict(kind="common", attached_to_ui=False, ui_index=None, row_count=0)])
    server = SimpleNamespace(pid=100, created=200, session=1, executable=record["executable"])
    return record, windows, state, sources, SimpleNamespace(instance=record["instance"], server=server)


class DemoBindingTests(unittest.TestCase):
    def test_binding_strict_types_and_identity(self):
        record, _, _, _, target = fixture()
        self.assertEqual(validate_binding(record, target, Path(record["executable"])), record)
        for change in (dict(version=True), dict(version=1), dict(startup_purpose="standard"), dict(pid=100), dict(pid="0100"), dict(pid="101"),
                       dict(created="201"), dict(session="2"), dict(instance="b" * 32),
                       dict(executable=r"C:\Other\QKeyMapper.exe"), dict(desktop="Default"),
                       dict(desktop="winsta0\\QkmDemoUnit"), dict(configuration=""), dict(unexpected=True)):
            with self.subTest(change=change), self.assertRaises(BridgeError) as failure:
                validate_binding({**record, **change}, target, Path(record["executable"]))
            self.assertEqual(failure.exception.code, "demo_record_mismatch")

    def test_old_record_and_missing_purpose_are_not_migrated(self):
        record, _, _, _, target = fixture()
        for version in (1, 2):
            changed = dict(record, version=version)
            del changed["startup_purpose"]
            with self.subTest(version=version), self.assertRaises(BridgeError):
                validate_binding(changed, target, Path(record["executable"]))
        self.assertEqual(record["version"], 2)

    def test_authenticated_policy_is_strict_and_run_is_never_allowed(self):
        record, windows, state, sources, _ = fixture()
        policies = [None, {}, {**DEMO_POLICY, "extra": False}, {**DEMO_POLICY, "startup_purpose": "standard"}]
        for key in DEMO_POLICY:
            if key == "startup_purpose":
                continue
            policies.extend({**DEMO_POLICY, key: value} for value in (True, 0, None, "false"))
            missing = dict(DEMO_POLICY)
            del missing[key]
            policies.append(missing)
        for policy in policies:
            with self.subTest(policy=policy):
                changed = dict(state, execution_policy=policy)
                self.assertEqual(readiness(record, windows, changed, sources, DEMO_SCOPES), "demo_configuration_only_required")
        self.assertEqual(readiness(record, windows, state, sources, DEMO_SCOPES | 4), "demo_configuration_only_required")

    def test_private_file_bounds_corruption_and_hardlink(self):
        root = Path(__file__).resolve().parents[2] / "out"
        with tempfile.TemporaryDirectory(prefix="demo-record-unit-", dir=root) as temporary:
            self.assertTrue(Path(temporary).resolve().is_relative_to(root.resolve()))
            own = identity(os.getpid())
            private = Path(temporary) / "private"
            security = win32security.SECURITY_ATTRIBUTES()
            security.SECURITY_DESCRIPTOR = win32security.ConvertStringSecurityDescriptorToSecurityDescriptor(
                "O:" + own.user + "D:P(A;OICI;FA;;;" + own.user + ")", 1)
            win32file.CreateDirectory(str(private), security)
            path = private / "demo.json"
            record = fixture()[0]
            raw = json.dumps(record).encode("utf-8")
            path.write_bytes(raw)
            self.assertEqual(read_binding(path, own), record)
            for invalid in (b"x" * 8193, b'{"version":1,"version":2}', b"{", b"[]", b""):
                path.write_bytes(invalid)
                with self.subTest(size=len(invalid)), self.assertRaises(BridgeError) as failure:
                    read_binding(path, own)
                self.assertEqual(failure.exception.code, "demo_record_rejected")
            path.write_bytes(raw)
            link = private / "linked.json"
            os.link(path, link)
            with self.assertRaises(BridgeError):
                read_binding(path, own)
            link.unlink()
            public = Path(temporary) / "public.json"
            public.write_bytes(raw)
            with self.assertRaises(BridgeError):
                read_binding(public, own)

    def test_readiness_fails_closed(self):
        record, windows, state, sources, _ = fixture()
        self.assertEqual(readiness(record, windows, state, sources, DEMO_SCOPES), "ready")
        for permission in (1, 3, 259):
            self.assertEqual(readiness(record, windows, state, sources, permission), "permission_denied")
        changes = [
            ("windows", "private_background_available", False, "unsupported_desktop"),
            ("state.configuration", "has_unsaved_changes", True, "demo_baseline_dirty"),
            ("state.configuration", "group", "Other", "demo_configuration_mismatch"),
            ("state.run", "gui_state", "stopping", "context_busy"),
            ("sources.source_snapshot", "read_basis", "", "context_busy"),
            ("sources.source_snapshot", "common_feature_enabled", True, "demo_baseline_not_empty"),
            ("sources", "next_offset", 1, "demo_baseline_not_empty"),
        ]
        for location, key, value, reason in changes:
            objects = copy.deepcopy(dict(windows=windows, state=state, sources=sources))
            target = objects
            for part in location.split("."):
                target = target[part]
            target[key] = value
            with self.subTest(location=location, key=key):
                self.assertEqual(readiness(record, **objects, granted=DEMO_SCOPES), reason)
        for index in (0, 1):
            changed = copy.deepcopy(sources)
            changed["items"][index]["row_count"] = 1
            self.assertEqual(readiness(record, windows, state, changed, DEMO_SCOPES), "demo_baseline_not_empty")
        for index, key, value in ((0, "minimized", True), (0, "native_visible", False), (1, "qt_visible", True)):
            changed = copy.deepcopy(windows)
            changed["items"][index][key] = value
            self.assertEqual(readiness(record, changed, state, sources, DEMO_SCOPES), "window_unavailable")
        changed = copy.deepcopy(sources)
        changed["source_snapshot"]["mapping_load_context"]["generation"] = "2"
        self.assertEqual(readiness(record, windows, state, changed, DEMO_SCOPES), "context_busy")
        self.assertEqual(readiness(record, windows, state, [], DEMO_SCOPES), "invalid_demo_state")


class DemoBridgeTests(unittest.IsolatedAsyncioTestCase):
    async def test_no_record_never_connects_or_queries(self):
        bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
        try:
            with patch.object(bridge, "_request") as request:
                value = await bridge.demo_state()
                self.assertEqual(value["result"]["reason"], "demo_not_configured")
                request.assert_not_called()
        finally:
            bridge.close()

    async def test_single_worker_read_only_requests_and_record_failure(self):
        record, windows, state, sources, target = fixture()
        bridge = Bridge(Path("unused/instance.json"), Path(record["executable"]), Path("unused/demo.json"))
        bridge._pipe = Mock(discovery=target)
        bridge._credential = SimpleNamespace(scopes=DEMO_SCOPES)
        own = SimpleNamespace(same_login=lambda other: True)
        try:
            with patch("qkeymapper_mcp.bridge.identity", side_effect=lambda pid: own if pid == os.getpid() else target.server), \
                    patch("qkeymapper_mcp.bridge.read_binding", return_value=record), \
                    patch.object(bridge, "_request", side_effect=[dict(ok=True, result=value) for value in (windows, state, sources)]) as request:
                value = await bridge.demo_state()
                self.assertTrue(value["result"]["ready"])
                self.assertEqual([call.args[0] for call in request.call_args_list], ["qkm_query", "qkm_get_state", "qkm_query"])
                self.assertEqual(value["result"]["reference_scope"], "preflight_observation_only")
            with patch("qkeymapper_mcp.bridge.identity", return_value=own), \
                    patch("qkeymapper_mcp.bridge.read_binding", side_effect=BridgeError("demo_record_rejected")), \
                    patch.object(bridge, "_request") as request:
                value = await bridge.demo_state()
                self.assertFalse(value["result"]["ready"])
                self.assertEqual(value["result"]["reason"], "demo_record_rejected")
                request.assert_not_called()
                self.assertIsNotNone(bridge._pipe)
        finally:
            bridge.close()


if __name__ == "__main__":
    unittest.main()
