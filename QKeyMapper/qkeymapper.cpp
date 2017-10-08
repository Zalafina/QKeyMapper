#include "qkeymapper.h"
#include "ui_qkeymapper.h"

//static const uint WIN_TITLESTR_MAX = 200U;
static const uint CYCLE_CHECK_TIMEOUT = 5000U;
static const int PROCESSINFO_TABLE_COLUMN = 3;

static const int PROCESS_NAME_COLUMN = 0;
static const int PROCESS_PID_COLUMN = 1;
static const int PROCESS_TITLE_COLUMN = 2;

QList<MAP_PROCESSINFO> QKeyMapper::static_ProcessInfoList = QList<MAP_PROCESSINFO>();

QKeyMapper::QKeyMapper(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QKeyMapper),
    m_KeyMapStatus(KEYMAP_IDLE),
    m_CycleCheckTimer(this),
    m_MapProcessInfo(),
    m_SysTrayIcon(NULL),
    m_KeyHook(NULL)
{
    ui->setupUi(this);
    initProcessInfoTable();
    ui->nameCheckBox->setFocusPolicy(Qt::NoFocus);
    ui->titleCheckBox->setFocusPolicy(Qt::NoFocus);
    ui->nameLineEdit->setFocusPolicy(Qt::NoFocus);
    ui->titleLineEdit->setFocusPolicy(Qt::NoFocus);

    loadKeyMapSetting();

    m_SysTrayIcon = new QSystemTrayIcon(this);
    m_SysTrayIcon->setIcon(QIcon(":/AppIcon.ico"));
    m_SysTrayIcon->setToolTip("QKeyMapper(Idle)");
    m_SysTrayIcon->show();

    QObject::connect(m_SysTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(SystrayIconActivated(QSystemTrayIcon::ActivationReason)));
    QObject::connect(&m_CycleCheckTimer, SIGNAL(timeout()), this, SLOT(cycleCheckProcessProc()));

    //m_CycleCheckTimer.start(CYCLE_CHECK_TIMEOUT);
    refreshProcessInfoTable();
}

QKeyMapper::~QKeyMapper()
{
    setKeyUnHook();

    delete ui;

    delete m_SysTrayIcon;
    m_SysTrayIcon = NULL;
}

void QKeyMapper::WindowStateChangedProc(void)
{
    if (true == isMinimized()){
#ifdef DEBUG_LOGOUT_ON
        qDebug("QKeyMapper::WindowStateChangedProc() -> Window Minimized: setHidden!");
#endif
        hide();
    }
}

void QKeyMapper::cycleCheckProcessProc(void)
{
    if (KEYMAP_IDLE != m_KeyMapStatus){
        bool checkresult = false;
        HWND hwnd = GetForegroundWindow();
        TCHAR titleBuffer[MAX_PATH];
        memset(titleBuffer, 0x00, sizeof(titleBuffer));

        int resultLength = GetWindowText(hwnd, titleBuffer, MAX_PATH);
        if (resultLength){
            QString filename;
            QString ProcessPath;
            getProcessInfoFromHWND( hwnd, ProcessPath);

            QString windowTitle = QString::fromWCharArray(titleBuffer);

            if (false == windowTitle.isEmpty() && false == ProcessPath.isEmpty()){
                QFileInfo fileinfo(ProcessPath);
                filename = fileinfo.fileName();
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << "ForegroundWindow: " << windowTitle << "(" << filename << ")";
#endif
            }

            if ((m_MapProcessInfo.FileName == filename)
                    && (m_MapProcessInfo.WindowTitle == windowTitle)){
                checkresult = true;
            }
        }

        if (true == checkresult){
            if (KEYMAP_CHECKING == m_KeyMapStatus){
                setKeyHook();
                m_KeyMapStatus = KEYMAP_MAPPING;

#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "KeyMapStatus change (" << m_KeyMapStatus << ")";
#endif
            }
        }
        else{
            if (KEYMAP_MAPPING == m_KeyMapStatus){
                setKeyUnHook();
                m_KeyMapStatus = KEYMAP_CHECKING;

#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "KeyMapStatus change (" << m_KeyMapStatus << ")";
#endif
            }
        }
    }
    else{
        //EnumWindows((WNDENUMPROC)QKeyMapper::EnumWindowsProc, 0);
    }
}

