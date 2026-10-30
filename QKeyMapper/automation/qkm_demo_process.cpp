#include "qkm_demo_process.h"
#include "qkm_demo_mode.h"
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QScopeGuard>
#include <QSettings>
#include <QUuid>
#include <sddl.h>
#include <tlhelp32.h>

namespace QkmAutomation {
namespace {
constexpr DWORD CloseMilliseconds = 5000;

struct ClosingWindows {
    DWORD processId;
    int posted = 0;
    std::optional<DWORD> error;
};

BOOL CALLBACK requestClose(HWND window, LPARAM argument)
{
    auto &closing = *reinterpret_cast<ClosingWindows *>(argument);
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId != closing.processId || GetWindow(window, GW_OWNER) || !IsWindowVisible(window)) return TRUE;
    if (!PostMessageW(window, WM_CLOSE, 0, 0)) {
        closing.error = GetLastError();
        return FALSE;
    }
    ++closing.posted;
    return TRUE;
}
} // namespace

DemoProcess::~DemoProcess()
{
    const bool closed = m_starting ? abortStart() : close();
    if (!closed && (m_process || m_desktop))
        qWarning() << "Demo process cleanup incomplete; retained target identity" << m_target.processId << m_target.creationTime;
    releaseHandles();
}

bool DemoProcess::fail(const char *code, std::optional<DWORD> error)
{
    m_failure = QString::fromLatin1(code);
    m_nativeError = error;
    return false;
}

bool DemoProcess::prepared(const QString &directory)
{
    const QString path = QDir::toNativeSeparators(QDir(directory).filePath(QStringLiteral("keymapdata.ini")));
    HANDLE file = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()), GENERIC_READ | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) return fail("deployment_unavailable", GetLastError());
    const auto closeFile = qScopeGuard([file]() { CloseHandle(file); });
    BY_HANDLE_FILE_INFORMATION info{};
    LARGE_INTEGER size{};
    if (!GetFileInformationByHandle(file, &info) || !GetFileSizeEx(file, &size))
        return fail("deployment_unavailable", GetLastError());
    if (GetFileType(file) != FILE_TYPE_DISK || (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
        || info.nNumberOfLinks != 1 || size.QuadPart <= 0 || size.QuadPart > 65536)
        return fail("deployment_not_prepared");
    QSettings settings(path, QSettings::IniFormat);
    // The marker and initial guard values are ASCII; authenticated scene checks still validate the full model.
    if (settings.value(QStringLiteral("McpDemoDeploymentVersion")).toString() != QStringLiteral("1")
        || settings.value(QStringLiteral("SettingSelect")).toString() != QStringLiteral("Demo104Seed"))
        return fail("deployment_not_prepared");
    for (const char *key : {"StartupAutoMonitoring", "StartupMinimized", "VirtualGamepadEnable", "MultiInputEnable",
            "EnableSystemFilterKeyChecked", "DisableFilterKeyClickSoundChecked", "CloseToSystemtray"})
        if (settings.value(QLatin1String(key), true).toBool()) return fail("deployment_not_prepared");
    if (!settings.value(QStringLiteral("CloseToSystemtray_Decided"), false).toBool()
        || settings.value(QStringLiteral("AutoStartup"), false).toBool()
        || settings.value(QStringLiteral("Demo104Seed/AutoStartMappingChecked"), 1).toString() != QStringLiteral("0")
        || settings.value(QStringLiteral("Demo104Seed/MappingTable_Common_Enabled"), true).toBool()
        || !settings.contains(QStringLiteral("Demo104Seed/KeyMapData_OriginalKeys"))
        || !settings.contains(QStringLiteral("Demo104Seed/KeyMapData_MappingKeys"))
        || !settings.value(QStringLiteral("Demo104Seed/KeyMapData_OriginalKeys")).toString().isEmpty()
        || !settings.value(QStringLiteral("Demo104Seed/KeyMapData_MappingKeys")).toString().isEmpty()
        || settings.status() != QSettings::NoError) return fail("deployment_not_prepared");
    return true;
}

