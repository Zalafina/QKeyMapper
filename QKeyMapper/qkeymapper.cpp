#include "qkeymapper.h"
#include "ui_qkeymapper.h"

//static const uint WIN_TITLESTR_MAX = 200U;
static const uint CYCLE_CHECK_TIMEOUT = 300U;
static const uint CYCLE_CHECK_LOOPCOUNT_MAX = 100000U;
static const uint CYCLE_CHECK_LOOPCOUNT_RESET = 500U;

static const uint CYCLE_REFRESH_PROCESSINFOTABLE_TIMEOUT = 3000U;

static const uint GLOBAL_MAPPING_START_WAIT = 2100U / CYCLE_CHECK_TIMEOUT;

static const int PROCESSINFO_TABLE_COLUMN_COUNT = 3;
static const int KEYMAPPINGDATA_TABLE_COLUMN_COUNT = 4;

static const int INITIAL_WINDOW_POSITION = -1;

static const int PROCESS_NAME_COLUMN = 0;
static const int PROCESS_PID_COLUMN = 1;
static const int PROCESS_TITLE_COLUMN = 2;

static const int PROCESS_NAME_COLUMN_WIDTH_MAX = 200;

static const int ORIGINAL_KEY_COLUMN = 0;
static const int MAPPING_KEY_COLUMN = 1;
static const int BURST_MODE_COLUMN = 2;
static const int LOCK_COLUMN = 3;

static const int DEFAULT_ICON_WIDTH = 48;
static const int DEFAULT_ICON_HEIGHT = 48;

static const int MOUSEWHEEL_SCROLL_NONE = 0;
static const int MOUSEWHEEL_SCROLL_UP = 1;
static const int MOUSEWHEEL_SCROLL_DOWN = 2;

static const int KEY_UP = 0;
static const int KEY_DOWN = 1;

static const int LANGUAGE_CHINESE = 0;
static const int LANGUAGE_ENGLISH = 1;

static const int CUSTOMSETTING_INDEX_MAX = 30;

static const int TITLESETTING_INDEX_INVALID = -1;
static const int TITLESETTING_INDEX_ANYTITLE = 0;
static const int TITLESETTING_INDEX_MAX = 9;

static const int BURST_TIME_MIN = 1;
static const int BURST_TIME_MAX = 5000;

static const int BURST_PRESS_TIME_DEFAULT   = 40;
static const int BURST_RELEASE_TIME_DEFAULT = 20;

static const int MAPPING_WAITTIME_MIN = 0;
static const int MAPPING_WAITTIME_MAX = 5000;

static const int DATA_PORT_MIN = 1;
static const int DATA_PORT_MAX = 65535;
static const int DATA_PORT_DEFAULT = 5300;

static const int GRIP_THRESHOLD_DECIMALS = 5;
static const double GRIP_THRESHOLD_BRAKE_MIN = 0.00001;
static const double GRIP_THRESHOLD_BRAKE_MAX = 200.00000;
static const double GRIP_THRESHOLD_BRAKE_DEFAULT = 0.47000;
static const double GRIP_THRESHOLD_ACCEL_MIN = 0.00001;
static const double GRIP_THRESHOLD_ACCEL_MAX = 200.00000;
static const double GRIP_THRESHOLD_ACCEL_DEFAULT = 0.70000;
static const double GRIP_THRESHOLD_SINGLE_STEP = 0.01;

static const int MOUSE_SPEED_MIN = 1;
static const int MOUSE_SPEED_MAX = 15;

static const int UI_SCALE_NORMAL = 0;
static const int UI_SCALE_1K_PERCENT_100 = 1;
static const int UI_SCALE_1K_PERCENT_125 = 2;
static const int UI_SCALE_1K_PERCENT_150 = 3;
static const int UI_SCALE_2K_PERCENT_100 = 4;
static const int UI_SCALE_2K_PERCENT_125 = 5;
static const int UI_SCALE_2K_PERCENT_150 = 6;
static const int UI_SCALE_4K_PERCENT_100 = 7;
static const int UI_SCALE_4K_PERCENT_125 = 8;
static const int UI_SCALE_4K_PERCENT_150 = 9;

#ifdef VIGEM_CLIENT_SUPPORT
static const int RECONNECT_VIGEMCLIENT_WAIT_TIME = 2000;

static const int VIRTUAL_JOYSTICK_SENSITIVITY_MIN = 1;
static const int VIRTUAL_JOYSTICK_SENSITIVITY_MAX = 1000;
static const int VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT = 12;
#endif

static const ULONG_PTR VIRTUAL_KEYBOARD_PRESS = 0xACBDACBD;
static const ULONG_PTR VIRTUAL_MOUSE_CLICK = 0xCEDFCEDF;

static const char *PROGRAM_NAME = "QKeyMapper";

static const char *DEFAULT_NAME = "ForzaHorizon4.exe";
static const char *CONFIG_FILENAME = "keymapdata.ini";
static const char *CONFIG_BACKUP_FILENAME = "keymapdata_backup.ini";

static const char *DISPLAYSWITCH_KEYSEQ         = "Ctrl+`";
static const char *MAPPINGSWITCH_KEYSEQ_DEFAULT = "Ctrl+F6";
static const char *ORIGINAL_KEYSEQ_DEFAULT      = PREFIX_SHORTCUT;

/* General global settings >>> */
static const char *LAST_WINDOWPOSITION = "LastWindowPosition";
static const char *LANGUAGE_INDEX = "LanguageIndex";
static const char *SETTINGSELECT = "SettingSelect";
static const char *AUTO_STARTUP = "AutoStartup";
static const char *PLAY_SOUNDEFFECT = "PlaySoundEffect";
static const char *WINDOWSWITCH_KEYSEQ = "WindowSwitch_KeySequence";
#ifdef VIGEM_CLIENT_SUPPORT
static const char *VIRTUALGAMEPAD_ENABLE = "VirtualGamepadEnable";
static const char *VIRTUALGAMEPAD_TYPE = "VirtualGamepadType";
#endif
/* General global settings <<< */

static const char *GROUPNAME_EXECUTABLE_SUFFIX = ".exe";
static const char *GROUPNAME_CUSTOMSETTING = "CustomSetting ";
static const char *GROUPNAME_CUSTOMGLOBALSETTING = "CustomGlobalSetting ";
static const char *GROUPNAME_GLOBALSETTING = "QKeyMapperGlobalSetting";
static const char *WINDOWTITLE_STRING = "Title";
static const char *ANYWINDOWTITLE_STRING = "AnyTitle";

static const char *KEYMAPDATA_ORIGINALKEYS = "KeyMapData_OriginalKeys";
static const char *KEYMAPDATA_MAPPINGKEYS = "KeyMapData_MappingKeys";
static const char *KEYMAPDATA_BURST = "KeyMapData_Burst";
static const char *KEYMAPDATA_LOCK = "KeyMapData_Lock";
static const char *KEYMAPDATA_BURSTPRESS_TIME = "KeyMapData_BurstPressTime";
static const char *KEYMAPDATA_BURSTRELEASE_TIME = "KeyMapData_BurstReleaseTime";
#ifdef VIGEM_CLIENT_SUPPORT
static const char *MOUSE2VJOY_X_SENSITIVITY = "Mouse2vJoy_XSensitivity";
static const char *MOUSE2VJOY_Y_SENSITIVITY = "Mouse2vJoy_YSensitivity";
static const char *MOUSE2VJOY_LOCKCURSOR = "Mouse2vJoy_LockCursor";
#endif
static const char *CLEARALL = "KeyMapData_ClearAll";

static const char *PROCESSINFO_FILENAME = "ProcessInfo_FileName";
static const char *PROCESSINFO_WINDOWTITLE = "ProcessInfo_WindowTitle";
static const char *PROCESSINFO_FILEPATH = "ProcessInfo_FilePath";
static const char *PROCESSINFO_FILENAME_CHECKED = "ProcessInfo_FileNameChecked";
static const char *PROCESSINFO_WINDOWTITLE_CHECKED = "ProcessInfo_WindowTitleChecked";

static const char *DATAPORT_NUMBER = "DataPortNumber";
static const char *GRIP_THRESHOLD_BRAKE = "GripThresholdBrake";
static const char *GRIP_THRESHOLD_ACCEL = "GripThresholdAccel";
static const char *DISABLEWINKEY_CHECKED = "DisableWinKeyChecked";
static const char *AUTOSTARTMAPPING_CHECKED = "AutoStartMappingChecked";
static const char *MAPPINGSWITCH_KEYSEQ = "MappingSwitch_KeySequence";

static const char *SAO_FONTFILENAME = ":/sao_ui.otf";

static const char *SOUNDFILE_START_QRC = ":/QKeyMapperStart.wav";
static const char *SOUNDFILE_START = "QKeyMapperStart.wav";
static const char *SOUNDFILE_STOP_QRC = ":/QKeyMapperStop.wav";
static const char *SOUNDFILE_STOP = "QKeyMapperStop.wav";

static const char *FONTNAME_ENGLISH = "Microsoft YaHei UI";
static const char *FONTNAME_CHINESE = "NSimSun";

static const char *VJOY_MOUSE2LS_STR = "vJoy-Mouse2LS";
static const char *VJOY_MOUSE2RS_STR = "vJoy-Mouse2RS";

static const char *VJOY_LT_BRAKE_STR = "vJoy-Key11(LT)_BRAKE";
static const char *VJOY_RT_BRAKE_STR = "vJoy-Key12(RT)_BRAKE";
static const char *VJOY_LT_ACCEL_STR = "vJoy-Key11(LT)_ACCEL";
static const char *VJOY_RT_ACCEL_STR = "vJoy-Key12(RT)_ACCEL";

static const char *JOY_LT2VJOYLT_STR = "Joy-Key11(LT)_2vJoyLT";
static const char *JOY_RT2VJOYRT_STR = "Joy-Key12(RT)_2vJoyRT";

static const char *JOY_LS2MOUSE_STR = "Joy-LS2Mouse";
static const char *JOY_RS2MOUSE_STR = "Joy-RS2Mouse";

static const char *MOUSE_STR_WHEEL_UP = "Mouse-WheelUp";
static const char *MOUSE_STR_WHEEL_DOWN = "Mouse-WheelDown";

static const char *VIRTUAL_GAMEPAD_X360 = "X360";
static const char *VIRTUAL_GAMEPAD_DS4  = "DS4";

static const char *REFRESHBUTTON_CHINESE = "刷新";
static const char *KEYMAPBUTTON_START_CHINESE = "开始映射";
static const char *KEYMAPBUTTON_STOP_CHINESE = "停止映射";
static const char *SAVEMAPLISTBUTTON_CHINESE = "保存设定";
static const char *DELETEONEBUTTON_CHINESE = "删除";
static const char *CLEARALLBUTTON_CHINESE = "清空";
static const char *ADDMAPDATABUTTON_CHINESE = "添加";
static const char *NAMECHECKBOX_CHINESE = "进程";
static const char *TITLECHECKBOX_CHINESE = "标题";
static const char *ORIKEYLABEL_CHINESE = "原始按键";
static const char *ORIKEYSEQLABEL_CHINESE = "原始组合键";
static const char *MAPKEYLABEL_CHINESE = "映射按键";
static const char *BURSTPRESSLABEL_CHINESE = "连发按下";
static const char *BURSTRELEASE_CHINESE = "连发抬起";
// static const char *BURSTPRESS_MSLABEL_CHINESE = "毫秒";
// static const char *BURSTRELEASE_MSLABEL_CHINESE = "毫秒";
static const char *WAITTIME_CHINESE = "延时";
// static const char *WAITTIME_MSLABEL_CHINESE = "毫秒";
static const char *MOUSEXSPEEDLABEL_CHINESE = "X轴速度";
static const char *MOUSEYSPEEDLABEL_CHINESE = "Y轴速度";
// static const char *SETTINGSELECTLABEL_CHINESE = "设定";
static const char *REMOVESETTINGBUTTON_CHINESE = "移除";
static const char *DISABLEWINKEYCHECKBOX_CHINESE = "禁用WIN键";
static const char *DATAPORTLABEL_CHINESE = "数据端口";
static const char *BRAKETHRESHOLDLABEL_CHINESE = "刹车阈值";
static const char *ACCELTHRESHOLDLABEL_CHINESE = "油门阈值";
static const char *AUTOSTARTMAPPINGCHECKBOX_CHINESE = "自动映射并最小化";
static const char *AUTOSTARTUPCHECKBOX_CHINESE = "开机自动启动";
static const char *SOUNDEFFECTCHECKBOX_CHINESE = "音效";
static const char *WINDOWSWITCHKEYLABEL_CHINESE = "显示切换键";
static const char *MAPPINGSWITCHKEYLABEL_CHINESE = "映射开关键";
static const char *PROCESSINFOTABLE_COL1_CHINESE = "进程";
static const char *PROCESSINFOTABLE_COL2_CHINESE = "进程号";
static const char *PROCESSINFOTABLE_COL3_CHINESE = "窗口标题";
static const char *KEYMAPDATATABLE_COL1_CHINESE = "原始按键";
static const char *KEYMAPDATATABLE_COL2_CHINESE = "映射按键";
static const char *KEYMAPDATATABLE_COL3_CHINESE = "连发";
static const char *KEYMAPDATATABLE_COL4_CHINESE = "锁定";
#ifdef VIGEM_CLIENT_SUPPORT
static const char *VIRTUALGAMEPADGROUPBOX_CHINESE = "虚拟游戏手柄";
static const char *VJOYXSENSLABEL_CHINESE = "X轴灵敏度";
static const char *VJOYYSENSLABEL_CHINESE = "Y轴灵敏度";
static const char *VIGEMBUSSTATUSLABEL_UNAVAILABLE_CHINESE = "ViGEm不可用";
static const char *VIGEMBUSSTATUSLABEL_AVAILABLE_CHINESE = "ViGEm可用";
static const char *INSTALLVIGEMBUSBUTTON_CHINESE = "安装ViGEm";
static const char *UNINSTALLVIGEMBUSBUTTON_CHINESE = "卸载ViGEm";
static const char *ENABLEVIRTUALJOYSTICKCHECKBOX_CHINESE = "虚拟手柄";
static const char *LOCKCURSORCHECKBOX_CHINESE = "锁定光标";
#endif

static const char *REFRESHBUTTON_ENGLISH = "Refresh";
static const char *KEYMAPBUTTON_START_ENGLISH = "MappingStart";
static const char *KEYMAPBUTTON_STOP_ENGLISH = "MappingStop";
static const char *SAVEMAPLISTBUTTON_ENGLISH = "SaveSetting";
static const char *DELETEONEBUTTON_ENGLISH = "Delete";
static const char *CLEARALLBUTTON_ENGLISH = "Clear";
static const char *ADDMAPDATABUTTON_ENGLISH = "ADD";
static const char *NAMECHECKBOX_ENGLISH = "Process";
static const char *TITLECHECKBOX_ENGLISH = "Title";
static const char *ORIKEYLABEL_ENGLISH = "OriKey";
static const char *ORIKEYSEQLABEL_ENGLISH = "OriKeySeq";
static const char *MAPKEYLABEL_ENGLISH = "MapKey";
static const char *BURSTPRESSLABEL_ENGLISH = "BurstPress";
static const char *BURSTRELEASE_ENGLISH = "BurstRelease";
// static const char *BURSTPRESS_MSLABEL_ENGLISH = "ms";
// static const char *BURSTRELEASE_MSLABEL_ENGLISH = "ms";
static const char *WAITTIME_ENGLISH = "Delay";
// static const char *WAITTIME_MSLABEL_ENGLISH = "ms";
static const char *MOUSEXSPEEDLABEL_ENGLISH = "X Speed";
static const char *MOUSEYSPEEDLABEL_ENGLISH = "Y Speed";
// static const char *SETTINGSELECTLABEL_ENGLISH = "Setting";
static const char *REMOVESETTINGBUTTON_ENGLISH = "Remove";
static const char *DISABLEWINKEYCHECKBOX_ENGLISH = "Disable WIN";
static const char *DATAPORTLABEL_ENGLISH = "DataPort";
static const char *BRAKETHRESHOLDLABEL_ENGLISH = "BrakeThreshold";
static const char *ACCELTHRESHOLDLABEL_ENGLISH = "AccelThreshold";
static const char *AUTOSTARTMAPPINGCHECKBOX_ENGLISH = "AutoMappingMinimize";
static const char *AUTOSTARTUPCHECKBOX_ENGLISH = "Auto Startup";
static const char *SOUNDEFFECTCHECKBOX_ENGLISH = "Sound Effect";
static const char *WINDOWSWITCHKEYLABEL_ENGLISH = "WindowSwitchKey";
static const char *MAPPINGSWITCHKEYLABEL_ENGLISH = "MappingSwitchKey";
static const char *PROCESSINFOTABLE_COL1_ENGLISH = "Process";
static const char *PROCESSINFOTABLE_COL2_ENGLISH = "PID";
static const char *PROCESSINFOTABLE_COL3_ENGLISH = "Window Title";
static const char *KEYMAPDATATABLE_COL1_ENGLISH = "OriginalKey";
static const char *KEYMAPDATATABLE_COL2_ENGLISH = "MappingKey";
static const char *KEYMAPDATATABLE_COL3_ENGLISH = "Burst";
static const char *KEYMAPDATATABLE_COL4_ENGLISH = "Lock";
#ifdef VIGEM_CLIENT_SUPPORT
static const char *VIRTUALGAMEPADGROUPBOX_ENGLISH = "Virtual Gamepad";
static const char *VJOYXSENSLABEL_ENGLISH = "X Sensitivity";
static const char *VJOYYSENSLABEL_ENGLISH = "Y Sensitivity";
static const char *VIGEMBUSSTATUSLABEL_UNAVAILABLE_ENGLISH = "ViGEmUnavailable";
static const char *VIGEMBUSSTATUSLABEL_AVAILABLE_ENGLISH = "ViGEmAvailable";
static const char *INSTALLVIGEMBUSBUTTON_ENGLISH = "InstallViGEm";
static const char *UNINSTALLVIGEMBUSBUTTON_ENGLISH = "UninstallViGEm";
static const char *ENABLEVIRTUALJOYSTICKCHECKBOX_ENGLISH = "VirtualGamepad";
static const char *LOCKCURSORCHECKBOX_ENGLISH = "Lock Cursor";
#endif

QKeyMapper *QKeyMapper::m_instance = Q_NULLPTR;
QString QKeyMapper::DEFAULT_TITLE = QString("Forza: Horizon 4");

bool QKeyMapper::m_isDestructing = false;
int QKeyMapper::s_GlobalSettingAutoStart = 0;
uint QKeyMapper::s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
QList<MAP_PROCESSINFO> QKeyMapper::static_ProcessInfoList = QList<MAP_PROCESSINFO>();
QList<MAP_KEYDATA> QKeyMapper::KeyMappingDataList = QList<MAP_KEYDATA>();
QList<MAP_KEYDATA> QKeyMapper::KeyMappingDataListGlobal = QList<MAP_KEYDATA>();
QHash<QString, QHotkey*> QKeyMapper::ShortcutsMap = QHash<QString, QHotkey*>();

QKeyMapper::QKeyMapper(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QKeyMapper),
    m_KeyMapStatus(KEYMAP_IDLE),
    m_LastWindowPosition(INITIAL_WINDOW_POSITION, INITIAL_WINDOW_POSITION),
    m_CycleCheckTimer(this),
    m_ProcessInfoTableRefreshTimer(this),
    m_MapProcessInfo(),
    m_SysTrayIcon(Q_NULLPTR),
#ifdef USE_SAOFONT
    m_SAO_FontFamilyID(-1),
    m_SAO_FontName(),
#endif
    m_ProcessInfoTableDelegate(Q_NULLPTR),
    m_KeyMappingDataTableDelegate(Q_NULLPTR),
    m_orikeyComboBox(new KeyListComboBox(this)),
    m_mapkeyComboBox(new KeyListComboBox(this)),
    m_windowswitchKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    m_mappingswitchKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    m_originalKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    m_HotKey_ShowHide(new QHotkey(this)),
    m_HotKey_StartStop(new QHotkey(this)),
    m_UI_Scale(UI_SCALE_NORMAL),
    loadSetting_flag(false)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("QKeyMapper() -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

    m_instance = this;
    ui->setupUi(this);
    QStyle* defaultStyle = QStyleFactory::create("windows");
    ui->virtualgamepadGroupBox->setStyle(defaultStyle);

#ifdef SETTINGSFILE_CONVERT
    bool settingNeedConvert = checkSettingsFileNeedtoConvert();
    if (settingNeedConvert) {
        bool backupRet = backupFile(CONFIG_FILENAME, CONFIG_BACKUP_FILENAME);
        if (backupRet) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "QKeyMapper() -> Need to convert Settings INI file! Backup Settings file Success.";
#endif
            convertSettingsFile();
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "QKeyMapper() -> Need to convert Settings INI file! But backup Settings file Failed!";
#endif
        }
    }
#endif

    extractSoundFiles();
    initAddKeyComboBoxes();
    initWindowSwitchKeySeqEdit();
    initMappingSwitchKeySeqEdit();
    initOriginalKeySeqEdit();

    QString fileDescription = getExeFileDescription();
    setWindowTitle(fileDescription);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "QKeyMapper() -> Set WindowTitle to FileDescription :" << fileDescription;
#endif

#ifdef USE_SAOFONT
    loadFontFile(SAO_FONTFILENAME, m_SAO_FontFamilyID, m_SAO_FontName);
#endif
    QString defaultTitle;
    defaultTitle.append(QChar(0x6781));
    defaultTitle.append(QChar(0x9650));
    defaultTitle.append(QChar(0x7ade));
    defaultTitle.append(QChar(0x901f));
    defaultTitle.append(QChar(0xff1a));
    defaultTitle.append(QChar(0x5730));
    defaultTitle.append(QChar(0x5e73));
    defaultTitle.append(QChar(0x7ebf));
    defaultTitle.append(QChar(0x0020));
    defaultTitle.append(QChar(0x0034));
    DEFAULT_TITLE = defaultTitle;

    QByteArray win_scale_factor = qgetenv("WINDOWS_SCALE_FACTOR");
    if (win_scale_factor == QByteArray("4K_1.0")) {
        m_UI_Scale = UI_SCALE_4K_PERCENT_100;
    }
    else if (win_scale_factor == QByteArray("2K_1.0")) {
        m_UI_Scale = UI_SCALE_2K_PERCENT_100;
    }
    else if (win_scale_factor == QByteArray("1K_1.0")) {
        m_UI_Scale = UI_SCALE_1K_PERCENT_100;
    }
    else if (win_scale_factor == QByteArray("1K_1.25")) {
        m_UI_Scale = UI_SCALE_1K_PERCENT_125;
    }
    else if (win_scale_factor == QByteArray("1K_1.5")) {
        m_UI_Scale = UI_SCALE_1K_PERCENT_150;
    }
    else if (win_scale_factor == QByteArray("2K_1.25")) {
        m_UI_Scale = UI_SCALE_2K_PERCENT_125;
    }
    else if (win_scale_factor == QByteArray("2K_1.5")) {
        m_UI_Scale = UI_SCALE_2K_PERCENT_150;
    }
    else if (win_scale_factor == QByteArray("4K_1.25")) {
        m_UI_Scale = UI_SCALE_4K_PERCENT_125;
    }
    else if (win_scale_factor == QByteArray("4K_1.5")) {
        m_UI_Scale = UI_SCALE_4K_PERCENT_150;
    }
    else {
        m_UI_Scale = UI_SCALE_NORMAL;
    }

#ifdef USE_SAOFONT
    if ((m_SAO_FontFamilyID != -1)
            && (false == m_SAO_FontName.isEmpty())){
        setControlCustomFont(m_SAO_FontName);
    }
#endif

    // set QTableWidget selected background-color
    setStyleSheet("QTableWidget::item:selected { background-color: rgb(190, 220, 255) }");

    ui->iconLabel->setStyle(QStyleFactory::create("windows"));
    setMapProcessInfo(QString(DEFAULT_NAME), QString(DEFAULT_TITLE), QString(), QString(), QIcon(":/DefaultIcon.ico"));
    ui->nameCheckBox->setChecked(true);
    ui->titleCheckBox->setChecked(true);

    ui->moveupButton->setFont(QFont("SimSun", 14));
    ui->movedownButton->setFont(QFont("SimSun", 16));

    initProcessInfoTable();
    ui->nameCheckBox->setFocusPolicy(Qt::NoFocus);
    ui->titleCheckBox->setFocusPolicy(Qt::NoFocus);
    ui->nameLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->titleLineEdit->setFocusPolicy(Qt::ClickFocus);

    ui->dataPortSpinBox->setRange(DATA_PORT_MIN, DATA_PORT_MAX);
    ui->brakeThresholdDoubleSpinBox->setDecimals(GRIP_THRESHOLD_DECIMALS);
    ui->brakeThresholdDoubleSpinBox->setRange(GRIP_THRESHOLD_BRAKE_MIN, GRIP_THRESHOLD_BRAKE_MAX);
    ui->accelThresholdDoubleSpinBox->setDecimals(GRIP_THRESHOLD_DECIMALS);
    ui->accelThresholdDoubleSpinBox->setRange(GRIP_THRESHOLD_ACCEL_MIN, GRIP_THRESHOLD_ACCEL_MAX);
    ui->waitTimeSpinBox->setRange(MAPPING_WAITTIME_MIN, MAPPING_WAITTIME_MAX);
    ui->burstpressSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    ui->burstreleaseSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    ui->mouseXSpeedSpinBox->setRange(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX);
    ui->mouseYSpeedSpinBox->setRange(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX);

    ui->dataPortSpinBox->setValue(DATA_PORT_DEFAULT);
    ui->brakeThresholdDoubleSpinBox->setValue(GRIP_THRESHOLD_BRAKE_DEFAULT);
    ui->brakeThresholdDoubleSpinBox->setSingleStep(GRIP_THRESHOLD_SINGLE_STEP);
    ui->accelThresholdDoubleSpinBox->setValue(GRIP_THRESHOLD_ACCEL_DEFAULT);
    ui->accelThresholdDoubleSpinBox->setSingleStep(GRIP_THRESHOLD_SINGLE_STEP);

    m_SysTrayIcon = new QSystemTrayIcon(this);
    m_SysTrayIcon->setIcon(QIcon(":/QKeyMapper.ico"));
    m_SysTrayIcon->setToolTip("QKeyMapper(Idle)");
    m_SysTrayIcon->show();

#ifdef VIGEM_CLIENT_SUPPORT
    ui->vJoyXSensSpinBox->setRange(VIRTUAL_JOYSTICK_SENSITIVITY_MIN, VIRTUAL_JOYSTICK_SENSITIVITY_MAX);
    ui->vJoyYSensSpinBox->setRange(VIRTUAL_JOYSTICK_SENSITIVITY_MIN, VIRTUAL_JOYSTICK_SENSITIVITY_MAX);
    ui->vJoyXSensSpinBox->setValue(VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT);
    ui->vJoyYSensSpinBox->setValue(VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT);

    bool isWin10Above = false;
    QOperatingSystemVersion osVersion = QOperatingSystemVersion::current();
    if (osVersion >= QOperatingSystemVersion::Windows10) {
        isWin10Above = true;
    }
    else {
        isWin10Above = false;
    }

    if (isWin10Above) {
        ui->vJoyXSensSpinBox->setEnabled(false);
        ui->vJoyYSensSpinBox->setEnabled(false);
        ui->vJoyXSensLabel->setEnabled(false);
        ui->vJoyYSensLabel->setEnabled(false);
        ui->lockCursorCheckBox->setEnabled(false);
        ui->enableVirtualJoystickCheckBox->setEnabled(false);

        int retval_alloc = QKeyMapper_Worker::ViGEmClient_Alloc();
        int retval_connect = QKeyMapper_Worker::ViGEmClient_Connect();
        Q_UNUSED(retval_alloc);
        Q_UNUSED(retval_connect);

        if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS != QKeyMapper_Worker::ViGEmClient_getConnectState()) {
#ifdef DEBUG_LOGOUT_ON
            qWarning("ViGEmClient initialize failed!!! -> retval_alloc(%d), retval_connect(%d)", retval_alloc, retval_connect);
#endif
        }

        updateViGEmBusLabelDisplay();
    }
    else {
        ui->enableVirtualJoystickCheckBox->setCheckState(Qt::Unchecked);
        ui->enableVirtualJoystickCheckBox->setEnabled(false);
        ui->installViGEmBusButton->setEnabled(false);
        // ui->uninstallViGEmBusButton->setEnabled(false);
        ui->ViGEmBusStatusLabel->setEnabled(false);
        ui->vJoyXSensSpinBox->setEnabled(false);
        ui->vJoyYSensSpinBox->setEnabled(false);

        ui->enableVirtualJoystickCheckBox->setVisible(false);
        ui->lockCursorCheckBox->setVisible(false);
        ui->installViGEmBusButton->setVisible(false);
        // ui->uninstallViGEmBusButton->setVisible(false);
        ui->ViGEmBusStatusLabel->setVisible(false);
        ui->vJoyXSensSpinBox->setVisible(false);
        ui->vJoyYSensSpinBox->setVisible(false);
        ui->vJoyXSensLabel->setVisible(false);
        ui->vJoyYSensLabel->setVisible(false);
        ui->virtualgamepadGroupBox->setVisible(false);
    }
