"""Formal demo_context checks on a synthetic owned private instance, without mapping input."""

import asyncio
import re
import time

from test_sdk_interop import BUSINESS_CAPABILITIES


async def exercise(client, request, variant, output, exe):
    assert variant in ("demo_context", "demo_context_read")
    caps = (await request("qkm_get_capabilities"))["result"]["business_capabilities"]
    assert {cap["id"] for cap in caps} == BUSINESS_CAPABILITIES
    cap = next(cap for cap in caps if cap["id"] == "demo.context.read")
    assert cap["required_scopes"] == 3 and cap["read_only"] is True
    assert cap["arguments_schema"]["additionalProperties"] is False
    ini = exe.parent / "keymapdata.ini"
    disk = ini.read_bytes()
    initial = (await request("qkm_get_state"))["result"]
    sources = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    args = dict(entity="demo_context")
    if variant == "demo_context_read":
        await request("qkm_query", args, error="permission_denied")
        assert (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"] == sources
        assert ini.read_bytes() == disk
        return dict(successful_saves=0,read_denied=True)

    async def context():
        value = (await request("qkm_query", args))["result"]
        assert value["schema_version"] == 1 and value["entity"] == "demo_context"
        assert re.fullmatch(r"[0-9a-f]{64}", value["context_revision"])
        assert value["reference_scope"] == "non_row_save_footprint_only"
        assert value["mapping_rows_excluded"] is True and value["write_authority"] is False
        assert value["source_snapshot"]["instance"] == initial["instance"]
        return value

    baseline = await context()
    for extra in ({"source_revision": "a" * 64}, {"catalog_revision": "a" * 64}, {"offset": 1}):
        await request("qkm_query", {**args, **extra}, error="invalid_arguments")
    schema = await client.call_tool("qkm_query", {**args, "path": str(ini)})
    assert schema.is_error
    source = sources["items"][0]
    add = (await request("qkm_preview_changes", dict(source_revision=sources["source_snapshot"]["revision"],
        source_tab_ref=source["source_tab_ref"], original_key="A", mapping_key="B")))["result"]
    await request("qkm_apply_changes", dict(preview_id=add["preview_id"]))
    added = await context()
    assert added["context_revision"] == baseline["context_revision"]
    assert added["source_snapshot"]["revision"] != baseline["source_snapshot"]["revision"]
    assert added["source_snapshot"]["configuration_selection"]["has_unsaved_changes"] is True

    async def marker(start, finish):
        (output / ("case.json." + start)).write_text("{}", encoding="utf-8")
        path = output / ("case.json." + finish)
        until = time.monotonic() + 5
        while not path.exists():
            assert time.monotonic() < until, finish
            await asyncio.sleep(.01)
        import json
        assert json.loads(path.read_text(encoding="utf-8"))["passed"]

    await marker("demo-name-change", "demo-name-changed")
    changed = await context()
    assert changed["gui_save_name"] == "DemoContext115Draft"
    assert changed["context_revision"] != baseline["context_revision"]
    assert changed["source_snapshot"]["revision"] == added["source_snapshot"]["revision"], "name change while dirty is not a row revision"
    await marker("demo-name-restore", "demo-name-restored")
    restored = await context()
    assert restored["context_revision"] == baseline["context_revision"]
    try:
        ini.write_bytes(disk + b"\n; demo context 115 external bytes\n")
        disk_changed = await context()
        assert disk_changed["context_revision"] != baseline["context_revision"]
        assert disk_changed["source_snapshot"]["revision"] == restored["source_snapshot"]["revision"]
    finally:
        ini.write_bytes(disk)
    assert (await context())["context_revision"] == baseline["context_revision"]
    listing = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    current = listing["items"][0]
    rows = (await request("qkm_query", dict(entity="mappings", source_revision=listing["source_snapshot"]["revision"],
        source_tab_ref=current["source_tab_ref"])))["result"]["items"]
    assert len(rows) == 1
    opened = (await request("qkm_open_mapping_editor", dict(source_ref=rows[0]["source_ref"], presentation="private_background")))["result"]
    await request("qkm_query", args, error="context_busy")
    await request("qkm_close_mapping_editor", dict(source_ref=rows[0]["source_ref"],
        window_ref=opened["window"]["window_ref"], presentation="private_background"))
    assert (await context())["context_revision"] == baseline["context_revision"]
    delete = (await request("qkm_preview_changes", dict(action="mapping.delete", source_revision=listing["source_snapshot"]["revision"],
        source_tab_ref=current["source_tab_ref"], source_refs=[rows[0]["source_ref"]])))["result"]
    await request("qkm_apply_changes", dict(preview_id=delete["preview_id"], confirm_delete=True))
    empty = await context()
    assert empty["context_revision"] == baseline["context_revision"] and empty["source_snapshot"]["revision"] != restored["source_snapshot"]["revision"]
    after = (await request("qkm_get_state"))["result"]
    assert after["mapping_load_context"] == initial["mapping_load_context"] and after["configuration"]["has_unsaved_changes"]
    assert ini.read_bytes() == disk
    return dict(successful_saves=0,context_stable_after_owned_rows=True,name_and_disk_change_detected=True,schema_errors=1)
