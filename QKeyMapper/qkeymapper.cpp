#include "qkeymapper.h"
#include "ui_qkeymapper.h"

//static const uint WIN_TITLESTR_MAX = 200U;
static const uint CYCLE_CHECK_TIMEOUT = 300U;
static const int PROCESSINFO_TABLE_COLUMN_COUNT = 3;
static const int KEYMAPPINGDATA_TABLE_COLUMN_COUNT = 4;

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

static const int SENDMODE_HOOK          = 0;
static const int SENDMODE_BURST_NORMAL  = 1;
static const int SENDMODE_BURST_STOP    = 2;

static const int CUSTOMSETTING_INDEX_MAX = 30;

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

static const ULONG_PTR VIRTUAL_KEYBOARD_PRESS = 0xACBDACBD;
static const ULONG_PTR VIRTUAL_MOUSE_CLICK = 0xCEDFCEDF;

static const char *DEFAULT_NAME = "ForzaHorizon4.exe";
static const char *CONFIG_FILENAME = "keymapdata.ini";

static const char *LANGUAGE_INDEX = "LanguageIndex";
static const char *SETTINGSELECT = "SettingSelect";
static const char *AUTO_STARTUP = "AutoStartup";
static const char *GROUPNAME_EXECUTABLE_SUFFIX = ".exe";
static const char *GROUPNAME_CUSTOMSETTING = "CustomSetting ";

static const char *KEYMAPDATA_ORIGINALKEYS = "KeyMapData_OriginalKeys";
static const char *KEYMAPDATA_MAPPINGKEYS = "KeyMapData_MappingKeys";
static const char *KEYMAPDATA_BURST = "KeyMapData_Burst";
static const char *KEYMAPDATA_LOCK = "KeyMapData_Lock";
static const char *KEYMAPDATA_BURSTPRESS_TIME = "KeyMapData_BurstPressTime";
static const char *KEYMAPDATA_BURSTRELEASE_TIME = "KeyMapData_BurstReleaseTime";
static const char *CLEARALL = "KeyMapData_ClearAll";

static const char *PROCESSINFO_FILENAME = "ProcessInfo_FileName";
static const char *PROCESSINFO_WINDOWTITLE = "ProcessInfo_WindowTitle";
static const char *PROCESSINFO_FILEPATH = "ProcessInfo_FilePath";
static const char *PROCESSINFO_FILENAME_CHECKED = "ProcessInfo_FileNameChecked";
static const char *PROCESSINFO_WINDOWTITLE_CHECKED = "ProcessInfo_WindowTitleChecked";

static const char *DISABLEWINKEY_CHECKED = "DisableWinKeyChecked";
static const char *AUTOSTARTMAPPING_CHECKED = "AutoStartMappingChecked";

static const char *SAO_FONTFILENAME = ":/sao_ui.otf";

static const char *SOUNDFILE_START_QRC = ":/QKeyMapperStart.wav";
static const char *SOUNDFILE_START = "QKeyMapperStart.wav";

static const char *FONTNAME_ENGLISH = "Microsoft YaHei UI";
static const char *FONTNAME_CHINESE = "NSimSun";

static const char *REFRESHBUTTON_CHINESE = "刷新";
static const char *KEYMAPBUTTON_START_CHINESE = "开始按键映射";
static const char *KEYMAPBUTTON_STOP_CHINESE = "停止按键映射";
static const char *SAVEMAPLISTBUTTON_CHINESE = "保存设定";
static const char *DELETEONEBUTTON_CHINESE = "删除单行";
static const char *CLEARALLBUTTON_CHINESE = "全部清除";
static const char *ADDMAPDATABUTTON_CHINESE = "添加";
static const char *NAMECHECKBOX_CHINESE = "文件名";
static const char *TITLECHECKBOX_CHINESE = "标题";
static const char *ORIKEYLABEL_CHINESE = "原始按键";
static const char *MAPKEYLABEL_CHINESE = "映射按键";
static const char *BURSTPRESSLABEL_CHINESE = "连发按下时间";
static const char *BURSTRELEASE_CHINESE = "连发抬起时间";
static const char *BURSTPRESS_MSLABEL_CHINESE = "毫秒";
static const char *BURSTRELEASE_MSLABEL_CHINESE = "毫秒";
static const char *SETTINGSELECTLABEL_CHINESE = "设定选择";
static const char *REMOVESETTINGBUTTON_CHINESE = "移除";
static const char *DISABLEWINKEYCHECKBOX_CHINESE = "禁用WIN按键";
static const char *AUTOSTARTMAPPINGCHECKBOX_CHINESE = "自动开始映射";
static const char *AUTOSTARTUPCHECKBOX_CHINESE = "开机自动启动";
static const char *PROCESSINFOTABLE_COL1_CHINESE = "文件名";
static const char *PROCESSINFOTABLE_COL2_CHINESE = "进程号";
static const char *PROCESSINFOTABLE_COL3_CHINESE = "标题";
static const char *KEYMAPDATATABLE_COL1_CHINESE = "原始按键";
static const char *KEYMAPDATATABLE_COL2_CHINESE = "映射按键";
static const char *KEYMAPDATATABLE_COL3_CHINESE = "连发";
static const char *KEYMAPDATATABLE_COL4_CHINESE = "锁定";

static const char *REFRESHBUTTON_ENGLISH = "Refresh";
static const char *KEYMAPBUTTON_START_ENGLISH = "KeyMappingStart";
static const char *KEYMAPBUTTON_STOP_ENGLISH = "KeyMappingStop";
static const char *SAVEMAPLISTBUTTON_ENGLISH = "SaveSetting";
static const char *DELETEONEBUTTON_ENGLISH = "Delete One";
static const char *CLEARALLBUTTON_ENGLISH = "Clear All";
static const char *ADDMAPDATABUTTON_ENGLISH = "ADD";
static const char *NAMECHECKBOX_ENGLISH = "Name";
static const char *TITLECHECKBOX_ENGLISH = "Title";
static const char *ORIKEYLABEL_ENGLISH = "OriKey";
static const char *MAPKEYLABEL_ENGLISH = "MapKey";
static const char *BURSTPRESSLABEL_ENGLISH = "BurstPress";
static const char *BURSTRELEASE_ENGLISH = "BurstRelease";
static const char *BURSTPRESS_MSLABEL_ENGLISH = "ms";
static const char *BURSTRELEASE_MSLABEL_ENGLISH = "ms";
static const char *SETTINGSELECTLABEL_ENGLISH = "SettingSelect";
static const char *REMOVESETTINGBUTTON_ENGLISH = "Remove";
static const char *DISABLEWINKEYCHECKBOX_ENGLISH = "Disable WIN Key";
static const char *AUTOSTARTMAPPINGCHECKBOX_ENGLISH = "Auto Start Mapping";
static const char *AUTOSTARTUPCHECKBOX_ENGLISH = "Auto Startup";
static const char *PROCESSINFOTABLE_COL1_ENGLISH = "Name";
static const char *PROCESSINFOTABLE_COL2_ENGLISH = "PID";
static const char *PROCESSINFOTABLE_COL3_ENGLISH = "Title";
static const char *KEYMAPDATATABLE_COL1_ENGLISH = "OriginalKey";
static const char *KEYMAPDATATABLE_COL2_ENGLISH = "MappingKey";
static const char *KEYMAPDATATABLE_COL3_ENGLISH = "Burst";
static const char *KEYMAPDATATABLE_COL4_ENGLISH = "Lock";


