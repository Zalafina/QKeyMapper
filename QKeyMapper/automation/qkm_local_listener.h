#ifndef QKEYMAPPER_AUTOMATION_LOCAL_LISTENER_H
#define QKEYMAPPER_AUTOMATION_LOCAL_LISTENER_H

#include "qkm_ipc_frame.h"
#include "qkm_local_identity.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QWinEventNotifier>

class QLocalSocket;

namespace QkmAutomation {

constexpr int PreauthTimeoutMs = 3000;
constexpr int HandshakeVersion = 1;

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

signals:
    void handshakeReceived(const QJsonObject &message, const QkmAutomation::ProcessIdentity &peer);
    void clientClosed(QkmAutomation::ListenerError reason);
    void failed();

private:
    bool armAccept();
    void acceptClient();
    void readClient();
    void endClient(ListenerError reason);
    void finishClient(ListenerError reason);
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
};

} // namespace QkmAutomation
#endif