#else
    ui->enableVirtualJoystickCheckBox->setCheckState(Qt::Unchecked);
    ui->enableVirtualJoystickCheckBox->setEnabled(false);
    ui->lockCursorCheckBox->setEnabled(false);
    ui->installViGEmBusButton->setEnabled(false);
    // ui->uninstallViGEmBusButton->setEnabled(false);
    ui->ViGEmBusStatusLabel->setEnabled(false);

    ui->enableVirtualJoystickCheckBox->setVisible(false);
    ui->lockCursorCheckBox->setVisible(false);
    ui->installViGEmBusButton->setVisible(false);
    // ui->uninstallViGEmBusButton->setVisible(false);
    ui->ViGEmBusStatusLabel->setVisible(false);
    ui->vJoyXSensSpinBox->setVisible(false);
    ui->vJoyYSensSpinBox->setVisible(false);
    ui->vJoyXSensLabel->setVisible(false);
    ui->vJoyYSensLabel->setVisible(false);
    ui->virtualgamepadGroupBox->setVisible(false);
#endif

    m_windowswitchKeySeqEdit->setDefaultKeySequence(DISPLAYSWITCH_KEYSEQ);
    m_mappingswitchKeySeqEdit->setDefaultKeySequence(MAPPINGSWITCH_KEYSEQ_DEFAULT);
    m_originalKeySeqEdit->setDefaultKeySequence(ORIGINAL_KEYSEQ_DEFAULT);
    initKeyMappingDataTable();
    loadSetting_flag = true;
    bool loadresult = loadKeyMapSetting(QString());
    Q_UNUSED(loadresult);
    loadSetting_flag = false;
    reloadUILanguage();
    resetFontSize();

    QObject::connect(m_SysTrayIcon, &QSystemTrayIcon::activated, this, &QKeyMapper::SystrayIconActivated);
    QObject::connect(&m_CycleCheckTimer, &QTimer::timeout, this, &QKeyMapper::cycleCheckProcessProc);
    QObject::connect(&m_ProcessInfoTableRefreshTimer, &QTimer::timeout, this, &QKeyMapper::cycleRefreshProcessInfoTableProc);
    QObject::connect(ui->keymapdataTable, &QTableWidget::cellChanged, this, &QKeyMapper::cellChanged_slot);

    QObject::connect(m_windowswitchKeySeqEdit, &KeySequenceEditOnlyOne::keySeqEditChanged_Signal, this, &QKeyMapper::onWindowSwitchKeySequenceChanged);
    QObject::connect(m_windowswitchKeySeqEdit, &KeySequenceEditOnlyOne::editingFinished, this, &QKeyMapper::onWindowSwitchKeySequenceEditingFinished);
    QObject::connect(m_mappingswitchKeySeqEdit, &KeySequenceEditOnlyOne::keySeqEditChanged_Signal, this, &QKeyMapper::onMappingSwitchKeySequenceChanged);
    QObject::connect(m_mappingswitchKeySeqEdit, &KeySequenceEditOnlyOne::editingFinished, this, &QKeyMapper::onMappingSwitchKeySequenceEditingFinished);
    QObject::connect(m_originalKeySeqEdit, &KeySequenceEditOnlyOne::keySeqEditChanged_Signal, this, &QKeyMapper::onOriginalKeySequenceChanged);
    QObject::connect(m_originalKeySeqEdit, &KeySequenceEditOnlyOne::editingFinished, this, &QKeyMapper::onOriginalKeySequenceEditingFinished);

    QObject::connect(this, &QKeyMapper::updateLockStatus_Signal, this, &QKeyMapper::updateLockStatusDisplay, Qt::QueuedConnection);
#ifdef VIGEM_CLIENT_SUPPORT
    QObject::connect(this, &QKeyMapper::updateViGEmBusStatus_Signal, this, &QKeyMapper::updateViGEmBusLabelDisplay);
    QObject::connect(m_orikeyComboBox, &KeyListComboBox::currentTextChanged, this, &QKeyMapper::OrikeyComboBox_currentTextChangedSlot);
#endif

    //m_CycleCheckTimer.start(CYCLE_CHECK_TIMEOUT);
    refreshProcessInfoTable();
#ifndef DEBUG_LOGOUT_ON
    m_ProcessInfoTableRefreshTimer.start(CYCLE_REFRESH_PROCESSINFOTABLE_TIMEOUT);
#endif

//    if (false == loadresult){
//        QMessageBox::warning(this, PROGRAM_NAME, tr("Load invalid keymapdata from ini file.\nReset to default values."));
//    }

    initHotKeySequence();
}

QKeyMapper::~QKeyMapper()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "~QKeyMapper() called.";
#endif
    m_isDestructing = true;

    freeShortcuts();

    delete m_orikeyComboBox;
    m_orikeyComboBox = Q_NULLPTR;
    delete m_mapkeyComboBox;
    m_mapkeyComboBox = Q_NULLPTR;

    delete m_windowswitchKeySeqEdit;
    m_windowswitchKeySeqEdit = Q_NULLPTR;

    delete m_mappingswitchKeySeqEdit;
    m_mappingswitchKeySeqEdit = Q_NULLPTR;

    delete m_originalKeySeqEdit;
    m_originalKeySeqEdit = Q_NULLPTR;

    delete m_HotKey_ShowHide;
    m_HotKey_ShowHide = Q_NULLPTR;
    delete m_HotKey_StartStop;
    m_HotKey_StartStop = Q_NULLPTR;

    delete ui;
    delete m_SysTrayIcon;
    m_SysTrayIcon = Q_NULLPTR;

    delete m_ProcessInfoTableDelegate;
    m_ProcessInfoTableDelegate = Q_NULLPTR;

    delete m_KeyMappingDataTableDelegate;
    m_KeyMappingDataTableDelegate = Q_NULLPTR;
}

void QKeyMapper::WindowStateChangedProc(void)
{
    if (true == isMinimized()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[WindowStateChangedProc]" << "QKeyMapper::WindowStateChangedProc() -> Window Minimized: setHidden!";
#endif
        hide();
    }
}

void QKeyMapper::cycleCheckProcessProc(void)
{
    if (KEYMAP_IDLE != m_KeyMapStatus)
    {
        int checkresult = 0;
        QString windowTitle;
        QString filename;
        HWND hwnd = GetForegroundWindow();
        TCHAR titleBuffer[MAX_PATH];
        memset(titleBuffer, 0x00, sizeof(titleBuffer));
        QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();

        if ((false == ui->nameCheckBox->isChecked())
            && (false == ui->titleCheckBox->isChecked())){
            checkresult = 2;
        }

        int resultLength = GetWindowText(hwnd, titleBuffer, MAX_PATH);
        if (resultLength){
            QString ProcessPath;
            getProcessInfoFromHWND( hwnd, ProcessPath);

            windowTitle = QString::fromWCharArray(titleBuffer);

            if (false == windowTitle.isEmpty() && false == ProcessPath.isEmpty()){
                QFileInfo fileinfo(ProcessPath);
                filename = fileinfo.fileName();
            }

            if (filename.isEmpty() != true) {
                int savecheckindex = checkAutoStartSaveSettings(filename, windowTitle);

                if ((TITLESETTING_INDEX_INVALID < savecheckindex && savecheckindex <= TITLESETTING_INDEX_MAX) && (KEYMAP_CHECKING == m_KeyMapStatus || KEYMAP_MAPPING_GLOBAL == m_KeyMapStatus)) {
                    bool needtoload = false;
                    QVariant nameChecked_Var;
                    QVariant titleChecked_Var;
                    QVariant fileName_Var;
                    QVariant windowTitle_Var;
                    bool nameChecked = false;
                    bool titleChecked = false;
                    QString readFileName;
                    QString readWindowTitle;
                    QString loadSettingSelectStr;

                    if (savecheckindex == TITLESETTING_INDEX_ANYTITLE) {
                        loadSettingSelectStr = filename + SEPARATOR_TITLESETTING + QString(ANYWINDOWTITLE_STRING);
                    }
                    else {
                        loadSettingSelectStr = filename + SEPARATOR_TITLESETTING + QString(WINDOWTITLE_STRING) + QString::number(savecheckindex);
                    }

                    if (readSaveSettingData(loadSettingSelectStr, PROCESSINFO_FILENAME_CHECKED, nameChecked_Var)) {
                        nameChecked = nameChecked_Var.toBool();
                    }
                    if (readSaveSettingData(loadSettingSelectStr, PROCESSINFO_WINDOWTITLE_CHECKED, titleChecked_Var)) {
                        titleChecked = titleChecked_Var.toBool();
                    }
                    if (readSaveSettingData(loadSettingSelectStr, PROCESSINFO_FILENAME, fileName_Var)) {
                        readFileName = fileName_Var.toString();
                    }
                    if (readSaveSettingData(loadSettingSelectStr, PROCESSINFO_WINDOWTITLE, windowTitle_Var)) {
                        readWindowTitle = windowTitle_Var.toString();
                    }

                    if ((true == nameChecked)
                            && (true == titleChecked)
                            && (savecheckindex != TITLESETTING_INDEX_ANYTITLE)){
                        needtoload = true;
                    }
                    else if (true == nameChecked
                            && savecheckindex == TITLESETTING_INDEX_ANYTITLE){
                        needtoload = true;
                    }

                    if (needtoload) {
                        QString curSettingSelectStr = ui->settingselectComboBox->currentText();
                        if (curSettingSelectStr != loadSettingSelectStr) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace().noquote() << "[cycleCheckProcessProc] "<< "Setting Check Matched! Load setting -> [" << loadSettingSelectStr << "]";
#endif
                            loadSetting_flag = true;
                            bool loadresult = loadKeyMapSetting(loadSettingSelectStr);
                            Q_UNUSED(loadresult)
                            loadSetting_flag = false;
                        }
                        else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[cycleCheckProcessProc]" << "Current setting select is already the same ->" << curSettingSelectStr;
#endif
                        }
                    }
                }

                if ((true == ui->nameCheckBox->isChecked())
                    && (true == ui->titleCheckBox->isChecked())){
                    if ((m_MapProcessInfo.FileName == filename)
                        && (m_MapProcessInfo.WindowTitle.isEmpty() == false)
                        && (m_MapProcessInfo.WindowTitle == windowTitle || windowTitle.contains(m_MapProcessInfo.WindowTitle))){
                        checkresult = 1;
                    }
                }
                else if (true == ui->nameCheckBox->isChecked()){
                    if (m_MapProcessInfo.FileName == filename){
                        checkresult = 1;
                    }
                }
                else{
                    // checkresult = 2;
                }

                if ((m_MapProcessInfo.FileName == filename)
                    && (m_MapProcessInfo.WindowTitle.isEmpty() == false)
                    && (m_MapProcessInfo.WindowTitle == windowTitle || windowTitle.contains(m_MapProcessInfo.WindowTitle))){
                    checkresult = 1;
                }
            }
        }

        bool isVisibleWindow = IsWindowVisible(hwnd);
        bool isExToolWindow = false;

        WINDOWINFO winInfo;
        winInfo.cbSize = sizeof(WINDOWINFO);
        if (GetWindowInfo(hwnd, &winInfo)) {
            if ((winInfo.dwExStyle & WS_EX_TOOLWINDOW) != 0)
                isExToolWindow = true;
        }

        /* Skip inVisibleWidow & ToolbarWindow >>> */
        bool isToolbarWindow = false;
        if (false == filename.isEmpty()
            && false == windowTitle.isEmpty()
            && true == isVisibleWindow
            && true == isExToolWindow) {
            isToolbarWindow = true;
        }
        /* Skip inVisibleWidow & ToolbarWindow <<< */

        bool GlobalMappingFlag = false;
        if (1 == ui->settingselectComboBox->currentIndex()
            && GROUPNAME_GLOBALSETTING == ui->settingselectComboBox->currentText()
            && 2 == checkresult) {
            GlobalMappingFlag = true;
        }

        if (checkresult){
            if (KEYMAP_CHECKING == m_KeyMapStatus || KEYMAP_MAPPING_GLOBAL == m_KeyMapStatus){
                if (GlobalMappingFlag) {
                    if (m_KeyMapStatus != KEYMAP_MAPPING_GLOBAL) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace() << "[cycleCheckProcessProc]" << " GlobalMappingFlag = " << GlobalMappingFlag << "," << " KeyMapStatus need to change [" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << "] -> [" << keymapstatusEnum.valueToKey(KEYMAP_MAPPING_GLOBAL) << "]";
#endif
                        setKeyHook(Q_NULLPTR);
                        m_KeyMapStatus = KEYMAP_MAPPING_GLOBAL;
                        s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
                        updateSystemTrayDisplay();
                        emit updateLockStatus_Signal();
                    }
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "[cycleCheckProcessProc]" << " checkresult = " << checkresult << "," << " KeyMapStatus need to change [" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << "] -> [" << keymapstatusEnum.valueToKey(KEYMAP_MAPPING_MATCHED) << "]" << ", ForegroundWindow: " << windowTitle << "(" << filename << ")";
                    qDebug().nospace() << "[cycleCheckProcessProc]" << " NameChecked = " << ui->nameCheckBox->isChecked() << "," << " TitleChecked = " << ui->titleCheckBox->isChecked();
                    qDebug().nospace() << "[cycleCheckProcessProc]" << " ProcessInfo.FileName = " << m_MapProcessInfo.FileName << "," << " ProcessInfo.WindowTitle = " << m_MapProcessInfo.WindowTitle;
                    qDebug().nospace() << "[cycleCheckProcessProc]" << " CurrentFileName = " << filename << "," << " CurrentWindowTitle = " << windowTitle;
                    qDebug().nospace() << "[cycleCheckProcessProc]" << " isVisibleWindow = " << isVisibleWindow << "," << " isExToolWindow =" << isExToolWindow;
                    qDebug().nospace() << "[cycleCheckProcessProc]" << " isToolbarWindow = " << isToolbarWindow;
#endif
                    if (isToolbarWindow) {
                        /* Add for "explorer.exe -> Program Manager" Bug Fix >>> */
                        if (filename == "explorer.exe"
                            && windowTitle == "Program Manager"
                            && isVisibleWindow == true
                            && isExToolWindow == true) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace() << "[cycleCheckProcessProc]" << "[BugFix] Do not skip ToolbarWindow \"explorer.exe -> Program Manager\" of KeyMapStatus(KEYMAP_MAPPING_MATCHED)";
#endif
                        }
                        /* Add for "explorer.exe -> Program Manager" Bug Fix <<< */
                        else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace() << "[cycleCheckProcessProc]" << " Skip ToolbarWindow of KeyMapStatus(KEYMAP_MAPPING_MATCHED)";
#endif
                            return;
                        }
                    }
                    playStartSound();
                    setKeyHook(hwnd);
                    m_KeyMapStatus = KEYMAP_MAPPING_MATCHED;
                    s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
                    updateSystemTrayDisplay();
                    emit updateLockStatus_Signal();
                }
            }
        }
        else{
            if (KEYMAP_MAPPING_MATCHED == m_KeyMapStatus){
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[cycleCheckProcessProc]" << " checkresult = " << checkresult << "," << " KeyMapStatus need to change [" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << "] -> [" << keymapstatusEnum.valueToKey(KEYMAP_CHECKING) << "]" << ", ForegroundWindow: " << windowTitle << "(" << filename << ")";
                qDebug().nospace() << "[cycleCheckProcessProc]" << " checkresult = " << checkresult << "," << " KeyMapStatus need to change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") " << "ForegroundWindow: " << windowTitle << "(" << filename << ")";
                qDebug().nospace() << "[cycleCheckProcessProc]" << " NameChecked = " << ui->nameCheckBox->isChecked() << "," << " TitleChecked = " << ui->titleCheckBox->isChecked();
                qDebug().nospace() << "[cycleCheckProcessProc]" << " ProcessInfo.FileName = " << m_MapProcessInfo.FileName << "," << " ProcessInfo.WindowTitle = " << m_MapProcessInfo.WindowTitle;
                qDebug().nospace() << "[cycleCheckProcessProc]" << " CurrentFileName = " << filename << "," << " CurrentWindowTitle = " << windowTitle;
                qDebug().nospace() << "[cycleCheckProcessProc]" << " isVisibleWindow = " << isVisibleWindow << "," << " isExToolWindow =" << isExToolWindow;
                qDebug().nospace() << "[cycleCheckProcessProc]" << " isToolbarWindow = " << isToolbarWindow;
#endif
                if (isToolbarWindow) {
                    /* Add for "explorer.exe -> Program Manager" Bug Fix >>> */
                    if (filename == "explorer.exe"
                        && windowTitle == "Program Manager"
                        && isVisibleWindow == true
                        && isExToolWindow == true) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace() << "[cycleCheckProcessProc]" << "[BugFix] Do not skip ToolbarWindow \"explorer.exe -> Program Manager\" of KeyMapStatus(KEYMAP_CHECKING)";
#endif
                    }
                    /* Add for "explorer.exe -> Program Manager" Bug Fix <<< */
                    else {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace() << "[cycleCheckProcessProc]" << " Skip ToolbarWindow of KeyMapStatus(KEYMAP_CHECKING)";
#endif
                        return;
                    }
                }
                playStopSound();
                setKeyUnHook();
                m_KeyMapStatus = KEYMAP_CHECKING;
                s_CycleCheckLoopCount = 0;
                updateSystemTrayDisplay();
                emit updateLockStatus_Signal();
            }
        }
    }
    else{
        //EnumWindows((WNDENUMPROC)QKeyMapper::EnumWindowsProc, 0);
    }

    if (m_KeyMapStatus == KEYMAP_CHECKING && GLOBAL_MAPPING_START_WAIT == s_CycleCheckLoopCount) {
        if (checkGlobalSettingAutoStart()) {
            loadSetting_flag = true;
            bool loadresult = loadKeyMapSetting(GROUPNAME_GLOBALSETTING);
            Q_UNUSED(loadresult);
            loadSetting_flag = false;
        }
    }

    s_CycleCheckLoopCount += 1;
    if (s_CycleCheckLoopCount > CYCLE_CHECK_LOOPCOUNT_MAX) {
        s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
    }
}

void QKeyMapper::cycleRefreshProcessInfoTableProc()
{
    if (false == isHidden()){
        refreshProcessInfoTable();
    }
}

void QKeyMapper::setKeyHook(HWND hWnd)
{
    updateShortcutsMap();

    emit QKeyMapper_Worker::getInstance()->setKeyHook_Signal(hWnd);
}

void QKeyMapper::setKeyUnHook(void)
{
    freeShortcuts();

    emit QKeyMapper_Worker::getInstance()->setKeyUnHook_Signal();
}

void QKeyMapper::setMapProcessInfo(const QString &filename, const QString &windowtitle, const QString &pid, const QString &filepath, const QIcon &windowicon)
{
    m_MapProcessInfo.PID = pid;
    m_MapProcessInfo.FilePath = filepath;

    if ((false == filename.isEmpty())
            && (false == windowtitle.isEmpty())
            && (false == windowicon.isNull())){
        m_MapProcessInfo.FileName = filename;
        m_MapProcessInfo.WindowTitle = windowtitle;
        m_MapProcessInfo.WindowIcon = windowicon;
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[setMapProcessInfo]"<< " Info Error: filename(" << filename << "), " << "windowtitle(" << windowtitle << ")";
#endif
    }
}

QString QKeyMapper::getExeFileDescription()
{
    QString exeFilePath = QCoreApplication::applicationFilePath();
    DWORD dummy;
    DWORD size = GetFileVersionInfoSize((LPCWSTR)exeFilePath.utf16(), &dummy);
    if (size == 0) {
        return QString();
    }

    QByteArray data(size, 0);
    if (!GetFileVersionInfo((LPCWSTR)exeFilePath.utf16(), 0, size, data.data())) {
        return QString();
    }

    void *value = nullptr;
    UINT length;
    if (!VerQueryValue(data.data(), L"\\StringFileInfo\\040904b0\\FileDescription", &value, &length)) {
        return QString();
    }

    QString retStr = QString::fromUtf16((ushort *)value, length);
    retStr.remove(QChar('\0'));
    return retStr;
}

#ifdef ADJUST_PRIVILEGES
BOOL QKeyMapper::AdjustPrivileges(void)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    TOKEN_PRIVILEGES oldtp;
    DWORD dwSize=sizeof(TOKEN_PRIVILEGES);
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
    //if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS_P, &hToken)) {
        if (GetLastError()==ERROR_CALL_NOT_IMPLEMENTED) return true;
        else return false;
    }
    if (!LookupPrivilegeValue(Q_NULLPTR, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }
    ZeroMemory(&tp, sizeof(tp));
    tp.PrivilegeCount=1;
    tp.Privileges[0].Luid=luid;
    tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
    /* Adjust Token Privileges */
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize)) {
        CloseHandle(hToken);
        return false;
    }
    // close handles
    CloseHandle(hToken);
    return true;
}

BOOL QKeyMapper::EnableDebugPrivilege(void)
{
  HANDLE hToken;
  BOOL fOk=FALSE;
  if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&hToken))
  {
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount=1;
    LookupPrivilegeValue(Q_NULLPTR,SE_DEBUG_NAME,&tp.Privileges[0].Luid);
    tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken,FALSE,&tp,sizeof(tp),Q_NULLPTR,Q_NULLPTR);

    fOk=(GetLastError()==ERROR_SUCCESS);
    CloseHandle(hToken);
  }
    return fOk;
}
#endif

void QKeyMapper::getProcessInfoFromPID(DWORD processID, QString &processPathStr)
{
    TCHAR szProcessPath[MAX_PATH] = TEXT("");
    TCHAR szImagePath[MAX_PATH] = TEXT("");

    // Get a handle to the process.
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );

#if 1
    if (Q_NULLPTR != hProcess )
    {
        if(!GetProcessImageFileName(hProcess, szImagePath, MAX_PATH))
        {
#ifdef DEBUG_LOGOUT_ON
            processPathStr = QString::fromWCharArray(szProcessPath);
            qDebug().nospace().noquote() << "[getProcessInfoFromPID]"<< " GetProcessImageFileName failure(" << processPathStr << ")";
#endif
            CloseHandle(hProcess);
            return;
        }

        if(!DosPathToNtPath(szImagePath, szProcessPath))
        {
#ifdef DEBUG_LOGOUT_ON
            processPathStr = QString::fromWCharArray(szProcessPath);
            qDebug().nospace().noquote() << "[getProcessInfoFromPID]"<< " DosPathToNtPath failure(" << processPathStr << ")";
#endif
            CloseHandle(hProcess);
            return;
        }

        processPathStr = QString::fromWCharArray(szProcessPath);
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[getProcessInfoFromPID]" << "OpenProcess Failure:" << hProcess << ", LastError:" << GetLastError();
#endif
    }
    CloseHandle( hProcess );

#else
    // Get the process name.
    if (Q_NULLPTR != hProcess )
    {
        GetModuleFileNameEx(hProcess, Q_NULLPTR, szProcessPath, MAX_PATH);
        //GetModuleFileName(hProcess, szProcessName, MAX_PATH);
        processPathStr = QString::fromWCharArray(szProcessPath);
    }

    // Release the handle to the process.
    CloseHandle( hProcess );
#endif
}

void QKeyMapper::getProcessInfoFromHWND(HWND hWnd, QString &processPathStr)
{
    DWORD dwProcessId = 0;
    TCHAR szProcessPath[MAX_PATH] = TEXT("");
    TCHAR szImagePath[MAX_PATH] = TEXT("");

    GetWindowThreadProcessId(hWnd, &dwProcessId);

    // Get a handle to the process.
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, dwProcessId );

    if (Q_NULLPTR != hProcess )
    {
        if(!GetProcessImageFileName(hProcess, szImagePath, MAX_PATH))
        {
            CloseHandle(hProcess);
            return;
        }

        if(!DosPathToNtPath(szImagePath, szProcessPath))
        {
            CloseHandle(hProcess);
            return;
        }

        processPathStr = QString::fromWCharArray(szProcessPath);
    }
    CloseHandle( hProcess );
}

HWND QKeyMapper::getHWND_byPID(DWORD dwProcessID)
{
    HWND hWnd = GetTopWindow(Q_NULLPTR);
    while ( hWnd )
    {
        DWORD pid = 0;
        DWORD dwTheardId = GetWindowThreadProcessId( hWnd,&pid);
        if (dwTheardId != 0)
        {
            if ( pid == dwProcessID/*your process id*/ )
            {
                // here h is the handle to the window

                if (GetTopWindow(hWnd))
                {
                    return hWnd;
                }
               // return h;
            }
        }
        hWnd = ::GetNextWindow( hWnd , GW_HWNDNEXT);
    }
    return Q_NULLPTR;
}

BOOL QKeyMapper::IsAltTabWindow(HWND hWnd)
{
    //  It must be a visible Window
    if (!IsWindowVisible(hWnd))
        return FALSE;

    //  It must not be a Tool bar window
    WINDOWINFO winInfo;
    winInfo.cbSize = sizeof(WINDOWINFO);
    if (GetWindowInfo(hWnd, &winInfo)) {
        if ((winInfo.dwExStyle & WS_EX_TOOLWINDOW) != 0)
            return FALSE;

        //  It must not be a cloaked window
        BOOL isCloaked = FALSE;
        HRESULT hr = DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(BOOL));
        if (SUCCEEDED(hr)) {
            return isCloaked == FALSE;
        }
        else {
            return FALSE;
        }
    }
    else {
        return FALSE;
    }
}

BOOL QKeyMapper::EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    //EnumChildWindows(hWnd, (WNDENUMPROC)QKeyMapper::EnumChildWindowsProc, 0);

    Q_UNUSED(lParam);

    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);

    if(FALSE == IsWindowVisible(hWnd)){
//#ifdef DEBUG_LOGOUT_ON
//        qDebug().nospace().noquote() << "[EnumWindowsProc] " << "(Invisible window)" << " [PID:" << dwProcessId <<"]";
//#endif
        return TRUE;
    }

    QString WindowText;
    QString ProcessPath;
    QString filename;
    TCHAR titleBuffer[MAX_PATH] = TEXT("");
    memset(titleBuffer, 0x00, sizeof(titleBuffer));

    getProcessInfoFromPID(dwProcessId, ProcessPath);
    int resultLength = GetWindowText(hWnd, titleBuffer, MAX_PATH);
    if (resultLength){
        WindowText = QString::fromWCharArray(titleBuffer);

        if ((false == WindowText.isEmpty())
                && (false == ProcessPath.isEmpty())
                ){
            MAP_PROCESSINFO ProcessInfo;
            QFileInfo fileinfo(ProcessPath);
            filename = fileinfo.fileName();

            ProcessInfo.FileName = filename;
            ProcessInfo.PID = QString::number(dwProcessId);
            ProcessInfo.WindowTitle = WindowText;
            ProcessInfo.FilePath = ProcessPath;

            HICON iconptr = (HICON)(LONG_PTR)GetClassLongPtr(hWnd, GCLP_HICON);
            if (iconptr != Q_NULLPTR){
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                ProcessInfo.WindowIcon = QIcon(QPixmap::fromImage(QImage::fromHICON(iconptr)));
#else
                ProcessInfo.WindowIcon = QIcon(QtWin::fromHICON(iconptr));
#endif
            }
            else{
                QFileIconProvider icon_provider;
                QIcon fileicon = icon_provider.icon(QFileInfo(ProcessPath));

                if (false == fileicon.isNull()){
                    ProcessInfo.WindowIcon = fileicon;
                }
            }

            if (ProcessInfo.WindowIcon.isNull() != true){
                BOOL isVisibleWindow = TRUE;
                bool isWin10Above = false;
                QOperatingSystemVersion osVersion = QOperatingSystemVersion::current();
                if (osVersion >= QOperatingSystemVersion::Windows10) {
                    isWin10Above = true;
                }
                else {
                    isWin10Above = false;
                }

                if (isWin10Above) {
                    isVisibleWindow = IsAltTabWindow(hWnd);
                    if (TRUE == isVisibleWindow) {
                        static_ProcessInfoList.append(ProcessInfo);
                    }
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[EnumWindowsProc] Windows version Win10 and above.";
#endif
                }
                else {
                    static_ProcessInfoList.append(ProcessInfo);
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[EnumWindowsProc] Windows version lower than Win10 (Win8.1/Win8/Win7...)";
#endif
                }

#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[EnumWindowsProc] " << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << "), " << "IsAltTabWindow -> " << (isVisibleWindow == TRUE);
#endif
            }
            else{
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[EnumWindowsProc] " << "(WindowIcon NULL)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
            }
        }
        else{
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[EnumWindowsProc] " << "(ProcessPath empty)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
        }
    }
    else{
//#ifdef DEBUG_LOGOUT_ON
//        WindowText = QString::fromWCharArray(titleBuffer);
//        qDebug().nospace().noquote() << "[EnumWindowsProc] " << "(WindowTitle empty)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
//#endif
    }

    return TRUE;
}

