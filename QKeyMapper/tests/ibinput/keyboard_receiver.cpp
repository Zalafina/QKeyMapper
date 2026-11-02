#include <QApplication>
#include <QDir>
#include <QCursor>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScreen>
#include <QPainter>
#include <QSaveFile>
#include <QSet>
#include <QTimer>
#include <QWidget>
#include <Windows.h>

namespace {
QString output;
void writeJson(const QString &name,const QJsonObject &value) {
    QSaveFile file(output+QLatin1Char('/')+name);
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(value).toJson())<0 || !file.commit()) qFatal("Receiver evidence write failed");
}
QString desktopName() {
    wchar_t name[128]{}; DWORD bytes=0;
    return GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()),UOI_NAME,name,sizeof(name),&bytes) ? QString::fromWCharArray(name) : QString();
}
class Receiver final : public QWidget {
public:
    explicit Receiver(bool live,bool mouse=false):m_live(live),m_mouse(mouse) {
        setWindowTitle(m_mouse ? QStringLiteral("IbInput product mouse receiver") : QStringLiteral("IbInput basic keyboard receiver"));
        setWindowFlag(Qt::WindowStaysOnTopHint); setFocusPolicy(Qt::StrongFocus); resize(540,m_mouse ? 340 : 240);
        setMouseTracking(m_mouse);
        if (live && m_mouse) {
            const QPoint cursor=QCursor::pos();
            if (QScreen *screen=QGuiApplication::screenAt(cursor)) {
                const QRect area=screen->availableGeometry();
                move(qBound(area.left(),cursor.x()-width()/2,qMax(area.left(),area.right()-width()+1)),
                    qBound(area.top()+30,cursor.y()-height()/2,qMax(area.top()+30,area.bottom()-height())));
            }
        }
        if (live) {
            m_timer.setInterval(50);
            connect(&m_timer,&QTimer::timeout,this,[this] {
                if (++m_ticks>3600) { m_aborted=true; publish(); qApp->exit(2); }
                if (QFile::exists(output+QStringLiteral("/receiver-close"))) qApp->quit();
            });
            m_timer.start();
        }
    }
    void observe(quint32 key,bool up) {
        const QSet<quint32> accepted=m_mouse ? QSet<quint32>{'Q','W','E','R','T','A','S','D','F'}
            : QSet<quint32>{'W','F','1','2',VK_F13,VK_F24};
        if (!accepted.contains(key)) return;
        const QString id=QString::number(key);
        if (up) { if (m_pressed.remove(key)) m_up[id]=m_up.value(id).toInt()+1; }
        else if (!m_pressed.contains(key)) { m_pressed.insert(key); m_down[id]=m_down.value(id).toInt()+1; }
        if (m_live) publish();
        update();
    }
    QJsonObject state() const {
        QJsonObject value{{"down",m_down},{"up",m_up},{"pressed_count",m_pressed.size()},{"aborted",m_aborted}};
        if (m_mouse) {
            value.insert(QStringLiteral("mouse_down"),m_mouseDown); value.insert(QStringLiteral("mouse_up"),m_mouseUp);
            value.insert(QStringLiteral("mouse_buttons"),static_cast<int>(m_mouseButtons));
            value.insert(QStringLiteral("moves"),m_moves); value.insert(QStringLiteral("move_buttons"),static_cast<int>(m_moveButtons));
            value.insert(QStringLiteral("wheel_up"),m_wheelUp); value.insert(QStringLiteral("wheel_down"),m_wheelDown);
            value.insert(QStringLiteral("wheel_delta"),m_wheelDelta); value.insert(QStringLiteral("wheel_buttons"),static_cast<int>(m_wheelButtons));
        }
        return value;
    }
    void publish() const { writeJson(QStringLiteral("receiver-state.json"),state()); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this); painter.fillRect(rect(),QColor(245,247,250)); painter.setPen(Qt::black);
        const QString instructions=m_mouse
            ? QStringLiteral("Owned product mouse receiver\nQ/W/E/R/T -> Left/Right/Middle/X1/X2\nA/S -> Wheel Up/Down | D/F -> Move Right/Left")
            : QStringLiteral("Owned keyboard receiver\nW -> F | 1 -> 2 | F13 -> F24");
        const auto compact=[](const QJsonObject &value) { return QString::fromUtf8(QJsonDocument(value).toJson(QJsonDocument::Compact)); };
        const QString status=m_mouse
            ? QStringLiteral("Mouse DOWN %1\nMouse UP %2\nHeld buttons %3 | moves %4 | move buttons %5\nWheel up/down %6/%7 | delta %8 | wheel buttons %9\n")
                .arg(compact(m_mouseDown)).arg(compact(m_mouseUp)).arg(static_cast<int>(m_mouseButtons)).arg(m_moves)
                .arg(static_cast<int>(m_moveButtons)).arg(m_wheelUp).arg(m_wheelDown).arg(m_wheelDelta).arg(static_cast<int>(m_wheelButtons))
                +QStringLiteral("Original DOWN %1\nOriginal UP %2\nHeld keys %3 | aborted %4")
                    .arg(compact(m_down)).arg(compact(m_up)).arg(m_pressed.size()).arg(m_aborted)
            : compact(state());
        painter.drawText(rect().adjusted(18,18,-18,-18),Qt::AlignLeft|Qt::TextWordWrap,
            instructions+QStringLiteral("\nEscape aborts; keep this window foreground.\n\n")+status);
    }
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key()==Qt::Key_Escape) { m_aborted=true; if (m_live) publish(); qApp->exit(2); return; }
        if (!event->isAutoRepeat()) observe(event->nativeVirtualKey(),false);
        event->accept();
    }
    void keyReleaseEvent(QKeyEvent *event) override {
        if (!event->isAutoRepeat()) observe(event->nativeVirtualKey(),true);
        event->accept();
    }
    void mousePressEvent(QMouseEvent *event) override {
        if (!m_mouse) { QWidget::mousePressEvent(event); return; }
        const QString key=QString::number(static_cast<int>(event->button()));
        m_mouseDown[key]=m_mouseDown.value(key).toInt()+1; m_mouseButtons=event->buttons();
        if (m_live) publish();
        update(); event->accept();
    }
    void mouseDoubleClickEvent(QMouseEvent *event) override { mousePressEvent(event); }
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (!m_mouse) { QWidget::mouseReleaseEvent(event); return; }
        const QString key=QString::number(static_cast<int>(event->button()));
        m_mouseUp[key]=m_mouseUp.value(key).toInt()+1; m_mouseButtons=event->buttons();
        if (m_live) publish();
        update(); event->accept();
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if (!m_mouse) { QWidget::mouseMoveEvent(event); return; }
        ++m_moves; m_moveButtons=event->buttons();
        if (m_live) publish();
        update(); event->accept();
    }
    void wheelEvent(QWheelEvent *event) override {
        if (!m_mouse) { QWidget::wheelEvent(event); return; }
        m_wheelDelta=event->angleDelta().y(); m_wheelButtons=event->buttons();
        if (m_wheelDelta>0) m_wheelUp+=m_wheelDelta/120;
        if (m_wheelDelta<0) m_wheelDown-=m_wheelDelta/120;
        if (m_live) publish();
        update(); event->accept();
    }
