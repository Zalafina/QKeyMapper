#ifndef QKEYMAPPER_H
#define QKEYMAPPER_H

#include <QDialog>
#include <QDebug>
#include <QMetaEnum>
#include <QMessageBox>
#include <QTimer>
#include <QTimerEvent>
#include <QThread>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QHash>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QtWin>
#endif
#include <QStyleFactory>
#include <QFontDatabase>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QCheckBox>
#include <QKeyEvent>
#include <QHotkey>
#include <QDir>
#include <QProcess>
#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <tchar.h>
#include <string.h>
//#include <QKeyEvent>
//#include <QProcess>
//#include <QTextCodec>

#include "qkeymapper_worker.h"

namespace Ui {
class QKeyMapper;
}

typedef struct
{
    QString FileName;
    QString PID;
    QString WindowTitle;
    QString FilePath;
    QIcon   WindowIcon;
}MAP_PROCESSINFO;

class StyledDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    StyledDelegate(QWidget *parent = Q_NULLPTR) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class KeyListComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit KeyListComboBox(QWidget *parent = Q_NULLPTR) : QComboBox(parent)
    {
        if (parent != Q_NULLPTR){
            m_KeyMapper_ptr = parent;
        }
    }

protected:
    void keyPressEvent(QKeyEvent *keyevent);

private:
    QWidget *m_KeyMapper_ptr;
};

class QKeyMapper : public QDialog
{
    Q_OBJECT

public:
    explicit QKeyMapper(QWidget *parent = Q_NULLPTR);
    ~QKeyMapper();

    static QKeyMapper *getInstance()
    {
        return m_instance;
    }

    #define QT_KEY_EXTENDED         (0x01000000U)
    #define QT_KEY_L_SHIFT          (0x00000001U)
    #define QT_KEY_L_CTRL           (0x00000002U)
    #define QT_KEY_L_ALT            (0x00000004U)
    #define QT_KEY_L_WIN            (0x00000008U)
    #define QT_KEY_R_SHIFT          (0x00000010U)
    #define QT_KEY_R_CTRL           (0x00000020U)
    #define QT_KEY_R_ALT            (0x00000040U)
    #define QT_KEY_R_WIN            (0x00000080U)

    enum KeyMapStatus
    {
        KEYMAP_IDLE = 0U,
        KEYMAP_CHECKING,
        KEYMAP_MAPPING
    };
    Q_ENUM(KeyMapStatus)

    Q_INVOKABLE void WindowStateChangedProc(void);
    Q_INVOKABLE void cycleCheckProcessProc(void);

    void setKeyHook(HWND hWnd);
    void setKeyUnHook(void);

    void setMapProcessInfo(const QString &filename, const QString &windowtitle, const QString &pid, const QString &filepath, const QIcon &windowicon);

#ifdef ADJUST_PRIVILEGES
    static BOOL EnableDebugPrivilege(void);
    static BOOL AdjustPrivileges(void);
#endif
    static void getProcessInfoFromPID(DWORD processID, QString &processPathStr);
    static void getProcessInfoFromHWND(HWND hWnd, QString &processPathStr);
    static HWND getHWND_byPID(DWORD dwProcessID);
    static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
    static BOOL CALLBACK EnumChildWindowsProc(HWND hWnd, LPARAM lParam);
    static BOOL DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath);
    static int findInKeyMappingDataList(const QString &keyname);

    // unused enum all process function >>>
    static void EnumProcessFunction(void);
    // unused enum all process function <<<

    bool getAutoStartMappingStatus(void);
    static bool getDisableWinKeyStatus(void);
    static int getBurstPressTime(void);
    static int getBurstReleaseTime(void);

signals:
    void updateLockStatus_Signal(void);

protected:
    void changeEvent(QEvent *event);

public slots:
    void on_keymapButton_clicked();

    void updateLockStatusDisplay(void);

#ifdef SINGLE_APPLICATION
    void raiseQKeyMapperWindow(void);
#endif

private slots:
    void HotKeyActivated();

    void SystrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    void on_cellChanged(int row, int col);

    void on_savemaplistButton_clicked();

    void on_refreshButton_clicked();

    void on_processinfoTable_doubleClicked(const QModelIndex &index);

    void on_addmapdataButton_clicked();

    void on_deleteoneButton_clicked();

    void on_clearallButton_clicked();

    void on_moveupButton_clicked();

    void on_movedownButton_clicked();

    void on_settingselectComboBox_textActivated(const QString &text);

    void on_removeSettingButton_clicked();

    void on_autoStartupCheckBox_stateChanged(int state);

private:
    void initHotKeySequence(void);
    void initProcessInfoTable(void);
    void refreshProcessInfoTable(void);
    void setProcessInfoTable(QList<MAP_PROCESSINFO> &processinfolist);
    void updateProcessInfoDisplay(void);

    void initKeyMappingDataTable(void);
    void initAddKeyComboBoxes(void);
    void refreshKeyMappingDataTable(void);

    bool checkSaveSettings(const QString &executablename);
    bool readSaveSettingData(const QString &group, const QString &key, QVariant &settingdata);
    void saveKeyMapSetting(void);
    bool loadKeyMapSetting(const QString &settingtext);
    bool checkMappingkeyStr(const QString &mappingkeystr);

    void loadFontFile(const QString fontfilename, int &returnback_fontid, QString &fontname);
    void setControlCustomFont(const QString &fontname);

    void changeControlEnableStatus(bool status);

    void extractSoundFiles();
    void playStartSound();

public:
    static QList<MAP_PROCESSINFO> static_ProcessInfoList;
    static QList<MAP_KEYDATA> KeyMappingDataList;

private:
    static QKeyMapper *m_instance;
    static QString DEFAULT_TITLE;
    Ui::QKeyMapper *ui;
    quint8 m_KeyMapStatus;
    QTimer m_CycleCheckTimer;
    MAP_PROCESSINFO m_MapProcessInfo;
    QSystemTrayIcon *m_SysTrayIcon;
    int m_SAO_FontFamilyID;
    QString m_SAO_FontName;
    StyledDelegate *m_ProcessInfoTableDelegate;
    StyledDelegate *m_KeyMappingDataTableDelegate;
    KeyListComboBox *m_orikeyComboBox;
    KeyListComboBox *m_mapkeyComboBox;
    QHotkey *m_HotKey;
};

#endif // QKEYMAPPER_H
