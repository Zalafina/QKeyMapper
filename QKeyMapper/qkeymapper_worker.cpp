#include "qkeymapper.h"
#include "qkeymapper_worker.h"

static const int KEY_INIT = -1;
static const int KEY_UP = 0;
static const int KEY_DOWN = 1;

static const int SENDMODE_NORMAL        = 1;
static const int SENDMODE_FORCE_STOP    = 2;

static const int MOUSE_WHEEL_UP = 1;
static const int MOUSE_WHEEL_DOWN = 2;

static const int MOUSE_WHEEL_KEYUP_WAITTIME = 20;

static const int SETMOUSEPOSITION_WAITTIME_MAX = 100;

static const int MAPPING_WAITTIME_MIN = 0;
static const int MAPPING_WAITTIME_MAX = 1000;

static const WORD XBUTTON_NONE = 0x0000;

static const int JOYSTICK_POV_ANGLE_RELEASE = -1;
static const int JOYSTICK_POV_ANGLE_UP      = 0;
static const int JOYSTICK_POV_ANGLE_DOWN    = 180;
static const int JOYSTICK_POV_ANGLE_LEFT    = 270;
static const int JOYSTICK_POV_ANGLE_RIGHT   = 90;
static const int JOYSTICK_POV_ANGLE_L_UP    = 315;
static const int JOYSTICK_POV_ANGLE_L_DOWN  = 225;
static const int JOYSTICK_POV_ANGLE_R_UP    = 45;
static const int JOYSTICK_POV_ANGLE_R_DOWN  = 135;

static const int JOYSTICK_AXIS_LS_HORIZONTAL    = 0;
static const int JOYSTICK_AXIS_LS_VERTICAL      = 1;
static const int JOYSTICK_AXIS_RS_HORIZONTAL    = 2;
static const int JOYSTICK_AXIS_RS_VERTICAL      = 3;
static const int JOYSTICK_AXIS_LT_BUTTON        = 4;
static const int JOYSTICK_AXIS_RT_BUTTON        = 5;

static const qreal JOYSTICK_AXIS_NEAR_ZERO_THRESHOLD = 1e-04;

static const qreal JOYSTICK_AXIS_LT_RT_KEYUP_THRESHOLD      = 0.15;
static const qreal JOYSTICK_AXIS_LT_RT_KEYDOWN_THRESHOLD    = 0.5;

static const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD                = -0.5;
static const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD              = 0.5;
static const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MIN_THRESHOLD       = -0.15;
static const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MAX_THRESHOLD       = 0.15;

static const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD            = -0.5;
static const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD           = 0.5;
static const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MIN_THRESHOLD     = -0.15;
static const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MAX_THRESHOLD     = 0.15;

static const qreal JOYSTICK2MOUSE_AXIS_MINUS_LOW_THRESHOLD  = -0.25;
static const qreal JOYSTICK2MOUSE_AXIS_MINUS_MID_THRESHOLD  = -0.50;
static const qreal JOYSTICK2MOUSE_AXIS_MINUS_HIGH_THRESHOLD  = -0.75;
static const qreal JOYSTICK2MOUSE_AXIS_PLUS_LOW_THRESHOLD   = 0.25;
static const qreal JOYSTICK2MOUSE_AXIS_PLUS_MID_THRESHOLD   = 0.50;
static const qreal JOYSTICK2MOUSE_AXIS_PLUS_HIGH_THRESHOLD   = 0.75;

static const int MOUSE_CURSOR_BOTTOMRIGHT_X = 65535;
static const int MOUSE_CURSOR_BOTTOMRIGHT_Y = 65535;

static const int JOY2MOUSE_CYCLECHECK_TIMEOUT = 2;

#ifdef VIGEM_CLIENT_SUPPORT
static const USHORT VIRTUALGAMPAD_VENDORID_X360     = 0x045E;
static const USHORT VIRTUALGAMPAD_PRODUCTID_X360    = 0xABCD;

static const USHORT VIRTUALGAMPAD_VENDORID_DS4      = 0x054C;
static const USHORT VIRTUALGAMPAD_PRODUCTID_DS4     = 0x05C4;
static const char *VIRTUALGAMPAD_SERIAL_DS4 = "c0-13-37-b6-0b-94";

static const BYTE XINPUT_TRIGGER_MIN     = 0;
static const BYTE XINPUT_TRIGGER_MAX     = 255;

static const SHORT XINPUT_THUMB_MIN     = -32768;
static const SHORT XINPUT_THUMB_RELEASE = 0;
static const SHORT XINPUT_THUMB_MAX     = 32767;

static const qreal THUMB_DISTANCE_MAX   = 32767;

static const BYTE AUTO_BRAKE_ADJUST_VALUE = 8;
static const BYTE AUTO_ACCEL_ADJUST_VALUE = 8;
static const BYTE AUTO_BRAKE_DEFAULT = 11 * AUTO_BRAKE_ADJUST_VALUE + 7;
static const BYTE AUTO_ACCEL_DEFAULT = 31 * AUTO_ACCEL_ADJUST_VALUE + 7;

static const double GRIP_THRESHOLD_MAX = 2000000.00000;

static const int AUTO_ADJUST_NONE   = 0b00;
static const int AUTO_ADJUST_BRAKE  = 0b01;
static const int AUTO_ADJUST_ACCEL  = 0b10;
static const int AUTO_ADJUST_BOTH   = 0b11;

static const int VJOY_UPDATE_NONE           = 0;
static const int VJOY_UPDATE_BUTTONS        = 1;
static const int VJOY_UPDATE_JOYSTICKS      = 2;
static const int VJOY_UPDATE_AUTO_BUTTONS   = 3;

static const int VIRTUAL_JOYSTICK_SENSITIVITY_MIN = 1;
static const int VIRTUAL_JOYSTICK_SENSITIVITY_MAX = 1000;
static const int VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT = 12;

static const int MOUSE2VJOY_RESET_TIMEOUT = 200;
static const int VJOY_KEYUP_WAITTIME = 20;
#endif

static const char *VJOY_MOUSE2LS_STR = "vJoy-Mouse2LS";
static const char *VJOY_MOUSE2RS_STR = "vJoy-Mouse2RS";

static const char *VJOY_LT_BRAKE_STR = "vJoy-Key11(LT)_BRAKE";
static const char *VJOY_RT_BRAKE_STR = "vJoy-Key12(RT)_BRAKE";
static const char *VJOY_LT_ACCEL_STR = "vJoy-Key11(LT)_ACCEL";
static const char *VJOY_RT_ACCEL_STR = "vJoy-Key12(RT)_ACCEL";

static const char *JOY_LS2MOUSE_STR = "Joy-LS2Mouse";
static const char *JOY_RS2MOUSE_STR = "Joy-RS2Mouse";

static const char *VIRTUAL_GAMEPAD_X360 = "X360";
static const char *VIRTUAL_GAMEPAD_DS4  = "DS4";

static const char *MOUSE_STR_WHEEL_UP = "Mouse-WheelUp";
static const char *MOUSE_STR_WHEEL_DOWN = "Mouse-WheelDown";

static const ULONG_PTR VIRTUAL_KEYBOARD_PRESS = 0xACBDACBD;
static const ULONG_PTR VIRTUAL_MOUSE_CLICK = 0xCEDFCEDF;
static const ULONG_PTR VIRTUAL_MOUSE_MOVE = 0xBFBCBFBC;
static const ULONG_PTR VIRTUAL_MOUSE_WHEEL = 0xEBFAEBFA;
static const ULONG_PTR VIRTUAL_WIN_PLUS_D = 0xDBDBDBDB;

bool QKeyMapper_Worker::s_isWorkerDestructing = false;
#ifdef HOOKSTART_ONSTARTUP
QAtomicBool QKeyMapper_Worker::s_AtomicHookProcStart = QAtomicBool();
#endif
bool QKeyMapper_Worker::s_forceSendVirtualKey = false;
QHash<QString, V_KEYCODE> QKeyMapper_Worker::VirtualKeyCodeMap = QHash<QString, V_KEYCODE>();
QHash<QString, V_MOUSECODE> QKeyMapper_Worker::VirtualMouseButtonMap = QHash<QString, V_MOUSECODE>();
QHash<WPARAM, QString> QKeyMapper_Worker::MouseButtonNameMap = QHash<WPARAM, QString>();
#ifdef MOUSEBUTTON_CONVERT
QHash<QString, QString> QKeyMapper_Worker::MouseButtonNameConvertMap = QHash<QString, QString>();
#endif
QHash<QString, int> QKeyMapper_Worker::JoyStickKeyMap = QHash<QString, int>();
QHash<QString, QHotkey*> QKeyMapper_Worker::ShortcutsMap = QHash<QString, QHotkey*>();
#ifdef VIGEM_CLIENT_SUPPORT
QHash<QString, XUSB_BUTTON> QKeyMapper_Worker::ViGEmButtonMap = QHash<QString, XUSB_BUTTON>();
#endif
QStringList QKeyMapper_Worker::pressedRealKeysList = QStringList();
QStringList QKeyMapper_Worker::pressedVirtualKeysList = QStringList();
QStringList QKeyMapper_Worker::pressedShortcutKeysList = QStringList();
#ifdef VIGEM_CLIENT_SUPPORT
QStringList QKeyMapper_Worker::pressedvJoyLStickKeys = QStringList();
QStringList QKeyMapper_Worker::pressedvJoyRStickKeys = QStringList();
QStringList QKeyMapper_Worker::pressedvJoyButtons = QStringList();
#endif
QHash<QString, QStringList> QKeyMapper_Worker::pressedMappingKeysMap = QHash<QString, QStringList>();
QStringList QKeyMapper_Worker::pressedLockKeysList = QStringList();
QStringList QKeyMapper_Worker::exchangeKeysList = QStringList();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
QRecursiveMutex QKeyMapper_Worker::sendinput_mutex = QRecursiveMutex();
#else
QMutex QKeyMapper_Worker::sendinput_mutex(QMutex::Recursive);
#endif
#ifdef DINPUT_TEST
GetDeviceStateT QKeyMapper_Worker::FuncPtrGetDeviceState = Q_NULLPTR;
GetDeviceDataT QKeyMapper_Worker::FuncPtrGetDeviceData = Q_NULLPTR;
int QKeyMapper_Worker::dinput_timerid = 0;
#endif
#ifdef VIGEM_CLIENT_SUPPORT
PVIGEM_CLIENT QKeyMapper_Worker::s_ViGEmClient = Q_NULLPTR;
PVIGEM_TARGET QKeyMapper_Worker::s_ViGEmTarget = Q_NULLPTR;
XUSB_REPORT QKeyMapper_Worker::s_ViGEmTarget_Report = XUSB_REPORT();
BYTE QKeyMapper_Worker::s_Auto_Brake = AUTO_BRAKE_DEFAULT;
BYTE QKeyMapper_Worker::s_Auto_Accel = AUTO_ACCEL_DEFAULT;
BYTE QKeyMapper_Worker::s_last_Auto_Brake = 0;
BYTE QKeyMapper_Worker::s_last_Auto_Accel = 0;
QKeyMapper_Worker::GripDetectState QKeyMapper_Worker::s_GripDetect_EnableState = QKeyMapper_Worker::GRIPDETECT_NONE;
QKeyMapper_Worker::ViGEmClient_ConnectState QKeyMapper_Worker::s_ViGEmClient_ConnectState = VIGEMCLIENT_DISCONNECTED;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
QRecursiveMutex QKeyMapper_Worker::s_ViGEmClient_Mutex = QRecursiveMutex();
#else
QMutex QKeyMapper_Worker::s_ViGEmClient_Mutex(QMutex::Recursive);
#endif
QPoint QKeyMapper_Worker::s_Mouse2vJoy_delta = QPoint();
QPoint QKeyMapper_Worker::s_Mouse2vJoy_prev = QPoint();
QKeyMapper_Worker::Mouse2vJoyState QKeyMapper_Worker::s_Mouse2vJoy_EnableState = QKeyMapper_Worker::MOUSE2VJOY_NONE;
#endif
QKeyMapper_Worker::Joy2MouseState QKeyMapper_Worker::s_Joy2Mouse_EnableState = QKeyMapper_Worker::JOY2MOUSE_NONE;
Joystick_AxisState QKeyMapper_Worker::s_JoyAxisState = Joystick_AxisState();

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
    m_sendInputTask(Q_NULLPTR),
    m_sendInputStopCondition(),
    m_sendInputStopMutex(),
    m_sendInputStopFlag(false),
    m_JoystickCapture(false),
#ifdef DINPUT_TEST
    m_DirectInput(Q_NULLPTR),
#endif
#ifdef VIGEM_CLIENT_SUPPORT
    m_Mouse2vJoyResetTimer(this),
#endif
    m_Joy2MouseCycleTimer(this),
    skipReleaseModifiersKeysList(),
    m_UdpSocket(Q_NULLPTR),
    m_BurstTimerMap(),
    m_BurstKeyUpTimerMap(),
    m_JoystickButtonMap(),
    m_JoystickDPadMap(),
    m_JoystickLStickMap(),
    m_JoystickRStickMap(),
    m_JoystickPOVMap()
{
    qRegisterMetaType<HWND>("HWND");
    qRegisterMetaType<V_KEYCODE>("V_KEYCODE");
    qRegisterMetaType<V_MOUSECODE>("V_MOUSECODE");
    qRegisterMetaType<QJoystickPOVEvent>("QJoystickPOVEvent");
    qRegisterMetaType<QJoystickAxisEvent>("QJoystickAxisEvent");
    qRegisterMetaType<QJoystickButtonEvent>("QJoystickButtonEvent");
    qRegisterMetaType<Qt::KeyboardModifiers>("Qt::KeyboardModifiers");

    Q_UNUSED(parent);

    QObject::connect(this, &QKeyMapper_Worker::setKeyHook_Signal, this, &QKeyMapper_Worker::setWorkerKeyHook, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::setKeyUnHook_Signal, this, &QKeyMapper_Worker::setWorkerKeyUnHook, Qt::QueuedConnection);
#ifndef HOOKSTART_ONSTARTUP
    QObject::connect(QKeyMapper_Hook_Proc::getInstance(), &QKeyMapper_Hook_Proc::setKeyHook_Signal, QKeyMapper_Hook_Proc::getInstance(), &QKeyMapper_Hook_Proc::onSetHookProcKeyHook, Qt::QueuedConnection);
    QObject::connect(QKeyMapper_Hook_Proc::getInstance(), &QKeyMapper_Hook_Proc::setKeyUnHook_Signal, QKeyMapper_Hook_Proc::getInstance(), &QKeyMapper_Hook_Proc::onSetHookProcKeyUnHook, Qt::QueuedConnection);
#endif
    QObject::connect(this, &QKeyMapper_Worker::startBurstTimer_Signal, this, &QKeyMapper_Worker::startBurstTimer, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::stopBurstTimer_Signal, this, &QKeyMapper_Worker::stopBurstTimer, Qt::QueuedConnection);
#if 0
    QObject::connect(this, SIGNAL(sendKeyboardInput_Signal(V_KEYCODE,int)), this, SLOT(sendKeyboardInput(V_KEYCODE,int)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(sendMouseClick_Signal(V_MOUSECODE,int)), this, SLOT(sendMouseClick(V_MOUSECODE,int)), Qt::QueuedConnection);
#endif
    QObject::connect(this, &QKeyMapper_Worker::sendInputKeys_Signal, this, &QKeyMapper_Worker::onSendInputKeys, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::send_WINplusD_Signal, this, &QKeyMapper_Worker::send_WINplusD, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper_Worker::HotKeyTrigger_Signal, this, &QKeyMapper_Worker::HotKeyHookProc, Qt::QueuedConnection);
#if 0
    QObject::connect(this, &QKeyMapper_Worker::sendSpecialVirtualKey_Signal, this, &QKeyMapper_Worker::sendSpecialVirtualKey, Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(onMouseWheel_Signal(int)), this, SLOT(onMouseWheel(int)), Qt::QueuedConnection);
#endif
#ifdef VIGEM_CLIENT_SUPPORT
    QObject::connect(this, &QKeyMapper_Worker::onMouseMove_Signal, this, &QKeyMapper_Worker::onMouseMove, Qt::QueuedConnection);

    m_Mouse2vJoyResetTimer.setTimerType(Qt::PreciseTimer);
    m_Mouse2vJoyResetTimer.setSingleShot(true);
    QObject::connect(&m_Mouse2vJoyResetTimer, &QTimer::timeout, this, &QKeyMapper_Worker::onMouse2vJoyResetTimeout);
#endif

    m_Joy2MouseCycleTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&m_Joy2MouseCycleTimer, &QTimer::timeout, this, &QKeyMapper_Worker::onJoy2MouseCycleTimeout);

    /* Connect QJoysticks Signals */
    QJoysticks *instance = QJoysticks::getInstance();
    QObject::connect(instance, &QJoysticks::countChanged, this, &QKeyMapper_Worker::onJoystickcountChanged, Qt::QueuedConnection);
    QObject::connect(instance, &QJoysticks::POVEvent, this, &QKeyMapper_Worker::onJoystickPOVEvent);
    QObject::connect(instance, &QJoysticks::axisEvent, this, &QKeyMapper_Worker::onJoystickAxisEvent);
    QObject::connect(instance, &QJoysticks::buttonEvent, this, &QKeyMapper_Worker::onJoystickButtonEvent);

    initVirtualKeyCodeMap();
    initVirtualMouseButtonMap();
    initJoystickKeyMap();
    initSkipReleaseModifiersKeysList();

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

#ifdef VIGEM_CLIENT_SUPPORT
    (void)ViGEmClient_Remove();
    (void)ViGEmClient_Disconnect();
    (void)ViGEmClient_Free();
#endif
}

