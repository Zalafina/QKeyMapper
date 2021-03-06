﻿#include "qkeymapper.h"
#include "ui_qkeymapper.h"

//static const uint WIN_TITLESTR_MAX = 200U;
static const uint CYCLE_CHECK_TIMEOUT = 1000U;
static const int PROCESSINFO_TABLE_COLUMN_COUNT = 3;
static const int KEYMAPPINGDATA_TABLE_COLUMN_COUNT = 2;

static const int PROCESS_NAME_COLUMN = 0;
static const int PROCESS_PID_COLUMN = 1;
static const int PROCESS_TITLE_COLUMN = 2;

static const int PROCESS_NAME_COLUMN_WIDTH_MAX = 200;

static const int ORIGINAL_KEY_COLUMN = 0;
static const int MAPPING_KEY_COLUMN = 1;

static const int DEFAULT_ICON_WIDTH = 64;
static const int DEFAULT_ICON_HEIGHT = 64;

static const QString DEFAULT_NAME("AWVSSAO.exe");
static const QString DEFAULT_TITLE("Accel World vs. Sword Art Online");

static const QString KEYMAPDATA_ORIGINALKEYS("KeyMapData/OriginalKeys");
static const QString KEYMAPDATA_MAPPINGKEYS("KeyMapData/MappingKeys");
static const QString CLEARALL("KeyMapData/ClearAll");

static const QString PROCESSINFO_FILENAME("ProcessInfo/FileName");
static const QString PROCESSINFO_WINDOWTITLE("ProcessInfo/WindowTitle");
static const QString PROCESSINFO_FILEPATH("ProcessInfo/FilePath");

static const QString SAO_FONTFILENAME(":/sao_ui.otf");

QList<MAP_PROCESSINFO> QKeyMapper::static_ProcessInfoList = QList<MAP_PROCESSINFO>();
QHash<QString, V_KEYCODE> QKeyMapper::VirtualKeyCodeMap = QHash<QString, V_KEYCODE>();
QList<MAP_KEYDATA> QKeyMapper::KeyMappingDataList = QList<MAP_KEYDATA>();
GetDeviceStateT QKeyMapper::FuncPtrGetDeviceState = Q_NULLPTR;
GetDeviceDataT QKeyMapper::FuncPtrGetDeviceData = Q_NULLPTR;
QComboBox *QKeyMapper::orikeyComboBox_static = Q_NULLPTR;
QComboBox *QKeyMapper::mapkeyComboBox_static = Q_NULLPTR;

