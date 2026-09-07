#include "crash_client.h"
#include <strsafe.h>
#include <vector>
#include "qkm_build_identity.h"

namespace QkmDiagnostics {
namespace {
SharedState *state = nullptr;
HANDLE mapping = nullptr, request = nullptr, completed = nullptr, ready = nullptr;
HANDLE notification = nullptr, shutdownEvent = nullptr, reporter = nullptr;
WCHAR directory[PathCapacity] = {};
DWORD initializationError = ERROR_NOT_READY;
LPTOP_LEVEL_EXCEPTION_FILTER previousFilter = nullptr;
// The high bit closes admission before resources can be released.
volatile LONG filterUsers = LONG_MIN;

LONG WINAPI exceptionFilter(EXCEPTION_POINTERS *exception)
{
    LONG users = readAtomic(filterUsers);
    for (;;) {
        if (users < 0) return EXCEPTION_CONTINUE_SEARCH;
        const LONG observed = InterlockedCompareExchange(&filterUsers, users + 1, users);
        if (observed == users) break;
        users = observed;
    }
    const LONG threadId = static_cast<LONG>(GetCurrentThreadId());
    const LONG owner = InterlockedCompareExchange(&state->crashOwner, threadId, 0);
    if (owner == threadId) {
        InterlockedDecrement(&filterUsers);
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (!owner) {
        state->crash.threadId = static_cast<DWORD>(threadId);
        __try {
            if (exception && exception->ExceptionRecord && exception->ContextRecord) {
                state->crash.record = *exception->ExceptionRecord;
                state->crash.context = *exception->ContextRecord;
                state->crash.nestedRecordOmitted = state->crash.record.ExceptionRecord != nullptr;
                state->crash.record.ExceptionRecord = nullptr;
                state->crash.valid = TRUE;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            state->crash.valid = FALSE;
        }
        InterlockedExchange(&state->crashPublished, 1);
        SetEvent(request);
    }
    HANDLE waits[] = { completed, reporter };
    WaitForMultipleObjects(2, waits, FALSE, CrashWaitMs);
    InterlockedDecrement(&filterUsers);
    return EXCEPTION_CONTINUE_SEARCH;
}

void closeResources()
{
    if (state) { UnmapViewOfFile(state); state = nullptr; }
    for (HANDLE *handle : { &mapping, &request, &completed, &ready, &notification, &shutdownEvent, &reporter }) {
        if (*handle) { CloseHandle(*handle); *handle = nullptr; }
    }
}
}

bool start(const char *qtVersion, const char *variant)
{
    WCHAR executable[PathCapacity] = {};
    const DWORD length = GetModuleFileNameW(nullptr, executable, static_cast<DWORD>(PathCapacity));
    if (!length || length >= PathCapacity) {
        initializationError = length ? ERROR_FILENAME_EXCED_RANGE : GetLastError();
        return false;
    }
    WCHAR *slash = wcsrchr(executable, L'\\');
    if (!slash) { initializationError = ERROR_BAD_PATHNAME; return false; }
    if (slash == executable + 2 && executable[1] == L':') slash[1] = 0;
    else *slash = 0;
    StringCchCopyW(directory, PathCapacity, executable);
    WCHAR helperPath[PathCapacity] = {};
    if (FAILED(StringCchPrintfW(helperPath, PathCapacity, L"%s%sQKeyMapperCrashReporter.exe", directory,
                               directory[wcslen(directory) - 1] == L'\\' ? L"" : L"\\"))) {
        initializationError = ERROR_FILENAME_EXCED_RANGE;
        return false;
    }
    SECURITY_ATTRIBUTES security = { sizeof(security), nullptr, TRUE };
    HANDLE target = nullptr;
    auto fail = [&]() {
        initializationError = GetLastError();
        if (!initializationError) initializationError = ERROR_GEN_FAILURE;
        if (target) CloseHandle(target);
        closeResources();
        return false;
    };
    mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, &security, PAGE_READWRITE, 0, sizeof(SharedState), nullptr);
    if (!mapping) return fail();
    state = static_cast<SharedState *>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedState)));
    if (!state) return fail();
    request = CreateEventW(&security, TRUE, FALSE, nullptr);
    if (!request) return fail();
    completed = CreateEventW(&security, TRUE, FALSE, nullptr);
    if (!completed) return fail();
    ready = CreateEventW(&security, TRUE, FALSE, nullptr);
    if (!ready) return fail();
    notification = CreateEventW(&security, FALSE, FALSE, nullptr);
    if (!notification) return fail();
    shutdownEvent = CreateEventW(&security, TRUE, FALSE, nullptr);
    if (!shutdownEvent) return fail();
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &target,
                         PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, TRUE, 0)) return fail();
    FILETIME created, exited, kernel, user;
    if (!GetProcessTimes(target, &created, &exited, &kernel, &user)) return fail();
    state->magic = ProtocolMagic;
    state->version = ProtocolVersion;
    state->size = sizeof(SharedState);
    state->machine = machineType();
    state->processId = GetCurrentProcessId();
    state->processCreated = fileTimeValue(created);
    state->hangEnabled = 1;
    state->hangSeconds = DefaultHangSeconds;
    state->readyError = ERROR_NOT_READY;
    StringCchCopyA(state->sourceRevision, 80, QKM_SOURCE_REVISION);
    StringCchCopyA(state->builtAt, 80, __DATE__ " " __TIME__);
    StringCchCopyA(state->qtVersion, 32, qtVersion);
    StringCchCopyA(state->buildVariant, 32, variant);
    HANDLE inherited[] = { target, mapping, request, completed, ready, notification, shutdownEvent };
    SIZE_T attributeBytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
    if (!attributeBytes) return fail();
    std::vector<BYTE> attributeStorage;
    try { attributeStorage.resize(attributeBytes); }
    catch (...) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return fail(); }
    auto attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());
    if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attributeBytes)) return fail();
    if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                   inherited, sizeof(inherited), nullptr, nullptr)) {
        const DWORD error = GetLastError();
        DeleteProcThreadAttributeList(attributes);
        SetLastError(error);
        return fail();
    }
    WCHAR command[1024] = {};
    StringCchPrintfW(command, 1024, L"\"%s\" %llu %llu %llu %llu %llu %llu %llu", helperPath,
        static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(target)),
        static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(mapping)),
        static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(request)),
        static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(completed)),
        static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(ready)),
        static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(notification)),
        static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(shutdownEvent)));
    STARTUPINFOEXW startup = {};
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = attributes;
    PROCESS_INFORMATION process = {};
    // Called before Qt/business threads start; the child inherits this mode even before its entry point.
    const UINT previousErrorMode = GetErrorMode();
    SetErrorMode(previousErrorMode | SEM_FAILCRITICALERRORS);
    const BOOL launched = CreateProcessW(helperPath, command, nullptr, nullptr, TRUE,
        EXTENDED_STARTUPINFO_PRESENT | NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
        nullptr, directory, &startup.StartupInfo, &process);
    const DWORD launchError = GetLastError();
    SetErrorMode(previousErrorMode);
    DeleteProcThreadAttributeList(attributes);
    CloseHandle(target);
    target = nullptr;
    for (HANDLE handle : { mapping, request, completed, ready, notification, shutdownEvent })
        SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0);
    if (!launched) { SetLastError(launchError); return fail(); }
    reporter = process.hProcess;
    CloseHandle(process.hThread);
    HANDLE waits[] = { ready, reporter };
    const DWORD wait = WaitForMultipleObjects(2, waits, FALSE, 1000);
    if (wait != WAIT_OBJECT_0 || readAtomic(state->readyError)) {
        DWORD error = wait == WAIT_OBJECT_0 ? static_cast<DWORD>(readAtomic(state->readyError))
            : wait == WAIT_FAILED ? GetLastError() : wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : ERROR_PROCESS_ABORTED;
        if (wait == WAIT_OBJECT_0 + 1) {
            if (!GetExitCodeProcess(reporter, &error)) error = GetLastError();
            else if (!error) error = ERROR_PROCESS_ABORTED;
        }
        // No request was admitted, so the child cannot be suspending this process.
        SetEvent(shutdownEvent);
        if (wait != WAIT_OBJECT_0 + 1) TerminateProcess(reporter, error);
        SetLastError(error);
        return fail();
    }
    initializationError = ERROR_SUCCESS;
    InterlockedExchange(&filterUsers, 0);
    previousFilter = SetUnhandledExceptionFilter(exceptionFilter);
    return true;
}

