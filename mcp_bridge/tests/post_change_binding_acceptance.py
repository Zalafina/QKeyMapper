"""Commit-snapshot references survive history and reject a later real GUI mutation."""

import asyncio
import json
import re
import time

from test_sdk_interop import BUSINESS_CAPABILITIES, business_state


async def exercise(client, request, variant, output, exe):
    assert variant in ("post_binding", "post_binding_read")
    caps = (await request("qkm_get_capabilities"))["result"]["business_capabilities"]
    assert {cap["id"] for cap in caps} == BUSINESS_CAPABILITIES
    for name in ("mapping.add.apply", "mapping.delete.apply"):
        cap = next(cap for cap in caps if cap["id"] == name)
        assert cap["source_binding_result"] == "commit_snapshot_or_null"
    disk = (exe.parent / "keymapdata.ini").read_bytes()
    initial = (await request("qkm_get_state"))["result"]
    listing = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    source = listing["items"][0]
    assert source["attached_to_ui"] and source["row_count"] == 0

    async def preview(original, mapped, snapshot):
        return (await request("qkm_preview_changes", dict(source_revision=snapshot["source_snapshot"]["revision"],
            source_tab_ref=snapshot["source_tab_ref"], original_key=original, mapping_key=mapped)))["result"]

    if variant == "post_binding_read":
        await request("qkm_preview_changes", dict(source_revision=listing["source_snapshot"]["revision"],
            source_tab_ref=source["source_tab_ref"], original_key="A", mapping_key="B"), error="permission_denied")
        await request("qkm_apply_changes", dict(preview_id="a" * 32), error="permission_denied")
        assert business_state((await request("qkm_get_state"))["result"]) == business_state(initial)
        assert (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"] == listing
        assert (exe.parent / "keymapdata.ini").read_bytes() == disk
        return dict(successful_saves=0, read_denied=True)

    def binding(operation, row):
        assert operation["status"] == "applied" and operation["result"]["modified"] is True
        result = operation["result"]
        assert result["saved"] is False and result["started_by_operation"] is False
        assert result["source_binding_status"] == "captured"
        value = result["source_binding_after"]
        assert value["reference_scope"] == "commit_snapshot_only"
        snapshot = value["source_snapshot"]
        revision = snapshot["revision"]
        assert re.fullmatch(r"[0-9a-f]{64}", revision) and snapshot["instance"] == initial["instance"]
        assert value["source_tab_ref"] == revision + ":source:0"
        assert value["source_ref"] == (revision + ":row:0:0" if row else None)
        return value

    async def rows(value, error=None):
        reply = await request("qkm_query", dict(entity="mappings", source_revision=value["source_snapshot"]["revision"],
            source_tab_ref=value["source_tab_ref"]), error=error)
        if error is None:
            assert reply["result"]["source_snapshot"] == value["source_snapshot"]
            return reply["result"]["items"]

    def history(value):
        return {key: item for key, item in value.items() if key not in ("replayed", "query_sampled_at_utc")}

    first = await preview("A", "B", {"source_snapshot": listing["source_snapshot"], **source})
    applied = (await request("qkm_apply_changes", dict(preview_id=first["preview_id"])))["result"]
    added = binding(applied, True)
    owned = await rows(added)
    assert len(owned) == 1 and owned[0]["source_ref"] == added["source_ref"]
    assert owned[0]["fields"]["KeyMapData_OriginalKeys"] == "A"
    assert owned[0]["fields"]["KeyMapData_MappingKeys"] == ["B"]
    delete = (await request("qkm_preview_changes", dict(action="mapping.delete",
        source_revision=added["source_snapshot"]["revision"], source_tab_ref=added["source_tab_ref"],
        source_refs=[added["source_ref"]])))["result"]
    deleted = (await request("qkm_apply_changes", dict(preview_id=delete["preview_id"], confirm_delete=True)))["result"]
    empty = binding(deleted, False)
    assert await rows(empty) == []
    assert empty["source_snapshot"]["revision"] != added["source_snapshot"]["revision"]
    assert history((await request("qkm_apply_changes", dict(preview_id=delete["preview_id"], confirm_delete=True)))["result"]) == history(deleted)
    assert history((await request("qkm_get_operation", dict(operation_id=first["operation_id"])))["result"]) == history(applied)
    await rows(added, error="stale_source")

    second = await preview("C", "D", empty)
    (output / "case.json.mutate-after-apply").write_text("{}", encoding="utf-8")
    late = (await request("qkm_apply_changes", dict(preview_id=second["preview_id"])))["result"]
    historical = binding(late, True)
    marker = output / "case.json.mutated-after-apply"
    until = time.monotonic() + 5
    while not marker.is_file():
        assert time.monotonic() < until, "post-reply GUI deletion not observed"
        await asyncio.sleep(.01)
    assert json.loads(marker.read_text(encoding="utf-8"))["passed"]
    await rows(historical, error="stale_source")
    await request("qkm_open_mapping_editor", dict(source_ref=historical["source_ref"],
        presentation="private_background"), error="stale_source")
    assert history((await request("qkm_get_operation", dict(operation_id=second["operation_id"])))["result"]) == history(late)
    assert history((await request("qkm_apply_changes", dict(preview_id=second["preview_id"])))["result"]) == history(late)
    fresh = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    assert fresh["items"][0]["row_count"] == 0
    assert fresh["source_snapshot"]["revision"] != historical["source_snapshot"]["revision"]
    assert (exe.parent / "keymapdata.ini").read_bytes() == disk
    return dict(successful_saves=0, commit_bindings=3, historical_binding_preserved=True,
        post_reply_gui_change_rejected=True, formal_additions=2, formal_deletions=1, gui_deletions=1)