void QKeyMapper_Worker::sendKeyboardInput(V_KEYCODE vkeycode, int keyupdown)
{
//    QMutexLocker locker(&sendinput_mutex);

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
    keyboard_input.ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
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
//    QMutexLocker locker(&sendinput_mutex);

    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = 0;
    mouse_input.mi.dy = 0;
    mouse_input.mi.mouseData = 0;
    mouse_input.mi.time = 0;
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
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
//    QMutexLocker locker(&sendinput_mutex);

    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = delta_x;
    mouse_input.mi.dy = delta_y;
    mouse_input.mi.mouseData = 0;
    mouse_input.mi.time = 0;
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_MOVE;
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
//    QMutexLocker locker(&sendinput_mutex);

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
//    QMutexLocker locker(&sendinput_mutex);

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
    double fScreenWidth    = ::GetSystemMetrics( SM_CXSCREEN )-1;
    double fScreenHeight  = ::GetSystemMetrics( SM_CYSCREEN )-1;
    double fx = point.x * ( 65535.0f / fScreenWidth );
    double fy = point.y * ( 65535.0f / fScreenHeight );

    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = fx;
    mouse_input.mi.dy = fy;
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
//    QMutexLocker locker(&sendinput_mutex);

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
void QKeyMapper_Worker::onMouseMove(int x, int y)
{
// #ifdef MOUSE_VERBOSE_LOG
//    qDebug() << "[onMouseMove]" << "Mouse Move -> Delta X =" << s_Mouse2vJoy_delta.x() << ", Delta Y = " << s_Mouse2vJoy_delta.y();
// #endif

    Q_UNUSED(x);
    Q_UNUSED(y);

    ViGEmClient_Mouse2JoystickUpdate(s_Mouse2vJoy_delta.x(), s_Mouse2vJoy_delta.y());
}

void QKeyMapper_Worker::onMouse2vJoyResetTimeout()
{
    ViGEmClient_JoysticksReset();

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onMouse2vJoyResetTimeout]" << "Reset the Joysticks to Release Center State.";
#endif
}

void QKeyMapper_Worker::onJoy2MouseCycleTimeout()
{
    joystick2MouseMoveProc(s_JoyAxisState);
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

void QKeyMapper_Worker::onSendInputKeys(QStringList inputKeys, int keyupdown, QString original_key, int sendmode)
{
// #ifdef DEBUG_LOGOUT_ON
//     qDebug("[onSendInputKeys] currentThread -> Name:%s, ID:0x%08X, Key[%s], UpDown:%d", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId(), original_key.toLatin1().constData(), keyupdown);
// #endif
#ifdef DEBUG_LOGOUT_ON
    QString threadIdStr = QString("0x%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 8, 16, QChar('0')).toUpper();
    qDebug() << "[onSendInputKeys] currentThread -> Name:" << QThread::currentThread()->objectName() << ", ID:" << threadIdStr << ", Key[" << original_key << "], UpDown:" << keyupdown;
#endif

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onSendInputKeys] m_sendInputStopFlag set True Start";
#endif
    m_sendInputStopMutex.lock();
    if (keyupdown == KEY_DOWN) {
        m_sendInputStopFlag = true;
        m_sendInputStopCondition.wakeAll();
    }
    m_sendInputStopMutex.unlock();
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onSendInputKeys] m_sendInputStopFlag set True Finished";
#endif
    bool waitfordone = QThreadPool::globalInstance()->waitForDone();
    Q_UNUSED(waitfordone);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onSendInputKeys] QThreadPool wait for task Done ->" << waitfordone;
#endif

//    m_sendInputTask = QRunnable::create([this, inputKeys, keyupdown, original_key, sendmode]() {
//        sendInputKeys(inputKeys, keyupdown, original_key, sendmode);
//    });
    m_sendInputTask = new SendInputTask(inputKeys, keyupdown, original_key, sendmode);
    QThreadPool::globalInstance()->start(m_sendInputTask);
}

void QKeyMapper_Worker::sendInputKeys(QStringList inputKeys, int keyupdown, QString original_key, int sendmode)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("[sendInputKeys] currentThread -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

//    QMutexLocker locker(&sendinput_mutex);

    Q_UNUSED(sendmode);
    int waitTime = 0;
    m_sendInputStopFlag = false;

    QString keySequenceStr = ":" + QString(KEYSEQUENCE_STR);

    if (original_key == MOUSE_STR_WHEEL_UP || original_key == MOUSE_STR_WHEEL_DOWN) {
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
#ifdef VIGEM_CLIENT_SUPPORT
    else if (original_key.contains(keySequenceStr)) {
        QString keyseq = inputKeys.constFirst();
        if (keyupdown == KEY_UP && keyseq.contains("vJoy-")) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[sendInputKeys] vJoy Key Up wait start ->" << keyseq;
#endif
            QThread::msleep(VJOY_KEYUP_WAITTIME);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[sendInputKeys] vJoy Key Up wait end ->" << keyseq;
#endif
        }
    }
#endif

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

    int keycount = 0;
    INPUT inputs[SEND_INPUTS_MAX] = { 0 };

    if (KEY_UP == keyupdown) {
        if (key_sequence_count > 1) {
            return;
        }

        QStringList mappingKeys = inputKeys.constFirst().split(SEPARATOR_PLUS);
        keycount = mappingKeys.size();

        if (keycount > MAPPING_KEYS_MAX) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("sendInputKeys(): Too many keys(%d) to mapping!!!", keycount);
#endif
            return;
        }

        pressedMappingKeysMap.remove(original_key);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys KeyUp -> original_key[" << original_key << "], " << "mappingKeys[" << mappingKeys << "]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
#endif

        for(auto it = mappingKeys.crbegin(); it != mappingKeys.crend(); ++it) {
            QString key = (*it);

#if 0
            waitTime = 0;
#endif
            if (key.contains(SEPARATOR_WAITTIME)) {
                QStringList waitTimeKeyList = key.split(SEPARATOR_WAITTIME);
                key = waitTimeKeyList.first();

#if 0
                waitTime = waitTimeKeyList.last().toInt();
                if (it + 1 == mappingKeys.crend()) {
                    waitTime = 0;
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[sendInputKeys] Last key of KEY_UP, do no need wait time";
#endif
                }
#endif
            }

            if (key.isEmpty()) {
                continue;
            }

            if (key == MOUSE_STR_WHEEL_UP || key == MOUSE_STR_WHEEL_DOWN) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendInputKeys] Do no need to send [" << key << "]" << " at KEY_UP";
#endif
                continue;
            }

            if (isPressedMappingKeysContains(key)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys still remain Key[ " << key << " ]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
                qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys skip KeyUp[ " << key << " ]" << " -> original_key[ " << original_key << " ], " << "mappingKeys[ " << mappingKeys << " ]";
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

            if (true == VirtualMouseButtonMap.contains(key)) {
                if (false == pressedVirtualKeysList.contains(key)) {
                    qWarning("sendInputKeys(): Mouse Button Up -> \"%s\" do not exist!!!", key.toStdString().c_str());
                    continue;
                }

                INPUT input = { 0 };
                V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);
                input.type = INPUT_MOUSE;
                input.mi.mouseData = vmousecode.MouseXButton;
                input.mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
                if (KEY_DOWN == keyupdown) {
                    input.mi.dwFlags = vmousecode.MouseDownCode;
                }
                else {
                    input.mi.dwFlags = vmousecode.MouseUpCode;
                }
                SendInput(1, &input, sizeof(INPUT));
            }
#ifdef VIGEM_CLIENT_SUPPORT
            else if (true == QKeyMapper_Worker::JoyStickKeyMap.contains(key)) {
                ViGEmClient_ReleaseButton(key);
            }
#endif
            else if (true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(key)) {
                if (original_key == KEYBOARD_MODIFIERS) {
                    s_forceSendVirtualKey = true;
                }
                else {
                    if (false == pressedVirtualKeysList.contains(key)) {
                        qWarning("sendInputKeys(): Key Up -> \"%s\" do not exist!!!", key.toStdString().c_str());
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
                input.ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
                input.ki.wVk = vkeycode.KeyCode;
                input.ki.wScan = MapVirtualKey(input.ki.wVk, MAPVK_VK_TO_VSC);
//#ifdef DEBUG_LOGOUT_ON
//                qDebug("sendInputKeys(): Key Up -> \"%s\", wScan->0x%08X", key.toStdString().c_str(), input.ki.wScan);
//#endif
                if (KEY_DOWN == keyupdown) {
                    input.ki.dwFlags = extenedkeyflag | 0;
                }
                else {
                    input.ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                }
                SendInput(1, &input, sizeof(INPUT));

                s_forceSendVirtualKey = false;
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("sendInputKeys(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
            }

#if 0
            m_sendInputStopMutex.lock();
            if (m_sendInputStopFlag) {
                waitTime = 0;
            }
            if (MAPPING_WAITTIME_MIN < waitTime && waitTime <= MAPPING_WAITTIME_MAX) {
                QDeadlineTimer deadline(waitTime, Qt::PreciseTimer);
                m_sendInputStopCondition.wait(&m_sendInputStopMutex, deadline);
            }
            m_sendInputStopMutex.unlock();
#endif
        }
    }
    else {
        if (1 == key_sequence_count) {
            QStringList mappingKeys = inputKeys.constFirst().split(SEPARATOR_PLUS);
            keycount = mappingKeys.size();

            if (keycount > MAPPING_KEYS_MAX) {
#ifdef DEBUG_LOGOUT_ON
                qWarning("sendInputKeys(): Too many keys(%d) to mapping!!!", keycount);
#endif
                return;
            }

            if (pressedMappingKeysMap.contains(original_key)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[sendInputKeys] Mapping KeyDown Skiped! pressedMappingKeys already exist! -> original_key[" << original_key << "], " << "mappingKeys[" << mappingKeys << "]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
#endif
                return;
            }

            pressedMappingKeysMap.insert(original_key, mappingKeys);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys KeyDown -> original_key[" << original_key << "], " << "mappingKeys[" << mappingKeys << "]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
#endif

            for (const QString &keyStr : qAsConst(mappingKeys)){
                QString key = keyStr;
                waitTime = 0;
                if (key.contains(SEPARATOR_WAITTIME)) {
                    QStringList waitTimeKeyList = key.split(SEPARATOR_WAITTIME);
                    waitTime = waitTimeKeyList.last().toInt();
                    key = waitTimeKeyList.first();
                }

                if (key.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[sendInputKeys] KeySequence KeyDown only wait time ->" << waitTime;
#endif
                }
                else if (key == MOUSE_STR_WHEEL_UP || key == MOUSE_STR_WHEEL_DOWN) {
                    INPUT input = { 0 };
                    input.type = INPUT_MOUSE;
                    input.mi.dwExtraInfo = VIRTUAL_MOUSE_WHEEL;
                    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
                    if (key == MOUSE_STR_WHEEL_UP) {
                        input.mi.mouseData = WHEEL_DELTA;
                    }
                    else {
                        input.mi.mouseData = -WHEEL_DELTA;
                    }
                    SendInput(1, &input, sizeof(INPUT));
                }
                else if (true == VirtualMouseButtonMap.contains(key)) {
                    if (true == pressedVirtualKeysList.contains(key)) {
                        qWarning("sendInputKeys(): Mouse Button Down -> \"%s\" already exist!!!", key.toStdString().c_str());
                        continue;
                    }

                    INPUT input = { 0 };
                    V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);
                    input.type = INPUT_MOUSE;
                    input.mi.mouseData = vmousecode.MouseXButton;
                    input.mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
                    if (KEY_DOWN == keyupdown) {
                        input.mi.dwFlags = vmousecode.MouseDownCode;
                    }
                    else {
                        input.mi.dwFlags = vmousecode.MouseUpCode;
                    }
                    SendInput(1, &input, sizeof(INPUT));
                }
#ifdef VIGEM_CLIENT_SUPPORT
                else if (true == QKeyMapper_Worker::JoyStickKeyMap.contains(key)) {
                    ViGEmClient_PressButton(key, false);
                }
#endif
                else if (true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(key)) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[sendInputKeys] VirtualKey send Key Start ->" << key << ", m_sendInputStopFlag=" << m_sendInputStopFlag << ", line:" << __LINE__;
#endif
                    if (true == pressedVirtualKeysList.contains(key)) {
                        qWarning("sendInputKeys(): Key Down -> \"%s\" already exist!!!", key.toStdString().c_str());
                        continue;
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
                    input.ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
                    input.ki.wVk = vkeycode.KeyCode;
                    input.ki.wScan = MapVirtualKey(input.ki.wVk, MAPVK_VK_TO_VSC);
//#ifdef DEBUG_LOGOUT_ON
//                    qDebug("sendInputKeys(): Key Down -> \"%s\", wScan->0x%08X", key.toStdString().c_str(), input.ki.wScan);
//#endif
                    if (KEY_DOWN == keyupdown) {
                        input.ki.dwFlags = extenedkeyflag | 0;
                    }
                    else {
                        input.ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                    }
                    SendInput(1, &input, sizeof(INPUT));
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[sendInputKeys] VirtualKey send Key End ->" << key << ", m_sendInputStopFlag=" << m_sendInputStopFlag << ", line:" << __LINE__;
#endif
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qWarning("sendInputKeys(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
                }

                m_sendInputStopMutex.lock();
                if (m_sendInputStopFlag) {
                    waitTime = 0;
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[sendInputKeys] m_sendInputStopFlag ->" << m_sendInputStopFlag << ", line:" << __LINE__;
#endif
                }
                if (MAPPING_WAITTIME_MIN < waitTime && waitTime <= MAPPING_WAITTIME_MAX) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[sendInputKeys] StopConditionWait Start -> Time=" << waitTime << ", m_sendInputStopFlag=" << m_sendInputStopFlag << ", line:" << __LINE__;
#endif
                    QDeadlineTimer deadline(waitTime, Qt::PreciseTimer);
                    m_sendInputStopCondition.wait(&m_sendInputStopMutex, deadline);
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[sendInputKeys] StopConditionWait Finished -> Time=" << waitTime << ", m_sendInputStopFlag=" << m_sendInputStopFlag << ", line:" << __LINE__;
#endif
                }
                m_sendInputStopMutex.unlock();
            }
        }
        /* key_sequence_count > 1 */
        else {
            sendKeySequenceList(inputKeys, original_key);
        }
    }
}

void QKeyMapper_Worker::send_WINplusD()
{
//    QMutexLocker locker(&sendinput_mutex);
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

void QKeyMapper_Worker::sendBurstKeyDown(const QString &burstKey)
{
    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(burstKey);

    if (findindex >=0){
        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
        QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
        sendInputKeys(mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
    }
}

void QKeyMapper_Worker::sendBurstKeyUp(const QString &burstKey, bool stop)
{
    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(burstKey);

    if (findindex >=0){
        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
        QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
        int sendmode = SENDMODE_NORMAL;
        if (true == stop) {
            sendmode = SENDMODE_FORCE_STOP;
        }
        sendInputKeys(mappingKeyList, KEY_UP, original_key, sendmode);
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

#if 0
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
#endif

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
                    s_ViGEmTarget = Q_NULLPTR;
#ifdef DEBUG_LOGOUT_ON
                    qWarning("[ViGEmClient_Remove] ViGEmTarget Remove failed!!!, Error=0x%08X -> [0x%08X]", error, s_ViGEmTarget);
#endif
                }
            }
            else {
                s_ViGEmTarget = Q_NULLPTR;
#ifdef DEBUG_LOGOUT_ON
                qWarning("[ViGEmClient_Remove] Reset VirtualGamepad Report State Failed!!!, Error=0x%08X -> ViGEmTarget[0x%08X]", error, s_ViGEmTarget);
#endif
            }
        }
        else {
            s_ViGEmTarget = Q_NULLPTR;
#ifdef DEBUG_LOGOUT_ON
            qWarning() << "[ViGEmClient_Remove]" << "ViGEmClient ConnectState or ViGEmTarget AttachState error!!! ->" << "ConnectState =" << s_ViGEmClient_ConnectState << ", Attached =" << vigem_target_is_attached(s_ViGEmTarget);
#endif
        }
    }
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

void QKeyMapper_Worker::ViGEmClient_PressButton(const QString &joystickButton, int autoAdjust)
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient != Q_NULLPTR && s_ViGEmTarget != Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        if (autoAdjust) {
#ifdef GRIP_VERBOSE_LOG
            qDebug().noquote().nospace() << "[ViGEmClient]" << " VirtualGamepad LT&RT Auto Adjust Update.";
#endif
        }
        else {
            qDebug().noquote().nospace() << "[ViGEmClient]" << " VirtualGamepad Button Press [" << joystickButton << "]";
        }
#endif

        if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[ViGEmClient_PressButton] ViGEmClient is not Connected(%d)!!! -> [0x%08X]", s_ViGEmClient_ConnectState, s_ViGEmClient);
#endif
            return;
        }

        if (vigem_target_is_attached(s_ViGEmTarget) != TRUE) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[ViGEmClient_PressButton] ViGEmTarget is not Attached!!! -> [0x%08X]", s_ViGEmTarget);
