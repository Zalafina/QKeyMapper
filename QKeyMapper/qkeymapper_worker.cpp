#include "qkeymapper.h"
#include "qkeymapper_worker.h"

static const int KEY_INIT = -1;
static const int KEY_UP = 0;
static const int KEY_DOWN = 1;

static const int MOUSE_WHEEL_UP = 1;
static const int MOUSE_WHEEL_DOWN = 2;

static const int MOUSE_WHEEL_KEYUP_WAITTIME = 20;

static const int MAPPING_WAITTIME_MIN = 0;
static const int MAPPING_WAITTIME_MAX = 1000;

static const WORD XBUTTON_NONE = 0x0000;

static const int SENDMODE_HOOK          = 0;
static const int SENDMODE_BURST_NORMAL  = 1;
static const int SENDMODE_BURST_STOP    = 2;

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

static const int MOUSE_CURSOR_BOTTOMRIGHT_X = 65535;
static const int MOUSE_CURSOR_BOTTOMRIGHT_Y = 65535;

#ifdef VIGEM_CLIENT_SUPPORT
static const BYTE XINPUT_TRIGGER_MIN     = 0;
static const BYTE XINPUT_TRIGGER_MAX     = 255;

static const SHORT XINPUT_THUMB_MIN     = -32768;
static const SHORT XINPUT_THUMB_RELEASE = 0;
static const SHORT XINPUT_THUMB_MAX     = 32767;

static const int VIRTUAL_JOYSTICK_SENSITIVITY_MIN = 1;
static const int VIRTUAL_JOYSTICK_SENSITIVITY_MAX = 1000;
static const int VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT = 12;

static const int MOUSE2VJOY_RESET_TIMEOUT = 200;
#endif

static const char *VJOY_STR_MOUSE2LS = "vJoy-Mouse2LS";
static const char *VJOY_STR_MOUSE2RS = "vJoy-Mouse2RS";


static const char *MOUSE_STR_WHEEL_UP = "Mouse-WheelUp";
static const char *MOUSE_STR_WHEEL_DOWN = "Mouse-WheelDown";

static const ULONG_PTR VIRTUAL_KEYBOARD_PRESS = 0xACBDACBD;
static const ULONG_PTR VIRTUAL_MOUSE_CLICK = 0xCEDFCEDF;
static const ULONG_PTR VIRTUAL_MOUSE_MOVE = 0xBFBCBFBC;
static const ULONG_PTR VIRTUAL_MOUSE_WHEEL = 0xEBFAEBFA;
static const ULONG_PTR VIRTUAL_WIN_PLUS_D = 0xDBDBDBDB;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
QMultiHash<QString, V_KEYCODE> QKeyMapper_Worker::VirtualKeyCodeMap = QMultiHash<QString, V_KEYCODE>();
#else
QHash<QString, V_KEYCODE> QKeyMapper_Worker::VirtualKeyCodeMap = QHash<QString, V_KEYCODE>();
#endif
QHash<QString, V_MOUSECODE> QKeyMapper_Worker::VirtualMouseButtonMap = QHash<QString, V_MOUSECODE>();
QHash<WPARAM, QString> QKeyMapper_Worker::MouseButtonNameMap = QHash<WPARAM, QString>();
QHash<QString, QString> QKeyMapper_Worker::MouseButtonNameConvertMap = QHash<QString, QString>();
QHash<QString, int> QKeyMapper_Worker::JoyStickKeyMap = QHash<QString, int>();
#ifdef VIGEM_CLIENT_SUPPORT
QHash<QString, XUSB_BUTTON> QKeyMapper_Worker::ViGEmButtonMap = QHash<QString, XUSB_BUTTON>();
#endif
QStringList QKeyMapper_Worker::pressedRealKeysList = QStringList();
QStringList QKeyMapper_Worker::pressedVirtualKeysList = QStringList();
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

QKeyMapper_Worker::QKeyMapper_Worker(QObject *parent) :
    m_KeyHook(Q_NULLPTR),
    m_MouseHook(Q_NULLPTR),
    m_JoystickCapture(false),
#ifdef QT_DEBUG
    m_LowLevelKeyboardHook_Enable(true),
    m_LowLevelMouseHook_Enable(true),
#endif
#ifdef DINPUT_TEST
    m_DirectInput(Q_NULLPTR),
#endif
#ifdef VIGEM_CLIENT_SUPPORT
    m_Mouse2vJoyResetTimer(this),
