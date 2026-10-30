"""Explicit SDK loading of non-secret workspace examples, not Agent auto-discovery."""

import asyncio
from contextlib import closing
import importlib
import json
import os
from pathlib import Path
import time
from unittest.mock import patch

import win32api
from mcp import Client, StdioServerParameters

from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.windows import identity
from test_sdk_interop import TOOLS, call, integrity


async def exercise(parameters, output, discovery, exe):
    output = output.resolve(strict=True)
    assert output.is_relative_to(Path(__file__).resolve().parents[2] / "out")
    own = identity(os.getpid())
    store = CredentialStore(discovery, exe, own.session)
    assert store.load() is None, "Do not replace or delete an existing credential."
    calls, spawned = [], []
    config = {"mcpServers": {"qkeymapper": {"command": parameters.command, "args": parameters.args},
                            "unrelated-example": {"command": "unused-example", "args": []}}}
    paths = []
    for name in ("workspace-a", "workspace-b"):
        path = output / name / "mcp-client.json"
        path.parent.mkdir()
        path.write_text(json.dumps(config, ensure_ascii=False, indent=2), encoding="utf-8")
        paths.append(path)
    original = [path.read_bytes() for path in paths]

    def configured(path, expected=parameters):
        data = json.loads(path.read_text(encoding="utf-8"))
        entry = data["mcpServers"]["qkeymapper"]
        assert entry == {"command": expected.command, "args": expected.args}
        return StdioServerParameters(command=entry["command"], args=entry["args"], env=expected.env, cwd=str(path.parent))

    broken = output / "broken.json"
    broken.write_text("{", encoding="utf-8")
    for path, error in ((output / "missing.json", FileNotFoundError), (broken, json.JSONDecodeError)):
        try:
            configured(path)
        except error:
            pass
        else:
            raise AssertionError("Missing or corrupt association must not select another target.")
    assert not spawned
    stdio = importlib.import_module("mcp.client.stdio")
    spawn = stdio._create_platform_compatible_process

    async def observed_spawn(*args, **kwargs):
        process = await spawn(*args, **kwargs)
        try:
            peer = identity(process.pid)
            assert own.same_login(peer) and Path(peer.executable) == Path(parameters.command)
            with closing(win32api.OpenProcess(0x1000, False, peer.pid)) as handle:
                assert integrity(handle) == 8192
            spawned.append(dict(pid=peer.pid, created=str(peer.created), executable=peer.executable, integrity=8192))
        except BaseException:
            process.terminate()
            await process.wait()
            raise
        return process

    async def request(client, label, name, arguments=None):
        value = await call(client, name, arguments)
        calls.append(dict(client=label, tool=name, arguments=arguments, value=value))
        (output / "workspace-calls.json").write_text(json.dumps(calls, ensure_ascii=False, indent=2), encoding="utf-8")
        return value

    async def settled(client, label):
        until = time.monotonic() + 10
        while True:
            state = await request(client, label, "qkm_get_connection_state")
            if not state["busy"]:
                return state
            assert time.monotonic() < until, state
            await asyncio.sleep(.03)

    async def observe(client, label):
        state = await settled(client, label)
        assert state["status"] == "connected" and state["scopes"] == 387
        ready = (await request(client, label, "qkm_get_demo_state"))["result"]
        assert ready["ready"] and ready["demo_session"]["generation"] == 0
        sources = (await request(client, label, "qkm_query", dict(entity="mapping_sources", limit=100)))["result"]
        assert sources["items"][0]["row_count"] == sources["items"][1]["row_count"] == 0
        return ready["target"], sources

    saved = None
    try:
        with patch.object(stdio, "_create_platform_compatible_process", observed_spawn):
            async with Client(configured(paths[0]), read_timeout_seconds=15, cache=None) as first:
                assert {item.name for item in (await first.list_tools()).tools} == TOOLS | {"qkm_get_demo_state", "qkm_demo_mapping"}
                await request(first, "a-first", "qkm_connect")
                assert (await settled(first, "a-first"))["status"] == "pairing_required"
                await request(first, "a-first", "qkm_request_pairing")
                peer_path = output / "case.json.peer"
                until = time.monotonic() + 15
                while not peer_path.is_file():
                    assert time.monotonic() < until, "Own fixture pairing request was not published."
                    await asyncio.sleep(.02)
                peer = json.loads(peer_path.read_text(encoding="utf-8"))
                assert {key: peer[key] for key in ("pid", "created", "executable")} == {
                    "pid": str(spawned[0]["pid"]), "created": spawned[0]["created"], "executable": spawned[0]["executable"]}
                (output / "case.json.client").write_text(json.dumps(peer), encoding="utf-8")
                baseline = await observe(first, "a-first")
                saved = store.load()
                assert saved is not None and saved.scopes == 387
                async with Client(configured(paths[1]), read_timeout_seconds=15, cache=None) as busy:
                    await request(busy, "b-occupied", "qkm_connect")
                    refused = await settled(busy, "b-occupied")
                    assert refused["status"] == "failed" and refused["error"] == "pipe_busy", refused
                assert await observe(first, "a-first-after-busy") == baseline
            for label, path in (("b-shared", paths[1]), ("a-new-session", paths[0])):
                async with Client(configured(path), read_timeout_seconds=15, cache=None) as client:
                    await request(client, label, "qkm_connect")
                    assert await observe(client, label) == baseline
                    assert store.load() == saved
            wrong_args = list(parameters.args)
            wrong_args[wrong_args.index("--executable") + 1] = parameters.command
            wrong = StdioServerParameters(command=parameters.command, args=wrong_args, env=parameters.env)
            path = output / "wrong-target.json"
            path.write_text(json.dumps({"mcpServers": {"qkeymapper": {"command": wrong.command, "args": wrong.args}}}), encoding="utf-8")
            async with Client(configured(path, wrong), read_timeout_seconds=15, cache=None) as client:
                await request(client, "wrong-target", "qkm_connect")
                refused = await settled(client, "wrong-target")
                assert refused["status"] == "failed" and refused["error"] == "executable_rejected", refused
            assert store.load() == saved
        assert len(spawned) == 5 and len({(item["pid"], item["created"]) for item in spawned}) == 5
        assert [path.read_bytes() for path in paths] == original
        assert sum(item["tool"] == "qkm_request_pairing" for item in calls) == 1
        summary = dict(successful_saves=0, scenes=0, native_images=0, schema_errors=0, host_ready=True,
            workspace_session_reuse=True, workspace_sharing="explicit same-target shared pairing; no permission isolation",
            authenticated_sessions=3, pairing_requests=1, busy_preserved_first=True, wrong_target_refused=True,
            missing_and_corrupt_associations_refused=True, client_loader="official SDK explicitly loaded examples; Agent/gateway auto-load unverified",
            credential_unchanged=True, no_business_writes=True)
        (output / "workspace-peers.json").write_text(json.dumps(spawned, indent=2), encoding="utf-8")
        for path in paths + [broken, output / "wrong-target.json", output / "workspace-calls.json", output / "workspace-peers.json"]:
            data = path.read_bytes()
            assert saved.secret not in data and saved.secret.hex().encode() not in data, "Secret in artifact: " + path.name
        (output / "workspace-pass.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
        return summary
    finally:
        store.forget()
