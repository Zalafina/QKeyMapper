#include "qkm_ibinput_client.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
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
#include <array>

namespace {
struct MouseButton { int vk; DWORD down,up,data; Qt::MouseButton qt; const char *name; };
const MouseButton buttons[]={
    {VK_LBUTTON,MOUSEEVENTF_LEFTDOWN,MOUSEEVENTF_LEFTUP,0,Qt::LeftButton,"Left"},
    {VK_RBUTTON,MOUSEEVENTF_RIGHTDOWN,MOUSEEVENTF_RIGHTUP,0,Qt::RightButton,"Right"},
    {VK_MBUTTON,MOUSEEVENTF_MIDDLEDOWN,MOUSEEVENTF_MIDDLEUP,0,Qt::MiddleButton,"Middle"},
    {VK_XBUTTON1,MOUSEEVENTF_XDOWN,MOUSEEVENTF_XUP,XBUTTON1,Qt::XButton1,"X1"},
    {VK_XBUTTON2,MOUSEEVENTF_XDOWN,MOUSEEVENTF_XUP,XBUTTON2,Qt::XButton2,"X2"}
};
QString desktopName(HDESK desktop) {
    wchar_t name[128]{}; DWORD bytes=0;
    return desktop && GetUserObjectInformationW(desktop,UOI_NAME,name,sizeof(name),&bytes) ? QString::fromWCharArray(name) : QString();
}
bool ordinaryIntegrity() {
    HANDLE token=nullptr;
    if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)) return false;
    DWORD size=0;
    GetTokenInformation(token,TokenIntegrityLevel,nullptr,0,&size);
    QByteArray bytes(static_cast<int>(size),'\0');
    const bool read=size && GetTokenInformation(token,TokenIntegrityLevel,bytes.data(),size,&size);
    CloseHandle(token);
    if (!read) return false;
    const auto label=reinterpret_cast<TOKEN_MANDATORY_LABEL *>(bytes.data());
    const auto count=*GetSidSubAuthorityCount(label->Label.Sid);
    return count && *GetSidSubAuthority(label->Label.Sid,count-1)==SECURITY_MANDATORY_MEDIUM_RID;
}
bool otherMapper() {
    const HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (snapshot==INVALID_HANDLE_VALUE) return true;
    PROCESSENTRY32W entry{}; entry.dwSize=sizeof(entry); bool found=false;
    if (Process32FirstW(snapshot,&entry)) do {
        if (entry.th32ProcessID!=GetCurrentProcessId() && _wcsnicmp(entry.szExeFile,L"QKeyMapper",10)==0) found=true;
    } while (Process32NextW(snapshot,&entry));
    CloseHandle(snapshot); return found;
}
int razerConnectionOnly(bool cycle) {
    const QString path=qEnvironmentVariable("QKM_IBINPUT_PROBE_LOG");
    QFile log(path);
    if (!QFileInfo(path).isAbsolute() || !log.open(QIODevice::WriteOnly|QIODevice::NewOnly)) return 2;
    const bool medium=ordinaryIntegrity();
    const bool conflict=otherMapper();
    QJsonObject result{{"pid",static_cast<int>(GetCurrentProcessId())},{"exe",QCoreApplication::applicationFilePath()},
        {"desktop",desktopName(GetThreadDesktop(GetCurrentThreadId()))},{"medium",medium},{"other_mapper",conflict},
        {"scope",QStringLiteral("Razer client connection diagnostics; no window, focus, input events or software changes")},
        {"client_send_event_calls",0}};
    bool connected=false,released=false;
    if (medium && !conflict) {
        QkmIbInputClient client;
        QkmIbInputClient::Preferences preferences; preferences.backend=QStringLiteral("razer");
        if (!client.setPreferences(preferences)) return 2;
        const auto generation=client.requestRun();
        const bool prepared=client.prepareRun(generation,{});
        const auto status=client.statusSnapshot();
        connected=prepared && status.connected;
        result.insert(QStringLiteral("prepared"),prepared);
        result.insert(QStringLiteral("connection_passed"),connected);
        result.insert(QStringLiteral("detail"),status.detail);
        result.insert(QStringLiteral("backend"),status.backend);
        released=client.closeAfterDrain(generation);
        if (connected && released && cycle) {
            const auto next=client.requestRun();
            const bool preparedAgain=client.prepareRun(next,{});
            const auto again=client.statusSnapshot();
            connected=preparedAgain && again.connected && again.backend==QStringLiteral("razer") && next>generation;
            result.insert(QStringLiteral("reconnected"),connected);
            result.insert(QStringLiteral("reconnect_detail"),again.detail);
            result.insert(QStringLiteral("reconnect_generation"),static_cast<qint64>(next));
            released=client.closeAfterDrain(next);
        }
        result.insert(QStringLiteral("released"),released);
        result.insert(QStringLiteral("diagnostic_complete"),true);
    }
    const QByteArray bytes=QJsonDocument(result).toJson();
    if (log.write(bytes)!=bytes.size() || !log.flush()) return 2;
    return !medium || conflict || !released ? 2 : connected ? 0 : 3;
}
class Probe final : public QWidget {
public:
    explicit Probe(const QString &logPath,bool connectionCycle,bool mouseSmoke):m_log(logPath),m_connectionCycle(connectionCycle),m_mouseSmoke(mouseSmoke) {
        setWindowTitle(QStringLiteral("IbInput G HUB basic probe"));
        setWindowFlag(Qt::WindowStaysOnTopHint);
        setFocusPolicy(Qt::StrongFocus); setMouseTracking(true); resize(500,320);
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
            QStringLiteral("IbInput G HUB basic probe\nPID %1 | Default desktop required\n%2\n\nMove %3 | F24 %4/%5 | Left %6/%7 | Wheel %8/%9\n\nEsc or focus loss aborts and releases owned input.").arg(GetCurrentProcessId()).arg(m_summary).arg(m_moves).arg(m_keyDown).arg(m_keyUp).arg(m_buttonDown[0]).arg(m_buttonUp[0]).arg(m_wheelUp).arg(m_wheelDown));
    }
    void mouseMoveEvent(QMouseEvent *event) override { ++m_moves; m_moveButtons=event->buttons(); }
    void mousePressEvent(QMouseEvent *event) override {
        for (size_t index=0;index<m_buttonDown.size();++index)
            if (event->button()==buttons[index].qt) ++m_buttonDown[index];
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        for (size_t index=0;index<m_buttonUp.size();++index)
            if (event->button()==buttons[index].qt) ++m_buttonUp[index];
        m_releaseButtons=event->buttons();
    }
    void keyPressEvent(QKeyEvent *event) override { if (event->key()==Qt::Key_F24) ++m_keyDown; if (event->key()==Qt::Key_Escape) finish(false,QStringLiteral("Escape")); }
    void keyReleaseEvent(QKeyEvent *event) override { if (event->key()==Qt::Key_F24) ++m_keyUp; }
    void wheelEvent(QWheelEvent *event) override {
        m_wheelButtons=event->buttons();
        const int delta=event->angleDelta().y();
        if (delta>0) { ++m_wheelUp; m_wheelUpDelta+=delta; }
        if (delta<0) { ++m_wheelDown; m_wheelDownDelta+=delta; }
    }
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
        if (m_done) return;
        if (GetForegroundWindow()!=reinterpret_cast<HWND>(winId())) {
            if (++m_activationWaits>180) { finish(false,QStringLiteral("Owned receiver was not activated within 90 seconds")); return; }
            if (m_activationWaits==1) write(QStringLiteral("WaitingForOwnedForeground; no backend initialization"));
            m_summary=QStringLiteral("Click this receiver's title bar once to start; no backend is initialized while waiting.");
            update();
            QTimer::singleShot(500,this,[this] { begin(); });
            return;
        }
        const HDESK input=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS);
        const QString inputName=desktopName(input); if (input) CloseDesktop(input);
        const QString actual=desktopName(GetThreadDesktop(GetCurrentThreadId()));
        write(QStringLiteral("PID=%1 EXE=%2 Desktop=%3 InputDesktop=%4").arg(GetCurrentProcessId()).arg(QCoreApplication::applicationFilePath(),actual,inputName));
        if (!require(m_log.isOpen() && actual==QStringLiteral("Default") && inputName==QStringLiteral("Default"),QStringLiteral("Desktop/log precondition failed"))) return;
        write(QStringLiteral("Mode=%1 Medium=%2").arg(m_connectionCycle ? QStringLiteral("connect-cycle") : m_mouseSmoke ? QStringLiteral("mouse-smoke") : QStringLiteral("smoke")).arg(ordinaryIntegrity()));
        if (!require(ordinaryIntegrity(),QStringLiteral("Ordinary Medium integrity required"))) return;
        if (!require(!otherMapper(),QStringLiteral("Existing QKeyMapper process; no input attempted"))) return;
        for (int key:{VK_SHIFT,VK_CONTROL,VK_MENU,VK_LWIN,VK_RWIN,VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2,VK_F24}) if (!require((GetAsyncKeyState(key)&0x8000)==0,QStringLiteral("Physical key/button already held"))) return;
        QkmIbInputClient::Preferences preferences; preferences.backend=QStringLiteral("logitech_ghub");
        if (!require(m_client.setPreferences(preferences),QStringLiteral("Preference rejected"))) return;
        setFocus(); m_summary=QStringLiteral("Running fixed approved actions"); m_timer.start(); update();
    }
    bool prepare() {
        QkmIbInputClient::Requirements requirements;
        if (!m_connectionCycle) {
            requirements.capabilities=QkmIbInputClient::RelativeMove|QkmIbInputClient::Buttons|QkmIbInputClient::VerticalWheel;
            if (m_mouseSmoke) {
                for (const auto &button:buttons) requirements.mouseButtons.insert(button.vk);
            } else {
                requirements.capabilities|=QkmIbInputClient::Keyboard;
                requirements.keys.insert(VK_F24); requirements.mouseButtons.insert(VK_LBUTTON);
            }
        }
        m_generation=m_client.requestRun();
        ++m_prepareRuns;
        const bool result=m_client.prepareRun(m_generation,requirements);
        write(QStringLiteral("Prepare capability=%1 accepted=%2 backend=%3 detail=%4").arg(requirements.capabilities).arg(result).arg(m_client.statusSnapshot().backend,m_client.statusSnapshot().detail));
        return require(result && m_client.statusSnapshot().connected,QStringLiteral("G HUB initialization failed"));
    }
    bool closeRun() {
        m_client.cancelRun(); const bool result=m_client.closeAfterDrain(m_generation);
        write(QStringLiteral("RunClosed generation=%1 released=%2 incomplete=%3").arg(m_generation).arg(result).arg(m_client.statusSnapshot().releaseIncomplete));
        m_generation=0; if (result) m_ownedButtons.clear();
        return require(result && !m_client.statusSnapshot().releaseIncomplete,QStringLiteral("Owned release incomplete"));
    }
    void sendMouse(DWORD flags,LONG x=0,LONG y=0,DWORD data=0) {
        INPUT input{}; input.type=INPUT_MOUSE; input.mi.dwFlags=flags; input.mi.dx=x; input.mi.dy=y; input.mi.mouseData=data;
        write(QStringLiteral("Send mouse flags=%1 dx=%2 dy=%3 data=%4").arg(flags).arg(x).arg(y).arg(data));
        for (const auto &button:buttons)
            if ((flags & button.down) && (!button.data || button.data==data)) m_ownedButtons.insert(button.vk);
        if (!require(m_client.sendEvent(input,m_generation),QStringLiteral("Mouse call failed"))) return;
        for (const auto &button:buttons)
            if ((flags & button.up) && (!button.data || button.data==data)) m_ownedButtons.remove(button.vk);
    }
    void sendKey(bool up) {
        INPUT input{}; input.type=INPUT_KEYBOARD; input.ki.wVk=VK_F24; input.ki.dwFlags=up ? KEYEVENTF_KEYUP : DWORD{0};
        write(QStringLiteral("Send F24 up=%1").arg(up));
        require(m_client.sendEvent(input,m_generation),QStringLiteral("Keyboard call failed"));
    }
    void step() {
        if (m_done) return;
        const HWND foreground=GetForegroundWindow();
        if (foreground!=reinterpret_cast<HWND>(winId())) {
            DWORD foregroundPid=0; GetWindowThreadProcessId(foreground,&foregroundPid);
            write(QStringLiteral("Foreground expected=%1 actual=%2 pid=%3 PrepareRuns=%4")
                .arg(static_cast<qulonglong>(winId())).arg(reinterpret_cast<quintptr>(foreground)).arg(foregroundPid).arg(m_prepareRuns));
        }
        if (!require(foreground==reinterpret_cast<HWND>(winId()),QStringLiteral("Foreground changed; aborted"))) return;
        const HDESK input=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS);
        const QString inputName=desktopName(input); if (input) CloseDesktop(input);
        if (!require(inputName==QStringLiteral("Default"),QStringLiteral("Input desktop changed; aborted"))) return;
        if (!require((GetAsyncKeyState(VK_ESCAPE)&0x8000)==0,QStringLiteral("Escape"))) return;
        if (!require(!otherMapper(),QStringLiteral("Conflicting QKeyMapper process appeared"))) return;
        for (int key:{VK_SHIFT,VK_CONTROL,VK_MENU,VK_LWIN,VK_RWIN,VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2})
            if (!require(m_ownedButtons.contains(key) || (GetAsyncKeyState(key)&0x8000)==0,QStringLiteral("Unowned key/button held"))) return;
        if (m_connectionCycle) {
            switch (m_step++) {
            case 0: prepare(); break;
            case 1: closeRun(); break;
            case 2: prepare(); break;
            case 3: if (closeRun()) finish(true,QStringLiteral("PASS initialization, normal close and one reconnect; no input sent")); break;
            default: finish(false,QStringLiteral("Unexpected connection step")); break;
            }
            update(); return;
        }
        if (!rect().contains(mapFromGlobal(QCursor::pos())) && m_step==0 && !m_placementAdjusted) {
            m_placementAdjusted=true;
            move(pos()+QCursor::pos()-mapToGlobal(rect().center()));
            write(QStringLiteral("Receiver repositioned before initialization; cursor was not moved"));
            return;
        }
        if (!require(rect().contains(mapFromGlobal(QCursor::pos())),QStringLiteral("Cursor outside owned receiver"))) return;
        if (m_mouseSmoke) { mouseStep(); update(); return; }
        switch (m_step++) {
        case 0: prepare(); break;
        case 1: m_before=QCursor::pos(); m_beforeMoves=m_moves; sendMouse(MOUSEEVENTF_MOVE,8,0); break;
        case 2: if (!require(QCursor::pos().x()>m_before.x() && m_moves>m_beforeMoves,QStringLiteral("Right movement not received"))) return; ++m_verifiedMoves; m_before=QCursor::pos(); m_beforeMoves=m_moves; sendMouse(MOUSEEVENTF_MOVE,-8,0); break;
        case 3: if (!require(QCursor::pos().x()<m_before.x() && m_moves>m_beforeMoves,QStringLiteral("Left movement not received"))) return; ++m_verifiedMoves; m_before=QCursor::pos(); m_beforeMoves=m_moves; sendMouse(MOUSEEVENTF_MOVE,0,6); break;
        case 4: if (!require(QCursor::pos().y()>m_before.y() && m_moves>m_beforeMoves,QStringLiteral("Down movement not received"))) return; ++m_verifiedMoves; m_before=QCursor::pos(); m_beforeMoves=m_moves; sendMouse(MOUSEEVENTF_MOVE,0,-6); break;
        case 5: if (!require(QCursor::pos().y()<m_before.y() && m_moves>m_beforeMoves,QStringLiteral("Up movement not received"))) return; ++m_verifiedMoves; break;
        case 6: sendKey(false); break;
        case 7: if (require(m_keyDown==1,QStringLiteral("F24 DOWN not received"))) sendKey(true); break;
        case 8: if (!require(m_keyUp==1 && !(GetAsyncKeyState(VK_F24)&0x8000),QStringLiteral("F24 UP not received"))) return; break;
        case 9: sendMouse(MOUSEEVENTF_LEFTDOWN); break;
        case 10: if (require(m_buttonDown[0]==1,QStringLiteral("Left DOWN not received"))) sendMouse(MOUSEEVENTF_LEFTUP); break;
        case 11: if (!require(m_buttonUp[0]==1 && !(GetAsyncKeyState(VK_LBUTTON)&0x8000),QStringLiteral("Left UP not received"))) return; break;
        case 12: sendMouse(MOUSEEVENTF_WHEEL,0,0,WHEEL_DELTA); break;
        case 13: if (require(m_wheelUp==1,QStringLiteral("Wheel UP not received"))) sendMouse(MOUSEEVENTF_WHEEL,0,0,static_cast<DWORD>(-WHEEL_DELTA)); break;
        case 14: if (!require(m_wheelDown==1 && m_wheelUpDelta==WHEEL_DELTA && m_wheelDownDelta==-WHEEL_DELTA && m_verifiedMoves==4,QStringLiteral("Wheel units or verified movements differ"))) return; if (closeRun()) finish(true,QStringLiteral("PASS basic G HUB client reception and normal release")); break;
        default: finish(false,QStringLiteral("Unexpected step")); break;
        }
        update();
    }
    void mouseStep() {
        const int step=m_step++;
        if (step==0) { prepare(); return; }
        if (step<=15) {
            const size_t index=static_cast<size_t>((step-1)/3);
            const auto &button=buttons[index];
            switch ((step-1)%3) {
            case 0: sendMouse(button.down,0,0,button.data); break;
            case 1:
                if (require(m_buttonDown[index]==1,QStringLiteral("%1 DOWN not received").arg(QLatin1String(button.name))))
                    sendMouse(button.up,0,0,button.data);
                break;
            case 2:
                require(m_buttonUp[index]==1 && !(GetAsyncKeyState(button.vk)&0x8000),
                    QStringLiteral("%1 UP not received").arg(QLatin1String(button.name)));
                break;
            }
            return;
        }
        switch (step) {
        case 16: sendMouse(MOUSEEVENTF_LEFTDOWN); break;
        case 17:
            if (!require(m_buttonDown[0]==2,QStringLiteral("Held Left DOWN not received"))) return;
            m_before=QCursor::pos(); m_beforeMoves=m_moves; sendMouse(MOUSEEVENTF_MOVE,8); break;
        case 18:
            if (!require(QCursor::pos().x()>m_before.x() && m_moves>m_beforeMoves && m_moveButtons==Qt::LeftButton,
                QStringLiteral("Held right movement not received"))) return;
            ++m_verifiedMoves; m_before=QCursor::pos(); m_beforeMoves=m_moves; sendMouse(MOUSEEVENTF_MOVE,-8); break;
        case 19:
            if (!require(QCursor::pos().x()<m_before.x() && m_moves>m_beforeMoves && m_moveButtons==Qt::LeftButton,
                QStringLiteral("Held left movement not received"))) return;
            ++m_verifiedMoves; sendMouse(MOUSEEVENTF_WHEEL,0,0,WHEEL_DELTA); break;
        case 20:
            if (!require(m_wheelUp==1 && m_wheelUpDelta==WHEEL_DELTA && m_wheelButtons==Qt::LeftButton,
                QStringLiteral("Held wheel UP not received"))) return;
            sendMouse(MOUSEEVENTF_WHEEL,0,0,static_cast<DWORD>(-WHEEL_DELTA)); break;
        case 21:
            if (!require(m_wheelDown==1 && m_wheelDownDelta==-WHEEL_DELTA && m_wheelButtons==Qt::LeftButton,
                QStringLiteral("Held wheel DOWN not received"))) return;
            closeRun(); break;
        case 22:
            for (size_t index=0;index<m_buttonUp.size();++index)
                if (!require(m_buttonDown[index]==(index==0 ? 2 : 1) && m_buttonUp[index]==m_buttonDown[index]
                    && !(GetAsyncKeyState(buttons[index].vk)&0x8000),QStringLiteral("Button counts or stop release differ"))) return;
            if (!require(m_releaseButtons==Qt::NoButton && m_verifiedMoves==2,
                QStringLiteral("Held state remains after normal stop"))) return;
            finish(true,QStringLiteral("PASS five buttons, held relative movement/wheel and stop release; client path only")); break;
        default: finish(false,QStringLiteral("Unexpected mouse step")); break;
        }
    }
    void finish(bool success,const QString &detail) {
        if (m_done) return;
        m_timer.stop(); m_client.cancelRun();
        if (m_generation && !m_client.closeAfterDrain(m_generation)) success=false;
        m_generation=0; m_done=true; m_passed=success; m_summary=detail;
        write(QStringLiteral("Result=%1 Step=%2 Moves=%3 F24=%4/%5 Left=%6/%7 Wheel=%8/%9 Detail=%10").arg(success ? QStringLiteral("PASS") : QStringLiteral("FAIL")).arg(m_step).arg(m_moves).arg(m_keyDown).arg(m_keyUp).arg(m_buttonDown[0]).arg(m_buttonUp[0]).arg(m_wheelUp).arg(m_wheelDown).arg(detail));
        write(QStringLiteral("VerifiedMoves=%1 WheelDelta=%2/%3").arg(m_verifiedMoves).arg(m_wheelUpDelta).arg(m_wheelDownDelta));
        for (size_t index=0;index<m_buttonDown.size();++index)
            write(QStringLiteral("Button=%1 Received=%2/%3").arg(QLatin1String(buttons[index].name)).arg(m_buttonDown[index]).arg(m_buttonUp[index]));
        update();
    }
    QFile m_log;
    QkmIbInputClient m_client;
    QTimer m_timer;
    quint64 m_generation=0;
    std::array<int,5> m_buttonDown{},m_buttonUp{};
    QSet<int> m_ownedButtons;
    Qt::MouseButtons m_moveButtons=Qt::NoButton,m_wheelButtons=Qt::NoButton,m_releaseButtons=Qt::NoButton;
    int m_step=0,m_moves=0,m_keyDown=0,m_keyUp=0,m_wheelUp=0,m_wheelDown=0;
    int m_beforeMoves=0,m_verifiedMoves=0,m_wheelUpDelta=0,m_wheelDownDelta=0,m_prepareRuns=0,m_activationWaits=0;
    bool m_connectionCycle=false,m_mouseSmoke=false;
    QPoint m_before;
    bool m_done=false,m_passed=false,m_placementAdjusted=false;
    QString m_summary=QStringLiteral("Waiting for safe preconditions");
};
}
int main(int argc,char **argv) {
    if (argc==2 && (QByteArray(argv[1])=="--razer-connect-only" || QByteArray(argv[1])=="--razer-connect-cycle-only")) {
        const bool cycle=QByteArray(argv[1])=="--razer-connect-cycle-only";
        QCoreApplication application(argc,argv);
        return razerConnectionOnly(cycle);
    }
    QApplication application(argc,argv);
    const bool smoke=application.arguments().contains(QStringLiteral("--ghub-smoke"));
    const bool connectionCycle=application.arguments().contains(QStringLiteral("--ghub-connect-cycle"));
    const bool mouseSmoke=application.arguments().contains(QStringLiteral("--mouse-smoke"));
    if (static_cast<int>(smoke)+static_cast<int>(connectionCycle)+static_cast<int>(mouseSmoke)!=1) return 2;
    const QString path=qEnvironmentVariable("QKM_IBINPUT_PROBE_LOG");
    if (path.isEmpty()) return 2;
    Probe receiver(path,connectionCycle,mouseSmoke); receiver.show();
    application.exec();
    return receiver.passed() ? 0 : 2;
}
