#ifndef QKEYMAPPER_WORKER_H
#define QKEYMAPPER_WORKER_H

#include <windows.h>
#include <QObject>
#include <QThread>
#include <QHash>
#include <QColor>
#include <QPoint>
#include <QSize>
// #include <QHotkey>
#include <QThreadPool>
#include <QWaitCondition>
#include <QDeadlineTimer>
#include <QUdpSocket>
#include <QNetworkDatagram>
#ifdef VIGEM_CLIENT_SUPPORT
#include <QTimer>
#endif
#include <QMutex>
#include <QJoysticks.h>
#include <orderedmap.h>
#include <GamepadMotion.hpp>
#ifdef VIGEM_CLIENT_SUPPORT
#include <ViGEm/Client.h>
#include <ViGEm/Util.h>
#endif
#ifdef DINPUT_TEST
#include <dinput.h>
#endif
#ifdef FAKERINPUT_SUPPORT
#include <fakerinputclient.h>
#endif

#include <QAtomicInteger>
using QAtomicBool = QAtomicInteger<bool>;

#include "volumecontroller.h"
#include "qkeymapper_constants.h"

QStringList splitMappingKeyString(const QString &mappingkeystr, int split_type, bool pure_keys = false);
QStringList splitOriginalKeyString(const QString &originalkeystr, bool pure_keys = false);
QString getRealOriginalKey(const QString &original_key);
QStringList expandRepeatKeys(const QStringList &inputKeys, int nesting_level = 0);
QStringList expandMacroKeys(const QStringList &inputKeys, int nesting_level = 0, QSet<QString> *expandingMacros = nullptr);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
typedef char16_t* Utf16Pointer;
#else
typedef ushort* Utf16Pointer;
#endif

