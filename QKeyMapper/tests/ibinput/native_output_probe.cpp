#define QKM_OUTPUT_PROBE_IMPLEMENTATION
#include "native_output_probe.h"
#include "../../qkm_ibinput_client.h"
#include <atomic>
#include <functional>

namespace {
std::atomic<int> initCalls{0}, ibInputCalls{0}, destroyCalls{0}, sendInputCalls{0}, cursorCalls{0};
POINT cursor{};
quint64 clockTicks = 0;
}
Send::Error __stdcall qkmProbeIbSendInit(Send::SendType, Send::InitFlags, void *) { ++initCalls; return Send::Error::Success; }
void __stdcall qkmProbeIbSendDestroy() { ++destroyCalls; }
UINT WINAPI qkmProbeIbSendInput(UINT count, LPINPUT inputs, int size)
{
    ++ibInputCalls;
    if (!inputs || size != sizeof(INPUT)) return 0;
    for (UINT index = 0; index < count; ++index) {
        const auto &input = inputs[index];
        if (input.type == INPUT_MOUSE && (input.mi.dwFlags & MOUSEEVENTF_MOVE) && !(input.mi.dwFlags & MOUSEEVENTF_ABSOLUTE)) {
            cursor.x += input.mi.dx; cursor.y += input.mi.dy;
        }
    }
    return count;
}
UINT WINAPI qkmProbeSendInput(UINT count, LPINPUT, int) { ++sendInputCalls; return count; }
BOOL WINAPI qkmProbeSetCursorPos(int, int) { ++cursorCalls; return TRUE; }

namespace QkmIbInputSystem {
BOOL cursorPosition(LPPOINT point) { *point = cursor; return TRUE; }
int metric(int index) {
    if (index == SM_CXSCREEN || index == SM_CXVIRTUALSCREEN) return 1920;
    if (index == SM_CYSCREEN || index == SM_CYVIRTUALSCREEN) return 1080;
    return 0;
}
quint64 ticks() { return clockTicks; }
void pause() { ++clockTicks; }
}

// Seed a valid admitted transaction so zero output cannot be explained by a stale generation.
struct QkmIbInputDemoTestAccess {
    static void seed(QkmIbInputClient &client, bool connected, bool fallback = false, bool ghub = false) {
        client.m_status = {}; client.m_published = {};
        client.m_status.connected = connected;
        client.m_status.fallback = fallback;
        client.m_status.generation = 1;
        client.m_status.backend = ghub ? QStringLiteral("logitech_ghub") : QStringLiteral("razer");
        client.m_requested.store(1); client.m_acceptingGeneration.store(1); client.m_stopGeneration.store(0);
        client.m_runCapabilities = 63;
        client.m_ownedKeys.clear(); client.m_ownedButtons.clear();
        client.publish();
    }
    static void clear(QkmIbInputClient &client) {
        client.m_status = {}; client.m_published = {}; client.m_requested.store(0);
        client.m_acceptingGeneration.store(0); client.m_stopGeneration.store(0);
        client.m_runCapabilities = 0; client.m_ownedKeys.clear(); client.m_ownedButtons.clear();
    }
    static void step(QkmIbInputClient &client) { (void)client.moveGHubTo(POINT{12, 10}, 1, false); }
};

namespace QkmNativeOutputProbe {
void reset() { initCalls = 0; ibInputCalls = 0; destroyCalls = 0; sendInputCalls = 0; cursorCalls = 0; cursor = {}; clockTicks = 0; }
QJsonObject counts() {
    return {{QStringLiteral("IbSendInit"), initCalls.load()}, {QStringLiteral("IbSendInput"), ibInputCalls.load()},
        {QStringLiteral("IbSendDestroy"), destroyCalls.load()}, {QStringLiteral("SendInput"), sendInputCalls.load()},
        {QStringLiteral("SetCursorPos"), cursorCalls.load()}};
}
QJsonArray exercise(bool demo)
{
    QJsonArray rows;
    auto &client = QkmIbInputClient::instance();
    const auto run = [&](const char *name, const char *positiveApi, bool connected, bool fallback, bool ghub,
                         const std::function<void()> &action) {
        QkmIbInputDemoTestAccess::seed(client, connected, fallback, ghub);
        reset(); action();
        const auto observed = counts();
        bool passed = true;
        if (demo) { for (const auto &value : observed) passed = passed && value.toInt() == 0; }
        else passed = observed.value(QString::fromLatin1(positiveApi)).toInt() > 0;
        rows.append(QJsonObject{{QStringLiteral("scenario"), QString::fromLatin1(name)},
            {QStringLiteral("native_boundary_calls"), observed}, {QStringLiteral("passed"), passed}});
        QkmIbInputDemoTestAccess::clear(client);
    };
    INPUT key{}; key.type = INPUT_KEYBOARD; key.ki.wVk = 'A';
    INPUT move{}; move.type = INPUT_MOUSE; move.mi.dwFlags = MOUSEEVENTF_MOVE;
    move.mi.dx = 12; move.mi.dy = 10;
    INPUT absolute = move; absolute.mi.dwFlags |= MOUSEEVENTF_ABSOLUTE;
    absolute.mi.dx = absolute.mi.dy = 32767;
    run("init", "IbSendInit", false, false, false, [&] {
        QkmIbInputClient::Requirements needs; needs.capabilities = QkmIbInputClient::Keyboard; needs.keys.insert('A');
        (void)client.prepareRun(1, needs);
    });
    run("keyboard", "IbSendInput", true, false, false, [&] { (void)client.sendEvent(key, 1); });
    run("pixel_absolute", "IbSendInput", true, false, true, [&] { (void)client.sendAbsoluteEvent(absolute, POINT{12, 10}, 1); });
    run("normalized_absolute", "IbSendInput", true, false, true, [&] { (void)client.sendEvent(absolute, 1); });
    run("relative", "IbSendInput", true, false, true, [&] { (void)client.sendEvent(move, 1); });
    run("internal_absolute_step", "IbSendInput", true, false, true, [&] { QkmIbInputDemoTestAccess::step(client); });
    run("worker_sendinput_fallback", "SendInput", false, true, false, [&] { qkmProbeWorkerIbOutput(0, 1); });
    run("worker_pixel_fallback", "SendInput", false, true, false, [&] { qkmProbeWorkerIbOutput(1, 1); });
    run("worker_relative", "IbSendInput", true, false, true, [&] { qkmProbeWorkerIbOutput(2, 1); });
    run("close", "IbSendDestroy", true, false, false, [&] { client.cancelRun(1); (void)client.closeAfterDrain(1); });
    // SetCursorPos has no call site in the affected path; calibrate its individual counter explicitly.
    if (!demo) {
        reset(); (void)qkmProbeSetCursorPos(0, 0);
        rows.append(QJsonObject{{QStringLiteral("scenario"), QStringLiteral("cursor_counter_calibration")},
            {QStringLiteral("native_boundary_calls"), counts()}, {QStringLiteral("passed"), cursorCalls == 1}});
    }
    reset();
    return rows;
}
}
