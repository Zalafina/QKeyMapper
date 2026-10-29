"""Offline Windows x64 bridge bundle using the fixed CPython embeddable distribution."""

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import struct
import subprocess
import sys
import zipfile


ROOT = Path(__file__).resolve().parents[2]
RUNTIME_NAME = "python-3.11.9-embed-amd64.zip"
RUNTIME_SIZE = 11249023
RUNTIME_SHA256 = "009d6bf7e3b2ddca3d784fa09f90fe54336d5b60f0e0f305c37f400bf83cfd3b"
MAX_ARCHIVE_BYTES = 128 * 1024 * 1024
RESERVED = {"con", "prn", "aux", "nul", *(f"com{i}" for i in range(1, 10)), *(f"lpt{i}" for i in range(1, 10))}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def no_reparse(path):
    path = path.absolute()
    for candidate in (path, *path.parents):
        if candidate.exists() or candidate.is_symlink():
            require(not (candidate.lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT), "Reparse path rejected: " + str(candidate))


def extract(archive, destination):
    no_reparse(destination)
    with zipfile.ZipFile(archive) as source:
        entries = source.infolist()
        require(sum(entry.file_size for entry in entries) <= MAX_ARCHIVE_BYTES, "Archive exceeds the extraction budget")
        for entry in entries:
            # ZipInfo normalizes Windows separators; validate the original member name.
            name = entry.orig_filename.rstrip("/")
            parts = name.split("/")
            require(name and "\\" not in name and ":" not in name and "\x00" not in name
                    and not PurePosixPath(name).is_absolute() and all(part not in ("", ".", "..")
                    and part[-1] not in (".", " ") and part.split(".", 1)[0].casefold() not in RESERVED for part in parts),
                    "Unsafe archive member: " + entry.filename)
            require(not stat.S_ISLNK(entry.external_attr >> 16), "Archive symlink rejected")
            target = destination.joinpath(*parts)
            no_reparse(target)
            if entry.is_dir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with source.open(entry) as incoming, target.open("xb") as outgoing:
                    shutil.copyfileobj(incoming, outgoing)


def locked_wheels(requirements, directory):
    selected = []
    names = set()
    for line in requirements.read_text(encoding="utf-8").splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        match = re.fullmatch(r"([A-Za-z0-9_.-]+)==([^\s]+) --hash=sha256:([0-9a-f]{64})", line)
        require(match is not None, "Expected an exact version and one wheel SHA256")
        name, version, expected = match.groups()
        canonical = re.sub(r"[-_.]+", "-", name).lower()
        require(canonical not in names, "Duplicate dependency in runtime lock")
        names.add(canonical)
        candidates = []
        for path in directory.glob("*.whl"):
            fields = path.name.split("-")
            if re.sub(r"[-_.]+", "-", fields[0]).lower() == canonical and fields[1] == version:
                no_reparse(path)
                if sha256(path) == expected:
                    candidates.append(path)
        require(len(candidates) == 1, "Missing or ambiguous locked wheel: " + name + "==" + version)
        selected.append((name, version, candidates[0], expected))
    require(selected, "Empty runtime dependency lock")
    return selected


def verify_signatures(runtime):
    command = r"""
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Security/Microsoft.PowerShell.Security.psd1')
@(foreach ($runtimeFile in @('python.exe', 'python3.dll', 'python311.dll')) {
    $signature = Get-AuthenticodeSignature -LiteralPath (Join-Path $env:QKM_BUNDLE_SIGNATURE_ROOT $runtimeFile)
    if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O=Python Software Foundation') {
        throw ('Runtime signature rejected: ' + $runtimeFile)
    }
    [pscustomobject]@{file=$runtimeFile; status=$signature.Status.ToString(); publisher=$signature.SignerCertificate.Subject}
}) | ConvertTo-Json -Compress
"""
    powershell = Path(os.environ["SystemRoot"]) / "System32/WindowsPowerShell/v1.0/powershell.exe"
    completed = subprocess.run([str(powershell), "-NoProfile", "-NonInteractive", "-Command", command],
        env={**os.environ, "QKM_BUNDLE_SIGNATURE_ROOT": str(runtime), "PSModulePath": str(powershell.parent / "Modules")},
        capture_output=True, encoding="utf-8", errors="replace", timeout=60)
    require(completed.returncode == 0, "Runtime signature validation failed: " + completed.stderr.strip())
    signatures = json.loads(completed.stdout.lstrip("\ufeff"))
    require(len(signatures) == 3 and all(item["status"] == "Valid" for item in signatures), "Incomplete signature validation")
    return signatures


def build(runtime_zip, wheel_directory, output):
    require(os.name == "nt" and sys.version_info[:3] == (3, 11, 9) and struct.calcsize("P") == 8,
            "Build with CPython 3.11.9 Windows x64")
    no_reparse(runtime_zip)
    require(runtime_zip.name == RUNTIME_NAME and runtime_zip.stat().st_size == RUNTIME_SIZE
            and sha256(runtime_zip) == RUNTIME_SHA256, "Embedded runtime identity mismatch")
    wheels = locked_wheels(ROOT / "mcp_bridge/requirements-windows.txt", wheel_directory)
    output = output.absolute()
    archive = output.parent / (output.name + ".zip")
    no_reparse(output); no_reparse(archive)
    require(not output.exists() and not archive.exists(), "Output directory and ZIP must both be new")
    output.mkdir(parents=True)
    runtime = output / "runtime"
    runtime.mkdir()
    extract(runtime_zip, runtime)
    signatures = verify_signatures(runtime)
    packages = runtime / "Lib/site-packages"
    packages.mkdir(parents=True)
    for _, _, wheel, _ in wheels:
        extract(wheel, packages)
    application = packages / "qkeymapper_mcp"
    application.mkdir()
    for source in sorted((ROOT / "mcp_bridge/src/qkeymapper_mcp").glob("*.py")):
        shutil.copyfile(source, application / source.name)
    (runtime / "python311._pth").write_bytes(b"python311.zip\n.\nLib/site-packages\nimport site\n")
    shutil.copyfile(ROOT / "LICENSE", output / "QKeyMapper-LICENSE.txt")
    shutil.copyfile(ROOT / "mcp_bridge/BUNDLE_README.md", output / "README.md")
    files = []
    for path in sorted(output.rglob("*")):
        if path.is_file():
            files.append(dict(path=path.relative_to(output).as_posix(), bytes=path.stat().st_size, sha256=sha256(path)))
    require(sum(item["bytes"] for item in files) <= 512 * 1024 * 1024, "Bundle exceeds the total budget")
    manifest = dict(schema_version=1, python="3.11.9", architecture="windows-x64", runtime_archive=RUNTIME_NAME,
        runtime_sha256=RUNTIME_SHA256, runtime_signatures=signatures,
        dependencies=[dict(name=name, version=version, wheel=path.name, sha256=digest) for name, version, path, digest in wheels], files=files)
    (output / "bundle-manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    with zipfile.ZipFile(archive, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zipped:
        for path in sorted(output.rglob("*")):
            if path.is_file():
                zipped.write(path, output.name + "/" + path.relative_to(output).as_posix())
    return dict(directory=str(output), archive=str(archive), sha256=sha256(archive), dependencies=len(wheels), files=len(files))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime-zip", type=Path, required=True)
    parser.add_argument("--wheel-directory", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    print(json.dumps(build(arguments.runtime_zip, arguments.wheel_directory, arguments.output), ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
