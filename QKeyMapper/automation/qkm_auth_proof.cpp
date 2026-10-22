#include "qkm_auth_proof.h"

#include <QMessageAuthenticationCode>
#include <QtEndian>
#include <bcrypt.h>

namespace QkmAutomation {
namespace {

#ifdef QKM_AUTOMATION_TESTING
bool randomTestFailure = false;
#endif

template<typename T>
void appendNumber(QByteArray &bytes, T number)
{
    char encoded[sizeof(T)];
    qToBigEndian(number, encoded);
    bytes.append(encoded, static_cast<int>(sizeof(encoded)));
}

} // namespace

bool isLowerHex(const QString &text, int length)
{
    if (length <= 0 || text.size() != length) return false;
    for (const QChar c : text) {
        if (!((c >= QLatin1Char('0') && c <= QLatin1Char('9'))
              || (c >= QLatin1Char('a') && c <= QLatin1Char('f')))) return false;
    }
    return true;
}

QByteArray secureRandomSecret()
{
#ifdef QKM_AUTOMATION_TESTING
    if (randomTestFailure) return {};
#endif
    QByteArray bytes(AuthSecretBytes, '\0');
    if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(bytes.data()), AuthSecretBytes,
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) return {};
    return bytes;
}

bool validClientName(const QString &name)
{
    if (name.trimmed().isEmpty() || name.toUtf8().size() > 80) return false;
    for (const QChar c : name) {
        if (c.category() == QChar::Other_Control || c.category() == QChar::Other_Format)
            return false;
    }
    return true;
}

QByteArray authTranscript(const AuthBinding &binding, const QByteArray &serverNonce, ProofRole role)
{
    if (binding.version != HandshakeVersion || !isLowerHex(binding.instanceId, 32)
        || !isLowerHex(binding.pairId, 32) || !binding.permissionEpoch
        || !sameLogonSession(binding.client, binding.server)
        || binding.clientNonce.size() != AuthSecretBytes || serverNonce.size() != AuthSecretBytes
        || (role != ProofRole::Server && role != ProofRole::Client
            && role != ProofRole::PairingConfirmation)) return {};
    QByteArray bytes("QKeyMapper-MCP-AUTH", 19);
    bytes.append('\0');
    bytes.append(static_cast<char>(role));
    appendNumber(bytes, binding.version);
    bytes.append(QByteArray::fromHex(binding.instanceId.toLatin1()));
    bytes.append(QByteArray::fromHex(binding.pairId.toLatin1()));
    appendNumber(bytes, binding.permissionEpoch);
    appendNumber(bytes, binding.client.processId);
    appendNumber(bytes, binding.client.creationTime);
    appendNumber(bytes, binding.server.processId);
    appendNumber(bytes, binding.server.creationTime);
    bytes.append(binding.clientNonce);
    bytes.append(serverNonce);
    return bytes;
}

QByteArray authenticationProof(const AuthBinding &binding, const QByteArray &serverNonce,
                               ProofRole role, const QByteArray &key)
{
    if (key.size() != AuthSecretBytes) return {};
    const QByteArray transcript = authTranscript(binding, serverNonce, role);
    if (transcript.isEmpty()) return {};
    return QMessageAuthenticationCode::hash(transcript, key, QCryptographicHash::Sha256);
}

bool equalAuthenticationProof(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != AuthSecretBytes || b.size() != AuthSecretBytes) return false;
    // A volatile accumulator keeps all fixed-size comparisons observable without early exit.
    volatile quint32 difference = 0;
    for (int i = 0; i < AuthSecretBytes; ++i)
        difference |= static_cast<uchar>(a.at(i)) ^ static_cast<uchar>(b.at(i));
    return difference == 0;
}

ServerChallenge::~ServerChallenge()
{
    cancel();
}

bool ServerChallenge::begin(const AuthBinding &binding, const QByteArray &key, ProofRole expectedRole)
{
    cancel();
    if (key.size() != AuthSecretBytes
        || (expectedRole != ProofRole::Client && expectedRole != ProofRole::PairingConfirmation)) return false;
    m_nonce = secureRandomSecret();
    if (m_nonce.isEmpty()) return false;
    m_proof = authenticationProof(binding, m_nonce, ProofRole::Server, key);
    m_expected = authenticationProof(binding, m_nonce, expectedRole, key);
    if (m_proof.isEmpty() || m_expected.isEmpty()) {
        cancel();
        return false;
    }
    m_pending = true;
    return true;
}

bool ServerChallenge::acceptClient(const QByteArray &proof)
{
    if (!m_pending) return false;
    m_pending = false;
    const bool valid = equalAuthenticationProof(proof, m_expected);
    if (!m_expected.isEmpty()) SecureZeroMemory(m_expected.data(), static_cast<SIZE_T>(m_expected.size()));
    m_expected.clear();
    return valid;
}

void ServerChallenge::cancel()
{
    m_pending = false;
    if (!m_expected.isEmpty()) SecureZeroMemory(m_expected.data(), static_cast<SIZE_T>(m_expected.size()));
    m_expected.clear();
    m_nonce.clear();
    m_proof.clear();
}

#ifdef QKM_AUTOMATION_TESTING
void setAuthRandomTestFailure(bool fail)
{
    randomTestFailure = fail;
}
#endif

} // namespace QkmAutomation
