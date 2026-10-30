#include "qkm_demo_pairing.h"
#include "qkm_demo_mode.h"
#include <QScopeGuard>
#include <process.h>
#include <cstdlib>
#include <cwchar>

namespace QkmAutomation {
namespace {
DemoSetupResult failure(const char *code, DWORD error = 0)
{
    return {QString::fromLatin1(code), error ? std::optional<DWORD>(error) : std::nullopt};
}

bool userObjectName(HANDLE object, wchar_t (&name)[256], DWORD &error)
{
    if (!object) { error = 0; return false; }
    DWORD needed = 0;
    if (object && GetUserObjectInformationW(object, UOI_NAME, name, sizeof(name), &needed)) return true;
    error = GetLastError();
    return false;
}
} // namespace

struct DemoPairingState {
    HANDLE process = nullptr, cancel = nullptr;
    HDESK desktop = nullptr, original = nullptr;
    QString desktopName, title, instructions, retry;
    ProcessIdentity target;
    QString executable;
    bool display = false;
    DWORD timeout = 0;
    DemoSetupResult result;
    ~DemoPairingState() {
        if (process) CloseHandle(process);
        if (cancel) CloseHandle(cancel);
        if (desktop) CloseDesktop(desktop);
        if (original) CloseDesktop(original);
    }
};

namespace {
enum class Input { Unknown, Original, Owned, Other };

Input inputDesktop(const DemoPairingState &state, DWORD &error)
{
    error = 0;
    HDESK desktop = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
    if (!desktop) { error = GetLastError(); return Input::Unknown; }
    wchar_t name[256]{};
    const bool known = userObjectName(desktop, name, error);
    CloseDesktop(desktop);
    if (!known) return Input::Unknown;
    if (_wcsicmp(name, L"Default") == 0) return Input::Original;
    return _wcsicmp(name, reinterpret_cast<LPCWSTR>(state.desktopName.utf16())) == 0 ? Input::Owned : Input::Other;
}

struct Prompt {
    DemoPairingState &state;
    ULONGLONG started = GetTickCount64();
    HWND window = nullptr;
    bool recovery = false, posted = false;
    const char *stopped = nullptr;
    DWORD error = 0;
};
thread_local Prompt *activePrompt = nullptr;

BOOL CALLBACK findPrompt(HWND window, LPARAM argument)
{
    wchar_t name[32]{};
    GetClassNameW(window, name, 32);
    if (std::wcscmp(name, L"#32770") != 0) return TRUE;
    auto &prompt = *reinterpret_cast<Prompt *>(argument);
    prompt.window = window;
    RECT area{}, bounds{};
    if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &area, 0) && GetWindowRect(window, &bounds))
        SetWindowPos(window, HWND_TOPMOST, area.right - (bounds.right - bounds.left) - 12,
            area.bottom - (bounds.bottom - bounds.top) - 12, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    return FALSE;
}

void CALLBACK pollPrompt(HWND, UINT, UINT_PTR, DWORD)
{
    auto *prompt = activePrompt;
    if (!prompt || prompt->posted) return;
    if (!prompt->window) EnumThreadWindows(GetCurrentThreadId(), findPrompt, reinterpret_cast<LPARAM>(prompt));
    if (!prompt->window) return;
    DWORD error = 0;
    const Input input = prompt->state.display ? inputDesktop(prompt->state, error) : Input::Owned;
    bool stop = input == Input::Original || input == Input::Other;
    if (input == Input::Other) prompt->stopped = "input_desktop_changed";
    if (!prompt->recovery && !stop) {
        const DWORD child = WaitForSingleObject(prompt->state.process, 0);
        if (child == WAIT_FAILED) { prompt->stopped = "target_wait_failed"; prompt->error = GetLastError(); }
        else if (child == WAIT_OBJECT_0) prompt->stopped = "target_exited";
        const DWORD cancelled = WaitForSingleObject(prompt->state.cancel, 0);
        if (cancelled == WAIT_FAILED && !prompt->stopped) { prompt->stopped = "cancel_wait_failed"; prompt->error = GetLastError(); }
        else if (cancelled == WAIT_OBJECT_0 && !prompt->stopped) prompt->stopped = "setup_cancelled";
        if (!prompt->stopped && GetTickCount64() - prompt->started >= prompt->state.timeout) prompt->stopped = "setup_timeout";
        stop = prompt->stopped != nullptr;
    }
    if (stop) {
        // This fresh worker owns only this native modal dialog; no external HWND is accepted.
        prompt->posted = PostMessageW(prompt->window, WM_CLOSE, 0, 0) != FALSE;
        if (!prompt->posted) { prompt->stopped = "prompt_close_failed"; prompt->error = GetLastError(); }
    }
}

DemoSetupResult showPrompt(DemoPairingState &state, const QString &text, bool recovery)
{
    Prompt prompt{state};
    prompt.recovery = recovery;
    activePrompt = &prompt;
    const auto reset = qScopeGuard([] { activePrompt = nullptr; });
    const UINT_PTR timer = SetTimer(nullptr, 0, 100, pollPrompt);
    if (!timer) return failure("prompt_timer_failed", GetLastError());
    const auto stop = qScopeGuard([timer] { KillTimer(nullptr, timer); });
    if (!MessageBoxW(nullptr, reinterpret_cast<LPCWSTR>(text.utf16()), reinterpret_cast<LPCWSTR>(state.title.utf16()),
            MB_OK | MB_ICONINFORMATION | MB_TOPMOST)) return failure("prompt_failed", GetLastError());
    return prompt.stopped ? failure(prompt.stopped, prompt.error) : DemoSetupResult{};
}

DemoSetupResult runPairing(DemoPairingState &state)
{
    const HDESK originalThread = GetThreadDesktop(GetCurrentThreadId());
    const auto actual = queryProcessIdentity(state.target.processId);
    if (!actual.ok() || !sameProcess(actual.identity, state.target)
        || queryProcessExecutable(actual.identity).compare(state.executable, Qt::CaseInsensitive) != 0)
        return failure("stale_target");
    const DWORD cancelled = WaitForSingleObject(state.cancel, 0);
    if (cancelled == WAIT_FAILED) return failure("cancel_wait_failed", GetLastError());
    if (cancelled == WAIT_OBJECT_0) return failure("setup_cancelled");
    if (!SetThreadDesktop(state.desktop)) return failure("thread_desktop_failed", GetLastError());
    DemoSetupResult result;
    auto restoreThread = qScopeGuard([originalThread] { SetThreadDesktop(originalThread); });
    bool shown = !state.display;
    bool attempted = false;
    if (state.display) {
        DWORD error = 0;
        if (inputDesktop(state, error) != Input::Original) result = failure("default_input_required", error);
        else {
            attempted = true;
            SetLastError(ERROR_SUCCESS);
            shown = SwitchDesktop(state.desktop) != FALSE;
            const DWORD native = shown ? 0 : GetLastError();
            if (!shown) result = failure("desktop_switch_failed", native);
            else if (inputDesktop(state, error) != Input::Owned) result = failure("desktop_display_unverified", error);
        }
    }
    if (shown && result.ok()) result = showPrompt(state, state.instructions, false);
    if (state.display && attempted) {
        for (;;) {
            DWORD error = 0;
            const Input input = inputDesktop(state, error);
            if (input == Input::Original) break;
            if (input == Input::Other) { result = failure("input_desktop_changed"); break; }
            DemoSetupResult restore = failure("desktop_restore_unverified", error);
            if (input == Input::Owned) {
                SetLastError(ERROR_SUCCESS);
                const bool switched = SwitchDesktop(state.original) != FALSE;
                const DWORD native = switched ? 0 : GetLastError();
                if (switched && inputDesktop(state, error) == Input::Original) break;
                restore = failure(switched ? "desktop_restore_unverified" : "desktop_restore_failed", switched ? error : native);
            }
            const QString detail = restore.failure + (restore.nativeError ? QStringLiteral(" (%1)").arg(*restore.nativeError) : QString());
            const auto retry = showPrompt(state, state.retry.arg(detail), true);
            // shortcut: unavailable recovery UI or abrupt host death needs manual Windows desktop recovery.
            if (!retry.ok()) { result = restore; break; }
        }
    }
    if (!SetThreadDesktop(originalThread) && result.ok()) result = failure("thread_restore_failed", GetLastError());
    restoreThread.dismiss();
    return result;
}

unsigned __stdcall pairingThread(void *argument)
{
    const auto holder = std::unique_ptr<std::shared_ptr<DemoPairingState>>(static_cast<std::shared_ptr<DemoPairingState> *>(argument));
    (*holder)->result = runPairing(**holder);
    return 0;
}
} // namespace