#endif
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

    Q_UNUSED(parent);

    QObject::connect(this, SIGNAL(setKeyHook_Signal(HWND)), this, SLOT(setWorkerKeyHook(HWND)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(setKeyUnHook_Signal()), this, SLOT(setWorkerKeyUnHook()), Qt::QueuedConnection);

    QObject::connect(this, SIGNAL(startBurstTimer_Signal(QString,int)), this, SLOT(startBurstTimer(QString,int)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(stopBurstTimer_Signal(QString,int)), this, SLOT(stopBurstTimer(QString,int)), Qt::QueuedConnection);

#if 0
    QObject::connect(this, SIGNAL(sendKeyboardInput_Signal(V_KEYCODE,int)), this, SLOT(sendKeyboardInput(V_KEYCODE,int)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(sendMouseClick_Signal(V_MOUSECODE,int)), this, SLOT(sendMouseClick(V_MOUSECODE,int)), Qt::QueuedConnection);
#endif
    QObject::connect(this, SIGNAL(sendInputKeys_Signal(QStringList,int,QString,int)), this, SLOT(sendInputKeys(QStringList,int,QString,int)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(send_WINplusD_Signal()), this, SLOT(send_WINplusD()), Qt::QueuedConnection);
#if 0
    QObject::connect(this, SIGNAL(onMouseWheel_Signal(int)), this, SLOT(onMouseWheel(int)), Qt::QueuedConnection);
#endif
#ifdef VIGEM_CLIENT_SUPPORT
    QObject::connect(this, SIGNAL(onMouseMove_Signal(int,int)), this, SLOT(onMouseMove(int,int)), Qt::QueuedConnection);

    m_Mouse2vJoyResetTimer.setTimerType(Qt::PreciseTimer);
    m_Mouse2vJoyResetTimer.setSingleShot(true);
    QObject::connect(&m_Mouse2vJoyResetTimer, SIGNAL(timeout()), this, SLOT(onMouse2vJoyResetTimeout()));
#endif

    /* Connect QJoysticks Signals */
    QJoysticks *instance = QJoysticks::getInstance();
    QObject::connect(instance, &QJoysticks::POVEvent, this, &QKeyMapper_Worker::onJoystickPOVEvent);
    QObject::connect(instance, &QJoysticks::axisEvent, this, &QKeyMapper_Worker::onJoystickAxisEvent);
    QObject::connect(instance, &QJoysticks::buttonEvent, this, &QKeyMapper_Worker::onJoystickButtonEvent);

    initVirtualKeyCodeMap();
    initVirtualMouseButtonMap();
    initJoystickKeyMap();

#ifdef VIGEM_CLIENT_SUPPORT
    initViGEmKeyMap();
#endif

#ifdef QT_DEBUG
    if (IsDebuggerPresent()) {
        m_LowLevelMouseHook_Enable = false;
#ifdef DEBUG_LOGOUT_ON
        qDebug("QKeyMapper_Worker() Win_Dbg = TRUE, set m_LowLevelMouseHook_Enable to FALSE");
#endif
    }
#endif
}

QKeyMapper_Worker::~QKeyMapper_Worker()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "~QKeyMapper_Worker() called.";
#endif

    setWorkerKeyUnHook();

#ifdef VIGEM_CLIENT_SUPPORT
    (void)ViGEmClient_Remove();
    (void)ViGEmClient_Disconnect();
    (void)ViGEmClient_Free();
#endif
}

void QKeyMapper_Worker::sendKeyboardInput(V_KEYCODE vkeycode, int keyupdown)
{
    QMutexLocker locker(&sendinput_mutex);

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
    QMutexLocker locker(&sendinput_mutex);

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

void QKeyMapper_Worker::sendMouseMove(int x, int y)
{
    QMutexLocker locker(&sendinput_mutex);

    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = x * (65535 / GetSystemMetrics(SM_CXSCREEN)); // x being coordinate in pixels
    mouse_input.mi.dy = y * (65535 / GetSystemMetrics(SM_CYSCREEN)); // y being coordinate in pixels
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
    QMutexLocker locker(&sendinput_mutex);

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
    QMutexLocker locker(&sendinput_mutex);

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
    mouse_input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

    // Send the mouse_input event
    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("setMouseToScreenCenter(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::setMouseToScreenBottomRight()
{
    QMutexLocker locker(&sendinput_mutex);

    // Initialize INPUT structure
    INPUT mouse_input = { 0 };
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = MOUSE_CURSOR_BOTTOMRIGHT_X;
    mouse_input.mi.dy = MOUSE_CURSOR_BOTTOMRIGHT_Y;
    mouse_input.mi.mouseData = 0;
    mouse_input.mi.time = 0;
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_MOVE;
    mouse_input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

    // Send the mouse_input event
    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("setMouseToScreenBottomRight(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

#ifdef VIGEM_CLIENT_SUPPORT
void QKeyMapper_Worker::onMouseMove(int x, int y)
{
//#ifdef DEBUG_LOGOUT_ON
//    qDebug() << "[onMouseMove]" << "Mouse Move -> Delta X =" << s_Mouse2vJoy_delta.x() << ", Delta Y = " << s_Mouse2vJoy_delta.y();
//#endif

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

void QKeyMapper_Worker::sendInputKeys(QStringList inputKeys, int keyupdown, QString original_key, int sendmode)
{
    Q_UNUSED(sendmode);

    if (original_key == MOUSE_STR_WHEEL_UP || original_key == MOUSE_STR_WHEEL_DOWN) {
        if (KEY_UP == keyupdown) {
            QThread::msleep(MOUSE_WHEEL_KEYUP_WAITTIME);
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

    QMutexLocker locker(&sendinput_mutex);

    int keycount = 0;
    INPUT inputs[SEND_INPUTS_MAX] = { 0 };

    if (KEY_UP == keyupdown) {
        if (key_sequence_count > 1) {
            return;
        }

        QStringList mappingKeys = inputKeys.constFirst().split(SEPARATOR_PLUS);
        keycount = mappingKeys.size();

        if (keycount >= SEND_INPUTS_MAX) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("sendInputKeys(): Too many keys(%d) to mapping!!!", keycount);
#endif
            return;
        }

        pressedMappingKeysMap.remove(original_key);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys KeyUp -> original_key[ " << original_key << " ], " << "mappingKeys[ " << mappingKeys << " ]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
#endif

        for(auto it = mappingKeys.crbegin(); it != mappingKeys.crend(); ++it) {
            QString key = (*it);

            int waitTime = 0;
            if (key.contains(SEPARATOR_WAITTIME)) {
                QStringList waitTimeKeyList = key.split(SEPARATOR_WAITTIME);
                waitTime = waitTimeKeyList.first().toInt();
                key = waitTimeKeyList.last();
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
            if (SENDMODE_HOOK == sendmode) {
                if ((original_key == key) && (keycount == 1)) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug("Mapping the same key, do not skip send mapping VirtualKey \"%s\" KEYUP!", key.toStdString().c_str());
#endif
                }
                else {
                }
            }
            else if (SENDMODE_BURST_STOP == sendmode) {
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
                if (false == pressedVirtualKeysList.contains(key)) {
                    qWarning("sendInputKeys(): Key Up -> \"%s\" do not exist!!!", key.toStdString().c_str());
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
//                qDebug("sendInputKeys(): Key Up -> \"%s\", wScan->0x%08X", key.toStdString().c_str(), input.ki.wScan);
//#endif
                if (KEY_DOWN == keyupdown) {
                    input.ki.dwFlags = extenedkeyflag | 0;
                }
                else {
                    input.ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                }
                SendInput(1, &input, sizeof(INPUT));
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("sendInputKeys(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
            }

            if (MAPPING_WAITTIME_MIN < waitTime && waitTime <= MAPPING_WAITTIME_MAX) {
                QThread::msleep(waitTime);
            }
        }
    }
    else {
        if (1 == key_sequence_count) {
            QStringList mappingKeys = inputKeys.constFirst().split(SEPARATOR_PLUS);
            keycount = mappingKeys.size();

            if (keycount >= SEND_INPUTS_MAX) {
#ifdef DEBUG_LOGOUT_ON
                qWarning("sendInputKeys(): Too many keys(%d) to mapping!!!", keycount);
#endif
                return;
            }

            pressedMappingKeysMap.insert(original_key, mappingKeys);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[sendInputKeys] pressedMappingKeys KeyDown -> original_key[" << original_key << "], " << "mappingKeys[" << mappingKeys << "]" << " : pressedMappingKeysMap -> " << pressedMappingKeysMap;
#endif

            for (const QString &keyStr : qAsConst(mappingKeys)){
                QString key = keyStr;
                int waitTime = 0;
                if (key.contains(SEPARATOR_WAITTIME)) {
                    QStringList waitTimeKeyList = key.split(SEPARATOR_WAITTIME);
                    waitTime = waitTimeKeyList.first().toInt();
                    key = waitTimeKeyList.last();
                    if (MAPPING_WAITTIME_MIN < waitTime && waitTime <= MAPPING_WAITTIME_MAX) {
                        QThread::msleep(waitTime);
                    }
                }

                if (key == MOUSE_STR_WHEEL_UP || key == MOUSE_STR_WHEEL_DOWN) {
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
                    ViGEmClient_PressButton(key);
                }
#endif
                else if (true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(key)) {
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
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qWarning("sendInputKeys(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
                }
            }
        }
        /* key_sequence_count > 1 */
        else {
            keycount = makeKeySequenceInputarray(inputKeys, inputs);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[sendInputKeys] " << "Key Sequence [" << inputKeys << "]," << "keycount =" << keycount;
#endif
        }
    }

#if 0
    if (keycount > 0) {
        UINT uSent = SendInput(keycount, inputs, sizeof(INPUT));
        if (uSent != keycount) {
#ifdef DEBUG_LOGOUT_ON
            qDebug("sendInputKeys(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
        }
    }
#endif
}

void QKeyMapper_Worker::send_WINplusD()
{
    QMutexLocker locker(&sendinput_mutex);
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
    int findindex = QKeyMapper::findInKeyMappingDataList(burstKey);

    if (findindex >=0){
        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
        QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
        sendInputKeys(mappingKeyList, KEY_DOWN, original_key, SENDMODE_BURST_NORMAL);
    }
}

void QKeyMapper_Worker::sendBurstKeyUp(const QString &burstKey, bool stop)
{
    int findindex = QKeyMapper::findInKeyMappingDataList(burstKey);

    if (findindex >=0){
        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
        QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
        int sendmode = SENDMODE_BURST_NORMAL;
        if (true == stop) {
            sendmode = SENDMODE_BURST_STOP;
        }
        sendInputKeys(mappingKeyList, KEY_UP, original_key, sendmode);
    }
}

void QKeyMapper_Worker::sendSpecialVirtualKeyDown(const QString &virtualKey)
{
    if (true == VirtualMouseButtonMap.contains(virtualKey)) {
        V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(virtualKey);
        sendMouseClick(vmousecode, KEY_DOWN);
    }
    else {
        V_KEYCODE map_vkeycode = QKeyMapper_Worker::VirtualKeyCodeMap.value(virtualKey);
        sendKeyboardInput(map_vkeycode, KEY_DOWN);
    }
}

void QKeyMapper_Worker::sendSpecialVirtualKeyUp(const QString &virtualKey)
{
    if (true == VirtualMouseButtonMap.contains(virtualKey)) {
        V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(virtualKey);
        sendMouseClick(vmousecode, KEY_UP);
    }
    else {
        V_KEYCODE map_vkeycode = QKeyMapper_Worker::VirtualKeyCodeMap.value(virtualKey);
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
            emit QKeyMapper::getInstance()->updateViGEmBusStatus_Signal();
#ifdef DEBUG_LOGOUT_ON
            qWarning("[ViGEmClient_Connect] ViGEm Bus connection failed with error code: 0x%08X", retval);
#endif
            return -1;
        }
    }
    else {
        return -1;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug("[ViGEmClient_Connect] ViGEmClient Connect() Success. -> [0x%08X]", s_ViGEmClient);
#endif

    s_ViGEmClient_ConnectState = VIGEMCLIENT_CONNECT_SUCCESS;
    emit QKeyMapper::getInstance()->updateViGEmBusStatus_Signal();

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

    s_ViGEmTarget = vigem_target_x360_alloc();

    if (s_ViGEmTarget == Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("[ViGEmClient_Add] ViGEmTarget Alloc failed with NULLPTR!!!");
#endif
        return -1;
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
    ULONG user_index = 255;
    VIGEM_ERROR error;
    Q_UNUSED(index);
    Q_UNUSED(error);
    if (s_ViGEmTarget != Q_NULLPTR) {
        index = vigem_target_get_index(s_ViGEmTarget);
        error = vigem_target_x360_get_user_index(s_ViGEmClient, s_ViGEmTarget, &user_index);
        if (error == VIGEM_ERROR_NONE) {
#ifdef DEBUG_LOGOUT_ON
            qDebug("[ViGEmClient_Add] ViGEmTarget Add Success, index(%lu), user_index(%lu). -> [0x%08X]", index, user_index, s_ViGEmTarget);
#endif
        }
        else {
            return -1;
        }
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
            VIGEM_ERROR error = vigem_target_x360_update(s_ViGEmClient, s_ViGEmTarget, s_ViGEmTarget_Report);

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
                qWarning("[ViGEmClient_Remove] ViGEmTarget Reset Buttons failed!!!, Error=0x%08X -> [0x%08X]", error, s_ViGEmTarget);
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
    emit QKeyMapper::getInstance()->updateViGEmBusStatus_Signal();
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
    emit QKeyMapper::getInstance()->updateViGEmBusStatus_Signal();
}

QKeyMapper_Worker::ViGEmClient_ConnectState QKeyMapper_Worker::ViGEmClient_getConnectState()
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    return s_ViGEmClient_ConnectState;
}

void QKeyMapper_Worker::ViGEmClient_PressButton(const QString &joystickButton)
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient != Q_NULLPTR && s_ViGEmTarget != Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().noquote().nospace() << "[ViGEmClient]" << " Joystick Button Press [" << joystickButton << "]";
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

        bool updateFlag = false;
        if (ViGEmButtonMap.contains(joystickButton)) {
            XUSB_BUTTON button = ViGEmButtonMap.value(joystickButton);

            s_ViGEmTarget_Report.wButtons = s_ViGEmTarget_Report.wButtons | button;
            updateFlag = true;
        }
        else if (joystickButton == "vJoy-Key11(LT)" || joystickButton == "vJoy-Key12(RT)") {
            if (joystickButton == "vJoy-Key11(LT)") {
                s_ViGEmTarget_Report.bLeftTrigger = XINPUT_TRIGGER_MAX;
            }
            else {
                s_ViGEmTarget_Report.bRightTrigger = XINPUT_TRIGGER_MAX;
            }
            updateFlag = true;
        }
        else if (joystickButton.contains("vJoy-LS-")) {
            if (joystickButton == "vJoy-LS-Up") {
                s_ViGEmTarget_Report.sThumbLY = XINPUT_THUMB_MAX;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-LS-Down") {
                s_ViGEmTarget_Report.sThumbLY = XINPUT_THUMB_MIN;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-LS-Left") {
                s_ViGEmTarget_Report.sThumbLX = XINPUT_THUMB_MIN;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-LS-Right") {
                s_ViGEmTarget_Report.sThumbLX = XINPUT_THUMB_MAX;
                updateFlag = true;
            }
        }
        else if (joystickButton.contains("vJoy-RS-")) {
            if (joystickButton == "vJoy-RS-Up") {
                s_ViGEmTarget_Report.sThumbRY = XINPUT_THUMB_MAX;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-RS-Down") {
                s_ViGEmTarget_Report.sThumbRY = XINPUT_THUMB_MIN;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-RS-Left") {
                s_ViGEmTarget_Report.sThumbRX = XINPUT_THUMB_MIN;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-RS-Right") {
                s_ViGEmTarget_Report.sThumbRX = XINPUT_THUMB_MAX;
                updateFlag = true;
            }
        }

        if (updateFlag) {
            VIGEM_ERROR error;
            error = vigem_target_x360_update(s_ViGEmClient, s_ViGEmTarget, s_ViGEmTarget_Report);
            Q_UNUSED(error);
#ifdef DEBUG_LOGOUT_ON
            if (error != VIGEM_ERROR_NONE) {
                qDebug("[ViGEmClient] Button Press Return code: 0x%08X", error);
            }
#endif
        }
    }
}

void QKeyMapper_Worker::ViGEmClient_ReleaseButton(const QString &joystickButton)
{
    QMutexLocker locker(&s_ViGEmClient_Mutex);

    if (s_ViGEmClient != Q_NULLPTR && s_ViGEmTarget != Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().noquote().nospace() << "[ViGEmClient]" << " Joystick Button Release [" << joystickButton << "]";
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

        bool updateFlag = false;
        if (ViGEmButtonMap.contains(joystickButton)) {
            XUSB_BUTTON button = ViGEmButtonMap.value(joystickButton);

            s_ViGEmTarget_Report.wButtons = s_ViGEmTarget_Report.wButtons & ~button;
            updateFlag = true;
        }
        else if (joystickButton == "vJoy-Key11(LT)" || joystickButton == "vJoy-Key12(RT)") {
            if (joystickButton == "vJoy-Key11(LT)") {
                s_ViGEmTarget_Report.bLeftTrigger = XINPUT_TRIGGER_MIN;
            }
            else {
                s_ViGEmTarget_Report.bRightTrigger = XINPUT_TRIGGER_MIN;
            }
            updateFlag = true;
        }
        else if (joystickButton.contains("vJoy-LS-")) {
            if (joystickButton == "vJoy-LS-Up") {
                s_ViGEmTarget_Report.sThumbLY = XINPUT_THUMB_RELEASE;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-LS-Down") {
                s_ViGEmTarget_Report.sThumbLY = XINPUT_THUMB_RELEASE;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-LS-Left") {
                s_ViGEmTarget_Report.sThumbLX = XINPUT_THUMB_RELEASE;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-LS-Right") {
                s_ViGEmTarget_Report.sThumbLX = XINPUT_THUMB_RELEASE;
                updateFlag = true;
            }
        }
        else if (joystickButton.contains("vJoy-RS-")) {
            if (joystickButton == "vJoy-RS-Up") {
                s_ViGEmTarget_Report.sThumbRY = XINPUT_THUMB_RELEASE;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-RS-Down") {
                s_ViGEmTarget_Report.sThumbRY = XINPUT_THUMB_RELEASE;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-RS-Left") {
                s_ViGEmTarget_Report.sThumbRX = XINPUT_THUMB_RELEASE;
                updateFlag = true;
            }
            else if (joystickButton == "vJoy-RS-Right") {
                s_ViGEmTarget_Report.sThumbRX = XINPUT_THUMB_RELEASE;
                updateFlag = true;
            }
        }

        if (updateFlag) {
            VIGEM_ERROR error;
            error = vigem_target_x360_update(s_ViGEmClient, s_ViGEmTarget, s_ViGEmTarget_Report);
            Q_UNUSED(error);
#ifdef DEBUG_LOGOUT_ON
            if (error != VIGEM_ERROR_NONE) {
                qDebug("[ViGEmClient] Button Release Return code: 0x%08X", error);
            }
#endif
        }
    }
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

    int findMouse2LSindex = QKeyMapper::findInKeyMappingDataList(VJOY_STR_MOUSE2LS);
    if (findMouse2LSindex >=0){
        leftJoystickUpdate = true;
    }

    int findMouse2RSindex = QKeyMapper::findInKeyMappingDataList(VJOY_STR_MOUSE2RS);
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

    int findMouse2LSindex = QKeyMapper::findInKeyMappingDataList(VJOY_STR_MOUSE2LS);
    if (findMouse2LSindex >=0){
        leftJoystickUpdate = true;
    }

    int findMouse2RSindex = QKeyMapper::findInKeyMappingDataList(VJOY_STR_MOUSE2RS);
    if (findMouse2RSindex >=0){
        rightJoystickUpdate = true;
    }

    if (leftJoystickUpdate || rightJoystickUpdate) {
        int vJoy_X_Sensitivity = QKeyMapper::getvJoyXSensitivity();
        int vJoy_Y_Sensitivity = QKeyMapper::getvJoyYSensitivity();

        // Mouse2Joystick core algorithm from "https://github.com/memethyl/Mouse2Joystick" >>>
        double x = -std::exp((-1.0 / vJoy_X_Sensitivity) * std::abs(delta_x)) + 1.0;
        double y = -std::exp((-1.0 / vJoy_Y_Sensitivity) * std::abs(delta_y)) + 1.0;
        // take the sign into account, expanding the range to (-1, 1)
        x *= sign(delta_x);
        y *= -sign(delta_y);
        // XInput joystick coordinates are signed shorts, so convert to (-32767, 32767)
        short leftX = (short)(32767.0 * x);
        short rightX = leftX;
        short leftY = (short)(32767.0 * y);
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
        error = vigem_target_x360_update(s_ViGEmClient, s_ViGEmTarget, s_ViGEmTarget_Report);
        Q_UNUSED(error);
        m_Mouse2vJoyResetTimer.start(MOUSE2VJOY_RESET_TIMEOUT);
#ifdef DEBUG_LOGOUT_ON
        if (error != VIGEM_ERROR_NONE) {
            qDebug("[ViGEmClient_Mouse2JoystickUpdate] Mouse2Joystick Update ErrorCode: 0x%08X", error);
        }
#endif
    }
}

void QKeyMapper_Worker::ViGEmClient_GamepadReset()
{
    if (s_ViGEmClient_ConnectState != VIGEMCLIENT_CONNECT_SUCCESS) {
        return;
    }

    if (s_ViGEmClient == Q_NULLPTR || s_ViGEmTarget == Q_NULLPTR) {
        return;
    }

    if (vigem_target_is_attached(s_ViGEmTarget) != TRUE) {
        return;
    }

    s_ViGEmTarget_Report.wButtons = 0;
    s_ViGEmTarget_Report.bLeftTrigger = 0;
    s_ViGEmTarget_Report.bRightTrigger = 0;
    s_ViGEmTarget_Report.sThumbLX = 0;
    s_ViGEmTarget_Report.sThumbLY = 0;
    s_ViGEmTarget_Report.sThumbRX = 0;
    s_ViGEmTarget_Report.sThumbRY = 0;

    VIGEM_ERROR error;
    error = vigem_target_x360_update(s_ViGEmClient, s_ViGEmTarget, s_ViGEmTarget_Report);
    Q_UNUSED(error);
#ifdef DEBUG_LOGOUT_ON
    if (error != VIGEM_ERROR_NONE) {
        qDebug("[ViGEmClient_GamepadReset] GamepadReset Update ErrorCode: 0x%08X", error);
    }
#endif
}

void QKeyMapper_Worker::ViGEmClient_JoysticksReset()
{
    if (MOUSE2VJOY_NONE == s_Mouse2vJoy_EnableState) {
        return;
    }

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

    VIGEM_ERROR error;
    error = vigem_target_x360_update(s_ViGEmClient, s_ViGEmTarget, s_ViGEmTarget_Report);
    Q_UNUSED(error);
#ifdef DEBUG_LOGOUT_ON
    if (error != VIGEM_ERROR_NONE) {
        qDebug("[ViGEmClient_GamepadReset] GamepadReset Update ErrorCode: 0x%08X", error);
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
}

void QKeyMapper_Worker::setWorkerKeyHook(HWND hWnd)
{
    clearAllBurstTimersAndLockKeys();
    pressedRealKeysList.clear();
    pressedVirtualKeysList.clear();
    pressedMappingKeysMap.clear();
    m_BurstTimerMap.clear();
    m_BurstKeyUpTimerMap.clear();
    pressedLockKeysList.clear();
    collectExchangeKeysList();

    s_Mouse2vJoy_delta.rx() = 0;
    s_Mouse2vJoy_delta.ry() = 0;
    s_Mouse2vJoy_prev.rx() = 0;
    s_Mouse2vJoy_prev.ry() = 0;
    ViGEmClient_GamepadReset();
    s_Mouse2vJoy_EnableState = ViGEmClient_checkMouse2JoystickEnableState();

    if(TRUE == IsWindowVisible(hWnd)){
        if (QKeyMapper::getLockCursorStatus() && s_Mouse2vJoy_EnableState != MOUSE2VJOY_NONE) {
            setMouseToScreenBottomRight();

            POINT pt;
            if (GetCursorPos(&pt)) {
                s_Mouse2vJoy_prev.rx() = pt.x;
                s_Mouse2vJoy_prev.ry() = pt.y;
#ifdef DEBUG_LOGOUT_ON
                qDebug("[setWorkerKeyHook] Centered Mouse Cursor Positoin -> X = %lu, Y = %lu", pt.x, pt.y);
#endif
            }
        }

#ifdef QT_DEBUG
        if (m_LowLevelKeyboardHook_Enable) {
            m_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper_Worker::LowLevelKeyboardHookProc, GetModuleHandle(Q_NULLPTR), 0);
        }
        if (m_LowLevelMouseHook_Enable) {
            m_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, QKeyMapper_Worker::LowLevelMouseHookProc, GetModuleHandle(Q_NULLPTR), 0);
        }
#else
        m_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper_Worker::LowLevelKeyboardHookProc, GetModuleHandle(Q_NULLPTR), 0);
        m_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, QKeyMapper_Worker::LowLevelMouseHookProc, GetModuleHandle(Q_NULLPTR), 0);
#endif
        setWorkerJoystickCaptureStart(hWnd);

#ifdef DEBUG_LOGOUT_ON
        qInfo("[setWorkerKeyHook] Normal Key Hook & Mouse Hook Started.");
#endif
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qWarning("[setWorkerKeyHook] Error: Invisible Window Handle!!!");
#endif
    }

//    setWorkerDInputKeyHook(hWnd);
}

void QKeyMapper_Worker::setWorkerKeyUnHook()
{
    clearAllBurstTimersAndLockKeys();
    pressedRealKeysList.clear();
    pressedVirtualKeysList.clear();
    pressedMappingKeysMap.clear();
    m_BurstTimerMap.clear();
    m_BurstKeyUpTimerMap.clear();
    pressedLockKeysList.clear();
    exchangeKeysList.clear();

    if (m_MouseHook != Q_NULLPTR) {
        UnhookWindowsHookEx(m_MouseHook);
        m_MouseHook = Q_NULLPTR;
    }

    if (m_KeyHook != Q_NULLPTR){
        UnhookWindowsHookEx(m_KeyHook);
        m_KeyHook = Q_NULLPTR;
#ifdef DEBUG_LOGOUT_ON
        qInfo("[setKeyUnHook] Normal Key Hook & Mouse Hook Released.");
#endif
    }

    setWorkerJoystickCaptureStop();
    //    setWorkerDInputKeyUnHook();

    if (QKeyMapper::getLockCursorStatus() && s_Mouse2vJoy_EnableState != MOUSE2VJOY_NONE) {
        setMouseToScreenCenter();
#ifdef DEBUG_LOGOUT_ON
        qDebug("[setWorkerKeyUnHook] Set Mouse Cursor Back to ScreenCenter.");
#endif
    }

    s_Mouse2vJoy_delta.rx() = 0;
    s_Mouse2vJoy_delta.ry() = 0;
    s_Mouse2vJoy_prev.rx() = 0;
    s_Mouse2vJoy_prev.ry() = 0;
    ViGEmClient_GamepadReset();
    s_Mouse2vJoy_EnableState = MOUSE2VJOY_NONE;
}

void QKeyMapper_Worker::setWorkerJoystickCaptureStart(HWND hWnd)
{
    Q_UNUSED(hWnd);
    m_JoystickCapture = true;
}

void QKeyMapper_Worker::setWorkerJoystickCaptureStop()
{
    m_JoystickCapture = false;
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

void QKeyMapper_Worker::onJoystickPOVEvent(const QJoystickPOVEvent &e)
{
#ifdef JOYSTICK_VERBOSE_LOG
    qDebug() << "[onJoystickPOVEvent]" << "POV ->" << e.pov << "," << "POV Angle ->" << e.angle;
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
    if (m_JoystickCapture) {
        checkJoystickAxis(e);
    }
}

void QKeyMapper_Worker::onJoystickButtonEvent(const QJoystickButtonEvent &e)
{
#ifdef JOYSTICK_VERBOSE_LOG
    qDebug() << "[onJoystickButtonEvent]" << "Button ->" << e.button << "," << "Pressed ->" << e.pressed;
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
    else if (JOYSTICK_AXIS_LS_VERTICAL == e.axis) {
        joystickLSVerticalProc(e);
    }
    else if (JOYSTICK_AXIS_LS_HORIZONTAL == e.axis) {
        joystickLSHorizontalProc(e);
    }
    else if (JOYSTICK_AXIS_RS_VERTICAL == e.axis) {
        joystickRSVerticalProc(e);
    }
    else if (JOYSTICK_AXIS_RS_HORIZONTAL == e.axis) {
        joystickRSHorizontalProc(e);
    }
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

LRESULT QKeyMapper_Worker::LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
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
            int findindex = QKeyMapper::findInKeyMappingDataList(keycodeString);
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
                        emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_HOOK);
                        returnFlag = true;
                    }
                    else { /* KEY_UP == keyupdown */
                        emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_HOOK);
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
                int findindex = QKeyMapper::findInKeyMappingDataList(keycodeString);
                if (pressedRealKeysList.contains(keycodeString) && findindex < 0){
#ifdef DEBUG_LOGOUT_ON
                    qDebug("[LowLevelKeyboardHookProc] RealKey \"%s\" is pressed down on keyboard, skip send mapping VirtualKey \"%s\" KEYUP!", keycodeString.toStdString().c_str(), keycodeString.toStdString().c_str());
#endif
                    returnFlag = true;
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
                int findindex = QKeyMapper::findInKeyMappingDataList(keycodeString);
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
                            emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_HOOK);
                            returnFlag = true;
                        }
                        else { /* KEY_UP == keyupdown */
                            emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_HOOK);
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
                short delta_abs = std::abs(zDelta);
                if (delta_abs >= WHEEL_DELTA) {
                    bool wheel_up_found = false;
                    bool wheel_down_found = false;
                    bool send_wheel_keys = false;
                    int findindex = -1;

                    int findWheelUpindex = QKeyMapper::findInKeyMappingDataList(MOUSE_STR_WHEEL_UP);
                    if (findWheelUpindex >=0){
                        wheel_up_found = true;
                    }

                    int findWheelDownindex = QKeyMapper::findInKeyMappingDataList(MOUSE_STR_WHEEL_DOWN);
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
                            qDebug() << "[LowLevelMouseHookProc]" << "Real Mouse Wheel Down -> Send Wheel Up Mapping Keys";
    #endif
                            send_wheel_keys = true;
                            findindex = findWheelDownindex;
                        }

                        if (send_wheel_keys) {
                            QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
                            QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
                            emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_HOOK);
                            emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_HOOK);
                            returnFlag = true;
                        }
                    }
                }
            }
        }

#if 0
        if (zDelta > 0)
        {
            short delta_abs = std::abs(zDelta);

            if (delta_abs >= WHEEL_DELTA) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[LowLevelMouseHookProc]" << "Mouse Wheel Up -> Delta =" << zDelta;
#endif
                int findindex = QKeyMapper::findInKeyMappingDataList(MOUSE_WHEEL_UP);

                emit QKeyMapper_Worker::getInstance()->onMouseWheel_Signal(MOUSE_WHEEL_UP);
            }
        }
        else if (zDelta < 0)
        {
            short delta_abs = std::abs(zDelta);

            if (delta_abs >= WHEEL_DELTA) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[LowLevelMouseHookProc]" << "Mouse Wheel Down -> Delta =" << zDelta;
#endif

                emit QKeyMapper_Worker::getInstance()->onMouseWheel_Signal(MOUSE_WHEEL_DOWN);
            }
        }
