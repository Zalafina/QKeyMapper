#include <QRandomGenerator>
#include "qkeymapper.h"
#include "qkeymapper_worker.h"

using namespace QKeyMapperConstants;

QMutex SendInputTask::s_SendInputTaskControllerMapMutex;
QHash<QString, SendInputTaskController> SendInputTask::s_SendInputTaskControllerMap;
SendInputTaskController SendInputTask::s_GlobalSendInputTaskController;

ULONG_PTR QKeyMapper_Worker::VIRTUAL_KEY_SEND = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_KEY_SEND_NORMAL = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_KEY_SEND_FORCE = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_KEY_SEND_MODIFIERS = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_KEY_SEND_BURST_TIMEOUT = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_KEY_SEND_BURST_STOP = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_KEY_SEND_KEYSEQ_NORMAL = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_KEY_SEND_KEYSEQ_HOLDDOWN = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_KEY_SEND_KEYSEQ_REPEAT = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_MOUSE_POINTCLICK = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_MOUSE_WHEEL = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_KEY_OVERLAY = 0;
ULONG_PTR QKeyMapper_Worker::VIRTUAL_RESEND_REALKEY = 0;
bool QKeyMapper_Worker::s_isWorkerDestructing = false;
QAtomicInt QKeyMapper_Worker::s_AtomicHookProcState = HOOKPROC_STATE_STOPPED;
QAtomicBool QKeyMapper_Worker::s_Mouse2vJoy_Hold;
QAtomicBool QKeyMapper_Worker::s_Gyro2Mouse_MoveActive;
QAtomicBool QKeyMapper_Worker::s_Crosshair_Normal;
QAtomicBool QKeyMapper_Worker::s_Crosshair_TypeA;
QAtomicBool QKeyMapper_Worker::s_Key2Mouse_Up;
QAtomicBool QKeyMapper_Worker::s_Key2Mouse_Down;
QAtomicBool QKeyMapper_Worker::s_Key2Mouse_Left;
QAtomicBool QKeyMapper_Worker::s_Key2Mouse_Right;
QAtomicBool QKeyMapper_Worker::s_KeyRecording;
QAtomicBool QKeyMapper_Worker::s_RestoreFilterKeysState;
qint32 QKeyMapper_Worker::s_LastCarOrdinal = 0;
QHash<QString, V_KEYCODE> QKeyMapper_Worker::VirtualKeyCodeMap = QHash<QString, V_KEYCODE>();
QHash<QString, V_MOUSECODE> QKeyMapper_Worker::VirtualMouseButtonMap = QHash<QString, V_MOUSECODE>();
QHash<WPARAM, QString> QKeyMapper_Worker::MouseButtonNameMap = QHash<WPARAM, QString>();
#ifdef MOUSEBUTTON_CONVERT
QHash<QString, QString> QKeyMapper_Worker::MouseButtonNameConvertMap = QHash<QString, QString>();
#endif
QStringList QKeyMapper_Worker::MultiKeyboardInputList = QStringList();
QStringList QKeyMapper_Worker::MultiMouseInputList = QStringList();
QStringList QKeyMapper_Worker::MultiVirtualGamepadInputList;
QStringList QKeyMapper_Worker::CombinationKeysList = QStringList();
QStringList QKeyMapper_Worker::SpecialOriginalKeysList;
QStringList QKeyMapper_Worker::SpecialMappingKeysList;
QList<quint8> QKeyMapper_Worker::SpecialVirtualKeyCodeList;
// QStringList QKeyMapper_Worker::skipReleaseModifiersKeysList = QStringList();
// QHash<QString, int> QKeyMapper_Worker::JoyStickKeyMap = QHash<QString, int>();
// QHash<QString, QHotkey*> QKeyMapper_Worker::ShortcutsMap = QHash<QString, QHotkey*>();
#ifdef VIGEM_CLIENT_SUPPORT
QHash<QString, XUSB_BUTTON> QKeyMapper_Worker::ViGEmButtonMap = QHash<QString, XUSB_BUTTON>();
#endif
QStringList QKeyMapper_Worker::pressedRealKeysList = QStringList();
QStringList QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput;
QList<RecordKeyData> QKeyMapper_Worker::recordKeyList;
QStringList QKeyMapper_Worker::recordMappingKeysList;
QElapsedTimer QKeyMapper_Worker::recordElapsedTimer;
// QStringList QKeyMapper_Worker::pressedCombinationRealKeysList;
QStringList QKeyMapper_Worker::pressedVirtualKeysList = QStringList();
QStringList QKeyMapper_Worker::pressedLongPressKeysList;
QStringList QKeyMapper_Worker::pressedDoublePressKeysList;
QList<QList<quint8>> QKeyMapper_Worker::pressedMultiKeyboardVKeyCodeList;
QStringList QKeyMapper_Worker::s_runningKeySequenceOrikeyList;
// QStringList QKeyMapper_Worker::pressedShortcutKeysList = QStringList();
QStringList QKeyMapper_Worker::combinationOriginalKeysList;
QStringList QKeyMapper_Worker::blockedKeysList;
QHash<QString, QList<int>> QKeyMapper_Worker::longPressOriginalKeysMap;
QHash<QString, QTimer*> QKeyMapper_Worker::s_longPressTimerMap;
QHash<QString, int> QKeyMapper_Worker::doublePressOriginalKeysMap;
QHash<QString, QTimer*> QKeyMapper_Worker::s_doublePressTimerMap;
QHash<QString, QTimer*> QKeyMapper_Worker::s_BurstKeyTimerMap;
QHash<QString, QTimer*> QKeyMapper_Worker::s_BurstKeyPressTimerMap;
QHash<QString, int> QKeyMapper_Worker::s_KeySequenceRepeatCount;
#ifdef VIGEM_CLIENT_SUPPORT
QList<OrderedMap<QString, BYTE>> QKeyMapper_Worker::pressedvJoyLStickKeysList;
QList<OrderedMap<QString, BYTE>> QKeyMapper_Worker::pressedvJoyRStickKeysList;
QList<QStringList> QKeyMapper_Worker::pressedvJoyButtonsList;
#endif
QHash<QString, QStringList> QKeyMapper_Worker::pressedMappingKeysMap;
QMutex QKeyMapper_Worker::s_PressedMappingKeysMapMutex;
QHash<QString, int> QKeyMapper_Worker::pressedLockKeysMap;
QStringList QKeyMapper_Worker::exchangeKeysList = QStringList();
QMutex QKeyMapper_Worker::s_BurstKeyTimerMutex;
#ifdef DINPUT_TEST
GetDeviceStateT QKeyMapper_Worker::FuncPtrGetDeviceState = Q_NULLPTR;
GetDeviceDataT QKeyMapper_Worker::FuncPtrGetDeviceData = Q_NULLPTR;
int QKeyMapper_Worker::dinput_timerid = 0;
#endif
#ifdef VIGEM_CLIENT_SUPPORT
PVIGEM_CLIENT QKeyMapper_Worker::s_ViGEmClient = Q_NULLPTR;
// PVIGEM_TARGET QKeyMapper_Worker::s_ViGEmTarget = Q_NULLPTR;
QList<PVIGEM_TARGET> QKeyMapper_Worker::s_ViGEmTargetList;
// XUSB_REPORT QKeyMapper_Worker::s_ViGEmTarget_Report = XUSB_REPORT();
QList<ViGEm_ReportData> QKeyMapper_Worker::s_ViGEmTarget_ReportList;
QStringList QKeyMapper_Worker::s_VirtualGamepadList = QStringList() << VIRTUAL_GAMEPAD_X360;
BYTE QKeyMapper_Worker::s_Auto_Brake = AUTO_BRAKE_DEFAULT;
BYTE QKeyMapper_Worker::s_Auto_Accel = AUTO_ACCEL_DEFAULT;
BYTE QKeyMapper_Worker::s_last_Auto_Brake = 0;
BYTE QKeyMapper_Worker::s_last_Auto_Accel = 0;
QKeyMapper_Worker::GripDetectStates QKeyMapper_Worker::s_GripDetect_EnableState = QKeyMapper_Worker::GRIPDETECT_NONE;
// QKeyMapper_Worker::Joy2vJoyState QKeyMapper_Worker::s_Joy2vJoyState = Joy2vJoyState();
QHash<int, QKeyMapper_Worker::Joy2vJoyState> QKeyMapper_Worker::s_Joy2vJoy_EnableStateMap;
QKeyMapper_Worker::ViGEmClient_ConnectState QKeyMapper_Worker::s_ViGEmClient_ConnectState = VIGEMCLIENT_DISCONNECTED;
QMutex QKeyMapper_Worker::s_ViGEmClient_Mutex;
QPoint QKeyMapper_Worker::s_Mouse2vJoy_delta = QPoint();
QPoint QKeyMapper_Worker::s_Mouse2vJoy_prev = QPoint();
// QList<QPoint> QKeyMapper_Worker::s_Mouse2vJoy_delta_List;
// QPoint QKeyMapper_Worker::s_Mouse2vJoy_delta_interception = QPoint();
// QKeyMapper_Worker::Mouse2vJoyStates QKeyMapper_Worker::s_Mouse2vJoy_EnableState = QKeyMapper_Worker::MOUSE2VJOY_NONE;
QHash<int, QKeyMapper_Worker::Mouse2vJoyData> QKeyMapper_Worker::s_Mouse2vJoy_EnableStateMap;
// QMutex QKeyMapper_Worker::s_MouseMove_delta_List_Mutex;
#endif
bool QKeyMapper_Worker::s_Key2Mouse_EnableState = false;
bool QKeyMapper_Worker::s_GameControllerSensor_EnableState = false;
// QKeyMapper_Worker::Joy2MouseStates QKeyMapper_Worker::s_Joy2Mouse_EnableState = QKeyMapper_Worker::JOY2MOUSE_NONE;
QHash<int, QKeyMapper_Worker::Joy2MouseStates> QKeyMapper_Worker::s_Joy2Mouse_EnableStateMap;
// Joystick_AxisState QKeyMapper_Worker::s_JoyAxisState = Joystick_AxisState();
QHash<int, Joystick_AxisState> QKeyMapper_Worker::s_JoyAxisStateMap;
int QKeyMapper_Worker::s_LastJoyAxisPlayerIndex = INITIAL_PLAYER_INDEX;

bool QKeyMapper_Hook_Proc::s_LowLevelKeyboardHook_Enable = true;
bool QKeyMapper_Hook_Proc::s_LowLevelMouseHook_Enable = true;

HHOOK QKeyMapper_Hook_Proc::s_KeyHook = Q_NULLPTR;
HHOOK QKeyMapper_Hook_Proc::s_MouseHook = Q_NULLPTR;

QKeyMapper_Worker::QKeyMapper_Worker(QObject *parent) :
    m_KeyHook(Q_NULLPTR),
    m_MouseHook(Q_NULLPTR),
#ifdef VIGEM_CLIENT_SUPPORT
    m_LastMouseCursorPoint(),
#endif
    m_JoystickCapture(false),
#ifdef DINPUT_TEST
    m_DirectInput(Q_NULLPTR),
#endif
#ifdef VIGEM_CLIENT_SUPPORT
    m_Mouse2vJoyResetTimerMap(),
#endif
    m_Key2MouseCycleTimer(this),
    m_UdpSocket(Q_NULLPTR),
//     m_BurstTimerMap(),
//     m_BurstKeyUpTimerMap(),
// #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
//     m_BurstTimerMutex(),
// #else
//     m_BurstTimerMutex(QMutex::Recursive),
// #endif
    m_JoystickButtonMap(),
    m_JoystickDPadMap(),
    m_JoystickLStickMap(),
    m_JoystickRStickMap(),
    m_JoystickPOVMap(),
    m_GamdpadMotion()
{
    qRegisterMetaType<HWND>("HWND");
    qRegisterMetaType<V_KEYCODE>("V_KEYCODE");
    qRegisterMetaType<V_MOUSECODE>("V_MOUSECODE");
    qRegisterMetaType<QJoystickPOVEvent>("QJoystickPOVEvent");
    qRegisterMetaType<QJoystickAxisEvent>("QJoystickAxisEvent");
    qRegisterMetaType<QJoystickButtonEvent>("QJoystickButtonEvent");
    qRegisterMetaType<Qt::KeyboardModifiers>("Qt::KeyboardModifiers");

    Q_UNUSED(parent);

    for (int i = 0; i < INTERCEPTION_MAX_KEYBOARD; ++i) {
        pressedMultiKeyboardVKeyCodeList.append(QList<quint8>());
    }
    for (int i = 0; i < VIRTUAL_GAMEPAD_NUMBER_MAX; ++i) {
        pressedvJoyLStickKeysList.append(OrderedMap<QString, BYTE>());
        pressedvJoyRStickKeysList.append(OrderedMap<QString, BYTE>());
        pressedvJoyButtonsList.append(QStringList());
    }
    // for (int i = 0; i < INTERCEPTION_MAX_MOUSE; ++i) {
    //     s_Mouse2vJoy_delta_List.append(QPoint());
    // }

    QObject::connect(this, &QKeyMapper_Worker::setKeyHook_Signal, this, &QKeyMapper_Worker::setWorkerKeyHook, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::setKeyUnHook_Signal, this, &QKeyMapper_Worker::setWorkerKeyUnHook, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::setKeyMappingRestart_Signal, this, &QKeyMapper_Worker::setKeyMappingRestart, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::allMappingKeysReleased_Signal, this, &QKeyMapper_Worker::allMappingKeysReleased, Qt::QueuedConnection);
    QObject::connect(QKeyMapper_Hook_Proc::getInstance(), &QKeyMapper_Hook_Proc::setKeyHook_Signal, QKeyMapper_Hook_Proc::getInstance(), &QKeyMapper_Hook_Proc::onSetHookProcKeyHook, Qt::QueuedConnection);
    QObject::connect(QKeyMapper_Hook_Proc::getInstance(), &QKeyMapper_Hook_Proc::setKeyUnHook_Signal, QKeyMapper_Hook_Proc::getInstance(), &QKeyMapper_Hook_Proc::onSetHookProcKeyUnHook, Qt::QueuedConnection);
    QObject::connect(QKeyMapper_Hook_Proc::getInstance(), &QKeyMapper_Hook_Proc::setKeyMappingRestart_Signal, QKeyMapper_Hook_Proc::getInstance(), &QKeyMapper_Hook_Proc::onSetHookProcKeyMappingRestart, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::sessionLockStateChanged_Signal, this, &QKeyMapper_Worker::sessionLockStateChanged, Qt::QueuedConnection);
    // QObject::connect(this, &QKeyMapper_Worker::startBurstTimer_Signal, this, &QKeyMapper_Worker::startBurstTimer, Qt::QueuedConnection);
    // QObject::connect(this, &QKeyMapper_Worker::stopBurstTimer_Signal, this, &QKeyMapper_Worker::stopBurstTimer, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::startBurstKeyTimer_Signal, this, &QKeyMapper_Worker::startBurstKeyTimer, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::stopBurstKeyTimer_Signal, this, &QKeyMapper_Worker::stopBurstKeyTimer, Qt::QueuedConnection);
#if 0
    QObject::connect(this, SIGNAL(sendKeyboardInput_Signal(V_KEYCODE,int)), this, SLOT(sendKeyboardInput(V_KEYCODE,int)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(sendMouseClick_Signal(V_MOUSECODE,int)), this, SLOT(sendMouseClick(V_MOUSECODE,int)), Qt::QueuedConnection);
#endif
    QObject::connect(this, &QKeyMapper_Worker::sendInputKeys_Signal, this, &QKeyMapper_Worker::onSendInputKeys, Qt::QueuedConnection);
    // QObject::connect(this, &QKeyMapper_Worker::send_WINplusD_Signal, this, &QKeyMapper_Worker::send_WINplusD, Qt::QueuedConnection);
    // QObject::connect(this, &QKeyMapper_Worker::HotKeyTrigger_Signal, this, &QKeyMapper_Worker::HotKeyHookProc, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::doFunctionMappingProc_Signal, this, &QKeyMapper_Worker::doFunctionMappingProc, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::gameControllerGyroEnabledSwitch_Signal, this, &QKeyMapper_Worker::onGameControllerGyroEnabledSwitch, Qt::QueuedConnection);
#if 0
    QObject::connect(this, &QKeyMapper_Worker::sendSpecialVirtualKey_Signal, this, &QKeyMapper_Worker::sendSpecialVirtualKey, Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(onMouseWheel_Signal(int)), this, SLOT(onMouseWheel(int)), Qt::QueuedConnection);
#endif
#ifdef VIGEM_CLIENT_SUPPORT
    QObject::connect(this, &QKeyMapper_Worker::onMouseMove_Signal, this, &QKeyMapper_Worker::onMouseMove, Qt::QueuedConnection);

    QObject::connect(this, &QKeyMapper_Worker::startMouse2vJoyResetTimer_Signal, this, &QKeyMapper_Worker::startMouse2vJoyResetTimer, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::stopMouse2vJoyResetTimer_Signal, this, &QKeyMapper_Worker::stopMouse2vJoyResetTimer, Qt::QueuedConnection);

    initMouse2vJoyResetTimerMap();
#endif

    m_Key2MouseCycleTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&m_Key2MouseCycleTimer, &QTimer::timeout, this, &QKeyMapper_Worker::onKey2MouseCycleTimeout);

    /* Connect QJoysticks Signals */
    QJoysticks *instance = QJoysticks::getInstance();
    // QObject::connect(instance, &QJoysticks::countChanged, this, &QKeyMapper_Worker::onJoystickcountChanged, Qt::QueuedConnection);
    QObject::connect(instance, &QJoysticks::joystickAdded, this, &QKeyMapper_Worker::onJoystickAdded, Qt::QueuedConnection);
    QObject::connect(instance, &QJoysticks::joystickRemoved, this, &QKeyMapper_Worker::onJoystickRemoved, Qt::QueuedConnection);
    QObject::connect(instance, &QJoysticks::POVEvent, this, &QKeyMapper_Worker::onJoystickPOVEvent);
    QObject::connect(instance, &QJoysticks::axisEvent, this, &QKeyMapper_Worker::onJoystickAxisEvent);
    QObject::connect(instance, &QJoysticks::buttonEvent, this, &QKeyMapper_Worker::onJoystickButtonEvent);
    QObject::connect(instance, &QJoysticks::sensorEvent, this, &QKeyMapper_Worker::onJoystickSensorEvent);

    initGlobalSendInputTaskController();

    generateVirtualInputRandomValues();
    initVirtualKeyCodeMap();
    initVirtualMouseButtonMap();
    initCombinationKeysList();
    initMultiKeyboardInputList();
    initMultiMouseInputList();
    initMultiVirtualGamepadInputList();
    initJoystickKeyMap();
    initSpecialOriginalKeysList();
    initSpecialMappingKeysList();
    initSpecialVirtualKeyCodeList();
    // initSkipReleaseModifiersKeysList();

#ifdef VIGEM_CLIENT_SUPPORT
    initViGEmKeyMap();
    m_LastMouseCursorPoint.x = -1;
    m_LastMouseCursorPoint.y = -1;
#endif
}

QKeyMapper_Worker::~QKeyMapper_Worker()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "~QKeyMapper_Worker() called.";
#endif
    s_isWorkerDestructing = true;

    setWorkerKeyUnHook();
    clearGlobalSendInputTaskController();

#ifdef VIGEM_CLIENT_SUPPORT
    // ViGEmClient_Remove();
    ViGEmClient_RemoveAllTargets();
    ViGEmClient_Disconnect();
    ViGEmClient_Free();
#endif
}

void QKeyMapper_Worker::postVirtualKeyCode(HWND hwnd, uint keycode, int keyupdown)
{
    if (keyupdown == KEY_DOWN) {
        SendMessageTimeout(
            hwnd,
            WM_KEYDOWN,
            keycode,
            0,
            SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
            SENDMESSAGE_TIMEOUT,
            nullptr
        );
    }
    else {
        SendMessageTimeout(
            hwnd,
            WM_KEYUP,
            keycode,
            0,
            SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
            SENDMESSAGE_TIMEOUT,
            nullptr
        );
    }
}

void QKeyMapper_Worker::sendUnicodeChar(wchar_t aChar)
{
    INPUT u_input[2];

    u_input[0].type = INPUT_KEYBOARD;
    u_input[0].ki.wVk = 0;
    u_input[0].ki.wScan = aChar;
    u_input[0].ki.dwFlags = KEYEVENTF_UNICODE;
    u_input[0].ki.time = 0;
    u_input[0].ki.dwExtraInfo = VIRTUAL_UNICODE_CHAR;

    u_input[1].type = INPUT_KEYBOARD;
    u_input[1].ki.wVk = 0;
    u_input[1].ki.wScan = aChar;
    u_input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    u_input[1].ki.time = 0;
    u_input[1].ki.dwExtraInfo = VIRTUAL_UNICODE_CHAR;

    SendInput(2, u_input, sizeof(INPUT));
}

void QKeyMapper_Worker::sendText(HWND window_hwnd, const QString &text)
{
    if (window_hwnd != NULL) {
        HWND hWnd = NULL;
        hWnd = FindWindowEx(QKeyMapper::s_CurrentMappingHWND, NULL, L"Edit", NULL);
        if (hWnd == NULL) {
            hWnd = FindWindowEx(QKeyMapper::s_CurrentMappingHWND, NULL, L"Scintilla", NULL);
        }

        for (const QChar &ch : text) {
            wchar_t wchar = ch.unicode();
            if (hWnd != NULL) {
                SendMessageTimeout(
                    hWnd,
                    WM_CHAR,
                    wchar,
                    0,
                    SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
                    SENDMESSAGE_TIMEOUT,
                    nullptr
                );
            } else {
                sendUnicodeChar(wchar);
            }
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[sendText]" << "SendText ->" << text << ", hWnd =" << hWnd;
#endif
    }
    else {
        for (const QChar &ch : text) {
            wchar_t wchar = ch.unicode();
            sendUnicodeChar(wchar);
        }
    }
}

void QKeyMapper_Worker::sendWindowMousePointClick(HWND hwnd, const QString &mousebutton, int keyupdown, const QPoint &mousepoint)
{
    // Get the client area rectangle (relative to the window)
    RECT clientRect;
    if (!GetClientRect(hwnd, &clientRect)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[sendWindowMouseButton] GetClientRect() failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
        return;
    }

    // Convert the top-left corner of the client area to screen coordinates
    POINT clientTopLeft = { clientRect.left, clientRect.top };
    if (!ClientToScreen(hwnd, &clientTopLeft)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[sendWindowMouseButton] ClientToScreen() failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
        return;
    }

    // Convert the relative coordinates within the client area to screen coordinates
    int x = clientTopLeft.x + mousepoint.x();
    int y = clientTopLeft.y + mousepoint.y();

    // Convert the screen coordinates to absolute coordinates required by SendInput
    double fScreenWidth  = GetSystemMetrics(SM_CXSCREEN) - 1;
    double fScreenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
    double fx = x * (65535.0 / fScreenWidth);
    double fy = y * (65535.0 / fScreenHeight);

    // Find the virtual mouse button code
    V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(mousebutton);

    // Configure the input structure
    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = static_cast<LONG>(fx);
    mouse_input.mi.dy = static_cast<LONG>(fy);
    mouse_input.mi.mouseData = vmousecode.MouseXButton;
    mouse_input.mi.time = 0;
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_POINTCLICK;
    mouse_input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    if (keyupdown == KEY_DOWN) {
        mouse_input.mi.dwFlags |= vmousecode.MouseDownCode;
    } else {
        mouse_input.mi.dwFlags |= vmousecode.MouseUpCode;
    }

    // Send the input
    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[sendWindowMouseButton] SendInput() failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::sendWindowMouseMoveToPoint(HWND hwnd, const QPoint &mousepoint)
{
    // Get the client area rectangle (relative to the window)
    RECT clientRect;
    if (!GetClientRect(hwnd, &clientRect)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[sendWindowMouseMoveToPoint] GetClientRect() failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
        return;
    }

    // Convert the top-left corner of the client area to screen coordinates
    POINT clientTopLeft = { clientRect.left, clientRect.top };
    if (!ClientToScreen(hwnd, &clientTopLeft)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[sendWindowMouseMoveToPoint] ClientToScreen() failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
        return;
    }

    // Convert the relative coordinates within the client area to screen coordinates
    int x = clientTopLeft.x + mousepoint.x();
    int y = clientTopLeft.y + mousepoint.y();

    // Convert the screen coordinates to absolute coordinates required by SendInput
    double fScreenWidth  = GetSystemMetrics(SM_CXSCREEN) - 1;
    double fScreenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
    double fx = x * (65535.0 / fScreenWidth);
    double fy = y * (65535.0 / fScreenHeight);

    // Configure the input structure
    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = static_cast<LONG>(fx);
    mouse_input.mi.dy = static_cast<LONG>(fy);
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_MOVE;
    mouse_input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    // Send the mouse_input event
    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[sendWindowMouseMoveToPoint] SendInput() failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::postMouseButton(HWND hwnd, const QString &mousebutton, int keyupdown, const QPoint &mousepoint)
{
    UINT messageMouseButton;
    WPARAM wParam = 0;
    int x = 0;
    int y = 0;

    if (mousepoint.x() >= 0 && mousepoint.y() >= 0) {
        x = mousepoint.x();
        y = mousepoint.y();
    }
    else {
        bool valid_point = true;
        POINT currentPos;
        if (!GetCursorPos(&currentPos)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[postMouseButton]" << "GetCursorPos error! ->" << GetLastError();;
#endif
            valid_point = false;
        }

        if (valid_point && !ScreenToClient(hwnd, &currentPos)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[postMouseButton]" << "ScreenToClient error! hwnd:" << hwnd << ", currentPos.x:" << currentPos.x << ", currentPos.y:" << currentPos.y << ", GetLastError:" << GetLastError();
#endif
            valid_point = false;
        }

        if (valid_point) {
            x = currentPos.x;
            y = currentPos.y;
        }
    }
    LPARAM lParam = MAKELPARAM(x, y);

    if (mousebutton == "Mouse-L") {
        messageMouseButton = (keyupdown == KEY_DOWN) ? WM_LBUTTONDOWN : WM_LBUTTONUP;
        wParam = MK_LBUTTON;
    }
    else if (mousebutton == "Mouse-R") {
        messageMouseButton = (keyupdown == KEY_DOWN) ? WM_RBUTTONDOWN : WM_RBUTTONUP;
        wParam = MK_RBUTTON;
    }
    else if (mousebutton == "Mouse-M") {
        messageMouseButton = (keyupdown == KEY_DOWN) ? WM_MBUTTONDOWN : WM_MBUTTONUP;
        wParam = MK_MBUTTON;
    }
    else if (mousebutton == "Mouse-X1") {
        messageMouseButton = (keyupdown == KEY_DOWN) ? WM_XBUTTONDOWN : WM_XBUTTONUP;
        wParam = MAKEWPARAM(MK_XBUTTON1, XBUTTON1);
    }
    else if (mousebutton == "Mouse-X2") {
        messageMouseButton = (keyupdown == KEY_DOWN) ? WM_XBUTTONDOWN : WM_XBUTTONUP;
        wParam = MAKEWPARAM(MK_XBUTTON2, XBUTTON2);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[postMouseButton]" << "Invalid Mouse Button Name :" << mousebutton;
#endif
        return;
    }

    SendMessageTimeout(
        hwnd,
        messageMouseButton,
        wParam,
        lParam,
        SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
        SENDMESSAGE_TIMEOUT,
        nullptr
    );
}

void QKeyMapper_Worker::postMouseWheel(HWND hwnd, const QString &mousewheel)
{
    WPARAM wParam = 0;
    LPARAM lParam = 0;
    UINT msg = 0;

    if (mousewheel == MOUSE_WHEEL_UP_STR) {
        msg = WM_MOUSEWHEEL;
        wParam = MAKEWPARAM(0, WHEEL_DELTA);
    }
    else if (mousewheel == MOUSE_WHEEL_DOWN_STR) {
        msg = WM_MOUSEWHEEL;
        wParam = MAKEWPARAM(0, -WHEEL_DELTA);
    }
    else if (mousewheel == MOUSE_WHEEL_LEFT_STR) {
        msg = WM_MOUSEHWHEEL;
        wParam = MAKEWPARAM(0, -WHEEL_DELTA);
    }
    else if (mousewheel == MOUSE_WHEEL_RIGHT_STR) {
        msg = WM_MOUSEHWHEEL;
        wParam = MAKEWPARAM(0, WHEEL_DELTA);
    }

    if (msg != 0) {
        SendMessageTimeout(
            hwnd,
            msg,
            wParam,
            lParam,
            SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
            SENDMESSAGE_TIMEOUT,
            nullptr
        );
    }
}

void QKeyMapper_Worker::postMouseMove(HWND hwnd, int delta_x, int delta_y)
{
#if 1
    POINT currentPos;
    if (!GetCursorPos(&currentPos)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[postMouseMove]" << "GetCursorPos error! ->" << GetLastError();;
#endif
        return;
    }

    if (!ScreenToClient(hwnd, &currentPos)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[postMouseMove]" << "ScreenToClient error! hwnd:" << hwnd << ", currentPos.x:" << currentPos.x << ", currentPos.y:" << currentPos.y << ", GetLastError:" << GetLastError();
#endif
        return;
    }

    POINT newPos;
    newPos.x = currentPos.x + delta_x;
    newPos.y = currentPos.y + delta_y;

    LPARAM lParam = MAKELPARAM(newPos.x, newPos.y);
    SendMessageTimeout(
        hwnd,
        WM_MOUSEMOVE,
        0,
        lParam,
        SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
        SENDMESSAGE_TIMEOUT,
        nullptr
    );

#else
    POINT currentPos;
    if (!GetCursorPos(&currentPos)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[postMouseMove]" << "GetCursorPos error! ->" << GetLastError();;
#endif
        return;
    }

    int new_x = currentPos.x + delta_x;
    int new_y = currentPos.y + delta_y;

    LPARAM lParam = MAKELPARAM(new_x, new_y);
    SendMessageTimeout(
        hwnd,
        WM_MOUSEMOVE,
        0,
        lParam,
        SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
        SENDMESSAGE_TIMEOUT,
        nullptr
    );
#endif
}

void QKeyMapper_Worker::postMouseMoveToPoint(HWND hwnd, const QPoint &mousepoint)
{
    POINT newPos;
    newPos.x = mousepoint.x();
    newPos.y = mousepoint.y();

    LPARAM lParam = MAKELPARAM(newPos.x, newPos.y);
    SendMessageTimeout(
        hwnd,
        WM_MOUSEMOVE,
        0,
        lParam,
        SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
        SENDMESSAGE_TIMEOUT,
        nullptr
    );
}

void QKeyMapper_Worker::sendKeyboardInput(V_KEYCODE vkeycode, int keyupdown)
{
    INPUT keyboard_input = { 0 };
    DWORD extenedkeyflag = 0;
    if (true == vkeycode.ExtenedFlag){
        extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
    }
    else{
        extenedkeyflag = 0;
    }
    keyboard_input.type = INPUT_KEYBOARD;
    keyboard_input.ki.time = 0;
    keyboard_input.ki.dwExtraInfo = VIRTUAL_RESEND_REALKEY;
    keyboard_input.ki.wVk = vkeycode.KeyCode;
    keyboard_input.ki.wScan = MapVirtualKey(keyboard_input.ki.wVk, MAPVK_VK_TO_VSC);
    if (KEY_DOWN == keyupdown) {
        keyboard_input.ki.dwFlags = extenedkeyflag | 0;
    }
    else {
        keyboard_input.ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
    }
    UINT uSent = SendInput(1, &keyboard_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("sendKeyboardInput(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::sendMouseClick(V_MOUSECODE vmousecode, int keyupdown)
{
    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = 0;
    mouse_input.mi.dy = 0;
    mouse_input.mi.mouseData = 0;
    mouse_input.mi.time = 0;
    mouse_input.mi.dwExtraInfo = VIRTUAL_RESEND_REALKEY;
    if (KEY_DOWN == keyupdown) {
        mouse_input.mi.dwFlags = vmousecode.MouseDownCode;
    }
    else {
        mouse_input.mi.dwFlags = vmousecode.MouseUpCode;
    }
    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("sendMouseClick(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::sendMouseMove(int delta_x, int delta_y)
{
    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = delta_x;
    mouse_input.mi.dy = delta_y;
    mouse_input.mi.mouseData = 0;
    mouse_input.mi.time = 0;
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_MOVE_BYKEYS;
    mouse_input.mi.dwFlags = MOUSEEVENTF_MOVE;
//    mouse_input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("sendMouseMove(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::sendMouseWheel(int wheel_updown)
{
    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_WHEEL;
    mouse_input.mi.dwFlags = MOUSEEVENTF_WHEEL;

    if (wheel_updown == MOUSE_WHEEL_UP) {
        mouse_input.mi.mouseData = WHEEL_DELTA;
    } else if (wheel_updown == MOUSE_WHEEL_DOWN) {
        mouse_input.mi.mouseData = -WHEEL_DELTA;
    } else {
        return;
    }

    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[sendMouseWheel] SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::setMouseToScreenCenter(void)
{
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Calculate the coordinates for the center of the screen
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;

    // Initialize INPUT structure
    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = (centerX * (65536 / screenWidth));
    mouse_input.mi.dy = (centerY * (65536 / screenHeight));
    mouse_input.mi.mouseData = 0;
    mouse_input.mi.time = 0;
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_MOVE;
    mouse_input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    // Send the mouse_input event
    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setMouseToScreenCenter] SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::setMouseToPoint(POINT point)
{
    // Calculate the new coordinates for the mouse input structure.
    double fScreenWidth     = GetSystemMetrics( SM_CXSCREEN )-1;
    double fScreenHeight    = GetSystemMetrics( SM_CYSCREEN )-1;
    double fx = point.x * ( 65535.0f / fScreenWidth );
    double fy = point.y * ( 65535.0f / fScreenHeight );

    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = static_cast<LONG>(fx);
    mouse_input.mi.dy = static_cast<LONG>(fy);
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_MOVE;
    mouse_input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    // Send the mouse_input event
    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setMouseToPoint] SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::setMouseToScreenBottomRight()
{
    // Initialize INPUT structure
    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = MOUSE_CURSOR_BOTTOMRIGHT_X;
    mouse_input.mi.dy = MOUSE_CURSOR_BOTTOMRIGHT_Y;
    mouse_input.mi.mouseData = 0;
    mouse_input.mi.time = 0;
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_MOVE;
    mouse_input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    // Send the mouse_input event
    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setMouseToScreenBottomRight] SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

POINT QKeyMapper_Worker::mousePositionAfterSetMouseToScreenBottomRight()
{
    POINT pt;

    // Get the screen resolution
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Calculate the theoretical mouse position
    pt.x = (MOUSE_CURSOR_BOTTOMRIGHT_X * screenWidth) / 65535 - 1;
    pt.y = (MOUSE_CURSOR_BOTTOMRIGHT_Y * screenHeight) / 65535 - 1;

    return pt;
}

#ifdef VIGEM_CLIENT_SUPPORT
void QKeyMapper_Worker::onMouseMove(int delta_x, int delta_y, int mouse_index)
{
    // Q_UNUSED(delta_x);
    // Q_UNUSED(delta_y);

    bool mouse2vjoy_update = false;
    if (s_Mouse2vJoy_EnableStateMap.contains(INITIAL_MOUSE_INDEX)) {
        mouse_index = -1;
        mouse2vjoy_update = true;
    }
    else if (s_Mouse2vJoy_EnableStateMap.contains(mouse_index)) {
        mouse2vjoy_update = true;
    }

    if (mouse2vjoy_update) {
        // int input_delta_x = 0;
        // int input_delta_y = 0;
        // if (mouse_index >= 0) {
        //     QMutexLocker locker(&s_MouseMove_delta_List_Mutex);
        //     input_delta_x = s_Mouse2vJoy_delta_List.at(mouse_index).x();
        //     input_delta_y = s_Mouse2vJoy_delta_List.at(mouse_index).y();
        //     s_Mouse2vJoy_delta_List[mouse_index] = QPoint();
        // }
        // else {
        //     if (Interception_Worker::s_InterceptStart) {
        //         input_delta_x = s_Mouse2vJoy_delta_interception.x();
        //         input_delta_y = s_Mouse2vJoy_delta_interception.y();
        //         s_Mouse2vJoy_delta_interception = QPoint();
        //     }
        //     else {
        //         input_delta_x = delta_x;
        //         input_delta_y = delta_y;
        //     }
        // }

#ifdef MOUSE_VERBOSE_LOG
        qDebug() << "[onMouseMove]" << "Mouse Move -> Delta X =" << delta_x << ", Delta Y =" << delta_y << ", MouseIndex =" << mouse_index;
#endif
        if (delta_x != 0 || delta_y != 0) {
            int gamepad_index = s_Mouse2vJoy_EnableStateMap[mouse_index].gamepad_index;
            ViGEmClient_Mouse2JoystickUpdate(delta_x, delta_y, mouse_index, gamepad_index);
        }
    }
}

void QKeyMapper_Worker::initMouse2vJoyResetTimerMap()
{
    for (int mouse_index = INITIAL_MOUSE_INDEX; mouse_index < INTERCEPTION_MAX_MOUSE; ++mouse_index) {
        QTimer *timer = new QTimer(this);
        timer->setTimerType(Qt::PreciseTimer);
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, this, [this, mouse_index]() {
            onMouse2vJoyResetTimeoutForMap(mouse_index);
        });
        m_Mouse2vJoyResetTimerMap.insert(mouse_index, timer);
    }
}

void QKeyMapper_Worker::stopMouse2vJoyResetTimerMap()
{
    /* stop all the timers in m_Mouse2vJoyResetTimerMap */
    for (QTimer *timer : std::as_const(m_Mouse2vJoyResetTimerMap)) {
        timer->stop();
    }
}

void QKeyMapper_Worker::onMouse2vJoyResetTimeoutForMap(int mouse_index)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onMouse2vJoyResetTimeoutForMap]" << "Mouse Index =" << mouse_index;
#endif

    if (s_Mouse2vJoy_EnableStateMap.contains(mouse_index)) {
        if (s_Mouse2vJoy_Hold) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[onMouse2vJoyResetTimeoutForMap]" << "Skip Mouse2vJoyReset for Mouse2vJoy_Hold is KEY_DOWN State, MouseIndex =" << mouse_index;
#endif
            return;
        }
    }

    int gamepad_index = s_Mouse2vJoy_EnableStateMap[mouse_index].gamepad_index;
    ViGEmClient_JoysticksReset(mouse_index, gamepad_index);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onMouse2vJoyResetTimeoutForMap]" << "Reset the VirtualJoysticks to Release Center State, MouseIndex =" << mouse_index << ", VirtualGamepadIndex =" << gamepad_index;
#endif
}

void QKeyMapper_Worker::onKey2MouseCycleTimeout()
{
    if (false == s_Joy2Mouse_EnableStateMap.isEmpty()) {
        bool joy2mouse_update = false;
        int player_index = s_LastJoyAxisPlayerIndex;
        if (s_Joy2Mouse_EnableStateMap.contains(INITIAL_PLAYER_INDEX)) {
            player_index = -1;
            joy2mouse_update = true;
        }
        else if (s_Joy2Mouse_EnableStateMap.contains(player_index)) {
            joy2mouse_update = true;
        }

        if (joy2mouse_update) {
            joystick2MouseMoveProc(player_index);
        }
    }
    else if (s_Key2Mouse_EnableState) {
        key2MouseMoveProc();
    }
}
#endif

void QKeyMapper_Worker::onMouseWheel(int wheel_updown)
{
    if (MOUSE_WHEEL_UP == wheel_updown) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onMouseWheel]" << "Mouse Wheel Up";
#endif
    }
    else if (MOUSE_WHEEL_DOWN == wheel_updown) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onMouseWheel]" << "Mouse Wheel Down";
#endif
    }
}

void QKeyMapper_Worker::onSendInputKeys(int rowindex, QStringList inputKeys, int keyupdown, QString original_key, int sendmode, int sendvirtualkey_state, QList<MAP_KEYDATA> *keyMappingDataList)
{
    SendInputTask *sendInputTask = new SendInputTask(rowindex, inputKeys, keyupdown, original_key, sendmode, sendvirtualkey_state, keyMappingDataList);

    SendInputTaskController *controller = Q_NULLPTR;
    controller = &SendInputTask::s_SendInputTaskControllerMap[sendInputTask->m_real_originalkey];

    controller->task_threadpool->start(sendInputTask);

#ifdef DEBUG_LOGOUT_ON
    QString threadIdStr = QString("0x%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 8, 16, QChar('0')).toUpper();
    qDebug().nospace().noquote() << "[onSendInputKeys] currentThread -> Name:" << QThread::currentThread()->objectName() << ", ID:" << threadIdStr << ", Originalkey[" << original_key << "], Real_originalkey[" << sendInputTask->m_real_originalkey << "] " << ((keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << ", MappingKeys[" << inputKeys << "], SendMode:" << sendmode << ", SendVirtualkeyState:" << sendvirtualkey_state;
#endif
}

void QKeyMapper_Worker::sendInputKeys(int rowindex, QStringList inputKeys, int keyupdown, QString original_key, int sendmode, SendInputTaskController controller, QList<MAP_KEYDATA> *keyMappingDataList)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("[sendInputKeys] currentThread -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

    Q_UNUSED(sendmode);
    int waitTime = 0;
    bool keyseq_start = false;
    bool keyseq_finished = false;
    int sendvirtualkey_state = controller.sendvirtualkey_state;
    int row_index = controller.task_rowindex;

    QString keySequenceStr = ":" + QString(KEYSEQUENCE_STR);

    if (original_key.contains(keySequenceStr)) {
        QString firstKeySeqPostStr = QString("%1%2").arg(keySequenceStr).arg(1);
        QString finalKeySeqPostStr = QString(":%1").arg(KEYSEQUENCE_FINAL_STR);
        if (original_key.endsWith(firstKeySeqPostStr)) {
            keyseq_start = true;
        }
        else if (original_key.endsWith(finalKeySeqPostStr)) {
            keyseq_finished = true;
        }

        QString keyseq = inputKeys.constFirst();
        if (keyupdown == KEY_DOWN) {
            if (keyseq_start) {
                if (*controller.task_stop_flag != INPUTSTOP_NONE) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "\033[1;34m[sendInputKeys] Mapping Key Sequence Start, task_stop_flag set back (" << *controller.task_stop_flag << ") -> INPUTSTOP_NONE\033[0m";
#endif
                    *controller.task_stop_flag = INPUTSTOP_NONE;
                }
            }
        }
        else if (keyupdown == KEY_UP && keyseq.contains("vJoy-")) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[sendInputKeys] vJoy Key Up wait start ->" << keyseq;
#endif
            QThread::msleep(VJOY_KEYUP_WAITTIME);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[sendInputKeys] vJoy Key Up wait end ->" << keyseq;
#endif
        }
    }
    else if (original_key.contains(MOUSE_WHEEL_STR)) {
        if (KEY_UP == keyupdown) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[sendInputKeys] Mouse Wheel KeyUp wait start ->" << original_key;
#endif
            QThread::msleep(MOUSE_WHEEL_KEYUP_WAITTIME);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[sendInputKeys] Mouse Wheel KeyUp wait end ->" << original_key;
#endif
        }
    }

    int key_sequence_count = inputKeys.size();
    if (key_sequence_count <= 0) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("sendInputKeys(): no input keys, size error(%d)!!!", key_sequence_count);
#endif
        return;
    }
    else if (key_sequence_count > KEY_SEQUENCE_MAX) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("sendInputKeys(): Key sequence is too long(%d)!!!", key_sequence_count);
#endif
        return;
    }

    if (keyMappingDataList == Q_NULLPTR && rowindex >= 0) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("sendInputKeys(): keyMappingDataList is NULL!!!");
#endif
        return;
    }

    static QRegularExpression mapkey_regex(R"(^([↓↑⇵！]?)([^⏱]+)(?:⏱(\d+))?$)");
    // Use non-greedy matching and multiline support for SendText
    static QRegularExpression sendTextRegexp(
        REGEX_PATTERN_SENDTEXT,
        QRegularExpression::MultilineOption
    );
    static QRegularExpression vjoy_regex("^(vJoy-[^@]+)(?:@([0-3]))?$");
    int keycount = 0;
    int sendtype = SENDTYPE_NORMAL;
    // INPUT inputs[SEND_INPUTS_MAX] = { 0 };
    bool postmappingkey = false;
    int fixedvkeycode = FIXED_VIRTUAL_KEY_CODE_NONE;

    // Use saved mapping table pointer to avoid array bounds issues during tab switching
    if (rowindex >= 0 && rowindex < keyMappingDataList->size()) {
        postmappingkey = keyMappingDataList->at(rowindex).PostMappingKey;
        fixedvkeycode = keyMappingDataList->at(rowindex).FixedVKeyCode;
        if (fixedvkeycode < FIXED_VIRTUAL_KEY_CODE_MIN || fixedvkeycode > FIXED_VIRTUAL_KEY_CODE_MAX) {
            fixedvkeycode = FIXED_VIRTUAL_KEY_CODE_NONE;
        }
    }

    if (KEY_UP == keyupdown) {
        if (key_sequence_count > 1) {
            /* Add for KeySequenceHoldDown >>> */
            if (sendmode == SENDMODE_KEYSEQ_HOLDDOWN) {
                QString original_key_holddown = original_key + ":" + KEYSEQUENCE_STR + HOLDDOWN_STR;
                bool pressedmappingkeys_contains = false;
                {
                QMutexLocker locker(&s_PressedMappingKeysMapMutex);
                pressedmappingkeys_contains = pressedMappingKeysMap.contains(original_key_holddown);
                }
                if (pressedmappingkeys_contains) {
#ifdef DEBUG_LOGOUT_ON
                    {
                    QMutexLocker locker(&s_PressedMappingKeysMapMutex);
                    qDebug().nospace().noquote() << "\033[1;34m[sendInputKeys] KeySeqHoldDown Final KeyUp -> original_key_holddown[" << original_key_holddown << "], " << "KeySequenceLastKeys[" << inputKeys.last() << "]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap << "\033[0m";
                    }
#endif
                    QStringList mappingKeyList = QStringList() << inputKeys.last();
                    emit_sendInputKeysSignal_Wrapper(rowindex, mappingKeyList, KEY_UP, original_key_holddown, SENDMODE_NORMAL);
                }
            }
            /* Add for KeySequenceHoldDown <<< */
            return;
        }

// #ifdef DEBUG_LOGOUT_ON
//         if (fixedvkeycode != FIXED_VIRTUAL_KEY_CODE_NONE) {
//             QString fixedvkeycodeStr = QString("0x%1").arg(QString::number(fixedvkeycode, 16).toUpper(), 2, '0');
//             QString debugmessage = QString("\033[1;32m[sendInputKeys] KeyUp FixedVirtualKeyCode = %1\033[0m").arg(fixedvkeycodeStr);
//             qDebug().nospace().noquote() << debugmessage;
//         }
// #endif

        const QString mappingkeys_str = inputKeys.constFirst();
        QStringList mappingKeys = splitMappingKeyString(mappingkeys_str, SPLIT_WITH_PLUS);
        keycount = mappingKeys.size();

        if (keycount > MAPPING_KEYS_MAX) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("sendInputKeys(): Too many keys(%d) to mapping!!!", keycount);
#endif
            return;
        }

        /* Add for KeySequenceHoldDown >>> */
        if (sendmode == SENDMODE_KEYSEQ_HOLDDOWN) {
            static QRegularExpression regex("(.+):KeySequenceHoldDown");
            QRegularExpressionMatch match = regex.match(original_key);
            if (match.hasMatch()) {
                QString original_key_holddown = match.captured(1);
                if (original_key_holddown.contains('@')) {
                    if (pressedRealKeysList.contains(original_key_holddown)) {
#ifdef DEBUG_LOGOUT_ON
                        /* \033[1;34m (Blue Bold Text) \033[0m */
                        /* \033[34m (Blue Text) */
                        /* \033[31m (Red Text) */
                        /* \033[32m (Green Text) */
                        qDebug().nospace().noquote() << "\033[1;34m[sendInputKeys] MultiInput KeySeqHoldDown skip KeyUp -> original_key_holddown[" << original_key_holddown << "], " << "mappingKeys[" << inputKeys << "]" << " : pressedRealKeysList -> " << pressedRealKeysList << "\033[0m";
#endif
                        /* return for KeySequenceHoldDown to skip KeyUp of last mappingkey. */
                        return;
                    }
                }
                else {
                    if (pressedRealKeysListRemoveMultiInput.contains(original_key_holddown)) {
#ifdef DEBUG_LOGOUT_ON
                        /* \033[1;34m (Blue Bold Text) \033[0m */
                        /* \033[34m (Blue Text) */
                        /* \033[31m (Red Text) */
                        /* \033[32m (Green Text) */
                        qDebug().nospace().noquote() << "\033[1;34m[sendInputKeys] KeySeqHoldDown skip KeyUp -> original_key_holddown[" << original_key_holddown << "], " << "mappingKeys[" << inputKeys << "]" << " : pressedRealKeysList -> " << pressedRealKeysList << "\033[0m";
#endif
                        /* return for KeySequenceHoldDown to skip KeyUp of last mappingkey. */
                        return;
                    }
                }

            }
        }
        /* Add for KeySequenceHoldDown <<< */

        bool pressedMappingKeysContains = false;
        bool pressedMappingKeysEmpty = false;
        {
        QMutexLocker locker(&s_PressedMappingKeysMapMutex);
        if (pressedMappingKeysMap.contains(original_key)) {
            pressedMappingKeysMap.remove(original_key);
            pressedMappingKeysContains = true;

            if (pressedMappingKeysMap.isEmpty()) {
                pressedMappingKeysEmpty = true;
            }
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys KeyUp -> original_key[" << original_key << "], " << "mappingKeys[" << mappingkeys_str << "]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
#endif

        if (HOOKPROC_STATE_STOPPED == s_AtomicHookProcState) {
            if (pressedMappingKeysEmpty) {
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[sendInputKeys] pressedMappingKeysMap is empty on s_AtomicHookProcState = HOOKPROC_STATE_STOPPED!");
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                emit QKeyMapper_Worker::getInstance()->allMappingKeysReleased_Signal();
            }
        }
        }

        for(auto it = mappingKeys.crbegin(); it != mappingKeys.crend(); ++it) {
            QString key = (*it);

#if 0
            waitTime = 0;
#endif
            QRegularExpressionMatch mapkey_match = mapkey_regex.match(key);
            QRegularExpressionMatch vjoy_match;
            sendtype = SENDTYPE_NORMAL;
            if (mapkey_match.hasMatch()) {
                QString prefix = mapkey_match.captured(1);
                key = mapkey_match.captured(2);

                if (prefix == PREFIX_SEND_DOWN) {
                    sendtype = SENDTYPE_DOWN;
                }
                else if (prefix == PREFIX_SEND_UP) {
                    sendtype = SENDTYPE_UP;
                }
                else if (prefix == PREFIX_SEND_BOTH) {
                    sendtype = SENDTYPE_BOTH;
                }
                else if (prefix == PREFIX_SEND_EXCLUSION) {
                    sendtype = SENDTYPE_EXCLUSION;
                }

                vjoy_match = vjoy_regex.match(key);
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[sendInputKeys] KeyUp mappingkey regularexpression not matched -> " << key;
#endif
            }

            if (key.isEmpty()
                || key == KEY_NONE_STR
                || sendtype == SENDTYPE_DOWN
                || sendtype == SENDTYPE_UP
                || sendtype == SENDTYPE_BOTH) {
                continue;
            }

            if (key == MOUSE_WHEEL_UP_STR
                || key == MOUSE_WHEEL_DOWN_STR
                || key == MOUSE_WHEEL_LEFT_STR
                || key == MOUSE_WHEEL_RIGHT_STR) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendInputKeys] Do no need to send [" << key << "]" << " at KEY_UP";
#endif
                continue;
            }

            if (isPressedMappingKeysContains(key) && sendmode != SENDMODE_FORCE_STOP && sendtype != SENDTYPE_EXCLUSION) {
#ifdef DEBUG_LOGOUT_ON
                {
                QMutexLocker locker(&s_PressedMappingKeysMapMutex);
                qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys still remain Key[ " << key << " ]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
                }
                qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys skip KeyUp[ " << key << " ]" << " -> original_key[ " << original_key << " ], " << "mappingKeys[ " << mappingkeys_str << " ]";
#endif
                continue;
            }

#if 0
            /* special hook key process */
            if (SENDMODE_NORMAL == sendmode) {
                if ((original_key == key) && (keycount == 1)) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug("Mapping the same key, do not skip send mapping VirtualKey \"%s\" KEYUP!", key.toStdString().c_str());
#endif
                }
                else {
                }
            }
            else if (SENDMODE_FORCE_STOP == sendmode) {
            }
#endif

            QRegularExpressionMatch sendTextMatch = sendTextRegexp.match(key);
            if (sendTextMatch.hasMatch()) {
                /* SendText KeyUp do nothing. */
            }
            else if (vjoy_match.hasMatch()) {
                if (original_key != CLEAR_VIRTUALKEYS) {
                    QString joystickButton = vjoy_match.captured(1);
                    QString gamepadIndexString = vjoy_match.captured(2);
                    if (gamepadIndexString.isEmpty()) {
                        ViGEmClient_ReleaseButton(key, 0);
                    }
                    else {
                        int gamepad_index = gamepadIndexString.toInt();
                        ViGEmClient_ReleaseButton(joystickButton, gamepad_index);
                    }
                }
            }
            else if (true == VirtualMouseButtonMap.contains(key)) {
                if (sendtype != SENDTYPE_EXCLUSION
                    && sendmode != SENDMODE_FORCE_STOP
                    && postmappingkey != true
                    && controller.sendvirtualkey_state != SENDVIRTUALKEY_STATE_BURST_STOP
                    && false == pressedVirtualKeysList.contains(key)) {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[sendInputKeys-Mouse] pressedVirtualKeysList not contains \"%1\" on send Mouse KEY_UP!!!! -> sendtype = %2, sendvirtualkey_state = %3").arg(key).arg(sendtype).arg(controller.sendvirtualkey_state);
                    qWarning().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                    continue;
                }

                int send_keyupdown = KEY_UP;
                if (sendtype == SENDTYPE_EXCLUSION) {
                    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(key, keyMappingDataList);
                    bool send_exclusion = (findindex < 0 || keyMappingDataList->at(findindex).PassThrough);

                    if (send_exclusion
                        && (pressedRealKeysListRemoveMultiInput.contains(key) || pressedVirtualKeysList.contains(key))) {
                        send_keyupdown = KEY_DOWN;
                    }
                    else {
                        continue;
                    }
                }
                INPUT input = { 0 };
                V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);
                input.type = INPUT_MOUSE;
                input.mi.mouseData = vmousecode.MouseXButton;
                if (sendtype == SENDTYPE_EXCLUSION) {
                    input.mi.dwExtraInfo = VIRTUAL_KEY_OVERLAY;
                }
                else {
                    input.mi.dwExtraInfo = VIRTUAL_KEY_SEND | sendvirtualkey_state;
                }
                if (KEY_DOWN == send_keyupdown) {
                    input.mi.dwFlags = vmousecode.MouseDownCode;
                }
                else {
                    input.mi.dwFlags = vmousecode.MouseUpCode;
                }
                if (postmappingkey) {
                    if (QKeyMapper::s_CurrentMappingHWND != NULL) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "[sendInputKeys] PostMappingKey(true), postMouseButton(" << key << ") " << ((send_keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << " -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                        postMouseButton(QKeyMapper::s_CurrentMappingHWND, key, send_keyupdown);
                    }
                }
                else {
                    SendInput(1, &input, sizeof(INPUT));
                }

                if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
                    for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                        postMouseButton(hwnd, key, send_keyupdown);
                    }
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[sendInputKeys] postMouseButton(" << key << ") " << ((send_keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << " -> " << QKeyMapper::s_last_HWNDList;
#endif
                }
            }
            else if (key.startsWith(MOUSE_MOVE_PREFIX) && key.endsWith(")")) {
                sendMouseMoveToPoint(key, postmappingkey);
            }
            else if (key.startsWith(MOUSE_BUTTON_PREFIX) && key.endsWith(")")) {
                sendMousePointClick(key, KEY_UP, postmappingkey);
            }
            else if (true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(key)) {
                if (controller.sendvirtualkey_state != SENDVIRTUALKEY_STATE_MODIFIERS
                    && sendmode != SENDMODE_FORCE_STOP
                    && fixedvkeycode == FIXED_VIRTUAL_KEY_CODE_NONE
                    && postmappingkey != true
                    && controller.sendvirtualkey_state != SENDVIRTUALKEY_STATE_BURST_STOP
                    && sendtype != SENDTYPE_EXCLUSION
                    && false == pressedVirtualKeysList.contains(key)) {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[sendInputKeys-Keyboard] pressedVirtualKeysList not contains \"%1\" on send Keyboard KEY_UP!!!! -> sendtype = %2, sendvirtualkey_state = %3").arg(key).arg(sendtype).arg(controller.sendvirtualkey_state);
                    qWarning().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                    continue;
                }

                int send_keyupdown = KEY_UP;
                if (sendtype == SENDTYPE_EXCLUSION) {
                    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(key, keyMappingDataList);
                    bool send_exclusion = (findindex < 0 || keyMappingDataList->at(findindex).PassThrough);

                    if (send_exclusion
                        && (pressedRealKeysListRemoveMultiInput.contains(key) || pressedVirtualKeysList.contains(key))) {
                        send_keyupdown = KEY_DOWN;
                    }
                    else {
                        continue;
                    }
                }
                INPUT input = { 0 };
                V_KEYCODE vkeycode = QKeyMapper_Worker::VirtualKeyCodeMap.value(key);
                DWORD extenedkeyflag = 0;
                if (true == vkeycode.ExtenedFlag){
                    extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
                }
                else{
                    extenedkeyflag = 0;
                }
                input.type = INPUT_KEYBOARD;
                bool normal_send = false;
                if (sendtype == SENDTYPE_EXCLUSION) {
                    input.ki.dwExtraInfo = VIRTUAL_KEY_OVERLAY;
                }
                else if (VK_MOUSE2VJOY_HOLD == vkeycode.KeyCode
                    || VK_KEY2MOUSE_UP == vkeycode.KeyCode
                    || VK_KEY2MOUSE_DOWN == vkeycode.KeyCode
                    || VK_KEY2MOUSE_LEFT == vkeycode.KeyCode
                    || VK_KEY2MOUSE_RIGHT == vkeycode.KeyCode
                    || VK_CROSSHAIR_NORMAL == vkeycode.KeyCode
                    || VK_CROSSHAIR_TYPEA == vkeycode.KeyCode
                    || VK_GYRO2MOUSE_HOLD == vkeycode.KeyCode
                    || VK_GYRO2MOUSE_MOVE == vkeycode.KeyCode) {
                    if (0 <= row_index && row_index < USHRT_MAX) {
                        input.ki.dwExtraInfo = VIRTUAL_CUSTOM_KEYS | row_index;
                    }
                    else {
                        input.ki.dwExtraInfo = VIRTUAL_CUSTOM_KEYS | USHRT_MAX;
                    }
                }
                else {
                    input.ki.dwExtraInfo = VIRTUAL_KEY_SEND | sendvirtualkey_state;
                    normal_send = true;
                }
                input.ki.wVk = vkeycode.KeyCode;
                input.ki.wScan = MapVirtualKey(input.ki.wVk, MAPVK_VK_TO_VSC);
//#ifdef DEBUG_LOGOUT_ON
//                qDebug("sendInputKeys(): Key Up -> \"%s\", wScan->0x%08X", key.toStdString().c_str(), input.ki.wScan);
//#endif
                if (KEY_DOWN == send_keyupdown) {
                    input.ki.dwFlags = extenedkeyflag | 0;
                }
                else {
                    input.ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                }
                if (normal_send && fixedvkeycode != FIXED_VIRTUAL_KEY_CODE_NONE) {
                    input.ki.wVk = fixedvkeycode;
                    input.ki.dwExtraInfo = VIRTUAL_KEY_OVERLAY;
                }
                if (postmappingkey) {
                    if (QKeyMapper::s_CurrentMappingHWND != NULL) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "[sendInputKeys] postmappingkey(true), postVirtualKeyCode(" << key << ") KeyUp -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                        postVirtualKeyCode(QKeyMapper::s_CurrentMappingHWND, vkeycode.KeyCode, send_keyupdown);
                    }
                }
                else {
                    UINT uSent = SendInput(1, &input, sizeof(INPUT));
                    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug("sendInputKeys(): Keyboard SendInput KEY_UP failed: Error=0x%X, Ret=%d", HRESULT_FROM_WIN32(GetLastError()), uSent);
#endif
                    }
                }

                if (QKeyMapper::getSendToSameTitleWindowsStatus()
                    && false == SpecialVirtualKeyCodeList.contains(vkeycode.KeyCode)) {
                    for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                        postVirtualKeyCode(hwnd, vkeycode.KeyCode, send_keyupdown);
                    }
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[sendInputKeys] postVirtualKeyCode(" << key << ") KeyUp -> " << QKeyMapper::s_last_HWNDList;
#endif
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("sendInputKeys(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
            }
        }

        if (sendmode == SENDMODE_BURSTKEY_STOP && rowindex >= 0) {
            if (!pressedMappingKeysContains) {
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[sendInputKeys] Originalkey \"%1\" bustkey is released on sendmode = SENDMODE_BURSTKEY_STOP, call resendRealKeyCodeOnStop()").arg(original_key);
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                resendRealKeyCodeOnStop(rowindex);
            }
        }
        else if (sendmode == SENDMODE_BURSTKEY_STOP_ON_HOOKSTOPPED && rowindex >= 0) {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("[sendInputKeys] Originalkey \"%1\" bustkey is released on sendmode = SENDMODE_BURSTKEY_STOP_ON_HOOKSTOPPED, call resendRealKeyCodeOnStop()").arg(original_key);
            qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            resendRealKeyCodeOnStop(rowindex);
        }
        else if (sendmode == SENDMODE_BURSTKEY_STOP_ON_HOOKRESTART && rowindex >= 0) {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("[sendInputKeys] Originalkey \"%1\" bustkey is released on sendmode = SENDMODE_BURSTKEY_STOP_ON_HOOKRESTART, call resendRealKeyCodeOnStop(restart)").arg(original_key);
            qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            resendRealKeyCodeOnStop(rowindex, true);
        }

        if (*controller.task_stop_flag == INPUTSTOP_SINGLE) {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("\033[1;34m[sendInputKeys] OriginalKey(%1) Single Mappingkeys KEY_UP finished, task_stop_flag set back INPUTSTOP_SINGLE -> INPUTSTOP_NONE\033[0m").arg(original_key);
            qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            *controller.task_stop_flag = INPUTSTOP_NONE;
        }
        else if (*controller.task_stop_flag == INPUTSTOP_KEYSEQ) {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("\033[1;34m[sendInputKeys] OriginalKey(%1) KeySequence KEY_UP breaked, task_stop_flag set INPUTSTOP_KEYSEQ -> INPUTSTOP_NONE\033[0m").arg(original_key);
            qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            *controller.task_stop_flag = INPUTSTOP_NONE;

            keyseq_finished = false;
            QString orikey_str = original_key.left(original_key.indexOf(":"));
            if (s_runningKeySequenceOrikeyList.contains(orikey_str)) {
                s_runningKeySequenceOrikeyList.removeAll(orikey_str);
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendInputKeys] Running KeySequence breaked on KEY_UP, remove OriginalKey:" << orikey_str << ", s_runningKeySequenceOrikeyList -> " << s_runningKeySequenceOrikeyList;
#endif

                int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(orikey_str, keyMappingDataList);
                if (findindex >= 0) {
                    resendRealKeyCodeOnStop(findindex);

                    int repeat_mode = keyMappingDataList->at(findindex).RepeatMode;
                    QStringList repeat_mappingKeyList = keyMappingDataList->at(findindex).Mapping_Keys;
                    QString repeat_original_key = keyMappingDataList->at(findindex).Original_Key;
                    int repeat_mappingkeylist_size = repeat_mappingKeyList.size();
                    if (repeat_mappingkeylist_size > 1 && REPEAT_MODE_BYTIMES == repeat_mode) {
                        if (s_KeySequenceRepeatCount.contains(repeat_original_key)) {
                            s_KeySequenceRepeatCount.remove(repeat_original_key);
                        }
                    }
                }
            }
        }

        if (keyseq_finished) {
            bool real_finished = true;
            QString orikey_str = original_key.left(original_key.indexOf(":"));
            int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(orikey_str, keyMappingDataList);

            if (findindex >= 0) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendInputKeys] KeySequence finished -> OriginalKey:" << orikey_str << ", Index:" << findindex;
#endif
                int repeat_mode = keyMappingDataList->at(findindex).RepeatMode;
                QStringList repeat_mappingKeyList = keyMappingDataList->at(findindex).Mapping_Keys;
                QString repeat_original_key = keyMappingDataList->at(findindex).Original_Key;
                int repeat_mappingkeylist_size = repeat_mappingKeyList.size();

                if (repeat_mappingkeylist_size > 1 && REPEAT_MODE_BYKEY == repeat_mode) {
                    bool isKeyPressed = false;
                    if (orikey_str.contains(SEPARATOR_LONGPRESS)) {
                        if (pressedLongPressKeysList.contains(orikey_str)) {
                            isKeyPressed = true;
                        }
                    }
                    else if (orikey_str.contains(SEPARATOR_DOUBLEPRESS)) {
                        static QRegularExpression doublepress_regex(R"(^(.+✖)(\d+)$)");
                        QRegularExpressionMatch doublepress_match = doublepress_regex.match(orikey_str);
                        if (doublepress_match.hasMatch()) {
                            QString doublepress_orikeystr = doublepress_match.captured(1);
                            if (pressedDoublePressKeysList.contains(doublepress_orikeystr)) {
                                isKeyPressed = true;
                            }
                        }
                    }
                    else if (pressedRealKeysList.contains(orikey_str)) {
                        isKeyPressed = true;
                    }

                    if (isKeyPressed) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "[sendInputKeys] Repeat KeySequence by key -> OriginalKey:" << orikey_str << ", Index:" << findindex;
#endif
                        real_finished = false;
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, repeat_mappingKeyList, KEY_DOWN, repeat_original_key, SENDMODE_KEYSEQ_REPEAT, SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT);
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, repeat_mappingKeyList, KEY_UP, repeat_original_key, SENDMODE_KEYSEQ_REPEAT, SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT);
                    }
                }
                else if (repeat_mappingkeylist_size > 1 && REPEAT_MODE_BYTIMES == repeat_mode) {
                    int repeat_times = keyMappingDataList->at(findindex).RepeatTimes;
                    if (s_KeySequenceRepeatCount.contains(repeat_original_key)) {
                        ++s_KeySequenceRepeatCount[repeat_original_key];
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "\033[1;34m[sendInputKeys] Repeat KeySequence by times count++ -> OriginalKey:" << repeat_original_key << ", RepeatCount:" << s_KeySequenceRepeatCount.value(repeat_original_key) << "\033[0m";
#endif
                        if (s_KeySequenceRepeatCount.value(repeat_original_key) >= repeat_times) {
                            s_KeySequenceRepeatCount.remove(repeat_original_key);
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace().noquote() << "\033[1;34m[sendInputKeys] Repeat KeySequence by times count reached -> OriginalKey:" << repeat_original_key << ", RepeatTimes:" << repeat_times << "\033[0m";
#endif
                        }
                        else {
                            real_finished = false;
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, repeat_mappingKeyList, KEY_DOWN, repeat_original_key, SENDMODE_KEYSEQ_REPEAT, SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, repeat_mappingKeyList, KEY_UP, repeat_original_key, SENDMODE_KEYSEQ_REPEAT, SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT);
                        }
                    }
                }

                if (real_finished) {
                    if (s_runningKeySequenceOrikeyList.contains(orikey_str)) {
                        s_runningKeySequenceOrikeyList.removeAll(orikey_str);
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "[sendInputKeys] Running KeySequence finished remove OriginalKey:" << orikey_str << ", s_runningKeySequenceOrikeyList -> " << s_runningKeySequenceOrikeyList;
#endif
                        resendRealKeyCodeOnStop(findindex);
                    }
                }
            }
        }
    }
    else {
        if (1 == key_sequence_count) {
            if (*controller.task_stop_flag) {
                bool skip = true;
                if (keyseq_finished) {
                    if (*controller.task_stop_flag != INPUTSTOP_NONE) {
#ifdef DEBUG_LOGOUT_ON
                        QString debugmessage = QString("\033[1;34m[sendInputKeys] OriginalKey(%1) KeySequence KEY_DOWN finished, task_stop_flag set back (%2) -> INPUTSTOP_NONE\033[0m").arg(original_key).arg(*controller.task_stop_flag);
                        qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                        *controller.task_stop_flag = INPUTSTOP_NONE;
                    }
                }

                if (*controller.task_stop_flag == INPUTSTOP_SINGLE) {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("\033[1;34m[sendInputKeys] OriginalKey(%1) Single Mappingkeys KEY_DOWN start, task_stop_flag set back INPUTSTOP_SINGLE -> INPUTSTOP_NONE\033[0m").arg(original_key);
                    qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                    *controller.task_stop_flag = INPUTSTOP_NONE;
                    skip = false;
                }

                if (skip) {
                    return;
                }
            }

// #ifdef DEBUG_LOGOUT_ON
//             if (fixedvkeycode != FIXED_VIRTUAL_KEY_CODE_NONE) {
//                 QString fixedvkeycodeStr = QString("0x%1").arg(QString::number(fixedvkeycode, 16).toUpper(), 2, '0');
//                 QString debugmessage = QString("\033[1;32m[sendInputKeys] KeyDown FixedVirtualKeyCode = %1\033[0m").arg(fixedvkeycodeStr);
//                 qDebug().nospace().noquote() << debugmessage;
//             }
// #endif

            const QString mappingkeys_str = inputKeys.constFirst();
            QStringList mappingKeys = splitMappingKeyString(mappingkeys_str, SPLIT_WITH_PLUS);
            keycount = mappingKeys.size();

            if (keycount > MAPPING_KEYS_MAX) {
#ifdef DEBUG_LOGOUT_ON
                qWarning("sendInputKeys(): Too many keys(%d) to mapping!!!", keycount);
#endif
                return;
            }

            bool pressedmappingkeys_contains = false;
            {
            QMutexLocker locker(&s_PressedMappingKeysMapMutex);
            pressedmappingkeys_contains = pressedMappingKeysMap.contains(original_key);
            }
            if (pressedmappingkeys_contains) {
#ifdef DEBUG_LOGOUT_ON
                {
                QMutexLocker locker(&s_PressedMappingKeysMapMutex);
                qDebug().nospace().noquote() << "[sendInputKeys] Mapping KeyDown Skiped! pressedMappingKeys already exist! -> original_key[" << original_key << "], " << "mappingKeys[" << mappingkeys_str << "]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
                }
#endif
                return;
            }

            {
            QMutexLocker locker(&s_PressedMappingKeysMapMutex);
            pressedMappingKeysMap.insert(original_key, mappingKeys);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys KeyDown -> original_key[" << original_key << "], " << "mappingKeys[" << mappingkeys_str << "]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
#endif
            }

            for (const QString &keyStr : std::as_const(mappingKeys)) {
                if (*controller.task_stop_flag) {
                    continue;
                }

                QString key = keyStr;
                waitTime = 0;

                QRegularExpressionMatch mapkey_match = mapkey_regex.match(key);
                QRegularExpressionMatch vjoy_match;
                sendtype = SENDTYPE_NORMAL;
                if (mapkey_match.hasMatch()) {
                    QString prefix = mapkey_match.captured(1);
                    key = mapkey_match.captured(2);
                    QString waitTimeString = mapkey_match.captured(3);
                    bool ok = true;
                    int waittime_temp = 0;

                    if (!waitTimeString.isEmpty()) {
                        waittime_temp = waitTimeString.toInt(&ok);
                        if (ok) {
                            waitTime = waittime_temp;
                        }
                    }

                    if (prefix == PREFIX_SEND_DOWN) {
                        sendtype = SENDTYPE_DOWN;
                    }
                    else if (prefix == PREFIX_SEND_UP) {
                        sendtype = SENDTYPE_UP;
                    }
                    else if (prefix == PREFIX_SEND_BOTH) {
                        sendtype = SENDTYPE_BOTH;
                    }
                    else if (prefix == PREFIX_SEND_EXCLUSION) {
                        sendtype = SENDTYPE_EXCLUSION;
                    }

                    vjoy_match = vjoy_regex.match(key);
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "[sendInputKeys] KeyDown mappingkey regularexpression not matched -> " << key;
#endif
                }

                QRegularExpressionMatch sendTextMatch = sendTextRegexp.match(key);
                if (key.isEmpty() || key == KEY_NONE_STR) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[sendInputKeys] KeySequence KeyDown only wait time ->" << waitTime;
#endif
                }
                else if (sendTextMatch.hasMatch()) {
                    QString text = sendTextMatch.captured(1);

                    const Qt::KeyboardModifiers modifiers_arg = Qt::ControlModifier;
                    releaseKeyboardModifiersDirect(modifiers_arg);

                    sendText(QKeyMapper::s_CurrentMappingHWND, text);
                }
                else if (vjoy_match.hasMatch()) {
                    QString joystickButton = vjoy_match.captured(1);
                    QString gamepadIndexString = vjoy_match.captured(2);
                    int send_keyupdown = KEY_DOWN;
                    if (sendtype == SENDTYPE_UP) {
                        send_keyupdown = KEY_UP;
                    }

                    if (gamepadIndexString.isEmpty()) {
                        if (send_keyupdown == KEY_DOWN) {
                            ViGEmClient_PressButton(key, AUTO_ADJUST_NONE, 0, INITIAL_PLAYER_INDEX);
                        }
                        else {
                            ViGEmClient_ReleaseButton(key, 0);
                        }
                    }
                    else {
                        int gamepad_index = gamepadIndexString.toInt();
                        if (send_keyupdown == KEY_DOWN) {
                            ViGEmClient_PressButton(joystickButton, AUTO_ADJUST_NONE, gamepad_index, INITIAL_PLAYER_INDEX);
                        }
                        else {
                            ViGEmClient_ReleaseButton(joystickButton, gamepad_index);
                        }
                    }

                    if (sendtype == SENDTYPE_BOTH) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[sendInputKeys] MappingKey SENDTYPE_BOTH vJoy KeyUp wait start ->" << key;
#endif
                        QThread::msleep(VJOY_KEYUP_WAITTIME);
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[sendInputKeys] MappingKey SENDTYPE_BOTH vJoy KeyUp wait end ->" << key;
#endif
                        if (gamepadIndexString.isEmpty()) {
                            ViGEmClient_ReleaseButton(key, 0);
                        }
                        else {
                            int gamepad_index = gamepadIndexString.toInt();
                            ViGEmClient_ReleaseButton(joystickButton, gamepad_index);
                        }
                    }
                }
                else if (key == MOUSE_WHEEL_UP_STR || key == MOUSE_WHEEL_DOWN_STR || key == MOUSE_WHEEL_LEFT_STR || key == MOUSE_WHEEL_RIGHT_STR) {
                    INPUT input = { 0 };
                    input.type = INPUT_MOUSE;
                    input.mi.dwExtraInfo = VIRTUAL_MOUSE_WHEEL;
                    input.mi.dwFlags = MOUSEEVENTF_WHEEL;

                    if (key == MOUSE_WHEEL_UP_STR) {
                        input.mi.mouseData = WHEEL_DELTA;
                    }
                    else if (key == MOUSE_WHEEL_DOWN_STR) {
                        input.mi.mouseData = -WHEEL_DELTA;
                    }
                    else if (key == MOUSE_WHEEL_LEFT_STR) {
                        input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
                        input.mi.mouseData = -WHEEL_DELTA;
                    }
                    else if (key == MOUSE_WHEEL_RIGHT_STR) {
                        input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
                        input.mi.mouseData = WHEEL_DELTA;
                    }

                    if (postmappingkey) {
                        if (QKeyMapper::s_CurrentMappingHWND != NULL) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace().noquote() << "[sendInputKeys] postmappingkey(true), postMouseWheel(" << key << ") -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                            postMouseWheel(QKeyMapper::s_CurrentMappingHWND, key);
                        }
                    }
                    else {
                        SendInput(1, &input, sizeof(INPUT));
                    }

                    if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
                        for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                            postMouseWheel(hwnd, key);
                        }
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "[sendInputKeys] postMouseWheel(" << key << ") -> " << QKeyMapper::s_last_HWNDList;
#endif
                    }
                }
                else if (true == VirtualMouseButtonMap.contains(key)) {
                    if (SENDTYPE_NORMAL == sendtype && true == pressedVirtualKeysList.contains(key)) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug("sendInputKeys(): Mouse Button Down -> \"%s\" already exist!!!", key.toStdString().c_str());
#endif
                        continue;
                    }

                    int send_keyupdown = KEY_DOWN;
                    if (sendtype == SENDTYPE_UP) {
                        send_keyupdown = KEY_UP;
                    }
                    else if (sendtype == SENDTYPE_EXCLUSION) {
                        int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(key, keyMappingDataList);
                        bool send_exclusion = (findindex < 0 || keyMappingDataList->at(findindex).PassThrough);

                        if (send_exclusion
                            && (pressedRealKeysListRemoveMultiInput.contains(key) || pressedVirtualKeysList.contains(key))) {
                            send_keyupdown = KEY_UP;
                        }
                        else {
                            continue;
                        }
                    }
                    INPUT input = { 0 };
                    V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);
                    input.type = INPUT_MOUSE;
                    input.mi.mouseData = vmousecode.MouseXButton;
                    if (sendtype == SENDTYPE_EXCLUSION) {
                        input.mi.dwExtraInfo = VIRTUAL_KEY_OVERLAY;
                    }
                    else {
                        input.mi.dwExtraInfo = VIRTUAL_KEY_SEND | sendvirtualkey_state;
                    }
                    if (KEY_DOWN == send_keyupdown) {
                        input.mi.dwFlags = vmousecode.MouseDownCode;
                    }
                    else {
                        input.mi.dwFlags = vmousecode.MouseUpCode;
                    }
                    if (postmappingkey) {
                        if (QKeyMapper::s_CurrentMappingHWND != NULL) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace().noquote() << "[sendInputKeys] postmappingkey(true), postMouseButton(" << key << ") " << ((send_keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << " -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                            postMouseButton(QKeyMapper::s_CurrentMappingHWND, key, send_keyupdown);
                        }
                    }
                    else {
                        SendInput(1, &input, sizeof(INPUT));
                    }

                    if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
                        for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                            postMouseButton(hwnd, key, send_keyupdown);
                        }
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "[sendInputKeys] postMouseButton(" << key << ") " << ((send_keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << " -> " << QKeyMapper::s_last_HWNDList;
#endif
                    }

                    if (sendtype == SENDTYPE_BOTH) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[sendInputKeys] MappingKey SENDTYPE_BOTH KeyUp wait start ->" << key;
#endif
                        QThread::msleep(SENDTYPE_BOTH_WAITTIME);
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[sendInputKeys] MappingKey SENDTYPE_BOTH KeyUp wait end ->" << key;
#endif
                        send_keyupdown = KEY_UP;
                        input.mi.dwFlags = vmousecode.MouseUpCode;
                        if (postmappingkey) {
                            if (QKeyMapper::s_CurrentMappingHWND != NULL) {
#ifdef DEBUG_LOGOUT_ON
                                qDebug().nospace().noquote() << "[sendInputKeys] postmappingkey(true), SENDTYPE_BOTH postMouseButton(" << key << ") " << ((send_keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << " -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                                postMouseButton(QKeyMapper::s_CurrentMappingHWND, key, send_keyupdown);
                            }
                        }
                        else {
                            SendInput(1, &input, sizeof(INPUT));
                        }

                        if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
                            for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                                postMouseButton(hwnd, key, send_keyupdown);
                            }
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace().noquote() << "[sendInputKeys] SENDTYPE_BOTH postMouseButton(" << key << ") " << ((send_keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << " -> " << QKeyMapper::s_last_HWNDList;
#endif
                        }
                    }
                }
                else if (key.startsWith(MOUSE_MOVE_PREFIX) && key.endsWith(")")) {
                    sendMouseMoveToPoint(key, postmappingkey);
                }
                else if (key.startsWith(MOUSE_BUTTON_PREFIX) && key.endsWith(")")) {
                    int send_keyupdown = KEY_DOWN;
                    if (sendtype == SENDTYPE_UP) {
                        send_keyupdown = KEY_UP;
                    }
                    sendMousePointClick(key, send_keyupdown, postmappingkey);

                    if (sendtype == SENDTYPE_BOTH) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[sendInputKeys] MappingKey SENDTYPE_BOTH KeyUp wait start ->" << key;
#endif
                        QThread::msleep(SENDTYPE_BOTH_WAITTIME);
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[sendInputKeys] MappingKey SENDTYPE_BOTH KeyUp wait end ->" << key;
#endif
                        send_keyupdown = KEY_UP;
                        sendMousePointClick(key, send_keyupdown, postmappingkey);
                    }
                }
                else if (true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(key)) {
                    if (SENDTYPE_NORMAL == sendtype && true == pressedVirtualKeysList.contains(key)) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug("sendInputKeys(): Key Down -> \"%s\" already exist!!!", key.toStdString().c_str());
#endif
                        continue;
                    }

                    int send_keyupdown = KEY_DOWN;
                    if (sendtype == SENDTYPE_UP) {
                        send_keyupdown = KEY_UP;
                    }
                    else if (sendtype == SENDTYPE_EXCLUSION) {
                        int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(key, keyMappingDataList);
                        bool send_exclusion = (findindex < 0 || keyMappingDataList->at(findindex).PassThrough);

                        if (send_exclusion
                            && (pressedRealKeysListRemoveMultiInput.contains(key) || pressedVirtualKeysList.contains(key))) {
                            send_keyupdown = KEY_UP;
                        }
                        else {
                            continue;
                        }
                    }
                    INPUT input = { 0 };
                    V_KEYCODE vkeycode = QKeyMapper_Worker::VirtualKeyCodeMap.value(key);
                    DWORD extenedkeyflag = 0;
                    if (true == vkeycode.ExtenedFlag){
                        extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
                    }
                    else{
                        extenedkeyflag = 0;
                    }
                    input.type = INPUT_KEYBOARD;
                    bool normal_send = false;
                    if (sendtype == SENDTYPE_EXCLUSION) {
                        input.ki.dwExtraInfo = VIRTUAL_KEY_OVERLAY;
                    }
                    else if (VK_MOUSE2VJOY_HOLD == vkeycode.KeyCode
                        || VK_KEY2MOUSE_UP == vkeycode.KeyCode
                        || VK_KEY2MOUSE_DOWN == vkeycode.KeyCode
                        || VK_KEY2MOUSE_LEFT == vkeycode.KeyCode
                        || VK_KEY2MOUSE_RIGHT == vkeycode.KeyCode
                        || VK_CROSSHAIR_NORMAL == vkeycode.KeyCode
                        || VK_CROSSHAIR_TYPEA == vkeycode.KeyCode
                        || VK_GYRO2MOUSE_HOLD == vkeycode.KeyCode
                        || VK_GYRO2MOUSE_MOVE == vkeycode.KeyCode) {
                        if (0 <= row_index && row_index < USHRT_MAX) {
                            input.ki.dwExtraInfo = VIRTUAL_CUSTOM_KEYS | row_index;
                        }
                        else {
                            input.ki.dwExtraInfo = VIRTUAL_CUSTOM_KEYS | USHRT_MAX;
                        }
                    }
                    else {
                        input.ki.dwExtraInfo = VIRTUAL_KEY_SEND | sendvirtualkey_state;
                        normal_send = true;
                    }
                    input.ki.wVk = vkeycode.KeyCode;
                    input.ki.wScan = MapVirtualKey(input.ki.wVk, MAPVK_VK_TO_VSC);
//#ifdef DEBUG_LOGOUT_ON
//                    qDebug("sendInputKeys(): Key Down -> \"%s\", wScan->0x%08X", key.toStdString().c_str(), input.ki.wScan);
//#endif
                    if (KEY_DOWN == send_keyupdown) {
                        input.ki.dwFlags = extenedkeyflag | 0;
                    }
                    else {
                        input.ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                    }
                    if (normal_send && fixedvkeycode != FIXED_VIRTUAL_KEY_CODE_NONE) {
                        input.ki.wVk = fixedvkeycode;
                        input.ki.dwExtraInfo = VIRTUAL_KEY_OVERLAY;
                    }
                    if (postmappingkey) {
                        if (QKeyMapper::s_CurrentMappingHWND != NULL) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace().noquote() << "[sendInputKeys] postmappingkey(true), postVirtualKeyCode(" << key << ") KeyDown -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                            postVirtualKeyCode(QKeyMapper::s_CurrentMappingHWND, vkeycode.KeyCode, send_keyupdown);
                        }
                    }
                    else {
                        UINT uSent = SendInput(1, &input, sizeof(INPUT));
                        if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug("sendInputKeys(): Keyboard SendInput KEY_DOWN failed: Error=0x%X, Ret=%d", HRESULT_FROM_WIN32(GetLastError()), uSent);
#endif
                        }
                    }

                    if (QKeyMapper::getSendToSameTitleWindowsStatus()
                        && false == SpecialVirtualKeyCodeList.contains(vkeycode.KeyCode)) {
                        for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                            postVirtualKeyCode(hwnd, vkeycode.KeyCode, send_keyupdown);
                        }
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "[sendInputKeys] postVirtualKeyCode(" << key << ") KeyDown -> " << QKeyMapper::s_last_HWNDList;
#endif
                    }

                    if (sendtype == SENDTYPE_BOTH) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[sendInputKeys] MappingKey SENDTYPE_BOTH KeyUp wait start ->" << key;
#endif
                        QThread::msleep(SENDTYPE_BOTH_WAITTIME);
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[sendInputKeys] MappingKey SENDTYPE_BOTH KeyUp wait end ->" << key;
#endif
                        send_keyupdown = KEY_UP;
                        input.ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                        if (postmappingkey) {
                            if (QKeyMapper::s_CurrentMappingHWND != NULL) {
#ifdef DEBUG_LOGOUT_ON
                                qDebug().nospace().noquote() << "[sendInputKeys] postmappingkey(true), SENDTYPE_BOTH postVirtualKeyCode(" << key << ") KeyDown -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                                postVirtualKeyCode(QKeyMapper::s_CurrentMappingHWND, vkeycode.KeyCode, send_keyupdown);
                            }
                        }
                        else {
                            SendInput(1, &input, sizeof(INPUT));
                        }

                        if (QKeyMapper::getSendToSameTitleWindowsStatus()
                            && false == SpecialVirtualKeyCodeList.contains(vkeycode.KeyCode)) {
                            for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                                postVirtualKeyCode(hwnd, vkeycode.KeyCode, send_keyupdown);
                            }
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace().noquote() << "[sendInputKeys] SENDTYPE_BOTH postVirtualKeyCode(" << key << ") KeyDown -> " << QKeyMapper::s_last_HWNDList;
#endif
                        }
                    }
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qWarning("sendInputKeys(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
                }

                if (*controller.task_stop_flag) {
                    waitTime = 0;
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[sendInputKeys] task_stop_flag ->" << *controller.task_stop_flag << ", line:" << __LINE__;
#endif
                }
                if (MAPPING_WAITTIME_MIN < waitTime && waitTime <= MAPPING_WAITTIME_MAX) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[sendInputKeys] StopConditionWait Start -> Time=" << waitTime << ", task_stop_flag=" << *controller.task_stop_flag << ", line:" << __LINE__;
#endif
                    QDeadlineTimer deadline(waitTime, Qt::PreciseTimer);
                    controller.task_stop_mutex->lock();
                    controller.task_stop_condition->wait(controller.task_stop_mutex, deadline);
                    controller.task_stop_mutex->unlock();
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[sendInputKeys] StopConditionWait Finished -> Time=" << waitTime << ", task_stop_flag=" << *controller.task_stop_flag << ", line:" << __LINE__;
#endif
                }
            }

            if (*controller.task_stop_flag == INPUTSTOP_SINGLE) {
                *controller.task_stop_flag = INPUTSTOP_NONE;
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("\033[1;34m[sendInputKeys] OriginalKey(%1) Single Mappingkeys KEY_DOWN finished, task_stop_flag set back INPUTSTOP_SINGLE -> INPUTSTOP_NONE\033[0m").arg(original_key);
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            }
            else if (*controller.task_stop_flag == INPUTSTOP_KEYSEQ) {
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("\033[1;34m[sendInputKeys] OriginalKey(%1) KeySequence KEY_DOWN breaked, task_stop_flag set INPUTSTOP_KEYSEQ -> INPUTSTOP_NONE\033[0m").arg(original_key);
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                *controller.task_stop_flag = INPUTSTOP_NONE;

                {
                QMutexLocker locker(&s_PressedMappingKeysMapMutex);
                pressedMappingKeysMap.remove(original_key);
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys KeySequence break -> original_key[" << original_key << "], " << "mappingKeys[" << mappingkeys_str << "]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
#endif
                }
                clearPressedVirtualKeysOfMappingKeys(mappingkeys_str);

                QString orikey_str = original_key.left(original_key.indexOf(":"));
                if (s_runningKeySequenceOrikeyList.contains(orikey_str)) {
                    s_runningKeySequenceOrikeyList.removeAll(orikey_str);
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[sendInputKeys] Running KeySequence breaked on KEY_DOWN, remove OriginalKey:" << orikey_str << ", s_runningKeySequenceOrikeyList -> " << s_runningKeySequenceOrikeyList;
#endif

                    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(orikey_str, keyMappingDataList);
                    if (findindex >= 0) {
                        resendRealKeyCodeOnStop(findindex);

                        int repeat_mode = keyMappingDataList->at(findindex).RepeatMode;
                        QStringList repeat_mappingKeyList = keyMappingDataList->at(findindex).Mapping_Keys;
                        QString repeat_original_key = keyMappingDataList->at(findindex).Original_Key;
                        int repeat_mappingkeylist_size = repeat_mappingKeyList.size();
                        if (repeat_mappingkeylist_size > 1 && REPEAT_MODE_BYTIMES == repeat_mode) {
                            if (s_KeySequenceRepeatCount.contains(repeat_original_key)) {
                                s_KeySequenceRepeatCount.remove(repeat_original_key);
                            }
                        }
                    }
                }
            }
        }
        /* key_sequence_count > 1 */
        else {
            sendKeySequenceList(inputKeys, original_key, sendmode, sendvirtualkey_state);
        }
    }
}

void QKeyMapper_Worker::sendMousePointClick(QString &mousepoint_str, int keyupdown, bool postmappingkey)
{
    static QRegularExpression regex(R"((Mouse-L|Mouse-R|Mouse-M|Mouse-X1|Mouse-X2)(:W)?(:BG)?\((\d+),(\d+)\))");
    QRegularExpressionMatch match = regex.match(mousepoint_str);

    if (match.hasMatch()) {
        QString mousebutton = match.captured(1);
        if (!VirtualMouseButtonMap.contains(mousebutton)) {
            return;
        }

        bool isWindowPoint = !match.captured(2).isEmpty();
        bool isPostBG = !match.captured(3).isEmpty();
        bool x_ok;
        bool y_ok;
        int x = match.captured(4).toInt(&x_ok);
        int y = match.captured(5).toInt(&y_ok);

        if (!x_ok || !y_ok || x < 0 || y < 0) {
            return;
        }

        if (isWindowPoint) {
            QPoint mousepoint(x, y);

            if (QKeyMapper::s_CurrentMappingHWND != NULL) {
                if (postmappingkey || isPostBG) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[sendMousePointClick] postmappingkey=true, postMouseButton(" << mousebutton << ", " << x << ", " << y << ") " << ((keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << " -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                    postMouseButton(QKeyMapper::s_CurrentMappingHWND, mousebutton, keyupdown, mousepoint);
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[sendMousePointClick] sendWindowMousePointClick(" << mousebutton << ", " << x << ", " << y << ") " << ((keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << " -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                    sendWindowMousePointClick(QKeyMapper::s_CurrentMappingHWND, mousebutton, keyupdown, mousepoint);
                }
            }

            if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
                for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                    if (QKeyMapper::s_CurrentMappingHWND == hwnd) {
                        continue;
                    }
                    postMouseButton(hwnd, mousebutton, keyupdown, mousepoint);
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendMousePointClick] Window : postMouseButton(" << mousebutton << ", " << x << ", " << y << ") " << ((keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << " -> " << QKeyMapper::s_last_HWNDList;
#endif
            }
        }
        else {
            double fScreenWidth     = GetSystemMetrics( SM_CXSCREEN )-1;
            double fScreenHeight    = GetSystemMetrics( SM_CYSCREEN )-1;
            double fx = x * ( 65535.0f / fScreenWidth );
            double fy = y * ( 65535.0f / fScreenHeight );
            V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(mousebutton);
            INPUT mouse_input = { 0 };
            mouse_input.type = INPUT_MOUSE;
            mouse_input.mi.dx = static_cast<LONG>(fx);
            mouse_input.mi.dy = static_cast<LONG>(fy);
            mouse_input.mi.mouseData = vmousecode.MouseXButton;
            mouse_input.mi.time = 0;
            mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_POINTCLICK;
            if (KEY_DOWN == keyupdown) {
                mouse_input.mi.dwFlags = vmousecode.MouseDownCode | MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            }
            else {
                mouse_input.mi.dwFlags = vmousecode.MouseUpCode | MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            }

            if (postmappingkey || isPostBG) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendMousePointClick] postmappingkey=true, skip Screen : sendMouseButton(" << mousebutton << ", " << x << ", " << y << ") " << ((keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp");
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendMousePointClick] Screen : sendMouseButton(" << mousebutton << ", " << x << ", " << y << ") " << ((keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp");
#endif
                UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
                if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug("[sendMousePointClick] Screen : sendMouseButton -> SendInput() failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
                }
            }

            if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
                QPoint mousepoint(x, y);
                for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                    postMouseButton(hwnd, mousebutton, keyupdown, mousepoint);
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendMousePointClick] Screen : postMouseButton(" << mousebutton << ", " << x << ", " << y << ") " << ((keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << " -> " << QKeyMapper::s_last_HWNDList;
#endif
            }
        }
    }
}

void QKeyMapper_Worker::sendMouseMoveToPoint(QString &mousepoint_str, bool postmappingkey)
{
    static QRegularExpression regex(R"(Mouse-Move(:W)?(:BG)?\((\d+),(\d+)\))");
    QRegularExpressionMatch match = regex.match(mousepoint_str);

    if (match.hasMatch()) {
        bool isWindowPoint = !match.captured(1).isEmpty();
        bool isPostBG = !match.captured(2).isEmpty();
        bool x_ok;
        bool y_ok;
        int x = match.captured(3).toInt(&x_ok);
        int y = match.captured(4).toInt(&y_ok);

        if (!x_ok || !y_ok || x < 0 || y < 0) {
            return;
        }

        QPoint mousepoint(x, y);
        if (isWindowPoint) {
            if (QKeyMapper::s_CurrentMappingHWND != NULL) {
                if (postmappingkey || isPostBG) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[sendMouseMoveToPoint] postmappingkey=true, skip sendWindowMouseMoveToPoint(" << x << ", " << y << ") -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[sendMouseMoveToPoint] sendWindowMouseMoveToPoint(" << x << ", " << y << ") -> " << QKeyMapper::s_CurrentMappingHWND;
#endif
                    sendWindowMouseMoveToPoint(QKeyMapper::s_CurrentMappingHWND, mousepoint);
                }
            }

            if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
                for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                    postMouseMoveToPoint(hwnd, mousepoint);
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendMouseMoveToPoint] Window : postMouseMoveToPoint(" << x << ", " << y << ") -> " << QKeyMapper::s_last_HWNDList;
#endif
            }
        }
        else {
            if (postmappingkey || isPostBG) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendMouseMoveToPoint] postmappingkey=true, skip Screen : setMouseToPoint(" << x << ", " << y << ")";
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendMouseMoveToPoint] Screen : setMouseToPoint(" << x << ", " << y << ")";
#endif
                POINT point = {x, y};
                setMouseToPoint(point);
            }

            if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
                for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                    postMouseMoveToPoint(hwnd, mousepoint);
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendMousePointClick] Screen : postMouseMoveToPoint(" << x << ", " << y << ") -> " << QKeyMapper::s_last_HWNDList;
#endif
            }
        }
    }
}

void QKeyMapper_Worker::emit_sendInputKeysSignal_Wrapper(int rowindex, QStringList &inputKeys, int keyupdown, QString &original_key_unchanged, int sendmode, int sendvirtualkey_state, QList<MAP_KEYDATA> *keyMappingDataList)
{
    bool skip_emitsignal = false;
    SendInputTaskController *controller = Q_NULLPTR;
    QString original_key = original_key_unchanged;

    int key_sequence_count = inputKeys.size();
    if (key_sequence_count == 1) {
        const QString mappingkeys_str = inputKeys.constFirst();
        if (mappingkeys_str.startsWith(KEYSEQUENCEBREAK_STR)) {
            if (keyupdown == KEY_DOWN) {
                breakAllRunningKeySequence();
            }
            return;
        }
    }

    if (SendInputTask::s_SendInputTaskControllerMap.contains(original_key)) {
        controller = &SendInputTask::s_SendInputTaskControllerMap[original_key];
    }

    if (keyMappingDataList == Q_NULLPTR) {
        keyMappingDataList = QKeyMapper::KeyMappingDataList;
    }

    int findindex = rowindex;
    bool burst = false;
    bool unbreakable = false;
    if (findindex >= 0) {
        burst = keyMappingDataList->at(findindex).Burst;
        unbreakable = keyMappingDataList->at(findindex).Unbreakable;
        if (burst && controller != Q_NULLPTR) {
            if (sendmode == SENDMODE_BURSTKEY_START) {
                controller->task_threadpool->clear();
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "\033[1;34m[emit_sendInputKeysSignal_Wrapper]" << " original_key(" << original_key << ") SENDMODE_BURSTKEY_START clear thread pool.\033[0m";
#endif
            }
            else if (sendmode == SENDMODE_BURSTKEY_STOP) {
                controller->task_threadpool->clear();
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "\033[1;34m[emit_sendInputKeysSignal_Wrapper]" << " original_key(" << original_key << ") SENDMODE_BURSTKEY_STOP clear thread pool.\033[0m";
#endif
            }
            else if (sendmode == SENDMODE_FORCE_STOP) {
                controller->task_threadpool->clear();
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "\033[1;34m[emit_sendInputKeysSignal_Wrapper]" << " original_key(" << original_key << ") SENDMODE_FORCE_STOP clear thread pool.\033[0m";
#endif
            }
        }
    }

    if (keyupdown == KEY_DOWN || unbreakable) {
        bool isKeySequence = false;
        bool isKeySequenceRunning = false;
        if (key_sequence_count > 1) {
            isKeySequence = true;
            if (s_runningKeySequenceOrikeyList.contains(original_key)) {
                isKeySequenceRunning = true;
            }
        }

        if (isKeySequence) {
            if (findindex >= 0) {
                int repeat_mode = keyMappingDataList->at(findindex).RepeatMode;
                int repeat_times = keyMappingDataList->at(findindex).RepeatTimes;

                if (sendmode == SENDMODE_NORMAL
                    && repeat_mode == REPEAT_MODE_BYTIMES
                    && repeat_times > 0) {
                    if (isKeySequenceRunning) {
                        skip_emitsignal = true;
                        s_KeySequenceRepeatCount[original_key] = -1;
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "\033[1;34m[emit_sendInputKeysSignal_Wrapper]" << " original_key(" << original_key << ") keysequence is running, skip to emit sendInputKeys_Signal()\033[0m";
#endif
                    }
                    else {
                        s_KeySequenceRepeatCount[original_key] = 0;
                    }
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "\033[1;34m[emit_sendInputKeysSignal_Wrapper]" << " original_key(" << original_key << ") repeat by times(" << repeat_times << ") start, sendmode(" << sendmode << ")\033[0m";
#endif
                }
            }

            if (sendmode == SENDMODE_NORMAL) {
                if (isKeySequenceRunning && controller != Q_NULLPTR) {
                    if (unbreakable) {
                        if (keyupdown == KEY_DOWN
                            || (keyupdown == KEY_UP && controller->task_keyup_sent)) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().noquote().nospace() << "\033[1;34m[emit_sendInputKeysSignal_Wrapper] Runing KeySequence contains unbreakable OriginalKey:" << original_key << ", skip " << ((keyupdown == KEY_DOWN) ? " KeyDown" : " KeyUp") << " sendInputKeys_Signal, s_runningKeySequenceOrikeyList -> " << s_runningKeySequenceOrikeyList << "\033[0m";
#endif
                            skip_emitsignal = true;
                        }
                    }
                    else {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().noquote().nospace() << "\033[1;34m[emit_sendInputKeysSignal_Wrapper] task_stop_flag = INPUTSTOP_KEYSEQ, Runing KeySequence contains OriginalKey:" << original_key << ", s_runningKeySequenceOrikeyList -> " << s_runningKeySequenceOrikeyList << "\033[0m";
#endif
                        // controller->task_stop_mutex->lock();
                        controller->task_threadpool->clear();
                        *controller->task_stop_flag = INPUTSTOP_KEYSEQ;
                        controller->task_stop_condition->wakeAll();
                        // controller->task_stop_mutex->unlock();
                    }
                }
            }
        }
        else {
            if (sendmode == SENDMODE_NORMAL
                && sendvirtualkey_state == SENDVIRTUALKEY_STATE_NORMAL) {
                bool pressedmappingkeys_contains = false;
                {
                QMutexLocker locker(&s_PressedMappingKeysMapMutex);
                pressedmappingkeys_contains = pressedMappingKeysMap.contains(original_key);
                }
                if (pressedmappingkeys_contains && controller != Q_NULLPTR) {
                    if (unbreakable) {
                        if (keyupdown == KEY_DOWN
                            || (keyupdown == KEY_UP && controller->task_keyup_sent)) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().noquote().nospace() << "\033[1;34m[emit_sendInputKeysSignal_Wrapper] pressedMappingKeysMap contains unbreakable OriginalKey:" << original_key << ", skip " << ((keyupdown == KEY_DOWN) ? " KeyDown" : " KeyUp") << " sendInputKeys_Signal, pressedMappingKeysMap -> " << pressedMappingKeysMap << "\033[0m";
#endif
                            skip_emitsignal = true;
                        }
                    }
                    else {
#ifdef DEBUG_LOGOUT_ON
                        {
                        QMutexLocker locker(&s_PressedMappingKeysMapMutex);
                        qDebug().noquote().nospace() << "\033[1;34m[emit_sendInputKeysSignal_Wrapper] task_stop_flag = INPUTSTOP_SINGLE, pressedMappingKeysMap contains OriginalKey:" << original_key << ", pressedMappingKeysMap -> " << pressedMappingKeysMap << "\033[0m";
                        }
#endif
                        // controller->task_stop_mutex->lock();
                        *controller->task_stop_flag = INPUTSTOP_SINGLE;
                        controller->task_stop_condition->wakeAll();
                        // controller->task_stop_mutex->unlock();
                    }
                }
            }
        }
    }
    else if (keyupdown == KEY_UP) {
        if (findindex >= 0) {
            int repeat_mode = keyMappingDataList->at(findindex).RepeatMode;

            if (sendmode == SENDMODE_NORMAL
                && repeat_mode == REPEAT_MODE_BYKEY) {
                bool isKeySequence = false;
                bool isKeySequenceRunning = false;
                if (key_sequence_count > 1) {
                    isKeySequence = true;
                    if (s_runningKeySequenceOrikeyList.contains(original_key)) {
                        isKeySequenceRunning = true;
                    }
                }

                if (sendmode == SENDMODE_NORMAL) {
                    if (isKeySequence && isKeySequenceRunning && controller != Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().noquote().nospace() << "\033[1;34m[emit_sendInputKeysSignal_Wrapper] Original KeyUp break repeat by key sequence, task_stop_flag = INPUTSTOP_KEYSEQ, Runing KeySequence contains OriginalKey:" << original_key << ", s_runningKeySequenceOrikeyList -> " << s_runningKeySequenceOrikeyList << "\033[0m";
#endif
                        // controller->task_stop_mutex->lock();
                        controller->task_threadpool->clear();
                        *controller->task_stop_flag = INPUTSTOP_KEYSEQ;
                        controller->task_stop_condition->wakeAll();
                        // controller->task_stop_mutex->unlock();
                    }
                }
            }
        }
    }

    if (false == skip_emitsignal) {
        emit sendInputKeys_Signal(rowindex, inputKeys, keyupdown, original_key, sendmode, sendvirtualkey_state, keyMappingDataList);
#ifdef DEBUG_LOGOUT_ON
        QAtomicInt task_stop_flag(INPUTSTOP_NONE);
        if (controller != Q_NULLPTR) {
            task_stop_flag = *controller->task_stop_flag;
        }
        qDebug().noquote().nospace() << "[emit_sendInputKeysSignal_Wrapper] sendInputKeys_Signal() -> OriginalKey[" << original_key << "]" << ((keyupdown == KEY_DOWN) ? " KeyDown" : " KeyUp") << ", Sendmode:" << sendmode << ", sendvirtualkey_state:" << sendvirtualkey_state << ", task_stop_flag:" << task_stop_flag;
#endif
    }
}

#if 0
void QKeyMapper_Worker::send_WINplusD()
{
    INPUT inputs[3] = { 0 };

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.dwExtraInfo = VIRTUAL_WIN_PLUS_D;
    inputs[0].ki.wVk = VK_LWIN;
    inputs[0].ki.wScan = MapVirtualKey(inputs[0].ki.wVk, MAPVK_VK_TO_VSC);
    inputs[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.dwExtraInfo = VIRTUAL_WIN_PLUS_D;
    inputs[1].ki.wVk = VK_D;
    inputs[1].ki.wScan = MapVirtualKey(inputs[1].ki.wVk, MAPVK_VK_TO_VSC);

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.dwExtraInfo = VIRTUAL_WIN_PLUS_D;
    inputs[2].ki.wVk = VK_LWIN;
    inputs[2].ki.wScan = MapVirtualKey(inputs[2].ki.wVk, MAPVK_VK_TO_VSC);
    inputs[2].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;

    UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    if (uSent != ARRAYSIZE(inputs))
    {
#ifdef DEBUG_LOGOUT_ON
        qDebug("send_WINplusD(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}
#endif

void QKeyMapper_Worker::sendBurstKeyDown(const QString &burstKey, bool start)
{
    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(burstKey);

    if (findindex >=0){
        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
        QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
        int sendmode = SENDMODE_NORMAL;
        int sendvirtualkey_state = SENDVIRTUALKEY_STATE_BURST_TIMEOUT;
        if (true == start) {
            sendmode = SENDMODE_BURSTKEY_START;
            sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
        }
        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, sendmode, sendvirtualkey_state);
    }
}

void QKeyMapper_Worker::sendBurstKeyUp(const QString &burstKey, bool stop)
{
    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(burstKey);

    if (findindex >=0){
        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
        QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
        int sendmode = SENDMODE_NORMAL;
        int sendvirtualkey_state = SENDVIRTUALKEY_STATE_BURST_TIMEOUT;
        if (true == stop) {
            sendmode = SENDMODE_BURSTKEY_STOP;
            sendvirtualkey_state = SENDVIRTUALKEY_STATE_BURST_STOP;
        }
        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, sendmode, sendvirtualkey_state);
    }
}

void QKeyMapper_Worker::sendBurstKeyDown(int findindex, bool start, QList<MAP_KEYDATA> *keyMappingDataList)
{
    if (findindex >= 0 && keyMappingDataList != Q_NULLPTR && findindex < keyMappingDataList->size()){
        QStringList mappingKeyList = keyMappingDataList->at(findindex).Mapping_Keys;
        QString original_key = keyMappingDataList->at(findindex).Original_Key;
        int sendmode = SENDMODE_NORMAL;
        int sendvirtualkey_state = SENDVIRTUALKEY_STATE_BURST_TIMEOUT;
        if (true == start) {
            sendmode = SENDMODE_BURSTKEY_START;
            sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
        }
        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, sendmode, sendvirtualkey_state, keyMappingDataList);
    }
}

void QKeyMapper_Worker::sendBurstKeyUp(int findindex, bool stop, QList<MAP_KEYDATA> *keyMappingDataList)
{
    if (findindex >= 0 && keyMappingDataList != Q_NULLPTR && findindex < keyMappingDataList->size()){
        QStringList mappingKeyList = keyMappingDataList->at(findindex).Mapping_Keys;
        QString original_key = keyMappingDataList->at(findindex).Original_Key;
        int sendmode = SENDMODE_NORMAL;
        int sendvirtualkey_state = SENDVIRTUALKEY_STATE_BURST_TIMEOUT;
        if (true == stop) {
            sendmode = SENDMODE_BURSTKEY_STOP;
            if (HOOKPROC_STATE_RESTART_STOPPING == s_AtomicHookProcState) {
                sendmode = SENDMODE_BURSTKEY_STOP_ON_HOOKRESTART;
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[sendBurstKeyUp] Burst key stopped on s_AtomicHookProcState = HOOKPROC_STATE_RESTART_STOPPING!");
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            }
            else if (HOOKPROC_STATE_STOPPING == s_AtomicHookProcState) {
                sendmode = SENDMODE_BURSTKEY_STOP_ON_HOOKSTOPPED;
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[sendBurstKeyUp] Burst key stopped on s_AtomicHookProcState = SENDMODE_BURSTKEY_STOP_ON_HOOKSTOPPED!");
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            }
            sendvirtualkey_state = SENDVIRTUALKEY_STATE_BURST_STOP;
        }
        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, sendmode, sendvirtualkey_state, keyMappingDataList);
    }
}

void QKeyMapper_Worker::sendBurstKeyUpForce(int findindex)
{
    if (findindex >= 0){
        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
        QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_FORCE_STOP, SENDVIRTUALKEY_STATE_FORCE);
    }
}

void QKeyMapper_Worker::startDataPortListener()
{
    if (s_GripDetect_EnableState != GRIPDETECT_NONE) {
        int udpDataport = QKeyMapper::getDataPortNumber();
        bool bindRet = m_UdpSocket->bind(QHostAddress::LocalHost, udpDataport);
        Q_UNUSED(bindRet);
#ifdef DEBUG_LOGOUT_ON
        if (!bindRet) {
            qDebug().nospace() << "[DataPortListener]" << "Start DataPortListener on port [" << udpDataport <<"] Failed with Error ->" << m_UdpSocket->errorString();
        }
        else {
            qDebug().nospace() << "[DataPortListener]" << "Start DataPortListener on port [" << udpDataport <<"] Success.";
        }
#endif
    }
}

void QKeyMapper_Worker::stopDataPortListener()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[DataPortListener]" << "Stop DataPortListener at state ->" << m_UdpSocket->state();
#endif
    m_UdpSocket->close();
}

void QKeyMapper_Worker::initGamepadMotion()
{
    m_GamdpadMotion.Reset();

    setGamepadMotionAutoCalibration(true, 0.36f, 0.015f);
    // setGamepadMotionAutoCalibration(false, 0.f, 0.f);
}

void QKeyMapper_Worker::setGamepadMotionAutoCalibration(bool enabled, float gyroThreshold, float accelThreshold)
{
    if (enabled) {
        m_GamdpadMotion.SetCalibrationMode(GamepadMotionHelpers::CalibrationMode::Stillness | GamepadMotionHelpers::CalibrationMode::SensorFusion);
        m_GamdpadMotion.Settings.StillnessGyroDelta = gyroThreshold;
        m_GamdpadMotion.Settings.StillnessAccelDelta = accelThreshold;
    }
    else {
        m_GamdpadMotion.SetCalibrationMode(GamepadMotionHelpers::CalibrationMode::Manual);
    }
}

void QKeyMapper_Worker::sendSpecialVirtualKey(const QString &keycodeString, int keyupdown)
{
    if (KEY_DOWN == keyupdown){
        sendSpecialVirtualKeyDown(keycodeString);
    }
    else {
        sendSpecialVirtualKeyUp(keycodeString);
    }
}

void QKeyMapper_Worker::sendSpecialVirtualKeyDown(const QString &virtualKey)
{
    if (VirtualMouseButtonMap.contains(virtualKey)) {
        V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(virtualKey);
        sendMouseClick(vmousecode, KEY_DOWN);
    }
    else if (VirtualKeyCodeMap.contains(virtualKey)) {
        V_KEYCODE map_vkeycode = VirtualKeyCodeMap.value(virtualKey);
        sendKeyboardInput(map_vkeycode, KEY_DOWN);
    }
}

void QKeyMapper_Worker::sendSpecialVirtualKeyUp(const QString &virtualKey)
{
    if (VirtualMouseButtonMap.contains(virtualKey)) {
        V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(virtualKey);
        sendMouseClick(vmousecode, KEY_UP);
    }
    else if (VirtualKeyCodeMap.contains(virtualKey)) {
        V_KEYCODE map_vkeycode = VirtualKeyCodeMap.value(virtualKey);
        sendKeyboardInput(map_vkeycode, KEY_UP);
    }
}

#ifdef VIGEM_CLIENT_SUPPORT
int QKeyMapper_Worker::ViGEmClient_Alloc()
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient != Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("[ViGEmClient_Alloc] ViGEmClient is already Alloced!!! -> [0x%08X]", s_ViGEmClient);
#endif
        return 0;
    }

    s_ViGEmClient = vigem_alloc();

    if (s_ViGEmClient == nullptr)
    {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[ViGEmClient_Alloc]" << "Failed to alloc ViGEmClient, not enough memory to do that?!!!";
#endif
        return -1;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug("[ViGEmClient_Alloc] ViGEmClient Alloc() Success. -> [0x%08X]", s_ViGEmClient);
#endif

    return 0;
}

int QKeyMapper_Worker::ViGEmClient_Connect()
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient != Q_NULLPTR) {
        const auto retval = vigem_connect(s_ViGEmClient);

        if (!VIGEM_SUCCESS(retval))
        {
            s_ViGEmClient_ConnectState = VIGEMCLIENT_CONNECT_FAILED;
            updateViGEmBusStatus();
#ifdef DEBUG_LOGOUT_ON
            qWarning("[ViGEmClient_Connect] ViGEm Bus connection failed with error code: 0x%08X", retval);
#endif
            return -1;
        }
    }
    else {
        s_ViGEmClient_ConnectState = VIGEMCLIENT_DISCONNECTED;
        updateViGEmBusStatus();
        return -1;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug("[ViGEmClient_Connect] ViGEmClient Connect() Success. -> [0x%08X]", s_ViGEmClient);
#endif

    s_ViGEmClient_ConnectState = VIGEMCLIENT_CONNECT_SUCCESS;
    updateViGEmBusStatus();

    return 0;
}

#if 0
int QKeyMapper_Worker::ViGEmClient_Add()
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient == Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[ViGEmClient_Add]" << "Null Pointer s_ViGEmClient!!!";
#endif
        return -1;
    }

    if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[ViGEmClient_Add]" << "ViGEmClient ConnectState is not Success!!! ->" << s_ViGEmClient_ConnectState;
#endif
        return -1;
    }

    if (s_ViGEmTarget != Q_NULLPTR && vigem_target_is_attached(s_ViGEmTarget)) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("[ViGEmClient_Add] ViGEmTarget is already Attached!!! -> [0x%08X]", s_ViGEmTarget);
#endif
        return 0;
    }

    if (VIRTUAL_GAMEPAD_DS4 == QKeyMapper::getVirtualGamepadType()) {
        s_ViGEmTarget = vigem_target_ds4_alloc();
    }
    else {
        s_ViGEmTarget = vigem_target_x360_alloc();
    }

    if (s_ViGEmTarget == Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("[ViGEmClient_Add] ViGEmTarget Alloc failed with NULLPTR!!!");
#endif
        return -1;
    }

    if (Xbox360Wired == vigem_target_get_type(s_ViGEmTarget)) {
        vigem_target_set_vid(s_ViGEmTarget, VIRTUALGAMPAD_VENDORID_X360);
        vigem_target_set_pid(s_ViGEmTarget, VIRTUALGAMPAD_PRODUCTID_X360);
    }

    //
    // Add client to the bus, this equals a plug-in event
    //
    const auto pir = vigem_target_add(s_ViGEmClient, s_ViGEmTarget);

    //
    // Error handling
    //
    if (!VIGEM_SUCCESS(pir))
    {
        s_ViGEmTarget = Q_NULLPTR;
#ifdef DEBUG_LOGOUT_ON
        qWarning("[ViGEmClient_Add] Target plugin failed with error code: 0x%08X", pir);
#endif
        return -1;
    }

    ULONG index = 255;
    VIGEM_ERROR error;
    Q_UNUSED(index);
    Q_UNUSED(error);
    if (s_ViGEmTarget != Q_NULLPTR) {
        index = vigem_target_get_index(s_ViGEmTarget);
#ifdef DEBUG_LOGOUT_ON
        qDebug("[ViGEmClient_Add] ViGEmTarget Add Success, index(%lu) -> [0x%08X]", index, s_ViGEmTarget);
#endif

        XUSB_REPORT_INIT(&s_ViGEmTarget_Report);
        s_ViGEmTarget_Report.sThumbLY = 1;
        VIGEM_ERROR error;
        if (DualShock4Wired == vigem_target_get_type(s_ViGEmTarget)) {
            DS4_REPORT ds4_report;
            DS4_REPORT_INIT(&ds4_report);
            XUSB_TO_DS4_REPORT(&s_ViGEmTarget_Report, &ds4_report);
            error = vigem_target_ds4_update(s_ViGEmClient, s_ViGEmTarget, ds4_report);
        }
        else {
            error = vigem_target_x360_update(s_ViGEmClient, s_ViGEmTarget, s_ViGEmTarget_Report);
        }
        Q_UNUSED(error);
#ifdef DEBUG_LOGOUT_ON
        if (error != VIGEM_ERROR_NONE) {
            qWarning("[ViGEmClient_Add] Reset VirtualGamepad Report State Failed!!!, Error=0x%08X -> ViGEmTarget[0x%08X]", error, s_ViGEmTarget);
        }
        else {
            qDebug("[ViGEmClient_Add] Reset VirtualGamepad -> ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", s_ViGEmTarget_Report.sThumbLX, s_ViGEmTarget_Report.sThumbLY, s_ViGEmTarget_Report.sThumbRX, s_ViGEmTarget_Report.sThumbRY);
        }
#endif
    }
    else {
        return -1;
    }

    return 0;
}
#endif

PVIGEM_TARGET QKeyMapper_Worker::ViGEmClient_AddTarget_byType(const QString &gamepadtype)
{
    PVIGEM_TARGET addedTarget = Q_NULLPTR;
    QMutexLocker locker(&s_ViGEmClient_Mutex);
    if (s_ViGEmClient == Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[ViGEmClient_AddTarget_byType]" << "Null Pointer s_ViGEmClient!!!";
#endif
        return addedTarget;
    }

    if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[ViGEmClient_AddTarget_byType]" << "ViGEmClient ConnectState is not Success!!! ->" << s_ViGEmClient_ConnectState;
#endif
        return addedTarget;
    }

    if (VIRTUAL_GAMEPAD_DS4 == gamepadtype) {
        addedTarget = vigem_target_ds4_alloc();
    }
    else {
        addedTarget = vigem_target_x360_alloc();
    }

    if (addedTarget == Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("[ViGEmClient_AddTarget_byType] ViGEmTarget Alloc failed with NULLPTR!!!");
#endif
        return addedTarget;
    }

    if (Xbox360Wired == vigem_target_get_type(addedTarget)) {
        vigem_target_set_vid(addedTarget, VIRTUALGAMPAD_VENDORID_X360);
        vigem_target_set_pid(addedTarget, VIRTUALGAMPAD_PRODUCTID_X360);
    }

    //
    // Add client to the bus, this equals a plug-in event
    //
    const auto pir = vigem_target_add(s_ViGEmClient, addedTarget);

    //
    // Error handling
    //
    if (!VIGEM_SUCCESS(pir))
    {
        addedTarget = Q_NULLPTR;
#ifdef DEBUG_LOGOUT_ON
        qWarning("[ViGEmClient_AddTarget_byType] Target plugin failed with error code: 0x%08X", pir);
#endif
        return addedTarget;
    }

    ULONG index = 255;
    VIGEM_ERROR error;
    Q_UNUSED(index);
    Q_UNUSED(error);
    if (addedTarget != Q_NULLPTR) {
        index = vigem_target_get_index(addedTarget);
#ifdef DEBUG_LOGOUT_ON
        QString target_str = QString("0x%1").arg(QString::number((qulonglong)addedTarget, 16).toUpper(), 8, '0');
        qDebug().noquote().nospace() << "[ViGEmClient_AddTarget_byType]" << " ViGEmTarget(" << gamepadtype << ") Add Success, index(" << index << ") -> " << target_str;
#endif
    }
    else {
        return addedTarget;
    }

    return addedTarget;
}

#if 0
void QKeyMapper_Worker::ViGEmClient_Remove()
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient != Q_NULLPTR && s_ViGEmTarget != Q_NULLPTR) {
        if (s_ViGEmClient_ConnectState == VIGEMCLIENT_CONNECT_SUCCESS && vigem_target_is_attached(s_ViGEmTarget)) {
            XUSB_REPORT_INIT(&s_ViGEmTarget_Report);
            VIGEM_ERROR error;
            if (DualShock4Wired == vigem_target_get_type(s_ViGEmTarget)) {
                DS4_REPORT ds4_report;
                DS4_REPORT_INIT(&ds4_report);
                XUSB_TO_DS4_REPORT(&s_ViGEmTarget_Report, &ds4_report);
                error = vigem_target_ds4_update(s_ViGEmClient, s_ViGEmTarget, ds4_report);
            }
            else {
                error = vigem_target_x360_update(s_ViGEmClient, s_ViGEmTarget, s_ViGEmTarget_Report);
            }

            if (error == VIGEM_ERROR_NONE) {
                error = vigem_target_remove(s_ViGEmClient, s_ViGEmTarget);
                if (error == VIGEM_ERROR_NONE) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug("[ViGEmClient_Remove] ViGEmTarget Remove Success. -> [0x%08X]", s_ViGEmTarget);
#endif
                    vigem_target_free(s_ViGEmTarget);
                    s_ViGEmTarget = Q_NULLPTR;
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qWarning("[ViGEmClient_Remove] ViGEmTarget Remove failed!!!, Error=0x%08X -> [0x%08X]", error, s_ViGEmTarget);
#endif
                    s_ViGEmTarget = Q_NULLPTR;
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("[ViGEmClient_Remove] Reset VirtualGamepad Report State Failed!!!, Error=0x%08X -> ViGEmTarget[0x%08X]", error, s_ViGEmTarget);
#endif
                s_ViGEmTarget = Q_NULLPTR;
            }
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning() << "[ViGEmClient_Remove]" << "ViGEmClient ConnectState or ViGEmTarget AttachState error!!! ->" << "ConnectState =" << s_ViGEmClient_ConnectState << ", Attached =" << vigem_target_is_attached(s_ViGEmTarget);
#endif
            s_ViGEmTarget = Q_NULLPTR;
        }
    }
}
#endif

void QKeyMapper_Worker::ViGEmClient_RemoveTarget(PVIGEM_TARGET target)
{
    PVIGEM_TARGET ViGEmTarget = target;
    if (s_ViGEmClient != Q_NULLPTR && ViGEmTarget != Q_NULLPTR) {
        QMutexLocker locker(&s_ViGEmClient_Mutex);
        if (s_ViGEmClient_ConnectState == VIGEMCLIENT_CONNECT_SUCCESS && vigem_target_is_attached(ViGEmTarget)) {
#ifdef DEBUG_LOGOUT_ON
            QString gamepadtype;
            if (DualShock4Wired == vigem_target_get_type(ViGEmTarget)) {
                gamepadtype = VIRTUAL_GAMEPAD_DS4;
            }
            else {
                gamepadtype = VIRTUAL_GAMEPAD_X360;
            }
#endif
            VIGEM_ERROR error;
            error = vigem_target_remove(s_ViGEmClient, ViGEmTarget);
            if (error == VIGEM_ERROR_NONE) {
#ifdef DEBUG_LOGOUT_ON
                QString target_str = QString("0x%1").arg(QString::number((qulonglong)ViGEmTarget, 16).toUpper(), 8, '0');
                qDebug().noquote().nospace() << "[ViGEmClient_RemoveTarget]" << " ViGEmTarget(" << gamepadtype << ") Remove Success. -> " << target_str;
#endif
                vigem_target_free(ViGEmTarget);
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("[ViGEmClient_RemoveTarget] ViGEmTarget Remove failed!!!, Error=0x%08X -> [0x%08X]", error, ViGEmTarget);
#endif
            }
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning() << "[ViGEmClient_RemoveTarget]" << "ViGEmClient ConnectState or ViGEmTarget AttachState error!!! ->" << "ConnectState =" << s_ViGEmClient_ConnectState << ", Attached =" << vigem_target_is_attached(ViGEmTarget);
#endif
        }
    }
}

void QKeyMapper_Worker::ViGEmClient_RemoveAllTargets()
{
    if (s_ViGEmTargetList.size() > 0 ) {
        int gamepad_index = s_ViGEmTargetList.size() - 1;
        for (auto it = s_ViGEmTargetList.rbegin(); it != s_ViGEmTargetList.rend(); ++it) {
            const PVIGEM_TARGET &target_toremove = *it;
            if (target_toremove != Q_NULLPTR) {
                ViGEmClient_GamepadReset_byIndex(gamepad_index);
                ViGEmClient_RemoveTarget(target_toremove);
            }
            gamepad_index--;
        }
    }

    s_ViGEmTargetList.clear();
    s_ViGEmTarget_ReportList.clear();
}

void QKeyMapper_Worker::ViGEmClient_Disconnect()
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient != Q_NULLPTR) {
        vigem_disconnect(s_ViGEmClient);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[ViGEmClient_Disconnect]" << "ViGEmClient Disconnect() Success.";
#endif
    }
    s_ViGEmClient_ConnectState = VIGEMCLIENT_DISCONNECTED;
    updateViGEmBusStatus();
}

void QKeyMapper_Worker::ViGEmClient_Free()
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient != Q_NULLPTR) {
        vigem_free(s_ViGEmClient);
        s_ViGEmClient = Q_NULLPTR;

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[ViGEmClient_Disconnect]" << "ViGEmClient Free() Success.";
#endif
    }
    s_ViGEmClient_ConnectState = VIGEMCLIENT_DISCONNECTED;
    updateViGEmBusStatus();
}

void QKeyMapper_Worker::saveVirtualGamepadList()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[saveVirtualGamepadList] Save VirtualGamepadList ->" << s_VirtualGamepadList;
#endif

    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    settingFile.setValue(VIRTUAL_GAMEPADLIST, s_VirtualGamepadList);
}

void QKeyMapper_Worker::loadVirtualGamepadList(const QStringList &gamepadlist)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[loadVirtualGamepadList] load VirtualGamepadList ->" << gamepadlist;
#endif

    s_VirtualGamepadList = gamepadlist;
}

void QKeyMapper_Worker::updateViGEmBusStatus()
{
    if (s_isWorkerDestructing) {
        return;
    }
    emit QKeyMapper::getInstance()->updateViGEmBusStatus_Signal();
}

void QKeyMapper_Worker::updateLockStatus()
{
    if (s_isWorkerDestructing) {
        return;
    }
    emit QKeyMapper::getInstance()->updateLockStatus_Signal();
}

QKeyMapper_Worker::ViGEmClient_ConnectState QKeyMapper_Worker::ViGEmClient_getConnectState()
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    return s_ViGEmClient_ConnectState;
}

void QKeyMapper_Worker::ViGEmClient_setConnectState(ViGEmClient_ConnectState connectstate)
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    s_ViGEmClient_ConnectState = connectstate;
}

void QKeyMapper_Worker::ViGEmClient_PressButton(const QString &joystickButton, int autoAdjust, int gamepad_index, int player_index)
{
    int gamepad_count = s_ViGEmTargetList.size();
    int gamepad_report_count = s_ViGEmTarget_ReportList.size();

    if (gamepad_index >= gamepad_count || gamepad_index >= gamepad_report_count) {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[ViGEmClient_PressButton]" << "VirtualGamepad Index Error! ->" << gamepad_index;
#endif
        return;
    }

    static QRegularExpression vjoy_pushlevel_keys_regex(R"(^vJoy-(Key11\(LT\)|Key12\(RT\)|(?:LS|RS)-(?:Up|Down|Left|Right|Radius))(?:\[(\d{1,3})\])?$)");
    QRegularExpressionMatch vjoy_pushlevel_keys_match = vjoy_pushlevel_keys_regex.match(joystickButton);
    PVIGEM_TARGET ViGEmTarget = s_ViGEmTargetList.at(gamepad_index);
    ViGEm_ReportData& reportData = s_ViGEmTarget_ReportList[gamepad_index];
    XUSB_REPORT& ViGEmTarget_Report = reportData.xusb_report;
    OrderedMap<QString, BYTE>& pressedvJoyLStickKeys_ref = pressedvJoyLStickKeysList[gamepad_index];
    OrderedMap<QString, BYTE>& pressedvJoyRStickKeys_ref = pressedvJoyRStickKeysList[gamepad_index];
    QStringList& pressedvJoyButtons_ref = pressedvJoyButtonsList[gamepad_index];

    {
    QMutexLocker locker(&s_ViGEmClient_Mutex);
    if (s_ViGEmClient != Q_NULLPTR && ViGEmTarget != Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        if (autoAdjust) {
#ifdef GRIP_VERBOSE_LOG
            qDebug().noquote().nospace() << "[ViGEmClient]" << " VirtualGamepad LT&RT Auto Adjust Update.";
#endif
        }
        else {
            qDebug().noquote().nospace() << "[ViGEmClient]" << " VirtualGamepad(" << gamepad_index << ") Button Press [" << joystickButton << "]";
        }
#endif

        if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[ViGEmClient_PressButton] ViGEmClient is not Connected(%d)!!! -> [0x%08X]", s_ViGEmClient_ConnectState, s_ViGEmClient);
#endif
            return;
        }

        if (vigem_target_is_attached(ViGEmTarget) != TRUE) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[ViGEmClient_PressButton] ViGEmTarget(%d) is not Attached!!! -> [0x%08X]", gamepad_index, ViGEmTarget);
#endif
            return;
        }

        int updateFlag = VJOY_UPDATE_NONE;
        if (joystickButton.isEmpty() && autoAdjust) {
            if (autoAdjust & AUTO_ADJUST_LT) {
                Joystick_AxisState JoyAxisState = s_JoyAxisStateMap.value(player_index);
                // Map -1.0~1.0 to 0~255 for BYTE representation
                ViGEmTarget_Report.bLeftTrigger = static_cast<BYTE>((JoyAxisState.left_trigger + 1.0) * 127.5 + 0.5);
                updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
            }
            else if (autoAdjust & AUTO_ADJUST_RT) {
                Joystick_AxisState JoyAxisState = s_JoyAxisStateMap.value(player_index);
                // Map -1.0~1.0 to 0~255 for BYTE representation
                ViGEmTarget_Report.bRightTrigger = static_cast<BYTE>((JoyAxisState.right_trigger + 1.0) * 127.5 + 0.5);
                updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
            }
            else {
                if (autoAdjust & AUTO_ADJUST_BRAKE) {
                    if (pressedvJoyButtons_ref.contains(VJOY_LT_BRAKE_STR)) {
                        if (s_last_Auto_Brake != s_Auto_Brake) {
                            ViGEmTarget_Report.bLeftTrigger = s_Auto_Brake;
                            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
                        }
                    }
                    if (pressedvJoyButtons_ref.contains(VJOY_RT_BRAKE_STR)) {
                        if (s_last_Auto_Brake != s_Auto_Brake) {
                            ViGEmTarget_Report.bRightTrigger = s_Auto_Brake;
                            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
                        }
                    }
                }

                if (autoAdjust & AUTO_ADJUST_ACCEL) {
                    if (pressedvJoyButtons_ref.contains(VJOY_LT_ACCEL_STR)) {
                        if (s_last_Auto_Accel != s_Auto_Accel) {
                            ViGEmTarget_Report.bLeftTrigger = s_Auto_Accel;
                            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
                        }
                    }
                    if (pressedvJoyButtons_ref.contains(VJOY_RT_ACCEL_STR)) {
                        if (s_last_Auto_Accel != s_Auto_Accel) {
                            ViGEmTarget_Report.bRightTrigger = s_Auto_Accel;
                            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
                        }
                    }
                }
            }
        }
        else if (vjoy_pushlevel_keys_match.hasMatch()) {
            QString pushlevelKeyStr = vjoy_pushlevel_keys_match.captured(1);
            QString pushlevelString = vjoy_pushlevel_keys_match.captured(2);
            int pushlevel = VJOY_PUSHLEVEL_MAX;
            if (!pushlevelString.isEmpty()) {
                bool ok = true;
                pushlevel = pushlevelString.toInt(&ok);
                if (!ok || pushlevelString == "0" || pushlevelString.startsWith('0') || pushlevel <= VJOY_PUSHLEVEL_MIN || pushlevel >= VJOY_PUSHLEVEL_MAX) {
                    pushlevel = VJOY_PUSHLEVEL_MAX;
                }
            }

            BYTE pushlevel_byte = static_cast<BYTE>(pushlevel);
            if (pushlevelKeyStr.startsWith("LS-")) {
                if (pushlevelKeyStr.startsWith("LS-Radius")) {
                    reportData.custom_radius_ls = pushlevel;
                }
                pressedvJoyLStickKeys_ref.insert(pushlevelKeyStr, pushlevel_byte);
                ViGEmClient_CheckJoysticksReportData(gamepad_index);
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (pushlevelKeyStr.startsWith("RS-")) {
                if (pushlevelKeyStr.startsWith("RS-Radius")) {
                    reportData.custom_radius_rs = pushlevel;
                }
                pressedvJoyRStickKeys_ref.insert(pushlevelKeyStr, pushlevel_byte);
                ViGEmClient_CheckJoysticksReportData(gamepad_index);
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (pushlevelKeyStr.endsWith("(LT)")) {
                ViGEmTarget_Report.bLeftTrigger = pushlevel_byte;
                updateFlag = VJOY_UPDATE_BUTTONS;
            }
            else if (pushlevelKeyStr.endsWith("(RT)")) {
                ViGEmTarget_Report.bRightTrigger = pushlevel_byte;
                updateFlag = VJOY_UPDATE_BUTTONS;
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug().noquote().nospace() << "[ViGEmClient_PressButton]" << " VirtualGamepad(" << gamepad_index << ") PushLevel Button Press [" << joystickButton << "], but updateFlag is not set!!! ";
#endif
            }
        }
        else if (ViGEmButtonMap.contains(joystickButton)) {
            XUSB_BUTTON button = ViGEmButtonMap.value(joystickButton);

            ViGEmTarget_Report.wButtons = ViGEmTarget_Report.wButtons | button;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton == VJOY_LT_BRAKE_STR) {
            ViGEmTarget_Report.bLeftTrigger = s_Auto_Brake;
            updateFlag = VJOY_UPDATE_BUTTONS;
            s_last_Auto_Brake = s_Auto_Brake;
        }
        else if (joystickButton == VJOY_RT_BRAKE_STR) {
            ViGEmTarget_Report.bRightTrigger = s_Auto_Brake;
            updateFlag = VJOY_UPDATE_BUTTONS;
            s_last_Auto_Brake = s_Auto_Brake;
        }
        else if (joystickButton == VJOY_LT_ACCEL_STR) {
            ViGEmTarget_Report.bLeftTrigger = s_Auto_Accel;
            updateFlag = VJOY_UPDATE_BUTTONS;
            s_last_Auto_Accel = s_Auto_Accel;
        }
        else if (joystickButton == VJOY_RT_ACCEL_STR) {
            ViGEmTarget_Report.bRightTrigger = s_Auto_Accel;
            updateFlag = VJOY_UPDATE_BUTTONS;
            s_last_Auto_Accel = s_Auto_Accel;
        }

        if (updateFlag) {
            VIGEM_ERROR error;
            if (DualShock4Wired == vigem_target_get_type(ViGEmTarget)) {
                DS4_REPORT ds4_report;
                DS4_REPORT_INIT(&ds4_report);
                XUSB_TO_DS4_REPORT(&ViGEmTarget_Report, &ds4_report);
                error = vigem_target_ds4_update(s_ViGEmClient, ViGEmTarget, ds4_report);
            }
            else {
                error = vigem_target_x360_update(s_ViGEmClient, ViGEmTarget, ViGEmTarget_Report);
            }
            Q_UNUSED(error);

            if (error == VIGEM_ERROR_NONE) {
                if (VJOY_UPDATE_BUTTONS == updateFlag) {
                    if (false == pressedvJoyButtons_ref.contains(joystickButton)){
                        pressedvJoyButtons_ref.append(joystickButton);
#ifdef DEBUG_LOGOUT_ON
                        qDebug().noquote().nospace() << "[pressedvJoyButtonsList](" << gamepad_index << ") Button Press" << " : Current Pressed vJoyButtons -> " << pressedvJoyButtons_ref;
#endif
                    }
                }
                else if (VJOY_UPDATE_AUTO_BUTTONS == updateFlag) {
                    if (autoAdjust & AUTO_ADJUST_BRAKE) {
                        s_last_Auto_Brake = s_Auto_Brake;
                    }

                    if (autoAdjust & AUTO_ADJUST_ACCEL) {
                        s_last_Auto_Accel = s_Auto_Accel;
                    }
                }
#ifdef JOYSTICK_VERBOSE_LOG
                qDebug("[ViGEmClient_Button](%d) Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d], LeftTrigger[%d], RightTrigger[%d]", gamepad_index, ViGEmTarget_Report.sThumbLX, ViGEmTarget_Report.sThumbLY, ViGEmTarget_Report.sThumbRX, ViGEmTarget_Report.sThumbRY, ViGEmTarget_Report.bLeftTrigger, ViGEmTarget_Report.bRightTrigger);
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug("[ViGEmClient_Button] Button Press Return code: 0x%08X", error);
#endif
            }
        }
    }
    }
}

void QKeyMapper_Worker::ViGEmClient_ReleaseButton(const QString &joystickButton, int gamepad_index)
{
    int gamepad_count = s_ViGEmTargetList.size();
    int gamepad_report_count = s_ViGEmTarget_ReportList.size();

    if (gamepad_index >= gamepad_count || gamepad_index >= gamepad_report_count) {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[ViGEmClient_ReleaseButton]" << "VirtualGamepad Index Error! ->" << gamepad_index;
#endif
        return;
    }

    static QRegularExpression vjoy_pushlevel_keys_regex(R"(^vJoy-(Key11\(LT\)|Key12\(RT\)|(?:LS|RS)-(?:Up|Down|Left|Right|Radius))(?:\[(\d{1,3})\])?$)");
    QRegularExpressionMatch vjoy_pushlevel_keys_match = vjoy_pushlevel_keys_regex.match(joystickButton);
    PVIGEM_TARGET ViGEmTarget = s_ViGEmTargetList.at(gamepad_index);
    ViGEm_ReportData& reportData = s_ViGEmTarget_ReportList[gamepad_index];
    XUSB_REPORT& ViGEmTarget_Report = reportData.xusb_report;
    OrderedMap<QString, BYTE>& pressedvJoyLStickKeys_ref = pressedvJoyLStickKeysList[gamepad_index];
    OrderedMap<QString, BYTE>& pressedvJoyRStickKeys_ref = pressedvJoyRStickKeysList[gamepad_index];
    QStringList& pressedvJoyButtons_ref = pressedvJoyButtonsList[gamepad_index];

    {
    QMutexLocker locker(&s_ViGEmClient_Mutex);
    if (s_ViGEmClient != Q_NULLPTR && ViGEmTarget != Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().noquote().nospace() << "[ViGEmClient]" << " VirtualGamepad Button Release [" << joystickButton << "]";
#endif

        if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[ViGEmClient_ReleaseButton] ViGEmClient is not Connected(%d)!!! -> [0x%08X]", s_ViGEmClient_ConnectState, s_ViGEmClient);
#endif
            return;
        }

        if (vigem_target_is_attached(ViGEmTarget) != TRUE) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[ViGEmClient_ReleaseButton] ViGEmTarget(%d) is not Attached!!! -> [0x%08X]", gamepad_index, ViGEmTarget);
#endif
            return;
        }

        int updateFlag = VJOY_UPDATE_NONE;
        if (vjoy_pushlevel_keys_match.hasMatch()) {
            QString pushlevelKeyStr = vjoy_pushlevel_keys_match.captured(1);
            if (pushlevelKeyStr.startsWith("LS-")) {
                if (pushlevelKeyStr.startsWith("LS-Radius")) {
                    reportData.custom_radius_ls = VJOY_STICK_RADIUS_MAX;
                }
                pressedvJoyLStickKeys_ref.remove(pushlevelKeyStr);
                ViGEmClient_CheckJoysticksReportData(gamepad_index);
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (pushlevelKeyStr.startsWith("RS-")) {
                if (pushlevelKeyStr.startsWith("RS-Radius")) {
                    reportData.custom_radius_rs = VJOY_STICK_RADIUS_MAX;
                }
                pressedvJoyRStickKeys_ref.remove(pushlevelKeyStr);
                ViGEmClient_CheckJoysticksReportData(gamepad_index);
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (pushlevelKeyStr.endsWith("(LT)")) {
                ViGEmTarget_Report.bLeftTrigger = XINPUT_TRIGGER_MIN;
                updateFlag = VJOY_UPDATE_BUTTONS;
            }
            else if (pushlevelKeyStr.endsWith("(RT)")) {
                ViGEmTarget_Report.bRightTrigger = XINPUT_TRIGGER_MIN;
                updateFlag = VJOY_UPDATE_BUTTONS;
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug().noquote().nospace() << "[ViGEmClient_ReleaseButton]" << " VirtualGamepad(" << gamepad_index << ") PushLevel Button Release [" << joystickButton << "], but updateFlag is not set!!! ";
#endif
            }
        }
        else if (ViGEmButtonMap.contains(joystickButton)) {
            XUSB_BUTTON button = ViGEmButtonMap.value(joystickButton);

            ViGEmTarget_Report.wButtons = ViGEmTarget_Report.wButtons & ~button;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton == VJOY_LT_BRAKE_STR) {
            ViGEmTarget_Report.bLeftTrigger = XINPUT_TRIGGER_MIN;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton == VJOY_RT_BRAKE_STR) {
            ViGEmTarget_Report.bRightTrigger = XINPUT_TRIGGER_MIN;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton == VJOY_LT_ACCEL_STR) {
            ViGEmTarget_Report.bLeftTrigger = XINPUT_TRIGGER_MIN;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton == VJOY_RT_ACCEL_STR) {
            ViGEmTarget_Report.bRightTrigger = XINPUT_TRIGGER_MIN;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }

        if (updateFlag) {
            VIGEM_ERROR error;
            if (DualShock4Wired == vigem_target_get_type(ViGEmTarget)) {
                DS4_REPORT ds4_report;
                DS4_REPORT_INIT(&ds4_report);
                XUSB_TO_DS4_REPORT(&ViGEmTarget_Report, &ds4_report);
                error = vigem_target_ds4_update(s_ViGEmClient, ViGEmTarget, ds4_report);
            }
            else {
                error = vigem_target_x360_update(s_ViGEmClient, ViGEmTarget, ViGEmTarget_Report);
            }
            Q_UNUSED(error);

            if (error == VIGEM_ERROR_NONE) {
                if (VJOY_UPDATE_BUTTONS == updateFlag) {
                    pressedvJoyButtons_ref.removeAll(joystickButton);
#ifdef DEBUG_LOGOUT_ON
                    qDebug().noquote().nospace() << "[pressedvJoyButtonsList](" << gamepad_index << ") Button Release" << " : Current Pressed vJoyButtons -> " << pressedvJoyButtons_ref;
                    qDebug("[ViGEmClient_Button](%d) Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", gamepad_index, ViGEmTarget_Report.sThumbLX, ViGEmTarget_Report.sThumbLY, ViGEmTarget_Report.sThumbRX, ViGEmTarget_Report.sThumbRY);
#endif
                    if (joystickButton == VJOY_LT_BRAKE_STR || joystickButton == VJOY_RT_BRAKE_STR) {
                        s_Auto_Brake = AUTO_BRAKE_DEFAULT;
                        s_last_Auto_Brake = 0;
                    }
                    else if (joystickButton == VJOY_LT_ACCEL_STR || joystickButton == VJOY_RT_ACCEL_STR) {
                        s_Auto_Accel = AUTO_ACCEL_DEFAULT;
                        s_last_Auto_Accel = 0;
                    }
                }
#ifdef JOYSTICK_VERBOSE_LOG
                qDebug("[ViGEmClient_Button] Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", ViGEmTarget_Report.sThumbLX, ViGEmTarget_Report.sThumbLY, ViGEmTarget_Report.sThumbRX, ViGEmTarget_Report.sThumbRY);
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug("[ViGEmClient_Button] Button Release Return code: 0x%08X", error);
#endif
            }
        }
    }
    }
}

void QKeyMapper_Worker::ViGEmClient_CheckJoysticksReportData(int gamepad_index)
{
    OrderedMap<QString, BYTE>& pressedvJoyLStickKeys_ref = pressedvJoyLStickKeysList[gamepad_index];
    OrderedMap<QString, BYTE>& pressedvJoyRStickKeys_ref = pressedvJoyRStickKeysList[gamepad_index];
    ViGEm_ReportData& reportData = s_ViGEmTarget_ReportList[gamepad_index];
    XUSB_REPORT& ViGEmTarget_Report = reportData.xusb_report;

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[ViGEmClient_CheckJoysticksReportData](" << gamepad_index << ") vJoyLSPressedKeys ->" << pressedvJoyLStickKeys_ref;
    qDebug().nospace() << "[ViGEmClient_CheckJoysticksReportData](" << gamepad_index << ") vJoyRSPressedKeys ->" << pressedvJoyRStickKeys_ref;
#endif

    // Reset thumb values
    ViGEmTarget_Report.sThumbLX = XINPUT_THUMB_RELEASE;
    ViGEmTarget_Report.sThumbLY = XINPUT_THUMB_RELEASE;
    ViGEmTarget_Report.sThumbRX = XINPUT_THUMB_RELEASE;
    ViGEmTarget_Report.sThumbRY = XINPUT_THUMB_RELEASE;

    // Update thumb values based on pressed keys
    int custom_radius_ls = VJOY_STICK_RADIUS_MAX;
    if (VJOY_STICK_RADIUS_MIN < reportData.custom_radius_ls && reportData.custom_radius_ls < VJOY_STICK_RADIUS_MAX) {
        custom_radius_ls = reportData.custom_radius_ls;
    }
    QStringList pressedLStickKeysList = pressedvJoyLStickKeys_ref.keys();
    for (const QString &key : std::as_const(pressedLStickKeysList)) {
        int pushlevel = pressedvJoyLStickKeys_ref.value(key);
        // Convert BYTE pushlevel (0~255) to SHORT (-32768~32767)
        SHORT scaledValue = static_cast<SHORT>(pushlevel * XINPUT_THUMB_MAX / VJOY_PUSHLEVEL_MAX);

        if (key == "LS-Up") {
            ViGEmTarget_Report.sThumbLY = scaledValue;
        } else if (key == "LS-Down") {
            ViGEmTarget_Report.sThumbLY = (pushlevel == VJOY_PUSHLEVEL_MAX) ? XINPUT_THUMB_MIN : -scaledValue;
        } else if (key == "LS-Left") {
            ViGEmTarget_Report.sThumbLX = (pushlevel == VJOY_PUSHLEVEL_MAX) ? XINPUT_THUMB_MIN : -scaledValue;
        } else if (key == "LS-Right") {
            ViGEmTarget_Report.sThumbLX = scaledValue;
        }
    }

    int custom_radius_rs = VJOY_STICK_RADIUS_MAX;
    if (VJOY_STICK_RADIUS_MIN < reportData.custom_radius_rs && reportData.custom_radius_rs < VJOY_STICK_RADIUS_MAX) {
        custom_radius_rs = reportData.custom_radius_rs;
    }
    QStringList pressedRStickKeysList = pressedvJoyRStickKeys_ref.keys();
    for (const QString &key : std::as_const(pressedRStickKeysList)) {
        int pushlevel = pressedvJoyRStickKeys_ref.value(key);
        // Convert BYTE pushlevel (0~255) to SHORT (-32768~32767)
        SHORT scaledValue = static_cast<SHORT>(pushlevel * XINPUT_THUMB_MAX / VJOY_PUSHLEVEL_MAX);

        if (key == "RS-Up") {
            ViGEmTarget_Report.sThumbRY = scaledValue;
        } else if (key == "RS-Down") {
            ViGEmTarget_Report.sThumbRY = (pushlevel == VJOY_PUSHLEVEL_MAX) ? XINPUT_THUMB_MIN : -scaledValue;
        } else if (key == "RS-Left") {
            ViGEmTarget_Report.sThumbRX = (pushlevel == VJOY_PUSHLEVEL_MAX) ? XINPUT_THUMB_MIN : -scaledValue;
        } else if (key == "RS-Right") {
            ViGEmTarget_Report.sThumbRX = scaledValue;
        }
    }

    if (ViGEmTarget_Report.sThumbLX != XINPUT_THUMB_RELEASE
        || ViGEmTarget_Report.sThumbLY != XINPUT_THUMB_RELEASE) {
        ViGEmClient_CalculateThumbValue(&ViGEmTarget_Report.sThumbLX, &ViGEmTarget_Report.sThumbLY, custom_radius_ls);
    }

    if (ViGEmTarget_Report.sThumbRX != XINPUT_THUMB_RELEASE
        || ViGEmTarget_Report.sThumbRY != XINPUT_THUMB_RELEASE) {
        ViGEmClient_CalculateThumbValue(&ViGEmTarget_Report.sThumbRX, &ViGEmTarget_Report.sThumbRY, custom_radius_rs);
    }

}

void QKeyMapper_Worker::ViGEmClient_CalculateThumbValue(SHORT *ori_ThumbX, SHORT *ori_ThumbY, uint custom_radius)
{
    SHORT ThumbX = *ori_ThumbX;
    SHORT ThumbY = *ori_ThumbY;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    qreal direction = qAtan2(ThumbY, ThumbX);
#else
    qreal direction = std::atan2(ThumbY, ThumbX);
#endif
    qint64 sumOfSquares = static_cast<qint64>(ThumbX) * static_cast<qint64>(ThumbX) + static_cast<qint64>(ThumbY) * static_cast<qint64>(ThumbY);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    qreal distance = qSqrt(static_cast<qreal>(sumOfSquares));
#else
    qreal distance = std::sqrt(static_cast<qreal>(sumOfSquares));
#endif

    if (distance > THUMB_DISTANCE_MAX) {
        qreal scale = THUMB_DISTANCE_MAX / distance;
        ThumbX = static_cast<SHORT>(ThumbX * scale);
        ThumbY = static_cast<SHORT>(ThumbY * scale);
        distance = THUMB_DISTANCE_MAX;

        // Recalculate direction after scaling ThumbX and ThumbY
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        direction = qAtan2(ThumbY, ThumbX);
#else
        direction = std::atan2(ThumbY, ThumbX);
#endif
    }

    // Only scale when custom_radius is valid
    if (VJOY_STICK_RADIUS_MIN < custom_radius && custom_radius < VJOY_STICK_RADIUS_MAX) {
        qreal ratio = static_cast<qreal>(custom_radius) / VJOY_STICK_RADIUS_MAX;
        qreal max_radius = THUMB_DISTANCE_MAX * ratio;
        if (distance > max_radius) {
            distance = max_radius;
        } else {
            distance = distance * ratio;
        }
    }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    SHORT newThumbX = static_cast<SHORT>(qRound(distance * qCos(direction)));
    SHORT newThumbY = static_cast<SHORT>(qRound(distance * qSin(direction)));
#else
    SHORT newThumbX = static_cast<SHORT>(std::round(distance * std::cos(direction)));
    SHORT newThumbY = static_cast<SHORT>(std::round(distance * std::sin(direction)));
#endif

#ifdef JOYSTICK_VERBOSE_LOG
    qDebug("[ViGEmClient_CalculateThumbValue] ori_ThumbX[%d], ori_ThumbY[%d], custom_radius[%d] -> Calculated ThumbX[%d], ThumbY[%d]", *ori_ThumbX, *ori_ThumbY, custom_radius, newThumbX, newThumbY);
#endif

    *ori_ThumbX = newThumbX;
    *ori_ThumbY = newThumbY;
}

#if 0
QKeyMapper_Worker::Mouse2vJoyStates QKeyMapper_Worker::ViGEmClient_checkMouse2JoystickEnableState()
{
    if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
        return MOUSE2VJOY_NONE;
    }

    if (s_ViGEmClient == Q_NULLPTR || s_ViGEmTarget == Q_NULLPTR) {
        return MOUSE2VJOY_NONE;
    }

    if (vigem_target_is_attached(s_ViGEmTarget) != TRUE) {
        return MOUSE2VJOY_NONE;
    }

    Mouse2vJoyStates mouse2joy_enablestate = MOUSE2VJOY_NONE;
    bool leftJoystickUpdate = false;
    bool rightJoystickUpdate = false;

    int findMouse2LSindex = QKeyMapper::findOriKeyInKeyMappingDataList(VJOY_MOUSE2LS_STR);
    if (findMouse2LSindex >=0){
        leftJoystickUpdate = true;
    }

    int findMouse2RSindex = QKeyMapper::findOriKeyInKeyMappingDataList(VJOY_MOUSE2RS_STR);
    if (findMouse2RSindex >=0){
        rightJoystickUpdate = true;
    }

    if (leftJoystickUpdate)
    {
        mouse2joy_enablestate |= MOUSE2VJOY_LEFT;
    }

    if (rightJoystickUpdate)
    {
        mouse2joy_enablestate |= MOUSE2VJOY_RIGHT;
    }

    return mouse2joy_enablestate;
}
#endif

QHash<int, QKeyMapper_Worker::Mouse2vJoyData> QKeyMapper_Worker::ViGEmClient_checkMouse2JoystickEnableStateMap()
{
    QHash<int, Mouse2vJoyData> Mouse2vJoy_EnableStateMap;

    if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
        return Mouse2vJoy_EnableStateMap;
    }

    if (s_ViGEmClient == Q_NULLPTR) {
        return Mouse2vJoy_EnableStateMap;
    }

    if (s_ViGEmTargetList.isEmpty()) {
        return Mouse2vJoy_EnableStateMap;
    }

    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList)) {
        if (keymapdata.Original_Key == VJOY_MOUSE2LS_STR
            || keymapdata.Original_Key == VJOY_MOUSE2RS_STR) {
            if (keymapdata.Original_Key == VJOY_MOUSE2LS_STR) {
                Mouse2vJoy_EnableStateMap[INITIAL_MOUSE_INDEX].states |= MOUSE2VJOY_LEFT;
            }
            else {
                Mouse2vJoy_EnableStateMap[INITIAL_MOUSE_INDEX].states |= MOUSE2VJOY_RIGHT;
            }

            QString mappingkey = keymapdata.Mapping_Keys.constFirst();
            int gamepad_index = 0;
            static QRegularExpression mapkey_regex("vJoy-Mouse2(L|R)S@([0-3])$");
            QRegularExpressionMatch mapkey_match = mapkey_regex.match(mappingkey);
            if (mapkey_match.hasMatch()) {
                QString gamepadIndexString = mapkey_match.captured(2);
                gamepad_index = gamepadIndexString.toInt();
            }

            if (gamepad_index > 0 && Mouse2vJoy_EnableStateMap[INITIAL_MOUSE_INDEX].gamepad_index == 0) {
                Mouse2vJoy_EnableStateMap[INITIAL_MOUSE_INDEX].gamepad_index = gamepad_index;
            }
        }
        else {
            static QRegularExpression regex(R"(vJoy-Mouse2(L|R)S@(\d))");
            QRegularExpressionMatch match = regex.match(keymapdata.Original_Key);
            if (match.hasMatch()) {
                int number = match.captured(2).toInt();
                if (match.captured(1) == "L") {
                    Mouse2vJoy_EnableStateMap[number].states |= MOUSE2VJOY_LEFT;
                } else if (match.captured(1) == "R") {
                    Mouse2vJoy_EnableStateMap[number].states |= MOUSE2VJOY_RIGHT;
                }

                QString mappingkey = keymapdata.Mapping_Keys.constFirst();
                int gamepad_index = 0;
                static QRegularExpression mapkey_regex("vJoy-Mouse2(L|R)S@([0-3])$");
                QRegularExpressionMatch mapkey_match = mapkey_regex.match(mappingkey);
                if (mapkey_match.hasMatch()) {
                    QString gamepadIndexString = mapkey_match.captured(2);
                    gamepad_index = gamepadIndexString.toInt();
                }

                if (gamepad_index > 0 && Mouse2vJoy_EnableStateMap[number].gamepad_index == 0) {
                    Mouse2vJoy_EnableStateMap[number].gamepad_index = gamepad_index;
                }
            }
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[ViGEmClient_checkMouse2JoystickEnableStateMap]" << "Mouse2vJoy_EnableStateMap ->" << Mouse2vJoy_EnableStateMap;
#endif

    return Mouse2vJoy_EnableStateMap;
}

void QKeyMapper_Worker::ViGEmClient_Mouse2JoystickUpdate(int delta_x, int delta_y, int mouse_index, int gamepad_index)
{
    if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
        return;
    }

    if (s_ViGEmClient == Q_NULLPTR) {
        return;
    }

    int gamepad_count = s_ViGEmTargetList.size();
    int gamepad_report_count = s_ViGEmTarget_ReportList.size();

    if (gamepad_index >= gamepad_count || gamepad_index >= gamepad_report_count) {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[ViGEmClient_Mouse2JoystickUpdate]" << "VirtualGamepad Index Error! ->" << gamepad_index;
#endif
        return;
    }

    PVIGEM_TARGET ViGEmTarget = s_ViGEmTargetList.at(gamepad_index);
    ViGEm_ReportData& reportData = s_ViGEmTarget_ReportList[gamepad_index];
    XUSB_REPORT& ViGEmTarget_Report = reportData.xusb_report;

    if (ViGEmTarget == Q_NULLPTR || vigem_target_is_attached(ViGEmTarget) != TRUE) {
        return;
    }

    Mouse2vJoyStates Mouse2vJoy_EnableState = s_Mouse2vJoy_EnableStateMap.value(mouse_index).states;
    bool leftJoystickUpdate = false;
    bool rightJoystickUpdate = false;

    if (Mouse2vJoy_EnableState & MOUSE2VJOY_LEFT) {
        leftJoystickUpdate = true;
    }

    if (Mouse2vJoy_EnableState & MOUSE2VJOY_RIGHT) {
        rightJoystickUpdate = true;
    }

    if (leftJoystickUpdate || rightJoystickUpdate) {
        int vJoy_X_Sensitivity = QKeyMapper::getvJoyXSensitivity();
        int vJoy_Y_Sensitivity = QKeyMapper::getvJoyYSensitivity();
        short leftX = 0;
        short leftY = 0;

        if (QKeyMapper::getvJoyDirectModeStatus()) {
            int direct_x;
            int direct_y;
            if (leftJoystickUpdate) {
                direct_x = ViGEmTarget_Report.sThumbLX;
                direct_y = ViGEmTarget_Report.sThumbLY;
            }
            else {
                direct_x = ViGEmTarget_Report.sThumbRX;
                direct_y = ViGEmTarget_Report.sThumbRY;
            }

            direct_x += delta_x * vJoy_X_Sensitivity;
            direct_y -= delta_y * vJoy_X_Sensitivity;

            // Clamp values to the joystick range (-32767, 32767)
            if (direct_x > 32767) {
                direct_x = 32767;
            }
            else if (direct_x < -32767) {
                direct_x = -32767;
            }
            if (direct_y > 32767) {
                direct_y = 32767;
            }
            else if (direct_y < -32767) {
                direct_y = -32767;
            }

            // Assign final joystick values
            leftX = direct_x;
            leftY = direct_y;
        }
        else {
            // Mouse2Joystick core algorithm from "https://github.com/memethyl/Mouse2Joystick" >>>
            int adjustedXSensitivity = VIRTUAL_JOYSTICK_SENSITIVITY_MAX / vJoy_X_Sensitivity;
            int adjustedYSensitivity = VIRTUAL_JOYSTICK_SENSITIVITY_MAX / vJoy_Y_Sensitivity;
            #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            qreal x = -qExp((-1.0 / adjustedXSensitivity) * qAbs(delta_x)) + 1.0;
            qreal y = -qExp((-1.0 / adjustedYSensitivity) * qAbs(delta_y)) + 1.0;
            #else
            qreal x = -std::exp((-1.0 / adjustedXSensitivity) * std::abs(delta_x)) + 1.0;
            qreal y = -std::exp((-1.0 / adjustedYSensitivity) * std::abs(delta_y)) + 1.0;
            #endif
            // take the sign into account, expanding the range to (-1, 1)
            x *= sign(delta_x);
            y *= -sign(delta_y);
            // XInput joystick coordinates are signed shorts, so convert to (-32767, 32767)
            leftX = (short)(32767.0 * x);
            leftY = (short)(32767.0 * y);
            // Mouse2Joystick core algorithm from "https://github.com/memethyl/Mouse2Joystick" <<<
        }

        short rightX = leftX;
        short rightY = leftY;

        if (leftJoystickUpdate) {
            int custom_radius_ls = VJOY_STICK_RADIUS_MAX;
            if (VJOY_STICK_RADIUS_MIN < reportData.custom_radius_ls && reportData.custom_radius_ls < VJOY_STICK_RADIUS_MAX) {
                custom_radius_ls = reportData.custom_radius_ls;
            }

            ViGEmClient_CalculateThumbValue(&leftX, &leftY, custom_radius_ls);

            ViGEmTarget_Report.sThumbLX = leftX;
            ViGEmTarget_Report.sThumbLY = leftY;
        }
        if (rightJoystickUpdate) {
            int custom_radius_rs = VJOY_STICK_RADIUS_MAX;
            if (VJOY_STICK_RADIUS_MIN < reportData.custom_radius_rs && reportData.custom_radius_rs < VJOY_STICK_RADIUS_MAX) {
                custom_radius_rs = reportData.custom_radius_rs;
            }

            ViGEmClient_CalculateThumbValue(&rightX, &rightY, custom_radius_rs);

            ViGEmTarget_Report.sThumbRX = rightX;
            ViGEmTarget_Report.sThumbRY = rightY;
        }

        VIGEM_ERROR error;
        {
            QMutexLocker locker(&s_ViGEmClient_Mutex);
            if (DualShock4Wired == vigem_target_get_type(ViGEmTarget)) {
                DS4_REPORT ds4_report;
                DS4_REPORT_INIT(&ds4_report);
                XUSB_TO_DS4_REPORT(&ViGEmTarget_Report, &ds4_report);
                error = vigem_target_ds4_update(s_ViGEmClient, ViGEmTarget, ds4_report);
            }
            else {
                error = vigem_target_x360_update(s_ViGEmClient, ViGEmTarget, ViGEmTarget_Report);
            }
        }
        Q_UNUSED(error);
        if (false == s_Mouse2vJoy_Hold) {
            int recenter_timeout = QKeyMapper::getvJoyRecenterTimeout();
            if (recenter_timeout > 0) {
                m_Mouse2vJoyResetTimerMap.value(mouse_index)->start(recenter_timeout);
            }
        }
#ifdef DEBUG_LOGOUT_ON
        if (error != VIGEM_ERROR_NONE) {
            qDebug("[ViGEmClient_Mouse2JoystickUpdate](%d) Mouse2Joystick Update ErrorCode: 0x%08X", gamepad_index, error);
        }
        else {
#ifdef JOYSTICK_VERBOSE_LOG
            qDebug("[ViGEmClient_Mouse2JoystickUpdate](%d) Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", gamepad_index, ViGEmTarget_Report.sThumbLX, ViGEmTarget_Report.sThumbLY, ViGEmTarget_Report.sThumbRX, ViGEmTarget_Report.sThumbRY);
#endif
        }
#endif
    }
}

void QKeyMapper_Worker::ViGEmClient_Joy2vJoystickUpdate(const Joy2vJoyState &joy2vjoystate, int sticktype, int gamepad_index, int player_index)
{
    if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
        return;
    }

    if (s_ViGEmClient == Q_NULLPTR) {
        return;
    }

    int gamepad_count = s_ViGEmTargetList.size();
    int gamepad_report_count = s_ViGEmTarget_ReportList.size();

    if (gamepad_index >= gamepad_count || gamepad_index >= gamepad_report_count) {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[ViGEmClient_Mouse2JoystickUpdate]" << "VirtualGamepad Index Error! ->" << gamepad_index;
#endif
        return;
    }

    PVIGEM_TARGET ViGEmTarget = s_ViGEmTargetList.at(gamepad_index);
    ViGEm_ReportData& reportData = s_ViGEmTarget_ReportList[gamepad_index];
    XUSB_REPORT& ViGEmTarget_Report = reportData.xusb_report;

    if (ViGEmTarget == Q_NULLPTR || vigem_target_is_attached(ViGEmTarget) != TRUE) {
        return;
    }

    // Convert the joystick axis values from qreal to short
    int leftX_int = 0;
    int leftY_int = 0;
    int rightX_int = 0;
    int rightY_int = 0;
    short leftX = 0;
    short leftY = 0;
    short rightX = 0;
    short rightY = 0;

    if (sticktype == JOY2VJOY_LEFTSTICK_X || sticktype == JOY2VJOY_LEFTSTICK_Y) {
        Joystick_AxisState JoyAxisState = s_JoyAxisStateMap.value(player_index);
        leftX_int = (int)(32767.0 * JoyAxisState.left_x);
        leftY_int = -(int)(32767.0 * JoyAxisState.left_y);
        if (leftX_int < -32767) {
            leftX_int = -32767;
        }
        if (leftX_int > 32767) {
            leftX_int = 32767;
        }
        if (leftY_int < -32767) {
            leftY_int = -32767;
        }
        if (leftY_int > 32767) {
            leftY_int = 32767;
        }
        leftX = (short)leftX_int;
        leftY = (short)leftY_int;
        // ViGEmClient_CalculateThumbValue(&leftX, &leftY);
    }
    else if (sticktype == JOY2VJOY_RIGHTSTICK_X || sticktype == JOY2VJOY_RIGHTSTICK_Y) {
        Joystick_AxisState JoyAxisState = s_JoyAxisStateMap.value(player_index);
        rightX_int = (int)(32767.0 * JoyAxisState.right_x);
        rightY_int = -(int)(32767.0 * JoyAxisState.right_y);
        if (rightX_int < -32767) {
            rightX_int = -32767;
        }
        if (rightX_int > 32767) {
            rightX_int = 32767;
        }
        if (rightY_int < -32767) {
            rightY_int = -32767;
        }
        if (rightY_int > 32767) {
            rightY_int = 32767;
        }
        rightX = (short)rightX_int;
        rightY = (short)rightY_int;
        // ViGEmClient_CalculateThumbValue(&rightX, &rightY);
    }

    // Update the virtual joystick's state based on the physical joystick's state
    if (sticktype == JOY2VJOY_LEFTSTICK_X) {
        if (joy2vjoystate.ls_state == JOY2VJOY_LS_2LSRS_BOTH) {
            ViGEmTarget_Report.sThumbLX = leftX;
            ViGEmTarget_Report.sThumbRX = leftX;
        }
        else if (joy2vjoystate.ls_state == JOY2VJOY_LS_2LS) {
            ViGEmTarget_Report.sThumbLX = leftX;
        }
        else if (joy2vjoystate.ls_state == JOY2VJOY_LS_2RS) {
            ViGEmTarget_Report.sThumbRX = leftX;
        }
    }
    else if (sticktype == JOY2VJOY_LEFTSTICK_Y) {
        if (joy2vjoystate.ls_state == JOY2VJOY_LS_2LSRS_BOTH) {
            ViGEmTarget_Report.sThumbLY = leftY;
            ViGEmTarget_Report.sThumbRY = leftY;
        }
        else if (joy2vjoystate.ls_state == JOY2VJOY_LS_2LS) {
            ViGEmTarget_Report.sThumbLY = leftY;
        }
        else if (joy2vjoystate.ls_state == JOY2VJOY_LS_2RS) {
            ViGEmTarget_Report.sThumbRY = leftY;
        }
    }
    else if (sticktype == JOY2VJOY_RIGHTSTICK_X) {
        if (joy2vjoystate.rs_state == JOY2VJOY_RS_2LSRS_BOTH) {
            ViGEmTarget_Report.sThumbLX = rightX;
            ViGEmTarget_Report.sThumbRX = rightX;
        }
        else if (joy2vjoystate.rs_state == JOY2VJOY_RS_2LS) {
            ViGEmTarget_Report.sThumbLX = rightX;
        }
        else if (joy2vjoystate.rs_state == JOY2VJOY_RS_2RS) {
            ViGEmTarget_Report.sThumbRX = rightX;
        }
    }
    else if (sticktype == JOY2VJOY_RIGHTSTICK_Y) {
        if (joy2vjoystate.rs_state == JOY2VJOY_RS_2LSRS_BOTH) {
            ViGEmTarget_Report.sThumbLY = rightY;
            ViGEmTarget_Report.sThumbRY = rightY;
        }
        else if (joy2vjoystate.rs_state == JOY2VJOY_RS_2LS) {
            ViGEmTarget_Report.sThumbLY = rightY;
        }
        else if (joy2vjoystate.rs_state == JOY2VJOY_RS_2RS) {
            ViGEmTarget_Report.sThumbRY = rightY;
        }
    }

    VIGEM_ERROR error;
    {
    QMutexLocker locker(&s_ViGEmClient_Mutex);
    if (DualShock4Wired == vigem_target_get_type(ViGEmTarget)) {
        DS4_REPORT ds4_report;
        DS4_REPORT_INIT(&ds4_report);
        XUSB_TO_DS4_REPORT(&ViGEmTarget_Report, &ds4_report);
        error = vigem_target_ds4_update(s_ViGEmClient, ViGEmTarget, ds4_report);
    }
    else {
        error = vigem_target_x360_update(s_ViGEmClient, ViGEmTarget, ViGEmTarget_Report);
    }
    }
    Q_UNUSED(error);
#ifdef DEBUG_LOGOUT_ON
    if (error != VIGEM_ERROR_NONE) {
        qDebug("[ViGEmClient_Joy2vJoystickUpdate](%d) Joy2vJoystick Update ErrorCode: 0x%08X", gamepad_index, error);
    }
    else {
#ifdef JOYSTICK_VERBOSE_LOG
        qDebug("[ViGEmClient_Joy2vJoystickUpdate]Joy(%d), vJoy(%d), Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", player_index, gamepad_index, ViGEmTarget_Report.sThumbLX, ViGEmTarget_Report.sThumbLY, ViGEmTarget_Report.sThumbRX, ViGEmTarget_Report.sThumbRY);
#endif
    }
#endif
}

#if 0
void QKeyMapper_Worker::ViGEmClient_GamepadReset()
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
        return;
    }

    if (s_ViGEmClient == Q_NULLPTR || s_ViGEmTarget == Q_NULLPTR) {
        return;
    }

    if (vigem_target_is_attached(s_ViGEmTarget) != TRUE) {
        return;
    }

    XUSB_REPORT_INIT(&s_ViGEmTarget_Report);
    VIGEM_ERROR error;
    if (DualShock4Wired == vigem_target_get_type(s_ViGEmTarget)) {
        DS4_REPORT ds4_report;
        DS4_REPORT_INIT(&ds4_report);
        XUSB_TO_DS4_REPORT(&s_ViGEmTarget_Report, &ds4_report);
        error = vigem_target_ds4_update(s_ViGEmClient, s_ViGEmTarget, ds4_report);
    }
    else {
        error = vigem_target_x360_update(s_ViGEmClient, s_ViGEmTarget, s_ViGEmTarget_Report);
    }
    Q_UNUSED(error);
#ifdef DEBUG_LOGOUT_ON
    if (error != VIGEM_ERROR_NONE) {
        qDebug("[ViGEmClient_GamepadReset] VirtualGamepad Reset Update ErrorCode: 0x%08X", error);
    }
    else {
#ifdef JOYSTICK_VERBOSE_LOG
        qDebug("[ViGEmClient_GamepadReset] Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", s_ViGEmTarget_Report.sThumbLX, s_ViGEmTarget_Report.sThumbLY, s_ViGEmTarget_Report.sThumbRX, s_ViGEmTarget_Report.sThumbRY);
#endif
    }
#endif
}
#endif

void QKeyMapper_Worker::ViGEmClient_AllGamepadReset()
{
    int gamepad_count = s_ViGEmTargetList.size();

    for (int gamepad_index = 0; gamepad_index < gamepad_count; gamepad_index++) {
        ViGEmClient_GamepadReset_byIndex(gamepad_index);
    }
}

void QKeyMapper_Worker::ViGEmClient_GamepadReset_byIndex(int gamepad_index)
{
    if (s_ViGEmTargetList.size() <= gamepad_index
        || s_ViGEmTarget_ReportList.size() <= gamepad_index
        || s_ViGEmTargetList.size() != s_ViGEmTarget_ReportList.size()) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("[ViGEmClient_GamepadReset_byIndex] Size error！ VirtualGamepadIndex = %d, s_ViGEmTargetList.size = %d, s_ViGEmTarget_ReportList.size = %d", gamepad_index, s_ViGEmTargetList.size(), s_ViGEmTarget_ReportList.size());
#endif
        return;
    }

    PVIGEM_TARGET ViGEmTarget = s_ViGEmTargetList.at(gamepad_index);

    if (ViGEmTarget == Q_NULLPTR) {
        return;
    }

    if (vigem_target_is_attached(ViGEmTarget) != TRUE) {
        return;
    }

    ViGEm_ReportData& reportData = s_ViGEmTarget_ReportList[gamepad_index];
    XUSB_REPORT& ViGEmTarget_Report = reportData.xusb_report;
    XUSB_REPORT_INIT(&ViGEmTarget_Report);
    reportData.custom_radius_ls = VJOY_STICK_RADIUS_MAX;
    reportData.custom_radius_rs = VJOY_STICK_RADIUS_MAX;
    VIGEM_ERROR error;
    if (DualShock4Wired == vigem_target_get_type(ViGEmTarget)) {
        DS4_REPORT ds4_report;
        DS4_REPORT_INIT(&ds4_report);
        error = vigem_target_ds4_update(s_ViGEmClient, ViGEmTarget, ds4_report);
    }
    else {
        ViGEmTarget_Report.sThumbLY = 1;
        error = vigem_target_x360_update(s_ViGEmClient, ViGEmTarget, ViGEmTarget_Report);
        Q_UNUSED(error);
        ViGEmTarget_Report.sThumbLY = XINPUT_THUMB_RELEASE;
        error = vigem_target_x360_update(s_ViGEmClient, ViGEmTarget, ViGEmTarget_Report);
    }
    Q_UNUSED(error);
#ifdef DEBUG_LOGOUT_ON
    if (error != VIGEM_ERROR_NONE) {
        qWarning("[ViGEmClient_GamepadReset_byIndex] Reset VirtualGamepad(%d) Report State Failed!!!, Error=0x%08X -> ViGEmTarget[0x%08X]", gamepad_index, error, ViGEmTarget);
    }
    else {
        qDebug("[ViGEmClient_GamepadReset_byIndex] Reset VirtualGamepad(%d) -> ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", gamepad_index, ViGEmTarget_Report.sThumbLX, ViGEmTarget_Report.sThumbLY, ViGEmTarget_Report.sThumbRX, ViGEmTarget_Report.sThumbRY);
    }
#endif
}

void QKeyMapper_Worker::ViGEmClient_JoysticksReset(int mouse_index, int gamepad_index)
{
    // if (MOUSE2VJOY_NONE == s_Mouse2vJoy_EnableState) {
    if (!s_Mouse2vJoy_EnableStateMap.contains(mouse_index)) {
        return;
    }

    if (s_ViGEmTargetList.size() <= gamepad_index
        || s_ViGEmTarget_ReportList.size() <= gamepad_index
        || s_ViGEmTargetList.size() != s_ViGEmTarget_ReportList.size()) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("[ViGEmClient_JoysticksReset] Size error！ VirtualGamepadIndex = %d, s_ViGEmTargetList.size = %d, s_ViGEmTarget_ReportList.size = %d", gamepad_index, s_ViGEmTargetList.size(), s_ViGEmTarget_ReportList.size());
#endif
        return;
    }

    PVIGEM_TARGET ViGEmTarget = s_ViGEmTargetList.at(gamepad_index);

    if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
        return;
    }

    if (s_ViGEmClient == Q_NULLPTR) {
        return;
    }

    if (ViGEmTarget == Q_NULLPTR) {
        return;
    }

    if (vigem_target_is_attached(ViGEmTarget) != TRUE) {
        return;
    }

    ViGEm_ReportData& reportData = s_ViGEmTarget_ReportList[gamepad_index];
    XUSB_REPORT& ViGEmTarget_Report = reportData.xusb_report;
    Mouse2vJoyStates Mouse2vJoy_EnableState = s_Mouse2vJoy_EnableStateMap.value(mouse_index).states;
    if (MOUSE2VJOY_LEFT == Mouse2vJoy_EnableState) {
        ViGEmTarget_Report.sThumbLX = 0;
        ViGEmTarget_Report.sThumbLY = 0;
    }
    else if (MOUSE2VJOY_RIGHT == Mouse2vJoy_EnableState) {
        ViGEmTarget_Report.sThumbRX = 0;
        ViGEmTarget_Report.sThumbRY = 0;
    }
    else {
        ViGEmTarget_Report.sThumbLX = 0;
        ViGEmTarget_Report.sThumbLY = 0;
        ViGEmTarget_Report.sThumbRX = 0;
        ViGEmTarget_Report.sThumbRY = 0;
    }

    ViGEmClient_CheckJoysticksReportData(gamepad_index);

    VIGEM_ERROR error;
    {
    QMutexLocker locker(&s_ViGEmClient_Mutex);
    if (DualShock4Wired == vigem_target_get_type(ViGEmTarget)) {
        DS4_REPORT ds4_report;
        DS4_REPORT_INIT(&ds4_report);
        XUSB_TO_DS4_REPORT(&ViGEmTarget_Report, &ds4_report);
        error = vigem_target_ds4_update(s_ViGEmClient, ViGEmTarget, ds4_report);
    }
    else {
        error = vigem_target_x360_update(s_ViGEmClient, ViGEmTarget, ViGEmTarget_Report);
    }
    }
    Q_UNUSED(error);
#ifdef DEBUG_LOGOUT_ON
    if (error != VIGEM_ERROR_NONE) {
        qDebug("[ViGEmClient_GamepadReset] GamepadReset Update ErrorCode: 0x%08X", error);
    }
    else {
#ifdef JOYSTICK_VERBOSE_LOG
        qDebug("[ViGEmClient_JoysticksReset](%d) Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", gamepad_index, ViGEmTarget_Report.sThumbLX, ViGEmTarget_Report.sThumbLY, ViGEmTarget_Report.sThumbRX, ViGEmTarget_Report.sThumbRY);
#endif
    }
#endif
}
#endif

#if 0
void QKeyMapper_Worker::timerEvent(QTimerEvent *event)
{
    QMutexLocker locker(&m_BurstTimerMutex);

    int timerID = event->timerId();
    auto it_burst_timer_keyup = std::find(m_BurstKeyUpTimerMap.cbegin(), m_BurstKeyUpTimerMap.cend(), timerID);
    auto it_burst_timer = std::find(m_BurstTimerMap.cbegin(), m_BurstTimerMap.cend(), timerID);
    if (it_burst_timer_keyup != m_BurstKeyUpTimerMap.cend()) {
        QString burstKey = m_BurstKeyUpTimerMap.key(timerID);
        if (false == burstKey.isEmpty()) {
            killTimer(timerID);
            m_BurstKeyUpTimerMap.remove(burstKey);
            // sendBurstKeyUp(burstKey, false);
            int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(burstKey);
            if (findindex == -1) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[QKeyMapper_Worker::timerEvent]" << "sendBurstKeyUp [" << burstKey << "] could not found in KeyMappingDataList!";
#endif
                return;
            }
            sendBurstKeyUp(findindex, false);
//#ifdef DEBUG_LOGOUT_ON
//            qDebug("timerEvent(): Key \"%s\" BurstPress timeout.", burstKey.toStdString().c_str());
//#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[QKeyMapper_Worker::timerEvent] Could not find TimerID(%d) in m_BurstKeyUpTimerMap!!!", timerID);
#endif
        }
    }
    else if (it_burst_timer != m_BurstTimerMap.cend()) {
        QString burstKey = m_BurstTimerMap.key(timerID);
        if (false == burstKey.isEmpty()) {
//#ifdef DEBUG_LOGOUT_ON
//            qDebug("timerEvent(): Key \"%s\" BurstTimer timeout.", burstKey.toStdString().c_str());
//#endif
            // sendBurstKeyDown(burstKey);
            int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(burstKey);
            if (findindex == -1) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[QKeyMapper_Worker::timerEvent]" << "sendBurstKeyDown [" << burstKey << "] could not found in KeyMappingDataList!" ;
#endif
                return;
            }
            sendBurstKeyDown(findindex);

            if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
                int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
                qDebug("[QKeyMapper_Worker::timerEvent] Key \"%s\" BurstKeyUpTimer(%d) already started, kill it first!", burstKey.toStdString().c_str(), existTimerID);
#endif
                killTimer(existTimerID);
                m_BurstKeyUpTimerMap.remove(burstKey);
            }
            // int burstpressTime = QKeyMapper::getBurstPressTime();
            int burstpressTime = QKeyMapper::KeyMappingDataList->at(findindex).BurstPressTime;
            int keyupTimerID = startTimer(burstpressTime, Qt::PreciseTimer);
            m_BurstKeyUpTimerMap.insert(burstKey, keyupTimerID);
#ifdef DEBUG_LOGOUT_ON
            qDebug("[QKeyMapper_Worker::timerEvent] Key \"%s\" Send BurstKeyUpTimer(%d), ID(%d), MappingDataIndex(%d)", burstKey.toStdString().c_str(), burstpressTime, keyupTimerID, findindex);
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[QKeyMapper_Worker::timerEvent] Could not find TimerID(%d) in BurstTimerMap!!!", timerID);
#endif
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning("[QKeyMapper_Worker::timerEvent] Could not find TimerID(%d) in both m_BurstKeyUpTimerMap & BurstTimerMap!!!", timerID);
#endif
    }
}
#endif

void QKeyMapper_Worker::threadStarted()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("threadStarted() -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

    /* UDP Data Port Listener */
    m_UdpSocket = new QUdpSocket(this);
    QObject::connect(m_UdpSocket, &QUdpSocket::readyRead, this, &QKeyMapper_Worker::processUdpPendingDatagrams);
}

void QKeyMapper_Worker::setWorkerKeyHook()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("\033[1;34m[QKeyMapper_Worker::setWorkerKeyHook] WorkerThread Hookproc Start.\033[0m");
#endif

    s_AtomicHookProcState = HOOKPROC_STATE_STARTING;

    // Handle FilterKeys state on mapping start
    handleFilterKeysOnMappingStart();

    // Q_UNUSED(hWnd);
    clearAllBurstKeyTimersAndLockKeys();
    breakAllRunningKeySequence();
    clearAllPressedVirtualKeys();
    clearAllPressedRealCombinationKeys();
    s_KeySequenceRepeatCount.clear();
    // pressedRealKeysList.clear();
    pressedVirtualKeysList.clear();
    // pressedCombinationRealKeysList.clear();
    pressedLongPressKeysList.clear();
    pressedDoublePressKeysList.clear();
    s_runningKeySequenceOrikeyList.clear();
    // pressedShortcutKeysList.clear();

    // clearAllLongPressTimers();
    // clearAllDoublePressTimers();
    combinationOriginalKeysList.clear();
    longPressOriginalKeysMap.clear();
    collectLongPressOriginalKeysMap();
    doublePressOriginalKeysMap.clear();
    collectDoublePressOriginalKeysMap();
    collectCombinationOriginalKeysList();
    collectBlockedKeysList();
#ifdef DEBUG_LOGOUT_ON
    if (combinationOriginalKeysList.isEmpty() == false) {
        qDebug() << "[setWorkerKeyHook]" << "combinationOriginalKeysList ->" << combinationOriginalKeysList;
    }
#endif

    {
    QMutexLocker locker(&s_PressedMappingKeysMapMutex);
    pressedMappingKeysMap.clear();
    }
    SendInputTask::clearSendInputTaskControllerMap();
    resetGlobalSendInputTaskController();
    pressedLockKeysMap.clear();
    collectExchangeKeysList();
    SendInputTask::initSendInputTaskControllerMap();

    releasePressedRealKeysOfOriginalKeys();

#ifdef VIGEM_CLIENT_SUPPORT
    s_Auto_Brake = AUTO_BRAKE_DEFAULT;
    s_Auto_Accel = AUTO_ACCEL_DEFAULT;
    s_last_Auto_Brake = 0;
    s_last_Auto_Accel = 0;
    s_GripDetect_EnableState = checkGripDetectEnableState();
    // s_Joy2vJoyState = checkJoy2vJoyState();
    s_Joy2vJoy_EnableStateMap = checkJoy2vJoyEnableStateMap();
    s_Mouse2vJoy_delta = QPoint();
    s_Mouse2vJoy_prev = QPoint();
    // s_Mouse2vJoy_delta_interception = QPoint();
    // {
    //     QMutexLocker locker(&s_MouseMove_delta_List_Mutex);
    //     s_Mouse2vJoy_delta_List.clear();
    //     for (int i = 0; i < INTERCEPTION_MAX_MOUSE; ++i) {
    //         s_Mouse2vJoy_delta_List.append(QPoint());
    //     }
    // }
    pressedvJoyLStickKeysList.clear();
    pressedvJoyRStickKeysList.clear();
    pressedvJoyButtonsList.clear();
    for (int i = 0; i < VIRTUAL_GAMEPAD_NUMBER_MAX; ++i) {
        pressedvJoyLStickKeysList.append(OrderedMap<QString, BYTE>());
        pressedvJoyRStickKeysList.append(OrderedMap<QString, BYTE>());
        pressedvJoyButtonsList.append(QStringList());
    }
    stopMouse2vJoyResetTimerMap();
    // ViGEmClient_GamepadReset();
    ViGEmClient_AllGamepadReset();
    // s_Mouse2vJoy_EnableState = ViGEmClient_checkMouse2JoystickEnableState();
    s_Mouse2vJoy_EnableStateMap = ViGEmClient_checkMouse2JoystickEnableStateMap();
#endif

    s_Key2Mouse_EnableState = checkKey2MouseEnableState();
    // s_Joy2Mouse_EnableState = checkJoystick2MouseEnableState();
    s_Joy2Mouse_EnableStateMap = checkJoy2MouseEnableStateMap();
    s_GameControllerSensor_EnableState = checkGyro2MouseEnableState();
    s_Gyro2Mouse_MoveActive = checkGyro2MouseMoveActiveState();

#ifdef VIGEM_CLIENT_SUPPORT
    // if (s_Mouse2vJoy_EnableState != MOUSE2VJOY_NONE && QKeyMapper::getvJoyLockCursorStatus()) {
    if ((!s_Mouse2vJoy_EnableStateMap.isEmpty()) && QKeyMapper::getvJoyLockCursorStatus()) {
        POINT pt;
        if (GetCursorPos(&pt)) {
            m_LastMouseCursorPoint = pt;
#ifdef DEBUG_LOGOUT_ON
            qDebug("[setWorkerKeyHook] Last Mouse Cursor Positoin -> X = %lu, Y = %lu", pt.x, pt.y);
#endif
        }

        POINT bottomrightPoint = mousePositionAfterSetMouseToScreenBottomRight();
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setWorkerKeyHook] mousePositionAfterSetMouseToScreenBottomRight -> X = %lu, Y = %lu", bottomrightPoint.x, bottomrightPoint.y);
#endif

        setMouseToScreenBottomRight();

        for (int loop = 0; loop < SETMOUSEPOSITION_WAITTIME_MAX; ++loop) {
            POINT pt;
            if (GetCursorPos(&pt)) {
                if (pt.x == bottomrightPoint.x && pt.y == bottomrightPoint.y) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug("[setWorkerKeyHook] Wait setMouseToScreenBottomRight OK -> loop = %d", loop);
#endif
                    break;
                }
            }
            QThread::msleep(1);
        }


        if (GetCursorPos(&pt)) {
            s_Mouse2vJoy_prev.rx() = pt.x;
            s_Mouse2vJoy_prev.ry() = pt.y;
#ifdef DEBUG_LOGOUT_ON
            qDebug("[setWorkerKeyHook] Current BottomRight Mouse Cursor Positoin -> X = %lu, Y = %lu", pt.x, pt.y);
#endif
        }
    }
#endif

    // m_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper_Worker::LowLevelKeyboardHookProc, GetModuleHandle(Q_NULLPTR), 0);
    // m_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, QKeyMapper_Worker::LowLevelMouseHookProc, GetModuleHandle(Q_NULLPTR), 0);
    setWorkerJoystickCaptureStart();

    // s_JoyAxisState = Joystick_AxisState();
    s_JoyAxisStateMap.clear();
    if (false == s_Joy2Mouse_EnableStateMap.isEmpty() || s_Key2Mouse_EnableState) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setWorkerKeyHook] Key2MouseCycleTimer Started.");
#endif
        m_Key2MouseCycleTimer.start(KEY2MOUSE_CYCLECHECK_TIMEOUT);
    }

    startDataPortListener();
//    setWorkerDInputKeyHook(hWnd);

    initGamepadMotion();
    if (s_GameControllerSensor_EnableState) {
        emit QJoysticks::getInstance()->setGameControllersSensorEnabled_signal(true);
    }

    s_AtomicHookProcState = HOOKPROC_STATE_STARTED;
#ifdef DEBUG_LOGOUT_ON
    qDebug("[QKeyMapper_Worker::setWorkerKeyHook] WorkerThread Hookproc End.");
#endif
}

void QKeyMapper_Worker::setWorkerKeyUnHook()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("\033[1;34m[QKeyMapper_Worker::setWorkerKeyUnHook] WorkerThread Unhookproc Start.\033[0m");
#endif

    s_AtomicHookProcState = HOOKPROC_STATE_STOPPING;

    // Handle FilterKeys state on mapping stop
    handleFilterKeysOnMappingStop();

    clearAllBurstKeyTimersAndLockKeys();
    breakAllRunningKeySequence();
    clearAllNormalPressedMappingKeys();
    // clearAllPressedVirtualKeys();
    clearAllPressedRealCombinationKeys();
    s_KeySequenceRepeatCount.clear();
    // pressedRealKeysList.clear();
    // pressedVirtualKeysList.clear();
    // pressedCombinationRealKeysList.clear();
    pressedLongPressKeysList.clear();
    pressedDoublePressKeysList.clear();
    // pressedShortcutKeysList.clear();
    // clearAllLongPressTimers();
    // clearAllDoublePressTimers();
    combinationOriginalKeysList.clear();
    longPressOriginalKeysMap.clear();
    doublePressOriginalKeysMap.clear();
    // {
    // QMutexLocker locker(&s_PressedMappingKeysMapMutex);
    // pressedMappingKeysMap.clear();
    // }
    pressedLockKeysMap.clear();
    exchangeKeysList.clear();
    // SendInputTask::clearSendInputTaskControllerMap();
    // resetGlobalSendInputTaskController();

    s_runningKeySequenceOrikeyList.clear();
    clearGlobalSendInputTaskControllerThreadPool();
    SendInputTask::clearSendInputTaskControllerThreadPool();

//    if (m_MouseHook != Q_NULLPTR) {
//        UnhookWindowsHookEx(m_MouseHook);
//        m_MouseHook = Q_NULLPTR;
//    }

//    if (m_KeyHook != Q_NULLPTR){
//        UnhookWindowsHookEx(m_KeyHook);
//        m_KeyHook = Q_NULLPTR;
//#ifdef DEBUG_LOGOUT_ON
//        qInfo("[setKeyUnHook] Normal Key Hook & Mouse Hook Released.");
//#endif
//    }

    s_Key2Mouse_EnableState = false;
    // s_Joy2Mouse_EnableState = JOY2MOUSE_NONE;
    s_Joy2Mouse_EnableStateMap.clear();
    s_GameControllerSensor_EnableState = false;
    setWorkerJoystickCaptureStop();

    if (m_Key2MouseCycleTimer.isActive()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setWorkerKeyUnHook] Key2MouseCycleTimer Stopped.");
#endif
        m_Key2MouseCycleTimer.stop();
    }
    // s_JoyAxisState = Joystick_AxisState();
    s_JoyAxisStateMap.clear();

    stopDataPortListener();
    //    setWorkerDInputKeyUnHook();
    initGamepadMotion();
    emit QJoysticks::getInstance()->setGameControllersSensorEnabled_signal(false);

#ifdef VIGEM_CLIENT_SUPPORT
    // if (s_Mouse2vJoy_EnableState != MOUSE2VJOY_NONE && isCursorAtBottomRight() && m_LastMouseCursorPoint.x >= 0) {
    if ((!s_Mouse2vJoy_EnableStateMap.isEmpty()) && isCursorAtBottomRight() && m_LastMouseCursorPoint.x >= 0) {
        setMouseToPoint(m_LastMouseCursorPoint);

#ifdef DEBUG_LOGOUT_ON
        POINT pt;
        if (GetCursorPos(&pt)) {
            qDebug("[setWorkerKeyUnHook] Set Mouse Cursor Back to Last Positoin -> X = %lu, Y = %lu", pt.x, pt.y);
        }
#endif
    }

    s_Auto_Brake = AUTO_BRAKE_DEFAULT;
    s_Auto_Accel = AUTO_ACCEL_DEFAULT;
    s_last_Auto_Brake = 0;
    s_last_Auto_Accel = 0;
    s_GripDetect_EnableState = GRIPDETECT_NONE;
    // s_Joy2vJoyState = Joy2vJoyState();
    s_Joy2vJoy_EnableStateMap.clear();
    s_Mouse2vJoy_delta = QPoint();
    s_Mouse2vJoy_prev = QPoint();
    // s_Mouse2vJoy_delta_interception = QPoint();
    // {
    //     QMutexLocker locker(&s_MouseMove_delta_List_Mutex);
    //     s_Mouse2vJoy_delta_List.clear();
    //     for (int i = 0; i < INTERCEPTION_MAX_MOUSE; ++i) {
    //         s_Mouse2vJoy_delta_List.append(QPoint());
    //     }
    // }
    pressedvJoyLStickKeysList.clear();
    pressedvJoyRStickKeysList.clear();
    pressedvJoyButtonsList.clear();
    for (int i = 0; i < VIRTUAL_GAMEPAD_NUMBER_MAX; ++i) {
        pressedvJoyLStickKeysList.append(OrderedMap<QString, BYTE>());
        pressedvJoyRStickKeysList.append(OrderedMap<QString, BYTE>());
        pressedvJoyButtonsList.append(QStringList());
    }
    stopMouse2vJoyResetTimerMap();
    // ViGEmClient_GamepadReset();
    if (!s_isWorkerDestructing) {
        ViGEmClient_AllGamepadReset();
    }
    // s_Mouse2vJoy_EnableState = MOUSE2VJOY_NONE;
    s_Mouse2vJoy_EnableStateMap.clear();
    m_LastMouseCursorPoint.x = -1;
    m_LastMouseCursorPoint.y = -1;
#endif

    bool allmappingkeysreleased = false;
    {
    QMutexLocker locker(&s_PressedMappingKeysMapMutex);
    allmappingkeysreleased = pressedMappingKeysMap.isEmpty();
    }

    if (allmappingkeysreleased) {
        clearAllPressedVirtualKeys();
        pressedVirtualKeysList.clear();

        SendInputTask::clearSendInputTaskControllerMap();
        resetGlobalSendInputTaskController();
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "\033[1;34m[QKeyMapper_Worker::setWorkerKeyUnHook] pressedMappingKeysMap is not empty, wait SendInputTask to clear all controllers & virtualkeys! pressedMappingKeysMap -> " << pressedMappingKeysMap << "\033[0m";
#endif
    }

    if (!s_isWorkerDestructing) {
        clearCustomKeyFlags();
    }

    s_AtomicHookProcState = HOOKPROC_STATE_STOPPED;

#ifdef DEBUG_LOGOUT_ON
    qDebug("\033[1;34m[QKeyMapper_Worker::setWorkerKeyUnHook] WorkerThread Unhookproc End.\033[0m");
#endif
}

void QKeyMapper_Worker::handleFilterKeysOnMappingStart()
{
    // Reset restore flag at the start of mapping
    s_RestoreFilterKeysState = false;

    // Check if we need to enable FilterKeys during mapping
    if (QKeyMapper::getEnableSystemFilterKeyChecked()) {
        // If FilterKeys is not enabled, enable it and set restore flag
        if (!QKeyMapper::isWindowsFilterKeysEnabled()) {
            QKeyMapper::setWindowsFilterKeysEnabled(true);
            s_RestoreFilterKeysState = true;
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper_Worker::handleFilterKeysOnMappingStart] Enabled FilterKeys for mapping, will restore on stop";
#endif
        } else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper_Worker::handleFilterKeysOnMappingStart] FilterKeys already enabled, no change needed";
#endif
        }
    } else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QKeyMapper_Worker::handleFilterKeysOnMappingStart] EnableSystemFilterKey is unchecked, no change needed";
#endif
    }
}

void QKeyMapper_Worker::handleFilterKeysOnMappingStop()
{
    // Only restore if the flag is set (we enabled FilterKeys and user didn't change it)
    if (s_RestoreFilterKeysState && QKeyMapper::isWindowsFilterKeysEnabled()) {
        // Disable FilterKeys (restore to original disabled state)
        QKeyMapper::setWindowsFilterKeysEnabled(false);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QKeyMapper_Worker::handleFilterKeysOnMappingStop] Restored FilterKeys to disabled state";
#endif
    } else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QKeyMapper_Worker::handleFilterKeysOnMappingStop] No FilterKeys state restore needed";
#endif
    }

    // Reset restore flag
    s_RestoreFilterKeysState = false;
}

void QKeyMapper_Worker::notifyUserChangedFilterKeys()
{
    // Clear the restore flag since user manually changed FilterKeys
    s_RestoreFilterKeysState = false;
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QKeyMapper_Worker::notifyUserChangedFilterKeys] User changed FilterKeys, clearing restore flag";
#endif
}

void QKeyMapper_Worker::setKeyMappingRestart()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("\033[1;34m[QKeyMapper_Worker::setKeyMappingRestart] KeyMapping Restart Start.>>>\033[0m");
#endif

    /* Restart Stopping process */
    QList<MAP_KEYDATA> *backup_KeyMappingDataList = QKeyMapper::KeyMappingDataList;
    if (QKeyMapper::lastKeyMappingDataList != Q_NULLPTR) {
        QKeyMapper::KeyMappingDataList = QKeyMapper::lastKeyMappingDataList;
    }

    s_AtomicHookProcState = HOOKPROC_STATE_RESTART_STOPPING;

    /* Stop Key Mapping Process */
    clearAllBurstKeyTimersAndLockKeys();
    breakAllRunningKeySequence();
    clearAllNormalPressedMappingKeys(true, backup_KeyMappingDataList);
    // clearAllPressedVirtualKeys();
    clearAllPressedRealCombinationKeys();
    s_KeySequenceRepeatCount.clear();
    // pressedCombinationRealKeysList.clear();
    pressedLongPressKeysList.clear();
    pressedDoublePressKeysList.clear();
    combinationOriginalKeysList.clear();
    longPressOriginalKeysMap.clear();
    doublePressOriginalKeysMap.clear();
    // {
    // QMutexLocker locker(&s_PressedMappingKeysMapMutex);
    // pressedMappingKeysMap.clear();
    // }
    pressedLockKeysMap.clear();
    exchangeKeysList.clear();
    // SendInputTask::clearSendInputTaskControllerMap();
    // resetGlobalSendInputTaskController();

    s_runningKeySequenceOrikeyList.clear();
    clearGlobalSendInputTaskControllerThreadPool();
    SendInputTask::clearSendInputTaskControllerThreadPool();

    s_Key2Mouse_EnableState = false;
    s_Joy2Mouse_EnableStateMap.clear();
    s_GameControllerSensor_EnableState = false;

    if (m_Key2MouseCycleTimer.isActive()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setKeyMappingRestart] Key2MouseCycleTimer Stopped.");
#endif
        m_Key2MouseCycleTimer.stop();
    }
    s_JoyAxisStateMap.clear();

    stopDataPortListener();
    emit QJoysticks::getInstance()->setGameControllersSensorEnabled_signal(false);

    if ((!s_Mouse2vJoy_EnableStateMap.isEmpty()) && isCursorAtBottomRight() && m_LastMouseCursorPoint.x >= 0) {
        setMouseToPoint(m_LastMouseCursorPoint);
    }

    s_Auto_Brake = AUTO_BRAKE_DEFAULT;
    s_Auto_Accel = AUTO_ACCEL_DEFAULT;
    s_last_Auto_Brake = 0;
    s_last_Auto_Accel = 0;
    s_GripDetect_EnableState = GRIPDETECT_NONE;
    s_Joy2vJoy_EnableStateMap.clear();
    s_Mouse2vJoy_delta = QPoint();
    s_Mouse2vJoy_prev = QPoint();
    pressedvJoyLStickKeysList.clear();
    pressedvJoyRStickKeysList.clear();
    pressedvJoyButtonsList.clear();
    for (int i = 0; i < VIRTUAL_GAMEPAD_NUMBER_MAX; ++i) {
        pressedvJoyLStickKeysList.append(OrderedMap<QString, BYTE>());
        pressedvJoyRStickKeysList.append(OrderedMap<QString, BYTE>());
        pressedvJoyButtonsList.append(QStringList());
    }
    stopMouse2vJoyResetTimerMap();
    ViGEmClient_AllGamepadReset();
    s_Mouse2vJoy_EnableStateMap.clear();
    m_LastMouseCursorPoint.x = -1;
    m_LastMouseCursorPoint.y = -1;

    bool allmappingkeysreleased = false;
    {
    QMutexLocker locker(&s_PressedMappingKeysMapMutex);
    allmappingkeysreleased = pressedMappingKeysMap.isEmpty();
    }
    if (!allmappingkeysreleased) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "\033[1;34m[QKeyMapper_Worker::setKeyMappingRestart] pressedMappingKeysMap is not empty, try to clear all controllers & virtualkeys! pressedMappingKeysMap -> " << pressedMappingKeysMap << "\033[0m";
#endif
    }
    clearAllPressedVirtualKeys();
    pressedVirtualKeysList.clear();
    SendInputTask::clearSendInputTaskControllerMap();
    resetGlobalSendInputTaskController();
    clearCustomKeyFlags(true);


    /* Restart Starting process */
    QKeyMapper::KeyMappingDataList = backup_KeyMappingDataList;

    s_AtomicHookProcState = HOOKPROC_STATE_RESTART_STARTING;

    /* Start Key Mapping Process */
    {
    QMutexLocker locker(&s_PressedMappingKeysMapMutex);
    pressedMappingKeysMap.clear();
    }
    collectLongPressOriginalKeysMap();
    collectDoublePressOriginalKeysMap();
    collectCombinationOriginalKeysList();
    collectBlockedKeysList();
#ifdef DEBUG_LOGOUT_ON
    if (combinationOriginalKeysList.isEmpty() == false) {
        qDebug() << "[setKeyMappingRestart]" << "combinationOriginalKeysList ->" << combinationOriginalKeysList;
    }
#endif
    collectExchangeKeysList();
    SendInputTask::initSendInputTaskControllerMap();

    releasePressedRealKeysOfOriginalKeys();

    s_GripDetect_EnableState = checkGripDetectEnableState();
    s_Joy2vJoy_EnableStateMap = checkJoy2vJoyEnableStateMap();
    s_Mouse2vJoy_EnableStateMap = ViGEmClient_checkMouse2JoystickEnableStateMap();
    s_Key2Mouse_EnableState = checkKey2MouseEnableState();
    s_Joy2Mouse_EnableStateMap = checkJoy2MouseEnableStateMap();
    s_GameControllerSensor_EnableState = checkGyro2MouseEnableState();
    s_Gyro2Mouse_MoveActive = checkGyro2MouseMoveActiveState();

    if ((!s_Mouse2vJoy_EnableStateMap.isEmpty()) && QKeyMapper::getvJoyLockCursorStatus()) {
        POINT pt;
        if (GetCursorPos(&pt)) {
            m_LastMouseCursorPoint = pt;
#ifdef DEBUG_LOGOUT_ON
            qDebug("[setKeyMappingRestart] Last Mouse Cursor Positoin -> X = %lu, Y = %lu", pt.x, pt.y);
#endif
        }

        POINT bottomrightPoint = mousePositionAfterSetMouseToScreenBottomRight();
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setKeyMappingRestart] mousePositionAfterSetMouseToScreenBottomRight -> X = %lu, Y = %lu", bottomrightPoint.x, bottomrightPoint.y);
#endif

        setMouseToScreenBottomRight();

        for (int loop = 0; loop < SETMOUSEPOSITION_WAITTIME_MAX; ++loop) {
            POINT pt;
            if (GetCursorPos(&pt)) {
                if (pt.x == bottomrightPoint.x && pt.y == bottomrightPoint.y) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug("[setKeyMappingRestart] Wait setMouseToScreenBottomRight OK -> loop = %d", loop);
#endif
                    break;
                }
            }
            QThread::msleep(1);
        }


        if (GetCursorPos(&pt)) {
            s_Mouse2vJoy_prev.rx() = pt.x;
            s_Mouse2vJoy_prev.ry() = pt.y;
#ifdef DEBUG_LOGOUT_ON
            qDebug("[setKeyMappingRestart] Current BottomRight Mouse Cursor Positoin -> X = %lu, Y = %lu", pt.x, pt.y);
#endif
        }
    }

    if (false == s_Joy2Mouse_EnableStateMap.isEmpty() || s_Key2Mouse_EnableState) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setKeyMappingRestart] Key2MouseCycleTimer Started.");
#endif
        m_Key2MouseCycleTimer.start(KEY2MOUSE_CYCLECHECK_TIMEOUT);
    }

    startDataPortListener();

    initGamepadMotion();
    if (s_GameControllerSensor_EnableState) {
        emit QJoysticks::getInstance()->setGameControllersSensorEnabled_signal(true);
    }

    s_AtomicHookProcState = HOOKPROC_STATE_STARTED;

#ifdef DEBUG_LOGOUT_ON
    qDebug("\033[1;34m[QKeyMapper_Worker::setKeyMappingRestart] KeyMapping Restart End.<<<\033[0m");
#endif
}

void QKeyMapper_Worker::allMappingKeysReleased()
{
    if (HOOKPROC_STATE_STOPPED == s_AtomicHookProcState) {
        bool pressedmappingkeys_empty = false;
        {
            QMutexLocker locker(&s_PressedMappingKeysMapMutex);
            pressedmappingkeys_empty = pressedMappingKeysMap.isEmpty();
        }
        if (pressedmappingkeys_empty) {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("[allMappingKeysReleased] pressedMappingKeysMap is empty on s_AtomicHookProcState = HOOKPROC_STATE_STOPPED, do virtualkey release process.");
            qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            /* Remove clearAllPressedVirtualKeys when Burst keys stop
             * on setWorkerKeyUnHook will call resendRealKeyCodeOnStop() */
            // clearAllPressedVirtualKeys();

            pressedVirtualKeysList.clear();

            SendInputTask::clearSendInputTaskControllerMap();
            resetGlobalSendInputTaskController();
        }
    }
}

void QKeyMapper_Worker::sessionLockStateChanged(bool locked)
{
    if (QKeyMapper::KEYMAP_MAPPING_GLOBAL == QKeyMapper::getInstance()->m_KeyMapStatus
        || QKeyMapper::KEYMAP_MAPPING_MATCHED == QKeyMapper::getInstance()->m_KeyMapStatus) {
        if (locked) {
            setWorkerKeyUnHook();
        }
        else {
            setWorkerKeyHook();
        }
    }

    pressedRealKeysList.clear();
    pressedRealKeysListRemoveMultiInput.clear();
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QKeyMapper_Worker::sessionLockStateChanged]" << "State:" << locked << ", pressedRealKeysList Cleared.";
#endif
}

void QKeyMapper_Worker::setWorkerJoystickCaptureStart(void)
{
    m_JoystickCapture = true;
}

void QKeyMapper_Worker::setWorkerJoystickCaptureStop()
{
    m_JoystickCapture = false;
}

#if 0
void QKeyMapper_Worker::HotKeyHookProc(const QString &keycodeString, int keyupdown)
{
#if 0
    if (KEY_DOWN == keyupdown){
        if (false == pressedShortcutKeysList.contains(keycodeString)){
            pressedShortcutKeysList.append(keycodeString);
        }
    }
    else {  /* KEY_UP == keyupdown */
        if (true == pressedShortcutKeysList.contains(keycodeString)){
            pressedShortcutKeysList.removeAll(keycodeString);
        }
    }
#endif

    bool returnFlag = false;
    QString keycodeStringForSearch = QString(OLD_PREFIX_SHORTCUT) + keycodeString;
    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeStringForSearch);
    returnFlag = hookBurstAndLockProc(keycodeStringForSearch, keyupdown);
    updatePressedRealKeysList(keycodeStringForSearch, keyupdown);

    if (false == returnFlag) {
        if (findindex >=0){
            QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
            QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
            if (KEY_DOWN == keyupdown){
                bool skipReleaseModifiers = false;

                if (mappingKeyList.size() == 1) {
                    QString mappingkey = mappingKeyList.constFirst();
                    if (skipReleaseModifiersKeysList.contains(mappingkey)) {
                        skipReleaseModifiers = true;
                    }
                }

                if (false == skipReleaseModifiers) {
                    const Qt::KeyboardModifiers modifiers_arg = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
                    releaseKeyboardModifiers(modifiers_arg);
                }
                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
            }
            else { /* KEY_UP == keyupdown */
                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
            }
        }
    }
}
#endif

QKeyMapper_Worker::GripDetectStates QKeyMapper_Worker::checkGripDetectEnableState()
{
    GripDetectStates gripdetect_enablestate = GRIPDETECT_NONE;
    bool gripdetect_brake = false;
    bool gripdetect_accel = false;

    int findvJoyLTBrakeIndex = QKeyMapper::findMapKeyInKeyMappingDataList(VJOY_LT_BRAKE_STR);
    int findvJoyRTBrakeIndex = QKeyMapper::findMapKeyInKeyMappingDataList(VJOY_RT_BRAKE_STR);
    if (findvJoyLTBrakeIndex >= 0 || findvJoyRTBrakeIndex >= 0){
        gripdetect_brake = true;
    }

    int findvJoyLTAccelIndex = QKeyMapper::findMapKeyInKeyMappingDataList(VJOY_LT_ACCEL_STR);
    int findvJoyRTAccelIndex = QKeyMapper::findMapKeyInKeyMappingDataList(VJOY_RT_ACCEL_STR);
    if (findvJoyLTAccelIndex >= 0 || findvJoyRTAccelIndex >= 0){
        gripdetect_accel = true;
    }

    if (gripdetect_brake) {
        gripdetect_enablestate |= GRIPDETECT_BRAKE;
    }

    if (gripdetect_accel) {
        gripdetect_enablestate |= GRIPDETECT_ACCEL;
    }

    return gripdetect_enablestate;
}

#if 0
QKeyMapper_Worker::Joy2vJoyState QKeyMapper_Worker::checkJoy2vJoyState()
{
    Joy2vJoyState joy2vjoystate = Joy2vJoyState();
    bool joy2vjoy_LT = false;
    bool joy2vjoy_RT = false;
    bool joy2vjoy_LS2LS = false;
    bool joy2vjoy_LS2RS = false;
    bool joy2vjoy_RS2LS = false;
    bool joy2vjoy_RS2RS = false;

    int findJoy2vJoyLTindex = QKeyMapper::findOriKeyInKeyMappingDataList(JOY_LT2VJOYLT_STR);
    if (findJoy2vJoyLTindex >= 0) {
        joy2vjoy_LT = true;
    }

    int findJoy2vJoyRTindex = QKeyMapper::findOriKeyInKeyMappingDataList(JOY_RT2VJOYRT_STR);
    if (findJoy2vJoyRTindex >= 0) {
        joy2vjoy_RT = true;
    }

    int findJoy2vJoy_LS2LSindex = QKeyMapper::findOriKeyInKeyMappingDataList(JOY_LS2VJOYLS_STR);
    if (findJoy2vJoy_LS2LSindex >= 0) {
        joy2vjoy_LS2LS = true;
    }

    int findJoy2vJoy_LS2RSindex = QKeyMapper::findOriKeyInKeyMappingDataList(JOY_LS2VJOYRS_STR);
    if (findJoy2vJoy_LS2RSindex >= 0) {
        joy2vjoy_LS2RS = true;
    }

    int findJoy2vJoy_RS2LSindex = QKeyMapper::findOriKeyInKeyMappingDataList(JOY_RS2VJOYLS_STR);
    if (findJoy2vJoy_RS2LSindex >= 0) {
        joy2vjoy_RS2LS = true;
    }

    int findJoy2vJoy_RS2RSindex = QKeyMapper::findOriKeyInKeyMappingDataList(JOY_RS2VJOYRS_STR);
    if (findJoy2vJoy_RS2RSindex >= 0) {
        joy2vjoy_RS2RS = true;
    }

    if (joy2vjoy_LT && joy2vjoy_RT) {
        joy2vjoystate.trigger_state = JOY2VJOY_TRIGGER_LTRT_BOTH;
    }
    else if (joy2vjoy_LT) {
        joy2vjoystate.trigger_state = JOY2VJOY_TRIGGER_LT;
    }
    else if (joy2vjoy_RT) {
        joy2vjoystate.trigger_state = JOY2VJOY_TRIGGER_RT;
    }

    if (joy2vjoy_LS2LS && joy2vjoy_LS2RS) {
        joy2vjoystate.ls_state = JOY2VJOY_LS_2LSRS_BOTH;
    }
    else if (joy2vjoy_LS2LS) {
        joy2vjoystate.ls_state = JOY2VJOY_LS_2LS;
    }
    else if (joy2vjoy_LS2RS) {
        joy2vjoystate.ls_state = JOY2VJOY_LS_2RS;
    }

    if (joy2vjoy_RS2LS && joy2vjoy_RS2RS) {
        joy2vjoystate.rs_state = JOY2VJOY_RS_2LSRS_BOTH;
    }
    else if (joy2vjoy_RS2LS) {
        joy2vjoystate.rs_state = JOY2VJOY_RS_2LS;
    }
    else if (joy2vjoy_RS2RS) {
        joy2vjoystate.rs_state = JOY2VJOY_RS_2RS;
    }

    return joy2vjoystate;
}
#endif

QHash<int, QKeyMapper_Worker::Joy2vJoyState> QKeyMapper_Worker::checkJoy2vJoyEnableStateMap()
{
    QHash<int, Joy2vJoyState> Joy2vJoy_EnableStateMap;

    static QRegularExpression joy2vjoy_orikey_regex(R"(^(Joy-(LS|RS|Key11\(LT\)|Key12\(RT\))_2vJoy(LS|RS|LT|RT))(?:@([0-9]))?$)");
    static QRegularExpression joy2vjoy_mapkey_regex(R"(^(Joy-(LS|RS|Key11\(LT\)|Key12\(RT\))_2vJoy(LS|RS|LT|RT))(?:@([0-3]))?$)");
    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList)) {
        QRegularExpressionMatch joy2vjoy_orikey_match = joy2vjoy_orikey_regex.match(keymapdata.Original_Key);

        if (joy2vjoy_orikey_match.hasMatch()) {
            QString originalkey_withoutindex = joy2vjoy_orikey_match.captured(1);
            QString controlType = joy2vjoy_orikey_match.captured(2);
            QString target = joy2vjoy_orikey_match.captured(3);
            QString playerIndexStr = joy2vjoy_orikey_match.captured(4);
            int playerIndex = playerIndexStr.isEmpty() ? INITIAL_PLAYER_INDEX : playerIndexStr.toInt();

            Joy2vJoyState &joy2vjoystate = Joy2vJoy_EnableStateMap[playerIndex];

            if (controlType == "Key11(LT)" && target == "LT") {
                joy2vjoystate.trigger_state |= JOY2VJOY_TRIGGER_LT;
            }
            else if (controlType == "Key12(RT)" && target == "RT") {
                joy2vjoystate.trigger_state |= JOY2VJOY_TRIGGER_RT;
            }
            else if (controlType == "LS" && target == "LS") {
                joy2vjoystate.ls_state |= JOY2VJOY_LS_2LS;
            }
            else if (controlType == "LS" && target == "RS") {
                joy2vjoystate.ls_state |= JOY2VJOY_LS_2RS;
            }
            else if (controlType == "RS" && target == "LS") {
                joy2vjoystate.rs_state |= JOY2VJOY_RS_2LS;
            }
            else if (controlType == "RS" && target == "RS") {
                joy2vjoystate.rs_state |= JOY2VJOY_RS_2RS;
            }

            int gamepad_index = 0;
            QString mappingkey_withoutindex;
            QString mappingkey = keymapdata.Mapping_Keys.constFirst();
            QRegularExpressionMatch joy2vjoy_mapkey_match = joy2vjoy_mapkey_regex.match(mappingkey);
            if (joy2vjoy_mapkey_match.hasMatch()) {
                mappingkey_withoutindex = joy2vjoy_mapkey_match.captured(1);
                QString gamepadIndexStr = joy2vjoy_mapkey_match.captured(4);
                gamepad_index = gamepadIndexStr.isEmpty() ? 0 : gamepadIndexStr.toInt();
            }

            if (gamepad_index > 0 && joy2vjoystate.gamepad_index == 0) {
                joy2vjoystate.gamepad_index = gamepad_index;
            }

            Q_UNUSED(originalkey_withoutindex);
            Q_UNUSED(mappingkey_withoutindex);
#ifdef DEBUG_LOGOUT_ON
            if (originalkey_withoutindex != mappingkey_withoutindex) {
                qDebug() << "[checkJoy2vJoyEnableStateMap]" << "OriginalKey and MappingKey unmatched! ->" << "OriKey: " << originalkey_withoutindex << "MapKey: " << mappingkey_withoutindex;
            }
#endif
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[checkJoy2vJoyEnableStateMap]" << "Joy2vJoy_EnableStateMap ->" << Joy2vJoy_EnableStateMap;
#endif

    return Joy2vJoy_EnableStateMap;
}

void QKeyMapper_Worker::processUdpPendingDatagrams()
{
    while (m_UdpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_UdpSocket->receiveDatagram();
        processForzaFormatData(datagram.data());
    }
}

void QKeyMapper_Worker::processForzaFormatData(const QByteArray &forzadata)
{
    qsizetype data_length = forzadata.size();
    bool extra_data = false;
    qsizetype buffer_offset = 0;

    if (data_length != FORZA_MOTOR_7_SLED_DATA_LENGTH
        && data_length != FORZA_MOTOR_7_DASH_DATA_LENGTH
        && data_length != FORZA_MOTOR_8_DASH_DATA_LENGTH
        && data_length != FORZA_HORIZON_DATA_LENGTH) {
        return;
    }
    else {
        if (data_length == FORZA_MOTOR_7_SLED_DATA_LENGTH) {
            extra_data = false;
        }
        else {
            extra_data = true;
            if (data_length == FORZA_HORIZON_DATA_LENGTH) {
                buffer_offset = FORZA_HORIZON_BUFFER_OFFSET;
            }
        }
    }

#ifdef GRIP_VERBOSE_LOG
    QString dataformat_str;
    if (data_length == FORZA_MOTOR_7_SLED_DATA_LENGTH) {
        dataformat_str = "Forza Motorsport 7 Sled";
    }
    else if (data_length == FORZA_MOTOR_7_DASH_DATA_LENGTH) {
        dataformat_str = "Forza Motorsport 7 Dash";
    }
    else if (data_length == FORZA_MOTOR_8_DASH_DATA_LENGTH) {
        dataformat_str = "Forza Motorsport 8 Dash";
    }
    else {
        dataformat_str = "Forza Horizon";
    }

    qDebug().nospace() << "[processForzaFormatData]" << dataformat_str << " data length = " << data_length;
#endif

    QByteArray firstPartData = forzadata.left(FIRAT_PART_DATA_LENGTH);
    QByteArray secondPartData;
    if (extra_data) {
        secondPartData = forzadata.mid(FIRAT_PART_DATA_LENGTH + buffer_offset, SECOND_PART_DATA_LENGTH);
    }

    QDataStream firstPartStream(firstPartData);
    firstPartStream.setByteOrder(QDataStream::LittleEndian);
    firstPartStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    QDataStream secondPartStream(secondPartData);
    secondPartStream.setByteOrder(QDataStream::LittleEndian);
    secondPartStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    /* First Part Data */
    qint32 is_race_on;
    quint32 timestamp_ms;
    float engine_max_rpm;
    float engine_idle_rpm;
    float current_engine_rpm;
    float acceleration_x;
    float acceleration_y;
    float acceleration_z;
    float velocity_x;
    float velocity_y;
    float velocity_z;
    float angular_velocity_x;
    float angular_velocity_y;
    float angular_velocity_z;
    float yaw;
    float pitch;
    float roll;
    float norm_suspension_travel_FL;
    float norm_suspension_travel_FR;
    float norm_suspension_travel_RL;
    float norm_suspension_travel_RR;
    float tire_slip_ratio_FL;
    float tire_slip_ratio_FR;
    float tire_slip_ratio_RL;
    float tire_slip_ratio_RR;
    float wheel_rotation_speed_FL;
    float wheel_rotation_speed_FR;
    float wheel_rotation_speed_RL;
    float wheel_rotation_speed_RR;
    float wheel_on_rumble_strip_FL;
    float wheel_on_rumble_strip_FR;
    float wheel_on_rumble_strip_RL;
    float wheel_on_rumble_strip_RR;
    float wheel_in_puddle_FL;
    float wheel_in_puddle_FR;
    float wheel_in_puddle_RL;
    float wheel_in_puddle_RR;
    float surface_rumble_FL;
    float surface_rumble_FR;
    float surface_rumble_RL;
    float surface_rumble_RR;
    float tire_slip_angle_FL;
    float tire_slip_angle_FR;
    float tire_slip_angle_RL;
    float tire_slip_angle_RR;
    float tire_combined_slip_FL;
    float tire_combined_slip_FR;
    float tire_combined_slip_RL;
    float tire_combined_slip_RR;
    float suspension_travel_meters_FL;
    float suspension_travel_meters_FR;
    float suspension_travel_meters_RL;
    float suspension_travel_meters_RR;
    qint32 car_ordinal;
    qint32 car_class;
    qint32 car_performance_index;
    qint32 drivetrain_type;
    qint32 num_cylinders;

    /* Second Part Data */
    float position_x;
    float position_y;
    float position_z;
    float speed;
    float power;
    float torque;
    float tire_temp_FL;
    float tire_temp_FR;
    float tire_temp_RL;
    float tire_temp_RR;
    float boost;
    float fuel;
    float dist_traveled;
    float best_lap_time;
    float last_lap_time;
    float cur_lap_time;
    float cur_race_time;
    quint16 lap_no;
    quint8 race_pos;
    quint8 accel;
    quint8 brake;
    quint8 clutch;
    quint8 handbrake;
    quint8 gear;
    qint8 steer;
    qint8 norm_driving_line;
    qint8 norm_ai_brake_diff;

    firstPartStream >> is_race_on;
    firstPartStream >> timestamp_ms;
    firstPartStream >> engine_max_rpm;
    firstPartStream >> engine_idle_rpm;
    firstPartStream >> current_engine_rpm;
    firstPartStream >> acceleration_x;
    firstPartStream >> acceleration_y;
    firstPartStream >> acceleration_z;
    firstPartStream >> velocity_x;
    firstPartStream >> velocity_y;
    firstPartStream >> velocity_z;
    firstPartStream >> angular_velocity_x;
    firstPartStream >> angular_velocity_y;
    firstPartStream >> angular_velocity_z;
    firstPartStream >> yaw;
    firstPartStream >> pitch;
    firstPartStream >> roll;
    firstPartStream >> norm_suspension_travel_FL;
    firstPartStream >> norm_suspension_travel_FR;
    firstPartStream >> norm_suspension_travel_RL;
    firstPartStream >> norm_suspension_travel_RR;
    firstPartStream >> tire_slip_ratio_FL;
    firstPartStream >> tire_slip_ratio_FR;
    firstPartStream >> tire_slip_ratio_RL;
    firstPartStream >> tire_slip_ratio_RR;
    firstPartStream >> wheel_rotation_speed_FL;
    firstPartStream >> wheel_rotation_speed_FR;
    firstPartStream >> wheel_rotation_speed_RL;
    firstPartStream >> wheel_rotation_speed_RR;
    firstPartStream >> wheel_on_rumble_strip_FL;
    firstPartStream >> wheel_on_rumble_strip_FR;
    firstPartStream >> wheel_on_rumble_strip_RL;
    firstPartStream >> wheel_on_rumble_strip_RR;
    firstPartStream >> wheel_in_puddle_FL;
    firstPartStream >> wheel_in_puddle_FR;
    firstPartStream >> wheel_in_puddle_RL;
    firstPartStream >> wheel_in_puddle_RR;
    firstPartStream >> surface_rumble_FL;
    firstPartStream >> surface_rumble_FR;
    firstPartStream >> surface_rumble_RL;
    firstPartStream >> surface_rumble_RR;
    firstPartStream >> tire_slip_angle_FL;
    firstPartStream >> tire_slip_angle_FR;
    firstPartStream >> tire_slip_angle_RL;
    firstPartStream >> tire_slip_angle_RR;
    firstPartStream >> tire_combined_slip_FL;
    firstPartStream >> tire_combined_slip_FR;
    firstPartStream >> tire_combined_slip_RL;
    firstPartStream >> tire_combined_slip_RR;
    firstPartStream >> suspension_travel_meters_FL;
    firstPartStream >> suspension_travel_meters_FR;
    firstPartStream >> suspension_travel_meters_RL;
    firstPartStream >> suspension_travel_meters_RR;
    firstPartStream >> car_ordinal;
    firstPartStream >> car_class;
    firstPartStream >> car_performance_index;
    firstPartStream >> drivetrain_type;
    firstPartStream >> num_cylinders;

    if (extra_data) {
    secondPartStream >> position_x;
    secondPartStream >> position_y;
    secondPartStream >> position_z;
    secondPartStream >> speed;
    secondPartStream >> power;
    secondPartStream >> torque;
    secondPartStream >> tire_temp_FL;
    secondPartStream >> tire_temp_FR;
    secondPartStream >> tire_temp_RL;
    secondPartStream >> tire_temp_RR;
    secondPartStream >> boost;
    secondPartStream >> fuel;
    secondPartStream >> dist_traveled;
    secondPartStream >> best_lap_time;
    secondPartStream >> last_lap_time;
    secondPartStream >> cur_lap_time;
    secondPartStream >> cur_race_time;
    secondPartStream >> lap_no;
    secondPartStream >> race_pos;
    secondPartStream >> accel;
    secondPartStream >> brake;
    secondPartStream >> clutch;
    secondPartStream >> handbrake;
    secondPartStream >> gear;
    secondPartStream >> steer;
    secondPartStream >> norm_driving_line;
    secondPartStream >> norm_ai_brake_diff;
    }

    // double average_slip_ratio = (qAbs(tire_slip_ratio_FL) + qAbs(tire_slip_ratio_FR) + qAbs(tire_slip_ratio_RL) + qAbs(tire_slip_ratio_RR)) / 4;
    // double average_slip_ratio = (qAbs(tire_slip_ratio_RL) + qAbs(tire_slip_ratio_RR)) / 2;
    // double max_slip_ratio = qMax(qMax(qAbs(tire_slip_ratio_FL), qAbs(tire_slip_ratio_FR)), qMax(qAbs(tire_slip_ratio_RL), qAbs(tire_slip_ratio_RR)));
    // double max_slip_ratio = qMax(qAbs(tire_slip_ratio_RL), qAbs(tire_slip_ratio_RR));
    double average_slip_ratio = (qAbs(tire_combined_slip_FL) + qAbs(tire_combined_slip_FR) + qAbs(tire_combined_slip_RL) + qAbs(tire_combined_slip_RR)) / 4;
    double max_slip_ratio = qMax(qMax(qAbs(tire_combined_slip_FL), qAbs(tire_combined_slip_FR)), qMax(qAbs(tire_combined_slip_RL), qAbs(tire_combined_slip_RR)));
    if (s_LastCarOrdinal != car_ordinal) {
        s_LastCarOrdinal = car_ordinal;
    }

#ifdef GRIP_VERBOSE_LOG
    // qDebug().nospace() << "[processForzaFormatData]" << " secondPartData = " << secondPartData.toHex();
    // qDebug().nospace() << "[processForzaFormatData]" << " thirdPartData = " << thirdPartData.toHex();
    qDebug() << "[processForzaFormatData]" << "tire_slip_ratio_FL =" << tire_slip_ratio_FL << ", tire_slip_ratio_FR =" << tire_slip_ratio_FR << ", tire_slip_ratio_RL =" << tire_slip_ratio_RL << ", tire_slip_ratio_RR =" << tire_slip_ratio_RR;
    qDebug() << "[processForzaFormatData]" << "tire_slip_angle_FL =" << tire_slip_angle_FL << ", tire_slip_angle_FR =" << tire_slip_angle_FR << ", tire_slip_angle_RL =" << tire_slip_angle_RL << ", tire_slip_angle_RR =" << tire_slip_angle_RR;
    qDebug() << "[processForzaFormatData]" << "tire_combined_slip_FL =" << tire_combined_slip_FL << ", tire_combined_slip_FR =" << tire_combined_slip_FR << ", tire_combined_slip_RL =" << tire_combined_slip_RL << ", tire_combined_slip_RR =" << tire_combined_slip_RR;
    // qDebug() << "[processForzaFormatData]" << "tire_slip_ratio_RL =" << tire_slip_ratio_RL << ", tire_slip_ratio_RR =" << tire_slip_ratio_RR;
    qDebug() << "[processForzaFormatData]" << "average_slip_ratio =" << average_slip_ratio << ", max_slip_ratio =" << max_slip_ratio << ", car_ordinal =" << car_ordinal;
#endif

    if (average_slip_ratio > GRIP_THRESHOLD_MAX) {
        average_slip_ratio = GRIP_THRESHOLD_MAX;
    }

    if (max_slip_ratio > GRIP_THRESHOLD_MAX) {
        max_slip_ratio = GRIP_THRESHOLD_MAX;
    }

    int autoadjust = AUTO_ADJUST_NONE;
    double gripThreshold_Brake = QKeyMapper::getBrakeThreshold();
    double gripThreshold_Accel = QKeyMapper::getAccelThreshold();
    int gamepad_index = 0;
    QStringList& pressedvJoyButtons_ref = pressedvJoyButtonsList[gamepad_index];
    if (average_slip_ratio > gripThreshold_Brake || max_slip_ratio > gripThreshold_Brake) {
    // if (average_slip_ratio > gripThreshold_Brake) {
        if (pressedvJoyButtons_ref.contains(VJOY_LT_BRAKE_STR) || pressedvJoyButtons_ref.contains(VJOY_RT_BRAKE_STR)){
            if (s_Auto_Brake > AUTO_BRAKE_ADJUST_VALUE) {
                s_Auto_Brake -= AUTO_BRAKE_ADJUST_VALUE;
#ifdef GRIP_VERBOSE_LOG
                qDebug() << "[processForzaFormatData]" << "s_Auto_Brake ----- ->" << s_Auto_Brake;
#endif
            }

            autoadjust |= AUTO_ADJUST_BRAKE;
        }
    }
    else {
        if (pressedvJoyButtons_ref.contains(VJOY_LT_BRAKE_STR) || pressedvJoyButtons_ref.contains(VJOY_RT_BRAKE_STR)){
            if (s_Auto_Brake < XINPUT_TRIGGER_MAX) {
                s_Auto_Brake += AUTO_BRAKE_ADJUST_VALUE;
                if (s_Auto_Brake > XINPUT_TRIGGER_MAX) {
                    s_Auto_Brake = XINPUT_TRIGGER_MAX;
                }
#ifdef GRIP_VERBOSE_LOG
                qDebug() << "[processForzaFormatData]" << "s_Auto_Brake +++++ ->" << s_Auto_Brake;
#endif
            }

            autoadjust |= AUTO_ADJUST_BRAKE;
        }
    }

    if (average_slip_ratio > gripThreshold_Accel || max_slip_ratio > gripThreshold_Accel) {
    // if (average_slip_ratio > gripThreshold_Accel) {
        if (pressedvJoyButtons_ref.contains(VJOY_LT_ACCEL_STR) || pressedvJoyButtons_ref.contains(VJOY_RT_ACCEL_STR)){
            if (s_Auto_Accel > AUTO_ACCEL_ADJUST_VALUE) {
                s_Auto_Accel -= AUTO_ACCEL_ADJUST_VALUE;
#ifdef GRIP_VERBOSE_LOG
                qDebug() << "[processForzaFormatData]" << "s_Auto_Accel ----- ->" << s_Auto_Accel;
#endif
            }

            autoadjust |= AUTO_ADJUST_ACCEL;
        }
    }
    else {
        if (pressedvJoyButtons_ref.contains(VJOY_LT_ACCEL_STR) || pressedvJoyButtons_ref.contains(VJOY_RT_ACCEL_STR)){
            if (s_Auto_Accel < XINPUT_TRIGGER_MAX) {
                s_Auto_Accel += AUTO_ACCEL_ADJUST_VALUE;
                if (s_Auto_Accel > XINPUT_TRIGGER_MAX) {
                    s_Auto_Accel = XINPUT_TRIGGER_MAX;
                }
#ifdef GRIP_VERBOSE_LOG
                qDebug() << "[processForzaFormatData]" << "s_Auto_Accel +++++ ->" << s_Auto_Accel;
#endif
            }

            autoadjust |= AUTO_ADJUST_ACCEL;
        }
    }

    if (autoadjust) {
#ifdef GRIP_VERBOSE_LOG
        qDebug() << "[processForzaFormatData]" << "Current Adjusted Auto Data ->" << "s_Auto_Brake =" << s_Auto_Brake << "s_last_Auto_Brake =" << s_last_Auto_Brake << ", s_Auto_Accel =" << s_Auto_Accel << ", s_last_Auto_Accel =" << s_last_Auto_Accel;
#endif
        QString autoadjustEmptyStr;
        ViGEmClient_PressButton(autoadjustEmptyStr, autoadjust, gamepad_index, INITIAL_PLAYER_INDEX);
    }
}

#ifdef DINPUT_TEST
void QKeyMapper_Worker::setWorkerDInputKeyHook(HWND hWnd)
{
    if(TRUE == IsWindowVisible(hWnd)){
        m_DirectInput = Q_NULLPTR;
        HRESULT hResult;
//        hResult = DirectInput8Create(GetModuleHandle(Q_NULLPTR), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&m_DirectInput, Q_NULLPTR);
//        QString process_name("bot_vice.exe");
//        HMODULE hModule = GetModuleHandle(process_name.toStdWString().c_str());
        HMODULE hModule = GetModuleHandle(Q_NULLPTR);
//        hResult = DirectInput8Create((HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&m_DirectInput, Q_NULLPTR);
        hResult = DirectInput8Create(hModule, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&m_DirectInput, Q_NULLPTR);
        if (DI_OK == hResult)
        {
            LPDIRECTINPUTDEVICE8 lpdiKeyboard;
            hResult = m_DirectInput->CreateDevice(GUID_SysKeyboard, &lpdiKeyboard, Q_NULLPTR);
            if (DI_OK == hResult)
            {
                FuncPtrGetDeviceState = (GetDeviceStateT)HookVTableFunction(lpdiKeyboard, hookGetDeviceState, 9);
                FuncPtrGetDeviceData = (GetDeviceDataT)HookVTableFunction(lpdiKeyboard, hookGetDeviceData, 10);
//                dinput_timerid = startTimer(300, Qt::PreciseTimer);
                qDebug().nospace().noquote() << "[DINPUT] " << "DirectInput Key Hook Started.";
            }
            else{
                m_DirectInput->Release();
                m_DirectInput = Q_NULLPTR;
                FuncPtrGetDeviceState = Q_NULLPTR;
                FuncPtrGetDeviceData = Q_NULLPTR;
                qDebug().noquote() << "[DINPUT]" << "Failed to Create Keyboard Device!";
            }
        }
        else{
            qDebug().noquote() << "[DINPUT]" << "Failed to acquire DirectInput handle";
        }
    }
    else{
        qDebug().nospace().noquote() << "[DINPUT] " << "Invisible window!";
    }
}

void QKeyMapper_Worker::setWorkerDInputKeyUnHook()
{
    if (m_DirectInput != Q_NULLPTR){
        m_DirectInput->Release();
        m_DirectInput = Q_NULLPTR;
        qDebug().nospace().noquote() << "[DINPUT] " << "DirectInput Key Hook Released.";
    }
    FuncPtrGetDeviceState = Q_NULLPTR;
    FuncPtrGetDeviceData = Q_NULLPTR;
}
#endif

#if 0
void QKeyMapper_Worker::startBurstTimer(const QString &burstKey, int mappingIndex)
{
    QMutexLocker locker(&m_BurstTimerMutex);

    Q_UNUSED(mappingIndex);
    if (true == m_BurstTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("[startBurstTimer] Key \"%s\" BurstTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstTimerMap.remove(burstKey);
    }
    if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("[startBurstTimer] Key \"%s\" BurstKeyUpTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstKeyUpTimerMap.remove(burstKey);
    }
    // int burstpressTime = QKeyMapper::getBurstPressTime();
    // int burstreleaseTime = QKeyMapper::getBurstReleaseTime();
    int burstpressTime = QKeyMapper::KeyMappingDataList->at(mappingIndex).BurstPressTime;
    int burstreleaseTime = QKeyMapper::KeyMappingDataList->at(mappingIndex).BurstReleaseTime;
    int burstTime = burstpressTime + burstreleaseTime;
    int timerID = startTimer(burstTime, Qt::PreciseTimer);
    m_BurstTimerMap.insert(burstKey, timerID);

    int keyupTimerID = startTimer(burstpressTime, Qt::PreciseTimer);
    m_BurstKeyUpTimerMap.insert(burstKey, keyupTimerID);

#ifdef DEBUG_LOGOUT_ON
    qDebug("[startBurstTimer] Key \"%s\", BurstTimer: %d, ID: %d, BurstUpTimer: %d, ID: %d, MappingDataIndex: %d", burstKey.toStdString().c_str(), burstTime, timerID, burstpressTime, keyupTimerID, mappingIndex);
#endif
}

void QKeyMapper_Worker::stopBurstTimer(const QString &burstKey, int mappingIndex)
{
    QMutexLocker locker(&m_BurstTimerMutex);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[stopBurstTimer] Key [" << burstKey << "], MappingIndex =" << mappingIndex;
#endif
    Q_UNUSED(mappingIndex);

    if (true == m_BurstTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstTimerMap.value(burstKey);
        killTimer(existTimerID);
        m_BurstTimerMap.remove(burstKey);

        if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
            // sendBurstKeyUp(burstKey, true);
            sendBurstKeyUp(mappingIndex, true);
#ifdef DEBUG_LOGOUT_ON
            qDebug("[stopBurstTimer] sendBurstKeyUp(\"%s\"), BurstTimer(%d) stoped.", burstKey.toStdString().c_str(), existTimerID);
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug("[stopBurstTimer] Do not need to sendBurstKeyUp(\"%s\"), BurstTimer(%d) stoped.", burstKey.toStdString().c_str(), existTimerID);
#endif
        }
    }

    if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("[stopBurstTimer] Key \"%s\" kill BurstKeyUpTimer(%d)", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstKeyUpTimerMap.remove(burstKey);
    }
}
#endif

#if 0
void QKeyMapper_Worker::onJoystickcountChanged()
{
    QList<QJoystickDevice *> joysticklist = QJoysticks::getInstance()->inputDevices();

    if (joysticklist.size() != s_ViGEmTargetList.size()) {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[onJoystickcountChanged]" << "Size error! QJoysticks size =" << joysticklist.size() << "ViGEmTargetList size =" << s_ViGEmTargetList.size();
#endif
        return;
    }
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onJoystickcountChanged]" << "JoystickList Start >>>" << joysticklist.size();
#endif
    int joystick_index = 0;
    for (const QJoystickDevice *joystick : std::as_const(joysticklist)) {
        bool virtualgamepad = false;
        USHORT vendorid = joystick->vendorid;
        USHORT productid = joystick->productid;
        int numbuttons = joystick->numbuttons;
        Q_UNUSED(numbuttons);

        if (vendorid == VIRTUALGAMPAD_VENDORID_X360
            && productid == VIRTUALGAMPAD_PRODUCTID_X360) {
            virtualgamepad = true;
        }
        else if (joystick->serial.startsWith(VIRTUALGAMPAD_SERIAL_PREFIX_DS4)
            && vendorid == VIRTUALGAMPAD_VENDORID_DS4
            && productid == VIRTUALGAMPAD_PRODUCTID_DS4) {
            virtualgamepad = true;
        }

        if (virtualgamepad) {
            QJoysticks::getInstance()->setBlacklisted(joystick_index, true);
        }

#ifdef DEBUG_LOGOUT_ON
        QString vendorIdStr = QString("0x%1").arg(QString::number(vendorid, 16).toUpper(), 4, '0');
        QString productIdStr = QString("0x%1").arg(QString::number(productid, 16).toUpper(), 4, '0');
        qDebug().nospace() << "[onJoystickcountChanged] Joystick[" << joystick_index << "] -> " << "Name = " << joystick->name << ", VendorID = " << vendorIdStr << ", ProductID = " << productIdStr << ", ButtonNums = " << numbuttons << ", Serial = " << joystick->serial << ", Virtualgamepad = " << virtualgamepad;
#endif

        joystick_index += 1;
    }
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onJoystickcountChanged]" << "JoystickList End <<<";
#endif
}
#endif

void QKeyMapper_Worker::onJoystickAdded(QJoystickDevice *joystick_added)
{
    if (joystick_added == Q_NULLPTR) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    QString vendorIdStr = QString("0x%1").arg(QString::number(joystick_added->vendorid, 16).toUpper(), 4, '0');
    QString productIdStr = QString("0x%1").arg(QString::number(joystick_added->productid, 16).toUpper(), 4, '0');
    QString debugmessage = QString("[onJoystickAdded] Added a New Gamepad -> Name=\"%1\", PlayerIndex=%2, ID=%3, VendorID=%4, ProductID=%5, ButtonNumbers=%6, Serial=%7, HasGyro=%8, HasAccel=%9")
        .arg(joystick_added->name)
        .arg(joystick_added->playerindex)
        .arg(joystick_added->id)
        .arg(vendorIdStr)
        .arg(productIdStr)
        .arg(joystick_added->numbuttons)
        .arg(joystick_added->serial,
             joystick_added->has_gyro ? "true" : "false",
             joystick_added->has_accel ? "true" : "false");
    qDebug().nospace().noquote() << debugmessage;
#endif

    bool virtualgamepad = false;
    USHORT vendorid = joystick_added->vendorid;
    USHORT productid = joystick_added->productid;

    if (vendorid == VIRTUALGAMPAD_VENDORID_X360
        && productid == VIRTUALGAMPAD_PRODUCTID_X360) {
        virtualgamepad = true;
    }
    else if (joystick_added->serial.startsWith(VIRTUALGAMPAD_SERIAL_PREFIX_DS4)
        && vendorid == VIRTUALGAMPAD_VENDORID_DS4
        && productid == VIRTUALGAMPAD_PRODUCTID_DS4) {
        virtualgamepad = true;
    }

    if (virtualgamepad) {
        joystick_added->blacklisted = true;
#ifdef DEBUG_LOGOUT_ON
        // Build debug message for a blacklisted virtual gamepad.
        // Note: use single-argument arg() chaining to avoid placeholder re-numbering issues.
        QString debugmessage =
            QString("[onJoystickAdded] VirtualGamepad[%1] PlayerIndex=%2, ID=%3, VendorID=%4, ProductID=%5, is Blacklisted!")
                .arg(joystick_added->name)              // %1 -> device name
                .arg(joystick_added->playerindex)       // %2 -> player index
                .arg(joystick_added->id)                // %3 -> internal id
                .arg(vendorIdStr)                       // %4 -> formatted vendor ID
                .arg(productIdStr);                     // %5 -> formatted product ID
        qDebug().nospace().noquote() << debugmessage;
#endif
    }

    emit QKeyMapper::getInstance()->updateGamepadSelectComboBox_Signal(JOYSTICK_INVALID_INSTANCE_ID);
}

void QKeyMapper_Worker::onJoystickRemoved(const QJoystickDevice joystick_removed)
{
    Q_UNUSED(joystick_removed);
#ifdef DEBUG_LOGOUT_ON
    QString vendorIdStr = QString("0x%1").arg(QString::number(joystick_removed.vendorid, 16).toUpper(), 4, '0');
    QString productIdStr = QString("0x%1").arg(QString::number(joystick_removed.productid, 16).toUpper(), 4, '0');

    // Build message with strictly ordered single-arg replacements
    QString debugmessage =
        QString("[onJoystickRemoved] Removed a Gamepad -> Name=\"%1\", PlayerIndex=%2, ID=%3, VendorID=%4, ProductID=%5, ButtonNumbers=%6, Serial=%7")
            .arg(joystick_removed.name)
            .arg(joystick_removed.playerindex)
            .arg(joystick_removed.id)
            .arg(vendorIdStr)
            .arg(productIdStr)
            .arg(joystick_removed.numbuttons)
            .arg(joystick_removed.serial);

    qDebug().nospace().noquote() << debugmessage;
#endif

    emit QKeyMapper::getInstance()->updateGamepadSelectComboBox_Signal(JOYSTICK_INVALID_INSTANCE_ID);
}

void QKeyMapper_Worker::onJoystickPOVEvent(const QJoystickPOVEvent &e)
{
#ifdef JOYSTICK_VERBOSE_LOG
    qDebug().nospace() << "[onJoystickPOVEvent]" << "P[" << e.joystick->playerindex << "] POV ->" << e.pov << "," << "POV Angle ->" << e.angle;
#endif

    if (e.joystick->blacklisted
        && QKeyMapper::getAcceptVirtualGamepadInputStatus() == false) {
        return;
    }

    checkJoystickPOV(e);
}

void QKeyMapper_Worker::onJoystickAxisEvent(const QJoystickAxisEvent &e)
{
#ifdef JOYSTICK_VERBOSE_LOG
    qDebug().nospace() << "[onJoystickAxisEvent]" << "P[" << e.joystick->playerindex << "] axis ->" << e.axis << "," << "axis value ->" << e.value;
#endif

    if (e.joystick->blacklisted
        && QKeyMapper::getAcceptVirtualGamepadInputStatus() == false) {
        return;
    }

    QJoystickAxisEvent axisEvent = e;
    #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if (qFabs(axisEvent.value) < JOYSTICK_AXIS_NEAR_ZERO_THRESHOLD) {
        axisEvent.value = 0;
    }
    #else
    if (std::fabs(axisEvent.value) < JOYSTICK_AXIS_NEAR_ZERO_THRESHOLD) {
        axisEvent.value = 0;
    }
    #endif
    checkJoystickAxis(axisEvent);
}

void QKeyMapper_Worker::onJoystickButtonEvent(const QJoystickButtonEvent &e)
{
#ifdef JOYSTICK_VERBOSE_LOG
    qDebug().nospace() << "[onJoystickButtonEvent]" << "P[" << e.joystick->playerindex << "] Button ->" << e.button << "," << "Pressed ->" << e.pressed;
#endif

    if (e.joystick->blacklisted
        && QKeyMapper::getAcceptVirtualGamepadInputStatus() == false) {
        return;
    }

    checkJoystickButtons(e);
}

void QKeyMapper_Worker::onJoystickSensorEvent(const QJoystickSensorEvent &e)
{
    /* Virtual gamecontroller do not set sensor enabled */
    if (e.joystick->blacklisted
        && QKeyMapper::getAcceptVirtualGamepadInputStatus() == false) {
        return;
    }

// #ifdef GAMECONTROLLER_SENSOR_VERBOSE_LOG
//     qDebug().nospace() << "[onJoystickSensorEvent] "
//                        << "P[" << e.joystick->playerindex << "] "
//                        << "GyroX ->" << e.gyroX  << ", "
//                        << "GyroY ->" << e.gyroY  << ", "
//                        << "GyroZ ->" << e.gyroZ  << ", "
//                        << "AccelX ->" << e.accelX  << ", "
//                        << "AccelY ->" << e.accelY  << ", "
//                        << "AccelZ ->" << e.accelZ  << ", "
//                        << "Timestamp ->" << e.timestamp;
// #endif

    checkJoystickSensor(e);
}

void QKeyMapper_Worker::onGameControllerGyroEnabledSwitch(int gamepadinfo_index)
{
#ifdef DEBUG_LOGOUT_ON
    QString debugmessage = QString("[onGameControllerGyroEnabledSwitch] Index(%1) game controller gyro enabled state switch").arg(gamepadinfo_index);
    qDebug().noquote().nospace() << debugmessage;
#endif

    if (QKeyMapper::getInstance()->m_GamepadInfoMap.contains(gamepadinfo_index)) {
        bool isvirtual = QKeyMapper::getInstance()->m_GamepadInfoMap.value(gamepadinfo_index).isvirtual;
        if (isvirtual != true) {
            int instance_id = QKeyMapper::getInstance()->m_GamepadInfoMap.value(gamepadinfo_index).instance_id;
            QJoysticks::getInstance()->switchSensorDisabled(instance_id);

            emit QKeyMapper::getInstance()->updateGamepadSelectComboBox_Signal(instance_id);
        }
    }
}

void QKeyMapper_Worker::checkJoystickButtons(const QJoystickButtonEvent &e)
{
    if (e.joystick == Q_NULLPTR)
        return;

    if (e.button_type == GameControllerButton) {
        GameControllerButtonCode buttonCode = (GameControllerButtonCode)e.button;

        if (m_ControllerButtonMap.contains(buttonCode)) {
            QString keycodeString = m_ControllerButtonMap.value(buttonCode);
            int keyupdown;
            if (e.pressed) {
                keyupdown = KEY_DOWN;
            }
            else {
                keyupdown = KEY_UP;
            }

            bool returnFlag;
            returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick);
            Q_UNUSED(returnFlag);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[checkJoystickButtons] Unknown Gamecontroller button:" << e.button;
#endif
        }
    }
    else {
        JoystickButtonCode buttonCode = (JoystickButtonCode)e.button;

        if (m_JoystickButtonMap.contains(buttonCode)) {
            QString keycodeString = m_JoystickButtonMap.value(buttonCode);
            int keyupdown;
            if (e.pressed) {
                keyupdown = KEY_DOWN;
            }
            else {
                keyupdown = KEY_UP;
            }

            bool returnFlag;
            returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick);
            Q_UNUSED(returnFlag);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[checkJoystickButtons] Unknown Joystick button:" << e.button;
#endif
        }
    }
}

void QKeyMapper_Worker::checkJoystickPOV(const QJoystickPOVEvent &e)
{
    if (e.joystick == Q_NULLPTR)
        return;

    int playerIndex = e.joystick->playerindex;

    if (m_JoystickPOVMap.contains(e.angle)) {
        JoystickDPadCode dpadCode = m_JoystickPOVMap.value(e.angle);

        int keyupdown = KEY_UP;
        if (JOYSTICK_DPAD_RELEASE == dpadCode) {
            keyupdown = KEY_UP;
        }
        else {
            keyupdown = KEY_DOWN;
        }

        QString pattern = QString("^Joy-DPad-(Up|Down|Left|Right)@%1$").arg(playerIndex);
        QRegularExpression filterPattern(pattern);
        static QRegularExpression joydpad_regex(R"(^(Joy-DPad-(Up|Down|Left|Right))@(\d)$)");
        QRegularExpressionMatch joydpad_match;
        QStringList joydpadpressedlist = pressedRealKeysList.filter(filterPattern);
        QStringList joydpadNeedtoRelease = joydpadpressedlist;
        bool returnFlag;
        if (KEY_DOWN == keyupdown) {
            QString keycodeString = m_JoystickDPadMap.value(dpadCode);
            QStringList tempDpadCodeStringList = keycodeString.split(',');

            for (const QString &dpadcodestr : std::as_const(tempDpadCodeStringList)) {
                QString dpadcodestrWithIndex = QString("%1@%2").arg(dpadcodestr).arg(playerIndex);
                joydpadNeedtoRelease.removeAll(dpadcodestrWithIndex);
            }

            for (const QString &dpadcodestr_withindex : std::as_const(joydpadNeedtoRelease)) {
                joydpad_match = joydpad_regex.match(dpadcodestr_withindex);
                QString dpadcodestr = joydpad_match.captured(1);
                returnFlag = JoyStickKeysProc(dpadcodestr, KEY_UP, e.joystick);
                Q_UNUSED(returnFlag);
            }

            for (const QString &dpadcodestr : std::as_const(tempDpadCodeStringList)) {
                returnFlag = JoyStickKeysProc(dpadcodestr, keyupdown, e.joystick);
                Q_UNUSED(returnFlag);
            }
        }
        else {
            for (const QString &dpadcodestr_withindex : std::as_const(joydpadpressedlist)){
                joydpad_match = joydpad_regex.match(dpadcodestr_withindex);
                QString dpadcodestr = joydpad_match.captured(1);
                returnFlag = JoyStickKeysProc(dpadcodestr, keyupdown, e.joystick);
                Q_UNUSED(returnFlag);
            }
        }
    }
}

void QKeyMapper_Worker::checkJoystickAxis(const QJoystickAxisEvent &e)
{
    bool joy2vjoy_update = false;
    int player_index = e.joystick->playerindex;
    if (player_index < JOYSTICK_PLAYER_INDEX_MIN || player_index > JOYSTICK_PLAYER_INDEX_MAX) {
        player_index = INITIAL_PLAYER_INDEX;
    }
    s_LastJoyAxisPlayerIndex = player_index;
    int gamepad_index = 0;
    Joy2vJoyState Joy2vJoy_EnableState = Joy2vJoyState();
    if (s_Joy2vJoy_EnableStateMap.contains(INITIAL_PLAYER_INDEX)) {
        player_index = -1;
        joy2vjoy_update = true;
    }
    else if (s_Joy2vJoy_EnableStateMap.contains(player_index)) {
        joy2vjoy_update = true;
    }

    if (joy2vjoy_update) {
        Joy2vJoy_EnableState = s_Joy2vJoy_EnableStateMap.value(player_index);
        gamepad_index = Joy2vJoy_EnableState.gamepad_index;
    }

    if (e.joystick->blacklisted) {
        s_JoyAxisStateMap[INITIAL_PLAYER_INDEX].isvirtual = true;
        s_JoyAxisStateMap[player_index].isvirtual = true;
    }
    else {
        s_JoyAxisStateMap[INITIAL_PLAYER_INDEX].isvirtual = false;
        s_JoyAxisStateMap[player_index].isvirtual = false;
    }

    if (JOYSTICK_AXIS_LT_BUTTON == e.axis) {
        // s_JoyAxisState.left_trigger = e.value;
        s_JoyAxisStateMap[INITIAL_PLAYER_INDEX].left_trigger = e.value;
        s_JoyAxisStateMap[player_index].left_trigger = e.value;
        if (JOY2VJOY_TRIGGER_LT == Joy2vJoy_EnableState.trigger_state || JOY2VJOY_TRIGGER_LTRT_BOTH == Joy2vJoy_EnableState.trigger_state) {
            QString autoadjustEmptyStr;
            ViGEmClient_PressButton(autoadjustEmptyStr, AUTO_ADJUST_LT, gamepad_index, player_index);
        }
        else {
            joystickLTRTButtonProc(e);
        }
    }
    else if (JOYSTICK_AXIS_RT_BUTTON == e.axis) {
        // s_JoyAxisState.right_trigger = e.value;
        s_JoyAxisStateMap[INITIAL_PLAYER_INDEX].right_trigger = e.value;
        s_JoyAxisStateMap[player_index].right_trigger = e.value;
        if (JOY2VJOY_TRIGGER_RT == Joy2vJoy_EnableState.trigger_state || JOY2VJOY_TRIGGER_LTRT_BOTH == Joy2vJoy_EnableState.trigger_state) {
            QString autoadjustEmptyStr;
            ViGEmClient_PressButton(autoadjustEmptyStr, AUTO_ADJUST_RT, gamepad_index, player_index);
        }
        else {
            joystickLTRTButtonProc(e);
        }
    }
    else if (JOYSTICK_AXIS_LS_HORIZONTAL == e.axis) {
        // s_JoyAxisState.left_x = e.value;
        s_JoyAxisStateMap[INITIAL_PLAYER_INDEX].left_x = e.value;
        s_JoyAxisStateMap[player_index].left_x = e.value;
        if (Joy2vJoy_EnableState.ls_state != JOY2VJOY_LS_NONE) {
            ViGEmClient_Joy2vJoystickUpdate(Joy2vJoy_EnableState, JOY2VJOY_LEFTSTICK_X, gamepad_index, player_index);
        }
        else {
            joystickLSHorizontalProc(e);
        }
    }
    else if (JOYSTICK_AXIS_LS_VERTICAL == e.axis) {
        // s_JoyAxisState.left_y = e.value;
        s_JoyAxisStateMap[INITIAL_PLAYER_INDEX].left_y = e.value;
        s_JoyAxisStateMap[player_index].left_y = e.value;
        if (Joy2vJoy_EnableState.ls_state != JOY2VJOY_LS_NONE) {
            ViGEmClient_Joy2vJoystickUpdate(Joy2vJoy_EnableState, JOY2VJOY_LEFTSTICK_Y, gamepad_index, player_index);
        }
        else {
            joystickLSVerticalProc(e);
        }
    }
    else if (JOYSTICK_AXIS_RS_HORIZONTAL == e.axis) {
        // s_JoyAxisState.right_x = e.value;
        s_JoyAxisStateMap[INITIAL_PLAYER_INDEX].right_x = e.value;
        s_JoyAxisStateMap[player_index].right_x = e.value;
        if (Joy2vJoy_EnableState.rs_state != JOY2VJOY_RS_NONE) {
            ViGEmClient_Joy2vJoystickUpdate(Joy2vJoy_EnableState, JOY2VJOY_RIGHTSTICK_X, gamepad_index, player_index);
        }
        else {
            joystickRSHorizontalProc(e);
        }
    }
    else if (JOYSTICK_AXIS_RS_VERTICAL == e.axis) {
        // s_JoyAxisState.right_y = e.value;
        s_JoyAxisStateMap[INITIAL_PLAYER_INDEX].right_y = e.value;
        s_JoyAxisStateMap[player_index].right_y = e.value;
        if (Joy2vJoy_EnableState.rs_state != JOY2VJOY_RS_NONE) {
            ViGEmClient_Joy2vJoystickUpdate(Joy2vJoy_EnableState, JOY2VJOY_RIGHTSTICK_Y, gamepad_index, player_index);
        }
        else {
            joystickRSVerticalProc(e);
        }
    }
}

void QKeyMapper_Worker::checkJoystickSensor(const QJoystickSensorEvent &e)
{
    if (e.joystick == Q_NULLPTR)
        return;

    GameControllerSensorData sensor_data;
    sensor_data.gyroX = e.gyroX;
    sensor_data.gyroY = e.gyroY;
    sensor_data.gyroZ = e.gyroZ;
    sensor_data.accelX = e.accelX;
    sensor_data.accelY = e.accelY;
    sensor_data.accelZ = e.accelZ;
    sensor_data.timestamp = e.timestamp;

    gyro2MouseMoveProc(sensor_data);
}

void QKeyMapper_Worker::startMouse2vJoyResetTimer(const QString &mouse2joy_keystr, int mouse_index_param)
{
    Q_UNUSED(mouse_index_param);
    Q_UNUSED(mouse2joy_keystr);

    int recenter_timeout = QKeyMapper::getvJoyRecenterTimeout();
    if (recenter_timeout <= 0) {
        return;
    }
    QList<int> mouse_index_list = s_Mouse2vJoy_EnableStateMap.keys();
    for (const int& mouse_index : std::as_const(mouse_index_list)) {
        m_Mouse2vJoyResetTimerMap.value(mouse_index)->start(recenter_timeout);
#ifdef DEBUG_LOGOUT_ON
        QString debugmessage = QString("[startMouse2vJoyResetTimer] %1 -> Start Mouse2vJoyRecenterTimer(%2), MouseIndex = %3").arg(mouse2joy_keystr).arg(recenter_timeout).arg(mouse_index);
        qDebug().noquote().nospace() << debugmessage;
#endif
    }
}

void QKeyMapper_Worker::stopMouse2vJoyResetTimer(const QString &mouse2joy_keystr, int mouse_index_param)
{
    Q_UNUSED(mouse_index_param);
    Q_UNUSED(mouse2joy_keystr);

    QList<int> mouse_index_list = s_Mouse2vJoy_EnableStateMap.keys();
    for (const int& mouse_index : std::as_const(mouse_index_list)) {
        m_Mouse2vJoyResetTimerMap.value(mouse_index)->stop();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[stopMouse2vJoyResetTimer]" << mouse2joy_keystr << "-> Stop Mouse2vJoyResetTimer, MouseIndex =" << mouse_index;
#endif
    }
}

#if 0
QKeyMapper_Worker::Joy2MouseStates QKeyMapper_Worker::checkJoystick2MouseEnableState()
{
    Joy2MouseStates joy2mouse_enablestate = JOY2MOUSE_NONE;
    bool leftJoy2Mouse = false;
    bool rightJoy2Mouse = false;

    int findLS2Mouseindex = QKeyMapper::findOriKeyInKeyMappingDataList(JOY_LS2MOUSE_STR);
    if (findLS2Mouseindex >= 0){
        leftJoy2Mouse = true;
    }

    int findRS2Mouseindex = QKeyMapper::findOriKeyInKeyMappingDataList(JOY_RS2MOUSE_STR);
    if (findRS2Mouseindex >= 0){
        rightJoy2Mouse = true;
    }

    if (leftJoy2Mouse && rightJoy2Mouse) {
        joy2mouse_enablestate = JOY2MOUSE_BOTH;
    }
    else if (leftJoy2Mouse) {
        joy2mouse_enablestate = JOY2MOUSE_LEFT;
    }
    else if (rightJoy2Mouse) {
        joy2mouse_enablestate = JOY2MOUSE_RIGHT;
    }

    return joy2mouse_enablestate;
}
#endif

QHash<int, QKeyMapper_Worker::Joy2MouseStates> QKeyMapper_Worker::checkJoy2MouseEnableStateMap()
{
    QHash<int, Joy2MouseStates> Joy2Mouse_EnableStateMap;

    static QRegularExpression joy2mouse_regex(R"(^(Joy-(LS|RS)2Mouse)(?:@([0-9]))?$)");
    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList)) {
        QRegularExpressionMatch joy2mouse_match = joy2mouse_regex.match(keymapdata.Original_Key);

        if (joy2mouse_match.hasMatch()) {
            QString originalkey_withoutindex = joy2mouse_match.captured(1);
            QString controlType = joy2mouse_match.captured(2);
            QString playerIndexStr = joy2mouse_match.captured(3);
            int playerIndex = playerIndexStr.isEmpty() ? INITIAL_PLAYER_INDEX : playerIndexStr.toInt();

            Joy2MouseStates &joy2mouse_enablestate = Joy2Mouse_EnableStateMap[playerIndex];

            if (controlType == "LS") {
                joy2mouse_enablestate |= JOY2MOUSE_LEFT;
            }
            else if (controlType == "RS") {
                joy2mouse_enablestate |= JOY2MOUSE_RIGHT;
            }

            QString mappingkey = keymapdata.Mapping_Keys.constFirst();
            Q_UNUSED(originalkey_withoutindex);
#ifdef DEBUG_LOGOUT_ON
            if (originalkey_withoutindex != mappingkey) {
                qDebug() << "[checkJoy2MouseEnableStateMap]" << "OriginalKey and MappingKey unmatched! ->" << "OriKey: " << originalkey_withoutindex << "MapKey: " << mappingkey;
            }
#endif
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[checkJoy2MouseEnableStateMap]" << "Joy2Mouse_EnableStateMap ->" << Joy2Mouse_EnableStateMap;
#endif

    return Joy2Mouse_EnableStateMap;
}

bool QKeyMapper_Worker::checkKey2MouseEnableState()
{
    bool key2mouse_enablestate = false;
    bool key2mouse_up = false;
    bool key2mouse_down = false;
    bool key2mouse_left = false;
    bool key2mouse_right = false;

    int findKey2Mouse_index = -1;
    findKey2Mouse_index = QKeyMapper::findMapKeyInKeyMappingDataList(KEY2MOUSE_UP_STR);
    if (findKey2Mouse_index >= 0){
        key2mouse_up = true;
    }

    findKey2Mouse_index = QKeyMapper::findMapKeyInKeyMappingDataList(KEY2MOUSE_DOWN_STR);
    if (findKey2Mouse_index >= 0){
        key2mouse_down = true;
    }

    findKey2Mouse_index = QKeyMapper::findMapKeyInKeyMappingDataList(KEY2MOUSE_LEFT_STR);
    if (findKey2Mouse_index >= 0){
        key2mouse_left = true;
    }

    findKey2Mouse_index = QKeyMapper::findMapKeyInKeyMappingDataList(KEY2MOUSE_RIGHT_STR);
    if (findKey2Mouse_index >= 0){
        key2mouse_right = true;
    }

    if (key2mouse_up || key2mouse_down || key2mouse_left || key2mouse_right) {
        key2mouse_enablestate = true;
    }

    return key2mouse_enablestate;
}

bool QKeyMapper_Worker::checkGyro2MouseEnableState()
{
    bool gyro2mouse_enablestate = false;

    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList)) {
        if (keymapdata.Original_Key.contains(JOY_GYRO2MOUSE_STR)) {
            gyro2mouse_enablestate = true;
            break;
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[checkGyro2MouseEnableState]" << "JoyGyro2Mouse_EnableState ->" << gyro2mouse_enablestate;
#endif

    return gyro2mouse_enablestate;
}

bool QKeyMapper_Worker::checkGyro2MouseMoveActiveState()
{
    bool gyro2mouse_moveactivestate = true;

    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList)) {
        if (keymapdata.Mapping_Keys.constFirst().contains(GYRO2MOUSE_MOVE_KEY_STR)
            || keymapdata.MappingKeys_KeyUp.constFirst().contains(GYRO2MOUSE_MOVE_KEY_STR)) {
            gyro2mouse_moveactivestate = false;
            break;
        }
        else if (keymapdata.Mapping_Keys.constFirst().contains(GYRO2MOUSE_HOLD_KEY_STR)
            || keymapdata.MappingKeys_KeyUp.constFirst().contains(GYRO2MOUSE_HOLD_KEY_STR)) {
            gyro2mouse_moveactivestate = true;
            break;
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[checkGyro2MouseMoveActiveState]" << "JoyGyro2Mouse_MoveActiveState ->" << gyro2mouse_moveactivestate;
#endif

    return gyro2mouse_moveactivestate;
}

void QKeyMapper_Worker::doFunctionMappingProc(const QString &func_keystring)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[doFunctionMappingProc]" << "Function KeyString ->" << func_keystring;
#endif

    if (func_keystring == FUNC_REFRESH) {
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    }
    else if (func_keystring == FUNC_LOCKSCREEN) {
        if( !LockWorkStation() ) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "LockWorkStation Failed with ->" << GetLastError();
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "LockWorkStation Success.";
#endif
        }
    }
    else if (func_keystring == FUNC_SHUTDOWN) {
        bool adjust_priv;
        adjust_priv = EnablePrivilege(SE_SHUTDOWN_NAME);
        if (adjust_priv) {
            if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCEIFHUNG, SHTDN_REASON_FLAG_PLANNED)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[doFunctionMappingProc]" << "System Shutdown Failed with ->" << GetLastError();
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[doFunctionMappingProc]" << "System Shutdown Success.";
#endif
            }
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "System Shutdown EnablePrivilege Failed with ->" << GetLastError();
#endif
        }
        adjust_priv = DisablePrivilege(SE_SHUTDOWN_NAME);

        if (!adjust_priv) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "System Shutdown DisablePrivilege Failed with ->" << GetLastError();
#endif
        }
    }
    else if (func_keystring == FUNC_REBOOT) {
        bool adjust_priv;
        adjust_priv = EnablePrivilege(SE_SHUTDOWN_NAME);
        if (adjust_priv) {
            if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG, SHTDN_REASON_FLAG_PLANNED)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[doFunctionMappingProc]" << "System Reboot Failed with ->" << GetLastError();
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[doFunctionMappingProc]" << "System Reboot Success.";
#endif
            }
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "System Reboot EnablePrivilege Failed with ->" << GetLastError();
#endif
        }
        adjust_priv = DisablePrivilege(SE_SHUTDOWN_NAME);

        if (!adjust_priv) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "System Reboot DisablePrivilege Failed with ->" << GetLastError();
#endif
        }
    }
    else if (func_keystring == FUNC_LOGOFF) {
        if (!ExitWindowsEx(EWX_LOGOFF | EWX_FORCEIFHUNG, SHTDN_REASON_FLAG_PLANNED)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "System Logoff Failed with ->" << GetLastError();
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "System Logoff Success.";
#endif
        }
    }
    else if (func_keystring == FUNC_SLEEP) {
        bool adjust_priv;
        adjust_priv = EnablePrivilege(SE_SHUTDOWN_NAME);
        if (adjust_priv) {
            if (!SetSuspendState(FALSE, FALSE, FALSE)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[doFunctionMappingProc]" << "System Sleep Failed with ->" << GetLastError();
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[doFunctionMappingProc]" << "System Sleep Success.";
#endif
            }
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "System Sleep EnablePrivilege Failed with ->" << GetLastError();
#endif
        }
        adjust_priv = DisablePrivilege(SE_SHUTDOWN_NAME);

        if (!adjust_priv) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "System Sleep DisablePrivilege Failed with ->" << GetLastError();
#endif
        }
    }
    else if (func_keystring == FUNC_HIBERNATE) {
        bool adjust_priv;
        adjust_priv = EnablePrivilege(SE_SHUTDOWN_NAME);
        if (adjust_priv) {
            if (!SetSuspendState(TRUE, FALSE, FALSE)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[doFunctionMappingProc]" << "System Sleep Failed with ->" << GetLastError();
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[doFunctionMappingProc]" << "System Sleep Success.";
#endif
            }
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "System Sleep EnablePrivilege Failed with ->" << GetLastError();
#endif
        }
        adjust_priv = DisablePrivilege(SE_SHUTDOWN_NAME);

        if (!adjust_priv) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[doFunctionMappingProc]" << "System Sleep DisablePrivilege Failed with ->" << GetLastError();
#endif
        }
    }
}

void QKeyMapper_Worker::joystickLTRTButtonProc(const QJoystickAxisEvent &e)
{
    int keyupdown = KEY_INIT;
    QString keycodeString_withoutIndex;
    QString keycodeString;
    /* LT Button & RT Button */
    if (JOYSTICK_AXIS_LT_BUTTON == e.axis) {
        keycodeString_withoutIndex = "Joy-Key11(LT)";
        int player_index = e.joystick->playerindex;
        if (JOYSTICK_PLAYER_INDEX_MIN <= player_index && player_index <= JOYSTICK_PLAYER_INDEX_MAX) {
            keycodeString = QString("%1@%2").arg(keycodeString_withoutIndex).arg(player_index);
        }
        else {
            keycodeString = keycodeString_withoutIndex;
        }
        if (pressedRealKeysList.contains(keycodeString)) {
            /* LT Button is already Pressed */
            if (e.value <= JOYSTICK_AXIS_LT_RT_KEYUP_THRESHOLD) {
                keyupdown = KEY_UP;
            }
        }
        else {
            /* LT Button has been Released */
            if (e.value >= JOYSTICK_AXIS_LT_RT_KEYDOWN_THRESHOLD) {
                keyupdown = KEY_DOWN;
            }
        }
    }
    else { /* JOYSTICK_AXIS_RT_BUTTON == e.axis */
        keycodeString_withoutIndex = "Joy-Key12(RT)";
        int player_index = e.joystick->playerindex;
        if (JOYSTICK_PLAYER_INDEX_MIN <= player_index && player_index <= JOYSTICK_PLAYER_INDEX_MAX) {
            keycodeString = QString("Joy-Key12(RT)@%1").arg(player_index);
        }
        else {
            keycodeString = keycodeString_withoutIndex;
        }
        if (pressedRealKeysList.contains(keycodeString)) {
            /* RT Button is already Pressed */
            if (e.value <= JOYSTICK_AXIS_LT_RT_KEYUP_THRESHOLD) {
                keyupdown = KEY_UP;
            }
        }
        else {
            /* RT Button has been Released */
            if (e.value >= JOYSTICK_AXIS_LT_RT_KEYDOWN_THRESHOLD) {
                keyupdown = KEY_DOWN;
            }
        }
    }

    if (KEY_UP == keyupdown || KEY_DOWN == keyupdown) {
        bool returnFlag;
        returnFlag = JoyStickKeysProc(keycodeString_withoutIndex, keyupdown, e.joystick);
        Q_UNUSED(returnFlag);
    }
}

void QKeyMapper_Worker::joystickLSHorizontalProc(const QJoystickAxisEvent &e)
{
    if (e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD
        || e.value >= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD
        || (JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MIN_THRESHOLD <= e.value
            && e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MAX_THRESHOLD)) {
        /* range to process */
    }
    else {
        return;
    }

    /* Left-Stick Horizontal Process */
    int keyupdown = KEY_INIT;
    QString keycodeString;
    QString keycodeString_LS_Left;
    QString keycodeString_LS_Right;
    int player_index = e.joystick->playerindex;
    QString keycodeString_LS_Left_withoutIndex = m_JoystickLStickMap.value(JOYSTICK_LS_LEFT);
    QString keycodeString_LS_Right_withoutIndex = m_JoystickLStickMap.value(JOYSTICK_LS_RIGHT);

    // Use player_index to merge with keycodeString_LS_Left & keycodeString_LS_Right
    if (JOYSTICK_PLAYER_INDEX_MIN <= player_index && player_index <= JOYSTICK_PLAYER_INDEX_MAX) {
        keycodeString_LS_Left = QString("%1@%2").arg(keycodeString_LS_Left_withoutIndex).arg(player_index);
        keycodeString_LS_Right = QString("%1@%2").arg(keycodeString_LS_Right_withoutIndex).arg(player_index);
    }
    else {
        keycodeString_LS_Left = keycodeString_LS_Left_withoutIndex;
        keycodeString_LS_Right = keycodeString_LS_Right_withoutIndex;
    }

    bool ls_Left_Pressed = false;
    bool ls_Right_Pressed = false;
    bool returnFlag;
    if (pressedRealKeysList.contains(keycodeString_LS_Left)) {
        ls_Left_Pressed = true;
    }
    if (pressedRealKeysList.contains(keycodeString_LS_Right)) {
        ls_Right_Pressed = true;
    }

    if (ls_Left_Pressed || ls_Right_Pressed) {
        /* Left-Stick Horizontal Left or Right changed to Release */
        if (JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MIN_THRESHOLD <= e.value
            && e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MAX_THRESHOLD) {
            keyupdown = KEY_UP;
        }
        /* Left-Stick Horizontal Left changed to Right */
        else if (ls_Left_Pressed && e.value >= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD) {
            /* Need to send Left-Stick Horizontal Left Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_LS_Left_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
            /* Need to send Left-Stick Horizontal Left Release first <<< */
            keycodeString = keycodeString_LS_Right_withoutIndex;
            keyupdown = KEY_DOWN;
        }
        /* Left-Stick Horizontal Right changed to Left */
        else if (ls_Right_Pressed && e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD) {
            /* Need to send Left-Stick Horizontal Right Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_LS_Right_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
            /* Need to send Left-Stick Horizontal Right Release first <<< */
            keycodeString = keycodeString_LS_Left_withoutIndex;
            keyupdown = KEY_DOWN;
        }
    }
    else {
        /* Left-Stick Horizontal Release change to Right  */
        if (e.value >= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD) {
            keycodeString = keycodeString_LS_Right_withoutIndex;
            keyupdown = KEY_DOWN;
        }
        /* Left-Stick Horizontal Release change to Left  */
        else if (e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD) {
            keycodeString = keycodeString_LS_Left_withoutIndex;
            keyupdown = KEY_DOWN;
        }
    }

    if (KEY_DOWN == keyupdown) {
        returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick);
        Q_UNUSED(returnFlag);
    }
    else if (KEY_UP == keyupdown){
        if (ls_Left_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_LS_Left_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
        }
        if (ls_Right_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_LS_Right_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
        }
    }
    else {
        /* Stick State not changed */
    }
}

void QKeyMapper_Worker::joystickLSVerticalProc(const QJoystickAxisEvent &e)
{
    if (e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD
        || e.value >= JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD
        || (JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MIN_THRESHOLD <= e.value
         && e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MAX_THRESHOLD)) {
        /* range to process */
    }
    else {
        return;
    }

    /* Left-Stick Vertical Process */
    int keyupdown = KEY_INIT;
    QString keycodeString;
    QString keycodeString_LS_Up;
    QString keycodeString_LS_Down;
    int player_index = e.joystick->playerindex;
    QString keycodeString_LS_Up_withoutIndex = m_JoystickLStickMap.value(JOYSTICK_LS_UP);
    QString keycodeString_LS_Down_withoutIndex = m_JoystickLStickMap.value(JOYSTICK_LS_DOWN);

    // Use player_index to merge with keycodeString_LS_Up & keycodeString_LS_Down
    if (JOYSTICK_PLAYER_INDEX_MIN <= player_index && player_index <= JOYSTICK_PLAYER_INDEX_MAX) {
        keycodeString_LS_Up = QString("%1@%2").arg(keycodeString_LS_Up_withoutIndex).arg(player_index);
        keycodeString_LS_Down = QString("%1@%2").arg(keycodeString_LS_Down_withoutIndex).arg(player_index);
    }
    else {
        keycodeString_LS_Up = keycodeString_LS_Up_withoutIndex;
        keycodeString_LS_Down = keycodeString_LS_Down_withoutIndex;
    }

    bool ls_Up_Pressed = false;
    bool ls_Down_Pressed = false;
    bool returnFlag;
    if (pressedRealKeysList.contains(keycodeString_LS_Up)) {
        ls_Up_Pressed = true;
    }
    if (pressedRealKeysList.contains(keycodeString_LS_Down)) {
        ls_Down_Pressed = true;
    }

    if (ls_Up_Pressed || ls_Down_Pressed) {
        /* Left-Stick Vertical Up or Down changed to Release */
        if (JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MIN_THRESHOLD <= e.value
            && e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MAX_THRESHOLD) {
            keyupdown = KEY_UP;
        }
        /* Left-Stick Vertical Up changed to Down */
        else if (ls_Up_Pressed && e.value >= JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD) {
            /* Need to send Left-Stick Vertical Up Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_LS_Up_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
            /* Need to send Left-Stick Vertical Up Release first <<< */
            keycodeString = keycodeString_LS_Down_withoutIndex;
            keyupdown = KEY_DOWN;
        }
        /* Left-Stick Vertical Down changed to Up */
        else if (ls_Down_Pressed && e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD) {
            /* Need to send Left-Stick Vertical Down Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_LS_Down_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
            /* Need to send Left-Stick Vertical Down Release first <<< */
            keycodeString = keycodeString_LS_Up_withoutIndex;
            keyupdown = KEY_DOWN;
        }
    }
    else {
        /* Left-Stick Vertical Release change to Down  */
        if (e.value >= JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD) {
            keycodeString = keycodeString_LS_Down_withoutIndex;
            keyupdown = KEY_DOWN;
        }
        /* Left-Stick Vertical Release change to Up  */
        else if (e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD) {
            keycodeString = keycodeString_LS_Up_withoutIndex;
            keyupdown = KEY_DOWN;
        }
    }

    if (KEY_DOWN == keyupdown) {
        returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick);
        Q_UNUSED(returnFlag);
    }
    else if (KEY_UP == keyupdown){
        if (ls_Up_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_LS_Up_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
        }
        if (ls_Down_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_LS_Down_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
        }
    }
    else {
        /* Stick State not changed */
    }
}

void QKeyMapper_Worker::joystickRSHorizontalProc(const QJoystickAxisEvent &e)
{
    if (e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD
        || e.value >= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD
        || (JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MIN_THRESHOLD <= e.value
            && e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MAX_THRESHOLD)) {
        /* range to process */
    }
    else {
        return;
    }

    /* Right-Stick Horizontal Process */
    int keyupdown = KEY_INIT;
    QString keycodeString;
    QString keycodeString_RS_Left;
    QString keycodeString_RS_Right;
    int player_index = e.joystick->playerindex;
    QString keycodeString_RS_Left_withoutIndex = m_JoystickRStickMap.value(JOYSTICK_RS_LEFT);
    QString keycodeString_RS_Right_withoutIndex = m_JoystickRStickMap.value(JOYSTICK_RS_RIGHT);

    // Use player_index to merge with keycodeString_RS_Left & keycodeString_RS_Right
    if (JOYSTICK_PLAYER_INDEX_MIN <= player_index && player_index <= JOYSTICK_PLAYER_INDEX_MAX) {
        keycodeString_RS_Left = QString("%1@%2").arg(keycodeString_RS_Left_withoutIndex).arg(player_index);
        keycodeString_RS_Right = QString("%1@%2").arg(keycodeString_RS_Right_withoutIndex).arg(player_index);
    }
    else {
        keycodeString_RS_Left = keycodeString_RS_Left_withoutIndex;
        keycodeString_RS_Right = keycodeString_RS_Right_withoutIndex;
    }

    bool rs_Left_Pressed = false;
    bool rs_Right_Pressed = false;
    bool returnFlag;
    if (pressedRealKeysList.contains(keycodeString_RS_Left)) {
        rs_Left_Pressed = true;
    }
    if (pressedRealKeysList.contains(keycodeString_RS_Right)) {
        rs_Right_Pressed = true;
    }

    if (rs_Left_Pressed || rs_Right_Pressed) {
        /* Right-Stick Horizontal Left or Right changed to Release */
        if (JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MIN_THRESHOLD <= e.value
            && e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MAX_THRESHOLD) {
            keyupdown = KEY_UP;
        }
        /* Right-Stick Horizontal Left changed to Right */
        else if (rs_Left_Pressed && e.value >= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD) {
            /* Need to send Right-Stick Horizontal Left Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_RS_Left_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
            /* Need to send Right-Stick Horizontal Left Release first <<< */
            keycodeString = keycodeString_RS_Right_withoutIndex;
            keyupdown = KEY_DOWN;
        }
        /* Right-Stick Horizontal Right changed to Left */
        else if (rs_Right_Pressed && e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD) {
            /* Need to send Right-Stick Horizontal Right Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_RS_Right_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
            /* Need to send Right-Stick Horizontal Right Release first <<< */
            keycodeString = keycodeString_RS_Left_withoutIndex;
            keyupdown = KEY_DOWN;
        }
    }
    else {
        /* Right-Stick Horizontal Release change to Right  */
        if (e.value >= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD) {
            keycodeString = keycodeString_RS_Right_withoutIndex;
            keyupdown = KEY_DOWN;
        }
        /* Right-Stick Horizontal Release change to Left  */
        else if (e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD) {
            keycodeString = keycodeString_RS_Left_withoutIndex;
            keyupdown = KEY_DOWN;
        }
    }

    if (KEY_DOWN == keyupdown) {
        returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick);
        Q_UNUSED(returnFlag);
    }
    else if (KEY_UP == keyupdown){
        if (rs_Left_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_RS_Left_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
        }
        if (rs_Right_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_RS_Right_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
        }
    }
    else {
        /* Stick State not changed */
    }
}

void QKeyMapper_Worker::joystickRSVerticalProc(const QJoystickAxisEvent &e)
{
    if (e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD
        || e.value >= JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD
        || (JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MIN_THRESHOLD <= e.value
            && e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MAX_THRESHOLD)) {
        /* range to process */
    }
    else {
        return;
    }

    /* Right-Stick Vertical Process */
    int keyupdown = KEY_INIT;
    QString keycodeString;
    QString keycodeString_RS_Up;
    QString keycodeString_RS_Down;
    int player_index = e.joystick->playerindex;
    QString keycodeString_RS_Up_withoutIndex = m_JoystickRStickMap.value(JOYSTICK_RS_UP);
    QString keycodeString_RS_Down_withoutIndex = m_JoystickRStickMap.value(JOYSTICK_RS_DOWN);

    // Use player_index to merge with keycodeString_RS_Up & keycodeString_RS_Down
    if (JOYSTICK_PLAYER_INDEX_MIN <= player_index && player_index <= JOYSTICK_PLAYER_INDEX_MAX) {
        keycodeString_RS_Up = QString("%1@%2").arg(keycodeString_RS_Up_withoutIndex).arg(player_index);
        keycodeString_RS_Down = QString("%1@%2").arg(keycodeString_RS_Down_withoutIndex).arg(player_index);
    }
    else {
        keycodeString_RS_Up = keycodeString_RS_Up_withoutIndex;
        keycodeString_RS_Down = keycodeString_RS_Down_withoutIndex;
    }

    bool rs_Up_Pressed = false;
    bool rs_Down_Pressed = false;
    bool returnFlag;
    if (pressedRealKeysList.contains(keycodeString_RS_Up)) {
        rs_Up_Pressed = true;
    }
    if (pressedRealKeysList.contains(keycodeString_RS_Down)) {
        rs_Down_Pressed = true;
    }

    if (rs_Up_Pressed || rs_Down_Pressed) {
        /* Right-Stick Vertical Up or Down changed to Release */
        if (JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MIN_THRESHOLD <= e.value
            && e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MAX_THRESHOLD) {
            keyupdown = KEY_UP;
        }
        /* Right-Stick Vertical Up changed to Down */
        else if (rs_Up_Pressed && e.value >= JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD) {
            /* Need to send Right-Stick Vertical Up Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_RS_Up_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
            /* Need to send Right-Stick Vertical Up Release first <<< */
            keycodeString = keycodeString_RS_Down_withoutIndex;
            keyupdown = KEY_DOWN;
        }
        /* Right-Stick Vertical Down changed to Up */
        else if (rs_Down_Pressed && e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD) {
            /* Need to send Right-Stick Vertical Down Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_RS_Down_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
            /* Need to send Right-Stick Vertical Down Release first <<< */
            keycodeString = keycodeString_RS_Up_withoutIndex;
            keyupdown = KEY_DOWN;
        }
    }
    else {
        /* Right-Stick Vertical Release change to Down  */
        if (e.value >= JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD) {
            keycodeString = keycodeString_RS_Down_withoutIndex;
            keyupdown = KEY_DOWN;
        }
        /* Right-Stick Vertical Release change to Up  */
        else if (e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD) {
            keycodeString = keycodeString_RS_Up_withoutIndex;
            keyupdown = KEY_DOWN;
        }
    }

    if (KEY_DOWN == keyupdown) {
        returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick);
        Q_UNUSED(returnFlag);
    }
    else if (KEY_UP == keyupdown){
        if (rs_Up_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_RS_Up_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
        }
        if (rs_Down_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_RS_Down_withoutIndex, KEY_UP, e.joystick);
            Q_UNUSED(returnFlag);
        }
    }
    else {
        /* Stick State not changed */
    }
}

int QKeyMapper_Worker::joystickCalculateDelta(qreal axis_value, int Speed_Factor, bool checkJoystick)
{
    int delta = 0;
    if (checkJoystick) {
        if (axis_value > JOYSTICK2MOUSE_AXIS_PLUS_LOW_THRESHOLD) {
            if (axis_value >= 1.0) {
                delta += Speed_Factor;
            }
            else {
                qreal range = 1.0 - JOYSTICK2MOUSE_AXIS_PLUS_LOW_THRESHOLD;
                qreal step = range / Speed_Factor;
                for (int i = 1; i <= Speed_Factor; i++) {
                    if (axis_value <= JOYSTICK2MOUSE_AXIS_PLUS_LOW_THRESHOLD + i * step) {
                        delta += i;
                        break;
                    }
                }
            }
        }
        else if (axis_value < JOYSTICK2MOUSE_AXIS_MINUS_LOW_THRESHOLD) {
            if (axis_value <= -1.0) {
                delta -= Speed_Factor;
            }
            else {
                qreal range = 1.0 + JOYSTICK2MOUSE_AXIS_MINUS_LOW_THRESHOLD;
                qreal step = range / Speed_Factor;
                for (int i = 1; i <= Speed_Factor; i++) {
                    if (axis_value >= JOYSTICK2MOUSE_AXIS_MINUS_LOW_THRESHOLD - i * step) {
                        delta -= i;
                        break;
                    }
                }
            }
        }
    }
    return delta;
}

void QKeyMapper_Worker::joystick2MouseMoveProc(int player_index)
{
    Joystick_AxisState axis_state = s_JoyAxisStateMap.value(player_index);
    if (axis_state.isvirtual) {
        return;
    }

    int delta_x = 0;
    int delta_y = 0;
    bool checkLeftJoystick = false;
    bool checkRightJoystick = false;
    int Speed_Factor_X = QKeyMapper::getJoystick2MouseSpeedX();
    int Speed_Factor_Y = QKeyMapper::getJoystick2MouseSpeedY();
    Joy2MouseStates Joy2Mouse_EnableState = s_Joy2Mouse_EnableStateMap.value(player_index);

    if (JOY2MOUSE_LEFT == Joy2Mouse_EnableState) {
        checkLeftJoystick = true;
    }
    else if (JOY2MOUSE_RIGHT == Joy2Mouse_EnableState) {
        checkRightJoystick = true;
    }
    else if (JOY2MOUSE_BOTH == Joy2Mouse_EnableState) {
        checkLeftJoystick = true;
        checkRightJoystick = true;
    }

    delta_x = joystickCalculateDelta(axis_state.left_x, Speed_Factor_X, checkLeftJoystick) + joystickCalculateDelta(axis_state.right_x, Speed_Factor_X, checkRightJoystick);
    delta_y = joystickCalculateDelta(axis_state.left_y, Speed_Factor_Y, checkLeftJoystick) + joystickCalculateDelta(axis_state.right_y, Speed_Factor_Y, checkRightJoystick);

    if (delta_x != 0 || delta_y != 0) {
#ifdef JOYSTICK_VERBOSE_LOG
        qDebug().nospace().noquote() << "[joystick2MouseMoveProc]" << "JoyAxis -> Left-X = " << axis_state.left_x << ", Left-Y = " << axis_state.left_y << ", Right-X = " << axis_state.right_x << ", Right-Y = " << axis_state.right_y \
                                     << ", delta_x = " << delta_x << ", delta_y = " << delta_y;
#endif
        sendMouseMove(delta_x, delta_y);

        if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
            for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                postMouseMove(hwnd, delta_x, delta_y);
            }
#ifdef MOUSE_VERBOSE_LOG
            qDebug().nospace().noquote() << "[joystick2MouseMoveProc] postMouseMove(" << delta_x << ", " << delta_y <<") -> " << QKeyMapper::s_last_HWNDList;
#endif
        }
    }
}

void QKeyMapper_Worker::key2MouseMoveProc()
{
    int delta_x = 0;
    int delta_y = 0;
    int Speed_Factor_X = QKeyMapper::getJoystick2MouseSpeedX();
    int Speed_Factor_Y = QKeyMapper::getJoystick2MouseSpeedY();
    int final_x = 0;
    int final_y = 0;
    bool key2mouse_up = s_Key2Mouse_Up;
    bool key2mouse_down = s_Key2Mouse_Down;
    bool key2mouse_left = s_Key2Mouse_Left;
    bool key2mouse_right = s_Key2Mouse_Right;
    QStringList key2mousePressedKeys;

    if (key2mouse_up) {
        final_y -= Speed_Factor_Y;
        key2mousePressedKeys.append(KEY2MOUSE_UP_STR);
    }
    if (key2mouse_down) {
        final_y += Speed_Factor_Y;
        key2mousePressedKeys.append(KEY2MOUSE_DOWN_STR);
    }
    if (key2mouse_left) {
        final_x -= Speed_Factor_X;
        key2mousePressedKeys.append(KEY2MOUSE_LEFT_STR);
    }
    if (key2mouse_right) {
        final_x += Speed_Factor_X;
        key2mousePressedKeys.append(KEY2MOUSE_RIGHT_STR);
    }
    Q_UNUSED(key2mousePressedKeys);

    delta_x += final_x;
    delta_y += final_y;

    if (delta_x != 0 || delta_y != 0) {
#ifdef MOUSE_VERBOSE_LOG
        qDebug().nospace().noquote() << "[key2MouseMoveProc]"   << "Key2MousePressedKeys = " << key2mousePressedKeys \
                                                                << ", Speed_Factor_X = " << Speed_Factor_X << ", Speed_Factor_Y = " << Speed_Factor_Y \
                                                                << ", final_x = " << final_x << ", final_y = " << final_y \
                                                                << "-> delta_x = " << delta_x << ", delta_y = " << delta_y;
#endif
        sendMouseMove(delta_x, delta_y);

        if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
            for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                postMouseMove(hwnd, delta_x, delta_y);
            }
#ifdef MOUSE_VERBOSE_LOG
            qDebug().nospace().noquote() << "[key2MouseMoveProc] postMouseMove(" << delta_x << ", " << delta_y <<") -> " << QKeyMapper::s_last_HWNDList;
#endif
        }
    }
}

void QKeyMapper_Worker::gyro2MouseMoveProc(const GameControllerSensorData &sensor_data)
{
    static uint64_t lastTimestamp = 0;
    uint64_t currentTimestamp = sensor_data.timestamp;

    /* Skip invalid timestamp sensor data */
    if (currentTimestamp == 0) {
        return;
    }

    float deltaTime = 0.0f;
    if (lastTimestamp != 0) {
        // Convert microseconds to seconds
        deltaTime = (currentTimestamp - lastTimestamp) / 1000000.0f;
    }

    if (currentTimestamp != lastTimestamp) {
        lastTimestamp = currentTimestamp;
    }

    m_GamdpadMotion.ProcessMotion(
        sensor_data.gyroX, sensor_data.gyroY, sensor_data.gyroZ,
        sensor_data.accelX, sensor_data.accelY, sensor_data.accelZ,
        deltaTime
    );

    float inGyroX, inGyroY, inGyroZ;
    m_GamdpadMotion.GetCalibratedGyro(inGyroX, inGyroY, inGyroZ);

    if (false == s_Gyro2Mouse_MoveActive) {
        return;
    }

    int mouse_x_source = QGyro2MouseOptionDialog::getGyro2Mouse_MouseXSource();
    int mouse_y_source = QGyro2MouseOptionDialog::getGyro2Mouse_MouseYSource();
    bool mouse_x_revert = QGyro2MouseOptionDialog::getGyro2Mouse_MouseXRevert();
    bool mouse_y_revert = QGyro2MouseOptionDialog::getGyro2Mouse_MouseYRevert();

    float gyroX = 0.0f, gyroY = 0.0f;

    // Handle X axis input source and inversion
    if (mouse_x_source == GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_X) {
        gyroX += mouse_x_revert ? -inGyroX : inGyroX;
    }
    else if (mouse_x_source == GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Z) {
        gyroX += mouse_x_revert ? inGyroZ : -inGyroZ;
    }
    else { // Default mouse_x_source == GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Y
        gyroX += mouse_x_revert ? inGyroY : -inGyroY;
    }

    // Handle Y axis input source and inversion
    if (mouse_y_source == GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Y) {
        gyroY += mouse_y_revert ? -inGyroY : inGyroY;
    }
    else if (mouse_y_source == GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Z) {
        gyroY += mouse_y_revert ? -inGyroZ : inGyroZ;
    }
    else { // Default mouse_y_source == GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_X
        gyroY += mouse_y_revert ? inGyroX : -inGyroX;
    }

    // gyroX += inGyroX;   //GyroAxisMask::X for GYRO-X
    // gyroX -= inGyroY;   //GyroAxisMask::Y for GYRO-X
    // gyroX -= inGyroZ;   //GyroAxisMask::Z for GYRO-X
    // gyroY -= inGyroX;   //GyroAxisMask::X for GYRO-Y
    // gyroY += inGyroY;   //GyroAxisMask::Y for GYRO-Y
    // gyroY += inGyroZ;   //GyroAxisMask::Z for GYRO-Y

    float min_x_sensitivity = static_cast<float>(QKeyMapper::getGyro2MouseMinXSensitivity());
    float min_y_sensitivity = static_cast<float>(QKeyMapper::getGyro2MouseMinYSensitivity());
    float max_x_sensitivity = static_cast<float>(QKeyMapper::getGyro2MouseMaxXSensitivity());
    float max_y_sensitivity = static_cast<float>(QKeyMapper::getGyro2MouseMaxYSensitivity());
    std::pair<float, float> lowSensXY = { min_x_sensitivity, min_y_sensitivity };
    std::pair<float, float> hiSensXY = { max_x_sensitivity, max_y_sensitivity };
    float minThreshold = static_cast<float>(QKeyMapper::getGyro2MouseMinThreshold());
    float maxThreshold = static_cast<float>(QKeyMapper::getGyro2MouseMaxThreshold());
    float magnitude = sqrt(gyroX * gyroX + gyroY * gyroY);
    magnitude -= minThreshold;
    if (magnitude < 0.0f) magnitude = 0.0f;
    float denom = maxThreshold - minThreshold;
    float newSensitivity = denom <= 0.0f ? (magnitude > 0.0f ? 1.0f : 0.0f) : (magnitude / denom);
    if (newSensitivity > 1.0f) newSensitivity = 1.0f;
    float sensX = lowSensXY.first * (1.0f - newSensitivity) + hiSensXY.first * newSensitivity;
    float sensY = lowSensXY.second * (1.0f - newSensitivity) + hiSensXY.second * newSensitivity;

    float mouseX = gyroX * sensX;
    float mouseY = gyroY * sensY;

    float gyro2mouse_x_speed = static_cast<float>(QKeyMapper::getGyro2MouseXSpeed());
    float gyro2mouse_y_speed = static_cast<float>(QKeyMapper::getGyro2MouseYSpeed());
    float moveX = mouseX * gyro2mouse_x_speed * deltaTime;
    float moveY = mouseY * gyro2mouse_y_speed * deltaTime;
    int delta_x = static_cast<int>(moveX);
    int delta_y = static_cast<int>(moveY);

    if (delta_x != 0 || delta_y != 0) {
#ifdef GAMECONTROLLER_SENSOR_VERBOSE_LOG
        qDebug().nospace()  << "[gyro2MouseMoveProc] "
                            << "Delta X ->" << delta_x << ", "
                            << "Delta Y ->" << delta_y;
#endif
        sendMouseMove(delta_x, delta_y);

        if (QKeyMapper::getSendToSameTitleWindowsStatus()) {
            for (const HWND &hwnd : std::as_const(QKeyMapper::s_last_HWNDList)) {
                postMouseMove(hwnd, delta_x, delta_y);
            }
#ifdef MOUSE_VERBOSE_LOG
            qDebug().nospace().noquote() << "[gyro2MouseMoveProc] postMouseMove(" << delta_x << ", " << delta_y <<") -> " << QKeyMapper::s_last_HWNDList;
#endif
        }
    }
}

ULONG_PTR QKeyMapper_Worker::generateUniqueRandomValue(QSet<ULONG_PTR> &existingValues)
{
    ULONG_PTR newValue;
    do {
        newValue = QRandomGenerator::global()->generate() & ~0xF;  // Mask out the lowest 4 bits (set them to zero)
    } while (existingValues.contains(newValue));
    existingValues.insert(newValue);
    return newValue;
}

void QKeyMapper_Worker::generateVirtualInputRandomValues()
{
    QSet<ULONG_PTR> generatedValues;
    generatedValues.insert(INTERCEPTION_EXTRA_INFO_BLOCKED);
    generatedValues.insert(VIRTUAL_UNICODE_CHAR);
    generatedValues.insert(VIRTUAL_CUSTOM_KEYS);
    generatedValues.insert(VIRTUAL_MOUSE_MOVE);
    generatedValues.insert(VIRTUAL_MOUSE_MOVE_BYKEYS);
    VIRTUAL_KEY_SEND = generateUniqueRandomValue(generatedValues);
    VIRTUAL_MOUSE_POINTCLICK = generateUniqueRandomValue(generatedValues);
    VIRTUAL_MOUSE_WHEEL = generateUniqueRandomValue(generatedValues);
    VIRTUAL_KEY_OVERLAY = generateUniqueRandomValue(generatedValues);
    VIRTUAL_RESEND_REALKEY = generateUniqueRandomValue(generatedValues);

    VIRTUAL_KEY_SEND_NORMAL = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_NORMAL;
    VIRTUAL_KEY_SEND_FORCE = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_FORCE;
    VIRTUAL_KEY_SEND_MODIFIERS = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_MODIFIERS;
    VIRTUAL_KEY_SEND_BURST_TIMEOUT = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_BURST_TIMEOUT;
    VIRTUAL_KEY_SEND_BURST_STOP = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_BURST_STOP;
    VIRTUAL_KEY_SEND_KEYSEQ_NORMAL = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_KEYSEQ_NORMAL;
    VIRTUAL_KEY_SEND_KEYSEQ_HOLDDOWN = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_KEYSEQ_HOLDDOWN;
    VIRTUAL_KEY_SEND_KEYSEQ_REPEAT = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT;

#ifdef DEBUG_LOGOUT_ON
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_NORMAL: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_NORMAL, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_FORCE: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_FORCE, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_MODIFIERS: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_MODIFIERS, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_BURST_TIMEOUT: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_BURST_TIMEOUT, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_BURST_STOP: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_BURST_STOP, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_KEYSEQ_NORMAL: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_KEYSEQ_NORMAL, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_KEYSEQ_HOLDDOWN: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_KEYSEQ_HOLDDOWN, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_KEYSEQ_REPEAT: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_KEYSEQ_REPEAT, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_MOUSE_POINTCLICK: 0x%08").arg((qulonglong)VIRTUAL_MOUSE_POINTCLICK, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_MOUSE_WHEEL: 0x%08").arg((qulonglong)VIRTUAL_MOUSE_WHEEL, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_OVERLAY: 0x%08").arg((qulonglong)VIRTUAL_KEY_OVERLAY, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_RESEND_REALKEY: 0x%08").arg((qulonglong)VIRTUAL_RESEND_REALKEY, 0, 16).toUpper();
#endif
}

int QKeyMapper_Worker::InterceptionKeyboardHookProc(UINT scan_code, int keyupdown, ULONG_PTR extra_info, bool ExtenedFlag_e0, bool ExtenedFlag_e1, int keyboard_index)
{
    Q_UNUSED(scan_code);
    Q_UNUSED(ExtenedFlag_e1);

    int returnFlag = INTERCEPTION_RETURN_NORMALSEND;
    ULONG_PTR extraInfo = extra_info;
    Q_UNUSED(extraInfo);
    V_KEYCODE vkeycode;
    vkeycode.KeyCode = (quint8)MapVirtualKey(scan_code, MAPVK_VSC_TO_VK);
    vkeycode.ExtenedFlag = ExtenedFlag_e0;

    /* Virtual KeyCode Convert >>> */
    if (SCANCODE_CTRL == scan_code) {
        if (ExtenedFlag_e1) {
            vkeycode.KeyCode = VK_PAUSE;
        }
        else if (ExtenedFlag_e0) {
            vkeycode.KeyCode = VK_RCONTROL;
        }
        else {
            vkeycode.KeyCode = VK_LCONTROL;
        }
    }
    else if (SCANCODE_ALT == scan_code) {
        if (ExtenedFlag_e0) {
            vkeycode.KeyCode = VK_RMENU;
        }
        else {
            vkeycode.KeyCode = VK_LMENU;
        }
    }
    else if (SCANCODE_LSHIFT == scan_code) {
        vkeycode.KeyCode = VK_LSHIFT;
    }
    else if (SCANCODE_RSHIFT == scan_code) {
        vkeycode.KeyCode = VK_RSHIFT;
        vkeycode.ExtenedFlag = true;
    }
    else if (SCANCODE_LWIN == scan_code) {
        vkeycode.KeyCode = VK_LWIN;
        vkeycode.ExtenedFlag = true;
    }
    else if (SCANCODE_RWIN == scan_code) {
        vkeycode.KeyCode = VK_RWIN;
        vkeycode.ExtenedFlag = true;
    }
    else if (SCANCODE_APPS == scan_code) {
        vkeycode.KeyCode = VK_APPS;
        vkeycode.ExtenedFlag = true;
    }
    else if (SCANCODE_DIVIDE == scan_code) {
        if (ExtenedFlag_e0) {
            vkeycode.KeyCode = VK_DIVIDE;
        }
    }
    else if (SCANCODE_NUMLOCK == scan_code) {
        vkeycode.KeyCode = VK_NUMLOCK;
        vkeycode.ExtenedFlag = true;
    }
    else if (SCANCODE_PRINTSCREEN == scan_code) {
        if (ExtenedFlag_e0) {
            vkeycode.KeyCode = VK_SNAPSHOT;
            vkeycode.ExtenedFlag = true;
        }
    }
    else if (SCANCODE_SNAPSHOT == scan_code) {
        vkeycode.KeyCode = VK_SNAPSHOT;
        vkeycode.ExtenedFlag = true;
    }
    /* Virtual KeyCode Convert <<< */

    QList<quint8>& pressedVKeyCodeList = pressedMultiKeyboardVKeyCodeList[keyboard_index];
    if (KEY_DOWN == keyupdown){
        if (pressedVKeyCodeList.contains(vkeycode.KeyCode)) {
            if (Interception_Worker::s_FilterKeys) {
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }
        }
        else {
            pressedVKeyCodeList.append(vkeycode.KeyCode);
        }
    }
    else {  /* KEY_UP == keyupdown */
        if (pressedVKeyCodeList.contains(vkeycode.KeyCode)){
            pressedVKeyCodeList.removeAll(vkeycode.KeyCode);
        }
    }

    QString keycodeString = VirtualKeyCodeMap.key(vkeycode);
    QString keycodeString_nochanged = keycodeString;

//#ifdef DEBUG_LOGOUT_ON
//    qDebug("\"%s\" (0x%02X), scanCode(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), vkcode, scan_code, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
//#endif

    if (false == keycodeString.isEmpty()){
#ifdef DEBUG_LOGOUT_ON
        qDebug("[InterceptionKeyboardHookProc] currentThread -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

#ifdef DEBUG_LOGOUT_ON
        if (KEY_DOWN == keyupdown){
            qDebug("[InterceptionKeyboardHookProc] RealKey: \"%s\" (0x%02X) KeyDown, scanCode(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X)", keycodeString.toStdString().c_str(), vkeycode.KeyCode, scan_code, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo);
        }
        else {
            qDebug("[InterceptionKeyboardHookProc] RealKey: \"%s\" (0x%02X) KeyUp, scanCode(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X)", keycodeString.toStdString().c_str(), vkeycode.KeyCode, scan_code, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo);
        }
#endif

        static bool show_screenpoints = false;
        static bool show_windowpoints = false;
        if (KEY_DOWN == keyupdown){
#ifdef DEBUG_LOGOUT_ON
            if (keycodeString == SHOW_KEY_DEBUGINFO) {
                {
                QMutexLocker locker(&s_PressedMappingKeysMapMutex);
                qDebug().nospace() << "\033[1;34m[InterceptionKeyboardHookProc]" << "Show debug info pressedMappingKeysMap -> " << QKeyMapper_Worker::pressedMappingKeysMap << "\033[0m";
                }
                qDebug().nospace() << "\033[1;34m[InterceptionKeyboardHookProc]" << "Show debug info pressedVirtualKeysList -> " << QKeyMapper_Worker::pressedVirtualKeysList << "\033[0m";
                qDebug().nospace() << "\033[1;34m[InterceptionKeyboardHookProc]" << "Show debug info pressedRealKeysList -> " << QKeyMapper_Worker::pressedRealKeysList << "\033[0m";
                qDebug().nospace() << "\033[1;34m[InterceptionKeyboardHookProc]" << "Show debug info pressedRealKeysListRemoveMultiInput -> " << QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput << "\033[0m";
            }
#endif

            QString mousePassThroughSwitchKey = QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_MousePassThroughSwitchKey;
            if ((QKeyMapper::KEYMAP_MAPPING_GLOBAL == QKeyMapper::getInstance()->m_KeyMapStatus
                || QKeyMapper::KEYMAP_MAPPING_MATCHED == QKeyMapper::getInstance()->m_KeyMapStatus)
                && (keycodeString == mousePassThroughSwitchKey)) {
                emit QKeyMapper::getInstance()->switchFloatingWindowMousePassThrough_Signal();
            }
            else if (keycodeString == SHOW_POINTS_IN_SCREEN_KEY) {
                    if (!show_screenpoints) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[InterceptionKeyboardHookProc]" << "Show Mouse Points In Screen KEY_DOWN -> ON";
#endif
                        show_screenpoints = true;
                        emit QKeyMapper::getInstance()->showMousePoints_Signal(SHOW_POINTSIN_SCREEN_ON);
                    }
            }
            else if (keycodeString == SHOW_POINTS_IN_WINDOW_KEY) {
                    if (!show_windowpoints) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[InterceptionKeyboardHookProc]" << "Show Mouse Points In Window KEY_DOWN -> ON";
#endif
                        show_windowpoints = true;
                        emit QKeyMapper::getInstance()->showMousePoints_Signal(SHOW_POINTSIN_WINDOW_ON);
                    }
            }
            else if (keycodeString == SHOW_CAR_ORDINAL_KEY) {
                    if (s_LastCarOrdinal > 0) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[InterceptionKeyboardHookProc]" << "Show CarOrdinal Key Pressed, CarOrdinal =" << s_LastCarOrdinal;
#endif
                        emit QKeyMapper::getInstance()->showCarOrdinal_Signal(s_LastCarOrdinal);
                    }
            }
        }
        else {
            if (keycodeString == SHOW_POINTS_IN_SCREEN_KEY) {
                if (show_screenpoints) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[InterceptionKeyboardHookProc]" << "Show Mouse Points In Screen KEY_UP -> OFF";
#endif
                    show_screenpoints = false;
                    emit QKeyMapper::getInstance()->showMousePoints_Signal(SHOW_POINTSIN_SCREEN_OFF);
                }
            }
            else if (keycodeString == SHOW_POINTS_IN_WINDOW_KEY) {
                if (show_windowpoints) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[InterceptionKeyboardHookProc]" << "Show Mouse Points In Window KEY_UP -> OFF";
#endif
                    show_windowpoints = false;
                    emit QKeyMapper::getInstance()->showMousePoints_Signal(SHOW_POINTSIN_WINDOW_OFF);
                }
            }
        }

        if (0 <= keyboard_index && keyboard_index < INTERCEPTION_MAX_KEYBOARD) {
            keycodeString = QString("%1@%2").arg(keycodeString, QString::number(keyboard_index));
        }

        int findindex = -1;
        if (HOOKPROC_STATE_STARTED == s_AtomicHookProcState) {
            returnFlag = (hookBurstAndLockProc(keycodeString, keyupdown) != KEY_PROC_NONE);
            findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
        }

        int intercept = updatePressedRealKeysList(keycodeString, keyupdown);
        bool mappingswitch_detected = detectMappingSwitchKey(keycodeString_nochanged, keyupdown);
        bool displayswitch_detected = detectDisplaySwitchKey(keycodeString_nochanged, keyupdown);
        if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
            if ((mappingswitch_detected || displayswitch_detected) && KEY_DOWN == keyupdown) {
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }
            else if (intercept == KEY_INTERCEPT_BLOCK_KEY_RECORD) {
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }
            else {
                return INTERCEPTION_RETURN_NORMALSEND;
            }
        }
        else {
            bool tabswitch_detected = detectMappingTableTabHotkeys(keycodeString_nochanged, keyupdown);
            if ((mappingswitch_detected || displayswitch_detected || tabswitch_detected) && KEY_DOWN == keyupdown) {
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }

            if (intercept == KEY_INTERCEPT_BLOCK) {
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }
        }

        int combinationkey_detected = detectCombinationKeys(keycodeString, keyupdown);
        if (combinationkey_detected) {
            if (KEY_DOWN == keyupdown) {
                if (KEY_INTERCEPT_PASSTHROUGH == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[InterceptionKeyboardHookProc] detectCombinationKeys (%1) KEY_DOWN return -> KEY_INTERCEPT_PASSTHROUGH").arg(keycodeString);
                    qDebug().nospace().noquote() << debugmessage;
#endif
                    return INTERCEPTION_RETURN_NORMALSEND;
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[InterceptionKeyboardHookProc] detectCombinationKeys (%1) KEY_DOWN return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                    qDebug().nospace().noquote() << debugmessage;
#endif
                    return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                }
            }
            else {
                if (findindex >= 0) {
                }
                else {
                    if (KEY_INTERCEPT_BLOCK == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                        QString debugmessage = QString("[InterceptionKeyboardHookProc] detectCombinationKeys (%1) KEY_UP return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                        qDebug().nospace().noquote() << debugmessage;
#endif
                        return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                    }
                    else {
#ifdef DEBUG_LOGOUT_ON
                        QString debugmessage = QString("[InterceptionKeyboardHookProc] detectCombinationKeys (%1) KEY_UP return -> %2").arg(keycodeString).arg(combinationkey_detected);
                        qDebug().nospace().noquote() << debugmessage;
#endif
                        return INTERCEPTION_RETURN_NORMALSEND;
                    }
                }
            }
        }

        if (KEY_UP == keyupdown && false == returnFlag){
            if (findindex >= 0
                && (QKeyMapper::KeyMappingDataList->at(findindex).Original_Key == keycodeString
                    || QKeyMapper::KeyMappingDataList->at(findindex).Original_Key == keycodeString_nochanged)) {
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[InterceptionKeyboardHookProc] \"%1\" or \"%2\" has found in OriginalKeys, do not skip RealKey \"%3\" KEY_UP").arg(keycodeString_nochanged, keycodeString, keycodeString);
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            }
            else {
                if (pressedVirtualKeysList.contains(keycodeString_nochanged)) {
                    returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_LOWLEVELHOOK;
#ifdef DEBUG_LOGOUT_ON
                    qDebug("[InterceptionKeyboardHookProc] VirtualKey \"%s\" is pressed down, skip RealKey \"%s\" KEY_UP!", keycodeString_nochanged.toStdString().c_str(), keycodeString_nochanged.toStdString().c_str());
#endif
                }
            }
        }

        if (INTERCEPTION_RETURN_NORMALSEND == returnFlag) {
            if (findindex >=0){
                QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
                QStringList mappingKey_KeyUpList = QKeyMapper::KeyMappingDataList->at(findindex).MappingKeys_KeyUp;
                QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
                QString firstmappingkey = mappingKeyList.constFirst();
                int mappingkeylist_size = mappingKeyList.size();
                if (mappingkeylist_size == 1 && firstmappingkey == KEY_BLOCKED_STR) {
#ifdef DEBUG_LOGOUT_ON
                    if (KEY_DOWN == keyupdown){
                        qDebug() << "[InterceptionKeyboardHookProc]" << "RealKey KEY_DOWN Blocked ->" << original_key;
                    }
                    else {
                        qDebug() << "[InterceptionKeyboardHookProc]" << "RealKey KEY_UP Blocked ->" << original_key;
                    }
#endif
                    returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                }
                else if (firstmappingkey.startsWith(FUNC_PREFIX) && mappingkeylist_size == 1) {
#ifdef DEBUG_LOGOUT_ON
                    if (KEY_DOWN == keyupdown){
                        qDebug() << "[InterceptionKeyboardHookProc]" << "Function KEY_DOWN ->" << firstmappingkey;
                    }
                    else {
                        qDebug() << "[InterceptionKeyboardHookProc]" << "Function KEY_UP ->" << firstmappingkey;
                    }
#endif
                    if (KEY_DOWN == keyupdown){
                        emit QKeyMapper_Worker::getInstance()->doFunctionMappingProc_Signal(firstmappingkey);
                    }

                    returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                }
                else {
                    if (firstmappingkey.startsWith(KEY2MOUSE_PREFIX) && mappingkeylist_size == 1) {
                        if (KEY_DOWN == keyupdown){
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[InterceptionKeyboardHookProc]" << "Key2Mouse Key(" << original_key << ") Down ->" << firstmappingkey;
#endif
                        }
                        else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[InterceptionKeyboardHookProc]" << "Key2Mouse Key(" << original_key << ") Up ->" << firstmappingkey;
#endif
                        }
                    }

                    int SendTiming = QKeyMapper::KeyMappingDataList->at(findindex).SendTiming;
                    bool KeySeqHoldDown = QKeyMapper::KeyMappingDataList->at(findindex).KeySeqHoldDown;
                    if (KEY_DOWN == keyupdown){
                        if (SENDTIMING_KEYDOWN == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                        }
                        else if (SENDTIMING_KEYUP == SendTiming) {
                            /* KEY_DOWN & SENDTIMING_KEYUP == SendTiming -> do nothing */
                        }
                        else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                        }
                        else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                        }
                        else { /* SENDTIMING_NORMAL == SendTiming */
                            /* Add for KeySequenceHoldDown >>> */
                            if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                            }
                            else {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            }
                            /* Add for KeySequenceHoldDown <<< */
                        }
                        returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                    }
                    else { /* KEY_UP == keyupdown */
                        if (SENDTIMING_KEYDOWN == SendTiming) {
                            /* KEY_UP & SENDTIMING_KEYDOWN == SendTiming -> do nothing */
                        }
                        else if (SENDTIMING_KEYUP == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                        }
                        else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                        }
                        else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);

                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                        }
                        else { /* SENDTIMING_NORMAL == SendTiming */
                            /* Add for KeySequenceHoldDown >>> */
                            if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                            }
                            else {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                            }
                            /* Add for KeySequenceHoldDown <<< */
                        }
                        returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                    }
                }

                bool PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                if (PassThrough && returnFlag) {
                    returnFlag = INTERCEPTION_RETURN_NORMALSEND;
#ifdef DEBUG_LOGOUT_ON
                    QString keyUpDown;
                    if (KEY_DOWN == keyupdown){
                        keyUpDown = "KEY_DOWN";
                    }
                    else {
                        keyUpDown = "KEY_UP";
                    }
                    qDebug().noquote().nospace() << "[InterceptionKeyboardHookProc]" << "PassThrough Key[" << original_key << "] " << keyUpDown;
#endif
                }
            }
        }
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug("[InterceptionKeyboardHookProc] UnknownKey (0x%02X) Input, scanCode(0x%08X), ExtenedFlag(%s)", vkeycode.KeyCode, scan_code, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
#endif
    }

    return returnFlag;
}

int QKeyMapper_Worker::InterceptionMouseHookProc(MouseEvent mouse_event, int delta_x, int delta_y, short delta_wheel, unsigned short flags, ULONG_PTR extra_info, int mouse_index)
{
    Q_UNUSED(flags);

    int returnFlag = INTERCEPTION_RETURN_NORMALSEND;
    ULONG_PTR extraInfo = extra_info;
    Q_UNUSED(extraInfo);

    if ((mouse_event == EVENT_LBUTTONDOWN || mouse_event == EVENT_LBUTTONUP)
        || (mouse_event == EVENT_RBUTTONDOWN || mouse_event == EVENT_RBUTTONUP)
        || (mouse_event == EVENT_MBUTTONDOWN || mouse_event == EVENT_MBUTTONUP)
        || (mouse_event == EVENT_X1BUTTONDOWN || mouse_event == EVENT_X1BUTTONUP)
        || (mouse_event == EVENT_X2BUTTONDOWN || mouse_event == EVENT_X2BUTTONUP)) {
        int keyupdown;
        if (mouse_event == EVENT_LBUTTONDOWN
            || mouse_event == EVENT_RBUTTONDOWN
            || mouse_event == EVENT_MBUTTONDOWN
            || mouse_event == EVENT_X1BUTTONDOWN
            || mouse_event == EVENT_X2BUTTONDOWN) {
            keyupdown = KEY_DOWN;
        }
        else {
            keyupdown = KEY_UP;
        }
        QString keycodeString;
        if ((mouse_event == EVENT_LBUTTONDOWN || mouse_event == EVENT_LBUTTONUP)) {
            keycodeString = MOUSE_L_STR;
        }
        else if ((mouse_event == EVENT_RBUTTONDOWN || mouse_event == EVENT_RBUTTONUP)) {
            keycodeString = MOUSE_R_STR;
        }
        else if ((mouse_event == EVENT_MBUTTONDOWN || mouse_event == EVENT_MBUTTONUP)) {
            keycodeString = MOUSE_M_STR;
        }
        else if ((mouse_event == EVENT_X1BUTTONDOWN || mouse_event == EVENT_X1BUTTONUP)) {
            keycodeString = MOUSE_X1_STR;
        }
        else if ((mouse_event == EVENT_X2BUTTONDOWN || mouse_event == EVENT_X2BUTTONUP)) {
            keycodeString = MOUSE_X2_STR;
        }
        else {
            keycodeString = MOUSE_L_STR;
        }
        QString keycodeString_nochanged = keycodeString;

#ifdef DEBUG_LOGOUT_ON
        qDebug("[InterceptionMouseHookProc] Real \"%s\" %s, extraInfo(0x%08X)", keycodeString.toStdString().c_str(), (keyupdown == KEY_DOWN?"Button Down":"Button Up"), extraInfo);
#endif
        if ((GetAsyncKeyState(PICK_SCREEN_POINT_KEY) & 0x8000) != 0 && mouse_event == EVENT_LBUTTONDOWN) {
            POINT pt;
            if (GetCursorPos(&pt)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[InterceptionMouseHookProc]" << "L-Ctrl + Mouse-Left Click Capture Screen MousePoint -> X =" << pt.x << ", Y=" << pt.y;
#endif
                QPoint point = QPoint(pt.x, pt.y);
                emit QKeyMapper::getInstance()->updateMousePointLabelDisplay_Signal(point);
            }
        }
        else if ((GetAsyncKeyState(PICK_WINDOW_POINT_KEY) & 0x8000) != 0 && mouse_event == EVENT_LBUTTONDOWN) {
            POINT pt;
            HWND hwnd = QKeyMapper::s_CurrentMappingHWND;
            if (hwnd != NULL && GetCursorPos(&pt)) {
                if (ScreenToClient(hwnd, &pt)) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[InterceptionMouseHookProc]" << "L-Alt + Mouse-Left Click Capture Window MousePoint -> X =" << pt.x << ", Y=" << pt.y;
#endif
                    QPoint point = QPoint(pt.x, pt.y);
                    emit QKeyMapper::getInstance()->updateMousePointLabelDisplay_Signal(point);
                }
            }
        }
//         else if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0 && mouse_event == EVENT_RBUTTONDOWN) {
// #ifdef DEBUG_LOGOUT_ON
//             qDebug() << "[InterceptionMouseHookProc]" << "L-Ctrl + Mouse-Right Click Clear MousePoint";
// #endif
//             QPoint point = QPoint(-500, -500);
//             emit QKeyMapper::getInstance()->updateMousePointLabelDisplay_Signal(point);
//         }

        if (0 <= mouse_index && mouse_index < INTERCEPTION_MAX_MOUSE) {
            keycodeString = QString("%1@%2").arg(keycodeString, QString::number(mouse_index));
        }

        int findindex = -1;
        if (HOOKPROC_STATE_STARTED == s_AtomicHookProcState) {
            returnFlag = (hookBurstAndLockProc(keycodeString, keyupdown) != KEY_PROC_NONE);
            findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
        }

        int intercept = updatePressedRealKeysList(keycodeString, keyupdown);
        bool mappingswitch_detected = detectMappingSwitchKey(keycodeString_nochanged, keyupdown);
        bool displayswitch_detected = detectDisplaySwitchKey(keycodeString_nochanged, keyupdown);
        if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
            if ((mappingswitch_detected || displayswitch_detected) && KEY_DOWN == keyupdown) {
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }
            else if (intercept == KEY_INTERCEPT_BLOCK_KEY_RECORD) {
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }
            else {
                return INTERCEPTION_RETURN_NORMALSEND;
            }
        }
        else {
            bool tabswitch_detected = detectMappingTableTabHotkeys(keycodeString_nochanged, keyupdown);
            if ((mappingswitch_detected || displayswitch_detected || tabswitch_detected) && KEY_DOWN == keyupdown) {
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }

            if (intercept == KEY_INTERCEPT_BLOCK) {
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }
        }

        int combinationkey_detected = detectCombinationKeys(keycodeString, keyupdown);
        if (combinationkey_detected) {
            if (KEY_DOWN == keyupdown) {
                if (KEY_INTERCEPT_PASSTHROUGH == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[InterceptionMouseHookProc] detectCombinationKeys (%1) KEY_DOWN return -> KEY_INTERCEPT_PASSTHROUGH").arg(keycodeString);
                    qDebug().nospace().noquote() << debugmessage;
#endif
                    return INTERCEPTION_RETURN_NORMALSEND;
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[InterceptionMouseHookProc] detectCombinationKeys (%1) KEY_DOWN return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                    qDebug().nospace().noquote() << debugmessage;
#endif
                    return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                }
            }
            else {
                if (findindex >= 0) {
// #ifdef DEBUG_LOGOUT_ON
//                     QString debugmessage = QString("[InterceptionMouseHookProc] OriginalKey \"%1\" found rowindex(%d), detectCombinationKeys KEY_UP return -> KEY_INTERCEPT_BLOCK").arg(keycodeString).arg(findindex);
//                     qDebug().nospace().noquote() << debugmessage;
// #endif
//                     return true;
                }
                else {
                    if (KEY_INTERCEPT_BLOCK == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                        QString debugmessage = QString("[InterceptionMouseHookProc] detectCombinationKeys (%1) KEY_UP return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                        qDebug().nospace().noquote() << debugmessage;
#endif
                        return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                    }
                    else {
#ifdef DEBUG_LOGOUT_ON
                        QString debugmessage = QString("[InterceptionMouseHookProc] detectCombinationKeys (%1) KEY_UP return -> %2").arg(keycodeString).arg(combinationkey_detected);
                        qDebug().nospace().noquote() << debugmessage;
#endif
                        return INTERCEPTION_RETURN_NORMALSEND;
                    }
                }
            }
        }

        if (KEY_UP == keyupdown && INTERCEPTION_RETURN_NORMALSEND == returnFlag){
            if (findindex >= 0
                && (QKeyMapper::KeyMappingDataList->at(findindex).Original_Key == keycodeString
                    || QKeyMapper::KeyMappingDataList->at(findindex).Original_Key == keycodeString_nochanged)) {
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[InterceptionMouseHookProc] \"%1\" or \"%2\" has found in OriginalKeys, do not skip RealKey \"%3\" KEY_UP").arg(keycodeString_nochanged, keycodeString, keycodeString);
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            }
            else {
                if (pressedVirtualKeysList.contains(keycodeString_nochanged)) {
                    returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_LOWLEVELHOOK;
#ifdef DEBUG_LOGOUT_ON
                    qDebug("[InterceptionMouseHookProc] Virtual \"%s\" is pressed down, skip Real \"%s\" KEY_UP!", keycodeString_nochanged.toStdString().c_str(), keycodeString_nochanged.toStdString().c_str());
#endif
                }
            }
        }

        if (INTERCEPTION_RETURN_NORMALSEND == returnFlag) {
            if (findindex >=0){
                QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
                QStringList mappingKey_KeyUpList = QKeyMapper::KeyMappingDataList->at(findindex).MappingKeys_KeyUp;
                QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
                QString firstmappingkey = mappingKeyList.constFirst();
                int mappingkeylist_size = mappingKeyList.size();
                if (mappingkeylist_size == 1 && firstmappingkey == KEY_BLOCKED_STR) {
#ifdef DEBUG_LOGOUT_ON
                    if (KEY_DOWN == keyupdown){
                        qDebug() << "[InterceptionMouseHookProc]" << "Real Mouse Button Down Blocked ->" << original_key;
                    }
                    else {
                        qDebug() << "[InterceptionMouseHookProc]" << "Real Mouse Button Up Blocked ->" << original_key;
                    }
#endif
                    returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                }
                else if (firstmappingkey.startsWith(FUNC_PREFIX) && mappingkeylist_size == 1) {
#ifdef DEBUG_LOGOUT_ON
                    if (KEY_DOWN == keyupdown){
                        qDebug() << "[InterceptionMouseHookProc]" << "Function KEY_DOWN ->" << firstmappingkey;
                    }
                    else {
                        qDebug() << "[InterceptionMouseHookProc]" << "Function KEY_UP ->" << firstmappingkey;
                    }
#endif
                    if (KEY_DOWN == keyupdown){
                        emit QKeyMapper_Worker::getInstance()->doFunctionMappingProc_Signal(firstmappingkey);
                    }

                    returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                }
                else {
                    if (firstmappingkey.startsWith(KEY2MOUSE_PREFIX) && mappingkeylist_size == 1) {
                        if (KEY_DOWN == keyupdown){
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[InterceptionMouseHookProc]" << "Key2Mouse Key(" << original_key << ") Down ->" << firstmappingkey;
#endif
                        }
                        else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[InterceptionMouseHookProc]" << "Key2Mouse Key(" << original_key << ") Up ->" << firstmappingkey;
#endif
                        }
                    }

                    int SendTiming = QKeyMapper::KeyMappingDataList->at(findindex).SendTiming;
                    bool KeySeqHoldDown = QKeyMapper::KeyMappingDataList->at(findindex).KeySeqHoldDown;
                    if (KEY_DOWN == keyupdown){
                        if (SENDTIMING_KEYDOWN == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                        }
                        else if (SENDTIMING_KEYUP == SendTiming) {
                            /* KEY_DOWN & SENDTIMING_KEYUP == SendTiming -> do nothing */
                        }
                        else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                        }
                        else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                        }
                        else { /* SENDTIMING_NORMAL == SendTiming */
                            /* Add for KeySequenceHoldDown >>> */
                            if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                            }
                            else {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            }
                            /* Add for KeySequenceHoldDown <<< */
                        }
                        returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                    }
                    else { /* KEY_UP == keyupdown */
                        if (SENDTIMING_KEYDOWN == SendTiming) {
                            /* KEY_UP & SENDTIMING_KEYDOWN == SendTiming -> do nothing */
                        }
                        else if (SENDTIMING_KEYUP == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                        }
                        else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                        }
                        else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);

                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                        }
                        else { /* SENDTIMING_NORMAL == SendTiming */
                            /* Add for KeySequenceHoldDown >>> */
                            if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                            }
                            else {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                            }
                            /* Add for KeySequenceHoldDown <<< */
                        }
                        returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                    }
                }

                bool PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                if (PassThrough && returnFlag) {
                    returnFlag = INTERCEPTION_RETURN_NORMALSEND;
#ifdef DEBUG_LOGOUT_ON
                    QString keyUpDown;
                    if (KEY_DOWN == keyupdown){
                        keyUpDown = "KEY_DOWN";
                    }
                    else {
                        keyUpDown = "KEY_UP";
                    }
                    qDebug().noquote().nospace() << "[InterceptionMouseHookProc]" << "PassThrough Key[" << original_key << "] " << keyUpDown;
#endif
                }
            }
        }
    }
    else if (mouse_event == EVENT_MOUSEWHEEL || mouse_event == EVENT_MOUSEHWHEEL) {
        short zDelta = delta_wheel;

        if (zDelta == 0) {
            return INTERCEPTION_RETURN_NORMALSEND;
        }

#ifdef DEBUG_LOGOUT_ON
        QString extraInfoStr = QString("0x%1").arg(QString::number(extraInfo, 16).toUpper(), 8, '0');
        if (zDelta > 0) {
            if (mouse_event == EVENT_MOUSEHWHEEL) {
                qDebug().noquote().nospace() << "[InterceptionMouseHookProc]" << " Real Mouse Wheel Right -> Delta =  " << zDelta << ", extraInfoStr = " << extraInfoStr;
            }
            else {
                qDebug().noquote().nospace() << "[InterceptionMouseHookProc]" << " Real Mouse Wheel Up    -> Delta =  " << zDelta << ", extraInfoStr = " << extraInfoStr;
            }
        }
        else {
            if (mouse_event == EVENT_MOUSEHWHEEL) {
                qDebug().noquote().nospace() << "[InterceptionMouseHookProc]" << " Real Mouse Wheel Left  -> Delta = " << zDelta << ", extraInfoStr = " << extraInfoStr;
            }
            else {
                qDebug().noquote().nospace() << "[InterceptionMouseHookProc]" << " Real Mouse Wheel Down  -> Delta = " << zDelta << ", extraInfoStr = " << extraInfoStr;
            }
        }
#endif
        QString keycodeString;
        int input_type;
        if (zDelta > 0) {
            if (mouse_event == EVENT_MOUSEHWHEEL) {
                keycodeString = MOUSE_WHEEL_RIGHT_STR;
                input_type = INPUT_MOUSE_WHEEL;
            }
            else {
                keycodeString = MOUSE_WHEEL_UP_STR;
                input_type = INPUT_MOUSE_WHEEL;
            }
        }
        else {
            if (mouse_event == EVENT_MOUSEHWHEEL) {
                keycodeString = MOUSE_WHEEL_LEFT_STR;
                input_type = INPUT_MOUSE_WHEEL;
            }
            else {
                keycodeString = MOUSE_WHEEL_DOWN_STR;
                input_type = INPUT_MOUSE_WHEEL;
            }
        }

        if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
            bool block = false;
            if (s_KeyRecording) {
                bool recorded = updateRecordKeyList(keycodeString, input_type);
                if (recorded) {
                    block = true;
                }
            }

            if (block) {
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }
            else {
                return INTERCEPTION_RETURN_NORMALSEND;
            }
        }

        if (0 <= mouse_index && mouse_index < INTERCEPTION_MAX_MOUSE) {
            keycodeString = QString("%1@%2").arg(keycodeString, QString::number(mouse_index));
        }

        int keyupdown = KEY_DOWN;
        (void)updatePressedRealKeysList(keycodeString, keyupdown);
        int combinationkey_detected = detectCombinationKeys(keycodeString, keyupdown);
        Q_UNUSED(combinationkey_detected);
        keyupdown = KEY_UP;
        (void)updatePressedRealKeysList(keycodeString, keyupdown);
        combinationkey_detected = detectCombinationKeys(keycodeString, keyupdown);
        if (combinationkey_detected) {
            if (KEY_INTERCEPT_PASSTHROUGH == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[InterceptionMouseHookProc] detectCombinationKeys MouseWheel (%1) return -> KEY_INTERCEPT_PASSTHROUGH").arg(keycodeString);
                qDebug().nospace().noquote() << debugmessage;
#endif
                return INTERCEPTION_RETURN_NORMALSEND;
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[InterceptionMouseHookProc] detectCombinationKeys MouseWheel (%1) return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                qDebug().nospace().noquote() << debugmessage;
#endif
                return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }
        }

        short delta_abs = qAbs(zDelta);
        if (delta_abs >= WHEEL_DELTA) {
            bool wheel_up_found = false;
            bool wheel_down_found = false;
            bool wheel_left_found = false;
            bool wheel_right_found = false;
            bool send_wheel_keys = false;

            QString keycodeString_WheelUp = MOUSE_WHEEL_UP_STR;
            QString keycodeString_WheelDown = MOUSE_WHEEL_DOWN_STR;
            QString keycodeString_WheelLeft = MOUSE_WHEEL_LEFT_STR;
            QString keycodeString_WheelRight = MOUSE_WHEEL_RIGHT_STR;
            if (0 <= mouse_index && mouse_index < INTERCEPTION_MAX_MOUSE) {
                keycodeString_WheelUp = QString("%1@%2").arg(keycodeString_WheelUp, QString::number(mouse_index));
                keycodeString_WheelDown = QString("%1@%2").arg(keycodeString_WheelDown, QString::number(mouse_index));
                keycodeString_WheelLeft = QString("%1@%2").arg(keycodeString_WheelLeft, QString::number(mouse_index));
                keycodeString_WheelRight = QString("%1@%2").arg(keycodeString_WheelRight, QString::number(mouse_index));
            }

            int findindex = -1;
            int findWheelUpindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString_WheelUp);
            if (findWheelUpindex >=0){
                wheel_up_found = true;
            }

            int findWheelDownindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString_WheelDown);
            if (findWheelDownindex >=0){
                wheel_down_found = true;
            }

            int findWheelLeftindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString_WheelLeft);
            if (findWheelLeftindex >=0){
                wheel_left_found = true;
            }

            int findWheelRightindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString_WheelRight);
            if (findWheelRightindex >=0){
                wheel_right_found = true;
            }

            if (wheel_up_found || wheel_down_found || wheel_left_found || wheel_right_found) {
                if (wheel_up_found && mouse_event == EVENT_MOUSEWHEEL && zDelta > 0) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[InterceptionMouseHookProc]" << "Real" << keycodeString_WheelUp << "-> Send Wheel Up Mapping Keys";
#endif
                    send_wheel_keys = true;
                    findindex = findWheelUpindex;
                }
                else if (wheel_down_found && mouse_event == EVENT_MOUSEWHEEL && zDelta < 0) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[InterceptionMouseHookProc]" << "Real" << keycodeString_WheelDown << "-> Send Wheel Down Mapping Keys";
#endif
                    send_wheel_keys = true;
                    findindex = findWheelDownindex;
                }
                else if (wheel_left_found && mouse_event == EVENT_MOUSEHWHEEL && zDelta < 0) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[InterceptionMouseHookProc]" << "Real" << keycodeString_WheelLeft << "-> Send Wheel Left Mapping Keys";
#endif
                    send_wheel_keys = true;
                    findindex = findWheelLeftindex;
                }
                else if (wheel_right_found && mouse_event == EVENT_MOUSEHWHEEL && zDelta > 0) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[InterceptionMouseHookProc]" << "Real" << keycodeString_WheelRight << "-> Send Wheel Right Mapping Keys";
#endif
                    send_wheel_keys = true;
                    findindex = findWheelRightindex;
                }

                if (send_wheel_keys) {
                    QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
                    QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;

                    if (mappingKeyList.size() == 1 && mappingKeyList.constFirst() == KEY_BLOCKED_STR) {
#ifdef DEBUG_LOGOUT_ON
                        if (wheel_left_found) {
                            qDebug() << "[InterceptionMouseHookProc]" << "Real Mouse Wheel Operation Blocked ->" << keycodeString_WheelLeft;
                        }
                        else if (wheel_right_found) {
                            qDebug() << "[InterceptionMouseHookProc]" << "Real Mouse Wheel Operation Blocked ->" << keycodeString_WheelRight;
                        }
                        else if (wheel_up_found) {
                            qDebug() << "[InterceptionMouseHookProc]" << "Real Mouse Wheel Operation Blocked ->" << keycodeString_WheelUp;
                        }
                        else {
                            qDebug() << "[InterceptionMouseHookProc]" << "Real Mouse Wheel Operation Blocked ->" << keycodeString_WheelDown;
                        }
#endif
                        returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                    }
                    else if (mappingKeyList.constFirst().startsWith(KEY2MOUSE_PREFIX) && mappingKeyList.size() == 1) {
                        returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                    }
                    else {
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                        returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                    }

                    bool PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                    if (PassThrough && returnFlag) {
                        returnFlag = INTERCEPTION_RETURN_NORMALSEND;
#ifdef DEBUG_LOGOUT_ON
                        qDebug().noquote().nospace() << "[InterceptionMouseHookProc]" << "PassThrough MouseWheel[" << original_key << "]";
#endif
                    }
                }
            }
        }
    }
    else if (mouse_event == EVENT_MOUSEMOVE) {
        if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
            return INTERCEPTION_RETURN_NORMALSEND;
        }

#ifdef MOUSE_VERBOSE_LOG
        qDebug() << "[InterceptionMouseHookProc]" << "Mouse Move -> Delta X =" << delta_x << ", Delta Y =" << delta_y << ", MouseIndex =" << mouse_index;
#endif

        if (!s_Mouse2vJoy_EnableStateMap.isEmpty()) {
            if (s_Mouse2vJoy_Hold) {
                if (QKeyMapper::getvJoyLockCursorStatus()) {
                    return INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
                }
                else {
                    return INTERCEPTION_RETURN_NORMALSEND;
                }
            }

            if (QKeyMapper::getvJoyLockCursorStatus()) {
                returnFlag = INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
            }
            emit QKeyMapper_Worker::getInstance()->onMouseMove_Signal(delta_x, delta_y, mouse_index);
        }

    }

    return returnFlag;
}

LRESULT QKeyMapper_Worker::LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;
    ULONG_PTR extraInfo_nochanged = pKeyBoard->dwExtraInfo;
    ULONG_PTR extraInfo;
    if ((extraInfo_nochanged & VIRTUAL_CUSTOM_KEYS) == VIRTUAL_CUSTOM_KEYS) {
        extraInfo = VIRTUAL_CUSTOM_KEYS;
    }
    else {
        extraInfo = extraInfo_nochanged & ~0xF;
    }

    if (extraInfo == VIRTUAL_RESEND_REALKEY
        || extraInfo == VIRTUAL_UNICODE_CHAR) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

    bool returnFlag = false;
    V_KEYCODE vkeycode;
    vkeycode.KeyCode = (quint8)pKeyBoard->vkCode;
    if (LLKHF_EXTENDED == (pKeyBoard->flags & LLKHF_EXTENDED)){
        vkeycode.ExtenedFlag = EXTENED_FLAG_TRUE;
    }
    else{
        vkeycode.ExtenedFlag = EXTENED_FLAG_FALSE;
    }

    /* Virtual KeyCode Convert >>> */
    if (SCANCODE_SNAPSHOT == pKeyBoard->scanCode) {
        vkeycode.KeyCode = VK_SNAPSHOT;
        vkeycode.ExtenedFlag = EXTENED_FLAG_TRUE;
    }
    /* Virtual KeyCode Convert <<< */

    QString keycodeString = VirtualKeyCodeMap.key(vkeycode);
    QString keycodeString_nochanged = keycodeString;

//#ifdef DEBUG_LOGOUT_ON
//    qDebug("\"%s\" (0x%02X),  wParam(0x%04X), scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, wParam, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
//#endif

    if ((false == keycodeString.isEmpty())
        && (WM_KEYDOWN == wParam || WM_KEYUP == wParam || WM_SYSKEYDOWN == wParam || WM_SYSKEYUP == wParam)){
#ifdef DEBUG_LOGOUT_ON
        qDebug("[LowLevelKeyboardHookProc] currentThread -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif
        int keyupdown;
        if (WM_KEYDOWN == wParam || WM_SYSKEYDOWN == wParam) {
            keyupdown = KEY_DOWN;
        }
        else {
            keyupdown = KEY_UP;
        }

        if (extraInfo != VIRTUAL_KEY_SEND
            && extraInfo != VIRTUAL_KEY_OVERLAY
            && extraInfo != VIRTUAL_CUSTOM_KEYS) {
            if (Interception_Worker::s_InterceptStart) {
                if (extraInfo == INTERCEPTION_EXTRA_INFO_BLOCKED) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[LowLevelKeyboardHookProc]" << "Block" << (keyupdown == KEY_DOWN?"KEY_DOWN":"KEY_UP") << "extraInfo = INTERCEPTION_EXTRA_INFO_BLOCKED";
#endif
                    return (LRESULT)TRUE;
                }
                else {
                    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                }
            }

#ifdef DEBUG_LOGOUT_ON
            if (WM_KEYDOWN == wParam){
                qDebug("[LowLevelKeyboardHookProc] RealKey: \"%s\" (0x%02X) KeyDown, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo);
            }
            else if (WM_KEYUP == wParam){
                qDebug("[LowLevelKeyboardHookProc] RealKey: \"%s\" (0x%02X) KeyUp, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo);
            }
            else if (WM_SYSKEYDOWN == wParam){
                qDebug("[LowLevelKeyboardHookProc] RealKey: \"%s\" (0x%02X) SysKeyDown, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo);
            }
            else if (WM_SYSKEYUP == wParam){
                qDebug("[LowLevelKeyboardHookProc] RealKey: \"%s\" (0x%02X) SysKeyUp, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo);
            }
#endif

            static bool show_screenpoints = false;
            static bool show_windowpoints = false;
            if (KEY_DOWN == keyupdown){
#ifdef DEBUG_LOGOUT_ON
                if (keycodeString == SHOW_KEY_DEBUGINFO) {
                    {
                    QMutexLocker locker(&s_PressedMappingKeysMapMutex);
                    qDebug().nospace() << "\033[1;34m[LowLevelKeyboardHookProc]" << "Show debug info pressedMappingKeysMap -> " << QKeyMapper_Worker::pressedMappingKeysMap << "\033[0m";
                    }
                    qDebug().nospace() << "\033[1;34m[LowLevelKeyboardHookProc]" << "Show debug info pressedVirtualKeysList -> " << QKeyMapper_Worker::pressedVirtualKeysList << "\033[0m";
                    qDebug().nospace() << "\033[1;34m[LowLevelKeyboardHookProc]" << "Show debug info pressedRealKeysList -> " << QKeyMapper_Worker::pressedRealKeysList << "\033[0m";
                    qDebug().nospace() << "\033[1;34m[LowLevelKeyboardHookProc]" << "Show debug info pressedRealKeysListRemoveMultiInput -> " << QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput << "\033[0m";
                }
#endif

                if ((QKeyMapper::KEYMAP_MAPPING_GLOBAL == QKeyMapper::getInstance()->m_KeyMapStatus
                    || QKeyMapper::KEYMAP_MAPPING_MATCHED == QKeyMapper::getInstance()->m_KeyMapStatus)
                    && (keycodeString == QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_MousePassThroughSwitchKey)) {
                    emit QKeyMapper::getInstance()->switchFloatingWindowMousePassThrough_Signal();
                }
                else if (keycodeString == SHOW_POINTS_IN_SCREEN_KEY) {
                        if (!show_screenpoints) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[LowLevelKeyboardHookProc]" << "Show Mouse Points In Screen KEY_DOWN -> ON";
#endif
                            show_screenpoints = true;
                            emit QKeyMapper::getInstance()->showMousePoints_Signal(SHOW_POINTSIN_SCREEN_ON);
                        }
                }
                else if (keycodeString == SHOW_POINTS_IN_WINDOW_KEY) {
                        if (!show_windowpoints) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[LowLevelKeyboardHookProc]" << "Show Mouse Points In Window KEY_DOWN -> ON";
#endif
                            show_windowpoints = true;
                            emit QKeyMapper::getInstance()->showMousePoints_Signal(SHOW_POINTSIN_WINDOW_ON);
                        }
                }
                else if (keycodeString == SHOW_CAR_ORDINAL_KEY) {
                        if (s_LastCarOrdinal > 0) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[LowLevelKeyboardHookProc]" << "Show CarOrdinal Key Pressed, CarOrdinal =" << s_LastCarOrdinal;
#endif
                            emit QKeyMapper::getInstance()->showCarOrdinal_Signal(s_LastCarOrdinal);
                        }
                }
            }
            else {
                if (keycodeString == SHOW_POINTS_IN_SCREEN_KEY) {
                    if (show_screenpoints) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[LowLevelKeyboardHookProc]" << "Show Mouse Points In Screen KEY_UP -> OFF";
#endif
                        show_screenpoints = false;
                        emit QKeyMapper::getInstance()->showMousePoints_Signal(SHOW_POINTSIN_SCREEN_OFF);
                    }
                }
                else if (keycodeString == SHOW_POINTS_IN_WINDOW_KEY) {
                    if (show_windowpoints) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[LowLevelKeyboardHookProc]" << "Show Mouse Points In Window KEY_UP -> OFF";
#endif
                        show_windowpoints = false;
                        emit QKeyMapper::getInstance()->showMousePoints_Signal(SHOW_POINTSIN_WINDOW_OFF);
                    }
                }
            }

            /* Add extraInfo check for Multi InputDevice */
            bool multi_input = false;
            if (extraInfo > INTERCEPTION_EXTRA_INFO && extraInfo <= (INTERCEPTION_EXTRA_INFO + INTERCEPTION_MAX_DEVICE)) {
                InterceptionDevice device = extraInfo - INTERCEPTION_EXTRA_INFO;
                if (interception_is_keyboard(device)) {
                    keycodeString = QString("%1@%2").arg(keycodeString, QString::number(device - INTERCEPTION_KEYBOARD(0)));
                    multi_input = true;
                }
            }
            Q_UNUSED(multi_input);

            int findindex = -1;
            if (HOOKPROC_STATE_STARTED == s_AtomicHookProcState) {
                returnFlag = (hookBurstAndLockProc(keycodeString, keyupdown) != KEY_PROC_NONE);
                findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
            }

            int intercept = updatePressedRealKeysList(keycodeString, keyupdown);
            bool mappingswitch_detected = detectMappingSwitchKey(keycodeString_nochanged, keyupdown);
            bool displayswitch_detected = detectDisplaySwitchKey(keycodeString_nochanged, keyupdown);
            if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
                if ((mappingswitch_detected || displayswitch_detected) && KEY_DOWN == keyupdown) {
                    return (LRESULT)TRUE;
                }
                else if (intercept == KEY_INTERCEPT_BLOCK_KEY_RECORD) {
                    return (LRESULT)TRUE;
                }
                else {
                    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                }
            }
            else {
                bool tabswitch_detected = detectMappingTableTabHotkeys(keycodeString_nochanged, keyupdown);
                if ((mappingswitch_detected || displayswitch_detected || tabswitch_detected) && KEY_DOWN == keyupdown) {
                    return (LRESULT)TRUE;
                }

                if (intercept == KEY_INTERCEPT_BLOCK) {
                    return (LRESULT)TRUE;
                }
            }

            int combinationkey_detected = detectCombinationKeys(keycodeString, keyupdown);
            if (combinationkey_detected) {
                if (KEY_DOWN == keyupdown) {
                    if (KEY_INTERCEPT_PASSTHROUGH == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                        QString debugmessage = QString("[LowLevelKeyboardHookProc] detectCombinationKeys (%1) KEY_DOWN return -> KEY_INTERCEPT_PASSTHROUGH").arg(keycodeString);
                        qDebug().nospace().noquote() << debugmessage;
#endif
                        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                    }
                    else {
#ifdef DEBUG_LOGOUT_ON
                        QString debugmessage = QString("[LowLevelKeyboardHookProc] detectCombinationKeys (%1) KEY_DOWN return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                        qDebug().nospace().noquote() << debugmessage;
#endif
                        return (LRESULT)TRUE;
                    }
                }
                else {
                    if (findindex >= 0) {
// #ifdef DEBUG_LOGOUT_ON
//                         QString debugmessage = QString("[LowLevelKeyboardHookProc] OriginalKey \"%1\" found rowindex(%2), detectCombinationKeys KEY_UP return -> KEY_INTERCEPT_BLOCK").arg(keycodeString).arg(findindex);
//                         qDebug().nospace().noquote() << debugmessage;
// #endif
//                         return (LRESULT)TRUE;
                    }
                    else {
                        if (KEY_INTERCEPT_BLOCK == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                            QString debugmessage = QString("[LowLevelKeyboardHookProc] detectCombinationKeys (%1) KEY_UP return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                            qDebug().nospace().noquote() << debugmessage;
#endif
                            return (LRESULT)TRUE;
                        }
                        else {
#ifdef DEBUG_LOGOUT_ON
                            QString debugmessage = QString("[LowLevelKeyboardHookProc] detectCombinationKeys (%1) KEY_UP return -> %2").arg(keycodeString).arg(combinationkey_detected);
                            qDebug().nospace().noquote() << debugmessage;
#endif
                            return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                        }
                    }
                }
            }

            if (KEY_UP == keyupdown && false == returnFlag){
                if (findindex >= 0
                    && (QKeyMapper::KeyMappingDataList->at(findindex).Original_Key == keycodeString
                        || QKeyMapper::KeyMappingDataList->at(findindex).Original_Key == QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keycodeString))) {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[LowLevelKeyboardHookProc] \"%1\" has found in OriginalKeys, do not skip RealKey \"%2\" KEY_UP").arg(keycodeString, keycodeString);
                    qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                }
                else {
                    if (pressedVirtualKeysList.contains(keycodeString)) {
                        returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                        qDebug("[LowLevelKeyboardHookProc] VirtualKey \"%s\" is pressed down, skip RealKey \"%s\" KEY_UP!", keycodeString.toStdString().c_str(), keycodeString.toStdString().c_str());
#endif
                    }
                }
            }

            if (false == returnFlag) {
                if (findindex >=0){
                    QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
                    QStringList mappingKey_KeyUpList = QKeyMapper::KeyMappingDataList->at(findindex).MappingKeys_KeyUp;
                    QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
                    QString firstmappingkey = mappingKeyList.constFirst();
                    int mappingkeylist_size = mappingKeyList.size();
                    if (mappingkeylist_size == 1 && firstmappingkey == KEY_BLOCKED_STR) {
#ifdef DEBUG_LOGOUT_ON
                        if (KEY_DOWN == keyupdown){
                            qDebug() << "[LowLevelKeyboardHookProc]" << "RealKey KEY_DOWN Blocked ->" << original_key;
                        }
                        else {
                            qDebug() << "[LowLevelKeyboardHookProc]" << "RealKey KEY_UP Blocked ->" << original_key;
                        }
#endif
                        returnFlag = true;
                    }
                    else if (firstmappingkey.startsWith(FUNC_PREFIX) && mappingkeylist_size == 1) {
#ifdef DEBUG_LOGOUT_ON
                        if (KEY_DOWN == keyupdown){
                            qDebug() << "[LowLevelKeyboardHookProc]" << "Function KEY_DOWN ->" << firstmappingkey;
                        }
                        else {
                            qDebug() << "[LowLevelKeyboardHookProc]" << "Function KEY_UP ->" << firstmappingkey;
                        }
#endif
                        if (KEY_DOWN == keyupdown){
                            emit QKeyMapper_Worker::getInstance()->doFunctionMappingProc_Signal(firstmappingkey);
                        }

                        returnFlag = true;
                    }
                    else {
                        if (firstmappingkey.startsWith(KEY2MOUSE_PREFIX) && mappingkeylist_size == 1) {
                            if (KEY_DOWN == keyupdown){
#ifdef DEBUG_LOGOUT_ON
                                qDebug() << "[LowLevelKeyboardHookProc]" << "Key2Mouse Key(" << original_key << ") Down ->" << firstmappingkey;
#endif
                            }
                            else {
#ifdef DEBUG_LOGOUT_ON
                                qDebug() << "[LowLevelKeyboardHookProc]" << "Key2Mouse Key(" << original_key << ") Up ->" << firstmappingkey;
#endif
                            }
                        }

                        int SendTiming = QKeyMapper::KeyMappingDataList->at(findindex).SendTiming;
                        bool KeySeqHoldDown = QKeyMapper::KeyMappingDataList->at(findindex).KeySeqHoldDown;
                        if (KEY_DOWN == keyupdown){
                            if (SENDTIMING_KEYDOWN == SendTiming) {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                            }
                            else if (SENDTIMING_KEYUP == SendTiming) {
                                /* KEY_DOWN & SENDTIMING_KEYUP == SendTiming -> do nothing */
                            }
                            else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                            }
                            else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            }
                            else { /* SENDTIMING_NORMAL == SendTiming */
                                /* Add for KeySequenceHoldDown >>> */
                                if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                                }
                                else {
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                }
                                /* Add for KeySequenceHoldDown <<< */
                            }
                            returnFlag = true;
                        }
                        else { /* KEY_UP == keyupdown */
                            if (SENDTIMING_KEYDOWN == SendTiming) {
                                /* KEY_UP & SENDTIMING_KEYDOWN == SendTiming -> do nothing */
                            }
                            else if (SENDTIMING_KEYUP == SendTiming) {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                            }
                            else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                            }
                            else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);

                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                            }
                            else { /* SENDTIMING_NORMAL == SendTiming */
                                /* Add for KeySequenceHoldDown >>> */
                                if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                                }
                                else {
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                                }
                                /* Add for KeySequenceHoldDown <<< */
                            }
                            returnFlag = true;
                        }
                    }

                    bool PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                    if (PassThrough && returnFlag) {
                        returnFlag = false;
#ifdef DEBUG_LOGOUT_ON
                        QString keyUpDown;
                        if (KEY_DOWN == keyupdown){
                            keyUpDown = "KEY_DOWN";
                        }
                        else {
                            keyUpDown = "KEY_UP";
                        }
                        qDebug().noquote().nospace() << "[LowLevelKeyboardHookProc]" << "PassThrough Key[" << original_key << "] " << keyUpDown;
#endif
                    }
                }
            }
        }
        else {
            int sendVirtualKeyState = SENDVIRTUALKEY_STATE_NORMAL;
            int row_index = INITIAL_ROW_INDEX;
            if (VIRTUAL_CUSTOM_KEYS == extraInfo) {
                int temp_row_index = static_cast<int>(extraInfo_nochanged & USHRT_MAX);
                if (0 <= temp_row_index && temp_row_index < USHRT_MAX) {
                    row_index = temp_row_index;
                }
            }
            else if (VIRTUAL_KEY_SEND == extraInfo) {
                sendVirtualKeyState = static_cast<int>(extraInfo_nochanged & 0xF);
            }

#ifdef DEBUG_LOGOUT_ON
            if (WM_KEYDOWN == wParam){
                qDebug("[LowLevelKeyboardHookProc] VirtualKey: \"%s\" (0x%02X) KeyDown, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X), sendVirtualKeyState(%d)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo, sendVirtualKeyState);
            }
            else if (WM_KEYUP == wParam){
                qDebug("[LowLevelKeyboardHookProc] VirtualKey: \"%s\" (0x%02X) KeyUp, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X), sendVirtualKeyState(%d)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo, sendVirtualKeyState);
            }
            else if (WM_SYSKEYDOWN == wParam){
                qDebug("[LowLevelKeyboardHookProc] VirtualKey: \"%s\" (0x%02X) SysKeyDown, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X), sendVirtualKeyState(%d)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo, sendVirtualKeyState);
            }
            else if (WM_SYSKEYUP == wParam){
                qDebug("[LowLevelKeyboardHookProc] VirtualKey: \"%s\" (0x%02X) SysKeyUp, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X), sendVirtualKeyState(%d)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo, sendVirtualKeyState);
            }
#endif

            if (VIRTUAL_KEY_OVERLAY == extraInfo) {
                return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
            }

            if (KEY_DOWN == keyupdown){
                if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
                    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                }

                if (false == pressedVirtualKeysList.contains(keycodeString)){
                    pressedVirtualKeysList.append(keycodeString);

                    if (keycodeString == KEY2MOUSE_UP_STR) {
                        s_Key2Mouse_Up = true;
                    }
                    else if (keycodeString == KEY2MOUSE_DOWN_STR) {
                        s_Key2Mouse_Down = true;
                    }
                    else if (keycodeString == KEY2MOUSE_LEFT_STR) {
                        s_Key2Mouse_Left = true;
                    }
                    else if (keycodeString == KEY2MOUSE_RIGHT_STR) {
                        s_Key2Mouse_Right = true;
                    }
                    else if (keycodeString == MOUSE2VJOY_HOLD_KEY_STR) {
                        s_Mouse2vJoy_Hold = true;
                    }
                    else if (keycodeString == GYRO2MOUSE_MOVE_KEY_STR) {
                        s_Gyro2Mouse_MoveActive = true;
                    }
                    else if (keycodeString == GYRO2MOUSE_HOLD_KEY_STR) {
                        s_Gyro2Mouse_MoveActive = false;
                    }
                    else if (keycodeString == CROSSHAIR_TYPEA_STR) {
                        s_Crosshair_TypeA = true;
                    }
                    else if (keycodeString == CROSSHAIR_NORMAL_STR) {
                        s_Crosshair_Normal = true;
                    }
                }
            }
            /* KEY_UP == keyupdown */
            else {
                if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState
                    && SENDVIRTUALKEY_STATE_BURST_STOP != sendVirtualKeyState
                    && SENDVIRTUALKEY_STATE_FORCE != sendVirtualKeyState) {
                    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                }

                if (SENDVIRTUALKEY_STATE_NORMAL == sendVirtualKeyState
                    || SENDVIRTUALKEY_STATE_BURST_STOP == sendVirtualKeyState
                    || SENDVIRTUALKEY_STATE_FORCE == sendVirtualKeyState) {
                    if (pressedRealKeysListRemoveMultiInput.contains(keycodeString) && !blockedKeysList.contains(keycodeString)){
                        int findindex = QKeyMapper::findOriKeyInKeyMappingDataList_RemoveMultiInput(keycodeString);
                        if (findindex < 0) {
#ifdef DEBUG_LOGOUT_ON
                            QString debugmessage = QString("[LowLevelKeyboardHookProc] RealKey \"%1\" is pressed down on keyboard, skip send mapping VirtualKey \"%2\" KEY_UP! sendVirtualKeyState = %3").arg(keycodeString, keycodeString).arg(sendVirtualKeyState);
                            qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                            returnFlag = true;
                        }
                    }
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[LowLevelKeyboardHookProc] sendVirtualKeyState = %1, check RealKey \"%2\" pressed down state on keyboard is passed!").arg(sendVirtualKeyState).arg(keycodeString);
                    qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                }

                pressedVirtualKeysList.removeAll(keycodeString);

                if (keycodeString == KEY2MOUSE_UP_STR) {
                    s_Key2Mouse_Up = false;
                }
                else if (keycodeString == KEY2MOUSE_DOWN_STR) {
                    s_Key2Mouse_Down = false;
                }
                else if (keycodeString == KEY2MOUSE_LEFT_STR) {
                    s_Key2Mouse_Left = false;
                }
                else if (keycodeString == KEY2MOUSE_RIGHT_STR) {
                    s_Key2Mouse_Right = false;
                }
                else if (keycodeString == MOUSE2VJOY_HOLD_KEY_STR) {
                    s_Mouse2vJoy_Hold = false;
                }
                else if (keycodeString == GYRO2MOUSE_MOVE_KEY_STR) {
                    s_Gyro2Mouse_MoveActive = false;
                }
                else if (keycodeString == GYRO2MOUSE_HOLD_KEY_STR) {
                    s_Gyro2Mouse_MoveActive = true;
                }
                else if (keycodeString == CROSSHAIR_TYPEA_STR) {
                    s_Crosshair_TypeA = false;
                }
                else if (keycodeString == CROSSHAIR_NORMAL_STR) {
                    s_Crosshair_Normal = false;
                }
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[LowLevelKeyboardHookProc]" << (keyupdown == KEY_DOWN?"KEY_DOWN":"KEY_UP") << " : pressedVirtualKeysList -> " << pressedVirtualKeysList;
#endif
            if (extraInfo == VIRTUAL_CUSTOM_KEYS) {
                if (keycodeString.startsWith(CROSSHAIR_PREFIX)) {
                    if (keyupdown == KEY_DOWN) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[LowLevelKeyboardHookProc]" << keycodeString << "KEY_DOWN Show Crosshair Start.";
#endif
                        emit QKeyMapper::getInstance()->showCrosshairStart_Signal(row_index, keycodeString);
                    }
                    else {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[LowLevelKeyboardHookProc]" << keycodeString << "KEY_UP Show Crosshair Stop.";
#endif
                        emit QKeyMapper::getInstance()->showCrosshairStop_Signal(row_index, keycodeString);
                    }
                }
                else if (keycodeString.startsWith(MOUSE2VJOY_PREFIX)
                    && false == s_Mouse2vJoy_EnableStateMap.isEmpty()) {
                    if (keycodeString == MOUSE2VJOY_HOLD_KEY_STR) {
                        if (keyupdown == KEY_UP) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[LowLevelKeyboardHookProc]" << keycodeString << "KEY_UP Start Mouse2vJoyResetTimer.";
#endif
                            emit QKeyMapper_Worker::getInstance()->startMouse2vJoyResetTimer_Signal(keycodeString, INITIAL_MOUSE_INDEX);
                        }
                        else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[LowLevelKeyboardHookProc]" << keycodeString << "KEY_DOWN Stop Mouse2vJoyResetTimer.";
#endif
                            emit QKeyMapper_Worker::getInstance()->stopMouse2vJoyResetTimer_Signal(keycodeString, INITIAL_MOUSE_INDEX);
                        }
                    }
                }

                returnFlag = true;
            }
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        QString keyupdownStr = "KEY_UPDOWN_UNKNOWN";
        if (WM_KEYDOWN == wParam || WM_SYSKEYDOWN == wParam) {
            keyupdownStr = "KEY_DOWN";
        }
        else if (WM_KEYUP == wParam || WM_SYSKEYUP == wParam) {
            keyupdownStr = "KEY_UP";
        }
        qDebug("[LowLevelKeyboardHookProc] UnknownKey (0x%02X) %s, scanCode(0x%08X), wParam(0x%08X), flags(0x%08X), ExtenedFlag(%s), extraInfo(0x%08X)", pKeyBoard->vkCode, keyupdownStr.toStdString().c_str(), pKeyBoard->scanCode, wParam, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false", extraInfo);
#endif
    }

    if (true == returnFlag){
#ifdef DEBUG_LOGOUT_ON
        qDebug("[LowLevelKeyboardHookProc] return TRUE");
#endif
        return (LRESULT)TRUE;
    }
    else{
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }
}

LRESULT QKeyMapper_Worker::LowLevelMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

    MSLLHOOKSTRUCT *pMouse = (MSLLHOOKSTRUCT *)lParam;
    ULONG_PTR extraInfo_nochanged = pMouse->dwExtraInfo;
    ULONG_PTR extraInfo = extraInfo_nochanged & ~0xF;
    DWORD mousedata = pMouse->mouseData;

    if (extraInfo == VIRTUAL_RESEND_REALKEY) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

    bool returnFlag = false;
    if ((wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP)
        || (wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP)
        || (wParam == WM_MBUTTONDOWN || wParam == WM_MBUTTONUP)
        || (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP)) {
        int keyupdown;
        WPARAM wParam_X;
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN || wParam == WM_XBUTTONDOWN) {
            keyupdown = KEY_DOWN;
        }
        else {
            keyupdown = KEY_UP;
        }
        if (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP) {
            WORD xbutton = GET_XBUTTON_WPARAM(mousedata);
            if ( xbutton == XBUTTON1 ) {
                wParam_X = MAKELONG(wParam, XBUTTON1);
            }
            else if ( xbutton == XBUTTON2 ) {
                wParam_X = MAKELONG(wParam, XBUTTON2);
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("[LowLevelMouseHookProc] Unsupport Mouse XButton -> 0x%04X", xbutton);
#endif
            }
        }
        else {
            wParam_X = MAKELONG(wParam, XBUTTON_NONE);
        }

        if (true == MouseButtonNameMap.contains(wParam_X)) {
            QString keycodeString = MouseButtonNameMap.value(wParam_X);
            QString keycodeString_nochanged = keycodeString;
            if (VIRTUAL_KEY_SEND == extraInfo
                || VIRTUAL_KEY_OVERLAY == extraInfo
                || VIRTUAL_MOUSE_POINTCLICK == extraInfo) {
                int sendVirtualKeyState = SENDVIRTUALKEY_STATE_NORMAL;
                if (VIRTUAL_KEY_SEND == extraInfo) {
                    sendVirtualKeyState = static_cast<int>(extraInfo_nochanged & 0xF);
                }

#ifdef DEBUG_LOGOUT_ON
                qDebug("[LowLevelMouseHookProc] Virtual \"%s\" %s, extraInfo(0x%08X), sendVirtualKeyState(%d)", MouseButtonNameMap.value(wParam_X).toStdString().c_str(), (keyupdown == KEY_DOWN?"Button Down":"Button Up"), extraInfo, sendVirtualKeyState);
#endif

                if (VIRTUAL_KEY_OVERLAY == extraInfo) {
                    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                }

                if (KEY_DOWN == keyupdown) {
                    if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
                        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                    }

                    if (false == pressedVirtualKeysList.contains(keycodeString)){
                        pressedVirtualKeysList.append(keycodeString);

                        if (keycodeString == KEY2MOUSE_UP_STR) {
                            s_Key2Mouse_Up = true;
                        }
                        else if (keycodeString == KEY2MOUSE_DOWN_STR) {
                            s_Key2Mouse_Down = true;
                        }
                        else if (keycodeString == KEY2MOUSE_LEFT_STR) {
                            s_Key2Mouse_Left = true;
                        }
                        else if (keycodeString == KEY2MOUSE_RIGHT_STR) {
                            s_Key2Mouse_Right = true;
                        }
                        else if (keycodeString == MOUSE2VJOY_HOLD_KEY_STR) {
                            s_Mouse2vJoy_Hold = true;
                        }
                        else if (keycodeString == GYRO2MOUSE_MOVE_KEY_STR) {
                            s_Gyro2Mouse_MoveActive = true;
                        }
                        else if (keycodeString == GYRO2MOUSE_HOLD_KEY_STR) {
                            s_Gyro2Mouse_MoveActive = false;
                        }
                        else if (keycodeString == CROSSHAIR_TYPEA_STR) {
                            s_Crosshair_TypeA = true;
                        }
                        else if (keycodeString == CROSSHAIR_NORMAL_STR) {
                            s_Crosshair_Normal = true;
                        }
                    }
                }
                else {
                    if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState
                        && SENDVIRTUALKEY_STATE_BURST_STOP != sendVirtualKeyState
                        && SENDVIRTUALKEY_STATE_FORCE != sendVirtualKeyState) {
                        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                    }

                    if (SENDVIRTUALKEY_STATE_NORMAL == sendVirtualKeyState
                        || SENDVIRTUALKEY_STATE_BURST_STOP == sendVirtualKeyState
                        || SENDVIRTUALKEY_STATE_FORCE == sendVirtualKeyState) {
                        if (pressedRealKeysListRemoveMultiInput.contains(keycodeString) && !blockedKeysList.contains(keycodeString)){
                            int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
                            if (findindex < 0) {
#ifdef DEBUG_LOGOUT_ON
                                QString debugmessage = QString("[LowLevelMouseHookProc] RealKey \"%1\" is pressed down on mouse, skip send mapping VirtualKey \"%2\" KEYUP! sendVirtualKeyState = %3").arg(keycodeString, keycodeString).arg(sendVirtualKeyState);
                                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                                returnFlag = true;
                            }
                        }
                    }
                    else {
#ifdef DEBUG_LOGOUT_ON
                        QString debugmessage = QString("[LowLevelMouseHookProc] sendVirtualKeyState = %1, check RealKey \"%2\" pressed down state on mouse is passed!").arg(sendVirtualKeyState).arg(keycodeString);
                        qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                    }

                    pressedVirtualKeysList.removeAll(keycodeString);

                    if (keycodeString == KEY2MOUSE_UP_STR) {
                        s_Key2Mouse_Up = false;
                    }
                    else if (keycodeString == KEY2MOUSE_DOWN_STR) {
                        s_Key2Mouse_Down = false;
                    }
                    else if (keycodeString == KEY2MOUSE_LEFT_STR) {
                        s_Key2Mouse_Left = false;
                    }
                    else if (keycodeString == KEY2MOUSE_RIGHT_STR) {
                        s_Key2Mouse_Right = false;
                    }
                    else if (keycodeString == MOUSE2VJOY_HOLD_KEY_STR) {
                        s_Mouse2vJoy_Hold = false;
                    }
                    else if (keycodeString == GYRO2MOUSE_MOVE_KEY_STR) {
                        s_Gyro2Mouse_MoveActive = false;
                    }
                    else if (keycodeString == GYRO2MOUSE_HOLD_KEY_STR) {
                        s_Gyro2Mouse_MoveActive = true;
                    }
                    else if (keycodeString == CROSSHAIR_TYPEA_STR) {
                        s_Crosshair_TypeA = false;
                    }
                    else if (keycodeString == CROSSHAIR_NORMAL_STR) {
                        s_Crosshair_Normal = false;
                    }
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[LowLevelMouseHookProc]" << (keyupdown == KEY_DOWN?"KEY_DOWN":"KEY_UP") << " : pressedVirtualKeysList -> " << pressedVirtualKeysList;
#endif
            }
            else {
                if (Interception_Worker::s_InterceptStart) {
                    if (extraInfo == INTERCEPTION_EXTRA_INFO_BLOCKED) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[LowLevelMouseHookProc]" << "Block" << (keyupdown == KEY_DOWN?"KEY_DOWN":"KEY_UP") << "extraInfo = INTERCEPTION_EXTRA_INFO_BLOCKED";
#endif
                        return (LRESULT)TRUE;
                    }
                    else {
                        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                    }
                }

#ifdef DEBUG_LOGOUT_ON
                qDebug("[LowLevelMouseHookProc] Real \"%s\" %s, extraInfo(0x%08X)", MouseButtonNameMap.value(wParam_X).toStdString().c_str(), (keyupdown == KEY_DOWN?"Button Down":"Button Up"), extraInfo);
#endif
                if ((GetAsyncKeyState(PICK_SCREEN_POINT_KEY) & 0x8000) != 0 && wParam == WM_LBUTTONDOWN) {
                    POINT pt;
                    if (GetCursorPos(&pt)) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[LowLevelMouseHookProc]" << "L-Ctrl + Mouse-Left Click Capture Screen MousePoint -> X =" << pt.x << ", Y=" << pt.y;
#endif
                        QPoint point = QPoint(pt.x, pt.y);
                        emit QKeyMapper::getInstance()->updateMousePointLabelDisplay_Signal(point);
                    }
                }
                else if ((GetAsyncKeyState(PICK_WINDOW_POINT_KEY) & 0x8000) != 0 && wParam == WM_LBUTTONDOWN) {
                    POINT pt;
                    HWND hwnd = QKeyMapper::s_CurrentMappingHWND;
                    if (hwnd != NULL && GetCursorPos(&pt)) {
                        if (ScreenToClient(hwnd, &pt)) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[LowLevelMouseHookProc]" << "L-Ctrl + Mouse-Left Click Capture Window MousePoint -> X =" << pt.x << ", Y=" << pt.y;
#endif
                            QPoint point = QPoint(pt.x, pt.y);
                            emit QKeyMapper::getInstance()->updateMousePointLabelDisplay_Signal(point);
                        }
                    }
                }
//                 else if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0 && wParam == WM_RBUTTONDOWN) {
// #ifdef DEBUG_LOGOUT_ON
//                     qDebug() << "[LowLevelMouseHookProc]" << "L-Ctrl + Mouse-Right Click Clear MousePoint";
// #endif
//                     QPoint point = QPoint(-500, -500);
//                     emit QKeyMapper::getInstance()->updateMousePointLabelDisplay_Signal(point);
//                 }

                /* Add extraInfo check for Multi InputDevice */
                bool multi_input = false;
                if (extraInfo > INTERCEPTION_EXTRA_INFO && extraInfo <= (INTERCEPTION_EXTRA_INFO + INTERCEPTION_MAX_DEVICE)) {
                    InterceptionDevice device = extraInfo - INTERCEPTION_EXTRA_INFO;
                    if (interception_is_mouse(device)) {
                        keycodeString = QString("%1@%2").arg(keycodeString, QString::number(device - INTERCEPTION_MOUSE(0)));
                        multi_input = true;
                    }
                }
                Q_UNUSED(multi_input);

                int findindex = -1;
                if (HOOKPROC_STATE_STARTED == s_AtomicHookProcState) {
                    returnFlag = (hookBurstAndLockProc(keycodeString, keyupdown) != KEY_PROC_NONE);
                    findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
                }

                int intercept = updatePressedRealKeysList(keycodeString, keyupdown);
                bool mappingswitch_detected = detectMappingSwitchKey(keycodeString_nochanged, keyupdown);
                bool displayswitch_detected = detectDisplaySwitchKey(keycodeString_nochanged, keyupdown);
                if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
                    if ((mappingswitch_detected || displayswitch_detected) && KEY_DOWN == keyupdown) {
                        return (LRESULT)TRUE;
                    }
                    else if (intercept == KEY_INTERCEPT_BLOCK_KEY_RECORD) {
                        return (LRESULT)TRUE;
                    }
                    else {
                        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                    }
                }
                else {
                    bool tabswitch_detected = detectMappingTableTabHotkeys(keycodeString_nochanged, keyupdown);
                    if ((mappingswitch_detected || displayswitch_detected || tabswitch_detected) && KEY_DOWN == keyupdown) {
                        return (LRESULT)TRUE;
                    }

                    if (intercept == KEY_INTERCEPT_BLOCK) {
                        return (LRESULT)TRUE;
                    }
                }

                int combinationkey_detected = detectCombinationKeys(keycodeString, keyupdown);
                if (combinationkey_detected) {
                    if (KEY_DOWN == keyupdown) {
                        if (KEY_INTERCEPT_PASSTHROUGH == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                            QString debugmessage = QString("[LowLevelMouseHookProc] detectCombinationKeys (%1) KEY_DOWN return -> KEY_INTERCEPT_PASSTHROUGH").arg(keycodeString);
                            qDebug().nospace().noquote() << debugmessage;
#endif
                            return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                        }
                        else {
#ifdef DEBUG_LOGOUT_ON
                            QString debugmessage = QString("[LowLevelMouseHookProc] detectCombinationKeys (%1) KEY_DOWN return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                            qDebug().nospace().noquote() << debugmessage;
#endif
                            return (LRESULT)TRUE;
                        }
                    }
                    else {
                        if (findindex >= 0) {
// #ifdef DEBUG_LOGOUT_ON
//                             QString debugmessage = QString("[LowLevelMouseHookProc] OriginalKey \"%1\" found rowindex(%d), detectCombinationKeys KEY_UP return -> KEY_INTERCEPT_BLOCK").arg(keycodeString).arg(findindex);
//                             qDebug().nospace().noquote() << debugmessage;
// #endif
//                             return (LRESULT)TRUE;
                        }
                        else {
                            if (KEY_INTERCEPT_BLOCK == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                                QString debugmessage = QString("[LowLevelMouseHookProc] detectCombinationKeys (%1) KEY_UP return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                                qDebug().nospace().noquote() << debugmessage;
#endif
                                return (LRESULT)TRUE;
                            }
                            else {
#ifdef DEBUG_LOGOUT_ON
                                QString debugmessage = QString("[LowLevelMouseHookProc] detectCombinationKeys (%1) KEY_UP return -> %2").arg(keycodeString).arg(combinationkey_detected);
                                qDebug().nospace().noquote() << debugmessage;
#endif
                                return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                            }
                        }
                    }
                }

                if (KEY_UP == keyupdown && false == returnFlag){
                    if (findindex >= 0
                        && (QKeyMapper::KeyMappingDataList->at(findindex).Original_Key == keycodeString
                            || QKeyMapper::KeyMappingDataList->at(findindex).Original_Key == QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keycodeString))) {
                    }
                    else {
                        if (pressedVirtualKeysList.contains(keycodeString)) {
                            returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                            qDebug("Virtual \"%s\" is pressed down, skip Real \"%s\" KEY_UP!", keycodeString.toStdString().c_str(), keycodeString.toStdString().c_str());
#endif
                        }
                    }
                }

                if (false == returnFlag) {
                    if (findindex >=0){
                        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
                        QStringList mappingKey_KeyUpList = QKeyMapper::KeyMappingDataList->at(findindex).MappingKeys_KeyUp;
                        QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
                        QString firstmappingkey = mappingKeyList.constFirst();
                        int mappingkeylist_size = mappingKeyList.size();
                        if (mappingkeylist_size == 1 && firstmappingkey == KEY_BLOCKED_STR) {
#ifdef DEBUG_LOGOUT_ON
                            if (KEY_DOWN == keyupdown){
                                qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Button Down Blocked ->" << original_key;
                            }
                            else {
                                qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Button Up Blocked ->" << original_key;
                            }
#endif
                            returnFlag = true;
                        }
                        else if (firstmappingkey.startsWith(FUNC_PREFIX) && mappingkeylist_size == 1) {
#ifdef DEBUG_LOGOUT_ON
                            if (KEY_DOWN == keyupdown){
                                qDebug() << "[LowLevelMouseHookProc]" << "Function KEY_DOWN ->" << firstmappingkey;
                            }
                            else {
                                qDebug() << "[LowLevelMouseHookProc]" << "Function KEY_UP ->" << firstmappingkey;
                            }
#endif
                            if (KEY_DOWN == keyupdown){
                                emit QKeyMapper_Worker::getInstance()->doFunctionMappingProc_Signal(firstmappingkey);
                            }

                            returnFlag = true;
                        }
                        else {
                            if (firstmappingkey.startsWith(KEY2MOUSE_PREFIX) && mappingkeylist_size == 1) {
                                if (KEY_DOWN == keyupdown){
#ifdef DEBUG_LOGOUT_ON
                                    qDebug() << "[LowLevelMouseHookProc]" << "Key2Mouse Key(" << original_key << ") Down ->" << firstmappingkey;
#endif
                                }
                                else {
#ifdef DEBUG_LOGOUT_ON
                                    qDebug() << "[LowLevelMouseHookProc]" << "Key2Mouse Key(" << original_key << ") Up ->" << firstmappingkey;
#endif
                                }
                            }

                            int SendTiming = QKeyMapper::KeyMappingDataList->at(findindex).SendTiming;
                            bool KeySeqHoldDown = QKeyMapper::KeyMappingDataList->at(findindex).KeySeqHoldDown;
                            if (KEY_DOWN == keyupdown){
                                if (SENDTIMING_KEYDOWN == SendTiming) {
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                                }
                                else if (SENDTIMING_KEYUP == SendTiming) {
                                    /* KEY_DOWN & SENDTIMING_KEYUP == SendTiming -> do nothing */
                                }
                                else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                                }
                                else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                }
                                else { /* SENDTIMING_NORMAL == SendTiming */
                                    /* Add for KeySequenceHoldDown >>> */
                                    if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                                    }
                                    else {
                                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                    }
                                    /* Add for KeySequenceHoldDown <<< */
                                }
                                returnFlag = true;
                            }
                            else { /* KEY_UP == keyupdown */
                                if (SENDTIMING_KEYDOWN == SendTiming) {
                                    /* KEY_UP & SENDTIMING_KEYDOWN == SendTiming -> do nothing */
                                }
                                else if (SENDTIMING_KEYUP == SendTiming) {
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                                }
                                else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                                }
                                else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);

                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                                }
                                else { /* SENDTIMING_NORMAL == SendTiming */
                                    /* Add for KeySequenceHoldDown >>> */
                                    if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                                    }
                                    else {
                                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                                    }
                                    /* Add for KeySequenceHoldDown <<< */
                                }
                                returnFlag = true;
                            }
                        }

                        bool PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                        if (PassThrough && returnFlag) {
                            returnFlag = false;
#ifdef DEBUG_LOGOUT_ON
                            QString keyUpDown;
                            if (KEY_DOWN == keyupdown){
                                keyUpDown = "KEY_DOWN";
                            }
                            else {
                                keyUpDown = "KEY_UP";
                            }
                            qDebug().noquote().nospace() << "[LowLevelMouseHookProc]" << "PassThrough Key[" << original_key << "] " << keyUpDown;
#endif
                        }
                    }
                }
            }
        }
    }
    else if (wParam == WM_MOUSEWHEEL || wParam == WM_MOUSEHWHEEL) {
        if (VIRTUAL_MOUSE_WHEEL == extraInfo) {
            if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
                return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
            }

#ifdef DEBUG_LOGOUT_ON
            short zDelta = GET_WHEEL_DELTA_WPARAM(mousedata);

#ifdef MOUSE_VERBOSE_LOG
            if (wParam == WM_MOUSEHWHEEL) {
                qDebug() << "[LowLevelMouseHookProc]" << "Virtual Mouse Wheel Horizontal -> Delta =" << zDelta;
            }
            else {
                qDebug() << "[LowLevelMouseHookProc]" << "Virtual Mouse Wheel Vertical -> Delta =" << zDelta;
            }
#endif
            if (zDelta > 0) {
                if (wParam == WM_MOUSEHWHEEL) {
                    qDebug() << "[LowLevelMouseHookProc]" << "Virtual Mouse Wheel Right";
                }
                else {
                    qDebug() << "[LowLevelMouseHookProc]" << "Virtual Mouse Wheel Up";
                }
            }
            else {
                if (wParam == WM_MOUSEHWHEEL) {
                    qDebug() << "[LowLevelMouseHookProc]" << "Virtual Mouse Wheel Left";
                }
                else {
                    qDebug() << "[LowLevelMouseHookProc]" << "Virtual Mouse Wheel Down";
                }
            }
#endif
        }
        else {
            short zDelta = GET_WHEEL_DELTA_WPARAM(mousedata);

            if (zDelta == 0) {
                return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
            }
            else if (Interception_Worker::s_InterceptStart) {
                if (extraInfo == INTERCEPTION_EXTRA_INFO_BLOCKED) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[LowLevelMouseHookProc]" << "Block Mouse Wheel extraInfo = INTERCEPTION_EXTRA_INFO_BLOCKED";
#endif
                    return (LRESULT)TRUE;
                }
                else {
                    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                }
            }

#ifdef DEBUG_LOGOUT_ON
            QString extraInfoStr = QString("0x%1").arg(QString::number(extraInfo, 16).toUpper(), 8, '0');
            if (zDelta > 0) {
                if (wParam == WM_MOUSEHWHEEL) {
                    qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Right -> Delta =" << zDelta << ", extraInfoStr =" << extraInfoStr;
                }
                else {
                    qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Up -> Delta =" << zDelta << ", extraInfoStr =" << extraInfoStr;
                }
            }
            else {
                if (wParam == WM_MOUSEHWHEEL) {
                    qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Left -> Delta =" << zDelta << ", extraInfoStr =" << extraInfoStr;
                }
                else {
                    qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Down -> Delta =" << zDelta << ", extraInfoStr =" << extraInfoStr;
                }
            }
#endif
            QString keycodeString;
            int input_type;
            if (zDelta > 0) {
                if (wParam == WM_MOUSEHWHEEL) {
                    keycodeString = MOUSE_WHEEL_RIGHT_STR;
                    input_type = INPUT_MOUSE_WHEEL;
                }
                else {
                    keycodeString = MOUSE_WHEEL_UP_STR;
                    input_type = INPUT_MOUSE_WHEEL;
                }
            }
            else {
                if (wParam == WM_MOUSEHWHEEL) {
                    keycodeString = MOUSE_WHEEL_LEFT_STR;
                    input_type = INPUT_MOUSE_WHEEL;
                }
                else {
                    keycodeString = MOUSE_WHEEL_DOWN_STR;
                    input_type = INPUT_MOUSE_WHEEL;
                }
            }

            if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
                bool block = false;
                if (s_KeyRecording) {
                    bool recorded = updateRecordKeyList(keycodeString, input_type);
                    if (recorded) {
                        block = true;
                    }
                }

                if (block) {
                    return (LRESULT)TRUE;
                }
                else {
                    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                }
            }

            /* Add extraInfo check for Multi InputDevice */
            bool multi_input = false;
            if (extraInfo > INTERCEPTION_EXTRA_INFO && extraInfo <= (INTERCEPTION_EXTRA_INFO + INTERCEPTION_MAX_DEVICE)) {
                InterceptionDevice device = extraInfo - INTERCEPTION_EXTRA_INFO;
                if (interception_is_mouse(device)) {
                    keycodeString = QString("%1@%2").arg(keycodeString, QString::number(device - INTERCEPTION_MOUSE(0)));
                    multi_input = true;
                }
            }
            Q_UNUSED(multi_input);

            int keyupdown = KEY_DOWN;
            (void)updatePressedRealKeysList(keycodeString, keyupdown);
            int combinationkey_detected = detectCombinationKeys(keycodeString, keyupdown);
            Q_UNUSED(combinationkey_detected);
            keyupdown = KEY_UP;
            (void)updatePressedRealKeysList(keycodeString, keyupdown);
            combinationkey_detected = detectCombinationKeys(keycodeString, keyupdown);
            if (combinationkey_detected) {
                if (KEY_INTERCEPT_PASSTHROUGH == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[LowLevelMouseHookProc] detectCombinationKeys MouseWheel (%1) return -> KEY_INTERCEPT_PASSTHROUGH").arg(keycodeString);
                    qDebug().nospace().noquote() << debugmessage;
#endif
                    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[LowLevelMouseHookProc] detectCombinationKeys MouseWheel (%1) return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                    qDebug().nospace().noquote() << debugmessage;
#endif
                    return (LRESULT)TRUE;
                }
            }

            short delta_abs = qAbs(zDelta);
            if (delta_abs >= WHEEL_DELTA) {
                bool wheel_up_found = false;
                bool wheel_down_found = false;
                bool wheel_left_found = false;
                bool wheel_right_found = false;
                bool send_wheel_keys = false;

                QString keycodeString_WheelUp = MOUSE_WHEEL_UP_STR;
                QString keycodeString_WheelDown = MOUSE_WHEEL_DOWN_STR;
                QString keycodeString_WheelLeft = MOUSE_WHEEL_LEFT_STR;
                QString keycodeString_WheelRight = MOUSE_WHEEL_RIGHT_STR;
                /* Add extraInfo check for Multi InputDevice */
                if (extraInfo > INTERCEPTION_EXTRA_INFO && extraInfo <= (INTERCEPTION_EXTRA_INFO + INTERCEPTION_MAX_DEVICE)) {
                    InterceptionDevice device = extraInfo - INTERCEPTION_EXTRA_INFO;
                    if (interception_is_mouse(device)) {
                        keycodeString_WheelUp = QString("%1@%2").arg(keycodeString_WheelUp, QString::number(device - INTERCEPTION_MOUSE(0)));
                        keycodeString_WheelDown = QString("%1@%2").arg(keycodeString_WheelDown, QString::number(device - INTERCEPTION_MOUSE(0)));
                        keycodeString_WheelLeft = QString("%1@%2").arg(keycodeString_WheelLeft, QString::number(device - INTERCEPTION_MOUSE(0)));
                        keycodeString_WheelRight = QString("%1@%2").arg(keycodeString_WheelRight, QString::number(device - INTERCEPTION_MOUSE(0)));
                    }
                }

                int findindex = -1;
                int findWheelUpindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString_WheelUp);
                if (findWheelUpindex >=0){
                    wheel_up_found = true;
                }

                int findWheelDownindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString_WheelDown);
                if (findWheelDownindex >=0){
                    wheel_down_found = true;
                }

                int findWheelLeftindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString_WheelLeft);
                if (findWheelLeftindex >=0){
                    wheel_left_found = true;
                }

                int findWheelRightindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString_WheelRight);
                if (findWheelRightindex >=0){
                    wheel_right_found = true;
                }

                if (wheel_up_found || wheel_down_found || wheel_left_found || wheel_right_found) {
                    if (wheel_up_found && wParam == WM_MOUSEWHEEL && zDelta > 0) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[LowLevelMouseHookProc]" << "Real" << keycodeString_WheelUp << "-> Send Wheel Up Mapping Keys";
#endif
                        send_wheel_keys = true;
                        findindex = findWheelUpindex;
                    }
                    else if (wheel_down_found && wParam == WM_MOUSEWHEEL && zDelta < 0) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[LowLevelMouseHookProc]" << "Real" << keycodeString_WheelDown << "-> Send Wheel Down Mapping Keys";
#endif
                        send_wheel_keys = true;
                        findindex = findWheelDownindex;
                    }
                    else if (wheel_left_found && wParam == WM_MOUSEHWHEEL && zDelta < 0) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[LowLevelMouseHookProc]" << "Real" << keycodeString_WheelLeft << "-> Send Wheel Left Mapping Keys";
#endif
                        send_wheel_keys = true;
                        findindex = findWheelLeftindex;
                    }
                    else if (wheel_right_found && wParam == WM_MOUSEHWHEEL && zDelta > 0) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[LowLevelMouseHookProc]" << "Real" << keycodeString_WheelRight << "-> Send Wheel Right Mapping Keys";
#endif
                        send_wheel_keys = true;
                        findindex = findWheelRightindex;
                    }

                    if (send_wheel_keys) {
                        QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
                        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;

                        if (mappingKeyList.size() == 1 && mappingKeyList.constFirst() == KEY_BLOCKED_STR) {
#ifdef DEBUG_LOGOUT_ON
                            if (wheel_left_found) {
                                qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Operation Blocked ->" << keycodeString_WheelLeft;
                            }
                            else if (wheel_right_found) {
                                qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Operation Blocked ->" << keycodeString_WheelRight;
                            }
                            else if (wheel_up_found) {
                                qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Operation Blocked ->" << keycodeString_WheelUp;
                            }
                            else {
                                qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Operation Blocked ->" << keycodeString_WheelDown;
                            }
#endif
                            returnFlag = true;
                        }
                        else if (mappingKeyList.constFirst().startsWith(KEY2MOUSE_PREFIX) && mappingKeyList.size() == 1) {
                            returnFlag = true;
                        }
                        else {
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                            returnFlag = true;
                        }

                        bool PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                        if (PassThrough && returnFlag) {
                            returnFlag = false;
#ifdef DEBUG_LOGOUT_ON
                            qDebug().noquote().nospace() << "[LowLevelMouseHookProc]" << "PassThrough MouseWheel[" << original_key << "]";
#endif
                        }
                    }
                }
            }
        }
    }
#ifdef VIGEM_CLIENT_SUPPORT
    else if (wParam == WM_MOUSEMOVE) {
        if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
            return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
        }

#ifdef MOUSE_VERBOSE_LOG
        QString extraInfoStr = QString("0x%1").arg(QString::number(extraInfo, 16).toUpper(), 8, '0');
        qDebug() << "[LowLevelMouseHookProc]" << "Mouse Move -> X =" << pMouse->pt.x << ", Y =" << pMouse->pt.y << ", extraInfoStr =" << extraInfoStr;
#endif

        if (Interception_Worker::s_InterceptStart != true && extraInfo != VIRTUAL_MOUSE_MOVE && extraInfo != VIRTUAL_MOUSE_POINTCLICK) {
            // if (s_Mouse2vJoy_EnableState != MOUSE2VJOY_NONE) {
            if (!s_Mouse2vJoy_EnableStateMap.isEmpty()) {
                /* Add extraInfo check for Multi InputDevice */
                InterceptionDevice mouse_device = 0;
                int mouse_index = INITIAL_MOUSE_INDEX;
                if (extraInfo > INTERCEPTION_EXTRA_INFO && extraInfo <= (INTERCEPTION_EXTRA_INFO + INTERCEPTION_MAX_DEVICE)) {
                    InterceptionDevice device = extraInfo - INTERCEPTION_EXTRA_INFO;
                    if (interception_is_mouse(device)) {
                        mouse_device = device;
                        mouse_index = mouse_device - INTERCEPTION_MOUSE(0);
                    }
                }

                if (s_Mouse2vJoy_Hold) {
                    if (QKeyMapper::getvJoyLockCursorStatus()) {
                        return (LRESULT)TRUE;
                    }
                    else {
                        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
                    }
                }

                s_Mouse2vJoy_delta.rx() = pMouse->pt.x - s_Mouse2vJoy_prev.x();
                s_Mouse2vJoy_delta.ry() = pMouse->pt.y - s_Mouse2vJoy_prev.y();

                if (QKeyMapper::getvJoyLockCursorStatus()) {
                    returnFlag = true;
                }
                else {
                    s_Mouse2vJoy_prev.rx() = pMouse->pt.x;
                    s_Mouse2vJoy_prev.ry() = pMouse->pt.y;
                }
// #ifdef MOUSE_VERBOSE_LOG
//                 qDebug() << "[LowLevelMouseHookProc]" << "Mouse Move -> Delta X =" << s_Mouse2vJoy_delta.x() << ", Delta Y = " << s_Mouse2vJoy_delta.y();
// #endif
                emit QKeyMapper_Worker::getInstance()->onMouseMove_Signal(s_Mouse2vJoy_delta.x(), s_Mouse2vJoy_delta.y(), mouse_index);
            }
        }
    }
#endif

    if (true == returnFlag){
//#ifdef DEBUG_LOGOUT_ON
//        qDebug("LowLevelMouseHookProc() -> return TRUE");
//#endif
        return (LRESULT)TRUE;
    }
    else{
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }
}

int QKeyMapper_Worker::hookBurstAndLockProc(const QString &keycodeString, int keyupdown)
{
    int keyproc = KEY_PROC_NONE;
    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
    bool update_lockstatus = false;

    if (KEY_DOWN == keyupdown){
        if (false == pressedRealKeysList.contains(keycodeString)){
            if (findindex >=0 && true == QKeyMapper::KeyMappingDataList->at(findindex).Burst) {
                if (true == QKeyMapper::KeyMappingDataList->at(findindex).Lock) {
                    if (LOCK_STATE_LOCKOFF != QKeyMapper::KeyMappingDataList->at(findindex).LockState) {
                        keyproc = KEY_PROC_LOCK;
#ifdef DEBUG_LOGOUT_ON
                        QString debugmessage = QString("[hookBurstAndLockProc] Lock ON & Burst ON(KEY_DOWN) -> Key \"%1\" LockState is ON, skip startBurstTimer()!").arg(keycodeString);
                        qDebug().nospace().noquote() << debugmessage;
#endif
                    }
                    else {
                        emit QKeyMapper_Worker::getInstance()->startBurstKeyTimer_Signal(keycodeString, findindex, QKeyMapper::KeyMappingDataList);
                        if (QKeyMapper::KeyMappingDataList->at(findindex).PassThrough) {
                            keyproc = KEY_PROC_NONE;
                        }
                        else {
                            keyproc = KEY_PROC_BURST;
                        }
                    }
                }
                else {
                    emit QKeyMapper_Worker::getInstance()->startBurstKeyTimer_Signal(keycodeString, findindex, QKeyMapper::KeyMappingDataList);
                    if (QKeyMapper::KeyMappingDataList->at(findindex).PassThrough) {
                        keyproc = KEY_PROC_NONE;
                    }
                    else {
                        keyproc = KEY_PROC_BURST;
                    }
                }
            }
        }

        if (findindex >=0 && true == QKeyMapper::KeyMappingDataList->at(findindex).Lock) {
            if (true == pressedLockKeysMap.contains(keycodeString)){
                (*QKeyMapper::KeyMappingDataList)[findindex].LockState = LOCK_STATE_LOCKOFF;
                update_lockstatus = true;
                pressedLockKeysMap.remove(keycodeString);
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[hookBurstAndLockProc] Key \"%1\" KeyDown LockState = OFF").arg(keycodeString);
                qDebug().nospace().noquote() << debugmessage << ", pressedLockKeysMap -> " << pressedLockKeysMap;
#endif
            }
            else {
                QString original_key = (*QKeyMapper::KeyMappingDataList)[findindex].Original_Key;
                QStringList pure_mappingkeys = (*QKeyMapper::KeyMappingDataList)[findindex].Pure_MappingKeys;
                QString pure_originalkeyStr = QKeyMapper::getOriginalKeyStringWithoutSuffix(original_key);
                if (pure_mappingkeys.contains(pure_originalkeyStr)) {
                    (*QKeyMapper::KeyMappingDataList)[findindex].LockState = LOCK_STATE_LOCKON_PLUS;
                }
                else {
                    (*QKeyMapper::KeyMappingDataList)[findindex].LockState = LOCK_STATE_LOCKON;
                }
                update_lockstatus = true;
                pressedLockKeysMap.insert(keycodeString, findindex);
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[hookBurstAndLockProc] Key \"%1\" KeyDown LockState = ON").arg(keycodeString);
                qDebug().nospace().noquote() << debugmessage << ", pressedLockKeysMap -> " << pressedLockKeysMap;
#endif
            }
        }
    }
    else {  /* KEY_UP == keyupdown */
        QList<int> pressedLockedRowIndexList = pressedLockKeysMap.values();
        for (const int rowindex : std::as_const(pressedLockedRowIndexList)) {
            bool mappingkeys_unlock = (*QKeyMapper::KeyMappingDataList)[rowindex].MappingKeyUnlock;
            if (mappingkeys_unlock) {
                QStringList pure_mappingkeys = (*QKeyMapper::KeyMappingDataList)[rowindex].Pure_MappingKeys;
                if (pure_mappingkeys.contains(keycodeString)) {
                    QString locked_orikey = pressedLockKeysMap.key(rowindex);
                    if (pressedLockKeysMap.contains(locked_orikey)) {
                        if ((*QKeyMapper::KeyMappingDataList)[rowindex].LockState > LOCK_STATE_LOCKOFF) {
                            --(*QKeyMapper::KeyMappingDataList)[rowindex].LockState;
                        }
                        if ((*QKeyMapper::KeyMappingDataList)[rowindex].LockState == LOCK_STATE_LOCKOFF) {
                            update_lockstatus = true;
                            pressedLockKeysMap.remove(locked_orikey);

#ifdef DEBUG_LOGOUT_ON
                            QString debugmessage = QString("[hookBurstAndLockProc] Key \"%1\" contains in OriginalKey(%2), rowindex(%3), MappingKeys(%4), remove it").arg(keycodeString, locked_orikey).arg(rowindex).arg((*QKeyMapper::KeyMappingDataList)[rowindex].Mapping_Keys.constFirst());
                            qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << ", pressedLockKeysMap -> " << pressedLockKeysMap << "\033[0m";
#endif

                            // Stop burst timer if Lock+Burst is enabled
                            if ((*QKeyMapper::KeyMappingDataList)[rowindex].Burst) {
                                emit QKeyMapper_Worker::getInstance()->stopBurstKeyTimer_Signal(locked_orikey, rowindex, QKeyMapper::KeyMappingDataList);
#ifdef DEBUG_LOGOUT_ON
                                QString burstmessage = QString("[hookBurstAndLockProc] Stop burst timer for unlocked key \"%1\"").arg(locked_orikey);
                                qDebug().nospace().noquote() << "\033[1;32m" << burstmessage << "\033[0m";
#endif
                            }
                            else {
                                QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(rowindex).Mapping_Keys;
                                QString original_key = QKeyMapper::KeyMappingDataList->at(rowindex).Original_Key;
                                QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(rowindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                            }
                        }
                    }
                }
            }
        }

        if (true == pressedRealKeysList.contains(keycodeString)
            || true == pressedLongPressKeysList.contains(keycodeString)
            || true == pressedDoublePressKeysList.contains(keycodeString)){
            if (findindex >=0) {
                if (true == QKeyMapper::KeyMappingDataList->at(findindex).Lock) {
                    /* Lock ON &  Burst ON */
                    if (true == QKeyMapper::KeyMappingDataList->at(findindex).Burst) {
                        if (LOCK_STATE_LOCKOFF != QKeyMapper::KeyMappingDataList->at(findindex).LockState) {
                            keyproc = KEY_PROC_LOCK;
#ifdef DEBUG_LOGOUT_ON
                            QString debugmessage = QString("[hookBurstAndLockProc] Lock ON & Burst ON(KEY_UP) -> Key \"%1\" LockState is ON, skip stopBurstTimer()!").arg(keycodeString);
                            qDebug().nospace().noquote() << debugmessage;
#endif
                        }
                        else {
                            emit QKeyMapper_Worker::getInstance()->stopBurstKeyTimer_Signal(keycodeString, findindex, QKeyMapper::KeyMappingDataList);
                            if (QKeyMapper::KeyMappingDataList->at(findindex).PassThrough) {
                                keyproc = KEY_PROC_NONE;
                            }
                            else {
                                keyproc = KEY_PROC_BURST;
                            }
                        }

                        QString original_key = (*QKeyMapper::KeyMappingDataList)[findindex].Original_Key;
                        if (original_key.contains(SEPARATOR_LONGPRESS)) {
                            QStringList pure_mappingkeys = (*QKeyMapper::KeyMappingDataList)[findindex].Pure_MappingKeys;
                            QString pure_originalkeyStr = QKeyMapper::getOriginalKeyStringWithoutSuffix(original_key);
                            if (!pure_mappingkeys.contains(pure_originalkeyStr)) {
                                if (KEY_PROC_LOCK == keyproc) {
                                    keyproc = KEY_PROC_LOCK_PASSTHROUGH;
#ifdef DEBUG_LOGOUT_ON
                                    QString debugmessage = QString("[hookBurstAndLockProc] Key \"%1\" KEY_UP on longpress MappingKeys(%2), do not skip KeyUp!").arg(keycodeString, (*QKeyMapper::KeyMappingDataList)[findindex].Mapping_Keys.constFirst());
                                    qDebug().nospace().noquote() << debugmessage;
#endif
                                }
                                else {
                                    keyproc = KEY_PROC_PASSTHROUGH;
                                }
                            }
                        }
                    }
                    /* Lock ON &  Burst OFF */
                    else {
                        if (LOCK_STATE_LOCKOFF != QKeyMapper::KeyMappingDataList->at(findindex).LockState) {
                            keyproc = KEY_PROC_LOCK;
#ifdef DEBUG_LOGOUT_ON
                            QString debugmessage = QString("[hookBurstAndLockProc] Lock ON & Burst OFF -> Key \"%1\" LockState is ON, skip KeyUp!").arg(keycodeString);
                            qDebug().nospace().noquote() << debugmessage;
#endif
                        }

                        QString original_key = (*QKeyMapper::KeyMappingDataList)[findindex].Original_Key;
                        if (original_key.contains(SEPARATOR_LONGPRESS)) {
                            QStringList pure_mappingkeys = (*QKeyMapper::KeyMappingDataList)[findindex].Pure_MappingKeys;
                            QString pure_originalkeyStr = QKeyMapper::getOriginalKeyStringWithoutSuffix(original_key);
                            if (!pure_mappingkeys.contains(pure_originalkeyStr)) {
                                if (KEY_PROC_LOCK == keyproc) {
                                    keyproc = KEY_PROC_LOCK_PASSTHROUGH;
#ifdef DEBUG_LOGOUT_ON
                                    QString debugmessage = QString("[hookBurstAndLockProc] Key \"%1\" KEY_UP on longpress MappingKeys(%2), do not skip KeyUp!").arg(keycodeString, (*QKeyMapper::KeyMappingDataList)[findindex].Mapping_Keys.constFirst());
                                    qDebug().nospace().noquote() << debugmessage;
#endif
                                }
                                else {
                                    keyproc = KEY_PROC_PASSTHROUGH;
                                }
                            }
                        }
                    }
                }
                else {
                    /* Lock OFF &  Burst ON */
                    if (true == QKeyMapper::KeyMappingDataList->at(findindex).Burst) {
                        emit QKeyMapper_Worker::getInstance()->stopBurstKeyTimer_Signal(keycodeString, findindex, QKeyMapper::KeyMappingDataList);
                        if (QKeyMapper::KeyMappingDataList->at(findindex).PassThrough) {
                            keyproc = KEY_PROC_NONE;
                        }
                        else {
                            keyproc = KEY_PROC_BURST;
                        }
                    }
                    /* Lock OFF &  Burst OFF do nothing */
                }
            }
        }
    }

    if (update_lockstatus) {
        updateLockStatus();
    }

    return keyproc;
}

int QKeyMapper_Worker::updatePressedRealKeysList(const QString &keycodeString, int keyupdown)
{
    int intercept = KEY_INTERCEPT_NONE;

    QString keycodeString_RemoveMultiInput = getKeycodeStringRemoveMultiInput(keycodeString);
    if (KEY_DOWN == keyupdown){
        if (false == pressedRealKeysList.contains(keycodeString)){
            pressedRealKeysList.append(keycodeString);
            sendLongPressTimers(keycodeString);
            intercept = sendDoublePressTimers(keycodeString);
        }
        if (false == pressedRealKeysListRemoveMultiInput.contains(keycodeString_RemoveMultiInput)){
            pressedRealKeysListRemoveMultiInput.append(keycodeString_RemoveMultiInput);

            if (QKeyMapper::KEYMAP_IDLE == QKeyMapper::getInstance()->m_KeyMapStatus) {
                emit QKeyMapper::getInstance()->updateKeyLineEditWithRealKeyListChanged_Signal(keycodeString_RemoveMultiInput, keyupdown);
            }
        }
    }
    else {  /* KEY_UP == keyupdown */
        if (true == pressedRealKeysList.contains(keycodeString)){
            pressedRealKeysList.removeAll(keycodeString);
            clearLongPressTimer(keycodeString);
            int intercept_longpress = longPressKeyProc(keycodeString, KEY_UP);
            int intercept_doublepress = doublePressKeyProc(keycodeString, KEY_UP);
            Q_UNUSED(intercept_longpress);
            // if (intercept_longpress != KEY_INTERCEPT_NONE) {
            //     intercept = intercept_longpress;
            // } else
            if (intercept_doublepress != KEY_INTERCEPT_NONE) {
                intercept = intercept_doublepress;
            }
        }
        if (true == pressedRealKeysListRemoveMultiInput.contains(keycodeString_RemoveMultiInput)){
            pressedRealKeysListRemoveMultiInput.removeAll(keycodeString_RemoveMultiInput);

            if (QKeyMapper::KEYMAP_IDLE == QKeyMapper::getInstance()->m_KeyMapStatus) {
                emit QKeyMapper::getInstance()->updateKeyLineEditWithRealKeyListChanged_Signal(keycodeString_RemoveMultiInput, keyupdown);
            }
        }
    }

    if (s_KeyRecording) {
        bool recorded = updateRecordKeyList(keycodeString_RemoveMultiInput, keyupdown);
        if (recorded) {
            intercept = KEY_INTERCEPT_BLOCK_KEY_RECORD;
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[pressedRealKeysList]" << (keyupdown == KEY_DOWN?"KEY_DOWN":"KEY_UP") << ": Current Pressed RealKeys ->" << pressedRealKeysList;
    qDebug() << "[pressedRealKeysListRemoveMultiInput]" << (keyupdown == KEY_DOWN?"KEY_DOWN":"KEY_UP") << ": Current Pressed RealKeys(without MultiInput) ->" << pressedRealKeysListRemoveMultiInput;
#endif

    return intercept;
}

void QKeyMapper_Worker::keyRecordStart()
{
    recordElapsedTimer.invalidate();
    recordKeyList.clear();
    recordMappingKeysList.clear();
    s_KeyRecording = true;
}

void QKeyMapper_Worker::keyRecordStop()
{
    s_KeyRecording = false;
    recordElapsedTimer.invalidate();
    // recordKeyList.clear();
    // recordMappingKeysList.clear();
}

void QKeyMapper_Worker::collectRecordKeysList(bool clicked)
{
    if (clicked) {
        if (recordKeyList.size() >= 2) {
            RecordKeyData prev_last_record_keydata = recordKeyList.at(recordKeyList.size() - 2);
            RecordKeyData last_record_keydata = recordKeyList.constLast();
            if (prev_last_record_keydata.keystring == MOUSE_L_STR
                && prev_last_record_keydata.input_type == INPUT_KEY_DOWN
                && last_record_keydata.keystring == MOUSE_L_STR
                && last_record_keydata.input_type == INPUT_KEY_UP) {
                recordKeyList.removeLast();
                recordKeyList.removeLast();
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "\033[1;34m[collectRecordKeysList]" << "recordKeyList remove record stop Mouse-L click.\033[0m";
#endif
            }
        }

        if (recordMappingKeysList.size() >= 2) {
            QString record_stop_click_down_str = QString("%1%2").arg(PREFIX_SEND_DOWN, MOUSE_L_STR);
            QString record_stop_click_up_str = QString("%1%2").arg(PREFIX_SEND_UP, MOUSE_L_STR);
            if (recordMappingKeysList.at(recordMappingKeysList.size() - 2).startsWith(record_stop_click_down_str)
                && recordMappingKeysList.constLast() == record_stop_click_up_str) {
                recordMappingKeysList.removeLast();
                recordMappingKeysList.removeLast();
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "\033[1;34m[collectRecordKeysList]" << "recordMappingKeysList remove record stop Mouse-L click.\033[0m";
#endif
            }
        }
    }
    else {
        if (!recordKeyList.isEmpty()) {
            RecordKeyData last_record_keydata = recordKeyList.constLast();
            if (last_record_keydata.keystring == KEY_RECORD_STOP_STR
                && last_record_keydata.input_type == INPUT_KEY_DOWN) {
                recordKeyList.removeLast();
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[collectRecordKeysList] recordKeyList remove record stop \"%1\" KEY_DOWN.").arg(KEY_RECORD_STOP_STR);
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            }
        }

        if (!recordMappingKeysList.isEmpty()) {
            QString record_stop_key_str = QString("%1%2").arg(PREFIX_SEND_DOWN, KEY_RECORD_STOP_STR);
            if (recordMappingKeysList.constLast() == record_stop_key_str) {
                recordMappingKeysList.removeLast();
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[collectRecordKeysList] recordMappingKeysList remove record stop \"%1\" KEY_DOWN.").arg(KEY_RECORD_STOP_STR);
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            }
        }
    }

    if (!recordMappingKeysList.isEmpty()) {
        if (recordMappingKeysList.constLast().contains(SEPARATOR_WAITTIME)) {
            QString lastRecordMappingKey = recordMappingKeysList.constLast();

            int index = lastRecordMappingKey.indexOf(SEPARATOR_WAITTIME);
            if (index != -1) {
                lastRecordMappingKey = lastRecordMappingKey.left(index);
                recordMappingKeysList[recordMappingKeysList.size() - 1] = lastRecordMappingKey;
            }
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "\033[1;34m[collectRecordKeysList]" << "recordKeyList ->" << recordKeyList << "\033[0m";
    qDebug() << "\033[1;34m[collectRecordKeysList]" << "recordMappingKeysList ->" << recordMappingKeysList << "\033[0m";
#endif
}

bool QKeyMapper_Worker::updateRecordKeyList(const QString &keycodeString, int input_type)
{
    bool input_recorded = false;
    bool skip_record_startstop_key = false;
    if (recordKeyList.isEmpty()
        && keycodeString == KEY_RECORD_START_STR
        && input_type == INPUT_KEY_UP) {
        skip_record_startstop_key = true;
    }
    else if (keycodeString == KEY_RECORD_STOP_STR) {
        skip_record_startstop_key = true;
    }

    if (!skip_record_startstop_key) {
        RecordKeyData record_keydata;
        qint64 elapsed_time = 0;

        if (recordElapsedTimer.isValid()) {
            elapsed_time = recordElapsedTimer.elapsed();
        }
        else {
            recordElapsedTimer.start();
            elapsed_time = 0;
        }

        if (recordKeyList.isEmpty()) {
            elapsed_time = 0;
        }

        record_keydata.keystring = keycodeString;
        record_keydata.input_type = input_type;
        record_keydata.elapsed_time = elapsed_time;

        recordKeyList.append(record_keydata);

        int recordlist_size = recordKeyList.size();
        QString prefix_keyupdown_str;
        if (record_keydata.input_type == INPUT_KEY_DOWN) {
            prefix_keyupdown_str = PREFIX_SEND_DOWN;
        }
        else if (record_keydata.input_type == INPUT_KEY_UP) {
            prefix_keyupdown_str = PREFIX_SEND_UP;
        }
        QString mappingKeyString = prefix_keyupdown_str + record_keydata.keystring;
        int wait_time = 0;
        if (recordlist_size == 1) {
            recordMappingKeysList.clear();
        }
        else { /* recordlist_size > 1 */
            RecordKeyData prev_record_keydata = recordKeyList.at(recordlist_size - 2);
            wait_time = record_keydata.elapsed_time - prev_record_keydata.elapsed_time;
            if (wait_time > MAPPING_WAITTIME_MAX) {
                wait_time = MAPPING_WAITTIME_MAX;
            }

            if (wait_time > 0) {
                QString wait_time_str = QString("%1%2").arg(SEPARATOR_WAITTIME).arg(wait_time);
                recordMappingKeysList[recordMappingKeysList.size() - 1].append(wait_time_str);
            }
        }

        recordMappingKeysList.append(mappingKeyString);
        emit QKeyRecord::getInstance()->updateKeyRecordLineEdit_Signal(false);
        input_recorded = true;

#ifdef DEBUG_LOGOUT_ON
        QString debugmessage = QString("[updateRecordKeyList] WaitTime:%1 -> \"%2\"").arg(wait_time).arg(mappingKeyString);
        qDebug().noquote().nospace() << debugmessage;
#endif

        if (keycodeString == MOUSE_L_STR) {
            input_recorded = false;
        }
    }

    return input_recorded;
}

bool QKeyMapper_Worker::detectDisplaySwitchKey(const QString &keycodeString, int keyupdown)
{
    bool detected = false;
    bool passthrough = false;
    QString displayswitchKey = QKeyMapper::s_WindowSwitchKeyString;
    if (displayswitchKey.startsWith(PREFIX_PASSTHROUGH)) {
        passthrough = true;
        displayswitchKey.remove(0, 1);
    }
    QStringList keys = displayswitchKey.split(SEPARATOR_PLUS);
    bool allKeysPressed = true;
    bool combinationKey = (keys.size() > 1) ? true : false;
    QList<int> pressedCombinationRealKeysOrder;

    for (const QString &key : std::as_const(keys))
    {
        int index = pressedRealKeysListRemoveMultiInput.indexOf(key);
        if (index < 0)
        {
            allKeysPressed = false;
            break;
        }
        else {
            pressedCombinationRealKeysOrder.append(index);
        }
    }

    if (combinationKey) {
        bool keyorder_increasing = isKeyOrderIncreasing(pressedCombinationRealKeysOrder);
        if (!keyorder_increasing) {
            allKeysPressed = false;
        }
    }

    if (KEY_DOWN == keyupdown && allKeysPressed && displayswitchKey.contains(keycodeString))
    {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[detectDisplaySwitchKey]" << "DisplaySwitchKey Activated ->" << displayswitchKey;
#endif
        emit QKeyMapper::getInstance()->HotKeyDisplaySwitchActivated_Signal(displayswitchKey);
        if (passthrough) {
            detected = false;
        }
        else {
            detected = true;
        }
    }

    return detected;
}

bool QKeyMapper_Worker::detectMappingSwitchKey(const QString &keycodeString, int keyupdown)
{
    bool detected = false;

    if (QKeyMapper::KEYMAP_IDLE == QKeyMapper::getInstance()->m_KeyMapStatus) {
        detected = detectMappingStartKey(keycodeString, keyupdown);
    }
    else {
        detected = detectMappingStopKey(keycodeString, keyupdown);
    }

    return detected;
}

bool QKeyMapper_Worker::detectMappingStartKey(const QString &keycodeString, int keyupdown)
{
    bool detected = false;
    bool passthrough = false;
    QString mappingswitchKey = QKeyMapper::s_MappingStartKeyString;
    if (mappingswitchKey.startsWith(PREFIX_PASSTHROUGH)) {
        passthrough = true;
        mappingswitchKey.remove(0, 1);
    }
    QStringList keys = mappingswitchKey.split(SEPARATOR_PLUS);
    bool allKeysPressed = true;
    bool combinationKey = (keys.size() > 1) ? true : false;
    QList<int> pressedCombinationRealKeysOrder;

    for (const QString &key : std::as_const(keys))
    {
        int index = pressedRealKeysListRemoveMultiInput.indexOf(key);
        if (index < 0)
        {
            allKeysPressed = false;
            break;
        }
        else {
            pressedCombinationRealKeysOrder.append(index);
        }
    }

    if (combinationKey) {
        bool keyorder_increasing = isKeyOrderIncreasing(pressedCombinationRealKeysOrder);
        if (!keyorder_increasing) {
            allKeysPressed = false;
        }
    }

    if (KEY_DOWN == keyupdown && allKeysPressed && mappingswitchKey.contains(keycodeString))
    {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[detectMappingSwitchKey]" << "MappingStartKey Activated ->" << mappingswitchKey;
#endif
        emit QKeyMapper::getInstance()->HotKeyMappingStart_Signal(mappingswitchKey);
        if (passthrough) {
            detected = false;
        }
        else {
            detected = true;
        }
    }

    return detected;
}

bool QKeyMapper_Worker::detectMappingStopKey(const QString &keycodeString, int keyupdown)
{
    bool detected = false;
    bool passthrough = false;
    QString mappingswitchKey = QKeyMapper::s_MappingStopKeyString;
    if (mappingswitchKey.startsWith(PREFIX_PASSTHROUGH)) {
        passthrough = true;
        mappingswitchKey.remove(0, 1);
    }
    QStringList keys = mappingswitchKey.split(SEPARATOR_PLUS);
    bool allKeysPressed = true;
    bool combinationKey = (keys.size() > 1) ? true : false;
    QList<int> pressedCombinationRealKeysOrder;

    for (const QString &key : std::as_const(keys))
    {
        int index = pressedRealKeysListRemoveMultiInput.indexOf(key);
        if (index < 0)
        {
            allKeysPressed = false;
            break;
        }
        else {
            pressedCombinationRealKeysOrder.append(index);
        }
    }

    if (combinationKey) {
        bool keyorder_increasing = isKeyOrderIncreasing(pressedCombinationRealKeysOrder);
        if (!keyorder_increasing) {
            allKeysPressed = false;
        }
    }

    if (KEY_DOWN == keyupdown && allKeysPressed && mappingswitchKey.contains(keycodeString))
    {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[detectMappingSwitchKey]" << "MappingStopKey Activated ->" << mappingswitchKey;
#endif
        emit QKeyMapper::getInstance()->HotKeyMappingStop_Signal(mappingswitchKey);
        if (passthrough) {
            detected = false;
        }
        else {
            detected = true;
        }
    }

    return detected;
}

bool QKeyMapper_Worker::detectMappingTableTabHotkeys(const QString &keycodeString, int keyupdown)
{
    bool detected = false;
    const QStringList switchTabHotkeys = QKeyMapper::s_MappingTableTabHotkeyMap.keys();

    for (const QString& tabHotkey : switchTabHotkeys)
    {
        QString keyToCheck = tabHotkey;
        QStringList keys = keyToCheck.split(SEPARATOR_PLUS);
        bool allKeysPressed = true;
        bool combinationKey = (keys.size() > 1) ? true : false;
        QList<int> pressedCombinationRealKeysOrder;

        for (const QString &key : std::as_const(keys))
        {
            int index = pressedRealKeysListRemoveMultiInput.indexOf(key);
            if (index < 0)
            {
                allKeysPressed = false;
                break;
            }
            else {
                pressedCombinationRealKeysOrder.append(index);
            }
        }

        if (combinationKey) {
            bool keyorder_increasing = isKeyOrderIncreasing(pressedCombinationRealKeysOrder);
            if (!keyorder_increasing) {
                allKeysPressed = false;
            }
        }

        if (KEY_DOWN == keyupdown && allKeysPressed && keyToCheck.contains(keycodeString))
        {
            emit QKeyMapper::getInstance()->HotKeyMappingTableSwitchTab_Signal(keyToCheck);
            detected = true;
            int tabindex_toswitch = QKeyMapper::tabIndexToSwitchByTabHotkey(keyToCheck);
            if (0 <= tabindex_toswitch &&  tabindex_toswitch < QKeyMapper::s_KeyMappingTabInfoList.size()) {
                QString ori_tabhotkeystring = QKeyMapper::s_KeyMappingTabInfoList.at(tabindex_toswitch).TabHotkey;
                if (ori_tabhotkeystring.startsWith(PREFIX_PASSTHROUGH)) {
                    detected = false;
                }
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[detectMappingTableTabHotkeys]" << "TabHotkey detected -> " << keyToCheck << ", PassThrough:" << (!detected);
#endif
            break;
        }
    }

    return detected;
}

int QKeyMapper_Worker::detectCombinationKeys(const QString &keycodeString, int keyupdown)
{
    int intercept = KEY_INTERCEPT_NONE;
    bool PassThrough = false;

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[detectCombinationKeys]" << "Current combinationOriginalKeysList ->" << combinationOriginalKeysList;
#endif

    for (const QString &combinationkey : std::as_const(combinationOriginalKeysList))
    {
        QStringList keys = combinationkey.split(SEPARATOR_PLUS);
        bool allKeysPressed = true;
        QList<int> pressedCombinationRealKeysOrder;

        for (const QString &key : std::as_const(keys))
        {
            if (key.contains('@')) {
                int index = pressedRealKeysList.indexOf(key);
                if (index < 0)
                {
                    allKeysPressed = false;
                    break;
                }
                else {
                    pressedCombinationRealKeysOrder.append(index);
                }
            }
            else {
                int index = pressedRealKeysListRemoveMultiInput.indexOf(key);
                if (index < 0)
                {
                    allKeysPressed = false;
                    break;
                }
                else {
                    pressedCombinationRealKeysOrder.append(index);
                }
            }
        }

        if (KEY_DOWN == keyupdown && allKeysPressed
            && (combinationkey.contains(keycodeString)
               || combinationkey.contains(getKeycodeStringRemoveMultiInput(keycodeString))))
        {
            bool combinationkey_matched = true;
            int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(combinationkey);
            bool checkcombkeyorder = QKeyMapper::KeyMappingDataList->at(findindex).CheckCombKeyOrder;
            if (checkcombkeyorder) {
                bool keyorder_increasing = isKeyOrderIncreasing(pressedCombinationRealKeysOrder);
                if (!keyorder_increasing) {
                    combinationkey_matched = false;
                }
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[detectCombinationKeys] CombinationKey(\"%1\") KEY_DOWN detected, isKeyOrderIncreasing(%2), pressedCombinationRealKeysOrder -> ").arg(combinationkey, (keyorder_increasing?"true":"false"));
                qDebug().nospace().noquote() << debugmessage << pressedCombinationRealKeysOrder;
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[detectCombinationKeys] CombinationKey(\"%1\") KEY_DOWN detected, pressedCombinationRealKeysOrder -> ").arg(combinationkey);
                qDebug().nospace().noquote() << debugmessage << pressedCombinationRealKeysOrder;
#endif
            }

            if (combinationkey_matched) {
                CombinationKeyProc(findindex, combinationkey, KEY_DOWN);
                if (findindex >= 0) {
                    PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                    if (PassThrough) {
                        intercept = KEY_INTERCEPT_PASSTHROUGH;
                    }
                    else {
                        intercept = KEY_INTERCEPT_BLOCK;
                    }

//                     pressedCombinationRealKeysList.append(pressedCombinationRealKeys);
// #ifdef DEBUG_LOGOUT_ON
//                     qDebug() << "[detectCombinationKeys]" << "CombinationKey Down pressedCombinationRealKeys ->" << pressedCombinationRealKeys;
// #endif
                }
                break;
            }
            else {
                continue;
            }
        }
        else if (pressedRealKeysList.contains(combinationkey)) {
            if (KEY_UP == keyupdown
                && (combinationkey.contains(keycodeString)
                    || combinationkey.contains(getKeycodeStringRemoveMultiInput(keycodeString))))
            {
                if (false == allKeysPressed) {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[detectCombinationKeys] CombinationKey(\"%1\") KEY_UP detected").arg(combinationkey);
                    qDebug().nospace().noquote() << debugmessage;
#endif
                    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(combinationkey);
                    CombinationKeyProc(findindex, combinationkey, KEY_UP);
                    if (findindex >= 0) {
                        PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                        if (PassThrough) {
                            intercept = KEY_INTERCEPT_PASSTHROUGH;
                        }
                        else {
                            if (keycodeString.startsWith(MOUSE_WHEEL_STR)) {
                                intercept = KEY_INTERCEPT_BLOCK;
                            }
                            else {
                                intercept = KEY_INTERCEPT_BLOCK_COMBINATIONKEYUP;
                            }
                        }

//                         pressedCombinationRealKeysList.removeAll(getKeycodeStringRemoveMultiInput(keycodeString));
// #ifdef DEBUG_LOGOUT_ON
//                         qDebug() << "[detectCombinationKeys]" << "CombinationKey Up pressedCombinationRealKeys ->" << pressedCombinationRealKeys;
// #endif
                    }
                    break;
                }
            }
        }
    }

    return intercept;
}

bool QKeyMapper_Worker::isKeyOrderIncreasing(const QList<int> &keyorder)
{
    for (int i = 1; i < keyorder.size(); ++i) {
        if (keyorder[i] <= keyorder[i - 1]) {
            return false;
        }
    }
    return true;
}

void QKeyMapper_Worker::CombinationKeyProc(int rowindex, const QString &keycodeString, int keyupdown)
{
    bool returnFlag = false;
    int findindex = rowindex;
    returnFlag = (hookBurstAndLockProc(keycodeString, keyupdown) != KEY_PROC_NONE);
    int intercept = updatePressedRealKeysList(keycodeString, keyupdown);

    if (intercept == KEY_INTERCEPT_BLOCK) {
        return;
    }

    if (false == returnFlag) {
        if (findindex >=0){
            QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
            QStringList mappingKey_KeyUpList = QKeyMapper::KeyMappingDataList->at(findindex).MappingKeys_KeyUp;
            QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
            QString firstmappingkey = mappingKeyList.constFirst();
            int mappingkeylist_size = mappingKeyList.size();

            // Check for BLOCKED and FUNC_PREFIX mappings for combination keys
            if (mappingkeylist_size == 1 && firstmappingkey == KEY_BLOCKED_STR) {
#ifdef DEBUG_LOGOUT_ON
                if (KEY_DOWN == keyupdown){
                    qDebug() << "[CombinationKeyProc]" << "CombinationKey KEY_DOWN Blocked ->" << original_key;
                }
                else {
                    qDebug() << "[CombinationKeyProc]" << "CombinationKey KEY_UP Blocked ->" << original_key;
                }
#endif
                return; // Block the combination key
            }
            else if (firstmappingkey.startsWith(FUNC_PREFIX) && mappingkeylist_size == 1) {
#ifdef DEBUG_LOGOUT_ON
                if (KEY_DOWN == keyupdown){
                    qDebug() << "[CombinationKeyProc]" << "Function CombinationKey KEY_DOWN ->" << firstmappingkey;
                }
                else {
                    qDebug() << "[CombinationKeyProc]" << "Function CombinationKey KEY_UP ->" << firstmappingkey;
                }
#endif
                if (KEY_DOWN == keyupdown){
                    emit QKeyMapper_Worker::getInstance()->doFunctionMappingProc_Signal(firstmappingkey);
                }
                return; // Function mapping handled
            }

            int SendTiming = QKeyMapper::KeyMappingDataList->at(findindex).SendTiming;
            bool KeySeqHoldDown = QKeyMapper::KeyMappingDataList->at(findindex).KeySeqHoldDown;
            if (KEY_DOWN == keyupdown){
                if (SENDTIMING_KEYDOWN == SendTiming) {
                    const Qt::KeyboardModifiers modifiers_arg = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
                    bool releasemodifier = releaseKeyboardModifiers(modifiers_arg, original_key, mappingKeyList);
                    int sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
                    if (releasemodifier) {
                        sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
                    }
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL, sendvirtualkey_state);
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                }
                else if (SENDTIMING_KEYUP == SendTiming) {
                    /* KEY_DOWN & SENDTIMING_KEYUP == SendTiming -> do nothing */
                }
                else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                    const Qt::KeyboardModifiers modifiers_arg = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
                    bool releasemodifier = releaseKeyboardModifiers(modifiers_arg, original_key, mappingKeyList);
                    int sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
                    if (releasemodifier) {
                        sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
                    }
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL, sendvirtualkey_state);
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                }
                else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                    const Qt::KeyboardModifiers modifiers_arg = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
                    bool releasemodifier = releaseKeyboardModifiers(modifiers_arg, original_key, mappingKeyList);
                    int sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
                    if (releasemodifier) {
                        sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
                    }
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL, sendvirtualkey_state);
                }
                else { /* SENDTIMING_NORMAL == SendTiming */
                    const Qt::KeyboardModifiers modifiers_arg = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
                    bool releasemodifier = releaseKeyboardModifiers(modifiers_arg, original_key, mappingKeyList);
                    int sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
                    if (releasemodifier) {
                        sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
                    }
                    /* Add for KeySequenceHoldDown >>> */
                    if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_KEYSEQ_HOLDDOWN, sendvirtualkey_state);
                    }
                    else {
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL, sendvirtualkey_state);
                    }
                    /* Add for KeySequenceHoldDown <<< */
                }
            }
            else { /* KEY_UP == keyupdown */
                if (SENDTIMING_KEYDOWN == SendTiming) {
                    /* KEY_UP & SENDTIMING_KEYDOWN == SendTiming -> do nothing */
                }
                else if (SENDTIMING_KEYUP == SendTiming) {
                    const Qt::KeyboardModifiers modifiers_arg = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
                    bool releasemodifier = releaseKeyboardModifiers(modifiers_arg, original_key, mappingKey_KeyUpList);
                    int sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
                    if (releasemodifier) {
                        sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
                    }
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL, sendvirtualkey_state);
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                }
                else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                    const Qt::KeyboardModifiers modifiers_arg = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
                    bool releasemodifier = releaseKeyboardModifiers(modifiers_arg, original_key, mappingKey_KeyUpList);
                    int sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
                    if (releasemodifier) {
                        sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
                    }
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL, sendvirtualkey_state);
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                }
                else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);

                    const Qt::KeyboardModifiers modifiers_arg = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
                    bool releasemodifier = releaseKeyboardModifiers(modifiers_arg, original_key, mappingKey_KeyUpList);
                    int sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
                    if (releasemodifier) {
                        sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
                    }
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL, sendvirtualkey_state);
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                }
                else { /* SENDTIMING_NORMAL == SendTiming */
                    /* Add for KeySequenceHoldDown >>> */
                    if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                    }
                    else {
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                    }
                    /* Add for KeySequenceHoldDown <<< */
                }
            }
        }
    }
}

bool QKeyMapper_Worker::releaseKeyboardModifiers(const Qt::KeyboardModifiers &modifiers, QString &original_key, const QStringList mappingkeyslist)
{
    if (mappingkeyslist.size() == 1 && mappingkeyslist.constFirst() == KEY_BLOCKED_STR) {
        return false;
    }

    bool releasemodifier = false;
    QStringList pressedKeyboardModifiersList;
    if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0 && modifiers.testFlag(Qt::ShiftModifier)) {
        pressedKeyboardModifiersList.append("L-Shift");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0 && modifiers.testFlag(Qt::ShiftModifier)) {
        pressedKeyboardModifiersList.append("R-Shift");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0 && modifiers.testFlag(Qt::ControlModifier)) {
        pressedKeyboardModifiersList.append("L-Ctrl");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0 && modifiers.testFlag(Qt::ControlModifier)) {
        pressedKeyboardModifiersList.append("R-Ctrl");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0 && modifiers.testFlag(Qt::AltModifier)) {
        pressedKeyboardModifiersList.append("L-Alt");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0 && modifiers.testFlag(Qt::AltModifier)) {
        pressedKeyboardModifiersList.append("R-Alt");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 && modifiers.testFlag(Qt::MetaModifier)) {
        pressedKeyboardModifiersList.append("L-Win");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_RWIN) & 0x8000) != 0 && modifiers.testFlag(Qt::MetaModifier)) {
        pressedKeyboardModifiersList.append("R-Win");
        releasemodifier = true;
    }

    SendInputTaskController &controller = SendInputTask::s_GlobalSendInputTaskController;
    controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
    for (const QString &modifierstr : std::as_const(pressedKeyboardModifiersList)) {
        QStringList mappingKeyList = QStringList() << modifierstr;
        // QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_MODIFIERS);
        QKeyMapper_Worker::getInstance()->sendInputKeys(-1, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL, controller);
    }
    controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;

    if (modifiers.testFlag(Qt::AltModifier)) {
        bool shift_ctrl_modifier_status = false;
        if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0
            || (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0
            || (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0
            || (GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0) {
            shift_ctrl_modifier_status = true;
        }
        QStringList mappingKeyList;
        if ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) {
            mappingKeyList = QStringList() << "L-Alt";
        }
        else if ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0) {
            mappingKeyList = QStringList() << "R-Alt";
        }

        if (!mappingKeyList.isEmpty() && !shift_ctrl_modifier_status) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[releaseKeyboardModifiers]" << "AltModifier Special Release!";
#endif

            releasemodifier = true;
            SendInputTaskController &controller = SendInputTask::s_GlobalSendInputTaskController;
            controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
            // QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_MODIFIERS);
            // QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_MODIFIERS);
            QKeyMapper_Worker::getInstance()->sendInputKeys(-1, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL, controller);
            QKeyMapper_Worker::getInstance()->sendInputKeys(-1, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL, controller);
            controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
        }
    }

    return releasemodifier;
}

bool QKeyMapper_Worker::releaseKeyboardModifiersDirect(const Qt::KeyboardModifiers &modifiers)
{
    bool releasemodifier = false;
    QStringList pressedKeyboardModifiersList;
    if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0 && modifiers.testFlag(Qt::ShiftModifier)) {
        pressedKeyboardModifiersList.append("L-Shift");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0 && modifiers.testFlag(Qt::ShiftModifier)) {
        pressedKeyboardModifiersList.append("R-Shift");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0 && modifiers.testFlag(Qt::ControlModifier)) {
        pressedKeyboardModifiersList.append("L-Ctrl");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0 && modifiers.testFlag(Qt::ControlModifier)) {
        pressedKeyboardModifiersList.append("R-Ctrl");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0 && modifiers.testFlag(Qt::AltModifier)) {
        pressedKeyboardModifiersList.append("L-Alt");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0 && modifiers.testFlag(Qt::AltModifier)) {
        pressedKeyboardModifiersList.append("R-Alt");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 && modifiers.testFlag(Qt::MetaModifier)) {
        pressedKeyboardModifiersList.append("L-Win");
        releasemodifier = true;
    }
    if ((GetAsyncKeyState(VK_RWIN) & 0x8000) != 0 && modifiers.testFlag(Qt::MetaModifier)) {
        pressedKeyboardModifiersList.append("R-Win");
        releasemodifier = true;
    }

    SendInputTaskController &controller = SendInputTask::s_GlobalSendInputTaskController;
    controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
    for (const QString &modifierstr : std::as_const(pressedKeyboardModifiersList)) {
        QStringList mappingKeyList = QStringList() << modifierstr;
        QString original_key = QString(KEYBOARD_MODIFIERS);
        QKeyMapper_Worker::getInstance()->sendInputKeys(-1, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL, controller);
    }
    controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;

    return releasemodifier;
}

void QKeyMapper_Worker::startBurstKeyTimer(const QString &burstKey, int mappingIndex, QList<MAP_KEYDATA> *keyMappingDataList)
{
    if (mappingIndex < 0 || keyMappingDataList == Q_NULLPTR || mappingIndex >= keyMappingDataList->size()) {
        return;
    }

    QMutexLocker locker(&s_BurstKeyTimerMutex);

    int burstpressTime = keyMappingDataList->at(mappingIndex).BurstPressTime;
    int burstreleaseTime = keyMappingDataList->at(mappingIndex).BurstReleaseTime;
    int burstTime = burstpressTime + burstreleaseTime;
    QKeyMapper_Worker *instance = QKeyMapper_Worker::getInstance();

    QTimer* burstkeypressTimer;
    if (s_BurstKeyPressTimerMap.contains(burstKey)) {
        burstkeypressTimer = s_BurstKeyPressTimerMap.value(burstKey);
    }
    else {
        burstkeypressTimer = new QTimer();
        burstkeypressTimer->setTimerType(Qt::PreciseTimer);
        burstkeypressTimer->setSingleShot(true);
        QObject::connect(burstkeypressTimer, &QTimer::timeout, instance, [burstKey, mappingIndex, keyMappingDataList]() {
            onBurstKeyPressTimeOut(burstKey, mappingIndex, keyMappingDataList);
        });
        s_BurstKeyPressTimerMap.insert(burstKey, burstkeypressTimer);
    }
    /* First mappingkey need to send on startBurstKeyTimer */
    sendBurstKeyDown(mappingIndex, true, keyMappingDataList);
    burstkeypressTimer->start(burstpressTime);

    QTimer* burstkeyTimer;
    if (s_BurstKeyTimerMap.contains(burstKey)) {
        burstkeyTimer = s_BurstKeyTimerMap.value(burstKey);
    }
    else {
        burstkeyTimer = new QTimer();
        burstkeyTimer->setTimerType(Qt::PreciseTimer);
        QObject::connect(burstkeyTimer, &QTimer::timeout, instance, [burstKey, mappingIndex, keyMappingDataList]() {
            onBurstKeyTimeOut(burstKey, mappingIndex, keyMappingDataList);
        });
        s_BurstKeyTimerMap.insert(burstKey, burstkeyTimer);
    }
    burstkeyTimer->start(burstTime);
}

void QKeyMapper_Worker::stopBurstKeyTimer(const QString &burstKey, int mappingIndex, QList<MAP_KEYDATA> *keyMappingDataList)
{
    QMutexLocker locker(&s_BurstKeyTimerMutex);
    Q_UNUSED(mappingIndex);
    Q_UNUSED(keyMappingDataList);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[stopBurstKeyTimer] Key [" << burstKey << "], MappingIndex =" << mappingIndex;
#endif

    if (s_BurstKeyPressTimerMap.contains(burstKey)) {
        QTimer* timer = s_BurstKeyPressTimerMap.value(burstKey);
        timer->stop();
        delete timer;
        s_BurstKeyPressTimerMap.remove(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[stopBurstKeyTimer] s_BurstKeyPressTimerMap contains [" << burstKey << "], BurstKeyPressTimer stopped & removed.";
#endif
    }

    if (s_BurstKeyTimerMap.contains(burstKey)) {
        QTimer* timer = s_BurstKeyTimerMap.value(burstKey);
        timer->stop();
        delete timer;
        s_BurstKeyTimerMap.remove(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[stopBurstKeyTimer] Key [" << burstKey << "], BurstKeyTimer stopped & removed.";
#endif
    }

    sendBurstKeyUp(mappingIndex, true, keyMappingDataList);
}

void QKeyMapper_Worker::stopBurstKeyTimerForce(const QString &burstKey, int mappingIndex)
{
    QMutexLocker locker(&s_BurstKeyTimerMutex);
    Q_UNUSED(mappingIndex);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[stopBurstKeyTimerForce] Key [" << burstKey << "], MappingIndex =" << mappingIndex;
#endif

    if (s_BurstKeyPressTimerMap.contains(burstKey)) {
        QTimer* timer = s_BurstKeyPressTimerMap.value(burstKey);
        timer->stop();
        delete timer;
        s_BurstKeyPressTimerMap.remove(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[stopBurstKeyTimerForce] s_BurstKeyPressTimerMap contains [" << burstKey << "], BurstKeyPressTimer stopped & removed.";
#endif
    }

    if (s_BurstKeyTimerMap.contains(burstKey)) {
        QTimer* timer = s_BurstKeyTimerMap.value(burstKey);
        timer->stop();
        delete timer;
        s_BurstKeyTimerMap.remove(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[stopBurstKeyTimerForce] Key [" << burstKey << "], BurstKeyTimer stopped & removed.";
#endif
    }

    sendBurstKeyUpForce(mappingIndex);
}

void QKeyMapper_Worker::resendRealKeyCodeOnStop(int rowindex, bool restart, QList<MAP_KEYDATA> *keyMappingDataListToCheck)
{
    if (pressedRealKeysListRemoveMultiInput.isEmpty()) {
        return;
    }

    QList<MAP_KEYDATA> *KeyMappingDataList_ForResend = QKeyMapper::KeyMappingDataList;

    if (restart) {
        if (QKeyMapper::lastKeyMappingDataList != Q_NULLPTR) {
            KeyMappingDataList_ForResend = QKeyMapper::lastKeyMappingDataList;
        }
    }

    bool hook_proc_stopped = false;
    if (HOOKPROC_STATE_STOPPED == s_AtomicHookProcState
        || HOOKPROC_STATE_STOPPING == s_AtomicHookProcState) {
        hook_proc_stopped = true;
    }
    QStringList pressedRealKeysListToCheck = pressedRealKeysListRemoveMultiInput;
    if (!hook_proc_stopped) {
        QList<MAP_KEYDATA> *KeyMappingDataList_ToCheck = KeyMappingDataList_ForResend;
        if (restart && keyMappingDataListToCheck != Q_NULLPTR) {
            KeyMappingDataList_ToCheck = keyMappingDataListToCheck;
        }

        QStringList currentBlockedKeysList = collectCertainMappingDataListBlockedKeysList(KeyMappingDataList_ToCheck);
        for (const QString &blockedKey : std::as_const(currentBlockedKeysList)) {
            pressedRealKeysListToCheck.removeAll(blockedKey);
        }

        QStringList pressedRealKeysListToCheckCopy = pressedRealKeysListToCheck;
        for (const QString &realkey : pressedRealKeysListToCheckCopy) {
            int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(realkey, KeyMappingDataList_ToCheck);
            if (findindex >= 0 && !KeyMappingDataList_ToCheck->at(findindex).PassThrough) {
                pressedRealKeysListToCheck.removeAll(realkey);
            }
        }
    }

    if (restart) {
        /* resendRealKeyCodeOnStop on mapping restart do not resend mapping tab switch hotkey */
        QString tabHotkey = QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).TabHotkey;
        if (tabHotkey.startsWith(PREFIX_PASSTHROUGH)) {
            tabHotkey.remove(0, 1);
        }
        pressedRealKeysListToCheck.removeAll(tabHotkey);
    }
    else {
        /* resendRealKeyCodeOnStop on mapping stop do not resend mapping stop hotkey */
        QString mappingStopKeyStr = QKeyMapper::s_MappingStopKeyString;
        if (mappingStopKeyStr.startsWith(PREFIX_PASSTHROUGH)) {
            mappingStopKeyStr.remove(0, 1);
        }
        pressedRealKeysListToCheck.removeAll(mappingStopKeyStr);
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[resendRealKeyCodeOnStop] pressedRealKeysListToCheck -> " << pressedRealKeysListToCheck;
#endif

    if (pressedRealKeysListToCheck.isEmpty()) {
        return;
    }

    QStringList keyListToCheck = KeyMappingDataList_ForResend->at(rowindex).Pure_MappingKeys;
    if (hook_proc_stopped || restart) {
        QStringList pure_originalkeylist = KeyMappingDataList_ForResend->at(rowindex).Pure_OriginalKeys;
        keyListToCheck = keyListToCheck + pure_originalkeylist;
    }
    keyListToCheck.removeDuplicates();

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[resendRealKeyCodeOnStop] keyListToCheck -> " << keyListToCheck;
#endif

    if (keyListToCheck.isEmpty()) {
        return;
    }

    for (const QString &keycodeString : std::as_const(keyListToCheck)) {
        if (pressedRealKeysListToCheck.contains(keycodeString)) {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("[resendRealKeyCodeOnStop] RealKey \"%1\" is still pressed down on BurstKey stop, resend \"%2\" KEY_DOWN.").arg(keycodeString, keycodeString);
            qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            QKeyMapper_Worker::getInstance()->sendSpecialVirtualKey(keycodeString, KEY_DOWN);
        }
    }
}

void QKeyMapper_Worker::collectBlockedKeysList()
{
    blockedKeysList.clear();

    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList)) {
        if (keymapdata.Mapping_Keys.size() == 1 && keymapdata.Mapping_Keys.constFirst().contains(KEY_BLOCKED_STR)) {
            blockedKeysList.append(keymapdata.Original_Key);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[collectBlockedKeysList]" << "blockedKeysList ->" << blockedKeysList;
#endif
}

QStringList QKeyMapper_Worker::collectCertainMappingDataListBlockedKeysList(QList<MAP_KEYDATA> *keyMappingDataListToCheck)
{
    QStringList collected_blockedKeysList;

    for (const MAP_KEYDATA &keymapdata : std::as_const(*keyMappingDataListToCheck)) {
        if (keymapdata.Mapping_Keys.size() == 1 && keymapdata.Mapping_Keys.constFirst().contains(KEY_BLOCKED_STR)) {
            collected_blockedKeysList.append(keymapdata.Original_Key);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[collectCertainMappingDataListBlockedKeysList]" << "collected_blockedKeysList ->" << collected_blockedKeysList;
#endif

    return collected_blockedKeysList;
}

void QKeyMapper_Worker::collectCombinationOriginalKeysList()
{
    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList))
    {
        if (keymapdata.Original_Key.contains(SEPARATOR_PLUS))
        {
            QString combinationkey = QKeyMapper::getOriginalKeyStringWithoutSuffix(keymapdata.Original_Key);
            if (!combinationkey.isEmpty()) {
                combinationOriginalKeysList.append(combinationkey);
            }
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[collectCombinationOriginalKeysList]" << "combinationOriginalKeysList ->" << combinationOriginalKeysList;
#endif
}

void QKeyMapper_Worker::collectLongPressOriginalKeysMap()
{
    static QRegularExpression regex(R"(^(.+)⏲(\d+)$)");
    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList))
    {
        QRegularExpressionMatch match = regex.match(keymapdata.Original_Key);
        if (match.hasMatch()) {
            QString original_key = match.captured(1);
            QString longPressTimeString = match.captured(2);
            int longpresstime = longPressTimeString.toInt();
            if (longPressOriginalKeysMap[original_key].contains(longpresstime) == false) {
                longPressOriginalKeysMap[original_key].append(longpresstime);
            }
        }
    }

    // Sort the QList<int> values in longPressOriginalKeysMap using range-based for loop
    for (QList<int> &valueList : longPressOriginalKeysMap) {
        std::sort(valueList.begin(), valueList.end());
    }

#ifdef DEBUG_LOGOUT_ON
    if (longPressOriginalKeysMap.isEmpty() == false) {
        qDebug() << "[collectLongPressOriginalKeysMap]" << "longPressOriginalKeysMap ->" << longPressOriginalKeysMap;
    }
#endif
}

void QKeyMapper_Worker::sendLongPressTimers(const QString &keycodeString)
{
    QString keycodeString_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keycodeString);

    if (longPressOriginalKeysMap.contains(keycodeString)) {
        QList<int> timeoutValueList = longPressOriginalKeysMap.value(keycodeString);

        for (int timeout : std::as_const(timeoutValueList)) {
            QString keycodeStringWithPressTime = keycodeString + QString(SEPARATOR_LONGPRESS) + QString::number(timeout);
            if (s_longPressTimerMap.contains(keycodeStringWithPressTime)) {
                s_longPressTimerMap[keycodeStringWithPressTime]->start(timeout);
            }
            else {
                QKeyMapper_Worker *instance = QKeyMapper_Worker::getInstance();
                QTimer* timer = new QTimer();
                timer->setTimerType(Qt::PreciseTimer);
                timer->setSingleShot(true);
                QObject::connect(timer, &QTimer::timeout, instance, [keycodeStringWithPressTime]() {
                    onLongPressTimeOut(keycodeStringWithPressTime);
                });
                timer->start(timeout);
                s_longPressTimerMap.insert(keycodeStringWithPressTime, timer);
            }
        }
    }
    else if (longPressOriginalKeysMap.contains(keycodeString_RemoveMultiInput)) {
        QList<int> timeoutValueList = longPressOriginalKeysMap.value(keycodeString_RemoveMultiInput);

        for (int timeout : std::as_const(timeoutValueList)) {
            QString keycodeStringWithPressTime = keycodeString_RemoveMultiInput + QString(SEPARATOR_LONGPRESS) + QString::number(timeout);
            if (s_longPressTimerMap.contains(keycodeStringWithPressTime)) {
                s_longPressTimerMap[keycodeStringWithPressTime]->start(timeout);
            }
            else {
                QKeyMapper_Worker *instance = QKeyMapper_Worker::getInstance();
                QTimer* timer = new QTimer();
                timer->setTimerType(Qt::PreciseTimer);
                timer->setSingleShot(true);
                QObject::connect(timer, &QTimer::timeout, instance, [keycodeStringWithPressTime]() {
                    onLongPressTimeOut(keycodeStringWithPressTime);
                });
                timer->start(timeout);
                s_longPressTimerMap.insert(keycodeStringWithPressTime, timer);
            }
        }
    }
}

void QKeyMapper_Worker::clearLongPressTimer(const QString &keycodeString)
{
    if (s_longPressTimerMap.isEmpty()) {
        return;
    }

    QString keycodeString_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keycodeString);
    QStringList removeKeys;
    QStringList longpressKeys = s_longPressTimerMap.keys();
    for (const QString &key : std::as_const(longpressKeys)) {
        QString keyWithoutTime = key.split(SEPARATOR_LONGPRESS).first();
        if (keyWithoutTime == keycodeString
            || keyWithoutTime == keycodeString_RemoveMultiInput) {
            QTimer *timer = s_longPressTimerMap.value(key);
            timer->stop();
            delete timer;
            removeKeys.append(key);
        }
    }

    for (const QString &key : removeKeys) {
        s_longPressTimerMap.remove(key);
    }
}

void QKeyMapper_Worker::removeLongPressTimerOnTimeout(const QString &keycodeStringWithPressTime)
{
    if (s_longPressTimerMap.contains(keycodeStringWithPressTime)) {
        QTimer *timer = s_longPressTimerMap.value(keycodeStringWithPressTime);
        timer->stop();
        delete timer;
        s_longPressTimerMap.remove(keycodeStringWithPressTime);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[removeLongPressTimerOnTimeout]" << "Remove [" << keycodeStringWithPressTime << "]";
        qDebug() << "[removeLongPressTimerOnTimeout]" << "Current s_longPressTimerMap ->" << s_longPressTimerMap;
#endif
    }
}

void QKeyMapper_Worker::clearAllLongPressTimers(void)
{
    if (s_longPressTimerMap.isEmpty()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    QString threadIdStr = QString("0x%1").arg(QString::number((qulonglong)QThread::currentThreadId(), 16).toUpper(), 8, '0');
    qDebug().nospace().noquote() << "[clearAllLongPressTimers] LongPressTimer ClearAll -> ThreadName:" << QThread::currentThread()->objectName() << ", ThreadID:" << threadIdStr;
#endif

    QList<QTimer*> longpressTimers = s_longPressTimerMap.values();
    for (QTimer *timer : std::as_const(longpressTimers)) {
        timer->stop();
        delete timer;
    }
    s_longPressTimerMap.clear();
}

int QKeyMapper_Worker::longPressKeyProc(const QString &keycodeString, int keyupdown)
{
    int intercept = KEY_INTERCEPT_NONE;

    if (HOOKPROC_STATE_STARTED != s_AtomicHookProcState) {
        return intercept;
    }

    if (KEY_DOWN == keyupdown) {
        int keyproc = hookBurstAndLockProc(keycodeString, keyupdown);
        int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);

        if (findindex >= 0) {
            if (false == pressedLongPressKeysList.contains(keycodeString)) {
                pressedLongPressKeysList.append(keycodeString);

                if (KEY_PROC_NONE == keyproc) {
                    QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
                    QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
                    if (original_key.contains(SEPARATOR_PLUS)) {
                        const Qt::KeyboardModifiers modifiers_arg = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
                        releaseKeyboardModifiers(modifiers_arg, original_key);
                    }
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                }

                bool PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                if (PassThrough) {
                    intercept = KEY_INTERCEPT_PASSTHROUGH;
                }
                else {
                    intercept = KEY_INTERCEPT_BLOCK;
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[longPressKeyProc]" << keycodeString << "KEY_DOWN pressedLongPressKeysList ->" << pressedLongPressKeysList;
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[longPressKeyProc]" << "pressedLongPressKeysList KEY_DOWN already contains [" << keycodeString << "]";
#endif
            }
        }
    }
    else {
        if (pressedLongPressKeysList.isEmpty()) {
            return KEY_INTERCEPT_NONE;
        }

        QStringList releaseKeys;
        QString keycodeString_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keycodeString);
        for (const QString &key : std::as_const(pressedLongPressKeysList)) {
            if (key.startsWith(keycodeString)
                || key.startsWith(keycodeString_RemoveMultiInput)) {
                int keyproc = hookBurstAndLockProc(key, keyupdown);
                int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(key);
                releaseKeys.append(key);

                if (findindex >=0){
                    if (keyproc != KEY_PROC_LOCK && keyproc != KEY_PROC_LOCK_PASSTHROUGH) {
                        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
                        QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                    }

                    bool PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                    if (PassThrough
                        || keyproc == KEY_PROC_LOCK_PASSTHROUGH
                        || keyproc == KEY_PROC_PASSTHROUGH) {
                        intercept = KEY_INTERCEPT_PASSTHROUGH;
                    }
                    else {
                        intercept = KEY_INTERCEPT_BLOCK;
                    }
                }
            }
        }

        for (const QString &key : releaseKeys) {
            pressedLongPressKeysList.removeAll(key);
        }

#ifdef DEBUG_LOGOUT_ON
        if (!releaseKeys.isEmpty()) {
            qDebug() << "[longPressKeyProc]" << releaseKeys << "KEY_UP pressedLongPressKeysList ->" << pressedLongPressKeysList;
        }
#endif
    }

    return intercept;
}

void QKeyMapper_Worker::collectDoublePressOriginalKeysMap()
{
    int keymapdataindex = 0;
    static QRegularExpression regex(R"(^(.+✖)(\d+)$)");
    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList))
    {
        QRegularExpressionMatch match = regex.match(keymapdata.Original_Key);
        if (match.hasMatch()) {
            QString original_key = match.captured(1);
            if (doublePressOriginalKeysMap.contains(original_key) == false) {
                doublePressOriginalKeysMap.insert(original_key, keymapdataindex);
            }
        }
        keymapdataindex += 1;
    }

#ifdef DEBUG_LOGOUT_ON
    if (doublePressOriginalKeysMap.isEmpty() == false) {
        qDebug() << "[collectDoublePressOriginalKeysMap]" << "doublePressOriginalKeysMap ->" << doublePressOriginalKeysMap;
    }
#endif
}

QHash<QString, int> QKeyMapper_Worker::currentDoublePressOriginalKeysMap()
{
    QHash<QString, int> currentDoublePressOriginalKeysMap;

    int keymapdataindex = 0;
    static QRegularExpression regex(R"(^(.+✖)(\d+)$)");
    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList))
    {
        QRegularExpressionMatch match = regex.match(keymapdata.Original_Key);
        if (match.hasMatch()) {
            QString original_key = match.captured(1);
            if (currentDoublePressOriginalKeysMap.contains(original_key) == false) {
                currentDoublePressOriginalKeysMap.insert(original_key, keymapdataindex);
            }
        }
        keymapdataindex += 1;
    }

#ifdef DEBUG_LOGOUT_ON
    if (currentDoublePressOriginalKeysMap.isEmpty()) {
        qDebug() << "[currentDoublePressOriginalKeysMap]" << "currentDoublePressOriginalKeysMap is Empty.";
    }
    else {
        qDebug() << "[currentDoublePressOriginalKeysMap]" << "currentDoublePressOriginalKeysMap ->" << currentDoublePressOriginalKeysMap;
    }
#endif

    return currentDoublePressOriginalKeysMap;
}

int QKeyMapper_Worker::sendDoublePressTimers(const QString &keycodeString)
{
    int intercept = KEY_INTERCEPT_NONE;
    int timeout = 0;
    QString keycodeString_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keycodeString);
    QString keycodeString_doublepress = keycodeString + "✖";
    QString keycodeString_RemoveMultiInput_doublepress = keycodeString_RemoveMultiInput + "✖";

    if (doublePressOriginalKeysMap.contains(keycodeString_doublepress)) {
        int findindex = doublePressOriginalKeysMap.value(keycodeString_doublepress, -1);
        if (findindex >= 0){
            QString original_key_withDoublePressTime = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
            static QRegularExpression regex(R"(^(.+)✖(\d+)$)");
            QRegularExpressionMatch match = regex.match(original_key_withDoublePressTime);
            if (match.hasMatch()) {
                // QString original_key = match.captured(1);
                QString doublePressTimeString = match.captured(2);
                timeout = doublePressTimeString.toInt();
            }

            if (timeout == 0) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[sendDoublePressTimers]" << "Invalid doublepress time of [" << original_key_withDoublePressTime << "]";
#endif
                return KEY_INTERCEPT_NONE;
            }
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[sendDoublePressTimers]" << "Invalid findindex of [" << keycodeString_doublepress << "]";
#endif
            return KEY_INTERCEPT_NONE;
        }

        if (s_doublePressTimerMap.contains(keycodeString_doublepress)) {
            clearDoublePressTimer(keycodeString_doublepress);
            intercept = doublePressKeyProc(keycodeString_doublepress, KEY_DOWN);
        }
        else {
            QKeyMapper_Worker *instance = QKeyMapper_Worker::getInstance();
            QTimer* timer = new QTimer();
            timer->setTimerType(Qt::PreciseTimer);
            timer->setSingleShot(true);
            QObject::connect(timer, &QTimer::timeout, instance, [keycodeString_doublepress]() {
                onDoublePressTimeOut(keycodeString_doublepress);
            });
            timer->start(timeout);
            s_doublePressTimerMap.insert(keycodeString_doublepress, timer);
        }
    }
    else if (doublePressOriginalKeysMap.contains(keycodeString_RemoveMultiInput_doublepress)) {
        int findindex = doublePressOriginalKeysMap.value(keycodeString_RemoveMultiInput_doublepress, -1);
        if (findindex >= 0){
            QString original_key_withDoublePressTime = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
            static QRegularExpression regex(R"(^(.+)✖(\d+)$)");
            QRegularExpressionMatch match = regex.match(original_key_withDoublePressTime);
            if (match.hasMatch()) {
                // QString original_key = match.captured(1);
                QString doublePressTimeString = match.captured(2);
                timeout = doublePressTimeString.toInt();
            }

            if (timeout == 0) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[sendDoublePressTimers]" << "Invalid doublepress time of [" << original_key_withDoublePressTime << "]";
#endif
                return KEY_INTERCEPT_NONE;
            }
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[sendDoublePressTimers]" << "Invalid findindex of [" << keycodeString_RemoveMultiInput_doublepress << "]";
#endif
            return KEY_INTERCEPT_NONE;
        }

        if (s_doublePressTimerMap.contains(keycodeString_RemoveMultiInput_doublepress)) {
            clearDoublePressTimer(keycodeString_RemoveMultiInput_doublepress);
            intercept = doublePressKeyProc(keycodeString_RemoveMultiInput_doublepress, KEY_DOWN);
        }
        else {
            QKeyMapper_Worker *instance = QKeyMapper_Worker::getInstance();
            QTimer* timer = new QTimer();
            timer->setTimerType(Qt::PreciseTimer);
            timer->setSingleShot(true);
            QObject::connect(timer, &QTimer::timeout, instance, [keycodeString_RemoveMultiInput_doublepress]() {
                onDoublePressTimeOut(keycodeString_RemoveMultiInput_doublepress);
            });
            timer->start(timeout);
            s_doublePressTimerMap.insert(keycodeString_RemoveMultiInput_doublepress, timer);
        }
    }

    return intercept;
}

void QKeyMapper_Worker::clearDoublePressTimer(const QString &keycodeString)
{
    if (s_doublePressTimerMap.isEmpty()) {
        return;
    }

    QStringList removeKeys;
    QStringList doublepressKeys = s_doublePressTimerMap.keys();
    for (const QString &key : std::as_const(doublepressKeys)) {
        if (key.contains(keycodeString)) {
            QTimer *timer = s_doublePressTimerMap.value(key);
            timer->stop();
            delete timer;
            removeKeys.append(key);
        }
    }

    for (const QString &key : removeKeys) {
        s_doublePressTimerMap.remove(key);
    }
}

void QKeyMapper_Worker::removeDoublePressTimerOnTimeout(const QString &keycodeString)
{
    if (s_doublePressTimerMap.contains(keycodeString)) {
        QTimer *timer = s_doublePressTimerMap.value(keycodeString);
        timer->stop();
        delete timer;
        s_doublePressTimerMap.remove(keycodeString);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[removeDoublePressTimerOnTimeout]" << "Remove [" << keycodeString << "]";
        qDebug() << "[removeDoublePressTimerOnTimeout]" << "Current s_doublePressTimerMap ->" << s_doublePressTimerMap;
#endif
    }
}

void QKeyMapper_Worker::clearAllDoublePressTimers()
{
    if (s_doublePressTimerMap.isEmpty()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    QString threadIdStr = QString("0x%1").arg(QString::number((qulonglong)QThread::currentThreadId(), 16).toUpper(), 8, '0');
    qDebug().nospace().noquote() << "[clearAllDoublePressTimers] DoublePressTimer ClearAll -> ThreadName:" << QThread::currentThread()->objectName() << ", ThreadID:" << threadIdStr;
#endif

    QList<QTimer*> doublepressTimers = s_doublePressTimerMap.values();
    for (QTimer *timer : std::as_const(doublepressTimers)) {
        timer->stop();
        delete timer;
    }
    s_doublePressTimerMap.clear();
}

int QKeyMapper_Worker::doublePressKeyProc(const QString &keycodeString, int keyupdown)
{
    int intercept = KEY_INTERCEPT_NONE;

    if (KEY_DOWN == keyupdown) {
        int keyproc = hookBurstAndLockProc(keycodeString, keyupdown);
        int findindex = doublePressOriginalKeysMap.value(keycodeString, -1);
        if (findindex >= 0) {
            if (false == pressedDoublePressKeysList.contains(keycodeString)) {
                pressedDoublePressKeysList.append(keycodeString);

                if (KEY_PROC_NONE == keyproc) {
                    QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
                    QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
                    bool releasemodifier = false;
                    int sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
                    if (original_key.contains(SEPARATOR_PLUS)) {
                        const Qt::KeyboardModifiers modifiers_arg = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
                        releasemodifier = releaseKeyboardModifiers(modifiers_arg, original_key);
                        if (releasemodifier) {
                            sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
                        }
                    }
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL, sendvirtualkey_state);
                }

                bool PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                if (PassThrough) {
                    intercept = KEY_INTERCEPT_PASSTHROUGH;
                }
                else {
                    intercept = KEY_INTERCEPT_BLOCK;
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[doublePressKeyProc]" << keycodeString << "KEY_DOWN pressedDoublePressKeysList ->" << pressedDoublePressKeysList;
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[doublePressKeyProc]" << "pressedDoublePressKeysList KEY_DOWN already contains [" << keycodeString << "]";
#endif
            }
        }
    }
    else {
        if (pressedDoublePressKeysList.isEmpty()) {
            return KEY_INTERCEPT_NONE;
        }

        QStringList releaseKeys;
        QString keycodeString_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keycodeString);
        for (const QString &doublepress_key : std::as_const(pressedDoublePressKeysList)) {
            QString key = doublepress_key;
            key.chop(1);
            if (key == keycodeString
                || key == keycodeString_RemoveMultiInput) {
                int keyproc = hookBurstAndLockProc(doublepress_key, keyupdown);
                int findindex = doublePressOriginalKeysMap.value(doublepress_key, -1);
                releaseKeys.append(doublepress_key);

                if (findindex >= 0) {
                    if (keyproc != KEY_PROC_LOCK) {
                        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
                        QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                    }

                    bool PassThrough = QKeyMapper::KeyMappingDataList->at(findindex).PassThrough;
                    if (PassThrough) {
                        intercept = KEY_INTERCEPT_PASSTHROUGH;
                    }
                    else {
                        intercept = KEY_INTERCEPT_BLOCK;
                    }
                }
            }
        }

        for (const QString &key : releaseKeys) {
            pressedDoublePressKeysList.removeAll(key);
        }

#ifdef DEBUG_LOGOUT_ON
        if (!releaseKeys.isEmpty()) {
            qDebug() << "[doublePressKeyProc]" << releaseKeys << "KEY_UP pressedDoublePressKeysList ->" << pressedDoublePressKeysList;
        }
#endif
    }

    return intercept;
}

QString QKeyMapper_Worker::getWindowsKeyName(uint virtualKeyCode)
{
    QString keynameStr;
    LONG lParam = MapVirtualKey(virtualKeyCode, MAPVK_VK_TO_VSC) << 16;
    // Set the extended key flag for certain keys
    if (virtualKeyCode == VK_RCONTROL || virtualKeyCode == VK_RMENU ||
        virtualKeyCode == VK_LWIN || virtualKeyCode == VK_RWIN ||
        virtualKeyCode == VK_VOLUME_MUTE || virtualKeyCode == VK_VOLUME_DOWN ||
        virtualKeyCode == VK_VOLUME_UP || virtualKeyCode == VK_MEDIA_NEXT_TRACK ||
        virtualKeyCode == VK_MEDIA_PREV_TRACK || virtualKeyCode == VK_MEDIA_STOP ||
        virtualKeyCode == VK_MEDIA_PLAY_PAUSE || virtualKeyCode == VK_BROWSER_BACK ||
        virtualKeyCode == VK_BROWSER_FORWARD || virtualKeyCode == VK_BROWSER_REFRESH ||
        virtualKeyCode == VK_BROWSER_STOP || virtualKeyCode == VK_BROWSER_SEARCH ||
        virtualKeyCode == VK_BROWSER_FAVORITES || virtualKeyCode == VK_BROWSER_HOME ||
        virtualKeyCode == VK_LAUNCH_MAIL || virtualKeyCode == VK_LAUNCH_MEDIA_SELECT) {
        lParam |= 1 << 24;
    }
    TCHAR keyName[50];
    memset(keyName, 0x00, sizeof(keyName));
    if (GetKeyNameText(lParam, keyName, sizeof(keyName) / sizeof(TCHAR)) > 0) {
        keynameStr = QString::fromWCharArray(keyName);
    } else {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[getWindowsKeyName] Failed to get keyname[0x%02X]!", virtualKeyCode);
#endif
    }

    return keynameStr;
}

QString QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(const QString &keycodeString)
{
    static QRegularExpression regex("@\\d$");
    QString result = keycodeString;
    result.remove(regex);
    return result;
}

void QKeyMapper_Worker::breakAllRunningKeySequence()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[breakAllRunningKeySequence] Current Running KeySequence, s_runningKeySequenceOrikeyList -> " << s_runningKeySequenceOrikeyList;
#endif

    for (const QString &keyseq_orikey : std::as_const(s_runningKeySequenceOrikeyList)) {
        SendInputTaskController *keyseq_break_controller = Q_NULLPTR;

        if (SendInputTask::s_SendInputTaskControllerMap.contains(keyseq_orikey)) {
            keyseq_break_controller = &SendInputTask::s_SendInputTaskControllerMap[keyseq_orikey];
        }

        if (keyseq_break_controller != Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("\033[1;34m[breakAllRunningKeySequence] OriginalKey(%1) Running KeySequence breaked, task_stop_flag = INPUTSTOP_KEYSEQ\033[0m").arg(keyseq_orikey);
            qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            keyseq_break_controller->task_threadpool->clear();
            *keyseq_break_controller->task_stop_flag = INPUTSTOP_KEYSEQ;
            keyseq_break_controller->task_stop_condition->wakeAll();
        }
    }
}

ParsedCommand QKeyMapper_Worker::parseUserInput(const QString &input)
{
    ParsedCommand result;
    QString str = input.trimmed();

    // 1. Extract WorkingDir="..."
    static QRegularExpression reWorkDir(
        "WorkingDir=\"([^\"]+)\"",
        QRegularExpression::CaseInsensitiveOption
    );
    QRegularExpressionMatch mDir = reWorkDir.match(str);
    if (mDir.hasMatch()) {
        QString workdir = mDir.captured(1);
        if (!workdir.isEmpty() && QFileInfo::exists(workdir) && QFileInfo(workdir).isDir()) {
            result.workDir = workdir;
        }
        str.remove(mDir.captured(0)); // Remove this part from the command line
    }

    // 2. Extract ShowOption=Max|Min|Hide
    static QRegularExpression reShowOpt(
        R"(ShowOption=(\w+))",
        QRegularExpression::CaseInsensitiveOption
    );
    QRegularExpressionMatch mShow = reShowOpt.match(str);
    if (mShow.hasMatch()) {
        QString opt = mShow.captured(1).toLower();
        if (opt == "max") {
            result.showCmd = SW_MAXIMIZE;
        } else if (opt == "min") {
            result.showCmd = SW_MINIMIZE;
        } else if (opt == "hide") {
            result.showCmd = SW_HIDE;
        }
        str.remove(mShow.captured(0));
    }

    // 3. The remaining part is treated as cmdLine
    result.cmdLine = str.trimmed();

    return result;
}

bool QKeyMapper_Worker::runCommand(const QString &cmdLine,
                                   bool runWait,
                                   const QString &workDir,
                                   int showCmd,
                                   const QString &verb)
{
    // Prepare STARTUPINFO for CreateProcess
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = static_cast<WORD>(showCmd);

    PROCESS_INFORMATION pi;
    std::wstring cmd = cmdLine.toStdWString(); // Persist command line
    std::wstring wdir = workDir.isEmpty() ? L"" : workDir.toStdWString();

    // Try CreateProcessW first (direct executable launch)
    if (CreateProcessW(
            nullptr,                        // Application name
            cmd.data(),                     // Command line (must be writable)
            nullptr, nullptr,               // Security attributes
            FALSE,                          // No handle inheritance
            0,                              // Creation flags
            nullptr,                        // Environment
            wdir.empty() ? nullptr : wdir.c_str(), // Working directory
            &si, &pi))
    {
        if (runWait) {
            WaitForSingleObject(pi.hProcess, INFINITE);
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    // If CreateProcess fails, try ShellExecuteExW
    SHELLEXECUTEINFOW sei = { sizeof(sei) };

    // Persist verb / file / directory
    std::wstring wverb = verb.isEmpty() ? L"" : verb.toStdWString();
    std::wstring wfile = cmdLine.toStdWString(); // Must be persisted
    std::wstring wdir2 = wdir; // Already persisted

    // Base fMask
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    // If it is a CLSID or Shell Namespace object, add SEE_MASK_INVOKEIDLIST
    if (!wfile.empty() && wfile.rfind(L"::", 0) == 0) {
        sei.fMask |= SEE_MASK_INVOKEIDLIST;
    }

    sei.lpVerb      = wverb.empty() ? nullptr : wverb.c_str();
    sei.lpFile      = wfile.c_str();
    sei.lpDirectory = wdir2.empty() ? nullptr : wdir2.c_str();
    sei.nShow       = showCmd;

    if (ShellExecuteExW(&sei)) {
        if (runWait && sei.hProcess) {
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }
        return true;
    }

    // Both methods failed
    return false;
}

void QKeyMapper_Worker::onLongPressTimeOut(const QString keycodeStringWithPressTime)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onLongPressTimeOut] keycodeStringWithPressTime ->" << keycodeStringWithPressTime;
#endif

    (void)longPressKeyProc(keycodeStringWithPressTime, KEY_DOWN);

    removeLongPressTimerOnTimeout(keycodeStringWithPressTime);
}

void QKeyMapper_Worker::onDoublePressTimeOut(const QString keycodeString)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onDoublePressTimeOut] keycodeString ->" << keycodeString;
#endif

    removeDoublePressTimerOnTimeout(keycodeString);
}

void QKeyMapper_Worker::onBurstKeyPressTimeOut(const QString burstKey, int mappingIndex, QList<MAP_KEYDATA> *keyMappingDataList)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[onBurstKeyPressTimeOut] Key [" << burstKey << "], MappingIndex =" << mappingIndex;
#endif
    QMutexLocker locker(&s_BurstKeyTimerMutex);
    if (s_BurstKeyPressTimerMap.contains(burstKey)) {
        QTimer* timer = s_BurstKeyPressTimerMap.value(burstKey);
        timer->stop();
        delete timer;
        s_BurstKeyPressTimerMap.remove(burstKey);
        sendBurstKeyUp(mappingIndex, false, keyMappingDataList);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
    qWarning().nospace().noquote() << "[QKeyMapper_Worker::onBurstKeyPressTimeOut]" << " Could not find Key [" << burstKey << "], MappingIndex = " << mappingIndex << ", in s_BurstKeyPressTimerMap!!!";
#endif
    }
}

void QKeyMapper_Worker::onBurstKeyTimeOut(const QString burstKey, int mappingIndex, QList<MAP_KEYDATA> *keyMappingDataList)
{
    if (keyMappingDataList == Q_NULLPTR || mappingIndex < 0 || mappingIndex >= keyMappingDataList->size()) {
#ifdef DEBUG_LOGOUT_ON
        qWarning().nospace().noquote() << "[QKeyMapper_Worker::onBurstKeyTimeOut]" << " Invalid keyMappingDataList pointer or mappingIndex out of bounds!!! Key [" << burstKey << "], MappingIndex = " << mappingIndex;
#endif
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[onBurstKeyTimeOut] Key [" << burstKey << "], MappingIndex =" << mappingIndex;
#endif

    QMutexLocker locker(&s_BurstKeyTimerMutex);
    if (s_BurstKeyTimerMap.contains(burstKey)) {
        int burstpressTime = keyMappingDataList->at(mappingIndex).BurstPressTime;
        QKeyMapper_Worker *instance = QKeyMapper_Worker::getInstance();
        QTimer* burstkeypressTimer;
        if (s_BurstKeyPressTimerMap.contains(burstKey)) {
            burstkeypressTimer = s_BurstKeyPressTimerMap.value(burstKey);
        }
        else {
            burstkeypressTimer = new QTimer();
            burstkeypressTimer->setTimerType(Qt::PreciseTimer);
            burstkeypressTimer->setSingleShot(true);
            QObject::connect(burstkeypressTimer, &QTimer::timeout, instance, [burstKey, mappingIndex, keyMappingDataList]() {
                onBurstKeyPressTimeOut(burstKey, mappingIndex, keyMappingDataList);
            });
            s_BurstKeyPressTimerMap.insert(burstKey, burstkeypressTimer);
        }
        burstkeypressTimer->start(burstpressTime);
        sendBurstKeyDown(mappingIndex, false, keyMappingDataList);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning().nospace().noquote() << "[QKeyMapper_Worker::onBurstKeyTimeOut]" << " Could not find Key [" << burstKey << "], MappingIndex = " << mappingIndex << ", in s_BurstKeyTimerMap!!!";
#endif
    }
}

bool QKeyMapper_Worker::JoyStickKeysProc(QString keycodeString, int keyupdown, const QJoystickDevice *joystick)
{
    Q_UNUSED(joystick);

#ifdef DEBUG_LOGOUT_ON
    if (KEY_DOWN == keyupdown){
        qDebug("[JoyStickKeysProc] RealKey: \"%s\" KeyDown -> [P%d][%s]", keycodeString.toStdString().c_str(), joystick->playerindex, joystick->name.toStdString().c_str());
    }
    else if (KEY_UP == keyupdown){
        qDebug("[JoyStickKeysProc] RealKey: \"%s\" KeyUp -> [P%d][%s]", keycodeString.toStdString().c_str(), joystick->playerindex, joystick->name.toStdString().c_str());
    }
    else {
        /* Do Nothing */
    }
#endif

    if (QKeyMapper::KEYMAP_IDLE == QKeyMapper::getInstance()->m_KeyMapStatus && KEY_DOWN == keyupdown) {
        emit QKeyMapper::getInstance()->updateKeyComboBoxWithJoystickKey_Signal(keycodeString);
    }

    QString keycodeString_nochanged = keycodeString;
    int player_index = joystick->playerindex;
    if (JOYSTICK_PLAYER_INDEX_MIN <= player_index && player_index <= JOYSTICK_PLAYER_INDEX_MAX) {
        keycodeString = QString("%1@%2").arg(keycodeString, QString::number(player_index));
    }

    bool returnFlag = false;
    int findindex = -1;

    if (m_JoystickCapture) {
        returnFlag = (hookBurstAndLockProc(keycodeString, keyupdown) != KEY_PROC_NONE);
        findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
    }

    int intercept = updatePressedRealKeysList(keycodeString, keyupdown);
    bool mappingswitch_detected = detectMappingSwitchKey(keycodeString_nochanged, keyupdown);
    bool displayswitch_detected = detectDisplaySwitchKey(keycodeString_nochanged, keyupdown);
    Q_UNUSED(mappingswitch_detected);
    Q_UNUSED(displayswitch_detected);
    if (!m_JoystickCapture) {
        if (intercept == KEY_INTERCEPT_BLOCK_KEY_RECORD) {
            return true;
        }
        else {
            return false;
        }
    }
    else {
        bool tabswitch_detected = detectMappingTableTabHotkeys(keycodeString_nochanged, keyupdown);
        Q_UNUSED(tabswitch_detected);
        if (intercept == KEY_INTERCEPT_BLOCK) {
            return true;
        }
    }

    int combinationkey_detected = detectCombinationKeys(keycodeString, keyupdown);
    Q_UNUSED(combinationkey_detected);

    if (combinationkey_detected) {
        if (KEY_DOWN == keyupdown) {
            if (KEY_INTERCEPT_BLOCK == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[JoyStickKeysProc] detectCombinationKeys (%1) KEY_DOWN return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                qDebug().nospace().noquote() << debugmessage;
#endif
                return true;
            }
        }
        else {
            if (findindex >= 0) {
// #ifdef DEBUG_LOGOUT_ON
//                 QString debugmessage = QString("[JoyStickKeysProc] OriginalKey \"%1\" found rowindex(%d), detectCombinationKeys KEY_UP return -> KEY_INTERCEPT_BLOCK").arg(keycodeString).arg(findindex);
//                 qDebug().nospace().noquote() << debugmessage;
// #endif
//                 return true;
            }
            else {
                if (KEY_INTERCEPT_BLOCK == combinationkey_detected) {
#ifdef DEBUG_LOGOUT_ON
                    QString debugmessage = QString("[JoyStickKeysProc] detectCombinationKeys (%1) KEY_UP return -> KEY_INTERCEPT_BLOCK").arg(keycodeString);
                    qDebug().nospace().noquote() << debugmessage;
#endif
                    return true;
                }
            }
        }
    }

    if (false == returnFlag) {
        if (findindex >=0){
            QStringList mappingKeyList = QKeyMapper::KeyMappingDataList->at(findindex).Mapping_Keys;
            QStringList mappingKey_KeyUpList = QKeyMapper::KeyMappingDataList->at(findindex).MappingKeys_KeyUp;
            QString original_key = QKeyMapper::KeyMappingDataList->at(findindex).Original_Key;
            int mappingkeylist_size = mappingKeyList.size();
            int SendTiming = QKeyMapper::KeyMappingDataList->at(findindex).SendTiming;
            bool KeySeqHoldDown = QKeyMapper::KeyMappingDataList->at(findindex).KeySeqHoldDown;
            if (KEY_DOWN == keyupdown){
                if (SENDTIMING_KEYDOWN == SendTiming) {
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                }
                else if (SENDTIMING_KEYUP == SendTiming) {
                    /* KEY_DOWN & SENDTIMING_KEYUP == SendTiming -> do nothing */
                }
                else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                }
                else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                }
                else { /* SENDTIMING_NORMAL == SendTiming */
                    /* Add for KeySequenceHoldDown >>> */
                    if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                    }
                    else {
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                    }
                    /* Add for KeySequenceHoldDown <<< */
                }
                returnFlag = true;
            }
            else { /* KEY_UP == keyupdown */
                if (SENDTIMING_KEYDOWN == SendTiming) {
                    /* KEY_UP & SENDTIMING_KEYDOWN == SendTiming -> do nothing */
                }
                else if (SENDTIMING_KEYUP == SendTiming) {
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                }
                else if (SENDTIMING_KEYDOWN_AND_KEYUP == SendTiming) {
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                }
                else if (SENDTIMING_NORMAL_AND_KEYUP == SendTiming) {
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);

                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                    QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKey_KeyUpList, KEY_UP, original_key, SENDMODE_NORMAL);
                }
                else { /* SENDTIMING_NORMAL == SendTiming */
                    /* Add for KeySequenceHoldDown >>> */
                    if (mappingkeylist_size > 1 && KeySeqHoldDown) {
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_KEYSEQ_HOLDDOWN);
                    }
                    else {
                        QKeyMapper_Worker::getInstance()->emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                    }
                    /* Add for KeySequenceHoldDown <<< */
                }
                returnFlag = true;
            }
        }
    }

//#ifdef DEBUG_LOGOUT_ON
//    qDebug() << "[JoyStickKeysProc]" << (keyupdown == KEY_DOWN?"KEY_DOWN":"KEY_UP") << " : pressedRealKeysList -> " << pressedRealKeysList;
//#endif

    return returnFlag;
}

#ifdef DINPUT_TEST
void *QKeyMapper_Worker::HookVTableFunction(void *pVTable, void *fnHookFunc, int nOffset)
{
    intptr_t ptrVtable = *((intptr_t*)pVTable); // Pointer to our chosen vtable
    intptr_t ptrFunction = ptrVtable + sizeof(intptr_t) * nOffset; // The offset to the function (remember it's a zero indexed array with a size of four bytes)
    intptr_t ptrOriginal = *((intptr_t*)ptrFunction); // Save original address

    // Edit the memory protection so we can modify it
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery((LPCVOID)ptrFunction, &mbi, sizeof(mbi));
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &mbi.Protect);

    // Overwrite the old function with our new one
    *((intptr_t*)ptrFunction) = (intptr_t)fnHookFunc;

    // Restore the protection
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &mbi.Protect);

    // Return the original function address incase we want to call it
    return (void*)ptrOriginal;
}

HRESULT QKeyMapper_Worker::hookGetDeviceState(IDirectInputDevice8W *pThis, DWORD cbData, LPVOID lpvData)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("hookGetDeviceState() Called");
#endif
    HRESULT result = FuncPtrGetDeviceState(pThis, cbData, lpvData);

    if (result == DI_OK) {
        if (cbData == 256){//caller is a keyboard
            BYTE* keystate_array = (BYTE*)lpvData;
            if  (keystate_array[DIK_W] & 0x80){
                qDebug().noquote() << "[DINPUT] hookGetDeviceData: [W] Key Pressed.";
            }
            if  (keystate_array[DIK_S] & 0x80){
                qDebug().noquote() << "[DINPUT] hookGetDeviceData: [S] Key Pressed.";
            }
            if  (keystate_array[DIK_A] & 0x80){
                qDebug().noquote() << "[DINPUT] hookGetDeviceData: [A] Key Pressed.";
            }
            if  (keystate_array[DIK_D] & 0x80){
                qDebug().noquote() << "[DINPUT] hookGetDeviceData: [D] Key Pressed.";
            }
        }
    }

    return result;
}

HRESULT QKeyMapper_Worker::hookGetDeviceData(IDirectInputDevice8W *pThis, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("hookGetDeviceData() Called");
#endif
    HRESULT result = FuncPtrGetDeviceData(pThis, cbObjectData, rgdod, pdwInOut, dwFlags);

    if (result == DI_OK) {
        for (DWORD i = 0; i < *pdwInOut; ++i) {
            if (LOBYTE(rgdod[i].dwData) > 0) { //key down
                if (rgdod[i].dwOfs == DIK_W) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [W] Key Down.";
                }
                else if (rgdod[i].dwOfs == DIK_S) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [S] Key Down.";
                }
                else if (rgdod[i].dwOfs == DIK_A) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [A] Key Down.";
                }
                else if (rgdod[i].dwOfs == DIK_D) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [D] Key Down.";
                }
            }
            if (LOBYTE(rgdod[i].dwData) == 0) { //key up
                if (rgdod[i].dwOfs == DIK_W) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [W] Key Up.";
                }
                else if (rgdod[i].dwOfs == DIK_S) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [S] Key Up.";
                }
                else if (rgdod[i].dwOfs == DIK_A) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [A] Key Up.";
                }
                else if (rgdod[i].dwOfs == DIK_D) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [D] Key Up.";
                }
            }
        }
    }
    return result;
}
#endif

void QKeyMapper_Worker::initVirtualKeyCodeMap()
{
    VirtualKeyCodeMap.insert        (KEY2MOUSE_UP_STR,          V_KEYCODE(VK_KEY2MOUSE_UP,      EXTENED_FLAG_TRUE));   // 0x8A (Key2Mouse-Up)
    VirtualKeyCodeMap.insert        (KEY2MOUSE_DOWN_STR,        V_KEYCODE(VK_KEY2MOUSE_DOWN,    EXTENED_FLAG_TRUE));   // 0x8B (Key2Mouse-Down)
    VirtualKeyCodeMap.insert        (KEY2MOUSE_LEFT_STR,        V_KEYCODE(VK_KEY2MOUSE_LEFT,    EXTENED_FLAG_TRUE));   // 0x8C (Key2Mouse-Left)
    VirtualKeyCodeMap.insert        (KEY2MOUSE_RIGHT_STR,       V_KEYCODE(VK_KEY2MOUSE_RIGHT,   EXTENED_FLAG_TRUE));   // 0x8D (Key2Mouse-Right)
    VirtualKeyCodeMap.insert        (MOUSE2VJOY_HOLD_KEY_STR,   V_KEYCODE(VK_MOUSE2VJOY_HOLD,   EXTENED_FLAG_TRUE));   // 0x3A (Mouse2vJoy-Hold)
    VirtualKeyCodeMap.insert        (GAMEPAD_HOME_STR,          V_KEYCODE(VK_GAMEPAD_HOME,      EXTENED_FLAG_FALSE));  // 0x07 (GamepadHome)
    VirtualKeyCodeMap.insert        (CROSSHAIR_NORMAL_STR,      V_KEYCODE(VK_CROSSHAIR_NORMAL,  EXTENED_FLAG_TRUE));   // 0x0A (Crosshair-Normal)
    VirtualKeyCodeMap.insert        (CROSSHAIR_TYPEA_STR,       V_KEYCODE(VK_CROSSHAIR_TYPEA,   EXTENED_FLAG_TRUE));   // 0x0B (Crosshair-TypeA)
    VirtualKeyCodeMap.insert        (GYRO2MOUSE_HOLD_KEY_STR,   V_KEYCODE(VK_GYRO2MOUSE_HOLD,   EXTENED_FLAG_TRUE));   // 0x0E (Gyro2Mouse-Hold)
    VirtualKeyCodeMap.insert        (GYRO2MOUSE_MOVE_KEY_STR,   V_KEYCODE(VK_GYRO2MOUSE_MOVE,   EXTENED_FLAG_TRUE));   // 0x0F (Gyro2Mouse-Move)

    // US 104 Keyboard Main Area
    // Row 1
    VirtualKeyCodeMap.insert        ("`",           V_KEYCODE(VK_OEM_3,         EXTENED_FLAG_FALSE));   // 0xC0
    VirtualKeyCodeMap.insert        ("1",           V_KEYCODE(VK_1,             EXTENED_FLAG_FALSE));   // 0x31
    VirtualKeyCodeMap.insert        ("2",           V_KEYCODE(VK_2,             EXTENED_FLAG_FALSE));   // 0x32
    VirtualKeyCodeMap.insert        ("3",           V_KEYCODE(VK_3,             EXTENED_FLAG_FALSE));   // 0x33
    VirtualKeyCodeMap.insert        ("4",           V_KEYCODE(VK_4,             EXTENED_FLAG_FALSE));   // 0x34
    VirtualKeyCodeMap.insert        ("5",           V_KEYCODE(VK_5,             EXTENED_FLAG_FALSE));   // 0x35
    VirtualKeyCodeMap.insert        ("6",           V_KEYCODE(VK_6,             EXTENED_FLAG_FALSE));   // 0x36
    VirtualKeyCodeMap.insert        ("7",           V_KEYCODE(VK_7,             EXTENED_FLAG_FALSE));   // 0x37
    VirtualKeyCodeMap.insert        ("8",           V_KEYCODE(VK_8,             EXTENED_FLAG_FALSE));   // 0x38
    VirtualKeyCodeMap.insert        ("9",           V_KEYCODE(VK_9,             EXTENED_FLAG_FALSE));   // 0x39
    VirtualKeyCodeMap.insert        ("0",           V_KEYCODE(VK_0,             EXTENED_FLAG_FALSE));   // 0x30
    VirtualKeyCodeMap.insert        ("-",           V_KEYCODE(VK_OEM_MINUS,     EXTENED_FLAG_FALSE));   // 0xBD
    VirtualKeyCodeMap.insert        ("=",           V_KEYCODE(VK_OEM_PLUS,      EXTENED_FLAG_FALSE));   // 0xBB
    VirtualKeyCodeMap.insert        ("Backspace",   V_KEYCODE(VK_BACK,          EXTENED_FLAG_FALSE));   // 0x08
    // Row 2
    VirtualKeyCodeMap.insert        ("Tab",         V_KEYCODE(VK_TAB,           EXTENED_FLAG_FALSE));   // 0x09
    VirtualKeyCodeMap.insert        ("Q",           V_KEYCODE(VK_Q,             EXTENED_FLAG_FALSE));   // 0x51
    VirtualKeyCodeMap.insert        ("W",           V_KEYCODE(VK_W,             EXTENED_FLAG_FALSE));   // 0x57
    VirtualKeyCodeMap.insert        ("E",           V_KEYCODE(VK_E,             EXTENED_FLAG_FALSE));   // 0x45
    VirtualKeyCodeMap.insert        ("R",           V_KEYCODE(VK_R,             EXTENED_FLAG_FALSE));   // 0x52
    VirtualKeyCodeMap.insert        ("T",           V_KEYCODE(VK_T,             EXTENED_FLAG_FALSE));   // 0x54
    VirtualKeyCodeMap.insert        ("Y",           V_KEYCODE(VK_Y,             EXTENED_FLAG_FALSE));   // 0x59
    VirtualKeyCodeMap.insert        ("U",           V_KEYCODE(VK_U,             EXTENED_FLAG_FALSE));   // 0x55
    VirtualKeyCodeMap.insert        ("I",           V_KEYCODE(VK_I,             EXTENED_FLAG_FALSE));   // 0x49
    VirtualKeyCodeMap.insert        ("O",           V_KEYCODE(VK_O,             EXTENED_FLAG_FALSE));   // 0x4F
    VirtualKeyCodeMap.insert        ("P",           V_KEYCODE(VK_P,             EXTENED_FLAG_FALSE));   // 0x50
    VirtualKeyCodeMap.insert        ("[",           V_KEYCODE(VK_OEM_4,         EXTENED_FLAG_FALSE));   // 0xDB
    VirtualKeyCodeMap.insert        ("]",           V_KEYCODE(VK_OEM_6,         EXTENED_FLAG_FALSE));   // 0xDD
    VirtualKeyCodeMap.insert        ("\\",          V_KEYCODE(VK_OEM_5,         EXTENED_FLAG_FALSE));   // 0xDC
    // Row 3
    VirtualKeyCodeMap.insert        ("CapsLock",    V_KEYCODE(VK_CAPITAL,       EXTENED_FLAG_FALSE));   // 0x14
    VirtualKeyCodeMap.insert        ("A",           V_KEYCODE(VK_A,             EXTENED_FLAG_FALSE));   // 0x41
    VirtualKeyCodeMap.insert        ("S",           V_KEYCODE(VK_S,             EXTENED_FLAG_FALSE));   // 0x53
    VirtualKeyCodeMap.insert        ("D",           V_KEYCODE(VK_D,             EXTENED_FLAG_FALSE));   // 0x44
    VirtualKeyCodeMap.insert        ("F",           V_KEYCODE(VK_F,             EXTENED_FLAG_FALSE));   // 0x46
    VirtualKeyCodeMap.insert        ("G",           V_KEYCODE(VK_G,             EXTENED_FLAG_FALSE));   // 0x47
    VirtualKeyCodeMap.insert        ("H",           V_KEYCODE(VK_H,             EXTENED_FLAG_FALSE));   // 0x48
    VirtualKeyCodeMap.insert        ("J",           V_KEYCODE(VK_J,             EXTENED_FLAG_FALSE));   // 0x4A
    VirtualKeyCodeMap.insert        ("K",           V_KEYCODE(VK_K,             EXTENED_FLAG_FALSE));   // 0x4B
    VirtualKeyCodeMap.insert        ("L",           V_KEYCODE(VK_L,             EXTENED_FLAG_FALSE));   // 0x4C
    VirtualKeyCodeMap.insert        (";",           V_KEYCODE(VK_OEM_1,         EXTENED_FLAG_FALSE));   // 0xBA
    VirtualKeyCodeMap.insert        ("'",           V_KEYCODE(VK_OEM_7,         EXTENED_FLAG_FALSE));   // 0xDE
    VirtualKeyCodeMap.insert        ("Enter",       V_KEYCODE(VK_RETURN,        EXTENED_FLAG_FALSE));   // 0x0D
    // Row 4
    VirtualKeyCodeMap.insert        ("L-Shift",     V_KEYCODE(VK_LSHIFT,        EXTENED_FLAG_FALSE));   // 0xA0
    VirtualKeyCodeMap.insert        ("Z",           V_KEYCODE(VK_Z,             EXTENED_FLAG_FALSE));   // 0x5A
    VirtualKeyCodeMap.insert        ("X",           V_KEYCODE(VK_X,             EXTENED_FLAG_FALSE));   // 0x58
    VirtualKeyCodeMap.insert        ("C",           V_KEYCODE(VK_C,             EXTENED_FLAG_FALSE));   // 0x43
    VirtualKeyCodeMap.insert        ("V",           V_KEYCODE(VK_V,             EXTENED_FLAG_FALSE));   // 0x56
    VirtualKeyCodeMap.insert        ("B",           V_KEYCODE(VK_B,             EXTENED_FLAG_FALSE));   // 0x42
    VirtualKeyCodeMap.insert        ("N",           V_KEYCODE(VK_N,             EXTENED_FLAG_FALSE));   // 0x4E
    VirtualKeyCodeMap.insert        ("M",           V_KEYCODE(VK_M,             EXTENED_FLAG_FALSE));   // 0x4D
    VirtualKeyCodeMap.insert        (",",           V_KEYCODE(VK_OEM_COMMA,     EXTENED_FLAG_FALSE));   // 0xBC
    VirtualKeyCodeMap.insert        (".",           V_KEYCODE(VK_OEM_PERIOD,    EXTENED_FLAG_FALSE));   // 0xBE
    VirtualKeyCodeMap.insert        ("/",           V_KEYCODE(VK_OEM_2,         EXTENED_FLAG_FALSE));   // 0xBF
    VirtualKeyCodeMap.insert        ("R-Shift",     V_KEYCODE(VK_RSHIFT,        EXTENED_FLAG_TRUE ));   // 0xA1 + E
    // Row 5
    VirtualKeyCodeMap.insert        ("L-Ctrl",      V_KEYCODE(VK_LCONTROL,      EXTENED_FLAG_FALSE));   // 0xA2
    VirtualKeyCodeMap.insert        ("L-Win",       V_KEYCODE(VK_LWIN,          EXTENED_FLAG_TRUE ));   // 0x5B + E
    VirtualKeyCodeMap.insert        ("L-Alt",       V_KEYCODE(VK_LMENU,         EXTENED_FLAG_FALSE));   // 0xA4
    VirtualKeyCodeMap.insert        ("Space",       V_KEYCODE(VK_SPACE,         EXTENED_FLAG_FALSE));   // 0x20
    VirtualKeyCodeMap.insert        ("R-Alt",       V_KEYCODE(VK_RMENU,         EXTENED_FLAG_TRUE ));   // 0xA5 + E
    VirtualKeyCodeMap.insert        ("Application", V_KEYCODE(VK_APPS,          EXTENED_FLAG_TRUE ));   // 0x5D + E
    VirtualKeyCodeMap.insert        ("R-Ctrl",      V_KEYCODE(VK_RCONTROL,      EXTENED_FLAG_TRUE ));   // 0xA3 + E
    VirtualKeyCodeMap.insert        ("R-Win",       V_KEYCODE(VK_RWIN,          EXTENED_FLAG_TRUE ));   // 0x5C + E
    // Old special keys
    // VirtualKeyCodeMap.insert        ("Shift",       V_KEYCODE(VK_SHIFT,         EXTENED_FLAG_FALSE));   // 0x10
    // VirtualKeyCodeMap.insert        ("Ctrl",        V_KEYCODE(VK_CONTROL,       EXTENED_FLAG_FALSE));   // 0x11
    // VirtualKeyCodeMap.insert        ("Alt",         V_KEYCODE(VK_MENU,          EXTENED_FLAG_FALSE));   // 0x12

    // Function Keys
    VirtualKeyCodeMap.insert        ("Esc",         V_KEYCODE(VK_ESCAPE,        EXTENED_FLAG_FALSE));   // 0x1B
    VirtualKeyCodeMap.insert        ("F1",          V_KEYCODE(VK_F1,            EXTENED_FLAG_FALSE));   // 0x70
    VirtualKeyCodeMap.insert        ("F2",          V_KEYCODE(VK_F2,            EXTENED_FLAG_FALSE));   // 0x71
    VirtualKeyCodeMap.insert        ("F3",          V_KEYCODE(VK_F3,            EXTENED_FLAG_FALSE));   // 0x72
    VirtualKeyCodeMap.insert        ("F4",          V_KEYCODE(VK_F4,            EXTENED_FLAG_FALSE));   // 0x73
    VirtualKeyCodeMap.insert        ("F5",          V_KEYCODE(VK_F5,            EXTENED_FLAG_FALSE));   // 0x74
    VirtualKeyCodeMap.insert        ("F6",          V_KEYCODE(VK_F6,            EXTENED_FLAG_FALSE));   // 0x75
    VirtualKeyCodeMap.insert        ("F7",          V_KEYCODE(VK_F7,            EXTENED_FLAG_FALSE));   // 0x76
    VirtualKeyCodeMap.insert        ("F8",          V_KEYCODE(VK_F8,            EXTENED_FLAG_FALSE));   // 0x77
    VirtualKeyCodeMap.insert        ("F9",          V_KEYCODE(VK_F9,            EXTENED_FLAG_FALSE));   // 0x78
    VirtualKeyCodeMap.insert        ("F10",         V_KEYCODE(VK_F10,           EXTENED_FLAG_FALSE));   // 0x79
    VirtualKeyCodeMap.insert        ("F11",         V_KEYCODE(VK_F11,           EXTENED_FLAG_FALSE));   // 0x7A
    VirtualKeyCodeMap.insert        ("F12",         V_KEYCODE(VK_F12,           EXTENED_FLAG_FALSE));   // 0x7B
    VirtualKeyCodeMap.insert        ("F13",         V_KEYCODE(VK_F13,           EXTENED_FLAG_FALSE));   // 0x7C
    VirtualKeyCodeMap.insert        ("F14",         V_KEYCODE(VK_F14,           EXTENED_FLAG_FALSE));   // 0x7D
    VirtualKeyCodeMap.insert        ("F15",         V_KEYCODE(VK_F15,           EXTENED_FLAG_FALSE));   // 0x7E
    VirtualKeyCodeMap.insert        ("F16",         V_KEYCODE(VK_F16,           EXTENED_FLAG_FALSE));   // 0x7F
    VirtualKeyCodeMap.insert        ("F17",         V_KEYCODE(VK_F17,           EXTENED_FLAG_FALSE));   // 0x80
    VirtualKeyCodeMap.insert        ("F18",         V_KEYCODE(VK_F18,           EXTENED_FLAG_FALSE));   // 0x81
    VirtualKeyCodeMap.insert        ("F19",         V_KEYCODE(VK_F19,           EXTENED_FLAG_FALSE));   // 0x82
    VirtualKeyCodeMap.insert        ("F20",         V_KEYCODE(VK_F20,           EXTENED_FLAG_FALSE));   // 0x83
    VirtualKeyCodeMap.insert        ("F21",         V_KEYCODE(VK_F21,           EXTENED_FLAG_FALSE));   // 0x84
    VirtualKeyCodeMap.insert        ("F22",         V_KEYCODE(VK_F22,           EXTENED_FLAG_FALSE));   // 0x85
    VirtualKeyCodeMap.insert        ("F23",         V_KEYCODE(VK_F23,           EXTENED_FLAG_FALSE));   // 0x86
    VirtualKeyCodeMap.insert        ("F24",         V_KEYCODE(VK_F24,           EXTENED_FLAG_FALSE));   // 0x87

    VirtualKeyCodeMap.insert        ("PrintScrn",   V_KEYCODE(VK_SNAPSHOT,      EXTENED_FLAG_TRUE ));   // 0x2C + E
    VirtualKeyCodeMap.insert        ("ScrollLock",  V_KEYCODE(VK_SCROLL,        EXTENED_FLAG_FALSE));   // 0x91
    VirtualKeyCodeMap.insert        ("Pause",       V_KEYCODE(VK_PAUSE,         EXTENED_FLAG_FALSE));   // 0x13

    VirtualKeyCodeMap.insert        ("Insert",      V_KEYCODE(VK_INSERT,        EXTENED_FLAG_TRUE ));   // 0x2D + E
    VirtualKeyCodeMap.insert        ("Delete",      V_KEYCODE(VK_DELETE,        EXTENED_FLAG_TRUE ));   // 0x2E + E
    VirtualKeyCodeMap.insert        ("Home",        V_KEYCODE(VK_HOME,          EXTENED_FLAG_TRUE ));   // 0x24 + E
    VirtualKeyCodeMap.insert        ("End",         V_KEYCODE(VK_END,           EXTENED_FLAG_TRUE ));   // 0x23 + E
    VirtualKeyCodeMap.insert        ("PageUp",      V_KEYCODE(VK_PRIOR,         EXTENED_FLAG_TRUE ));   // 0x21 + E
    VirtualKeyCodeMap.insert        ("PageDown",    V_KEYCODE(VK_NEXT,          EXTENED_FLAG_TRUE ));   // 0x22 + E

    VirtualKeyCodeMap.insert        ("Up",          V_KEYCODE(VK_UP,            EXTENED_FLAG_TRUE ));   // 0x26 + E
    VirtualKeyCodeMap.insert        ("Down",        V_KEYCODE(VK_DOWN,          EXTENED_FLAG_TRUE ));   // 0x28 + E
    VirtualKeyCodeMap.insert        ("Left",        V_KEYCODE(VK_LEFT,          EXTENED_FLAG_TRUE ));   // 0x25 + E
    VirtualKeyCodeMap.insert        ("Right",       V_KEYCODE(VK_RIGHT,         EXTENED_FLAG_TRUE ));   // 0x27 + E

    //NumberPad Keys
    VirtualKeyCodeMap.insert        ("NumLock",             V_KEYCODE(VK_NUMLOCK,       EXTENED_FLAG_TRUE ));   // 0x90 + E
    VirtualKeyCodeMap.insert        ("Num/",                V_KEYCODE(VK_DIVIDE,        EXTENED_FLAG_TRUE ));   // 0x6F + E
    VirtualKeyCodeMap.insert        ("Num*",                V_KEYCODE(VK_MULTIPLY,      EXTENED_FLAG_FALSE));   // 0x6A
    VirtualKeyCodeMap.insert        ("Num-",               V_KEYCODE(VK_SUBTRACT,      EXTENED_FLAG_FALSE));   // 0x6D
    VirtualKeyCodeMap.insert        ("Num＋",               V_KEYCODE(VK_ADD,           EXTENED_FLAG_FALSE));   // 0x6B
    VirtualKeyCodeMap.insert        ("Num.",                V_KEYCODE(VK_DECIMAL,       EXTENED_FLAG_FALSE));   // 0x6E
    VirtualKeyCodeMap.insert        ("Num0",                V_KEYCODE(VK_NUMPAD0,       EXTENED_FLAG_FALSE));   // 0x60
    VirtualKeyCodeMap.insert        ("Num1",                V_KEYCODE(VK_NUMPAD1,       EXTENED_FLAG_FALSE));   // 0x61
    VirtualKeyCodeMap.insert        ("Num2",                V_KEYCODE(VK_NUMPAD2,       EXTENED_FLAG_FALSE));   // 0x62
    VirtualKeyCodeMap.insert        ("Num3",                V_KEYCODE(VK_NUMPAD3,       EXTENED_FLAG_FALSE));   // 0x63
    VirtualKeyCodeMap.insert        ("Num4",                V_KEYCODE(VK_NUMPAD4,       EXTENED_FLAG_FALSE));   // 0x64
    VirtualKeyCodeMap.insert        ("Num5",                V_KEYCODE(VK_NUMPAD5,       EXTENED_FLAG_FALSE));   // 0x65
    VirtualKeyCodeMap.insert        ("Num6",                V_KEYCODE(VK_NUMPAD6,       EXTENED_FLAG_FALSE));   // 0x66
    VirtualKeyCodeMap.insert        ("Num7",                V_KEYCODE(VK_NUMPAD7,       EXTENED_FLAG_FALSE));   // 0x67
    VirtualKeyCodeMap.insert        ("Num8",                V_KEYCODE(VK_NUMPAD8,       EXTENED_FLAG_FALSE));   // 0x68
    VirtualKeyCodeMap.insert        ("Num9",                V_KEYCODE(VK_NUMPAD9,       EXTENED_FLAG_FALSE));   // 0x69
    VirtualKeyCodeMap.insert        ("NumEnter",            V_KEYCODE(VK_RETURN,        EXTENED_FLAG_TRUE ));   // 0x0D + E
    //NumLock Off NumberPadKeys
    VirtualKeyCodeMap.insert        ("Num.(NumOFF)",       V_KEYCODE(VK_DELETE,        EXTENED_FLAG_FALSE));   // 0x2E
    VirtualKeyCodeMap.insert        ("Num0(NumOFF)",       V_KEYCODE(VK_INSERT,        EXTENED_FLAG_FALSE));   // 0x2D
    VirtualKeyCodeMap.insert        ("Num1(NumOFF)",       V_KEYCODE(VK_END,           EXTENED_FLAG_FALSE));   // 0x23
    VirtualKeyCodeMap.insert        ("Num2(NumOFF)",       V_KEYCODE(VK_DOWN,          EXTENED_FLAG_FALSE));   // 0x28
    VirtualKeyCodeMap.insert        ("Num3(NumOFF)",       V_KEYCODE(VK_NEXT,          EXTENED_FLAG_FALSE));   // 0x22
    VirtualKeyCodeMap.insert        ("Num4(NumOFF)",       V_KEYCODE(VK_LEFT,          EXTENED_FLAG_FALSE));   // 0x25
    VirtualKeyCodeMap.insert        ("Num5(NumOFF)",       V_KEYCODE(VK_CLEAR,         EXTENED_FLAG_FALSE));   // 0x0C
    VirtualKeyCodeMap.insert        ("Num6(NumOFF)",       V_KEYCODE(VK_RIGHT,         EXTENED_FLAG_FALSE));   // 0x27
    VirtualKeyCodeMap.insert        ("Num7(NumOFF)",       V_KEYCODE(VK_HOME,          EXTENED_FLAG_FALSE));   // 0x24
    VirtualKeyCodeMap.insert        ("Num8(NumOFF)",       V_KEYCODE(VK_UP,            EXTENED_FLAG_FALSE));   // 0x26
    VirtualKeyCodeMap.insert        ("Num9(NumOFF)",       V_KEYCODE(VK_PRIOR,         EXTENED_FLAG_FALSE));   // 0x21

    // MultiMedia keys
    VirtualKeyCodeMap.insert        ("VolumeMute",            V_KEYCODE(VK_VOLUME_MUTE,           EXTENED_FLAG_TRUE));   // 0xAD
    VirtualKeyCodeMap.insert        ("VolumeDown",            V_KEYCODE(VK_VOLUME_DOWN,           EXTENED_FLAG_TRUE));   // 0xAE
    VirtualKeyCodeMap.insert        ("VolumeUp",              V_KEYCODE(VK_VOLUME_UP,             EXTENED_FLAG_TRUE));   // 0xAF
    VirtualKeyCodeMap.insert        ("MediaNext",          V_KEYCODE(VK_MEDIA_NEXT_TRACK,      EXTENED_FLAG_TRUE));   // 0xB0
    VirtualKeyCodeMap.insert        ("MediaPrev",          V_KEYCODE(VK_MEDIA_PREV_TRACK,      EXTENED_FLAG_TRUE));   // 0xB1
    VirtualKeyCodeMap.insert        ("MediaStop",          V_KEYCODE(VK_MEDIA_STOP,            EXTENED_FLAG_TRUE));   // 0xB2
    VirtualKeyCodeMap.insert        ("MediaPlayPause",     V_KEYCODE(VK_MEDIA_PLAY_PAUSE,      EXTENED_FLAG_TRUE));   // 0xB3
    VirtualKeyCodeMap.insert        ("LaunchMail",         V_KEYCODE(VK_LAUNCH_MAIL,           EXTENED_FLAG_TRUE));   // 0xB4
    VirtualKeyCodeMap.insert        ("SelectMedia",        V_KEYCODE(VK_LAUNCH_MEDIA_SELECT,   EXTENED_FLAG_TRUE));   // 0xB5
    VirtualKeyCodeMap.insert        ("LaunchApp1",         V_KEYCODE(VK_LAUNCH_APP1,           EXTENED_FLAG_TRUE));   // 0xB6
    VirtualKeyCodeMap.insert        ("LaunchApp2",         V_KEYCODE(VK_LAUNCH_APP2,           EXTENED_FLAG_TRUE));   // 0xB7

    // Browser keys
    VirtualKeyCodeMap.insert        ("BrowserBack",        V_KEYCODE(VK_BROWSER_BACK,          EXTENED_FLAG_TRUE));   // 0xA6
    VirtualKeyCodeMap.insert        ("BrowserForward",     V_KEYCODE(VK_BROWSER_FORWARD,       EXTENED_FLAG_TRUE));   // 0xA7
    VirtualKeyCodeMap.insert        ("BrowserRefresh",     V_KEYCODE(VK_BROWSER_REFRESH,       EXTENED_FLAG_TRUE));   // 0xA8
    VirtualKeyCodeMap.insert        ("BrowserStop",        V_KEYCODE(VK_BROWSER_STOP,          EXTENED_FLAG_TRUE));   // 0xA9
    VirtualKeyCodeMap.insert        ("BrowserSearch",      V_KEYCODE(VK_BROWSER_SEARCH,        EXTENED_FLAG_TRUE));   // 0xAA
    VirtualKeyCodeMap.insert        ("BrowserFavorites",   V_KEYCODE(VK_BROWSER_FAVORITES,     EXTENED_FLAG_TRUE));   // 0xAB
    VirtualKeyCodeMap.insert        ("BrowserHome",        V_KEYCODE(VK_BROWSER_HOME,          EXTENED_FLAG_TRUE));   // 0xAC
}

void QKeyMapper_Worker::initVirtualMouseButtonMap()
{
    VirtualMouseButtonMap.insert("Mouse-L",             V_MOUSECODE(MOUSEEVENTF_LEFTDOWN,       MOUSEEVENTF_LEFTUP,     0           )); // Mouse Button Left
    VirtualMouseButtonMap.insert("Mouse-R",             V_MOUSECODE(MOUSEEVENTF_RIGHTDOWN,      MOUSEEVENTF_RIGHTUP,    0           )); // Mouse Button Right
    VirtualMouseButtonMap.insert("Mouse-M",             V_MOUSECODE(MOUSEEVENTF_MIDDLEDOWN,     MOUSEEVENTF_MIDDLEUP,   0           )); // Mouse Button Middle
    VirtualMouseButtonMap.insert("Mouse-X1",            V_MOUSECODE(MOUSEEVENTF_XDOWN,          MOUSEEVENTF_XUP,        XBUTTON1    )); // Mouse Button X1
    VirtualMouseButtonMap.insert("Mouse-X2",            V_MOUSECODE(MOUSEEVENTF_XDOWN,          MOUSEEVENTF_XUP,        XBUTTON2    )); // Mouse Button X2
    VirtualMouseButtonMap.insert(MOUSE_WHEEL_UP_STR,    V_MOUSECODE(MOUSEEVENTF_WHEEL,          MOUSEEVENTF_WHEEL,      0           )); // Mouse Wheel Up
    VirtualMouseButtonMap.insert(MOUSE_WHEEL_DOWN_STR,  V_MOUSECODE(MOUSEEVENTF_WHEEL,          MOUSEEVENTF_WHEEL,      0           )); // Mouse Wheel Down
    VirtualMouseButtonMap.insert(MOUSE_WHEEL_LEFT_STR,  V_MOUSECODE(MOUSEEVENTF_HWHEEL,         MOUSEEVENTF_HWHEEL,     0           )); // Mouse Wheel Left
    VirtualMouseButtonMap.insert(MOUSE_WHEEL_RIGHT_STR, V_MOUSECODE(MOUSEEVENTF_HWHEEL,         MOUSEEVENTF_HWHEEL,     0           )); // Mouse Wheel Right

    MouseButtonNameMap.insert(MAKELONG(WM_LBUTTONDOWN,  XBUTTON_NONE),   "Mouse-L");
    MouseButtonNameMap.insert(MAKELONG(WM_LBUTTONUP,    XBUTTON_NONE),   "Mouse-L");
    MouseButtonNameMap.insert(MAKELONG(WM_RBUTTONDOWN,  XBUTTON_NONE),   "Mouse-R");
    MouseButtonNameMap.insert(MAKELONG(WM_RBUTTONUP,    XBUTTON_NONE),   "Mouse-R");
    MouseButtonNameMap.insert(MAKELONG(WM_MBUTTONDOWN,  XBUTTON_NONE),   "Mouse-M");
    MouseButtonNameMap.insert(MAKELONG(WM_MBUTTONUP,    XBUTTON_NONE),   "Mouse-M");
    MouseButtonNameMap.insert(MAKELONG(WM_XBUTTONDOWN,  XBUTTON1    ),   "Mouse-X1");
    MouseButtonNameMap.insert(MAKELONG(WM_XBUTTONUP,    XBUTTON1    ),   "Mouse-X1");
    MouseButtonNameMap.insert(MAKELONG(WM_XBUTTONDOWN,  XBUTTON2    ),   "Mouse-X2");
    MouseButtonNameMap.insert(MAKELONG(WM_XBUTTONUP,    XBUTTON2    ),   "Mouse-X2");

#ifdef MOUSEBUTTON_CONVERT
    MouseButtonNameConvertMap.insert("L-Mouse",     "Mouse-L"   );
    MouseButtonNameConvertMap.insert("R-Mouse",     "Mouse-R"   );
    MouseButtonNameConvertMap.insert("M-Mouse",     "Mouse-M"   );
    MouseButtonNameConvertMap.insert("X1-Mouse",    "Mouse-X1"  );
    MouseButtonNameConvertMap.insert("X2-Mouse",    "Mouse-X2"  );
#endif
}

void QKeyMapper_Worker::initMultiKeyboardInputList()
{
    MultiKeyboardInputList = QStringList() \
            /* Keyboard Keys */
            << "A"
            << "B"
            << "C"
            << "D"
            << "E"
            << "F"
            << "G"
            << "H"
            << "I"
            << "J"
            << "K"
            << "L"
            << "M"
            << "N"
            << "O"
            << "P"
            << "Q"
            << "R"
            << "S"
            << "T"
            << "U"
            << "V"
            << "W"
            << "X"
            << "Y"
            << "Z"
            << "1"
            << "2"
            << "3"
            << "4"
            << "5"
            << "6"
            << "7"
            << "8"
            << "9"
            << "0"
            << "Up"
            << "Down"
            << "Left"
            << "Right"
            << "Insert"
            << "Delete"
            << "Home"
            << "End"
            << "PageUp"
            << "PageDown"
            << "Space"
            << "Tab"
            << "Enter"
            << "L-Shift"
            << "R-Shift"
            << "L-Ctrl"
            << "R-Ctrl"
            << "L-Alt"
            << "R-Alt"
            << "L-Win"
            << "R-Win"
            << "Backspace"
            << "`"
            << "-"
            << "="
            << "["
            << "]"
            << "\\"
            << ";"
            << "'"
            << ","
            << "."
            << "/"
            << "Esc"
            << "F1"
            << "F2"
            << "F3"
            << "F4"
            << "F5"
            << "F6"
            << "F7"
            << "F8"
            << "F9"
            << "F10"
            << "F11"
            << "F12"
            << "F13"
            << "F14"
            << "F15"
            << "F16"
            << "F17"
            << "F18"
            << "F19"
            << "F20"
            << "F21"
            << "F22"
            << "F23"
            << "F24"
            << "CapsLock"
            << "Application"
            << "PrintScrn"
            << "ScrollLock"
            << "Pause"
            << "NumLock"
            << "Num/"
            << "Num*"
            << "Num-"
            << "Num＋"
            << "Num."
            << "Num0"
            << "Num1"
            << "Num2"
            << "Num3"
            << "Num4"
            << "Num5"
            << "Num6"
            << "Num7"
            << "Num8"
            << "Num9"
            << "NumEnter"
            << "Num.(NumOFF)"
            << "Num0(NumOFF)"
            << "Num1(NumOFF)"
            << "Num2(NumOFF)"
            << "Num3(NumOFF)"
            << "Num4(NumOFF)"
            << "Num5(NumOFF)"
            << "Num6(NumOFF)"
            << "Num7(NumOFF)"
            << "Num8(NumOFF)"
            << "Num9(NumOFF)"
            << "VolumeMute"
            << "VolumeDown"
            << "VolumeUp"
            << "MediaNext"
            << "MediaPrev"
            << "MediaStop"
            << "MediaPlayPause"
            << "LaunchMail"
            << "SelectMedia"
            << "LaunchApp1"
            << "LaunchApp2"
            << "BrowserBack"
            << "BrowserForward"
            << "BrowserRefresh"
            << "BrowserStop"
            << "BrowserSearch"
            << "BrowserFavorites"
            << "BrowserHome"
            ;
}

void QKeyMapper_Worker::initMultiMouseInputList()
{
    MultiMouseInputList = QStringList() \
            /* Mouse Keys */
            << "Mouse-L"
            << "Mouse-R"
            << "Mouse-M"
            << "Mouse-X1"
            << "Mouse-X2"
            << MOUSE_WHEEL_UP_STR
            << MOUSE_WHEEL_DOWN_STR
            << MOUSE_WHEEL_LEFT_STR
            << MOUSE_WHEEL_RIGHT_STR
            << VJOY_MOUSE2LS_STR
            << VJOY_MOUSE2RS_STR
               ;
}

void QKeyMapper_Worker::initMultiVirtualGamepadInputList()
{
    MultiVirtualGamepadInputList = QStringList() \
            /* Virtual Gampad Keys */
            << "vJoy-LS-Up"
            << "vJoy-LS-Down"
            << "vJoy-LS-Left"
            << "vJoy-LS-Right"
            << "vJoy-RS-Up"
            << "vJoy-RS-Down"
            << "vJoy-RS-Left"
            << "vJoy-RS-Right"
            << "vJoy-DPad-Up"
            << "vJoy-DPad-Down"
            << "vJoy-DPad-Left"
            << "vJoy-DPad-Right"
            << "vJoy-Key1(A/×)"
            << "vJoy-Key2(B/○)"
            << "vJoy-Key3(X/□)"
            << "vJoy-Key4(Y/△)"
            << "vJoy-Key5(LB)"
            << "vJoy-Key6(RB)"
            << "vJoy-Key7(Back)"
            << "vJoy-Key8(Start)"
            << "vJoy-Key9(LS-Click)"
            << "vJoy-Key10(RS-Click)"
            << "vJoy-Key11(LT)"
            << "vJoy-Key12(RT)"
            << "vJoy-Key13(Guide)"
            ;
}

void QKeyMapper_Worker::initCombinationKeysList()
{
    CombinationKeysList = QStringList() \
            /* Mouse Keys */
            << "Mouse-L"
            << "Mouse-R"
            << "Mouse-M"
            << "Mouse-X1"
            << "Mouse-X2"
            << MOUSE_WHEEL_UP_STR
            << MOUSE_WHEEL_DOWN_STR
            << MOUSE_WHEEL_LEFT_STR
            << MOUSE_WHEEL_RIGHT_STR
            /* Keyboard Keys */
            << "A"
            << "B"
            << "C"
            << "D"
            << "E"
            << "F"
            << "G"
            << "H"
            << "I"
            << "J"
            << "K"
            << "L"
            << "M"
            << "N"
            << "O"
            << "P"
            << "Q"
            << "R"
            << "S"
            << "T"
            << "U"
            << "V"
            << "W"
            << "X"
            << "Y"
            << "Z"
            << "1"
            << "2"
            << "3"
            << "4"
            << "5"
            << "6"
            << "7"
            << "8"
            << "9"
            << "0"
            << "Up"
            << "Down"
            << "Left"
            << "Right"
            << "Insert"
            << "Delete"
            << "Home"
            << "End"
            << "PageUp"
            << "PageDown"
            << "Space"
            << "Tab"
            << "Enter"
            << "L-Shift"
            << "R-Shift"
            << "L-Ctrl"
            << "R-Ctrl"
            << "L-Alt"
            << "R-Alt"
            << "L-Win"
            << "R-Win"
            << "Backspace"
            << "`"
            << "-"
            << "="
            << "["
            << "]"
            << "\\"
            << ";"
            << "'"
            << ","
            << "."
            << "/"
            << "Esc"
            << "F1"
            << "F2"
            << "F3"
            << "F4"
            << "F5"
            << "F6"
            << "F7"
            << "F8"
            << "F9"
            << "F10"
            << "F11"
            << "F12"
            << "F13"
            << "F14"
            << "F15"
            << "F16"
            << "F17"
            << "F18"
            << "F19"
            << "F20"
            << "F21"
            << "F22"
            << "F23"
            << "F24"
            << "CapsLock"
            << "Application"
            << "PrintScrn"
            << "ScrollLock"
            << "Pause"
            << "NumLock"
            << "Num/"
            << "Num*"
            << "Num-"
            << "Num＋"
            << "Num."
            << "Num0"
            << "Num1"
            << "Num2"
            << "Num3"
            << "Num4"
            << "Num5"
            << "Num6"
            << "Num7"
            << "Num8"
            << "Num9"
            << "NumEnter"
            << "Num.(NumOFF)"
            << "Num0(NumOFF)"
            << "Num1(NumOFF)"
            << "Num2(NumOFF)"
            << "Num3(NumOFF)"
            << "Num4(NumOFF)"
            << "Num5(NumOFF)"
            << "Num6(NumOFF)"
            << "Num7(NumOFF)"
            << "Num8(NumOFF)"
            << "Num9(NumOFF)"
            /* Joystick Keys */
            << "Joy-LS-Up"
            << "Joy-LS-Down"
            << "Joy-LS-Left"
            << "Joy-LS-Right"
            << "Joy-RS-Up"
            << "Joy-RS-Down"
            << "Joy-RS-Left"
            << "Joy-RS-Right"
            << "Joy-DPad-Up"
            << "Joy-DPad-Down"
            << "Joy-DPad-Left"
            << "Joy-DPad-Right"
            << "Joy-Misc1"
            << "Joy-Paddle1"
            << "Joy-Paddle2"
            << "Joy-Paddle3"
            << "Joy-Paddle4"
            << "Joy-Touchpad"
            << "Joy-Key1(A/×)"
            << "Joy-Key2(B/○)"
            << "Joy-Key3(X/□)"
            << "Joy-Key4(Y/△)"
            << "Joy-Key5(LB)"
            << "Joy-Key6(RB)"
            << "Joy-Key7(Back)"
            << "Joy-Key8(Start)"
            << "Joy-Key9(LS-Click)"
            << "Joy-Key10(RS-Click)"
            << "Joy-Key11(LT)"
            << "Joy-Key12(RT)"
            << "Joy-Key13(Guide)"
            << "Joy-Key14"
            << "Joy-Key15"
            << "Joy-Key16"
            << "Joy-Key17"
            << "Joy-Key18"
            << "Joy-Key19"
            << "Joy-Key20"
            << "Joy-Key21"
            << "Joy-Key22"
            << "Joy-Key23"
            << "Joy-Key24"
            << "Joy-Key25"
            << "Joy-Key26"
            << "Joy-Key27"
            << "Joy-Key28"
            << "Joy-Key29"
            << "Joy-Key30"
            ;
}

void QKeyMapper_Worker::initJoystickKeyMap()
{
#if 0
    /* Joystick 2Mouse */
    JoyStickKeyMap.insert(JOY_LS2MOUSE_STR              ,   (int)JOYSTICK_LS_MOUSE          );
    JoyStickKeyMap.insert(JOY_RS2MOUSE_STR              ,   (int)JOYSTICK_RS_MOUSE          );
    JoyStickKeyMap.insert(JOY_GYRO2MOUSE_STR            ,   (int)0                          );
    /* Joystick Buttons */
    JoyStickKeyMap.insert("Joy-Key1(A/×)"              ,   (int)JOYSTICK_BUTTON_0          );
    JoyStickKeyMap.insert("Joy-Key2(B/○)"              ,   (int)JOYSTICK_BUTTON_1          );
    JoyStickKeyMap.insert("Joy-Key3(X/□)"              ,   (int)JOYSTICK_BUTTON_2          );
    JoyStickKeyMap.insert("Joy-Key4(Y/△)"              ,   (int)JOYSTICK_BUTTON_3          );
    JoyStickKeyMap.insert("Joy-Key5(LB)"                ,   (int)JOYSTICK_BUTTON_4          );
    JoyStickKeyMap.insert("Joy-Key6(RB)"                ,   (int)JOYSTICK_BUTTON_5          );
    JoyStickKeyMap.insert("Joy-Key7(Back)"              ,   (int)JOYSTICK_BUTTON_6          );
    JoyStickKeyMap.insert("Joy-Key8(Start)"             ,   (int)JOYSTICK_BUTTON_7          );
    JoyStickKeyMap.insert("Joy-Key9(LS-Click)"          ,   (int)JOYSTICK_BUTTON_8          );
    JoyStickKeyMap.insert("Joy-Key10(RS-Click)"         ,   (int)JOYSTICK_BUTTON_9          );
    JoyStickKeyMap.insert("Joy-Key13(Guide)"            ,   (int)JOYSTICK_BUTTON_10         );
    JoyStickKeyMap.insert("Joy-Key11(LT)"               ,   (int)JOYSTICK_LEFT_TRIGGER      );
    JoyStickKeyMap.insert("Joy-Key12(RT)"               ,   (int)JOYSTICK_RIGHT_TRIGGER     );
    JoyStickKeyMap.insert(JOY_LT2VJOYLT_STR             ,   (int)JOYSTICK_LEFT_TRIGGER      );
    JoyStickKeyMap.insert(JOY_RT2VJOYRT_STR             ,   (int)JOYSTICK_RIGHT_TRIGGER     );
    /* Joystick 2vJoy L/R Stick */
    JoyStickKeyMap.insert(JOY_LS2VJOYLS_STR              ,  (int)JOYSTICK_LS_2VJOY          );
    JoyStickKeyMap.insert(JOY_RS2VJOYRS_STR              ,  (int)JOYSTICK_RS_2VJOY          );
    JoyStickKeyMap.insert(JOY_LS2VJOYRS_STR              ,  (int)JOYSTICK_LS_2VJOY          );
    JoyStickKeyMap.insert(JOY_RS2VJOYLS_STR              ,  (int)JOYSTICK_RS_2VJOY          );

    /* Joystick Expanded Buttons */
    JoyStickKeyMap.insert("Joy-Key14"                   ,   (int)JOYSTICK_BUTTON_11         );
    JoyStickKeyMap.insert("Joy-Key15"                   ,   (int)JOYSTICK_BUTTON_12         );
    JoyStickKeyMap.insert("Joy-Key16"                   ,   (int)JOYSTICK_BUTTON_13         );
    JoyStickKeyMap.insert("Joy-Key17"                   ,   (int)JOYSTICK_BUTTON_14         );
    JoyStickKeyMap.insert("Joy-Key18"                   ,   (int)JOYSTICK_BUTTON_15         );
    JoyStickKeyMap.insert("Joy-Key19"                   ,   (int)JOYSTICK_BUTTON_16         );
    JoyStickKeyMap.insert("Joy-Key20"                   ,   (int)JOYSTICK_BUTTON_17         );
    JoyStickKeyMap.insert("Joy-Key21"                   ,   (int)JOYSTICK_BUTTON_18         );
    JoyStickKeyMap.insert("Joy-Key22"                   ,   (int)JOYSTICK_BUTTON_19         );
    JoyStickKeyMap.insert("Joy-Key23"                   ,   (int)JOYSTICK_BUTTON_20         );
    JoyStickKeyMap.insert("Joy-Key24"                   ,   (int)JOYSTICK_BUTTON_21         );
    JoyStickKeyMap.insert("Joy-Key25"                   ,   (int)JOYSTICK_BUTTON_22         );
    JoyStickKeyMap.insert("Joy-Key26"                   ,   (int)JOYSTICK_BUTTON_23         );
    JoyStickKeyMap.insert("Joy-Key27"                   ,   (int)JOYSTICK_BUTTON_24         );
    JoyStickKeyMap.insert("Joy-Key28"                   ,   (int)JOYSTICK_BUTTON_25         );
    JoyStickKeyMap.insert("Joy-Key29"                   ,   (int)JOYSTICK_BUTTON_26         );
    JoyStickKeyMap.insert("Joy-Key30"                   ,   (int)JOYSTICK_BUTTON_27         );

    /* Joystick DPad Direction */
    JoyStickKeyMap.insert("Joy-DPad-Up"                 ,   (int)JOYSTICK_DPAD_UP           );
    JoyStickKeyMap.insert("Joy-DPad-Down"               ,   (int)JOYSTICK_DPAD_DOWN         );
    JoyStickKeyMap.insert("Joy-DPad-Left"               ,   (int)JOYSTICK_DPAD_LEFT         );
    JoyStickKeyMap.insert("Joy-DPad-Right"              ,   (int)JOYSTICK_DPAD_RIGHT        );
    /* Joystick Left-Stick Direction */
    JoyStickKeyMap.insert("Joy-LS-Up"                   ,   (int)JOYSTICK_LS_UP             );
    JoyStickKeyMap.insert("Joy-LS-Down"                 ,   (int)JOYSTICK_LS_DOWN           );
    JoyStickKeyMap.insert("Joy-LS-Left"                 ,   (int)JOYSTICK_LS_LEFT           );
    JoyStickKeyMap.insert("Joy-LS-Right"                ,   (int)JOYSTICK_LS_RIGHT          );
    /* Joystick Right-Stick Direction */
    JoyStickKeyMap.insert("Joy-RS-Up"                   ,   (int)JOYSTICK_RS_UP             );
    JoyStickKeyMap.insert("Joy-RS-Down"                 ,   (int)JOYSTICK_RS_DOWN           );
    JoyStickKeyMap.insert("Joy-RS-Left"                 ,   (int)JOYSTICK_RS_LEFT           );
    JoyStickKeyMap.insert("Joy-RS-Right"                ,   (int)JOYSTICK_RS_RIGHT          );
#endif

    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_A,               "Joy-Key1(A/×)"                 );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_B,               "Joy-Key2(B/○)"                 );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_X,               "Joy-Key3(X/□)"                 );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_Y,               "Joy-Key4(Y/△)"                );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_BACK,            "Joy-Key7(Back)"                );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_GUIDE,           "Joy-Key13(Guide)"              );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_START,           "Joy-Key8(Start)"               );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_LEFTSTICK,       "Joy-Key9(LS-Click)"            );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_RIGHTSTICK,      "Joy-Key10(RS-Click)"           );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_LEFTSHOULDER,    "Joy-Key5(LB)"                  );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_RIGHTSHOULDER,   "Joy-Key6(RB)"                  );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_DPAD_UP,         "Joy-DPad-Up"                   );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_DPAD_DOWN,       "Joy-DPad-Down"                 );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_DPAD_LEFT,       "Joy-DPad-Left"                 );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_DPAD_RIGHT,      "Joy-DPad-Right"                );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_MISC1,           "Joy-Misc1"                     );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_PADDLE1,         "Joy-Paddle1"                   );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_PADDLE2,         "Joy-Paddle2"                   );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_PADDLE3,         "Joy-Paddle3"                   );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_PADDLE4,         "Joy-Paddle4"                   );
    m_ControllerButtonMap.insert(CONTROLLER_BUTTON_TOUCHPAD,        "Joy-Touchpad"                  );

    /* Joystick Buttons Map */
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_0,       "Joy-Key1(A/×)"                 );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_1,       "Joy-Key2(B/○)"                 );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_2,       "Joy-Key3(X/□)"                 );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_3,       "Joy-Key4(Y/△)"                 );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_4,       "Joy-Key5(LB)"                  );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_5,       "Joy-Key6(RB)"                  );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_6,       "Joy-Key7(Back)"                );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_7,       "Joy-Key8(Start)"               );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_8,       "Joy-Key9(LS-Click)"            );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_9,       "Joy-Key10(RS-Click)"           );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_10,      "Joy-Key13(Guide)"              );

    /* Joystick Expanded Buttons Map */
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_11,      "Joy-Key14"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_12,      "Joy-Key15"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_13,      "Joy-Key16"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_14,      "Joy-Key17"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_15,      "Joy-Key18"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_16,      "Joy-Key19"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_17,      "Joy-Key20"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_18,      "Joy-Key21"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_19,      "Joy-Key22"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_20,      "Joy-Key23"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_21,      "Joy-Key24"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_22,      "Joy-Key25"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_23,      "Joy-Key26"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_24,      "Joy-Key27"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_25,      "Joy-Key28"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_26,      "Joy-Key29"                     );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_27,      "Joy-Key30"                     );

    /* Joystick DPad Direction Map */
    m_JoystickDPadMap.insert(JOYSTICK_DPAD_UP,          "Joy-DPad-Up"                   );
    m_JoystickDPadMap.insert(JOYSTICK_DPAD_DOWN,        "Joy-DPad-Down"                 );
    m_JoystickDPadMap.insert(JOYSTICK_DPAD_LEFT,        "Joy-DPad-Left"                 );
    m_JoystickDPadMap.insert(JOYSTICK_DPAD_RIGHT,       "Joy-DPad-Right"                );
    m_JoystickDPadMap.insert(JOYSTICK_DPAD_L_UP,        "Joy-DPad-Left,Joy-DPad-Up"     );
    m_JoystickDPadMap.insert(JOYSTICK_DPAD_L_DOWN,      "Joy-DPad-Left,Joy-DPad-Down"   );
    m_JoystickDPadMap.insert(JOYSTICK_DPAD_R_UP,        "Joy-DPad-Right,Joy-DPad-Up"    );
    m_JoystickDPadMap.insert(JOYSTICK_DPAD_R_DOWN,      "Joy-DPad-Right,Joy-DPad-Down"  );

    /* Joystick Left-Stick Direction Map */
    m_JoystickLStickMap.insert(JOYSTICK_LS_UP,          "Joy-LS-Up"                     );
    m_JoystickLStickMap.insert(JOYSTICK_LS_DOWN,        "Joy-LS-Down"                   );
    m_JoystickLStickMap.insert(JOYSTICK_LS_LEFT,        "Joy-LS-Left"                   );
    m_JoystickLStickMap.insert(JOYSTICK_LS_RIGHT,       "Joy-LS-Right"                  );

    /* Joystick Right-Stick Direction Map */
    m_JoystickRStickMap.insert(JOYSTICK_RS_UP,          "Joy-RS-Up"                     );
    m_JoystickRStickMap.insert(JOYSTICK_RS_DOWN,        "Joy-RS-Down"                   );
    m_JoystickRStickMap.insert(JOYSTICK_RS_LEFT,        "Joy-RS-Left"                   );
    m_JoystickRStickMap.insert(JOYSTICK_RS_RIGHT,       "Joy-RS-Right"                  );

    /* Joystick POV Angle Map */
    m_JoystickPOVMap.insert(JOYSTICK_POV_ANGLE_RELEASE, JOYSTICK_DPAD_RELEASE           );
    m_JoystickPOVMap.insert(JOYSTICK_POV_ANGLE_UP,      JOYSTICK_DPAD_UP                );
    m_JoystickPOVMap.insert(JOYSTICK_POV_ANGLE_DOWN,    JOYSTICK_DPAD_DOWN              );
    m_JoystickPOVMap.insert(JOYSTICK_POV_ANGLE_LEFT,    JOYSTICK_DPAD_LEFT              );
    m_JoystickPOVMap.insert(JOYSTICK_POV_ANGLE_RIGHT,   JOYSTICK_DPAD_RIGHT             );
    m_JoystickPOVMap.insert(JOYSTICK_POV_ANGLE_L_UP,    JOYSTICK_DPAD_L_UP              );
    m_JoystickPOVMap.insert(JOYSTICK_POV_ANGLE_L_DOWN,  JOYSTICK_DPAD_L_DOWN            );
    m_JoystickPOVMap.insert(JOYSTICK_POV_ANGLE_R_UP,    JOYSTICK_DPAD_R_UP              );
    m_JoystickPOVMap.insert(JOYSTICK_POV_ANGLE_R_DOWN,  JOYSTICK_DPAD_R_DOWN            );
}

void QKeyMapper_Worker::initSpecialOriginalKeysList()
{
    SpecialOriginalKeysList = QStringList() \
            << VJOY_MOUSE2LS_STR
            << VJOY_MOUSE2RS_STR
            << JOY_LS2MOUSE_STR
            << JOY_RS2MOUSE_STR
            << JOY_GYRO2MOUSE_STR
            << JOY_LS2VJOYLS_STR
            << JOY_RS2VJOYRS_STR
            << JOY_LS2VJOYRS_STR
            << JOY_RS2VJOYLS_STR
            << JOY_LT2VJOYLT_STR
            << JOY_RT2VJOYRT_STR
            ;
}

void QKeyMapper_Worker::initSpecialMappingKeysList()
{
    SpecialMappingKeysList = QStringList() \
            << KEY_BLOCKED_STR
            << KEYSEQUENCEBREAK_STR
            << KEY2MOUSE_UP_STR
            << KEY2MOUSE_DOWN_STR
            << KEY2MOUSE_LEFT_STR
            << KEY2MOUSE_RIGHT_STR
            << FUNC_REFRESH
            << FUNC_LOCKSCREEN
            << FUNC_SHUTDOWN
            << FUNC_REBOOT
            << FUNC_LOGOFF
            << FUNC_SLEEP
            << FUNC_HIBERNATE
            << MOUSE2VJOY_HOLD_KEY_STR
            << GYRO2MOUSE_HOLD_KEY_STR
            << GYRO2MOUSE_MOVE_KEY_STR
            << CROSSHAIR_NORMAL_STR
            << CROSSHAIR_TYPEA_STR
            << VJOY_LT_BRAKE_STR
            << VJOY_RT_BRAKE_STR
            << VJOY_LT_ACCEL_STR
            << VJOY_RT_ACCEL_STR
            ;
}

void QKeyMapper_Worker::initSpecialVirtualKeyCodeList()
{
    SpecialVirtualKeyCodeList = QList<quint8>() \
            << VK_KEY2MOUSE_UP
            << VK_KEY2MOUSE_DOWN
            << VK_KEY2MOUSE_LEFT
            << VK_KEY2MOUSE_RIGHT
            << VK_MOUSE2VJOY_HOLD
            << VK_GAMEPAD_HOME
            << VK_CROSSHAIR_NORMAL
            << VK_CROSSHAIR_TYPEA
            ;
}

#if 0
void QKeyMapper_Worker::initSkipReleaseModifiersKeysList()
{
    skipReleaseModifiersKeysList = QStringList() \
        << "VolumeMute"
        << "VolumeDown"
        << "VolumeUp"
        ;
}
#endif

#ifdef VIGEM_CLIENT_SUPPORT
void QKeyMapper_Worker::initViGEmKeyMap()
{
#if 0
    /* Virtual Joystick Buttons */
    JoyStickKeyMap.insert(VJOY_MOUSE2LS_STR             ,   (int)JOYSTICK_LS_MOUSE      );
    JoyStickKeyMap.insert(VJOY_MOUSE2RS_STR             ,   (int)JOYSTICK_RS_MOUSE      );

    JoyStickKeyMap.insert("vJoy-Key1(A/×)"              ,   (int)JOYSTICK_BUTTON_0      );
    JoyStickKeyMap.insert("vJoy-Key2(B/○)"              ,   (int)JOYSTICK_BUTTON_1      );
    JoyStickKeyMap.insert("vJoy-Key3(X/□)"              ,   (int)JOYSTICK_BUTTON_2      );
    JoyStickKeyMap.insert("vJoy-Key4(Y/△)"              ,   (int)JOYSTICK_BUTTON_3      );
    JoyStickKeyMap.insert("vJoy-Key5(LB)"               ,   (int)JOYSTICK_BUTTON_4      );
    JoyStickKeyMap.insert("vJoy-Key6(RB)"               ,   (int)JOYSTICK_BUTTON_5      );
    JoyStickKeyMap.insert("vJoy-Key7(Back)"             ,   (int)JOYSTICK_BUTTON_6      );
    JoyStickKeyMap.insert("vJoy-Key8(Start)"            ,   (int)JOYSTICK_BUTTON_7      );
    JoyStickKeyMap.insert("vJoy-Key9(LS-Click)"         ,   (int)JOYSTICK_BUTTON_8      );
    JoyStickKeyMap.insert("vJoy-Key10(RS-Click)"        ,   (int)JOYSTICK_BUTTON_9      );
    JoyStickKeyMap.insert("vJoy-Key13(Guide)"           ,   (int)JOYSTICK_BUTTON_10     );
    JoyStickKeyMap.insert("vJoy-Key11(LT)"              ,   (int)JOYSTICK_LEFT_TRIGGER  );
    JoyStickKeyMap.insert("vJoy-Key12(RT)"              ,   (int)JOYSTICK_RIGHT_TRIGGER );
    /* Virtual Joystick Special Buttons for ForzaHorizon */
    JoyStickKeyMap.insert(VJOY_LT_BRAKE_STR        ,   (int)JOYSTICK_LEFT_TRIGGER  );
    JoyStickKeyMap.insert(VJOY_LT_ACCEL_STR        ,   (int)JOYSTICK_LEFT_TRIGGER  );
    JoyStickKeyMap.insert(VJOY_RT_BRAKE_STR        ,   (int)JOYSTICK_RIGHT_TRIGGER );
    JoyStickKeyMap.insert(VJOY_RT_ACCEL_STR        ,   (int)JOYSTICK_RIGHT_TRIGGER );

    /* Virtual Joystick DPad Direction */
    JoyStickKeyMap.insert("vJoy-DPad-Up"                ,   (int)JOYSTICK_DPAD_UP       );
    JoyStickKeyMap.insert("vJoy-DPad-Down"              ,   (int)JOYSTICK_DPAD_DOWN     );
    JoyStickKeyMap.insert("vJoy-DPad-Left"              ,   (int)JOYSTICK_DPAD_LEFT     );
    JoyStickKeyMap.insert("vJoy-DPad-Right"             ,   (int)JOYSTICK_DPAD_RIGHT    );
    /* Virtual Joystick Left-Stick Direction */
    JoyStickKeyMap.insert("vJoy-LS-Up"                  ,   (int)JOYSTICK_LS_UP         );
    JoyStickKeyMap.insert("vJoy-LS-Down"                ,   (int)JOYSTICK_LS_DOWN       );
    JoyStickKeyMap.insert("vJoy-LS-Left"                ,   (int)JOYSTICK_LS_LEFT       );
    JoyStickKeyMap.insert("vJoy-LS-Right"               ,   (int)JOYSTICK_LS_RIGHT      );
    /* Virtual Joystick Right-Stick Direction */
    JoyStickKeyMap.insert("vJoy-RS-Up"                  ,   (int)JOYSTICK_RS_UP         );
    JoyStickKeyMap.insert("vJoy-RS-Down"                ,   (int)JOYSTICK_RS_DOWN       );
    JoyStickKeyMap.insert("vJoy-RS-Left"                ,   (int)JOYSTICK_RS_LEFT       );
    JoyStickKeyMap.insert("vJoy-RS-Right"               ,   (int)JOYSTICK_RS_RIGHT      );
#endif

    /* ViGEm Virtual Joystick Buttons */
    ViGEmButtonMap.insert("vJoy-Key1(A/×)"              ,   XUSB_GAMEPAD_A              );
    ViGEmButtonMap.insert("vJoy-Key2(B/○)"              ,   XUSB_GAMEPAD_B              );
    ViGEmButtonMap.insert("vJoy-Key3(X/□)"              ,   XUSB_GAMEPAD_X              );
    ViGEmButtonMap.insert("vJoy-Key4(Y/△)"              ,   XUSB_GAMEPAD_Y              );
    ViGEmButtonMap.insert("vJoy-Key5(LB)"               ,   XUSB_GAMEPAD_LEFT_SHOULDER  );
    ViGEmButtonMap.insert("vJoy-Key6(RB)"               ,   XUSB_GAMEPAD_RIGHT_SHOULDER );
    ViGEmButtonMap.insert("vJoy-Key7(Back)"             ,   XUSB_GAMEPAD_BACK           );
    ViGEmButtonMap.insert("vJoy-Key8(Start)"            ,   XUSB_GAMEPAD_START          );
    ViGEmButtonMap.insert("vJoy-Key9(LS-Click)"         ,   XUSB_GAMEPAD_LEFT_THUMB     );
    ViGEmButtonMap.insert("vJoy-Key10(RS-Click)"        ,   XUSB_GAMEPAD_RIGHT_THUMB    );
    ViGEmButtonMap.insert("vJoy-Key13(Guide)"           ,   XUSB_GAMEPAD_GUIDE          );
    /* ViGEm Virtual Joystick DPad Direction */
    ViGEmButtonMap.insert("vJoy-DPad-Up"                ,   XUSB_GAMEPAD_DPAD_UP        );
    ViGEmButtonMap.insert("vJoy-DPad-Down"              ,   XUSB_GAMEPAD_DPAD_DOWN      );
    ViGEmButtonMap.insert("vJoy-DPad-Left"              ,   XUSB_GAMEPAD_DPAD_LEFT      );
    ViGEmButtonMap.insert("vJoy-DPad-Right"             ,   XUSB_GAMEPAD_DPAD_RIGHT     );
}

bool QKeyMapper_Worker::isCursorAtBottomRight()
{
    bool ret = false;
    POINT pt;
    POINT bottomrightPoint = mousePositionAfterSetMouseToScreenBottomRight();
    if (GetCursorPos(&pt)) {
        if (pt.x == bottomrightPoint.x && pt.y == bottomrightPoint.y) {
            ret = true;
        }
    }

    return ret;
}
#endif

void QKeyMapper_Worker::clearAllBurstKeyTimersAndLockKeys()
{
    QList<QString> burstKeys;
    {
    QMutexLocker locker(&s_BurstKeyTimerMutex);
    burstKeys = s_BurstKeyTimerMap.keys();
    }
    for (const QString &burstKey : std::as_const(burstKeys)) {
        int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(burstKey);
        if (findindex >= 0) {
            if (true == QKeyMapper::KeyMappingDataList->at(findindex).Lock) {
                if (true == pressedLockKeysMap.contains(burstKey)){
                    (*QKeyMapper::KeyMappingDataList)[findindex].LockState = LOCK_STATE_LOCKOFF;
                    pressedLockKeysMap.remove(burstKey);
#ifdef DEBUG_LOGOUT_ON
                    qDebug("[clearAllBurstKeyTimersAndLockKeys] Key \"%s\" KeyDown LockState -> OFF", burstKey.toStdString().c_str());
#endif
                }
            }

            // stopBurstKeyTimerForce(burstKey, findindex);
            stopBurstKeyTimer(burstKey, findindex, QKeyMapper::KeyMappingDataList);
        }
    }

    {
    QMutexLocker locker(&s_BurstKeyTimerMutex);
    QList<QString> burstKeyPressKeys = s_BurstKeyPressTimerMap.keys();
    for (const QString &burstKey : std::as_const(burstKeyPressKeys)) {
        QTimer* timer = s_BurstKeyPressTimerMap.value(burstKey);
        timer->stop();
        delete timer;
        s_BurstKeyPressTimerMap.remove(burstKey);
    }

    s_BurstKeyPressTimerMap.clear();
    s_BurstKeyTimerMap.clear();
    }

    if (!s_isWorkerDestructing) {
        for (int index = 0; index < QKeyMapper::KeyMappingDataList->size(); index++) {
            (*QKeyMapper::KeyMappingDataList)[index].LockState = LOCK_STATE_LOCKOFF;
        }
    }
}

void QKeyMapper_Worker::clearAllPressedVirtualKeys()
{
    SendInputTaskController &controller = SendInputTask::s_GlobalSendInputTaskController;
    controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_FORCE;
    for (const QString &virtualkeystr : std::as_const(pressedVirtualKeysList)) {
        QStringList mappingKeyList = QStringList() << virtualkeystr;
        QString original_key = QString(CLEAR_VIRTUALKEYS);
        // emit_sendInputKeysSignal_Wrapper(mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
        QKeyMapper_Worker::getInstance()->sendInputKeys(-1, mappingKeyList, KEY_UP, original_key, SENDMODE_FORCE_STOP, controller);
    }
    controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;

    // QStringList pressedMappingOriginalKeys = pressedMappingKeysMap.keys();

    // for (const QString &original_key : std::as_const(pressedMappingOriginalKeys)){
    //     QStringList mappingKeyList = pressedMappingKeysMap.value(original_key);;
    //     emit_sendInputKeysSignal_Wrapper(mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
    // }
}

void QKeyMapper_Worker::clearPressedVirtualKeysOfMappingKeys(const QString &mappingkeys)
{
    QStringList mappingKeyListToClear = splitMappingKeyString(mappingkeys, SPLIT_WITH_PLUS, true);

//     QStringList pressedRealKeysListToCheck = pressedRealKeysListRemoveMultiInput;
//     for (const QString &blockedKey : blockedKeysList) {
//         pressedRealKeysListToCheck.removeAll(blockedKey);
//     }
//     QStringList pressedRealKeysListToCheckCopy = pressedRealKeysListToCheck;
//     for (const QString &realkey : pressedRealKeysListToCheckCopy) {
//         int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(realkey);
//         if (findindex >= 0 && !QKeyMapper::KeyMappingDataList->at(findindex).PassThrough) {
//             pressedRealKeysListToCheck.removeAll(realkey);
//         }
//     }
// #ifdef DEBUG_LOGOUT_ON
//     qDebug().nospace() << "[clearPressedVirtualKeysOfMappingKeys] pressedRealKeysListToCheck -> " << pressedRealKeysListToCheck;
// #endif

    SendInputTaskController &controller = SendInputTask::s_GlobalSendInputTaskController;
    controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_FORCE;
    for (const QString &virtualkeystr : std::as_const(pressedVirtualKeysList)) {
        if (mappingKeyListToClear.contains(virtualkeystr)
            // && !pressedRealKeysListToCheck.contains(virtualkeystr)
            ) {
            QStringList mappingKeyList = QStringList() << virtualkeystr;
            QString original_key = QString(CLEAR_VIRTUALKEYS);
            // emit_sendInputKeysSignal_Wrapper(mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
            QKeyMapper_Worker::getInstance()->sendInputKeys(-1, mappingKeyList, KEY_UP, original_key, SENDMODE_FORCE_STOP, controller);
        }
    }
    controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
}

void QKeyMapper_Worker::clearAllNormalPressedMappingKeys(bool restart, QList<MAP_KEYDATA> *keyMappingDataListToCheck)
{
    QHash<QString, QStringList> pressedMappingKeysMapCopy;
    {
    QMutexLocker locker(&s_PressedMappingKeysMapMutex);
    pressedMappingKeysMapCopy = pressedMappingKeysMap;
    }

    if (pressedMappingKeysMapCopy.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "\033[1;34m[clearAllNormalPressedMappingKeys] pressedMappingKeysMap is empty, skip clear NormalPressedMappingKeys.\033[0m";
#endif
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "\033[1;34m[clearAllNormalPressedMappingKeys] Current pressedMappingKeysMap -> " << pressedMappingKeysMap <<"\033[0m";
#endif

    QList<MAP_KEYDATA> *KeyMappingDataList_ToClear = QKeyMapper::KeyMappingDataList;

    QList<int> clearedRowIndexList;
    QStringList pressedOriginalKeys = pressedMappingKeysMapCopy.keys();
    for (const QString& original_key : std::as_const(pressedOriginalKeys)) {
        bool cleared = false;
        QString real_originalkey = getRealOriginalKey(original_key);
        SendInputTaskController *controller = Q_NULLPTR;
        if (SendInputTask::s_SendInputTaskControllerMap.contains(real_originalkey)) {
            controller = &SendInputTask::s_SendInputTaskControllerMap[real_originalkey];
        }
        int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(real_originalkey, KeyMappingDataList_ToClear);
        if (findindex >= 0) {
            // QStringList mappingKeyList = KeyMappingDataList_ToClear->at(findindex).Mapping_Keys;
            bool burst = KeyMappingDataList_ToClear->at(findindex).Burst;
            if (controller != Q_NULLPTR
                // && mappingKeyList.size() == 1
                && false == burst) {
                *controller->task_stop_flag = INPUTSTOP_SINGLE;
                controller->task_stop_condition->wakeAll();
                QStringList pure_mappingKeys = KeyMappingDataList_ToClear->at(findindex).Pure_MappingKeys;
                for (const QString &keycodeString : std::as_const(pure_mappingKeys)) {
                    if (pressedVirtualKeysList.contains(keycodeString)) {
#ifdef DEBUG_LOGOUT_ON
                        QString debugmessage = QString("[clearAllNormalPressedMappingKeys] VirtualKey \"%1\" of OriginalKey(%2) is still pressed down on Mapping stop, send KEY_UP to clear directly.").arg(keycodeString, real_originalkey);
                        qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                        if (keycodeString.startsWith(CROSSHAIR_PREFIX)) {
                            // emit showCrosshairStop_Signal in clearCustomKeyFlags()
                        }
                        else {
                            QKeyMapper_Worker::getInstance()->sendSpecialVirtualKey(keycodeString, KEY_UP);
                        }
                        pressedVirtualKeysList.removeAll(keycodeString);
                    }
                }
                cleared = true;
            }
        }

        if (cleared) {
            QMutexLocker locker(&s_PressedMappingKeysMapMutex);
            pressedMappingKeysMap.remove(original_key);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "\033[1;34m[clearAllNormalPressedMappingKeys] Clear OriginalKey(" << original_key << ") -> MappingKeys(" << pressedMappingKeysMapCopy[original_key] << "from pressedMappingKeysMap.\033[0m";
#endif
        }

        if (cleared) {
            if (!clearedRowIndexList.contains(findindex)) {
                clearedRowIndexList.append(findindex);
            }
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "\033[1;34m[clearAllNormalPressedMappingKeys] Cleared pressedMappingKeysMap -> " << pressedMappingKeysMap <<"\033[0m";
    qDebug().nospace().noquote() << "\033[1;34m[clearAllNormalPressedMappingKeys] Cleared pressedVirtualKeysList -> " << pressedVirtualKeysList <<"\033[0m";
    qDebug().nospace().noquote() << "\033[1;34m[clearAllNormalPressedMappingKeys] clearedRowIndexList -> " << clearedRowIndexList <<"\033[0m";
#endif

    bool pressedMappingKeysEmpty = false;
    {
    QMutexLocker locker(&s_PressedMappingKeysMapMutex);
    if (pressedMappingKeysMap.isEmpty()) {
        pressedMappingKeysEmpty = true;
    }
    }

    if (pressedMappingKeysEmpty) {
        for (const int &rowindex : clearedRowIndexList) {
            if (rowindex >= 0 && rowindex < KeyMappingDataList_ToClear->size()) {
                if (restart) {
                    resendRealKeyCodeOnStop(rowindex, true, keyMappingDataListToCheck);
                }
                else {
                    resendRealKeyCodeOnStop(rowindex);
                }
            }
        }
    }
}

void QKeyMapper_Worker::clearCustomKeyFlags(bool restart)
{
    s_Mouse2vJoy_Hold = false;
    s_Gyro2Mouse_MoveActive = false;
    s_Key2Mouse_Up = false;
    s_Key2Mouse_Down = false;
    s_Key2Mouse_Left = false;
    s_Key2Mouse_Right = false;
    if (!restart) {
        s_Crosshair_Normal = false;
        s_Crosshair_TypeA = false;
        emit QKeyMapper::getInstance()->showCrosshairStop_Signal(INITIAL_ROW_INDEX, CROSSHAIR_NORMAL_STR);
    }
}

void QKeyMapper_Worker::clearAllPressedRealCombinationKeys()
{
    QStringList newPressedRealKeysList;
    for (const QString& key : std::as_const(pressedRealKeysList)) {
        if (false == key.contains(SEPARATOR_PLUS)) {
            newPressedRealKeysList.append(key);
        }
    }
    pressedRealKeysList = newPressedRealKeysList;

    newPressedRealKeysList.clear();
    for (const QString& key : std::as_const(pressedRealKeysListRemoveMultiInput)) {
        if (false == key.contains(SEPARATOR_PLUS)) {
            newPressedRealKeysList.append(key);
        }
    }
    pressedRealKeysListRemoveMultiInput = newPressedRealKeysList;
}

void QKeyMapper_Worker::collectExchangeKeysList()
{
    exchangeKeysList.clear();

    QHash<QString, QString> singlemapping_keymap;
    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList))
    {
        if (keymapdata.Mapping_Keys.size() == 1)
        {
            singlemapping_keymap.insert(keymapdata.Original_Key, keymapdata.Mapping_Keys.constFirst());
        }
    }

    if (false == singlemapping_keymap.isEmpty())
    {
        QList<QString> singlemappingKeys = singlemapping_keymap.keys();
        for (const QString &key : std::as_const(singlemappingKeys))
        {
            if (singlemapping_keymap.value(singlemapping_keymap.value(key)) == key)
            {
                exchangeKeysList.append(key);
            }
        }
#ifdef DEBUG_LOGOUT_ON
        if (false == exchangeKeysList.isEmpty())
        {
            qDebug() << "exchangeKeysList -> " << exchangeKeysList;
        }
#endif
    }
}

bool QKeyMapper_Worker::isPressedMappingKeysContains(QString &key)
{
    bool pressedmappingkeys_isempty = false;
    {
    QMutexLocker locker(&s_PressedMappingKeysMapMutex);
    pressedmappingkeys_isempty = pressedMappingKeysMap.isEmpty();
    }
    if (pressedmappingkeys_isempty) {
        return false;
    }

    bool result = false;
    QList<QStringList> remainPressedMappingKeys;
    {
    QMutexLocker locker(&s_PressedMappingKeysMapMutex);
    remainPressedMappingKeys = pressedMappingKeysMap.values();
    }

    for (const QStringList &mappingkeys : std::as_const(remainPressedMappingKeys)){
        for (const QString &mapkey : std::as_const(mappingkeys)){
            if (mapkey == key) {
                result = true;
                break;
            }
        }
    }

    return result;
}

void QKeyMapper_Worker::initGlobalSendInputTaskController()
{
    SendInputTaskController &controller = SendInputTask::s_GlobalSendInputTaskController;
    controller.task_threadpool = new QThreadPool();
    controller.task_threadpool->setMaxThreadCount(1);
    controller.task_stop_mutex = new QMutex();
    controller.task_stop_condition = new QWaitCondition();
    controller.task_stop_flag = new QAtomicInt(INPUTSTOP_NONE);
    controller.task_rowindex = INITIAL_ROW_INDEX;
    controller.task_keyup_sent = false;
}

void QKeyMapper_Worker::resetGlobalSendInputTaskController()
{
    SendInputTaskController &controller = SendInputTask::s_GlobalSendInputTaskController;
    controller.task_threadpool->clear();
    controller.task_stop_condition->wakeAll();
    if (controller.task_stop_mutex->try_lock_for(std::chrono::milliseconds(TRY_LOCK_WAIT_TIME))) {
        controller.task_stop_mutex->unlock();
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "\033[1;34m[resetGlobalSendInputTaskController] Try lock wait failed!!!\033[0m";
#endif
    }
    *controller.task_stop_flag = INPUTSTOP_NONE;
    controller.task_threadpool->waitForDone(100);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "\033[1;34m[resetGlobalSendInputTaskController] Reset Global SendInputTask controllers finished.\033[0m";
#endif
}

void QKeyMapper_Worker::clearGlobalSendInputTaskController()
{
    SendInputTaskController &controller = SendInputTask::s_GlobalSendInputTaskController;

    controller.task_threadpool->clear();
    // Ensure mutex is unlocked before deleting it
    controller.task_stop_condition->wakeAll();
    if (controller.task_stop_mutex->try_lock_for(std::chrono::milliseconds(TRY_LOCK_WAIT_TIME))) {
        controller.task_stop_mutex->unlock();
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "\033[1;34m[clearGlobalSendInputTaskController] Try lock wait failed!!!\033[0m";
#endif
    }
    *controller.task_stop_flag = INPUTSTOP_NONE;

    // Delete the Controller objects
    delete controller.task_stop_mutex;
    delete controller.task_stop_condition;
    delete controller.task_stop_flag;

    // Delete the ThreadPool
    controller.task_threadpool->waitForDone(100);
    delete controller.task_threadpool;
}

void QKeyMapper_Worker::clearGlobalSendInputTaskControllerThreadPool()
{
    SendInputTaskController &controller = SendInputTask::s_GlobalSendInputTaskController;
    controller.task_threadpool->clear();
    controller.task_stop_condition->wakeAll();
}

void QKeyMapper_Worker::releasePressedRealKeysOfOriginalKeys(void)
{
    if (pressedRealKeysListRemoveMultiInput.isEmpty()) {
        return;
    }

    QStringList pressedRealKeysListToCheck = pressedRealKeysListRemoveMultiInput;

    QStringList keycodeStringListToRelease;
    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList)) {
        QStringList pure_originalkeylist = keymapdata.Pure_OriginalKeys;
        for (const QString &keycodeString : pure_originalkeylist) {
            if (pressedRealKeysListToCheck.contains(keycodeString)) {
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[releasePressedRealKeysOfOriginalKeys] RealKey \"%1\" is pressed down on Mapping Start/Restart, add to keycodeStringListToRelease.").arg(keycodeString);
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
                keycodeStringListToRelease.append(keycodeString);
            }
        }
    }

    keycodeStringListToRelease.removeDuplicates();

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "\033[1;34m[releasePressedRealKeysOfOriginalKeys] keycodeStringListToRelease -> " << keycodeStringListToRelease <<"\033[0m";
#endif

    if (keycodeStringListToRelease.isEmpty()) {
        return;
    }

    for (const QString &keycodeString : keycodeStringListToRelease) {
        QKeyMapper_Worker::getInstance()->sendSpecialVirtualKey(keycodeString, KEY_UP);
    }
}

#if 0
int QKeyMapper_Worker::makeKeySequenceInputarray(QStringList &keyseq_list, INPUT *input_array)
{
    int index = 0;
    int keycount = 0;
    INPUT *input_p = Q_NULLPTR;

    for (const QString &keyseq : std::as_const(keyseq_list)){
        QStringList mappingKeys = keyseq.split(SEPARATOR_PLUS);
        for (const QString &key : std::as_const(mappingKeys)){
            if (key == MOUSE_WHEEL_UP_STR || key == MOUSE_WHEEL_DOWN_STR) {
                input_p = &input_array[index];
                input_p->type = INPUT_MOUSE;
                input_p->mi.dwExtraInfo = VIRTUAL_MOUSE_WHEEL;
                input_p->mi.dwFlags = MOUSEEVENTF_WHEEL;
                if (key == MOUSE_WHEEL_UP_STR) {
                    input_p->mi.mouseData = WHEEL_DELTA;
                }
                else {
                    input_p->mi.mouseData = -WHEEL_DELTA;
                }
                keycount++;
                index++;
            }
            else if (true == VirtualMouseButtonMap.contains(key)) {
                V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);

                input_p = &input_array[index];
                input_p->type = INPUT_MOUSE;
                input_p->mi.mouseData = vmousecode.MouseXButton;
                input_p->mi.dwExtraInfo = VIRTUAL_KEY_SEND;
                input_p->mi.dwFlags = vmousecode.MouseDownCode;
                keycount++;
                index++;
            }
            else if (true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(key)) {
                V_KEYCODE vkeycode = QKeyMapper_Worker::VirtualKeyCodeMap.value(key);
                DWORD extenedkeyflag = 0;
                if (true == vkeycode.ExtenedFlag){
                    extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
                }
                else{
                    extenedkeyflag = 0;
                }

                input_p = &input_array[index];
                input_p->type = INPUT_KEYBOARD;
                input_p->ki.dwExtraInfo = VIRTUAL_KEY_SEND;
                input_p->ki.wVk = vkeycode.KeyCode;
                input_p->ki.wScan = MapVirtualKey(input_p->ki.wVk, MAPVK_VK_TO_VSC);
                input_p->ki.dwFlags = extenedkeyflag | 0;
                keycount++;
                index++;
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("makeKeySequenceInputarray(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
            }

            if (keycount >= SEND_INPUTS_MAX) {
#ifdef DEBUG_LOGOUT_ON
                qWarning() << "makeKeySequenceInputarray():" << "Too many keys" << keycount << "in key sequence [" << keyseq << "]";
#endif
                return 0;
            }
        }

        for(auto it = mappingKeys.crbegin(); it != mappingKeys.crend(); ++it) {
            QString key = (*it);
            if (key == MOUSE_WHEEL_UP_STR || key == MOUSE_WHEEL_DOWN_STR) {
                continue;
            }
            else if (true == VirtualMouseButtonMap.contains(key)) {
                V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);

                input_p = &input_array[index];
                input_p->type = INPUT_MOUSE;
                input_p->mi.mouseData = vmousecode.MouseXButton;
                input_p->mi.dwExtraInfo = VIRTUAL_KEY_SEND;
                input_p->mi.dwFlags = vmousecode.MouseUpCode;
                keycount++;
                index++;
            }
            else if (true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(key)) {
                V_KEYCODE vkeycode = QKeyMapper_Worker::VirtualKeyCodeMap.value(key);
                DWORD extenedkeyflag = 0;
                if (true == vkeycode.ExtenedFlag){
                    extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
                }
                else{
                    extenedkeyflag = 0;
                }

                input_p = &input_array[index];
                input_p->type = INPUT_KEYBOARD;
                input_p->ki.dwExtraInfo = VIRTUAL_KEY_SEND;
                input_p->ki.wVk = vkeycode.KeyCode;
                input_p->ki.wScan = MapVirtualKey(input_p->ki.wVk, MAPVK_VK_TO_VSC);
                input_p->ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                keycount++;
                index++;
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("makeKeySequenceInputarray(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
            }
        }
    }

    return keycount;
}
#endif

void QKeyMapper_Worker::sendKeySequenceList(QStringList &keyseq_list, QString &original_key, int sendmode, int sendvirtualkey_state)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[sendKeySequenceList]" << " original_key(" << original_key << "), sendmode(" << sendmode << ")";
#endif

    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(original_key);
    if (sendmode == SENDMODE_NORMAL) {
        if (findindex >= 0) {
            int repeat_mode = QKeyMapper::KeyMappingDataList->at(findindex).RepeatMode;
            if (repeat_mode != REPEAT_MODE_NONE) {
                sendvirtualkey_state = SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT;
#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[sendKeySequenceList] original_key(%1), repeat_mode(%2), sendvirtualkey_state = SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT").arg(original_key).arg(repeat_mode);
                qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
            }
        }
    }

    int index = 1;
    int size = keyseq_list.size();
    for (const QString &keyseq : std::as_const(keyseq_list)){
        QStringList mappingKeyList = QStringList() << keyseq;

        if (sendmode == SENDMODE_KEYSEQ_HOLDDOWN) {
            sendvirtualkey_state = SENDVIRTUALKEY_STATE_KEYSEQ_HOLDDOWN;
        }
        /* Add for KeySequenceHoldDown >>> */
        if (sendmode == SENDMODE_KEYSEQ_HOLDDOWN && index == size) {
            QString original_key_forKeySeq = original_key + ":" + KEYSEQUENCE_STR + HOLDDOWN_STR;
            emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key_forKeySeq, SENDMODE_KEYSEQ_HOLDDOWN, sendvirtualkey_state);
            emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key_forKeySeq, SENDMODE_KEYSEQ_HOLDDOWN, sendvirtualkey_state);
        }
        else {
            if (!s_runningKeySequenceOrikeyList.contains(original_key)) {
                s_runningKeySequenceOrikeyList.append(original_key);
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendKeySequenceList] Running KeySequence add OriginalKey:" << original_key << ", s_runningKeySequenceOrikeyList -> " << s_runningKeySequenceOrikeyList;
#endif
            }

            QString original_key_forKeySeq = original_key + ":" + KEYSEQUENCE_STR + QString::number(index);
            if (index == size) {
                QString finalPostStr = QString(":%1").arg(KEYSEQUENCE_FINAL_STR);
                original_key_forKeySeq.append(finalPostStr);
            }

            // if (SENDVIRTUALKEY_STATE_NORMAL == sendvirtualkey_state) {
            //     emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key_forKeySeq, SENDMODE_KEYSEQ_NORMAL, SENDVIRTUALKEY_STATE_KEYSEQ_NORMAL);
            //     emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key_forKeySeq, SENDMODE_KEYSEQ_NORMAL, SENDVIRTUALKEY_STATE_KEYSEQ_NORMAL);
            // }
            // else {
            //     emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key_forKeySeq, SENDMODE_KEYSEQ_NORMAL, sendvirtualkey_state);
            //     emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key_forKeySeq, SENDMODE_KEYSEQ_NORMAL, sendvirtualkey_state);
            // }
            emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_DOWN, original_key_forKeySeq, SENDMODE_KEYSEQ_NORMAL, sendvirtualkey_state);
            emit_sendInputKeysSignal_Wrapper(findindex, mappingKeyList, KEY_UP, original_key_forKeySeq, SENDMODE_KEYSEQ_NORMAL, sendvirtualkey_state);
        }
        /* Add for KeySequenceHoldDown <<< */

        index += 1;
    }
}

QKeyMapper_Hook_Proc::QKeyMapper_Hook_Proc(QObject *parent)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "QKeyMapper_Hook_Proc() Called.";
#endif
    Q_UNUSED(parent);

#ifdef QT_DEBUG
    if (IsDebuggerPresent()) {
        s_LowLevelKeyboardHook_Enable = false;
        s_LowLevelMouseHook_Enable = false;
#ifdef DEBUG_LOGOUT_ON
        qDebug("QKeyMapper_Hook_Proc() Win_Dbg = TRUE, set QKeyMapper_Hook_Proc::s_LowLevelMouseHook_Enable to FALSE");
#endif
    }
#endif
}

QKeyMapper_Hook_Proc::~QKeyMapper_Hook_Proc()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "~QKeyMapper_Hook_Proc() Called.";
#endif
}

void QKeyMapper_Hook_Proc::HookProcThreadStarted()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("[HookProcThreadStarted] currentThread -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

    if (s_LowLevelKeyboardHook_Enable) {
        if (s_KeyHook == Q_NULLPTR) {
            s_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper_Worker::LowLevelKeyboardHookProc, GetModuleHandle(Q_NULLPTR), 0);
#ifdef DEBUG_LOGOUT_ON
            if (s_KeyHook != Q_NULLPTR) {
                qDebug("[SetHookProc] HookProcThreadStarted() Keyboard SetWindowsHookEx Success. -> 0x%08X", s_KeyHook);
            }
#endif
        }
    }
    if (s_LowLevelMouseHook_Enable) {
        if (s_MouseHook == Q_NULLPTR) {
            s_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, QKeyMapper_Worker::LowLevelMouseHookProc, GetModuleHandle(Q_NULLPTR), 0);
#ifdef DEBUG_LOGOUT_ON
            if (s_MouseHook != Q_NULLPTR) {
                qDebug("[SetHookProc] HookProcThreadStarted() Mouse SetWindowsHookEx Success. -> 0x%08X", s_MouseHook);
            }
#endif
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qInfo("[SetHookProc] HookProcThreadStarted() Keyboard Hook & Mouse Hook Started.");
#endif
}

void QKeyMapper_Hook_Proc::HookProcThreadFinished()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("[HookProcThreadFinished] currentThread -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

    bool unhook_ret = 0;
    if (s_KeyHook != Q_NULLPTR){
        void* keyboardhook_p = (void*)s_KeyHook;
        unhook_ret = UnhookWindowsHookEx(s_KeyHook);
        s_KeyHook = Q_NULLPTR;
        Q_UNUSED(keyboardhook_p);

#ifdef DEBUG_LOGOUT_ON
        if (0 == unhook_ret) {
            qDebug() << "[SetHookProc]" << "HookProcThreadFinished() Keyboard UnhookWindowsHookEx Failure! LastError:" << GetLastError();
        }
        else {
            qDebug("[SetHookProc] HookProcThreadFinished() Keyboard UnhookWindowsHookEx Success. -> 0x%08X", keyboardhook_p);
        }
#endif
    }

    if (s_MouseHook != Q_NULLPTR) {
        void* mousehook_p = (void*)s_MouseHook;
        unhook_ret = UnhookWindowsHookEx(s_MouseHook);
        s_MouseHook = Q_NULLPTR;
        Q_UNUSED(mousehook_p);

#ifdef DEBUG_LOGOUT_ON
        if (0 == unhook_ret) {
            qDebug() << "[SetHookProc]" << "HookProcThreadFinished() Mouse UnhookWindowsHookEx Failure! LastError:" << GetLastError();
        }
        else {
            qDebug("[SetHookProc] HookProcThreadFinished() Mouse UnhookWindowsHookEx Success. -> 0x%08X", mousehook_p);
        }
#endif
    }
    Q_UNUSED(unhook_ret);

#ifdef DEBUG_LOGOUT_ON
    qInfo("[SetHookProc] HookProcThreadFinished() Keyboard Hook & Mouse Hook Stopped.");
#endif
}

void QKeyMapper_Hook_Proc::onSetHookProcKeyHook()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("[onSetHookProcKeyHook] HookProcThread Hookproc Start.");
#endif

    QKeyMapper_Worker::clearAllLongPressTimers();
    QKeyMapper_Worker::clearAllDoublePressTimers();

#ifdef DEBUG_LOGOUT_ON
    qDebug("[onSetHookProcKeyHook] HookProcThread Hookproc End.");
#endif
}

void QKeyMapper_Hook_Proc::onSetHookProcKeyUnHook()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("[onSetHookProcKeyUnHook] HookProcThread Unhookproc Start.");
#endif

    QKeyMapper_Worker::clearAllLongPressTimers();
    QKeyMapper_Worker::clearAllDoublePressTimers();

#ifdef DEBUG_LOGOUT_ON
    qDebug("[onSetHookProcKeyUnHook] HookProcThread Unhookproc End.");
#endif
}

void QKeyMapper_Hook_Proc::onSetHookProcKeyMappingRestart()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("[onSetHookProcKeyHook] HookProcThread MappingRestart Start.");
#endif

    QKeyMapper_Worker::clearAllLongPressTimers();
    QKeyMapper_Worker::clearAllDoublePressTimers();

#ifdef DEBUG_LOGOUT_ON
    qDebug("[onSetHookProcKeyHook] HookProcThread MappingRestart End.");
#endif
}

bool EnablePrivilege(LPCWSTR privilege)
{
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        TOKEN_PRIVILEGES tkp;

        if (LookupPrivilegeValue(NULL, privilege, &tkp.Privileges[0].Luid))
        {
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if (AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, NULL))
            {
                CloseHandle(hToken);
                return (GetLastError() == ERROR_SUCCESS);
            }
        }

        CloseHandle(hToken);
    }

    return false;
}

bool DisablePrivilege(LPCWSTR privilege)
{
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        TOKEN_PRIVILEGES tkp;

        if (LookupPrivilegeValue(NULL, privilege, &tkp.Privileges[0].Luid))
        {
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = 0; // PRIVILEGE_DISABLED

            if (AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, NULL))
            {
                CloseHandle(hToken);
                return (GetLastError() == ERROR_SUCCESS);
            }
        }

        CloseHandle(hToken);
    }

    return FALSE;
}

QStringList splitMappingKeyString(const QString &mappingkeystr, int split_type, bool pure_keys)
{
    // Modify the regex to capture SendText(…) followed by any non-separator characters
    static QRegularExpression plusandnext_split_regex(R"((SendText\([^)]+\)[^+»]*)|([^+»]+))");
    static QRegularExpression next_split_regex(R"((SendText\([^)]+\)[^»]*)|([^»]+))");
    static QRegularExpression plus_split_regex(R"((SendText\([^)]+\)[^+]*)|([^+]+))");
    static QRegularExpression mapkey_regex(R"(^([↓↑⇵！]?)([^\[⏱]+)(?:\[(\d{1,3})\])?(?:⏱(\d+))?$)");

    QStringList splitted_mappingkeys;
    QString remainingString = mappingkeystr;

    // Use a global match to capture both SendText(…) and normal keys
    QRegularExpressionMatchIterator iter;
    if (SPLIT_WITH_PLUSANDNEXT == split_type) {
        iter = plusandnext_split_regex.globalMatch(remainingString);
    }
    else if (SPLIT_WITH_NEXT == split_type) {
        iter = next_split_regex.globalMatch(remainingString);
    }
    else {
        iter = plus_split_regex.globalMatch(remainingString);
    }

    QString sendtext_start = "SendText(";
    QString sendtext_end = ")";
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        if (match.hasMatch()) {
            QString keystr = match.captured(0);
            if (pure_keys) {
                QRegularExpressionMatch mapkey_match = mapkey_regex.match(keystr);
                if (mapkey_match.hasMatch()) {
                    keystr = mapkey_match.captured(2);
                }
            }

            if (SPLIT_WITH_NEXT == split_type
                && !splitted_mappingkeys.isEmpty()
                && splitted_mappingkeys.constLast().contains(sendtext_start)
                && !splitted_mappingkeys.constLast().contains(sendtext_end)) {
                splitted_mappingkeys.last().append(SEPARATOR_NEXTARROW + keystr);
            }
            else {
                splitted_mappingkeys.append(keystr);
            }
        }
    }

    return splitted_mappingkeys;
}

QStringList splitOriginalKeyString(const QString &originalkeystr, bool pure_keys)
{
    QString original_key = QKeyMapper::getOriginalKeyStringWithoutSuffix(originalkeystr);
    QStringList orikeylist = original_key.split(SEPARATOR_PLUS);

    if (pure_keys) {
        QStringList pure_orikeylist;
        static QRegularExpression orikey_regex(R"(^(.+?)(?:@([0-9]))?$)");
        for (const QString &orikey : std::as_const(orikeylist)) {
            QRegularExpressionMatch orikey_match = orikey_regex.match(orikey);
            if (orikey_match.hasMatch()) {
                pure_orikeylist.append(orikey_match.captured(1));
            }
        }
        orikeylist = pure_orikeylist;
    }

    return orikeylist;
}

QString getRealOriginalKey(const QString &original_key)
{
    return original_key.left(original_key.indexOf(":"));
}

SendInputTask::SendInputTask(int rowindex, const QStringList &inputKeys, int keyupdown, const QString &original_key, int sendmode, int sendvirtualkey_state, QList<MAP_KEYDATA> *keyMappingDataList) :
    m_rowindex(rowindex),
    m_inputKeys(inputKeys),
    m_keyupdown(keyupdown),
    m_original_key(original_key),
    m_real_originalkey(original_key),
    m_sendmode(sendmode),
    m_sendvirtualkey_state(sendvirtualkey_state)
{
    // Set the real original key
    m_real_originalkey = getRealOriginalKey(original_key);

    // Save current mapping table pointer to avoid array bounds issues during tab switching
    if (keyMappingDataList != Q_NULLPTR) {
        m_keyMappingDataList = keyMappingDataList;
    }

    {
        // Lock the map access mutex
        QMutexLocker mapLocker(&s_SendInputTaskControllerMapMutex);

        // Check if the Controller for this original key already exists
        if (!s_SendInputTaskControllerMap.contains(m_real_originalkey)) {
            // Create a new Controller struct and insert it into the map
            SendInputTaskController controller;
            controller.task_threadpool = new QThreadPool();
            controller.task_threadpool->setMaxThreadCount(1);
            controller.task_stop_mutex = new QMutex();
            controller.task_stop_condition = new QWaitCondition();
            controller.task_stop_flag = new QAtomicInt(INPUTSTOP_NONE);
            controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
            controller.task_rowindex = INITIAL_ROW_INDEX;
            s_SendInputTaskControllerMap.insert(m_real_originalkey, controller);
        }
        else {
            SendInputTaskController *controller = Q_NULLPTR;
            controller = &s_SendInputTaskControllerMap[m_real_originalkey];
            if (keyupdown == KEY_UP) {
                controller->task_keyup_sent = true;
            }
            else {
                controller->task_keyup_sent = false;
            }
        }
    }
}

void SendInputTask::run()
{
    // Retrieve the controller for m_real_originalkey
    SendInputTaskController *controller = Q_NULLPTR;
    controller = &s_SendInputTaskControllerMap[m_real_originalkey];

#ifdef DEBUG_LOGOUT_ON
    QString threadIdStr = QString("0x%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 8, 16, QChar('0')).toUpper();
    qDebug().nospace().noquote() << "\033[1;34m[SendInputTask::run] Task Run Start Thread -> ID:" << threadIdStr << ", Originalkey[" << m_original_key << "], Real_originalkey[" << m_real_originalkey << "] " << ((m_keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << ", MappingKeys[" << m_inputKeys << "], SendMode:" << m_sendmode << "], RowIndex:" << m_rowindex << "\033[0m";
#endif

    if (m_rowindex >= 0) {
        controller->task_rowindex = m_rowindex;
    }
    else {
        controller->task_rowindex = INITIAL_ROW_INDEX;
    }

    // Execute the input sending task
    controller->sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
    if (m_sendvirtualkey_state == SENDVIRTUALKEY_STATE_MODIFIERS) {
        controller->sendvirtualkey_state = SENDVIRTUALKEY_STATE_MODIFIERS;
    }
    else if (m_sendvirtualkey_state == SENDVIRTUALKEY_STATE_BURST_TIMEOUT) {
        controller->sendvirtualkey_state = SENDVIRTUALKEY_STATE_BURST_TIMEOUT;
    }
    else if (m_sendvirtualkey_state == SENDVIRTUALKEY_STATE_BURST_STOP) {
        controller->sendvirtualkey_state = SENDVIRTUALKEY_STATE_BURST_STOP;
    }
    else if (m_sendvirtualkey_state == SENDVIRTUALKEY_STATE_KEYSEQ_NORMAL) {
        controller->sendvirtualkey_state = SENDVIRTUALKEY_STATE_KEYSEQ_NORMAL;
    }
    else if (m_sendvirtualkey_state == SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT) {
        controller->sendvirtualkey_state = SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT;
    }
    else if (m_sendvirtualkey_state == SENDVIRTUALKEY_STATE_KEYSEQ_HOLDDOWN) {
        controller->sendvirtualkey_state = SENDVIRTUALKEY_STATE_KEYSEQ_HOLDDOWN;
    }
    else if (m_sendvirtualkey_state == SENDVIRTUALKEY_STATE_FORCE) {
        controller->sendvirtualkey_state = SENDVIRTUALKEY_STATE_FORCE;
    }
    QKeyMapper_Worker::getInstance()->sendInputKeys(m_rowindex, m_inputKeys, m_keyupdown, m_original_key, m_sendmode, *controller, m_keyMappingDataList);
    controller->sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
    controller->task_rowindex = INITIAL_ROW_INDEX;

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "\033[1;34m[SendInputTask::run] Task Run Finished Thread -> ID:" << threadIdStr << ", Originalkey[" << m_original_key << "], Real_originalkey[" << m_real_originalkey << "] " << ((m_keyupdown == KEY_DOWN) ? "KeyDown" : "KeyUp") << ", MappingKeys[" << m_inputKeys << "], SendMode:" << m_sendmode << "\033[0m";
#endif
}

void SendInputTask::initSendInputTaskControllerMap()
{
    // Lock the global mutex to ensure thread-safe access to the map
    QMutexLocker mapLocker(&s_SendInputTaskControllerMapMutex);

    // Iterate through KeyMappingDataList
    for (const MAP_KEYDATA &keymapdata : std::as_const(*QKeyMapper::KeyMappingDataList)) {
        // Get the original key from keymapdata
        QString originalKey = keymapdata.Original_Key;

        // Check if the Controller for this original key already exists
        if (!s_SendInputTaskControllerMap.contains(originalKey)) {
            // Create a new Controller struct and insert it into the map
            SendInputTaskController controller;
            controller.task_threadpool = new QThreadPool();
            controller.task_threadpool->setMaxThreadCount(1);
            controller.task_stop_mutex = new QMutex();
            controller.task_stop_condition = new QWaitCondition();
            controller.task_stop_flag = new QAtomicInt(INPUTSTOP_NONE);
            controller.sendvirtualkey_state = SENDVIRTUALKEY_STATE_NORMAL;
            controller.task_rowindex = INITIAL_ROW_INDEX;
            controller.task_keyup_sent = false;
            s_SendInputTaskControllerMap.insert(originalKey, controller);
        }
    }
}

void SendInputTask::clearSendInputTaskControllerMap()
{
    // Lock the global mutex to ensure thread-safe access to the map
    QMutexLocker mapLocker(&s_SendInputTaskControllerMapMutex);

    // Iterate through the map to delete each QMutex pointer
    for (SendInputTaskController &controller : s_SendInputTaskControllerMap)
    {
        controller.task_threadpool->clear();
        // Ensure mutex is unlocked before deleting it
        controller.task_stop_condition->wakeAll();
        if (controller.task_stop_mutex->try_lock_for(std::chrono::milliseconds(TRY_LOCK_WAIT_TIME))) {
            controller.task_stop_mutex->unlock();
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "\033[1;34m[clearSendInputTaskControllerMap] Try lock wait failed, skip this lock!!!\033[0m";
            continue;
#endif
        }
        *controller.task_stop_flag = INPUTSTOP_NONE;

        // Delete the Controller objects
        delete controller.task_stop_mutex;
        delete controller.task_stop_condition;
        delete controller.task_stop_flag;

        // Delete the ThreadPool
        controller.task_threadpool->waitForDone(100);
        delete controller.task_threadpool;
    }

    // Clear the entire map
    s_SendInputTaskControllerMap.clear();

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "\033[1;34m[SendInputTask::clearSendInputTaskControllerMap] Clear all SendInputTask controllers finished.\033[0m";
#endif
}

void SendInputTask::clearSendInputTaskControllerThreadPool()
{
    for (SendInputTaskController &controller : s_SendInputTaskControllerMap)
    {
        controller.task_threadpool->clear();
        controller.task_stop_condition->wakeAll();
    }
}