typedef struct MAP_KEYDATA
{
    QString Original_Key;
    QStringList Mapping_Keys;
    QStringList Pure_MappingKeys;
    QStringList Pure_OriginalKeys;
    QStringList MappingKeys_KeyUp;
    QStringList Pure_MappingKeys_KeyUp;
    QString Note;
    QString Category;
    bool Burst;
    int BurstPressTime;
    int BurstReleaseTime;
    bool Lock;
    bool MappingKeyUnlock;
    bool DisableOriginalKeyUnlock;
    bool DisableFnKeySwitch;
    bool PostMappingKey;
    int SendMappingKeyMethod;
    int FixedVKeyCode;
    uint LockState;
    bool CheckCombKeyOrder;
    bool Unbreakable;
    bool PassThrough;
    int SendTiming;
    int PasteTextMode;
    bool KeySeqHoldDown;
    int RepeatMode;
    int RepeatTimes;
    QColor Crosshair_CenterColor;
    int Crosshair_CenterSize;
    int Crosshair_CenterOpacity;
    QColor Crosshair_CrosshairColor;
    int Crosshair_CrosshairWidth;
    int Crosshair_CrosshairLength;
    int Crosshair_CrosshairOpacity;
    bool Crosshair_ShowCenter;
    bool Crosshair_ShowTop;
    bool Crosshair_ShowBottom;
    bool Crosshair_ShowLeft;
    bool Crosshair_ShowRight;
    int Crosshair_X_Offset;
    int Crosshair_Y_Offset;

    MAP_KEYDATA() :
      Original_Key()
    , Mapping_Keys()
    , Pure_MappingKeys()
    , Pure_OriginalKeys()
    , MappingKeys_KeyUp()
    , Pure_MappingKeys_KeyUp()
    , Note()
    , Category()
    , Burst(false)
    , BurstPressTime(QKeyMapperConstants::BURST_PRESS_TIME_DEFAULT)
    , BurstReleaseTime(QKeyMapperConstants::BURST_RELEASE_TIME_DEFAULT)
    , Lock(false)
    , MappingKeyUnlock(false)
    , DisableOriginalKeyUnlock(false)
    , DisableFnKeySwitch(false)
    , PostMappingKey(false)
    , SendMappingKeyMethod(QKeyMapperConstants::SENDMAPPINGKEY_METHOD_FAKERINPUT)
    , FixedVKeyCode(QKeyMapperConstants::FIXED_VIRTUAL_KEY_CODE_NONE)
    , LockState(QKeyMapperConstants::LOCK_STATE_LOCKOFF)
    , CheckCombKeyOrder(true)
    , Unbreakable(false)
    , PassThrough(false)
    , SendTiming(QKeyMapperConstants::SENDTIMING_NORMAL)
    , PasteTextMode(QKeyMapperConstants::PASTETEXT_MODE_SHIFTINSERT)
    , KeySeqHoldDown(false)
    , RepeatMode(QKeyMapperConstants::REPEAT_MODE_NONE)
    , RepeatTimes(QKeyMapperConstants::REPEAT_TIMES_DEFAULT)
    , Crosshair_CenterColor(QKeyMapperConstants::CROSSHAIR_CENTERCOLOR_DEFAULT)
    , Crosshair_CenterSize(QKeyMapperConstants::CROSSHAIR_CENTERSIZE_DEFAULT)
    , Crosshair_CenterOpacity(QKeyMapperConstants::CROSSHAIR_CENTEROPACITY_DEFAULT)
    , Crosshair_CrosshairColor(QKeyMapperConstants::CROSSHAIR_CROSSHAIRCOLOR_DEFAULT)
    , Crosshair_CrosshairWidth(QKeyMapperConstants::CROSSHAIR_CROSSHAIRWIDTH_DEFAULT)
    , Crosshair_CrosshairLength(QKeyMapperConstants::CROSSHAIR_CROSSHAIRLENGTH_DEFAULT)
    , Crosshair_CrosshairOpacity(QKeyMapperConstants::CROSSHAIR_CROSSHAIROPACITY_DEFAULT)
    , Crosshair_ShowCenter(true)
    , Crosshair_ShowTop(true)
    , Crosshair_ShowBottom(true)
    , Crosshair_ShowLeft(true)
    , Crosshair_ShowRight(true)
    , Crosshair_X_Offset(QKeyMapperConstants::CROSSHAIR_X_OFFSET_DEFAULT)
    , Crosshair_Y_Offset(QKeyMapperConstants::CROSSHAIR_Y_OFFSET_DEFAULT)
    {}

    MAP_KEYDATA(QString originalkey, QString mappingkeys, QString mappingkeys_keyup, QString note, QString category,
                bool burst, int burstpresstime, int burstreleasetime,
                bool lock, bool mappingkeys_unlock, bool disable_originalkeyunlock, bool disable_fnkeyswitch,
                bool postmappingkey, int fixedvkeycode,
                bool checkcombkeyorder, bool unbreakable, bool passthrough,
                int sendtiming, int pastetextmode, bool keyseqholddown,
                int repeat_mode, int repeat_times,
                QColor crosshair_centercolor, int crosshair_centersize, int crosshair_centeropacity,
                QColor crosshair_crosshaircolor, int crosshair_crosshairwidth, int crosshair_crosshairlength, int crosshair_crosshairopacity,
                int crosshair_showcenter, int crosshair_showtop, int crosshair_showbottom, int crosshair_showleft, int crosshair_showright,
                int crosshair_x_offset, int crosshair_y_offset)
    {
        Original_Key = originalkey;
        Mapping_Keys = splitMappingKeyString(mappingkeys, QKeyMapperConstants::SPLIT_WITH_NEXT);
        Pure_MappingKeys = splitMappingKeyString(mappingkeys, QKeyMapperConstants::SPLIT_WITH_PLUSANDNEXT, true);
        Pure_MappingKeys.removeDuplicates();
        Pure_OriginalKeys = splitOriginalKeyString(originalkey, true);
        Pure_OriginalKeys.removeDuplicates();
        if (mappingkeys_keyup.isEmpty()) {
            MappingKeys_KeyUp = Mapping_Keys;
            Pure_MappingKeys_KeyUp = Pure_MappingKeys;
        }
        else {
            MappingKeys_KeyUp = splitMappingKeyString(mappingkeys_keyup, QKeyMapperConstants::SPLIT_WITH_NEXT);
            Pure_MappingKeys_KeyUp = splitMappingKeyString(mappingkeys_keyup, QKeyMapperConstants::SPLIT_WITH_PLUSANDNEXT, true);
            Pure_MappingKeys_KeyUp.removeDuplicates();
        }
        Note = note;
        Category = category;
        Burst = burst;
        BurstPressTime = burstpresstime;
        BurstReleaseTime = burstreleasetime;
        Lock = lock;
        MappingKeyUnlock = mappingkeys_unlock;
        DisableOriginalKeyUnlock = disable_originalkeyunlock;
        DisableFnKeySwitch = disable_fnkeyswitch;
        PostMappingKey = postmappingkey;
        FixedVKeyCode = fixedvkeycode;
        LockState = QKeyMapperConstants::LOCK_STATE_LOCKOFF;
        CheckCombKeyOrder = checkcombkeyorder;
        Unbreakable = unbreakable;
        PassThrough = passthrough;
        SendTiming = sendtiming;
        PasteTextMode = pastetextmode;
        KeySeqHoldDown = keyseqholddown;
        RepeatMode = repeat_mode;
        RepeatTimes = repeat_times;
        Crosshair_CenterColor = crosshair_centercolor;
        Crosshair_CenterSize = crosshair_centersize;
        Crosshair_CenterOpacity = crosshair_centeropacity;
        Crosshair_CrosshairColor = crosshair_crosshaircolor;
        Crosshair_CrosshairWidth = crosshair_crosshairwidth;
        Crosshair_CrosshairLength = crosshair_crosshairlength;
        Crosshair_CrosshairOpacity = crosshair_crosshairopacity;
        Crosshair_ShowCenter = crosshair_showcenter;
        Crosshair_ShowTop = crosshair_showtop;
        Crosshair_ShowBottom = crosshair_showbottom;
        Crosshair_ShowLeft = crosshair_showleft;
        Crosshair_ShowRight = crosshair_showright;
        Crosshair_X_Offset = crosshair_x_offset;
        Crosshair_Y_Offset = crosshair_y_offset;
    }

    bool operator==(const MAP_KEYDATA& other) const
    {
        return ((Original_Key == other.Original_Key)
                && (Mapping_Keys == other.Mapping_Keys)
                && (MappingKeys_KeyUp == other.MappingKeys_KeyUp)
                && (Note == other.Note)
                && (Category == other.Category) // Compare new category field
                && (Burst == other.Burst)
                && (BurstPressTime == other.BurstPressTime)
                && (BurstReleaseTime == other.BurstReleaseTime)
                && (Lock == other.Lock)
                && (MappingKeyUnlock == other.MappingKeyUnlock)
                && (DisableOriginalKeyUnlock == other.DisableOriginalKeyUnlock)
                && (DisableFnKeySwitch == other.DisableFnKeySwitch)
                && (PostMappingKey == other.PostMappingKey)
                && (FixedVKeyCode == other.FixedVKeyCode)
                && (CheckCombKeyOrder == other.CheckCombKeyOrder)
                && (Unbreakable == other.Unbreakable)
                && (PassThrough == other.PassThrough)
                && (SendTiming == other.SendTiming)
                && (PasteTextMode == other.PasteTextMode)
                && (KeySeqHoldDown == other.KeySeqHoldDown)
                && (RepeatMode == other.RepeatMode)
                && (RepeatTimes == other.RepeatTimes)
                && (Crosshair_CenterColor == other.Crosshair_CenterColor)
                && (Crosshair_CenterSize == other.Crosshair_CenterSize)
                && (Crosshair_CenterOpacity == other.Crosshair_CenterOpacity)
                && (Crosshair_CrosshairColor == other.Crosshair_CrosshairColor)
                && (Crosshair_CrosshairWidth == other.Crosshair_CrosshairWidth)
                && (Crosshair_CrosshairLength == other.Crosshair_CrosshairLength)
                && (Crosshair_CrosshairOpacity == other.Crosshair_CrosshairOpacity)
                && (Crosshair_ShowCenter == other.Crosshair_ShowCenter)
                && (Crosshair_ShowTop == other.Crosshair_ShowTop)
                && (Crosshair_ShowBottom == other.Crosshair_ShowBottom)
                && (Crosshair_ShowLeft == other.Crosshair_ShowLeft)
                && (Crosshair_ShowRight == other.Crosshair_ShowRight)
                && (Crosshair_X_Offset == other.Crosshair_X_Offset)
                && (Crosshair_Y_Offset == other.Crosshair_Y_Offset));
    }

#ifdef DEBUG_LOGOUT_ON
    friend QDebug operator<<(QDebug debug, const MAP_KEYDATA& data)
    {
        QDebugStateSaver saver(debug);
        QString fixedvkeycodeStr = QString("0x%1").arg(QString::number(data.FixedVKeyCode, 16).toUpper(), 2, '0');
        debug.nospace() << "\nMAP_KEYDATA["
                        << "Original_Key:" << data.Original_Key
                        << ", Mapping_Keys:" << data.Mapping_Keys
                        << ", MappingKeys_KeyUp:" << data.MappingKeys_KeyUp
                        << ", Note:" << data.Note
                        << ", Category:" << data.Category
                        << ", Burst:" << data.Burst
                        << ", BurstPressTime:" << data.BurstPressTime
                        << ", BurstReleaseTime:" << data.BurstReleaseTime
                        << ", Lock:" << data.Lock
                        << ", MappingKeyUnlock:" << data.MappingKeyUnlock
                        << ", DisableOriginalKeyUnlock:" << data.DisableOriginalKeyUnlock
                        << ", DisableFnKeySwitch:" << data.DisableFnKeySwitch
                        << ", PostMappingKey:" << data.PostMappingKey
                        << ", FixedVKeyCode:" << fixedvkeycodeStr
                        << ", LockState:" << data.LockState
                        << ", CheckCombKeyOrder:" << data.CheckCombKeyOrder
                        << ", Unbreakable:" << data.Unbreakable
                        << ", PassThrough:" << data.PassThrough
                        << ", SendTiming:" << data.SendTiming
                        << ", PasteTextMode:" << data.PasteTextMode
                        << ", KeySeqHoldDown:" << data.KeySeqHoldDown
                        << ", RepeatMode:" << data.RepeatMode
                        << ", RepeatTimes:" << data.RepeatTimes
                        << ", Crosshair_CenterColor:" << data.Crosshair_CenterColor
                        << ", Crosshair_CenterSize:" << data.Crosshair_CenterSize
                        << ", Crosshair_CenterOpacity:" << data.Crosshair_CenterOpacity
                        << ", Crosshair_CrosshairColor:" << data.Crosshair_CrosshairColor
                        << ", Crosshair_CrosshairWidth:" << data.Crosshair_CrosshairWidth
                        << ", Crosshair_CrosshairLength:" << data.Crosshair_CrosshairLength
                        << ", Crosshair_CrosshairOpacity:" << data.Crosshair_CrosshairOpacity
                        << ", Crosshair_ShowCenter:" << data.Crosshair_ShowCenter
                        << ", Crosshair_ShowTop:" << data.Crosshair_ShowTop
                        << ", Crosshair_ShowBottom:" << data.Crosshair_ShowBottom
                        << ", Crosshair_ShowLeft:" << data.Crosshair_ShowLeft
                        << ", Crosshair_ShowRight:" << data.Crosshair_ShowRight
                        << ", Crosshair_X_Offset:" << data.Crosshair_X_Offset
                        << ", Crosshair_Y_Offset:" << data.Crosshair_Y_Offset
                        << "]";
        return debug;
    }
#endif
}MAP_KEYDATA_st;

