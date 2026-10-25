"""Owned stock GUI sources and bounded common additions through the official SDK."""

import argparse
import asyncio
import hashlib
import json
import os
import re
import shutil
import subprocess
from pathlib import Path

import win32api
from mcp import Client

from test_product_interop import Product, connection_pending
from managed_test import reject_nested_cache
from test_sdk_interop import BUSINESS_CAPABILITIES, ROOT, business_state, call, connection, elevation, integrity, parameters
from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.windows import identity


def record(output, name, value):
    (output / (name + ".json")).write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")


def visible_rows(gui):
    return [(row["original"], row["mapping"]) for index, row in enumerate(gui["mapping_rows"])
            if index not in gui["separator_rows"]]


def source_rows(page):
    return [(row["fields"]["KeyMapData_OriginalKeys"], row["fields"]["KeyMapData_MappingKeys"])
            for row in page["items"]]


async def query(client, output, name, arguments, error=None):
    result = await call(client, "qkm_query", arguments, error=error)
    record(output, name, {"arguments": arguments, "response": result})
    if error:
        assert result["ok"] is False and result["error"] == error, result
    else:
        assert result["ok"], result
    return result.get("result")


async def sources(client, output, label):
    listing = await query(client, output, label + "_sources_0", {"entity": "mapping_sources", "limit": 1})
    revision = listing["source_snapshot"]["revision"]
    items = list(listing["items"])
    next_offset = listing["next_offset"]
    while next_offset is not None:
        page = await query(client, output, label + "_sources_" + str(next_offset), {
            "entity": "mapping_sources", "source_revision": revision, "offset": next_offset, "limit": 1})
        assert page["source_snapshot"] == listing["source_snapshot"] and page["total"] == listing["total"]
        items += page["items"]
        next_offset = page["next_offset"]
    assert len(items) == listing["total"]
    rows = []
    for index, source in enumerate(items):
        arguments = {"entity": "mappings", "source_revision": revision, "source_tab_ref": source["source_tab_ref"], "limit": 1}
        page = await query(client, output, label + "_rows_" + str(index) + "_0", arguments)
        assert page["source"] == source and page["total"] == source["row_count"]
        all_rows = list(page["items"])
        next_offset = page["next_offset"]
        while next_offset is not None:
            following = await query(client, output, label + "_rows_" + str(index) + "_" + str(next_offset),
                                    dict(arguments, offset=next_offset))
            assert following["source"] == source and following["source_snapshot"] == listing["source_snapshot"]
            all_rows += following["items"]
            next_offset = following["next_offset"]
        assert len(all_rows) == page["total"]
        rows.append(dict(page, items=all_rows, next_offset=None))
    snapshot = listing["source_snapshot"]
    assert snapshot["reference_scope"] == "source_snapshot_only" and snapshot["scope"] == "current_memory_sources"
    assert snapshot["read_basis"] == "pristine_idle"
    assert all(source["origin"] == "source_memory" for source in items)
    refs = [row["source_ref"] for page in rows for row in page["items"]]
    assert len(refs) == len(set(refs))
    for index, page in enumerate(rows):
        assert [row["source_row"] for row in page["items"]] == list(range(items[index]["row_count"]))
        assert all(row["source_tab_ref"] == items[index]["source_tab_ref"] for row in page["items"])
    return {"snapshot": snapshot, "sources": items, "rows": rows}


async def stale(client, output, label, previous):
    revision = previous["snapshot"]["revision"]
    await query(client, output, label + "_stale_listing", {"entity": "mapping_sources", "source_revision": revision}, "stale_source")
    await query(client, output, label + "_stale_rows", {"entity": "mappings", "source_revision": revision,
        "source_tab_ref": previous["sources"][0]["source_tab_ref"]}, "stale_source")


