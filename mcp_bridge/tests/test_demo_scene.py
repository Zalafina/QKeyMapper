"""Focused orchestration checks; simulated windows are not native image acceptance."""

import copy
import asyncio
import threading
import time
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

import win32event

from qkeymapper_mcp.bridge import Bridge
from qkeymapper_mcp.demo import DEMO_SCOPES
from qkeymapper_mcp.demo_scene import DemoSession
from qkeymapper_mcp.protocol import BridgeError
from test_demo import fixture
from test_sdk_interop import BUSINESS_CAPABILITIES


class Product:
    def __init__(self):
        self.record, self.windows, self.state, self.sources, _ = fixture()
        self.windows["items"][0]["window_ref"] = "e" * 64
        self.windows["items"][1]["window_ref"] = "d" * 64
        self.revision = 1
        self.context = "c" * 64
        self.row = None
        self.preview = None
        self.calls = []
        self.fail = None
        self.mutate = None
        self.legacy = False
        self.roles = ("main", "item_setup", "mapping_advanced")
        self.missing = set()
        self.advanced = dict(role="mapping_advanced", window_ref="f" * 64, qt_visible=False)
        self.settings_unchanged = True
        self.dirty = False

    def snapshot(self):
        snapshot = copy.deepcopy(self.sources["source_snapshot"])
        snapshot["revision"] = f"{self.revision:064x}"
        snapshot["configuration_selection"]["has_unsaved_changes"] = self.dirty
        return snapshot

    def source_page(self):
        value = copy.deepcopy(self.sources)
        value["source_snapshot"] = self.snapshot()
        for index, item in enumerate(value["items"]):
            item["source_tab_ref"] = self.snapshot()["revision"] + ":source:" + str(index)
            item["row_count"] = int(self.row is not None) if index == 0 else 0
        return value

    def binding(self, row):
        revision = self.snapshot()["revision"]
        return dict(reference_scope="commit_snapshot_only", source_snapshot=self.snapshot(),
            source_tab_ref=revision + ":source:0", source_ref=revision + ":row:0:0" if row else None)

    def request(self, method, args):
        self.calls.append((method, copy.deepcopy(args)))
        if self.mutate:
            self.mutate(self, method, args)
        if self.fail:
            result = self.fail(method, args)
            if result is not None:
                return result
        if method == "qkm_get_capabilities":
            result = dict(business_capabilities=[dict(id=name, source_binding_result="legacy" if self.legacy else "commit_snapshot_or_null")
                for name in BUSINESS_CAPABILITIES if name not in self.missing])
            capture = next(item for item in result["business_capabilities"] if item["id"] == "windows.capture")
            capture["arguments_schema"] = dict(properties=dict(role=dict(enum=list(self.roles))))
        elif method == "qkm_get_state":
            result = copy.deepcopy(self.state)
            result["configuration"]["has_unsaved_changes"] = self.dirty
        elif method == "qkm_query":
            if "source_revision" in args and args["source_revision"] != self.snapshot()["revision"]:
                return dict(ok=False, error="stale_source")
            if args["entity"] == "windows":
                result = copy.deepcopy(self.windows)
            elif args["entity"] == "mapping_sources":
                result = self.source_page()
            elif args["entity"] == "demo_context":
                result = dict(source_snapshot=self.snapshot(), context_revision=self.context,
                    reference_scope="non_row_save_footprint_only", mapping_rows_excluded=True, write_authority=False)
            else:
                result = dict(source_snapshot=self.snapshot(), total=1, next_offset=None,
                    items=[{**self.row, "source_ref": self.binding(True)["source_ref"], "source_tab_ref": self.binding(True)["source_tab_ref"], "source_row": 0}])
        elif method == "qkm_preview_changes":
            self.preview = args.get("action", "mapping.add")
            if self.preview == "mapping.add":
                row = dict(fields=dict(KeyMapData_OriginalKeys=args["original_key"], KeyMapData_MappingKeys=[args["mapping_key"]]), derived={})
                self.pending = row
                changes = [dict(kind="insert_mapping", source_row=0, mapping=row)]
            else:
                changes = [dict(source_ref=self.binding(True)["source_ref"], mapping=self.row)]
            result = dict(preview_id="p", operation_id="o", description=dict(action=self.preview, confirmation_required=False, changes=changes))
        elif method == "qkm_apply_changes":
            self.row = self.pending if self.preview == "mapping.add" else None
            self.dirty = True
            self.revision += 1
            result = dict(status="applied", result=dict(modified=True, saved=False, started_by_operation=False,
                source_binding_status="captured", source_binding_after=self.binding(self.row is not None)))
        elif method == "qkm_open_mapping_editor":
            self.windows["items"][1]["qt_visible"] = True
            result = dict(target=self.windows["target"], window=self.windows["items"][1])
        elif method == "qkm_close_mapping_editor":
            self.windows["items"][1]["qt_visible"] = False
            result = dict(target=self.windows["target"], closed=True)
        elif method == "qkm_open_mapping_settings":
            self.advanced["qt_visible"] = True
            result = dict(target=self.windows["target"], window=self.advanced, opened=True, activation_requested=False,
                source_context=self.snapshot(), source_tab_ref=self.binding(True)["source_tab_ref"])
        elif method == "qkm_close_mapping_settings":
            self.advanced["qt_visible"] = False
            result = dict(target=self.windows["target"], closed=True)
        elif method == "qkm_capture_window":
            result = dict(target=self.windows["target"], role=args["role"], png_base64="simulated")
            if args["role"] == "item_setup":
                result["edit_target"] = dict(source_ref=self.binding(True)["source_ref"])
            elif args["role"] == "mapping_advanced":
                result.update(source_context=self.snapshot(), settings_scope="live_loaded_settings_not_committed_by_source_revision")
                if self.settings_unchanged is not None:
                    result["settings_editor_unchanged"] = self.settings_unchanged
        else:
            raise AssertionError(method)
        return dict(ok=True, result=copy.deepcopy(result))


