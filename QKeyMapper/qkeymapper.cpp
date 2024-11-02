#include "qkeymapper.h"
#include "ui_qkeymapper.h"
#include "qkeymapper_constants.h"

QKeyMapper *QKeyMapper::m_instance = Q_NULLPTR;
QString QKeyMapper::DEFAULT_TITLE = QString("Forza: Horizon 4");

bool QKeyMapper::s_isDestructing = false;
int QKeyMapper::s_GlobalSettingAutoStart = 0;
uint QKeyMapper::s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
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

QKeyMapper::QKeyMapper(QWidget *parent) :
    QDialog(parent),
    m_UI_Scale(UI_SCALE_NORMAL),
    m_KeyMapStatus(KEYMAP_IDLE),
    ui(new Ui::QKeyMapper),
    m_LastWindowPosition(INITIAL_WINDOW_POSITION, INITIAL_WINDOW_POSITION),
    m_CycleCheckTimer(this),
    m_ProcessInfoTableRefreshTimer(this),
    m_MapProcessInfo(),
    m_SysTrayIcon(Q_NULLPTR),
    m_SysTrayIconMenu(Q_NULLPTR),
    m_TrayIconMenu_ShowHideAction(Q_NULLPTR),
    m_TrayIconMenu_QuitAction(Q_NULLPTR),
    m_PopupMessageLabel(Q_NULLPTR),
    m_PopupMessageAnimation(Q_NULLPTR),
    m_PopupMessageTimer(this),
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
    m_GamepadInfoMap(),
    m_SettingSelectListWithoutDescription(),
    // m_windowswitchKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    // m_mappingswitchKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    // m_originalKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    // m_HotKey_ShowHide(new QHotkey(this)),
    // m_HotKey_StartStop(new QHotkey(this)),
    loadSetting_flag(false),
    m_MainWindowHandle(NULL),
    m_TransParentHandle(NULL),
    m_TransParentWindowInitialX(0),
    m_TransParentWindowInitialY(0),
    m_TransParentWindowInitialWidth(0),
    m_TransParentWindowInitialHeight(0),
    m_deviceListWindow(Q_NULLPTR),
    m_ItemSetupDialog(Q_NULLPTR),
    m_TableSetupDialog(Q_NULLPTR),
    m_PopupNotification(Q_NULLPTR)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("QKeyMapper() -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

    m_TransParentHandle = createTransparentWindow();

    m_instance = this;
    ui->setupUi(this);

    QStyle* windowsStyle = QStyleFactory::create("windows");
    QStyle* fusionStyle = QStyleFactory::create("Fusion");

    ui->settingTabWidget->setStyle(windowsStyle);

    // Iterate through all child widgets of settingTabWidget and set their style to Fusion.
    for (int tabindex = 0; tabindex < ui->settingTabWidget->count(); ++tabindex) {
        QWidget *tabPage = ui->settingTabWidget->widget(tabindex);
        tabPage->setStyle(fusionStyle);
    }
    ui->settingTabWidget->setCurrentIndex(ui->settingTabWidget->indexOf(ui->mapping));
    // ui->virtualgamepadGroupBox->setStyle(defaultStyle);
    // ui->multiInputGroupBox->setStyle(defaultStyle);

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

    extractSoundFiles();
    initAddKeyComboBoxes();
    initWindowSwitchKeyLineEdit();
    initMappingSwitchKeyLineEdit();
    // initOriginalKeySeqEdit();
    initCombinationKeyLineEdit();
    initInputDeviceSelectComboBoxes();
    initPopupMessage();

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

    // set QTableWidget selected background-color
    setStyleSheet("QTableWidget::item:selected { background-color: rgb(190, 220, 255) }");

    ui->iconLabel->setStyle(QStyleFactory::create("windows"));
    ui->pointDisplayLabel->setStyle(QStyleFactory::create("windows"));
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
    ui->descriptionLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->sendTextLineEdit->setFocusPolicy(Qt::ClickFocus);

    ui->dataPortSpinBox->setRange(DATA_PORT_MIN, DATA_PORT_MAX);
    ui->brakeThresholdDoubleSpinBox->setDecimals(GRIP_THRESHOLD_DECIMALS);
    ui->brakeThresholdDoubleSpinBox->setRange(GRIP_THRESHOLD_BRAKE_MIN, GRIP_THRESHOLD_BRAKE_MAX);
    ui->accelThresholdDoubleSpinBox->setDecimals(GRIP_THRESHOLD_DECIMALS);
    ui->accelThresholdDoubleSpinBox->setRange(GRIP_THRESHOLD_ACCEL_MIN, GRIP_THRESHOLD_ACCEL_MAX);
    ui->waitTimeSpinBox->setRange(MAPPING_WAITTIME_MIN, MAPPING_WAITTIME_MAX);
    ui->pressTimeSpinBox->setRange(PRESSTIME_MIN, PRESSTIME_MAX);
    // ui->burstpressSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    // ui->burstreleaseSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    ui->mouseXSpeedSpinBox->setRange(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX);
    ui->mouseYSpeedSpinBox->setRange(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX);

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
    ui->lockCursorCheckBox->setEnabled(false);
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
            << POSITION_NONE_STR_CHINESE
            << POSITION_TOP_LEFT_STR_CHINESE
            << POSITION_TOP_CENTER_STR_CHINESE
            << POSITION_TOP_RIGHT_STR_CHINESE
            << POSITION_BOTTOM_LEFT_STR_CHINESE
            << POSITION_BOTTOM_CENTER_STR_CHINESE
            << POSITION_BOTTOM_RIGHT_STR_CHINESE
            ;
    ui->notificationComboBox->addItems(positoin_list);
    ui->notificationComboBox->setCurrentIndex(NOTIFICATION_POSITION_TOP_RIGHT);

    // m_windowswitchKeySeqEdit->setDefaultKeySequence(DISPLAYSWITCH_KEY_DEFAULT);
    // m_mappingswitchKeySeqEdit->setDefaultKeySequence(MAPPINGSWITCH_KEY_DEFAULT);
    // m_originalKeySeqEdit->setDefaultKeySequence(ORIGINAL_KEYSEQ_DEFAULT);
    ui->windowswitchkeyLineEdit->setText(DISPLAYSWITCH_KEY_DEFAULT);
    ui->mappingStartKeyLineEdit->setText(MAPPINGSWITCH_KEY_DEFAULT);
    ui->mappingStopKeyLineEdit->setText(MAPPINGSWITCH_KEY_DEFAULT);
    initKeyMappingTabWidget();
    m_ItemSetupDialog = new QItemSetupDialog(this);
    m_TableSetupDialog = new QTableSetupDialog(this);
    loadSetting_flag = true;
    bool loadresult = loadKeyMapSetting(QString());
    Q_UNUSED(loadresult);
    loadSetting_flag = false;

    m_PopupNotification = new QPopupNotification(Q_NULLPTR);
    m_deviceListWindow = new QInputDeviceListWindow(this);
    // m_ItemSetupDialog->setWindowFlags(Qt::Popup);

    Interception_Worker::syncDisabledKeyboardList();
    Interception_Worker::syncDisabledMouseList();

    updateSysTrayIconMenuText();
    reloadUILanguage();
    resetFontSize();

    updateMultiInputStatus();
#ifdef VIGEM_CLIENT_SUPPORT
    updateViGEmBusStatus();
    updateVirtualGamepadListDisplay();
#endif

    QObject::connect(m_SysTrayIcon, &QSystemTrayIcon::activated, this, &QKeyMapper::SystrayIconActivated);
    QObject::connect(&m_CycleCheckTimer, &QTimer::timeout, this, &QKeyMapper::cycleCheckProcessProc);
    QObject::connect(&m_ProcessInfoTableRefreshTimer, &QTimer::timeout, this, &QKeyMapper::cycleRefreshProcessInfoTableProc);

    QObject::connect(m_KeyMappingTabWidget, &QTabWidget::currentChanged, this, &QKeyMapper::keyMappingTabWidgetCurrentChanged);
    QObject::connect(m_KeyMappingTabWidget, &QTabWidget::tabBarDoubleClicked, this, &QKeyMapper::onKeyMappingTabWidgetTabBarDoubleClicked);
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

    QObject::connect(this, &QKeyMapper::updateLockStatus_Signal, this, &QKeyMapper::updateLockStatusDisplay, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::updateMousePointLabelDisplay_Signal, this, &QKeyMapper::updateMousePointLabelDisplay, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::showMousePoints_Signal, this, &QKeyMapper::showMousePoints, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::showCarOrdinal_Signal, this, &QKeyMapper::showCarOrdinal, Qt::QueuedConnection);
#ifdef VIGEM_CLIENT_SUPPORT
    QObject::connect(this, &QKeyMapper::updateViGEmBusStatus_Signal, this, &QKeyMapper::updateViGEmBusStatus);
    QObject::connect(this, &QKeyMapper::updateVirtualGamepadListDisplay_Signal, this, &QKeyMapper::updateVirtualGamepadListDisplay);
    QObject::connect(m_orikeyComboBox, &KeyListComboBox::currentTextChanged, this, &QKeyMapper::OrikeyComboBox_currentTextChangedSlot);
#endif
    QObject::connect(this, &QKeyMapper::updateMultiInputStatus_Signal, this, &QKeyMapper::updateMultiInputStatus);
    QObject::connect(this, &QKeyMapper::updateInputDeviceSelectComboBoxes_Signal, this, &QKeyMapper::updateInputDeviceSelectComboBoxes);
    QObject::connect(this, &QKeyMapper::updateGamepadSelectComboBox_Signal, this, &QKeyMapper::updateGamepadSelectComboBox);

    //m_CycleCheckTimer.start(CYCLE_CHECK_TIMEOUT);
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

    // Unregister WTS session notifications
    WTSUnRegisterSessionNotification(reinterpret_cast<HWND>(winId()));

    m_MainWindowHandle = NULL;
    destoryTransparentWindow(m_TransParentHandle);
    m_TransParentHandle = NULL;
    // freeShortcuts();

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

    exitDeleteKeyMappingTabWidget();
}

