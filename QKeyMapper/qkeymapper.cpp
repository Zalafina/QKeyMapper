#include "qkeymapper.h"
#include "ui_qkeymapper.h"

//static const uint WIN_TITLESTR_MAX = 200U;
static const uint CYCLE_CHECK_TIMEOUT = 500U;
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

static const int DEFAULT_ICON_WIDTH = 64;
static const int DEFAULT_ICON_HEIGHT = 64;

static const int MOUSEWHEEL_SCROLL_NONE = 0;
static const int MOUSEWHEEL_SCROLL_UP = 1;
static const int MOUSEWHEEL_SCROLL_DOWN = 2;

static const int KEY_UP = 0;
static const int KEY_DOWN = 1;

static const int SENDMODE_HOOK          = 0;
static const int SENDMODE_BURST_NORMAL  = 1;
static const int SENDMODE_BURST_STOP    = 2;

static const int SEND_INPUTS_MAX = 20;

static const ULONG_PTR VIRTUAL_KEYBOARD_PRESS = 0xACBDACBD;
static const ULONG_PTR VIRTUAL_MOUSE_CLICK = 0xCEDFCEDF;

static const QString DEFAULT_NAME("ForzaHorizon4.exe");
static QString DEFAULT_TITLE("Forza: Horizon 4");

static const QString SETTINGSELECT("SettingSelect");

static const QString KEYMAPDATA_ORIGINALKEYS("KeyMapData_OriginalKeys");
static const QString KEYMAPDATA_MAPPINGKEYS("KeyMapData_MappingKeys");
static const QString KEYMAPDATA_BURST("KeyMapData_Burst");
static const QString KEYMAPDATA_LOCK("KeyMapData_Lock");
static const QString KEYMAPDATA_BURSTPRESS_TIME("KeyMapData_BurstPressTime");
static const QString KEYMAPDATA_BURSTRELEASE_TIME("KeyMapData_BurstReleaseTime");
static const QString CLEARALL("KeyMapData_ClearAll");

static const QString PROCESSINFO_FILENAME("ProcessInfo_FileName");
static const QString PROCESSINFO_WINDOWTITLE("ProcessInfo_WindowTitle");
static const QString PROCESSINFO_FILEPATH("ProcessInfo_FilePath");
static const QString PROCESSINFO_FILENAME_CHECKED("ProcessInfo_FileNameChecked");
static const QString PROCESSINFO_WINDOWTITLE_CHECKED("ProcessInfo_WindowTitleChecked");

static const QString DISABLEWINKEY_CHECKED("DisableWinKeyChecked");
static const QString AUTOSTARTMAPPING_CHECKED("AutoStartMappingChecked");

static const QString SAO_FONTFILENAME(":/sao_ui.otf");

QList<MAP_PROCESSINFO> QKeyMapper::static_ProcessInfoList = QList<MAP_PROCESSINFO>();
QHash<QString, V_KEYCODE> QKeyMapper::VirtualKeyCodeMap = QHash<QString, V_KEYCODE>();
QHash<QString, V_MOUSECODE> QKeyMapper::VirtualMouseButtonMap = QHash<QString, V_MOUSECODE>();
QHash<WPARAM, QString> QKeyMapper::MouseButtonNameMap = QHash<WPARAM, QString>();
QList<MAP_KEYDATA> QKeyMapper::KeyMappingDataList = QList<MAP_KEYDATA>();
QStringList QKeyMapper::pressedRealKeysList = QStringList();
QStringList QKeyMapper::pressedVirtualKeysList = QStringList();
QStringList QKeyMapper::pressedLockKeysList = QStringList();
GetDeviceStateT QKeyMapper::FuncPtrGetDeviceState = Q_NULLPTR;
GetDeviceDataT QKeyMapper::FuncPtrGetDeviceData = Q_NULLPTR;
QComboBox *QKeyMapper::orikeyComboBox_static = Q_NULLPTR;
QComboBox *QKeyMapper::mapkeyComboBox_static = Q_NULLPTR;
QCheckBox *QKeyMapper::disableWinKeyCheckBox_static = Q_NULLPTR;
QKeyMapper *QKeyMapper::selfPtr_static = Q_NULLPTR;

QKeyMapper::QKeyMapper(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QKeyMapper),
    m_KeyMapStatus(KEYMAP_IDLE),
    m_CycleCheckTimer(this),
    m_MapProcessInfo(),
    m_SysTrayIcon(Q_NULLPTR),
    m_KeyHook(Q_NULLPTR),
#ifdef SUPPORT_MOUSE_LL_HOOK
    m_MouseHook(Q_NULLPTR),
#endif
    m_DirectInput(Q_NULLPTR),
    m_SAO_FontFamilyID(-1),
    m_SAO_FontName(),
    m_ProcessInfoTableDelegate(Q_NULLPTR),
    m_KeyMappingDataTableDelegate(Q_NULLPTR),
    m_orikeyComboBox(new KeyListComboBox(this)),
    m_mapkeyComboBox(new KeyListComboBox(this)),
    m_HotKey(new QHotkey(this)),
    m_BurstTimerMap(),
    m_BurstKeyUpTimerMap()
{
    ui->setupUi(this);
    initAddKeyComboBoxes();
    orikeyComboBox_static = m_orikeyComboBox;
    mapkeyComboBox_static = m_mapkeyComboBox;
    disableWinKeyCheckBox_static = ui->disableWinKeyCheckBox;
    selfPtr_static = this;
    loadFontFile(SAO_FONTFILENAME, m_SAO_FontFamilyID, m_SAO_FontName);
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

    if ((m_SAO_FontFamilyID != -1)
            && (false == m_SAO_FontName.isEmpty())){
        setControlCustomFont(m_SAO_FontName);
    }

    // set QTableWidget selected background-color
    setStyleSheet("QTableWidget::item:selected { background-color: rgb(190, 220, 255) }");

    ui->iconLabel->setStyle(QStyleFactory::create("windows"));
    setMapProcessInfo(QString(DEFAULT_NAME), QString(DEFAULT_TITLE), QString(), QString(), QIcon(":/DefaultIcon.ico"));
    ui->nameCheckBox->setChecked(true);
    ui->titleCheckBox->setChecked(true);

    ui->moveupButton->setFont(QFont("SimSun", 14));
    ui->movedownButton->setFont(QFont("SimSun", 16));

    initVirtualKeyCodeMap();
    initVirtualMouseButtonMap();
    initProcessInfoTable();
    ui->nameCheckBox->setFocusPolicy(Qt::NoFocus);
    ui->titleCheckBox->setFocusPolicy(Qt::NoFocus);
    ui->nameLineEdit->setFocusPolicy(Qt::NoFocus);
    ui->titleLineEdit->setFocusPolicy(Qt::NoFocus);

    m_SysTrayIcon = new QSystemTrayIcon(this);
#ifdef USE_SABER_ICON
    m_SysTrayIcon->setIcon(QIcon(":/AppIcon_Saber.ico"));
#else
    m_SysTrayIcon->setIcon(QIcon(":/AppIcon.ico"));
#endif
    m_SysTrayIcon->setToolTip("QKeyMapper(Idle)");
    m_SysTrayIcon->show();

    initKeyMappingDataTable();
    bool loadresult = loadKeyMapSetting(0);
    Q_UNUSED(loadresult);

    QObject::connect(m_SysTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(SystrayIconActivated(QSystemTrayIcon::ActivationReason)));
    QObject::connect(&m_CycleCheckTimer, SIGNAL(timeout()), this, SLOT(cycleCheckProcessProc()));
    QObject::connect(ui->keymapdataTable, SIGNAL(cellChanged(int,int)), this, SLOT(on_cellChanged(int,int)));

    QObject::connect(this, SIGNAL(startBurstTimer_Signal(QString,int)), this, SLOT(startBurstTimer(QString,int)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(stopBurstTimer_Signal(QString,int)), this, SLOT(stopBurstTimer(QString,int)), Qt::QueuedConnection);
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
    setKeyUnHook();

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

        if (checkresult){
            if (KEYMAP_CHECKING == m_KeyMapStatus){
                setKeyHook(hwnd);
                //setDInputKeyHook(hwnd);
                m_KeyMapStatus = KEYMAP_MAPPING;
                emit updateLockStatus_Signal();

#ifdef DEBUG_LOGOUT_ON
                if (1 == checkresult) {
                    qDebug().nospace().noquote() << "[cycleCheckProcessProc]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") " << "ForegroundWindow: " << windowTitle << "(" << filename << ")";
                }
                else {
                    qDebug().nospace().noquote() << "[cycleCheckProcessProc]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ")";
                }
#endif
            }
        }
        else{
            if (KEYMAP_MAPPING == m_KeyMapStatus){
                setKeyUnHook();
                setDInputKeyUnHook();
                m_KeyMapStatus = KEYMAP_CHECKING;
                emit updateLockStatus_Signal();

#ifdef DEBUG_LOGOUT_ON
                if (1 == checkresult) {
                    qDebug().nospace() << "[cycleCheckProcessProc]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ") " << "ForegroundWindow: " << windowTitle << "(" << filename << ")";
                }
                else {
                    qDebug().nospace() << "[cycleCheckProcessProc]" << " KeyMapStatus change (" << keymapstatusEnum.valueToKey(m_KeyMapStatus) << ")";
                }
#endif
            }
        }
    }
    else{
        //EnumWindows((WNDENUMPROC)QKeyMapper::EnumWindowsProc, 0);
    }
}

void QKeyMapper::setKeyHook(HWND hWnd)
{
    if(TRUE == IsWindowVisible(hWnd)){
        clearAllBurstTimersAndLockKeys();
        pressedRealKeysList.clear();
        pressedVirtualKeysList.clear();
        m_BurstTimerMap.clear();
        m_BurstKeyUpTimerMap.clear();
        pressedLockKeysList.clear();
        m_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper::LowLevelKeyboardHookProc, GetModuleHandle(Q_NULLPTR), 0);
#ifdef SUPPORT_MOUSE_LL_HOOK
        m_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, QKeyMapper::LowLevelMouseHookProc, GetModuleHandle(Q_NULLPTR), 0);
#endif
        qDebug().nospace().noquote() << "[setKeyHook] " << "Normal Key Hook & Mouse Hook Started.";
    }
    else{
        qDebug().nospace().noquote() << "[setKeyHook] " << "Error: Invisible Window Handle!";
    }
}