QKeyMapper *QKeyMapper::m_instance = Q_NULLPTR;
QString QKeyMapper::DEFAULT_TITLE = QString("Forza: Horizon 4");
QList<MAP_PROCESSINFO> QKeyMapper::static_ProcessInfoList = QList<MAP_PROCESSINFO>();
QList<MAP_KEYDATA> QKeyMapper::KeyMappingDataList = QList<MAP_KEYDATA>();

QKeyMapper::QKeyMapper(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QKeyMapper),
    m_KeyMapStatus(KEYMAP_IDLE),
    m_CycleCheckTimer(this),
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
    m_HotKey(new QHotkey(this)),
    m_UI_Scale(UI_SCALE_NORMAL)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("QKeyMapper() -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif

    m_instance = this;
    ui->setupUi(this);
    extractSoundFiles();
    initAddKeyComboBoxes();
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
    ui->nameLineEdit->setFocusPolicy(Qt::NoFocus);
    ui->titleLineEdit->setFocusPolicy(Qt::NoFocus);

    m_SysTrayIcon = new QSystemTrayIcon(this);
    m_SysTrayIcon->setIcon(QIcon(":/QKeyMapper.ico"));
    m_SysTrayIcon->setToolTip("QKeyMapper(Idle)");
    m_SysTrayIcon->show();

    initKeyMappingDataTable();
    bool loadresult = loadKeyMapSetting(QString());
    Q_UNUSED(loadresult);
    reloadUILanguage();
    resetFontSize();

    QObject::connect(m_SysTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(SystrayIconActivated(QSystemTrayIcon::ActivationReason)));
    QObject::connect(&m_CycleCheckTimer, SIGNAL(timeout()), this, SLOT(cycleCheckProcessProc()));
    QObject::connect(ui->keymapdataTable, SIGNAL(cellChanged(int,int)), this, SLOT(cellChanged_slot(int,int)));

    QObject::connect(this, SIGNAL(updateLockStatus_Signal()), this, SLOT(updateLockStatusDisplay()), Qt::QueuedConnection);

    //m_CycleCheckTimer.start(CYCLE_CHECK_TIMEOUT);
    refreshProcessInfoTable();

//    if (false == loadresult){
//        QMessageBox::warning(this, tr("QKeyMapper"), tr("Load invalid keymapdata from ini file.\nReset to default values."));
//    }

    initHotKeySequence();
}

QKeyMapper::~QKeyMapper()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "~QKeyMapper() called.";
#endif

    delete m_orikeyComboBox;
    m_orikeyComboBox = Q_NULLPTR;
    delete m_mapkeyComboBox;
    m_mapkeyComboBox = Q_NULLPTR;

    delete m_HotKey;
    m_HotKey = Q_NULLPTR;

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
                bool savecheckresult = checkSaveSettings(filename);

                if (savecheckresult && KEYMAP_CHECKING == m_KeyMapStatus) {
                    bool needtoload = false;
                    QVariant nameChecked_Var;
                    QVariant titleChecked_Var;
                    QVariant fileName_Var;
                    QVariant windowTitle_Var;
                    bool nameChecked = false;
                    bool titleChecked = false;
                    QString readFileName;
                    QString readWindowTitle;

                    if (readSaveSettingData(filename, PROCESSINFO_FILENAME_CHECKED, nameChecked_Var)) {
                        nameChecked = nameChecked_Var.toBool();
                    }
                    if (readSaveSettingData(filename, PROCESSINFO_WINDOWTITLE_CHECKED, titleChecked_Var)) {
                        titleChecked = titleChecked_Var.toBool();
                    }
                    if (readSaveSettingData(filename, PROCESSINFO_FILENAME, fileName_Var)) {
                        readFileName = fileName_Var.toString();
                    }
                    if (readSaveSettingData(filename, PROCESSINFO_WINDOWTITLE, windowTitle_Var)) {
                        readWindowTitle = windowTitle_Var.toString();
                    }

                    if ((true == nameChecked)
                            && (true == titleChecked)){
                        if ((filename == readFileName)
                                && (windowTitle == readWindowTitle)){
                            needtoload = true;
                        }
                    }
                    else if (true == nameChecked){
                        if (filename == readFileName){
                            needtoload = true;
                        }
                    }
                    else if (true == titleChecked){
                        if (windowTitle == windowTitle){
                            needtoload = true;
                        }
                    }

                    if (needtoload) {
                        QString curSettingSelectStr = ui->settingselectComboBox->currentText();
                        if (curSettingSelectStr != filename) {
#ifdef DEBUG_LOGOUT_ON
                            qDebug().nospace().noquote() << "[cycleCheckProcessProc] "<< "Setting Check Matched! Load setting -> [" << filename << "]";
#endif
                            bool loadresult = loadKeyMapSetting(filename);
                            Q_UNUSED(loadresult)
                        }
                        else {
#ifdef DEBUG_LOGOUT_ON
                            qDebug() << "[cycleCheckProcessProc]" << "Current setting select is already the same ->" << curSettingSelectStr;
#endif
                        }
                    }
                }
            }

            if ((true == ui->nameCheckBox->isChecked())
                    && (true == ui->titleCheckBox->isChecked())){
                if ((m_MapProcessInfo.FileName == filename)
                        && (m_MapProcessInfo.WindowTitle == windowTitle)){
                    checkresult = 1;
                }
            }
            else if (true == ui->nameCheckBox->isChecked()){
                if (m_MapProcessInfo.FileName == filename){
                    checkresult = 1;
                }
            }
            else if (true == ui->titleCheckBox->isChecked()){
                if (m_MapProcessInfo.WindowTitle == windowTitle){
                    checkresult = 1;
                }
            }
            else{
                // checkresult = 2;
            }

            if ((m_MapProcessInfo.FileName == filename)
                    && (m_MapProcessInfo.WindowTitle == windowTitle)){
                checkresult = 1;
            }
        }

        bool isAltTabWindow = IsAltTabWindow(hwnd);

        if (checkresult){
            if (KEYMAP_CHECKING == m_KeyMapStatus){
                playStartSound();
                setKeyHook(hwnd);
                m_KeyMapStatus = KEYMAP_MAPPING;
                updateSystemTrayDisplay();
                emit updateLockStatus_Signal();

#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[cycleCheckProcessProc]" << " checkresult = " << checkresult << "," << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") " << "ForegroundWindow: " << windowTitle << "(" << filename << "), " << "IsAltTabWindow -> " << isAltTabWindow;
                qDebug().nospace() << "[cycleCheckProcessProc]" << " NameChecked = " << ui->nameCheckBox->isChecked() << "," << " TitleChecked = " << ui->titleCheckBox->isChecked();
                qDebug().nospace() << "[cycleCheckProcessProc]" << " ProcessInfo.FileName = " << m_MapProcessInfo.FileName << "," << " ProcessInfo.WindowTitle = " << m_MapProcessInfo.WindowTitle;
                qDebug().nospace() << "[cycleCheckProcessProc]" << " CurrentFileName = " << filename << "," << " CurrentWindowTitle = " << windowTitle;
#endif
            }
        }
        else{
            if (KEYMAP_MAPPING == m_KeyMapStatus){
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[cycleCheckProcessProc]" << " checkresult = " << checkresult << "," << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") " << "ForegroundWindow: " << windowTitle << "(" << filename << "), " << "IsAltTabWindow -> " << isAltTabWindow;
                qDebug().nospace() << "[cycleCheckProcessProc]" << " NameChecked = " << ui->nameCheckBox->isChecked() << "," << " TitleChecked = " << ui->titleCheckBox->isChecked();
                qDebug().nospace() << "[cycleCheckProcessProc]" << " ProcessInfo.FileName = " << m_MapProcessInfo.FileName << "," << " ProcessInfo.WindowTitle = " << m_MapProcessInfo.WindowTitle;
                qDebug().nospace() << "[cycleCheckProcessProc]" << " CurrentFileName = " << filename << "," << " CurrentWindowTitle = " << windowTitle;
#endif

                /* Add for mouse2joystick_Custom_CEMU.exe Bug Fix >>> */
                if (filename == "mouse2joystick_Custom_CEMU.exe"
                    && windowTitle == "Controller"
                    && m_MapProcessInfo.FileName == "Cemu.exe"
                    && false == isAltTabWindow) {
#ifdef DEBUG_LOGOUT_ON
                    bool isVisibleWindow = IsVisibleWindow(hwnd);
                    Q_UNUSED(isVisibleWindow);
                    for (int loop = 0; loop < 10; loop++) {
                        qDebug().nospace() << "[cycleCheckProcessProc]" << "[BugFix] Cemu.exe -> Controller(mouse2joystick_Custom_CEMU.exe)(isVisibleWindow == false)";
                    }
#endif
                    return;
                }
                /* Add for mouse2joystick_Custom_CEMU.exe Bug Fix <<< */

                setKeyUnHook();
                m_KeyMapStatus = KEYMAP_CHECKING;
                updateSystemTrayDisplay();
                emit updateLockStatus_Signal();
            }
        }
    }
    else{
        //EnumWindows((WNDENUMPROC)QKeyMapper::EnumWindowsProc, 0);
    }
}

