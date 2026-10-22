"""Native Windows identity, private discovery and cancellable named-pipe I/O."""

import ctypes as c
import ctypes.wintypes as w
import os
import time
from contextlib import ExitStack, closing
from dataclasses import dataclass, field
from pathlib import Path

import pywintypes
import win32api
import win32con
import win32event
import win32file
import win32pipe
import win32security

from .protocol import (BridgeError, MAX_FRAME, PIPE_PREFIX, decimal,
                       decode, encode, fields, hex_bytes, require)

_kernel = c.WinDLL("kernel32", use_last_error=True)
_kernel.GetProcessTimes.argtypes = [w.HANDLE] + [c.POINTER(w.FILETIME)] * 4
_kernel.GetProcessTimes.restype = w.BOOL
_kernel.QueryFullProcessImageNameW.argtypes = [w.HANDLE, w.DWORD, w.LPWSTR, c.POINTER(w.DWORD)]
_kernel.QueryFullProcessImageNameW.restype = w.BOOL


class _FileId(c.Structure):
    _fields_ = [("volume", c.c_ulonglong), ("identifier", c.c_ubyte * 16)]


_kernel.GetFileInformationByHandleEx.argtypes = [w.HANDLE, c.c_int, w.LPVOID, w.DWORD]
_kernel.GetFileInformationByHandleEx.restype = w.BOOL


@dataclass(frozen=True)
class Identity:
    pid: int
    created: int
    session: int
    user: str = field(repr=False)
    logon: str = field(repr=False)
    authentication: int = field(repr=False)
    executable: str

    def same_login(self, other) -> bool:
        return (self.user, self.logon, self.authentication, self.session) == (
            other.user, other.logon, other.authentication, other.session)


def pristine_thread() -> None:
    try:
        token = win32security.OpenThreadToken(win32api.GetCurrentThread(), win32con.TOKEN_QUERY, True)
    except pywintypes.error as error:
        require(error.winerror == 1008, "thread_context_rejected")
        return
    token.Close()
    raise BridgeError("thread_context_rejected")


def identity(pid: int) -> Identity:
    with closing(win32api.OpenProcess(0x1000 | win32con.SYNCHRONIZE, False, pid)) as process:
        require(win32event.WaitForSingleObject(process, 0) == win32event.WAIT_TIMEOUT, "identity_rejected")
        times = [w.FILETIME() for _ in range(4)]
        require(bool(_kernel.GetProcessTimes(int(process), *(c.byref(t) for t in times))), "identity_rejected")
        created = times[0].dwLowDateTime | times[0].dwHighDateTime << 32
        path = c.create_unicode_buffer(32768)
        size = w.DWORD(len(path))
        require(bool(_kernel.QueryFullProcessImageNameW(int(process), 0, path, c.byref(size))), "identity_rejected")
        with closing(win32security.OpenProcessToken(process, win32con.TOKEN_QUERY)) as token:
            require(win32security.GetTokenInformation(token, win32security.TokenType) == 1, "identity_rejected")
            user = win32security.GetTokenInformation(token, win32security.TokenUser)[0]
            groups = win32security.GetTokenInformation(token, win32security.TokenGroups)
            logons = [sid for sid, flags in groups if flags & 0xC0000000 == 0xC0000000]
            require(len(logons) == 1, "identity_rejected")
            session = win32security.GetTokenInformation(token, win32security.TokenSessionId)
            authentication = win32security.GetTokenInformation(token, win32security.TokenStatistics)["AuthenticationId"]
            result = Identity(pid, created, session, win32security.ConvertSidToStringSid(user),
                              win32security.ConvertSidToStringSid(logons[0]), authentication, path.value)
        require(win32event.WaitForSingleObject(process, 0) == win32event.WAIT_TIMEOUT, "identity_rejected")
        return result


def file_identity(path: str) -> tuple:
    with closing(win32file.CreateFile(path, 0x80, 7, None, win32con.OPEN_EXISTING, 0, None)) as handle:
        info = _FileId()
        require(bool(_kernel.GetFileInformationByHandleEx(int(handle), 18, c.byref(info), c.sizeof(info))), "executable_rejected")
        return info.volume, bytes(info.identifier)


def _private(path: Path, user: str, directory: bool):
    access = 0x80 | win32con.READ_CONTROL
    if not directory:
        access |= win32con.GENERIC_READ
    flags = win32file.FILE_FLAG_OPEN_REPARSE_POINT | (win32con.FILE_FLAG_BACKUP_SEMANTICS if directory else 0)
    handle = win32file.CreateFile(str(path), access, 3 if directory else 1, None, win32con.OPEN_EXISTING, flags, None)
    try:
        info = win32file.GetFileInformationByHandle(handle)
        attributes = info[0]
        require(not attributes & win32con.FILE_ATTRIBUTE_REPARSE_POINT, "unsafe_discovery")
        require(bool(attributes & win32con.FILE_ATTRIBUTE_DIRECTORY) == directory, "unsafe_discovery")
        require(directory or (info[7] == 1 and win32file.GetFileType(handle) == win32file.FILE_TYPE_DISK), "unsafe_discovery")
        sd = win32security.GetSecurityInfo(handle, win32security.SE_FILE_OBJECT,
                                          win32security.OWNER_SECURITY_INFORMATION | win32security.DACL_SECURITY_INFORMATION)
        acl = sd.GetSecurityDescriptorDacl()
        require(win32security.ConvertSidToStringSid(sd.GetSecurityDescriptorOwner()) == user
                and acl is not None and acl.GetAceCount() == 1, "unsafe_discovery")
        (kind, flags), mask, sid = acl.GetAce(0)
        require(kind == 0 and mask == win32file.FILE_ALL_ACCESS
                and win32security.ConvertSidToStringSid(sid) == user, "unsafe_discovery")
        if directory:
            require(sd.GetSecurityDescriptorControl()[0] & 0x1000 != 0 and flags & 3 == 3, "unsafe_discovery")
        return handle
    except BaseException:
        handle.Close()
        raise


