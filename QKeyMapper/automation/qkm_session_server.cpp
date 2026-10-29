#include "qkm_session_server.h"
#include "qkm_demo_mode.h"
#include "qkm_private_storage.h"

#include <QUuid>
#include <QTimer>
#include <QJsonArray>
#include <QScopeGuard>
#include <limits>
#include <utility>

namespace QkmAutomation {
namespace {

quint32 minimumScopes(const QString &method)
{
    if (method == QStringLiteral("qkm_open_mapping_editor") || method == QStringLiteral("qkm_close_mapping_editor")
        || method == QStringLiteral("qkm_open_mapping_settings") || method == QStringLiteral("qkm_close_mapping_settings"))
        return Read | ShowWindows;
    if (method == QStringLiteral("qkm_capture_window")) return Read | CaptureWindows;
    if (method == QStringLiteral("qkm_preview_changes") || method == QStringLiteral("qkm_apply_changes")
        || method == QStringLiteral("qkm_cancel_changes"))
        return Read | Configure;
    if (method == QStringLiteral("qkm_start_mapping") || method == QStringLiteral("qkm_stop_mapping"))
        return Read | RunMapping;
    for (const char *name : {"qkm_get_capabilities", "qkm_get_state", "qkm_query", "qkm_get_help",
                            "qkm_get_operation", "qkm_wait_state"}) {
        if (method == QLatin1String(name)) return Read;
    }
    return 0;
}

void eraseSecret(QByteArray &secret)
{
    if (!secret.isEmpty()) SecureZeroMemory(secret.data(), static_cast<SIZE_T>(secret.size()));
    secret.clear();
}

PairingInfo info(const PairCredential &pair)
{
    return {pair.pairId, pair.clientName, pair.permissionEpoch, pair.scopes};
}

PairStoreError leaseError(DWORD error)
{
    return error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION || error == ERROR_BUSY
        ? PairStoreError::Busy : PairStoreError::UnsafeStorage;
}

void eraseCredentials(QList<PairCredential> &credentials)
{
    for (PairCredential &credential : credentials) eraseSecret(credential.secret);
    credentials.clear();
}

} // namespace

AutomationSessionServer::AutomationSessionServer(QString storeDirectory, QByteArray applicationBinding,
                                                 QObject *parent)
    : QObject(parent), m_directory(std::move(storeDirectory)), m_store(m_directory, std::move(applicationBinding))
{
    connect(&m_listener, &LocalPipeListener::handshakeReceived, this, &AutomationSessionServer::onHello);
    connect(&m_listener, &LocalPipeListener::messageReceived, this, &AutomationSessionServer::onMessage);
    connect(&m_listener, &LocalPipeListener::clientClosed, this, [this]() { clearSession(); });
    connect(&m_listener, &LocalPipeListener::failed, this, [this]() {
        disable();
        emit failed(QStringLiteral("transport_failure"));
    });
}

AutomationSessionServer::~AutomationSessionServer()
{
    blockSignals(true);
    disable();
}

bool AutomationSessionServer::enable()
{
    if (m_enabled) return true;
    const auto owner = queryProcessIdentity(GetCurrentProcessId());
    m_storeError = PairStoreError::InvalidContext;
    if (!owner.ok()) return false;
    if (!m_storeLease.acquire(m_directory, owner.identity.userSid, true)) {
        m_storeError = leaseError(m_storeLease.lastError());
        return false;
    }
    if (!m_store.load(m_credentials, m_storeError)) {
        eraseCredentials(m_credentials);
        m_offlinePairings.clear();
        m_storeLease.release();
        return false;
    }
    if (!m_listener.start()) {
        disable();
        return false;
    }
    if (!publishInstanceDiscovery(m_directory, m_listener.instanceId(), m_listener.pipeName(), m_discoveryError)) {
        disable();
        return false;
    }
    m_discoveryInstance = m_listener.instanceId();
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    m_discoveryLease = own.ok() ? Storage::privateFile(discoveryPath(), own.identity.userSid) : INVALID_HANDLE_VALUE;
    InstanceDescriptor published;
    if (m_discoveryLease == INVALID_HANDLE_VALUE
        || !readInstanceDiscovery(m_directory, queryProcessExecutable(own.identity), published, m_discoveryError)
        || published.instanceId != m_discoveryInstance || published.processId != GetCurrentProcessId()) {
        disable();
        m_discoveryError = DiscoveryError::IdentityRejected;
        return false;
    }
    m_enabled = true;
    emit enabledChanged(true);
    return true;
}

void AutomationSessionServer::disable()
{
    const bool wasEnabled = m_enabled;
    m_enabled = false;
    m_listener.stop();
    clearSession();
    if (wasEnabled || !m_credentials.isEmpty()) {
        m_offlinePairings.clear();
        for (const PairCredential &credential : std::as_const(m_credentials)) m_offlinePairings.append(info(credential));
    }
    eraseCredentials(m_credentials);
    if (m_discoveryLease != INVALID_HANDLE_VALUE) {
        CloseHandle(m_discoveryLease);
        m_discoveryLease = INVALID_HANDLE_VALUE;
    }
    const QString discoveryInstance = std::exchange(m_discoveryInstance, {});
    const bool removed = discoveryInstance.isEmpty()
        || removeInstanceDiscovery(m_directory, discoveryInstance, m_discoveryError);
    m_storeLease.release();
    if (wasEnabled) emit enabledChanged(false);
    if (!removed) emit failed(QStringLiteral("discovery_cleanup_failed"));
}

QList<PairingInfo> AutomationSessionServer::pairings() const
{
    if (!m_enabled) return m_offlinePairings;
    QList<PairingInfo> result;
    for (const PairCredential &pair : m_credentials) result.append(info(pair));
    return result;
}

bool AutomationSessionServer::refreshPairings()
{
    if (m_enabled) return true;
    QList<PairCredential> credentials;
    const auto erase = qScopeGuard([&]() { eraseCredentials(credentials); });
    m_offlinePairings.clear();
    if (!m_store.load(credentials, m_storeError)) return false;
    for (const PairCredential &credential : std::as_const(credentials)) m_offlinePairings.append(info(credential));
    return true;
}

int AutomationSessionServer::findPair(const QString &pairId) const
{
    for (int i = 0; i < m_credentials.size(); ++i) {
        if (m_credentials.at(i).pairId == pairId) return i;
    }
    return -1;
}

void AutomationSessionServer::clearSession()
{
    const QString request = std::exchange(m_pairingRequest, {});
    m_stage = Stage::Idle;
    m_connection.clear();
    m_request.clear();
    m_challenge.cancel();
    eraseSecret(m_pair.secret);
    m_pair = {};
    m_binding = {};
    if (!request.isEmpty()) {
        QTimer::singleShot(0, this, [this, request]() {
            emit pairingFinished(request, false, QStringLiteral("connection_closed"));
        });
    }
}

void AutomationSessionServer::closeSession()
{
    m_listener.closeClient();
    clearSession();
}

void AutomationSessionServer::onHello(const QJsonObject &message, const ProcessIdentity &peer)
{
    clearSession();
    m_connection = m_listener.connectionId();
    m_binding.instanceId = m_listener.instanceId();
    m_binding.client = peer;
    m_binding.server = queryProcessIdentity(GetCurrentProcessId()).identity;
    m_binding.clientNonce = QByteArray::fromHex(message.value(QStringLiteral("client_nonce")).toString().toLatin1());
    if (message.value(QStringLiteral("type")) == QJsonValue(QStringLiteral("pair.request"))) {
        if (m_credentials.size() >= MaxPairCredentials
            || (m_lastPairingRequest.isValid() && m_lastPairingRequest.elapsed() < 1000)) {
            closeSession();
            return;
        }
        m_pair.clientName = message.value(QStringLiteral("client_name")).toString();
        m_pairingRequest = QUuid::createUuid().toString(QUuid::Id128);
        m_stage = Stage::Approval;
        if (!m_listener.prepareMessage(m_connection, false, 60000, false)) {
            closeSession();
            return;
        }
        m_lastPairingRequest.start();
        const PairingRequest request{m_pairingRequest, m_connection, m_pair.clientName, peer};
        emit pairingRequested(request);
        return;
    }
    const int index = findPair(message.value(QStringLiteral("pair_id")).toString());
    if (index < 0) {
        closeSession();
        return;
    }
    m_pair = m_credentials.at(index);
    m_pair.scopes &= startupAllowedScopes();
    if (!beginChallenge(false)) closeSession();
}

bool AutomationSessionServer::approvePairing(const QString &requestId, quint32 scopes)
{
    if (m_stage != Stage::Approval || requestId.isEmpty() || requestId != m_pairingRequest
        || !(scopes & Read) || (scopes & ~startupAllowedScopes()) || !m_listener.matchesConnection(m_connection))
        return false;
    m_pair.pairId = QUuid::createUuid().toString(QUuid::Id128);
    m_pair.permissionEpoch = 1;
    m_pair.scopes = scopes;
    m_pair.secret = secureRandomSecret();
    if (!validPairCredential(m_pair) || !beginChallenge(true)) {
        closeSession();
        return false;
    }
    return true;
}

bool AutomationSessionServer::rejectPairing(const QString &requestId)
{
    if (requestId.isEmpty() || requestId != m_pairingRequest) return false;
    closeSession();
    return true;
}

bool AutomationSessionServer::beginChallenge(bool pairing)
{
    m_binding.pairId = m_pair.pairId;
    m_binding.permissionEpoch = m_pair.permissionEpoch;
    const ProofRole role = pairing ? ProofRole::PairingConfirmation : ProofRole::Client;
    if (!m_challenge.begin(m_binding, m_pair.secret, role)) return false;
    m_stage = pairing ? Stage::PairConfirmation : Stage::Proof;
    return m_listener.prepareMessage(m_connection, false, PreauthTimeoutMs, true)
        && m_listener.sendMessage(m_connection, challengeMessage(pairing));
}

QJsonObject AutomationSessionServer::challengeMessage(bool pairing) const
{
    QJsonObject message{{QStringLiteral("type"), pairing ? QStringLiteral("pair.grant") : QStringLiteral("challenge")},
        {QStringLiteral("version"), HandshakeVersion}, {QStringLiteral("instance"), m_binding.instanceId},
        {QStringLiteral("pair_id"), m_pair.pairId}, {QStringLiteral("epoch"), QString::number(m_pair.permissionEpoch)},
        {QStringLiteral("server_nonce"), QString::fromLatin1(m_challenge.serverNonce().toHex())},
        {QStringLiteral("server_proof"), QString::fromLatin1(m_challenge.serverProof().toHex())},
        {QStringLiteral("client_pid"), QString::number(m_binding.client.processId)},
        {QStringLiteral("client_created"), QString::number(m_binding.client.creationTime)},
        {QStringLiteral("server_pid"), QString::number(m_binding.server.processId)},
        {QStringLiteral("server_created"), QString::number(m_binding.server.creationTime)}};
    if (pairing) {
        message.insert(QStringLiteral("secret"), QString::fromLatin1(m_pair.secret.toHex()));
        message.insert(QStringLiteral("scopes"), int(m_pair.scopes));
    }
    return message;
}

void AutomationSessionServer::onMessage(const QString &connectionId, const QJsonObject &message)
{
    if (connectionId != m_connection || !m_listener.matchesConnection(connectionId)) return;
    if (m_stage == Stage::Proof || m_stage == Stage::PairConfirmation) {
        const bool pairing = m_stage == Stage::PairConfirmation;
        const QString type = pairing ? QStringLiteral("pair.confirm") : QStringLiteral("proof");
        const QJsonValue proof = message.value(QStringLiteral("proof"));
        if (message.size() != 2 || message.value(QStringLiteral("type")) != QJsonValue(type)
            || !proof.isString() || !isLowerHex(proof.toString(), 64)
            || !m_challenge.acceptClient(QByteArray::fromHex(proof.toString().toLatin1()))
            || !becomeReady(pairing)) closeSession();
        return;
    }
    const QString requestId = message.value(QStringLiteral("id")).toString();
    const QJsonValue method = message.value(QStringLiteral("method"));
    if (m_stage != Stage::Ready || !m_request.isEmpty() || message.size() != 3
        || !isLowerHex(requestId, 32) || !method.isString() || method.toString().size() > 64
        || !message.value(QStringLiteral("arguments")).isObject()) {
        closeSession();
        return;
    }
    const int index = findPair(m_pair.pairId);
    if (index < 0 || m_credentials.at(index).permissionEpoch != m_pair.permissionEpoch) {
        closeSession();
        return;
    }
    m_request = requestId;
    const quint32 required = minimumScopes(method.toString());
    if (!required) {
        replyError(connectionId, requestId, QStringLiteral("unsupported_method"), QStringLiteral("Method is not supported."));
        return;
    }
    if ((m_pair.scopes & required) != required) {
        replyError(connectionId, requestId, QStringLiteral("permission_denied"), QStringLiteral("Required permission was not granted."));
        return;
    }
    if (!m_handler) {
        if (method.toString() == QStringLiteral("qkm_get_capabilities")) {
            if (!message.value(QStringLiteral("arguments")).toObject().isEmpty()) {
                replyError(connectionId, requestId, QStringLiteral("invalid_arguments"), QStringLiteral("No arguments are accepted."));
                return;
            }
            replyCapabilities(connectionId, requestId, {});
            return;
        }
        replyError(connectionId, requestId, QStringLiteral("unsupported_capability"), QStringLiteral("Business capability is not connected."));
        return;
    }
    const auto handler = m_handler;
    handler({connectionId, requestId, method.toString(), message.value(QStringLiteral("arguments")).toObject(),
               m_pair.pairId, m_pair.permissionEpoch, m_pair.scopes});
}

bool AutomationSessionServer::becomeReady(bool pairing)
{
    if (pairing) {
        QList<PairCredential> updated = m_credentials;
        updated.append(m_pair);
        if (!m_store.save(updated, m_storeError)) {
            QTimer::singleShot(0, this, [this]() { emit failed(QStringLiteral("pair_store_failure")); });
            return false;
        }
        m_credentials = updated;
    }
    m_stage = Stage::Ready;
    if (!m_listener.prepareMessage(m_connection, true, 0, true)
        || !m_listener.sendMessage(m_connection, {{QStringLiteral("type"), QStringLiteral("ready")},
            {QStringLiteral("version"), HandshakeVersion}, {QStringLiteral("instance"), m_binding.instanceId},
            {QStringLiteral("pair_id"), m_pair.pairId}, {QStringLiteral("epoch"), QString::number(m_pair.permissionEpoch)},
            {QStringLiteral("scopes"), int(m_pair.scopes)}})) return false;
    const QString guiRequest = std::exchange(m_pairingRequest, {});
    const QString connection = m_connection;
    const PairingInfo pair = info(m_pair);
    if (!guiRequest.isEmpty()) {
        QTimer::singleShot(0, this, [this, guiRequest]() { emit pairingFinished(guiRequest, true, {}); });
    }
    emit authenticated(connection, pair);
    return true;
}

void AutomationSessionServer::setRequestHandler(std::function<void(AutomationRequest)> handler)
{
    m_handler = std::move(handler);
}

bool AutomationSessionServer::isRequestCurrent(const AutomationRequest &request) const
{
    const int index = findPair(request.pairId);
    const quint32 required = minimumScopes(request.method);
    return m_enabled && m_stage == Stage::Ready && !request.requestId.isEmpty()
        && request.connectionId == m_connection && request.requestId == m_request
        && request.pairId == m_pair.pairId && request.permissionEpoch == m_pair.permissionEpoch
        && request.scopes == m_pair.scopes && required != 0 && (m_pair.scopes & required) == required
        && index >= 0 && m_credentials.at(index).permissionEpoch == request.permissionEpoch
        && (m_credentials.at(index).scopes & startupAllowedScopes()) == request.scopes && m_listener.matchesConnection(request.connectionId);
}

bool AutomationSessionServer::sendReply(const QString &connectionId, const QString &requestId, QJsonObject response)
{
    if (m_stage != Stage::Ready || connectionId != m_connection || requestId.isEmpty()
        || requestId != m_request) return false;
    response.insert(QStringLiteral("id"), requestId);
    if (!m_listener.prepareMessage(connectionId, true, 0, true)
        || !m_listener.sendMessage(connectionId, response)) return false;
    m_request.clear();
    return true;
}

bool AutomationSessionServer::reply(const QString &connectionId, const QString &requestId, const QJsonObject &result)
{
    return sendReply(connectionId, requestId, {{QStringLiteral("ok"), true}, {QStringLiteral("result"), result}});
}

bool AutomationSessionServer::replyCapabilities(const QString &connectionId, const QString &requestId,
                                               const QJsonArray &businessCapabilities)
{
    QJsonObject result{{QStringLiteral("interface_version"), HandshakeVersion},
        {QStringLiteral("instance"), m_listener.instanceId()}, {QStringLiteral("scopes"), int(m_pair.scopes)},
        {QStringLiteral("stage"), businessCapabilities.isEmpty() ? QStringLiteral("transport_only") : QStringLiteral("business")},
        {QStringLiteral("business_capabilities"), businessCapabilities},
        {QStringLiteral("max_frame_bytes"), MaxFramePayload}};
    if (configurationOnlyDemo()) result.insert(QStringLiteral("execution_policy"), configurationOnlyPolicy());
    return reply(connectionId, requestId, result);
}

bool AutomationSessionServer::replyError(const QString &connectionId, const QString &requestId,
                                        const QString &code, const QString &message)
{
    return sendReply(connectionId, requestId, {{QStringLiteral("ok"), false},
        {QStringLiteral("error"), QJsonObject{{QStringLiteral("code"), code}, {QStringLiteral("message"), message}}}});
}

bool AutomationSessionServer::revoke(const QString &pairId)
{
    Storage::DirectoryLease offlineLease;
    QList<PairCredential> updated;
    const auto erase = qScopeGuard([&]() { eraseCredentials(updated); });
    if (m_enabled) {
        updated = m_credentials;
    } else {
        const auto owner = queryProcessIdentity(GetCurrentProcessId());
        m_storeError = PairStoreError::InvalidContext;
        if (!owner.ok()) return false;
        if (!offlineLease.acquire(m_directory, owner.identity.userSid, false)) {
            m_storeError = leaseError(offlineLease.lastError());
            return false;
        }
        if (!m_store.load(updated, m_storeError)) return false;
        m_offlinePairings.clear();
        for (const PairCredential &credential : std::as_const(updated)) m_offlinePairings.append(info(credential));
    }
    int index = -1;
    for (int i = 0; i < updated.size(); ++i) if (updated.at(i).pairId == pairId) { index = i; break; }
    if (index < 0) return false;
    updated.removeAt(index);
    if (!m_store.save(updated, m_storeError)) {
        if (m_enabled) disable();
        emit failed(QStringLiteral("revocation_not_saved"));
        return false;
    }
    if (m_enabled) m_credentials = updated;
    else {
        m_offlinePairings.clear();
        for (const PairCredential &credential : std::as_const(updated)) m_offlinePairings.append(info(credential));
    }
    offlineLease.release();
    if (m_pair.pairId == pairId) closeSession();
    return true;
}

bool AutomationSessionServer::updatePermissions(const QString &pairId, quint32 scopes)
{
    const int index = findPair(pairId);
    if (index < 0 || !(scopes & Read) || (scopes & ~KnownScopes)
        || ((scopes & ~m_credentials.at(index).scopes) & ~startupAllowedScopes())
        || m_credentials.at(index).permissionEpoch == (std::numeric_limits<quint64>::max)()) return false;
    QList<PairCredential> updated = m_credentials;
    updated[index].scopes = scopes;
    ++updated[index].permissionEpoch;
    if (!m_store.save(updated, m_storeError)) {
        disable();
        emit failed(QStringLiteral("permission_change_not_saved"));
        return false;
    }
    m_credentials = updated;
    if (m_pair.pairId == pairId) closeSession();
    return true;
}

} // namespace QkmAutomation
