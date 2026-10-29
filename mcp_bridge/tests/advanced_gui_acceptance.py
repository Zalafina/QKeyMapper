"""Independent Qt click/Escape advanced-window baseline, using a formal MCP-created row."""

import asyncio
import base64
import hashlib
import json
import struct
import time

from test_sdk_interop import business_state


def read_frame(path):
    with path.open("rb") as stream:
        data = stream.read(256 * 1024 + 1)
    assert len(data) <= 256 * 1024
    return json.loads(data.decode("utf-8"))


async def exercise(client, request, variant, output, exe, *, capture=False):
    assert variant in ("advanced_gui", "advanced_gui_no_capture")
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
    if capture:
        capabilities = (await request("qkm_get_capabilities"))["result"]["business_capabilities"]
        window_cap = next(value for value in capabilities if value["id"] == "windows.read")
        assert window_cap["arguments_schema"]["properties"]["window_role"]["const"] == "mapping_advanced"
        hidden = (await request("qkm_query", dict(entity="windows", window_role="mapping_advanced")))["result"]
        assert hidden["total"] == 1 and hidden["items"][0]["role"] == "mapping_advanced" and not hidden["items"][0]["qt_visible"]
        await request("qkm_query", dict(entity="mapping_sources", window_role="mapping_advanced"), error="invalid_arguments")
        await request("qkm_query", dict(entity="windows", window_role="mapping_advanced", offset=2), error="invalid_arguments")

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

    opened = await marker("advanced-open", "advanced-opened")
    expected = read_frame(output / "expected-target.json")
    start = read_frame(output / "start.json")
    epochs = set()
    for role in ("mapping_main_gui", "mapping_advanced_gui"):
        frame = read_frame(output / ("case.json.frame-" + role))
        assert frame["scene"] == "advanced-gui119" and frame["role"] == role
        assert frame["instance"] == initial["instance"] and frame["desktop"] == expected["desktop"]
        assert frame["pid"] == str(start["ProcessId"]) and int(frame["hwnd"]) > 0
        assert frame["load_generation"] == initial["mapping_load_context"]["generation"]
        epochs.add(frame["source_epoch"])
        png = base64.b64decode(frame.pop("png_base64"), validate=True)
        assert 0 < len(png) == frame["bytes"] <= 160 * 1024 and png[:8] == b"\x89PNG\r\n\x1a\n"
        assert struct.unpack(">II", png[16:24]) == (frame["width"], frame["height"])
        assert frame["width"] * frame["height"] <= 8 * 1024 * 1024
        frame["sha256"] = hashlib.sha256(png).hexdigest()
        (output / ("gui-" + role + ".png")).write_bytes(png)
        (output / ("gui-" + role + ".json")).write_text(json.dumps(frame, ensure_ascii=False, indent=2), encoding="utf-8")
    assert len(epochs) == 1
    matched = None
    if capture:
        standard = (await request("qkm_query", dict(entity="windows")))["result"]
        assert [item["role"] for item in standard["items"]] == ["main", "item_setup"]
        selected = (await request("qkm_query", dict(entity="windows", window_role="mapping_advanced")))["result"]
        assert selected["total"] == 1 and selected["target"] == standard["target"]
        visible = selected["items"][0]
        assert visible["qt_visible"] and visible["native_visible"] and visible["native_ready"] and not visible["minimized"]
        capture_args = dict(role="mapping_advanced", window_ref=visible["window_ref"],
            source_revision=owned["source_snapshot"]["revision"], source_tab_ref=owned["source_tab_ref"])
        if variant == "advanced_gui_no_capture":
            await request("qkm_capture_window", capture_args, error="permission_denied")
        else:
            await request("qkm_capture_window", {**capture_args, "source_revision": sources["source_snapshot"]["revision"],
                "source_tab_ref": sources["items"][0]["source_tab_ref"]}, error="stale_source")
            image = (await request("qkm_capture_window", capture_args))["result"]
            assert image["settings_scope"] == "live_loaded_settings_not_committed_by_source_revision"
            assert image["source_context"] == owned["source_snapshot"] and "edit_target" not in image
            actual_png = next(path for path in output.glob("mcp-image-*-mapping_advanced.png"))
            matched = actual_png.read_bytes() == (output / "gui-mapping_advanced_gui.png").read_bytes()
    assert (await request("qkm_query", row_args))["result"] == rows
    await request("qkm_query", dict(entity="demo_context"), error="context_busy")
    deletion_args = dict(action="mapping.delete", source_revision=owned["source_snapshot"]["revision"],
        source_tab_ref=owned["source_tab_ref"], source_refs=[owned["source_ref"]])
    await request("qkm_preview_changes", deletion_args, error="context_busy")
    closed = await marker("advanced-close", "advanced-closed")
    assert closed["input_fingerprint_unchanged"] and closed["original_page_restored"]
    if capture:
        hidden = (await request("qkm_query", dict(entity="windows", window_role="mapping_advanced")))["result"]["items"][0]
        assert not hidden["qt_visible"] and hidden["window_ref"] != visible["window_ref"]
        if variant != "advanced_gui_no_capture":
            await request("qkm_capture_window", capture_args, error="stale_window")
    assert (await request("qkm_query", row_args))["result"] == rows
    after_gui = (await request("qkm_query", dict(entity="demo_context")))["result"]
    assert after_gui == context and (exe.parent / "keymapdata.ini").read_bytes() == disk
    deletion = (await request("qkm_preview_changes", deletion_args))["result"]
    deleted = (await request("qkm_apply_changes", dict(preview_id=deletion["preview_id"], confirm_delete=True)))["result"]
    assert deleted["status"] == "applied" and deleted["result"]["deleted_count"] == 1
    final = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    assert final["source_snapshot"] == deleted["result"]["source_binding_after"]["source_snapshot"]
    assert final["items"][0]["row_count"] == final["items"][1]["row_count"] == 0
    assert (await request("qkm_query", dict(entity="demo_context")))["result"]["context_revision"] == before["context_revision"]
    assert (exe.parent / "keymapdata.ini").read_bytes() == disk
    return dict(successful_saves=0, independent_advanced_gui=True, gui_frames=2, schema_errors=0,
        dialog_active_observed=opened["dialog_active"], full_context_preserved=True,
        formal_advanced_capture=capture and variant != "advanced_gui_no_capture", capture_matches_same_gui_png=matched)