#endif
            return;
        }

        int updateFlag = VJOY_UPDATE_NONE;
        if (joystickButton.isEmpty() && autoAdjust) {
            if (autoAdjust & AUTO_ADJUST_BRAKE) {
                if (pressedvJoyButtons.contains(VJOY_LT_BRAKE_STR)) {
                    s_ViGEmTarget_Report.bLeftTrigger = s_Auto_Brake;
                    updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
                }
                if (pressedvJoyButtons.contains(VJOY_RT_BRAKE_STR)) {
                    s_ViGEmTarget_Report.bRightTrigger = s_Auto_Brake;
                    updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
                }
            }

            if (autoAdjust & AUTO_ADJUST_ACCEL) {
                if (pressedvJoyButtons.contains(VJOY_LT_ACCEL_STR)) {
                    s_ViGEmTarget_Report.bLeftTrigger = s_Auto_Accel;
                    updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
                }
                if (pressedvJoyButtons.contains(VJOY_RT_ACCEL_STR)) {
                    s_ViGEmTarget_Report.bRightTrigger = s_Auto_Accel;
                    updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
                }
            }
        }
        else if (ViGEmButtonMap.contains(joystickButton)) {
            XUSB_BUTTON button = ViGEmButtonMap.value(joystickButton);

            s_ViGEmTarget_Report.wButtons = s_ViGEmTarget_Report.wButtons | button;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton == VJOY_LT_BRAKE_STR) {
            s_ViGEmTarget_Report.bLeftTrigger = s_Auto_Brake;
            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
        }
        else if (joystickButton == VJOY_RT_BRAKE_STR) {
            s_ViGEmTarget_Report.bRightTrigger = s_Auto_Brake;
            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
        }
        else if (joystickButton == VJOY_LT_ACCEL_STR) {
            s_ViGEmTarget_Report.bLeftTrigger = s_Auto_Accel;
            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
        }
        else if (joystickButton == VJOY_RT_ACCEL_STR) {
            s_ViGEmTarget_Report.bRightTrigger = s_Auto_Accel;
            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
        }
        else if (joystickButton == "vJoy-Key11(LT)") {
            s_ViGEmTarget_Report.bLeftTrigger = XINPUT_TRIGGER_MAX;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton == "vJoy-Key12(RT)") {
            s_ViGEmTarget_Report.bRightTrigger = XINPUT_TRIGGER_MAX;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton.startsWith("vJoy-LS-")) {
            if (joystickButton == "vJoy-LS-Up") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-LS-Down") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-LS-Left") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-LS-Right") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }

            if (updateFlag) {
                if (false == pressedvJoyLStickKeys.contains(joystickButton)){
                    pressedvJoyLStickKeys.append(joystickButton);
                }
                ViGEmClient_CheckJoysticksReportData();
            }
        }
        else if (joystickButton.startsWith("vJoy-RS-")) {
            if (joystickButton == "vJoy-RS-Up") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-RS-Down") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-RS-Left") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-RS-Right") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }

            if (updateFlag) {
                if (false == pressedvJoyRStickKeys.contains(joystickButton)){
                    pressedvJoyRStickKeys.append(joystickButton);
                }
                ViGEmClient_CheckJoysticksReportData();
            }
        }

        if (updateFlag) {
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

            if (error == VIGEM_ERROR_NONE) {
                if (VJOY_UPDATE_BUTTONS == updateFlag || VJOY_UPDATE_AUTO_BUTTONS == updateFlag) {
                    if (false == pressedvJoyButtons.contains(joystickButton)){
                        pressedvJoyButtons.append(joystickButton);
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[pressedvJoyButtons]" << "Button Press" << ": Current Pressed vJoyButtons -> " << pressedvJoyButtons;
#endif
                    }

                    if (VJOY_UPDATE_AUTO_BUTTONS == updateFlag) {
                        s_last_Auto_Brake = s_Auto_Brake;
                        s_last_Auto_Accel = s_Auto_Accel;
                    }
                }
#ifdef JOYSTICK_VERBOSE_LOG
                qDebug("[ViGEmClient_Button] Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", s_ViGEmTarget_Report.sThumbLX, s_ViGEmTarget_Report.sThumbLY, s_ViGEmTarget_Report.sThumbRX, s_ViGEmTarget_Report.sThumbRY);
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

void QKeyMapper_Worker::ViGEmClient_ReleaseButton(const QString &joystickButton)
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient != Q_NULLPTR && s_ViGEmTarget != Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().noquote().nospace() << "[ViGEmClient]" << " VirtualGamepad Button Release [" << joystickButton << "]";
#endif

        if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[ViGEmClient_ReleaseButton] ViGEmClient is not Connected(%d)!!! -> [0x%08X]", s_ViGEmClient_ConnectState, s_ViGEmClient);
#endif
            return;
        }

        if (vigem_target_is_attached(s_ViGEmTarget) != TRUE) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[ViGEmClient_ReleaseButton] ViGEmTarget is not Attached!!! -> [0x%08X]", s_ViGEmTarget);
#endif
            return;
        }

        int updateFlag = VJOY_UPDATE_NONE;
        if (ViGEmButtonMap.contains(joystickButton)) {
            XUSB_BUTTON button = ViGEmButtonMap.value(joystickButton);

            s_ViGEmTarget_Report.wButtons = s_ViGEmTarget_Report.wButtons & ~button;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton == VJOY_LT_BRAKE_STR) {
            s_ViGEmTarget_Report.bLeftTrigger = XINPUT_TRIGGER_MIN;
            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
        }
        else if (joystickButton == VJOY_RT_BRAKE_STR) {
            s_ViGEmTarget_Report.bRightTrigger = XINPUT_TRIGGER_MIN;
            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
        }
        else if (joystickButton == VJOY_LT_ACCEL_STR) {
            s_ViGEmTarget_Report.bLeftTrigger = XINPUT_TRIGGER_MIN;
            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
        }
        else if (joystickButton == VJOY_RT_ACCEL_STR) {
            s_ViGEmTarget_Report.bRightTrigger = XINPUT_TRIGGER_MIN;
            updateFlag = VJOY_UPDATE_AUTO_BUTTONS;
        }
        else if (joystickButton == "vJoy-Key11(LT)") {
            s_ViGEmTarget_Report.bLeftTrigger = XINPUT_TRIGGER_MIN;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton == "vJoy-Key12(RT)") {
            s_ViGEmTarget_Report.bRightTrigger = XINPUT_TRIGGER_MIN;
            updateFlag = VJOY_UPDATE_BUTTONS;
        }
        else if (joystickButton.startsWith("vJoy-LS-")) {
            if (joystickButton == "vJoy-LS-Up") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-LS-Down") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-LS-Left") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-LS-Right") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }

            if (updateFlag) {
                pressedvJoyLStickKeys.removeAll(joystickButton);
                ViGEmClient_CheckJoysticksReportData();
            }
        }
        else if (joystickButton.startsWith("vJoy-RS-")) {
            if (joystickButton == "vJoy-RS-Up") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-RS-Down") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-RS-Left") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }
            else if (joystickButton == "vJoy-RS-Right") {
                updateFlag = VJOY_UPDATE_JOYSTICKS;
            }

            if (updateFlag) {
                pressedvJoyRStickKeys.removeAll(joystickButton);
                ViGEmClient_CheckJoysticksReportData();
            }
        }

        if (updateFlag) {
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

            if (error == VIGEM_ERROR_NONE) {
                if (VJOY_UPDATE_BUTTONS == updateFlag || VJOY_UPDATE_AUTO_BUTTONS == updateFlag) {
                    pressedvJoyButtons.removeAll(joystickButton);
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[pressedvJoyButtons]" << "Button Release" << ": Current Pressed vJoyButtons -> " << pressedvJoyButtons;
#endif

                    if (VJOY_UPDATE_AUTO_BUTTONS == updateFlag) {
                        s_Auto_Brake = AUTO_BRAKE_DEFAULT;
                        s_Auto_Accel = AUTO_ACCEL_DEFAULT;
                        s_last_Auto_Brake = 0;
                        s_last_Auto_Accel = 0;
                    }
#ifdef DEBUG_LOGOUT_ON
                    qDebug("[ViGEmClient_Button] Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", s_ViGEmTarget_Report.sThumbLX, s_ViGEmTarget_Report.sThumbLY, s_ViGEmTarget_Report.sThumbRX, s_ViGEmTarget_Report.sThumbRY);
#endif
                }
#ifdef JOYSTICK_VERBOSE_LOG
                qDebug("[ViGEmClient_Button] Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", s_ViGEmTarget_Report.sThumbLX, s_ViGEmTarget_Report.sThumbLY, s_ViGEmTarget_Report.sThumbRX, s_ViGEmTarget_Report.sThumbRY);
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

void QKeyMapper_Worker::ViGEmClient_CheckJoysticksReportData()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[ViGEmClient_CheckJoysticksReportData] vJoyLSPressedKeys ->" << pressedvJoyLStickKeys;
    qDebug() << "[ViGEmClient_CheckJoysticksReportData] vJoyRSPressedKeys ->" << pressedvJoyRStickKeys;
#endif

    // Reset thumb values
    s_ViGEmTarget_Report.sThumbLX = XINPUT_THUMB_RELEASE;
    s_ViGEmTarget_Report.sThumbLY = XINPUT_THUMB_RELEASE;
    s_ViGEmTarget_Report.sThumbRX = XINPUT_THUMB_RELEASE;
    s_ViGEmTarget_Report.sThumbRY = XINPUT_THUMB_RELEASE;

    // Update thumb values based on pressed keys
    for (const QString &key : pressedvJoyLStickKeys) {
        if (key == "vJoy-LS-Up") {
            s_ViGEmTarget_Report.sThumbLY = XINPUT_THUMB_MAX;
        } else if (key == "vJoy-LS-Down") {
            s_ViGEmTarget_Report.sThumbLY = XINPUT_THUMB_MIN;
        } else if (key == "vJoy-LS-Left") {
            s_ViGEmTarget_Report.sThumbLX = XINPUT_THUMB_MIN;
        } else if (key == "vJoy-LS-Right") {
            s_ViGEmTarget_Report.sThumbLX = XINPUT_THUMB_MAX;
        }
    }

    for (const QString &key : pressedvJoyRStickKeys) {
        if (key == "vJoy-RS-Up") {
            s_ViGEmTarget_Report.sThumbRY = XINPUT_THUMB_MAX;
        } else if (key == "vJoy-RS-Down") {
            s_ViGEmTarget_Report.sThumbRY = XINPUT_THUMB_MIN;
        } else if (key == "vJoy-RS-Left") {
            s_ViGEmTarget_Report.sThumbRX = XINPUT_THUMB_MIN;
        } else if (key == "vJoy-RS-Right") {
            s_ViGEmTarget_Report.sThumbRX = XINPUT_THUMB_MAX;
        }
    }

    if (s_ViGEmTarget_Report.sThumbLX != XINPUT_THUMB_RELEASE
        && s_ViGEmTarget_Report.sThumbLY != XINPUT_THUMB_RELEASE) {
        ViGEmClient_CalculateThumbValue(&s_ViGEmTarget_Report.sThumbLX, &s_ViGEmTarget_Report.sThumbLY);
    }

    if (s_ViGEmTarget_Report.sThumbRX != XINPUT_THUMB_RELEASE
        && s_ViGEmTarget_Report.sThumbRY != XINPUT_THUMB_RELEASE) {
        ViGEmClient_CalculateThumbValue(&s_ViGEmTarget_Report.sThumbRX, &s_ViGEmTarget_Report.sThumbRY);
    }

}

void QKeyMapper_Worker::ViGEmClient_CalculateThumbValue(SHORT *ori_ThumbX, SHORT *ori_ThumbY)
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

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    SHORT newThumbX = static_cast<SHORT>(qRound(distance * qCos(direction)));
    SHORT newThumbY = static_cast<SHORT>(qRound(distance * qSin(direction)));
#else
    SHORT newThumbX = static_cast<SHORT>(std::round(distance * std::cos(direction)));
    SHORT newThumbY = static_cast<SHORT>(std::round(distance * std::sin(direction)));
#endif

    *ori_ThumbX = newThumbX;
    *ori_ThumbY = newThumbY;

#ifdef JOYSTICK_VERBOSE_LOG
    qDebug("[ViGEmClient_CalculateThumbValue] Calculated ThumbX[%d], ThumbY[%d]", newThumbX, newThumbY);
#endif
}

QKeyMapper_Worker::Mouse2vJoyState QKeyMapper_Worker::ViGEmClient_checkMouse2JoystickEnableState()
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

    Mouse2vJoyState mouse2joy_enablestate = MOUSE2VJOY_NONE;
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

    if (leftJoystickUpdate && rightJoystickUpdate) {
        mouse2joy_enablestate = MOUSE2VJOY_BOTH;
    }
    else if (leftJoystickUpdate) {
        mouse2joy_enablestate = MOUSE2VJOY_LEFT;
    }
    else if (rightJoystickUpdate) {
        mouse2joy_enablestate = MOUSE2VJOY_RIGHT;
    }

    return mouse2joy_enablestate;
}

void QKeyMapper_Worker::ViGEmClient_Mouse2JoystickUpdate(int delta_x, int delta_y)
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

    bool leftJoystickUpdate = false;
    bool rightJoystickUpdate = false;

    if (MOUSE2VJOY_LEFT == s_Mouse2vJoy_EnableState) {
        leftJoystickUpdate = true;
    }
    else if (MOUSE2VJOY_RIGHT == s_Mouse2vJoy_EnableState) {
        rightJoystickUpdate = true;
    }
    else if (MOUSE2VJOY_BOTH == s_Mouse2vJoy_EnableState) {
        leftJoystickUpdate = true;
        rightJoystickUpdate = true;
    }

    // int findMouse2LSindex = QKeyMapper::findOriKeyInKeyMappingDataList(VJOY_MOUSE2LS_STR);
    // if (findMouse2LSindex >=0){
    //     leftJoystickUpdate = true;
    // }

    // int findMouse2RSindex = QKeyMapper::findOriKeyInKeyMappingDataList(VJOY_MOUSE2RS_STR);
    // if (findMouse2RSindex >=0){
    //     rightJoystickUpdate = true;
    // }

    if (leftJoystickUpdate || rightJoystickUpdate) {
        int vJoy_X_Sensitivity = QKeyMapper::getvJoyXSensitivity();
        int vJoy_Y_Sensitivity = QKeyMapper::getvJoyYSensitivity();

        // Mouse2Joystick core algorithm from "https://github.com/memethyl/Mouse2Joystick" >>>
        #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        qreal x = -qExp((-1.0 / vJoy_X_Sensitivity) * qAbs(delta_x)) + 1.0;
        qreal y = -qExp((-1.0 / vJoy_Y_Sensitivity) * qAbs(delta_y)) + 1.0;
        #else
        qreal x = -std::exp((-1.0 / vJoy_X_Sensitivity) * std::abs(delta_x)) + 1.0;
        qreal y = -std::exp((-1.0 / vJoy_Y_Sensitivity) * std::abs(delta_y)) + 1.0;
        #endif
        // take the sign into account, expanding the range to (-1, 1)
        x *= sign(delta_x);
        y *= -sign(delta_y);
        // XInput joystick coordinates are signed shorts, so convert to (-32767, 32767)
        short leftX = (short)(32767.0 * x);
        short leftY = (short)(32767.0 * y);
        ViGEmClient_CalculateThumbValue(&leftX, &leftY);
        short rightX = leftX;
        short rightY = leftY;
        // Mouse2Joystick core algorithm from "https://github.com/memethyl/Mouse2Joystick" <<<

        if (leftJoystickUpdate) {
            s_ViGEmTarget_Report.sThumbLX = leftX;
            s_ViGEmTarget_Report.sThumbLY = leftY;
        }
        if (rightJoystickUpdate) {
            s_ViGEmTarget_Report.sThumbRX = rightX;
            s_ViGEmTarget_Report.sThumbRY = rightY;
        }

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
        m_Mouse2vJoyResetTimer.start(MOUSE2VJOY_RESET_TIMEOUT);
#ifdef DEBUG_LOGOUT_ON
        if (error != VIGEM_ERROR_NONE) {
            qDebug("[ViGEmClient_Mouse2JoystickUpdate] Mouse2Joystick Update ErrorCode: 0x%08X", error);
        }
        else {
#ifdef JOYSTICK_VERBOSE_LOG
            qDebug("[ViGEmClient_Mouse2JoystickUpdate] Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", s_ViGEmTarget_Report.sThumbLX, s_ViGEmTarget_Report.sThumbLY, s_ViGEmTarget_Report.sThumbRX, s_ViGEmTarget_Report.sThumbRY);
#endif
        }
#endif
    }
}

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

