#include "qkm_ibinput_client.h"
#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QFile>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QTextStream>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>
#include <tlhelp32.h>

namespace {
QString desktopName(HDESK desktop) {
    wchar_t name[128]{}; DWORD bytes=0;
    return desktop && GetUserObjectInformationW(desktop,UOI_NAME,name,sizeof(name),&bytes) ? QString::fromWCharArray(name) : QString();
}
bool otherMapper() {
    const HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (snapshot==INVALID_HANDLE_VALUE) return true;
    PROCESSENTRY32W entry{}; entry.dwSize=sizeof(entry); bool found=false;
    if (Process32FirstW(snapshot,&entry)) do {
        if (_wcsicmp(entry.szExeFile,L"QKeyMapper.exe")==0) found=true;
    } while (Process32NextW(snapshot,&entry));
    CloseHandle(snapshot); return found;
}
class Probe final : public QWidget {
public:
    explicit Probe(const QString &logPath):m_log(logPath) {
        setWindowTitle(QStringLiteral("IbInput G HUB basic probe"));
        setFocusPolicy(Qt::StrongFocus); setMouseTracking(true); resize(460,250);
        const QPoint cursor=QCursor::pos();
        if (QScreen *screen=QGuiApplication::screenAt(cursor)) {
            const QRect area=screen->availableGeometry();
            move(qBound(area.left(),cursor.x()-width()/2,qMax(area.left(),area.right()-width()+1)),
                 qBound(area.top()+30,cursor.y()-height()/2,qMax(area.top()+30,area.bottom()-height())));
        }
        m_log.open(QIODevice::WriteOnly|QIODevice::Text);
        m_timer.setInterval(180);
        connect(&m_timer,&QTimer::timeout,this,[this] { step(); });
        QTimer::singleShot(800,this,[this] { begin(); });
    }
    bool passed() const { return m_done && m_passed; }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this); painter.fillRect(rect(),QColor(245,247,250)); painter.setPen(Qt::black);
        painter.drawText(rect().adjusted(18,18,-18,-18),Qt::AlignLeft|Qt::TextWordWrap,
            QStringLiteral("IbInput G HUB basic probe\nPID %1 | Default desktop required\n%2\n\nMove %3 | F24 %4/%5 | Left %6/%7 | Wheel %8/%9\n\nEsc or focus loss aborts and releases owned input.").arg(GetCurrentProcessId()).arg(m_summary).arg(m_moves).arg(m_keyDown).arg(m_keyUp).arg(m_leftDown).arg(m_leftUp).arg(m_wheelUp).arg(m_wheelDown));
    }
    void mouseMoveEvent(QMouseEvent *) override { ++m_moves; }
    void mousePressEvent(QMouseEvent *event) override { if (event->button()==Qt::LeftButton) ++m_leftDown; }
    void mouseReleaseEvent(QMouseEvent *event) override { if (event->button()==Qt::LeftButton) ++m_leftUp; }
    void keyPressEvent(QKeyEvent *event) override { if (event->key()==Qt::Key_F24) ++m_keyDown; if (event->key()==Qt::Key_Escape) finish(false,QStringLiteral("Escape")); }
    void keyReleaseEvent(QKeyEvent *event) override { if (event->key()==Qt::Key_F24) ++m_keyUp; }
    void wheelEvent(QWheelEvent *event) override { if (event->angleDelta().y()>0) ++m_wheelUp; if (event->angleDelta().y()<0) ++m_wheelDown; }
    void closeEvent(QCloseEvent *event) override {
        m_timer.stop(); m_client.cancelRun();
        if (m_generation && !m_client.closeAfterDrain(m_generation)) m_passed=false;
        write(QStringLiteral("CloseResult=%1").arg(passed() ? QStringLiteral("PASS") : QStringLiteral("FAIL")));
        QWidget::closeEvent(event);
    }
