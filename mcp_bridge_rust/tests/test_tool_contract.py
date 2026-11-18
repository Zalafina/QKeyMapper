"""Compare implemented Rust tools with the existing Python stdio contract."""

import argparse
import asyncio
import json
import sys
from pathlib import Path

from mcp import Client, StdioServerParameters

HERE = Path(__file__).resolve()
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "mcp_bridge/src"))
from qkeymapper_mcp.__main__ import _ERROR_FEEDBACK, _PRODUCT_ERRORS

CATALOG = HERE.parents[1] / "src/tool_catalog.json"
TOOLS = {
    "qkm_get_connection_state", "qkm_connect", "qkm_request_pairing", "qkm_disconnect",
    "qkm_get_capabilities", "qkm_get_state", "qkm_get_help", "qkm_query", "qkm_get_operation",
    "qkm_preview_changes", "qkm_apply_changes", "qkm_cancel_changes",
}


def config(exe, *args):
    absent = ROOT / "out/rust_sidecar/catalog-only/instance.json"
    return StdioServerParameters(command=str(exe), args=[
        *args, "--executable", sys.executable, "--discovery-file", str(absent),
    ])


def metadata(tools):
    return [tool.model_dump(by_alias=True, exclude_none=True) for tool in tools]


async def selected_catalog():
    async with Client(config(sys.executable, str(ROOT / "mcp_bridge/run_bridge.py"))) as client:
        tools = [tool for tool in (await client.list_tools()).tools if tool.name in TOOLS]
        assert {tool.name for tool in tools} == TOOLS
        return {"tools": metadata(tools), "feedback": {key: list(value) for key, value in _ERROR_FEEDBACK.items()},
                "reviewed_product_errors": sorted(_PRODUCT_ERRORS)}


async def compare(exe):
    baseline = await selected_catalog()
    assert json.loads(CATALOG.read_text(encoding="utf-8")) == baseline
    for mode in ("legacy", "2026-07-28"):
        async with Client(config(exe), mode=mode) as client, Client(
            config(sys.executable, str(ROOT / "mcp_bridge/run_bridge.py")), mode=mode,
        ) as python:
            assert metadata((await client.list_tools()).tools) == baseline["tools"]
            for sidecar in (client, python):
                unknown = await sidecar.call_tool("qkm_unknown")
                assert unknown.is_error and unknown.structured_content is None
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
            ):
                for index, arguments in enumerate((*invalids, valid)):
                    left = await client.call_tool(name, arguments)
                    right = await python.call_tool(name, arguments)
                    assert left.is_error and right.is_error, (mode, name, arguments)
                    if index == len(invalids):
                        assert left.structured_content == right.structured_content, (mode, name)
                        assert left.structured_content["error"] == "not_connected"
                        assert json.loads(left.content[0].text) == left.structured_content
                    else:
                        assert left.structured_content is None and right.structured_content is None
            print("PASS", mode, len(TOOLS), "tool contracts and error layers")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--export-catalog", action="store_true")
    group.add_argument("--exe", type=Path)
    args = parser.parse_args()
    if args.export_catalog:
        catalog = asyncio.run(selected_catalog())
        CATALOG.write_text(json.dumps(catalog, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
                           encoding="utf-8", newline="\n")
    else:
        asyncio.run(compare(args.exe.resolve(strict=True)))