BOOL QKeyMapper::EnumChildWindowsProc(HWND hWnd, LPARAM lParam)
{
    Q_UNUSED(lParam);
    QString WindowText;
    QString ProcessPath;
    QString filename;
    DWORD dwProcessId = 0;
    TCHAR titleBuffer[MAX_PATH] = TEXT("");
    memset(titleBuffer, 0x00, sizeof(titleBuffer));

    GetWindowThreadProcessId(hWnd, &dwProcessId);
    getProcessInfoFromPID(dwProcessId, ProcessPath);
    int resultLength = GetWindowText(hWnd, titleBuffer, MAX_PATH);
    if (resultLength){
        WindowText = QString::fromWCharArray(titleBuffer);

        if ((false == WindowText.isEmpty())
                && (false == ProcessPath.isEmpty())){
            MAP_PROCESSINFO ProcessInfo;
            QFileInfo fileinfo(ProcessPath);
            filename = fileinfo.fileName();

            ProcessInfo.FileName = filename;
            ProcessInfo.PID = QString::number(dwProcessId);
            ProcessInfo.WindowTitle = WindowText;
            ProcessInfo.FilePath = ProcessPath;

            HICON iconptr = (HICON)(LONG_PTR)GetClassLongPtr(hWnd, GCLP_HICON);
            if (iconptr != Q_NULLPTR){
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                ProcessInfo.WindowIcon = QIcon(QPixmap::fromImage(QImage::fromHICON(iconptr)));
#else
                ProcessInfo.WindowIcon = QIcon(QtWin::fromHICON(iconptr));
#endif
            }
            else{
                QFileIconProvider icon_provider;
                QIcon fileicon = icon_provider.icon(QFileInfo(ProcessPath));

                if (false == fileicon.isNull()){
                    ProcessInfo.WindowIcon = fileicon;
                }
            }

            if(FALSE == IsWindowVisible(hWnd)){
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[EnumChildWindowsProc] " << "(Invisible window)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
                return TRUE;
            }

            if (ProcessInfo.WindowIcon.isNull() != true){
                static_ProcessInfoList.append(ProcessInfo);

#ifdef DEBUG_LOGOUT_ON
                if (iconptr != Q_NULLPTR){
                    qDebug().nospace().noquote() << "[EnumChildWindowsProc] " << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
                }
                else{
//                    qDebug().nospace().noquote() << "[EnumChildWindowsProc] " << "(HICON pointer NULL)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
                }
#endif
            }
            else{
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[EnumChildWindowsProc] " << "(WindowIcon NULL)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
            }
        }
        else{
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[EnumChildWindowsProc] " << "(ProcessPath empty)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
        }
    }
    else{
//#ifdef DEBUG_LOGOUT_ON
//        WindowText = QString::fromWCharArray(titleBuffer);
//        qDebug().nospace().noquote() << "[EnumChildWindowsProc] " << "(WindowTitle empty)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
//#endif
    }

    return TRUE;
}

BOOL QKeyMapper::DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath)
{
    TCHAR           szDriveStr[500];
    TCHAR           szDrive[3];
    TCHAR           szDevName[100];
    size_t          cchDevName;
    INT             i;

    if(!pszDosPath || !pszNtPath )
        return FALSE;

    if(GetLogicalDriveStrings(sizeof(szDriveStr), szDriveStr))
    {
        for(i = 0; szDriveStr[i]; i += 4)
        {
            if(!lstrcmpi(&(szDriveStr[i]), TEXT("A:\\")) || !lstrcmpi(&(szDriveStr[i]), TEXT("B:\\")))
                continue;

            szDrive[0] = szDriveStr[i];
            szDrive[1] = szDriveStr[i + 1];
            szDrive[2] = '\0';
            if(!QueryDosDevice(szDrive, szDevName, 100))
                return FALSE;

            cchDevName = (size_t)lstrlen(szDevName);
            if(_wcsnicmp(pszDosPath, szDevName, cchDevName) == 0)
            {
                lstrcpy(pszNtPath, szDrive);
                lstrcat(pszNtPath, pszDosPath + cchDevName);

                return TRUE;
            }
        }
    }

    lstrcpy(pszNtPath, pszDosPath);

    return FALSE;
}

int QKeyMapper::findOriKeyInKeyMappingDataList(const QString &keyname)
{
    int returnindex = -1;
    int keymapdataindex = 0;

    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
    {
        if (keymapdata.Original_Key == keyname){
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}

int QKeyMapper::findOriKeyInKeyMappingDataListGlobal(const QString &keyname)
{
    int returnindex = -1;
    int keymapdataindex = 0;

    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataListGlobal))
    {
        if (keymapdata.Original_Key == keyname){
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}

int QKeyMapper::findMapKeyInKeyMappingDataList(const QString &keyname)
{
    int returnindex = -1;
    int keymapdataindex = 0;

    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
    {
        if (keymapdata.Mapping_Keys.contains(keyname)){
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}

void QKeyMapper::EnumProcessFunction(void)
{
#if 0
    DWORD aProcesses[1024], cbNeeded, cProcesses;

    if (EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) != 0)
    {
        // Calculate how many process identifiers were returned.
        cProcesses = cbNeeded / sizeof(DWORD);
        // Print the name and process identifier for each process.
        unsigned int i;
        for ( i = 0; i < cProcesses; i++ )
        {
            QString ProcessName;
            QString WindowText;
            getProcessInfoFromPID( aProcesses[i], ProcessName);
            WindowText = GetWindowTextByProcessId(aProcesses[i]);

            //if (false == WindowText.isEmpty() && false == ProcessName.isEmpty()){
            qDebug().nospace().noquote() << WindowText <<" [PID:" << aProcesses[i] <<"]" << "(" << ProcessName << ")";
            //}
        }
    }
#endif

#if 1
    PROCESSENTRY32 pe32;
    HWND hWnd;

    pe32.dwSize = sizeof(pe32);
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(hProcessSnap != INVALID_HANDLE_VALUE)
    {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[EnumProcessFunction]" << "CreateToolhelp32Snapshot List Start >>>";
#endif

        BOOL bMore = Process32First(hProcessSnap,&pe32);
        while(bMore)
        {
            QString ProcessPath;
            QString WindowText;
            QString filename;

            getProcessInfoFromPID(pe32.th32ProcessID, ProcessPath);
            hWnd = getHWND_byPID(pe32.th32ProcessID);
            filename = QString::fromWCharArray(pe32.szExeFile);

            DWORD dwProcessId = pe32.th32ProcessID;
            TCHAR titleBuffer[MAX_PATH] = TEXT("");
            memset(titleBuffer, 0x00, sizeof(titleBuffer));

            int resultLength = GetWindowText(hWnd, titleBuffer, MAX_PATH);
            if (resultLength){
                WindowText = QString::fromWCharArray(titleBuffer);

                if ((false == WindowText.isEmpty())
                        && (false == ProcessPath.isEmpty())){
                    MAP_PROCESSINFO ProcessInfo;
                    ProcessInfo.FileName = filename;
                    ProcessInfo.PID = QString::number(dwProcessId);
                    ProcessInfo.WindowTitle = WindowText;
                    ProcessInfo.FilePath = ProcessPath;

                    HICON iconptr = (HICON)(LONG_PTR)GetClassLongPtr(hWnd, GCLP_HICON);
                    if (iconptr != Q_NULLPTR){
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                        ProcessInfo.WindowIcon = QIcon(QPixmap::fromImage(QImage::fromHICON(iconptr)));
#else
                        ProcessInfo.WindowIcon = QIcon(QtWin::fromHICON(iconptr));
#endif
                    }
                    else{
                        QFileIconProvider icon_provider;
                        QIcon fileicon = icon_provider.icon(QFileInfo(ProcessPath));

                        if (false == fileicon.isNull()){
                            ProcessInfo.WindowIcon = fileicon;
                        }
                    }

                    //            if(FALSE == IsWindowVisible(hWnd)){
                    //#ifdef DEBUG_LOGOUT_ON
                    //                qDebug().nospace().noquote() << "[EnumWindowsProc] " << "(Invisible window)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
                    //#endif
                    //                return TRUE;
                    //            }

                    if (ProcessInfo.WindowIcon.isNull() != true){
                        static_ProcessInfoList.append(ProcessInfo);

#ifdef DEBUG_LOGOUT_ON
                        if (iconptr != Q_NULLPTR){
                            qDebug().nospace().noquote() << "[EnumProcessFunction] " << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
                        }
                        else{
//                            qDebug().nospace().noquote() << "[EnumProcessFunction] " << "(HICON pointer NULL)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
                        }
#endif
                    }
                    else{
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "[EnumProcessFunction] " << "(WindowIcon NULL)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
                    }
                }
                else{
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace().noquote() << "[EnumProcessFunction] " << "(ProcessPath empty)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
                }
            }
            else{
//#ifdef DEBUG_LOGOUT_ON
//                WindowText = QString::fromWCharArray(titleBuffer);
//                qDebug().nospace().noquote() << "[EnumProcessFunction] " << "(WindowTitle empty)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
//#endif
            }

            bMore = Process32Next(hProcessSnap,&pe32);
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[EnumProcessFunction]" << "CreateToolhelp32Snapshot List End <<<";
#endif
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[EnumProcessFunction]" << "CreateToolhelp32Snapshot Failure!!";
#endif
    }
    CloseHandle(hProcessSnap);
#endif

}

Qt::CheckState QKeyMapper::getAutoStartMappingStatus()
{
    return ui->autoStartMappingCheckBox->checkState();
}

bool QKeyMapper::getDisableWinKeyStatus()
{
    if (m_isDestructing) {
        return false;
    }

    if (true == getInstance()->ui->disableWinKeyCheckBox->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}

int QKeyMapper::getBurstPressTime()
{
    return getInstance()->ui->burstpressSpinBox->value();
}

int QKeyMapper::getBurstReleaseTime()
{
    return getInstance()->ui->burstreleaseSpinBox->value();
}

int QKeyMapper::getJoystick2MouseSpeedX()
{
    return getInstance()->ui->mouseXSpeedSpinBox->value();
}

int QKeyMapper::getJoystick2MouseSpeedY()
{
    return getInstance()->ui->mouseYSpeedSpinBox->value();
}

int QKeyMapper::getvJoyXSensitivity()
{
    return getInstance()->ui->vJoyXSensSpinBox->value();
}

int QKeyMapper::getvJoyYSensitivity()
{
    return getInstance()->ui->vJoyYSensSpinBox->value();
}

QString QKeyMapper::getVirtualGamepadType()
{
    return getInstance()->ui->virtualGamepadTypeComboBox->currentText();
}

bool QKeyMapper::getLockCursorStatus()
{
    if (true == getInstance()->ui->lockCursorCheckBox->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}

int QKeyMapper::getGlobalSettingAutoStart()
{
    return s_GlobalSettingAutoStart;
}

bool QKeyMapper::checkGlobalSettingAutoStart()
{
    if (s_GlobalSettingAutoStart == Qt::Checked && false == KeyMappingDataListGlobal.isEmpty()) {
        return true;
    }
    else {
        return false;
    }
}

int QKeyMapper::getDataPortNumber()
{
    return getInstance()->ui->dataPortSpinBox->value();
}

double QKeyMapper::getBrakeThreshold()
{
    return getInstance()->ui->brakeThresholdDoubleSpinBox->value();
}

double QKeyMapper::getAccelThreshold()
{
    return getInstance()->ui->accelThresholdDoubleSpinBox->value();
}

void QKeyMapper::changeEvent(QEvent *event)
{
    if(event->type()==QEvent::WindowStateChange)
    {
        QTimer::singleShot(0, this, SLOT(WindowStateChangedProc()));
    }
    QDialog::changeEvent(event);
}

void QKeyMapper::keyPressEvent(QKeyEvent *event)
{
   if (event->key() == Qt::Key_F5) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyPressEvent]" << "F5 Key Pressed -> refreshProcessInfoTable()";
#endif
#ifndef DEBUG_LOGOUT_ON
        m_ProcessInfoTableRefreshTimer.start(CYCLE_REFRESH_PROCESSINFOTABLE_TIMEOUT);
#endif
        refreshProcessInfoTable();

   }
   else if (event->key() != Qt::Key_Escape) {
       QDialog::keyPressEvent(event);
   }
}

void QKeyMapper::on_keymapButton_clicked()
{
    MappingStart(MAPPINGSTART_BUTTONCLICK);
}

void QKeyMapper::MappingStart(MappingStartMode startmode)
{
    Q_UNUSED(startmode);
    QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
    QMetaEnum mappingstartmodeEnum = QMetaEnum::fromType<QKeyMapper::MappingStartMode>();
    bool startKeyMap = false;

    if (KEYMAP_IDLE == m_KeyMapStatus){
        bool fileNameCheckOK = true;
        bool windowTitleCheckOK = true;
        bool fileNameExist = !m_MapProcessInfo.FileName.isEmpty();
        bool windowTitleExist = !m_MapProcessInfo.WindowTitle.isEmpty();

        if (ui->nameCheckBox->checkState() == Qt::Checked && false == fileNameExist) {
            fileNameCheckOK = false;
        }

        if (ui->titleCheckBox->checkState() == Qt::Checked && false == windowTitleExist) {
            windowTitleCheckOK = false;
        }

        if (true == fileNameCheckOK && true == windowTitleCheckOK){
            m_CycleCheckTimer.start(CYCLE_CHECK_TIMEOUT);
            if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                ui->keymapButton->setText(KEYMAPBUTTON_STOP_ENGLISH);
            }
            else {
                ui->keymapButton->setText(KEYMAPBUTTON_STOP_CHINESE);
            }
            m_KeyMapStatus = KEYMAP_CHECKING;
            s_CycleCheckLoopCount = 0;
            updateSystemTrayDisplay();
            emit updateLockStatus_Signal();
            startKeyMap = true;

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[MappingStart]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") -> " << mappingstartmodeEnum.valueToKey(startmode);
#endif
        }
        else{
            if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                QMessageBox::warning(this, PROGRAM_NAME, "Please doubleclick process info table to select valid processinfo for key mapping.");
            }
            else {
                QMessageBox::warning(this, PROGRAM_NAME, "请双击进程列表为按键映射选择有效的进程信息。");
            }
        }
    }
    else{
        m_CycleCheckTimer.stop();
        m_SysTrayIcon->setToolTip("QKeyMapper(Idle)");
        m_SysTrayIcon->setIcon(QIcon(":/QKeyMapper.ico"));
        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
            ui->keymapButton->setText(KEYMAPBUTTON_START_ENGLISH);
        }
        else {
            ui->keymapButton->setText(KEYMAPBUTTON_START_CHINESE);
        }

        if (KEYMAP_MAPPING_MATCHED == m_KeyMapStatus) {
            playStopSound();
        }
        setKeyUnHook();
        m_KeyMapStatus = KEYMAP_IDLE;
        s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
        emit updateLockStatus_Signal();

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[MappingStart]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") -> " << mappingstartmodeEnum.valueToKey(startmode);
#endif
    }

    if (m_KeyMapStatus != KEYMAP_IDLE){
        changeControlEnableStatus(false);
    }
    else{
        changeControlEnableStatus(true);
#ifdef VIGEM_CLIENT_SUPPORT
        emit updateViGEmBusStatus_Signal();
#endif
    }

    if (true == startKeyMap) {
        cycleCheckProcessProc();
    }
}

void QKeyMapper::HotKeyActivated(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers)
{
    Q_UNUSED(keyseqstr);
    Q_UNUSED(modifiers);

    if (false == isHidden()){
        m_LastWindowPosition = pos(); // Save the current position before hiding
        hide();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[HotKeyActivated] Hide Window, LastWindowPosition ->" << m_LastWindowPosition;
#endif
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[HotKeyActivated] Show Window, LastWindowPosition ->" << m_LastWindowPosition;
#endif
        if (m_LastWindowPosition.x() != INITIAL_WINDOW_POSITION && m_LastWindowPosition.y() != INITIAL_WINDOW_POSITION) {
            move(m_LastWindowPosition); // Restore the position before showing
        }
        showNormal();
        activateWindow();
        raise();
    }
}

void QKeyMapper::HotKeyStartStopActivated(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers)
{
    QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
    Q_UNUSED(keyseqstr);
    Q_UNUSED(keymapstatusEnum);
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[HotKeyStartStopActivated] Shortcut[" << keyseqstr << "], KeyboardModifiers = " << modifiers <<", KeyMapStatus = " << keymapstatusEnum.valueToKey(m_KeyMapStatus);
#endif

    MappingStart(MAPPINGSTART_HOTKEY);

    /* Add for "explorer.exe" AltModifier Bug Fix >>> */
    HWND hwnd = GetForegroundWindow();
    if (hwnd == NULL) {
        return;
    }

    QString filename;
    QString ProcessPath;
    getProcessInfoFromHWND( hwnd, ProcessPath);

    if (false == ProcessPath.isEmpty()){
        QFileInfo fileinfo(ProcessPath);
        filename = fileinfo.fileName();
    }

    if ("explorer.exe" == filename) {
        bool isVisibleWindow = IsWindowVisible(hwnd);
        bool isExToolWindow = false;

        WINDOWINFO winInfo;
        winInfo.cbSize = sizeof(WINDOWINFO);
        if (GetWindowInfo(hwnd, &winInfo)) {
            if ((winInfo.dwExStyle & WS_EX_TOOLWINDOW) != 0)
                isExToolWindow = true;
        }

        if (isVisibleWindow && !isExToolWindow) {
            if (modifiers.testFlag(Qt::AltModifier)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[HotKeyStartStopActivated] AltModifier & ForegroundWindow Process -> " << filename << ", isVisibleWindow = " << isVisibleWindow << ", isExToolWindow = " << isExToolWindow;
#endif
                const Qt::KeyboardModifiers modifiers_arg = Qt::AltModifier;
                QKeyMapper_Worker::getInstance()->releaseKeyboardModifiers(modifiers_arg);
            }
        }
    }
    /* Add for "explorer.exe" AltModifier Bug Fix <<< */
}

void QKeyMapper::onWindowSwitchKeySequenceChanged(const QKeySequence &keysequence)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onWindowSwitchKeySequenceChanged] KeySequence changed ->" << keysequence.toString(QKeySequence::NativeText);
#endif
    Q_UNUSED(keysequence);

    if (false == m_windowswitchKeySeqEdit->keySequence().isEmpty()) {
        updateWindowSwitchKeySeq(m_windowswitchKeySeqEdit->keySequence());
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onWindowSwitchKeySequenceChanged]" << "Set Window Switch KeySequence ->" << m_windowswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText);
#endif
    }
    m_windowswitchKeySeqEdit->clearFocus();
}

void QKeyMapper::onWindowSwitchKeySequenceEditingFinished()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onWindowSwitchKeySequenceEditingFinished] Current KeySequence ->" << m_windowswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText);
#endif

    if (m_windowswitchKeySeqEdit->keySequence().isEmpty()) {
        if (m_windowswitchKeySeqEdit->lastKeySequence().isEmpty()) {
            m_windowswitchKeySeqEdit->setKeySequence(QKeySequence(m_windowswitchKeySeqEdit->defaultKeySequence()));
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[onWindowSwitchKeySequenceEditingFinished]" << "Last KeySequence is Empty, set to DEFAULT ->" << m_windowswitchKeySeqEdit->defaultKeySequence();
#endif
        }
        else {
            m_windowswitchKeySeqEdit->setKeySequence(QKeySequence(m_windowswitchKeySeqEdit->lastKeySequence()));
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[onWindowSwitchKeySequenceEditingFinished]" << "Current KeySequence is Empty, set to LAST ->" << m_windowswitchKeySeqEdit->lastKeySequence();
#endif
        }
    }
    m_windowswitchKeySeqEdit->clearFocus();
}

void QKeyMapper::onMappingSwitchKeySequenceChanged(const QKeySequence &keysequence)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onMappingSwitchKeySequenceChanged] KeySequence changed ->" << keysequence.toString(QKeySequence::NativeText);
#endif
    Q_UNUSED(keysequence);

    if (false == m_mappingswitchKeySeqEdit->keySequence().isEmpty()) {
        updateMappingSwitchKeySeq(m_mappingswitchKeySeqEdit->keySequence());
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onMappingSwitchKeySequenceChanged]" << "Set Mapping Switch KeySequence ->" << m_mappingswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText);
#endif
    }
    m_mappingswitchKeySeqEdit->clearFocus();
}

void QKeyMapper::onMappingSwitchKeySequenceEditingFinished()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onMappingSwitchKeySequenceEditingFinished] Current KeySequence ->" << m_mappingswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText);
#endif

    if (m_mappingswitchKeySeqEdit->keySequence().isEmpty()) {
        if (m_mappingswitchKeySeqEdit->lastKeySequence().isEmpty()) {
            m_mappingswitchKeySeqEdit->setKeySequence(QKeySequence(m_mappingswitchKeySeqEdit->defaultKeySequence()));
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[onMappingSwitchKeySequenceEditingFinished]" << "Last KeySequence is Empty, set to DEFAULT ->" << m_mappingswitchKeySeqEdit->defaultKeySequence();
#endif
        }
        else {
            m_mappingswitchKeySeqEdit->setKeySequence(QKeySequence(m_mappingswitchKeySeqEdit->lastKeySequence()));
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[onMappingSwitchKeySequenceEditingFinished]" << "Current KeySequence is Empty, set to LAST ->" << m_mappingswitchKeySeqEdit->lastKeySequence();
#endif
        }
    }
    m_mappingswitchKeySeqEdit->clearFocus();
}

void QKeyMapper::onOriginalKeySequenceChanged(const QKeySequence &keysequence)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onOriginalKeySequenceChanged] KeySequence changed ->" << keysequence.toString(QKeySequence::NativeText);
#endif
    Q_UNUSED(keysequence);
    m_originalKeySeqEdit->clearFocus();
}

void QKeyMapper::onOriginalKeySequenceEditingFinished()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onOriginalKeySequenceEditingFinished] Current KeySequence ->" << m_originalKeySeqEdit->keySequence().toString(QKeySequence::NativeText);
#endif

    if (m_originalKeySeqEdit->keySequence().isEmpty()) {
        if (m_originalKeySeqEdit->lastKeySequence().isEmpty()) {
            m_originalKeySeqEdit->clear();
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[onOriginalKeySequenceEditingFinished]" << "Last KeySequence is Empty, Clear it!!!";
#endif
        }
        else {
            m_originalKeySeqEdit->setKeySequence(QKeySequence(m_originalKeySeqEdit->lastKeySequence()));
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[onOriginalKeySequenceEditingFinished]" << "Current KeySequence is Empty, set to LAST ->" << m_originalKeySeqEdit->lastKeySequence();
#endif
        }
    }
    else if (m_originalKeySeqEdit->keySequence().toString(QKeySequence::NativeText) == m_originalKeySeqEdit->defaultKeySequence()) {
        m_originalKeySeqEdit->clear();
    }
    else {
        m_originalKeySeqEdit->setLastKeySequence(m_originalKeySeqEdit->keySequence().toString());
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onOriginalKeySequenceEditingFinished]" << "Set Mapping Switch KeySequence ->" << m_originalKeySeqEdit->keySequence().toString(QKeySequence::NativeText);
#endif
    }
    m_originalKeySeqEdit->clearFocus();

}

void QKeyMapper::SystrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (QSystemTrayIcon::DoubleClick == reason){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[SystrayIconActivated]" << "SystemTray double clicked: showNormal()!!";
#endif

        showNormal();
        activateWindow();
        raise();
    }
}

void QKeyMapper::cellChanged_slot(int row, int col)
{
#if 0
    if ((col == BURST_MODE_COLUMN || col == LOCK_COLUMN)
            && ui->keymapdataTable->item(row, col)->checkState() == Qt::Checked) {
        if (KeyMappingDataList[row].Mapping_Keys.size() > 1) {
            KeyMappingDataList[row].Burst = false;
            KeyMappingDataList[row].Lock = false;
            QTableWidgetItem *burstCheckBox = new QTableWidgetItem();
            burstCheckBox->setCheckState(Qt::Unchecked);
            ui->keymapdataTable->setItem(row, BURST_MODE_COLUMN    , burstCheckBox);
            QTableWidgetItem *lockCheckBox = new QTableWidgetItem();
            lockCheckBox->setCheckState(Qt::Unchecked);
            ui->keymapdataTable->setItem(row, LOCK_COLUMN    , lockCheckBox);

            QString message = "Key sequence with \"»\" do not support Burst or Lock mode!";
            QMessageBox::warning(this, PROGRAM_NAME, tr(message.toStdString().c_str()));
#ifdef DEBUG_LOGOUT_ON
            qDebug("[%s]: row(%d) could not set burst or lock for key sequence(%d)", __func__, row, KeyMappingDataList[row].Mapping_Keys.size());
#endif
            return;
        }
    }
#endif

    if (col == BURST_MODE_COLUMN) {
        bool burst = false;
        if (ui->keymapdataTable->item(row, col)->checkState() == Qt::Checked) {
            burst = true;
        }
        else {
            burst = false;
        }

        if (burst != KeyMappingDataList.at(row).Burst) {
            KeyMappingDataList[row].Burst = burst;
#ifdef DEBUG_LOGOUT_ON
            qDebug("[%s]: row(%d) burst changed to (%s)", __func__, row, burst == true?"ON":"OFF");
#endif
        }
    }
    else if (col == LOCK_COLUMN) {
        bool lock = false;
        if (ui->keymapdataTable->item(row, col)->checkState() == Qt::Checked) {
            lock = true;
        }
        else {
            lock = false;
        }

        if (lock != KeyMappingDataList.at(row).Lock) {
            KeyMappingDataList[row].Lock = lock;
#ifdef DEBUG_LOGOUT_ON
            qDebug("[%s]: row(%d) lock changed to (%s)", __func__, row, lock == true?"ON":"OFF");
#endif
        }
    }
}

#ifdef VIGEM_CLIENT_SUPPORT
void QKeyMapper::OrikeyComboBox_currentTextChangedSlot(const QString &text)
{
    if (VJOY_MOUSE2LS_STR == text
        || VJOY_MOUSE2RS_STR == text
        || JOY_LS2MOUSE_STR == text
        || JOY_RS2MOUSE_STR == text
        || JOY_LT2VJOYLT_STR == text
        || JOY_RT2VJOYRT_STR == text) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[OriKeyListComboBox_currentTextChanged]" << "Text ->" << text;
#endif
        m_mapkeyComboBox->setCurrentText(QString());
        m_mapkeyComboBox->setEnabled(false);
    }
    else {
        m_mapkeyComboBox->setEnabled(true);
    }
}
#endif

bool QKeyMapper::backupFile(const QString &originalFile, const QString &backupFile)
{
    return QFile::copy(originalFile, backupFile);
}

bool QKeyMapper::checkSettingsFileNeedtoConvert()
{
    bool ret = false;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    QStringList groups = settingFile.childGroups();

    bool containsOldMouseButton = false;
    bool endsWithEXE = false;
    bool containsTitleGroup = false;

    bool shouldBreak = false;
    for (const QString &group : qAsConst(groups)) {
        if (shouldBreak) break;
        settingFile.beginGroup(group);
        QStringList keys = {KEYMAPDATA_ORIGINALKEYS, KEYMAPDATA_MAPPINGKEYS};
        for (const QString &key : keys) {
            if (shouldBreak) break;
            if (settingFile.contains(key)) {
                QStringList values = settingFile.value(key).toStringList();
                for (QString &value : values) {
                    if (QKeyMapper_Worker::MouseButtonNameConvertMap.contains(value)) {
                        containsOldMouseButton = true;
                        shouldBreak = true;
                        break;
                    }
                }
            }
        }
        settingFile.endGroup();
    }

    for (const QString &group : qAsConst(groups)){
        if (group.endsWith(GROUPNAME_EXECUTABLE_SUFFIX, Qt::CaseInsensitive)) {
            endsWithEXE = true;
        }

        if (group.contains(GROUPNAME_EXECUTABLE_SUFFIX + QString(SEPARATOR_TITLESETTING) + WINDOWTITLE_STRING)
            || group.contains(GROUPNAME_EXECUTABLE_SUFFIX + QString(SEPARATOR_TITLESETTING) + ANYWINDOWTITLE_STRING)) {
            containsTitleGroup = true;
        }
    }

    if (containsOldMouseButton) {
        ret = true;
    }

    if (endsWithEXE && containsTitleGroup == false) {
        ret = true;
    }

    return ret;
}

void QKeyMapper::renameSettingsGroup(QSettings &settings, const QString &oldName, const QString &newName)
{
    settings.beginGroup(oldName);
    QStringList keys = settings.allKeys();
    QMap<QString, QVariant> values;
    for (const QString &key : keys) {
        values[key] = settings.value(key);
    }
    settings.endGroup();

    settings.beginGroup(newName);
    for (const QString &key : keys) {
        settings.setValue(key, values[key]);
    }
    settings.endGroup();

    settings.remove(oldName);
}

