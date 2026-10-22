#ifndef QKEYMAPPER_AUTOMATION_LOCAL_LISTENER_H
#define QKEYMAPPER_AUTOMATION_LOCAL_LISTENER_H

#include "qkm_ipc_frame.h"
#include "qkm_local_identity.h"
#include "qkm_auth_proof.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QWinEventNotifier>
#include <optional>

class QLocalSocket;

namespace QkmAutomation {

constexpr int PreauthTimeoutMs = 3000;
constexpr int AuthorizedFrameTimeoutMs = 10000;

class AutomationSessionServer;

enum class ListenerError {
    IdentityRejected, InvalidFrame, InvalidHandshake,
    Timeout, Disconnected, Rejected
};

// Owns one local pipe; a valid first message is not authentication or business authorization.
class LocalPipeListener : public QObject {
    Q_OBJECT
public:
    explicit LocalPipeListener(QObject *parent = nullptr);
    ~LocalPipeListener() override;
    bool start();
    void stop();
    void closeClient();
    bool isListening() const { return m_pipe != INVALID_HANDLE_VALUE; }
    QString pipeName() const { return m_pipeName; }
    QString instanceId() const { return m_instanceId; }
    QString connectionId() const { return m_connectionId; }

signals:
    void handshakeReceived(const QJsonObject &message, const QkmAutomation::ProcessIdentity &peer);
    void clientClosed(QkmAutomation::ListenerError reason);
    void failed();
    void messageReceived(const QString &connectionId, const QJsonObject &message);

private:
    friend class AutomationSessionServer;
    bool matchesConnection(const QString &connectionId) const;
    bool prepareMessage(const QString &connectionId, bool authorized, int timeoutMs, bool allowInput);
    bool sendMessage(const QString &connectionId, const QJsonObject &message);
    bool armAccept();
    void acceptClient();
    void readClient();
    void endClient(ListenerError reason);
    void finishClient(ListenerError reason);
    void tryFinishClient();
    void disposeSocket();
    void fail();

    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    HANDLE m_event = nullptr;
    OVERLAPPED m_accept{};
    bool m_acceptPending = false;
    QWinEventNotifier m_notifier;
    QTimer m_timeout;
    QElapsedTimer m_elapsed;
    QLocalSocket *m_socket = nullptr;
    FrameReader m_reader{MaxHandshakePayload};
    ProcessIdentity m_own;
    ProcessIdentity m_peer;
    QString m_pipeName;
    QString m_instanceId;
    bool m_received = false;
    bool m_disposingSocket = false;
    std::optional<ListenerError> m_closing;
    QJsonObject m_handshake;
    PipeRequestVerifier m_verifier;
    QString m_connectionId;
    bool m_firstMessage = true;
    bool m_frameStarted = true;
    int m_deadlineMs = PreauthTimeoutMs;
    int m_payloadLimit = MaxHandshakePayload;
};

} // namespace QkmAutomation
#endif