void QKeyMapper::WindowStateChangedProc(void)
{
    if (true == isMinimized()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[WindowStateChangedProc]" << "QKeyMapper::WindowStateChangedProc() -> Window Minimized: setHidden!";
#endif
        closeTableSetupDialog();
        closeItemSetupDialog();
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
        else if (true == getSendToSameTitleWindowsStatus()
            && true == ui->nameCheckBox->isChecked()
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
                    if (checkresult > 1) {
                        setKeyHook(NULL);
                    }
                    else {
                        setKeyHook(hwnd);
                    }
                    m_KeyMapStatus = KEYMAP_MAPPING_MATCHED;
                    mappingStartNotification();
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
                mappingStopNotification();
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
    updateHWNDListProc();

    if (false == isHidden()){
        refreshProcessInfoTable();
        initInputDeviceSelectComboBoxes();
    }
}

void QKeyMapper::updateHWNDListProc()
{
    s_hWndList.clear();
    if (false == m_MapProcessInfo.WindowTitle.isEmpty()
        && false == m_MapProcessInfo.FileName.isEmpty()) {
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

QString QKeyMapper::getProcessNameFromPID(DWORD dwProcessId)
{
    QString ProcessPath;
    QString processName;

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

    if (false == ProcessPath.isEmpty()){
        QFileInfo fileinfo(ProcessPath);
        processName = fileinfo.fileName();
    }

    return processName;
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
    bool fileNameCheckOK = true;
    bool windowTitleCheckOK = true;
    bool fileNameExist = !QKeyMapper::getInstance()->m_MapProcessInfo.FileName.isEmpty();
    bool windowTitleExist = !QKeyMapper::getInstance()->m_MapProcessInfo.WindowTitle.isEmpty();

    if (QKeyMapper::getInstance()->ui->nameCheckBox->checkState() == Qt::Checked && false == fileNameExist) {
        fileNameCheckOK = false;
    }

    if (QKeyMapper::getInstance()->ui->titleCheckBox->checkState() == Qt::Checked && false == windowTitleExist) {
        windowTitleCheckOK = false;
    }

    if (true == fileNameCheckOK
        && true == windowTitleCheckOK
        && false == WindowText.isEmpty()){
        DWORD dwProcessId = 0;
        GetWindowThreadProcessId(hWnd, &dwProcessId);
        QString processName = getProcessNameFromPID(dwProcessId);

        if (processName == QKeyMapper::getInstance()->m_MapProcessInfo.FileName
            && WindowText.contains(QKeyMapper::getInstance()->m_MapProcessInfo.WindowTitle)) {
            if (!s_hWndList.contains(hWnd)) {
                s_hWndList.append(hWnd);
            }
        }
    }
}

int QKeyMapper::findOriKeyInKeyMappingDataList(const QString &keyname)
{
    int returnindex = -1;
    int keymapdataindex = 0;
    QString keyname_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keyname);

    for (const MAP_KEYDATA &keymapdata : qAsConst(*KeyMappingDataList))
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

int QKeyMapper::findOriKeyInKeyMappingDataList(const QString &keyname, bool &removemultiinput)
{
    int returnindex = -1;
    int keymapdataindex = 0;
    QString keyname_RemoveMultiInput = QKeyMapper_Worker::getKeycodeStringRemoveMultiInput(keyname);
    removemultiinput = false;

    for (const MAP_KEYDATA &keymapdata : qAsConst(*KeyMappingDataList))
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

    for (const MAP_KEYDATA &keymapdata : qAsConst(*KeyMappingDataList))
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

    for (const MAP_KEYDATA &keymapdata : qAsConst(*KeyMappingDataList))
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

    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataListGlobal))
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

    for (const MAP_KEYDATA &keymapdata : qAsConst(*KeyMappingDataList))
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
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = "Invalid original key format.";
        } else {
            result.errorMessage = "无效的原始按键格式。";
        }
        return result;
    }

    QString key_without_suffix = full_key_match.captured(1);
    QString longPressTimeString = full_key_match.captured(2);
    QString doublePressTimeString = full_key_match.captured(3);

    QStringList orikeylist = key_without_suffix.split(SEPARATOR_PLUS);
    if (orikeylist.isEmpty()) {
        result.isValid = false;
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = "OriginalKey is empty.";
        } else {
            result.errorMessage = "原始按键为空";
        }
        return result;
    }

    // Check for duplicate keys
    int numRemoved = orikeylist.removeDuplicates();
    if (numRemoved > 0) {
        result.isValid = false;
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = "OriginalKey contains duplicate keys.";
        } else {
            result.errorMessage = "原始按键中存在重复按键";
        }
        return result;
    }

    if (orikeylist.size() > 1) {
        // Check if any key is a special key
        for (const QString &orikey : orikeylist) {
            if (QKeyMapper_Worker::SpecialOriginalKeysList.contains(orikey)) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Oricombinationkey contains specialkey \"%1\"").arg(orikey);
                } else {
                    result.errorMessage = QString("原始组合键包含特殊按键 \"%1\"").arg(orikey);
                }
                return result;
            }
        }

        // Validate each individual key in the combination
        for (const QString &orikey : orikeylist) {
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
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Duplicate original key \"%1\"").arg(originalkeystr);
                } else {
                    result.errorMessage = QString("已存在相同的原始按键 \"%1\"").arg(originalkeystr);
                }
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
                    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                        result.errorMessage = QString("Originalkey \"%1\" does not match special mappingkey \"%2\"").arg(orikey_noindex, mapkey_noindex);
                    } else {
                        result.errorMessage = QString("原始按键 \"%1\" 与映射特殊按键 \"%2\" 不匹配").arg(orikey_noindex, mapkey_noindex);
                    }
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
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Invalid press time \"%1\"").arg(isLongPress ? longPressTimeString : doublePressTimeString);
                } else {
                    result.errorMessage = QString("无效的按压时间 \"%1\"").arg(isLongPress ? longPressTimeString : doublePressTimeString);
                }
                return result;
            }
        } else {
            result.isValid = false;
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("Invalid time suffix \"%1\"").arg(isLongPress ? longPressTimeString : doublePressTimeString);
            } else {
                result.errorMessage = QString("无效的时间后缀 \"%1\"").arg(isLongPress ? longPressTimeString : doublePressTimeString);
            }
            return result;
        }
    }

    if (0 <= update_rowindex && update_rowindex < QKeyMapper::KeyMappingDataList->size()) {
        QStringList mappingkeys = QKeyMapper::KeyMappingDataList->at(update_rowindex).Mapping_Keys;

        if (mappingkeys.size() == 1 && mappingkeys.constFirst() == KEY_BLOCKED_STR) {
            if (originalkeystr.contains(JOY_KEY_PREFIX)) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Game controller keys could not be blocked!");
                } else {
                    result.errorMessage = QString("游戏手柄按键无法被屏蔽!");
                }
                return result;
            }
            else if (isLongPress || isDoublePress) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Could not block original key with time suffix!");
                } else {
                    result.errorMessage = QString("不能屏蔽带有时间后缀的原始按键!");
                }
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
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = QString("Invalid key format \"%1\"").arg(orikey);
        } else {
            result.errorMessage = QString("无效按键格式 \"%1\"").arg(orikey);
        }
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
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("Invalid key \"%1\"").arg(original_key);
            } else {
                result.errorMessage = QString("无效按键 \"%1\"").arg(original_key);
            }
        }
    }

    if (validKey) {
        if (isLongPress || isDoublePress) {
            bool ok;
            int pressTime = isLongPress ? longPressTimeString.toInt(&ok) : doublePressTimeString.toInt(&ok);

            if (!ok || pressTime <= PRESSTIME_MIN || pressTime > PRESSTIME_MAX || (isLongPress && longPressTimeString.startsWith('0')) || (isDoublePress && doublePressTimeString.startsWith('0'))) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Invalid press time \"%1\"").arg(isLongPress ? longPressTimeString : doublePressTimeString);
                } else {
                    result.errorMessage = QString("无效的按压时间 \"%1\"").arg(isLongPress ? longPressTimeString : doublePressTimeString);
                }
            }
        }

        if (result.isValid && update_rowindex >= 0) {
            int findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(orikey);
            if (findindex != -1 && findindex != update_rowindex) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Duplicate key \"%1\"").arg(orikey);
                } else {
                    result.errorMessage = QString("已存在相同的按键 \"%1\"").arg(orikey);
                }
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
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = QString("Invalid key format \"%1\"").arg(orikey);
        } else {
            result.errorMessage = QString("无效按键格式 \"%1\"").arg(orikey);
        }
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
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("Invalid key \"%1\"").arg(original_key);
            } else {
                result.errorMessage = QString("无效按键 \"%1\"").arg(original_key);
            }
        }
    }

    if (validKey) {
        if (result.isValid && update_rowindex >= 0) {
            int findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(orikey);
            if (findindex != -1 && findindex != update_rowindex) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Duplicate key \"%1\"").arg(orikey);
                } else {
                    result.errorMessage = QString("已存在相同的按键 \"%1\"").arg(orikey);
                }
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
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = "MappingKeys is empty.";
        }
        else {
            result.errorMessage = "映射按键为空";
        }
        return result;
    }
    else if (mappingkeyseqlist.size() >= KEY_SEQUENCE_MAX) {
        result.isValid = false;
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = "Mapping key sequence is too long!";
        }
        else {
            result.errorMessage = "映射按键序列过长!";
        }
        return result;
    }

    static QRegularExpression removeindex_regex("@\\d$");

    for (const QString& mappingkeys : mappingkeyseqlist) {
        result.isValid = true;
        QStringList Mapping_Keys = splitMappingKeyString(mappingkeys, SPLIT_WITH_PLUS);

        int duplicatesRemoved = Mapping_Keys.removeDuplicates();
        if (duplicatesRemoved > 0) {
            result.isValid = false;
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("MappingKeys contains duplicate key \"%1\"").arg(mappingkeys);
            } else {
                result.errorMessage = QString("映射按键中包含重复按键 \"%1\"").arg(mappingkeys);
            }
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
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("MappingKeys contains specialkey \"%1\"").arg(foundSpecialMappingKey);
                } else {
                    result.errorMessage = QString("映射按键中包含特殊按键 \"%1\"").arg(foundSpecialMappingKey);
                }
                return result;
            }
            if (!foundSpecialOriginalKey.isEmpty()) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("MappingCombinationKeys contains specialkey \"%1\"").arg(foundSpecialOriginalKey);
                } else {
                    result.errorMessage = QString("映射组合按键中包含特殊按键 \"%1\"").arg(foundSpecialOriginalKey);
                }
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
                        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                            result.errorMessage = QString("Mappingkey \"%1\" does not match special originalkey \"%2\"").arg(mapkey_noindex, orikey_noindex);
                        } else {
                            result.errorMessage = QString("映射按键 \"%1\" 与原始特殊按键 \"%2\" 不匹配").arg(mapkey_noindex, orikey_noindex);
                        }
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
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Game controller keys could not be blocked!");
                } else {
                    result.errorMessage = QString("游戏手柄按键无法被屏蔽!");
                }
                return result;
            }
            else if (originalkeystr.contains(SEPARATOR_LONGPRESS) || originalkeystr.contains(SEPARATOR_DOUBLEPRESS)) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Could not block original key with time suffix!");
                } else {
                    result.errorMessage = QString("不能屏蔽带有时间后缀的原始按键!");
                }
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

        if (!QItemSetupDialog::s_valiedMappingKeyList.contains(mapping_key)) {
            static QRegularExpression vjoy_regex("^(vJoy-[^@]+)(?:@([0-3]))?$");
            static QRegularExpression joy2vjoy_mapkey_regex(R"(^(Joy-(LS|RS|Key11\(LT\)|Key12\(RT\))_2vJoy(LS|RS|LT|RT))(?:@([0-3]))?$)");
            static QRegularExpression mousepoint_regex(R"(^Mouse-(L|R|M|X1|X2)(:W)?\((\d+),(\d+)\)$)");
            static QRegularExpression sendtext_regex(R"(^SendText\((.+)\)$)"); // RegularExpression to match "SendText(string)"
            QRegularExpressionMatch vjoy_match = vjoy_regex.match(mapping_key);
            QRegularExpressionMatch joy2vjoy_mapkey_match = joy2vjoy_mapkey_regex.match(mapping_key);
            QRegularExpressionMatch mousepoint_match = mousepoint_regex.match(mapping_key);
            QRegularExpressionMatch sendtext_match = sendtext_regex.match(mapping_key);

            if (vjoy_match.hasMatch()) {
                static QRegularExpression vjoy_keys_regex("^vJoy-.+$");
                QStringList vJoyKeyList = QItemSetupDialog::s_valiedMappingKeyList.filter(vjoy_keys_regex);
                QString vjoy_key = vjoy_match.captured(1);

                if (!vJoyKeyList.contains(vjoy_key)) {
                    result.isValid = false;
                    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                        result.errorMessage = QString("Invalid vJoy-Key \"%1\"").arg(mapping_key);
                    } else {
                        result.errorMessage = QString("无效虚拟游戏手柄按键 \"%1\"").arg(mapping_key);
                    }
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
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Invalid key \"%1\"").arg(mapping_key);
                } else {
                    result.errorMessage = QString("无效按键 \"%1\"").arg(mapping_key);
                }
            }
        }

        if (result.isValid && !waitTimeString.isEmpty()) {
            if (!ok || waitTimeString == "0" || waitTimeString.startsWith('0') || waittime <= MAPPING_WAITTIME_MIN || waittime > MAPPING_WAITTIME_MAX) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Invalid waittime \"%1\"").arg(waitTimeString);
                } else {
                    result.errorMessage = QString("无效延时时间 \"%1\"").arg(waitTimeString);
                }
            }
        }
    }
    else {
        result.isValid = false;
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = QString("Invalid format \"%1\"").arg(mapkey);
        } else {
            result.errorMessage = QString("无效格式 \"%1\"").arg(mapkey);
        }
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
    for (const QString &mousekey : qAsConst(mouseNameConvertList)){
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

#if 1
void QKeyMapper::DrawMousePoints(HWND hwnd, HDC hdc, int showMode)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[DrawMousePoints] Show Mode = " << (showMode == SHOW_MODE_WINDOW ? "SHOW_MODE_WINDOW" : "SHOW_MODE_SCREEN");
#endif

    Q_UNUSED(hwnd);
    const QList<MousePoint_Info>& MousePointsList = (showMode == SHOW_MODE_WINDOW) ? WindowMousePointsList : ScreenMousePointsList;

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
        if (pointInfo.map_key.startsWith(MOUSE_R_STR)) {
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

LRESULT QKeyMapper::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
    case WM_ERASEBKGND:
    {
        HDC hdc = GetDC(hwnd);
        clearTransparentWindow(hwnd, hdc);
        ReleaseDC(hwnd, hdc);
        return 1;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

HWND QKeyMapper::createTransparentWindow()
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = QKeyMapper::WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = L"QKeyMapper_TransparentWindow";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"QKeyMapper_TransparentWindow",
        NULL, WS_POPUP, 0, 0, screenWidth, screenHeight, NULL, NULL, hInstance, NULL);

    // Set the opacity of the window (0 = fully transparent, 255 = fully opaque)
    BYTE opacity = 120; // 50% opacity
    SetLayeredWindowAttributes(hwnd, 0, opacity, LWA_ALPHA);

    ShowWindow(hwnd, SW_HIDE);

    // Save the initial width & height of TransparentWindow
    m_TransParentWindowInitialWidth = screenWidth;
    m_TransParentWindowInitialHeight = screenHeight;

    // Initialize the show mode to SHOW_MODE_SCREEN
    SetWindowLongPtr(hwnd, GWLP_USERDATA, SHOW_MODE_SCREEN);

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

void QKeyMapper::destoryTransparentWindow(HWND hwnd)
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

bool QKeyMapper::getKeySequenceSerialProcessStatus()
{
    return false;
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
    }

    return isvalid;
}

int QKeyMapper::tabIndexToSwitchByTabHotkey(const QString &hotkey_string)
{
    int tabindex_toswitch = -1;
    QList<int> tabindex_list = s_MappingTableTabHotkeyMap.value(hotkey_string);
    if (!tabindex_list.isEmpty()) {
        if (tabindex_list.size() == 1){
            if (tabindex_list.at(0) != s_KeyMappingTabWidgetCurrentIndex) {
                tabindex_toswitch = tabindex_list.at(0);
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
    QStringList notesList;
    QStringList burstList;
    QStringList burstpresstimeList;
    QStringList burstreleasetimeList;
    QStringList lockList;
    QStringList passthroughList;
    QStringList keyup_actionList;
    QStringList keyseqholddownList;
    QStringList repeatmodeList;
    QStringList repeattimesList;

    for (const MAP_KEYDATA &keymapdata : qAsConst(*mappingDataList))
    {
        original_keys << keymapdata.Original_Key;
        QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
        mapping_keysList  << mappingkeys_str;
        notesList << keymapdata.Note;
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
        if (true == keymapdata.PassThrough) {
            passthroughList.append("ON");
        }
        else {
            passthroughList.append("OFF");
        }
        if (true == keymapdata.KeyUp_Action) {
            keyup_actionList.append("ON");
        }
        else {
            keyup_actionList.append("OFF");
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
    }

    keyMappingDataFile.setValue(KEYMAPDATA_ORIGINALKEYS, original_keys );
    keyMappingDataFile.setValue(KEYMAPDATA_MAPPINGKEYS, mapping_keysList);
    keyMappingDataFile.setValue(KEYMAPDATA_NOTE, notesList);
    keyMappingDataFile.setValue(KEYMAPDATA_BURST, burstList);
    keyMappingDataFile.setValue(KEYMAPDATA_BURSTPRESS_TIME, burstpresstimeList);
    keyMappingDataFile.setValue(KEYMAPDATA_BURSTRELEASE_TIME , burstreleasetimeList);
    keyMappingDataFile.setValue(KEYMAPDATA_LOCK, lockList);
    keyMappingDataFile.setValue(KEYMAPDATA_PASSTHROUGH, passthroughList);
    keyMappingDataFile.setValue(KEYMAPDATA_KEYUP_ACTION, keyup_actionList);
    keyMappingDataFile.setValue(KEYMAPDATA_KEYSEQHOLDDOWN, keyseqholddownList);
    keyMappingDataFile.setValue(KEYMAPDATA_REPEATMODE, repeatmodeList);
    keyMappingDataFile.setValue(KEYMAPDATA_REPEATIMES, repeattimesList);

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
    QStringList burstStringList;
    QStringList burstpressStringList;
    QStringList burstreleaseStringList;
    QStringList lockStringList;
    QStringList passthroughStringList;
    QStringList keyup_actionStringList;
    QStringList keyseqholddownStringList;
    QStringList repeatmodeStringList;
    QStringList repeattimesStringList;
    QStringList notesList;
    QList<bool> burstList;
    QList<int> burstpresstimeList;
    QList<int> burstreleasetimeList;
    QList<bool> lockList;
    QList<bool> passthroughList;
    QList<bool> keyup_actionList;
    QList<bool> keyseqholddownList;
    QList<int> repeatmodeList;
    QList<int> repeattimesList;
    QList<MAP_KEYDATA> loadkeymapdata;

    if ((true == keyMappingDataFile.contains(KEYMAPDATA_ORIGINALKEYS))
        && (true == keyMappingDataFile.contains(KEYMAPDATA_MAPPINGKEYS))){
        original_keys   = keyMappingDataFile.value(KEYMAPDATA_ORIGINALKEYS).toStringList();
        mapping_keys    = keyMappingDataFile.value(KEYMAPDATA_MAPPINGKEYS).toStringList();

        int mappingdata_size = original_keys.size();
        QStringList stringListAllOFF;
        QStringList stringListAllZERO;
        QStringList burstpressStringListDefault;
        QStringList burstreleaseStringListDefault;
        QStringList repeattimesStringListDefault;
        for (int i = 0; i < mappingdata_size; ++i) {
            stringListAllOFF << "OFF";
            stringListAllZERO << "0";
            burstpressStringListDefault.append(QString::number(BURST_PRESS_TIME_DEFAULT));
            burstreleaseStringListDefault.append(QString::number(BURST_RELEASE_TIME_DEFAULT));
            repeattimesStringListDefault.append(QString::number(REPEAT_TIMES_DEFAULT));
        }
        burstStringList         = stringListAllOFF;
        burstpressStringList    = burstpressStringListDefault;
        burstreleaseStringList  = burstreleaseStringListDefault;
        lockStringList          = stringListAllOFF;
        passthroughStringList   = stringListAllOFF;
        keyup_actionStringList   = stringListAllOFF;
        keyseqholddownStringList = stringListAllOFF;
        repeatmodeStringList = stringListAllZERO;
        repeattimesStringList = repeattimesStringListDefault;

        if (true == keyMappingDataFile.contains(KEYMAPDATA_NOTE)) {
            notesList = keyMappingDataFile.value(KEYMAPDATA_NOTE).toStringList();
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
        if (true == keyMappingDataFile.contains(KEYMAPDATA_PASSTHROUGH)) {
            passthroughStringList = keyMappingDataFile.value(KEYMAPDATA_PASSTHROUGH).toStringList();
        }
        if (true == keyMappingDataFile.contains(KEYMAPDATA_KEYUP_ACTION)) {
            keyup_actionStringList = keyMappingDataFile.value(KEYMAPDATA_KEYUP_ACTION).toStringList();
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

        if (original_keys.size() == mapping_keys.size() && original_keys.size() > 0) {

            if (notesList.size() < original_keys.size()) {
                int diff = original_keys.size() - notesList.size();
                for (int i = 0; i < diff; ++i) {
                    notesList.append(QString());
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
                const QString &passthrough = (i < passthroughStringList.size()) ? passthroughStringList.at(i) : "OFF";
                if (passthrough == "ON") {
                    passthroughList.append(true);
                } else {
                    passthroughList.append(false);
                }
            }

            for (int i = 0; i < original_keys.size(); i++) {
                const QString &keyup_action = (i < keyup_actionStringList.size()) ? keyup_actionStringList.at(i) : "OFF";
                if (keyup_action == "ON") {
                    keyup_actionList.append(true);
                } else {
                    keyup_actionList.append(false);
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

            int loadindex = 0;
            for (const QString &ori_key_nochange : qAsConst(original_keys)){
                QString ori_key = ori_key_nochange;
                if (ori_key.startsWith(OLD_PREFIX_SHORTCUT)) {
                    ori_key.remove(0, 1);
                }

                bool checkoriginalstr = checkOriginalkeyStr(ori_key);
                bool checkmappingstr = checkMappingkeyStr(mapping_keys[loadindex]);

                if (true == checkoriginalstr && true == checkmappingstr) {
                    loadkeymapdata.append(MAP_KEYDATA(ori_key,
                                                      mapping_keys.at(loadindex),
                                                      notesList.at(loadindex),
                                                      burstList.at(loadindex),
                                                      burstpresstimeList.at(loadindex),
                                                      burstreleasetimeList.at(loadindex),
                                                      lockList.at(loadindex),
                                                      passthroughList.at(loadindex),
                                                      keyup_actionList.at(loadindex),
                                                      keyseqholddownList.at(loadindex),
                                                      repeatmodeList.at(loadindex),
                                                      repeattimesList.at(loadindex)
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
    for (const MAP_KEYDATA &keymapdata : qAsConst(loadkeymapdata)) {
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
        if (m_MainWindowHandle == NULL) {
            m_MainWindowHandle = reinterpret_cast<HWND>(winId());
            // Enable WTS session notifications
            WTSRegisterSessionNotification(m_MainWindowHandle, NOTIFY_FOR_THIS_SESSION);

#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::showEvent] m_MainWindowHandle ->" << m_MainWindowHandle;
#endif
        }


        QTimer::singleShot(100, this, [=]() {
            if (m_KeyMapStatus == KEYMAP_IDLE){
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[QKeyMapper::showEvent]" << "Set and Clear Focus.";
#endif
                m_KeyMappingDataTable->setFocus();
                m_KeyMappingDataTable->clearFocus();
            }
        });
    }

    QDialog::showEvent(event);
}

void QKeyMapper::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        QTimer::singleShot(0, this, SLOT(WindowStateChangedProc()));
    }
    else if (event->type() == QEvent::WinIdChange)
    {
        m_MainWindowHandle = reinterpret_cast<HWND>(winId());
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QKeyMapper::changeEvent]" << "m_MainWindowHandle ->" << m_MainWindowHandle;
#endif
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
#ifdef DEBUG_LOGOUT_ON
    else if (event->key() == KEY_SHOW_DEBUGINFO) {
        qDebug().nospace() << "\033[1;34m[QKeyMapper::keyPressEvent]" << "Show debug info pressedMappingKeysMap -> " << QKeyMapper_Worker::pressedMappingKeysMap << "\033[0m";
        qDebug().nospace() << "\033[1;34m[QKeyMapper::keyPressEvent]" << "Show debug info pressedVirtualKeysList -> " << QKeyMapper_Worker::pressedVirtualKeysList << "\033[0m";
        qDebug().nospace() << "\033[1;34m[QKeyMapper::keyPressEvent]" << "Show debug info pressedRealKeysList -> " << QKeyMapper_Worker::pressedRealKeysList << "\033[0m";
        qDebug().nospace() << "\033[1;34m[QKeyMapper::keyPressEvent]" << "Show debug info pressedRealKeysListRemoveMultiInput -> " << QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput << "\033[0m";
        return;
    }
#endif
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

void QKeyMapper::on_keymapButton_clicked()
{
    MappingSwitch(MAPPINGSTART_BUTTONCLICK);
}

void QKeyMapper::MappingSwitch(MappingStartMode startmode)
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
                showWarningPopup("Please doubleclick process info table to select valid processinfo for key mapping.");
            }
            else {
                showWarningPopup("请双击进程列表为按键映射选择有效的进程信息");
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
        mappingStopNotification();
        s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
        emit updateLockStatus_Signal();

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[MappingStart]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") -> " << mappingstartmodeEnum.valueToKey(startmode);
#endif
    }

    if (m_KeyMapStatus != KEYMAP_IDLE){
        closeTableSetupDialog();
        closeItemSetupDialog();
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

    int tabindex_toswitch = tabIndexToSwitchByTabHotkey(hotkey_string);

    if (tabindex_toswitch >= 0) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[HotKeyMappingTableSwitchTab] Switch tab index(" << s_KeyMappingTabWidgetCurrentIndex << "->" << tabindex_toswitch << ")";
#endif
        clearLockStatusDisplay();
        forceSwitchKeyMappingTabWidgetIndex(tabindex_toswitch);

        if (m_KeyMapStatus == KEYMAP_MAPPING_MATCHED
            || m_KeyMapStatus == KEYMAP_MAPPING_GLOBAL) {
            /* Key Mapping Restart */
            setKeyMappingRestart();
            mappingTabSwitchNotification();
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
    int insert_index = tab_count - 1;

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

    KeyMappingTableWidget->horizontalHeader()->setStretchLastSection(true);
    KeyMappingTableWidget->horizontalHeader()->setHighlightSections(false);

    resizeKeyMappingDataTableColumnWidth(KeyMappingTableWidget);

    KeyMappingTableWidget->verticalHeader()->setVisible(false);
    KeyMappingTableWidget->verticalHeader()->setDefaultSectionSize(25);
    KeyMappingTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    KeyMappingTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    KeyMappingTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    /* Suuport Drag&Drop for KeyMappingData Table */
    KeyMappingTableWidget->setDragEnabled(true);
    KeyMappingTableWidget->setDragDropMode(QAbstractItemView::InternalMove);

    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        KeyMappingTableWidget->setHorizontalHeaderLabels(QStringList()  << KEYMAPDATATABLE_COL1_ENGLISH
                                                                        << KEYMAPDATATABLE_COL2_ENGLISH
                                                                        << KEYMAPDATATABLE_COL3_ENGLISH
                                                                        << KEYMAPDATATABLE_COL4_ENGLISH);
    }
    else {
        KeyMappingTableWidget->setHorizontalHeaderLabels(QStringList()  << KEYMAPDATATABLE_COL1_CHINESE
                                                                        << KEYMAPDATATABLE_COL2_CHINESE
                                                                        << KEYMAPDATATABLE_COL3_CHINESE
                                                                        << KEYMAPDATATABLE_COL4_CHINESE);
    }
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

    // Insert the new tab at the specified index with the generated tabName
    m_KeyMappingTabWidget->insertTab(insert_index, KeyMappingTableWidget, tabName);

    KeyMappingTab_Info tab_info;
    QList<MAP_KEYDATA> *keyMappingData = new QList<MAP_KEYDATA>();
    tab_info.TabName = tabName;
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
    if (m_KeyMappingTabWidget->count() <= 2 || s_KeyMappingTabInfoList.size() <= 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[removeTabFromKeyMappingTabWidget] Can not remove the last tab!" << " ValidTabWidgetCount:" << m_KeyMappingTabWidget->count() - 1 << ", TabInfoListSize:" << s_KeyMappingTabInfoList.size();
#endif
        return REMOVE_MAPPINGTAB_LASTONE;
    }

    if ((tabindex < 0) || (tabindex > m_KeyMappingTabWidget->count() - 2) || (tabindex > s_KeyMappingTabInfoList.size() - 1)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[removeTabFromKeyMappingTabWidget] Invalid index : " << tabindex << ", ValidTabWidgetCount:" << m_KeyMappingTabWidget->count() - 1 << ", TabInfoListSize:" << s_KeyMappingTabInfoList.size();
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
                if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                    showWarningPopup("Invalid input format for WindowSwitchKey!");
                }
                else {
                    showWarningPopup("显示切换键输入格式错误！");
                }
            }
            else if (lineEdit->objectName() == MAPPINGSTARTKEY_LINEEDIT_NAME) {
                lineEdit->setText(s_MappingStartKeyString);
                if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                    showWarningPopup("Invalid input format for MappingStartKey!");
                }
                else {
                    showWarningPopup("映射开启键输入格式错误！");
                }
            }
            else if (lineEdit->objectName() == MAPPINGSTOPKEY_LINEEDIT_NAME) {
                lineEdit->setText(s_MappingStopKeyString);
                if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                    showWarningPopup("Invalid input format for MappingStopKey!");
                }
                else {
                    showWarningPopup("映射关闭键输入格式错误！");
                }
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
        qWarning() << "[onTrayIconMenuShowHideAction]" << "ShowHideAction Triggered.";
#endif

    switchShowHide();
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
}

#ifdef VIGEM_CLIENT_SUPPORT
void QKeyMapper::OrikeyComboBox_currentTextChangedSlot(const QString &text)
{
    if (VJOY_MOUSE2LS_STR == text
        || VJOY_MOUSE2RS_STR == text
        || JOY_LS2MOUSE_STR == text
        || JOY_RS2MOUSE_STR == text
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
}
#endif

bool QKeyMapper::backupFile(const QString &originalFile, const QString &backupFile)
{
    QFile targetFile(backupFile);
    if (targetFile.exists()) {
        targetFile.remove();
    }

    return QFile::copy(originalFile, backupFile);
}

#ifdef SETTINGSFILE_CONVERT
bool QKeyMapper::checkSettingsFileNeedtoConvert()
{
    QFile file(CONFIG_FILENAME);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Could not open the file for reading");
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
        qWarning("Could not open the file for reading");
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
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    settingFile.setIniCodec("UTF-8");
#endif
    // int burstpressTime = ui->burstpressSpinBox->value();
    // int burstreleaseTime = ui->burstreleaseSpinBox->value();
    int key2mouse_XSpeed = ui->mouseXSpeedSpinBox->value();
    int key2mouse_YSpeed = ui->mouseYSpeedSpinBox->value();
#ifdef VIGEM_CLIENT_SUPPORT
    int vJoy_X_Sensitivity = ui->vJoyXSensSpinBox->value();
    int vJoy_Y_Sensitivity = ui->vJoyYSensSpinBox->value();
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

    if (LANGUAGE_ENGLISH == languageIndex) {
        settingFile.setValue(LANGUAGE_INDEX , LANGUAGE_ENGLISH);
    }
    else {
        settingFile.setValue(LANGUAGE_INDEX , LANGUAGE_CHINESE);
    }

    if (!ui->processListButton->isChecked()) {
        settingFile.setValue(SHOW_PROCESSLIST, false);
    }
    else {
        settingFile.setValue(SHOW_PROCESSLIST, true);
    }

    if (ui->showNotesButton->isChecked()) {
        settingFile.setValue(SHOW_NOTES, true);
    }
    else {
        settingFile.setValue(SHOW_NOTES, false);
    }

    settingFile.setValue(NOTIFICATION_POSITION , ui->notificationComboBox->currentIndex());
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
                showWarningPopup(message);
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
                showWarningPopup(message);
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

    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_LASTTABINDEX, s_KeyMappingTabWidgetCurrentIndex);

    QStringList tabnamelist;
    QStringList tabhotkeylist;
    QString original_keys_forsave;
    QString mapping_keysList_forsave;
    QString notesList_forsave;
    QString burstList_forsave;
    QString burstpresstimeList_forsave;
    QString burstreleasetimeList_forsave;
    QString lockList_forsave;
    QString passthroughList_forsave;
    QString keyup_actionList_forsave;
    QString keyseqholddownList_forsave;
    QString repeatmodeList_forsave;
    QString repeattimesList_forsave;

    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index) {
        QString tabName = s_KeyMappingTabInfoList.at(index).TabName;
        QString tabHotkey = s_KeyMappingTabInfoList.at(index).TabHotkey;
        if (isTabTextDuplicateInStringList(tabName, tabnamelist)) {
            tabName = QString();
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[saveKeyMapSetting] TabName:" << tabName << " is already exists, set a empty tabname!";
#endif
        }
        tabnamelist.append(tabName);
        tabhotkeylist.append(tabHotkey);

        QList<MAP_KEYDATA> *mappingDataList = s_KeyMappingTabInfoList.at(index).KeyMappingData;

        // append SEPARATOR_KEYMAPDATA_LEVEL2 to QString variable forsave if it is not the first index
        if (index > 0) {
            original_keys_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            mapping_keysList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            notesList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            burstList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            burstpresstimeList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            burstreleasetimeList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            lockList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            passthroughList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            keyup_actionList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            keyseqholddownList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            repeatmodeList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
            repeattimesList_forsave.append(SEPARATOR_KEYMAPDATA_LEVEL2);
        }

        QStringList original_keys;
        QStringList mapping_keysList;
        QStringList notesList;
        QStringList burstList;
        QStringList burstpresstimeList;
        QStringList burstreleasetimeList;
        QStringList lockList;
        QStringList passthroughList;
        QStringList keyup_actionList;
        QStringList keyseqholddownList;
        QStringList repeatmodeList;
        QStringList repeattimesList;
        if (mappingDataList->size() > 0) {
            for (const MAP_KEYDATA &keymapdata : qAsConst(*mappingDataList))
            {
                original_keys << keymapdata.Original_Key;
                QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
                mapping_keysList  << mappingkeys_str;
                notesList << keymapdata.Note;
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
                if (true == keymapdata.PassThrough) {
                    passthroughList.append("ON");
                }
                else {
                    passthroughList.append("OFF");
                }
                if (true == keymapdata.KeyUp_Action) {
                    keyup_actionList.append("ON");
                }
                else {
                    keyup_actionList.append("OFF");
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
            }
        }
        // join QStringList variables first (use SEPARATOR_KEYMAPDATA_LEVEL1)
        QString original_keys_str = original_keys.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString mapping_keysList_str = mapping_keysList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString notesList_str = notesList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString burstList_str = burstList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString burstpresstimeList_str = burstpresstimeList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString burstreleasetimeList_str = burstreleasetimeList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString lockList_str = lockList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString passthroughList_str = passthroughList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString keyup_actionList_str = keyup_actionList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString keyseqholddownList_str = keyseqholddownList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString repeatmodeList_str = repeatmodeList.join(SEPARATOR_KEYMAPDATA_LEVEL1);
        QString repeattimesList_str = repeattimesList.join(SEPARATOR_KEYMAPDATA_LEVEL1);

        // append joined QString variables to forsave variables
        original_keys_forsave.append(original_keys_str);
        mapping_keysList_forsave.append(mapping_keysList_str);
        notesList_forsave.append(notesList_str);
        burstList_forsave.append(burstList_str);
        burstpresstimeList_forsave.append(burstpresstimeList_str);
        burstreleasetimeList_forsave.append(burstreleasetimeList_str);
        lockList_forsave.append(lockList_str);
        passthroughList_forsave.append(passthroughList_str);
        keyup_actionList_forsave.append(keyup_actionList_str);
        keyseqholddownList_forsave.append(keyseqholddownList_str);
        repeatmodeList_forsave.append(repeatmodeList_str);
        repeattimesList_forsave.append(repeattimesList_str);
    }

    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABNAMELIST, tabnamelist);
    settingFile.setValue(saveSettingSelectStr+MAPPINGTABLE_TABHOTKEYLIST, tabhotkeylist);

    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_ORIGINALKEYS, original_keys_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_MAPPINGKEYS , mapping_keysList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_NOTE , notesList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURST , burstList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTPRESS_TIME , burstpresstimeList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME , burstreleasetimeList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_LOCK , lockList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_PASSTHROUGH , passthroughList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_KEYUP_ACTION , keyup_actionList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_KEYSEQHOLDDOWN , keyseqholddownList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_REPEATMODE, repeatmodeList_forsave);
    settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_REPEATIMES, repeattimesList_forsave);

    settingFile.setValue(saveSettingSelectStr+KEY2MOUSE_X_SPEED , key2mouse_XSpeed);
    settingFile.setValue(saveSettingSelectStr+KEY2MOUSE_Y_SPEED , key2mouse_YSpeed);
    settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_X_SENSITIVITY , vJoy_X_Sensitivity);
    settingFile.setValue(saveSettingSelectStr+MOUSE2VJOY_Y_SENSITIVITY , vJoy_Y_Sensitivity);

    if (saveGlobalSetting) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[saveKeyMapSetting]" << "GlobalSetting do not need processinfo!";
#endif

        // settingFile.setValue(saveSettingSelectStr+DISABLEWINKEY_CHECKED, false);
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

        settingFile.setValue(saveSettingSelectStr+PROCESSINFO_DESCRIPTION, ui->descriptionLineEdit->text());
        // settingFile.setValue(saveSettingSelectStr+DISABLEWINKEY_CHECKED, ui->disableWinKeyCheckBox->isChecked());
    }

    settingFile.setValue(saveSettingSelectStr+AUTOSTARTMAPPING_CHECKED, ui->autoStartMappingCheckBox->checkState());
    settingFile.setValue(saveSettingSelectStr+SENDTOSAMEWINDOWS_CHECKED, ui->sendToSameTitleWindowsCheckBox->isChecked());
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
    bool loadresult = loadKeyMapSetting(savedSettingName);
    Q_UNUSED(loadresult);
    loadSetting_flag = false;

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;
    if (true == loadresult) {
        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
            popupMessage = "Save success : " + savedSettingName;
        }
        else {
            popupMessage = "保存成功 : " + savedSettingName;
        }
        popupMessageColor = "#44bd32";
        bool backupRet = backupFile(CONFIG_FILENAME, CONFIG_LATEST_FILENAME);
        if (backupRet) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[saveKeyMapSetting]" << "Save setting success ->" << savedSettingName;
#endif
        }
    }
    else {
        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
            popupMessage = "Save failure : " + savedSettingName;
        }
        else {
            popupMessage = "映射数据错误 : " + savedSettingName;
        }
        popupMessageColor = "#d63031";
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[saveKeyMapSetting]" << "Mapping data error, Save setting failure!!! ->" << savedSettingName;
#endif
    }
    showPopupMessage(popupMessage, popupMessageColor, popupMessageDisplayTime);
}

bool QKeyMapper::loadKeyMapSetting(const QString &settingtext)
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
    if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
        globalSettingNameWithDescStr = QString(SETTING_DESCRIPTION_FORMAT).arg(GROUPNAME_GLOBALSETTING, GLOBALSETTING_DESC_ENGLISH);
    }
    else {
        globalSettingNameWithDescStr = QString(SETTING_DESCRIPTION_FORMAT).arg(GROUPNAME_GLOBALSETTING, GLOBALSETTING_DESC_CHINESE);
    }
    ui->settingselectComboBox->addItem(globalSettingNameWithDescStr);
    m_SettingSelectListWithoutDescription.append(GROUPNAME_GLOBALSETTING);
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
        QString tempSettingSelectStr = group + "/";
        if (group.endsWith(QString(SEPARATOR_TITLESETTING)+ANYWINDOWTITLE_STRING, Qt::CaseInsensitive)
            || group.contains(QString(SEPARATOR_TITLESETTING)+WINDOWTITLE_STRING)
            || group.startsWith(PROCESS_UNKNOWN+QString(SEPARATOR_TITLESETTING)+WINDOWTITLE_STRING)) {
            if ((true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_FILENAME))
                    && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_WINDOWTITLE))
                    && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_FILEPATH))
                    && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_FILENAME_CHECKED))
                    && (true == settingFile.contains(tempSettingSelectStr+PROCESSINFO_WINDOWTITLE_CHECKED))){
                valid_setting = true;
            }
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
            validgroups_fullmatch.append(group);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting] Setting select add FullMatch ->" << group;