void QKeyMapper::convertSettingsFile()
{
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    QStringList groups = settingFile.childGroups();

    for (const QString &group : qAsConst(groups)) {
        settingFile.beginGroup(group);
        QStringList keys = {KEYMAPDATA_ORIGINALKEYS, KEYMAPDATA_MAPPINGKEYS};
        for (const QString &key : keys) {
            if (settingFile.contains(key)) {
                QStringList values = settingFile.value(key).toStringList();
                for (QString &value : values) {
                    if (QKeyMapper_Worker::MouseButtonNameConvertMap.contains(value)) {
                        value = QKeyMapper_Worker::MouseButtonNameConvertMap[value];
                    }
                }
                settingFile.setValue(key, values);
            }
        }
        settingFile.endGroup();
    }

    for (const QString &group : qAsConst(groups)){
        if (group.endsWith(GROUPNAME_EXECUTABLE_SUFFIX, Qt::CaseInsensitive)) {
            QVariant nameChecked_Var;
            QVariant titleChecked_Var;
            bool nameChecked = false;
            bool titleChecked = false;
            QString newGroupName;
            if (readSaveSettingData(group, PROCESSINFO_FILENAME_CHECKED, nameChecked_Var)) {
                nameChecked = nameChecked_Var.toBool();
            }
            if (readSaveSettingData(group, PROCESSINFO_WINDOWTITLE_CHECKED, titleChecked_Var)) {
                titleChecked = titleChecked_Var.toBool();
            }

            if (nameChecked && titleChecked) {
                newGroupName = group + SEPARATOR_TITLESETTING + QString(WINDOWTITLE_STRING) + QString::number(1);
            }
            else if (nameChecked && titleChecked == false) {
                newGroupName = group + SEPARATOR_TITLESETTING + QString(ANYWINDOWTITLE_STRING);
            }

            if (newGroupName.isEmpty() == false) {
                renameSettingsGroup(settingFile, group, newGroupName);
            }
        }

        if (group.startsWith(GROUPNAME_CUSTOMSETTING, Qt::CaseInsensitive)
            && group.endsWith(GROUPNAME_EXECUTABLE_SUFFIX, Qt::CaseInsensitive) != true) {
            QString newGroupName = group;
            newGroupName = newGroupName.replace(GROUPNAME_CUSTOMSETTING, GROUPNAME_CUSTOMGLOBALSETTING);
            renameSettingsGroup(settingFile, group, newGroupName);
        }
    }
}

int QKeyMapper::checkAutoStartSaveSettings(const QString &executablename, const QString &windowtitle)
{
    int ret_index = TITLESETTING_INDEX_INVALID;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    QStringList groups = settingFile.childGroups();

    int index = TITLESETTING_INDEX_INVALID;
    for (index = 1; index <= TITLESETTING_INDEX_MAX; index++) {
        QString subgroup = executablename + SEPARATOR_TITLESETTING + QString(WINDOWTITLE_STRING) + QString::number(index);

        if (groups.contains(subgroup)) {
            QVariant windowTitle_Var;
            if (readSaveSettingData(subgroup, PROCESSINFO_WINDOWTITLE, windowTitle_Var)) {
                QString titleStr = windowTitle_Var.toString();
                if (titleStr == windowtitle) {
                    Qt::CheckState autoStartMappingChecked = Qt::Unchecked;
                    QVariant autoStartMappingChecked_Var;
                    if (readSaveSettingData(subgroup, AUTOSTARTMAPPING_CHECKED, autoStartMappingChecked_Var)) {
                        autoStartMappingChecked = (Qt::CheckState)autoStartMappingChecked_Var.toInt();
                        if (Qt::Checked == autoStartMappingChecked) {
                            ret_index = index;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (ret_index == TITLESETTING_INDEX_INVALID) {
        for (index = 1; index <= TITLESETTING_INDEX_MAX; index++) {
            QString subgroup = executablename + SEPARATOR_TITLESETTING + QString(WINDOWTITLE_STRING) + QString::number(index);

            if (groups.contains(subgroup)) {
                QVariant windowTitle_Var;
                if (readSaveSettingData(subgroup, PROCESSINFO_WINDOWTITLE, windowTitle_Var)) {
                    QString titleStr = windowTitle_Var.toString();
                    if (windowtitle.contains(titleStr)) {
                        Qt::CheckState autoStartMappingChecked = Qt::Unchecked;
                        QVariant autoStartMappingChecked_Var;
                        if (readSaveSettingData(subgroup, AUTOSTARTMAPPING_CHECKED, autoStartMappingChecked_Var)) {
                            autoStartMappingChecked = (Qt::CheckState)autoStartMappingChecked_Var.toInt();
                            if (Qt::Checked == autoStartMappingChecked) {
                                ret_index = index;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    if (ret_index == TITLESETTING_INDEX_INVALID) {
        QString subgroup = executablename + SEPARATOR_TITLESETTING + QString(ANYWINDOWTITLE_STRING);

        if (groups.contains(subgroup)) {
            Qt::CheckState autoStartMappingChecked = Qt::Unchecked;
            QVariant autoStartMappingChecked_Var;
            if (readSaveSettingData(subgroup, AUTOSTARTMAPPING_CHECKED, autoStartMappingChecked_Var)) {
                autoStartMappingChecked = (Qt::CheckState)autoStartMappingChecked_Var.toInt();
                if (Qt::Checked == autoStartMappingChecked) {
                    ret_index = TITLESETTING_INDEX_ANYTITLE;
                }
            }
        }
    }

    return ret_index;
}

int QKeyMapper::checkSaveSettings(const QString &executablename, const QString &windowtitle)
{
    int ret_index = TITLESETTING_INDEX_INVALID;
    int contains_index = TITLESETTING_INDEX_INVALID;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    QStringList groups = settingFile.childGroups();

    int index = TITLESETTING_INDEX_INVALID;
    for (index = 1; index <= TITLESETTING_INDEX_MAX; index++) {
        QString subgroup = executablename + SEPARATOR_TITLESETTING + QString(WINDOWTITLE_STRING) + QString::number(index);

        if (groups.contains(subgroup)) {
            QVariant windowTitle_Var;
            if (readSaveSettingData(subgroup, PROCESSINFO_WINDOWTITLE, windowTitle_Var)) {
                QString titleStr = windowTitle_Var.toString();
                if (contains_index == TITLESETTING_INDEX_INVALID
                    && windowtitle.contains(titleStr)) {
                    contains_index = index;
                }
                if (windowtitle == titleStr) {
                    ret_index = index;
                    break;
                }
            }
        }
    }

    if (ret_index == TITLESETTING_INDEX_INVALID
        && contains_index != TITLESETTING_INDEX_INVALID) {
        ret_index = contains_index;
    }

    return ret_index;
}

bool QKeyMapper::readSaveSettingData(const QString &group, const QString &key, QVariant &settingdata)
{
    bool readresult = false;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    QString setting_key = group + "/" + key;
    if (true == settingFile.contains(setting_key)){
        settingdata = settingFile.value(setting_key);
        readresult = true;
    }

    return readresult;
}

void QKeyMapper::saveKeyMapSetting(void)
{
    if (ui->keymapdataTable->rowCount() == KeyMappingDataList.size()){
        QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        settingFile.setIniCodec("UTF-8");
#endif
        QStringList original_keys;
        QStringList mapping_keysList;
        QStringList burstList;
        QStringList lockList;
        int burstpressTime = ui->burstpressSpinBox->value();
        int burstreleaseTime = ui->burstreleaseSpinBox->value();
#ifdef VIGEM_CLIENT_SUPPORT
        int vJoy_X_Sensitivity = ui->vJoyXSensSpinBox->value();
        int vJoy_Y_Sensitivity = ui->vJoyYSensSpinBox->value();
#endif

        QString saveSettingSelectStr;
        QString cursettingSelectStr = ui->settingselectComboBox->currentText();
        int languageIndex = ui->languageComboBox->currentIndex();
        bool saveGlobalSetting = false;

        m_LastWindowPosition = pos();
        settingFile.setValue(LAST_WINDOWPOSITION, m_LastWindowPosition);

        if (LANGUAGE_ENGLISH == languageIndex) {
            settingFile.setValue(LANGUAGE_INDEX , LANGUAGE_ENGLISH);
        }
        else {
            settingFile.setValue(LANGUAGE_INDEX , LANGUAGE_CHINESE);
        }

        settingFile.setValue(VIRTUALGAMEPAD_TYPE , ui->virtualGamepadTypeComboBox->currentText());

        if (m_windowswitchKeySeqEdit->keySequence().isEmpty()) {
            if (m_windowswitchKeySeqEdit->lastKeySequence().isEmpty()) {
                m_windowswitchKeySeqEdit->setKeySequence(QKeySequence(m_windowswitchKeySeqEdit->defaultKeySequence()));
            }
            else {
                m_windowswitchKeySeqEdit->setKeySequence(QKeySequence(m_windowswitchKeySeqEdit->lastKeySequence()));
            }
        }
        m_windowswitchKeySeqEdit->clearFocus();
        if (false == m_windowswitchKeySeqEdit->keySequence().isEmpty()) {
            updateWindowSwitchKeySeq(m_windowswitchKeySeqEdit->keySequence());
            settingFile.setValue(WINDOWSWITCH_KEYSEQ, m_windowswitchKeySeqEdit->keySequence().toString());
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[saveKeyMapSetting]" << "Save & Set Window Switch KeySequence ->" << m_windowswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText);
#endif
        }

        if (cursettingSelectStr == GROUPNAME_GLOBALSETTING && ui->settingselectComboBox->currentIndex() == 1) {
            saveGlobalSetting = true;
            saveSettingSelectStr = cursettingSelectStr;
            settingFile.setValue(SETTINGSELECT , saveSettingSelectStr);
            saveSettingSelectStr = saveSettingSelectStr + "/";
        }
        else if (cursettingSelectStr.startsWith(GROUPNAME_CUSTOMGLOBALSETTING, Qt::CaseInsensitive)
                && cursettingSelectStr.endsWith(GROUPNAME_EXECUTABLE_SUFFIX, Qt::CaseInsensitive) != true) {
            saveSettingSelectStr = cursettingSelectStr;
            settingFile.setValue(SETTINGSELECT , saveSettingSelectStr);
            saveSettingSelectStr = saveSettingSelectStr + "/";
        }
        else {
            QStringList groups = settingFile.childGroups();
            QStringList validgroups_customsetting;
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[saveKeyMapSetting]" << "childGroups >>" << groups;
#endif

            for (const QString &group : qAsConst(groups)){
                if (group.startsWith(GROUPNAME_CUSTOMGLOBALSETTING, Qt::CaseInsensitive)
                        && group.endsWith(GROUPNAME_EXECUTABLE_SUFFIX, Qt::CaseInsensitive) != true) {
                    validgroups_customsetting.append(group);
                }
            }

            int customGlobalSettingIndex;
            if (validgroups_customsetting.size() > 0) {
                QString lastgroup = validgroups_customsetting.last();
                QString lastNumberStr = lastgroup.remove(GROUPNAME_CUSTOMGLOBALSETTING);

                if (lastNumberStr.toInt() >= CUSTOMSETTING_INDEX_MAX) {
                    QString message = "There is already " + lastNumberStr + " CustomGlobalSettings, please remove some CustomGlobalSettings!";
                    QMessageBox::warning(this, PROGRAM_NAME, message);
                    return;
                }
                else {
                    customGlobalSettingIndex = lastNumberStr.toInt() + 1;
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[saveKeyMapSetting] Last custom setting index ->" << lastNumberStr.toInt();
                qDebug() << "[saveKeyMapSetting] Save current custom setting to ->" << customGlobalSettingIndex;
#endif
            }
            else {
                customGlobalSettingIndex = 1;
            }

            if ((false == ui->nameLineEdit->text().isEmpty())
                     && (false == ui->titleLineEdit->text().isEmpty())
                     && (true == ui->nameCheckBox->isChecked())
                     && (true == ui->titleCheckBox->isChecked())
                     && (ui->nameLineEdit->text() == m_MapProcessInfo.FileName)
                     && (m_MapProcessInfo.FilePath.isEmpty() != true)){
                QStringList groups = settingFile.childGroups();
                int index = -1;
                for (index = 1; index <= TITLESETTING_INDEX_MAX; index++) {
                    QString subgroup = m_MapProcessInfo.FileName + SEPARATOR_TITLESETTING + QString(WINDOWTITLE_STRING) + QString::number(index);

                    if (groups.contains(subgroup)) {
                        QVariant windowTitle_Var;
                        if (readSaveSettingData(subgroup, PROCESSINFO_WINDOWTITLE, windowTitle_Var)) {
                            QString titleStr = windowTitle_Var.toString();
                            if (titleStr == ui->titleLineEdit->text()) {
                                break;
                            }
                        }
                    }
                    else {
                        break;
                    }
                }

                if (0 < index && index <= TITLESETTING_INDEX_MAX) {
                    QString subgroup = m_MapProcessInfo.FileName + SEPARATOR_TITLESETTING + QString(WINDOWTITLE_STRING) + QString::number(index);
                    settingFile.setValue(SETTINGSELECT , subgroup);
                    saveSettingSelectStr = subgroup + "/";
                }
                else {
                    QString message = "There is already " + QString::number(TITLESETTING_INDEX_MAX) + " settings of [" + m_MapProcessInfo.FileName + "], please remove some settings of [" + m_MapProcessInfo.FileName + "] first!";
                    QMessageBox::warning(this, PROGRAM_NAME, message);
                    return;
                }
            }
            else if ((false == ui->nameLineEdit->text().isEmpty())
                && (true == ui->nameCheckBox->isChecked())
                && (false == ui->titleCheckBox->isChecked())
                && (ui->nameLineEdit->text() == m_MapProcessInfo.FileName)
                && (m_MapProcessInfo.FilePath.isEmpty() != true)){
                QString subgroup = m_MapProcessInfo.FileName + SEPARATOR_TITLESETTING + QString(ANYWINDOWTITLE_STRING);
                settingFile.setValue(SETTINGSELECT , subgroup);
                saveSettingSelectStr = subgroup + "/";
            }
            else {
                if (customGlobalSettingIndex < 10) {
                    saveSettingSelectStr = QString(GROUPNAME_CUSTOMGLOBALSETTING) + " " + QString::number(customGlobalSettingIndex);
                }
                else {
                    saveSettingSelectStr = QString(GROUPNAME_CUSTOMGLOBALSETTING) + QString::number(customGlobalSettingIndex);
                }
                settingFile.setValue(SETTINGSELECT , saveSettingSelectStr);
                saveSettingSelectStr = saveSettingSelectStr + "/";
            }
        }

        if (KeyMappingDataList.size() > 0){
            for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
            {
                original_keys << keymapdata.Original_Key;
                QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
                mapping_keysList  << mappingkeys_str;
                if (true == keymapdata.Burst) {
                    burstList.append("ON");
                }
                else {
                    burstList.append("OFF");
                }
                if (true == keymapdata.Lock) {
                    lockList.append("ON");
                }
                else {
                    lockList.append("OFF");
                }
            }
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_ORIGINALKEYS, original_keys );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_MAPPINGKEYS , mapping_keysList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURST , burstList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_LOCK , lockList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTPRESS_TIME , burstpressTime  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME , burstreleaseTime  );
#ifdef VIGEM_CLIENT_SUPPORT
            settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_X_SENSITIVITY , vJoy_X_Sensitivity  );
            settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_Y_SENSITIVITY , vJoy_Y_Sensitivity  );
#endif

            settingFile.remove(saveSettingSelectStr+CLEARALL);
        }
        else{
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_ORIGINALKEYS, original_keys );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_MAPPINGKEYS , mapping_keysList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURST , burstList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_LOCK , lockList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTPRESS_TIME , burstpressTime  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME , burstreleaseTime  );
#ifdef VIGEM_CLIENT_SUPPORT
            settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_X_SENSITIVITY , vJoy_X_Sensitivity  );
            settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_Y_SENSITIVITY , vJoy_Y_Sensitivity  );
#endif

            settingFile.setValue(saveSettingSelectStr+CLEARALL, QString("ClearList"));
        }

        if (saveGlobalSetting) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[saveKeyMapSetting]" << "GlobalSetting do not need processinfo!";
#endif

            settingFile.setValue(saveSettingSelectStr+DISABLEWINKEY_CHECKED, false);
        }
        else {
            if ((false == ui->nameLineEdit->text().isEmpty())
                    && (false == ui->titleLineEdit->text().isEmpty())
                    // && (ui->titleLineEdit->text() == m_MapProcessInfo.WindowTitle)
                    && (ui->nameLineEdit->text() == m_MapProcessInfo.FileName)){
                settingFile.setValue(saveSettingSelectStr+PROCESSINFO_FILENAME, m_MapProcessInfo.FileName);
                settingFile.setValue(saveSettingSelectStr+PROCESSINFO_WINDOWTITLE, ui->titleLineEdit->text());

                if (false == m_MapProcessInfo.FilePath.isEmpty()){
                    settingFile.setValue(saveSettingSelectStr+PROCESSINFO_FILEPATH, m_MapProcessInfo.FilePath);
                }
                else{
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[saveKeyMapSetting]" << "FilePath is empty, unsaved.";
#endif
                }

                settingFile.setValue(saveSettingSelectStr+PROCESSINFO_FILENAME_CHECKED, ui->nameCheckBox->isChecked());
                settingFile.setValue(saveSettingSelectStr+PROCESSINFO_WINDOWTITLE_CHECKED, ui->titleCheckBox->isChecked());
            }
            else{
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[saveKeyMapSetting]" << "Unmatch display processinfo & stored processinfo.";
#endif
            }

            settingFile.setValue(saveSettingSelectStr+DISABLEWINKEY_CHECKED, ui->disableWinKeyCheckBox->isChecked());
        }

        settingFile.setValue(saveSettingSelectStr+AUTOSTARTMAPPING_CHECKED, ui->autoStartMappingCheckBox->checkState());
#ifdef VIGEM_CLIENT_SUPPORT
        settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_LOCKCURSOR, ui->lockCursorCheckBox->isChecked());
#endif
        settingFile.setValue(saveSettingSelectStr+DATAPORT_NUMBER, ui->dataPortSpinBox->value());
        double gripThresholdBrake = ui->brakeThresholdDoubleSpinBox->value();
        gripThresholdBrake = round(gripThresholdBrake * pow(10, GRIP_THRESHOLD_DECIMALS)) / pow(10, GRIP_THRESHOLD_DECIMALS);
        settingFile.setValue(saveSettingSelectStr+GRIP_THRESHOLD_BRAKE, gripThresholdBrake);
        double gripThresholdAccel = ui->accelThresholdDoubleSpinBox->value();
        gripThresholdAccel = round(gripThresholdAccel * pow(10, GRIP_THRESHOLD_DECIMALS)) / pow(10, GRIP_THRESHOLD_DECIMALS);
        settingFile.setValue(saveSettingSelectStr+GRIP_THRESHOLD_ACCEL, gripThresholdAccel);

        if (m_mappingswitchKeySeqEdit->keySequence().isEmpty()) {
            if (m_mappingswitchKeySeqEdit->lastKeySequence().isEmpty()) {
                m_mappingswitchKeySeqEdit->setKeySequence(QKeySequence(m_mappingswitchKeySeqEdit->defaultKeySequence()));
            }
            else {
                m_mappingswitchKeySeqEdit->setKeySequence(QKeySequence(m_mappingswitchKeySeqEdit->lastKeySequence()));
            }
        }
        m_mappingswitchKeySeqEdit->clearFocus();
        if (false == m_mappingswitchKeySeqEdit->keySequence().isEmpty()) {
            updateMappingSwitchKeySeq(m_mappingswitchKeySeqEdit->keySequence());
            settingFile.setValue(saveSettingSelectStr+MAPPINGSWITCH_KEYSEQ, m_mappingswitchKeySeqEdit->keySequence().toString());
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[saveKeyMapSetting]" << " Save & Set Mapping Switch KeySequence [" << saveSettingSelectStr+MAPPINGSWITCH_KEYSEQ << "] -> \"" << m_mappingswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText) << "\"";
#endif
        }

        const QString savedSettingName = saveSettingSelectStr.remove("/");
        loadSetting_flag = true;
        bool loadresult = loadKeyMapSetting(savedSettingName);
        Q_UNUSED(loadresult);
        loadSetting_flag = false;
#ifdef DEBUG_LOGOUT_ON
        if (true == loadresult) {
            qDebug() << "[saveKeyMapSetting]" << "Save setting success ->" << savedSettingName;
        }
        else {
            qWarning() << "[saveKeyMapSetting]" << "Save setting failure!!! ->" << savedSettingName;
        }
#endif
    }
    else{
        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
            QMessageBox::warning(this, PROGRAM_NAME, "Invalid KeyMap Data to save!");
        }
        else {
            QMessageBox::warning(this, PROGRAM_NAME, "错误的映射数据，保存失败！");
        }
    }
}

bool QKeyMapper::loadKeyMapSetting(const QString &settingtext)
{
    bool loadDefault = false;
    bool loadGlobalSetting = false;
    bool clearallcontainsflag = true;
    bool selectSettingContainsFlag = false;
    quint8 datavalidflag = 0xFF;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    settingFile.setIniCodec("UTF-8");
#endif

    if (settingtext.isEmpty()) {
        if (true == settingFile.contains(LAST_WINDOWPOSITION)){
            m_LastWindowPosition = settingFile.value(LAST_WINDOWPOSITION, QPoint()).toPoint();
            move(m_LastWindowPosition);
        }

        if (true == settingFile.contains(LANGUAGE_INDEX)){
            int languageIndex = settingFile.value(LANGUAGE_INDEX).toInt();
            if (languageIndex >= 0 && languageIndex < ui->languageComboBox->count()) {
                ui->languageComboBox->setCurrentIndex(languageIndex);
            }
            else {
                ui->languageComboBox->setCurrentIndex(LANGUAGE_CHINESE);
            }
        }
        else {
            ui->languageComboBox->setCurrentIndex(LANGUAGE_CHINESE);
        }

        if (true == settingFile.contains(VIRTUALGAMEPAD_TYPE)){
            QString virtualGamepadType = settingFile.value(VIRTUALGAMEPAD_TYPE).toString();
            if (virtualGamepadType == VIRTUAL_GAMEPAD_X360 || virtualGamepadType == VIRTUAL_GAMEPAD_DS4) {
                ui->virtualGamepadTypeComboBox->setCurrentText(virtualGamepadType);
            }
            else {
                ui->virtualGamepadTypeComboBox->setCurrentText(VIRTUAL_GAMEPAD_X360);
            }
        }
        else {
            ui->virtualGamepadTypeComboBox->setCurrentText(VIRTUAL_GAMEPAD_X360);
        }

        QString loadedwindowswitchKeySeqStr;
        if (true == settingFile.contains(WINDOWSWITCH_KEYSEQ)){
            loadedwindowswitchKeySeqStr = settingFile.value(WINDOWSWITCH_KEYSEQ).toString();
            if (loadedwindowswitchKeySeqStr.isEmpty()) {
                loadedwindowswitchKeySeqStr = m_windowswitchKeySeqEdit->defaultKeySequence();
            }
        }
        else {
            loadedwindowswitchKeySeqStr = m_windowswitchKeySeqEdit->defaultKeySequence();
        }
        m_windowswitchKeySeqEdit->setKeySequence(QKeySequence(loadedwindowswitchKeySeqStr));
        updateWindowSwitchKeySeq(m_windowswitchKeySeqEdit->keySequence());
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Load & Set Window Switch KeySequence ->" << m_windowswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText);
#endif

        if (true == settingFile.contains(PLAY_SOUNDEFFECT)){
            bool soundeffectChecked = settingFile.value(PLAY_SOUNDEFFECT).toBool();
            if (true == soundeffectChecked) {
                ui->soundEffectCheckBox->setChecked(true);
            }
            else {
                ui->soundEffectCheckBox->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Sound Effect Checkbox ->" << soundeffectChecked;
#endif
        }
        else {
            ui->soundEffectCheckBox->setChecked(true);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Do not contains PlaySoundEffect, PlaySoundEffect set to Checked.";
#endif
        }

        if (true == settingFile.contains(AUTO_STARTUP)){
            bool autostartupChecked = settingFile.value(AUTO_STARTUP).toBool();
            if (true == autostartupChecked) {
                ui->autoStartupCheckBox->setChecked(true);
            }
            else {
                ui->autoStartupCheckBox->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Auto Startup Checkbox ->" << autostartupChecked;
#endif
        }
        else {
            ui->autoStartupCheckBox->setChecked(false);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Do not contains AutoStartup, AutoStartup set to Unchecked.";
#endif
        }

#ifdef VIGEM_CLIENT_SUPPORT
        if (true == settingFile.contains(VIRTUALGAMEPAD_ENABLE)){
            bool virtualjoystickenableChecked = settingFile.value(VIRTUALGAMEPAD_ENABLE).toBool();
            if (true == virtualjoystickenableChecked) {
                ui->enableVirtualJoystickCheckBox->setChecked(true);
            }
            else {
                ui->enableVirtualJoystickCheckBox->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Virtual Joystick Enable Checkbox ->" << virtualjoystickenableChecked;
#endif
        }
        else {
            ui->enableVirtualJoystickCheckBox->setChecked(false);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Do not contains VirtualGamepadEnable, VirtualGamepadEnable set to Unchecked.";
#endif
        }
#endif
    }
    else if (GROUPNAME_GLOBALSETTING == settingtext) {
        loadGlobalSetting = true;
    }

    QString settingSelectStr;

    ui->settingselectComboBox->clear();
    ui->settingselectComboBox->addItem(QString());
    ui->settingselectComboBox->addItem(GROUPNAME_GLOBALSETTING);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->settingselectComboBox->model());
    QStandardItem* item = model->item(1);
    item->setData(QColor(Qt::darkMagenta), Qt::ForegroundRole);
#else
    QBrush colorBrush(Qt::darkMagenta);
    ui->settingselectComboBox->setItemData(1, colorBrush, Qt::TextColorRole);
#endif
    QStringList groups = settingFile.childGroups();
    QStringList validgroups;
    QStringList validgroups_fullmatch;
    QStringList validgroups_customsetting;
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[loadKeyMapSetting]" << "childGroups >>" << groups;
#endif
    for (const QString &group : qAsConst(groups)){
        bool valid_setting = false;
        if (group.endsWith(GROUPNAME_EXECUTABLE_SUFFIX+QString(SEPARATOR_TITLESETTING)+ANYWINDOWTITLE_STRING, Qt::CaseInsensitive) || group.contains(GROUPNAME_EXECUTABLE_SUFFIX+QString(SEPARATOR_TITLESETTING)+WINDOWTITLE_STRING)) {
            QString tempSettingSelectStr = group + "/";
            if ((true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_FILENAME))
                    && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_WINDOWTITLE))
                    && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_FILEPATH))
                    && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_FILENAME_CHECKED))
                    && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_WINDOWTITLE_CHECKED))){
                valid_setting = true;
            }
        }

        if (true == valid_setting) {
            ui->settingselectComboBox->addItem(group);
            validgroups_fullmatch.append(group);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting] Setting select add FullMatch ->" << group;
