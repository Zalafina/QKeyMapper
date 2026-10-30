"""Ordinary-permission standalone bundle imports and official MCP stdio acceptance."""

import argparse
import asyncio
import json
import os
from pathlib import Path
import subprocess
import time

from mcp import Client, StdioServerParameters

from test_sdk_interop import TOOLS, call


async def exercise(bundle, output):
    bundle = bundle.resolve(strict=True)
    output.mkdir(parents=True, exist_ok=False)
    poison = output / "unrelated 中文 cwd"
    poison.mkdir()
    (poison / "qkeymapper_mcp.py").write_text("raise RuntimeError('CWD or PYTHONPATH imported')\n", encoding="utf-8")
    runtime = bundle / "runtime"
    executable = runtime / "python.exe"
    environment = {**os.environ, "PATH": os.pathsep.join((os.environ["SystemRoot"], str(Path(os.environ["SystemRoot"]) / "System32"))),
        "PYTHONPATH": str(poison), "PYTHONHOME": str(poison), "PYTHONUSERBASE": str(poison), "QKM_BUNDLE_TEST_ROOT": str(runtime)}
    probe = """
import importlib.metadata as metadata, importlib.util, json, os, pathlib, struct, sys
import qkeymapper_mcp, mcp, win32api, win32file, win32security, win32pipe, pywintypes, pythoncom, pydantic_core, cryptography, rpds
root = pathlib.Path(os.environ['QKM_BUNDLE_TEST_ROOT']).resolve()
assert sys.version_info[:3] == (3, 11, 9) and struct.calcsize('P') == 8
assert sys.flags.isolated and sys.flags.ignore_environment and sys.flags.no_user_site and sys.dont_write_bytecode
assert pathlib.Path(sys.prefix).resolve() == pathlib.Path(sys.base_prefix).resolve() == root
assert all(pathlib.Path(value).resolve().is_relative_to(root) for value in sys.path)
assert pathlib.Path(qkeymapper_mcp.__file__).resolve().is_relative_to(root)
assert pathlib.Path(win32api.__file__).resolve().is_relative_to(root)
assert importlib.util.find_spec('pip') is None
installed = {dist.metadata['Name'].lower().replace('_','-'): dist.version for dist in metadata.distributions()}
expected = {item['name'].lower().replace('_','-'): item['version'] for item in json.loads((root.parent/'bundle-manifest.json').read_text(encoding='utf-8'))['dependencies']}
assert installed == expected, (installed, expected)
print(json.dumps(dict(python=sys.version, prefix=str(root), paths=sys.path, dependencies=installed, isolated=True, pip_present=False), ensure_ascii=False))
"""
    imported = subprocess.run([str(executable), "-I", "-B", "-X", "utf8", "-c", probe], env=environment,
        cwd=poison, capture_output=True, encoding="utf-8", timeout=20, creationflags=subprocess.CREATE_NO_WINDOW)
    (output / "imports.log").write_text(imported.stdout + imported.stderr, encoding="utf-8")
    assert imported.returncode == 0, imported.stderr
    parameters = StdioServerParameters(command=str(executable), args=["-I", "-B", "-X", "utf8", "-m", "qkeymapper_mcp",
        "--discovery-file", str(output / "absent/instance.json"), "--executable", str(output / "QKeyMapper-not-started.exe")],
        cwd=str(poison), env=environment)
    async with Client(parameters, read_timeout_seconds=15, cache=None) as client:
        tools = (await client.list_tools()).tools
        assert {tool.name for tool in tools} == TOOLS
        assert all(tool.model_dump(by_alias=True)["inputSchema"]["additionalProperties"] is False for tool in tools)
        replies = [await call(client, "qkm_get_connection_state"),
                   await call(client, "qkm_get_capabilities", error="not_connected"),
                   await call(client, "qkm_connect")]
        assert replies[0]["status"] == "disconnected" and not replies[0]["busy"]
        assert replies[2]["accepted"]
        until = time.monotonic() + 5
        while True:
            state = await call(client, "qkm_get_connection_state")
            replies.append(state)
            (output / "sdk-calls.json").write_text(json.dumps(replies, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
            if state["status"] == "failed" and not state["busy"]:
                assert state["error"] == "application_unavailable"
                break
            assert time.monotonic() < until, state
            await asyncio.sleep(.01)
    configured_tools = {}
    for serial in (False, True):
        arguments = [*parameters.args, "--demo-record", str(output / "absent/demo.json")]
        if serial:
            arguments.append("--enable-demo-mapping")
        configured = StdioServerParameters(command=str(executable), args=arguments, cwd=str(poison), env=environment)
        async with Client(configured, read_timeout_seconds=15, cache=None) as client:
            names = {item.name for item in (await client.list_tools()).tools}
            expected = TOOLS | {"qkm_get_demo_state"} | ({"qkm_demo_mapping"} if serial else set())
            assert names == expected
            await call(client, "qkm_get_demo_state", error="not_connected")
            if serial:
                await call(client, "qkm_demo_mapping", dict(original_key="A", mapping_key="B"), error="not_connected")
            state = await call(client, "qkm_get_connection_state")
            assert state["status"] == "failed" and state["error"] == "not_connected" and not state["busy"], state
            configured_tools["serial_demo" if serial else "record_only"] = len(names)
    result = dict(passed=True, tools=len(tools), configured_tools=configured_tools, runtime=str(executable), no_product_start=True, replies=replies)
    (output / "sdk-pass.json").write_text(json.dumps(result, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    print(json.dumps(asyncio.run(exercise(arguments.bundle, arguments.output.absolute())), ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
