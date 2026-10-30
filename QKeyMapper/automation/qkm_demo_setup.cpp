#include "qkm_demo_setup.h"
#include "qkm_demo_mode.h"
#include <QCoreApplication>
#include <QScopeGuard>
#include <QThread>
#include <tlhelp32.h>

namespace QkmAutomation {
namespace {
struct InheritedSetup {
    HANDLE request = nullptr, ack = nullptr;
    ~InheritedSetup() {
        if (request) CloseHandle(request);
        if (ack) CloseHandle(ack);
    }
} inherited;

quint64 takeNumber(const char *key)
{
    const QString text = qEnvironmentVariable(key);
    qunsetenv(key);
    bool ok = false;
    const quint64 value = text.toULongLong(&ok);
    return ok && QString::number(value) == text ? value : 0;
}

DWORD currentParent()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    const auto close = qScopeGuard([snapshot] { CloseHandle(snapshot); });
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (entry.th32ProcessID == GetCurrentProcessId()) return entry.th32ParentProcessID;
        } while (Process32NextW(snapshot, &entry));
    }
    return 0;
}
} // namespace

void prepareInheritedDemoSetup()
{
    if (!configurationOnlyDemo()) return;
    const quint64 request = takeNumber(DemoSetupRequestEnv), ack = takeNumber(DemoSetupAckEnv);
    const quint64 parentId = takeNumber(DemoSetupParentEnv), created = takeNumber(DemoSetupCreatedEnv);
    if (!request || !ack || request == ack || quintptr(request) != request || quintptr(ack) != ack
        || !parentId || parentId > MAXDWORD || !created || !currentThreadOnPrivateDesktop()
        || inherited.request || inherited.ack || currentParent() != parentId) return;
    const auto own = queryProcessIdentity(GetCurrentProcessId()), owner = queryProcessIdentity(DWORD(parentId));
    if (!own.ok() || !owner.ok() || owner.identity.creationTime != created || !sameLogonSession(own.identity, owner.identity)) return;
    const QString executable = queryProcessExecutable(own.identity);
    if (executable.isEmpty() || executable.compare(queryProcessExecutable(owner.identity), Qt::CaseInsensitive) != 0) return;
    HANDLE requestHandle = reinterpret_cast<HANDLE>(quintptr(request)), ackHandle = reinterpret_cast<HANDLE>(quintptr(ack));
    if (!ResetEvent(requestHandle) || !ResetEvent(ackHandle)) return;
    if (!SetHandleInformation(requestHandle, HANDLE_FLAG_INHERIT, 0) || !SetHandleInformation(ackHandle, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(requestHandle); CloseHandle(ackHandle);
        return;
    }
    inherited.request = requestHandle; inherited.ack = ackHandle;
}

DemoSetupEndpoint::DemoSetupEndpoint(std::function<bool()> showManagement, QObject *parent)
    : QObject(parent), m_notifier(this)
{
    if (!configurationOnlyDemo() || !currentThreadOnPrivateDesktop() || !QCoreApplication::instance()
        || QThread::currentThread() != QCoreApplication::instance()->thread()) return;
    m_request = inherited.request; m_ack = inherited.ack;
    inherited.request = inherited.ack = nullptr;
    if (!ready()) return;
    const HDESK desktop = GetThreadDesktop(GetCurrentThreadId());
    m_notifier.setHandle(m_request);
    connect(&m_notifier, &QWinEventNotifier::activated, this, [this, desktop, showManagement] {
        if (GetThreadDesktop(GetCurrentThreadId()) == desktop && showManagement()) SetEvent(m_ack);
    });
    m_notifier.setEnabled(true);
    SetEvent(m_ack);
}

DemoSetupEndpoint::~DemoSetupEndpoint()
{
    m_notifier.setEnabled(false);
    m_notifier.setHandle(nullptr);
    if (m_request) CloseHandle(m_request);
    if (m_ack) CloseHandle(m_ack);
}

} // namespace QkmAutomation
