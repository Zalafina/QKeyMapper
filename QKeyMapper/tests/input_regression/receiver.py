"""Owned, ordinary Win32 key receiver; it never installs hooks or injects input."""

import argparse
import ctypes
from ctypes import wintypes
import json
import os
from pathlib import Path
import re
import sys
import time

import win32api
import win32con
import win32gui


ROOT = Path(__file__).resolve().parents[3]
RESULT_ROOT = ROOT / "out/mcp_product/engine61"
RESET, FLUSH, CLOSE, COUNT = (win32con.WM_APP + index for index in range(1, 5))
ALLOWED_KEYS = frozenset((*range(0x41, 0x46), *range(0x7C, 0x88)))
TITLE = "QKeyMapper ENGINE-61 键盘接收窗口"
user32 = ctypes.WinDLL("user32", use_last_error=True)
user32.GetThreadDesktop.argtypes = [wintypes.DWORD]
user32.GetThreadDesktop.restype = wintypes.HANDLE
user32.GetUserObjectInformationW.argtypes = [wintypes.HANDLE, ctypes.c_int, wintypes.LPVOID,
                                            wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)]
user32.GetUserObjectInformationW.restype = wintypes.BOOL
user32.GetMessageExtraInfo.argtypes = []
user32.GetMessageExtraInfo.restype = ctypes.c_ssize_t
user32.SetTimer.argtypes = [wintypes.HWND, ctypes.c_size_t, wintypes.UINT, wintypes.LPVOID]
user32.SetTimer.restype = ctypes.c_size_t
user32.OpenInputDesktop.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
user32.OpenInputDesktop.restype = wintypes.HANDLE
user32.CloseDesktop.argtypes = [wintypes.HANDLE]
user32.CloseDesktop.restype = wintypes.BOOL


def desktop_name():
    name, needed = ctypes.create_unicode_buffer(256), wintypes.DWORD()
    desktop = user32.GetThreadDesktop(win32api.GetCurrentThreadId())
    if not user32.GetUserObjectInformationW(desktop, 2, name, ctypes.sizeof(name), ctypes.byref(needed)):
        raise ctypes.WinError(ctypes.get_last_error())
    return name.value


def input_desktop_name():
    desktop = user32.OpenInputDesktop(0, False, 1)
    if not desktop:
        return None
    try:
        name, needed = ctypes.create_unicode_buffer(256), wintypes.DWORD()
        if not user32.GetUserObjectInformationW(desktop, 2, name, ctypes.sizeof(name), ctypes.byref(needed)):
            return None
        return name.value
    finally:
        user32.CloseDesktop(desktop)


def write_json(path, value):
    temporary = path.with_name(path.name + ".new")
    temporary.write_text(json.dumps(value, ensure_ascii=False), encoding="utf-8")
    temporary.replace(path)


