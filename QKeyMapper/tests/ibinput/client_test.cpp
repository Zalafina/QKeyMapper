#include "qkm_ibinput_client.h"
#include <IbInputSimulator/InputSimulator.hpp>
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <algorithm>
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
int failSendAt=0;
std::function<void()> duringInit;
std::function<void()> duringSend;
void check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
INPUT key(WORD vk,bool up=false) { INPUT value{}; value.type=INPUT_KEYBOARD; value.ki.wVk=vk; value.ki.dwFlags=up ? KEYEVENTF_KEYUP : DWORD{0}; return value; }
struct MouseButton { WORD vk; DWORD down,up,data; };
const MouseButton mouseButtons[]={
    {VK_LBUTTON,MOUSEEVENTF_LEFTDOWN,MOUSEEVENTF_LEFTUP,0},
    {VK_RBUTTON,MOUSEEVENTF_RIGHTDOWN,MOUSEEVENTF_RIGHTUP,0},
    {VK_MBUTTON,MOUSEEVENTF_MIDDLEDOWN,MOUSEEVENTF_MIDDLEUP,0},
    {VK_XBUTTON1,MOUSEEVENTF_XDOWN,MOUSEEVENTF_XUP,XBUTTON1},
    {VK_XBUTTON2,MOUSEEVENTF_XDOWN,MOUSEEVENTF_XUP,XBUTTON2}
};
INPUT mouse(DWORD flags,LONG x=0,LONG y=0,DWORD data=0) {
    INPUT value{}; value.type=INPUT_MOUSE; value.mi.dwFlags=flags;
    value.mi.dx=x; value.mi.dy=y; value.mi.mouseData=data; return value;
}
void reset() { attempts.clear(); events.clear(); initFails=false; sendFails=false; failSendAt=0; duringInit={}; duringSend={}; }
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
void razerCase() {
    QkmIbInputClient client;
    QkmIbInputClient::Preferences preferences; preferences.backend=QStringLiteral("razer");
    check(client.setPreferences(preferences),"Razer preference rejected");
    QkmIbInputClient::Requirements required;
    required.capabilities=QkmIbInputClient::Keyboard|QkmIbInputClient::Buttons|QkmIbInputClient::RelativeMove|QkmIbInputClient::VerticalWheel;
    required.keys={'F',VK_RCONTROL,VK_DIVIDE};
    const auto token=client.requestRun();
    check(client.prepareRun(token,required) && attempts==std::vector<Send::SendType>{Send::SendType::Razer},
        "Explicit Razer did not select its own channel");
    check(client.sendEvent(key('F'),token) && client.sendEvent(mouse(MOUSEEVENTF_LEFTDOWN),token),"Razer owned DOWN failed");
    check(client.sendEvent(mouse(MOUSEEVENTF_MOVE,8,0),token)
        && client.sendEvent(mouse(MOUSEEVENTF_WHEEL,0,0,WHEEL_DELTA),token),"Razer held move/wheel rejected");
    client.cancelRun();
    check(client.closeAfterDrain(token) && events.size()==6 && !client.statusSnapshot().releaseIncomplete,
        "Razer same-channel cleanup failed");
    reset();
    required.keys={VK_PAUSE};
    QkmIbInputClient pause; check(pause.setPreferences(preferences),"Razer Pause preference rejected");
    const auto next=pause.requestRun();
    check(pause.prepareRun(next,required) && pause.sendEvent(key(VK_PAUSE),next)
        && pause.sendEvent(key(VK_PAUSE,true),next) && pause.closeAfterDrain(next),"Razer Pause did not round-trip");
    reset();
    QkmIbInputClient automatic;
    duringInit=[] { initFails=attempts.back()!=Send::SendType::Razer; };
    required.keys={'F'};
    const auto automaticToken=automatic.requestRun();
    check(automatic.prepareRun(automaticToken,required)
        && attempts==std::vector<Send::SendType>{Send::SendType::LogitechGHubNew,Send::SendType::Razer}
        && automatic.statusSnapshot().backend==QStringLiteral("razer") && !automatic.statusSnapshot().fallback,
        "Automatic selection did not reach Razer after G HUB failed");
    check(automatic.sendEvent(key('F'),automaticToken) && automatic.sendEvent(key('F',true),automaticToken)
        && automatic.closeAfterDrain(automaticToken),"Automatic Razer run did not release");
    reset(); initFails=true;
    QkmIbInputClient blocked; check(blocked.setPreferences(preferences),"Manual Razer failure preference rejected");
    check(!blocked.prepareRun(blocked.requestRun(),required)
        && attempts==std::vector<Send::SendType>{Send::SendType::Razer},"Manual Razer failure tried another backend");
    reset(); initFails=true; preferences.unavailablePolicy=QStringLiteral("sendinput_before_start");
    QkmIbInputClient fallback; check(fallback.setPreferences(preferences),"Manual Razer fallback preference rejected");
    const auto frozen=fallback.requestRun();
    check(fallback.prepareRun(frozen,required) && fallback.statusSnapshot().fallback && !fallback.statusSnapshot().connected
        && !fallback.sendEvent(key('F'),frozen) && events.empty()
        && attempts==std::vector<Send::SendType>{Send::SendType::Razer} && fallback.closeAfterDrain(frozen),
        "Razer pre-start SendInput fallback sent through a driver or changed candidates");
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
void restartCase() {
    QkmIbInputClient client;
    QkmIbInputClient::Requirements required; required.capabilities=QkmIbInputClient::Keyboard;
    const auto old=client.requestRun();
    check(client.prepareRun(old,required) && client.sendEvent(key('F'),old),"restart baseline failed");
    const auto next=client.requestRun();
    check(!client.sendEvent(key('G'),old),"old task sent after new intent");
    check(client.closeAfterDrain(old) && events.size()==2 && (events.back().ki.dwFlags & KEYEVENTF_KEYUP),
        "old report did not release before restart");
    check(client.acceptingInput(),"old close cancelled the new intent");
    client.cancelRun(old);
    check(client.acceptingInput() && client.prepareRun(next,required),"old cancellation reached the new run");
    check(client.sendEvent(key('G'),next) && client.closeAfterDrain(next),"new run could not send/release");
    const auto third=client.requestRun();
    check(client.prepareRun(third,required),"second restart baseline failed");
    const auto cancelled=client.requestRun();
    client.cancelRun();
    check(client.closeAfterDrain(third) && !client.acceptingInput(),"old close revived a cancelled intent");
    check(!client.prepareRun(cancelled,required),"a stop before restart was ignored");
}
void stopActionCase() {
    QkmIbInputClient client;
    QkmIbInputClient::Requirements required;
    required.capabilities=QkmIbInputClient::Keyboard|QkmIbInputClient::Buttons|QkmIbInputClient::RelativeMove|QkmIbInputClient::VerticalWheel;
    const auto token=client.requestRun();
    check(client.prepareRun(token,required) && !client.beginStopAction(token),"normal run gained stop-action scope");
    check(client.sendEvent(key('F'),token),"stop-action baseline failed");
    client.cancelRun();
    check(!client.beginStopAction(token),"stop action began before owned inputs drained");
    check(client.releaseOwnedInputs(token) && events.size()==2,"old report did not release before stop action");
    check(client.beginStopAction(token) && client.stopActionAllowed(token),"drained stop scope was not granted");
    check(!client.sendEvent(key('G'),token),"ordinary DOWN bypassed cancellation");
    check(client.sendEvent(key('G'),token,true),"explicit stop DOWN rejected");
    check(client.sendEvent(mouse(MOUSEEVENTF_LEFTDOWN),token,true),"explicit stop button rejected");
    check(client.sendEvent(mouse(MOUSEEVENTF_MOVE,1,0),token,true)
        && client.sendEvent(mouse(MOUSEEVENTF_WHEEL,0,0,WHEEL_DELTA),token,true),"explicit stop move/wheel rejected");
    client.cancelRun();
    check(!client.stopActionAllowed(token) && !client.sendEvent(key('H'),token,true),"cancelled stop scope sent a new DOWN");
    check(client.closeAfterDrain(token) && !client.stopActionAllowed(token),"stop scope did not close");
    const auto next=client.requestRun();
    check(client.prepareRun(next,required) && !client.stopActionAllowed(next),"stop scope leaked to next run");
    check(client.sendEvent(key('F'),next),"release-failure baseline failed");
    client.cancelRun(); sendFails=true;
    check(!client.releaseOwnedInputs(next) && !client.beginStopAction(next),"failed release opened stop scope");
    check(!client.sendEvent(key('G'),next,true),"failed release admitted a stop DOWN");
    sendFails=false; check(client.closeAfterDrain(next),"same-backend final release failed");
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
void modifiersCase() {
    QkmIbInputClient client;
    const auto token=client.requestRun();
    check(client.prepareRun(token,needs(QkmIbInputClient::Keyboard)),"modifier preparation failed");
    const WORD modifiers[]={VK_LCONTROL,VK_RCONTROL,VK_LSHIFT,VK_RSHIFT,VK_LMENU,VK_RMENU,VK_LWIN,VK_RWIN};
    for (WORD vk:modifiers) check(client.sendEvent(key(vk),token),"modifier DOWN failed");
    for (WORD vk='A';vk<='F';++vk) check(client.sendEvent(key(vk),token),"modifiers consumed an ordinary key slot");
    check(events.size()==14,"modifier report event count changed");
    check(!client.sendEvent(key('G'),token) && events.size()==14,"seventh ordinary key reached the API");
    check(client.closeAfterDrain(token) && events.size()==28,"mixed keyboard report did not fully release");
    for (const auto &event:events)
        check(event.type==INPUT_KEYBOARD,"keyboard cleanup changed event type");
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
    auto requirements=needs(QkmIbInputClient::HorizontalWheel);
    const auto token=blocked.requestRun();
    check(!blocked.prepareRun(token,requirements) && attempts.empty(),"unsupported wheel reached initialization");
    reset();
    QkmIbInputClient combined;
    requirements=needs(QkmIbInputClient::Buttons|QkmIbInputClient::RelativeMove|QkmIbInputClient::VerticalWheel);
    for (const auto &button:mouseButtons) requirements.mouseButtons.insert(button.vk);
    const auto run=combined.requestRun();
    check(combined.prepareRun(run,requirements),"supported mouse primitives rejected");
    check(combined.closeAfterDrain(run),"combined capability close failed");
}
void buttonsCase() {
    QkmIbInputClient client;
    auto requirements=needs(QkmIbInputClient::Buttons|QkmIbInputClient::RelativeMove|QkmIbInputClient::VerticalWheel);
    for (const auto &button:mouseButtons) requirements.mouseButtons.insert(button.vk);
    const auto generation=client.requestRun();
    check(client.prepareRun(generation,requirements),"five-button preparation failed");
    for (const auto &button:mouseButtons) {
        check(client.sendEvent(mouse(button.down,0,0,button.data),generation),"button DOWN rejected");
        check(events.back().mi.dwFlags==button.down && events.back().mi.mouseData==button.data,"button identity changed");
    }
    check(client.sendEvent(mouse(MOUSEEVENTF_MOVE,2,-3),generation),"held-button movement rejected");
    check(client.sendEvent(mouse(MOUSEEVENTF_WHEEL,0,0,static_cast<DWORD>(-3*WHEEL_DELTA)),generation),"held-button wheel rejected");
    check(events.size()==9,"mouse primitive count changed");
    for (size_t index=6;index<9;++index)
        check(events[index].mi.dwFlags==MOUSEEVENTF_WHEEL && static_cast<LONG>(events[index].mi.mouseData)==-WHEEL_DELTA,"negative wheel unit changed");
    client.cancelRun();
    check(!client.sendEvent(mouse(MOUSEEVENTF_LEFTDOWN),generation),"late button DOWN accepted");
    check(!client.sendEvent(mouse(MOUSEEVENTF_WHEEL,0,0,WHEEL_DELTA),generation),"late wheel accepted");
    check(!client.sendEvent(mouse(MOUSEEVENTF_RIGHTUP),generation+1),"stale button UP accepted");
    check(events.size()==9 && client.closeAfterDrain(generation),"owned buttons did not drain");
    check(events.size()==14 && attempts.size()==1,"close switched backend or omitted button release");
    for (const auto &button:mouseButtons) {
        const auto count=std::count_if(events.begin()+9,events.end(),[&button](const INPUT &event) {
            return event.type==INPUT_MOUSE && event.mi.dwFlags==button.up && event.mi.mouseData==button.data;
        });
        check(count==1,"owned button release identity/count changed");
    }
}
void mouseFailureCase() {
    QkmIbInputClient partial;
    auto requirements=needs(QkmIbInputClient::Buttons|QkmIbInputClient::VerticalWheel);
    requirements.mouseButtons.insert(VK_XBUTTON1);
    const auto token=partial.requestRun();
    check(partial.prepareRun(token,requirements),"partial wheel preparation failed");
    check(partial.sendEvent(mouse(MOUSEEVENTF_XDOWN,0,0,XBUTTON1),token),"held X1 failed");
    failSendAt=3;
    check(!partial.sendEvent(mouse(MOUSEEVENTF_WHEEL,0,0,4*WHEEL_DELTA),token),"partial wheel failure hidden");
    check(events.size()==3 && attempts.size()==1,"wheel continued or changed backend after failure");
    check(partial.closeAfterDrain(token),"partial wheel did not release owned X1");
    check(events.size()==4 && events.back().mi.dwFlags==MOUSEEVENTF_XUP && events.back().mi.mouseData==XBUTTON1,"partial wheel release changed X identity");
    reset();
    QkmIbInputClient uncertain;
    const auto down=uncertain.requestRun();
    check(uncertain.prepareRun(down,needs(QkmIbInputClient::Buttons)),"unknown DOWN preparation failed");
    sendFails=true;
    check(!uncertain.sendEvent(mouse(MOUSEEVENTF_RIGHTDOWN),down),"failed DOWN hidden");
    sendFails=false;
    check(uncertain.closeAfterDrain(down),"unknown DOWN was not released");
    check(events.size()==2 && events.back().mi.dwFlags==MOUSEEVENTF_RIGHTUP,"unknown DOWN omitted same-button release");
    reset();
    QkmIbInputClient failedRelease;
    const auto release=failedRelease.requestRun();
    check(failedRelease.prepareRun(release,needs(QkmIbInputClient::Buttons)),"failed release preparation failed");
    check(failedRelease.sendEvent(mouse(MOUSEEVENTF_LEFTDOWN),release),"release test DOWN failed");
    sendFails=true;
    check(!failedRelease.closeAfterDrain(release) && failedRelease.statusSnapshot().releaseIncomplete,"mouse release failure hidden");
    check(failedRelease.busy() && !failedRelease.setPreferences({}),"failed mouse release unlocked preferences");
    const auto next=failedRelease.requestRun();
    check(!failedRelease.prepareRun(next,{}) && attempts.size()==1,"failed mouse release restarted backend");
}
void mouseBoundsCase() {
    const INPUT invalid[]={
        mouse(MOUSEEVENTF_HWHEEL,0,0,WHEEL_DELTA),
        mouse(MOUSEEVENTF_WHEEL,0,0,1),
        mouse(MOUSEEVENTF_XDOWN,0,0,0),
        mouse(MOUSEEVENTF_XUP,0,0,3),
        mouse(MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_RIGHTDOWN),
        mouse(MOUSEEVENTF_MOVE|MOUSEEVENTF_LEFTUP,1),
        mouse(MOUSEEVENTF_MOVE|MOUSEEVENTF_WHEEL,1,0,WHEEL_DELTA),
        mouse(MOUSEEVENTF_MOVE,32768),
        mouse(MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE,1,1)
    };
    for (const auto &event:invalid) {
        reset(); QkmIbInputClient client;
        const auto token=client.requestRun();
        check(client.prepareRun(token,needs(QkmIbInputClient::Buttons|QkmIbInputClient::RelativeMove|QkmIbInputClient::VerticalWheel)),"bounds preparation failed");
        check(!client.sendEvent(event,token) && events.empty(),"invalid mouse event reached native API");
        check(client.closeAfterDrain(token),"invalid mouse event damaged cleanup");
    }
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
    events.push_back(*inputs); if (duringSend) duringSend(); return sendFails || (failSendAt && events.size()==static_cast<size_t>(failSendAt)) ? 0 : 1;
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    static_assert(sizeof(INPUT)==(sizeof(void *)==8 ? 40 : 28));
    try {
#ifndef QKM_IBINPUT_VALIDATION_PROBE
        {
            check(QkmIbInputClient::capabilities(QStringLiteral("logitech_ghub"))==(QkmIbInputClient::RelativeMove|QkmIbInputClient::Keyboard|QkmIbInputClient::Buttons|QkmIbInputClient::VerticalWheel),"product capabilities differ from implemented primitives");
            QSet<quint16> allowed;
            for (WORD vk='A';vk<='Z';++vk) allowed.insert(vk);
            for (WORD vk='0';vk<='9';++vk) allowed.insert(vk);
            for (WORD vk=VK_F1;vk<=VK_F24;++vk) allowed.insert(vk);
            for (WORD vk=VK_NUMPAD0;vk<=VK_DIVIDE;++vk) if (vk!=VK_SEPARATOR) allowed.insert(vk);
            for (WORD vk=VK_LSHIFT;vk<=VK_RMENU;++vk) allowed.insert(vk);
            for (WORD vk=VK_PRIOR;vk<=VK_DOWN;++vk) allowed.insert(vk);
            for (WORD vk=VK_OEM_1;vk<=VK_OEM_3;++vk) allowed.insert(vk);
            for (WORD vk=VK_OEM_4;vk<=VK_OEM_7;++vk) allowed.insert(vk);
            for (WORD vk:{WORD(VK_BACK),WORD(VK_TAB),WORD(VK_RETURN),WORD(VK_ESCAPE),WORD(VK_SPACE),WORD(VK_CAPITAL),
                WORD(VK_INSERT),WORD(VK_DELETE),WORD(VK_NUMLOCK),WORD(VK_SCROLL),WORD(VK_SNAPSHOT),WORD(VK_PAUSE),
                WORD(VK_APPS),WORD(VK_LWIN),WORD(VK_RWIN)}) allowed.insert(vk);
            check(allowed.size()==115,"FakerInput-equivalent standard key set changed");
            for (quint16 vk=0;vk<256;++vk) check(QkmIbInputClient::isSupportedKey(vk)==allowed.contains(vk),"product key whitelist differs");
            for (WORD unsupported:{WORD(VK_SHIFT),WORD(VK_CONTROL),WORD(VK_MENU),WORD(VK_SEPARATOR),WORD(VK_CLEAR),
                WORD(VK_VOLUME_UP),WORD(VK_PROCESSKEY),WORD(VK_PACKET),WORD(VK_OEM_102)}) {
                QkmIbInputClient blocked; auto requirements=needs(QkmIbInputClient::Keyboard); requirements.keys.insert(unsupported);
                const auto token=blocked.requestRun();
                check(!blocked.prepareRun(token,requirements) && attempts.empty(),"unsupported key reached driver initialization");
            }
            QkmIbInputClient client;
            auto keyboard=needs(QkmIbInputClient::Keyboard); keyboard.keys=allowed;
            const auto token=client.requestRun();
            check(client.prepareRun(token,keyboard),"standard keyboard preparation failed");
            for (quint16 vk:allowed) check(client.sendEvent(key(vk),token) && client.sendEvent(key(vk,true),token),"standard key did not round-trip");
            check(events.size()==230 && client.closeAfterDrain(token),"standard keyboard did not release");
            reset();
            const auto movement=client.requestRun();
            check(client.prepareRun(movement,needs(QkmIbInputClient::RelativeMove)),"product movement admission failed");
            INPUT move{}; move.type=INPUT_MOUSE; move.mi.dwFlags=MOUSEEVENTF_MOVE; move.mi.dx=1;
            check(client.sendEvent(move,movement) && events.size()==1,"admitted product movement rejected");
            check(!client.sendEvent(key('A'),movement) && events.size()==1,"unadmitted keyboard reached API");
            check(client.closeAfterDrain(movement),"product capability close failed");
            QSet<quint16> supportedButtons;
            for (const auto &button:mouseButtons) supportedButtons.insert(button.vk);
            for (quint16 vk=0;vk<256;++vk) check(QkmIbInputClient::isSupportedButton(vk)==supportedButtons.contains(vk),"mouse button set differs");
            reset(); razerCase(); reset(); admissionCase(); reset(); restartCase(); reset(); stopActionCase(); reset(); failureCase(); reset(); reportCase(); reset(); modifiersCase();
            reset(); buttonsCase(); reset(); mouseFailureCase(); reset(); mouseBoundsCase();
            reset(); concurrentCase();
            std::cout<<"PASS simulated production gate: 115 keys, five buttons, wheel units, rejected representations, movement, cancellation, failures, slots and concurrent cleanup; no real input.\n"; return 0;
        }
#endif
        const std::pair<const char *,void(*)()> cases[]={{"settings",settingsCase},{"razer",razerCase},{"admission",admissionCase},{"restart",restartCase},{"stop_action",stopActionCase},{"fallback",fallbackCase},{"failure",failureCase},{"report",reportCase},{"modifiers",modifiersCase},{"mouse",mouseCase},{"capability",capabilityCase},{"buttons",buttonsCase},{"mouse_failure",mouseFailureCase},{"mouse_bounds",mouseBoundsCase},{"concurrent",concurrentCase}};
        for (const auto &test:cases) { reset(); test.second(); std::cout<<"PASS simulated client "<<test.first<<'\n'; }
        std::cout<<"No driver initialization, hooks or actual input; formal C API substituted.\n"; return 0;
    } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 2; }
}
