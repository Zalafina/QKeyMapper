"""Check the verified Windows bridge development baseline without changing it."""

import importlib.metadata
import os
import platform
import re
import struct
import sys
import sysconfig
from pathlib import Path


PYTHON_VERSION = (3, 11, 9)
ROOT = Path(__file__).resolve().parent


def normalized(name):
    return re.sub(r"[-_.]+", "-", name).lower()


def check():
    errors = []
    if (os.name != "nt" or platform.python_implementation() != "CPython" or struct.calcsize("P") != 8
            or sysconfig.get_platform() != "win-amd64"):
        errors.append("Use Windows x64 CPython.")
    if sys.version_info[:3] != PYTHON_VERSION:
        errors.append("Python version must be " + ".".join(map(str, PYTHON_VERSION)) + ".")
    config = Path(sys.prefix) / "pyvenv.cfg"
    if sys.prefix == sys.base_prefix or not config.is_file() or not re.search(
            r"(?im)^include-system-site-packages\s*=\s*false\s*$", config.read_text(encoding="utf-8")):
        errors.append("Use a venv isolated from system site-packages.")
    expected = {}
    for filename in ("requirements-windows.txt", "requirements-dev-windows.txt"):
        for line in (ROOT / filename).read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if not line or line.startswith("#") or line == "-r requirements-windows.txt":
                continue
            match = re.fullmatch(r"([A-Za-z0-9_.-]+)==([^\s]+)", line)
            if not match:
                raise ValueError("Unpinned requirement in " + filename)
            name, version = normalized(match[1]), match[2]
            if name in expected and expected[name] != version:
                raise ValueError("Conflicting pinned requirement: " + name)
            expected[name] = version
    actual = {}
    for distribution in importlib.metadata.distributions():
        name = normalized(distribution.metadata["Name"])
        if name in actual:
            errors.append("Duplicate distribution: " + name)
        actual[name] = distribution.version
    for name in sorted(expected.keys() | actual.keys()):
        if actual.get(name) != expected.get(name):
            errors.append(f"{name}: expected {expected.get(name, 'absent')}, found {actual.get(name, 'missing')}")
    return errors


if __name__ == "__main__":
    failures = check()
    for failure in failures:
        print("FAIL", failure)
    if failures:
        sys.exit(1)
    print("PASS Windows x64 CPython 3.11.9; isolated venv; all runtime and development package versions match.")