struct MappingMacroData
{
    QString MappingMacro;
    QString Category;
    QString Note;
};

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
    bool isvirtual;
};

struct RecordKeyData {
    QString keystring;
    int input_type;
    qint64 elapsed_time;

    RecordKeyData() : keystring(), input_type(QKeyMapperConstants::INPUT_INIT), elapsed_time(0) {}

#ifdef DEBUG_LOGOUT_ON
    friend QDebug operator<<(QDebug debug, const RecordKeyData& data)
    {
        QDebugStateSaver saver(debug);
        QString input_type_str;
        if (QKeyMapperConstants::INPUT_INIT == data.input_type) {
			input_type_str = "INPUT_INIT";
		}
		else if (QKeyMapperConstants::INPUT_KEY_UP == data.input_type) {
			input_type_str = "INPUT_KEY_UP";
		}
		else if (QKeyMapperConstants::INPUT_KEY_DOWN == data.input_type) {
			input_type_str = "INPUT_KEY_DOWN";
		}
		else if (QKeyMapperConstants::INPUT_MOUSE_WHEEL == data.input_type) {
			input_type_str = "INPUT_MOUSE_WHEEL";
		}
		else {
			input_type_str = "UNKNOWN_INPUT_TYPE";
		}

        debug.nospace() << "\nRecordKeyData["
                        << "KeyString: " << data.keystring
                        << ", InputType: " << input_type_str
                        << ", ElapsedTime: " << data.elapsed_time << "]";
        return debug;
    }
#endif
};

struct GameControllerSensorData
{
    float gyroX;
    float gyroY;
    float gyroZ;
    float accelX;
    float accelY;
    float accelZ;
    uint64_t timestamp;
};

struct ViGEm_ReportData
{
    XUSB_REPORT xusb_report;
    QAtomicInteger<uint> custom_radius_ls;
    QAtomicInteger<uint> custom_radius_rs;

    ViGEm_ReportData()
        : xusb_report()
        , custom_radius_ls(QKeyMapperConstants::VJOY_STICK_RADIUS_MAX)
        , custom_radius_rs(QKeyMapperConstants::VJOY_STICK_RADIUS_MAX)
    {}
};

struct SendInputTaskController {
    QThreadPool *task_threadpool;
    QAtomicInt *task_stop_flag; // NOLINT(clang-analyzer-webkit.NoUncountedMemberChecker)
    QMutex *task_stop_mutex;
    QWaitCondition *task_stop_condition;
    int sendvirtualkey_state;
    int task_rowindex;
    bool task_keyup_sent;
};

struct ParsedRunCommand {
    QString cmdLine;
    bool runWait = false;
    QString workDir;
    QString systemVerb;
    int showCmd = SW_SHOWNORMAL;
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
class SendInputTask : public QRunnable
{
public:
    SendInputTask(int rowindex, const QStringList& inputKeys, int keyupdown, const QString& original_key, int sendmode, int sendvirtualkey_state, QList<MAP_KEYDATA> *keyMappingDataList = Q_NULLPTR);

    void run() override;

public:
    static void initSendInputTaskControllerMap(void);
    static void clearSendInputTaskControllerMap(void);
    static void clearSendInputTaskControllerThreadPool(void);

public:
    static QMutex s_SendInputTaskControllerMapMutex;
    static QHash<QString, SendInputTaskController> s_SendInputTaskControllerMap;
    static SendInputTaskController s_GlobalSendInputTaskController;

public:
    int m_rowindex;
    QStringList m_inputKeys;
    int m_keyupdown;
    QString m_original_key;
    QString m_real_originalkey;
    int m_sendmode;
    int m_sendvirtualkey_state;