#endif
        }
    }

    for (const QString &group : qAsConst(groups)){
        bool valid_setting = false;
        if (group.startsWith(GROUPNAME_CUSTOMGLOBALSETTING, Qt::CaseInsensitive)
                && group.endsWith(GROUPNAME_EXECUTABLE_SUFFIX, Qt::CaseInsensitive) != true) {
            valid_setting = true;
        }

        if (true == valid_setting) {
            ui->settingselectComboBox->addItem(group);
            validgroups_customsetting.append(group);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting] Setting select add CustomGlobalSetting ->" << group;
#endif
        }
    }

    if (groups.contains(GROUPNAME_GLOBALSETTING)) {
        QString settingSelectStr_bak = settingSelectStr;

        settingSelectStr = QString(GROUPNAME_GLOBALSETTING) + "/";
        QStringList original_keys;
        QStringList mapping_keys;
        QStringList burstStringList;
        QStringList lockStringList;
        QList<bool> burstList;
        QList<bool> lockList;
        QList<MAP_KEYDATA> loadkeymapdata;
        bool global_datavalid = false;

        if ((true == settingFile.contains(settingSelectStr+KEYMAPDATA_ORIGINALKEYS))
            && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYS))
            && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURST))
            && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_LOCK))
            && (true == settingFile.contains(settingSelectStr+AUTOSTARTMAPPING_CHECKED))){
            original_keys   = settingFile.value(settingSelectStr+KEYMAPDATA_ORIGINALKEYS).toStringList();
            mapping_keys    = settingFile.value(settingSelectStr+KEYMAPDATA_MAPPINGKEYS).toStringList();
            burstStringList = settingFile.value(settingSelectStr+KEYMAPDATA_BURST).toStringList();
            lockStringList  = settingFile.value(settingSelectStr+KEYMAPDATA_LOCK).toStringList();

            if ((original_keys.size() == mapping_keys.size())
                && (original_keys.size() == burstStringList.size())){
                global_datavalid = true;

                if (original_keys.size() > 0){
                    for (const QString &burst : qAsConst(burstStringList)){
                        if (burst == "ON") {
                            burstList.append(true);
                        }
                        else {
                            burstList.append(false);
                        }
                    }

                    for (const QString &lock : qAsConst(lockStringList)){
                        if (lock == "ON") {
                            lockList.append(true);
                        }
                        else {
                            lockList.append(false);
                        }
                    }

                    int loadindex = 0;
                    for (const QString &ori_key : qAsConst(original_keys)){
                        bool keyboardmapcontains = QKeyMapper_Worker::VirtualKeyCodeMap.contains(ori_key);
                        bool mousemapcontains = QKeyMapper_Worker::VirtualMouseButtonMap.contains(ori_key);
                        bool joystickmapcontains = QKeyMapper_Worker::JoyStickKeyMap.contains(ori_key);
                        QString appendOriKey = ori_key;
                        if (ori_key.startsWith(PREFIX_SHORTCUT)) {
                            keyboardmapcontains = true;
                        }
#ifdef MOUSEBUTTON_CONVERT
                        if (QKeyMapper_Worker::MouseButtonNameConvertMap.contains(ori_key)) {
                            appendOriKey = QKeyMapper_Worker::MouseButtonNameConvertMap.value(ori_key);
                            mousemapcontains = true;
                        }
#endif
                        bool checkmappingstr = checkMappingkeyStr(mapping_keys[loadindex]);

                        if ((true == keyboardmapcontains || true == mousemapcontains || true == joystickmapcontains)
                            && (true == checkmappingstr)){
                            loadkeymapdata.append(MAP_KEYDATA(appendOriKey, mapping_keys.at(loadindex), burstList.at(loadindex), lockList.at(loadindex)));
                        }
                        else{
                            global_datavalid = false;
#ifdef DEBUG_LOGOUT_ON
                            qWarning("[loadKeyMapSetting] GlobalSetting invalid data loaded -> keyboardmapcontains(%s), mousemapcontains(%s), joystickmapcontains(%s), checkmappingstr(%s)", keyboardmapcontains?"true":"false", mousemapcontains?"true":"false", joystickmapcontains?"true":"false", checkmappingstr?"true":"false");
#endif
                            break;
                        }

                        loadindex += 1;
                    }
                }
            }
        }

        Qt::CheckState autoStartMappingCheckState_global = Qt::Unchecked;
        if (true == settingFile.contains(settingSelectStr+AUTOSTARTMAPPING_CHECKED)){
            autoStartMappingCheckState_global = (Qt::CheckState)settingFile.value(settingSelectStr+AUTOSTARTMAPPING_CHECKED).toInt();
        }
        else {
            autoStartMappingCheckState_global = Qt::Unchecked;
        }
        s_GlobalSettingAutoStart = autoStartMappingCheckState_global;
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "GlobalSettingAutoStartMapping =" << s_GlobalSettingAutoStart;
#endif

        if (global_datavalid && false == loadkeymapdata.isEmpty()) {
            KeyMappingDataListGlobal = loadkeymapdata;
            validgroups.append(GROUPNAME_GLOBALSETTING);
        }
        else {
            KeyMappingDataListGlobal.clear();
        }

        settingSelectStr = settingSelectStr_bak;
    }

    validgroups = validgroups + validgroups_fullmatch + validgroups_customsetting;

    if (true == settingtext.isEmpty()) {
        if (true == settingFile.contains(SETTINGSELECT)){
            QVariant settingSelect = settingFile.value(SETTINGSELECT);
            if (settingSelect.canConvert(QMetaType::QString)) {
                settingSelectStr = settingSelect.toString();
            }

            if (false == validgroups.contains(settingSelectStr)) {
                if (settingSelectStr == GROUPNAME_GLOBALSETTING) {
#ifdef DEBUG_LOGOUT_ON
                    qWarning() << "[loadKeyMapSetting] No valid Global Setting, Create a empty one! ->" << settingSelectStr;
#endif
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qWarning() << "[loadKeyMapSetting] Invalid setting select name ->" << settingSelectStr;
#endif
                    settingSelectStr.clear();
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[loadKeyMapSetting] Setting select name ->" << settingSelectStr;
#endif
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Startup load setting" << settingSelectStr;
#endif
        }
        else {
            settingSelectStr.clear();
            clearallcontainsflag = false;
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Startup loading do not contain setting select!";
#endif
        }
    }
    else {
        /* Select setting from combobox */
        if (true == settingFile.contains(SETTINGSELECT)){
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "SettingSelect combox select Setting" << settingtext;
#endif
            QVariant settingSelect = settingFile.value(SETTINGSELECT);
            if (settingSelect.canConvert(QMetaType::QString)) {
                settingSelectStr = settingSelect.toString();
            }

            if (settingSelectStr != settingtext) {
                settingFile.setValue(SETTINGSELECT , settingtext);
                settingSelectStr = settingtext;
            }

            if (false == validgroups.contains(settingSelectStr)) {
                if (settingSelectStr == GROUPNAME_GLOBALSETTING) {
#ifdef DEBUG_LOGOUT_ON
                    qWarning() << "[loadKeyMapSetting] No valid Global Setting, Create a empty one! ->" << settingSelectStr;
#endif
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qWarning() << "[loadKeyMapSetting] Invalid setting select name ->" << settingSelectStr;
#endif
                    settingSelectStr.clear();
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[loadKeyMapSetting] Setting select name ->" << settingSelectStr;
#endif
            }

            if (false == settingSelectStr.contains("/")) {
                settingSelectStr = settingSelectStr + "/";
            }

            if ((true == settingFile.contains(settingSelectStr+KEYMAPDATA_ORIGINALKEYS))
                    && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYS))
                    && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURST))
                    && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_LOCK))){
                selectSettingContainsFlag = true;
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[loadKeyMapSetting]" << "SettingSelect combox select loading contains Setting" << settingSelectStr;
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "SettingSelect combox select loading do not contain Setting" << settingSelectStr;
#endif
            }
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "SettingSelect combox select loading do not contain SettingSelect";
#endif
        }
    }

    if (settingSelectStr.isEmpty() != true) {
        if (false == settingSelectStr.contains("/")) {
            settingSelectStr = settingSelectStr + "/";
        }

        if (true == settingtext.isEmpty() || true == selectSettingContainsFlag) {
            if (false == settingFile.contains(settingSelectStr+CLEARALL)){
                clearallcontainsflag = false;
            }
        }

        if (settingSelectStr == QString(GROUPNAME_GLOBALSETTING) + "/") {
            loadGlobalSetting = true;
        }
    }
    else {
        if (loadGlobalSetting) {
            settingSelectStr = QString(GROUPNAME_GLOBALSETTING) + "/";
        }
        else {
            settingSelectStr.clear();
        }
    }

    if (false == clearallcontainsflag){
        QStringList original_keys;
        QStringList mapping_keys;
        QStringList burstStringList;
        QStringList lockStringList;
        QList<bool> burstList;
        QList<bool> lockList;
        QList<MAP_KEYDATA> loadkeymapdata;

        if ((true == settingFile.contains(settingSelectStr+KEYMAPDATA_ORIGINALKEYS))
                && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYS))
                && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURST))
                && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_LOCK))){
            original_keys   = settingFile.value(settingSelectStr+KEYMAPDATA_ORIGINALKEYS).toStringList();
            mapping_keys    = settingFile.value(settingSelectStr+KEYMAPDATA_MAPPINGKEYS).toStringList();
            burstStringList = settingFile.value(settingSelectStr+KEYMAPDATA_BURST).toStringList();
            lockStringList  = settingFile.value(settingSelectStr+KEYMAPDATA_LOCK).toStringList();

            if ((original_keys.size() == mapping_keys.size())
                    && (original_keys.size() == burstStringList.size())){
                datavalidflag = true;

                if (original_keys.size() > 0){
                    for (const QString &burst : qAsConst(burstStringList)){
                        if (burst == "ON") {
                            burstList.append(true);
                        }
                        else {
                            burstList.append(false);
                        }
                    }

                    for (const QString &lock : qAsConst(lockStringList)){
                        if (lock == "ON") {
                            lockList.append(true);
                        }
                        else {
                            lockList.append(false);
                        }
                    }

                    int loadindex = 0;
                    for (const QString &ori_key : qAsConst(original_keys)){
                        bool keyboardmapcontains = QKeyMapper_Worker::VirtualKeyCodeMap.contains(ori_key);
                        bool mousemapcontains = QKeyMapper_Worker::VirtualMouseButtonMap.contains(ori_key);
                        bool joystickmapcontains = QKeyMapper_Worker::JoyStickKeyMap.contains(ori_key);
                        QString appendOriKey = ori_key;
                        if (ori_key.startsWith(PREFIX_SHORTCUT)) {
                            keyboardmapcontains = true;
                        }
#ifdef MOUSEBUTTON_CONVERT
                        if (QKeyMapper_Worker::MouseButtonNameConvertMap.contains(ori_key)) {
                            appendOriKey = QKeyMapper_Worker::MouseButtonNameConvertMap.value(ori_key);
                            mousemapcontains = true;
                        }
#endif
                        bool checkmappingstr = checkMappingkeyStr(mapping_keys[loadindex]);

                        if ((true == keyboardmapcontains || true == mousemapcontains || true == joystickmapcontains)
                                && (true == checkmappingstr)){
                            loadkeymapdata.append(MAP_KEYDATA(appendOriKey, mapping_keys.at(loadindex), burstList.at(loadindex), lockList.at(loadindex)));
                        }
                        else{
                            datavalidflag = false;
#ifdef DEBUG_LOGOUT_ON
                            qWarning("[loadKeyMapSetting] Invalid data loaded -> keyboardmapcontains(%s), mousemapcontains(%s), joystickmapcontains(%s), checkmappingstr(%s)", keyboardmapcontains?"true":"false", mousemapcontains?"true":"false", joystickmapcontains?"true":"false", checkmappingstr?"true":"false");
#endif
                            break;
                        }

                        loadindex += 1;
                    }
                }
            }
        }

        if (datavalidflag != (quint8)true){
            if (loadGlobalSetting && (0xFF == datavalidflag)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[loadKeyMapSetting]" << "Empty Global Setting do not load default!";
#endif
            }
            else {
                KeyMappingDataList.clear();
                KeyMappingDataList.append(MAP_KEYDATA("I",          "L-Shift + ]}",     false,  false));
                KeyMappingDataList.append(MAP_KEYDATA("K",          "L-Shift + [{",     false,  false));
                KeyMappingDataList.append(MAP_KEYDATA("H",          "S",                false,  false));
                KeyMappingDataList.append(MAP_KEYDATA("Space",      "S",                false,  false));
                KeyMappingDataList.append(MAP_KEYDATA("F",          "Enter",            false,  false));
                loadDefault = true;
            }
        }
        else{
            KeyMappingDataList = loadkeymapdata;
        }
    }
    else{
        KeyMappingDataList.clear();
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[loadKeyMapSetting]" << "refreshKeyMappingDataTable()";
#endif
    refreshKeyMappingDataTable();

    if (loadGlobalSetting) {
        ui->nameLineEdit->setText(QString());
        ui->titleLineEdit->setText(QString());
        ui->nameCheckBox->setChecked(false);
        ui->titleCheckBox->setChecked(false);
        ui->disableWinKeyCheckBox->setChecked(false);

        ui->nameLineEdit->setEnabled(false);
        ui->titleLineEdit->setEnabled(false);
        ui->nameCheckBox->setEnabled(false);
        ui->titleCheckBox->setEnabled(false);
        ui->removeSettingButton->setEnabled(false);
        ui->disableWinKeyCheckBox->setEnabled(false);

        ui->iconLabel->clear();
        m_MapProcessInfo = MAP_PROCESSINFO();
    }
    else {
        ui->nameLineEdit->setEnabled(true);
        ui->titleLineEdit->setEnabled(true);
        ui->nameCheckBox->setEnabled(true);
        ui->titleCheckBox->setEnabled(true);
        ui->removeSettingButton->setEnabled(true);
        ui->disableWinKeyCheckBox->setEnabled(true);

        if ((true == settingFile.contains(settingSelectStr+PROCESSINFO_FILENAME))
                && (true == settingFile.contains(settingSelectStr+PROCESSINFO_WINDOWTITLE))){
            m_MapProcessInfo.FileName = settingFile.value(settingSelectStr+PROCESSINFO_FILENAME).toString();
            m_MapProcessInfo.WindowTitle = settingFile.value(settingSelectStr+PROCESSINFO_WINDOWTITLE).toString();

            ui->nameLineEdit->setText(m_MapProcessInfo.FileName);
            ui->titleLineEdit->setText(m_MapProcessInfo.WindowTitle);
        }
        else {
            ui->nameLineEdit->setText(QString());
            ui->titleLineEdit->setText(QString());
        }

        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_FILEPATH)){
            m_MapProcessInfo.FilePath = settingFile.value(settingSelectStr+PROCESSINFO_FILEPATH).toString();
        }
        else {
            m_MapProcessInfo = MAP_PROCESSINFO();
        }
        ui->iconLabel->clear();

        if (true == loadDefault) {
            setMapProcessInfo(QString(DEFAULT_NAME), QString(DEFAULT_TITLE), QString(), QString(), QIcon(":/DefaultIcon.ico"));
        }

        updateProcessInfoDisplay();

        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_FILENAME_CHECKED)){
            bool fileNameChecked = settingFile.value(settingSelectStr+PROCESSINFO_FILENAME_CHECKED).toBool();
            if (true == fileNameChecked) {
                ui->nameCheckBox->setChecked(true);
            }
            else {
                ui->nameCheckBox->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "FileNameChecked =" << fileNameChecked;
#endif
        }
        else {
            ui->nameCheckBox->setChecked(false);
        }

        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_WINDOWTITLE_CHECKED)){
            bool windowTitleChecked = settingFile.value(settingSelectStr+PROCESSINFO_WINDOWTITLE_CHECKED).toBool();
            if (true == windowTitleChecked) {
                ui->titleCheckBox->setChecked(true);
            }
            else {
                ui->titleCheckBox->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "WindowTitleChecked =" << windowTitleChecked;
#endif
        }
        else {
            ui->titleCheckBox->setChecked(false);
        }

        if (true == loadDefault) {
            ui->nameCheckBox->setChecked(true);
            ui->titleCheckBox->setChecked(true);
        }

        if (true == settingFile.contains(settingSelectStr+DISABLEWINKEY_CHECKED)){
            bool disableWinKeyChecked = settingFile.value(settingSelectStr+DISABLEWINKEY_CHECKED).toBool();
            if (true == disableWinKeyChecked) {
                ui->disableWinKeyCheckBox->setChecked(true);
            }
            else {
                ui->disableWinKeyCheckBox->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "DisableWinKeyChecked =" << disableWinKeyChecked;
#endif
        }
        else {
            ui->disableWinKeyCheckBox->setChecked(false);
        }
    }

    if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURSTPRESS_TIME)){
        int burstpressTime = settingFile.value(settingSelectStr+KEYMAPDATA_BURSTPRESS_TIME).toInt();
        if (BURST_TIME_MIN <= burstpressTime && burstpressTime <= BURST_TIME_MAX) {
            ui->burstpressSpinBox->setValue(burstpressTime);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "BurstPressTime =" << burstpressTime;
#endif
    }
    else {
        ui->burstpressSpinBox->setValue(BURST_PRESS_TIME_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME)){
        int burstreleaseTime = settingFile.value(settingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME).toInt();
        if (BURST_TIME_MIN <= burstreleaseTime && burstreleaseTime <= BURST_TIME_MAX) {
            ui->burstreleaseSpinBox->setValue(burstreleaseTime);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "BurstReleaseTime =" << burstreleaseTime;
#endif
    }
    else {
        ui->burstreleaseSpinBox->setValue(BURST_RELEASE_TIME_DEFAULT);
    }

#ifdef VIGEM_CLIENT_SUPPORT
    if (true == settingFile.contains(settingSelectStr+MOUSE2VJOY_X_SENSITIVITY)){
        int vJoy_X_Sensitivity = settingFile.value(settingSelectStr+MOUSE2VJOY_X_SENSITIVITY).toInt();
        ui->vJoyXSensSpinBox->setValue(vJoy_X_Sensitivity);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "vJoy X Sensitivity =" << vJoy_X_Sensitivity;
#endif
    }
    else {
        ui->vJoyXSensSpinBox->setValue(VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+MOUSE2VJOY_Y_SENSITIVITY)){
        int vJoy_Y_Sensitivity = settingFile.value(settingSelectStr+MOUSE2VJOY_Y_SENSITIVITY).toInt();
        ui->vJoyYSensSpinBox->setValue(vJoy_Y_Sensitivity);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "vJoy Y Sensitivity =" << vJoy_Y_Sensitivity;
#endif
    }
    else {
        ui->vJoyYSensSpinBox->setValue(VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+MOUSE2VJOY_LOCKCURSOR)){
        bool lockCursorChecked = settingFile.value(settingSelectStr+MOUSE2VJOY_LOCKCURSOR).toBool();
        if (true == lockCursorChecked) {
            ui->lockCursorCheckBox->setChecked(true);
        }
        else {
            ui->lockCursorCheckBox->setChecked(false);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "LockCursorChecked =" << lockCursorChecked;
#endif
    }
    else {
        ui->lockCursorCheckBox->setChecked(false);
    }
#endif

    if (true == settingFile.contains(settingSelectStr+DATAPORT_NUMBER)){
        int dataPortNumber = settingFile.value(settingSelectStr+DATAPORT_NUMBER).toInt();
        ui->dataPortSpinBox->setValue(dataPortNumber);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "DataPortNumber =" << dataPortNumber;
#endif
    }
    else {
        ui->dataPortSpinBox->setValue(DATA_PORT_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GRIP_THRESHOLD_BRAKE)){
        double gripThresholdBrake = settingFile.value(settingSelectStr+GRIP_THRESHOLD_BRAKE).toDouble();
        ui->brakeThresholdDoubleSpinBox->setValue(gripThresholdBrake);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "GripThresholdBrake =" << gripThresholdBrake;
#endif
    }
    else {
        ui->accelThresholdDoubleSpinBox->setValue(GRIP_THRESHOLD_BRAKE_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GRIP_THRESHOLD_ACCEL)){
        double gripThresholdAccel = settingFile.value(settingSelectStr+GRIP_THRESHOLD_ACCEL).toDouble();
        ui->accelThresholdDoubleSpinBox->setValue(gripThresholdAccel);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "GripThresholdAccel =" << gripThresholdAccel;
#endif
    }
    else {
        ui->accelThresholdDoubleSpinBox->setValue(GRIP_THRESHOLD_ACCEL_DEFAULT);
    }

    Qt::CheckState autoStartMappingCheckState = Qt::Unchecked;
    if (true == settingFile.contains(settingSelectStr+AUTOSTARTMAPPING_CHECKED)){
        autoStartMappingCheckState = (Qt::CheckState)settingFile.value(settingSelectStr+AUTOSTARTMAPPING_CHECKED).toInt();
    }
    else {
        autoStartMappingCheckState = Qt::Unchecked;
    }
    ui->autoStartMappingCheckBox->setCheckState(autoStartMappingCheckState);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[loadKeyMapSetting]" << "AutoStartMappingCheckState =" << autoStartMappingCheckState;
#endif

    QString loadedmappingswitchKeySeqStr;
    if (true == settingFile.contains(settingSelectStr+MAPPINGSWITCH_KEYSEQ)){
        loadedmappingswitchKeySeqStr = settingFile.value(settingSelectStr+MAPPINGSWITCH_KEYSEQ).toString();
        if (loadedmappingswitchKeySeqStr.isEmpty()) {
            loadedmappingswitchKeySeqStr = m_mappingswitchKeySeqEdit->defaultKeySequence();
        }
    }
    else {
        loadedmappingswitchKeySeqStr = m_mappingswitchKeySeqEdit->defaultKeySequence();
    }
    m_mappingswitchKeySeqEdit->setKeySequence(QKeySequence(loadedmappingswitchKeySeqStr));
    updateMappingSwitchKeySeq(m_mappingswitchKeySeqEdit->keySequence());
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[loadKeyMapSetting]" << " Load & Set Mapping Switch KeySequence [" << settingSelectStr+MAPPINGSWITCH_KEYSEQ << "] -> \"" << m_mappingswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText) << "\"";
#endif

    if (false == datavalidflag){
        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
            QMessageBox::warning(this, PROGRAM_NAME, "<html><head/><body><p align=\"center\">Load invalid keymapdata from INI file.</p><p align=\"center\">Reset to default values.</p></body></html>");
        }
        else {
            QMessageBox::warning(this, PROGRAM_NAME, "<html><head/><body><p align=\"center\">从INI文件加载了无效的设定数据。</p><p align=\"center\">显示默认设定。</p></body></html>");
        }
        return false;
    }
    else {
        ui->nameLineEdit->setCursorPosition(0);
        ui->titleLineEdit->setCursorPosition(0);

        if (settingSelectStr.isEmpty() != true) {
            settingSelectStr = settingSelectStr.remove("/");
            ui->settingselectComboBox->setCurrentText(settingSelectStr);
        }

        if ((Qt::Checked == autoStartMappingCheckState) && (true == settingtext.isEmpty())) {
            MappingStart(MAPPINGSTART_LOADSETTING);
        }
        return true;
    }
}

bool QKeyMapper::checkMappingkeyStr(QString &mappingkeystr)
{
#ifdef MOUSEBUTTON_CONVERT
    QStringList mouseNameConvertList = QKeyMapper_Worker::MouseButtonNameConvertMap.keys();
    for (const QString &mousekey : qAsConst(mouseNameConvertList)){
        mappingkeystr.replace(mousekey, QKeyMapper_Worker::MouseButtonNameConvertMap.value(mousekey));
    }
#endif

    bool checkResult = true;
    static QRegularExpression regexp("\\s[+»]\\s");
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QStringList Mapping_Keys = mappingkeystr.split(regexp, Qt::SkipEmptyParts);
#else
    QStringList Mapping_Keys = mappingkeystr.split(regexp, QString::SkipEmptyParts);
#endif
    for (const QString &mapping_key : qAsConst(Mapping_Keys)){
        if (false == QKeyMapper_Worker::VirtualKeyCodeMap.contains(mapping_key)
            && false == QKeyMapper_Worker::VirtualMouseButtonMap.contains(mapping_key)
            && false == QKeyMapper_Worker::JoyStickKeyMap.contains(mapping_key)
            && false == mapping_key.contains(SEPARATOR_WAITTIME)){
            checkResult = false;
            break;
        }
    }

    return checkResult;
}

void QKeyMapper::loadFontFile(const QString fontfilename, int &returnback_fontid, QString &fontname)
{
    returnback_fontid = -1;
    fontname = QString();
    QFile fontFile(fontfilename);
    if(!fontFile.open(QIODevice::ReadOnly))
    {
#ifdef DEBUG_LOGOUT_ON
        qDebug()<<"Open font file failure!!!";
#endif
        return;
    }

    returnback_fontid = QFontDatabase::addApplicationFont(fontfilename);
    QStringList loadedFontFamilies = QFontDatabase::applicationFontFamilies(returnback_fontid);
    if(false == loadedFontFamilies.isEmpty())
    {
        fontname = loadedFontFamilies.at(0);

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[loadFontFile]" << " Load font from \"" << fontfilename <<"\", FontName: " << loadedFontFamilies;
#endif
    }
    fontFile.close();
}

#ifdef USE_SAOFONT
void QKeyMapper::setControlCustomFont(const QString &fontname)
{
    QFont customFont(fontname);

    customFont.setPointSize(18);
    // ui->refreshButton->setFont(customFont);
    ui->keymapButton->setFont(customFont);
    ui->savemaplistButton->setFont(customFont);

    customFont.setPointSize(11);
    ui->deleteoneButton->setFont(customFont);
    ui->clearallButton->setFont(customFont);
    ui->addmapdataButton->setFont(customFont);
    ui->nameCheckBox->setFont(customFont);
    ui->titleCheckBox->setFont(customFont);
    ui->orikeyLabel->setFont(customFont);
    ui->mapkeyLabel->setFont(customFont);
    ui->burstpressLabel->setFont(customFont);
    ui->burstpress_msLabel->setFont(customFont);
    ui->burstreleaseLabel->setFont(customFont);
    ui->burstrelease_msLabel->setFont(customFont);
    ui->settingselectLabel->setFont(customFont);
    ui->removeSettingButton->setFont(customFont);
    ui->nextarrowCheckBox->setFont(customFont);

    ui->processinfoTable->horizontalHeader()->setFont(customFont);
    ui->keymapdataTable->horizontalHeader()->setFont(customFont);

    customFont.setPointSize(12);
    ui->disableWinKeyCheckBox->setFont(customFont);
    ui->autoStartMappingCheckBox->setFont(customFont);
    ui->autoStartupCheckBox->setFont(customFont);
}
#endif

void QKeyMapper::setControlFontEnglish()
{
    QFont customFont(FONTNAME_ENGLISH);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(18);
    }
    else {
        customFont.setPointSize(14);
    }
    // ui->refreshButton->setFont(customFont);
    ui->keymapButton->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(14);
    }
    else {
        customFont.setPointSize(12);
    }
    ui->savemaplistButton->setFont(customFont);
    ui->addmapdataButton->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(11);
    }
    else {
        customFont.setPointSize(9);
    }
    ui->deleteoneButton->setFont(customFont);
    ui->clearallButton->setFont(customFont);
    ui->nameCheckBox->setFont(customFont);
    ui->titleCheckBox->setFont(customFont);
    ui->orikeyLabel->setFont(customFont);
    ui->orikeySeqLabel->setFont(customFont);
    ui->mapkeyLabel->setFont(customFont);
    ui->burstpressLabel->setFont(customFont);
    // ui->burstpress_msLabel->setFont(customFont);
    ui->burstreleaseLabel->setFont(customFont);
    // ui->burstrelease_msLabel->setFont(customFont);
    // ui->settingselectLabel->setFont(customFont);
    ui->removeSettingButton->setFont(customFont);
    ui->nextarrowCheckBox->setFont(customFont);
    ui->waitTimeLabel->setFont(customFont);
    // ui->waitTime_msLabel->setFont(customFont);
    ui->mouseXSpeedLabel->setFont(customFont);
    ui->mouseYSpeedLabel->setFont(customFont);

    ui->processinfoTable->horizontalHeader()->setFont(customFont);
    ui->keymapdataTable->horizontalHeader()->setFont(customFont);

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(10);
    // }
    // else {
    //     customFont.setPointSize(8);
    // }
    ui->windowswitchkeyLabel->setFont(customFont);
    ui->mappingswitchkeyLabel->setFont(customFont);
    ui->installViGEmBusButton->setFont(customFont);
    // ui->uninstallViGEmBusButton->setFont(customFont);
    ui->enableVirtualJoystickCheckBox->setFont(customFont);
    ui->lockCursorCheckBox->setFont(customFont);
    ui->ViGEmBusStatusLabel->setFont(customFont);
    ui->vJoyXSensLabel->setFont(customFont);
    ui->vJoyYSensLabel->setFont(customFont);
    ui->virtualgamepadGroupBox->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(12);
    }
    else {
        customFont.setPointSize(8);
    }
    ui->autoStartMappingCheckBox->setFont(customFont);
    ui->autoStartupCheckBox->setFont(customFont);
    ui->soundEffectCheckBox->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(10);
    }
    else {
        customFont.setPointSize(8);
    }
    ui->disableWinKeyCheckBox->setFont(customFont);
    ui->dataPortLabel->setFont(customFont);
    ui->brakeThresholdLabel->setFont(customFont);
    ui->accelThresholdLabel->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        QRect curGeometry = ui->virtualGamepadTypeComboBox->geometry();
        curGeometry.moveLeft(curGeometry.x() - 10);
        ui->virtualGamepadTypeComboBox->setGeometry(curGeometry);
    }
}

