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
QList<MAP_KEYDATA> QKeyMapper::KeyMappingDataList = QList<MAP_KEYDATA>();
QList<MAP_KEYDATA> QKeyMapper::KeyMappingDataListGlobal = QList<MAP_KEYDATA>();
QList<MousePoint_Info> QKeyMapper::ScreenMousePointsList = QList<MousePoint_Info>();
QList<MousePoint_Info> QKeyMapper::WindowMousePointsList = QList<MousePoint_Info>();
// QHash<QString, QHotkey*> QKeyMapper::ShortcutsMap = QHash<QString, QHotkey*>();
QString QKeyMapper::s_WindowSwitchKeyString = DISPLAYSWITCH_KEY_DEFAULT;
QString QKeyMapper::s_MappingSwitchKeyString = MAPPINGSWITCH_KEY_DEFAULT;

QKeyMapper::QKeyMapper(QWidget *parent) :
    QDialog(parent),
    m_UI_Scale(UI_SCALE_NORMAL),
    ui(new Ui::QKeyMapper),
    m_KeyMapStatus(KEYMAP_IDLE),
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
    m_KeyMappingDataTable(Q_NULLPTR),
    m_ProcessInfoTableDelegate(Q_NULLPTR),
    // m_KeyMappingDataTableDelegate(Q_NULLPTR),
    m_orikeyComboBox(new KeyListComboBox(this)),
    m_mapkeyComboBox(new KeyListComboBox(this)),
    // m_windowswitchKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    // m_mappingswitchKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    // m_originalKeySeqEdit(new KeySequenceEditOnlyOne(this)),
    // m_HotKey_ShowHide(new QHotkey(this)),
    // m_HotKey_StartStop(new QHotkey(this)),
    loadSetting_flag(false),
    m_TransParentHandle(NULL),
    m_TransParentWindowInitialX(0),
    m_TransParentWindowInitialY(0),
    m_TransParentWindowInitialWidth(0),
    m_TransParentWindowInitialHeight(0),
    m_deviceListWindow(Q_NULLPTR),
    m_ItemSetupDialog(Q_NULLPTR)
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

    // m_windowswitchKeySeqEdit->setDefaultKeySequence(DISPLAYSWITCH_KEY_DEFAULT);
    // m_mappingswitchKeySeqEdit->setDefaultKeySequence(MAPPINGSWITCH_KEY_DEFAULT);
    // m_originalKeySeqEdit->setDefaultKeySequence(ORIGINAL_KEYSEQ_DEFAULT);
    ui->windowswitchkeyLineEdit->setText(DISPLAYSWITCH_KEY_DEFAULT);
    ui->mappingswitchkeyLineEdit->setText(MAPPINGSWITCH_KEY_DEFAULT);
    initKeyMappingDataTable();
    loadSetting_flag = true;
    bool loadresult = loadKeyMapSetting(QString());
    Q_UNUSED(loadresult);
    loadSetting_flag = false;

    Interception_Worker::syncDisabledKeyboardList();
    Interception_Worker::syncDisabledMouseList();

    m_deviceListWindow = new QInputDeviceListWindow(this);
    m_ItemSetupDialog = new QItemSetupDialog(this);
    m_ItemSetupDialog->setWindowFlags(Qt::Popup);

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
    QObject::connect(m_KeyMappingDataTable, &QTableWidget::cellChanged, this, &QKeyMapper::cellChanged_slot);
    QObject::connect(m_KeyMappingDataTable, &QTableWidget::itemSelectionChanged, this, &QKeyMapper::on_keymapdataTable_itemSelectionChanged);
    QObject::connect(this, &QKeyMapper::keyMappingTableDragDropMove_Signal, this, &QKeyMapper::keyMappingTableDragDropMove);
    QObject::connect(m_KeyMappingDataTable, &QTableWidget::itemDoubleClicked, this, &QKeyMapper::keyMappingTableItemDoubleClicked);
    // QObject::connect(m_KeyMappingDataTable, &QTableWidget::cellDoubleClicked, this, &QKeyMapper::keyMappingTableCellDoubleClicked);
    QObject::connect(this, &QKeyMapper::setupDialogClosed_Signal, this, &QKeyMapper::setupDialogClosed);
    QObject::connect(this, &QKeyMapper::showPopupMessage_Signal, this, &QKeyMapper::showPopupMessage);

    // QObject::connect(m_windowswitchKeySeqEdit, &KeySequenceEditOnlyOne::keySeqEditChanged_Signal, this, &QKeyMapper::onWindowSwitchKeySequenceChanged);
    // QObject::connect(m_windowswitchKeySeqEdit, &KeySequenceEditOnlyOne::editingFinished, this, &QKeyMapper::onWindowSwitchKeySequenceEditingFinished);
    // QObject::connect(m_mappingswitchKeySeqEdit, &KeySequenceEditOnlyOne::keySeqEditChanged_Signal, this, &QKeyMapper::onMappingSwitchKeySequenceChanged);
    // QObject::connect(m_mappingswitchKeySeqEdit, &KeySequenceEditOnlyOne::editingFinished, this, &QKeyMapper::onMappingSwitchKeySequenceEditingFinished);
    // QObject::connect(m_originalKeySeqEdit, &KeySequenceEditOnlyOne::keySeqEditChanged_Signal, this, &QKeyMapper::onOriginalKeySequenceChanged);
    // QObject::connect(m_originalKeySeqEdit, &KeySequenceEditOnlyOne::editingFinished, this, &QKeyMapper::onOriginalKeySequenceEditingFinished);

    QObject::connect(this, &QKeyMapper::HotKeyDisplaySwitchActivated_Signal, this, &QKeyMapper::HotKeyDisplaySwitchActivated, Qt::QueuedConnection);
    QObject::connect(this, &QKeyMapper::HotKeyMappingSwitchActivated_Signal, this, &QKeyMapper::HotKeyMappingSwitchActivated, Qt::QueuedConnection);

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

    //m_CycleCheckTimer.start(CYCLE_CHECK_TIMEOUT);
    refreshProcessInfoTable();
    resizeKeyMappingDataTableColumnWidth();
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

    destoryTransparentWindow(m_TransParentHandle);
    m_TransParentHandle = NULL;
    // freeShortcuts();

    delete m_orikeyComboBox;
    m_orikeyComboBox = Q_NULLPTR;
    delete m_mapkeyComboBox;
    m_mapkeyComboBox = Q_NULLPTR;

    // delete m_windowswitchKeySeqEdit;
    // m_windowswitchKeySeqEdit = Q_NULLPTR;

    // delete m_mappingswitchKeySeqEdit;
    // m_mappingswitchKeySeqEdit = Q_NULLPTR;

    // delete m_originalKeySeqEdit;
    // m_originalKeySeqEdit = Q_NULLPTR;

    // delete m_HotKey_ShowHide;
    // m_HotKey_ShowHide = Q_NULLPTR;
    // delete m_HotKey_StartStop;
    // m_HotKey_StartStop = Q_NULLPTR;

    delete ui;
    delete m_SysTrayIcon;
    m_SysTrayIcon = Q_NULLPTR;

    delete m_ProcessInfoTableDelegate;
    m_ProcessInfoTableDelegate = Q_NULLPTR;

    // delete m_KeyMappingDataTableDelegate;
    // m_KeyMappingDataTableDelegate = Q_NULLPTR;

    if (m_deviceListWindow != Q_NULLPTR) {
        delete m_deviceListWindow;
        m_deviceListWindow = Q_NULLPTR;
    }

    if (m_ItemSetupDialog != Q_NULLPTR) {
        delete m_ItemSetupDialog;
        m_ItemSetupDialog = Q_NULLPTR;
    }
}

void QKeyMapper::WindowStateChangedProc(void)
{
    if (true == isMinimized()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[WindowStateChangedProc]" << "QKeyMapper::WindowStateChangedProc() -> Window Minimized: setHidden!";
#endif
        // closeItemSetupDialog();
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
                        setKeyHook(NULL);
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
                    if (checkresult > 1) {
                        setKeyHook(NULL);
                    }
                    else {
                        setKeyHook(hwnd);
                    }
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
    updateHWNDListProc();

    if (false == isHidden()){
        refreshProcessInfoTable();
        initInputDeviceSelectComboBoxes();
    }
}

void QKeyMapper::updateHWNDListProc()
{
    s_hWndList.clear();
    if (!m_MapProcessInfo.WindowTitle.isEmpty()) {
        EnumWindows((WNDENUMPROC)QKeyMapper::EnumWindowsBgProc, 0);
    }

    s_last_HWNDList = s_hWndList;

#ifdef DEBUG_LOGOUT_ON
    if (!s_hWndList.isEmpty()) {
        qDebug().nospace() << "[updateHWNDListProc] " << m_MapProcessInfo.WindowTitle << " lastHWNDList[" << s_last_HWNDList.size() << "] -> " << s_last_HWNDList;
    }
    if (s_CurrentMappingHWND != NULL) {
        qDebug().nospace() << "[updateHWNDListProc] " << "Title=" << m_MapProcessInfo.WindowTitle << ", Process=" << m_MapProcessInfo.FileName << " -> " << s_CurrentMappingHWND;
    }
#endif
}

void QKeyMapper::setKeyHook(HWND hWnd)
{
    // updateShortcutsMap();

    s_CurrentMappingHWND = hWnd;
    emit QKeyMapper_Worker::getInstance()->setKeyHook_Signal(hWnd);
}

void QKeyMapper::setKeyUnHook(void)
{
    // freeShortcuts();

    s_CurrentMappingHWND = NULL;
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

    QString WindowText;
    TCHAR titleBuffer[MAX_PATH] = TEXT("");
    memset(titleBuffer, 0x00, sizeof(titleBuffer));

    int resultLength = GetWindowText(hWnd, titleBuffer, MAX_PATH);
    if (resultLength){
        WindowText = QString::fromWCharArray(titleBuffer);
        collectWindowsHWND(WindowText, hWnd);

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
                if (s_CurrentMappingHWND == NULL) {
                    s_CurrentMappingHWND = hWnd;
                }
            }
        }
    }

    return TRUE;
}