void QKeyMapper::setKeyUnHook(void)
{
    if (m_KeyHook != Q_NULLPTR){
        clearAllBurstTimersAndLockKeys();
        pressedRealKeysList.clear();
        pressedVirtualKeysList.clear();
        m_BurstTimerMap.clear();
        m_BurstKeyUpTimerMap.clear();
        pressedLockKeysList.clear();
        UnhookWindowsHookEx(m_KeyHook);
        m_KeyHook = Q_NULLPTR;
        qDebug().nospace().noquote() << "[setKeyUnHook] " << "Normal Key Hook & Mouse Hook Released.";
    }

#ifdef SUPPORT_MOUSE_LL_HOOK
    if (m_MouseHook != Q_NULLPTR) {
        UnhookWindowsHookEx(m_MouseHook);
        m_MouseHook = Q_NULLPTR;
    }
#endif
}

void QKeyMapper::setDInputKeyHook(HWND hWnd)
{
    if(TRUE == IsWindowVisible(hWnd)){
        m_DirectInput = Q_NULLPTR;
        HRESULT hResult;
        hResult = DirectInput8Create(GetModuleHandle(Q_NULLPTR), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&m_DirectInput, Q_NULLPTR);
        //hResult = DirectInput8Create((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&m_DirectInput, Q_NULLPTR);
        if (DI_OK == hResult)
        {
            LPDIRECTINPUTDEVICE8 lpdiKeyboard;
            hResult = m_DirectInput->CreateDevice(GUID_SysKeyboard, &lpdiKeyboard, Q_NULLPTR);
            if (DI_OK == hResult)
            {
                FuncPtrGetDeviceState = (GetDeviceStateT)HookVTableFunction(lpdiKeyboard, hookGetDeviceState, 9);
                FuncPtrGetDeviceData = (GetDeviceDataT)HookVTableFunction(lpdiKeyboard, hookGetDeviceData, 10);
                qDebug().nospace().noquote() << "[DINPUT] " << "DirectInput Key Hook Started.";
            }
            else{
                m_DirectInput->Release();
                qDebug().noquote() << "[DINPUT]" << "Failed to Create Keyboard Device!";
            }
        }
        else{
            qDebug().noquote() << "[DINPUT]" << "Failed to acquire DirectInput handle";
        }
    }
    else{
        qDebug().nospace().noquote() << "[DINPUT] " << "Invisible window!";
    }
}

