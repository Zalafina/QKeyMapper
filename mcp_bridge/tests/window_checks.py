"""Focused checks using an already authenticated official SDK request function."""

from datetime import datetime
import re


async def verify_window_query(request, target, instance: str, desktop: str) -> dict:
    page = (await request("qkm_query", {"entity": "windows"}))["result"]
    assert page["schema_version"] == 1 and page["entity"] == "windows"
    assert datetime.fromisoformat(page["sampled_at_utc"].replace("Z", "+00:00")).utcoffset().total_seconds() == 0
    assert page["reference_scope"] == "instance_window_lifetime_only"
    assert page["target"] == {"instance": instance, "pid": str(target.pid), "created": str(target.created),
                              "executable": target.executable, "desktop": desktop}
    assert type(page["foreground_in_instance"]) is bool
    assert page["total"] == 2 and page["offset"] == 0 and page["next_offset"] is None
    assert [item["role"] for item in page["items"]] == ["main", "item_setup"]
    for item in page["items"]:
        assert item["exists"] and int(item["window_generation"]) > 0
        assert all(type(item[key]) is bool for key in ("qt_visible", "native_ready", "native_visible", "minimized", "foreground_exact"))
        assert item["logical_width"] > 0 and item["logical_height"] > 0 and item["device_pixel_ratio"] > 0
        if item["native_ready"]:
            assert re.fullmatch("[0-9a-f]{64}", item["window_ref"])
        else:
            assert item["window_ref"] is None and not item["native_visible"] and not item["foreground_exact"]
    return page


async def verify_window_query_arguments(request) -> None:
    capabilities = (await request("qkm_get_capabilities"))["result"]
    capability = next(item for item in capabilities["business_capabilities"] if item["id"] == "windows.read")
    assert capability["method"] == "qkm_query" and capability["required_scopes"] == 1 and capability["read_only"]
    schema = capability["arguments_schema"]
    assert schema["required"] == ["entity"] and schema["additionalProperties"] is False
    assert set(schema["properties"]) == {"entity", "offset", "limit"}
    assert schema["properties"]["entity"]["const"] == "windows"
    first = (await request("qkm_query", {"entity": "windows", "limit": 1}))["result"]
    second = (await request("qkm_query", {"entity": "windows", "offset": 1, "limit": 1}))["result"]
    empty = (await request("qkm_query", {"entity": "windows", "offset": 2}))["result"]
    assert first["next_offset"] == 1 and first["items"][0]["role"] == "main"
    assert second["next_offset"] is None and second["items"][0]["role"] == "item_setup"
    assert empty["items"] == [] and empty["next_offset"] is None
    for extra in ({"offset": 3}, {"source_revision": "a" * 64}, {"catalog_revision": "a" * 64}):
        await request("qkm_query", {"entity": "windows", **extra}, error="invalid_arguments")
