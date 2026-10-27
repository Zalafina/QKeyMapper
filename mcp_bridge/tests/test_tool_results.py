import asyncio
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from mcp import Client, StdioServerParameters
from mcp.server.mcpserver.tools import Tool

from qkeymapper_mcp.__main__ import ToolResult, tool_result


def stdio_error_fixture():
    from qkeymapper_mcp.__main__ import main
    from qkeymapper_mcp.bridge import Bridge
    codes = {"configurations": "permission_denied", "tabs": "stale_catalog",
             "mapping_sources": "stale_source", "mappings": "context_busy"}
    def request(method, arguments):
        if method in ("qkm_get_state", "qkm_get_operation"):
            return {"ok": True, "result": {"outcome": "failed", "error": "save_failed"}}
        if method == "qkm_apply_changes" and arguments.get("preview_id") == "f" * 32:
            return {"ok": False, "error": "operation_limit", "message": "Fixed in-flight result capacity rejection."}
        if method in ("qkm_preview_changes", "qkm_apply_changes", "qkm_cancel_changes"):
            return {"ok": False, "error": "stale_preview", "message": "Fixed fixture rejection."}
        return {"ok": False, "error": codes[arguments["entity"]], "message": "Fixed fixture rejection."}
    # Only this test child replaces the endpoint; the registered tools and SDK transport are real.
    with patch.object(Bridge, "_request", side_effect=request), \
         patch.object(sys, "argv", ["fixture", "--discovery-file", "unused/instance.json", "--executable", sys.executable]):
        main()