void QKeyMapper_Worker::ViGEmClient_JoysticksReset()
{
    if (MOUSE2VJOY_NONE == s_Mouse2vJoy_EnableState) {
        return;
    }

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

    if (MOUSE2VJOY_LEFT == s_Mouse2vJoy_EnableState) {
        s_ViGEmTarget_Report.sThumbLX = 0;
        s_ViGEmTarget_Report.sThumbLY = 0;
    }
    else if (MOUSE2VJOY_RIGHT == s_Mouse2vJoy_EnableState) {
        s_ViGEmTarget_Report.sThumbRX = 0;
        s_ViGEmTarget_Report.sThumbRY = 0;
    }
    else {
        s_ViGEmTarget_Report.sThumbLX = 0;
        s_ViGEmTarget_Report.sThumbLY = 0;
        s_ViGEmTarget_Report.sThumbRX = 0;
        s_ViGEmTarget_Report.sThumbRY = 0;
    }

    ViGEmClient_CheckJoysticksReportData();

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
        qDebug("[ViGEmClient_GamepadReset] GamepadReset Update ErrorCode: 0x%08X", error);
    }
    else {
#ifdef JOYSTICK_VERBOSE_LOG
        qDebug("[ViGEmClient_JoysticksReset] Current ThumbLX[%d], ThumbLY[%d], ThumbRX[%d], ThumbRY[%d]", s_ViGEmTarget_Report.sThumbLX, s_ViGEmTarget_Report.sThumbLY, s_ViGEmTarget_Report.sThumbRX, s_ViGEmTarget_Report.sThumbRY);
#endif
    }
#endif
}
#endif

void QKeyMapper_Worker::timerEvent(QTimerEvent *event)
{
    int timerID = event->timerId();
    auto it_burst_timer_keyup = std::find(m_BurstKeyUpTimerMap.cbegin(), m_BurstKeyUpTimerMap.cend(), timerID);
    auto it_burst_timer = std::find(m_BurstTimerMap.cbegin(), m_BurstTimerMap.cend(), timerID);
    if (it_burst_timer_keyup != m_BurstKeyUpTimerMap.cend()) {
        QString burstKey = m_BurstKeyUpTimerMap.key(timerID);
        if (false == burstKey.isEmpty()) {
            killTimer(timerID);
            m_BurstKeyUpTimerMap.remove(burstKey);
            sendBurstKeyUp(burstKey, false);
//#ifdef DEBUG_LOGOUT_ON
//            qDebug("timerEvent(): Key \"%s\" BurstPress timeout.", burstKey.toStdString().c_str());
//#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("timerEvent(): Could not find TimerID(%d) in m_BurstKeyUpTimerMap!!!", timerID);
#endif
        }
    }
    else if (it_burst_timer != m_BurstTimerMap.cend()) {
        QString burstKey = m_BurstTimerMap.key(timerID);
        if (false == burstKey.isEmpty()) {
//#ifdef DEBUG_LOGOUT_ON
//            qDebug("timerEvent(): Key \"%s\" BurstTimer timeout.", burstKey.toStdString().c_str());
//#endif
            sendBurstKeyDown(burstKey);

            if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
                int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
                qDebug("timerEvent(): Key \"%s\" BurstKeyUpTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
                killTimer(existTimerID);
                m_BurstKeyUpTimerMap.remove(burstKey);
            }
            int burstpressTime = QKeyMapper::getBurstPressTime();
            int keyupTimerID = startTimer(burstpressTime, Qt::PreciseTimer);
            m_BurstKeyUpTimerMap.insert(burstKey, keyupTimerID);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("timerEvent(): Could not find TimerID(%d) in BurstTimerMap!!!", timerID);
#endif
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning("timerEvent(): Could not find TimerID(%d) in both m_BurstKeyUpTimerMap & BurstTimerMap!!!", timerID);
#endif
    }
}

void QKeyMapper_Worker::threadStarted()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("threadStarted() -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

    /* UDP Data Port Listener */
    m_UdpSocket = new QUdpSocket(this);
    QObject::connect(m_UdpSocket, &QUdpSocket::readyRead, this, &QKeyMapper_Worker::processUdpPendingDatagrams);
}

void QKeyMapper_Worker::setWorkerKeyHook(HWND hWnd)
{
    Q_UNUSED(hWnd);
    clearAllBurstTimersAndLockKeys();
    pressedRealKeysList.clear();
    pressedVirtualKeysList.clear();
    pressedShortcutKeysList.clear();
    pressedMappingKeysMap.clear();
    m_BurstTimerMap.clear();
    m_BurstKeyUpTimerMap.clear();
    pressedLockKeysList.clear();
    collectExchangeKeysList();

#ifdef VIGEM_CLIENT_SUPPORT
    s_Auto_Brake = AUTO_BRAKE_DEFAULT;
    s_Auto_Accel = AUTO_ACCEL_DEFAULT;
    s_last_Auto_Brake = 0;
    s_last_Auto_Accel = 0;
    s_GripDetect_EnableState = checkGripDetectEnableState();
    s_Mouse2vJoy_delta.rx() = 0;
    s_Mouse2vJoy_delta.ry() = 0;
    s_Mouse2vJoy_prev.rx() = 0;
    s_Mouse2vJoy_prev.ry() = 0;
    pressedvJoyLStickKeys.clear();
    pressedvJoyRStickKeys.clear();
    pressedvJoyButtons.clear();
    ViGEmClient_GamepadReset();
    s_Mouse2vJoy_EnableState = ViGEmClient_checkMouse2JoystickEnableState();
#endif

    s_Joy2Mouse_EnableState = checkJoystick2MouseEnableState();

#ifdef VIGEM_CLIENT_SUPPORT
    if (s_Mouse2vJoy_EnableState != MOUSE2VJOY_NONE && QKeyMapper::getLockCursorStatus()) {
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

#ifdef HOOKSTART_ONSTARTUP
    s_AtomicHookProcStart = true;
#else
    emit QKeyMapper_Hook_Proc::getInstance()->setKeyHook_Signal(hWnd);
#endif
    // m_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper_Worker::LowLevelKeyboardHookProc, GetModuleHandle(Q_NULLPTR), 0);
    // m_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, QKeyMapper_Worker::LowLevelMouseHookProc, GetModuleHandle(Q_NULLPTR), 0);
    setWorkerJoystickCaptureStart();
    startDataPortListener();
//    setWorkerDInputKeyHook(hWnd);
}

void QKeyMapper_Worker::setWorkerKeyUnHook()
{
    clearAllBurstTimersAndLockKeys();
    pressedRealKeysList.clear();
    pressedVirtualKeysList.clear();
    pressedShortcutKeysList.clear();
    pressedMappingKeysMap.clear();
    m_BurstTimerMap.clear();
    m_BurstKeyUpTimerMap.clear();
    pressedLockKeysList.clear();
    exchangeKeysList.clear();

#ifdef HOOKSTART_ONSTARTUP
    s_AtomicHookProcStart = false;
#else
    emit QKeyMapper_Hook_Proc::getInstance()->setKeyUnHook_Signal();
#endif

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

    s_Joy2Mouse_EnableState = JOY2MOUSE_NONE;
    setWorkerJoystickCaptureStop();
    stopDataPortListener();
    //    setWorkerDInputKeyUnHook();

#ifdef VIGEM_CLIENT_SUPPORT
    if (s_Mouse2vJoy_EnableState != MOUSE2VJOY_NONE && isCursorAtBottomRight() && m_LastMouseCursorPoint.x >= 0) {
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
    s_Mouse2vJoy_delta.rx() = 0;
    s_Mouse2vJoy_delta.ry() = 0;
    s_Mouse2vJoy_prev.rx() = 0;
    s_Mouse2vJoy_prev.ry() = 0;
    pressedvJoyLStickKeys.clear();
    pressedvJoyRStickKeys.clear();
    pressedvJoyButtons.clear();
    ViGEmClient_GamepadReset();
    s_Mouse2vJoy_EnableState = MOUSE2VJOY_NONE;
    m_LastMouseCursorPoint.x = -1;
    m_LastMouseCursorPoint.y = -1;
#endif
}

void QKeyMapper_Worker::setWorkerJoystickCaptureStart(void)
{
    m_JoystickCapture = true;

    s_JoyAxisState = Joystick_AxisState();
    if (s_Joy2Mouse_EnableState != JOY2MOUSE_NONE) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setWorkerJoystickCaptureStart] Joy2MouseCycleTimer Started.");
#endif
        m_Joy2MouseCycleTimer.start(JOY2MOUSE_CYCLECHECK_TIMEOUT);
    }
}

void QKeyMapper_Worker::setWorkerJoystickCaptureStop()
{
    m_JoystickCapture = false;

#ifdef DEBUG_LOGOUT_ON
    qDebug("[setWorkerJoystickCaptureStop] Joy2MouseCycleTimer Stopped.");
#endif

    m_Joy2MouseCycleTimer.stop();
    s_JoyAxisState = Joystick_AxisState();
}

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
    QString keycodeStringForSearch = QString(PREFIX_SHORTCUT) + keycodeString;
    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeStringForSearch);
    returnFlag = hookBurstAndLockProc(keycodeStringForSearch, keyupdown);

    if (false == returnFlag) {
        if (findindex >=0){
            QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
            QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
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
                emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
            }
            else { /* KEY_UP == keyupdown */
                emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
            }
        }
    }
}

void QKeyMapper_Worker::releaseKeyboardModifiers(const Qt::KeyboardModifiers &modifiers)
{
    QStringList pressedKeyboardModifiersList;
    if ((GetKeyState(VK_LSHIFT) & 0x8000) != 0 && modifiers.testFlag(Qt::ShiftModifier)) {
        pressedKeyboardModifiersList.append("L-Shift");
    }
    if ((GetKeyState(VK_RSHIFT) & 0x8000) != 0 && modifiers.testFlag(Qt::ShiftModifier)) {
        pressedKeyboardModifiersList.append("R-Shift");
    }
    if ((GetKeyState(VK_LCONTROL) & 0x8000) != 0 && modifiers.testFlag(Qt::ControlModifier)) {
        pressedKeyboardModifiersList.append("L-Ctrl");
    }
    if ((GetKeyState(VK_RCONTROL) & 0x8000) != 0 && modifiers.testFlag(Qt::ControlModifier)) {
        pressedKeyboardModifiersList.append("R-Ctrl");
    }
    if ((GetKeyState(VK_LMENU) & 0x8000) != 0 && modifiers.testFlag(Qt::AltModifier)) {
        pressedKeyboardModifiersList.append("L-Alt");
    }
    if ((GetKeyState(VK_RMENU) & 0x8000) != 0 && modifiers.testFlag(Qt::AltModifier)) {
        pressedKeyboardModifiersList.append("R-Alt");
    }
    if ((GetKeyState(VK_LWIN) & 0x8000) != 0 && modifiers.testFlag(Qt::MetaModifier)) {
        pressedKeyboardModifiersList.append("L-Win");
    }
    if ((GetKeyState(VK_RWIN) & 0x8000) != 0 && modifiers.testFlag(Qt::MetaModifier)) {
        pressedKeyboardModifiersList.append("R-Win");
    }

    for (const QString &modifierstr : qAsConst(pressedKeyboardModifiersList)) {
        QStringList mappingKeyList = QStringList() << modifierstr;
        QString original_key = QString(KEYBOARD_MODIFIERS);
        emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
    }

    if (modifiers == Qt::AltModifier) {
        QStringList mappingKeyList;
        if ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) {
            mappingKeyList = QStringList() << "L-Alt";
        }
        else if ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0) {
            mappingKeyList = QStringList() << "R-Alt";
        }
        else {
            mappingKeyList = QStringList() << "L-Alt";
        }
        QString original_key = QString(KEYBOARD_MODIFIERS);
        emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
        emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
    }
}

QKeyMapper_Worker::GripDetectState QKeyMapper_Worker::checkGripDetectEnableState()
{
    GripDetectState gripdetect_enablestate = GRIPDETECT_NONE;
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

    if (gripdetect_brake && gripdetect_accel) {
        gripdetect_enablestate = GRIPDETECT_BOTH;
    }
    else if (gripdetect_brake) {
        gripdetect_enablestate = GRIPDETECT_BRAKE;
    }
    else if (gripdetect_accel) {
        gripdetect_enablestate = GRIPDETECT_ACCEL;
    }

    return gripdetect_enablestate;
}

void QKeyMapper_Worker::processUdpPendingDatagrams()
{
    while (m_UdpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_UdpSocket->receiveDatagram();
        processForzaHorizon4FormatData(datagram.data());
    }
}

