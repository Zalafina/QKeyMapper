"""Exercise a built protocol fixture over real stdio pipes on Windows."""

import argparse
import asyncio
import base64
import hashlib
import json
import struct
import subprocess
import time
import unittest
from contextlib import asynccontextmanager
from pathlib import Path

TOOL = "qkm_protocol_fixture"
LEGACY = ("2024-11-05", "2025-03-26", "2025-06-18", "2025-11-25")
MODERN = "2026-07-28"
INFO = {"name": "qkm-raw-client", "version": "1"}
META_PREFIX = "io.modelcontextprotocol/"
TIMEOUT = 5
CREATE_FLAGS = getattr(subprocess, "CREATE_NO_WINDOW", 0)


def metadata(version=MODERN):
    return {META_PREFIX + k: v for k, v in {
        "protocolVersion": version,
        "clientCapabilities": {},
        "clientInfo": INFO,
    }.items()}


def request(method, identifier, params=None):
    value = {"jsonrpc": "2.0", "method": method}
    if identifier is not None:
        value["id"] = identifier
    if params is not None:
        value["params"] = params
    return (json.dumps(value, ensure_ascii=False) + "\n").encode("utf-8")


class Peer:
    def __init__(self, process):
        self.process = process
        self.identifier = 0

    async def send(self, *messages):
        self.process.stdin.write(b"".join(messages))
        await asyncio.wait_for(self.process.stdin.drain(), TIMEOUT)

    async def receive(self, identifier):
        line = await asyncio.wait_for(self.process.stdout.readline(), TIMEOUT)
        assert line.endswith(b"\n"), f"missing complete response: {line!r}"
        value = json.loads(line.decode("utf-8", errors="strict"))
        assert value["jsonrpc"] == "2.0" and value.get("id") == identifier, value
        return value

    async def rpc(self, method, params=None):
        self.identifier += 1
        await self.send(request(method, self.identifier, params))
        return await self.receive(self.identifier)

    async def initialize(self, version=LEGACY[-1], *, notify=True):
        reply = await self.rpc("initialize", {
            "protocolVersion": version,
            "capabilities": {},
            "clientInfo": INFO,
        })
        if notify:
            await self.send(request("notifications/initialized", None))
        return reply["result"]["protocolVersion"]

    async def call(self, args=None, *, inline=False, extra=None):
        params = {"name": TOOL}
        if args is not None:
            params["arguments"] = args
        if inline:
            params["_meta"] = metadata()
        if extra:
            params.update(extra)
        return await self.rpc("tools/call", params)

    async def finish(self, timeout=TIMEOUT):
        self.process.stdin.close()
        remaining = await asyncio.wait_for(self.process.stdout.read(), timeout)
        errors = await asyncio.wait_for(self.process.stderr.read(), timeout)
        code = await asyncio.wait_for(self.process.wait(), timeout)
        return code, remaining, errors


@asynccontextmanager
async def start_fixture(exe):
    process = await asyncio.create_subprocess_exec(
        str(exe), "--protocol-fixture",
        stdin=asyncio.subprocess.PIPE, stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE, creationflags=CREATE_FLAGS,
    )
    try:
        yield Peer(process)
    finally:
        if process.returncode is None:
            process.stdin.close()
            try:
                await asyncio.wait_for(process.communicate(), TIMEOUT)
            except TimeoutError:
                process.kill()
                await asyncio.wait_for(process.communicate(), TIMEOUT)


