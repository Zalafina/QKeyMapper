"""Official stdio checks for the bounded native demo_ready* private fixture."""

import json

from test_sdk_interop import business_state


async def exercise(client, request, variant, output, exe):
    expected = {"demo_ready": "ready", "demo_ready_read": "permission_denied",
                "demo_ready_nonempty": "demo_baseline_not_empty", "demo_ready_dirty": "demo_baseline_dirty"}
    assert variant in expected
    ini = exe.parent / "keymapdata.ini"
    disk = ini.read_bytes()
    initial = business_state((await request("qkm_get_state"))["result"])
    sources = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    definition = next(tool for tool in (await client.list_tools()).tools if tool.name == "qkm_get_demo_state")
    metadata = definition.model_dump(by_alias=True)
    assert metadata["annotations"]["readOnlyHint"] is True
    assert metadata["inputSchema"]["additionalProperties"] is False
    rejected = await client.call_tool("qkm_get_demo_state", dict(pid=123))
    assert rejected.is_error
    record = output / "host-record/demo.json"
    original = record.read_bytes()
    binding = json.loads(original.decode("utf-8"))
    observed = (await request("qkm_get_demo_state"))["result"]
    assert observed["configured"] is True and observed["ready"] is (variant == "demo_ready")
    assert observed["reason"] == expected[variant] and observed["reference_scope"] == "preflight_observation_only"
    assert observed["target"] == {key: binding[key] for key in ("instance", "pid", "created", "executable", "desktop")}
    try:
        for change in (dict(created=str(int(binding["created"]) + 1)), dict(instance="f" * 32),
                       dict(session=str(int(binding["session"]) + 1)), dict(executable=binding["executable"] + ".wrong"),
                       dict(desktop=binding["desktop"] + "_wrong"), dict(unexpected=True)):
            record.write_bytes(json.dumps({**binding, **change}).encode("utf-8"))
            rejected = (await request("qkm_get_demo_state"))["result"]
            assert rejected["ready"] is False and rejected["reason"] == "demo_record_mismatch"
            assert "target" not in rejected
            assert (await request("qkm_get_connection_state"))["status"] == "connected"
        record.write_bytes(b"{")
        rejected = (await request("qkm_get_demo_state"))["result"]
        assert rejected["ready"] is False and rejected["reason"] == "demo_record_rejected"
    finally:
        record.write_bytes(original)
    restored = (await request("qkm_get_demo_state"))["result"]
    assert restored["reason"] == expected[variant] and restored["target"] == observed["target"]
    assert business_state((await request("qkm_get_state"))["result"]) == initial
    assert (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"] == sources
    assert ini.read_bytes() == disk
    return dict(successful_saves=0,ready=observed["ready"],reason=observed["reason"],schema_errors=1)