void QKeyMapper::setKeyHook(HWND hWnd)
{
    emit QKeyMapper_Worker::getInstance()->setKeyHook_Signal(hWnd);
}

void QKeyMapper::setKeyUnHook(void)
{
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
            return false;

        //  It must not be a cloaked window
        BOOL isCloaked = TRUE;
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

BOOL QKeyMapper::IsVisibleWindow(HWND hWnd)
{
    //  It must be a visible Window
    if (!IsWindowVisible(hWnd)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[IsVisibleWindow] " << "IsWindowVisible() -> FALSE";
#endif
        return FALSE;
    }

    //  It must not be a Tool bar window
    WINDOWINFO winInfo;
    winInfo.cbSize = sizeof(WINDOWINFO);
    if (GetWindowInfo(hWnd, &winInfo)) {
        if ((winInfo.dwExStyle & WS_EX_TOOLWINDOW) != 0) {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[IsVisibleWindow] " << "WS_EX_TOOLWINDOW -> MATCH";
#endif
            return false;
        }

        //  It must not be a cloaked window
        BOOL isCloaked = TRUE;
        HRESULT hr = DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(BOOL));
        if (SUCCEEDED(hr)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[IsVisibleWindow] " << "isCloaked -> " << (isCloaked == TRUE);
#endif

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
                //&& (WindowText != QString("QKeyMapper"))
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
                BOOL isVisibleWindow = IsAltTabWindow(hWnd);
                if (TRUE == isVisibleWindow) {
                    static_ProcessInfoList.append(ProcessInfo);
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
                //&& (WindowText != QString("QKeyMapper"))
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

int QKeyMapper::findInKeyMappingDataList(const QString &keyname)
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
                        //&& (WindowText != QString("QKeyMapper"))
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

bool QKeyMapper::getAutoStartMappingStatus()
{
    if (true == ui->autoStartMappingCheckBox->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}

bool QKeyMapper::getDisableWinKeyStatus()
{
    if (true == getInstance()->ui->disableWinKeyCheckBox->isChecked()) {
        return true;
    }
    else {
        return false;
    }
}

int QKeyMapper::getBurstPressTime()
{
    return getInstance()->ui->burstpressComboBox->currentText().toInt();
}

int QKeyMapper::getBurstReleaseTime()
{
    return getInstance()->ui->burstreleaseComboBox->currentText().toInt();
}

void QKeyMapper::changeEvent(QEvent *event)
{
    if(event->type()==QEvent::WindowStateChange)
    {
        QTimer::singleShot(0, this, SLOT(WindowStateChangedProc()));
    }
    QDialog::changeEvent(event);
}

void QKeyMapper::on_keymapButton_clicked()
{
    QMetaEnum keymapstatusEnum = QMetaEnum::fromType<QKeyMapper::KeyMapStatus>();
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
            updateSystemTrayDisplay();
            emit updateLockStatus_Signal();
            startKeyMap = true;

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[KeyMappingButton]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") " << "on_keymapButton_clicked()";
#endif
        }
        else{
            QMessageBox::warning(this, tr("QKeyMapper"), tr("Invalid process info for key mapping."));
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
        setKeyUnHook();
        m_KeyMapStatus = KEYMAP_IDLE;
        emit updateLockStatus_Signal();

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[KeyMappingButton]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") " << "on_keymapButton_clicked()";
#endif
    }

    if (m_KeyMapStatus != KEYMAP_IDLE){
        changeControlEnableStatus(false);
    }
    else{
        changeControlEnableStatus(true);
    }

    if (true == startKeyMap) {
        cycleCheckProcessProc();
    }
}

void QKeyMapper::HotKeyActivated()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "HotKeyActivated() Called, is Hidden:" << isHidden();
#endif
    if (false == isHidden()){
        hide();
    }
    else{
        showNormal();
        activateWindow();
        raise();
    }
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
            QMessageBox::warning(this, tr("QKeyMapper"), tr(message.toStdString().c_str()));
#ifdef DEBUG_LOGOUT_ON
            qDebug("[%s]: row(%d) could not set burst or lock for key sequence(%d)", __func__, row, KeyMappingDataList[row].Mapping_Keys.size());
#endif
            return;
        }
    }

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

bool QKeyMapper::checkSaveSettings(const QString &executablename)
{
    bool checkresult = false;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    QStringList groups = settingFile.childGroups();

    if (groups.contains(executablename)) {
#if 0
        QString curSettingSelectStr = ui->settingselectComboBox->currentText();
        if (curSettingSelectStr == executablename) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[checkSaveSettings]" << "Current setting select is already the same ->" << executablename;
#endif
            return false;
        }
#endif

        QString settingSelectStr = executablename + "/";
        bool autoStartMappingChecked = false;
        if (true == settingFile.contains(settingSelectStr+AUTOSTARTMAPPING_CHECKED)){
            autoStartMappingChecked = settingFile.value(settingSelectStr+AUTOSTARTMAPPING_CHECKED).toBool();
            if (true == autoStartMappingChecked) {
                checkresult = true;
            }
        }
    }

    return checkresult;
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
        QString burstpressTimeString = ui->burstpressComboBox->currentText();
        QString burstreleaseTimeString = ui->burstreleaseComboBox->currentText();

        QString saveSettingSelectStr;
        QString cursettingSelectStr = ui->settingselectComboBox->currentText();
        int languageIndex = ui->languageComboBox->currentIndex();

        if (LANGUAGE_ENGLISH == languageIndex) {
            settingFile.setValue(LANGUAGE_INDEX , LANGUAGE_ENGLISH);
        }
        else {
            settingFile.setValue(LANGUAGE_INDEX , LANGUAGE_CHINESE);
        }

        if (cursettingSelectStr.startsWith(GROUPNAME_CUSTOMSETTING, Qt::CaseInsensitive)
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
                if (group.startsWith(GROUPNAME_CUSTOMSETTING, Qt::CaseInsensitive)
                        && group.endsWith(GROUPNAME_EXECUTABLE_SUFFIX, Qt::CaseInsensitive) != true) {
                    validgroups_customsetting.append(group);
                }
            }

            int customSettingIndex;
            if (validgroups_customsetting.size() > 0) {
                QString lastgroup = validgroups_customsetting.last();
                QString lastNumberStr = lastgroup.remove(GROUPNAME_CUSTOMSETTING);

                if (lastNumberStr.toInt() >= CUSTOMSETTING_INDEX_MAX) {
                    QString message = "Too big custom setting number " + lastNumberStr + ", please remove some custom settings!";
                    QMessageBox::warning(this, tr("QKeyMapper"), tr(message.toStdString().c_str()));
                    return;
                }
                else {
                    customSettingIndex = lastNumberStr.toInt() + 1;
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[saveKeyMapSetting] Last custom setting index ->" << lastNumberStr.toInt();
                qDebug() << "[saveKeyMapSetting] Save current custom setting to ->" << customSettingIndex;
#endif
            }
            else {
                customSettingIndex = 1;
            }

            if ((false == ui->nameLineEdit->text().isEmpty())
                    && (false == ui->titleLineEdit->text().isEmpty())
                    && (true == ui->nameCheckBox->isChecked())
                    /*&& (true == ui->titleCheckBox->isChecked())*/
                    && (ui->nameLineEdit->text() == m_MapProcessInfo.FileName)
                    /*&& (ui->titleLineEdit->text() == m_MapProcessInfo.WindowTitle)*/
                    && (m_MapProcessInfo.FilePath.isEmpty() != true)){
                settingFile.setValue(SETTINGSELECT , m_MapProcessInfo.FileName);
                saveSettingSelectStr = m_MapProcessInfo.FileName + "/";
            }
            else {
                if (customSettingIndex < 10) {
                    saveSettingSelectStr = QString(GROUPNAME_CUSTOMSETTING) + " " + QString::number(customSettingIndex);
                }
                else {
                    saveSettingSelectStr = QString(GROUPNAME_CUSTOMSETTING) + QString::number(customSettingIndex);
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
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTPRESS_TIME , burstpressTimeString  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME , burstreleaseTimeString  );

            settingFile.remove(saveSettingSelectStr+CLEARALL);
        }
        else{
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_ORIGINALKEYS, original_keys );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_MAPPINGKEYS , mapping_keysList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURST , burstList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_LOCK , lockList  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTPRESS_TIME , burstpressTimeString  );
            settingFile.setValue(saveSettingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME , burstreleaseTimeString  );
            settingFile.setValue(saveSettingSelectStr+CLEARALL, QString("ClearList"));
        }

        if ((false == ui->nameLineEdit->text().isEmpty())
                && (false == ui->titleLineEdit->text().isEmpty())
                && (ui->nameLineEdit->text() == m_MapProcessInfo.FileName)
                && (ui->titleLineEdit->text() == m_MapProcessInfo.WindowTitle)){
            settingFile.setValue(saveSettingSelectStr+PROCESSINFO_FILENAME, m_MapProcessInfo.FileName);
            settingFile.setValue(saveSettingSelectStr+PROCESSINFO_WINDOWTITLE, m_MapProcessInfo.WindowTitle);

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
            settingFile.setValue(saveSettingSelectStr+DISABLEWINKEY_CHECKED, ui->disableWinKeyCheckBox->isChecked());
            settingFile.setValue(saveSettingSelectStr+AUTOSTARTMAPPING_CHECKED, ui->autoStartMappingCheckBox->isChecked());
        }
        else{
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[saveKeyMapSetting]" << "Unmatch display processinfo & stored processinfo.";
#endif
        }

        const QString savedSettingName = saveSettingSelectStr.remove("/");
        bool loadresult = loadKeyMapSetting(savedSettingName);
        Q_UNUSED(loadresult);
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
        QMessageBox::warning(this, tr("QKeyMapper"), tr("Invalid KeyMap Data."));
    }
}

