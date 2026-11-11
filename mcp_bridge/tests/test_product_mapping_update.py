"""Selected stock Release mapping.update acceptance; own private runtime and GUI pairings only."""

import argparse
import asyncio
from contextlib import closing
from copy import deepcopy
import importlib
import json
import os
from pathlib import Path
from unittest.mock import patch
import winreg

from mcp import Client, StdioServerParameters
import win32api

from test_product_interop import Product, connection_pending
from test_product_persistence import filter_keys
from test_sdk_interop import call, connection, integrity
from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.windows import identity

ROOT = Path(__file__).resolve().parents[2]
CASES = ("normal", "cold", "normal_clear_up", "disabled", "common", "common_conflict", "complex", "read")
FLAG_CASES = ("flag_normal", "flag_cold", "flag_disabled", "flag_common", "flag_hidden", "flag_conflict", "flag_common_conflict", "flag_read")

GUARD_CASES = ("key_normal_conflict", "key_common_priority")


async def run(args):
    phases = args.case or CASES
    for prefix in ("", "flag_"):
        if prefix + "cold" in phases:
            assert prefix + "normal" in phases and phases.index(prefix + "normal") < phases.index(prefix + "cold")
    assert args.executable.is_relative_to(ROOT / "out/mcp_product")
    assert args.output.is_relative_to(ROOT / "out/mcp_product")
    assert args.gui_baseline.is_relative_to(ROOT / "out")
    if any(phase in FLAG_CASES for phase in phases):
        assert args.disabled_gui_baseline and args.disabled_gui_baseline.is_relative_to(ROOT / "out")
    args.output.mkdir(parents=True, exist_ok=False)
    ini = args.executable.parent / "keymapdata.ini"
    seed = (ROOT / "QKeyMapper/tests/sendinput_lifecycle/mapping_edit_fixture.ini.example").read_bytes()
    own = identity(os.getpid())
    with closing(win32api.OpenProcess(0x1000, False, own.pid)) as handle:
        assert integrity(handle) == 8192
    calls, peers, completed = [], [], []
    warm = {}
    stdio = importlib.import_module("mcp.client.stdio")
    spawn = stdio._create_platform_compatible_process

    async def observed_spawn(*values, **keywords):
        process = await spawn(*values, **keywords)
        try:
            peer = identity(process.pid)
            assert own.same_login(peer) and Path(peer.executable) == args.bridge_python
            with closing(win32api.OpenProcess(0x1000, False, peer.pid)) as handle:
                assert integrity(handle) == 8192
            peers.append(dict(pid=peer.pid, created=str(peer.created), executable=peer.executable, integrity=8192))
            (args.output / "bridge-peers.json").write_text(json.dumps(peers, indent=2), encoding="utf-8")
        except BaseException:
            process.terminate()
            await process.wait()
            raise
        return process

    async def request(client, phase, name, arguments=None, *, error=None):
        value = await call(client, name, arguments, error=error)
        calls.append(dict(phase=phase, tool=name, arguments=arguments, value=value))
        (args.output / "sdk-calls.json").write_text(json.dumps(calls, ensure_ascii=False, indent=2), encoding="utf-8")
        return value

    async def snapshot(client, phase):
        listing = (await request(client, phase, "qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
        assert listing["next_offset"] is None
        revision = listing["source_snapshot"]["revision"]
        result, references = [], []
        for source in listing["items"]:
            rows = (await request(client, phase, "qkm_query", dict(entity="mappings", source_revision=revision,
                source_tab_ref=source["source_tab_ref"], limit=100)))["result"]
            assert rows["next_offset"] is None
            clean = {key: value for key, value in source.items() if key not in ("source_tab_ref", "row_count")}
            clean["rows"] = [{key: value for key, value in row.items()
                if key not in ("source_ref", "source_tab_ref", "source_row")} for row in rows["items"]]
            result.append(clean)
            references.append(rows["items"])
        return result, references, revision

    async def session(product, phase, *, read_only=False, cold=False):
        flag = phase in FLAG_CASES
        variant = phase[5:] if flag else phase
        assert (await product.action("OpenAdvanced"))["mapping_button_text"] == "映射开始"
        management = await product.action("OpenMcp")
        assert management["pair_count"] == 0 and not management["enabled"]
        enabled = await product.action("Enable")
        discovery = Path(enabled["discovery"])
        store = CredentialStore(discovery, args.executable, own.session, persistent=True)
        assert store.load() is None, "Do not replace an existing credential."
        parameters = StdioServerParameters(command=str(args.bridge_python), args=["-I", "-B", "-X", "utf8", "-m",
            "qkeymapper_mcp", "--persistent-target", "--executable", str(args.executable)], cwd=str(product.output))
        try:
            async with Client(parameters, cache=None) as client:
                await request(client, phase, "qkm_connect")
                await connection(client, "pairing_required")
                await request(client, phase, "qkm_request_pairing")
                await connection_pending(client)
                await product.action("ApproveRead" if read_only else "ApproveConfigure", str(args.bridge_python))
                authenticated = await connection(client, "connected")
                assert authenticated["scopes"] == (1 if read_only else 3)
                (product.output / "authenticated.json").write_text(json.dumps(authenticated), encoding="utf-8")
                await product.action("CloseMcp")
                if variant.startswith("common"):
                    selected = await product.action("ReadCommon")
                    original = "C" if flag and variant == "common_conflict" else "F13"
                    assert selected["mapping_rows"][0] == dict(original=original, mapping="B")
                before, refs, revision = await snapshot(client, phase)
                # UI attachment does not identify the currently selected source.
                kind = "common" if variant.startswith("common") else "normal"
                index = next(index for index, source in enumerate(before) if source["attached_to_ui"] and source["kind"] == kind)
                target = refs[index][0]["source_ref"]
                original_ini = ini.read_bytes()
                if read_only:
                    fields = dict(disabled=True) if flag else dict(original_key="E", mapping_key="C")
                    await request(client, phase, "qkm_preview_changes", dict(action="mapping.update", source_ref=target,
                        **fields), error="permission_denied")
                    assert (await snapshot(client, phase))[0] == before and ini.read_bytes() == original_ini
                    if cold:
                        assert authenticated["instance"] != warm["instance"]
                        assert before[index]["rows"][0]["fields"] == warm["row"]["fields"]
                        assert before[index]["rows"][0]["derived"]["pure_original_keys"] == [warm["row"]["fields"]["KeyMapData_OriginalKeys"]]
                        await request(client, phase, "qkm_query", dict(entity="mappings", source_revision=warm["revision"],
                            source_tab_ref=warm["source_tab_ref"]), error="stale_source")
                        old_result = (await request(client, phase, "qkm_get_operation", dict(operation_id=warm["operation"])))["result"]
                        assert old_result["status"] == "unknown"
                    return
                if flag and variant in ("conflict", "common_conflict"):
                    assert before[index]["rows"][0]["fields"]["KeyMapData_Disabled"]
                    assert not before[0]["rows"][1]["fields"]["KeyMapData_Disabled"]
                    assert before[index]["rows"][0]["fields"]["KeyMapData_OriginalKeys"] == "C"
                    await request(client, phase, "qkm_preview_changes", dict(action="mapping.update", source_ref=target,
                        disabled=False), error="unsupported_capability")
                    assert (await snapshot(client, phase))[0] == before and ini.read_bytes() == original_ini
                    return
                if phase in GUARD_CASES:
                    original = "E" if phase == "key_common_priority" else "C"
                    await request(client, phase, "qkm_preview_changes", dict(action="mapping.update", source_ref=target,
                        original_key=original), error="unsupported_capability")
                    assert (await snapshot(client, phase))[0] == before and ini.read_bytes() == original_ini
                    return
                if not flag and phase in ("common_conflict", "complex"):
                    if phase == "complex":
                        assert before[index]["rows"][0]["fields"]["KeyMapData_MappingKeys"] == ["B", "D"]
                    await request(client, phase, "qkm_preview_changes", dict(action="mapping.update", source_ref=target,
                        original_key="C"), error="unsupported_capability")
                    assert (await snapshot(client, phase))[0] == before and ini.read_bytes() == original_ini
                    return
                baseline_path = (args.disabled_gui_baseline / ("qt6-" + variant + "-01/case.json") if flag
                    else args.gui_baseline / ("qt6-" + phase + "-02/case.json"))
                assert baseline_path.stat().st_size <= 2 * 1024 * 1024
                baseline = json.loads(baseline_path.read_bytes())["settingsGeneral"]
                assert before == baseline["before"]["sources"], "Stock input must match the independent GUI input."
                fields = (dict(disabled=not before[index]["rows"][0]["fields"]["KeyMapData_Disabled"]) if flag
                    else dict(original_key="E", mapping_key="C"))
                change = dict(action="mapping.update", source_ref=target, **fields)
                if not flag:
                    if phase == "normal_clear_up":
                        change["key_up_mapping_key"] = ""
                    else:
                        await request(client, phase, "qkm_preview_changes", dict(action="mapping.update", source_ref=target,
                            key_up_mapping_key=""), error="invalid_mapping")
                preview = (await request(client, phase, "qkm_preview_changes", change))["result"]
                assert (await snapshot(client, phase))[0] == before and ini.read_bytes() == original_ini
                await request(client, phase, "qkm_cancel_changes", dict(preview_id=preview["preview_id"]))
                assert (await snapshot(client, phase))[0] == before
                preview = (await request(client, phase, "qkm_preview_changes", change))["result"]
                applied = (await request(client, phase, "qkm_apply_changes", dict(preview_id=preview["preview_id"])))["result"]
                assert applied["status"] == "applied" and applied["result"]["modified"]
                assert not applied["result"]["saved"] and not applied["result"]["started_by_operation"]
                after, next_refs, after_revision = await snapshot(client, phase)
                assert after == baseline["after_item" if flag else "after"]["sources"], "Every source field and derived sample must match the real GUI."
                assert ini.read_bytes() == original_ini
                repeated = (await request(client, phase, "qkm_apply_changes", dict(preview_id=preview["preview_id"])))["result"]
                assert repeated["replayed"] and repeated["result"] == applied["result"]
                assert all(repeated[key] == applied[key] for key in
                    ("operation_id", "status", "target", "sampled_at_utc", "result_known", "persistence"))
                assert (await snapshot(client, phase))[0] == after
                await request(client, phase, "qkm_preview_changes", change, error="stale_source")
                if flag and variant == "hidden":
                    await request(client, phase, "qkm_preview_changes", dict(action="mapping.update",
                        source_ref=next_refs[index][0]["source_ref"], disabled=False), error="invalid_target")
                    assert (await snapshot(client, phase))[0] == after
                else:
                    no_op = (await request(client, phase, "qkm_preview_changes", dict(action="mapping.update",
                        source_ref=next_refs[index][0]["source_ref"], **fields)))["result"]
                    no_change = (await request(client, phase, "qkm_apply_changes", dict(preview_id=no_op["preview_id"])))["result"]
                    assert not no_change["result"]["modified"] and (await snapshot(client, phase))[2] == after_revision
                if phase == "normal_clear_up":
                    for key_up in ("D", "C"):
                        current_refs = (await snapshot(client, phase))[1]
                        explicit = (await request(client, phase, "qkm_preview_changes", dict(action="mapping.update",
                            source_ref=current_refs[index][0]["source_ref"], key_up_mapping_key=key_up)))["result"]
                        await request(client, phase, "qkm_apply_changes", dict(preview_id=explicit["preview_id"]))
                        expected = deepcopy(after)
                        expected[index]["rows"][0]["fields"]["KeyMapData_MappingKeys_KeyUp"] = [key_up]
                        expected[index]["rows"][0]["fields"]["KeyMapData_MappingKeys_KeyUp_Comments"] = [""]
                        expected[index]["rows"][0]["derived"]["pure_key_up_mapping_keys"] = [key_up]
                        assert (await snapshot(client, phase))[0] == expected and ini.read_bytes() == original_ini
                    current_refs = (await snapshot(client, phase))[1]
                    restored = (await request(client, phase, "qkm_preview_changes", dict(action="mapping.update",
                        source_ref=current_refs[index][0]["source_ref"], key_up_mapping_key="")))["result"]
                    await request(client, phase, "qkm_apply_changes", dict(preview_id=restored["preview_id"]))
                    assert (await snapshot(client, phase))[0] == after
                gui = await product.action("ReadMapping")
                assert gui["mapping_button_text"] == "映射开始"
                if not flag:
                    assert gui["mapping_rows"][0] == dict(original="E", mapping="C")
                (product.output / "gui-comparison.json").write_text(json.dumps(dict(before=True, after=True,
                    baseline=str(baseline_path), fields=after[index]["rows"][0]), ensure_ascii=False, indent=2), encoding="utf-8")
                if flag and variant in ("disabled", "common"):
                    current_refs = (await snapshot(client, phase))[1]
                    restored = (await request(client, phase, "qkm_preview_changes", dict(action="mapping.update",
                        source_ref=current_refs[index][0]["source_ref"], disabled=before[index]["rows"][0]["fields"]["KeyMapData_Disabled"])))["result"]
                    await request(client, phase, "qkm_apply_changes", dict(preview_id=restored["preview_id"]))
                    assert (await snapshot(client, phase))[0] == baseline["after_table"]["sources"]
                if variant == "normal":
                    saved = (await request(client, phase, "qkm_preview_changes", dict(action="configuration.save",
                        setting_name="Edit141Seed")))["result"]
                    result = (await request(client, phase, "qkm_apply_changes", dict(preview_id=saved["preview_id"],
                        confirm_scope=True)))["result"]
                    assert result["status"] == "applied" and result["result"]["saved"] and result["result"]["reloaded"]
                    warm.update(instance=authenticated["instance"], row=after[index]["rows"][0], revision=after_revision,
                        source_tab_ref=next_refs[index][0]["source_tab_ref"], operation=preview["preview_id"])
        finally:
            # This dedicated store was empty before this session; never clean an unrelated pairing.
            credential = store.load()
            if credential is not None:
                await product.action("OpenAdvanced")
                current = await product.action("OpenMcp")
                assert current["pair_count"] == 1
                await product.action("Revoke")
                await product.action("Disable")
                store.forget()
                assert store.load() is None

    with patch.object(stdio, "_create_platform_compatible_process", observed_spawn):
        for phase in phases:
            flag = phase in FLAG_CASES
            variant = phase[5:] if flag else phase
            try:
                with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Enum\ROOT\FAKERINPUT"):
                    raise AssertionError("FakerInput is present; review ordinary shutdown scope before starting.")
            except FileNotFoundError:
                pass
            if variant != "cold":
                payload = seed
                if variant == "disabled" or (flag and variant == "conflict"):
                    payload = payload.replace(b"KeyMapData_SendMappingKeyMethod=", b"KeyMapData_Disabled=ON#OFF|OFF#OFF\nKeyMapData_SendMappingKeyMethod=")
                if phase == "normal_clear_up":
                    payload = payload.replace(b"KeyMapData_SendTiming=NORMAL#NORMAL|NORMAL#NORMAL", b"KeyMapData_SendTiming=KEYDOWN_AND_KEYUP#NORMAL|NORMAL#NORMAL")
                if phase == "common_conflict":
                    payload = payload.replace(b"MappingTable_TabIncludeCommonList=OFF, OFF", b"MappingTable_TabIncludeCommonList=ON, OFF")
                if phase == "complex":
                    payload = payload.replace(b"KeyMapData_MappingKeys=B#D|", "KeyMapData_MappingKeys=B»D#D|".encode("utf-8"))
                if flag and variant in ("common", "common_conflict"):
                    payload = payload.replace(b"MappingTable_TabIncludeCommonList=OFF, OFF", b"MappingTable_TabIncludeCommonList=ON, OFF")
                if flag and variant == "hidden":
                    payload = payload.replace(b"HideDisabled=false\n", b"HideDisabled=true\n")
                if flag and variant == "conflict":
                    payload = payload.replace(b"KeyMapData_OriginalKeys=A#C|", b"KeyMapData_OriginalKeys=C#C|")
                if flag and variant == "common_conflict":
                    payload = payload.replace(b"KeyMapData_SendMappingKeyMethod=", b"KeyMapData_Disabled=OFF#OFF|ON#OFF\nKeyMapData_SendMappingKeyMethod=")
                    payload = payload.replace(b"KeyMapData_OriginalKeys=A#C|F13#F14", b"KeyMapData_OriginalKeys=A#C|C#F14")
                if phase == "key_common_priority":
                    payload = payload.replace(b"MappingTable_TabIncludeCommonList=OFF, OFF", b"MappingTable_TabIncludeCommonList=ON, OFF")
                    payload = payload.replace(b"KeyMapData_OriginalKeys=A#C|F13#F14", b"KeyMapData_OriginalKeys=A#C|E#F14")
                ini.write_bytes(payload)
            output = args.output / phase
            output.mkdir()
            system_before = filter_keys()
            (output / "filter-keys-before.json").write_text(json.dumps(system_before), encoding="utf-8")
            product = Product(args.executable, output, lifetime_seconds=600)
            try:
                await product.started()
                await session(product, phase, read_only=variant in ("read", "cold"), cold=variant == "cold")
            finally:
                await product.close()
                system_after = filter_keys()
                (output / "filter-keys-after.json").write_text(json.dumps(system_after), encoding="utf-8")
                assert system_after == system_before, "Do not automatically restore changed system settings."
                with closing(win32api.OpenProcess(0x1000, False, own.pid)) as handle:
                    assert integrity(handle) == 8192
            completed.append(phase)
            (args.output / "completed-phases.json").write_text(json.dumps(completed), encoding="utf-8")
    compared = sum(phase in ("normal", "normal_clear_up", "disabled", "common", "flag_normal", "flag_disabled", "flag_common", "flag_hidden") for phase in completed)
    (args.output / "passed.json").write_text(json.dumps(dict(completed=completed, gui_baselines=compared,
        tools=17, business_capabilities=23, mapping_started=False, system_input_injected=False,
        bridge_peers=peers), ensure_ascii=False, indent=2), encoding="utf-8")
    print("PASS stock Release selected cases: " + ", ".join(completed), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=lambda value: Path(value).resolve(strict=True), required=True)
    parser.add_argument("--bridge-python", type=lambda value: Path(value).resolve(strict=True), required=True)
    parser.add_argument("--gui-baseline", type=lambda value: Path(value).resolve(strict=True), required=True)
    parser.add_argument("--output", type=lambda value: Path(value).resolve(), required=True)
    parser.add_argument("--disabled-gui-baseline", type=lambda value: Path(value).resolve(strict=True))
    parser.add_argument("--case", nargs="+", choices=(*CASES, *FLAG_CASES, *GUARD_CASES), help="Run selected cases; each cold requires its normal case earlier in the same run.")
    asyncio.run(run(parser.parse_args()))
