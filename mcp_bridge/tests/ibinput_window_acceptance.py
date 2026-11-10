"""Fixed IbInput Popup acceptance through an authenticated official MCP client."""

import asyncio


async def source(request):
    page = (await request("qkm_query", dict(entity="mapping_sources")))["result"]
    return dict(source_tab_ref=page["items"][0]["source_tab_ref"],
                source_revision=page["source_snapshot"]["revision"])


async def window(request):
    page = (await request("qkm_query", dict(entity="windows", window_role="ibinput_settings")))["result"]
    assert page["private_background_available"] and page["target"]["desktop"].startswith("Qkm")
    assert not page["foreground_in_instance"] and page["total"] == 1
    return page["items"][0]


async def exercise(request, gui, grant, capture_gui):
    role = "ibinput_settings"
    initial = (await request("qkm_get_state"))["result"]
    binding = await source(request)
    args = dict(source_tab_ref=binding["source_tab_ref"], presentation="private_background", window_role=role)
    assert not (await window(request)).get("qt_visible", False)
    await request("qkm_open_mapping_settings", args, error="permission_denied")
    await request("qkm_close_mapping_settings", {**args, "window_ref": "b" * 64}, error="permission_denied")
    await request("qkm_capture_window", {**binding, "role": role, "window_ref": "b" * 64}, error="permission_denied")

    await grant("grant_show", 131)
    binding = await source(request)
    args["source_tab_ref"] = binding["source_tab_ref"]
    await gui("ibinput_open")
    visible = await window(request)
    assert visible["qt_visible"] and visible["native_visible"]
    await request("qkm_close_mapping_settings", {**args, "window_ref": visible["window_ref"]}, error="editor_not_owned")
    await request("qkm_open_mapping_settings", args, error="context_busy")
    await request("qkm_capture_window", {**binding, "role": role, "window_ref": visible["window_ref"]}, error="permission_denied")
    await gui("ibinput_close")
    opened = (await request("qkm_open_mapping_settings", args))["result"]
    assert opened["opened"] and opened["role"] == role and not opened["activation_requested"]
    assert opened["window"]["window_ref"] != visible["window_ref"]
    closed = (await request("qkm_close_mapping_settings", {**args, "window_ref": opened["window"]["window_ref"]}))["result"]
    assert closed["closed"] and not closed["window"]["qt_visible"] and not closed["window"]["native_visible"]

    await grant("grant_capture", 387)
    binding = await source(request)
    args["source_tab_ref"] = binding["source_tab_ref"]
    before = (await request("qkm_get_state"))["result"]
    context = (await request("qkm_query", dict(entity="demo_context")))["result"]
    pending = (await request("qkm_preview_changes", dict(action="ibinput.preferences.update",
        preferences_revision=before["ibinput"]["preferences_revision"],
        preferred_backend=before["ibinput"]["preferences"]["preferred_backend"])))["result"]

    async def capture(reference, owned):
        result = (await request("qkm_capture_window", {**binding, "role": role, "window_ref": reference}))["result"]
        assert result["settings_scope"] == "live_ibinput_preferences_and_passive_status"
        assert result.get("settings_editor_unchanged") is owned
        return result

    gui_window = await gui("ibinput_open")
    visible = await window(request)
    await capture(visible["window_ref"], None)
    await capture_gui(gui_window["window"])
    await gui("ibinput_close")
    opened = (await request("qkm_open_mapping_settings", args))["result"]
    reference = opened["window"]["window_ref"]
    for _ in range(2):
        image = await capture(reference, True)
        assert image["preferences_revision"] == before["ibinput"]["preferences_revision"]
        await asyncio.sleep(.1)
    await request("qkm_open_mapping_settings", {**args, "source_tab_ref": "a" * 64 + ":source:0"}, error="stale_source")
    await request("qkm_close_mapping_settings", {**args, "window_ref": visible["window_ref"]}, error="stale_window")
    await request("qkm_close_mapping_settings", {**args, "window_ref": reference})
    await request("qkm_capture_window", {**binding, "role": role, "window_ref": reference}, error="stale_window")
    after = (await request("qkm_get_state"))["result"]
    assert before["configuration"] == after["configuration"]
    assert before["ibinput"]["preferences_revision"] == after["ibinput"]["preferences_revision"]
    assert before["ibinput"]["preferences"] == after["ibinput"]["preferences"]
    assert not after["ibinput"]["connected"] and after["ibinput"]["last_result"] is None
    assert (await request("qkm_query", dict(entity="demo_context")))["result"] == context
    applied = (await request("qkm_apply_changes", dict(preview_id=pending["preview_id"])))["result"]
    assert applied["status"] == "applied" and applied["result"]["modified"] is False

    # A fresh authenticated connection cannot inherit another connection's open window.
    opened = (await request("qkm_open_mapping_settings", args))["result"]
    await grant(None, 387)
    await request("qkm_close_mapping_settings", {**args, "window_ref": opened["window"]["window_ref"]}, error="editor_not_owned")
    await gui("ibinput_close")
    context_cases = []
    for _ in range(2):
        binding = await source(request)
        args["source_tab_ref"] = binding["source_tab_ref"]
        opened = (await request("qkm_open_mapping_settings", args))["result"]
        await gui("ibinput_preferences_live")
        current = await window(request)
        changed_binding = await source(request)
        source_changed = changed_binding != binding
        await request("qkm_close_mapping_settings", {**args, "window_ref": current["window_ref"]},
                      error="stale_source" if source_changed else "editor_draft_changed")
        binding = changed_binding
        await capture(current["window_ref"], None if source_changed else False)
        await gui("ibinput_close")
        changed = (await request("qkm_get_state"))["result"]
        assert changed["ibinput"]["preferences_revision"] != before["ibinput"]["preferences_revision"]
        context_cases.append("source_and_preferences_changed" if source_changed else "preferences_changed_on_dirty_source")
        restore = (await request("qkm_preview_changes", dict(action="ibinput.preferences.update",
            preferences_revision=changed["ibinput"]["preferences_revision"], **before["ibinput"]["preferences"])))["result"]
        await request("qkm_apply_changes", dict(preview_id=restore["preview_id"]))
    assert (await request("qkm_get_state"))["result"]["ibinput"]["preferences"] == initial["ibinput"]["preferences"]
    return dict(fixed_popup=True, independent_gui=True, no_show_denied=True, no_capture_denied=True,
                passive_preview_preserved=True, old_connection_close_denied=True, changed_preferences_close_denied=True, context_cases=context_cases)