void QKeyMapper_Worker::processForzaHorizon4FormatData(const QByteArray &fh4data)
{
    QDataStream stream(fh4data);
    stream.setByteOrder(QDataStream::LittleEndian);

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

    stream >> is_race_on;
    stream >> timestamp_ms;
    stream >> engine_max_rpm;
    stream >> engine_idle_rpm;
    stream >> current_engine_rpm;
    stream >> acceleration_x;
    stream >> acceleration_y;
    stream >> acceleration_z;
    stream >> velocity_x;
    stream >> velocity_y;
    stream >> velocity_z;
    stream >> angular_velocity_x;
    stream >> angular_velocity_y;
    stream >> angular_velocity_z;
    stream >> yaw;
    stream >> pitch;
    stream >> roll;
    stream >> norm_suspension_travel_FL;
    stream >> norm_suspension_travel_FR;
    stream >> norm_suspension_travel_RL;
    stream >> norm_suspension_travel_RR;
    stream >> tire_slip_ratio_FL;
    stream >> tire_slip_ratio_FR;
    stream >> tire_slip_ratio_RL;
    stream >> tire_slip_ratio_RR;
    stream >> wheel_rotation_speed_FL;
    stream >> wheel_rotation_speed_FR;
    stream >> wheel_rotation_speed_RL;
    stream >> wheel_rotation_speed_RR;
    stream >> wheel_on_rumble_strip_FL;
    stream >> wheel_on_rumble_strip_FR;
    stream >> wheel_on_rumble_strip_RL;
    stream >> wheel_on_rumble_strip_RR;
    stream >> wheel_in_puddle_FL;
    stream >> wheel_in_puddle_FR;
    stream >> wheel_in_puddle_RL;
    stream >> wheel_in_puddle_RR;
    stream >> surface_rumble_FL;
    stream >> surface_rumble_FR;
    stream >> surface_rumble_RL;
    stream >> surface_rumble_RR;
    stream >> tire_slip_angle_FL;
    stream >> tire_slip_angle_FR;
    stream >> tire_slip_angle_RL;
    stream >> tire_slip_angle_RR;
    stream >> tire_combined_slip_FL;
    stream >> tire_combined_slip_FR;
    stream >> tire_combined_slip_RL;
    stream >> tire_combined_slip_RR;
    stream >> suspension_travel_meters_FL;
    stream >> suspension_travel_meters_FR;
    stream >> suspension_travel_meters_RL;
    stream >> suspension_travel_meters_RR;
    stream >> car_ordinal;
    stream >> car_class;
    stream >> car_performance_index;
    stream >> drivetrain_type;
    stream >> num_cylinders;

    double average_slip_ratio = (qAbs(tire_slip_ratio_FL) + qAbs(tire_slip_ratio_FR) + qAbs(tire_slip_ratio_RL) + qAbs(tire_slip_ratio_RR)) / 4;
    double max_slip_ratio = qMax(qMax(qAbs(tire_slip_ratio_FL), qAbs(tire_slip_ratio_FR)), qMax(qAbs(tire_slip_ratio_RL), qAbs(tire_slip_ratio_RR)));

#ifdef GRIP_VERBOSE_LOG
    qDebug() << "[processForzaHorizon4FormatData]" << "tire_slip_ratio_FL =" << tire_slip_ratio_FL << ", tire_slip_ratio_FR =" << tire_slip_ratio_FR << ", tire_slip_ratio_RL =" << tire_slip_ratio_RL << ", tire_slip_ratio_RR =" << tire_slip_ratio_RR;
    qDebug() << "[processForzaHorizon4FormatData]" << "tire_combined_slip_FL =" << tire_combined_slip_FL << ", tire_combined_slip_FR =" << tire_combined_slip_FR << ", tire_combined_slip_RL =" << tire_combined_slip_RR << ", tire_slip_ratio_RR =" << tire_combined_slip_RR;
    qDebug() << "[processForzaHorizon4FormatData]" << "average_slip_ratio =" << average_slip_ratio << ", max_slip_ratio =" << max_slip_ratio;
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
    if (average_slip_ratio > gripThreshold_Brake || max_slip_ratio > gripThreshold_Brake) {
    // if (average_slip_ratio > gripThreshold_Brake) {
        if (pressedvJoyButtons.contains("vJoy-Key11(LT)_BRAKE") || pressedvJoyButtons.contains("vJoy-Key12(RT)_BRAKE")){
            if (s_Auto_Brake > AUTO_BRAKE_ADJUST_VALUE) {
                s_Auto_Brake -= AUTO_BRAKE_ADJUST_VALUE;
#ifdef GRIP_VERBOSE_LOG
                qDebug() << "[processForzaHorizon4FormatData]" << "s_Auto_Brake ----- ->" << s_Auto_Brake;
#endif
            }

            autoadjust |= AUTO_ADJUST_BRAKE;
        }
    }
    else {
        if (pressedvJoyButtons.contains("vJoy-Key11(LT)_BRAKE") || pressedvJoyButtons.contains("vJoy-Key12(RT)_BRAKE")){
            if (s_Auto_Brake < XINPUT_TRIGGER_MAX) {
                s_Auto_Brake += AUTO_BRAKE_ADJUST_VALUE;
                if (s_Auto_Brake > XINPUT_TRIGGER_MAX) {
                    s_Auto_Brake = XINPUT_TRIGGER_MAX;
                }
#ifdef GRIP_VERBOSE_LOG
                qDebug() << "[processForzaHorizon4FormatData]" << "s_Auto_Brake +++++ ->" << s_Auto_Brake;
#endif
            }

            autoadjust |= AUTO_ADJUST_BRAKE;
        }
    }

    if (average_slip_ratio > gripThreshold_Accel || max_slip_ratio > gripThreshold_Accel) {
    // if (average_slip_ratio > gripThreshold_Accel) {
        if (pressedvJoyButtons.contains("vJoy-Key11(LT)_ACCEL") || pressedvJoyButtons.contains("vJoy-Key12(RT)_ACCEL")){
            if (s_Auto_Accel > AUTO_ACCEL_ADJUST_VALUE) {
                s_Auto_Accel -= AUTO_ACCEL_ADJUST_VALUE;
#ifdef GRIP_VERBOSE_LOG
                qDebug() << "[processForzaHorizon4FormatData]" << "s_Auto_Accel ----- ->" << s_Auto_Accel;
#endif
            }

            autoadjust |= AUTO_ADJUST_ACCEL;
        }
    }
    else {
        if (pressedvJoyButtons.contains("vJoy-Key11(LT)_ACCEL") || pressedvJoyButtons.contains("vJoy-Key12(RT)_ACCEL")){
            if (s_Auto_Accel < XINPUT_TRIGGER_MAX) {
                s_Auto_Accel += AUTO_BRAKE_ADJUST_VALUE;
                if (s_Auto_Accel > XINPUT_TRIGGER_MAX) {
                    s_Auto_Accel = XINPUT_TRIGGER_MAX;
                }
#ifdef GRIP_VERBOSE_LOG
                qDebug() << "[processForzaHorizon4FormatData]" << "s_Auto_Accel +++++ ->" << s_Auto_Accel;
#endif
            }

            autoadjust |= AUTO_ADJUST_ACCEL;
        }
    }

    if (autoadjust) {
#ifdef GRIP_VERBOSE_LOG
        qDebug() << "[processForzaHorizon4FormatData]" << "Current Adjusted Auto Data ->" << "s_Auto_Brake =" << s_Auto_Brake << ", s_Auto_Accel =" << s_Auto_Accel;
#endif
        QString autoadjustEmptyStr;
        ViGEmClient_PressButton(autoadjustEmptyStr, autoadjust);
    }

#if 0
    qDebug() << "is_race_on:" << is_race_on;
    qDebug() << "timestamp_ms:" << timestamp_ms;
    qDebug() << "engine_max_rpm:" << engine_max_rpm;
    qDebug() << "engine_idle_rpm:" << engine_idle_rpm;
    qDebug() << "current_engine_rpm:" << current_engine_rpm;
    qDebug() << "acceleration_x:" << acceleration_x;
    qDebug() << "acceleration_y:" << acceleration_y;
    qDebug() << "acceleration_z:" << acceleration_z;
    qDebug() << "velocity_x:" << velocity_x;
    qDebug() << "velocity_y:" << velocity_y;
    qDebug() << "velocity_z:" << velocity_z;
    qDebug() << "angular_velocity_x:" << angular_velocity_x;
    qDebug() << "angular_velocity_y:" << angular_velocity_y;
    qDebug() << "angular_velocity_z:" << angular_velocity_z;
    qDebug() << "yaw:" << yaw;
    qDebug() << "pitch:" << pitch;
    qDebug() << "roll:" << roll;
    qDebug() << "norm_suspension_travel_FL:" << norm_suspension_travel_FL;
    qDebug() << "norm_suspension_travel_FR:" << norm_suspension_travel_FR;
    qDebug() << "norm_suspension_travel_RL:" << norm_suspension_travel_RL;
    qDebug() << "norm_suspension_travel_RR:" << norm_suspension_travel_RR;
    qDebug() << "tire_slip_ratio_FL:" << tire_slip_ratio_FL;
    qDebug() << "tire_slip_ratio_FR:" << tire_slip_ratio_FR;
    qDebug() << "tire_slip_ratio_RL:" << tire_slip_ratio_RL;
    qDebug() << "tire_slip_ratio_RR:" << tire_slip_ratio_RR;
    qDebug() << "wheel_rotation_speed_FL:" << wheel_rotation_speed_FL;
    qDebug() << "wheel_rotation_speed_FR:" << wheel_rotation_speed_FR;
    qDebug() << "wheel_rotation_speed_RL:" << wheel_rotation_speed_RL;
    qDebug() << "wheel_rotation_speed_RR:" << wheel_rotation_speed_RR;
    qDebug() << "wheel_on_rumble_strip_FL:" << wheel_on_rumble_strip_FL;
    qDebug() << "wheel_on_rumble_strip_FR:" << wheel_on_rumble_strip_FR;
    qDebug() << "wheel_on_rumble_strip_RL:" << wheel_on_rumble_strip_RL;
    qDebug() << "wheel_on_rumble_strip_RR:" << wheel_on_rumble_strip_RR;
    qDebug() << "wheel_in_puddle_FL:" << wheel_in_puddle_FL;
    qDebug() << "wheel_in_puddle_FR:" << wheel_in_puddle_FR;
    qDebug() << "wheel_in_puddle_RL:" << wheel_in_puddle_RL;
    qDebug() << "wheel_in_puddle_RR:" << wheel_in_puddle_RR;
    qDebug() << "surface_rumble_FL:" << surface_rumble_FL;
    qDebug() << "surface_rumble_FR:" << surface_rumble_FR;
    qDebug() << "surface_rumble_RL:" << surface_rumble_RL;
    qDebug() << "surface_rumble_RR:" << surface_rumble_RR;
    qDebug() << "tire_slip_angle_FL:" << tire_slip_angle_FL;
    qDebug() << "tire_slip_angle_FR:" << tire_slip_angle_FR;
    qDebug() << "tire_slip_angle_RL:" << tire_slip_angle_RL;
    qDebug() << "tire_slip_angle_RR:" << tire_slip_angle_RR;
    qDebug() << "tire_combined_slip_FL:" << tire_combined_slip_FL;
    qDebug() << "tire_combined_slip_FR:" << tire_combined_slip_FR;
    qDebug() << "tire_combined_slip_RL:" << tire_combined_slip_RL;
    qDebug() << "tire_combined_slip_RR:" << tire_combined_slip_RR;
    qDebug() << "suspension_travel_meters_FL:" << suspension_travel_meters_FL;
    qDebug() << "suspension_travel_meters_FR:" << suspension_travel_meters_FR;
    qDebug() << "suspension_travel_meters_RL:" << suspension_travel_meters_RL;
    qDebug() << "suspension_travel_meters_RR:" << suspension_travel_meters_RR;
    qDebug() << "car_ordinal:" << car_ordinal;
    qDebug() << "car_class:" << car_class;
    qDebug() << "car_performance_index:" << car_performance_index;
    qDebug() << "drivetrain_type:" << drivetrain_type;
    qDebug() << "num_cylinders:" << num_cylinders;
#endif
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

void QKeyMapper_Worker::startBurstTimer(const QString &burstKey, int mappingIndex)
{
    Q_UNUSED(mappingIndex);
    if (true == m_BurstTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("startBurstTimer(): Key \"%s\" BurstTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstTimerMap.remove(burstKey);
    }
    if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("startBurstTimer(): Key \"%s\" BurstKeyUpTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstKeyUpTimerMap.remove(burstKey);
    }
    int burstpressTime = QKeyMapper::getBurstPressTime();
    int burstreleaseTime = QKeyMapper::getBurstReleaseTime();
    int burstTime = burstpressTime + burstreleaseTime;
    int timerID = startTimer(burstTime, Qt::PreciseTimer);
    m_BurstTimerMap.insert(burstKey, timerID);

    int keyupTimerID = startTimer(burstpressTime, Qt::PreciseTimer);
    m_BurstKeyUpTimerMap.insert(burstKey, keyupTimerID);

#ifdef DEBUG_LOGOUT_ON
    qDebug("startBurstTimer(): Key \"%s\", Timer:%d, ID:%d", burstKey.toStdString().c_str(), burstTime, timerID);
#endif
}

void QKeyMapper_Worker::stopBurstTimer(const QString &burstKey, int mappingIndex)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("stopBurstTimer(): Key \"%s\"", burstKey.toStdString().c_str());
#endif
    Q_UNUSED(mappingIndex);

    if (true == m_BurstTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstTimerMap.value(burstKey);
        killTimer(existTimerID);
        m_BurstTimerMap.remove(burstKey);

        if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
            sendBurstKeyUp(burstKey, true);
#ifdef DEBUG_LOGOUT_ON
            qDebug("stopBurstTimer(): sendBurstKeyUp(\"%s\"), BurstTimer(%d) stoped.", burstKey.toStdString().c_str(), existTimerID);
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug("stopBurstTimer(): Do not need to sendBurstKeyUp(\"%s\"), BurstTimer(%d) stoped.", burstKey.toStdString().c_str(), existTimerID);
#endif
        }
    }

    if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("stopBurstTimer(): Key \"%s\" kill BurstKeyUpTimer(%d)", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstKeyUpTimerMap.remove(burstKey);
    }
}

void QKeyMapper_Worker::onJoystickcountChanged()
{
    QList<QJoystickDevice *> joysticklist = QJoysticks::getInstance()->inputDevices();
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onJoystickcountChanged]" << "JoystickList Start >>>";
#endif
    int joystick_index = 0;
    for (const QJoystickDevice *joystick : qAsConst(joysticklist)) {
        bool virtualgamepad = false;
        USHORT vendorid = joystick->vendorid;
        USHORT productid = joystick->productid;

        if (vendorid == VIRTUALGAMPAD_VENDORID_X360
            && productid == VIRTUALGAMPAD_PRODUCTID_X360) {
            virtualgamepad = true;
        }
        else if (joystick->serial == VIRTUALGAMPAD_SERIAL_DS4
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
        qDebug().nospace() << "[onJoystickcountChanged] Joystick[" << joystick_index << "] -> " << "Name = " << joystick->name << ", VendorID = " << vendorIdStr << ", ProductID = " << productIdStr << ", Serial = " << joystick->serial << ", Virtualgamepad = " << virtualgamepad;
#endif

        joystick_index += 1;
    }
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onJoystickcountChanged]" << "JoystickList End <<<";
#endif
}

void QKeyMapper_Worker::onJoystickPOVEvent(const QJoystickPOVEvent &e)
{
#ifdef JOYSTICK_VERBOSE_LOG
    qDebug() << "[onJoystickPOVEvent]" << "POV ->" << e.pov << "," << "POV Angle ->" << e.angle;
#endif

#ifdef SDL_JOYSTICK_BLACKLIST
    if (e.joystick->blacklisted) {
        return;
    }
#endif

    if (m_JoystickCapture) {
        checkJoystickPOV(e);
    }
}

void QKeyMapper_Worker::onJoystickAxisEvent(const QJoystickAxisEvent &e)
{
#ifdef JOYSTICK_VERBOSE_LOG
    qDebug() << "[onJoystickAxisEvent]" << "axis ->" << e.axis << "," << "axis value ->" << e.value;
#endif

#ifdef SDL_JOYSTICK_BLACKLIST
    if (e.joystick->blacklisted) {
        return;
    }
#endif

    if (m_JoystickCapture) {
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
}

void QKeyMapper_Worker::onJoystickButtonEvent(const QJoystickButtonEvent &e)
{
#ifdef JOYSTICK_VERBOSE_LOG
    qDebug() << "[onJoystickButtonEvent]" << "Button ->" << e.button << "," << "Pressed ->" << e.pressed;
#endif

#ifdef SDL_JOYSTICK_BLACKLIST
    if (e.joystick->blacklisted) {
        return;
    }
#endif

    if (m_JoystickCapture) {
        checkJoystickButtons(e);
    }
}

void QKeyMapper_Worker::checkJoystickButtons(const QJoystickButtonEvent &e)
{
    if (e.joystick == Q_NULLPTR)
        return;

    JoystickButtonCode buttonCode = (JoystickButtonCode)e.button;

    if (m_JoystickButtonMap.contains(buttonCode)) {
        bool pressed = e.pressed;
        QString keycodeString = m_JoystickButtonMap.value(buttonCode);
        int keyupdown;
        if (pressed) {
            keyupdown = KEY_DOWN;
        }
        else {
            keyupdown = KEY_UP;
        }

        bool returnFlag;
        returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick->name);
        Q_UNUSED(returnFlag);
    }
}

void QKeyMapper_Worker::checkJoystickPOV(const QJoystickPOVEvent &e)
{
    if (e.joystick == Q_NULLPTR)
        return;

    if (m_JoystickPOVMap.contains(e.angle)) {
        JoystickDPadCode dpadCode = m_JoystickPOVMap.value(e.angle);

        int keyupdown = KEY_UP;
        if (JOYSTICK_DPAD_RELEASE == dpadCode) {
            keyupdown = KEY_UP;
        }
        else {
            keyupdown = KEY_DOWN;
        }

        QStringList joydpadpressedlist = pressedRealKeysList.filter("Joy-DPad");
        QStringList joydpadNeedtoRelease = joydpadpressedlist;
        bool returnFlag;
        if (KEY_DOWN == keyupdown) {
            QString keycodeString = m_JoystickDPadMap.value(dpadCode);
            QStringList tempDpadCodeStringList = keycodeString.split(',');

            for (const QString &dpadcodestr : qAsConst(tempDpadCodeStringList)) {
                joydpadNeedtoRelease.removeAll(dpadcodestr);
            }

            for (const QString &dpadcodestr : qAsConst(joydpadNeedtoRelease)) {
                returnFlag = JoyStickKeysProc(dpadcodestr, KEY_UP, e.joystick->name);
                Q_UNUSED(returnFlag);
            }

            for (const QString &dpadcodestr : qAsConst(tempDpadCodeStringList)) {
                returnFlag = JoyStickKeysProc(dpadcodestr, keyupdown, e.joystick->name);
                Q_UNUSED(returnFlag);
            }
        }
        else {
            joydpadpressedlist = pressedRealKeysList.filter("Joy-DPad");
            for (const QString &joydpadstr : qAsConst(joydpadpressedlist)){
                returnFlag = JoyStickKeysProc(joydpadstr, keyupdown, e.joystick->name);
                Q_UNUSED(returnFlag);
            }
        }
    }
}

void QKeyMapper_Worker::checkJoystickAxis(const QJoystickAxisEvent &e)
{
    if (JOYSTICK_AXIS_LT_BUTTON == e.axis || JOYSTICK_AXIS_RT_BUTTON == e.axis) {
        joystickLTRTButtonProc(e);
    }
    else if (JOYSTICK_AXIS_LS_HORIZONTAL == e.axis) {
        s_JoyAxisState.left_x = e.value;
        joystickLSHorizontalProc(e);
    }
    else if (JOYSTICK_AXIS_LS_VERTICAL == e.axis) {
        s_JoyAxisState.left_y = e.value;
        joystickLSVerticalProc(e);
    }
    else if (JOYSTICK_AXIS_RS_HORIZONTAL == e.axis) {
        s_JoyAxisState.right_x = e.value;
        joystickRSHorizontalProc(e);
    }
    else if (JOYSTICK_AXIS_RS_VERTICAL == e.axis) {
        s_JoyAxisState.right_y = e.value;
        joystickRSVerticalProc(e);
    }
}

