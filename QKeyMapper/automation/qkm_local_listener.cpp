#include "qkm_local_listener.h"

#include <QLocalSocket>
#include <QScopeGuard>
#include <QUuid>
#include <sddl.h>

namespace QkmAutomation {
namespace {

bool hexString(const QJsonValue &value, int length)
{
    return value.isString() && isLowerHex(value.toString(), length);
}

bool validHello(const QJsonObject &message, const QString &instance)
{
    if (message.value(QStringLiteral("version")) != QJsonValue(HandshakeVersion)
        || message.value(QStringLiteral("instance")) != QJsonValue(instance)
        || !hexString(message.value(QStringLiteral("client_nonce")), 64)) return false;
    const QString type = message.value(QStringLiteral("type")).toString();
    QStringList fields{QStringLiteral("type"), QStringLiteral("version"),
                       QStringLiteral("instance"), QStringLiteral("client_nonce")};
    if (type == QStringLiteral("hello")) {
        fields.append(QStringLiteral("pair_id"));
        if (!hexString(message.value(QStringLiteral("pair_id")), 32)) return false;
    } else if (type == QStringLiteral("pair.request")) {
        fields.append(QStringLiteral("client_name"));
        const QJsonValue value = message.value(QStringLiteral("client_name"));
        if (!value.isString()) return false;
        if (!validClientName(value.toString())) return false;
    } else {
        return false;
    }
    if (message.size() != fields.size()) return false;
    for (const QString &field : fields) {
        if (!message.contains(field)) return false;
    }
    return true;
}

} // namespace

LocalPipeListener::LocalPipeListener(QObject *parent) : QObject(parent)
{
    m_timeout.setSingleShot(true);
    m_timeout.setTimerType(Qt::PreciseTimer);
    connect(&m_timeout, &QTimer::timeout, this, [this]() { endClient(ListenerError::Timeout); });
    connect(&m_notifier, &QWinEventNotifier::activated, this, [this]() { acceptClient(); });
    connect(&m_verifier, &PipeRequestVerifier::finished, this, [this](const IdentityResult &result) {
        if (!isListening()) return;
        if (result.error == IdentityError::ContextRestoreFailed
            || result.error == IdentityError::WorkerContextNotEmpty) {
            fail();
            return;
        }
        if (m_closing) {
            tryFinishClient();
            return;
        }
        if (!result.ok() || !sameProcess(result.identity, m_peer)) {
            endClient(ListenerError::IdentityRejected);
            return;
        }
        if (m_elapsed.hasExpired(m_deadlineMs)) {
            endClient(ListenerError::Timeout);
            return;
        }
        const IdentityResult current = queryPipePeerIdentity(m_pipe, PipeSide::Server);
        if (!current.ok() || !sameProcess(current.identity, m_peer)) {
            endClient(ListenerError::IdentityRejected);
            return;
        }
        if (!m_socket || m_socket->bytesAvailable()) {
            endClient(ListenerError::InvalidHandshake);
            return;
        }
        const QJsonObject message = m_handshake;
        m_handshake = {};
        const ProcessIdentity peer = m_peer;
        if (m_firstMessage) {
            m_firstMessage = false;
            emit handshakeReceived(message, peer);
        } else {
            const QString connection = m_connectionId;
            emit messageReceived(connection, message);
        }
    });
}

LocalPipeListener::~LocalPipeListener()
{
    stop();
}

bool LocalPipeListener::start()
{
    if (isListening()) return true;
    if (m_verifier.isRunning()) return false;
    const IdentityResult own = queryProcessIdentity(GetCurrentProcessId());
    if (!own.ok()) return false;
    LPWSTR sid = nullptr;
    if (!ConvertSidToStringSidW(const_cast<char *>(own.identity.logonSid.constData()), &sid))
        return false;
    const auto freeSid = qScopeGuard([sid]() { LocalFree(sid); });
    // Do not grant generic write: it also permits creating another server instance.
    const QString sddl = QStringLiteral("D:P(A;;0x0012019b;;;%1)").arg(QString::fromWCharArray(sid));
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            reinterpret_cast<LPCWSTR>(sddl.utf16()), SDDL_REVISION_1, &descriptor, nullptr))
        return false;
    const auto freeDescriptor = qScopeGuard([descriptor]() { LocalFree(descriptor); });
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), descriptor, FALSE};
    m_instanceId = QUuid::createUuid().toString(QUuid::Id128);
    m_pipeName = QString::fromLatin1(PipeNamePrefix) + m_instanceId;
    // shortcut: one client at a time; revisit only when bridge concurrency is required.
    m_pipe = CreateNamedPipeW(reinterpret_cast<LPCWSTR>(m_pipeName.utf16()),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        1, MaxHandshakePayload, MaxHandshakePayload, PreauthTimeoutMs, &attributes);
    if (m_pipe == INVALID_HANDLE_VALUE) {
        stop();
        return false;
    }
    m_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_event) {
        stop();
        return false;
    }
    m_own = own.identity;
    m_notifier.setHandle(m_event);
    if (!armAccept()) {
        stop();
        return false;
    }
    return true;
}

