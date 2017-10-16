#ifndef QLINKKEEPER_H
#define QLINKKEEPER_H

#include <QDialog>
#include <QDebug>
#include <QMetaEnum>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QHash>
#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <tchar.h>
#include <string.h>
//#include <QKeyEvent>
//#include <QProcess>
//#include <QTextCodec>

namespace Ui {
class QKeyMapper;
}

typedef struct
{
    QString FileName;
    QString PID;
    QString WindowTitle;
    QString FilePath;
}MAP_PROCESSINFO;

typedef struct MAP_KEYDATA
{
    QString Original_Key;
    QString Mapping_Key;

    MAP_KEYDATA() : Original_Key(), Mapping_Key() {}

    MAP_KEYDATA(QString originalkey, QString mappingkey)
    {
        Original_Key = originalkey;
        Mapping_Key = mappingkey;
    }

    bool operator==(const MAP_KEYDATA& other) const
    {
        return ((Original_Key == other.Original_Key)
                && (Mapping_Key == other.Mapping_Key));
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

class QKeyMapper : public QDialog
{
    Q_OBJECT

public:
    explicit QKeyMapper(QWidget *parent = 0);
    ~QKeyMapper();

    #define EXTENED_FLAG_TRUE   true
    #define EXTENED_FLAG_FALSE  false

    enum KeyMapStatus
    {
        KEYMAP_IDLE = 0U,
        KEYMAP_CHECKING,
        KEYMAP_MAPPING
    };
    Q_ENUM(KeyMapStatus)

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

    Q_INVOKABLE void WindowStateChangedProc(void);
    Q_INVOKABLE void cycleCheckProcessProc(void);

    void setKeyHook(void);
    void setKeyUnHook(void);

    void setMapProcessInfo(QString &filename, QString &windowtitle, QString &pid, QString &filepath);
    static void getProcessInfoFromPID(DWORD processID, QString &processPathStr);
    static void getProcessInfoFromHWND(HWND hWnd, QString &processPathStr);
    static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
    static BOOL DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath);
    static int findInKeyMappingDataList(QString &keyname);

    // unused enum all process function >>>
    static void EnumProcessFunction(void);
    // unused enum all process function <<<

protected:
    void changeEvent(QEvent *event);

private slots:
    void SystrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    void on_keymapButton_clicked();

    void on_savemaplistButton_clicked();

    void on_refreshButton_clicked();

    void on_processinfoTable_doubleClicked(const QModelIndex &index);

    void on_addmapdataButton_clicked();

    void on_deleteoneButton_clicked();

    void on_clearallButton_clicked();

private:
    static LRESULT CALLBACK LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    void initVirtualKeyCodeMap(void);
    void initProcessInfoTable(void);
    void refreshProcessInfoTable(void);
    void setProcessInfoTable(QList<MAP_PROCESSINFO> &processinfolist);

    void initKeyMappingDataTable(void);
    void initAddKeyComboBoxes(void);

    void saveKeyMapSetting(void);
    void loadKeyMapSetting(void);

    void changeControlEnableStatus(bool status);

public:
    static QList<MAP_PROCESSINFO> static_ProcessInfoList;
    static QHash<QString, V_KEYCODE> VirtualKeyCodeMap;
    static QList<MAP_KEYDATA> KeyMappingDataList;

private:
    Ui::QKeyMapper *ui;
    quint8 m_KeyMapStatus;
    QTimer m_CycleCheckTimer;
    MAP_PROCESSINFO m_MapProcessInfo;
    QSystemTrayIcon *m_SysTrayIcon;
    HHOOK m_KeyHook;
};

#endif // QLINKKEEPER_H
