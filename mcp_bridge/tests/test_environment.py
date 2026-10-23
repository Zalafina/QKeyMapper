"""Check that the development gate catches interpreter and package drift."""

import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import check_environment


class EnvironmentTests(unittest.TestCase):
    def test_current_baseline(self):
        self.assertEqual(check_environment.check(), [])

    def test_wrong_python_patch(self):
        with patch.object(check_environment.sys, "version_info", (3, 11, 10)):
            self.assertTrue(any("Python version" in error for error in check_environment.check()))

    def test_wrong_architecture(self):
        with patch.object(check_environment.sysconfig, "get_platform", return_value="win-arm64"):
            self.assertIn("Use Windows x64 CPython.", check_environment.check())

    def test_requires_venv(self):
        with patch.object(check_environment.sys, "prefix", check_environment.sys.base_prefix):
            self.assertIn("Use a venv isolated from system site-packages.", check_environment.check())

    def test_requires_sha256_pinned_requirements(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "requirements-dev-windows.txt").write_text("-r requirements-windows.txt\n", encoding="utf-8")
            for requirement in ("mcp==2.3.0", "mcp>=2.3.0 --hash=sha256:" + "0" * 64,
                    "mcp==2.3.0 --hash=sha256:1234", "mcp==2.3.0 --hash=sha256:" + "g" * 64,
                    "mcp==2.3.0 --hash=md5:" + "0" * 64):
                with self.subTest(requirement=requirement), patch.object(check_environment, "ROOT", root):
                    (root / "requirements-windows.txt").write_text(requirement + "\n", encoding="utf-8")
                    with self.assertRaisesRegex(ValueError, "Unpinned or unhashed"):
                        check_environment.check()

    def test_package_drift(self):
        distributions = list(check_environment.importlib.metadata.distributions())
        mcp = next(item for item in distributions if check_environment.normalized(item.metadata["Name"]) == "mcp")
        without_mcp = [item for item in distributions if item is not mcp]
        for installed, expected in ((without_mcp, "mcp: expected 2.3.0, found missing"),
                (without_mcp + [SimpleNamespace(metadata={"Name": "mcp"}, version="0.0.0")], "mcp: expected 2.3.0, found 0.0.0"),
                (distributions + [SimpleNamespace(metadata={"Name": "unexpected-package"}, version="1.0")],
                 "unexpected-package: expected absent, found 1.0"),
                (distributions + [SimpleNamespace(metadata={"Name": "MCP"}, version="2.3.0")], "Duplicate distribution: mcp")):
            with self.subTest(expected=expected), patch.object(check_environment.importlib.metadata, "distributions", return_value=installed):
                self.assertIn(expected, check_environment.check())


if __name__ == "__main__":
    unittest.main()
