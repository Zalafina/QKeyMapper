"""Offline package input boundaries; native bundle startup is validated separately."""

import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from build_windows_bundle import RUNTIME_NAME, build, extract, locked_wheels


class BundleTests(unittest.TestCase):
    def setUp(self):
        output = Path(__file__).resolve().parents[2] / "out"
        self.temporary = tempfile.TemporaryDirectory(prefix="bundle-unit-", dir=output)
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        assert self.root.resolve().is_relative_to(output.resolve())

    def archive(self, name, data=b"content"):
        path = self.root / "input.zip"
        with zipfile.ZipFile(path, "w") as archive:
            entry = zipfile.ZipInfo(name)
            entry.filename = entry.orig_filename = name
            archive.writestr(entry, data)
        return path

    def test_valid_archive_and_existing_file_is_never_overwritten(self):
        archive = self.archive("package/module.py")
        target = self.root / "output"
        extract(archive, target)
        self.assertEqual((target / "package/module.py").read_bytes(), b"content")
        with self.assertRaises(FileExistsError):
            extract(archive, target)
        self.assertEqual((target / "package/module.py").read_bytes(), b"content")

    def test_traversal_drive_ads_ambiguous_and_device_paths_are_rejected(self):
        for name in ("../escape", "/absolute", "C:/drive", "package\\escape", "package/stream:ads",
                     "package/./escape", "package//escape", "package/space ", "package/dot.", "NUL.txt"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                extract(self.archive(name), self.root / "output")
        self.assertFalse((self.root / "escape").exists())

    def test_archive_symlink_and_crc_damage_are_rejected(self):
        info = zipfile.ZipInfo("linked")
        info.create_system = 3
        info.external_attr = 0o120777 << 16
        path = self.root / "link.zip"
        with zipfile.ZipFile(path, "w") as archive:
            archive.writestr(info, "outside")
        with self.assertRaises(ValueError):
            extract(path, self.root / "output")
        path = self.archive("value.bin", b"unique payload")
        path.write_bytes(path.read_bytes().replace(b"unique payload", b"broken payload"))
        with self.assertRaises(zipfile.BadZipFile):
            extract(path, self.root / "damaged")

    def test_wheel_requires_both_exact_version_and_selected_hash(self):
        wheel = self.root / "demo-1.0-py3-none-any.whl"
        wheel.write_bytes(b"selected wheel")
        digest = hashlib.sha256(wheel.read_bytes()).hexdigest()
        lock = self.root / "requirements.txt"
        lock.write_text("demo==1.0 --hash=sha256:" + digest + "\n", encoding="utf-8")
        self.assertEqual(locked_wheels(lock, self.root)[0][2], wheel)
        wheel.write_bytes(b"tampered wheel")
        with self.assertRaises(ValueError):
            locked_wheels(lock, self.root)
        wheel.write_bytes(b"selected wheel")
        wheel.rename(self.root / "demo-2.0-py3-none-any.whl")
        with self.assertRaises(ValueError):
            locked_wheels(lock, self.root)

    def test_incomplete_and_duplicate_locks_are_rejected(self):
        lock = self.root / "requirements.txt"
        for content in ("", "demo>=1.0\n", "demo==1.0\n", "-r other.txt\n"):
            lock.write_text(content, encoding="utf-8")
            with self.assertRaises(ValueError):
                locked_wheels(lock, self.root)
        wheel = self.root / "demo-1.0-py3-none-any.whl"
        wheel.write_bytes(b"selected wheel")
        entry = "demo==1.0 --hash=sha256:" + hashlib.sha256(wheel.read_bytes()).hexdigest() + "\n"
        lock.write_text(entry + entry, encoding="utf-8")
        with self.assertRaises(ValueError):
            locked_wheels(lock, self.root)

    def test_runtime_identity_fails_before_any_output_is_created(self):
        runtime = self.root / RUNTIME_NAME
        runtime.write_bytes(b"untrusted runtime")
        with self.assertRaisesRegex(ValueError, "identity mismatch"):
            build(runtime, self.root, self.root / "bundle")
        self.assertFalse((self.root / "bundle").exists())

    def test_real_windows_junction_is_not_an_extraction_target(self):
        real = self.root / "real"
        real.mkdir()
        link = self.root / "linked"
        result = subprocess.run([os.environ["COMSPEC"], "/d", "/c", "mklink", "/J", str(link), str(real)],
            capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
        self.assertEqual(result.returncode, 0, result.stderr)
        try:
            with self.assertRaisesRegex(ValueError, "Reparse path rejected"):
                extract(self.archive("escape.py"), link)
            self.assertFalse((real / "escape.py").exists())
        finally:
            link.rmdir()


if __name__ == "__main__":
    unittest.main()