void QKeyMapper::setControlFontChinese()
{
    QFont customFont(FONTNAME_CHINESE, 18, QFont::Bold);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(18);
    }
    else {
        customFont.setPointSize(14);
    }
    // ui->refreshButton->setFont(customFont);
    ui->keymapButton->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(14);
    }
    else {
        customFont.setPointSize(12);
    }
    ui->savemaplistButton->setFont(customFont);
    ui->addmapdataButton->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(11);
    }
    else {
        customFont.setPointSize(9);
    }
    ui->deleteoneButton->setFont(customFont);
    ui->clearallButton->setFont(customFont);
    ui->nameCheckBox->setFont(customFont);
    ui->titleCheckBox->setFont(customFont);
    ui->orikeyLabel->setFont(customFont);
    ui->orikeySeqLabel->setFont(customFont);
    ui->mapkeyLabel->setFont(customFont);
    ui->burstpressLabel->setFont(customFont);
    // ui->burstpress_msLabel->setFont(customFont);
    ui->burstreleaseLabel->setFont(customFont);
    // ui->burstrelease_msLabel->setFont(customFont);
    // ui->settingselectLabel->setFont(customFont);
    ui->removeSettingButton->setFont(customFont);
    ui->nextarrowCheckBox->setFont(customFont);
    ui->waitTimeLabel->setFont(customFont);
    // ui->waitTime_msLabel->setFont(customFont);
    ui->mouseXSpeedLabel->setFont(customFont);
    ui->mouseYSpeedLabel->setFont(customFont);

    ui->processinfoTable->horizontalHeader()->setFont(customFont);
    ui->keymapdataTable->horizontalHeader()->setFont(customFont);

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(11);
    // }
    // else {
    //     customFont.setPointSize(8);
    // }
    ui->windowswitchkeyLabel->setFont(customFont);
    ui->mappingswitchkeyLabel->setFont(customFont);
    ui->installViGEmBusButton->setFont(customFont);
    // ui->uninstallViGEmBusButton->setFont(customFont);
    ui->enableVirtualJoystickCheckBox->setFont(customFont);
    ui->lockCursorCheckBox->setFont(customFont);
    ui->ViGEmBusStatusLabel->setFont(customFont);
    ui->vJoyXSensLabel->setFont(customFont);
    ui->vJoyYSensLabel->setFont(customFont);
    ui->virtualgamepadGroupBox->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(12);
    }
    else {
        customFont.setPointSize(10);
    }
    ui->autoStartMappingCheckBox->setFont(customFont);
    ui->autoStartupCheckBox->setFont(customFont);
    ui->soundEffectCheckBox->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(11);
    }
    else {
        customFont.setPointSize(9);
    }
    ui->disableWinKeyCheckBox->setFont(customFont);
    ui->dataPortLabel->setFont(customFont);
    ui->brakeThresholdLabel->setFont(customFont);
    ui->accelThresholdLabel->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        QRect curGeometry = ui->virtualGamepadTypeComboBox->geometry();
        curGeometry.moveLeft(curGeometry.x() - 10);
        ui->virtualGamepadTypeComboBox->setGeometry(curGeometry);
    }
}

void QKeyMapper::changeControlEnableStatus(bool status)
{
    if (true == status && GROUPNAME_GLOBALSETTING == ui->settingselectComboBox->currentText()) {
        ui->nameCheckBox->setEnabled(false);
        ui->titleCheckBox->setEnabled(false);
        ui->removeSettingButton->setEnabled(false);
        ui->disableWinKeyCheckBox->setEnabled(false);
    }
    else {
        ui->nameCheckBox->setEnabled(status);
        ui->titleCheckBox->setEnabled(status);
        ui->removeSettingButton->setEnabled(status);
        ui->disableWinKeyCheckBox->setEnabled(status);
    }

    //ui->nameLineEdit->setEnabled(status);
    //ui->titleLineEdit->setEnabled(status);
    ui->titleLineEdit->setReadOnly(!status);
    ui->autoStartMappingCheckBox->setEnabled(status);
    ui->autoStartupCheckBox->setEnabled(status);
    ui->soundEffectCheckBox->setEnabled(status);
    ui->languageComboBox->setEnabled(status);
    ui->virtualGamepadTypeComboBox->setEnabled(status);
    ui->burstpressSpinBox->setEnabled(status);
    ui->burstreleaseSpinBox->setEnabled(status);
    ui->settingselectComboBox->setEnabled(status);
    // ui->settingselectLabel->setEnabled(status);
    ui->burstpressLabel->setEnabled(status);
    // ui->burstpress_msLabel->setEnabled(status);
    ui->burstreleaseLabel->setEnabled(status);
    // ui->burstrelease_msLabel->setEnabled(status);
    ui->waitTimeLabel->setEnabled(status);
    // ui->waitTime_msLabel->setEnabled(status);
    ui->waitTimeSpinBox->setEnabled(status);
    ui->mouseXSpeedLabel->setEnabled(status);
    ui->mouseYSpeedLabel->setEnabled(status);
    ui->mouseXSpeedSpinBox->setEnabled(status);
    ui->mouseYSpeedSpinBox->setEnabled(status);

    ui->dataPortLabel->setEnabled(status);
    ui->dataPortSpinBox->setEnabled(status);
    ui->brakeThresholdLabel->setEnabled(status);
    ui->brakeThresholdDoubleSpinBox->setEnabled(status);
    ui->accelThresholdLabel->setEnabled(status);
    ui->accelThresholdDoubleSpinBox->setEnabled(status);

    ui->orikeyLabel->setEnabled(status);
    ui->orikeySeqLabel->setEnabled(status);
    m_originalKeySeqEdit->setEnabled(status);
    ui->mapkeyLabel->setEnabled(status);
    m_orikeyComboBox->setEnabled(status);

#ifdef VIGEM_CLIENT_SUPPORT
    if (true == status
        && (m_orikeyComboBox->currentText() == VJOY_MOUSE2LS_STR
            || m_orikeyComboBox->currentText() == VJOY_MOUSE2RS_STR
            || m_orikeyComboBox->currentText() == JOY_LS2MOUSE_STR
            || m_orikeyComboBox->currentText() == JOY_RS2MOUSE_STR
            || m_orikeyComboBox->currentText() == JOY_LT2VJOYLT_STR
            || m_orikeyComboBox->currentText() == JOY_RT2VJOYRT_STR)) {
        m_mapkeyComboBox->setCurrentText(QString());
        m_mapkeyComboBox->setEnabled(false);
    }
    else {
        m_mapkeyComboBox->setEnabled(status);
    }
#else
    m_mapkeyComboBox->setEnabled(status);
#endif

    ui->addmapdataButton->setEnabled(status);
    ui->deleteoneButton->setEnabled(status);
    ui->clearallButton->setEnabled(status);

#ifdef VIGEM_CLIENT_SUPPORT
    if (false == status) {
        ui->enableVirtualJoystickCheckBox->setEnabled(status);
    }
    ui->installViGEmBusButton->setEnabled(status);
    // ui->uninstallViGEmBusButton->setEnabled(status);
    ui->virtualgamepadGroupBox->setEnabled(status);

    if (false == status || ui->enableVirtualJoystickCheckBox->isChecked()) {
        ui->vJoyXSensLabel->setEnabled(status);
        ui->vJoyYSensLabel->setEnabled(status);
        ui->vJoyXSensSpinBox->setEnabled(status);
        ui->vJoyYSensSpinBox->setEnabled(status);
        ui->lockCursorCheckBox->setEnabled(status);
    }
#endif

    ui->moveupButton->setEnabled(status);
    ui->movedownButton->setEnabled(status);
    ui->nextarrowCheckBox->setEnabled(status);

    ui->windowswitchkeyLabel->setEnabled(status);
    m_windowswitchKeySeqEdit->setEnabled(status);
    ui->mappingswitchkeyLabel->setEnabled(status);
    m_mappingswitchKeySeqEdit->setEnabled(status);

    // ui->refreshButton->setEnabled(status);
    ui->savemaplistButton->setEnabled(status);

    ui->processinfoTable->setEnabled(status);
    ui->keymapdataTable->setEnabled(status);
}

void QKeyMapper::extractSoundFiles()
{
    QFile soundFileStartLocal(SOUNDFILE_START);
    if (false == soundFileStartLocal.exists()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractSoundFiles]" << "Local sound file QKeyMapperStart.wav do not exist";
#endif
        QFile soundFileStartQRC(SOUNDFILE_START_QRC);
        if (soundFileStartQRC.exists()){
            soundFileStartQRC.copy(SOUNDFILE_START);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning() << "[extractSoundFiles]" << "QRC sound file QKeyMapperStart.wav do not exist!!!";
#endif
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractSoundFiles]" << "Local sound file QKeyMapperStart.wav already exist";
#endif
    }

    QFile soundFileStopLocal(SOUNDFILE_STOP);
    if (false == soundFileStopLocal.exists()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractSoundFiles]" << "Local sound file QKeyMapperStop.wav do not exist";
#endif
        QFile soundFileStopQRC(SOUNDFILE_STOP_QRC);
        if (soundFileStopQRC.exists()){
            soundFileStopQRC.copy(SOUNDFILE_STOP);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning() << "[extractSoundFiles]" << "QRC sound file QKeyMapperStop.wav do not exist!!!";
#endif
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractSoundFiles]" << "Local sound file QKeyMapperStop.wav already exist";
#endif
    }
}

void QKeyMapper::playStartSound()
{
    if (!ui->soundEffectCheckBox->isChecked()) {
        return;
    }

    QFile soundFileStartLocal(SOUNDFILE_START);

    if (soundFileStartLocal.exists()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[playStartSound]" << "Play sound file ->" << SOUNDFILE_START;
#endif
        std::wstring startSound = QString(SOUNDFILE_START).toStdWString();
        PlaySound(startSound.c_str(), NULL/*AfxGetInstanceHandle()*/, SND_FILENAME|SND_ASYNC);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[playStartSound]" << "Sound file do not exist ->" << SOUNDFILE_START;
#endif
    }
}

void QKeyMapper::playStopSound()
{
    if (!ui->soundEffectCheckBox->isChecked()) {
        return;
    }

    QFile soundFileStopLocal(SOUNDFILE_STOP);

    if (soundFileStopLocal.exists()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[playStopSound]" << "Play sound file ->" << SOUNDFILE_STOP;
#endif
        std::wstring stopSound = QString(SOUNDFILE_STOP).toStdWString();
        PlaySound(stopSound.c_str(), NULL/*AfxGetInstanceHandle()*/, SND_FILENAME|SND_ASYNC);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[playStopSound]" << "Sound file do not exist ->" << SOUNDFILE_STOP;
#endif
    }
}

#ifdef VIGEM_CLIENT_SUPPORT
int QKeyMapper::installViGEmBusDriver()
{
    QString operate_str = QString("runas");
    QString executable_str = QString("ViGEmBusDriver\\nefconw.exe");
    QString remove_old_devnode1_argument_str = QString("--remove-device-node --hardware-id Nefarius\\ViGEmBus\\Gen1 --class-guid 4D36E97D-E325-11CE-BFC1-08002BE10318");
    QString remove_old_devnode2_argument_str = QString("--remove-device-node --hardware-id Root\\ViGEmBus --class-guid 4D36E97D-E325-11CE-BFC1-08002BE10318");
    QString create_devnode_argument_str = QString("--create-device-node --hardware-id Nefarius\\ViGEmBus\\Gen1 --class-name System --class-guid 4D36E97D-E325-11CE-BFC1-08002BE10318");
    QString install_argument_str = QString("--install-driver --inf-path \"ViGEmBusDriver\\ViGEmBus.inf\"");

    std::wstring operate;
    std::wstring executable;
    std::wstring argument;
    HINSTANCE ret_instance;
    INT64 ret;

    /* ViGEmBus Remove Old Device Node 1 */
    operate = operate_str.toStdWString();
    executable = executable_str.toStdWString();
    argument = remove_old_devnode1_argument_str.toStdWString();

    ret_instance = ShellExecute(Q_NULLPTR, operate.c_str(), executable.c_str(), argument.c_str(), Q_NULLPTR, SW_HIDE);
    ret = (INT64)ret_instance;
    if(ret > 32) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Remove Old ViGEmBus Device Node 1 Success. ->" << ret;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Remove Old ViGEmBus Device Node 1 Failed!!! ->" << ret;
#endif
    }

    /* ViGEmBus Remove Old Device Node 2 */
    operate = operate_str.toStdWString();
    executable = executable_str.toStdWString();
    argument = remove_old_devnode2_argument_str.toStdWString();

    ret_instance = ShellExecute(Q_NULLPTR, operate.c_str(), executable.c_str(), argument.c_str(), Q_NULLPTR, SW_HIDE);
    ret = (INT64)ret_instance;
    if(ret > 32) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Remove Old ViGEmBus Device Node 2 Success. ->" << ret;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Remove Old ViGEmBus Device Node 2 Failed!!! ->" << ret;
#endif
    }

    /* ViGEmBus Create Device Node */
    operate = operate_str.toStdWString();
    executable = executable_str.toStdWString();
    argument = create_devnode_argument_str.toStdWString();

    ret_instance = ShellExecute(Q_NULLPTR, operate.c_str(), executable.c_str(), argument.c_str(), Q_NULLPTR, SW_HIDE);
    ret = (INT64)ret_instance;
    if(ret > 32) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Create ViGEmBus Device Node Success. ->" << ret;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Create ViGEmBus Device Node Failed!!! ->" << ret;
#endif
    }


    /* ViGEmBus Install Inf Driver */
    operate = operate_str.toStdWString();
    executable = executable_str.toStdWString();
    argument = install_argument_str.toStdWString();

    ret_instance = ShellExecute(Q_NULLPTR, operate.c_str(), executable.c_str(), argument.c_str(), Q_NULLPTR, SW_HIDE);
    ret = (INT64)ret_instance;
    if(ret > 32) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Install ViGEmBus INF Driver Success. ->" << ret;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Install ViGEmBus INF Driver Failed!!! ->" << ret;
#endif
    }

    return 0;
}

int QKeyMapper::uninstallViGEmBusDriver()
{
    QString operate_str = QString("runas");
    QString executable_str = QString("ViGEmBusDriver\\nefconw.exe");
    QString remove_devnode_argument_str = QString("--remove-device-node --hardware-id Nefarius\\ViGEmBus\\Gen1 --class-name System --class-guid 4D36E97D-E325-11CE-BFC1-08002BE10318");
    QString uninstall_argument_str = QString("--uninstall-driver --inf-path \"ViGEmBusDriver\\ViGEmBus.inf\"");

    std::wstring operate;
    std::wstring executable;
    std::wstring argument;
    HINSTANCE ret_instance;
    INT64 ret;

    /* ViGEmBus Remove Device Node */
    operate = operate_str.toStdWString();
    executable = executable_str.toStdWString();
    argument = remove_devnode_argument_str.toStdWString();

    ret_instance = ShellExecute(Q_NULLPTR, operate.c_str(), executable.c_str(), argument.c_str(), Q_NULLPTR, SW_HIDE);
    ret = (INT64)ret_instance;
    if(ret > 32) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Remove ViGEmBus Device Node Success. ->" << ret;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Remove ViGEmBus Device Node Failed!!! ->" << ret;
#endif
    }

    /* ViGEmBus Uninstall Inf Driver */
    operate = operate_str.toStdWString();
    executable = executable_str.toStdWString();
    argument = uninstall_argument_str.toStdWString();

    ret_instance = ShellExecute(Q_NULLPTR, operate.c_str(), executable.c_str(), argument.c_str(), Q_NULLPTR, SW_HIDE);
    ret = (INT64)ret_instance;
    if(ret > 32) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Uninstall ViGEmBus INF Driver Success. ->" << ret;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Uninstall ViGEmBus INF Driver Failed!!! ->" << ret;
#endif
    }

    return 0;
}

void QKeyMapper::updateViGEmBusLabelDisplay()
{
    QKeyMapper_Worker::ViGEmClient_ConnectState connectstate = QKeyMapper_Worker::ViGEmClient_getConnectState();

#ifdef DEBUG_LOGOUT_ON
    static QKeyMapper_Worker::ViGEmClient_ConnectState lastConnectState = QKeyMapper_Worker::VIGEMCLIENT_CONNECTING;
    if (lastConnectState != connectstate) {
        lastConnectState = connectstate;
        qDebug() << "[updateViGEmBusLabelDisplay]" << "ViGEmClient Connect State ->" << lastConnectState;
    }
#endif

    int languageIndex = ui->languageComboBox->currentIndex();
    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == connectstate) {
        ui->enableVirtualJoystickCheckBox->setEnabled(true);
        // ui->installViGEmBusButton->setEnabled(false);
        if (LANGUAGE_ENGLISH == languageIndex) {
            ui->installViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_ENGLISH);
        }
        else {
            ui->installViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_CHINESE);
        }

        ui->ViGEmBusStatusLabel->setStyleSheet("color:green;");
        if (LANGUAGE_ENGLISH == languageIndex) {
            ui->ViGEmBusStatusLabel->setText(VIGEMBUSSTATUSLABEL_AVAILABLE_ENGLISH);
        }
        else {
            ui->ViGEmBusStatusLabel->setText(VIGEMBUSSTATUSLABEL_AVAILABLE_CHINESE);
        }
    }
    else {
        ui->enableVirtualJoystickCheckBox->setEnabled(false);
        // ui->installViGEmBusButton->setEnabled(true);
        if (LANGUAGE_ENGLISH == languageIndex) {
            ui->installViGEmBusButton->setText(INSTALLVIGEMBUSBUTTON_ENGLISH);
        }
        else {
            ui->installViGEmBusButton->setText(INSTALLVIGEMBUSBUTTON_CHINESE);
        }

        ui->ViGEmBusStatusLabel->setStyleSheet("color:red;");
        if (LANGUAGE_ENGLISH == languageIndex) {
            ui->ViGEmBusStatusLabel->setText(VIGEMBUSSTATUSLABEL_UNAVAILABLE_ENGLISH);
        }
        else {
            ui->ViGEmBusStatusLabel->setText(VIGEMBUSSTATUSLABEL_UNAVAILABLE_CHINESE);
        }
    }
}

void QKeyMapper::reconnectViGEmClient()
{
    int retval_connect = QKeyMapper_Worker::ViGEmClient_Connect();
    Q_UNUSED(retval_connect);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[reconnectViGEmClient]" << "ViGEmClient Connect State ->" << QKeyMapper_Worker::ViGEmClient_getConnectState();
#endif

    emit updateViGEmBusStatus_Signal();

    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == QKeyMapper_Worker::ViGEmClient_getConnectState()) {
        ui->enableVirtualJoystickCheckBox->setEnabled(true);
    }
    else {
        ui->enableVirtualJoystickCheckBox->setCheckState(Qt::Unchecked);
        ui->enableVirtualJoystickCheckBox->setEnabled(false);
    }
}
#endif

void QKeyMapper::on_savemaplistButton_clicked()
{
    saveKeyMapSetting();
}

void QKeyMapper::initHotKeySequence()
{
    QObject::connect(m_HotKey_ShowHide, &QHotkey::activated, this, &QKeyMapper::HotKeyActivated);

    QObject::connect(m_HotKey_StartStop, &QHotkey::activated, this, &QKeyMapper::HotKeyStartStopActivated);
}

void QKeyMapper::initProcessInfoTable(void)
{
    //ui->processinfoTable->setStyle(QStyleFactory::create("windows"));
    ui->processinfoTable->setFocusPolicy(Qt::NoFocus);
    ui->processinfoTable->setColumnCount(PROCESSINFO_TABLE_COLUMN_COUNT);
//    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << "Name"
//                                                                    << "PID"
//                                                                    << "Title" );

    ui->processinfoTable->horizontalHeader()->setStretchLastSection(true);
    ui->processinfoTable->horizontalHeader()->setHighlightSections(false);
    ui->processinfoTable->verticalHeader()->setVisible(false);
    ui->processinfoTable->verticalHeader()->setDefaultSectionSize(25);
    ui->processinfoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->processinfoTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->processinfoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_ProcessInfoTableDelegate = new StyledDelegate(ui->processinfoTable);
    ui->processinfoTable->setItemDelegateForColumn(PROCESS_PID_COLUMN, m_ProcessInfoTableDelegate);

#ifdef DEBUG_LOGOUT_ON
//    qDebug() << "verticalHeader->isVisible" << ui->processinfoTable->verticalHeader()->isVisible();
//    qDebug() << "selectionBehavior" << ui->processinfoTable->selectionBehavior();
//    qDebug() << "selectionMode" << ui->processinfoTable->selectionMode();
//    qDebug() << "editTriggers" << ui->processinfoTable->editTriggers();
//    qDebug() << "verticalHeader-DefaultSectionSize" << ui->processinfoTable->verticalHeader()->defaultSectionSize();
#endif
}

void QKeyMapper::refreshProcessInfoTable(void)
{
    bool isSelected = false;
    QList<QTableWidgetItem*> items = ui->processinfoTable->selectedItems();
    QString selectedProcess;
    QString selectedPID;
    QString selectedTitle;
    if (!items.empty()) {
        selectedProcess = items.at(PROCESS_NAME_COLUMN)->text();
        selectedPID = items.at(PROCESS_PID_COLUMN)->text();
        selectedTitle = items.at(PROCESS_TITLE_COLUMN)->text();
        isSelected = true;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[refreshProcessInfoTable]" << "Selected[" << items.size() << "] -> " << selectedProcess << " | " << selectedPID << " | " << selectedTitle;
#endif
    }

    ui->processinfoTable->clearContents();
    ui->processinfoTable->setRowCount(0);

    static_ProcessInfoList.clear();
#if 1
    EnumWindows((WNDENUMPROC)QKeyMapper::EnumWindowsProc, 0);
#else
    EnumProcessFunction();
#endif
    setProcessInfoTable(static_ProcessInfoList);

    ui->processinfoTable->sortItems(PROCESS_NAME_COLUMN);
    ui->processinfoTable->resizeColumnToContents(PROCESS_NAME_COLUMN);
    if (ui->processinfoTable->columnWidth(PROCESS_NAME_COLUMN) > PROCESS_NAME_COLUMN_WIDTH_MAX){
        ui->processinfoTable->setColumnWidth(PROCESS_NAME_COLUMN, PROCESS_NAME_COLUMN_WIDTH_MAX);
    }

    ui->processinfoTable->resizeColumnToContents(PROCESS_PID_COLUMN);

    if (isSelected) {
        int reselectrow = -1;
        for (int i = 0; i < ui->processinfoTable->rowCount(); ++i) {
            if (ui->processinfoTable->item(i, PROCESS_NAME_COLUMN)->text() == selectedProcess
                && ui->processinfoTable->item(i, PROCESS_PID_COLUMN)->text() == selectedPID
                && ui->processinfoTable->item(i, PROCESS_TITLE_COLUMN)->text() == selectedTitle) {
                reselectrow = i;
                break;
            }
        }

        if (reselectrow != -1) {
            QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, PROCESSINFO_TABLE_COLUMN_COUNT - 1);
            ui->processinfoTable->setRangeSelected(selection, true);
        }
    }
}

void QKeyMapper::setProcessInfoTable(QList<MAP_PROCESSINFO> &processinfolist)
{
    int rowindex = 0;
    ui->processinfoTable->setRowCount(processinfolist.size());
    for (const MAP_PROCESSINFO &processinfo : processinfolist)
    {
#if 0
        QFileInfo file_info(processinfo.FilePath);
        QFileIconProvider icon_provider;
        QIcon fileicon = icon_provider.icon(file_info);

        if (true == fileicon.isNull()){
            continue;
        }
#endif
        QTableWidgetItem *filename_TableItem = new QTableWidgetItem(processinfo.WindowIcon, processinfo.FileName);
        filename_TableItem->setToolTip(processinfo.FileName);
        ui->processinfoTable->setItem(rowindex, 0, filename_TableItem);

        QTableWidgetItem *pid_TableItem = new QTableWidgetItem(processinfo.PID);
        ui->processinfoTable->setItem(rowindex, 1, pid_TableItem);
        //ui->processinfoTable->item(rowindex, 1)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QTableWidgetItem *windowtitle_TableItem = new QTableWidgetItem(processinfo.WindowTitle);
        windowtitle_TableItem->setToolTip(processinfo.WindowTitle);
        ui->processinfoTable->setItem(rowindex, 2, windowtitle_TableItem);

        rowindex += 1;
    }
}

void QKeyMapper::updateProcessInfoDisplay()
{
    ui->nameLineEdit->setText(m_MapProcessInfo.FileName);
    ui->titleLineEdit->setText(m_MapProcessInfo.WindowTitle);
    if ((false == m_MapProcessInfo.FilePath.isEmpty())
            && (true == QFileInfo::exists(m_MapProcessInfo.FilePath))){
        ui->nameLineEdit->setToolTip(m_MapProcessInfo.FilePath);

        QFileIconProvider icon_provider;
        QIcon fileicon = icon_provider.icon(QFileInfo(m_MapProcessInfo.FilePath));

        if (false == fileicon.isNull()){
            m_MapProcessInfo.WindowIcon = fileicon;
            QList<QSize> iconsizeList = fileicon.availableSizes();
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[UpdateProcessInfo]" << "Icon availableSizes:" << iconsizeList;
#endif
            QSize selectedSize = QSize(0, 0);
            QSize selectedSize_previous = QSize(DEFAULT_ICON_WIDTH, DEFAULT_ICON_HEIGHT);
            for(const QSize &iconsize : qAsConst(iconsizeList)){
                if ((iconsize.width() >= DEFAULT_ICON_WIDTH)
                        && (iconsize.height() >= DEFAULT_ICON_HEIGHT)){
                    selectedSize = iconsize;
                    break;
                }
                selectedSize_previous = iconsize;
            }

            if (selectedSize == QSize(0, 0)){
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[UpdateProcessInfo]" << "No available icon size, use previous icon size:" << selectedSize_previous;
#endif
                selectedSize = selectedSize_previous;
            }
            else if (selectedSize.width() > DEFAULT_ICON_WIDTH || selectedSize.height() > DEFAULT_ICON_HEIGHT) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[UpdateProcessInfo]" << "Icon size larger than default, use previous icon size:" << selectedSize_previous;
#endif
                selectedSize = selectedSize_previous;
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[UpdateProcessInfo]" << "Icon selectedSize is" << selectedSize;
#endif
            QPixmap IconPixmap = m_MapProcessInfo.WindowIcon.pixmap(selectedSize);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[UpdateProcessInfo]" << "Pixmap devicePixelRatio is" << IconPixmap.devicePixelRatio();
//            IconPixmap.save("selecticon.png");
#endif
            ui->iconLabel->setPixmap(IconPixmap);
        }
        else{
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[LoadSetting]" << "Load & retrive file icon failure!!!";
#endif
        }
    }
    else{
        if ((DEFAULT_NAME == ui->nameLineEdit->text())
                && (DEFAULT_TITLE == ui->titleLineEdit->text())){
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[LoadSetting]" << "Default icon availableSizes:" << m_MapProcessInfo.WindowIcon.availableSizes();
#endif
            ui->iconLabel->setPixmap(m_MapProcessInfo.WindowIcon.pixmap(QSize(DEFAULT_ICON_WIDTH, DEFAULT_ICON_HEIGHT)));
        }
    }
}

void QKeyMapper::updateSystemTrayDisplay()
{
    if (KEYMAP_CHECKING == m_KeyMapStatus) {
        m_SysTrayIcon->setIcon(QIcon(":/QKeyMapper_checking.ico"));
        m_SysTrayIcon->setToolTip("QKeyMapper(Checking : " + m_MapProcessInfo.FileName + ")");
    }
    else if (KEYMAP_MAPPING_MATCHED == m_KeyMapStatus) {
        m_SysTrayIcon->setIcon(QIcon(":/QKeyMapper_mapping_matched.ico"));
        m_SysTrayIcon->setToolTip("QKeyMapper(Mapping : " + m_MapProcessInfo.FileName + ")");
    }
    else if (KEYMAP_MAPPING_GLOBAL == m_KeyMapStatus) {
        /* Need to make a new global mapping status ICO */
        m_SysTrayIcon->setIcon(QIcon(":/QKeyMapper_mapping_global.ico"));
        m_SysTrayIcon->setToolTip("QKeyMapper(Mapping : Global)");
    }
}

void QKeyMapper::initKeyMappingDataTable(void)
{
    //ui->keymapdataTable->setStyle(QStyleFactory::create("windows"));
    ui->keymapdataTable->setFocusPolicy(Qt::NoFocus);
    ui->keymapdataTable->setColumnCount(KEYMAPPINGDATA_TABLE_COLUMN_COUNT);
//    ui->keymapdataTable->setHorizontalHeaderLabels(QStringList()   << "Original Key"
//                                                                    << "Mapping Key"
//                                                                    << "Burst"
//                                                                    << "Lock");

    ui->keymapdataTable->horizontalHeader()->setStretchLastSection(true);
    ui->keymapdataTable->horizontalHeader()->setHighlightSections(false);
    ui->keymapdataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    int original_key_width = ui->keymapdataTable->width()/4 - 15;
    int burst_mode_width = ui->keymapdataTable->width()/5 - 40;
    int lock_width = ui->keymapdataTable->width()/5 - 40;
    int mapping_key_width = ui->keymapdataTable->width() - original_key_width - burst_mode_width - lock_width - 12;
    ui->keymapdataTable->setColumnWidth(ORIGINAL_KEY_COLUMN, original_key_width);
    ui->keymapdataTable->setColumnWidth(MAPPING_KEY_COLUMN, mapping_key_width);
    ui->keymapdataTable->setColumnWidth(BURST_MODE_COLUMN, burst_mode_width);
    ui->keymapdataTable->setColumnWidth(LOCK_COLUMN, lock_width);
    ui->keymapdataTable->verticalHeader()->setVisible(false);
    ui->keymapdataTable->verticalHeader()->setDefaultSectionSize(25);
    ui->keymapdataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->keymapdataTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->keymapdataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

#ifdef DEBUG_LOGOUT_ON
//    qDebug() << "verticalHeader->isVisible" << ui->keymapdataTable->verticalHeader()->isVisible();
//    qDebug() << "selectionBehavior" << ui->keymapdataTable->selectionBehavior();
//    qDebug() << "selectionMode" << ui->keymapdataTable->selectionMode();
//    qDebug() << "editTriggers" << ui->keymapdataTable->editTriggers();
//    qDebug() << "verticalHeader-DefaultSectionSize" << ui->keymapdataTable->verticalHeader()->defaultSectionSize();
#endif
}

