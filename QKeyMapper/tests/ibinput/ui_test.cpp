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
#include <QTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextLayout>
#include <QRegularExpression>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTranslator>
#include <QVBoxLayout>
#include <iostream>
#include <stdexcept>

#include <setupapi.h>
#include <tlhelp32.h>

namespace {
enum class RazerChannelView { Live, Missing, QueryFailed };
RazerChannelView channelView=RazerChannelView::Live;
HDEVINFO WINAPI testGetDevices(const GUID *guid,PCWSTR enumerator,HWND parent,DWORD flags) {
    if (channelView==RazerChannelView::Live) return SetupDiGetClassDevsW(guid,enumerator,parent,flags);
    if (channelView==RazerChannelView::QueryFailed) { SetLastError(ERROR_ACCESS_DENIED); return INVALID_HANDLE_VALUE; }
    return reinterpret_cast<HDEVINFO>(1);
}
BOOL WINAPI testEnumInterfaces(HDEVINFO devices,PSP_DEVINFO_DATA info,const GUID *guid,DWORD index,PSP_DEVICE_INTERFACE_DATA item) {
    if (channelView==RazerChannelView::Live) return SetupDiEnumDeviceInterfaces(devices,info,guid,index,item);
    SetLastError(ERROR_NO_MORE_ITEMS); return FALSE;
}
BOOL WINAPI testDestroyDevices(HDEVINFO devices) {
    return channelView==RazerChannelView::Live ? SetupDiDestroyDeviceInfoList(devices) : TRUE;
}
}
LSTATUS WINAPI testOpenRegistration(HKEY key,LPCWSTR subkey,DWORD options,REGSAM access,PHKEY output) {
    if (channelView!=RazerChannelView::Live && QString::fromWCharArray(subkey).endsWith(QStringLiteral("logi_joy_bus_enum")))
        return channelView==RazerChannelView::Missing ? ERROR_FILE_NOT_FOUND : ERROR_ACCESS_DENIED;
    return RegOpenKeyExW(key,subkey,options,access,output);
}
HANDLE WINAPI testProcesses(DWORD flags,DWORD pid) {
    if (channelView==RazerChannelView::QueryFailed) { SetLastError(ERROR_ACCESS_DENIED); return INVALID_HANDLE_VALUE; }
    return CreateToolhelp32Snapshot(flags,pid);
}
// Keep the real Popup while substituting passive query failures in selected scenes.
#define RegOpenKeyExW testOpenRegistration
#define CreateToolhelp32Snapshot testProcesses
#define SetupDiGetClassDevsW testGetDevices
#define SetupDiEnumDeviceInterfaces testEnumInterfaces
#define SetupDiDestroyDeviceInfoList testDestroyDevices
#include "../../qkm_ibinput_popup.cpp"
#undef RegOpenKeyExW
#undef CreateToolhelp32Snapshot
#undef SetupDiGetClassDevsW
#undef SetupDiEnumDeviceInterfaces
#undef SetupDiDestroyDeviceInfoList