private:
    void write(const QString &line) { if (m_log.isOpen()) { QTextStream stream(&m_log); stream<<QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)<<' '<<line<<'\n'; stream.flush(); m_log.flush(); } }
    bool require(bool value,const QString &detail) { if (!value) finish(false,detail); return value; }
    void begin() {
        const HDESK input=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS);
        const QString inputName=desktopName(input); if (input) CloseDesktop(input);
        const QString actual=desktopName(GetThreadDesktop(GetCurrentThreadId()));
        write(QStringLiteral("PID=%1 EXE=%2 Desktop=%3 InputDesktop=%4").arg(GetCurrentProcessId()).arg(QCoreApplication::applicationFilePath(),actual,inputName));
        if (!require(m_log.isOpen() && actual==QStringLiteral("Default") && inputName==QStringLiteral("Default"),QStringLiteral("Desktop/log precondition failed"))) return;
        if (!require(!otherMapper(),QStringLiteral("Existing QKeyMapper process; no input attempted"))) return;
        for (int key:{VK_SHIFT,VK_CONTROL,VK_MENU,VK_LWIN,VK_RWIN,VK_LBUTTON,VK_F24}) if (!require((GetAsyncKeyState(key)&0x8000)==0,QStringLiteral("Physical key/button already held"))) return;
        QkmIbInputClient::Preferences preferences; preferences.backend=QStringLiteral("logitech_ghub");
        if (!require(m_client.setPreferences(preferences),QStringLiteral("Preference rejected"))) return;
        activateWindow(); raise(); setFocus(); m_summary=QStringLiteral("Running fixed approved actions"); m_timer.start(); update();
    }
    bool prepare() {
        QkmIbInputClient::Requirements requirements;
        requirements.capabilities=QkmIbInputClient::RelativeMove|QkmIbInputClient::Keyboard|QkmIbInputClient::Buttons|QkmIbInputClient::VerticalWheel;
        requirements.keys.insert(VK_F24); requirements.mouseButtons.insert(VK_LBUTTON);
        m_generation=m_client.requestRun();
        const bool result=m_client.prepareRun(m_generation,requirements);
        write(QStringLiteral("Prepare capability=%1 accepted=%2 backend=%3 detail=%4").arg(requirements.capabilities).arg(result).arg(m_client.statusSnapshot().backend,m_client.statusSnapshot().detail));
        return require(result && m_client.statusSnapshot().connected,QStringLiteral("G HUB initialization failed"));
    }
    bool closeRun() {
        m_client.cancelRun(); const bool result=m_client.closeAfterDrain(m_generation);
        write(QStringLiteral("RunClosed generation=%1 released=%2 incomplete=%3").arg(m_generation).arg(result).arg(m_client.statusSnapshot().releaseIncomplete));
        m_generation=0; return require(result && !m_client.statusSnapshot().releaseIncomplete,QStringLiteral("Owned release incomplete"));
    }
    void sendMouse(DWORD flags,LONG x=0,LONG y=0,DWORD data=0) {
        INPUT input{}; input.type=INPUT_MOUSE; input.mi.dwFlags=flags; input.mi.dx=x; input.mi.dy=y; input.mi.mouseData=data;
        write(QStringLiteral("Send mouse flags=%1 dx=%2 dy=%3 data=%4").arg(flags).arg(x).arg(y).arg(data));
        require(m_client.sendEvent(input,m_generation),QStringLiteral("Mouse call failed"));
    }
    void sendKey(bool up) {
        INPUT input{}; input.type=INPUT_KEYBOARD; input.ki.wVk=VK_F24; input.ki.dwFlags=up ? KEYEVENTF_KEYUP : DWORD{0};
        write(QStringLiteral("Send F24 up=%1").arg(up));
        require(m_client.sendEvent(input,m_generation),QStringLiteral("Keyboard call failed"));
    }
    void step() {
        if (m_done) return;
        if (!require(GetForegroundWindow()==reinterpret_cast<HWND>(winId()),QStringLiteral("Foreground changed; aborted"))) return;
        if (!require((GetAsyncKeyState(VK_ESCAPE)&0x8000)==0,QStringLiteral("Escape"))) return;
        if (!rect().contains(mapFromGlobal(QCursor::pos())) && m_step==0 && !m_placementAdjusted) {
            m_placementAdjusted=true;
            move(pos()+QCursor::pos()-mapToGlobal(rect().center()));
            write(QStringLiteral("Receiver repositioned before initialization; cursor was not moved"));
            return;
        }
        if (!require(rect().contains(mapFromGlobal(QCursor::pos())),QStringLiteral("Cursor outside owned receiver"))) return;
        switch (m_step++) {
        case 0: prepare(); break;
        case 1: m_before=QCursor::pos(); sendMouse(MOUSEEVENTF_MOVE,8,0); break;
        case 2: if (!require(QCursor::pos().x()>m_before.x(),QStringLiteral("Right movement not observed"))) return; m_before=QCursor::pos(); sendMouse(MOUSEEVENTF_MOVE,-8,0); break;
        case 3: if (!require(QCursor::pos().x()<m_before.x(),QStringLiteral("Left movement not observed"))) return; m_before=QCursor::pos(); sendMouse(MOUSEEVENTF_MOVE,0,6); break;
        case 4: if (!require(QCursor::pos().y()>m_before.y(),QStringLiteral("Down movement not observed"))) return; m_before=QCursor::pos(); sendMouse(MOUSEEVENTF_MOVE,0,-6); break;
        case 5: if (!require(QCursor::pos().y()<m_before.y() && m_moves>=2,QStringLiteral("Up/receiver movement not observed"))) return; break;
        case 6: sendKey(false); break;
        case 7: if (require(m_keyDown==1,QStringLiteral("F24 DOWN not received"))) sendKey(true); break;
        case 8: if (!require(m_keyUp==1 && !(GetAsyncKeyState(VK_F24)&0x8000),QStringLiteral("F24 UP not received"))) return; break;
        case 9: sendMouse(MOUSEEVENTF_LEFTDOWN); break;
        case 10: if (require(m_leftDown==1,QStringLiteral("Left DOWN not received"))) sendMouse(MOUSEEVENTF_LEFTUP); break;
        case 11: if (!require(m_leftUp==1 && !(GetAsyncKeyState(VK_LBUTTON)&0x8000),QStringLiteral("Left UP not received"))) return; break;
        case 12: sendMouse(MOUSEEVENTF_WHEEL,0,0,WHEEL_DELTA); break;
        case 13: if (require(m_wheelUp==1,QStringLiteral("Wheel UP not received"))) sendMouse(MOUSEEVENTF_WHEEL,0,0,static_cast<DWORD>(-WHEEL_DELTA)); break;
        case 14: if (!require(m_wheelDown==1,QStringLiteral("Wheel DOWN not received"))) return; if (closeRun()) finish(true,QStringLiteral("PASS basic G HUB client reception and normal release")); break;
        default: finish(false,QStringLiteral("Unexpected step")); break;
        }
        update();
    }
    void finish(bool success,const QString &detail) {
        if (m_done) return;
        m_timer.stop(); m_client.cancelRun();
        if (m_generation && !m_client.closeAfterDrain(m_generation)) success=false;
        m_generation=0; m_done=true; m_passed=success; m_summary=detail;
        write(QStringLiteral("Result=%1 Step=%2 Moves=%3 F24=%4/%5 Left=%6/%7 Wheel=%8/%9 Detail=%10").arg(success ? QStringLiteral("PASS") : QStringLiteral("FAIL")).arg(m_step).arg(m_moves).arg(m_keyDown).arg(m_keyUp).arg(m_leftDown).arg(m_leftUp).arg(m_wheelUp).arg(m_wheelDown).arg(detail));
        update();
    }
    QFile m_log;
    QkmIbInputClient m_client;
    QTimer m_timer;
    quint64 m_generation=0;
    int m_step=0,m_moves=0,m_keyDown=0,m_keyUp=0,m_leftDown=0,m_leftUp=0,m_wheelUp=0,m_wheelDown=0;
    QPoint m_before;
    bool m_done=false,m_passed=false,m_placementAdjusted=false;
    QString m_summary=QStringLiteral("Waiting for safe preconditions");
};
}
int main(int argc,char **argv) {
    QApplication application(argc,argv);
    if (!application.arguments().contains(QStringLiteral("--ghub-smoke"))) return 2;
    const QString path=qEnvironmentVariable("QKM_IBINPUT_PROBE_LOG");
    if (path.isEmpty()) return 2;
    Probe receiver(path); receiver.show();
    application.exec();
    return receiver.passed() ? 0 : 2;
}