class SceneTests(unittest.TestCase):
    def run_scene(self, product, session=None):
        session = session or DemoSession()
        return session, session.run(product.request, product.record, DEMO_SCOPES, "A", "B")

    def test_two_rounds_retain_dirty_and_use_only_commit_references(self):
        product = Product()
        session, result = self.run_scene(product)
        self.assertTrue(result["ok"], result)
        self.assertTrue(session.receipt and product.dirty)
        first = result["result"]["demo_scene"]["scene_id"]
        _, second = self.run_scene(product, session)
        self.assertTrue(second["ok"], second)
        self.assertNotEqual(first, second["result"]["demo_scene"]["scene_id"])
        self.assertEqual(product.revision, 5)
        self.assertIsNone(product.row)
        self.assertFalse(product.windows["items"][1]["qt_visible"])
        self.assertTrue(all(method not in ("qkm_connect", "qkm_request_pairing") for method, _ in product.calls))

    def test_legacy_and_permissions_fail_before_any_write(self):
        for legacy, scope in ((True, DEMO_SCOPES), (False, 1)):
            product = Product(); product.legacy = legacy
            session = DemoSession()
            result = session.run(product.request, product.record, scope, "A", "B")
            self.assertFalse(result["ok"])
            self.assertFalse(session.quarantined)
            self.assertFalse(any(method == "qkm_apply_changes" for method, _ in product.calls))

    def test_known_capture_failure_recovers_once_and_allows_next_round(self):
        product = Product()
        product.fail = lambda method, args: dict(ok=False, error="capture_failed") if method == "qkm_capture_window" else None
        session, result = self.run_scene(product)
        self.assertEqual(result["error"], "capture_failed")
        self.assertTrue(result["result"]["last"]["reset_verified"])
        self.assertFalse(session.quarantined)
        self.assertIsNone(product.row)
        product.fail = None
        self.assertTrue(self.run_scene(product, session)[1]["ok"])

    def test_foreign_source_and_non_row_change_are_never_deleted(self):
        for change in ("source", "context"):
            product = Product()
            def mutate(p, method, args):
                if method == "qkm_close_mapping_editor":
                    if change == "source":
                        p.revision += 1
                    else:
                        p.context = "e" * 64
            product.mutate = mutate
            session, result = self.run_scene(product)
            self.assertFalse(result["ok"])
            self.assertTrue(session.quarantined)
            self.assertIsNotNone(product.row)
            self.assertEqual(sum(method == "qkm_apply_changes" for method, _ in product.calls), 1)
            before = len(product.calls)
            self.assertEqual(self.run_scene(product, session)[1]["error"], "demo_quarantined")
            self.assertEqual(len(product.calls), before)

    def test_timeout_and_cancel_at_capture_or_failure_cleanup_quarantine(self):
        for code in ("timeout", "cancelled"):
            for cleanup in (False, True):
                product = Product()
                def fail(method, args):
                    if method == "qkm_capture_window":
                        if cleanup:
                            return dict(ok=False, error="capture_failed")
                        raise BridgeError(code)
                    if cleanup and method == "qkm_close_mapping_editor":
                        raise BridgeError(code)
                product.fail = fail
                session = DemoSession()
                with self.assertRaises(BridgeError):
                    self.run_scene(product, session)
                self.assertTrue(session.quarantined)
                self.assertIsNone(session.receipt)
                self.assertIsNotNone(product.row)
                self.assertEqual(sum(method == "qkm_apply_changes" for method, _ in product.calls), 1)

    def test_changed_completed_receipt_is_not_reused(self):
        for change in ("source", "context", "target"):
            product = Product(); session, result = self.run_scene(product)
            self.assertTrue(result["ok"])
            if change == "source":
                product.revision += 1
            elif change == "context":
                product.context = "e" * 64
            else:
                product.record["created"] = "999"
            before = sum(method == "qkm_apply_changes" for method, _ in product.calls)
            self.assertFalse(self.run_scene(product, session)[1]["ok"])
            self.assertTrue(session.quarantined)
            self.assertEqual(sum(method == "qkm_apply_changes" for method, _ in product.calls), before)

    def test_unavailable_post_binding_does_not_guess_or_retry(self):
        product = Product(); session = DemoSession()
        def request(method, args):
            value = product.request(method, args)
            if method == "qkm_apply_changes":
                value["result"]["result"].update(source_binding_status="unavailable", source_binding_after=None)
            return value
        result = session.run(request, product.record, DEMO_SCOPES, "A", "B")
        self.assertEqual(result["error"], "demo_source_binding_unavailable")
        self.assertTrue(session.quarantined)
        self.assertIsNotNone(product.row)
        self.assertEqual(sum(method == "qkm_apply_changes" for method, _ in product.calls), 1)

    def test_editor_draft_failure_is_preserved_without_close_retry(self):
        product = Product()
        product.fail = lambda method, args: dict(ok=False, error="editor_draft_changed") if method == "qkm_close_mapping_editor" else None
        session, result = self.run_scene(product)
        self.assertEqual(result["error"], "editor_draft_changed")
        self.assertTrue(session.quarantined and product.windows["items"][1]["qt_visible"])
        self.assertIsNotNone(product.row)
        self.assertEqual(sum(method == "qkm_close_mapping_editor" for method, _ in product.calls), 1)
        self.assertEqual(sum(method == "qkm_apply_changes" for method, _ in product.calls), 1)

    def test_main_view_captures_the_existing_main_window_without_editor_calls(self):
        product = Product(); session = DemoSession()
        result = session.run(product.request, product.record, DEMO_SCOPES, "A", "B", "main")
        self.assertTrue(result["ok"], result)
        self.assertEqual(result["result"]["role"], "main")
        self.assertEqual(result["result"]["demo_scene"]["view"], "main")
        self.assertFalse(any(method in ("qkm_open_mapping_editor", "qkm_close_mapping_editor") for method, _ in product.calls))
        self.assertIsNone(product.row)
        self.assertTrue(session.receipt)

    def test_unsupported_view_contract_rejects_before_mutation(self):
        product = Product(); product.roles = ("item_setup",)
        result = DemoSession().run(product.request, product.record, DEMO_SCOPES, "A", "B", "main")
        self.assertEqual(result["error"], "unsupported_capability")
        self.assertFalse(any(method == "qkm_apply_changes" for method, _ in product.calls))

    def test_advanced_then_default_view_use_their_own_source_contract_and_cleanup(self):
        product = Product(); session = DemoSession()
        result = session.run(product.request, product.record, DEMO_SCOPES, "A", "B", "mapping_advanced")
        self.assertTrue(result["ok"], result)
        self.assertEqual(result["result"]["demo_scene"]["view"], "mapping_advanced")
        self.assertTrue(result["result"]["settings_editor_unchanged"])
        self.assertFalse(product.advanced["qt_visible"])
        self.assertIsNone(product.row)
        self.assertTrue(session.receipt)
        lifecycle = [(method, args) for method, args in product.calls if method in ("qkm_open_mapping_settings", "qkm_close_mapping_settings")]
        self.assertEqual([method for method, _ in lifecycle], ["qkm_open_mapping_settings", "qkm_close_mapping_settings"])
        for _, args in lifecycle:
            self.assertEqual(args["source_tab_ref"], result["result"]["source_context"]["revision"] + ":source:0")
            self.assertNotIn("source_ref", args)
        self.assertFalse(any(method in ("qkm_open_mapping_editor", "qkm_close_mapping_editor") for method, _ in product.calls))
        self.assertTrue(session.run(product.request, product.record, DEMO_SCOPES, "C", "D")["ok"])
        self.assertFalse(product.windows["items"][1]["qt_visible"])
        self.assertEqual(session.generation, 2)

    def test_missing_advanced_capability_rejects_before_writing(self):
        for capability in ("mapping.settings.open", "mapping.settings.close"):
            product = Product(); product.missing = {capability}
            result = DemoSession().run(product.request, product.record, DEMO_SCOPES, "A", "B", "mapping_advanced")
            self.assertEqual(result["error"], "unsupported_capability")
            self.assertFalse(any(method == "qkm_apply_changes" for method, _ in product.calls))

    def test_unconfirmed_advanced_image_is_not_returned_even_if_safe_cleanup_succeeds(self):
        for matched in (False, None, 1):
            product = Product(); product.settings_unchanged = matched
            session = DemoSession()
            result = session.run(product.request, product.record, DEMO_SCOPES, "A", "B", "mapping_advanced")
            self.assertEqual(result["error"], "demo_settings_changed")
            self.assertNotIn("png_base64", result["result"])
            self.assertTrue(result["result"]["last"]["reset_verified"])
            self.assertFalse(session.quarantined or product.advanced["qt_visible"])
            self.assertIsNone(product.row)

    def test_advanced_draft_failure_preserves_scene_without_close_retry(self):
        product = Product(); session = DemoSession()
        product.fail = lambda method, args: dict(ok=False, error="editor_draft_changed") if method == "qkm_close_mapping_settings" else None
        result = session.run(product.request, product.record, DEMO_SCOPES, "A", "B", "mapping_advanced")
        self.assertEqual(result["error"], "editor_draft_changed")
        self.assertTrue(session.quarantined and product.advanced["qt_visible"])
        self.assertIsNotNone(product.row)
        self.assertEqual(sum(method == "qkm_close_mapping_settings" for method, _ in product.calls), 1)
        self.assertEqual(sum(method == "qkm_apply_changes" for method, _ in product.calls), 1)

    def test_unknown_advanced_window_results_do_not_trigger_cleanup_or_retry(self):
        for stage in ("qkm_open_mapping_settings", "qkm_capture_window", "qkm_close_mapping_settings"):
            product = Product(); session = DemoSession()
            def fail(method, args):
                if method == stage:
                    raise BridgeError("timeout")
            product.fail = fail
            with self.assertRaises(BridgeError):
                session.run(product.request, product.record, DEMO_SCOPES, "A", "B", "mapping_advanced")
            self.assertTrue(session.quarantined)
            self.assertIsNone(session.receipt)
            self.assertIsNotNone(product.row)
            self.assertEqual(sum(method == stage for method, _ in product.calls), 1)
            self.assertEqual(sum(method == "qkm_apply_changes" for method, _ in product.calls), 1)


