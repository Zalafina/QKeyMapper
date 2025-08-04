#include "qkeymapper.h"
#include "ui_qkeymapper.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;
using namespace Gdiplus;

QKeyMapper *QKeyMapper::m_instance = Q_NULLPTR;
QString QKeyMapper::DEFAULT_TITLE = QString("Forza: Horizon 4");

bool QKeyMapper::s_isDestructing = false;
HWINEVENTHOOK QKeyMapper::s_WinEventHook = Q_NULLPTR;
int QKeyMapper::s_GlobalSettingAutoStart = 0;
#ifdef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
uint QKeyMapper::s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
#endif
HWND QKeyMapper::s_CurrentMappingHWND = NULL;
QList<MAP_PROCESSINFO> QKeyMapper::static_ProcessInfoList = QList<MAP_PROCESSINFO>();
QList<HWND> QKeyMapper::s_hWndList;
QList<HWND> QKeyMapper::s_last_HWNDList;
QList<KeyMappingTab_Info> QKeyMapper::s_KeyMappingTabInfoList;
int QKeyMapper::s_KeyMappingTabWidgetCurrentIndex = 0;
int QKeyMapper::s_KeyMappingTabWidgetLastIndex = 0;
// QList<MAP_KEYDATA> QKeyMapper::KeyMappingDataList = QList<MAP_KEYDATA>();
QList<MAP_KEYDATA> *QKeyMapper::KeyMappingDataList = Q_NULLPTR;
QList<MAP_KEYDATA> *QKeyMapper::lastKeyMappingDataList = Q_NULLPTR;
// QList<MAP_KEYDATA> QKeyMapper::KeyMappingDataListGlobal = QList<MAP_KEYDATA>();
QList<MousePoint_Info> QKeyMapper::ScreenMousePointsList = QList<MousePoint_Info>();
QList<MousePoint_Info> QKeyMapper::WindowMousePointsList = QList<MousePoint_Info>();
// QHash<QString, QHotkey*> QKeyMapper::ShortcutsMap = QHash<QString, QHotkey*>();
QString QKeyMapper::s_WindowSwitchKeyString = DISPLAYSWITCH_KEY_DEFAULT;
QString QKeyMapper::s_MappingStartKeyString = MAPPINGSWITCH_KEY_DEFAULT;
QString QKeyMapper::s_MappingStopKeyString = MAPPINGSWITCH_KEY_DEFAULT;
QHash<QString, QList<int>> QKeyMapper::s_MappingTableTabHotkeyMap;
qreal QKeyMapper::s_UI_scale_value = 1.0;
QList<MAP_KEYDATA> QKeyMapper::s_CopiedMappingData;
QHash<int, QStringList> QKeyMapper::s_OriginalKeysCategoryMap;
QHash<int, QStringList> QKeyMapper::s_MappingKeysCategoryMap;
QIcon QKeyMapper::s_Icon_Blank;

QKeyMapper::QKeyMapper(QWidget *parent) :
    QDialog(parent),
    m_UI_Scale(UI_SCALE_NORMAL),
    m_KeyMapStatus(KEYMAP_IDLE),
    ui(new Ui::QKeyMapper),
    m_LastWindowPosition(INITIAL_WINDOW_POSITION, INITIAL_WINDOW_POSITION),
#ifdef CYCLECHECKTIMER_ENABLED
    m_CycleCheckTimer(this),
#endif
#ifndef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
    m_CheckGlobalSettingSwitchTimer(this),
#endif
    m_ProcessInfoTableRefreshTimer(this),
    m_MapProcessInfo(),
    m_SysTrayIcon(Q_NULLPTR),
    m_SysTrayIconMenu(Q_NULLPTR),
    m_TrayIconMenu_ShowHideAction(Q_NULLPTR),
    m_TrayIconMenu_QuitAction(Q_NULLPTR),
    m_PopupMessageLabel(Q_NULLPTR),
    m_PopupMessageAnimation(Q_NULLPTR),
#ifdef USE_SAOFONT
    m_SAO_FontFamilyID(-1),
    m_SAO_FontName(),
#endif
    m_KeyMappingTabWidget(Q_NULLPTR),
    m_KeyMappingDataTable(Q_NULLPTR),
    m_ProcessInfoTableDelegate(Q_NULLPTR),
    // m_KeyMappingDataTableDelegate(Q_NULLPTR),
    m_orikeyComboBox(new KeyListComboBox(this)),
    m_mapkeyComboBox(new KeyListComboBox(this)),
    m_SelectColorDialog(Q_NULLPTR),
    m_GamepadInfoMap(),
    m_SettingSelectListWithoutDescription(),
    // m_windowswitchKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    // m_mappingswitchKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    // m_originalKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    // m_HotKey_ShowHide(new QHotkey(this)),
    // m_HotKey_StartStop(new QHotkey(this)),
    loadSetting_flag(false),
    m_qt_Translator(new QTranslator(this)),
#ifdef USE_QTRANSLATOR
    m_custom_Translator(new QTranslator(this)),
#endif
    m_TransParentHandle(NULL),
    m_TransParentWindowInitialX(0),
    m_TransParentWindowInitialY(0),
    m_TransParentWindowInitialWidth(0),
    m_TransParentWindowInitialHeight(0),
    m_CrosshairHandle(NULL),
    m_CrosshairWindowInitialX(0),
    m_CrosshairWindowInitialY(0),
    m_CrosshairWindowInitialWidth(0),
    m_CrosshairWindowInitialHeight(0),
    m_GdiplusToken(NULL),
    m_deviceListWindow(Q_NULLPTR),
    m_Gyro2MouseOptionDialog(Q_NULLPTR),
    m_TrayIconSelectDialog(Q_NULLPTR),
    m_ItemSetupDialog(Q_NULLPTR),
    m_TableSetupDialog(Q_NULLPTR),
    m_PopupNotification(Q_NULLPTR),
    m_FloatingIconWindow(Q_NULLPTR)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("QKeyMapper() -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

    initBlankIconForComboBox();
    InitializeGDIPlus();
    m_TransParentHandle = createTransparentWindow();
    m_CrosshairHandle = createCrosshairWindow();

    m_instance = this;
    ui->setupUi(this);

    // Explicitly create the native MainWindow HWND at startup.
    // This ensures that even if the application is minimized to system tray at startup,
    // and show() is never called, winId() will still return a valid HWND handle.
    createWinId();
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QKeyMapper()]" << "Mainwindow HWND ID ->" << reinterpret_cast<HWND>(winId());
#endif

    QStyle* windowsStyle = QStyleFactory::create("windows");
    QStyle* fusionStyle = QStyleFactory::create("Fusion");

    ui->settingTabWidget->setStyle(windowsStyle);
    ui->pushLevelSlider->setStyle(windowsStyle);

    // Iterate through all child widgets of settingTabWidget and set their style to Fusion.
    for (int tabindex = 0; tabindex < ui->settingTabWidget->count(); ++tabindex) {
        QWidget *tabPage = ui->settingTabWidget->widget(tabindex);
        tabPage->setStyle(fusionStyle);
    }
    ui->settingTabWidget->setCurrentIndex(ui->settingTabWidget->indexOf(ui->windowinfo));
    // ui->virtualgamepadGroupBox->setStyle(defaultStyle);
    // ui->multiInputGroupBox->setStyle(defaultStyle);

    ui->gamepadSelectComboBox->view()->installEventFilter(this);

#ifdef QT_DEBUG
    ui->pointDisplayLabel->setText("X:1100, Y:1200");
#endif

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

    initKeysCategoryMap();
    initQSimpleUpdater();
    extractSoundFiles();
    initAddKeyComboBoxes();
    initWindowInfoMatchComboBoxes();
    initWindowSwitchKeyLineEdit();
    initMappingSwitchKeyLineEdit();
    // initOriginalKeySeqEdit();
    initCombinationKeyLineEdit();
    initInputDeviceSelectComboBoxes();
    initCategoryFilterControls();
    initPopupMessage();
    initPushLevelSlider();

    QString fileDescription = getExeFileDescription();
    QString platformString = getPlatformString();
    QString title = QString("%1 %2").arg(fileDescription, platformString);
    setWindowTitle(title);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "QKeyMapper() -> Set WindowTitle :" << title;
    qDebug() << "QKeyMapper() -> ProductVersion :" << getExeProductVersion();
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

    int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    HDC hdc = GetDC(NULL);
    int width = GetDeviceCaps(hdc, DESKTOPHORZRES);
    double dWidth = static_cast<double>(width);
    double dScreenWidth = static_cast<double>(nScreenWidth);
    s_UI_scale_value = dWidth / dScreenWidth;
    ReleaseDC(NULL, hdc);

#ifdef USE_SAOFONT
    if ((m_SAO_FontFamilyID != -1)
            && (false == m_SAO_FontName.isEmpty())){
        setControlCustomFont(m_SAO_FontName);
    }
#endif

    // Enable WTS session notifications
    WTSRegisterSessionNotification(reinterpret_cast<HWND>(winId()), NOTIFY_FOR_THIS_SESSION);

#ifndef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
    m_CheckGlobalSettingSwitchTimer.setInterval(CHECK_GLOBALSETTING_SWITCH_TIMEOUT);
    m_CheckGlobalSettingSwitchTimer.setSingleShot(true);
#endif

    // set QTableWidget selected background-color
    setStyleSheet("QTableWidget::item:selected { background-color: rgb(190, 220, 255) }");

    ui->iconLabel->setStyle(windowsStyle);
    ui->pointDisplayLabel->setStyle(windowsStyle);
    // setMapProcessInfo(QString(DEFAULT_NAME), QString(DEFAULT_TITLE), QString(), QString(), QIcon(":/DefaultIcon.ico"));
    // ui->processCheckBox->setChecked(true);
    // ui->titleCheckBox->setChecked(true);

    // ui->moveupButton->setFont(QFont("SimSun", 14));
    // ui->movedownButton->setFont(QFont("SimSun", 16));

    ui->oriList_SelectKeyboardButton->setStyle(windowsStyle);
    ui->oriList_SelectMouseButton->setStyle(windowsStyle);
    ui->oriList_SelectGamepadButton->setStyle(windowsStyle);
    ui->oriList_SelectFunctionButton->setStyle(windowsStyle);
    ui->mapList_SelectKeyboardButton->setStyle(windowsStyle);
    ui->mapList_SelectMouseButton->setStyle(windowsStyle);
    ui->mapList_SelectGamepadButton->setStyle(windowsStyle);
    ui->mapList_SelectFunctionButton->setStyle(windowsStyle);
    ui->oriList_SelectKeyboardButton->setIcon(QIcon(":/keyboard.png"));
    ui->oriList_SelectMouseButton->setIcon(QIcon(":/mouse.png"));
    ui->oriList_SelectGamepadButton->setIcon(QIcon(":/gamepad.png"));
    ui->oriList_SelectFunctionButton->setIcon(QIcon(":/function.png"));
    ui->mapList_SelectKeyboardButton->setIcon(QIcon(":/keyboard.png"));
    ui->mapList_SelectMouseButton->setIcon(QIcon(":/mouse.png"));
    ui->mapList_SelectGamepadButton->setIcon(QIcon(":/gamepad.png"));
    ui->mapList_SelectFunctionButton->setIcon(QIcon(":/function.png"));
    ui->oriList_SelectKeyboardButton->setChecked(true);
    ui->oriList_SelectMouseButton->setChecked(true);
    ui->oriList_SelectGamepadButton->setChecked(true);
    ui->oriList_SelectFunctionButton->setChecked(true);
    ui->mapList_SelectKeyboardButton->setChecked(true);
    ui->mapList_SelectMouseButton->setChecked(true);
    ui->mapList_SelectGamepadButton->setChecked(true);
    ui->mapList_SelectFunctionButton->setChecked(true);

    // Initialize category button state (default: unchecked/hidden)
    ui->showCategoryButton->setChecked(false);

    initProcessInfoTable();
    // ui->processCheckBox->setFocusPolicy(Qt::NoFocus);
    // ui->titleCheckBox->setFocusPolicy(Qt::NoFocus);
    ui->settingNameLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->processLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->windowTitleLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->descriptionLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->sendTextLineEdit->setFocusPolicy(Qt::ClickFocus);

    ui->dataPortSpinBox->setRange(DATA_PORT_MIN, DATA_PORT_MAX);
    ui->brakeThresholdDoubleSpinBox->setDecimals(GRIP_THRESHOLD_DECIMALS);
    ui->brakeThresholdDoubleSpinBox->setRange(GRIP_THRESHOLD_BRAKE_MIN, GRIP_THRESHOLD_BRAKE_MAX);
    ui->accelThresholdDoubleSpinBox->setDecimals(GRIP_THRESHOLD_DECIMALS);
    ui->accelThresholdDoubleSpinBox->setRange(GRIP_THRESHOLD_ACCEL_MIN, GRIP_THRESHOLD_ACCEL_MAX);
    ui->waitTimeSpinBox->setRange(MAPPING_WAITTIME_MIN, MAPPING_WAITTIME_MAX);
    ui->waitTimeSpinBox->setRange(MAPPING_WAITTIME_MIN, MAPPING_WAITTIME_MAX);
    ui->pressTimeSpinBox->setRange(PRESSTIME_MIN, PRESSTIME_MAX);
    // ui->burstpressSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    // ui->burstreleaseSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    ui->mouseXSpeedSpinBox->setRange(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX);
    ui->mouseYSpeedSpinBox->setRange(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX);

    ui->Gyro2MouseXSpeedSpinBox->setRange(GYRO2MOUSE_SPEED_MIN, GYRO2MOUSE_SPEED_MAX);
    ui->Gyro2MouseYSpeedSpinBox->setRange(GYRO2MOUSE_SPEED_MIN, GYRO2MOUSE_SPEED_MAX);
    ui->Gyro2MouseXSpeedSpinBox->setSingleStep(GYRO2MOUSE_SPEED_SINGLESTEP);
    ui->Gyro2MouseYSpeedSpinBox->setSingleStep(GYRO2MOUSE_SPEED_SINGLESTEP);
    ui->Gyro2MouseXSpeedSpinBox->setValue(GYRO2MOUSE_SPEED_DEFAULT);
    ui->Gyro2MouseYSpeedSpinBox->setValue(GYRO2MOUSE_SPEED_DEFAULT);

    ui->Gyro2MouseMinXSensSpinBox->setRange(GYRO2MOUSE_GYRO_SENS_MIN, GYRO2MOUSE_MAX_GYRO_SENS_DEFAULT);
    ui->Gyro2MouseMinYSensSpinBox->setRange(GYRO2MOUSE_GYRO_SENS_MIN, GYRO2MOUSE_MAX_GYRO_SENS_DEFAULT);
    ui->Gyro2MouseMaxXSensSpinBox->setRange(GYRO2MOUSE_MIN_GYRO_SENS_DEFAULT, GYRO2MOUSE_GYRO_SENS_MAX);
    ui->Gyro2MouseMaxYSensSpinBox->setRange(GYRO2MOUSE_MIN_GYRO_SENS_DEFAULT, GYRO2MOUSE_GYRO_SENS_MAX);
    ui->Gyro2MouseMinXSensSpinBox->setSingleStep(GYRO2MOUSE_GYRO_SENS_SINGLESTEP);
    ui->Gyro2MouseMinYSensSpinBox->setSingleStep(GYRO2MOUSE_GYRO_SENS_SINGLESTEP);
    ui->Gyro2MouseMaxXSensSpinBox->setSingleStep(GYRO2MOUSE_GYRO_SENS_SINGLESTEP);
    ui->Gyro2MouseMaxYSensSpinBox->setSingleStep(GYRO2MOUSE_GYRO_SENS_SINGLESTEP);
    ui->Gyro2MouseMinXSensSpinBox->setValue(GYRO2MOUSE_MIN_GYRO_SENS_DEFAULT);
    ui->Gyro2MouseMinYSensSpinBox->setValue(GYRO2MOUSE_MIN_GYRO_SENS_DEFAULT);
    ui->Gyro2MouseMaxXSensSpinBox->setValue(GYRO2MOUSE_MAX_GYRO_SENS_DEFAULT);
    ui->Gyro2MouseMaxYSensSpinBox->setValue(GYRO2MOUSE_MAX_GYRO_SENS_DEFAULT);

    ui->Gyro2MouseMinThresholdSpinBox->setRange(GYRO2MOUSE_GYRO_THRESHOLD_MIN, GYRO2MOUSE_MAX_GYRO_THRESHOLD_DEFAULT);
    ui->Gyro2MouseMaxThresholdSpinBox->setRange(GYRO2MOUSE_MIN_GYRO_THRESHOLD_DEFAULT, GYRO2MOUSE_GYRO_THRESHOLD_MAX);
    ui->Gyro2MouseMinThresholdSpinBox->setSingleStep(GYRO2MOUSE_GYRO_THRESHOLD_SINGLESTEP);
    ui->Gyro2MouseMaxThresholdSpinBox->setSingleStep(GYRO2MOUSE_GYRO_THRESHOLD_SINGLESTEP);
    ui->Gyro2MouseMinThresholdSpinBox->setValue(GYRO2MOUSE_MIN_GYRO_THRESHOLD_DEFAULT);
    ui->Gyro2MouseMaxThresholdSpinBox->setValue(GYRO2MOUSE_MAX_GYRO_THRESHOLD_DEFAULT);

    ui->dataPortSpinBox->setValue(DATA_PORT_DEFAULT);
    ui->brakeThresholdDoubleSpinBox->setValue(GRIP_THRESHOLD_BRAKE_DEFAULT);
    ui->brakeThresholdDoubleSpinBox->setSingleStep(GRIP_THRESHOLD_SINGLE_STEP);
    ui->accelThresholdDoubleSpinBox->setValue(GRIP_THRESHOLD_ACCEL_DEFAULT);
    ui->accelThresholdDoubleSpinBox->setSingleStep(GRIP_THRESHOLD_SINGLE_STEP);

    initSysTrayIcon();

#ifdef VIGEM_CLIENT_SUPPORT
    ui->vJoyXSensSpinBox->setRange(VIRTUAL_JOYSTICK_SENSITIVITY_MIN, VIRTUAL_JOYSTICK_SENSITIVITY_MAX);
    ui->vJoyYSensSpinBox->setRange(VIRTUAL_JOYSTICK_SENSITIVITY_MIN, VIRTUAL_JOYSTICK_SENSITIVITY_MAX);
    ui->vJoyXSensSpinBox->setValue(VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT);
    ui->vJoyYSensSpinBox->setValue(VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT);
    ui->vJoyRecenterSpinBox->setRange(MOUSE2VJOY_RECENTER_TIMEOUT_MIN, MOUSE2VJOY_RECENTER_TIMEOUT_MAX);
    ui->vJoyRecenterSpinBox->setValue(MOUSE2VJOY_RECENTER_TIMEOUT_DEFAULT);
    ui->virtualGamepadNumberSpinBox->setRange(VIRTUAL_GAMEPAD_NUMBER_MIN, VIRTUAL_GAMEPAD_NUMBER_MAX);

    bool isWin10Above = false;
    QOperatingSystemVersion osVersion = QOperatingSystemVersion::current();
    if (osVersion >= QOperatingSystemVersion::Windows10) {
        isWin10Above = true;
    }
    else {
        isWin10Above = false;
    }

    ui->vJoyXSensSpinBox->setEnabled(false);
    ui->vJoyYSensSpinBox->setEnabled(false);
    ui->vJoyXSensLabel->setEnabled(false);
    ui->vJoyYSensLabel->setEnabled(false);
    ui->vJoyRecenterLabel->setEnabled(false);
    ui->vJoyRecenterSpinBox->setEnabled(false);
    ui->lockCursorCheckBox->setEnabled(false);
    ui->directModeCheckBox->setEnabled(false);
    ui->enableVirtualJoystickCheckBox->setEnabled(false);
    ui->virtualGamepadNumberSpinBox->setEnabled(false);
    ui->virtualGamepadListComboBox->setEnabled(false);

    int retval_alloc = QKeyMapper_Worker::ViGEmClient_Alloc();
    int retval_connect = QKeyMapper_Worker::ViGEmClient_Connect();
    Q_UNUSED(retval_alloc);
    Q_UNUSED(retval_connect);

    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS != QKeyMapper_Worker::ViGEmClient_getConnectState()) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("ViGEmClient initialize failed!!! -> retval_alloc(%d), retval_connect(%d)", retval_alloc, retval_connect);
#endif
    }

    if (!isWin10Above) {
        ui->installViGEmBusButton->setEnabled(false);
        ui->installViGEmBusButton->setVisible(false);
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

    ui->notificationComboBox->clear();
    QStringList positoin_list = QStringList() \
            << tr("None")
            << tr("Top Left")
            << tr("Top Center")
            << tr("Top Right")
            << tr("Bottom Left")
            << tr("Bottom Center")
            << tr("Bottom Right")
            ;
    ui->notificationComboBox->addItems(positoin_list);
    ui->notificationComboBox->setCurrentIndex(NOTIFICATION_POSITION_TOP_RIGHT);

    // m_windowswitchKeySeqEdit->setDefaultKeySequence(DISPLAYSWITCH_KEY_DEFAULT);
    // m_mappingswitchKeySeqEdit->setDefaultKeySequence(MAPPINGSWITCH_KEY_DEFAULT);
    // m_originalKeySeqEdit->setDefaultKeySequence(ORIGINAL_KEYSEQ_DEFAULT);
    ui->windowswitchkeyLineEdit->setText(DISPLAYSWITCH_KEY_DEFAULT);
    ui->mappingStartKeyLineEdit->setText(MAPPINGSWITCH_KEY_DEFAULT);
    ui->mappingStopKeyLineEdit->setText(MAPPINGSWITCH_KEY_DEFAULT);
    initSelectColorDialog();
    initKeyMappingTabWidget();
    m_ItemSetupDialog = new QItemSetupDialog(this);
    m_TableSetupDialog = new QTableSetupDialog(this);
    m_Gyro2MouseOptionDialog = new QGyro2MouseOptionDialog(this);
    m_TrayIconSelectDialog = new QTrayIconSelectDialog(this);
    m_NotificationSetupDialog = new QNotificationSetupDialog(this);
    m_FloatingIconWindow = new QFloatingIconWindow(Q_NULLPTR);
    loadSetting_flag = true;
    QString loadresult = loadKeyMapSetting(QString());
    ui->settingNameLineEdit->setText(loadresult);
    Q_UNUSED(loadresult);
    loadSetting_flag = false;

    initGyro2MouseSpinBoxes();

    m_PopupNotification = new QPopupNotification(Q_NULLPTR);
    m_deviceListWindow = new QInputDeviceListWindow(this);
    // m_ItemSetupDialog->setWindowFlags(Qt::Popup);

    Interception_Worker::syncDisabledKeyboardList();
    Interception_Worker::syncDisabledMouseList();

    updateSysTrayIconMenuText();
    reloadUILanguage();
    resetFontSize();
    ui->settingselectComboBox->setToolTip(ui->settingselectComboBox->currentText());

    updateSystemTrayDisplay();
    m_SysTrayIcon->show();

    updateMultiInputStatus();
#ifdef VIGEM_CLIENT_SUPPORT
    updateViGEmBusStatus();
    updateVirtualGamepadListDisplay();
#endif

    QObject::connect(m_SysTrayIcon, &QSystemTrayIcon::activated, this, &QKeyMapper::SystrayIconActivated);
#ifdef CYCLECHECKTIMER_ENABLED
    QObject::connect(&m_CycleCheckTimer, &QTimer::timeout, this, &QKeyMapper::matchForegroundWindow);
#endif
#ifndef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
    QObject::connect(&m_CheckGlobalSettingSwitchTimer, &QTimer::timeout, this, &QKeyMapper::checkGlobalSettingSwitchTimeout);
#endif
    QObject::connect(&m_ProcessInfoTableRefreshTimer, &QTimer::timeout, this, &QKeyMapper::cycleRefreshProcessInfoTableProc);

    QObject::connect(m_KeyMappingTabWidget, &QTabWidget::currentChanged, this, &QKeyMapper::keyMappingTabWidgetCurrentChanged);
    QObject::connect(m_KeyMappingTabWidget, &QTabWidget::tabBarDoubleClicked, this, &QKeyMapper::onKeyMappingTabWidgetTabBarDoubleClicked);
    QObject::connect(m_KeyMappingTabWidget, &KeyMappingTabWidget::tabOrderChanged, this, &QKeyMapper::onKeyMappingTabWidgetTabOrderChanged);
    updateKeyMappingDataTableConnection();
    QObject::connect(this, &QKeyMapper::keyMappingTableDragDropMove_Signal, this, &QKeyMapper::keyMappingTableDragDropMove);
    QObject::connect(this, &QKeyMapper::setupDialogClosed_Signal, this, &QKeyMapper::setupDialogClosed);
    QObject::connect(this, &QKeyMapper::showPopupMessage_Signal, this, &QKeyMapper::showPopupMessage);

    // QObject::connect(m_windowswitchKeySeqEdit, &KeySequenceEditOnlyOne::keySeqEditChanged_Signal, this, &QKeyMapper::onWindowSwitchKeySequenceChanged);
    // QObject::connect(m_windowswitchKeySeqEdit, &KeySequenceEditOnlyOne::editingFinished, this, &QKeyMapper::onWindowSwitchKeySequenceEditingFinished);
    // QObject::connect(m_mappingswitchKeySeqEdit, &KeySequenceEditOnlyOne::keySeqEditChanged_Signal, this, &QKeyMapper::onMappingSwitchKeySequenceChanged);
    // QObject::connect(m_mappingswitchKeySeqEdit, &KeySequenceEditOnlyOne::editingFinished, this, &QKeyMapper::onMappingSwitchKeySequenceEditingFinished);
    // QObject::connect(m_originalKeySeqEdit, &KeySequenceEditOnlyOne::keySeqEditChanged_Signal, this, &QKeyMapper::onOriginalKeySequenceChanged);
    // QObject::connect(m_originalKeySeqEdit, &KeySequenceEditOnlyOne::editingFinished, this, &QKeyMapper::onOriginalKeySequenceEditingFinished);

    QObject::connect(this, &QKeyMapper::HotKeyDisplaySwitchActivated_Signal, this, &QKeyMapper::HotKeyDisplaySwitchActivated, Qt::QueuedConnection);
    // QObject::connect(this, &QKeyMapper::HotKeyMappingSwitchActivated_Signal, this, &QKeyMapper::HotKeyMappingSwitchActivated, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::HotKeyMappingStart_Signal, this, &QKeyMapper::HotKeyMappingStart, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::HotKeyMappingStop_Signal, this, &QKeyMapper::HotKeyMappingStop, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::HotKeyMappingTableSwitchTab_Signal, this, &QKeyMapper::HotKeyMappingTableSwitchTab, Qt::QueuedConnection);

    QObject::connect(this, &QKeyMapper::checkOSVersionMatched_Signal, this, &QKeyMapper::checkOSVersionMatched, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::checkFilterKeysEnabled_Signal, this, &QKeyMapper::checkFilterKeysEnabled, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::updateLockStatus_Signal, this, &QKeyMapper::updateLockStatusDisplay, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::updateMousePointLabelDisplay_Signal, this, &QKeyMapper::updateMousePointLabelDisplay, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::showMousePoints_Signal, this, &QKeyMapper::showMousePoints, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::showCarOrdinal_Signal, this, &QKeyMapper::showCarOrdinal, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::showCrosshairStart_Signal, this, &QKeyMapper::showCrosshairStart, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::showCrosshairStop_Signal, this, &QKeyMapper::showCrosshairStop, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::updateViGEmBusStatus_Signal, this, &QKeyMapper::updateViGEmBusStatus, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::updateVirtualGamepadListDisplay_Signal, this, &QKeyMapper::updateVirtualGamepadListDisplay);
    QObject::connect(m_orikeyComboBox, &KeyListComboBox::currentTextChanged, this, &QKeyMapper::OrikeyComboBox_currentTextChangedSlot);
    QObject::connect(m_mapkeyComboBox, &KeyListComboBox::currentTextChanged, this, &QKeyMapper::MapkeyComboBox_currentTextChangedSlot);
    QObject::connect(this, &QKeyMapper::updateMultiInputStatus_Signal, this, &QKeyMapper::updateMultiInputStatus);
    QObject::connect(this, &QKeyMapper::updateInputDeviceSelectComboBoxes_Signal, this, &QKeyMapper::updateInputDeviceSelectComboBoxes);
    QObject::connect(this, &QKeyMapper::updateGamepadSelectComboBox_Signal, this, &QKeyMapper::updateGamepadSelectComboBox, Qt::QueuedConnection);

    updateHWNDListProc();
    refreshProcessInfoTable();
    if (!ui->processListButton->isChecked()) {
        hideProcessList();
        setKeyMappingTabWidgetWideMode();
    }
    refreshAllKeyMappingTagWidget();
    // resizeAllKeyMappingTabWidgetColumnWidth();
#ifdef QT_NO_DEBUG
    m_ProcessInfoTableRefreshTimer.start(CYCLE_REFRESH_PROCESSINFOTABLE_TIMEOUT);
#endif

//    if (false == loadresult){
//        QMessageBox::warning(this, PROGRAM_NAME, tr("Load invalid keymapdata from ini file.\nReset to default values."));
//    }

    // initHotKeySequence();
}

QKeyMapper::~QKeyMapper()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "~QKeyMapper() called.";
#endif
    s_isDestructing = true;

    stopWinEventHook();
    // Unregister WTS session notifications
    WTSUnRegisterSessionNotification(reinterpret_cast<HWND>(winId()));

    destroyTransparentWindow(m_TransParentHandle);
    m_TransParentHandle = NULL;
    destroyCrosshairWindow(m_CrosshairHandle);
    m_CrosshairHandle = NULL;
    ShutdownGDIPlus();

    delete ui;

    if (m_ProcessInfoTableDelegate != Q_NULLPTR) {
        delete m_ProcessInfoTableDelegate;
        m_ProcessInfoTableDelegate = Q_NULLPTR;
    }

    // delete m_KeyMappingDataTableDelegate;
    // m_KeyMappingDataTableDelegate = Q_NULLPTR;

    if (m_PopupNotification != Q_NULLPTR) {
        delete m_PopupNotification;
        m_PopupNotification = Q_NULLPTR;
    }

    if (m_FloatingIconWindow != Q_NULLPTR) {
        m_FloatingIconWindow->hideFloatingWindow();
        delete m_FloatingIconWindow;
        m_FloatingIconWindow = Q_NULLPTR;
    }

    exitDeleteKeyMappingTabWidget();
}

void QKeyMapper::WindowStateChangedProc(void)
{
    if (true == isMinimized()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[WindowStateChangedProc]" << "QKeyMapper::WindowStateChangedProc() -> Window Minimized!";
#endif
        closeSelectColorDialog();
        closeTableSetupDialog();
        closeFloatingWindowSetupDialog();
        closeItemSetupDialog();
        closeCrosshairSetupDialog();
        closeGyro2MouseAdvancedSettingDialog();
        closeTrayIconSelectDialog();
        closeNotificationSetupDialog();
        // hide();
    }
}

#if 0
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
        Q_UNUSED(keymapstatusEnum);

        if ((false == ui->processCheckBox->isChecked())
            && (false == ui->titleCheckBox->isChecked())){
            checkresult = 2;
        }
        else if (true == getSendToSameTitleWindowsStatus()
            && true == ui->processCheckBox->isChecked()
            && true == ui->titleCheckBox->isChecked()){
            if ((m_MapProcessInfo.FileName.isEmpty() == false)
                && (m_MapProcessInfo.WindowTitle.isEmpty() == false)){
                checkresult = 3;
            }
        }

        int resultLength = GetWindowText(hwnd, titleBuffer, MAX_PATH);
        if (resultLength){
            windowTitle = QString::fromWCharArray(titleBuffer);
            QString ProcessPath;
            getProcessInfoFromHWND( hwnd, ProcessPath);

            if (ProcessPath.isEmpty()) {
                bool adjust_priv;
                adjust_priv = EnablePrivilege(SE_DEBUG_NAME);
                if (adjust_priv) {
                    getProcessInfoFromHWND( hwnd, ProcessPath);
                }
                else {
                    qDebug() << "[cycleCheckProcessProc]" << "getProcessInfoFromHWND EnablePrivilege(SE_DEBUG_NAME) Failed with ->" << GetLastError();
                }
                adjust_priv = DisablePrivilege(SE_DEBUG_NAME);

                if (!adjust_priv) {
                    qDebug() << "[doFunctionMappingProc]" << "getProcessInfoFromHWND DisablePrivilege(SE_DEBUG_NAME) Failed with ->" << GetLastError();
                }

#ifdef DEBUG_LOGOUT_ON
                if (ProcessPath.isEmpty()) {
                    qDebug().nospace().noquote() << "[cycleCheckProcessProc] " << "EnablePrivilege(SE_DEBUG_NAME) getProcessInfoFromHWND Failed!";
                }
                else {
                    qDebug().nospace().noquote() << "[cycleCheckProcessProc] " << "EnablePrivilege(SE_DEBUG_NAME) getProcessInfoFromHWND Success -> " << ProcessPath;
                }
#endif
            }

            if (ProcessPath.isEmpty()) {
                ProcessPath = PROCESS_UNKNOWN;
            }

            if (false == windowTitle.isEmpty()){
                QFileInfo fileinfo(ProcessPath);
                filename = fileinfo.fileName();
            }

            if (!filename.isEmpty()) {
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
                        // QString curSettingSelectStr = ui->settingselectComboBox->currentText();
                        QString curSettingSelectStr;
                        int curSettingSelectIndex = ui->settingselectComboBox->currentIndex();
                        if (0 < curSettingSelectIndex && curSettingSelectIndex < m_SettingSelectListWithoutDescription.size()) {
                            curSettingSelectStr = m_SettingSelectListWithoutDescription.at(curSettingSelectIndex);
                        }
                        else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().noquote().nospace() << "[cycleCheckProcessProc]" << "Need to load setting select index is invalid("<< curSettingSelectIndex << "), m_SettingSelectListWithoutDescription ->" << m_SettingSelectListWithoutDescription;
#endif
                        }
                        if (curSettingSelectStr != loadSettingSelectStr) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace().noquote() << "[cycleCheckProcessProc] "<< "Setting Check Matched! Load setting -> [" << loadSettingSelectStr << "]";
#endif
                            loadSetting_flag = true;
                            QString loadresult = loadKeyMapSetting(loadSettingSelectStr);
                            ui->settingNameLineEdit->setText(loadresult);
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

                if ((true == ui->processCheckBox->isChecked())
                    && (true == ui->titleCheckBox->isChecked())){
                    if ((m_MapProcessInfo.FileName == filename)
                        && (m_MapProcessInfo.WindowTitle.isEmpty() == false)
                        && (m_MapProcessInfo.WindowTitle == windowTitle || windowTitle.contains(m_MapProcessInfo.WindowTitle))){
                        checkresult = 1;
                    }
                }
                else if (true == ui->processCheckBox->isChecked()){
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

        bool isVisibleWindow = false;
        bool isExToolWindow = false;
        bool isToolbarWindow = false;

        if (checkresult <= 1) {
            isVisibleWindow = IsWindowVisible(hwnd);
            WINDOWINFO winInfo;
            winInfo.cbSize = sizeof(WINDOWINFO);
            if (GetWindowInfo(hwnd, &winInfo)) {
                if ((winInfo.dwExStyle & WS_EX_TOOLWINDOW) != 0)
                    isExToolWindow = true;
            }

            /* Skip inVisibleWidow & ToolbarWindow >>> */
            if (false == filename.isEmpty()
                && false == windowTitle.isEmpty()
                && true == isVisibleWindow
                && true == isExToolWindow) {
                isToolbarWindow = true;
            }
            else if (true == filename.isEmpty()
                && true == windowTitle.isEmpty()
                && false == isVisibleWindow
                && true == isExToolWindow) {
                isToolbarWindow = true;
            }
            /* Skip inVisibleWidow & ToolbarWindow <<< */
        }

        bool GlobalMappingFlag = false;
        if (GLOBALSETTING_INDEX == ui->settingselectComboBox->currentIndex()
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

                        setKeyHook(NULL);
                        m_KeyMapStatus = KEYMAP_MAPPING_GLOBAL;
                        mappingStartNotification();
#ifdef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
                        s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
#else
                        m_CheckGlobalSettingSwitchTimer.stop();
#endif
                        emit updateLockStatus_Signal();
                    }
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "[cycleCheckProcessProc]" << " checkresult = " << checkresult << "," << " KeyMapStatus need to change [" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << "] -> [" << keymapstatusEnum.valueToKey(KEYMAP_MAPPING_MATCHED) << "]" << ", ForegroundWindow: " << windowTitle << "(" << filename << ")";
                    qDebug().nospace() << "[cycleCheckProcessProc]" << " NameChecked = " << ui->processCheckBox->isChecked() << "," << " TitleChecked = " << ui->titleCheckBox->isChecked();
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
                        /* Add for "PixPin.exe -> PixPin" Bug Fix >>> */
                        else if (filename == "PixPin.exe"
                            && windowTitle == "PixPin"
                            && isVisibleWindow == true
                            && isExToolWindow == true) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace() << "[cycleCheckProcessProc]" << "[BugFix] Do not skip ToolbarWindow \"PixPin.exe -> PixPin\" of KeyMapStatus(KEYMAP_MAPPING_MATCHED)";
#endif
                        }
                        /* Add for "PixPin.exe -> PixPin" Bug Fix <<< */
                        else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace() << "[cycleCheckProcessProc]" << " Skip ToolbarWindow of KeyMapStatus(KEYMAP_MAPPING_MATCHED)";
#endif
                            return;
                        }
                    }

                    playStartSound();
                    if (checkresult > 1) {
                        setKeyHook(NULL);
                    }
                    else {
                        setKeyHook(hwnd);
                    }
                    m_KeyMapStatus = KEYMAP_MAPPING_MATCHED;
                    mappingStartNotification();
#ifdef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
                    s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
#else
                    m_CheckGlobalSettingSwitchTimer.stop();
#endif
                    emit updateLockStatus_Signal();
                }
            }
        }
        else{
            if (KEYMAP_MAPPING_MATCHED == m_KeyMapStatus){
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[cycleCheckProcessProc]" << " checkresult = " << checkresult << "," << " KeyMapStatus need to change [" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << "] -> [" << keymapstatusEnum.valueToKey(KEYMAP_CHECKING) << "]" << ", ForegroundWindow: " << windowTitle << "(" << filename << ")";
                qDebug().nospace() << "[cycleCheckProcessProc]" << " checkresult = " << checkresult << "," << " KeyMapStatus need to change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") " << "ForegroundWindow: " << windowTitle << "(" << filename << ")";
                qDebug().nospace() << "[cycleCheckProcessProc]" << " NameChecked = " << ui->processCheckBox->isChecked() << "," << " TitleChecked = " << ui->titleCheckBox->isChecked();
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
                    /* Add for "PixPin.exe -> PixPin" Bug Fix >>> */
                    else if (filename == "PixPin.exe"
                        && windowTitle == "PixPin"
                        && isVisibleWindow == true
                        && isExToolWindow == true) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace() << "[cycleCheckProcessProc]" << "[BugFix] Do not skip ToolbarWindow \"PixPin.exe -> PixPin\" of KeyMapStatus(KEYMAP_CHECKING)";
#endif
                    }
                    /* Add for "PixPin.exe -> PixPin" Bug Fix <<< */
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
                mappingStopNotification();
#ifdef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
                s_CycleCheckLoopCount = 0;
#else
                m_CheckGlobalSettingSwitchTimer.start();
#endif
                emit updateLockStatus_Signal();
            }
        }
    }
    else{
        //EnumWindows((WNDENUMPROC)QKeyMapper::EnumWindowsProc, 0);
    }

#ifdef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
    if (m_KeyMapStatus == KEYMAP_CHECKING && GLOBAL_MAPPING_START_WAIT == s_CycleCheckLoopCount) {
        if (checkGlobalSettingAutoStart()) {
            loadSetting_flag = true;
            QString loadresult = loadKeyMapSetting(GROUPNAME_GLOBALSETTING);
            ui->settingNameLineEdit->setText(loadresult);
            Q_UNUSED(loadresult);
            loadSetting_flag = false;
        }
    }

    s_CycleCheckLoopCount += 1;
    if (s_CycleCheckLoopCount > CYCLE_CHECK_LOOPCOUNT_MAX) {
        s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
    }
#endif
}
#endif

void QKeyMapper::matchForegroundWindow()
{
    if (KEYMAP_IDLE != m_KeyMapStatus) {
        enum class MatchResult {
            NoMatch = 0,            // NoMatch,              same as checkresult = 0
            ProcessMatched = 1,     // ProcessMatched，      same as checkresult = 1
            IgnoreBothChecks = 2,   // IgnoreBothChecks,     same as checkresult = 2
            SendToSameWindows = 3   // SendToSameWindows,    same as checkresult = 3
        };

        MatchResult matchResult = MatchResult::NoMatch;
        QString windowTitle;
        QString processName;
        QString filename;
        QString ProcessPath;
        HWND hwnd = GetForegroundWindow();
        TCHAR titleBuffer[MAX_PATH];
        memset(titleBuffer, 0x00, sizeof(titleBuffer));
        QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
        Q_UNUSED(keymapstatusEnum);

        int matchProcessIndex = ui->checkProcessComboBox->currentIndex();
        int matchWindowTitleIndex = ui->checkWindowTitleComboBox->currentIndex();
        bool matchProcess = (matchProcessIndex != WINDOWINFO_MATCH_INDEX_IGNORE && !m_MapProcessInfo.FileName.isEmpty());
        bool matchWindowTitle = (matchWindowTitleIndex != WINDOWINFO_MATCH_INDEX_IGNORE && !m_MapProcessInfo.WindowTitle.isEmpty());

        if (!matchProcess && !matchWindowTitle) {
            matchResult = MatchResult::IgnoreBothChecks;
        }
        else if (getSendToSameTitleWindowsStatus()
            && matchWindowTitle){
            matchResult = MatchResult::SendToSameWindows;
        }

        int resultLength = GetWindowText(hwnd, titleBuffer, MAX_PATH);
        if (resultLength){
            windowTitle = QString::fromWCharArray(titleBuffer);
            getProcessInfoFromHWND( hwnd, ProcessPath);

            if (ProcessPath.isEmpty()) {
                bool adjust_priv;
                adjust_priv = EnablePrivilege(SE_DEBUG_NAME);
                if (adjust_priv) {
                    getProcessInfoFromHWND( hwnd, ProcessPath);
                }
                else {
                    qDebug() << "[matchForegroundWindow]" << "getProcessInfoFromHWND EnablePrivilege(SE_DEBUG_NAME) Failed with ->" << GetLastError();
                }
                adjust_priv = DisablePrivilege(SE_DEBUG_NAME);

                if (!adjust_priv) {
                    qDebug() << "[matchForegroundWindow]" << "getProcessInfoFromHWND DisablePrivilege(SE_DEBUG_NAME) Failed with ->" << GetLastError();
                }

#ifdef DEBUG_LOGOUT_ON
                if (ProcessPath.isEmpty()) {
                    qDebug().nospace().noquote() << "[matchForegroundWindow] " << "EnablePrivilege(SE_DEBUG_NAME) getProcessInfoFromHWND Failed!";
                }
                else {
                    qDebug().nospace().noquote() << "[matchForegroundWindow] " << "EnablePrivilege(SE_DEBUG_NAME) getProcessInfoFromHWND Success -> " << ProcessPath;
                }
#endif
            }

            if (ProcessPath.isEmpty()) {
                ProcessPath = PROCESS_UNKNOWN;
            }

            if (false == windowTitle.isEmpty()){
                QFileInfo fileinfo(ProcessPath);
                filename = fileinfo.fileName();
                processName = ProcessPath;
            }

            if (!processName.isEmpty()) {
                QString autoMatchSettingGroup = matchAutoStartSaveSettings(processName, windowTitle);

                if (!autoMatchSettingGroup.isEmpty() && (KEYMAP_CHECKING == m_KeyMapStatus || KEYMAP_MAPPING_GLOBAL == m_KeyMapStatus)) {
                    QString curSettingSelectStr;
                    int curSettingSelectIndex = ui->settingselectComboBox->currentIndex();
                    if (0 < curSettingSelectIndex && curSettingSelectIndex < m_SettingSelectListWithoutDescription.size()) {
                        curSettingSelectStr = m_SettingSelectListWithoutDescription.at(curSettingSelectIndex);
                    }
                    else {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().noquote().nospace() << "[matchForegroundWindow]" << "Need to load setting select index is invalid("<< curSettingSelectIndex << "), m_SettingSelectListWithoutDescription ->" << m_SettingSelectListWithoutDescription;
#endif
                    }
                    if (curSettingSelectStr != autoMatchSettingGroup) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace().noquote() << "[matchForegroundWindow] "<< "Setting Check Matched! Load setting -> [" << autoMatchSettingGroup << "]";
#endif
                        loadSetting_flag = true;
                        QString loadresult = loadKeyMapSetting(autoMatchSettingGroup);
                        ui->settingNameLineEdit->setText(loadresult);
                        Q_UNUSED(loadresult)
                        loadSetting_flag = false;
                    }
                    else {
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[matchForegroundWindow]" << "Current setting select is already the same ->" << curSettingSelectStr;
#endif
                    }
                }

                // Perform matching logic based on ComboBox selection
                bool processMatched = false;
                bool windowTitleMatched = false;

                // Check for process name match
                if (matchProcess) {
                    if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_EQUALS) {
                        processMatched = (m_MapProcessInfo.FileName == processName);
                    }
                    else if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_CONTAINS) {
                        processMatched = processName.contains(m_MapProcessInfo.FileName);
                    }
                    else if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_STARTSWITH) {
                        processMatched = processName.startsWith(m_MapProcessInfo.FileName);
                    }
                    else if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_ENDSWITH) {
                        processMatched = processName.endsWith(m_MapProcessInfo.FileName);
                    }
                }
                else {
                    processMatched = true; // Treat as matched if process name check is ignored
                }

                // Check for window title match
                if (matchWindowTitle) {
                    if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_EQUALS) {
                        windowTitleMatched = (m_MapProcessInfo.WindowTitle == windowTitle);
                    }
                    else if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_CONTAINS) {
                        windowTitleMatched = windowTitle.contains(m_MapProcessInfo.WindowTitle);
                    }
                    else if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_STARTSWITH) {
                        windowTitleMatched = windowTitle.startsWith(m_MapProcessInfo.WindowTitle);
                    }
                    else if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_ENDSWITH) {
                        windowTitleMatched = windowTitle.endsWith(m_MapProcessInfo.WindowTitle);
                    }
                }
                else {
                    windowTitleMatched = true; // Treat as matched if window title check is ignored
                }

                // Set matchResult based on the matching outcome
                if (matchProcess && matchWindowTitle) {
                    // Check both process name and window title
                    if (processMatched && windowTitleMatched) {
                        matchResult = MatchResult::ProcessMatched;
                    }
                }
                else if (matchProcess) {
                    // Check process name only
                    if (processMatched) {
                        matchResult = MatchResult::ProcessMatched;
                    }
                }
                else if (matchWindowTitle) {
                    // Check window title only
                    if (windowTitleMatched) {
                        matchResult = MatchResult::ProcessMatched;
                    }
                }
            }
        }

        bool isVisibleWindow = false;
        bool isExToolWindow = false;
        bool isToolbarWindow = false;

        if (matchResult == MatchResult::NoMatch || matchResult == MatchResult::ProcessMatched) {
            isVisibleWindow = IsWindowVisible(hwnd);
            WINDOWINFO winInfo;
            winInfo.cbSize = sizeof(WINDOWINFO);
            if (GetWindowInfo(hwnd, &winInfo)) {
                if ((winInfo.dwExStyle & WS_EX_TOOLWINDOW) != 0)
                    isExToolWindow = true;
            }

            /* Skip inVisibleWidow & ToolbarWindow >>> */
            if (false == filename.isEmpty()
                && false == windowTitle.isEmpty()
                && true == isVisibleWindow
                && true == isExToolWindow) {
                isToolbarWindow = true;
            }
            else if (true == filename.isEmpty()
                && true == windowTitle.isEmpty()
                && false == isVisibleWindow
                && true == isExToolWindow) {
                isToolbarWindow = true;
            }
            /* Skip inVisibleWidow & ToolbarWindow <<< */
        }

        bool GlobalMappingFlag = false;
        if (GLOBALSETTING_INDEX == ui->settingselectComboBox->currentIndex()
            && matchResult == MatchResult::IgnoreBothChecks) {
            GlobalMappingFlag = true;
        }

        if (matchResult != MatchResult::NoMatch){
            if (KEYMAP_CHECKING == m_KeyMapStatus || KEYMAP_MAPPING_GLOBAL == m_KeyMapStatus){
                if (GlobalMappingFlag) {
                    if (m_KeyMapStatus != KEYMAP_MAPPING_GLOBAL) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace() << "[matchForegroundWindow]" << " GlobalMappingFlag = " << GlobalMappingFlag << "," << " KeyMapStatus need to change [" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << "] -> [" << keymapstatusEnum.valueToKey(KEYMAP_MAPPING_GLOBAL) << "]";
#endif

                        setKeyHook(NULL);
                        m_KeyMapStatus = KEYMAP_MAPPING_GLOBAL;
                        mappingStartNotification();
                        m_CheckGlobalSettingSwitchTimer.stop();
                        emit updateLockStatus_Signal();
                    }
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "[matchForegroundWindow]" << " MatchResult = " << static_cast<int>(matchResult) << "," << " KeyMapStatus need to change [" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << "] -> [" << keymapstatusEnum.valueToKey(KEYMAP_MAPPING_MATCHED) << "]" << ", ForegroundWindow: " << windowTitle << "(" << filename << ")";
                    qDebug().nospace() << "[matchForegroundWindow]" << " MatchProcessIndex = " << matchProcessIndex << "," << " MatchWindowTitleIndex = " << matchWindowTitleIndex;
                    qDebug().nospace() << "[matchForegroundWindow]" << " ProcessInfo.FileName = " << m_MapProcessInfo.FileName << "," << " ProcessInfo.WindowTitle = " << m_MapProcessInfo.WindowTitle;
                    qDebug().nospace() << "[matchForegroundWindow]" << " ProcessPath = " << ProcessPath;
                    qDebug().nospace() << "[matchForegroundWindow]" << " CurrentFileName = " << filename << "," << " CurrentWindowTitle = " << windowTitle;
                    qDebug().nospace() << "[matchForegroundWindow]" << " isVisibleWindow = " << isVisibleWindow << "," << " isExToolWindow =" << isExToolWindow;
                    qDebug().nospace() << "[matchForegroundWindow]" << " isToolbarWindow = " << isToolbarWindow;
#endif
                    if (isToolbarWindow) {
                        /* Add for "explorer.exe -> Program Manager" Bug Fix >>> */
                        if (filename == "explorer.exe"
                            && windowTitle == "Program Manager"
                            && isVisibleWindow == true
                            && isExToolWindow == true) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace() << "[matchForegroundWindow]" << "[BugFix] Do not skip ToolbarWindow \"explorer.exe -> Program Manager\" of KeyMapStatus(KEYMAP_MAPPING_MATCHED)";
#endif
                        }
                        /* Add for "explorer.exe -> Program Manager" Bug Fix <<< */
                        /* Add for "PixPin.exe -> PixPin" Bug Fix >>> */
                        else if (filename == "PixPin.exe"
                            && windowTitle == "PixPin"
                            && isVisibleWindow == true
                            && isExToolWindow == true) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace() << "[matchForegroundWindow]" << "[BugFix] Do not skip ToolbarWindow \"PixPin.exe -> PixPin\" of KeyMapStatus(KEYMAP_MAPPING_MATCHED)";
#endif
                        }
                        /* Add for "PixPin.exe -> PixPin" Bug Fix <<< */
                        else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace() << "[matchForegroundWindow]" << " Skip ToolbarWindow of KeyMapStatus(KEYMAP_MAPPING_MATCHED)";
#endif
                            return;
                        }
                    }

                    playStartSound();
                    if (matchResult == MatchResult::IgnoreBothChecks || matchResult == MatchResult::SendToSameWindows) {
                        setKeyHook(NULL);
                    }
                    else {
                        setKeyHook(hwnd);
                    }
                    m_KeyMapStatus = KEYMAP_MAPPING_MATCHED;
                    mappingStartNotification();
                    m_CheckGlobalSettingSwitchTimer.stop();
                    emit updateLockStatus_Signal();
                }
            }
        }
        else{
            if (KEYMAP_MAPPING_MATCHED == m_KeyMapStatus){
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[matchForegroundWindow]" << " MatchResult = " << static_cast<int>(matchResult) << "," << " KeyMapStatus need to change [" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << "] -> [" << keymapstatusEnum.valueToKey(KEYMAP_CHECKING) << "]" << ", ForegroundWindow: " << windowTitle << "(" << filename << ")";
                qDebug().nospace() << "[matchForegroundWindow]" << " MatchResult = " << static_cast<int>(matchResult) << "," << " KeyMapStatus need to change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") " << "ForegroundWindow: " << windowTitle << "(" << filename << ")";
                qDebug().nospace() << "[matchForegroundWindow]" << " MatchProcessIndex = " << matchProcessIndex << "," << " MatchWindowTitleIndex = " << matchWindowTitleIndex;
                qDebug().nospace() << "[matchForegroundWindow]" << " ProcessInfo.FileName = " << m_MapProcessInfo.FileName << "," << " ProcessInfo.WindowTitle = " << m_MapProcessInfo.WindowTitle;
                qDebug().nospace() << "[matchForegroundWindow]" << " ProcessPath = " << ProcessPath;
                qDebug().nospace() << "[matchForegroundWindow]" << " CurrentFileName = " << filename << "," << " CurrentWindowTitle = " << windowTitle;
                qDebug().nospace() << "[matchForegroundWindow]" << " isVisibleWindow = " << isVisibleWindow << "," << " isExToolWindow =" << isExToolWindow;
                qDebug().nospace() << "[matchForegroundWindow]" << " isToolbarWindow = " << isToolbarWindow;
#endif
                if (isToolbarWindow) {
                    /* Add for "explorer.exe -> Program Manager" Bug Fix >>> */
                    if (filename == "explorer.exe"
                        && windowTitle == "Program Manager"
                        && isVisibleWindow == true
                        && isExToolWindow == true) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace() << "[matchForegroundWindow]" << "[BugFix] Do not skip ToolbarWindow \"explorer.exe -> Program Manager\" of KeyMapStatus(KEYMAP_CHECKING)";
#endif
                    }
                    /* Add for "explorer.exe -> Program Manager" Bug Fix <<< */
                    /* Add for "PixPin.exe -> PixPin" Bug Fix >>> */
                    else if (filename == "PixPin.exe"
                        && windowTitle == "PixPin"
                        && isVisibleWindow == true
                        && isExToolWindow == true) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace() << "[matchForegroundWindow]" << "[BugFix] Do not skip ToolbarWindow \"PixPin.exe -> PixPin\" of KeyMapStatus(KEYMAP_CHECKING)";
#endif
                    }
                    /* Add for "PixPin.exe -> PixPin" Bug Fix <<< */
                    else {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace() << "[matchForegroundWindow]" << " Skip ToolbarWindow of KeyMapStatus(KEYMAP_CHECKING)";
#endif
                        return;
                    }
                }
                playStopSound();
                setKeyUnHook();
                m_KeyMapStatus = KEYMAP_CHECKING;
                mappingStopNotification();
                m_CheckGlobalSettingSwitchTimer.start();
                emit updateLockStatus_Signal();
            }
        }
    }

    // Update floating window position if using window-based reference point
    if (m_FloatingIconWindow
        && KEYMAP_MAPPING_MATCHED == m_KeyMapStatus) {
        m_FloatingIconWindow->updatePositionForCurrentWindow();
    }
}

#ifndef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
void QKeyMapper::checkGlobalSettingSwitchTimeout()
{
#ifdef DEBUG_LOGOUT_ON
    QString debugmessage = QString("[checkGlobalSettingSwitchTimeout] %1ms Timer timeout, m_KeyMapStatus = %2").arg(CHECK_GLOBALSETTING_SWITCH_TIMEOUT).arg(QMetaEnum::fromType<QKeyMapper::KeyMapStatus>().valueToKey(m_KeyMapStatus));
    qDebug().nospace().noquote() << debugmessage;
#endif
    if (m_KeyMapStatus == KEYMAP_CHECKING) {
        if (checkGlobalSettingAutoStart()) {
            loadSetting_flag = true;
            QString loadresult = loadKeyMapSetting(GROUPNAME_GLOBALSETTING);
            ui->settingNameLineEdit->setText(loadresult);
            Q_UNUSED(loadresult);
            loadSetting_flag = false;
            matchForegroundWindow();
        }
    }
}
#endif

void QKeyMapper::cycleRefreshProcessInfoTableProc()
{
    updateHWNDListProc();

    if (false == isHidden()){
        refreshProcessInfoTable();
        initInputDeviceSelectComboBoxes();
    }
}

void QKeyMapper::updateHWNDListProc()
{
    s_hWndList.clear();
    QString processNameString = m_MapProcessInfo.FileName;
    QString windowTitleString = m_MapProcessInfo.WindowTitle;
    int matchProcessIndex = ui->checkProcessComboBox->currentIndex();
    int matchWindowTitleIndex = ui->checkWindowTitleComboBox->currentIndex();
    bool matchProcess = (matchProcessIndex != WINDOWINFO_MATCH_INDEX_IGNORE && !processNameString.isEmpty());
    bool matchWindowTitle = (matchWindowTitleIndex != WINDOWINFO_MATCH_INDEX_IGNORE && !windowTitleString.isEmpty());
    if (matchProcess || matchWindowTitle) {
        EnumWindows((WNDENUMPROC)QKeyMapper::EnumWindowsBgProc, 0);
    }

    s_last_HWNDList = s_hWndList;

    if (s_last_HWNDList.isEmpty()) {
        s_CurrentMappingHWND = NULL;
    }
    else if (s_last_HWNDList.contains(s_CurrentMappingHWND) == false) {
        s_CurrentMappingHWND = s_last_HWNDList.constFirst();
    }

#ifdef DEBUG_LOGOUT_ON
    if (!s_last_HWNDList.isEmpty()) {
        qDebug().nospace() << "[updateHWNDListProc] " << m_MapProcessInfo.WindowTitle << " lastHWNDList[" << s_last_HWNDList.size() << "] -> " << s_last_HWNDList;
    } else {
        qDebug().nospace() << "[updateHWNDListProc] " << m_MapProcessInfo.WindowTitle << " lastHWNDList is empty";
    }

    if (s_CurrentMappingHWND != NULL) {
        qDebug().nospace() << "[updateHWNDListProc] " << "Title=" << m_MapProcessInfo.WindowTitle << ", Process=" << m_MapProcessInfo.FileName << " -> " << s_CurrentMappingHWND;
    } else {
        qDebug().nospace() << "[updateHWNDListProc] " << "Title=" << m_MapProcessInfo.WindowTitle << ", Process=" << m_MapProcessInfo.FileName << " -> s_CurrentMappingHWND is NULL";
    }
#endif
}

void QKeyMapper::changeLanguage(const QString &langCode)
{
    QString qt_translations_path;
    if (qEnvironmentVariableIsEmpty("QTDIR")) {
        qt_translations_path = "translations";
    }
    else {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        qt_translations_path = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
        qt_translations_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
    }
    // Load the appropriate translation file based on language code
    if (m_qt_Translator->load("qt_" + langCode + ".qm", qt_translations_path)) {
        qApp->installTranslator(m_qt_Translator);
    }

#ifdef USE_QTRANSLATOR
    QString custom_translations_path;
    if (LANGUAGECODE_ENGLISH == langCode || LANGUAGECODE_ENGLISH_US == langCode) {
        custom_translations_path = ":/QKeyMapper_en_US.qm";
    }
    else if (LANGUAGECODE_JAPANESE == langCode) {
        custom_translations_path = ":/QKeyMapper_ja_JP.qm";
    }
    else {
        custom_translations_path = ":/QKeyMapper_zh_CN.qm";
    }
    if (m_custom_Translator->load(custom_translations_path)) {
        qApp->installTranslator(m_custom_Translator);
    }
#endif
}

void QKeyMapper::setKeyHook(HWND hWnd)
{
    // updateShortcutsMap();

    if (hWnd == NULL) {
        if (s_last_HWNDList.isEmpty()
            || s_last_HWNDList.contains(s_CurrentMappingHWND) == false) {
            s_CurrentMappingHWND = NULL;
        }
    }
    else {
        s_CurrentMappingHWND = hWnd;
    }
    emit QKeyMapper_Hook_Proc::getInstance()->setKeyHook_Signal();
    emit QKeyMapper_Worker::getInstance()->setKeyHook_Signal();
}

void QKeyMapper::setKeyUnHook(void)
{
    // freeShortcuts();

    if (s_last_HWNDList.isEmpty()
        || s_last_HWNDList.contains(s_CurrentMappingHWND) == false) {
        s_CurrentMappingHWND = NULL;
    }
    emit QKeyMapper_Hook_Proc::getInstance()->setKeyUnHook_Signal();
    emit QKeyMapper_Worker::getInstance()->setKeyUnHook_Signal();
}

void QKeyMapper::setKeyMappingRestart()
{
    emit QKeyMapper_Hook_Proc::getInstance()->setKeyMappingRestart_Signal();
    emit QKeyMapper_Worker::getInstance()->setKeyMappingRestart_Signal();
}

void QKeyMapper::startWinEventHook()
{
    if (s_WinEventHook == Q_NULLPTR) {
        s_WinEventHook = SetWinEventHook(
            EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
            NULL,
            WinEventProc,
            0, 0,
            WINEVENT_OUTOFCONTEXT
        );
#ifdef DEBUG_LOGOUT_ON
        if (s_WinEventHook != Q_NULLPTR) {
            qDebug("[SetHookProc] QKeyMapper::setKeyHook() SetWinEventHook Success. -> 0x%08X", (void*)s_WinEventHook);
        }
#endif
    }
}

void QKeyMapper::stopWinEventHook()
{
    bool unhook_ret = 0;
    if (s_WinEventHook != Q_NULLPTR){
        unhook_ret = UnhookWinEvent(s_WinEventHook);
        s_WinEventHook = Q_NULLPTR;

#ifdef DEBUG_LOGOUT_ON
        if (0 == unhook_ret) {
            qDebug() << "[SetHookProc]" << "QKeyMapper::setKeyUnHook() UnhookWinEvent Failure! LastError:" << GetLastError();
        }
        else {
            qDebug("[SetHookProc] QKeyMapper::setKeyUnHook() UnhookWinEvent Success. -> 0x%08X", (void*)s_WinEventHook);
        }
#endif
    }
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

QPushButton *QKeyMapper::getOriListSelectKeyboardButton() const
{
    return ui->oriList_SelectKeyboardButton;
}

QPushButton *QKeyMapper::getOriListSelectMouseButton() const
{
    return ui->oriList_SelectMouseButton;
}

QPushButton *QKeyMapper::getOriListSelectGamepadButton() const
{
    return ui->oriList_SelectGamepadButton;
}

QPushButton *QKeyMapper::getOriListSelectFunctionButton() const
{
    return ui->oriList_SelectFunctionButton;
}

QPushButton *QKeyMapper::getMapListSelectKeyboardButton() const
{
    return ui->mapList_SelectKeyboardButton;
}

QPushButton *QKeyMapper::getMapListSelectMouseButton() const
{
    return ui->mapList_SelectMouseButton;
}

QPushButton *QKeyMapper::getMapListSelectGamepadButton() const
{
    return ui->mapList_SelectGamepadButton;
}

QPushButton *QKeyMapper::getMapListSelectFunctionButton() const
{
    return ui->mapList_SelectFunctionButton;
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

    QString retStr = QString::fromUtf16(reinterpret_cast<Utf16Pointer>(value), length);
    retStr.remove(QChar('\0'));
    return retStr;
}

QString QKeyMapper::getExeProductVersion()
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
    if (!VerQueryValue(data.data(), L"\\StringFileInfo\\040904b0\\ProductVersion", &value, &length)) {
        return QString();
    }

    QString retStr = QString::fromUtf16(reinterpret_cast<Utf16Pointer>(value), length);
    retStr.remove(QChar('\0'));
    return retStr;
}

QString QKeyMapper::getPlatformString()
{
    QString platform_string;
    QString qt_version;
    QString architecture;
    if (QSysInfo::buildCpuArchitecture() == "x86_64")
    {
        architecture = "x64";
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[getPlatformString] Qt is build for a 64-bit operating system ->" << " architecture =" << architecture;
#endif

    }
    else if (QSysInfo::buildCpuArchitecture() == "i386")
    {
        architecture = "x86";
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[getPlatformString] Qt is build for a 32-bit operating system ->" << " architecture =" << architecture;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[getPlatformString] Unknown Qt build architecture!";
#endif
    }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    qt_version = "Qt6";
#else
    qt_version = "Qt5";
#endif

    if (!architecture.isEmpty()) {
        platform_string = QString("%1_%2").arg(qt_version, architecture);
    }

    return platform_string;
}

bool QKeyMapper::IsFilterKeysEnabled()
{
    FILTERKEYS filterKeys = { sizeof(FILTERKEYS) };
    if (SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &filterKeys, 0)) {
        // Check if the dwFlags in the FILTERKEYS structure contains the FKF_FILTERKEYSON flag
        return (filterKeys.dwFlags & FKF_FILTERKEYSON) != 0;
    }
    return false;
}

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
            qDebug().nospace().noquote() << "[getProcessInfoFromPID]"<< " GetProcessImageFileName failure(" << processPathStr << ")";
#endif
            CloseHandle(hProcess);
            return;
        }

        if(!DosPathToNtPath(szImagePath, szProcessPath))
        {
            processPathStr = QString::fromWCharArray(szProcessPath);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[getProcessInfoFromPID]"<< " DosPathToNtPath failure(" << processPathStr << ")";
#endif
            CloseHandle(hProcess);
            return;
        }

        processPathStr = QString::fromWCharArray(szProcessPath);
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[getProcessInfoFromPID]" << "OpenProcess Failure:" << processID << ", LastError:" << GetLastError();
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
// #ifdef DEBUG_LOGOUT_ON
//             qDebug().nospace().noquote() << "[getProcessInfoFromHWND]"<< " GetProcessImageFileName failure(" << processPathStr << ")";
// #endif
            CloseHandle(hProcess);
            return;
        }

        if(!DosPathToNtPath(szImagePath, szProcessPath))
        {
            processPathStr = QString::fromWCharArray(szProcessPath);
// #ifdef DEBUG_LOGOUT_ON
//             qDebug().nospace().noquote() << "[getProcessInfoFromHWND]"<< " DosPathToNtPath failure(" << processPathStr << ")";
// #endif
            CloseHandle(hProcess);
            return;
        }
        processPathStr = QString::fromWCharArray(szProcessPath);
    }
    CloseHandle( hProcess );
}

QString QKeyMapper::getProcessPathFromPID(DWORD dwProcessId)
{
    QString ProcessPath;
    // QString processName;

    getProcessInfoFromPID(dwProcessId, ProcessPath);

    if (ProcessPath.isEmpty()) {
        bool adjust_priv;
        adjust_priv = EnablePrivilege(SE_DEBUG_NAME);
        if (adjust_priv) {
            getProcessInfoFromPID(dwProcessId, ProcessPath);
        }
        else {
            qDebug() << "[EnumWindowsProc]" << "getProcessInfoFromPID EnablePrivilege(SE_DEBUG_NAME) Failed with ->" << GetLastError();
        }
        adjust_priv = DisablePrivilege(SE_DEBUG_NAME);

        if (!adjust_priv) {
            qDebug() << "[EnumWindowsProc]" << "getProcessInfoFromPID DisablePrivilege(SE_DEBUG_NAME) Failed with ->" << GetLastError();
        }
#ifdef DEBUG_LOGOUT_ON
        if (ProcessPath.isEmpty()) {
            qDebug().nospace().noquote() << "[EnumWindowsProc] " << "EnablePrivilege(SE_DEBUG_NAME) getProcessInfoFromPID Failed! -> " << " [PID:" << dwProcessId <<"]";
        }
        else {
            qDebug().nospace().noquote() << "[EnumWindowsProc] " << "EnablePrivilege(SE_DEBUG_NAME) getProcessInfoFromPID Success -> " << ProcessPath << " [PID:" << dwProcessId <<"]";
        }
#endif
    }

    // if (false == ProcessPath.isEmpty()){
    //     QFileInfo fileinfo(ProcessPath);
    //     processName = fileinfo.fileName();
    // }

    return ProcessPath;
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

BOOL QKeyMapper::enumIconGroupsProc(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam)
{
    Q_UNUSED(hModule);
    Q_UNUSED(lpType);
    IconEnumData* pData = reinterpret_cast<IconEnumData*>(lParam);

    // If we only want the first group and haven't processed it yet
    if (pData->onlyFirstGroup && !pData->firstGroupProcessed) {
        pData->firstGroupName = lpName;
        pData->firstGroupProcessed = true;

#ifdef EXTRACTICON_VERBOSE_LOG
        if (IS_INTRESOURCE(lpName)) {
            qDebug().nospace() << "[enumIconGroupsProc] Found first icon group (ID): " << reinterpret_cast<ULONG_PTR>(lpName);
        } else {
            QString groupName = QString::fromWCharArray(lpName);
            qDebug().nospace() << "[enumIconGroupsProc] Found first icon group (Name): " << groupName;
        }
#endif
        return FALSE; // Stop enumeration after finding the first group
    }

    return TRUE; // Continue enumeration if not in "first group only" mode
}

BOOL QKeyMapper::enumIconsProc(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam)
{
    IconEnumData* pData = reinterpret_cast<IconEnumData*>(lParam);

    HRSRC hRes = FindResourceW(hModule, lpName, lpType);
    if (!hRes) {
        return TRUE; // Continue enumeration
    }

    DWORD size = SizeofResource(hModule, hRes);
    HGLOBAL hGlobal = LoadResource(hModule, hRes);
    if (!hGlobal) {
        return TRUE; // Continue enumeration
    }

    BYTE* pBytes = static_cast<BYTE*>(LockResource(hGlobal));
    if (!pBytes) {
        return TRUE; // Continue enumeration
    }

    // Create icon from resource data
    HICON hIcon = CreateIconFromResourceEx(pBytes, size, TRUE, 0x00030000, 0, 0, 0);
    if (hIcon) {
        // Get icon size information
        ICONINFOEX iconInfoEx = { sizeof(ICONINFOEX) };
        if (GetIconInfoEx(hIcon, &iconInfoEx) && iconInfoEx.hbmColor) {
            BITMAP bitmap;
            if (GetObject(iconInfoEx.hbmColor, sizeof(bitmap), &bitmap) > 0) {
                int iconSize = bitmap.bmWidth; // Assume square icons
                int bitCount = bitmap.bmBitsPixel; // Get color depth
                pData->icons.append(IconInfo(hIcon, iconSize, bitCount));

#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[extractBestIconFromExecutable] Found icon: " << iconSize << "x" << iconSize << " (" << bitCount << " bits)";
#endif
            } else {
                DestroyIcon(hIcon); // Clean up if we can't get size info
            }

            // Clean up bitmap handles
            if (iconInfoEx.hbmColor) DeleteObject(iconInfoEx.hbmColor);
            if (iconInfoEx.hbmMask) DeleteObject(iconInfoEx.hbmMask);
        } else {
            DestroyIcon(hIcon); // Clean up if we can't get icon info
        }
    }

    return TRUE; // Continue enumeration
}

// Static helper function to extract icon using traditional ExtractIconEx method
// This serves as a fallback when resource enumeration fails or for simple icon extraction needs
QIcon QKeyMapper::extractIconFromExecutable(const QString &filePath, int targetSize)
{
    QIcon result;

    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return result;
    }

    std::wstring wFilePath = filePath.toStdWString();
    HICON hLargeIcon = nullptr;
    HICON hSmallIcon = nullptr;

    // Extract both large and small icons from executable
    if (ExtractIconExW(wFilePath.c_str(), 0, &hLargeIcon, &hSmallIcon, 1) > 0) {
        // Choose appropriate icon based on target size
        HICON selectedIcon = (targetSize > 32 && hLargeIcon) ? hLargeIcon : hSmallIcon;

        if (selectedIcon) {
            // Convert HICON to QPixmap
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(selectedIcon));
#else
            QPixmap pixmap = QtWin::fromHICON(selectedIcon);
#endif

            if (!pixmap.isNull()) {
                result.addPixmap(pixmap);
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[extractIconFromExecutable] Successfully extracted icon, pixmap size:" << pixmap.size();
#endif
            }
        }

        // Clean up icon handles
        if (hLargeIcon) DestroyIcon(hLargeIcon);
        if (hSmallIcon) DestroyIcon(hSmallIcon);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractIconFromExecutable] Failed to extract icon from:" << filePath;
#endif
    }

    return result;
}

QIcon QKeyMapper::extractBestIconFromExecutable(const QString &filePath, int targetSize)
{
    QIcon result;

    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return result;
    }

    std::wstring wFilePath = filePath.toStdWString();

    // Load the executable as an image resource
    HMODULE hModule = LoadLibraryExW(wFilePath.c_str(), NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (!hModule) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractBestIconFromExecutable] Failed to load module:" << filePath;
#endif
        return result;
    }

    IconEnumData enumData;
    enumData.hModule = hModule;
    enumData.onlyFirstGroup = true;
    enumData.firstGroupProcessed = false;
    enumData.firstGroupName = nullptr;

    // First, enumerate icon groups to find the first group
    EnumResourceNamesW(hModule, RT_GROUP_ICON, enumIconGroupsProc, reinterpret_cast<LONG_PTR>(&enumData));

#ifdef EXTRACTICON_VERBOSE_LOG
    qDebug() << "[extractBestIconFromExecutable] First group enumeration completed, firstGroupProcessed:" << enumData.firstGroupProcessed;
#endif

    // Now enumerate only the icons from the first group
    if (enumData.firstGroupProcessed && enumData.firstGroupName) {
        // Load the icon group resource to get the icon IDs
        HRSRC hGroupRes = FindResourceW(hModule, enumData.firstGroupName, RT_GROUP_ICON);
        if (hGroupRes) {
            HGLOBAL hGroupGlobal = LoadResource(hModule, hGroupRes);
            if (hGroupGlobal) {
                BYTE* pGroupData = static_cast<BYTE*>(LockResource(hGroupGlobal));
                if (pGroupData) {
                    // Parse the icon group structure
                    GRPICONDIR* pDir = reinterpret_cast<GRPICONDIR*>(pGroupData);
                    GRPICONDIRENTRY* pEntries = reinterpret_cast<GRPICONDIRENTRY*>(pGroupData + sizeof(GRPICONDIR));

#ifdef EXTRACTICON_VERBOSE_LOG
                    qDebug().nospace() << "[extractBestIconFromExecutable] First icon group contains " << pDir->idCount << " icons";
                    qDebug().nospace() << "[extractBestIconFromExecutable] Group info - Reserved:" << pDir->idReserved
                                      << ", Type:" << pDir->idType << ", Count:" << pDir->idCount;
#endif

                    // Load each icon from the first group
                    for (int i = 0; i < pDir->idCount; ++i) {
                        GRPICONDIRENTRY& entry = pEntries[i];

#ifdef EXTRACTICON_VERBOSE_LOG
                        qDebug().nospace() << "[extractBestIconFromExecutable] Processing icon " << (i+1) << "/" << pDir->idCount
                                          << " - Size:" << (int)entry.bWidth << "x" << (int)entry.bHeight 
                                          << ", BitCount:" << entry.wBitCount << ", ID:" << entry.nID;
#endif

                        HRSRC hIconRes = FindResourceW(hModule, MAKEINTRESOURCEW(entry.nID), RT_ICON);
                        if (hIconRes) {
                            DWORD iconSize = SizeofResource(hModule, hIconRes);
                            HGLOBAL hIconGlobal = LoadResource(hModule, hIconRes);
                            if (hIconGlobal) {
                                BYTE* pIconData = static_cast<BYTE*>(LockResource(hIconGlobal));
                                if (pIconData) {
                                    HICON hIcon = CreateIconFromResourceEx(pIconData, iconSize, TRUE, 0x00030000, 0, 0, 0);
                                    if (hIcon) {
                                        int iconSizePixels = (entry.bWidth == 0) ? 256 : entry.bWidth; // 0 means 256
                                        int bitCount = entry.wBitCount; // Get color depth from icon group entry
                                        enumData.icons.append(IconInfo(hIcon, iconSizePixels, bitCount));

#ifdef EXTRACTICON_VERBOSE_LOG
                                        qDebug().nospace() << "[extractBestIconFromExecutable] Successfully loaded icon from first group: "
                                                          << iconSizePixels << "x" << iconSizePixels << " (" << bitCount << " bits)";
#endif
                                    } else {
#ifdef DEBUG_LOGOUT_ON
                                        qDebug().nospace() << "[extractBestIconFromExecutable] Failed to create icon from resource ID " << entry.nID;
#endif
                                    }
                                } else {
#ifdef DEBUG_LOGOUT_ON
                                    qDebug().nospace() << "[extractBestIconFromExecutable] Failed to lock resource for icon ID " << entry.nID;
#endif
                                }
                            } else {
#ifdef DEBUG_LOGOUT_ON
                                qDebug().nospace() << "[extractBestIconFromExecutable] Failed to load resource for icon ID " << entry.nID;
#endif
                            }
                        } else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace() << "[extractBestIconFromExecutable] Failed to find resource for icon ID " << entry.nID;
#endif
                        }
                    }
                }
            }
        }
    }
    else {
        // Fallback: if no icon groups found, enumerate all icons (original behavior)
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractBestIconFromExecutable] No icon groups found, falling back to enumerate all icons";
#endif
        enumData.onlyFirstGroup = false;
        EnumResourceNamesW(hModule, RT_ICON, enumIconsProc, reinterpret_cast<LONG_PTR>(&enumData));
    }

#ifdef EXTRACTICON_VERBOSE_LOG
    qDebug() << "[extractBestIconFromExecutable] Total icons collected:" << enumData.icons.size();
#endif

    // Sort icons by quality: first by color depth (descending), then by size (descending)
    // This ensures we prioritize higher bit depth icons for better visual quality
    std::sort(enumData.icons.begin(), enumData.icons.end(),
              [](const IconInfo& a, const IconInfo& b) {
                  // First priority: higher bit count (better color quality)
                  if (a.bitCount != b.bitCount) {
                      return a.bitCount > b.bitCount;
                  }
                  // Second priority: larger size if bit count is the same
                  return a.size > b.size;
              });

    // Find the best icon based on target size and quality
    // Icons are now sorted by bit count (descending) then size (descending)
    HICON bestIcon = nullptr;
    int bestSize = 0;
    int bestBitCount = 0;
    int bestIndex = -1;

    // Strategy: Find the highest quality icon that fits the target size
    // If no icon fits exactly, prefer higher bit count over size matching
    for (int i = 0; i < enumData.icons.size(); ++i) {
        const IconInfo& iconInfo = enumData.icons[i];
        int iconSize = iconInfo.size;
        int bitCount = iconInfo.bitCount;

        if (iconSize <= targetSize) {
            // Found an icon that fits within target size
            // Since icons are sorted by quality first, this is the best quality that fits
            bestIcon = iconInfo.hIcon;
            bestSize = iconSize;
            bestBitCount = bitCount;
            bestIndex = i;
#ifdef EXTRACTICON_VERBOSE_LOG
            qDebug().nospace() << "[extractBestIconFromExecutable] Found optimal icon: " << bestSize << "x" << bestSize
                              << " (" << bestBitCount << " bits) for target: " << targetSize << "x" << targetSize;
#endif
            break;
        } else if (!bestIcon) {
            // If no icon fits within target size, use the first one (highest quality)
            // This will be the highest bit count icon available
            bestIcon = iconInfo.hIcon;
            bestSize = iconSize;
            bestBitCount = bitCount;
            bestIndex = i;
        }
    }

    // If we only found oversized icons, bestIcon contains the highest quality one
    if (bestIcon && bestSize > targetSize) {
#ifdef EXTRACTICON_VERBOSE_LOG
        qDebug().nospace() << "[extractBestIconFromExecutable] Using highest quality available icon: " << bestSize << "x" << bestSize
                          << " (" << bestBitCount << " bits) for target: " << targetSize << "x" << targetSize;
#endif
    }

    if (bestIcon) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[extractBestIconFromExecutable] Selected best icon: " << bestSize << "x" << bestSize
                          << " (" << bestBitCount << " bits) for target: " << targetSize << "x" << targetSize;
#endif

        // Convert HICON to QPixmap
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(bestIcon));
#else
        QPixmap pixmap = QtWin::fromHICON(bestIcon);
#endif

        if (!pixmap.isNull()) {
            result.addPixmap(pixmap);

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[extractBestIconFromExecutable] Successfully extracted icon, actual pixmap size: " << pixmap.size();
            qDebug().nospace() << "[extractBestIconFromExecutable] QIcon availableSizes: " << result.availableSizes();
#endif
        }
    }

    // Clean up all icons except the one we used
    for (int i = 0; i < enumData.icons.size(); ++i) {
        if (i != bestIndex) {
            DestroyIcon(enumData.icons[i].hIcon);
        }
    }

    // Clean up the selected icon after conversion to QPixmap
    if (bestIcon) {
        DestroyIcon(bestIcon);
    }

    // Free the loaded module
    FreeLibrary(hModule);

    // If resource enumeration failed, fallback to ExtractIconEx as backup
    if (result.isNull()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractBestIconFromExecutable] Resource enumeration failed, trying ExtractIconEx fallback";
#endif
        result = extractIconFromExecutable(filePath, targetSize);
    }

    return result;
}

#if 0
QIcon QKeyMapper::extractAllBestIconsFromExecutable(const QString &filePath)
{
    QIcon result;

    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return result;
    }

    std::wstring wFilePath = filePath.toStdWString();

    // Load the executable as an image resource
    HMODULE hModule = LoadLibraryExW(wFilePath.c_str(), NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (!hModule) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractAllBestIconsFromExecutable] Failed to load module:" << filePath;
#endif
        return result;
    }

    IconEnumData enumData;
    enumData.hModule = hModule;
    enumData.onlyFirstGroup = true;
    enumData.firstGroupProcessed = false;
    enumData.firstGroupName = nullptr;

    // First, enumerate icon groups to find the first group
    EnumResourceNamesW(hModule, RT_GROUP_ICON, enumIconGroupsProc, reinterpret_cast<LONG_PTR>(&enumData));

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[extractAllBestIconsFromExecutable] First group enumeration completed, firstGroupProcessed:" << enumData.firstGroupProcessed;
#endif

    // Now enumerate only the icons from the first group
    if (enumData.firstGroupProcessed && enumData.firstGroupName) {
        // Load the icon group resource to get the icon IDs
        HRSRC hGroupRes = FindResourceW(hModule, enumData.firstGroupName, RT_GROUP_ICON);
        if (hGroupRes) {
            HGLOBAL hGroupGlobal = LoadResource(hModule, hGroupRes);
            if (hGroupGlobal) {
                BYTE* pGroupData = static_cast<BYTE*>(LockResource(hGroupGlobal));
                if (pGroupData) {
                    // Parse the icon group structure
                    GRPICONDIR* pDir = reinterpret_cast<GRPICONDIR*>(pGroupData);
                    GRPICONDIRENTRY* pEntries = reinterpret_cast<GRPICONDIRENTRY*>(pGroupData + sizeof(GRPICONDIR));

#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "[extractAllBestIconsFromExecutable] First icon group contains " << pDir->idCount << " icons";
                    qDebug().nospace() << "[extractAllBestIconsFromExecutable] Group info - Reserved:" << pDir->idReserved
                                      << ", Type:" << pDir->idType << ", Count:" << pDir->idCount;
#endif

                    // Load each icon from the first group
                    for (int i = 0; i < pDir->idCount; ++i) {
                        GRPICONDIRENTRY& entry = pEntries[i];

#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace() << "[extractAllBestIconsFromExecutable] Processing icon " << (i+1) << "/" << pDir->idCount
                                          << " - Size:" << (int)entry.bWidth << "x" << (int)entry.bHeight
                                          << ", BitCount:" << entry.wBitCount << ", ID:" << entry.nID;
#endif

                        HRSRC hIconRes = FindResourceW(hModule, MAKEINTRESOURCEW(entry.nID), RT_ICON);
                        if (hIconRes) {
                            DWORD iconSize = SizeofResource(hModule, hIconRes);
                            HGLOBAL hIconGlobal = LoadResource(hModule, hIconRes);
                            if (hIconGlobal) {
                                BYTE* pIconData = static_cast<BYTE*>(LockResource(hIconGlobal));
                                if (pIconData) {
                                    HICON hIcon = CreateIconFromResourceEx(pIconData, iconSize, TRUE, 0x00030000, 0, 0, 0);
                                    if (hIcon) {
                                        int iconSizePixels = (entry.bWidth == 0) ? 256 : entry.bWidth; // 0 means 256
                                        int bitCount = entry.wBitCount; // Get color depth from icon group entry
                                        enumData.icons.append(IconInfo(hIcon, iconSizePixels, bitCount));

#ifdef DEBUG_LOGOUT_ON
                                        qDebug().nospace() << "[extractAllBestIconsFromExecutable] Successfully loaded icon from first group: "
                                                          << iconSizePixels << "x" << iconSizePixels << " (" << bitCount << " bits)";
#endif
                                    } else {
#ifdef DEBUG_LOGOUT_ON
                                        qDebug().nospace() << "[extractAllBestIconsFromExecutable] Failed to create icon from resource ID " << entry.nID;
#endif
                                    }
                                } else {
#ifdef DEBUG_LOGOUT_ON
                                    qDebug().nospace() << "[extractAllBestIconsFromExecutable] Failed to lock resource for icon ID " << entry.nID;
#endif
                                }
                            } else {
#ifdef DEBUG_LOGOUT_ON
                                qDebug().nospace() << "[extractAllBestIconsFromExecutable] Failed to load resource for icon ID " << entry.nID;
#endif
                            }
                        } else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace() << "[extractAllBestIconsFromExecutable] Failed to find resource for icon ID " << entry.nID;
#endif
                        }
                    }
                }
            }
        }
    }
    else {
        // Fallback: if no icon groups found, enumerate all icons (original behavior)
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractAllBestIconsFromExecutable] No icon groups found, falling back to enumerate all icons";
#endif
        enumData.onlyFirstGroup = false;
        EnumResourceNamesW(hModule, RT_ICON, enumIconsProc, reinterpret_cast<LONG_PTR>(&enumData));
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[extractAllBestIconsFromExecutable] Total icons collected:" << enumData.icons.size();
#endif

    // Find the highest bit count (best quality) in the collected icons
    int maxBitCount = 0;
    for (const IconInfo& iconInfo : enumData.icons) {
        if (iconInfo.bitCount > maxBitCount) {
            maxBitCount = iconInfo.bitCount;
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[extractAllBestIconsFromExecutable] Highest bit count found: " << maxBitCount << " bits";
#endif

    // Filter icons to only include those with the highest bit count
    QList<IconInfo> highestQualityIcons;
    for (const IconInfo& iconInfo : enumData.icons) {
        if (iconInfo.bitCount == maxBitCount) {
            highestQualityIcons.append(iconInfo);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[extractAllBestIconsFromExecutable] Filtered to " << highestQualityIcons.size() << " icons with " << maxBitCount << " bits";
#endif

    // Sort the highest quality icons by size in descending order (largest to smallest)
    std::sort(highestQualityIcons.begin(), highestQualityIcons.end(),
              [](const IconInfo& a, const IconInfo& b) {
                  return a.size > b.size; // Sort by size (descending)
              });

    // Convert all highest quality icons to QPixmap and add them to the result QIcon
    // Process in order from largest to smallest
    for (int i = 0; i < highestQualityIcons.size(); ++i) {
        const IconInfo& iconInfo = highestQualityIcons[i];
        HICON hIcon = iconInfo.hIcon;
        int iconSize = iconInfo.size;
        int bitCount = iconInfo.bitCount;

        if (hIcon) {
            // Convert HICON to QPixmap
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(hIcon));
#else
            QPixmap pixmap = QtWin::fromHICON(hIcon);
#endif

            if (!pixmap.isNull()) {
                result.addPixmap(pixmap);
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[extractAllBestIconsFromExecutable] Added icon: " << iconSize << "x" << iconSize 
                                  << " (" << bitCount << " bits) - actual pixmap: " << pixmap.size();
#endif
            }
        }
    }

    // Clean up all icon handles
    for (const IconInfo& iconInfo : enumData.icons) {
        if (iconInfo.hIcon) {
            DestroyIcon(iconInfo.hIcon);
        }
    }

    // Free the loaded module
    FreeLibrary(hModule);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[extractAllBestIconsFromExecutable] Successfully extracted highest quality icons, QIcon availableSizes: " << result.availableSizes();
#endif

    // If resource enumeration failed, fallback to extracting standard icons
    if (result.isNull()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractAllBestIconsFromExecutable] Resource enumeration failed, trying ExtractIconEx fallback";
#endif

        // Try to extract both large and small icons as fallback
        HICON hLargeIcon = nullptr;
        HICON hSmallIcon = nullptr;

        if (ExtractIconExW(wFilePath.c_str(), 0, &hLargeIcon, &hSmallIcon, 1) > 0) {
            // Add large icon first (if available)
            if (hLargeIcon) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                QPixmap largePixmap = QPixmap::fromImage(QImage::fromHICON(hLargeIcon));
#else
                QPixmap largePixmap = QtWin::fromHICON(hLargeIcon);
#endif
                if (!largePixmap.isNull()) {
                    result.addPixmap(largePixmap);
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[extractAllBestIconsFromExecutable] Fallback: Added large icon, pixmap size:" << largePixmap.size();
#endif
                }
            }

            // Add small icon second (if available and different from large)
            if (hSmallIcon) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                QPixmap smallPixmap = QPixmap::fromImage(QImage::fromHICON(hSmallIcon));
#else
                QPixmap smallPixmap = QtWin::fromHICON(hSmallIcon);
#endif
                if (!smallPixmap.isNull()) {
                    result.addPixmap(smallPixmap);
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[extractAllBestIconsFromExecutable] Fallback: Added small icon, pixmap size:" << smallPixmap.size();
#endif
                }
            }
        }

        // Clean up icon handles
        if (hLargeIcon) DestroyIcon(hLargeIcon);
        if (hSmallIcon) DestroyIcon(hSmallIcon);
    }

    return result;
}
#endif

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

    int resultLength = GetWindowText(hWnd, titleBuffer, MAX_PATH);
    if (resultLength){
        WindowText = QString::fromWCharArray(titleBuffer);
        getProcessInfoFromPID(dwProcessId, ProcessPath);

        if (ProcessPath.isEmpty()) {
            bool adjust_priv;
            adjust_priv = EnablePrivilege(SE_DEBUG_NAME);
            if (adjust_priv) {
                getProcessInfoFromPID(dwProcessId, ProcessPath);
            }
            else {
                qDebug() << "[EnumWindowsProc]" << "getProcessInfoFromPID EnablePrivilege(SE_DEBUG_NAME) Failed with ->" << GetLastError();
            }
            adjust_priv = DisablePrivilege(SE_DEBUG_NAME);

            if (!adjust_priv) {
                qDebug() << "[EnumWindowsProc]" << "getProcessInfoFromPID DisablePrivilege(SE_DEBUG_NAME) Failed with ->" << GetLastError();
            }
#ifdef DEBUG_LOGOUT_ON
            if (ProcessPath.isEmpty()) {
                qDebug().nospace().noquote() << "[EnumWindowsProc] " << "EnablePrivilege(SE_DEBUG_NAME) getProcessInfoFromPID Failed! -> " << " [PID:" << dwProcessId <<"]";
            }
            else {
                qDebug().nospace().noquote() << "[EnumWindowsProc] " << "EnablePrivilege(SE_DEBUG_NAME) getProcessInfoFromPID Success -> " << ProcessPath << " [PID:" << dwProcessId <<"]";
            }
#endif
        }

        if (ProcessPath.isEmpty()) {
            ProcessPath = PROCESS_UNKNOWN;
        }

        if (false == WindowText.isEmpty()){
            MAP_PROCESSINFO ProcessInfo;
            QFileInfo fileinfo(ProcessPath);
            filename = fileinfo.fileName();

            ProcessInfo.FileName = filename;
            ProcessInfo.PID = QString::number(dwProcessId);
            ProcessInfo.WindowTitle = WindowText;
            ProcessInfo.FilePath = ProcessPath;

            QIcon fileicon;
            fileicon = extractIconFromExecutable(ProcessPath);

            if (fileicon.isNull()) {
                QFileIconProvider icon_provider;
                fileicon = icon_provider.icon(QFileInfo(ProcessPath));
            }

            if (fileicon.isNull()) {
                HICON iconptr = (HICON)(LONG_PTR)GetClassLongPtr(hWnd, GCLP_HICON);
                if (iconptr != Q_NULLPTR){
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                    fileicon = QIcon(QPixmap::fromImage(QImage::fromHICON(iconptr)));
#else
                    fileicon = QIcon(QtWin::fromHICON(iconptr));
#endif
                }
            }

            if (!fileicon.isNull()) {
                ProcessInfo.WindowIcon = fileicon;
            }

            if (!ProcessInfo.FilePath.isEmpty()){
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
                }
                else {
                    static_ProcessInfoList.append(ProcessInfo);
                }

#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[EnumWindowsProc] " << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << "), " << "IsAltTabWindow = " << (isVisibleWindow == TRUE) << ", Win10Above = " << isWin10Above;
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

#if 0
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

            if (!ProcessInfo.WindowIcon.isNull()){
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
#endif

BOOL QKeyMapper::DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath)
{
    TCHAR           szDriveStr[500];
    TCHAR           szDrive[3];
    TCHAR           szDevName[100];
    size_t          cchDevName;
    INT             i;

    if(!pszDosPath || !pszNtPath )
        return FALSE;

    // Check for network paths and handle them separately
    if (_wcsnicmp(pszDosPath, L"\\Device\\Mup\\", 12) == 0)
    {
        lstrcpy(pszNtPath, L"\\\\");
        lstrcat(pszNtPath, pszDosPath + wcslen(L"\\Device\\Mup\\"));
        return TRUE;
    }
    else if (_wcsnicmp(pszDosPath, L"\\Device\\LanmanRedirector\\", 25) == 0)
    {
        lstrcpy(pszNtPath, L"\\\\");
        lstrcat(pszNtPath, pszDosPath + wcslen(L"\\Device\\LanmanRedirector\\"));
        return TRUE;
    }

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

BOOL QKeyMapper::EnumWindowsBgProc(HWND hWnd, LPARAM lParam)
{
    Q_UNUSED(lParam);

    if(FALSE == IsWindowVisible(hWnd)){
        return TRUE;
    }

    TCHAR titleBuffer[MAX_PATH] = TEXT("");
    memset(titleBuffer, 0x00, sizeof(titleBuffer));

    int resultLength = GetWindowText(hWnd, titleBuffer, MAX_PATH);
    if (resultLength){
        QString WindowText = QString::fromWCharArray(titleBuffer);
        collectWindowsHWND(WindowText, hWnd);
    }

    return TRUE;
}

void QKeyMapper::collectWindowsHWND(const QString &WindowText, HWND hWnd)
{
    QString processNameString = QKeyMapper::getInstance()->m_MapProcessInfo.FileName;
    QString windowTitleString = QKeyMapper::getInstance()->m_MapProcessInfo.WindowTitle;
    int matchProcessIndex = getMatchProcessNameIndex();
    int matchWindowTitleIndex = getMatchWindowTitleIndex();
    bool matchProcess = (matchProcessIndex != WINDOWINFO_MATCH_INDEX_IGNORE && !processNameString.isEmpty());
    bool matchWindowTitle = (matchWindowTitleIndex != WINDOWINFO_MATCH_INDEX_IGNORE && !windowTitleString.isEmpty());

    if (!matchProcess && !matchWindowTitle) {
        return;
    }

    if (false == WindowText.isEmpty()){
        DWORD dwProcessId = 0;
        GetWindowThreadProcessId(hWnd, &dwProcessId);
        QString processPath = getProcessPathFromPID(dwProcessId);

        // Perform matching logic based on ComboBox selection
        bool processMatched = false;
        bool windowTitleMatched = false;

        // Check for process name match
        if (matchProcess) {
            if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_EQUALS) {
                processMatched = (processNameString == processPath);
            }
            else if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_CONTAINS) {
                processMatched = processPath.contains(processNameString);
            }
            else if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_STARTSWITH) {
                processMatched = processPath.startsWith(processNameString);
            }
            else if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_ENDSWITH) {
                processMatched = processPath.endsWith(processNameString);
            }
        }
        else {
            processMatched = true; // Treat as matched if process name check is ignored
        }

        // Check for window title match
        if (matchWindowTitle) {
            if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_EQUALS) {
                windowTitleMatched = (windowTitleString == WindowText);
            }
            else if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_CONTAINS) {
                windowTitleMatched = WindowText.contains(windowTitleString);
            }
            else if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_STARTSWITH) {
                windowTitleMatched = WindowText.startsWith(windowTitleString);
            }
            else if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_ENDSWITH) {
                windowTitleMatched = WindowText.endsWith(windowTitleString);
            }
        }
        else {
            windowTitleMatched = true; // Treat as matched if window title check is ignored
        }

        // Set matchResult based on the matching outcome
        bool hwndMatched = false;
        if (matchProcess && matchWindowTitle) {
            // Check both process name and window title
            if (processMatched && windowTitleMatched) {
                hwndMatched = true;
            }
        }
        else if (matchProcess) {
            // Check process name only
            if (processMatched) {
                hwndMatched = true;
            }
        }
        else if (matchWindowTitle) {
            // Check window title only
            if (windowTitleMatched) {
                hwndMatched = true;
            }
        }

        if (hwndMatched) {
            if (!s_hWndList.contains(hWnd)) {
                s_hWndList.append(hWnd);
            }
        }
    }
}

void QKeyMapper::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    Q_UNUSED(hWinEventHook);
    Q_UNUSED(hwnd);
    Q_UNUSED(idObject);
    Q_UNUSED(idChild);
    Q_UNUSED(dwEventThread);
    Q_UNUSED(dwmsEventTime);

    if (event == EVENT_SYSTEM_FOREGROUND) {
#ifdef DEBUG_LOGOUT_ON
        QString windowTitle;
        TCHAR titleBuffer[MAX_PATH];
        memset(titleBuffer, 0x00, sizeof(titleBuffer));

        int resultLength = GetWindowText(hwnd, titleBuffer, MAX_PATH);
        if (resultLength){
            windowTitle = QString::fromWCharArray(titleBuffer);
            qDebug().nospace() << "\033[1;34m[QKeyMapper::WinEventProc]" << "EVENT_SYSTEM_FOREGROUND Foregound Window Title ->" << windowTitle << "\033[0m";
        }
#endif

        getInstance()->matchForegroundWindow();
    }
}

int QKeyMapper::findOriKeyInKeyMappingDataList(const QString &keyname, QList<MAP_KEYDATA> *keyMappingDataListToCheck)
{
    if (keyMappingDataListToCheck == Q_NULLPTR) {
        keyMappingDataListToCheck = KeyMappingDataList;
    }

    int returnindex = -1;
    int keymapdataindex = 0;
    QString keyname_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keyname);

    for (const MAP_KEYDATA &keymapdata : std::as_const(*keyMappingDataListToCheck))
    {
        if (keymapdata.Original_Key == keyname
            || keymapdata.Original_Key == keyname_RemoveMultiInput){
            returnindex = keymapdataindex;
            break;
        }
        else if (keyname.endsWith(SEPARATOR_DOUBLEPRESS)
            && (keymapdata.Original_Key.startsWith(keyname)
                || keymapdata.Original_Key.startsWith(keyname_RemoveMultiInput))) {
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}

int QKeyMapper::findOriKeyInKeyMappingDataList_RemoveMultiInput(const QString &keyname)
{
    int returnindex = -1;
    int keymapdataindex = 0;
    QString keyname_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keyname);

    for (const MAP_KEYDATA &keymapdata : std::as_const(*KeyMappingDataList))
    {
        QString originalkey_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keymapdata.Original_Key);
        if (originalkey_RemoveMultiInput == keyname_RemoveMultiInput
            || originalkey_RemoveMultiInput == keyname){
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}

#if 0
int QKeyMapper::findOriKeyInCertainKeyMappingDataList(const QString &keyname, QList<MAP_KEYDATA> *keyMappingDataListToCheck)
{
    int returnindex = -1;
    int keymapdataindex = 0;
    QString keyname_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keyname);

    for (const MAP_KEYDATA &keymapdata : std::as_const(*keyMappingDataListToCheck))
    {
        if (keymapdata.Original_Key == keyname
            || keymapdata.Original_Key == keyname_RemoveMultiInput){
            returnindex = keymapdataindex;
            break;
        }
        else if (keyname.endsWith(SEPARATOR_DOUBLEPRESS)
            && (keymapdata.Original_Key.startsWith(keyname)
                || keymapdata.Original_Key.startsWith(keyname_RemoveMultiInput))) {
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}
#endif

int QKeyMapper::findOriKeyInKeyMappingDataList(const QString &keyname, bool &removemultiinput)
{
    int returnindex = -1;
    int keymapdataindex = 0;
    QString keyname_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keyname);
    removemultiinput = false;

    for (const MAP_KEYDATA &keymapdata : std::as_const(*KeyMappingDataList))
    {
        if (keymapdata.Original_Key == keyname){
            returnindex = keymapdataindex;
            break;
        }
        else if (keymapdata.Original_Key == keyname_RemoveMultiInput){
            returnindex = keymapdataindex;
            removemultiinput = true;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}

int QKeyMapper::findOriKeyInKeyMappingDataList_ForAddMappingData(const QString &keyname)
{
    int returnindex = -1;
    int keymapdataindex = 0;

    for (const MAP_KEYDATA &keymapdata : std::as_const(*KeyMappingDataList))
    {
        if (keymapdata.Original_Key == keyname){
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}

int QKeyMapper::findOriKeyInKeyMappingDataList_ForDoublePress(const QString &keyname)
{
    int returnindex = -1;
    int keymapdataindex = 0;
    QString keyname_doublepress = keyname + QString(SEPARATOR_DOUBLEPRESS);

    for (const MAP_KEYDATA &keymapdata : std::as_const(*KeyMappingDataList))
    {
        if (keymapdata.Original_Key.startsWith(keyname_doublepress)){
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}

#if 0
int QKeyMapper::findOriKeyInKeyMappingDataListGlobal(const QString &keyname)
{
    int returnindex = -1;
    int keymapdataindex = 0;
    QString keyname_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keyname);

    for (const MAP_KEYDATA &keymapdata : std::as_const(KeyMappingDataListGlobal))
    {
        if (keymapdata.Original_Key == keyname
            || keymapdata.Original_Key == keyname_RemoveMultiInput){
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}
#endif

int QKeyMapper::findMapKeyInKeyMappingDataList(const QString &keyname)
{
    int returnindex = -1;
    int keymapdataindex = 0;

    for (const MAP_KEYDATA &keymapdata : std::as_const(*KeyMappingDataList))
    {
        if (keymapdata.Mapping_Keys.contains(keyname)){
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}

ValidationResult QKeyMapper::validateOriginalKeyString(const QString &originalkeystr, int update_rowindex)
{
    ValidationResult result;
    result.isValid = true;

    // Regular expression to match the entire key with optional time suffix
    static QRegularExpression full_key_regex(R"(^(.+?)(?:⏲(\d+)|✖(\d+))?$)");
    static QRegularExpression removeindex_regex("@\\d$");

    QRegularExpressionMatch full_key_match = full_key_regex.match(originalkeystr);
    if (!full_key_match.hasMatch()) {
        result.isValid = false;
        result.errorMessage = tr("Invalid original key format.");
        return result;
    }

    QString key_without_suffix = full_key_match.captured(1);
    QString longPressTimeString = full_key_match.captured(2);
    QString doublePressTimeString = full_key_match.captured(3);

    QStringList orikeylist = key_without_suffix.split(SEPARATOR_PLUS);
    if (orikeylist.isEmpty()) {
        result.isValid = false;
        result.errorMessage = tr("OriginalKey is empty.");
        return result;
    }

    // Check for duplicate keys
    int numRemoved = orikeylist.removeDuplicates();
    if (numRemoved > 0) {
        result.isValid = false;
        result.errorMessage = tr("OriginalKey contains duplicate keys.");
        return result;
    }

    if (orikeylist.size() > 1) {
        // Check if any key is a special key
        for (const QString &orikey : std::as_const(orikeylist)) {
            if (QKeyMapper_Worker::SpecialOriginalKeysList.contains(orikey)) {
                result.isValid = false;
                result.errorMessage = tr("Oricombinationkey contains specialkey \"%1\"").arg(orikey);
                return result;
            }
        }

        // Validate each individual key in the combination
        for (const QString &orikey : std::as_const(orikeylist)) {
            result = validateSingleOriginalKeyWithoutTimeSuffix(orikey, -1);
            if (!result.isValid) {
                return result;
            }
        }

        // Check for duplicate combination key
        if (result.isValid && update_rowindex >= 0) {
            int findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(originalkeystr);

            if (findindex != -1 && findindex != update_rowindex) {
                result.isValid = false;
                result.errorMessage = tr("Duplicate original key \"%1\"").arg(originalkeystr);
            }
        }
    }
    else {
        const QString orikey = orikeylist.constFirst();

        if (0 <= update_rowindex && update_rowindex < QKeyMapper::KeyMappingDataList->size()) {
            QString orikey_noindex = orikey;
            QStringList mappingkeys = QKeyMapper::KeyMappingDataList->at(update_rowindex).Mapping_Keys;

            if (mappingkeys.size() == 1
                && false == mappingkeys.constFirst().contains(SEPARATOR_PLUS)) {
                QString mapkey_noindex = mappingkeys.constFirst();
                orikey_noindex.remove(removeindex_regex);
                mapkey_noindex.remove(removeindex_regex);

                if (QKeyMapper_Worker::SpecialOriginalKeysList.contains(mapkey_noindex)
                    && mapkey_noindex != orikey_noindex) {
                    result.isValid = false;
                    result.errorMessage = tr("Originalkey \"%1\" does not match special mappingkey \"%2\"").arg(orikey_noindex, mapkey_noindex);
                    return result;
                }
            }
        }

        result = validateSingleOriginalKeyWithoutTimeSuffix(orikey, update_rowindex);
        if (!result.isValid) {
            return result;
        }
    }

    // Validate time suffix if it exists
    bool isLongPress = false;
    bool isDoublePress = false;
    if (!longPressTimeString.isEmpty() || !doublePressTimeString.isEmpty()) {
        isLongPress = !longPressTimeString.isEmpty();
        isDoublePress = !doublePressTimeString.isEmpty();

        if (isLongPress || isDoublePress) {
            bool ok;
            int pressTime = isLongPress ? longPressTimeString.toInt(&ok) : doublePressTimeString.toInt(&ok);

            if (!ok || pressTime <= PRESSTIME_MIN || pressTime > PRESSTIME_MAX || (isLongPress && longPressTimeString.startsWith('0')) || (isDoublePress && doublePressTimeString.startsWith('0'))) {
                result.isValid = false;
                result.errorMessage = tr("Invalid press time \"%1\"").arg(isLongPress ? longPressTimeString : doublePressTimeString);
                return result;
            }
        } else {
            result.isValid = false;
            result.errorMessage = tr("Invalid time suffix \"%1\"").arg(isLongPress ? longPressTimeString : doublePressTimeString);
            return result;
        }
    }

    if (0 <= update_rowindex && update_rowindex < QKeyMapper::KeyMappingDataList->size()) {
        QStringList mappingkeys = QKeyMapper::KeyMappingDataList->at(update_rowindex).Mapping_Keys;

        if (mappingkeys.size() == 1 && mappingkeys.constFirst() == KEY_BLOCKED_STR) {
            if (originalkeystr.contains(JOY_KEY_PREFIX)) {
                result.isValid = false;
                result.errorMessage = tr("Game controller keys could not be blocked!");
                return result;
            }
            else if (isLongPress || isDoublePress) {
                result.isValid = false;
                result.errorMessage = tr("Could not block original key with time suffix!");
                return result;
            }
        }
    }

    return result;
}

ValidationResult QKeyMapper::validateSingleOriginalKey(const QString &orikey, int update_rowindex)
{
    ValidationResult result;
    result.isValid = true;

    // Regular expression to validate original key
    static QRegularExpression key_regex(R"(^(.+?)(?:@([0-9]))?(?:⏲(\d+)|✖(\d+))?$)");

    QRegularExpressionMatch key_match = key_regex.match(orikey);

    if (!key_match.hasMatch()) {
        result.isValid = false;
        result.errorMessage = tr("Invalid key format \"%1\"").arg(orikey);
        return result;
    }

    QString original_key = key_match.captured(1);
    QString indexString = key_match.captured(2);
    QString longPressTimeString = key_match.captured(3);
    QString doublePressTimeString = key_match.captured(4);

    bool isLongPress = !longPressTimeString.isEmpty();
    bool isDoublePress = !doublePressTimeString.isEmpty();
    bool validKey = QItemSetupDialog::s_valiedOriginalKeyList.contains(original_key);

    if (!validKey) {
        // Separate index and time suffixes
        QString keyWithoutSuffix = original_key;

        // Remove index suffix if it exists
        if (!indexString.isEmpty()) {
            keyWithoutSuffix = keyWithoutSuffix + "@" + indexString;
        }

        // Check again without suffixes
        validKey = QItemSetupDialog::s_valiedOriginalKeyList.contains(keyWithoutSuffix);

        if (!validKey) {
            result.isValid = false;
            result.errorMessage = tr("Invalid key \"%1\"").arg(original_key);
        }
    }

    if (validKey) {
        if (isLongPress || isDoublePress) {
            bool ok;
            int pressTime = isLongPress ? longPressTimeString.toInt(&ok) : doublePressTimeString.toInt(&ok);

            if (!ok || pressTime <= PRESSTIME_MIN || pressTime > PRESSTIME_MAX || (isLongPress && longPressTimeString.startsWith('0')) || (isDoublePress && doublePressTimeString.startsWith('0'))) {
                result.isValid = false;
                result.errorMessage = tr("Invalid press time \"%1\"").arg(isLongPress ? longPressTimeString : doublePressTimeString);
            }
        }

        if (result.isValid && update_rowindex >= 0) {
            int findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(orikey);
            if (findindex != -1 && findindex != update_rowindex) {
                result.isValid = false;
                result.errorMessage = tr("Duplicate key \"%1\"").arg(orikey);
            }
        }
    }

    return result;
}

ValidationResult QKeyMapper::validateSingleOriginalKeyWithoutTimeSuffix(const QString &orikey, int update_rowindex)
{
    ValidationResult result;
    result.isValid = true;

    // Regular expression to validate original key
    static QRegularExpression key_regex(R"(^(.+?)(?:@([0-9]))?$)");

    QRegularExpressionMatch key_match = key_regex.match(orikey);

    if (!key_match.hasMatch()) {
        result.isValid = false;
        result.errorMessage = tr("Invalid key format \"%1\"").arg(orikey);
        return result;
    }

    QString original_key = key_match.captured(1);
    QString indexString = key_match.captured(2);

    bool validKey = QItemSetupDialog::s_valiedOriginalKeyList.contains(original_key);

    if (!validKey) {
        // Separate index and time suffixes
        QString keyWithoutSuffix = original_key;

        // Remove index suffix if it exists
        if (!indexString.isEmpty()) {
            keyWithoutSuffix = keyWithoutSuffix + "@" + indexString;
        }

        // Check again without suffixes
        validKey = QItemSetupDialog::s_valiedOriginalKeyList.contains(keyWithoutSuffix);

        if (!validKey) {
            result.isValid = false;
            result.errorMessage = tr("Invalid key \"%1\"").arg(original_key);
        }
    }

    if (validKey) {
        if (original_key == JOY_GYRO2MOUSE_STR && !indexString.isEmpty()) {
            result.isValid = false;
            result.errorMessage = tr("Invalid key format \"%1\", do not add Player suffix to Joy-Gyro2Mouse.").arg(orikey);
            return result;
        }
    }

    if (validKey) {
        if (result.isValid && update_rowindex >= 0) {
            int findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(orikey);
            if (findindex != -1 && findindex != update_rowindex) {
                result.isValid = false;
                result.errorMessage = tr("Duplicate key \"%1\"").arg(orikey);
            }
        }
    }

    return result;
}

ValidationResult QKeyMapper::validateMappingKeyString(const QString &mappingkeystr, const QStringList &mappingkeyseqlist, int update_rowindex)
{
    Q_UNUSED(mappingkeystr);
    ValidationResult result;
    result.isValid = true;

    if (mappingkeyseqlist.isEmpty()) {
        result.isValid = false;
        result.errorMessage = tr("MappingKeys is empty.");
        return result;
    }
    else if (mappingkeyseqlist.size() >= KEY_SEQUENCE_MAX) {
        result.isValid = false;
        result.errorMessage = tr("Mapping key sequence exceeds the maximum length!");
        return result;
    }
    else if (mappingkeyseqlist.size() > 1 && mappingkeystr.contains(PREFIX_SEND_EXCLUSION)) {
        result.isValid = false;
        result.errorMessage = tr("After override prefix \"%1\" could not be used in keysequence").arg(PREFIX_SEND_EXCLUSION);
        return result;
    }

    static QRegularExpression removeindex_regex("@\\d$");

    for (const QString& mappingkeys : mappingkeyseqlist) {
        result.isValid = true;
        QStringList Mapping_Keys = splitMappingKeyString(mappingkeys, SPLIT_WITH_PLUS);

        int duplicatesRemoved = Mapping_Keys.removeDuplicates();
        if (duplicatesRemoved > 0) {
            result.isValid = false;
            result.errorMessage = tr("MappingKeys contains duplicate key \"%1\"").arg(mappingkeys);
            return result;
        }

        QStringList pure_Mapping_Keys = splitMappingKeyString(mappingkeys, SPLIT_WITH_PLUS, true);
        int pure_duplicatesRemoved = pure_Mapping_Keys.removeDuplicates();
        if (pure_duplicatesRemoved > 0) {
            result.isValid = false;
            result.errorMessage = tr("MappingKeys contains duplicate key \"%1\"").arg(mappingkeys);
            return result;
        }

        if (Mapping_Keys.size() > 1) {
            QString foundSpecialOriginalKey;
            QString foundSpecialMappingKey;
            // Check Mapping_Keys contains keystring in QKeyMapper_Worker::SpecialMappingKeysList
            for (const QString& mapkey : Mapping_Keys) {
                QString mapkey_noindex = mapkey;
                mapkey_noindex.remove(removeindex_regex);
                if (QKeyMapper_Worker::SpecialMappingKeysList.contains(mapkey)) {
                    foundSpecialMappingKey = mapkey;
                    break;
                }
                if (QKeyMapper_Worker::SpecialOriginalKeysList.contains(mapkey_noindex)) {
                    foundSpecialOriginalKey = mapkey;
                    break;
                }
            }
            if (!foundSpecialMappingKey.isEmpty()) {
                result.isValid = false;
                result.errorMessage = tr("MappingKeys contains specialkey \"%1\"").arg(foundSpecialMappingKey);
                return result;
            }
            if (!foundSpecialOriginalKey.isEmpty()) {
                result.isValid = false;
                result.errorMessage = tr("MappingCombinationKeys contains specialkey \"%1\"").arg(foundSpecialOriginalKey);
                return result;
            }

            for (const QString& mapkey : Mapping_Keys)
            {
                result = validateSingleMappingKey(mapkey);
                if (result.isValid == false) {
                    return result;
                }
            }
        }
        else {
            const QString mapkey = Mapping_Keys.constFirst();

            if (0 <= update_rowindex && update_rowindex < QKeyMapper::KeyMappingDataList->size()) {
                QString orikey_noindex = QKeyMapper::KeyMappingDataList->at(update_rowindex).Original_Key;
                QString mapkey_noindex = mapkey;

                if (false == orikey_noindex.contains(SEPARATOR_PLUS)) {
                    orikey_noindex.remove(removeindex_regex);
                    mapkey_noindex.remove(removeindex_regex);

                    if (QKeyMapper_Worker::SpecialOriginalKeysList.contains(orikey_noindex)
                        && mapkey_noindex != orikey_noindex) {
                        result.isValid = false;
                        result.errorMessage = tr("Mappingkey \"%1\" does not match special originalkey \"%2\"").arg(mapkey_noindex, orikey_noindex);
                        return result;
                    }
                }
            }

            result = validateSingleMappingKey(mapkey);
            if (result.isValid == false) {
                return result;
            }
        }
    }

    if (0 <= update_rowindex && update_rowindex < QKeyMapper::KeyMappingDataList->size()) {
        QString originalkeystr = QKeyMapper::KeyMappingDataList->at(update_rowindex).Original_Key;

        if (mappingkeystr == KEY_BLOCKED_STR) {
            if (originalkeystr.contains(JOY_KEY_PREFIX)) {
                result.isValid = false;
                result.errorMessage = tr("Game controller keys could not be blocked!");
                return result;
            }
            else if (originalkeystr.contains(SEPARATOR_LONGPRESS) || originalkeystr.contains(SEPARATOR_DOUBLEPRESS)) {
                result.isValid = false;
                result.errorMessage = tr("Could not block original key with time suffix!");
                return result;
            }
        }
    }

    return result;
}

ValidationResult QKeyMapper::validateSingleMappingKey(const QString &mapkey)
{
    ValidationResult result;
    result.isValid = true;

    static QRegularExpression mapkey_regex(R"(^([↓↑⇵！]?)([^⏱]+)(?:⏱(\d+))?$)");

    QRegularExpressionMatch mapkey_match = mapkey_regex.match(mapkey);
    if (mapkey_match.hasMatch()) {
        QString prefix = mapkey_match.captured(1);
        QString mapping_key = mapkey_match.captured(2);
        QString waitTimeString = mapkey_match.captured(3);
        bool ok = true;
        int waittime = 0;
        Q_UNUSED(prefix);

        if (!waitTimeString.isEmpty()) {
            waittime = waitTimeString.toInt(&ok);
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[validateSingleMappingKey]" << "Prefix(" << prefix << "), Time(" << waittime << ") -> " << mapkey;
#endif
        bool deprecated_compatible = false;
        if (QKeyMapper::getInstance()->loadSetting_flag && mapping_key == MOUSE2VJOY_DIRECT_KEY_STR_DEPRECATED) {
            deprecated_compatible = true;
        }

        if (!QItemSetupDialog::s_valiedMappingKeyList.contains(mapping_key)
            && !deprecated_compatible) {
            static QRegularExpression vjoy_regex("^(vJoy-[^@]+)(?:@([0-3]))?$");
            static QRegularExpression joy2vjoy_mapkey_regex(R"(^(Joy-(LS|RS|Key11\(LT\)|Key12\(RT\))_2vJoy(LS|RS|LT|RT))(?:@([0-3]))?$)");
            static QRegularExpression mousepoint_regex(R"(^Mouse-(L|R|M|X1|X2|Move)(:W)?(:BG)?\((\d+),(\d+)\)$)");
            static QRegularExpression sendtext_regex(R"(^SendText\((.+)\)$)"); // RegularExpression to match "SendText(string)"
            QRegularExpressionMatch vjoy_match = vjoy_regex.match(mapping_key);
            QRegularExpressionMatch joy2vjoy_mapkey_match = joy2vjoy_mapkey_regex.match(mapping_key);
            QRegularExpressionMatch mousepoint_match = mousepoint_regex.match(mapping_key);
            QRegularExpressionMatch sendtext_match = sendtext_regex.match(mapping_key);

            if (vjoy_match.hasMatch()) {
                static QRegularExpression vjoy_keys_regex("^vJoy-.+$");
                // static QRegularExpression vjoy_pushlevel_keys_regex(R"(^vJoy-(Key11\(LT\)|Key12\(RT\)|LS-(Up|Down|Left|Right)|RS-(Up|Down|Left|Right))(?:\[(\d{1,3})\])?$)");
                static QRegularExpression vjoy_pushlevel_keys_regex(R"(^vJoy-(Key11\(LT\)|Key12\(RT\)|(?:LS|RS)-(?:Up|Down|Left|Right|Radius))(?:\[(\d{1,3})\])?$)");
                QStringList vJoyKeyList = QItemSetupDialog::s_valiedMappingKeyList.filter(vjoy_keys_regex);
                QString vjoy_key = vjoy_match.captured(1);
                QRegularExpressionMatch vjoy_pushlevel_keys_match = vjoy_pushlevel_keys_regex.match(vjoy_key);

                if (vjoy_pushlevel_keys_match.hasMatch()) {
                    QString pushlevelString = vjoy_pushlevel_keys_match.captured(2);
                    if (!pushlevelString.isEmpty()) {
                        bool ok = true;
                        int pushlevel = 0;
                        pushlevel = pushlevelString.toInt(&ok);
                        if (!ok || pushlevelString == "0" || pushlevelString.startsWith('0') || pushlevel <= VJOY_PUSHLEVEL_MIN || pushlevel >= VJOY_PUSHLEVEL_MAX) {
                            result.isValid = false;
                            result.errorMessage = tr("Invalid pushlevel[%1] of vJoy-Key \"%2\", valid range 1~254").arg(pushlevelString, mapping_key);
                        }
                    }
                }
                else if (!vJoyKeyList.contains(vjoy_key)) {
                    result.isValid = false;
                    result.errorMessage = tr("Invalid vJoy-Key \"%1\"").arg(mapping_key);
                }
            }
            else if (joy2vjoy_mapkey_match.hasMatch()) {
                result.isValid = true;
            }
            else if (mousepoint_match.hasMatch() || sendtext_match.hasMatch()) {
                result.isValid = true;
            }
            else {
                result.isValid = false;
                result.errorMessage = tr("Invalid key \"%1\"").arg(mapping_key);
            }
        }

        if (result.isValid && !waitTimeString.isEmpty()) {
            if (!ok || waitTimeString == "0" || waitTimeString.startsWith('0') || waittime <= MAPPING_WAITTIME_MIN || waittime > MAPPING_WAITTIME_MAX) {
                result.isValid = false;
                result.errorMessage = tr("Invalid waittime \"%1\"").arg(waitTimeString);
            }
        }
    }
    else {
        result.isValid = false;
        result.errorMessage = tr("Invalid format \"%1\"").arg(mapkey);
    }

    return result;
}

bool QKeyMapper::checkOriginalkeyStr(const QString &originalkeystr)
{
    ValidationResult result = QKeyMapper::validateOriginalKeyString(originalkeystr, -1);

    if (result.isValid) {
        return true;
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().quote() << "\033[1;31m[checkOriginalkeyStr]" << "Invalid OriginalKey: " << originalkeystr << ", " << "errorMessage: " << result.errorMessage << "\033[0m";
#endif
        return false;
    }
}

bool QKeyMapper::checkMappingkeyStr(QString &mappingkeystr)
{
#ifdef MOUSEBUTTON_CONVERT
    QStringList mouseNameConvertList = QKeyMapper_Worker::MouseButtonNameConvertMap.keys();
    for (const QString &mousekey : std::as_const(mouseNameConvertList)){
        mappingkeystr.replace(mousekey, QKeyMapper_Worker::MouseButtonNameConvertMap.value(mousekey));
    }
#endif
#ifdef SEPARATOR_CONVERT
    mappingkeystr.replace(OLD_SEPARATOR_PLUS, SEPARATOR_PLUS);
    mappingkeystr.replace(OLD_SEPARATOR_NEXTARROW, SEPARATOR_NEXTARROW);
#endif

    QStringList mappingKeySeqList = splitMappingKeyString(mappingkeystr, SPLIT_WITH_NEXT);
    ValidationResult result = QKeyMapper::validateMappingKeyString(mappingkeystr, mappingKeySeqList, INITIAL_ROW_INDEX);

    if (result.isValid) {
        return true;
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "\033[1;31m[checkMappingkeyStr]" << "Invalid MappingKey: " << mappingkeystr << ", " << "errorMessage: " << result.errorMessage << "\033[0m";
#endif
        return false;
    }
}

void QKeyMapper::collectMappingTableTabHotkeys()
{
    s_MappingTableTabHotkeyMap.clear();

    // Iterate over s_KeyMappingTabInfoList
    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index)
    {
        // Get the current TabHotkey
        QString tabHotkey = s_KeyMappingTabInfoList.at(index).TabHotkey;

        // Skip if TabHotkey is empty
        if (tabHotkey.isEmpty()) {
            continue;
        }

        // Check if it starts with PREFIX_PASSTHROUGH, if so, remove the prefix
        if (tabHotkey.startsWith(PREFIX_PASSTHROUGH)) {
            tabHotkey.remove(0, 1);
        }

        // Check if s_MappingTableTabHotkeyMap contains the TabHotkey as a key
        if (!s_MappingTableTabHotkeyMap.contains(tabHotkey)) {
            // If not, add a new key and initialize its value (QList<int>)
            s_MappingTableTabHotkeyMap[tabHotkey] = QList<int>();
        }

        // Append the current index to the QList<int> corresponding to the key
        s_MappingTableTabHotkeyMap[tabHotkey].append(index);

        // Sort the QList<int> to ensure indices are in ascending order
        std::sort(s_MappingTableTabHotkeyMap[tabHotkey].begin(), s_MappingTableTabHotkeyMap[tabHotkey].end());
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[collectMappingTableTabHotkeys]" << "s_MappingTableTabHotkeyMap ->" << s_MappingTableTabHotkeyMap;
#endif
}

QString QKeyMapper::getOriginalKeyStringWithoutSuffix(const QString &originalkeystr)
{
    static QRegularExpression full_orikey_regex(R"(^(.+?)(?:⏲(\d+)|✖(\d+))?$)");
    QString orikey_without_suffix;

    QRegularExpressionMatch full_orikey_match = full_orikey_regex.match(originalkeystr);
    if (full_orikey_match.hasMatch()) {
        orikey_without_suffix = full_orikey_match.captured(1);
    }

    return orikey_without_suffix;
}

void QKeyMapper::copyStringToClipboard(const QString &string)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(string);
}

bool QKeyMapper::backupFile(const QString &sourceFile, const QString &backupFile)
{
    QFile source(sourceFile);
    if (!source.exists()) {
#ifdef DEBUG_LOGOUT_ON
        QString debugmessage = QString("[QKeyMapper::backupFile] Source file does not exist: %1").arg(sourceFile);
        qDebug().noquote() << debugmessage;
#endif
        return false;
    }

    QFile targetFile(backupFile);
    if (targetFile.exists()) {
        targetFile.remove();
    }

    QDir backupDir = QFileInfo(backupFile).absoluteDir();
    if (!backupDir.exists()) {
        if (!backupDir.mkpath(".")) {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("[QKeyMapper::backupFile] Failed to create backup directory: %1").arg(backupDir.absolutePath());
            qDebug().noquote() << debugmessage;
#endif
            return false;
        }
    }

    return QFile::copy(sourceFile, backupFile);
}

QString QKeyMapper::escapeSendTextForSaving(const QString &text)
{
    QString result = text;
    result.replace("#", "░");
    result.replace("|", "┋");
    return result;
}

QString QKeyMapper::unescapeSendTextForLoading(const QString &text)
{
    QString result = text;
    result.replace("░", "#");
    result.replace("┋", "|");
    return result;
}

#if 0
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

                    if (!ProcessInfo.WindowIcon.isNull()){
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
#endif

#if 1
void QKeyMapper::DrawMousePoints(HWND hwnd, HDC hdc, int showMode)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[DrawMousePoints] Show Mode = " << (showMode == SHOW_MODE_WINDOW_MOUSEPOINTS ? "SHOW_MODE_WINDOW_MOUSEPOINTS" : "SHOW_MODE_SCREEN_MOUSEPOINTS");
#endif

    Q_UNUSED(hwnd);
    const QList<MousePoint_Info>& MousePointsList = (showMode == SHOW_MODE_WINDOW_MOUSEPOINTS) ? WindowMousePointsList : ScreenMousePointsList;

    if (MousePointsList.isEmpty()) {
        return;
    }

    for (const MousePoint_Info& pointInfo : MousePointsList) {
        int x = pointInfo.x;
        int y = pointInfo.y;

        if (x < 0 || y < 0) {
            continue;
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[DrawMousePoints]" << pointInfo.map_key << " -> X = " << x << ", Y = " << y;
#endif

        COLORREF color = MOUSE_L_COLOR;
        if (pointInfo.map_key.startsWith(MOUSE_MOVE_PREFIX)) {
            color = MOUSE_MOVE_COLOR;
        } else if (pointInfo.map_key.startsWith(MOUSE_R_STR)) {
            color = MOUSE_R_COLOR;
        } else if (pointInfo.map_key.startsWith(MOUSE_M_STR)) {
            color = MOUSE_M_COLOR;
        } else if (pointInfo.map_key.startsWith(MOUSE_X1_STR)) {
            color = MOUSE_X1_COLOR;
        } else if (pointInfo.map_key.startsWith(MOUSE_X2_STR)) {
            color = MOUSE_X2_COLOR;
        }

        int radius = MOUSE_POINT_RADIUS;

        // Calculate the coordinates of the top-left and bottom-right of the circle
        int left = x - radius;
        int top = y - radius;
        int right = x + radius;
        int bottom = y + radius;

        // Draw the circle
        HBRUSH hBrush = CreateSolidBrush(color);
        HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);
        Ellipse(hdc, left, top, right, bottom);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);
    }
}

#else
void QKeyMapper::DrawMousePoints(HWND hwnd, HDC hdc)
{
    Q_UNUSED(hwnd);
    QString mousepoint_str = getInstance()->ui->pointDisplayLabel->text();

    if (mousepoint_str.isEmpty()) {
        return;
    }

    int x = -1;
    int y = -1;
    QStringList mousepointstrlist = mousepoint_str.split(",");
    if (mousepointstrlist.size() != 2) {
        return;
    }

    QString x_str = mousepointstrlist.at(0).trimmed().remove("X:");
    QString y_str = mousepointstrlist.at(1).trimmed().remove("Y:");

    bool x_ok;
    bool y_ok;
    x = x_str.toInt(&x_ok);
    y = y_str.toInt(&y_ok);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[DrawMousePoints]" << " X = " << x << ", Y = " << y;
#endif

    if (!x_ok || !y_ok || x < 0 || y < 0) {
        return;
    }

    COLORREF color = MOUSE_L_COLOR;
    int radius = MOUSE_POINT_RADIUS;

    // Create a memory DC and a bitmap for double buffering
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    HDC memdc = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
    HGDIOBJ original = SelectObject(memdc, bmp);

    // Copy the current content of the window to the memory DC
    BitBlt(memdc, 0, 0, screenWidth, screenHeight, hdc, 0, 0, SRCCOPY);

    // Calculate the coordinates of the top-left and bottom-right of the circle
    int left = x - radius;
    int top = y - radius;
    int right = x + radius;
    int bottom = y + radius;

    // Draw the circle on the memory DC
    HBRUSH hBrush = CreateSolidBrush(color);
    HGDIOBJ hOldBrush = SelectObject(memdc, hBrush);
    Ellipse(memdc, left, top, right, bottom);
    SelectObject(memdc, hOldBrush);
    DeleteObject(hBrush);

    // Copy the updated memory DC to the window DC
    BitBlt(hdc, 0, 0, screenWidth, screenHeight, memdc, 0, 0, SRCCOPY);

    // Clean up resources
    SelectObject(memdc, original);
    DeleteObject(bmp);
    DeleteDC(memdc);
}
#endif

LRESULT QKeyMapper::MousePointsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int showMode = GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawMousePoints(hwnd, hdc, showMode);
        EndPaint(hwnd, &ps);
        break;
    }
    // case WM_ERASEBKGND:
    // {
    //     HDC hdc = GetDC(hwnd);
    //     clearTransparentWindow(hwnd, hdc);
    //     ReleaseDC(hwnd, hdc);
    //     return 1;
    // }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void QKeyMapper::InitializeGDIPlus()
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_GdiplusToken, &gdiplusStartupInput, NULL);
}

void QKeyMapper::ShutdownGDIPlus()
{
    GdiplusShutdown(m_GdiplusToken);
}

void QKeyMapper::initBlankIconForComboBox()
{
    QPixmap blankPixmap(COMBOBOX_BLANK_ICON_WIDTH, COMBOBOX_BLANK_ICON_HEIGHT);
    blankPixmap.fill(Qt::transparent);
    s_Icon_Blank = QIcon(blankPixmap);
}

HWND QKeyMapper::createTransparentWindow()
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Register window class
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = QKeyMapper::MousePointsWndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszClassName = L"QKeyMapper_TransparentWindow";
    RegisterClass(&wc);

    // Create layered window
    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"QKeyMapper_TransparentWindow",
        NULL,
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        screenWidth,
        screenHeight,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    // Set the opacity of the window (0 = fully transparent, 255 = fully opaque)
    BYTE opacity = 120; // 50% opacity
    SetLayeredWindowAttributes(hwnd, 0, opacity, LWA_ALPHA);

    ShowWindow(hwnd, SW_HIDE);

    // Save the initial width & height of TransparentWindow
    m_TransParentWindowInitialWidth = screenWidth;
    m_TransParentWindowInitialHeight = screenHeight;

    // Initialize the show mode to SHOW_MODE_SCREEN_MOUSEPOINTS
    SetWindowLongPtr(hwnd, GWLP_USERDATA, SHOW_MODE_SCREEN_MOUSEPOINTS);

    return hwnd;
}

void QKeyMapper::resizeTransparentWindow(HWND hwnd, int x, int y, int width, int height)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[resizeTransparentWindow]"<< " Resize TransparentWindow to x:" << x << " y:" << y << " width:" << width << " height:" << height;
#endif
    // Adjust the window size and position
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
}

void QKeyMapper::destroyTransparentWindow(HWND hwnd)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Destroy the window
    DestroyWindow(hwnd);

    // Unregister the window class
    UnregisterClass(L"QKeyMapper_TransparentWindow", hInstance);
}

void QKeyMapper::clearTransparentWindow(HWND hwnd, HDC hdc)
{
    QString mousepoint_str = getInstance()->ui->pointDisplayLabel->text();
    if (mousepoint_str.isEmpty()) {
        return;
    }

    // Get the window area
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    // Create a transparent brush
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
    HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);

    // Use the transparent brush to draw a rectangle to cover the previous content
    Rectangle(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

    // Restore the original brush
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);
}

void QKeyMapper::DrawCrosshair(HWND hwnd, HDC hdc, int showParam)
{
    // Extract the show_mode (high 2 bytes) and rowindex (low 2 bytes) from showParam
    int showMode = (showParam >> 16) & 0xFFFF;  // High 2 bytes for show_mode
    int rowindex = showParam & 0xFFFF;          // Low 2 bytes for rowindex

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[DrawCrosshair] Show Mode = " << (showParam == SHOW_MODE_CROSSHAIR_TYPEA ? "SHOW_MODE_CROSSHAIR_TYPEA" : "SHOW_MODE_CROSSHAIR_NORMAL") << ", Row Index = " << rowindex;
#endif

    if (rowindex < 0 || rowindex >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    MAP_KEYDATA keymapdata = QKeyMapper::KeyMappingDataList->at(rowindex);

    // Get the window width and height
    RECT rect;
    GetClientRect(hwnd, &rect);

    // Create Graphics object
    Graphics graphics(hdc);
    // graphics.SetCompositingMode(CompositingModeSourceOver);
    graphics.SetCompositingQuality(CompositingQualityHighQuality);
    // graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    int centerX = (rect.right - rect.left) / 2;
    int centerY = (rect.bottom - rect.top) / 2;

    // X Offset & Y Offset
    int x_offset = keymapdata.Crosshair_X_Offset;
    int y_offset = keymapdata.Crosshair_Y_Offset;

    centerX += x_offset;
    centerY += y_offset;

    // Draw flags
    bool draw_center = keymapdata.Crosshair_ShowCenter;
    bool draw_top = keymapdata.Crosshair_ShowTop;
    bool draw_bottom = keymapdata.Crosshair_ShowBottom;
    bool draw_left = keymapdata.Crosshair_ShowLeft;
    bool draw_right = keymapdata.Crosshair_ShowRight;

    // Centerdot setting values
    // int centerdot_opacity = keymapdata.Crosshair_CenterOpacity; // Centerdot opacity value
    int centerdot_opacity = CROSSHAIR_OPACITY_MAX; // Centerdot opacity value
    int dotRadius = keymapdata.Crosshair_CenterSize;
    BYTE centerdot_R = keymapdata.Crosshair_CenterColor.red();
    BYTE centerdot_G = keymapdata.Crosshair_CenterColor.green();
    BYTE centerdot_B = keymapdata.Crosshair_CenterColor.blue();

    // Crosshair setting values
    // int crosshair_opacity = keymapdata.Crosshair_CrosshairOpacity; // Crosshair line opacity value
    int crosshair_opacity = CROSSHAIR_OPACITY_MAX; // Crosshair line opacity value
    int lineWidth = keymapdata.Crosshair_CrosshairWidth;   // Line width
    int lineLength = keymapdata.Crosshair_CrosshairLength; // Line length
    BYTE crosshair_R = keymapdata.Crosshair_CrosshairColor.red();
    BYTE crosshair_G = keymapdata.Crosshair_CrosshairColor.green();
    BYTE crosshair_B = keymapdata.Crosshair_CrosshairColor.blue();

    // Calculate half of the line width
    int halfLineWidth = lineWidth / 2;

    if (SHOW_MODE_CROSSHAIR_TYPEA == showMode) {
        // SHOW_MODE_CROSSHAIR_TYPEA Crosshair color and style

        // Crosshair color and style
        Color crossHairColor(crosshair_opacity, crosshair_R, crosshair_G, crosshair_B);

        // Set the pen
        Pen crosshair_pen(crossHairColor, lineWidth);

        if (draw_top) {
            // Draw top line (only the far half)
            graphics.DrawLine(&crosshair_pen, centerX, centerY - halfLineWidth - lineLength, centerX, centerY - halfLineWidth - lineLength + (lineLength / 2));
        }

        if (draw_bottom) {
            // Draw bottom line (only the far half)
            graphics.DrawLine(&crosshair_pen, centerX, centerY + halfLineWidth + (lineLength / 2), centerX, centerY + halfLineWidth + lineLength);
        }

        if (draw_left) {
            // Draw left line (only the far half)
            graphics.DrawLine(&crosshair_pen, centerX - halfLineWidth - lineLength, centerY, centerX - halfLineWidth - lineLength + (lineLength / 2), centerY);
        }

        if (draw_right) {
            // Draw right line (only the far half)
            graphics.DrawLine(&crosshair_pen, centerX + halfLineWidth + (lineLength / 2), centerY, centerX + halfLineWidth + lineLength, centerY);
        }
    }
    else {
        // SHOW_MODE_CROSSHAIR_NORMAL Crosshair color and style

        // Crosshair color and style
        Color crossHairColor(crosshair_opacity, crosshair_R, crosshair_G, crosshair_B);

        // Set the pen
        Pen crosshair_pen(crossHairColor, lineWidth);

        if (draw_top) {
            // Draw top line
            graphics.DrawLine(&crosshair_pen, centerX, centerY - halfLineWidth - lineLength, centerX, centerY - halfLineWidth);
        }

        if (draw_bottom) {
            // Draw bottom line
            graphics.DrawLine(&crosshair_pen, centerX, centerY + halfLineWidth, centerX, centerY + halfLineWidth + lineLength);
        }

        if (draw_left) {
            // Draw left line
            graphics.DrawLine(&crosshair_pen, centerX - halfLineWidth - lineLength, centerY, centerX - halfLineWidth, centerY);
        }

        if (draw_right) {
            // Draw right line
            graphics.DrawLine(&crosshair_pen, centerX + halfLineWidth, centerY, centerX + halfLineWidth + lineLength, centerY);
        }
    }

    if (draw_center) {
        // Draw center dot rectangle
        Color centerDotColor(centerdot_opacity, centerdot_R, centerdot_G, centerdot_B);
        Pen centerdot_pen(centerDotColor, dotRadius * 2);
        graphics.DrawLine(&centerdot_pen, centerX, centerY - dotRadius, centerX, centerY + dotRadius);
    }
}

LRESULT QKeyMapper::CrosshairWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int showParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawCrosshair(hwnd, hdc, showParam);
        EndPaint(hwnd, &ps);
        break;
    }
    // case WM_ERASEBKGND:
    // {
    //     HDC hdc = GetDC(hwnd);
    //     clearCrosshairWindow(hwnd, hdc);
    //     ReleaseDC(hwnd, hdc);
    //     return 1;
    // }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

HWND QKeyMapper::createCrosshairWindow()
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Register window class
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = QKeyMapper::CrosshairWndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszClassName = L"QKeyMapper_CrosshairWindow";
    RegisterClass(&wc);

    // Create layered window
    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"QKeyMapper_CrosshairWindow",
        NULL,
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        screenWidth,
        screenHeight,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    // Pixels with pure black color (RGB value 0, 0, 0) will be treated as transparent
    // SetLayeredWindowAttributes(hwnd, RGB(200, 200, 200), 0, LWA_COLORKEY);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), CROSSHAIR_CROSSHAIROPACITY_DEFAULT, LWA_ALPHA | LWA_COLORKEY);

    ShowWindow(hwnd, SW_HIDE);

    // Save the initial width & height of CrosshairWindow
    m_CrosshairWindowInitialWidth = screenWidth;
    m_CrosshairWindowInitialHeight = screenHeight;

    // Initialize the show mode to SHOW_MODE_CROSSHAIR_NORMAL
    SetWindowLongPtr(hwnd, GWLP_USERDATA, SHOW_MODE_CROSSHAIR_NORMAL);

    return hwnd;
}

void QKeyMapper::resizeCrosshairWindow(HWND hwnd, int x, int y, int width, int height)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[resizeCrosshairWindow]"<< " Resize CrosshairWindow to x:" << x << " y:" << y << " width:" << width << " height:" << height;
#endif
    // Adjust the window size and position
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
}

void QKeyMapper::destroyCrosshairWindow(HWND hwnd)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Destroy the window
    DestroyWindow(hwnd);

    // Unregister the window class
    UnregisterClass(L"QKeyMapper_CrosshairWindow", hInstance);
}

void QKeyMapper::clearCrosshairWindow(HWND hwnd, HDC hdc)
{
    // Get the window area
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    // Create a black brush
    HBRUSH hBrush = CreateSolidBrush(RGB(200, 200, 200));
    HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);

    // Use the black brush to draw a rectangle to cover the previous content
    // Since the window uses layered attributes with black as the transparency key,
    // filling the window with black will clear all previous content by making it transparent
    Rectangle(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

    // Restore the original brush
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);
}

QPoint QKeyMapper::getMousePointFromLabelString(QString &labelstr)
{
    QPoint mousepoint = QPoint(-1, -1);

    if (labelstr.isEmpty()) {
        return mousepoint;
    }

    QStringList mousepointstrlist = labelstr.split(",");
    if (mousepointstrlist.size() != 2) {
        return mousepoint;
    }

    QString x_str = mousepointstrlist.at(0).trimmed().remove("X:");
    QString y_str = mousepointstrlist.at(1).trimmed().remove("Y:");

    bool x_ok;
    bool y_ok;
    int x = x_str.toInt(&x_ok);
    int y = y_str.toInt(&y_ok);

    if (x_ok && y_ok && x >= 0 && y >= 0) {
        mousepoint.rx() = x;
        mousepoint.ry() = y;
    }

    return mousepoint;
}

bool QKeyMapper::getStartupMinimizedStatus()
{
    if (true == getInstance()->ui->startupMinimizedCheckBox->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}

int QKeyMapper::getLanguageIndex()
{
    return getInstance()->ui->languageComboBox->currentIndex();
}

int QKeyMapper::getCurrentSettingSelectIndex()
{
    return getInstance()->ui->settingselectComboBox->currentIndex();
}

bool QKeyMapper::getKeyMappingDataTableItemBurstStatus(int rowindex)
{
    QTableWidgetItem* burstItem = getInstance()->m_KeyMappingDataTable->item(rowindex, BURST_MODE_COLUMN);
    bool isBurstItemEnabled = burstItem && (burstItem->flags() & Qt::ItemIsEnabled);

    return isBurstItemEnabled;
}

bool QKeyMapper::getKeyMappingDataTableItemLockStatus(int rowindex)
{
    QTableWidgetItem* lockItem = getInstance()->m_KeyMappingDataTable->item(rowindex, LOCK_COLUMN);
    bool isLockItemEnabled = lockItem && (lockItem->flags() & Qt::ItemIsEnabled);

    return isLockItemEnabled;
}

QString QKeyMapper::getCurrentOriKeyText()
{
    return getInstance()->m_orikeyComboBox->currentText();
}

QString QKeyMapper::getCurrentOriCombinationKeyText()
{
    return getInstance()->ui->combinationKeyLineEdit->text();
}

QString QKeyMapper::getSendTextString()
{
    return getInstance()->ui->sendTextLineEdit->text();
}

void QKeyMapper::setCurrentOriCombinationKeyText(const QString &newcombinationkeytext)
{
    return getInstance()->ui->combinationKeyLineEdit->setText(newcombinationkeytext);
}

int QKeyMapper::getMatchProcessNameIndex()
{
    return getInstance()->ui->checkProcessComboBox->currentIndex();
}

int QKeyMapper::getMatchWindowTitleIndex()
{
    return getInstance()->ui->checkWindowTitleComboBox->currentIndex();
}

#if 0
bool QKeyMapper::getDisableWinKeyStatus()
{
    if (s_isDestructing) {
        return false;
    }

    if (true == getInstance()->ui->disableWinKeyCheckBox->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}
#endif

#if 0
int QKeyMapper::getBurstPressTime()
{
    return getInstance()->ui->burstpressSpinBox->value();
}

int QKeyMapper::getBurstReleaseTime()
{
    return getInstance()->ui->burstreleaseSpinBox->value();
}
#endif

int QKeyMapper::getJoystick2MouseSpeedX()
{
    return getInstance()->ui->mouseXSpeedSpinBox->value();
}

int QKeyMapper::getJoystick2MouseSpeedY()
{
    return getInstance()->ui->mouseYSpeedSpinBox->value();
}

double QKeyMapper::getGyro2MouseXSpeed()
{
    return getInstance()->ui->Gyro2MouseXSpeedSpinBox->value();
}

double QKeyMapper::getGyro2MouseYSpeed()
{
    return getInstance()->ui->Gyro2MouseYSpeedSpinBox->value();
}

double QKeyMapper::getGyro2MouseMinThreshold()
{
    return getInstance()->ui->Gyro2MouseMinThresholdSpinBox->value();
}

double QKeyMapper::getGyro2MouseMaxThreshold()
{
    return getInstance()->ui->Gyro2MouseMaxThresholdSpinBox->value();
}

double QKeyMapper::getGyro2MouseMinXSensitivity()
{
    return getInstance()->ui->Gyro2MouseMinXSensSpinBox->value();
}

double QKeyMapper::getGyro2MouseMinYSensitivity()
{
    return getInstance()->ui->Gyro2MouseMinYSensSpinBox->value();
}

double QKeyMapper::getGyro2MouseMaxXSensitivity()
{
    return getInstance()->ui->Gyro2MouseMaxXSensSpinBox->value();
}

double QKeyMapper::getGyro2MouseMaxYSensitivity()
{
    return getInstance()->ui->Gyro2MouseMaxYSensSpinBox->value();
}

int QKeyMapper::getvJoyXSensitivity()
{
    return getInstance()->ui->vJoyXSensSpinBox->value();
}

int QKeyMapper::getvJoyYSensitivity()
{
    return getInstance()->ui->vJoyYSensSpinBox->value();
}

int QKeyMapper::getvJoyRecenterTimeout()
{
    return getInstance()->ui->vJoyRecenterSpinBox->value();
}

QString QKeyMapper::getVirtualGamepadType()
{
    return getInstance()->ui->virtualGamepadTypeComboBox->currentText();
}

bool QKeyMapper::getvJoyLockCursorStatus()
{
    return getInstance()->ui->lockCursorCheckBox->isChecked();
}

bool QKeyMapper::getvJoyDirectModeStatus()
{
    return getInstance()->ui->directModeCheckBox->isChecked();
}

int QKeyMapper::getGlobalSettingAutoStart()
{
    return s_GlobalSettingAutoStart;
}

bool QKeyMapper::checkGlobalSettingAutoStart()
{
    // if (s_GlobalSettingAutoStart == Qt::Checked && false == KeyMappingDataListGlobal.isEmpty()) {
    if (s_GlobalSettingAutoStart == Qt::Checked) {
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

bool QKeyMapper::getSendToSameTitleWindowsStatus()
{
    if (false == getInstance()->ui->autoStartMappingCheckBox->isChecked()
        && true == getInstance()->ui->sendToSameTitleWindowsCheckBox->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}

bool QKeyMapper::getAcceptVirtualGamepadInputStatus()
{
    if (true == getInstance()->ui->acceptVirtualGamepadInputCheckBox->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}

bool QKeyMapper::getProcessIconAsTrayIconStatus()
{
    return getInstance()->ui->ProcessIconAsTrayIconCheckBox->isChecked();
}

bool QKeyMapper::isTabTextDuplicate(const QString &tabName)
{
    // Iterate through tabinfolist to check if there is a duplicate tabname
    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index) {
        if (s_KeyMappingTabInfoList.at(index).TabName == tabName) {
            return true;
        }
    }
    return false;
}

bool QKeyMapper::isTabTextDuplicateInStringList(const QString &tabName, const QStringList &tabNameList)
{
    for (const QString &tab_name : tabNameList) {
        if (tab_name == tabName) {
            return true;
        }
    }
    return false;
}

bool QKeyMapper::validateCombinationKey(QString &input)
{
    bool isvalid = true;

    QStringList keylist = input.split(SEPARATOR_PLUS);
    if (keylist.isEmpty())
    {
        isvalid = false;
    }
    else
    {
        for (const QString& key : keylist)
        {
            if (!QKeyMapper_Worker::CombinationKeysList.contains(key))
            {
                isvalid = false;
                break;
            }
        }

        if (isvalid) {
            // Check for duplicate keys
            int numRemoved = keylist.removeDuplicates();
            if (numRemoved > 0) {
                isvalid = false;
            }
        }
    }

    return isvalid;
}

int QKeyMapper::tabIndexToSwitchByTabHotkey(const QString &hotkey_string, bool *isSame)
{
    int tabindex_toswitch = -1;
    if (isSame) *isSame = false;
    QList<int> tabindex_list = s_MappingTableTabHotkeyMap.value(hotkey_string);
    if (!tabindex_list.isEmpty()) {
        if (tabindex_list.size() == 1){
            if (tabindex_list.at(0) != s_KeyMappingTabWidgetCurrentIndex) {
                tabindex_toswitch = tabindex_list.at(0);
            }
            else {
                if (isSame) *isSame = true;
            }
        }
        else {
            if (tabindex_list.contains(s_KeyMappingTabWidgetCurrentIndex)) {
                int index = tabindex_list.indexOf(s_KeyMappingTabWidgetCurrentIndex);
                if (index == tabindex_list.size() - 1) {
                    tabindex_toswitch = tabindex_list.at(0);
                }
                else {
                    tabindex_toswitch = tabindex_list.at(index + 1);
                }
            }
            else {
                tabindex_toswitch = tabindex_list.at(0);
            }
        }
    }

    return tabindex_toswitch;
}

bool QKeyMapper::exportKeyMappingDataToFile(int tabindex, const QString &filename)
{
    if (tabindex < 0 || tabindex >= s_KeyMappingTabInfoList.size()) {
        return false;
    }

    if (filename.isEmpty()) {
        return false;
    }

    QList<MAP_KEYDATA> *mappingDataList = s_KeyMappingTabInfoList.at(tabindex).KeyMappingData;

    if (mappingDataList->isEmpty()) {
        return false;
    }

    QSettings keyMappingDataFile(filename, QSettings::IniFormat);
    QStringList original_keys;
    QStringList mapping_keysList;
    QStringList mappingkeys_keyupList;
    QStringList notesList;
    QStringList categorysList;
    QStringList burstList;
    QStringList burstpresstimeList;
    QStringList burstreleasetimeList;
    QStringList lockList;
    QStringList mappingkeyunlockList;
    QStringList postmappingkeyList;
    QStringList fixedvkeycodeList;
    QStringList checkcombkeyorderList;
    QStringList unbreakableList;
    QStringList passthroughList;
    QStringList sendtimingList;
    QStringList keyseqholddownList;
    QStringList repeatmodeList;
    QStringList repeattimesList;
    QStringList crosshair_centercolorList;
    QStringList crosshair_centersizeList;
    QStringList crosshair_centeropacityList;
    QStringList crosshair_crosshaircolorList;
    QStringList crosshair_crosshairwidthList;
    QStringList crosshair_crosshairlengthList;
    QStringList crosshair_crosshairopacityList;
    QStringList crosshair_showcenterList;
    QStringList crosshair_showtopList;
    QStringList crosshair_showbottomList;
    QStringList crosshair_showleftList;
    QStringList crosshair_showrightList;
    QStringList crosshair_x_offsetList;
    QStringList crosshair_y_offsetList;

    for (const MAP_KEYDATA &keymapdata : std::as_const(*mappingDataList))
    {
        original_keys << keymapdata.Original_Key;
        QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
        mapping_keysList  << mappingkeys_str;
        QString mappingkeys_keyup_str = keymapdata.MappingKeys_KeyUp.join(SEPARATOR_NEXTARROW);
        if (mappingkeys_keyup_str.isEmpty()) {
            mappingkeys_keyupList << mappingkeys_str;
        }
        else {
            mappingkeys_keyupList << mappingkeys_keyup_str;
        }
        notesList << keymapdata.Note;
        categorysList << keymapdata.Category;
        if (true == keymapdata.Burst) {
            burstList.append("ON");
        }
        else {
            burstList.append("OFF");
        }
        if (BURST_TIME_MIN <= keymapdata.BurstPressTime && keymapdata.BurstPressTime <= BURST_TIME_MAX) {
            burstpresstimeList.append(QString::number(keymapdata.BurstPressTime));
        }
        else {
            burstpresstimeList.append(QString::number(BURST_PRESS_TIME_DEFAULT));
        }
        if (BURST_TIME_MIN <= keymapdata.BurstReleaseTime && keymapdata.BurstReleaseTime <= BURST_TIME_MAX) {
            burstreleasetimeList.append(QString::number(keymapdata.BurstReleaseTime));
        }
        else {
            burstreleasetimeList.append(QString::number(BURST_RELEASE_TIME_DEFAULT));
        }
        if (true == keymapdata.Lock) {
            lockList.append("ON");
        }
        else {
            lockList.append("OFF");
        }
        if (true == keymapdata.MappingKeyUnlock) {
            mappingkeyunlockList.append("ON");
        }
        else {
            mappingkeyunlockList.append("OFF");
        }
        if (true == keymapdata.PostMappingKey) {
            postmappingkeyList.append("ON");
        }
        else {
            postmappingkeyList.append("OFF");
        }
        if (FIXED_VIRTUAL_KEY_CODE_MIN <= keymapdata.FixedVKeyCode && keymapdata.FixedVKeyCode <= FIXED_VIRTUAL_KEY_CODE_MAX) {
            fixedvkeycodeList.append(QString::number(keymapdata.FixedVKeyCode, 16));
        }
        else {
            fixedvkeycodeList.append(QString::number(FIXED_VIRTUAL_KEY_CODE_NONE, 16));
        }
        if (true == keymapdata.CheckCombKeyOrder) {
            checkcombkeyorderList.append("ON");
        }
        else {
            checkcombkeyorderList.append("OFF");
        }
        if (true == keymapdata.Unbreakable) {
            unbreakableList.append("ON");
        }
        else {
            unbreakableList.append("OFF");
        }
        if (true == keymapdata.PassThrough) {
            passthroughList.append("ON");
        }
        else {
            passthroughList.append("OFF");
        }

        if (SENDTIMING_KEYDOWN == keymapdata.SendTiming) {
            sendtimingList.append(SENDTIMING_STR_KEYDOWN);
        }
        else if (SENDTIMING_KEYUP == keymapdata.SendTiming) {
            sendtimingList.append(SENDTIMING_STR_KEYUP);
        }
        else if (SENDTIMING_KEYDOWN_AND_KEYUP == keymapdata.SendTiming) {
            sendtimingList.append(SENDTIMING_STR_KEYDOWN_AND_KEYUP);
        }
        else if (SENDTIMING_NORMAL_AND_KEYUP == keymapdata.SendTiming) {
            sendtimingList.append(SENDTIMING_STR_NORMAL_AND_KEYUP);
        }
        else {
            sendtimingList.append(SENDTIMING_STR_NORMAL);
        }

        if (true == keymapdata.KeySeqHoldDown) {
            keyseqholddownList.append("ON");
        }
        else {
            keyseqholddownList.append("OFF");
        }
        if (REPEAT_MODE_NONE <= keymapdata.RepeatMode && keymapdata.RepeatMode <= REPEAT_MODE_BYTIMES) {
            repeatmodeList.append(QString::number(keymapdata.RepeatMode));
        }
        else {
            repeatmodeList.append(QString::number(REPEAT_MODE_NONE));
        }
        if (REPEAT_TIMES_MIN <= keymapdata.RepeatTimes && keymapdata.RepeatTimes <= REPEAT_TIMES_MAX) {
            repeattimesList.append(QString::number(keymapdata.RepeatTimes));
        }
        else {
            repeattimesList.append(QString::number(REPEAT_TIMES_DEFAULT));
        }
        if (keymapdata.Crosshair_CenterColor.isValid()) {
            crosshair_centercolorList.append(keymapdata.Crosshair_CenterColor.name().remove("#"));
        }
        else {
            crosshair_centercolorList.append(CROSSHAIR_CENTERCOLOR_DEFAULT);
        }
        if (CROSSHAIR_CENTERSIZE_MIN <= keymapdata.Crosshair_CenterSize && keymapdata.Crosshair_CenterSize <= CROSSHAIR_CENTERSIZE_MAX) {
            crosshair_centersizeList.append(QString::number(keymapdata.Crosshair_CenterSize));
        }
        else {
            crosshair_centersizeList.append(QString::number(CROSSHAIR_CENTERSIZE_DEFAULT));
        }
        if (CROSSHAIR_OPACITY_MIN <= keymapdata.Crosshair_CenterOpacity && keymapdata.Crosshair_CenterOpacity <= CROSSHAIR_OPACITY_MAX) {
            crosshair_centeropacityList.append(QString::number(keymapdata.Crosshair_CenterOpacity));
        }
        else {
            crosshair_centeropacityList.append(QString::number(CROSSHAIR_CENTEROPACITY_DEFAULT));
        }
        if (keymapdata.Crosshair_CrosshairColor.isValid()) {
            crosshair_crosshaircolorList.append(keymapdata.Crosshair_CrosshairColor.name().remove("#"));
        }
        else {
            crosshair_crosshaircolorList.append(CROSSHAIR_CROSSHAIRCOLOR_DEFAULT);
        }
        if (CROSSHAIR_CROSSHAIRWIDTH_MIN <= keymapdata.Crosshair_CrosshairWidth && keymapdata.Crosshair_CrosshairWidth <= CROSSHAIR_CROSSHAIRWIDTH_MAX) {
            crosshair_crosshairwidthList.append(QString::number(keymapdata.Crosshair_CrosshairWidth));
        }
        else {
            crosshair_crosshairwidthList.append(QString::number(CROSSHAIR_CROSSHAIRWIDTH_DEFAULT));
        }
        if (CROSSHAIR_CROSSHAIRLENGTH_MIN <= keymapdata.Crosshair_CrosshairLength && keymapdata.Crosshair_CrosshairLength <= CROSSHAIR_CROSSHAIRLENGTH_MAX) {
            crosshair_crosshairlengthList.append(QString::number(keymapdata.Crosshair_CrosshairLength));
        }
        else {
            crosshair_crosshairlengthList.append(QString::number(CROSSHAIR_CROSSHAIRLENGTH_DEFAULT));
        }
        if (CROSSHAIR_OPACITY_MIN <= keymapdata.Crosshair_CrosshairOpacity && keymapdata.Crosshair_CrosshairOpacity <= CROSSHAIR_OPACITY_MAX) {
            crosshair_crosshairopacityList.append(QString::number(keymapdata.Crosshair_CrosshairOpacity));
        }
        else {
            crosshair_crosshairopacityList.append(QString::number(CROSSHAIR_CROSSHAIROPACITY_DEFAULT));
        }
        if (true == keymapdata.Crosshair_ShowCenter) {
            crosshair_showcenterList.append("ON");
        }
        else {
            crosshair_showcenterList.append("OFF");
        }
        if (true == keymapdata.Crosshair_ShowTop) {
            crosshair_showtopList.append("ON");
        }
        else {
            crosshair_showtopList.append("OFF");
        }
        if (true == keymapdata.Crosshair_ShowBottom) {
            crosshair_showbottomList.append("ON");
        }
        else {
            crosshair_showbottomList.append("OFF");
        }
        if (true == keymapdata.Crosshair_ShowLeft) {
            crosshair_showleftList.append("ON");
        }
        else {
            crosshair_showleftList.append("OFF");
        }
        if (true == keymapdata.Crosshair_ShowRight) {
            crosshair_showrightList.append("ON");
        }
        else {
            crosshair_showrightList.append("OFF");
        }
        if (CROSSHAIR_X_OFFSET_MIN <= keymapdata.Crosshair_X_Offset && keymapdata.Crosshair_X_Offset <= CROSSHAIR_X_OFFSET_MAX) {
            crosshair_x_offsetList.append(QString::number(keymapdata.Crosshair_X_Offset));
        }
        else {
            crosshair_x_offsetList.append(QString::number(CROSSHAIR_X_OFFSET_DEFAULT));
        }
        if (CROSSHAIR_Y_OFFSET_MIN <= keymapdata.Crosshair_Y_Offset && keymapdata.Crosshair_Y_Offset <= CROSSHAIR_Y_OFFSET_MAX) {
            crosshair_y_offsetList.append(QString::number(keymapdata.Crosshair_Y_Offset));
        }
        else {
            crosshair_y_offsetList.append(QString::number(CROSSHAIR_Y_OFFSET_DEFAULT));
        }
    }

    keyMappingDataFile.setValue(KEYMAPDATA_ORIGINALKEYS, original_keys );
    keyMappingDataFile.setValue(KEYMAPDATA_MAPPINGKEYS, mapping_keysList);
    keyMappingDataFile.setValue(KEYMAPDATA_MAPPINGKEYS_KEYUP, mappingkeys_keyupList);
    keyMappingDataFile.setValue(KEYMAPDATA_NOTE, notesList);
    keyMappingDataFile.setValue(KEYMAPDATA_CATEGORY, categorysList);
    keyMappingDataFile.setValue(KEYMAPDATA_BURST, burstList);
    keyMappingDataFile.setValue(KEYMAPDATA_BURSTPRESS_TIME, burstpresstimeList);
    keyMappingDataFile.setValue(KEYMAPDATA_BURSTRELEASE_TIME , burstreleasetimeList);
    keyMappingDataFile.setValue(KEYMAPDATA_LOCK, lockList);
    keyMappingDataFile.setValue(KEYMAPDATA_MAPPINGKEYUNLOCK, mappingkeyunlockList);
    keyMappingDataFile.setValue(KEYMAPDATA_POSTMAPPINGKEY, postmappingkeyList);
    keyMappingDataFile.setValue(KEYMAPDATA_FIXEDVKEYCODE, fixedvkeycodeList);
    keyMappingDataFile.setValue(KEYMAPDATA_CHECKCOMBKEYORDER, checkcombkeyorderList);
    keyMappingDataFile.setValue(KEYMAPDATA_UNBREAKABLE, unbreakableList);
    keyMappingDataFile.setValue(KEYMAPDATA_PASSTHROUGH, passthroughList);
    keyMappingDataFile.setValue(KEYMAPDATA_SENDTIMING, sendtimingList);
    keyMappingDataFile.setValue(KEYMAPDATA_KEYSEQHOLDDOWN, keyseqholddownList);
    keyMappingDataFile.setValue(KEYMAPDATA_REPEATMODE, repeatmodeList);
    keyMappingDataFile.setValue(KEYMAPDATA_REPEATIMES, repeattimesList);

    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_CENTERCOLOR, crosshair_centercolorList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_CENTERSIZE, crosshair_centersizeList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_CENTEROPACITY, crosshair_centeropacityList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_CROSSHAIRCOLOR, crosshair_crosshaircolorList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_CROSSHAIRWIDTH, crosshair_crosshairwidthList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_CROSSHAIRLENGTH, crosshair_crosshairlengthList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_CROSSHAIROPACITY, crosshair_crosshairopacityList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_SHOWCENTER, crosshair_showcenterList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_SHOWTOP, crosshair_showtopList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_SHOWBOTTOM, crosshair_showbottomList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_SHOWLEFT, crosshair_showleftList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_SHOWRIGHT, crosshair_showrightList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_X_OFFSET, crosshair_x_offsetList);
    keyMappingDataFile.setValue(KEYMAPDATA_CROSSHAIR_Y_OFFSET, crosshair_y_offsetList);

    return true;
}

bool QKeyMapper::importKeyMappingDataFromFile(int tabindex, const QString &filename)
{
    if (tabindex < 0 || tabindex >= s_KeyMappingTabInfoList.size()) {
        return false;
    }

    if (!QFileInfo::exists(filename)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[importKeyMappingDataFromFile]" << " QFileInfo::exists(" << filename << ") = false";
#endif
        return false;
    }

    QSettings keyMappingDataFile(filename, QSettings::IniFormat);
    QStringList original_keys;
    QStringList mapping_keys;
    QStringList mappingkeys_keyup;
    QStringList burstStringList;
    QStringList burstpressStringList;
    QStringList burstreleaseStringList;
    QStringList lockStringList;
    QStringList mappingkeyunlockStringList;
    QStringList postmappingkeyStringList;
    QStringList fixedvkeycodeStringList;
    QStringList checkcombkeyorderStringList;
    QStringList unbreakableStringList;
    QStringList passthroughStringList;
    QStringList sendtimingStringList;
    QStringList keyseqholddownStringList;
    QStringList repeatmodeStringList;
    QStringList repeattimesStringList;
    QStringList notesList;
    QStringList categorysList;
    QStringList crosshair_centercolorStringList;
    QStringList crosshair_centersizeStringList;
    QStringList crosshair_centeropacityStringList;
    QStringList crosshair_crosshaircolorStringList;
    QStringList crosshair_crosshairwidthStringList;
    QStringList crosshair_crosshairlengthStringList;
    QStringList crosshair_crosshairopacityStringList;
    QStringList crosshair_showcenterStringList;
    QStringList crosshair_showtopStringList;
    QStringList crosshair_showbottomStringList;
    QStringList crosshair_showleftStringList;
    QStringList crosshair_showrightStringList;
    QStringList crosshair_x_offsetStringList;
    QStringList crosshair_y_offsetStringList;
    QList<bool> burstList;
    QList<int> burstpresstimeList;
    QList<int> burstreleasetimeList;
    QList<bool> lockList;
    QList<bool> mappingkeyunlockList;
    QList<bool> postmappingkeyList;
    QList<int> fixedvkeycodeList;
    QList<bool> checkcombkeyorderList;
    QList<bool> unbreakableList;
    QList<bool> passthroughList;
    QList<int> sendtimingList;
    QList<bool> keyseqholddownList;
    QList<int> repeatmodeList;
    QList<int> repeattimesList;
    QList<QColor> crosshair_centercolorList;
    QList<int> crosshair_centersizeList;
    QList<int> crosshair_centeropacityList;
    QList<QColor> crosshair_crosshaircolorList;
    QList<int> crosshair_crosshairwidthList;
    QList<int> crosshair_crosshairlengthList;
    QList<int> crosshair_crosshairopacityList;
    QList<bool> crosshair_showcenterList;
    QList<bool> crosshair_showtopList;
    QList<bool> crosshair_showbottomList;
    QList<bool> crosshair_showleftList;
    QList<bool> crosshair_showrightList;
    QList<int> crosshair_x_offsetList;
    QList<int> crosshair_y_offsetList;
    QList<MAP_KEYDATA> loadkeymapdata;

    if ((true == keyMappingDataFile.contains(KEYMAPDATA_ORIGINALKEYS))
        && (true == keyMappingDataFile.contains(KEYMAPDATA_MAPPINGKEYS))){
        original_keys   = keyMappingDataFile.value(KEYMAPDATA_ORIGINALKEYS).toStringList();
        mapping_keys    = keyMappingDataFile.value(KEYMAPDATA_MAPPINGKEYS).toStringList();
        mappingkeys_keyup = keyMappingDataFile.value(KEYMAPDATA_MAPPINGKEYS_KEYUP).toStringList();

        if (mappingkeys_keyup.size() != mapping_keys.size()) {
            mappingkeys_keyup = mapping_keys;
        }

        int mappingdata_size = original_keys.size();
        QStringList stringListAllON;
        QStringList stringListAllOFF;
        QStringList stringListAllZERO;
        QStringList stringListAllNORMAL;
        QStringList burstpressStringListDefault;
        QStringList burstreleaseStringListDefault;
        QStringList fixedvkeycodeStringListDefault;
        QStringList repeattimesStringListDefault;
        QStringList crosshair_centercolorStringListDefault;
        QStringList crosshair_centersizeStringListDefault;
        QStringList crosshair_centeropacityStringListDefault;
        QStringList crosshair_crosshaircolorStringListDefault;
        QStringList crosshair_crosshairwidthStringListDefault;
        QStringList crosshair_crosshairlengthStringListDefault;
        QStringList crosshair_crosshairopacityStringListDefault;
        for (int i = 0; i < mappingdata_size; ++i) {
            stringListAllON << "ON";
            stringListAllOFF << "OFF";
            stringListAllZERO << "0";
            stringListAllNORMAL << SENDTIMING_STR_NORMAL;
            burstpressStringListDefault.append(QString::number(BURST_PRESS_TIME_DEFAULT));
            burstreleaseStringListDefault.append(QString::number(BURST_RELEASE_TIME_DEFAULT));
            fixedvkeycodeStringListDefault.append(QString::number(FIXED_VIRTUAL_KEY_CODE_NONE, 16));
            repeattimesStringListDefault.append(QString::number(REPEAT_TIMES_DEFAULT));
            crosshair_centercolorStringListDefault.append(CROSSHAIR_CENTERCOLOR_DEFAULT);
            crosshair_centersizeStringListDefault.append(QString::number(CROSSHAIR_CENTERSIZE_DEFAULT));
            crosshair_centeropacityStringListDefault.append(QString::number(CROSSHAIR_CENTEROPACITY_DEFAULT));
            crosshair_crosshaircolorStringListDefault.append(CROSSHAIR_CROSSHAIRCOLOR_DEFAULT);
            crosshair_crosshairwidthStringListDefault.append(QString::number(CROSSHAIR_CROSSHAIRWIDTH_DEFAULT));
            crosshair_crosshairlengthStringListDefault.append(QString::number(CROSSHAIR_CROSSHAIRLENGTH_DEFAULT));
            crosshair_crosshairopacityStringListDefault.append(QString::number(CROSSHAIR_CROSSHAIROPACITY_DEFAULT));
        }
        burstStringList         = stringListAllOFF;
        burstpressStringList    = burstpressStringListDefault;
        burstreleaseStringList  = burstreleaseStringListDefault;
        lockStringList          = stringListAllOFF;
        mappingkeyunlockStringList = stringListAllOFF;
        postmappingkeyStringList = stringListAllOFF;
        fixedvkeycodeStringList = fixedvkeycodeStringListDefault;
        checkcombkeyorderStringList = stringListAllON;
        unbreakableStringList = stringListAllOFF;
        passthroughStringList   = stringListAllOFF;
        sendtimingStringList   = stringListAllNORMAL;
        keyseqholddownStringList = stringListAllOFF;
        repeatmodeStringList = stringListAllZERO;
        repeattimesStringList = repeattimesStringListDefault;
        crosshair_centercolorStringList = crosshair_centercolorStringListDefault;
        crosshair_centersizeStringList = crosshair_centersizeStringListDefault;
        crosshair_centeropacityStringList = crosshair_centeropacityStringListDefault;
        crosshair_crosshaircolorStringList = crosshair_crosshaircolorStringListDefault;
        crosshair_crosshairwidthStringList = crosshair_crosshairwidthStringListDefault;
        crosshair_crosshairlengthStringList = crosshair_crosshairlengthStringListDefault;
        crosshair_crosshairopacityStringList = crosshair_crosshairopacityStringListDefault;
        crosshair_showcenterStringList = stringListAllON;
        crosshair_showtopStringList = stringListAllON;
        crosshair_showbottomStringList = stringListAllON;
        crosshair_showleftStringList = stringListAllON;
        crosshair_showrightStringList = stringListAllON;
        crosshair_x_offsetStringList = stringListAllZERO;
        crosshair_y_offsetStringList = stringListAllZERO;

        if (true == keyMappingDataFile.contains(KEYMAPDATA_NOTE)) {
            notesList = keyMappingDataFile.value(KEYMAPDATA_NOTE).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CATEGORY)) {
            categorysList = keyMappingDataFile.value(KEYMAPDATA_CATEGORY).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_BURST)) {
            burstStringList = keyMappingDataFile.value(KEYMAPDATA_BURST).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_BURSTPRESS_TIME)) {
            burstpressStringList = keyMappingDataFile.value(KEYMAPDATA_BURSTPRESS_TIME).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_BURSTRELEASE_TIME)) {
            burstreleaseStringList = keyMappingDataFile.value(KEYMAPDATA_BURSTRELEASE_TIME).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_LOCK)) {
            lockStringList = keyMappingDataFile.value(KEYMAPDATA_LOCK).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_MAPPINGKEYUNLOCK)) {
            mappingkeyunlockStringList = keyMappingDataFile.value(KEYMAPDATA_MAPPINGKEYUNLOCK).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_POSTMAPPINGKEY)) {
            postmappingkeyStringList = keyMappingDataFile.value(KEYMAPDATA_POSTMAPPINGKEY).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_FIXEDVKEYCODE)) {
            fixedvkeycodeStringList = keyMappingDataFile.value(KEYMAPDATA_FIXEDVKEYCODE).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CHECKCOMBKEYORDER)) {
            checkcombkeyorderStringList = keyMappingDataFile.value(KEYMAPDATA_CHECKCOMBKEYORDER).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_UNBREAKABLE)) {
            unbreakableStringList = keyMappingDataFile.value(KEYMAPDATA_UNBREAKABLE).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_PASSTHROUGH)) {
            passthroughStringList = keyMappingDataFile.value(KEYMAPDATA_PASSTHROUGH).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_SENDTIMING)) {
            sendtimingStringList = keyMappingDataFile.value(KEYMAPDATA_SENDTIMING).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_KEYSEQHOLDDOWN)) {
            keyseqholddownStringList = keyMappingDataFile.value(KEYMAPDATA_KEYSEQHOLDDOWN).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_REPEATMODE)) {
            repeatmodeStringList = keyMappingDataFile.value(KEYMAPDATA_REPEATMODE).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_REPEATIMES)) {
            repeattimesStringList = keyMappingDataFile.value(KEYMAPDATA_REPEATIMES).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_CENTERCOLOR)) {
            crosshair_centercolorStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_CENTERCOLOR).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_CENTERSIZE)) {
            crosshair_centersizeStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_CENTERSIZE).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_CENTEROPACITY)) {
            crosshair_centeropacityStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_CENTEROPACITY).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_CROSSHAIRCOLOR)) {
            crosshair_crosshaircolorStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_CROSSHAIRCOLOR).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_CROSSHAIRWIDTH)) {
            crosshair_crosshairwidthStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_CROSSHAIRWIDTH).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_CROSSHAIRLENGTH)) {
            crosshair_crosshairlengthStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_CROSSHAIRLENGTH).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_CROSSHAIROPACITY)) {
            crosshair_crosshairopacityStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_CROSSHAIROPACITY).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_SHOWCENTER)) {
            crosshair_showcenterStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_SHOWCENTER).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_SHOWTOP)) {
            crosshair_showtopStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_SHOWTOP).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_SHOWBOTTOM)) {
            crosshair_showbottomStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_SHOWBOTTOM).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_SHOWLEFT)) {
            crosshair_showleftStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_SHOWLEFT).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_SHOWRIGHT)) {
            crosshair_showrightStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_SHOWRIGHT).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_X_OFFSET)) {
            crosshair_x_offsetStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_X_OFFSET).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_CROSSHAIR_Y_OFFSET)) {
            crosshair_y_offsetStringList = keyMappingDataFile.value(KEYMAPDATA_CROSSHAIR_Y_OFFSET).toStringList();
        }

        if (original_keys.size() == mapping_keys.size() && original_keys.size() > 0) {

            if (notesList.size() < original_keys.size()) {
                int diff = original_keys.size() - notesList.size();
                for (int i = 0; i < diff; ++i) {
                    notesList.append(QString());
                }
            }

            if (categorysList.size() < original_keys.size()) {
                int diff = original_keys.size() - categorysList.size();
                for (int i = 0; i < diff; ++i) {
                    categorysList.append(QString());
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &burst = (i < burstStringList.size()) ? burstStringList.at(i) : "OFF";
                if (burst == "ON") {
                    burstList.append(true);
                } else {
                    burstList.append(false);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &burstpresstimeStr = (i < burstpressStringList.size()) ? burstpressStringList.at(i) : QString::number(BURST_PRESS_TIME_DEFAULT);
                bool ok;
                int burstpresstime = burstpresstimeStr.toInt(&ok);
                if (!ok || burstpresstime < BURST_TIME_MIN || burstpresstime > BURST_TIME_MAX) {
                    burstpresstime = BURST_PRESS_TIME_DEFAULT;
                }
                burstpresstimeList.append(burstpresstime);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &burstreleasetimeStr = (i < burstreleaseStringList.size()) ? burstreleaseStringList.at(i) : QString::number(BURST_RELEASE_TIME_DEFAULT);
                bool ok;
                int burstreleasetime = burstreleasetimeStr.toInt(&ok);
                if (!ok || burstreleasetime < BURST_TIME_MIN || burstreleasetime > BURST_TIME_MAX) {
                    burstreleasetime = BURST_RELEASE_TIME_DEFAULT;
                }
                burstreleasetimeList.append(burstreleasetime);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &lock = (i < lockStringList.size()) ? lockStringList.at(i) : "OFF";
                if (lock == "ON") {
                    lockList.append(true);
                } else {
                    lockList.append(false);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &mappingkeyunlock = (i < mappingkeyunlockStringList.size()) ? mappingkeyunlockStringList.at(i) : "OFF";
                if (mappingkeyunlock == "ON") {
                    mappingkeyunlockList.append(true);
                } else {
                    mappingkeyunlockList.append(false);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &postmappingkey = (i < postmappingkeyStringList.size()) ? postmappingkeyStringList.at(i) : "OFF";
                if (postmappingkey == "ON") {
                    postmappingkeyList.append(true);
                } else {
                    postmappingkeyList.append(false);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &fixedvkeycodeStr = (i < fixedvkeycodeStringList.size()) ? fixedvkeycodeStringList.at(i) : QString::number(FIXED_VIRTUAL_KEY_CODE_NONE, 16);
                bool ok;
                int fixedvkeycode = fixedvkeycodeStr.toInt(&ok, 16);
                if (!ok || fixedvkeycode < FIXED_VIRTUAL_KEY_CODE_MIN || fixedvkeycode > FIXED_VIRTUAL_KEY_CODE_MAX) {
                    fixedvkeycode = FIXED_VIRTUAL_KEY_CODE_NONE;
                }
                fixedvkeycodeList.append(fixedvkeycode);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &checkcombkeyorder = (i < checkcombkeyorderStringList.size()) ? checkcombkeyorderStringList.at(i) : "ON";
                if (checkcombkeyorder == "OFF") {
                    checkcombkeyorderList.append(false);
                } else {
                    checkcombkeyorderList.append(true);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &unbreakable = (i < unbreakableStringList.size()) ? unbreakableStringList.at(i) : "OFF";
                if (unbreakable == "ON") {
                    unbreakableList.append(true);
                } else {
                    unbreakableList.append(false);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &passthrough = (i < passthroughStringList.size()) ? passthroughStringList.at(i) : "OFF";
                if (passthrough == "ON") {
                    passthroughList.append(true);
                } else {
                    passthroughList.append(false);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &sendtiming = (i < sendtimingStringList.size()) ? sendtimingStringList.at(i) : SENDTIMING_STR_NORMAL;
                if (sendtiming == SENDTIMING_STR_KEYDOWN) {
                    sendtimingList.append(SENDTIMING_KEYDOWN);
                }
                else if (sendtiming == SENDTIMING_STR_KEYUP) {
                    sendtimingList.append(SENDTIMING_KEYUP);
                }
                else if (sendtiming == SENDTIMING_STR_KEYDOWN_AND_KEYUP) {
                    sendtimingList.append(SENDTIMING_KEYDOWN_AND_KEYUP);
                }
                else if (sendtiming == SENDTIMING_STR_NORMAL_AND_KEYUP) {
                    sendtimingList.append(SENDTIMING_NORMAL_AND_KEYUP);
                }
                else {
                    sendtimingList.append(SENDTIMING_NORMAL);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &keyseqholddown = (i < keyseqholddownStringList.size()) ? keyseqholddownStringList.at(i) : "OFF";
                if (keyseqholddown == "ON") {
                    keyseqholddownList.append(true);
                } else {
                    keyseqholddownList.append(false);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &repeatmodeStr = (i < repeatmodeStringList.size()) ? repeatmodeStringList.at(i) : QString::number(REPEAT_MODE_NONE);
                bool ok;
                int repeatmode = repeatmodeStr.toInt(&ok);
                if (!ok || repeatmode < REPEAT_MODE_NONE || repeatmode > REPEAT_MODE_BYTIMES) {
                    repeatmode = REPEAT_MODE_NONE;
                }
                repeatmodeList.append(repeatmode);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &repeattimesStr = (i < repeattimesStringList.size()) ? repeattimesStringList.at(i) : QString::number(REPEAT_TIMES_DEFAULT);
                bool ok;
                int repeattimes = repeattimesStr.toInt(&ok);
                if (!ok || repeattimes < REPEAT_TIMES_MIN || repeattimes > REPEAT_TIMES_MAX) {
                    repeattimes = REPEAT_TIMES_DEFAULT;
                }
                repeattimesList.append(repeattimes);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_centercolorStr = (i < crosshair_centercolorStringList.size()) ? crosshair_centercolorStringList.at(i) : CROSSHAIR_CENTERCOLOR_DEFAULT;
                QColor crosshair_centercolor = QColor(QString("%1%2").arg("#", crosshair_centercolorStr));
                bool isvalid = crosshair_centercolor.isValid();
                if (!isvalid) {
                    crosshair_centercolor = CROSSHAIR_CENTERCOLOR_DEFAULT_QCOLOR;
                }
                crosshair_centercolorList.append(crosshair_centercolor);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_centersizeStr = (i < crosshair_centersizeStringList.size()) ? crosshair_centersizeStringList.at(i) : QString::number(CROSSHAIR_CENTERSIZE_DEFAULT);
                bool ok;
                int crosshair_centersize = crosshair_centersizeStr.toInt(&ok);
                if (!ok || crosshair_centersize < CROSSHAIR_CENTERSIZE_MIN || crosshair_centersize > CROSSHAIR_CENTERSIZE_MAX) {
                    crosshair_centersize = CROSSHAIR_CENTERSIZE_DEFAULT;
                }
                crosshair_centersizeList.append(crosshair_centersize);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_centeropacityStr = (i < crosshair_centeropacityStringList.size()) ? crosshair_centeropacityStringList.at(i) : QString::number(CROSSHAIR_CENTEROPACITY_DEFAULT);
                bool ok;
                int crosshair_centeropacity = crosshair_centeropacityStr.toInt(&ok);
                if (!ok || crosshair_centeropacity < CROSSHAIR_OPACITY_MIN || crosshair_centeropacity > CROSSHAIR_OPACITY_MAX) {
                    crosshair_centeropacity = CROSSHAIR_CENTEROPACITY_DEFAULT;
                }
                crosshair_centeropacityList.append(crosshair_centeropacity);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_crosshaircolorStr = (i < crosshair_crosshaircolorStringList.size()) ? crosshair_crosshaircolorStringList.at(i) : CROSSHAIR_CROSSHAIRCOLOR_DEFAULT;
                QColor crosshair_crosshaircolor = QColor(QString("%1%2").arg("#", crosshair_crosshaircolorStr));
                bool isvalid = crosshair_crosshaircolor.isValid();
                if (!isvalid) {
                    crosshair_crosshaircolor = CROSSHAIR_CROSSHAIRCOLOR_DEFAULT_QCOLOR;
                }
                crosshair_crosshaircolorList.append(crosshair_crosshaircolor);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_crosshairwidthStr = (i < crosshair_crosshairwidthStringList.size()) ? crosshair_crosshairwidthStringList.at(i) : QString::number(CROSSHAIR_CROSSHAIRWIDTH_DEFAULT);
                bool ok;
                int crosshair_crosshairwidth = crosshair_crosshairwidthStr.toInt(&ok);
                if (!ok || crosshair_crosshairwidth < CROSSHAIR_CROSSHAIRWIDTH_MIN || crosshair_crosshairwidth > CROSSHAIR_CROSSHAIRWIDTH_MAX) {
                    crosshair_crosshairwidth = CROSSHAIR_CROSSHAIRWIDTH_DEFAULT;
                }
                crosshair_crosshairwidthList.append(crosshair_crosshairwidth);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_crosshairlengthStr = (i < crosshair_crosshairlengthStringList.size()) ? crosshair_crosshairlengthStringList.at(i) : QString::number(CROSSHAIR_CROSSHAIRLENGTH_DEFAULT);
                bool ok;
                int crosshair_crosshairlength = crosshair_crosshairlengthStr.toInt(&ok);
                if (!ok || crosshair_crosshairlength < CROSSHAIR_CROSSHAIRLENGTH_MIN || crosshair_crosshairlength > CROSSHAIR_CROSSHAIRLENGTH_MAX) {
                    crosshair_crosshairlength = CROSSHAIR_CROSSHAIRLENGTH_DEFAULT;
                }
                crosshair_crosshairlengthList.append(crosshair_crosshairlength);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_crosshairopacityStr = (i < crosshair_crosshairopacityStringList.size()) ? crosshair_crosshairopacityStringList.at(i) : QString::number(CROSSHAIR_CROSSHAIROPACITY_DEFAULT);
                bool ok;
                int crosshair_crosshairopacity = crosshair_crosshairopacityStr.toInt(&ok);
                if (!ok || crosshair_crosshairopacity < CROSSHAIR_OPACITY_MIN || crosshair_crosshairopacity > CROSSHAIR_OPACITY_MAX) {
                    crosshair_crosshairopacity = CROSSHAIR_CROSSHAIROPACITY_DEFAULT;
                }
                crosshair_crosshairopacityList.append(crosshair_crosshairopacity);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_showcenter = (i < crosshair_showcenterStringList.size()) ? crosshair_showcenterStringList.at(i) : "ON";
                if (crosshair_showcenter == "OFF") {
                    crosshair_showcenterList.append(false);
                }
                else {
                    crosshair_showcenterList.append(true);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_showtop = (i < crosshair_showtopStringList.size()) ? crosshair_showtopStringList.at(i) : "ON";
                if (crosshair_showtop == "OFF") {
                    crosshair_showtopList.append(false);
                }
                else {
                    crosshair_showtopList.append(true);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_showbottom = (i < crosshair_showbottomStringList.size()) ? crosshair_showbottomStringList.at(i) : "ON";
                if (crosshair_showbottom == "OFF") {
                    crosshair_showbottomList.append(false);
                }
                else {
                    crosshair_showbottomList.append(true);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_showleft = (i < crosshair_showleftStringList.size()) ? crosshair_showleftStringList.at(i) : "ON";
                if (crosshair_showleft == "OFF") {
                    crosshair_showleftList.append(false);
                }
                else {
                    crosshair_showleftList.append(true);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_showright = (i < crosshair_showrightStringList.size()) ? crosshair_showrightStringList.at(i) : "ON";
                if (crosshair_showright == "OFF") {
                    crosshair_showrightList.append(false);
                }
                else {
                    crosshair_showrightList.append(true);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_x_offsetStr = (i < crosshair_x_offsetStringList.size()) ? crosshair_x_offsetStringList.at(i) : QString::number(CROSSHAIR_X_OFFSET_DEFAULT);
                bool ok;
                int crosshair_x_offset = crosshair_x_offsetStr.toInt(&ok);
                if (!ok || crosshair_x_offset < CROSSHAIR_X_OFFSET_MIN || crosshair_x_offset > CROSSHAIR_X_OFFSET_MAX) {
                    crosshair_x_offset = CROSSHAIR_X_OFFSET_DEFAULT;
                }
                crosshair_x_offsetList.append(crosshair_x_offset);
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &crosshair_y_offsetStr = (i < crosshair_y_offsetStringList.size()) ? crosshair_y_offsetStringList.at(i) : QString::number(CROSSHAIR_Y_OFFSET_DEFAULT);
                bool ok;
                int crosshair_y_offset = crosshair_y_offsetStr.toInt(&ok);
                if (!ok || crosshair_y_offset < CROSSHAIR_Y_OFFSET_MIN || crosshair_y_offset > CROSSHAIR_Y_OFFSET_MAX) {
                    crosshair_y_offset = CROSSHAIR_Y_OFFSET_DEFAULT;
                }
                crosshair_y_offsetList.append(crosshair_y_offset);
            }

            int loadindex = 0;
            for (const QString &ori_key_nochange : std::as_const(original_keys)){
                QString ori_key = ori_key_nochange;
                if (ori_key.startsWith(OLD_PREFIX_SHORTCUT)) {
                    ori_key.remove(0, 1);
                }

                bool checkoriginalstr = checkOriginalkeyStr(ori_key);
                bool checkmappingstr = checkMappingkeyStr(mapping_keys[loadindex]);

                if (mappingkeys_keyup[loadindex].isEmpty()) {
                    mappingkeys_keyup[loadindex] = mapping_keys[loadindex];
                }
                bool checkmapping_keyupstr = checkMappingkeyStr(mappingkeys_keyup[loadindex]);
                if (!checkmapping_keyupstr) {
                    mappingkeys_keyup[loadindex] = mapping_keys[loadindex];
                }

                if (true == checkoriginalstr
                    && true == checkmappingstr) {
                    loadkeymapdata.append(MAP_KEYDATA(ori_key,
                                                      mapping_keys.at(loadindex),
                                                      mappingkeys_keyup.at(loadindex),
                                                      notesList.at(loadindex),
                                                      categorysList.at(loadindex),
                                                      burstList.at(loadindex),
                                                      burstpresstimeList.at(loadindex),
                                                      burstreleasetimeList.at(loadindex),
                                                      lockList.at(loadindex),
                                                      mappingkeyunlockList.at(loadindex),
                                                      postmappingkeyList.at(loadindex),
                                                      fixedvkeycodeList.at(loadindex),
                                                      checkcombkeyorderList.at(loadindex),
                                                      unbreakableList.at(loadindex),
                                                      passthroughList.at(loadindex),
                                                      sendtimingList.at(loadindex),
                                                      keyseqholddownList.at(loadindex),
                                                      repeatmodeList.at(loadindex),
                                                      repeattimesList.at(loadindex),
                                                      crosshair_centercolorList.at(loadindex),
                                                      crosshair_centersizeList.at(loadindex),
                                                      crosshair_centeropacityList.at(loadindex),
                                                      crosshair_crosshaircolorList.at(loadindex),
                                                      crosshair_crosshairwidthList.at(loadindex),
                                                      crosshair_crosshairlengthList.at(loadindex),
                                                      crosshair_crosshairopacityList.at(loadindex),
                                                      crosshair_showcenterList.at(loadindex),
                                                      crosshair_showtopList.at(loadindex),
                                                      crosshair_showbottomList.at(loadindex),
                                                      crosshair_showleftList.at(loadindex),
                                                      crosshair_showrightList.at(loadindex),
                                                      crosshair_x_offsetList.at(loadindex),
                                                      crosshair_y_offsetList.at(loadindex)
                                                      ));
                }

                loadindex += 1;
            }
        }
    }

    if (loadkeymapdata.isEmpty()) {
        return false;
    }

    bool import_result = false;
    QList<MAP_KEYDATA> *mappingDataList = s_KeyMappingTabInfoList.at(tabindex).KeyMappingData;
    for (const MAP_KEYDATA &keymapdata : std::as_const(loadkeymapdata)) {
        int findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(keymapdata.Original_Key);
        if (findindex != -1) {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[importKeyMappingDataFromFile]" << "Duplicate original key found -> index : " << findindex << ", originalkey : " << keymapdata.Original_Key;
#endif
            continue;
        }
        mappingDataList->append(keymapdata);
        import_result = true;
    }

    return import_result;
}

void QKeyMapper::updateKeyMappingDataListMappingKeys(int rowindex, const QString &mappingkeystr)
{
    if (rowindex < 0 || rowindex >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    QStringList mappingKeySeqList = splitMappingKeyString(mappingkeystr, SPLIT_WITH_NEXT);
    (*QKeyMapper::KeyMappingDataList)[rowindex].Mapping_Keys = mappingKeySeqList;

    QStringList pure_mappingKeys = splitMappingKeyString(mappingkeystr, SPLIT_WITH_PLUSANDNEXT, true);
    pure_mappingKeys.removeDuplicates();
    (*QKeyMapper::KeyMappingDataList)[rowindex].Pure_MappingKeys = pure_mappingKeys;
}

void QKeyMapper::updateKeyMappingDataListKeyUpMappingKeys(int rowindex, const QString &mappingkeystr)
{
    if (rowindex < 0 || rowindex >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    QStringList mappingKeySeqList = splitMappingKeyString(mappingkeystr, SPLIT_WITH_NEXT);
    (*QKeyMapper::KeyMappingDataList)[rowindex].MappingKeys_KeyUp = mappingKeySeqList;

    QStringList pure_mappingKeys = splitMappingKeyString(mappingkeystr, SPLIT_WITH_PLUSANDNEXT, true);
    pure_mappingKeys.removeDuplicates();
    (*QKeyMapper::KeyMappingDataList)[rowindex].Pure_MappingKeys_KeyUp = pure_mappingKeys;
}

bool QKeyMapper::validateSendTimingByKeyMapData(const MAP_KEYDATA &keymapdata)
{
    bool disable_sendtiming = false;

    if (keymapdata.Original_Key == VJOY_MOUSE2LS_STR || keymapdata.Original_Key == VJOY_MOUSE2RS_STR) {
        disable_sendtiming = true;
    }

    if (keymapdata.Original_Key == JOY_LS2VJOYLS_STR
        || keymapdata.Original_Key == JOY_RS2VJOYRS_STR
        || keymapdata.Original_Key == JOY_LS2VJOYRS_STR
        || keymapdata.Original_Key == JOY_RS2VJOYLS_STR
        || keymapdata.Original_Key == JOY_LT2VJOYLT_STR
        || keymapdata.Original_Key == JOY_RT2VJOYRT_STR) {
        disable_sendtiming = true;
    }

    if (keymapdata.Original_Key == JOY_LS2MOUSE_STR
        || keymapdata.Original_Key == JOY_RS2MOUSE_STR
        || keymapdata.Original_Key == JOY_GYRO2MOUSE_STR) {
        disable_sendtiming = true;
    }

    if (keymapdata.Original_Key.contains(MOUSE_WHEEL_STR)) {
        disable_sendtiming = true;
    }

    if (keymapdata.Mapping_Keys.constFirst().contains(KEY_BLOCKED_STR)
        || keymapdata.MappingKeys_KeyUp.constFirst().contains(KEY_BLOCKED_STR)) {
        disable_sendtiming = true;
    }
    else if (keymapdata.Mapping_Keys.constFirst().startsWith(KEY2MOUSE_PREFIX)
        || keymapdata.MappingKeys_KeyUp.constFirst().startsWith(KEY2MOUSE_PREFIX)) {
        disable_sendtiming = true;
    }
    else if (keymapdata.Mapping_Keys.constFirst().contains(MOUSE2VJOY_HOLD_KEY_STR)
        || keymapdata.MappingKeys_KeyUp.constFirst().contains(MOUSE2VJOY_HOLD_KEY_STR)) {
        disable_sendtiming = true;
    }
    else if (keymapdata.Mapping_Keys.constFirst().startsWith(GYRO2MOUSE_PREFIX)
        || keymapdata.MappingKeys_KeyUp.constFirst().startsWith(GYRO2MOUSE_PREFIX)) {
        disable_sendtiming = true;
    }
    else if (keymapdata.Mapping_Keys.constFirst().startsWith(CROSSHAIR_PREFIX)
        || keymapdata.MappingKeys_KeyUp.constFirst().startsWith(CROSSHAIR_PREFIX)) {
        disable_sendtiming = true;
    }
    else if (keymapdata.Mapping_Keys.constFirst().startsWith(VJOY_LS_RADIUS_STR)
        || keymapdata.MappingKeys_KeyUp.constFirst().startsWith(VJOY_RS_RADIUS_STR)) {
        disable_sendtiming = true;
    }
    else if (keymapdata.Mapping_Keys.constFirst().contains(VJOY_LT_BRAKE_STR)
        || keymapdata.Mapping_Keys.constFirst().contains(VJOY_RT_BRAKE_STR)
        || keymapdata.Mapping_Keys.constFirst().contains(VJOY_LT_ACCEL_STR)
        || keymapdata.Mapping_Keys.constFirst().contains(VJOY_RT_ACCEL_STR)
        || keymapdata.MappingKeys_KeyUp.constFirst().contains(VJOY_LT_BRAKE_STR)
        || keymapdata.MappingKeys_KeyUp.constFirst().contains(VJOY_RT_BRAKE_STR)
        || keymapdata.MappingKeys_KeyUp.constFirst().contains(VJOY_LT_ACCEL_STR)
        || keymapdata.MappingKeys_KeyUp.constFirst().contains(VJOY_RT_ACCEL_STR)) {
        disable_sendtiming = true;
    }

    return (!disable_sendtiming);
}

ValidationResult QKeyMapper::updateWithZipUpdater(const QString &update_filepath)
{
    ValidationResult result;
    result.isValid = true;

    QFileInfo updateFileInfo(update_filepath);
    QString update_filename = updateFileInfo.fileName();
    QString update_dirname = updateFileInfo.dir().dirName();

    QString zipupdater_exe = QString("%1/%2").arg(ZIPUPDATER_DIR, ZIPUPDATER_EXE);
    QString zipupdater_dll_msvcp = QString("%1/%2").arg(ZIPUPDATER_DIR, ZIPUPDATER_DLL_MSVCP);
    QString zipupdater_dll_vcruntime = QString("%1/%2").arg(ZIPUPDATER_DIR, ZIPUPDATER_DLL_VCRUNTIME);
    QString missingFile;
    if (!QFile::exists(zipupdater_exe)) {
        missingFile = zipupdater_exe;
    }
    else if (!QFile::exists(zipupdater_dll_msvcp)) {
        missingFile = zipupdater_dll_msvcp;
    }
    else if (!QFile::exists(zipupdater_dll_vcruntime)) {
        missingFile = zipupdater_dll_vcruntime;
    }

    if (!missingFile.isEmpty()) {
        result.errorMessage = tr("ZipUpdater program file %1 does not exist!").arg(missingFile);
        result.isValid = false;
        return result;
    }

    QFileInfo update_dirInfo(update_dirname);
    if (update_dirInfo.exists() && update_dirInfo.isDir()) {
    }
    else {
        result.errorMessage = tr("Update directory %1 does not exist!").arg(update_dirname);
        result.isValid = false;
        return result;
    }

    QStringList filesToCopy = {ZIPUPDATER_EXE, ZIPUPDATER_DLL_MSVCP, ZIPUPDATER_DLL_VCRUNTIME};
    for (const QString &file : filesToCopy) {
        QString sourceFilePath = QString("%1/%2").arg(ZIPUPDATER_DIR, file);
        QString targetFilePath = QString("%1/%2").arg(update_dirname, file);

        if (QFile::exists(targetFilePath)) {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("[updateWithZipUpdater] Update program file %1 already exists. Removing it first.").arg(file);
            qDebug().noquote().nospace() << debugmessage;
#endif
            if (!QFile::remove(targetFilePath)) {
                result.errorMessage = tr("Failed to remove existing update program file %1!").arg(file);
                result.isValid = false;
                return result;
            }
        }

        if (!QFile::copy(sourceFilePath, targetFilePath)) {
            result.errorMessage = tr("Failed to copy update program file %1 to %2!").arg(file, update_dirname);
            result.isValid = false;
            return result;
        }
    }

    QString zipupdater_exe_path = QString("%1/%2").arg(update_dirname, ZIPUPDATER_EXE);
    QStringList arguments;
    QString copyFromFolder;
    QString platformString = getPlatformString();
    QString copyToPath = QCoreApplication::applicationDirPath();
    QString restartExePath = QCoreApplication::applicationFilePath();
    QString language;
    if (update_filename.contains(platformString)) {
        copyFromFolder = QString("%1_%2").arg(PROGRAM_NAME, platformString);
    }
    else {
        result.errorMessage = tr("The update file name %1 does not match the platform string %2 !").arg(update_filename, platformString);
        result.isValid = false;
        return result;
    }
    if (LANGUAGE_CHINESE == QKeyMapper::getLanguageIndex()) {
        language = "--chinese";
    }
    else if (LANGUAGE_JAPANESE == QKeyMapper::getLanguageIndex()) {
        language = "--japanese";
    }
    arguments << update_filename << copyFromFolder << copyToPath << restartExePath << language;

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[updateWithZipUpdater] UpdateProgram :" << zipupdater_exe_path;
    qDebug() << "[updateWithZipUpdater] Arguments     :" << arguments;
#endif

#ifdef DEBUG_LOGOUT_ON
    if (false == QKeyMapper::getInstance()->isHidden()) {
        qDebug() << "[updateWithZipUpdater] Force hide the window before start zipupdater.exe";
    }
#endif
    QKeyMapper::getInstance()->forceHide();

    QProcess process;
    bool started = process.startDetached(zipupdater_exe_path, arguments);

    if (!started) {
        result.errorMessage = tr("Failed to start update program %1!").arg(ZIPUPDATER_EXE);
        result.isValid = false;
        return result;
    }

    return result;
}

bool QKeyMapper::isSelectColorDialogVisible()
{
    if (getInstance()->m_SelectColorDialog != Q_NULLPTR
        && getInstance()->m_SelectColorDialog->isVisible()) {
        return true;
    }
    else {
        return false;
    }
}

QIcon QKeyMapper::setTabCustomImage(int tabindex, QString &imagepath)
{
    if (tabindex < 0 || tabindex >= s_KeyMappingTabInfoList.size()) {
        return QIcon();
    }

    if (!QFileInfo::exists(imagepath)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[QKeyMapper::setTabCustomImage]" << " QFileInfo::exists(" << imagepath << ") = false";
#endif
        return QIcon();
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QTableSetupDialog::setTabCustomImage]" << "Set Custom Image Path =" << imagepath;
#endif

    // First, try to load the image using the original path
    QString appDir = QCoreApplication::applicationDirPath();
    QFileInfo imgInfo(imagepath);
    QString imgAbsPath = imgInfo.absoluteFilePath();

    QString pathToLoad;

    // Check if the image is in the application directory or its subdirectory, and prepare the relative path
    if (imgAbsPath.startsWith(appDir, Qt::CaseInsensitive)) {
        QString relPath = QDir(appDir).relativeFilePath(imgAbsPath);
        pathToLoad = relPath;
    } else {
        pathToLoad = imgAbsPath;
    }

    // Try to load the image using the converted path
    QIcon icon_loaded(pathToLoad);
    if (!icon_loaded.isNull()) {
        s_KeyMappingTabInfoList[tabindex].TabCustomImage_Path = pathToLoad;
        imagepath = pathToLoad;
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QKeyMapper::setTabCustomImage] Store path:" << pathToLoad;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QKeyMapper::setTabCustomImage] Failed to load image from path:" << pathToLoad;
#endif
    }

    return icon_loaded;
}

void QKeyMapper::clearTabCustomImage(int tabindex)
{
    if (tabindex < 0 || tabindex >= s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QTableSetupDialog::clearTabCustomImage]" << "Clear Custom Image for Tab Index[" << tabindex << "] :" << s_KeyMappingTabInfoList[tabindex].TabName;
#endif

    s_KeyMappingTabInfoList[tabindex].TabCustomImage_Path.clear();
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
bool QKeyMapper::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#else
bool QKeyMapper::nativeEvent(const QByteArray &eventType, void *message, long *result)
#endif
{
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_WTSSESSION_CHANGE) {
            if (msg->wParam == WTS_SESSION_LOCK) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[QKeyMapper::nativeEvent]" << "Session Locked.";
#endif
                sessionLockStateChanged(true);
            } else if (msg->wParam == WTS_SESSION_UNLOCK) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[QKeyMapper::nativeEvent]" << "Session Unlocked.";
#endif
                sessionLockStateChanged(false);
            }
        }
    }

    return QWidget::nativeEvent(eventType, message, result);
}

void QKeyMapper::showEvent(QShowEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QKeyMapper::showEvent]" << "QKeyMapper show ->" << event->spontaneous();
#endif

    if (false == event->spontaneous()) {
#ifdef DEBUG_LOGOUT_ON
        QRect parentRect = geometry();
        QString debugmessage = QString("[QKeyMapper::showEvent] Call QSimpleUpdater::setGeometryWithParentWidget(%1, %2)").arg(parentRect.x()).arg(parentRect.y());
        qDebug().noquote().nospace() << debugmessage;
#endif
        QSimpleUpdater::getInstance()->setGeometryWithParentWidget(this);

        QTimer::singleShot(100, this, [=]() {
            if (m_KeyMapStatus == KEYMAP_IDLE){
                QWidget *focused = QApplication::focusWidget();
                if (focused && focused != this) {
                    focused->clearFocus();
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[QKeyMapper::showEvent]" << "Clear initial Focus.";
#endif
                }
            }
        });
    }

    QDialog::showEvent(event);
}

void QKeyMapper::closeEvent(QCloseEvent *event)
{
    if ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0 || (GetAsyncKeyState(VK_RMENU) & 0x8000) != 0) {
        if ((GetAsyncKeyState(VK_F4) & 0x8000) != 0) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::closeEvent] Alt + F4 pressed, quit application!";
#endif
            event->accept();
            return;
        }
    }

    bool force_showdialog = false;
    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
        force_showdialog = true;
    }

    bool closeto_systemtray = isCloseToSystemtray(force_showdialog);

    if (closeto_systemtray) {
        if (m_deviceListWindow->isVisible()) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::closeEvent]" << "DeviceList Windows isVisible!";
#endif
            return;
        }

        if (false == isHidden()) {
            m_LastWindowPosition = pos(); // Save the current position before hiding
            closeSelectColorDialog();
            closeTableSetupDialog();
            closeFloatingWindowSetupDialog();
            closeItemSetupDialog();
            closeCrosshairSetupDialog();
            closeGyro2MouseAdvancedSettingDialog();
            closeTrayIconSelectDialog();
            closeNotificationSetupDialog();
            hide();
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::closeEvent] Hide Window on closeEvent, LastWindowPosition ->" << m_LastWindowPosition;
#endif
        }

        event->ignore();
    }
    else {
        event->accept();
    }
}

void QKeyMapper::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        QTimer::singleShot(0, this, SLOT(WindowStateChangedProc()));
    }

    QDialog::changeEvent(event);
}

void QKeyMapper::keyPressEvent(QKeyEvent *event)
{
    /* Check L-Ctrl+S to Save settings */
    if (m_KeyMapStatus == KEYMAP_IDLE
        && event->key() == Qt::Key_S
        && QT_KEY_L_CTRL == (event->nativeModifiers() & QT_KEY_L_CTRL)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QKeyMapper::keyPressEvent]" << "\"L-Ctrl+S\" CombinationKey Pressed -> saveKeyMappingDataToFile()";
#endif
        saveKeyMapSetting();
        return;
    }
    else if (m_KeyMapStatus == KEYMAP_IDLE && event->key() == KEY_REFRESH) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyPressEvent]" << "F5 Key Pressed -> refreshProcessInfoTable()";
#endif
// #ifdef QT_NO_DEBUG
//         m_ProcessInfoTableRefreshTimer.start(CYCLE_REFRESH_PROCESSINFOTABLE_TIMEOUT);
// #endif
        updateHWNDListProc();
        refreshProcessInfoTable();
        (void)Interception_Worker::getRefreshedKeyboardDeviceList();
        (void)Interception_Worker::getRefreshedMouseDeviceList();
        Interception_Worker::syncDisabledKeyboardList();
        Interception_Worker::syncDisabledMouseList();
        // m_deviceListWindow->updateDeviceListInfo();
        initInputDeviceSelectComboBoxes();
        return;
    }
#if 0
    else if (event->key() == KEY_PASSTHROUGH) {
        if (m_KeyMapStatus == KEYMAP_IDLE){
            int currentrowindex = -1;
            QList<QTableWidgetItem*> items = m_KeyMappingDataTable->selectedItems();
            if (items.size() > 0) {
                QTableWidgetItem* selectedItem = items.at(0);
                currentrowindex = m_KeyMappingDataTable->row(selectedItem);

                if (KeyMappingDataList->at(currentrowindex).PassThrough) {
                    (*KeyMappingDataList)[currentrowindex].PassThrough = false;
                }
                else {
                    (*KeyMappingDataList)[currentrowindex].PassThrough = true;
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug().noquote().nospace() << "[PassThroughStatus]" << "F12 Key Pressed -> Selected mappingdata original_key[" << KeyMappingDataList->at(currentrowindex).Original_Key << "] PassThrough = " << (*KeyMappingDataList)[currentrowindex].PassThrough;
#endif
                refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);
                // int reselectrow = currentrowindex;
                // QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
                // m_KeyMappingDataTable->setRangeSelected(selection, true);
                return;
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[PassThroughStatus]" << "F12 Key Pressed -> There is no selected mapping data";
#endif
            }
        }
    }
    else if (event->key() == KEY_KEYUP_ACTION) {
        if (m_KeyMapStatus == KEYMAP_IDLE){
            int currentrowindex = -1;
            QList<QTableWidgetItem*> items = m_KeyMappingDataTable->selectedItems();
            if (items.size() > 0) {
                QTableWidgetItem* selectedItem = items.at(0);
                currentrowindex = m_KeyMappingDataTable->row(selectedItem);

                if (KeyMappingDataList->at(currentrowindex).KeyUp_Action) {
                    (*KeyMappingDataList)[currentrowindex].KeyUp_Action = false;
                }
                else {
                    (*KeyMappingDataList)[currentrowindex].KeyUp_Action = true;
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug().noquote().nospace() << "[KeyUp_ActionStatus]" << "F2 Key Pressed -> Selected mappingdata original_key[" << KeyMappingDataList->at(currentrowindex).Original_Key << "] KeyUp_Action = " << (*KeyMappingDataList)[currentrowindex].KeyUp_Action;
#endif
                refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);
                int reselectrow = currentrowindex;
                QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
                m_KeyMappingDataTable->setRangeSelected(selection, true);
                return;
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[KeyUp_ActionStatus]" << "F2 Key Pressed -> There is no selected mapping data";
#endif
            }
        }
    }
    else if (event->key() == KEY_REMOVE_LAST) {
        if (m_KeyMapStatus == KEYMAP_IDLE) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[Remove_Last]" << "Backspace Key Pressed -> remove last mapping key.";
#endif
            int currentrowindex = -1;
            QList<QTableWidgetItem*> items = m_KeyMappingDataTable->selectedItems();
            if (items.size() > 0) {
                QTableWidgetItem* selectedItem = items.at(0);
                currentrowindex = m_KeyMappingDataTable->row(selectedItem);

                bool refresh_flag = false;
                QStringList newMappingKeyList;
                QStringList mappingKeyList = KeyMappingDataList.at(currentrowindex).Mapping_Keys;
                // QString original_key = KeyMappingDataList.at(currentrowindex).Original_Key;
                QString firstmappingkey = mappingKeyList.constFirst();
                int mappingkeylist_size = mappingKeyList.size();

                if (mappingkeylist_size > 1) {
                    QString lastmappingkey = mappingKeyList.constLast();

                    if (lastmappingkey.contains(SEPARATOR_PLUS)) {
                        QStringList splitList = lastmappingkey.split(SEPARATOR_PLUS);
                        splitList.removeLast();
                        QString newLastMappingkey = splitList.join(SEPARATOR_PLUS);
                        mappingKeyList.removeLast();
                        mappingKeyList.append(newLastMappingkey);
                        newMappingKeyList = mappingKeyList;
                        refresh_flag = true;
                    }
                    else {
                        mappingKeyList.removeLast();
                        newMappingKeyList = mappingKeyList;
                        refresh_flag = true;
                    }
                }
                else {
                    if (firstmappingkey.contains(SEPARATOR_PLUS)) {
                        QStringList splitList = firstmappingkey.split(SEPARATOR_PLUS);
                        splitList.removeLast();
                        newMappingKeyList.append(splitList.join(SEPARATOR_PLUS));
                        refresh_flag = true;
                    }
                }

                if (refresh_flag) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().noquote().nospace() << "[Remove_Last]" << "Backspace Key Pressed, newMappingKeyList after remove last -> [" << newMappingKeyList << "]";
#endif
                    KeyMappingDataList[currentrowindex].Mapping_Keys = newMappingKeyList;

                    refreshKeyMappingDataTable();
                    int reselectrow = currentrowindex;
                    QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
                    m_KeyMappingDataTable->setRangeSelected(selection, true);
                    return;
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[Remove_Last]" << "Backspace Key Pressed -> There is no selected mapping data";
#endif
            }
        }
    }
#endif
    else if (event->key() == Qt::Key_Escape) {
       return;
    }

    QDialog::keyPressEvent(event);
}

void QKeyMapper::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = QApplication::focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}

bool QKeyMapper::eventFilter(QObject *object, QEvent *event)
{
    if (m_KeyMapStatus == KEYMAP_IDLE
        && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_F2) {
            int highlightedRow = -1;
            QAbstractItemView *view = ui->gamepadSelectComboBox->view();
            if (view->isVisible()) {
                QModelIndex highlightedIndex = view->currentIndex();
                if (highlightedIndex.isValid()) {
                    highlightedRow = highlightedIndex.row();

                    if (highlightedRow >= 1) {
// #ifdef DEBUG_LOGOUT_ON
//                         QString debugmessage = QString("[GamepadSelectComboBox] F2 Key pressed on item(%1) selected.").arg(highlightedRow);
//                         qDebug().noquote() << debugmessage;
// #endif
                        emit QKeyMapper_Worker::getInstance()->gameControllerGyroEnabledSwitch_Signal(highlightedRow-1);
                    }
                }
            }
        }
    }
    return QDialog::eventFilter(object, event);
}

void QKeyMapper::on_keymapButton_clicked()
{
    MappingSwitch(MAPPINGSTART_BUTTONCLICK);
}

void QKeyMapper::MappingSwitch(MappingStartMode startmode)
{
    QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
    QMetaEnum mappingstartmodeEnum = QMetaEnum::fromType<QKeyMapper::MappingStartMode>();
    bool startKeyMap = false;
    Q_UNUSED(startmode);
    Q_UNUSED(keymapstatusEnum);
    Q_UNUSED(mappingstartmodeEnum);

    if (KEYMAP_IDLE == m_KeyMapStatus){
#ifdef CYCLECHECKTIMER_ENABLED
        m_CycleCheckTimer.start(CYCLE_CHECK_TIMEOUT);
#endif
        ui->keymapButton->setText(tr("MappingStop"));
        m_KeyMapStatus = KEYMAP_CHECKING;
        startWinEventHook();
#ifdef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
        s_CycleCheckLoopCount = 0;
#else
        m_CheckGlobalSettingSwitchTimer.start();
#endif
        updateSystemTrayDisplay();
        emit updateLockStatus_Signal();
        startKeyMap = true;

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[MappingStart]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") -> " << mappingstartmodeEnum.valueToKey(startmode);
#endif
    }
    else{
#ifdef CYCLECHECKTIMER_ENABLED
        m_CycleCheckTimer.stop();
#endif
        ui->keymapButton->setText(tr("MappingStart"));

        if (KEYMAP_MAPPING_MATCHED == m_KeyMapStatus) {
            playStopSound();
        }
        stopWinEventHook();
        setKeyUnHook();
        m_KeyMapStatus = KEYMAP_IDLE;
        mappingStopNotification();
#ifdef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
        s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
#else
        m_CheckGlobalSettingSwitchTimer.stop();
#endif
        emit updateLockStatus_Signal();

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[MappingStart]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") -> " << mappingstartmodeEnum.valueToKey(startmode);
#endif
    }

    if (m_KeyMapStatus != KEYMAP_IDLE){
        closeSelectColorDialog();
        closeTableSetupDialog();
        closeFloatingWindowSetupDialog();
        closeItemSetupDialog();
        closeCrosshairSetupDialog();
        closeGyro2MouseAdvancedSettingDialog();
        closeTrayIconSelectDialog();
        closeNotificationSetupDialog();
        changeControlEnableStatus(false);

        QString curSettingSelectStr;
        int curSettingSelectIndex = ui->settingselectComboBox->currentIndex();
        if (GLOBALSETTING_INDEX < curSettingSelectIndex && curSettingSelectIndex < m_SettingSelectListWithoutDescription.size()) {
            curSettingSelectStr = m_SettingSelectListWithoutDescription.at(curSettingSelectIndex);
            if (ui->settingNameLineEdit->text() != curSettingSelectStr) {
                ui->settingNameLineEdit->setText(curSettingSelectStr);
            }
        }
    }
    else{
        changeControlEnableStatus(true);
#ifdef VIGEM_CLIENT_SUPPORT
        emit updateViGEmBusStatus_Signal();
#endif
    }

    if (true == startKeyMap) {
        matchForegroundWindow();
    }
}

#if 0
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

    MappingSwitch(MAPPINGSTART_HOTKEY);

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
#endif

static void BringWindowToTopEx(HWND hwnd)
{
    AttachThreadInput(GetWindowThreadProcessId(GetForegroundWindow(), NULL), GetCurrentThreadId(), true);
    BringWindowToTop(hwnd);
    // SetForegroundWindow(hwnd);
    AttachThreadInput(GetWindowThreadProcessId(GetForegroundWindow(), NULL), GetCurrentThreadId(), false);
}

void QKeyMapper::HotKeyDisplaySwitchActivated(const QString &hotkey_string)
{
    Q_UNUSED(hotkey_string);
#ifdef DEBUG_LOGOUT_ON
    QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
    qDebug().nospace() << "[HotKeyDisplaySwitchActivated] DisplaySwitchKey[" << hotkey_string << "] Activated, KeyMapStatus = " << keymapstatusEnum.valueToKey(m_KeyMapStatus);
#endif

    switchShowHide();
}

void QKeyMapper::HotKeyMappingSwitchActivated(const QString &hotkey_string)
{
    if (m_deviceListWindow->isVisible()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
    Q_UNUSED(keymapstatusEnum);
    qDebug().nospace() << "[HotKeyMappingSwitchActivated] MappingSwitchKey[" << hotkey_string << "] Activated, KeyMapStatus = " << keymapstatusEnum.valueToKey(m_KeyMapStatus);
#endif

    MappingSwitch(MAPPINGSTART_HOTKEY);

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
            if (hotkey_string.contains("Alt")) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[HotKeyMappingSwitchActivated] AltModifier & ForegroundWindow Process -> " << filename << ", isVisibleWindow = " << isVisibleWindow << ", isExToolWindow = " << isExToolWindow;
#endif
                const Qt::KeyboardModifiers modifiers_arg = Qt::AltModifier;
                QString original_key = QString(KEYBOARD_MODIFIERS);
                QKeyMapper_Worker::getInstance()->releaseKeyboardModifiers(modifiers_arg, original_key);
            }
        }
    }
    /* Add for "explorer.exe" AltModifier Bug Fix <<< */
}

void QKeyMapper::HotKeyMappingStart(const QString &hotkey_string)
{
#ifdef DEBUG_LOGOUT_ON
    QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
    qDebug().nospace() << "[HotKeyMappingStart] MappingStartKey[" << hotkey_string << "], KeyMapStatus = " << keymapstatusEnum.valueToKey(m_KeyMapStatus);
#endif

    if (KEYMAP_IDLE == m_KeyMapStatus){
        HotKeyMappingSwitchActivated(hotkey_string);
    }
}

void QKeyMapper::HotKeyMappingStop(const QString &hotkey_string)
{
#ifdef DEBUG_LOGOUT_ON
    QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
    qDebug().nospace() << "[HotKeyMappingStop] MappingStopKey[" << hotkey_string << "], KeyMapStatus = " << keymapstatusEnum.valueToKey(m_KeyMapStatus);
#endif

    if (KEYMAP_IDLE != m_KeyMapStatus){
        HotKeyMappingSwitchActivated(hotkey_string);
    }
}

void QKeyMapper::HotKeyMappingTableSwitchTab(const QString &hotkey_string)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[HotKeyMappingTableSwitchTab] TabHotkey:" << hotkey_string << "->" << s_MappingTableTabHotkeyMap.value(hotkey_string) << ", LastTabIndex:" << s_KeyMappingTabWidgetCurrentIndex;
#endif

    bool isSame = false;
    int tabindex_toswitch = tabIndexToSwitchByTabHotkey(hotkey_string, &isSame);

    if (tabindex_toswitch >= 0) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[HotKeyMappingTableSwitchTab] Switch tab index(" << s_KeyMappingTabWidgetCurrentIndex << "->" << tabindex_toswitch << ")";
#endif
        clearLockStatusDisplay();
        forceSwitchKeyMappingTabWidgetIndex(tabindex_toswitch);
        updateCategoryFilterByShowCategoryState();

        if (m_KeyMapStatus == KEYMAP_MAPPING_MATCHED
            || m_KeyMapStatus == KEYMAP_MAPPING_GLOBAL) {
            /* Key Mapping Restart */
            setKeyMappingRestart();
            mappingTabSwitchNotification(false);
        }
    }
    else {
        if (isSame) {
            if (m_KeyMapStatus == KEYMAP_MAPPING_MATCHED
                || m_KeyMapStatus == KEYMAP_MAPPING_GLOBAL) {
                mappingTabSwitchNotification(true);
            }
        }
    }
}

void QKeyMapper::switchKeyMappingTabIndex(int index)
{
    if (0 <= index && index < s_KeyMappingTabInfoList.size()) {
        m_KeyMappingDataTable = qobject_cast<KeyMappingDataTableWidget*>(m_KeyMappingTabWidget->widget(index));
        lastKeyMappingDataList = KeyMappingDataList;
        KeyMappingDataList = s_KeyMappingTabInfoList.at(index).KeyMappingData;
        s_KeyMappingTabWidgetLastIndex = s_KeyMappingTabWidgetCurrentIndex;
        s_KeyMappingTabWidgetCurrentIndex = index;
    }
}

bool QKeyMapper::addTabToKeyMappingTabWidget(const QString& customTabName)
{
    int tab_count = m_KeyMappingTabWidget->count();
    int insert_index = tab_count;  // Insert at the end instead of before the "+" tab

    // Determine tab name based on custom name or default naming scheme
    QString tabName = customTabName.isEmpty() ? QString("%1%2").arg(MAPPINGTABLE_TAB_TEXT).arg(insert_index + 1) : customTabName;

    // Check if tabName already exists. If a duplicate is found, generate a unique name in the format "tabName(001~999)"
    if (isTabTextDuplicate(tabName)) {
        bool uniqueNameFound = false;
        for (int i = 1; i <= 999; ++i) {
            QString tempName = QString("%1(%2)").arg(tabName).arg(i, 3, 10, QChar('0'));
            if (!isTabTextDuplicate(tempName)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[addTabToKeyMappingTabWidget] TabName:" << tabName << " is already exists, set a unique tabname:" << tempName;
#endif
                tabName = tempName;
                uniqueNameFound = true;
                break;
            }
        }
        // If no unique name is found after checking all possible values (001~999), return false
        if (!uniqueNameFound) {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[addTabToKeyMappingTabWidget] Can not found unique name for TabName:" << tabName << ", return false";
#endif
            return false;
        }
    }

    KeyMappingDataTableWidget *KeyMappingTableWidget = new KeyMappingDataTableWidget(this);

    int left = KEYMAPPINGDATATABLE_NARROW_LEFT;
    int width = KEYMAPPINGDATATABLE_NARROW_WIDTH;
    int top = KEYMAPPINGDATATABLE_TOP;
    int height = KEYMAPPINGDATATABLE_HEIGHT;
    if (!ui->processListButton->isChecked()) {
        left    = KEYMAPPINGDATATABLE_WIDE_LEFT;
        width   = KEYMAPPINGDATATABLE_WIDE_WIDTH;
    }
    KeyMappingTableWidget->setGeometry(QRect(left, top, width, height));

    KeyMappingTableWidget->setFocusPolicy(Qt::NoFocus);
    KeyMappingTableWidget->setColumnCount(KEYMAPPINGDATA_TABLE_COLUMN_COUNT);

    // Initialize category column visibility based on current button state
    KeyMappingTableWidget->setCategoryColumnVisible(ui->showCategoryButton->isChecked());

    KeyMappingTableWidget->horizontalHeader()->setStretchLastSection(true);
    KeyMappingTableWidget->horizontalHeader()->setHighlightSections(false);

    resizeKeyMappingDataTableColumnWidth(KeyMappingTableWidget);

    KeyMappingTableWidget->verticalHeader()->setVisible(false);
    KeyMappingTableWidget->verticalHeader()->setDefaultSectionSize(25);
    KeyMappingTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    KeyMappingTableWidget->setSelectionMode(QAbstractItemView::ContiguousSelection);
    // Allow editing only for specific columns (will be controlled per item)
    KeyMappingTableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);

    /* Suuport Drag&Drop for KeyMappingData Table */
    KeyMappingTableWidget->setDragEnabled(true);
    KeyMappingTableWidget->setDragDropMode(QAbstractItemView::InternalMove);

    KeyMappingTableWidget->setHorizontalHeaderLabels(QStringList()  << tr("OriginalKey")
                                                                    << tr("MappingKey")
                                                                    << tr("Burst")
                                                                    << tr("Lock")
                                                                    << tr("Category"));
    QFont customFont(FONTNAME_ENGLISH, 9);
    KeyMappingTableWidget->setFont(customFont);
    KeyMappingTableWidget->horizontalHeader()->setFont(customFont);

#ifdef DEBUG_LOGOUT_ON
    // qDebug() << "verticalHeader->isVisible" << KeyMappingTableWidget->verticalHeader()->isVisible();
    // qDebug() << "selectionBehavior" << KeyMappingTableWidget->selectionBehavior();
    // qDebug() << "selectionMode" << KeyMappingTableWidget->selectionMode();
    // qDebug() << "editTriggers" << KeyMappingTableWidget->editTriggers();
    // qDebug() << "verticalHeader-DefaultSectionSize" << KeyMappingTableWidget->verticalHeader()->defaultSectionSize();
#endif

    KeyMappingTableWidget->setStyle(QStyleFactory::create("Fusion"));

    // Add the new tab at the end with the generated tabName
    m_KeyMappingTabWidget->addTab(KeyMappingTableWidget, tabName);

    KeyMappingTab_Info tab_info;
    QList<MAP_KEYDATA> *keyMappingData = new QList<MAP_KEYDATA>();
    tab_info.TabName = tabName;
    tab_info.TabCustomImage_ShowPosition = TAB_CUSTOMIMAGE_POSITION_DEFAULT;
    tab_info.TabCustomImage_Padding = TAB_CUSTOMIMAGE_PADDING_DEFAULT;
    tab_info.TabCustomImage_ShowAsTrayIcon = TAB_CUSTOMIMAGE_SHOW_AS_TRAYICON_DEFAULT;
    tab_info.TabCustomImage_ShowAsFloatingWindow = TAB_CUSTOMIMAGE_SHOW_AS_FLOATINGWINDOW_DEFAULT;
    tab_info.FloatingWindow_ReferencePoint = FLOATINGWINDOW_REFERENCEPOINT_DEFAULT;
    tab_info.FloatingWindow_Radius = FLOATINGWINDOW_RADIUS_DEFAULT;
    tab_info.FloatingWindow_Opacity = FLOATINGWINDOW_OPACITY_DEFAULT;
    tab_info.FloatingWindow_MousePassThrough = FLOATINGWINDOW_MOUSE_PASSTHROUGH_DEFAULT;
    tab_info.KeyMappingDataTable = KeyMappingTableWidget;
    tab_info.KeyMappingData = keyMappingData;

    s_KeyMappingTabInfoList.append(tab_info);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[addTabToKeyMappingTabWidget] Add a new tab with TabName:" << tabName;
#endif
    return true;
}

int QKeyMapper::removeTabFromKeyMappingTabWidget(int tabindex)
{
    if (m_KeyMappingTabWidget->count() <= 1 || s_KeyMappingTabInfoList.size() <= 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[removeTabFromKeyMappingTabWidget] Can not remove the last tab!" << " ValidTabWidgetCount:" << m_KeyMappingTabWidget->count() << ", TabInfoListSize:" << s_KeyMappingTabInfoList.size();
#endif
        return REMOVE_MAPPINGTAB_LASTONE;
    }

    if ((tabindex < 0) || (tabindex >= m_KeyMappingTabWidget->count()) || (tabindex >= s_KeyMappingTabInfoList.size())) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[removeTabFromKeyMappingTabWidget] Invalid index : " << tabindex << ", ValidTabWidgetCount:" << m_KeyMappingTabWidget->count() << ", TabInfoListSize:" << s_KeyMappingTabInfoList.size();
#endif
        return REMOVE_MAPPINGTAB_FAILED;
    }

    m_KeyMappingTabWidget->blockSignals(true);
    disconnectKeyMappingDataTableConnection();

    m_KeyMappingTabWidget->removeTab(tabindex);
    if (s_KeyMappingTabInfoList.at(tabindex).KeyMappingDataTable != Q_NULLPTR) {
        delete s_KeyMappingTabInfoList.at(tabindex).KeyMappingDataTable;
    }
    if (s_KeyMappingTabInfoList.at(tabindex).KeyMappingData != Q_NULLPTR) {
        delete s_KeyMappingTabInfoList.at(tabindex).KeyMappingData;
    }
    s_KeyMappingTabInfoList.removeAt(tabindex);

    if (s_KeyMappingTabWidgetCurrentIndex > s_KeyMappingTabInfoList.size() - 1) {
        s_KeyMappingTabWidgetCurrentIndex = s_KeyMappingTabInfoList.size() - 1;
    }

    setKeyMappingTabWidgetCurrentIndex(s_KeyMappingTabWidgetCurrentIndex);
    switchKeyMappingTabIndex(s_KeyMappingTabWidgetCurrentIndex);
    updateKeyMappingDataTableConnection();

    m_KeyMappingTabWidget->blockSignals(false);

    return REMOVE_MAPPINGTAB_SUCCESS;
}

// Tab reordering methods implementation

void QKeyMapper::moveTabInKeyMappingTabWidget(int from, int to)
{
    // Check if we're in IDLE state and indexes are valid
    if (m_KeyMapStatus != KEYMAP_IDLE) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[moveTabInKeyMappingTabWidget] Cannot move tab while not in IDLE state";
#endif
        return;
    }

    // Validate indexes
    int lastValidIndex = s_KeyMappingTabInfoList.size() - 1;
    if (from < 0 || from > lastValidIndex || to < 0 || to > lastValidIndex) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[moveTabInKeyMappingTabWidget] Invalid indexes: from=" << from << "to=" << to << "lastValidIndex=" << lastValidIndex;
#endif
        return;
    }

    if (from == to) {
        return; // No need to move
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[moveTabInKeyMappingTabWidget] Moving tab from" << from << "to" << to;
#endif

    // Block signals to prevent recursion
    m_KeyMappingTabWidget->blockSignals(true);
    disconnectKeyMappingDataTableConnection();

    // Move the tab info data
    KeyMappingTab_Info tabInfo = s_KeyMappingTabInfoList.takeAt(from);
    s_KeyMappingTabInfoList.insert(to, tabInfo);

    // Update current and last index trackers
    if (s_KeyMappingTabWidgetCurrentIndex == from) {
        s_KeyMappingTabWidgetCurrentIndex = to;
    }
    else if (from < s_KeyMappingTabWidgetCurrentIndex && to >= s_KeyMappingTabWidgetCurrentIndex) {
        s_KeyMappingTabWidgetCurrentIndex--;
    }
    else if (from > s_KeyMappingTabWidgetCurrentIndex && to <= s_KeyMappingTabWidgetCurrentIndex) {
        s_KeyMappingTabWidgetCurrentIndex++;
    }

    if (s_KeyMappingTabWidgetLastIndex == from) {
        s_KeyMappingTabWidgetLastIndex = to;
    }
    else if (from < s_KeyMappingTabWidgetLastIndex && to >= s_KeyMappingTabWidgetLastIndex) {
        s_KeyMappingTabWidgetLastIndex--;
    }
    else if (from > s_KeyMappingTabWidgetLastIndex && to <= s_KeyMappingTabWidgetLastIndex) {
        s_KeyMappingTabWidgetLastIndex++;
    }

    // Ensure we stay on the same logical tab after move
    setKeyMappingTabWidgetCurrentIndex(s_KeyMappingTabWidgetCurrentIndex);
    switchKeyMappingTabIndex(s_KeyMappingTabWidgetCurrentIndex);
    updateKeyMappingDataTableConnection();

    m_KeyMappingTabWidget->blockSignals(false);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[moveTabInKeyMappingTabWidget] Tab moved successfully, new current index:" << s_KeyMappingTabWidgetCurrentIndex;
#endif

    collectMappingTableTabHotkeys();
}

int QKeyMapper::copySelectedKeyMappingDataToCopiedList()
{
    int copied_count = -1;
    QList<QTableWidgetSelectionRange> selectedRanges = m_KeyMappingDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[DeleteItem] There is no selected item";
#endif
        return copied_count;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int top_row = range.topRow();
    int bottom_row = range.bottomRow();

    s_CopiedMappingData.clear();
    for (int row = top_row; row <= bottom_row; ++row) {
        s_CopiedMappingData.append(KeyMappingDataList->at(row));
    }
    copied_count = s_CopiedMappingData.size();

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[copySelectedKeyMappingDataToCopiedList] Ctrl+C pressed, copy(" << copied_count << ") selected keymapping data to s_CopiedMappingData -> " << s_CopiedMappingData;
#endif

    return copied_count;
}

int QKeyMapper::insertKeyMappingDataFromCopiedList()
{
    int inserted_count = -1;
    if (s_CopiedMappingData.isEmpty()) {
        return inserted_count;
    }

    QList<MAP_KEYDATA> insertMappingDataList;
    for (const MAP_KEYDATA &keymapdata : s_CopiedMappingData) {
        int findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(keymapdata.Original_Key);
        if (findindex != -1) {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[insertKeyMappingDataFromCopiedList]" << "Duplicate original key found -> index : " << findindex << ", originalkey : " << keymapdata.Original_Key;
#endif
            continue;
        }
        insertMappingDataList.append(keymapdata);
    }

    inserted_count = insertMappingDataList.size();
    if (insertMappingDataList.isEmpty()) {
        return inserted_count;
    }

    bool insertToEnd = false;
    int insertRow = -1;
    QList<QTableWidgetSelectionRange> selectedRanges = m_KeyMappingDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[insertKeyMappingDataFromCopiedList] There is no selected item, insert to the end.";
#endif
        insertToEnd = true;
    }
    else {
        // Get the selected top row
        QTableWidgetSelectionRange range = selectedRanges.first();
        insertRow = range.topRow();
    }

    if (insertToEnd) {
        KeyMappingDataList->append(insertMappingDataList);
    }
    else {
        // Insert the rows at the new position on reverse order
        for (int i = insertMappingDataList.size() - 1; i >= 0; --i) {
            KeyMappingDataList->insert(insertRow, insertMappingDataList.at(i));
        }
    }

    // refresh table display
    refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);
    // Update the mouse points list
    updateMousePointsList();

    // Reselect inserted rows
    if (inserted_count > 0) {
        int startRow = insertToEnd ? m_KeyMappingDataTable->rowCount() - inserted_count : insertRow;
        int endRow = startRow + inserted_count - 1;
        QTableWidgetSelectionRange newSelection = QTableWidgetSelectionRange(startRow, 0, endRow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        m_KeyMappingDataTable->clearSelection();
        m_KeyMappingDataTable->setRangeSelected(newSelection, true);
    }

#ifdef DEBUG_LOGOUT_ON
    QString debugmessage;
    if (insertToEnd) {
        debugmessage = QString("[copySelectedKeyMappingDataToCopiedList] Ctrl+V pressed, append (%1) items to the end of current mapping table -> ").arg(inserted_count);
    }
    else {
        debugmessage = QString("[copySelectedKeyMappingDataToCopiedList] Ctrl+V pressed, insert (%1) items to row(%2) of current mapping table -> ").arg(inserted_count).arg(insertRow);
    }
    qDebug().nospace().noquote() << debugmessage << insertMappingDataList;
#endif

    return inserted_count;
}

void QKeyMapper::onHotKeyLineEditEditingFinished()
{
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender());
    if (lineEdit)
    {
        QString ori_inputstring = lineEdit->text();
        QString inputstring = ori_inputstring;
        if (inputstring.startsWith(PREFIX_PASSTHROUGH)) {
            inputstring.remove(0, 1);
        }
        if (validateCombinationKey(inputstring))
        {
            if (lineEdit->objectName() == WINDOWSWITCHKEY_LINEEDIT_NAME) {
                updateWindowSwitchKeyString(ori_inputstring);
            }
            else if (lineEdit->objectName() == MAPPINGSTARTKEY_LINEEDIT_NAME) {
                updateMappingStartKeyString(ori_inputstring);
            }
            else if (lineEdit->objectName() == MAPPINGSTOPKEY_LINEEDIT_NAME) {
                updateMappingStopKeyString(ori_inputstring);
            }
        }
        else
        {
            if (lineEdit->objectName() == WINDOWSWITCHKEY_LINEEDIT_NAME) {
                lineEdit->setText(s_WindowSwitchKeyString);
                showFailurePopup(tr("Invalid input format for WindowSwitchKey!"));
            }
            else if (lineEdit->objectName() == MAPPINGSTARTKEY_LINEEDIT_NAME) {
                lineEdit->setText(s_MappingStartKeyString);
                showFailurePopup(tr("Invalid input format for MappingStartKey!"));
            }
            else if (lineEdit->objectName() == MAPPINGSTOPKEY_LINEEDIT_NAME) {
                lineEdit->setText(s_MappingStopKeyString);
                showFailurePopup(tr("Invalid input format for MappingStopKey!"));
            }
        }
    }
}

#if 0
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
#endif

void QKeyMapper::SystrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (QSystemTrayIcon::DoubleClick == reason) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[SystrayIconActivated]" << "SystemTray double clicked: switch show or hide!";
#endif
        switchShowHide();
    }
    else if (QSystemTrayIcon::Context == reason) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[SystrayIconActivated]" << "SystemTray right clicked: context menu display!";
#endif
        updateSysTrayIconMenuText();

        QPoint pos = QCursor::pos();
        int menu_height = m_SysTrayIconMenu->sizeHint().height();
        pos.setY(pos.y() - menu_height);
        m_SysTrayIconMenu->popup(pos);
    }
}

void QKeyMapper::onTrayIconMenuShowHideAction()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onTrayIconMenuShowHideAction]" << "ShowHideAction Triggered.";
#endif

    switchShowHide();
}

void QKeyMapper::onTrayIconMenuQuitAction()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onTrayIconMenuQuitAction]" << "QuitAction Triggered.";
#endif

    QApplication::quit();
}

void QKeyMapper::cellChanged_slot(int row, int col)
{
#if 0
    if ((col == BURST_MODE_COLUMN || col == LOCK_COLUMN)
            && m_KeyMappingDataTable->item(row, col)->checkState() == Qt::Checked) {
        if (KeyMappingDataList[row].Mapping_Keys.size() > 1) {
            KeyMappingDataList[row].Burst = false;
            KeyMappingDataList[row].Lock = false;
            QTableWidgetItem *burstCheckBox = new QTableWidgetItem();
            burstCheckBox->setCheckState(Qt::Unchecked);
            m_KeyMappingDataTable->setItem(row, BURST_MODE_COLUMN    , burstCheckBox);
            QTableWidgetItem *lockCheckBox = new QTableWidgetItem();
            lockCheckBox->setCheckState(Qt::Unchecked);
            m_KeyMappingDataTable->setItem(row, LOCK_COLUMN    , lockCheckBox);

            QString message = "Key sequence with \"»\" do not support Burst or Lock mode!";
            QMessageBox::warning(this, PROGRAM_NAME, tr(message.toStdString().c_str()));
#ifdef DEBUG_LOGOUT_ON
            qDebug("[%s]: row(%d) could not set burst or lock for key sequence(%d)", __func__, row, KeyMappingDataList[row].Mapping_Keys.size());
#endif
            return;
        }
    }
#endif

    int row_count = QKeyMapper::KeyMappingDataList->size();
    if (row >= row_count || row < 0) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("\033[1;31m[%s]: row(%d) out of range, row_count(%d), col(%d)\033[0m", __func__, row, row_count, col);
#endif
        return;
    }

    if (col == BURST_MODE_COLUMN) {
        bool burst = false;
        if (m_KeyMappingDataTable->item(row, col)->checkState() == Qt::Checked) {
            burst = true;
        }
        else {
            burst = false;
        }

        if (burst != KeyMappingDataList->at(row).Burst) {
            (*KeyMappingDataList)[row].Burst = burst;
#ifdef DEBUG_LOGOUT_ON
            qDebug("[%s]: row(%d) burst changed to (%s)", __func__, row, burst == true?"ON":"OFF");
#endif
        }
    }
    else if (col == LOCK_COLUMN) {
        bool lock = false;
        if (m_KeyMappingDataTable->item(row, col)->checkState() == Qt::Checked) {
            lock = true;
        }
        else {
            lock = false;
        }

        if (lock != KeyMappingDataList->at(row).Lock) {
            (*KeyMappingDataList)[row].Lock = lock;
#ifdef DEBUG_LOGOUT_ON
            qDebug("[%s]: row(%d) lock changed to (%s)", __func__, row, lock == true?"ON":"OFF");
#endif
        }
    }
    else if (col == CATEGORY_COLUMN) {
        QString category = m_KeyMappingDataTable->item(row, col)->text();
        if (category != KeyMappingDataList->at(row).Category) {
            (*KeyMappingDataList)[row].Category = category;
#ifdef DEBUG_LOGOUT_ON
            qDebug("[%s]: row(%d) category changed to (%s)", __func__, row, category.toLocal8Bit().constData());
#endif
            // Update category filter ComboBox
            if (m_KeyMappingDataTable->isCategoryColumnVisible()) {
                updateCategoryFilterComboBox();
            }
        }
    }
}

void QKeyMapper::OrikeyComboBox_currentTextChangedSlot(const QString &text)
{
    if (VJOY_MOUSE2LS_STR == text
        || VJOY_MOUSE2RS_STR == text
        || JOY_LS2MOUSE_STR == text
        || JOY_RS2MOUSE_STR == text
        || JOY_GYRO2MOUSE_STR == text
        || JOY_LS2VJOYLS_STR == text
        || JOY_RS2VJOYRS_STR == text
        || JOY_LS2VJOYRS_STR == text
        || JOY_RS2VJOYLS_STR == text
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

    if (!text.isEmpty()) {
        m_orikeyComboBox->setToolTip(text);
    }
}

void QKeyMapper::MapkeyComboBox_currentTextChangedSlot(const QString &text)
{
    if (!text.isEmpty()) {
        m_mapkeyComboBox->setToolTip(text);
    }
}

#ifdef SETTINGSFILE_CONVERT
bool QKeyMapper::checkSettingsFileNeedtoConvert()
{
    QFile file(CONFIG_FILENAME);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("[checkSettingsFileNeedtoConvert] Could not open the file for reading");
#endif
        return false;
    }

    QStringList fileContent;
    QTextStream in(&file);
    while (!in.atEnd()) {
        fileContent << in.readLine();
    }
    file.close();

    static QRegularExpression keymapdata_regex("^KeyMapData_.+=.+, .+$");

    for (int i = 0; i < fileContent.size(); ++i) {
        QString &line = fileContent[i];
        QRegularExpressionMatch match = keymapdata_regex.match(line);
        if (match.hasMatch()) {
            return true;
        }
    }

    return false;
}

void QKeyMapper::convertSettingsFile()
{
    QFile file(CONFIG_FILENAME);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("[convertSettingsFile] Could not open the file for reading");
#endif
        return;
    }

    QStringList fileContent;
    QTextStream in(&file);
    while (!in.atEnd()) {
        fileContent << in.readLine();
    }
    file.close();

    static QRegularExpression keymapdata_regex("^KeyMapData_.+=.+, .+$");
    bool modified = false;

    for (int i = 0; i < fileContent.size(); ++i) {
        QString &line = fileContent[i];
        QRegularExpressionMatch match = keymapdata_regex.match(line);
        if (match.hasMatch()) {
            line.replace(", ", SEPARATOR_KEYMAPDATA_LEVEL1);
            modified = true;
        }
    }

    if (modified) {
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            qWarning("Could not open the file for writing");
            return;
        }

        QTextStream out(&file);
        for (const QString &line : fileContent) {
            out << line << "\n";
        }
        file.close();
    }

    return;
}
#endif

#if 0
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
#endif

QString QKeyMapper::matchAutoStartSaveSettings(const QString &processpath, const QString &windowtitle)
{
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    QStringList groups = settingFile.childGroups();
    groups.removeOne(GROUPNAME_GLOBALSETTING);

    // Store candidates for fallback matching (lower priority)
    QString processOnlyMatch;
    QString windowTitleOnlyMatch;

    for (const QString &group : std::as_const(groups)) {
        // Skip groups without autoStartMapping enabled
        Qt::CheckState autoStartMappingChecked = Qt::Unchecked;
        QVariant autoStartMappingChecked_Var;
        if (readSaveSettingData(group, AUTOSTARTMAPPING_CHECKED, autoStartMappingChecked_Var)) {
            autoStartMappingChecked = (Qt::CheckState)autoStartMappingChecked_Var.toInt();
        }
        if (autoStartMappingChecked != Qt::Checked) {
            continue;
        }

        QString tempSettingSelectStr = group + "/";

        // Get match indices
        bool ok = false;
        int matchProcessIndex = settingFile.value(tempSettingSelectStr+PROCESSINFO_FILENAME_MATCH_INDEX).toInt(&ok);
        if (!ok || matchProcessIndex < WINDOWINFO_MATCH_INDEX_MIN || matchProcessIndex > WINDOWINFO_MATCH_INDEX_MAX) {
            matchProcessIndex = WINDOWINFO_MATCH_INDEX_DEFAULT;
        }

        ok = false;
        int matchWindowTitleIndex = settingFile.value(tempSettingSelectStr+PROCESSINFO_WINDOWTITLE_MATCH_INDEX).toInt(&ok);
        if (!ok || matchWindowTitleIndex < WINDOWINFO_MATCH_INDEX_MIN || matchWindowTitleIndex > WINDOWINFO_MATCH_INDEX_MAX) {
            matchWindowTitleIndex = WINDOWINFO_MATCH_INDEX_DEFAULT;
        }

        // Get match strings
        QString processNameString = settingFile.value(tempSettingSelectStr + PROCESSINFO_FILENAME).toString();
        QString windowTitleString = settingFile.value(tempSettingSelectStr + PROCESSINFO_WINDOWTITLE).toString();

        // Determine what needs to match
        bool matchProcess = (matchProcessIndex != WINDOWINFO_MATCH_INDEX_IGNORE && !processNameString.isEmpty());
        bool matchWindowTitle = (matchWindowTitleIndex != WINDOWINFO_MATCH_INDEX_IGNORE && !windowTitleString.isEmpty());

        if (!matchProcess && !matchWindowTitle) {
            continue; // Skip groups that don't require matching
        }

        // Helper function to check process matching
        auto checkProcessMatch = [&]() -> bool {
            if (processpath.isEmpty() || processNameString.isEmpty()) {
                return false;
            }

            if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_EQUALS) {
                return processpath == processNameString;
            }
            else if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_CONTAINS) {
                return processpath.contains(processNameString);
            }
            else if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_STARTSWITH) {
                return processpath.startsWith(processNameString);
            }
            else if (matchProcessIndex == WINDOWINFO_MATCH_INDEX_ENDSWITH) {
                return processpath.endsWith(processNameString);
            }
            return false;
        };

        // Helper function to check window title matching
        auto checkWindowTitleMatch = [&]() -> bool {
            if (windowtitle.isEmpty() || windowTitleString.isEmpty()) {
                return false;
            }

            if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_EQUALS) {
                return windowtitle == windowTitleString;
            }
            else if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_CONTAINS) {
                return windowtitle.contains(windowTitleString);
            }
            else if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_STARTSWITH) {
                return windowtitle.startsWith(windowTitleString);
            }
            else if (matchWindowTitleIndex == WINDOWINFO_MATCH_INDEX_ENDSWITH) {
                return windowtitle.endsWith(windowTitleString);
            }
            return false;
        };

        // Priority 1: Both process and window title need to match
        if (matchProcess && matchWindowTitle) {
            if (checkProcessMatch() && checkWindowTitleMatch()) {
                return group;
            }
        }

        // Store candidates for lower priority matches
        // Priority 2: Only process matches (store first match)
        if (processOnlyMatch.isEmpty() && matchProcess && !matchWindowTitle) {
            if (checkProcessMatch()) {
                processOnlyMatch = group;
            }
        }

        // Priority 3: Only window title matches (store first match)
        if (windowTitleOnlyMatch.isEmpty() && !matchProcess && matchWindowTitle) {
            if (checkWindowTitleMatch()) {
                windowTitleOnlyMatch = group;
            }
        }
    }

    // Return fallback matches in priority order
    if (!processOnlyMatch.isEmpty()) {
        return processOnlyMatch;
    }

    if (!windowTitleOnlyMatch.isEmpty()) {
        return windowTitleOnlyMatch;
    }

    // No match found
    return QString();
}

#if 0
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
#endif

QString QKeyMapper::matchSavedSettings(const QString &processpath, const QString &windowtitle)
{
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    QStringList groups = settingFile.childGroups();
    groups.removeOne(GROUPNAME_GLOBALSETTING);

    // Store candidates for fallback matching (lower priority)
    QString processPathOnlyMatch;
    QString windowTitleContainsMatch;

    for (const QString &group : std::as_const(groups)) {
        QString tempSettingSelectStr = group + "/";

        QString filepathString = settingFile.value(tempSettingSelectStr+PROCESSINFO_FILEPATH).toString();
        QString windowtitleString = settingFile.value(tempSettingSelectStr+PROCESSINFO_WINDOWTITLE).toString();
        bool filepathNeedMatch = false;
        bool windowtitleNeedMatch = false;
        if (!filepathString.trimmed().isEmpty()) {
            filepathNeedMatch = true;
        }
        if (!windowtitleString.trimmed().isEmpty()) {
            windowtitleNeedMatch = true;
        }

        if (!filepathNeedMatch && !windowtitleNeedMatch) {
            continue; // Skip groups that don't require matching
        }

        // Priority 1: Exact match for both processpath and windowtitle
        if (filepathNeedMatch && windowtitleNeedMatch
            && processpath == filepathString && windowtitle == windowtitleString) {
            return group;
        }

        // Priority 2: Exact processpath match and windowtitle contains windowtitleString
        if (filepathNeedMatch && windowtitleNeedMatch
            && processpath == filepathString && windowtitle.contains(windowtitleString)) {
            return group;
        }

        // Store candidates for lower priority matches
        // Priority 3: Only processpath matches (store first match)
        if (processPathOnlyMatch.isEmpty() && filepathNeedMatch && processpath == filepathString) {
            processPathOnlyMatch = group;
        }

        // Priority 4: Only windowtitle contains match (store first match)
        if (windowTitleContainsMatch.isEmpty() && windowtitleNeedMatch && windowtitle == windowtitleString) {
            windowTitleContainsMatch = group;
        }
    }

    // Return fallback matches in priority order
    if (!processPathOnlyMatch.isEmpty()) {
        return processPathOnlyMatch;
    }

    if (!windowTitleContainsMatch.isEmpty()) {
        return windowTitleContainsMatch;
    }

    // No match found
    return QString();
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
    // Create RAII guard to automatically save and restore category filter state
    CategoryFilterStateGuard filterGuard(this);

    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    settingFile.setIniCodec("UTF-8");
#endif
    // int burstpressTime = ui->burstpressSpinBox->value();
    // int burstreleaseTime = ui->burstreleaseSpinBox->value();
#ifdef VIGEM_CLIENT_SUPPORT
    int vJoy_X_Sensitivity = ui->vJoyXSensSpinBox->value();
    int vJoy_Y_Sensitivity = ui->vJoyYSensSpinBox->value();
    int vJoy_Recenter_Timeout = ui->vJoyRecenterSpinBox->value();
#endif

    QString saveSettingSelectStr;
    // QString cursettingSelectStr = ui->settingselectComboBox->currentText();
    QString cursettingSelectStr;
    int curSettingSelectIndex = ui->settingselectComboBox->currentIndex();
    if (0 < curSettingSelectIndex && curSettingSelectIndex < m_SettingSelectListWithoutDescription.size()) {
        cursettingSelectStr = m_SettingSelectListWithoutDescription.at(curSettingSelectIndex);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().noquote().nospace() << "[saveKeyMapSetting]" << "Current setting select index is invalid("<< curSettingSelectIndex << "), m_SettingSelectListWithoutDescription ->" << m_SettingSelectListWithoutDescription;
#endif
    }
    int languageIndex = ui->languageComboBox->currentIndex();
    bool saveGlobalSetting = false;

    m_LastWindowPosition = pos();
    settingFile.setValue(LAST_WINDOWPOSITION, m_LastWindowPosition);

    QString productVersion = getExeProductVersion();
    QString platformString = getPlatformString();
    if (!productVersion.isEmpty()) {
        settingFile.setValue(PROGRAM_VERSION, productVersion);
    }
    if (!platformString.isEmpty()) {
        settingFile.setValue(PROGRAM_PLATFORM, platformString);
    }

    if (LANGUAGE_ENGLISH == languageIndex) {
        settingFile.setValue(LANGUAGE_INDEX , LANGUAGE_ENGLISH);
    }
    else if (LANGUAGE_JAPANESE == languageIndex) {
        settingFile.setValue(LANGUAGE_INDEX , LANGUAGE_JAPANESE);
    }
    else {
        settingFile.setValue(LANGUAGE_INDEX , LANGUAGE_CHINESE);
    }

    settingFile.setValue(PLAY_SOUNDEFFECT, ui->soundEffectCheckBox->isChecked());
    settingFile.setValue(STARTUP_MINIMIZED, ui->startupMinimizedCheckBox->isChecked());
    settingFile.setValue(STARTUP_AUTOMONITORING, ui->startupAutoMonitoringCheckBox->isChecked());
    settingFile.setValue(SHOW_PROCESSLIST, ui->processListButton->isChecked());
    settingFile.setValue(SHOW_NOTES, ui->showNotesButton->isChecked());
    settingFile.setValue(SHOW_CATEGORYS, ui->showCategoryButton->isChecked());
    settingFile.setValue(NOTIFICATION_POSITION , ui->notificationComboBox->currentIndex());

    QColor notification_fontcolor;
    QString notification_fontcolor_name;
    notification_fontcolor = m_NotificationSetupDialog->getNotification_FontColor();
    if (notification_fontcolor.isValid()) {
        notification_fontcolor_name = notification_fontcolor.name();
    }
    QColor notification_bgcolor;
    QString notification_bgcolor_name;
    notification_bgcolor = m_NotificationSetupDialog->getNotification_BackgroundColor();
    if (notification_bgcolor.isValid()) {
        notification_bgcolor_name = notification_bgcolor.name(QColor::HexArgb);
    }
    settingFile.setValue(NOTIFICATION_FONTCOLOR,        notification_fontcolor_name);
    settingFile.setValue(NOTIFICATION_BACKGROUNDCOLOR,  notification_bgcolor_name);
    // settingFile.setValue(NOTIFICATION_FONTCOLOR,        m_NotificationSetupDialog->getNotification_FontColor());
    // settingFile.setValue(NOTIFICATION_BACKGROUNDCOLOR,  m_NotificationSetupDialog->getNotification_BackgroundColor());
    settingFile.setValue(NOTIFICATION_FONTSIZE,         m_NotificationSetupDialog->getNotification_FontSize());
    settingFile.setValue(NOTIFICATION_FONTWEIGHT,       m_NotificationSetupDialog->getNotification_FontWeight());
    settingFile.setValue(NOTIFICATION_FONTITALIC,       m_NotificationSetupDialog->getNotification_FontIsItalic());
    settingFile.setValue(NOTIFICATION_DISPLAYDURATION,  m_NotificationSetupDialog->getNotification_DisplayDuration());
    settingFile.setValue(NOTIFICATION_FADEINDURATION,   m_NotificationSetupDialog->getNotification_FadeInDuration());
    settingFile.setValue(NOTIFICATION_FADEOUTDURATION,  m_NotificationSetupDialog->getNotification_FadeOutDuration());
    settingFile.setValue(NOTIFICATION_BORDERRADIUS,     m_NotificationSetupDialog->getNotification_BorderRadius());
    settingFile.setValue(NOTIFICATION_PADDING,          m_NotificationSetupDialog->getNotification_Padding());
    settingFile.setValue(NOTIFICATION_OPACITY,          m_NotificationSetupDialog->getNotification_Opacity());
    settingFile.setValue(NOTIFICATION_X_OFFSET,         m_NotificationSetupDialog->getNotification_X_Offset());
    settingFile.setValue(NOTIFICATION_Y_OFFSET,         m_NotificationSetupDialog->getNotification_Y_Offset());

    settingFile.setValue(TRAYICON_IDLE , m_TrayIconSelectDialog->getTrayIcon_IdleStateIcon());
    settingFile.setValue(TRAYICON_MONITORING , m_TrayIconSelectDialog->getTrayIcon_MonitoringStateIcon());
    settingFile.setValue(TRAYICON_GLOBAL , m_TrayIconSelectDialog->getTrayIcon_GlobalStateIcon());
    settingFile.setValue(TRAYICON_MATCHED , m_TrayIconSelectDialog->getTrayIcon_MatchedStateIcon());
    if (UPDATE_SITE_GITEE == ui->updateSiteComboBox->currentIndex()) {
        settingFile.setValue(UPDATE_SITE, UPDATE_SITE_GITEE);
    }
    else {
        settingFile.setValue(UPDATE_SITE, UPDATE_SITE_GITHUB);
    }
    settingFile.setValue(VIRTUALGAMEPAD_TYPE , ui->virtualGamepadTypeComboBox->currentText());
    settingFile.setValue(VIRTUAL_GAMEPADLIST, QKeyMapper_Worker::s_VirtualGamepadList);

//         if (m_windowswitchKeySeqEdit->keySequence().isEmpty()) {
//             if (m_windowswitchKeySeqEdit->lastKeySequence().isEmpty()) {
//                 m_windowswitchKeySeqEdit->setKeySequence(QKeySequence(m_windowswitchKeySeqEdit->defaultKeySequence()));
//             }
//             else {
//                 m_windowswitchKeySeqEdit->setKeySequence(QKeySequence(m_windowswitchKeySeqEdit->lastKeySequence()));
//             }
//         }
//         m_windowswitchKeySeqEdit->clearFocus();
//         if (false == m_windowswitchKeySeqEdit->keySequence().isEmpty()) {
//             updateWindowSwitchKeySeq(m_windowswitchKeySeqEdit->keySequence());
//             settingFile.setValue(WINDOWSWITCH_KEYSEQ, m_windowswitchKeySeqEdit->keySequence().toString());
// #ifdef DEBUG_LOGOUT_ON
//             qDebug() << "[saveKeyMapSetting]" << "Save & Set Window Switch KeySequence ->" << m_windowswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText);
// #endif
//         }

    if (s_WindowSwitchKeyString.isEmpty()) {
        s_WindowSwitchKeyString = DISPLAYSWITCH_KEY_DEFAULT;
    }
    ui->windowswitchkeyLineEdit->clearFocus();
    if (false == s_WindowSwitchKeyString.isEmpty()) {
        settingFile.setValue(WINDOWSWITCH_KEYSEQ, s_WindowSwitchKeyString);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[saveKeyMapSetting]" << "Save & Set Window Switch Key ->" << s_WindowSwitchKeyString;
#endif
    }

    if (cursettingSelectStr == GROUPNAME_GLOBALSETTING && ui->settingselectComboBox->currentIndex() == GLOBALSETTING_INDEX) {
        saveGlobalSetting = true;
        saveSettingSelectStr = cursettingSelectStr;
        settingFile.setValue(SETTINGSELECT , saveSettingSelectStr);
        saveSettingSelectStr = saveSettingSelectStr + "/";
    }
    else {
        QString settingNameString = ui->settingNameLineEdit->text().trimmed();
        QStringList groups = settingFile.childGroups();

        // Check if setting name is empty
        if (settingNameString.isEmpty()) {
            showFailurePopup(tr("Setting name cannot be empty. Please enter a valid setting name."));
            return;
        }

        // Check for invalid characters in INI group names
        // INI format doesn't allow certain characters in section names
        const QString invalidChars = "[]=/\\";
        QString foundInvalidChars;
        for (int i = 0; i < invalidChars.length(); i++) {
            if (settingNameString.contains(invalidChars.at(i))) {
                if (!foundInvalidChars.isEmpty()) {
                    foundInvalidChars += " ";
                }
                foundInvalidChars += invalidChars.at(i);
            }
        }
        // Also check for newline characters (though they're unlikely in QLineEdit)
        if (settingNameString.contains('\n') || settingNameString.contains('\r')) {
            if (!foundInvalidChars.isEmpty()) {
                foundInvalidChars += " ";
            }
            foundInvalidChars += tr("newline characters");
        }
        if (!foundInvalidChars.isEmpty()) {
            showFailurePopup(tr("Setting name cannot contain the following characters: %1").arg(foundInvalidChars));
            return;
        }

        // Get current selected setting name from combobox (if any)
        QString currentSelectedSetting;
        if (0 < curSettingSelectIndex && curSettingSelectIndex < m_SettingSelectListWithoutDescription.size()) {
            currentSelectedSetting = m_SettingSelectListWithoutDescription.at(curSettingSelectIndex);
        }

        // Check if the setting name matches current selection
        if (settingNameString == GROUPNAME_GLOBALSETTING && ui->settingselectComboBox->currentIndex() != GLOBALSETTING_INDEX) {
            showFailurePopup(tr("Please select \"%1\", if you want to modify the global keymapping setting.").arg(GROUPNAME_GLOBALSETTING));
            return;
        }
        else if (currentSelectedSetting == settingNameString) {
            // Case 5: Overwrite existing selected setting
            saveSettingSelectStr = settingNameString;
            settingFile.setValue(SETTINGSELECT, saveSettingSelectStr);
            saveSettingSelectStr = saveSettingSelectStr + "/";
        }
        else if (groups.contains(settingNameString)) {
            // Case 7: Setting name exists but not currently selected - ask for confirmation
            QString message = tr("Setting \"%1\" already exists. Do you want to overwrite it?").arg(settingNameString);
            QMessageBox::StandardButton reply = QMessageBox::question(this, PROGRAM_NAME, message,
                                                                       QMessageBox::Yes | QMessageBox::No,
                                                                       QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                saveSettingSelectStr = settingNameString;
                settingFile.setValue(SETTINGSELECT, saveSettingSelectStr);
                saveSettingSelectStr = saveSettingSelectStr + "/";
            } else {
                // User cancelled, don't save
                return;
            }
        }
        else {
            // Case 6: New setting name - create new setting
            saveSettingSelectStr = settingNameString;
            settingFile.setValue(SETTINGSELECT, saveSettingSelectStr);
            saveSettingSelectStr = saveSettingSelectStr + "/";
        }
    }

#if 0
    if (cursettingSelectStr == GROUPNAME_GLOBALSETTING && ui->settingselectComboBox->currentIndex() == GLOBALSETTING_INDEX) {
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

        for (const QString &group : std::as_const(groups)){
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
                showFailurePopup(message);
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

        if ((false == ui->processLineEdit->text().isEmpty())
                 && (false == ui->windowTitleLineEdit->text().isEmpty())
                 && (true == ui->processCheckBox->isChecked())
                 && (true == ui->titleCheckBox->isChecked())
                 && (ui->processLineEdit->text() == m_MapProcessInfo.FileName)
                 && (!m_MapProcessInfo.FilePath.isEmpty())){
            QStringList groups = settingFile.childGroups();
            int index = -1;
            for (index = 1; index <= TITLESETTING_INDEX_MAX; index++) {
                QString subgroup = m_MapProcessInfo.FileName + SEPARATOR_TITLESETTING + QString(WINDOWTITLE_STRING) + QString::number(index);

                if (groups.contains(subgroup)) {
                    QVariant windowTitle_Var;
                    if (readSaveSettingData(subgroup, PROCESSINFO_WINDOWTITLE, windowTitle_Var)) {
                        QString titleStr = windowTitle_Var.toString();
                        if (titleStr == ui->windowTitleLineEdit->text()) {
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
                showFailurePopup(message);
                return;
            }
        }
        else if ((false == ui->processLineEdit->text().isEmpty())
            && (true == ui->processCheckBox->isChecked())
            && (false == ui->titleCheckBox->isChecked())
            && (ui->processLineEdit->text() == m_MapProcessInfo.FileName)
            && (!m_MapProcessInfo.FilePath.isEmpty())){
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
#endif

    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_LASTTABINDEX, s_KeyMappingTabWidgetCurrentIndex);

    QStringList tabnamelist;
    QStringList tabhotkeylist;
    QStringList tabfontcolorlist;
    QStringList tabbgcolorlist;
    QStringList tabcustomimage_pathList;
    QStringList tabcustomimage_showpositionList;
    QStringList tabcustomimage_paddingList;
    QStringList tabcustomimage_showastrayiconList;
    QStringList tabcustomimage_showasfloatingwindowList;
    QVariantList tabcustomimage_trayiconpixelList;
    QVariantList floatingwindow_positionList;
    QVariantList floatingwindow_sizeList;
    QStringList floatingwindow_bgcolorList;
    QStringList floatingwindow_radiusList;
    QStringList floatingwindow_opacityList;
    QStringList floatingwindow_mousepassthroughList;
    QString original_keys_forsave;
    QString mapping_keysList_forsave;
    QString mappingkeys_keyupList_forsave;
    QVariantList notesList_forsave;
    QVariantList categorysList_forsave;
    QString burstList_forsave;
    QString burstpresstimeList_forsave;
    QString burstreleasetimeList_forsave;
    QString lockList_forsave;
    QString mappingkeyunlockList_forsave;
    QString postmappingkeyList_forsave;
    QString fixedvkeycodeList_forsave;
    QString checkcombkeyorderList_forsave;
    QString unbreakableList_forsave;
    QString passthroughList_forsave;
    QString sendtimingList_forsave;
    QString keyseqholddownList_forsave;
    QString repeatmodeList_forsave;
    QString repeattimesList_forsave;
    QString crosshair_centercolorList_forsave;
    QString crosshair_centersizeList_forsave;
    QString crosshair_centeropacityList_forsave;
    QString crosshair_crosshaircolorList_forsave;
    QString crosshair_crosshairwidthList_forsave;
    QString crosshair_crosshairlengthList_forsave;
    QString crosshair_crosshairopacityList_forsave;
    QString crosshair_showcenterList_forsave;
    QString crosshair_showtopList_forsave;
    QString crosshair_showbottomList_forsave;
    QString crosshair_showleftList_forsave;
    QString crosshair_showrightList_forsave;
    QString crosshair_x_offsetList_forsave;
    QString crosshair_y_offsetList_forsave;

    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index) {
        QString tabName = s_KeyMappingTabInfoList.at(index).TabName;
        QString tabHotkey = s_KeyMappingTabInfoList.at(index).TabHotkey;
        QString tabFontColor;
        QString tabBGColor;
        QString tabCustomImage_Path = s_KeyMappingTabInfoList.at(index).TabCustomImage_Path;
        int tabCustomImage_ShowPosition = s_KeyMappingTabInfoList.at(index).TabCustomImage_ShowPosition;
        int tabCustomImage_Padding = s_KeyMappingTabInfoList.at(index).TabCustomImage_Padding;
        bool tabCustomImage_ShowAsTrayIcon = s_KeyMappingTabInfoList.at(index).TabCustomImage_ShowAsTrayIcon;
        bool tabCustomImage_ShowAsFloatingWindow = s_KeyMappingTabInfoList.at(index).TabCustomImage_ShowAsFloatingWindow;
        QSize tabCustomImage_TrayIconPixel = s_KeyMappingTabInfoList.at(index).TabCustomImage_TrayIconPixel;
        QPoint floatingWindow_Position = s_KeyMappingTabInfoList.at(index).FloatingWindow_Position;
        QSize floatingWindow_Size = s_KeyMappingTabInfoList.at(index).FloatingWindow_Size;
        QString floatingWindow_BGColor;
        int floatingWindow_Radius = s_KeyMappingTabInfoList.at(index).FloatingWindow_Radius;
        double floatingWindow_Opacity = s_KeyMappingTabInfoList.at(index).FloatingWindow_Opacity;
        bool floatingWindow_MousePassThrough = s_KeyMappingTabInfoList.at(index).FloatingWindow_MousePassThrough;
        if (isTabTextDuplicateInStringList(tabName, tabnamelist)) {
            tabName.clear();
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[saveKeyMapSetting] TabName:" << tabName << " is already exists, set a empty tabname!";
#endif
        }
        if (s_KeyMappingTabInfoList.at(index).TabFontColor.isValid()) {
            tabFontColor = s_KeyMappingTabInfoList.at(index).TabFontColor.name();
        }
        if (s_KeyMappingTabInfoList.at(index).TabBackgroundColor.isValid()) {
            tabBGColor = s_KeyMappingTabInfoList.at(index).TabBackgroundColor.name(QColor::HexArgb);
        }
        if (s_KeyMappingTabInfoList.at(index).FloatingWindow_BackgroundColor.isValid()) {
            floatingWindow_BGColor = s_KeyMappingTabInfoList.at(index).FloatingWindow_BackgroundColor.name(QColor::HexArgb);
        }
        tabnamelist.append(tabName);
        tabhotkeylist.append(tabHotkey);
        tabfontcolorlist.append(tabFontColor);
        tabbgcolorlist.append(tabBGColor);
        tabcustomimage_pathList.append(tabCustomImage_Path);
        tabcustomimage_showpositionList.append(QString::number(tabCustomImage_ShowPosition));
        tabcustomimage_paddingList.append(QString::number(tabCustomImage_Padding));
        tabcustomimage_showastrayiconList.append(tabCustomImage_ShowAsTrayIcon ? "ON" : "OFF");
        tabcustomimage_showasfloatingwindowList.append(tabCustomImage_ShowAsFloatingWindow ? "ON" : "OFF");
        tabcustomimage_trayiconpixelList.append(tabCustomImage_TrayIconPixel);
        floatingwindow_positionList.append(floatingWindow_Position);
        floatingwindow_sizeList.append(floatingWindow_Size);
        floatingwindow_bgcolorList.append(floatingWindow_BGColor);
        floatingwindow_radiusList.append(QString::number(floatingWindow_Radius));
        floatingwindow_opacityList.append(QString::number(floatingWindow_Opacity, 'f', FLOATINGWINDOW_OPACITY_DECIMALS));
        floatingwindow_mousepassthroughList.append(floatingWindow_MousePassThrough ? "ON" : "OFF");

        QList<MAP_KEYDATA> *mappingDataList = s_KeyMappingTabInfoList.at(index).KeyMappingData;

        // append SEPARATOR_KEYMAPDATA_LEVEL2 to QString variable forsave if it is not the first index
        if (index > 0) {
            original_keys_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            mapping_keysList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            mappingkeys_keyupList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            burstList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            burstpresstimeList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            burstreleasetimeList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            lockList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            mappingkeyunlockList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            postmappingkeyList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            fixedvkeycodeList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            checkcombkeyorderList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            unbreakableList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            passthroughList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            sendtimingList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            keyseqholddownList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            repeatmodeList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            repeattimesList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_centercolorList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_centersizeList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_centeropacityList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_crosshaircolorList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_crosshairwidthList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_crosshairlengthList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_crosshairopacityList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_showcenterList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_showtopList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_showbottomList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_showleftList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_showrightList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_x_offsetList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            crosshair_y_offsetList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
        }

        QStringList original_keys;
        QStringList mapping_keysList;
        QStringList mappingkeys_keyupList;
        QStringList notesList;
        QStringList categorysList;
        QStringList burstList;
        QStringList burstpresstimeList;
        QStringList burstreleasetimeList;
        QStringList lockList;
        QStringList mappingkeyunlockList;
        QStringList postmappingkeyList;
        QStringList fixedvkeycodeList;
        QStringList checkcombkeyorderList;
        QStringList unbreakableList;
        QStringList passthroughList;
        QStringList sendtimingList;
        QStringList keyseqholddownList;
        QStringList repeatmodeList;
        QStringList repeattimesList;
        QStringList crosshair_centercolorList;
        QStringList crosshair_centersizeList;
        QStringList crosshair_centeropacityList;
        QStringList crosshair_crosshaircolorList;
        QStringList crosshair_crosshairwidthList;
        QStringList crosshair_crosshairlengthList;
        QStringList crosshair_crosshairopacityList;
        QStringList crosshair_showcenterList;
        QStringList crosshair_showtopList;
        QStringList crosshair_showbottomList;
        QStringList crosshair_showleftList;
        QStringList crosshair_showrightList;
        QStringList crosshair_x_offsetList;
        QStringList crosshair_y_offsetList;
        if (mappingDataList->size() > 0) {
            for (const MAP_KEYDATA &keymapdata : std::as_const(*mappingDataList))
            {
                original_keys << keymapdata.Original_Key;

                // Escape SendText content in mapping keys for safe saving
                QStringList escapedMappingKeys;
                for (const QString &key : keymapdata.Mapping_Keys) {
                    escapedMappingKeys << escapeSendTextForSaving(key);
                }
                QString mappingkeys_str = escapedMappingKeys.join(SEPARATOR_NEXTARROW);
                mapping_keysList  << mappingkeys_str;

                // Escape SendText content in keyup mapping keys for safe saving
                QStringList escapedMappingKeysKeyUp;
                for (const QString &key : keymapdata.MappingKeys_KeyUp) {
                    escapedMappingKeysKeyUp << escapeSendTextForSaving(key);
                }
                QString mappingkeys_keyup_str = escapedMappingKeysKeyUp.join(SEPARATOR_NEXTARROW);
                if (mappingkeys_keyup_str.isEmpty()) {
                    mappingkeys_keyupList << mappingkeys_str;
                }
                else {
                    mappingkeys_keyupList << mappingkeys_keyup_str;
                }
                notesList << keymapdata.Note;
                categorysList << keymapdata.Category;
                if (true == keymapdata.Burst) {
                    burstList.append("ON");
                }
                else {
                    burstList.append("OFF");
                }
                if (BURST_TIME_MIN <= keymapdata.BurstPressTime && keymapdata.BurstPressTime <= BURST_TIME_MAX) {
                    burstpresstimeList.append(QString::number(keymapdata.BurstPressTime));
                }
                else {
                    burstpresstimeList.append(QString::number(BURST_PRESS_TIME_DEFAULT));
                }
                if (BURST_TIME_MIN <= keymapdata.BurstReleaseTime && keymapdata.BurstReleaseTime <= BURST_TIME_MAX) {
                    burstreleasetimeList.append(QString::number(keymapdata.BurstReleaseTime));
                }
                else {
                    burstreleasetimeList.append(QString::number(BURST_RELEASE_TIME_DEFAULT));
                }
                if (true == keymapdata.Lock) {
                    lockList.append("ON");
                }
                else {
                    lockList.append("OFF");
                }
                if (true == keymapdata.MappingKeyUnlock) {
                    mappingkeyunlockList.append("ON");
                }
                else {
                    mappingkeyunlockList.append("OFF");
                }
                if (true == keymapdata.PostMappingKey) {
                    postmappingkeyList.append("ON");
                }
                else {
                    postmappingkeyList.append("OFF");
                }
                if (FIXED_VIRTUAL_KEY_CODE_MIN <= keymapdata.FixedVKeyCode && keymapdata.FixedVKeyCode <= FIXED_VIRTUAL_KEY_CODE_MAX) {
                    fixedvkeycodeList.append(QString::number(keymapdata.FixedVKeyCode, 16));
                }
                else {
                    fixedvkeycodeList.append(QString::number(FIXED_VIRTUAL_KEY_CODE_NONE, 16));
                }
                if (true == keymapdata.CheckCombKeyOrder) {
                    checkcombkeyorderList.append("ON");
                }
                else {
                    checkcombkeyorderList.append("OFF");
                }
                if (true == keymapdata.Unbreakable) {
                    unbreakableList.append("ON");
                }
                else {
                    unbreakableList.append("OFF");
                }
                if (true == keymapdata.PassThrough) {
                    passthroughList.append("ON");
                }
                else {
                    passthroughList.append("OFF");
                }

                if (SENDTIMING_KEYDOWN == keymapdata.SendTiming) {
                    sendtimingList.append(SENDTIMING_STR_KEYDOWN);
                }
                else if (SENDTIMING_KEYUP == keymapdata.SendTiming) {
                    sendtimingList.append(SENDTIMING_STR_KEYUP);
                }
                else if (SENDTIMING_KEYDOWN_AND_KEYUP == keymapdata.SendTiming) {
                    sendtimingList.append(SENDTIMING_STR_KEYDOWN_AND_KEYUP);
                }
                else if (SENDTIMING_NORMAL_AND_KEYUP == keymapdata.SendTiming) {
                    sendtimingList.append(SENDTIMING_STR_NORMAL_AND_KEYUP);
                }
                else {
                    sendtimingList.append(SENDTIMING_STR_NORMAL);
                }

                if (true == keymapdata.KeySeqHoldDown) {
                    keyseqholddownList.append("ON");
                }
                else {
                    keyseqholddownList.append("OFF");
                }
                if (REPEAT_MODE_NONE <= keymapdata.RepeatMode && keymapdata.RepeatMode <= REPEAT_MODE_BYTIMES) {
                    repeatmodeList.append(QString::number(keymapdata.RepeatMode));
                }
                else {
                    repeatmodeList.append(QString::number(REPEAT_MODE_NONE));
                }
                if (REPEAT_TIMES_MIN <= keymapdata.RepeatTimes && keymapdata.RepeatTimes <= REPEAT_TIMES_MAX) {
                    repeattimesList.append(QString::number(keymapdata.RepeatTimes));
                }
                else {
                    repeattimesList.append(QString::number(REPEAT_TIMES_DEFAULT));
                }
                if (keymapdata.Crosshair_CenterColor.isValid()) {
                    crosshair_centercolorList.append(keymapdata.Crosshair_CenterColor.name().remove("#"));
                }
                else {
                    crosshair_centercolorList.append(CROSSHAIR_CENTERCOLOR_DEFAULT);
                }
                if (CROSSHAIR_CENTERSIZE_MIN <= keymapdata.Crosshair_CenterSize && keymapdata.Crosshair_CenterSize <= CROSSHAIR_CENTERSIZE_MAX) {
                    crosshair_centersizeList.append(QString::number(keymapdata.Crosshair_CenterSize));
                }
                else {
                    crosshair_centersizeList.append(QString::number(CROSSHAIR_CENTERSIZE_DEFAULT));
                }
                if (CROSSHAIR_OPACITY_MIN <= keymapdata.Crosshair_CenterOpacity && keymapdata.Crosshair_CenterOpacity <= CROSSHAIR_OPACITY_MAX) {
                    crosshair_centeropacityList.append(QString::number(keymapdata.Crosshair_CenterOpacity));
                }
                else {
                    crosshair_centeropacityList.append(QString::number(CROSSHAIR_CENTEROPACITY_DEFAULT));
                }
                if (keymapdata.Crosshair_CrosshairColor.isValid()) {
                    crosshair_crosshaircolorList.append(keymapdata.Crosshair_CrosshairColor.name().remove("#"));
                }
                else {
                    crosshair_crosshaircolorList.append(CROSSHAIR_CROSSHAIRCOLOR_DEFAULT);
                }
                if (CROSSHAIR_CROSSHAIRWIDTH_MIN <= keymapdata.Crosshair_CrosshairWidth && keymapdata.Crosshair_CrosshairWidth <= CROSSHAIR_CROSSHAIRWIDTH_MAX) {
                    crosshair_crosshairwidthList.append(QString::number(keymapdata.Crosshair_CrosshairWidth));
                }
                else {
                    crosshair_crosshairwidthList.append(QString::number(CROSSHAIR_CROSSHAIRWIDTH_DEFAULT));
                }
                if (CROSSHAIR_CROSSHAIRLENGTH_MIN <= keymapdata.Crosshair_CrosshairLength && keymapdata.Crosshair_CrosshairLength <= CROSSHAIR_CROSSHAIRLENGTH_MAX) {
                    crosshair_crosshairlengthList.append(QString::number(keymapdata.Crosshair_CrosshairLength));
                }
                else {
                    crosshair_crosshairlengthList.append(QString::number(CROSSHAIR_CROSSHAIRLENGTH_DEFAULT));
                }
                if (CROSSHAIR_OPACITY_MIN <= keymapdata.Crosshair_CrosshairOpacity && keymapdata.Crosshair_CrosshairOpacity <= CROSSHAIR_OPACITY_MAX) {
                    crosshair_crosshairopacityList.append(QString::number(keymapdata.Crosshair_CrosshairOpacity));
                }
                else {
                    crosshair_crosshairopacityList.append(QString::number(CROSSHAIR_CROSSHAIROPACITY_DEFAULT));
                }
                if (true == keymapdata.Crosshair_ShowCenter) {
                    crosshair_showcenterList.append("ON");
                }
                else {
                    crosshair_showcenterList.append("OFF");
                }
                if (true == keymapdata.Crosshair_ShowTop) {
                    crosshair_showtopList.append("ON");
                }
                else {
                    crosshair_showtopList.append("OFF");
                }
                if (true == keymapdata.Crosshair_ShowBottom) {
                    crosshair_showbottomList.append("ON");
                }
                else {
                    crosshair_showbottomList.append("OFF");
                }
                if (true == keymapdata.Crosshair_ShowLeft) {
                    crosshair_showleftList.append("ON");
                }
                else {
                    crosshair_showleftList.append("OFF");
                }
                if (true == keymapdata.Crosshair_ShowRight) {
                    crosshair_showrightList.append("ON");
                }
                else {
                    crosshair_showrightList.append("OFF");
                }
                if (CROSSHAIR_X_OFFSET_MIN <= keymapdata.Crosshair_X_Offset && keymapdata.Crosshair_X_Offset <= CROSSHAIR_X_OFFSET_MAX) {
                    crosshair_x_offsetList.append(QString::number(keymapdata.Crosshair_X_Offset));
                }
                else {
                    crosshair_x_offsetList.append(QString::number(CROSSHAIR_X_OFFSET_DEFAULT));
                }
                if (CROSSHAIR_Y_OFFSET_MIN <= keymapdata.Crosshair_Y_Offset && keymapdata.Crosshair_Y_Offset <= CROSSHAIR_Y_OFFSET_MAX) {
                    crosshair_y_offsetList.append(QString::number(keymapdata.Crosshair_Y_Offset));
                }
                else {
                    crosshair_y_offsetList.append(QString::number(CROSSHAIR_Y_OFFSET_DEFAULT));
                }
            }
        }
        // join QStringList variables first (use SEPARATOR_KEYMAPDATA_LEVEL1)
        QString original_keys_str = original_keys.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString mapping_keysList_str = mapping_keysList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString mappingkeys_keyupList_str = mappingkeys_keyupList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString burstList_str = burstList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString burstpresstimeList_str = burstpresstimeList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString burstreleasetimeList_str = burstreleasetimeList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString lockList_str = lockList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString mappingkeyunlockList_str = mappingkeyunlockList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString postmappingkeyList_str = postmappingkeyList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString fixedvkeycodeList_str = fixedvkeycodeList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString checkcombkeyorderList_str = checkcombkeyorderList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString unbreakableList_str = unbreakableList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString passthroughList_str = passthroughList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString sendtimingList_str = sendtimingList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString keyseqholddownList_str = keyseqholddownList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString repeatmodeList_str = repeatmodeList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString repeattimesList_str = repeattimesList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_centercolorList_str = crosshair_centercolorList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_centersizeList_str = crosshair_centersizeList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_centeropacityList_str = crosshair_centeropacityList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_crosshaircolorList_str = crosshair_crosshaircolorList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_crosshairwidthList_str = crosshair_crosshairwidthList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_crosshairlengthList_str = crosshair_crosshairlengthList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_crosshairopacityList_str = crosshair_crosshairopacityList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_showcenterList_str = crosshair_showcenterList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_showtopList_str = crosshair_showtopList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_showbottomList_str = crosshair_showbottomList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_showleftList_str = crosshair_showleftList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_showrightList_str = crosshair_showrightList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_x_offsetList_str = crosshair_x_offsetList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString crosshair_y_offsetList_str = crosshair_y_offsetList.join(SEPARATOR_KEYMAPDATA_LEVEL1);

        // append joined QString variables to forsave variables
        original_keys_forsave.append(original_keys_str);
        mapping_keysList_forsave.append(mapping_keysList_str);
        mappingkeys_keyupList_forsave.append(mappingkeys_keyupList_str);
        notesList_forsave.append(notesList);
        categorysList_forsave.append(categorysList);
        burstList_forsave.append(burstList_str);
        burstpresstimeList_forsave.append(burstpresstimeList_str);
        burstreleasetimeList_forsave.append(burstreleasetimeList_str);
        lockList_forsave.append(lockList_str);
        mappingkeyunlockList_forsave.append(mappingkeyunlockList_str);
        postmappingkeyList_forsave.append(postmappingkeyList_str);
        fixedvkeycodeList_forsave.append(fixedvkeycodeList_str);
        checkcombkeyorderList_forsave.append(checkcombkeyorderList_str);
        unbreakableList_forsave.append(unbreakableList_str);
        passthroughList_forsave.append(passthroughList_str);
        sendtimingList_forsave.append(sendtimingList_str);
        keyseqholddownList_forsave.append(keyseqholddownList_str);
        repeatmodeList_forsave.append(repeatmodeList_str);
        repeattimesList_forsave.append(repeattimesList_str);
        crosshair_centercolorList_forsave.append(crosshair_centercolorList_str);
        crosshair_centersizeList_forsave.append(crosshair_centersizeList_str);
        crosshair_centeropacityList_forsave.append(crosshair_centeropacityList_str);
        crosshair_crosshaircolorList_forsave.append(crosshair_crosshaircolorList_str);
        crosshair_crosshairwidthList_forsave.append(crosshair_crosshairwidthList_str);
        crosshair_crosshairlengthList_forsave.append(crosshair_crosshairlengthList_str);
        crosshair_crosshairopacityList_forsave.append(crosshair_crosshairopacityList_str);
        crosshair_showcenterList_forsave.append(crosshair_showcenterList_str);
        crosshair_showtopList_forsave.append(crosshair_showtopList_str);
        crosshair_showbottomList_forsave.append(crosshair_showbottomList_str);
        crosshair_showleftList_forsave.append(crosshair_showleftList_str);
        crosshair_showrightList_forsave.append(crosshair_showrightList_str);
        crosshair_x_offsetList_forsave.append(crosshair_x_offsetList_str);
        crosshair_y_offsetList_forsave.append(crosshair_y_offsetList_str);
    }

    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABNAMELIST, tabnamelist);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABHOTKEYLIST, tabhotkeylist);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABFONTCOLORLIST, tabfontcolorlist);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABBGCOLORLIST, tabbgcolorlist);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_PATHLIST, tabcustomimage_pathList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_SHOWPOSITIONLIST, tabcustomimage_showpositionList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_PADDINGLIST, tabcustomimage_paddingList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_SHOWASTRAYICONLIST, tabcustomimage_showastrayiconList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_SHOWASFLOATINGWINDOWLIST, tabcustomimage_showasfloatingwindowList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_TRAYICON_PIXELLIST, tabcustomimage_trayiconpixelList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_POSITIONLIST, floatingwindow_positionList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_SIZELIST, floatingwindow_sizeList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_BGCOLORLIST, floatingwindow_bgcolorList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_RADIUSLIST, floatingwindow_radiusList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_OPACITYLIST, floatingwindow_opacityList);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_MOUSEPASSTHROUGHLIST, floatingwindow_mousepassthroughList);

    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_ORIGINALKEYS, original_keys_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_MAPPINGKEYS , mapping_keysList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_MAPPINGKEYS_KEYUP , mappingkeys_keyupList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_NOTE , notesList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CATEGORY, categorysList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURST , burstList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTPRESS_TIME , burstpresstimeList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME , burstreleasetimeList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_LOCK , lockList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_MAPPINGKEYUNLOCK , mappingkeyunlockList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_POSTMAPPINGKEY , postmappingkeyList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_FIXEDVKEYCODE , fixedvkeycodeList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CHECKCOMBKEYORDER , checkcombkeyorderList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_UNBREAKABLE , unbreakableList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_PASSTHROUGH , passthroughList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_SENDTIMING , sendtimingList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_KEYSEQHOLDDOWN , keyseqholddownList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_REPEATMODE, repeatmodeList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_REPEATIMES, repeattimesList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_CENTERCOLOR, crosshair_centercolorList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_CENTERSIZE, crosshair_centersizeList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_CENTEROPACITY, crosshair_centeropacityList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIRCOLOR, crosshair_crosshaircolorList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIRWIDTH, crosshair_crosshairwidthList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIRLENGTH, crosshair_crosshairlengthList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIROPACITY, crosshair_crosshairopacityList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWCENTER, crosshair_showcenterList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWTOP, crosshair_showtopList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWBOTTOM, crosshair_showbottomList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWLEFT, crosshair_showleftList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWRIGHT, crosshair_showrightList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_X_OFFSET, crosshair_x_offsetList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_CROSSHAIR_Y_OFFSET, crosshair_y_offsetList_forsave);

    settingFile.setValue(saveSettingSelectStr+KEY2MOUSE_X_SPEED , ui->mouseXSpeedSpinBox->value());
    settingFile.setValue(saveSettingSelectStr+KEY2MOUSE_Y_SPEED , ui->mouseYSpeedSpinBox->value());

    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_X_SPEED, ui->Gyro2MouseXSpeedSpinBox->value());
    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_Y_SPEED, ui->Gyro2MouseYSpeedSpinBox->value());
    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_MIN_GYRO_X_SENSITIVITY, ui->Gyro2MouseMinXSensSpinBox->value());
    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_MIN_GYRO_Y_SENSITIVITY, ui->Gyro2MouseMinYSensSpinBox->value());
    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_MAX_GYRO_X_SENSITIVITY, ui->Gyro2MouseMaxXSensSpinBox->value());
    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_MAX_GYRO_Y_SENSITIVITY, ui->Gyro2MouseMaxYSensSpinBox->value());
    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_MIN_GYRO_THRESHOLD, ui->Gyro2MouseMinThresholdSpinBox->value());
    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_MAX_GYRO_THRESHOLD, ui->Gyro2MouseMaxThresholdSpinBox->value());

    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_MOUSE_X_SOURCE, m_Gyro2MouseOptionDialog->getGyro2Mouse_MouseXSource());
    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_MOUSE_Y_SOURCE, m_Gyro2MouseOptionDialog->getGyro2Mouse_MouseYSource());
    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_MOUSE_X_REVERT, m_Gyro2MouseOptionDialog->getGyro2Mouse_MouseXRevert());
    settingFile.setValue(saveSettingSelectStr+GYRO2MOUSE_MOUSE_Y_REVERT, m_Gyro2MouseOptionDialog->getGyro2Mouse_MouseYRevert());

    settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_X_SENSITIVITY , vJoy_X_Sensitivity);
    settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_Y_SENSITIVITY , vJoy_Y_Sensitivity);
    settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_RECENTER_TIMEOUT, vJoy_Recenter_Timeout);

    if (saveGlobalSetting) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[saveKeyMapSetting]" << "GlobalSetting do not need processinfo!";
#endif
    }
    else {
        if (!m_MapProcessInfo.FilePath.isEmpty()){
            settingFile.setValue(saveSettingSelectStr+PROCESSINFO_FILEPATH, m_MapProcessInfo.FilePath);
        }

        settingFile.setValue(saveSettingSelectStr+PROCESSINFO_FILENAME, ui->processLineEdit->text());
        settingFile.setValue(saveSettingSelectStr+PROCESSINFO_WINDOWTITLE, ui->windowTitleLineEdit->text());
        settingFile.setValue(saveSettingSelectStr+PROCESSINFO_FILENAME_MATCH_INDEX, ui->checkProcessComboBox->currentIndex());
        settingFile.setValue(saveSettingSelectStr+PROCESSINFO_WINDOWTITLE_MATCH_INDEX, ui->checkWindowTitleComboBox->currentIndex());

        settingFile.setValue(saveSettingSelectStr+PROCESSINFO_DESCRIPTION, ui->descriptionLineEdit->text());
    }

    settingFile.setValue(saveSettingSelectStr+AUTOSTARTMAPPING_CHECKED, ui->autoStartMappingCheckBox->checkState());
    settingFile.setValue(saveSettingSelectStr+SENDTOSAMEWINDOWS_CHECKED, ui->sendToSameTitleWindowsCheckBox->isChecked());
    settingFile.setValue(saveSettingSelectStr+ACCEPTVIRTUALGAMEPADINPUT_CHECKED, ui->acceptVirtualGamepadInputCheckBox->isChecked());
    settingFile.setValue(saveSettingSelectStr+PROCESSICON_AS_TRAYICON_CHECKED, ui->ProcessIconAsTrayIconCheckBox->isChecked());
#ifdef VIGEM_CLIENT_SUPPORT
    settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_LOCKCURSOR, ui->lockCursorCheckBox->isChecked());
    settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_DIRECTMODE, ui->directModeCheckBox->isChecked());
#endif
    settingFile.setValue(saveSettingSelectStr+DATAPORT_NUMBER, ui->dataPortSpinBox->value());
    double gripThresholdBrake = ui->brakeThresholdDoubleSpinBox->value();
    gripThresholdBrake = round(gripThresholdBrake * pow(10, GRIP_THRESHOLD_DECIMALS)) / pow(10, GRIP_THRESHOLD_DECIMALS);
    settingFile.setValue(saveSettingSelectStr+GRIP_THRESHOLD_BRAKE, gripThresholdBrake);
    double gripThresholdAccel = ui->accelThresholdDoubleSpinBox->value();
    gripThresholdAccel = round(gripThresholdAccel * pow(10, GRIP_THRESHOLD_DECIMALS)) / pow(10, GRIP_THRESHOLD_DECIMALS);
    settingFile.setValue(saveSettingSelectStr+GRIP_THRESHOLD_ACCEL, gripThresholdAccel);

    settingFile.setValue(saveSettingSelectStr+FILTER_KEYS, ui->filterKeysCheckBox->isChecked());

//         if (m_mappingswitchKeySeqEdit->keySequence().isEmpty()) {
//             if (m_mappingswitchKeySeqEdit->lastKeySequence().isEmpty()) {
//                 m_mappingswitchKeySeqEdit->setKeySequence(QKeySequence(m_mappingswitchKeySeqEdit->defaultKeySequence()));
//             }
//             else {
//                 m_mappingswitchKeySeqEdit->setKeySequence(QKeySequence(m_mappingswitchKeySeqEdit->lastKeySequence()));
//             }
//         }
//         m_mappingswitchKeySeqEdit->clearFocus();
//         if (false == m_mappingswitchKeySeqEdit->keySequence().isEmpty()) {
//             updateMappingSwitchKeySeq(m_mappingswitchKeySeqEdit->keySequence());
//             settingFile.setValue(saveSettingSelectStr+MAPPINGSTART_KEY, m_mappingswitchKeySeqEdit->keySequence().toString());
// #ifdef DEBUG_LOGOUT_ON
//             qDebug().nospace().noquote() << "[saveKeyMapSetting]" << " Save & Set Mapping Switch KeySequence [" << saveSettingSelectStr+MAPPINGSTART_KEY << "] -> \"" << m_mappingswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText) << "\"";
// #endif
//         }

    if (s_MappingStartKeyString.isEmpty()) {
        s_MappingStartKeyString = MAPPINGSWITCH_KEY_DEFAULT;
    }
    ui->mappingStartKeyLineEdit->clearFocus();
    if (false == s_MappingStartKeyString.isEmpty()) {
        settingFile.setValue(saveSettingSelectStr+MAPPINGSTART_KEY, s_MappingStartKeyString);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[saveKeyMapSetting]" << " Save & Set Mapping Start Key [" << saveSettingSelectStr+MAPPINGSTART_KEY << "] -> \"" << s_MappingStartKeyString << "\"";
#endif
    }

    if (s_MappingStopKeyString.isEmpty()) {
        s_MappingStopKeyString = MAPPINGSWITCH_KEY_DEFAULT;
    }
    ui->mappingStopKeyLineEdit->clearFocus();
    if (false == s_MappingStopKeyString.isEmpty()) {
        settingFile.setValue(saveSettingSelectStr+MAPPINGSTOP_KEY, s_MappingStopKeyString);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[saveKeyMapSetting]" << " Save & Set Mapping Stop Key [" << saveSettingSelectStr+MAPPINGSTOP_KEY << "] -> \"" << s_MappingStopKeyString << "\"";
#endif
    }

    const QString savedSettingName = saveSettingSelectStr.remove("/");

    loadSetting_flag = true;
    QString loadresult = loadKeyMapSetting(savedSettingName);
    Q_UNUSED(loadresult);
    loadSetting_flag = false;

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;
    if (loadresult == savedSettingName) {
        ui->settingNameLineEdit->setText(loadresult);
        popupMessage = tr("Save success : ") + savedSettingName;
        popupMessageColor = SUCCESS_COLOR;
        bool backupRet = backupFile(CONFIG_FILENAME, CONFIG_LATEST_FILENAME);
        if (backupRet) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[saveKeyMapSetting]" << "Save setting success ->" << savedSettingName;
#endif
        }
    }
    else {
        ui->settingNameLineEdit->setText(QString());
        popupMessage = tr("Save failure : ") + savedSettingName;
        popupMessageColor = FAILURE_COLOR;
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[saveKeyMapSetting]" << "Mapping data error, Save setting failure!!! ->" << savedSettingName;
#endif
    }
    showPopupMessage(popupMessage, popupMessageColor, popupMessageDisplayTime);
}

QString QKeyMapper::loadKeyMapSetting(const QString &settingtext)
{
    bool loadDefault = false;
    bool loadGlobalSetting = false;
    bool initKeyMappingTable = false;
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

        if (true == settingFile.contains(SHOW_PROCESSLIST)){
            bool showProcessList = settingFile.value(SHOW_PROCESSLIST).toBool();
            if (!showProcessList) {
                ui->processListButton->setChecked(false);
            }
            else {
                ui->processListButton->setChecked(true);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Show ProcessList Button ->" << showProcessList;
#endif
        }
        else {
            ui->processListButton->setChecked(true);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Do not contains ShowProcessList, Show ProcessList Button set to Checked.";
#endif
        }

        if (true == settingFile.contains(SHOW_NOTES)){
            bool showNotes = settingFile.value(SHOW_NOTES).toBool();
            if (showNotes) {
                ui->showNotesButton->setChecked(true);
            }
            else {
                ui->showNotesButton->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Show Notes Button ->" << showNotes;
#endif
        }
        else {
            ui->showNotesButton->setChecked(false);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Do not contains ShowNotes, Show Notes Button set to Unchecked.";
#endif
        }

        if (true == settingFile.contains(SHOW_CATEGORYS)){
            bool showCategorys = settingFile.value(SHOW_CATEGORYS).toBool();
            if (showCategorys) {
                ui->showCategoryButton->setChecked(true);
            }
            else {
                ui->showCategoryButton->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Show Categorys Button ->" << showCategorys;
#endif
        }
        else {
            ui->showCategoryButton->setChecked(false);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Do not contains ShowCategorys, Show Categorys Button set to Unchecked.";
#endif
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

        QStringList virtualGamepadList;
        if (true == settingFile.contains(VIRTUAL_GAMEPADLIST)){
            virtualGamepadList = settingFile.value(VIRTUAL_GAMEPADLIST).toStringList();
            if (!virtualGamepadList.isEmpty()) {
                if (virtualGamepadList.size() == 1
                    && virtualGamepadList.constFirst() != getVirtualGamepadType()) {
                    virtualGamepadList[0] = getVirtualGamepadType();
                }
            }
            else {
                virtualGamepadList = QStringList() << getVirtualGamepadType();
            }
        }
        else {
            virtualGamepadList = QStringList() << getVirtualGamepadType();
        }
        QKeyMapper_Worker::loadVirtualGamepadList(virtualGamepadList);

//         QString loadedwindowswitchKeySeqStr;
//         if (true == settingFile.contains(WINDOWSWITCH_KEYSEQ)){
//             loadedwindowswitchKeySeqStr = settingFile.value(WINDOWSWITCH_KEYSEQ).toString();
//             if (loadedwindowswitchKeySeqStr.isEmpty()) {
//                 loadedwindowswitchKeySeqStr = m_windowswitchKeySeqEdit->defaultKeySequence();
//             }
//         }
//         else {
//             loadedwindowswitchKeySeqStr = m_windowswitchKeySeqEdit->defaultKeySequence();
//         }
//         m_windowswitchKeySeqEdit->setKeySequence(QKeySequence(loadedwindowswitchKeySeqStr));
//         updateWindowSwitchKeySeq(m_windowswitchKeySeqEdit->keySequence());
// #ifdef DEBUG_LOGOUT_ON
//         qDebug() << "[loadKeyMapSetting]" << "Load & Set Window Switch KeySequence ->" << m_windowswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText);
// #endif

        QString loadedwindowswitchKeySeqStr;
        if (true == settingFile.contains(WINDOWSWITCH_KEYSEQ)){
            loadedwindowswitchKeySeqStr = settingFile.value(WINDOWSWITCH_KEYSEQ).toString();
            if (loadedwindowswitchKeySeqStr.isEmpty()) {
                loadedwindowswitchKeySeqStr = DISPLAYSWITCH_KEY_DEFAULT;
            }
        }
        else {
            loadedwindowswitchKeySeqStr = DISPLAYSWITCH_KEY_DEFAULT;
        }
        updateWindowSwitchKeyString(loadedwindowswitchKeySeqStr);
        ui->windowswitchkeyLineEdit->setText(s_WindowSwitchKeyString);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Load & Set Window Switch Key ->" << s_WindowSwitchKeyString;
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

        if (true == settingFile.contains(NOTIFICATION_POSITION)){
            int notification_position = settingFile.value(NOTIFICATION_POSITION).toInt();
            if (notification_position >= 0 && notification_position <= NOTIFICATION_POSITION_BOTTOM_RIGHT) {
                ui->notificationComboBox->setCurrentIndex(notification_position);
            }
            else {
                ui->notificationComboBox->setCurrentIndex(NOTIFICATION_POSITION_TOP_RIGHT);
            }
        }
        else {
            ui->notificationComboBox->setCurrentIndex(NOTIFICATION_POSITION_TOP_RIGHT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Position ->" << ui->notificationComboBox->currentIndex();
#endif

#if 0
        if (true == settingFile.contains(NOTIFICATION_FONTCOLOR)){
            QColor notification_fontcolor = settingFile.value(NOTIFICATION_FONTCOLOR).value<QColor>();
            if (notification_fontcolor.isValid()) {
                m_NotificationSetupDialog->setNotification_FontColor(notification_fontcolor);
            }
            else {
                m_NotificationSetupDialog->setNotification_FontColor(NOTIFICATION_COLOR_NORMAL_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_FontColor(NOTIFICATION_COLOR_NORMAL_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Font Color ->" << m_NotificationSetupDialog->getNotification_FontColor().name();
#endif

        if (true == settingFile.contains(NOTIFICATION_BACKGROUNDCOLOR)){
            QColor notification_bgcolor = settingFile.value(NOTIFICATION_BACKGROUNDCOLOR).value<QColor>();
            if (notification_bgcolor.isValid()) {
                m_NotificationSetupDialog->setNotification_BackgroundColor(notification_bgcolor);
            }
            else {
                m_NotificationSetupDialog->setNotification_BackgroundColor(NOTIFICATION_BACKGROUND_COLOR_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_BackgroundColor(NOTIFICATION_BACKGROUND_COLOR_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote()
            << "[loadKeyMapSetting] Notification Background Color -> " << m_NotificationSetupDialog->getNotification_BackgroundColor().name(QColor::HexArgb)
            << ", Alpha: " << m_NotificationSetupDialog->getNotification_BackgroundColor().alpha();
#endif
#endif

        if (true == settingFile.contains(NOTIFICATION_FONTCOLOR)){
            QString notification_fontcolor_str = settingFile.value(NOTIFICATION_FONTCOLOR).toString();
            QColor notification_fontcolor(notification_fontcolor_str);
            if (notification_fontcolor.isValid()) {
                m_NotificationSetupDialog->setNotification_FontColor(notification_fontcolor);
            }
            else {
                m_NotificationSetupDialog->setNotification_FontColor(NOTIFICATION_COLOR_NORMAL_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_FontColor(NOTIFICATION_COLOR_NORMAL_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Font Color ->" << m_NotificationSetupDialog->getNotification_FontColor().name();
#endif

        if (true == settingFile.contains(NOTIFICATION_BACKGROUNDCOLOR)){
            QString notification_bgcolor_str = settingFile.value(NOTIFICATION_BACKGROUNDCOLOR).toString();
            QColor notification_bgcolor(notification_bgcolor_str);
            if (notification_bgcolor.isValid()) {
                m_NotificationSetupDialog->setNotification_BackgroundColor(notification_bgcolor);
            }
            else {
                m_NotificationSetupDialog->setNotification_BackgroundColor(NOTIFICATION_BACKGROUND_COLOR_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_BackgroundColor(NOTIFICATION_BACKGROUND_COLOR_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote()
            << "[loadKeyMapSetting] Notification Background Color -> " << m_NotificationSetupDialog->getNotification_BackgroundColor().name(QColor::HexArgb)
            << ", Alpha: " << m_NotificationSetupDialog->getNotification_BackgroundColor().alpha();
#endif

        if (true == settingFile.contains(NOTIFICATION_FONTSIZE)){
            int notification_fontsize = settingFile.value(NOTIFICATION_FONTSIZE).toInt();
            if (NOTIFICATION_FONT_SIZE_MIN <= notification_fontsize && notification_fontsize <= NOTIFICATION_FONT_SIZE_MAX) {
                m_NotificationSetupDialog->setNotification_FontSize(notification_fontsize);
            }
            else {
                m_NotificationSetupDialog->setNotification_FontSize(NOTIFICATION_FONT_SIZE_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_FontSize(NOTIFICATION_FONT_SIZE_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Font Size ->" << m_NotificationSetupDialog->getNotification_FontSize();
#endif

        if (true == settingFile.contains(NOTIFICATION_FONTWEIGHT)){
            int notification_fontweight = settingFile.value(NOTIFICATION_FONTWEIGHT).toInt();
            if (NOTIFICATION_FONT_WEIGHT_MIN <= notification_fontweight && notification_fontweight <= NOTIFICATION_FONT_WEIGHT_MAX) {
                m_NotificationSetupDialog->setNotification_FontWeight(notification_fontweight);
            }
            else {
                m_NotificationSetupDialog->setNotification_FontWeight(NOTIFICATION_FONT_WEIGHT_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_FontWeight(NOTIFICATION_FONT_WEIGHT_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Font Weight ->" << m_NotificationSetupDialog->getNotification_FontWeight();
#endif

        if (true == settingFile.contains(NOTIFICATION_FONTITALIC)){
            bool notification_fontitalic = settingFile.value(NOTIFICATION_FONTITALIC).toBool();
            m_NotificationSetupDialog->setNotification_FontIsItalic(notification_fontitalic);
        }
        else {
            m_NotificationSetupDialog->setNotification_FontIsItalic(NOTIFICATION_FONT_ITALIC_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Font Italic Checkbox ->" << m_NotificationSetupDialog->getNotification_FontIsItalic();
#endif

        if (true == settingFile.contains(NOTIFICATION_DISPLAYDURATION)){
            int notification_displayduration = settingFile.value(NOTIFICATION_DISPLAYDURATION).toInt();
            if (NOTIFICATION_DURATION_MIN <= notification_displayduration && notification_displayduration <= NOTIFICATION_DURATION_MAX) {
                m_NotificationSetupDialog->setNotification_DisplayDuration(notification_displayduration);
            }
            else {
                m_NotificationSetupDialog->setNotification_DisplayDuration(NOTIFICATION_DISPLAY_DURATION_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_DisplayDuration(NOTIFICATION_DISPLAY_DURATION_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Display Duration ->" << m_NotificationSetupDialog->getNotification_DisplayDuration();
#endif

        if (true == settingFile.contains(NOTIFICATION_FADEINDURATION)){
            int notification_fadeinduration = settingFile.value(NOTIFICATION_FADEINDURATION).toInt();
            if (NOTIFICATION_DURATION_MIN <= notification_fadeinduration && notification_fadeinduration <= NOTIFICATION_DURATION_MAX) {
                m_NotificationSetupDialog->setNotification_FadeInDuration(notification_fadeinduration);
            }
            else {
                m_NotificationSetupDialog->setNotification_FadeInDuration(NOTIFICATION_FADEIN_DURATION_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_FadeInDuration(NOTIFICATION_FADEIN_DURATION_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification FadeIn Duration ->" << m_NotificationSetupDialog->getNotification_FadeInDuration();
#endif

        if (true == settingFile.contains(NOTIFICATION_FADEOUTDURATION)){
            int notification_fadeoutduration = settingFile.value(NOTIFICATION_FADEOUTDURATION).toInt();
            if (NOTIFICATION_DURATION_MIN <= notification_fadeoutduration && notification_fadeoutduration <= NOTIFICATION_DURATION_MAX) {
                m_NotificationSetupDialog->setNotification_FadeOutDuration(notification_fadeoutduration);
            }
            else {
                m_NotificationSetupDialog->setNotification_FadeOutDuration(NOTIFICATION_FADEOUT_DURATION_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_FadeOutDuration(NOTIFICATION_FADEOUT_DURATION_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification FadeOut Duration ->" << m_NotificationSetupDialog->getNotification_FadeOutDuration();
#endif

        if (true == settingFile.contains(NOTIFICATION_BORDERRADIUS)){
            int notification_border_radius = settingFile.value(NOTIFICATION_BORDERRADIUS).toInt();
            if (NOTIFICATION_BORDER_RADIUS_MIN <= notification_border_radius && notification_border_radius <= NOTIFICATION_BORDER_RADIUS_MAX) {
                m_NotificationSetupDialog->setNotification_BorderRadius(notification_border_radius);
            }
            else {
                m_NotificationSetupDialog->setNotification_BorderRadius(NOTIFICATION_BORDER_RADIUS_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_BorderRadius(NOTIFICATION_BORDER_RADIUS_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Border Radius ->" << m_NotificationSetupDialog->getNotification_BorderRadius();
#endif

        if (true == settingFile.contains(NOTIFICATION_PADDING)){
            int notification_padding = settingFile.value(NOTIFICATION_PADDING).toInt();
            if (NOTIFICATION_PADDING_MIN <= notification_padding && notification_padding <= NOTIFICATION_PADDING_MAX) {
                m_NotificationSetupDialog->setNotification_Padding(notification_padding);
            }
            else {
                m_NotificationSetupDialog->setNotification_Padding(NOTIFICATION_PADDING_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_Padding(NOTIFICATION_PADDING_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Padding ->" << m_NotificationSetupDialog->getNotification_Padding();
#endif

        if (true == settingFile.contains(NOTIFICATION_OPACITY)){
            double notification_opacity = settingFile.value(NOTIFICATION_OPACITY).toDouble();
            if (NOTIFICATION_OPACITY_MIN <= notification_opacity && notification_opacity <= NOTIFICATION_OPACITY_MAX) {
                m_NotificationSetupDialog->setNotification_Opacity(notification_opacity);
            }
            else {
                m_NotificationSetupDialog->setNotification_Opacity(NOTIFICATION_OPACITY_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_Opacity(NOTIFICATION_OPACITY_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Opacity ->" << m_NotificationSetupDialog->getNotification_Opacity();
#endif

        if (true == settingFile.contains(NOTIFICATION_X_OFFSET)){
            int notification_x_offset = settingFile.value(NOTIFICATION_X_OFFSET).toInt();
            if (NOTIFICATION_OFFSET_MIN <= notification_x_offset && notification_x_offset <= NOTIFICATION_OFFSET_MAX) {
                m_NotificationSetupDialog->setNotification_X_Offset(notification_x_offset);
            }
            else {
                m_NotificationSetupDialog->setNotification_X_Offset(NOTIFICATION_X_OFFSET_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_X_Offset(NOTIFICATION_X_OFFSET_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification X Offset ->" << m_NotificationSetupDialog->getNotification_X_Offset();
#endif

        if (true == settingFile.contains(NOTIFICATION_Y_OFFSET)){
            int notification_y_offset = settingFile.value(NOTIFICATION_Y_OFFSET).toInt();
            if (NOTIFICATION_OFFSET_MIN <= notification_y_offset && notification_y_offset <= NOTIFICATION_OFFSET_MAX) {
                m_NotificationSetupDialog->setNotification_Y_Offset(notification_y_offset);
            }
            else {
                m_NotificationSetupDialog->setNotification_Y_Offset(NOTIFICATION_Y_OFFSET_DEFAULT);
            }
        }
        else {
            m_NotificationSetupDialog->setNotification_Y_Offset(NOTIFICATION_Y_OFFSET_DEFAULT);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Notification Y Offset ->" << m_NotificationSetupDialog->getNotification_Y_Offset();
#endif

        if (true == settingFile.contains(TRAYICON_IDLE)){
            QString trayicon_idle = settingFile.value(TRAYICON_IDLE).toString();
            if (!trayicon_idle.isEmpty() && trayicon_idle.endsWith(ICON_FILE_SUFFIX)) {
                m_TrayIconSelectDialog->setTrayIcon_IdleStateIcon(trayicon_idle);
            }
            else {
                m_TrayIconSelectDialog->setTrayIcon_IdleStateIcon(TRAYICON_IDLE_DEFAULT_FILE);
            }
        }
        else {
            m_TrayIconSelectDialog->setTrayIcon_IdleStateIcon(TRAYICON_IDLE_DEFAULT_FILE);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "TrayIcon Idle ->" << m_TrayIconSelectDialog->getTrayIcon_IdleStateIcon();
#endif

        if (true == settingFile.contains(TRAYICON_MONITORING)){
            QString trayicon_monitoring = settingFile.value(TRAYICON_MONITORING).toString();
            if (!trayicon_monitoring.isEmpty() && trayicon_monitoring.endsWith(ICON_FILE_SUFFIX)) {
                m_TrayIconSelectDialog->setTrayIcon_MonitoringStateIcon(trayicon_monitoring);
            }
            else {
                m_TrayIconSelectDialog->setTrayIcon_MonitoringStateIcon(TRAYICON_MONITORING_DEFAULT_FILE);
            }
        }
        else {
            m_TrayIconSelectDialog->setTrayIcon_MonitoringStateIcon(TRAYICON_MONITORING_DEFAULT_FILE);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "TrayIcon Monitoring ->" << m_TrayIconSelectDialog->getTrayIcon_MonitoringStateIcon();
#endif

        if (true == settingFile.contains(TRAYICON_GLOBAL)){
            QString trayicon_global = settingFile.value(TRAYICON_GLOBAL).toString();
            if (!trayicon_global.isEmpty() && trayicon_global.endsWith(ICON_FILE_SUFFIX)) {
                m_TrayIconSelectDialog->setTrayIcon_GlobalStateIcon(trayicon_global);
            }
            else {
                m_TrayIconSelectDialog->setTrayIcon_GlobalStateIcon(TRAYICON_GLOBAL_DEFAULT_FILE);
            }
        }
        else {
            m_TrayIconSelectDialog->setTrayIcon_GlobalStateIcon(TRAYICON_GLOBAL_DEFAULT_FILE);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "TrayIcon Global ->" << m_TrayIconSelectDialog->getTrayIcon_GlobalStateIcon();
#endif

        if (true == settingFile.contains(TRAYICON_MATCHED)){
            QString trayicon_matched = settingFile.value(TRAYICON_MATCHED).toString();
            if (!trayicon_matched.isEmpty() && trayicon_matched.endsWith(ICON_FILE_SUFFIX)) {
                m_TrayIconSelectDialog->setTrayIcon_MatchedStateIcon(trayicon_matched);
            }
            else {
                m_TrayIconSelectDialog->setTrayIcon_MatchedStateIcon(TRAYICON_MATCHED_DEFAULT_FILE);
            }
        }
        else {
            m_TrayIconSelectDialog->setTrayIcon_MatchedStateIcon(TRAYICON_MATCHED_DEFAULT_FILE);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "TrayIcon Matched ->" << m_TrayIconSelectDialog->getTrayIcon_MatchedStateIcon();
#endif

        if (true == settingFile.contains(UPDATE_SITE)){
            int update_site = settingFile.value(UPDATE_SITE).toInt();
            if (UPDATE_SITE_GITEE == update_site) {
                ui->updateSiteComboBox->setCurrentIndex(UPDATE_SITE_GITEE);
            }
            else {
                ui->updateSiteComboBox->setCurrentIndex(UPDATE_SITE_GITHUB);
            }
        }
        else {
            ui->updateSiteComboBox->setCurrentIndex(UPDATE_SITE_GITHUB);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Update Site ->" << (ui->updateSiteComboBox->currentIndex() == UPDATE_SITE_GITEE ? "Gitee" : "GitHub");
#endif

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

        if (true == settingFile.contains(STARTUP_MINIMIZED)){
            bool startupminimizedChecked = settingFile.value(STARTUP_MINIMIZED).toBool();
            if (true == startupminimizedChecked) {
                ui->startupMinimizedCheckBox->setChecked(true);
            }
            else {
                ui->startupMinimizedCheckBox->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Startup Minimized Checkbox ->" << startupminimizedChecked;
#endif
        }
        else {
            ui->startupMinimizedCheckBox->setChecked(false);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Do not contains StartupMinimized, StartupMinimized set to Unchecked.";
#endif
        }

        if (true == settingFile.contains(STARTUP_AUTOMONITORING)){
            bool startupautomonitoringChecked = settingFile.value(STARTUP_AUTOMONITORING).toBool();
            if (true == startupautomonitoringChecked) {
                ui->startupAutoMonitoringCheckBox->setChecked(true);
            }
            else {
                ui->startupAutoMonitoringCheckBox->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Startup AutoMonitoring Checkbox ->" << startupautomonitoringChecked;
#endif
        }
        else {
            ui->startupAutoMonitoringCheckBox->setChecked(false);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Do not contains StartupAutoMonitoring, StartupAutoMonitoring set to Unchecked.";
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

        if (true == settingFile.contains(MULTI_INPUT_ENABLE)){
            bool multiInputEnableChecked = settingFile.value(MULTI_INPUT_ENABLE).toBool();
            if (true == multiInputEnableChecked) {
                ui->multiInputEnableCheckBox->setChecked(true);
            }
            else {
                ui->multiInputEnableCheckBox->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "MultiInput Enable Checkbox ->" << multiInputEnableChecked;
#endif
        }
        else {
            ui->multiInputEnableCheckBox->setChecked(false);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Do not contains MultiInputEnable, MultiInputEnable set to Unchecked.";
#endif
        }

        if (true == settingFile.contains(DISABLED_KEYBOARDLIST)){
            QStringList disabledKeyboardList = settingFile.value(DISABLED_KEYBOARDLIST).toStringList();
            Interception_Worker::loadDisabledKeyboardList(disabledKeyboardList);
        }

        if (true == settingFile.contains(DISABLED_MOUSELIST)){
            QStringList disabledMouseList = settingFile.value(DISABLED_MOUSELIST).toStringList();
            Interception_Worker::loadDisabledMouseList(disabledMouseList);
        }
    }
    else if (GROUPNAME_GLOBALSETTING == settingtext) {
        loadGlobalSetting = true;
    }

    QString settingSelectStr;

    m_SettingSelectListWithoutDescription.clear();
    ui->settingselectComboBox->clear();
    ui->settingselectComboBox->addItem(QString());
    m_SettingSelectListWithoutDescription.append(QString());
    QString globalSettingNameWithDescStr;
    globalSettingNameWithDescStr = QString(SETTING_DESCRIPTION_FORMAT).arg(GROUPNAME_GLOBALSETTING, tr("Global keymapping setting"));
    ui->settingselectComboBox->addItem(globalSettingNameWithDescStr);
    m_SettingSelectListWithoutDescription.append(GROUPNAME_GLOBALSETTING);
// #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
//     QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->settingselectComboBox->model());
//     QStandardItem* item = model->item(1);
//     item->setData(QColor(Qt::darkMagenta), Qt::ForegroundRole);
// #else
//     QBrush colorBrush(Qt::darkMagenta);
//     ui->settingselectComboBox->setItemData(1, colorBrush, Qt::TextColorRole);
// #endif
    ui->settingselectComboBox->setItemIcon(GLOBALSETTING_INDEX, QIcon(":/function.png"));
    QStringList groups = settingFile.childGroups();
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[loadKeyMapSetting]" << "childGroups >>" << groups;
#endif
    QStringList validgroups;

    for (const QString &group : std::as_const(groups)){
        if (group == GROUPNAME_GLOBALSETTING) {
            continue;
        }

        QString tempSettingSelectStr = group + "/";

        QIcon settingIcon = QKeyMapper::s_Icon_Blank;
        QString filepathString;
        if (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_FILEPATH)) {
            filepathString = settingFile.value(tempSettingSelectStr+PROCESSINFO_FILEPATH).toString();
        }
        if (!filepathString.isEmpty()
            && QFileInfo::exists(filepathString)){
            QIcon fileicon;
            fileicon = extractIconFromExecutable(filepathString);

            if (fileicon.isNull()) {
                QFileIconProvider icon_provider;
                fileicon = icon_provider.icon(QFileInfo(filepathString));
            }

            if (!fileicon.isNull()) {
                settingIcon = fileicon;
            }
        }

        QString descriptionString;
        if (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_DESCRIPTION)) {
            descriptionString = settingFile.value(tempSettingSelectStr+PROCESSINFO_DESCRIPTION).toString();
        }
        QString groupnameWithDescription = group;
        if (!descriptionString.isEmpty()) {
            groupnameWithDescription = QString(SETTING_DESCRIPTION_FORMAT).arg(group, descriptionString);
        }

        ui->settingselectComboBox->addItem(settingIcon, groupnameWithDescription);
        m_SettingSelectListWithoutDescription.append(group);
        validgroups.append(group);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting] Setting select add FullMatch ->" << group;
#endif
    }

#if 0
    QStringList validgroups_fullmatch;
    QStringList validgroups_customsetting;
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[loadKeyMapSetting]" << "childGroups >>" << groups;
#endif
    for (const QString &group : std::as_const(groups)){
        bool valid_setting = false;
        QString tempSettingSelectStr = group + "/";
        if ((true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_FILENAME))
            && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_WINDOWTITLE))
            && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_FILEPATH))
            && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_FILENAME_CHECKED))
            && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_WINDOWTITLE_CHECKED))){
            valid_setting = true;
        }

        if (true == valid_setting) {
            QString filepathString = settingFile.value(tempSettingSelectStr+PROCESSINFO_FILEPATH).toString();
            QIcon settingIcon = QKeyMapper::s_Icon_Blank;
            if (!filepathString.isEmpty()
                && QFileInfo::exists(filepathString)){
                QFileIconProvider icon_provider;
                QIcon fileicon = icon_provider.icon(QFileInfo(filepathString));
                if (!fileicon.isNull()) {
                    settingIcon = fileicon;
                }
            }

            QString descriptionString;
            if (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_DESCRIPTION)) {
                descriptionString = settingFile.value(tempSettingSelectStr+PROCESSINFO_DESCRIPTION).toString();
            }
            QString groupnameWithDescription = group;
            if (!descriptionString.isEmpty()) {
                groupnameWithDescription = QString(SETTING_DESCRIPTION_FORMAT).arg(group, descriptionString);
            }

            ui->settingselectComboBox->addItem(settingIcon, groupnameWithDescription);
            m_SettingSelectListWithoutDescription.append(group);
            validgroups_fullmatch.append(group);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting] Setting select add FullMatch ->" << group;
#endif
        }
    }

    for (const QString &group : std::as_const(groups)){
        bool valid_setting = false;
        QString tempSettingSelectStr = group + "/";
        if (group.startsWith(GROUPNAME_CUSTOMGLOBALSETTING, Qt::CaseInsensitive)
                && group.endsWith(GROUPNAME_EXECUTABLE_SUFFIX, Qt::CaseInsensitive) != true) {
            valid_setting = true;
        }

        if (true == valid_setting) {
            QString descriptionString;
            if (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_DESCRIPTION)) {
                descriptionString = settingFile.value(tempSettingSelectStr+PROCESSINFO_DESCRIPTION).toString();
            }
            QString groupnameWithDescription = group;
            if (!descriptionString.isEmpty()) {
                groupnameWithDescription = QString(SETTING_DESCRIPTION_FORMAT).arg(group, descriptionString);
            }

            ui->settingselectComboBox->addItem(groupnameWithDescription);
            m_SettingSelectListWithoutDescription.append(group);
            validgroups_customsetting.append(group);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting] Setting select add CustomGlobalSetting ->" << group;
#endif
        }
    }
#endif

    if (groups.contains(GROUPNAME_GLOBALSETTING)) {
        QString settingSelectStr_bak = settingSelectStr;

        settingSelectStr = QString(GROUPNAME_GLOBALSETTING) + "/";

        bool global_datavalid = false;

        if ((true == settingFile.contains(settingSelectStr+KEYMAPDATA_ORIGINALKEYS))
            && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYS))){
            QString original_keys_loaded;
            QString mapping_keys_loaded;
            QStringList original_keys_split;
            QStringList mapping_keys_split;

            original_keys_loaded   = settingFile.value(settingSelectStr+KEYMAPDATA_ORIGINALKEYS).toString();
            mapping_keys_loaded    = settingFile.value(settingSelectStr+KEYMAPDATA_MAPPINGKEYS).toString();

            if (original_keys_loaded.isEmpty() && mapping_keys_loaded.isEmpty()) {
                global_datavalid = true;
            }
            else {
                original_keys_split = original_keys_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
                mapping_keys_split = mapping_keys_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);

                if (original_keys_split.size() == mapping_keys_split.size()
                    && original_keys_split.size() > 0) {
                    global_datavalid = true;
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

        if (global_datavalid) {
            validgroups.append(GROUPNAME_GLOBALSETTING);
        }

        settingSelectStr = settingSelectStr_bak;
    }

    // validgroups = validgroups + validgroups_fullmatch + validgroups_customsetting;

    if (true == settingtext.isEmpty()) {
        if (true == settingFile.contains(SETTINGSELECT)){
            QVariant settingSelect = settingFile.value(SETTINGSELECT);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            if (settingSelect.metaType().id() == QMetaType::QString) {
#else
            if (settingSelect.canConvert(QMetaType::QString)) {
#endif
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
                initKeyMappingTable = true;
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
            initKeyMappingTable = true;
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
            if (settingSelect.canConvert<QString>()) {
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
                initKeyMappingTable = true;
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
                    && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYS))){
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
            initKeyMappingTable = true;
        }
    }

    if (!settingSelectStr.isEmpty()) {
        if (false == settingSelectStr.contains("/")) {
            settingSelectStr = settingSelectStr + "/";
        }

        if (!settingtext.isEmpty() && !selectSettingContainsFlag) {
            initKeyMappingTable = true;
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
        initKeyMappingTable = true;
    }

    QStringList tabhotkeylist_loaded;
    QStringList tabfontcolorlist_loaded;
    QStringList tabbgcolorlist_loaded;
    QStringList tabcustomimage_pathlist_loaded;
    QStringList tabcustomimage_showpositionlist_loaded;
    QStringList tabcustomimage_paddinglist_loaded;
    QStringList tabcustomimage_showastrayiconlist_loaded;
    QStringList tabcustomimage_showasfloatingwindowlist_loaded;
    QVariantList tabcustomimage_trayiconpixellist_loaded;
    QVariantList floatingwindow_positionlist_loaded;
    QVariantList floatingwindow_sizelist_loaded;
    QStringList floatingwindow_bgcolorlist_loaded;
    QStringList floatingwindow_radiuslist_loaded;
    QStringList floatingwindow_opacitylist_loaded;
    QStringList floatingwindow_mousepassthroughlist_loaded;
    if ((true == settingFile.contains(settingSelectStr+KEYMAPDATA_ORIGINALKEYS))
        && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYS))) {
        QStringList tabnamelist_loaded;
        QString original_keys_loaded;
        QString mapping_keys_loaded;
        QString mappingkeys_keyup_loaded;
        bool notes_load_asString = false;
        QString notes_loaded_string;
        QVariantList notes_loaded;
        QVariantList categorys_loaded;
        QString burstData_loaded;
        QString burstpressData_loaded;
        QString burstreleaseData_loaded;
        QString lockData_loaded;
        QString mappingkeyunlockData_loaded;
        QString postmappingkeyData_loaded;
        QString fixedvkeycodeData_loaded;
        QString checkcombkeyorderData_loaded;
        QString unbreakableData_loaded;
        QString passthroughData_loaded;
        // QString keyup_actionData_loaded;
        QString sendtimingData_loaded;
        QString keyseqholddownData_loaded;
        QString repeatmodeData_loaded;
        QString repeattimesData_loaded;
        QString crosshair_centercolorData_loaded;
        QString crosshair_centersizeData_loaded;
        QString crosshair_centeropacityData_loaded;
        QString crosshair_crosshaircolorData_loaded;
        QString crosshair_crosshairwidthData_loaded;
        QString crosshair_crosshairlengthData_loaded;
        QString crosshair_crosshairopacityData_loaded;
        QString crosshair_showcenterData_loaded;
        QString crosshair_showtopData_loaded;
        QString crosshair_showbottomData_loaded;
        QString crosshair_showleftData_loaded;
        QString crosshair_showrightData_loaded;
        QString crosshair_x_offsetData_loaded;
        QString crosshair_y_offsetData_loaded;
        int table_count = 0;
        QStringList original_keys_split;
        QStringList mapping_keys_split;
        QStringList mappingkeys_keyup_split;
        QStringList notes_split_string;
        QList<QStringList> notes_split;
        QList<QStringList> categorys_split;
        QStringList burstData_split;
        QStringList burstpressData_split;
        QStringList burstreleaseData_split;
        QStringList lockData_split;
        QStringList mappingkeyunlockData_split;
        QStringList fixedvkeycodeData_split;
        QStringList postmappingkeyData_split;
        QStringList checkcombkeyorderData_split;
        QStringList unbreakableData_split;
        QStringList passthroughData_split;
        // QStringList keyup_actionData_split;
        QStringList sendtimingData_split;
        QStringList keyseqholddownData_split;
        QStringList repeatmodeData_split;
        QStringList repeattimesData_split;
        QStringList crosshair_centercolorData_split;
        QStringList crosshair_centersizeData_split;
        QStringList crosshair_centeropacityData_split;
        QStringList crosshair_crosshaircolorData_split;
        QStringList crosshair_crosshairwidthData_split;
        QStringList crosshair_crosshairlengthData_split;
        QStringList crosshair_crosshairopacityData_split;
        QStringList crosshair_showcenterData_split;
        QStringList crosshair_showtopData_split;
        QStringList crosshair_showbottomData_split;
        QStringList crosshair_showleftData_split;
        QStringList crosshair_showrightData_split;
        QStringList crosshair_x_offsetData_split;
        QStringList crosshair_y_offsetData_split;

        original_keys_loaded    = settingFile.value(settingSelectStr+KEYMAPDATA_ORIGINALKEYS).toString();
        mapping_keys_loaded     = settingFile.value(settingSelectStr+KEYMAPDATA_MAPPINGKEYS).toString();
        if (settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYS_KEYUP)) {
            mappingkeys_keyup_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_MAPPINGKEYS_KEYUP).toString();
        }
        else {
            mappingkeys_keyup_loaded = mapping_keys_loaded;
        }
        tabnamelist_loaded      = settingFile.value(settingSelectStr+MAPPINGTABLE_TABNAMELIST).toStringList();
        tabhotkeylist_loaded    = settingFile.value(settingSelectStr+MAPPINGTABLE_TABHOTKEYLIST).toStringList();
        tabfontcolorlist_loaded = settingFile.value(settingSelectStr+MAPPINGTABLE_TABFONTCOLORLIST).toStringList();
        tabbgcolorlist_loaded   = settingFile.value(settingSelectStr+MAPPINGTABLE_TABBGCOLORLIST).toStringList();
        tabcustomimage_pathlist_loaded              = settingFile.value(settingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_PATHLIST).toStringList();
        tabcustomimage_showpositionlist_loaded      = settingFile.value(settingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_SHOWPOSITIONLIST).toStringList();
        tabcustomimage_paddinglist_loaded           = settingFile.value(settingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_PADDINGLIST).toStringList();
        tabcustomimage_showastrayiconlist_loaded    = settingFile.value(settingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_SHOWASTRAYICONLIST).toStringList();
        tabcustomimage_showasfloatingwindowlist_loaded = settingFile.value(settingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_SHOWASFLOATINGWINDOWLIST).toStringList();
        tabcustomimage_trayiconpixellist_loaded     = settingFile.value(settingSelectStr+MAPPINGTABLE_TABCUSTOMIMAGE_TRAYICON_PIXELLIST).toList();
        floatingwindow_positionlist_loaded          = settingFile.value(settingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_POSITIONLIST).toList();
        floatingwindow_sizelist_loaded              = settingFile.value(settingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_SIZELIST).toList();
        floatingwindow_bgcolorlist_loaded           = settingFile.value(settingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_BGCOLORLIST).toStringList();
        floatingwindow_radiuslist_loaded           = settingFile.value(settingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_RADIUSLIST).toStringList();
        floatingwindow_opacitylist_loaded           = settingFile.value(settingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_OPACITYLIST).toStringList();
        floatingwindow_mousepassthroughlist_loaded  = settingFile.value(settingSelectStr+MAPPINGTABLE_FLOATINGWINDOW_MOUSEPASSTHROUGHLIST).toStringList();

        // Check if we have any tab configuration data to determine if we should initialize
        if (tabnamelist_loaded.isEmpty()) {
            initKeyMappingTable = true;
        }
        else {
            // We have tab configuration data, process the mapping data if it exists
            if (!original_keys_loaded.isEmpty() && !mapping_keys_loaded.isEmpty()) {
                original_keys_split = original_keys_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
                mapping_keys_split = mapping_keys_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
                mappingkeys_keyup_split = mappingkeys_keyup_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);

                if (original_keys_split.size() == mapping_keys_split.size()
                    && original_keys_split.size() > 0) {
                    table_count = original_keys_split.size();
                }

                if (mappingkeys_keyup_split.size() != mapping_keys_split.size()) {
                    mappingkeys_keyup_split = mapping_keys_split;
                }
            }
            else {
                // Even if we don't have mapping data, we should still create tabs based on tabnamelist
                // This ensures that saved tab configurations (names, hotkeys, colors, etc.) are loaded
                table_count = tabnamelist_loaded.size();

                // Initialize empty split arrays for consistency
                original_keys_split.clear();
                mapping_keys_split.clear();
                mappingkeys_keyup_split.clear();
                for (int i = 0; i < table_count; ++i) {
                    original_keys_split.append(QString());
                    mapping_keys_split.append(QString());
                    mappingkeys_keyup_split.append(QString());
                }
            }
        }

        if (false == initKeyMappingTable && table_count > 0) {
            clearKeyMappingTabWidget();
            KeyMappingDataList->clear();

            if (tabhotkeylist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    tabhotkeylist_loaded.append(QString());
                }
            }
            if (tabfontcolorlist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    tabfontcolorlist_loaded.append(QString());
                }
            }
            if (tabbgcolorlist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    tabbgcolorlist_loaded.append(QString());
                }
            }
            if (tabcustomimage_pathlist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    tabcustomimage_pathlist_loaded.append(QString());
                }
            }
            if (tabcustomimage_showpositionlist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    tabcustomimage_showpositionlist_loaded.append(QString());
                }
            }
            if (tabcustomimage_paddinglist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    tabcustomimage_paddinglist_loaded.append(QString());
                }
            }
            if (tabcustomimage_showastrayiconlist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    tabcustomimage_showastrayiconlist_loaded.append(QString());
                }
            }
            if (tabcustomimage_showasfloatingwindowlist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    tabcustomimage_showasfloatingwindowlist_loaded.append(QString());
                }
            }
            if (tabcustomimage_trayiconpixellist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    tabcustomimage_trayiconpixellist_loaded.append(QSize());
                }
            }
            if (floatingwindow_positionlist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    floatingwindow_positionlist_loaded.append(QPoint(-80000, -80000));
                }
            }
            if (floatingwindow_sizelist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    floatingwindow_sizelist_loaded.append(QSize());
                }
            }
            if (floatingwindow_bgcolorlist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    floatingwindow_bgcolorlist_loaded.append(QString());
                }
            }
            if (floatingwindow_radiuslist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    floatingwindow_radiuslist_loaded.append(QString());
                }
            }
            if (floatingwindow_opacitylist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    floatingwindow_opacitylist_loaded.append(QString());
                }
            }
            if (floatingwindow_mousepassthroughlist_loaded.isEmpty()) {
                for (int i = 0; i < table_count; ++i) {
                    floatingwindow_mousepassthroughlist_loaded.append(QString());
                }
            }

            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_NOTE)) {
                QVariant notes_loaded_variant = settingFile.value(settingSelectStr+KEYMAPDATA_NOTE);
                notes_load_asString = notes_loaded_variant.canConvert<QString>();
                if (notes_load_asString) {
                    notes_loaded_string = notes_loaded_variant.toString();
                    notes_split_string = notes_loaded_string.split(SEPARATOR_KEYMAPDATA_LEVEL2);
                }
                else {
                    notes_loaded = notes_loaded_variant.toList();
                    for (const QVariant &variant : std::as_const(notes_loaded)) {
                        notes_split.append(variant.toStringList());
                    }
                }
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CATEGORY)) {
                categorys_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CATEGORY).toList();
                for (const QVariant &variant : std::as_const(categorys_loaded)) {
                    categorys_split.append(variant.toStringList());
                }
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURST)) {
                burstData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_BURST).toString();
                burstData_split = burstData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURSTPRESS_TIME)) {
                burstpressData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_BURSTPRESS_TIME).toString();
                burstpressData_split = burstpressData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME)) {
                burstreleaseData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME).toString();
                burstreleaseData_split = burstreleaseData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_LOCK)) {
                lockData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_LOCK).toString();
                lockData_split = lockData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYUNLOCK)) {
                mappingkeyunlockData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_MAPPINGKEYUNLOCK).toString();
                mappingkeyunlockData_split = mappingkeyunlockData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_POSTMAPPINGKEY)) {
                postmappingkeyData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_POSTMAPPINGKEY).toString();
                postmappingkeyData_split = postmappingkeyData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_FIXEDVKEYCODE)) {
                fixedvkeycodeData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_FIXEDVKEYCODE).toString();
                fixedvkeycodeData_split = fixedvkeycodeData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CHECKCOMBKEYORDER)) {
                checkcombkeyorderData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CHECKCOMBKEYORDER).toString();
                checkcombkeyorderData_split = checkcombkeyorderData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_UNBREAKABLE)) {
                unbreakableData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_UNBREAKABLE).toString();
                unbreakableData_split = unbreakableData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_PASSTHROUGH)) {
                passthroughData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_PASSTHROUGH).toString();
                passthroughData_split = passthroughData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            // if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_KEYUP_ACTION)) {
            //     keyup_actionData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_KEYUP_ACTION).toString();
            //     keyup_actionData_split = keyup_actionData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            // }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_SENDTIMING)) {
                sendtimingData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_SENDTIMING).toString();
                sendtimingData_split = sendtimingData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_KEYSEQHOLDDOWN)) {
                keyseqholddownData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_KEYSEQHOLDDOWN).toString();
                keyseqholddownData_split = keyseqholddownData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_REPEATMODE)) {
                repeatmodeData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_REPEATMODE).toString();
                repeatmodeData_split = repeatmodeData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_REPEATIMES)) {
                repeattimesData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_REPEATIMES).toString();
                repeattimesData_split = repeattimesData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_CENTERCOLOR)) {
                crosshair_centercolorData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_CENTERCOLOR).toString();
                crosshair_centercolorData_split = crosshair_centercolorData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_CENTERSIZE)) {
                crosshair_centersizeData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_CENTERSIZE).toString();
                crosshair_centersizeData_split = crosshair_centersizeData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_CENTEROPACITY)) {
                crosshair_centeropacityData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_CENTEROPACITY).toString();
                crosshair_centeropacityData_split = crosshair_centeropacityData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIRCOLOR)) {
                crosshair_crosshaircolorData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIRCOLOR).toString();
                crosshair_crosshaircolorData_split = crosshair_crosshaircolorData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIRWIDTH)) {
                crosshair_crosshairwidthData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIRWIDTH).toString();
                crosshair_crosshairwidthData_split = crosshair_crosshairwidthData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIRLENGTH)) {
                crosshair_crosshairlengthData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIRLENGTH).toString();
                crosshair_crosshairlengthData_split = crosshair_crosshairlengthData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIROPACITY)) {
                crosshair_crosshairopacityData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_CROSSHAIROPACITY).toString();
                crosshair_crosshairopacityData_split = crosshair_crosshairopacityData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWCENTER)) {
                crosshair_showcenterData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWCENTER).toString();
                crosshair_showcenterData_split = crosshair_showcenterData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWTOP)) {
                crosshair_showtopData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWTOP).toString();
                crosshair_showtopData_split = crosshair_showtopData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWBOTTOM)) {
                crosshair_showbottomData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWBOTTOM).toString();
                crosshair_showbottomData_split = crosshair_showbottomData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWLEFT)) {
                crosshair_showleftData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWLEFT).toString();
                crosshair_showleftData_split = crosshair_showleftData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWRIGHT)) {
                crosshair_showrightData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_SHOWRIGHT).toString();
                crosshair_showrightData_split = crosshair_showrightData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_X_OFFSET)) {
                crosshair_x_offsetData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_X_OFFSET).toString();
                crosshair_x_offsetData_split = crosshair_x_offsetData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_CROSSHAIR_Y_OFFSET)) {
                crosshair_y_offsetData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_CROSSHAIR_Y_OFFSET).toString();
                crosshair_y_offsetData_split = crosshair_y_offsetData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }

            for (int index = 0; index < table_count && datavalidflag != false; ++index) {
                QList<MAP_KEYDATA> loadkeymapdata;
                bool empty_flag = false;

                // Check if we have mapping data for this tab index
                bool hasMappingData = (index < original_keys_split.size() && index < mapping_keys_split.size());

                if (hasMappingData) {
                    if (original_keys_split.at(index).isEmpty() && mapping_keys_split.at(index).isEmpty()) {
                        empty_flag = true;
                    }
                }
                else {
                    // No mapping data for this tab, treat as empty but still create the tab
                    empty_flag = true;
                }

                if (!empty_flag && hasMappingData) {
                    QStringList original_keys;
                    QStringList mapping_keys;
                    QStringList mappingkeys_keyup;
                    QStringList burstStringList;
                    QStringList burstpressStringList;
                    QStringList burstreleaseStringList;
                    QStringList lockStringList;
                    QStringList mappingkeyunlockStringList;
                    QStringList postmappingkeyStringList;
                    QStringList fixedvkeycodeStringList;
                    QStringList checkcombkeyorderStringList;
                    QStringList unbreakableStringList;
                    QStringList passthroughStringList;
                    // QStringList keyup_actionStringList;
                    QStringList sendtimingStringList;
                    QStringList keyseqholddownStringList;
                    QStringList repeatmodeStringList;
                    QStringList repeattimesStringList;
                    QStringList notesList;
                    QStringList categorysList;
                    QStringList crosshair_centercolorStringList;
                    QStringList crosshair_centersizeStringList;
                    QStringList crosshair_centeropacityStringList;
                    QStringList crosshair_crosshaircolorStringList;
                    QStringList crosshair_crosshairwidthStringList;
                    QStringList crosshair_crosshairlengthStringList;
                    QStringList crosshair_crosshairopacityStringList;
                    QStringList crosshair_showcenterStringList;
                    QStringList crosshair_showtopStringList;
                    QStringList crosshair_showbottomStringList;
                    QStringList crosshair_showleftStringList;
                    QStringList crosshair_showrightStringList;
                    QStringList crosshair_x_offsetStringList;
                    QStringList crosshair_y_offsetStringList;
                    QList<bool> burstList;
                    QList<int> burstpresstimeList;
                    QList<int> burstreleasetimeList;
                    QList<bool> lockList;
                    QList<bool> mappingkeyunlockList;
                    QList<bool> postmappingkeyList;
                    QList<int> fixedvkeycodeList;
                    QList<bool> checkcombkeyorderList;
                    QList<bool> unbreakableList;
                    QList<bool> passthroughList;
                    // QList<bool> keyup_actionList;
                    QList<int> sendtimingList;
                    QList<bool> keyseqholddownList;
                    QList<int> repeatmodeList;
                    QList<int> repeattimesList;
                    QList<QColor> crosshair_centercolorList;
                    QList<int> crosshair_centersizeList;
                    QList<int> crosshair_centeropacityList;
                    QList<QColor> crosshair_crosshaircolorList;
                    QList<int> crosshair_crosshairwidthList;
                    QList<int> crosshair_crosshairlengthList;
                    QList<int> crosshair_crosshairopacityList;
                    QList<bool> crosshair_showcenterList;
                    QList<bool> crosshair_showtopList;
                    QList<bool> crosshair_showbottomList;
                    QList<bool> crosshair_showleftList;
                    QList<bool> crosshair_showrightList;
                    QList<int> crosshair_x_offsetList;
                    QList<int> crosshair_y_offsetList;

                    original_keys = original_keys_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    mapping_keys = mapping_keys_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    mappingkeys_keyup = mappingkeys_keyup_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);

                    if (mappingkeys_keyup.size() != mapping_keys.size()) {
                        mappingkeys_keyup = mapping_keys;
                    }

                    int mappingdata_size = original_keys.size();
                    QStringList stringListAllON;
                    QStringList stringListAllOFF;
                    QStringList stringListAllZERO;
                    QStringList stringListAllNORMAL;
                    QStringList burstpressStringListDefault;
                    QStringList burstreleaseStringListDefault;
                    QStringList fixedvkeycodeStringListDefault;
                    QStringList repeattimesStringListDefault;
                    QStringList crosshair_centercolorStringListDefault;
                    QStringList crosshair_centersizeStringListDefault;
                    QStringList crosshair_centeropacityStringListDefault;
                    QStringList crosshair_crosshaircolorStringListDefault;
                    QStringList crosshair_crosshairwidthStringListDefault;
                    QStringList crosshair_crosshairlengthStringListDefault;
                    QStringList crosshair_crosshairopacityStringListDefault;
                    for (int i = 0; i < mappingdata_size; ++i) {
                        stringListAllON << "ON";
                        stringListAllOFF << "OFF";
                        stringListAllZERO << "0";
                        stringListAllNORMAL << SENDTIMING_STR_NORMAL;
                        burstpressStringListDefault.append(QString::number(BURST_PRESS_TIME_DEFAULT));
                        burstreleaseStringListDefault.append(QString::number(BURST_RELEASE_TIME_DEFAULT));
                        fixedvkeycodeStringListDefault.append(QString::number(FIXED_VIRTUAL_KEY_CODE_NONE, 16));
                        repeattimesStringListDefault.append(QString::number(REPEAT_TIMES_DEFAULT));
                        crosshair_centercolorStringListDefault.append(CROSSHAIR_CENTERCOLOR_DEFAULT);
                        crosshair_centersizeStringListDefault.append(QString::number(CROSSHAIR_CENTERSIZE_DEFAULT));
                        crosshair_centeropacityStringListDefault.append(QString::number(CROSSHAIR_CENTEROPACITY_DEFAULT));
                        crosshair_crosshaircolorStringListDefault.append(CROSSHAIR_CROSSHAIRCOLOR_DEFAULT);
                        crosshair_crosshairwidthStringListDefault.append(QString::number(CROSSHAIR_CROSSHAIRWIDTH_DEFAULT));
                        crosshair_crosshairlengthStringListDefault.append(QString::number(CROSSHAIR_CROSSHAIRLENGTH_DEFAULT));
                        crosshair_crosshairopacityStringListDefault.append(QString::number(CROSSHAIR_CROSSHAIROPACITY_DEFAULT));
                    }
                    burstStringList         = stringListAllOFF;
                    burstpressStringList    = burstpressStringListDefault;
                    burstreleaseStringList  = burstreleaseStringListDefault;
                    lockStringList          = stringListAllOFF;
                    mappingkeyunlockStringList = stringListAllOFF;
                    postmappingkeyStringList = stringListAllOFF;
                    fixedvkeycodeStringList = fixedvkeycodeStringListDefault;
                    checkcombkeyorderStringList = stringListAllON;
                    unbreakableStringList   = stringListAllOFF;
                    passthroughStringList   = stringListAllOFF;
                    // keyup_actionStringList   = stringListAllOFF;
                    sendtimingStringList   = stringListAllNORMAL;
                    keyseqholddownStringList = stringListAllOFF;
                    repeatmodeStringList = stringListAllZERO;
                    repeattimesStringList = repeattimesStringListDefault;
                    crosshair_centercolorStringList = crosshair_centercolorStringListDefault;
                    crosshair_centersizeStringList = crosshair_centersizeStringListDefault;
                    crosshair_centeropacityStringList = crosshair_centeropacityStringListDefault;
                    crosshair_crosshaircolorStringList = crosshair_crosshaircolorStringListDefault;
                    crosshair_crosshairwidthStringList = crosshair_crosshairwidthStringListDefault;
                    crosshair_crosshairlengthStringList = crosshair_crosshairlengthStringListDefault;
                    crosshair_crosshairopacityStringList = crosshair_crosshairopacityStringListDefault;
                    crosshair_showcenterStringList = stringListAllON;
                    crosshair_showtopStringList = stringListAllON;
                    crosshair_showbottomStringList = stringListAllON;
                    crosshair_showleftStringList = stringListAllON;
                    crosshair_showrightStringList = stringListAllON;
                    crosshair_x_offsetStringList = stringListAllZERO;
                    crosshair_y_offsetStringList = stringListAllZERO;

                    if (notes_load_asString) {
                        if (notes_split_string.size() == table_count) {
                            notesList = notes_split_string.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                        }
                    }
                    else {
                        if (notes_split.size() == table_count) {
                            notesList = notes_split.at(index);
                        }
                    }
                    if (categorys_split.size() == table_count) {
                        categorysList = categorys_split.at(index);
                    }
                    if (burstData_split.size() == table_count) {
                        burstStringList = burstData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (burstpressData_split.size() == table_count) {
                        burstpressStringList = burstpressData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (burstreleaseData_split.size() == table_count) {
                        burstreleaseStringList = burstreleaseData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (lockData_split.size() == table_count) {
                        lockStringList = lockData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (mappingkeyunlockData_split.size() == table_count) {
                        mappingkeyunlockStringList = mappingkeyunlockData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (postmappingkeyData_split.size() == table_count) {
                        postmappingkeyStringList = postmappingkeyData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (fixedvkeycodeData_split.size() == table_count) {
                        fixedvkeycodeStringList = fixedvkeycodeData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (checkcombkeyorderData_split.size() == table_count) {
                        checkcombkeyorderStringList = checkcombkeyorderData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (unbreakableData_split.size() == table_count) {
                        unbreakableStringList = unbreakableData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (passthroughData_split.size() == table_count) {
                        passthroughStringList = passthroughData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    // if (keyup_actionData_split.size() == table_count) {
                    //     keyup_actionStringList = keyup_actionData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    // }
                    if (sendtimingData_split.size() == table_count) {
                        sendtimingStringList = sendtimingData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (keyseqholddownData_split.size() == table_count) {
                        keyseqholddownStringList = keyseqholddownData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (repeatmodeData_split.size() == table_count) {
                        repeatmodeStringList = repeatmodeData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (repeattimesData_split.size() == table_count) {
                        repeattimesStringList = repeattimesData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_centercolorData_split.size() == table_count) {
                        crosshair_centercolorStringList = crosshair_centercolorData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_centersizeData_split.size() == table_count) {
                        crosshair_centersizeStringList = crosshair_centersizeData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_centeropacityData_split.size() == table_count) {
                        crosshair_centeropacityStringList = crosshair_centeropacityData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_crosshaircolorData_split.size() == table_count) {
                        crosshair_crosshaircolorStringList = crosshair_crosshaircolorData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_crosshairwidthData_split.size() == table_count) {
                        crosshair_crosshairwidthStringList = crosshair_crosshairwidthData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_crosshairlengthData_split.size() == table_count) {
                        crosshair_crosshairlengthStringList = crosshair_crosshairlengthData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_crosshairopacityData_split.size() == table_count) {
                        crosshair_crosshairopacityStringList = crosshair_crosshairopacityData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_showcenterData_split.size() == table_count) {
                        crosshair_showcenterStringList = crosshair_showcenterData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_showtopData_split.size() == table_count) {
                        crosshair_showtopStringList = crosshair_showtopData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_showbottomData_split.size() == table_count) {
                        crosshair_showbottomStringList = crosshair_showbottomData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_showleftData_split.size() == table_count) {
                        crosshair_showleftStringList = crosshair_showleftData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_showrightData_split.size() == table_count) {
                        crosshair_showrightStringList = crosshair_showrightData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_x_offsetData_split.size() == table_count) {
                        crosshair_x_offsetStringList = crosshair_x_offsetData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (crosshair_y_offsetData_split.size() == table_count) {
                        crosshair_y_offsetStringList = crosshair_y_offsetData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }

                    if (original_keys.size() == mapping_keys.size() && original_keys.size() > 0) {
                        datavalidflag = true;

                        if (notesList.size() < original_keys.size()) {
                            int diff = original_keys.size() - notesList.size();
                            for (int i = 0; i < diff; ++i) {
                                notesList.append(QString());
                            }
                        }

                        if (categorysList.size() < original_keys.size()) {
                            int diff = original_keys.size() - categorysList.size();
                            for (int i = 0; i < diff; ++i) {
                                categorysList.append(QString());
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &burst = (i < burstStringList.size()) ? burstStringList.at(i) : "OFF";
                            if (burst == "ON") {
                                burstList.append(true);
                            } else {
                                burstList.append(false);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &burstpresstimeStr = (i < burstpressStringList.size()) ? burstpressStringList.at(i) : QString::number(BURST_PRESS_TIME_DEFAULT);
                            bool ok;
                            int burstpresstime = burstpresstimeStr.toInt(&ok);
                            if (!ok || burstpresstime < BURST_TIME_MIN || burstpresstime > BURST_TIME_MAX) {
                                burstpresstime = BURST_PRESS_TIME_DEFAULT;
                            }
                            burstpresstimeList.append(burstpresstime);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &burstreleasetimeStr = (i < burstreleaseStringList.size()) ? burstreleaseStringList.at(i) : QString::number(BURST_RELEASE_TIME_DEFAULT);
                            bool ok;
                            int burstreleasetime = burstreleasetimeStr.toInt(&ok);
                            if (!ok || burstreleasetime < BURST_TIME_MIN || burstreleasetime > BURST_TIME_MAX) {
                                burstreleasetime = BURST_RELEASE_TIME_DEFAULT;
                            }
                            burstreleasetimeList.append(burstreleasetime);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &lock = (i < lockStringList.size()) ? lockStringList.at(i) : "OFF";
                            if (lock == "ON") {
                                lockList.append(true);
                            } else {
                                lockList.append(false);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &mappingkeyunlock = (i < mappingkeyunlockStringList.size()) ? mappingkeyunlockStringList.at(i) : "OFF";
                            if (mappingkeyunlock == "ON") {
                                mappingkeyunlockList.append(true);
                            } else {
                                mappingkeyunlockList.append(false);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &postmappingkey = (i < postmappingkeyStringList.size()) ? postmappingkeyStringList.at(i) : "OFF";
                            if (postmappingkey == "ON") {
                                postmappingkeyList.append(true);
                            } else {
                                postmappingkeyList.append(false);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &fixedvkeycodeStr = (i < fixedvkeycodeStringList.size()) ? fixedvkeycodeStringList.at(i) : QString::number(FIXED_VIRTUAL_KEY_CODE_NONE, 16);
                            bool ok;
                            int fixedvkeycode = fixedvkeycodeStr.toInt(&ok, 16);
                            if (!ok || fixedvkeycode < FIXED_VIRTUAL_KEY_CODE_MIN || fixedvkeycode > FIXED_VIRTUAL_KEY_CODE_MAX) {
                                fixedvkeycode = FIXED_VIRTUAL_KEY_CODE_NONE;
                            }
                            fixedvkeycodeList.append(fixedvkeycode);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &checkcombkeyorder = (i < checkcombkeyorderStringList.size()) ? checkcombkeyorderStringList.at(i) : "ON";
                            if (checkcombkeyorder == "OFF") {
                                checkcombkeyorderList.append(false);
                            } else {
                                checkcombkeyorderList.append(true);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &unbreakable = (i < unbreakableStringList.size()) ? unbreakableStringList.at(i) : "OFF";
                            if (unbreakable == "ON") {
                                unbreakableList.append(true);
                            } else {
                                unbreakableList.append(false);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &passthrough = (i < passthroughStringList.size()) ? passthroughStringList.at(i) : "OFF";
                            if (passthrough == "ON") {
                                passthroughList.append(true);
                            } else {
                                passthroughList.append(false);
                            }
                        }

                        // for (int i = 0; i < original_keys.size(); i++) {
                        //     const QString &keyup_action = (i < keyup_actionStringList.size()) ? keyup_actionStringList.at(i) : "OFF";
                        //     if (keyup_action == "ON") {
                        //         keyup_actionList.append(true);
                        //     } else {
                        //         keyup_actionList.append(false);
                        //     }
                        // }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &sendtiming = (i < sendtimingStringList.size()) ? sendtimingStringList.at(i) : SENDTIMING_STR_NORMAL;
                            if (sendtiming == SENDTIMING_STR_KEYDOWN) {
                                sendtimingList.append(SENDTIMING_KEYDOWN);
                            }
                            else if (sendtiming == SENDTIMING_STR_KEYUP) {
                                sendtimingList.append(SENDTIMING_KEYUP);
                            }
                            else if (sendtiming == SENDTIMING_STR_KEYDOWN_AND_KEYUP) {
                                sendtimingList.append(SENDTIMING_KEYDOWN_AND_KEYUP);
                            }
                            else if (sendtiming == SENDTIMING_STR_NORMAL_AND_KEYUP) {
                                sendtimingList.append(SENDTIMING_NORMAL_AND_KEYUP);
                            }
                            else {
                                sendtimingList.append(SENDTIMING_NORMAL);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &keyseqholddown = (i < keyseqholddownStringList.size()) ? keyseqholddownStringList.at(i) : "OFF";
                            if (keyseqholddown == "ON") {
                                keyseqholddownList.append(true);
                            } else {
                                keyseqholddownList.append(false);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &repeatmodeStr = (i < repeatmodeStringList.size()) ? repeatmodeStringList.at(i) : QString::number(REPEAT_MODE_NONE);
                            bool ok;
                            int repeatmode = repeatmodeStr.toInt(&ok);
                            if (!ok || repeatmode < REPEAT_MODE_NONE || repeatmode > REPEAT_MODE_BYTIMES) {
                                repeatmode = REPEAT_MODE_NONE;
                            }
                            repeatmodeList.append(repeatmode);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &repeattimesStr = (i < repeattimesStringList.size()) ? repeattimesStringList.at(i) : QString::number(REPEAT_TIMES_DEFAULT);
                            bool ok;
                            int repeattimes = repeattimesStr.toInt(&ok);
                            if (!ok || repeattimes < REPEAT_TIMES_MIN || repeattimes > REPEAT_TIMES_MAX) {
                                repeattimes = REPEAT_TIMES_DEFAULT;
                            }
                            repeattimesList.append(repeattimes);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_centercolorStr = (i < crosshair_centercolorStringList.size()) ? crosshair_centercolorStringList.at(i) : CROSSHAIR_CENTERCOLOR_DEFAULT;
                            QColor crosshair_centercolor = QColor(QString("%1%2").arg("#", crosshair_centercolorStr));
                            bool isvalid = crosshair_centercolor.isValid();
                            if (!isvalid) {
                                crosshair_centercolor = CROSSHAIR_CENTERCOLOR_DEFAULT_QCOLOR;
                            }
                            crosshair_centercolorList.append(crosshair_centercolor);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_centersizeStr = (i < crosshair_centersizeStringList.size()) ? crosshair_centersizeStringList.at(i) : QString::number(CROSSHAIR_CENTERSIZE_DEFAULT);
                            bool ok;
                            int crosshair_centersize = crosshair_centersizeStr.toInt(&ok);
                            if (!ok || crosshair_centersize < CROSSHAIR_CENTERSIZE_MIN || crosshair_centersize > CROSSHAIR_CENTERSIZE_MAX) {
                                crosshair_centersize = CROSSHAIR_CENTERSIZE_DEFAULT;
                            }
                            crosshair_centersizeList.append(crosshair_centersize);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_centeropacityStr = (i < crosshair_centeropacityStringList.size()) ? crosshair_centeropacityStringList.at(i) : QString::number(CROSSHAIR_CENTEROPACITY_DEFAULT);
                            bool ok;
                            int crosshair_centeropacity = crosshair_centeropacityStr.toInt(&ok);
                            if (!ok || crosshair_centeropacity < CROSSHAIR_OPACITY_MIN || crosshair_centeropacity > CROSSHAIR_OPACITY_MAX) {
                                crosshair_centeropacity = CROSSHAIR_CENTEROPACITY_DEFAULT;
                            }
                            crosshair_centeropacityList.append(crosshair_centeropacity);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_crosshaircolorStr = (i < crosshair_crosshaircolorStringList.size()) ? crosshair_crosshaircolorStringList.at(i) : CROSSHAIR_CROSSHAIRCOLOR_DEFAULT;
                            QColor crosshair_crosshaircolor = QColor(QString("%1%2").arg("#", crosshair_crosshaircolorStr));
                            bool isvalid = crosshair_crosshaircolor.isValid();
                            if (!isvalid) {
                                crosshair_crosshaircolor = CROSSHAIR_CROSSHAIRCOLOR_DEFAULT_QCOLOR;
                            }
                            crosshair_crosshaircolorList.append(crosshair_crosshaircolor);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_crosshairwidthStr = (i < crosshair_crosshairwidthStringList.size()) ? crosshair_crosshairwidthStringList.at(i) : QString::number(CROSSHAIR_CROSSHAIRWIDTH_DEFAULT);
                            bool ok;
                            int crosshair_crosshairwidth = crosshair_crosshairwidthStr.toInt(&ok);
                            if (!ok || crosshair_crosshairwidth < CROSSHAIR_CROSSHAIRWIDTH_MIN || crosshair_crosshairwidth > CROSSHAIR_CROSSHAIRWIDTH_MAX) {
                                crosshair_crosshairwidth = CROSSHAIR_CROSSHAIRWIDTH_DEFAULT;
                            }
                            crosshair_crosshairwidthList.append(crosshair_crosshairwidth);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_crosshairlengthStr = (i < crosshair_crosshairlengthStringList.size()) ? crosshair_crosshairlengthStringList.at(i) : QString::number(CROSSHAIR_CROSSHAIRLENGTH_DEFAULT);
                            bool ok;
                            int crosshair_crosshairlength = crosshair_crosshairlengthStr.toInt(&ok);
                            if (!ok || crosshair_crosshairlength < CROSSHAIR_CROSSHAIRLENGTH_MIN || crosshair_crosshairlength > CROSSHAIR_CROSSHAIRLENGTH_MAX) {
                                crosshair_crosshairlength = CROSSHAIR_CROSSHAIRLENGTH_DEFAULT;
                            }
                            crosshair_crosshairlengthList.append(crosshair_crosshairlength);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_crosshairopacityStr = (i < crosshair_crosshairopacityStringList.size()) ? crosshair_crosshairopacityStringList.at(i) : QString::number(CROSSHAIR_CROSSHAIROPACITY_DEFAULT);
                            bool ok;
                            int crosshair_crosshairopacity = crosshair_crosshairopacityStr.toInt(&ok);
                            if (!ok || crosshair_crosshairopacity < CROSSHAIR_OPACITY_MIN || crosshair_crosshairopacity > CROSSHAIR_OPACITY_MAX) {
                                crosshair_crosshairopacity = CROSSHAIR_CROSSHAIROPACITY_DEFAULT;
                            }
                            crosshair_crosshairopacityList.append(crosshair_crosshairopacity);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_showcenter = (i < crosshair_showcenterStringList.size()) ? crosshair_showcenterStringList.at(i) : "ON";
                            if (crosshair_showcenter == "OFF") {
                                crosshair_showcenterList.append(false);
                            }
                            else {
                                crosshair_showcenterList.append(true);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_showtop = (i < crosshair_showtopStringList.size()) ? crosshair_showtopStringList.at(i) : "ON";
                            if (crosshair_showtop == "OFF") {
                                crosshair_showtopList.append(false);
                            }
                            else {
                                crosshair_showtopList.append(true);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_showbottom = (i < crosshair_showbottomStringList.size()) ? crosshair_showbottomStringList.at(i) : "ON";
                            if (crosshair_showbottom == "OFF") {
                                crosshair_showbottomList.append(false);
                            }
                            else {
                                crosshair_showbottomList.append(true);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_showleft = (i < crosshair_showleftStringList.size()) ? crosshair_showleftStringList.at(i) : "ON";
                            if (crosshair_showleft == "OFF") {
                                crosshair_showleftList.append(false);
                            }
                            else {
                                crosshair_showleftList.append(true);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_showright = (i < crosshair_showrightStringList.size()) ? crosshair_showrightStringList.at(i) : "ON";
                            if (crosshair_showright == "OFF") {
                                crosshair_showrightList.append(false);
                            }
                            else {
                                crosshair_showrightList.append(true);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_x_offsetStr = (i < crosshair_x_offsetStringList.size()) ? crosshair_x_offsetStringList.at(i) : QString::number(CROSSHAIR_X_OFFSET_DEFAULT);
                            bool ok;
                            int crosshair_x_offset = crosshair_x_offsetStr.toInt(&ok);
                            if (!ok || crosshair_x_offset < CROSSHAIR_X_OFFSET_MIN || crosshair_x_offset > CROSSHAIR_X_OFFSET_MAX) {
                                crosshair_x_offset = CROSSHAIR_X_OFFSET_DEFAULT;
                            }
                            crosshair_x_offsetList.append(crosshair_x_offset);
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &crosshair_y_offsetStr = (i < crosshair_y_offsetStringList.size()) ? crosshair_y_offsetStringList.at(i) : QString::number(CROSSHAIR_Y_OFFSET_DEFAULT);
                            bool ok;
                            int crosshair_y_offset = crosshair_y_offsetStr.toInt(&ok);
                            if (!ok || crosshair_y_offset < CROSSHAIR_Y_OFFSET_MIN || crosshair_y_offset > CROSSHAIR_Y_OFFSET_MAX) {
                                crosshair_y_offset = CROSSHAIR_Y_OFFSET_DEFAULT;
                            }
                            crosshair_y_offsetList.append(crosshair_y_offset);
                        }

                        int loadindex = 0;
                        for (const QString &ori_key_nochange : std::as_const(original_keys)){
                            QString ori_key = ori_key_nochange;
                            if (ori_key.startsWith(OLD_PREFIX_SHORTCUT)) {
                                ori_key.remove(0, 1);
                            }

                            bool checkoriginalstr = checkOriginalkeyStr(ori_key);

                            // Unescape SendText content in mapping keys for safe loading
                            QString unescaped_mapping_keys = unescapeSendTextForLoading(mapping_keys[loadindex]);
                            bool checkmappingstr = checkMappingkeyStr(unescaped_mapping_keys);

                            if (mappingkeys_keyup[loadindex].isEmpty()) {
                                mappingkeys_keyup[loadindex] = mapping_keys[loadindex];
                            }
                            // Unescape SendText content in keyup mapping keys for safe loading
                            QString unescaped_mappingkeys_keyup = unescapeSendTextForLoading(mappingkeys_keyup[loadindex]);
                            bool checkmapping_keyupstr = checkMappingkeyStr(unescaped_mappingkeys_keyup);
                            if (!checkmapping_keyupstr) {
                                mappingkeys_keyup[loadindex] = mapping_keys[loadindex];
                                unescaped_mappingkeys_keyup = unescaped_mapping_keys;
                            }

                            if (true == checkoriginalstr && true == checkmappingstr) {
                                loadkeymapdata.append(MAP_KEYDATA(ori_key,
                                                                  unescaped_mapping_keys,
                                                                  unescaped_mappingkeys_keyup,
                                                                  notesList.at(loadindex),
                                                                  categorysList.at(loadindex),
                                                                  burstList.at(loadindex),
                                                                  burstpresstimeList.at(loadindex),
                                                                  burstreleasetimeList.at(loadindex),
                                                                  lockList.at(loadindex),
                                                                  mappingkeyunlockList.at(loadindex),
                                                                  postmappingkeyList.at(loadindex),
                                                                  fixedvkeycodeList.at(loadindex),
                                                                  checkcombkeyorderList.at(loadindex),
                                                                  unbreakableList.at(loadindex),
                                                                  passthroughList.at(loadindex),
                                                                  sendtimingList.at(loadindex),
                                                                  keyseqholddownList.at(loadindex),
                                                                  repeatmodeList.at(loadindex),
                                                                  repeattimesList.at(loadindex),
                                                                  crosshair_centercolorList.at(loadindex),
                                                                  crosshair_centersizeList.at(loadindex),
                                                                  crosshair_centeropacityList.at(loadindex),
                                                                  crosshair_crosshaircolorList.at(loadindex),
                                                                  crosshair_crosshairwidthList.at(loadindex),
                                                                  crosshair_crosshairlengthList.at(loadindex),
                                                                  crosshair_crosshairopacityList.at(loadindex),
                                                                  crosshair_showcenterList.at(loadindex),
                                                                  crosshair_showtopList.at(loadindex),
                                                                  crosshair_showbottomList.at(loadindex),
                                                                  crosshair_showleftList.at(loadindex),
                                                                  crosshair_showrightList.at(loadindex),
                                                                  crosshair_x_offsetList.at(loadindex),
                                                                  crosshair_y_offsetList.at(loadindex)
                                                                  ));
                            }
                            else{
                                datavalidflag = false;
#ifdef DEBUG_LOGOUT_ON
                                qWarning("[loadKeyMapSetting] Invalid data loaded -> checkoriginalstr(%s), checkmappingstr(%s)", checkoriginalstr?"true":"false", checkmappingstr?"true":"false");
#endif
                                break;
                            }

                            loadindex += 1;
                        }
                    }
                }
                else {
                    datavalidflag = true;
                }

                bool tabvalid_flag = true;
                if (index  == s_KeyMappingTabInfoList.size()) {
                    tabvalid_flag = addTabToKeyMappingTabWidget();
                }

                if (tabvalid_flag) {
                    (*s_KeyMappingTabInfoList[index].KeyMappingData) = loadkeymapdata;
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[loadKeyMapSetting]" << "addTabToKeyMappingTabWidget() failed! index =" << index;
#endif
                }
            }

            for (int index = 0; index < tabnamelist_loaded.size(); ++index) {
                if ((index < s_KeyMappingTabInfoList.size()) && (index < m_KeyMappingTabWidget->count())) {
                    s_KeyMappingTabInfoList[index].TabName = tabnamelist_loaded.at(index);
                    m_KeyMappingTabWidget->setTabText(index, tabnamelist_loaded.at(index));
                }
            }

            if (datavalidflag != (quint8)true) {
                if (loadGlobalSetting && (0xFF == datavalidflag)) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[loadKeyMapSetting]" << "Empty Global Setting do not load default!";
#endif
                }
                else {
                    clearKeyMappingTabWidget();
                    KeyMappingDataList->clear();
                    loadDefault = true;
                }
            }
        }
        else {
            clearKeyMappingTabWidget();
            KeyMappingDataList->clear();
        }
    }
    else {
        clearKeyMappingTabWidget();
        KeyMappingDataList->clear();
    }

    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index) {
        if (index < tabhotkeylist_loaded.size()) {
            QString tabhotkeystring = tabhotkeylist_loaded.at(index);
            QString ori_tabhotkeystring = tabhotkeystring;
            if (tabhotkeystring.startsWith(PREFIX_PASSTHROUGH)) {
                tabhotkeystring.remove(0, 1);
            }

            if (tabhotkeystring.isEmpty() == false
                && QKeyMapper::validateCombinationKey(tabhotkeystring)) {
                s_KeyMappingTabInfoList[index].TabHotkey = ori_tabhotkeystring;
            }
            else {
                s_KeyMappingTabInfoList[index].TabHotkey.clear();
            }
        }
        else {
            s_KeyMappingTabInfoList[index].TabHotkey.clear();
        }
        // updateKeyMappingTabWidgetTabDisplay(index);
    }

    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index) {
        if (index < tabfontcolorlist_loaded.size()) {
            QColor TabFontColor = QColor(tabfontcolorlist_loaded.at(index));
            if (TabFontColor.isValid()) {
                s_KeyMappingTabInfoList[index].TabFontColor = TabFontColor;
            }
            else {
                s_KeyMappingTabInfoList[index].TabFontColor = QColor();
            }
        }
        if (index < tabbgcolorlist_loaded.size()) {
            QColor TabBGColor = QColor(tabbgcolorlist_loaded.at(index));
            if (TabBGColor.isValid()) {
                s_KeyMappingTabInfoList[index].TabBackgroundColor = TabBGColor;
            }
            else {
                s_KeyMappingTabInfoList[index].TabBackgroundColor = QColor();
            }
        }
        if (index < tabcustomimage_trayiconpixellist_loaded.size()) {
            QSize trayiconpixel = tabcustomimage_trayiconpixellist_loaded.at(index).toSize();
            if (!trayiconpixel.isEmpty()) {
                s_KeyMappingTabInfoList[index].TabCustomImage_TrayIconPixel = trayiconpixel;
            }
            else {
                s_KeyMappingTabInfoList[index].TabCustomImage_TrayIconPixel = QSize();
            }
        }
        if (index < tabcustomimage_pathlist_loaded.size()) {
            QString tabcustomimage_path = tabcustomimage_pathlist_loaded.at(index);
            QIcon icon_loaded(tabcustomimage_path);

            if (!icon_loaded.isNull()) {
                s_KeyMappingTabInfoList[index].TabCustomImage_Path = tabcustomimage_path;

                QList<QSize> availableSizes = icon_loaded.availableSizes();
                if (!availableSizes.contains(s_KeyMappingTabInfoList[index].TabCustomImage_TrayIconPixel)) {
                    s_KeyMappingTabInfoList[index].TabCustomImage_TrayIconPixel = QSize();
                }
            }
            else {
                s_KeyMappingTabInfoList[index].TabCustomImage_Path.clear();
                s_KeyMappingTabInfoList[index].TabCustomImage_TrayIconPixel = QSize();
            }
        }
        if (index < tabcustomimage_showpositionlist_loaded.size()) {
            int tabcustomimage_showposition = tabcustomimage_showpositionlist_loaded.at(index).toInt();
            if (TAB_CUSTOMIMAGE_POSITION_MIN <= tabcustomimage_showposition && tabcustomimage_showposition <= TAB_CUSTOMIMAGE_POSITION_MAX) {
                s_KeyMappingTabInfoList[index].TabCustomImage_ShowPosition = tabcustomimage_showposition;
            }
            else {
                s_KeyMappingTabInfoList[index].TabCustomImage_ShowPosition = TAB_CUSTOMIMAGE_POSITION_DEFAULT;
            }
        }
        if (index < tabcustomimage_paddinglist_loaded.size()) {
            int tabcustomimage_padding = tabcustomimage_paddinglist_loaded.at(index).toInt();
            if (TAB_CUSTOMIMAGE_PADDING_MIN <= tabcustomimage_padding && tabcustomimage_padding <= TAB_CUSTOMIMAGE_PADDING_MAX) {
                s_KeyMappingTabInfoList[index].TabCustomImage_Padding = tabcustomimage_padding;
            }
            else {
                s_KeyMappingTabInfoList[index].TabCustomImage_Padding = TAB_CUSTOMIMAGE_PADDING_DEFAULT;
            }
        }
        if (index < floatingwindow_positionlist_loaded.size()) {
            QPoint floatingwindow_position = floatingwindow_positionlist_loaded.at(index).toPoint();
            if (floatingwindow_position.x() < FLOATINGWINDOW_POSITION_MIN.x()
                || floatingwindow_position.y() < FLOATINGWINDOW_POSITION_MIN.y()
                || floatingwindow_position.x() > FLOATINGWINDOW_POSITION_MAX.x()
                || floatingwindow_position.y() > FLOATINGWINDOW_POSITION_MAX.y()) {
                s_KeyMappingTabInfoList[index].FloatingWindow_Position = FLOATINGWINDOW_POSITION_DEFAULT;
            }
            else {
                s_KeyMappingTabInfoList[index].FloatingWindow_Position = floatingwindow_position;
            }
        }
        if (index < floatingwindow_sizelist_loaded.size()) {
            QSize floatingwindow_size = floatingwindow_sizelist_loaded.at(index).toSize();
            if (floatingwindow_size.width() < FLOATINGWINDOW_SIZE_MIN
                || floatingwindow_size.height() < FLOATINGWINDOW_SIZE_MIN
                || floatingwindow_size.width() > FLOATINGWINDOW_SIZE_MAX
                || floatingwindow_size.height() > FLOATINGWINDOW_SIZE_MAX) {
                s_KeyMappingTabInfoList[index].FloatingWindow_Size = FLOATINGWINDOW_SIZE_DEFAULT;
            }
            else {
                s_KeyMappingTabInfoList[index].FloatingWindow_Size = floatingwindow_size;
            }
        }
        if (index < floatingwindow_bgcolorlist_loaded.size()) {
            QColor FloatingWindowBGColor = QColor(floatingwindow_bgcolorlist_loaded.at(index));
            if (FloatingWindowBGColor.isValid()) {
                s_KeyMappingTabInfoList[index].FloatingWindow_BackgroundColor = FloatingWindowBGColor;
            }
            else {
                s_KeyMappingTabInfoList[index].FloatingWindow_BackgroundColor = QColor();
            }
        }
        if (index < floatingwindow_radiuslist_loaded.size()) {
            int floatingwindow_radius = floatingwindow_radiuslist_loaded.at(index).toDouble();
            if (FLOATINGWINDOW_RADIUS_MIN <= floatingwindow_radius && floatingwindow_radius <= FLOATINGWINDOW_RADIUS_MAX) {
                s_KeyMappingTabInfoList[index].FloatingWindow_Radius = floatingwindow_radius;
            }
            else {
                s_KeyMappingTabInfoList[index].FloatingWindow_Radius = FLOATINGWINDOW_RADIUS_DEFAULT;
            }
        }
        if (index < floatingwindow_opacitylist_loaded.size()) {
            double floatingwindow_opacity = floatingwindow_opacitylist_loaded.at(index).toDouble();
            if (FLOATINGWINDOW_OPACITY_MIN <= floatingwindow_opacity && floatingwindow_opacity <= FLOATINGWINDOW_OPACITY_MAX) {
                s_KeyMappingTabInfoList[index].FloatingWindow_Opacity = floatingwindow_opacity;
            }
            else {
                s_KeyMappingTabInfoList[index].FloatingWindow_Opacity = FLOATINGWINDOW_OPACITY_DEFAULT;
            }
        }
        if (index < tabcustomimage_showastrayiconlist_loaded.size()) {
            const QString &str = tabcustomimage_showastrayiconlist_loaded.at(index);
            s_KeyMappingTabInfoList[index].TabCustomImage_ShowAsTrayIcon =
                (str == "ON") ? true :
                (str == "OFF") ? false :
                TAB_CUSTOMIMAGE_SHOW_AS_TRAYICON_DEFAULT;
        }
        if (index < tabcustomimage_showasfloatingwindowlist_loaded.size()) {
            const QString &str = tabcustomimage_showasfloatingwindowlist_loaded.at(index);
            s_KeyMappingTabInfoList[index].TabCustomImage_ShowAsFloatingWindow =
                (str == "ON") ? true :
                (str == "OFF") ? false :
                TAB_CUSTOMIMAGE_SHOW_AS_FLOATINGWINDOW_DEFAULT;
        }
        if (index < floatingwindow_mousepassthroughlist_loaded.size()) {
            const QString &str = floatingwindow_mousepassthroughlist_loaded.at(index);
            s_KeyMappingTabInfoList[index].FloatingWindow_MousePassThrough =
                (str == "ON") ? true :
                (str == "OFF") ? false :
                FLOATINGWINDOW_MOUSE_PASSTHROUGH_DEFAULT;
        }
        updateKeyMappingTabWidgetTabDisplay(index);
    }

    collectMappingTableTabHotkeys();

    int last_mappingtable_index = 0;
    if (true == settingFile.contains(settingSelectStr+MAPPINGTABLE_LASTTABINDEX)){
        last_mappingtable_index = settingFile.value(settingSelectStr+MAPPINGTABLE_LASTTABINDEX).toInt();

        if (last_mappingtable_index < 0 || last_mappingtable_index >= s_KeyMappingTabInfoList.size()) {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[loadKeyMapSetting]" << "Invalid MappingTable_LastTabIndex = " << last_mappingtable_index << ", set index = 0";
#endif
            last_mappingtable_index = 0;
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "MappingTable_LastTabIndex =" << last_mappingtable_index;
#endif
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Do not contains MappingTable_LastTabIndex, set last tabindex =" << last_mappingtable_index;
#endif
    }

    forceSwitchKeyMappingTabWidgetIndex(last_mappingtable_index);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[loadKeyMapSetting]" << "refreshKeyMappingDataTable()";
#endif
    refreshAllKeyMappingTagWidget();

    if (loadGlobalSetting && loadDefault != true) {
        ui->settingNameLineEdit->setText(GROUPNAME_GLOBALSETTING);
        ui->settingNameLineEdit->setReadOnly(true);
        ui->processLineEdit->setText(QString());
        ui->windowTitleLineEdit->setText(QString());
        ui->descriptionLineEdit->setReadOnly(true);
        ui->descriptionLineEdit->setText(tr("Global keymapping setting"));
        // ui->processCheckBox->setChecked(false);
        // ui->titleCheckBox->setChecked(false);
        ui->checkProcessComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_IGNORE);
        ui->checkWindowTitleComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_IGNORE);
        // ui->disableWinKeyCheckBox->setChecked(false);
        ui->sendToSameTitleWindowsCheckBox->setChecked(false);
        ui->ProcessIconAsTrayIconCheckBox->setChecked(false);

        // ui->settingNameLineEdit->setEnabled(false);
        ui->processLineEdit->setEnabled(false);
        ui->restoreProcessPathButton->setEnabled(false);
        ui->windowTitleLineEdit->setEnabled(false);
        // ui->processCheckBox->setEnabled(false);
        // ui->titleCheckBox->setEnabled(false);
        ui->processLabel->setEnabled(false);
        ui->windowTitleLabel->setEnabled(false);
        ui->checkProcessComboBox->setEnabled(false);
        ui->checkWindowTitleComboBox->setEnabled(false);
        ui->removeSettingButton->setEnabled(false);
        // ui->disableWinKeyCheckBox->setEnabled(false);
        ui->sendToSameTitleWindowsCheckBox->setEnabled(false);
        ui->ProcessIconAsTrayIconCheckBox->setEnabled(false);

        ui->iconLabel->clear();
        m_MapProcessInfo = MAP_PROCESSINFO();
    }
    else {
        if (KEYMAP_IDLE == m_KeyMapStatus){
            ui->settingNameLineEdit->setEnabled(true);
        }
        ui->settingNameLineEdit->setReadOnly(false);
        ui->processLineEdit->setEnabled(true);
        ui->restoreProcessPathButton->setEnabled(true);
        ui->windowTitleLineEdit->setEnabled(true);
        // ui->processCheckBox->setEnabled(true);
        // ui->titleCheckBox->setEnabled(true);
        ui->processLabel->setEnabled(true);
        ui->windowTitleLabel->setEnabled(true);
        ui->checkProcessComboBox->setEnabled(true);
        ui->checkWindowTitleComboBox->setEnabled(true);
        ui->removeSettingButton->setEnabled(true);
        // ui->disableWinKeyCheckBox->setEnabled(true);
        ui->sendToSameTitleWindowsCheckBox->setEnabled(true);
        ui->ProcessIconAsTrayIconCheckBox->setEnabled(true);

        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_FILENAME)){
            m_MapProcessInfo.FileName = settingFile.value(settingSelectStr+PROCESSINFO_FILENAME).toString();
            ui->processLineEdit->setText(m_MapProcessInfo.FileName);
        }
        else {
            ui->processLineEdit->setText(QString());
        }

        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_WINDOWTITLE)){
            m_MapProcessInfo.WindowTitle = settingFile.value(settingSelectStr+PROCESSINFO_WINDOWTITLE).toString();
            ui->windowTitleLineEdit->setText(m_MapProcessInfo.WindowTitle);
        }
        else {
            ui->windowTitleLineEdit->setText(QString());
        }

        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_FILEPATH)){
            m_MapProcessInfo.FilePath = settingFile.value(settingSelectStr+PROCESSINFO_FILEPATH).toString();
        }
        else {
            m_MapProcessInfo = MAP_PROCESSINFO();
        }
        ui->iconLabel->clear();

        ui->descriptionLineEdit->setReadOnly(false);
        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_DESCRIPTION)){
            ui->descriptionLineEdit->setText(settingFile.value(settingSelectStr+PROCESSINFO_DESCRIPTION).toString());
        }
        else {
            ui->descriptionLineEdit->clear();
        }

        if (true == loadDefault) {
            // setMapProcessInfo(QString(DEFAULT_NAME), QString(DEFAULT_TITLE), QString(), QString(), QIcon(":/DefaultIcon.ico"));
            ui->settingNameLineEdit->setText(QString());
            ui->processLineEdit->setText(QString());
            ui->windowTitleLineEdit->setText(QString());
            ui->descriptionLineEdit->setText(QString());
            // ui->processCheckBox->setChecked(false);
            // ui->titleCheckBox->setChecked(false);
            ui->checkProcessComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
            ui->checkWindowTitleComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
            ui->sendToSameTitleWindowsCheckBox->setChecked(false);
            ui->ProcessIconAsTrayIconCheckBox->setChecked(false);
            m_MapProcessInfo = MAP_PROCESSINFO();
        }

        updateProcessInfoDisplay();

        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_FILENAME_MATCH_INDEX)){
            bool ok = false;
            int matchProcessIndex = settingFile.value(settingSelectStr+PROCESSINFO_FILENAME_MATCH_INDEX).toInt(&ok);
            if (!ok || matchProcessIndex < WINDOWINFO_MATCH_INDEX_MIN || matchProcessIndex > WINDOWINFO_MATCH_INDEX_MAX) {
                matchProcessIndex = WINDOWINFO_MATCH_INDEX_DEFAULT;
            }
            ui->checkProcessComboBox->setCurrentIndex(matchProcessIndex);
#ifdef DEBUG_LOGOUT_ON
            QString matchProcessString = ui->checkProcessComboBox->itemText(matchProcessIndex);
            qDebug() << "[loadKeyMapSetting]" << "FileNameMatchIndex =" << matchProcessString;
#endif
        }
        else {
            ui->checkProcessComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
        }

        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_WINDOWTITLE_MATCH_INDEX)){
            bool ok = false;
            int matchWindowTitleIndex = settingFile.value(settingSelectStr+PROCESSINFO_WINDOWTITLE_MATCH_INDEX).toInt(&ok);
            if (!ok || matchWindowTitleIndex < WINDOWINFO_MATCH_INDEX_MIN || matchWindowTitleIndex > WINDOWINFO_MATCH_INDEX_MAX) {
                matchWindowTitleIndex = WINDOWINFO_MATCH_INDEX_DEFAULT;
            }
            ui->checkWindowTitleComboBox->setCurrentIndex(matchWindowTitleIndex);
#ifdef DEBUG_LOGOUT_ON
            QString matchWindowTitleString = ui->checkProcessComboBox->itemText(matchWindowTitleIndex);
            qDebug() << "[loadKeyMapSetting]" << "WindowTitleMatchIndex =" << matchWindowTitleString;
#endif
        }
        else {
            ui->checkWindowTitleComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
        }

#if 0
        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_FILENAME_CHECKED)){
            bool fileNameChecked = settingFile.value(settingSelectStr+PROCESSINFO_FILENAME_CHECKED).toBool();
            if (true == fileNameChecked) {
                ui->processCheckBox->setChecked(true);
            }
            else {
                ui->processCheckBox->setChecked(false);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "FileNameChecked =" << fileNameChecked;
#endif
        }
        else {
            ui->processCheckBox->setChecked(false);
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
#endif

        if (true == loadDefault) {
            // ui->processCheckBox->setChecked(false);
            // ui->titleCheckBox->setChecked(false);
            ui->checkProcessComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
            ui->checkWindowTitleComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
        }
    }

#if 0
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
#endif

    if (true == settingFile.contains(settingSelectStr+KEY2MOUSE_X_SPEED)){
        int mouseXSpeed = settingFile.value(settingSelectStr+KEY2MOUSE_X_SPEED).toInt();
        ui->mouseXSpeedSpinBox->setValue(mouseXSpeed);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Mouse X Speed =" << mouseXSpeed;
#endif
    }
    else {
        ui->mouseXSpeedSpinBox->setValue(MOUSE_SPEED_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+KEY2MOUSE_Y_SPEED)){
        int mouseYSpeed = settingFile.value(settingSelectStr+KEY2MOUSE_Y_SPEED).toInt();
        ui->mouseYSpeedSpinBox->setValue(mouseYSpeed);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Mouse Y Speed =" << mouseYSpeed;
#endif
    }
    else {
        ui->mouseYSpeedSpinBox->setValue(MOUSE_SPEED_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_X_SPEED)){
        double gyro2mouseXSpeed = settingFile.value(settingSelectStr+GYRO2MOUSE_X_SPEED).toDouble();
        ui->Gyro2MouseXSpeedSpinBox->setValue(gyro2mouseXSpeed);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse X Speed =" << gyro2mouseXSpeed;
#endif
    }
    else {
        ui->Gyro2MouseXSpeedSpinBox->setValue(GYRO2MOUSE_SPEED_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_Y_SPEED)){
        double gyro2mouseYSpeed = settingFile.value(settingSelectStr+GYRO2MOUSE_Y_SPEED).toDouble();
        ui->Gyro2MouseYSpeedSpinBox->setValue(gyro2mouseYSpeed);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse Y Speed =" << gyro2mouseYSpeed;
#endif
    }
    else {
        ui->Gyro2MouseYSpeedSpinBox->setValue(GYRO2MOUSE_SPEED_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_MIN_GYRO_X_SENSITIVITY)){
        double gyro2mouseMinXSensitivity = settingFile.value(settingSelectStr+GYRO2MOUSE_MIN_GYRO_X_SENSITIVITY).toDouble();
        ui->Gyro2MouseMinXSensSpinBox->setValue(gyro2mouseMinXSensitivity);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse Min Gyro X Sensitivity =" << gyro2mouseMinXSensitivity;
#endif
    }
    else {
        ui->Gyro2MouseMinXSensSpinBox->setValue(GYRO2MOUSE_MIN_GYRO_SENS_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_MIN_GYRO_Y_SENSITIVITY)){
        double gyro2mouseMinYSensitivity = settingFile.value(settingSelectStr+GYRO2MOUSE_MIN_GYRO_Y_SENSITIVITY).toDouble();
        ui->Gyro2MouseMinYSensSpinBox->setValue(gyro2mouseMinYSensitivity);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse Min Gyro Y Sensitivity =" << gyro2mouseMinYSensitivity;
#endif
    }
    else {
        ui->Gyro2MouseMinYSensSpinBox->setValue(GYRO2MOUSE_MIN_GYRO_SENS_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_MAX_GYRO_X_SENSITIVITY)){
        double gyro2mouseMaxXSensitivity = settingFile.value(settingSelectStr+GYRO2MOUSE_MAX_GYRO_X_SENSITIVITY).toDouble();
        ui->Gyro2MouseMaxXSensSpinBox->setValue(gyro2mouseMaxXSensitivity);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse Max Gyro X Sensitivity =" << gyro2mouseMaxXSensitivity;
#endif
    }
    else {
        ui->Gyro2MouseMaxXSensSpinBox->setValue(GYRO2MOUSE_MAX_GYRO_SENS_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_MAX_GYRO_Y_SENSITIVITY)){
        double gyro2mouseMaxYSensitivity = settingFile.value(settingSelectStr+GYRO2MOUSE_MAX_GYRO_Y_SENSITIVITY).toDouble();
        ui->Gyro2MouseMaxYSensSpinBox->setValue(gyro2mouseMaxYSensitivity);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse Max Gyro Y Sensitivity =" << gyro2mouseMaxYSensitivity;
#endif
    }
    else {
        ui->Gyro2MouseMaxYSensSpinBox->setValue(GYRO2MOUSE_MAX_GYRO_SENS_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_MIN_GYRO_THRESHOLD)){
        double gyro2mouseMinThreshold = settingFile.value(settingSelectStr+GYRO2MOUSE_MIN_GYRO_THRESHOLD).toDouble();
        ui->Gyro2MouseMinThresholdSpinBox->setValue(gyro2mouseMinThreshold);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse Min Gyro Threshold =" << gyro2mouseMinThreshold;
#endif
    }
    else {
        ui->Gyro2MouseMinThresholdSpinBox->setValue(GYRO2MOUSE_MIN_GYRO_THRESHOLD_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_MAX_GYRO_THRESHOLD)){
        double gyro2mouseMaxThreshold = settingFile.value(settingSelectStr+GYRO2MOUSE_MAX_GYRO_THRESHOLD).toDouble();
        ui->Gyro2MouseMaxThresholdSpinBox->setValue(gyro2mouseMaxThreshold);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse Max Gyro Threshold =" << gyro2mouseMaxThreshold;
#endif
    }
    else {
        ui->Gyro2MouseMaxThresholdSpinBox->setValue(GYRO2MOUSE_MAX_GYRO_THRESHOLD_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_MOUSE_X_SOURCE)){
        int gyro2mouseXSourceIndex = settingFile.value(settingSelectStr+GYRO2MOUSE_MOUSE_X_SOURCE).toInt();
        if (GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_X <= gyro2mouseXSourceIndex && gyro2mouseXSourceIndex <= GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Z) {
            m_Gyro2MouseOptionDialog->setGyro2Mouse_MouseXSource(gyro2mouseXSourceIndex);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse X Source =" << gyro2mouseXSourceIndex;
#endif
        }
        else {
            m_Gyro2MouseOptionDialog->setGyro2Mouse_MouseXSource(GYRO2MOUSE_MOUSE_X_INPUT_SOURCE_DEFAULT);
        }
    }
    else {
        m_Gyro2MouseOptionDialog->setGyro2Mouse_MouseXSource(GYRO2MOUSE_MOUSE_X_INPUT_SOURCE_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_MOUSE_Y_SOURCE)){
        int gyro2mouseYSourceIndex = settingFile.value(settingSelectStr+GYRO2MOUSE_MOUSE_Y_SOURCE).toInt();
        if (GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_X <= gyro2mouseYSourceIndex && gyro2mouseYSourceIndex <= GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Z) {
            m_Gyro2MouseOptionDialog->setGyro2Mouse_MouseYSource(gyro2mouseYSourceIndex);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse Y Source =" << gyro2mouseYSourceIndex;
#endif
        }
        else {
            m_Gyro2MouseOptionDialog->setGyro2Mouse_MouseYSource(GYRO2MOUSE_MOUSE_Y_INPUT_SOURCE_DEFAULT);
        }
    }
    else {
        m_Gyro2MouseOptionDialog->setGyro2Mouse_MouseYSource(GYRO2MOUSE_MOUSE_Y_INPUT_SOURCE_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_MOUSE_X_REVERT)){
        bool gyro2mouseXRevert = settingFile.value(settingSelectStr+GYRO2MOUSE_MOUSE_X_REVERT).toBool();
        m_Gyro2MouseOptionDialog->setGyro2Mouse_MouseXRevert(gyro2mouseXRevert);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse X Revert =" << gyro2mouseXRevert;
#endif
    }
    else {
        m_Gyro2MouseOptionDialog->setGyro2Mouse_MouseXRevert(false);
    }

    if (true == settingFile.contains(settingSelectStr+GYRO2MOUSE_MOUSE_Y_REVERT)){
        bool gyro2mouseYRevert = settingFile.value(settingSelectStr+GYRO2MOUSE_MOUSE_Y_REVERT).toBool();
        m_Gyro2MouseOptionDialog->setGyro2Mouse_MouseYRevert(gyro2mouseYRevert);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Gyro2Mouse Y Revert =" << gyro2mouseYRevert;
#endif
    }
    else {
        m_Gyro2MouseOptionDialog->setGyro2Mouse_MouseYRevert(false);
    }

#ifdef VIGEM_CLIENT_SUPPORT
    if (true == settingFile.contains(settingSelectStr+MOUSE2VJOY_X_SENSITIVITY)){
        int vJoy_X_Sensitivity = settingFile.value(settingSelectStr+MOUSE2VJOY_X_SENSITIVITY).toInt();
        ui->vJoyXSensSpinBox->setValue(vJoy_X_Sensitivity);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Mouse2vJoy X Sensitivity =" << vJoy_X_Sensitivity;
#endif
    }
    else {
        ui->vJoyXSensSpinBox->setValue(VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+MOUSE2VJOY_Y_SENSITIVITY)){
        int vJoy_Y_Sensitivity = settingFile.value(settingSelectStr+MOUSE2VJOY_Y_SENSITIVITY).toInt();
        ui->vJoyYSensSpinBox->setValue(vJoy_Y_Sensitivity);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Mouse2vJoy Y Sensitivity =" << vJoy_Y_Sensitivity;
#endif
    }
    else {
        ui->vJoyYSensSpinBox->setValue(VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT);
    }

    if (true == settingFile.contains(settingSelectStr+MOUSE2VJOY_RECENTER_TIMEOUT)){
        int vJoy_Recenter_Timeout = settingFile.value(settingSelectStr+MOUSE2VJOY_RECENTER_TIMEOUT).toInt();
        ui->vJoyRecenterSpinBox->setValue(vJoy_Recenter_Timeout);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Mouse2vJoy Recenter Timeout =" << vJoy_Recenter_Timeout;
#endif
    }
    else {
        ui->vJoyRecenterSpinBox->setValue(MOUSE2VJOY_RECENTER_TIMEOUT_DEFAULT);
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
        qDebug() << "[loadKeyMapSetting]" << "Mouse2vJoy LockCursorChecked =" << lockCursorChecked;
#endif
    }
    else {
        ui->lockCursorCheckBox->setChecked(false);
    }

    if (true == settingFile.contains(settingSelectStr+MOUSE2VJOY_DIRECTMODE)){
        bool directModeChecked = settingFile.value(settingSelectStr+MOUSE2VJOY_DIRECTMODE).toBool();
        if (true == directModeChecked) {
            ui->directModeCheckBox->setChecked(true);
        }
        else {
            ui->directModeCheckBox->setChecked(false);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Mouse2vJoy DirectModeChecked =" << directModeChecked;
#endif
    }
    else {
        ui->directModeCheckBox->setChecked(false);
    }
#endif

    if (true == settingFile.contains(settingSelectStr+FILTER_KEYS)){
        bool filterKeysChecked = settingFile.value(settingSelectStr+FILTER_KEYS).toBool();
        if (true == filterKeysChecked) {
            ui->filterKeysCheckBox->setChecked(true);
            Interception_Worker::s_FilterKeys = true;
        }
        else {
            ui->filterKeysCheckBox->setChecked(false);
            Interception_Worker::s_FilterKeys = false;
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "FilterKeys Checked =" << filterKeysChecked;
#endif
    }
    else {
        ui->filterKeysCheckBox->setChecked(true);
        Interception_Worker::s_FilterKeys = true;
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "Do not contains FilterKeys, FilterKeys set to Checked.";
#endif
    }

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

    if (true == settingFile.contains(settingSelectStr+AUTOSTARTMAPPING_CHECKED)){
        bool ok = false;
        Qt::CheckState autoStartMappingCheckState = (Qt::CheckState)settingFile.value(settingSelectStr+AUTOSTARTMAPPING_CHECKED).toInt(&ok);
        if (!ok || autoStartMappingCheckState < Qt::Unchecked || autoStartMappingCheckState > Qt::Checked) {
            autoStartMappingCheckState = Qt::Unchecked;
        }
        ui->autoStartMappingCheckBox->setCheckState(autoStartMappingCheckState);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "AutoStartMappingCheckState =" << autoStartMappingCheckState;
#endif
    }
    else {
        ui->autoStartMappingCheckBox->setCheckState(Qt::Unchecked);
    }

    if (true == settingFile.contains(settingSelectStr+SENDTOSAMEWINDOWS_CHECKED)){
        bool sendToSameWindowsChecked = settingFile.value(settingSelectStr+SENDTOSAMEWINDOWS_CHECKED).toBool();
        if (true == sendToSameWindowsChecked) {
            ui->sendToSameTitleWindowsCheckBox->setChecked(true);
        }
        else {
            ui->sendToSameTitleWindowsCheckBox->setChecked(false);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "SendToSameWindowsChecked =" << sendToSameWindowsChecked;
#endif
    }
    else {
        ui->sendToSameTitleWindowsCheckBox->setChecked(false);
    }

    if (ui->autoStartMappingCheckBox->isChecked()) {
        ui->sendToSameTitleWindowsCheckBox->setEnabled(false);
    }

    if (true == settingFile.contains(settingSelectStr+ACCEPTVIRTUALGAMEPADINPUT_CHECKED)){
        bool acceptVirtualGamepadInputChecked = settingFile.value(settingSelectStr+ACCEPTVIRTUALGAMEPADINPUT_CHECKED).toBool();
        if (true == acceptVirtualGamepadInputChecked) {
            ui->acceptVirtualGamepadInputCheckBox->setChecked(true);
        }
        else {
            ui->acceptVirtualGamepadInputCheckBox->setChecked(false);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "AcceptVirtualGamdpadInputChecked =" << acceptVirtualGamepadInputChecked;
#endif
    }
    else {
        ui->acceptVirtualGamepadInputCheckBox->setChecked(false);
    }

    if (true == settingFile.contains(settingSelectStr+PROCESSICON_AS_TRAYICON_CHECKED)){
        bool processIconAsTrayIconChecked = settingFile.value(settingSelectStr+PROCESSICON_AS_TRAYICON_CHECKED).toBool();
        if (true == processIconAsTrayIconChecked) {
            ui->ProcessIconAsTrayIconCheckBox->setChecked(true);
        }
        else {
            ui->ProcessIconAsTrayIconCheckBox->setChecked(false);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "ProcessIconAsTrayIconChecked =" << processIconAsTrayIconChecked;
#endif
    }
    else {
        ui->ProcessIconAsTrayIconCheckBox->setChecked(false);
    }

//     QString loadedmappingswitchKeySeqStr;
//     if (true == settingFile.contains(settingSelectStr+MAPPINGSTART_KEY)){
//         loadedmappingswitchKeySeqStr = settingFile.value(settingSelectStr+MAPPINGSTART_KEY).toString();
//         if (loadedmappingswitchKeySeqStr.isEmpty()) {
//             loadedmappingswitchKeySeqStr = m_mappingswitchKeySeqEdit->defaultKeySequence();
//         }
//     }
//     else {
//         loadedmappingswitchKeySeqStr = m_mappingswitchKeySeqEdit->defaultKeySequence();
//     }
//     m_mappingswitchKeySeqEdit->setKeySequence(QKeySequence(loadedmappingswitchKeySeqStr));
//     updateMappingSwitchKeySeq(m_mappingswitchKeySeqEdit->keySequence());
// #ifdef DEBUG_LOGOUT_ON
//     qDebug().nospace().noquote() << "[loadKeyMapSetting]" << " Load & Set Mapping Switch KeySequence [" << settingSelectStr+MAPPINGSTART_KEY << "] -> \"" << m_mappingswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText) << "\"";
// #endif

    QString loadedmappingStartKeyStr;
    if (true == settingFile.contains(settingSelectStr+MAPPINGSTART_KEY)){
        loadedmappingStartKeyStr = settingFile.value(settingSelectStr+MAPPINGSTART_KEY).toString();
        if (loadedmappingStartKeyStr.isEmpty()) {
            loadedmappingStartKeyStr = MAPPINGSWITCH_KEY_DEFAULT;
        }
    }
    else {
        loadedmappingStartKeyStr = MAPPINGSWITCH_KEY_DEFAULT;
    }
    updateMappingStartKeyString(loadedmappingStartKeyStr);
    ui->mappingStartKeyLineEdit->setText(s_MappingStartKeyString);
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[loadKeyMapSetting]" << " Load & Set Mapping Start Key [" << settingSelectStr+MAPPINGSTART_KEY << "] -> \"" << s_MappingStartKeyString << "\"";
#endif

    QString loadedmappingStopKeyStr;
    if (true == settingFile.contains(settingSelectStr+MAPPINGSTOP_KEY)){
        loadedmappingStopKeyStr = settingFile.value(settingSelectStr+MAPPINGSTOP_KEY).toString();
        if (loadedmappingStopKeyStr.isEmpty()) {
            loadedmappingStopKeyStr = MAPPINGSWITCH_KEY_DEFAULT;
        }
    }
    else {
        loadedmappingStopKeyStr = MAPPINGSWITCH_KEY_DEFAULT;
    }
    updateMappingStopKeyString(loadedmappingStopKeyStr);
    ui->mappingStopKeyLineEdit->setText(s_MappingStopKeyString);
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[loadKeyMapSetting]" << " Load & Set Mapping Stop Key [" << settingSelectStr+MAPPINGSTOP_KEY << "] -> \"" << s_MappingStopKeyString << "\"";
#endif

    QString loadedSettingString;
    if (false == datavalidflag){
        showFailurePopup(tr("Invalid mapping data : ") + settingtext);
        return loadedSettingString;
    }
    else {
        ui->settingNameLineEdit->setCursorPosition(ui->settingNameLineEdit->text().length());
        ui->processLineEdit->setCursorPosition(ui->processLineEdit->text().length());
        ui->windowTitleLineEdit->setCursorPosition(ui->windowTitleLineEdit->text().length());

        if (!settingSelectStr.isEmpty()) {
            settingSelectStr = settingSelectStr.remove("/");
            int settingSelectIndex = 0;
            if (m_SettingSelectListWithoutDescription.contains(settingSelectStr)) {
                settingSelectIndex = m_SettingSelectListWithoutDescription.indexOf(settingSelectStr);
                if (settingSelectIndex < 0) {
                    settingSelectIndex = 0;
                }
            }
            ui->settingselectComboBox->setCurrentIndex(settingSelectIndex);
            // ui->settingselectComboBox->setCurrentText(settingSelectStr);
            loadedSettingString = settingSelectStr;
            ui->settingselectComboBox->setToolTip(ui->settingselectComboBox->currentText());
        }

        if (ui->startupAutoMonitoringCheckBox->isChecked() && settingtext.isEmpty()) {
            MappingSwitch(MAPPINGSTART_LOADSETTING);
        }
        return loadedSettingString;
    }
}

void QKeyMapper::loadFontFile(const QString fontfilename, int &returnback_fontid, QString &fontname)
{
    returnback_fontid = -1;
    fontname.clear();
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
    ui->deleteSelectedButton->setFont(customFont);
    ui->clearallButton->setFont(customFont);
    ui->addmapdataButton->setFont(customFont);
    ui->processCheckBox->setFont(customFont);
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
    m_KeyMappingDataTable->horizontalHeader()->setFont(customFont);

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

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(11);
    // }
    // else {
    //     customFont.setPointSize(9);
    // }
    // ui->processinfoTable->horizontalHeader()->setFont(customFont);
    // m_KeyMappingDataTable->horizontalHeader()->setFont(customFont);

    customFont.setPointSize(9);
    ui->addTabButton->setFont(customFont);
    ui->deleteSelectedButton->setFont(customFont);
    ui->clearallButton->setFont(customFont);
    ui->processListButton->setFont(customFont);
    ui->showNotesButton->setFont(customFont);
    ui->showCategoryButton->setFont(customFont);
    // ui->processCheckBox->setFont(customFont);
    // ui->titleCheckBox->setFont(customFont);
    ui->processLabel->setFont(customFont);
    ui->windowTitleLabel->setFont(customFont);
    ui->restoreProcessPathButton->setFont(customFont);
    ui->settingNameLabel->setFont(customFont);
    ui->descriptionLabel->setFont(customFont);
    ui->orikeyLabel->setFont(customFont);
    ui->orikeySeqLabel->setFont(customFont);
    ui->mapkeyLabel->setFont(customFont);
    // ui->burstpressLabel->setFont(customFont);
    // ui->burstpress_msLabel->setFont(customFont);
    // ui->burstreleaseLabel->setFont(customFont);
    // ui->burstrelease_msLabel->setFont(customFont);
    // ui->settingselectLabel->setFont(customFont);
    ui->removeSettingButton->setFont(customFont);
    ui->nextarrowCheckBox->setFont(customFont);
    ui->waitTimeLabel->setFont(customFont);
    ui->pushLevelLabel->setFont(customFont);
    ui->sendTextLabel->setFont(customFont);
    ui->keyPressTypeComboBox->setFont(customFont);
    ui->pointLabel->setFont(customFont);
    // ui->pointDisplayLabel->setFont(customFont);
    // ui->waitTime_msLabel->setFont(customFont);
    ui->mouseXSpeedLabel->setFont(customFont);
    ui->mouseYSpeedLabel->setFont(customFont);
    ui->keyboardSelectLabel->setFont(customFont);
    ui->mouseSelectLabel->setFont(customFont);
    ui->gamepadSelectLabel->setFont(customFont);

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(10);
    // }
    // else {
    //     customFont.setPointSize(8);
    // }
    ui->settingTabWidget->setFont(customFont);
    // ui->settingTabWidget->tabBar()->setFont(customFont);
    ui->windowswitchkeyLabel->setFont(customFont);
    ui->checkUpdateButton->setFont(customFont);
    ui->mappingStartKeyLabel->setFont(customFont);
    ui->mappingStopKeyLabel->setFont(customFont);
    ui->installViGEmBusButton->setFont(customFont);
    // ui->uninstallViGEmBusButton->setFont(customFont);
    ui->enableVirtualJoystickCheckBox->setFont(customFont);
    ui->lockCursorCheckBox->setFont(customFont);
    ui->directModeCheckBox->setFont(customFont);
    ui->ViGEmBusStatusLabel->setFont(customFont);
    ui->vJoyXSensLabel->setFont(customFont);
    ui->vJoyYSensLabel->setFont(customFont);
    ui->vJoyRecenterLabel->setFont(customFont);
    // ui->virtualgamepadGroupBox->setFont(customFont);
    // ui->multiInputGroupBox->setFont(customFont);
    ui->installInterceptionButton->setFont(customFont);
    ui->multiInputDeviceListButton->setFont(customFont);
    ui->multiInputStatusLabel->setFont(customFont);
    ui->multiInputEnableCheckBox->setFont(customFont);
    ui->filterKeysCheckBox->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(9);
    }
    else {
        customFont.setPointSize(9);
    }
    ui->autoStartMappingCheckBox->setFont(customFont);
    ui->sendToSameTitleWindowsCheckBox->setFont(customFont);
    ui->acceptVirtualGamepadInputCheckBox->setFont(customFont);
    ui->autoStartupCheckBox->setFont(customFont);
    ui->startupMinimizedCheckBox->setFont(customFont);
    ui->startupAutoMonitoringCheckBox->setFont(customFont);
    ui->soundEffectCheckBox->setFont(customFont);
    ui->notificationLabel->setFont(customFont);
    ui->languageLabel->setFont(customFont);
    ui->updateSiteLabel->setFont(customFont);
    ui->selectTrayIconButton->setFont(customFont);
    ui->ProcessIconAsTrayIconCheckBox->setFont(customFont);

    ui->Gyro2MouseXSpeedLabel->setFont(customFont);
    ui->Gyro2MouseYSpeedLabel->setFont(customFont);
    ui->Gyro2MouseMinXSensLabel->setFont(customFont);
    ui->Gyro2MouseMinYSensLabel->setFont(customFont);
    ui->Gyro2MouseMaxXSensLabel->setFont(customFont);
    ui->Gyro2MouseMaxYSensLabel->setFont(customFont);
    ui->Gyro2MouseMinThresholdLabel->setFont(customFont);
    ui->Gyro2MouseMaxThresholdLabel->setFont(customFont);
    ui->Gyro2MouseAdvancedSettingButton->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(9);
    }
    else {
        customFont.setPointSize(9);
    }
    // ui->disableWinKeyCheckBox->setFont(customFont);
    ui->dataPortLabel->setFont(customFont);
    ui->brakeThresholdLabel->setFont(customFont);
    ui->accelThresholdLabel->setFont(customFont);
}

void QKeyMapper::setControlFontChinese()
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

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(11);
    // }
    // else {
    //     customFont.setPointSize(9);
    // }
    // ui->processinfoTable->horizontalHeader()->setFont(customFont);
    // m_KeyMappingDataTable->horizontalHeader()->setFont(customFont);

    customFont.setPointSize(9);
    ui->addTabButton->setFont(customFont);
    ui->deleteSelectedButton->setFont(customFont);
    ui->clearallButton->setFont(customFont);
    ui->processListButton->setFont(customFont);
    ui->showNotesButton->setFont(customFont);
    ui->showCategoryButton->setFont(customFont);
    // ui->processCheckBox->setFont(customFont);
    // ui->titleCheckBox->setFont(customFont);
    ui->processLabel->setFont(customFont);
    ui->windowTitleLabel->setFont(customFont);
    ui->restoreProcessPathButton->setFont(customFont);
    ui->settingNameLabel->setFont(customFont);
    ui->descriptionLabel->setFont(customFont);
    ui->orikeyLabel->setFont(customFont);
    ui->orikeySeqLabel->setFont(customFont);
    ui->mapkeyLabel->setFont(customFont);
    // ui->burstpressLabel->setFont(customFont);
    // ui->burstpress_msLabel->setFont(customFont);
    // ui->burstreleaseLabel->setFont(customFont);
    // ui->burstrelease_msLabel->setFont(customFont);
    // ui->settingselectLabel->setFont(customFont);
    ui->removeSettingButton->setFont(customFont);
    ui->nextarrowCheckBox->setFont(customFont);
    ui->waitTimeLabel->setFont(customFont);
    ui->pushLevelLabel->setFont(customFont);
    ui->sendTextLabel->setFont(customFont);
    ui->keyPressTypeComboBox->setFont(customFont);
    ui->pointLabel->setFont(customFont);
    // ui->pointDisplayLabel->setFont(customFont);
    // ui->waitTime_msLabel->setFont(customFont);
    ui->mouseXSpeedLabel->setFont(customFont);
    ui->mouseYSpeedLabel->setFont(customFont);
    ui->keyboardSelectLabel->setFont(customFont);
    ui->mouseSelectLabel->setFont(customFont);
    ui->gamepadSelectLabel->setFont(customFont);

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(10);
    // }
    // else {
    //     customFont.setPointSize(8);
    // }
    ui->settingTabWidget->setFont(customFont);
    // ui->settingTabWidget->tabBar()->setFont(customFont);
    ui->windowswitchkeyLabel->setFont(customFont);
    ui->checkUpdateButton->setFont(customFont);
    ui->mappingStartKeyLabel->setFont(customFont);
    ui->mappingStopKeyLabel->setFont(customFont);
    ui->installViGEmBusButton->setFont(customFont);
    // ui->uninstallViGEmBusButton->setFont(customFont);
    ui->enableVirtualJoystickCheckBox->setFont(customFont);
    ui->lockCursorCheckBox->setFont(customFont);
    ui->directModeCheckBox->setFont(customFont);
    ui->ViGEmBusStatusLabel->setFont(customFont);
    ui->vJoyXSensLabel->setFont(customFont);
    ui->vJoyYSensLabel->setFont(customFont);
    ui->vJoyRecenterLabel->setFont(customFont);
    // ui->virtualgamepadGroupBox->setFont(customFont);
    // ui->multiInputGroupBox->setFont(customFont);
    ui->installInterceptionButton->setFont(customFont);
    ui->multiInputDeviceListButton->setFont(customFont);
    ui->multiInputStatusLabel->setFont(customFont);
    ui->multiInputEnableCheckBox->setFont(customFont);
    ui->filterKeysCheckBox->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(9);
    }
    else {
        customFont.setPointSize(9);
    }
    ui->autoStartMappingCheckBox->setFont(customFont);
    ui->sendToSameTitleWindowsCheckBox->setFont(customFont);
    ui->acceptVirtualGamepadInputCheckBox->setFont(customFont);
    ui->autoStartupCheckBox->setFont(customFont);
    ui->startupMinimizedCheckBox->setFont(customFont);
    ui->startupAutoMonitoringCheckBox->setFont(customFont);
    ui->soundEffectCheckBox->setFont(customFont);
    ui->notificationLabel->setFont(customFont);
    ui->languageLabel->setFont(customFont);
    ui->updateSiteLabel->setFont(customFont);
    ui->selectTrayIconButton->setFont(customFont);
    ui->ProcessIconAsTrayIconCheckBox->setFont(customFont);

    ui->Gyro2MouseXSpeedLabel->setFont(customFont);
    ui->Gyro2MouseYSpeedLabel->setFont(customFont);
    ui->Gyro2MouseMinXSensLabel->setFont(customFont);
    ui->Gyro2MouseMinYSensLabel->setFont(customFont);
    ui->Gyro2MouseMaxXSensLabel->setFont(customFont);
    ui->Gyro2MouseMaxYSensLabel->setFont(customFont);
    ui->Gyro2MouseMinThresholdLabel->setFont(customFont);
    ui->Gyro2MouseMaxThresholdLabel->setFont(customFont);
    ui->Gyro2MouseAdvancedSettingButton->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(9);
    }
    else {
        customFont.setPointSize(9);
    }
    // ui->disableWinKeyCheckBox->setFont(customFont);
    ui->dataPortLabel->setFont(customFont);
    ui->brakeThresholdLabel->setFont(customFont);
    ui->accelThresholdLabel->setFont(customFont);
}

void QKeyMapper::setControlFontJapanese()
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

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(11);
    // }
    // else {
    //     customFont.setPointSize(9);
    // }
    // ui->processinfoTable->horizontalHeader()->setFont(customFont);
    // m_KeyMappingDataTable->horizontalHeader()->setFont(customFont);

    customFont.setPointSize(9);
    ui->addTabButton->setFont(customFont);
    ui->deleteSelectedButton->setFont(customFont);
    ui->clearallButton->setFont(customFont);
    ui->processListButton->setFont(customFont);
    ui->showNotesButton->setFont(customFont);
    ui->showCategoryButton->setFont(customFont);
    // ui->processCheckBox->setFont(customFont);
    // ui->titleCheckBox->setFont(customFont);
    ui->processLabel->setFont(customFont);
    ui->windowTitleLabel->setFont(customFont);
    ui->restoreProcessPathButton->setFont(customFont);
    ui->settingNameLabel->setFont(customFont);
    ui->descriptionLabel->setFont(customFont);
    ui->orikeyLabel->setFont(customFont);
    ui->orikeySeqLabel->setFont(customFont);
    ui->mapkeyLabel->setFont(customFont);
    // ui->burstpressLabel->setFont(customFont);
    // ui->burstpress_msLabel->setFont(customFont);
    // ui->burstreleaseLabel->setFont(customFont);
    // ui->burstrelease_msLabel->setFont(customFont);
    // ui->settingselectLabel->setFont(customFont);
    ui->removeSettingButton->setFont(customFont);
    ui->nextarrowCheckBox->setFont(customFont);
    ui->waitTimeLabel->setFont(customFont);
    ui->pushLevelLabel->setFont(customFont);
    ui->sendTextLabel->setFont(customFont);
    ui->keyPressTypeComboBox->setFont(customFont);
    ui->pointLabel->setFont(customFont);
    // ui->pointDisplayLabel->setFont(customFont);
    // ui->waitTime_msLabel->setFont(customFont);
    ui->mouseXSpeedLabel->setFont(customFont);
    ui->mouseYSpeedLabel->setFont(customFont);
    ui->keyboardSelectLabel->setFont(customFont);
    ui->mouseSelectLabel->setFont(customFont);
    ui->gamepadSelectLabel->setFont(customFont);

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(10);
    // }
    // else {
    //     customFont.setPointSize(8);
    // }
    ui->settingTabWidget->setFont(customFont);
    // ui->settingTabWidget->tabBar()->setFont(customFont);
    ui->windowswitchkeyLabel->setFont(customFont);
    ui->checkUpdateButton->setFont(customFont);
    ui->mappingStartKeyLabel->setFont(customFont);
    ui->mappingStopKeyLabel->setFont(customFont);
    ui->installViGEmBusButton->setFont(customFont);
    // ui->uninstallViGEmBusButton->setFont(customFont);
    ui->enableVirtualJoystickCheckBox->setFont(customFont);
    ui->lockCursorCheckBox->setFont(customFont);
    ui->directModeCheckBox->setFont(customFont);
    ui->ViGEmBusStatusLabel->setFont(customFont);
    ui->vJoyXSensLabel->setFont(customFont);
    ui->vJoyYSensLabel->setFont(customFont);
    ui->vJoyRecenterLabel->setFont(customFont);
    // ui->virtualgamepadGroupBox->setFont(customFont);
    // ui->multiInputGroupBox->setFont(customFont);
    ui->installInterceptionButton->setFont(customFont);
    ui->multiInputDeviceListButton->setFont(customFont);
    ui->multiInputStatusLabel->setFont(customFont);
    ui->multiInputEnableCheckBox->setFont(customFont);
    ui->filterKeysCheckBox->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(9);
    }
    else {
        customFont.setPointSize(9);
    }
    ui->autoStartMappingCheckBox->setFont(customFont);
    ui->sendToSameTitleWindowsCheckBox->setFont(customFont);
    ui->acceptVirtualGamepadInputCheckBox->setFont(customFont);
    ui->autoStartupCheckBox->setFont(customFont);
    ui->startupMinimizedCheckBox->setFont(customFont);
    ui->startupAutoMonitoringCheckBox->setFont(customFont);
    ui->soundEffectCheckBox->setFont(customFont);
    ui->notificationLabel->setFont(customFont);
    ui->languageLabel->setFont(customFont);
    ui->updateSiteLabel->setFont(customFont);
    ui->selectTrayIconButton->setFont(customFont);
    ui->ProcessIconAsTrayIconCheckBox->setFont(customFont);

    ui->Gyro2MouseXSpeedLabel->setFont(customFont);
    ui->Gyro2MouseYSpeedLabel->setFont(customFont);
    ui->Gyro2MouseMinXSensLabel->setFont(customFont);
    ui->Gyro2MouseMinYSensLabel->setFont(customFont);
    ui->Gyro2MouseMaxXSensLabel->setFont(customFont);
    ui->Gyro2MouseMaxYSensLabel->setFont(customFont);
    ui->Gyro2MouseMinThresholdLabel->setFont(customFont);
    ui->Gyro2MouseMaxThresholdLabel->setFont(customFont);
    ui->Gyro2MouseAdvancedSettingButton->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(9);
    }
    else {
        customFont.setPointSize(9);
    }
    // ui->disableWinKeyCheckBox->setFont(customFont);
    ui->dataPortLabel->setFont(customFont);
    ui->brakeThresholdLabel->setFont(customFont);
    ui->accelThresholdLabel->setFont(customFont);
}

void QKeyMapper::changeControlEnableStatus(bool status)
{
    // if (true == status && GLOBALSETTING_INDEX == ui->settingselectComboBox->currentIndex()) {
    //     ui->processCheckBox->setEnabled(false);
    //     ui->titleCheckBox->setEnabled(false);
    //     ui->processLabel->setEnabled(false);
    //     ui->windowTitleLabel->setEnabled(false);
    //     ui->removeSettingButton->setEnabled(false);
    //     // ui->disableWinKeyCheckBox->setEnabled(false);
    //     ui->sendToSameTitleWindowsCheckBox->setEnabled(false);
    //     ui->ProcessIconAsTrayIconCheckBox->setEnabled(false);
    // }
    // else {
    //     ui->processCheckBox->setEnabled(status);
    //     ui->titleCheckBox->setEnabled(status);
    //     ui->processLabel->setEnabled(status);
    //     ui->windowTitleLabel->setEnabled(status);
    //     ui->removeSettingButton->setEnabled(status);
    //     // ui->disableWinKeyCheckBox->setEnabled(status);
    //     if (ui->autoStartMappingCheckBox->isChecked()) {
    //         ui->sendToSameTitleWindowsCheckBox->setEnabled(false);
    //     }
    //     else {
    //         ui->sendToSameTitleWindowsCheckBox->setEnabled(status);
    //     }
    //     ui->ProcessIconAsTrayIconCheckBox->setEnabled(status);
    // }

    //ui->processLineEdit->setEnabled(status);
    //ui->windowTitleLineEdit->setEnabled(status);
    // ui->processLineEdit->setReadOnly(!status);
    // ui->windowTitleLineEdit->setReadOnly(!status);
    // ui->descriptionLabel->setEnabled(status);
    // ui->descriptionLineEdit->setEnabled(status);
    ui->settingNameLabel->setEnabled(status);
    ui->settingNameLineEdit->setEnabled(status);
    ui->acceptVirtualGamepadInputCheckBox->setEnabled(status);
    ui->autoStartupCheckBox->setEnabled(status);
    ui->startupMinimizedCheckBox->setEnabled(status);
    ui->startupAutoMonitoringCheckBox->setEnabled(status);
    ui->soundEffectCheckBox->setEnabled(status);
    ui->notificationLabel->setEnabled(status);
    ui->notificationComboBox->setEnabled(status);
    ui->languageLabel->setEnabled(status);
    ui->languageComboBox->setEnabled(status);
    ui->updateSiteLabel->setEnabled(status);
    ui->updateSiteComboBox->setEnabled(status);
    ui->virtualGamepadTypeComboBox->setEnabled(status);
    ui->keyPressTypeComboBox->setEnabled(status);
    // ui->burstpressSpinBox->setEnabled(status);
    // ui->burstreleaseSpinBox->setEnabled(status);
    ui->settingselectComboBox->setEnabled(status);
    // ui->settingselectLabel->setEnabled(status);
    // ui->burstpressLabel->setEnabled(status);
    // ui->burstpress_msLabel->setEnabled(status);
    // ui->burstreleaseLabel->setEnabled(status);
    // ui->burstrelease_msLabel->setEnabled(status);
    ui->waitTimeLabel->setEnabled(status);
    ui->pushLevelLabel->setEnabled(status);
    ui->sendTextLabel->setEnabled(status);
    ui->pointLabel->setEnabled(status);
    // ui->waitTime_msLabel->setEnabled(status);
    ui->waitTimeSpinBox->setEnabled(status);
    ui->pushLevelSlider->setEnabled(status);
    ui->pushLevelSpinBox->setEnabled(status);
    ui->pressTimeSpinBox->setEnabled(status);
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

    ui->oriList_SelectKeyboardButton->setEnabled(status);
    ui->oriList_SelectMouseButton->setEnabled(status);
    ui->oriList_SelectGamepadButton->setEnabled(status);
    ui->oriList_SelectFunctionButton->setEnabled(status);
    ui->mapList_SelectKeyboardButton->setEnabled(status);
    ui->mapList_SelectMouseButton->setEnabled(status);
    ui->mapList_SelectGamepadButton->setEnabled(status);
    ui->mapList_SelectFunctionButton->setEnabled(status);
    ui->orikeyLabel->setEnabled(status);
    ui->orikeySeqLabel->setEnabled(status);
    // m_originalKeySeqEdit->setEnabled(status);
    ui->combinationKeyLineEdit->setEnabled(status);
    ui->sendTextLineEdit->setEnabled(status);
    ui->mapkeyLabel->setEnabled(status);
    m_orikeyComboBox->setEnabled(status);

#ifdef VIGEM_CLIENT_SUPPORT
    if (true == status
        && (m_orikeyComboBox->currentText() == VJOY_MOUSE2LS_STR
            || m_orikeyComboBox->currentText() == VJOY_MOUSE2RS_STR
            || m_orikeyComboBox->currentText() == JOY_LS2MOUSE_STR
            || m_orikeyComboBox->currentText() == JOY_RS2MOUSE_STR
            || m_orikeyComboBox->currentText() == JOY_GYRO2MOUSE_STR
            || m_orikeyComboBox->currentText() == JOY_LS2VJOYLS_STR
            || m_orikeyComboBox->currentText() == JOY_RS2VJOYRS_STR
            || m_orikeyComboBox->currentText() == JOY_LS2VJOYRS_STR
            || m_orikeyComboBox->currentText() == JOY_RS2VJOYLS_STR
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
    ui->addTabButton->setEnabled(status);
    ui->deleteSelectedButton->setEnabled(status);
    ui->clearallButton->setEnabled(status);
    ui->processListButton->setEnabled(status);
    ui->showNotesButton->setEnabled(status);
    ui->showCategoryButton->setEnabled(status);
    ui->CategoryFilterComboBox->setEnabled(status);

    ui->settingTabWidget->setEnabled(status);

#ifdef VIGEM_CLIENT_SUPPORT
    if (false == status) {
        ui->enableVirtualJoystickCheckBox->setEnabled(status);
    }
    ui->installViGEmBusButton->setEnabled(status);
    // ui->uninstallViGEmBusButton->setEnabled(status);
    // ui->virtualgamepadGroupBox->setEnabled(status);

    if (false == status || ui->enableVirtualJoystickCheckBox->isChecked()) {
        ui->vJoyXSensLabel->setEnabled(status);
        ui->vJoyYSensLabel->setEnabled(status);
        ui->vJoyXSensSpinBox->setEnabled(status);
        ui->vJoyYSensSpinBox->setEnabled(status);
        ui->vJoyRecenterLabel->setEnabled(status);
        ui->vJoyRecenterSpinBox->setEnabled(status);
        ui->lockCursorCheckBox->setEnabled(status);
        ui->directModeCheckBox->setEnabled(status);
        ui->virtualGamepadNumberSpinBox->setEnabled(status);
        ui->virtualGamepadListComboBox->setEnabled(status);
    }
#endif

    ui->installInterceptionButton->setEnabled(status);
    // ui->multiInputGroupBox->setEnabled(status);
    if (false == status || Interception_Worker::INTERCEPTION_AVAILABLE == Interception_Worker::getInterceptionState()) {
        ui->multiInputEnableCheckBox->setEnabled(status);
        ui->multiInputDeviceListButton->setEnabled(status);
        ui->keyboardSelectLabel->setEnabled(status);
        ui->mouseSelectLabel->setEnabled(status);
        ui->keyboardSelectComboBox->setEnabled(status);
        ui->mouseSelectComboBox->setEnabled(status);
        if (status
            && Interception_Worker::INTERCEPTION_AVAILABLE == Interception_Worker::getInterceptionState()
            && ui->multiInputEnableCheckBox->isChecked()) {
            ui->filterKeysCheckBox->setEnabled(true);
        }
        else {
            ui->filterKeysCheckBox->setEnabled(false);
        }
    }

    ui->gamepadSelectLabel->setEnabled(status);
    ui->gamepadSelectComboBox->setEnabled(status);

    // ui->moveupButton->setEnabled(status);
    // ui->movedownButton->setEnabled(status);
    ui->nextarrowCheckBox->setEnabled(status);

    ui->windowswitchkeyLabel->setEnabled(status);
    ui->checkUpdateButton->setEnabled(status);
    // m_windowswitchKeySeqEdit->setEnabled(status);
    ui->windowswitchkeyLineEdit->setEnabled(status);
    ui->mappingStartKeyLabel->setEnabled(status);
    ui->mappingStopKeyLabel->setEnabled(status);
    // m_mappingswitchKeySeqEdit->setEnabled(status);
    ui->mappingStartKeyLineEdit->setEnabled(status);
    ui->mappingStopKeyLineEdit->setEnabled(status);

    // ui->refreshButton->setEnabled(status);
    ui->savemaplistButton->setEnabled(status);

    m_KeyMappingTabWidget->setEnabled(status);
    ui->processinfoTable->setEnabled(status);
    // m_KeyMappingDataTable->setEnabled(status);
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

void QKeyMapper::mappingStartNotification()
{
    ScopedTrayUpdater trayUpdater(this);

    QString popupNotification;
    int position = ui->notificationComboBox->currentIndex();
    if (NOTIFICATION_POSITION_NONE == position) {
        return;
    }
    QString currentSelectedSetting = ui->settingselectComboBox->currentText();
    int currentSelectedIndex = ui->settingselectComboBox->currentIndex();
    QString tabName = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabName;
    QColor tabFontColor = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabFontColor;
    QColor tabBGColor = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabBackgroundColor;
    int tabCustomImage_ShowPosition = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_ShowPosition;
    QString tabCustomImage_Path = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_Path;
    QString description = ui->descriptionLineEdit->text();
    QString notificationSetting;
    if (description.isEmpty()) {
        notificationSetting = currentSelectedSetting;
    }
    else {
        notificationSetting = description;
    }
    QString color_str;
    popupNotification = tr("StartMapping [") + notificationSetting + "]" + " - " + tabName;
    if (tabFontColor.isValid()) {
        color_str = tabFontColor.name();
    }
    else {
        if (GLOBALSETTING_INDEX == currentSelectedIndex) {
            color_str = NOTIFICATION_COLOR_GLOBAL_DEFAULT_STR;
        }
        else {
            QColor notification_font_color = m_NotificationSetupDialog->getNotification_FontColor();
            if (notification_font_color.isValid()) {
                color_str = notification_font_color.name();
            }
            else {
                color_str = NOTIFICATION_COLOR_NORMAL_DEFAULT_STR;
            }
        }
    }

    // Setup Notification Options
    PopupNotificationOptions opts;
    opts.color = color_str;
    opts.position = position;
    opts.size = m_NotificationSetupDialog->getNotification_FontSize();
    opts.displayDuration = m_NotificationSetupDialog->getNotification_DisplayDuration();
    if (tabBGColor.isValid()) {
        opts.backgroundColor = tabBGColor;
    }
    else {
        opts.backgroundColor = m_NotificationSetupDialog->getNotification_BackgroundColor();
    }
    opts.windowOpacity = m_NotificationSetupDialog->getNotification_Opacity();
    opts.padding = m_NotificationSetupDialog->getNotification_Padding();
    opts.borderRadius = m_NotificationSetupDialog->getNotification_BorderRadius();
    int font_weight = m_NotificationSetupDialog->getNotification_FontWeight();
    if (NOTIFICATION_FONT_WEIGHT_LIGHT == font_weight) {
        opts.fontWeight = QFont::Light;
    }
    else if (NOTIFICATION_FONT_WEIGHT_NORMAL == font_weight) {
        opts.fontWeight = QFont::Normal;
    }
    else {
        opts.fontWeight = QFont::Bold;
    }
    opts.fontItalic = m_NotificationSetupDialog->getNotification_FontIsItalic();
    opts.fadeInDuration = m_NotificationSetupDialog->getNotification_FadeInDuration();
    opts.fadeOutDuration = m_NotificationSetupDialog->getNotification_FadeOutDuration();
    opts.xOffset = m_NotificationSetupDialog->getNotification_X_Offset();
    opts.yOffset = m_NotificationSetupDialog->getNotification_Y_Offset();

    if (tabCustomImage_ShowPosition != TAB_CUSTOMIMAGE_SHOW_NONE
        && !tabCustomImage_Path.isEmpty()) {
        opts.iconPath = tabCustomImage_Path;
        opts.iconPosition = tabCustomImage_ShowPosition;
        opts.iconPadding = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_Padding;
    }

    bool tabCustomImage_ShowAsFloatingWindow = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_ShowAsFloatingWindow;
    if (tabCustomImage_ShowAsFloatingWindow
        && !tabCustomImage_Path.isEmpty()) {
        FloatingWindowOptions options;
        options.position = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_Position;
        options.referencePoint = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_ReferencePoint;
        options.size = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_Size;
        options.backgroundColor = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_BackgroundColor;
        options.windowOpacity = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_Opacity;
        options.mousePassThrough = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_MousePassThrough;
        if (!options.backgroundColor.isValid()) {
            options.backgroundColor = NOTIFICATION_BACKGROUND_COLOR_DEFAULT;
        }
        options.borderRadius = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_Radius;
        options.iconPath = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_Path;

        QKeyMapper::getInstance()->showFloatingIconWindow(options);
    }
    else {
        QKeyMapper::getInstance()->hideFloatingIconWindow();
    }

    // Show Notification Popup
    showNotificationPopup(popupNotification, opts);
}

void QKeyMapper::mappingStopNotification()
{
    ScopedTrayUpdater trayUpdater(this);

    QString popupNotification;
    QString mappingStatusString;
    if (KEYMAP_IDLE == m_KeyMapStatus) {
        mappingStatusString = tr("Idle");
    }
    else if (KEYMAP_CHECKING == m_KeyMapStatus) {
        mappingStatusString = tr("Monitoring");
    }

    if (mappingStatusString.isEmpty()) {
        return;
    }

    popupNotification = tr("StopMapping [") + mappingStatusString + "]";

    showNotificationPopup(popupNotification);

    QKeyMapper::getInstance()->hideFloatingIconWindow();
}

void QKeyMapper::mappingTabSwitchNotification(bool isSame)
{
    ScopedTrayUpdater trayUpdater(this);

    QString popupNotification;
    int position = ui->notificationComboBox->currentIndex();
    if (NOTIFICATION_POSITION_NONE == position) {
        return;
    }
    // QString currentSelectedSetting = ui->settingselectComboBox->currentText();
    int currentSelectedIndex = ui->settingselectComboBox->currentIndex();
    QString tabName = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabName;
    QColor tabFontColor = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabFontColor;
    QColor tabBGColor = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabBackgroundColor;
    int tabCustomImage_ShowPosition = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_ShowPosition;
    QString tabCustomImage_Path = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_Path;
    QString color_str;
    // popupNotification = tr("MappingTabSwitch [") + currentSelectedSetting + "]" + " - " + tabName;
    if (isSame) {
        popupNotification = tr("TabisAlready") + " - " + tabName;
    }
    else {
        popupNotification = tr("MappingTabSwitch") + " - " + tabName;
    }
    if (tabFontColor.isValid()) {
        color_str = tabFontColor.name();
    }
    else {
        if (GLOBALSETTING_INDEX == currentSelectedIndex) {
            color_str = NOTIFICATION_COLOR_GLOBAL_DEFAULT_STR;
        }
        else {
            QColor notification_font_color = m_NotificationSetupDialog->getNotification_FontColor();
            if (notification_font_color.isValid()) {
                color_str = notification_font_color.name();
            }
            else {
                color_str = NOTIFICATION_COLOR_NORMAL_DEFAULT_STR;
            }
        }
    }

    // Setup Notification Options
    PopupNotificationOptions opts;
    opts.color = color_str;
    opts.position = position;
    opts.size = m_NotificationSetupDialog->getNotification_FontSize();
    opts.displayDuration = m_NotificationSetupDialog->getNotification_DisplayDuration();
    if (tabBGColor.isValid()) {
        opts.backgroundColor = tabBGColor;
    }
    else {
        opts.backgroundColor = m_NotificationSetupDialog->getNotification_BackgroundColor();
    }
    opts.windowOpacity = m_NotificationSetupDialog->getNotification_Opacity();
    opts.padding = m_NotificationSetupDialog->getNotification_Padding();
    opts.borderRadius = m_NotificationSetupDialog->getNotification_BorderRadius();
    int font_weight = m_NotificationSetupDialog->getNotification_FontWeight();
    if (NOTIFICATION_FONT_WEIGHT_LIGHT == font_weight) {
        opts.fontWeight = QFont::Light;
    }
    else if (NOTIFICATION_FONT_WEIGHT_NORMAL == font_weight) {
        opts.fontWeight = QFont::Normal;
    }
    else {
        opts.fontWeight = QFont::Bold;
    }
    opts.fontItalic = m_NotificationSetupDialog->getNotification_FontIsItalic();
    opts.fadeInDuration = m_NotificationSetupDialog->getNotification_FadeInDuration();
    opts.fadeOutDuration = m_NotificationSetupDialog->getNotification_FadeOutDuration();
    opts.xOffset = m_NotificationSetupDialog->getNotification_X_Offset();
    opts.yOffset = m_NotificationSetupDialog->getNotification_Y_Offset();

    if (tabCustomImage_ShowPosition != TAB_CUSTOMIMAGE_SHOW_NONE
        && !tabCustomImage_Path.isEmpty()) {
        opts.iconPath = tabCustomImage_Path;
        opts.iconPosition = tabCustomImage_ShowPosition;
        opts.iconPadding = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_Padding;
    }

    bool tabCustomImage_ShowAsFloatingWindow = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_ShowAsFloatingWindow;
    if (tabCustomImage_ShowAsFloatingWindow
        && !tabCustomImage_Path.isEmpty()) {
        FloatingWindowOptions options;
        options.position = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_Position;
        options.referencePoint = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_ReferencePoint;
        options.size = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_Size;
        options.backgroundColor = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_BackgroundColor;
        options.windowOpacity = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_Opacity;
        options.mousePassThrough = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_MousePassThrough;
        if (!options.backgroundColor.isValid()) {
            options.backgroundColor = NOTIFICATION_BACKGROUND_COLOR_DEFAULT;
        }
        options.borderRadius = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).FloatingWindow_Radius;
        options.iconPath = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_Path;

        QKeyMapper::getInstance()->showFloatingIconWindow(options);
    }
    else {
        QKeyMapper::getInstance()->hideFloatingIconWindow();
    }

    // Show Notification Popup
    showNotificationPopup(popupNotification, opts);
}

void QKeyMapper::switchToWindowInfoTab()
{
    if (KEYMAP_IDLE == m_KeyMapStatus) {
        if (ui->settingTabWidget->currentWidget() != ui->windowinfo) {
            ui->settingTabWidget->setCurrentIndex(ui->settingTabWidget->indexOf(ui->windowinfo));
        }
    }
}

void QKeyMapper::closeSelectColorDialog()
{
    if (m_SelectColorDialog && m_SelectColorDialog->isVisible()) {
         m_SelectColorDialog->reject();
    }
}

void QKeyMapper::showInputDeviceListWindow()
{
    if (!m_deviceListWindow->isVisible()) {
        m_deviceListWindow->show();
    }
}

void QKeyMapper::closeInputDeviceListWindow()
{
    if (Q_NULLPTR == m_deviceListWindow) {
        return;
    }

    if (m_deviceListWindow->isVisible()) {
        m_deviceListWindow->close();
    }
}

void QKeyMapper::showGyro2MouseAdvancedSettingWindow()
{
    if (!m_Gyro2MouseOptionDialog->isVisible()) {
        m_Gyro2MouseOptionDialog->show();
    }
}

void QKeyMapper::closeGyro2MouseAdvancedSettingDialog()
{
    if (Q_NULLPTR == m_Gyro2MouseOptionDialog) {
        return;
    }

    if (m_Gyro2MouseOptionDialog->isVisible()) {
        m_Gyro2MouseOptionDialog->close();
    }
}

void QKeyMapper::showTrayIconSelectDialog()
{
    if (!m_TrayIconSelectDialog->isVisible()) {
        m_TrayIconSelectDialog->show();
    }
}

void QKeyMapper::closeTrayIconSelectDialog()
{
    if (Q_NULLPTR == m_TrayIconSelectDialog) {
        return;
    }

    if (m_TrayIconSelectDialog->isVisible()) {
        m_TrayIconSelectDialog->close();
    }
}

void QKeyMapper::showNotificationSetupDialog()
{
    if (!m_NotificationSetupDialog->isVisible()) {
        m_NotificationSetupDialog->show();
    }
}

void QKeyMapper::closeNotificationSetupDialog()
{
    if (Q_NULLPTR == m_NotificationSetupDialog) {
        return;
    }

    if (m_NotificationSetupDialog->isVisible()) {
        m_NotificationSetupDialog->close();
    }
}

void QKeyMapper::showItemSetupDialog(int tabindex, int row)
{
    if (Q_NULLPTR == m_ItemSetupDialog) {
        return;
    }

    if (!m_ItemSetupDialog->isVisible()) {
        // int y_offset = m_KeyMappingDataTable->rowViewportPosition(row);
        // int width = m_ItemSetupDialog->width();
        // int height = m_ItemSetupDialog->height();
        // QPoint globalPos = m_KeyMappingDataTable->mapToGlobal(QPoint(0, 0));
        // int top = globalPos.y() + y_offset + m_KeyMappingDataTable->horizontalHeader()->height();
        // int left = globalPos.x() - width - 5;
        // m_ItemSetupDialog->setGeometry(QRect(left, top, width, height));

        // QRect windowGeometry = this->geometry();
        // QRect tableGeometry = m_KeyMappingDataTable->geometry();
        // int x = windowGeometry.x() + (windowGeometry.width() - m_ItemSetupDialog->width()) / 2;
        // int y = windowGeometry.y() +  tableGeometry.y() + tableGeometry.height() + m_KeyMappingDataTable->horizontalHeader()->height();
        // m_ItemSetupDialog->move(x, y);

        QRect windowGeometry = this->geometry();
        int x = windowGeometry.x() + (windowGeometry.width() - m_ItemSetupDialog->width()) / 2;
        int y = windowGeometry.y() + (windowGeometry.height() - m_ItemSetupDialog->height()) / 2;
        int y_offset = 50;
        y += y_offset;
        m_ItemSetupDialog->move(x, y);

        m_ItemSetupDialog->setTabIndex(tabindex);
        m_ItemSetupDialog->setItemRow(row);
        m_ItemSetupDialog->show();
    }
}

void QKeyMapper::closeItemSetupDialog()
{
    if (Q_NULLPTR == m_ItemSetupDialog) {
        return;
    }

    if (m_ItemSetupDialog->isVisible()) {
        m_ItemSetupDialog->close();
    }
}

void QKeyMapper::closeCrosshairSetupDialog()
{
    if (Q_NULLPTR == QItemSetupDialog::getInstance()->m_CrosshairSetupDialog) {
        return;
    }

    if (QItemSetupDialog::getInstance()->m_CrosshairSetupDialog->isVisible()) {
        QItemSetupDialog::getInstance()->m_CrosshairSetupDialog->close();
    }
}

void QKeyMapper::closeFloatingWindowSetupDialog()
{
    if (Q_NULLPTR == QTableSetupDialog::getInstance()->m_FloatingWindowSetupDialog) {
        return;
    }

    if (QTableSetupDialog::getInstance()->m_FloatingWindowSetupDialog->isVisible()) {
        QTableSetupDialog::getInstance()->m_FloatingWindowSetupDialog->close();
    }
}

void QKeyMapper::showTableSetupDialog(int tabindex)
{
    if (Q_NULLPTR == m_TableSetupDialog) {
        return;
    }

    if (!m_TableSetupDialog->isVisible()) {
        QRect windowGeometry = this->geometry();
        int x = windowGeometry.x() + (windowGeometry.width() - m_TableSetupDialog->width()) / 2;
        int y = windowGeometry.y() + (windowGeometry.height() - m_TableSetupDialog->height()) / 2;
        int y_offset = -150;
        y += y_offset;
        m_TableSetupDialog->move(x, y);

        m_TableSetupDialog->setTabIndex(tabindex);
        m_TableSetupDialog->setSettingSelectIndex(ui->settingselectComboBox->currentIndex());
        m_TableSetupDialog->show();
    }
}

void QKeyMapper::closeTableSetupDialog()
{
    if (Q_NULLPTR == m_TableSetupDialog) {
        return;
    }

    if (m_TableSetupDialog->isVisible()) {
        m_TableSetupDialog->close();
    }
}

int QKeyMapper::installInterceptionDriver()
{
    QString operate_str = QString("runas");
    QString executable_str = QString("InterceptionDriver\\install-interception.exe");
    QString argument_str = QString("/install");

    std::wstring operate;
    std::wstring executable;
    std::wstring argument;
    HINSTANCE ret_instance;
    INT64 ret;

    /* Install Interception Driver */
    operate = operate_str.toStdWString();
    executable = executable_str.toStdWString();
    argument = argument_str.toStdWString();

    ret_instance = ShellExecute(Q_NULLPTR, operate.c_str(), executable.c_str(), argument.c_str(), Q_NULLPTR, SW_HIDE);
    ret = (INT64)ret_instance;
    if(ret > 32) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installInterceptionDriver] Install Interception Driver Success. ->" << ret;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installInterceptionDriver] Install Interception Driver Failed!!! ->" << ret;
#endif
        return -1;
    }

    return 0;
}

int QKeyMapper::uninstallInterceptionDriver()
{
    QString operate_str = QString("runas");
    QString executable_str = QString("InterceptionDriver\\install-interception.exe");
    QString argument_str = QString("/uninstall");

    std::wstring operate;
    std::wstring executable;
    std::wstring argument;
    HINSTANCE ret_instance;
    INT64 ret;

    /* Install Interception Driver */
    operate = operate_str.toStdWString();
    executable = executable_str.toStdWString();
    argument = argument_str.toStdWString();

    ret_instance = ShellExecute(Q_NULLPTR, operate.c_str(), executable.c_str(), argument.c_str(), Q_NULLPTR, SW_HIDE);
    ret = (INT64)ret_instance;
    if(ret > 32) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[uninstallInterceptionDriver] Uninstall Interception Driver Success. ->" << ret;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[uninstallInterceptionDriver] Uninstall Interception Driver Failed!!! ->" << ret;
#endif
        return -1;
    }

    return 0;
}

#ifdef VIGEM_CLIENT_SUPPORT
int QKeyMapper::installViGEmBusDriver()
{
    QString operate_str = QString("runas");
    QString remove_old_devnode1_argument_str = QString("--remove-device-node --hardware-id Nefarius\\ViGEmBus\\Gen1 --class-guid 4D36E97D-E325-11CE-BFC1-08002BE10318");
    QString remove_old_devnode2_argument_str = QString("--remove-device-node --hardware-id Root\\ViGEmBus --class-guid 4D36E97D-E325-11CE-BFC1-08002BE10318");
    QString create_devnode_argument_str = QString("--create-device-node --hardware-id Nefarius\\ViGEmBus\\Gen1 --class-name System --class-guid 4D36E97D-E325-11CE-BFC1-08002BE10318");
    QString executable_str;
    QString install_argument_str;

    if (QSysInfo::currentCpuArchitecture() == "x86_64")
    {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Running on a 64-bit operating system ->" << " Use ViGEmBusDriver x64 Driver Files.";
#endif
        executable_str = QString("ViGEmBusDriver\\x64\\nefconw.exe");
        install_argument_str = QString("--install-driver --inf-path \"ViGEmBusDriver\\x64\\ViGEmBus.inf\"");
    }
    else if (QSysInfo::currentCpuArchitecture() == "i386")
    {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Running on a 32-bit operating system ->" << " Use ViGEmBusDriver x86 Driver Files.";
#endif
        executable_str = QString("ViGEmBusDriver\\x86\\nefconw.exe");
        install_argument_str = QString("--install-driver --inf-path \"ViGEmBusDriver\\x86\\ViGEmBus.inf\"");
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[installViGEmBusDriver] Unknown operating system architecture!";
#endif
        return -1;
    }

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
    QString remove_devnode_argument_str = QString("--remove-device-node --hardware-id Nefarius\\ViGEmBus\\Gen1 --class-name System --class-guid 4D36E97D-E325-11CE-BFC1-08002BE10318");
    QString executable_str;
    QString uninstall_argument_str;

    if (QSysInfo::currentCpuArchitecture() == "x86_64")
    {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[uninstallViGEmBusDriver] Running on a 64-bit operating system ->" << " Use ViGEmBusDriver x64 Driver Files.";
#endif
        executable_str = QString("ViGEmBusDriver\\x64\\nefconw.exe");
        uninstall_argument_str = QString("--uninstall-driver --inf-path \"ViGEmBusDriver\\x64\\ViGEmBus.inf\"");
    }
    else if (QSysInfo::currentCpuArchitecture() == "i386")
    {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[uninstallViGEmBusDriver] Running on a 32-bit operating system ->" << " Use ViGEmBusDriver x86 Driver Files.";
#endif
        executable_str = QString("ViGEmBusDriver\\x86\\nefconw.exe");
        uninstall_argument_str = QString("--uninstall-driver --inf-path \"ViGEmBusDriver\\x86\\ViGEmBus.inf\"");
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[uninstallViGEmBusDriver] Unknown operating system architecture!";
#endif
        return -1;
    }

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

void QKeyMapper::updateViGEmBusStatus()
{
    QKeyMapper_Worker::ViGEmClient_ConnectState connectstate = QKeyMapper_Worker::ViGEmClient_getConnectState();

#ifdef DEBUG_LOGOUT_ON
    static QKeyMapper_Worker::ViGEmClient_ConnectState lastConnectState = QKeyMapper_Worker::VIGEMCLIENT_CONNECTING;
    if (lastConnectState != connectstate) {
        lastConnectState = connectstate;
        qDebug() << "[updateViGEmBusStatus]" << "ViGEmClient Connect State ->" << lastConnectState;
    }
#endif

    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == connectstate) {
        if (m_KeyMapStatus == KEYMAP_IDLE){
            ui->enableVirtualJoystickCheckBox->setEnabled(true);
        }
        // ui->installViGEmBusButton->setEnabled(false);
        ui->installViGEmBusButton->setText(tr("UninstallViGEm"));

        ui->ViGEmBusStatusLabel->setStyleSheet("color:green;");
        ui->ViGEmBusStatusLabel->setText(tr("ViGEmAvailable"));
    }
    else {
        ui->enableVirtualJoystickCheckBox->setEnabled(false);
        // ui->installViGEmBusButton->setEnabled(true);
        ui->installViGEmBusButton->setText(tr("InstallViGEm"));

        ui->ViGEmBusStatusLabel->setStyleSheet("color:red;");
        ui->ViGEmBusStatusLabel->setText(tr("ViGEmUnavailable"));
    }
}

void QKeyMapper::updateVirtualGamepadListDisplay()
{
    int gamepad_number = QKeyMapper_Worker::s_VirtualGamepadList.size();

    if (gamepad_number < VIRTUAL_GAMEPAD_NUMBER_MIN || gamepad_number > VIRTUAL_GAMEPAD_NUMBER_MAX) {
        return;
    }

    ui->virtualGamepadNumberSpinBox->setValue(gamepad_number);

    int lastIndex = ui->virtualGamepadListComboBox->currentIndex();
    ui->virtualGamepadListComboBox->clear();

    QStringList gamepadList;
    gamepadList.append(QString());
    int gamepad_index = 0;
    for (const QString &gamepad : std::as_const(QKeyMapper_Worker::s_VirtualGamepadList))
    {
        QString gamepad_Str;
        if (gamepad == VIRTUAL_GAMEPAD_DS4) {
            gamepad_Str = QString("[%1] %2").arg(QString::number(gamepad_index), VIRTUAL_GAMEPAD_DS4);
        }
        else {
            gamepad_Str = QString("[%1] %2").arg(QString::number(gamepad_index), VIRTUAL_GAMEPAD_X360);
        }
        gamepadList.append(gamepad_Str);
        gamepad_index++;
    }
    ui->virtualGamepadListComboBox->addItems(gamepadList);

    if (0 <= lastIndex && lastIndex < ui->virtualGamepadListComboBox->count()) {
        ui->virtualGamepadListComboBox->setCurrentIndex(lastIndex);
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

void QKeyMapper::updateMultiInputStatus()
{
    Interception_Worker::Interception_State interception_state = Interception_Worker::getInterceptionState();

#ifdef DEBUG_LOGOUT_ON
    static Interception_Worker::Interception_State lastInterceptionState = Interception_Worker::INTERCEPTION_INIT;
    if (lastInterceptionState != interception_state) {
        lastInterceptionState = interception_state;
        qDebug() << "[updateMultiInputStatus]" << "Interception State ->" << lastInterceptionState;
    }
#endif

    if (Interception_Worker::INTERCEPTION_AVAILABLE == interception_state) {
        if (m_KeyMapStatus == KEYMAP_IDLE){
            ui->multiInputEnableCheckBox->setEnabled(true);
            ui->multiInputDeviceListButton->setEnabled(true);
            ui->keyboardSelectLabel->setEnabled(true);
            ui->mouseSelectLabel->setEnabled(true);
            ui->keyboardSelectComboBox->setEnabled(true);
            ui->mouseSelectComboBox->setEnabled(true);
            if (ui->multiInputEnableCheckBox->isChecked()) {
                ui->filterKeysCheckBox->setEnabled(true);
            }
            else {
                ui->filterKeysCheckBox->setEnabled(false);
            }
        }

        ui->installInterceptionButton->setText(tr("Uninstall Driver"));

        ui->multiInputStatusLabel->setStyleSheet("color:green;");
        ui->multiInputStatusLabel->setText(tr("Available"));
    }
    else if (Interception_Worker::INTERCEPTION_REBOOTREQUIRED == interception_state) {
        ui->multiInputEnableCheckBox->setChecked(false);
        ui->multiInputEnableCheckBox->setEnabled(false);
        ui->multiInputDeviceListButton->setEnabled(false);
        ui->filterKeysCheckBox->setEnabled(false);
        ui->keyboardSelectLabel->setEnabled(false);
        ui->mouseSelectLabel->setEnabled(false);
        ui->keyboardSelectComboBox->setCurrentIndex(0);
        ui->mouseSelectComboBox->setCurrentIndex(0);
        ui->keyboardSelectComboBox->setEnabled(false);
        ui->mouseSelectComboBox->setEnabled(false);

        ui->installInterceptionButton->setText(tr("Install Driver"));

        ui->multiInputStatusLabel->setStyleSheet("color: orange;");
        ui->multiInputStatusLabel->setText(tr("RebootRequired"));
    }
    else {
        ui->multiInputEnableCheckBox->setChecked(false);
        ui->multiInputEnableCheckBox->setEnabled(false);
        ui->multiInputDeviceListButton->setEnabled(false);
        ui->filterKeysCheckBox->setEnabled(false);
        ui->keyboardSelectLabel->setEnabled(false);
        ui->mouseSelectLabel->setEnabled(false);
        ui->keyboardSelectComboBox->setCurrentIndex(0);
        ui->mouseSelectComboBox->setCurrentIndex(0);
        ui->keyboardSelectComboBox->setEnabled(false);
        ui->mouseSelectComboBox->setEnabled(false);

        ui->installInterceptionButton->setText(tr("Install Driver"));

        ui->multiInputStatusLabel->setStyleSheet("color: red;");
        ui->multiInputStatusLabel->setText(tr("Unavailable"));
    }
}

void QKeyMapper::updateInputDeviceSelectComboBoxes()
{
    initInputDeviceSelectComboBoxes();
}

void QKeyMapper::updateGamepadSelectComboBox(int instance_id)
{
    QMap<int, Gamepad_Info> GamepadInfoMap;
    QString changed_gamepad_string;

    QList<QJoystickDevice *> joysticklist = QJoysticks::getInstance()->inputDevices();

    for (const QJoystickDevice *joystick : std::as_const(joysticklist)) {
        int player_index = joystick->playerindex;
        if (JOYSTICK_PLAYER_INDEX_MIN <= player_index && player_index <= JOYSTICK_PLAYER_INDEX_MAX) {
            Gamepad_Info gamepadinfo;
            gamepadinfo.instance_id = joystick->instanceID;
            gamepadinfo.name = joystick->name;
            gamepadinfo.vendorid = joystick->vendorid;
            gamepadinfo.productid = joystick->productid;
            gamepadinfo.serial = joystick->serial;
            gamepadinfo.isvirtual = joystick->blacklisted;
            gamepadinfo.gyro_enabled = GAMEPADINFO_GYRO_NONE;
            gamepadinfo.info_string = QString("[%1] %2 [VID=0x%3][PID=0x%4]")
                .arg(player_index)
                .arg(joystick->name,
                     QString::number(joystick->vendorid, 16).toUpper().rightJustified(4, '0'),
                     QString::number(joystick->productid, 16).toUpper().rightJustified(4, '0'));
            if (joystick->has_gyro && joystick->blacklisted != true) {
                if (instance_id != JOYSTICK_INVALID_INSTANCE_ID) {
                    if (joystick->instanceID == instance_id) {
                        if (joystick->sensor_disabled) {
                            changed_gamepad_string = gamepadinfo.info_string + tr(" GyroDisabled");
                        }
                        else {
                            changed_gamepad_string = gamepadinfo.info_string + tr(" GyroEnabled");
                        }
                    }
                }
                if (joystick->sensor_disabled) {
                    gamepadinfo.gyro_enabled = GAMEPADINFO_GYRO_DISABLED;
                    gamepadinfo.info_string.append("[GyroDisabled]");
                }
                else {
                    gamepadinfo.gyro_enabled = GAMEPADINFO_GYRO_ENABLED;
                    gamepadinfo.info_string.append("[GyroEnabled]");
                }
            }
            if (gamepadinfo.isvirtual) {
                gamepadinfo.info_string.append("[ViGEM]");
            }

            GamepadInfoMap.insert(player_index, gamepadinfo);
        }
    }

    m_GamepadInfoMap = GamepadInfoMap;

    /* Save current hight row if combobox list is showing */
    int highlightedRow = -1;
    QAbstractItemView *view = ui->gamepadSelectComboBox->view();
    if (view->isVisible()) {
        QModelIndex highlightedIndex = view->currentIndex();
        highlightedRow = highlightedIndex.row();
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[updateGamepadSelectComboBox] Current highlight row =" << highlightedRow;
#endif
    }

    ui->gamepadSelectComboBox->clear();
    QStringList gamepadInfoList;
    gamepadInfoList.append(QString());

    QList<int> playerindexlist = GamepadInfoMap.keys();
    for (const int& playerindex : std::as_const(playerindexlist)) {
        const Gamepad_Info& gamepadinfo = m_GamepadInfoMap.value(playerindex);
        gamepadInfoList.append(gamepadinfo.info_string);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "PlayerIndex:" << playerindex << ", InfoString:" << gamepadinfo.info_string;
#endif
    }

    ui->gamepadSelectComboBox->addItems(gamepadInfoList);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->gamepadSelectComboBox->model());
    if (model) {
        for (int row = 0; row < model->rowCount(); ++row) {
            QStandardItem *item = model->item(row);
            if (item && item->text().endsWith("[ViGEM]")) {
                item->setData(QColor(Qt::darkMagenta), Qt::ForegroundRole);
            }
        }
    }
#else
    for (int row = 0; row < ui->gamepadSelectComboBox->count(); ++row) {
        if (ui->gamepadSelectComboBox->itemText(row).endsWith("[ViGEM]")) {
            ui->gamepadSelectComboBox->setItemData(row, QBrush(Qt::darkMagenta), Qt::TextColorRole);
        }
    }
#endif

    /* Restore current hight row */
    if (view->isVisible() && highlightedRow >= 0) {
        QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->gamepadSelectComboBox->model());
        if (model && highlightedRow < model->rowCount()) {
            view->setCurrentIndex(model->index(highlightedRow, 0));
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[updateGamepadSelectComboBox] Restore highlight row =" << highlightedRow;
#endif
        }
    }

    if (changed_gamepad_string.isEmpty() == false) {
        showInformationPopup(changed_gamepad_string);
    }
}

void QKeyMapper::updateKeyMappingTabWidgetTabName(int tabindex, const QString &tabname)
{
    if ((tabindex < 0) || (tabindex >= m_KeyMappingTabWidget->count()) || (tabindex >= s_KeyMappingTabInfoList.size())) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[updateKeyMappingTabWidgetTabName] Invalid index : " << tabindex << ", ValidTabWidgetCount:" << m_KeyMappingTabWidget->count() << ", TabInfoListSize:" << s_KeyMappingTabInfoList.size();
#endif
        return;
    }

    m_KeyMappingTabWidget->setTabText(tabindex, tabname);
    if (s_KeyMappingTabInfoList.at(tabindex).TabName != tabname) {
        s_KeyMappingTabInfoList[tabindex].TabName = tabname;
    }
}

void QKeyMapper::updateKeyMappingTabInfoHotkey(int tabindex, const QString &tabhotkey)
{
    if (tabindex < 0 || tabindex > s_KeyMappingTabInfoList.size() - 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[updateKeyMappingTabInfoHotkey] Invalid index : " << tabindex << ", TabInfoListSize:" << s_KeyMappingTabInfoList.size();
#endif
        return;
    }

    if (s_KeyMappingTabInfoList.at(tabindex).TabHotkey != tabhotkey) {
        s_KeyMappingTabInfoList[tabindex].TabHotkey = tabhotkey;
        updateKeyMappingTabWidgetTabDisplay(tabindex);
    }

    collectMappingTableTabHotkeys();
}

void QKeyMapper::on_savemaplistButton_clicked()
{
    saveKeyMapSetting();
}

#if 0
void QKeyMapper::initHotKeySequence()
{
    QObject::connect(m_HotKey_ShowHide, &QHotkey::activated, this, &QKeyMapper::HotKeyActivated);

    QObject::connect(m_HotKey_StartStop, &QHotkey::activated, this, &QKeyMapper::HotKeyStartStopActivated);
}
#endif

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

void QKeyMapper::initSysTrayIcon()
{
    m_SysTrayIcon = new QSystemTrayIcon(this);
    // m_SysTrayIcon->setIcon(QIcon(":/Blue1.ico"));
    // m_SysTrayIcon->setToolTip("QKeyMapper(" + tr("Idle") + ")");

    m_SysTrayIconMenu = new SystrayMenu(this);
    m_TrayIconMenu_ShowHideAction = new QAction(this);
    m_TrayIconMenu_QuitAction = new QAction(this);

    // When the Show/Hide menu item is clicked, toggle the visibility of the window
    connect(m_TrayIconMenu_ShowHideAction, &QAction::triggered, this, &QKeyMapper::onTrayIconMenuShowHideAction);
    // When the Quit menu item is clicked, close the application
    connect(m_TrayIconMenu_QuitAction, &QAction::triggered, this, &QKeyMapper::onTrayIconMenuQuitAction);

    updateSysTrayIconMenuText();

    m_SysTrayIconMenu->addAction(m_TrayIconMenu_ShowHideAction);
    m_SysTrayIconMenu->addSeparator(); // Add a separator horizontal line
    m_SysTrayIconMenu->addAction(m_TrayIconMenu_QuitAction);

    // Add the created menu to the system tray icon
    m_SysTrayIcon->setContextMenu(m_SysTrayIconMenu);

    // m_SysTrayIcon->show();
}

void QKeyMapper::initPopupMessage()
{
    m_PopupMessageLabel = new QLabel(this);
    m_PopupMessageLabel->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_PopupMessageLabel->setAttribute(Qt::WA_TranslucentBackground);
    m_PopupMessageLabel->setAttribute(Qt::WA_ShowWithoutActivating);
    m_PopupMessageLabel->setAlignment(Qt::AlignCenter);

    m_PopupMessageAnimation = new QPropertyAnimation(m_PopupMessageLabel, "windowOpacity", this);
    // QObject::connect(m_PopupMessageAnimation, &QPropertyAnimation::finished, m_PopupMessageLabel, &QLabel::hide);
    QObject::connect(m_PopupMessageAnimation, &QPropertyAnimation::finished, this, [this]() {
        m_PopupMessageLabel->hide();
        m_PopupMessageLabel->clear();
    });
}

void QKeyMapper::initPushLevelSlider()
{
    ui->pushLevelSlider->setRange(VJOY_PUSHLEVEL_MIN + 1, VJOY_PUSHLEVEL_MAX);
    ui->pushLevelSpinBox->setRange(VJOY_PUSHLEVEL_MIN + 1, VJOY_PUSHLEVEL_MAX);

    ui->pushLevelSlider->setValue(VJOY_PUSHLEVEL_MAX);
    ui->pushLevelSpinBox->setValue(VJOY_PUSHLEVEL_MAX);

    QObject::connect(ui->pushLevelSlider, &QSlider::valueChanged, ui->pushLevelSpinBox, &QSpinBox::setValue);
    QObject::connect(ui->pushLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), ui->pushLevelSlider, &QSlider::setValue);
}

void QKeyMapper::initWindowInfoMatchComboBoxes()
{
    QStringList windowinfoMatchList;
    windowinfoMatchList.append(tr("Ignore"));
    windowinfoMatchList.append(tr("Equals"));
    windowinfoMatchList.append(tr("Contains"));
    windowinfoMatchList.append(tr("StartsWith"));
    windowinfoMatchList.append(tr("EndsWith"));
    ui->checkProcessComboBox->addItems(windowinfoMatchList);
    ui->checkWindowTitleComboBox->addItems(windowinfoMatchList);
    ui->checkProcessComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
    ui->checkWindowTitleComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
}

void QKeyMapper::updateSysTrayIconMenuText()
{
    QString showActionText;
    QString hideActionText;
    QString quitActionText;

    showActionText = tr("Show");
    hideActionText = tr("Hide");
    quitActionText = tr("Quit");

    if (false == isHidden()) {
        m_TrayIconMenu_ShowHideAction->setText(hideActionText);
    }
    else {
        m_TrayIconMenu_ShowHideAction->setText(showActionText);
    }
    m_TrayIconMenu_QuitAction->setText(quitActionText);

    QFont customFont = m_TrayIconMenu_ShowHideAction->font();
    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale
        || UI_SCALE_4K_PERCENT_125 == m_UI_Scale
        || UI_SCALE_4K_PERCENT_100 == m_UI_Scale) {
    }
    else {
        customFont.setPointSize(9);
    }
    m_TrayIconMenu_ShowHideAction->setFont(customFont);
    m_TrayIconMenu_QuitAction->setFont(customFont);

#ifdef DEBUG_LOGOUT_ON
    QFont ShowHideActionFont = m_TrayIconMenu_ShowHideAction->font();
    QFont QuitActionFont = m_TrayIconMenu_QuitAction->font();
    qDebug() << "ShowHideActionFont ->" << ShowHideActionFont;
    qDebug() << "QuitActionFont ->" << QuitActionFont;
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
    ui->processLineEdit->setText(m_MapProcessInfo.FileName);
    ui->windowTitleLineEdit->setText(m_MapProcessInfo.WindowTitle);
    if ((false == m_MapProcessInfo.FilePath.isEmpty())
        && (true == QFileInfo::exists(m_MapProcessInfo.FilePath))){
        ui->processLineEdit->setToolTip(m_MapProcessInfo.FilePath);

        // Use the new icon extraction method, prefer extracting icon with specified size
        QIcon fileicon;
        fileicon = extractBestIconFromExecutable(m_MapProcessInfo.FilePath);

        if (fileicon.isNull()) {
            // If the new method fails, fallback to the original QFileIconProvider method
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[UpdateProcessInfo]" << "New icon extraction failed, falling back to QFileIconProvider";
#endif
            QFileIconProvider icon_provider;
            fileicon = icon_provider.icon(QFileInfo(m_MapProcessInfo.FilePath));
        }

        if (!fileicon.isNull()) {
            m_MapProcessInfo.WindowIcon = fileicon;
        }

        if (!m_MapProcessInfo.WindowIcon.isNull()) {
#ifdef DEBUG_LOGOUT_ON
            QList<QSize> iconsizeList = m_MapProcessInfo.WindowIcon.availableSizes();
            qDebug() << "[UpdateProcessInfo]" << "Icon availableSizes:" << iconsizeList;
#endif
            // Directly create pixmap from the extracted icon, as we have ensured the correct size
            QPixmap scaled_pixmap = m_MapProcessInfo.WindowIcon.pixmap(QSize(DEFAULT_ICON_WIDTH, DEFAULT_ICON_WIDTH));
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[UpdateProcessInfo]" << " Icon Scaled(" << QSize(DEFAULT_ICON_WIDTH, DEFAULT_ICON_WIDTH) << ") pixmap size: " << scaled_pixmap.size();
#endif

            ui->iconLabel->setPixmap(scaled_pixmap);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[UpdateProcessInfo]" << "Load & retrive file icon failure!!!";
#endif
            ui->iconLabel->clear();
        }
    }
    else{
        ui->iconLabel->clear();
    }
}

void QKeyMapper::showQKeyMapperWindowToTop()
{
    /* BringWindowToTopEx() may cause OBS program registered shortcut be invalid. >>> */
    // BringWindowToTopEx(reinterpret_cast<HWND>(winId()));
    SetWindowPos(reinterpret_cast<HWND>(winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    /* BringWindowToTopEx() may cause OBS program registered shortcut be invalid. <<< */
    showNormal();
    activateWindow();
    raise();
}

void QKeyMapper::switchShowHide()
{
    if (m_deviceListWindow->isVisible()) {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[switchShowHide]" << "DeviceList Windows isVisible!";
#endif
        return;
    }

    if (false == isHidden()) {
        m_LastWindowPosition = pos(); // Save the current position before hiding
        closeSelectColorDialog();
        closeTableSetupDialog();
        closeFloatingWindowSetupDialog();
        closeItemSetupDialog();
        closeCrosshairSetupDialog();
        closeGyro2MouseAdvancedSettingDialog();
        closeTrayIconSelectDialog();
        closeNotificationSetupDialog();
        hide();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[switchShowHide] Hide Window, LastWindowPosition ->" << m_LastWindowPosition;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[switchShowHide] Show Window, LastWindowPosition ->" << m_LastWindowPosition;
#endif

        if (m_LastWindowPosition.x() != INITIAL_WINDOW_POSITION && m_LastWindowPosition.y() != INITIAL_WINDOW_POSITION) {
            move(m_LastWindowPosition); // Restore the position before showing
        }

        showQKeyMapperWindowToTop();
    }
}

void QKeyMapper::forceHide()
{
    if (false == isHidden()) {
        m_LastWindowPosition = pos(); // Save the current position before hiding
        closeSelectColorDialog();
        closeInputDeviceListWindow();
        closeTableSetupDialog();
        closeFloatingWindowSetupDialog();
        closeItemSetupDialog();
        closeCrosshairSetupDialog();
        closeGyro2MouseAdvancedSettingDialog();
        closeTrayIconSelectDialog();
        closeNotificationSetupDialog();
        hide();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QKeyMapper::forceHide] Force hide Window, LastWindowPosition ->" << m_LastWindowPosition;
#endif
    }
}

void QKeyMapper::hideProcessList()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[hideProcessList] Hide the process info list, show wide keymapping table.";
#endif

    ui->processinfoTable->setVisible(false);
}

void QKeyMapper::showProcessList()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[showProcessList] Show the process info list, show narrow keymapping table.";
#endif

    ui->processinfoTable->setVisible(true);
}

void QKeyMapper::setKeyMappingTabWidgetWideMode()
{
    if (ui->processListButton->isChecked()) {
        return;
    }

    QRect widgetGeometry = m_KeyMappingTabWidget->geometry();
    int widget_left = KEYMAPPINGTABWIDGET_WIDE_LEFT;
    int widget_width = KEYMAPPINGTABWIDGET_WIDE_WIDTH;
    int widget_top = widgetGeometry.top();
    int widget_height = widgetGeometry.height();
    m_KeyMappingTabWidget->setGeometry(QRect(widget_left, widget_top, widget_width, widget_height));

    int table_left = KEYMAPPINGDATATABLE_WIDE_LEFT;
    int table_width = KEYMAPPINGDATATABLE_WIDE_WIDTH;
    int table_top = KEYMAPPINGDATATABLE_TOP;
    int table_height = KEYMAPPINGDATATABLE_HEIGHT;

    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index) {
        KeyMappingDataTableWidget *mappingDataTable = s_KeyMappingTabInfoList.at(index).KeyMappingDataTable;
        mappingDataTable->setGeometry(QRect(table_left, table_top, table_width, table_height));
    }
}

void QKeyMapper::setKeyMappingTabWidgetNarrowMode()
{
    if (!ui->processListButton->isChecked()) {
        return;
    }

    QRect widgetGeometry = m_KeyMappingTabWidget->geometry();
    int widget_left = KEYMAPPINGTABWIDGET_NARROW_LEFT;
    int widget_width = KEYMAPPINGTABWIDGET_NARROW_WIDTH;
    int widget_top = widgetGeometry.top();
    int widget_height = widgetGeometry.height();
    m_KeyMappingTabWidget->setGeometry(QRect(widget_left, widget_top, widget_width, widget_height));

    int table_left = KEYMAPPINGDATATABLE_NARROW_LEFT;
    int table_width = KEYMAPPINGDATATABLE_NARROW_WIDTH;
    int table_top = KEYMAPPINGDATATABLE_TOP;
    int table_height = KEYMAPPINGDATATABLE_HEIGHT;

    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index) {
        KeyMappingDataTableWidget *mappingDataTable = s_KeyMappingTabInfoList.at(index).KeyMappingDataTable;
        mappingDataTable->setGeometry(QRect(table_left, table_top, table_width, table_height));
    }
}

bool QKeyMapper::isCloseToSystemtray(bool force_showdialog)
{
    bool closeto_systemtray = false;

#ifdef DEBUG_LOGOUT_ON
    QString debugmessage = QString("\033[1;34m[isCloseToSystemtray] Check setting to show close to systemtray dialog. ForceShowDialog = %1\033[0m").arg(force_showdialog?"true":"false");
    qDebug().noquote() << debugmessage;
#endif

    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    bool setting_contains = false;
    if (true == settingFile.contains(CLOSETO_SYSTEMTRAY)){
        setting_contains = true;
        closeto_systemtray = settingFile.value(CLOSETO_SYSTEMTRAY).toBool();
    }

    if (force_showdialog || setting_contains != true) {
        QString message = tr(
            "If you want the program to be hidden to the system tray when you click the close button, please check the option below.\n"
            "If you do not check it and click \"OK\", the program will close directly when you click the close button in the future.\n"
            "Hold the \"L-Ctrl\" key and click the close button to show this dialog again."
        );
        QString checkbox_message = tr("Hide the program to the system tray when clicking the close button");
        bool ischecked = showMessageBoxWithCheckbox(this, message, checkbox_message, CustomMessageBox::Question);
        if (ischecked) {
            settingFile.setValue(CLOSETO_SYSTEMTRAY, true);
            closeto_systemtray = true;
        }
        else {
            settingFile.setValue(CLOSETO_SYSTEMTRAY, false);
            closeto_systemtray = false;
        }
    }

    return closeto_systemtray;
}

void QKeyMapper::updateSystemTrayDisplay()
{
    QString TrayInfo;
    QString description = ui->descriptionLineEdit->text();
    if (!description.isEmpty()) {
        TrayInfo = description;
    }
    else {
        TrayInfo = ui->settingselectComboBox->currentText();
    }

    QString systray_tooltip_status;
    if (KEYMAP_CHECKING == m_KeyMapStatus) {
        m_SysTrayIcon->setIcon(m_TrayIconSelectDialog->getMonitoringStateQIcon());
        systray_tooltip_status = tr("Monitoring : ") + TrayInfo;
    }
    else if (KEYMAP_MAPPING_MATCHED == m_KeyMapStatus) {
        QString tabCustomImage_Path = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_Path;
        bool tabCustomImage_ShowAsTrayIcon = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_ShowAsTrayIcon;

        bool show_tabcustomimageicon = false;
        QIcon customImageIcon;
        if (!tabCustomImage_Path.isEmpty() && tabCustomImage_ShowAsTrayIcon) {
            customImageIcon = QIcon(tabCustomImage_Path);
            show_tabcustomimageicon = true;
        }
        bool processicon_as_trayicon = getProcessIconAsTrayIconStatus();
        QIcon trayicon;

        if (show_tabcustomimageicon && !customImageIcon.isNull()) {
            QList<QSize> iconsizeList = customImageIcon.availableSizes();
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::updateSystemTrayDisplay]" << "Tab custom icon availableSizes:" << iconsizeList;
#endif
            QSize trayicon_pixel = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabCustomImage_TrayIconPixel;
            if (iconsizeList.contains(trayicon_pixel)) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                QPixmap scaled_pixmap = customImageIcon.pixmap(trayicon_pixel, 1.0);
#else
                qreal devicePixelRatio = qApp->devicePixelRatio();
                QSize deviceIndependentSize = QSize(
                    qRound(trayicon_pixel.width() / devicePixelRatio),
                    qRound(trayicon_pixel.height() / devicePixelRatio)
                );
                QPixmap scaled_pixmap = customImageIcon.pixmap(deviceIndependentSize);
#endif

#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[QKeyMapper::updateSystemTrayDisplay]" << " TabCustomImage_TrayIconPixel: " << trayicon_pixel <<", Scaled pixmap size: " << scaled_pixmap.size();
#endif
                trayicon = QIcon(scaled_pixmap);
            }
            else {
                trayicon = customImageIcon;
            }
        }
        else if (processicon_as_trayicon && !m_MapProcessInfo.WindowIcon.isNull()){
            trayicon = m_MapProcessInfo.WindowIcon;
        }
        else {
            trayicon = m_TrayIconSelectDialog->getMatchedStateQIcon();
        }

        m_SysTrayIcon->setIcon(trayicon);
        systray_tooltip_status = tr("Mapping : ") + TrayInfo;
    }
    else if (KEYMAP_MAPPING_GLOBAL == m_KeyMapStatus) {
        /* Need to make a new global mapping status ICO */
        m_SysTrayIcon->setIcon(m_TrayIconSelectDialog->getGlobalStateQIcon());
        systray_tooltip_status = tr("Mapping : Global");
    }
    else {
        m_SysTrayIcon->setIcon(m_TrayIconSelectDialog->getIdleStateQIcon());
        systray_tooltip_status = tr("Idle");
    }
    systray_tooltip_status.prepend("【");
    systray_tooltip_status.append("】");

    // Combine with newline
    QString systray_tooltip = QString("%1\n%2").arg(PROGRAM_NAME, systray_tooltip_status);

    // Replace '&' with '&&&' to avoid issues in the system tray tooltip display such as "A&B"
    systray_tooltip.replace("&", "&&&");
    m_SysTrayIcon->setToolTip(systray_tooltip);
}

void QKeyMapper::showInformationPopup(const QString &message)
{
    showPopupMessage(message, SUCCESS_COLOR, 3000);
}

void QKeyMapper::showWarningPopup(const QString &message)
{
    showPopupMessage(message, WARNING_COLOR, 3000);
}

void QKeyMapper::showFailurePopup(const QString &message)
{
    showPopupMessage(message, FAILURE_COLOR, 3000);
}

void QKeyMapper::showNotificationPopup(const QString &message, const PopupNotificationOptions &options)
{
    if (Q_NULLPTR == m_PopupNotification) {
        return;
    }
    m_PopupNotification->showPopupNotification(message, options);
}

void QKeyMapper::showNotificationPopup(const QString &message)
{
    if (Q_NULLPTR == m_PopupNotification) {
        return;
    }

    QString color_str;
    QColor notification_font_color = m_NotificationSetupDialog->getNotification_FontColor();
    if (notification_font_color.isValid()) {
        color_str = notification_font_color.name();
    }
    else {
        color_str = NOTIFICATION_COLOR_NORMAL_DEFAULT_STR;
    }

    PopupNotificationOptions opts;
    opts.color = color_str;
    opts.position = ui->notificationComboBox->currentIndex();
    opts.size = m_NotificationSetupDialog->getNotification_FontSize();
    opts.displayDuration = m_NotificationSetupDialog->getNotification_DisplayDuration();
    opts.backgroundColor = m_NotificationSetupDialog->getNotification_BackgroundColor();
    opts.windowOpacity = m_NotificationSetupDialog->getNotification_Opacity();
    opts.padding = m_NotificationSetupDialog->getNotification_Padding();
    opts.borderRadius = m_NotificationSetupDialog->getNotification_BorderRadius();
    int font_weight = m_NotificationSetupDialog->getNotification_FontWeight();
    if (NOTIFICATION_FONT_WEIGHT_LIGHT == font_weight) {
        opts.fontWeight = QFont::Light;
    }
    else if (NOTIFICATION_FONT_WEIGHT_NORMAL == font_weight) {
        opts.fontWeight = QFont::Normal;
    }
    else {
        opts.fontWeight = QFont::Bold;
    }
    opts.fontItalic = m_NotificationSetupDialog->getNotification_FontIsItalic();
    opts.fadeInDuration = m_NotificationSetupDialog->getNotification_FadeInDuration();
    opts.fadeOutDuration = m_NotificationSetupDialog->getNotification_FadeOutDuration();
    opts.xOffset = m_NotificationSetupDialog->getNotification_X_Offset();
    opts.yOffset = m_NotificationSetupDialog->getNotification_Y_Offset();

    m_PopupNotification->showPopupNotification(message, opts);
}

void QKeyMapper::showFloatingIconWindow(const FloatingWindowOptions &options)
{
    if (Q_NULLPTR == m_FloatingIconWindow) {
        return;
    }
    m_FloatingIconWindow->showFloatingWindow(options);
}

void QKeyMapper::hideFloatingIconWindow()
{
    if (Q_NULLPTR == m_FloatingIconWindow) {
        return;
    }
    m_FloatingIconWindow->hideFloatingWindow();
}

void QKeyMapper::updateFloatingIconWindow(const FloatingWindowOptions &options)
{
    if (Q_NULLPTR == m_FloatingIconWindow) {
        return;
    }
    m_FloatingIconWindow->updateWindowSettings(options);
}

FloatingWindowOptions QKeyMapper::getCurrentFloatingWindowOptions() const
{
    if (Q_NULLPTR == m_FloatingIconWindow) {
        return FloatingWindowOptions();
    }
    return m_FloatingIconWindow->getCurrentOptions();
}

void QKeyMapper::initSelectColorDialog()
{
    if (m_SelectColorDialog == Q_NULLPTR) {
        m_SelectColorDialog = new QColorDialog(this);
    }
}

void QKeyMapper::setParentForSelectColorDialog(QWidget *parent)
{
    if (m_SelectColorDialog == Q_NULLPTR) {
        m_SelectColorDialog = new QColorDialog(this);
    }
    else {
        m_SelectColorDialog->setParent(parent);
    }
}

bool QKeyMapper::showMessageBoxWithCheckbox(QWidget *parent, QString message, QString checkbox_message, CustomMessageBox::IconType icontype)
{
    CustomMessageBox msgBox(parent, message, checkbox_message, icontype);
    bool ischecked = false;
    if (msgBox.exec() == QDialog::Accepted) {
        if (msgBox.isCheckBoxChecked()) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[showMessageBoxWithCheckbox]" << "Checkbox is checked ：" << message;
#endif
            ischecked = true;
        }
    }

    return ischecked;
}

bool QKeyMapper::isMappingDataTableFiltered()
{
    if (ui->showCategoryButton->isChecked()
        && ui->CategoryFilterComboBox->currentIndex() != CATEGORY_FILTER_ALL_INDEX) {
        return true;
    }
    else {
        return false;
    }
}

void QKeyMapper::onCategoryFilterChanged(int index)
{
    if (!m_KeyMappingDataTable) {
        return;
    }

    if (index == CATEGORY_FILTER_ALL_INDEX) {
        // Index 0 is always "All" - clear filter to show all items
        m_KeyMappingDataTable->clearCategoryFilter();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onCategoryFilterChanged]" << "Selected 'All' - clearing category filter";
#endif
    } else {
        // Index > 0 is a user category - apply the filter
        QString selectedCategory = ui->CategoryFilterComboBox->itemText(index);
        m_KeyMappingDataTable->setCategoryFilter(selectedCategory);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onCategoryFilterChanged]" << "Selected category:" << selectedCategory << "at index:" << index;
#endif
    }
}

void QKeyMapper::updateCategoryFilterComboBox(void)
{
    if (!m_KeyMappingDataTable) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[updateCategoryFilterComboBox]" << "m_KeyMappingDataTable is null, skipping update";
#endif
        return;
    }

    // Store the current filter value before updating ComboBox
    QString currentFilter = m_KeyMappingDataTable->m_CategoryFilter;

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[updateCategoryFilterComboBox]" << "Category filter:" << currentFilter;
#endif

    ui->CategoryFilterComboBox->clear();
    ui->CategoryFilterComboBox->addItem(tr("All"));

    QStringList categories = m_KeyMappingDataTable->getAvailableCategories();

    // Separate regular categories from the special "Blank" option
    QStringList regularCategories;
    bool hasBlankOption = false;

    for (const QString &category : std::as_const(categories)) {
        if (category == tr("Blank")) {
            hasBlankOption = true;
        } else {
            regularCategories.append(category);
        }
    }

    // Sort regular categories and add them
    regularCategories.sort();
    ui->CategoryFilterComboBox->addItems(regularCategories);

    // Add "Blank" option at the end if it exists
    if (hasBlankOption) {
        ui->CategoryFilterComboBox->addItem(tr("Blank"));
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[updateCategoryFilterComboBox]" << "Available categories:" << categories;
#endif

    int index = -1;

    // Check if we're looking for the "clear filter" state (empty filter)
    if (currentFilter.isEmpty()) {
        // Empty filter means "show all", so select index 0
        index = CATEGORY_FILTER_ALL_INDEX;
    } else if (currentFilter == tr("Blank")) {
        // Looking for blank entries
        if (hasBlankOption) {
            index = ui->CategoryFilterComboBox->count() - 1; // "Blank" is always last
        }
    } else {
        // We're looking for a specific category
        // Start searching from sorted regular categories to avoid the built-in "All" at index 0
        index = regularCategories.indexOf(currentFilter);
        if (index != -1) {
            index += 1; // Adjust index to account for "All" at index 0
        }
    }

    if (index != -1) {
        ui->CategoryFilterComboBox->setCurrentIndex(index);
#ifdef DEBUG_LOGOUT_ON
        if (index == CATEGORY_FILTER_ALL_INDEX) {
            qDebug() << "[updateCategoryFilterComboBox]" << "Restored filter to 'All' (show all items)";
        } else {
            qDebug() << "[updateCategoryFilterComboBox]" << "Restored filter to category:" << currentFilter << "at index:" << index;
        }
#endif
    } else {
        // Category not found, clear filter and default to "All"
        m_KeyMappingDataTable->m_CategoryFilter.clear();
        ui->CategoryFilterComboBox->setCurrentIndex(CATEGORY_FILTER_ALL_INDEX);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[updateCategoryFilterComboBox]" << "Previous filter not found, defaulting to 'All'";
#endif
    }
}

void QKeyMapper::updateCategoryFilterByShowCategoryState()
{
    // Update category-related controls after tab switch
    if (ui->showCategoryButton->isChecked()) {
        // If category feature is enabled, update the new tab accordingly
        if (m_KeyMappingDataTable) {
            m_KeyMappingDataTable->setCategoryColumnVisible(true);
            resizeKeyMappingDataTableColumnWidth(m_KeyMappingDataTable);
        }
        // Update filter combobox with categories from the new tab
        updateCategoryFilterComboBox();
    }
    else {
        // If category feature is disabled, ensure the new tab also has it disabled
        if (m_KeyMappingDataTable) {
            m_KeyMappingDataTable->setCategoryColumnVisible(false);
            resizeKeyMappingDataTableColumnWidth(m_KeyMappingDataTable);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[updateCategoryFilterByShowCategoryState]" << "Category button checked:" << ui->showCategoryButton->isChecked();
    if (m_KeyMappingDataTable) {
        qDebug() << "[updateCategoryFilterByShowCategoryState]" << "Category column visible:" << m_KeyMappingDataTable->isCategoryColumnVisible();
    }
#endif
}

void QKeyMapper::initKeyMappingTabWidget(void)
{
    m_KeyMappingTabWidget = new KeyMappingTabWidget(this);

    int left = KEYMAPPINGTABWIDGET_NARROW_LEFT;
    int width = KEYMAPPINGTABWIDGET_NARROW_WIDTH;
    if (!ui->processListButton->isChecked()) {
        left    = KEYMAPPINGTABWIDGET_WIDE_LEFT;
        width   = KEYMAPPINGTABWIDGET_WIDE_WIDTH;
    }
    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        m_KeyMappingTabWidget->setGeometry(QRect(left, 11, width, 346));
    }
    else {
        m_KeyMappingTabWidget->setGeometry(QRect(left, 7, width, 346));
    }
    QStyle* windowsStyle = QStyleFactory::create("windows");
    m_KeyMappingTabWidget->setStyle(windowsStyle);
    ui->addTabButton->setStyle(windowsStyle);
    m_KeyMappingTabWidget->setFocusPolicy(Qt::ClickFocus);
    m_KeyMappingTabWidget->setFont(QFont(FONTNAME_ENGLISH, 9));

    bool addtab_result = addTabToKeyMappingTabWidget();
    Q_UNUSED(addtab_result);
#ifdef DEBUG_LOGOUT_ON
    if (false == addtab_result) {
        qWarning() << "[initKeyMappingTabWidget]" << "addTabToKeyMappingTabWidget failed!";
    }
#endif

    setKeyMappingTabWidgetCurrentIndex(0);
    switchKeyMappingTabIndex(0);
}

void QKeyMapper::clearKeyMappingTabWidget()
{
    m_KeyMappingTabWidget->blockSignals(true);
    disconnectKeyMappingDataTableConnection();

    for (int tabindex = m_KeyMappingTabWidget->count() - 1; tabindex > 0; --tabindex) {
        m_KeyMappingTabWidget->removeTab(tabindex);
    }

    for (int index = s_KeyMappingTabInfoList.size() - 1; index > 0; --index) {
        if (s_KeyMappingTabInfoList.at(index).KeyMappingDataTable != Q_NULLPTR) {
            delete s_KeyMappingTabInfoList.at(index).KeyMappingDataTable;
        }
        if (s_KeyMappingTabInfoList.at(index).KeyMappingData != Q_NULLPTR) {
            delete s_KeyMappingTabInfoList.at(index).KeyMappingData;
        }
        s_KeyMappingTabInfoList.removeLast();
    }

    if (s_KeyMappingTabInfoList.size() > 0) {
        s_KeyMappingTabInfoList[0].TabName = "Tab1";
        s_KeyMappingTabInfoList[0].TabHotkey.clear();
        s_KeyMappingTabInfoList[0].TabFontColor = QColor();
        s_KeyMappingTabInfoList[0].TabBackgroundColor = QColor();
        s_KeyMappingTabInfoList[0].TabCustomImage_Path.clear();
        s_KeyMappingTabInfoList[0].TabCustomImage_ShowPosition = TAB_CUSTOMIMAGE_POSITION_DEFAULT;
        s_KeyMappingTabInfoList[0].TabCustomImage_Padding = TAB_CUSTOMIMAGE_PADDING_DEFAULT;
        s_KeyMappingTabInfoList[0].TabCustomImage_ShowAsTrayIcon = TAB_CUSTOMIMAGE_SHOW_AS_TRAYICON_DEFAULT;
        s_KeyMappingTabInfoList[0].TabCustomImage_ShowAsFloatingWindow = TAB_CUSTOMIMAGE_SHOW_AS_FLOATINGWINDOW_DEFAULT;
    }

    m_KeyMappingTabWidget->setTabText(0, "Tab1");
    setKeyMappingTabWidgetCurrentIndex(0);
    switchKeyMappingTabIndex(0);
    updateKeyMappingDataTableConnection();

    m_KeyMappingTabWidget->blockSignals(false);
}

void QKeyMapper::exitDeleteKeyMappingTabWidget()
{
    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index) {
        if (s_KeyMappingTabInfoList.at(index).KeyMappingData != Q_NULLPTR) {
            delete s_KeyMappingTabInfoList.at(index).KeyMappingData;
        }
    }
}

void QKeyMapper::disconnectKeyMappingDataTableConnection()
{
    if (m_KeyMappingDataTable != Q_NULLPTR) {
        QObject::disconnect(m_KeyMappingDataTable, &QTableWidget::cellChanged, this, &QKeyMapper::cellChanged_slot);
        QObject::disconnect(m_KeyMappingDataTable, &QTableWidget::itemSelectionChanged, this, &QKeyMapper::keyMappingTabl_ItemSelectionChanged);
        QObject::disconnect(m_KeyMappingDataTable, &QTableWidget::itemDoubleClicked, this, &QKeyMapper::keyMappingTableItemDoubleClicked);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[disconnectKeyMappingDataTableConnection]" << "Invalid m_KeyMappingDataTable pointer!";
#endif
    }
}

void QKeyMapper::updateKeyMappingDataTableConnection()
{
    if (m_KeyMappingDataTable != Q_NULLPTR) {
        QObject::connect(m_KeyMappingDataTable, &QTableWidget::cellChanged, this, &QKeyMapper::cellChanged_slot, Qt::UniqueConnection);
        QObject::connect(m_KeyMappingDataTable, &QTableWidget::itemSelectionChanged, this, &QKeyMapper::keyMappingTabl_ItemSelectionChanged, Qt::UniqueConnection);
        QObject::connect(m_KeyMappingDataTable, &QTableWidget::itemDoubleClicked, this, &QKeyMapper::keyMappingTableItemDoubleClicked, Qt::UniqueConnection);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[updateKeyMappingDataTableConnection]" << "Invalid m_KeyMappingDataTable pointer!";
#endif
    }
}

void QKeyMapper::resizeKeyMappingDataTableColumnWidth(KeyMappingDataTableWidget *mappingDataTable)
{
    mappingDataTable->resizeColumnToContents(ORIGINAL_KEY_COLUMN);

    int original_key_width_min = mappingDataTable->width()/5 - 15;
    int original_key_width_max = mappingDataTable->width() / 2;
    int original_key_width = mappingDataTable->columnWidth(ORIGINAL_KEY_COLUMN);

    mappingDataTable->resizeColumnToContents(BURST_MODE_COLUMN);
    int burst_mode_width = mappingDataTable->columnWidth(BURST_MODE_COLUMN);
    int lock_width = burst_mode_width;
    burst_mode_width += 8;

    int category_width = 0;
    if (mappingDataTable->isCategoryColumnVisible()) {
        lock_width += 8; // Add padding for lock column
        mappingDataTable->horizontalHeader()->setStretchLastSection(false);
        int category_width_max = mappingDataTable->width() / 5;
        mappingDataTable->resizeColumnToContents(CATEGORY_COLUMN);
        category_width = mappingDataTable->columnWidth(CATEGORY_COLUMN);
        if (category_width < burst_mode_width) {
            category_width = burst_mode_width;
        }
        if (category_width > category_width_max) {
            category_width = category_width_max;
        }
        mappingDataTable->horizontalHeader()->setStretchLastSection(true);
    }

    if (original_key_width < original_key_width_min) {
        original_key_width = original_key_width_min;
    }
    else if (original_key_width > original_key_width_max) {
        original_key_width = original_key_width_max;
    }

    int mapping_key_width_min = mappingDataTable->width()/5 - 15;
    int mapping_key_width = mappingDataTable->width() - original_key_width - burst_mode_width - lock_width - category_width - 16;
    if (mapping_key_width < mapping_key_width_min) {
        mapping_key_width = mapping_key_width_min;
    }

    mappingDataTable->setColumnWidth(ORIGINAL_KEY_COLUMN, original_key_width);
    mappingDataTable->setColumnWidth(MAPPING_KEY_COLUMN, mapping_key_width);
    mappingDataTable->setColumnWidth(BURST_MODE_COLUMN, burst_mode_width);
    mappingDataTable->setColumnWidth(LOCK_COLUMN, lock_width);
    if (mappingDataTable->isCategoryColumnVisible()) {
        mappingDataTable->setColumnWidth(CATEGORY_COLUMN, category_width);
    }
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[resizeKeyMappingDataTableColumnWidth]" << "mappingDataTable->rowCount" << mappingDataTable->rowCount();
    qDebug() << "[resizeKeyMappingDataTableColumnWidth]" << "original_key_width =" << original_key_width << ", mapping_key_width =" << mapping_key_width << ", burst_mode_width =" << burst_mode_width << ", lock_width =" << lock_width << ", category_width =" << category_width;
#endif
}

void QKeyMapper::resizeAllKeyMappingTabWidgetColumnWidth()
{
    for (int tabindex = 0; tabindex < m_KeyMappingTabWidget->count(); ++tabindex) {
        KeyMappingDataTableWidget *mappingDataTable = qobject_cast<KeyMappingDataTableWidget*>(m_KeyMappingTabWidget->widget(tabindex));
        resizeKeyMappingDataTableColumnWidth(mappingDataTable);
    }
}

void QKeyMapper::clearLockStatusDisplay()
{
    if (!m_KeyMappingDataTable) {
        return;
    }

    int rowCount = m_KeyMappingDataTable->rowCount();

    for (int row = 0; row < rowCount; ++row) {
        m_KeyMappingDataTable->item(row, LOCK_COLUMN)->setForeground(Qt::black);
    }
}

void QKeyMapper::initQSimpleUpdater()
{
    QString qkeymapper_updates_url_github = CHECK_UPDATES_URL_GITHUB;
    QString qkeymapper_updates_url_gitee = CHECK_UPDATES_URL_GITEE;
    QString productVersion = getExeProductVersion();
    QString platformString = getPlatformString();
    QString downloadDir = UPDATE_DOWNLOAD_DIR;
    QString user_agent = UPDATER_USER_AGENT_X64;

    /* Config Update parameter for Github */
    QSimpleUpdater::getInstance()->setModuleName                (qkeymapper_updates_url_github, PROGRAM_NAME);
    QSimpleUpdater::getInstance()->setModuleVersion             (qkeymapper_updates_url_github, productVersion);
    QSimpleUpdater::getInstance()->setPlatformKey               (qkeymapper_updates_url_github, platformString);
    QSimpleUpdater::getInstance()->setUserAgentString           (qkeymapper_updates_url_github, user_agent);
    QSimpleUpdater::getInstance()->setDownloaderEnabled         (qkeymapper_updates_url_github, true);
    QSimpleUpdater::getInstance()->setUseCustomInstallProcedures(qkeymapper_updates_url_github, true);
    QSimpleUpdater::getInstance()->setNotifyOnUpdate            (qkeymapper_updates_url_github, true);
    QSimpleUpdater::getInstance()->setNotifyOnFinish            (qkeymapper_updates_url_github, true);
    QSimpleUpdater::getInstance()->setDownloadDir               (qkeymapper_updates_url_github, downloadDir);

    /* Config Update parameter for Gitee */
    QSimpleUpdater::getInstance()->setModuleName                (qkeymapper_updates_url_gitee, PROGRAM_NAME);
    QSimpleUpdater::getInstance()->setModuleVersion             (qkeymapper_updates_url_gitee, productVersion);
    QSimpleUpdater::getInstance()->setPlatformKey               (qkeymapper_updates_url_gitee, platformString);
    QSimpleUpdater::getInstance()->setUserAgentString           (qkeymapper_updates_url_gitee, user_agent);
    QSimpleUpdater::getInstance()->setDownloaderEnabled         (qkeymapper_updates_url_gitee, true);
    QSimpleUpdater::getInstance()->setUseCustomInstallProcedures(qkeymapper_updates_url_gitee, true);
    QSimpleUpdater::getInstance()->setNotifyOnUpdate            (qkeymapper_updates_url_gitee, true);
    QSimpleUpdater::getInstance()->setNotifyOnFinish            (qkeymapper_updates_url_gitee, true);
    QSimpleUpdater::getInstance()->setDownloadDir               (qkeymapper_updates_url_gitee, downloadDir);

    QObject::connect(QSimpleUpdater::getInstance(), &QSimpleUpdater::downloadFinished, this, &QKeyMapper::onUpdateDownloadFinished);

#ifdef DEBUG_LOGOUT_ON
    qDebug().noquote().nospace() << "[initQSimpleUpdater] setModuleName     : " << PROGRAM_NAME;
    qDebug().noquote().nospace() << "[initQSimpleUpdater] setModuleVersion  : " << productVersion;
    qDebug().noquote().nospace() << "[initQSimpleUpdater] setPlatformKey    : " << platformString;
    qDebug().noquote().nospace() << "[initQSimpleUpdater] setDownloadDir    : " << downloadDir;
    qDebug().noquote().nospace() << "[initQSimpleUpdater] setUserAgentString: " << user_agent;
#endif
}

void QKeyMapper::initKeysCategoryMap()
{
    /* Original keylists */
    QStringList& original_mouse_keylist = s_OriginalKeysCategoryMap[KEY_TYPE_MOUSE];
    QStringList& original_keyboard_keylist = s_OriginalKeysCategoryMap[KEY_TYPE_KEYBOARD];
    QStringList& original_gamepad_keylist = s_OriginalKeysCategoryMap[KEY_TYPE_GAMEPAD];
    QStringList& original_function_keylist = s_OriginalKeysCategoryMap[KEY_TYPE_FUNCTION];

    /* Mapping keylists */
    QStringList& mapping_common_keylist = s_MappingKeysCategoryMap[KEY_TYPE_COMMON];
    QStringList& mapping_mouse_keylist = s_MappingKeysCategoryMap[KEY_TYPE_MOUSE];
    QStringList& mapping_keyboard_keylist = s_MappingKeysCategoryMap[KEY_TYPE_KEYBOARD];
    QStringList& mapping_gamepad_keylist = s_MappingKeysCategoryMap[KEY_TYPE_GAMEPAD];
    QStringList& mapping_function_keylist = s_MappingKeysCategoryMap[KEY_TYPE_FUNCTION];

    /* Mapping Common Keys */
    mapping_common_keylist = QStringList() \
        << KEY_BLOCKED_STR
        << KEY_NONE_STR
        << SENDTEXT_STR
        << KEYSEQUENCEBREAK_STR
        ;

    /* Original Keyboard Keys */
    original_keyboard_keylist = QStringList() \
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
        << "1"
        << "2"
        << "3"
        << "4"
        << "5"
        << "6"
        << "7"
        << "8"
        << "9"
        << "0"
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
        << "L-Shift"
        << "R-Shift"
        << "L-Ctrl"
        << "R-Ctrl"
        << "L-Alt"
        << "R-Alt"
        << "L-Win"
        << "R-Win"
        << "Backspace"
        << "`"
        << "-"
        << "="
        << "["
        << "]"
        << "\\"
        << ";"
        << "'"
        << ","
        << "."
        << "/"
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
        << "Num/"
        << "Num*"
        << "Num-"
        << "Num＋"
        << "Num."
        << "Num0"
        << "Num1"
        << "Num2"
        << "Num3"
        << "Num4"
        << "Num5"
        << "Num6"
        << "Num7"
        << "Num8"
        << "Num9"
        << "NumEnter"
        << "Num.(NumOFF)"
        << "Num0(NumOFF)"
        << "Num1(NumOFF)"
        << "Num2(NumOFF)"
        << "Num3(NumOFF)"
        << "Num4(NumOFF)"
        << "Num5(NumOFF)"
        << "Num6(NumOFF)"
        << "Num7(NumOFF)"
        << "Num8(NumOFF)"
        << "Num9(NumOFF)"
        ;

    /* Mapping Keyboard Keys */
    mapping_keyboard_keylist = original_keyboard_keylist;

    /* Original Mouse Keys */
    original_mouse_keylist = QStringList() \
        << MOUSE_L_STR
        << MOUSE_R_STR
        << MOUSE_M_STR
        << MOUSE_X1_STR
        << MOUSE_X2_STR
        << MOUSE_WHEEL_UP_STR
        << MOUSE_WHEEL_DOWN_STR
        << MOUSE_WHEEL_LEFT_STR
        << MOUSE_WHEEL_RIGHT_STR
        ;

    /* Mapping Mouse Keys */
    mapping_mouse_keylist = original_mouse_keylist;
    mapping_mouse_keylist << QStringList() \
        << KEY2MOUSE_UP_STR
        << KEY2MOUSE_DOWN_STR
        << KEY2MOUSE_LEFT_STR
        << KEY2MOUSE_RIGHT_STR
        << MOUSE_L_WINDOWPOINT_STR
        << MOUSE_R_WINDOWPOINT_STR
        << MOUSE_M_WINDOWPOINT_STR
        << MOUSE_X1_WINDOWPOINT_STR
        << MOUSE_X2_WINDOWPOINT_STR
        << MOUSE_MOVE_WINDOWPOINT_STR
        << MOUSE_L_SCREENPOINT_STR
        << MOUSE_R_SCREENPOINT_STR
        << MOUSE_M_SCREENPOINT_STR
        << MOUSE_X1_SCREENPOINT_STR
        << MOUSE_X2_SCREENPOINT_STR
        << MOUSE_MOVE_SCREENPOINT_STR
        ;

    /* Original Gamdpad Keys */
    original_gamepad_keylist = QStringList() \
        << VJOY_MOUSE2LS_STR
        << VJOY_MOUSE2RS_STR
        << JOY_LS2MOUSE_STR
        << JOY_RS2MOUSE_STR
        << JOY_GYRO2MOUSE_STR
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
        << "Joy-Key13(Guide)"
        << "Joy-Key14"
        << "Joy-Key15"
        << "Joy-Key16"
        << "Joy-Key17"
        << "Joy-Key18"
        << "Joy-Key19"
        << "Joy-Key20"
        << "Joy-Key21"
        << "Joy-Key22"
        << "Joy-Key23"
        << "Joy-Key24"
        << "Joy-Key25"
        << "Joy-Key26"
        << "Joy-Key27"
        << "Joy-Key28"
        << "Joy-Key29"
        << "Joy-Key30"
        << JOY_LS2VJOYLS_STR
        << JOY_RS2VJOYRS_STR
        << JOY_LS2VJOYRS_STR
        << JOY_RS2VJOYLS_STR
        << JOY_LT2VJOYLT_STR
        << JOY_RT2VJOYRT_STR
        ;

    /* Mapping Gamdpad Keys */
    mapping_gamepad_keylist = QStringList() \
        << GYRO2MOUSE_HOLD_KEY_STR
        << GYRO2MOUSE_MOVE_KEY_STR
        << MOUSE2VJOY_HOLD_KEY_STR
        << VJOY_LS_RADIUS_STR
        << VJOY_RS_RADIUS_STR
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
        << "vJoy-Key13(Guide)"
        << VJOY_LT_BRAKE_STR
        << VJOY_RT_BRAKE_STR
        << VJOY_LT_ACCEL_STR
        << VJOY_RT_ACCEL_STR
        ;

    /* Original Function Keys */
    original_function_keylist = QStringList() \
        << "VolumeMute"
        << "VolumeDown"
        << "VolumeUp"
        << "MediaNext"
        << "MediaPrev"
        << "MediaStop"
        << "MediaPlayPause"
        << "LaunchMail"
        << "SelectMedia"
        << "LaunchApp1"
        << "LaunchApp2"
        << "BrowserBack"
        << "BrowserForward"
        << "BrowserRefresh"
        << "BrowserStop"
        << "BrowserSearch"
        << "BrowserFavorites"
        << "BrowserHome"
        ;

    /* Mapping Function Keys */
    mapping_function_keylist = QStringList() \
        << CROSSHAIR_NORMAL_STR
        << CROSSHAIR_TYPEA_STR
        << "VolumeMute"
        << "VolumeDown"
        << "VolumeUp"
        << "MediaNext"
        << "MediaPrev"
        << "MediaStop"
        << "MediaPlayPause"
        << "LaunchMail"
        << "SelectMedia"
        << "LaunchApp1"
        << "LaunchApp2"
        << "BrowserBack"
        << "BrowserForward"
        << "BrowserRefresh"
        << "BrowserStop"
        << "BrowserSearch"
        << "BrowserFavorites"
        << "BrowserHome"
        << FUNC_REFRESH
        << FUNC_LOCKSCREEN
        << FUNC_SHUTDOWN
        << FUNC_REBOOT
        << FUNC_LOGOFF
        << FUNC_SLEEP
        << FUNC_HIBERNATE
        ;
}

void QKeyMapper::initAddKeyComboBoxes(void)
{
    int left = ui->orikeyLabel->x() + ui->orikeyLabel->width() + 5;
    int top = ui->orikeyLabel->y();
    m_orikeyComboBox->setObjectName(ORIKEY_COMBOBOX_NAME);
    m_orikeyComboBox->setGeometry(QRect(left, top, 161, 22));
    m_orikeyComboBox->setFocusPolicy(Qt::WheelFocus);
    // m_orikeyComboBox->setEditable(true);
    left = ui->mapkeyLabel->x() + ui->mapkeyLabel->width() + 5;
    top = ui->mapkeyLabel->y();
    m_mapkeyComboBox->setObjectName(MAPKEY_COMBOBOX_NAME);
    m_mapkeyComboBox->setGeometry(QRect(left, top, 151, 22));
    m_mapkeyComboBox->setFocusPolicy(Qt::WheelFocus);
    // m_mapkeyComboBox->setEditable(true);

    updateOriginalKeyListComboBox();
    updateMappingKeyListComboBox();
}

void QKeyMapper::initInputDeviceSelectComboBoxes()
{
    initKeyboardSelectComboBox();
    initMouseSelectComboBox();
}

void QKeyMapper::initCategoryFilterControls()
{
    // Initialize category filter ComboBox
    ui->CategoryFilterComboBox->clear();
    ui->CategoryFilterComboBox->addItem(tr("All"));
    ui->CategoryFilterComboBox->setVisible(false);
}

void QKeyMapper::initKeyboardSelectComboBox()
{
    if (!ui->keyboardSelectComboBox->isEnabled()) {
        return;
    }

    int lastIndex = ui->keyboardSelectComboBox->currentIndex();
    ui->keyboardSelectComboBox->clear();

    QStringList keyboardDeviceList;
    keyboardDeviceList.append(QString());

    QList<InputDevice> keyboardlist = Interception_Worker::getKeyboardDeviceList();
    for (const InputDevice &inputdevice : std::as_const(keyboardlist))
    {
        QString deviceStr;
        if (inputdevice.deviceinfo.devicedesc.isEmpty()) {
            deviceStr = QString("[%1] %2").arg(QString::number(inputdevice.device - INTERCEPTION_KEYBOARD(0)), QString(NO_INPUTDEVICE));
        }
        else {
            deviceStr = QString("[%1] %2").arg(QString::number(inputdevice.device - INTERCEPTION_KEYBOARD(0)), inputdevice.deviceinfo.devicedesc);
        }
        keyboardDeviceList.append(deviceStr);
    }
    ui->keyboardSelectComboBox->addItems(keyboardDeviceList);

    if (lastIndex != 0) {
        ui->keyboardSelectComboBox->setCurrentIndex(lastIndex);
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[initKeyboardSelectComboBox]" << "keyboardDeviceList ->" << keyboardDeviceList;
#endif
}

void QKeyMapper::initMouseSelectComboBox()
{
    if (!ui->mouseSelectComboBox->isEnabled()) {
        return;
    }

    int lastIndex = ui->mouseSelectComboBox->currentIndex();
    ui->mouseSelectComboBox->clear();

    QStringList mouseDeviceList;
    mouseDeviceList.append(QString());

    QList<InputDevice> mouselist = Interception_Worker::getMouseDeviceList();
    for (const InputDevice &inputdevice : std::as_const(mouselist))
    {
        QString deviceStr;
        if (inputdevice.deviceinfo.devicedesc.isEmpty()) {
            deviceStr = QString("[%1] %2").arg(QString::number(inputdevice.device - INTERCEPTION_MOUSE(0)), QString(NO_INPUTDEVICE));
        }
        else {
            deviceStr = QString("[%1] %2").arg(QString::number(inputdevice.device - INTERCEPTION_MOUSE(0)), inputdevice.deviceinfo.devicedesc);
        }
        mouseDeviceList.append(deviceStr);
    }
    ui->mouseSelectComboBox->addItems(mouseDeviceList);

    if (lastIndex != 0) {
        ui->mouseSelectComboBox->setCurrentIndex(lastIndex);
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[initMouseSelectComboBox]" << "mouseDeviceList ->" << mouseDeviceList;
#endif
}

void QKeyMapper::initWindowSwitchKeyLineEdit()
{
    // int top = ui->windowswitchkeyLabel->y();
    // int left = ui->windowswitchkeyLabel->x() + ui->windowswitchkeyLabel->width() + 5;
    // m_windowswitchKeySeqEdit->setObjectName(QStringLiteral("windowswitchKeySeqEdit"));
    // m_windowswitchKeySeqEdit->setGeometry(QRect(left, top, 110, 21));
    // m_windowswitchKeySeqEdit->setFocusPolicy(Qt::ClickFocus);

    QLineEdit *lineEdit = ui->windowswitchkeyLineEdit;
    // left = 390;
    // top = ui->windowswitchkeyLabel->y();
    // lineEdit->setGeometry(QRect(left, top, 110, 21));
    lineEdit->setFocusPolicy(Qt::ClickFocus);
    QObject::connect(lineEdit, &QLineEdit::textChanged, [lineEdit]() {
        lineEdit->setToolTip(lineEdit->text());
    });
    QObject::connect(lineEdit, &QLineEdit::editingFinished, this, &QKeyMapper::onHotKeyLineEditEditingFinished);
}

void QKeyMapper::initMappingSwitchKeyLineEdit()
{
    // int top = ui->mappingStartKeyLabel->y();
    // int left = ui->mappingStartKeyLabel->x() + ui->mappingStartKeyLabel->width() + 5;
    // m_mappingswitchKeySeqEdit->setObjectName(QStringLiteral("mappingswitchKeySeqEdit"));
    // m_mappingswitchKeySeqEdit->setGeometry(QRect(left, top, 110, 21));
    // m_mappingswitchKeySeqEdit->setFocusPolicy(Qt::ClickFocus);

    QLineEdit *lineEdit = ui->mappingStartKeyLineEdit;
    // left = 390;
    // top = ui->mappingStartKeyLabel->y();
    // lineEdit->setGeometry(QRect(left, top, 110, 21));
    lineEdit->setFocusPolicy(Qt::ClickFocus);
    QObject::connect(lineEdit, &QLineEdit::textChanged, [lineEdit]() {
        lineEdit->setToolTip(lineEdit->text());
    });
    QObject::connect(lineEdit, &QLineEdit::editingFinished, this, &QKeyMapper::onHotKeyLineEditEditingFinished);

    lineEdit = ui->mappingStopKeyLineEdit;
    // left = 390;
    // top = ui->mappingStartKeyLabel->y();
    // lineEdit->setGeometry(QRect(left, top, 110, 21));
    lineEdit->setFocusPolicy(Qt::ClickFocus);
    QObject::connect(lineEdit, &QLineEdit::textChanged, [lineEdit]() {
        lineEdit->setToolTip(lineEdit->text());
    });
    QObject::connect(lineEdit, &QLineEdit::editingFinished, this, &QKeyMapper::onHotKeyLineEditEditingFinished);
}

void QKeyMapper::initGyro2MouseSpinBoxes()
{
    // Set MinXSensitivity & MaxXSensitivity SpinBox range with current value
    ui->Gyro2MouseMaxXSensSpinBox->setMinimum(ui->Gyro2MouseMinXSensSpinBox->value());
    ui->Gyro2MouseMinXSensSpinBox->setMaximum(ui->Gyro2MouseMaxXSensSpinBox->value());
    // Set MinYSensitivity & MaxYSensitivity SpinBox range with current value
    ui->Gyro2MouseMaxYSensSpinBox->setMinimum(ui->Gyro2MouseMinYSensSpinBox->value());
    ui->Gyro2MouseMinYSensSpinBox->setMaximum(ui->Gyro2MouseMaxYSensSpinBox->value());
    // Set MinThreshold & MaxThreshold SpinBox range with current value
    ui->Gyro2MouseMaxThresholdSpinBox->setMinimum(ui->Gyro2MouseMinThresholdSpinBox->value());
    ui->Gyro2MouseMinThresholdSpinBox->setMaximum(ui->Gyro2MouseMaxThresholdSpinBox->value());

    // Connect MinXSensitivity & MaxXSensitivity SpinBox
    QObject::connect(ui->Gyro2MouseMinXSensSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double minValue){
        ui->Gyro2MouseMaxXSensSpinBox->setMinimum(minValue);
    });
    QObject::connect(ui->Gyro2MouseMaxXSensSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double maxValue){
        ui->Gyro2MouseMinXSensSpinBox->setMaximum(maxValue);
    });

    // Connect MinYSensitivity & MaxYSensitivity SpinBox
    QObject::connect(ui->Gyro2MouseMinYSensSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double minValue){
        ui->Gyro2MouseMaxYSensSpinBox->setMinimum(minValue);
    });
    QObject::connect(ui->Gyro2MouseMaxYSensSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double maxValue){
        ui->Gyro2MouseMinYSensSpinBox->setMaximum(maxValue);
    });

    // Connect MinThreshold & MaxThreshold SpinBox
    QObject::connect(ui->Gyro2MouseMinThresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double minValue){
        ui->Gyro2MouseMaxThresholdSpinBox->setMinimum(minValue);
    });
    QObject::connect(ui->Gyro2MouseMaxThresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double maxValue){
        ui->Gyro2MouseMinThresholdSpinBox->setMaximum(maxValue);
    });
}

#if 0
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
#endif

void QKeyMapper::updateWindowSwitchKeyString(const QString &keystring)
{
    if (s_WindowSwitchKeyString != keystring) {
        s_WindowSwitchKeyString = keystring;
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[updateWindowSwitchKeyString]" << "s_WindowSwitchKeyString update ->" << keystring;
#endif
    }
}

void QKeyMapper::updateMappingStartKeyString(const QString &keystring)
{
    if (s_MappingStartKeyString != keystring) {
        s_MappingStartKeyString = keystring;
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[updateMappingStartKeyString]" << "s_MappingStartKeyString update ->" << keystring;
#endif
    }
}

void QKeyMapper::updateMappingStopKeyString(const QString &keystring)
{
    if (s_MappingStopKeyString != keystring) {
        s_MappingStopKeyString = keystring;
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[updateMappingStopKeyString]" << "s_MappingStopKeyString update ->" << keystring;
#endif
    }
}

#if 0
void QKeyMapper::initOriginalKeySeqEdit()
{
    m_originalKeySeqEdit->setObjectName(QStringLiteral("originalKeySeqEdit"));
    // int left = m_orikeyComboBox->x();
    // int top = ui->waitTimeSpinBox->y();
    int width = m_orikeyComboBox->width();
    int height = m_orikeyComboBox->height();
    m_originalKeySeqEdit->setGeometry(QRect(378, 490, width, height));
    m_originalKeySeqEdit->setFocusPolicy(Qt::ClickFocus);
}
#endif

void QKeyMapper::initCombinationKeyLineEdit()
{
    int left = m_orikeyComboBox->x();
    int top = ui->orikeySeqLabel->y();
    int width = m_orikeyComboBox->width();
    int height = m_orikeyComboBox->height();
    ui->combinationKeyLineEdit->setGeometry(QRect(left, top, width, height));
    ui->combinationKeyLineEdit->setFocusPolicy(Qt::ClickFocus);
    QLineEdit *lineEdit = ui->combinationKeyLineEdit;
    QObject::connect(lineEdit, &QLineEdit::textChanged, [lineEdit]() {
        lineEdit->setToolTip(lineEdit->text());
    });
}

void QKeyMapper::updateOriginalKeyListComboBox()
{
    bool isMouseKeys_Selected = ui->oriList_SelectMouseButton->isChecked();
    bool isKeyboardKeys_Selected = ui->oriList_SelectKeyboardButton->isChecked();
    bool isGamepadKeys_Selected = ui->oriList_SelectGamepadButton->isChecked();
    bool isFunctionKeys_Selected = ui->oriList_SelectFunctionButton->isChecked();

    m_orikeyComboBox->clear();
    m_orikeyComboBox->addItem(QString());
    if (isKeyboardKeys_Selected || isMouseKeys_Selected || isGamepadKeys_Selected || isFunctionKeys_Selected) {
        m_orikeyComboBox->clear();
        m_orikeyComboBox->addItem(QString());
        if (isMouseKeys_Selected) {
            QIcon icon = QIcon(":/mouse.png");
            const QStringList keyList = QKeyMapper::s_OriginalKeysCategoryMap.value(KEY_TYPE_MOUSE);
            for (const QString &key : keyList) {
                m_orikeyComboBox->addItem(icon, key);
            }
        }
        if (isKeyboardKeys_Selected) {
            QIcon icon = QIcon(":/keyboard.png");
            const QStringList keyList = QKeyMapper::s_OriginalKeysCategoryMap.value(KEY_TYPE_KEYBOARD);
            for (const QString &key : keyList) {
                m_orikeyComboBox->addItem(icon, key);
            }
        }
        if (isGamepadKeys_Selected) {
            QIcon icon = QIcon(":/gamepad.png");
            const QStringList keyList = QKeyMapper::s_OriginalKeysCategoryMap.value(KEY_TYPE_GAMEPAD);
            for (const QString &key : keyList) {
                m_orikeyComboBox->addItem(icon, key);
            }
        }
        if (isFunctionKeys_Selected) {
            QIcon icon = QIcon(":/function.png");
            const QStringList keyList = QKeyMapper::s_OriginalKeysCategoryMap.value(KEY_TYPE_FUNCTION);
            for (const QString &key : keyList) {
                m_orikeyComboBox->addItem(icon, key);
            }
        }
    }

    if (m_ItemSetupDialog != Q_NULLPTR) {
        m_ItemSetupDialog->updateOriginalKeyListComboBox();
    }
}

void QKeyMapper::updateMappingKeyListComboBox()
{
    bool isMouseKeys_Selected = ui->mapList_SelectMouseButton->isChecked();
    bool isKeyboardKeys_Selected = ui->mapList_SelectKeyboardButton->isChecked();
    bool isGamepadKeys_Selected = ui->mapList_SelectGamepadButton->isChecked();
    bool isFunctionKeys_Selected = ui->mapList_SelectFunctionButton->isChecked();

    m_mapkeyComboBox->clear();
    m_mapkeyComboBox->addItem(QString());

    const QIcon &common_icon = QKeyMapper::s_Icon_Blank;
    const QStringList common_keyList = QKeyMapper::s_MappingKeysCategoryMap.value(KEY_TYPE_COMMON);
    for (const QString &key : common_keyList) {
        m_mapkeyComboBox->addItem(common_icon, key);
    }

    if (isKeyboardKeys_Selected || isMouseKeys_Selected || isGamepadKeys_Selected || isFunctionKeys_Selected) {
        if (isMouseKeys_Selected) {
            QIcon icon = QIcon(":/mouse.png");
            const QStringList keyList = QKeyMapper::s_MappingKeysCategoryMap.value(KEY_TYPE_MOUSE);
            for (const QString &key : keyList) {
                m_mapkeyComboBox->addItem(icon, key);
            }
        }
        if (isKeyboardKeys_Selected) {
            QIcon icon = QIcon(":/keyboard.png");
            const QStringList keyList = QKeyMapper::s_MappingKeysCategoryMap.value(KEY_TYPE_KEYBOARD);
            for (const QString &key : keyList) {
                m_mapkeyComboBox->addItem(icon, key);
            }
        }
        if (isGamepadKeys_Selected) {
            QIcon icon = QIcon(":/gamepad.png");
            const QStringList keyList = QKeyMapper::s_MappingKeysCategoryMap.value(KEY_TYPE_GAMEPAD);
            for (const QString &key : keyList) {
                m_mapkeyComboBox->addItem(icon, key);
            }
        }
        if (isFunctionKeys_Selected) {
            QIcon icon = QIcon(":/function.png");
            const QStringList keyList = QKeyMapper::s_MappingKeysCategoryMap.value(KEY_TYPE_FUNCTION);
            for (const QString &key : keyList) {
                m_mapkeyComboBox->addItem(icon, key);
            }
        }
    }

    if (m_ItemSetupDialog != Q_NULLPTR) {
        m_ItemSetupDialog->updateMappingKeyListComboBox();
    }
}

void QKeyMapper::setKeyMappingTabWidgetCurrentIndex(int index)
{
    if (0 <= index && index < m_KeyMappingTabWidget->count()) {
        if (m_KeyMappingTabWidget->currentIndex() != index) {
            m_KeyMappingTabWidget->setCurrentIndex(index);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[setKeyMappingTabWidgetCurrentIndex]" << "set m_KeyMappingTabWidget index :" << index;
#endif
    }
}

void QKeyMapper::forceSwitchKeyMappingTabWidgetIndex(int index)
{
    m_KeyMappingTabWidget->blockSignals(true);
    disconnectKeyMappingDataTableConnection();
    setKeyMappingTabWidgetCurrentIndex(index);
    switchKeyMappingTabIndex(index);
    updateKeyMappingDataTableConnection();
    m_KeyMappingTabWidget->blockSignals(false);
}

void QKeyMapper::refreshKeyMappingDataTableByTabIndex(int tabindex)
{
    if (0 <= tabindex && tabindex < QKeyMapper::s_KeyMappingTabInfoList.size()) {
        KeyMappingDataTableWidget *mappingDataTable = s_KeyMappingTabInfoList.at(tabindex).KeyMappingDataTable;
        QList<MAP_KEYDATA> *mappingDataList = s_KeyMappingTabInfoList.at(tabindex).KeyMappingData;

        refreshKeyMappingDataTable(mappingDataTable, mappingDataList);
    }
}

void QKeyMapper::refreshKeyMappingDataTable(KeyMappingDataTableWidget *mappingDataTable, QList<MAP_KEYDATA> *mappingDataList)
{
    mappingDataTable->clearContents();
    mappingDataTable->setRowCount(0);

    if (false == mappingDataList->isEmpty()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshKeyMappingDataTable]" << "mappingDataList Start >>>";
#endif
        int rowindex = 0;
        mappingDataTable->setRowCount(mappingDataList->size());
        for (const MAP_KEYDATA &keymapdata : std::as_const(*mappingDataList))
        {
            bool disable_burst = false;
            bool disable_lock = false;

#ifdef VIGEM_CLIENT_SUPPORT
            if (keymapdata.Original_Key == VJOY_MOUSE2LS_STR || keymapdata.Original_Key == VJOY_MOUSE2RS_STR) {
                disable_burst = true;
                disable_lock = true;
            }

            if (keymapdata.Original_Key == JOY_LS2VJOYLS_STR
                || keymapdata.Original_Key == JOY_RS2VJOYRS_STR
                || keymapdata.Original_Key == JOY_LS2VJOYRS_STR
                || keymapdata.Original_Key == JOY_RS2VJOYLS_STR
                || keymapdata.Original_Key == JOY_LT2VJOYLT_STR
                || keymapdata.Original_Key == JOY_RT2VJOYRT_STR) {
                disable_burst = true;
                disable_lock = true;
            }
#endif
            if (keymapdata.Original_Key == JOY_LS2MOUSE_STR
                || keymapdata.Original_Key == JOY_RS2MOUSE_STR
                || keymapdata.Original_Key == JOY_GYRO2MOUSE_STR) {
                disable_burst = true;
                disable_lock = true;
            }

            if (keymapdata.Original_Key.contains(MOUSE_WHEEL_STR)) {
                disable_burst = true;
                disable_lock = true;
            }

            if (keymapdata.Mapping_Keys.size() > 1) {
                disable_burst = true;
                disable_lock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().contains(SEPARATOR_WAITTIME)) {
                disable_burst = true;
                disable_lock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().contains(KEY_BLOCKED_STR)) {
                disable_burst = true;
                disable_lock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().startsWith(KEY2MOUSE_PREFIX)) {
                disable_burst = true;
                disable_lock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().startsWith(CROSSHAIR_PREFIX)) {
                disable_burst = true;
                // disable_lock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().startsWith(FUNC_PREFIX)) {
                disable_burst = true;
                disable_lock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().contains(MOUSE2VJOY_HOLD_KEY_STR)) {
                disable_burst = true;
                // disable_lock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().startsWith(GYRO2MOUSE_PREFIX)) {
                disable_burst = true;
                // disable_lock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().startsWith(VJOY_LS_RADIUS_STR)
                || keymapdata.Mapping_Keys.constFirst().startsWith(VJOY_RS_RADIUS_STR)) {
                disable_burst = true;
                // disable_lock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().contains(VJOY_LT_BRAKE_STR)
                || keymapdata.Mapping_Keys.constFirst().contains(VJOY_RT_BRAKE_STR)
                || keymapdata.Mapping_Keys.constFirst().contains(VJOY_LT_ACCEL_STR)
                || keymapdata.Mapping_Keys.constFirst().contains(VJOY_RT_ACCEL_STR)) {
                disable_burst = true;
                disable_lock = true;
            }

            /* ORIGINAL_KEY_COLUMN */
            QString mapdata_note = keymapdata.Note;
            QString orikey_withnote;
            if (ui->showNotesButton->isChecked() && !mapdata_note.isEmpty()) {
                orikey_withnote = QString(ORIKEY_WITHNOTE_FORMAT).arg(keymapdata.Original_Key, mapdata_note);
            }
            else {
                orikey_withnote = keymapdata.Original_Key;
            }
            if (keymapdata.Original_Key.contains(SEPARATOR_PLUS)) {
                orikey_withnote = QString(PREFIX_SHORTCUT) + orikey_withnote;
            }
            QTableWidgetItem *ori_TableItem = new QTableWidgetItem(orikey_withnote);
            ori_TableItem->setToolTip(orikey_withnote);
            ori_TableItem->setFlags(ori_TableItem->flags() & ~Qt::ItemIsEditable); // Make read-only
            if (keymapdata.PassThrough) {
                ori_TableItem->setForeground(QBrush(STATUS_ON_COLOR));
            }
            mappingDataTable->setItem(rowindex, ORIGINAL_KEY_COLUMN  , ori_TableItem);

            /* MAPPING_KEY_COLUMN */
            QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
            QTableWidgetItem *mapping_TableItem = new QTableWidgetItem(mappingkeys_str);
            mapping_TableItem->setToolTip(mappingkeys_str);
            mapping_TableItem->setFlags(mapping_TableItem->flags() & ~Qt::ItemIsEditable); // Make read-only
            mappingDataTable->setItem(rowindex, MAPPING_KEY_COLUMN   , mapping_TableItem);

            /* BURST_MODE_COLUMN */
            QTableWidgetItem *burstCheckBox = new QTableWidgetItem();
            if (keymapdata.Burst == true) {
                burstCheckBox->setCheckState(Qt::Checked);
            }
            else {
                burstCheckBox->setCheckState(Qt::Unchecked);
            }

#ifdef VIGEM_CLIENT_SUPPORT
            if (disable_burst) {
                burstCheckBox->setFlags(burstCheckBox->flags() & ~Qt::ItemIsEnabled);
            }
#endif
            mappingDataTable->setItem(rowindex, BURST_MODE_COLUMN    , burstCheckBox);

            /* LOCK_COLUMN */
            QTableWidgetItem *lockCheckBox = new QTableWidgetItem();
            if (keymapdata.Lock == true) {
                lockCheckBox->setCheckState(Qt::Checked);
            }
            else {
                lockCheckBox->setCheckState(Qt::Unchecked);
            }

#ifdef VIGEM_CLIENT_SUPPORT
            if (disable_lock) {
                lockCheckBox->setFlags(lockCheckBox->flags() & ~Qt::ItemIsEnabled);
            }
#endif
            mappingDataTable->setItem(rowindex, LOCK_COLUMN    , lockCheckBox);

            /* CATEGORY_COLUMN */
            QTableWidgetItem *categoryItem = new QTableWidgetItem(keymapdata.Category);
            categoryItem->setToolTip(keymapdata.Category);
            // Category column should be editable when visible
            if (mappingDataTable->isCategoryColumnVisible()) {
                categoryItem->setFlags(categoryItem->flags() | Qt::ItemIsEditable);
            } else {
                categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsEditable);
            }
            mappingDataTable->setItem(rowindex, CATEGORY_COLUMN, categoryItem);

            rowindex += 1;

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[refreshKeyMappingDataTable] " << keymapdata.Original_Key << " -> " << keymapdata.Mapping_Keys << ", PureMappingKeys -> " << keymapdata.Pure_MappingKeys;
#endif
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshKeyMappingDataTable]" << "mappingDataList End <<<";
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshKeyMappingDataTable]" << "Empty mappingDataList";
#endif
    }

    resizeKeyMappingDataTableColumnWidth(mappingDataTable);

    // Update category filter ComboBox if category column is visible
    if (mappingDataTable->isCategoryColumnVisible()) {
        updateCategoryFilterComboBox();
    }
}

void QKeyMapper::updateKeyMappingTabWidgetTabDisplay(int tabindex)
{
    if ((tabindex < 0) || (tabindex >= m_KeyMappingTabWidget->count()) || (tabindex >= s_KeyMappingTabInfoList.size())) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[updateKeyMappingTabWidgetTabDisplay] Invalid index : " << tabindex << ", ValidTabWidgetCount:" << m_KeyMappingTabWidget->count() << ", TabInfoListSize:" << s_KeyMappingTabInfoList.size();
#endif
        return;
    }

    const QString tab_hotkey = s_KeyMappingTabInfoList.at(tabindex).TabHotkey;
    const QString tab_name = s_KeyMappingTabInfoList.at(tabindex).TabName;
    // const QColor tab_color = s_KeyMappingTabInfoList.at(tabindex).TabFontColor;
    const QString tab_customimage_path = s_KeyMappingTabInfoList.at(tabindex).TabCustomImage_Path;
    if (false == tab_hotkey.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[updateKeyMappingTabWidgetTabDisplay] Tabindex[" << tabindex << "] Set Tabbar textcolor & tooltips, TabHotkey : " << s_KeyMappingTabInfoList.at(tabindex).TabHotkey;
#endif

        QString tabText = QString(PREFIX_SHORTCUT) + tab_name;
        m_KeyMappingTabWidget->tabBar()->setTabText(tabindex, tabText);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[updateKeyMappingTabWidgetTabDisplay] Tabindex[" << tabindex << "] Clear Tabbar prefix & tooltips, TabHotkey : " << s_KeyMappingTabInfoList.at(tabindex).TabHotkey;
#endif

        m_KeyMappingTabWidget->tabBar()->setTabText(tabindex, tab_name);
    }

    // m_KeyMappingTabWidget->tabBar()->setTabTextColor(tabindex, tab_color);
    QIcon icon_loaded;
    if (!tab_customimage_path.isEmpty()) {
        icon_loaded = QIcon(tab_customimage_path);
    }
    m_KeyMappingTabWidget->tabBar()->setTabIcon(tabindex, icon_loaded);

    QString tooltip_str;
    if (false == tab_hotkey.isEmpty()) {
        tooltip_str = tr("Hotkey : %1").arg(tab_hotkey);
    }
    m_KeyMappingTabWidget->tabBar()->setTabToolTip(tabindex, tooltip_str);
}

void QKeyMapper::refreshAllKeyMappingTagWidget()
{
    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index) {
        KeyMappingDataTableWidget *mappingDataTable = s_KeyMappingTabInfoList.at(index).KeyMappingDataTable;
        QList<MAP_KEYDATA> *mappingDataList = s_KeyMappingTabInfoList.at(index).KeyMappingData;

        refreshKeyMappingDataTable(mappingDataTable, mappingDataList);
    }

    updateMousePointsList();
    updateCategoryFilterByShowCategoryState();
}

void QKeyMapper::updateMousePointsList()
{
    if (KeyMappingDataList->isEmpty()) {
        ScreenMousePointsList.clear();
        WindowMousePointsList.clear();
        return;
    }

    static QRegularExpression mousepoint_regex(R"(Mouse-(L|R|M|X1|X2|Move)(:W)?(:BG)?\((\d+),(\d+)\))");
    QRegularExpressionMatch mousepoint_match;
    ScreenMousePointsList.clear();
    WindowMousePointsList.clear();

    for (const MAP_KEYDATA &keymapdata : std::as_const(*KeyMappingDataList))
    {
        QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);

        mousepoint_match = mousepoint_regex.match(mappingkeys_str);
        while (mousepoint_match.hasMatch())
        {
            QString ori_key = keymapdata.Original_Key;
            QString map_key = mousepoint_match.captured(0);
            bool isWindowPoint = !mousepoint_match.captured(2).isEmpty();
            QString x_str = mousepoint_match.captured(4);
            QString y_str = mousepoint_match.captured(5);
            int x = x_str.isEmpty() ? -1 : x_str.toInt();
            int y = y_str.isEmpty() ? -1 : y_str.toInt();

            QList<MousePoint_Info>& targetList = isWindowPoint ? WindowMousePointsList : ScreenMousePointsList;

            // Check if the same "Mouse-?(int,int)" string already exists in the target list
            bool alreadyExists = false;
            for (const MousePoint_Info &info : targetList)
            {
                if (info.map_key == map_key)
                {
                    alreadyExists = true;
                    break;
                }
            }

            // Add the new MousePoint_Info if it doesn't already exist
            if (!alreadyExists)
            {
                MousePoint_Info info;
                info.ori_key = ori_key;
                info.map_key = map_key;
                info.x = x;
                info.y = y;
                targetList.append(info);
            }

            int matchEnd = mousepoint_match.capturedEnd();
            mappingkeys_str = mappingkeys_str.mid(matchEnd);
            mousepoint_match = mousepoint_regex.match(mappingkeys_str);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[updateMousePointsList]" << "Updated ScreenMousePointsList ->" << ScreenMousePointsList;
    qDebug() << "[updateMousePointsList]" << "Updated WindowMousePointsList ->" << WindowMousePointsList;
#endif
}

void QKeyMapper::reloadUILanguage()
{
    int languageIndex = ui->languageComboBox->currentIndex();

    if (LANGUAGE_ENGLISH == languageIndex) {
        changeLanguage(LANGUAGECODE_ENGLISH);
        setControlFontEnglish();
    }
    else if (LANGUAGE_JAPANESE == languageIndex) {
        changeLanguage(LANGUAGECODE_JAPANESE);
        setControlFontJapanese();
    }
    else {
        changeLanguage(LANGUAGECODE_CHINESE);
        setControlFontChinese();
    }
    setUILanguage(languageIndex);

    // QRect curGeometry = ui->virtualGamepadTypeComboBox->geometry();
    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     curGeometry.moveLeft(VIRTUALGAMEPADTYPECOMBOBOX_X - 10);
    //     ui->virtualGamepadTypeComboBox->setGeometry(curGeometry);
    // }
    // else {
    //     curGeometry.moveLeft(VIRTUALGAMEPADTYPECOMBOBOX_X);
    //     ui->virtualGamepadTypeComboBox->setGeometry(curGeometry);
    // }

#ifdef VIGEM_CLIENT_SUPPORT
    emit updateViGEmBusStatus_Signal();
#endif
    emit updateMultiInputStatus_Signal();
}

void QKeyMapper::setUILanguage(int languageindex)
{
    if (m_KeyMapStatus != KEYMAP_IDLE) {
        ui->keymapButton->setText(tr("MappingStop"));
    }
    else {
        ui->keymapButton->setText(tr("MappingStart"));
    }

    // ui->refreshButton->setText(REFRESHBUTTON_CHINESE);
    ui->savemaplistButton->setText(tr("SaveSetting"));
    ui->savemaplistButton->setToolTip(tr("Hotkey : L-Ctrl+S"));
    ui->addTabButton->setText(tr("AddTab"));
    ui->deleteSelectedButton->setText(tr("Delete"));
    ui->clearallButton->setText(tr("Clear"));
    ui->processListButton->setText(tr("ProcessList"));
    ui->showNotesButton->setText(tr("ShowNotes"));
    ui->showCategoryButton->setText(tr("ShowCategory"));

    // Update category filter controls text
    updateCategoryFilterComboBox();

    ui->addmapdataButton->setText(tr("ADD"));
    // ui->processCheckBox->setText(tr("Process"));
    // ui->titleCheckBox->setText(tr("Title"));
    ui->processLabel->setText(tr("Process"));
    ui->windowTitleLabel->setText(tr("Title"));
    ui->restoreProcessPathButton->setText(tr("Restore"));
    ui->settingNameLabel->setText(tr("SettingName"));
    ui->descriptionLabel->setText(tr("Description"));
    if (GLOBALSETTING_INDEX == ui->settingselectComboBox->currentIndex()) {
        ui->descriptionLineEdit->setText(tr("Global keymapping setting"));
    }
    QString globalSettingNameWithDescStr = QString(SETTING_DESCRIPTION_FORMAT).arg(GROUPNAME_GLOBALSETTING, tr("Global keymapping setting"));
    ui->settingselectComboBox->setItemText(GLOBALSETTING_INDEX, globalSettingNameWithDescStr);
    ui->oriList_SelectKeyboardButton->setToolTip(tr("Keyboard Keys"));
    ui->oriList_SelectMouseButton->setToolTip(tr("Mouse Keys"));
    ui->oriList_SelectGamepadButton->setToolTip(tr("Gamepad Keys"));
    ui->oriList_SelectFunctionButton->setToolTip(tr("Function Keys"));
    ui->mapList_SelectKeyboardButton->setToolTip(tr("Keyboard Keys"));
    ui->mapList_SelectMouseButton->setToolTip(tr("Mouse Keys"));
    ui->mapList_SelectGamepadButton->setToolTip(tr("Gamepad Keys"));
    ui->mapList_SelectFunctionButton->setToolTip(tr("Function Keys"));
    ui->orikeyLabel->setText(tr("OriKey"));
    ui->orikeySeqLabel->setText(tr("OriKeySeq"));
    ui->mapkeyLabel->setText(tr("MapKey"));
    // ui->burstpressLabel->setText(BURSTPRESSLABEL_CHINESE);
    // ui->burstreleaseLabel->setText(BURSTRELEASE_CHINESE);
    // ui->burstpress_msLabel->setText(BURSTPRESS_MSLABEL_CHINESE);
    // ui->burstrelease_msLabel->setText(BURSTRELEASE_MSLABEL_CHINESE);
    ui->waitTimeLabel->setText(tr("Delay"));
    ui->waitTimeSpinBox->setSuffix(tr(" ms"));
    ui->pressTimeSpinBox->setSuffix(tr(" ms"));
    ui->pushLevelLabel->setText(tr("PushLevel"));
    ui->sendTextLabel->setText(tr("Text"));
    ui->keyPressTypeComboBox->clear();
    ui->keyPressTypeComboBox->addItem(tr("LongPress"));
    ui->keyPressTypeComboBox->addItem(tr("DoublePress"));
    ui->pointLabel->setText(tr("Point"));
    // ui->waitTime_msLabel->setText(WAITTIME_MSLABEL_CHINESE);
    ui->mouseXSpeedLabel->setText(tr("X Speed"));
    ui->mouseYSpeedLabel->setText(tr("Y Speed"));
    // ui->settingselectLabel->setText(SETTINGSELECTLABEL_CHINESE);
    ui->removeSettingButton->setText(tr("Remove"));
    // ui->disableWinKeyCheckBox->setText(DISABLEWINKEYCHECKBOX_CHINESE);
    ui->dataPortLabel->setText(tr("DataPort"));
    ui->brakeThresholdLabel->setText(tr("BrakeValue"));
    ui->accelThresholdLabel->setText(tr("AccelValue"));
    ui->autoStartMappingCheckBox->setText(tr("Auto Match Foreground"));
    ui->sendToSameTitleWindowsCheckBox->setText(tr("Send To Same Windows"));
    ui->acceptVirtualGamepadInputCheckBox->setText(tr("Accept Virtual Gamepad Input"));
    ui->autoStartupCheckBox->setText(tr("Auto Startup"));
    ui->startupMinimizedCheckBox->setText(tr("Startup Minimized"));
    ui->startupAutoMonitoringCheckBox->setText(tr("Startup AutoMonitoring"));
    ui->soundEffectCheckBox->setText(tr("Sound"));
    ui->notificationLabel->setText(tr("Notification"));
    ui->languageLabel->setText(tr("Language"));
    ui->updateSiteLabel->setText(tr("UpdateSite"));
    ui->windowswitchkeyLabel->setText(tr("ShowHideKey"));
    ui->checkUpdateButton->setText(tr("Check Updates"));
    ui->mappingStartKeyLabel->setText(tr("MappingStart"));
    ui->mappingStopKeyLabel->setText(tr("MappingStop"));
    ui->Gyro2MouseXSpeedLabel->setText(tr("Gyro2Mouse X Speed"));
    ui->Gyro2MouseYSpeedLabel->setText(tr("Gyro2Mouse Y Speed"));
    ui->Gyro2MouseMinXSensLabel->setText(tr("MinXSens"));
    ui->Gyro2MouseMinYSensLabel->setText(tr("MinYSens"));
    ui->Gyro2MouseMaxXSensLabel->setText(tr("MaxXSens"));
    ui->Gyro2MouseMaxYSensLabel->setText(tr("MaxYSens"));
    ui->Gyro2MouseMinThresholdLabel->setText(tr("MinThres"));
    ui->Gyro2MouseMaxThresholdLabel->setText(tr("MaxThres"));
    ui->Gyro2MouseAdvancedSettingButton->setText(tr("Advanced"));
    ui->selectTrayIconButton->setText(tr("Select Tray Icon"));
    ui->notificationAdvancedSettingButton->setText(tr("Noti Advanced"));
    ui->ProcessIconAsTrayIconCheckBox->setText(tr("ProcessIcon as TrayIcon"));

    ui->notificationComboBox->setItemText(NOTIFICATION_POSITION_NONE,           tr("None"));
    ui->notificationComboBox->setItemText(NOTIFICATION_POSITION_TOP_LEFT,       tr("Top Left"));
    ui->notificationComboBox->setItemText(NOTIFICATION_POSITION_TOP_CENTER,     tr("Top Center"));
    ui->notificationComboBox->setItemText(NOTIFICATION_POSITION_TOP_RIGHT,      tr("Top Right"));
    ui->notificationComboBox->setItemText(NOTIFICATION_POSITION_BOTTOM_LEFT,    tr("Bottom Left"));
    ui->notificationComboBox->setItemText(NOTIFICATION_POSITION_BOTTOM_CENTER,  tr("Bottom Center"));
    ui->notificationComboBox->setItemText(NOTIFICATION_POSITION_BOTTOM_RIGHT,   tr("Bottom Right"));

    ui->checkProcessComboBox->setItemText(WINDOWINFO_MATCH_INDEX_IGNORE,        tr("Ignore"));
    ui->checkProcessComboBox->setItemText(WINDOWINFO_MATCH_INDEX_EQUALS,        tr("Equals"));
    ui->checkProcessComboBox->setItemText(WINDOWINFO_MATCH_INDEX_CONTAINS,      tr("Contains"));
    ui->checkProcessComboBox->setItemText(WINDOWINFO_MATCH_INDEX_STARTSWITH,    tr("StartsWith"));
    ui->checkProcessComboBox->setItemText(WINDOWINFO_MATCH_INDEX_ENDSWITH,      tr("EndsWith"));
    ui->checkWindowTitleComboBox->setItemText(WINDOWINFO_MATCH_INDEX_IGNORE,    tr("Ignore"));
    ui->checkWindowTitleComboBox->setItemText(WINDOWINFO_MATCH_INDEX_EQUALS,    tr("Equals"));
    ui->checkWindowTitleComboBox->setItemText(WINDOWINFO_MATCH_INDEX_CONTAINS,  tr("Contains"));
    ui->checkWindowTitleComboBox->setItemText(WINDOWINFO_MATCH_INDEX_STARTSWITH,tr("StartsWith"));
    ui->checkWindowTitleComboBox->setItemText(WINDOWINFO_MATCH_INDEX_ENDSWITH,  tr("EndsWith"));

    QTabWidget *tabWidget = ui->settingTabWidget;
    tabWidget->setTabText(tabWidget->indexOf(ui->windowinfo),       tr("WindowInfo")    );
    tabWidget->setTabText(tabWidget->indexOf(ui->general),          tr("General")       );
    tabWidget->setTabText(tabWidget->indexOf(ui->mapping),          tr("Mapping")       );
    tabWidget->setTabText(tabWidget->indexOf(ui->virtual_gamepad),  tr("VirtualGamepad"));
    tabWidget->setTabText(tabWidget->indexOf(ui->gyro2mouse),       tr("Gyro2Mouse")    );
    tabWidget->setTabText(tabWidget->indexOf(ui->multi_input),      tr("Multi-Input")   );
    tabWidget->setTabText(tabWidget->indexOf(ui->forza),            tr("Forza")         );

    tabWidget->setTabToolTip(tabWidget->indexOf(ui->windowinfo),       tr("WindowInfo setting tab tooltip.")    );
    tabWidget->setTabToolTip(tabWidget->indexOf(ui->general),          tr("General setting tab tooltip.")       );
    tabWidget->setTabToolTip(tabWidget->indexOf(ui->mapping),          tr("Mapping setting tab tooltip.")       );
    tabWidget->setTabToolTip(tabWidget->indexOf(ui->virtual_gamepad),  tr("VirtualGamepad setting tab tooltip."));
    tabWidget->setTabToolTip(tabWidget->indexOf(ui->gyro2mouse),       tr("Gyro2Mouse setting tab tooltip.")    );
    tabWidget->setTabToolTip(tabWidget->indexOf(ui->multi_input),      tr("Multi-Input setting tab tooltip")    );
    tabWidget->setTabToolTip(tabWidget->indexOf(ui->forza),            tr("Forza setting tab tooltip.")         );

#ifdef VIGEM_CLIENT_SUPPORT
    // ui->virtualgamepadGroupBox->setTitle(VIRTUALGAMEPADGROUPBOX_CHINESE);
    ui->enableVirtualJoystickCheckBox->setText(tr("VirtualGamepad"));
    ui->lockCursorCheckBox->setText(tr("Lock Cursor"));
    ui->directModeCheckBox->setText(tr("Direct Mode"));
    ui->vJoyXSensLabel->setText(tr("X Sens"));
    ui->vJoyYSensLabel->setText(tr("Y Sens"));
    ui->vJoyRecenterLabel->setText(tr("Recenter"));
    ui->vJoyRecenterSpinBox->setSuffix(tr(" ms"));
    ui->vJoyRecenterSpinBox->setSpecialValueText(tr("Unrecenter"));
    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == QKeyMapper_Worker::ViGEmClient_getConnectState()) {
        ui->installViGEmBusButton->setText(tr("UninstallViGEm"));
    }
    else {
        ui->installViGEmBusButton->setText(tr("InstallViGEm"));
    }
    // ui->uninstallViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_CHINESE);
#endif
    ui->keyboardSelectLabel->setText(tr("Keyboard"));
    ui->mouseSelectLabel->setText(tr("Mouse"));
    ui->gamepadSelectLabel->setText(tr("Gamepad"));
    // ui->multiInputGroupBox->setTitle(MULTIINPUTGROUPBOX_CHINESE);
    ui->multiInputEnableCheckBox->setText(tr("MultiDevice"));
    ui->filterKeysCheckBox->setText(tr("FilterKeys"));
    ui->multiInputDeviceListButton->setText(tr("DeviceList"));
    if (Interception_Worker::INTERCEPTION_AVAILABLE == Interception_Worker::getInterceptionState()) {
        ui->installInterceptionButton->setText(tr("Uninstall Driver"));
    }
    else {
        ui->installInterceptionButton->setText(tr("Install Driver"));
    }

    /* KeyMappingTabWidget HorizontalHeader Update for current language */
    for (int tabindex = 0; tabindex < m_KeyMappingTabWidget->count(); ++tabindex) {
        KeyMappingDataTableWidget *mappingTable = qobject_cast<KeyMappingDataTableWidget*>(m_KeyMappingTabWidget->widget(tabindex));
        mappingTable->setHorizontalHeaderLabels(QStringList()       << tr("OriginalKey")
                                                                    << tr("MappingKey")
                                                                    << tr("Burst")
                                                                    << tr("Lock")
                                                                    << tr("Category"));
    }

    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << tr("Process")
                                                                    << tr("PID")
                                                                    << tr("Window Title"));

    if (m_deviceListWindow != Q_NULLPTR) {
        m_deviceListWindow->setUILanguage(languageindex);
    }

    if (m_Gyro2MouseOptionDialog != Q_NULLPTR) {
        m_Gyro2MouseOptionDialog->setUILanguage(languageindex);
    }

    if (m_TrayIconSelectDialog != Q_NULLPTR) {
        m_TrayIconSelectDialog->setUILanguage(languageindex);
    }

    if (m_NotificationSetupDialog != Q_NULLPTR) {
        m_NotificationSetupDialog->setUILanguage(languageindex);
    }

    if (m_TableSetupDialog != Q_NULLPTR) {
        m_TableSetupDialog->setUILanguage(languageindex);
    }

    if (m_ItemSetupDialog != Q_NULLPTR) {
        m_ItemSetupDialog->setUILanguage(languageindex);
    }
}

#if 0
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
    ui->savemaplistButton->setToolTip("快捷键 : L-Ctrl+S");
    ui->deleteSelectedButton->setText(DELETEONEBUTTON_CHINESE);
    ui->clearallButton->setText(CLEARALLBUTTON_CHINESE);
    ui->processListButton->setText(PROCESSLISTBUTTON_CHINESE);
    ui->showNotesButton->setText(SHOWNOTESBUTTON_CHINESE);
    ui->addmapdataButton->setText(ADDMAPDATABUTTON_CHINESE);
    ui->processCheckBox->setText(NAMECHECKBOX_CHINESE);
    ui->titleCheckBox->setText(TITLECHECKBOX_CHINESE);
    ui->descriptionLabel->setText(SETTINGDESCLABEL_CHINESE);
    if (GLOBALSETTING_INDEX == ui->settingselectComboBox->currentIndex()) {
        ui->descriptionLineEdit->setText(GLOBALSETTING_DESC_CHINESE);
    }
    QString globalSettingNameWithDescStr = QString(SETTING_DESCRIPTION_FORMAT).arg(GROUPNAME_GLOBALSETTING, GLOBALSETTING_DESC_CHINESE);
    ui->settingselectComboBox->setItemText(GLOBALSETTING_INDEX, globalSettingNameWithDescStr);
    ui->orikeyLabel->setText(ORIKEYLABEL_CHINESE);
    ui->orikeySeqLabel->setText(ORIKEYSEQLABEL_CHINESE);
    ui->mapkeyLabel->setText(MAPKEYLABEL_CHINESE);
    // ui->burstpressLabel->setText(BURSTPRESSLABEL_CHINESE);
    // ui->burstreleaseLabel->setText(BURSTRELEASE_CHINESE);
    // ui->burstpress_msLabel->setText(BURSTPRESS_MSLABEL_CHINESE);
    // ui->burstrelease_msLabel->setText(BURSTRELEASE_MSLABEL_CHINESE);
    ui->waitTimeLabel->setText(WAITTIME_CHINESE);
    ui->waitTimeSpinBox->setSuffix(MILLISECOND_SUFFIX_CHINESE);
    ui->pressTimeSpinBox->setSuffix(MILLISECOND_SUFFIX_CHINESE);
    ui->pushLevelLabel->setText(PUSHLEVEL_CHINESE);
    ui->sendTextLabel->setText(SENDTEXTLABEL_CHINESE);
    ui->keyPressTypeComboBox->clear();
    ui->keyPressTypeComboBox->addItem(LONGPRESS_CHINESE);
    ui->keyPressTypeComboBox->addItem(DOUBLEPRESS_CHINESE);
    ui->pointLabel->setText(POINT_CHINESE);
    // ui->waitTime_msLabel->setText(WAITTIME_MSLABEL_CHINESE);
    ui->mouseXSpeedLabel->setText(MOUSEXSPEEDLABEL_CHINESE);
    ui->mouseYSpeedLabel->setText(MOUSEYSPEEDLABEL_CHINESE);
    // ui->settingselectLabel->setText(SETTINGSELECTLABEL_CHINESE);
    ui->removeSettingButton->setText(REMOVESETTINGBUTTON_CHINESE);
    // ui->disableWinKeyCheckBox->setText(DISABLEWINKEYCHECKBOX_CHINESE);
    ui->dataPortLabel->setText(DATAPORTLABEL_CHINESE);
    ui->brakeThresholdLabel->setText(BRAKETHRESHOLDLABEL_CHINESE);
    ui->accelThresholdLabel->setText(ACCELTHRESHOLDLABEL_CHINESE);
    ui->autoStartMappingCheckBox->setText(AUTOSTARTMAPPINGCHECKBOX_CHINESE);
    ui->sendToSameTitleWindowsCheckBox->setText(SENDTOSAMETITLEWINDOWSCHECKBOX_CHINESE);
    ui->acceptVirtualGamepadInputCheckBox->setText(ACCEPTVIRTUALGAMEPADINPUTCHECKBOX_CHINESE);
    ui->autoStartupCheckBox->setText(AUTOSTARTUPCHECKBOX_CHINESE);
    ui->startupMinimizedCheckBox->setText(STARTUPMINIMIZEDCHECKBOX_CHINESE);
    ui->soundEffectCheckBox->setText(SOUNDEFFECTCHECKBOX_CHINESE);
    ui->notificationLabel->setText(NOTIFICATIONLABEL_CHINESE);
    ui->languageLabel->setText(LANGUAGELABEL_CHINESE);
    ui->updateSiteLabel->setText(UPDATESITELABEL_CHINESE);
    ui->windowswitchkeyLabel->setText(WINDOWSWITCHKEYLABEL_CHINESE);
    ui->checkUpdateButton->setText(CHECKUPDATEBUTTON_CHINESE);
    ui->mappingStartKeyLabel->setText(MAPPINGSTARTKEYLABEL_CHINESE);
    ui->mappingStopKeyLabel->setText(MAPPINGSTOPKEYLABEL_CHINESE);

    int last_notification_position = ui->notificationComboBox->currentIndex();
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[setUILanguage_Chinese]" << "last_notification_position" << last_notification_position;
#endif
    ui->notificationComboBox->clear();
    QStringList positoin_list = QStringList() \
            << POSITION_NONE_STR_CHINESE
            << POSITION_TOP_LEFT_STR_CHINESE
            << POSITION_TOP_CENTER_STR_CHINESE
            << POSITION_TOP_RIGHT_STR_CHINESE
            << POSITION_BOTTOM_LEFT_STR_CHINESE
            << POSITION_BOTTOM_CENTER_STR_CHINESE
            << POSITION_BOTTOM_RIGHT_STR_CHINESE
            ;
    ui->notificationComboBox->addItems(positoin_list);
    ui->notificationComboBox->setCurrentIndex(last_notification_position);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[setUILanguage_Chinese]" << "notificationComboBox->currentIndex()" << ui->notificationComboBox->currentIndex();
#endif

    QTabWidget *tabWidget = ui->settingTabWidget;
    tabWidget->setTabText(tabWidget->indexOf(ui->general),          SETTINGTAB_GENERAL_CHINESE      );
    tabWidget->setTabText(tabWidget->indexOf(ui->mapping),          SETTINGTAB_MAPPING_CHINESE      );
    tabWidget->setTabText(tabWidget->indexOf(ui->virtual_gamepad),  SETTINGTAB_VGAMEPAD_CHINESE     );
    tabWidget->setTabText(tabWidget->indexOf(ui->multi_input),      SETTINGTAB_MULTIINPUT_CHINESE   );
    tabWidget->setTabText(tabWidget->indexOf(ui->forza),            SETTINGTAB_FORZA_CHINESE        );

#ifdef VIGEM_CLIENT_SUPPORT
    // ui->virtualgamepadGroupBox->setTitle(VIRTUALGAMEPADGROUPBOX_CHINESE);
    ui->enableVirtualJoystickCheckBox->setText(ENABLEVIRTUALJOYSTICKCHECKBOX_CHINESE);
    ui->lockCursorCheckBox->setText(LOCKCURSORCHECKBOX_CHINESE);
    ui->directModeCheckBox->setText(DIRECTMODECHECKBOX_CHINESE);
    ui->vJoyXSensLabel->setText(VJOYXSENSLABEL_CHINESE);
    ui->vJoyYSensLabel->setText(VJOYYSENSLABEL_CHINESE);
    ui->vJoyRecenterLabel->setText(VJOYRECENTERLABEL_CHINESE);
    ui->vJoyRecenterSpinBox->setSuffix(MILLISECOND_SUFFIX_CHINESE);
    ui->vJoyRecenterSpinBox->setSpecialValueText(VJOYRECENTERSPINBOX_UNRECENTER_CHINESE);
    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == QKeyMapper_Worker::ViGEmClient_getConnectState()) {
        ui->installViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_CHINESE);
    }
    else {
        ui->installViGEmBusButton->setText(INSTALLVIGEMBUSBUTTON_CHINESE);
    }
    // ui->uninstallViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_CHINESE);
#endif
    ui->keyboardSelectLabel->setText(KEYBOARDSELECTLABEL_CHINESE);
    ui->mouseSelectLabel->setText(MOUSESELECTLABEL_CHINESE);
    ui->gamepadSelectLabel->setText(GAMEPADSELECTLABEL_CHINESE);
    // ui->multiInputGroupBox->setTitle(MULTIINPUTGROUPBOX_CHINESE);
    ui->multiInputEnableCheckBox->setText(MULTIINPUTENABLECHECKBOX_CHINESE);
    ui->filterKeysCheckBox->setText(FILTERKEYSCHECKBOX_CHINESE);
    ui->multiInputDeviceListButton->setText(MULTIINPUTDEVICELISTBUTTON_CHINESE);
    if (Interception_Worker::INTERCEPTION_AVAILABLE == Interception_Worker::getInterceptionState()) {
        ui->installInterceptionButton->setText(UNINSTALLINTERCEPTIONBUTTON_CHINESE);
    }
    else {
        ui->installInterceptionButton->setText(INSTALLINTERCEPTIONBUTTON_CHINESE);
    }

    /* KeyMappingTabWidget HorizontalHeader Update for current language */
    for (int tabindex = 0; tabindex < m_KeyMappingTabWidget->count(); ++tabindex) {
        KeyMappingDataTableWidget *mappingTable = qobject_cast<KeyMappingDataTableWidget*>(m_KeyMappingTabWidget->widget(tabindex));
        mappingTable->setHorizontalHeaderLabels(QStringList()   << KEYMAPDATATABLE_COL1_CHINESE
                                                                << KEYMAPDATATABLE_COL2_CHINESE
                                                                << KEYMAPDATATABLE_COL3_CHINESE
                                                                << KEYMAPDATATABLE_COL4_CHINESE
                                                                << KEYMAPDATATABLE_COL5_CHINESE);
    }

    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << PROCESSINFOTABLE_COL1_CHINESE
                                                                  << PROCESSINFOTABLE_COL2_CHINESE
                                                                  << PROCESSINFOTABLE_COL3_CHINESE );

    if (m_deviceListWindow != Q_NULLPTR) {
        m_deviceListWindow->setUILanguage(LANGUAGE_CHINESE);
    }

    if (m_TableSetupDialog != Q_NULLPTR) {
        m_TableSetupDialog->setUILanguage(LANGUAGE_CHINESE);
    }

    if (m_ItemSetupDialog != Q_NULLPTR) {
        m_ItemSetupDialog->setUILanguage(LANGUAGE_CHINESE);
    }
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
    ui->savemaplistButton->setToolTip("Hotkey : L-Ctrl+S");
    ui->deleteSelectedButton->setText(DELETEONEBUTTON_ENGLISH);
    ui->clearallButton->setText(CLEARALLBUTTON_ENGLISH);
    ui->processListButton->setText(PROCESSLISTBUTTON_ENGLISH);
    ui->showNotesButton->setText(SHOWNOTESBUTTON_ENGLISH);
    ui->addmapdataButton->setText(ADDMAPDATABUTTON_ENGLISH);
    ui->processCheckBox->setText(NAMECHECKBOX_ENGLISH);
    ui->titleCheckBox->setText(TITLECHECKBOX_ENGLISH);
    ui->descriptionLabel->setText(SETTINGDESCLABEL_ENGLISH);
    if (GLOBALSETTING_INDEX == ui->settingselectComboBox->currentIndex()) {
        ui->descriptionLineEdit->setText(GLOBALSETTING_DESC_ENGLISH);
    }
    QString globalSettingNameWithDescStr = QString(SETTING_DESCRIPTION_FORMAT).arg(GROUPNAME_GLOBALSETTING, GLOBALSETTING_DESC_ENGLISH);
    ui->settingselectComboBox->setItemText(GLOBALSETTING_INDEX, globalSettingNameWithDescStr);
    ui->orikeyLabel->setText(ORIKEYLABEL_ENGLISH);
    ui->orikeySeqLabel->setText(ORIKEYSEQLABEL_ENGLISH);
    ui->mapkeyLabel->setText(MAPKEYLABEL_ENGLISH);
    // ui->burstpressLabel->setText(BURSTPRESSLABEL_ENGLISH);
    // ui->burstreleaseLabel->setText(BURSTRELEASE_ENGLISH);
    // ui->burstpress_msLabel->setText(BURSTPRESS_MSLABEL_ENGLISH);
    // ui->burstrelease_msLabel->setText(BURSTRELEASE_MSLABEL_ENGLISH);
    ui->waitTimeLabel->setText(WAITTIME_ENGLISH);
    ui->waitTimeSpinBox->setSuffix(MILLISECOND_SUFFIX_ENGLISH);
    ui->pressTimeSpinBox->setSuffix(MILLISECOND_SUFFIX_ENGLISH);
    ui->pushLevelLabel->setText(PUSHLEVEL_ENGLISH);
    ui->sendTextLabel->setText(SENDTEXTLABEL_ENGLISH);
    ui->keyPressTypeComboBox->clear();
    ui->keyPressTypeComboBox->addItem(LONGPRESS_ENGLISH);
    ui->keyPressTypeComboBox->addItem(DOUBLEPRESS_ENGLISH);
    ui->pointLabel->setText(POINT_ENGLISH);
    // ui->waitTime_msLabel->setText(WAITTIME_MSLABEL_ENGLISH);
    ui->mouseXSpeedLabel->setText(MOUSEXSPEEDLABEL_ENGLISH);
    ui->mouseYSpeedLabel->setText(MOUSEYSPEEDLABEL_ENGLISH);
    // ui->settingselectLabel->setText(SETTINGSELECTLABEL_ENGLISH);
    ui->removeSettingButton->setText(REMOVESETTINGBUTTON_ENGLISH);
    // ui->disableWinKeyCheckBox->setText(DISABLEWINKEYCHECKBOX_ENGLISH);
    ui->dataPortLabel->setText(DATAPORTLABEL_ENGLISH);
    ui->brakeThresholdLabel->setText(BRAKETHRESHOLDLABEL_ENGLISH);
    ui->accelThresholdLabel->setText(ACCELTHRESHOLDLABEL_ENGLISH);
    ui->autoStartMappingCheckBox->setText(AUTOSTARTMAPPINGCHECKBOX_ENGLISH);
    ui->sendToSameTitleWindowsCheckBox->setText(SENDTOSAMETITLEWINDOWSCHECKBOX_ENGLISH);
    ui->acceptVirtualGamepadInputCheckBox->setText(ACCEPTVIRTUALGAMEPADINPUTCHECKBOX_ENGLISH);
    ui->autoStartupCheckBox->setText(AUTOSTARTUPCHECKBOX_ENGLISH);
    ui->startupMinimizedCheckBox->setText(STARTUPMINIMIZEDCHECKBOX_ENGLISH);
    ui->soundEffectCheckBox->setText(SOUNDEFFECTCHECKBOX_ENGLISH);
    ui->notificationLabel->setText(NOTIFICATIONLABEL_ENGLISH);
    ui->languageLabel->setText(LANGUAGELABEL_ENGLISH);
    ui->updateSiteLabel->setText(UPDATESITELABEL_ENGLISH);
    ui->windowswitchkeyLabel->setText(WINDOWSWITCHKEYLABEL_ENGLISH);
    ui->checkUpdateButton->setText(CHECKUPDATEBUTTON_ENGLISH);
    ui->mappingStartKeyLabel->setText(MAPPINGSTARTKEYLABEL_ENGLISH);
    ui->mappingStopKeyLabel->setText(MAPPINGSTOPKEYLABEL_ENGLISH);

    int last_notification_position = ui->notificationComboBox->currentIndex();
    ui->notificationComboBox->clear();
    QStringList positoin_list = QStringList() \
            << POSITION_NONE_STR_ENGLISH
            << POSITION_TOP_LEFT_STR_ENGLISH
            << POSITION_TOP_CENTER_STR_ENGLISH
            << POSITION_TOP_RIGHT_STR_ENGLISH
            << POSITION_BOTTOM_LEFT_STR_ENGLISH
            << POSITION_BOTTOM_CENTER_STR_ENGLISH
            << POSITION_BOTTOM_RIGHT_STR_ENGLISH
            ;
    ui->notificationComboBox->addItems(positoin_list);
    ui->notificationComboBox->setCurrentIndex(last_notification_position);

    QTabWidget *tabWidget = ui->settingTabWidget;
    tabWidget->setTabText(tabWidget->indexOf(ui->general),          SETTINGTAB_GENERAL_ENGLISH      );
    tabWidget->setTabText(tabWidget->indexOf(ui->mapping),          SETTINGTAB_MAPPING_ENGLISH      );
    tabWidget->setTabText(tabWidget->indexOf(ui->virtual_gamepad),  SETTINGTAB_VGAMEPAD_ENGLISH     );
    tabWidget->setTabText(tabWidget->indexOf(ui->multi_input),      SETTINGTAB_MULTIINPUT_ENGLISH   );
    tabWidget->setTabText(tabWidget->indexOf(ui->forza),            SETTINGTAB_FORZA_ENGLISH        );

#ifdef VIGEM_CLIENT_SUPPORT
    // ui->virtualgamepadGroupBox->setTitle(VIRTUALGAMEPADGROUPBOX_ENGLISH);
    ui->enableVirtualJoystickCheckBox->setText(ENABLEVIRTUALJOYSTICKCHECKBOX_ENGLISH);
    ui->lockCursorCheckBox->setText(LOCKCURSORCHECKBOX_ENGLISH);
    ui->directModeCheckBox->setText(DIRECTMODECHECKBOX_ENGLISH);
    ui->vJoyXSensLabel->setText(VJOYXSENSLABEL_ENGLISH);
    ui->vJoyYSensLabel->setText(VJOYYSENSLABEL_ENGLISH);
    ui->vJoyRecenterLabel->setText(VJOYRECENTERLABEL_ENGLISH);
    ui->vJoyRecenterSpinBox->setSuffix(MILLISECOND_SUFFIX_ENGLISH);
    ui->vJoyRecenterSpinBox->setSpecialValueText(VJOYRECENTERSPINBOX_UNRECENTER_ENGLISH);
    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == QKeyMapper_Worker::ViGEmClient_getConnectState()) {
        ui->installViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_ENGLISH);
    }
    else {
        ui->installViGEmBusButton->setText(INSTALLVIGEMBUSBUTTON_ENGLISH);
    }
    // ui->uninstallViGEmBusButton->setText(UNINSTALLVIGEMBUSBUTTON_ENGLISH);
#endif
    ui->keyboardSelectLabel->setText(KEYBOARDSELECTLABEL_ENGLISH);
    ui->mouseSelectLabel->setText(MOUSESELECTLABEL_ENGLISH);
    ui->gamepadSelectLabel->setText(GAMEPADSELECTLABEL_ENGLISH);
    // ui->multiInputGroupBox->setTitle(MULTIINPUTGROUPBOX_ENGLISH);
    ui->multiInputEnableCheckBox->setText(MULTIINPUTENABLECHECKBOX_ENGLISH);
    ui->filterKeysCheckBox->setText(FILTERKEYSCHECKBOX_ENGLISH);
    ui->multiInputDeviceListButton->setText(MULTIINPUTDEVICELISTBUTTON_ENGLISH);
    if (Interception_Worker::INTERCEPTION_AVAILABLE == Interception_Worker::getInterceptionState()) {
        ui->installInterceptionButton->setText(UNINSTALLINTERCEPTIONBUTTON_ENGLISH);
    }
    else {
        ui->installInterceptionButton->setText(INSTALLINTERCEPTIONBUTTON_ENGLISH);
    }

    /* KeyMappingTabWidget TabText Update for current language */
    for (int tabindex = 0; tabindex < m_KeyMappingTabWidget->count(); ++tabindex) {
        KeyMappingDataTableWidget *mappingTable = qobject_cast<KeyMappingDataTableWidget*>(m_KeyMappingTabWidget->widget(tabindex));
        mappingTable->setHorizontalHeaderLabels(QStringList()   << KEYMAPDATATABLE_COL1_ENGLISH
                                                                << KEYMAPDATATABLE_COL2_ENGLISH
                                                                << KEYMAPDATATABLE_COL3_ENGLISH
                                                                << KEYMAPDATATABLE_COL4_ENGLISH
                                                                << KEYMAPDATATABLE_COL5_ENGLISH);
    }

    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << PROCESSINFOTABLE_COL1_ENGLISH
                                                                  << PROCESSINFOTABLE_COL2_ENGLISH
                                                                  << PROCESSINFOTABLE_COL3_ENGLISH );

    if (m_deviceListWindow != Q_NULLPTR) {
        m_deviceListWindow->setUILanguage(LANGUAGE_ENGLISH);
    }

    if (m_TableSetupDialog != Q_NULLPTR) {
        m_TableSetupDialog->setUILanguage(LANGUAGE_ENGLISH);
    }

    if (m_ItemSetupDialog != Q_NULLPTR) {
        m_ItemSetupDialog->setUILanguage(LANGUAGE_ENGLISH);
    }
}
#endif

void QKeyMapper::resetFontSize()
{
    ui->nextarrowCheckBox->setFont(QFont("Arial", 16));

    QFont customFont(FONTNAME_ENGLISH, 9);
    if (UI_SCALE_2K_PERCENT_100 == m_UI_Scale
        || UI_SCALE_2K_PERCENT_125 == m_UI_Scale
        || UI_SCALE_2K_PERCENT_150 == m_UI_Scale
        || UI_SCALE_1K_PERCENT_100 == m_UI_Scale
        || UI_SCALE_1K_PERCENT_125 == m_UI_Scale
        || UI_SCALE_1K_PERCENT_150 == m_UI_Scale) {

        ui->settingNameLineEdit->setFont(customFont);
        ui->processLineEdit->setFont(customFont);
        ui->windowTitleLineEdit->setFont(customFont);
        ui->checkProcessComboBox->setFont(customFont);
        ui->checkWindowTitleComboBox->setFont(customFont);
        ui->descriptionLineEdit->setFont(customFont);
        ui->languageComboBox->setFont(customFont);
        ui->notificationComboBox->setFont(customFont);
        ui->updateSiteComboBox->setFont(customFont);
        ui->virtualGamepadTypeComboBox->setFont(customFont);
        m_orikeyComboBox->setFont(customFont);
        m_mapkeyComboBox->setFont(customFont);
        ui->keyboardSelectComboBox->setFont(QFont(FONTNAME_ENGLISH, 8));
        ui->mouseSelectComboBox->setFont(QFont(FONTNAME_ENGLISH, 8));
        ui->gamepadSelectComboBox->setFont(QFont(FONTNAME_ENGLISH, 8));
        ui->settingselectComboBox->setFont(customFont);
        // m_windowswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        // m_mappingswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->windowswitchkeyLineEdit->setFont(customFont);
        ui->mappingStartKeyLineEdit->setFont(customFont);
        ui->mappingStopKeyLineEdit->setFont(customFont);
        // m_originalKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->combinationKeyLineEdit->setFont(customFont);
        ui->sendTextLineEdit->setFont(customFont);
        ui->waitTimeSpinBox->setFont(customFont);
        ui->pushLevelSpinBox->setFont(customFont);
        ui->pressTimeSpinBox->setFont(customFont);
        ui->pointDisplayLabel->setFont(customFont);
        ui->dataPortSpinBox->setFont(customFont);
        ui->brakeThresholdDoubleSpinBox->setFont(customFont);
        ui->accelThresholdDoubleSpinBox->setFont(customFont);
        ui->mouseXSpeedSpinBox->setFont(customFont);
        ui->mouseYSpeedSpinBox->setFont(customFont);

        // ui->burstpressSpinBox->setFont(customFont);
        // ui->burstreleaseSpinBox->setFont(customFont);
        ui->processinfoTable->setFont(customFont);
        ui->processinfoTable->horizontalHeader()->setFont(customFont);
        // m_KeyMappingTabWidget->tabBar()->setFont(QFont(FONTNAME_ENGLISH, 8));
        // for (int tabindex = 0; tabindex < m_KeyMappingTabWidget->count(); ++tabindex) {
        //     KeyMappingDataTableWidget *mappingTable = qobject_cast<KeyMappingDataTableWidget*>(m_KeyMappingTabWidget->widget(tabindex));
        //     mappingTable->setFont(customFont);
        //     mappingTable->horizontalHeader()->setFont(customFont);
        // }

        ui->vJoyXSensSpinBox->setFont(customFont);
        ui->vJoyYSensSpinBox->setFont(customFont);
        ui->virtualGamepadNumberSpinBox->setFont(customFont);
        ui->virtualGamepadListComboBox->setFont(customFont);
    }
    else {
        ui->settingNameLineEdit->setFont(customFont);
        ui->processLineEdit->setFont(customFont);
        ui->windowTitleLineEdit->setFont(customFont);
        ui->checkProcessComboBox->setFont(customFont);
        ui->checkWindowTitleComboBox->setFont(customFont);
        ui->descriptionLineEdit->setFont(customFont);
        ui->languageComboBox->setFont(customFont);
        ui->notificationComboBox->setFont(customFont);
        ui->updateSiteComboBox->setFont(customFont);
        ui->virtualGamepadTypeComboBox->setFont(customFont);
        m_orikeyComboBox->setFont(customFont);
        m_mapkeyComboBox->setFont(customFont);
        ui->keyboardSelectComboBox->setFont(QFont(FONTNAME_ENGLISH, 8));
        ui->mouseSelectComboBox->setFont(QFont(FONTNAME_ENGLISH, 8));
        ui->gamepadSelectComboBox->setFont(QFont(FONTNAME_ENGLISH, 8));
        ui->settingselectComboBox->setFont(customFont);
        // m_windowswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        // m_mappingswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->windowswitchkeyLineEdit->setFont(customFont);
        ui->mappingStartKeyLineEdit->setFont(customFont);
        ui->mappingStopKeyLineEdit->setFont(customFont);
        // m_originalKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->combinationKeyLineEdit->setFont(customFont);
        ui->sendTextLineEdit->setFont(customFont);
        ui->waitTimeSpinBox->setFont(customFont);
        ui->pushLevelSpinBox->setFont(customFont);
        ui->pressTimeSpinBox->setFont(customFont);
        ui->pointDisplayLabel->setFont(customFont);
        ui->dataPortSpinBox->setFont(customFont);
        ui->brakeThresholdDoubleSpinBox->setFont(customFont);
        ui->accelThresholdDoubleSpinBox->setFont(customFont);
        ui->mouseXSpeedSpinBox->setFont(customFont);
        ui->mouseYSpeedSpinBox->setFont(customFont);

        // ui->burstpressSpinBox->setFont(customFont);
        // ui->burstreleaseSpinBox->setFont(customFont);
        ui->processinfoTable->setFont(customFont);
        ui->processinfoTable->horizontalHeader()->setFont(customFont);
        // m_KeyMappingTabWidget->tabBar()->setFont(customFont);
        // for (int tabindex = 0; tabindex < m_KeyMappingTabWidget->count(); ++tabindex) {
        //     KeyMappingDataTableWidget *mappingTable = qobject_cast<KeyMappingDataTableWidget*>(m_KeyMappingTabWidget->widget(tabindex));
        //     mappingTable->setFont(customFont);
        //     mappingTable->horizontalHeader()->setFont(customFont);
        // }

        ui->vJoyXSensSpinBox->setFont(customFont);
        ui->vJoyYSensSpinBox->setFont(customFont);
        ui->virtualGamepadNumberSpinBox->setFont(customFont);
        ui->virtualGamepadListComboBox->setFont(customFont);
    }

    if (m_deviceListWindow != Q_NULLPTR) {
        m_deviceListWindow->resetFontSize();
    }

    if (m_TableSetupDialog != Q_NULLPTR) {
        m_TableSetupDialog->resetFontSize();
    }

    if (m_ItemSetupDialog != Q_NULLPTR) {
        m_ItemSetupDialog->resetFontSize();
    }
}

void QKeyMapper::sessionLockStateChanged(bool locked)
{
    emit QKeyMapper_Worker::getInstance()->sessionLockStateChanged_Signal(locked);
}

void QKeyMapper::checkOSVersionMatched()
{
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10
        && QSysInfo::currentCpuArchitecture() == "x86_64") {
        QString platformString = getPlatformString();
        if (platformString.startsWith("Qt5")) {
            QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
            bool notshow_versionunmatched = false;
            if (true == settingFile.contains(NOTSHOW_VERSION_UNMATCHED)){
                notshow_versionunmatched = settingFile.value(NOTSHOW_VERSION_UNMATCHED).toBool();
            }

            if (notshow_versionunmatched != true) {
                QString message = tr("For Windows 10 or higher 64-bit system, it is recommended to use the Qt6_x64 version. The Qt5 version is provided only for compatibility with Windows 7.");
                QString checkbox_message = tr("Do not show this message again");
                bool ischecked = showMessageBoxWithCheckbox(this, message, checkbox_message, CustomMessageBox::Warning);
                if (ischecked) {
                    settingFile.setValue(NOTSHOW_VERSION_UNMATCHED, true);
                }
            }
        }
    }
}

void QKeyMapper::checkFilterKeysEnabled()
{
    if (IsFilterKeysEnabled()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "\033[1;34m[checkFilterKeysEnabled]" << "FilterKeys -> Enabled\033[0m";
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "\033[1;34m[checkFilterKeysEnabled]" << "FilterKeys -> Disabled\033[0m";
#endif
        QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
        bool notshow_filterkeys_disabled = false;
        if (true == settingFile.contains(NOTSHOW_FILTERKEYS_DISABLED)){
            notshow_filterkeys_disabled = settingFile.value(NOTSHOW_FILTERKEYS_DISABLED).toBool();
        }

        if (notshow_filterkeys_disabled != true) {
            QString message = tr("Using QKeyMapper is strongly recommended to enable the FilterKeys feature in Windows to avoid various unexpected issues.");
            QString checkbox_message = tr("Do not show this message again");
            bool ischecked = showMessageBoxWithCheckbox(this, message, checkbox_message, CustomMessageBox::Warning);
            if (ischecked) {
                settingFile.setValue(NOTSHOW_FILTERKEYS_DISABLED, true);
            }
        }
    }
}

void QKeyMapper::updateLockStatusDisplay()
{
    int rowindex = 0;
    for (const MAP_KEYDATA &keymapdata : std::as_const(*KeyMappingDataList))
    {
        if (m_KeyMapStatus == KEYMAP_MAPPING_MATCHED
            || m_KeyMapStatus == KEYMAP_MAPPING_GLOBAL) {
            if (keymapdata.Lock == true) {
                if (keymapdata.LockState != LOCK_STATE_LOCKOFF) {
                    m_KeyMappingDataTable->item(rowindex, LOCK_COLUMN)->setForeground(QBrush(STATUS_ON_COLOR));
                }
                else {
                    m_KeyMappingDataTable->item(rowindex, LOCK_COLUMN)->setForeground(Qt::black);
                }
            }
        }
        else {
            // m_KeyMappingDataTable->item(rowindex, LOCK_COLUMN)->setText(QString());
            m_KeyMappingDataTable->item(rowindex, LOCK_COLUMN)->setForeground(Qt::black);
        }

        rowindex += 1;
    }
}

void QKeyMapper::updateMousePointLabelDisplay(const QPoint &point)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[updateMousePointLabelDisplay]" << " Point X: " << point.x() << ", Point Y: " << point.y();
#endif
    if (point.x() >= 0 && point.y() >= 0) {
        QString labelText = QString("X:%1, Y:%2").arg(point.x()).arg(point.y());
        ui->pointDisplayLabel->setText(labelText);
    }
    // else {
    //     ui->pointDisplayLabel->clear();
    // }
}

void QKeyMapper::showMousePoints(int showpoints_trigger)
{
    if (SHOW_POINTSIN_SCREEN_ON == showpoints_trigger) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[showMousePoints]" << "Show Points Trigger -> SHOW_POINTSIN_SCREEN_ON";
#endif
        if (ScreenMousePointsList.isEmpty()) {
            return;
        }

        if (IsWindowVisible(m_TransParentHandle)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[showMousePoints]" << "TransParentWindow is already visible.";
#endif
            return;
        }

        // Set show mode to SCREEN
        SetWindowLongPtr(m_TransParentHandle, GWLP_USERDATA, SHOW_MODE_SCREEN_MOUSEPOINTS);
        resizeTransparentWindow(m_TransParentHandle, m_TransParentWindowInitialX, m_TransParentWindowInitialY, m_TransParentWindowInitialWidth, m_TransParentWindowInitialHeight);

        // SetWindowPos(m_TransParentHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
        // SWP_SHOWWINDOW parameter will show this window after SetWindowPos() called.
        ShowWindow(m_TransParentHandle, SW_SHOW);
    }
    else if (SHOW_POINTSIN_SCREEN_OFF == showpoints_trigger) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[showMousePoints]" << "Show Points Trigger -> SHOW_POINTSIN_SCREEN_OFF";
#endif
        ShowWindow(m_TransParentHandle, SW_HIDE);
    }
    else if (SHOW_POINTSIN_WINDOW_ON == showpoints_trigger) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[showMousePoints]" << "Show Points Trigger -> SHOW_POINTSIN_WINDOW_ON";
#endif
        if (WindowMousePointsList.isEmpty()) {
            return;
        }

        if (IsWindowVisible(m_TransParentHandle)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[showMousePoints]" << "TransParentWindow is already visible.";
#endif
            return;
        }

        RECT clientRect;
        WINDOWINFO winInfo;
        winInfo.cbSize = sizeof(WINDOWINFO);
        if (s_CurrentMappingHWND != NULL && GetWindowInfo(s_CurrentMappingHWND, &winInfo)) {
            clientRect = winInfo.rcClient;
            int clientWidth = clientRect.right - clientRect.left;
            int clientHeight = clientRect.bottom - clientRect.top;
            // Set show mode to WINDOW
            SetWindowLongPtr(m_TransParentHandle, GWLP_USERDATA, SHOW_MODE_WINDOW_MOUSEPOINTS);
            resizeTransparentWindow(m_TransParentHandle, clientRect.left, clientRect.top, clientWidth, clientHeight);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[showMousePoints]"<< " CurrentMappingHWND clientRect -> x:" << clientRect.left << ", y:" << clientRect.top << ", w:" << clientWidth << ", h:" << clientHeight;
#endif
            ShowWindow(m_TransParentHandle, SW_SHOW);
        }
    }
    else if (SHOW_POINTSIN_WINDOW_OFF == showpoints_trigger) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[showMousePoints]" << "Show Points Trigger -> SHOW_POINTSIN_WINDOW_OFF";
#endif
        ShowWindow(m_TransParentHandle, SW_HIDE);
    }
}

void QKeyMapper::showPopupMessage(const QString& message, const QString& color, int displayDuration)
{
    if (!m_PopupMessageLabel || !m_PopupMessageAnimation) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[showPopupMessage]" << "PopupMessage not initialized!";
#endif
        return;
    }

    m_PopupMessageAnimation->stop();
    m_PopupMessageLabel->hide();
    m_PopupMessageLabel->clear();

    // QString styleSheet = QString("background-color: rgba(0, 0, 0, 180); color: white; padding: 15px; border-radius: 5px; color: %1;").arg(color);
    QString styleSheet = QString("color: %1;").arg(color);
    m_PopupMessageLabel->setStyleSheet(styleSheet);

    QFont customFont(FONTNAME_ENGLISH, 16, QFont::Bold);
    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(20);
    }
    m_PopupMessageLabel->setFont(customFont);
    m_PopupMessageLabel->setText(message);
    m_PopupMessageLabel->adjustSize();

    QRect windowGeometry = this->geometry();
    int x = windowGeometry.x() + (windowGeometry.width() - m_PopupMessageLabel->width()) / 2;
    int y = windowGeometry.y() + (windowGeometry.height() - m_PopupMessageLabel->height()) / 2;
    m_PopupMessageLabel->move(x, y);

    m_PopupMessageAnimation->setDuration(displayDuration);
    m_PopupMessageAnimation->setStartValue(1.0);
    m_PopupMessageAnimation->setEndValue(0.0);
    m_PopupMessageAnimation->start(QAbstractAnimation::KeepWhenStopped);

    m_PopupMessageLabel->show();
}

void QKeyMapper::showCarOrdinal(qint32 car_ordinal)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[showCarOrdinal]" << "CarOrdinal =" << car_ordinal;
#endif

    QString car_ordinal_str = QString::number(car_ordinal);
    ui->pointDisplayLabel->setText(car_ordinal_str);
}

void QKeyMapper::showCrosshairStart(int rowindex, const QString &crosshair_keystr)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[showCrosshairStart]" << "Crosshair =" << crosshair_keystr << ", RowIndex =" << rowindex;
#endif
    Q_UNUSED(rowindex);
    Q_UNUSED(crosshair_keystr);

    int show_mode = SHOW_MODE_NONE;

    if (QKeyMapper_Worker::s_Crosshair_TypeA) {
        show_mode = SHOW_MODE_CROSSHAIR_TYPEA;
    }
    else if (QKeyMapper_Worker::s_Crosshair_Normal) {
        show_mode = SHOW_MODE_CROSSHAIR_NORMAL;
    }

    if (show_mode == SHOW_MODE_NONE) {
        return;
    }

    if (rowindex < 0 || rowindex >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }
    MAP_KEYDATA keymapdata = QKeyMapper::KeyMappingDataList->at(rowindex);

    // Combine show_mode and rowindex into a single 32-bit value (high 2 bytes for show_mode, low 2 bytes for rowindex)
    ULONG_PTR show_param = (show_mode << 16) | (rowindex & 0xFFFF);

    // Set the combined show_param to the window user data
    SetWindowLongPtr(m_CrosshairHandle, GWLP_USERDATA, show_param);

    RECT clientRect;
    WINDOWINFO winInfo;
    winInfo.cbSize = sizeof(WINDOWINFO);
    if (s_CurrentMappingHWND != NULL && GetWindowInfo(s_CurrentMappingHWND, &winInfo)) {
        clientRect = winInfo.rcClient;
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;
        resizeTransparentWindow(m_CrosshairHandle, clientRect.left, clientRect.top, clientWidth, clientHeight);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[showCrosshairStart]"<< " CurrentMappingHWND clientRect -> x:" << clientRect.left << ", y:" << clientRect.top << ", w:" << clientWidth << ", h:" << clientHeight;
#endif
    }
    else {
        resizeTransparentWindow(m_CrosshairHandle, m_CrosshairWindowInitialX, m_CrosshairWindowInitialY, m_CrosshairWindowInitialWidth, m_CrosshairWindowInitialHeight);
    }

    int opacity = keymapdata.Crosshair_CrosshairOpacity;
    SetLayeredWindowAttributes(m_CrosshairHandle, RGB(0, 0, 0), opacity, LWA_ALPHA | LWA_COLORKEY);
    ShowWindow(m_CrosshairHandle, SW_SHOW);
}

void QKeyMapper::showCrosshairStop(int rowindex, const QString &crosshair_keystr)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[showCrosshairStop]" << "Crosshair =" << crosshair_keystr << ", RowIndex =" << rowindex;
#endif
    Q_UNUSED(rowindex);
    Q_UNUSED(crosshair_keystr);

    if (false == QKeyMapper_Worker::s_Crosshair_Normal
        && false == QKeyMapper_Worker::s_Crosshair_TypeA) {
        ShowWindow(m_CrosshairHandle, SW_HIDE);
    }
}

void QKeyMapper::onKeyMappingTabWidgetTabBarDoubleClicked(int index)
{
    if ((0 <= index)
        && (index < m_KeyMappingTabWidget->count())
        && (index < s_KeyMappingTabInfoList.size())) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onKeyMappingTabWidgetTabBarDoubleClicked]" << "m_KeyMappingTabWidget TabBar doubleclicked :" << index;
#endif

        showTableSetupDialog(index);
    }
}

void QKeyMapper::keyMappingTabWidgetCurrentChanged(int index)
{
    if (0 <= index && index < m_KeyMappingTabWidget->count()) {
        disconnectKeyMappingDataTableConnection();
        switchKeyMappingTabIndex(index);
        updateCategoryFilterByShowCategoryState();
        updateKeyMappingDataTableConnection();

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyMappingTabWidgetCurrentChanged]" << "m_KeyMappingTabWidget tab changed :" << index;
#endif
    }
}

void QKeyMapper::onKeyMappingTabWidgetTabOrderChanged(int from, int to)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onKeyMappingTabWidgetTabOrderChanged] Tab moved from" << from << "to" << to;
#endif

    // Handle the tab reordering in our data structures
    moveTabInKeyMappingTabWidget(from, to);
}

void QKeyMapper::keyMappingTableDragDropMove(int top_row, int bottom_row, int dragged_to)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[keyMappingTableDragDropMove] DragDrop : Rows" << top_row << ":" << bottom_row << "->" << dragged_to;
#endif

    int mappingdata_size = KeyMappingDataList->size();
    if (top_row >= 0 && bottom_row < mappingdata_size && dragged_to >= 0 && dragged_to < mappingdata_size
        && (dragged_to > bottom_row || dragged_to < top_row)) {
        int draged_row_count = bottom_row - top_row + 1;
        bool isDraggedToBottom = (dragged_to > bottom_row);

        if (isDraggedToBottom) {
            for (int i = 0; i < draged_row_count; ++i) {
                KeyMappingDataList->move(top_row, dragged_to);
            }
        }
        else {
            /* Dragged to top */
            for (int i = draged_row_count - 1; i >= 0; --i) {
                KeyMappingDataList->move(bottom_row, dragged_to);
            }
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyMappingTableDragDropMove] : refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);

        // Reselect the moved rows
        QTableWidgetSelectionRange newSelection;
        if (isDraggedToBottom) {
            newSelection = QTableWidgetSelectionRange(dragged_to - draged_row_count + 1, 0, dragged_to, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        }
        else {
            newSelection = QTableWidgetSelectionRange(dragged_to, 0, dragged_to + draged_row_count - 1, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        }
        m_KeyMappingDataTable->clearSelection();
        m_KeyMappingDataTable->setRangeSelected(newSelection, true);

#ifdef DEBUG_LOGOUT_ON
        if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList->size()) {
            qDebug("keyMappingTableDragDropMove : KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList->size());
        }
#endif
    }
}

void QKeyMapper::keyMappingTableItemDoubleClicked(QTableWidgetItem *item)
{
    if (item == Q_NULLPTR) {
        return;
    }

    int rowindex = item->row();
    int columnindex = item->column();

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[keyMappingTableItemDoubleClicked]" << "Row" << rowindex << "Column" << columnindex << "DoubleClicked";
#endif

    // Check if the double-clicked item is in the Category column
    if (columnindex == CATEGORY_COLUMN && m_KeyMappingDataTable && m_KeyMappingDataTable->isCategoryColumnVisible()) {
        // If it's the category column and it's visible, allow inline editing
        m_KeyMappingDataTable->editItem(item);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyMappingTableItemDoubleClicked]" << "Category column double-clicked, entering edit mode";
#endif
    }
    else {
        // For all other columns, show the item setup dialog as before
        showItemSetupDialog(s_KeyMappingTabWidgetCurrentIndex, rowindex);
    }
}

void QKeyMapper::setupDialogClosed()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[setupDialogClosed]" << "refreshKeyMappingDataTable()";
#endif

    int reselectrow = -1;
    QList<QTableWidgetItem*> items = m_KeyMappingDataTable->selectedItems();
    if (items.size() > 0) {
        QTableWidgetItem* selectedItem = items.at(0);
        reselectrow = m_KeyMappingDataTable->row(selectedItem);
    }

    refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);
    updateMousePointsList();

    if (reselectrow >= 0) {
        QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        m_KeyMappingDataTable->setRangeSelected(selection, true);
    }
}

void QKeyMapper::onUpdateDownloadFinished(const QString &url, const QString &filepath)
{
    Q_UNUSED(url);
    QFileInfo fileinfo(filepath);
    QString filename = fileinfo.fileName();
    QString dirname = fileinfo.dir().dirName();

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[onUpdateDownloadFinished]" << "URL ->" << url;
    qDebug() << "[onUpdateDownloadFinished]" << "filepath ->" << filepath;
    qDebug() << "[onUpdateDownloadFinished]" << "filename ->" << filename;
    qDebug() << "[onUpdateDownloadFinished]" << "dirname ->" << dirname;
#endif

    // Check if the file exists
    if (!fileinfo.exists()) {
        QString errorMessage = tr("The upgrade package %1 does not exist in the directory %2. Download failed!").arg(filename, dirname);
        showFailurePopup(errorMessage);
        return;
    }

    QString message = tr("<html><head/><body><p align=\"center\">The upgrade package <b>%1</b> has been successfully downloaded to the directory <b>%2</b>.</p>")
            .arg(filename.toHtmlEscaped(), dirname.toHtmlEscaped());
    message += tr("<p align=\"center\"><b>Click [Yes] to automatically close the program and upgrade</b>, otherwise handle it manually.</p></body></html>");

    QMessageBox::StandardButton reply = QMessageBox::question(this, PROGRAM_NAME, message);

    if (reply == QMessageBox::Yes) {
        QFileInfo configFileInfo(CONFIG_FILENAME);
        QString backupFilename = QString("%1_%2_%3_%4.ini").arg(configFileInfo.baseName(), getExeProductVersion(), getPlatformString(),  QDateTime::currentDateTime().toString("yyyyMMdd-hhmm"));
        QString bakcFilePath = QString(SETTINGS_BACKUP_DIR) + "/" + backupFilename;

        bool backupResult = backupFile(CONFIG_LATEST_FILENAME, bakcFilePath);
        if (backupResult) {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("[onUpdateDownloadFinished] Backup \"%1\" to \"%2\" success.").arg(CONFIG_LATEST_FILENAME, bakcFilePath);
            qDebug().nospace().noquote() << debugmessage;
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("[onUpdateDownloadFinished] Backup \"%1\" to \"%2\" failed!").arg(CONFIG_LATEST_FILENAME, bakcFilePath);
            qDebug().nospace().noquote() << debugmessage;
#endif
        }

        ValidationResult result = updateWithZipUpdater(filepath);
        if (result.isValid) {
            QApplication::quit();
        }
        else {
            showFailurePopup(result.errorMessage);
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onUpdateDownloadFinished] User chose [No] to handle the update later.";
#endif
    }
}

#if 0
void QKeyMapper::keyMappingTableCellDoubleClicked(int row, int column)
{
    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyMappingTableCellDoubleClicked]" << "L-Ctrl Pressed + DoubleClicked ->" << "Row =" << row << ", Column =" << column;
#endif
    }
}
#endif

#ifdef SINGLE_APPLICATION
void QKeyMapper::otherInstancesStarted()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[raiseQKeyMapperWindow]" << "Other QKeyMapper Instances started.";
#endif

    showQKeyMapperWindowToTop();
}
#endif

#if 0
void QKeyMapper::updateShortcutsMap()
{
    freeShortcuts();

    for (const MAP_KEYDATA &keymapdata : std::as_const(KeyMappingDataList))
    {
        if (keymapdata.Original_Key.startsWith(OLD_PREFIX_SHORTCUT))
        {
            QString shortcutstr = keymapdata.Original_Key;
            shortcutstr.remove(OLD_PREFIX_SHORTCUT);
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

    for (QHotkey* shortcut : std::as_const(HotkeysList)) {
        bool unregister = shortcut->setRegistered(false);
        Q_UNUSED(unregister);
#ifdef DEBUG_LOGOUT_ON
        if (false == unregister) {
            qWarning() << "[freeShortcuts]" << "unregister Shortcut[" << shortcut->shortcut().toString() << "] Failed!!!";
        }
#endif
    }

    for (QHotkey* shortcut : std::as_const(HotkeysList)) {
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
#endif

void QKeyMapper::on_processinfoTable_doubleClicked(const QModelIndex &index)
{
    if ((KEYMAP_IDLE == m_KeyMapStatus)
        && (true == ui->processinfoTable->isEnabled())){
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[SelectProcessInfo]" << "Table DoubleClicked [" << index.row() << "] " << ui->processinfoTable->item(index.row(), 0)->text() << ", " << ui->processinfoTable->item(index.row(), 2)->text();
#endif
        ui->processLineEdit->setEnabled(true);
        ui->windowTitleLineEdit->setEnabled(true);
        ui->restoreProcessPathButton->setEnabled(true);
        // ui->processCheckBox->setEnabled(true);
        // ui->titleCheckBox->setEnabled(true);
        ui->processLabel->setEnabled(true);
        ui->windowTitleLabel->setEnabled(true);
        ui->checkProcessComboBox->setEnabled(true);
        ui->checkWindowTitleComboBox->setEnabled(true);
        ui->removeSettingButton->setEnabled(true);

        // QString filename = ui->processinfoTable->item(index.row(), PROCESS_NAME_COLUMN)->text();
        QString windowTitle = ui->processinfoTable->item(index.row(), PROCESS_TITLE_COLUMN)->text();
        QString pidStr = ui->processinfoTable->item(index.row(), PROCESS_PID_COLUMN)->text();
        QString ProcessPath;
        DWORD dwProcessId = pidStr.toULong();

        getProcessInfoFromPID(dwProcessId, ProcessPath);

        QString loadSettingSelectStr = matchSavedSettings(ProcessPath, windowTitle);
        if (loadSettingSelectStr.isEmpty()) {
            ui->settingselectComboBox->setCurrentText(QString());
            ui->descriptionLineEdit->clear();
            ui->checkProcessComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
            ui->checkWindowTitleComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
            ui->settingNameLineEdit->setText(windowTitle);
        }
        else {
            QString curSettingSelectStr;
            int curSettingSelectIndex = ui->settingselectComboBox->currentIndex();
            if (0 < curSettingSelectIndex && curSettingSelectIndex < m_SettingSelectListWithoutDescription.size()) {
                curSettingSelectStr = m_SettingSelectListWithoutDescription.at(curSettingSelectIndex);
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug().noquote().nospace() << "[on_processinfoTable_doubleClicked]" << "Doubleclick to load setting select index is invalid("<< curSettingSelectIndex << "), m_SettingSelectListWithoutDescription ->" << m_SettingSelectListWithoutDescription;
#endif
            }
            if (curSettingSelectStr != loadSettingSelectStr) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[on_processinfoTable_doubleClicked] "<< "Setting Check Matched! Load setting -> [" << loadSettingSelectStr << "]";
#endif
                loadSetting_flag = true;
                QString loadresult = loadKeyMapSetting(loadSettingSelectStr);
                ui->settingNameLineEdit->setText(loadresult);
                Q_UNUSED(loadresult)
                loadSetting_flag = false;

                if (loadresult == loadSettingSelectStr) {
                    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) == 0) {
                        switchToWindowInfoTab();
                        return;
                    }
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[on_processinfoTable_doubleClicked]" << "Current setting select is already the same ->" << curSettingSelectStr;
#endif
                if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) == 0) {
                    switchToWindowInfoTab();
                    QString message = tr("The current selected setting is already \"%1\"").arg(curSettingSelectStr);
                    QKeyMapper::getInstance()->showInformationPopup(message);
                    return;
                }
            }
        }

        if (ProcessPath.isEmpty()) {
            bool adjust_priv;
            adjust_priv = EnablePrivilege(SE_DEBUG_NAME);
            if (adjust_priv) {
                getProcessInfoFromPID(dwProcessId, ProcessPath);
            }
            else {
                qDebug() << "[on_processinfoTable_doubleClicked]" << "getProcessInfoFromPID EnablePrivilege(SE_DEBUG_NAME) Failed with ->" << GetLastError();
            }
            adjust_priv = DisablePrivilege(SE_DEBUG_NAME);

            if (!adjust_priv) {
                qDebug() << "[on_processinfoTable_doubleClicked]" << "getProcessInfoFromPID DisablePrivilege(SE_DEBUG_NAME) Failed with ->" << GetLastError();
            }
#ifdef DEBUG_LOGOUT_ON
            if (ProcessPath.isEmpty()) {
                qDebug().nospace().noquote() << "[on_processinfoTable_doubleClicked] " << "EnablePrivilege(SE_DEBUG_NAME) getProcessInfoFromPID Failed! -> " << " [PID:" << dwProcessId <<"]";
            }
            else {
                qDebug().nospace().noquote() << "[on_processinfoTable_doubleClicked] " << "EnablePrivilege(SE_DEBUG_NAME) getProcessInfoFromPID Success -> " << ProcessPath << " [PID:" << dwProcessId <<"]";
            }
#endif
        }

        if (ProcessPath.isEmpty()) {
            ProcessPath = ui->processinfoTable->item(index.row(), PROCESS_NAME_COLUMN)->text();
        }

        if (ProcessPath.isEmpty()
            || !QFileInfo::exists(ProcessPath)){
            return;
        }

        QIcon fileicon;
        fileicon = extractBestIconFromExecutable(ProcessPath);

        if (fileicon.isNull()) {
            // If the new method fails, fallback to the original QFileIconProvider method
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[on_processinfoTable_doubleClicked]" << "New icon extraction failed, falling back to QFileIconProvider";
#endif
            QFileIconProvider icon_provider;
            fileicon = icon_provider.icon(QFileInfo(ProcessPath));
        }

        if (fileicon.isNull()) {
            fileicon = ui->processinfoTable->item(index.row(), PROCESS_NAME_COLUMN)->icon();
        }

        setMapProcessInfo(ui->processinfoTable->item(index.row(), PROCESS_NAME_COLUMN)->text(),
                          ui->processinfoTable->item(index.row(), PROCESS_TITLE_COLUMN)->text(),
                          ui->processinfoTable->item(index.row(), PROCESS_PID_COLUMN)->text(),
                          ProcessPath,
                          fileicon);


        if (!fileicon.isNull()) {
#ifdef DEBUG_LOGOUT_ON
            QList<QSize> iconsizeList = fileicon.availableSizes();
            qDebug() << "[on_processinfoTable_doubleClicked]" << "Icon availableSizes:" << iconsizeList;
#endif
            // Directly create pixmap from the extracted icon, as we have ensured the correct size
            QPixmap scaled_pixmap = m_MapProcessInfo.WindowIcon.pixmap(QSize(DEFAULT_ICON_WIDTH, DEFAULT_ICON_WIDTH));
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[on_processinfoTable_doubleClicked]" << " Icon Scaled(" << QSize(DEFAULT_ICON_WIDTH, DEFAULT_ICON_WIDTH) << ") pixmap size: " << scaled_pixmap.size();
#endif

            ui->iconLabel->setPixmap(scaled_pixmap);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[on_processinfoTable_doubleClicked]" << "Load & retrive file icon failure!!!";
#endif
            ui->iconLabel->clear();
        }

#if 0
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
        for(const QSize &iconsize : std::as_const(iconsizeList)){
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
#endif

        ui->processLineEdit->setToolTip(ProcessPath);

        ui->processLineEdit->setText(ProcessPath);
        ui->windowTitleLineEdit->setText(windowTitle);

        if (ui->settingselectComboBox->currentIndex() != GLOBALSETTING_INDEX) {
            ui->settingNameLineEdit->setReadOnly(false);
        }

        switchToWindowInfoTab();
    }
}

void QKeyMapper::on_addmapdataButton_clicked()
{
    bool isDoublePress = false;
    bool multiInputSupport = false;
    if (Interception_Worker::INTERCEPTION_AVAILABLE == Interception_Worker::getInterceptionState()) {
        multiInputSupport = true;
    }
    QString currentOriKeyText;
    QString currentOriKeyTextWithoutPostfix;
    QString currentMapKeyText = m_mapkeyComboBox->currentText();
    QString currentMapKeyComboBoxText = currentMapKeyText;
    QString currentOriKeyComboBoxText = m_orikeyComboBox->currentText();
    QString currentOriCombinationKeyText = ui->combinationKeyLineEdit->text();
    // QString currentOriKeyShortcutText = m_originalKeySeqEdit->keySequence().toString();
    if (false == currentOriKeyComboBoxText.isEmpty()) {
        currentOriKeyText = currentOriKeyComboBoxText;

        int keyboardselect_index = ui->keyboardSelectComboBox->currentIndex();
        if (ui->keyboardSelectComboBox->isEnabled() && keyboardselect_index > 0) {
            if (QKeyMapper_Worker::MultiKeyboardInputList.contains(currentOriKeyText)) {
                currentOriKeyText = QString("%1@%2").arg(currentOriKeyText, QString::number(keyboardselect_index - 1));
            }
        }

        int mouseselect_index = ui->mouseSelectComboBox->currentIndex();
        if (ui->mouseSelectComboBox->isEnabled() && mouseselect_index > 0) {
            if (QKeyMapper_Worker::MultiMouseInputList.contains(currentOriKeyText)) {
                currentOriKeyText = QString("%1@%2").arg(currentOriKeyText, QString::number(mouseselect_index - 1));
            }
        }

        QString gamepadselect_string = ui->gamepadSelectComboBox->currentText();
        if (currentOriKeyText.startsWith("Joy-")
            && false == gamepadselect_string.isEmpty()) {
            static QRegularExpression gamepadinfo_regex(R"(^\[(\d+)\] (.*?) \[VID=0x([0-9A-F]+)\]\[PID=0x([0-9A-F]+)\](?:\[ViGEM\])?$)");
            QRegularExpressionMatch gamepadinfo_match = gamepadinfo_regex.match(gamepadselect_string);
            if (gamepadinfo_match.hasMatch()) {
                QString player_index_string = gamepadinfo_match.captured(1);
                if (m_GamepadInfoMap.contains(player_index_string.toInt())) {
                    currentOriKeyText = QString("%1@%2").arg(currentOriKeyText, player_index_string);
                }
            }
        }
    }
    else if (false == currentOriCombinationKeyText.isEmpty()) {
        bool valid_combinationkey = true;
        if (currentOriCombinationKeyText.startsWith(SEPARATOR_PLUS) || currentOriCombinationKeyText.endsWith(SEPARATOR_PLUS)) {
            valid_combinationkey = false;
        }
        else {
            QStringList combinationkeyslist = currentOriCombinationKeyText.split(SEPARATOR_PLUS);
            if (combinationkeyslist.size() <= 1) {
                valid_combinationkey = false;
            }
            else {
                if (multiInputSupport) {
                    static QRegularExpression reg("@[0-9]$");
                    QStringList keyslist;
                    for (const QString &key : std::as_const(combinationkeyslist)) {
                        bool multi_input = false;
                        QString pure_key;
                        QRegularExpressionMatch match = reg.match(key);
                        if (match.hasMatch()) {
                            int atIndex = key.lastIndexOf('@');
                            pure_key = key.mid(0, atIndex);
                            multi_input = true;
                        } else {
                            pure_key = key;
                        }

                        if (keyslist.contains(pure_key)) {
                            valid_combinationkey = false;
                            break;
                        }

                        if (multi_input) {
                            if (QKeyMapper_Worker::MultiKeyboardInputList.contains(pure_key)) {
                            }
                            else if (QKeyMapper_Worker::MultiMouseInputList.contains(pure_key)) {
                            }
                            else {
                                valid_combinationkey = false;
                                break;
                            }
                        }
                        else {
                            if (!QKeyMapper_Worker::CombinationKeysList.contains(pure_key)) {
                                valid_combinationkey = false;
                                break;
                            }
                        }
                        keyslist.append(pure_key);
                    }
                }
                else {
                    QStringList keyslist;
                    for (const QString &key : std::as_const(combinationkeyslist)) {
                        if (keyslist.contains(key)) {
                            valid_combinationkey = false;
                            break;
                        }
                        if (false == QKeyMapper_Worker::CombinationKeysList.contains(key)) {
                            valid_combinationkey = false;
                            break;
                        }
                        keyslist.append(key);
                    }
                }
            }
        }

        if (valid_combinationkey) {
            currentOriKeyText = currentOriCombinationKeyText;
        }
        else {
            showFailurePopup(tr("Invalid input format for the original key combination!"));
            return;
        }
    }

    if (currentOriKeyText.isEmpty() || (m_mapkeyComboBox->isEnabled() && currentMapKeyText.isEmpty() && ui->nextarrowCheckBox->isChecked() == false)) {
        return;
    }

    currentOriKeyTextWithoutPostfix = currentOriKeyText;
    int pressTime = ui->pressTimeSpinBox->value();
    bool isSpecialOriginalKey = QKeyMapper_Worker::SpecialOriginalKeysList.contains(currentOriKeyComboBoxText);
    if (ui->keyPressTypeComboBox->currentIndex() == KEYPRESS_TYPE_LONGPRESS && pressTime > 0 && isSpecialOriginalKey == false && currentMapKeyComboBoxText != KEY_BLOCKED_STR) {
        currentOriKeyText = currentOriKeyText + QString(SEPARATOR_LONGPRESS) + QString::number(pressTime);
    }
    else if (ui->keyPressTypeComboBox->currentIndex() == KEYPRESS_TYPE_DOUBLEPRESS && pressTime > 0 && isSpecialOriginalKey == false && currentMapKeyComboBoxText != KEY_BLOCKED_STR){
        currentOriKeyText = currentOriKeyText + QString(SEPARATOR_DOUBLEPRESS) + QString::number(pressTime);
        isDoublePress = true;
    }

    static QRegularExpression vjoy_pushlevel_keys_regex(R"(^vJoy-(Key11\(LT\)|Key12\(RT\)|(?:LS|RS)-(?:Up|Down|Left|Right|Radius))$)");
    bool already_exist = false;
    int findindex = -1;
    // findindex = findOriKeyInKeyMappingDataList(currentOriKeyText);
    if (isDoublePress) {
        findindex = findOriKeyInKeyMappingDataList_ForDoublePress(currentOriKeyTextWithoutPostfix);
    }
    else {
        findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(currentOriKeyText);
    }
    if (findindex != -1){
        if (isSpecialOriginalKey) {
            already_exist = true;
        }
        else if (KeyMappingDataList->at(findindex).Mapping_Keys.size() == 1
                && false == ui->nextarrowCheckBox->isChecked()
                && KeyMappingDataList->at(findindex).Mapping_Keys.contains(currentMapKeyText) == true){
                already_exist = true;
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "KeyMap already exist at KeyMappingDataList index : " << findindex;
#endif
        }
    }

    if (false == already_exist) {
        if (findindex != -1){
            if (currentMapKeyText == KEY_BLOCKED_STR
                || currentMapKeyText == KEYSEQUENCEBREAK_STR
                || currentMapKeyText.startsWith(KEY2MOUSE_PREFIX)
                || currentMapKeyText.startsWith(CROSSHAIR_PREFIX)
                || currentMapKeyText.startsWith(FUNC_PREFIX)
                || currentMapKeyText.startsWith(GYRO2MOUSE_PREFIX)
                || currentMapKeyText == MOUSE2VJOY_HOLD_KEY_STR
                || currentMapKeyText == VJOY_LS_RADIUS_STR
                || currentMapKeyText == VJOY_RS_RADIUS_STR
                || currentMapKeyText == VJOY_LT_BRAKE_STR
                || currentMapKeyText == VJOY_RT_BRAKE_STR
                || currentMapKeyText == VJOY_LT_ACCEL_STR
                || currentMapKeyText == VJOY_RT_ACCEL_STR) {
                already_exist = true;
            }
            else {
                MAP_KEYDATA keymapdata = KeyMappingDataList->at(findindex);
                if (keymapdata.Mapping_Keys.contains(KEY_BLOCKED_STR)
                    || keymapdata.Mapping_Keys.contains(KEYSEQUENCEBREAK_STR)
                    || keymapdata.Mapping_Keys.contains(KEY2MOUSE_PREFIX)
                    || keymapdata.Mapping_Keys.contains(CROSSHAIR_PREFIX)
                    || keymapdata.Mapping_Keys.contains(FUNC_PREFIX)
                    || keymapdata.Mapping_Keys.contains(MOUSE2VJOY_HOLD_KEY_STR)
                    || keymapdata.Mapping_Keys.contains(GYRO2MOUSE_PREFIX)
                    || keymapdata.Mapping_Keys.contains(VJOY_LS_RADIUS_STR)
                    || keymapdata.Mapping_Keys.contains(VJOY_RS_RADIUS_STR)
                    || keymapdata.Mapping_Keys.contains(VJOY_LT_BRAKE_STR)
                    || keymapdata.Mapping_Keys.contains(VJOY_RT_BRAKE_STR)
                    || keymapdata.Mapping_Keys.contains(VJOY_LT_ACCEL_STR)
                    || keymapdata.Mapping_Keys.contains(VJOY_RT_ACCEL_STR)){
                    already_exist = true;
                }
            }
        }
        else {
            if (ui->nextarrowCheckBox->isChecked()) {
                if (currentMapKeyText == KEY_BLOCKED_STR
                    || currentMapKeyText == KEYSEQUENCEBREAK_STR
                    || currentMapKeyText.startsWith(KEY2MOUSE_PREFIX)
                    || currentMapKeyText.startsWith(CROSSHAIR_PREFIX)
                    || currentMapKeyText.startsWith(FUNC_PREFIX)
                    || currentMapKeyText.startsWith(GYRO2MOUSE_PREFIX)
                    || currentMapKeyText == MOUSE2VJOY_HOLD_KEY_STR
                    || currentMapKeyText == VJOY_LS_RADIUS_STR
                    || currentMapKeyText == VJOY_RS_RADIUS_STR
                    || currentMapKeyText == VJOY_LT_BRAKE_STR
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
            MAP_KEYDATA keymapdata = KeyMappingDataList->at(findindex);
            if (keymapdata.Mapping_Keys.size() >= KEY_SEQUENCE_MAX) {
                QString message = tr("Key sequence mapping to \"%1\" exceeds the maximum length!").arg(currentOriKeyText);
                showFailurePopup(message);
                return;
            }

            QRegularExpressionMatch vjoy_pushlevel_keys_match = vjoy_pushlevel_keys_regex.match(currentMapKeyText);
            int virtualgamepad_index = ui->virtualGamepadListComboBox->currentIndex();
            if (vjoy_pushlevel_keys_match.hasMatch()) {
                int pushlevel = ui->pushLevelSpinBox->value();
                if (virtualgamepad_index > 0) {
                    if (pushlevel != VJOY_PUSHLEVEL_MAX) {
                        /* Add [pushlevel] value postfix */
                        currentMapKeyText = QString("%1[%2]@%3").arg(currentMapKeyText, QString::number(pushlevel), QString::number(virtualgamepad_index - 1));
                    }
                    else { /* pushlevel == VJOY_PUSHLEVEL_MAX */
                        /* Do not add [pushlevel] value postfix */
                        currentMapKeyText = QString("%1@%2").arg(currentMapKeyText, QString::number(virtualgamepad_index - 1));
                    }
                }
                else {
                    if (pushlevel != VJOY_PUSHLEVEL_MAX) {
                        /* Add [pushlevel] value postfix */
                        currentMapKeyText = QString("%1[%2]").arg(currentMapKeyText, QString::number(pushlevel));
                    }
                }
            }
            else if (virtualgamepad_index > 0
                && QKeyMapper_Worker::MultiVirtualGamepadInputList.contains(currentMapKeyText)) {
                currentMapKeyText = QString("%1@%2").arg(currentMapKeyText, QString::number(virtualgamepad_index - 1));
            }
            else if (currentMapKeyText.startsWith(MOUSE_BUTTON_PREFIX) && currentMapKeyText.endsWith(MOUSE_SCREENPOINT_POSTFIX)) {
                QString mousepointstr = ui->pointDisplayLabel->text();
                if (mousepointstr.isEmpty()) {
                    QString message = tr("Need to set a screen mouse point with \"%1\" click!").arg(tr("L-Ctrl+Mouse-Left Click"));
                    showFailurePopup(message);
                    return;
                }
                else {
                    QPoint mousepoint = getMousePointFromLabelString(mousepointstr);
                    int x = mousepoint.x();
                    int y = mousepoint.y();

                    if (x >= 0 && y >= 0) {
                        currentMapKeyText = currentMapKeyText.remove(MOUSE_SCREENPOINT_POSTFIX) + QString("(%1,%2)").arg(x).arg(y);

                        if (keymapdata.Mapping_Keys.size() == 1
                            && keymapdata.Mapping_Keys.constFirst().contains(currentMapKeyText)
                            && !ui->nextarrowCheckBox->isChecked()) {
                            QString message = tr("Already set a same screen mouse point!");
                            showFailurePopup(message);
                            return;
                        }
                    }
                }
            }
            else if (currentMapKeyText.startsWith(MOUSE_BUTTON_PREFIX) && currentMapKeyText.endsWith(MOUSE_WINDOWPOINT_POSTFIX)) {
                QString mousepointstr = ui->pointDisplayLabel->text();
                if (mousepointstr.isEmpty()) {
                    QString message = tr("Need to set a window mouse point with \"%1\" click!").arg(tr("L-Alt+Mouse-Left Click"));
                    showFailurePopup(message);
                    return;
                }
                else {
                    QPoint mousepoint = getMousePointFromLabelString(mousepointstr);
                    int x = mousepoint.x();
                    int y = mousepoint.y();

                    if (x >= 0 && y >= 0) {
                        currentMapKeyText = currentMapKeyText.remove(MOUSE_WINDOWPOINT_POSTFIX) + QString(":W(%1,%2)").arg(x).arg(y);

                        if (keymapdata.Mapping_Keys.size() == 1
                            && keymapdata.Mapping_Keys.constFirst().contains(currentMapKeyText)
                            && !ui->nextarrowCheckBox->isChecked()) {
                            QString message = tr("Already set a same window mouse point!");
                            showFailurePopup(message);
                            return;
                        }
                    }
                }
            }
            else if (currentMapKeyText == SENDTEXT_STR) {
                QString sendtext = ui->sendTextLineEdit->text();
                if (sendtext.isEmpty()) {
                    QString message = tr("Please input the text to send!");
                    showFailurePopup(message);
                    return;
                }
                else {
                    currentMapKeyText = QString("SendText(%1)").arg(sendtext);
                }
            }
            else if (currentMapKeyText == KEYSEQUENCEBREAK_STR) {
                QString message = tr("KeySequenceBreak key can not be set duplicated!");
                showFailurePopup(message);
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

            QStringList mappingKeySeqList = splitMappingKeyString(mappingkeys_str, SPLIT_WITH_NEXT);
            ValidationResult result = QKeyMapper::validateMappingKeyString(mappingkeys_str, mappingKeySeqList, INITIAL_ROW_INDEX);
            if (!result.isValid) {
                showFailurePopup(result.errorMessage);
                return;
            }

#ifdef DEBUG_LOGOUT_ON
            qDebug() << "mappingkeys_str after add:" << mappingkeys_str;
#endif

            QString mappingkeys_keyup_str;
            if (keymapdata.MappingKeys_KeyUp.isEmpty()) {
                mappingkeys_keyup_str = mappingkeys_str;
            }
            else {
                mappingkeys_keyup_str = keymapdata.MappingKeys_KeyUp.join(SEPARATOR_NEXTARROW);
            }
            KeyMappingDataList->replace(findindex, MAP_KEYDATA(currentOriKeyText,
                                                               mappingkeys_str,
                                                               mappingkeys_keyup_str,
                                                               keymapdata.Note,
                                                               keymapdata.Category,
                                                               keymapdata.Burst,
                                                               keymapdata.BurstPressTime,
                                                               keymapdata.BurstReleaseTime,
                                                               keymapdata.Lock,
                                                               keymapdata.MappingKeyUnlock,
                                                               keymapdata.PostMappingKey,
                                                               keymapdata.FixedVKeyCode,
                                                               keymapdata.CheckCombKeyOrder,
                                                               keymapdata.Unbreakable,
                                                               keymapdata.PassThrough,
                                                               keymapdata.SendTiming,
                                                               keymapdata.KeySeqHoldDown,
                                                               keymapdata.RepeatMode,
                                                               keymapdata.RepeatTimes,
                                                               keymapdata.Crosshair_CenterColor,
                                                               keymapdata.Crosshair_CenterSize,
                                                               keymapdata.Crosshair_CenterOpacity,
                                                               keymapdata.Crosshair_CrosshairColor,
                                                               keymapdata.Crosshair_CrosshairWidth,
                                                               keymapdata.Crosshair_CrosshairLength,
                                                               keymapdata.Crosshair_CrosshairOpacity,
                                                               keymapdata.Crosshair_ShowCenter,
                                                               keymapdata.Crosshair_ShowTop,
                                                               keymapdata.Crosshair_ShowBottom,
                                                               keymapdata.Crosshair_ShowLeft,
                                                               keymapdata.Crosshair_ShowRight,
                                                               keymapdata.Crosshair_X_Offset,
                                                               keymapdata.Crosshair_Y_Offset
                                                               ));
        }
        else {
            if (isSpecialOriginalKey) {
                currentMapKeyText = currentOriKeyComboBoxText;

                int virtualgamepad_index = ui->virtualGamepadListComboBox->currentIndex();
                if (virtualgamepad_index > 0) {
                    currentMapKeyText = QString("%1@%2").arg(currentMapKeyText, QString::number(virtualgamepad_index - 1));
                }
            }
            else {
                QRegularExpressionMatch vjoy_pushlevel_keys_match = vjoy_pushlevel_keys_regex.match(currentMapKeyText);
                int virtualgamepad_index = ui->virtualGamepadListComboBox->currentIndex();
                if (vjoy_pushlevel_keys_match.hasMatch()) {
                    int pushlevel = ui->pushLevelSpinBox->value();
                    if (virtualgamepad_index > 0) {
                        if (pushlevel != VJOY_PUSHLEVEL_MAX) {
                            /* Add [pushlevel] value postfix */
                            currentMapKeyText = QString("%1[%2]@%3").arg(currentMapKeyText, QString::number(pushlevel), QString::number(virtualgamepad_index - 1));
                        }
                        else { /* pushlevel == VJOY_PUSHLEVEL_MAX */
                            /* Do not add [pushlevel] value postfix */
                            currentMapKeyText = QString("%1@%2").arg(currentMapKeyText, QString::number(virtualgamepad_index - 1));
                        }
                    }
                    else {
                        if (pushlevel != VJOY_PUSHLEVEL_MAX) {
                            /* Add [pushlevel] value postfix */
                            currentMapKeyText = QString("%1[%2]").arg(currentMapKeyText, QString::number(pushlevel));
                        }
                    }
                }
                else if (virtualgamepad_index > 0
                    && QKeyMapper_Worker::MultiVirtualGamepadInputList.contains(currentMapKeyText)) {
                    currentMapKeyText = QString("%1@%2").arg(currentMapKeyText, QString::number(virtualgamepad_index - 1));
                }
                else if (currentMapKeyText.startsWith(MOUSE_BUTTON_PREFIX) && currentMapKeyText.endsWith(MOUSE_SCREENPOINT_POSTFIX)) {
                    QString mousepointstr = ui->pointDisplayLabel->text();
                    if (mousepointstr.isEmpty()) {
                        QString message = tr("Need to set a screen mouse point with \"%1\" click!").arg(tr("L-Ctrl+Mouse-Left Click"));
                        showFailurePopup(message);
                        return;
                    }
                    else {
                        QPoint mousepoint = getMousePointFromLabelString(mousepointstr);
                        int x = mousepoint.x();
                        int y = mousepoint.y();

                        if (x >= 0 && y >= 0) {
                            currentMapKeyText = currentMapKeyText.remove(MOUSE_SCREENPOINT_POSTFIX) + QString("(%1,%2)").arg(x).arg(y);
                        }
                    }
                }
                else if (currentMapKeyText.startsWith(MOUSE_BUTTON_PREFIX) && currentMapKeyText.endsWith(MOUSE_WINDOWPOINT_POSTFIX)) {
                    QString mousepointstr = ui->pointDisplayLabel->text();
                    if (mousepointstr.isEmpty()) {
                        QString message = tr("Need to set a window mouse point with \"%1\" click!").arg(tr("L-Alt+Mouse-Left Click"));
                        showFailurePopup(message);
                        return;
                    }
                    else {
                        QPoint mousepoint = getMousePointFromLabelString(mousepointstr);
                        int x = mousepoint.x();
                        int y = mousepoint.y();

                        if (x >= 0 && y >= 0) {
                            currentMapKeyText = currentMapKeyText.remove(MOUSE_WINDOWPOINT_POSTFIX) + QString(":W(%1,%2)").arg(x).arg(y);
                        }
                    }
                }
                else if (currentMapKeyText == SENDTEXT_STR) {
                    QString sendtext = ui->sendTextLineEdit->text();
                    if (sendtext.isEmpty()) {
                        QString message = tr("Please input the text to send!");
                        showFailurePopup(message);
                        return;
                    }
                    else {
                        currentMapKeyText = QString("SendText(%1)").arg(sendtext);
                    }
                }
                else if (currentMapKeyText == KEY_BLOCKED_STR) {
                    if (currentOriKeyText.contains(JOY_KEY_PREFIX)) {
                        QString message = tr("Game controller keys could not be blocked!");
                        showFailurePopup(message);
                        return;
                    }
                }
                else if (currentMapKeyText == KEYSEQUENCEBREAK_STR) {
                    // QString break_keysStr = ui->combinationKeyLineEdit->text();
                    // if (break_keysStr.isEmpty()) {
                    //     currentMapKeyText = KEYSEQUENCEBREAK_STR;
                    // }
                    // else {
                    //     // check break_keysStr as on_originalKeyUpdateButton_clicked use validateOriginalKeyString
                    // }

                    // currentMapKeyText = KEYSEQUENCEBREAK_STR;
                }

                int waitTime = ui->waitTimeSpinBox->value();
                if (waitTime > 0
                    && currentMapKeyComboBoxText != KEY_BLOCKED_STR
                    && currentMapKeyComboBoxText != KEYSEQUENCEBREAK_STR
                    && currentMapKeyComboBoxText.startsWith(KEY2MOUSE_PREFIX) == false
                    && currentMapKeyComboBoxText.startsWith(CROSSHAIR_PREFIX) == false
                    && currentMapKeyComboBoxText.startsWith(FUNC_PREFIX) == false
                    && currentMapKeyComboBoxText.startsWith(GYRO2MOUSE_PREFIX) == false
                    && currentMapKeyComboBoxText != MOUSE2VJOY_HOLD_KEY_STR
                    && currentMapKeyComboBoxText != VJOY_LS_RADIUS_STR
                    && currentMapKeyComboBoxText != VJOY_RS_RADIUS_STR
                    && currentMapKeyComboBoxText != VJOY_LT_BRAKE_STR
                    && currentMapKeyComboBoxText != VJOY_RT_BRAKE_STR
                    && currentMapKeyComboBoxText != VJOY_LT_ACCEL_STR
                    && currentMapKeyComboBoxText != VJOY_RT_ACCEL_STR) {
                    currentMapKeyText = currentMapKeyText + QString(SEPARATOR_WAITTIME) + QString::number(waitTime);
                }
            }

            QStringList mappingKeySeqList = splitMappingKeyString(currentMapKeyText, SPLIT_WITH_NEXT);
            ValidationResult result = QKeyMapper::validateMappingKeyString(currentMapKeyText, mappingKeySeqList, INITIAL_ROW_INDEX);
            if (!result.isValid) {
                showFailurePopup(result.errorMessage);
                return;
            }

            KeyMappingDataList->append(MAP_KEYDATA(currentOriKeyText,                       /* originalkey QString */
                                                   currentMapKeyText,                       /* mappingkeys QString */
                                                   currentMapKeyText,                       /* mappingkeys_keyup QString */
                                                   QString(),                               /* note QString */
                                                   QString(),                               /* category QString */
                                                   false,                                   /* burst bool */
                                                   BURST_PRESS_TIME_DEFAULT,                /* burstpresstime int */
                                                   BURST_RELEASE_TIME_DEFAULT,              /* burstreleasetime int */
                                                   false,                                   /* lock bool */
                                                   false,                                   /* mappingkeys_unlock bool */
                                                   false,                                   /* postmappingkey bool */
                                                   FIXED_VIRTUAL_KEY_CODE_NONE,             /* fixedvkeycode int */
                                                   true,                                    /* checkcombkeyorder bool */
                                                   false,                                   /* unbreakable bool */
                                                   false,                                   /* passthrough bool */
                                                   SENDTIMING_NORMAL,                       /* sendtiming int */
                                                   false,                                   /* keyseqholddown bool */
                                                   REPEAT_MODE_NONE,                        /* repeat_mode int */
                                                   REPEAT_TIMES_DEFAULT,                    /* repeat_times int */
                                                   CROSSHAIR_CENTERCOLOR_DEFAULT_QCOLOR,    /* crosshair_centercolor QColor */
                                                   CROSSHAIR_CENTERSIZE_DEFAULT,            /* crosshair_centersize int */
                                                   CROSSHAIR_CENTEROPACITY_DEFAULT,         /* crosshair_centeropacity int */
                                                   CROSSHAIR_CROSSHAIRCOLOR_DEFAULT_QCOLOR, /* crosshair_crosshaircolor QColor */
                                                   CROSSHAIR_CROSSHAIRWIDTH_DEFAULT,        /* crosshair_crosshairwidth int */
                                                   CROSSHAIR_CROSSHAIRLENGTH_DEFAULT,       /* crosshair_crosshairlength int */
                                                   CROSSHAIR_CROSSHAIROPACITY_DEFAULT,      /* crosshair_crosshairopacity int */
                                                   true,                                    /* crosshair_showcenter bool */
                                                   true,                                    /* crosshair_showtop bool */
                                                   true,                                    /* crosshair_showbottom bool */
                                                   true,                                    /* crosshair_showleft bool */
                                                   true,                                    /* crosshair_showright bool */
                                                   CROSSHAIR_X_OFFSET_DEFAULT,              /* crosshair_x_offset int */
                                                   CROSSHAIR_Y_OFFSET_DEFAULT               /* crosshair_y_offset int */
                                                   ));
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "Add keymapdata :" << currentOriKeyText << "to" << currentMapKeyText;
#endif
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);
    }
    else {
        showFailurePopup(tr("Conflict with exist Keys!"));
    }
}

void QKeyMapper::selectedItemsMoveUp()
{
    if (isMappingDataTableFiltered()) {
        QString message;
        message = tr("Cannot move items while the mapping table is filtered!");
        showWarningPopup(message);
        return;
    }

    QList<QTableWidgetSelectionRange> selectedRanges = m_KeyMappingDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveUpItem] There is no selected item";
#endif
        return;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();

    if (topRow <= 0) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveUpItem] Cannot move up, already at the top";
#endif
        return;
    }

    bool move_to_top = false;
    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
        move_to_top = true;
        // Move the selected rows to the top (preserve order)
        QList<MAP_KEYDATA> tempItems;
        // Save selected items in order
        for (int row = topRow; row <= bottomRow; ++row) {
            tempItems.append(KeyMappingDataList->at(row));
        }
        // Remove selected items from bottom to top
        for (int row = bottomRow; row >= topRow; --row) {
            KeyMappingDataList->removeAt(row);
        }
        // Insert items at the top in reverse order to maintain original sequence
        for (int i = tempItems.size() - 1; i >= 0; --i) {
            KeyMappingDataList->insert(0, tempItems.at(i));
        }
    }
    else {
        // Move the selected rows up
        for (int row = topRow; row <= bottomRow; ++row) {
            KeyMappingDataList->move(row, row - 1);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    QString debugmessage = QString("[selectedItemsMoveUp] %1: topRow(%2), bottomRow(%3)").arg(move_to_top?"MoveTop":"MoveUp").arg(topRow).arg(bottomRow);
    qDebug().nospace().noquote() << debugmessage;
#endif

#ifdef DEBUG_LOGOUT_ON
    qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
    refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);

    QTableWidgetSelectionRange newSelection;
    if (move_to_top) {
        // Reselect the moved rows at the top
        newSelection = QTableWidgetSelectionRange(0, 0, bottomRow - topRow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
    }
    else {
        // Reselect the moved rows
        newSelection = QTableWidgetSelectionRange(topRow - 1, 0, bottomRow - 1, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
    }
    m_KeyMappingDataTable->clearSelection();
    m_KeyMappingDataTable->setRangeSelected(newSelection, true);

    // Scroll to make the selected items visible
    QTableWidgetItem *itemToScrollTo = m_KeyMappingDataTable->item(newSelection.topRow(), 0);
    if (itemToScrollTo) {
        m_KeyMappingDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList->size()) {
        qDebug("MoveUp:KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList->size());
    }
#endif
}

void QKeyMapper::selectedItemsMoveDown()
{
    if (isMappingDataTableFiltered()) {
        QString message;
        message = tr("Cannot move items while the mapping table is filtered!");
        showWarningPopup(message);
        return;
    }

    QList<QTableWidgetSelectionRange> selectedRanges = m_KeyMappingDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveDownItem] There is no selected item";
#endif
        return;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();

    if (bottomRow >= m_KeyMappingDataTable->rowCount() - 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveDownItem] Cannot move down, already at the bottom";
#endif
        return;
    }

    bool move_to_bottom = false;
    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
        move_to_bottom = true;
        // Move the selected rows to the bottom (preserve order)
        QList<MAP_KEYDATA> tempItems;
        // Save selected items in order
        for (int row = topRow; row <= bottomRow; ++row) {
            tempItems.append(KeyMappingDataList->at(row));
        }
        // Remove selected items from bottom to top
        for (int row = bottomRow; row >= topRow; --row) {
            KeyMappingDataList->removeAt(row);
        }
        // Append items to the bottom in original order
        for (int i = 0; i < tempItems.size(); ++i) {
            KeyMappingDataList->append(tempItems.at(i));
        }
    }
    else {
        // Move the selected rows down
        for (int row = bottomRow; row >= topRow; --row) {
            KeyMappingDataList->move(row, row + 1);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    QString debugmessage = QString("[selectedItemsMoveDown] %1: topRow(%2), bottomRow(%3)").arg(move_to_bottom?"MoveBottom":"MoveDown").arg(topRow).arg(bottomRow);
    qDebug().nospace().noquote() << debugmessage;
#endif

#ifdef DEBUG_LOGOUT_ON
    qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
    refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);

    QTableWidgetSelectionRange newSelection;
    if (move_to_bottom) {
        // Reselect the moved rows at the bottom
        newSelection = QTableWidgetSelectionRange(m_KeyMappingDataTable->rowCount() - (bottomRow - topRow + 1), 0, m_KeyMappingDataTable->rowCount() - 1, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
    }
    else {
        // Reselect the moved rows
        newSelection = QTableWidgetSelectionRange(topRow + 1, 0, bottomRow + 1, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
    }
    m_KeyMappingDataTable->clearSelection();
    m_KeyMappingDataTable->setRangeSelected(newSelection, true);

    // Scroll to make the selected items visible
    QTableWidgetItem *itemToScrollTo = m_KeyMappingDataTable->item(newSelection.bottomRow(), 0);
    if (itemToScrollTo) {
        m_KeyMappingDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList->size()) {
        qDebug("MoveDown:KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList->size());
    }
#endif
}

void QKeyMapper::on_addTabButton_clicked()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[on_addTabButton_clicked]" << "Add tab button clicked!";
#endif
    bool addtab_result = addTabToKeyMappingTabWidget();
    Q_UNUSED(addtab_result);
#ifdef DEBUG_LOGOUT_ON
    if (false == addtab_result) {
        qWarning() << "[on_addTabButton_clicked]" << "addTabToKeyMappingTabWidget failed!";
    }
#endif
}

void QKeyMapper::on_deleteSelectedButton_clicked()
{
    QList<QTableWidgetSelectionRange> selectedRanges = m_KeyMappingDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[DeleteItem] There is no selected item";
#endif
        return;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();

#ifdef DEBUG_LOGOUT_ON
    qDebug("Delete: topRow(%d), bottomRow(%d)", topRow, bottomRow);
#endif

    // Remove the selected rows from bottom to top
    for (int row = bottomRow; row >= topRow; --row) {
        m_KeyMappingDataTable->removeRow(row);
        KeyMappingDataList->removeAt(row);
    }

    // refresh table display
    refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);
    // Update the mouse points list
    updateMousePointsList();

    // Reselect the row at the top of the deleted range, or the last row if the table is empty
    if (m_KeyMappingDataTable->rowCount() > 0) {
        int newRow = qMin(topRow, m_KeyMappingDataTable->rowCount() - 1);
        QTableWidgetSelectionRange newSelection = QTableWidgetSelectionRange(newRow, 0, newRow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        m_KeyMappingDataTable->clearSelection();
        m_KeyMappingDataTable->setRangeSelected(newSelection, true);
    }

#ifdef DEBUG_LOGOUT_ON
    if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList->size()) {
        qDebug("Delete: KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList->size());
    }
#endif
}

void QKeyMapper::on_clearallButton_clicked()
{
    QString message = tr("Are you sure you want to clear all data in the mapping table?");

    QMessageBox::StandardButton reply = QMessageBox::warning(this, PROGRAM_NAME, message, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_KeyMappingDataTable->clearContents();
        m_KeyMappingDataTable->setRowCount(0);
        KeyMappingDataList->clear();
        ScreenMousePointsList.clear();
        WindowMousePointsList.clear();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_clearallButton_clicked]" << "User press confirm button of ClearAll Warning MessageBox.";
#endif
    }
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
    /* Check L-Ctrl+S to Save settings */
    if (QKeyMapper::KEYMAP_IDLE == QKeyMapper::getInstance()->m_KeyMapStatus
        && keyevent->key() == Qt::Key_S
        && QT_KEY_L_CTRL == (keyevent->nativeModifiers() & QT_KEY_L_CTRL)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[KeyListComboBox::keyPressEvent]" << "\"L-Ctrl+S\" CombinationKey Pressed -> saveKeyMappingDataToFile()";
#endif
        QKeyMapper::getInstance()->saveKeyMapSetting();
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyListComboBox_KeyPress]" << "Key:" << (Qt::Key)keyevent->key() << "Modifiers:" << keyevent->modifiers();
    qDebug("[KeyListComboBox_KeyPress] VirtualKey(0x%08X), ScanCode(0x%08X), nModifiers(0x%08X)", keyevent->nativeVirtualKey(), keyevent->nativeScanCode(), keyevent->nativeModifiers());
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
            if (objectName() != SETUPDIALOG_MAPKEY_COMBOBOX_NAME) {
                keycodeString = QString("L-Ctrl");
            }
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
        if (objectName() == ORIKEY_COMBOBOX_NAME && keycodeString == QString("Backspace")) {
            this->setCurrentText(QString());
        }
        else if ((objectName() == SETUPDIALOG_ORIKEY_COMBOBOX_NAME || objectName() == SETUPDIALOG_MAPKEY_COMBOBOX_NAME)
            && keycodeString == QString("Esc")) {
            QComboBox::keyPressEvent(keyevent);
        }
        else {
            this->setCurrentText(keycodeString);

#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[KeyListComboBox_KeyPress]" << "convert to VirtualKeyCodeMap:" << keycodeString;
#endif
        }
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[KeyListComboBox_KeyPress]" << "Unknown key not found in VirtualKeyCodeMap.";
#endif
    }

    // QComboBox::keyPressEvent(keyevent);
}

void KeyListComboBox::mousePressEvent(QMouseEvent *event)
{
    if (objectName() == ORIKEY_COMBOBOX_NAME) {
        if (event->button() == Qt::RightButton) {
            QString currentOriKeyText = QKeyMapper::getCurrentOriKeyText();
            QString currentOriCombinationKeyText = QKeyMapper::getCurrentOriCombinationKeyText();
            if (currentOriKeyText.isEmpty() == false
                && QKeyMapper_Worker::CombinationKeysList.contains(currentOriKeyText)) {
                QString newCombinationKeyText;
                if (currentOriCombinationKeyText.isEmpty()) {
                    newCombinationKeyText = currentOriKeyText;
                }
                else {
                    newCombinationKeyText = currentOriCombinationKeyText + QString(SEPARATOR_PLUS) + currentOriKeyText;
                }
                QKeyMapper::getInstance()->setCurrentOriCombinationKeyText(newCombinationKeyText);
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[KeyListComboBox_MousePress]" << "Set new CombinationKeyText ->" << newCombinationKeyText;
#endif
            }
        }
    }
    else if (objectName() == SETUPDIALOG_ORIKEY_COMBOBOX_NAME) {
        if (event->button() == Qt::RightButton) {
            QString currentOriKeyText = QItemSetupDialog::getOriginalKeyText();
            QString currentOriKeyListText = QItemSetupDialog::getCurrentOriKeyListText();
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[KeyListComboBox_MousePress]" << "Mouse Right Click on SetupDialog_OriginalKeyListComboBox.";
            qDebug().nospace() << "[KeyListComboBox_MousePress]" << " currentOriKeyText -> " << currentOriKeyText << ", currentOriKeyListText -> " << currentOriKeyListText;
#endif
            if (currentOriKeyListText.isEmpty() == false) {
                QString newOriKeyText;
                if (currentOriKeyText.isEmpty()) {
                    newOriKeyText = currentOriKeyListText;
                }
                else {
                    int cursorPos = QItemSetupDialog::getOriginalKeyCursorPosition();
                    bool isCursorAtEnd = (cursorPos == currentOriKeyText.length());

                    if (isCursorAtEnd) {
                        if (currentOriKeyListText == SEPARATOR_LONGPRESS
                            || currentOriKeyListText == SEPARATOR_DOUBLEPRESS) {
                            newOriKeyText = currentOriKeyText + currentOriKeyListText;
                        }
                        else {
                            newOriKeyText = currentOriKeyText + QString(SEPARATOR_PLUS) + currentOriKeyListText;
                        }
                    }
                    else {
                        if (currentOriKeyListText == SEPARATOR_LONGPRESS
                            || currentOriKeyListText == SEPARATOR_DOUBLEPRESS) {
                            newOriKeyText = currentOriKeyText.left(cursorPos) + currentOriKeyListText + currentOriKeyText.right(currentOriKeyText.length() - cursorPos);
                        }
                        else {
                            newOriKeyText = currentOriKeyText.left(cursorPos) + currentOriKeyListText + QString(SEPARATOR_PLUS) + currentOriKeyText.right(currentOriKeyText.length() - cursorPos);
                        }
                    }
                }
                QItemSetupDialog::getInstance()->setOriginalKeyText(newOriKeyText);
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[KeyListComboBox_MousePress]" << "SetupDialog Set new OriginalKeyText ->" << newOriKeyText;
#endif
            }
        }
    }
    else if (objectName() == SETUPDIALOG_MAPKEY_COMBOBOX_NAME) {
        if (event->button() == Qt::RightButton) {
            QString currentMapKeyText = QItemSetupDialog::getMappingKeyText();
            QString currentMapKeyListText = QItemSetupDialog::getCurrentMapKeyListText();
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[KeyListComboBox_MousePress]" << "Mouse Right Click on SetupDialog_MappingKeyListComboBox.";
            qDebug().nospace() << "[KeyListComboBox_MousePress]" << " currentMapKeyText -> " << currentMapKeyText << ", currentMapKeyListText -> " << currentMapKeyListText;
#endif
            if (currentMapKeyListText.isEmpty() == false) {
                if (currentMapKeyListText.startsWith(MOUSE_BUTTON_PREFIX)) {
                    if (currentMapKeyListText.endsWith(MOUSE_SCREENPOINT_POSTFIX)) {
                        currentMapKeyListText = currentMapKeyListText.remove(MOUSE_SCREENPOINT_POSTFIX) + QString("(,)");
                    }
                    else if (currentMapKeyListText.endsWith(MOUSE_WINDOWPOINT_POSTFIX)) {
                        currentMapKeyListText = currentMapKeyListText.remove(MOUSE_WINDOWPOINT_POSTFIX) + QString(":W(,)");
                    }
                }

                QString newMapKeyText;
                if (currentMapKeyText.isEmpty()) {
                    newMapKeyText = currentMapKeyListText;
                }
                else {
                    int cursorPos = QItemSetupDialog::getMappingKeyCursorPosition();
                    bool isCursorAtEnd = (cursorPos == currentMapKeyText.length());

                    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
                        if (currentMapKeyListText == SEPARATOR_WAITTIME
                            || currentMapKeyListText == PREFIX_SEND_DOWN
                            || currentMapKeyListText == PREFIX_SEND_UP
                            || currentMapKeyListText == PREFIX_SEND_BOTH
                            || currentMapKeyListText == PREFIX_SEND_EXCLUSION) {
                            newMapKeyText = currentMapKeyText + currentMapKeyListText;
                        }
                        else {
                            newMapKeyText = currentMapKeyText + QString(SEPARATOR_NEXTARROW) + currentMapKeyListText;
                        }
                    }
                    else {
                        if (isCursorAtEnd) {
                            if (currentMapKeyListText == SEPARATOR_WAITTIME
                                || currentMapKeyListText == PREFIX_SEND_DOWN
                                || currentMapKeyListText == PREFIX_SEND_UP
                                || currentMapKeyListText == PREFIX_SEND_BOTH
                                || currentMapKeyListText == PREFIX_SEND_EXCLUSION) {
                                newMapKeyText = currentMapKeyText + currentMapKeyListText;
                            }
                            else {
                                newMapKeyText = currentMapKeyText + QString(SEPARATOR_PLUS) + currentMapKeyListText;
                            }
                        }
                        else {
                            if (currentMapKeyListText == SEPARATOR_WAITTIME
                                || currentMapKeyListText == PREFIX_SEND_DOWN
                                || currentMapKeyListText == PREFIX_SEND_UP
                                || currentMapKeyListText == PREFIX_SEND_BOTH
                                || currentMapKeyListText == PREFIX_SEND_EXCLUSION) {
                                newMapKeyText = currentMapKeyText.left(cursorPos) + currentMapKeyListText + currentMapKeyText.right(currentMapKeyText.length() - cursorPos);
                            }
                            else {
                                newMapKeyText = currentMapKeyText.left(cursorPos) + currentMapKeyListText + QString(SEPARATOR_PLUS) + currentMapKeyText.right(currentMapKeyText.length() - cursorPos);
                            }
                        }
                    }
                }
                QItemSetupDialog::getInstance()->setMappingKeyText(newMapKeyText);
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[KeyListComboBox_MousePress]" << "SetupDialog Set new MappingKeyText ->" << newMapKeyText;
#endif
            }
        }
    }

    QComboBox::mousePressEvent(event);
}

void KeyListComboBox::showPopup()
{
    QComboBox::showPopup();

    QAbstractItemView *view = this->view();
    if (view) {
        QModelIndex idx = model()->index(currentIndex(), 0);
        view->setCurrentIndex(idx);
        // view->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

QPopupNotification::QPopupNotification(QWidget *parent)
    : QWidget(parent)
    , m_BackgroundFrame(new QFrame(this))
    , m_IconLabel(new QLabel(this))
    , m_TextLabel(new QLabel(this))
    , m_Layout(new QHBoxLayout(this))
    , m_Timer(this)
    , m_StartAnimation(new QPropertyAnimation(this, "windowOpacity", this))
    , m_StopAnimation(new QPropertyAnimation(this, "windowOpacity", this))
    , m_CurrentPopupOptions()
{
    // Config Window Attribute
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // Config Layout
    m_Layout->setContentsMargins(0, 0, 0, 0);
    m_Layout->addWidget(m_IconLabel);
    m_Layout->addWidget(m_TextLabel);
    m_BackgroundFrame->setLayout(m_Layout);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_BackgroundFrame);
    setLayout(mainLayout);

    // Hide the Icon Label initially
    m_IconLabel->hide();

    // Config animations
    // QObject::connect(m_StopAnimation, &QPropertyAnimation::finished, this, &QWidget::hide);
    QObject::connect(m_StopAnimation, &QPropertyAnimation::finished, this, [this]() {
        this->hide();
        m_IconLabel->clear();
        m_TextLabel->clear();
    });

    QObject::connect(&m_Timer, &QTimer::timeout, this, &QPopupNotification::hideNotification);
    m_Timer.setSingleShot(true);
}

void QPopupNotification::showPopupNotification(const QString &message, const PopupNotificationOptions &options)
{
    m_StartAnimation->stop();
    m_StopAnimation->stop();
    m_IconLabel->clear();
    m_TextLabel->clear();
    hide(); // Hide the window before updating content

    if (options.displayDuration < 0 || options.position == NOTIFICATION_POSITION_NONE) {
        return;
    }

    // --- 1. Icon Handling ---
    m_IconLabel->hide(); // Hide icon by default
    QIcon icon_loaded(options.iconPath);
    bool hasIcon = (options.iconPosition != TAB_CUSTOMIMAGE_SHOW_NONE) && !options.iconPath.isEmpty() && !icon_loaded.isNull();

    if (hasIcon) {
        // Ensure widgets are in the correct order in the layout
        m_Layout->removeWidget(m_IconLabel);
        m_Layout->removeWidget(m_TextLabel);
        if (options.iconPosition == TAB_CUSTOMIMAGE_SHOW_RIGHT) {
            m_Layout->addWidget(m_TextLabel);
            m_Layout->addWidget(m_IconLabel);
        }
        else {
            m_Layout->addWidget(m_IconLabel);
            m_Layout->addWidget(m_TextLabel);
        }
        m_Layout->setSpacing(options.iconPadding);
    } else {
        m_Layout->setSpacing(0);
    }

    // --- 2. Frame & Label StyleSheet Setup ---
    m_IconLabel->setStyleSheet("background: transparent;");
    QString textLabelStyleSheet = QString("background: transparent; padding: %1px; color: %2;")
            .arg(options.padding)
            .arg(options.color);
    m_TextLabel->setStyleSheet(textLabelStyleSheet);

    QString backgroundStyleSheet = QString("background-color: rgba(%1, %2, %3, %4); border-radius: %5px;")
            .arg(options.backgroundColor.red())
            .arg(options.backgroundColor.green())
            .arg(options.backgroundColor.blue())
            .arg(options.backgroundColor.alpha())
            .arg(options.borderRadius);
    m_BackgroundFrame->setStyleSheet(backgroundStyleSheet);

    // --- 3. Font and Text Setup ---
    QFont customFont(FONTNAME_ENGLISH, options.size, options.fontWeight, options.fontItalic);
    m_TextLabel->setFont(customFont);
    m_TextLabel->setText(message);
    m_TextLabel->adjustSize(); // Adjust text label first to get its height

    // --- 4. Icon Scaling and Display ---
    if (hasIcon) {
        int textHeight = m_TextLabel->height();
        QSize iconSize = QSize(textHeight, textHeight);
        m_IconLabel->setFixedSize(iconSize);
#ifdef DEBUG_LOGOUT_ON
        QList<QSize> iconsizeList = icon_loaded.availableSizes();
        qDebug() << "[QPopupNotification::showPopupNotification]" << "Icon availableSizes:" << iconsizeList;
#endif
        QPixmap scaled_pixmap = icon_loaded.pixmap(iconSize);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[QPopupNotification::showPopupNotification]" << " Scaled(" << iconSize << ") pixmap size: " << scaled_pixmap.size();
#endif
        m_IconLabel->setPixmap(scaled_pixmap);
        m_IconLabel->show();
    }

    // --- 5. Adjust Main Widget Size and Position ---
    adjustSize(); // Adjust the main widget's size to fit the layout content

    // Position the notification based on the options
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    qreal scale = QKeyMapper::s_UI_scale_value;
    int x_right_offset = 20;
    if (scale < 1.10) x_right_offset = 30;
    int x = screenGeometry.width() - width() - x_right_offset; // Use the main widget's width
    int y = 10;

    if (options.position == NOTIFICATION_POSITION_TOP_LEFT) {
        x = 10; y = 10;
    } else if (options.position == NOTIFICATION_POSITION_TOP_CENTER) {
        x = (screenGeometry.width() - width()) / 2; y = 10;
    } else if (options.position == NOTIFICATION_POSITION_TOP_RIGHT) {
        x = screenGeometry.width() - width() - x_right_offset; y = 10;
    } else if (options.position == NOTIFICATION_POSITION_BOTTOM_LEFT) {
        x = 10; y = screenGeometry.height() - height() - 30;
    } else if (options.position == NOTIFICATION_POSITION_BOTTOM_CENTER) {
        x = (screenGeometry.width() - width()) / 2; y = screenGeometry.height() - height() - 30;
    } else if (options.position == NOTIFICATION_POSITION_BOTTOM_RIGHT) {
        x = screenGeometry.width() - width() - x_right_offset; y = screenGeometry.height() - height() - 30;
    }

    // Apply offsets
    x += options.xOffset;
    y += options.yOffset;
    move(x, y);

    // --- 6. Opacity and Animations ---
    setWindowOpacity(std::clamp(options.windowOpacity, 0.0, 1.0));

    // Fade-in animation
    if (options.fadeInDuration > 0) {
        m_StartAnimation->setDuration(options.fadeInDuration);
        m_StartAnimation->setStartValue(0.0);
        m_StartAnimation->setEndValue(windowOpacity());
        m_StartAnimation->start(QAbstractAnimation::KeepWhenStopped);
    }

    // Show the notification and start the timer
    show();

    // If displayDuration is 0, keep the notification visible permanently (never disappear)
    // Otherwise, start the timer to auto-hide after the specified duration
    if (options.displayDuration > 0) {
        m_Timer.start(options.displayDuration);
    }

    // Save the current options
    m_CurrentPopupOptions = options;
}

void QPopupNotification::hideNotification()
{
    m_StopAnimation->stop();

    if (m_CurrentPopupOptions.fadeOutDuration > 0) {
        m_StopAnimation->setDuration(m_CurrentPopupOptions.fadeOutDuration);
        m_StopAnimation->setStartValue(windowOpacity());
        m_StopAnimation->setEndValue(0.0);
        m_StopAnimation->start(QAbstractAnimation::KeepWhenStopped);
    }
    else {
        m_IconLabel->clear();
        m_TextLabel->clear();
        hide();
    }
}

QFloatingIconWindow::QFloatingIconWindow(QWidget *parent)
    : QWidget(parent)
    , m_IconLabel(nullptr)
    , m_CurrentOpacity(1.0)
    , m_MousePassThrough(false)
    , m_Dragging(false)
    , m_Resizing(false)
    , m_LastMappingHWND(nullptr)
{
    // Set window flags for a topmost, frameless window
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose, false);

    // Enable mouse tracking to receive mouse move events even without mouse button pressed
    setMouseTracking(true);

    // Create and setup the icon label
    m_IconLabel = new QLabel(this);
    m_IconLabel->setAlignment(Qt::AlignCenter);
    m_IconLabel->setScaledContents(true);

    // Make the icon label transparent for mouse events so parent window can receive them
    m_IconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // Set default size and position
    resize(FLOATINGWINDOW_SIZE_DEFAULT);
    move(FLOATINGWINDOW_POSITION_DEFAULT);

    // Hide initially
    hide();

    QObject::connect(this, &QFloatingIconWindow::windowPositionChanged, this, &QFloatingIconWindow::onWindowPositionChanged);
    QObject::connect(this, &QFloatingIconWindow::windowSizeChanged, this, &QFloatingIconWindow::onWindowSizeChanged);
    QObject::connect(this, &QFloatingIconWindow::windowOpacityChanged, this, &QFloatingIconWindow::onWindowOpacityChanged);
    // QObject::connect(this, &QFloatingIconWindow::windowMousePassThroughChanged, this, &QFloatingIconWindow::onWindowMousePassThroughChanged);
    QObject::connect(QKeyMapper::getInstance(), &QKeyMapper::switchFloatingWindowMousePassThrough_Signal, this, &QFloatingIconWindow::onSwitchFloatingWindowMousePassThrough);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::QFloatingIconWindow] Floating icon window created";
#endif

    this->createWinId();
}

QFloatingIconWindow::~QFloatingIconWindow()
{
    if (m_IconLabel) {
        delete m_IconLabel;
        m_IconLabel = nullptr;
    }
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::~QFloatingIconWindow] Floating icon window destroyed";
#endif
}

void QFloatingIconWindow::showFloatingWindow(const FloatingWindowOptions &options)
{
    hide(); // Hide the window before updating content

    if (!isValidReferencePoint(options.referencePoint)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QFloatingIconWindow::showFloatingWindow] Error: Invalid reference point specified" << options.referencePoint;
#endif
        return;
    }

    m_CurrentOptions = options;
    m_CurrentOpacity = options.windowOpacity;
    m_MousePassThrough = options.mousePassThrough;

    // Calculate absolute position based on reference point (in physical coordinates)
    QPoint physicalAbsolutePosition;

    if (isWindowReferencePoint(options.referencePoint)) {
        // Window-based reference point - need valid mapping window
        if (QKeyMapper::s_CurrentMappingHWND == nullptr) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QFloatingIconWindow::showFloatingWindow] Error: Window reference point specified but no current mapping window available";
#endif
            return; // Don't show floating window
        }

        physicalAbsolutePosition = calculateAbsolutePosition(options.position, options.referencePoint);
        if (physicalAbsolutePosition == QPoint(-1, -1)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QFloatingIconWindow::showFloatingWindow] Error: Cannot calculate absolute position for window reference point" << options.referencePoint;
#endif
            return; // Don't show floating window
        }

        // Store current window state for tracking
        m_LastMappingHWND = QKeyMapper::s_CurrentMappingHWND;
        getWindowClientRect(QKeyMapper::s_CurrentMappingHWND, m_LastWindowClientRect);
    } else {
        // Screen-based reference point (using physical coordinates)
        physicalAbsolutePosition = calculateAbsolutePosition(options.position, options.referencePoint);
        if (physicalAbsolutePosition == QPoint(-1, -1)) {
            // Fallback to direct position if calculation fails
            physicalAbsolutePosition = options.position;
        }

        // Clear window tracking for screen-based positioning
        m_LastMappingHWND = nullptr;
        m_LastWindowClientRect = QRect();
    }

    // Convert physical coordinates to Qt coordinates for window positioning
    QPoint qtAbsolutePosition = physicalToQtCoordinates(physicalAbsolutePosition);

    // Set window geometry (ensure square shape)
    int squareSize = qMax(options.size.width(), options.size.height());
    resize(squareSize, squareSize);
    move(qtAbsolutePosition);

    // Load icon if path is provided
    QFileInfo icon_fileinfo(options.iconPath);
    if (icon_fileinfo.exists()) {
        loadIconFromPath(options.iconPath);
    }

    // Update window appearance
    setMousePassThroughEnabled(options.mousePassThrough);
    updateIconDisplay();
    updateWindowStyle();

    // Show the window without activating it (to avoid stealing focus)
    show();

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::showFloatingWindow] Shown at Qt position:" << qtAbsolutePosition
             << "physical position:" << physicalAbsolutePosition
             << "from relative position:" << options.position
             << "with reference point:" << options.referencePoint
             << "size:" << QSize(squareSize, squareSize) << "opacity:" << options.windowOpacity;
#endif
}

void QFloatingIconWindow::hideFloatingWindow()
{
    hide();
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::hideFloatingWindow] Window hidden";
#endif
}

void QFloatingIconWindow::updateWindowSettings(const FloatingWindowOptions &options)
{
    m_CurrentOptions = options;
    m_CurrentOpacity = options.windowOpacity;
    m_MousePassThrough = options.mousePassThrough;

    // Calculate absolute position from relative position and reference point (in physical coordinates)
    QPoint physicalAbsolutePosition = calculateAbsolutePosition(options.position, options.referencePoint);
    QPoint qtAbsolutePosition = physicalToQtCoordinates(physicalAbsolutePosition);

    // Update geometry if changed (ensure square shape)
    int squareSize = qMax(options.size.width(), options.size.height());
    QSize newSize(squareSize, squareSize);
    if (size() != newSize) {
        resize(newSize);
    }
    if (pos() != qtAbsolutePosition) {
        move(qtAbsolutePosition);
    }

    // Reload icon if path changed
    QFileInfo icon_fileinfo(options.iconPath);
    if (m_CurrentOptions.iconPath != options.iconPath && icon_fileinfo.exists()) {
        loadIconFromPath(options.iconPath);
    }

    // Update appearance
    updateIconDisplay();
    updateWindowStyle();

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::updateWindowSettings] Updated - Qt position:" << qtAbsolutePosition
             << "physical position:" << physicalAbsolutePosition
             << "relative position:" << options.position
             << "reference point:" << options.referencePoint
             << "size:" << newSize << "opacity:" << options.windowOpacity;
#endif
}

void QFloatingIconWindow::setMousePassThroughEnabled(bool enabled)
{
    if (enabled) {
        enableMousePassThrough();
    }
    else {
        disableMousePassThrough();
    }
}

void QFloatingIconWindow::onWindowPositionChanged(const QPoint &newPosition)
{
    int current_tabindex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    if (current_tabindex < 0 || current_tabindex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::onWindowPositionChanged] Floating Window Position ->" << newPosition
             << ", Current Tab Index:" << current_tabindex;
#endif

    if (newPosition.x() < FLOATINGWINDOW_POSITION_MIN.x()
        || newPosition.y() < FLOATINGWINDOW_POSITION_MIN.y()
        || newPosition.x() > FLOATINGWINDOW_POSITION_MAX.x()
        || newPosition.y() > FLOATINGWINDOW_POSITION_MAX.y()) {
        return;
    }
    QKeyMapper::s_KeyMappingTabInfoList[current_tabindex].FloatingWindow_Position = newPosition;
}

void QFloatingIconWindow::onWindowSizeChanged(const QSize &newSize)
{
    int current_tabindex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    if (current_tabindex < 0 || current_tabindex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::onWindowSizeChanged] Floating Window Size ->" << newSize
             << ", Current Tab Index:" << current_tabindex;
#endif

    if (newSize.width() < FLOATINGWINDOW_SIZE_MIN
        || newSize.height() < FLOATINGWINDOW_SIZE_MIN
        || newSize.width() > FLOATINGWINDOW_SIZE_MAX
        || newSize.height() > FLOATINGWINDOW_SIZE_MAX) {
        return;
    }
    QKeyMapper::s_KeyMappingTabInfoList[current_tabindex].FloatingWindow_Size = newSize;
}

void QFloatingIconWindow::onWindowOpacityChanged(double newOpacity)
{
    int current_tabindex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    if (current_tabindex < 0 || current_tabindex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::onWindowOpacityChanged] Floating Window Opacity ->" << newOpacity
             << ", Current Tab Index:" << current_tabindex;
#endif

    if (newOpacity < FLOATINGWINDOW_OPACITY_MIN || newOpacity > FLOATINGWINDOW_OPACITY_MAX) {
        return;
    }
    QKeyMapper::s_KeyMappingTabInfoList[current_tabindex].FloatingWindow_Opacity = newOpacity;
}

#if 0
void QFloatingIconWindow::onWindowMousePassThroughChanged(bool enabled)
{
    int current_tabindex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    if (current_tabindex < 0 || current_tabindex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::onWindowMousePassThroughChanged] Floating Window Mouse PassThrough ->" << enabled
             << ", Current Tab Index:" << current_tabindex;
#endif

    if (m_MousePassThrough == enabled) {
        return;
    }

    m_MousePassThrough = enabled;
    QKeyMapper::s_KeyMappingTabInfoList[current_tabindex].FloatingWindow_MousePassThrough = enabled;

    if (enabled) {
        enableMousePassThrough();
    }
    else {
        disableMousePassThrough();
    }
}
#endif

void QFloatingIconWindow::onSwitchFloatingWindowMousePassThrough()
{
    QKeyMapper::KeyMapStatus keymap_status = QKeyMapper::getInstance()->m_KeyMapStatus;
    if (keymap_status != QKeyMapper::KEYMAP_MAPPING_MATCHED
        && keymap_status != QKeyMapper::KEYMAP_MAPPING_GLOBAL) {
        return;
    }

    if (!isVisible()) {
        return;
    }

    int current_tabindex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    if (current_tabindex < 0 || current_tabindex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    // Check if mouse cursor is within the floating window bounds
    QPoint globalMousePos = QCursor::pos();
    QPoint localMousePos = mapFromGlobal(globalMousePos);
    if (!rect().contains(localMousePos)) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::onSwitchFloatingWindowMousePassThrough] Switch Floating Window Mouse PassThrough on current state :" << m_MousePassThrough;
#endif

    if (m_MousePassThrough) {
        m_MousePassThrough = false;
    }
    else {
        m_MousePassThrough = true;
    }
    QKeyMapper::s_KeyMappingTabInfoList[current_tabindex].FloatingWindow_MousePassThrough = m_MousePassThrough;
    setMousePassThroughEnabled(m_MousePassThrough);
}

void QFloatingIconWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set up the background brush with opacity
    QColor bgColor = m_CurrentOptions.backgroundColor;
    painter.setBrush(QBrush(bgColor));
    painter.setPen(Qt::NoPen);

    // Draw rounded rectangle
    if (m_CurrentOptions.borderRadius > 0) {
        painter.drawRoundedRect(rect(), m_CurrentOptions.borderRadius, m_CurrentOptions.borderRadius);
    } else {
        painter.drawRect(rect());
    }

    // Draw resize handle indicator in bottom-right corner
    // if (isInResizeHandle(mapFromGlobal(QCursor::pos()))) {
    //     QRect handleRect = getResizeHandleRect();
    //     painter.setBrush(QBrush(QColor(100, 100, 100, 100)));
    //     painter.drawRect(handleRect);
    // }
}

void QFloatingIconWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (isInResizeHandle(event->pos())) {
            // Start resizing
            m_Resizing = true;
            m_ResizeStartSize = size();
            // Use physical cursor position for consistent coordinates
            m_ResizeStartMousePos = getPhysicalCursorPosition();
            setCursor(Qt::SizeFDiagCursor);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QFloatingIconWindow::mousePressEvent] Start resizing from size:" << m_ResizeStartSize
                     << "physical cursor pos:" << m_ResizeStartMousePos;
#endif
        } else {
            // Start dragging
            m_Dragging = true;
            // Calculate drag offset using physical coordinates
            QPoint physicalWindowPos = qtToPhysicalCoordinates(frameGeometry().topLeft());
            QPoint physicalCursorPos = getPhysicalCursorPosition();
            m_DragStartPosition = physicalCursorPos - physicalWindowPos;
            setCursor(Qt::ClosedHandCursor);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QFloatingIconWindow::mousePressEvent] Start dragging from position:" << pos()
                     << "physical window pos:" << physicalWindowPos << "physical cursor pos:" << physicalCursorPos;
#endif
        }
    }
    QWidget::mousePressEvent(event);
}

void QFloatingIconWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_Resizing && (event->buttons() & Qt::LeftButton)) {
        // Handle window resizing (maintain 1:1 aspect ratio)
        QPoint physicalMousePos = getPhysicalCursorPosition();
        QPoint mouseDelta = physicalMousePos - m_ResizeStartMousePos;

        // Calculate new size (use the larger delta to maintain square shape)
        int deltaSize = qMax(mouseDelta.x(), mouseDelta.y());
        int newSize = qMax(FLOATINGWINDOW_SIZE_MIN, qMin(FLOATINGWINDOW_SIZE_MAX, m_ResizeStartSize.width() + deltaSize));

        QSize newWindowSize(newSize, newSize);
        if (newWindowSize != size()) {
            resize(newWindowSize);
            m_CurrentOptions.size = newWindowSize;
            emit windowSizeChanged(newWindowSize);
            // emit windowSettingsChanged(m_CurrentOptions);
        }
        // Keep resize cursor during resize operation
        setCursor(Qt::SizeFDiagCursor);
    } else if (m_Dragging && (event->buttons() & Qt::LeftButton)) {
        // Handle window dragging with reference point consideration using physical coordinates
        QPoint physicalMousePos = getPhysicalCursorPosition();
        QPoint newPhysicalPos = physicalMousePos - m_DragStartPosition;

        // Convert physical position back to Qt coordinates for window positioning
        QPoint newQtPos = physicalToQtCoordinates(newPhysicalPos);

        if (newQtPos != pos()) {
            move(newQtPos);

            // Calculate the new relative position based on current reference point using physical coordinates
            QPoint newRelativePos = calculateRelativePosition(newPhysicalPos, m_CurrentOptions.referencePoint);
            m_CurrentOptions.position = newRelativePos;

            emit windowPositionChanged(newQtPos);
            // emit windowSettingsChanged(m_CurrentOptions);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QFloatingIconWindow::mouseMoveEvent] Dragged to Qt pos:" << newQtPos
                     << "physical pos:" << newPhysicalPos << "relative pos:" << newRelativePos
                     << "reference point:" << m_CurrentOptions.referencePoint;
#endif
        }
        // Keep drag cursor during drag operation
        setCursor(Qt::ClosedHandCursor);
    } else {
        // Update cursor based on position when not dragging or resizing
        updateCursorForPosition(event->pos());
    }

    QWidget::mouseMoveEvent(event);
}

void QFloatingIconWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
#ifdef DEBUG_LOGOUT_ON
        if (m_Dragging) {
            qDebug() << "[QFloatingIconWindow::mouseReleaseEvent] Finished dragging to position:" << pos();
        }
        if (m_Resizing) {
            qDebug() << "[QFloatingIconWindow::mouseReleaseEvent] Finished resizing to size:" << size();
        }
#endif
        m_Dragging = false;
        m_Resizing = false;

        // After releasing mouse button, update cursor based on current position
        updateCursorForPosition(event->pos());

        emit windowPositionChanged(pos());
        emit windowSizeChanged(size());
    }
    QWidget::mouseReleaseEvent(event);
}

void QFloatingIconWindow::wheelEvent(QWheelEvent *event)
{
    // Adjust opacity with mouse wheel
    double opacityChange = (event->angleDelta().y() > 0) ? WHEEL_OPACITY_STEP : -WHEEL_OPACITY_STEP;
    double newOpacity = qMax(FLOATINGWINDOW_OPACITY_MIN, qMin(FLOATINGWINDOW_OPACITY_MAX, m_CurrentOpacity + opacityChange));

    if (qAbs(newOpacity - m_CurrentOpacity) > 0.001) {
        m_CurrentOpacity = newOpacity;
        m_CurrentOptions.windowOpacity = newOpacity;
        updateWindowStyle();
        emit windowOpacityChanged(newOpacity);
        // emit windowSettingsChanged(m_CurrentOptions);
    }

    QWidget::wheelEvent(event);
}

void QFloatingIconWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateIconDisplay();

    // After resize, update cursor if mouse is still over the window
    if (underMouse()) {
        QPoint mousePos = mapFromGlobal(QCursor::pos());
        updateCursorForPosition(mousePos);
    }
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void QFloatingIconWindow::enterEvent(QEnterEvent *event)
{
    // When mouse enters the window, update cursor based on position
    QPoint localPos = event->position().toPoint();
#else
void QFloatingIconWindow::enterEvent(QEvent *event)
{
    // When mouse enters the window, update cursor based on position
    QPoint localPos = mapFromGlobal(QCursor::pos());
#endif
    updateCursorForPosition(localPos);
    QWidget::enterEvent(event);
}

void QFloatingIconWindow::leaveEvent(QEvent *event)
{
    // When mouse leaves the window, restore default cursor
    setCursor(Qt::ArrowCursor);
    QWidget::leaveEvent(event);
}

void QFloatingIconWindow::updateWindowStyle()
{
    // Set window opacity
    setWindowOpacity(m_CurrentOpacity);

    // Update the widget to trigger a repaint
    update();
}

void QFloatingIconWindow::loadIconFromPath(const QString &iconPath)
{
    m_LoadedIcon = QIcon(iconPath);
    if (m_LoadedIcon.isNull()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QFloatingIconWindow::loadIconFromPath] Failed to load icon from:" << iconPath;
#endif
    } else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QFloatingIconWindow::loadIconFromPath] Successfully loaded icon from:" << iconPath;
#endif
    }
}

void QFloatingIconWindow::updateIconDisplay()
{
    if (!m_IconLabel || m_LoadedIcon.isNull()) {
        return;
    }

    // Calculate icon size considering padding
    int availableSize = qMin(width(), height()) - (m_CurrentOptions.iconPadding * 2);
    availableSize = qMax(1, availableSize); // Ensure positive size

    QSize iconSize(availableSize, availableSize);
    QPixmap iconPixmap = m_LoadedIcon.pixmap(iconSize);

    // Set the pixmap to the label
    m_IconLabel->setPixmap(iconPixmap);

    // Update label geometry with padding
    int padding = m_CurrentOptions.iconPadding;
    m_IconLabel->setGeometry(padding, padding, width() - 2 * padding, height() - 2 * padding);
}

QRect QFloatingIconWindow::getResizeHandleRect() const
{
    return QRect(width() - RESIZE_HANDLE_SIZE, height() - RESIZE_HANDLE_SIZE,
                 RESIZE_HANDLE_SIZE, RESIZE_HANDLE_SIZE);
}

bool QFloatingIconWindow::isInResizeHandle(const QPoint &pos) const
{
    return getResizeHandleRect().contains(pos);
}

void QFloatingIconWindow::updateCursorForPosition(const QPoint &pos)
{
    if (m_Dragging || m_Resizing) {
        // Don't change cursor during active drag/resize operations
        return;
    }

    if (isInResizeHandle(pos)) {
        setCursor(Qt::SizeFDiagCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void QFloatingIconWindow::enableMousePassThrough()
{
    HWND hwnd = reinterpret_cast<HWND>(this->winId());
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
}

void QFloatingIconWindow::disableMousePassThrough()
{
    HWND hwnd = reinterpret_cast<HWND>(this->winId());
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle &= ~WS_EX_TRANSPARENT;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
}

// ============================================================================
// Reference Point Helper Functions
// ============================================================================

bool QFloatingIconWindow::isWindowReferencePoint(int referencePoint) const
{
    return (referencePoint >= QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPLEFT &&
            referencePoint <= QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMCENTER);
}

bool QFloatingIconWindow::getWindowClientRect(HWND hwnd, QRect &clientRect) const
{
    if (hwnd == nullptr) {
        return false;
    }

    WINDOWINFO winInfo;
    winInfo.cbSize = sizeof(WINDOWINFO);
    if (!GetWindowInfo(hwnd, &winInfo)) {
        return false;
    }

    RECT rect = winInfo.rcClient;
    clientRect = QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    return true;
}

QPoint QFloatingIconWindow::getScreenReferencePoint(int referencePoint) const
{
    // Use Windows API to get physical screen dimensions (excluding DPI scaling)
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // For multi-monitor setups, use virtual screen dimensions
    int virtualScreenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualScreenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int virtualScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtualScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // Use primary screen for single monitor or virtual screen for multi-monitor
    int useWidth = (virtualScreenWidth > screenWidth) ? virtualScreenWidth : screenWidth;
    int useHeight = (virtualScreenHeight > screenHeight) ? virtualScreenHeight : screenHeight;
    int useLeft = (virtualScreenWidth > screenWidth) ? virtualScreenLeft : 0;
    int useTop = (virtualScreenHeight > screenHeight) ? virtualScreenTop : 0;

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::getScreenReferencePoint] Physical screen size:"
             << useWidth << "x" << useHeight << "at" << QPoint(useLeft, useTop);
#endif

    switch (referencePoint) {
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPLEFT:
        return QPoint(useLeft, useTop);
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPRIGHT:
        return QPoint(useLeft + useWidth, useTop);
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPCENTER:
        return QPoint(useLeft + useWidth / 2, useTop);
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMLEFT:
        return QPoint(useLeft, useTop + useHeight);
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMRIGHT:
        return QPoint(useLeft + useWidth, useTop + useHeight);
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMCENTER:
        return QPoint(useLeft + useWidth / 2, useTop + useHeight);
    default:
        return QPoint(useLeft, useTop); // Default to top-left
    }
}

QPoint QFloatingIconWindow::getWindowReferencePoint(int referencePoint, const QRect &clientRect) const
{
    int clientWidth = clientRect.width();
    int clientHeight = clientRect.height();
    int clientLeft = clientRect.left();
    int clientTop = clientRect.top();

    switch (referencePoint) {
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPLEFT:
        return QPoint(clientLeft, clientTop);
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPRIGHT:
        return QPoint(clientLeft + clientWidth, clientTop);
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPCENTER:
        return QPoint(clientLeft + clientWidth / 2, clientTop);
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMLEFT:
        return QPoint(clientLeft, clientTop + clientHeight);
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMRIGHT:
        return QPoint(clientLeft + clientWidth, clientTop + clientHeight);
    case QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMCENTER:
        return QPoint(clientLeft + clientWidth / 2, clientTop + clientHeight);
    default:
        return QPoint(clientLeft, clientTop); // Default to window top-left
    }
}

QPoint QFloatingIconWindow::calculateAbsolutePosition(const QPoint &relativePosition, int referencePoint) const
{
    QPoint referenceOrigin;

    if (isWindowReferencePoint(referencePoint)) {
        // Window-based reference point
        QRect clientRect;
        if (!getWindowClientRect(QKeyMapper::s_CurrentMappingHWND, clientRect)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QFloatingIconWindow::calculateAbsolutePosition] Warning: Cannot get window client rect for reference point" << referencePoint;
#endif
            return QPoint(-1, -1); // Invalid position to indicate error
        }
        referenceOrigin = getWindowReferencePoint(referencePoint, clientRect);
    } else {
        // Screen-based reference point
        referenceOrigin = getScreenReferencePoint(referencePoint);
    }

    return referenceOrigin + relativePosition;
}

QPoint QFloatingIconWindow::calculateRelativePosition(const QPoint &physicalPosition, int referencePoint) const
{
    QPoint referenceOrigin;

    if (isWindowReferencePoint(referencePoint)) {
        // Window-based reference point
        QRect clientRect;
        if (!getWindowClientRect(QKeyMapper::s_CurrentMappingHWND, clientRect)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QFloatingIconWindow::calculateRelativePosition] Warning: Cannot get window client rect for reference point" << referencePoint;
#endif
            return physicalPosition; // Return original position if can't calculate relative
        }
        referenceOrigin = getWindowReferencePoint(referencePoint, clientRect);
    } else {
        // Screen-based reference point (already using physical coordinates)
        referenceOrigin = getScreenReferencePoint(referencePoint);
    }

    QPoint relativePos = physicalPosition - referenceOrigin;

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingIconWindow::calculateRelativePosition] Physical pos:" << physicalPosition
             << "reference origin:" << referenceOrigin << "relative pos:" << relativePos;
#endif

    return relativePos;
}

bool QFloatingIconWindow::isValidReferencePoint(int referencePoint)
{
    return (referencePoint >= QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_MIN &&
            referencePoint <= QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_MAX);
}

void QFloatingIconWindow::updatePositionForCurrentWindow()
{
    if (!isVisible()) {
        return;
    }

    // Only update position for window-based reference points
    if (!isWindowReferencePoint(m_CurrentOptions.referencePoint)) {
        return;
    }

    // Check if we have a valid mapping window
    if (QKeyMapper::s_CurrentMappingHWND == nullptr) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QFloatingIconWindow::updatePositionForCurrentWindow] No current mapping window available";
#endif
        return;
    }

    // Get current window client rect
    QRect currentClientRect;
    if (!getWindowClientRect(QKeyMapper::s_CurrentMappingHWND, currentClientRect)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QFloatingIconWindow::updatePositionForCurrentWindow] Cannot get current window client rect";
#endif
        return;
    }

    // Check if the window has moved or resized since last update
    if (m_LastMappingHWND == QKeyMapper::s_CurrentMappingHWND &&
        m_LastWindowClientRect == currentClientRect) {
        // No change, no need to update
        return;
    }

    // Calculate new absolute position based on current window (in physical coordinates)
    QPoint newPhysicalPosition = calculateAbsolutePosition(m_CurrentOptions.position, m_CurrentOptions.referencePoint);
    if (newPhysicalPosition == QPoint(-1, -1)) {
        // Error calculating position
        return;
    }

    // Convert to Qt coordinates for window positioning
    QPoint newQtPosition = physicalToQtCoordinates(newPhysicalPosition);

    // Update position if it has changed
    if (newQtPosition != pos()) {
        move(newQtPosition);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QFloatingIconWindow::updatePositionForCurrentWindow] Updated Qt position from"
                 << pos() << "to" << newQtPosition << "physical position:" << newPhysicalPosition
                 << "based on reference point" << m_CurrentOptions.referencePoint;
#endif
    }

    // Update tracking variables
    m_LastMappingHWND = QKeyMapper::s_CurrentMappingHWND;
    m_LastWindowClientRect = currentClientRect;
}

QPoint QFloatingIconWindow::qtToPhysicalCoordinates(const QPoint &qtPoint) const
{
    // Get DPI scaling factor
    qreal dpiScale = QApplication::primaryScreen()->devicePixelRatio();

    // Convert Qt logical coordinates to physical coordinates
    return QPoint(qRound(qtPoint.x() * dpiScale), qRound(qtPoint.y() * dpiScale));
}

QPoint QFloatingIconWindow::physicalToQtCoordinates(const QPoint &physicalPoint) const
{
    // Get DPI scaling factor
    qreal dpiScale = QApplication::primaryScreen()->devicePixelRatio();

    // Convert physical coordinates to Qt logical coordinates
    return QPoint(qRound(physicalPoint.x() / dpiScale), qRound(physicalPoint.y() / dpiScale));
}

QPoint QFloatingIconWindow::getPhysicalCursorPosition() const
{
    // Use Windows API to get true physical cursor position
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    return QPoint(cursorPos.x, cursorPos.y);
}

#if 0
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
#endif

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
        QString curSettingSelectStr;
        int curSettingSelectIndex = ui->settingselectComboBox->currentIndex();
        if (0 < curSettingSelectIndex && curSettingSelectIndex < m_SettingSelectListWithoutDescription.size()) {
            curSettingSelectStr = m_SettingSelectListWithoutDescription.at(curSettingSelectIndex);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug().noquote().nospace() << "[on_settingselectComboBox_textActivated/textChanged]" << "Change to setting select index is invalid("<< curSettingSelectIndex << "), m_SettingSelectListWithoutDescription ->" << m_SettingSelectListWithoutDescription;
#endif
        }
        QString loadresult = loadKeyMapSetting(curSettingSelectStr);
        ui->settingNameLineEdit->setText(loadresult);
        Q_UNUSED(loadresult);
        loadSetting_flag = false;
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_settingselectComboBox_textActivated/textChanged] Select setting text Empty!";
#endif
        ui->settingNameLineEdit->setEnabled(true);
        ui->settingNameLineEdit->setReadOnly(false);
        ui->processLineEdit->setEnabled(true);
        ui->windowTitleLineEdit->setEnabled(true);
        ui->restoreProcessPathButton->setEnabled(true);
        // ui->processCheckBox->setEnabled(true);
        // ui->titleCheckBox->setEnabled(true);
        ui->processLabel->setEnabled(true);
        ui->windowTitleLabel->setEnabled(true);
        ui->checkProcessComboBox->setEnabled(true);
        ui->checkWindowTitleComboBox->setEnabled(true);
        ui->removeSettingButton->setEnabled(true);
        // ui->disableWinKeyCheckBox->setEnabled(true);
        ui->descriptionLineEdit->clear();
        ui->descriptionLineEdit->setReadOnly(false);
    }
}

void QKeyMapper::on_removeSettingButton_clicked()
{
    int currentSettingIndex = ui->settingselectComboBox->currentIndex();
    if (currentSettingIndex == 0 || currentSettingIndex == GLOBALSETTING_INDEX) {
        return;
    }

    QString settingSelectStr;
    if (0 < currentSettingIndex && currentSettingIndex < m_SettingSelectListWithoutDescription.size()) {
        settingSelectStr = m_SettingSelectListWithoutDescription.at(currentSettingIndex);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().noquote().nospace() << "[on_removeSettingButton_clicked]" << "Current setting select index is invalid("<< currentSettingIndex << "), m_SettingSelectListWithoutDescription ->" << m_SettingSelectListWithoutDescription;
#endif
    }

    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    QStringList groups = settingFile.childGroups();
    if (groups.contains(settingSelectStr)) {

        QString message = tr("Are you sure you want to remove the setting \"%1\"?").arg(settingSelectStr);
        QMessageBox::StandardButton reply = QMessageBox::question(this, PROGRAM_NAME, message,
                                                                   QMessageBox::Yes | QMessageBox::No,
                                                                   QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            // User cancelled, don't remove
            return;
        }

        settingFile.remove(settingSelectStr);
        ui->settingselectComboBox->removeItem(currentSettingIndex);
        m_SettingSelectListWithoutDescription.removeAt(currentSettingIndex);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[removeSetting] Remove setting select ->" << settingSelectStr;
#endif
        // currentSettingText = ui->settingselectComboBox->currentText();
        QString curSettingSelectStr;
        int curSettingSelectIndex = ui->settingselectComboBox->currentIndex();
        if (0 < curSettingSelectIndex && curSettingSelectIndex < m_SettingSelectListWithoutDescription.size()) {
            curSettingSelectStr = m_SettingSelectListWithoutDescription.at(curSettingSelectIndex);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug().noquote().nospace() << "[on_removeSettingButton_clicked]" << "Next setting select index is invalid("<< curSettingSelectIndex << "), m_SettingSelectListWithoutDescription ->" << m_SettingSelectListWithoutDescription;
#endif
        }
        if (false == curSettingSelectStr.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[removeSetting] Change to Setting ->" << curSettingSelectStr;
#endif
            loadSetting_flag = true;
            QString loadresult = loadKeyMapSetting(curSettingSelectStr);
            ui->settingNameLineEdit->setText(loadresult);
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
    resetFontSize();
    refreshProcessInfoTable();
    resizeKeyMappingDataTableColumnWidth(m_KeyMappingDataTable);

#ifdef DEBUG_LOGOUT_ON
    QString languageStr;
    if (LANGUAGE_ENGLISH == index) {
        languageStr = "English";
    }
    else if (LANGUAGE_JAPANESE == index) {
        languageStr = "Japanese";
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
    qDebug() << "[EnableVirtualGamepad] Enable Virtual Gamepad state changed ->" << (Qt::CheckState)state;
#endif

#ifdef VIGEM_CLIENT_SUPPORT
    bool checked = false;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);

    if (Qt::Checked == state) {
        bool enable_result = false;
        if (QKeyMapper_Worker::s_VirtualGamepadList.size() <= VIRTUAL_GAMEPAD_NUMBER_MAX
            && QKeyMapper_Worker::s_ViGEmTargetList.isEmpty()
            && QKeyMapper_Worker::s_ViGEmTarget_ReportList.isEmpty()) {
            if (QKeyMapper_Worker::s_VirtualGamepadList.size() == 1
                && QKeyMapper_Worker::s_VirtualGamepadList.constFirst() != getVirtualGamepadType()) {
                QKeyMapper_Worker::s_VirtualGamepadList[0] = getVirtualGamepadType();
            }

            int gamepad_index = 0;
            for (const QString &gamepad_type : std::as_const(QKeyMapper_Worker::s_VirtualGamepadList)){
                PVIGEM_TARGET added_target = QKeyMapper_Worker::ViGEmClient_AddTarget_byType(gamepad_type);
                if (added_target != Q_NULLPTR) {
                    QKeyMapper_Worker::s_ViGEmTarget_ReportList.append(ViGEm_ReportData());
                    QKeyMapper_Worker::s_ViGEmTargetList.append(added_target);
                    QKeyMapper_Worker::ViGEmClient_GamepadReset_byIndex(gamepad_index);
                    gamepad_index++;
                }
            }

            if (QKeyMapper_Worker::s_ViGEmTargetList.size() == QKeyMapper_Worker::s_VirtualGamepadList.size()) {
                enable_result = true;
            }
        }

        if (enable_result) {
            checked = true;
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[EnableVirtualGamepad]" << " Enable Virtual Gamepad success(" << QKeyMapper_Worker::s_VirtualGamepadList.size() << ") -> " << QKeyMapper_Worker::s_VirtualGamepadList;
#endif
        }
        else {
            ui->enableVirtualJoystickCheckBox->setCheckState(Qt::Unchecked);
#ifdef DEBUG_LOGOUT_ON
            qWarning() << "[EnableVirtualJoystick] Enable All Virtual Gamepad failed!!!";
#endif
        }
    }
    else {
        QKeyMapper_Worker::ViGEmClient_RemoveAllTargets();
        checked = false;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[EnableVirtualGamepad]" << " Disable Virtual Gamepad success(" << QKeyMapper_Worker::s_VirtualGamepadList.size() << ") -> " << QKeyMapper_Worker::s_VirtualGamepadList;
#endif
    }

    if (true == checked) {
        ui->vJoyXSensSpinBox->setEnabled(true);
        ui->vJoyYSensSpinBox->setEnabled(true);
        ui->vJoyXSensLabel->setEnabled(true);
        ui->vJoyYSensLabel->setEnabled(true);
        ui->vJoyRecenterLabel->setEnabled(true);
        ui->vJoyRecenterSpinBox->setEnabled(true);
        ui->lockCursorCheckBox->setEnabled(true);
        ui->directModeCheckBox->setEnabled(true);
        ui->virtualGamepadNumberSpinBox->setEnabled(true);
        ui->virtualGamepadListComboBox->setEnabled(true);

        settingFile.setValue(VIRTUALGAMEPAD_ENABLE , true);
    }
    else {
        ui->vJoyXSensSpinBox->setEnabled(false);
        ui->vJoyYSensSpinBox->setEnabled(false);
        ui->vJoyXSensLabel->setEnabled(false);
        ui->vJoyYSensLabel->setEnabled(false);
        ui->vJoyRecenterLabel->setEnabled(false);
        ui->vJoyRecenterSpinBox->setEnabled(false);
        ui->lockCursorCheckBox->setEnabled(false);
        ui->directModeCheckBox->setEnabled(false);
        ui->virtualGamepadNumberSpinBox->setEnabled(false);
        ui->virtualGamepadListComboBox->setEnabled(false);

        settingFile.setValue(VIRTUALGAMEPAD_ENABLE , false);
    }
#endif

    emit updateVirtualGamepadListDisplay_Signal();
}


void QKeyMapper::on_installViGEmBusButton_clicked()
{
#ifdef VIGEM_CLIENT_SUPPORT
    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == QKeyMapper_Worker::ViGEmClient_getConnectState()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "Uninstall ViGEm Bus.";
#endif

        // QKeyMapper_Worker::ViGEmClient_Remove();
        QKeyMapper_Worker::ViGEmClient_RemoveAllTargets();
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

#if 0
void QKeyMapper::on_soundEffectCheckBox_stateChanged(int state)
{
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
#endif

void QKeyMapper::on_installInterceptionButton_clicked()
{
    Interception_Worker::Interception_State currentInterceptionState = Interception_Worker::getInterceptionState();

    if (Interception_Worker::INTERCEPTION_AVAILABLE == currentInterceptionState) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_installInterceptionButton_clicked]" << "Uninstall Interception Driver, InterceptionState ->" << currentInterceptionState;
#endif

        Interception_Worker::getInstance()->doUnloadInterception();

        (void)uninstallInterceptionDriver();
        Interception_Worker::setRebootRequiredFlag();

        emit updateMultiInputStatus_Signal();

        Interception_Worker::Interception_State newInterceptionState = Interception_Worker::getInterceptionState();

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_installInterceptionButton_clicked]" << "Uninstall Interception Driver, New InterceptionState ->" << newInterceptionState;
#endif

        if (Interception_Worker::INTERCEPTION_REBOOTREQUIRED == newInterceptionState) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[on_installInterceptionButton_clicked]" << "Reboot required after uninstall Interception Driver, InterceptionState ->" << newInterceptionState;
#endif
            /* Show Reboot Required MessageBox after Uninstall Interception Driver */
            QString message = tr("System reboot is required for the changes to take effect after uninstalling Interception driver.");
            QMessageBox::warning(this, PROGRAM_NAME, message);
        }
    }
    else {
        /* Show a confirmation dialog before installing the driver */
        QString message = tr("Under special scenarios such as repeatedly plugging and unplugging input devices or repeatedly putting the system into sleep and waking it up while using Interception driver, issues like mouse or keyboard input device failure may occur. Please carefully read the software instructions related to multi-input devices before proceeding.\n\nDo you confirm to continue installing Interception driver?");
        QMessageBox::StandardButton reply = QMessageBox::warning(this, PROGRAM_NAME, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[on_installInterceptionButton_clicked]" << "User chose not to install the Interception Driver.";
#endif
            return;
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_installInterceptionButton_clicked]" << "Install Interception Driver, Current InterceptionState ->" << currentInterceptionState;
#endif
        (void)installInterceptionDriver();

        int loop = 0;
        for (loop = 0; loop < INSTALL_INTERCEPTION_LOOP_WAIT_TIME_MAX; loop++) {
            if (Interception_Worker::isInterceptionDriverFileExist()) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[on_installInterceptionButton_clicked]" << "Install Interception Driver, InterceptionDriverFileExist() wait time ->" << loop * INSTALL_INTERCEPTION_LOOP_WAIT_TIME;
#endif
                break;
            }
            else {
                QThread::msleep(INSTALL_INTERCEPTION_LOOP_WAIT_TIME);
            }
        }

        emit updateMultiInputStatus_Signal();

        Interception_Worker::Interception_State newInterceptionState = Interception_Worker::getInterceptionState();

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_installInterceptionButton_clicked]" << "Install Interception Driver, New InterceptionState ->" << newInterceptionState;
#endif

        if (Interception_Worker::INTERCEPTION_REBOOTREQUIRED == newInterceptionState) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[on_installInterceptionButton_clicked]" << "Reboot required after install Interception Driver, InterceptionState ->" << newInterceptionState;
#endif
            /* Show Reboot Required MessageBox after Install Interception Driver */
            QString message = tr("System reboot is required for the changes to take effect after installing Interception driver.");
            QMessageBox::warning(this, PROGRAM_NAME, message);
        }
    }
}

void QKeyMapper::on_multiInputDeviceListButton_clicked()
{
    showInputDeviceListWindow();
}

void QKeyMapper::on_multiInputEnableCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[MultiInput] MultiInput Enable state changed ->" << (Qt::CheckState)state;
#endif

    if (Qt::Checked == state) {
        Interception_Worker::startInterception();
    }
    else {
        Interception_Worker::stopInterception();
    }

    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);

    if (Qt::Checked == state) {
        settingFile.setValue(MULTI_INPUT_ENABLE , true);
    }
    else {
        settingFile.setValue(MULTI_INPUT_ENABLE , false);
    }

    updateMultiInputStatus();
}

void QKeyMapper::on_virtualGamepadNumberSpinBox_valueChanged(int number)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[on_virtualGamepadNumberSpinBox_valueChanged] number changed ->" << number;
#endif
    int gamepad_number = QKeyMapper_Worker::s_VirtualGamepadList.size();

    if (number < VIRTUAL_GAMEPAD_NUMBER_MIN || number > VIRTUAL_GAMEPAD_NUMBER_MAX) {
        ui->virtualGamepadNumberSpinBox->setValue(gamepad_number);
        return;
    }

    if ((number == gamepad_number + 1)
        && (number == QKeyMapper_Worker::s_ViGEmTargetList.size() + 1)
        && (number == QKeyMapper_Worker::s_ViGEmTarget_ReportList.size() + 1)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_virtualGamepadNumberSpinBox_valueChanged] Virtual Gamepad number increased," << gamepad_number << "->" << number;
#endif
        QString gamepad_type = getVirtualGamepadType();
        PVIGEM_TARGET added_target = QKeyMapper_Worker::ViGEmClient_AddTarget_byType(gamepad_type);
        if (added_target != Q_NULLPTR) {
            QKeyMapper_Worker::s_ViGEmTarget_ReportList.append(ViGEm_ReportData());
            QKeyMapper_Worker::s_VirtualGamepadList.append(gamepad_type);
            QKeyMapper_Worker::s_ViGEmTargetList.append(added_target);
            QKeyMapper_Worker::ViGEmClient_GamepadReset_byIndex(gamepad_number);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[on_virtualGamepadNumberSpinBox_valueChanged]" << " Add Virtual Gamepad(" << gamepad_type << ") success -> " << QKeyMapper_Worker::s_VirtualGamepadList;
#endif
        }

        emit updateVirtualGamepadListDisplay_Signal();
    }
    else if ((number == gamepad_number - 1)
         && (number == QKeyMapper_Worker::s_ViGEmTargetList.size() - 1)
         && (number == QKeyMapper_Worker::s_ViGEmTarget_ReportList.size() - 1)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_virtualGamepadNumberSpinBox_valueChanged] Virtual Gamepad number decreased," << gamepad_number << "->" << number;
#endif
        PVIGEM_TARGET target_toremove = QKeyMapper_Worker::s_ViGEmTargetList.at(number);
        if (target_toremove != Q_NULLPTR) {
#ifdef DEBUG_LOGOUT_ON
            QString removed_gamepadtype = QKeyMapper_Worker::s_VirtualGamepadList.last();
#endif
            QKeyMapper_Worker::ViGEmClient_GamepadReset_byIndex(number);
            QKeyMapper_Worker::ViGEmClient_RemoveTarget(target_toremove);
            QKeyMapper_Worker::s_ViGEmTargetList.removeLast();
            QKeyMapper_Worker::s_ViGEmTarget_ReportList.removeLast();
            QKeyMapper_Worker::s_VirtualGamepadList.removeLast();
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[on_virtualGamepadNumberSpinBox_valueChanged]" << " Remove Virtual Gamepad(" << removed_gamepadtype << ") success -> " << QKeyMapper_Worker::s_VirtualGamepadList;
#endif
        }

        emit updateVirtualGamepadListDisplay_Signal();
    }
    else {
        ui->virtualGamepadNumberSpinBox->setValue(gamepad_number);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_virtualGamepadNumberSpinBox_valueChanged]" << "Size error! s_ViGEmTargetList.size =" << QKeyMapper_Worker::s_ViGEmTargetList.size() << ", s_ViGEmTarget_ReportList.size =" << QKeyMapper_Worker::s_ViGEmTarget_ReportList.size() << ", s_VirtualGamepadList ->" << QKeyMapper_Worker::s_VirtualGamepadList;
#endif
    }
}

void QKeyMapper::on_filterKeysCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[MultiInput] Filter Keys state changed ->" << (Qt::CheckState)state;
#endif

    if (Qt::Checked == state) {
        Interception_Worker::s_FilterKeys = true;
    }
    else {
        Interception_Worker::s_FilterKeys = false;
    }
}

void QKeyMapper::keyMappingTabl_ItemSelectionChanged()
{
    if (m_KeyMapStatus != KEYMAP_IDLE){
        return;
    }

    QList<QTableWidgetItem*> selectedItems = m_KeyMappingDataTable->selectedItems();
    if (selectedItems.isEmpty() == false) {
        QTableWidgetItem* selectedItem = selectedItems.at(0);
        int currentrowindex = m_KeyMappingDataTable->row(selectedItem);
        Q_UNUSED(currentrowindex);
        // m_KeyMappingDataTable->setFocus();
        // m_KeyMappingDataTable->clearFocus();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyMappingTabl_ItemSelectionChanged] Selected Item Index =" << currentrowindex;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyMappingTabl_ItemSelectionChanged] No Item Selected.";
#endif
    }
}

void KeyMappingTabWidget::keyPressEvent(QKeyEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyMappingTabWidget::keyPressEvent]" << "Key:" << (Qt::Key)event->key() << "Modifiers:" << event->modifiers();
#endif

    if (QKeyMapper::KEYMAP_IDLE == QKeyMapper::getInstance()->m_KeyMapStatus) {
        if (event->key() == Qt::Key_Up) {
            QKeyMapper::getInstance()->selectedItemsMoveUp();
            return;
        }
        else if (event->key() == Qt::Key_Down) {
            QKeyMapper::getInstance()->selectedItemsMoveDown();
            return;
        }
        else if (event->key() == Qt::Key_Delete) {
            QKeyMapper::getInstance()->on_deleteSelectedButton_clicked();
            return;
        }
        else if (event->key() == Qt::Key_C && (event->modifiers() & Qt::ControlModifier)) {
            int copied_count = QKeyMapper::getInstance()->copySelectedKeyMappingDataToCopiedList();
            if (copied_count > 0) {
                QString message = tr("%1 selected mapping data copied.").arg(copied_count);
                QKeyMapper::getInstance()->showInformationPopup(message);
                return;
            }
        }
        else if (event->key() == Qt::Key_V && (event->modifiers() & Qt::ControlModifier)) {
            int inserted_count = QKeyMapper::getInstance()->insertKeyMappingDataFromCopiedList();
            int copied_count = QKeyMapper::s_CopiedMappingData.size();
            if (inserted_count == 0) {
                QString message = tr("%1 copied mapping data are completely duplicated and could not be inserted!").arg(copied_count);
                QKeyMapper::getInstance()->showFailurePopup(message);
            }
            else if (inserted_count > 0) {
                QString message;
                if (inserted_count != copied_count) {
                    message = tr("Inserted %1 copied mapping data into current mapping table. %2 duplicated ones were not inserted.")
                                  .arg(inserted_count)
                                  .arg(copied_count - inserted_count);
                    QKeyMapper::getInstance()->showWarningPopup(message);
                } else {
                    message = tr("Inserted %1 copied mapping data into current mapping table.")
                                  .arg(inserted_count);
                    QKeyMapper::getInstance()->showInformationPopup(message);
                }
            }
            return;
        }
    }

    QTabWidget::keyPressEvent(event);
}

void KeyMappingTabWidget::onTabMoved(int from, int to)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyMappingTabWidget::onTabMoved] from:" << from << "to:" << to;
#endif

    // Emit signal to notify parent class about tab reorder
    emit tabOrderChanged(from, to);
}

void KeyMappingDataTableWidget::startDrag(Qt::DropActions supportedActions)
{
    QList<QTableWidgetSelectionRange> selectedRanges = this->selectedRanges();
    if (!selectedRanges.isEmpty()) {
        QTableWidgetSelectionRange range = selectedRanges.first();
        m_DraggedTopRow = range.topRow();
        m_DraggedBottomRow = range.bottomRow();
    }
    QTableWidget::startDrag(supportedActions);
}

void KeyMappingDataTableWidget::dropEvent(QDropEvent *event)
{
    if (event->dropAction() == Qt::MoveAction) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        int droppedRow = rowAt(event->position().toPoint().y());
#else
        int droppedRow = rowAt(event->pos().y());
#endif

        if (droppedRow < 0) {
            droppedRow = rowCount() - 1;
        }

        emit QKeyMapper::getInstance()->keyMappingTableDragDropMove_Signal(m_DraggedTopRow, m_DraggedBottomRow, droppedRow);
    }
}

void KeyMappingDataTableWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (QKeyMapper::getInstance()->isMappingDataTableFiltered()) {
        QString message;
        message = tr("Cannot move items while the mapping table is filtered!");
        QKeyMapper::getInstance()->showWarningPopup(message);
        event->ignore();
    } else {
        QTableWidget::dragEnterEvent(event);
    }
}

void KeyMappingDataTableWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (QKeyMapper::getInstance()->isMappingDataTableFiltered()) {
        event->ignore();
    } else {
        QTableWidget::dragMoveEvent(event);
    }
}

// Category filtering methods implementation
void KeyMappingDataTableWidget::setCategoryFilter(const QString &category)
{
    m_CategoryFilter = category;
    updateRowVisibility();
}

void KeyMappingDataTableWidget::clearCategoryFilter()
{
    m_CategoryFilter.clear();
    updateRowVisibility();
}

QStringList KeyMappingDataTableWidget::getAvailableCategories() const
{
    QStringList categories;

    if (!isCategoryColumnVisible()) {
        return categories;
    }

    bool hasNonEmptyCategories = false;
    bool hasEmptyCategories = false;

    for (int row = 0; row < rowCount(); ++row) {
        QTableWidgetItem *categoryItem = item(row, CATEGORY_COLUMN);
        QString category;

        if (categoryItem) {
            category = categoryItem->text().trimmed();
        }

        if (category.isEmpty()) {
            hasEmptyCategories = true;
        } else {
            hasNonEmptyCategories = true;
            if (!categories.contains(category)) {
#ifdef DEBUG_LOGOUT_ON
                // Avoid confusion with the built-in "All" option
                // Users can still create an "All" category, but we'll warn about it
                if (category == tr("All")) {
                    qDebug() << "[getAvailableCategories]" << "Warning: Found user-created category named 'All', which may cause confusion with the built-in 'All' option";
                }
#endif
                categories.append(category);
            }
        }
    }

    // Add "(Blanks)" option only if there are both non-empty and empty categories
    // If all categories are empty, showing "(Blanks)" would be the same as "All"
    if (hasEmptyCategories && hasNonEmptyCategories) {
        categories.append(tr("Blank"));
    }

    return categories;
}

void KeyMappingDataTableWidget::setCategoryColumnVisible(bool visible)
{
    m_CategoryColumnVisible = visible;

    if (visible) {
        showColumn(CATEGORY_COLUMN);
        updateRowVisibility();
        // Enable editing for category column items
        for (int row = 0; row < rowCount(); ++row) {
            QTableWidgetItem *categoryItem = item(row, CATEGORY_COLUMN);
            if (categoryItem) {
                categoryItem->setFlags(categoryItem->flags() | Qt::ItemIsEditable);
            }
        }
    }
    else {
        hideColumn(CATEGORY_COLUMN);
        updateRowVisibility(); // Only show all the rows when hiding column
        // Disable editing for category column items
        for (int row = 0; row < rowCount(); ++row) {
            QTableWidgetItem *categoryItem = item(row, CATEGORY_COLUMN);
            if (categoryItem) {
                categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsEditable);
            }
        }
    }
}

bool KeyMappingDataTableWidget::isCategoryColumnVisible() const
{
    return m_CategoryColumnVisible;
}

void KeyMappingDataTableWidget::updateRowVisibility()
{
    if (!isCategoryColumnVisible() || m_CategoryFilter.isEmpty()) {
        // Show all rows when no filter is active
        for (int row = 0; row < rowCount(); ++row) {
            setRowHidden(row, false);
        }
        return;
    }

    // Filter rows based on category
    for (int row = 0; row < rowCount(); ++row) {
        QTableWidgetItem *categoryItem = item(row, CATEGORY_COLUMN);
        bool shouldShow = false;

        if (m_CategoryFilter == tr("Blank")) {
            // Show rows with empty/blank categories
            if (categoryItem) {
                QString itemCategory = categoryItem->text().trimmed();
                shouldShow = itemCategory.isEmpty();
            } else {
                // No item means it's also considered blank
                shouldShow = true;
            }
        } else {
            // Show rows matching the specific category
            if (categoryItem) {
                QString itemCategory = categoryItem->text().trimmed();
                shouldShow = (itemCategory == m_CategoryFilter);
            }
        }

        setRowHidden(row, !shouldShow);
    }
}

#if 0
void QKeyMapper::on_startupMinimizedCheckBox_stateChanged(int state)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[StartupMinimized] Startup Minimized state changed ->" << (Qt::CheckState)state;
#endif

    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);

    if (Qt::Checked == state) {
        settingFile.setValue(STARTUP_MINIMIZED , true);
    }
    else {
        settingFile.setValue(STARTUP_MINIMIZED , false);
    }
}
#endif

void QKeyMapper::on_autoStartMappingCheckBox_stateChanged(int state)
{
    if (Qt::Checked == state) {
        ui->sendToSameTitleWindowsCheckBox->setEnabled(false);
    }
    else {
        if (m_KeyMapStatus == KEYMAP_IDLE){
            if (GLOBALSETTING_INDEX == ui->settingselectComboBox->currentIndex()) {
                ui->sendToSameTitleWindowsCheckBox->setEnabled(false);
            }
            else {
                ui->sendToSameTitleWindowsCheckBox->setEnabled(true);
            }
        }
    }
}

void QKeyMapper::on_processListButton_toggled(bool checked)
{
    if (!checked) {
        hideProcessList();
        setKeyMappingTabWidgetWideMode();
        refreshAllKeyMappingTagWidget();
    }
    else {
        setKeyMappingTabWidgetNarrowMode();
        refreshAllKeyMappingTagWidget();
        showProcessList();
    }
}

void QKeyMapper::on_showNotesButton_toggled(bool checked)
{
    Q_UNUSED(checked);
    refreshAllKeyMappingTagWidget();
}

void QKeyMapper::on_showCategoryButton_toggled(bool checked)
{
    if (m_KeyMappingDataTable) {
        m_KeyMappingDataTable->setCategoryColumnVisible(checked);
        resizeKeyMappingDataTableColumnWidth(m_KeyMappingDataTable);
    }

    ui->CategoryFilterComboBox->setVisible(checked);

    if (checked) {
        updateCategoryFilterComboBox();
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[on_showCategoryButton_toggled]" << "Category controls visibility set to:" << checked;
#endif
}

void QKeyMapper::on_checkUpdateButton_clicked()
{
    QString qkeymapper_updates_url;

    if (UPDATE_SITE_GITEE == ui->updateSiteComboBox->currentIndex()) {
        qkeymapper_updates_url = CHECK_UPDATES_URL_GITEE;
    }
    else {
        qkeymapper_updates_url = CHECK_UPDATES_URL_GITHUB;
    }

    QSimpleUpdater::getInstance()->checkForUpdates(qkeymapper_updates_url);
}

void SystrayMenu::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        bool menu_visible = isVisible();
        if (menu_visible) {
            QAction *action = this->actionAt(event->pos());
            if (action == QKeyMapper::getInstance()->m_TrayIconMenu_ShowHideAction) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[SystrayMenu::mousePressEvent] Mouse Left-Button Pressed on action ShowHide";
#endif
                m_MenuItem_Pressed = SYSTRAY_MENU_ITEM_PRESSED_SHOWHIDE;
            }
            else if (action == QKeyMapper::getInstance()->m_TrayIconMenu_QuitAction) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[SystrayMenu::mousePressEvent] Mouse Left-Button Pressed on action Quit";
#endif
                m_MenuItem_Pressed = SYSTRAY_MENU_ITEM_PRESSED_QUIT;
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[SystrayMenu::mousePressEvent] Mouse Left-Button Pressed outside the menu";
#endif
                m_MenuItem_Pressed = SYSTRAY_MENU_ITEM_PRESSED_NONE;
            }
        }

        QMenu::mousePressEvent(event);
    }

    // QMenu::mousePressEvent(event);
}

void SystrayMenu::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        bool click_matched = false;
        bool menu_visible = isVisible();
        if (menu_visible) {
            QAction *action = this->actionAt(event->pos());
            if (action == QKeyMapper::getInstance()->m_TrayIconMenu_ShowHideAction) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[SystrayMenu::mouseReleaseEvent] Mouse Left-Button Released on action ShowHide.";
#endif
                if (m_MenuItem_Pressed == SYSTRAY_MENU_ITEM_PRESSED_SHOWHIDE) {
                    click_matched = true;
                }
            }
            else if (action == QKeyMapper::getInstance()->m_TrayIconMenu_QuitAction) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[SystrayMenu::mouseReleaseEvent] Mouse Left-Button Released on action Quit.";
#endif
                if (m_MenuItem_Pressed == SYSTRAY_MENU_ITEM_PRESSED_QUIT) {
                    click_matched = true;
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[SystrayMenu::mouseReleaseEvent] Mouse Left-Button Released outside the menu.";
#endif
            }
        }
        m_MenuItem_Pressed = SYSTRAY_MENU_ITEM_PRESSED_NONE;

        if (click_matched) {
            QMenu::mouseReleaseEvent(event);
        }
    }

    // QMenu::mouseReleaseEvent(event);
}

void QKeyMapper::on_Gyro2MouseAdvancedSettingButton_clicked()
{
    showGyro2MouseAdvancedSettingWindow();
}

void QKeyMapper::on_selectTrayIconButton_clicked()
{
    showTrayIconSelectDialog();
}

void QKeyMapper::on_notificationAdvancedSettingButton_clicked()
{
    showNotificationSetupDialog();
}

void QKeyMapper::on_oriList_SelectKeyboardButton_toggled(bool checked)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyListButtonToggled] Original KeyList Select Keyboard Button toggled ->" << checked;
#endif
    Q_UNUSED(checked);
    updateOriginalKeyListComboBox();
}


void QKeyMapper::on_oriList_SelectMouseButton_toggled(bool checked)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyListButtonToggled] Original KeyList Select Mouse Button toggled ->" << checked;
#endif
    Q_UNUSED(checked);
    updateOriginalKeyListComboBox();
}


void QKeyMapper::on_oriList_SelectGamepadButton_toggled(bool checked)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyListButtonToggled] Original KeyList Select Gamepad Button toggled ->" << checked;
#endif
    Q_UNUSED(checked);
    updateOriginalKeyListComboBox();
}


void QKeyMapper::on_oriList_SelectFunctionButton_toggled(bool checked)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyListButtonToggled] Original KeyList Select Function Button toggled ->" << checked;
#endif
    Q_UNUSED(checked);
    updateOriginalKeyListComboBox();
}


void QKeyMapper::on_mapList_SelectKeyboardButton_toggled(bool checked)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyListButtonToggled] Mapping KeyList Select Keyboard Button toggled ->" << checked;
#endif
    Q_UNUSED(checked);
    updateMappingKeyListComboBox();
}


void QKeyMapper::on_mapList_SelectMouseButton_toggled(bool checked)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyListButtonToggled] Mapping KeyList Select Mouse Button toggled ->" << checked;
#endif
    Q_UNUSED(checked);
    updateMappingKeyListComboBox();
}


void QKeyMapper::on_mapList_SelectGamepadButton_toggled(bool checked)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyListButtonToggled] Mapping KeyList Select Gamepad Button toggled ->" << checked;
#endif
    Q_UNUSED(checked);
    updateMappingKeyListComboBox();
}


void QKeyMapper::on_mapList_SelectFunctionButton_toggled(bool checked)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[KeyListButtonToggled] Mapping KeyList Select Function Button toggled ->" << checked;
#endif
    Q_UNUSED(checked);
    updateMappingKeyListComboBox();
}

void QKeyMapper::on_CategoryFilterComboBox_currentIndexChanged(int index)
{
    onCategoryFilterChanged(index);
}

void QKeyMapper::on_CategoryFilterComboBox_currentTextChanged(const QString &text)
{
    ui->CategoryFilterComboBox->setToolTip(text);
}

void QKeyMapper::on_restoreProcessPathButton_clicked()
{
    if (!m_MapProcessInfo.FilePath.isEmpty()
        && ui->processLineEdit->text() != m_MapProcessInfo.FilePath) {
        QString message = tr("Restore to absolute process path \"%1\"?").arg(m_MapProcessInfo.FilePath);
        QMessageBox::StandardButton reply = QMessageBox::question(this, PROGRAM_NAME, message,
                                                                   QMessageBox::Yes | QMessageBox::No,
                                                                   QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            // User cancelled, don't restore
            return;
        }

        ui->processLineEdit->setText(m_MapProcessInfo.FilePath);
    }
}

void QKeyMapper::on_processinfoTable_clicked(const QModelIndex &index)
{
    Q_UNUSED(index);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QKeyMapper::on_processinfoTable_clicked] clicked.";
#endif

    QWidget *focused = QApplication::focusWidget();
    if (focused && focused != this) {
        focused->clearFocus();
    }
}

QString QKeyMapper::getCurrentCategoryFilter() const
{
    if (ui->CategoryFilterComboBox && ui->CategoryFilterComboBox->currentIndex() >= 0) {
        return ui->CategoryFilterComboBox->currentText();
    }
    return QString();
}

bool QKeyMapper::isCategoryFilterVisible() const
{
    return ui->showCategoryButton && ui->showCategoryButton->isChecked();
}

void QKeyMapper::restoreCategoryFilterState(const QString& filter, bool showState)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[restoreCategoryFilterState] Attempting to restore - Filter:" << filter << ", ShowState:" << showState;
#endif

    // Restore category filter visibility state first
    if (ui->showCategoryButton && ui->showCategoryButton->isChecked() != showState) {
        ui->showCategoryButton->setChecked(showState);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[restoreCategoryFilterState] Set showCategoryButton to:" << showState;
#endif
    }

    // Restore category filter selection if filter was visible
    if (showState && !filter.isEmpty() && ui->CategoryFilterComboBox) {
        int index = ui->CategoryFilterComboBox->findText(filter);
        if (index >= 0 && index != ui->CategoryFilterComboBox->currentIndex()) {
            ui->CategoryFilterComboBox->setCurrentIndex(index);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[restoreCategoryFilterState] Set CategoryFilterComboBox index to:" << index << "(" << filter << ")";
#endif
        }
        else if (index < 0) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[restoreCategoryFilterState] Warning: Filter text '" << filter << "' not found in ComboBox";
#endif
        }
    }
}

// ================================
// CategoryFilterStateGuard Implementation
// ================================

QKeyMapper::CategoryFilterStateGuard::CategoryFilterStateGuard(QKeyMapper* parent)
    : m_parent(parent)
    , m_savedShowState(false)
{
    if (m_parent) {
        // Save current filter state
        m_savedFilter = m_parent->getCurrentCategoryFilter();
        m_savedShowState = m_parent->isCategoryFilterVisible();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[CategoryFilterStateGuard] Saved filter state - Filter:" << m_savedFilter << ", ShowState:" << m_savedShowState;
#endif
    }
}

QKeyMapper::CategoryFilterStateGuard::~CategoryFilterStateGuard()
{
    if (m_parent) {
        // Restore saved filter state
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[CategoryFilterStateGuard] Restoring filter state - Filter:" << m_savedFilter << ", ShowState:" << m_savedShowState;
#endif
        m_parent->restoreCategoryFilterState(m_savedFilter, m_savedShowState);
    }
}
