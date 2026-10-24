"""Bounded ENGINE-61 keyboard and process measurements, without product instrumentation."""

import ctypes
from ctypes import wintypes
import math
import time

import win32api
import win32con
import win32gui
import win32process

from receiver import desktop_name, input_desktop_name


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [("dx", wintypes.LONG), ("dy", wintypes.LONG), ("mouseData", wintypes.DWORD),
                ("dwFlags", wintypes.DWORD), ("time", wintypes.DWORD), ("dwExtraInfo", ctypes.c_size_t)]


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [("wVk", wintypes.WORD), ("wScan", wintypes.WORD), ("dwFlags", wintypes.DWORD),
                ("time", wintypes.DWORD), ("dwExtraInfo", ctypes.c_size_t)]


class HARDWAREINPUT(ctypes.Structure):
    _fields_ = [("uMsg", wintypes.DWORD), ("wParamL", wintypes.WORD), ("wParamH", wintypes.WORD)]


class PAYLOAD(ctypes.Union):
    _fields_ = [("mi", MOUSEINPUT), ("ki", KEYBDINPUT), ("hi", HARDWAREINPUT)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wintypes.DWORD), ("payload", PAYLOAD)]


class THREADENTRY32(ctypes.Structure):
    _fields_ = [("dwSize", wintypes.DWORD), ("cntUsage", wintypes.DWORD), ("th32ThreadID", wintypes.DWORD),
                ("th32OwnerProcessID", wintypes.DWORD), ("tpBasePri", wintypes.LONG),
                ("tpDeltaPri", wintypes.LONG), ("dwFlags", wintypes.DWORD)]


user32 = ctypes.WinDLL("user32", use_last_error=True)
user32.SendInput.argtypes = [wintypes.UINT, ctypes.POINTER(INPUT), ctypes.c_int]
user32.SendInput.restype = wintypes.UINT
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
kernel32.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
kernel32.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
kernel32.Thread32First.argtypes = [wintypes.HANDLE, ctypes.POINTER(THREADENTRY32)]
kernel32.Thread32Next.argtypes = kernel32.Thread32First.argtypes
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
kernel32.OpenThread.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
kernel32.OpenThread.restype = wintypes.HANDLE
kernel32.GetProcessIdOfThread.argtypes = [wintypes.HANDLE]
kernel32.GetProcessIdOfThread.restype = wintypes.DWORD
kernel32.GetThreadTimes.argtypes = [wintypes.HANDLE, *([ctypes.POINTER(wintypes.FILETIME)] * 4)]
kernel32.GetThreadTimes.restype = wintypes.BOOL
thread_description = getattr(kernel32, "GetThreadDescription", None)
if thread_description is not None:
    thread_description.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.LPWSTR)]
    thread_description.restype = ctypes.c_long
kernel32.LocalFree.argtypes = [ctypes.c_void_p]
kernel32.LocalFree.restype = ctypes.c_void_p
assert ctypes.sizeof(INPUT) == 40 and ctypes.sizeof(KEYBDINPUT) == 24, "Use the pinned x64 runtime"
SOURCE_KEYS = frozenset((0x41, *range(0x7C, 0x82), 0x87))  # A, F13-F18, F24.
SOURCE_TAG = 0x514B4D6100000000


def keyboard_input(key, down):
    assert key in SOURCE_KEYS
    return INPUT(1, PAYLOAD(ki=KEYBDINPUT(key, 0, 0 if down else win32con.KEYEVENTF_KEYUP, 0, SOURCE_TAG)))