def run(args):
    assert not ctypes.windll.shell32.IsUserAnAdmin(), "Receiver must remain non-elevated"
    actual_desktop = desktop_name()
    assert actual_desktop == args.desktop
    assert re.fullmatch(r"QkmReceiver_[0-9a-f]{32}", actual_desktop) or (
        actual_desktop == "Default" and args.allow_default and not args.self_test)
    assert args.output.is_relative_to(RESULT_ROOT.resolve()) and args.output != RESULT_ROOT.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    events, sequence, aborted = [], 0, False

    def on_message(window, message, key, data):
        nonlocal sequence, aborted
        try:
            if message == RESET:
                assert 0 < key <= 1000
                events.clear()
                sequence = key
                write_json(args.output / ("reset-" + str(sequence) + ".json"), {"sequence": sequence})
                return 0
            if message == FLUSH:
                assert sequence == key
                write_json(args.output / ("events-" + str(sequence) + ".json"),
                           {"sequence": sequence, "events": events, "desktop": actual_desktop})
                return 0
            if message == COUNT:
                assert key == 0 or key in ALLOWED_KEYS
                return sum(event["key"] == key for event in events) if key else len(events)
            if message in (CLOSE, win32con.WM_CLOSE):
                aborted = message == win32con.WM_CLOSE
                win32gui.DestroyWindow(window)
                return 0
            if message in (win32con.WM_KEYDOWN, win32con.WM_KEYUP, win32con.WM_SYSKEYDOWN, win32con.WM_SYSKEYUP):
                if key == win32con.VK_ESCAPE:
                    aborted = True
                    win32gui.DestroyWindow(window)
                    return 0
                if key in ALLOWED_KEYS:
                    assert len(events) < 50000, "Receiver event limit exceeded"
                    events.append({"key": key, "down": message in (win32con.WM_KEYDOWN, win32con.WM_SYSKEYDOWN),
                                   "qpc_ns": time.perf_counter_ns(),
                                   "extra_info": ctypes.c_size_t(user32.GetMessageExtraInfo()).value})
                return 0
            if message == win32con.WM_TIMER:
                aborted = True
                win32gui.DestroyWindow(window)
                return 0
            if message == win32con.WM_DESTROY:
                win32gui.PostQuitMessage(0)
                return 0
        except Exception:
            aborted = True
            write_json(args.output / "error.json", {"error": "receiver_protocol_or_event_limit"})
            win32gui.DestroyWindow(window)
            return 0
        return win32gui.DefWindowProc(window, message, key, data)

    window_class = win32gui.WNDCLASS()
    window_class.hInstance = win32api.GetModuleHandle(None)
    window_class.lpszClassName = "QkmEngine61Receiver_" + str(os.getpid())
    window_class.lpfnWndProc = on_message
    window_class.hCursor = win32gui.LoadCursor(0, win32con.IDC_ARROW)
    window_class.hbrBackground = win32con.COLOR_WINDOW + 1
    atom = win32gui.RegisterClass(window_class)
    window = win32gui.CreateWindowEx(0, atom, TITLE, win32con.WS_OVERLAPPEDWINDOW,
                                    80, 80, 700, 180, 0, 0, window_class.hInstance, None)
    win32gui.CreateWindowEx(0, "STATIC", "自动键盘回归测试：请保持本窗口焦点，暂勿操作键鼠。\r\n"
        "按 Esc 可中止；切换焦点后测试控制器会停止发送。\r\n只记录固定测试键，不保存输入文本。",
        win32con.WS_CHILD | win32con.WS_VISIBLE, 16, 10, 650, 74, window, 0, window_class.hInstance, None)
    focus_window = win32gui.CreateWindowEx(0, "BUTTON", "键盘测试接收区（Esc 中止）",
        win32con.WS_CHILD | win32con.WS_VISIBLE | win32con.WS_TABSTOP,
        16, 88, 650, 30, window, 1001, window_class.hInstance, None)

    def on_key_message(child, message, key, data):
        if message in (win32con.WM_KEYDOWN, win32con.WM_KEYUP, win32con.WM_SYSKEYDOWN, win32con.WM_SYSKEYUP):
            return on_message(window, message, key, data)
        return win32gui.CallWindowProc(original_key_proc, child, message, key, data)

    original_key_proc = win32gui.SetWindowLong(focus_window, win32con.GWL_WNDPROC, on_key_message)
    win32gui.ShowWindow(window, win32con.SW_SHOWNOACTIVATE)
    win32gui.UpdateWindow(window)
    if not user32.SetTimer(window, 1, 30000 if args.self_test else 240000, None):
        raise ctypes.WinError(ctypes.get_last_error())
    write_json(args.output / "started.json", {"pid": os.getpid(), "window": window, "focus_window": focus_window,
        "desktop": actual_desktop, "executable": sys.executable, "self_test": args.self_test,
        "clock": time.get_clock_info("perf_counter").implementation})
    try:
        win32gui.PumpMessages()
    finally:
        write_json(args.output / "closed.json", {"pid": os.getpid(), "aborted": aborted, "events": events})
        win32gui.UnregisterClass(window_class.lpszClassName, window_class.hInstance)
    return 2 if aborted else 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--desktop", required=True)
    parser.add_argument("--allow-default", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    options = parser.parse_args()
    options.output = options.output.resolve()
    sys.exit(run(options))
