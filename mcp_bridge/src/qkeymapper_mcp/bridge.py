"""One native worker, one request at a time, and no automatic write retries."""

import asyncio
import hmac
import os
import secrets
import threading
import time
import uuid
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import pywintypes
import win32event

from .credentials import CredentialStore
from .protocol import (BridgeError, Credential, MAX_HANDSHAKE, client_name, decimal,
                       fields, hex_bytes, proof, require, scopes)
from .windows import Pipe, discover, identity, pristine_thread


class Bridge:
    def __init__(self, discovery: Path, executable: Path):
        self.discovery, self.executable = discovery, executable
        self._worker = ThreadPoolExecutor(max_workers=1, thread_name_prefix="qkm-pipe")
        self._lock = threading.Lock()
        self._cancel = win32event.CreateEvent(None, True, False, None)
        self._busy = False
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
            finally:
                with self._lock:
                    self._busy = False
        with self._lock:
            if self._closed or self._busy or self._closing:
                return None
            self._busy = True
            win32event.ResetEvent(self._cancel)
            return self._worker.submit(run)

    def connect(self, *, pair: bool = False) -> dict:
        future = self._submit(lambda: self._connect(pair))
        return {"accepted": future is not None, **self.state()}

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

    async def _call(self, method: str) -> dict:
        future = self._submit(lambda: self._request(method))
        if future is None:
            return {"ok": False, "error": "busy_or_closed"}
        try:
            return await asyncio.wrap_future(future)
        except asyncio.CancelledError:
            win32event.SetEvent(self._cancel)
            raise

    def _request(self, method: str) -> dict:
        require(self._pipe is not None and self._credential is not None, "not_connected")
        request_id = uuid.uuid4().hex
        deadline = time.monotonic() + 10
        self._pipe.send({"id": request_id, "method": method, "arguments": {}}, deadline)
        response = self._pipe.receive(deadline)
        require(type(response.get("ok")) is bool and response.get("id") == request_id)
        if response["ok"]:
            fields(response, "id ok result")
            require(type(response["result"]) is dict)
        else:
            fields(response, "id ok error")
            fields(response["error"], "code message")
            require(type(response["error"]["code"]) is str and len(response["error"]["code"]) <= 64)
            response = {"ok": False, "error": response["error"]["code"]}
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
