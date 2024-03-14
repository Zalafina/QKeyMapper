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
#include <QOperatingSystemVersion>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QCheckBox>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QStandardItemModel>
#include <QValidator>
#include <QDir>
#include <QProcess>
#include <windows.h>
#include <process.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <tchar.h>
#include <string.h>
#include <dwmapi.h>
#include <Shlobj.h>
#include <powrprof.h>
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

#if 0
class KeySequenceEditOnlyOne : public QKeySequenceEdit
{
    Q_OBJECT

public:
    explicit KeySequenceEditOnlyOne(QWidget *parent = Q_NULLPTR) :
        QKeySequenceEdit(parent),
        m_DefaultKeySequence(),
        m_LastKeySequence()
    {
    }

    void setDefaultKeySequence(const QString &keysequencestr);
    QString defaultKeySequence();
    void setLastKeySequence(const QString &keysequencestr);
    QString lastKeySequence();

signals:
    void keySeqEditChanged_Signal(const QKeySequence &keysequence);

protected:
    virtual void keyPressEvent(QKeyEvent* pEvent);

private:
    QString m_DefaultKeySequence;
    QString m_LastKeySequence;
};
#endif

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

    #define MOUSE_L_COLOR           RGB(232, 131, 124)
    #define MOUSE_R_COLOR           RGB(90, 165, 250)
    #define MOUSE_M_COLOR           RGB(124, 205, 76)
    #define MOUSE_X1_COLOR          RGB(239, 199, 75)
    #define MOUSE_X2_COLOR          RGB(153, 138, 249)

    enum KeyMapStatus
    {
        KEYMAP_IDLE = 0U,
        KEYMAP_CHECKING,
        KEYMAP_MAPPING_GLOBAL,
        KEYMAP_MAPPING_MATCHED
    };
    Q_ENUM(KeyMapStatus)

    enum MappingStartMode
    {
        MAPPINGSTART_BUTTONCLICK = 0U,
        MAPPINGSTART_HOTKEY,
        MAPPINGSTART_LOADSETTING
    };
    Q_ENUM(MappingStartMode)

public slots:
    void WindowStateChangedProc(void);
    void cycleCheckProcessProc(void);
    void cycleRefreshProcessInfoTableProc(void);

public:
    void setKeyHook(HWND hWnd);
    void setKeyUnHook(void);

    void setMapProcessInfo(const QString &filename, const QString &windowtitle, const QString &pid, const QString &filepath, const QIcon &windowicon);

    QString getExeFileDescription(void);

    static void getProcessInfoFromPID(DWORD processID, QString &processPathStr);
    static void getProcessInfoFromHWND(HWND hWnd, QString &processPathStr);
    static HWND getHWND_byPID(DWORD dwProcessID);
    static BOOL IsAltTabWindow(HWND hWnd);
    static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
    static BOOL CALLBACK EnumChildWindowsProc(HWND hWnd, LPARAM lParam);
    static BOOL DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath);
    static int findOriKeyInKeyMappingDataList(const QString &keyname);
    static int findOriKeyInKeyMappingDataListGlobal(const QString &keyname);
    static int findMapKeyInKeyMappingDataList(const QString &keyname);

    // unused enum all process function >>>
    static void EnumProcessFunction(void);
    // unused enum all process function <<<

    static void DrawMousePoints(HWND hwnd, HDC hdc);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static HWND createTransparentWindow(void);
    static void clearTransparentWindow(HWND hwnd, HDC hdc);

    Qt::CheckState getAutoStartMappingStatus(void);
    // static bool getDisableWinKeyStatus(void);
    static int getBurstPressTime(void);
    static int getBurstReleaseTime(void);
    static int getJoystick2MouseSpeedX(void);
    static int getJoystick2MouseSpeedY(void);
    static int getvJoyXSensitivity(void);
    static int getvJoyYSensitivity(void);
    static QString getVirtualGamepadType(void);
    static bool getLockCursorStatus(void);
    static int getGlobalSettingAutoStart(void);
    static bool checkGlobalSettingAutoStart(void);
    static int getDataPortNumber(void);
    static double getBrakeThreshold(void);
    static double getAccelThreshold(void);

signals:
    void HotKeyDisplaySwitchActivated_Signal(const QString &hotkey_string);
    void HotKeyMappingSwitchActivated_Signal(const QString &hotkey_string);
    void updateLockStatus_Signal(void);
    void updateMousePointLabelDisplay_Signal(const QPoint &point);
    void showMousePoints_Signal(int onoff);
#ifdef VIGEM_CLIENT_SUPPORT
    void updateViGEmBusStatus_Signal(void);
#endif

protected:
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;

public slots:
    void on_keymapButton_clicked();

    void MappingStart(MappingStartMode startmode);

    void updateLockStatusDisplay(void);

    void updateMousePointLabelDisplay(const QPoint &point);

    void showMousePoints(int onoff);

#ifdef SINGLE_APPLICATION
    void raiseQKeyMapperWindow(void);
#endif

    // void updateShortcutsMap(void);
    // void freeShortcuts(void);
    // void HotKeyForMappingActivated(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers);
    // void HotKeyForMappingReleased(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers);

    // void HotKeyActivated(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers);
    // void HotKeyStartStopActivated(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers);

    void HotKeyDisplaySwitchActivated(const QString &hotkey_string);
    void HotKeyMappingSwitchActivated(const QString &hotkey_string);

