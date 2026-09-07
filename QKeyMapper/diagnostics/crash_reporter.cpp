#include "crash_files.h"
#include "hang_monitor.h"
#include <dbghelp.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <strsafe.h>
#include <cerrno>
#include <sstream>
#include <cstdlib>
#include <cwchar>
#include <winver.h>

using namespace QkmDiagnostics;
namespace {
struct Job {
    Kind kind = Hang;
    DWORD sequence = 0;
    LONG stalledMask = 0;
    Observation observation;
    ULONGLONG started = 0;
    ULONGLONG lastBeat[SlotCount] = {};
};
struct Reporter {
    HANDLE target = nullptr, crashEvent = nullptr, completed = nullptr, notification = nullptr, shutdownEvent = nullptr;
    HANDLE work = nullptr, done = nullptr;
    SharedState *state = nullptr;
    std::wstring directory;
    std::string applicationVersion;
    volatile LONG cancelReason = 0, quit = 0;
    Job job;
    Result outcome = {};
    Result prepared = {};
    volatile LONG preparedReady = 0;
};

void publish(Reporter &reporter, const Result &result)
{
    InterlockedIncrement(&reporter.state->resultSequence);
    reporter.state->result = result;
    InterlockedIncrement(&reporter.state->resultSequence);
    SetEvent(reporter.notification);
}
BOOL CALLBACK dumpCallback(PVOID parameter, const PMINIDUMP_CALLBACK_INPUT input, PMINIDUMP_CALLBACK_OUTPUT output)
{
    auto &reporter = *static_cast<Reporter *>(parameter);
    if (input->CallbackType == CancelCallback) {
        output->CheckCancel = TRUE;
        output->Cancel = readAtomic(reporter.cancelReason) != 0
            || awakeMilliseconds() - reporter.job.started >= DumpTimeoutMs
            || WaitForSingleObject(reporter.target, 0) == WAIT_OBJECT_0;
    }
    return TRUE;
}
std::string metadata(const Reporter &reporter, const Job &job, const std::wstring &stem,
    const Result &result, DWORD requestedFlags, DWORD actualFlags, DWORD firstError, bool finished)
{
    const SharedState &state = *reporter.state;
    std::ostringstream text;
    text << "QKM-DIAGNOSTICS-V1\ngroup=" << utf8(stem) << "\n"
         << "protocol=" << ProtocolVersion << "\nsource=" << state.sourceRevision
         << "\nbuilt_at=" << state.builtAt << "\nqt=" << state.qtVersion
         << "\nbuild_variant=" << state.buildVariant << "\napplication_version=" << reporter.applicationVersion
         << "\narchitecture=" << (state.machine == IMAGE_FILE_MACHINE_AMD64 ? "x64" : "x86")
         << "\npid=" << state.processId << "\nprocess_created=" << state.processCreated
         << "\nkind=" << (job.kind == Crash ? "Crash" : "SuspectedHang")
         << "\nphase=" << job.observation.phase << "\nprogress=" << job.observation.progress
         << "\nstalled_mask=" << job.stalledMask << "\nhang_timeout_seconds=" << job.observation.timeoutSeconds
         << "\nstatus=" << (finished ? (result.status == Complete ? "complete" : result.status == TimedOut ? "timeout" : "failed") : "in-progress")
         << "\nerror=" << result.error << "\nfailure_stage=" << result.failureStage
         << "\nlog_directory_creation_error=" << result.directoryError
         << "\nroot_fallback=" << result.fallback << "\nactual_path=" << utf8(result.path)
         << "\nmetadata_error=" << result.metadataError << "\ndump_flags_requested=" << requestedFlags
         << "\ndump_flags_actual=" << actualFlags << "\nfirst_dump_error=" << firstError
         << "\nelapsed_ms=" << awakeMilliseconds() - job.started << "\n";
    for (unsigned i = 0; i < SlotCount; ++i)
        text << "thread_" << i << "_id=" << job.observation.threadIds[i]
             << "\nthread_" << i << "_heartbeat=" << job.observation.counts[i]
             << "\nthread_" << i << "_last_progress_awake_ms=" << job.lastBeat[i] << "\n";
    if (job.kind == Crash) {
        text << "exception_thread=" << state.crash.threadId << "\nexception_context_valid=" << state.crash.valid
             << "\nnested_exception_omitted=" << state.crash.nestedRecordOmitted << "\n";
        if (state.crash.valid)
            text << "exception_code=0x" << std::hex << state.crash.record.ExceptionCode
                 << "\nexception_address=0x" << reinterpret_cast<ULONG_PTR>(state.crash.record.ExceptionAddress) << "\n";
    }
    return text.str();
}

Result capture(Reporter &reporter, const Job &job)
{
    Result result = {};
    result.kind = job.kind;
    result.status = Failed;
    Directory output;
    if (!selectDirectory(reporter.directory, output)) {
        result.error = GetLastError();
        result.failureStage = DirectorySelection;
        result.directoryError = output.creationError;
        return result;
    }
    result.directoryError = output.creationError;
    result.fallback = output.fallback;
    SYSTEMTIME time;
    GetSystemTime(&time);
    WCHAR name[160] = {};
    StringCchPrintfW(name, 160, L"QKM_Diag_v1_%04u%02u%02u_%02u%02u%02u_%03u_%lu_%016llX_%lu_%s",
        time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds,
        reporter.state->processId, reporter.state->processCreated, job.sequence, job.kind == Crash ? L"Crash" : L"Hang");
    const std::wstring stem(name), base = childPath(output.path, stem);
    const std::wstring partial = base + L".partial", final = base + L".dmp", companion = base + L".txt";
    if (!supportedPath(partial)) { result.error = GetLastError(); result.failureStage = PathValidation; return result; }
    StringCchCopyW(result.path, PathCapacity, partial.c_str());
    reporter.prepared = result;
    InterlockedExchange(&reporter.preparedReady, 1);
    const DWORD requestedFlags = MiniDumpNormal | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules;
    DWORD actualFlags = requestedFlags, firstError = 0;
    FileHandle recordGuard(CreateFileW(companion.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    HANDLE record = recordGuard.value;
    if (record == INVALID_HANDLE_VALUE) {
        result.metadataError = GetLastError();
        if (result.metadataError == ERROR_FILE_EXISTS || result.metadataError == ERROR_ALREADY_EXISTS) {
            result.error = result.metadataError;
            result.failureStage = MetadataWriting;
            return result;
        }
    } else {
        result.metadataError = writeText(record, metadata(reporter, job, stem, result, requestedFlags, actualFlags, 0, false));
    }
    FileHandle dumpGuard(CreateFileW(partial.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    HANDLE dump = dumpGuard.value;
    if (dump == INVALID_HANDLE_VALUE) {
        result.error = GetLastError();
        result.failureStage = FileCreation;
    } else {
        CrashContext crash = {};
        EXCEPTION_POINTERS pointers = {};
        MINIDUMP_EXCEPTION_INFORMATION exception = {};
        bool validThread = true;
        DWORD threadError = ERROR_SUCCESS;
        if (job.kind == Crash) {
            crash = reporter.state->crash;
            // Never follow a pointer from the target's exception chain in this process.
            crash.record.ExceptionRecord = nullptr;
            HANDLE thread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, crash.threadId);
            if (!thread) threadError = GetLastError();
            else {
                const DWORD owner = GetProcessIdOfThread(thread);
                threadError = !owner ? GetLastError() : owner != reporter.state->processId ? ERROR_INVALID_THREAD_ID : ERROR_SUCCESS;
            }
            validThread = threadError == ERROR_SUCCESS;
            if (crash.valid && crash.record.NumberParameters > EXCEPTION_MAXIMUM_PARAMETERS) {
                validThread = false;
                threadError = ERROR_INVALID_DATA;
            }
            if (thread) CloseHandle(thread);
            pointers.ExceptionRecord = &crash.record;
            pointers.ContextRecord = &crash.context;
            exception.ThreadId = crash.threadId;
            exception.ExceptionPointers = &pointers;
            exception.ClientPointers = FALSE;
        }
        MINIDUMP_CALLBACK_INFORMATION callback = { dumpCallback, &reporter };
        BOOL written = FALSE;
        if (validThread) {
            written = MiniDumpWriteDump(reporter.target, reporter.state->processId, dump,
                static_cast<MINIDUMP_TYPE>(actualFlags), job.kind == Crash && crash.valid ? &exception : nullptr, nullptr, &callback);
            result.error = written ? ERROR_SUCCESS : GetLastError();
            result.failureStage = written ? NoFailure : DumpWriting;
            if (!written && result.error == static_cast<DWORD>(E_INVALIDARG) && !readAtomic(reporter.cancelReason)
                && awakeMilliseconds() - job.started < DumpTimeoutMs) {
                firstError = result.error;
                LARGE_INTEGER start = {};
                if (SetFilePointerEx(dump, start, nullptr, FILE_BEGIN) && SetEndOfFile(dump)) {
                    actualFlags = MiniDumpNormal;
                    written = MiniDumpWriteDump(reporter.target, reporter.state->processId, dump, MiniDumpNormal,
                        job.kind == Crash && crash.valid ? &exception : nullptr, nullptr, &callback);
                    result.error = written ? ERROR_SUCCESS : GetLastError();
                } else result.error = GetLastError();
            }
        } else { result.error = threadError; result.failureStage = ExceptionValidation; }
        if (written && !FlushFileBuffers(dump)) { written = FALSE; result.error = GetLastError(); result.failureStage = DumpFlush; }
        if (!dumpGuard.close() && written) { written = FALSE; result.error = GetLastError(); result.failureStage = DumpClose; }
        if (written && !readAtomic(reporter.cancelReason) && awakeMilliseconds() - job.started < DumpTimeoutMs) {
            if (MoveFileExW(partial.c_str(), final.c_str(), 0)) {
                result.status = Complete;
                result.failureStage = NoFailure;
                StringCchCopyW(result.path, PathCapacity, final.c_str());
            } else { result.error = GetLastError(); result.failureStage = DumpRename; }
        } else if (readAtomic(reporter.cancelReason) == ERROR_TIMEOUT || awakeMilliseconds() - job.started >= DumpTimeoutMs) {
            result.status = TimedOut;
            result.failureStage = CaptureCancellation;
            if (!result.error) result.error = ERROR_TIMEOUT;
        } else if (!result.error) { result.error = ERROR_CANCELLED; result.failureStage = CaptureCancellation; }
    }
    if (record != INVALID_HANDLE_VALUE) {
        const DWORD error = writeText(record, metadata(reporter, job, stem, result, requestedFlags, actualFlags, firstError, true));
        if (error) result.metadataError = error;
        if (!recordGuard.close() && !result.metadataError) result.metadataError = GetLastError();
    }
    return result;
}

DWORD WINAPI writerThread(void *parameter)
{
    auto &reporter = *static_cast<Reporter *>(parameter);
    for (;;) {
        if (WaitForSingleObject(reporter.work, INFINITE) != WAIT_OBJECT_0 || readAtomic(reporter.quit)) return 0;
        try {
            if (reporter.job.kind == Cleanup) {
                reporter.outcome = {};
                reporter.outcome.kind = Cleanup;
                reporter.outcome.error = cleanupFiles(reporter.directory, &reporter.cancelReason);
                reporter.outcome.status = reporter.outcome.error ? Failed : Complete;
            } else reporter.outcome = capture(reporter, reporter.job);
        } catch (...) {
            reporter.outcome = {};
            reporter.outcome.kind = reporter.job.kind;
            reporter.outcome.status = Failed;
            reporter.outcome.error = ERROR_NOT_ENOUGH_MEMORY;
            reporter.outcome.failureStage = ReporterFailure;
        }
        SetEvent(reporter.done);
    }
}

bool isParent(HANDLE target)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    bool matches = false;
    if (Process32FirstW(snapshot, &entry)) do {
        if (entry.th32ProcessID == GetCurrentProcessId()) {
            matches = entry.th32ParentProcessID == GetProcessId(target);
            break;
        }
    } while (Process32NextW(snapshot, &entry));
    CloseHandle(snapshot);
    return matches;
}

int runReporter(HANDLE *handles)
{
    // Process lifetime storage also survives a supervisor allocation failure.
    static Reporter reporter;
    reporter.target = handles[0];
    reporter.crashEvent = handles[2];
    reporter.completed = handles[3];
    reporter.notification = handles[5];
    reporter.shutdownEvent = handles[6];
    reporter.state = static_cast<SharedState *>(MapViewOfFile(handles[1], FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedState)));
    if (!reporter.state) return ERROR_INVALID_DATA;
    SharedState &state = *reporter.state;
    auto failReady = [&](DWORD error) {
        InterlockedExchange(&state.readyError, static_cast<LONG>(error));
        SetEvent(handles[4]);
        return static_cast<int>(error);
    };
    FILETIME created, exited, kernel, user;
    if (state.magic != ProtocolMagic || state.version != ProtocolVersion || state.size != sizeof(SharedState)
        || state.machine != machineType() || state.processId != GetProcessId(reporter.target)
        || !GetProcessTimes(reporter.target, &created, &exited, &kernel, &user)
        || state.processCreated != fileTimeValue(created) || !isParent(reporter.target)
        || state.sourceRevision[79] || state.builtAt[79] || state.qtVersion[31] || state.buildVariant[31])
        return failReady(ERROR_INVALID_DATA);
    WCHAR executable[PathCapacity] = {};
    DWORD length = PathCapacity;
    if (!QueryFullProcessImageNameW(reporter.target, 0, executable, &length)) return failReady(GetLastError());
    if (length >= PathCapacity) return failReady(ERROR_FILENAME_EXCED_RANGE);
    DWORD ignored = 0;
    const DWORD versionSize = GetFileVersionInfoSizeW(executable, &ignored);
    if (versionSize) {
        std::vector<BYTE> version(versionSize);
        if (GetFileVersionInfoW(executable, 0, versionSize, version.data())) {
            WCHAR *productVersion = nullptr;
            UINT size = 0;
            if (VerQueryValueW(version.data(), L"\\StringFileInfo\\040904b0\\ProductVersion",
                    reinterpret_cast<void **>(&productVersion), &size)
                && productVersion && size > 1 && size <= version.size() / sizeof(WCHAR)) {
                const WCHAR *end = std::wmemchr(productVersion, L'\0', size);
                if (end && end != productVersion)
                    reporter.applicationVersion = utf8(std::wstring(productVersion, static_cast<size_t>(end - productVersion)));
            }
            // Keep the fixed numeric version as a fallback for missing or invalid strings.
            VS_FIXEDFILEINFO *info = nullptr;
            if (reporter.applicationVersion.empty()
                && VerQueryValueW(version.data(), L"\\", reinterpret_cast<void **>(&info), &size)
                && info && size >= sizeof(*info) && info->dwSignature == 0xFEEF04BD) {
                reporter.applicationVersion = std::to_string(HIWORD(info->dwProductVersionMS)) + "."
                    + std::to_string(LOWORD(info->dwProductVersionMS)) + "." + std::to_string(HIWORD(info->dwProductVersionLS))
                    + "." + std::to_string(LOWORD(info->dwProductVersionLS));
            }
        }
    }
    if (reporter.applicationVersion.empty()) reporter.applicationVersion = "unknown";
    WCHAR *slash = wcsrchr(executable, L'\\');
    if (!slash) return failReady(ERROR_BAD_PATHNAME);
    if (slash == executable + 2 && executable[1] == L':') slash[1] = 0;
    else *slash = 0;
    reporter.directory = executable;
    WCHAR ownPath[PathCapacity] = {};
    const DWORD ownLength = GetModuleFileNameW(nullptr, ownPath, PathCapacity);
    if (!ownLength) return failReady(GetLastError());
    if (ownLength >= PathCapacity) return failReady(ERROR_FILENAME_EXCED_RANGE);
    slash = wcsrchr(ownPath, L'\\');
    if (!slash) return failReady(ERROR_BAD_PATHNAME);
    if (slash == ownPath + 2 && ownPath[1] == L':') slash[1] = 0;
    else *slash = 0;
    if (_wcsicmp(ownPath, executable)) return failReady(ERROR_BAD_PATHNAME);
    reporter.work = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!reporter.work) return failReady(GetLastError());
    reporter.done = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!reporter.done) return failReady(GetLastError());
    HANDLE writer = CreateThread(nullptr, 0, writerThread, &reporter, 0, nullptr);
    if (!writer) return failReady(GetLastError());
    InterlockedExchange(&state.readyError, 0);
    SetEvent(handles[4]);

