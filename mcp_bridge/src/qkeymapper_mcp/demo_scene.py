"""One fixed memory-only scene, using the existing authenticated business operations."""

import re
import uuid

from .demo import readiness


class SceneError(Exception):
    def __init__(self, code):
        self.code = code


class DemoSession:
    def __init__(self):
        self.receipt = None
        self.quarantined = False
        self.generation = 0
        self.last = {}

    def status(self):
        return dict(receipt_available=self.receipt is not None, quarantined=self.quarantined,
                    generation=self.generation, last=dict(self.last))

    def run(self, request, binding, granted, original, mapped, view="item_setup"):
        if self.quarantined:
            return dict(ok=False, error="demo_quarantined", result=self.status())
        self.generation += 1
        self.last = dict(scene_id=uuid.uuid4().hex, generation=self.generation, stage="preflight", reset_verified=False)
        target = {key: binding[key] for key in ("instance", "pid", "created", "executable", "desktop")}
        had_receipt = self.receipt is not None
        mutating = reset_started = reset_verified = False
        owned = opened = None
        operations = {}

        def check(condition, code="invalid_demo_state"):
            if not condition:
                raise SceneError(code)

        def call(stage, method, arguments=None):
            self.last = {**self.last, "stage": stage, "operations": dict(operations)}
            value = request(method, arguments or {})
            check(value.get("ok") is True, value.get("error", "invalid_demo_state"))
            value = value.get("result")
            check(type(value) is dict)
            return value

        def observe(expected_dirty):
            windows = call("observe_windows", "qkm_query", dict(entity="windows"))
            state = call("observe_state", "qkm_get_state")
            sources = call("observe_sources", "qkm_query", dict(entity="mapping_sources", limit=100))
            reason = readiness(binding, windows, state, sources, granted, expected_dirty=expected_dirty)
            check(reason == "ready", reason)
            context = call("observe_context", "qkm_query", dict(entity="demo_context"))
            check(context.get("source_snapshot") == sources.get("source_snapshot"), "demo_context_changed")
            check(context.get("reference_scope") == "non_row_save_footprint_only"
                  and context.get("mapping_rows_excluded") is True and context.get("write_authority") is False
                  and type(context.get("context_revision")) is str
                  and re.fullmatch(r"[0-9a-f]{64}", context["context_revision"]) is not None)
            return sources, context

        def post_binding(operation, row):
            check(operation.get("status") == "applied", "demo_operation_not_applied")
            value = operation.get("result", {})
            check(value.get("modified") is True and value.get("saved") is False
                  and value.get("started_by_operation") is False)
            result = value.get("source_binding_after")
            check(value.get("source_binding_status") == "captured" and type(result) is dict,
                  "demo_source_binding_unavailable")
            snapshot = result.get("source_snapshot", {})
            revision = snapshot.get("revision")
            check(type(revision) is str and re.fullmatch(r"[0-9a-f]{64}", revision) is not None
                  and snapshot.get("instance") == target["instance"]
                  and result.get("reference_scope") == "commit_snapshot_only"
                  and result.get("source_tab_ref") == revision + ":source:0"
                  and result.get("source_ref") == (revision + ":row:0:0" if row else None))
            return result

        def verify_owned():
            arguments = dict(source_revision=owned["source_snapshot"]["revision"], source_tab_ref=owned["source_tab_ref"])
            listing = call("verify_owned_sources", "qkm_query", dict(entity="mapping_sources",
                source_revision=arguments["source_revision"], limit=100))
            expected = [{**item, "source_tab_ref": arguments["source_revision"] + ":source:" + str(index),
                         "row_count": 1 if index == 0 else 0} for index, item in enumerate(initial_sources["items"])]
            check(listing.get("source_snapshot") == owned["source_snapshot"] and listing.get("items") == expected
                  and listing.get("total") == 2 and listing.get("next_offset") is None, "demo_context_changed")
            rows = call("verify_owned_row", "qkm_query", dict(entity="mappings", limit=100, **arguments))
            expected_row = {**template, "source_ref": owned["source_ref"], "source_tab_ref": owned["source_tab_ref"], "source_row": 0}
            check(rows.get("source_snapshot") == owned["source_snapshot"] and rows.get("items") == [expected_row]
                  and rows.get("total") == 1 and rows.get("next_offset") is None, "demo_context_changed")
            context = call("verify_owned_context", "qkm_query", dict(entity="demo_context"))
            check(context.get("source_snapshot") == owned["source_snapshot"]
                  and context.get("context_revision") == baseline_context, "demo_context_changed")

        def reset():
            nonlocal opened, reset_verified
            if opened is not None:
                closed = call("close_editor", "qkm_close_mapping_editor", dict(source_ref=owned["source_ref"],
                    window_ref=opened["window"]["window_ref"], presentation="private_background"))
                check(closed.get("closed") is True and closed.get("target") == target)
                opened = None
            verify_owned()
            preview = call("preview_cleanup", "qkm_preview_changes", dict(action="mapping.delete",
                source_revision=owned["source_snapshot"]["revision"], source_tab_ref=owned["source_tab_ref"], source_refs=[owned["source_ref"]]))
            description = preview.get("description", {})
            changes = description.get("changes", [])
            check(description.get("action") == "mapping.delete" and len(changes) == 1
                  and changes[0].get("source_ref") == owned["source_ref"] and changes[0].get("mapping") == template)
            operations["cleanup"] = preview["operation_id"]
            deleted = call("apply_cleanup", "qkm_apply_changes", dict(preview_id=preview["preview_id"], confirm_delete=True))
            final = post_binding(deleted, False)
            sources, context = observe(True)
            check(sources["source_snapshot"] == final["source_snapshot"]
                  and context["context_revision"] == baseline_context, "demo_context_changed")
            self.receipt = dict(target=target, source_snapshot=final["source_snapshot"], context_revision=baseline_context)
            reset_verified = True
            return dict(verified=True, source_snapshot=final["source_snapshot"], context_revision=baseline_context,
                        saved=False, started_by_operation=False, dirty_retained=True)

        try:
            caps = call("capabilities", "qkm_get_capabilities").get("business_capabilities", [])
            capabilities = {item.get("id"): item for item in caps if type(item) is dict}
            check(all(name in capabilities for name in ("mapping.add.preview", "mapping.add.apply", "mapping.delete.preview",
                "mapping.delete.apply", "mapping.editor.open", "mapping.editor.close", "windows.capture", "demo.context.read")),
                "unsupported_capability")
            check(all(capabilities[name].get("source_binding_result") == "commit_snapshot_or_null"
                      for name in ("mapping.add.apply", "mapping.delete.apply")), "unsupported_capability")
            roles = capabilities["windows.capture"].get("arguments_schema", {}).get("properties", {}).get("role", {}).get("enum", [])
            check(view in roles, "unsupported_capability")
            initial_sources, context = observe(had_receipt)
            baseline_context = context["context_revision"]
            if had_receipt:
                check(self.receipt == dict(target=target, source_snapshot=initial_sources["source_snapshot"],
                                          context_revision=baseline_context), "demo_context_changed")
            self.receipt = None
            preview = call("preview_mapping", "qkm_preview_changes", dict(original_key=original, mapping_key=mapped,
                source_revision=initial_sources["source_snapshot"]["revision"], source_tab_ref=initial_sources["items"][0]["source_tab_ref"]))
            description = preview.get("description", {})
            changes = description.get("changes", [])
            check(description.get("action") == "mapping.add" and description.get("confirmation_required") is False
                  and len(changes) == 1 and changes[0].get("kind") == "insert_mapping" and changes[0].get("source_row") == 0)
            template = changes[0]["mapping"]
            check(template.get("fields", {}).get("KeyMapData_OriginalKeys") == original
                  and template.get("fields", {}).get("KeyMapData_MappingKeys") == [mapped])
            operations["create"] = preview["operation_id"]
            mutating = True
            applied = call("apply_mapping", "qkm_apply_changes", dict(preview_id=preview["preview_id"]))
            owned = post_binding(applied, True)
            verify_owned()
            if view == "item_setup":
                opened = call("open_editor", "qkm_open_mapping_editor", dict(source_ref=owned["source_ref"], presentation="private_background"))
                check(opened.get("target") == target and type(opened.get("window")) is dict)
                window = opened["window"]
            else:
                windows = call("observe_capture_window", "qkm_query", dict(entity="windows"))
                check(windows.get("target") == target and windows.get("private_background_available") is True)
                window = next((item for item in windows.get("items", []) if item.get("role") == "main"), {})
                check(all(window.get(key) is True for key in ("qt_visible", "native_ready", "native_visible"))
                      and window.get("minimized") is False, "window_unavailable")
            capture = call("capture_editor" if view == "item_setup" else "capture_main", "qkm_capture_window", dict(role=view, window_ref=window["window_ref"],
                source_revision=owned["source_snapshot"]["revision"], source_tab_ref=owned["source_tab_ref"]))
            check(capture.get("target") == target and capture.get("role") == view
                  and (view == "main" or capture.get("edit_target", {}).get("source_ref") == owned["source_ref"]))
            reset_started = True
            cleanup = reset()
            self.last = {**self.last, "stage": "completed", "operations": dict(operations), "reset_verified": True}
            return dict(ok=True, result={**capture, "demo_scene": {**self.last, "original_key": original,
                "mapping_key": mapped, "view": view, "cleanup": cleanup, "delivery": "mcp_image_only_gateway_delivery_unverified"}})
        except SceneError as error:
            failed_stage = self.last["stage"]
            cleanup_error = None
            if owned is not None and not reset_started:
                reset_started = True
                try:
                    reset()
                except SceneError as failure:
                    cleanup_error = failure.code
            if (mutating or had_receipt) and not reset_verified:
                self.quarantined = True
                self.receipt = None
            self.last = {**self.last, "stage": "failed", "failed_stage": failed_stage, "error": error.code,
                         "cleanup_error": cleanup_error, "reset_verified": reset_verified, "operations": dict(operations)}
            return dict(ok=False, error=error.code, result=self.status())
        except BaseException:
            if mutating or had_receipt:
                self.quarantined = True
                self.receipt = None
            self.last = {**self.last, "transport_or_internal_failure": True, "reset_verified": False, "operations": dict(operations)}
            raise
        finally:
            if (mutating or had_receipt) and not reset_verified:
                self.quarantined = True
                self.receipt = None
