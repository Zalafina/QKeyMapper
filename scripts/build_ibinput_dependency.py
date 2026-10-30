"""Rebuild pinned IbInputSimulator artifacts; never loads a DLL or opens a device."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

PINS = {
    "IbInputSimulator": ("Chaoses-Ib/IbInputSimulator", "0aba4f4f2b2053f106cee4c3be0a6201d76ca585"),
    "IbWinCppLib": ("Chaoses-Ib/IbWinCppLib", "f630f0e3a2c757652298c626fcd52469e5d5bc81"),
    "Detours": ("microsoft/Detours", "1dbd4ea281d73d478aa8d09a8b83fbd265f715f7"),
    "rapidjson": ("Tencent/rapidjson", "24b5e7a8b27f42fa16b96fc70aade9106cf7102f"),
}
ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "out/ibinput"
SOURCES = OUT / "sources"
PATCH = ROOT / "QKeyMapper/IbInputSimulator/patches/0001-logitech-report-safety.patch"
LOGITECH = "Simulator/include/IbInputSimulator/SendTypes/Logitech.hpp"


def run(args, log, cwd=None, env=None):
    args = [str(x) for x in args]
    if env is not None:
        search_path = next((v for k,v in env.items() if k.upper() == "PATH"), "")
        args[0] = shutil.which(args[0], path=search_path) or args[0]
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("w", encoding="utf-8") as stream:
        stream.write(subprocess.list2cmdline([str(x) for x in args]) + "\n")
        stream.flush()
        result = subprocess.run([str(x) for x in args], cwd=cwd, env=env, stdout=stream, stderr=subprocess.STDOUT)
        stream.write(f"\nNativeExitCode={result.returncode}\n")
    print(f"{log.relative_to(ROOT)}: exit {result.returncode}", flush=True)
    if result.returncode:
        shutil.copy2(log, log.with_name(log.stem + f"-failed-{time.time_ns()}" + log.suffix))
        print(log.read_text(encoding="utf-8", errors="replace")[-9000:])
        raise RuntimeError(f"Native tool failed: {log}")


def git(path, *args):
    return subprocess.check_output(["git", "-c", f"safe.directory={path.as_posix()}", "-C", str(path), *args])


def environment(arch):
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
    installation = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    vcvars = Path(installation) / "VC/Auxiliary/Build/vcvarsall.bat"
    command = f'call "{vcvars}" {arch} 10.0.19041.0 >nul && set'
    data = subprocess.check_output(f'"{os.environ["COMSPEC"]}" /d /s /c "{command}"', text=True, errors="replace")
    return {line.split("=", 1)[0]: line.split("=", 1)[1] for line in data.splitlines() if "=" in line and not line.startswith("=")}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--arch", choices=("x64", "x86"), default="x64")
    parser.add_argument("--review-patch", action="store_true", help="Build review-only patched artifacts under out; does not deploy them.")
    parser.add_argument("--deploy", action="store_true", help="Deploy the selected rebuilt artifacts to the product vendor directory after review.")
    parser.add_argument("--tests", action="store_true", help="Compile and run recording substitutes against both original and patched code.")
    args = parser.parse_args()
    SOURCES.mkdir(parents=True, exist_ok=True)
    for name, (repo, revision) in PINS.items():
        source = SOURCES / name
        if not source.exists():
            run(["git", "clone", "--no-checkout", f"https://github.com/{repo}.git", source], OUT / f"logs/clone-{name}.log")
            run(["git", "-C", source, "checkout", "--detach", revision], OUT / f"logs/checkout-{name}.log")
        if git(source, "rev-parse", "HEAD").decode().strip() != revision:
            raise RuntimeError(f"Unexpected dependency revision: {source}")
        if git(source, "diff", "--cached", "--name-only").strip() or git(source, "ls-files", "--others", "--exclude-standard").strip():
            raise RuntimeError(f"Unexpected staged or untracked dependency edits; preserved: {source}")
        changes = git(source, "diff", "--name-only").decode().splitlines()
        if changes and not (name == "IbInputSimulator" and changes == [LOGITECH] and git(source, "diff", "--", LOGITECH) == PATCH.read_bytes()):
            raise RuntimeError(f"Unrecognized dependency edits; preserved: {source}")
    env = environment(args.arch)
    logs = OUT / "logs" / args.arch
    detours = SOURCES / "Detours"
    run(["nmake", "/nologo"], logs / "detours.log", cwd=detours / "src", env=env)
    includes = OUT / "include/detours"
    includes.mkdir(parents=True, exist_ok=True)
    for name in ("detours.h", "detver.h"):
        shutil.copy2(detours / "include" / name, includes / name)
    rapid = OUT / f"rapidjson-{args.arch}"
    rapid_install=OUT / f"rapidjson-install-{args.arch}"
    run(["cmake", "-S", SOURCES / "rapidjson", "-B", rapid, "-G", "NMake Makefiles", "-DRAPIDJSON_BUILD_DOC=OFF", "-DRAPIDJSON_BUILD_EXAMPLES=OFF", "-DRAPIDJSON_BUILD_TESTS=OFF", "-DRAPIDJSON_BUILD_CXX11=OFF", f"-DCMAKE_INSTALL_DIR={rapid_install / 'lib/cmake/RapidJSON'}", f"-DINCLUDE_INSTALL_DIR={rapid_install / 'include'}", f"-DLIB_INSTALL_DIR={rapid_install / 'lib'}", f"-DDOC_INSTALL_DIR={rapid_install / 'share/doc/RapidJSON'}", f"-DCMAKE_INSTALL_PREFIX={rapid_install}", "-DCMAKE_BUILD_TYPE=Release"], logs / "rapidjson-configure.log", env=env)
    rapid_install=OUT / f"rapidjson-install-{args.arch}"
    run(["cmake", "--install", rapid, "--prefix", rapid_install], logs / "rapidjson-install.log", env=env)
    original = OUT / "original-source"
    if not original.exists():
        shutil.copytree(SOURCES / "IbInputSimulator", original, ignore=shutil.ignore_patterns(".git"))
    (original / LOGITECH).write_bytes(git(SOURCES / "IbInputSimulator", "show", f"HEAD:{LOGITECH}"))
    reviewed = OUT / "review-source"
    if args.review_patch or args.tests:
        if not reviewed.exists():
            shutil.copytree(original, reviewed)
        for relative in (LOGITECH, "Simulator/include/IbInputSimulator/SendTypes/usb.hpp", "Simulator/source/API 3.cpp", "Simulator/source/dllmain.cpp"):
            (reviewed / relative).write_bytes((original / relative).read_bytes())
        for patch in sorted(PATCH.parent.glob("*.patch")):
            run(["git", "apply", "--unsafe-paths", f"--directory={reviewed.as_posix()}", "--check", patch], logs / f"{patch.stem}-check.log", cwd=ROOT)
            run(["git", "apply", "--unsafe-paths", f"--directory={reviewed.as_posix()}", patch], logs / f"{patch.stem}-apply.log", cwd=ROOT)
    source = reviewed if args.review_patch else original
    output = OUT / ("review" if args.review_patch else "original") / args.arch
    run(["cmake", "-S", source / "Simulator", "-B", output, "-G", "NMake Makefiles", "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=ON", "-DBUILD_TESTING=OFF", f"-DFETCHCONTENT_SOURCE_DIR_IBWINCPP={SOURCES / 'IbWinCppLib'}", f"-DDETOURS_INCLUDE_DIR={OUT / 'include'}", f"-DDETOURS_LIBRARY={detours / ('lib.X64' if args.arch == 'x64' else 'lib.X86') / 'detours.lib'}", f"-DRapidJSON_DIR={rapid_install / 'lib/cmake/RapidJSON'}"], logs / ("review-configure.log" if args.review_patch else "original-configure.log"), env=env)
    run(["cmake", "--build", output], logs / ("review-build.log" if args.review_patch else "original-build.log"), env=env)
    run(["dumpbin", "/headers", "/exports", "/imports", "/dependents", output / "IbInputSimulator.dll"], logs / ("review-pe.log" if args.review_patch else "original-pe.log"), env=env)
    run(["dumpbin", "/headers", output / "IbInputSimulator.lib"], logs / ("review-lib.log" if args.review_patch else "original-lib.log"), env=env)
    for name in ("IbInputSimulator.dll", "IbInputSimulator.lib"):
        artifact = output / name
        print(f"{artifact.relative_to(ROOT)} {artifact.stat().st_size} SHA256={hashlib.sha256(artifact.read_bytes()).hexdigest()}")
    if args.tests:
        for label, implementation in (("original", original), ("review", reviewed)):
            test = OUT / "tests" / args.arch / label
            test.mkdir(parents=True, exist_ok=True)
            run(["cl", "/nologo", "/std:c++20", "/EHsc", "/MD", "/O2", "/DNDEBUG", "/DNOMINMAX", "/D_CRT_SECURE_NO_WARNINGS", "/D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING", f"/I{implementation / 'Simulator/include'}", f"/I{SOURCES / 'IbWinCppLib/include'}", f"/I{SOURCES / 'rapidjson/include'}", f"/I{OUT / 'include'}", ROOT / "QKeyMapper/tests/ibinput/logitech_report_test.cpp", "/Fe:report_test.exe", "/link", "user32.lib", "ntdll.lib"], logs / f"{label}-test-build.log", cwd=test, env=env)
            run(["cl", "/nologo", "/std:c++20", "/EHsc", "/MD", "/O2", "/DNDEBUG", f"/I{implementation / 'Simulator/include'}", f"/I{implementation / 'Simulator/source'}", ROOT / "QKeyMapper/tests/ibinput/argument_bounds_test.cpp", "/Fe:argument_test.exe"], logs / f"{label}-argument-build.log", cwd=test, env=env)
            bound_result=subprocess.run([str(test / "argument_test.exe")], cwd=test, env=env, capture_output=True, text=True)
            (logs / f"{label}-argument-tests.log").write_text(bound_result.stdout+bound_result.stderr+f"NativeExitCode={bound_result.returncode}\n", encoding="utf-8")
            if bound_result.returncode != (0 if label=="review" else 2):
                raise RuntimeError(f"Unexpected argument bounds result: {label}")
            if label == "review":
                run([test / "report_test.exe"], logs / "review-tests.log", cwd=test, env=env)
            else:
                for case in ("mouse", "keyboard", "partial", "activation", "compensation"):
                    result = subprocess.run([str(test / "report_test.exe"), case], cwd=test, env=env, capture_output=True, text=True)
                    (logs / f"original-regression-{case}.log").write_text(result.stdout + result.stderr + f"NativeExitCode={result.returncode}\n", encoding="utf-8")
                    if result.returncode != 2:
                        raise RuntimeError(f"Regression was not reproduced: {case}, exit={result.returncode}")
                    print(f"Original regression reproduced (simulated): {case}")
    if args.deploy:
        vendor=ROOT / "QKeyMapper/IbInputSimulator"
        header=vendor / "include/IbInputSimulator"
        destination=vendor / "lib" / args.arch
        header.mkdir(parents=True,exist_ok=True)
        destination.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source / "Simulator/include/IbInputSimulator/InputSimulator.hpp", header / "InputSimulator.hpp")
        for name in ("IbInputSimulator.dll", "IbInputSimulator.lib"):
            shutil.copy2(output / name,destination / name)
        print(f"Deployed {args.arch} {'reviewed patch build' if args.review_patch else 'original build'} to product vendor directory")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
