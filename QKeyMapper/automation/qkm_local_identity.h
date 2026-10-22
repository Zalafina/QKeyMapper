#ifndef QKEYMAPPER_AUTOMATION_LOCAL_IDENTITY_H
#define QKEYMAPPER_AUTOMATION_LOCAL_IDENTITY_H

#include <QByteArray>
#include <QtCore/qt_windows.h>

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
    PeerUnavailable, SessionMismatch
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

} // namespace QkmAutomation
#endif
