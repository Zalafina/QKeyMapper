#ifndef QKEYMAPPER_AUTOMATION_SESSION_SERVER_H
#define QKEYMAPPER_AUTOMATION_SESSION_SERVER_H

#include "qkm_local_listener.h"
#include "qkm_pair_store.h"
#include "qkm_instance_discovery.h"
#include "qkm_private_storage.h"

#include <functional>
#include <QJsonArray>

namespace QkmAutomation {

struct PairingRequest {
    QString requestId;
    QString connectionId;
    QString clientName;
    ProcessIdentity peer;
};

struct PairingInfo {
    QString pairId;
    QString clientName;
    quint64 permissionEpoch = 0;
    quint32 scopes = 0;
};

struct AutomationRequest {
    QString connectionId;
    QString requestId;
    QString method;
    QJsonObject arguments;
    QString pairId;
    quint64 permissionEpoch = 0;
    quint32 scopes = 0;
};

class AutomationSessionServer : public QObject {
    Q_OBJECT
public:
    AutomationSessionServer(QString storeDirectory, QByteArray applicationBinding, QObject *parent = nullptr);
    ~AutomationSessionServer() override;
    bool enable();
    void disable();
    bool isEnabled() const { return m_enabled; }
    QString pipeName() const { return m_listener.pipeName(); }
    QString instanceId() const { return m_listener.instanceId(); }
    QString discoveryPath() const { return instanceDiscoveryPath(m_directory); }
    DiscoveryError lastDiscoveryError() const { return m_discoveryError; }
    bool acceptingConnections() const { return m_enabled && m_listener.m_notifier.isEnabled(); }
    PairStoreError lastStoreError() const { return m_storeError; }
    QList<PairingInfo> pairings() const;
    bool refreshPairings();
    bool approvePairing(const QString &requestId, quint32 scopes);
    bool rejectPairing(const QString &requestId);
    bool revoke(const QString &pairId);
    bool updatePermissions(const QString &pairId, quint32 scopes);
    void setRequestHandler(std::function<void(AutomationRequest)> handler);
    bool isRequestCurrent(const AutomationRequest &request) const;
    bool reply(const QString &connectionId, const QString &requestId, const QJsonObject &result);
    bool replyCapabilities(const QString &connectionId, const QString &requestId,
                           const QJsonArray &businessCapabilities);
    bool replyError(const QString &connectionId, const QString &requestId,
                    const QString &code, const QString &message);

signals:
    void enabledChanged(bool enabled);
    void pairingRequested(const QkmAutomation::PairingRequest &request);
    void pairingFinished(const QString &requestId, bool success, const QString &reason);
    void authenticated(const QString &connectionId, const QkmAutomation::PairingInfo &pair);
    void failed(const QString &code);

private:
    enum class Stage { Idle, Approval, Proof, PairConfirmation, Ready };
    void onHello(const QJsonObject &message, const ProcessIdentity &peer);
    void onMessage(const QString &connectionId, const QJsonObject &message);
    void clearSession();
    void closeSession();
    bool beginChallenge(bool pairing);
    bool becomeReady(bool pairing);
    QJsonObject challengeMessage(bool pairing) const;
    bool sendReply(const QString &connectionId, const QString &requestId, QJsonObject response);
    int findPair(const QString &pairId) const;

    LocalPipeListener m_listener;
    QString m_directory;
    QString m_discoveryInstance;
    HANDLE m_discoveryLease = INVALID_HANDLE_VALUE;
    DiscoveryError m_discoveryError = DiscoveryError::None;
    PairCredentialStore m_store;
    Storage::DirectoryLease m_storeLease;
    QList<PairCredential> m_credentials;
    QList<PairingInfo> m_offlinePairings;
    PairStoreError m_storeError = PairStoreError::None;
    ServerChallenge m_challenge;
    AuthBinding m_binding;
    PairCredential m_pair;
    QString m_connection;
    QString m_pairingRequest;
    QString m_request;
    Stage m_stage = Stage::Idle;
    bool m_enabled = false;
    QElapsedTimer m_lastPairingRequest;
    std::function<void(AutomationRequest)> m_handler;
};

} // namespace QkmAutomation
#endif