bool QKeyMapper::loadKeyMapSetting(const QString &settingtext)
{
    bool loadDefault = false;
    bool clearallcontainsflag = true;
    bool selectSettingContainsFlag = false;
    quint8 datavalidflag = 0xFF;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    settingFile.setIniCodec("UTF-8");
#endif

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

    QString settingSelectStr;

    ui->settingselectComboBox->clear();
    ui->settingselectComboBox->addItem(QString());
    QStringList groups = settingFile.childGroups();
    QStringList validgroups;
    QStringList validgroups_fullmatch;
    QStringList validgroups_customsetting;
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[loadKeyMapSetting]" << "childGroups >>" << groups;
#endif
    for (const QString &group : qAsConst(groups)){
        bool valid_setting = false;
        if (group.endsWith(GROUPNAME_EXECUTABLE_SUFFIX, Qt::CaseInsensitive)) {
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
        if (group.startsWith(GROUPNAME_CUSTOMSETTING, Qt::CaseInsensitive)
                && group.endsWith(GROUPNAME_EXECUTABLE_SUFFIX, Qt::CaseInsensitive) != true) {
            valid_setting = true;
        }

        if (true == valid_setting) {
            ui->settingselectComboBox->addItem(group);
            validgroups_customsetting.append(group);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting] Setting select add CustomSetting ->" << group;
#endif
        }
    }

    validgroups = validgroups_fullmatch + validgroups_customsetting;

    if (true == settingtext.isEmpty()) {
        if (true == settingFile.contains(SETTINGSELECT)){
            QVariant settingSelect = settingFile.value(SETTINGSELECT);
            if (settingSelect.canConvert(QMetaType::QString)) {
                settingSelectStr = settingSelect.toString();
            }

            if (false == validgroups.contains(settingSelectStr)) {
#ifdef DEBUG_LOGOUT_ON
                qWarning() << "[loadKeyMapSetting] Invalid setting select name ->" << settingSelectStr;
#endif
                settingSelectStr.clear();
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
#ifdef DEBUG_LOGOUT_ON
                qWarning() << "[loadKeyMapSetting] Invalid setting select name ->" << settingSelectStr;
#endif
                settingSelectStr.clear();
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
    }
    else {
        settingSelectStr.clear();
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
                        bool checkmappingstr = checkMappingkeyStr(mapping_keys.at(loadindex));
                        if ((true == keyboardmapcontains || true == mousemapcontains)
                                && (true == checkmappingstr)){
                            loadkeymapdata.append(MAP_KEYDATA(ori_key, mapping_keys.at(loadindex), burstList.at(loadindex), lockList.at(loadindex)));
                        }
                        else{
                            datavalidflag = false;
#ifdef DEBUG_LOGOUT_ON
                            qWarning("[loadKeyMapSetting] Invalid data loaded -> keyboardmapcontains(%s), mousemapcontains(%s), checkmappingstr(%s)", keyboardmapcontains?"true":"false", mousemapcontains?"true":"false", checkmappingstr?"true":"false");
#endif
                            break;
                        }

                        loadindex += 1;
                    }
                }
            }
        }

        if (datavalidflag != (quint8)true){
            KeyMappingDataList.append(MAP_KEYDATA("I",          "L-Shift + ]}",     false,  false));
            KeyMappingDataList.append(MAP_KEYDATA("K",          "L-Shift + [{",     false,  false));
            KeyMappingDataList.append(MAP_KEYDATA("H",          "S",                false,  false));
            KeyMappingDataList.append(MAP_KEYDATA("Space",      "S",                false,  false));
            KeyMappingDataList.append(MAP_KEYDATA("F",          "Enter",            false,  false));
            loadDefault = true;
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
        ui->iconLabel->clear();
    }

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

    if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURSTPRESS_TIME)){
        QString burstpressTimeString = settingFile.value(settingSelectStr+KEYMAPDATA_BURSTPRESS_TIME).toString();
        if (false == burstpressTimeString.isEmpty()) {
            ui->burstpressComboBox->setCurrentText(burstpressTimeString);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "BurstPressTime =" << burstpressTimeString;
#endif
    }
    else {
        ui->burstpressComboBox->setCurrentText(QString("40"));
    }

    if (true == settingFile.contains(settingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME)){
        QString burstreleaseTimeString = settingFile.value(settingSelectStr+KEYMAPDATA_BURSTRELEASE_TIME).toString();
        if (false == burstreleaseTimeString.isEmpty()) {
            ui->burstreleaseComboBox->setCurrentText(burstreleaseTimeString);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "BurstReleaseTime =" << burstreleaseTimeString;
#endif
    }
    else {
        ui->burstreleaseComboBox->setCurrentText(QString("20"));
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

    bool autoStartMappingChecked = false;
    if (true == settingFile.contains(settingSelectStr+AUTOSTARTMAPPING_CHECKED)){
        autoStartMappingChecked = settingFile.value(settingSelectStr+AUTOSTARTMAPPING_CHECKED).toBool();
        if (true == autoStartMappingChecked) {
            ui->autoStartMappingCheckBox->setChecked(true);
        }
        else {
            ui->autoStartMappingCheckBox->setChecked(false);
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << "AutoStartMappingChecked =" << autoStartMappingChecked;
#endif
    }
    else {
        ui->autoStartMappingCheckBox->setChecked(false);
    }

    if (false == datavalidflag){
        QMessageBox::warning(this, tr("QKeyMapper"), tr("<html><head/><body><p align=\"center\">Load invalid keymapdata from ini file.</p><p align=\"center\">Reset to default values.</p></body></html>"));
        return false;
    }
    else{
        if ((true == autoStartMappingChecked) && (true == settingtext.isEmpty())) {
            on_keymapButton_clicked();
        }

        if (settingSelectStr.isEmpty() != true) {
            settingSelectStr = settingSelectStr.remove("/");
            ui->settingselectComboBox->setCurrentText(settingSelectStr);
        }
        return true;
    }
}

bool QKeyMapper::checkMappingkeyStr(const QString &mappingkeystr)
{
    bool checkResult = true;
    static QRegularExpression regexp("\\s[+»]\\s");
    QStringList Mapping_Keys = mappingkeystr.split(regexp, Qt::SkipEmptyParts);
    for (const QString &mapping_key : qAsConst(Mapping_Keys)){
        if (false == QKeyMapper_Worker::VirtualKeyCodeMap.contains(mapping_key)
            && false == QKeyMapper_Worker::VirtualMouseButtonMap.contains(mapping_key)){
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
    ui->refreshButton->setFont(customFont);
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

    if (UI_SCALE_2K_PERCENT_100 == m_UI_Scale) {
        customFont.setPointSize(14);
    }
    else {
        customFont.setPointSize(18);
    }
    ui->refreshButton->setFont(customFont);
    ui->keymapButton->setFont(customFont);
    ui->savemaplistButton->setFont(customFont);

    if (UI_SCALE_2K_PERCENT_100 == m_UI_Scale) {
        customFont.setPointSize(9);
    }
    else {
        customFont.setPointSize(11);
    }
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

    if (UI_SCALE_2K_PERCENT_100 == m_UI_Scale) {
        customFont.setPointSize(10);
    }
    else {
        customFont.setPointSize(12);
    }
    ui->disableWinKeyCheckBox->setFont(customFont);
    ui->autoStartMappingCheckBox->setFont(customFont);
    ui->autoStartupCheckBox->setFont(customFont);
}

void QKeyMapper::setControlFontChinese()
{
    QFont customFont(FONTNAME_CHINESE, 18, QFont::Bold);

    if (UI_SCALE_2K_PERCENT_100 == m_UI_Scale) {
        customFont.setPointSize(16);
    }
    else {
        customFont.setPointSize(18);
    }
    ui->refreshButton->setFont(customFont);
    ui->keymapButton->setFont(customFont);
    ui->savemaplistButton->setFont(customFont);

    if (UI_SCALE_2K_PERCENT_100 == m_UI_Scale) {
        customFont.setPointSize(9);
    }
    else {
        customFont.setPointSize(11);
    }
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

    if (UI_SCALE_2K_PERCENT_100 == m_UI_Scale) {
        customFont.setPointSize(10);
    }
    else {
        customFont.setPointSize(12);
    }
    ui->disableWinKeyCheckBox->setFont(customFont);
    ui->autoStartMappingCheckBox->setFont(customFont);
    ui->autoStartupCheckBox->setFont(customFont);
}

void QKeyMapper::changeControlEnableStatus(bool status)
{
    ui->nameCheckBox->setEnabled(status);
    ui->titleCheckBox->setEnabled(status);
    ui->disableWinKeyCheckBox->setEnabled(status);
    ui->autoStartMappingCheckBox->setEnabled(status);
    ui->autoStartupCheckBox->setEnabled(status);
    ui->languageComboBox->setEnabled(status);
    ui->burstpressComboBox->setEnabled(status);
    ui->burstreleaseComboBox->setEnabled(status);
    ui->settingselectComboBox->setEnabled(status);
    //ui->nameLineEdit->setEnabled(status);
    //ui->titleLineEdit->setEnabled(status);

    ui->orikeyLabel->setEnabled(status);
    ui->mapkeyLabel->setEnabled(status);
    m_orikeyComboBox->setEnabled(status);
    m_mapkeyComboBox->setEnabled(status);
    ui->addmapdataButton->setEnabled(status);
    ui->deleteoneButton->setEnabled(status);
    ui->clearallButton->setEnabled(status);
    ui->removeSettingButton->setEnabled(status);
    ui->moveupButton->setEnabled(status);
    ui->movedownButton->setEnabled(status);
    ui->nextarrowCheckBox->setEnabled(status);

    ui->refreshButton->setEnabled(status);
    ui->savemaplistButton->setEnabled(status);

    ui->processinfoTable->setEnabled(status);
    ui->keymapdataTable->setEnabled(status);
}

void QKeyMapper::extractSoundFiles()
{
    QFile soundFileStartLocal(SOUNDFILE_START);
    if (false == soundFileStartLocal.exists()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractSoundFiles]" << "Local sound file QKeyMapper.wav do not exist";
#endif
        QFile soundFileStartQRC(SOUNDFILE_START_QRC);
        if (soundFileStartQRC.exists()){
            soundFileStartQRC.copy(SOUNDFILE_START);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning() << "[extractSoundFiles]" << "QRC sound file QKeyMapper.wav do not exist!!!";
#endif
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[extractSoundFiles]" << "Local sound file QKeyMapper.wav already exist";
#endif
    }
}

void QKeyMapper::playStartSound()
{
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

void QKeyMapper::on_savemaplistButton_clicked()
{
    saveKeyMapSetting();
}

void QKeyMapper::initHotKeySequence()
{
    connect( m_HotKey, &QHotkey::activated, this, &QKeyMapper::HotKeyActivated);

    QKeySequence hotkeysequence = QKeySequence::fromString("Ctrl+`");
    m_HotKey->setShortcut(hotkeysequence, true);
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

        ui->processinfoTable->setItem(rowindex, 0, new QTableWidgetItem(processinfo.WindowIcon, processinfo.FileName));
        ui->processinfoTable->setItem(rowindex, 1, new QTableWidgetItem(processinfo.PID));
        //ui->processinfoTable->item(rowindex, 1)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->processinfoTable->setItem(rowindex, 2, new QTableWidgetItem(processinfo.WindowTitle));

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
    else if (KEYMAP_MAPPING == m_KeyMapStatus) {
        m_SysTrayIcon->setIcon(QIcon(":/QKeyMapper_mapping.ico"));
        m_SysTrayIcon->setToolTip("QKeyMapper(Mapping : " + m_MapProcessInfo.FileName + ")");
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

    int original_key_width = ui->keymapdataTable->width()/5;
    int burst_mode_width = ui->keymapdataTable->width()/5 - 35;
    int lock_width = ui->keymapdataTable->width()/5 - 20;
    int mapping_key_width = ui->keymapdataTable->width() - original_key_width - burst_mode_width - lock_width - 2;
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
            << "L-Mouse"
            << "R-Mouse"
            << "M-Mouse"
            << "X1-Mouse"
            << "X2-Mouse"
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
            << "Num Enter";

    m_orikeyComboBox->setObjectName(QStringLiteral("orikeyComboBox"));
    m_orikeyComboBox->setGeometry(QRect(607, 390, 82, 22));
    m_mapkeyComboBox->setObjectName(QStringLiteral("mapkeyComboBox"));
    m_mapkeyComboBox->setGeometry(QRect(755, 390, 82, 22));

    QStringList orikeycodelist = keycodelist;
    m_orikeyComboBox->addItems(orikeycodelist);
    m_mapkeyComboBox->addItems(keycodelist);
}

void QKeyMapper::refreshKeyMappingDataTable()
{
    ui->keymapdataTable->clearContents();
    ui->keymapdataTable->setRowCount(0);

    if (false == KeyMappingDataList.isEmpty()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "KeyMappingDataList Start >>>";
#endif
        int rowindex = 0;
        ui->keymapdataTable->setRowCount(KeyMappingDataList.size());
        for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
        {
            /* ORIGINAL_KEY_COLUMN */
            ui->keymapdataTable->setItem(rowindex, ORIGINAL_KEY_COLUMN  , new QTableWidgetItem(keymapdata.Original_Key));

            /* MAPPING_KEY_COLUMN */
            QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
            ui->keymapdataTable->setItem(rowindex, MAPPING_KEY_COLUMN   , new QTableWidgetItem(mappingkeys_str));

            /* BURST_MODE_COLUMN */
            QTableWidgetItem *burstCheckBox = new QTableWidgetItem();
            if (keymapdata.Burst == true) {
                burstCheckBox->setCheckState(Qt::Checked);
            }
            else {
                burstCheckBox->setCheckState(Qt::Unchecked);
            }
            ui->keymapdataTable->setItem(rowindex, BURST_MODE_COLUMN    , burstCheckBox);

            /* LOCK_COLUMN */
            QTableWidgetItem *lockCheckBox = new QTableWidgetItem();
            if (keymapdata.Lock == true) {
                lockCheckBox->setCheckState(Qt::Checked);
            }
            else {
                lockCheckBox->setCheckState(Qt::Unchecked);
            }
            ui->keymapdataTable->setItem(rowindex, LOCK_COLUMN    , lockCheckBox);

            rowindex += 1;

#ifdef DEBUG_LOGOUT_ON
            qDebug() << keymapdata.Original_Key << "to" << keymapdata.Mapping_Keys;
#endif
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "KeyMappingDataList End   <<<";
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

}

void QKeyMapper::setUILanguage_Chinese()
{
    setControlFontChinese();

    if (KEYMAP_CHECKING == m_KeyMapStatus || KEYMAP_MAPPING == m_KeyMapStatus) {
        ui->keymapButton->setText(KEYMAPBUTTON_STOP_CHINESE);
    }
    else {
        ui->keymapButton->setText(KEYMAPBUTTON_START_CHINESE);
    }

    ui->refreshButton->setText(REFRESHBUTTON_CHINESE);
    ui->savemaplistButton->setText(SAVEMAPLISTBUTTON_CHINESE);
    ui->deleteoneButton->setText(DELETEONEBUTTON_CHINESE);
    ui->clearallButton->setText(CLEARALLBUTTON_CHINESE);
    ui->addmapdataButton->setText(ADDMAPDATABUTTON_CHINESE);
    ui->nameCheckBox->setText(NAMECHECKBOX_CHINESE);
    ui->titleCheckBox->setText(TITLECHECKBOX_CHINESE);
    ui->orikeyLabel->setText(ORIKEYLABEL_CHINESE);
    ui->mapkeyLabel->setText(MAPKEYLABEL_CHINESE);
    ui->burstpressLabel->setText(BURSTPRESSLABEL_CHINESE);
    ui->burstreleaseLabel->setText(BURSTRELEASE_CHINESE);
    ui->burstpress_msLabel->setText(BURSTPRESS_MSLABEL_CHINESE);
    ui->burstrelease_msLabel->setText(BURSTRELEASE_MSLABEL_CHINESE);
    ui->settingselectLabel->setText(SETTINGSELECTLABEL_CHINESE);
    ui->removeSettingButton->setText(REMOVESETTINGBUTTON_CHINESE);
    ui->disableWinKeyCheckBox->setText(DISABLEWINKEYCHECKBOX_CHINESE);
    ui->autoStartMappingCheckBox->setText(AUTOSTARTMAPPINGCHECKBOX_CHINESE);
    ui->autoStartupCheckBox->setText(AUTOSTARTUPCHECKBOX_CHINESE);

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

    if (KEYMAP_CHECKING == m_KeyMapStatus || KEYMAP_MAPPING == m_KeyMapStatus) {
        ui->keymapButton->setText(KEYMAPBUTTON_STOP_ENGLISH);
    }
    else {
        ui->keymapButton->setText(KEYMAPBUTTON_START_ENGLISH);
    }
    ui->refreshButton->setText(REFRESHBUTTON_ENGLISH);
    ui->savemaplistButton->setText(SAVEMAPLISTBUTTON_ENGLISH);
    ui->deleteoneButton->setText(DELETEONEBUTTON_ENGLISH);
    ui->clearallButton->setText(CLEARALLBUTTON_ENGLISH);
    ui->addmapdataButton->setText(ADDMAPDATABUTTON_ENGLISH);
    ui->nameCheckBox->setText(NAMECHECKBOX_ENGLISH);
    ui->titleCheckBox->setText(TITLECHECKBOX_ENGLISH);
    ui->orikeyLabel->setText(ORIKEYLABEL_ENGLISH);
    ui->mapkeyLabel->setText(MAPKEYLABEL_ENGLISH);
    ui->burstpressLabel->setText(BURSTPRESSLABEL_ENGLISH);
    ui->burstreleaseLabel->setText(BURSTRELEASE_ENGLISH);
    ui->burstpress_msLabel->setText(BURSTPRESS_MSLABEL_ENGLISH);
    ui->burstrelease_msLabel->setText(BURSTRELEASE_MSLABEL_ENGLISH);
    ui->settingselectLabel->setText(SETTINGSELECTLABEL_ENGLISH);
    ui->removeSettingButton->setText(REMOVESETTINGBUTTON_ENGLISH);
    ui->disableWinKeyCheckBox->setText(DISABLEWINKEYCHECKBOX_ENGLISH);
    ui->autoStartMappingCheckBox->setText(AUTOSTARTMAPPINGCHECKBOX_ENGLISH);
    ui->autoStartupCheckBox->setText(AUTOSTARTUPCHECKBOX_ENGLISH);

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

    if (UI_SCALE_2K_PERCENT_100 == m_UI_Scale) {
        QFontInfo fontinfo = ui->nameLineEdit->fontInfo();
        if (fontinfo.pointSize() > 10) {
            ui->nameLineEdit->setFont(QFont(fontinfo.family(), 10));
        }

        fontinfo = ui->titleLineEdit->fontInfo();
        if (fontinfo.pointSize() > 10) {
            ui->titleLineEdit->setFont(QFont(fontinfo.family(), 10));
        }

        fontinfo = ui->languageComboBox->fontInfo();
        if (fontinfo.pointSize() > 10) {
            ui->languageComboBox->setFont(QFont(fontinfo.family(), 10));
        }

        fontinfo = ui->burstpressComboBox->fontInfo();
        if (fontinfo.pointSize() > 10) {
            ui->burstpressComboBox->setFont(QFont(fontinfo.family(), 10));
        }

        fontinfo = ui->burstreleaseComboBox->fontInfo();
        if (fontinfo.pointSize() > 10) {
            ui->burstreleaseComboBox->setFont(QFont(fontinfo.family(), 10));
        }

        fontinfo = m_orikeyComboBox->fontInfo();
        if (fontinfo.pointSize() > 10) {
            m_orikeyComboBox->setFont(QFont(fontinfo.family(), 10));
        }

        fontinfo = m_mapkeyComboBox->fontInfo();
        if (fontinfo.pointSize() > 10) {
            m_mapkeyComboBox->setFont(QFont(fontinfo.family(), 10));
        }

        fontinfo = ui->settingselectComboBox->fontInfo();
        if (fontinfo.pointSize() > 10) {
            ui->settingselectComboBox->setFont(QFont(fontinfo.family(), 10));
        }

        fontinfo = ui->processinfoTable->fontInfo();
        if (fontinfo.pointSize() > 10) {
            ui->processinfoTable->setFont(QFont(fontinfo.family(), 10));
        }

        fontinfo = ui->keymapdataTable->fontInfo();
        if (fontinfo.pointSize() > 10) {
            ui->keymapdataTable->setFont(QFont(fontinfo.family(), 10));
        }
    }
}

void QKeyMapper::updateLockStatusDisplay()
{
    int rowindex = 0;
    for (const MAP_KEYDATA &keymapdata : qAsConst(KeyMappingDataList))
    {
        if (m_KeyMapStatus == KEYMAP_MAPPING) {
            if (keymapdata.Lock == true) {
                if (keymapdata.LockStatus == true) {
                    ui->keymapdataTable->item(rowindex, LOCK_COLUMN)->setText("ON");
                    ui->keymapdataTable->item(rowindex, LOCK_COLUMN)->setForeground(Qt::magenta);
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

void QKeyMapper::on_refreshButton_clicked()
{
    ui->processinfoTable->clearContents();
    ui->processinfoTable->setRowCount(0);
    refreshProcessInfoTable();
}

void QKeyMapper::on_processinfoTable_doubleClicked(const QModelIndex &index)
{
    if ((KEYMAP_IDLE == m_KeyMapStatus)
            && (true == ui->nameLineEdit->isEnabled())
            && (true == ui->titleLineEdit->isEnabled())){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[SelectProcessInfo]" << "Table DoubleClicked" << index.row() << ui->processinfoTable->item(index.row(), 0)->text() << ui->processinfoTable->item(index.row(), 2)->text();
#endif

        ui->nameLineEdit->setText(ui->processinfoTable->item(index.row(), 0)->text());
        ui->titleLineEdit->setText(ui->processinfoTable->item(index.row(), 2)->text());
        ui->settingselectComboBox->setCurrentText(QString());

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
    if ((true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(m_orikeyComboBox->currentText())
            || true == QKeyMapper_Worker::VirtualMouseButtonMap.contains(m_orikeyComboBox->currentText()))
        && (true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(m_mapkeyComboBox->currentText())
            || true == QKeyMapper_Worker::VirtualMouseButtonMap.contains(m_mapkeyComboBox->currentText()))){
        bool already_exist = false;
        int findindex = findInKeyMappingDataList(m_orikeyComboBox->currentText());

        if (findindex != -1){
            if (KeyMappingDataList.at(findindex).Mapping_Keys.size() == 1
                    && false == ui->nextarrowCheckBox->isChecked()
                    && KeyMappingDataList.at(findindex).Mapping_Keys.contains(m_mapkeyComboBox->currentText()) == true){
                already_exist = true;
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "KeyMap already exist at KeyMappingDataList index : " << findindex;
#endif
            }
        }

        if (false == already_exist){
            if (findindex != -1){
                MAP_KEYDATA keymapdata = KeyMappingDataList.at(findindex);
                if (keymapdata.Mapping_Keys.size() >= KEY_SEQUENCE_MAX) {
                    QString message = QString("Key sequence mapping to \"%1\" is too long!").arg(m_orikeyComboBox->currentText());
                    QMessageBox::warning(this, tr("QKeyMapper"), tr(message.toStdString().c_str()));
                    return;
                }
                QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "mappingkeys_str before add:" << mappingkeys_str;
#endif
                if (ui->nextarrowCheckBox->isChecked()) {
                    mappingkeys_str = mappingkeys_str + SEPARATOR_NEXTARROW + m_mapkeyComboBox->currentText();
                }
                else {
                    mappingkeys_str = mappingkeys_str + SEPARATOR_PLUS + m_mapkeyComboBox->currentText();
                }

#ifdef DEBUG_LOGOUT_ON
                qDebug() << "mappingkeys_str after add:" << mappingkeys_str;
#endif
                KeyMappingDataList.replace(findindex, MAP_KEYDATA(m_orikeyComboBox->currentText(), mappingkeys_str, keymapdata.Burst, keymapdata.Lock));
            }
            else {
                KeyMappingDataList.append(MAP_KEYDATA(m_orikeyComboBox->currentText(), m_mapkeyComboBox->currentText(), false, false));
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "Add keymapdata :" << m_orikeyComboBox->currentText() << "to" << m_mapkeyComboBox->currentText();
#endif
            }

#ifdef DEBUG_LOGOUT_ON
            qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
            refreshKeyMappingDataTable();
        }
        else{
            QMessageBox::warning(this, tr("QKeyMapper"), tr("Conflict with exist Keys."));
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

void QKeyMapper::on_moveupButton_clicked()
{
    int currentrowindex = -1;
    QList<QTableWidgetItem*> items = ui->keymapdataTable->selectedItems();
    if (false == items.empty() && items.size() == KEYMAPPINGDATA_TABLE_COLUMN_COUNT) {
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
    if (false == items.empty() && items.size() == KEYMAPPINGDATA_TABLE_COLUMN_COUNT) {
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

void QKeyMapper::on_settingselectComboBox_textActivated(const QString &text)
{
    if (false == text.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[settingselectComboBox] Change to Setting ->" << text;
#endif
        bool loadresult = loadKeyMapSetting(text);
        Q_UNUSED(loadresult);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[settingselectComboBox] Select setting text error ->" << text;
#endif
    }
}


void QKeyMapper::on_removeSettingButton_clicked()
{
    QString settingSelectStr = ui->settingselectComboBox->currentText();
    if (true == settingSelectStr.isEmpty()) {
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
            bool loadresult = loadKeyMapSetting(currentSettingText);
            Q_UNUSED(loadresult);
        }
    }
}


void QKeyMapper::on_autoStartupCheckBox_stateChanged(int state)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[autoStartup] Auto startup state changed ->" << (Qt::CheckState)state;
#endif
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    if (Qt::Checked == state) {
        settingFile.setValue(AUTO_STARTUP , true);

        std::wstring operate = QString("runas").toStdWString();
        std::wstring executable = QString("schtasks").toStdWString();
        std::wstring argument = QString("/create /f /sc onlogon /rl highest /tn QKeyMapper /tr " + QCoreApplication::applicationFilePath()).toStdWString();
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

        std::wstring operate = QString("runas").toStdWString();
        std::wstring executable = QString("schtasks").toStdWString();
        std::wstring argument = QString("/delete /f /tn QKeyMapper").toStdWString();
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