void QKeyMapper::setDInputKeyUnHook(void)
{
    if (m_DirectInput != Q_NULLPTR){
        m_DirectInput->Release();
        m_DirectInput = Q_NULLPTR;
        qDebug().nospace().noquote() << "[DINPUT] " << "DirectInput Key Hook Released.";
    }
    FuncPtrGetDeviceState = Q_NULLPTR;
    FuncPtrGetDeviceData = Q_NULLPTR;
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

QKeyMapper *QKeyMapper::static_Ptr()
{
    return selfPtr_static;
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
BOOL QKeyMapper::EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    //EnumChildWindows(hWnd, (WNDENUMPROC)QKeyMapper::EnumChildWindowsProc, 0);

    Q_UNUSED(lParam);

    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);

    if(FALSE == IsWindowVisible(hWnd)){
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[EnumWindowsProc] " << "(Invisible window)" << " [PID:" << dwProcessId <<"]";
#endif
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
                ProcessInfo.WindowIcon = QIcon(QtWin::fromHICON(iconptr));
            }
            else{
                QFileIconProvider icon_provider;
                QIcon fileicon = icon_provider.icon(QFileInfo(ProcessPath));

                if (false == fileicon.isNull()){
                    ProcessInfo.WindowIcon = fileicon;
                }
            }

            if (ProcessInfo.WindowIcon.isNull() != true){
                static_ProcessInfoList.append(ProcessInfo);

#ifdef DEBUG_LOGOUT_ON
                if (iconptr != Q_NULLPTR){
                    qDebug().nospace().noquote() << "[EnumWindowsProc] " << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
                }
                else{
                    qDebug().nospace().noquote() << "[EnumWindowsProc] " << "(HICON pointer NULL)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
                }
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
#ifdef DEBUG_LOGOUT_ON
        WindowText = QString::fromWCharArray(titleBuffer);
        qDebug().nospace().noquote() << "[EnumWindowsProc] " << "(WindowTitle empty)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
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
                ProcessInfo.WindowIcon = QIcon(QtWin::fromHICON(iconptr));
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
                    qDebug().nospace().noquote() << "[EnumChildWindowsProc] " << "(HICON pointer NULL)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
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
#ifdef DEBUG_LOGOUT_ON
        WindowText = QString::fromWCharArray(titleBuffer);
        qDebug().nospace().noquote() << "[EnumChildWindowsProc] " << "(WindowTitle empty)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
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
    bool keymapdatacontainsflag = false;
    for (const MAP_KEYDATA &keymapdata : KeyMappingDataList)
    {
        if (keymapdata.Original_Key == keyname){
            keymapdatacontainsflag = true;
            returnindex = keymapdataindex;
            break;
        }

        keymapdataindex += 1;
    }

    return returnindex;
}

void QKeyMapper::sendKeyboardInput(V_KEYCODE &vkeycode, int keyupdown)
{
    INPUT keyboard_input;
    DWORD extenedkeyflag = 0;
    if (true == vkeycode.ExtenedFlag){
        extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
    }
    else{
        extenedkeyflag = 0;
    }
    keyboard_input.type = INPUT_KEYBOARD;
    keyboard_input.ki.wScan = 0;
    keyboard_input.ki.time = 0;
    keyboard_input.ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
    keyboard_input.ki.wVk = vkeycode.KeyCode;
    if (KEY_DOWN == keyupdown) {
        keyboard_input.ki.dwFlags = extenedkeyflag | 0;
    }
    else {
        keyboard_input.ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
    }
    UINT uSent = SendInput(1, &keyboard_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("sendKeyboardInput(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper::sendMouseInput(V_MOUSECODE &vmousecode, int keyupdown)
{
    INPUT mouse_input;
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = 0;
    mouse_input.mi.dy = 0;
    mouse_input.mi.mouseData = 0;
    mouse_input.mi.time = 0;
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
    if (KEY_DOWN == keyupdown) {
        mouse_input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | vmousecode.MouseDownCode;
    }
    else {
        mouse_input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | vmousecode.MouseUpCode;
    }
    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("sendMouseInput(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper::sendInputKeys(QStringList &inputKeys, int keyupdown, QString &original_key, int sendmode)
{
    int keycount = inputKeys.size();
    if (keycount <= 0) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("sendInputKeys(): no input keys, size error(%d)!!!", keycount);
#endif
        return;
    }
    else if (keycount > SEND_INPUTS_MAX) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("sendInputKeys(): too many input keys(%d)!!!", keycount);
#endif
        return;
    }

    int index = 0;
    INPUT inputs[SEND_INPUTS_MAX] = { 0 };

    if (KEY_UP == keyupdown) {
        for(auto it = inputKeys.crbegin(); it != inputKeys.crend(); ++it) {
            QString key = (*it);

            /* special hook key process */
            if (SENDMODE_HOOK == sendmode) {
                if ((original_key == key) && (keycount == 1)) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug("Mapping the same key, do not skip send mapping VirtualKey \"%s\" KEYUP!", key.toStdString().c_str());
#endif
                }
                else {
                    if (pressedRealKeysList.contains(key)){
                        pressedVirtualKeysList.removeAll(key);
#ifdef DEBUG_LOGOUT_ON
                        qDebug("RealKey \"%s\" is pressed down on keyboard, skip send mapping VirtualKey \"%s\" KEYUP!", key.toStdString().c_str(), key.toStdString().c_str());
                        qDebug("Remove \"%s\" in pressedVirtualKeysList.", key.toStdString().c_str());
#endif
                        continue;
                    }
                }
            }
            else if (SENDMODE_BURST_STOP == sendmode) {
                if (pressedRealKeysList.contains(key)){
                    pressedVirtualKeysList.removeAll(key);
#ifdef DEBUG_LOGOUT_ON
                    qDebug("stopBurstTimer()->sendBurstKeyUp()->sendInputKeys(): RealKey \"%s\" is pressed down on keyboard, skip send mapping VirtualKey \"%s\" KEYUP!", key.toStdString().c_str(), key.toStdString().c_str());
#endif
                    continue;
                }
            }

            INPUT *input_p = &inputs[index];
            if (true == VirtualMouseButtonMap.contains(key)) {
                V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);
                input_p->type = INPUT_MOUSE;
                input_p->mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
                if (KEY_DOWN == keyupdown) {
                    input_p->mi.dwFlags = MOUSEEVENTF_ABSOLUTE | vmousecode.MouseDownCode;
                }
                else {
                    input_p->mi.dwFlags = MOUSEEVENTF_ABSOLUTE | vmousecode.MouseUpCode;
                }
            }
            else if (true == VirtualKeyCodeMap.contains(key)) {
                V_KEYCODE vkeycode = VirtualKeyCodeMap.value(key);
                DWORD extenedkeyflag = 0;
                if (true == vkeycode.ExtenedFlag){
                    extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
                }
                else{
                    extenedkeyflag = 0;
                }
                input_p->type = INPUT_KEYBOARD;
                input_p->ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
                input_p->ki.wVk = vkeycode.KeyCode;
                if (KEY_DOWN == keyupdown) {
                    input_p->ki.dwFlags = extenedkeyflag | 0;
                }
                else {
                    input_p->ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("sendInputKeys(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
            }
            index++;
        }
    }
    else {
        for (const QString &key : inputKeys){
            INPUT *input_p = &inputs[index];
            if (true == VirtualMouseButtonMap.contains(key)) {
                V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);
                input_p->type = INPUT_MOUSE;
                input_p->mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
                if (KEY_DOWN == keyupdown) {
                    input_p->mi.dwFlags = MOUSEEVENTF_ABSOLUTE | vmousecode.MouseDownCode;
                }
                else {
                    input_p->mi.dwFlags = MOUSEEVENTF_ABSOLUTE | vmousecode.MouseUpCode;
                }
            }
            else if (true == VirtualKeyCodeMap.contains(key)) {
                V_KEYCODE vkeycode = VirtualKeyCodeMap.value(key);
                DWORD extenedkeyflag = 0;
                if (true == vkeycode.ExtenedFlag){
                    extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
                }
                else{
                    extenedkeyflag = 0;
                }
                input_p->type = INPUT_KEYBOARD;
                input_p->ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
                input_p->ki.wVk = vkeycode.KeyCode;
                if (KEY_DOWN == keyupdown) {
                    input_p->ki.dwFlags = extenedkeyflag | 0;
                }
                else {
                    input_p->ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("sendInputKeys(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
            }
            index++;
        }
    }

    UINT uSent = SendInput(keycount, inputs, sizeof(INPUT));
    if (uSent != keycount) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("sendInputKeys(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper::sendBurstKeyDown(const QString &burstKey)
{
    int findindex = findInKeyMappingDataList(burstKey);

    if (findindex >=0){
        QStringList mappingKeyList = KeyMappingDataList.at(findindex).Mapping_Keys;
        QString original_key = KeyMappingDataList.at(findindex).Original_Key;
        sendInputKeys(mappingKeyList, KEY_DOWN, original_key, SENDMODE_BURST_NORMAL);
    }
}

void QKeyMapper::sendBurstKeyUp(const QString &burstKey, bool stop)
{
    int findindex = findInKeyMappingDataList(burstKey);

    if (findindex >=0){
        QStringList mappingKeyList = KeyMappingDataList.at(findindex).Mapping_Keys;
        QString original_key = KeyMappingDataList.at(findindex).Original_Key;
        int sendmode = SENDMODE_BURST_NORMAL;
        if (true == stop) {
            sendmode = SENDMODE_BURST_STOP;
        }
        sendInputKeys(mappingKeyList, KEY_UP, original_key, sendmode);
    }
}

void QKeyMapper::sendSpecialVirtualKeyDown(const QString &virtualKey)
{
    if (true == VirtualMouseButtonMap.contains(virtualKey)) {
        V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(virtualKey);
        sendMouseInput(vmousecode, KEY_DOWN);
    }
    else {
        V_KEYCODE map_vkeycode = VirtualKeyCodeMap.value(virtualKey);
        sendKeyboardInput(map_vkeycode, KEY_DOWN);
    }
}

void QKeyMapper::sendSpecialVirtualKeyUp(const QString &virtualKey)
{
    if (true == VirtualMouseButtonMap.contains(virtualKey)) {
        V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(virtualKey);
        sendMouseInput(vmousecode, KEY_UP);
    }
    else {
        V_KEYCODE map_vkeycode = VirtualKeyCodeMap.value(virtualKey);
        sendKeyboardInput(map_vkeycode, KEY_UP);
    }
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
                        ProcessInfo.WindowIcon = QIcon(QtWin::fromHICON(iconptr));
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
                            qDebug().nospace().noquote() << "[EnumProcessFunction] " << "(HICON pointer NULL)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
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
#ifdef DEBUG_LOGOUT_ON
                WindowText = QString::fromWCharArray(titleBuffer);
                qDebug().nospace().noquote() << "[EnumProcessFunction] " << "(WindowTitle empty)" << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
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

void QKeyMapper::changeEvent(QEvent *event)
{
    if(event->type()==QEvent::WindowStateChange)
    {
        QTimer::singleShot(0, this, SLOT(WindowStateChangedProc()));
    }
    QDialog::changeEvent(event);
}

void QKeyMapper::timerEvent(QTimerEvent *event)
{
    int timerID = event->timerId();

    if (true == m_BurstKeyUpTimerMap.values().contains(timerID)) {
        QString burstKey = m_BurstKeyUpTimerMap.key(timerID);
        if (false == burstKey.isEmpty()) {
            killTimer(timerID);
            m_BurstKeyUpTimerMap.remove(burstKey);
            sendBurstKeyUp(burstKey, false);
//#ifdef DEBUG_LOGOUT_ON
//            qDebug("timerEvent(): Key \"%s\" BurstPress timeout.", burstKey.toStdString().c_str());
//#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("timerEvent(): Could not find TimerID(%d) in m_BurstKeyUpTimerMap!!!", timerID);
#endif
        }
    }
    else if (true == m_BurstTimerMap.values().contains(timerID)) {
        QString burstKey = m_BurstTimerMap.key(timerID);
        if (false == burstKey.isEmpty()) {
//#ifdef DEBUG_LOGOUT_ON
//            qDebug("timerEvent(): Key \"%s\" BurstTimer timeout.", burstKey.toStdString().c_str());
//#endif
            sendBurstKeyDown(burstKey);

            if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
                int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
                qDebug("timerEvent(): Key \"%s\" BurstKeyUpTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
                killTimer(existTimerID);
                m_BurstKeyUpTimerMap.remove(burstKey);
            }
            int burstpressTime = ui->burstpressComboBox->currentText().toInt();
            int keyupTimerID = startTimer(burstpressTime, Qt::PreciseTimer);
            m_BurstKeyUpTimerMap.insert(burstKey, keyupTimerID);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("timerEvent(): Could not find TimerID(%d) in BurstTimerMap!!!", timerID);
#endif
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning("timerEvent(): Could not find TimerID(%d) in both m_BurstKeyUpTimerMap & BurstTimerMap!!!", timerID);
#endif
    }
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
            m_SysTrayIcon->setToolTip("QKeyMapper(Mapping : " + m_MapProcessInfo.FileName + ")");
#ifdef USE_SABER_ICON
            m_SysTrayIcon->setIcon(QIcon(":/AppIcon_Saber_Working.ico"));
#else
            m_SysTrayIcon->setIcon(QIcon(":/AppIcon_Working.ico"));
#endif
            ui->keymapButton->setText("KeyMappingStop");
            m_KeyMapStatus = KEYMAP_CHECKING;
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
#ifdef USE_SABER_ICON
        m_SysTrayIcon->setIcon(QIcon(":/AppIcon_Saber.ico"));
#else
        m_SysTrayIcon->setIcon(QIcon(":/AppIcon.ico"));
#endif
        ui->keymapButton->setText("KeyMappingStart");
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

void QKeyMapper::on_cellChanged(int row, int col)
{
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

void QKeyMapper::saveKeyMapSetting(void)
{
    if (ui->keymapdataTable->rowCount() == KeyMappingDataList.size()){
        QSettings settingFile(QString("keymapdata.ini"), QSettings::IniFormat);
        settingFile.setIniCodec("UTF-8");
        QStringList original_keys;
        QStringList mapping_keysList;
        QStringList burstList;
        QStringList lockList;
        QString burstpressTimeString = ui->burstpressComboBox->currentText();
        QString burstreleaseTimeString = ui->burstreleaseComboBox->currentText();
        int settingSelectIndex = ui->settingselectComboBox->currentIndex();

        settingFile.setValue(SETTINGSELECT , settingSelectIndex);
        QString settingSelIndexStr = QString::number(settingSelectIndex+1) + "/";

        if (KeyMappingDataList.size() > 0){
            for (const MAP_KEYDATA &keymapdata : KeyMappingDataList)
            {
                original_keys << keymapdata.Original_Key;
                QString mappingkeys_str;
                if (keymapdata.Mapping_Keys.size() == 1){
                    mappingkeys_str = keymapdata.Mapping_Keys.constFirst();
                }
                else {
                    mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_STR);
                }
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
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_ORIGINALKEYS, original_keys );
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_MAPPINGKEYS , mapping_keysList  );
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_BURST , burstList  );
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_LOCK , lockList  );
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_BURSTPRESS_TIME , burstpressTimeString  );
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_BURSTRELEASE_TIME , burstreleaseTimeString  );

            settingFile.remove(settingSelIndexStr+CLEARALL);
        }
        else{
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_ORIGINALKEYS, original_keys );
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_MAPPINGKEYS , mapping_keysList  );
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_BURST , burstList  );
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_LOCK , lockList  );
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_BURSTPRESS_TIME , burstpressTimeString  );
            settingFile.setValue(settingSelIndexStr+KEYMAPDATA_BURSTRELEASE_TIME , burstreleaseTimeString  );
            settingFile.setValue(settingSelIndexStr+CLEARALL, QString("ClearList"));
        }

        if ((false == ui->nameLineEdit->text().isEmpty())
                && (false == ui->titleLineEdit->text().isEmpty())
                && (ui->nameLineEdit->text() == m_MapProcessInfo.FileName)
                && (ui->titleLineEdit->text() == m_MapProcessInfo.WindowTitle)){
            settingFile.setValue(settingSelIndexStr+PROCESSINFO_FILENAME, m_MapProcessInfo.FileName);
            settingFile.setValue(settingSelIndexStr+PROCESSINFO_WINDOWTITLE, m_MapProcessInfo.WindowTitle);

            if (false == m_MapProcessInfo.FilePath.isEmpty()){
                settingFile.setValue(settingSelIndexStr+PROCESSINFO_FILEPATH, m_MapProcessInfo.FilePath);
            }
            else{
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[saveKeyMapSetting]" << "FilePath is empty, unsaved.";
#endif
            }

            settingFile.setValue(settingSelIndexStr+PROCESSINFO_FILENAME_CHECKED, ui->nameCheckBox->isChecked());
            settingFile.setValue(settingSelIndexStr+PROCESSINFO_WINDOWTITLE_CHECKED, ui->titleCheckBox->isChecked());
            settingFile.setValue(settingSelIndexStr+DISABLEWINKEY_CHECKED, ui->disableWinKeyCheckBox->isChecked());
            settingFile.setValue(settingSelIndexStr+AUTOSTARTMAPPING_CHECKED, ui->autoStartMappingCheckBox->isChecked());
        }
        else{
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[saveKeyMapSetting]" << "Unmatch display processinfo & stored processinfo.";
#endif
        }

    }
    else{
        QMessageBox::warning(this, tr("QKeyMapper"), tr("Invalid KeyMap Data."));
    }
}

