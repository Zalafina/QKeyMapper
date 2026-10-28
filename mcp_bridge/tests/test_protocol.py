import asyncio
import os
import threading
import time
import unittest
from dataclasses import replace
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import AsyncMock, Mock, patch

from qkeymapper_mcp.protocol import KNOWN_SCOPES, BridgeError, Credential, client_name, decimal, decode, encode, proof


class ProtocolTests(unittest.TestCase):
    def test_fixed_proofs(self):
        client = SimpleNamespace(pid=0x12345678, created=0xFEDCBA9876543210)
        server = SimpleNamespace(pid=0x87654321, created=0x0123456789ABCDEF)
        args = ("00112233445566778899aabbccddeeff", "ffeeddccbbaa99887766554433221100", (1 << 64) - 1,
                client, server, bytes(range(32)), bytes(range(32, 64)), bytes(range(1, 33)))
        vectors = ("707f9b7b991818ffac75ed543bf43e5bbc13beaa56a821a496207772369ca8b0",
                   "832a59307600f9ca81e2a58f282f54ecd6f492e8c149662a15371ac05e359f38",
                   "d0846e2424fc8034f6723ac5d5e7bdc951af20a7de76f1f3a42485fa9a3b824b")
        for role, expected in enumerate(vectors, 1):
            self.assertEqual(proof(role, *args).hex(), expected)

    def test_json_round_trip(self):
        value = {"中文": [True, None, "日本語", {"key": 1.5, "nul": "x\0y"}]}
        frame = encode(value)
        self.assertEqual(int.from_bytes(frame[:4], "big"), len(frame) - 4)
        self.assertEqual(decode(frame[4:]), value)

    def test_invalid_json(self):
        invalid = (b"", b"[]", b"{}{}", b'{"x":1,"x":2}', b'{"x":NaN}', b'{"x":1e999}',
                   b'\xef\xbb\xbf{}', b'{"x":"\\ud800"}',
                   b'{"x":"\xff"}', b'{"x":' + b'[' * 33 + b'0' + b']' * 33 + b'}')
        for data in invalid:
            with self.subTest(data=data), self.assertRaises(BridgeError):
                decode(data)
        with self.assertRaises(BridgeError):
            encode({"x": float("inf")})
        with self.assertRaises(BridgeError):
            encode({1: "invalid key"})
        with self.assertRaises(BridgeError):
            encode({"x": "a" * 4096}, 4096)

    def test_canonical_integer(self):
        self.assertEqual(decimal(str((1 << 64) - 1), 64), (1 << 64) - 1)
        for value in (True, 1, 1.0, "01", "-1", "+1", "1 ", "1e1", "0", str(1 << 64), "９"):
            with self.subTest(value=value), self.assertRaises(BridgeError):
                decimal(value, 64)
        self.assertEqual(decimal("0", 32, zero=True), 0)

    def test_credential_validation_and_repr(self):
        value = {"pair_id": "a" * 32, "epoch": "1", "scopes": 1, "secret": "f1" * 32}
        credential = Credential.from_json(value)
        self.assertNotIn(value["secret"], repr(credential))
        self.assertEqual(credential.to_json(), value)
        for mask in (1, 127, 1 | 128, 1 | 256, KNOWN_SCOPES):
            with self.subTest(mask=mask):
                changed = {**value, "scopes": mask}
                self.assertEqual(Credential.from_json(changed).to_json(), changed)
        for key, item in (("scopes", True), ("scopes", 2), ("scopes", 128), ("scopes", 256),
                          ("scopes", KNOWN_SCOPES + 2), ("secret", "FA" * 32), ("extra", 1)):
            with self.subTest(key=key, item=item), self.assertRaises(BridgeError):
                Credential.from_json({**value, key: item})

    def test_client_names(self):
        client_name("普通测试 日本語")
        for name in (" ", "a\n", "a\u202e", "a" * 81, "\ud800"):
            with self.subTest(name=name), self.assertRaises(BridgeError):
                client_name(name)


