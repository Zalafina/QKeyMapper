#include "qkm_demo_mode.h"
#include "qkm_pair_store.h"
#include <QCoreApplication>
#include <atomic>
#include <windows.h>

namespace QkmAutomation {
namespace {
std::atomic_bool Initialized{false};
std::atomic_bool ConfigurationOnly{false};
#if defined(QKM_AUTOMATION_TESTING) || defined(QKM_LIFECYCLE_TESTING)
std::atomic<quint64> Denials{0};
#endif
}

StartupPurpose parseStartupPurpose(const QStringList &arguments)
{
    bool requested = false;
    for (const auto &argument : arguments) {
        if (!argument.startsWith(QStringLiteral("--demo-"))) continue;
        if (argument != QStringLiteral("--demo-configuration-only") || requested) return StartupPurpose::Invalid;
        requested = true;
    }
    return requested ? StartupPurpose::ConfigurationOnly : StartupPurpose::Standard;
}

bool currentThreadOnPrivateDesktop()
{
    wchar_t station[256]{}, current[256]{}, input[256]{};
    DWORD needed = 0;
    if (!GetUserObjectInformationW(GetProcessWindowStation(), UOI_NAME, station, sizeof(station), &needed)
        || QString::fromWCharArray(station).compare(QStringLiteral("WinSta0"), Qt::CaseInsensitive) != 0
        || !GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, current, sizeof(current), &needed)
        || QString::fromWCharArray(current).compare(QStringLiteral("Default"), Qt::CaseInsensitive) == 0) return false;
    const HDESK inputDesktop = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
    if (!inputDesktop) return false;
    const bool known = GetUserObjectInformationW(inputDesktop, UOI_NAME, input, sizeof(input), &needed) != FALSE;
    CloseDesktop(inputDesktop);
    return known && QString::fromWCharArray(current).compare(QString::fromWCharArray(input), Qt::CaseInsensitive) != 0;
}

bool initializeStartupPurpose(StartupPurpose purpose)
{
    if (QCoreApplication::instance() || purpose == StartupPurpose::Invalid || Initialized.exchange(true)) return false;
    if (purpose == StartupPurpose::ConfigurationOnly && !currentThreadOnPrivateDesktop()) return false;
    // Publish once before any application, worker or device singleton is constructed.
    ConfigurationOnly.store(purpose == StartupPurpose::ConfigurationOnly);
    return true;
}

bool configurationOnlyDemo()
{
    return ConfigurationOnly.load(std::memory_order_relaxed);
}

quint32 startupAllowedScopes()
{
    return configurationOnlyDemo() ? Read | Configure | ShowWindows | CaptureWindows : KnownScopes;
}

QJsonObject configurationOnlyPolicy()
{
    return {{QStringLiteral("startup_purpose"), QStringLiteral("configuration_only_demo")},
        {QStringLiteral("mapping_allowed"), false}, {QStringLiteral("system_input_allowed"), false},
        {QStringLiteral("device_initialization_allowed"), false}, {QStringLiteral("system_setting_writes_allowed"), false}};
}

bool denyDemoEffect()
{
    if (!configurationOnlyDemo()) return false;
#if defined(QKM_AUTOMATION_TESTING) || defined(QKM_LIFECYCLE_TESTING)
    Denials.fetch_add(1, std::memory_order_relaxed);
#endif
    return true;
}

#if defined(QKM_AUTOMATION_TESTING) || defined(QKM_LIFECYCLE_TESTING)
quint64 demoEffectDenialsForTest() { return Denials.load(std::memory_order_relaxed); }
#endif
} // namespace QkmAutomation
