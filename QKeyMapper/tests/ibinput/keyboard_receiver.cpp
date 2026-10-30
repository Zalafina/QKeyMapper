#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
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
    explicit Receiver(bool live):m_live(live) {
        setWindowTitle(QStringLiteral("IbInput basic keyboard receiver"));
        setWindowFlag(Qt::WindowStaysOnTopHint); setFocusPolicy(Qt::StrongFocus); resize(540,240);
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
        if (!QSet<quint32>{'W','F','1','2',VK_F13,VK_F24}.contains(key)) return;
        const QString id=QString::number(key);
        if (up) { if (m_pressed.remove(key)) m_up[id]=m_up.value(id).toInt()+1; }
        else if (!m_pressed.contains(key)) { m_pressed.insert(key); m_down[id]=m_down.value(id).toInt()+1; }
        if (m_live) publish();
        update();
    }
    QJsonObject state() const {
        return {{"down",m_down},{"up",m_up},{"pressed_count",m_pressed.size()},{"aborted",m_aborted}};
    }
    void publish() const { writeJson(QStringLiteral("receiver-state.json"),state()); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this); painter.fillRect(rect(),QColor(245,247,250)); painter.setPen(Qt::black);
        painter.drawText(rect().adjusted(18,18,-18,-18),Qt::AlignLeft|Qt::TextWordWrap,
            QStringLiteral("Owned keyboard receiver\nW -> F | 1 -> 2 | F13 -> F24\nEscape aborts; keep this window foreground.\n\n%1")
                .arg(QString::fromUtf8(QJsonDocument(state()).toJson(QJsonDocument::Compact))));
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
private:
    bool m_live=false,m_aborted=false;
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
        return state.value(QStringLiteral("down")).toObject().value(QStringLiteral("70")).toInt()==1
            && state.value(QStringLiteral("up")).toObject().value(QStringLiteral("70")).toInt()==1
            && state.value(QStringLiteral("pressed_count")).toInt()==0 ? 0 : 2;
    }
    output=qEnvironmentVariable("QKM_IBINPUT_KEYBOARD_OUTPUT");
    if (argc!=1 || !QDir(output).exists() || desktopName()!=QStringLiteral("Default")) return 2;
    Receiver receiver(true); receiver.show(); receiver.publish();
    writeJson(QStringLiteral("receiver-ready.json"),{{"pid",static_cast<int>(GetCurrentProcessId())},
        {"hwnd",QString::number(static_cast<qulonglong>(receiver.winId()))},{"desktop",desktopName()}});
    return app.exec();
}