void QKeyMapper::collectWindowsHWND(const QString &titlestring, HWND hWnd)
{
    QString processTitle = QKeyMapper::getInstance()->m_MapProcessInfo.WindowTitle;
    if (!processTitle.isEmpty()) {
        if (titlestring.contains(processTitle)) {
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

    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
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

    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
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

int QKeyMapper::findOriKeyInKeyMappingDataList_ForDoublePress(const QString &keyname)
{
    int returnindex = -1;
    int keymapdataindex = 0;
    QString keyname_doublepress = keyname + QString(SEPARATOR_DOUBLEPRESS);

    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
    {
        if (keymapdata.Original_Key.startsWith(keyname_doublepress)){
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

ValidationResult QKeyMapper::validateOriginalKeyString(const QString &originalkeystr, int update_rowindex)
{
    ValidationResult result;
    result.isValid = true;

    QStringList orikeylist = originalkeystr.split(SEPARATOR_PLUS);
    if (orikeylist.isEmpty()) {
        result.isValid = false;
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = "OriginalKey is empty.";
        }
        else {
            result.errorMessage = "原始按键为空";
        }
        return result;
    }

    /* Check for duplicate Original keys */
    int numRemoved = orikeylist.removeDuplicates();
    if (numRemoved > 0) {
        result.isValid = false;
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = "OriginalKey contains duplicate keys.";
        }
        else {
            result.errorMessage = "原始按键中存在重复按键";
        }
    }
    else {
        if (orikeylist.size() > 1) {
            QString foundSpecialKey;
            /* Check orikeylist contains keystring in QKeyMapper_Worker::SpecialOriginalKeysList */
            for (const QString& orikey : orikeylist) {
                if (QKeyMapper_Worker::SpecialOriginalKeysList.contains(orikey)) {
                    foundSpecialKey = orikey;
                    break;
                }
            }
            if (!foundSpecialKey.isEmpty()) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Oricombinationkey contains specialkey \"%1\"").arg(foundSpecialKey);
                } else {
                    result.errorMessage = QString("原始组合键包含特殊按键 \"%1\"").arg(foundSpecialKey);
                }

                return result;
            }

            for (const QString& orikey : orikeylist)
            {
                result = validateSingleKeyInOriginalCombinationKey(orikey);
                if (result.isValid == false) {
                    break;
                }
            }

            if (result.isValid) {
                QString original_key = QString(PREFIX_SHORTCUT) + originalkeystr;
                int findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(original_key);

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

            result = validateSingleOriginalKey(orikey, update_rowindex);
        }
    }

    return result;
}

ValidationResult QKeyMapper::validateSingleOriginalKey(const QString &orikey, int update_rowindex)
{
    ValidationResult result;
    result.isValid = true;

    static QRegularExpression longpress_regex("^(.+)⏲(\\d{1,4})$");
    static QRegularExpression doublepress_regex("^(.+)✖(\\d{1,4})$");
    static QRegularExpression multiinput_regex("^(.+)@[0-9]$");

    QRegularExpressionMatch longpress_match = longpress_regex.match(orikey);
    QRegularExpressionMatch doublepress_match = doublepress_regex.match(orikey);
    QRegularExpressionMatch multiinput_match = multiinput_regex.match(orikey);

    if (longpress_match.hasMatch()) {
        int findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(orikey);
        QString original_key = longpress_match.captured(1);
        QString longPressTimeString = longpress_match.captured(2);
        bool ok;
        int longpresstime = longPressTimeString.toInt(&ok);

        QRegularExpressionMatch longpress_multiinput_match = multiinput_regex.match(original_key);
        if (longpress_multiinput_match.hasMatch()) {
            original_key = longpress_multiinput_match.captured(1);
        }

        if (!QItemSetupDialog::s_valiedOriginalKeyList.contains(original_key)) {
            result.isValid = false;
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("Invalid orilongpresskey \"%1\"").arg(original_key);
            } else {
                result.errorMessage = QString("无效的原始长按按键 \"%1\"").arg(original_key);
            }
        }
        else if (!ok
                || longPressTimeString == "0"
                || longPressTimeString.startsWith('0')
                || longpresstime <= PRESSTIME_MIN
                || longpresstime > PRESSTIME_MAX) {
            result.isValid = false;
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("Invalid longpresstime \"%1\"").arg(longPressTimeString);
            } else {
                result.errorMessage = QString("无效的长按时间 \"%1\"").arg(longPressTimeString);
            }
        }
        else if (findindex != -1 && findindex != update_rowindex) {
            result.isValid = false;
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("Duplicate orilongpresskey \"%1\"").arg(orikey);
            } else {
                result.errorMessage = QString("已存在相同的原始长按按键 \"%1\"").arg(orikey);
            }
        }
    }
    else if (doublepress_match.hasMatch()) {
        QHash<QString, int> currentDoublePressOriginalKeysMap = QKeyMapper_Worker::currentDoublePressOriginalKeysMap();
        QString original_key = doublepress_match.captured(1);
        QString original_key_doublepress = original_key + "✖";
        QString doublePressTimeString = doublepress_match.captured(2);
        bool ok;
        int doublepresstime = doublePressTimeString.toInt(&ok);

        QRegularExpressionMatch doublepress_multiinput_match = multiinput_regex.match(original_key);
        if (doublepress_multiinput_match.hasMatch()) {
            original_key = doublepress_multiinput_match.captured(1);
        }

        if (!QItemSetupDialog::s_valiedOriginalKeyList.contains(original_key)) {
            result.isValid = false;
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("Invalid oridoublepresskey \"%1\"").arg(original_key);
            } else {
                result.errorMessage = QString("无效的原始双击按键 \"%1\"").arg(original_key);
            }
        }
        else if (!ok
                || doublePressTimeString == "0"
                || doublePressTimeString.startsWith('0')
                || doublepresstime <= PRESSTIME_MIN
                || doublepresstime > PRESSTIME_MAX) {
            result.isValid = false;
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("Invalid doublepresstime \"%1\"").arg(doublepresstime);
            } else {
                result.errorMessage = QString("无效的双击时间 \"%1\"").arg(doublepresstime);
            }
        }
        else if (currentDoublePressOriginalKeysMap.contains(original_key_doublepress)) {
            int update_doublepress_orikey_rowindex = currentDoublePressOriginalKeysMap.value(original_key_doublepress);
            if (update_doublepress_orikey_rowindex != update_rowindex) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Duplicate oridoublepresskey \"%1\"").arg(original_key_doublepress);
                }
                else {
                    result.errorMessage = QString("已存在相同的原始双击按键 \"%1\"").arg(original_key_doublepress);
                }
            }
        }
    }
    else {
        QString original_key = orikey;
        if (multiinput_match.hasMatch()) {
            original_key = multiinput_match.captured(1);
        }

        if (!QItemSetupDialog::s_valiedOriginalKeyList.contains(original_key)) {
            result.isValid = false;
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("Invalid key \"%1\"").arg(orikey);
            }
            else {
                result.errorMessage = QString("无效按键 \"%1\"").arg(orikey);
            }
        }
        else {
            int findindex = findOriKeyInKeyMappingDataList_ForAddMappingData(orikey);

            if (findindex != -1 && findindex != update_rowindex) {
                result.isValid = false;
                if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                    result.errorMessage = QString("Duplicate original key \"%1\"").arg(orikey);
                } else {
                    result.errorMessage = QString("已存在相同的原始按键 \"%1\"").arg(orikey);
                }
            }
        }
    }

    return result;
}

ValidationResult QKeyMapper::validateSingleKeyInOriginalCombinationKey(const QString &orikey)
{
    ValidationResult result;
    result.isValid = true;

    static QRegularExpression multiinput_regex("^(.+)@[0-9]$");
    QRegularExpressionMatch multiinput_match = multiinput_regex.match(orikey);

    QString original_key = orikey;
    if (multiinput_match.hasMatch()) {
        original_key = multiinput_match.captured(1);
    }

    if (!QItemSetupDialog::s_valiedOriginalKeyList.contains(original_key)) {
        result.isValid = false;
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            result.errorMessage = QString("Invalid key \"%1\"").arg(orikey);
        }
        else {
            result.errorMessage = QString("无效按键 \"%1\"").arg(orikey);
        }
    }

    return result;
}

