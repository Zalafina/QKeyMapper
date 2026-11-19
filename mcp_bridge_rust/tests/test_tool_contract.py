"""Check the committed MCP contract and independent wire expectations via stdio."""

import argparse
import asyncio
import json
import subprocess
import sys
from pathlib import Path

from mcp import Client, StdioServerParameters

HERE = Path(__file__).resolve()
ROOT = HERE.parents[2]
CATALOG = HERE.parents[1] / "src/tool_catalog.json"
DEMO_STATE = "qkm_get_demo_state"
DEMO_MAPPING = "qkm_demo_mapping"
RECORD = ROOT / "out/rust_sidecar/catalog-only/creation.json"
TOOLS = {
    "qkm_get_connection_state", "qkm_connect", "qkm_request_pairing", "qkm_disconnect",
    "qkm_get_capabilities", "qkm_get_state", "qkm_wait_state", "qkm_get_help", "qkm_query", "qkm_get_operation",
    "qkm_preview_changes", "qkm_apply_changes", "qkm_cancel_changes", "qkm_stop_mapping", "qkm_start_mapping",
    "qkm_open_mapping_editor", "qkm_close_mapping_editor", "qkm_open_mapping_settings",
    "qkm_close_mapping_settings", "qkm_capture_window",
}


def config(exe, *args):
    absent = ROOT / "out/rust_sidecar/catalog-only/instance.json"
    return StdioServerParameters(command=str(exe), args=[
        *args, "--executable", str(exe), "--discovery-file", str(absent),
    ])


def metadata(tools):
    return [tool.model_dump(by_alias=True, exclude_none=True) for tool in tools]


def expect_not_connected(result):
    assert result.is_error is True
    expected = {
        "ok": False,
        "error": "not_connected",
        "message": "尚未连接 QKeyMapper。",
        "next_steps": ["查询连接状态，再使用已有配对连接；需要配对时由用户在产品内批准。"],
    }
    assert result.structured_content == expected, result
    assert len(result.content) == 1 and result.content[0].type == "text"
    assert json.loads(result.content[0].text) == expected


