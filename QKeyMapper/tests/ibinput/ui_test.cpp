#include "qkm_ibinput_client.h"
#include "qkm_ibinput_popup.h"
#include <IbInputSimulator/InputSimulator.hpp>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPalette>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTranslator>
#include <QVBoxLayout>
#include <iostream>
#include <stdexcept>

namespace {
int initCalls=0,sendCalls=0;
void check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
QString desktopName(HDESK desktop) {
    wchar_t name[128]{}; DWORD size=0;
    return GetUserObjectInformationW(desktop,UOI_NAME,name,sizeof(name),&size) ? QString::fromWCharArray(name) : QString();
}
void writeJson(const QString &path,const QJsonObject &value) {
    QFile output(path);
    check(output.open(QIODevice::WriteOnly|QIODevice::Truncate),"Cannot write UI evidence");
    check(output.write(QJsonDocument(value).toJson())>0,"UI evidence write failed");
}
}
DLLAPI Send::Error __stdcall IbSendInit(Send::SendType,Send::InitFlags,void *) { ++initCalls; return Send::Error::DeviceNotFound; }
DLLAPI void __stdcall IbSendDestroy() {}
DLLAPI UINT WINAPI IbSendInput(UINT,LPINPUT,int) { ++sendCalls; return 0; }

int main(int argc,char **argv) {
    if (argc!=2 || QString::fromLocal8Bit(argv[1])!=QStringLiteral("--ibinput-ui-check")) return 2;
    const QString root=qEnvironmentVariable("QKM_IBINPUT_UI_ROOT");
    const QString translations=qEnvironmentVariable("QKM_IBINPUT_QM_ROOT");
    if (!QDir(root).exists() || !QDir(translations).exists()) return 2;
    const QString name=QStringLiteral("QkmIbInputUi_%1").arg(GetCurrentProcessId());
    const HDESK original=GetThreadDesktop(GetCurrentThreadId());
    const HDESK owned=CreateDesktopW(reinterpret_cast<LPCWSTR>(name.utf16()),nullptr,nullptr,0,
        DESKTOP_CREATEWINDOW|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS|DESKTOP_ENUMERATE,nullptr);
    if (!owned || !SetThreadDesktop(owned)) { if (owned) CloseDesktop(owned); return 2; }
    int nativeExit=2;
    {
        QApplication app(argc,argv);
        QApplication::setStyle(QStringLiteral("Fusion"));
        QApplication::setFont(QFont(QStringLiteral("Microsoft YaHei UI"),9));
        QWidget host;
        host.setWindowTitle(QStringLiteral("IbInput private UI component"));
        auto *layout=new QVBoxLayout(&host);
        auto *anchor=new QPushButton(QStringLiteral("IbInput..."),&host);
        layout->addWidget(anchor); host.resize(580,160); host.move(40,40); host.show();
        QkmIbInputPopup popup(&host);
        QTranslator translator;
        const QPalette light=app.palette();
        struct Scene { const char *language; qreal scale; bool dark; };
        const Scene scenes[]={{"zh_CN",1.0,false},{"en_US",1.0,false},{"ja_JP",1.0,false},
            {"zh_CN",1.5,true},{"ja_JP",1.5,true}};
        int index=0,changed=0,connectionRequests=0;
        QObject::connect(&popup,&QkmIbInputPopup::preferencesChanged,&host,[&] { ++changed; });
        QObject::connect(&popup,&QkmIbInputPopup::connectionCheckRequested,&host,[&] { ++connectionRequests; });
        QTimer timer;
        timer.setInterval(80);
        QElapsedTimer deadline; deadline.start();
        bool awaitingCapture=false;
        QObject::connect(&timer,&QTimer::timeout,&host,[&] {
            try {
                check(desktopName(GetThreadDesktop(GetCurrentThreadId()))==name,"UI thread left its owned private desktop");
                check(initCalls==0 && sendCalls==0,"UI reached an input API");
                if (awaitingCapture) {
                    check(deadline.elapsed()<90000,"UI capture acknowledgement timed out");
                    if (!QFile::exists(root+QStringLiteral("/captured-%1").arg(index))) return;
                    awaitingCapture=false; ++index;
                }
                timer.stop();
                if (index==5) {
                    popup.hide();
                    writeJson(root+QStringLiteral("/result.json"),{{"passed",true},{"scenes",index},
                        {"init_calls",initCalls},{"send_calls",sendCalls},{"desktop",name},
                        {"scope",QStringLiteral("Actual Popup component; simulated C API; no product startup, Hook or driver")}});
                    app.exit(0); return;
                }
                popup.hide(); popup.connectionChecked(); popup.setRunning(false);
                app.removeTranslator(&translator);
                const QString language=QString::fromLatin1(scenes[index].language);
                check(translator.load(translations+QStringLiteral("/QKeyMapper_")+language+QStringLiteral(".qm")),"Formal QM missing");
                app.installTranslator(&translator);
                QPalette palette=light;
                if (scenes[index].dark) {
                    palette.setColor(QPalette::Window,QColor(55,55,55));
                    palette.setColor(QPalette::WindowText,Qt::white);
                    palette.setColor(QPalette::Base,QColor(60,60,60));
                    palette.setColor(QPalette::Text,Qt::white);
                    palette.setColor(QPalette::Button,QColor(55,55,55));
                    palette.setColor(QPalette::ButtonText,Qt::white);
                }
                app.setPalette(palette);
                check(QkmIbInputClient::instance().setPreferences({}),"UI preferences remained busy");
                popup.showAt(anchor,scenes[index].scale);
                QTest::qWait(100);
                const QList<QComboBox *> combos{popup.findChild<QComboBox *>(QStringLiteral("ibInputBackendComboBox")),
                    popup.findChild<QComboBox *>(QStringLiteral("ibInputPolicyComboBox"))};
                const QList<QPushButton *> buttons{popup.findChild<QPushButton *>(QStringLiteral("ibInputRefreshButton")),
                    popup.findChild<QPushButton *>(QStringLiteral("ibInputCheckButton"))};
                check(combos[0] && combos[1] && buttons[0] && buttons[1],"Named Popup controls missing");
                check(combos.size()==2 && buttons.size()==2,"Unexpected Popup controls");
                for (auto *button:buttons) check(!button->autoDefault(),"Popup button is an automatic default");
                const int changedBefore=changed;
                combos[0]->setCurrentIndex(1); combos[1]->setCurrentIndex(1);
                check(changed==changedBefore+2,"Preferences did not report exactly two changes");
                const auto preferences=QkmIbInputClient::instance().preferences();
                check(preferences.backend==QStringLiteral("logitech_ghub")
                    && preferences.unavailablePolicy==QStringLiteral("sendinput_before_start"),"UI preference values changed");
                QTemporaryDir temporary;
                check(temporary.isValid(),"Temporary INI unavailable");
                const QString ini=temporary.path()+QStringLiteral("/ui.ini");
                { QSettings settings(ini,QSettings::IniFormat);
                  QkmIbInputClient::writePreferences(settings,preferences); settings.sync();
                  check(settings.status()==QSettings::NoError,"UI preference save failed"); }
                { QSettings settings(ini,QSettings::IniFormat);
                  const auto restored=QkmIbInputClient::readPreferences(settings);
                  check(restored.backend==preferences.backend && restored.unavailablePolicy==preferences.unavailablePolicy,"UI preference readback changed"); }
                const QString expectedVersion=qEnvironmentVariable("QKM_IBINPUT_EXPECTED_GHUB_VERSION");
                if (!expectedVersion.isEmpty()) check(popup.findChild<QPlainTextEdit *>()->toPlainText().contains(expectedVersion),"Installed G HUB version string was changed");
                const auto before=QkmIbInputClient::instance().statusSnapshot();
                popup.refresh(); buttons[0]->click();
                auto *feedback=popup.findChild<QLabel *>(QStringLiteral("ibInputFeedbackLabel"));
                check(feedback,"Operation feedback label missing");
                const auto expectFeedback=[&](const char *source) {
                    QString actual=feedback->text();
                    actual.replace(QRegularExpression(QStringLiteral("[0-9]{2}:[0-9]{2}:[0-9]{2}")),QStringLiteral("%1"));
                    check(actual==QCoreApplication::translate("QkmIbInputPopup",source),"Operation feedback was not localized or timed");
                };
                expectFeedback("Status refreshed at %1.");
                const QString explicitFeedback=feedback->text();
                popup.refresh();
                check(feedback->text()==explicitFeedback,"Passive refresh replaced the manual operation result");
                check(initCalls==0 && changed==changedBefore+2
                    && QkmIbInputClient::instance().statusSnapshot().sampledAt==before.sampledAt,"Passive refresh changed configuration or connection state");
                popup.setRunning(true); popup.showAt(anchor,scenes[index].scale); QTest::qWait(40);
                check(popup.isVisible(),"Running Popup is not visible");
                check(!combos[0]->isEnabled() && !combos[1]->isEnabled() && !buttons[1]->isEnabled(),"Running UI did not freeze preferences");
                popup.setRunning(false);
                buttons[1]->click();
                check(popup.connectionPending() && !combos[0]->isEnabled(),"Connection request did not freeze preferences");
                expectFeedback("Checking connection...");
                popup.connectionChecked(true);
                expectFeedback("Connection check passed at %1. The temporary connection is closed.");
                check(!popup.connectionPending() && combos[0]->isEnabled(),"Completed connection request stayed locked");
                buttons[1]->click(); popup.connectionChecked(false);
                expectFeedback("Connection check failed at %1. See the result below.");
                combos[0]->showPopup(); QTest::qWait(40);
                check(popup.isVisible(),"ComboBox hid the parent Popup");
                combos[0]->hidePopup(); QTest::qWait(40);
                check(popup.isVisible(),"Closing ComboBox hid the parent Popup");
                QTest::keyClick(&popup,Qt::Key_Escape); QTest::qWait(40);
                check(!popup.isVisible(),"Escape did not close the Popup");
                for (auto *childTimer:popup.findChildren<QTimer *>()) check(!childTimer->isActive(),"Hidden Popup retained its refresh timer");
                popup.showAt(anchor,scenes[index].scale); QTest::qWait(80);
                if (index==0 || index==4) { buttons[1]->click(); popup.connectionChecked(true); }
                else if (index==2) buttons[0]->click();
                else if (index==3) buttons[1]->click();
                QTest::qWait(80);
                check(!popup.findChild<QPlainTextEdit *>()->toPlainText().contains(QStringLiteral("checkpoint"),Qt::CaseInsensitive)
                    && !popup.findChild<QPlainTextEdit *>()->toPlainText().contains(QStringLiteral("检查点")),"Developer checkpoint text remains visible");
                for (auto *widget:QList<QWidget *>{combos[0],combos[1],buttons[0],buttons[1],feedback,popup.findChild<QPlainTextEdit *>()}) {
                    check(widget && popup.rect().contains(QRect(widget->mapTo(&popup,QPoint()),widget->size())),"Popup control clipped by its parent");
                }
                const QString label=popup.findChildren<QLabel *>().front()->text();
                check(language==QStringLiteral("en_US") || label!=QStringLiteral("Preferred backend"),"Runtime language remained English");
                writeJson(root+QStringLiteral("/scene.json"),{{"index",index},{"language",language},
                    {"scale",scenes[index].scale},{"dark",scenes[index].dark},{"pid",static_cast<int>(GetCurrentProcessId())},
                    {"hwnd",QString::number(static_cast<qulonglong>(popup.winId()))},{"desktop",name},
                    {"label",label},{"width",popup.width()},{"height",popup.height()}});
                awaitingCapture=true; deadline.restart(); timer.start();
            } catch (const std::exception &error) {
                const bool popupVisible=popup.isVisible();
                timer.stop(); popup.hide();
                writeJson(root+QStringLiteral("/result.json"),{{"passed",false},{"scene",index},
                    {"error",QString::fromUtf8(error.what())},{"popup_visible",popupVisible},{"pending",popup.connectionPending()},{"init_calls",initCalls},{"send_calls",sendCalls},{"desktop",name}});
                std::cerr<<error.what()<<'\n'; app.exit(2);
            }
        });
        timer.start();
        nativeExit=app.exec();
    }
    const bool restored=SetThreadDesktop(original);
    CloseDesktop(owned);
    return restored ? nativeExit : 2;
}
