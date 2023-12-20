#ifndef QKEYMAPPER_WORKER_H
#define QKEYMAPPER_WORKER_H

#include <windows.h>
#include <QObject>
#include <QThread>
#include <QHash>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QRecursiveMutex>
#else
#include <QMutex>
#endif
#include <QJoysticks.h>
#ifdef VIGEM_CLIENT_SUPPORT
#include <ViGEm/Client.h>
#endif
#ifdef DINPUT_TEST
#include <dinput.h>
#endif

class QKeyMapper;

#define EXTENED_FLAG_TRUE   true
#define EXTENED_FLAG_FALSE  false

#define SEPARATOR_PLUS          (" + ")
#define SEPARATOR_NEXTARROW     (" » ")

#define SEND_INPUTS_MAX     (30)
#define KEY_SEQUENCE_MAX    (8)

typedef struct MAP_KEYDATA
{
    QString Original_Key;
    QStringList Mapping_Keys;
    bool Burst;
    bool Lock;
    bool LockStatus;

    MAP_KEYDATA() : Original_Key(), Mapping_Keys(), Burst(false), Lock(false), LockStatus(false) {}

    MAP_KEYDATA(QString originalkey, QString mappingkeys, bool burst, bool lock)
    {
        Original_Key = originalkey;
        Mapping_Keys = mappingkeys.split(SEPARATOR_NEXTARROW);
        Burst = burst;
        Lock = lock;
        LockStatus = false;
    }

    bool operator==(const MAP_KEYDATA& other) const
    {
        return ((Original_Key == other.Original_Key)
                && (Mapping_Keys == other.Mapping_Keys)
                && (Burst == other.Burst)
                && (Lock == other.Lock));
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

#ifdef DINPUT_TEST
typedef HRESULT(WINAPI* GetDeviceStateT)(IDirectInputDevice8* pThis, DWORD cbData, LPVOID lpvData);
typedef HRESULT(WINAPI* GetDeviceDataT)(IDirectInputDevice8*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
#endif

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
        JOYSTICK_BUTTON_11
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

    enum JoystickLStickCode
    {
        JOYSTICK_LS_RELEASE = 0,
        JOYSTICK_LS_UP,
        JOYSTICK_LS_DOWN,
        JOYSTICK_LS_LEFT,
        JOYSTICK_LS_RIGHT
    };
    Q_ENUM(JoystickLStickCode)

    enum JoystickRStickCode
    {
        JOYSTICK_RS_RELEASE = 0,
        JOYSTICK_RS_UP,
        JOYSTICK_RS_DOWN,
        JOYSTICK_RS_LEFT,
        JOYSTICK_RS_RIGHT
    };
    Q_ENUM(JoystickRStickCode)

#ifdef VIGEM_CLIENT_SUPPORT
    enum ViGEmClient_ConnectState
    {
        VIGEMCLIENT_DISCONNECTED = 0,
        VIGEMCLIENT_CONNECT_SUCCESS,
        VIGEMCLIENT_CONNECT_FAILED,
    };
    Q_ENUM(ViGEmClient_ConnectState)
#endif

public slots:
    void sendKeyboardInput(V_KEYCODE vkeycode, int keyupdown);
    void sendMouseInput(V_MOUSECODE vmousecode, int keyupdown);
    void sendMouseMove(int x, int y);
    void sendInputKeys(QStringList inputKeys, int keyupdown, QString original_key, int sendmode);
    void send_WINplusD(void);

public:
    void sendBurstKeyDown(const QString &burstKey);
    void sendBurstKeyUp(const QString &burstKey, bool stop);
    void sendSpecialVirtualKeyDown(const QString &virtualKey);
    void sendSpecialVirtualKeyUp(const QString &virtualKey);

#ifdef VIGEM_CLIENT_SUPPORT
public:
    static int ViGEmClient_Alloc(void);
    static int ViGEmClient_Connect(void);
    static int ViGEmClient_Add(void);
    static void ViGEmClient_Remove(void);
    static void ViGEmClient_Disconnect(void);
    static void ViGEmClient_Free(void);

    static void ViGEmClient_PressButton(const QString &joystickButton);
    static void ViGEmClient_ReleaseButton(const QString &joystickButton);
#endif

signals:
    void setKeyHook_Signal(HWND hWnd);
    void setKeyUnHook_Signal(void);
    void startBurstTimer_Signal(const QString &burstKey, int mappingIndex);
    void stopBurstTimer_Signal(const QString &burstKey, int mappingIndex);
    void sendKeyboardInput_Signal(V_KEYCODE vkeycode, int keyupdown);
    void sendMouseInput_Signal(V_MOUSECODE vmousecode, int keyupdown);
    void sendInputKeys_Signal(QStringList inputKeys, int keyupdown, QString original_key, int sendmode);
    void send_WINplusD_Signal(void);

protected:
    void timerEvent(QTimerEvent *event) override;

public slots:
    void threadStarted(void);
    void setWorkerKeyHook(HWND hWnd);
    void setWorkerKeyUnHook(void);
    void setWorkerJoystickCaptureStart(HWND hWnd);
    void setWorkerJoystickCaptureStop(void);
#ifdef DINPUT_TEST
    void setWorkerDInputKeyHook(HWND hWnd);
    void setWorkerDInputKeyUnHook(void);
#endif
    void startBurstTimer(const QString &burstKey, int mappingIndex);
    void stopBurstTimer(const QString &burstKey, int mappingIndex);
    void onJoystickPOVEvent(const QJoystickPOVEvent &e);
    void onJoystickAxisEvent(const QJoystickAxisEvent &e);
    void onJoystickButtonEvent(const QJoystickButtonEvent &e);

    void checkJoystickButtons(const QJoystickButtonEvent &e);
    void checkJoystickPOV(const QJoystickPOVEvent &e);
    void checkJoystickAxis(const QJoystickAxisEvent &e);

private:
    void joystickLTRTButtonProc(const QJoystickAxisEvent &e);
    void joystickLSVerticalProc(const QJoystickAxisEvent &e);
    void joystickLSHorizontalProc(const QJoystickAxisEvent &e);
    void joystickRSVerticalProc(const QJoystickAxisEvent &e);
    void joystickRSHorizontalProc(const QJoystickAxisEvent &e);

private:
    static LRESULT CALLBACK LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    static bool hookBurstAndLockProc(const QString &keycodeString, int keyupdown);

    bool JoyStickKeysProc(const QString &keycodeString, int keyupdown, const QString &joystickName);
#ifdef DINPUT_TEST
    static void* HookVTableFunction(void* pVTable, void* fnHookFunc, int nOffset);
    static HRESULT WINAPI hookGetDeviceState(IDirectInputDevice8* pThis, DWORD cbData, LPVOID lpvData);
    static HRESULT WINAPI hookGetDeviceData(IDirectInputDevice8* pThis, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags);
#endif

    void initVirtualKeyCodeMap(void);
    void initVirtualMouseButtonMap(void);
    void initJoystickKeyMap(void);
#ifdef VIGEM_CLIENT_SUPPORT
    void initViGEmKeyMap(void);
#endif
    void clearAllBurstTimersAndLockKeys(void);
    void collectExchangeKeysList(void);
    bool isPressedMappingKeysContains(QString &key);
public:
    int makeKeySequenceInputarray(QStringList &keyseq_list, INPUT *input_array);

public:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    static QMultiHash<QString, V_KEYCODE> VirtualKeyCodeMap;
#else
    static QHash<QString, V_KEYCODE> VirtualKeyCodeMap;
#endif
    static QHash<QString, V_MOUSECODE> VirtualMouseButtonMap;
    static QHash<WPARAM, QString> MouseButtonNameMap;
    static QHash<QString, QString> MouseButtonNameConvertMap;
    static QHash<QString, int> JoyStickKeyMap;
#ifdef VIGEM_CLIENT_SUPPORT
    static QHash<QString, XUSB_BUTTON> ViGEmButtonMap;
#endif
    static QStringList pressedRealKeysList;
    static QStringList pressedVirtualKeysList;
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
    static PVIGEM_TARGET s_ViGEmTarget;
    static ViGEmClient_ConnectState s_ViGEmClient_ConnectState;
    static XUSB_REPORT s_ViGEmTarget_Report;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    static QRecursiveMutex s_ViGEmClient_Mutex;
#else
    static QMutex s_ViGEmClient_Mutex;
#endif
#endif

private:
    HHOOK m_KeyHook;
    HHOOK m_MouseHook;
    bool m_JoystickCapture;
#ifdef QT_DEBUG
    bool m_LowLevelKeyboardHook_Enable;
    bool m_LowLevelMouseHook_Enable;
#endif
#ifdef DINPUT_TEST
    IDirectInput8* m_DirectInput;
#endif
    QHash<QString, int> m_BurstTimerMap;
    QHash<QString, int> m_BurstKeyUpTimerMap;
    QHash<JoystickButtonCode, QString> m_JoystickButtonMap;
    QHash<JoystickDPadCode, QString> m_JoystickDPadMap;
    QHash<JoystickLStickCode, QString> m_JoystickLStickMap;
    QHash<JoystickRStickCode, QString> m_JoystickRStickMap;
    QHash<int, JoystickDPadCode> m_JoystickPOVMap;
};

#endif // QKEYMAPPER_WORKER_H
