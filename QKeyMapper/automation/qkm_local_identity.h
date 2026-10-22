#ifndef QKEYMAPPER_AUTOMATION_LOCAL_IDENTITY_H
#define QKEYMAPPER_AUTOMATION_LOCAL_IDENTITY_H

#include <QByteArray>
#include <QObject>
#include <QWinEventNotifier>
#include <QtCore/qt_windows.h>
#include <memory>

namespace QkmAutomation {

struct ProcessIdentity {
    quint32 processId = 0;
    quint64 creationTime = 0;
    quint32 sessionId = 0;
    quint64 authenticationId = 0;
    QByteArray userSid;
    QByteArray logonSid;
    bool isValid() const;
};

enum class IdentityError {
    None, ProcessUnavailable, TokenUnavailable, InvalidToken,
    PeerUnavailable, SessionMismatch, RequestUnavailable, RequestMismatch,
    WorkerContextNotEmpty, ContextRestoreFailed
};

struct IdentityResult {
    ProcessIdentity identity;
    IdentityError error = IdentityError::None;
    bool ok() const { return error == IdentityError::None && identity.isValid(); }
};

enum class PipeSide { Server, Client };

IdentityResult queryProcessIdentity(quint32 processId);
// Requires a local-only pipe with a logon-SID DACL; returns the peer process's primary-token identity.
IdentityResult queryPipePeerIdentity(HANDLE pipe, PipeSide side);
bool sameLogonSession(const ProcessIdentity &a, const ProcessIdentity &b);
bool sameProcess(const ProcessIdentity &a, const ProcessIdentity &b);

struct RequestIdentityState;

// Use on the owning Qt thread, after a pipe read; completion follows the native worker's exit.
class PipeRequestVerifier : public QObject {
    Q_OBJECT
public:
    explicit PipeRequestVerifier(QObject *parent = nullptr);
    ~PipeRequestVerifier() override;
    bool start(HANDLE pipe, const ProcessIdentity &expected);
    bool isRunning() const { return m_thread != nullptr; }

signals:
    void finished(const QkmAutomation::IdentityResult &result);

private:
    HANDLE m_thread = nullptr;
    QWinEventNotifier m_done;
    std::shared_ptr<RequestIdentityState> m_state;
};

#ifdef QKM_AUTOMATION_TESTING
// Test-target only: the gate is duplicated by start(), so its caller retains ownership.
void setRequestIdentityTestHooks(HANDLE gate, bool failRestore);
int requestIdentityTestWorkerCount();
#endif

} // namespace QkmAutomation
#endif