QKeyMapper::QKeyMapper(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QKeyMapper),
    m_KeyMapStatus(KEYMAP_IDLE),
    m_CycleCheckTimer(this),
    m_MapProcessInfo(),
    m_SysTrayIcon(Q_NULLPTR),
    m_KeyHook(Q_NULLPTR),
    m_DirectInput(Q_NULLPTR),
    m_SAO_FontFamilyID(-1),
    m_SAO_FontName(),
    m_ProcessInfoTableDelegate(Q_NULLPTR),
    m_KeyMappingDataTableDelegate(Q_NULLPTR),
    m_orikeyComboBox(new KeyListComboBox(this)),
    m_mapkeyComboBox(new KeyListComboBox(this)),
    m_HotKey(new QHotkey(this))
{
    ui->setupUi(this);
    initAddKeyComboBoxes();
    orikeyComboBox_static = m_orikeyComboBox;
    mapkeyComboBox_static = m_mapkeyComboBox;
    loadFontFile(SAO_FONTFILENAME, m_SAO_FontFamilyID, m_SAO_FontName);

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

    initVirtualKeyCodeMap();
    initProcessInfoTable();
    ui->nameCheckBox->setFocusPolicy(Qt::NoFocus);
    ui->titleCheckBox->setFocusPolicy(Qt::NoFocus);
    ui->nameLineEdit->setFocusPolicy(Qt::NoFocus);
    ui->titleLineEdit->setFocusPolicy(Qt::NoFocus);

    initKeyMappingDataTable();
    bool loadresult = loadKeyMapSetting();
    Q_UNUSED(loadresult);

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

    m_SysTrayIcon = new QSystemTrayIcon(this);
    m_SysTrayIcon->setIcon(QIcon(":/AppIcon.ico"));
    m_SysTrayIcon->setToolTip("QKeyMapper(Idle)");
    m_SysTrayIcon->show();

    QObject::connect(m_SysTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(SystrayIconActivated(QSystemTrayIcon::ActivationReason)));
    QObject::connect(&m_CycleCheckTimer, SIGNAL(timeout()), this, SLOT(cycleCheckProcessProc()));

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
        bool checkresult = false;
        QString windowTitle;
        QString filename;
        HWND hwnd = GetForegroundWindow();
        TCHAR titleBuffer[MAX_PATH];
        memset(titleBuffer, 0x00, sizeof(titleBuffer));

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
                    checkresult = true;
                }
            }
            else if (true == ui->nameCheckBox->isChecked()){
                if (m_MapProcessInfo.FileName == filename){
                    checkresult = true;
                }
            }
            else if (true == ui->titleCheckBox->isChecked()){
                if (m_MapProcessInfo.WindowTitle == windowTitle){
                    checkresult = true;
                }
            }
            else{
                checkresult = true;
            }

            if ((m_MapProcessInfo.FileName == filename)
                    && (m_MapProcessInfo.WindowTitle == windowTitle)){
                checkresult = true;
            }
        }

        if (true == checkresult){
            if (KEYMAP_CHECKING == m_KeyMapStatus){
                setKeyHook(hwnd);
                //setDInputKeyHook(hwnd);
                m_KeyMapStatus = KEYMAP_MAPPING;

#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "[cycleCheckProcessProc]" << " KeyMapStatus change (" << m_KeyMapStatus << ") " << "ForegroundWindow: " << windowTitle << "(" << filename << ")";
#endif
            }
        }
        else{
            if (KEYMAP_MAPPING == m_KeyMapStatus){
                setKeyUnHook();
                setDInputKeyUnHook();
                m_KeyMapStatus = KEYMAP_CHECKING;

#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[cycleCheckProcessProc]" << " KeyMapStatus change (" << m_KeyMapStatus << ") " << "ForegroundWindow: " << windowTitle << "(" << filename << ")";
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
        m_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper::LowLevelKeyboardHookProc, GetModuleHandle(Q_NULLPTR), 0);
        qDebug().nospace().noquote() << "[setKeyHook] " << "Normal Key Hook Started.";
    }
    else{
        qDebug().nospace().noquote() << "[setKeyHook] " << "Error: Invisible Window Handle!";
    }
}

