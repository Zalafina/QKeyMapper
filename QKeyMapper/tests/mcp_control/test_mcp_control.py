import argparse
import ctypes as c
import ctypes.wintypes as w
import json
import os
import pathlib
import shutil
import subprocess
import time
import uuid
ROOT = pathlib.Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser(description='Run MCP control tests on a private Windows desktop without switching the input desktop.')
parser.add_argument('--test-exe', type=pathlib.Path, required=True)
parser.add_argument('--ipc-exe', type=pathlib.Path, required=True)
parser.add_argument('--qt-bin', type=pathlib.Path, required=True)
parser.add_argument('--output', type=pathlib.Path, required=True)
parser.add_argument('--powershell', default=shutil.which('pwsh') or shutil.which('powershell'))
parser.add_argument('--screenshots', action='store_true')
args = parser.parse_args()
if os.name != 'nt':
    parser.error('Windows is required.')
if not __debug__:
    parser.error('Run without Python -O; validation asserts must remain enabled.')
args.test_exe = args.test_exe.resolve()
args.ipc_exe = args.ipc_exe.resolve()
args.qt_bin = args.qt_bin.resolve()
BASE = args.output.resolve()
if not BASE.is_relative_to((ROOT / 'out').resolve()):
    parser.error('--output must be inside this checkout out/.')
for exe, name in ((args.test_exe, 'QKeyMapperMcpControlTest.exe'), (args.ipc_exe, 'QKeyMapperMcpIpcTest.exe')):
    if not exe.is_file() or exe.name != name:
        parser.error('Build and select the intended ' + name)
if not args.qt_bin.is_dir():
    parser.error('--qt-bin is not a directory.')
if args.screenshots and (not args.powershell):
    parser.error('PowerShell is required for screenshots.')
BASE.mkdir(parents=True, exist_ok=True)
k = c.WinDLL('kernel32', use_last_error=True)
u = c.WinDLL('user32', use_last_error=True)
a = c.WinDLL('advapi32', use_last_error=True)

class SA(c.Structure):
    _fields_ = [('nLength', w.DWORD), ('lpSecurityDescriptor', w.LPVOID), ('bInheritHandle', w.BOOL)]

class SI(c.Structure):
    _fields_ = [('cb', w.DWORD), ('lpReserved', w.LPWSTR), ('lpDesktop', w.LPWSTR), ('lpTitle', w.LPWSTR), ('dwX', w.DWORD), ('dwY', w.DWORD), ('dwXSize', w.DWORD), ('dwYSize', w.DWORD), ('dwXCountChars', w.DWORD), ('dwYCountChars', w.DWORD), ('dwFillAttribute', w.DWORD), ('dwFlags', w.DWORD), ('wShowWindow', w.WORD), ('cbReserved2', w.WORD), ('lpReserved2', w.LPVOID), ('hStdInput', w.HANDLE), ('hStdOutput', w.HANDLE), ('hStdError', w.HANDLE)]

class PI(c.Structure):
    _fields_ = [('hProcess', w.HANDLE), ('hThread', w.HANDLE), ('dwProcessId', w.DWORD), ('dwThreadId', w.DWORD)]
k.CreateProcessW.argtypes = [w.LPCWSTR, w.LPWSTR, w.LPVOID, w.LPVOID, w.BOOL, w.DWORD, w.LPVOID, w.LPCWSTR, c.POINTER(SI), c.POINTER(PI)]
k.CreateProcessW.restype = w.BOOL
k.CreateFileW.argtypes = [w.LPCWSTR, w.DWORD, w.DWORD, c.POINTER(SA), w.DWORD, w.DWORD, w.HANDLE]
k.CreateFileW.restype = w.HANDLE
k.GetCurrentProcess.restype = w.HANDLE
k.CloseHandle.argtypes = [w.HANDLE]
k.WaitForSingleObject.argtypes = [w.HANDLE, w.DWORD]
k.GetExitCodeProcess.argtypes = [w.HANDLE, c.POINTER(w.DWORD)]
k.TerminateProcess.argtypes = [w.HANDLE, w.UINT]
a.OpenProcessToken.argtypes = [w.HANDLE, w.DWORD, c.POINTER(w.HANDLE)]
a.GetTokenInformation.argtypes = [w.HANDLE, c.c_int, w.LPVOID, w.DWORD, c.POINTER(w.DWORD)]
u.CreateDesktopW.argtypes = [w.LPCWSTR, w.LPCWSTR, w.LPVOID, w.DWORD, w.DWORD, w.LPVOID]
u.CreateDesktopW.restype = w.HANDLE
u.CloseDesktop.argtypes = [w.HANDLE]
u.PostMessageW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]
u.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]

def elevation(process):
    token = w.HANDLE()
    assert a.OpenProcessToken(process, 8, c.byref(token))
    v = w.DWORD()
    n = w.DWORD()
    try:
        assert a.GetTokenInformation(token, 20, c.byref(v), 4, c.byref(n))
        return v.value
    finally:
        k.CloseHandle(token)