void stop()
{
    if (!state) return;
    // ponytail: retain resources until process exit if a crash already owns them; never close a handle under its waiter.
    if (InterlockedCompareExchange(&filterUsers, LONG_MIN, 0) != 0) return;
    SetUnhandledExceptionFilter(previousFilter);
    SetEvent(shutdownEvent);
    WaitForSingleObject(reporter, ExitWaitMs);
    closeResources();
}
DWORD startupError() { return initializationError; }
const WCHAR *applicationDirectory() { return directory; }
HANDLE reporterHandle() { return reporter; }
HANDLE notificationHandle() { return notification; }
bool readResult(Result &result)
{
    if (!state) return false;
    const LONG before = readAtomic(state->resultSequence);
    if (!before || (before & 1)) return false;
    result = state->result;
    MemoryBarrier();
    return before == readAtomic(state->resultSequence);
}
void configureHang(bool enabled, DWORD seconds)
{
    if (!state) return;
    if (seconds < 10 || seconds > 600) seconds = DefaultHangSeconds;
    InterlockedExchange(&state->hangSeconds, static_cast<LONG>(seconds));
    InterlockedExchange(&state->hangEnabled, enabled ? 1 : 0);
}
bool hangEnabled() { return state && readAtomic(state->hangEnabled); }
void markPhase(Phase phase)
{
    if (state) { InterlockedExchange(&state->phase, phase); markProgress(); }
}
void markProgress() { if (state) InterlockedIncrement(&state->progress); }
void expectThread(Slot slot) { if (state) InterlockedOr(&state->expectedThreads, 1L << slot); }
void beginHeartbeat(Slot slot)
{
    if (!state) return;
    InterlockedExchange(&state->pulses[slot].threadId, static_cast<LONG>(GetCurrentThreadId()));
    heartbeat(slot);
}
void heartbeat(Slot slot) { if (state) InterlockedIncrement(&state->pulses[slot].count); }
void disableHang() { if (state) InterlockedExchange(&state->hangEnabled, 0); }
}
