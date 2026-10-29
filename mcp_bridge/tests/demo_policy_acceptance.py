"""Formal mode/record refusals and recovery on the owned private demo fixture."""

import json

from demo_scene_acceptance import exercise as scenes
from test_sdk_interop import business_state


async def exercise(client, request, variant, output, exe):
    assert variant in ("demo_sequence", "demo_ready")
    disk = (exe.parent / "keymapdata.ini").read_bytes()
    initial = business_state((await request("qkm_get_state"))["result"])
    sources = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    record = output / "host-record/demo.json"
    original = record.read_bytes()
    binding = json.loads(original.decode("utf-8"))
    assert binding["version"] == 2 and binding["startup_purpose"] == "configuration_only_demo"
    try:
        legacy = dict(binding, version=1)
        del legacy["startup_purpose"]
        for changed in (legacy, dict(binding, startup_purpose="standard")):
            record.write_bytes(json.dumps(changed).encode("utf-8"))
            value = (await request("qkm_get_demo_state"))["result"]
            assert value["ready"] is False and value["reason"] == "demo_record_mismatch"
            assert "target" not in value
            assert (await request("qkm_get_connection_state"))["status"] == "connected"
    finally:
        record.write_bytes(original)
    restored = (await request("qkm_get_demo_state"))["result"]
    if variant == "demo_sequence":
        assert restored["ready"] and restored["demo_session"]["generation"] == 0
    else:
        assert restored["ready"] is False and restored["reason"] == "demo_configuration_only_required"
        failed = await request("qkm_demo_mapping", dict(original_key="A", mapping_key="B"),
                               error="demo_configuration_only_required")
        assert not failed["result"]["quarantined"] and not failed["result"]["receipt_available"]
        assert (await request("qkm_get_connection_state"))["status"] == "connected"
    assert business_state((await request("qkm_get_state"))["result"]) == initial
    assert (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"] == sources
    assert (exe.parent / "keymapdata.ini").read_bytes() == disk
    if variant == "demo_ready":
        return dict(successful_saves=0, scenes=0, schema_errors=0, record_refusals=2, standard_mode_refused=True)
    details = await scenes(client, request, variant, output, exe, views=("mapping_advanced", "item_setup"))
    return dict(details, record_refusals=2, configuration_only_verified=True)
