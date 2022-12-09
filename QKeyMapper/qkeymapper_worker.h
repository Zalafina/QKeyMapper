#ifndef QKEYMAPPER_WORKER_H
#define QKEYMAPPER_WORKER_H

#include <windows.h>
#include <QObject>
#include <QThread>
#include <QHash>
#include <QMutex>

class QKeyMapper;

#define EXTENED_FLAG_TRUE   true
#define EXTENED_FLAG_FALSE  false

#define SEPARATOR_STR   (" + ")

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
        Mapping_Keys = mappingkeys.split(SEPARATOR_STR);
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

    V_MOUSECODE() : MouseDownCode(0x0000), MouseUpCode(0x0000) {}

    V_MOUSECODE(DWORD mousedowncode, DWORD mouseupcode)
    {
        MouseDownCode = mousedowncode;
        MouseUpCode = mouseupcode;
    }

    bool operator==(const V_MOUSECODE& other) const
    {
        return ((MouseDownCode == other.MouseDownCode)
                && (MouseUpCode == other.MouseUpCode));
    }
}V_MOUSECODE_st;

class QKeyMapper_Worker : public QObject
{
    Q_OBJECT
public:
    explicit QKeyMapper_Worker(QObject *parent = nullptr);
    ~QKeyMapper_Worker();

    static QKeyMapper_Worker *getInstance()
    {
        if(m_instance == Q_NULLPTR)
            m_instance = new QKeyMapper_Worker();
        return m_instance;
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

public slots:
    void sendKeyboardInput(V_KEYCODE vkeycode, int keyupdown);
    void sendMouseInput(V_MOUSECODE vmousecode, int keyupdown);
    void sendInputKeys(QStringList inputKeys, int keyupdown, QString original_key, int sendmode);

public:
    void sendBurstKeyDown(const QString &burstKey);
    void sendBurstKeyUp(const QString &burstKey, bool stop);
    void sendSpecialVirtualKeyDown(const QString &virtualKey);
    void sendSpecialVirtualKeyUp(const QString &virtualKey);

signals:
    void setKeyHook_Signal(HWND hWnd);
    void setKeyUnHook_Signal(void);
    void startBurstTimer_Signal(const QString &burstKey, int mappingIndex);
    void stopBurstTimer_Signal(const QString &burstKey, int mappingIndex);
    void sendKeyboardInput_Signal(V_KEYCODE vkeycode, int keyupdown);
    void sendMouseInput_Signal(V_MOUSECODE vmousecode, int keyupdown);
    void sendInputKeys_Signal(QStringList inputKeys, int keyupdown, QString original_key, int sendmode);

protected:
    void timerEvent(QTimerEvent *event) override;

public slots:
    void threadStarted(void);
    void setWorkerKeyHook(HWND hWnd);
    void setWorkerKeyUnHook(void);
    void startBurstTimer(const QString &burstKey, int mappingIndex);
    void stopBurstTimer(const QString &burstKey, int mappingIndex);

private:
    static LRESULT CALLBACK LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    void initVirtualKeyCodeMap(void);
    void initVirtualMouseButtonMap(void);
    void clearAllBurstTimersAndLockKeys(void);

public:
    static QHash<QString, V_KEYCODE> VirtualKeyCodeMap;
    static QHash<QString, V_MOUSECODE> VirtualMouseButtonMap;
    static QHash<WPARAM, QString> MouseButtonNameMap;
    static QStringList pressedRealKeysList;
    static QStringList pressedVirtualKeysList;
    static QStringList pressedLockKeysList;

    static QMutex *sendinput_mutex;

private:
    static QKeyMapper_Worker *m_instance;
    HHOOK m_KeyHook;
#ifdef SUPPORT_MOUSE_LL_HOOK
    HHOOK m_MouseHook;
#endif
    QHash<QString, int> m_BurstTimerMap;
    QHash<QString, int> m_BurstKeyUpTimerMap;
};

#endif // QKEYMAPPER_WORKER_H