private slots:
    void onHotKeyLineEditEditingFinished(void);

    // void onWindowSwitchKeySequenceChanged(const QKeySequence &keysequence);
    // void onWindowSwitchKeySequenceEditingFinished();
    // void onMappingSwitchKeySequenceChanged(const QKeySequence &keysequence);
    // void onMappingSwitchKeySequenceEditingFinished();
    // void onOriginalKeySequenceChanged(const QKeySequence &keysequence);
    // void onOriginalKeySequenceEditingFinished();

    void SystrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    void cellChanged_slot(int row, int col);

#ifdef VIGEM_CLIENT_SUPPORT
    void OrikeyComboBox_currentTextChangedSlot(const QString &text);
#endif

    void on_savemaplistButton_clicked();

    void on_processinfoTable_doubleClicked(const QModelIndex &index);

    void on_addmapdataButton_clicked();

    void on_deleteoneButton_clicked();

    void on_clearallButton_clicked();

    void on_moveupButton_clicked();

    void on_movedownButton_clicked();

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void on_settingselectComboBox_textActivated(const QString &text);
#else
    void on_settingselectComboBox_currentTextChanged(const QString &text);
#endif

    void on_removeSettingButton_clicked();

    void on_autoStartupCheckBox_stateChanged(int state);

    void on_languageComboBox_currentIndexChanged(int index);

    void on_enableVirtualJoystickCheckBox_stateChanged(int state);

    void on_installViGEmBusButton_clicked();

    // void on_uninstallViGEmBusButton_clicked();

    void on_soundEffectCheckBox_stateChanged(int state);

private:
    // void initHotKeySequence(void);
    void initProcessInfoTable(void);
    void refreshProcessInfoTable(void);
    void setProcessInfoTable(QList<MAP_PROCESSINFO> &processinfolist);
    void updateProcessInfoDisplay(void);
    void updateSystemTrayDisplay(void);

    void initKeyMappingDataTable(void);
    void initAddKeyComboBoxes(void);
    void initWindowSwitchKeyLineEdit(void);
    void initMappingSwitchKeyLineEdit(void);
    // void updateWindowSwitchKeySeq(const QKeySequence &keysequence);
    // void updateMappingSwitchKeySeq(const QKeySequence &keysequence);
    void updateWindowSwitchKeyString(const QString &keystring);
    void updateMappingSwitchKeyString(const QString &keystring);
    // void initOriginalKeySeqEdit(void);
    void initCombinationKeyLineEdit(void);
    void refreshKeyMappingDataTable(void);
    void reloadUILanguage(void);
    void setUILanguage_Chinese(void);
    void setUILanguage_English(void);
    void resetFontSize(void);

    bool backupFile(const QString &originalFile, const QString &backupFile);
    bool checkSettingsFileNeedtoConvert(void);
    void renameSettingsGroup(QSettings &settings, const QString &oldName, const QString &newName);
    void convertSettingsFile(void);
    int checkAutoStartSaveSettings(const QString &executablename, const QString &windowtitle);
    int checkSaveSettings(const QString &executablename, const QString &windowtitle);
    bool readSaveSettingData(const QString &group, const QString &key, QVariant &settingdata);
    void saveKeyMapSetting(void);
    bool loadKeyMapSetting(const QString &settingtext);
    bool checkMappingkeyStr(QString &mappingkeystr);
    bool validateCombinationKey(QString &input);

    void loadFontFile(const QString fontfilename, int &returnback_fontid, QString &fontname);
#ifdef USE_SAOFONT
    void setControlCustomFont(const QString &fontname);
#endif
    void setControlFontEnglish(void);
    void setControlFontChinese(void);

    void changeControlEnableStatus(bool status);

    void extractSoundFiles();
    void playStartSound();
    void playStopSound();

#ifdef VIGEM_CLIENT_SUPPORT
    int installViGEmBusDriver(void);
    int uninstallViGEmBusDriver(void);
public slots:
    void updateViGEmBusLabelDisplay(void);
    Q_INVOKABLE void reconnectViGEmClient(void);
#endif

public:
    static bool m_isDestructing;
    static int s_GlobalSettingAutoStart;
    static uint s_CycleCheckLoopCount;
    static QList<MAP_PROCESSINFO> static_ProcessInfoList;
    static QList<MAP_KEYDATA> KeyMappingDataList;
    static QList<MAP_KEYDATA> KeyMappingDataListGlobal;
    // static QHash<QString, QHotkey*> ShortcutsMap;
    static QString s_WindowSwitchKeyString;
    static QString s_MappingSwitchKeyString;

private:
    static QKeyMapper *m_instance;
    static QString DEFAULT_TITLE;
    Ui::QKeyMapper *ui;
    quint8 m_KeyMapStatus;
    QPoint m_LastWindowPosition;
    QTimer m_CycleCheckTimer;
    QTimer m_ProcessInfoTableRefreshTimer;
    MAP_PROCESSINFO m_MapProcessInfo;
    QSystemTrayIcon *m_SysTrayIcon;
#ifdef USE_SAOFONT
    int m_SAO_FontFamilyID;
    QString m_SAO_FontName;
#endif
    StyledDelegate *m_ProcessInfoTableDelegate;
    StyledDelegate *m_KeyMappingDataTableDelegate;
    KeyListComboBox *m_orikeyComboBox;
    KeyListComboBox *m_mapkeyComboBox;
    // KeySequenceEditOnlyOne *m_windowswitchKeySeqEdit;
    // KeySequenceEditOnlyOne *m_mappingswitchKeySeqEdit;
    // KeySequenceEditOnlyOne *m_originalKeySeqEdit;
    // QHotkey *m_HotKey_ShowHide;
    // QHotkey *m_HotKey_StartStop;
    int m_UI_Scale;
    bool loadSetting_flag;
    HWND m_TransParentHandle;
};

#endif // QKEYMAPPER_H