QKeyMapper_Worker::Joy2MouseState QKeyMapper_Worker::checkJoystick2MouseEnableState()
{
    Joy2MouseState joy2mouse_enablestate = JOY2MOUSE_NONE;
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

void QKeyMapper_Worker::joystickLTRTButtonProc(const QJoystickAxisEvent &e)
{
    int keyupdown = KEY_INIT;
    QString keycodeString;
    /* LT Button & RT Button */
    if (JOYSTICK_AXIS_LT_BUTTON == e.axis) {
        keycodeString = m_JoystickButtonMap.value(JOYSTICK_BUTTON_10);
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
        keycodeString = m_JoystickButtonMap.value(JOYSTICK_BUTTON_11);
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
        returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick->name);
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

    QString keycodeString_LS_Left = m_JoystickLStickMap.value(JOYSTICK_LS_LEFT);
    QString keycodeString_LS_Right = m_JoystickLStickMap.value(JOYSTICK_LS_RIGHT);
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
            returnFlag = JoyStickKeysProc(keycodeString_LS_Left, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
            /* Need to send Left-Stick Horizontal Left Release first <<< */
            keycodeString = keycodeString_LS_Right;
            keyupdown = KEY_DOWN;
        }
        /* Left-Stick Horizontal Right changed to Left */
        else if (ls_Right_Pressed && e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD) {
            /* Need to send Left-Stick Horizontal Right Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_LS_Right, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
            /* Need to send Left-Stick Horizontal Right Release first <<< */
            keycodeString = keycodeString_LS_Left;
            keyupdown = KEY_DOWN;
        }
    }
    else {
        /* Left-Stick Horizontal Release change to Right  */
        if (e.value >= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD) {
            keycodeString = keycodeString_LS_Right;
            keyupdown = KEY_DOWN;
        }
        /* Left-Stick Horizontal Release change to Left  */
        else if (e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD) {
            keycodeString = keycodeString_LS_Left;
            keyupdown = KEY_DOWN;
        }
    }

    if (KEY_DOWN == keyupdown) {
        returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick->name);
        Q_UNUSED(returnFlag);
    }
    else if (KEY_UP == keyupdown){
        if (ls_Left_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_LS_Left, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
        }
        if (ls_Right_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_LS_Right, KEY_UP, e.joystick->name);
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

    QString keycodeString_LS_Up = m_JoystickLStickMap.value(JOYSTICK_LS_UP);
    QString keycodeString_LS_Down = m_JoystickLStickMap.value(JOYSTICK_LS_DOWN);
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
            returnFlag = JoyStickKeysProc(keycodeString_LS_Up, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
            /* Need to send Left-Stick Vertical Up Release first <<< */
            keycodeString = keycodeString_LS_Down;
            keyupdown = KEY_DOWN;
        }
        /* Left-Stick Vertical Down changed to Up */
        else if (ls_Down_Pressed && e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD) {
            /* Need to send Left-Stick Vertical Down Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_LS_Down, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
            /* Need to send Left-Stick Vertical Down Release first <<< */
            keycodeString = keycodeString_LS_Up;
            keyupdown = KEY_DOWN;
        }
    }
    else {
        /* Left-Stick Vertical Release change to Down  */
        if (e.value >= JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD) {
            keycodeString = keycodeString_LS_Down;
            keyupdown = KEY_DOWN;
        }
        /* Left-Stick Vertical Release change to Up  */
        else if (e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD) {
            keycodeString = keycodeString_LS_Up;
            keyupdown = KEY_DOWN;
        }
    }

    if (KEY_DOWN == keyupdown) {
        returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick->name);
        Q_UNUSED(returnFlag);
    }
    else if (KEY_UP == keyupdown){
        if (ls_Up_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_LS_Up, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
        }
        if (ls_Down_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_LS_Down, KEY_UP, e.joystick->name);
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

    QString keycodeString_RS_Left = m_JoystickRStickMap.value(JOYSTICK_RS_LEFT);
    QString keycodeString_RS_Right = m_JoystickRStickMap.value(JOYSTICK_RS_RIGHT);
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
            returnFlag = JoyStickKeysProc(keycodeString_RS_Left, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
            /* Need to send Right-Stick Horizontal Left Release first <<< */
            keycodeString = keycodeString_RS_Right;
            keyupdown = KEY_DOWN;
        }
        /* Right-Stick Horizontal Right changed to Left */
        else if (rs_Right_Pressed && e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD) {
            /* Need to send Right-Stick Horizontal Right Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_RS_Right, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
            /* Need to send Right-Stick Horizontal Right Release first <<< */
            keycodeString = keycodeString_RS_Left;
            keyupdown = KEY_DOWN;
        }
    }
    else {
        /* Right-Stick Horizontal Release change to Right  */
        if (e.value >= JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD) {
            keycodeString = keycodeString_RS_Right;
            keyupdown = KEY_DOWN;
        }
        /* Right-Stick Horizontal Release change to Left  */
        else if (e.value <= JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD) {
            keycodeString = keycodeString_RS_Left;
            keyupdown = KEY_DOWN;
        }
    }

    if (KEY_DOWN == keyupdown) {
        returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick->name);
        Q_UNUSED(returnFlag);
    }
    else if (KEY_UP == keyupdown){
        if (rs_Left_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_RS_Left, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
        }
        if (rs_Right_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_RS_Right, KEY_UP, e.joystick->name);
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

    QString keycodeString_RS_Up = m_JoystickRStickMap.value(JOYSTICK_RS_UP);
    QString keycodeString_RS_Down = m_JoystickRStickMap.value(JOYSTICK_RS_DOWN);
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
            returnFlag = JoyStickKeysProc(keycodeString_RS_Up, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
            /* Need to send Right-Stick Vertical Up Release first <<< */
            keycodeString = keycodeString_RS_Down;
            keyupdown = KEY_DOWN;
        }
        /* Right-Stick Vertical Down changed to Up */
        else if (rs_Down_Pressed && e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD) {
            /* Need to send Right-Stick Vertical Down Release first >>> */
            returnFlag = JoyStickKeysProc(keycodeString_RS_Down, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
            /* Need to send Right-Stick Vertical Down Release first <<< */
            keycodeString = keycodeString_RS_Up;
            keyupdown = KEY_DOWN;
        }
    }
    else {
        /* Right-Stick Vertical Release change to Down  */
        if (e.value >= JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD) {
            keycodeString = keycodeString_RS_Down;
            keyupdown = KEY_DOWN;
        }
        /* Right-Stick Vertical Release change to Up  */
        else if (e.value <= JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD) {
            keycodeString = keycodeString_RS_Up;
            keyupdown = KEY_DOWN;
        }
    }

    if (KEY_DOWN == keyupdown) {
        returnFlag = JoyStickKeysProc(keycodeString, keyupdown, e.joystick->name);
        Q_UNUSED(returnFlag);
    }
    else if (KEY_UP == keyupdown){
        if (rs_Up_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_RS_Up, KEY_UP, e.joystick->name);
            Q_UNUSED(returnFlag);
        }
        if (rs_Down_Pressed) {
            returnFlag = JoyStickKeysProc(keycodeString_RS_Down, KEY_UP, e.joystick->name);
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
            qreal range = 1.0 - JOYSTICK2MOUSE_AXIS_PLUS_LOW_THRESHOLD;
            qreal step = range / Speed_Factor;
            for (int i = 1; i <= Speed_Factor; i++) {
                if (axis_value <= JOYSTICK2MOUSE_AXIS_PLUS_LOW_THRESHOLD + i * step) {
                    delta += i;
                    break;
                }
            }
        }
        else if (axis_value < JOYSTICK2MOUSE_AXIS_MINUS_LOW_THRESHOLD) {
            qreal range = 1.0 - JOYSTICK2MOUSE_AXIS_MINUS_LOW_THRESHOLD;
            qreal step = range / Speed_Factor;
            for (int i = 1; i <= Speed_Factor; i++) {
                if (axis_value >= JOYSTICK2MOUSE_AXIS_MINUS_LOW_THRESHOLD - i * step) {
                    delta -= i;
                    break;
                }
            }
        }
    }
    return delta;
}

void QKeyMapper_Worker::joystick2MouseMoveProc(const Joystick_AxisState &axis_state)
{
    int delta_x = 0;
    int delta_y = 0;
    bool checkLeftJoystick = false;
    bool checkRightJoystick = false;
    int Speed_Factor_X = QKeyMapper::getJoystick2MouseSpeedX();
    int Speed_Factor_Y = QKeyMapper::getJoystick2MouseSpeedY();

    if (JOY2MOUSE_LEFT == s_Joy2Mouse_EnableState) {
        checkLeftJoystick = true;
    }
    else if (JOY2MOUSE_RIGHT == s_Joy2Mouse_EnableState) {
        checkRightJoystick = true;
    }
    else if (JOY2MOUSE_BOTH == s_Joy2Mouse_EnableState) {
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
    }
}

LRESULT QKeyMapper_Worker::LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
#ifdef HOOKSTART_ONSTARTUP
    bool hookprocstart = QKeyMapper_Worker::s_AtomicHookProcStart;
    if (!hookprocstart) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }
#endif

    if (nCode != HC_ACTION) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;
    ULONG_PTR extraInfo = pKeyBoard->dwExtraInfo;

    bool returnFlag = false;
    V_KEYCODE vkeycode;
    vkeycode.KeyCode = (quint8)pKeyBoard->vkCode;
    if (LLKHF_EXTENDED == (pKeyBoard->flags & LLKHF_EXTENDED)){
        vkeycode.ExtenedFlag = EXTENED_FLAG_TRUE;
    }
    else{
        vkeycode.ExtenedFlag = EXTENED_FLAG_FALSE;
    }

    QString keycodeString = VirtualKeyCodeMap.key(vkeycode);

    if (VIRTUAL_WIN_PLUS_D == extraInfo) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[LowLevelKeyboardHookProc] Ignore extraInfo:VIRTUAL_WIN_PLUS_D(0x%08X) -> \"%s\" (0x%02X),  wParam(0x%04X), scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", extraInfo, keycodeString.toStdString().c_str(), pKeyBoard->vkCode, wParam, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
#endif
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

