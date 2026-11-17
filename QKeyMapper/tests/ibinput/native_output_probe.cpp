#define QKM_OUTPUT_PROBE_IMPLEMENTATION
#include "native_output_probe.h"
#include "../../qkm_ibinput_client.h"
#include <QMutex>
#include <QMutexLocker>
#include <atomic>
#include <cstring>
#include <functional>

namespace {
std::atomic<int> initCalls{0}, ibInputCalls{0}, destroyCalls{0}, sendInputCalls{0}, cursorCalls{0};
POINT cursor{};
quint64 clockTicks = 0;

struct SendInputProbeState {
    QMutex mutex;
    QList<WORD> downVks;
    QList<WORD> upVks;
    WORD breakKey = 0;
    int breakValue = 0;
    bool breakOnDown = false;
    WORD failNextUpKey = 0;
};

// Kept outside SendInputProbeState to satisfy strict clang-analyzer-webkit checks; always guarded by sendInputState().mutex.
QAtomicInt *sendInputBreakFlag = nullptr;

SendInputProbeState &sendInputState()
{
    static SendInputProbeState state;
    return state;
}

#ifdef FAKERINPUT_SUPPORT
struct FakerInputProbeState {
    QMutex mutex;
    std::atomic<int> keyboardUpdates{0};
    std::atomic<int> relativeMouseUpdates{0};
    std::atomic<int> absoluteMouseUpdates{0};
    BYTE lastShiftFlags = 0;
    BYTE lastKeyCodes[KBD_KEY_CODES]{};
    BYTE lastMouseButtons = 0;
    QkmNativeOutputProbe::FakerInputKeyboardObserver keyboardObserver = nullptr;
    QkmNativeOutputProbe::FakerInputMouseObserver mouseObserver = nullptr;
    int dummyClientToken = 0;
};

FakerInputProbeState &fakerInputState()
{
    static FakerInputProbeState state;
    return state;
}
#endif
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
UINT WINAPI qkmProbeSendInput(UINT count, LPINPUT inputs, int) {
    ++sendInputCalls;
    auto &state = sendInputState();
    QMutexLocker locker(&state.mutex);
    if (state.failNextUpKey != 0 && inputs && count == 1 && inputs[0].type == INPUT_KEYBOARD
        && (inputs[0].ki.dwFlags & KEYEVENTF_KEYUP) && state.failNextUpKey == inputs[0].ki.wVk) {
        state.failNextUpKey = 0;
        return 0;
    }
    for (UINT index = 0; inputs && index < count; ++index) {
        if (inputs[index].type != INPUT_KEYBOARD) {
            continue;
        }
        const bool isKeyUp = (inputs[index].ki.dwFlags & KEYEVENTF_KEYUP) != 0;
        if (isKeyUp) {
            state.upVks.append(inputs[index].ki.wVk);
        } else {
            state.downVks.append(inputs[index].ki.wVk);
        }
        if (sendInputBreakFlag && inputs[index].ki.wVk == state.breakKey && (isKeyUp != state.breakOnDown)) {
            QAtomicInt *const targetFlag = sendInputBreakFlag;
            sendInputBreakFlag = nullptr;
            targetFlag->storeRelease(state.breakValue);
        }
    }
    return count;
}
BOOL WINAPI qkmProbeSetCursorPos(int, int) { ++cursorCalls; return TRUE; }

#ifdef FAKERINPUT_SUPPORT
// Fixed substitutes for the entire opt-in probe lifetime, including startup and teardown.
pfakerinput_client qkmProbeFakerInputAlloc()
{
    return reinterpret_cast<pfakerinput_client>(&fakerInputState().dummyClientToken);
}

void qkmProbeFakerInputFree(pfakerinput_client clientHandle) { Q_UNUSED(clientHandle); }
bool qkmProbeFakerInputConnect(pfakerinput_client clientHandle) { return clientHandle != nullptr; }
void qkmProbeFakerInputDisconnect(pfakerinput_client clientHandle) { Q_UNUSED(clientHandle); }

bool qkmProbeFakerInputUpdateKeyboard(pfakerinput_client clientHandle, BYTE shiftKeyFlags, BYTE keyCodes[KBD_KEY_CODES])
{
    auto &state = fakerInputState();
    if (clientHandle == nullptr) {
        return false;
    }
    BYTE copiedKeyCodes[KBD_KEY_CODES]{};
    if (keyCodes != nullptr) {
        std::memcpy(copiedKeyCodes, keyCodes, sizeof(copiedKeyCodes));
    }
    QkmNativeOutputProbe::FakerInputKeyboardObserver observer = nullptr;
    {
        QMutexLocker locker(&state.mutex);
        state.lastShiftFlags = shiftKeyFlags;
        std::memcpy(state.lastKeyCodes, copiedKeyCodes, sizeof(copiedKeyCodes));
        observer = state.keyboardObserver;
    }
    ++state.keyboardUpdates;
    if (observer != nullptr) {
        observer(shiftKeyFlags, copiedKeyCodes);
    }
    return true;
}

bool qkmProbeFakerInputUpdateRelativeMouse(pfakerinput_client clientHandle, BYTE button,
    SHORT x, SHORT y, BYTE wheelPosition, BYTE hWheelPosition)
{
    auto &state = fakerInputState();
    Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(wheelPosition); Q_UNUSED(hWheelPosition);
    if (clientHandle == nullptr) {
        return false;
    }
    QkmNativeOutputProbe::FakerInputMouseObserver observer = nullptr;
    {
        QMutexLocker locker(&state.mutex);
        state.lastMouseButtons = button;
        observer = state.mouseObserver;
    }
    ++state.relativeMouseUpdates;
    if (observer != nullptr) {
        observer(button);
    }
    return true;
}

bool qkmProbeFakerInputUpdateAbsoluteMouse(pfakerinput_client clientHandle, BYTE button,
    USHORT x, USHORT y, BYTE wheelPosition, BYTE hWheelPosition)
{
    auto &state = fakerInputState();
    Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(wheelPosition); Q_UNUSED(hWheelPosition);
    if (clientHandle == nullptr) {
        return false;
    }
    QkmNativeOutputProbe::FakerInputMouseObserver observer = nullptr;
    {
        QMutexLocker locker(&state.mutex);
        state.lastMouseButtons = button;
        observer = state.mouseObserver;
    }
    ++state.absoluteMouseUpdates;
    if (observer != nullptr) {
        observer(button);
    }
    return true;
}
#endif

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

namespace {
void setBreakRule(SendInputProbeState &state, QAtomicInt *flag, WORD virtualKey, int stopValue, bool onDown)
{
    QMutexLocker locker(&state.mutex);
    sendInputBreakFlag = flag;
    state.breakKey = virtualKey;
    state.breakValue = stopValue;
    state.breakOnDown = onDown;
}
}

namespace QkmNativeOutputProbe {
void breakAfterUp(QAtomicInt *flag, WORD virtualKey, int stopValue)
{
    setBreakRule(sendInputState(), flag, virtualKey, stopValue, false);
}

void breakAfterDown(QAtomicInt *flag, WORD virtualKey, int stopValue)
{
    setBreakRule(sendInputState(), flag, virtualKey, stopValue, true);
}

void failNextSendInputUp(WORD virtualKey)
{
    auto &state = sendInputState();
    QMutexLocker locker(&state.mutex);
    state.failNextUpKey = virtualKey;
}

int sendInputDownCount(WORD virtualKey)
{
    auto &state = sendInputState();
    QMutexLocker locker(&state.mutex);
    return static_cast<int>(state.downVks.count(virtualKey));
}

int sendInputUpCount(WORD virtualKey)
{
    auto &state = sendInputState();
    QMutexLocker locker(&state.mutex);
    return static_cast<int>(state.upVks.count(virtualKey));
}

void reset()
{
    {
        auto &state = sendInputState();
        QMutexLocker locker(&state.mutex);
        sendInputBreakFlag = nullptr;
        state.breakKey = 0;
        state.breakValue = 0;
        state.breakOnDown = false;
        state.failNextUpKey = 0;
        state.downVks.clear();
        state.upVks.clear();
    }
    initCalls = 0; ibInputCalls = 0; destroyCalls = 0; sendInputCalls = 0; cursorCalls = 0; cursor = {}; clockTicks = 0;
}

#ifdef FAKERINPUT_SUPPORT
void resetFakerInput()
{
    auto &state = fakerInputState();
    QMutexLocker locker(&state.mutex);
    state.keyboardUpdates = 0;
    state.relativeMouseUpdates = 0;
    state.absoluteMouseUpdates = 0;
    state.lastShiftFlags = 0;
    std::memset(state.lastKeyCodes, 0, sizeof(state.lastKeyCodes));
    state.lastMouseButtons = 0;
}

void setFakerInputObservers(FakerInputKeyboardObserver keyboardObserver, FakerInputMouseObserver mouseObserver)
{
    auto &state = fakerInputState();
    QMutexLocker locker(&state.mutex);
    state.keyboardObserver = keyboardObserver;
    state.mouseObserver = mouseObserver;
}

QJsonObject fakerInputCounts()
{
    auto &state = fakerInputState();
    QMutexLocker locker(&state.mutex);
    int activeKeys = 0;
    for (BYTE code : state.lastKeyCodes) {
        if (code != 0) {
            ++activeKeys;
        }
    }
    return {
        {QStringLiteral("keyboardUpdates"), state.keyboardUpdates.load()},
        {QStringLiteral("relativeMouseUpdates"), state.relativeMouseUpdates.load()},
        {QStringLiteral("absoluteMouseUpdates"), state.absoluteMouseUpdates.load()},
        {QStringLiteral("lastShiftFlags"), int(state.lastShiftFlags)},
        {QStringLiteral("activeKeyCodes"), activeKeys},
        {QStringLiteral("lastMouseButtons"), int(state.lastMouseButtons)}
    };
}

BYTE lastFakerInputShiftFlags()
{
    auto &state = fakerInputState();
    QMutexLocker locker(&state.mutex);
    return state.lastShiftFlags;
}

QList<BYTE> lastFakerInputKeyCodes()
{
    auto &state = fakerInputState();
    QMutexLocker locker(&state.mutex);
    QList<BYTE> result;
    result.reserve(KBD_KEY_CODES);
    for (BYTE code : state.lastKeyCodes) {
        result.append(code);
    }
    return result;
}

BYTE lastFakerInputMouseButtons()
{
    auto &state = fakerInputState();
    QMutexLocker locker(&state.mutex);
    return state.lastMouseButtons;
}
#endif

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
