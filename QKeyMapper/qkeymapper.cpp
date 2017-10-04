#include "qkeymapper.h"
#include "ui_qkeymapper.h"

QKeyMapper::QKeyMapper(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QKeyMapper),
    m_KeyMapStatus(KEYMAP_IDLE),
    m_SysTrayIcon(NULL),
    m_KeyHook(NULL)
{
    ui->setupUi(this);
    //setFocusPolicy(Qt::StrongFocus);

    setKeyHook();

    loadKeyMapList();

    m_SysTrayIcon = new QSystemTrayIcon(this);
    m_SysTrayIcon->setIcon(QIcon(":/AppIcon.ico"));
    m_SysTrayIcon->setToolTip("QKeyMapper(Idle)");
    m_SysTrayIcon->show();

    QObject::connect(m_SysTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(SystrayIconActivated(QSystemTrayIcon::ActivationReason)));
}

QKeyMapper::~QKeyMapper()
{
    delete ui;

    delete m_SysTrayIcon;
    m_SysTrayIcon = NULL;

    UnhookWindowsHookEx(m_KeyHook);
    m_KeyHook = NULL;
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

void QKeyMapper::setKeyHook(void)
{
    m_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper::LowLevelKeyboardHookProc, GetModuleHandle(NULL),0);
}

void QKeyMapper::changeEvent(QEvent *event)
{
    if(event->type()==QEvent::WindowStateChange)
    {
        QTimer::singleShot(0, this, SLOT(WindowStateChangedProc()));
    }
    QDialog::changeEvent(event);
}

//void QKeyMapper::keyPressEvent(QKeyEvent *event)
//{
//    switch (event->key()) {
//    case Qt::Key_Space:
//        focusNextChild();
//        break;
//    default:
//        break;
//    }

//    qDebug() << "Some key pressed:" << Qt::Key(event->key());

//    QDialog::keyPressEvent(event);
//}

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

void QKeyMapper::saveKeyMapList(void)
{

}

void QKeyMapper::loadKeyMapList(void)
{

}


void QKeyMapper::on_savemaplistButton_clicked()
{
    QMessageBox::warning(this, tr("QKeyMapper"), tr("None valid KeyMap Data."));
}

void QKeyMapper::on_keymapButton_clicked()
{
    QMessageBox::warning(this, tr("QKeyMapper"), tr("Test"));
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
    case VK_RIGHT:
        if (WM_KEYDOWN == wParam){
            keybd_event(VK_L, 0, 0, 0);
            return 1;
        }
        else if (WM_KEYUP == wParam){
            keybd_event(VK_L, 0, KEYEVENTF_KEYUP, 0);
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
