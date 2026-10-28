#ifndef QKEYMAPPER_AUTOMATION_PAIR_STORE_H
#define QKEYMAPPER_AUTOMATION_PAIR_STORE_H

#include "qkm_auth_proof.h"

#include <QList>

namespace QkmAutomation {

enum Scope : quint32 {
    Read = 1, Configure = 2, RunMapping = 4, Observe = 8,
    TestInput = 16, ManageDevices = 32, ExecuteCommands = 64,
    ShowWindows = 128, CaptureWindows = 256
};
constexpr quint32 KnownScopes = Read | Configure | RunMapping | Observe
    | TestInput | ManageDevices | ExecuteCommands | ShowWindows | CaptureWindows;
constexpr int MaxPairCredentials = 32;

struct PairCredential {
    QString pairId;
    QString clientName;
    quint64 permissionEpoch = 0;
    quint32 scopes = 0;
    QByteArray secret;
};

bool validPairCredential(const PairCredential &credential);

enum class PairStoreError { None, InvalidData, InvalidContext, UnsafeStorage, IoError, ProtectionFailed, Busy };

class PairCredentialStore {
public:
    PairCredentialStore(QString directory, QByteArray applicationBinding);
    QString filePath() const;
    bool load(QList<PairCredential> &credentials, PairStoreError &error) const;
    bool save(const QList<PairCredential> &credentials, PairStoreError &error) const;

private:
    QString m_directory;
    QByteArray m_applicationBinding;
};

#ifdef QKM_AUTOMATION_TESTING
QByteArray protectPairStoreTestPlain(const QByteArray &plain, const QByteArray &applicationBinding);
#endif

} // namespace QkmAutomation
#endif