//#ifdef DEBUG_LOGOUT_ON
//    qDebug("\"%s\" (0x%02X),  wParam(0x%04X), scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, wParam, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
//#endif

    if ((false == keycodeString.isEmpty())
        && (WM_KEYDOWN == wParam || WM_KEYUP == wParam || WM_SYSKEYDOWN == wParam || WM_SYSKEYUP == wParam)){
#ifdef DEBUG_LOGOUT_ON
        qDebug("[LowLevelKeyboardHookProc] currentThread -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());

        if (extraInfo != VIRTUAL_KEYBOARD_PRESS) {
            if (WM_KEYDOWN == wParam){
                qDebug("[LowLevelKeyboardHookProc] RealKey: \"%s\" (0x%02X) KeyDown, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
            }
            else if (WM_KEYUP == wParam){
                qDebug("[LowLevelKeyboardHookProc] RealKey: \"%s\" (0x%02X) KeyUp, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
            }
            else if (WM_SYSKEYDOWN == wParam){
                qDebug("[LowLevelKeyboardHookProc] RealKey: \"%s\" (0x%02X) SysKeyDown, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
            }
            else if (WM_SYSKEYUP == wParam){
                qDebug("[LowLevelKeyboardHookProc] RealKey: \"%s\" (0x%02X) SysKeyUp, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
            }
            else{
            }
        }
        else {
            if (WM_KEYDOWN == wParam){
                qDebug("[LowLevelKeyboardHookProc] VirtualKey: \"%s\" (0x%02X) KeyDown, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
            }
            else if (WM_KEYUP == wParam){
                qDebug("[LowLevelKeyboardHookProc] VirtualKey: \"%s\" (0x%02X) KeyUp, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
            }
            else if (WM_SYSKEYDOWN == wParam){
                qDebug("[LowLevelKeyboardHookProc] VirtualKey: \"%s\" (0x%02X) SysKeyDown, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
            }
            else if (WM_SYSKEYUP == wParam){
                qDebug("[LowLevelKeyboardHookProc] VirtualKey: \"%s\" (0x%02X) SysKeyUp, scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
            }
            else{
            }
        }
#endif
        int keyupdown;
        if (WM_KEYDOWN == wParam || WM_SYSKEYDOWN == wParam) {
            keyupdown = KEY_DOWN;
        }
        else {
            keyupdown = KEY_UP;
        }

        if (extraInfo != VIRTUAL_KEYBOARD_PRESS) {
            int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
            returnFlag = hookBurstAndLockProc(keycodeString, keyupdown);

            if (true == QKeyMapper::getDisableWinKeyStatus()) {
                if (KEY_DOWN == keyupdown) {
                    if ("D" == keycodeString && pressedRealKeysList.contains("L-Win")) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug("[LowLevelKeyboardHookProc] \"L-Win + D\" pressed!");
#endif
                        emit QKeyMapper_Worker::getInstance()->send_WINplusD_Signal();
                    }
                }

                if (("L-Win" == keycodeString)
                    || ("R-Win" == keycodeString)
                    || ("Application" == keycodeString)) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug("[LowLevelKeyboardHookProc] Disable \"%s\" (0x%02X), wParam(0x%04X), scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, wParam, pKeyBoard->scanCode, pKeyBoard->flags);
#endif
                    returnFlag = true;
                }
            }

            if (KEY_UP == keyupdown && false == returnFlag){
                if (findindex >=0 && (QKeyMapper::KeyMappingDataList.at(findindex).Original_Key == keycodeString)) {
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
                    QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
                    QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
                    if (KEY_DOWN == keyupdown){
                        emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                        returnFlag = true;
                    }
                    else { /* KEY_UP == keyupdown */
                        emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                        returnFlag = true;
                    }
                }
            }
        }
        else {
            if (KEY_DOWN == keyupdown){
                if (false == pressedVirtualKeysList.contains(keycodeString)){
                    pressedVirtualKeysList.append(keycodeString);
                }
            }
            /* KEY_UP == keyupdown */
            else {
                if (s_forceSendVirtualKey != true) {
                    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
                    if (pressedRealKeysList.contains(keycodeString) && findindex < 0){
    #ifdef DEBUG_LOGOUT_ON
                        qDebug("[LowLevelKeyboardHookProc] RealKey \"%s\" is pressed down on keyboard, skip send mapping VirtualKey \"%s\" KEYUP!", keycodeString.toStdString().c_str(), keycodeString.toStdString().c_str());
    #endif
                        returnFlag = true;
                    }
                }

                pressedVirtualKeysList.removeAll(keycodeString);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[LowLevelKeyboardHookProc]" << (keyupdown == KEY_DOWN?"KEY_DOWN":"KEY_UP") << " : pressedVirtualKeysList -> " << pressedVirtualKeysList;
#endif
        }
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug("[LowLevelKeyboardHookProc] UnknownKey (0x%02X) Input, scanCode(0x%08X), wParam(0x%08X), flags(0x%08X), ExtenedFlag(%s)", pKeyBoard->vkCode, pKeyBoard->scanCode, wParam, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
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
#ifdef HOOKSTART_ONSTARTUP
    bool hookprocstart = QKeyMapper_Worker::s_AtomicHookProcStart;
    if (!hookprocstart) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }
#endif

    if (nCode != HC_ACTION) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

    MSLLHOOKSTRUCT *pMouse = (MSLLHOOKSTRUCT *)lParam;
    ULONG_PTR extraInfo = pMouse->dwExtraInfo;
    DWORD mousedata = pMouse->mouseData;

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
            if (VIRTUAL_MOUSE_CLICK == extraInfo) {
#ifdef DEBUG_LOGOUT_ON
                qDebug("Virtual \"%s\" %s", MouseButtonNameMap.value(wParam_X).toStdString().c_str(), (keyupdown == KEY_DOWN?"Button Down":"Button Up"));
#endif
                if (KEY_DOWN == keyupdown) {
                    if (false == pressedVirtualKeysList.contains(keycodeString)){
                        pressedVirtualKeysList.append(keycodeString);
                    }
                }
                else {
                    pressedVirtualKeysList.removeAll(keycodeString);
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug("Real \"%s\" %s", MouseButtonNameMap.value(wParam_X).toStdString().c_str(), (keyupdown == KEY_DOWN?"Button Down":"Button Up"));
#endif
                int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
                returnFlag = hookBurstAndLockProc(keycodeString, keyupdown);

                if (KEY_UP == keyupdown && false == returnFlag){
                    if (findindex >=0 && (QKeyMapper::KeyMappingDataList.at(findindex).Original_Key == keycodeString)) {
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
                        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
                        QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
                        if (KEY_DOWN == keyupdown){
                            emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            returnFlag = true;
                        }
                        else { /* KEY_UP == keyupdown */
                            emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                            returnFlag = true;
                        }
                    }
                }
            }
        }
    }
    else if (wParam == WM_MOUSEWHEEL) {
        if (VIRTUAL_MOUSE_WHEEL == extraInfo) {
#ifdef DEBUG_LOGOUT_ON
            short zDelta = GET_WHEEL_DELTA_WPARAM(mousedata);

            if (zDelta != 0) {
#ifdef MOUSE_VERBOSE_LOG
                qDebug() << "[LowLevelMouseHookProc]" << "Virtual Mouse Wheel -> Delta =" << zDelta;
#endif
                if (zDelta > 0) {
                    qDebug() << "[LowLevelMouseHookProc]" << "Virtual Mouse Wheel Up";
                }
                else {
                    qDebug() << "[LowLevelMouseHookProc]" << "Virtual Mouse Wheel Down";
                }
            }
#endif
        }
        else {
            short zDelta = GET_WHEEL_DELTA_WPARAM(mousedata);

            if (zDelta != 0) {
#ifdef MOUSE_VERBOSE_LOG
                qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel -> Delta =" << zDelta;
#endif
                short delta_abs = qAbs(zDelta);
                if (delta_abs >= WHEEL_DELTA) {
                    bool wheel_up_found = false;
                    bool wheel_down_found = false;
                    bool send_wheel_keys = false;
                    int findindex = -1;

                    int findWheelUpindex = QKeyMapper::findOriKeyInKeyMappingDataList(MOUSE_STR_WHEEL_UP);
                    if (findWheelUpindex >=0){
                        wheel_up_found = true;
                    }

                    int findWheelDownindex = QKeyMapper::findOriKeyInKeyMappingDataList(MOUSE_STR_WHEEL_DOWN);
                    if (findWheelDownindex >=0){
                        wheel_down_found = true;
                    }

                    if (wheel_up_found || wheel_down_found) {
                        if (wheel_up_found && zDelta > 0) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Up -> Send Wheel Up Mapping Keys";
#endif
                            send_wheel_keys = true;
                            findindex = findWheelUpindex;
                        }
                        else if (wheel_down_found && zDelta < 0) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Down -> Send Wheel Down Mapping Keys";
#endif
                            send_wheel_keys = true;
                            findindex = findWheelDownindex;
                        }

                        if (send_wheel_keys) {
                            QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
                            QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
                            emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                            emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
                            returnFlag = true;
                        }
                    }
                }
            }
        }
    }
#ifdef VIGEM_CLIENT_SUPPORT
    else if (wParam == WM_MOUSEMOVE) {
#ifdef MOUSE_VERBOSE_LOG
        qDebug() << "[LowLevelMouseHookProc]" << "Mouse Move -> X =" << pMouse->pt.x << ", Y = " << pMouse->pt.y;
#endif

        if (extraInfo != VIRTUAL_MOUSE_MOVE) {
            if (s_Mouse2vJoy_EnableState != MOUSE2VJOY_NONE) {
                s_Mouse2vJoy_delta.rx() = pMouse->pt.x - s_Mouse2vJoy_prev.x();
                s_Mouse2vJoy_delta.ry() = pMouse->pt.y - s_Mouse2vJoy_prev.y();

                if (QKeyMapper::getLockCursorStatus()) {
                    returnFlag = true;
                }
                else {
                    s_Mouse2vJoy_prev.rx() = pMouse->pt.x;
                    s_Mouse2vJoy_prev.ry() = pMouse->pt.y;
                }
// #ifdef MOUSE_VERBOSE_LOG
//                 qDebug() << "[LowLevelMouseHookProc]" << "Mouse Move -> Delta X =" << s_Mouse2vJoy_delta.x() << ", Delta Y = " << s_Mouse2vJoy_delta.y();
// #endif
                emit QKeyMapper_Worker::getInstance()->onMouseMove_Signal(pMouse->pt.x, pMouse->pt.y);
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

bool QKeyMapper_Worker::hookBurstAndLockProc(const QString &keycodeString, int keyupdown)
{
    bool returnFlag = false;
    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);

    if (KEY_DOWN == keyupdown){
        if (false == pressedRealKeysList.contains(keycodeString)){
            if (findindex >=0 && true == QKeyMapper::KeyMappingDataList.at(findindex).Burst) {
                if (true == QKeyMapper::KeyMappingDataList.at(findindex).Lock) {
                    if (true == QKeyMapper::KeyMappingDataList.at(findindex).LockStatus) {
                        returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                        qDebug("hookBurstAndLockProc(): Lock ON & Burst ON(KEY_DOWN) -> Key \"%s\" LockStatus is ON, skip startBurstTimer()!", keycodeString.toStdString().c_str());
#endif
                    }
                    else {
                        emit QKeyMapper_Worker::getInstance()->startBurstTimer_Signal(keycodeString, findindex);
                    }
                }
                else {
                    emit QKeyMapper_Worker::getInstance()->startBurstTimer_Signal(keycodeString, findindex);
                }
            }
            pressedRealKeysList.append(keycodeString);
        }

        if (findindex >=0 && true == QKeyMapper::KeyMappingDataList.at(findindex).Lock) {
            if (true == pressedLockKeysList.contains(keycodeString)){
                QKeyMapper::KeyMappingDataList[findindex].LockStatus = false;
                pressedLockKeysList.removeAll(keycodeString);
                updateLockStatus();
#ifdef DEBUG_LOGOUT_ON
                qDebug("hookBurstAndLockProc(): Key \"%s\" KeyDown LockStatus -> OFF", keycodeString.toStdString().c_str());
#endif
            }
            else {
                QKeyMapper::KeyMappingDataList[findindex].LockStatus = true;
                pressedLockKeysList.append(keycodeString);
                updateLockStatus();
#ifdef DEBUG_LOGOUT_ON
                qDebug("hookBurstAndLockProc(): Key \"%s\" KeyDown LockStatus -> ON", keycodeString.toStdString().c_str());
#endif
            }
        }
    }
    else {  /* KEY_UP == keyupdown */
        if (true == pressedRealKeysList.contains(keycodeString)){
            if (findindex >=0) {
                if (true == QKeyMapper::KeyMappingDataList.at(findindex).Lock) {
                    /* Lock ON &  Burst ON */
                    if (true == QKeyMapper::KeyMappingDataList.at(findindex).Burst) {
                        if (true == QKeyMapper::KeyMappingDataList.at(findindex).LockStatus) {
                            returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                            qDebug("hookBurstAndLockProc(): Lock ON & Burst ON(KEY_UP) -> Key \"%s\" LockStatus is ON, skip stopBurstTimer()!", keycodeString.toStdString().c_str());
#endif
                        }
                        else {
                            emit QKeyMapper_Worker::getInstance()->stopBurstTimer_Signal(keycodeString, findindex);
                            returnFlag = true;
                        }
                    }
                    /* Lock ON &  Burst OFF */
                    else {
                        if (true == QKeyMapper::KeyMappingDataList.at(findindex).LockStatus) {
                            returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                            qDebug("hookBurstAndLockProc(): Lock ON & Burst OFF -> Key \"%s\" LockStatus is ON, skip KeyUp!", keycodeString.toStdString().c_str());
#endif
                        }
                    }
                }
                else {
                    /* Lock OFF &  Burst ON */
                    if (true == QKeyMapper::KeyMappingDataList.at(findindex).Burst) {
                        emit QKeyMapper_Worker::getInstance()->stopBurstTimer_Signal(keycodeString, findindex);
                        returnFlag = true;
                    }
                    /* Lock OFF &  Burst OFF do nothing */
                }
            }
            pressedRealKeysList.removeAll(keycodeString);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[pressedRealKeysList]" << (keyupdown == KEY_DOWN?"KEY_DOWN":"KEY_UP") << " : Current Pressed RealKeys -> " << pressedRealKeysList;
#endif

    return returnFlag;
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

bool QKeyMapper_Worker::JoyStickKeysProc(const QString &keycodeString, int keyupdown, const QString &joystickName)
{
    Q_UNUSED(joystickName);

#ifdef DEBUG_LOGOUT_ON
    if (KEY_DOWN == keyupdown){
        qDebug("[JoyStickKeysProc] RealKey: \"%s\" KeyDown -> [%s]", keycodeString.toStdString().c_str(), joystickName.toStdString().c_str());
    }
    else if (KEY_UP == keyupdown){
        qDebug("[JoyStickKeysProc] RealKey: \"%s\" KeyUp -> [%s]", keycodeString.toStdString().c_str(), joystickName.toStdString().c_str());
    }
    else {
        /* Do Nothing */
    }
#endif

    bool returnFlag = false;
    int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keycodeString);
    returnFlag = hookBurstAndLockProc(keycodeString, keyupdown);

    if (false == returnFlag) {
        if (findindex >=0){
            QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
            QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
            if (KEY_DOWN == keyupdown){
                emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_NORMAL);
                returnFlag = true;
            }
            else { /* KEY_UP == keyupdown */
                emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_NORMAL);
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
    // US 104 Keyboard Main Area
    // Row 1
    VirtualKeyCodeMap.insert        ("`~",          V_KEYCODE(VK_OEM_3,         EXTENED_FLAG_FALSE));   // 0xC0
    VirtualKeyCodeMap.insert        ("1!",          V_KEYCODE(VK_1,             EXTENED_FLAG_FALSE));   // 0x31
    VirtualKeyCodeMap.insert        ("2@",          V_KEYCODE(VK_2,             EXTENED_FLAG_FALSE));   // 0x32
    VirtualKeyCodeMap.insert        ("3#",          V_KEYCODE(VK_3,             EXTENED_FLAG_FALSE));   // 0x33
    VirtualKeyCodeMap.insert        ("4$",          V_KEYCODE(VK_4,             EXTENED_FLAG_FALSE));   // 0x34
    VirtualKeyCodeMap.insert        ("5%",          V_KEYCODE(VK_5,             EXTENED_FLAG_FALSE));   // 0x35
    VirtualKeyCodeMap.insert        ("6^",          V_KEYCODE(VK_6,             EXTENED_FLAG_FALSE));   // 0x36
    VirtualKeyCodeMap.insert        ("7&",          V_KEYCODE(VK_7,             EXTENED_FLAG_FALSE));   // 0x37
    VirtualKeyCodeMap.insert        ("8*",          V_KEYCODE(VK_8,             EXTENED_FLAG_FALSE));   // 0x38
    VirtualKeyCodeMap.insert        ("9(",          V_KEYCODE(VK_9,             EXTENED_FLAG_FALSE));   // 0x39
    VirtualKeyCodeMap.insert        ("0)",          V_KEYCODE(VK_0,             EXTENED_FLAG_FALSE));   // 0x30
    VirtualKeyCodeMap.insert        ("-_",          V_KEYCODE(VK_OEM_MINUS,     EXTENED_FLAG_FALSE));   // 0xBD
    VirtualKeyCodeMap.insert        ("=+",          V_KEYCODE(VK_OEM_PLUS,      EXTENED_FLAG_FALSE));   // 0xBB
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
    VirtualKeyCodeMap.insert        ("[{",          V_KEYCODE(VK_OEM_4,         EXTENED_FLAG_FALSE));   // 0xDB
    VirtualKeyCodeMap.insert        ("]}",          V_KEYCODE(VK_OEM_6,         EXTENED_FLAG_FALSE));   // 0xDD
    VirtualKeyCodeMap.insert        ("\\|",         V_KEYCODE(VK_OEM_5,         EXTENED_FLAG_FALSE));   // 0xDC
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
    VirtualKeyCodeMap.insert        (";:",          V_KEYCODE(VK_OEM_1,         EXTENED_FLAG_FALSE));   // 0xBA
    VirtualKeyCodeMap.insert        ("'\"",         V_KEYCODE(VK_OEM_7,         EXTENED_FLAG_FALSE));   // 0xDE
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
    VirtualKeyCodeMap.insert        (",<",          V_KEYCODE(VK_OEM_COMMA,     EXTENED_FLAG_FALSE));   // 0xBC
    VirtualKeyCodeMap.insert        (".>",          V_KEYCODE(VK_OEM_PERIOD,    EXTENED_FLAG_FALSE));   // 0xBE
    VirtualKeyCodeMap.insert        ("/?",          V_KEYCODE(VK_OEM_2,         EXTENED_FLAG_FALSE));   // 0xBF
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
    VirtualKeyCodeMap.insert        ("Shift",       V_KEYCODE(VK_SHIFT,         EXTENED_FLAG_FALSE));   // 0x10
    VirtualKeyCodeMap.insert        ("Ctrl",        V_KEYCODE(VK_CONTROL,       EXTENED_FLAG_FALSE));   // 0x11
    VirtualKeyCodeMap.insert        ("Alt",         V_KEYCODE(VK_MENU,          EXTENED_FLAG_FALSE));   // 0x12

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
    VirtualKeyCodeMap.insert        ("Num /",               V_KEYCODE(VK_DIVIDE,        EXTENED_FLAG_TRUE ));   // 0x6F + E
    VirtualKeyCodeMap.insert        ("Num *",               V_KEYCODE(VK_MULTIPLY,      EXTENED_FLAG_FALSE));   // 0x6A
    VirtualKeyCodeMap.insert        ("Num -",               V_KEYCODE(VK_SUBTRACT,      EXTENED_FLAG_FALSE));   // 0x6D
    VirtualKeyCodeMap.insert        ("Num +",               V_KEYCODE(VK_ADD,           EXTENED_FLAG_FALSE));   // 0x6B
    VirtualKeyCodeMap.insert        ("Num .",               V_KEYCODE(VK_DECIMAL,       EXTENED_FLAG_FALSE));   // 0x6E
    VirtualKeyCodeMap.insert        ("Num 0",               V_KEYCODE(VK_NUMPAD0,       EXTENED_FLAG_FALSE));   // 0x60
    VirtualKeyCodeMap.insert        ("Num 1",               V_KEYCODE(VK_NUMPAD1,       EXTENED_FLAG_FALSE));   // 0x61
    VirtualKeyCodeMap.insert        ("Num 2",               V_KEYCODE(VK_NUMPAD2,       EXTENED_FLAG_FALSE));   // 0x62
    VirtualKeyCodeMap.insert        ("Num 3",               V_KEYCODE(VK_NUMPAD3,       EXTENED_FLAG_FALSE));   // 0x63
    VirtualKeyCodeMap.insert        ("Num 4",               V_KEYCODE(VK_NUMPAD4,       EXTENED_FLAG_FALSE));   // 0x64
    VirtualKeyCodeMap.insert        ("Num 5",               V_KEYCODE(VK_NUMPAD5,       EXTENED_FLAG_FALSE));   // 0x65
    VirtualKeyCodeMap.insert        ("Num 6",               V_KEYCODE(VK_NUMPAD6,       EXTENED_FLAG_FALSE));   // 0x66
    VirtualKeyCodeMap.insert        ("Num 7",               V_KEYCODE(VK_NUMPAD7,       EXTENED_FLAG_FALSE));   // 0x67
    VirtualKeyCodeMap.insert        ("Num 8",               V_KEYCODE(VK_NUMPAD8,       EXTENED_FLAG_FALSE));   // 0x68
    VirtualKeyCodeMap.insert        ("Num 9",               V_KEYCODE(VK_NUMPAD9,       EXTENED_FLAG_FALSE));   // 0x69
    VirtualKeyCodeMap.insert        ("Num Enter",           V_KEYCODE(VK_RETURN,        EXTENED_FLAG_TRUE ));   // 0x0D + E
    //NumLock Off NumberPadKeys
    VirtualKeyCodeMap.insert        ("Num .(NumOFF)",       V_KEYCODE(VK_DELETE,        EXTENED_FLAG_FALSE));   // 0x2E
    VirtualKeyCodeMap.insert        ("Num 0(NumOFF)",       V_KEYCODE(VK_INSERT,        EXTENED_FLAG_FALSE));   // 0x2D
    VirtualKeyCodeMap.insert        ("Num 1(NumOFF)",       V_KEYCODE(VK_END,           EXTENED_FLAG_FALSE));   // 0x23
    VirtualKeyCodeMap.insert        ("Num 2(NumOFF)",       V_KEYCODE(VK_DOWN,          EXTENED_FLAG_FALSE));   // 0x28
    VirtualKeyCodeMap.insert        ("Num 3(NumOFF)",       V_KEYCODE(VK_NEXT,          EXTENED_FLAG_FALSE));   // 0x22
    VirtualKeyCodeMap.insert        ("Num 4(NumOFF)",       V_KEYCODE(VK_LEFT,          EXTENED_FLAG_FALSE));   // 0x25
    VirtualKeyCodeMap.insert        ("Num 5(NumOFF)",       V_KEYCODE(VK_CLEAR,         EXTENED_FLAG_FALSE));   // 0x0C
    VirtualKeyCodeMap.insert        ("Num 6(NumOFF)",       V_KEYCODE(VK_RIGHT,         EXTENED_FLAG_FALSE));   // 0x27
    VirtualKeyCodeMap.insert        ("Num 7(NumOFF)",       V_KEYCODE(VK_HOME,          EXTENED_FLAG_FALSE));   // 0x24
    VirtualKeyCodeMap.insert        ("Num 8(NumOFF)",       V_KEYCODE(VK_UP,            EXTENED_FLAG_FALSE));   // 0x26
    VirtualKeyCodeMap.insert        ("Num 9(NumOFF)",       V_KEYCODE(VK_PRIOR,         EXTENED_FLAG_FALSE));   // 0x21

    // MultiMedia keys
    VirtualKeyCodeMap.insert        ("Vol Mute",            V_KEYCODE(VK_VOLUME_MUTE,           EXTENED_FLAG_TRUE));   // 0xAD
    VirtualKeyCodeMap.insert        ("Vol Down",            V_KEYCODE(VK_VOLUME_DOWN,           EXTENED_FLAG_TRUE));   // 0xAE
    VirtualKeyCodeMap.insert        ("Vol Up",              V_KEYCODE(VK_VOLUME_UP,             EXTENED_FLAG_TRUE));   // 0xAF
    VirtualKeyCodeMap.insert        ("Media Next",          V_KEYCODE(VK_MEDIA_NEXT_TRACK,      EXTENED_FLAG_TRUE));   // 0xB0
    VirtualKeyCodeMap.insert        ("Media Prev",          V_KEYCODE(VK_MEDIA_PREV_TRACK,      EXTENED_FLAG_TRUE));   // 0xB1
    VirtualKeyCodeMap.insert        ("Media Stop",          V_KEYCODE(VK_MEDIA_STOP,            EXTENED_FLAG_TRUE));   // 0xB2
    VirtualKeyCodeMap.insert        ("Media PlayPause",     V_KEYCODE(VK_MEDIA_PLAY_PAUSE,      EXTENED_FLAG_TRUE));   // 0xB3
    VirtualKeyCodeMap.insert        ("Launch Mail",         V_KEYCODE(VK_LAUNCH_MAIL,           EXTENED_FLAG_TRUE));   // 0xB4
    VirtualKeyCodeMap.insert        ("Select Media",        V_KEYCODE(VK_LAUNCH_MEDIA_SELECT,   EXTENED_FLAG_TRUE));   // 0xB5
    VirtualKeyCodeMap.insert        ("Launch App1",         V_KEYCODE(VK_LAUNCH_APP1,           EXTENED_FLAG_TRUE));   // 0xB6
    VirtualKeyCodeMap.insert        ("Launch App2",         V_KEYCODE(VK_LAUNCH_APP2,           EXTENED_FLAG_TRUE));   // 0xB7

    // Browser keys
    VirtualKeyCodeMap.insert        ("Browser Back",        V_KEYCODE(VK_BROWSER_BACK,          EXTENED_FLAG_TRUE));   // 0xA6
    VirtualKeyCodeMap.insert        ("Browser Forward",     V_KEYCODE(VK_BROWSER_FORWARD,       EXTENED_FLAG_TRUE));   // 0xA7
    VirtualKeyCodeMap.insert        ("Browser Refresh",     V_KEYCODE(VK_BROWSER_REFRESH,       EXTENED_FLAG_TRUE));   // 0xA8
    VirtualKeyCodeMap.insert        ("Browser Stop",        V_KEYCODE(VK_BROWSER_STOP,          EXTENED_FLAG_TRUE));   // 0xA9
    VirtualKeyCodeMap.insert        ("Browser Search",      V_KEYCODE(VK_BROWSER_SEARCH,        EXTENED_FLAG_TRUE));   // 0xAA
    VirtualKeyCodeMap.insert        ("Browser Favorites",   V_KEYCODE(VK_BROWSER_FAVORITES,     EXTENED_FLAG_TRUE));   // 0xAB
    VirtualKeyCodeMap.insert        ("Browser Home",        V_KEYCODE(VK_BROWSER_HOME,          EXTENED_FLAG_TRUE));   // 0xAC
}

void QKeyMapper_Worker::initVirtualMouseButtonMap()
{
    VirtualMouseButtonMap.insert("Mouse-L",             V_MOUSECODE(MOUSEEVENTF_LEFTDOWN,       MOUSEEVENTF_LEFTUP,     0           )); // Mouse Button Left
    VirtualMouseButtonMap.insert("Mouse-R",             V_MOUSECODE(MOUSEEVENTF_RIGHTDOWN,      MOUSEEVENTF_RIGHTUP,    0           )); // Mouse Button Right
    VirtualMouseButtonMap.insert("Mouse-M",             V_MOUSECODE(MOUSEEVENTF_MIDDLEDOWN,     MOUSEEVENTF_MIDDLEUP,   0           )); // Mouse Button Middle
    VirtualMouseButtonMap.insert("Mouse-X1",            V_MOUSECODE(MOUSEEVENTF_XDOWN,          MOUSEEVENTF_XUP,        XBUTTON1    )); // Mouse Button X1
    VirtualMouseButtonMap.insert("Mouse-X2",            V_MOUSECODE(MOUSEEVENTF_XDOWN,          MOUSEEVENTF_XUP,        XBUTTON2    )); // Mouse Button X2
    VirtualMouseButtonMap.insert(MOUSE_STR_WHEEL_UP,    V_MOUSECODE(MOUSEEVENTF_WHEEL,          MOUSEEVENTF_WHEEL,      0           )); // Mouse Wheel Up
    VirtualMouseButtonMap.insert(MOUSE_STR_WHEEL_DOWN,  V_MOUSECODE(MOUSEEVENTF_WHEEL,          MOUSEEVENTF_WHEEL,      0           )); // Mouse Wheel Down

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
void QKeyMapper_Worker::initJoystickKeyMap()
{
    /* Joystick 2Mouse */
    JoyStickKeyMap.insert(JOY_LS2MOUSE_STR                ,   (int)JOYSTICK_LS_MOUSE          );
    JoyStickKeyMap.insert(JOY_RS2MOUSE_STR                ,   (int)JOYSTICK_RS_MOUSE          );
    /* Joystick Buttons */
    JoyStickKeyMap.insert("Joy-Key1(A/×)"                 ,   (int)JOYSTICK_BUTTON_0          );
    JoyStickKeyMap.insert("Joy-Key2(B/○)"                 ,   (int)JOYSTICK_BUTTON_1          );
    JoyStickKeyMap.insert("Joy-Key3(X/□)"                 ,   (int)JOYSTICK_BUTTON_2          );
    JoyStickKeyMap.insert("Joy-Key4(Y/△)"                 ,   (int)JOYSTICK_BUTTON_3          );
    JoyStickKeyMap.insert("Joy-Key5(LB)"                  ,   (int)JOYSTICK_BUTTON_4          );
    JoyStickKeyMap.insert("Joy-Key6(RB)"                  ,   (int)JOYSTICK_BUTTON_5          );
    JoyStickKeyMap.insert("Joy-Key7(Back)"                ,   (int)JOYSTICK_BUTTON_6          );
    JoyStickKeyMap.insert("Joy-Key8(Start)"               ,   (int)JOYSTICK_BUTTON_7          );
    JoyStickKeyMap.insert("Joy-Key9(LS-Click)"            ,   (int)JOYSTICK_BUTTON_8          );
    JoyStickKeyMap.insert("Joy-Key10(RS-Click)"           ,   (int)JOYSTICK_BUTTON_9          );
    JoyStickKeyMap.insert("Joy-Key11(LT)"                 ,   (int)JOYSTICK_BUTTON_10         );
    JoyStickKeyMap.insert("Joy-Key12(RT)"                 ,   (int)JOYSTICK_BUTTON_11         );
    /* Joystick DPad Direction */
    JoyStickKeyMap.insert("Joy-DPad-Up"                   ,   (int)JOYSTICK_DPAD_UP           );
    JoyStickKeyMap.insert("Joy-DPad-Down"                 ,   (int)JOYSTICK_DPAD_DOWN         );
    JoyStickKeyMap.insert("Joy-DPad-Left"                 ,   (int)JOYSTICK_DPAD_LEFT         );
    JoyStickKeyMap.insert("Joy-DPad-Right"                ,   (int)JOYSTICK_DPAD_RIGHT        );
    /* Joystick Left-Stick Direction */
    JoyStickKeyMap.insert("Joy-LS-Up"                     ,   (int)JOYSTICK_LS_UP             );
    JoyStickKeyMap.insert("Joy-LS-Down"                   ,   (int)JOYSTICK_LS_DOWN           );
    JoyStickKeyMap.insert("Joy-LS-Left"                   ,   (int)JOYSTICK_LS_LEFT           );
    JoyStickKeyMap.insert("Joy-LS-Right"                  ,   (int)JOYSTICK_LS_RIGHT          );
    /* Joystick Right-Stick Direction */
    JoyStickKeyMap.insert("Joy-RS-Up"                     ,   (int)JOYSTICK_RS_UP             );
    JoyStickKeyMap.insert("Joy-RS-Down"                   ,   (int)JOYSTICK_RS_DOWN           );
    JoyStickKeyMap.insert("Joy-RS-Left"                   ,   (int)JOYSTICK_RS_LEFT           );
    JoyStickKeyMap.insert("Joy-RS-Right"                  ,   (int)JOYSTICK_RS_RIGHT          );

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
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_10,      "Joy-Key11(LT)"                 );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_11,      "Joy-Key12(RT)"                 );

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

void QKeyMapper_Worker::initSkipReleaseModifiersKeysList()
{
    skipReleaseModifiersKeysList = QStringList() \
        << "Vol Mute"
        << "Vol Down"
        << "Vol Up"
        ;
}

#ifdef VIGEM_CLIENT_SUPPORT
void QKeyMapper_Worker::initViGEmKeyMap()
{
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
    JoyStickKeyMap.insert("vJoy-Key11(LT)"              ,   (int)JOYSTICK_BUTTON_10     );
    JoyStickKeyMap.insert("vJoy-Key12(RT)"              ,   (int)JOYSTICK_BUTTON_11     );
    /* Virtual Joystick Special Buttons for ForzaHorizon */
    JoyStickKeyMap.insert("vJoy-Key11(LT)_BRAKE"        ,   (int)JOYSTICK_BUTTON_10     );
    JoyStickKeyMap.insert("vJoy-Key11(LT)_ACCEL"        ,   (int)JOYSTICK_BUTTON_10     );
    JoyStickKeyMap.insert("vJoy-Key12(RT)_BRAKE"        ,   (int)JOYSTICK_BUTTON_11     );
    JoyStickKeyMap.insert("vJoy-Key12(RT)_ACCEL"        ,   (int)JOYSTICK_BUTTON_11     );

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

void QKeyMapper_Worker::clearAllBurstTimersAndLockKeys()
{
    QList<QString> burstKeys = m_BurstTimerMap.keys();
    for (const QString &key : qAsConst(burstKeys)) {
        int timerID = m_BurstTimerMap.value(key, 0);
        if (timerID > 0) {
            int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(key);
            if (findindex >= 0) {
                if (true == QKeyMapper::KeyMappingDataList.at(findindex).Lock) {
                    if (true == pressedLockKeysList.contains(key)){
                        QKeyMapper::KeyMappingDataList[findindex].LockStatus = false;
                        pressedLockKeysList.removeAll(key);
#ifdef DEBUG_LOGOUT_ON
                        qDebug("clearAllBurstTimersAndLockKeys() : Key \"%s\" KeyDown LockStatus -> OFF", key.toStdString().c_str());
#endif
                    }
                }

                stopBurstTimer(key, findindex);
            }
        }
    }

    QList<QString> burstKeyUpKeys = m_BurstKeyUpTimerMap.keys();
    for (const QString &key : qAsConst(burstKeyUpKeys)) {
        int timerID = m_BurstKeyUpTimerMap.value(key, 0);
        if (timerID > 0) {
            killTimer(timerID);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("[clearAllBurstTimersAndLockKeys] Key \"%s\" could not find in m_BurstKeyUpTimerMap!!!", key.toStdString().c_str());
#endif
        }
    }

    for (int index = 0; index < QKeyMapper::KeyMappingDataList.size(); index++) {
        QKeyMapper::KeyMappingDataList[index].LockStatus = false;
    }
}

void QKeyMapper_Worker::collectExchangeKeysList()
{
    exchangeKeysList.clear();

    QHash<QString, QString> singlemapping_keymap;
    for (const MAP_KEYDATA &keymapdata : qAsConst(QKeyMapper::KeyMappingDataList))
    {
        if (keymapdata.Mapping_Keys.size() == 1)
        {
            singlemapping_keymap.insert(keymapdata.Original_Key, keymapdata.Mapping_Keys.constFirst());
        }
    }

    if (false == singlemapping_keymap.isEmpty())
    {
        QList<QString> singlemappingKeys = singlemapping_keymap.keys();
        for (const QString &key : qAsConst(singlemappingKeys))
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
    bool result = false;

    QList<QStringList> remainPressedMappingKeys = pressedMappingKeysMap.values();

    for (const QStringList &mappingkeys : qAsConst(remainPressedMappingKeys)){
        for (const QString &mapkey : qAsConst(mappingkeys)){
            if (mapkey == key) {
                result = true;
                break;
            }
        }
    }

    return result;
}

#if 0
int QKeyMapper_Worker::makeKeySequenceInputarray(QStringList &keyseq_list, INPUT *input_array)
{
    int index = 0;
    int keycount = 0;
    INPUT *input_p = Q_NULLPTR;

    for (const QString &keyseq : qAsConst(keyseq_list)){
        QStringList mappingKeys = keyseq.split(SEPARATOR_PLUS);
        for (const QString &key : qAsConst(mappingKeys)){
            if (key == MOUSE_STR_WHEEL_UP || key == MOUSE_STR_WHEEL_DOWN) {
                input_p = &input_array[index];
                input_p->type = INPUT_MOUSE;
                input_p->mi.dwExtraInfo = VIRTUAL_MOUSE_WHEEL;
                input_p->mi.dwFlags = MOUSEEVENTF_WHEEL;
                if (key == MOUSE_STR_WHEEL_UP) {
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
                input_p->mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
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
                input_p->ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
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
            if (key == MOUSE_STR_WHEEL_UP || key == MOUSE_STR_WHEEL_DOWN) {
                continue;
            }
            else if (true == VirtualMouseButtonMap.contains(key)) {
                V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);

                input_p = &input_array[index];
                input_p->type = INPUT_MOUSE;
                input_p->mi.mouseData = vmousecode.MouseXButton;
                input_p->mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
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
                input_p->ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
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

void QKeyMapper_Worker::sendKeySequenceList(QStringList &keyseq_list, QString &original_key)
{
    int index = 1;
    for (const QString &keyseq : qAsConst(keyseq_list)){
        QString original_key_forKeySeq = original_key + ":" + KEYSEQUENCE_STR + QString::number(index);
        QStringList mappingKeyList = QStringList() << keyseq;

        emit sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key_forKeySeq, SENDMODE_NORMAL);
        emit sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key_forKeySeq, SENDMODE_NORMAL);

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
        s_LowLevelMouseHook_Enable = false;
#ifdef DEBUG_LOGOUT_ON
        qDebug("QKeyMapper_Hook_Proc() Win_Dbg = TRUE, set QKeyMapper_Hook_Proc::s_LowLevelMouseHook_Enable to FALSE");
#endif
    }
#endif

#ifdef HOOKSTART_ONSTARTUP
    if (s_LowLevelKeyboardHook_Enable) {
        if (s_KeyHook == Q_NULLPTR) {
            s_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper_Worker::LowLevelKeyboardHookProc, GetModuleHandle(Q_NULLPTR), 0);
#ifdef DEBUG_LOGOUT_ON
            if (s_KeyHook != Q_NULLPTR) {
                qDebug("[SetHookProc] QKeyMapper_Hook_Proc() Keyboard SetWindowsHookEx Success. -> 0x%08X", s_KeyHook);
            }
#endif
        }
    }
    if (s_LowLevelMouseHook_Enable) {
        if (s_MouseHook == Q_NULLPTR) {
            s_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, QKeyMapper_Worker::LowLevelMouseHookProc, GetModuleHandle(Q_NULLPTR), 0);
#ifdef DEBUG_LOGOUT_ON
            if (s_MouseHook != Q_NULLPTR) {
                qDebug("[SetHookProc] QKeyMapper_Hook_Proc() Mouse SetWindowsHookEx Success. -> 0x%08X", s_MouseHook);
            }
#endif
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qInfo("[SetHookProc] QKeyMapper_Hook_Proc() Keyboard Hook & Mouse Hook Started.");
#endif
#endif
}

QKeyMapper_Hook_Proc::~QKeyMapper_Hook_Proc()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "~QKeyMapper_Hook_Proc() Called.";
#endif

#ifdef HOOKSTART_ONSTARTUP
    bool unhook_ret = 0;
    if (s_KeyHook != Q_NULLPTR){
        void* keyboardhook_p = (void*)s_KeyHook;
        unhook_ret = UnhookWindowsHookEx(s_KeyHook);
        s_KeyHook = Q_NULLPTR;
        Q_UNUSED(keyboardhook_p);

#ifdef DEBUG_LOGOUT_ON
        if (0 == unhook_ret) {
            qDebug() << "[SetHookProc]" << "~QKeyMapper_Hook_Proc() Keyboard UnhookWindowsHookEx Failure! LastError:" << GetLastError();
        }
        else {
            qDebug("[SetHookProc] ~QKeyMapper_Hook_Proc() Keyboard UnhookWindowsHookEx Success. -> 0x%08X", keyboardhook_p);
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
            qDebug() << "[SetHookProc]" << "~QKeyMapper_Hook_Proc() Mouse UnhookWindowsHookEx Failure! LastError:" << GetLastError();
        }
        else {
            qDebug("[SetHookProc] ~QKeyMapper_Hook_Proc() Mouse UnhookWindowsHookEx Success. -> 0x%08X", mousehook_p);
        }
#endif
    }
    Q_UNUSED(unhook_ret);

#ifdef DEBUG_LOGOUT_ON
    qInfo("[SetHookProc] ~QKeyMapper_Hook_Proc() Keyboard Hook & Mouse Hook Stopped.");
#endif
#endif
}

#ifndef HOOKSTART_ONSTARTUP
void QKeyMapper_Hook_Proc::onSetHookProcKeyHook(HWND hWnd)
{
    Q_UNUSED(hWnd);

    if (s_LowLevelKeyboardHook_Enable) {
        if (s_KeyHook == Q_NULLPTR) {
            s_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper_Worker::LowLevelKeyboardHookProc, GetModuleHandle(Q_NULLPTR), 0);
#ifdef DEBUG_LOGOUT_ON
            if (s_KeyHook != Q_NULLPTR) {
                qDebug("[SetHookProc] Keyboard SetWindowsHookEx Success. -> 0x%08X", s_KeyHook);
            }
#endif
        }
    }
    if (s_LowLevelMouseHook_Enable) {
        if (s_MouseHook == Q_NULLPTR) {
            s_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, QKeyMapper_Worker::LowLevelMouseHookProc, GetModuleHandle(Q_NULLPTR), 0);
#ifdef DEBUG_LOGOUT_ON
            if (s_MouseHook != Q_NULLPTR) {
                qDebug("[SetHookProc] Mouse SetWindowsHookEx Success. -> 0x%08X", s_MouseHook);
            }
#endif
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qInfo("[SetHookProc] Keyboard Hook & Mouse Hook Started.");
#endif
}

void QKeyMapper_Hook_Proc::onSetHookProcKeyUnHook()
{
    bool unhook_ret = 0;

    if (s_KeyHook != Q_NULLPTR){
        void* keyboardhook_p = (void*)s_KeyHook;
        unhook_ret = UnhookWindowsHookEx(s_KeyHook);
        s_KeyHook = Q_NULLPTR;
        Q_UNUSED(keyboardhook_p);

#ifdef DEBUG_LOGOUT_ON
        if (0 == unhook_ret) {
            qDebug() << "[SetHookProc]" << "Keyboard UnhookWindowsHookEx Failure! LastError:" << GetLastError();
        }
        else {
            qDebug("[SetHookProc] Keyboard UnhookWindowsHookEx Success. -> 0x%08X", keyboardhook_p);
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
            qDebug() << "[SetHookProc]" << "Mouse UnhookWindowsHookEx Failure! LastError:" << GetLastError();
        }
        else {
            qDebug("[SetHookProc] Mouse UnhookWindowsHookEx Success. -> 0x%08X", mousehook_p);
        }
#endif
    }

    Q_UNUSED(unhook_ret);

#ifdef DEBUG_LOGOUT_ON
    qInfo("[SetHookProc] Keyboard Hook & Mouse Hook Stopped.");
#endif
}
#endif