class StdioTests(unittest.IsolatedAsyncioTestCase):
    exe: Path

    async def test_legacy_versions(self):
        for version in LEGACY:
            with self.subTest(version=version):
                async with start_fixture(self.exe) as peer:
                    self.assertEqual(await peer.initialize(version), version)
                    listing = (await peer.rpc("tools/list"))["result"]
                    self.assertNotIn("resultType", listing)
                    self.assertEqual([tool["name"] for tool in listing["tools"]], [TOOL])
                    result = (await peer.call())["result"]
                    self.assertFalse(result["isError"])
                    self.assertNotIn("resultType", result)
                    self.assertEqual(json.loads(result["content"][0]["text"])["call"], 1)
                    self.assertEqual(await peer.finish(), (0, b"", b""))

    async def test_modern_discover_and_direct(self):
        for discover in (True, False):
            with self.subTest(discover=discover):
                async with start_fixture(self.exe) as peer:
                    if discover:
                        info = (await peer.rpc("server/discover", {"_meta": metadata()}))["result"]
                        self.assertEqual(set(info["supportedVersions"]), {*LEGACY, MODERN})
                        self.assertIn("tools", info["capabilities"])
                        self.assertEqual(info["_meta"][META_PREFIX + "serverInfo"]["name"],
                                         "qkeymapper-mcp-rust")
                    await peer.send(request("unknown/notification", None))
                    result = (await peer.call({"text": "中文と日本語"}, inline=True))["result"]
                    self.assertEqual(result["resultType"], "complete")
                    self.assertEqual(result["structuredContent"]["text"], "中文と日本語")
                    self.assertEqual(await peer.finish(), (0, b"", b""))

    async def test_probe_then_legacy(self):
        for params in ({}, {"_meta": metadata()}):
            async with start_fixture(self.exe) as peer:
                probe = await peer.rpc("server/discover", params)
                self.assertIn("result" if params else "error", probe)
                self.assertEqual(await peer.initialize(), LEGACY[-1])
                self.assertEqual((await peer.call())["result"]["structuredContent"]["call"], 1)
                self.assertEqual(await peer.finish(), (0, b"", b""))

    async def test_unknown_and_pre_initialize(self):
        async with start_fixture(self.exe) as peer:
            await peer.send(request("unknown/notification", None))
            self.assertIn("error", await peer.rpc("unknown/request"))
            self.assertIn("error", await peer.call())
            self.assertEqual((await peer.rpc("ping"))["result"], {})
            self.assertEqual(await peer.initialize(), LEGACY[-1])
            await peer.send(request("unknown/notification", None))
            self.assertEqual((await peer.rpc("unknown/request"))["error"]["code"], -32601)
            self.assertEqual((await peer.call())["result"]["structuredContent"]["call"], 1)
            self.assertEqual(await peer.finish(), (0, b"", b""))

    async def test_version_and_metadata_errors(self):
        async with start_fixture(self.exe) as peer:
            error = (await peer.rpc("tools/list", {"_meta": metadata("2099-07-28")}))["error"]
            self.assertEqual(error["code"], -32022)
            self.assertEqual(set(error["data"]["supported"]), {*LEGACY, MODERN})
            first = (await peer.call(inline=True))["result"]
            self.assertEqual(first["structuredContent"]["call"], 1)
            for meta in (None, {}, {**metadata(), META_PREFIX + "clientCapabilities": 1}):
                self.assertEqual((await peer.call(extra={"_meta": meta}))["error"]["code"], -32602)
            self.assertEqual((await peer.rpc("ping", {"_meta": metadata()}))["error"]["code"], -32601)
            self.assertEqual((await peer.rpc("unknown/request", {"_meta": metadata()}))["error"]["code"], -32601)
            self.assertEqual((await peer.call(inline=True))["result"]["structuredContent"]["call"], 2)
            self.assertEqual(await peer.finish(), (0, b"", b""))
        for requested in ("2099-07-28", MODERN):
            async with start_fixture(self.exe) as peer:
                # An initialize response can select a supported legacy version.
                self.assertIn(await peer.initialize(requested, notify=False), LEGACY)
                self.assertEqual(await peer.finish(), (0, b"", b""))

    async def test_malformed_oversized_utf8_and_flush(self):
        async with start_fixture(self.exe) as peer:
            await peer.initialize()
            await peer.send(b"{broken json\n", b"x" * (64 * 1024 + 1) + b"\n")
            self.assertEqual(await peer.rpc("ping"), {"jsonrpc": "2.0", "id": 2, "result": {}})
            await peer.send(b"[]\n")
            self.assertEqual((await peer.receive(None))["error"]["code"], -32600)
            parts = request("tools/call", "cn-日", {"name": TOOL, "arguments": {"text": "中文\n日本語"}})
            split = parts.index("中".encode("utf-8")) + 1
            await peer.send(parts[:split])
            await peer.send(parts[split:-1] + b"\r\n")
            received = (await peer.receive("cn-日"))["result"]
            self.assertEqual(received["structuredContent"]["text"], "中文\n日本語")
            self.assertEqual(await peer.finish(), (0, b"", b""))

    async def test_schema_results_and_png(self):
        from jsonschema import Draft202012Validator

        async with start_fixture(self.exe) as peer:
            await peer.initialize()
            schema = (await peer.rpc("tools/list"))["result"]["tools"][0]["inputSchema"]
            Draft202012Validator.check_schema(schema)
            validator = Draft202012Validator(schema)
            self.assertEqual(schema["properties"]["mode"]["default"], "text")
            for bad in ({"mode": None}, {"text": None}, {"text": False}, {"mode": "other"},
                        {"unknown": 1}, {"text": "界" * 257}):
                self.assertFalse(validator.is_valid(bad), bad)
                self.assertEqual((await peer.call(bad))["error"]["code"], -32602, bad)
            self.assertTrue(validator.is_valid({"text": "界" * 256}))
            first = (await peer.call())["result"]
            self.assertEqual(first["structuredContent"]["text"], "QKeyMapper 协议 / 確認")
            self.assertEqual(first["structuredContent"]["call"], 1)
            self.assertEqual(json.loads(first["content"][0]["text"]), first["structuredContent"])
            err = (await peer.call({"mode": "error"}))["result"]
            self.assertIs(err["isError"], True)
            self.assertEqual(err["structuredContent"]["error"], "fixture_rejected")
            image = (await peer.call({"mode": "image"}))["result"]
            block = image["content"][1]
            self.assertEqual((block["type"], block["mimeType"]), ("image", "image/png"))
            decoded = base64.b64decode(block["data"], validate=True)
            self.assertEqual(decoded[:8], b"\x89PNG\r\n\x1a\n")
            self.assertEqual(struct.unpack(">II", decoded[16:24]), (2, 2))
            self.assertNotIn(block["data"], json.dumps(image["structuredContent"]))
            self.assertEqual(await peer.finish(), (0, b"", b""))

    async def test_eof_and_cancellation(self):
        wait = {"name": TOOL, "arguments": {"mode": "wait"}}
        async with start_fixture(self.exe) as peer:
            await peer.send(b'{"jsonrpc":')
            self.assertEqual(await peer.finish(), (0, b"", b""))
        async with start_fixture(self.exe) as peer:
            await peer.initialize()
            await peer.send(request("tools/call", "cancel-me", wait))
            self.assertEqual((await peer.call())["result"]["structuredContent"]["call"], 2)
            with self.assertRaises(TimeoutError):
                await asyncio.wait_for(peer.receive("cancel-me"), 0.1)
            await peer.send(request("notifications/cancelled", None, {"requestId": "cancel-me"}))
            self.assertIn("result", await peer.rpc("ping"))
            self.assertEqual(await peer.finish(), (0, b"", b""))
        async with start_fixture(self.exe) as peer:
            await peer.initialize()
            await peer.send(request("tools/call", "close-with-eof", wait))
            self.assertEqual((await peer.call())["result"]["structuredContent"]["call"], 2)
            # rmcp gives accepted requests a five-second response drain on EOF.
            self.assertEqual(await peer.finish(timeout=7), (0, b"", b""))

    def test_broken_output_with_input_open(self):
        with subprocess.Popen(
            [self.exe, "--protocol-fixture"], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, creationflags=CREATE_FLAGS,
        ) as child:
            child.stdout.close()
            child.stdin.write(request("tools/list", 1, {"_meta": metadata()}))
            child.stdin.flush()
            try:
                self.assertEqual(child.wait(timeout=TIMEOUT), 1)
                self.assertIn(b"MCP stdio output failed", child.stderr.read())
            finally:
                if child.poll() is None:
                    child.stdin.close()
                    try:
                        child.wait(timeout=TIMEOUT)
                    except subprocess.TimeoutExpired:
                        child.kill()

    def test_requires_explicit_fixture_flag(self):
        result = subprocess.run([self.exe], capture_output=True, timeout=TIMEOUT, creationflags=CREATE_FLAGS)
        self.assertEqual(result.returncode, 2)
        self.assertFalse(result.stdout)

    async def test_python_sdk(self):
        from mcp import Client, StdioServerParameters

        config = StdioServerParameters(command=str(self.exe), args=["--protocol-fixture"])
        for mode in ("legacy", MODERN):
            with self.subTest(mode=mode):
                async with Client(config, mode=mode, read_timeout_seconds=TIMEOUT) as client:
                    self.assertEqual([tool.name for tool in (await client.list_tools()).tools], [TOOL])
                    rejected = await client.call_tool(TOOL, {"mode": "error"})
                    self.assertTrue(rejected.is_error)
                    self.assertEqual(rejected.structured_content["error"], "fixture_rejected")
                    captured = await client.call_tool(TOOL, {"mode": "image"})
                    self.assertEqual([item.type for item in captured.content], ["text", "image"])


async def sample_startup(exe):
    samples = []
    for _ in range(11):
        start = time.perf_counter()
        async with start_fixture(exe) as peer:
            listing = await peer.rpc("tools/list", {"_meta": metadata()})
            assert listing["result"]["tools"][0]["name"] == TOOL
            samples.append(round((time.perf_counter() - start) * 1000, 2))
            assert await peer.finish() == (0, b"", b"")
    print(json.dumps({"exe_sha256": hashlib.sha256(exe.read_bytes()).hexdigest(),
                      "process_start_to_tools_list_ms": samples}, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--sample-startup", action="store_true")
    args, tests = parser.parse_known_args()
    StdioTests.exe = args.exe.resolve(strict=True)
    if args.sample_startup:
        asyncio.run(sample_startup(StdioTests.exe))
    else:
        unittest.main(argv=[__file__, *tests])
