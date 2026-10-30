"""Read-only readiness before the fixed host normally closes its live product child."""

import json
import os
from pathlib import Path

from qkeymapper_mcp.demo import read_binding
from qkeymapper_mcp.windows import identity
from test_sdk_interop import business_state


async def exercise(client, request, variant, output, exe):
    assert variant == "demo_ready"
    disk = (exe.parent / "keymapdata.ini").read_bytes()
    capabilities = (await request("qkm_get_capabilities"))["result"]
    source = json.loads((output / "demo-record-source.json").read_text(encoding="utf-8"))
    path = Path(capabilities["demo_record"])
    assert source["owner"] == "product" and path == Path(source["path"])
    record = read_binding(path, identity(os.getpid()))
    assert Path(record["executable"]) == exe and record["startup_purpose"] == "configuration_only_demo"
    initial = business_state((await request("qkm_get_state"))["result"])
    sources = (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
    original = path.read_bytes()
    for _ in range(2):
        observed = (await request("qkm_get_demo_state"))["result"]
        assert observed["ready"] and observed["reason"] == "ready"
        assert observed["target"] == {key: record[key] for key in ("instance", "pid", "created", "executable", "desktop")}
        assert observed["demo_session"]["generation"] == 0
    assert business_state((await request("qkm_get_state"))["result"]) == initial
    assert (await request("qkm_query", dict(entity="mapping_sources", limit=100)))["result"] == sources
    assert path.read_bytes() == original and (exe.parent / "keymapdata.ini").read_bytes() == disk
    return dict(successful_saves=0, scenes=0, native_images=0, schema_errors=0,
                product_record_verified=True, host_ready=True)
