"""Bounded private-desktop GUI/SDK acceptance, called by the warm product fixture."""

import asyncio
import json
import time
from pathlib import Path

from mcp import Client

from test_product_interop import Product, connection_pending
from test_sdk_interop import business_state, call, connection, parameters


def record(output, name, value):
    (output / (name + ".json")).write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")


async def snapshot(client):
    state = (await call(client, "qkm_get_state"))["result"]
    listing = (await call(client, "qkm_query", {"entity": "mapping_sources"}))["result"]
    target = next(source for source in listing["items"]
                  if source["attached_to_ui"] and source["ui_index"] == state["current_tab"]["index"])
    revision = listing["source_snapshot"]["revision"]
    rows = (await call(client, "qkm_query", {"entity": "mappings", "source_revision": revision,
                  "source_tab_ref": target["source_tab_ref"], "limit": 100}))["result"]
    assert rows["next_offset"] is None and rows["total"] <= 8, "Use the bounded synthetic fixture"
    return {"state": state, "listing": listing, "target": target, "revision": revision, "rows": rows["items"]}


def contents(value):
    return [{key: row[key] for key in ("fields", "derived", "lock_state_sample")} for row in value["rows"]]


async def preview(client, source, original="A", mapping="B"):
    return (await call(client, "qkm_preview_changes", {"source_revision": source["revision"],
        "source_tab_ref": source["target"]["source_tab_ref"], "original_key": original,
        "mapping_key": mapping}))["result"]


async def operation(client, identifier):
    return (await call(client, "qkm_get_operation", {"operation_id": identifier}))["result"]


async def pairing(product, client, executable, configure):
    await product.action("OpenAdvanced")
    await product.action("OpenMcp")
    await call(client, "qkm_request_pairing")
    await connection_pending(client)
    await product.action("ApproveConfigure" if configure else "ApproveRead", executable)
    state = await connection(client, "connected")
    assert state["scopes"] == (3 if configure else 1)
    await product.action("CloseMcp")


