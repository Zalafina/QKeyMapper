#include "qkm_local_identity.h"

#include <QScopeGuard>
#include <cstddef>
#include <limits>
#include <process.h>

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

bool readTokenIdentity(HANDLE token, ProcessIdentity &identity)
{
    const QByteArray userData = tokenInformation(token, TokenUser);
    const QByteArray groupData = tokenInformation(token, TokenGroups);
    const QByteArray statisticsData = tokenInformation(token, TokenStatistics);
    const QByteArray sessionData = tokenInformation(token, TokenSessionId);
    if (userData.size() < static_cast<int>(sizeof(TOKEN_USER))
        || groupData.size() < static_cast<int>(sizeof(TOKEN_GROUPS))
        || statisticsData.size() < static_cast<int>(sizeof(TOKEN_STATISTICS))
        || sessionData.size() < static_cast<int>(sizeof(DWORD)))
        return false;
    const auto *groups = reinterpret_cast<const TOKEN_GROUPS *>(groupData.constData());
    const auto groupCapacity = (static_cast<size_t>(groupData.size()) - offsetof(TOKEN_GROUPS, Groups))
        / sizeof(SID_AND_ATTRIBUTES);
    if (groups->GroupCount > groupCapacity) return false;
    const auto *statistics = reinterpret_cast<const TOKEN_STATISTICS *>(statisticsData.constData());
    identity.sessionId = *reinterpret_cast<const DWORD *>(sessionData.constData());
    identity.authenticationId = (quint64(static_cast<quint32>(statistics->AuthenticationId.HighPart)) << 32)
        | statistics->AuthenticationId.LowPart;
    identity.userSid = sidBytes(reinterpret_cast<const TOKEN_USER *>(userData.constData())->User.Sid);
    identity.logonSid.clear();
    for (DWORD i = 0; i < groups->GroupCount; ++i) {
        if ((groups->Groups[i].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID) {
            identity.logonSid = sidBytes(groups->Groups[i].Sid);
            break;
        }
    }
    return identity.isValid();
}

IdentityResult inspectRequest(HANDLE pipe, const ProcessIdentity &expected)
{
    const IdentityResult peer = queryPipePeerIdentity(pipe, PipeSide::Server);
    if (!peer.ok() || !sameProcess(peer.identity, expected))
        return {{}, IdentityError::RequestMismatch};
    HANDLE token = nullptr;
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token)) {
        CloseHandle(token);
        return {{}, IdentityError::WorkerContextNotEmpty};
    }
    if (GetLastError() != ERROR_NO_TOKEN || !ImpersonateNamedPipeClient(pipe))
        return {{}, IdentityError::RequestUnavailable};
    bool restored = false;
    IdentityResult result{{}, IdentityError::RequestUnavailable};
    {
        const auto revert = qScopeGuard([&]() { restored = RevertToSelf(); });
        if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token)) {
            const auto closeToken = qScopeGuard([token]() { CloseHandle(token); });
            ProcessIdentity request = peer.identity;
            if (readTokenIdentity(token, request)) {
                result = sameLogonSession(request, expected)
                    ? IdentityResult{request, IdentityError::None}
                    : IdentityResult{{}, IdentityError::RequestMismatch};
            }
        }
    }
    // A failed restore must never reach more application work, even on this disposable thread.
    if (!restored) return {{}, IdentityError::ContextRestoreFailed};
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token)) {
        CloseHandle(token);
        return {{}, IdentityError::ContextRestoreFailed};
    }
    if (GetLastError() != ERROR_NO_TOKEN) return {{}, IdentityError::ContextRestoreFailed};
    if (!result.ok()) return result;
    const IdentityResult after = queryPipePeerIdentity(pipe, PipeSide::Server);
    if (!after.ok() || !sameProcess(after.identity, expected))
        return {{}, IdentityError::RequestMismatch};
    return result;
}

} // namespace

struct RequestIdentityState {
    HANDLE pipe = INVALID_HANDLE_VALUE;
    ProcessIdentity expected;
    IdentityResult result{{}, IdentityError::RequestUnavailable};
    ~RequestIdentityState() { if (pipe != INVALID_HANDLE_VALUE) CloseHandle(pipe); }
};

namespace {

unsigned __stdcall inspectRequestThread(void *argument)
{
    std::unique_ptr<std::shared_ptr<RequestIdentityState>> holder(
        static_cast<std::shared_ptr<RequestIdentityState> *>(argument));
    const std::shared_ptr<RequestIdentityState> state = *holder;
    holder.reset();
    try {
        state->result = inspectRequest(state->pipe, state->expected);
    } catch (...) {
        state->result = {{}, IdentityError::RequestUnavailable};
    }
    // No QObject, event loop, signal or business callback runs on this thread.
    return 0;
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
    ProcessIdentity identity;
    identity.processId = processId;
    identity.creationTime = (quint64(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    if (!readTokenIdentity(token, identity)) return {{}, IdentityError::InvalidToken};
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

PipeRequestVerifier::PipeRequestVerifier(QObject *parent) : QObject(parent)
{
    connect(&m_done, &QWinEventNotifier::activated, this, [this]() {
        if (!m_thread || WaitForSingleObject(m_thread, 0) != WAIT_OBJECT_0) return;
        m_done.setEnabled(false);
        m_done.setHandle(nullptr);
        const IdentityResult result = m_state->result;
        CloseHandle(m_thread);
        m_thread = nullptr;
        m_state.reset();
        emit finished(result);
    });
}

PipeRequestVerifier::~PipeRequestVerifier()
{
    m_done.setEnabled(false);
    m_done.setHandle(nullptr);
    if (m_thread) CloseHandle(m_thread);
    // The thread owns its data and duplicate handle until it exits; destruction never waits.
}

bool PipeRequestVerifier::start(HANDLE pipe, const ProcessIdentity &expected)
{
    if (m_thread || !pipe || pipe == INVALID_HANDLE_VALUE || !expected.isValid()) return false;
    auto state = std::make_shared<RequestIdentityState>();
    if (!DuplicateHandle(GetCurrentProcess(), pipe, GetCurrentProcess(), &state->pipe,
                         0, FALSE, DUPLICATE_SAME_ACCESS)) return false;
    state->expected = expected;
    auto holder = std::make_unique<std::shared_ptr<RequestIdentityState>>(state);
    m_thread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, inspectRequestThread,
                                                      holder.get(), 0, nullptr));
    if (!m_thread) return false;
    holder.release();
    m_state = state;
    m_done.setHandle(m_thread);
    m_done.setEnabled(true);
    return true;
}

} // namespace QkmAutomation
