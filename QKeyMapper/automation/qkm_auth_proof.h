#ifndef QKEYMAPPER_AUTOMATION_AUTH_PROOF_H
#define QKEYMAPPER_AUTOMATION_AUTH_PROOF_H

#include "qkm_local_identity.h"

#include <QString>

namespace QkmAutomation {

constexpr int HandshakeVersion = 1;
constexpr int AuthSecretBytes = 32;

enum class ProofRole : quint8 { Invalid = 0, Server = 1, Client = 2, PairingConfirmation = 3 };

struct AuthBinding {
    quint32 version = HandshakeVersion;
    QString instanceId;
    QString pairId;
    quint64 permissionEpoch = 0;
    ProcessIdentity client;
    ProcessIdentity server;
    QByteArray clientNonce;
};

bool isLowerHex(const QString &text, int length);
QByteArray secureRandomSecret();
QByteArray authTranscript(const AuthBinding &binding, const QByteArray &serverNonce, ProofRole role);
QByteArray authenticationProof(const AuthBinding &binding, const QByteArray &serverNonce,
                               ProofRole role, const QByteArray &key);
bool equalAuthenticationProof(const QByteArray &a, const QByteArray &b);

class ServerChallenge {
public:
    ServerChallenge() = default;
    ~ServerChallenge();
    bool begin(const AuthBinding &binding, const QByteArray &key);
    bool acceptClient(const QByteArray &proof);
    void cancel();
    bool isPending() const { return m_pending; }
    QByteArray serverNonce() const { return m_nonce; }
    QByteArray serverProof() const { return m_proof; }

private:
    Q_DISABLE_COPY_MOVE(ServerChallenge)
    QByteArray m_nonce;
    QByteArray m_proof;
    QByteArray m_expected;
    bool m_pending = false;
};

#ifdef QKM_AUTOMATION_TESTING
void setAuthRandomTestFailure(bool fail);
#endif

} // namespace QkmAutomation
#endif