assert elevation(k.GetCurrentProcess()) == 0
# Never switch the input desktop.
name = 'QkmMcp_' + uuid.uuid4().hex
desktop = u.CreateDesktopW(name, None, None, 0, 511, None)
assert desktop, c.get_last_error()

def start(args, cwd, env, log):
    sa = SA(c.sizeof(SA), None, True)
    out = k.CreateFileW(str(log), 1073741824, 1, c.byref(sa), 2, 0, None)
    inp = k.CreateFileW('NUL', 2147483648, 3, c.byref(sa), 3, 0, None)
    assert out not in (None, c.c_void_p(-1).value) and inp not in (None, c.c_void_p(-1).value)
    info = SI()
    info.cb = c.sizeof(SI)
    info.lpDesktop = 'winsta0\\' + name
    info.dwFlags = 256
    info.hStdInput = inp
    info.hStdOutput = out
    info.hStdError = out
    block = c.create_unicode_buffer('\x00'.join((f'{x}={y}' for x, y in sorted(env.items(), key=lambda z: z[0].upper()))) + '\x00\x00')
    cmd = c.create_unicode_buffer(subprocess.list2cmdline([str(x) for x in args]))
    pi = PI()
    try:
        assert k.CreateProcessW(None, cmd, None, None, True, 134218752, block, str(cwd), c.byref(info), c.byref(pi)), c.get_last_error()
    finally:
        k.CloseHandle(inp)
        k.CloseHandle(out)
    assert elevation(pi.hProcess) == 0
    return pi

def finish(pi, timeout=60000):
    try:
        if k.WaitForSingleObject(pi.hProcess, timeout) != 0:
            k.TerminateProcess(pi.hProcess, 99)
            k.WaitForSingleObject(pi.hProcess, 5000)
            raise RuntimeError('owned test timed out')
        result = w.DWORD()
        assert k.GetExitCodeProcess(pi.hProcess, c.byref(result))
        return result.value
    finally:
        k.CloseHandle(pi.hThread)
        k.CloseHandle(pi.hProcess)

def environment():
    env = os.environ.copy()
    env['PATH'] = str(args.qt_bin) + os.pathsep + env['PATH']
    env['QKM_TEST_DESKTOP'] = name
    env['QKM_IPC_TEST_EXE'] = str(args.ipc_exe)
    return env
try:
    env = environment()
    pi = start([args.test_exe, '-o', str(BASE / 'results.txt') + ',txt'], BASE, env, BASE / 'runtime.log')
    pid = pi.dwProcessId
    result = finish(pi)
    record = {'desktop': name, 'pid': pid, 'elevated': 0, 'exit': result, 'test_exe': str(args.test_exe), 'ipc_exe': str(args.ipc_exe), 'qt_bin': str(args.qt_bin)}
    (BASE / 'run_private.json').write_text(json.dumps(record, indent=2), encoding='utf-8')
    print(json.dumps(record), flush=True)
    if result:
        raise RuntimeError('Control tests failed; see results.txt and runtime.log.')
    if args.screenshots:
        for lang in (0, 1, 2):
            for state in ('off', 'pending', 'paired-off', 'busy-off'):
                label = f'mcp_control_{lang}_{state}_{uuid.uuid4().hex[:6]}'
                statefile = BASE / (label + '.json')
                pi = start([args.test_exe, '--preview', lang, state, statefile], BASE, env, BASE / (label + '.log'))
                try:
                    deadline = time.monotonic() + 10
                    while not statefile.exists():
                        assert time.monotonic() < deadline, 'No preview state'
                        assert k.WaitForSingleObject(pi.hProcess, 0) == 258, 'Preview exited'
                        time.sleep(0.05)
                    record = json.loads(statefile.read_text(encoding='utf-8'))
                    assert record['desktop'] == name and record['pid'] == pi.dwProcessId
                    assert record['native_visible'] and record['native_dpi'] > 0 and record['qt_dpr'] > 0
                    assert not record['interface_enabled']
                    if state in ('paired-off', 'busy-off'):
                        assert record['pair_count'] == 1 and record['revoke_enabled']
                        assert record['store_busy'] == (state == 'busy-off')
                    # The capture process checks HWND ownership and closes it on that private desktop.
                    cap = start([args.powershell, '-NoProfile', '-File', pathlib.Path(__file__).with_name('capture_mcp_control.ps1'), '-TestPid', pi.dwProcessId, '-WindowHandle', record['hwnd'], '-Label', label], ROOT, env, BASE / (label + '_capture.log'))
                    assert finish(cap, 15000) == 0
                    dest = BASE / (label + '.png')
                    shutil.copy2(ROOT / 'test_snapshots' / ('snapshot_' + label + '.png'), dest)
                    print(dest, flush=True)
                finally:
                    assert finish(pi, 25000) == 0
finally:
    u.CloseDesktop(desktop)