def focus_click(window, focus_window, pid, *, allowed):
    assert allowed and desktop_name() == input_desktop_name() == "Default"
    assert not ctypes.windll.shell32.IsUserAnAdmin()
    assert win32process.GetWindowThreadProcessId(window)[1] == pid
    assert win32process.GetWindowThreadProcessId(focus_window)[1] == pid
    assert not any(win32api.GetAsyncKeyState(key) & 0x8000 for key in range(1, 255))
    assert win32gui.IsChild(window, focus_window)
    left, top, right, bottom = win32gui.GetWindowRect(focus_window)
    point, previous = ((left + right) // 2, (top + bottom) // 2), win32api.GetCursorPos()
    win32gui.SetWindowPos(window, win32con.HWND_TOP, 0, 0, 0, 0,
        win32con.SWP_NOMOVE | win32con.SWP_NOSIZE | win32con.SWP_NOACTIVATE)
    try:
        win32api.SetCursorPos(point)
        assert win32gui.WindowFromPoint(point) == focus_window, "Owned receiver is obscured; click refused"
        events = (INPUT * 2)(*(INPUT(0, PAYLOAD(mi=MOUSEINPUT(0, 0, 0, flag, 0, SOURCE_TAG)))
                             for flag in (win32con.MOUSEEVENTF_LEFTDOWN, win32con.MOUSEEVENTF_LEFTUP)))
        accepted = user32.SendInput(2, events, ctypes.sizeof(INPUT))
        if accepted == 1:
            release = INPUT(0, PAYLOAD(mi=MOUSEINPUT(0, 0, 0, win32con.MOUSEEVENTF_LEFTUP, 0, SOURCE_TAG)))
            assert user32.SendInput(1, ctypes.byref(release), ctypes.sizeof(INPUT)) == 1, "Test click release failed"
        assert accepted == 2, "Focus click rejected"
    finally:
        if win32api.GetCursorPos() == point:
            win32api.SetCursorPos(previous)


class Sender:
    def __init__(self, window, pid, *, allow_default):
        assert allow_default and desktop_name() == "Default"
        assert not ctypes.windll.shell32.IsUserAnAdmin(), "Input controller must remain ordinary"
        self.window, self.pid, self.held = window, pid, set()
        assert not any(win32api.GetAsyncKeyState(key) & 0x8000 for key in SOURCE_KEYS | frozenset(range(0x42, 0x46)))
        self.check_target()

    def check_target(self):
        assert input_desktop_name() == "Default", "Input desktop changed; input stopped"
        assert win32gui.IsWindow(self.window), "Owned receiver closed"
        assert win32process.GetWindowThreadProcessId(self.window)[1] == self.pid, "Receiver identity changed"
        assert win32gui.GetForegroundWindow() == self.window, "Focus left the owned receiver; input stopped"
        assert not any(win32api.GetAsyncKeyState(key) & 0x8000 for key in
                       (win32con.VK_SHIFT, win32con.VK_CONTROL, win32con.VK_MENU, win32con.VK_LWIN, win32con.VK_RWIN)), \
            "A modifier is held; input stopped"

    def send(self, key, down):
        self.check_target()
        assert (key not in self.held) if down else (key in self.held)
        event = keyboard_input(key, down)
        sent_at = time.perf_counter_ns()
        assert user32.SendInput(1, ctypes.byref(event), ctypes.sizeof(INPUT)) == 1, "SendInput rejected"
        if down:
            self.held.add(key)
        else:
            self.held.remove(key)
        return {"key": key, "down": down, "qpc_ns": sent_at}

    def release(self):
        # Only this sender's accepted key-downs are released, even if focus has changed.
        for key in tuple(self.held):
            event = keyboard_input(key, False)
            if user32.SendInput(1, ctypes.byref(event), ctypes.sizeof(INPUT)) == 1:
                self.held.remove(key)
        assert not self.held, "Test-owned keys could not be released"

    def force_original_up(self, mapped_tag):
        self.check_target()
        assert self.held == {0x41}, "Only the test-held A may receive the late-release probe"
        assert type(mapped_tag) is int and 0 < mapped_tag < 2**64 and mapped_tag & 0xF == 0
        assert mapped_tag != SOURCE_TAG, "Use the tag observed from this product's mapped B"
        event = keyboard_input(0x41, False)
        event.payload.ki.dwExtraInfo = mapped_tag | 1  # SENDVIRTUALKEY_STATE_FORCE.
        sent_at = time.perf_counter_ns()
        assert user32.SendInput(1, ctypes.byref(event), ctypes.sizeof(INPUT)) == 1, "Late-release probe rejected"
        return {"key": 0x41, "down": False, "qpc_ns": sent_at, "extra_info": mapped_tag | 1}


def distribution(values):
    assert values and all(math.isfinite(value) and value >= 0 for value in values)
    ordered = sorted(values)
    return {"count": len(ordered), "min": ordered[0], "p50": ordered[math.ceil(len(ordered) * .5) - 1],
            "p95": ordered[math.ceil(len(ordered) * .95) - 1],
            "p99": ordered[math.ceil(len(ordered) * .99) - 1], "max": ordered[-1]}


def plain_latencies(sent, received, output_key):
    assert len(sent) == len(received) and sent, "Missing or extra receiver events"
    assert len(sent) % 2 == 0
    delays = []
    for index, (source, target) in enumerate(zip(sent, received)):
        assert source["key"] == 0x41 and target["key"] == output_key, "Wrong mapping output"
        assert source["down"] == target["down"] == (index % 2 == 0), "Unbalanced or reordered input"
        delay = (target["qpc_ns"] - source["qpc_ns"]) / 1_000_000
        assert 0 <= delay < 1000, "Invalid or unbounded receiver latency"
        delays.append(delay)
    return distribution(delays)


def _thread_sample(thread_id, pid):
    result = {"thread_id": thread_id}
    thread = kernel32.OpenThread(0x0800, False, thread_id)  # THREAD_QUERY_LIMITED_INFORMATION.
    if not thread:
        return {**result, "read_error": ctypes.get_last_error()}
    try:
        owner = kernel32.GetProcessIdOfThread(thread)
        if not owner:
            return {**result, "read_error": ctypes.get_last_error()}
        if owner != pid:
            return {**result, "owner_changed": True}
        created, exited, kernel, user = (wintypes.FILETIME() for _ in range(4))
        if not kernel32.GetThreadTimes(thread, ctypes.byref(created), ctypes.byref(exited),
                                      ctypes.byref(kernel), ctypes.byref(user)):
            return {**result, "read_error": ctypes.get_last_error()}
        def ticks(value):
            return value.dwLowDateTime | (value.dwHighDateTime << 32)
        result.update(creation_100ns=ticks(created), cpu_100ns=ticks(kernel) + ticks(user),
                      kernel_100ns=ticks(kernel), user_100ns=ticks(user), exited=bool(ticks(exited)))
        if thread_description is not None:
            name = wintypes.LPWSTR()
            try:
                status = thread_description(thread, ctypes.byref(name))
                if status >= 0 and name:
                    result["name"] = name.value[:128]
                elif status < 0:
                    result["name_error"] = status
            finally:
                if name:
                    kernel32.LocalFree(ctypes.cast(name, ctypes.c_void_p))
        return result
    finally:
        kernel32.CloseHandle(thread)


def process_sample(process, pid, window, *, thread_cpu=False):
    times = win32process.GetProcessTimes(process)
    memory = win32process.GetProcessMemoryInfo(process)
    snapshot = kernel32.CreateToolhelp32Snapshot(4, 0)  # TH32CS_SNAPTHREAD.
    if snapshot == ctypes.c_void_p(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    threads, details = 0, []
    try:
        entry = THREADENTRY32()
        entry.dwSize = ctypes.sizeof(entry)
        more = kernel32.Thread32First(snapshot, ctypes.byref(entry))
        while more:
            if entry.th32OwnerProcessID == pid:
                threads += 1
                if thread_cpu:
                    details.append(_thread_sample(entry.th32ThreadID, pid))
            more = kernel32.Thread32Next(snapshot, ctypes.byref(entry))
        assert ctypes.get_last_error() == 18, "Thread snapshot failed"  # ERROR_NO_MORE_FILES.
    finally:
        kernel32.CloseHandle(snapshot)
    started = time.perf_counter_ns()
    try:
        win32gui.SendMessageTimeout(window, win32con.WM_NULL, 0, 0, win32con.SMTO_ABORTIFHUNG, 500)
        gui = {"wm_null_ms": (time.perf_counter_ns() - started) / 1_000_000}
    except Exception:
        gui = {"wm_null_unavailable": True}  # UIPI failure is not a latency sample.
    detail = {"thread_cpu": details, "kernel_100ns": times["KernelTime"], "user_100ns": times["UserTime"]} if thread_cpu else {}
    return {"qpc_ns": time.perf_counter_ns(), "cpu_100ns": times["KernelTime"] + times["UserTime"],
            "working_set": memory["WorkingSetSize"], "private_commit": memory["PagefileUsage"],
            "threads": threads, **gui, **detail}