ValidationResult QKeyMapper::validateMappingKeyString(const QString &mappingkeystr, const QStringList &mappingkeyseqlist)
{
    ValidationResult result;
    result.isValid = true;

    static QRegularExpression regexp("[+»]");
    QStringList Mapping_Keys = mappingkeystr.split(regexp);

    if (Mapping_Keys.isEmpty()) {
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

    if (Mapping_Keys.size() > 1) {
        QString foundSpecialKey;
        /* Check Mapping_Keys contains keystring in QKeyMapper_Worker::SpecialMappingKeysList */
        for (const QString& mapkey : Mapping_Keys) {
            if (QKeyMapper_Worker::SpecialMappingKeysList.contains(mapkey)) {
                foundSpecialKey = mapkey;
                break;
            }
        }
        if (!foundSpecialKey.isEmpty()) {
            result.isValid = false;
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                result.errorMessage = QString("MappingKeys contains specialkey \"%1\"").arg(foundSpecialKey);
            } else {
                result.errorMessage = QString("映射按键中包含特殊按键 \"%1\"").arg(foundSpecialKey);
            }

            return result;
        }

        for (const QString& mapkey : Mapping_Keys)
        {
            result = validateSingleMappingKey(mapkey);
            if (result.isValid == false) {
                break;
            }
        }
    }
    else {
        const QString mapkey = Mapping_Keys.constFirst();

        result = validateSingleMappingKey(mapkey);
    }

    return result;
}