class ToolResultTests(unittest.TestCase):
    def test_state_comparison_excludes_only_the_validated_sample_time(self):
        from test_sdk_interop import business_state
        before = {"configuration": {"has_unsaved_changes": False},
                  "run": {"gui_state": "idle", "sampled_at_utc": "2026-10-25T08:00:00.000Z"}}
        after = dict(before, run=dict(before["run"], sampled_at_utc="2026-10-25T08:00:01.000Z"))
        self.assertEqual(business_state(before), business_state(after))
        self.assertNotEqual(business_state(before), business_state(dict(after,
            configuration={"has_unsaved_changes": True})))
        self.assertNotEqual(business_state(before), business_state(dict(after,
            run=dict(after["run"], gui_state="stopping"))))
        with self.assertRaises((ValueError, AssertionError)):
            business_state(dict(after, run=dict(after["run"], sampled_at_utc="not a sample time")))

    def test_stdio_business_errors_and_failed_operation_data(self):
        from jsonschema import Draft202012Validator
        from test_sdk_interop import TOOLS, call
        async def exercise(directory):
            root = Path(__file__).resolve().parents[1]
            config = StdioServerParameters(command=sys.executable,
                args=["-B", "-c", "from test_tool_results import stdio_error_fixture; stdio_error_fixture()"],
                env={"PYTHONPATH": os.pathsep.join((str(root / "src"), str(root / "tests")))}, cwd=directory)
            async with Client(config) as client:
                tools = (await client.list_tools()).tools
                self.assertEqual({tool.name for tool in tools}, TOOLS)
                schema = next(tool for tool in tools if tool.name == "qkm_preview_changes").model_dump(by_alias=True)["inputSchema"]
                validator = Draft202012Validator(schema)
                mapping = {"source_revision": "a" * 64, "source_tab_ref": "a" * 64 + ":source:0",
                           "original_key": "A", "mapping_key": "B"}
                save = {"action": "configuration.save", "setting_name": "配置95"}
                for valid in (mapping, dict(mapping, action="mapping.add"), save):
                    self.assertFalse(list(validator.iter_errors(valid)), valid)
                for invalid in ({}, {"action": "configuration.save"}, dict(save, setting_name=None),
                                dict(save, original_key="A"), dict(mapping, mapping_key=None),
                                dict(mapping, setting_name="extra"), dict(save, unknown=True)):
                    self.assertTrue(list(validator.iter_errors(invalid)), invalid)
                for entity, code in (("configurations", "permission_denied"), ("tabs", "stale_catalog"),
                                     ("mapping_sources", "stale_source"), ("mappings", "context_busy")):
                    arguments = {"entity": entity}
                    if entity == "mappings":
                        arguments.update(source_revision="a" * 64, source_tab_ref="a" * 64 + ":source:0")
                    value = await call(client, "qkm_query", arguments, error=code)
                    self.assertEqual(value["message"], "Fixed fixture rejection.")
                value = await call(client, "qkm_get_state")
                self.assertEqual(value["result"]["outcome"], "failed")
                change = {"source_revision": "a" * 64, "source_tab_ref": "a" * 64 + ":source:0",
                          "original_key": "A", "mapping_key": "B"}
                for name, arguments in (("qkm_preview_changes", change),
                        ("qkm_preview_changes", {"action": "configuration.save", "setting_name": "配置95"}),
                        ("qkm_apply_changes", {"preview_id": "b" * 32}),
                        ("qkm_cancel_changes", {"preview_id": "b" * 32})):
                    await call(client, name, arguments, error="stale_preview")
                capacity = await call(client, "qkm_apply_changes", {"preview_id": "f" * 32}, error="operation_limit")
                self.assertEqual(capacity["message"], "Fixed in-flight result capacity rejection.")
                self.assertTrue(capacity["next_steps"])
                value = await call(client, "qkm_get_operation", {"operation_id": "b" * 32})
                self.assertEqual(value["result"]["outcome"], "failed")
        with tempfile.TemporaryDirectory() as directory:
            asyncio.run(exercise(directory))

    def test_call_failures_set_error_and_actionable_structured_content(self):
        for value, code in (({"ok": False, "error": "not_connected"}, "not_connected"),
                            ({"accepted": False, "ok": False, "error": "busy_or_closed"}, "busy_or_closed"),
                            ({"ok": False, "error": "permission_denied"}, "permission_denied"),
                            ({"ok": False, "error": "stale_source"}, "stale_source")):
            with self.subTest(code=code):
                result = tool_result(value)
                self.assertTrue(result.is_error)
                content = result.structured_content
                self.assertIs(content["ok"], False)
                self.assertEqual(content["error"], code)
                self.assertTrue(content["message"])
                self.assertTrue(content["next_steps"])
                self.assertEqual(json.loads(result.content[0].text), content)

    def test_failed_state_or_operation_is_successfully_queried_data(self):
        for value in ({"status": "failed", "error": "application_unavailable", "busy": False},
                      {"ok": True, "result": {"outcome": "failed", "error": "save_failed"}},
                      {"accepted": True, "status": "failed", "error": "application_unavailable"}):
            with self.subTest(value=value):
                result = tool_result(value)
                self.assertFalse(result.is_error)
                self.assertEqual(result.structured_content, value)

    def test_reviewed_product_explanation_is_preserved(self):
        result = tool_result({"ok": False, "error": "context_busy", "message": "Not ready.",
                              "next_steps": ["untrusted action"]})
        self.assertEqual(result.structured_content["message"], "Not ready.")
        self.assertNotIn("untrusted action", result.structured_content["next_steps"])

    def test_native_and_unknown_errors_do_not_leak_exception_text(self):
        for code in ("internal_error", "native_operation_failed", "future_error"):
            with self.subTest(code=code):
                result = tool_result({"ok": False, "error": code, "message": "PRIVATE_EXCEPTION_TEXT"})
                self.assertTrue(result.is_error)
                self.assertEqual(result.structured_content["error"], code)
                self.assertNotIn("PRIVATE_EXCEPTION_TEXT", result.model_dump_json())

    def test_native_sdk_result_retains_dictionary_output_schema(self):
        def qkm_fixture() -> ToolResult:
            return tool_result({"ok": True, "result": {"value": 1}})
        definition = Tool.from_function(qkm_fixture, structured_output=True)
        self.assertEqual(definition.output_schema["type"], "object")
        for value in ({"ok": True, "result": {"value": 1}}, {"ok": False, "error": "not_connected"}):
            result = tool_result(value)
            self.assertIs(definition.fn_metadata.convert_result(result), result)


if __name__ == "__main__":
    unittest.main()
