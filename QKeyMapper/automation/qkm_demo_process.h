#ifndef QKM_DEMO_PROCESS_H
#define QKM_DEMO_PROCESS_H

#include "qkm_local_identity.h"
#include "qkm_private_storage.h"
#include <optional>

namespace QkmAutomation {

class DemoProcess {
public:
    DemoProcess() = default;
    ~DemoProcess();
    bool start();
    bool close();
    bool running() const;
    const ProcessIdentity &target() const { return m_target; }
    QString executable() const { return m_executable; }
    QString desktop() const { return m_desktopName; }
    HANDLE processHandle() const { return m_process; }
    QString failure() const { return m_failure; }
    std::optional<DWORD> nativeError() const { return m_nativeError; }
    std::optional<DWORD> cleanupError() const { return m_cleanupError; }
    std::optional<DWORD> exitCode() const { return m_exitCode; }

private:
    Q_DISABLE_COPY_MOVE(DemoProcess)
    bool fail(const char *code, std::optional<DWORD> error = {});
    bool prepared(const QString &directory);
    bool noLivePeer(const ProcessIdentity &own);
    bool abortStart();
    bool finishExit();
    bool releaseHandles();

    ProcessIdentity m_target;
    QString m_executable, m_desktopName, m_failure;
    HANDLE m_process = nullptr, m_thread = nullptr;
    HDESK m_desktop = nullptr;
    bool m_starting = false, m_closeRequested = false;
    std::optional<DWORD> m_nativeError, m_cleanupError, m_exitCode;
    Storage::DirectoryLease m_lease;
};

} // namespace QkmAutomation
#endif
