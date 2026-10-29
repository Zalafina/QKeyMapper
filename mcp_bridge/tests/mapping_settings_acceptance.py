"""Owned advanced-window lifecycle against an independent original Qt GUI baseline."""

import asyncio
import base64
import hashlib
import time

from jsonschema import Draft202012Validator

from advanced_gui_acceptance import read_frame
from test_sdk_interop import business_state


async def exercise(client, request, variant, output, exe):
    assert variant in ("settings_window", "settings_no_show")
    tools = {tool.name: tool for tool in (await client.list_tools()).tools}
    schema_errors = 0
    for name in ("qkm_open_mapping_settings", "qkm_close_mapping_settings"):
        metadata = tools[name].model_dump(by_alias=True)
        assert metadata["annotations"]["readOnlyHint"] is False
        arguments = dict(source_tab_ref="a" * 64 + ":source:0", presentation="private_background")
        if "close" in name:
            arguments["window_ref"] = "b" * 64
        validator = Draft202012Validator(metadata["inputSchema"])
        assert not list(validator.iter_errors(arguments))
        for extra in (dict(pid=1), dict(hwnd="1"), dict(path="other.exe"), dict(role="main"),
                      dict(discard=True), dict(presentation="foreground"),
                      dict(source_tab_ref="a" * 64 + ":row:0:0")):
            assert list(validator.iter_errors({**arguments, **extra})), (name, extra)
            schema_errors += 1
    capabilities = (await request("qkm_get_capabilities"))["result"]["business_capabilities"]
    for action in ("open", "close"):
        capability = next(item for item in capabilities if item["id"] == "mapping.settings." + action)
        assert capability["required_scopes"] == 129 and capability["read_only"] is False
        assert capability["arguments_schema"]["additionalProperties"] is False

    initial = business_state((await request("qkm_get_state"))["result"])
    sources = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    before = (await request("qkm_query", dict(entity="demo_context")))["result"]
    disk = (exe.parent / "keymapdata.ini").read_bytes()
    preview = (await request("qkm_preview_changes", dict(original_key="A", mapping_key="B",
        source_revision=sources["source_snapshot"]["revision"], source_tab_ref=sources["items"][0]["source_tab_ref"])))["result"]
    applied = (await request("qkm_apply_changes", dict(preview_id=preview["preview_id"])))["result"]
    assert applied["status"] == "applied" and applied["result"]["source_binding_status"] == "captured"
    owned = applied["result"]["source_binding_after"]
    row_args = dict(entity="mappings", source_revision=owned["source_snapshot"]["revision"], source_tab_ref=owned["source_tab_ref"])
    rows = (await request("qkm_query", row_args))["result"]
    assert len(rows["items"]) == 1 and rows["items"][0]["source_ref"] == owned["source_ref"]
    context = (await request("qkm_query", dict(entity="demo_context")))["result"]
    assert context["context_revision"] == before["context_revision"] and context["source_snapshot"] == owned["source_snapshot"]
    args = dict(source_tab_ref=owned["source_tab_ref"], presentation="private_background")

    async def window():
        page = (await request("qkm_query", dict(entity="windows", window_role="mapping_advanced")))["result"]
        assert page["total"] == 1 and page["items"][0]["role"] == "mapping_advanced"
        return page["items"][0]

    async def marker(start, finish):
        (output / ("case.json." + start)).write_text("{}", encoding="utf-8")
        path = output / ("case.json." + finish)
        until = time.monotonic() + 5
        while not path.is_file():
            assert time.monotonic() < until, finish
            await asyncio.sleep(.01)
        value = read_frame(path)
        assert value["passed"], value
        return value

    gui_matches = None
    frames = 0
    assert not (await window())["qt_visible"]
    if variant == "settings_no_show":
        await request("qkm_open_mapping_settings", args, error="permission_denied")
        await request("qkm_close_mapping_settings", {**args, "window_ref": "b" * 64}, error="permission_denied")
        assert not (await window())["qt_visible"]
    else:
        await request("qkm_open_mapping_settings", {**args, "source_tab_ref": sources["items"][0]["source_tab_ref"]}, error="stale_source")
        await marker("advanced-open", "advanced-opened")
        expected = read_frame(output / "expected-target.json")
        start = read_frame(output / "start.json")
        for role in ("mapping_main_gui", "mapping_advanced_gui"):
            frame = read_frame(output / ("case.json.frame-" + role))
            assert frame["scene"] == "advanced-gui119" and frame["role"] == role
            assert frame["instance"] == initial["instance"] and frame["desktop"] == expected["desktop"]
            assert frame["pid"] == str(start["ProcessId"]) and int(frame["hwnd"]) > 0
            assert frame["load_generation"] == initial["mapping_load_context"]["generation"]
            png = base64.b64decode(frame["png_base64"], validate=True)
            assert 0 < len(png) == frame["bytes"] <= 160 * 1024
            (output / ("gui-" + role + ".png")).write_bytes(png)
            frames += 1
        gui_window = await window()
        assert gui_window["qt_visible"] and gui_window["native_visible"]
        await request("qkm_close_mapping_settings", {**args, "window_ref": gui_window["window_ref"]}, error="editor_not_owned")
        await request("qkm_open_mapping_settings", args, error="context_busy")
        closed = await marker("advanced-close", "advanced-closed")
        assert closed["input_fingerprint_unchanged"] and closed["original_page_restored"]
        assert (await request("qkm_query", dict(entity="demo_context")))["result"] == context

        opened = (await request("qkm_open_mapping_settings", args))["result"]
        assert opened["opened"] and not opened["activation_requested"] and opened["role"] == "mapping_advanced"
        assert opened["source_context"] == owned["source_snapshot"] and opened["source_tab_ref"] == owned["source_tab_ref"]
        visible = opened["window"]
        assert visible["qt_visible"] and visible["native_visible"] and not visible["minimized"]
        assert visible["window_ref"] != gui_window["window_ref"]
        capture_args = dict(role="mapping_advanced", window_ref=visible["window_ref"],
            source_revision=owned["source_snapshot"]["revision"], source_tab_ref=owned["source_tab_ref"])

        async def capture(unchanged):
            value = (await request("qkm_capture_window", capture_args))["result"]
            assert value["settings_scope"] == "live_loaded_settings_not_committed_by_source_revision"
            assert value["settings_editor_unchanged"] is unchanged
            assert value["source_context"] == owned["source_snapshot"]
            return value

        image = await capture(True)
        gui_matches = image["sha256"] == hashlib.sha256((output / "gui-mapping_advanced_gui.png").read_bytes()).hexdigest()
        await request("qkm_open_mapping_settings", args, error="context_busy")
        await marker("settings-edit", "settings-edited")
        await request("qkm_close_mapping_settings", {**args, "window_ref": visible["window_ref"]}, error="editor_draft_changed")
        assert (await window())["qt_visible"]
        await capture(False)
        assert (await request("qkm_query", row_args))["result"] == rows
        await marker("settings-restore", "settings-restored")
        await capture(True)
        closed = (await request("qkm_close_mapping_settings", {**args, "window_ref": visible["window_ref"]}))["result"]
        assert closed["closed"] and not closed["activation_requested"]
        assert not closed["window"]["qt_visible"] and not closed["window"]["native_visible"]
        await request("qkm_close_mapping_settings", {**args, "window_ref": visible["window_ref"]}, error="stale_window")
        second = (await request("qkm_open_mapping_settings", args))["result"]
        assert second["opened"] and second["window"]["window_ref"] != visible["window_ref"]
        assert (await request("qkm_close_mapping_settings", {**args, "window_ref": second["window"]["window_ref"]}))["result"]["closed"]

    assert (await request("qkm_query", row_args))["result"] == rows
    assert (await request("qkm_query", dict(entity="demo_context")))["result"] == context
    assert (exe.parent / "keymapdata.ini").read_bytes() == disk
    deletion = (await request("qkm_preview_changes", dict(action="mapping.delete",
        source_revision=owned["source_snapshot"]["revision"], source_tab_ref=owned["source_tab_ref"],
        source_refs=[owned["source_ref"]])))["result"]
    deleted = (await request("qkm_apply_changes", dict(preview_id=deletion["preview_id"], confirm_delete=True)))["result"]
    assert deleted["status"] == "applied" and deleted["result"]["deleted_count"] == 1
    final = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    assert final["source_snapshot"] == deleted["result"]["source_binding_after"]["source_snapshot"]
    assert final["items"][0]["row_count"] == final["items"][1]["row_count"] == 0
    assert (await request("qkm_query", dict(entity="demo_context")))["result"]["context_revision"] == before["context_revision"]
    assert (exe.parent / "keymapdata.ini").read_bytes() == disk
    return dict(successful_saves=0, schema_errors=schema_errors, gui_frames=frames,
        formal_settings_window=variant == "settings_window", full_context_preserved=True,
        capture_matches_original_gui_png=gui_matches)