#endif
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
    int findindex = QKeyMapper::findInKeyMappingDataList(keycodeString);

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
                emit QKeyMapper::getInstance()->updateLockStatus_Signal();
#ifdef DEBUG_LOGOUT_ON
                qDebug("hookBurstAndLockProc(): Key \"%s\" KeyDown LockStatus -> OFF", keycodeString.toStdString().c_str());
#endif
            }
            else {
                QKeyMapper::KeyMappingDataList[findindex].LockStatus = true;
                pressedLockKeysList.append(keycodeString);
                emit QKeyMapper::getInstance()->updateLockStatus_Signal();
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

    return returnFlag;
}

bool QKeyMapper_Worker::JoyStickKeysProc(const QString &keycodeString, int keyupdown, const QString &joystickName)
{
    Q_UNUSED(joystickName);
    bool returnFlag = false;

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

    int findindex = QKeyMapper::findInKeyMappingDataList(keycodeString);
    returnFlag = hookBurstAndLockProc(keycodeString, keyupdown);

    if (false == returnFlag) {
        if (findindex >=0){
            QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
            QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
            if (KEY_DOWN == keyupdown){
                emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_HOOK);
                returnFlag = true;
            }
            else { /* KEY_UP == keyupdown */
                emit QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_HOOK);
                returnFlag = true;
            }
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[JoyStickKeysProc]" << (keyupdown == KEY_DOWN?"KEY_DOWN":"KEY_UP") << " : pressedRealKeysList -> " << pressedRealKeysList;
#endif

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
    VirtualKeyCodeMap.insert        ("NumLock",     V_KEYCODE(VK_NUMLOCK,       EXTENED_FLAG_TRUE ));   // 0x90 + E
    VirtualKeyCodeMap.insert        ("Num /",       V_KEYCODE(VK_DIVIDE,        EXTENED_FLAG_TRUE ));   // 0x6F + E
    VirtualKeyCodeMap.insert        ("Num *",       V_KEYCODE(VK_MULTIPLY,      EXTENED_FLAG_FALSE));   // 0x6A
    VirtualKeyCodeMap.insert        ("Num -",       V_KEYCODE(VK_SUBTRACT,      EXTENED_FLAG_FALSE));   // 0x6D
    VirtualKeyCodeMap.insert        ("Num +",       V_KEYCODE(VK_ADD,           EXTENED_FLAG_FALSE));   // 0x6B
    VirtualKeyCodeMap.insert        ("Num .",       V_KEYCODE(VK_DECIMAL,       EXTENED_FLAG_FALSE));   // 0x6E
    VirtualKeyCodeMap.insert        ("Num 0",       V_KEYCODE(VK_NUMPAD0,       EXTENED_FLAG_FALSE));   // 0x60
    VirtualKeyCodeMap.insert        ("Num 1",       V_KEYCODE(VK_NUMPAD1,       EXTENED_FLAG_FALSE));   // 0x61
    VirtualKeyCodeMap.insert        ("Num 2",       V_KEYCODE(VK_NUMPAD2,       EXTENED_FLAG_FALSE));   // 0x62
    VirtualKeyCodeMap.insert        ("Num 3",       V_KEYCODE(VK_NUMPAD3,       EXTENED_FLAG_FALSE));   // 0x63
    VirtualKeyCodeMap.insert        ("Num 4",       V_KEYCODE(VK_NUMPAD4,       EXTENED_FLAG_FALSE));   // 0x64
    VirtualKeyCodeMap.insert        ("Num 5",       V_KEYCODE(VK_NUMPAD5,       EXTENED_FLAG_FALSE));   // 0x65
    VirtualKeyCodeMap.insert        ("Num 6",       V_KEYCODE(VK_NUMPAD6,       EXTENED_FLAG_FALSE));   // 0x66
    VirtualKeyCodeMap.insert        ("Num 7",       V_KEYCODE(VK_NUMPAD7,       EXTENED_FLAG_FALSE));   // 0x67
    VirtualKeyCodeMap.insert        ("Num 8",       V_KEYCODE(VK_NUMPAD8,       EXTENED_FLAG_FALSE));   // 0x68
    VirtualKeyCodeMap.insert        ("Num 9",       V_KEYCODE(VK_NUMPAD9,       EXTENED_FLAG_FALSE));   // 0x69
    VirtualKeyCodeMap.insert        ("Num Enter",   V_KEYCODE(VK_RETURN,        EXTENED_FLAG_TRUE ));   // 0x0D + E
    //NumLock Off NumberPadKeys
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    VirtualKeyCodeMap.insert        ("Num .",       V_KEYCODE(VK_DELETE,        EXTENED_FLAG_FALSE));   // 0x2E
    VirtualKeyCodeMap.insert        ("Num 0",       V_KEYCODE(VK_INSERT,        EXTENED_FLAG_FALSE));   // 0x2D
    VirtualKeyCodeMap.insert        ("Num 1",       V_KEYCODE(VK_END,           EXTENED_FLAG_FALSE));   // 0x23
    VirtualKeyCodeMap.insert        ("Num 2",       V_KEYCODE(VK_DOWN,          EXTENED_FLAG_FALSE));   // 0x28
    VirtualKeyCodeMap.insert        ("Num 3",       V_KEYCODE(VK_NEXT,          EXTENED_FLAG_FALSE));   // 0x22
    VirtualKeyCodeMap.insert        ("Num 4",       V_KEYCODE(VK_LEFT,          EXTENED_FLAG_FALSE));   // 0x25
    VirtualKeyCodeMap.insert        ("Num 5",       V_KEYCODE(VK_CLEAR,         EXTENED_FLAG_FALSE));   // 0x0C
    VirtualKeyCodeMap.insert        ("Num 6",       V_KEYCODE(VK_RIGHT,         EXTENED_FLAG_FALSE));   // 0x27
    VirtualKeyCodeMap.insert        ("Num 7",       V_KEYCODE(VK_HOME,          EXTENED_FLAG_FALSE));   // 0x24
    VirtualKeyCodeMap.insert        ("Num 8",       V_KEYCODE(VK_UP,            EXTENED_FLAG_FALSE));   // 0x26
    VirtualKeyCodeMap.insert        ("Num 9",       V_KEYCODE(VK_PRIOR,         EXTENED_FLAG_FALSE));   // 0x21
#else
    VirtualKeyCodeMap.insertMulti   ("Num .",       V_KEYCODE(VK_DELETE,        EXTENED_FLAG_FALSE));   // 0x2E
    VirtualKeyCodeMap.insertMulti   ("Num 0",       V_KEYCODE(VK_INSERT,        EXTENED_FLAG_FALSE));   // 0x2D
    VirtualKeyCodeMap.insertMulti   ("Num 1",       V_KEYCODE(VK_END,           EXTENED_FLAG_FALSE));   // 0x23
    VirtualKeyCodeMap.insertMulti   ("Num 2",       V_KEYCODE(VK_DOWN,          EXTENED_FLAG_FALSE));   // 0x28
    VirtualKeyCodeMap.insertMulti   ("Num 3",       V_KEYCODE(VK_NEXT,          EXTENED_FLAG_FALSE));   // 0x22
    VirtualKeyCodeMap.insertMulti   ("Num 4",       V_KEYCODE(VK_LEFT,          EXTENED_FLAG_FALSE));   // 0x25
    VirtualKeyCodeMap.insertMulti   ("Num 5",       V_KEYCODE(VK_CLEAR,         EXTENED_FLAG_FALSE));   // 0x0C
    VirtualKeyCodeMap.insertMulti   ("Num 6",       V_KEYCODE(VK_RIGHT,         EXTENED_FLAG_FALSE));   // 0x27
    VirtualKeyCodeMap.insertMulti   ("Num 7",       V_KEYCODE(VK_HOME,          EXTENED_FLAG_FALSE));   // 0x24
    VirtualKeyCodeMap.insertMulti   ("Num 8",       V_KEYCODE(VK_UP,            EXTENED_FLAG_FALSE));   // 0x26
    VirtualKeyCodeMap.insertMulti   ("Num 9",       V_KEYCODE(VK_PRIOR,         EXTENED_FLAG_FALSE));   // 0x21
#endif

    // MultiMedia keys
    VirtualKeyCodeMap.insert        ("Vol Mute",            V_KEYCODE(VK_VOLUME_MUTE,       EXTENED_FLAG_TRUE));   // 0xAD
    VirtualKeyCodeMap.insert        ("Vol Down",            V_KEYCODE(VK_VOLUME_DOWN,       EXTENED_FLAG_TRUE));   // 0xAE
    VirtualKeyCodeMap.insert        ("Vol Up",              V_KEYCODE(VK_VOLUME_UP,         EXTENED_FLAG_TRUE));   // 0xAF
    VirtualKeyCodeMap.insert        ("Media Next",          V_KEYCODE(VK_MEDIA_NEXT_TRACK,  EXTENED_FLAG_TRUE));   // 0xB0
    VirtualKeyCodeMap.insert        ("Media Prev",          V_KEYCODE(VK_MEDIA_PREV_TRACK,  EXTENED_FLAG_TRUE));   // 0xB1
    VirtualKeyCodeMap.insert        ("Media Stop",          V_KEYCODE(VK_MEDIA_STOP,        EXTENED_FLAG_TRUE));   // 0xB2
    VirtualKeyCodeMap.insert        ("Media PlayPause",     V_KEYCODE(VK_MEDIA_PLAY_PAUSE,  EXTENED_FLAG_TRUE));   // 0xB3
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

    MouseButtonNameConvertMap.insert("L-Mouse",     "Mouse-L"   );
    MouseButtonNameConvertMap.insert("R-Mouse",     "Mouse-R"   );
    MouseButtonNameConvertMap.insert("M-Mouse",     "Mouse-M"   );
    MouseButtonNameConvertMap.insert("X1-Mouse",    "Mouse-X1"  );
    MouseButtonNameConvertMap.insert("X2-Mouse",    "Mouse-X2"  );
}
void QKeyMapper_Worker::initJoystickKeyMap()
{
    /* Joystick Buttons */
    JoyStickKeyMap.insert("Joy-Key1(A)"                   ,   (int)JOYSTICK_BUTTON_0          );
    JoyStickKeyMap.insert("Joy-Key2(B)"                   ,   (int)JOYSTICK_BUTTON_1          );
    JoyStickKeyMap.insert("Joy-Key3(X)"                   ,   (int)JOYSTICK_BUTTON_2          );
    JoyStickKeyMap.insert("Joy-Key4(Y)"                   ,   (int)JOYSTICK_BUTTON_3          );
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
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_0,       "Joy-Key1(A)"                   );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_1,       "Joy-Key2(B)"                   );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_2,       "Joy-Key3(X)"                   );
    m_JoystickButtonMap.insert(JOYSTICK_BUTTON_3,       "Joy-Key4(Y)"                   );
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

