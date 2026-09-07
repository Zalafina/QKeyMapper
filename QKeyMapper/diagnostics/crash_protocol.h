#ifndef QKM_CRASH_PROTOCOL_H
#define QKM_CRASH_PROTOCOL_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstddef>
#include <climits>

namespace QkmDiagnostics {
constexpr DWORD ProtocolVersion = 1;
constexpr DWORD ProtocolMagic = 0x514b4d44;
constexpr DWORD HeartbeatIntervalMs = 2000;
constexpr DWORD DumpTimeoutMs = 15000;
constexpr DWORD CrashWaitMs = 30000;
constexpr DWORD ExitWaitMs = 2000;
constexpr DWORD DefaultHangSeconds = 30;
constexpr size_t PathCapacity = MAX_PATH;
enum Phase : LONG { Starting, Running, Stopping };
enum Slot : unsigned { Ui, Hook, Worker, Interception, SlotCount };
enum Kind : DWORD { Crash, Hang, Cleanup };
enum Status : DWORD { Complete, Failed, TimedOut, Unavailable };
enum FailureStage : DWORD {
    NoFailure, DirectorySelection, PathValidation, FileCreation, ExceptionValidation,
    DumpWriting, DumpFlush, DumpClose, DumpRename, MetadataWriting, CaptureCancellation, ReporterFailure
};

inline LONG readAtomic(volatile LONG &value) { return InterlockedCompareExchange(&value, 0, 0); }
inline ULONGLONG fileTimeValue(const FILETIME &value)
{ return (static_cast<ULONGLONG>(value.dwHighDateTime) << 32) | value.dwLowDateTime; }
inline ULONGLONG awakeMilliseconds()
{
    ULONGLONG time = 0;
    QueryUnbiasedInterruptTime(&time);
    return time / 10000;
}

struct alignas(64) Pulse {
    volatile LONG count;
    volatile LONG threadId;
};
struct CrashContext {
    DWORD threadId;
    BOOL valid;
    BOOL nestedRecordOmitted;
    EXCEPTION_RECORD record;
    CONTEXT context;
};
struct Result {
    Kind kind;
    Status status;
    FailureStage failureStage;
    DWORD error;
    DWORD directoryError;
    DWORD metadataError;
    BOOL fallback;
    WCHAR path[PathCapacity];
};
struct SharedState {
    DWORD magic;
    DWORD version;
    DWORD size;
    DWORD machine;
    DWORD processId;
    ULONGLONG processCreated;
    char sourceRevision[80];
    char builtAt[80];
    char qtVersion[32];
    char buildVariant[32];
    volatile LONG readyError;
    volatile LONG phase;
    volatile LONG progress;
    volatile LONG hangEnabled;
    volatile LONG hangSeconds;
    volatile LONG expectedThreads;
    volatile LONG crashOwner;
    volatile LONG crashPublished;
    Pulse pulses[SlotCount];
    CrashContext crash;
    volatile LONG resultSequence;
    Result result;
};
static_assert(sizeof(LONG) == 4, "Protocol counters must be 32 bit");
static_assert(offsetof(SharedState, pulses) % 64 == 0, "Heartbeats must be aligned");
inline DWORD machineType()
{
#ifdef _WIN64
    return IMAGE_FILE_MACHINE_AMD64;
#else
    return IMAGE_FILE_MACHINE_I386;
#endif
}
}
#endif