void QKeyMapper::setKeyHook(void)
{
    m_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper::LowLevelKeyboardHookProc, GetModuleHandle(NULL),0);
}

void QKeyMapper::setKeyUnHook(void)
{
    if (m_KeyHook != NULL){
        UnhookWindowsHookEx(m_KeyHook);
        m_KeyHook = NULL;
    }
}

void QKeyMapper::setMapProcessInfo(QString &filename, QString &windowtitle)
{
    if ((false == filename.isEmpty())
            && (false == windowtitle.isEmpty())){
        m_MapProcessInfo.FileName = filename;
        m_MapProcessInfo.WindowTitle = windowtitle;
    }
    else{
        qDebug().nospace().noquote() << "setMapProcessInfo Error: filename(" << filename << "), " << "windowtitle(" << windowtitle << ")";
    }
}

void QKeyMapper::getProcessInfoFromPID(DWORD processID, QString &processPathStr)
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    // Get a handle to the process.
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );

    // Get the process name.
    if (NULL != hProcess )
    {
        GetModuleFileNameEx(hProcess, NULL, szProcessName, MAX_PATH);
        //GetModuleFileName(hProcess, szProcessName, MAX_PATH);
        processPathStr = QString::fromWCharArray(szProcessName);
    }

    // Release the handle to the process.
    CloseHandle( hProcess );
}

void QKeyMapper::getProcessInfoFromHWND(HWND hWnd, QString &processPathStr)
{
    DWORD dwProcessId = 0;
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    GetWindowThreadProcessId(hWnd, &dwProcessId);

    // Get a handle to the process.
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, dwProcessId );

    // Get the process name.
    if (NULL != hProcess )
    {
        GetModuleFileNameEx(hProcess, NULL, szProcessName, MAX_PATH);
        //GetModuleFileName(hProcess, szProcessName, MAX_PATH);
        processPathStr = QString::fromWCharArray(szProcessName);
    }

    // Release the handle to the process.
    CloseHandle( hProcess );
}

BOOL QKeyMapper::EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    Q_UNUSED(lParam);

    if(TRUE == IsWindowVisible(hWnd)){
        DWORD dwProcessId = 0;
        TCHAR titleBuffer[MAX_PATH];
        memset(titleBuffer, 0x00, sizeof(titleBuffer));

        GetWindowThreadProcessId(hWnd, &dwProcessId);
        int resultLength = GetWindowText(hWnd, titleBuffer, MAX_PATH);
        if (resultLength){
            QString WindowText;
            QString ProcessPath;
            QString filename;

            WindowText = QString::fromWCharArray(titleBuffer);
            getProcessInfoFromPID( dwProcessId, ProcessPath);

            if (false == WindowText.isEmpty() && false == ProcessPath.isEmpty()){
                MAP_PROCESSINFO ProcessInfo;
                QFileInfo fileinfo(ProcessPath);
                filename = fileinfo.fileName();

                ProcessInfo.FileName = filename;
                ProcessInfo.PID = QString::number(dwProcessId);
                ProcessInfo.WindowTitle = WindowText;
                ProcessInfo.FilePath = ProcessPath;
                static_ProcessInfoList.append(ProcessInfo);

#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace().noquote() << WindowText <<" [PID:" << dwProcessId <<"]" << "(" << filename << ")";
#endif
            }
        }
    }

    return TRUE;
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