async def exercise(args, own, store, discovery, passed):
    output = args.output / "mapping_changes"
    output.mkdir()
    product = Product(args.executable, output, lifetime_seconds=420)
    config = parameters(discovery, args.executable, output)
    try:
        await product.started()
        await product.action("OpenAdvanced")
        await product.action("OpenMcp")
        await product.action("Enable")
        async with Client(config) as client:
            await pairing(product, client, own.executable, False)
            await product.action("SelectBlank")
            initial = await snapshot(client)
            arguments = {"source_revision": initial["revision"], "source_tab_ref": initial["target"]["source_tab_ref"],
                         "original_key": "A", "mapping_key": "B"}
            for name, params in (("qkm_preview_changes", arguments),
                    ("qkm_apply_changes", {"preview_id": "0" * 32}),
                    ("qkm_cancel_changes", {"preview_id": "0" * 32})):
                await call(client, name, params, error="permission_denied")
            assert (await operation(client, "0" * 32))["status"] == "unknown"
            assert contents(await snapshot(client)) == []
            passed("mapping writes and cancellation require Configure; unknown result query is successful data")

            gui = await product.action("AddBasicMapping")
            gui_source = await snapshot(client)
            assert gui["mapping_rows"] == [{"original": "A", "mapping": "B"}]
            assert gui_source["state"]["configuration"]["has_unsaved_changes"] is True
            record(output, "gui_baseline", {"gui": gui, "source": gui_source})
            await product.capture(product.window, "gui_added")
            await product.action("SelectSaved")
            await product.action("SelectBlank")
            assert contents(await snapshot(client)) == []
            await product.action("OpenAdvanced")
            await product.action("OpenMcp")
            await product.action("Revoke")
            await pairing(product, client, own.executable, True)
            initial = await snapshot(client)
            ini = args.executable.parent / "keymapdata.ini"
            disk = ini.read_bytes()
            created = await preview(client, initial)
            assert created["description"]["modified"] is False and created["expires_in_ms"] == 120000
            assert created["description"]["confirmation_required"] is False
            assert business_state((await snapshot(client))["state"]) == business_state(initial["state"])
            cancelled = (await call(client, "qkm_cancel_changes", {"preview_id": created["preview_id"]}))["result"]
            assert cancelled["status"] == "cancelled"
            await call(client, "qkm_apply_changes", {"preview_id": created["preview_id"]}, error="stale_preview")
            assert (await operation(client, created["preview_id"]))["status"] == "cancelled"
            passed("preview and cancel leave source, dirty and disk unchanged; consumed cancel cannot apply")

            replaced = await preview(client, initial)
            created = await preview(client, initial)
            await call(client, "qkm_apply_changes", {"preview_id": replaced["preview_id"]}, error="preview_superseded")
            applied = (await call(client, "qkm_apply_changes", {"preview_id": created["preview_id"]}))["result"]
            actual = await snapshot(client)
            mcp_gui = await product.action("ReadMapping")
            assert applied["status"] == "applied" and applied["replayed"] is False
            assert applied["result"]["modified"] is True and applied["result"]["saved"] is False
            assert applied["result"]["started_by_operation"] is False
            assert contents(actual) == contents(gui_source), "Independent GUI and MCP business rows differ"
            assert actual["rows"][0]["fields"]["KeyMapData_OriginalKeys"] == "A"
            assert actual["rows"][0]["fields"]["KeyMapData_MappingKeys"] == ["B"]
            assert actual["state"]["configuration"] == gui_source["state"]["configuration"]
            assert business_state(actual["state"])["run"] == business_state(gui_source["state"])["run"]
            assert mcp_gui["mapping_rows"] == gui["mapping_rows"]
            assert mcp_gui["mapping_button_text"] == gui["mapping_button_text"]
            assert ini.read_bytes() == disk
            record(output, "gui_mcp_equivalence", {"gui": gui_source, "mcp": actual, "mcp_gui": mcp_gui, "operation": applied,
                   "excluded_metadata": ["source references bind instance/pair/epoch/load generation; GUI input collected separately"],
                   "ini_unchanged_by_mcp": True})
            await product.capture(product.window, "mcp_added")
            passed("official MCP A to B matches independent GUI full fields, derived keys, dirty and stopped state")

            replay = (await call(client, "qkm_apply_changes", {"preview_id": created["preview_id"]}))["result"]
            assert replay["replayed"] is True and contents(await snapshot(client)) == contents(actual)
            await call(client, "qkm_disconnect")
            await connection(client, "disconnected")
            await call(client, "qkm_connect")
            await connection(client, "connected")
            assert (await operation(client, created["preview_id"]))["status"] == "applied"
            passed("repeated apply cannot duplicate a row; known operation survives a normal disconnect/reconnect")

            stale = await preview(client, actual, "C", "D")
            await product.action("AddBasicMapping")
            before = await snapshot(client)
            assert len(before["rows"]) == 2 and before["rows"][1]["fields"]["KeyMapData_Disabled"] is True
            await call(client, "qkm_apply_changes", {"preview_id": stale["preview_id"]}, error="stale_source")
            after = await snapshot(client)
            assert contents(after) == contents(before) and after["revision"] == before["revision"]
            failure = await operation(client, stale["preview_id"])
            assert failure["status"] == "rejected" and failure["result"]["modified"] is False
            assert ini.read_bytes() == disk
            record(output, "stale_after_gui", {"before": before, "after": after, "operation": failure})
            passed("GUI edit invalidates a full-content preview; rejection has zero partial mapping/disk changes")

            stale = await preview(client, after, "C", "D")
            await product.action("SelectSecond")
            before = await snapshot(client)
            await call(client, "qkm_apply_changes", {"preview_id": stale["preview_id"]}, error="stale_source")
            assert (await snapshot(client))["revision"] == before["revision"]
            await product.action("OpenAdvanced")
            await product.action("OpenMcp")
            await product.action("Revoke")
            await pairing(product, client, own.executable, True)
            assert (await operation(client, stale["preview_id"]))["status"] == "unknown"
            passed("configuration switch invalidates preview; a new pairing cannot inspect or reuse old operations")
            await product.action("SelectBlank")
            created = await preview(client, await snapshot(client))
            expires = time.monotonic() + created["expires_in_ms"] / 1000 + 0.15
            print("WAIT real preview expiry; no mapping is running", flush=True)
            while time.monotonic() < expires:
                await asyncio.sleep(max(0, min(1, expires - time.monotonic())))
            await call(client, "qkm_apply_changes", {"preview_id": created["preview_id"]}, error="preview_expired")
            assert contents(await snapshot(client)) == []
            expired = await operation(client, created["preview_id"])
            assert expired["status"] == "cancelled" and expired["error"]["code"] == "preview_expired"
            record(output, "expired", expired)
            passed("real 120 second expiry rejects apply and retains the cancelled historical result")
            await product.action("OpenAdvanced")
            await product.action("OpenMcp")
            await product.action("Revoke")
            await product.action("Disable")
            await product.wait(lambda: not discovery.exists())
        record(output, "result", {"desktop": product.desktop_name, "executable": str(args.executable),
               "scopes": "Read+Configure only", "mapping_started": False, "checks": "see parent results"})
    finally:
        store.forget()
        await product.close()
