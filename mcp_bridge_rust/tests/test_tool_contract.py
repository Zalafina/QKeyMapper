"""Check the committed MCP contract and independent wire expectations via stdio."""

import argparse
import asyncio
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from jsonschema import Draft202012Validator
from mcp import Client, StdioServerParameters

HERE = Path(__file__).resolve()
ROOT = HERE.parents[2]
CATALOG = HERE.parents[1] / "src/tool_catalog.json"
DEMO_STATE = "qkm_get_demo_state"
DEMO_MAPPING = "qkm_demo_mapping"
EXPORT = "qkm_export_macros"
EXPORT_TABLE = "qkm_export_mapping_table"
EXPORT_CONFIGURATIONS = "qkm_export_configurations"
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
    assert {tool["name"] for tool in catalog["tools"]} == TOOLS | {
        DEMO_STATE, DEMO_MAPPING, EXPORT, EXPORT_TABLE, EXPORT_CONFIGURATIONS}
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
            query = next(tool.input_schema for tool in tools if tool.name == "qkm_query")
            assert set(query["properties"]["entity"]["enum"]) == {
                "configurations", "tabs", "mapping_sources", "mappings", "windows", "demo_context", "macros", "ignore_rules"}
            expect_not_connected(await client.call_tool(
                "qkm_query", {"entity": "macros", "offset": 1, "catalog_revision": "a" * 64}))
            expect_not_connected(await client.call_tool(
                "qkm_query", {"entity": "ignore_rules", "offset": 1, "catalog_revision": "a" * 64}))
            previews = next(tool.input_schema for tool in tools if tool.name == "qkm_preview_changes")
            macro_rule, = (rule for rule in previews["oneOf"]
                           if rule["properties"]["action"].get("const") == "macro.upsert")
            assert macro_rule["required"] == ["action", "macro"]
            assert macro_rule["propertyNames"] == {"enum": ["action", "macro"]}
            macro_fields = previews["properties"]["macro"]
            assert not macro_fields["additionalProperties"]
            assert macro_fields["required"] == ["catalog_revision", "scope"]
            assert macro_rule["properties"]["macro"] == {"required": ["name", "content"],
                "propertyNames": {"enum": ["catalog_revision", "scope", "name", "content", "category", "note",
                                           "insert_at", "segment_comments"]}}
            delete_macro_rule, = (rule for rule in previews["oneOf"]
                                  if rule["properties"]["action"].get("const") == "macro.delete")
            delete_names = macro_fields["properties"]["names"]
            assert delete_names == {"type": "array", "minItems": 1, "maxItems": 100, "uniqueItems": True,
                                    "items": {"type": "string", "minLength": 1, "maxLength": 32767}}
            assert delete_macro_rule["properties"]["macro"] == {"required": ["names"],
                "propertyNames": {"enum": ["catalog_revision", "scope", "names"]}}
            import_rule, = (rule for rule in previews["oneOf"]
                            if rule["properties"]["action"].get("const") == "macro.import")
            assert import_rule["required"] == ["action", "macro"]
            assert import_rule["properties"]["macro"] == {"required": ["file_name", "on_conflict"],
                "propertyNames": {"enum": ["catalog_revision", "scope", "file_name", "on_conflict"]}}
            assert macro_fields["properties"]["on_conflict"] == {"enum": ["keep", "overwrite"]}
            import_args = {"catalog_revision": revision, "scope": "universal",
                           "file_name": "宏 示例.ini", "on_conflict": "keep"}
            import_request = {"action": "macro.import", "macro": import_args}
            unavailable = await client.call_tool("qkm_preview_changes", import_request)
            assert unavailable.is_error and unavailable.structured_content["error"] == "backup_directory_unavailable"
            table_import = {"action": "table.import", "source_tab_ref": tab, "file_name": "表 格.ini"}
            unavailable = await client.call_tool("qkm_preview_changes", table_import)
            assert unavailable.is_error and unavailable.structured_content["error"] == "backup_directory_unavailable"
            removal = {"catalog_revision": "c" * 64, "scope": "universal", "names": ["Shared", "Other"]}
            remove = {"action": "macro.delete", "macro": removal}
            expect_not_connected(await client.call_tool("qkm_preview_changes", remove))
            for invalid in (removal | {"names": []}, removal | {"names": ["X", "X"]},
                            removal | {"names": ["M"] * 101}, removal | {"content": "A"}):
                denied = await client.call_tool("qkm_preview_changes", remove | {"macro": invalid})
                assert denied.is_error is True and denied.structured_content is None
            assert macro_fields["properties"]["scope"]["enum"] == ["configuration", "universal"]
            macro = {"catalog_revision": "b" * 64, "scope": "configuration", "name": "New", "content": "A»B"}
            upsert = {"action": "macro.upsert", "macro": macro}
            validator = Draft202012Validator(previews)
            for request in (upsert, remove, import_request, table_import):
                validator.validate(request)
            configuration_import = {"action": "configuration.import", "catalog_revision": revision,
                                    "groups": ["合成配置", "Other"], "file_name": "配置.ini", "on_conflict": "keep"}
            validator.validate(configuration_import)
            unavailable = await client.call_tool("qkm_preview_changes", configuration_import)
            assert unavailable.is_error and unavailable.structured_content["error"] == "backup_directory_unavailable"
            save_import = {"action": "configuration.save_import", "import_draft_id": "a" * 32}
            validator.validate(save_import)
            expect_not_connected(await client.call_tool("qkm_preview_changes", save_import))
            for invalid in (configuration_import | {"groups": []}, configuration_import | {"groups": ["D", "D"]},
                            configuration_import | {"on_conflict": "replace"}, configuration_import | {"document_base64": "QQ=="},
                            configuration_import | {"file_name": "../config.ini"}, save_import | {"import_draft_id": "a" * 31},
                            save_import | {"groups": ["Other"]}):
                assert not validator.is_valid(invalid)
                denied = await client.call_tool("qkm_preview_changes", invalid)
                assert denied.is_error is True and denied.structured_content is None
            move_configuration = {"action": "configuration.move", "catalog_revision": revision,
                                  "group": "合成配置", "target_group": "Other"}
            reset_order = {"action": "configuration.reset_order", "catalog_revision": revision}
            remove_configuration = {"action": "configuration.remove", "catalog_revision": revision,
                                    "group": "Other"}
            load_configuration = {"action": "configuration.load", "catalog_revision": revision,
                                  "group": "Other"}
            load_global = {"action": "configuration.load", "catalog_revision": revision,
                           "group": "QKeyMapperGlobalSetting"}
            load_empty = {"action": "configuration.load_empty", "catalog_revision": revision}
            for request in (move_configuration, reset_order, remove_configuration, load_configuration, load_global, load_empty):
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for invalid in (move_configuration | {"target_group": None},
                            move_configuration | {"group": "bad\nname"},
                            move_configuration | {"source_tab_ref": tab},
                            reset_order | {"group": "Other"}, reset_order | {"catalog_revision": "old"},
                            remove_configuration | {"target_group": "Extra"}, remove_configuration | {"group": ""},
                            load_configuration | {"file_name": "other.ini"}, load_configuration | {"group": None},
                            load_configuration | {"group": ""},
                            load_empty | {"group": "Other"}, load_empty | {"catalog_revision": "old"}):
                assert not validator.is_valid(invalid)
                denied = await client.call_tool("qkm_preview_changes", invalid)
                assert denied.is_error is True and denied.structured_content is None
            for extra in ({"document_base64": "QUJD"}, {"file_name": "../escape.ini"}, {"overwrite": True}):
                assert not validator.is_valid(table_import | extra)
            for action_name, field in (("general.set_save_window_size", "save_window_size"),
                                       ("general.set_save_splitter_position", "save_splitter_position"),
                                       ("general.set_sound_effect", "play_sound_effect"),
                                       ("configuration.set_process_icon_as_tray_icon", "process_icon_as_tray_icon"),
                                       ("configuration.set_auto_match_foreground", "auto_match_foreground"),
                                       ("configuration.set_send_to_same_windows", "send_to_same_windows")):
                option = {"action": action_name, "source_tab_ref": tab, field: True}
                for value in (True, False):
                    request = option | {field: value}
                    validator.validate(request)
                    expect_not_connected(await client.call_tool("qkm_preview_changes", request))
                for extra in ({field: 1}, {field: "true"}, {field: None}, {"window_display_mode": 1}):
                    assert not validator.is_valid(option | extra)
                    denied = await client.call_tool("qkm_preview_changes", option | extra)
                    assert denied.is_error is True and denied.structured_content is None
            for section, field, count in (("general", "table_insert_mode", 2), ("general", "table_edit_trigger", 2),
                                          ("configuration", "window_display_mode", 3)):
                request = {"action": f"{section}.set_{field}", "source_tab_ref": tab, field: 0}
                for mode_value in range(count):
                    args = request | {field: mode_value}
                    validator.validate(args)
                    expect_not_connected(await client.call_tool("qkm_preview_changes", args))
                for fields in ({field: count}, {field: None}, {field: True}, {field: -1}, {"mapping_key": "A"}):
                    args = request | fields
                    assert not validator.is_valid(args)
                    denied = await client.call_tool("qkm_preview_changes", args)
                    assert denied.is_error is True and denied.structured_content is None
                assert not validator.is_valid(move | {field: 1})
            switch_timeout = {"action": "general.set_global_switch_timeout", "source_tab_ref": tab}
            for ms in (0, 2000, 999_999_999):
                args = switch_timeout | {"timeout_ms": ms}
                validator.validate(args)
                expect_not_connected(await client.call_tool("qkm_preview_changes", args))
            for bad in ({"timeout_ms": -1}, {"timeout_ms": 1_000_000_000}, {"timeout_ms": 1.5},
                        {"timeout_ms": "2000"}, {"timeout_ms": True}, {"extra": 1}):
                args = switch_timeout | bad
                assert not validator.is_valid(args)
                denied = await client.call_tool("qkm_preview_changes", args)
                assert denied.is_error is True and denied.structured_content is None
            fullscreen = {"action": "general.set_fullscreen_global_mapping", "source_tab_ref": tab}
            for fields in ({"disable_in_fullscreen": True}, {"disable_in_fullscreen": False},
                           {"allowed_processes": ""}, {"allowed_processes": "demo.exe;notepad.exe"},
                           {"disable_in_fullscreen": True, "allowed_processes": "demo.exe"}):
                request = fullscreen | {"fullscreen_global_mapping": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for fields in ({}, {"disable_in_fullscreen": False, "allowed_processes": "demo.exe"},
                           {"disable_in_fullscreen": 1}, {"disable_in_fullscreen": None},
                           {"allowed_processes": None}, {"allowed_processes": 123},
                           {"allowed_processes": "a\nb"}, {"allowed_processes": "a\rb"},
                           {"allowed_processes": "a\0b"}, {"allowed_processes": "a" * 32768},
                           {"disable_in_fullscreen": True, "extra": 1}):
                request = fullscreen | {"fullscreen_global_mapping": fields}
                assert not validator.is_valid(request), fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            key2mouse = {"action": "configuration.set_key2mouse_settings", "source_tab_ref": tab}
            for fields in ({"x_speed": 1}, {"y_speed": 99}, {"polling_interval_ms": 1},
                           {"x_speed": 15, "y_speed": 25, "polling_interval_ms": 5}):
                request = key2mouse | {"key2mouse": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for fields in ({}, {"x_speed": 0}, {"x_speed": 100}, {"y_speed": 0}, {"y_speed": 100},
                           {"polling_interval_ms": 0}, {"polling_interval_ms": 100},
                           {"x_speed": 1.5}, {"y_speed": True}, {"polling_interval_ms": None},
                           {"x_speed": 3, "extra": 1}):
                request = key2mouse | {"key2mouse": fields}
                assert not validator.is_valid(request), fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            gyro_opts = {"action": "configuration.set_gyro2mouse_options", "source_tab_ref": tab}
            for fields in ({"mouse_x_source": 0}, {"mouse_y_source": 2}, {"mouse_x_revert": True},
                           {"mouse_y_revert": False},
                           {"mouse_x_source": 0, "mouse_y_source": 2, "mouse_x_revert": True, "mouse_y_revert": False}):
                request = gyro_opts | {"gyro2mouse_options": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for fields in ({}, {"mouse_x_source": -1}, {"mouse_x_source": 3}, {"mouse_y_source": -1},
                           {"mouse_y_source": 3}, {"mouse_x_source": 1.5}, {"mouse_y_source": True},
                           {"mouse_x_source": None}, {"mouse_x_revert": 1}, {"mouse_y_revert": "true"},
                           {"mouse_y_revert": None}, {"mouse_x_source": 1, "extra": 1}):
                request = gyro_opts | {"gyro2mouse_options": fields}
                assert not validator.is_valid(request), fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            gamepad_thresh = {"action": "configuration.set_gamepad_thresholds", "source_tab_ref": tab}
            for groups in ({"lt": {"press": 80, "light_press": 50, "release": 20}},
                           {"rt": {"press": 100, "light_press": 100, "release": 100}},
                           {"ls": {"push": 0, "light_push": 0, "release": 0}},
                           {"rs": {"push": 85, "light_push": 45}},
                           {"lt": {"press": 70}, "rs": {"release": 10}}):
                request = gamepad_thresh | {"gamepad_thresholds": groups}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for groups in ({}, {"lt": {}}, {"unknown": {"press": 50}}, {"lt": {"push": 50}},
                           {"ls": {"press": 50}}, {"lt": {"press": -1}}, {"rt": {"press": 101}},
                           {"ls": {"push": 1.5}}, {"rs": {"push": True}}, {"lt": {"press": None}},
                           {"lt": None}, {"lt": {"press": 50}, "extra": {}}):
                request = gamepad_thresh | {"gamepad_thresholds": groups}
                assert not validator.is_valid(request), groups
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            gamepad_touchpad = {"action": "configuration.set_gamepad_touchpad", "source_tab_ref": tab}
            for fields in ({"x_speed": 0},
                           {"y_speed": 200},
                           {"x_speed": 12.35, "y_speed": 0.5, "invert_x": True, "invert_y": False},
                           {"invert_x": False},
                           {"invert_y": True}):
                request = gamepad_touchpad | {"gamepad_touchpad": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for fields in ({}, {"x_speed": -0.01}, {"y_speed": 200.01}, {"x_speed": 1.234},
                           {"x_speed": "0.5"}, {"x_speed": True}, {"x_speed": None},
                           {"invert_x": 1}, {"invert_y": None}, {"invert_x": True, "extra": False}):
                request = gamepad_touchpad | {"gamepad_touchpad": fields}
                if fields in ({}, {"x_speed": -0.01}, {"y_speed": 200.01}, {"x_speed": "0.5"},
                              {"x_speed": True}, {"x_speed": None}, {"invert_x": 1},
                              {"invert_y": None}, {"invert_x": True, "extra": False}):
                    assert not validator.is_valid(request), fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            accept_virtual = {"action": "configuration.set_accept_virtual_gamepad_input", "source_tab_ref": tab}
            for enabled in (True, False):
                request = accept_virtual | {"accept_virtual_gamepad_input": enabled}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for bad in (accept_virtual,
                        accept_virtual | {"accept_virtual_gamepad_input": 1},
                        accept_virtual | {"accept_virtual_gamepad_input": "true"},
                        accept_virtual | {"accept_virtual_gamepad_input": None},
                        accept_virtual | {"accept_virtual_gamepad_input": True, "extra": True}):
                assert not validator.is_valid(bad), bad
                denied = await client.call_tool("qkm_preview_changes", bad)
                assert denied.is_error is True and denied.structured_content is None
            vbutton_panel = {"action": "configuration.set_vbutton_panel_settings", "source_tab_ref": tab}
            for fields in ({"columns": 6, "opacity": 0.65, "default_show": False, "bg_color": "#aa112233", "button_color": "#224466"},
                           {"max_rows": 2, "button_width": 640, "button_height": 4, "margin": 50, "radius": 100,
                            "reference_point": 11, "offset_x": -50000, "offset_y": 50000,
                            "font_size": 72, "font_weight": 2, "font_family": "",
                            "always_on_top": False, "drag_enabled": False,
                            "pressed_color": "#112233", "locked_color": "#445566", "text_color": "#ddeeff"}):
                request = vbutton_panel | {"vbutton_panel": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for bad_fields in ({}, {"columns": 0}, {"columns": 21}, {"max_rows": 1}, {"max_rows": 1000},
                               {"button_width": 3}, {"opacity": 0.09}, {"opacity": 1.01}, {"opacity": 0.555},
                               {"margin": -1}, {"radius": 101}, {"reference_point": 12}, {"offset_x": 50001},
                               {"font_size": 0}, {"font_weight": 3}, {"font_family": "Consolas\nX"},
                               {"bg_color": "#112233"}, {"button_color": "#ff112233"},
                               {"columns": None}, {"columns": 3, "extra": 1}):
                request = vbutton_panel | {"vbutton_panel": bad_fields}
                if "opacity" not in bad_fields or bad_fields["opacity"] != 0.555:
                    assert not validator.is_valid(request), bad_fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            for notify_action in ("general.set_notification_style", "configuration.set_custom_notification_style"):
                notify_base = {"action": notify_action, "source_tab_ref": tab}
                for fields in ({"font_color": "#12AB34", "opacity": 0.65, "font_family": "", "font_italic": True},
                               {"background_color": "#88112233", "font_size": 72, "font_weight": 0,
                                "display_duration": 0, "fade_in_duration": 99999, "fade_out_duration": 500,
                                "border_radius": 1000, "padding": 0, "opacity": 0.0,
                                "x_offset": -5000, "y_offset": 5000}):
                    request = notify_base | {"notification_style": fields}
                    validator.validate(request)
                    expect_not_connected(await client.call_tool("qkm_preview_changes", request))
                for bad_fields in ({}, {"font_color": "#88112233"}, {"background_color": "#112233"},
                                   {"font_size": 0}, {"font_size": 73}, {"font_weight": 3},
                                   {"font_italic": 1}, {"display_duration": -1}, {"fade_in_duration": 100000},
                                   {"border_radius": 1001}, {"padding": -1}, {"opacity": -0.01},
                                   {"opacity": 1.01}, {"opacity": 0.123}, {"x_offset": -5001}, {"y_offset": 5001},
                                   {"font_family": "Consolas\nX"}, {"font_family": "A" * 256},
                                   {"font_size": None}, {"font_size": 12, "extra": 1}):
                    request = notify_base | {"notification_style": bad_fields}
                    if "opacity" not in bad_fields or bad_fields["opacity"] != 0.123:
                        assert not validator.is_valid(request), bad_fields
                    denied = await client.call_tool("qkm_preview_changes", request)
                    assert denied.is_error is True and denied.structured_content is None
            tray_base = {"action": "general.set_tray_icons", "source_tab_ref": tab}
            for fields in ({"idle": ":/Black.ico", "monitoring": "default"},
                           {"global": "custom_trayicons/mcp_custom.ico", "matched": ":/Yellow.ico"}):
                request = tray_base | {"tray_icons": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for bad_fields in ({}, {"idle": ""}, {"monitoring": "a\nb"}, {"global": "a" * 256},
                               {"idle": 123}, {"idle": None}, {"idle": "default", "extra": 1}):
                request = tray_base | {"tray_icons": bad_fields}
                assert not validator.is_valid(request), bad_fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            fwin_base = {"action": "tab.set_floating_window", "source_tab_ref": tab}
            for fields in ({"background_color": "#AA112233", "size": 128, "opacity": 0.65, "mouse_pass_through": True},
                           {"background_color": "default", "size": 16, "radius": 320, "opacity": 0.1,
                            "reference_point": 11, "position_x": -50000, "position_y": 50000,
                            "mouse_pass_through": False, "mouse_pass_through_switch_key": "None"}):
                request = fwin_base | {"floating_window": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for bad_fields in ({}, {"background_color": "#112233"}, {"size": 15}, {"size": 641},
                               {"radius": -1}, {"radius": 321}, {"opacity": 0.09}, {"opacity": 1.01},
                               {"opacity": 0.555}, {"reference_point": 12}, {"position_x": 50001},
                               {"mouse_pass_through": 1}, {"mouse_pass_through_switch_key": ""},
                               {"mouse_pass_through_switch_key": "F3\nX"}, {"size": None},
                               {"size": 64, "extra": 1}):
                request = fwin_base | {"floating_window": bad_fields}
                if "opacity" not in bad_fields or bad_fields["opacity"] != 0.555:
                    assert not validator.is_valid(request), bad_fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            cimg_base = {"action": "tab.set_custom_image", "source_tab_ref": tab}
            for fields in ({"image_path": "icons/tab_01.ico", "show_position": 2, "padding": 12,
                            "show_as_tray_icon": True, "show_as_floating_window": True, "tray_icon_pixel": "32x32"},
                           {"image_path": "", "tray_icon_pixel": "default"}):
                request = cimg_base | {"custom_image": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for bad_fields in ({}, {"image_path": "C:/temp/a.ico"}, {"image_path": "../a.ico"},
                               {"image_path": "/a.ico"}, {"image_path": "icons/a.bmp"},
                               {"show_position": -1}, {"show_position": 3},
                               {"padding": -1}, {"padding": 1001},
                               {"show_as_tray_icon": 1}, {"show_as_floating_window": "true"},
                               {"tray_icon_pixel": "32×32"}, {"tray_icon_pixel": "0x32"},
                               {"padding": None}, {"padding": 4, "extra": 1}):
                request = cimg_base | {"custom_image": bad_fields}
                if "image_path" not in bad_fields:
                    assert not validator.is_valid(request), bad_fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            crosshair_base = {"action": "mapping.set_crosshair", "source_ref": row}
            for fields in ({"center_color": "#AABBCC", "center_size": 500, "center_opacity": 255,
                            "crosshair_color": "default", "crosshair_width": 1, "crosshair_length": 900,
                            "crosshair_opacity": 0, "show_center": False, "show_top": True,
                            "show_bottom": False, "show_left": True, "show_right": False,
                            "x_offset": -5000, "y_offset": 5000},
                           {"center_color": "default", "x_offset": 0}):
                request = crosshair_base | {"crosshair": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for bad_fields in ({}, {"center_color": "#11223344"}, {"crosshair_color": "70a1ff"},
                               {"center_size": 0}, {"center_size": 501}, {"center_opacity": -1}, {"center_opacity": 256},
                               {"crosshair_width": 0}, {"crosshair_width": 501}, {"crosshair_length": 0}, {"crosshair_length": 901},
                               {"crosshair_opacity": 256}, {"show_center": 1}, {"x_offset": -5001}, {"y_offset": 5001},
                               {"x_offset": 1.5}, {"center_size": None}, {"center_size": 2, "extra": 1}):
                request = crosshair_base | {"crosshair": bad_fields}
                assert not validator.is_valid(request), bad_fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            denied = await client.call_tool("qkm_preview_changes", {"action": "mapping.set_crosshair", "source_tab_ref": tab, "crosshair": {"center_size": 5}})
            assert denied.is_error is True and denied.structured_content is None
            fb_base = {"action": "mapping.set_floating_button", "source_ref": row}
            for fields in ({"enable": True, "label": "Attack-01", "show_on_mapping_start": False,
                            "show_tool_tip": True, "sync_pressed_locked_state": False, "always_on_top": False,
                            "mouse_pass_through": True, "drag_to_move": False, "sync_group_id": 999,
                            "button_color": "#80AABBCC", "pressed_color": "#112233", "locked_color": "default",
                            "text_color": "#FFEEDD", "border_color": "#40010203", "border_width": 50,
                            "width": 640, "height": 4, "font_size": 72, "font_weight": 2, "font_family": "Consolas",
                            "radius": 1000, "normal_opacity": 0.01, "pressed_opacity": 0.55, "locked_opacity": 1.0,
                            "enable_gradient_fill": False, "enable_hover_animation": True,
                            "hover_effect_strength": 200, "hover_glow_strength": 0, "hover_contrast_mode": 3,
                            "hover_custom_color": "#CC556677", "hover_animation_duration": 500,
                            "reference_point": 11, "x_offset": -50000, "y_offset": 50000},
                           {"style_code": "FBS1;w=120;h=40", "enable": True}):
                request = fb_base | {"floating_button": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for bad_fields in ({}, {"style_code": "   "}, {"style_code": "FBS1;w=1\n2"},
                               {"label": "bad#label"}, {"label": "bad|label"}, {"label": "bad\nlabel"},
                               {"font_family": "Consolas\nX"}, {"sync_group_id": -1}, {"sync_group_id": 1000},
                               {"button_color": "#12345"}, {"border_width": 51}, {"width": 3}, {"height": 641},
                               {"font_size": 0}, {"font_weight": 3}, {"radius": 1001},
                               {"normal_opacity": 0.0}, {"pressed_opacity": 1.01}, {"locked_opacity": 0.123},
                               {"hover_contrast_mode": 4}, {"hover_animation_duration": 79},
                               {"reference_point": 12}, {"x_offset": -50001}, {"y_offset": 50001},
                               {"x_offset": 1.5}, {"enable": 1}, {"enable": None}, {"enable": True, "extra": 1}):
                request = fb_base | {"floating_button": bad_fields}
                if bad_fields.get("locked_opacity") != 0.123:
                    assert not validator.is_valid(request), bad_fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            denied = await client.call_tool("qkm_preview_changes", {"action": "mapping.set_floating_button", "source_tab_ref": tab, "floating_button": {"enable": True}})
            assert denied.is_error is True and denied.structured_content is None
            drag_coord_req = {"action": "general.set_floating_button_drag_coordinate", "source_tab_ref": tab, "show_drag_coordinate": False}
            validator.validate(drag_coord_req)
            expect_not_connected(await client.call_tool("qkm_preview_changes", drag_coord_req))
            for bad_dc in (
                {"action": "general.set_floating_button_drag_coordinate", "source_tab_ref": tab},
                {"action": "general.set_floating_button_drag_coordinate", "source_tab_ref": tab, "show_drag_coordinate": 1},
                {"action": "general.set_floating_button_drag_coordinate", "source_tab_ref": tab, "show_drag_coordinate": True, "extra": 1},
            ):
                assert not validator.is_valid(bad_dc), bad_dc
                denied = await client.call_tool("qkm_preview_changes", bad_dc)
                assert denied.is_error is True and denied.structured_content is None
            sg_note = {"action": "configuration.set_floating_button_sync_group_note", "source_tab_ref": tab}
            for fields in ({"group_id": 12, "note": "同步组😀"}, {"group_id": 999, "note": ""}):
                request = sg_note | {"sync_group_note": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for bad_fields in ({}, {"group_id": 0, "note": "ok"}, {"group_id": 1000, "note": "ok"},
                               {"group_id": 1, "note": "Note\nLine2"}, {"group_id": 1, "note": "A" * 1025},
                               {"group_id": 1}, {"group_id": 1, "note": "ok", "extra": 1}):
                request = sg_note | {"sync_group_note": bad_fields}
                assert not validator.is_valid(request), bad_fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            sg_move = {"action": "mapping.move_floating_button_sync_group", "source_tab_ref": tab}
            for fields in ({"group_id": 1, "delta_x": -100000, "delta_y": 100000}, {"group_id": 999, "delta_x": 15}, {"group_id": 5, "delta_y": -20}):
                request = sg_move | {"sync_group_move": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for bad_fields in ({}, {"group_id": 0, "delta_x": 1}, {"group_id": 1000, "delta_y": 1},
                               {"group_id": 1}, {"group_id": 1, "delta_x": 0}, {"group_id": 1, "delta_x": 0, "delta_y": 0},
                               {"group_id": 1, "delta_x": 100001}, {"group_id": 1, "delta_y": -100001},
                               {"group_id": 1, "delta_x": 1.5}, {"group_id": 1, "delta_x": None},
                               {"group_id": 1, "delta_x": 1, "extra": 1}):
                request = sg_move | {"sync_group_move": bad_fields}
                if bad_fields not in ({"group_id": 1, "delta_x": 0}, {"group_id": 1, "delta_x": 0, "delta_y": 0}):
                    assert not validator.is_valid(request), bad_fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            ui_prefs = {"action": "general.set_ui_preferences", "source_tab_ref": tab}
            assert validator.is_valid({**ui_prefs, "general_ui_preferences": {
                "language": 1,
                "theme": 2,
                "notification_position": 5,
                "show_process_list": False,
                "update_site": 1,
                "startup_minimized": True,
                "window_switch_key": "L-Ctrl+F7",
            }})
            for bad_prefs in (
                {"general_ui_preferences": {}},
                {"general_ui_preferences": {"language": -1}},
                {"general_ui_preferences": {"language": 3}},
                {"general_ui_preferences": {"language": None}},
                {"general_ui_preferences": {"theme": 3}},
                {"general_ui_preferences": {"notification_position": 7}},
                {"general_ui_preferences": {"show_process_list": "false"}},
                {"general_ui_preferences": {"update_site": 2}},
                {"general_ui_preferences": {"startup_minimized": 1}},
                {"general_ui_preferences": {"window_switch_key": ""}},
                {"general_ui_preferences": {"window_switch_key": "   "}},
                {"general_ui_preferences": {"window_switch_key": "L-Ctrl+A\nB"}},
                {"general_ui_preferences": {"window_switch_key": "A" * 257}},
                {"general_ui_preferences": {"language": 1, "extra": 1}},
                {"general_ui_preferences": {"language": 1}, "extra": 1},
            ):
                request = {**ui_prefs, **bad_prefs}
                assert not validator.is_valid(request), bad_prefs
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            behavior = {"action": "general.set_behavior", "source_tab_ref": tab}
            assert validator.is_valid({**behavior, "general_behavior": {
                "startup_auto_monitoring": True,
                "close_to_system_tray": False,
                "mapping_start_action_mode": 1,
            }})
            for bad_behavior in (
                {"general_behavior": {}},
                {"general_behavior": {"startup_auto_monitoring": 1}},
                {"general_behavior": {"startup_auto_monitoring": None}},
                {"general_behavior": {"close_to_system_tray": "false"}},
                {"general_behavior": {"mapping_start_action_mode": -1}},
                {"general_behavior": {"mapping_start_action_mode": 2}},
                {"general_behavior": {"mapping_start_action_mode": None}},
                {"general_behavior": {"close_to_system_tray": True, "extra": 1}},
                {"general_behavior": {"close_to_system_tray": True}, "extra": 1},
            ):
                request = {**behavior, **bad_behavior}
                assert not validator.is_valid(request), bad_behavior
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            hotkeys = {"action": "configuration.set_mapping_hotkeys", "source_tab_ref": tab}
            assert validator.is_valid({**hotkeys, "mapping_hotkeys": {
                "mapping_start_key": "L-Ctrl+F8",
                "mapping_stop_key": "L-Shift+F9",
            }})
            for bad_hotkeys in (
                {"mapping_hotkeys": {}},
                {"mapping_hotkeys": {"mapping_start_key": ""}},
                {"mapping_hotkeys": {"mapping_stop_key": "   "}},
                {"mapping_hotkeys": {"mapping_start_key": None}},
                {"mapping_hotkeys": {"mapping_stop_key": 123}},
                {"mapping_hotkeys": {"mapping_start_key": "F8\nF9"}},
                {"mapping_hotkeys": {"mapping_stop_key": "A" * 257}},
                {"mapping_hotkeys": {"mapping_start_key": "F8", "extra": 1}},
                {"mapping_hotkeys": {"mapping_start_key": "F8"}, "extra": 1},
            ):
                request = {**hotkeys, **bad_hotkeys}
                assert not validator.is_valid(request), bad_hotkeys
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            profile_desc = {"action": "configuration.set_description", "source_tab_ref": tab}
            assert validator.is_valid({**profile_desc, "description": "测试说明🎮"})
            assert validator.is_valid({**profile_desc, "description": ""})
            for bad_desc in (
                {"description": None},
                {"description": 123},
                {"description": "Line1\nLine2"},
                {"description": "Line1\rLine2"},
                {"description": "A" * 1025},
                {"description": "ok", "extra": 1},
            ):
                request = {**profile_desc, **bad_desc}
                assert not validator.is_valid(request), bad_desc
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            gyro_params = {"action": "configuration.set_gyro2mouse_params", "source_tab_ref": tab}
            assert validator.is_valid({**gyro_params, "gyro2mouse_params": {
                "x_speed": 12.35, "y_speed": 99.99,
                "min_x_sensitivity": 0.0, "max_x_sensitivity": 200.0,
                "min_y_sensitivity": 7.5, "max_y_sensitivity": 7.5,
                "min_threshold": 1.5, "max_threshold": 500.0,
            }})
            assert validator.is_valid({**gyro_params, "gyro2mouse_params": {"x_speed": 0}})
            for bad_gyro_params in (
                {"gyro2mouse_params": {}},
                {"gyro2mouse_params": {"x_speed": -0.01}},
                {"gyro2mouse_params": {"y_speed": 100.0}},
                {"gyro2mouse_params": {"min_x_sensitivity": -0.1}},
                {"gyro2mouse_params": {"max_x_sensitivity": 200.1}},
                {"gyro2mouse_params": {"min_threshold": -0.1}},
                {"gyro2mouse_params": {"max_threshold": 500.1}},
                {"gyro2mouse_params": {"x_speed": None}},
                {"gyro2mouse_params": {"x_speed": "3.0"}},
                {"gyro2mouse_params": {"x_speed": 3.0, "extra": 1}},
                {"gyro2mouse_params": {"x_speed": 3.0}, "extra": 1},
            ):
                request = {**gyro_params, **bad_gyro_params}
                assert not validator.is_valid(request), bad_gyro_params
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            for bad_precision_or_pair in (
                {"x_speed": 1.234},
                {"min_x_sensitivity": 7.25},
                {"min_threshold": 1.55},
                {"min_x_sensitivity": 20.1, "max_x_sensitivity": 20.0},
                {"min_y_sensitivity": 30.0, "max_y_sensitivity": 15.0},
                {"min_threshold": 50.1, "max_threshold": 50.0},
            ):
                request = {**gyro_params, "gyro2mouse_params": bad_precision_or_pair}
                assert validator.is_valid(request), bad_precision_or_pair
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            point_keys = {"action": "configuration.set_point_display_keys", "source_tab_ref": tab}
            for fields in ({"window_point_key": "None"}, {"screen_point_key": "F10"},
                           {"window_point_key": "None", "screen_point_key": "F10"},
                           {"window_point_key": "A" * 64}):
                request = point_keys | {"point_display_keys": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for fields in ({}, {"window_point_key": ""}, {"screen_point_key": "A" * 65},
                           {"window_point_key": "F8\nF9"}, {"window_point_key": "F8\rF9"},
                           {"window_point_key": "F8\0F9"}, {"window_point_key": None},
                           {"screen_point_key": 123}, {"window_point_key": "F8", "extra": "F9"}):
                request = point_keys | {"point_display_keys": fields}
                assert not validator.is_valid(request), fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            custom_notify = {"action": "configuration.set_custom_notification", "source_tab_ref": tab}
            for fields in ({"enabled": True}, {"enabled": False}, {"position": 0}, {"position": 6},
                           {"enabled": True, "position": 5}):
                request = custom_notify | {"custom_notification": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for fields in ({}, {"enabled": 1}, {"position": True}, {"position": -1}, {"position": 7},
                           {"position": 3.5}, {"position": None}, {"enabled": False, "position": 3},
                           {"enabled": True, "extra": 1}):
                request = custom_notify | {"custom_notification": fields}
                assert not validator.is_valid(request), fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            win_criteria = {"action": "configuration.set_window_match_criteria", "source_tab_ref": tab}
            for fields in ({"process_name": "DemoApp.exe"},
                           {"process_name": "", "process_match_mode": 0},
                           {"window_title": "^MyWindow-[0-9]+$", "window_title_match_mode": 5},
                           {"class_name": "A" * 1024, "class_name_match_mode": 3},
                           {"process_name": "DemoApp.exe", "process_match_mode": 1,
                            "window_title": "Title", "window_title_match_mode": 2,
                            "class_name": "Class", "class_name_match_mode": 4}):
                request = win_criteria | {"window_match_criteria": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for fields in ({}, {"process_name": "A" * 1025}, {"window_title": "A\nB"},
                           {"class_name": "A\rB"}, {"process_name": "A\0B"}, {"process_name": None},
                           {"process_match_mode": True}, {"window_title_match_mode": 1.5},
                           {"class_name_match_mode": -1}, {"process_match_mode": 6},
                           {"process_name": "DemoApp.exe", "extra": 1}):
                request = win_criteria | {"window_match_criteria": fields}
                assert not validator.is_valid(request), fields
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            startup = {"action": "general.set_startup_position", "source_tab_ref": tab}
            point = {"x": -50000, "y": 50000}
            for fields in ({"mode": 0}, {"mode": 1}, {"mode": 2}, {"point": point}, {"mode": 2, "point": point}):
                request = startup | {"startup_position": fields}
                validator.validate(request)
                expect_not_connected(await client.call_tool("qkm_preview_changes", request))
            for fields in ({}, {"mode": True}, {"mode": 3}, {"mode": None, "point": point},
                           {"mode": 1, "point": point}, {"point": {"x": 30}}, {"point": {"x": 30, "y": 50001}},
                           {"point": point | {"extra": 1}}, {"mode": 2, "point": None}):
                request = startup | {"startup_position": fields}
                assert not validator.is_valid(request)
                denied = await client.call_tool("qkm_preview_changes", request)
                assert denied.is_error is True and denied.structured_content is None
            for scale in (0, 2, 10):
                scale_req = {"action": "general.set_display_scale", "source_tab_ref": tab, "display_scale": scale}
                validator.validate(scale_req)
                expect_not_connected(await client.call_tool("qkm_preview_changes", scale_req))
            for bad_scale in (
                {"action": "general.set_display_scale", "source_tab_ref": tab},
                {"action": "general.set_display_scale", "display_scale": 2},
                {"action": "general.set_display_scale", "source_tab_ref": row, "display_scale": 2},
                {"action": "general.set_display_scale", "source_tab_ref": tab, "display_scale": -1},
                {"action": "general.set_display_scale", "source_tab_ref": tab, "display_scale": 11},
                {"action": "general.set_display_scale", "source_tab_ref": tab, "display_scale": 1.5},
                {"action": "general.set_display_scale", "source_tab_ref": tab, "display_scale": True},
                {"action": "general.set_display_scale", "source_tab_ref": tab, "display_scale": 2, "extra": 1},
            ):
                assert not validator.is_valid(bad_scale), bad_scale
                denied = await client.call_tool("qkm_preview_changes", bad_scale)
                assert denied.is_error is True and denied.structured_content is None
            expect_not_connected(await client.call_tool("qkm_preview_changes", upsert))
            assert macro_fields["properties"]["segment_comments"] == {"type": ["array", "null"], "minItems": 1,
                                                                      "items": {"type": "string", "maxLength": 32767}}
            with_notes = upsert | {"macro": macro | {"segment_comments": ["", "second"]}}
            expect_not_connected(await client.call_tool("qkm_preview_changes", with_notes))
            for invalid in (True, [], ["", 3]):
                result = await client.call_tool("qkm_preview_changes", upsert | {"macro": macro | {"segment_comments": invalid}})
                assert result.is_error is True and result.structured_content is None
            placement = {"position": "below", "anchor_name": "Existing"}
            nested_position, null_position = macro_fields["properties"]["insert_at"]["anyOf"]
            assert not nested_position["additionalProperties"] and null_position == {"type": "null"}
            assert nested_position["required"] == ["position", "anchor_name"]
            assert nested_position["properties"]["position"]["enum"] == ["above", "below"]
            expect_not_connected(await client.call_tool("qkm_preview_changes", upsert | {"macro": macro | {"insert_at": placement}}))
            for invalid in (placement | {"position": "top"}, placement | {"anchor_name": ""}, placement | {"extra": True}):
                result = await client.call_tool("qkm_preview_changes", upsert | {"macro": macro | {"insert_at": invalid}})
                assert result.is_error is True and result.structured_content is None
            for invalid in ({"action": "macro.upsert"}, upsert | {"note": ""},
                            upsert | {"macro": macro | {"scope": "other"}},
                            upsert | {"macro": macro | {"content": ""}},
                            upsert | {"macro": macro | {"note": 2}},
                            upsert | {"macro": macro | {"extra": True}}):
                denied = await client.call_tool("qkm_preview_changes", invalid)
                assert denied.is_error is True and denied.structured_content is None
            ignore_rule_base = {"catalog_revision": revision, "rule_name": "  CustomRule  "}
            ignore_upsert = {"action": "ignore_rules.upsert", "ignore_rule": ignore_rule_base}
            for fields in (
                ignore_rule_base,
                ignore_rule_base | {
                    "rule_name": "A" * 1024,
                    "process_name": "DemoApp.exe",
                    "process_match_mode": 1,
                    "window_title": "^MyWindow$",
                    "window_title_match_mode": 5,
                    "class_name": "Qt6QWindow",
                    "class_name_match_mode": 3,
                    "description": "Note",
                    "disabled": True,
                },
            ):
                req = {"action": "ignore_rules.upsert", "ignore_rule": fields}
                validator.validate(req)
                expect_not_connected(await client.call_tool("qkm_preview_changes", req))
            for bad in (
                {"catalog_revision": "bad", "rule_name": "Rule"},
                {"catalog_revision": revision},
                {"catalog_revision": revision, "rule_name": ""},
                {"catalog_revision": revision, "rule_name": "   "},
                {"catalog_revision": revision, "rule_name": "A" * 1025},
                {"catalog_revision": revision, "rule_name": "Bad\nRule"},
                {"catalog_revision": revision, "rule_name": "Rule", "process_name": "A" * 1025},
                {"catalog_revision": revision, "rule_name": "Rule", "description": "L1\rL2"},
                {"catalog_revision": revision, "rule_name": "Rule", "process_match_mode": -1},
                {"catalog_revision": revision, "rule_name": "Rule", "window_title_match_mode": 6},
                {"catalog_revision": revision, "rule_name": "Rule", "class_name_match_mode": True},
                {"catalog_revision": revision, "rule_name": "Rule", "disabled": 1},
                {"catalog_revision": revision, "rule_name": "Rule", "extra": 1},
            ):
                bad_req = {"action": "ignore_rules.upsert", "ignore_rule": bad}
                assert not validator.is_valid(bad_req), bad
                denied = await client.call_tool("qkm_preview_changes", bad_req)
                assert denied.is_error is True and denied.structured_content is None
            denied = await client.call_tool("qkm_preview_changes", ignore_upsert | {"extra": 1})
            assert denied.is_error is True and denied.structured_content is None
            ignore_del_rule = {"catalog_revision": revision, "rule_names": ["RuleA", "RuleB"]}
            ignore_del = {"action": "ignore_rules.delete", "ignore_rule": ignore_del_rule}
            validator.validate(ignore_del)
            expect_not_connected(await client.call_tool("qkm_preview_changes", ignore_del))
            for bad_del in (
                {"catalog_revision": revision},
                {"catalog_revision": revision, "rule_names": []},
                {"catalog_revision": revision, "rule_names": ["A", "A"]},
                {"catalog_revision": revision, "rule_names": [f"R{i}" for i in range(101)]},
                {"catalog_revision": revision, "rule_names": [""]},
                {"catalog_revision": revision, "rule_names": ["Bad\nRule"]},
                {"catalog_revision": revision, "rule_names": ["A" * 1025]},
                {"catalog_revision": revision, "rule_names": ["A"], "rule_name": "A"},
            ):
                bad_req = {"action": "ignore_rules.delete", "ignore_rule": bad_del}
                assert not validator.is_valid(bad_req), bad_del
                denied = await client.call_tool("qkm_preview_changes", bad_req)
                assert denied.is_error is True and denied.structured_content is None
            ignore_def_rule = {"catalog_revision": revision}
            ignore_def = {"action": "ignore_rules.restore_defaults", "ignore_rule": ignore_def_rule}
            validator.validate(ignore_def)
            expect_not_connected(await client.call_tool("qkm_preview_changes", ignore_def))
            for bad_def in (
                {},
                {"catalog_revision": "bad"},
                {"catalog_revision": None},
                {"catalog_revision": revision, "rule_name": "A"},
                {"catalog_revision": revision, "rule_names": ["A"]},
            ):
                bad_req = {"action": "ignore_rules.restore_defaults", "ignore_rule": bad_def}
                assert not validator.is_valid(bad_req), bad_def
                denied = await client.call_tool("qkm_preview_changes", bad_req)
                assert denied.is_error is True and denied.structured_content is None
            restore_path_req = {"action": "configuration.restore_process_path", "source_tab_ref": tab}
            validator.validate(restore_path_req)
            expect_not_connected(await client.call_tool("qkm_preview_changes", restore_path_req))
            for bad_rp in (
                {"action": "configuration.restore_process_path"},
                {"action": "configuration.restore_process_path", "source_tab_ref": row},
                {"action": "configuration.restore_process_path", "source_tab_ref": tab, "extra": True},
            ):
                assert not validator.is_valid(bad_rp), bad_rp
                denied = await client.call_tool("qkm_preview_changes", bad_rp)
                assert denied.is_error is True and denied.structured_content is None
            legacy_preview_fields = set(previews["properties"]) - {
                "copy_to", "macro", "ignore_rule", "table_insert_mode", "table_edit_trigger", "window_display_mode", "file_name",
                "save_window_size", "save_splitter_position", "play_sound_effect", "timeout_ms",
                "fullscreen_global_mapping", "key2mouse", "gyro2mouse_options", "gamepad_thresholds", "gamepad_touchpad", "accept_virtual_gamepad_input", "vbutton_panel", "notification_style", "tray_icons", "floating_window", "custom_image", "crosshair", "floating_button", "show_drag_coordinate", "sync_group_note", "sync_group_move", "general_ui_preferences", "general_behavior", "mapping_hotkeys", "description", "gyro2mouse_params", "process_icon_as_tray_icon", "point_display_keys",
                "custom_notification", "window_match_criteria", "auto_match_foreground", "send_to_same_windows", "startup_position",
                "display_scale", "catalog_revision", "groups", "on_conflict", "import_draft_id", "group", "target_group"}
            move_rule, = (rule for rule in previews["oneOf"]
                          if rule["properties"]["action"].get("enum") == moves)
            assert move_rule["required"] == ["action"]
            single_rule, batch_rule = move_rule["oneOf"]
            assert single_rule["required"] == ["source_ref"]
            assert single_rule["properties"] == {"source_ref": {"type": "string"}, "source_refs": {"type": "null"}}
            assert batch_rule["required"] == ["source_refs"]
            assert batch_rule["properties"] == {
                "source_ref": {"type": "null"}, "source_refs": {"type": "array", "minItems": 2}}
            assert set(move_rule["properties"]) == legacy_preview_fields
            assert all(rule == {"type": "null"} for field, rule in move_rule["properties"].items()
                       if field not in {"action", "source_ref", "source_refs"})
            rename_rule, = (rule for rule in previews["oneOf"]
                            if rule["properties"]["action"].get("const") == "tab.rename")
            assert set(rename_rule["required"]) == set(rename)
            assert rename_rule["properties"]["source_tab_ref"] == {"type": "string"}
            assert rename_rule["properties"]["tab_name"] == {"type": "string"}
            assert set(rename_rule["properties"]) == legacy_preview_fields
            assert all(rule == {"type": "null"} for field, rule in rename_rule["properties"].items()
                       if field not in rename)
            add_rule, = (rule for rule in previews["oneOf"]
                         if "tab.add_blank" in rule["properties"]["action"].get("enum", []))
            assert set(add_rule["properties"]["action"]["enum"]) == {"tab.add_blank", "tab.copy", "tab.delete", "tab.clear"}
            assert set(add_rule["required"]) == set(add_tab) == set(copy_tab) == set(delete_tab) == set(clear_tab)
            assert add_rule["properties"]["source_tab_ref"] == {"type": "string"}
            assert set(add_rule["properties"]) == legacy_preview_fields
            assert all(rule == {"type": "null"} for field, rule in add_rule["properties"].items()
                       if field not in add_tab)
            move_tab_rule, = (rule for rule in previews["oneOf"]
                              if rule["properties"]["action"].get("const") == "tab.move")
            assert set(move_tab_rule["required"]) == set(move_tab)
            assert move_tab_rule["properties"]["target_tab_ref"] == {"type": "string"}
            assert set(move_tab_rule["properties"]) == legacy_preview_fields
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
                    tool for tool in catalog["tools"] if tool["name"] not in {EXPORT, EXPORT_TABLE, EXPORT_CONFIGURATIONS}
                    and (tool["name"] != DEMO_MAPPING or enabled)]
                expect_not_connected(await client.call_tool(DEMO_STATE))
                if enabled:
                    mapping = {"original_key": "A", "mapping_key": "B"}
                    expect_not_connected(await client.call_tool(DEMO_MAPPING, mapping))
                    for extra in ({"view": "ibinput_settings"}, {"original_key": "a"}, {"pid": 123}):
                        denied = await client.call_tool(DEMO_MAPPING, mapping | extra)
                        assert denied.is_error is True and denied.structured_content is None
        print("PASS", mode, "explicit and product demo-record tool gates")

        with tempfile.TemporaryDirectory(prefix="qkm-backup-contract-", dir=ROOT / "out") as directory:
            async with Client(config(exe, "--macro-backup-directory", directory), mode=mode, cache=None) as client:
                tools = (await client.list_tools()).tools
                assert {tool.name for tool in tools} == TOOLS | {EXPORT, EXPORT_TABLE, EXPORT_CONFIGURATIONS}
                transfer, = (tool for tool in tools if tool.name == EXPORT)
                assert transfer.annotations.read_only_hint is False and transfer.annotations.destructive_hint is True
                schema = transfer.input_schema
                assert schema["additionalProperties"] is False
                assert set(schema["required"]) == {"catalog_revision", "scope", "file_name"}
                assert set(schema["properties"]) == {"catalog_revision", "scope", "file_name", "names", "overwrite"}
                assert schema["properties"]["overwrite"] == {"type": "boolean", "default": False}
                request = {"catalog_revision": revision, "scope": "universal", "file_name": "宏 示例.ini"}
                expect_not_connected(await client.call_tool(EXPORT, request))
                expect_not_connected(await client.call_tool(EXPORT, request | {"names": ["Z", "共享"]}))
                table, = (tool for tool in tools if tool.name == EXPORT_TABLE)
                table_request = {"source_tab_ref": tab, "file_name": "表 格.ini"}
                assert table.annotations.read_only_hint is False and table.annotations.destructive_hint is True
                assert set(table.input_schema["required"]) == set(table_request)
                assert set(table.input_schema["properties"]) == set(table_request) | {"overwrite"}
                Draft202012Validator(table.input_schema).validate(table_request)
                expect_not_connected(await client.call_tool(EXPORT_TABLE, table_request))
                expect_not_connected(await client.call_tool(EXPORT_TABLE, table_request | {"overwrite": True}))
                for extra in ({"source_tab_ref": row}, {"file_name": "../example.ini"},
                              {"file_name": "NUL.ini"}, {"file_name": "Example.txt"},
                              {"overwrite": 1}, {"catalog_revision": revision}):
                    denied = await client.call_tool(EXPORT_TABLE, table_request | extra)
                    assert denied.is_error is True and denied.structured_content is None, extra
                configuration, = (tool for tool in tools if tool.name == EXPORT_CONFIGURATIONS)
                configuration_request = {"catalog_revision": revision, "groups": ["已保存", "Other"],
                    "file_name": "配置 备份.ini"}
                assert configuration.annotations.read_only_hint is False and configuration.annotations.destructive_hint is True
                assert set(configuration.input_schema["required"]) == set(configuration_request)
                Draft202012Validator(configuration.input_schema).validate(configuration_request)
                expect_not_connected(await client.call_tool(EXPORT_CONFIGURATIONS, configuration_request))
                expect_not_connected(await client.call_tool(EXPORT_CONFIGURATIONS, configuration_request | {"overwrite": True}))
                for extra in ({"groups": []}, {"groups": ["A", "A"]}, {"groups": [""]}, {"groups": ["A\n"]},
                              {"groups": list(map(str, range(101)))}, {"file_name": "../other.ini"},
                              {"file_name": "NUL.ini"}, {"overwrite": 1}, {"source_tab_ref": tab}):
                    denied = await client.call_tool(EXPORT_CONFIGURATIONS, configuration_request | extra)
                    assert denied.is_error is True and denied.structured_content is None, extra
                expect_not_connected(await client.call_tool("qkm_preview_changes", import_request))
                expect_not_connected(await client.call_tool("qkm_preview_changes",
                                      import_request | {"macro": import_args | {"on_conflict": "overwrite"}}))
                expect_not_connected(await client.call_tool("qkm_preview_changes", table_import))
                for extra in ({"source_tab_ref": row}, {"file_name": "NUL.ini"}, {"file_name": "../escape.ini"},
                              {"document_base64": "AAAA"}, {"overwrite": True}, {"source_revision": revision}):
                    denied = await client.call_tool("qkm_preview_changes", table_import | extra)
                    assert denied.is_error is True and denied.structured_content is None, extra
                for fields in (request, import_args | {"file_name": "../escape.ini"},
                               import_args | {"on_conflict": "replace"}, import_args | {"document_base64": "AAAA"},
                               import_args | {"names": ["Z"]}, import_args | {"catalog_revision": "stale"}):
                    denied = await client.call_tool("qkm_preview_changes", import_request | {"macro": fields})
                    assert denied.is_error is True and denied.structured_content is None, fields
                invalid = ({"names": []}, {"names": None}, {"names": ["A", "A"]}, {"overwrite": 1},
                           {"file_name": "../escape.ini"}, {"file_name": r"C:\escape.ini"},
                           {"file_name": "A.ini:other"}, {"file_name": "NUL.ini"}, {"file_name": "example.txt"},
                           {"scope": "both"}, {"catalog_revision": "A" * 64}, {"path": "unused"})
                for extra in invalid:
                    denied = await client.call_tool(EXPORT, request | extra)
                    assert denied.is_error is True and denied.structured_content is None, extra
            assert not list(Path(directory).iterdir())
        print("PASS", mode, "opt-in backup contracts")

    parameters = config(exe, "--enable-demo-mapping")
    rejected = subprocess.run([parameters.command, *parameters.args], capture_output=True, timeout=5)
    assert rejected.returncode == 2 and rejected.stdout == b""
    invalid_directory = ROOT / "out/rust_sidecar/catalog-only/nonexistent-macro-backup-directory"
    assert not invalid_directory.exists()
    parameters = config(exe, "--macro-backup-directory", str(invalid_directory))
    rejected = subprocess.run([parameters.command, *parameters.args], capture_output=True, timeout=5)
    assert rejected.returncode != 0 and rejected.stdout == b"" and b"local directory" in rejected.stderr
    assert not invalid_directory.exists()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    args = parser.parse_args()
    if sys.flags.optimize:
        parser.error("Run with assertions enabled.")
    asyncio.run(check_contract(args.exe.resolve(strict=True)))