#if 0
    PROCESSENTRY32 pe32;
    QString ProcessName;
    QString WindowText;

    pe32.dwSize = sizeof(pe32);
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(hProcessSnap != INVALID_HANDLE_VALUE)
    {
        qDebug() << "CreateToolhelp32Snapshot Success!!";

        BOOL bMore = Process32First(hProcessSnap,&pe32);
        while(bMore)
        {
            ProcessName = QString::fromWCharArray(pe32.szExeFile);
            WindowText = GetWindowTextByProcessId(pe32.th32ProcessID);

            qDebug().nospace().noquote() << WindowText <<" [PID:" << pe32.th32ProcessID <<"]" << "(" << ProcessName << ")";
            //if (false == WindowText.isEmpty()){

            //}

            bMore = Process32Next(hProcessSnap,&pe32);
        }
    }
    else{
        qDebug() << "CreateToolhelp32Snapshot Failure!!";
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

void QKeyMapper::SystrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (QSystemTrayIcon::DoubleClick == reason){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "QKeyMapper::SystrayIconActivated() -> SystemTray double clicked: showNormal()!!";
#endif

        showNormal();
        activateWindow();
        raise();
    }
}

void QKeyMapper::saveKeyMapSetting(void)
{

}

void QKeyMapper::loadKeyMapSetting(void)
{

}

void QKeyMapper::on_keymapButton_clicked()
{
    if (KEYMAP_IDLE == m_KeyMapStatus){
        if ((false == m_MapProcessInfo.FileName.isEmpty())
                && (false == m_MapProcessInfo.WindowTitle.isEmpty())){
            m_CycleCheckTimer.start(CYCLE_CHECK_TIMEOUT);
            m_SysTrayIcon->setToolTip("QKeyMapper(Mapping : " + m_MapProcessInfo.FileName + ")");
            ui->keymapButton->setText("MapStop");
            m_KeyMapStatus = KEYMAP_CHECKING;
        }
        else{
            QMessageBox::warning(this, tr("QKeyMapper"), tr("Invalid process info for key mapping."));
        }
    }
    else{
        m_CycleCheckTimer.stop();
        m_SysTrayIcon->setToolTip("QKeyMapper(Idle)");
        ui->keymapButton->setText("MapStart");
        setKeyUnHook();
        m_KeyMapStatus = KEYMAP_IDLE;
    }
}

void QKeyMapper::on_savemaplistButton_clicked()
{
    QMessageBox::warning(this, tr("QKeyMapper"), tr("None valid KeyMap Data."));
}

