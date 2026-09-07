#include "../crash_client.h"
#include "../hang_monitor.h"
#include "../crash_files.h"
#include <strsafe.h>
#include <cstdio>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

using namespace QkmDiagnostics;
namespace {
void selfTest()
{
    HangMonitor monitor;
    Observation value;
    value.phase = Running;
    value.expected = (1 << SlotCount) - 1;
    LONG stalled = 0;
    assert(!monitor.sample(value, 0, false, stalled));
    for (ULONGLONG now = 2000; now <= 32000; now += 2000) {
        for (unsigned i : { Ui, Hook, Interception }) ++value.counts[i];
        const bool capture = monitor.sample(value, now, false, stalled);
        assert(capture == (now == 30000));
        if (now >= 30000) assert(stalled == (1 << Worker));
    }
    for (ULONGLONG now = 34000; now <= 602000; now += 2000) {
        for (auto &count : value.counts) ++count;
        assert(!monitor.sample(value, now, false, stalled));
    }
    assert(monitor.sample(value, 632000, false, stalled));
    assert(!monitor.sample(value, 634000, false, stalled));
    assert(!monitor.sample(value, 700000, true, stalled));
    assert(!monitor.sample(value, 702000, false, stalled));
    HangMonitor longThreshold;
    value.timeoutSeconds = 120;
    assert(!longThreshold.sample(value, 0, false, stalled));
    assert(longThreshold.sample(value, 120000, false, stalled));
    longThreshold.reset(value, 122000);
    for (ULONGLONG now = 124000; now <= 800000; now += 2000) {
        for (unsigned i : { Ui, Hook, Interception }) ++value.counts[i];
        assert(!longThreshold.sample(value, now, false, stalled));
    }
    value.timeoutSeconds = DefaultHangSeconds;
    HangMonitor lifecycle;
    value.phase = Starting;
    assert(!lifecycle.sample(value, 0, false, stalled));
    assert(!lifecycle.sample(value, 119999, false, stalled));
    assert(lifecycle.sample(value, 120000, false, stalled));
    HangMonitor shutdown;
    value.phase = Stopping;
    assert(!shutdown.sample(value, 0, false, stalled));
    ++value.progress;
    assert(!shutdown.sample(value, 59000, false, stalled));
    assert(!shutdown.sample(value, 60000, false, stalled));
    assert(shutdown.sample(value, 119000, false, stalled));
    HangMonitor limits;
    value.phase = Running;
    for (unsigned attempt = 0; attempt < 4; ++attempt) {
        const ULONGLONG base = attempt * 700000ULL;
        for (ULONGLONG now = base; now <= base + 62000; now += 2000) {
            for (auto &count : value.counts) ++count;
            assert(!limits.sample(value, now, false, stalled));
        }
        assert(limits.sample(value, base + 92000, false, stalled) == (attempt < 3));
    }
    std::puts("Heartbeat deadline, isolation, recovery, cooldown, lifecycle and attempt-limit checks passed.");
}
__declspec(noinline) void crashHere()
{
    RaiseException(0xE0424B4D, EXCEPTION_NONCONTINUABLE, 0, nullptr);
}
DWORD WINAPI crashThread(void *event)
{
    if (event) WaitForSingleObject(static_cast<HANDLE>(event), INFINITE);
    crashHere();
    return 0;
}
struct PulseThread { Slot slot; HANDLE stop; HANDLE resume; bool stalled; };
DWORD WINAPI pulseThread(void *parameter)
{
    const auto &pulse = *static_cast<PulseThread *>(parameter);
    beginHeartbeat(pulse.slot);
    if (pulse.stalled) WaitForSingleObject(pulse.resume, INFINITE);
    while (WaitForSingleObject(pulse.stop, HeartbeatIntervalMs) == WAIT_TIMEOUT) heartbeat(pulse.slot);
    return 0;
}
int probeDirectory()
{
    WCHAR path[PathCapacity] = {};
    const DWORD count = GetModuleFileNameW(nullptr, path, PathCapacity);
    if (!count || count >= PathCapacity) return ERROR_FILENAME_EXCED_RANGE;
    WCHAR *slash = wcsrchr(path, L'\\');
    if (!slash) return ERROR_BAD_PATHNAME;
    if (slash == path + 2 && path[1] == L':') slash[1] = 0;
    else *slash = 0;
    Directory output;
    if (!selectDirectory(path, output)) return static_cast<int>(GetLastError());
    const std::wstring probe = childPath(output.path, L"diagnostic-directory-probe.tmp");
    HANDLE file = CreateFileW(probe.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                             FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    const DWORD error = file == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    std::wprintf(L"fallback=%u;directory_error=%lu;file_error=%lu;path=%s\n",
                 output.fallback, output.creationError, error, output.path.c_str());
    return static_cast<int>(error);
}
}
int wmain(int count, WCHAR **arguments)
{
    const std::wstring mode = count == 2 ? arguments[1] : L"--self-test";
    if (mode == L"--self-test") { selfTest(); return 0; }
    if (mode == L"--probe-directory") return probeDirectory();
    if (mode == L"--cleanup") {
        WCHAR path[PathCapacity] = {};
        const DWORD length = GetModuleFileNameW(nullptr, path, PathCapacity);
        if (!length || length >= PathCapacity) return ERROR_FILENAME_EXCED_RANGE;
        WCHAR *slash = wcsrchr(path, L'\\');
        if (!slash) return ERROR_BAD_PATHNAME;
        if (slash == path + 2 && path[1] == L':') slash[1] = 0;
        else *slash = 0;
        return static_cast<int>(cleanupFiles(path));
    }
    if (mode != L"--crash-startup" && mode != L"--crash-ui" && mode != L"--crash-worker"
        && mode != L"--race-crash" && mode != L"--normal" && mode != L"--kill-helper"
        && mode != L"--hang-ui" && mode != L"--hang-worker" && mode != L"--hang-hook"
        && mode != L"--hang-interception" && mode != L"--hang-startup" && mode != L"--hang-shutdown"
        && mode != L"--exit-without-cleanup" && mode != L"--missing-import") return ERROR_INVALID_PARAMETER;
    const bool missingImport = mode == L"--missing-import";
    const UINT testErrorMode = SEM_NOGPFAULTERRORBOX | (missingImport ? 0 : SEM_FAILCRITICALERRORS);
    SetErrorMode(testErrorMode);
    Session session("native-test", "isolated-test");
    if (GetErrorMode() != testErrorMode) return ERROR_INVALID_DATA;
    if (missingImport) {
        const DWORD error = startupError();
        std::printf("Diagnostic startup failed: 0x%08lX; target continued; error mode restored.\n", error);
        // Windows can report a loader NTSTATUS or a CreateProcess Win32 error.
        if (error != 0xC0000135UL && error != ERROR_MOD_NOT_FOUND) return ERROR_INVALID_DATA;
        if (reporterHandle() || hangEnabled()) return ERROR_INVALID_DATA;
        Sleep(2000);
        return 0;
    }
    if (startupError()) return static_cast<int>(startupError());
    if (mode == L"--crash-startup") crashHere();
    if (mode == L"--exit-without-cleanup") ExitProcess(0);
    if (mode == L"--kill-helper") { TerminateProcess(reporterHandle(), ERROR_PROCESS_ABORTED); return 0; }
    if (mode == L"--hang-startup") { Sleep(137000); return 0; }
    if (mode == L"--hang-shutdown") { markPhase(Stopping); Sleep(77000); return 0; }
    markPhase(Running);
    if (mode == L"--crash-ui") crashHere();
    if (mode == L"--crash-worker") {
        HANDLE thread = CreateThread(nullptr, 0, crashThread, nullptr, 0, nullptr);
        if (!thread) return static_cast<int>(GetLastError());
        WaitForSingleObject(thread, 35000);
        CloseHandle(thread);
        return ERROR_TIMEOUT;
    }
    if (mode == L"--race-crash") {
        HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE threads[] = { CreateThread(nullptr, 0, crashThread, event, 0, nullptr), CreateThread(nullptr, 0, crashThread, event, 0, nullptr) };
        if (!event || !threads[0] || !threads[1]) return ERROR_NOT_ENOUGH_MEMORY;
        SetEvent(event);
        WaitForMultipleObjects(2, threads, TRUE, 35000);
        return ERROR_TIMEOUT;
    }
    HANDLE stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE resume = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!stopEvent || !resume) return ERROR_NOT_ENOUGH_MEMORY;
    PulseThread pulses[] = { { Hook, stopEvent, resume, mode == L"--hang-hook" },
        { Worker, stopEvent, resume, mode == L"--hang-worker" }, { Interception, stopEvent, resume, mode == L"--hang-interception" } };
    HANDLE threads[3] = {};
    for (unsigned i = 0; i < 3; ++i) {
        expectThread(pulses[i].slot);
        threads[i] = CreateThread(nullptr, 0, pulseThread, &pulses[i], 0, nullptr);
        if (!threads[i]) ExitProcess(ERROR_NOT_ENOUGH_MEMORY);
    }
    expectThread(Ui);
    beginHeartbeat(Ui);
    const ULONGLONG startTime = GetTickCount64();
    const DWORD duration = mode == L"--normal" ? 6000 : 50000;
    while (GetTickCount64() - startTime < duration) {
        if (GetTickCount64() - startTime >= 40000) SetEvent(resume);
        if (mode != L"--hang-ui" || GetTickCount64() - startTime >= 40000) heartbeat(Ui);
        Sleep(HeartbeatIntervalMs);
    }
    markPhase(Stopping);
    SetEvent(resume);
    SetEvent(stopEvent);
    WaitForMultipleObjects(3, threads, TRUE, 5000);
    for (HANDLE thread : threads) CloseHandle(thread);
    CloseHandle(stopEvent);
    CloseHandle(resume);
    return 0;
}
