"""Package a built x64 Rust sidecar and its actual app-local dependencies."""

import argparse
import hashlib
import json
import os
import re
import shutil
import stat
import struct
import subprocess
import zipfile
from html.parser import HTMLParser
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CRATE = ROOT / "mcp_bridge_rust"
TARGET = "x86_64-pc-windows-msvc"
EXE = "qkeymapper-mcp-rust.exe"
STANDALONE_EXE = "qkeymapper_mcp.exe"
MERGED_LICENSE = "QKeyMapper-MCP-Licenses.txt"
CRT = "vcruntime140.dll"
SYSTEM_LIBRARIES = {
    "advapi32.dll", "bcryptprimitives.dll", "combase.dll", "crypt32.dll",
    "kernel32.dll", "ntdll.dll", "shell32.dll",
}

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
            attributes = candidate.lstat().st_file_attributes
            require(not (attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT), f"Reparse path rejected: {candidate}")


def require_x64(path):
    no_reparse(path)
    with path.open("rb") as file:
        header = file.read(64)
        require(len(header) == 64 and header[:2] == b"MZ", f"Invalid Windows image: {path.name}")
        file.seek(struct.unpack_from("<I", header, 60)[0])
        require(file.read(6) == b"PE\0\0\x64\x86", f"Expected a PE x64 image: {path.name}")


def dependencies(executable, dumpbin):
    completed = subprocess.run([str(dumpbin), "/dependents", str(executable)], cwd=ROOT, check=True,
                               capture_output=True, text=True, errors="replace", timeout=30)
    libraries = {line.strip().lower() for line in completed.stdout.splitlines()
                 if re.fullmatch(r"[a-z0-9_.-]+\.dll", line.strip(), re.IGNORECASE)}
    require("kernel32.dll" in libraries, "Could not identify the Windows import list")
    unexpected = {name for name in libraries - SYSTEM_LIBRARIES - {CRT}
                  if not name.startswith(("api-ms-win-core-", "api-ms-win-crt-"))}
    require(not unexpected, "Review additional DLL dependencies: " + ", ".join(sorted(unexpected)))
    return sorted(libraries)


def signed_runtime(path):
    script = r"""
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Security/Microsoft.PowerShell.Security.psd1')
$item = Get-Item -LiteralPath $env:QKM_RUST_REDIST_DLL
$signature = Get-AuthenticodeSignature -LiteralPath $item.FullName
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notlike '*O=Microsoft Corporation*') {
    throw ('Visual C++ runtime signature rejected: ' + $signature.Status)
}
@{publisher=$signature.SignerCertificate.Subject; version=$item.VersionInfo.FileVersion} | ConvertTo-Json -Compress
"""
    executable = Path(os.environ["SystemRoot"]) / "System32/WindowsPowerShell/v1.0/powershell.exe"
    result = subprocess.run([str(executable), "-NoProfile", "-NonInteractive", "-Command", script],
                            env={**os.environ, "QKM_RUST_REDIST_DLL":str(path),
                                 "PSModulePath":str(executable.parent / "Modules")},
                            capture_output=True, encoding="utf-8", errors="replace", timeout=30)
    require(result.returncode == 0, "Runtime signature validation failed: " + result.stderr.strip())
    return json.loads(result.stdout)


def notices(output):
    report = subprocess.run(
        ["cargo", "metadata", "--manifest-path", str(CRATE / "Cargo.toml"), "--format-version", "1",
         "--filter-platform", TARGET, "--locked", "--offline"],
        cwd=CRATE, check=True, capture_output=True, encoding="utf-8", timeout=45)
    metadata = json.loads(report.stdout)
    resolved = {node["id"] for node in metadata["resolve"]["nodes"]}
    dependencies = []
    for package in sorted(metadata["packages"], key=lambda item: (item["name"], item["version"])):
        if package["id"] not in resolved or package["source"] is None:
            continue
        require(package["source"].startswith("registry+"), f"Review license of {package['name']}")
        source = Path(package["manifest_path"]).parent
        name, version = package["name"], package["version"]
        files = {path for path in source.iterdir() if path.is_file() and path.name.upper().startswith(
            ("LICENSE", "LICENCE", "NOTICE", "COPYING", "COPYRIGHT", "UNLICENSE"))}
        if package["license_file"] is not None:
            files.add(source / package["license_file"])
        require(files, f"Missing original license notices for {name} {version}")
        directory = output / "third-party" / f"{name}-{version}"
        directory.mkdir(parents=True)
        for file in sorted(files):
            no_reparse(file)
            require(file.resolve().is_relative_to(source.resolve()), "License file escapes its package")
            require(not (directory / file.name).exists(), f"Duplicate notice name for {name} {version}: {file.name}")
            shutil.copyfile(file, directory / file.name)
        dependencies.append({"name":name, "version":version, "license":package["license"],
                             "notices":[file.name for file in sorted(files)]})
    require(dependencies, "No crates were resolved for the x64 package")
    return dependencies


class NoticeText(HTMLParser):
    def __init__(self):
        super().__init__()
        self.parts = []

    def handle_data(self, data):
        self.parts.append(data)