class SceneBridgeTests(unittest.IsolatedAsyncioTestCase):
    async def test_explicit_flag_and_input_boundary(self):
        with self.assertRaises(ValueError):
            Bridge(Path("unused/instance.json"), Path("unused.exe"), enable_demo_mapping=True)
        bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
        try:
            with patch.object(bridge, "_request") as request:
                self.assertEqual((await bridge.demo_mapping("A", "B"))["error"], "demo_not_enabled")
                request.assert_not_called()
        finally:
            bridge.close()
        bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"), Path("unused/demo.json"), enable_demo_mapping=True)
        try:
            with patch.object(bridge, "_request") as request:
                for key in ("AA", "a", "CTRL", 1, "\n"):
                    self.assertEqual((await bridge.demo_mapping(key, "B"))["error"], "invalid_arguments")
                for view in ("other", "Default", 1, None):
                    self.assertEqual((await bridge.demo_mapping("A", "B", view))["error"], "invalid_arguments")
                request.assert_not_called()
        finally:
            bridge.close()

    async def test_single_worker_busy_and_cancel_quarantine(self):
        for cancel in (False, True):
            product = Product()
            bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"), Path("unused/demo.json"), enable_demo_mapping=True)
            bridge._pipe = Mock()
            bridge._credential = Mock(scopes=DEMO_SCOPES)
            entered, release = threading.Event(), threading.Event()
            def request(method, args):
                if method == "qkm_capture_window":
                    entered.set()
                    until = time.monotonic() + 3
                    while not release.wait(.01):
                        if win32event.WaitForSingleObject(bridge._cancel, 0) == win32event.WAIT_OBJECT_0:
                            raise BridgeError("cancelled")
                        if time.monotonic() >= until:
                            raise AssertionError("simulated capture wait timed out")
                return product.request(method, args)
            try:
                with patch.object(bridge, "_verified_demo_binding", return_value=product.record), patch.object(bridge, "_request", side_effect=request):
                    task = asyncio.create_task(bridge.demo_mapping("A", "B"))
                    self.assertTrue(await asyncio.to_thread(entered.wait, 2))
                    self.assertEqual((await bridge.query("mapping_sources"))["error"], "busy_or_closed")
                    if cancel:
                        task.cancel()
                        with self.assertRaises(asyncio.CancelledError):
                            await task
                        until = time.monotonic() + 2
                        while bridge.state()["busy"]:
                            self.assertLess(time.monotonic(), until)
                            await asyncio.sleep(.01)
                        state = bridge.state()
                        self.assertEqual(state["status"], "disconnected")
                        self.assertTrue(state["demo_session"]["quarantined"])
                        self.assertEqual(state["demo_session"]["last"]["stage"], "capture_editor")
                        self.assertIsNotNone(product.row)
                    else:
                        release.set()
                        self.assertTrue((await task)["ok"])
            finally:
                release.set()
                bridge.close()


if __name__ == "__main__":
    unittest.main()