async def run(args):
    assert elevation(win32api.GetCurrentProcess()) == 0 and integrity(win32api.GetCurrentProcess()) == 0x2000
    checks = []
    def passed(name):
        checks.append(name)
        print("PASS", name, flush=True)
    product, store = Product(args.executable, args.output), None
    try:
        await product.started()
        # Establish the GUI business baseline before enabling or reading MCP.
        for action, expected in (("ReadTab1", [("A", "B"), ("G", "H"), ("E", "F")]),
                                 ("ReadTab2", [("C", "D"), ("E", "F")]),
                                 ("ReadCommon", [("E", "F")])):
            gui = await product.action(action)
            assert gui["configuration_display"] == "McpSaved" and visible_rows(gui) == expected, gui
        baseline = await product.action("ReadTab1")
        assert baseline["mapping_button_text"] == "映射开始"
        passed("independent real GUI: two normal Tabs and one common source with repeated common display rows")
        await product.action("OpenAdvanced")
        initial = await product.action("OpenMcp")
        assert initial["enabled"] is False and initial["pair_count"] == 0 and initial["discovery"] == ""
        enabled = await product.action("Enable")
        discovery = Path(enabled["discovery"])
        own = identity(os.getpid())
        binding = hashlib.sha256(("QKeyMapper-MCP-App/v1\0" + args.executable.as_posix() + "\0" + str(own.session)).encode()).hexdigest()
        assert discovery.parent.name == binding and discovery.name == "instance.json"
        await product.wait(discovery.is_file)
        store = CredentialStore(discovery, args.executable, own.session)
        assert store.load() is None
        async with Client(parameters(discovery, args.executable, args.output)) as client:
            await call(client, "qkm_connect")
            await connection(client, "pairing_required")
            await call(client, "qkm_request_pairing")
            await connection_pending(client)
            await product.action("ApproveRead", own.executable)
            connected = await connection(client, "connected")
            assert connected["scopes"] == 1
            capabilities = (await call(client, "qkm_get_capabilities"))["result"]
            by_id = {item["id"]: item for item in capabilities["business_capabilities"]}
            assert set(by_id) == BUSINESS_CAPABILITIES
            assert by_id["source.read"]["read_only"] is True
            await product.action("CloseMcp")
            before = await product.action("ReadTab1")
            ini = args.executable.parent / "keymapdata.ini"
            disk = ini.read_bytes()
            state = (await call(client, "qkm_get_state"))["result"]
            first = await sources(client, args.output, "initial")
            assert first["snapshot"]["common_feature_enabled"] is True
            assert [source["kind"] for source in first["sources"]] == ["normal", "normal", "common"]
            assert [source["row_count"] for source in first["sources"]] == [2, 1, 1]
            assert first["sources"][2]["internal_name"] == "__QKM_COMMON__" and first["sources"][2]["attached_to_ui"] is True
            assert [source_rows(page) for page in first["rows"]] == [[("A", ["B"]), ("G", ["H"])], [("C", ["D"])], [("E", ["F"])]]
            assert first["rows"][0]["items"][0]["fields"]["KeyMapData_Note"] == "源行甲"
            assert first["rows"][0]["items"][1]["fields"]["KeyMapData_Note"] == "源行乙"
            assert first["snapshot"]["configuration_selection"] == state["configuration"]
            assert first["snapshot"]["mapping_load_context"] == state["mapping_load_context"]
            assert state["run"]["stop_completion"] == "unobserved" and state["business_revision"] is None
            for label, arguments, expected in (
                    ("fresh_offset", {"entity": "mapping_sources", "offset": 1}, "invalid_arguments"),
                    ("listing_target", {"entity": "mapping_sources", "source_tab_ref": first["sources"][0]["source_tab_ref"]}, "invalid_arguments"),
                    ("missing_revision", {"entity": "mappings"}, "invalid_arguments"),
                    ("missing_target", {"entity": "mappings", "source_revision": first["snapshot"]["revision"]}, "invalid_arguments"),
                    ("reference_mismatch", {"entity": "mappings", "source_revision": "0" * 64,
                        "source_tab_ref": first["sources"][0]["source_tab_ref"]}, "stale_source")):
                await query(client, args.output, "arguments_" + label, arguments, expected)
            passed("source argument dependencies and mismatched revision return formal tool errors")
            assert await sources(client, args.output, "repeated") == first
            assert business_state((await call(client, "qkm_get_state"))["result"]) == business_state(state) and ini.read_bytes() == disk
            after = await product.action("ReadMapping")
            for field in ("configuration_display", "mapping_rows", "separator_rows", "mapping_button_text", "stop_shortcut_edit"):
                assert before[field] == after[field], field
            record(args.output, "readonly", {"state": state, "before": before, "after": after, "ini_unchanged": True})
            passed("formal source pagination: non-current normal source, common source once, distinct references and exact read-only invariance")
            # Viewing a different Tab must not retarget source references.
            await product.action("ReadTab2")
            assert await sources(client, args.output, "other_view") == first
            passed("source references do not follow the GUI current Tab")
            moved_gui = await product.action("MoveSecondUp")
            assert visible_rows(moved_gui) == [("G", "H"), ("A", "B"), ("E", "F")], moved_gui
            await stale(client, args.output, "move", first)
            moved = await sources(client, args.output, "moved")
            assert source_rows(moved["rows"][0]) == [("G", ["H"]), ("A", ["B"])]
            assert source_rows(moved["rows"][2]) == [("E", ["F"])]
            passed("real GUI Move Up changes source order and rejects old listing and row targets")
            restored_gui = await product.action("RestoreFirstRows")
            assert visible_rows(restored_gui) == [("A", "B"), ("G", "H"), ("E", "F")]
            moved_again_gui = await product.action("MoveSecondUp")
            assert visible_rows(moved_again_gui) == visible_rows(moved_gui)
            await stale(client, args.output, "same_content_move", moved)
            moved_again = await sources(client, args.output, "moved_again")
            assert source_rows(moved_again["rows"][0]) == source_rows(moved["rows"][0])
            assert moved_again["snapshot"] != moved["snapshot"]
            passed("same-content reorder round trip invalidates old references even while already dirty")
            reload_gui = await product.action("ReloadCurrent")
            assert visible_rows(reload_gui) == [("A", "B"), ("G", "H"), ("E", "F")]
            await stale(client, args.output, "reload", moved_again)
            reloaded = await sources(client, args.output, "reloaded")
            assert source_rows(reloaded["rows"][0]) == source_rows(first["rows"][0])
            second_gui = await product.action("SelectSecond")
            assert visible_rows(second_gui) == [("C", "D")]
            await stale(client, args.output, "second", reloaded)
            second = await sources(client, args.output, "second")
            assert second["snapshot"]["common_feature_enabled"] is False
            assert [source["kind"] for source in second["sources"]] == ["normal", "common"]
            assert [source_rows(page) for page in second["rows"]] == [[("C", ["D"])], [("E", ["F"])]]
            common = second["sources"][1]
            assert common["attached_to_ui"] is False and common["ui_index"] is None
            assert second["snapshot"]["mapping_load_context"]["group"] == "McpSecond"
            passed("GUI reload and profile switch invalidate references; detached common memory remains distinct from last-load provenance")
            invalid_gui = await product.action("SelectInvalid")
            assert visible_rows(invalid_gui) == []
            await query(client, args.output, "failed_load", {"entity": "mapping_sources"}, "context_busy")
            await query(client, args.output, "failed_load_invalid_arguments", {"entity": "mapping_sources", "offset": 1}, "invalid_arguments")
            invalid_state = (await call(client, "qkm_get_state"))["result"]
            assert invalid_state["mapping_load_context"]["outcome"] == "invalid_mapping_data"
            assert invalid_state["mapping_load_context"]["group"] is None
            recovered_gui = await product.action("SelectSaved")
            assert visible_rows(recovered_gui) == [("A", "B"), ("G", "H"), ("E", "F")]
            await stale(client, args.output, "recover", second)
            recovered = await sources(client, args.output, "recovered")
            assert [source_rows(page) for page in recovered["rows"]] == [source_rows(page) for page in first["rows"]]
            passed("failed GUI load refuses unsafe source reads, accepted recovery exposes fresh real sources")
            await product.action("OpenAdvanced")
            await product.action("OpenMcp")
            await product.action("Revoke")
            await call(client, "qkm_connect")
            await connection(client, "failed")
            await call(client, "qkm_request_pairing")
            await connection_pending(client)
            await product.action("ApproveRead", own.executable)
            await connection(client, "connected")
            await stale(client, args.output, "new_pair", recovered)
            repaired = await sources(client, args.output, "new_pair")
            assert [source_rows(page) for page in repaired["rows"]] == [source_rows(page) for page in recovered["rows"]]
            passed("new GUI pairing changes reference authorization binding without changing source data")
            await product.action("Revoke")
            await call(client, "qkm_request_pairing")
            await connection_pending(client)
            await product.action("ApproveConfigure", own.executable)
            assert (await connection(client, "connected"))["scopes"] == 3
            await product.action("CloseMcp")
            await product.action("ReadCommon")
            common_before = await sources(client, args.output, "common_before")
            common_source = next(item for item in common_before["sources"] if item["kind"] == "common")
            change = {"source_revision": common_before["snapshot"]["revision"],
                "source_tab_ref": common_source["source_tab_ref"], "original_key": "C", "mapping_key": "B"}
            disk = ini.read_bytes()
            planned = (await call(client, "qkm_preview_changes", change))["result"]
            assert planned["description"]["confirmation_required"] is True
            conflicts = [item for item in planned["description"]["changes"] if item["kind"] == "disable_mapping"]
            assert conflicts and all(item["original_key"] == "C" for item in conflicts)
            await call(client, "qkm_apply_changes", {"preview_id": planned["preview_id"]}, error="confirmation_required")
            refused = await sources(client, args.output, "common_unconfirmed")
            assert refused == common_before and ini.read_bytes() == disk
            rejected = (await call(client, "qkm_get_operation", {"operation_id": planned["preview_id"]}))["result"]
            assert rejected["status"] == "rejected" and rejected["result"]["modified"] is False
            planned = (await call(client, "qkm_preview_changes", change))["result"]
            applied = (await call(client, "qkm_apply_changes", {"preview_id": planned["preview_id"],
                "confirm_conflicts": True}))["result"]
            assert applied["status"] == "applied" and applied["result"]["saved"] is False
            common_after = await sources(client, args.output, "common_applied")
            disabled = {(item["source_tab_ref"], item["source_row"]) for item in conflicts}
            for before_page, after_page in zip(common_before["rows"], common_after["rows"]):
                expected = [dict(row["fields"]) for row in before_page["items"]]
                for row, fields in zip(before_page["items"], expected):
                    if (row["source_tab_ref"], row["source_row"]) in disabled:
                        fields["KeyMapData_Disabled"] = True
                actual = [row["fields"] for row in after_page["items"]]
                if before_page["source"]["kind"] == "common":
                    assert actual[:-1] == expected
                    assert actual[-1]["KeyMapData_OriginalKeys"] == "C" and actual[-1]["KeyMapData_MappingKeys"] == ["B"]
                    assert actual[-1]["KeyMapData_Disabled"] is False
                else:
                    assert actual == expected
            replay = (await call(client, "qkm_apply_changes", {"preview_id": planned["preview_id"],
                "confirm_conflicts": True}))["result"]
            assert replay["replayed"] is True and ini.read_bytes() == disk
            gui_common = await product.action("ReadMapping")
            assert visible_rows(gui_common) == [("E", "F"), ("C", "B")]
            record(args.output, "common_change", {"preview": planned, "rejected": rejected,
                "applied": applied, "replayed": replay, "gui": gui_common, "ini_unchanged": True})
            passed("common addition previews all disabling effects, refuses missing confirmation without partial writes and applies exactly once")
            await product.action("ReadTab1")
            target = next(item for item in common_after["sources"] if item["kind"] == "common")
            await call(client, "qkm_preview_changes", {"source_revision": common_after["snapshot"]["revision"],
                "source_tab_ref": target["source_tab_ref"], "original_key": "D", "mapping_key": "E"}, error="context_busy")
            assert ini.read_bytes() == disk
            passed("a noncurrent common target is refused without an automatic Tab switch")
            await product.action("OpenAdvanced")
            await product.action("OpenMcp")
            await product.action("Revoke")
            await product.action("Disable")
            await product.wait(lambda: not discovery.exists())
            passed("owned pairing revoked and interface disabled")
    finally:
        try:
            if store is not None:
                store.forget()
        finally:
            await product.close()
    record(args.output, "results", {"checks": checks, "desktop": product.desktop_name, "executable": str(args.executable),
        "server_elevated": 1, "controller_elevated": 0, "gsudo_cache": args.gsudo_cache,
        "mapping_started": False, "stop_completion_verified": False})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--test-exe", type=Path, required=True)
    parser.add_argument("--qt-bin", type=Path, required=True)
    parser.add_argument("--gsudo-cache", action="store_true")
    args = parser.parse_args()
    reject_nested_cache(args.gsudo_cache)
    for name in ("executable", "output", "test_exe", "qt_bin"):
        setattr(args, name, getattr(args, name).resolve())
    base = (ROOT.parent / "out/mcp_product").resolve()
    if not __debug__ or not args.executable.is_relative_to(base) or not args.executable.is_file() or not re.fullmatch(
            r"QKeyMapperMcpAcceptance_[a-zA-Z0-9_]+\.exe", args.executable.name):
        parser.error("Select a fresh isolated product under out/mcp_product; assertions must remain enabled.")
    if not args.output.is_relative_to(base) or args.output == base or not args.test_exe.is_relative_to(ROOT.parent / "out/tests/mcp_control"):
        parser.error("Use a new product evidence directory and the built ordinary control maintenance target.")
    assert elevation(win32api.GetCurrentProcess()) == 0
    args.output.mkdir(parents=True, exist_ok=False)
    ini = args.executable.parent / "keymapdata.ini"
    template = (ROOT.parent / "QKeyMapper/tests/mcp_control/product_fixture.ini.example").read_text(encoding="utf-8")
    assert ini.read_text(encoding="utf-8") == template
    contents = template.replace("[General]\n", "[General]\nSettingSelect=McpSaved\n", 1)
    contents += ("\n[McpSaved]\nMappingTable_TabNameList=Tab1, Tab2, Common\n"
        "MappingTable_TabInternalNameList=, , __QKM_COMMON__\nMappingTable_Common_Enabled=true\n"
        "KeyMapData_OriginalKeys=A#G|C|E\nKeyMapData_MappingKeys=B#H|D|F\n"
        "KeyMapData_Note=源行甲#源行乙|第二页|共通保留\nEnableSystemFilterKeyChecked=false\nDisableFilterKeyClickSoundChecked=false\n")
    for group, original, mapping in (("McpSecond", "C", "D"), ("McpInvalid", "MCP_INVALID_ORIGINAL", "B")):
        contents += ("\n[" + group + "]\nMappingTable_TabNameList=Tab1\nKeyMapData_OriginalKeys=" + original
            + "\nKeyMapData_MappingKeys=" + mapping + "\nEnableSystemFilterKeyChecked=false\nDisableFilterKeyClickSoundChecked=false\n")
    ini.write_text(contents, encoding="utf-8")
    env = dict(os.environ, PATH=str(args.qt_bin) + os.pathsep + os.environ["PATH"])
    with (args.output / "fixture_writer.log").open("wb") as log:
        subprocess.run([str(args.test_exe), "--source-fixture", str(ini)], cwd=ROOT.parent, env=env,
                       stdout=log, stderr=log, creationflags=subprocess.CREATE_NO_WINDOW, check=True, timeout=20)
    asyncio.run(run(args))