    HangMonitor monitor;
    bool busy = false, pendingCrash = false, stopping = false, targetDead = false;
    bool cleanupScheduled = false, hangDisabled = false, timeoutPublished = false;
    ULONGLONG shutdownAt = 0, crashAt = 0, previousAwake = awakeMilliseconds(), previousTick = GetTickCount64();
    DWORD sequence = 0;
    auto queue = [&](Kind kind, LONG stalledMask) {
        reporter.job = {};
        reporter.job.kind = kind;
        reporter.job.sequence = ++sequence;
        reporter.job.stalledMask = stalledMask;
        reporter.job.started = awakeMilliseconds();
        reporter.job.observation = observe(state);
        for (unsigned i = 0; i < SlotCount; ++i) reporter.job.lastBeat[i] = monitor.lastHeartbeat(i);
        InterlockedExchange(&reporter.cancelReason, 0);
        InterlockedExchange(&reporter.preparedReady, 0);
        busy = true;
        timeoutPublished = false;
        SetEvent(reporter.work);
    };
    for (;;) {
        // Signaled lifetime events are removed after receipt to avoid a busy loop.
        HANDLE waits[4];
        unsigned waitCount = 0;
        if (!targetDead) waits[waitCount++] = reporter.target;
        if (!stopping) waits[waitCount++] = reporter.shutdownEvent;
        if (!pendingCrash) waits[waitCount++] = reporter.crashEvent;
        waits[waitCount++] = reporter.done;
        const DWORD wait = WaitForMultipleObjects(waitCount, waits, FALSE,
            !busy && cleanupScheduled && !readAtomic(state.hangEnabled) ? INFINITE : HeartbeatIntervalMs);
        const ULONGLONG now = awakeMilliseconds();
        if (wait >= WAIT_OBJECT_0 && wait < WAIT_OBJECT_0 + waitCount) {
            HANDLE event = waits[wait - WAIT_OBJECT_0];
            if (event == reporter.target) { targetDead = stopping = true; shutdownAt = now; }
            else if (event == reporter.shutdownEvent) { stopping = true; shutdownAt = now; }
            else if (event == reporter.crashEvent) {
                if (readAtomic(state.crashPublished)) { pendingCrash = true; crashAt = now; }
                else ResetEvent(reporter.crashEvent);
            } else if (event == reporter.done) {
                busy = false;
                if (reporter.job.kind != Cleanup || reporter.outcome.error) publish(reporter, reporter.outcome);
                if (reporter.job.kind == Crash) { SetEvent(reporter.completed); break; }
                monitor.reset(observe(state), now);
            }
        } else if (wait == WAIT_FAILED) {
            stopping = true;
            if (!shutdownAt) shutdownAt = now;
        }
        if (busy) {
            if (pendingCrash && reporter.job.kind != Crash) InterlockedExchange(&reporter.cancelReason, ERROR_CANCELLED);
            if (stopping) InterlockedExchange(&reporter.cancelReason, ERROR_CANCELLED);
            if (now - reporter.job.started >= DumpTimeoutMs) {
                InterlockedExchange(&reporter.cancelReason, ERROR_TIMEOUT);
                hangDisabled = true;
                InterlockedExchange(&state.hangEnabled, 0);
                if (!timeoutPublished) {
                    Result result = readAtomic(reporter.preparedReady) ? reporter.prepared : Result{};
                    result.kind = reporter.job.kind;
                    result.status = TimedOut;
                    result.error = ERROR_TIMEOUT;
                    result.failureStage = CaptureCancellation;
                    publish(reporter, result);
                    timeoutPublished = true;
                }
            }
            if ((targetDead && now - shutdownAt >= ExitWaitMs)
                || (pendingCrash && now - crashAt >= CrashWaitMs - ExitWaitMs)) {
                // Only abandon a writer when the target is dead or irrevocably crashing.
                SetEvent(reporter.completed);
                TerminateProcess(GetCurrentProcess(), ERROR_TIMEOUT);
            }
            continue;
        }
        if (stopping) break;
        if (pendingCrash) { queue(Crash, 0); continue; }
        const ULONGLONG tick = GetTickCount64();
        const bool resumed = tick - previousTick > now - previousAwake + 1000;
        previousTick = tick;
        previousAwake = now;
        BOOL debugging = FALSE;
        const bool debuggerKnown = CheckRemoteDebuggerPresent(reporter.target, &debugging) != FALSE;
        const Observation observation = observe(state);
        LONG stalled = 0;
        const bool paused = resumed || !debuggerKnown || debugging || !readAtomic(state.hangEnabled) || hangDisabled;
        if (monitor.sample(observation, now, paused, stalled)) { queue(Hang, stalled); continue; }
        if (!cleanupScheduled && observation.phase == Running) { cleanupScheduled = true; queue(Cleanup, 0); }
    }
    InterlockedExchange(&reporter.quit, 1);
    SetEvent(reporter.work);
    if (WaitForSingleObject(writer, ExitWaitMs) != WAIT_OBJECT_0) ExitProcess(ERROR_TIMEOUT);
    CloseHandle(writer);
    CloseHandle(reporter.work);
    CloseHandle(reporter.done);
    UnmapViewOfFile(reporter.state);
    return 0;
}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int count = 0;
    WCHAR **arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!arguments || count != 8) return ERROR_INVALID_PARAMETER;
    HANDLE handles[7] = {};
    for (int i = 0; i < 7; ++i) {
        WCHAR *end = nullptr;
        errno = 0;
        const unsigned long long value = wcstoull(arguments[i + 1], &end, 10);
        if (!value || !end || *end || arguments[i + 1][0] == L'-' || errno == ERANGE
            || value > static_cast<unsigned long long>(~static_cast<ULONG_PTR>(0))) return ERROR_INVALID_PARAMETER;
        handles[i] = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(value));
        DWORD flags = 0;
        if (!GetHandleInformation(handles[i], &flags)) return ERROR_INVALID_HANDLE;
        SetHandleInformation(handles[i], HANDLE_FLAG_INHERIT, 0);
    }
    LocalFree(arguments);
    int result;
    try { result = runReporter(handles); }
    catch (...) { result = ERROR_NOT_ENOUGH_MEMORY; }
    for (HANDLE handle : handles) CloseHandle(handle);
    // Worker state must never unwind underneath a writer after an unexpected supervisor failure.
    ExitProcess(static_cast<UINT>(result));
}