void QKeyMapper::initAddKeyComboBoxes(void)
{
    QStringList keycodelist = QStringList() \
            << ""
            << "Mouse-L"
            << "Mouse-R"
            << "Mouse-M"
            << "Mouse-X1"
            << "Mouse-X2"
            << "Mouse-WheelUp"
            << "Mouse-WheelDown"
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
            << "1!"
            << "2@"
            << "3#"
            << "4$"
            << "5%"
            << "6^"
            << "7&"
            << "8*"
            << "9("
            << "0)"
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
            << "Shift"
            << "L-Shift"
            << "R-Shift"
            << "Ctrl"
            << "L-Ctrl"
            << "R-Ctrl"
            << "Alt"
            << "L-Alt"
            << "R-Alt"
            << "L-Win"
            << "R-Win"
            << "Backspace"
            << "`~"
            << "-_"
            << "=+"
            << "[{"
            << "]}"
            << "\\|"
            << ";:"
            << "'\""
            << ",<"
            << ".>"
            << "/?"
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
            << "Num /"
            << "Num *"
            << "Num -"
            << "Num +"
            << "Num ."
            << "Num 0"
            << "Num 1"
            << "Num 2"
            << "Num 3"
            << "Num 4"
            << "Num 5"
            << "Num 6"
            << "Num 7"
            << "Num 8"
            << "Num 9"
            << "Num Enter"
            << "Num .(NumOFF)"
            << "Num 0(NumOFF)"
            << "Num 1(NumOFF)"
            << "Num 2(NumOFF)"
            << "Num 3(NumOFF)"
            << "Num 4(NumOFF)"
            << "Num 5(NumOFF)"
            << "Num 6(NumOFF)"
            << "Num 7(NumOFF)"
            << "Num 8(NumOFF)"
            << "Num 9(NumOFF)"
            << "Vol Mute"
            << "Vol Down"
            << "Vol Up"
            << "Media Next"
            << "Media Prev"
            << "Media Stop"
            << "Media PlayPause"
            << "Launch Mail"
            << "Select Media"
            << "Launch App1"
            << "Launch App2"
            << "Browser Back"
            << "Browser Forward"
            << "Browser Refresh"
            << "Browser Stop"
            << "Browser Search"
            << "Browser Favorites"
            << "Browser Home"
#ifdef VIGEM_CLIENT_SUPPORT
            << VJOY_MOUSE2LS_STR
            << VJOY_MOUSE2RS_STR
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
            << VJOY_LT_BRAKE_STR
            << VJOY_RT_BRAKE_STR
            << VJOY_LT_ACCEL_STR
            << VJOY_RT_ACCEL_STR
#endif
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
#ifdef VIGEM_CLIENT_SUPPORT
            << JOY_LT2VJOYLT_STR
            << JOY_RT2VJOYRT_STR
#endif
            << JOY_LS2MOUSE_STR
            << JOY_RS2MOUSE_STR
            ;

    int top = ui->orikeyLabel->y();
    m_orikeyComboBox->setObjectName(QStringLiteral("orikeyComboBox"));
    m_orikeyComboBox->setGeometry(QRect(587, top, 122, 22));
    m_mapkeyComboBox->setObjectName(QStringLiteral("mapkeyComboBox"));
    m_mapkeyComboBox->setGeometry(QRect(775, top, 122, 22));

    QStringList orikeycodelist = keycodelist;

    /* Remove Joy Keys from MappingKey ComboBox >>> */
#ifdef VIGEM_CLIENT_SUPPORT
    QRegularExpression re_vjoy("^vJoy-");
    QStringList vJoyStrlist = orikeycodelist.filter(re_vjoy);
    vJoyStrlist.removeOne(VJOY_MOUSE2LS_STR);
    vJoyStrlist.removeOne(VJOY_MOUSE2RS_STR);

    // Remove Strings start with "vJoy-" from orikeyComboBox
    for (const QString &joystr : qAsConst(vJoyStrlist)){
        orikeycodelist.removeOne(joystr);
    }
    keycodelist.removeOne(VJOY_MOUSE2LS_STR);
    keycodelist.removeOne(VJOY_MOUSE2RS_STR);
#endif

    QRegularExpression re_Joy("^Joy-");
    QStringList JoyStrlist = orikeycodelist.filter(re_Joy);

    // Remove Strings start with "Joy-" from mapkeyComboBox
    for (const QString &joystr : qAsConst(JoyStrlist)){
        keycodelist.removeOne(joystr);
    }
    /* Remove Joy Keys from MappingKey ComboBox <<< */

    m_orikeyComboBox->addItems(orikeycodelist);
    m_mapkeyComboBox->addItems(keycodelist);
}

void QKeyMapper::initWindowSwitchKeySeqEdit()
{
    int top = ui->windowswitchkeyLabel->y();
    int left = ui->windowswitchkeyLabel->x() + ui->windowswitchkeyLabel->width() + 5;
    m_windowswitchKeySeqEdit->setObjectName(QStringLiteral("windowswitchKeySeqEdit"));
    m_windowswitchKeySeqEdit->setGeometry(QRect(left, top, 110, 21));
    m_windowswitchKeySeqEdit->setFocusPolicy(Qt::ClickFocus);
}

void QKeyMapper::initMappingSwitchKeySeqEdit()
{
    int top = ui->mappingswitchkeyLabel->y();
    int left = ui->mappingswitchkeyLabel->x() + ui->mappingswitchkeyLabel->width() + 5;
    m_mappingswitchKeySeqEdit->setObjectName(QStringLiteral("mappingswitchKeySeqEdit"));
    m_mappingswitchKeySeqEdit->setGeometry(QRect(left, top, 110, 21));
    m_mappingswitchKeySeqEdit->setFocusPolicy(Qt::ClickFocus);
}

void QKeyMapper::updateWindowSwitchKeySeq(const QKeySequence &keysequence)
{
    m_windowswitchKeySeqEdit->setLastKeySequence(keysequence.toString());
    m_HotKey_ShowHide->setShortcut(keysequence, true);
}

void QKeyMapper::updateMappingSwitchKeySeq(const QKeySequence &keysequence)
{
    m_mappingswitchKeySeqEdit->setLastKeySequence(keysequence.toString());
    m_HotKey_StartStop->setShortcut(keysequence, true);
}

void QKeyMapper::initOriginalKeySeqEdit()
{
    m_originalKeySeqEdit->setObjectName(QStringLiteral("originalKeySeqEdit"));
    int left = m_orikeyComboBox->x();
    int top = ui->waitTimeSpinBox->y();
    int width = m_orikeyComboBox->width();
    int height = m_orikeyComboBox->height();
    m_originalKeySeqEdit->setGeometry(QRect(left, top, width, height));
    m_originalKeySeqEdit->setFocusPolicy(Qt::ClickFocus);
}

void QKeyMapper::refreshKeyMappingDataTable()
{
    ui->keymapdataTable->clearContents();
    ui->keymapdataTable->setRowCount(0);

    if (false == KeyMappingDataList.isEmpty()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshKeyMappingDataTable]" << "KeyMappingDataList Start >>>";
#endif
        int rowindex = 0;
        ui->keymapdataTable->setRowCount(KeyMappingDataList.size());
        for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
        {
            bool disable_burstandlock = false;

#ifdef VIGEM_CLIENT_SUPPORT
            if (keymapdata.Original_Key == VJOY_MOUSE2LS_STR || keymapdata.Original_Key == VJOY_MOUSE2RS_STR) {
                disable_burstandlock = true;
            }

            if (keymapdata.Original_Key == JOY_LT2VJOYLT_STR || keymapdata.Original_Key == JOY_RT2VJOYRT_STR) {
                disable_burstandlock = true;
            }
#endif
            if (keymapdata.Original_Key == JOY_LS2MOUSE_STR || keymapdata.Original_Key == JOY_RS2MOUSE_STR) {
                disable_burstandlock = true;
            }

            if (keymapdata.Original_Key == MOUSE_STR_WHEEL_UP || keymapdata.Original_Key == MOUSE_STR_WHEEL_DOWN) {
                disable_burstandlock = true;
            }

            if (keymapdata.Mapping_Keys.size() > 1) {
                disable_burstandlock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().contains(SEPARATOR_WAITTIME)) {
                disable_burstandlock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().contains(VJOY_LT_BRAKE_STR)
                || keymapdata.Mapping_Keys.constFirst().contains(VJOY_RT_BRAKE_STR)
                || keymapdata.Mapping_Keys.constFirst().contains(VJOY_LT_ACCEL_STR)
                || keymapdata.Mapping_Keys.constFirst().contains(VJOY_RT_ACCEL_STR)) {
                disable_burstandlock = true;
            }

            /* ORIGINAL_KEY_COLUMN */
            QTableWidgetItem *ori_TableItem = new QTableWidgetItem(keymapdata.Original_Key);
            ori_TableItem->setToolTip(keymapdata.Original_Key);
            ui->keymapdataTable->setItem(rowindex, ORIGINAL_KEY_COLUMN  , ori_TableItem);

            /* MAPPING_KEY_COLUMN */
            QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
            QTableWidgetItem *mapping_TableItem = new QTableWidgetItem(mappingkeys_str);
            mapping_TableItem->setToolTip(mappingkeys_str);
            ui->keymapdataTable->setItem(rowindex, MAPPING_KEY_COLUMN   , mapping_TableItem);

            /* BURST_MODE_COLUMN */
            QTableWidgetItem *burstCheckBox = new QTableWidgetItem();
            if (keymapdata.Burst == true) {
                burstCheckBox->setCheckState(Qt::Checked);
            }
            else {
                burstCheckBox->setCheckState(Qt::Unchecked);
            }

#ifdef VIGEM_CLIENT_SUPPORT
            if (disable_burstandlock) {
                burstCheckBox->setFlags(burstCheckBox->flags() & ~Qt::ItemIsEnabled);
            }
#endif
            ui->keymapdataTable->setItem(rowindex, BURST_MODE_COLUMN    , burstCheckBox);

            /* LOCK_COLUMN */
            QTableWidgetItem *lockCheckBox = new QTableWidgetItem();
            if (keymapdata.Lock == true) {
                lockCheckBox->setCheckState(Qt::Checked);
            }
            else {
                lockCheckBox->setCheckState(Qt::Unchecked);
            }

#ifdef VIGEM_CLIENT_SUPPORT
            if (disable_burstandlock) {
                lockCheckBox->setFlags(lockCheckBox->flags() & ~Qt::ItemIsEnabled);
            }
#endif
            ui->keymapdataTable->setItem(rowindex, LOCK_COLUMN    , lockCheckBox);

            rowindex += 1;

#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[refreshKeyMappingDataTable]" << keymapdata.Original_Key << "->" << keymapdata.Mapping_Keys;
#endif
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshKeyMappingDataTable]" << "KeyMappingDataList End <<<";
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshKeyMappingDataTable]" << "Empty KeyMappingDataList";
#endif
    }
}

void QKeyMapper::reloadUILanguage()
{
    int languageIndex = ui->languageComboBox->currentIndex();

    if (LANGUAGE_ENGLISH == languageIndex) {
        setUILanguage_English();
    }
    else {
        setUILanguage_Chinese();
    }

#ifdef VIGEM_CLIENT_SUPPORT
    emit updateViGEmBusStatus_Signal();
#endif
}

void QKeyMapper::setUILanguage_Chinese()
{
    setControlFontChinese();

    if (m_KeyMapStatus != KEYMAP_IDLE) {
        ui->keymapButton->setText(KEYMAPBUTTON_STOP_CHINESE);
    }
    else {
        ui->keymapButton->setText(KEYMAPBUTTON_START_CHINESE);
    }

    // ui->refreshButton->setText(REFRESHBUTTON_CHINESE);
    ui->savemaplistButton->setText(SAVEMAPLISTBUTTON_CHINESE);
    ui->deleteoneButton->setText(DELETEONEBUTTON_CHINESE);
    ui->clearallButton->setText(CLEARALLBUTTON_CHINESE);
    ui->addmapdataButton->setText(ADDMAPDATABUTTON_CHINESE);
    ui->nameCheckBox->setText(NAMECHECKBOX_CHINESE);
    ui->titleCheckBox->setText(TITLECHECKBOX_CHINESE);
    ui->orikeyLabel->setText(ORIKEYLABEL_CHINESE);
    ui->orikeySeqLabel->setText(ORIKEYSEQLABEL_CHINESE);
    ui->mapkeyLabel->setText(MAPKEYLABEL_CHINESE);
    ui->burstpressLabel->setText(BURSTPRESSLABEL_CHINESE);
    ui->burstreleaseLabel->setText(BURSTRELEASE_CHINESE);
    // ui->burstpress_msLabel->setText(BURSTPRESS_MSLABEL_CHINESE);
    // ui->burstrelease_msLabel->setText(BURSTRELEASE_MSLABEL_CHINESE);
    ui->waitTimeLabel->setText(WAITTIME_CHINESE);
    // ui->waitTime_msLabel->setText(WAITTIME_MSLABEL_CHINESE);
    ui->mouseXSpeedLabel->setText(MOUSEXSPEEDLABEL_CHINESE);
    ui->mouseYSpeedLabel->setText(MOUSEYSPEEDLABEL_CHINESE);
    // ui->settingselectLabel->setText(SETTINGSELECTLABEL_CHINESE);
    ui->removeSettingButton->setText(REMOVESETTINGBUTTON_CHINESE);
    ui->disableWinKeyCheckBox->setText(DISABLEWINKEYCHECKBOX_CHINESE);
    ui->dataPortLabel->setText(DATAPORTLABEL_CHINESE);
    ui->brakeThresholdLabel->setText(BRAKETHRESHOLDLABEL_CHINESE);
    ui->accelThresholdLabel->setText(ACCELTHRESHOLDLABEL_CHINESE);
    ui->autoStartMappingCheckBox->setText(AUTOSTARTMAPPINGCHECKBOX_CHINESE);
    ui->autoStartupCheckBox->setText(AUTOSTARTUPCHECKBOX_CHINESE);
    ui->soundEffectCheckBox->setText(SOUNDEFFECTCHECKBOX_CHINESE);
    ui->windowswitchkeyLabel->setText(WINDOWSWITCHKEYLABEL_CHINESE);
    ui->mappingswitchkeyLabel->setText(MAPPINGSWITCHKEYLABEL_CHINESE);
#ifdef VIGEM_CLIENT_SUPPORT
    ui->virtualgamepadGroupBox->setTitle(VIRTUALGAMEPADGROUPBOX_CHINESE);
    ui->enableVirtualJoystickCheckBox->setText(ENABLEVIRTUALJOYSTICKCHECKBOX_CHINESE);
    ui->lockCursorCheckBox->setText(LOCKCURSORCHECKBOX_CHINESE);
    ui->vJoyXSensLabel->setText(VJOYXSENSLABEL_CHINESE);
    ui->vJoyYSensLabel->setText(VJOYYSENSLABEL_CHINESE);
    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == QKeyMapper_Worker::ViGEmClient_getConnectState()) {
        ui->installViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_CHINESE);
    }
    else {
        ui->installViGEmBusButton->setText(INSTALLVIGEMBUSBUTTON_CHINESE);
    }
    // ui->uninstallViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_CHINESE);
#endif

    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << PROCESSINFOTABLE_COL1_CHINESE
                                                                  << PROCESSINFOTABLE_COL2_CHINESE
                                                                  << PROCESSINFOTABLE_COL3_CHINESE );
    ui->keymapdataTable->setHorizontalHeaderLabels(QStringList()   << KEYMAPDATATABLE_COL1_CHINESE
                                                                 << KEYMAPDATATABLE_COL2_CHINESE
                                                                 << KEYMAPDATATABLE_COL3_CHINESE
                                                                 << KEYMAPDATATABLE_COL4_CHINESE);
}

void QKeyMapper::setUILanguage_English()
{
    setControlFontEnglish();

    if (m_KeyMapStatus != KEYMAP_IDLE) {
        ui->keymapButton->setText(KEYMAPBUTTON_STOP_ENGLISH);
    }
    else {
        ui->keymapButton->setText(KEYMAPBUTTON_START_ENGLISH);
    }

    // ui->refreshButton->setText(REFRESHBUTTON_ENGLISH);
    ui->savemaplistButton->setText(SAVEMAPLISTBUTTON_ENGLISH);
    ui->deleteoneButton->setText(DELETEONEBUTTON_ENGLISH);
    ui->clearallButton->setText(CLEARALLBUTTON_ENGLISH);
    ui->addmapdataButton->setText(ADDMAPDATABUTTON_ENGLISH);
    ui->nameCheckBox->setText(NAMECHECKBOX_ENGLISH);
    ui->titleCheckBox->setText(TITLECHECKBOX_ENGLISH);
    ui->orikeyLabel->setText(ORIKEYLABEL_ENGLISH);
    ui->orikeySeqLabel->setText(ORIKEYSEQLABEL_ENGLISH);
    ui->mapkeyLabel->setText(MAPKEYLABEL_ENGLISH);
    ui->burstpressLabel->setText(BURSTPRESSLABEL_ENGLISH);
    ui->burstreleaseLabel->setText(BURSTRELEASE_ENGLISH);
    // ui->burstpress_msLabel->setText(BURSTPRESS_MSLABEL_ENGLISH);
    // ui->burstrelease_msLabel->setText(BURSTRELEASE_MSLABEL_ENGLISH);
    ui->waitTimeLabel->setText(WAITTIME_ENGLISH);
    // ui->waitTime_msLabel->setText(WAITTIME_MSLABEL_ENGLISH);
    ui->mouseXSpeedLabel->setText(MOUSEXSPEEDLABEL_ENGLISH);
    ui->mouseYSpeedLabel->setText(MOUSEYSPEEDLABEL_ENGLISH);
    // ui->settingselectLabel->setText(SETTINGSELECTLABEL_ENGLISH);
    ui->removeSettingButton->setText(REMOVESETTINGBUTTON_ENGLISH);
    ui->disableWinKeyCheckBox->setText(DISABLEWINKEYCHECKBOX_ENGLISH);
    ui->dataPortLabel->setText(DATAPORTLABEL_ENGLISH);
    ui->brakeThresholdLabel->setText(BRAKETHRESHOLDLABEL_ENGLISH);
    ui->accelThresholdLabel->setText(ACCELTHRESHOLDLABEL_ENGLISH);
    ui->autoStartMappingCheckBox->setText(AUTOSTARTMAPPINGCHECKBOX_ENGLISH);
    ui->autoStartupCheckBox->setText(AUTOSTARTUPCHECKBOX_ENGLISH);
    ui->soundEffectCheckBox->setText(SOUNDEFFECTCHECKBOX_ENGLISH);
    ui->windowswitchkeyLabel->setText(WINDOWSWITCHKEYLABEL_ENGLISH);
    ui->mappingswitchkeyLabel->setText(MAPPINGSWITCHKEYLABEL_ENGLISH);
#ifdef VIGEM_CLIENT_SUPPORT
    ui->virtualgamepadGroupBox->setTitle(VIRTUALGAMEPADGROUPBOX_ENGLISH);
    ui->enableVirtualJoystickCheckBox->setText(ENABLEVIRTUALJOYSTICKCHECKBOX_ENGLISH);
    ui->lockCursorCheckBox->setText(LOCKCURSORCHECKBOX_ENGLISH);
    ui->vJoyXSensLabel->setText(VJOYXSENSLABEL_ENGLISH);
    ui->vJoyYSensLabel->setText(VJOYYSENSLABEL_ENGLISH);
    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == QKeyMapper_Worker::ViGEmClient_getConnectState()) {
        ui->installViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_ENGLISH);
    }
    else {
        ui->installViGEmBusButton->setText(INSTALLVIGEMBUSBUTTON_ENGLISH);
    }
    // ui->uninstallViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_ENGLISH);
#endif

    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << PROCESSINFOTABLE_COL1_ENGLISH
                                                                  << PROCESSINFOTABLE_COL2_ENGLISH
                                                                  << PROCESSINFOTABLE_COL3_ENGLISH );
    ui->keymapdataTable->setHorizontalHeaderLabels(QStringList()   << KEYMAPDATATABLE_COL1_ENGLISH
                                                                 << KEYMAPDATATABLE_COL2_ENGLISH
                                                                 << KEYMAPDATATABLE_COL3_ENGLISH
                                                                 << KEYMAPDATATABLE_COL4_ENGLISH);
}

void QKeyMapper::resetFontSize()
{
    ui->nextarrowCheckBox->setFont(QFont("Arial", 16));

    if (UI_SCALE_2K_PERCENT_100 == m_UI_Scale
        || UI_SCALE_2K_PERCENT_125 == m_UI_Scale
        || UI_SCALE_2K_PERCENT_150 == m_UI_Scale
        || UI_SCALE_1K_PERCENT_100 == m_UI_Scale
        || UI_SCALE_1K_PERCENT_125 == m_UI_Scale
        || UI_SCALE_1K_PERCENT_150 == m_UI_Scale) {

        ui->nameLineEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->titleLineEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->languageComboBox->setFont(QFont("Microsoft YaHei", 9));
        ui->virtualGamepadTypeComboBox->setFont(QFont("Microsoft YaHei", 9));
        m_orikeyComboBox->setFont(QFont("Microsoft YaHei", 9));
        m_mapkeyComboBox->setFont(QFont("Microsoft YaHei", 9));
        ui->settingselectComboBox->setFont(QFont("Microsoft YaHei", 9));
        m_windowswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        m_mappingswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        m_originalKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->waitTimeSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->dataPortSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->brakeThresholdDoubleSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->accelThresholdDoubleSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->mouseXSpeedSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->mouseYSpeedSpinBox->setFont(QFont("Microsoft YaHei", 9));

        ui->burstpressSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->burstreleaseSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->processinfoTable->setFont(QFont("Microsoft YaHei", 9));
        ui->keymapdataTable->setFont(QFont("Microsoft YaHei", 9));

        ui->vJoyXSensSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->vJoyYSensSpinBox->setFont(QFont("Microsoft YaHei", 9));
    }
    else {
        ui->nameLineEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->titleLineEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->languageComboBox->setFont(QFont("Microsoft YaHei", 9));
        ui->virtualGamepadTypeComboBox->setFont(QFont("Microsoft YaHei", 9));
        m_orikeyComboBox->setFont(QFont("Microsoft YaHei", 9));
        m_mapkeyComboBox->setFont(QFont("Microsoft YaHei", 9));
        ui->settingselectComboBox->setFont(QFont("Microsoft YaHei", 9));
        m_windowswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        m_mappingswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        m_originalKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->waitTimeSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->dataPortSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->brakeThresholdDoubleSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->accelThresholdDoubleSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->mouseXSpeedSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->mouseYSpeedSpinBox->setFont(QFont("Microsoft YaHei", 9));

        ui->burstpressSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->burstreleaseSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->processinfoTable->setFont(QFont("Microsoft YaHei", 9));
        ui->keymapdataTable->setFont(QFont("Microsoft YaHei", 9));

        ui->vJoyXSensSpinBox->setFont(QFont("Microsoft YaHei", 9));
        ui->vJoyYSensSpinBox->setFont(QFont("Microsoft YaHei", 9));
    }
}

void QKeyMapper::updateLockStatusDisplay()
{
    int rowindex = 0;
    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
    {
        if (m_KeyMapStatus == KEYMAP_MAPPING_MATCHED) {
            if (keymapdata.Lock == true) {
                if (keymapdata.LockStatus == true) {
                    ui->keymapdataTable->item(rowindex, LOCK_COLUMN)->setText("ON");
                    ui->keymapdataTable->item(rowindex, LOCK_COLUMN)->setForeground(Qt::darkMagenta);
                }
                else {
                    ui->keymapdataTable->item(rowindex, LOCK_COLUMN)->setText("OFF");
                    ui->keymapdataTable->item(rowindex, LOCK_COLUMN)->setForeground(Qt::black);
                }
            }
        }
        else {
            ui->keymapdataTable->item(rowindex, LOCK_COLUMN)->setText(QString());
            ui->keymapdataTable->item(rowindex, LOCK_COLUMN)->setForeground(Qt::black);
        }

        rowindex += 1;
    }
}

#ifdef SINGLE_APPLICATION
void QKeyMapper::raiseQKeyMapperWindow()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[raiseQKeyMapperWindow]" << "Secondary QKeyMapper Instances started.";
#endif
    showNormal();
    activateWindow();
    raise();
}
#endif