@unittest.skipUnless(os.name == "nt", "Windows native APIs")
class NativeTests(unittest.TestCase):
    def test_linked_token_query_failure_never_supplies_a_link(self):
        import pywintypes
        from qkeymapper_mcp import windows
        with patch.object(windows.win32security, "GetTokenInformation", side_effect=pywintypes.error(5, "test", "denied")):
            self.assertEqual(windows._linked_authentication(object(), "user", "logon", 1), 0)

    def test_linked_login_requires_reciprocal_os_identity(self):
        from qkeymapper_mcp.windows import Identity
        first = Identity(10, 100, 1, "user", "logon", 1, "client.exe")
        second = replace(first, authentication=2)
        self.assertFalse(first.same_login(second))
        first = replace(first, linked_authentication=2)
        self.assertFalse(first.same_login(second))
        second = replace(second, linked_authentication=1)
        self.assertTrue(first.same_login(second))
        self.assertTrue(second.same_login(first))
        for changed in (replace(second, user="other"), replace(second, logon="other"),
                        replace(second, session=2), replace(second, linked_authentication=3)):
            self.assertFalse(first.same_login(changed))

    def test_challenge_identity_schema_and_role_rejection(self):
        from qkeymapper_mcp import bridge as module
        from qkeymapper_mcp.windows import Discovery, Identity
        own = Identity(10, 100, 1, "user", "logon", 1, "client.exe")
        server = Identity(20, 200, 1, "user", "logon", 1, "server.exe")
        target = Discovery("a" * 32, "unused", server)
        credential = Credential("b" * 32, 1, 1, b"k" * 32)
        nonce, remote_nonce = b"c" * 32, b"s" * 32
        args = (target.instance, credential.pair_id, 1, own, server, nonce, remote_nonce, credential.secret)
        message = {"type": "challenge", "version": 1, "instance": target.instance,
            "pair_id": credential.pair_id, "epoch": "1", "server_nonce": remote_nonce.hex(),
            "server_proof": proof(1, *args).hex(), "client_pid": "10", "client_created": "100",
            "server_pid": "20", "server_created": "200"}
        for change in ({"extra": True}, {"version": True}, {"server_pid": "21"},
                       {"client_created": "101"}, {"epoch": "0"}, {"server_proof": proof(2, *args).hex()}):
            with self.subTest(change=change):
                store, pipe = Mock(), Mock()
                store.load.return_value = credential
                pipe.receive.return_value = {**message, **change}
                bridge = module.Bridge(Path("unused/instance.json"), Path("unused.exe"))
                try:
                    with patch.object(module, "identity", return_value=own), patch.object(module, "discover", return_value=target), \
                         patch.object(module, "CredentialStore", return_value=store), patch.object(module, "Pipe", return_value=pipe), \
                         patch.object(module.secrets, "token_bytes", return_value=nonce):
                        with self.assertRaises(BridgeError):
                            bridge._connect(False)
                    self.assertEqual(pipe.send.call_count, 1)
                    store.save.assert_not_called()
                    self.assertNotEqual(bridge.state()["status"], "connected")
                finally:
                    bridge.close()

    def test_window_permissions_reconnect_keeps_pairing(self):
        from qkeymapper_mcp import bridge as module
        from qkeymapper_mcp.windows import Discovery, Identity
        own = Identity(10, 100, 1, "user", "logon", 1, "client.exe")
        server = Identity(20, 200, 1, "user", "logon", 1, "server.exe")
        target = Discovery("a" * 32, "unused", server)
        original = Credential("b" * 32, 1, 1, b"k" * 32)
        nonce, remote_nonce = b"c" * 32, b"s" * 32
        args = (target.instance, original.pair_id, 2, own, server, nonce, remote_nonce, original.secret)
        challenge = {"type": "challenge", "version": 1, "instance": target.instance,
            "pair_id": original.pair_id, "epoch": "2", "server_nonce": remote_nonce.hex(),
            "server_proof": proof(1, *args).hex(), "client_pid": "10", "client_created": "100",
            "server_pid": "20", "server_created": "200"}
        for mask in (1 | 128, 1 | 256, 1 | 128 | 256):
            with self.subTest(mask=mask):
                store, pipe = Mock(), Mock()
                store.load.return_value = original
                pipe.receive.side_effect = [challenge, {"type": "ready", "version": 1,
                    "instance": target.instance, "pair_id": original.pair_id, "epoch": "2", "scopes": mask}]
                bridge = module.Bridge(Path("unused/instance.json"), Path("unused.exe"))
                try:
                    with patch.object(module, "identity", return_value=own), patch.object(module, "discover", return_value=target), \
                         patch.object(module, "CredentialStore", return_value=store), patch.object(module, "Pipe", return_value=pipe), \
                         patch.object(module.secrets, "token_bytes", return_value=nonce):
                        self.assertEqual(bridge._connect(False)["scopes"], mask)
                    store.save.assert_called_once_with(Credential(original.pair_id, 2, mask, original.secret))
                    self.assertEqual(pipe.send.call_args_list[0].args[0]["type"], "hello")
                    self.assertEqual(pipe.send.call_args_list[1].args[0]["type"], "proof")
                finally:
                    bridge.close()

    def test_identity_and_image(self):
        from qkeymapper_mcp.windows import file_identity, identity, pristine_thread
        pristine_thread()
        own = identity(os.getpid())
        self.assertGreater(own.created, 0)
        self.assertEqual(own, identity(os.getpid()))
        self.assertTrue(own.same_login(identity(os.getpid())))
        self.assertEqual(len(file_identity(own.executable)[1]), 16)

    def test_bounded_worker_and_repeated_close(self):
        from qkeymapper_mcp.bridge import Bridge
        bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
        entered, release = threading.Event(), threading.Event()
        def operation():
            entered.set()
            release.wait(2)
        try:
            future = bridge._submit(operation)
            self.assertTrue(entered.wait(1))
            self.assertFalse(bridge.connect()["accepted"])
            self.assertEqual(asyncio.run(bridge.get_state()), {"ok": False, "error": "busy_or_closed"})
            self.assertEqual(asyncio.run(bridge.query("tabs")), {"ok": False, "error": "busy_or_closed"})
            bridge.disconnect()
            bridge.disconnect()
            self.assertFalse(bridge.connect()["accepted"])
            release.set()
            future.result(2)
            deadline = time.monotonic() + 2
            while bridge.state()["busy"] and time.monotonic() < deadline:
                time.sleep(0.01)
            self.assertEqual(bridge.state()["status"], "disconnected")
            self.assertFalse(bridge.state()["busy"])
        finally:
            release.set()
            bridge.close()
        bridge.close()
        self.assertFalse(bridge.connect()["accepted"])

    def test_cancel_before_worker_starts_recovers_admission(self):
        from qkeymapper_mcp.bridge import Bridge
        bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
        entered, release = threading.Event(), threading.Event()
        def block_worker():
            entered.set()
            release.wait(5)
        blocker = bridge._worker.submit(block_worker)
        try:
            self.assertTrue(entered.wait(1))
            async def exercise():
                with patch.object(bridge, "_request", return_value={"ok": True}) as request:
                    task = asyncio.create_task(bridge.get_state())
                    await asyncio.sleep(0)
                    self.assertTrue(bridge.state()["busy"])
                    task.cancel()
                    with self.assertRaises(asyncio.CancelledError):
                        await task
                    await asyncio.sleep(0)
                    self.assertFalse(bridge.state()["busy"])
                    request.assert_not_called()
                    release.set()
                    blocker.result(1)
                    self.assertEqual(await bridge.get_state(), {"ok": True})
                    self.assertEqual(request.call_count, 1)
            asyncio.run(exercise())
        finally:
            release.set()
            bridge.close()

    def test_running_cancel_stays_busy_until_cleanup(self):
        import win32event
        from qkeymapper_mcp.bridge import Bridge
        bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
        entered, cancelled, drained = threading.Event(), threading.Event(), threading.Event()
        def request(*_args):
            entered.set()
            self.assertEqual(win32event.WaitForSingleObject(bridge._cancel, 3000), win32event.WAIT_OBJECT_0)
            cancelled.set()
            drained.wait(5)
            raise BridgeError("cancelled")
        try:
            async def exercise():
                with patch.object(bridge, "_request", side_effect=request) as native:
                    task = asyncio.create_task(bridge.get_state())
                    await asyncio.sleep(0)
                    self.assertTrue(entered.wait(1))
                    task.cancel()
                    with self.assertRaises(asyncio.CancelledError):
                        await task
                    self.assertTrue(cancelled.wait(1))
                    self.assertTrue(bridge.state()["busy"])
                    self.assertEqual(await bridge.get_state(), {"ok": False, "error": "busy_or_closed"})
                    self.assertEqual(native.call_count, 1)
                    bridge.disconnect()
                    bridge.disconnect()
                    self.assertFalse(bridge.connect()["accepted"])
                    drained.set()
                    bridge._worker.submit(lambda: None).result(2)
                    self.assertFalse(bridge.state()["busy"])
                    self.assertEqual(bridge.state()["status"], "disconnected")
                    with patch.object(bridge, "_connect", return_value={"status": "connected"}):
                        self.assertTrue(bridge.connect()["accepted"])
                        bridge._worker.submit(lambda: None).result(1)
            asyncio.run(exercise())
        finally:
            drained.set()
            bridge.close()
        bridge.close()

    def test_late_coroutine_cancel_does_not_cancel_next_job(self):
        import win32event
        from qkeymapper_mcp.bridge import Bridge
        bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
        entered, release = threading.Event(), threading.Event()
        def next_job():
            entered.set()
            release.wait(5)
        try:
            async def exercise():
                delayed_delivery = asyncio.get_running_loop().create_future()
                with patch.object(bridge, "_request", return_value={"ok": True}), \
                     patch("qkeymapper_mcp.bridge.asyncio.wrap_future", return_value=delayed_delivery) as wrap:
                    task = asyncio.create_task(bridge.get_state())
                    await asyncio.sleep(0)
                    wrap.call_args.args[0].result(1)
                    bridge._worker.submit(lambda: None).result(1)
                    next_future = bridge._submit(next_job)
                    self.assertIsNotNone(next_future)
                    self.assertTrue(entered.wait(1))
                    task.cancel()
                    with self.assertRaises(asyncio.CancelledError):
                        await task
                    self.assertEqual(win32event.WaitForSingleObject(bridge._cancel, 0), win32event.WAIT_TIMEOUT)
                    self.assertTrue(bridge.state()["busy"])
                    release.set()
                    next_future.result(1)
            asyncio.run(exercise())
        finally:
            release.set()
            bridge.close()

    def test_submit_failure_restores_admission(self):
        from qkeymapper_mcp.bridge import Bridge
        bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
        try:
            with patch.object(bridge._worker, "submit", side_effect=RuntimeError("fixture submit failure")):
                with self.assertRaisesRegex(RuntimeError, "fixture submit failure"):
                    bridge._submit(lambda: None)
            self.assertFalse(bridge.state()["busy"])
            future = bridge._submit(lambda: "next")
            self.assertIsNotNone(future)
            self.assertEqual(future.result(1), "next")
        finally:
            bridge.close()

    def test_already_done_callback_is_registered_outside_lock(self):
        from concurrent.futures import Future
        from qkeymapper_mcp.bridge import Bridge
        bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
        completed = Future()
        completed.set_result("done")
        add_callback = completed.add_done_callback
        def register(callback):
            self.assertTrue(bridge._lock.acquire(blocking=False))
            bridge._lock.release()
            add_callback(callback)
        try:
            with patch.object(bridge._worker, "submit", return_value=completed), \
                 patch.object(completed, "add_done_callback", side_effect=register) as callback:
                self.assertIs(bridge._submit(lambda: None), completed)
                callback.assert_called_once()
            self.assertFalse(bridge.state()["busy"])
        finally:
            bridge.close()

    def test_fixed_read_rpc_dispatch(self):
        from qkeymapper_mcp.bridge import Bridge
        for entry, method in (("get_capabilities", "qkm_get_capabilities"), ("get_state", "qkm_get_state")):
            with self.subTest(entry=entry):
                bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
                pipe = Mock()
                pipe.discovery.instance = "a" * 32
                pipe.receive.side_effect = lambda _deadline: {"id": pipe.send.call_args.args[0]["id"],
                    "ok": True, "result": {"native": "value"}}
                bridge._pipe = pipe
                bridge._credential = Credential("b" * 32, 1, 1, b"k" * 32)
                try:
                    result = asyncio.run(getattr(bridge, entry)())
                    self.assertTrue(result["ok"])
                    self.assertEqual(result["result"], {"native": "value"})
                    self.assertEqual(pipe.send.call_args.args[0]["method"], method)
                    self.assertEqual(pipe.send.call_args.args[0]["arguments"], {})
                    self.assertEqual(bridge.state()["status"], "connected")
                    self.assertNotIn("business_capabilities", bridge.state())
                finally:
                    bridge.close()

    def test_catalog_query_rpc_and_pagination_arguments(self):
        from qkeymapper_mcp.bridge import Bridge
        bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
        pipe = Mock()
        pipe.discovery.instance = "a" * 32
        pipe.receive.side_effect = lambda _deadline: {"id": pipe.send.call_args.args[0]["id"],
            "ok": True, "result": {"items": []}}
        bridge._pipe = pipe
        bridge._credential = Credential("b" * 32, 1, 1, b"k" * 32)
        try:
            for entity, arguments, expected in (("tabs", {}, {"entity": "tabs", "offset": 0, "limit": 50}),
                    ("tabs", {"catalog_revision": "c" * 64, "offset": 1, "limit": 1},
                     {"entity": "tabs", "catalog_revision": "c" * 64, "offset": 1, "limit": 1}),
                    ("mappings", {"source_revision": "d" * 64, "source_tab_ref": "d" * 64 + ":source:1", "offset": 2},
                     {"entity": "mappings", "source_revision": "d" * 64, "source_tab_ref": "d" * 64 + ":source:1", "offset": 2, "limit": 50})):
                self.assertTrue(asyncio.run(bridge.query(entity, **arguments))["ok"])
                request = pipe.send.call_args.args[0]
                self.assertEqual(request["method"], "qkm_query")
                self.assertEqual(request["arguments"], expected)
        finally:
            bridge.close()

    def test_read_rpc_errors_keep_application_failures_distinct(self):
        from qkeymapper_mcp.bridge import Bridge
        cases = (({"ok": True, "result": []}, "invalid_message", "failed"),
                 ({"ok": True, "result": {}, "extra": True}, "invalid_message", "failed"),
                 ({"ok": "true", "result": {}}, "invalid_message", "failed"),
                 ({"ok": False, "error": {"code": "bad code", "message": "Rejected."}}, "invalid_message", "failed"),
                 ({"ok": False, "error": {"code": "x" * 65, "message": "Rejected."}}, "invalid_message", "failed"),
                 ({"ok": False, "error": {"code": "context_busy", "message": []}}, "invalid_message", "failed"),
                 ({"ok": False, "error": {"code": "context_busy", "message": "x" * 1025}}, "invalid_message", "failed"),
                 ({"ok": False, "error": {"code": "context_busy", "message": "键" * 342}}, "invalid_message", "failed"),
                 ({"ok": False, "error": {"code": "context_busy", "message": "Not ready."}},
                  "context_busy", "connected"),
                 ({"ok": False, "error": {"code": "context_busy", "message": "x" * 1024}},
                  "context_busy", "connected"))
        for response, error, status in cases:
            with self.subTest(response=response):
                bridge = Bridge(Path("unused/instance.json"), Path("unused.exe"))
                pipe = Mock()
                pipe.discovery.instance = "a" * 32
                pipe.receive.side_effect = lambda _deadline: {"id": pipe.send.call_args.args[0]["id"], **response}
                bridge._pipe = pipe
                bridge._credential = Credential("b" * 32, 1, 1, b"k" * 32)
                try:
                    expected = {"ok": False, "error": error}
                    if status == "connected":
                        expected["message"] = response["error"]["message"]
                    self.assertEqual(asyncio.run(bridge.get_state()), expected)
                    self.assertEqual(bridge.state()["status"], status)
                finally:
                    bridge.close()

    def test_impersonated_worker_is_rejected(self):
        from concurrent.futures import ThreadPoolExecutor
        import win32security
        from qkeymapper_mcp.windows import pristine_thread
        def check():
            win32security.ImpersonateSelf(win32security.SecurityIdentification)
            try:
                with self.assertRaises(BridgeError):
                    pristine_thread()
            finally:
                win32security.RevertToSelf()
            pristine_thread()
        with ThreadPoolExecutor(max_workers=1) as worker:
            worker.submit(check).result(2)