namespace {
int initCalls=0,fixtureInitCalls=0,sendCalls=0;
bool acceptGhub=false,acceptRazer=false;
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
DLLAPI Send::Error __stdcall IbSendInit(Send::SendType type,Send::InitFlags,void *) {
    ++initCalls;
    return (acceptGhub && type==Send::SendType::LogitechGHubNew) || (acceptRazer && type==Send::SendType::Razer)
        ? Send::Error::Success : Send::Error::DeviceNotFound;
}
DLLAPI void __stdcall IbSendDestroy() {}
DLLAPI UINT WINAPI IbSendInput(UINT,LPINPUT,int) { ++sendCalls; return 0; }

int main(int argc,char **argv) {
    if (argc!=2 || QString::fromLocal8Bit(argv[1])!=QStringLiteral("--ibinput-ui-check")) return 2;
    const QString root=qEnvironmentVariable("QKM_IBINPUT_UI_ROOT");
    const QString translations=qEnvironmentVariable("QKM_IBINPUT_QM_ROOT");
    if (!QDir(root).exists() || !QDir(translations).exists()) return 2;
    const QString name=QStringLiteral("QkmIbInputUi_%1").arg(GetCurrentProcessId());
    const HDESK original=GetThreadDesktop(GetCurrentThreadId());
    // Qt 5 needs a thread-local GetMessage hook on this private desktop.
    const HDESK owned=CreateDesktopW(reinterpret_cast<LPCWSTR>(name.utf16()),nullptr,nullptr,0,
        DESKTOP_CREATEWINDOW|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS|DESKTOP_ENUMERATE|DESKTOP_HOOKCONTROL,nullptr);
    if (!owned || !SetThreadDesktop(owned)) { if (owned) CloseDesktop(owned); return 2; }
    int nativeExit=2;
    {
        std::cerr<<"UI runtime "<<qVersion()<<'\n';
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
        struct Scene { const char *language; qreal scale; bool dark; RazerChannelView view=RazerChannelView::Live; int backendCase=0; int errorCase=0; };
        const Scene scenes[]={{"zh_CN",1.0,false},{"en_US",1.0,false},{"ja_JP",1.0,false},
            {"zh_CN",1.5,true},{"ja_JP",1.5,true},{"zh_CN",1.0,false,RazerChannelView::Missing},
            {"zh_CN",1.0,false,RazerChannelView::QueryFailed},{"zh_CN",1.0,true},{"zh_CN",1.0,true},
            {"zh_CN",1.0,false,RazerChannelView::Live,1},{"zh_CN",1.0,true,RazerChannelView::Live,2},
            {"en_US",1.0,false,RazerChannelView::Live,3},{"ja_JP",1.5,true,RazerChannelView::Live,4},
            {"zh_CN",1.0,true,RazerChannelView::Live,3},{"zh_CN",1.0,false,RazerChannelView::Live,4},
            {"zh_CN",1.0,true,RazerChannelView::Live,0,1},{"en_US",1.0,false,RazerChannelView::Live,0,1},
            {"ja_JP",1.5,true,RazerChannelView::Live,0,1},{"zh_CN",1.0,true,RazerChannelView::Live,0,2},
            {"en_US",1.0,false,RazerChannelView::Live,0,2},{"ja_JP",1.5,true,RazerChannelView::Live,0,2}};
        constexpr int sceneCount=sizeof(scenes)/sizeof(*scenes);
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
                check(initCalls==fixtureInitCalls && sendCalls==0,"UI reached an input API");
                if (awaitingCapture) {
                    check(deadline.elapsed()<90000,"UI capture acknowledgement timed out");
                    if (!QFile::exists(root+QStringLiteral("/captured-%1").arg(index))) return;
                    const auto completed=QkmIbInputClient::instance().statusSnapshot();
                    if (completed.connected) check(QkmIbInputClient::instance().closeAfterDrain(completed.generation),"Fixture connection did not close");
                    popup.setRunning(false);
                    awaitingCapture=false; ++index;
                }
                timer.stop();
                if (index==sceneCount) {
                    popup.hide();
                    writeJson(root+QStringLiteral("/result.json"),{{"passed",true},{"scenes",index},
                        {"init_calls",initCalls},{"fixture_init_calls",fixtureInitCalls},{"unexpected_init_calls",initCalls-fixtureInitCalls},{"send_calls",sendCalls},{"desktop",name},
                        {"scope",QStringLiteral("Actual Popup component; simulated input C API; no product startup, mapping Hook or driver")}});
                    app.exit(0); return;
                }
                popup.hide(); popup.connectionChecked(); popup.setRunning(false);
                app.removeTranslator(&translator);
                channelView=scenes[index].view;
                const QString language=QString::fromLatin1(scenes[index].language);
                check(translator.load(translations+QStringLiteral("/QKeyMapper_")+language+QStringLiteral(".qm")),"Formal QM missing");
                for (const char *source:{"Logitech G HUB","Razer driver","Current backend: %1 (fallback: %2)","Updated at %1","Driver registered","Driver not registered","Query failed","Version: %1","Processes: %1","Services: %1","Connection check failed at %1.","Detected","Not detected","Automatic order: %1","Razer","%1 (automatic selection)","Connection check passed at %1: automatically selected %2. The temporary connection is closed."})
                    check(!translator.translate("QkmIbInputPopup",source).isEmpty(),"Razer QM source is missing");
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
                auto *autoHint=popup.findChild<QLabel *>(QStringLiteral("ibInputAutoHintLabel"));
                check(autoHint && autoHint->isVisible(),"Automatic order hint is missing");
                QStringList displayedOrder;
                for (int candidate=1;candidate<combos[0]->count();++candidate) displayedOrder.append(combos[0]->itemText(candidate));
                check(autoHint->text()==QCoreApplication::translate("QkmIbInputPopup","Automatic order: %1").arg(displayedOrder.join(QStringLiteral(" → "))),"Displayed automatic order differs from the selector");
                const int changedBefore=changed;
                combos[0]->setCurrentIndex(1); combos[1]->setCurrentIndex(1);
                check(changed==changedBefore+2,"Preferences did not report exactly two changes");
                check(!autoHint->isVisible(),"Manual selection retained the automatic hint");
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
                if (!expectedVersion.isEmpty()) check(popup.findChild<QTextEdit *>(QStringLiteral("ibInputDetails"))->toPlainText().contains(expectedVersion),"Installed G HUB version string was changed");
                const auto before=QkmIbInputClient::instance().statusSnapshot();
                popup.refresh(); buttons[0]->click();
                auto *feedback=popup.findChild<QTextEdit *>(QStringLiteral("ibInputFeedbackLabel"));
                check(feedback,"Operation feedback label missing");
                check(feedback->frameShape()==QFrame::WinPanel && feedback->frameShadow()==QFrame::Sunken,
                    "Feedback does not follow the existing sunken panel style");
                const auto feedbackText=[&] { return feedback->toPlainText(); };
                const auto expectFeedback=[&](const char *source) {
                    QString actual=feedbackText().section(QLatin1Char('\n'),0,0);
                    actual.replace(QRegularExpression(QStringLiteral("[0-9]{2}:[0-9]{2}:[0-9]{2}")),QStringLiteral("%1"));
                    check(actual==QCoreApplication::translate("QkmIbInputPopup",source),"Operation feedback was not localized or timed");
                };
                expectFeedback("Status refreshed at %1.");
                const QString explicitFeedback=feedbackText();
                popup.refresh();
                check(feedbackText()==explicitFeedback,"Passive refresh replaced the manual operation result");
                check(initCalls==fixtureInitCalls && changed==changedBefore+2
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
                expectFeedback("Connection check failed at %1.");
                combos[0]->showPopup(); QTest::qWait(40);
                check(popup.isVisible(),"ComboBox hid the parent Popup");
                combos[0]->hidePopup(); QTest::qWait(40);
                check(popup.isVisible(),"Closing ComboBox hid the parent Popup");
                QTest::keyClick(&popup,Qt::Key_Escape); QTest::qWait(40);
                check(!popup.isVisible(),"Escape did not close the Popup");
                for (auto *childTimer:popup.findChildren<QTimer *>()) check(!childTimer->isActive(),"Hidden Popup retained its refresh timer");
                popup.showAt(anchor,scenes[index].scale); QTest::qWait(80);
                if (index==0 || index==3 || index==4) { buttons[1]->click(); popup.connectionChecked(true); }
                else if (index==2) buttons[0]->click();
                QTest::qWait(80);
                if (index==0 || index==7 || channelView!=RazerChannelView::Live) { combos[0]->setCurrentIndex(3); popup.refresh(); }
                if (index==0 || index==7) { buttons[1]->click(); popup.connectionChecked(true); }
                auto *details=popup.findChild<QTextEdit *>(QStringLiteral("ibInputDetails"));
                const QString text=details->toPlainText();
                for (const char *source:{"Logitech G HUB","Razer driver","Services: %1"})
                    check(text.contains(QCoreApplication::translate("QkmIbInputPopup",source).replace(QStringLiteral("%1"),QString())),
                        "Grouped passive status was not refreshed in the active language");
                check(text.contains(QCoreApplication::translate("QkmIbInputPopup","Updated at %1").arg(QString())),"Compact sample time missing");
                check(details->horizontalScrollBar()->maximum()==0,"Normal grouped status needs horizontal scrolling");
                check(details->verticalScrollBar()->maximum()==0,"Normal grouped status needs vertical scrolling");
                check(details->isReadOnly(),"Grouped status is editable");
                QTextCursor selection=details->document()->find(QCoreApplication::translate("QkmIbInputPopup","Razer driver"));
                check(!selection.isNull(),"Cannot select grouped status");
                details->setTextCursor(selection); popup.refresh();
                check(details->textCursor().selectedText()==selection.selectedText(),"Passive refresh lost the selected text");
                details->setTextCursor(QTextCursor(details->document()));
                if (channelView==RazerChannelView::Missing)
                    check(text.contains(QCoreApplication::translate("QkmIbInputPopup","Driver not registered")),"Missing G HUB registration was not distinguished");
                if (channelView==RazerChannelView::QueryFailed)
                    check(text.contains(QCoreApplication::translate("QkmIbInputPopup","Query failed")),"Query failure was reported as absence");
                check(combos[0]->itemText(3)==QCoreApplication::translate("QkmIbInputPopup","Razer"),"Razer backend name was not translated");
                auto *help=popup.findChild<QLabel *>(QStringLiteral("ibInputHelpLabel"));
                check(help && help->text().contains(QStringLiteral("https://github.com/Chaoses-Ib/IbInputSimulator/issues/7"))
                    && !help->text().contains(QStringLiteral("www.razer.com/synapse")),"Razer guide points to an incompatible download");
                const char *missingGuide="QKeyMapper did not detect a Razer driver that IbInput can use. See the installation requirements in the Razer driver guide.";
                const char *failedGuide="QKeyMapper could not check the Razer driver status. Click Refresh status or Check connection.";
                if (channelView==RazerChannelView::Missing)
                    check(help->text().contains(QCoreApplication::translate("QkmIbInputPopup",missingGuide).toHtmlEscaped()),
                        "Missing Razer driver has no actionable guidance");
                else if (channelView==RazerChannelView::QueryFailed)
                    check(help->text().contains(QCoreApplication::translate("QkmIbInputPopup",failedGuide).toHtmlEscaped())
                        && !help->text().contains(QCoreApplication::translate("QkmIbInputPopup",missingGuide).toHtmlEscaped()),
                        "Query failure recommends driver installation");
                else check(!help->text().contains(QCoreApplication::translate("QkmIbInputPopup",missingGuide).toHtmlEscaped()),
                    "Observed Razer driver still recommends installing it");
                check(initCalls==fixtureInitCalls && sendCalls==0,"Razer observation initialized a driver");
                check(!text.contains(QStringLiteral("Currently supports"))
                    && !text.contains(QCoreApplication::translate("QkmIbInputPopup","Last result: %1").arg(QString()))
                    && !text.contains(QCoreApplication::translate("QkmIbInputPopup","Status refresh does not initialize a driver.")),
                    "Central status still contains duplicate or developer text");
                if (index==0) {
                    const QString lightHtml=details->toHtml();
                    QPalette changedPalette=app.palette(); changedPalette.setColor(QPalette::Window,QColor(55,55,55));
                    changedPalette.setColor(QPalette::Text,Qt::white); changedPalette.setColor(QPalette::Base,QColor(60,60,60));
                    app.setPalette(changedPalette); QTest::qWait(30);
                    check(details->toHtml()!=lightHtml && details->toHtml().contains(QStringLiteral("#b0b0b0")),"Visible theme change did not recolor the text");
                    app.setPalette(light); QTest::qWait(30);
                    QTranslator liveLanguage;
                    check(liveLanguage.load(translations+QStringLiteral("/QKeyMapper_ja_JP.qm")),"Live language QM missing");
                    app.removeTranslator(&translator); app.installTranslator(&liveLanguage); QTest::qWait(30);
                    check(details->toPlainText().contains(liveLanguage.translate("QkmIbInputPopup","Razer driver")),"Visible language change did not rebuild status");
                    app.removeTranslator(&liveLanguage); app.installTranslator(&translator); QTest::qWait(30);
                    const int height=details->minimumHeight();
                    details->setMinimumHeight(0); details->setMaximumHeight(30); popup.adjustSize(); QTest::qWait(30);
                    check(details->verticalScrollBar()->maximum()>0,"Constrained status has no optional scrollbar");
                    details->verticalScrollBar()->setValue(details->verticalScrollBar()->maximum());
                    const int scroll=details->verticalScrollBar()->value(); popup.refresh();
                    check(details->verticalScrollBar()->value()==scroll,"Refresh lost the scrolled position");
                    details->setMaximumHeight(QWIDGETSIZE_MAX); details->setMinimumHeight(height); popup.adjustSize(); QTest::qWait(30);
                }
                if (index==8) {
                    QkmIbInputClient::Requirements rejection;
                    rejection.rejection=QStringLiteral("razer: 6 <driver> & channel; logitech_ghub: 5; cleanup detail: ")+QString(90,QLatin1Char('X'));
                    const auto generation=QkmIbInputClient::instance().requestRun();
                    check(!QkmIbInputClient::instance().prepareRun(generation,rejection),"UI error fixture was accepted");
                    popup.connectionChecked(false); popup.adjustSize(); QTest::qWait(30);
                    check(feedbackText().contains(rejection.rejection),"Failure reason or literal markup was lost");
                    check(!details->toPlainText().contains(rejection.rejection),"Failure reason remained duplicated in the central panel");
                    check(popup.width()<=480,"Long feedback unnecessarily widened the Popup");
                    check(feedback->document()->lastBlock().layout()->lineCount()>2,"Unbroken failure text was not wrapped");
                    check(feedback->isReadOnly() && feedback->maximumHeight()<popup.height()/2,"Long feedback cannot be safely inspected");
                    const QString originalError=rejection.rejection;
                    rejection.rejection=QString(2048,QLatin1Char('X'));
                    check(!QkmIbInputClient::instance().prepareRun(QkmIbInputClient::instance().requestRun(),rejection),"Long error fixture was accepted");
                    popup.connectionChecked(false); QTest::qWait(30);
                    check(feedback->verticalScrollBar()->maximum()>0 && feedbackText().contains(rejection.rejection),"Very long feedback is truncated instead of scrollable");
                    rejection.rejection=originalError;
                    check(!QkmIbInputClient::instance().prepareRun(QkmIbInputClient::instance().requestRun(),rejection),"Restored error fixture was accepted");
                    popup.connectionChecked(false); QTest::qWait(30);
                }
                if (scenes[index].backendCase) {
                    auto &client=QkmIbInputClient::instance();
                    combos[0]->setCurrentIndex(0); combos[1]->setCurrentIndex(0); popup.refresh();
                    check(autoHint->isVisible(),"Automatic hint did not return after manual selection");
                    const auto runCheck=[&](int target) {
                        buttons[1]->click(); acceptGhub=target==1; acceptRazer=target==2;
                        const bool success=client.checkConnection();
                        fixtureInitCalls+=target==1 ? 1 : 3;
                        check(initCalls==fixtureInitCalls && success==(target!=0),"Fixture automatic connection order changed");
                        return qMakePair(success,success ? client.statusSnapshot().backend : QString());
                    };
                    const bool razer=scenes[index].backendCase==2;
                    auto checked=runCheck(razer ? 2 : 1);
                    check(checked.second==(razer ? QStringLiteral("razer") : QStringLiteral("logitech_ghub")),"Check fixture chose another backend");
                    const bool runtime=scenes[index].backendCase>=3;
                    quint64 generation=0;
                    if (runtime) {
                        acceptGhub=false; acceptRazer=true;
                        QkmIbInputClient::Requirements requirements; requirements.capabilities=QkmIbInputClient::Keyboard;
                        generation=client.requestRun();
                        check(client.prepareRun(generation,requirements),"Runtime fixture did not connect"); fixtureInitCalls+=2;
                        check(initCalls==fixtureInitCalls && client.statusSnapshot().backend==QStringLiteral("razer"),"Runtime fixture did not skip an unsupported legacy backend");
                    }
                    popup.connectionChecked(checked.first,checked.second);
                    const char *passed="Connection check passed at %1: automatically selected %2. The temporary connection is closed.";
                    const QString selected=QCoreApplication::translate("QkmIbInputPopup",razer ? "Razer" : "Logitech G HUB");
                    QString actual=feedbackText().section(QLatin1Char('\n'),0,0);
                    actual.replace(QRegularExpression(QStringLiteral("[0-9]{2}:[0-9]{2}:[0-9]{2}")),QStringLiteral("%1"));
                    check(actual==QCoreApplication::translate("QkmIbInputPopup",passed).arg(QStringLiteral("%1"),selected),"Check feedback did not keep its own backend result");
                    if (scenes[index].backendCase==1) {
                        const QString previous=feedbackText();
                        combos[0]->setCurrentIndex(3); combos[0]->setCurrentIndex(0); popup.refresh();
                        check(feedbackText()!=previous,"Preference change restored a stale successful check");
                        popup.connectionChecked(true,checked.second);
                        check(feedbackText()!=previous,"Delayed check result overwrote a changed preference");
                        checked=runCheck(0); popup.connectionChecked(checked.first,checked.second);
                        expectFeedback("Connection check failed at %1.");
                        checked=runCheck(1); popup.connectionChecked(checked.first,checked.second);
                    }
                    if (runtime) {
                        popup.setRunning(true); popup.refresh();
                        check(details->toPlainText().contains(QCoreApplication::translate("QkmIbInputPopup","%1 (automatic selection)").arg(QCoreApplication::translate("QkmIbInputPopup","Razer"))),"Runtime did not show its actual automatically selected backend");
                        check(!feedbackText().contains(QCoreApplication::translate("QkmIbInputPopup",passed).arg(QStringLiteral("%1"),selected)),"Historical check replaced current runtime status");
                        if (scenes[index].backendCase==4) {
                            check(client.closeAfterDrain(generation),"Stopped fixture did not close"); popup.setRunning(false); popup.refresh();
                            check(details->toPlainText().contains(QCoreApplication::translate("QkmIbInputPopup","Not connected")),"Stopped fixture still claims a connection");
                            check(feedbackText().contains(selected),"Stopped view lost its last check backend");
                            check(feedbackText().contains(QCoreApplication::translate("QkmIbInputPopup","Last result: %1").arg(client.statusSnapshot().detail)),"Stopped view presents a historical detail as a current connection");
                        }
                    }
                    const int beforeRefresh=initCalls,changes=changed;
                    popup.refresh(); buttons[0]->click();
                    check(initCalls==beforeRefresh && changed==changes,"Automatic hint triggered initialization or configuration changes");
                    // Keep the check result visible for the stopped previews after the refresh assertion.
                    if (!runtime) { checked=runCheck(razer ? 2 : 1); popup.connectionChecked(checked.first,checked.second); }
                    else if (scenes[index].backendCase==4) { popup.setRunning(true); popup.setRunning(false); popup.refresh(); }
                    acceptGhub=false; acceptRazer=false;
                    popup.adjustSize(); QTest::qWait(30);
                    check(details->horizontalScrollBar()->maximum()==0 && details->verticalScrollBar()->maximum()==0,"Automatic backend status does not fit");
                }
                if (scenes[index].errorCase) {
                    QkmIbInputClient::Requirements rejection;
                    const char *source=scenes[index].errorCase==1
                        ? "Logitech G HUB could not reach (%1, %2); last position: (%3, %4). Mapping must stop."
                        : "IbInput could not read the cursor position (Windows error %1). Mapping must stop.";
                    const QString translated=QCoreApplication::translate("QkmIbInputClient",source);
                    check(language==QStringLiteral("en_US") || translated!=QString::fromLatin1(source),"Position error translation is missing");
                    rejection.rejection=scenes[index].errorCase==1 ? translated.arg(-3456).arg(7890).arg(-1234).arg(5678) : translated.arg(5);
                    check(!QkmIbInputClient::instance().prepareRun(QkmIbInputClient::instance().requestRun(),rejection),"Position error fixture was accepted");
                    popup.connectionChecked(false); popup.adjustSize(); QTest::qWait(30);
                    check(feedbackText().contains(rejection.rejection) && !feedbackText().contains(QLatin1String("%1")),"Position error text or placeholders were lost");
                    check(feedback->horizontalScrollBar()->maximum()==0,"Position error did not wrap");
                }
                check(!popup.findChild<QTextEdit *>(QStringLiteral("ibInputDetails"))->toPlainText().contains(QStringLiteral("checkpoint"),Qt::CaseInsensitive)
                    && !popup.findChild<QTextEdit *>(QStringLiteral("ibInputDetails"))->toPlainText().contains(QStringLiteral("检查点")),"Developer checkpoint text remains visible");
                for (auto *widget:QList<QWidget *>{combos[0],combos[1],buttons[0],buttons[1],feedback,popup.findChild<QTextEdit *>(QStringLiteral("ibInputDetails"))}) {
                    check(widget && popup.rect().contains(QRect(widget->mapTo(&popup,QPoint()),widget->size())),"Popup control clipped by its parent");
                }
                if (autoHint->isVisible()) check(popup.rect().contains(QRect(autoHint->mapTo(&popup,QPoint()),autoHint->size())),"Automatic hint was clipped");
                const QString label=popup.findChildren<QLabel *>().front()->text();
                check(language==QStringLiteral("en_US") || label!=QStringLiteral("Preferred backend"),"Runtime language remained English");
                writeJson(root+QStringLiteral("/scene.json"),{{"index",index},{"language",language},
                    {"scale",scenes[index].scale},{"dark",scenes[index].dark},{"error_fixture",scenes[index].errorCase},{"feedback",feedbackText()},{"backend_fixture",scenes[index].backendCase},{"interface_simulation",channelView!=RazerChannelView::Live},{"pid",static_cast<int>(GetCurrentProcessId())},
                    {"hwnd",QString::number(static_cast<qulonglong>(popup.winId()))},{"desktop",name},
                    {"label",label},{"width",popup.width()},{"height",popup.height()}});
                awaitingCapture=true; deadline.restart(); timer.start();
            } catch (const std::exception &error) {
                const bool popupVisible=popup.isVisible();
                timer.stop(); popup.hide();
                writeJson(root+QStringLiteral("/result.json"),{{"passed",false},{"scene",index},
                    {"error",QString::fromUtf8(error.what())},{"popup_visible",popupVisible},{"pending",popup.connectionPending()},{"init_calls",initCalls},{"fixture_init_calls",fixtureInitCalls},{"unexpected_init_calls",initCalls-fixtureInitCalls},{"send_calls",sendCalls},{"desktop",name}});
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