def rust_library_notices():
    sysroot = subprocess.run(["rustc", "--print", "sysroot"], cwd=CRATE, check=True,
                             capture_output=True, encoding="utf-8", timeout=15).stdout.strip()
    docs = Path(sysroot) / "share" / "doc" / "rust"
    report = docs / "COPYRIGHT-library.html"
    require(report.is_file(), "Install the Rust Standard Library copyright notices for the selected toolchain.")
    parser = NoticeText()
    parser.feed(report.read_text(encoding="utf-8-sig"))
    parser.close()
    yield f"Rust Standard Library / {report.name}", "".join(parser.parts)
    # The upstream report includes out-of-tree texts and names the in-tree licenses.
    for name in ("Apache-2.0", "MIT", "Unicode-3.0", "LLVM-exception", "BSD-2-Clause"):
        path = docs / "licenses" / f"{name}.txt"
        yield f"Rust Standard Library / {name}", path.read_text(encoding="utf-8-sig")


def merge_notices(output, dependencies):
    resource = (ROOT / "QKeyMapper" / "QKeyMapper.rc").read_text(encoding="utf-8-sig")
    copyright_notice = re.search(r'VALUE\s+"LegalCopyright",\s*"([^"]+)"', resource)
    require(copyright_notice is not None, "Missing QKeyMapper copyright notice.")
    with (output / MERGED_LICENSE).open("w", encoding="utf-8", newline="\n") as merged:
        def section(title, text):
            merged.write(f"{title}\n{'=' * len(title)}\n\n{text}\n\n")

        project_license = (output / "QKeyMapper-LICENSE.txt").read_text(encoding="utf-8-sig")
        section("QKeyMapper and QKeyMapper MCP Sidecar (GPL-3.0-only)",
                f"{copyright_notice[1]}\n\n{project_license}")
        for dependency in dependencies:
            crate = f"{dependency['name']}-{dependency['version']}"
            for name in dependency["notices"]:
                text = (output / "third-party" / crate / name).read_text(encoding="utf-8-sig")
                section(f"{crate} / {name}", text)
        for title, text in rust_library_notices():
            section(title, text)


def rust_sources():
    files = ["src", "Cargo.toml", "Cargo.lock", "rust-toolchain.toml"]
    status = subprocess.run(["git", "status", "--porcelain", "--untracked-files=all", "--", *files],
                            cwd=CRATE, check=True, capture_output=True, text=True, timeout=10)
    require(not status.stdout, "Build and package from committed Rust sources")
    refs = [f"HEAD:mcp_bridge_rust/{file}" for file in files]
    revisions = subprocess.run(["git", "rev-parse", *refs], cwd=CRATE, check=True,
                               capture_output=True, text=True, timeout=10).stdout.splitlines()
    return dict(zip(files, revisions, strict=True))


def package(args):
    exe, dumpbin = args.executable.absolute(), args.dumpbin.absolute()
    output = args.output.absolute()
    archive = output.with_suffix(".zip")
    require(output.resolve().is_relative_to(ROOT / "out") and output.name == archive.stem,
            "Use a new component directory under out/")
    no_reparse(output)
    no_reparse(archive)
    require(not output.exists() and not archive.exists(), "Choose a fresh component directory and ZIP")
    require(exe.name == EXE, "Use the cargo-built Sidecar")
    require_x64(exe)
    imports = dependencies(exe, dumpbin)
    needs_crt = CRT in imports
    require(needs_crt == (args.crt is not None),
            "Pass --crt for a binary that imports VCRUNTIME140.dll; omit it for a standalone binary")
    binary = EXE if needs_crt else STANDALONE_EXE
    runtime = {}
    if args.crt is not None:
        crt = args.crt.absolute()
        require(crt.name.lower() == CRT and crt.parent.name == "Microsoft.VC143.CRT"
                and crt.parent.parent.name.lower() == "x64", "Use the installed x64 MSVC redistributable")
        require_x64(crt)
        runtime["VCRUNTIME140.dll"] = signed_runtime(crt)
    sources = rust_sources()
    output.mkdir(parents=True)
    shutil.copyfile(exe, output / binary)
    if args.crt is not None:
        shutil.copyfile(crt, output / "VCRUNTIME140.dll")
    shutil.copyfile(ROOT / "LICENSE", output / "QKeyMapper-LICENSE.txt")
    readme = (CRATE / "BUNDLE_README.md").read_text(encoding="utf-8").replace(STANDALONE_EXE, binary)
    (output / "README.md").write_text(readme, encoding="utf-8", newline="\n")
    release_notices = notices(output)
    if not needs_crt:
        merge_notices(output, release_notices)
    files = [{"path":file.relative_to(output).as_posix(), "bytes":file.stat().st_size, "sha256":sha256(file)}
             for file in sorted(output.rglob("*")) if file.is_file()]
    manifest = {
        "schema_version":1, "architecture":TARGET, "binary":binary,
        "cargo_lock_sha256":sha256(CRATE / "Cargo.lock"), "rust_sources":sources,
        "pe_imports":imports, "app_local":runtime,
        "dependencies":release_notices, "files":files,
    }
    manifest_path = output / "bundle-manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    with zipfile.ZipFile(archive, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zipped:
        for file in sorted(output.rglob("*")):
            if file.is_file():
                entry = zipfile.ZipInfo(output.name + "/" + file.relative_to(output).as_posix())
                entry.create_system = 0
                zipped.writestr(entry, file.read_bytes(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    return {"archive":str(archive), "sha256":sha256(archive), "files":len(files) + 1,
            "unpacked_bytes":sum(file["bytes"] for file in files) + manifest_path.stat().st_size,
            "zip_bytes":archive.stat().st_size}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for flag in ("--executable", "--dumpbin", "--output"):
        parser.add_argument(flag, type=Path, required=True)
    parser.add_argument("--crt", type=Path, help="x64 app-local CRT required by a dynamic binary")
    print(json.dumps(package(parser.parse_args()), ensure_ascii=False, indent=2))
