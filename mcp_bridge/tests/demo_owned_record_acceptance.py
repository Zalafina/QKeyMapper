"""Consume the product's own private record without a host-written replacement."""

import json
import os
from pathlib import Path

import pywintypes
import win32con
import win32file

from demo_scene_acceptance import exercise as scenes
from qkeymapper_mcp.demo import read_binding
from qkeymapper_mcp.windows import identity


async def exercise(client, request, variant, output, exe):
    assert variant == "demo_sequence"
    capabilities = (await request("qkm_get_capabilities"))["result"]
    path = Path(capabilities["demo_record"])
    source = json.loads((output / "demo-record-source.json").read_text(encoding="utf-8"))
    assert source["owner"] == "product" and path == Path(source["path"])
    assert not (output / "host-record").exists()
    record = read_binding(path, identity(os.getpid()))
    assert record["version"] == 2 and record["startup_purpose"] == "configuration_only_demo"
    assert Path(record["executable"]) == exe and record["configuration"] == "Demo104Seed"
    original = path.read_bytes()
    try:
        handle = win32file.CreateFile(str(path), win32con.GENERIC_WRITE, win32con.FILE_SHARE_READ,
                                     None, win32con.OPEN_EXISTING, 0, None)
    except pywintypes.error as error:
        code = getattr(error, "winerror", None)
        assert code == 32, (code, str(error))
        (output / "record-write-refused.json").write_text(json.dumps(dict(winerror=code)) + "\n", encoding="utf-8")
    else:
        handle.Close()
        raise AssertionError("the live product record unexpectedly permits a writer")
    ready = (await request("qkm_get_demo_state"))["result"]
    assert ready["ready"] and ready["target"] == {key: record[key]
        for key in ("instance", "pid", "created", "executable", "desktop")}
    result = await scenes(client, request, variant, output, exe, views=("mapping_advanced", "item_setup"))
    assert path.read_bytes() == original
    assert read_binding(path, identity(os.getpid())) == record
    return dict(result, product_record_verified=True, product_record_write_refused=True)
