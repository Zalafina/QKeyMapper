"""One native worker, one request at a time, and no automatic write retries."""

import asyncio
import hmac
import os
import re
import secrets
import threading
import time
import uuid
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import pywintypes
import win32event

from .credentials import CredentialStore
from .demo import read_binding, readiness, validate_binding
from .images import validate_capture
from .protocol import (BridgeError, Credential, MAX_HANDSHAKE, client_name, decimal,
                       fields, hex_bytes, proof, require, scopes)
from .windows import Pipe, discover, identity, pristine_thread


class Bridge:
    def __init__(self, discovery: Path, executable: Path, demo_record: Path | None = None):
        self.discovery, self.executable = discovery, executable
        self._demo_record = demo_record
        self._worker = ThreadPoolExecutor(max_workers=1, thread_name_prefix="qkm-pipe")
        self._lock = threading.Lock()
        self._cancel = win32event.CreateEvent(None, True, False, None)
        self._busy = False
        self._active = None
        self._closed = False
        self._closing = False
        self._pipe = None
        self._credential = None
        self._state = {"status": "disconnected"}

    def state(self) -> dict:
        with self._lock:
            return {**self._state, "busy": self._busy or self._closing}

    def _set(self, status: str, **extra) -> None:
        with self._lock:
            self._state = {"status": status, **extra}

    def _disconnect(self) -> None:
        if self._pipe is not None:
            self._pipe.close()
        self._pipe = self._credential = None

    def _submit(self, operation):
        def run():
            try:
                pristine_thread()
                return operation()
            except (BridgeError, pywintypes.error, OSError) as error:
                self._disconnect()
                code = error.code if isinstance(error, BridgeError) else "native_operation_failed"
                self._set("disconnected" if code == "cancelled" else "failed", error=code)
                return {"ok": False, "error": code}
            except Exception:
                self._disconnect()
                self._set("failed", error="internal_error")
                return {"ok": False, "error": "internal_error"}
        def finished(done):
            with self._lock:
                if self._active is done:
                    self._active = None
                    self._busy = False
        with self._lock:
            if self._closed or self._busy or self._closing:
                return None
            self._busy = True
            try:
                win32event.ResetEvent(self._cancel)
                future = self._worker.submit(run)
            except BaseException:
                self._busy = False
                raise
            self._active = future
        # An already completed or cancelled Future invokes this callback immediately.
        future.add_done_callback(finished)
        return future

    def connect(self, *, pair: bool = False) -> dict:
        future = self._submit(lambda: self._connect(pair))
        result = {"accepted": future is not None, **self.state()}
        if future is None:
            result.update(ok=False, error="busy_or_closed")
        return result

    def _connect(self, pair: bool) -> dict:
        self._disconnect()
        self._set("connecting")
        own = identity(os.getpid())
        target = discover(self.discovery, self.executable, own)
        store = CredentialStore(self.discovery, self.executable, own.session)
        credential = None if pair else store.load()
        if not pair and credential is None:
            self._set("pairing_required")
            return self.state()
        self._pipe = Pipe(target, own, self._cancel)
        nonce = secrets.token_bytes(32)
        hello = {"type": "pair.request" if pair else "hello", "version": 1,
                 "instance": target.instance, "client_nonce": nonce.hex()}
        if pair:
            name = "QKeyMapper MCP bridge"
            client_name(name)
            hello["client_name"] = name
        else:
            hello["pair_id"] = credential.pair_id
        self._pipe.send(hello, time.monotonic() + 3, MAX_HANDSHAKE)
        if pair:
            self._set("awaiting_gui_approval")
        challenge = self._pipe.receive(time.monotonic() + (62 if pair else 3), MAX_HANDSHAKE)
        deadline = time.monotonic() + 3
        names = "type version instance pair_id epoch server_nonce server_proof client_pid client_created server_pid server_created"
        fields(challenge, names + (" secret scopes" if pair else ""))
        require(challenge["type"] == ("pair.grant" if pair else "challenge")
                and type(challenge["version"]) is int and challenge["version"] == 1
                and challenge["instance"] == target.instance)
        require((decimal(challenge["client_pid"], 32), decimal(challenge["client_created"], 64)) == (own.pid, own.created)
                and (decimal(challenge["server_pid"], 32), decimal(challenge["server_created"], 64))
                    == (target.server.pid, target.server.created), "identity_rejected")
        hex_bytes(challenge["pair_id"], 16)
        epoch = decimal(challenge["epoch"], 64)
        server_nonce = hex_bytes(challenge["server_nonce"], 32)
        if pair:
            credential = Credential(challenge["pair_id"], epoch, scopes(challenge["scopes"]), hex_bytes(challenge["secret"], 32))
        require(challenge["pair_id"] == credential.pair_id and epoch >= credential.epoch, "credential_rejected")
        args = (target.instance, credential.pair_id, epoch, own, target.server, nonce, server_nonce, credential.secret)
        require(hmac.compare_digest(hex_bytes(challenge["server_proof"], 32), proof(1, *args)), "server_proof_rejected")
        if pair:
            # Never confirm an authorization whose client key could not be persisted.
            store.save(credential)
        self._pipe.send({"type": "pair.confirm" if pair else "proof", "proof": proof(3 if pair else 2, *args).hex()}, deadline, MAX_HANDSHAKE)
        ready = self._pipe.receive(deadline, MAX_HANDSHAKE)
        fields(ready, "type version instance pair_id epoch scopes")
        require(ready["type"] == "ready" and type(ready["version"]) is int and ready["version"] == 1
                and ready["instance"] == target.instance and ready["pair_id"] == credential.pair_id
                and decimal(ready["epoch"], 64) == epoch)
        granted = scopes(ready["scopes"])
        require(not pair or granted == credential.scopes)
        current = Credential(credential.pair_id, epoch, granted, credential.secret)
        if current != credential:
            store.save(current)
        self._credential = current
        self._set("connected", instance=target.instance, scopes=granted, last_verified_unix=time.time())
        return self.state()

    async def get_capabilities(self) -> dict:
        return await self._call("qkm_get_capabilities")

    async def get_state(self) -> dict:
        return await self._call("qkm_get_state")

    async def get_help(self, topic: str, *, offset: int = 0, limit: int = 50) -> dict:
        return await self._call("qkm_get_help", {"topic": topic, "offset": offset, "limit": limit})

    async def query(self, entity: str, *, catalog_revision: str | None = None,
                    source_revision: str | None = None, source_tab_ref: str | None = None,
                    offset: int = 0, limit: int = 50) -> dict:
        arguments = {"entity": entity, "offset": offset, "limit": limit}
        if catalog_revision is not None:
            arguments["catalog_revision"] = catalog_revision
        if source_revision is not None:
            arguments["source_revision"] = source_revision
        if source_tab_ref is not None:
            arguments["source_tab_ref"] = source_tab_ref
        return await self._call("qkm_query", arguments)

    async def preview_changes(self, source_revision: str | None = None, source_tab_ref: str | None = None,
                              original_key: str | None = None, mapping_key: str | None = None,
                              insert_row: int | None = None, *, action: str = "mapping.add", setting_name: str | None = None,
                              source_refs: list[str] | None = None) -> dict:
        arguments = {key: value for key, value in dict(source_revision=source_revision, source_tab_ref=source_tab_ref,
            original_key=original_key, mapping_key=mapping_key, insert_row=insert_row, setting_name=setting_name,
            source_refs=source_refs).items()
            if value is not None}
        if action == "mapping.add":
            arguments["insert_row"] = insert_row
        else:
            arguments["action"] = action
        return await self._call("qkm_preview_changes", arguments)

    async def apply_changes(self, preview_id: str, confirm_conflicts: bool = False, *,
                            confirm_scope: bool = False, confirm_overwrite: bool = False, confirm_delete: bool = False) -> dict:
        arguments = dict(preview_id=preview_id, confirm_conflicts=confirm_conflicts)
        if confirm_scope is not False:
            arguments["confirm_scope"] = confirm_scope
        if confirm_overwrite is not False:
            arguments["confirm_overwrite"] = confirm_overwrite
        if confirm_delete is not False:
            arguments["confirm_delete"] = confirm_delete
        return await self._call("qkm_apply_changes", arguments)

    async def cancel_changes(self, preview_id: str) -> dict:
        return await self._call("qkm_cancel_changes", dict(preview_id=preview_id))

    async def get_operation(self, operation_id: str) -> dict:
        return await self._call("qkm_get_operation", dict(operation_id=operation_id))

    async def capture_window(self, role: str, window_ref: str, source_revision: str, source_tab_ref: str) -> dict:
        return await self._call("qkm_capture_window", dict(role=role, window_ref=window_ref,
            source_revision=source_revision, source_tab_ref=source_tab_ref))

    async def open_mapping_editor(self, source_ref: str, presentation: str) -> dict:
        return await self._call("qkm_open_mapping_editor", dict(source_ref=source_ref, presentation=presentation))

    async def close_mapping_editor(self, source_ref: str, window_ref: str, presentation: str) -> dict:
        return await self._call("qkm_close_mapping_editor", dict(source_ref=source_ref, window_ref=window_ref, presentation=presentation))

    async def demo_state(self) -> dict:
        return await self._perform(self._demo_state)

    def _demo_state(self) -> dict:
        result = {"configured": self._demo_record is not None, "ready": False,
                  "reference_scope": "preflight_observation_only"}
        if self._demo_record is None:
            return {"ok": True, "result": {**result, "reason": "demo_not_configured"}}
        require(self._pipe is not None and self._credential is not None, "not_connected")
        target = self._pipe.discovery
        own = identity(os.getpid())
        try:
            binding = validate_binding(read_binding(self._demo_record, own), target, self.executable)
            live = identity(target.server.pid)
            require(own.same_login(live) and (live.created, live.session, live.executable)
                    == (target.server.created, target.server.session, target.server.executable), "demo_record_mismatch")
        except BridgeError as error:
            code = error.code if error.code in ("demo_record_rejected", "demo_record_mismatch") else "demo_record_mismatch"
            return {"ok": True, "result": {**result, "reason": code}}
        samples = []
        for method, arguments in (("qkm_query", {"entity": "windows"}), ("qkm_get_state", {}),
                                  ("qkm_query", {"entity": "mapping_sources", "limit": 100})):
            response = self._request(method, arguments)
            if response.get("ok") is not True:
                return {"ok": True, "result": {**result, "reason": response.get("error", "invalid_demo_state")}}
            samples.append(response["result"])
        windows, state, sources = samples
        reason = readiness(binding, windows, state, sources, self._credential.scopes)
        if reason in ("demo_record_mismatch", "invalid_demo_state"):
            return {"ok": True, "result": {**result, "reason": reason}}
        summary = {key: binding[key] for key in ("instance", "pid", "created", "executable", "desktop")}
        return {"ok": True, "result": {**result, "ready": reason == "ready", "reason": reason,
                "target": summary, "configuration": binding["configuration"],
                "source_snapshot": sources.get("source_snapshot", {})}}

    async def _call(self, method: str, arguments: dict | None = None) -> dict:
        return await self._perform(lambda: self._request(method, arguments if arguments is not None else {}))

    async def _perform(self, operation) -> dict:
        future = self._submit(operation)
        if future is None:
            return {"ok": False, "error": "busy_or_closed"}
        try:
            return await asyncio.wrap_future(future)
        except asyncio.CancelledError:
            with self._lock:
                if self._active is future:
                    win32event.SetEvent(self._cancel)
            raise

    def _request(self, method: str, arguments: dict) -> dict:
        require(self._pipe is not None and self._credential is not None, "not_connected")
        request_id = uuid.uuid4().hex
        deadline = time.monotonic() + 10
        self._pipe.send({"id": request_id, "method": method, "arguments": arguments}, deadline)
        response = self._pipe.receive(deadline)
        require(type(response.get("ok")) is bool and response.get("id") == request_id)
        if response["ok"]:
            fields(response, "id ok result")
            require(type(response["result"]) is dict)
            if method == "qkm_capture_window":
                response["result"] = validate_capture(response["result"], self._pipe.discovery, arguments)
        else:
            fields(response, "id ok error")
            fields(response["error"], "code message")
            code, message = response["error"]["code"], response["error"]["message"]
            require(type(code) is str and re.fullmatch(r"[a-z][a-z0-9_]{0,63}", code) is not None)
            require(type(message) is str and len(message.encode("utf-8")) <= 1024)
            response = {"ok": False, "error": code, "message": message}
        self._set("connected", instance=self._pipe.discovery.instance,
                  scopes=self._credential.scopes, last_verified_unix=time.time())
        return response

    def disconnect(self) -> dict:
        with self._lock:
            if not self._closed and not self._closing:
                self._closing = True
                win32event.SetEvent(self._cancel)
                # At most one close is queued; new operations cannot overtake it.
                self._worker.submit(self._close_connection)
        return {"accepted": True, "status": "disconnecting"}

    def _close_connection(self) -> dict:
        self._disconnect()
        self._set("disconnected")
        with self._lock:
            self._closing = False
        return self.state()

    def close(self) -> None:
        with self._lock:
            if self._closed:
                return
            self._closed = True
            win32event.SetEvent(self._cancel)
            self._worker.submit(self._close_connection)
        self._worker.shutdown(wait=True)
        self._cancel.Close()