class PreviewForwardingTests(unittest.IsolatedAsyncioTestCase):
    async def test_original_mapping_wire_shape_and_explicit_save_branch(self):
        import sys
        from qkeymapper_mcp.bridge import Bridge
        bridge = Bridge(Path("unused/instance.json"), Path(sys.executable))
        with patch.object(bridge, "_call", new_callable=AsyncMock, return_value={"ok": True}) as call:
            revision = "a" * 64
            await bridge.preview_changes(revision, revision + ":source:0", "A", "B")
            call.assert_awaited_with("qkm_preview_changes", dict(source_revision=revision, source_tab_ref=revision + ":source:0",
                original_key="A", mapping_key="B", insert_row=None))
            await bridge.preview_changes(action="configuration.save", setting_name="配置95")
            call.assert_awaited_with("qkm_preview_changes", {"action": "configuration.save", "setting_name": "配置95"})
            await bridge.preview_changes(original_key="A", action="configuration.save", setting_name="配置95")
            self.assertIn("original_key", call.await_args.args[1])
            self.assertIn("setting_name", call.await_args.args[1])
            await bridge.apply_changes("a" * 32)
            call.assert_awaited_with("qkm_apply_changes", {"preview_id": "a" * 32, "confirm_conflicts": False})
            await bridge.apply_changes("b" * 32, confirm_scope=True, confirm_overwrite=True)
            call.assert_awaited_with("qkm_apply_changes", {"preview_id": "b" * 32, "confirm_conflicts": False,
                "confirm_scope": True, "confirm_overwrite": True})
            await bridge.apply_changes("c" * 32, confirm_scope="false", confirm_overwrite=1)
            call.assert_awaited_with("qkm_apply_changes", {"preview_id": "c" * 32, "confirm_conflicts": False,
                "confirm_scope": "false", "confirm_overwrite": 1})


if __name__ == "__main__":
    unittest.main()
