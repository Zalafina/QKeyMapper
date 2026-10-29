#include "qkm_ibinput_client.h"
#include <IbInputSimulator/InputSimulator.hpp>
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
std::vector<Send::SendType> attempts;
std::vector<INPUT> events;
bool initFails=false, sendFails=false;
std::function<void()> duringInit;
std::function<void()> duringSend;
void check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
INPUT key(WORD vk,bool up=false) { INPUT value{}; value.type=INPUT_KEYBOARD; value.ki.wVk=vk; value.ki.dwFlags=up ? KEYEVENTF_KEYUP : DWORD{0}; return value; }
void reset() { attempts.clear(); events.clear(); initFails=false; sendFails=false; duringInit={}; duringSend={}; }
void settingsCase() {
    QTemporaryDir temporary(QDir::tempPath()+QStringLiteral("/QKeyMapper-ibinput-XXXXXX"));
    check(temporary.isValid(),"temporary INI failed");
    const QString path=temporary.path()+QStringLiteral("/test.ini");
    QkmIbInputClient::Preferences preferences; preferences.backend=QStringLiteral("logitech_ghub"); preferences.unavailablePolicy=QStringLiteral("sendinput_before_start");
    { QSettings settings(path,QSettings::IniFormat); QkmIbInputClient::writePreferences(settings,preferences); settings.sync(); check(settings.status()==QSettings::NoError,"INI write failed"); }
    { QSettings settings(path,QSettings::IniFormat); const auto restored=QkmIbInputClient::readPreferences(settings); check(restored.backend==preferences.backend && restored.unavailablePolicy==preferences.unavailablePolicy,"global preferences changed"); settings.setValue(QStringLiteral("IbInputPreferredBackend"),QStringLiteral("invalid")); settings.sync(); }
    { QSettings settings(path,QSettings::IniFormat); QString diagnostic; const auto restored=QkmIbInputClient::readPreferences(settings,&diagnostic); check(restored.backend==QStringLiteral("auto") && !diagnostic.isEmpty(),"invalid preference not diagnosed"); }
    const QStringList expected{QStringLiteral("logitech_ghub"),QStringLiteral("logitech_legacy"),QStringLiteral("razer")};
    check(QkmIbInputClient::candidates(QStringLiteral("auto"))==expected,"auto candidate order changed");
    check(QkmIbInputClient::candidates(QStringLiteral("razer"))==QStringList{QStringLiteral("razer")},"manual candidate changed");
}
void admissionCase() {
    QkmIbInputClient client;
    check(!client.sendEvent(key('A'),1) && events.empty(),"uninitialized send reached API");
    QkmIbInputClient::Requirements requirements; requirements.capabilities=QkmIbInputClient::Keyboard;
    const auto generation=client.requestRun();
    check(client.prepareRun(generation,requirements),"mock preparation failed");
    check(attempts.size()==1 && attempts[0]==Send::SendType::LogitechGHubNew,"wrong candidate initialized");
    check(!client.setPreferences({}),"preferences changed during a run");
    check(!client.sendEvent(key('A'),generation+1) && events.empty(),"stale generation sent input");
    check(client.sendEvent(key('A'),generation),"mock down failed");
    check(client.sendEvent(key('A'),generation) && events.size()==1,"duplicate down consumed a slot");
    client.cancelRun();
    check(!client.sendEvent(key('B'),generation),"late down accepted");
    check(client.sendEvent(key('A',true),generation),"release rejected while stopping");
    check(client.closeAfterDrain(generation),"normal close incomplete");
}
void fallbackCase() {
    QkmIbInputClient client;
    QkmIbInputClient::Preferences preferences; preferences.unavailablePolicy=QStringLiteral("sendinput_before_start");
    check(client.setPreferences(preferences),"fallback preference failed");
    initFails=true;
    const auto generation=client.requestRun();
    check(client.prepareRun(generation,{}),"pre-start fallback rejected");
    check(client.statusSnapshot().fallback && !client.statusSnapshot().connected,"fallback not frozen");
    check(!client.sendEvent(key('A'),generation) && events.empty(),"fallback sent through another backend");
    check(client.preferences().backend==QStringLiteral("auto"),"source preference mutated");
    check(client.closeAfterDrain(generation),"fallback close failed");
    reset();
    QkmIbInputClient blocked;
    const auto token=blocked.requestRun();
    QkmIbInputClient::Requirements invalid; invalid.rejection=QStringLiteral("structural conflict");
    check(!blocked.prepareRun(token,invalid) && attempts.empty(),"invalid mapping bypassed admission");
}
QkmIbInputClient::Requirements needs(unsigned capabilities) { QkmIbInputClient::Requirements result; result.capabilities=capabilities; return result; }
void failureCase() {
    QkmIbInputClient client;
    const auto generation=client.requestRun();
    check(client.prepareRun(generation,needs(QkmIbInputClient::Keyboard)),"mock failure preparation failed");
    sendFails=true;
    check(!client.sendEvent(key('A'),generation),"send failure hidden");
    check(attempts.size()==1,"send failure switched backend");
    check(!client.closeAfterDrain(generation) && client.statusSnapshot().releaseIncomplete,"failed release hidden");
    check(client.busy() && !client.setPreferences({}),"incomplete release allowed configuration changes");
    const auto token=client.requestRun();
    check(!client.prepareRun(token,{}) && attempts.size()==1,"incomplete release restarted");
    reset();
    QkmIbInputClient cancelled;
    duringInit=[&cancelled] { cancelled.cancelRun(); };
    const auto cancelledToken=cancelled.requestRun();
    check(!cancelled.prepareRun(cancelledToken,{}) && !cancelled.statusSnapshot().connected,"cancelled initialization stayed connected");
}
void reportCase() {
    QkmIbInputClient client;
    const auto generation=client.requestRun();
    check(client.prepareRun(generation,needs(QkmIbInputClient::Keyboard)),"report preparation failed");
    for (WORD vk='A';vk<='F';++vk) check(client.sendEvent(key(vk),generation),"six keys rejected");
    check(!client.sendEvent(key('G'),generation) && events.size()==6,"full report reached API");
    check(client.closeAfterDrain(generation),"full report cleanup failed");
}
void mouseCase() {
    QkmIbInputClient client;
    const auto generation=client.requestRun();
    check(client.prepareRun(generation,needs(QkmIbInputClient::VerticalWheel)),"mouse preparation failed");
    INPUT wheel{}; wheel.type=INPUT_MOUSE; wheel.mi.dwFlags=MOUSEEVENTF_WHEEL; wheel.mi.mouseData=3*WHEEL_DELTA;
    check(client.sendEvent(wheel,generation) && events.size()==3,"wheel units changed");
    for (const auto &event:events) check(event.mi.mouseData==WHEEL_DELTA,"wheel report unit changed");
    INPUT combined{}; combined.type=INPUT_MOUSE; combined.mi.dwFlags=MOUSEEVENTF_MOVE|MOUSEEVENTF_LEFTUP; combined.mi.dx=1;
    client.cancelRun();
    check(!client.sendEvent(combined,generation) && events.size()==3,"combined release bypassed movement stop");
    check(client.closeAfterDrain(generation),"mouse cleanup incomplete");
}
void capabilityCase() {
    QkmIbInputClient checkOnly;
    const auto connection=checkOnly.requestRun();
    check(checkOnly.prepareRun(connection,{}),"connection-only preparation failed");
    check(!checkOnly.sendEvent(key('A'),connection) && events.empty(),"connection check admitted keyboard input");
    check(checkOnly.closeAfterDrain(connection),"connection-only close failed");
    reset();
    QkmIbInputClient blocked;
    auto requirements=needs(QkmIbInputClient::Buttons|QkmIbInputClient::RelativeMove);
    requirements.buttonStateOverlap=true; requirements.mouseButtons.insert(VK_LBUTTON);
    const auto token=blocked.requestRun();
    check(!blocked.prepareRun(token,requirements) && attempts.empty(),"button overlap reached initialization");
    reset();
    QkmIbInputClient sequential;
    requirements.buttonStateOverlap=false;
    const auto run=sequential.requestRun();
    check(sequential.prepareRun(run,requirements),"sequential basic probe rejected");
    check(sequential.closeAfterDrain(run),"sequential close failed");
}
void concurrentCase() {
    QkmIbInputClient client;
    const auto generation=client.requestRun();
    check(client.prepareRun(generation,needs(QkmIbInputClient::Keyboard)),"concurrent preparation failed");
    std::mutex mutex; std::condition_variable condition; bool entered=false,release=false;
    duringSend=[&] { std::unique_lock<std::mutex> lock(mutex); entered=true; condition.notify_all(); condition.wait(lock,[&] { return release; }); };
    bool sent=false,closed=false;
    std::thread sender([&] { sent=client.sendEvent(key('A'),generation); });
    bool observed=false;
    { std::unique_lock<std::mutex> lock(mutex); observed=condition.wait_for(lock,std::chrono::seconds(3),[&] { return entered; }); }
    client.cancelRun();
    const auto status=client.statusSnapshot();
    std::thread closer([&] { closed=client.closeAfterDrain(generation); });
    { std::lock_guard<std::mutex> lock(mutex); release=true; } condition.notify_all();
    sender.join(); closer.join();
    check(observed && status.connected && sent && closed,"close did not drain the accepted call");
    check(events.size()==2 && (events.back().ki.dwFlags & KEYEVENTF_KEYUP),"accepted key was not released");
}
}
DLLAPI Send::Error __stdcall IbSendInit(Send::SendType type,Send::InitFlags flags,void *argument) {
    check(flags==0 && argument==nullptr,"reserved init arguments changed"); attempts.push_back(type);
    if (duringInit) duringInit();
    return initFails ? Send::Error::DeviceNotFound : Send::Error::Success;
}
DLLAPI void __stdcall IbSendDestroy() {}
DLLAPI UINT WINAPI IbSendInput(UINT count,LPINPUT inputs,int size) {
    check(count==1 && size==sizeof(INPUT),"batched or ABI-incompatible input");
    events.push_back(*inputs); if (duringSend) duringSend(); return sendFails ? 0 : 1;
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    static_assert(sizeof(INPUT)==(sizeof(void *)==8 ? 40 : 28));
    try {
        if (!(QkmIbInputClient::capabilities(QStringLiteral("logitech_ghub")) & QkmIbInputClient::Keyboard)) {
            QkmIbInputClient client;
            const auto token=client.requestRun();
            check(!client.prepareRun(token,needs(QkmIbInputClient::Keyboard)) && attempts.empty(),"unverified product capability reached initialization");
            const auto movement=client.requestRun();
            check(client.prepareRun(movement,needs(QkmIbInputClient::RelativeMove)),"product movement admission failed");
            INPUT move{}; move.type=INPUT_MOUSE; move.mi.dwFlags=MOUSEEVENTF_MOVE; move.mi.dx=1;
            check(client.sendEvent(move,movement) && events.size()==1,"admitted product movement rejected");
            check(!client.sendEvent(key('A'),movement) && events.size()==1,"unadmitted keyboard reached API");
            check(client.closeAfterDrain(movement),"product capability close failed");
            std::cout<<"PASS simulated production capability gate; no driver or real input.\n"; return 0;
        }
        const std::pair<const char *,void(*)()> cases[]={{"settings",settingsCase},{"admission",admissionCase},{"fallback",fallbackCase},{"failure",failureCase},{"report",reportCase},{"mouse",mouseCase},{"capability",capabilityCase},{"concurrent",concurrentCase}};
        for (const auto &test:cases) { reset(); test.second(); std::cout<<"PASS simulated client "<<test.first<<'\n'; }
        std::cout<<"No driver initialization, hooks or actual input; formal C API substituted.\n"; return 0;
    } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 2; }
}