    // Save mapping table pointer to avoid array bounds issues during tab switching
    QList<MAP_KEYDATA> *m_keyMappingDataList = Q_NULLPTR;
};

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
        JOYSTICK_BUTTON_20,
        JOYSTICK_BUTTON_21,
        JOYSTICK_BUTTON_22,
        JOYSTICK_BUTTON_23,
        JOYSTICK_BUTTON_24,
        JOYSTICK_BUTTON_25,
        JOYSTICK_BUTTON_26,
        JOYSTICK_BUTTON_27
    };
    Q_ENUM(JoystickButtonCode)

    enum GameControllerButtonCode
    {
        CONTROLLER_BUTTON_INVALID = -1,
        CONTROLLER_BUTTON_A,
        CONTROLLER_BUTTON_B,
        CONTROLLER_BUTTON_X,
        CONTROLLER_BUTTON_Y,
        CONTROLLER_BUTTON_BACK,
        CONTROLLER_BUTTON_GUIDE,
        CONTROLLER_BUTTON_START,
        CONTROLLER_BUTTON_LEFTSTICK,
        CONTROLLER_BUTTON_RIGHTSTICK,
        CONTROLLER_BUTTON_LEFTSHOULDER,
        CONTROLLER_BUTTON_RIGHTSHOULDER,
        CONTROLLER_BUTTON_DPAD_UP,
        CONTROLLER_BUTTON_DPAD_DOWN,
        CONTROLLER_BUTTON_DPAD_LEFT,
        CONTROLLER_BUTTON_DPAD_RIGHT,
        CONTROLLER_BUTTON_MISC1,    /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button */
        CONTROLLER_BUTTON_PADDLE1,  /* Xbox Elite paddle P1 (upper left, facing the back) */
        CONTROLLER_BUTTON_PADDLE2,  /* Xbox Elite paddle P3 (upper right, facing the back) */
        CONTROLLER_BUTTON_PADDLE3,  /* Xbox Elite paddle P2 (lower left, facing the back) */
        CONTROLLER_BUTTON_PADDLE4,  /* Xbox Elite paddle P4 (lower right, facing the back) */
        CONTROLLER_BUTTON_TOUCHPAD, /* PS4/PS5 touchpad button */
        CONTROLLER_BUTTON_MAX
    };
    Q_ENUM(GameControllerButtonCode)

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

#ifdef FAKERINPUT_SUPPORT
    enum FakerInputClient_ConnectState
    {
        FAKERINPUT_DISCONNECTED = 0,
        FAKERINPUT_CONNECT_FAILED,
        FAKERINPUT_CONNECTING,
        FAKERINPUT_CONNECT_SUCCESS,
    };
    Q_ENUM(FakerInputClient_ConnectState)