bool DemoProcess::noLivePeer(const ProcessIdentity &own)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return fail("peer_check_unavailable", GetLastError());
    const auto closeSnapshot = qScopeGuard([snapshot]() { CloseHandle(snapshot); });
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (!Process32FirstW(snapshot, &entry)) return fail("peer_check_unavailable", GetLastError());
    const QString name = QFileInfo(m_executable).fileName();
    do {
        if (entry.th32ProcessID == own.processId
            || QString::fromWCharArray(entry.szExeFile).compare(name, Qt::CaseInsensitive) != 0) continue;
        DWORD session = 0;
        if (!ProcessIdToSessionId(entry.th32ProcessID, &session)) return fail("peer_check_unavailable", GetLastError());
        if (session != own.sessionId) continue;
        const auto peer = queryProcessIdentity(entry.th32ProcessID);
        if (!peer.ok()) return fail("peer_check_unavailable");
        const QString executable = queryProcessExecutable(peer.identity);
        if (executable.isEmpty()) return fail("peer_check_unavailable");
        if (executable.compare(m_executable, Qt::CaseInsensitive) == 0) return fail("deployment_has_live_instance");
    } while (Process32NextW(snapshot, &entry));
    const DWORD error = GetLastError();
    return error == ERROR_NO_MORE_FILES ? true : fail("peer_check_unavailable", error);
}

bool DemoProcess::start()
{
    if (!configurationOnlyDemo()) return fail("configuration_only_required");
    if (m_process || m_desktop) return fail("already_started");
    m_failure.clear(); m_nativeError.reset(); m_cleanupError.reset(); m_exitCode.reset();
    m_desktopName.clear(); m_executable.clear();
    m_target = {}; m_closeRequested = false;
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    if (!own.ok()) return fail("identity_unavailable");
    m_executable = queryProcessExecutable(own.identity);
    if (m_executable.isEmpty()) return fail("identity_unavailable");
    if (!prepared(QFileInfo(m_executable).absolutePath())) return false;
    wchar_t station[256]{};
    DWORD needed = 0;
    if (!GetUserObjectInformationW(GetProcessWindowStation(), UOI_NAME, station, sizeof(station), &needed))
        return fail("station_unavailable", GetLastError());
    if (QString::fromWCharArray(station).compare(QStringLiteral("WinSta0"), Qt::CaseInsensitive) != 0)
        return fail("unsupported_station");
    LPWSTR sid = nullptr;
    if (!ConvertSidToStringSidW(const_cast<char *>(own.identity.userSid.constData()), &sid))
        return fail("desktop_security_failed", GetLastError());
    const auto freeSid = qScopeGuard([sid]() { LocalFree(sid); });
    const QString sddl = QStringLiteral("O:%1D:P(A;;GA;;;%1)").arg(QString::fromWCharArray(sid));
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(reinterpret_cast<LPCWSTR>(sddl.utf16()),
            SDDL_REVISION_1, &descriptor, nullptr)) return fail("desktop_security_failed", GetLastError());
    const auto freeDescriptor = qScopeGuard([descriptor]() { LocalFree(descriptor); });
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), descriptor, FALSE};
    const QString leaseDirectory = QDir(QFileInfo(m_executable).absolutePath())
        .filePath(QStringLiteral("mcp-demo-owner-%1").arg(own.identity.sessionId));
    if (!m_lease.acquire(leaseDirectory, own.identity.userSid, true)) return fail("deployment_in_use", m_lease.lastError());
    if (!noLivePeer(own.identity)) { releaseHandles(); return false; }
    m_desktopName = QStringLiteral("QkmDemoOwned_%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    m_desktop = CreateDesktopW(reinterpret_cast<LPCWSTR>(m_desktopName.utf16()), nullptr, nullptr, 0,
        DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS, &security);
    if (!m_desktop) {
        const DWORD error = GetLastError();
        releaseHandles();
        return fail("desktop_creation_failed", error);
    }
    std::wstring desktop = (QStringLiteral("winsta0\\") + m_desktopName).toStdWString();
    std::wstring command = (QLatin1Char('"') + m_executable + QStringLiteral("\" --demo-configuration-only")).toStdWString();
    const QString directory = QDir::toNativeSeparators(QFileInfo(m_executable).absolutePath());
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup); startup.lpDesktop = desktop.data();
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(reinterpret_cast<LPCWSTR>(m_executable.utf16()), command.data(), nullptr, nullptr, FALSE,
            CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW, nullptr,
            reinterpret_cast<LPCWSTR>(directory.utf16()), &startup, &process)) {
        const DWORD error = GetLastError();
        releaseHandles();
        return fail("process_creation_failed", error);
    }
    m_process = process.hProcess; m_thread = process.hThread; m_starting = true;
    m_target.processId = process.dwProcessId;
    const auto child = queryProcessIdentity(process.dwProcessId);
    if (!child.ok() || !sameLogonSession(own.identity, child.identity)
        || queryProcessExecutable(child.identity).compare(m_executable, Qt::CaseInsensitive) != 0) {
        fail("child_identity_mismatch");
        abortStart();
        return false;
    }
    m_target = child.identity;
    if (ResumeThread(m_thread) == DWORD(-1)) {
        fail("child_resume_failed", GetLastError());
        abortStart();
        return false;
    }
    CloseHandle(m_thread); m_thread = nullptr; m_starting = false;
    return true;
}