async def demo_popup(request):
    before = (await request("qkm_get_state"))["result"]
    binding = await source(request)
    args = dict(source_tab_ref=binding["source_tab_ref"], presentation="private_background", window_role="ibinput_settings")
    opened = (await request("qkm_open_mapping_settings", args))["result"]
    image = (await request("qkm_capture_window", {**binding, "role": "ibinput_settings",
        "window_ref": opened["window"]["window_ref"]}))["result"]
    assert image["settings_editor_unchanged"] is True
    assert image["preferences_revision"] == before["ibinput"]["preferences_revision"]
    closed = (await request("qkm_close_mapping_settings", {**args, "window_ref": opened["window"]["window_ref"]}))["result"]
    assert closed["closed"] and not closed["window"]["qt_visible"]
    after = (await request("qkm_get_state"))["result"]
    assert after["ibinput"]["preferences_revision"] == before["ibinput"]["preferences_revision"]
    assert after["configuration"] == before["configuration"]
    assert not after["ibinput"]["connected"] and after["ibinput"]["last_result"] is None


async def read_only_exercise(request, gui, grant, capture_gui):
    before = (await request("qkm_get_state"))["result"]
    await grant("grant_capture", 257)
    binding = await source(request)
    args = dict(source_tab_ref=binding["source_tab_ref"], presentation="private_background", window_role="ibinput_settings")
    visible = await gui("ibinput_open")
    reference = (await window(request))["window_ref"]
    await request("qkm_capture_window", {**binding, "role": "ibinput_settings", "window_ref": reference})
    await capture_gui(visible["window"])
    await request("qkm_open_mapping_settings", args, error="permission_denied")
    await request("qkm_close_mapping_settings", {**args, "window_ref": reference}, error="permission_denied")
    await gui("ibinput_close")
    await grant("grant_show", 385)
    binding = await source(request)
    args["source_tab_ref"] = binding["source_tab_ref"]
    opened = (await request("qkm_open_mapping_settings", args))["result"]
    reference = opened["window"]["window_ref"]
    await request("qkm_capture_window", {**binding, "role": "ibinput_settings", "window_ref": reference})
    await request("qkm_close_mapping_settings", {**args, "window_ref": reference})
    await request("qkm_preview_changes", dict(action="ibinput.preferences.update",
        preferences_revision=before["ibinput"]["preferences_revision"], preferred_backend="razer"), error="permission_denied")
    after = (await request("qkm_get_state"))["result"]
    assert after["configuration"] == before["configuration"]
    assert after["ibinput"]["preferences_revision"] == before["ibinput"]["preferences_revision"]
    assert not after["ibinput"]["connected"] and after["ibinput"]["last_result"] is None
    assert await source(request) == binding
    return dict(capture_without_show=True, display_without_configure=True, configure_still_denied=True)