@dataclass(frozen=True)
class Discovery:
    instance: str
    pipe: str
    server: Identity


def discover(path: Path, expected_executable: Path, own: Identity) -> Discovery:
    try:
        with ExitStack() as stack:
            stack.enter_context(closing(_private(path.parent, own.user, True)))
            handle = stack.enter_context(closing(_private(path, own.user, False)))
            size = win32file.GetFileSize(handle)
            require(0 < size <= 128 * 1024, "invalid_discovery")
            _, raw = win32file.ReadFile(handle, size)
            require(len(raw) == size, "invalid_discovery")
            data = decode(raw, 128 * 1024)
            fields(data, "version instance pipe pid created session executable")
            require(type(data["version"]) is int and data["version"] == 1, "invalid_discovery")
            hex_bytes(data["instance"], 16)
            require(data["pipe"] == PIPE_PREFIX + data["instance"], "invalid_discovery")
            pid = decimal(data["pid"], 32)
            created = decimal(data["created"], 64)
            session = decimal(data["session"], 32, zero=True)
            exe = data["executable"]
            require(type(exe) is str and 0 < len(exe) <= 32768 and "\0" not in exe and os.path.isabs(exe), "invalid_discovery")
            actual = identity(pid)
            require(own.same_login(actual) and (actual.created, actual.session) == (created, session), "identity_rejected")
            # Never open a path supplied by the hint until the OS process path has matched it.
            require(os.path.normcase(os.path.normpath(exe)) == os.path.normcase(os.path.normpath(actual.executable)), "executable_rejected")
            require(file_identity(actual.executable) == file_identity(str(expected_executable)), "executable_rejected")
            return Discovery(data["instance"], data["pipe"], actual)
    except pywintypes.error as error:
        if error.winerror in (2, 3):
            raise BridgeError("application_unavailable") from None
        raise BridgeError("discovery_rejected") from None


class Pipe:
    def __init__(self, discovery: Discovery, own: Identity, cancel):
        self.discovery, self.own, self.cancel = discovery, own, cancel
        self.handle = None
        deadline = time.monotonic() + 3
        while self.handle is None:
            self._remaining(deadline)
            try:
                self.handle = win32file.CreateFile(discovery.pipe, 0x0012019B, 0, None,
                    win32con.OPEN_EXISTING, win32con.FILE_FLAG_OVERLAPPED | 0x00100000 | 0x00010000, None)
            except pywintypes.error as error:
                if error.winerror != 231:
                    raise BridgeError("pipe_unavailable") from None
                try:
                    win32pipe.WaitNamedPipe(discovery.pipe, 100)
                except pywintypes.error as wait_error:
                    if wait_error.winerror not in (121, 231):
                        raise BridgeError("pipe_unavailable") from None
        try:
            self.verify()
        except BaseException:
            self.close()
            raise

    def verify(self) -> None:
        actual = identity(win32pipe.GetNamedPipeServerProcessId(self.handle))
        require(actual == self.discovery.server and self.own.same_login(actual)
                and win32pipe.GetNamedPipeServerSessionId(self.handle) == actual.session, "identity_rejected")

    def close(self) -> None:
        if self.handle is not None:
            self.handle.Close()
            self.handle = None

    def _remaining(self, deadline: float) -> int:
        require(win32event.WaitForSingleObject(self.cancel, 0) != win32event.WAIT_OBJECT_0, "cancelled")
        remaining = deadline - time.monotonic()
        require(remaining > 0, "timeout")
        return max(1, int(remaining * 1000))

    def _io(self, buffer, deadline: float, *, write: bool):
        self._remaining(deadline)
        ov = pywintypes.OVERLAPPED()
        ov.hEvent = win32event.CreateEvent(None, True, False, None)
        try:
            try:
                error, retained = (win32file.WriteFile if write else win32file.ReadFile)(self.handle, buffer, ov)
                if error == 997:
                    wait = win32event.WaitForMultipleObjects((ov.hEvent, self.cancel), False, self._remaining(deadline))
                    if wait != win32event.WAIT_OBJECT_0:
                        raise BridgeError("cancelled" if wait == win32event.WAIT_OBJECT_0 + 1 else "timeout")
                count = win32file.GetOverlappedResult(self.handle, ov, False)
                return count if write else bytes(retained[:count])
            except BaseException:
                # Cancellation is issued on the initiating thread; drain before releasing borrowed buffers.
                try:
                    win32file.CancelIo(self.handle)
                except pywintypes.error:
                    pass
                try:
                    win32file.GetOverlappedResult(self.handle, ov, True)
                except pywintypes.error:
                    pass
                raise
        except pywintypes.error:
            raise BridgeError("connection_closed") from None
        finally:
            ov.hEvent.Close()

    def send(self, message: dict, deadline: float, limit: int = MAX_FRAME) -> None:
        self.verify()
        data = encode(message, limit)
        offset = 0
        while offset < len(data):
            sent = self._io(data[offset:], deadline, write=True)
            require(sent > 0, "connection_closed")
            offset += sent

    def receive(self, deadline: float, limit: int = MAX_FRAME) -> dict:
        def read_exact(size):
            data = bytearray()
            while len(data) < size:
                part = self._io(win32file.AllocateReadBuffer(size - len(data)), deadline, write=False)
                require(bool(part), "connection_closed")
                data.extend(part)
            return bytes(data)
        size = int.from_bytes(read_exact(4), "big")
        require(0 < size <= limit)
        message = decode(read_exact(size), limit)
        self.verify()
        return message
