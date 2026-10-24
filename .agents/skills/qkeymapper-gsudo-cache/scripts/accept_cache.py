"""Owned controller/cache acceptance only; no QKeyMapper, driver or system-input tests."""

import argparse
from contextlib import closing
from dataclasses import asdict
import json
from pathlib import Path
import subprocess
import sys
import time

import cache_control as app


def wait_for(predicate, seconds=15):
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        if predicate():
            return
        time.sleep(.1)
    raise AssertionError("Timed out waiting for the observed cache/controller condition")


def run(scenario, output):
    assert app.token_info() == {"elevated": False, "integrity": 8192}
    assert output.resolve().is_relative_to(app.ROOT / "out")
    output.mkdir(parents=True, exist_ok=False)
    events = []
    owner = None
    baseline_pipes = set(app.native_status(__import__("shutil").which("gsudo.exe"))["CacheSessions"])

    def command(*words):
        value = app.client(list(words))
        events.append({"command": list(words), "result": app.public(value)})
        (output / "events.json").write_bytes(json.dumps(events, ensure_ascii=False, indent=2).encode("utf-8"))
        print(words, value.get("action", value.get("status")), value.get("ok"), flush=True)
        return value

    try:
        assert command("status")["controller"] is None
        if scenario == "ordinary":
            code = "import cache_control as app,json; state,created=app.ensure_controller(); print(json.dumps({'pid':state['identity']['pid'],'created':created}))"
            children = [subprocess.Popen([sys.executable, "-B", "-X", "utf8", "-c", code], cwd=app.SCRIPT.parent,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, creationflags=app.NO_WINDOW) for _ in range(2)]
            reports = []
            for child in children:
                stdout, stderr = child.communicate(timeout=20)
                assert child.returncode == 0, stderr.decode("utf-8", errors="replace")
                reports.append(json.loads(stdout))
            assert reports[0]["pid"] == reports[1]["pid"] and sum(x["created"] for x in reports) == 1, reports
            value = command("status")
            assert value["identity"]["pid"] == reports[0]["pid"] and not value["cache_available"]
            assert app.token_info(value["identity"]["pid"]) == {"elevated": False, "integrity": 8192}
            assert command("off")["action"] == "closed"
        elif scenario == "basic":
            value = command("2m")
            assert value["ok"] and value["controller_created"] and value["cache_available"], value
            owner = app.Identity(**value["identity"])
            assert owner.same_login(app.identity(__import__("os").getpid()))
            assert command("5s")["action"] == "shortened"
            assert command("30s")["action"] == "extended"
            time.sleep(6)
            value = command("status")
            assert value["cache_available"] and value["identity"]["pid"] == owner.pid and value["remaining_seconds"] > 15
            assert command("5h")["action"] == "extended"
            value = command("restore")
            assert value["action"] == "restored" and 295 <= value["remaining_seconds"] <= 300
            state = app.verified_controller()
            scope = app.send(state, {"op": "authorize", "scope": {"identity_probe": {"reason": "User-approved CACHE-SKILL-69 cache-only acceptance; fixed read-only identity probe."}}})
            assert scope["ok"]
            job = app.send(state, {"op": "run", "kind": "identity_probe", "arguments": {}})
            assert job["ok"], job
            probe = json.loads(Path(job["log"]).read_text(encoding="utf-8-sig"))
            assert probe["elevated"] and probe["integrity"] == 12288
            assert app.Identity(**probe["identity"]).same_login(owner)
            (output / "probe-summary.json").write_bytes(json.dumps({"controller_pid": owner.pid,
                "controller_integrity": 8192, "probe_integrity": probe["integrity"], "same_login": True}).encode())
            assert command("5s")["action"] == "shortened"
            wait_for(lambda: app.verified_controller() is None)
            closed = app.read_json(app.RUNTIME / "closed.json")
            assert closed["close_reason"] == "deadline_expired" and not closed["cache_available"]
        elif scenario == "crash":
            value = command("2m")
            assert value["ok"] and value["cache_available"]
            owner = app.Identity(**value["identity"])
            state = app.verified_controller()
            assert state["identity"]["pid"] == owner.pid and state["current_job"] is None
            assert app.identity(owner.pid) == owner and app.token_info(owner.pid) == {"elevated": False, "integrity": 8192}
            with closing(app.win32api.OpenProcess(0x1000 | 0x100000 | 1, False, owner.pid)) as process:
                # Terminate only this verified, ordinary idle test controller to exercise PID-lifetime cleanup.
                app.win32api.TerminateProcess(process, 73)
                assert app.win32event.WaitForSingleObject(process, 10000) == 0
            wait_for(lambda: app.verified_controller() is None)
        else:
            value = command("2m")
            assert value.get("ok") is False and value.get("error") == "needs_uac", value
            wait_for(lambda: app.verified_controller() is None)
        wait_for(lambda: set(app.native_status(__import__("shutil").which("gsudo.exe"))["CacheSessions"]) <= baseline_pipes)
        result = {"passed": True, "scenario": scenario, "controller": asdict(owner) if owner else None,
                  "no_owned_cache_remaining": True, "system_input": False, "product_started": False}
        (output / "result.json").write_bytes(json.dumps(result, indent=2).encode("utf-8"))
        print("PASS", scenario, "no owned cache remaining", flush=True)
    finally:
        if app.verified_controller():
            value = app.client(["off"])
            print("CLEANUP", value.get("action"), flush=True)
            wait_for(lambda: app.verified_controller() is None)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenario", choices=("ordinary", "basic", "crash", "cancel"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    run(args.scenario, args.output)
