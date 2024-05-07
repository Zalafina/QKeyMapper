#ifndef QKEYMAPPER_WORKER_H
#define QKEYMAPPER_WORKER_H

#include <windows.h>
#include <QObject>
#include <QThread>
#include <QHash>
#include <QColor>
// #include <QHotkey>
#include <QThreadPool>
#include <QWaitCondition>
#include <QDeadlineTimer>
#include <QUdpSocket>
#include <QNetworkDatagram>
#ifdef VIGEM_CLIENT_SUPPORT
#include <QTimer>
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QRecursiveMutex>
#else
#include <QMutex>
#endif
#include <QJoysticks.h>
#ifdef VIGEM_CLIENT_SUPPORT
#include <ViGEm/Client.h>
#include <ViGEm/Util.h>
#endif
#ifdef DINPUT_TEST
#include <dinput.h>
#endif

#ifdef HOOKSTART_ONSTARTUP
#include <QAtomicInteger>
using QAtomicBool = QAtomicInteger<bool>;
#endif

#define SEPARATOR_PLUS          (" + ")
#define SEPARATOR_NEXTARROW     (" » ")
#define SEPARATOR_WAITTIME      ("⏱")
#define SEPARATOR_PRESSTIME     ("⏲")
#define POSTFIX_DOUBLECLICK     ("✖")
#define SEPARATOR_TITLESETTING  ("|")

// #define KEYBOARD_MODIFIERS      ("KeyboardModifiers")
// #define KEYSEQUENCE_STR         ("KeySequence")
// #define CLEAR_VIRTUALKEYS       ("ClearVirtualKeys")

#define MAPPING_KEYS_MAX        (60)
#define SEND_INPUTS_MAX         (100)
#define KEY_SEQUENCE_MAX        (60)

typedef struct MAP_KEYDATA
{
    QString Original_Key;
    QStringList Mapping_Keys;
    bool Burst;
    bool Lock;
    bool LockStatus;
    bool PassThrough;

    MAP_KEYDATA() : Original_Key(), Mapping_Keys(), Burst(false), Lock(false), LockStatus(false), PassThrough(false) {}

    MAP_KEYDATA(QString originalkey, QString mappingkeys, bool burst, bool lock, bool passthrough)
    {
        Original_Key = originalkey;
        Mapping_Keys = mappingkeys.split(SEPARATOR_NEXTARROW);
        Burst = burst;
        Lock = lock;
        LockStatus = false;
        PassThrough = passthrough;
    }

    bool operator==(const MAP_KEYDATA& other) const
    {
        return ((Original_Key == other.Original_Key)
                && (Mapping_Keys == other.Mapping_Keys)
                && (Burst == other.Burst)
                && (Lock == other.Lock)
                && (PassThrough == other.PassThrough));
    }
}MAP_KEYDATA_st;

typedef struct V_KEYCODE
{
    quint8 KeyCode;
    bool ExtenedFlag;

    V_KEYCODE() : KeyCode(0x00), ExtenedFlag(false) {}

    V_KEYCODE(quint8 keycode, bool extenedflag)
    {
        KeyCode = keycode;
        ExtenedFlag = extenedflag;
    }

    bool operator==(const V_KEYCODE& other) const
    {
        return ((KeyCode == other.KeyCode)
                && (ExtenedFlag == other.ExtenedFlag));
    }
}V_KEYCODE_st;

typedef struct V_MOUSECODE
{
    DWORD MouseDownCode;
    DWORD MouseUpCode;
    DWORD MouseXButton;

    V_MOUSECODE() : MouseDownCode(0x0000), MouseUpCode(0x0000), MouseXButton(0x0000) {}

    V_MOUSECODE(DWORD mousedowncode, DWORD mouseupcode, DWORD mousexbutton)
    {
        MouseDownCode = mousedowncode;
        MouseUpCode = mouseupcode;
        MouseXButton = mousexbutton;
    }

    bool operator==(const V_MOUSECODE& other) const
    {
        return ((MouseDownCode == other.MouseDownCode)
                && (MouseUpCode == other.MouseUpCode)
                && (MouseXButton == other.MouseXButton));
    }
}V_MOUSECODE_st;