void LocalPipeListener::stop()
{
    m_timeout.stop();
    m_notifier.setEnabled(false);
    disposeSocket();
    if (m_acceptPending) {
        CancelIoEx(m_pipe, &m_accept);
        DWORD ignored = 0;
        // The OVERLAPPED and its event must outlive completion, including cancellation.
        GetOverlappedResult(m_pipe, &m_accept, &ignored, TRUE);
        m_acceptPending = false;
    }
    if (m_pipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(m_pipe);
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
    m_notifier.setHandle(nullptr);
    if (m_event) {
        CloseHandle(m_event);
        m_event = nullptr;
    }
    m_own = {};
    m_pipeName.clear();
    m_instanceId.clear();
    m_handshake = {};
    m_closing.reset();
    m_disposingSocket = false;
    m_connectionId.clear();
}

bool LocalPipeListener::armAccept()
{
    m_accept = {};
    m_accept.hEvent = m_event;
    if (!ResetEvent(m_event)) return false;
    const BOOL connected = ConnectNamedPipe(m_pipe, &m_accept);
    const DWORD error = connected ? ERROR_SUCCESS : GetLastError();
    if (!connected && error != ERROR_IO_PENDING && error != ERROR_PIPE_CONNECTED) return false;
    m_acceptPending = !connected && error == ERROR_IO_PENDING;
    if (!m_acceptPending && !SetEvent(m_event)) return false;
    m_notifier.setEnabled(true);
    return true;
}

void LocalPipeListener::acceptClient()
{
    m_notifier.setEnabled(false);
    if (!isListening() || m_socket) return;
    if (m_acceptPending) {
        DWORD ignored = 0;
        if (!GetOverlappedResult(m_pipe, &m_accept, &ignored, FALSE)) {
            fail();
            return;
        }
        m_acceptPending = false;
    }
    m_elapsed.start();
    const IdentityResult peer = queryPipePeerIdentity(m_pipe, PipeSide::Server);
    if (!peer.ok() || !sameLogonSession(peer.identity, m_own)) {
        endClient(ListenerError::IdentityRejected);
        return;
    }
    HANDLE duplicate = INVALID_HANDLE_VALUE;
    if (!DuplicateHandle(GetCurrentProcess(), m_pipe, GetCurrentProcess(), &duplicate,
                         0, FALSE, DUPLICATE_SAME_ACCESS)) {
        fail();
        return;
    }
    // Retain the original pipe across reconnects so its name cannot be taken over.
    m_socket = new QLocalSocket(this);
    m_socket->setReadBufferSize(MaxHandshakePayload + 4);
    m_peer = peer.identity;
    m_reader = FrameReader(MaxHandshakePayload);
    m_received = false;
    m_connectionId = QUuid::createUuid().toString(QUuid::Id128);
    m_firstMessage = true;
    m_frameStarted = true;
    m_deadlineMs = PreauthTimeoutMs;
    m_payloadLimit = MaxHandshakePayload;
    connect(m_socket, &QLocalSocket::readyRead, this, [this]() { readClient(); });
    connect(m_socket, &QLocalSocket::disconnected, this, [this]() {
        endClient(ListenerError::Disconnected);
    });
    if (!m_socket->setSocketDescriptor(reinterpret_cast<qintptr>(duplicate))) {
        CloseHandle(duplicate);
        fail();
        return;
    }
    const qint64 remaining = PreauthTimeoutMs - m_elapsed.elapsed();
    if (remaining <= 0) {
        endClient(ListenerError::Timeout);
        return;
    }
    m_timeout.start(static_cast<int>(remaining));
    readClient();
}

void LocalPipeListener::readClient()
{
    if (!m_socket) return;
    if (!m_frameStarted) {
        if (!m_socket->bytesAvailable()) return;
        m_frameStarted = true;
        m_elapsed.restart();
        m_timeout.start(m_deadlineMs);
    }
    if (m_elapsed.hasExpired(m_deadlineMs)) {
        endClient(ListenerError::Timeout);
        return;
    }
    if (m_received && m_socket->bytesAvailable()) {
        endClient(ListenerError::InvalidHandshake);
        return;
    }
    QJsonObject message;
    const FrameReadResult result = m_reader.read(*m_socket, message);
    if (result == FrameReadResult::Failed) {
        endClient(ListenerError::InvalidFrame);
        return;
    }
    if (result == FrameReadResult::Incomplete) return;
    if (m_firstMessage && !validHello(message, m_instanceId)) {
        endClient(ListenerError::InvalidHandshake);
        return;
    }
    const IdentityResult current = queryPipePeerIdentity(m_pipe, PipeSide::Server);
    if (!current.ok() || !sameProcess(current.identity, m_peer)) {
        endClient(ListenerError::IdentityRejected);
        return;
    }
    m_received = true;
    if (m_socket->bytesAvailable()) {
        endClient(ListenerError::InvalidHandshake);
        return;
    }
    m_handshake = message;
    if (!m_verifier.start(m_pipe, m_peer)) fail();
}

void LocalPipeListener::disposeSocket()
{
    if (!m_socket) return;
    QLocalSocket *socket = m_socket;
    m_socket = nullptr;
    socket->disconnect(this);
    socket->abort();
    socket->deleteLater();
    m_peer = {};
    m_reader = FrameReader(MaxHandshakePayload);
    m_received = false;
    m_connectionId.clear();
}

void LocalPipeListener::endClient(ListenerError reason)
{
    if (!isListening()) return;
    if (reason == ListenerError::Timeout && m_verifier.isRunning()) {
        // A stuck native check disables the endpoint; never spawn replacements or wait in Qt.
        fail();
        return;
    }
    if (m_closing) return;
    m_closing = reason;
    m_handshake = {};
    if (!m_verifier.isRunning()) m_timeout.stop();
    if (m_socket) {
        const QString instance = m_instanceId;
        QLocalSocket *socket = m_socket;
        m_disposingSocket = true;
        disposeSocket();
        // Qt5 emits disconnected before releasing its pipe; rearm only after destruction.
        connect(socket, &QObject::destroyed, this, [this, instance]() {
            if (isListening() && m_instanceId == instance) {
                m_disposingSocket = false;
                tryFinishClient();
            }
        }, Qt::QueuedConnection);
    }
    tryFinishClient();
}

void LocalPipeListener::tryFinishClient()
{
    if (!isListening() || !m_closing || m_disposingSocket || m_verifier.isRunning()) return;
    const ListenerError reason = *m_closing;
    m_closing.reset();
    m_timeout.stop();
    finishClient(reason);
}

void LocalPipeListener::finishClient(ListenerError reason)
{
    if (!DisconnectNamedPipe(m_pipe) && GetLastError() != ERROR_PIPE_NOT_CONNECTED) {
        fail();
        return;
    }
    if (!armAccept()) {
        fail();
        return;
    }
    emit clientClosed(reason);
}

void LocalPipeListener::closeClient()
{
    if (m_socket) endClient(ListenerError::Rejected);
}

void LocalPipeListener::fail()
{
    stop();
    emit failed();
}

bool LocalPipeListener::matchesConnection(const QString &connectionId) const
{
    if (!isListening() || !m_socket || m_closing || connectionId.isEmpty()
        || connectionId != m_connectionId || m_socket->state() != QLocalSocket::ConnectedState) return false;
    const IdentityResult current = queryPipePeerIdentity(m_pipe, PipeSide::Server);
    return current.ok() && sameProcess(current.identity, m_peer);
}

bool LocalPipeListener::prepareMessage(const QString &connectionId, bool authorized,
                                      int timeoutMs, bool allowInput)
{
    if (!matchesConnection(connectionId) || m_verifier.isRunning()) return false;
    if (m_frameStarted && m_elapsed.hasExpired(m_deadlineMs)) {
        endClient(ListenerError::Timeout);
        return false;
    }
    if (m_socket->bytesAvailable()) {
        endClient(ListenerError::InvalidHandshake);
        return false;
    }
    m_payloadLimit = authorized ? MaxFramePayload : MaxHandshakePayload;
    m_socket->setReadBufferSize(m_payloadLimit + 4);
    m_reader = FrameReader(m_payloadLimit);
    m_received = !allowInput;
    m_deadlineMs = authorized ? AuthorizedFrameTimeoutMs : timeoutMs;
    m_frameStarted = !authorized;
    m_timeout.stop();
    if (m_frameStarted) {
        m_elapsed.restart();
        m_timeout.start(m_deadlineMs);
    }
    return true;
}

bool LocalPipeListener::sendMessage(const QString &connectionId, const QJsonObject &message)
{
    if (!matchesConnection(connectionId)) return false;
    FrameError error;
    const QByteArray frame = encodeFrame(message, m_payloadLimit, error);
    if (error != FrameError::None || m_socket->bytesToWrite() + frame.size() > 2 * (m_payloadLimit + 4)
        || m_socket->write(frame) != frame.size()) {
        endClient(ListenerError::InvalidFrame);
        return false;
    }
    m_socket->flush();
    return true;
}

} // namespace QkmAutomation