bool QKeyMapper::loadKeyMapSetting(int settingIndex)
{
    bool loadDefault = false;
    bool clearallcontainsflag = true;
    bool selectSettingContainsFlag = false;
    quint8 datavalidflag = 0xFF;
    QSettings settingFile(QString("keymapdata.ini"), QSettings::IniFormat);
    settingFile.setIniCodec("UTF-8");
    QString settingSelIndexStr;

    int settingSelectIndex = -1;
    if (0 == settingIndex) {
        if (true == settingFile.contains(SETTINGSELECT)){
            settingSelectIndex = settingFile.value(SETTINGSELECT).toInt();
            ui->settingselectComboBox->setCurrentIndex(settingSelectIndex);
            settingSelectIndex = settingSelectIndex + 1;
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Startup load setting" << settingSelectIndex;
#endif
        }
        else {
            settingSelectIndex = -1;
            clearallcontainsflag = false;
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "Startup loading do not contain setting select!";
#endif
        }
    }
    else {
        /* Select setting from combobox */
        if (true == settingFile.contains(SETTINGSELECT)){
            settingSelectIndex = settingIndex;
            settingSelIndexStr = QString::number(settingSelectIndex) + "/";
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "SettingSelect combox select Setting" << settingSelectIndex;
#endif

            if ((true == settingFile.contains(settingSelIndexStr+KEYMAPDATA_ORIGINALKEYS))
                    && (true == settingFile.contains(settingSelIndexStr+KEYMAPDATA_MAPPINGKEYS))
                    && (true == settingFile.contains(settingSelIndexStr+KEYMAPDATA_BURST))
                    && (true == settingFile.contains(settingSelIndexStr+KEYMAPDATA_LOCK))){
                selectSettingContainsFlag = true;
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[loadKeyMapSetting]" << "SettingSelect combox select loading contains Setting" << settingSelectIndex;
#endif
            }
            else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "SettingSelect combox select loading do not contain Setting" << settingSelectIndex;
#endif
            }
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << "SettingSelect combox select loading do not contain SettingSelect";
#endif
        }
    }

    if (settingSelectIndex > 0) {
        settingSelIndexStr = QString::number(settingSelectIndex) + "/";

        if (0 == settingIndex || true == selectSettingContainsFlag) {
            if (false == settingFile.contains(settingSelIndexStr+CLEARALL)){
                clearallcontainsflag = false;
            }
        }
    }
    else {
        settingSelIndexStr = QString();
    }

    if (false == clearallcontainsflag){
        QStringList original_keys;
        QStringList mapping_keys;
        QStringList burstStringList;
        QStringList lockStringList;
        QList<bool> burstList;
        QList<bool> lockList;
        QList<MAP_KEYDATA> loadkeymapdata;

        if ((true == settingFile.contains(settingSelIndexStr+KEYMAPDATA_ORIGINALKEYS))
                && (true == settingFile.contains(settingSelIndexStr+KEYMAPDATA_MAPPINGKEYS))
                && (true == settingFile.contains(settingSelIndexStr+KEYMAPDATA_BURST))
                && (true == settingFile.contains(settingSelIndexStr+KEYMAPDATA_LOCK))){
            original_keys   = settingFile.value(settingSelIndexStr+KEYMAPDATA_ORIGINALKEYS).toStringList();
            mapping_keys    = settingFile.value(settingSelIndexStr+KEYMAPDATA_MAPPINGKEYS).toStringList();
            burstStringList = settingFile.value(settingSelIndexStr+KEYMAPDATA_BURST).toStringList();
            lockStringList  = settingFile.value(settingSelIndexStr+KEYMAPDATA_LOCK).toStringList();

            if ((original_keys.size() == mapping_keys.size())
                    && (original_keys.size() == burstStringList.size())){
                datavalidflag = true;

                if (original_keys.size() > 0){
                    for (const QString &burst : burstStringList){
                        if (burst == "ON") {
                            burstList.append(true);
                        }
                        else {
                            burstList.append(false);
                        }
                    }

                    for (const QString &lock : lockStringList){
                        if (lock == "ON") {
                            lockList.append(true);
                        }
                        else {
                            lockList.append(false);
                        }
                    }

                    int loadindex = 0;
                    for (const QString &ori_key : original_keys){
                        if ((true == VirtualKeyCodeMap.contains(ori_key) || true == VirtualMouseButtonMap.contains(ori_key))
                                && (true == checkMappingkeyStr(mapping_keys.at(loadindex)))){
                            loadkeymapdata.append(MAP_KEYDATA(ori_key, mapping_keys.at(loadindex), burstList.at(loadindex), lockList.at(loadindex)));
                        }
                        else{
                            datavalidflag = false;
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
    qDebug() << __func__ << ": refreshKeyMappingDataTable()";
#endif
    refreshKeyMappingDataTable();

    if ((true == settingFile.contains(settingSelIndexStr+PROCESSINFO_FILENAME))
            && (true == settingFile.contains(settingSelIndexStr+PROCESSINFO_WINDOWTITLE))){
        m_MapProcessInfo.FileName = settingFile.value(settingSelIndexStr+PROCESSINFO_FILENAME).toString();
        m_MapProcessInfo.WindowTitle = settingFile.value(settingSelIndexStr+PROCESSINFO_WINDOWTITLE).toString();

        ui->nameLineEdit->setText(m_MapProcessInfo.FileName);
        ui->titleLineEdit->setText(m_MapProcessInfo.WindowTitle);
    }
    else {
        ui->nameLineEdit->setText(QString());
        ui->titleLineEdit->setText(QString());
    }

    if (true == settingFile.contains(settingSelIndexStr+PROCESSINFO_FILEPATH)){
        m_MapProcessInfo.FilePath = settingFile.value(settingSelIndexStr+PROCESSINFO_FILEPATH).toString();
    }
    else {
        m_MapProcessInfo = MAP_PROCESSINFO();
        ui->iconLabel->clear();
    }

    if (true == loadDefault) {
        setMapProcessInfo(QString(DEFAULT_NAME), QString(DEFAULT_TITLE), QString(), QString(), QIcon(":/DefaultIcon.ico"));
    }

    updateProcessInfoDisplay();

    if (true == settingFile.contains(settingSelIndexStr+PROCESSINFO_FILENAME_CHECKED)){
        bool fileNameChecked = settingFile.value(settingSelIndexStr+PROCESSINFO_FILENAME_CHECKED).toBool();
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

    if (true == settingFile.contains(settingSelIndexStr+PROCESSINFO_WINDOWTITLE_CHECKED)){
        bool windowTitleChecked = settingFile.value(settingSelIndexStr+PROCESSINFO_WINDOWTITLE_CHECKED).toBool();
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

    if (true == settingFile.contains(settingSelIndexStr+KEYMAPDATA_BURSTPRESS_TIME)){
        QString burstpressTimeString = settingFile.value(settingSelIndexStr+KEYMAPDATA_BURSTPRESS_TIME).toString();
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

    if (true == settingFile.contains(settingSelIndexStr+KEYMAPDATA_BURSTRELEASE_TIME)){
        QString burstreleaseTimeString = settingFile.value(settingSelIndexStr+KEYMAPDATA_BURSTRELEASE_TIME).toString();
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

    if (true == settingFile.contains(settingSelIndexStr+DISABLEWINKEY_CHECKED)){
        bool disableWinKeyChecked = settingFile.value(settingSelIndexStr+DISABLEWINKEY_CHECKED).toBool();
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
    if (true == settingFile.contains(settingSelIndexStr+AUTOSTARTMAPPING_CHECKED)){
        autoStartMappingChecked = settingFile.value(settingSelIndexStr+AUTOSTARTMAPPING_CHECKED).toBool();
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
        if ((true == autoStartMappingChecked) && (0 == settingIndex)) {
            on_keymapButton_clicked();
        }
        return true;
    }
}

bool QKeyMapper::checkMappingkeyStr(const QString &mappingkeystr)
{
    bool checkResult = true;
    QStringList Mapping_Keys = mappingkeystr.split(SEPARATOR_STR);
    for (const QString &mapping_key : Mapping_Keys){
        if (false == VirtualKeyCodeMap.contains(mapping_key)
            && false == VirtualMouseButtonMap.contains(mapping_key)){
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

void QKeyMapper::setControlCustomFont(const QString &fontname)
{
    QFont customFont(fontname);

    customFont.setPointSize(20);
    ui->refreshButton->setFont(customFont);
    ui->keymapButton->setFont(customFont);
    ui->savemaplistButton->setFont(customFont);

    customFont.setPointSize(12);
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

    ui->processinfoTable->horizontalHeader()->setFont(customFont);
    ui->keymapdataTable->horizontalHeader()->setFont(customFont);

    customFont.setPointSize(14);
    ui->disableWinKeyCheckBox->setFont(customFont);
    ui->autoStartMappingCheckBox->setFont(customFont);
}

void QKeyMapper::changeControlEnableStatus(bool status)
{
    ui->nameCheckBox->setEnabled(status);
    ui->titleCheckBox->setEnabled(status);
    ui->disableWinKeyCheckBox->setEnabled(status);
    ui->autoStartMappingCheckBox->setEnabled(status);
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

    ui->refreshButton->setEnabled(status);
    ui->savemaplistButton->setEnabled(status);

    ui->processinfoTable->setEnabled(status);
    ui->keymapdataTable->setEnabled(status);
}

void QKeyMapper::startBurstTimer(const QString &burstKey, int mappingIndex)
{
    Q_UNUSED(mappingIndex);
    if (true == m_BurstTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("startBurstTimer(): Key \"%s\" BurstTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstTimerMap.remove(burstKey);
    }
    if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("startBurstTimer(): Key \"%s\" BurstKeyUpTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstKeyUpTimerMap.remove(burstKey);
    }
    int burstpressTime = ui->burstpressComboBox->currentText().toInt();
    int burstreleaseTime = ui->burstreleaseComboBox->currentText().toInt();
    int burstTime = burstpressTime + burstreleaseTime;
    int timerID = startTimer(burstTime, Qt::PreciseTimer);
    m_BurstTimerMap.insert(burstKey, timerID);

    int keyupTimerID = startTimer(burstpressTime, Qt::PreciseTimer);
    m_BurstKeyUpTimerMap.insert(burstKey, keyupTimerID);

#ifdef DEBUG_LOGOUT_ON
    qDebug("startBurstTimer(): Key \"%s\", Timer:%d, ID:%d", burstKey.toStdString().c_str(), burstTime, timerID);
#endif
}

void QKeyMapper::stopBurstTimer(const QString &burstKey, int mappingIndex)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("stopBurstTimer(): Key \"%s\"", burstKey.toStdString().c_str());
#endif
    if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("stopBurstTimer(): Key \"%s\" kill BurstKeyUpTimer(%d)", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstKeyUpTimerMap.remove(burstKey);
    }

    if (true == m_BurstTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstTimerMap.value(burstKey);
        killTimer(existTimerID);
        m_BurstTimerMap.remove(burstKey);
        sendBurstKeyUp(burstKey, true);
#ifdef DEBUG_LOGOUT_ON
        qDebug("stopBurstTimer(): Key \"%s\" BurstTimer(%d) stoped.", burstKey.toStdString().c_str(), existTimerID);
#endif

        QStringList mappingKeyList = KeyMappingDataList.at(mappingIndex).Mapping_Keys;
        for (const QString &key : pressedRealKeysList){
            if ((true == mappingKeyList.contains(key))
                && (burstKey != key)
                && (false == pressedVirtualKeysList.contains(key))) {
#ifdef DEBUG_LOGOUT_ON
                qDebug("stopBurstTimer() : RealKey \"%s\" is still pressed down on keyboard, call sendSpecialVirtualKeyDown(\"%s\").", key.toStdString().c_str(), key.toStdString().c_str());
#endif
                sendSpecialVirtualKeyDown(key);
            }
        }
    }
}

void QKeyMapper::on_savemaplistButton_clicked()
{
    saveKeyMapSetting();
}

LRESULT QKeyMapper::LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;

    bool returnFlag = false;
    V_KEYCODE vkeycode;
    vkeycode.KeyCode = (quint8)pKeyBoard->vkCode;
    if (LLKHF_EXTENDED == (pKeyBoard->flags & LLKHF_EXTENDED)){
        vkeycode.ExtenedFlag = EXTENED_FLAG_TRUE;
    }
    else{
        vkeycode.ExtenedFlag = EXTENED_FLAG_FALSE;
    }

    QString keycodeString = VirtualKeyCodeMap.key(vkeycode);

//#ifdef DEBUG_LOGOUT_ON
//    qDebug("\"%s\" (0x%02X),  wParam(0x%04X), scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, wParam, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
//#endif

    if ((false == keycodeString.isEmpty())
        && (WM_KEYDOWN == wParam || WM_KEYUP == wParam)){

#ifdef DEBUG_LOGOUT_ON
        if (pKeyBoard->scanCode != 0){
            if (WM_KEYDOWN == wParam){
                qDebug("RealKey: \"%s\" (0x%02X) KeyDown, scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
            }
            else if (WM_KEYUP == wParam){
                qDebug("RealKey: \"%s\" (0x%02X) KeyUp, scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
            }
            else{
            }
        }
        else {
            if (WM_KEYDOWN == wParam){
                qDebug("VirtualMapKey: \"%s\" (0x%02X) KeyDown, scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
            }
            else if (WM_KEYUP == wParam){
                qDebug("VirtualMapKey: \"%s\" (0x%02X) KeyUp, scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
            }
            else{
            }
        }
#endif

        if (pKeyBoard->scanCode != 0){
            int findindex = findInKeyMappingDataList(keycodeString);

            if (WM_KEYDOWN == wParam){
                if (false == pressedRealKeysList.contains(keycodeString)){
                    if (findindex >=0 && true == KeyMappingDataList.at(findindex).Burst) {
                        if (true == KeyMappingDataList.at(findindex).Lock) {
                            if (true == KeyMappingDataList.at(findindex).LockStatus) {
                                returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                                qDebug("Lock ON & Burst ON(KeyDown) -> Key \"%s\" LockStatus is ON, skip startBurstTimer()!", keycodeString.toStdString().c_str());
#endif
                            }
                            else {
                                emit QKeyMapper::static_Ptr()->startBurstTimer_Signal(keycodeString, findindex);
                            }
                        }
                        else {
                            emit QKeyMapper::static_Ptr()->startBurstTimer_Signal(keycodeString, findindex);
                        }
                    }
                    pressedRealKeysList.append(keycodeString);
                }

                if (findindex >=0 && true == KeyMappingDataList.at(findindex).Lock) {
                    if (true == pressedLockKeysList.contains(keycodeString)){
                        KeyMappingDataList[findindex].LockStatus = false;
                        pressedLockKeysList.removeAll(keycodeString);
                        emit QKeyMapper::static_Ptr()->updateLockStatus_Signal();
#ifdef DEBUG_LOGOUT_ON
                        qDebug("Key \"%s\" KeyDown LockStatus -> OFF", keycodeString.toStdString().c_str());
#endif
                    }
                    else {
                        KeyMappingDataList[findindex].LockStatus = true;
                        pressedLockKeysList.append(keycodeString);
                        emit QKeyMapper::static_Ptr()->updateLockStatus_Signal();
#ifdef DEBUG_LOGOUT_ON
                        qDebug("Key \"%s\" KeyDown LockStatus -> ON", keycodeString.toStdString().c_str());
#endif
                    }
                }
            }
            else if (WM_KEYUP == wParam){
                if (true == pressedRealKeysList.contains(keycodeString)){
                    if (findindex >=0) {
                        if (true == KeyMappingDataList.at(findindex).Lock) {
                            /* Lock ON &  Burst ON */
                            if (true == KeyMappingDataList.at(findindex).Burst) {
                                if (true == KeyMappingDataList.at(findindex).LockStatus) {
                                    returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                                    qDebug("Lock ON & Burst ON(KeyUp) -> Key \"%s\" LockStatus is ON, skip stopBurstTimer()!", keycodeString.toStdString().c_str());
#endif
                                }
                                else {
                                    emit QKeyMapper::static_Ptr()->stopBurstTimer_Signal(keycodeString, findindex);
                                }
                            }
                            /* Lock ON &  Burst OFF */
                            else {
                                if (true == KeyMappingDataList.at(findindex).LockStatus) {
                                    returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                                    qDebug("Lock ON & Burst OFF -> Key \"%s\" LockStatus is ON, skip KeyUp!", keycodeString.toStdString().c_str());
#endif
                                }
                            }
                        }
                        else {
                            /* Lock OFF &  Burst ON */
                            if (true == KeyMappingDataList.at(findindex).Burst) {
                                emit QKeyMapper::static_Ptr()->stopBurstTimer_Signal(keycodeString, findindex);
                            }
                            /* Lock OFF &  Burst OFF do nothing */
                        }
                    }
                    pressedRealKeysList.removeAll(keycodeString);
                }
            }

            if (true == QKeyMapper::disableWinKeyCheckBox_static->isChecked()) {
                if (WM_KEYDOWN == wParam) {
                    if ("D" == keycodeString && pressedRealKeysList.contains("L-Win")) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug("\"L-Win + D\" pressed!");
#endif
                        V_KEYCODE map_vkeycode = VirtualKeyCodeMap.value("L-Win");
                        sendKeyboardInput(map_vkeycode, KEY_DOWN);
                    }
                }

                if (WM_KEYUP == wParam) {
                    if (("D" == keycodeString && pressedRealKeysList.contains("L-Win"))
                            || ("L-Win" == keycodeString && pressedRealKeysList.contains("D"))) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug("\"L-Win + D\" released by \"%s\" keyup!", keycodeString.toStdString().c_str());
#endif
                        V_KEYCODE map_vkeycode = VirtualKeyCodeMap.value("L-Win");
                        sendKeyboardInput(map_vkeycode, KEY_UP);
                    }
                }

                if ((WM_KEYDOWN == wParam)
                     || (WM_KEYUP == wParam)) {
                    if (("L-Win" == keycodeString)
                        || ("R-Win" == keycodeString)
                        || ("Application" == keycodeString)) {
                        qDebug("Disable \"%s\" (0x%02X), wParam(0x%04X), scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, wParam, pKeyBoard->scanCode, pKeyBoard->flags);
                        returnFlag = true;
                    }
                }
            }

            if (WM_KEYUP == wParam && false == returnFlag){
                if (findindex >=0 && (KeyMappingDataList.at(findindex).Original_Key == keycodeString)) {
                }
                else {
                    if (pressedVirtualKeysList.contains(keycodeString)) {
                        returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                        qDebug("VirtualKey \"%s\" is pressed down, skip RealKey \"%s\" KEYUP!", keycodeString.toStdString().c_str(), keycodeString.toStdString().c_str());
#endif
                    }
                }
            }

            if (false == returnFlag) {
                if (findindex >=0){
                    QStringList mappingKeyList = KeyMappingDataList.at(findindex).Mapping_Keys;
                    QString original_key = KeyMappingDataList.at(findindex).Original_Key;
                    if (WM_KEYDOWN == wParam){
                        sendInputKeys(mappingKeyList, KEY_DOWN, original_key, SENDMODE_HOOK);
                        returnFlag = true;
                    }
                    else if (WM_KEYUP == wParam){
                        sendInputKeys(mappingKeyList, KEY_UP, original_key, SENDMODE_HOOK);
                        returnFlag = true;
                    }
                }
            }
        }
        else {
            if (WM_KEYDOWN == wParam){
                if (false == pressedVirtualKeysList.contains(keycodeString)){
                    pressedVirtualKeysList.append(keycodeString);
                }
            }
            else if (WM_KEYUP == wParam){
                pressedVirtualKeysList.removeAll(keycodeString);
            }
        }
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug("UnknownKey (0x%02X) Input, scanCode(0x%08X), wParam(0x%08X), flags(0x%08X)", pKeyBoard->vkCode, pKeyBoard->scanCode, wParam, pKeyBoard->flags);
#endif
    }

    if (true == returnFlag){
#ifdef DEBUG_LOGOUT_ON
        qDebug("LowLevelKeyboardHookProc() -> return TRUE");
#endif
        return (LRESULT)TRUE;
    }
    else{
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }
}

LRESULT QKeyMapper::LowLevelMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

    LPARAM extraInfo = GetMessageExtraInfo();

//    MSLLHOOKSTRUCT *pMouse = (MSLLHOOKSTRUCT *)lParam;

    if ((wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP)
        || (wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP)
        || (wParam == WM_MBUTTONDOWN || wParam == WM_MBUTTONUP)
        /*|| wParam == WM_MOUSEWHEEL*/) {
        if (true == MouseButtonNameMap.contains(wParam)) {
#ifdef DEBUG_LOGOUT_ON
            if(VIRTUAL_MOUSE_CLICK == extraInfo) {
                qDebug("Virtual \"%s\"", MouseButtonNameMap.value(wParam).toStdString().c_str());
            }
            else {
                qDebug("Real \"%s\"", MouseButtonNameMap.value(wParam).toStdString().c_str());
            }
#endif
        }

//        if (wParam == WM_MOUSEWHEEL) {
//            int wheel_scroll = MOUSEWHEEL_SCROLL_NONE;
//            if (pMouse->mouseData > 0) {
//                wheel_scroll = MOUSEWHEEL_SCROLL_UP;
//            }
//            else if (pMouse->mouseData < 0) {
//                wheel_scroll = MOUSEWHEEL_SCROLL_DOWN;
//            }

//            if (MOUSEWHEEL_SCROLL_UP == wheel_scroll) {
//#ifdef DEBUG_LOGOUT_ON
//                qDebug("MouseWheel Scroll Up");
//#endif
//            }
//            else {
//#ifdef DEBUG_LOGOUT_ON
//                qDebug("MouseWheel Scroll Down");
//#endif
//            }
//        }
    }

    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
}

void *QKeyMapper::HookVTableFunction(void *pVTable, void *fnHookFunc, int nOffset)
{
    intptr_t ptrVtable = *((intptr_t*)pVTable); // Pointer to our chosen vtable
    intptr_t ptrFunction = ptrVtable + sizeof(intptr_t) * nOffset; // The offset to the function (remember it's a zero indexed array with a size of four bytes)
    intptr_t ptrOriginal = *((intptr_t*)ptrFunction); // Save original address

    // Edit the memory protection so we can modify it
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery((LPCVOID)ptrFunction, &mbi, sizeof(mbi));
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &mbi.Protect);

    // Overwrite the old function with our new one
    *((intptr_t*)ptrFunction) = (intptr_t)fnHookFunc;

    // Restore the protection
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &mbi.Protect);

    // Return the original function address incase we want to call it
    return (void*)ptrOriginal;
}

HRESULT QKeyMapper::hookGetDeviceState(IDirectInputDevice8W *pThis, DWORD cbData, LPVOID lpvData)
{
    HRESULT result = FuncPtrGetDeviceState(pThis, cbData, lpvData);

    if (result == DI_OK) {
        if (cbData == 256){//caller is a keyboard
            BYTE* keystate_array = (BYTE*)lpvData;
            if  (keystate_array[DIK_W] & 0x80){
                qDebug().noquote() << "[DINPUT] hookGetDeviceData: [W] Key Pressed.";
            }
            if  (keystate_array[DIK_S] & 0x80){
                qDebug().noquote() << "[DINPUT] hookGetDeviceData: [S] Key Pressed.";
            }
            if  (keystate_array[DIK_A] & 0x80){
                qDebug().noquote() << "[DINPUT] hookGetDeviceData: [A] Key Pressed.";
            }
            if  (keystate_array[DIK_D] & 0x80){
                qDebug().noquote() << "[DINPUT] hookGetDeviceData: [D] Key Pressed.";
            }
        }
    }

    return result;
}

HRESULT QKeyMapper::hookGetDeviceData(IDirectInputDevice8W *pThis, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags)
{
    HRESULT result = FuncPtrGetDeviceData(pThis, cbObjectData, rgdod, pdwInOut, dwFlags);

    if (result == DI_OK) {
        for (DWORD i = 0; i < *pdwInOut; ++i) {
            if (LOBYTE(rgdod[i].dwData) > 0) { //key down
                if (rgdod[i].dwOfs == DIK_W) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [W] Key Down.";
                }
                else if (rgdod[i].dwOfs == DIK_S) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [S] Key Down.";
                }
                else if (rgdod[i].dwOfs == DIK_A) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [A] Key Down.";
                }
                else if (rgdod[i].dwOfs == DIK_D) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [D] Key Down.";
                }
            }
            if (LOBYTE(rgdod[i].dwData) == 0) { //key up
                if (rgdod[i].dwOfs == DIK_W) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [W] Key Up.";
                }
                else if (rgdod[i].dwOfs == DIK_S) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [S] Key Up.";
                }
                else if (rgdod[i].dwOfs == DIK_A) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [A] Key Up.";
                }
                else if (rgdod[i].dwOfs == DIK_D) {
                    qDebug().noquote() << "[DINPUT] hookGetDeviceData: [D] Key Up.";
                }
            }
        }
    }
    return result;
}

void QKeyMapper::initHotKeySequence()
{
    connect( m_HotKey, &QHotkey::activated, this, &QKeyMapper::HotKeyActivated);

    QKeySequence hotkeysequence = QKeySequence::fromString("Ctrl+`");
    m_HotKey->setShortcut(hotkeysequence, true);
}

void QKeyMapper::initVirtualKeyCodeMap(void)
{
    // US 104 Keyboard Main Area
    // Row 1
    VirtualKeyCodeMap.insert        ("`~",          V_KEYCODE(VK_OEM_3,         EXTENED_FLAG_FALSE));   // 0xC0
    VirtualKeyCodeMap.insert        ("1!",          V_KEYCODE(VK_1,             EXTENED_FLAG_FALSE));   // 0x31
    VirtualKeyCodeMap.insert        ("2@",          V_KEYCODE(VK_2,             EXTENED_FLAG_FALSE));   // 0x32
    VirtualKeyCodeMap.insert        ("3#",          V_KEYCODE(VK_3,             EXTENED_FLAG_FALSE));   // 0x33
    VirtualKeyCodeMap.insert        ("4$",          V_KEYCODE(VK_4,             EXTENED_FLAG_FALSE));   // 0x34
    VirtualKeyCodeMap.insert        ("5%",          V_KEYCODE(VK_5,             EXTENED_FLAG_FALSE));   // 0x35
    VirtualKeyCodeMap.insert        ("6^",          V_KEYCODE(VK_6,             EXTENED_FLAG_FALSE));   // 0x36
    VirtualKeyCodeMap.insert        ("7&",          V_KEYCODE(VK_7,             EXTENED_FLAG_FALSE));   // 0x37
    VirtualKeyCodeMap.insert        ("8*",          V_KEYCODE(VK_8,             EXTENED_FLAG_FALSE));   // 0x38
    VirtualKeyCodeMap.insert        ("9(",          V_KEYCODE(VK_9,             EXTENED_FLAG_FALSE));   // 0x39
    VirtualKeyCodeMap.insert        ("0)",          V_KEYCODE(VK_0,             EXTENED_FLAG_FALSE));   // 0x30
    VirtualKeyCodeMap.insert        ("-_",          V_KEYCODE(VK_OEM_MINUS,     EXTENED_FLAG_FALSE));   // 0xBD
    VirtualKeyCodeMap.insert        ("=+",          V_KEYCODE(VK_OEM_PLUS,      EXTENED_FLAG_FALSE));   // 0xBB
    VirtualKeyCodeMap.insert        ("Backspace",   V_KEYCODE(VK_BACK,          EXTENED_FLAG_FALSE));   // 0x08
    // Row 2
    VirtualKeyCodeMap.insert        ("Tab",         V_KEYCODE(VK_TAB,           EXTENED_FLAG_FALSE));   // 0x09
    VirtualKeyCodeMap.insert        ("Q",           V_KEYCODE(VK_Q,             EXTENED_FLAG_FALSE));   // 0x51
    VirtualKeyCodeMap.insert        ("W",           V_KEYCODE(VK_W,             EXTENED_FLAG_FALSE));   // 0x57
    VirtualKeyCodeMap.insert        ("E",           V_KEYCODE(VK_E,             EXTENED_FLAG_FALSE));   // 0x45
    VirtualKeyCodeMap.insert        ("R",           V_KEYCODE(VK_R,             EXTENED_FLAG_FALSE));   // 0x52
    VirtualKeyCodeMap.insert        ("T",           V_KEYCODE(VK_T,             EXTENED_FLAG_FALSE));   // 0x54
    VirtualKeyCodeMap.insert        ("Y",           V_KEYCODE(VK_Y,             EXTENED_FLAG_FALSE));   // 0x59
    VirtualKeyCodeMap.insert        ("U",           V_KEYCODE(VK_U,             EXTENED_FLAG_FALSE));   // 0x55
    VirtualKeyCodeMap.insert        ("I",           V_KEYCODE(VK_I,             EXTENED_FLAG_FALSE));   // 0x49
    VirtualKeyCodeMap.insert        ("O",           V_KEYCODE(VK_O,             EXTENED_FLAG_FALSE));   // 0x4F
    VirtualKeyCodeMap.insert        ("P",           V_KEYCODE(VK_P,             EXTENED_FLAG_FALSE));   // 0x50
    VirtualKeyCodeMap.insert        ("[{",          V_KEYCODE(VK_OEM_4,         EXTENED_FLAG_FALSE));   // 0xDB
    VirtualKeyCodeMap.insert        ("]}",          V_KEYCODE(VK_OEM_6,         EXTENED_FLAG_FALSE));   // 0xDD
    VirtualKeyCodeMap.insert        ("\\|",         V_KEYCODE(VK_OEM_5,         EXTENED_FLAG_FALSE));   // 0xDC
    // Row 3
    VirtualKeyCodeMap.insert        ("CapsLock",    V_KEYCODE(VK_CAPITAL,       EXTENED_FLAG_FALSE));   // 0x14
    VirtualKeyCodeMap.insert        ("A",           V_KEYCODE(VK_A,             EXTENED_FLAG_FALSE));   // 0x41
    VirtualKeyCodeMap.insert        ("S",           V_KEYCODE(VK_S,             EXTENED_FLAG_FALSE));   // 0x53
    VirtualKeyCodeMap.insert        ("D",           V_KEYCODE(VK_D,             EXTENED_FLAG_FALSE));   // 0x44
    VirtualKeyCodeMap.insert        ("F",           V_KEYCODE(VK_F,             EXTENED_FLAG_FALSE));   // 0x46
    VirtualKeyCodeMap.insert        ("G",           V_KEYCODE(VK_G,             EXTENED_FLAG_FALSE));   // 0x47
    VirtualKeyCodeMap.insert        ("H",           V_KEYCODE(VK_H,             EXTENED_FLAG_FALSE));   // 0x48
    VirtualKeyCodeMap.insert        ("J",           V_KEYCODE(VK_J,             EXTENED_FLAG_FALSE));   // 0x4A
    VirtualKeyCodeMap.insert        ("K",           V_KEYCODE(VK_K,             EXTENED_FLAG_FALSE));   // 0x4B
    VirtualKeyCodeMap.insert        ("L",           V_KEYCODE(VK_L,             EXTENED_FLAG_FALSE));   // 0x4C
    VirtualKeyCodeMap.insert        (";:",          V_KEYCODE(VK_OEM_1,         EXTENED_FLAG_FALSE));   // 0xBA
    VirtualKeyCodeMap.insert        ("'\"",         V_KEYCODE(VK_OEM_7,         EXTENED_FLAG_FALSE));   // 0xDE
    VirtualKeyCodeMap.insert        ("Enter",       V_KEYCODE(VK_RETURN,        EXTENED_FLAG_FALSE));   // 0x0D
    // Row 4
    VirtualKeyCodeMap.insert        ("L-Shift",     V_KEYCODE(VK_LSHIFT,        EXTENED_FLAG_FALSE));   // 0xA0
    VirtualKeyCodeMap.insert        ("Z",           V_KEYCODE(VK_Z,             EXTENED_FLAG_FALSE));   // 0x5A
    VirtualKeyCodeMap.insert        ("X",           V_KEYCODE(VK_X,             EXTENED_FLAG_FALSE));   // 0x58
    VirtualKeyCodeMap.insert        ("C",           V_KEYCODE(VK_C,             EXTENED_FLAG_FALSE));   // 0x43
    VirtualKeyCodeMap.insert        ("V",           V_KEYCODE(VK_V,             EXTENED_FLAG_FALSE));   // 0x56
    VirtualKeyCodeMap.insert        ("B",           V_KEYCODE(VK_B,             EXTENED_FLAG_FALSE));   // 0x42
    VirtualKeyCodeMap.insert        ("N",           V_KEYCODE(VK_N,             EXTENED_FLAG_FALSE));   // 0x4E
    VirtualKeyCodeMap.insert        ("M",           V_KEYCODE(VK_M,             EXTENED_FLAG_FALSE));   // 0x4D
    VirtualKeyCodeMap.insert        (",<",          V_KEYCODE(VK_OEM_COMMA,     EXTENED_FLAG_FALSE));   // 0xBC
    VirtualKeyCodeMap.insert        (".>",          V_KEYCODE(VK_OEM_PERIOD,    EXTENED_FLAG_FALSE));   // 0xBE
    VirtualKeyCodeMap.insert        ("/?",          V_KEYCODE(VK_OEM_2,         EXTENED_FLAG_FALSE));   // 0xBF
    VirtualKeyCodeMap.insert        ("R-Shift",     V_KEYCODE(VK_RSHIFT,        EXTENED_FLAG_TRUE ));   // 0xA1 + E
    // Row 5
    VirtualKeyCodeMap.insert        ("L-Ctrl",      V_KEYCODE(VK_LCONTROL,      EXTENED_FLAG_FALSE));   // 0xA2
    VirtualKeyCodeMap.insert        ("L-Win",       V_KEYCODE(VK_LWIN,          EXTENED_FLAG_TRUE ));   // 0x5B + E
    VirtualKeyCodeMap.insert        ("L-Alt",       V_KEYCODE(VK_LMENU,         EXTENED_FLAG_FALSE));   // 0xA4
    VirtualKeyCodeMap.insert        ("Space",       V_KEYCODE(VK_SPACE,         EXTENED_FLAG_FALSE));   // 0x20
    VirtualKeyCodeMap.insert        ("R-Alt",       V_KEYCODE(VK_RMENU,         EXTENED_FLAG_TRUE ));   // 0xA5 + E
    VirtualKeyCodeMap.insert        ("Application", V_KEYCODE(VK_APPS,          EXTENED_FLAG_TRUE ));   // 0x5D + E
    VirtualKeyCodeMap.insert        ("R-Ctrl",      V_KEYCODE(VK_RCONTROL,      EXTENED_FLAG_TRUE ));   // 0xA3 + E
    VirtualKeyCodeMap.insert        ("R-Win",       V_KEYCODE(VK_RWIN,          EXTENED_FLAG_TRUE ));   // 0x5C + E
    // Old special keys
    VirtualKeyCodeMap.insert        ("Shift",       V_KEYCODE(VK_SHIFT,         EXTENED_FLAG_FALSE));   // 0x10
    VirtualKeyCodeMap.insert        ("Ctrl",        V_KEYCODE(VK_CONTROL,       EXTENED_FLAG_FALSE));   // 0x11
    VirtualKeyCodeMap.insert        ("Alt",         V_KEYCODE(VK_MENU,          EXTENED_FLAG_FALSE));   // 0x12

    // Function Keys
    VirtualKeyCodeMap.insert        ("Esc",         V_KEYCODE(VK_ESCAPE,        EXTENED_FLAG_FALSE));   // 0x1B
    VirtualKeyCodeMap.insert        ("F1",          V_KEYCODE(VK_F1,            EXTENED_FLAG_FALSE));   // 0x70
    VirtualKeyCodeMap.insert        ("F2",          V_KEYCODE(VK_F2,            EXTENED_FLAG_FALSE));   // 0x71
    VirtualKeyCodeMap.insert        ("F3",          V_KEYCODE(VK_F3,            EXTENED_FLAG_FALSE));   // 0x72
    VirtualKeyCodeMap.insert        ("F4",          V_KEYCODE(VK_F4,            EXTENED_FLAG_FALSE));   // 0x73
    VirtualKeyCodeMap.insert        ("F5",          V_KEYCODE(VK_F5,            EXTENED_FLAG_FALSE));   // 0x74
    VirtualKeyCodeMap.insert        ("F6",          V_KEYCODE(VK_F6,            EXTENED_FLAG_FALSE));   // 0x75
    VirtualKeyCodeMap.insert        ("F7",          V_KEYCODE(VK_F7,            EXTENED_FLAG_FALSE));   // 0x76
    VirtualKeyCodeMap.insert        ("F8",          V_KEYCODE(VK_F8,            EXTENED_FLAG_FALSE));   // 0x77
    VirtualKeyCodeMap.insert        ("F9",          V_KEYCODE(VK_F9,            EXTENED_FLAG_FALSE));   // 0x78
    VirtualKeyCodeMap.insert        ("F10",         V_KEYCODE(VK_F10,           EXTENED_FLAG_FALSE));   // 0x79
    VirtualKeyCodeMap.insert        ("F11",         V_KEYCODE(VK_F11,           EXTENED_FLAG_FALSE));   // 0x7A
    VirtualKeyCodeMap.insert        ("F12",         V_KEYCODE(VK_F12,           EXTENED_FLAG_FALSE));   // 0x7B
    VirtualKeyCodeMap.insert        ("F13",         V_KEYCODE(VK_F13,           EXTENED_FLAG_FALSE));   // 0x7C
    VirtualKeyCodeMap.insert        ("F14",         V_KEYCODE(VK_F14,           EXTENED_FLAG_FALSE));   // 0x7D
    VirtualKeyCodeMap.insert        ("F15",         V_KEYCODE(VK_F15,           EXTENED_FLAG_FALSE));   // 0x7E
    VirtualKeyCodeMap.insert        ("F16",         V_KEYCODE(VK_F16,           EXTENED_FLAG_FALSE));   // 0x7F
    VirtualKeyCodeMap.insert        ("F17",         V_KEYCODE(VK_F17,           EXTENED_FLAG_FALSE));   // 0x80
    VirtualKeyCodeMap.insert        ("F18",         V_KEYCODE(VK_F18,           EXTENED_FLAG_FALSE));   // 0x81
    VirtualKeyCodeMap.insert        ("F19",         V_KEYCODE(VK_F19,           EXTENED_FLAG_FALSE));   // 0x82
    VirtualKeyCodeMap.insert        ("F20",         V_KEYCODE(VK_F20,           EXTENED_FLAG_FALSE));   // 0x83
    VirtualKeyCodeMap.insert        ("F21",         V_KEYCODE(VK_F21,           EXTENED_FLAG_FALSE));   // 0x84
    VirtualKeyCodeMap.insert        ("F22",         V_KEYCODE(VK_F22,           EXTENED_FLAG_FALSE));   // 0x85
    VirtualKeyCodeMap.insert        ("F23",         V_KEYCODE(VK_F23,           EXTENED_FLAG_FALSE));   // 0x86
    VirtualKeyCodeMap.insert        ("F24",         V_KEYCODE(VK_F24,           EXTENED_FLAG_FALSE));   // 0x87

    VirtualKeyCodeMap.insert        ("PrintScrn",   V_KEYCODE(VK_SNAPSHOT,      EXTENED_FLAG_TRUE ));   // 0x2C + E
    VirtualKeyCodeMap.insert        ("ScrollLock",  V_KEYCODE(VK_SCROLL,        EXTENED_FLAG_FALSE));   // 0x91
    VirtualKeyCodeMap.insert        ("Pause",       V_KEYCODE(VK_PAUSE,         EXTENED_FLAG_FALSE));   // 0x13

    VirtualKeyCodeMap.insert        ("Insert",      V_KEYCODE(VK_INSERT,        EXTENED_FLAG_TRUE ));   // 0x2D + E
    VirtualKeyCodeMap.insert        ("Delete",      V_KEYCODE(VK_DELETE,        EXTENED_FLAG_TRUE ));   // 0x2E + E
    VirtualKeyCodeMap.insert        ("Home",        V_KEYCODE(VK_HOME,          EXTENED_FLAG_TRUE ));   // 0x24 + E
    VirtualKeyCodeMap.insert        ("End",         V_KEYCODE(VK_END,           EXTENED_FLAG_TRUE ));   // 0x23 + E
    VirtualKeyCodeMap.insert        ("PageUp",      V_KEYCODE(VK_PRIOR,         EXTENED_FLAG_TRUE ));   // 0x21 + E
    VirtualKeyCodeMap.insert        ("PageDown",    V_KEYCODE(VK_NEXT,          EXTENED_FLAG_TRUE ));   // 0x22 + E

    VirtualKeyCodeMap.insert        ("Up",          V_KEYCODE(VK_UP,            EXTENED_FLAG_TRUE ));   // 0x26 + E
    VirtualKeyCodeMap.insert        ("Down",        V_KEYCODE(VK_DOWN,          EXTENED_FLAG_TRUE ));   // 0x28 + E
    VirtualKeyCodeMap.insert        ("Left",        V_KEYCODE(VK_LEFT,          EXTENED_FLAG_TRUE ));   // 0x25 + E
    VirtualKeyCodeMap.insert        ("Right",       V_KEYCODE(VK_RIGHT,         EXTENED_FLAG_TRUE ));   // 0x27 + E

    //NumberPad Keys
    VirtualKeyCodeMap.insert        ("NumLock",     V_KEYCODE(VK_NUMLOCK,       EXTENED_FLAG_TRUE ));   // 0x90 + E
    VirtualKeyCodeMap.insert        ("Num /",       V_KEYCODE(VK_DIVIDE,        EXTENED_FLAG_TRUE ));   // 0x6F + E
    VirtualKeyCodeMap.insert        ("Num *",       V_KEYCODE(VK_MULTIPLY,      EXTENED_FLAG_FALSE));   // 0x6A
    VirtualKeyCodeMap.insert        ("Num -",       V_KEYCODE(VK_SUBTRACT,      EXTENED_FLAG_FALSE));   // 0x6D
    VirtualKeyCodeMap.insert        ("Num +",       V_KEYCODE(VK_ADD,           EXTENED_FLAG_FALSE));   // 0x6B
    VirtualKeyCodeMap.insert        ("Num .",       V_KEYCODE(VK_DECIMAL,       EXTENED_FLAG_FALSE));   // 0x6E
    VirtualKeyCodeMap.insert        ("Num 0",       V_KEYCODE(VK_NUMPAD0,       EXTENED_FLAG_FALSE));   // 0x60
    VirtualKeyCodeMap.insert        ("Num 1",       V_KEYCODE(VK_NUMPAD1,       EXTENED_FLAG_FALSE));   // 0x61
    VirtualKeyCodeMap.insert        ("Num 2",       V_KEYCODE(VK_NUMPAD2,       EXTENED_FLAG_FALSE));   // 0x62
    VirtualKeyCodeMap.insert        ("Num 3",       V_KEYCODE(VK_NUMPAD3,       EXTENED_FLAG_FALSE));   // 0x63
    VirtualKeyCodeMap.insert        ("Num 4",       V_KEYCODE(VK_NUMPAD4,       EXTENED_FLAG_FALSE));   // 0x64
    VirtualKeyCodeMap.insert        ("Num 5",       V_KEYCODE(VK_NUMPAD5,       EXTENED_FLAG_FALSE));   // 0x65
    VirtualKeyCodeMap.insert        ("Num 6",       V_KEYCODE(VK_NUMPAD6,       EXTENED_FLAG_FALSE));   // 0x66
    VirtualKeyCodeMap.insert        ("Num 7",       V_KEYCODE(VK_NUMPAD7,       EXTENED_FLAG_FALSE));   // 0x67
    VirtualKeyCodeMap.insert        ("Num 8",       V_KEYCODE(VK_NUMPAD8,       EXTENED_FLAG_FALSE));   // 0x68
    VirtualKeyCodeMap.insert        ("Num 9",       V_KEYCODE(VK_NUMPAD9,       EXTENED_FLAG_FALSE));   // 0x69
    VirtualKeyCodeMap.insert        ("Num Enter",   V_KEYCODE(VK_RETURN,        EXTENED_FLAG_TRUE ));   // 0x0D + E
    //NumLock Off NumberPadKeys
    VirtualKeyCodeMap.insertMulti   ("Num .",       V_KEYCODE(VK_DELETE,        EXTENED_FLAG_FALSE));   // 0x2E
    VirtualKeyCodeMap.insertMulti   ("Num 0",       V_KEYCODE(VK_INSERT,        EXTENED_FLAG_FALSE));   // 0x2D
    VirtualKeyCodeMap.insertMulti   ("Num 1",       V_KEYCODE(VK_END,           EXTENED_FLAG_FALSE));   // 0x23
    VirtualKeyCodeMap.insertMulti   ("Num 2",       V_KEYCODE(VK_DOWN,          EXTENED_FLAG_FALSE));   // 0x28
    VirtualKeyCodeMap.insertMulti   ("Num 3",       V_KEYCODE(VK_NEXT,          EXTENED_FLAG_FALSE));   // 0x22
    VirtualKeyCodeMap.insertMulti   ("Num 4",       V_KEYCODE(VK_LEFT,          EXTENED_FLAG_FALSE));   // 0x25
    VirtualKeyCodeMap.insertMulti   ("Num 5",       V_KEYCODE(VK_CLEAR,         EXTENED_FLAG_FALSE));   // 0x0C
    VirtualKeyCodeMap.insertMulti   ("Num 6",       V_KEYCODE(VK_RIGHT,         EXTENED_FLAG_FALSE));   // 0x27
    VirtualKeyCodeMap.insertMulti   ("Num 7",       V_KEYCODE(VK_HOME,          EXTENED_FLAG_FALSE));   // 0x24
    VirtualKeyCodeMap.insertMulti   ("Num 8",       V_KEYCODE(VK_UP,            EXTENED_FLAG_FALSE));   // 0x26
    VirtualKeyCodeMap.insertMulti   ("Num 9",       V_KEYCODE(VK_PRIOR,         EXTENED_FLAG_FALSE));   // 0x21
}

void QKeyMapper::initVirtualMouseButtonMap()
{
    VirtualMouseButtonMap.insert("L-Mouse",     V_MOUSECODE(MOUSEEVENTF_LEFTDOWN,       MOUSEEVENTF_LEFTUP  ));   // Left Mouse Button
    VirtualMouseButtonMap.insert("R-Mouse",     V_MOUSECODE(MOUSEEVENTF_RIGHTDOWN,      MOUSEEVENTF_RIGHTUP ));   // Right Mouse Button
    VirtualMouseButtonMap.insert("M-Mouse",     V_MOUSECODE(MOUSEEVENTF_MIDDLEDOWN,     MOUSEEVENTF_MIDDLEUP));   // Middle Mouse Button

    MouseButtonNameMap.insert(WM_LBUTTONDOWN,   "L-Mouse Button Down");
    MouseButtonNameMap.insert(WM_LBUTTONUP,     "L-Mouse Button Up");
    MouseButtonNameMap.insert(WM_RBUTTONDOWN,   "R-Mouse Button Down");
    MouseButtonNameMap.insert(WM_RBUTTONUP,     "R-Mouse Button Up");
    MouseButtonNameMap.insert(WM_MBUTTONDOWN,   "M-Mouse Button Down");
    MouseButtonNameMap.insert(WM_MBUTTONUP,     "M-Mouse Button Up");
}

void QKeyMapper::initProcessInfoTable(void)
{
    //ui->processinfoTable->setStyle(QStyleFactory::create("windows"));
    ui->processinfoTable->setFocusPolicy(Qt::NoFocus);
    ui->processinfoTable->setColumnCount(PROCESSINFO_TABLE_COLUMN_COUNT);
    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << "Name"
                                                                    << "PID"
                                                                    << "Title" );

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
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[LoadSetting]" << "Icon availableSizes:" << fileicon.availableSizes();
#endif
            QSize selectedSize = QSize(0, 0);
            for(const QSize &iconsize : fileicon.availableSizes()){
                if ((iconsize.width() <= DEFAULT_ICON_WIDTH)
                        && (iconsize.height() <= DEFAULT_ICON_HEIGHT)){
                    selectedSize = iconsize;
                }
            }

            if ((selectedSize.width() == 0)
                    || (selectedSize.height() == 0)){
                selectedSize = QSize(DEFAULT_ICON_WIDTH, DEFAULT_ICON_HEIGHT);
            }
            QPixmap IconPixmap = m_MapProcessInfo.WindowIcon.pixmap(selectedSize);
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
            ui->iconLabel->setPixmap(m_MapProcessInfo.WindowIcon.pixmap(QSize(DEFAULT_ICON_WIDTH, DEFAULT_ICON_HEIGHT)));
        }
    }
}

void QKeyMapper::initKeyMappingDataTable(void)
{
    //ui->keymapdataTable->setStyle(QStyleFactory::create("windows"));
    ui->keymapdataTable->setFocusPolicy(Qt::NoFocus);
    ui->keymapdataTable->setColumnCount(KEYMAPPINGDATA_TABLE_COLUMN_COUNT);
    ui->keymapdataTable->setHorizontalHeaderLabels(QStringList()   << "Original Key"
                                                                    << "Mapping Key"
                                                                    << "Burst"
                                                                    << "Lock");

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
#ifdef SUPPORT_MOUSE_BUTTON
            << "L-Mouse"
            << "R-Mouse"
            << "M-Mouse"
#endif
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
#ifdef SUPPORT_MOUSE_BUTTON
    orikeycodelist.removeOne("L-Mouse");
    orikeycodelist.removeOne("R-Mouse");
    orikeycodelist.removeOne("M-Mouse");
#endif
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
        for (const MAP_KEYDATA &keymapdata : KeyMappingDataList)
        {
            /* ORIGINAL_KEY_COLUMN */
            ui->keymapdataTable->setItem(rowindex, ORIGINAL_KEY_COLUMN  , new QTableWidgetItem(keymapdata.Original_Key));

            /* MAPPING_KEY_COLUMN */
            QString mappingkeys_str;
            if (keymapdata.Mapping_Keys.size() == 1) {
                mappingkeys_str = keymapdata.Mapping_Keys.constFirst();
            }
            else {
                mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_STR);
            }
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

void QKeyMapper::updateLockStatusDisplay()
{
    int rowindex = 0;
    for (const MAP_KEYDATA &keymapdata : KeyMappingDataList)
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

void QKeyMapper::clearAllBurstTimersAndLockKeys()
{
    QList<QString> burstKeyUpKeys = m_BurstKeyUpTimerMap.keys();
    for (const QString &key : burstKeyUpKeys) {
        int timerID = m_BurstTimerMap.value(key, 0);
        if (timerID > 0) {
            killTimer(timerID);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("startBurstTimer(): Key \"%s\" could not find in m_BurstKeyUpTimerMap!!!", key.toStdString().c_str());
#endif
        }
    }

    QList<QString> burstKeys = m_BurstTimerMap.keys();
    for (const QString &key : burstKeys) {
        int timerID = m_BurstTimerMap.value(key, 0);
        if (timerID > 0) {
            int findindex = findInKeyMappingDataList(key);
            if (findindex >= 0) {
                if (true == KeyMappingDataList.at(findindex).Lock) {
                    if (true == pressedLockKeysList.contains(key)){
                        KeyMappingDataList[findindex].LockStatus = false;
                        pressedLockKeysList.removeAll(key);
#ifdef DEBUG_LOGOUT_ON
                        qDebug("clearAllBurstTimersAndLockKeys() : Key \"%s\" KeyDown LockStatus -> OFF", key.toStdString().c_str());
#endif
                    }
                }

                emit stopBurstTimer_Signal(key, findindex);
            }
        }
    }

    for (int index = 0; index < KeyMappingDataList.size(); index++) {
        KeyMappingDataList[index].LockStatus = false;
    }
}

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

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[SelectProcessInfo]" << "Icon availableSizes:" << fileicon.availableSizes();
#endif
        QSize selectedSize = QSize(0, 0);
        for(const QSize &iconsize : fileicon.availableSizes()){
            if ((iconsize.width() <= DEFAULT_ICON_WIDTH)
                    && (iconsize.height() <= DEFAULT_ICON_HEIGHT)){
                selectedSize = iconsize;
            }
        }

        if ((selectedSize.width() == 0)
                || (selectedSize.height() == 0)){
            selectedSize = QSize(DEFAULT_ICON_WIDTH, DEFAULT_ICON_HEIGHT);
        }
        QPixmap IconPixmap = m_MapProcessInfo.WindowIcon.pixmap(selectedSize);
        ui->iconLabel->setPixmap(IconPixmap);

        ui->nameLineEdit->setToolTip(ProcessPath);
    }
}

void QKeyMapper::on_addmapdataButton_clicked()
{
    if ((true == VirtualKeyCodeMap.contains(m_orikeyComboBox->currentText())
            || true == VirtualMouseButtonMap.contains(m_orikeyComboBox->currentText()))
        && (true == VirtualKeyCodeMap.contains(m_mapkeyComboBox->currentText())
            || true == VirtualMouseButtonMap.contains(m_mapkeyComboBox->currentText()))){
        bool already_exist = false;
        int findindex = findInKeyMappingDataList(m_orikeyComboBox->currentText());

        if (findindex != -1){
            if (KeyMappingDataList.at(findindex).Mapping_Keys.contains(m_mapkeyComboBox->currentText()) == true){
                already_exist = true;
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "KeyMap already exist at KeyMappingDataList index : " << findindex;
#endif
            }
        }

        if (false == already_exist){
            if (findindex != -1){
                MAP_KEYDATA keymapdata = KeyMappingDataList.at(findindex);
                QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_STR);
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "mappingkeys_str before add:" << mappingkeys_str;
#endif
                mappingkeys_str = mappingkeys_str + SEPARATOR_STR + m_mapkeyComboBox->currentText();

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

    QString keycodeString = QKeyMapper::VirtualKeyCodeMap.key(vkeycode);

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
    }
}

void QKeyMapper::on_settingselectComboBox_currentIndexChanged(int index)
{
    int settingIndex = index + 1;

    if (settingIndex > 0 ) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[settingselectComboBox] Change to Setting [%d]", settingIndex);
#endif
        bool loadresult = loadKeyMapSetting(settingIndex);
        Q_UNUSED(loadresult);
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug("[settingselectComboBox] Select settingIndex error [%d]", settingIndex);
#endif
    }
}
