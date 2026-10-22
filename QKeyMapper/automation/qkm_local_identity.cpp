#include "qkm_local_identity.h"

#include <QScopeGuard>
#include <cstddef>
#include <limits>

namespace QkmAutomation {
namespace {

QByteArray tokenInformation(HANDLE token, TOKEN_INFORMATION_CLASS kind)
{
    DWORD size = 0;
    if (GetTokenInformation(token, kind, nullptr, 0, &size)
        || GetLastError() != ERROR_INSUFFICIENT_BUFFER || !size
        || size > static_cast<DWORD>((std::numeric_limits<int>::max)())) return {};
    QByteArray bytes(static_cast<int>(size), '\0');
    if (!GetTokenInformation(token, kind, bytes.data(), size, &size)) return {};
    return bytes;
}

QByteArray sidBytes(PSID sid)
{
    if (!sid || !IsValidSid(sid)) return {};
    return QByteArray(static_cast<const char *>(sid), static_cast<int>(GetLengthSid(sid)));
}

bool validSid(const QByteArray &bytes)
{
    if (bytes.size() < static_cast<int>(sizeof(SID))) return false;
    // Check the count before IsValidSid can inspect a variable-length SID.
    const auto *sid = reinterpret_cast<const SID *>(bytes.constData());
    const int length = 8 + 4 * sid->SubAuthorityCount;
    return length == bytes.size()
        && IsValidSid(const_cast<char *>(bytes.constData()));
}

} // namespace

bool ProcessIdentity::isValid() const
{
    return processId && creationTime && authenticationId
        && validSid(userSid) && validSid(logonSid);
}

IdentityResult queryProcessIdentity(quint32 processId)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
                                 FALSE, processId);
    if (!process) return {{}, IdentityError::ProcessUnavailable};
    const auto closeProcess = qScopeGuard([process]() { CloseHandle(process); });
    if (WaitForSingleObject(process, 0) != WAIT_TIMEOUT)
        return {{}, IdentityError::ProcessUnavailable};
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(process, &created, &exited, &kernel, &user))
        return {{}, IdentityError::ProcessUnavailable};
    HANDLE token = nullptr;
    if (!OpenProcessToken(process, TOKEN_QUERY, &token))
        return {{}, IdentityError::TokenUnavailable};
    const auto closeToken = qScopeGuard([token]() { CloseHandle(token); });
    const QByteArray userData = tokenInformation(token, TokenUser);
    const QByteArray groupData = tokenInformation(token, TokenGroups);
    const QByteArray statisticsData = tokenInformation(token, TokenStatistics);
    const QByteArray sessionData = tokenInformation(token, TokenSessionId);
    if (userData.size() < static_cast<int>(sizeof(TOKEN_USER))
        || groupData.size() < static_cast<int>(sizeof(TOKEN_GROUPS))
        || statisticsData.size() < static_cast<int>(sizeof(TOKEN_STATISTICS))
        || sessionData.size() < static_cast<int>(sizeof(DWORD)))
        return {{}, IdentityError::InvalidToken};
    const auto *groups = reinterpret_cast<const TOKEN_GROUPS *>(groupData.constData());
    const auto groupCapacity = (static_cast<size_t>(groupData.size()) - offsetof(TOKEN_GROUPS, Groups))
        / sizeof(SID_AND_ATTRIBUTES);
    if (groups->GroupCount > groupCapacity) return {{}, IdentityError::InvalidToken};
    const auto *statistics = reinterpret_cast<const TOKEN_STATISTICS *>(statisticsData.constData());
    ProcessIdentity identity;
    identity.processId = processId;
    identity.creationTime = (quint64(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    identity.sessionId = *reinterpret_cast<const DWORD *>(sessionData.constData());
    identity.authenticationId = (quint64(static_cast<quint32>(statistics->AuthenticationId.HighPart)) << 32)
        | statistics->AuthenticationId.LowPart;
    identity.userSid = sidBytes(reinterpret_cast<const TOKEN_USER *>(userData.constData())->User.Sid);
    for (DWORD i = 0; i < groups->GroupCount; ++i) {
        if ((groups->Groups[i].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID) {
            identity.logonSid = sidBytes(groups->Groups[i].Sid);
            break;
        }
    }
    if (!identity.isValid()) return {{}, IdentityError::InvalidToken};
    if (WaitForSingleObject(process, 0) != WAIT_TIMEOUT)
        return {{}, IdentityError::ProcessUnavailable};
    return {identity, IdentityError::None};
}

IdentityResult queryPipePeerIdentity(HANDLE pipe, PipeSide side)
{
    ULONG processId = 0, sessionId = 0;
    bool found = false;
    if (side == PipeSide::Server) {
        found = GetNamedPipeClientProcessId(pipe, &processId)
            && GetNamedPipeClientSessionId(pipe, &sessionId);
    } else if (side == PipeSide::Client) {
        found = GetNamedPipeServerProcessId(pipe, &processId)
            && GetNamedPipeServerSessionId(pipe, &sessionId);
    }
    if (!found) return {{}, IdentityError::PeerUnavailable};
    IdentityResult result = queryProcessIdentity(processId);
    if (!result.ok()) return result;
    if (result.identity.sessionId != sessionId) return {{}, IdentityError::SessionMismatch};
    return result;
}

bool sameLogonSession(const ProcessIdentity &a, const ProcessIdentity &b)
{
    return a.isValid() && b.isValid() && a.sessionId == b.sessionId
        && a.authenticationId == b.authenticationId
        && a.userSid == b.userSid && a.logonSid == b.logonSid;
}

bool sameProcess(const ProcessIdentity &a, const ProcessIdentity &b)
{
    return sameLogonSession(a, b) && a.processId == b.processId
        && a.creationTime == b.creationTime;
}

} // namespace QkmAutomation
