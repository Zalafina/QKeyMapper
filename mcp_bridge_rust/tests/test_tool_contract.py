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
    move = {"action": "mapping.move_up", "source_ref": row}
    down = {**move, "action": "mapping.move_down"}
    top = {**move, "action": "mapping.move_to_top"}
    bottom = {**move, "action": "mapping.move_to_bottom"}
    refs = [row, revision + ":row:0:1"]
    batch = {"action": "mapping.move_to_top", "source_refs": refs}
    moves = ["mapping.move_up", "mapping.move_down", "mapping.move_to_top", "mapping.move_to_bottom"]
    note = {"action": "mapping.update", "source_ref": row, "note": "  测试#分组|備考😀  "}
    category = {"action": "mapping.update", "source_ref": row, "category": "  组 #A|分類  "}
    other_row = row.rsplit(":", 1)[0] + ":3"
    category_batch = {"action": "mapping.update", "source_refs": [other_row, row], "category": "批量分類"}
    category_filter = {"action": "tab.set_category_filter", "source_tab_ref": tab, "category_filter": ["", "批量分類"]}
    disabled_visibility = {"action": "configuration.set_show_disabled_rows", "source_tab_ref": tab, "show_disabled_rows": True}
    row_modes = {"action": "mapping.update", "source_ref": row, "row_modes": {"burst": True, "lock": False}}
    send_timing = {"action": "mapping.update", "source_ref": row, "send_timing": 4}
    display = {"action": "configuration.set_mapping_display", "source_tab_ref": tab,
               "mapping_display": {"category_column": True, "notes": False}}
    rename = {"action": "tab.rename", "source_tab_ref": tab, "tab_name": "测试 Tab"}
    hotkey = {"action": "tab.set_hotkey", "source_tab_ref": tab, "tab_hotkey": " $ F9 "}
    notification = {"action": "tab.set_notification_mode", "source_tab_ref": tab, "notification_mode": 0}
    colors = {"action": "tab.set_notification_colors", "source_tab_ref": tab,
              "notification_font_color": "#123456", "notification_background_color": "#80405060"}
    add_tab = {"action": "tab.add_blank", "source_tab_ref": tab}
    copy_tab = {"action": "tab.copy", "source_tab_ref": tab}
    delete_tab = {"action": "tab.delete", "source_tab_ref": tab}
    clear_tab = {"action": "tab.clear", "source_tab_ref": tab}
    move_tab = {"action": "tab.move", "source_tab_ref": tab, "target_tab_ref": revision + ":source:2"}
    select_tab = {**move_tab, "action": "tab.select"}
    common_inheritance = {"action": "tab.set_common_inheritance", "source_tab_ref": tab, "include_common": False}
    common_collapsed = {"action": "tab.set_common_collapsed", "source_tab_ref": tab, "common_collapsed": True}
    common_feature = {"action": "configuration.set_common_enabled", "source_tab_ref": tab, "common_feature_enabled": False}
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
            previews = next(tool.input_schema for tool in tools if tool.name == "qkm_preview_changes")
            move_rule, = (rule for rule in previews["oneOf"]
                          if rule["properties"]["action"].get("enum") == moves)
            assert move_rule["required"] == ["action"]
            single_rule, batch_rule = move_rule["oneOf"]
            assert single_rule["required"] == ["source_ref"]
            assert single_rule["properties"] == {"source_ref": {"type": "string"}, "source_refs": {"type": "null"}}
            assert batch_rule["required"] == ["source_refs"]
            assert batch_rule["properties"] == {
                "source_ref": {"type": "null"}, "source_refs": {"type": "array", "minItems": 2}}
            assert set(move_rule["properties"]) == set(previews["properties"]) - {"copy_to"}
            assert all(rule == {"type": "null"} for field, rule in move_rule["properties"].items()
                       if field not in {"action", "source_ref", "source_refs"})
            rename_rule, = (rule for rule in previews["oneOf"]
                            if rule["properties"]["action"].get("const") == "tab.rename")
            assert set(rename_rule["required"]) == set(rename)
            assert rename_rule["properties"]["source_tab_ref"] == {"type": "string"}
            assert rename_rule["properties"]["tab_name"] == {"type": "string"}
            assert set(rename_rule["properties"]) == set(previews["properties"]) - {"copy_to"}
            assert all(rule == {"type": "null"} for field, rule in rename_rule["properties"].items()
                       if field not in rename)
            add_rule, = (rule for rule in previews["oneOf"]
                         if "tab.add_blank" in rule["properties"]["action"].get("enum", []))
            assert set(add_rule["properties"]["action"]["enum"]) == {"tab.add_blank", "tab.copy", "tab.delete", "tab.clear"}
            assert set(add_rule["required"]) == set(add_tab) == set(copy_tab) == set(delete_tab) == set(clear_tab)
            assert add_rule["properties"]["source_tab_ref"] == {"type": "string"}
            assert set(add_rule["properties"]) == set(previews["properties"]) - {"copy_to"}
            assert all(rule == {"type": "null"} for field, rule in add_rule["properties"].items()
                       if field not in add_tab)
            move_tab_rule, = (rule for rule in previews["oneOf"]
                              if rule["properties"]["action"].get("const") == "tab.move")
            assert set(move_tab_rule["required"]) == set(move_tab)
            assert move_tab_rule["properties"]["target_tab_ref"] == {"type": "string"}
            assert set(move_tab_rule["properties"]) == set(previews["properties"]) - {"copy_to"}
            selection_rule, = (rule for rule in previews["oneOf"]
                                 if rule["properties"]["action"].get("const") == "tab.select")
            assert set(selection_rule["required"]) == set(select_tab)
            assert selection_rule["properties"]["target_tab_ref"] == {"type": "string"}
            hotkey_rule, = (rule for rule in previews["oneOf"]
                              if rule["properties"]["action"].get("const") == "tab.set_hotkey")
            assert set(hotkey_rule["required"]) == set(hotkey)
            assert hotkey_rule["properties"]["tab_hotkey"] == {"type": "string", "maxLength": 32767}
            assert selection_rule["properties"]["tab_hotkey"] == {"type": "null"}
            notify_rule, = (rule for rule in previews["oneOf"]
                             if rule["properties"]["action"].get("const") == "tab.set_notification_mode")
            assert set(notify_rule["required"]) == set(notification)
            assert notify_rule["properties"]["notification_mode"] == {"type": "integer", "enum": [0, 1, 2]}
            assert hotkey_rule["properties"]["notification_mode"] == {"type": "null"}
            color_rule, = (rule for rule in previews["oneOf"]
                           if rule["properties"]["action"].get("const") == "tab.set_notification_colors")
            assert set(color_rule["required"]) == {"action", "source_tab_ref"}
            assert [rule["required"] for rule in color_rule["anyOf"]] == [
                ["notification_font_color"], ["notification_background_color"]]
            assert notify_rule["properties"]["notification_font_color"] == {"type": "null"}
            note_rule, = (rule for rule in previews["oneOf"]
                          if rule["properties"]["action"].get("const") == "mapping.update")
            assert note_rule["properties"]["note"] == {"type": "string", "maxLength": 32767}
            assert {"required": ["note"]} in note_rule["anyOf"]
            assert {"required": ["disabled"]} in note_rule["dependentSchemas"]["note"]["not"]["anyOf"]
            assert rename_rule["properties"]["note"] == {"type": "null"}
            assert note_rule["properties"]["category"] == {"type": "string", "maxLength": 32767}
            assert {"required": ["category"]} in note_rule["anyOf"]
            assert {"required": ["note"]} in note_rule["dependentSchemas"]["category"]["not"]["anyOf"]
            assert rename_rule["properties"]["category"] == {"type": "null"}
            assert note_rule["oneOf"] == [{"required": ["source_ref"]}, {"required": ["source_refs"]}]
            assert note_rule["dependentSchemas"]["source_refs"]["oneOf"] == [
                {"required": ["category"]},
                {"required": ["row_modes"], "properties": {"row_modes": {
                    "required": ["original_trigger"], "properties": {
                        "original_trigger": {"not": {"required": ["duration_ms"]}}}}}}]
            assert note_rule["properties"]["source_refs"]["minItems"] == 2
            assert note_rule["properties"]["source_refs"]["uniqueItems"] is True
            assert note_rule["properties"]["row_modes"] == {
                "type": "object", "additionalProperties": False, "minProperties": 1,
                "dependentSchemas": {"original_trigger": {"maxProperties": 1}},
                "properties": {"burst": {"type": "boolean"}, "lock": {"type": "boolean"},
                               "burst_press_ms": {"type": "integer", "minimum": 1, "maximum": 99999},
                               "burst_release_ms": {"type": "integer", "minimum": 1, "maximum": 99999},
                               "mapping_key_unlock": {"type": "boolean"},
                               "disable_original_key_unlock": {"type": "boolean"},
                               "sequence_hold_down": {"type": "boolean"},
                               "pass_through": {"type": "boolean"},
                               "check_combo_order": {"type": "boolean"},
                               "unbreakable": {"type": "boolean"},
                               "disable_fn_key_switch": {"type": "boolean"},
                               "fixed_vkey_code": {"type": "integer", "minimum": 0, "maximum": 255},
                               "paste_text_mode": {"type": "integer", "minimum": 0, "maximum": 1},
                               "repeat_mode": {"type": "integer", "minimum": 0, "maximum": 2},
                               "repeat_times": {"type": "integer", "minimum": 0, "maximum": 99999999},
                               "original_trigger": {"type": "object", "additionalProperties": False,
                                   "properties": {"type": {"enum": ["normal", "long_press", "double_press"]},
                                                  "duration_ms": {"type": "integer", "minimum": 1, "maximum": 9999}},
                                   "required": ["type"], "if": {"properties": {"type": {"const": "normal"}}},
                                   "then": {"not": {"required": ["duration_ms"]}}}}}
            assert note_rule["properties"]["send_timing"] == {"type": "integer", "minimum": 0, "maximum": 4}
            assert {"required": ["send_timing"]} in note_rule["anyOf"]
            assert {"required": ["key_up_mapping_key"]} in note_rule["dependentSchemas"]["send_timing"]["not"]["anyOf"]
            assert {"required": ["row_modes"]} in note_rule["anyOf"]
            assert {"required": ["category"]} in note_rule["dependentSchemas"]["row_modes"]["not"]["anyOf"]
            duplicate_rule, = (rule for rule in previews["oneOf"]
                               if rule["properties"]["action"].get("const") == "mapping.duplicate")
            assert duplicate_rule["required"] == ["action", "copy_to"]
            assert duplicate_rule["oneOf"] == [{"required": ["source_ref"]}, {"required": ["source_refs"]}]
            assert duplicate_rule["properties"]["source_refs"]["minItems"] == 2
            assert duplicate_rule["properties"]["source_refs"]["uniqueItems"] is True
            assert previews["properties"]["copy_to"] == duplicate_rule["properties"]["copy_to"]
            assert previews["dependentSchemas"]["copy_to"] == {
                "required": ["action"], "properties": {"action": {"const": "mapping.duplicate"}}}
            copy_destination = duplicate_rule["properties"]["copy_to"]
            assert copy_destination["properties"]["position"]["enum"] == ["top", "bottom", "above", "below"]
            assert copy_destination["then"]["required"] == ["anchor_ref"]
            assert copy_destination["properties"]["target_tab_ref"] == previews["properties"]["target_tab_ref"]["anyOf"][0]
            assert copy_destination["else"]["not"]["required"] == ["anchor_ref"]
            filter_rule, = (rule for rule in previews["oneOf"]
                            if rule["properties"]["action"].get("const") == "tab.set_category_filter")
            assert set(filter_rule["required"]) == set(category_filter)
            assert filter_rule["properties"]["category_filter"]["oneOf"] == [
                {"const": "all"}, {"type": "array", "maxItems": 100, "uniqueItems": True,
                                   "items": {"type": "string", "maxLength": 32767}}]
            assert note_rule["properties"]["category_filter"] == {"type": "null"}
            visibility_rule, = (rule for rule in previews["oneOf"]
                                if rule["properties"]["action"].get("const") == "configuration.set_show_disabled_rows")
            assert set(visibility_rule["required"]) == set(disabled_visibility)
            assert visibility_rule["properties"]["show_disabled_rows"] == {"type": "boolean"}
            display_rule, = (rule for rule in previews["oneOf"]
                             if rule["properties"]["action"].get("const") == "configuration.set_mapping_display")
            assert set(display_rule["required"]) == set(display)
            assert display_rule["properties"]["mapping_display"] == {
                "type": "object", "additionalProperties": False, "minProperties": 1,
                "properties": {field: {"type": "boolean"} for field in ("category_column", "notes", "floating_column")}}
            common_rule, = (rule for rule in previews["oneOf"]
                            if rule["properties"]["action"].get("const") == "tab.set_common_inheritance")
            assert set(common_rule["required"]) == set(common_inheritance)
            assert common_rule["properties"]["include_common"] == {"type": "boolean"}
            collapse_rule, = (rule for rule in previews["oneOf"]
                              if rule["properties"]["action"].get("const") == "tab.set_common_collapsed")
            assert set(collapse_rule["required"]) == set(common_collapsed)
            assert collapse_rule["properties"]["common_collapsed"] == {"type": "boolean"}
            feature_rule, = (rule for rule in previews["oneOf"]
                              if rule["properties"]["action"].get("const") == "configuration.set_common_enabled")
            assert set(feature_rule["required"]) == set(common_feature)
            assert feature_rule["properties"]["common_feature_enabled"] == {"type": "boolean"}
            assert collapse_rule["properties"]["common_feature_enabled"] == {"type": "null"}
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
                    {"unavailable_policy": "fallback"}, {"tab_name": "Tab"})),
                ("qkm_preview_changes", move, (
                    {"action": "mapping.move_up"}, {**move, "source_ref": None},
                    {**move, "source_ref": tab}, {**move, "insert_row": 0},
                    {**move, "send_method": 0}, {**move, "source_tab_ref": tab})),
                ("qkm_preview_changes", down, (
                    {"action": "mapping.move_down"}, {**down, "source_ref": window},
                    {**down, "source_refs": [row]}, {**down, "disabled": False})),
                ("qkm_preview_changes", top, (
                    {"action": "mapping.move_to_top"}, {**top, "insert_row": 0}, {**top, "source_ref": tab})),
                ("qkm_preview_changes", bottom, (
                    {"action": "mapping.move_to_bottom"}, {**bottom, "source_revision": revision},
                    {**bottom, "source_ref": row + ":0"})),
                ("qkm_preview_changes", batch, (
                    {**batch, "source_ref": row}, {**batch, "source_refs": [row]},
                    {**batch, "source_refs": [row, tab]},
                    {**batch, "source_refs": [revision + f":row:0:{i}" for i in range(101)]})),
                ("qkm_preview_changes", {**batch, "action": "mapping.move_up"}, (
                    {"action": "mapping.move_up", "source_refs": []},
                    {"action": "mapping.move_up", "source_ref": row, "source_refs": refs})),
                ("qkm_preview_changes", {**batch, "action": "mapping.move_down"}, (
                    {"action": "mapping.move_down", "source_refs": [row]},
                    {"action": "mapping.move_down", "source_refs": [row, window]})),
                ("qkm_preview_changes", rename, (
                    {"action": "tab.rename", "source_tab_ref": tab}, {**rename, "source_tab_ref": row},
                    {**rename, "source_ref": row}, {**rename, "tab_name": "two\nlines"},
                    {**rename, "tab_name": "\u0000"}, {**rename, "tab_name": "A" * 32768})),
                ("qkm_preview_changes", add_tab, (
                    {"action": "tab.add_blank"}, {**add_tab, "source_tab_ref": row},
                    {**add_tab, "tab_name": "Custom name"}, {**add_tab, "activate_new_tab": False})),
                ("qkm_preview_changes", copy_tab, (
                    {"action": "tab.copy"}, {**copy_tab, "source_tab_ref": row},
                    {**copy_tab, "tab_name": "Custom name"}, {**copy_tab, "activate_new_tab": True})),
                ("qkm_preview_changes", delete_tab, (
                    {"action": "tab.delete"}, {**delete_tab, "source_tab_ref": row},
                    {**delete_tab, "source_refs": [row]}, {**delete_tab, "confirm_delete": True})),
                ("qkm_preview_changes", clear_tab, (
                    {"action": "tab.clear"}, {**clear_tab, "source_tab_ref": row},
                    {**clear_tab, "source_refs": [row]}, {**clear_tab, "confirm_delete": True})),
                ("qkm_preview_changes", move_tab, (
                    {"action": "tab.move", "source_tab_ref": tab}, {**move_tab, "target_tab_ref": tab},
                    {**move_tab, "target_tab_ref": row}, {**move_tab, "target_tab_ref": "b" * 64 + ":source:2"})),
                ("qkm_preview_changes", hotkey, (
                    {"action": "tab.set_hotkey", "source_tab_ref": tab},
                    {**hotkey, "tab_hotkey": 9}, {**hotkey, "tab_hotkey": "A\nB"}, {**hotkey, "tab_name": "Tab3"})),
                ("qkm_preview_changes", {**hotkey, "tab_hotkey": ""}, ()),
                ("qkm_preview_changes", notification, (
                    {"action": "tab.set_notification_mode", "source_tab_ref": tab},
                    {**notification, "notification_mode": -1}, {**notification, "notification_mode": 3},
                    {**notification, "notification_mode": 1.5}, {**notification, "notification_mode": False},
                    {**notification, "notification_mode": "1"}, {**notification, "tab_hotkey": "F8"})),
                ("qkm_preview_changes", {**notification, "notification_mode": 1}, ()),
                ("qkm_preview_changes", {**notification, "notification_mode": 2}, ()),
                ("qkm_preview_changes", note, (
                    {**note, "note": 1}, {**note, "note": "first\nsecond"}, {**note, "note": "x" * 32768},
                    {**note, "note": "bad\0value"}, {**note, "disabled": True}, {**note, "send_method": 0},
                    {**note, "action": "mapping.move_up"})),
                ("qkm_preview_changes", {**note, "note": ""}, ()),
                ("qkm_preview_changes", category, (
                    {**category, "category": None}, {**category, "category": 1},
                    {**category, "category": "bad\rtext"}, {**category, "category": "😀" * 16384},
                    {**category, "note": ""}, {**category, "send_method": 0},
                    {**category, "action": "mapping.move_up"})),
                ("qkm_preview_changes", {**category, "category": ""}, ()),
                ("qkm_preview_changes", row_modes, (
                    {**row_modes, "row_modes": None}, {**row_modes, "row_modes": {}},
                    {**row_modes, "row_modes": {"burst": 1}}, {**row_modes, "row_modes": {"float": True}},
                    {**row_modes, "send_method": 0}, {**row_modes, "note": ""},
                    {**row_modes, "source_ref": None, "source_refs": [row, other_row]})),
                ("qkm_preview_changes", {**row_modes, "row_modes": {"lock": True}}, ()),
                ("qkm_preview_changes", send_timing, (
                    {**send_timing, "send_timing": -1}, {**send_timing, "send_timing": 5},
                    {**send_timing, "send_timing": True}, {**send_timing, "send_timing": None},
                    {**send_timing, "send_timing": 2.5}, {**send_timing, "key_up_mapping_key": "B"},
                    {**send_timing, "row_modes": {"lock": True}})),
                ("qkm_preview_changes", {**send_timing, "send_timing": 0}, ()),
                ("qkm_preview_changes", {**row_modes, "row_modes": {"lock": True, "mapping_key_unlock": True,
                                                                    "disable_original_key_unlock": False}}, (
                    {**row_modes, "row_modes": {"mapping_key_unlock": 1}},
                    {**row_modes, "row_modes": {"disable_original_key_unlock": "false"}},
                    {**row_modes, "row_modes": {"disable_original_key_unlock": None}})),
                ("qkm_preview_changes", {**row_modes, "row_modes": {"original_trigger": {"type": "double_press",
                                                                                         "duration_ms": 9999}}}, (
                    {**row_modes, "row_modes": {"original_trigger": {"type": "normal", "duration_ms": 5}}},
                    {**row_modes, "row_modes": {"original_trigger": {"type": "long_press", "duration_ms": 0}}},
                    {**row_modes, "row_modes": {"original_trigger": {"type": "long_press", "duration_ms": 10000}}},
                    {**row_modes, "row_modes": {"original_trigger": {"type": "long_press", "duration_ms": None}}},
                    {**row_modes, "row_modes": {"original_trigger": {"type": "other"}}},
                    {**row_modes, "row_modes": {"original_trigger": {"type": "normal", "extra": 1}}},
                    {**row_modes, "row_modes": {"original_trigger": {"type": "normal"}, "lock": True}})),
                ("qkm_preview_changes", {"action": "mapping.duplicate", "source_ref": row,
                                         "copy_to": {"position": "above", "anchor_ref": row}}, (
                    {"action": "mapping.duplicate", "source_ref": row, "copy_to": {"position": "below"}},
                    {"action": "mapping.duplicate", "source_ref": row, "copy_to": {"position": "top", "anchor_ref": row}},
                    {"action": "mapping.duplicate", "source_ref": row, "copy_to": {"position": "above",
                                                                                 "anchor_ref": row.rsplit(":", 2)[0]+":8:0"}},
                    {"action": "mapping.add", "source_ref": row, "copy_to": {"position": "top"}},
                    {"action": "mapping.duplicate", "source_ref": row, "copy_to": {"position": "bottom", "extra": True}})),
                ("qkm_preview_changes", {"action": "mapping.duplicate", "source_ref": row,
                                         "copy_to": {"position": "bottom"}}, ()),
                ("qkm_preview_changes", {"action": "mapping.duplicate", "source_ref": row,
                    "copy_to": {"position": "below", "target_tab_ref": row.split(":")[0]+":source:8",
                                "anchor_ref": row.rsplit(":", 2)[0]+":8:0"}}, (
                    {"action": "mapping.duplicate", "source_ref": row,
                     "copy_to": {"position": "below", "target_tab_ref": row.split(":")[0]+":source:8", "anchor_ref": row}},
                    {"action": "mapping.duplicate", "source_ref": row,
                     "copy_to": {"position": "bottom", "target_tab_ref": "f"*64+":source:8"}},
                    {"action": "mapping.duplicate", "source_ref": row,
                     "copy_to": {"position": "top", "target_tab_ref": None}})),
                ("qkm_preview_changes", {"action": "mapping.duplicate", "source_refs": [row, row.rsplit(":", 1)[0]+":3"],
                                         "copy_to": {"position": "below", "anchor_ref": row}}, (
                    {"action": "mapping.duplicate", "source_refs": [row, row], "copy_to": {"position": "bottom"}},
                    {"action": "mapping.duplicate", "source_refs": [row], "copy_to": {"position": "top"}},
                    {"action": "mapping.duplicate", "source_refs": [row, row.rsplit(":", 2)[0]+":8:0"],
                     "copy_to": {"position": "bottom"}},
                    {"action": "mapping.duplicate", "source_ref": row, "source_refs": [row, row.rsplit(":", 1)[0]+":3"],
                     "copy_to": {"position": "bottom"}})),
                ("qkm_preview_changes", {"action": "mapping.update", "source_refs": [row, row.rsplit(":", 1)[0]+":3"],
                                         "row_modes": {"original_trigger": {"type": "double_press"}}}, (
                    {"action": "mapping.update", "source_refs": [row, row],
                     "row_modes": {"original_trigger": {"type": "double_press"}}},
                    {"action": "mapping.update", "source_refs": [row],
                     "row_modes": {"original_trigger": {"type": "double_press"}}},
                    {"action": "mapping.update", "source_refs": [row, row.rsplit(":", 1)[0]+":3"],
                     "row_modes": {"original_trigger": {"type": "double_press", "duration_ms": 500}}},
                    {"action": "mapping.update", "source_refs": [row, row.rsplit(":", 1)[0]+":3"],
                     "row_modes": {"lock": True}})),
                ("qkm_preview_changes", {**row_modes, "row_modes": {"original_trigger": {"type": "normal"}}}, ()),
                ("qkm_preview_changes", {**row_modes, "row_modes": {"original_trigger": {"type": "long_press"}}}, ()),
                ("qkm_preview_changes", {**row_modes, "row_modes": {"fixed_vkey_code": 255,
                                                                    "paste_text_mode": 0, "disable_fn_key_switch": True}}, (
                    {**row_modes, "row_modes": {"fixed_vkey_code": 256}},
                    {**row_modes, "row_modes": {"fixed_vkey_code": -1}},
                    {**row_modes, "row_modes": {"paste_text_mode": 2}},
                    {**row_modes, "row_modes": {"paste_text_mode": 0.5}},
                    {**row_modes, "row_modes": {"disable_fn_key_switch": 1}})),
                ("qkm_preview_changes", {**row_modes, "row_modes": {"pass_through": True,
                                                                    "check_combo_order": False, "unbreakable": True}}, (
                    {**row_modes, "row_modes": {"pass_through": 1}},
                    {**row_modes, "row_modes": {"check_combo_order": "false"}},
                    {**row_modes, "row_modes": {"unbreakable": None}})),
                ("qkm_preview_changes", {**row_modes, "row_modes": {"sequence_hold_down": False,
                                                                    "repeat_mode": 2, "repeat_times": 0}}, (
                    {**row_modes, "row_modes": {"sequence_hold_down": 0}},
                    {**row_modes, "row_modes": {"repeat_mode": 3}},
                    {**row_modes, "row_modes": {"repeat_times": -1}},
                    {**row_modes, "row_modes": {"repeat_times": 1.25}},
                    {**row_modes, "row_modes": {"repeat_times": 100000000}},
                    {**row_modes, "row_modes": {"repeat_mode": None}})),
                ("qkm_preview_changes", {**row_modes, "row_modes": {"burst_press_ms": 1, "burst_release_ms": 99999}}, (
                    {**row_modes, "row_modes": {"burst_press_ms": 0}},
                    {**row_modes, "row_modes": {"burst_release_ms": 100000}},
                    {**row_modes, "row_modes": {"burst_press_ms": 30.5}},
                    {**row_modes, "row_modes": {"burst_release_ms": "20"}},
                    {**row_modes, "row_modes": {"burst_press_ms": True}})),
                ("qkm_preview_changes", category_filter, (
                    {**category_filter, "category_filter": None}, {**category_filter, "category_filter": "none"},
                    {**category_filter, "category_filter": ["", ""]}, {**category_filter, "category_filter": [False]},
                    {**category_filter, "category_filter": ["two\nlines"]},
                    {**category_filter, "category_filter": [str(i) for i in range(101)]},
                    {**category_filter, "source_tab_ref": row}, {**category_filter, "category": ""})),
                ("qkm_preview_changes", {**category_filter, "category_filter": "all"}, ()),
                ("qkm_preview_changes", {**category_filter, "category_filter": []}, ()),
                ("qkm_preview_changes", disabled_visibility, (
                    {**disabled_visibility, "show_disabled_rows": None},
                    {**disabled_visibility, "show_disabled_rows": 1},
                    {**disabled_visibility, "show_disabled_rows": "false"},
                    {**disabled_visibility, "category_filter": []})),
                ("qkm_preview_changes", {**disabled_visibility, "show_disabled_rows": False}, ()),
                ("qkm_preview_changes", display, (
                    {**display, "mapping_display": {}}, {**display, "mapping_display": {"notes": None}},
                    {**display, "mapping_display": {"category_column": 0}}, {**display, "mapping_display": {"unknown": True}},
                    {**display, "mapping_display": [True]}, {**display, "show_disabled_rows": True})),
                ("qkm_preview_changes", {**display, "mapping_display": {"floating_column": True}}, ()),
                ("qkm_preview_changes", category_batch, (
                    {**category_batch, "source_ref": row}, {**category_batch, "source_refs": [row]},
                    {**category_batch, "source_refs": [row, row]},
                    {**category_batch, "source_refs": [row, "a" * 64 + ":row:2:0"]},
                    {**category_batch, "source_refs": [row, "b" * 64 + ":row:0:3"]},
                    {**category_batch, "category": None}, {**category_batch, "note": ""},
                    {"action": "mapping.update", "source_refs": [row, other_row], "send_method": 0})),
                ("qkm_preview_changes", colors, (
                    {"action": "tab.set_notification_colors", "source_tab_ref": tab},
                    {**colors, "notification_background_color": "#405060"},
                    {**colors, "notification_font_color": "#ff123456"},
                    {**colors, "notification_font_color": 123}, {**colors, "notification_mode": 0})),
                ("qkm_preview_changes", {"action": colors["action"], "source_tab_ref": tab,
                                         "notification_font_color": "default"}, ()),
                ("qkm_preview_changes", select_tab, (
                    {"action": "tab.select", "source_tab_ref": tab}, {**select_tab, "target_tab_ref": row},
                    {**select_tab, "target_tab_ref": "b" * 64 + ":source:2"})),
                ("qkm_preview_changes", {**select_tab, "target_tab_ref": tab}, ()),
                ("qkm_preview_changes", common_inheritance, (
                    {"action": "tab.set_common_inheritance", "source_tab_ref": tab},
                    {**common_inheritance, "include_common": 1}, {**common_inheritance, "source_tab_ref": row})),
                ("qkm_preview_changes", common_collapsed, (
                    {"action": "tab.set_common_collapsed", "source_tab_ref": tab},
                    {**common_collapsed, "common_collapsed": "true"}, {**common_collapsed, "include_common": True})),
                ("qkm_preview_changes", common_feature, (
                    {"action": "configuration.set_common_enabled", "source_tab_ref": tab},
                    {**common_feature, "common_feature_enabled": 0}, {**common_feature, "source_tab_ref": row},
                    {**common_feature, "confirm_conflicts": True})),
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
