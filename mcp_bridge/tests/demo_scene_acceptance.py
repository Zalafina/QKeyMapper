"""Two real formal scenes on a synthetic owned private instance; channel delivery is separate."""

import re

from test_sdk_interop import BUSINESS_CAPABILITIES, business_state


async def exercise(client, request, variant, output, exe, *, views=("item_setup", "item_setup")):
    assert variant in ("demo_sequence", "demo_sequence_read")
    tool = next(tool for tool in (await client.list_tools()).tools if tool.name == "qkm_demo_mapping")
    metadata = tool.model_dump(by_alias=True)
    assert metadata["annotations"]["readOnlyHint"] is False and metadata["annotations"]["destructiveHint"] is True
    assert metadata["annotations"]["idempotentHint"] is False and metadata["inputSchema"]["additionalProperties"] is False
    assert set(metadata["inputSchema"]["properties"]) == {"original_key", "mapping_key", "view"}
    assert metadata["inputSchema"]["properties"]["view"]["default"] == "item_setup"
    assert set(metadata["inputSchema"]["properties"]["view"]["enum"]) == {"main", "item_setup", "mapping_advanced"}
    caps = (await request("qkm_get_capabilities"))["result"]["business_capabilities"]
    assert {cap["id"] for cap in caps} == BUSINESS_CAPABILITIES
    disk = (exe.parent / "keymapdata.ini").read_bytes()
    initial = business_state((await request("qkm_get_state"))["result"])
    sources = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    for extra in (dict(pid=123), dict(hwnd=123), dict(path="unused"), dict(source_ref="a" * 64),
                  dict(presentation="Default"), dict(original_key="AA"), dict(original_key="a"), dict(original_key=1)):
        reply = await client.call_tool("qkm_demo_mapping", dict(original_key="A", mapping_key="B") | extra)
        assert reply.is_error and not any(item.type == "image" for item in reply.content)
    if variant == "demo_sequence_read":
        value = await request("qkm_demo_mapping", dict(original_key="A", mapping_key="B", view=views[0]), error="permission_denied")
        assert value["result"]["quarantined"] is False and not value["result"]["receipt_available"]
        assert business_state((await request("qkm_get_state"))["result"]) == initial
        assert (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"] == sources
        assert (exe.parent / "keymapdata.ini").read_bytes() == disk
        return dict(successful_saves=0, scenes=0, read_denied=True, schema_errors=8)
    ready = (await request("qkm_get_demo_state"))["result"]
    assert ready["ready"] and ready["demo_session"]["generation"] == 0
    baseline = (await request("qkm_query", dict(entity="demo_context")))["result"]["context_revision"]
    scene_ids = set()
    for generation, (original, mapped) in enumerate((("A", "B"), ("C", "D")), 1):
        view = views[generation - 1]
        arguments = dict(original_key=original, mapping_key=mapped)
        if view != "item_setup":
            arguments["view"] = view
        value = (await request("qkm_demo_mapping", arguments))["result"]
        scene = value["demo_scene"]
        assert re.fullmatch(r"[0-9a-f]{32}", scene["scene_id"]) and scene["scene_id"] not in scene_ids
        scene_ids.add(scene["scene_id"])
        assert scene["generation"] == generation and scene["stage"] == "completed" and scene["reset_verified"]
        assert scene["original_key"] == original and scene["mapping_key"] == mapped
        assert value["target"] == ready["target"] and value["role"] == view and scene["view"] == view
        if view == "item_setup":
            assert value["edit_target"]["source_ref"].startswith(value["source_revision"] + ":row:0:")
        elif view == "mapping_advanced":
            assert value["settings_editor_unchanged"] is True
            assert value["settings_scope"] == "live_loaded_settings_not_committed_by_source_revision"
            assert value["source_context"]["revision"] == value["source_revision"]
            assert "edit_target" not in value
            window = (await request("qkm_query", dict(entity="windows", window_role="mapping_advanced")))["result"]["items"][0]
            assert not window["qt_visible"] and not window["native_visible"]
        assert scene["cleanup"]["verified"] and scene["cleanup"]["dirty_retained"] and scene["cleanup"]["saved"] is False
        after = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
        assert after["items"][0]["row_count"] == after["items"][1]["row_count"] == 0
        assert after["source_snapshot"] == scene["cleanup"]["source_snapshot"]
        assert value["source_revision"] != after["source_snapshot"]["revision"]
        context = (await request("qkm_query", dict(entity="demo_context")))["result"]
        assert context["context_revision"] == baseline == scene["cleanup"]["context_revision"]
        observation = (await request("qkm_get_demo_state"))["result"]
        assert observation["ready"] is False and observation["reason"] == "demo_baseline_dirty"
        assert observation["demo_session"]["receipt_available"] and not observation["demo_session"]["quarantined"]
        assert observation["demo_session"]["generation"] == generation
        state = business_state((await request("qkm_get_state"))["result"])
        assert state["mapping_load_context"] == initial["mapping_load_context"] and state["configuration"]["has_unsaved_changes"]
        for operation_id in scene["operations"].values():
            operation = (await request("qkm_get_operation", dict(operation_id=operation_id)))["result"]
            assert operation["status"] == "applied" and operation["result"]["saved"] is False
        assert (exe.parent / "keymapdata.ini").read_bytes() == disk
    return dict(successful_saves=0, scenes=2, native_images=2, full_scene_cleanup_verified=True, schema_errors=8)
