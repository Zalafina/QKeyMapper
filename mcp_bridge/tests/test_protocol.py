import asyncio
import os
import threading
import time
import unittest
from dataclasses import replace
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock, patch

from qkeymapper_mcp.protocol import BridgeError, Credential, client_name, decimal, decode, encode, proof


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
        for key, item in (("scopes", True), ("scopes", 2), ("scopes", 129), ("secret", "FA" * 32), ("extra", 1)):
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

    def test_read_rpc_errors_keep_application_failures_distinct(self):
        from qkeymapper_mcp.bridge import Bridge
        cases = (({"ok": True, "result": []}, "invalid_message", "failed"),
                 ({"ok": True, "result": {}, "extra": True}, "invalid_message", "failed"),
                 ({"ok": "true", "result": {}}, "invalid_message", "failed"),
                 ({"ok": False, "error": {"code": "context_busy", "message": "Not ready."}},
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
                    self.assertEqual(asyncio.run(bridge.get_state()), {"ok": False, "error": error})
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


if __name__ == "__main__":
    unittest.main()