bool DemoProcess::running() const
{
    return m_process && WaitForSingleObject(m_process, 0) == WAIT_TIMEOUT;
}

bool DemoProcess::abortStart()
{
    if (m_process) {
        if (!TerminateProcess(m_process, ERROR_PROCESS_ABORTED)) { m_cleanupError = GetLastError(); return false; }
        const DWORD wait = WaitForSingleObject(m_process, CloseMilliseconds);
        if (wait != WAIT_OBJECT_0) {
            if (wait == WAIT_FAILED) m_cleanupError = GetLastError();
            return false;
        }
    }
    return releaseHandles();
}

bool DemoProcess::finishExit()
{
    DWORD code = 0;
    const bool known = GetExitCodeProcess(m_process, &code) != FALSE;
    const DWORD error = known ? ERROR_SUCCESS : GetLastError();
    if (known) m_exitCode = code;
    const bool released = releaseHandles();
    if (!known) return fail("exit_code_unavailable", error);
    if (!released) return fail("desktop_cleanup_failed", m_cleanupError);
    m_failure.clear(); m_nativeError.reset();
    return true;
}

bool DemoProcess::close()
{
    if (!m_process) return releaseHandles();
    if (m_starting) return abortStart();
    QElapsedTimer timer;
    timer.start();
    do {
        const DWORD wait = WaitForSingleObject(m_process, 0);
        if (wait == WAIT_OBJECT_0) return finishExit();
        if (wait == WAIT_FAILED) return fail("wait_failed", GetLastError());
        if (!m_closeRequested) {
            ClosingWindows closing{m_target.processId};
            if (!EnumDesktopWindows(m_desktop, requestClose, reinterpret_cast<LPARAM>(&closing)))
                return fail("close_request_failed", closing.error ? closing.error : std::optional<DWORD>(GetLastError()));
            m_closeRequested = closing.posted > 0;
        }
        const DWORD remaining = CloseMilliseconds - DWORD(qMin<qint64>(timer.elapsed(), CloseMilliseconds));
        WaitForSingleObject(m_process, qMin<DWORD>(50, remaining));
    } while (timer.elapsed() < CloseMilliseconds);
    if (WaitForSingleObject(m_process, 0) == WAIT_OBJECT_0) return finishExit();
    return fail("close_timeout");
}

bool DemoProcess::releaseHandles()
{
    if (m_thread) { CloseHandle(m_thread); m_thread = nullptr; }
    if (m_process) { CloseHandle(m_process); m_process = nullptr; }
    if (m_desktop) {
        if (!CloseDesktop(m_desktop)) { m_cleanupError = GetLastError(); return false; }
        m_desktop = nullptr;
    }
    m_starting = false;
    m_lease.release();
    return true;
}
} // namespace QkmAutomation
