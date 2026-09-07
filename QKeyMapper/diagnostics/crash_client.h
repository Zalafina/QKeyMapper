#ifndef QKM_CRASH_CLIENT_H
#define QKM_CRASH_CLIENT_H
#include "crash_protocol.h"

namespace QkmDiagnostics {
bool start(const char *qtVersion, const char *variant);
void stop();
DWORD startupError();
const WCHAR *applicationDirectory();
HANDLE reporterHandle();
HANDLE notificationHandle();
bool readResult(Result &result);
void configureHang(bool enabled, DWORD seconds);
bool hangEnabled();
void markPhase(Phase phase);
void markProgress();
void expectThread(Slot slot);
void beginHeartbeat(Slot slot);
void heartbeat(Slot slot);
void disableHang();
struct Session {
    Session(const char *qtVersion, const char *variant) { start(qtVersion, variant); }
    ~Session() { stop(); }
    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;
};
}
#endif