private:
    bool m_live=false,m_mouse=false,m_aborted=false;
    QJsonObject m_mouseDown,m_mouseUp;
    Qt::MouseButtons m_mouseButtons=Qt::NoButton,m_moveButtons=Qt::NoButton,m_wheelButtons=Qt::NoButton;
    int m_moves=0,m_wheelUp=0,m_wheelDown=0,m_wheelDelta=0;
    int m_ticks=0;
    QTimer m_timer;
    QSet<quint32> m_pressed;
    QJsonObject m_down,m_up;
};
}
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    if (app.arguments()==QStringList{app.arguments().front(),QStringLiteral("--self-check")}) {
        Receiver receiver(false);
        QKeyEvent down(QEvent::KeyPress,Qt::Key_F,Qt::NoModifier,0,'F',0);
        QKeyEvent up(QEvent::KeyRelease,Qt::Key_F,Qt::NoModifier,0,'F',0);
        QApplication::sendEvent(&receiver,&down); QApplication::sendEvent(&receiver,&down); QApplication::sendEvent(&receiver,&up);
        receiver.observe('X',false);
        const auto state=receiver.state();
        if (state.value(QStringLiteral("down")).toObject().value(QStringLiteral("70")).toInt()!=1
            || state.value(QStringLiteral("up")).toObject().value(QStringLiteral("70")).toInt()!=1
            || state.value(QStringLiteral("pressed_count")).toInt()!=0) return 2;
        Receiver mouse(false,true);
        const QPointF point(40,40);
        for (Qt::MouseButton button:{Qt::LeftButton,Qt::RightButton,Qt::MiddleButton,Qt::XButton1,Qt::XButton2}) {
            QMouseEvent press(QEvent::MouseButtonPress,point,point,point,button,button,Qt::NoModifier);
            QMouseEvent release(QEvent::MouseButtonRelease,point,point,point,button,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(&mouse,&press); QApplication::sendEvent(&mouse,&release);
            const QString id=QString::number(static_cast<int>(button));
            const auto observed=mouse.state();
            if (observed.value(QStringLiteral("mouse_down")).toObject().value(id).toInt()!=1
                || observed.value(QStringLiteral("mouse_up")).toObject().value(id).toInt()!=1) return 2;
        }
        QMouseEvent move(QEvent::MouseMove,point,point,point,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&mouse,&move);
        for (int delta:{120,-120}) {
            QWheelEvent wheel(point,point,QPoint(),QPoint(0,delta),Qt::LeftButton,Qt::NoModifier,Qt::NoScrollPhase,false);
            QApplication::sendEvent(&mouse,&wheel);
        }
        const auto observed=mouse.state();
        const QString snapshot=qEnvironmentVariable("QKM_IBINPUT_RECEIVER_SNAPSHOT");
        if (!snapshot.isEmpty()) {
            QFile desktop(snapshot+QStringLiteral(".desktop"));
            if (!desktop.open(QIODevice::WriteOnly) || desktop.write(desktopName().toUtf8())<0 || !mouse.grab().save(snapshot)) return 2;
        }
        return observed.value(QStringLiteral("mouse_buttons")).toInt()==0
            && observed.value(QStringLiteral("moves")).toInt()==1 && observed.value(QStringLiteral("move_buttons")).toInt()==1
            && observed.value(QStringLiteral("wheel_up")).toInt()==1 && observed.value(QStringLiteral("wheel_down")).toInt()==1
            && observed.value(QStringLiteral("wheel_delta")).toInt()==-120 && observed.value(QStringLiteral("wheel_buttons")).toInt()==1 ? 0 : 2;
    }
    output=qEnvironmentVariable("QKM_IBINPUT_KEYBOARD_OUTPUT");
    const bool mouse=app.arguments()==QStringList{app.arguments().front(),QStringLiteral("--mouse")};
    if ((!mouse && argc!=1) || !QDir(output).exists() || desktopName()!=QStringLiteral("Default")) return 2;
    Receiver receiver(true,mouse); receiver.show(); receiver.publish();
    writeJson(QStringLiteral("receiver-ready.json"),{{"pid",static_cast<int>(GetCurrentProcessId())},
        {"hwnd",QString::number(static_cast<qulonglong>(receiver.winId()))},{"desktop",desktopName()}});
    return app.exec();
}