async def check_contract(exe):
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    assert {tool["name"] for tool in catalog["tools"]} == TOOLS | {DEMO_STATE, DEMO_MAPPING}
    revision = "a" * 64
    tab = revision + ":source:0"
    row = revision + ":row:0:0"
    window = "b" * 64
    editor = {"source_ref": row, "presentation": "private_background"}
    settings = {"source_tab_ref": tab, "presentation": "private_background"}
    capture = {"role": "ibinput_settings", "window_ref": window, "source_revision": revision, "source_tab_ref": tab}
    start = {"instance": "a" * 32, "request_id": "b" * 32, "source_revision": revision,
             "source_tab_ref": tab, "save_policy": "start_only"}
    for mode in ("legacy", "2026-07-28"):
        async with Client(config(exe), mode=mode, cache=None) as client:
            tools = (await client.list_tools()).tools
            assert len(tools) == len(TOOLS)
            assert metadata(tools) == [tool for tool in catalog["tools"] if tool["name"] in TOOLS]
            stop = next(tool for tool in tools if tool.name == "qkm_stop_mapping")
            assert stop.annotations.read_only_hint is False and stop.annotations.idempotent_hint is True
            assert stop.input_schema["additionalProperties"] is False
            assert set(stop.input_schema["required"]) == {"instance", "request_id"}
            assert set(stop.input_schema["properties"]) == {"instance", "request_id"}
            for field in stop.input_schema["properties"].values():
                assert field["type"] == "string" and field["pattern"] == "^[0-9a-f]{32}$"
                assert field["minLength"] == field["maxLength"] == 32
            start_tool = next(tool for tool in tools if tool.name == "qkm_start_mapping")
            assert set(start_tool.input_schema["required"]) == set(start)
            assert start_tool.input_schema["properties"]["save_policy"]["const"] == "start_only"
            for name in ("qkm_unknown", DEMO_STATE, DEMO_MAPPING):
                unknown = await client.call_tool(name)
                assert unknown.is_error is True and unknown.structured_content is None
            for name, valid, invalids in (
                ("qkm_get_help", {"topic": "basic_mapping"}, (
                    {}, {"topic": "unknown"}, {"topic": True}, {"topic": "mapping_keys", "offset": -1},
                    {"topic": "original_keys", "limit": 101}, {"topic": "original_keys", "offset": 1.0},
                    {"topic": "original_keys", "limit": None}, {"topic": "basic_mapping", "extra": True})),
                ("qkm_query", {"entity": "tabs"}, (
                    {"entity": "unknown"}, {"entity": "tabs", "offset": True},
                    {"entity": "tabs", "offset": 2147483648}, {"entity": "tabs", "limit": 0},
                    {"entity": "windows", "window_role": "item_setup"},
                    {"entity": "tabs", "catalog_revision": "A" * 64},
                    {"entity": "mappings", "source_tab_ref": "a" * 64 + ":source:01"})),
                ("qkm_wait_state", {"condition": "stopped_and_quiescent"}, (
                    {}, {"condition": "running"}, {"condition": True}, {"condition": None},
                    *({"condition": "mapping_active", "timeout_ms": timeout}
                      for timeout in (99, 5001, 1000.0, True, None)),
                    {"condition": "monitoring_enabled", "extra": True})),
                ("qkm_stop_mapping", {"instance": "a" * 32, "request_id": "b" * 32}, (
                    {}, {"request_id": "b" * 32}, {"instance": "a" * 32},
                    {"instance": "A" * 32, "request_id": "b" * 32},
                    {"instance": "a" * 32, "request_id": None},
                    {"instance": "a" * 32, "request_id": "b" * 32, "force": True})),
                ("qkm_start_mapping", start, (
                    {}, {**start, "save_policy": "save"}, {**start, "source_revision": "b" * 64},
                    {**start, "source_tab_ref": row}, {**start, "instance": "A" * 32}, {**start, "force": True})),
                ("qkm_get_operation", {"operation_id": "a" * 32},
                 ({}, {"operation_id": "A" * 32}, {"operation_id": None}, {"operation_id": "bad"})),
                ("qkm_preview_changes", {"action": "configuration.save", "setting_name": "\u8bbe" * 32767}, (
                    {"action": "unknown"}, {"action": None}, {"send_method": True}, {"send_method": 4},
                    {"source_ref": "a" * 64 + ":row:0:01"}, {"source_refs": []},
                    {"setting_name": ""}, {"setting_name": "A" * 32768}, {"key_up_mapping_key": "AB"},
                    {"insert_row": 2147483648}, {"original_key": "a"}, {"disabled": 1},
                    {"unavailable_policy": "fallback"})),
                ("qkm_apply_changes", {"preview_id": "a" * 32, "confirm_scope": True}, (
                    {}, {"preview_id": "?" * 32}, {"preview_id": "a" * 32, "confirm_scope": 1},
                    {"preview_id": "a" * 32, "confirm_overwrite": None})),
                ("qkm_cancel_changes", {"preview_id": "a" * 32}, (
                    {}, {"preview_id": 1}, {"preview_id": "a" * 32, "confirm_delete": True})),
                ("qkm_open_mapping_editor", editor, (
                    {}, {**editor, "presentation": "Default"}, {**editor, "source_ref": tab},
                    {**editor, "window_ref": window})),
                ("qkm_close_mapping_editor", {**editor, "window_ref": window}, (
                    editor, {**editor, "window_ref": 1}, {**editor, "window_ref": window, "discard": True})),
                ("qkm_open_mapping_settings", settings, (
                    {}, {**settings, "window_role": "main"}, {**settings, "window_role": None})),
                ("qkm_close_mapping_settings", {**settings, "window_role": "ibinput_settings", "window_ref": window}, (
                    settings, {**settings, "window_ref": window, "window_role": "item_setup"})),
                ("qkm_capture_window", capture, (
                    {}, {**capture, "role": "other"}, {**capture, "source_revision": "A" * 64},
                    {**capture, "window_ref": window[:63]}, {**capture, "path": "capture.png"})),
            ):
                for arguments in invalids:
                    denied = await client.call_tool(name, arguments)
                    assert denied.is_error is True and denied.structured_content is None, (mode, name, arguments)
                expect_not_connected(await client.call_tool(name, valid))
            print("PASS", mode, len(TOOLS), "tool contracts and error layers")
        for flags in (("--demo-record", str(RECORD)), ("--product-demo-record",),
                      ("--product-demo-record", "--enable-demo-mapping")):
            async with Client(config(exe, *flags), mode=mode, cache=None) as client:
                enabled = "--enable-demo-mapping" in flags
                assert metadata((await client.list_tools()).tools) == [
                    tool for tool in catalog["tools"] if tool["name"] != DEMO_MAPPING or enabled]
                expect_not_connected(await client.call_tool(DEMO_STATE))
                if enabled:
                    mapping = {"original_key": "A", "mapping_key": "B"}
                    expect_not_connected(await client.call_tool(DEMO_MAPPING, mapping))
                    for extra in ({"view": "ibinput_settings"}, {"original_key": "a"}, {"pid": 123}):
                        denied = await client.call_tool(DEMO_MAPPING, mapping | extra)
                        assert denied.is_error is True and denied.structured_content is None
        print("PASS", mode, "explicit and product demo-record tool gates")

    parameters = config(exe, "--enable-demo-mapping")
    rejected = subprocess.run([parameters.command, *parameters.args], capture_output=True, timeout=5)
    assert rejected.returncode == 2 and rejected.stdout == b""


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    args = parser.parse_args()
    if sys.flags.optimize:
        parser.error("Run with assertions enabled.")
    asyncio.run(check_contract(args.exe.resolve(strict=True)))