DemoPairingSession::DemoPairingSession(QObject *parent) : QObject(parent)
{
    connect(&m_done, &QWinEventNotifier::activated, this, [this] {
        if (!m_thread || WaitForSingleObject(m_thread, 0) != WAIT_OBJECT_0) return;
        m_done.setEnabled(false); m_done.setHandle(nullptr);
        m_result = m_state->result;
        CloseHandle(m_thread); m_thread = nullptr; m_state.reset();
        emit finished();
    });
}

DemoPairingSession::~DemoPairingSession()
{
    cancel();
    m_done.setEnabled(false); m_done.setHandle(nullptr);
    // Keep the original desktop available until the worker has handled return or explicit recovery.
    if (m_thread) { WaitForSingleObject(m_thread, INFINITE); CloseHandle(m_thread); }
}

void DemoPairingSession::cancel()
{
    if (m_state && !SetEvent(m_state->cancel)) m_result = failure("setup_cancel_failed", GetLastError());
}

bool DemoPairingSession::start(DemoProcess &owner, const QString &title, const QString &instructions, const QString &retry)
{
    return startImpl(owner, title, instructions, retry, true, 15 * 60 * 1000);
}

bool DemoPairingSession::startImpl(DemoProcess &owner, const QString &title, const QString &instructions,
    const QString &retry, bool display, DWORD timeout)
{
    if (isRunning()) { m_result = failure("setup_busy"); return false; }
    m_result = {};
    const auto reject = [this](const char *code, DWORD error = 0) { m_result = failure(code, error); return false; };
    if (!configurationOnlyDemo() || !owner.running() || !owner.target().isValid()) return reject("setup_unavailable");
    wchar_t station[256]{}, thread[256]{};
    DWORD error = 0;
    if (!userObjectName(GetProcessWindowStation(), station, error) || _wcsicmp(station, L"WinSta0") != 0)
        return reject("window_station_unavailable", error);
    if (display && (!userObjectName(GetThreadDesktop(GetCurrentThreadId()), thread, error) || _wcsicmp(thread, L"Default") != 0))
        return reject("default_input_required", error);
    auto state = std::make_shared<DemoPairingState>();
    state->target = owner.target(); state->executable = owner.executable(); state->desktopName = owner.desktop();
    state->title = title; state->instructions = instructions; state->retry = retry; state->display = display; state->timeout = timeout;
    if (display) {
        state->original = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP);
        if (!state->original) return reject("input_desktop_unavailable", GetLastError());
        wchar_t originalName[256]{};
        if (!userObjectName(state->original, originalName, error) || _wcsicmp(originalName, L"Default") != 0)
            return reject("default_input_required", error);
        if (inputDesktop(*state, error) != Input::Original) return reject("default_input_required", error);
        const auto management = owner.requestManagement();
        if (!management.ok()) { m_result = management; return false; }
    }
    if (!DuplicateHandle(GetCurrentProcess(), owner.processHandle(), GetCurrentProcess(), &state->process,
            SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, 0)) return reject("target_handle_failed", GetLastError());
    state->cancel = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!state->cancel) return reject("cancel_event_failed", GetLastError());
    state->desktop = OpenDesktopW(reinterpret_cast<LPCWSTR>(state->desktopName.utf16()), 0, FALSE,
        DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | (display ? DESKTOP_SWITCHDESKTOP : 0));
    if (!state->desktop) return reject("owned_desktop_unavailable", GetLastError());
    auto holder = std::make_unique<std::shared_ptr<DemoPairingState>>(state);
    _set_doserrno(0);
    m_thread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, pairingThread, holder.get(), 0, nullptr));
    if (!m_thread) { unsigned long native = 0; _get_doserrno(&native); return reject("setup_thread_failed", DWORD(native)); }
    holder.release(); m_state = state;
    m_done.setHandle(m_thread); m_done.setEnabled(true);
    return true;
}

#ifdef QKM_AUTOMATION_TESTING
QJsonObject DemoPairingSession::workerForTest() const
{
    if (!m_thread) return {{QStringLiteral("running"), false}};
    const DWORD wait = WaitForSingleObject(m_thread, 0);
    DWORD code = 0;
    const bool known = GetExitCodeThread(m_thread, &code) != FALSE;
    const DWORD error = known ? 0 : GetLastError();
    return {{QStringLiteral("running"), true}, {QStringLiteral("thread_id"), int(GetThreadId(m_thread))},
        {QStringLiteral("wait_result"), int(wait)}, {QStringLiteral("notifier_enabled"), m_done.isEnabled()},
        {QStringLiteral("exit_code"), known ? QJsonValue(int(code)) : QJsonValue(QJsonValue::Null)},
        {QStringLiteral("native_error"), error ? QJsonValue(int(error)) : QJsonValue(QJsonValue::Null)}};
}

bool DemoPairingSession::startPromptForTest(DemoProcess &owner, const QString &title, const QString &text, DWORD timeout)
{
    if (!currentThreadOnPrivateDesktop()) { m_result = failure("private_test_required"); return false; }
    return startImpl(owner, title, text, QString(), false, timeout);
}
#endif

} // namespace QkmAutomation
