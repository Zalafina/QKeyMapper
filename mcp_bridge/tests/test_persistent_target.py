"""Stable target derivation and v1/v2 separation; no real login change or credential writes."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import winreg

import pywintypes

from qkeymapper_mcp.bridge import Bridge
from qkeymapper_mcp.credentials import CredentialStore
from qkeymapper_mcp.protocol import BridgeError
from qkeymapper_mcp.windows import persistent_target


class PersistentTargetTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="qkm-persist137-")
        self.addCleanup(self.temporary.cleanup)
        self.executable = Path(self.temporary.name) / "演示 target 日本語.exe"
        self.executable.write_bytes(b"context fixture")
        self.discovery = Path(self.temporary.name) / "instance.json"
        self.machine = "00112233-4455-6677-8899-aabbccddeeff"
        registry = patch("qkeymapper_mcp.credentials.winreg.QueryValueEx", return_value=(self.machine, winreg.REG_SZ))
        registry.start()
        self.addCleanup(registry.stop)

    def test_session_is_only_a_runtime_namespace(self):
        first, binding = persistent_target(self.executable, 1)
        for session in (0, 2, 0xFFFFFFFF):
            other, other_binding = persistent_target(self.executable, session)
            self.assertEqual(binding, other_binding)
            self.assertEqual(first.parents[2], other.parents[2])
            self.assertEqual(other.parent.name, str(session))
            self.assertNotEqual(first, other)
        self.assertFalse(first.parents[2].exists(), "Derivation must not create a target store.")

    def test_alias_and_other_path(self):
        first = persistent_target(self.executable, 1)
        if Path(str(self.executable).upper()).exists():
            self.assertEqual(first, persistent_target(Path(str(self.executable).upper()), 1))
        other = self.executable.with_name("other.exe")
        other.write_bytes(self.executable.read_bytes())
        self.assertNotEqual(first[1], persistent_target(other, 1)[1])

    def test_invalid_targets(self):
        for executable, session in ((self.executable.parent, 1), (self.executable.with_name("missing.exe"), 1),
                                    (Path("relative.exe"), 1), (self.executable, -1), (self.executable, 0x100000000),
                                    (self.executable, True)):
            with self.subTest(executable=executable, session=session), self.assertRaises(BridgeError):
                persistent_target(executable, session)

    def test_v1_unchanged_and_v2_not_session_bound(self):
        old = CredentialStore(self.discovery, self.executable, 1)
        expected = ("QKeyMapper-MCP-Bridge/v1\0" + self.machine + "\0" + os.path.normcase(str(self.discovery))
                    + "\0" + os.path.normcase(str(self.executable)) + "\0" + "1").encode("utf-8")
        self.assertEqual(old.target, "QKeyMapper.MCP.Bridge/" + hashlib.sha256(expected).hexdigest())
        first = CredentialStore(self.discovery, self.executable, 1, persistent=True)
        second = CredentialStore(self.discovery.with_name("unused.json"), self.executable, 2, persistent=True)
        self.assertEqual(first.target, second.target)
        self.assertEqual(first._entropy, second._entropy)
        self.assertTrue(first.target.startswith("QKeyMapper.MCP.Bridge/v2/"))
        self.assertNotEqual(old.target, first.target)
        self.assertNotEqual(old.target, CredentialStore(self.discovery, self.executable, 2).target)
        with patch("qkeymapper_mcp.credentials.winreg.QueryValueEx", return_value=("another-machine", winreg.REG_SZ)):
            self.assertNotEqual(first.target, CredentialStore(self.discovery, self.executable, 1, persistent=True).target)

    def test_missing_v2_does_not_read_or_change_legacy_credentials(self):
        store = CredentialStore(self.discovery, self.executable, 1, persistent=True)
        with patch("qkeymapper_mcp.credentials.win32cred.CredRead", side_effect=pywintypes.error(1168, "CredRead", "fixture")) as read, \
                patch("qkeymapper_mcp.credentials.win32cred.CredWrite") as write, \
                patch("qkeymapper_mcp.credentials.win32cred.CredDelete") as delete:
            self.assertIsNone(store.load())
            read.assert_called_once_with(store.target, 1)
            write.assert_not_called()
            delete.assert_not_called()

    def test_bridge_resolves_only_explicit_mode_and_reports_missing_pairing(self):
        bridge = Bridge(None, self.executable, persistent_target=True)
        self.addCleanup(bridge.close)
        resolved, _ = persistent_target(self.executable, 37)
        with patch("qkeymapper_mcp.bridge.identity", return_value=SimpleNamespace(session=37)), \
                patch("qkeymapper_mcp.bridge.discover") as discover, \
                patch("qkeymapper_mcp.bridge.CredentialStore") as store, patch("qkeymapper_mcp.bridge.Pipe") as pipe:
            store.return_value.load.return_value = None
            result = bridge._connect(False)
            self.assertEqual(result["status"], "pairing_required")
            self.assertEqual(bridge.discovery, resolved)
            store.assert_called_once_with(resolved, self.executable, 37, persistent=True)
            self.assertEqual(discover.call_args.args[:2], (resolved, self.executable))
            pipe.assert_not_called()
        with self.assertRaises(ValueError):
            Bridge(self.discovery, self.executable, persistent_target=True)
        with self.assertRaises(ValueError):
            Bridge(None, self.executable)

    def test_native_cpp_contract(self):
        executable = Path(os.environ.get("QKM_IPC_TEST_EXE", ""))
        if not executable.is_file():
            self.skipTest("Set QKM_IPC_TEST_EXE to the matching built IPC component.")
        for target in (executable, self.executable):
            for session in (0, 1, 2, 0xFFFFFFFF):
                with self.subTest(target=target, session=session):
                    native = subprocess.run([str(executable), "--target-context", str(target), str(session)],
                        capture_output=True, timeout=10, check=True)
                    record = json.loads(native.stdout)
                    discovery, binding = persistent_target(target, session)
                    self.assertEqual(record["version"], 2)
                    self.assertEqual(record["binding"], binding.hex())
                    self.assertEqual(os.path.normcase(record["pair_directory"]), os.path.normcase(str(discovery.parents[2])))
                    self.assertEqual(os.path.normcase(record["runtime_directory"]), os.path.normcase(str(discovery.parent)))


if __name__ == "__main__":
    unittest.main()