#ifdef VIGEM_CLIENT_SUPPORT
void QKeyMapper_Worker::initViGEmKeyMap()
{
    /* Virtual Joystick Buttons */
    JoyStickKeyMap.insert(VJOY_STR_MOUSE2LS             ,   (int)JOYSTICK_LS_MOUSE      );
    JoyStickKeyMap.insert(VJOY_STR_MOUSE2RS             ,   (int)JOYSTICK_RS_MOUSE      );

    JoyStickKeyMap.insert("vJoy-Key1(A)"                ,   (int)JOYSTICK_BUTTON_0      );
    JoyStickKeyMap.insert("vJoy-Key2(B)"                ,   (int)JOYSTICK_BUTTON_1      );
    JoyStickKeyMap.insert("vJoy-Key3(X)"                ,   (int)JOYSTICK_BUTTON_2      );
    JoyStickKeyMap.insert("vJoy-Key4(Y)"                ,   (int)JOYSTICK_BUTTON_3      );
    JoyStickKeyMap.insert("vJoy-Key5(LB)"               ,   (int)JOYSTICK_BUTTON_4      );
    JoyStickKeyMap.insert("vJoy-Key6(RB)"               ,   (int)JOYSTICK_BUTTON_5      );
    JoyStickKeyMap.insert("vJoy-Key7(Back)"             ,   (int)JOYSTICK_BUTTON_6      );
    JoyStickKeyMap.insert("vJoy-Key8(Start)"            ,   (int)JOYSTICK_BUTTON_7      );
    JoyStickKeyMap.insert("vJoy-Key9(LS-Click)"         ,   (int)JOYSTICK_BUTTON_8      );
    JoyStickKeyMap.insert("vJoy-Key10(RS-Click)"        ,   (int)JOYSTICK_BUTTON_9      );
    JoyStickKeyMap.insert("vJoy-Key11(LT)"              ,   (int)JOYSTICK_BUTTON_10     );
    JoyStickKeyMap.insert("vJoy-Key12(RT)"              ,   (int)JOYSTICK_BUTTON_11     );
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
    ViGEmButtonMap.insert("vJoy-Key1(A)"                ,   XUSB_GAMEPAD_A              );
    ViGEmButtonMap.insert("vJoy-Key2(B)"                ,   XUSB_GAMEPAD_B              );
    ViGEmButtonMap.insert("vJoy-Key3(X)"                ,   XUSB_GAMEPAD_X              );
    ViGEmButtonMap.insert("vJoy-Key4(Y)"                ,   XUSB_GAMEPAD_Y              );
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
#endif

void QKeyMapper_Worker::clearAllBurstTimersAndLockKeys()
{
    QList<QString> burstKeys = m_BurstTimerMap.keys();
    for (const QString &key : qAsConst(burstKeys)) {
        int timerID = m_BurstTimerMap.value(key, 0);
        if (timerID > 0) {
            int findindex = QKeyMapper::findInKeyMappingDataList(key);
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
