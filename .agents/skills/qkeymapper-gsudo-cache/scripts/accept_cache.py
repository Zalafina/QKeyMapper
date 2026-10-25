"""Owned cache acceptance; source_cleanup additionally needs approved private-product GUI scope."""

if not __debug__:
    raise RuntimeError("Run without -O; ownership and acceptance assertions must stay enabled.")

import argparse
from contextlib import closing
from dataclasses import asdict
from datetime import timedelta
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import uuid

import cache_control as app


def wait_for(predicate, seconds=15):
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        if predicate():
            return
        time.sleep(.1)
    raise AssertionError("Timed out waiting for the observed cache/controller condition")


def run(scenario, output, job_file=None, scope_file=None):
    assert app.token_info() == {"elevated": False, "integrity": 8192}
    assert output.resolve().is_relative_to(app.ROOT / "out")
    output.mkdir(parents=True, exist_ok=False)
    events = []
    owner = None
    baseline_pipes = set(app.native_status(shutil.which("gsudo.exe"))["CacheSessions"])

    def command(*words):
        value = app.client(list(words))
        events.append({"command": list(words), "result": app.public(value)})
        (output / "events.json").write_bytes(json.dumps(events, ensure_ascii=False, indent=2).encode("utf-8"))
        print(words, value.get("action", value.get("status")), value.get("ok"), flush=True)
        return value

    try:
        assert command("status")["controller"] is None
        if scenario == "source_cleanup":
            job, scope = app.read_json(job_file, 8192), app.read_json(scope_file, 8192)
            assert type(job) is dict and set(job) == {"kind", "arguments"} and job["kind"] == "source"
            scope = app.validate_scope({"source": scope["source"]})
            app.build_job(app.ROOT, sys.executable, "source", job["arguments"], scope)
            value = command("12m")
            assert value["ok"] and value["cache_available"], value
            owner = app.Identity(**value["identity"])
            state = app.verified_controller()
            assert app.send(state, {"op": "authorize", "scope": scope})["ok"]
            request_id = uuid.uuid4().hex
            assert app.send(state, {"op": "run", **job}, timeout=0, request_id=request_id)["error"] == "result_unknown"
            result_file = app.RUNTIME / "results" / (request_id + ".json")
            product_output = Path(job["arguments"]["output"])

            def final_cleanup_started():
                if result_file.exists():
                    raise AssertionError("Source job ended before the selected cleanup boundary: " + str(app.read_json(result_file)))
                return len(list(product_output.glob("*_Revoke.log"))) == 2

            print("Waiting for the fixed source suite's final revoke/disable/close sequence.", flush=True)
            wait_for(final_cleanup_started, 630)
            closing = app.send(state, {"op": "off"})
            assert closing["current_job"] == "source" and closing["status"] == "closing", closing
            during = command("status")
            assert not during["cache_available"] and during["current_job"] == "source", during
            print("Cache is off while source cleanup remains active; approve the existing cleanup UAC if shown.", flush=True)
            wait_for(result_file.is_file, 180)
            result = app.read_json(result_file)
            assert result["ok"] and result["exit_code"] == 0 and result["close_reason"] == "user_off", result
            assert not result["deadline_expired"]
            assert len(app.read_json(product_output / "results.json")["checks"]) == 10
            assert app.read_json(product_output / "closed.json")["exit_code"] == 0
            wait_for(lambda: app.verified_controller() is None)
            (output / "source-cleanup.json").write_bytes(json.dumps({"during": app.public(during), "job": result,
                "product_exit_code": 0, "system_input": False}, indent=2).encode("utf-8"))
        elif scenario == "coexist":
            holder = subprocess.Popen([sys.executable, "-B", "-c", "import sys; sys.stdin.buffer.read(1)"],
                stdin=subprocess.PIPE, creationflags=app.NO_WINDOW)
            external = app.identity(holder.pid)
            gsudo = shutil.which("gsudo.exe")
            try:
                assert app.token_info(holder.pid) == {"elevated": False, "integrity": 8192}
                with (output / "external-cache.log").open("wb") as log:
                    result = subprocess.run([gsudo, "cache", "on", "-p", str(holder.pid), "-d", "00:02:00"],
                        stdout=log, stderr=log, creationflags=app.NO_WINDOW, timeout=85)
                assert result.returncode == 0
                external_pipes = set(app.native_status(gsudo)["CacheSessions"]) - baseline_pipes
                assert len(external_pipes) == 1
                absent = command("status")
                assert absent["controller"] is None and absent["native_sessions_observed"] >= 1
                assert command("off")["controller"] is None
                assert external_pipes <= set(app.native_status(gsudo)["CacheSessions"])
                value = command("2m")
                assert value["ok"] and value["cache_available"]
                owner = app.Identity(**value["identity"])
                assert owner.pid != external.pid
                assert command("off")["action"] == "closed"
                assert external_pipes <= set(app.native_status(gsudo)["CacheSessions"])
                (output / "coexist.json").write_bytes(json.dumps({"external_pid": external.pid,
                    "managed_pid": owner.pid, "external_cache_preserved": True, "no_adoption": True}).encode())
            finally:
                try:
                    if app.verified_controller():
                        command("off")
                finally:
                    try:
                        if holder.poll() is None:
                            assert app.identity(holder.pid) == external
                            subprocess.run([gsudo, "cache", "off", "-p", str(external.pid)], check=True,
                                capture_output=True, creationflags=app.NO_WINDOW, timeout=5)
                    finally:
                        holder.stdin.close()
                        assert holder.wait(timeout=10) == 0
        elif scenario in ("pending_off", "pending_expiry"):
            state, created = app.ensure_controller()
            assert created
            owner = app.Identity(**state["identity"])
            request_id = uuid.uuid4().hex
            seconds = 2 if scenario == "pending_expiry" else 120
            result = app.send(state, {"op": "set", "deadline": (app.utc_now() + timedelta(seconds=seconds)).isoformat()},
                              timeout=0, request_id=request_id)
            assert result["error"] == "result_unknown"
            wait_for(lambda: (app.verified_controller() or {}).get("pending_request") == request_id)
            if scenario == "pending_off":
                closing = app.send(state, {"op": "off"})
                assert closing["pending_request"] == request_id, closing
                print("OFF recorded while UAC/cache-on is pending; approve this test UAC now.", flush=True)
            else:
                print("Wait at least 3 seconds before approving this test UAC; requested window is 2 seconds.", flush=True)
            wait_for(lambda: (app.RUNTIME / "results" / (request_id + ".json")).is_file(), 85)
            result = app.read_json(app.RUNTIME / "results" / (request_id + ".json"))
            assert result.get("error") == "cache_update_failed", result
            assert result["native_exit_code"] == 0, result
            assert result["requested_window_expired"] == (scenario == "pending_expiry"), result
            wait_for(lambda: app.verified_controller() is None)
            closed = app.read_json(app.RUNTIME / "closed.json")
            assert not closed["cache_available"] and closed["deadline"] is None
            if scenario == "pending_off":
                assert closed["close_reason"] == "user_off", closed
            (output / "late-result.json").write_bytes(json.dumps({"result": result, "closed": app.public(closed)}, indent=2).encode("utf-8"))
        elif scenario == "ordinary":
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
            assert owner.same_login(app.identity(os.getpid()))
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
        elif scenario in ("active_off", "active_expiry"):
            value = command("2m")
            assert value["ok"] and value["cache_available"], value
            owner = app.Identity(**value["identity"])
            state = app.verified_controller()
            assert app.send(state, {"op": "authorize", "scope": {"identity_probe": {
                "reason": "User-approved CACHE-SKILL-69 fixed read-only identity and cache lifecycle acceptance."}}})["ok"]
            request_id = uuid.uuid4().hex
            assert app.send(state, {"op": "run", "kind": "identity_probe", "arguments": {}},
                            timeout=0, request_id=request_id)["error"] == "result_unknown"
            log = app.RUNTIME / "logs" / (request_id + "-job.log")

            def probe_started():
                try:
                    return json.loads(log.read_text(encoding="utf-8-sig"))["integrity"] == 12288
                except (OSError, ValueError):
                    return False

            wait_for(probe_started)
            probe = json.loads(log.read_text(encoding="utf-8-sig"))
            assert probe["elevated"] and app.Identity(**probe["identity"]).same_login(owner)
            assert command("status")["current_job"] == "identity_probe"
            if scenario == "active_off":
                closing = app.send(state, {"op": "off"})
                assert closing["ok"] and closing["current_job"] == "identity_probe"
            else:
                assert command("1s")["action"] == "shortened"
                time.sleep(1.4)
            during = command("status")
            assert during["status"] == "closing" and not during["cache_available"]
            assert during["current_job"] == "identity_probe", during
            wait_for(lambda: app.verified_controller() is None)
            job = app.read_json(app.RUNTIME / "results" / (request_id + ".json"))
            assert job["ok"] and job["exit_code"] == 0, job
            assert job["deadline_expired"] == (scenario == "active_expiry")
            assert job["close_reason"] == ("deadline_expired" if scenario == "active_expiry" else "user_off")
            (output / "active-boundary.json").write_bytes(json.dumps({"probe_integrity": 12288,
                "same_login": True, "during": app.public(during), "job": job}, indent=2).encode("utf-8"))
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
        wait_for(lambda: set(app.native_status(shutil.which("gsudo.exe"))["CacheSessions"]) <= baseline_pipes)
        result = {"passed": True, "scenario": scenario, "controller": asdict(owner) if owner else None,
                  "no_owned_cache_remaining": True, "system_input": False, "product_started": scenario == "source_cleanup"}
        (output / "result.json").write_bytes(json.dumps(result, indent=2).encode("utf-8"))
        print("PASS", scenario, "no owned cache remaining", flush=True)
    finally:
        if app.verified_controller():
            value = app.client(["off"])
            print("CLEANUP", value.get("action"), flush=True)
            wait_for(lambda: app.verified_controller() is None)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenario", choices=("ordinary", "basic", "crash", "cancel", "active_off", "active_expiry", "pending_off", "pending_expiry", "coexist", "source_cleanup"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--job-file", type=Path)
    parser.add_argument("--scope-file", type=Path)
    args = parser.parse_args()
    if args.scenario == "source_cleanup" and (not args.job_file or not args.scope_file):
        parser.error("source_cleanup requires an approved fixed source job and its existing scope evidence.")
    run(args.scenario, args.output, args.job_file, args.scope_file)