void QKeyMapper::updateShortcutsMap()
{
    freeShortcuts();

    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
    {
        if (keymapdata.Original_Key.startsWith(PREFIX_SHORTCUT))
        {
            QString shortcutstr = keymapdata.Original_Key;
            shortcutstr.remove(PREFIX_SHORTCUT);
            if (false == ShortcutsMap.contains(shortcutstr)) {
                ShortcutsMap.insert(shortcutstr, new QHotkey(this));
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning() << "[updateShortcutsMap]" << "Already contains Shortcut!!! ->" << shortcutstr;
#endif
            }
            QHotkey* hotkey = ShortcutsMap.value(shortcutstr);
            QObject::connect(hotkey, &QHotkey::activated, this, &QKeyMapper::HotKeyForMappingActivated);
            QObject::connect(hotkey, &QHotkey::released,  this, &QKeyMapper::HotKeyForMappingReleased);
            hotkey->setShortcut(QKeySequence(shortcutstr), true);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[updateShortcutsMap]" << "ShortcutsList ->" << ShortcutsMap.keys();
#endif
}

void QKeyMapper::freeShortcuts()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("[freeShortcuts] currentThread -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

    if (ShortcutsMap.isEmpty()) {
        return;
    }

    QList<QHotkey*> HotkeysList = ShortcutsMap.values();

    for (QHotkey* shortcut : qAsConst(HotkeysList)) {
        bool unregister = shortcut->setRegistered(false);
        Q_UNUSED(unregister);
#ifdef DEBUG_LOGOUT_ON
        if (false == unregister) {
            qWarning() << "[freeShortcuts]" << "unregister Shortcut[" << shortcut->shortcut().toString() << "] Failed!!!";
        }
#endif
    }

    for (QHotkey* shortcut : qAsConst(HotkeysList)) {
        if (shortcut != Q_NULLPTR) {
            delete shortcut;
        }
    }

    ShortcutsMap.clear();

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[freeShortcuts]" << "ShortcutsList ->" << ShortcutsMap.keys();
#endif
}

void QKeyMapper::HotKeyForMappingActivated(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[HotKeyForMappingActivated] Shortcut Activated [" << keyseqstr << "], KeyboardModifiers[" << modifiers << "]";
#endif

    Q_UNUSED(modifiers);

    emit QKeyMapper_Worker::getInstance()->HotKeyTrigger_Signal(keyseqstr, KEY_DOWN);
}

void QKeyMapper::HotKeyForMappingReleased(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[HotKeyForMappingActivated] Shortcut Released [" << keyseqstr << "], KeyboardModifiers[" << modifiers << "]";
#endif

    Q_UNUSED(modifiers);
    emit QKeyMapper_Worker::getInstance()->HotKeyTrigger_Signal(keyseqstr, KEY_UP);
}

void QKeyMapper::on_processinfoTable_doubleClicked(const QModelIndex &index)
{
    if ((KEYMAP_IDLE == m_KeyMapStatus)
            && (true == ui->processinfoTable->isEnabled())){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[SelectProcessInfo]" << "Table DoubleClicked" << index.row() << ui->processinfoTable->item(index.row(), 0)->text() << ui->processinfoTable->item(index.row(), 2)->text();
#endif
        ui->nameLineEdit->setEnabled(true);
        ui->titleLineEdit->setEnabled(true);
        ui->nameCheckBox->setEnabled(true);
        ui->titleCheckBox->setEnabled(true);
        ui->removeSettingButton->setEnabled(true);
        ui->disableWinKeyCheckBox->setEnabled(true);

        QString filename = ui->processinfoTable->item(index.row(), 0)->text();
        QString windowTitle = ui->processinfoTable->item(index.row(), 2)->text();

        int checksaveindex = checkSaveSettings(filename, windowTitle);
        if (TITLESETTING_INDEX_ANYTITLE < checksaveindex && checksaveindex <= TITLESETTING_INDEX_MAX) {
            QString loadSettingSelectStr = filename + SEPARATOR_TITLESETTING + QString(WINDOWTITLE_STRING) + QString::number(checksaveindex);
            QString curSettingSelectStr = ui->settingselectComboBox->currentText();
            if (curSettingSelectStr != loadSettingSelectStr) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[on_processinfoTable_doubleClicked] "<< "Setting Check Matched! Load setting -> [" << loadSettingSelectStr << "]";
#endif
                loadSetting_flag = true;
                bool loadresult = loadKeyMapSetting(loadSettingSelectStr);
                Q_UNUSED(loadresult)
                loadSetting_flag = false;
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[on_processinfoTable_doubleClicked]" << "Current setting select is already the same ->" << curSettingSelectStr;
#endif
            }
        }
        else {
            ui->settingselectComboBox->setCurrentText(QString());
        }

        ui->nameLineEdit->setText(filename);
        ui->titleLineEdit->setText(windowTitle);

        QString pidStr = ui->processinfoTable->item(index.row(), PROCESS_PID_COLUMN)->text();
        QString ProcessPath;
        DWORD dwProcessId = pidStr.toULong();

        getProcessInfoFromPID(dwProcessId, ProcessPath);

        QIcon fileicon = ui->processinfoTable->item(index.row(), PROCESS_NAME_COLUMN)->icon();
        setMapProcessInfo(ui->processinfoTable->item(index.row(), PROCESS_NAME_COLUMN)->text(),
                          ui->processinfoTable->item(index.row(), PROCESS_TITLE_COLUMN)->text(),
                          ui->processinfoTable->item(index.row(), PROCESS_PID_COLUMN)->text(),
                          ProcessPath,
                          fileicon);

        QList<QSize> iconsizeList = fileicon.availableSizes();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[SelectProcessInfo]" << "Icon availableSizes:" << iconsizeList;
#endif
        QSize selectedSize = QSize(0, 0);
        QSize selectedSize_previous = QSize(DEFAULT_ICON_WIDTH, DEFAULT_ICON_HEIGHT);
        for(const QSize &iconsize : qAsConst(iconsizeList)){
            if ((iconsize.width() >= DEFAULT_ICON_WIDTH)
                    && (iconsize.height() >= DEFAULT_ICON_HEIGHT)){
                selectedSize = iconsize;
                break;
            }
            selectedSize_previous = iconsize;
        }

        if (selectedSize == QSize(0, 0)){
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[SelectProcessInfo]" << "No available icon size, use previous icon size:" << selectedSize_previous;
#endif
            selectedSize = selectedSize_previous;
        }
        else if (selectedSize.width() > DEFAULT_ICON_WIDTH || selectedSize.height() > DEFAULT_ICON_HEIGHT) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[SelectProcessInfo]" << "Icon size larger than default, use previous icon size:" << selectedSize_previous;
#endif
            selectedSize = selectedSize_previous;
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[SelectProcessInfo]" << "Icon selectedSize is" << selectedSize;
#endif
        QPixmap IconPixmap = m_MapProcessInfo.WindowIcon.pixmap(selectedSize);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[SelectProcessInfo]" << "Pixmap devicePixelRatio is" << IconPixmap.devicePixelRatio();
//        IconPixmap.save("selecticon.png");
#endif
        ui->iconLabel->setPixmap(IconPixmap);
        ui->nameLineEdit->setToolTip(ProcessPath);
    }
}

void QKeyMapper::on_addmapdataButton_clicked()
{
    QString currentOriKeyText;
    QString currentMapKeyText = m_mapkeyComboBox->currentText();
    QString currentOriKeyComboBoxText = m_orikeyComboBox->currentText();
    QString currentOriKeyShortcutText = m_originalKeySeqEdit->keySequence().toString();
    if (false == currentOriKeyComboBoxText.isEmpty()) {
        currentOriKeyText = currentOriKeyComboBoxText;
    }
    else if (false == currentOriKeyShortcutText.isEmpty()) {
        currentOriKeyText = QString(PREFIX_SHORTCUT) + currentOriKeyShortcutText;
    }

    if (currentOriKeyText.isEmpty() || (m_mapkeyComboBox->isEnabled() && currentMapKeyText.isEmpty() && ui->nextarrowCheckBox->isChecked() == false)) {
        return;
    }

    bool already_exist = false;
    int findindex = -1;
    findindex = findOriKeyInKeyMappingDataList(currentOriKeyText);
    if (findindex != -1){
        if (VJOY_MOUSE2LS_STR == currentOriKeyText
            || VJOY_MOUSE2RS_STR == currentOriKeyText
            || JOY_LS2MOUSE_STR == currentOriKeyText
            || JOY_RS2MOUSE_STR == currentOriKeyText
            || JOY_LT2VJOYLT_STR == currentOriKeyText
            || JOY_RT2VJOYRT_STR == currentOriKeyText) {
            already_exist = true;
        }
        else if (KeyMappingDataList.at(findindex).Mapping_Keys.size() == 1
                && false == ui->nextarrowCheckBox->isChecked()
                && KeyMappingDataList.at(findindex).Mapping_Keys.contains(currentMapKeyText) == true){
                already_exist = true;
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "KeyMap already exist at KeyMappingDataList index : " << findindex;
#endif
        }
    }

    if (false == already_exist) {
        if (findindex != -1){
            if (currentMapKeyText == VJOY_LT_BRAKE_STR
                || currentMapKeyText == VJOY_RT_BRAKE_STR
                || currentMapKeyText == VJOY_LT_ACCEL_STR
                || currentMapKeyText == VJOY_RT_ACCEL_STR) {
                already_exist = true;
            }
            else {
                MAP_KEYDATA keymapdata = KeyMappingDataList.at(findindex);
                if (keymapdata.Mapping_Keys.contains(VJOY_LT_BRAKE_STR)
                    || keymapdata.Mapping_Keys.contains(VJOY_RT_BRAKE_STR)
                    || keymapdata.Mapping_Keys.contains(VJOY_LT_ACCEL_STR)
                    || keymapdata.Mapping_Keys.contains(VJOY_RT_ACCEL_STR)){
                    already_exist = true;
                }
            }
        }
        else {
            if (ui->nextarrowCheckBox->isChecked()) {
                if (currentMapKeyText == VJOY_LT_BRAKE_STR
                    || currentMapKeyText == VJOY_RT_BRAKE_STR
                    || currentMapKeyText == VJOY_LT_ACCEL_STR
                    || currentMapKeyText == VJOY_RT_ACCEL_STR) {
                    already_exist = true;
                }
            }
        }
    }

    if (false == already_exist) {
        if (findindex != -1) {
            MAP_KEYDATA keymapdata = KeyMappingDataList.at(findindex);
            if (keymapdata.Mapping_Keys.size() >= KEY_SEQUENCE_MAX) {
                QString message;
                if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                    message = QString("Key sequence mapping to \"%1\" is too long!").arg(currentOriKeyText);
                }
                else {
                    message = QString("映射到\"%1\"的按键序列太长了!").arg(currentOriKeyText);
                }
                QMessageBox::warning(this, PROGRAM_NAME, message);
                return;
            }
            QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "mappingkeys_str before add:" << mappingkeys_str;
#endif
            int waitTime = ui->waitTimeSpinBox->value();
            if (waitTime > 0) {
                currentMapKeyText = currentMapKeyText + QString(SEPARATOR_WAITTIME) + QString::number(waitTime);
            }
            if (ui->nextarrowCheckBox->isChecked()) {
                mappingkeys_str = mappingkeys_str + SEPARATOR_NEXTARROW + currentMapKeyText;
            }
            else {
                mappingkeys_str = mappingkeys_str + SEPARATOR_PLUS + currentMapKeyText;
            }

#ifdef DEBUG_LOGOUT_ON
            qDebug() << "mappingkeys_str after add:" << mappingkeys_str;
#endif
            KeyMappingDataList.replace(findindex, MAP_KEYDATA(currentOriKeyText, mappingkeys_str, keymapdata.Burst, keymapdata.Lock));
        }
        else {
            if (VJOY_MOUSE2LS_STR == currentOriKeyText
                || VJOY_MOUSE2RS_STR == currentOriKeyText
                || JOY_LS2MOUSE_STR == currentOriKeyText
                || JOY_RS2MOUSE_STR == currentOriKeyText
                || JOY_LT2VJOYLT_STR == currentOriKeyText
                || JOY_RT2VJOYRT_STR == currentOriKeyText) {
                currentMapKeyText = currentOriKeyText;
            }
            else {
                int waitTime = ui->waitTimeSpinBox->value();
                if (waitTime > 0
                    && currentMapKeyText != VJOY_LT_BRAKE_STR
                    && currentMapKeyText != VJOY_RT_BRAKE_STR
                    && currentMapKeyText != VJOY_LT_ACCEL_STR
                    && currentMapKeyText != VJOY_RT_ACCEL_STR) {
                    currentMapKeyText = currentMapKeyText + QString(SEPARATOR_WAITTIME) + QString::number(waitTime);
                }
            }

            KeyMappingDataList.append(MAP_KEYDATA(currentOriKeyText, currentMapKeyText, false, false));
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "Add keymapdata :" << currentOriKeyText << "to" << currentMapKeyText;
#endif
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable();
    }
    else {
        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
            QMessageBox::warning(this, PROGRAM_NAME, "Conflict with exist Keys!");
        }
        else {
            QMessageBox::warning(this, PROGRAM_NAME, "与已存在的按键映射冲突！");
        }
    }
}

void QKeyMapper::on_deleteoneButton_clicked()
{
    int currentrowindex = ui->keymapdataTable->currentRow();

#ifdef DEBUG_LOGOUT_ON
    qDebug("DeleteOne: currentRow(%d)", currentrowindex);
#endif

    if (currentrowindex >= 0){
        ui->keymapdataTable->removeRow(currentrowindex);
        KeyMappingDataList.removeAt(currentrowindex);

        /* do not refresh for select cursor hold position */
//        refreshKeyMappingDataTable();
#ifdef DEBUG_LOGOUT_ON
        if (ui->keymapdataTable->rowCount() != KeyMappingDataList.size()){
            qDebug("KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", ui->keymapdataTable->rowCount(), KeyMappingDataList.size());
        }
#endif

    }
}

void QKeyMapper::on_clearallButton_clicked()
{
    ui->keymapdataTable->clearContents();
    ui->keymapdataTable->setRowCount(0);
    KeyMappingDataList.clear();
}

void StyledDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (PROCESS_PID_COLUMN == index.column())
    {
        QStyleOptionViewItem myOption = option;
        myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;

        QStyledItemDelegate::paint(painter, myOption, index);
    }
    else{
        QStyledItemDelegate::paint(painter, option, index);
    }
}

void KeyListComboBox::keyPressEvent(QKeyEvent *keyevent)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyListComboBox_Press]" << "Key:" << (Qt::Key)keyevent->key() << "Modifiers:" << keyevent->modifiers();
    qDebug("[KeyListComboBox_Press] VirtualKey(0x%08X), ScanCode(0x%08X), nModifiers(0x%08X)", keyevent->nativeVirtualKey(), keyevent->nativeScanCode(), keyevent->nativeModifiers());
    qDebug("[CAPS Status] KeyState(0x%04X)", GetKeyState(VK_CAPITAL));
#endif

    V_KEYCODE vkeycode;
    vkeycode.KeyCode = (quint8)keyevent->nativeVirtualKey();
    if (QT_KEY_EXTENDED == (keyevent->nativeModifiers() & QT_KEY_EXTENDED)){
        vkeycode.ExtenedFlag = EXTENED_FLAG_TRUE;
    }
    else{
        vkeycode.ExtenedFlag = EXTENED_FLAG_FALSE;
    }

    QString keycodeString = QKeyMapper_Worker::VirtualKeyCodeMap.key(vkeycode);

    if (VK_SHIFT == vkeycode.KeyCode){
        if (QT_KEY_L_SHIFT == (keyevent->nativeModifiers() & QT_KEY_L_SHIFT)){
            keycodeString = QString("L-Shift");
        }
        else if (QT_KEY_R_SHIFT == (keyevent->nativeModifiers() & QT_KEY_R_SHIFT)){
            keycodeString = QString("R-Shift");
        }
    }
    else if (VK_CONTROL == vkeycode.KeyCode){
        if (QT_KEY_L_CTRL == (keyevent->nativeModifiers() & QT_KEY_L_CTRL)){
            keycodeString = QString("L-Ctrl");
        }
        else if (QT_KEY_R_CTRL == (keyevent->nativeModifiers() & QT_KEY_R_CTRL)){
            keycodeString = QString("R-Ctrl");
        }
    }
    else if (VK_MENU == vkeycode.KeyCode){
        if (QT_KEY_L_ALT == (keyevent->nativeModifiers() & QT_KEY_L_ALT)){
            keycodeString = QString("L-Alt");
        }
        else if (QT_KEY_R_ALT == (keyevent->nativeModifiers() & QT_KEY_R_ALT)){
            keycodeString = QString("R-Alt");
        }
    }
    else{
    }

    if (false == keycodeString.isEmpty()){
        if ((keycodeString == QString("L-Win"))
                || (keycodeString == QString("R-Win"))){
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[KeyListComboBox_Press]" <<"Don't act on" << keycodeString;
#endif
        }
        else{
            if (keycodeString == QString("Enter")){
                QComboBox::keyPressEvent(keyevent);
            }
            else{
                this->setCurrentText(keycodeString);

#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[KeyListComboBox_Press]" << "convert to VirtualKeyCodeMap:" << keycodeString;
#endif
            }
        }
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[KeyListComboBox_Press]" << "Unknown key not found in VirtualKeyCodeMap.";
#endif
    }

    //QComboBox::keyPressEvent(keyevent);
}

void KeySequenceEditOnlyOne::setDefaultKeySequence(const QString &keysequencestr)
{
    m_DefaultKeySequence = keysequencestr;
}

QString KeySequenceEditOnlyOne::defaultKeySequence()
{
    return m_DefaultKeySequence;
}

void KeySequenceEditOnlyOne::setLastKeySequence(const QString &keysequencestr)
{
    m_LastKeySequence = keysequencestr;
}

QString KeySequenceEditOnlyOne::lastKeySequence()
{
    return m_LastKeySequence;
}

void KeySequenceEditOnlyOne::keyPressEvent(QKeyEvent* pEvent)
{
    __super::keyPressEvent(pEvent);

    QKeySequence keySeq = keySequence();
    QString keySeqStr = keySeq.toString(QKeySequence::NativeText);
    if (keySeq.count() <= 0 || keySeq.isEmpty())
    {
        return;
    }

    QKeySequence setKeySeq;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QKeyCombination keyComb = keySeq[0];
    if (keyComb.key() == Qt::Key_Backspace)
    {
        setKeySeq = QKeySequence(defaultKeySequence());
    }
    else {
        setKeySeq = QKeySequence(keyComb);
    }
#else
    int keyComb = keySeq[0];
    if (keyComb == Qt::Key_Backspace)
    {
        setKeySeq = QKeySequence(defaultKeySequence());
    }
    else {
        setKeySeq = QKeySequence(keyComb);
    }
#endif

    if (false == setKeySeq.isEmpty()) {
        QKeySequence keyseqConverted;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        keyseqConverted = QKeySequence(pEvent->keyCombination());
#else
        keyseqConverted = QKeySequence(pEvent->modifiers() + pEvent->key());
#endif
        QString keyseqConvertedStr = keyseqConverted.toString(QKeySequence::NativeText);
        if (keyseqConvertedStr != keySeqStr) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[KeySequenceEditOnlyOne]" << "Convert KeySequence [" << keySeqStr << "] -> [" << keyseqConvertedStr << "]";
#endif
            keySeqStr = keyseqConvertedStr;
            setKeySeq = QKeySequence(keyseqConvertedStr);
        }

        if (keySeqStr.length() < 2 || false == keySeqStr.contains("+") || keySeqStr.startsWith("Num+"))
        {
            setKeySeq = QKeySequence();
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[KeySequenceEditOnlyOne]" << "Change KeySequence [" << keySeqStr << "] -> [" << setKeySeq.toString(QKeySequence::NativeText) << "]";
#endif
        }
        setKeySequence(setKeySeq);
        emit keySeqEditChanged_Signal(setKeySeq);
    }
}

void QKeyMapper::on_moveupButton_clicked()
{
    int currentrowindex = -1;
    QList<QTableWidgetItem*> items = ui->keymapdataTable->selectedItems();
    if (items.size() > 0) {
        QTableWidgetItem* selectedItem = items.at(0);
        currentrowindex = ui->keymapdataTable->row(selectedItem);
#ifdef DEBUG_LOGOUT_ON
        if (currentrowindex > 0){
            qDebug() << "[MoveUpItem]" << selectedItem->text();
        }
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveUpItem] There is no selected item";
#endif
    }

    if (currentrowindex > 0){
#ifdef DEBUG_LOGOUT_ON
        qDebug("MoveUp: currentRow(%d)", currentrowindex);
#endif
        KeyMappingDataList.move(currentrowindex, currentrowindex-1);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable();

        int reselectrow = currentrowindex - 1;
        QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        ui->keymapdataTable->setRangeSelected(selection, true);

#ifdef DEBUG_LOGOUT_ON
        if (ui->keymapdataTable->rowCount() != KeyMappingDataList.size()){
            qDebug("MoveUp:KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", ui->keymapdataTable->rowCount(), KeyMappingDataList.size());
        }
#endif
    }
}

void QKeyMapper::on_movedownButton_clicked()
{
    int currentrowindex = -1;
    QList<QTableWidgetItem*> items = ui->keymapdataTable->selectedItems();
    if (items.size() > 0) {
        QTableWidgetItem* selectedItem = items.at(0);
        currentrowindex = ui->keymapdataTable->row(selectedItem);
#ifdef DEBUG_LOGOUT_ON
        if (currentrowindex >= 0
            && currentrowindex < (ui->keymapdataTable->rowCount()-1)){
            qDebug() << "[MoveDownItem]" << selectedItem->text();
        }
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveDownItem] There is no selected item";
#endif
    }

    if (currentrowindex >= 0
        && currentrowindex < (ui->keymapdataTable->rowCount()-1)){
#ifdef DEBUG_LOGOUT_ON
        qDebug("MoveDown: currentRow(%d)", currentrowindex);
#endif
        KeyMappingDataList.move(currentrowindex, currentrowindex+1);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable();

        int reselectrow = currentrowindex + 1;
        QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        ui->keymapdataTable->setRangeSelected(selection, true);

#ifdef DEBUG_LOGOUT_ON
        if (ui->keymapdataTable->rowCount() != KeyMappingDataList.size()){
            qDebug("MoveDown:KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", ui->keymapdataTable->rowCount(), KeyMappingDataList.size());
        }
#endif

//        int keycount = 0;
//        INPUT inputs[SEND_INPUTS_MAX] = { 0 };
//        QStringList mappingKeyList = KeyMappingDataList.at(reselectrow).Mapping_Keys;
//        keycount = QKeyMapper_Worker::getInstance()->makeKeySequenceInputarray(mappingKeyList, inputs);
//#ifdef DEBUG_LOGOUT_ON
//        qDebug("makeKeySequenceInputarray() -> keycount = %d", keycount);
//#endif
    }
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void QKeyMapper::on_settingselectComboBox_textActivated(const QString &text)
#else
void QKeyMapper::on_settingselectComboBox_currentTextChanged(const QString &text)
#endif
{
    if (loadSetting_flag) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_settingselectComboBox_textActivated/textChanged] Loading Setting not finished!";
#endif
        return;
    }

    if (false == text.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_settingselectComboBox_textActivated/textChanged] Change to Setting ->" << text;
#endif
        loadSetting_flag = true;
        bool loadresult = loadKeyMapSetting(text);
        Q_UNUSED(loadresult);
        loadSetting_flag = false;
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_settingselectComboBox_textActivated/textChanged] Select setting text Empty!";
#endif
        ui->nameLineEdit->setEnabled(true);
        ui->titleLineEdit->setEnabled(true);
        ui->nameCheckBox->setEnabled(true);
        ui->titleCheckBox->setEnabled(true);
        ui->removeSettingButton->setEnabled(true);
        ui->disableWinKeyCheckBox->setEnabled(true);
    }
}

void QKeyMapper::on_removeSettingButton_clicked()
{
    QString settingSelectStr = ui->settingselectComboBox->currentText();
    if (true == settingSelectStr.isEmpty() || settingSelectStr == GROUPNAME_GLOBALSETTING) {
        return;
    }

    int currentSettingIndex = ui->settingselectComboBox->currentIndex();
    QString currentSettingText;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    QStringList groups = settingFile.childGroups();
    if (groups.contains(settingSelectStr)) {
        settingFile.remove(settingSelectStr);
        ui->settingselectComboBox->removeItem(currentSettingIndex);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[removeSetting] Remove setting select ->" << settingSelectStr;
#endif
        currentSettingText = ui->settingselectComboBox->currentText();
        if (false == currentSettingText.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[removeSetting] Change to Setting ->" << currentSettingText;
#endif
            loadSetting_flag = true;
            bool loadresult = loadKeyMapSetting(currentSettingText);
            Q_UNUSED(loadresult);
            loadSetting_flag = false;
        }
    }
}


void QKeyMapper::on_autoStartupCheckBox_stateChanged(int state)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[autoStartup] Auto startup state changed ->" << (Qt::CheckState)state;
#endif
    bool isWin10Above = false;
    QOperatingSystemVersion osVersion = QOperatingSystemVersion::current();
    if (osVersion >= QOperatingSystemVersion::Windows10) {
        isWin10Above = true;
    }
    else {
        isWin10Above = false;
    }

    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    const char operate_char_binary[] = { 0b01110010, 0b01110101, 0b01101110, 0b01100001, 0b01110011, 0b00000000 }; // "runas"
    QString operate_str = QString(operate_char_binary);
    const char  executable_char_binary[] = {0b01110011, 0b01100011, 0b01101000, 0b01110100, 0b01100001, 0b01110011, 0b01101011, 0b01110011, 0b00000000}; // "schtasks"
    QString executable_str = QString(executable_char_binary);
    QString start_delay_str = QString("/delay 0000:30");
    QString create_argument_str;
    if (isWin10Above) {
        create_argument_str = QString("/create /f /sc onlogon ") + start_delay_str + QString(" /rl highest /tn QKeyMapper /tr ") + QCoreApplication::applicationFilePath();
    }
    else {
        create_argument_str = QString("/create /f /sc onlogon /rl highest /tn QKeyMapper /tr ") + QCoreApplication::applicationFilePath();
    }
    QString delete_argument_str = QString("/delete /f /tn QKeyMapper");
    if (Qt::Checked == state) {
        settingFile.setValue(AUTO_STARTUP , true);

        std::wstring operate = operate_str.toStdWString();
        std::wstring executable = executable_str.toStdWString();
        std::wstring argument = create_argument_str.toStdWString();
        HINSTANCE ret_instance = ShellExecute(Q_NULLPTR, operate.c_str(), executable.c_str(), argument.c_str(), Q_NULLPTR, SW_HIDE);
        INT64 ret = (INT64)ret_instance;
        if(ret > 32) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[autoStartup] schtasks create success ->" << ret;
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[autoStartup] schtasks create failed!!! ->" << ret;
#endif
        }
    }
    else {
        settingFile.setValue(AUTO_STARTUP , false);

        std::wstring operate = operate_str.toStdWString();
        std::wstring executable = executable_str.toStdWString();
        std::wstring argument = delete_argument_str.toStdWString();
        HINSTANCE ret_instance = ShellExecute(Q_NULLPTR, operate.c_str(), executable.c_str(), argument.c_str(), Q_NULLPTR, SW_HIDE);
        INT64 ret = (INT64)ret_instance;
        if(ret > 32) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[autoStartup] schtasks delete success ->" << ret;
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[autoStartup] schtasks delete failed!!! ->" << ret;
#endif
        }
    }
}


void QKeyMapper::on_languageComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    reloadUILanguage();

#ifdef DEBUG_LOGOUT_ON
    QString languageStr;
    if (LANGUAGE_ENGLISH == index) {
        languageStr = "English";
    }
    else {
        languageStr = "Chinese";
    }
    qDebug() << "[Language] Language changed ->" << languageStr;
#endif
}


void QKeyMapper::on_enableVirtualJoystickCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[EnableVirtualJoystick] Enable Virtual Joystick state changed ->" << (Qt::CheckState)state;
#endif

#ifdef VIGEM_CLIENT_SUPPORT
    bool checked = false;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);

    if (Qt::Checked == state) {
        int retval = QKeyMapper_Worker::ViGEmClient_Add();

        if (retval != 0) {
            ui->enableVirtualJoystickCheckBox->setCheckState(Qt::Unchecked);
#ifdef DEBUG_LOGOUT_ON
            qWarning() << "[EnableVirtualJoystick] Enable Virtual Joystick failed!!!";
#endif
        }
        else {
            checked = true;
        }
    }
    else {
        QKeyMapper_Worker::ViGEmClient_Remove();
        checked = false;
    }

    if (true == checked && QKeyMapper_Worker::s_ViGEmTarget != Q_NULLPTR) {
        ui->vJoyXSensSpinBox->setEnabled(true);
        ui->vJoyYSensSpinBox->setEnabled(true);
        ui->vJoyXSensLabel->setEnabled(true);
        ui->vJoyYSensLabel->setEnabled(true);
        ui->lockCursorCheckBox->setEnabled(true);

        settingFile.setValue(VIRTUALGAMEPAD_ENABLE , true);
    }
    else {
        ui->vJoyXSensSpinBox->setEnabled(false);
        ui->vJoyYSensSpinBox->setEnabled(false);
        ui->vJoyXSensLabel->setEnabled(false);
        ui->vJoyYSensLabel->setEnabled(false);
        ui->lockCursorCheckBox->setEnabled(false);

        settingFile.setValue(VIRTUALGAMEPAD_ENABLE , false);
    }
#endif
}


void QKeyMapper::on_installViGEmBusButton_clicked()
{
#ifdef VIGEM_CLIENT_SUPPORT
    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == QKeyMapper_Worker::ViGEmClient_getConnectState()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "Uninstall ViGEm Bus.";
#endif

        QKeyMapper_Worker::ViGEmClient_Remove();
        QKeyMapper_Worker::ViGEmClient_Disconnect();
        QKeyMapper_Worker::ViGEmClient_Free();

        emit updateViGEmBusStatus_Signal();
        ui->enableVirtualJoystickCheckBox->setCheckState(Qt::Unchecked);
        ui->enableVirtualJoystickCheckBox->setEnabled(false);

        (void)uninstallViGEmBusDriver();
    }
    else {
        QKeyMapper_Worker::ViGEmClient_ConnectState connectstate = QKeyMapper_Worker::ViGEmClient_getConnectState();
        if (QKeyMapper_Worker::VIGEMCLIENT_CONNECTING == connectstate
            || QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == connectstate) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[on_installViGEmBusButton_clicked]" <<"Skip Install ViGEm Bus on ConnectState ->" << connectstate;
#endif
            return;
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "Install ViGEm Bus.";
#endif
        QKeyMapper_Worker::ViGEmClient_setConnectState(QKeyMapper_Worker::VIGEMCLIENT_CONNECTING);
        emit updateViGEmBusStatus_Signal();

        if (QKeyMapper_Worker::s_ViGEmClient == Q_NULLPTR) {
            int retval_alloc = QKeyMapper_Worker::ViGEmClient_Alloc();
            if (retval_alloc != 0) {
#ifdef DEBUG_LOGOUT_ON
                qWarning("[on_installViGEmBusButton_clicked] ViGEmClient Alloc Failed!!! -> retval_alloc(%d)", retval_alloc);
#endif
            }
        }

        (void)installViGEmBusDriver();

        QTimer::singleShot(RECONNECT_VIGEMCLIENT_WAIT_TIME, this, SLOT(reconnectViGEmClient()));
    }
#endif
}

#if 0
void QKeyMapper::on_uninstallViGEmBusButton_clicked()
{
#ifdef VIGEM_CLIENT_SUPPORT

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "Uninstall ViGEm Bus.";
#endif

    QKeyMapper_Worker::ViGEmClient_Remove();
    QKeyMapper_Worker::ViGEmClient_Disconnect();
    QKeyMapper_Worker::ViGEmClient_Free();

    emit updateViGEmBusStatus_Signal();
    ui->enableVirtualJoystickCheckBox->setCheckState(Qt::Unchecked);
    ui->enableVirtualJoystickCheckBox->setEnabled(false);

    (void)uninstallViGEmBusDriver();
#endif
}
#endif

void QKeyMapper::on_soundEffectCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[SoundEffect] Play Sound Effect state changed ->" << (Qt::CheckState)state;
#endif

    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);

    if (Qt::Checked == state) {
        settingFile.setValue(PLAY_SOUNDEFFECT , true);
    }
    else {
        settingFile.setValue(PLAY_SOUNDEFFECT , false);
    }
}