#endif
        }
    }

    for (const QString &group : qAsConst(groups)){
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
    if ((true == settingFile.contains(settingSelectStr+KEYMAPDATA_ORIGINALKEYS))
        && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYS))) {
        QStringList tabnamelist_loaded;
        QString original_keys_loaded;
        QString mapping_keys_loaded;
        QString notes_loaded;
        QString burstData_loaded;
        QString burstpressData_loaded;
        QString burstreleaseData_loaded;
        QString lockData_loaded;
        QString passthroughData_loaded;
        QString keyup_actionData_loaded;
        QString keyseqholddownData_loaded;
        QString repeatmodeData_loaded;
        QString repeattimesData_loaded;
        int table_count = 0;
        QStringList original_keys_split;
        QStringList mapping_keys_split;
        QStringList notes_split;
        QStringList burstData_split;
        QStringList burstpressData_split;
        QStringList burstreleaseData_split;
        QStringList lockData_split;
        QStringList passthroughData_split;
        QStringList keyup_actionData_split;
        QStringList keyseqholddownData_split;
        QStringList repeatmodeData_split;
        QStringList repeattimesData_split;

        original_keys_loaded    = settingFile.value(settingSelectStr+KEYMAPDATA_ORIGINALKEYS).toString();
        mapping_keys_loaded     = settingFile.value(settingSelectStr+KEYMAPDATA_MAPPINGKEYS).toString();
        tabnamelist_loaded      = settingFile.value(settingSelectStr+MAPPINGTABLE_TABNAMELIST).toStringList();
        tabhotkeylist_loaded    = settingFile.value(settingSelectStr+MAPPINGTABLE_TABHOTKEYLIST).toStringList();

        if (original_keys_loaded.isEmpty() || mapping_keys_loaded.isEmpty()) {
            initKeyMappingTable = true;
        }
        else {
            original_keys_split = original_keys_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            mapping_keys_split = mapping_keys_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);

            if (original_keys_split.size() == mapping_keys_split.size()
                && original_keys_split.size() > 0) {
                table_count = original_keys_split.size();
            }
        }

        if (false == initKeyMappingTable && table_count > 0) {
            clearKeyMappingTabWidget();
            KeyMappingDataList->clear();

            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_NOTE)) {
                notes_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_NOTE).toString();
                notes_split = notes_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
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
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_PASSTHROUGH)) {
                passthroughData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_PASSTHROUGH).toString();
                passthroughData_split = passthroughData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_KEYUP_ACTION)) {
                keyup_actionData_loaded = settingFile.value(settingSelectStr+KEYMAPDATA_KEYUP_ACTION).toString();
                keyup_actionData_split = keyup_actionData_loaded.split(SEPARATOR_KEYMAPDATA_LEVEL2);
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

            for (int index = 0; index < table_count && datavalidflag != false; ++index) {
                QList<MAP_KEYDATA> loadkeymapdata;
                bool empty_flag = false;

                if (original_keys_split.at(index).isEmpty() && mapping_keys_split.at(index).isEmpty()) {
                    empty_flag = true;
                }

                if (!empty_flag) {
                    QStringList original_keys;
                    QStringList mapping_keys;
                    QStringList burstStringList;
                    QStringList burstpressStringList;
                    QStringList burstreleaseStringList;
                    QStringList lockStringList;
                    QStringList passthroughStringList;
                    QStringList keyup_actionStringList;
                    QStringList keyseqholddownStringList;
                    QStringList repeatmodeStringList;
                    QStringList repeattimesStringList;
                    QStringList notesList;
                    QList<bool> burstList;
                    QList<int> burstpresstimeList;
                    QList<int> burstreleasetimeList;
                    QList<bool> lockList;
                    QList<bool> passthroughList;
                    QList<bool> keyup_actionList;
                    QList<bool> keyseqholddownList;
                    QList<int> repeatmodeList;
                    QList<int> repeattimesList;

                    original_keys = original_keys_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    mapping_keys = mapping_keys_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);

                    int mappingdata_size = original_keys.size();
                    QStringList stringListAllOFF;
                    QStringList stringListAllZERO;
                    QStringList burstpressStringListDefault;
                    QStringList burstreleaseStringListDefault;
                    QStringList repeattimesStringListDefault;
                    for (int i = 0; i < mappingdata_size; ++i) {
                        stringListAllOFF << "OFF";
                        stringListAllZERO << "0";
                        burstpressStringListDefault.append(QString::number(BURST_PRESS_TIME_DEFAULT));
                        burstreleaseStringListDefault.append(QString::number(BURST_RELEASE_TIME_DEFAULT));
                        repeattimesStringListDefault.append(QString::number(REPEAT_TIMES_DEFAULT));
                    }
                    burstStringList         = stringListAllOFF;
                    burstpressStringList    = burstpressStringListDefault;
                    burstreleaseStringList  = burstreleaseStringListDefault;
                    lockStringList          = stringListAllOFF;
                    passthroughStringList   = stringListAllOFF;
                    keyup_actionStringList   = stringListAllOFF;
                    keyseqholddownStringList = stringListAllOFF;
                    repeatmodeStringList = stringListAllZERO;
                    repeattimesStringList = repeattimesStringListDefault;

                    if (notes_split.size() == table_count) {
                        notesList = notes_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
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
                    if (passthroughData_split.size() == table_count) {
                        passthroughStringList = passthroughData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
                    }
                    if (keyup_actionData_split.size() == table_count) {
                        keyup_actionStringList = keyup_actionData_split.at(index).split(SEPARATOR_KEYMAPDATA_LEVEL1);
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

                    if (original_keys.size() == mapping_keys.size() && original_keys.size() > 0) {
                        datavalidflag = true;

                        if (notesList.size() < original_keys.size()) {
                            int diff = original_keys.size() - notesList.size();
                            for (int i = 0; i < diff; ++i) {
                                notesList.append(QString());
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
                            const QString &passthrough = (i < passthroughStringList.size()) ? passthroughStringList.at(i) : "OFF";
                            if (passthrough == "ON") {
                                passthroughList.append(true);
                            } else {
                                passthroughList.append(false);
                            }
                        }

                        for (int i = 0; i < original_keys.size(); i++) {
                            const QString &keyup_action = (i < keyup_actionStringList.size()) ? keyup_actionStringList.at(i) : "OFF";
                            if (keyup_action == "ON") {
                                keyup_actionList.append(true);
                            } else {
                                keyup_actionList.append(false);
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

                        int loadindex = 0;
                        for (const QString &ori_key_nochange : qAsConst(original_keys)){
                            QString ori_key = ori_key_nochange;
                            if (ori_key.startsWith(OLD_PREFIX_SHORTCUT)) {
                                ori_key.remove(0, 1);
                            }

                            bool checkoriginalstr = checkOriginalkeyStr(ori_key);
                            bool checkmappingstr = checkMappingkeyStr(mapping_keys[loadindex]);

                            if (true == checkoriginalstr && true == checkmappingstr) {
                                loadkeymapdata.append(MAP_KEYDATA(ori_key,
                                                                  mapping_keys.at(loadindex),
                                                                  notesList.at(loadindex),
                                                                  burstList.at(loadindex),
                                                                  burstpresstimeList.at(loadindex),
                                                                  burstreleasetimeList.at(loadindex),
                                                                  lockList.at(loadindex),
                                                                  passthroughList.at(loadindex),
                                                                  keyup_actionList.at(loadindex),
                                                                  keyseqholddownList.at(loadindex),
                                                                  repeatmodeList.at(loadindex),
                                                                  repeattimesList.at(loadindex)
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
                if ((index < s_KeyMappingTabInfoList.size()) && (index < m_KeyMappingTabWidget->count() - 1)) {
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
        ui->nameLineEdit->setText(QString());
        ui->titleLineEdit->setText(QString());
        ui->descriptionLineEdit->setReadOnly(true);
        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
            ui->descriptionLineEdit->setText(GLOBALSETTING_DESC_ENGLISH);
        }
        else {
            ui->descriptionLineEdit->setText(GLOBALSETTING_DESC_CHINESE);
        }
        ui->nameCheckBox->setChecked(false);
        ui->titleCheckBox->setChecked(false);
        // ui->disableWinKeyCheckBox->setChecked(false);
        ui->sendToSameTitleWindowsCheckBox->setChecked(false);

        ui->nameLineEdit->setEnabled(false);
        ui->titleLineEdit->setEnabled(false);
        ui->nameCheckBox->setEnabled(false);
        ui->titleCheckBox->setEnabled(false);
        ui->removeSettingButton->setEnabled(false);
        // ui->disableWinKeyCheckBox->setEnabled(false);
        ui->sendToSameTitleWindowsCheckBox->setEnabled(false);

        ui->iconLabel->clear();
        m_MapProcessInfo = MAP_PROCESSINFO();
    }
    else {
        ui->nameLineEdit->setEnabled(true);
        ui->titleLineEdit->setEnabled(true);
        ui->nameCheckBox->setEnabled(true);
        ui->titleCheckBox->setEnabled(true);
        ui->removeSettingButton->setEnabled(true);
        // ui->disableWinKeyCheckBox->setEnabled(true);
        ui->sendToSameTitleWindowsCheckBox->setEnabled(true);

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

        ui->descriptionLineEdit->setReadOnly(false);
        if (true == settingFile.contains(settingSelectStr+PROCESSINFO_DESCRIPTION)){
            ui->descriptionLineEdit->setText(settingFile.value(settingSelectStr+PROCESSINFO_DESCRIPTION).toString());
        }
        else {
            ui->descriptionLineEdit->clear();
        }

        if (true == loadDefault) {
            // setMapProcessInfo(QString(DEFAULT_NAME), QString(DEFAULT_TITLE), QString(), QString(), QIcon(":/DefaultIcon.ico"));
            ui->nameLineEdit->setText(QString());
            ui->titleLineEdit->setText(QString());
            ui->descriptionLineEdit->setText(QString());
            ui->nameCheckBox->setChecked(false);
            ui->titleCheckBox->setChecked(false);
            ui->sendToSameTitleWindowsCheckBox->setChecked(false);
            m_MapProcessInfo = MAP_PROCESSINFO();
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
            ui->nameCheckBox->setChecked(false);
            ui->titleCheckBox->setChecked(false);
        }

#if 0
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
#endif
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

    if (false == datavalidflag){
        // if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
        //     QMessageBox::warning(this, PROGRAM_NAME, "<html><head/><body><p align=\"center\">Load invalid keymapdata from INI file.</p><p align=\"center\">Reset to default values.</p></body></html>");
        // }
        // else {
        //     QMessageBox::warning(this, PROGRAM_NAME, "<html><head/><body><p align=\"center\">从INI文件加载了无效的设定数据。</p><p align=\"center\">显示默认设定。</p></body></html>");
        // }
        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
            showWarningPopup("Invalid mapping data : " + settingtext);
        }
        else {
            showWarningPopup("无效的映射数据 : " + settingtext);
        }
        return false;
    }
    else {
        ui->nameLineEdit->setCursorPosition(0);
        ui->titleLineEdit->setCursorPosition(0);

        if (settingSelectStr.isEmpty() != true) {
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
        }

        if ((Qt::Checked == autoStartMappingCheckState) && (true == settingtext.isEmpty())) {
            MappingSwitch(MAPPINGSTART_LOADSETTING);
        }
        return true;
    }
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
    ui->deleteoneButton->setFont(customFont);
    ui->clearallButton->setFont(customFont);
    ui->processListButton->setFont(customFont);
    ui->showNotesButton->setFont(customFont);
    ui->nameCheckBox->setFont(customFont);
    ui->titleCheckBox->setFont(customFont);
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
    ui->settingTabWidget->tabBar()->setFont(customFont);
    ui->windowswitchkeyLabel->setFont(customFont);
    ui->mappingStartKeyLabel->setFont(customFont);
    ui->mappingStopKeyLabel->setFont(customFont);
    ui->installViGEmBusButton->setFont(customFont);
    // ui->uninstallViGEmBusButton->setFont(customFont);
    ui->enableVirtualJoystickCheckBox->setFont(customFont);
    ui->lockCursorCheckBox->setFont(customFont);
    ui->ViGEmBusStatusLabel->setFont(customFont);
    ui->vJoyXSensLabel->setFont(customFont);
    ui->vJoyYSensLabel->setFont(customFont);
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
    ui->autoStartupCheckBox->setFont(customFont);
    ui->startupMinimizedCheckBox->setFont(customFont);
    ui->soundEffectCheckBox->setFont(customFont);
    ui->notificationLabel->setFont(customFont);
    ui->languageLabel->setFont(customFont);

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
    ui->processListButton->setFont(customFont);
    ui->showNotesButton->setFont(customFont);
    ui->nameCheckBox->setFont(customFont);
    ui->titleCheckBox->setFont(customFont);
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

    // ui->processinfoTable->horizontalHeader()->setFont(customFont);
    // m_KeyMappingDataTable->horizontalHeader()->setFont(customFont);

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(11);
    // }
    // else {
    //     customFont.setPointSize(8);
    // }
    ui->settingTabWidget->tabBar()->setFont(customFont);
    ui->windowswitchkeyLabel->setFont(customFont);
    ui->mappingStartKeyLabel->setFont(customFont);
    ui->mappingStopKeyLabel->setFont(customFont);
    ui->installViGEmBusButton->setFont(customFont);
    // ui->uninstallViGEmBusButton->setFont(customFont);
    ui->enableVirtualJoystickCheckBox->setFont(customFont);
    ui->lockCursorCheckBox->setFont(customFont);
    ui->ViGEmBusStatusLabel->setFont(customFont);
    ui->vJoyXSensLabel->setFont(customFont);
    ui->vJoyYSensLabel->setFont(customFont);
    // ui->virtualgamepadGroupBox->setFont(customFont);
    // ui->multiInputGroupBox->setFont(customFont);
    ui->installInterceptionButton->setFont(customFont);
    ui->multiInputDeviceListButton->setFont(customFont);
    ui->multiInputStatusLabel->setFont(customFont);
    ui->multiInputEnableCheckBox->setFont(customFont);
    ui->filterKeysCheckBox->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(11);
    }
    else {
        customFont.setPointSize(9);
    }
    ui->autoStartMappingCheckBox->setFont(customFont);
    ui->sendToSameTitleWindowsCheckBox->setFont(customFont);
    ui->autoStartupCheckBox->setFont(customFont);
    ui->startupMinimizedCheckBox->setFont(customFont);
    ui->soundEffectCheckBox->setFont(customFont);
    ui->notificationLabel->setFont(customFont);
    ui->languageLabel->setFont(customFont);

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(11);
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
    if (true == status && GLOBALSETTING_INDEX == ui->settingselectComboBox->currentIndex()) {
        ui->nameCheckBox->setEnabled(false);
        ui->titleCheckBox->setEnabled(false);
        ui->removeSettingButton->setEnabled(false);
        // ui->disableWinKeyCheckBox->setEnabled(false);
        ui->sendToSameTitleWindowsCheckBox->setEnabled(false);
    }
    else {
        ui->nameCheckBox->setEnabled(status);
        ui->titleCheckBox->setEnabled(status);
        ui->removeSettingButton->setEnabled(status);
        // ui->disableWinKeyCheckBox->setEnabled(status);
        if (ui->autoStartMappingCheckBox->isChecked()) {
            ui->sendToSameTitleWindowsCheckBox->setEnabled(false);
        }
        else {
            ui->sendToSameTitleWindowsCheckBox->setEnabled(status);
        }
    }

    //ui->nameLineEdit->setEnabled(status);
    //ui->titleLineEdit->setEnabled(status);
    ui->titleLineEdit->setReadOnly(!status);
    ui->descriptionLabel->setEnabled(status);
    ui->descriptionLineEdit->setEnabled(status);
    ui->autoStartMappingCheckBox->setEnabled(status);
    ui->autoStartupCheckBox->setEnabled(status);
    ui->startupMinimizedCheckBox->setEnabled(status);
    ui->soundEffectCheckBox->setEnabled(status);
    ui->notificationLabel->setEnabled(status);
    ui->notificationComboBox->setEnabled(status);
    ui->languageLabel->setEnabled(status);
    ui->languageComboBox->setEnabled(status);
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
    ui->sendTextLabel->setEnabled(status);
    ui->pointLabel->setEnabled(status);
    // ui->waitTime_msLabel->setEnabled(status);
    ui->waitTimeSpinBox->setEnabled(status);
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
    ui->deleteoneButton->setEnabled(status);
    ui->clearallButton->setEnabled(status);
    ui->processListButton->setEnabled(status);
    ui->showNotesButton->setEnabled(status);

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
        ui->lockCursorCheckBox->setEnabled(status);
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

    ui->moveupButton->setEnabled(status);
    ui->movedownButton->setEnabled(status);
    ui->nextarrowCheckBox->setEnabled(status);

    ui->windowswitchkeyLabel->setEnabled(status);
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
    QString popupNotification;
    int position = ui->notificationComboBox->currentIndex();
    if (NOTIFICATION_POSITION_NONE == position) {
        return;
    }
    QString currentSelectedSetting = ui->settingselectComboBox->currentText();
    int currentSelectedIndex = ui->settingselectComboBox->currentIndex();
    QString tabName = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabName;
    QString color = "#d6a2e8";
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        popupNotification = "StartMapping [" + currentSelectedSetting + "]" + "->" + tabName;
    }
    else {
        popupNotification = "开始映射 [" + currentSelectedSetting + "]" + "->" + tabName;
        if (GLOBALSETTING_INDEX == currentSelectedIndex) {
            color = "#26de81";
        }
    }
    showNotificationPopup(popupNotification, color, position);
}

void QKeyMapper::mappingStopNotification()
{
    QString popupNotification;
    int position = ui->notificationComboBox->currentIndex();
    if (NOTIFICATION_POSITION_NONE == position) {
        return;
    }
    QString mappingStatusString;
    if (KEYMAP_IDLE == m_KeyMapStatus) {
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            mappingStatusString = "Idle";
        }
        else {
            mappingStatusString = "空闲";
        }
    }
    else if (KEYMAP_CHECKING == m_KeyMapStatus) {
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            mappingStatusString = "Checking";
        }
        else {
            mappingStatusString = "监测中";
        }
    }

    if (mappingStatusString.isEmpty()) {
        return;
    }

    QString color = "#d6a2e8";
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        popupNotification = "StopMapping [" + mappingStatusString + "]";
    }
    else {
        popupNotification = "停止映射 [" + mappingStatusString + "]";
    }
    showNotificationPopup(popupNotification, color, position);
}

void QKeyMapper::mappingTabSwitchNotification()
{
    QString popupNotification;
    int position = ui->notificationComboBox->currentIndex();
    if (NOTIFICATION_POSITION_NONE == position) {
        return;
    }
    QString currentSelectedSetting = ui->settingselectComboBox->currentText();
    int currentSelectedIndex = ui->settingselectComboBox->currentIndex();
    QString tabName = s_KeyMappingTabInfoList.at(s_KeyMappingTabWidgetCurrentIndex).TabName;
    QString color = "#d6a2e8";
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        popupNotification = "MappingTabSwitch [" + currentSelectedSetting + "]" + "->" + tabName;
    }
    else {
        popupNotification = "切换映射表 [" + currentSelectedSetting + "]" + "->" + tabName;
        if (GLOBALSETTING_INDEX == currentSelectedIndex) {
            color = "#26de81";
        }
    }
    showNotificationPopup(popupNotification, color, position);
}

void QKeyMapper::showInputDeviceListWindow()
{
    if (!m_deviceListWindow->isVisible()) {
        m_deviceListWindow->show();
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

    int languageIndex = ui->languageComboBox->currentIndex();
    if (QKeyMapper_Worker::VIGEMCLIENT_CONNECT_SUCCESS == connectstate) {
        if (m_KeyMapStatus == KEYMAP_IDLE){
            ui->enableVirtualJoystickCheckBox->setEnabled(true);
        }
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
    for (const QString &gamepad : qAsConst(QKeyMapper_Worker::s_VirtualGamepadList))
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

    int languageIndex = ui->languageComboBox->currentIndex();
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

        if (LANGUAGE_ENGLISH == languageIndex) {
            ui->installInterceptionButton->setText(UNINSTALLINTERCEPTIONBUTTON_ENGLISH);
        }
        else {
            ui->installInterceptionButton->setText(UNINSTALLINTERCEPTIONBUTTON_CHINESE);
        }

        ui->multiInputStatusLabel->setStyleSheet("color:green;");
        if (LANGUAGE_ENGLISH == languageIndex) {
            ui->multiInputStatusLabel->setText(MULTIINPUTSTATUSLABEL_AVAILABLE_ENGLISH);
        }
        else {
            ui->multiInputStatusLabel->setText(MULTIINPUTSTATUSLABEL_AVAILABLE_CHINESE);
        }
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

        if (LANGUAGE_ENGLISH == languageIndex) {
            ui->installInterceptionButton->setText(INSTALLINTERCEPTIONBUTTON_ENGLISH);
        }
        else {
            ui->installInterceptionButton->setText(INSTALLINTERCEPTIONBUTTON_CHINESE);
        }

        ui->multiInputStatusLabel->setStyleSheet("color: orange;");
        if (LANGUAGE_ENGLISH == languageIndex) {
            ui->multiInputStatusLabel->setText(MULTIINPUTSTATUSLABEL_REBOOTREQUIRED_ENGLISH);
        }
        else {
            ui->multiInputStatusLabel->setText(MULTIINPUTSTATUSLABEL_REBOOTREQUIRED_CHINESE);
        }
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

        if (LANGUAGE_ENGLISH == languageIndex) {
            ui->installInterceptionButton->setText(INSTALLINTERCEPTIONBUTTON_ENGLISH);
        }
        else {
            ui->installInterceptionButton->setText(INSTALLINTERCEPTIONBUTTON_CHINESE);
        }

        ui->multiInputStatusLabel->setStyleSheet("color: red;");
        if (LANGUAGE_ENGLISH == languageIndex) {
            ui->multiInputStatusLabel->setText(MULTIINPUTSTATUSLABEL_UNAVAILABLE_ENGLISH);
        }
        else {
            ui->multiInputStatusLabel->setText(MULTIINPUTSTATUSLABEL_UNAVAILABLE_CHINESE);
        }
    }
}

void QKeyMapper::updateInputDeviceSelectComboBoxes()
{
    initInputDeviceSelectComboBoxes();
}

void QKeyMapper::updateGamepadSelectComboBox()
{
    QMap<int, Gamepad_Info> GamepadInfoMap;

    QList<QJoystickDevice *> joysticklist = QJoysticks::getInstance()->inputDevices();

    for (const QJoystickDevice *joystick : qAsConst(joysticklist)) {
        int player_index = joystick->playerindex;
        if (JOYSTICK_PLAYER_INDEX_MIN <= player_index && player_index <= JOYSTICK_PLAYER_INDEX_MAX) {
            Gamepad_Info gamepadinfo;
            gamepadinfo.name = joystick->name;
            gamepadinfo.vendorid = joystick->vendorid;
            gamepadinfo.productid = joystick->productid;
            gamepadinfo.serial = joystick->serial;
            gamepadinfo.isvirtual = joystick->blacklisted;
            gamepadinfo.info_string = QString("[%1] %2 [VID=0x%3][PID=0x%4]")
                .arg(player_index)
                .arg(joystick->name,
                     QString::number(joystick->vendorid, 16).toUpper().rightJustified(4, '0'),
                     QString::number(joystick->productid, 16).toUpper().rightJustified(4, '0'));
            if (gamepadinfo.isvirtual) {
                gamepadinfo.info_string.append("[ViGEM]");
            }

            GamepadInfoMap.insert(player_index, gamepadinfo);
        }
    }

    m_GamepadInfoMap = GamepadInfoMap;

    ui->gamepadSelectComboBox->clear();
    QStringList gamepadInfoList;
    gamepadInfoList.append(QString());

    QList<int> playerindexlist = GamepadInfoMap.keys();
    for (const int& playerindex : playerindexlist) {
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
}

void QKeyMapper::updateKeyMappingTabWidgetTabName(int tabindex, const QString &tabname)
{
    if ((tabindex < 0) || (tabindex > m_KeyMappingTabWidget->count() - 2) || (tabindex > s_KeyMappingTabInfoList.size() - 1)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[updateKeyMappingTabWidgetTabName] Invalid index : " << tabindex << ", ValidTabWidgetCount:" << m_KeyMappingTabWidget->count() - 1 << ", TabInfoListSize:" << s_KeyMappingTabInfoList.size();
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
    m_SysTrayIcon->setIcon(QIcon(":/QKeyMapper.ico"));
    m_SysTrayIcon->setToolTip("QKeyMapper(Idle)");

    m_SysTrayIconMenu = new QMenu(this);
    m_TrayIconMenu_ShowHideAction = new QAction(this);
    m_TrayIconMenu_QuitAction = new QAction(this);

    // When the Show/Hide menu item is clicked, toggle the visibility of the window
    connect(m_TrayIconMenu_ShowHideAction, &QAction::triggered, this, &QKeyMapper::onTrayIconMenuShowHideAction);
    // When the Quit menu item is clicked, close the application
    connect(m_TrayIconMenu_QuitAction, &QAction::triggered, qApp, &QApplication::quit);

    updateSysTrayIconMenuText();

    m_SysTrayIconMenu->addAction(m_TrayIconMenu_ShowHideAction);
    m_SysTrayIconMenu->addSeparator(); // Add a separator horizontal line
    m_SysTrayIconMenu->addAction(m_TrayIconMenu_QuitAction);

    // Add the created menu to the system tray icon
    m_SysTrayIcon->setContextMenu(m_SysTrayIconMenu);

    m_SysTrayIcon->show();
}

void QKeyMapper::initPopupMessage()
{
    m_PopupMessageLabel = new QLabel(this);
    m_PopupMessageLabel->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    m_PopupMessageLabel->setAttribute(Qt::WA_TranslucentBackground);
    m_PopupMessageLabel->setAttribute(Qt::WA_ShowWithoutActivating);
    m_PopupMessageLabel->setAlignment(Qt::AlignCenter);

    m_PopupMessageAnimation = new QPropertyAnimation(m_PopupMessageLabel, "windowOpacity", this);
    m_PopupMessageAnimation->setStartValue(1.0);
    m_PopupMessageAnimation->setEndValue(0.0);
}

void QKeyMapper::updateSysTrayIconMenuText()
{
    QString showActionText;
    QString hideActionText;
    QString quitActionText;

    int languageIndex = ui->languageComboBox->currentIndex();
    if (LANGUAGE_ENGLISH == languageIndex) {
        showActionText = TRAYMENU_SHOWACTION_ENGLISH;
        hideActionText = TRAYMENU_HIDEACTION_ENGLISH;
        quitActionText = TRAYMENU_QUITACTION_ENGLISH;
    }
    else {
        showActionText = TRAYMENU_SHOWACTION_CHINESE;
        hideActionText = TRAYMENU_HIDEACTION_CHINESE;
        quitActionText = TRAYMENU_QUITACTION_CHINESE;
    }

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

void QKeyMapper::showQKeyMapperWindowToTop()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[showQKeyMapperWindowToTop]" << "m_MainWindowHandle ->" << m_MainWindowHandle;
#endif
    /* BringWindowToTopEx() may cause OBS program registered shortcut be invalid. >>> */
    if (m_MainWindowHandle != NULL) {
        // BringWindowToTopEx(m_MainWindowHandle);
        SetWindowPos(m_MainWindowHandle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
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
        closeTableSetupDialog();
        closeItemSetupDialog();
        hide();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[switchShowHide] Hide Window, LastWindowPosition ->" << m_LastWindowPosition;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[switchShowHide] Show Window, LastWindowPosition ->" << m_LastWindowPosition;
#endif
        if (NULL == m_MainWindowHandle) {
            m_MainWindowHandle = reinterpret_cast<HWND>(winId());
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::switchShowHide]" << "m_MainWindowHandle ->" << m_MainWindowHandle;
#endif
        }

        if (m_LastWindowPosition.x() != INITIAL_WINDOW_POSITION && m_LastWindowPosition.y() != INITIAL_WINDOW_POSITION) {
            move(m_LastWindowPosition); // Restore the position before showing
        }

        showQKeyMapperWindowToTop();
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

void QKeyMapper::showWarningPopup(const QString &message)
{
    showPopupMessage(message, "#d63031", 3000);
}

void QKeyMapper::showNotificationPopup(const QString &message, const QString &color, int position)
{
    if (Q_NULLPTR == m_PopupNotification) {
        return;
    }
    m_PopupNotification->showPopupNotification(message, color, 3000, position);
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
    m_KeyMappingTabWidget->setFocusPolicy(Qt::NoFocus);
    m_KeyMappingTabWidget->setStyle(QStyleFactory::create("windows"));
    m_KeyMappingTabWidget->setFont(QFont(FONTNAME_ENGLISH, 9));

    KeyMappingDataTableWidget *AddTAB_TableWidget = new KeyMappingDataTableWidget(this);
    m_KeyMappingTabWidget->addTab(AddTAB_TableWidget, ADDTAB_TAB_TEXT);
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

    for (int tabindex = m_KeyMappingTabWidget->count() - 2; tabindex > 0; --tabindex) {
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

    int original_key_width_min = mappingDataTable->width()/4 - 15;
    int original_key_width = mappingDataTable->columnWidth(ORIGINAL_KEY_COLUMN);

    mappingDataTable->resizeColumnToContents(BURST_MODE_COLUMN);
    int burst_mode_width = mappingDataTable->columnWidth(BURST_MODE_COLUMN);
    int lock_width = burst_mode_width;
    burst_mode_width += 8;
    // int burst_mode_width = mappingDataTable->width()/5 - 40;
    // int lock_width = mappingDataTable->width()/5 - 40;

    if (original_key_width < original_key_width_min) {
        original_key_width = original_key_width_min;
    }

    int mapping_key_width_min = mappingDataTable->width()/4 - 15;
    int mapping_key_width = mappingDataTable->width() - original_key_width - burst_mode_width - lock_width - 16;
    if (mapping_key_width < mapping_key_width_min) {
        mapping_key_width = mapping_key_width_min;
    }

    mappingDataTable->setColumnWidth(ORIGINAL_KEY_COLUMN, original_key_width);
    mappingDataTable->setColumnWidth(MAPPING_KEY_COLUMN, mapping_key_width);
    mappingDataTable->setColumnWidth(BURST_MODE_COLUMN, burst_mode_width);
    mappingDataTable->setColumnWidth(LOCK_COLUMN, lock_width);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[resizeKeyMappingDataTableColumnWidth]" << "mappingDataTable->rowCount" << mappingDataTable->rowCount();
    qDebug() << "[resizeKeyMappingDataTableColumnWidth]" << "original_key_width =" << original_key_width << ", mapping_key_width =" << mapping_key_width << ", burst_mode_width =" << burst_mode_width << ", lock_width =" << lock_width;
#endif
}

void QKeyMapper::resizeAllKeyMappingTabWidgetColumnWidth()
{
    for (int tabindex = 0; tabindex < m_KeyMappingTabWidget->count() - 1; ++tabindex) {
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
void QKeyMapper::initAddKeyComboBoxes(void)
{
    QStringList keycodelist = QStringList() \
            << ""
            << KEY_BLOCKED_STR
            << KEY_NONE_STR
            << MOUSE_L_STR
            << MOUSE_R_STR
            << MOUSE_M_STR
            << MOUSE_X1_STR
            << MOUSE_X2_STR
            << MOUSE_L_WINDOWPOINT_STR
            << MOUSE_R_WINDOWPOINT_STR
            << MOUSE_M_WINDOWPOINT_STR
            << MOUSE_X1_WINDOWPOINT_STR
            << MOUSE_X2_WINDOWPOINT_STR
            << MOUSE_L_SCREENPOINT_STR
            << MOUSE_R_SCREENPOINT_STR
            << MOUSE_M_SCREENPOINT_STR
            << MOUSE_X1_SCREENPOINT_STR
            << MOUSE_X2_SCREENPOINT_STR
            << MOUSE_WHEEL_UP_STR
            << MOUSE_WHEEL_DOWN_STR
            << MOUSE_WHEEL_LEFT_STR
            << MOUSE_WHEEL_RIGHT_STR
            << SENDTEXT_STR
            << KEYSEQUENCEBREAK_STR
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
            << KEY2MOUSE_UP_STR
            << KEY2MOUSE_DOWN_STR
            << KEY2MOUSE_LEFT_STR
            << KEY2MOUSE_RIGHT_STR
#ifdef VIGEM_CLIENT_SUPPORT
            << VJOY_MOUSE2LS_STR
            << VJOY_MOUSE2RS_STR
            << MOUSE2VJOY_HOLD_KEY_STR
            << MOUSE2VJOY_DIRECT_KEY_STR
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
            << "Joy-Key13(Guide)"
#ifdef VIGEM_CLIENT_SUPPORT
            << JOY_LS2VJOYLS_STR
            << JOY_RS2VJOYRS_STR
            << JOY_LS2VJOYRS_STR
            << JOY_RS2VJOYLS_STR
            << JOY_LT2VJOYLT_STR
            << JOY_RT2VJOYRT_STR
#endif
            << JOY_LS2MOUSE_STR
            << JOY_RS2MOUSE_STR
            << "Joy-Key14"
            << "Joy-Key15"
            << "Joy-Key16"
            << "Joy-Key17"
            << "Joy-Key18"
            << "Joy-Key19"
            ;

    int left = ui->orikeyLabel->x() + ui->orikeyLabel->width() + 5;
    int top = ui->orikeyLabel->y();
    m_orikeyComboBox->setObjectName(ORIKEY_COMBOBOX_NAME);
    m_orikeyComboBox->setGeometry(QRect(left, top, 142, 22));
    left = ui->mapkeyLabel->x() + ui->mapkeyLabel->width() + 5;
    top = ui->mapkeyLabel->y();
    m_mapkeyComboBox->setObjectName(MAPKEY_COMBOBOX_NAME);
    m_mapkeyComboBox->setGeometry(QRect(left, top, 122, 22));

    QStringList orikeycodelist = keycodelist;
    orikeycodelist.removeOne(KEY_NONE_STR);
    orikeycodelist.removeOne(KEY_BLOCKED_STR);
    orikeycodelist.removeOne(MOUSE_L_WINDOWPOINT_STR);
    orikeycodelist.removeOne(MOUSE_R_WINDOWPOINT_STR);
    orikeycodelist.removeOne(MOUSE_M_WINDOWPOINT_STR);
    orikeycodelist.removeOne(MOUSE_X1_WINDOWPOINT_STR);
    orikeycodelist.removeOne(MOUSE_X2_WINDOWPOINT_STR);
    orikeycodelist.removeOne(MOUSE_L_SCREENPOINT_STR);
    orikeycodelist.removeOne(MOUSE_R_SCREENPOINT_STR);
    orikeycodelist.removeOne(MOUSE_M_SCREENPOINT_STR);
    orikeycodelist.removeOne(MOUSE_X1_SCREENPOINT_STR);
    orikeycodelist.removeOne(MOUSE_X2_SCREENPOINT_STR);
    orikeycodelist.removeOne(SENDTEXT_STR);
    orikeycodelist.removeOne(KEYSEQUENCEBREAK_STR);
    orikeycodelist.removeOne(KEY2MOUSE_UP_STR);
    orikeycodelist.removeOne(KEY2MOUSE_DOWN_STR);
    orikeycodelist.removeOne(KEY2MOUSE_LEFT_STR);
    orikeycodelist.removeOne(KEY2MOUSE_RIGHT_STR);
    orikeycodelist.removeOne(MOUSE2VJOY_HOLD_KEY_STR);
    orikeycodelist.removeOne(MOUSE2VJOY_DIRECT_KEY_STR);

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

    QRegularExpression re_Func("^Func-");
    QStringList FuncStrlist = orikeycodelist.filter(re_Func);
    // Remove Strings start with "Func-" from orikeyComboBox
    for (const QString &funcstr : qAsConst(FuncStrlist)){
        orikeycodelist.removeOne(funcstr);
    }

    m_orikeyComboBox->addItems(orikeycodelist);
    m_mapkeyComboBox->addItems(keycodelist);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(m_mapkeyComboBox->model());
    QStandardItem* item = model->item(1);
    item->setData(QColor(Qt::darkMagenta), Qt::ForegroundRole);
    item = model->item(2);
    item->setData(QColor(Qt::darkMagenta), Qt::ForegroundRole);
#else
    QBrush colorBrush(Qt::darkMagenta);
    m_mapkeyComboBox->setItemData(1, colorBrush, Qt::TextColorRole);
#endif
}

void QKeyMapper::initInputDeviceSelectComboBoxes()
{
    initKeyboardSelectComboBox();
    initMouseSelectComboBox();
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
    for (const InputDevice &inputdevice : keyboardlist)
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
    for (const InputDevice &inputdevice : mouselist)
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

void QKeyMapper::setKeyMappingTabWidgetCurrentIndex(int index)
{
    if (0 <= index && index < m_KeyMappingTabWidget->count() - 1) {
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
        for (const MAP_KEYDATA &keymapdata : qAsConst(*mappingDataList))
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
            if (keymapdata.Original_Key == JOY_LS2MOUSE_STR || keymapdata.Original_Key == JOY_RS2MOUSE_STR) {
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
            else if (keymapdata.Mapping_Keys.constFirst().startsWith(FUNC_PREFIX)) {
                disable_burst = true;
                disable_lock = true;
            }
            else if (keymapdata.Mapping_Keys.constFirst().contains(MOUSE2VJOY_HOLD_KEY_STR)
                || keymapdata.Mapping_Keys.constFirst().contains(MOUSE2VJOY_DIRECT_KEY_STR)) {
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
            if (keymapdata.PassThrough) {
                ori_TableItem->setForeground(QBrush(STATUS_ON_COLOR));
            }
            if (keymapdata.KeyUp_Action) {
                QFont font = ori_TableItem->font();
                font.setUnderline(true);
                ori_TableItem->setFont(font);
            }
            mappingDataTable->setItem(rowindex, ORIGINAL_KEY_COLUMN  , ori_TableItem);

            /* MAPPING_KEY_COLUMN */
            QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
            QTableWidgetItem *mapping_TableItem = new QTableWidgetItem(mappingkeys_str);
            mapping_TableItem->setToolTip(mappingkeys_str);
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
}

void QKeyMapper::refreshAllKeyMappingTagWidget()
{
    for (int index = 0; index < s_KeyMappingTabInfoList.size(); ++index) {
        KeyMappingDataTableWidget *mappingDataTable = s_KeyMappingTabInfoList.at(index).KeyMappingDataTable;
        QList<MAP_KEYDATA> *mappingDataList = s_KeyMappingTabInfoList.at(index).KeyMappingData;

        refreshKeyMappingDataTable(mappingDataTable, mappingDataList);
    }

    updateMousePointsList();
}

void QKeyMapper::updateMousePointsList()
{
    if (KeyMappingDataList->isEmpty()) {
        ScreenMousePointsList.clear();
        WindowMousePointsList.clear();
        return;
    }

    static QRegularExpression mousepoint_regex(R"(Mouse-(L|R|M|X1|X2)(:W)?\((\d+),(\d+)\))");
    QRegularExpressionMatch mousepoint_match;
    ScreenMousePointsList.clear();
    WindowMousePointsList.clear();

    for (const MAP_KEYDATA &keymapdata : qAsConst(*KeyMappingDataList))
    {
        QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);

        mousepoint_match = mousepoint_regex.match(mappingkeys_str);
        while (mousepoint_match.hasMatch())
        {
            QString ori_key = keymapdata.Original_Key;
            QString map_key = mousepoint_match.captured(0);
            bool isWindowPoint = !mousepoint_match.captured(2).isEmpty();
            QString x_str = mousepoint_match.captured(3);
            QString y_str = mousepoint_match.captured(4);
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
        setUILanguage_English();
    }
    else {
        setUILanguage_Chinese();
    }

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
    ui->processListButton->setText(PROCESSLISTBUTTON_CHINESE);
    ui->showNotesButton->setText(SHOWNOTESBUTTON_CHINESE);
    ui->addmapdataButton->setText(ADDMAPDATABUTTON_CHINESE);
    ui->nameCheckBox->setText(NAMECHECKBOX_CHINESE);
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
    ui->autoStartupCheckBox->setText(AUTOSTARTUPCHECKBOX_CHINESE);
    ui->startupMinimizedCheckBox->setText(STARTUPMINIMIZEDCHECKBOX_CHINESE);
    ui->soundEffectCheckBox->setText(SOUNDEFFECTCHECKBOX_CHINESE);
    ui->notificationLabel->setText(NOTIFICATIONLABEL_CHINESE);
    ui->languageLabel->setText(LANGUAGELABEL_CHINESE);
    ui->windowswitchkeyLabel->setText(WINDOWSWITCHKEYLABEL_CHINESE);
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
    for (int tabindex = 0; tabindex < m_KeyMappingTabWidget->count() - 1; ++tabindex) {
        KeyMappingDataTableWidget *mappingTable = qobject_cast<KeyMappingDataTableWidget*>(m_KeyMappingTabWidget->widget(tabindex));
        mappingTable->setHorizontalHeaderLabels(QStringList()   << KEYMAPDATATABLE_COL1_CHINESE
                                                                << KEYMAPDATATABLE_COL2_CHINESE
                                                                << KEYMAPDATATABLE_COL3_CHINESE
                                                                << KEYMAPDATATABLE_COL4_CHINESE);
    }

    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << PROCESSINFOTABLE_COL1_CHINESE
                                                                  << PROCESSINFOTABLE_COL2_CHINESE
                                                                  << PROCESSINFOTABLE_COL3_CHINESE );

    if (m_deviceListWindow != Q_NULLPTR) {
        m_deviceListWindow->setUILanguagee(LANGUAGE_CHINESE);
    }

    if (m_TableSetupDialog != Q_NULLPTR) {
        m_TableSetupDialog->setUILanguagee(LANGUAGE_CHINESE);
    }

    if (m_ItemSetupDialog != Q_NULLPTR) {
        m_ItemSetupDialog->setUILanguagee(LANGUAGE_CHINESE);
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
    ui->deleteoneButton->setText(DELETEONEBUTTON_ENGLISH);
    ui->clearallButton->setText(CLEARALLBUTTON_ENGLISH);
    ui->processListButton->setText(PROCESSLISTBUTTON_ENGLISH);
    ui->showNotesButton->setText(SHOWNOTESBUTTON_ENGLISH);
    ui->addmapdataButton->setText(ADDMAPDATABUTTON_ENGLISH);
    ui->nameCheckBox->setText(NAMECHECKBOX_ENGLISH);
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
    ui->autoStartupCheckBox->setText(AUTOSTARTUPCHECKBOX_ENGLISH);
    ui->startupMinimizedCheckBox->setText(STARTUPMINIMIZEDCHECKBOX_ENGLISH);
    ui->soundEffectCheckBox->setText(SOUNDEFFECTCHECKBOX_ENGLISH);
    ui->notificationLabel->setText(NOTIFICATIONLABEL_ENGLISH);
    ui->languageLabel->setText(LANGUAGELABEL_ENGLISH);
    ui->windowswitchkeyLabel->setText(WINDOWSWITCHKEYLABEL_ENGLISH);
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
    for (int tabindex = 0; tabindex < m_KeyMappingTabWidget->count() - 1; ++tabindex) {
        KeyMappingDataTableWidget *mappingTable = qobject_cast<KeyMappingDataTableWidget*>(m_KeyMappingTabWidget->widget(tabindex));
        mappingTable->setHorizontalHeaderLabels(QStringList()   << KEYMAPDATATABLE_COL1_ENGLISH
                                                                << KEYMAPDATATABLE_COL2_ENGLISH
                                                                << KEYMAPDATATABLE_COL3_ENGLISH
                                                                << KEYMAPDATATABLE_COL4_ENGLISH);
    }

    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << PROCESSINFOTABLE_COL1_ENGLISH
                                                                  << PROCESSINFOTABLE_COL2_ENGLISH
                                                                  << PROCESSINFOTABLE_COL3_ENGLISH );

    if (m_deviceListWindow != Q_NULLPTR) {
        m_deviceListWindow->setUILanguagee(LANGUAGE_ENGLISH);
    }

    if (m_TableSetupDialog != Q_NULLPTR) {
        m_TableSetupDialog->setUILanguagee(LANGUAGE_ENGLISH);
    }

    if (m_ItemSetupDialog != Q_NULLPTR) {
        m_ItemSetupDialog->setUILanguagee(LANGUAGE_ENGLISH);
    }
}

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

        ui->nameLineEdit->setFont(customFont);
        ui->titleLineEdit->setFont(customFont);
        ui->descriptionLineEdit->setFont(customFont);
        ui->languageComboBox->setFont(customFont);
        ui->notificationComboBox->setFont(customFont);
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
        ui->nameLineEdit->setFont(customFont);
        ui->titleLineEdit->setFont(customFont);
        ui->descriptionLineEdit->setFont(customFont);
        ui->languageComboBox->setFont(customFont);
        ui->notificationComboBox->setFont(customFont);
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

void QKeyMapper::updateLockStatusDisplay()
{
    int rowindex = 0;
    for (const MAP_KEYDATA &keymapdata : qAsConst(*KeyMappingDataList))
    {
        if (m_KeyMapStatus == KEYMAP_MAPPING_MATCHED
            || m_KeyMapStatus == KEYMAP_MAPPING_GLOBAL) {
            if (keymapdata.Lock == true) {
                if (keymapdata.LockStatus == true) {
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
        SetWindowLongPtr(m_TransParentHandle, GWLP_USERDATA, SHOW_MODE_SCREEN);
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
            SetWindowLongPtr(m_TransParentHandle, GWLP_USERDATA, SHOW_MODE_WINDOW);
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

void QKeyMapper::showPopupMessage(const QString& message, const QString& color, int displayTime)
{
    if (!m_PopupMessageLabel || !m_PopupMessageAnimation) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[showPopupMessage]" << "PopupMessage not initialized!";
#endif
        return;
    }

    QString styleSheet = QString("background-color: rgba(0, 0, 0, 180); color: white; padding: 15px; border-radius: 5px; color: %1;").arg(color);
    m_PopupMessageLabel->setStyleSheet(styleSheet);
    m_PopupMessageLabel->setWindowFlag(Qt::WindowStaysOnTopHint);

    // QFont customFont;
    // if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
    //     customFont.setFamily(FONTNAME_ENGLISH);
    // }
    // else {
    //     customFont.setFamily(FONTNAME_CHINESE);
    // }
    // customFont.setPointSize(16);
    // customFont.setBold(true);
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
    m_PopupMessageLabel->show();

    m_PopupMessageAnimation->stop();
    m_PopupMessageAnimation->setDuration(displayTime);
    m_PopupMessageAnimation->start(QAbstractAnimation::KeepWhenStopped);

    m_PopupMessageTimer.setSingleShot(true);
    connect(&m_PopupMessageTimer, &QTimer::timeout, m_PopupMessageLabel, &QLabel::hide);
    m_PopupMessageTimer.start(displayTime);
}

void QKeyMapper::showCarOrdinal(qint32 car_ordinal)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[showCarOrdinal]" << "CarOrdinal =" << car_ordinal;
#endif

    QString car_ordinal_str = QString::number(car_ordinal);
    ui->pointDisplayLabel->setText(car_ordinal_str);
}

void QKeyMapper::onKeyMappingTabWidgetTabBarDoubleClicked(int index)
{
    if ((0 <= index)
        && (index < m_KeyMappingTabWidget->count() - 1)
        && (index < s_KeyMappingTabInfoList.size())) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onKeyMappingTabWidgetTabBarDoubleClicked]" << "m_KeyMappingTabWidget TabBar doubleclicked :" << index;
#endif

        showTableSetupDialog(index);
    }
    else if ((index == m_KeyMappingTabWidget->count() - 1)
        && (m_KeyMappingTabWidget->tabText(index) == ADDTAB_TAB_TEXT)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[onKeyMappingTabWidgetTabBarDoubleClicked]" << "m_KeyMappingTabWidget \"+\" tab doubleclicked!";
#endif
        bool addtab_result = addTabToKeyMappingTabWidget();
        Q_UNUSED(addtab_result);
#ifdef DEBUG_LOGOUT_ON
        if (false == addtab_result) {
            qWarning() << "[onKeyMappingTabWidgetTabBarDoubleClicked]" << "addTabToKeyMappingTabWidget failed!";
        }
#endif
    }
}

void QKeyMapper::keyMappingTabWidgetCurrentChanged(int index)
{
    if ((index == m_KeyMappingTabWidget->count() - 1)
        && (m_KeyMappingTabWidget->tabText(index) == ADDTAB_TAB_TEXT)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyMappingTabWidgetCurrentChanged]" << "m_KeyMappingTabWidget \"+\" tab clicked, do nothing.";
#endif
        setKeyMappingTabWidgetCurrentIndex(s_KeyMappingTabWidgetCurrentIndex);
    }
    else if (0 <= index && index < m_KeyMappingTabWidget->count() - 1) {
        disconnectKeyMappingDataTableConnection();
        switchKeyMappingTabIndex(index);
        updateKeyMappingDataTableConnection();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyMappingTabWidgetCurrentChanged]" << "m_KeyMappingTabWidget tab changed :" << index;
#endif
    }
}

void QKeyMapper::keyMappingTableDragDropMove(int from, int to)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[keyMappingTableDragDropMove] DragDrop : Row" << from << "->" << to;
#endif

    int mappingdata_size = KeyMappingDataList->size();
    if (from >= 0 && from < mappingdata_size && to >= 0 && to < mappingdata_size) {
        KeyMappingDataList->move(from, to);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyMappingTableDragDropMove] : refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);

        int reselectrow = to;
        QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        m_KeyMappingDataTable->setRangeSelected(selection, true);

#ifdef DEBUG_LOGOUT_ON
        if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList->size()){
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
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[keyMappingTableItemDoubleClicked]" << "Row" << rowindex << "DoubleClicked";
#endif

    showItemSetupDialog(s_KeyMappingTabWidgetCurrentIndex, rowindex);
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

    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
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
#endif

void QKeyMapper::on_processinfoTable_doubleClicked(const QModelIndex &index)
{
    if ((KEYMAP_IDLE == m_KeyMapStatus)
            && (true == ui->processinfoTable->isEnabled())){
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[SelectProcessInfo]" << "Table DoubleClicked [" << index.row() << "] " << ui->processinfoTable->item(index.row(), 0)->text() << ", " << ui->processinfoTable->item(index.row(), 2)->text();
#endif
        ui->nameLineEdit->setEnabled(true);
        ui->titleLineEdit->setEnabled(true);
        ui->nameCheckBox->setEnabled(true);
        ui->titleCheckBox->setEnabled(true);
        ui->removeSettingButton->setEnabled(true);
        // ui->disableWinKeyCheckBox->setEnabled(true);

        QString filename = ui->processinfoTable->item(index.row(), 0)->text();
        QString windowTitle = ui->processinfoTable->item(index.row(), 2)->text();

        int checksaveindex = checkSaveSettings(filename, windowTitle);
        if (TITLESETTING_INDEX_ANYTITLE < checksaveindex && checksaveindex <= TITLESETTING_INDEX_MAX) {
            QString loadSettingSelectStr = filename + SEPARATOR_TITLESETTING + QString(WINDOWTITLE_STRING) + QString::number(checksaveindex);
            // QString curSettingSelectStr = ui->settingselectComboBox->currentText();
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
            ui->descriptionLineEdit->clear();
        }

        ui->nameLineEdit->setText(filename);
        ui->titleLineEdit->setText(windowTitle);

        QString pidStr = ui->processinfoTable->item(index.row(), PROCESS_PID_COLUMN)->text();
        QString ProcessPath;
        DWORD dwProcessId = pidStr.toULong();

        getProcessInfoFromPID(dwProcessId, ProcessPath);

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
                    for (const QString &key : qAsConst(combinationkeyslist)) {
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
                    for (const QString &key : qAsConst(combinationkeyslist)) {
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
            if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                showWarningPopup("Invalid input format for the original key combination!");
            }
            else {
                showWarningPopup("原始组合键输入格式错误！");
            }
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
                || currentMapKeyText.startsWith(FUNC_PREFIX)
                || currentMapKeyText == MOUSE2VJOY_HOLD_KEY_STR
                || currentMapKeyText == MOUSE2VJOY_DIRECT_KEY_STR
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
                    || keymapdata.Mapping_Keys.contains(FUNC_PREFIX)
                    || keymapdata.Mapping_Keys.contains(MOUSE2VJOY_HOLD_KEY_STR)
                    || keymapdata.Mapping_Keys.contains(MOUSE2VJOY_DIRECT_KEY_STR)
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
                    || currentMapKeyText.startsWith(FUNC_PREFIX)
                    || currentMapKeyText == MOUSE2VJOY_HOLD_KEY_STR
                    || currentMapKeyText == MOUSE2VJOY_DIRECT_KEY_STR
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
                QString message;
                if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                    message = QString("Key sequence mapping to \"%1\" is too long!").arg(currentOriKeyText);
                }
                else {
                    message = QString("映射到\"%1\"的按键序列太长了!").arg(currentOriKeyText);
                }
                showWarningPopup(message);
                return;
            }

            int virtualgamepad_index = ui->virtualGamepadListComboBox->currentIndex();
            if (virtualgamepad_index > 0
                && QKeyMapper_Worker::MultiVirtualGamepadInputList.contains(currentMapKeyText)) {
                currentMapKeyText = QString("%1@%2").arg(currentMapKeyText, QString::number(virtualgamepad_index - 1));
            }
            else if (currentMapKeyText.startsWith(MOUSE_BUTTON_PREFIX) && currentMapKeyText.endsWith(MOUSE_SCREENPOINT_POSTFIX)) {
                QString mousepointstr = ui->pointDisplayLabel->text();
                if (mousepointstr.isEmpty()) {
                    QString message;
                    if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                        message = QString("Need to set a screen mouse point with \"%1\" click!").arg("L-Ctrl+Mouse-Left Click");
                    }
                    else {
                        message = QString("需要使用\"%1\"设置一个屏幕坐标点!").arg("L-Ctrl+鼠标左键点击");
                    }
                    showWarningPopup(message);
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
                            QString message;
                            if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                                message = QString("Already set a same screen mouse point!");
                            }
                            else {
                                message = QString("已经保存了一个相同的屏幕坐标点!");
                            }
                            showWarningPopup(message);
                            return;
                        }
                    }
                }
            }
            else if (currentMapKeyText.startsWith(MOUSE_BUTTON_PREFIX) && currentMapKeyText.endsWith(MOUSE_WINDOWPOINT_POSTFIX)) {
                QString mousepointstr = ui->pointDisplayLabel->text();
                if (mousepointstr.isEmpty()) {
                    QString message;
                    if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                        message = QString("Need to set a window mouse point with \"%1\" click!").arg("L-Alt+Mouse-Left Click");
                    }
                    else {
                        message = QString("需要使用\"%1\"设置一个窗口坐标点!").arg("L-Alt+鼠标左键点击");
                    }
                    showWarningPopup(message);
                    return;
                }
                else {
                    QPoint mousepoint = getMousePointFromLabelString(mousepointstr);
                    int x = mousepoint.x();
                    int y = mousepoint.y();

                    if (x >= 0 && y >= 0) {
                        currentMapKeyText = currentMapKeyText.remove(MOUSE_WINDOWPOINT_POSTFIX) + QString(":W(%1,%2)").arg(x, y);

                        if (keymapdata.Mapping_Keys.size() == 1
                            && keymapdata.Mapping_Keys.constFirst().contains(currentMapKeyText)
                            && !ui->nextarrowCheckBox->isChecked()) {
                            QString message;
                            if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                                message = QString("Already set a same window mouse point!");
                            }
                            else {
                                message = QString("已经保存了一个相同的窗口坐标点!");
                            }
                            showWarningPopup(message);
                            return;
                        }
                    }
                }
            }
            else if (currentMapKeyText == SENDTEXT_STR) {
                QString sendtext = ui->sendTextLineEdit->text();
                if (sendtext.isEmpty()) {
                    QString message;
                    if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                        message = QString("Please input the text to send!");
                    }
                    else {
                        message = QString("请输入要发送的文本!");
                    }
                    showWarningPopup(message);
                    return;
                }
                else {
                    currentMapKeyText = QString("SendText(%1)").arg(sendtext);
                }
            }
            else if (currentMapKeyText == KEYSEQUENCEBREAK_STR) {
                QString message;
                if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                    message = QString("KeySequenceBreak key can not be set duplicated!");
                }
                else {
                    message = QString("按键映射序列打断键不能重复设置!");
                }
                showWarningPopup(message);
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

            KeyMappingDataList->replace(findindex, MAP_KEYDATA(currentOriKeyText,
                                                               mappingkeys_str,
                                                               keymapdata.Note,
                                                               keymapdata.Burst,
                                                               keymapdata.BurstPressTime,
                                                               keymapdata.BurstReleaseTime,
                                                               keymapdata.Lock,
                                                               keymapdata.PassThrough,
                                                               keymapdata.KeyUp_Action,
                                                               keymapdata.KeySeqHoldDown,
                                                               keymapdata.RepeatMode,
                                                               keymapdata.RepeatTimes
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
                int virtualgamepad_index = ui->virtualGamepadListComboBox->currentIndex();
                if (virtualgamepad_index > 0
                    && QKeyMapper_Worker::MultiVirtualGamepadInputList.contains(currentMapKeyText)) {
                    currentMapKeyText = QString("%1@%2").arg(currentMapKeyText, QString::number(virtualgamepad_index - 1));
                }
                else if (currentMapKeyText.startsWith(MOUSE_BUTTON_PREFIX) && currentMapKeyText.endsWith(MOUSE_SCREENPOINT_POSTFIX)) {
                    QString mousepointstr = ui->pointDisplayLabel->text();
                    if (mousepointstr.isEmpty()) {
                        QString message;
                        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                            message = QString("Need to set a screen mouse point with \"%1\" click!").arg("L-Ctrl+Mouse-Left Click");
                        }
                        else {
                            message = QString("需要使用\"%1\"设置一个屏幕坐标点!").arg("L-Ctrl+鼠标左键点击");
                        }
                        showWarningPopup(message);
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
                        QString message;
                        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                            message = QString("Need to set a window mouse point with \"%1\" click!").arg("L-Alt+Mouse-Left Click");
                        }
                        else {
                            message = QString("需要使用\"%1\"设置一个窗口坐标点!").arg("L-Alt+鼠标左键点击");
                        }
                        showWarningPopup(message);
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
                        QString message;
                        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                            message = QString("Please input the text to send!");
                        }
                        else {
                            message = QString("请输入要发送的文本!");
                        }
                        showWarningPopup(message);
                        return;
                    }
                    else {
                        currentMapKeyText = QString("SendText(%1)").arg(sendtext);
                    }
                }
                else if (currentMapKeyText == KEY_BLOCKED_STR) {
                    if (currentOriKeyText.contains(JOY_KEY_PREFIX)) {
                        QString message;
                        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                            message = QString("Game controller keys could not be blocked!");
                        }
                        else {
                            message = QString("游戏手柄按键无法被屏蔽!");
                        }
                        showWarningPopup(message);
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
                    && currentMapKeyComboBoxText.startsWith(FUNC_PREFIX) == false
                    && currentMapKeyComboBoxText != MOUSE2VJOY_HOLD_KEY_STR
                    && currentMapKeyComboBoxText != MOUSE2VJOY_DIRECT_KEY_STR
                    && currentMapKeyComboBoxText != VJOY_LT_BRAKE_STR
                    && currentMapKeyComboBoxText != VJOY_RT_BRAKE_STR
                    && currentMapKeyComboBoxText != VJOY_LT_ACCEL_STR
                    && currentMapKeyComboBoxText != VJOY_RT_ACCEL_STR) {
                    currentMapKeyText = currentMapKeyText + QString(SEPARATOR_WAITTIME) + QString::number(waitTime);
                }
            }

            KeyMappingDataList->append(MAP_KEYDATA(currentOriKeyText,
                                                   currentMapKeyText,
                                                   QString(),
                                                   false,
                                                   BURST_PRESS_TIME_DEFAULT,
                                                   BURST_RELEASE_TIME_DEFAULT,
                                                   false,
                                                   false,
                                                   false,
                                                   false,
                                                   REPEAT_MODE_NONE,
                                                   REPEAT_TIMES_DEFAULT
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
        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
            showWarningPopup("Conflict with exist Keys!");
        }
        else {
            showWarningPopup("与已存在的按键映射冲突！");
        }
    }
}

void QKeyMapper::on_deleteoneButton_clicked()
{
    int currentrowindex = m_KeyMappingDataTable->currentRow();

#ifdef DEBUG_LOGOUT_ON
    qDebug("DeleteOne: currentRow(%d)", currentrowindex);
#endif

    if (currentrowindex >= 0){
        m_KeyMappingDataTable->removeRow(currentrowindex);
        KeyMappingDataList->removeAt(currentrowindex);

        /* do not refresh for select cursor hold position */
//        refreshKeyMappingDataTable();
        updateMousePointsList();
#ifdef DEBUG_LOGOUT_ON
        if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList->size()){
            qDebug("KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList->size());
        }
#endif

    }
}

void QKeyMapper::on_clearallButton_clicked()
{
    int language_index = ui->languageComboBox->currentIndex();
    QString message;
    if (LANGUAGE_ENGLISH == language_index) {
        message = "Are you sure you want to clear all data in the mapping table?";
    }
    else {
        message = "请确认是否要清除映射表中全部数据？";
    }

    QMessageBox::StandardButton reply;
    if (LANGUAGE_ENGLISH == language_index) {
        reply = QMessageBox::warning(this, PROGRAM_NAME, message, QMessageBox::Yes | QMessageBox::No);
    }
    else {
        reply = QMessageBox::warning(this, PROGRAM_NAME, message, QMessageBox::Yes | QMessageBox::No);
    }

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

QPopupNotification::QPopupNotification(QWidget *parent) :
    QWidget(parent),
    m_DisplayTime(3000)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    m_Label = new QLabel(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_Label);
    setLayout(layout);

    // Setup the animation for showing the notification
    m_StartAnimation = new QPropertyAnimation(this, "windowOpacity", this);
    m_StartAnimation->setDuration(500); // 0.5 seconds
    m_StartAnimation->setStartValue(0.0);
    m_StartAnimation->setEndValue(1.0);

    m_StopAnimation = new QPropertyAnimation(this, "windowOpacity", this);
    m_StopAnimation->setDuration(500); // 0.5 seconds
    m_StopAnimation->setStartValue(1.0);
    m_StopAnimation->setEndValue(0.0);
    QObject::connect(m_StopAnimation, &QPropertyAnimation::finished, this, &QWidget::close);

    // Setup the timer for hiding the notification
    QObject::connect(&m_Timer, &QTimer::timeout, this, &QPopupNotification::hideNotification);
    m_Timer.setSingleShot(true);
}

void QPopupNotification::showPopupNotification(const QString &message, const QString &color, int displayTime, int position)
{
    m_StartAnimation->stop();
    m_StopAnimation->stop();
    hide(); // Hide the window before updating content

    m_DisplayTime = displayTime;

    QString styleSheet = QString("background-color: rgba(0, 0, 0, 180); color: white; padding: 15px; border-radius: 5px; color: %1;").arg(color);
    m_Label->setStyleSheet(styleSheet);

    QFont customFont(FONTNAME_ENGLISH, 16, QFont::Bold);
    if (UI_SCALE_4K_PERCENT_150 == QKeyMapper::getInstance()->m_UI_Scale) {
        customFont.setPointSize(16);
    }
    m_Label->setFont(customFont);
    m_Label->setText(message);
    m_Label->adjustSize();

    // Position the notification based on the position parameter
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    qreal scale = QKeyMapper::s_UI_scale_value;
    // Default to top-right
    int x_right_offset = 20;
    if (scale < 1.10) {
        x_right_offset = 30;
    }
    int x = screenGeometry.width() - m_Label->width() - x_right_offset;
    int y = 10;
    if (position == NOTIFICATION_POSITION_TOP_LEFT) {
        x = 10; // 10 pixels from the left edge
        y = 10; // 10 pixels from the top edge
    }
    else if (position == NOTIFICATION_POSITION_TOP_CENTER) {
        x = (screenGeometry.width() - m_Label->width()) / 2; // Centered horizontally
        y = 10; // 10 pixels from the top edge
    }
    else if (position == NOTIFICATION_POSITION_TOP_RIGHT) {
        x = screenGeometry.width() - m_Label->width() - x_right_offset; // 30 pixels from the right edge
        y = 10; // 10 pixels from the top edge
    }
    else if (position == NOTIFICATION_POSITION_BOTTOM_LEFT) {
        x = 10; // 10 pixels from the left edge
        y = screenGeometry.height() - m_Label->height() - 30; // 30 pixels from the bottom edge
    }
    else if (position == NOTIFICATION_POSITION_BOTTOM_CENTER) {
        x = (screenGeometry.width() - m_Label->width()) / 2; // Centered horizontally
        y = screenGeometry.height() - m_Label->height() - 30; // 30 pixels from the bottom edge
    }
    else if (position == NOTIFICATION_POSITION_BOTTOM_RIGHT) {
        x = screenGeometry.width() - m_Label->width() - x_right_offset; // 30 pixels from the right edge
        y = screenGeometry.height() - m_Label->height() - 30; // 30 pixels from the bottom edge
    }
    move(x, y);

    // Adjust the size of the window to fit the new content
    adjustSize();

    // Show the notification and start the animation
    show();
    m_StartAnimation->start(QAbstractAnimation::KeepWhenStopped);

    // Start the timer for hiding the notification
    m_Timer.start(m_DisplayTime);
}

void QPopupNotification::hideNotification()
{
    m_StopAnimation->stop();
    m_StopAnimation->start(QAbstractAnimation::KeepWhenStopped);
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

void QKeyMapper::on_moveupButton_clicked()
{
    int currentrowindex = -1;
    QList<QTableWidgetItem*> items = m_KeyMappingDataTable->selectedItems();
    if (items.size() > 0) {
        QTableWidgetItem* selectedItem = items.at(0);
        currentrowindex = m_KeyMappingDataTable->row(selectedItem);
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
        KeyMappingDataList->move(currentrowindex, currentrowindex-1);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);

        int reselectrow = currentrowindex - 1;
        QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        m_KeyMappingDataTable->setRangeSelected(selection, true);

#ifdef DEBUG_LOGOUT_ON
        if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList->size()){
            qDebug("MoveUp:KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList->size());
        }
#endif
    }
}

void QKeyMapper::on_movedownButton_clicked()
{
    int currentrowindex = -1;
    QList<QTableWidgetItem*> items = m_KeyMappingDataTable->selectedItems();
    if (items.size() > 0) {
        QTableWidgetItem* selectedItem = items.at(0);
        currentrowindex = m_KeyMappingDataTable->row(selectedItem);
#ifdef DEBUG_LOGOUT_ON
        if (currentrowindex >= 0
            && currentrowindex < (m_KeyMappingDataTable->rowCount()-1)){
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
        && currentrowindex < (m_KeyMappingDataTable->rowCount()-1)){
#ifdef DEBUG_LOGOUT_ON
        qDebug("MoveDown: currentRow(%d)", currentrowindex);
#endif
        KeyMappingDataList->move(currentrowindex, currentrowindex+1);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable(m_KeyMappingDataTable, KeyMappingDataList);

        int reselectrow = currentrowindex + 1;
        QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        m_KeyMappingDataTable->setRangeSelected(selection, true);

#ifdef DEBUG_LOGOUT_ON
        if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList->size()){
            qDebug("MoveDown:KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList->size());
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
        bool loadresult = loadKeyMapSetting(curSettingSelectStr);
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
            bool loadresult = loadKeyMapSetting(curSettingSelectStr);
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
            for (const QString &gamepad_type : qAsConst(QKeyMapper_Worker::s_VirtualGamepadList)){
                PVIGEM_TARGET added_target = QKeyMapper_Worker::ViGEmClient_AddTarget_byType(gamepad_type);
                if (added_target != Q_NULLPTR) {
                    QKeyMapper_Worker::s_ViGEmTarget_ReportList.append(XUSB_REPORT());
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
        ui->lockCursorCheckBox->setEnabled(true);
        ui->virtualGamepadNumberSpinBox->setEnabled(true);
        ui->virtualGamepadListComboBox->setEnabled(true);

        settingFile.setValue(VIRTUALGAMEPAD_ENABLE , true);
    }
    else {
        ui->vJoyXSensSpinBox->setEnabled(false);
        ui->vJoyYSensSpinBox->setEnabled(false);
        ui->vJoyXSensLabel->setEnabled(false);
        ui->vJoyYSensLabel->setEnabled(false);
        ui->lockCursorCheckBox->setEnabled(false);
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

void QKeyMapper::on_installInterceptionButton_clicked()
{
    Interception_Worker::Interception_State currentInterceptionState = Interception_Worker::getInterceptionState();
    int languageIndex = ui->languageComboBox->currentIndex();

    if (Interception_Worker::INTERCEPTION_AVAILABLE == currentInterceptionState) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_installViGEmBusButton_clicked]" << "Uninstall Interception Driver, InterceptionState ->" << currentInterceptionState;
#endif

        Interception_Worker::getInstance()->doUnloadInterception();

        (void)uninstallInterceptionDriver();
        Interception_Worker::setRebootRequiredFlag();

        emit updateMultiInputStatus_Signal();

        Interception_Worker::Interception_State newInterceptionState = Interception_Worker::getInterceptionState();

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_installViGEmBusButton_clicked]" << "Uninstall Interception Driver, New InterceptionState ->" << newInterceptionState;
#endif

        if (Interception_Worker::INTERCEPTION_REBOOTREQUIRED == newInterceptionState) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[on_installViGEmBusButton_clicked]" << "Reboot required after uninstall Interception Driver, InterceptionState ->" << newInterceptionState;
#endif
            /* Show Reboot Required MessageBox after Uninstall Interception Driver */
            if (LANGUAGE_ENGLISH == languageIndex) {
                QMessageBox::warning(this, PROGRAM_NAME, "System reboot is required for the changes to take effect after uninstalling the multi-input device driver.");
            }
            else {
                QMessageBox::warning(this, PROGRAM_NAME, "卸载多输入设备驱动后需要重新启动操作系统生效。");
            }
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_installViGEmBusButton_clicked]" << "Install Interception Driver, Current InterceptionState ->" << currentInterceptionState;
#endif
        (void)installInterceptionDriver();

        int loop = 0;
        for (loop = 0; loop < INSTALL_INTERCEPTION_LOOP_WAIT_TIME_MAX; loop++) {
            if (Interception_Worker::isInterceptionDriverFileExist()) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[on_installViGEmBusButton_clicked]" << "Install Interception Driver, InterceptionDriverFileExist() wait time ->" << loop * INSTALL_INTERCEPTION_LOOP_WAIT_TIME;
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
        qDebug() << "[on_installViGEmBusButton_clicked]" << "Install Interception Driver, New InterceptionState ->" << newInterceptionState;
#endif

        if (Interception_Worker::INTERCEPTION_REBOOTREQUIRED == newInterceptionState) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[on_installViGEmBusButton_clicked]" << "Reboot required after install Interception Driver, InterceptionState ->" << newInterceptionState;
#endif
            /* Show Reboot Required MessageBox after Install Interception Driver */
            if (LANGUAGE_ENGLISH == languageIndex) {
                QMessageBox::warning(this, PROGRAM_NAME, "System reboot is required for the changes to take effect after installing the multi-input device driver.");
            }
            else {
                QMessageBox::warning(this, PROGRAM_NAME, "安装多输入设备驱动后需要重新启动操作系统生效。");
            }
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
            QKeyMapper_Worker::s_ViGEmTarget_ReportList.append(XUSB_REPORT());
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
        m_KeyMappingDataTable->setFocus();
        m_KeyMappingDataTable->clearFocus();
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

void KeyMappingDataTableWidget::startDrag(Qt::DropActions supportedActions)
{
    m_DraggedRow = currentRow();
    QTableWidget::startDrag(supportedActions);
}

void KeyMappingDataTableWidget::dropEvent(QDropEvent *event)
{
    if (event->dropAction() == Qt::MoveAction) {
        // blockSignals(true);

        int droppedRow = rowAt(event->pos().y());
// #ifdef DEBUG_LOGOUT_ON
//         qDebug() << "[KeyMappingDataTableWidget::dropEvent] MoveAction : Row" << m_DraggedRow << "->" << droppedRow;
// #endif
        emit QKeyMapper::getInstance()->keyMappingTableDragDropMove_Signal(m_DraggedRow, droppedRow);

        // blockSignals(false);
    }
}

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