ValidationResult QKeyMapper::validateSingleMappingKey(const QString &mapkey)
{
    ValidationResult result;
    result.isValid = true;

    static QRegularExpression mapkey_regex("^(.+)⏱(\\d+)$");

    QRegularExpressionMatch mapkey_match = mapkey_regex.match(mapkey);
    if (mapkey_match.hasMatch()) {
        QString mapping_key = mapkey_match.captured(1);
        QString waitTimeString = mapkey_match.captured(2);
        bool ok;
        int waittime = waitTimeString.toInt(&ok);

        if (!QItemSetupDialog::s_valiedMappingKeyList.contains(mapping_key)) {
            static QRegularExpression vjoy_regex("^(vJoy-.+)@([0-3])$");
            static QRegularExpression screenpoint_regex("^Mouse-(L|R|M|X1|X2)\\((\\d+),(\\d+)\\)$");
            QRegularExpressionMatch vjoy_match = vjoy_regex.match(mapping_key);
            QRegularExpressionMatch screenpoint_match = screenpoint_regex.match(mapping_key);

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
            else if (screenpoint_match.hasMatch()) {
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

        if (result.isValid) {
            if (!ok
                || waitTimeString == "0"
                || waitTimeString.startsWith('0')
                || waittime <= MAPPING_WAITTIME_MIN
                || waittime > MAPPING_WAITTIME_MAX) {
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
        QString mapping_key = mapkey;
        if (!QItemSetupDialog::s_valiedMappingKeyList.contains(mapping_key)) {
            static QRegularExpression vjoy_regex("^(vJoy-.+)@([0-3])$");
            static QRegularExpression screenpoint_regex("^Mouse-(L|R|M|X1|X2)\\((\\d+),(\\d+)\\)$");
            QRegularExpressionMatch vjoy_match = vjoy_regex.match(mapping_key);
            QRegularExpressionMatch screenpoint_match = screenpoint_regex.match(mapping_key);

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
            else if (screenpoint_match.hasMatch()) {
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
    }

    return result;
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

#if 0
bool QKeyMapper::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_DISPLAYCHANGE) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::nativeEvent]" << "WM_DISPLAYCHANGE";
#endif
            // updateQtDisplayEnvironment();
            // const QObjectList& child_list = children();
            // for (QObject* child : child_list)
            // {
            //     QWidget* w = dynamic_cast<QWidget*>(child);
            //     if (w)
            //         w->repaint();
            // }
        }
        else if (msg->message == WM_DPICHANGED) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::nativeEvent]" << "WM_DPICHANGED";
#endif
        }
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif

void QKeyMapper::showEvent(QShowEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QKeyMapper::showEvent]" << "QKeyMapper show ->" << event->spontaneous();
#endif

    if (false == event->spontaneous()) {
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
    if(event->type()==QEvent::WindowStateChange)
    {
        QTimer::singleShot(0, this, SLOT(WindowStateChangedProc()));
    }
    QDialog::changeEvent(event);
}

void QKeyMapper::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == KEY_REFRESH) {
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
    else if (event->key() == KEY_PASSTHROUGH) {
        if (m_KeyMapStatus == KEYMAP_IDLE){
            int currentrowindex = -1;
            QList<QTableWidgetItem*> items = m_KeyMappingDataTable->selectedItems();
            if (items.size() > 0) {
                QTableWidgetItem* selectedItem = items.at(0);
                currentrowindex = m_KeyMappingDataTable->row(selectedItem);

                if (KeyMappingDataList.at(currentrowindex).PassThrough) {
                    KeyMappingDataList[currentrowindex].PassThrough = false;
                }
                else {
                    KeyMappingDataList[currentrowindex].PassThrough = true;
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug().noquote().nospace() << "[PassThroughStatus]" << "F12 Key Pressed -> Selected mappingdata original_key[" << KeyMappingDataList.at(currentrowindex).Original_Key << "] PassThrough = " << KeyMappingDataList[currentrowindex].PassThrough;
#endif
                refreshKeyMappingDataTable();
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

                if (KeyMappingDataList.at(currentrowindex).KeyUp_Action) {
                    KeyMappingDataList[currentrowindex].KeyUp_Action = false;
                }
                else {
                    KeyMappingDataList[currentrowindex].KeyUp_Action = true;
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug().noquote().nospace() << "[KeyUp_ActionStatus]" << "F2 Key Pressed -> Selected mappingdata original_key[" << KeyMappingDataList.at(currentrowindex).Original_Key << "] KeyUp_Action = " << KeyMappingDataList[currentrowindex].KeyUp_Action;
#endif
                refreshKeyMappingDataTable();
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
    else if (event->key() == Qt::Key_Escape) {
       return;
    }

    QDialog::keyPressEvent(event);
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
        s_CycleCheckLoopCount = CYCLE_CHECK_LOOPCOUNT_RESET;
        emit updateLockStatus_Signal();

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[MappingStart]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") -> " << mappingstartmodeEnum.valueToKey(startmode);
#endif
    }

    if (m_KeyMapStatus != KEYMAP_IDLE){
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
#endif

static void BringWindowToTopEx(HWND hwnd)
{
    AttachThreadInput(GetWindowThreadProcessId(GetForegroundWindow(), NULL), GetCurrentThreadId(), true) ;
    SetForegroundWindow(hwnd) ;
    SetFocus(hwnd) ;
    AttachThreadInput(GetWindowThreadProcessId(GetForegroundWindow(), NULL), GetCurrentThreadId(), false) ;
}

static BOOL CALLBACK focusChildProcWindow(HWND hwnd, LPARAM lParam)
{
    DWORD processId = static_cast<DWORD>(lParam);
    DWORD windowProcessId = NULL;
    GetWindowThreadProcessId(hwnd, &windowProcessId);
    if(windowProcessId == processId)
    {
        BringWindowToTopEx(hwnd);
        return FALSE;
    }

    return TRUE;
}

void QKeyMapper::HotKeyDisplaySwitchActivated(const QString &hotkey_string)
{
    if (m_deviceListWindow->isVisible()) {
        return;
    }

    QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
    Q_UNUSED(hotkey_string);
    Q_UNUSED(keymapstatusEnum);
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[HotKeyDisplaySwitchActivated] DisplaySwitchKey[" << hotkey_string << "] Activated, KeyMapStatus = " << keymapstatusEnum.valueToKey(m_KeyMapStatus);
#endif

    if (false == isHidden()){
        m_LastWindowPosition = pos(); // Save the current position before hiding
        // closeItemSetupDialog();
        hide();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[HotKeyDisplaySwitchActivated] Hide Window, LastWindowPosition ->" << m_LastWindowPosition;
#endif
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[HotKeyDisplaySwitchActivated] Show Window, LastWindowPosition ->" << m_LastWindowPosition;
#endif
        if (m_LastWindowPosition.x() != INITIAL_WINDOW_POSITION && m_LastWindowPosition.y() != INITIAL_WINDOW_POSITION) {
            move(m_LastWindowPosition); // Restore the position before showing
        }

        /* Remove BringWindowToTopEx() because of it will cause other program registered shortcut be invalid. >>> */
#if 0
#ifdef QT_NO_DEBUG
        DWORD pid = getpid();
        EnumWindows(focusChildProcWindow, static_cast<LPARAM>(pid));
#else
        if (!IsDebuggerPresent()) {
            DWORD pid = getpid();
            EnumWindows(focusChildProcWindow, static_cast<LPARAM>(pid));
        }
#endif
#endif
        /* Remove BringWindowToTopEx() because of it will cause other program registered shortcut be invalid. <<< */
        showNormal();
        activateWindow();
        raise();
    }
}

void QKeyMapper::HotKeyMappingSwitchActivated(const QString &hotkey_string)
{
    if (m_deviceListWindow->isVisible()) {
        return;
    }

    QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
    Q_UNUSED(hotkey_string);
    Q_UNUSED(keymapstatusEnum);
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[HotKeyMappingSwitchActivated] MappingSwitchKey[" << hotkey_string << "] Activated, KeyMapStatus = " << keymapstatusEnum.valueToKey(m_KeyMapStatus);
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
            if (hotkey_string.contains("Alt")) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[HotKeyMappingSwitchActivated] AltModifier & ForegroundWindow Process -> " << filename << ", isVisibleWindow = " << isVisibleWindow << ", isExToolWindow = " << isExToolWindow;
#endif
                const Qt::KeyboardModifiers modifiers_arg = Qt::AltModifier;
                QKeyMapper_Worker::getInstance()->releaseKeyboardModifiers(modifiers_arg);
            }
        }
    }
    /* Add for "explorer.exe" AltModifier Bug Fix <<< */
}

void QKeyMapper::onHotKeyLineEditEditingFinished()
{
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender());
    if (lineEdit)
    {
        QString inputstring = lineEdit->text();
        if (validateCombinationKey(inputstring))
        {
            if (lineEdit->objectName() == WINDOWSWITCHKEY_LINEEDIT_NAME) {
                updateWindowSwitchKeyString(inputstring);
            }
            else if (lineEdit->objectName() == MAPPINGSWITCHKEY_LINEEDIT_NAME) {
                updateMappingSwitchKeyString(inputstring);
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
            else if (lineEdit->objectName() == MAPPINGSWITCHKEY_LINEEDIT_NAME) {
                lineEdit->setText(s_MappingSwitchKeyString);
                if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
                    showWarningPopup("Invalid input format for MappingSwitchKey!");
                }
                else {
                    showWarningPopup("映射开关键输入格式错误！");
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

        if (burst != KeyMappingDataList.at(row).Burst) {
            KeyMappingDataList[row].Burst = burst;
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
    if (m_KeyMappingDataTable->rowCount() == KeyMappingDataList.size()){
        QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        settingFile.setIniCodec("UTF-8");
#endif
        QStringList original_keys;
        QStringList mapping_keysList;
        QStringList burstList;
        QStringList burstpresstimeList;
        QStringList burstreleasetimeList;
        QStringList lockList;
        QStringList passthroughList;
        QStringList keyup_actionList;
        QStringList keyseqholddownList;
        // int burstpressTime = ui->burstpressSpinBox->value();
        // int burstreleaseTime = ui->burstreleaseSpinBox->value();
        int key2mouse_XSpeed = ui->mouseXSpeedSpinBox->value();
        int key2mouse_YSpeed = ui->mouseYSpeedSpinBox->value();
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
            }
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_ORIGINALKEYS, original_keys );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_MAPPINGKEYS , mapping_keysList );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURST , burstList );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTPRESS_TIME , burstpresstimeList );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME , burstreleasetimeList );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_LOCK , lockList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_PASSTHROUGH , passthroughList );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_KEYUP_ACTION , keyup_actionList );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_KEYSEQHOLDDOWN , keyseqholddownList );
            settingFile.setValue(saveSettingSelectStr+KEY2MOUSE_X_SPEED , key2mouse_XSpeed  );
            settingFile.setValue(saveSettingSelectStr+KEY2MOUSE_Y_SPEED , key2mouse_YSpeed  );
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
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTPRESS_TIME , burstpresstimeList );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME , burstreleasetimeList );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_LOCK , lockList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_PASSTHROUGH , passthroughList );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_KEYUP_ACTION , keyup_actionList );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_KEYSEQHOLDDOWN , keyseqholddownList );
            settingFile.setValue(saveSettingSelectStr+KEY2MOUSE_X_SPEED , key2mouse_XSpeed  );
            settingFile.setValue(saveSettingSelectStr+KEY2MOUSE_Y_SPEED , key2mouse_YSpeed  );
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
//             settingFile.setValue(saveSettingSelectStr+MAPPINGSWITCH_KEYSEQ, m_mappingswitchKeySeqEdit->keySequence().toString());
// #ifdef DEBUG_LOGOUT_ON
//             qDebug().nospace().noquote() << "[saveKeyMapSetting]" << " Save & Set Mapping Switch KeySequence [" << saveSettingSelectStr+MAPPINGSWITCH_KEYSEQ << "] -> \"" << m_mappingswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText) << "\"";
// #endif
//         }

        if (s_MappingSwitchKeyString.isEmpty()) {
            s_MappingSwitchKeyString = MAPPINGSWITCH_KEY_DEFAULT;
        }
        ui->mappingswitchkeyLineEdit->clearFocus();
        if (false == s_MappingSwitchKeyString.isEmpty()) {
            settingFile.setValue(saveSettingSelectStr+MAPPINGSWITCH_KEYSEQ, s_MappingSwitchKeyString);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[saveKeyMapSetting]" << " Save & Set Mapping Switch Key [" << saveSettingSelectStr+MAPPINGSWITCH_KEYSEQ << "] -> \"" << s_MappingSwitchKeyString << "\"";
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
    else{
        QString popupMessage;
        QString popupMessageColor;
        int popupMessageDisplayTime = 3000;
        if (LANGUAGE_ENGLISH == ui->languageComboBox->currentIndex()) {
            popupMessage = "Mapping list length error, unable to save.";
        }
        else {
            popupMessage = "映射列表长度错误，无法保存";
        }
        popupMessageColor = "#d63031";
        showPopupMessage(popupMessage, popupMessageColor, popupMessageDisplayTime);
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[saveKeyMapSetting]" << "KeyMappingDataList size error!!!";
#endif
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
        if (group.endsWith(QString(SEPARATOR_TITLESETTING)+ANYWINDOWTITLE_STRING, Qt::CaseInsensitive)
            || group.contains(QString(SEPARATOR_TITLESETTING)+WINDOWTITLE_STRING)
            || group.startsWith(PROCESS_UNKNOWN+QString(SEPARATOR_TITLESETTING)+WINDOWTITLE_STRING)) {
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
        QStringList burstpressStringList;
        QStringList burstreleaseStringList;
        QStringList lockStringList;
        QStringList passthroughStringList;
        QStringList keyup_actionStringList;
        QStringList keyseqholddownStringList;
        QList<bool> burstList;
        QList<int> burstpresstimeList;
        QList<int> burstreleasetimeList;
        QList<bool> lockList;
        QList<bool> passthroughList;
        QList<bool> keyup_actionList;
        QList<bool> keyseqholddownList;
        QList<MAP_KEYDATA> loadkeymapdata;
        bool global_datavalid = false;

        if ((true == settingFile.contains(settingSelectStr+KEYMAPDATA_ORIGINALKEYS))
            && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYS))){
            original_keys           = settingFile.value(settingSelectStr+KEYMAPDATA_ORIGINALKEYS).toStringList();
            mapping_keys            = settingFile.value(settingSelectStr+KEYMAPDATA_MAPPINGKEYS).toStringList();
            int mappingdata_size = original_keys.size();
            QStringList stringListAllOFF;
            QStringList burstpressStringListDefault;
            QStringList burstreleaseStringListDefault;
            for (int i = 0; i < mappingdata_size; ++i) {
                stringListAllOFF << "OFF";
                burstpressStringListDefault.append(QString::number(BURST_PRESS_TIME_DEFAULT));
                burstreleaseStringListDefault.append(QString::number(BURST_RELEASE_TIME_DEFAULT));
            }
            burstStringList         = stringListAllOFF;
            burstpressStringList    = burstpressStringListDefault;
            burstreleaseStringList  = burstreleaseStringListDefault;
            lockStringList          = stringListAllOFF;
            passthroughStringList   = stringListAllOFF;
            keyup_actionStringList  = stringListAllOFF;
            keyseqholddownStringList  = stringListAllOFF;
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURST)) {
                burstStringList = settingFile.value(settingSelectStr+KEYMAPDATA_BURST).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURSTPRESS_TIME)) {
                burstpressStringList = settingFile.value(settingSelectStr+KEYMAPDATA_BURSTPRESS_TIME).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME)) {
                burstreleaseStringList = settingFile.value(settingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_LOCK)) {
                lockStringList = settingFile.value(settingSelectStr+KEYMAPDATA_LOCK).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_PASSTHROUGH)) {
                passthroughStringList = settingFile.value(settingSelectStr+KEYMAPDATA_PASSTHROUGH).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_KEYUP_ACTION)) {
                keyup_actionStringList = settingFile.value(settingSelectStr+KEYMAPDATA_KEYUP_ACTION).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_KEYSEQHOLDDOWN)) {
                keyseqholddownStringList = settingFile.value(settingSelectStr+KEYMAPDATA_KEYSEQHOLDDOWN).toStringList();
            }

            if (original_keys.size() == mapping_keys.size() && original_keys.size() > 0) {
                global_datavalid = true;

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

                int loadindex = 0;
                static QRegularExpression reg("@[0-9]$");
                for (const QString &ori_key_nochange : qAsConst(original_keys)){
                    QString ori_key;
                    QRegularExpressionMatch match = reg.match(ori_key_nochange);
                    if (match.hasMatch()) {
                        int atIndex = ori_key_nochange.lastIndexOf('@');
                        ori_key = ori_key_nochange.mid(0, atIndex);
                    } else {
                        ori_key = ori_key_nochange;
                    }

                    bool keyboardmapcontains = QKeyMapper_Worker::VirtualKeyCodeMap.contains(ori_key);
                    bool mousemapcontains = QKeyMapper_Worker::VirtualMouseButtonMap.contains(ori_key);
                    bool joystickmapcontains = QKeyMapper_Worker::JoyStickKeyMap.contains(ori_key);
                    QString appendOriKey = ori_key_nochange;
                    if (ori_key.startsWith(PREFIX_SHORTCUT)) {
                        keyboardmapcontains = true;
                    }
                    else if (ori_key.contains(SEPARATOR_LONGPRESS)) {
                        keyboardmapcontains = true;
                    }
                    else if (ori_key.contains(SEPARATOR_DOUBLEPRESS)) {
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
                        loadkeymapdata.append(MAP_KEYDATA(appendOriKey, mapping_keys.at(loadindex), burstList.at(loadindex), burstpresstimeList.at(loadindex), burstreleasetimeList.at(loadindex), lockList.at(loadindex), passthroughList.at(loadindex), keyup_actionList.at(loadindex), keyseqholddownList.at(loadindex)));
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
        QStringList burstpressStringList;
        QStringList burstreleaseStringList;
        QStringList lockStringList;
        QStringList passthroughStringList;
        QStringList keyup_actionStringList;
        QStringList keyseqholddownStringList;
        QList<bool> burstList;
        QList<int> burstpresstimeList;
        QList<int> burstreleasetimeList;
        QList<bool> lockList;
        QList<bool> passthroughList;
        QList<bool> keyup_actionList;
        QList<bool> keyseqholddownList;
        QList<MAP_KEYDATA> loadkeymapdata;

        if ((true == settingFile.contains(settingSelectStr+KEYMAPDATA_ORIGINALKEYS))
            && (true == settingFile.contains(settingSelectStr+KEYMAPDATA_MAPPINGKEYS))){
            original_keys   = settingFile.value(settingSelectStr+KEYMAPDATA_ORIGINALKEYS).toStringList();
            mapping_keys    = settingFile.value(settingSelectStr+KEYMAPDATA_MAPPINGKEYS).toStringList();

            int mappingdata_size = original_keys.size();
            QStringList stringListAllOFF;
            QStringList burstpressStringListDefault;
            QStringList burstreleaseStringListDefault;
            for (int i = 0; i < mappingdata_size; ++i) {
                stringListAllOFF << "OFF";
                burstpressStringListDefault.append(QString::number(BURST_PRESS_TIME_DEFAULT));
                burstreleaseStringListDefault.append(QString::number(BURST_RELEASE_TIME_DEFAULT));
            }
            burstStringList         = stringListAllOFF;
            burstpressStringList    = burstpressStringListDefault;
            burstreleaseStringList  = burstreleaseStringListDefault;
            lockStringList          = stringListAllOFF;
            passthroughStringList   = stringListAllOFF;
            keyup_actionStringList   = stringListAllOFF;
            keyseqholddownStringList = stringListAllOFF;
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURST)) {
                burstStringList = settingFile.value(settingSelectStr+KEYMAPDATA_BURST).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURSTPRESS_TIME)) {
                burstpressStringList = settingFile.value(settingSelectStr+KEYMAPDATA_BURSTPRESS_TIME).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME)) {
                burstreleaseStringList = settingFile.value(settingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_LOCK)) {
                lockStringList = settingFile.value(settingSelectStr+KEYMAPDATA_LOCK).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_PASSTHROUGH)) {
                passthroughStringList = settingFile.value(settingSelectStr+KEYMAPDATA_PASSTHROUGH).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_KEYUP_ACTION)) {
                keyup_actionStringList = settingFile.value(settingSelectStr+KEYMAPDATA_KEYUP_ACTION).toStringList();
            }
            if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_KEYSEQHOLDDOWN)) {
                keyseqholddownStringList = settingFile.value(settingSelectStr+KEYMAPDATA_KEYSEQHOLDDOWN).toStringList();
            }

            if (original_keys.size() == mapping_keys.size() && original_keys.size() > 0) {
                datavalidflag = true;

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

                int loadindex = 0;
                static QRegularExpression reg("@[0-9]$");
                for (const QString &ori_key_nochange : qAsConst(original_keys)){
                    QString ori_key;
                    QRegularExpressionMatch match = reg.match(ori_key_nochange);
                    if (match.hasMatch()) {
                        int atIndex = ori_key_nochange.lastIndexOf('@');
                        ori_key = ori_key_nochange.mid(0, atIndex);
                    } else {
                        ori_key = ori_key_nochange;
                    }
                    bool keyboardmapcontains = QKeyMapper_Worker::VirtualKeyCodeMap.contains(ori_key);
                    bool mousemapcontains = QKeyMapper_Worker::VirtualMouseButtonMap.contains(ori_key);
                    bool joystickmapcontains = QKeyMapper_Worker::JoyStickKeyMap.contains(ori_key);
                    QString appendOriKey = ori_key_nochange;
                    if (ori_key.startsWith(PREFIX_SHORTCUT)) {
                        keyboardmapcontains = true;
                    }
                    else if (ori_key.contains(SEPARATOR_LONGPRESS)) {
                        keyboardmapcontains = true;
                    }
                    else if (ori_key.contains(SEPARATOR_DOUBLEPRESS)) {
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
                        loadkeymapdata.append(MAP_KEYDATA(appendOriKey, mapping_keys.at(loadindex), burstList.at(loadindex), burstpresstimeList.at(loadindex), burstreleasetimeList.at(loadindex), lockList.at(loadindex), passthroughList.at(loadindex), keyup_actionList.at(loadindex), keyseqholddownList.at(loadindex)));
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

        if (datavalidflag != (quint8)true){
            if (loadGlobalSetting && (0xFF == datavalidflag)) {
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[loadKeyMapSetting]" << "Empty Global Setting do not load default!";
#endif
            }
            else {
                KeyMappingDataList.clear();
                // KeyMappingDataList.append(MAP_KEYDATA("I",          "L-Shift + ]}",     false, BURST_PRESS_TIME_DEFAULT, BURST_RELEASE_TIME_DEFAULT, false, false, false));
                // KeyMappingDataList.append(MAP_KEYDATA("K",          "L-Shift + [{",     false, BURST_PRESS_TIME_DEFAULT, BURST_RELEASE_TIME_DEFAULT, false, false, false));
                // KeyMappingDataList.append(MAP_KEYDATA("H",          "S",                false, BURST_PRESS_TIME_DEFAULT, BURST_RELEASE_TIME_DEFAULT, false, false, false));
                // KeyMappingDataList.append(MAP_KEYDATA("Space",      "S",                false, BURST_PRESS_TIME_DEFAULT, BURST_RELEASE_TIME_DEFAULT, false, false, false));
                // KeyMappingDataList.append(MAP_KEYDATA("F",          "Enter",            false, BURST_PRESS_TIME_DEFAULT, BURST_RELEASE_TIME_DEFAULT, false, false, false));
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

    if (loadGlobalSetting && loadDefault != true) {
        ui->nameLineEdit->setText(QString());
        ui->titleLineEdit->setText(QString());
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

        if (true == loadDefault) {
            // setMapProcessInfo(QString(DEFAULT_NAME), QString(DEFAULT_TITLE), QString(), QString(), QIcon(":/DefaultIcon.ico"));
            ui->nameLineEdit->setText(QString());
            ui->titleLineEdit->setText(QString());
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

//     QString loadedmappingswitchKeySeqStr;
//     if (true == settingFile.contains(settingSelectStr+MAPPINGSWITCH_KEYSEQ)){
//         loadedmappingswitchKeySeqStr = settingFile.value(settingSelectStr+MAPPINGSWITCH_KEYSEQ).toString();
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
//     qDebug().nospace().noquote() << "[loadKeyMapSetting]" << " Load & Set Mapping Switch KeySequence [" << settingSelectStr+MAPPINGSWITCH_KEYSEQ << "] -> \"" << m_mappingswitchKeySeqEdit->keySequence().toString(QKeySequence::NativeText) << "\"";
// #endif

    QString loadedmappingswitchKeySeqStr;
    if (true == settingFile.contains(settingSelectStr+MAPPINGSWITCH_KEYSEQ)){
        loadedmappingswitchKeySeqStr = settingFile.value(settingSelectStr+MAPPINGSWITCH_KEYSEQ).toString();
        if (loadedmappingswitchKeySeqStr.isEmpty()) {
            loadedmappingswitchKeySeqStr = MAPPINGSWITCH_KEY_DEFAULT;
        }
    }
    else {
        loadedmappingswitchKeySeqStr = MAPPINGSWITCH_KEY_DEFAULT;
    }
    updateMappingSwitchKeyString(loadedmappingswitchKeySeqStr);
    ui->mappingswitchkeyLineEdit->setText(s_MappingSwitchKeyString);
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[loadKeyMapSetting]" << " Load & Set Mapping Switch Key [" << settingSelectStr+MAPPINGSWITCH_KEYSEQ << "] -> \"" << s_MappingSwitchKeyString << "\"";
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
#ifdef SEPARATOR_CONVERT
    mappingkeystr.replace(OLD_SEPARATOR_PLUS, SEPARATOR_PLUS);
    mappingkeystr.replace(OLD_SEPARATOR_NEXTARROW, SEPARATOR_NEXTARROW);
#endif

    bool checkResult = true;
    // static QRegularExpression regexp("\\s[+»]\\s");
    static QRegularExpression regexp("[+»]");
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QStringList Mapping_Keys = mappingkeystr.split(regexp, Qt::SkipEmptyParts);
#else
    QStringList Mapping_Keys = mappingkeystr.split(regexp, QString::SkipEmptyParts);
#endif
    for (const QString &mapping_key : qAsConst(Mapping_Keys)){
        if (false == QKeyMapper_Worker::VirtualKeyCodeMap.contains(mapping_key)
            && mapping_key != KEY_NONE_STR
            && mapping_key != KEY_BLOCKED_STR
            // && false == QKeyMapper_Worker::VirtualMouseButtonMap.contains(mapping_key)
            && false == mapping_key.startsWith(MOUSE_BUTTON_PREFIX)
            // && false == QKeyMapper_Worker::JoyStickKeyMap.contains(mapping_key)
            && false == mapping_key.startsWith(JOY_KEY_PREFIX)
            && false == mapping_key.startsWith(VJOY_KEY_PREFIX)
            && false == mapping_key.startsWith("Func-")
            && false == mapping_key.contains(SEPARATOR_WAITTIME)){
            checkResult = false;
            break;
        }
    }

    return checkResult;
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

    if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
        customFont.setPointSize(11);
    }
    else {
        customFont.setPointSize(9);
    }
    ui->processinfoTable->horizontalHeader()->setFont(customFont);
    m_KeyMappingDataTable->horizontalHeader()->setFont(customFont);

    customFont.setPointSize(9);
    ui->deleteoneButton->setFont(customFont);
    ui->clearallButton->setFont(customFont);
    ui->nameCheckBox->setFont(customFont);
    ui->titleCheckBox->setFont(customFont);
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
    ui->keyPressTypeComboBox->setFont(customFont);
    ui->pointLabel->setFont(customFont);
    // ui->pointDisplayLabel->setFont(customFont);
    // ui->waitTime_msLabel->setFont(customFont);
    ui->mouseXSpeedLabel->setFont(customFont);
    ui->mouseYSpeedLabel->setFont(customFont);
    ui->keyboardSelectLabel->setFont(customFont);
    ui->mouseSelectLabel->setFont(customFont);

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(10);
    // }
    // else {
    //     customFont.setPointSize(8);
    // }
    ui->settingTabWidget->tabBar()->setFont(customFont);
    ui->windowswitchkeyLabel->setFont(customFont);
    ui->mappingswitchkeyLabel->setFont(customFont);
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
    ui->nameCheckBox->setFont(customFont);
    ui->titleCheckBox->setFont(customFont);
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
    ui->keyPressTypeComboBox->setFont(customFont);
    ui->pointLabel->setFont(customFont);
    // ui->pointDisplayLabel->setFont(customFont);
    // ui->waitTime_msLabel->setFont(customFont);
    ui->mouseXSpeedLabel->setFont(customFont);
    ui->mouseYSpeedLabel->setFont(customFont);
    ui->keyboardSelectLabel->setFont(customFont);
    ui->mouseSelectLabel->setFont(customFont);

    ui->processinfoTable->horizontalHeader()->setFont(customFont);
    m_KeyMappingDataTable->horizontalHeader()->setFont(customFont);

    // if (UI_SCALE_4K_PERCENT_150 == m_UI_Scale) {
    //     customFont.setPointSize(11);
    // }
    // else {
    //     customFont.setPointSize(8);
    // }
    ui->settingTabWidget->tabBar()->setFont(customFont);
    ui->windowswitchkeyLabel->setFont(customFont);
    ui->mappingswitchkeyLabel->setFont(customFont);
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
    if (true == status && GROUPNAME_GLOBALSETTING == ui->settingselectComboBox->currentText()) {
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
    ui->autoStartMappingCheckBox->setEnabled(status);
    ui->autoStartupCheckBox->setEnabled(status);
    ui->startupMinimizedCheckBox->setEnabled(status);
    ui->soundEffectCheckBox->setEnabled(status);
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

    ui->moveupButton->setEnabled(status);
    ui->movedownButton->setEnabled(status);
    ui->nextarrowCheckBox->setEnabled(status);

    ui->windowswitchkeyLabel->setEnabled(status);
    // m_windowswitchKeySeqEdit->setEnabled(status);
    ui->windowswitchkeyLineEdit->setEnabled(status);
    ui->mappingswitchkeyLabel->setEnabled(status);
    // m_mappingswitchKeySeqEdit->setEnabled(status);
    ui->mappingswitchkeyLineEdit->setEnabled(status);

    // ui->refreshButton->setEnabled(status);
    ui->savemaplistButton->setEnabled(status);

    ui->processinfoTable->setEnabled(status);
    m_KeyMappingDataTable->setEnabled(status);
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

void QKeyMapper::showInputDeviceListWindow()
{
    if (!m_deviceListWindow->isVisible()) {
        m_deviceListWindow->show();
    }
}

void QKeyMapper::showItemSetupDialog(int row)
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
        // closeItemSetupDialog();
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

        showNormal();
        activateWindow();
        raise();
    }
}

void QKeyMapper::showWarningPopup(const QString &message)
{
    showPopupMessage(message, "#d63031", 3000);
}

void QKeyMapper::initKeyMappingDataTable(void)
{
    // 创建你的自定义控件
    m_KeyMappingDataTable = new KeyMappingDataTableWidget(this);
    m_KeyMappingDataTable->setGeometry(QRect(530, 30, 450, 330));

    // ui->keymapdataTable->setVisible(false);
    //m_KeyMappingDataTable->setStyle(QStyleFactory::create("windows"));
    m_KeyMappingDataTable->setFocusPolicy(Qt::NoFocus);
    m_KeyMappingDataTable->setColumnCount(KEYMAPPINGDATA_TABLE_COLUMN_COUNT);
//    m_KeyMappingDataTable->setHorizontalHeaderLabels(QStringList()   << "Original Key"
//                                                                    << "Mapping Key"
//                                                                    << "Burst"
//                                                                    << "Lock");

    m_KeyMappingDataTable->horizontalHeader()->setStretchLastSection(true);
    m_KeyMappingDataTable->horizontalHeader()->setHighlightSections(false);
    // m_KeyMappingDataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    int original_key_width = m_KeyMappingDataTable->width()/4 - 15;
    int burst_mode_width = m_KeyMappingDataTable->width()/5 - 40;
    int lock_width = m_KeyMappingDataTable->width()/5 - 40;
    int mapping_key_width = m_KeyMappingDataTable->width() - original_key_width - burst_mode_width - lock_width - 12;
    m_KeyMappingDataTable->setColumnWidth(ORIGINAL_KEY_COLUMN, original_key_width);
    m_KeyMappingDataTable->setColumnWidth(MAPPING_KEY_COLUMN, mapping_key_width);
    m_KeyMappingDataTable->setColumnWidth(BURST_MODE_COLUMN, burst_mode_width);
    m_KeyMappingDataTable->setColumnWidth(LOCK_COLUMN, lock_width);
    m_KeyMappingDataTable->verticalHeader()->setVisible(false);
    m_KeyMappingDataTable->verticalHeader()->setDefaultSectionSize(25);
    m_KeyMappingDataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_KeyMappingDataTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_KeyMappingDataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    /* Suuport Drag&Drop for KeyMappingData Table */
    m_KeyMappingDataTable->setDragEnabled(true);
    m_KeyMappingDataTable->setDragDropMode(QAbstractItemView::InternalMove);

#ifdef DEBUG_LOGOUT_ON
//    qDebug() << "verticalHeader->isVisible" << m_KeyMappingDataTable->verticalHeader()->isVisible();
//    qDebug() << "selectionBehavior" << m_KeyMappingDataTable->selectionBehavior();
//    qDebug() << "selectionMode" << m_KeyMappingDataTable->selectionMode();
//    qDebug() << "editTriggers" << m_KeyMappingDataTable->editTriggers();
//    qDebug() << "verticalHeader-DefaultSectionSize" << m_KeyMappingDataTable->verticalHeader()->defaultSectionSize();
#endif
}

void QKeyMapper::resizeKeyMappingDataTableColumnWidth()
{
    m_KeyMappingDataTable->resizeColumnToContents(ORIGINAL_KEY_COLUMN);

    int original_key_width_min = m_KeyMappingDataTable->width()/4 - 15;
    int original_key_width = m_KeyMappingDataTable->columnWidth(ORIGINAL_KEY_COLUMN);

    m_KeyMappingDataTable->resizeColumnToContents(BURST_MODE_COLUMN);
    int burst_mode_width = m_KeyMappingDataTable->columnWidth(BURST_MODE_COLUMN);
    int lock_width = burst_mode_width;
    // int burst_mode_width = m_KeyMappingDataTable->width()/5 - 40;
    // int lock_width = m_KeyMappingDataTable->width()/5 - 40;

    if (original_key_width < original_key_width_min) {
        original_key_width = original_key_width_min;
    }

    int mapping_key_width_min = m_KeyMappingDataTable->width()/4 - 15;
    int mapping_key_width = m_KeyMappingDataTable->width() - original_key_width - burst_mode_width - lock_width - 12;
    if (mapping_key_width < mapping_key_width_min) {
        mapping_key_width = mapping_key_width_min;
    }

    m_KeyMappingDataTable->setColumnWidth(ORIGINAL_KEY_COLUMN, original_key_width);
    m_KeyMappingDataTable->setColumnWidth(MAPPING_KEY_COLUMN, mapping_key_width);
    m_KeyMappingDataTable->setColumnWidth(BURST_MODE_COLUMN, burst_mode_width);
    m_KeyMappingDataTable->setColumnWidth(LOCK_COLUMN, lock_width);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[resizeKeyMappingDataTableColumnWidth]" << "m_KeyMappingDataTable->rowCount" << m_KeyMappingDataTable->rowCount();
    qDebug() << "[resizeKeyMappingDataTableColumnWidth]" << "original_key_width =" << original_key_width << ", mapping_key_width =" << mapping_key_width << ", burst_mode_width =" << burst_mode_width << ", lock_width =" << lock_width;
#endif
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
    // int top = ui->mappingswitchkeyLabel->y();
    // int left = ui->mappingswitchkeyLabel->x() + ui->mappingswitchkeyLabel->width() + 5;
    // m_mappingswitchKeySeqEdit->setObjectName(QStringLiteral("mappingswitchKeySeqEdit"));
    // m_mappingswitchKeySeqEdit->setGeometry(QRect(left, top, 110, 21));
    // m_mappingswitchKeySeqEdit->setFocusPolicy(Qt::ClickFocus);

    QLineEdit *lineEdit = ui->mappingswitchkeyLineEdit;
    // left = 390;
    // top = ui->mappingswitchkeyLabel->y();
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

void QKeyMapper::updateMappingSwitchKeyString(const QString &keystring)
{
    if (s_MappingSwitchKeyString != keystring) {
        s_MappingSwitchKeyString = keystring;
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[updateMappingSwitchKeyString]" << "s_MappingSwitchKeyString update ->" << keystring;
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

void QKeyMapper::refreshKeyMappingDataTable()
{
    m_KeyMappingDataTable->clearContents();
    m_KeyMappingDataTable->setRowCount(0);

    if (false == KeyMappingDataList.isEmpty()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshKeyMappingDataTable]" << "KeyMappingDataList Start >>>";
#endif
        int rowindex = 0;
        m_KeyMappingDataTable->setRowCount(KeyMappingDataList.size());
        for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
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

            if (keymapdata.Original_Key.contains(MOUSE_WHEEL_UP_STR)
                || keymapdata.Original_Key.contains(MOUSE_WHEEL_DOWN_STR)
                || keymapdata.Original_Key.contains(MOUSE_WHEEL_LEFT_STR)
                || keymapdata.Original_Key.contains(MOUSE_WHEEL_RIGHT_STR)) {
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
            QTableWidgetItem *ori_TableItem = new QTableWidgetItem(keymapdata.Original_Key);
            ori_TableItem->setToolTip(keymapdata.Original_Key);
            if (keymapdata.PassThrough) {
                ori_TableItem->setForeground(QBrush(STATUS_ON_COLOR));
            }
            if (keymapdata.KeyUp_Action) {
                QFont font = ori_TableItem->font();
                font.setUnderline(true);
                ori_TableItem->setFont(font);
            }
            m_KeyMappingDataTable->setItem(rowindex, ORIGINAL_KEY_COLUMN  , ori_TableItem);

            /* MAPPING_KEY_COLUMN */
            QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
            QTableWidgetItem *mapping_TableItem = new QTableWidgetItem(mappingkeys_str);
            mapping_TableItem->setToolTip(mappingkeys_str);
            m_KeyMappingDataTable->setItem(rowindex, MAPPING_KEY_COLUMN   , mapping_TableItem);

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
            m_KeyMappingDataTable->setItem(rowindex, BURST_MODE_COLUMN    , burstCheckBox);

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
            m_KeyMappingDataTable->setItem(rowindex, LOCK_COLUMN    , lockCheckBox);

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

    resizeKeyMappingDataTableColumnWidth();
    updateMousePointsList();
}

void QKeyMapper::updateMousePointsList()
{
    if (KeyMappingDataList.isEmpty()) {
        ScreenMousePointsList.clear();
        WindowMousePointsList.clear();
        return;
    }

    static QRegularExpression mousepoint_regex("Mouse-(L|R|M|X1|X2)(:W)?\\((\\d+),(\\d+)\\)");
    QRegularExpressionMatch mousepoint_match;
    ScreenMousePointsList.clear();
    WindowMousePointsList.clear();

    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
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
    ui->addmapdataButton->setText(ADDMAPDATABUTTON_CHINESE);
    ui->nameCheckBox->setText(NAMECHECKBOX_CHINESE);
    ui->titleCheckBox->setText(TITLECHECKBOX_CHINESE);
    ui->orikeyLabel->setText(ORIKEYLABEL_CHINESE);
    ui->orikeySeqLabel->setText(ORIKEYSEQLABEL_CHINESE);
    ui->mapkeyLabel->setText(MAPKEYLABEL_CHINESE);
    // ui->burstpressLabel->setText(BURSTPRESSLABEL_CHINESE);
    // ui->burstreleaseLabel->setText(BURSTRELEASE_CHINESE);
    // ui->burstpress_msLabel->setText(BURSTPRESS_MSLABEL_CHINESE);
    // ui->burstrelease_msLabel->setText(BURSTRELEASE_MSLABEL_CHINESE);
    ui->waitTimeLabel->setText(WAITTIME_CHINESE);
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
    ui->windowswitchkeyLabel->setText(WINDOWSWITCHKEYLABEL_CHINESE);
    ui->mappingswitchkeyLabel->setText(MAPPINGSWITCHKEYLABEL_CHINESE);

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

    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << PROCESSINFOTABLE_COL1_CHINESE
                                                                  << PROCESSINFOTABLE_COL2_CHINESE
                                                                  << PROCESSINFOTABLE_COL3_CHINESE );
    m_KeyMappingDataTable->setHorizontalHeaderLabels(QStringList()   << KEYMAPDATATABLE_COL1_CHINESE
                                                                 << KEYMAPDATATABLE_COL2_CHINESE
                                                                 << KEYMAPDATATABLE_COL3_CHINESE
                                                                 << KEYMAPDATATABLE_COL4_CHINESE);

    if (m_deviceListWindow != Q_NULLPTR) {
        m_deviceListWindow->setUILanguagee(LANGUAGE_CHINESE);
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
    ui->addmapdataButton->setText(ADDMAPDATABUTTON_ENGLISH);
    ui->nameCheckBox->setText(NAMECHECKBOX_ENGLISH);
    ui->titleCheckBox->setText(TITLECHECKBOX_ENGLISH);
    ui->orikeyLabel->setText(ORIKEYLABEL_ENGLISH);
    ui->orikeySeqLabel->setText(ORIKEYSEQLABEL_ENGLISH);
    ui->mapkeyLabel->setText(MAPKEYLABEL_ENGLISH);
    // ui->burstpressLabel->setText(BURSTPRESSLABEL_ENGLISH);
    // ui->burstreleaseLabel->setText(BURSTRELEASE_ENGLISH);
    // ui->burstpress_msLabel->setText(BURSTPRESS_MSLABEL_ENGLISH);
    // ui->burstrelease_msLabel->setText(BURSTRELEASE_MSLABEL_ENGLISH);
    ui->waitTimeLabel->setText(WAITTIME_ENGLISH);
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
    ui->windowswitchkeyLabel->setText(WINDOWSWITCHKEYLABEL_ENGLISH);
    ui->mappingswitchkeyLabel->setText(MAPPINGSWITCHKEYLABEL_ENGLISH);

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

    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << PROCESSINFOTABLE_COL1_ENGLISH
                                                                  << PROCESSINFOTABLE_COL2_ENGLISH
                                                                  << PROCESSINFOTABLE_COL3_ENGLISH );
    m_KeyMappingDataTable->setHorizontalHeaderLabels(QStringList()   << KEYMAPDATATABLE_COL1_ENGLISH
                                                                 << KEYMAPDATATABLE_COL2_ENGLISH
                                                                 << KEYMAPDATATABLE_COL3_ENGLISH
                                                                 << KEYMAPDATATABLE_COL4_ENGLISH);

    if (m_deviceListWindow != Q_NULLPTR) {
        m_deviceListWindow->setUILanguagee(LANGUAGE_ENGLISH);
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
        ui->languageComboBox->setFont(customFont);
        ui->virtualGamepadTypeComboBox->setFont(customFont);
        m_orikeyComboBox->setFont(customFont);
        m_mapkeyComboBox->setFont(customFont);
        ui->keyboardSelectComboBox->setFont(QFont(FONTNAME_ENGLISH, 8));
        ui->mouseSelectComboBox->setFont(QFont(FONTNAME_ENGLISH, 8));
        ui->settingselectComboBox->setFont(customFont);
        // m_windowswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        // m_mappingswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->windowswitchkeyLineEdit->setFont(customFont);
        ui->mappingswitchkeyLineEdit->setFont(customFont);
        // m_originalKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->combinationKeyLineEdit->setFont(customFont);
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
        m_KeyMappingDataTable->setFont(customFont);

        ui->vJoyXSensSpinBox->setFont(customFont);
        ui->vJoyYSensSpinBox->setFont(customFont);
        ui->virtualGamepadNumberSpinBox->setFont(customFont);
        ui->virtualGamepadListComboBox->setFont(customFont);
    }
    else {
        ui->nameLineEdit->setFont(customFont);
        ui->titleLineEdit->setFont(customFont);
        ui->languageComboBox->setFont(customFont);
        ui->virtualGamepadTypeComboBox->setFont(customFont);
        m_orikeyComboBox->setFont(customFont);
        m_mapkeyComboBox->setFont(customFont);
        ui->keyboardSelectComboBox->setFont(QFont(FONTNAME_ENGLISH, 8));
        ui->mouseSelectComboBox->setFont(QFont(FONTNAME_ENGLISH, 8));
        ui->settingselectComboBox->setFont(customFont);
        // m_windowswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        // m_mappingswitchKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->windowswitchkeyLineEdit->setFont(customFont);
        ui->mappingswitchkeyLineEdit->setFont(customFont);
        // m_originalKeySeqEdit->setFont(QFont("Microsoft YaHei", 9));
        ui->combinationKeyLineEdit->setFont(customFont);
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
        m_KeyMappingDataTable->setFont(customFont);

        ui->vJoyXSensSpinBox->setFont(customFont);
        ui->vJoyYSensSpinBox->setFont(customFont);
        ui->virtualGamepadNumberSpinBox->setFont(customFont);
        ui->virtualGamepadListComboBox->setFont(customFont);
    }

    if (m_deviceListWindow != Q_NULLPTR) {
        m_deviceListWindow->resetFontSize();
    }

    if (m_ItemSetupDialog != Q_NULLPTR) {
        m_ItemSetupDialog->resetFontSize();
    }
}

void QKeyMapper::updateLockStatusDisplay()
{
    int rowindex = 0;
    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
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
            m_KeyMappingDataTable->item(rowindex, LOCK_COLUMN)->setText(QString());
            m_KeyMappingDataTable->item(rowindex, LOCK_COLUMN)->setForeground(Qt::black);
        }

        rowindex += 1;
    }
}

void QKeyMapper::updateMousePointLabelDisplay(const QPoint &point)
{
    if (point.x() >= 0 && point.y() >= 0) {
        QString labelText = QString("X:%1, Y:%2").arg(point.x()).arg(point.y());
        ui->pointDisplayLabel->setText(labelText);
    }
    else {
        ui->pointDisplayLabel->clear();
    }
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

void QKeyMapper::keyMappingTableDragDropMove(int from, int to)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[keyMappingTableDragDropMove] DragDrop : Row" << from << "->" << to;
#endif

    int mappingdata_size = KeyMappingDataList.size();
    if (from >= 0 && from < mappingdata_size && to >= 0 && to < mappingdata_size) {
        KeyMappingDataList.move(from, to);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[keyMappingTableDragDropMove] : refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable();

        int reselectrow = to;
        QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        m_KeyMappingDataTable->setRangeSelected(selection, true);

#ifdef DEBUG_LOGOUT_ON
        if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList.size()){
            qDebug("keyMappingTableDragDropMove : KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList.size());
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

    showItemSetupDialog(rowindex);
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

    refreshKeyMappingDataTable();

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

#if 0
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
#endif

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
        // ui->disableWinKeyCheckBox->setEnabled(true);

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
            currentOriKeyText = QString(PREFIX_SHORTCUT) + currentOriCombinationKeyText;
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
    // else if (false == currentOriKeyShortcutText.isEmpty()) {
    //     currentOriKeyText = QString(PREFIX_SHORTCUT) + currentOriKeyShortcutText;
    // }

    if (currentOriKeyText.isEmpty() || (m_mapkeyComboBox->isEnabled() && currentMapKeyText.isEmpty() && ui->nextarrowCheckBox->isChecked() == false)) {
        return;
    }

    currentOriKeyTextWithoutPostfix = currentOriKeyText;
    int pressTime = ui->pressTimeSpinBox->value();
    bool isSpecialOriginalKey = QKeyMapper_Worker::SpecialOriginalKeysList.contains(currentOriKeyComboBoxText);
    if (ui->keyPressTypeComboBox->currentIndex() == KEYPRESS_TYPE_LONGPRESS && pressTime > 0 && isSpecialOriginalKey == false) {
        currentOriKeyText = currentOriKeyText + QString(SEPARATOR_LONGPRESS) + QString::number(pressTime);
    }
    else if (ui->keyPressTypeComboBox->currentIndex() == KEYPRESS_TYPE_DOUBLEPRESS && pressTime > 0 && isSpecialOriginalKey == false){
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
        if (VJOY_MOUSE2LS_STR == currentOriKeyComboBoxText
            || VJOY_MOUSE2RS_STR == currentOriKeyComboBoxText
            || JOY_LS2MOUSE_STR == currentOriKeyComboBoxText
            || JOY_RS2MOUSE_STR == currentOriKeyComboBoxText
            || JOY_LS2VJOYLS_STR == currentOriKeyComboBoxText
            || JOY_RS2VJOYRS_STR == currentOriKeyComboBoxText
            || JOY_LS2VJOYRS_STR == currentOriKeyComboBoxText
            || JOY_RS2VJOYLS_STR == currentOriKeyComboBoxText
            || JOY_LT2VJOYLT_STR == currentOriKeyComboBoxText
            || JOY_RT2VJOYRT_STR == currentOriKeyComboBoxText) {
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
            if (currentMapKeyText == KEY_BLOCKED_STR
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
                MAP_KEYDATA keymapdata = KeyMappingDataList.at(findindex);
                if (keymapdata.Mapping_Keys.contains(KEY_BLOCKED_STR)
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
            MAP_KEYDATA keymapdata = KeyMappingDataList.at(findindex);
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
                        currentMapKeyText = currentMapKeyText.remove(MOUSE_WINDOWPOINT_POSTFIX) + QString(":W(%1,%2)").arg(x).arg(y);

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

            KeyMappingDataList.replace(findindex, MAP_KEYDATA(currentOriKeyText, mappingkeys_str, keymapdata.Burst, keymapdata.BurstPressTime, keymapdata.BurstReleaseTime, keymapdata.Lock, keymapdata.PassThrough, keymapdata.KeyUp_Action, keymapdata.KeySeqHoldDown));
        }
        else {
            if (VJOY_MOUSE2LS_STR == currentOriKeyComboBoxText
                || VJOY_MOUSE2RS_STR == currentOriKeyComboBoxText
                || JOY_LS2MOUSE_STR == currentOriKeyComboBoxText
                || JOY_RS2MOUSE_STR == currentOriKeyComboBoxText
                || JOY_LS2VJOYLS_STR == currentOriKeyComboBoxText
                || JOY_RS2VJOYRS_STR == currentOriKeyComboBoxText
                || JOY_LS2VJOYRS_STR == currentOriKeyComboBoxText
                || JOY_RS2VJOYLS_STR == currentOriKeyComboBoxText
                || JOY_LT2VJOYLT_STR == currentOriKeyComboBoxText
                || JOY_RT2VJOYRT_STR == currentOriKeyComboBoxText) {
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

                int waitTime = ui->waitTimeSpinBox->value();
                if (waitTime > 0
                    && currentMapKeyComboBoxText != KEY_BLOCKED_STR
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

            KeyMappingDataList.append(MAP_KEYDATA(currentOriKeyText, currentMapKeyText, false, BURST_PRESS_TIME_DEFAULT, BURST_RELEASE_TIME_DEFAULT, false, false, false, false));
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
        KeyMappingDataList.removeAt(currentrowindex);

        /* do not refresh for select cursor hold position */
//        refreshKeyMappingDataTable();
        updateMousePointsList();
#ifdef DEBUG_LOGOUT_ON
        if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList.size()){
            qDebug("KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList.size());
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
        KeyMappingDataList.clear();
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
        else{
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
                QString newMapKeyText;
                if (currentMapKeyText.isEmpty()) {
                    newMapKeyText = currentMapKeyListText;
                }
                else {
                    int cursorPos = QItemSetupDialog::getMappingKeyCursorPosition();
                    bool isCursorAtEnd = (cursorPos == currentMapKeyText.length());

                    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
                        if (currentMapKeyListText == SEPARATOR_WAITTIME) {
                            newMapKeyText = currentMapKeyText + currentMapKeyListText;
                        }
                        else {
                            newMapKeyText = currentMapKeyText + QString(SEPARATOR_NEXTARROW) + currentMapKeyListText;
                        }
                    }
                    else {
                        if (isCursorAtEnd) {
                            if (currentMapKeyListText == SEPARATOR_WAITTIME) {
                                newMapKeyText = currentMapKeyText + currentMapKeyListText;
                            }
                            else {
                                newMapKeyText = currentMapKeyText + QString(SEPARATOR_PLUS) + currentMapKeyListText;
                            }
                        }
                        else {
                            if (currentMapKeyListText == SEPARATOR_WAITTIME) {
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
        KeyMappingDataList.move(currentrowindex, currentrowindex-1);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable();

        int reselectrow = currentrowindex - 1;
        QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        m_KeyMappingDataTable->setRangeSelected(selection, true);

#ifdef DEBUG_LOGOUT_ON
        if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList.size()){
            qDebug("MoveUp:KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList.size());
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
        KeyMappingDataList.move(currentrowindex, currentrowindex+1);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
        refreshKeyMappingDataTable();

        int reselectrow = currentrowindex + 1;
        QTableWidgetSelectionRange selection = QTableWidgetSelectionRange(reselectrow, 0, reselectrow, KEYMAPPINGDATA_TABLE_COLUMN_COUNT - 1);
        m_KeyMappingDataTable->setRangeSelected(selection, true);

#ifdef DEBUG_LOGOUT_ON
        if (m_KeyMappingDataTable->rowCount() != KeyMappingDataList.size()){
            qDebug("MoveDown:KeyMapData sync error!!! DataTableSize(%d), DataListSize(%d)", m_KeyMappingDataTable->rowCount(), KeyMappingDataList.size());
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
        // ui->disableWinKeyCheckBox->setEnabled(true);
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
    resetFontSize();
    refreshProcessInfoTable();
    resizeKeyMappingDataTableColumnWidth();

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

void QKeyMapper::on_keymapdataTable_itemSelectionChanged()
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
        qDebug() << "[on_keymapdataTable_itemSelectionChanged] Selected Item Index =" << currentrowindex;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_keymapdataTable_itemSelectionChanged] No Item Selected.";
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
//         qDebug() << "[KeyMappingDataTable::dropEvent] MoveAction : Row" << m_DraggedRow << "->" << droppedRow;
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
            if (GROUPNAME_GLOBALSETTING == ui->settingselectComboBox->currentText()) {
                ui->sendToSameTitleWindowsCheckBox->setEnabled(false);
            }
            else {
                ui->sendToSameTitleWindowsCheckBox->setEnabled(true);
            }
        }
    }
}