LRESULT QKeyMapper::LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;

    switch (pKeyBoard->vkCode)
    {
    case VK_LSHIFT:
        if (WM_KEYDOWN == wParam){
            keybd_event(VK_OEM_MINUS, 0, 0, 0);
            return 1;
        }
        else if (WM_KEYUP == wParam){
            keybd_event(VK_OEM_MINUS, 0, KEYEVENTF_KEYUP, 0);
            return 1;
        }
        else{
            // do nothing.
        }
        break;
    case VK_LCONTROL:
        if (WM_KEYDOWN == wParam){
            keybd_event(VK_OEM_PLUS, 0, 0, 0);
            return 1;
        }
        else if (WM_KEYUP == wParam){
            keybd_event(VK_OEM_PLUS, 0, KEYEVENTF_KEYUP, 0);
            return 1;
        }
        else{
            // do nothing.
        }
        break;

    case VK_UP:
        if (WM_KEYDOWN == wParam){
            keybd_event(VK_I, 0, 0, 0);
            return 1;
        }
        else if (WM_KEYUP == wParam){
            keybd_event(VK_I, 0, KEYEVENTF_KEYUP, 0);
            return 1;
        }
        else{
            // do nothing.
        }
        break;
    case VK_DOWN:
        if (WM_KEYDOWN == wParam){
            keybd_event(VK_K, 0, 0, 0);
            return 1;
        }
        else if (WM_KEYUP == wParam){
            keybd_event(VK_K, 0, KEYEVENTF_KEYUP, 0);
            return 1;
        }
        else{
            // do nothing.
        }
        break;
    case VK_LEFT:
        if (WM_KEYDOWN == wParam){
            keybd_event(VK_H, 0, 0, 0);
            return 1;
        }
        else if (WM_KEYUP == wParam){
            keybd_event(VK_H, 0, KEYEVENTF_KEYUP, 0);
            return 1;
        }
        else{
            // do nothing.
        }
        break;
    case VK_RIGHT:
        if (WM_KEYDOWN == wParam){
            keybd_event(VK_J, 0, 0, 0);
            return 1;
        }
        else if (WM_KEYUP == wParam){
            keybd_event(VK_J, 0, KEYEVENTF_KEYUP, 0);
            return 1;
        }
        else{
            // do nothing.
        }
        break;
    default:
        break;
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void QKeyMapper::initProcessInfoTable(void)
{
    ui->processinfoTable->setFocusPolicy(Qt::NoFocus);
    ui->processinfoTable->setColumnCount(PROCESSINFO_TABLE_COLUMN);
    ui->processinfoTable->setHorizontalHeaderLabels(QStringList()   << "Name"
                                                                    << "PID"
                                                                    << "Title" );

    ui->processinfoTable->horizontalHeader()->setStretchLastSection(true);
    ui->processinfoTable->verticalHeader()->setVisible(false);
    ui->processinfoTable->verticalHeader()->setDefaultSectionSize(25);
    ui->processinfoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->processinfoTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->processinfoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "verticalHeader->isVisible" << ui->processinfoTable->verticalHeader()->isVisible();
    qDebug() << "selectionBehavior" << ui->processinfoTable->selectionBehavior();
    qDebug() << "selectionMode" << ui->processinfoTable->selectionMode();
    qDebug() << "editTriggers" << ui->processinfoTable->editTriggers();
    qDebug() << "verticalHeader-DefaultSectionSize" << ui->processinfoTable->verticalHeader()->defaultSectionSize();
#endif
}

void QKeyMapper::refreshProcessInfoTable(void)
{
    static_ProcessInfoList.clear();
    EnumWindows((WNDENUMPROC)QKeyMapper::EnumWindowsProc, 0);
    setProcessInfoTable(static_ProcessInfoList);

    ui->processinfoTable->sortItems(PROCESS_NAME_COLUMN);
}

void QKeyMapper::setProcessInfoTable(QList<MAP_PROCESSINFO> &processinfolist)
{
    int rowindex = 0;
    ui->processinfoTable->setRowCount(processinfolist.size());
    for (const MAP_PROCESSINFO &processinfo : processinfolist)
    {
        QFileInfo file_info(processinfo.FilePath);
        QFileIconProvider icon_provider;
        ui->processinfoTable->setItem(rowindex, 0, new QTableWidgetItem(icon_provider.icon(file_info), processinfo.FileName));
        ui->processinfoTable->setItem(rowindex, 1, new QTableWidgetItem(processinfo.PID));
        ui->processinfoTable->setItem(rowindex, 2, new QTableWidgetItem(processinfo.WindowTitle));

        rowindex += 1;
    }
}

void QKeyMapper::on_refreshButton_clicked()
{
    ui->processinfoTable->clearContents();
    refreshProcessInfoTable();
}

void QKeyMapper::on_processinfoTable_doubleClicked(const QModelIndex &index)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "Table DoubleClicked" << index.row() << ui->processinfoTable->item(index.row(), 0)->text() << ui->processinfoTable->item(index.row(), 2)->text();
#endif

    ui->nameLineEdit->setText(ui->processinfoTable->item(index.row(), 0)->text());
    ui->titleLineEdit->setText(ui->processinfoTable->item(index.row(), 2)->text());

    setMapProcessInfo(ui->processinfoTable->item(index.row(), 0)->text(), ui->processinfoTable->item(index.row(), 2)->text());
}