struct Joystick_AxisState {
    qreal left_x;
    qreal left_y;
    qreal right_x;
    qreal right_y;
    qreal left_trigger;
    qreal right_trigger;
};

enum Joy2vJoyTriggerState
{
    JOY2VJOY_TRIGGER_NONE,
    JOY2VJOY_TRIGGER_LT,
    JOY2VJOY_TRIGGER_RT,
    JOY2VJOY_TRIGGER_LTRT_BOTH
};

enum Joy2vJoyLeftStickState
{
    JOY2VJOY_LS_NONE,
    JOY2VJOY_LS_2LS,
    JOY2VJOY_LS_2RS,
    JOY2VJOY_LS_2LSRS_BOTH
};

enum Joy2vJoyRightStickState
{
    JOY2VJOY_RS_NONE,
    JOY2VJOY_RS_2LS,
    JOY2VJOY_RS_2RS,
    JOY2VJOY_RS_2LSRS_BOTH
};

struct Joy2vJoyState {
    Joy2vJoyTriggerState trigger_state;
    Joy2vJoyLeftStickState ls_state;
    Joy2vJoyRightStickState rs_state;
};

#ifdef DINPUT_TEST
typedef HRESULT(WINAPI* GetDeviceStateT)(IDirectInputDevice8* pThis, DWORD cbData, LPVOID lpvData);
typedef HRESULT(WINAPI* GetDeviceDataT)(IDirectInputDevice8*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
#endif

// https://stackoverflow.com/a/4609795
template <typename T>
int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

class QKeyMapper_Worker : public QObject
{
    Q_OBJECT
public:
    explicit QKeyMapper_Worker(QObject *parent = Q_NULLPTR);
    ~QKeyMapper_Worker();

    static QKeyMapper_Worker *getInstance()
    {
        static QKeyMapper_Worker m_instance;
        return &m_instance;
    }

    enum VirtualKeyCode
    {
        VK_0     = 0x30,
        VK_1,   // 0x31
        VK_2,   // 0x32
        VK_3,   // 0x33
        VK_4,   // 0x34
        VK_5,   // 0x35
        VK_6,   // 0x36
        VK_7,   // 0x37
        VK_8,   // 0x38
        VK_9,   // 0x39

        VK_A     = 0x41,
        VK_B,   // 0x42
        VK_C,   // 0x43
        VK_D,   // 0x44
        VK_E,   // 0x45
        VK_F,   // 0x46
        VK_G,   // 0x47
        VK_H,   // 0x48
        VK_I,   // 0x49
        VK_J,   // 0x4A
        VK_K,   // 0x4B
        VK_L,   // 0x4C
        VK_M,   // 0x4D
        VK_N,   // 0x4E
        VK_O,   // 0x4F
        VK_P,   // 0x50
        VK_Q,   // 0x51
        VK_R,   // 0x52
        VK_S,   // 0x53
        VK_T,   // 0x54
        VK_U,   // 0x55
        VK_V,   // 0x56
        VK_W,   // 0x57
        VK_X,   // 0x58
        VK_Y,   // 0x59
        VK_Z    // 0x5A
    };
    Q_ENUM(VirtualKeyCode)

    enum MouseEvent
    {
        EVENT_NONE = 0,
        EVENT_MOUSEMOVE,
        EVENT_LBUTTONDOWN,
        EVENT_LBUTTONUP,
        EVENT_RBUTTONDOWN,
        EVENT_RBUTTONUP,
        EVENT_MBUTTONDOWN,
        EVENT_MBUTTONUP,
        EVENT_X1BUTTONDOWN,
        EVENT_X1BUTTONUP,
        EVENT_X2BUTTONDOWN,
        EVENT_X2BUTTONUP,
        EVENT_MOUSEWHEEL,
        EVENT_MOUSEHWHEEL
    };
    Q_ENUM(MouseEvent)

    enum JoystickButtonCode
    {
        JOYSTICK_BUTTON_0 = 0,
        JOYSTICK_BUTTON_1,
        JOYSTICK_BUTTON_2,
        JOYSTICK_BUTTON_3,
        JOYSTICK_BUTTON_4,
        JOYSTICK_BUTTON_5,
        JOYSTICK_BUTTON_6,
        JOYSTICK_BUTTON_7,
        JOYSTICK_BUTTON_8,
        JOYSTICK_BUTTON_9,
        JOYSTICK_BUTTON_10,
        JOYSTICK_BUTTON_11,
        JOYSTICK_BUTTON_12,
        JOYSTICK_BUTTON_13,
        JOYSTICK_BUTTON_14,
        JOYSTICK_BUTTON_15,
        JOYSTICK_BUTTON_16,
        JOYSTICK_BUTTON_17,
        JOYSTICK_BUTTON_18,
        JOYSTICK_BUTTON_19,
        JOYSTICK_BUTTON_20
    };
    Q_ENUM(JoystickButtonCode)

    enum JoystickDPadCode
    {
        JOYSTICK_DPAD_RELEASE = 0,
        JOYSTICK_DPAD_UP,
        JOYSTICK_DPAD_DOWN,
        JOYSTICK_DPAD_LEFT,
        JOYSTICK_DPAD_RIGHT,
        JOYSTICK_DPAD_L_UP,
        JOYSTICK_DPAD_L_DOWN,
        JOYSTICK_DPAD_R_UP,
        JOYSTICK_DPAD_R_DOWN
    };
    Q_ENUM(JoystickDPadCode)

    enum JoystickTrigger
    {
        JOYSTICK_LEFT_TRIGGER = 0,
        JOYSTICK_RIGHT_TRIGGER
    };
    Q_ENUM(JoystickTrigger)

    enum JoystickLStickCode
    {
        JOYSTICK_LS_RELEASE = 0,
        JOYSTICK_LS_UP,
        JOYSTICK_LS_DOWN,
        JOYSTICK_LS_LEFT,
        JOYSTICK_LS_RIGHT,
        JOYSTICK_LS_MOUSE,
        JOYSTICK_LS_2VJOY
    };
    Q_ENUM(JoystickLStickCode)

    enum JoystickRStickCode
    {
        JOYSTICK_RS_RELEASE = 0,
        JOYSTICK_RS_UP,
        JOYSTICK_RS_DOWN,
        JOYSTICK_RS_LEFT,
        JOYSTICK_RS_RIGHT,
        JOYSTICK_RS_MOUSE,
        JOYSTICK_RS_2VJOY
    };
    Q_ENUM(JoystickRStickCode)

#ifdef VIGEM_CLIENT_SUPPORT
    enum ViGEmClient_ConnectState
    {
        VIGEMCLIENT_DISCONNECTED = 0,
        VIGEMCLIENT_CONNECT_FAILED,
        VIGEMCLIENT_CONNECTING,
        VIGEMCLIENT_CONNECT_SUCCESS,
    };
    Q_ENUM(ViGEmClient_ConnectState)

    enum Mouse2vJoyState
    {
        MOUSE2VJOY_NONE     = 0x00,
        MOUSE2VJOY_LEFT     = 0x01,
        MOUSE2VJOY_RIGHT    = 0x02,
        MOUSE2VJOY_BOTH     = MOUSE2VJOY_LEFT | MOUSE2VJOY_RIGHT,
    };
    Q_ENUM(Mouse2vJoyState)
    Q_DECLARE_FLAGS(Mouse2vJoyStates, Mouse2vJoyState)

    struct Mouse2vJoyData {
        Mouse2vJoyStates states;
        int gamepad_index;

#ifdef DEBUG_LOGOUT_ON
        friend QDebug operator<<(QDebug debug, const Mouse2vJoyData& data) {
            QDebugStateSaver saver(debug);
            debug.nospace() << "Mouse2vJoyData("
                            << "gamepad_index:" << data.gamepad_index
                            << ", states:" << data.states
                            << ")";
            return debug;
        }
#endif
    };

    enum GripDetectState
    {
        GRIPDETECT_NONE     = 0,
        GRIPDETECT_BRAKE    = 1,
        GRIPDETECT_ACCEL    = 2,
        GRIPDETECT_BOTH     = GRIPDETECT_BRAKE | GRIPDETECT_ACCEL
    };
    Q_ENUM(GripDetectState)
    Q_DECLARE_FLAGS(GripDetectStates, GripDetectState)

    Q_ENUM(Joy2vJoyTriggerState)
    Q_ENUM(Joy2vJoyLeftStickState)
    Q_ENUM(Joy2vJoyRightStickState)
#endif

    enum Joy2MouseState
    {
        JOY2MOUSE_NONE,
        JOY2MOUSE_LEFT,
        JOY2MOUSE_RIGHT,
        JOY2MOUSE_BOTH
    };
    Q_ENUM(Joy2MouseState)

public slots:
    void sendKeyboardInput(V_KEYCODE vkeycode, int keyupdown);
    void sendMouseClick(V_MOUSECODE vmousecode, int keyupdown);
    void sendMouseMove(int delta_x, int delta_y);
    void sendMouseWheel(int wheel_updown);
    void setMouseToScreenCenter(void);
    void setMouseToPoint(POINT point);
    void setMouseToScreenBottomRight(void);
    POINT mousePositionAfterSetMouseToScreenBottomRight(void);
#ifdef VIGEM_CLIENT_SUPPORT
    void onMouseMove(int delta_x, int delta_y, int mouse_index);
    // void onMouse2vJoyResetTimeout(void);
    void initMouse2vJoyResetTimerMap(void);
    void stopMouse2vJoyResetTimerMap(void);
    void onMouse2vJoyResetTimeoutForMap(int mouse_index);
#endif
    void onKey2MouseCycleTimeout(void);
    void onMouseWheel(int wheel_updown);
    void onSendInputKeys(QStringList inputKeys, int keyupdown, QString original_key, int sendmode);
    void sendInputKeys(QStringList inputKeys, int keyupdown, QString original_key, int sendmode);
    // void send_WINplusD(void);
    void sendMousePointClick(QString &mousepoint_str, int keyupdown);

public:
    void sendBurstKeyDown(const QString &burstKey);
    void sendBurstKeyUp(const QString &burstKey, bool stop);
#if 0
    void sendSpecialVirtualKey(const QString &keycodeString, int keyupdown);
    void sendSpecialVirtualKeyDown(const QString &virtualKey);
    void sendSpecialVirtualKeyUp(const QString &virtualKey);
#endif

    void startDataPortListener(void);
    void stopDataPortListener(void);

#ifdef VIGEM_CLIENT_SUPPORT
public:
    static int ViGEmClient_Alloc(void);
    static int ViGEmClient_Connect(void);
    // static int ViGEmClient_Add(void);
    static PVIGEM_TARGET ViGEmClient_AddTarget_byType(const QString &gamepadtype);
    // static void ViGEmClient_Remove(void);
    static void ViGEmClient_RemoveTarget(PVIGEM_TARGET target);
    static void ViGEmClient_RemoveAllTargets(void);
    static void ViGEmClient_Disconnect(void);
    static void ViGEmClient_Free(void);

    static void saveVirtualGamepadList(void);
    static void loadVirtualGamepadList(const QStringList& gamepadlist);

    static void updateViGEmBusStatus(void);
    static void updateLockStatus(void);

    static ViGEmClient_ConnectState ViGEmClient_getConnectState(void);
    static void ViGEmClient_setConnectState(ViGEmClient_ConnectState connectstate);

    static void ViGEmClient_PressButton(const QString &joystickButton, int autoAdjust, int gamepad_index);
    static void ViGEmClient_ReleaseButton(const QString &joystickButton, int gamepad_index);
    static void ViGEmClient_CheckJoysticksReportData(int gamepad_index);
    static void ViGEmClient_CalculateThumbValue(SHORT* ori_ThumbX, SHORT* ori_ThumbY);

    // static Mouse2vJoyStates ViGEmClient_checkMouse2JoystickEnableState(void);
    static QHash<int, Mouse2vJoyData> ViGEmClient_checkMouse2JoystickEnableStateMap(void);
    void ViGEmClient_Mouse2JoystickUpdate(int delta_x, int delta_y, int mouse_index, int gamepad_index);
    void ViGEmClient_Joy2vJoystickUpdate(int sticktype, int gamepad_index);
    // void ViGEmClient_GamepadReset(void);
    void ViGEmClient_AllGamepadReset(void);
    static void ViGEmClient_GamepadReset_byIndex(int gamepad_index);
    void ViGEmClient_JoysticksReset(int mouse_index, int gamepad_index);
#endif

signals:
    void setKeyHook_Signal(HWND hWnd);
    void setKeyUnHook_Signal(void);
    void startBurstTimer_Signal(const QString &burstKey, int mappingIndex);
    void stopBurstTimer_Signal(const QString &burstKey, int mappingIndex);
#if 0
    void sendKeyboardInput_Signal(V_KEYCODE vkeycode, int keyupdown);
    void sendMouseClick_Signal(V_MOUSECODE vmousecode, int keyupdown);
#endif
    void sendInputKeys_Signal(QStringList inputKeys, int keyupdown, QString original_key, int sendmode);
#ifdef VIGEM_CLIENT_SUPPORT
    void onMouseMove_Signal(int delta_x, int delta_y, int mouse_index);
#endif
#if 0
    void onMouseWheel_Signal(int wheel_updown);
#endif
    // void send_WINplusD_Signal(void);
    // void HotKeyTrigger_Signal(const QString &keycodeString, int keyupdown);
#if 0
    void sendSpecialVirtualKey_Signal(const QString &keycodeString, int keyupdown);
#endif
    void startMouse2vJoyResetTimer_Signal(const QString &mouse2joy_keystr, int mouse_index);
    void stopMouse2vJoyResetTimer_Signal(const QString &mouse2joy_keystr, int mouse_index);
    void doFunctionMappingProc_Signal(const QString &func_keystring);

protected:
    void timerEvent(QTimerEvent *event) override;

public slots:
    void threadStarted(void);
    void setWorkerKeyHook(HWND hWnd);
    void setWorkerKeyUnHook(void);
    void setWorkerJoystickCaptureStart(void);
    void setWorkerJoystickCaptureStop(void);
    // void HotKeyHookProc(const QString &keycodeString, int keyupdown);
    GripDetectStates checkGripDetectEnableState(void);
    Joy2vJoyState checkJoy2vJoyState(void);
    void processUdpPendingDatagrams(void);
    void processForzaFormatData(const QByteArray &forzadata);

#ifdef DINPUT_TEST
    void setWorkerDInputKeyHook(HWND hWnd);
    void setWorkerDInputKeyUnHook(void);
#endif
    void startBurstTimer(const QString &burstKey, int mappingIndex);
    void stopBurstTimer(const QString &burstKey, int mappingIndex);
    // void onJoystickcountChanged(void);
    void onJoystickAdded(const QJoystickDevice *joystick_added);
    void onJoystickRemoved(const QJoystickDevice joystick_removed);
    void onJoystickPOVEvent(const QJoystickPOVEvent &e);
    void onJoystickAxisEvent(const QJoystickAxisEvent &e);
    void onJoystickButtonEvent(const QJoystickButtonEvent &e);

    void checkJoystickButtons(const QJoystickButtonEvent &e);
    void checkJoystickPOV(const QJoystickPOVEvent &e);
    void checkJoystickAxis(const QJoystickAxisEvent &e);

    void startMouse2vJoyResetTimer(const QString &mouse2joy_keystr, int mouse_index_param);
    void stopMouse2vJoyResetTimer(const QString &mouse2joy_keystr, int mouse_index_param);
    Joy2MouseState checkJoystick2MouseEnableState(void);
    bool checkKey2MouseEnableState(void);
    void doFunctionMappingProc(const QString &func_keystring);

private:
    void joystickLTRTButtonProc(const QJoystickAxisEvent &e);
    void joystickLSHorizontalProc(const QJoystickAxisEvent &e);
    void joystickLSVerticalProc(const QJoystickAxisEvent &e);
    void joystickRSHorizontalProc(const QJoystickAxisEvent &e);
    void joystickRSVerticalProc(const QJoystickAxisEvent &e);
    int  joystickCalculateDelta(qreal axis_value, int Speed_Factor, bool checkJoystick);
    void joystick2MouseMoveProc(const Joystick_AxisState &axis_state);
    void key2MouseMoveProc(void);

public:
    static bool InterceptionKeyboardHookProc(UINT scan_code, int keyupdown, ULONG_PTR extra_info, bool ExtenedFlag_e0, bool ExtenedFlag_e1, int keyboard_index);
    static bool InterceptionMouseHookProc(MouseEvent mouse_event, int delta_x, int delta_y, short delta_wheel, unsigned short flags, ULONG_PTR extra_info, int mouse_index);
    static LRESULT CALLBACK LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    static bool hookBurstAndLockProc(const QString &keycodeString, int keyupdown);
    static void updatePressedRealKeysList(const QString &keycodeString, int keyupdown);
    static bool detectDisplaySwitchKey(const QString &keycodeString, int keyupdown);
    static bool detectMappingSwitchKey(const QString &keycodeString, int keyupdown);
    static int detectCombinationKeys(const QString &keycodeString, int keyupdown);
    static int CombinationKeyProc(const QString &keycodeString, int keyupdown);
    static void releaseKeyboardModifiers(const Qt::KeyboardModifiers &modifiers);

    static void collectCombinationOriginalKeysList(void);
    static void collectLongPressOriginalKeysMap(void);
    static void sendLongPressTimers(const QString &keycodeString);
    static void clearLongPressTimer(const QString &keycodeString);
    static void removeLongPressTimerOnTimeout(const QString &keycodeStringWithPressTime);
    static void clearAllLongPressTimers(void);
    static void longPressKeyProc(const QString &keycodeString, int keyupdown);
    static void collectDoubleClickOriginalKeysMap(void);
    static void sendDoubleClickTimers(const QString &keycodeString);
    static void clearDoubleClickTimer(const QString &keycodeString);
    static void removeDoubleClickTimerOnTimeout(const QString &keycodeString);
    static void clearAllDoubleClickTimers(void);
    static void doubleClickKeyProc(const QString &keycodeString, int keyupdown);
    static QString getWindowsKeyName(uint virtualKeyCode);
    static QString getKeycodeStringRemoveMultiInput(const QString &keycodeString);

public slots:
    static void onLongPressTimeOut(const QString keycodeStringWithPressTime);
    static void onDoubleClickTimeOut(const QString keycodeString);

private:
    bool JoyStickKeysProc(const QString &keycodeString, int keyupdown, const QString &joystickName);
#ifdef DINPUT_TEST
    static void* HookVTableFunction(void* pVTable, void* fnHookFunc, int nOffset);
    static HRESULT WINAPI hookGetDeviceState(IDirectInputDevice8* pThis, DWORD cbData, LPVOID lpvData);
    static HRESULT WINAPI hookGetDeviceData(IDirectInputDevice8* pThis, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags);
#endif

    void initVirtualKeyCodeMap(void);
    void initVirtualMouseButtonMap(void);
    void initMultiKeyboardInputList(void);
    void initMultiMouseInputList(void);
    void initMultiVirtualGamepadInputList(void);
    void initCombinationKeysList(void);
    void initJoystickKeyMap(void);
    // void initSkipReleaseModifiersKeysList(void);
#ifdef VIGEM_CLIENT_SUPPORT
    void initViGEmKeyMap(void);
    bool isCursorAtBottomRight(void);
#endif
    void clearAllBurstTimersAndLockKeys(void);
    void clearAllPressedVirtualKeys(void);
    void clearAllPressedRealCombinationKeys(void);
    void collectExchangeKeysList(void);
    bool isPressedMappingKeysContains(QString &key);
public:
#if 0
    int makeKeySequenceInputarray(QStringList &keyseq_list, INPUT *input_array);
#endif
    void sendKeySequenceList(QStringList &keyseq_list, QString &original_key);

public:
    static bool s_isWorkerDestructing;
#ifdef HOOKSTART_ONSTARTUP
    static QAtomicBool s_AtomicHookProcStart;
#endif
    static QAtomicBool s_Mouse2vJoy_Hold;
    static QAtomicBool s_Mouse2vJoy_Direct;
    static QAtomicBool s_Key2Mouse_Up;
    static QAtomicBool s_Key2Mouse_Down;
    static QAtomicBool s_Key2Mouse_Left;
    static QAtomicBool s_Key2Mouse_Right;
    static bool s_forceSendVirtualKey;
    static qint32 s_LastCarOrdinal;
    static QHash<QString, V_KEYCODE> VirtualKeyCodeMap;
    static QHash<QString, V_MOUSECODE> VirtualMouseButtonMap;
    static QHash<WPARAM, QString> MouseButtonNameMap;
#ifdef MOUSEBUTTON_CONVERT
    static QHash<QString, QString> MouseButtonNameConvertMap;
#endif
    static QStringList MultiKeyboardInputList;
    static QStringList MultiMouseInputList;
    static QStringList MultiVirtualGamepadInputList;
    static QStringList CombinationKeysList;
    // static QStringList skipReleaseModifiersKeysList;
    static QHash<QString, int> JoyStickKeyMap;
    // static QHash<QString, QHotkey*> ShortcutsMap;
#ifdef VIGEM_CLIENT_SUPPORT
    static QHash<QString, XUSB_BUTTON> ViGEmButtonMap;
#endif
    static QStringList pressedRealKeysList;
    static QStringList pressedRealKeysListRemoveMultiInput;
    static QStringList pressedVirtualKeysList;
    static QStringList pressedLongPressKeysList;
    static QStringList pressedDoubleClickKeysList;
    static QList<QList<quint8>> pressedMultiKeyboardVKeyCodeList;
    // static QStringList pressedShortcutKeysList;
    static QStringList combinationOriginalKeysList;
    static QHash<QString, QList<int>> longPressOriginalKeysMap;
    static QHash<QString, QTimer*> s_longPressTimerMap;
    static QHash<QString, int> doubleClickOriginalKeysMap;
    static QHash<QString, QTimer*> s_doubleClickTimerMap;
#ifdef VIGEM_CLIENT_SUPPORT
    static QList<QStringList> pressedvJoyLStickKeysList;
    static QList<QStringList> pressedvJoyRStickKeysList;
    static QList<QStringList> pressedvJoyButtonsList;
#endif
    static QHash<QString, QStringList> pressedMappingKeysMap;
    static QStringList pressedLockKeysList;
    static QStringList exchangeKeysList;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    static QRecursiveMutex sendinput_mutex;
#else
    static QMutex sendinput_mutex;
#endif
#ifdef DINPUT_TEST
    static GetDeviceStateT FuncPtrGetDeviceState;
    static GetDeviceDataT FuncPtrGetDeviceData;
    static int dinput_timerid;
#endif
#ifdef VIGEM_CLIENT_SUPPORT
    static PVIGEM_CLIENT s_ViGEmClient;
    // static PVIGEM_TARGET s_ViGEmTarget;
    static QList<PVIGEM_TARGET> s_ViGEmTargetList;
    static ViGEmClient_ConnectState s_ViGEmClient_ConnectState;
    // static XUSB_REPORT s_ViGEmTarget_Report;
    static QList<XUSB_REPORT> s_ViGEmTarget_ReportList;
    static QStringList s_VirtualGamepadList;
    static BYTE s_Auto_Brake;
    static BYTE s_Auto_Accel;
    static BYTE s_last_Auto_Brake;
    static BYTE s_last_Auto_Accel;
    static GripDetectStates s_GripDetect_EnableState;
    static Joy2vJoyState s_Joy2vJoyState;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    static QRecursiveMutex s_ViGEmClient_Mutex;
#else
    static QMutex s_ViGEmClient_Mutex;
#endif
    static QPoint s_Mouse2vJoy_delta;
    static QPoint s_Mouse2vJoy_prev;
    // static QList<QPoint> s_Mouse2vJoy_delta_List;
    // static QPoint s_Mouse2vJoy_delta_interception;
    // static Mouse2vJoyStates s_Mouse2vJoy_EnableState;
    static QHash<int, Mouse2vJoyData> s_Mouse2vJoy_EnableStateMap;
    // static QMutex s_MouseMove_delta_List_Mutex;
#endif

    static bool s_Key2Mouse_EnableState;
    static Joy2MouseState s_Joy2Mouse_EnableState;
    static Joystick_AxisState s_JoyAxisState;

private:
    HHOOK m_KeyHook;
    HHOOK m_MouseHook;
#ifdef VIGEM_CLIENT_SUPPORT
    POINT m_LastMouseCursorPoint;
#endif
    QRunnable *m_sendInputTask;
    QWaitCondition m_sendInputStopCondition;
    QMutex m_sendInputStopMutex;
    bool m_sendInputStopFlag;
    bool m_JoystickCapture;
#ifdef DINPUT_TEST
    IDirectInput8* m_DirectInput;
#endif
#ifdef VIGEM_CLIENT_SUPPORT
    // QTimer m_Mouse2vJoyResetTimer;
    QHash<int, QTimer*> m_Mouse2vJoyResetTimerMap;
#endif
    QTimer m_Key2MouseCycleTimer;
    QUdpSocket *m_UdpSocket;
    QHash<QString, int> m_BurstTimerMap;
    QHash<QString, int> m_BurstKeyUpTimerMap;
    QHash<JoystickButtonCode, QString> m_JoystickButtonMap;
    QHash<JoystickDPadCode, QString> m_JoystickDPadMap;
    QHash<JoystickLStickCode, QString> m_JoystickLStickMap;
    QHash<JoystickRStickCode, QString> m_JoystickRStickMap;
    QHash<int, JoystickDPadCode> m_JoystickPOVMap;
};

class SendInputTask : public QRunnable
{
public:
    SendInputTask(const QStringList& inputKeys, int keyupdown, const QString& original_key, int sendmode)
        : m_inputKeys(inputKeys), m_keyupdown(keyupdown), m_original_key(original_key), m_sendmode(sendmode)
    {
    }

    void run() override
    {
        QKeyMapper_Worker::getInstance()->sendInputKeys(m_inputKeys, m_keyupdown, m_original_key, m_sendmode);
    }

private:
    QStringList m_inputKeys;
    int m_keyupdown;
    QString m_original_key;
    int m_sendmode;
};

class QKeyMapper_Hook_Proc : public QObject
{
    Q_OBJECT
public:
    explicit QKeyMapper_Hook_Proc(QObject *parent = Q_NULLPTR);
    ~QKeyMapper_Hook_Proc();

    static QKeyMapper_Hook_Proc *getInstance()
    {
        static QKeyMapper_Hook_Proc m_instance;
        return &m_instance;
    }

signals:
    void setKeyHook_Signal(HWND hWnd);
    void setKeyUnHook_Signal(void);

public slots:
    void HookProcThreadStarted(void);
    void HookProcThreadFinished(void);
#ifndef HOOKSTART_ONSTARTUP
    void onSetHookProcKeyHook(HWND hWnd);
    void onSetHookProcKeyUnHook(void);
#endif

public:
    static bool s_LowLevelKeyboardHook_Enable;
    static bool s_LowLevelMouseHook_Enable;

    static HHOOK s_KeyHook;
    static HHOOK s_MouseHook;
};

bool EnablePrivilege(LPCWSTR privilege);
bool DisablePrivilege(LPCWSTR privilege);

#endif // QKEYMAPPER_WORKER_H