void QKeyMapper::setKeyUnHook(void)
{
    if (m_KeyHook != Q_NULLPTR){
        UnhookWindowsHookEx(m_KeyHook);
        m_KeyHook = Q_NULLPTR;
        qDebug().nospace().noquote() << "[setKeyUnHook] " << "Normal Key Hook Released.";
    }
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
    if (KEYMAP_IDLE == m_KeyMapStatus){
        if ((false == m_MapProcessInfo.FileName.isEmpty())
                && (false == m_MapProcessInfo.WindowTitle.isEmpty())){
            m_CycleCheckTimer.start(CYCLE_CHECK_TIMEOUT);
            m_SysTrayIcon->setToolTip("QKeyMapper(Mapping : " + m_MapProcessInfo.FileName + ")");
            m_SysTrayIcon->setIcon(QIcon(":/AppIcon_Working.png"));
            ui->keymapButton->setText("KeyMappingStop");
            m_KeyMapStatus = KEYMAP_CHECKING;

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[KeyMappingButton]" << " KeyMapStatus change (" << m_KeyMapStatus << ") " << "on_keymapButton_clicked()";
#endif
        }
        else{
            QMessageBox::warning(this, tr("QKeyMapper"), tr("Invalid process info for key mapping."));
        }
    }
    else{
        m_CycleCheckTimer.stop();
        m_SysTrayIcon->setToolTip("QKeyMapper(Idle)");
        m_SysTrayIcon->setIcon(QIcon(":/AppIcon.ico"));
        ui->keymapButton->setText("KeyMappingStart");
        setKeyUnHook();
        m_KeyMapStatus = KEYMAP_IDLE;

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[KeyMappingButton]" << " KeyMapStatus change (" << m_KeyMapStatus << ") " << "on_keymapButton_clicked()";
#endif
    }

    if (m_KeyMapStatus != KEYMAP_IDLE){
        changeControlEnableStatus(false);
    }
    else{
        changeControlEnableStatus(true);
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

void QKeyMapper::saveKeyMapSetting(void)
{
    if (ui->keymapdataTable->rowCount() == KeyMappingDataList.size()){
        QSettings settingFile(QString("keymapdata.ini"), QSettings::IniFormat);
        settingFile.setIniCodec("UTF-8");
        QStringList original_keys;
        QStringList mapping_keys;

        if (KeyMappingDataList.size() > 0){
            for (const MAP_KEYDATA &keymapdata : KeyMappingDataList)
            {
                original_keys << keymapdata.Original_Key;
                mapping_keys  << keymapdata.Mapping_Key;
            }
            settingFile.setValue(KEYMAPDATA_ORIGINALKEYS, original_keys );
            settingFile.setValue(KEYMAPDATA_MAPPINGKEYS , mapping_keys  );

            settingFile.remove(CLEARALL);
        }
        else{
            settingFile.setValue(KEYMAPDATA_ORIGINALKEYS, original_keys );
            settingFile.setValue(KEYMAPDATA_MAPPINGKEYS , mapping_keys  );
            settingFile.setValue(CLEARALL, QString("ClearList"));
        }

        if ((false == ui->nameLineEdit->text().isEmpty())
                && (false == ui->titleLineEdit->text().isEmpty())
                && (ui->nameLineEdit->text() == m_MapProcessInfo.FileName)
                && (ui->titleLineEdit->text() == m_MapProcessInfo.WindowTitle)){
            settingFile.setValue(PROCESSINFO_FILENAME, m_MapProcessInfo.FileName);
            settingFile.setValue(PROCESSINFO_WINDOWTITLE, m_MapProcessInfo.WindowTitle);

            if (false == m_MapProcessInfo.FilePath.isEmpty()){
                settingFile.setValue(PROCESSINFO_FILEPATH, m_MapProcessInfo.FilePath);
            }
            else{
#ifdef DEBUG_LOGOUT_ON
                qDebug() << "[saveKeyMapSetting]" << "FilePath is empty, unsaved.";
#endif
            }
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

bool QKeyMapper::loadKeyMapSetting(void)
{
    bool clearallcontainsflag = true;
    quint8 datavalidflag = 0xFF;
    QSettings settingFile(QString("keymapdata.ini"), QSettings::IniFormat);
    settingFile.setIniCodec("UTF-8");

    if (false == settingFile.contains(CLEARALL)){
        clearallcontainsflag = false;
    }

    if (false == clearallcontainsflag){
        QStringList original_keys;
        QStringList mapping_keys;
        QList<MAP_KEYDATA> loadkeymapdata;

        if ((true == settingFile.contains(KEYMAPDATA_ORIGINALKEYS))
                && (true == settingFile.contains(KEYMAPDATA_MAPPINGKEYS))){
            original_keys = settingFile.value(KEYMAPDATA_ORIGINALKEYS).toStringList();
            mapping_keys  = settingFile.value(KEYMAPDATA_MAPPINGKEYS ).toStringList();

            if (original_keys.size() == mapping_keys.size()){
                datavalidflag = true;

                if (original_keys.size() > 0){
                    int loadindex = 0;
                    for (const QString &ori_key : original_keys){

                        if ((true == VirtualKeyCodeMap.contains(ori_key))
                                && (true == VirtualKeyCodeMap.contains(mapping_keys.at(loadindex)))){
                            loadkeymapdata.append(MAP_KEYDATA(ori_key, mapping_keys.at(loadindex)));
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
            KeyMappingDataList.append(MAP_KEYDATA("L-Shift",          "-_"            ));
            KeyMappingDataList.append(MAP_KEYDATA("L-Ctrl",           "=+"            ));
            KeyMappingDataList.append(MAP_KEYDATA("I",                "Up"            ));
            KeyMappingDataList.append(MAP_KEYDATA("K",                "Down"          ));
            KeyMappingDataList.append(MAP_KEYDATA("H",                "Left"          ));
            KeyMappingDataList.append(MAP_KEYDATA("J",                "Right"         ));
            KeyMappingDataList.append(MAP_KEYDATA("Tab",              "0)"            ));
        }
        else{
            KeyMappingDataList = loadkeymapdata;
        }
    }
    else{
        KeyMappingDataList.clear();
    }

    if (false == KeyMappingDataList.isEmpty()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << " KeyMappingDataList Start >>>";
#endif
        int rowindex = 0;
        ui->keymapdataTable->setRowCount(KeyMappingDataList.size());
        for (const MAP_KEYDATA &keymapdata : KeyMappingDataList)
        {
            ui->keymapdataTable->setItem(rowindex, ORIGINAL_KEY_COLUMN  , new QTableWidgetItem(keymapdata.Original_Key));
            ui->keymapdataTable->setItem(rowindex, MAPPING_KEY_COLUMN   , new QTableWidgetItem(keymapdata.Mapping_Key));

            rowindex += 1;

#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[loadKeyMapSetting]" << keymapdata.Original_Key << "to" << keymapdata.Mapping_Key;
#endif
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[loadKeyMapSetting]" << " KeyMappingDataList End   <<<";
#endif
    }

    if ((true == settingFile.contains(PROCESSINFO_FILENAME))
            && (true == settingFile.contains(PROCESSINFO_WINDOWTITLE))){
        m_MapProcessInfo.FileName = settingFile.value(PROCESSINFO_FILENAME).toString();
        m_MapProcessInfo.WindowTitle = settingFile.value(PROCESSINFO_WINDOWTITLE).toString();

        ui->nameLineEdit->setText(m_MapProcessInfo.FileName);
        ui->titleLineEdit->setText(m_MapProcessInfo.WindowTitle);
    }

    if (true == settingFile.contains(PROCESSINFO_FILEPATH)){
        m_MapProcessInfo.FilePath = settingFile.value(PROCESSINFO_FILEPATH).toString();
    }

    if (false == datavalidflag){
        QMessageBox::warning(this, tr("QKeyMapper"), tr("<html><head/><body><p align=\"center\">Load invalid keymapdata from ini file.</p><p align=\"center\">Reset to default values.</p></body></html>"));
        return false;
    }
    else{
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

    ui->processinfoTable->horizontalHeader()->setFont(customFont);
    ui->keymapdataTable->horizontalHeader()->setFont(customFont);
}

void QKeyMapper::changeControlEnableStatus(bool status)
{
    ui->nameCheckBox->setEnabled(status);
    ui->titleCheckBox->setEnabled(status);
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

void QKeyMapper::on_savemaplistButton_clicked()
{
    saveKeyMapSetting();
}

LRESULT QKeyMapper::LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;

#if 1
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

    if (false == keycodeString.isEmpty()){

        if (pKeyBoard->scanCode != 0){
            int findindex = findInKeyMappingDataList(keycodeString);

            if (findindex >=0){
                V_KEYCODE map_vkeycode = VirtualKeyCodeMap.value(KeyMappingDataList.at(findindex).Mapping_Key);
                DWORD extenedkeyflag = 0;
                if (true == map_vkeycode.ExtenedFlag){
                    extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
                }
                else{
                    extenedkeyflag = 0;
                }

                if (WM_KEYDOWN == wParam){
                    keybd_event(map_vkeycode.KeyCode, 0, extenedkeyflag | 0, 0);
                    returnFlag = true;
                }
                else if (WM_KEYUP == wParam){
                    keybd_event(map_vkeycode.KeyCode, 0, extenedkeyflag | KEYEVENTF_KEYUP, 0);
                    returnFlag = true;
                }
                else{
                    // do nothing.
                }
            }
        }

#ifdef DEBUG_LOGOUT_ON
        if (WM_KEYDOWN == wParam){
            qDebug("\"%s\" (0x%02X) KeyDown, scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
        }
        else if (WM_KEYUP == wParam){
            qDebug("\"%s\" (0x%02X) KeyUp, scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
        }
        else{
        }
#endif
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug("UnknownKey (0x%02X) KeyDown, scanCode(0x%08X), flags(0x%08X)", pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
#endif
    }

    if (true == returnFlag){
        return 1;
    }
    else{
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

#else
    switch (pKeyBoard->vkCode)
    {
//    case VK_RETURN:
//        if (WM_KEYDOWN == wParam){
//            if (LLKHF_EXTENDED == (pKeyBoard->flags & LLKHF_EXTENDED)){
//                qDebug() << "Num Enter PressDown";
//            }else{
//                qDebug() << "Enter PressDown";
//            }
//        }
//        else if (WM_KEYUP == wParam){
//            if (LLKHF_EXTENDED == (pKeyBoard->flags & LLKHF_EXTENDED)){
//                qDebug() << "Num Enter ReleaseUp";
//            }else{
//                qDebug() << "Enter ReleaseUp";
//            }
//        }
//        else{
//            // do nothing.
//        }
//        break;
    default:
        break;
    }

    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
#endif
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

void QKeyMapper::initKeyMappingDataTable(void)
{
    //ui->keymapdataTable->setStyle(QStyleFactory::create("windows"));
    ui->keymapdataTable->setFocusPolicy(Qt::NoFocus);
    ui->keymapdataTable->setColumnCount(KEYMAPPINGDATA_TABLE_COLUMN_COUNT);
    ui->keymapdataTable->setHorizontalHeaderLabels(QStringList()   << "Original Key"
                                                                    << "Mapping Key" );

    ui->keymapdataTable->horizontalHeader()->setStretchLastSection(true);
    ui->keymapdataTable->horizontalHeader()->setHighlightSections(false);
    ui->keymapdataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->keymapdataTable->setColumnWidth(ORIGINAL_KEY_COLUMN, ui->keymapdataTable->width()/2);
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

    m_orikeyComboBox->addItems(keycodelist);
    m_mapkeyComboBox->addItems(keycodelist);
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
    if ((true == VirtualKeyCodeMap.contains(m_orikeyComboBox->currentText()))
            && (true == VirtualKeyCodeMap.contains(m_mapkeyComboBox->currentText()))){
        if (m_orikeyComboBox->currentText() != m_mapkeyComboBox->currentText()){

            int findindex = findInKeyMappingDataList(m_orikeyComboBox->currentText());

            if (-1 == findindex){
                KeyMappingDataList.append(MAP_KEYDATA(m_orikeyComboBox->currentText(), m_mapkeyComboBox->currentText()));

#ifdef DEBUG_LOGOUT_ON
                qDebug() << "Add keymapdata :" << m_orikeyComboBox->currentText() << "to" << m_mapkeyComboBox->currentText();
#endif

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
                        ui->keymapdataTable->setItem(rowindex, ORIGINAL_KEY_COLUMN  , new QTableWidgetItem(keymapdata.Original_Key));
                        ui->keymapdataTable->setItem(rowindex, MAPPING_KEY_COLUMN   , new QTableWidgetItem(keymapdata.Mapping_Key));

                        rowindex += 1;

#ifdef DEBUG_LOGOUT_ON
                        qDebug() << keymapdata.Original_Key << "to" << keymapdata.Mapping_Key;
#endif
                    }

#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "KeyMappingDataList End   <<<";
#endif
                }
            }
            else{
                QMessageBox::warning(this, tr("QKeyMapper"), tr("Conflict with exist Keys."));
            }
        }
        else{
            QMessageBox::warning(this, tr("QKeyMapper"), tr("Should not mapping to the same key."));
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