#endif

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

    enum Joy2vJoyTriggerState
    {
        JOY2VJOY_TRIGGER_NONE   = 0,
        JOY2VJOY_TRIGGER_LT     = 1,
        JOY2VJOY_TRIGGER_RT     = 2,
        JOY2VJOY_TRIGGER_LTRT_BOTH = JOY2VJOY_TRIGGER_LT | JOY2VJOY_TRIGGER_RT
    };
    Q_ENUM(Joy2vJoyTriggerState)
    Q_DECLARE_FLAGS(Joy2vJoyTriggerStates, Joy2vJoyTriggerState)

    enum Joy2vJoyLeftStickState
    {
        JOY2VJOY_LS_NONE    = 0,
        JOY2VJOY_LS_2LS     = 1,
        JOY2VJOY_LS_2RS     = 2,
        JOY2VJOY_LS_2LSRS_BOTH = JOY2VJOY_LS_2LS | JOY2VJOY_LS_2RS
    };
    Q_ENUM(Joy2vJoyLeftStickState)
    Q_DECLARE_FLAGS(Joy2vJoyLeftStickStates, Joy2vJoyLeftStickState)

    enum Joy2vJoyRightStickState
    {
        JOY2VJOY_RS_NONE    = 0,
        JOY2VJOY_RS_2LS     = 1,
        JOY2VJOY_RS_2RS     = 2,
        JOY2VJOY_RS_2LSRS_BOTH = JOY2VJOY_RS_2LS | JOY2VJOY_RS_2RS
    };
    Q_ENUM(Joy2vJoyRightStickState)
    Q_DECLARE_FLAGS(Joy2vJoyRightStickStates, Joy2vJoyRightStickState)

    struct Joy2vJoyState {
        int gamepad_index;
        Joy2vJoyTriggerStates trigger_state;
        Joy2vJoyLeftStickStates ls_state;
        Joy2vJoyRightStickStates rs_state;

#ifdef DEBUG_LOGOUT_ON
        friend QDebug operator<<(QDebug debug, const Joy2vJoyState& state) {
            QDebugStateSaver saver(debug);
            debug.nospace() << "Joy2vJoyState("
                            << "gamepad_index:" << state.gamepad_index
                            << ", trigger_state:" << state.trigger_state
                            << ", ls_state:" << state.ls_state
                            << ", rs_state:" << state.rs_state
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
#endif

    enum Joy2MouseState
    {
        JOY2MOUSE_NONE      = 0,
        JOY2MOUSE_LEFT      = 1,
        JOY2MOUSE_RIGHT     = 2,
        JOY2MOUSE_BOTH      = JOY2MOUSE_LEFT | JOY2MOUSE_RIGHT
    };
    Q_ENUM(Joy2MouseState)
    Q_DECLARE_FLAGS(Joy2MouseStates, Joy2MouseState)

public slots:
    void sendKeyboardInput(V_KEYCODE vkeycode, int keyupdown);
    void sendMouseClick(V_MOUSECODE vmousecode, int keyupdown);
private:
    HWND findFocusedEditControl(HWND window_hwnd);
    bool verifyClipboardContent(const QString& expectedText, bool fullCompare = false);
    bool setClipboardTextWithVerification(const QString& text, int maxRetries = 3);
    bool restoreClipboardWithVerification(HGLOBAL hOriginalDataCopy, int maxRetries = 3);
    void pasteWithShiftIns(void);
    void pasteWithCtrlV(void);
public:
    void postVirtualKeyCode(HWND hwnd, uint keycode, int keyupdown);
    void sendUnicodeChar(wchar_t aChar);
    void sendText(HWND window_hwnd, const QString& text);
    void pasteText(HWND window_hwnd, const QString& text, int mode = QKeyMapperConstants::PASTETEXT_MODE_SHIFTINSERT);
    void sendWindowMousePointClick(HWND hwnd, const QString &mousebutton, int keyupdown, const QPoint &mousepoint = QPoint(-1, -1));
    void sendWindowMouseMoveToPoint(HWND hwnd, const QPoint &mousepoint = QPoint(-1, -1));
    void postMouseButton(HWND hwnd, const QString &mousebutton, int keyupdown, const QPoint &mousepoint = QPoint(-1, -1));

    // Volume control functions
    void processSetVolumeMapping(const QString& volumeCommand);
    void postMouseWheel(HWND hwnd, const QString &mousewheel);
    void postMouseMove(HWND hwnd, int delta_x, int delta_y);
    void postMouseMoveToPoint(HWND hwnd, const QPoint &mousepoint = QPoint(-1, -1));
    void sendMouseMove(int delta_x, int delta_y);
    void sendMouseWheel(int wheel_updown);
    void setMouseToScreenCenter(void);
    void setMouseToPoint(POINT point);
    void setMouseToScreenBottomRight(void);
    POINT mousePositionAfterSetMouseToScreenBottomRight(void);
public slots:
    void onMouseMove(int delta_x, int delta_y, int mouse_index);
    // void onMouse2vJoyResetTimeout(void);
public:
    void initMouse2vJoyResetTimerMap(void);
    void stopMouse2vJoyResetTimerMap(void);
    void onMouse2vJoyResetTimeoutForMap(int mouse_index);
public slots:
    void onKey2MouseCycleTimeout(void);
    void onMouseWheel(int wheel_updown);
    void onSendInputKeys(int rowindex, QStringList inputKeys, int keyupdown, QString original_key, int sendmode, int sendvirtualkey_state, QList<MAP_KEYDATA> *keyMappingDataList);
public:
    void sendInputKeys(int rowindex, QStringList inputKeys, int keyupdown, QString original_key, int sendmode, SendInputTaskController controller, QList<MAP_KEYDATA> *keyMappingDataList = Q_NULLPTR);
    // void send_WINplusD(void);
    void sendMousePointClick(QString &mousepoint_str, int keyupdown, bool postmappingkey);
    void sendMouseMoveToPoint(QString &mousepoint_str, bool postmappingkey);
    void saveMousePosition(void);
    void restoreMousePosition(void);
    void emit_sendInputKeysSignal_Wrapper(int rowindex, QStringList &inputKeys, int keyupdown, QString &original_key_unchanged, int sendmode, int sendvirtualkey_state = QKeyMapperConstants::SENDVIRTUALKEY_STATE_NORMAL, QList<MAP_KEYDATA> *keyMappingDataList = Q_NULLPTR);

public:
    static void sendBurstKeyDown(const QString &burstKey, bool start);
    static void sendBurstKeyUp(const QString &burstKey, bool stop);
    static void sendBurstKeyDown(int findindex, bool start, QList<MAP_KEYDATA> *keyMappingDataList);
    static void sendBurstKeyUp(int findindex, bool stop, QList<MAP_KEYDATA> *keyMappingDataList);
    void sendBurstKeyUpForce(int findindex);
    void sendSpecialVirtualKey(const QString &keycodeString, int keyupdown);
    void sendSpecialVirtualKeyDown(const QString &virtualKey);
    void sendSpecialVirtualKeyUp(const QString &virtualKey);

    void startDataPortListener(void);
    void stopDataPortListener(void);

    void initGamepadMotion(void);
    void setGamepadMotionAutoCalibration(bool enabled, float gyroThreshold, float accelThreshold);

#ifdef FAKERINPUT_SUPPORT
public:
    static int FakerInputClient_Alloc(void);
    static int FakerInputClient_Connect(void);
    static void FakerInputClient_Disconnect(void);
    static void FakerInputClient_Free(void);
    static FakerInputClient_ConnectState FakerInputClient_getConnectState(void);
    static void FakerInputClient_setConnectState(FakerInputClient_ConnectState connectstate);
    static bool FakerInputClient_sendKeyboardInput(quint8 vkeycode, bool extendedFlag, int keyupdown);
    static BYTE VirtualKeyCodeToHIDUsageCode(quint8 vkeycode);
    static BYTE VirtualKeyCodeToHIDModifierFlag(quint8 vkeycode);
    static void updateFakerInputStatus(void);
private:
    static void initVK2HIDCodeMap(void);
#endif

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

    static void ViGEmClient_PressButton(const QString &joystickButton, int autoAdjust, int gamepad_index, int player_index, QJoystickEventType event_type = GameControllerEvent);
    static void ViGEmClient_ReleaseButton(const QString &joystickButton, int gamepad_index);
    static void ViGEmClient_CheckJoysticksReportData(int gamepad_index);
    static void ViGEmClient_CalculateThumbValue(SHORT* ori_ThumbX, SHORT* ori_ThumbY, uint custom_radius = QKeyMapperConstants::VJOY_STICK_RADIUS_MAX);

    // static Mouse2vJoyStates ViGEmClient_checkMouse2JoystickEnableState(void);
    static QHash<int, Mouse2vJoyData> ViGEmClient_checkMouse2JoystickEnableStateMap(void);
    void ViGEmClient_Mouse2JoystickUpdate(int delta_x, int delta_y, int mouse_index, int gamepad_index);
    void ViGEmClient_Joy2vJoystickUpdate(const Joy2vJoyState &joy2vjoystate, int sticktype, int gamepad_index, int player_index);
    // void ViGEmClient_GamepadReset(void);
    void ViGEmClient_AllGamepadReset(void);
    static void ViGEmClient_GamepadReset_byIndex(int gamepad_index);
    void ViGEmClient_JoysticksReset(int mouse_index, int gamepad_index);
#endif

signals:
    void setKeyHook_Signal(void);
    void setKeyUnHook_Signal(void);
    void setKeyMappingRestart_Signal(void);
    void allMappingKeysReleased_Signal(void);
    void sessionLockStateChanged_Signal(bool locked);
    // void startBurstTimer_Signal(const QString &burstKey, int mappingIndex);
    // void stopBurstTimer_Signal(const QString &burstKey, int mappingIndex);
    void startBurstKeyTimer_Signal(const QString &burstKey, int mappingIndex, QList<MAP_KEYDATA> *keyMappingDataList);
    void stopBurstKeyTimer_Signal(const QString &burstKey, int mappingIndex, QList<MAP_KEYDATA> *keyMappingDataList);
#if 0
    void sendKeyboardInput_Signal(V_KEYCODE vkeycode, int keyupdown);
    void sendMouseClick_Signal(V_MOUSECODE vmousecode, int keyupdown);
#endif
    void sendInputKeys_Signal(int rowindex, QStringList inputKeys, int keyupdown, QString original_key, int sendmode, int sendvirtualkey_state, QList<MAP_KEYDATA> *keyMappingDataList);
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

    void gameControllerGyroEnabledSwitch_Signal(int gamepadinfo_index);

// protected:
//     void timerEvent(QTimerEvent *event) override;

public slots:
    void threadStarted(void);
    void threadFinished(void);
    void setWorkerKeyHook(void);
    void setWorkerKeyUnHook(void);
    void setKeyMappingRestart(void);
    void allMappingKeysReleased(void);
    void sessionLockStateChanged(bool locked);
    void setWorkerJoystickCaptureStart(void);
    void setWorkerJoystickCaptureStop(void);
    // void HotKeyHookProc(const QString &keycodeString, int keyupdown);
    QKeyMapper_Worker::GripDetectStates checkGripDetectEnableState(void);
    // Joy2vJoyState checkJoy2vJoyState(void);
    QHash<int, QKeyMapper_Worker::Joy2vJoyState> checkJoy2vJoyEnableStateMap(void);
    void processUdpPendingDatagrams(void);
    void processForzaFormatData(const QByteArray &forzadata);

#ifdef DINPUT_TEST
    void setWorkerDInputKeyHook(HWND hWnd);
    void setWorkerDInputKeyUnHook(void);
#endif
    // void startBurstTimer(const QString &burstKey, int mappingIndex);
    // void stopBurstTimer(const QString &burstKey, int mappingIndex);
    // void onJoystickcountChanged(void);
    void onJoystickAdded(QJoystickDevice *joystick_added);
    void onJoystickRemoved(const QJoystickDevice joystick_removed);
    void onJoystickPOVEvent(const QJoystickPOVEvent &e);
    void onJoystickAxisEvent(const QJoystickAxisEvent &e);
    void onJoystickButtonEvent(const QJoystickButtonEvent &e);
    void onJoystickSensorEvent(const QJoystickSensorEvent &e);
    void onJoystickBatteryEvent(const QJoystickBatteryEvent &e);

    void onGameControllerGyroEnabledSwitch(int gamepadinfo_index);

    void checkJoystickButtons(const QJoystickButtonEvent &e);
    void checkJoystickPOV(const QJoystickPOVEvent &e);
    void checkJoystickAxis(const QJoystickAxisEvent &e);
    void checkJoystickSensor(const QJoystickSensorEvent &e);

    void startMouse2vJoyResetTimer(const QString &mouse2joy_keystr, int mouse_index_param);
    void stopMouse2vJoyResetTimer(const QString &mouse2joy_keystr, int mouse_index_param);
    // Joy2MouseStates checkJoystick2MouseEnableState(void);
    QHash<int, QKeyMapper_Worker::Joy2MouseStates> checkJoy2MouseEnableStateMap(void);
    bool checkKey2MouseEnableState(void);
    bool checkGyro2MouseEnableState(void);
    bool checkGyro2MouseMoveActiveState(void);
    void doFunctionMappingProc(const QString &func_keystring);

    // FilterKeys management functions
    void handleFilterKeysOnMappingStart();
    void handleFilterKeysOnMappingStop();
    void notifyUserChangedFilterKeys();

private:
    void joystickLTRTButtonProc(const QJoystickAxisEvent &e);
    void joystickLSHorizontalProc(const QJoystickAxisEvent &e);
    void joystickLSVerticalProc(const QJoystickAxisEvent &e);
    void joystickRSHorizontalProc(const QJoystickAxisEvent &e);
    void joystickRSVerticalProc(const QJoystickAxisEvent &e);
    int  joystickCalculateDelta(qreal axis_value, int Speed_Factor, bool checkJoystick);
    void joystick2MouseMoveProc(int player_index);
    void key2MouseMoveProc(void);
    void gyro2MouseMoveProc(const GameControllerSensorData &sensor_data);

public:
    static ULONG_PTR generateUniqueRandomValue(QSet<ULONG_PTR>& existingValues);
    static void generateVirtualInputRandomValues(void);

    static int InterceptionKeyboardHookProc(UINT scan_code, int keyupdown, ULONG_PTR extra_info, bool ExtenedFlag_e0, bool ExtenedFlag_e1, int keyboard_index);
    static int InterceptionMouseHookProc(MouseEvent mouse_event, int delta_x, int delta_y, short delta_wheel, unsigned short flags, ULONG_PTR extra_info, int mouse_index);
    static LRESULT CALLBACK LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    static int hookBurstAndLockProc(const QString &keycodeString, int keyupdown);
    static int updatePressedRealKeysList(const QString &keycodeString, int keyupdown);
    static void keyRecordStart(void);
    static void keyRecordStop(void);
    static void collectRecordKeysList(bool clicked);
    static bool updateRecordKeyList(const QString &keycodeString, int input_type);
    static bool detectDisplaySwitchKey(const QString &keycodeString, int keyupdown);
    static bool detectMappingSwitchKey(const QString &keycodeString, int keyupdown);
    static bool detectMappingStartKey(const QString &keycodeString, int keyupdown);
    static bool detectMappingStopKey(const QString &keycodeString, int keyupdown);
    static bool detectMappingTableTabHotkeys(const QString &keycodeString, int keyupdown);
    static int detectCombinationKeys(const QString &keycodeString, int keyupdown);
    static bool isKeyOrderIncreasing(const QList<int>& keyorder);
    static void CombinationKeyProc(int rowindex, const QString &keycodeString, int keyupdown);
    static bool releaseKeyboardModifiers(const Qt::KeyboardModifiers modifiers, const QString &original_key = QKeyMapperConstants::KEYBOARD_MODIFIERS,const QStringList &mappingkeyslist = QStringList());
    static bool releaseKeyboardModifiersDirect(const Qt::KeyboardModifiers modifiers);
    static void startBurstKeyTimer(const QString &burstKey, int mappingIndex, QList<MAP_KEYDATA> *keyMappingDataList);
    static void stopBurstKeyTimer(const QString &burstKey, int mappingIndex, QList<MAP_KEYDATA> *keyMappingDataList);
    void stopBurstKeyTimerForce(const QString &burstKey, int mappingIndex);
    static void resendRealKeyCodeOnStop(int rowindex, bool restart = false, QList<MAP_KEYDATA> *keyMappingDataListToCheck = Q_NULLPTR);

    static void collectBlockedKeysList(void);
    static QStringList collectCertainMappingDataListBlockedKeysList(QList<MAP_KEYDATA> *keyMappingDataListToCheck);
    static void collectCombinationOriginalKeysList(void);
    static void collectLongPressOriginalKeysMap(void);
    static QHash<QString, int> currentLongPressOriginalKeysMap(void);
    static void sendLongPressTimers(const QString &keycodeString);
    static void clearLongPressTimer(const QString &keycodeString);
    static void removeLongPressTimerOnTimeout(const QString &keycodeStringWithPressTime);
    static void clearAllLongPressTimers(void);
    static int longPressKeyProc(const QString &keycodeString, int keyupdown);
    static void collectDoublePressOriginalKeysMap(void);
    static QHash<QString, int> currentDoublePressOriginalKeysMap(void);
    static int sendDoublePressTimers(const QString &keycodeString);
    static void clearDoublePressTimer(const QString &keycodeString);
    static void removeDoublePressTimerOnTimeout(const QString &keycodeString);
    static void clearAllDoublePressTimers(void);
    static int doublePressKeyProc(const QString &keycodeString, int keyupdown);
    static QString getWindowsKeyName(uint virtualKeyCode);
    static QString getKeycodeStringRemoveMultiInput(const QString &keycodeString);
    static void breakAllRunningKeySequence(void);

    static ParsedRunCommand parseRunCommandUserInput(const QString &input);
    // Run an external command with optional wait, working directory, window show mode, and verb.
    // Parameters:
    //   cmdLine  - Command line string (program + arguments, URL, CLSID, etc.)
    //   runWait  - If true, wait until the process finishes
    //   workDir  - Working directory (empty string = current directory)
    //   showCmd  - Window show mode (SW_SHOWNORMAL, SW_SHOWMAXIMIZED, SW_SHOWMINIMIZED, SW_HIDE)
    //   verb     - Shell verb (e.g., "open", "edit", "print"), empty = default verb
    static bool runCommand(const QString &cmdLine,
                           bool runWait = false,
                           const QString &workDir = QString(),
                           int showCmd = SW_SHOWNORMAL,
                           const QString &verb = QString());

public slots:
    static void onLongPressTimeOut(const QString keycodeStringWithPressTime);
    static void onDoublePressTimeOut(const QString keycodeString);
    static void onBurstKeyPressTimeOut(const QString burstKey, int mappingIndex, QList<MAP_KEYDATA> *keyMappingDataList);
    static void onBurstKeyTimeOut(const QString burstKey, int mappingIndex, QList<MAP_KEYDATA> *keyMappingDataList);

private:
    bool JoyStickKeysProc(QString keycodeString, int keyupdown, const QJoystickDevice *joystick);
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
    void initSpecialOriginalKeysList(void);
    void initSpecialMappingKeysList(void);
    void initSpecialVirtualKeyCodeList(void);
    // void initSkipReleaseModifiersKeysList(void);
#ifdef VIGEM_CLIENT_SUPPORT
    void initViGEmKeyMap(void);
    bool isCursorAtBottomRight(void);
#endif
    void clearAllBurstKeyTimersAndLockKeys(void);
    void clearAllPressedVirtualKeys(void);
    void clearPressedVirtualKeysOfMappingKeys(const QString &mappingkeys);
    void clearAllNormalPressedMappingKeys(bool restart = false, QList<MAP_KEYDATA> *keyMappingDataListToCheck = Q_NULLPTR);
    void clearCustomKeyFlags(bool restart = false);
    void clearAllPressedRealCombinationKeys(void);
    void collectExchangeKeysList(void);
    bool isPressedMappingKeysContains(QString &key);
    void initGlobalSendInputTaskController(void);
    void resetGlobalSendInputTaskController(void);
    void clearGlobalSendInputTaskController(void);
    void clearGlobalSendInputTaskControllerThreadPool(void);
    void releasePressedRealKeysOfOriginalKeys(void);
    void emitSendOnMappingStartKeys(void);
    void emitSendOnSwitchTabKeys(void);

public:
#if 0
    int makeKeySequenceInputarray(QStringList &keyseq_list, INPUT *input_array);
#endif
    void sendKeySequenceList(QStringList &keyseq_list, QString &original_key, int sendmode, int sendvirtualkey_state);

public:
    static ULONG_PTR VIRTUAL_KEY_SEND;
    static ULONG_PTR VIRTUAL_KEY_SEND_NORMAL;
    static ULONG_PTR VIRTUAL_KEY_SEND_FORCE;
    static ULONG_PTR VIRTUAL_KEY_SEND_MODIFIERS;
    static ULONG_PTR VIRTUAL_KEY_SEND_BURST_TIMEOUT;
    static ULONG_PTR VIRTUAL_KEY_SEND_BURST_STOP;
    static ULONG_PTR VIRTUAL_KEY_SEND_KEYSEQ_NORMAL;
    static ULONG_PTR VIRTUAL_KEY_SEND_KEYSEQ_HOLDDOWN;
    static ULONG_PTR VIRTUAL_KEY_SEND_KEYSEQ_REPEAT;
    static ULONG_PTR VIRTUAL_MOUSE_POINTCLICK;
    static ULONG_PTR VIRTUAL_MOUSE_WHEEL;
    static ULONG_PTR VIRTUAL_KEY_OVERLAY;
    static ULONG_PTR VIRTUAL_RESEND_REALKEY;
    static bool s_isWorkerDestructing;
    static QAtomicInt s_AtomicHookProcState;
    static QAtomicBool s_BlockKeyboard;
    static QAtomicBool s_BlockMouse;
    static QAtomicBool s_Mouse2vJoy_Hold;
    static QAtomicBool s_Gyro2Mouse_MoveActive;
    static QAtomicBool s_Crosshair_Normal;
    static QAtomicBool s_Crosshair_TypeA;
    static QAtomicBool s_Key2Mouse_Up;
    static QAtomicBool s_Key2Mouse_Down;
    static QAtomicBool s_Key2Mouse_Left;
    static QAtomicBool s_Key2Mouse_Right;
    static QAtomicBool s_KeyRecording;
    static QAtomicBool s_RestoreFilterKeysState;
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
    static QStringList SpecialOriginalKeysList;
    static QStringList SendOnOriginalKeysList;
    static QStringList SpecialMappingKeysList;
    static QList<quint8> SpecialVirtualKeyCodeList;
    // static QStringList skipReleaseModifiersKeysList;
    // static QHash<QString, int> JoyStickKeyMap;
    // static QHash<QString, QHotkey*> ShortcutsMap;
#ifdef VIGEM_CLIENT_SUPPORT
    static QHash<QString, XUSB_BUTTON> ViGEmButtonMap;
#endif
    static QStringList pressedRealKeysList;
    static QStringList pressedRealKeysListRemoveMultiInput;
    static QList<RecordKeyData> recordKeyList;
    static QStringList recordMappingKeysList;
    static QElapsedTimer recordElapsedTimer;
    // static QStringList pressedCombinationRealKeysList;
    static QStringList pressedVirtualKeysList;
    static QStringList pressedLongPressKeysList;
    static QStringList pressedDoublePressKeysList;
    static QList<QList<quint8>> pressedMultiKeyboardVKeyCodeList;
    static QStringList s_runningKeySequenceOrikeyList;
    // static QStringList pressedShortcutKeysList;
    static QStringList combinationOriginalKeysList;
    static QStringList blockedKeysList;
    static QHash<QString, QList<int>> longPressOriginalKeysMap;
    static QHash<QString, QTimer*> s_longPressTimerMap;
    static QHash<QString, int> doublePressOriginalKeysMap;
    static QHash<QString, QTimer*> s_doublePressTimerMap;
    static QHash<QString, QTimer*> s_BurstKeyTimerMap;
    static QHash<QString, QTimer*> s_BurstKeyPressTimerMap;
    static QHash<QString, int> s_KeySequenceRepeatCount;
#ifdef VIGEM_CLIENT_SUPPORT
    static QList<OrderedMap<QString, BYTE>> pressedvJoyLStickKeysList;
    static QList<OrderedMap<QString, BYTE>> pressedvJoyRStickKeysList;
    static QList<QStringList> pressedvJoyButtonsList;
#endif
    static QHash<QString, QStringList> pressedMappingKeysMap;
    static QMutex s_PressedMappingKeysMapMutex;
    static QHash<QString, int> pressedLockKeysMap;
    static QStringList exchangeKeysList;
    static QMutex s_BurstKeyTimerMutex;
#ifdef DINPUT_TEST
    static GetDeviceStateT FuncPtrGetDeviceState;
    static GetDeviceDataT FuncPtrGetDeviceData;
    static int dinput_timerid;
#endif
#ifdef FAKERINPUT_SUPPORT
    static pfakerinput_client s_FakerInputClient;
    static FakerInputClient_ConnectState s_FakerInputClient_ConnectState;
    static QMutex s_FakerInputClient_Mutex;
    static BYTE s_FakerInputKeyboardReport_ShiftFlags;
    static BYTE s_FakerInputKeyboardReport_KeyCodes[KBD_KEY_CODES];
    static QHash<quint8, BYTE> s_VK2HIDCodeMap;
    static QHash<quint8, BYTE> s_VK2HIDModifierMap;
#endif
#ifdef VIGEM_CLIENT_SUPPORT
    static PVIGEM_CLIENT s_ViGEmClient;
    // static PVIGEM_TARGET s_ViGEmTarget;
    static QList<PVIGEM_TARGET> s_ViGEmTargetList;
    static ViGEmClient_ConnectState s_ViGEmClient_ConnectState;
    // static XUSB_REPORT s_ViGEmTarget_Report;
    static QList<ViGEm_ReportData> s_ViGEmTarget_ReportList;
    static QStringList s_VirtualGamepadList;
    static BYTE s_Auto_Brake;
    static BYTE s_Auto_Accel;
    static BYTE s_last_Auto_Brake;
    static BYTE s_last_Auto_Accel;
    static GripDetectStates s_GripDetect_EnableState;
    // static Joy2vJoyState s_Joy2vJoyState;
    static QHash<int, Joy2vJoyState> s_Joy2vJoy_EnableStateMap;
    static QMutex s_ViGEmClient_Mutex;
    static QPoint s_Mouse2vJoy_delta;
    static QPoint s_Mouse2vJoy_prev;
    // static QList<QPoint> s_Mouse2vJoy_delta_List;
    // static QPoint s_Mouse2vJoy_delta_interception;
    // static Mouse2vJoyStates s_Mouse2vJoy_EnableState;
    static QHash<int, Mouse2vJoyData> s_Mouse2vJoy_EnableStateMap;
    // static QMutex s_MouseMove_delta_List_Mutex;
#endif

    static bool s_Key2Mouse_EnableState;
    static bool s_GameControllerSensor_EnableState;
    // static Joy2MouseStates s_Joy2Mouse_EnableState;
    static QHash<int, Joy2MouseStates> s_Joy2Mouse_EnableStateMap;
    // static Joystick_AxisState s_JoyAxisState;
    static QHash<int, Joystick_AxisState> s_JoyAxisStateMap;
    static int s_LastJoyAxisPlayerIndex;
    static QPoint s_SavedMousePosition;

private:
    HHOOK m_KeyHook;
    HHOOK m_MouseHook;
#ifdef VIGEM_CLIENT_SUPPORT
    POINT m_LastMouseCursorPoint;
#endif
    bool m_JoystickCapture;
#ifdef DINPUT_TEST
    IDirectInput8* m_DirectInput;
#endif
#ifdef VIGEM_CLIENT_SUPPORT
    QHash<int, QTimer*> m_Mouse2vJoyResetTimerMap;
#endif
    QTimer m_Key2MouseCycleTimer;
    QUdpSocket *m_UdpSocket;
//     QHash<QString, int> m_BurstTimerMap;
//     QHash<QString, int> m_BurstKeyUpTimerMap;
// #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
//     QRecursiveMutex m_BurstTimerMutex;
// #else
//     QMutex m_BurstTimerMutex;
// #endif
    QHash<JoystickButtonCode, QString> m_JoystickButtonMap;
    QHash<GameControllerButtonCode, QString> m_ControllerButtonMap;
    QHash<JoystickDPadCode, QString> m_JoystickDPadMap;
    QHash<JoystickLStickCode, QString> m_JoystickLStickMap;
    QHash<JoystickRStickCode, QString> m_JoystickRStickMap;
    QHash<int, JoystickDPadCode> m_JoystickPOVMap;
    GamepadMotion m_GamdpadMotion;
    bool m_ComInitialized = false;

    // Volume control instance
    VolumeController m_volumeController;
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
    void setKeyHook_Signal(void);
    void setKeyUnHook_Signal(void);
    void setKeyMappingRestart_Signal(void);

public slots:
    void HookProcThreadStarted(void);
    void HookProcThreadFinished(void);
    void onSetHookProcKeyHook();
    void onSetHookProcKeyUnHook(void);
    void onSetHookProcKeyMappingRestart(void);

public:
    static bool s_LowLevelKeyboardHook_Enable;
    static bool s_LowLevelMouseHook_Enable;

    static HHOOK s_KeyHook;
    static HHOOK s_MouseHook;
};

bool EnablePrivilege(LPCWSTR privilege);
bool DisablePrivilege(LPCWSTR privilege);

#endif // QKEYMAPPER_WORKER_H
