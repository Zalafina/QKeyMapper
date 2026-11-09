#include "qkm_ibinput_client.h"
#include "automation/qkm_demo_mode.h"
#include <IbInputSimulator/InputSimulator.hpp>
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace QkmIbInputSystem {
std::mutex mutex;
POINT cursor{};
double exactX=0,exactY=0,gainX=1,gainY=1;
int left=0,top=0,width=1920,height=1080,delay=0,reads=0,failReadAt=0;
quint64 clock=0;
bool nonlinear=false,frozen=false;
struct Pending { quint64 due; LONG x,y; };
std::vector<Pending> pending;
std::function<void()> onPause;
std::thread::id advancingThread;
void reset() {
    std::lock_guard<std::mutex> lock(mutex);
    cursor={}; exactX=exactY=0; gainX=gainY=1;
    left=top=delay=reads=failReadAt=0; width=1920; height=1080; clock=0;
    nonlinear=frozen=false; pending.clear(); onPause={}; advancingThread={};
}
void move(LONG x,LONG y) {
    std::lock_guard<std::mutex> lock(mutex);
    pending.push_back({clock+static_cast<quint64>(delay),x,y});
}
BOOL cursorPosition(LPPOINT point) {
    std::lock_guard<std::mutex> lock(mutex);
    if (++reads==failReadAt) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    for (auto it=pending.begin();it!=pending.end();) {
        if (it->due>clock) { ++it; continue; }
        const auto gain=[&](LONG value,double linear) { return nonlinear ? (std::abs(value)>10 ? 2.6 : 0.55) : linear; };
        if (!frozen) {
            exactX=std::clamp(exactX+it->x*gain(it->x,gainX),double(left),double(left+width-1));
            exactY=std::clamp(exactY+it->y*gain(it->y,gainY),double(top),double(top+height-1));
            cursor={static_cast<LONG>(std::llround(exactX)),static_cast<LONG>(std::llround(exactY))};
        }
        it=pending.erase(it);
    }
    *point=cursor; return TRUE;
}
int metric(int index) {
    if (index==SM_XVIRTUALSCREEN) return left;
    if (index==SM_YVIRTUALSCREEN) return top;
    if (index==SM_CXSCREEN || index==SM_CXVIRTUALSCREEN) return width;
    if (index==SM_CYSCREEN || index==SM_CYVIRTUALSCREEN) return height;
    return 0;
}
quint64 ticks() { std::lock_guard<std::mutex> lock(mutex); return clock; }
void pause() {
    std::function<void()> callback;
    { std::lock_guard<std::mutex> lock(mutex); if (advancingThread==std::thread::id{} || advancingThread==std::this_thread::get_id()) ++clock; callback=std::move(onPause); onPause={}; }
    if (callback) callback();
    std::this_thread::yield();
}
}

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
void reset() { QkmIbInputSystem::reset(); attempts.clear(); events.clear(); initFails=false; sendFails=false; failSendAt=0; duringInit={}; duringSend={}; }
void preferencesRevisionCase() {
    QkmIbInputClient client;
    const auto original=client.preferences();
    const auto revision=client.preferencesRevision();
    check(client.setPreferences(original) && client.preferencesRevision()==revision,"same preferences changed semantic revision");
    check(!client.setPreferences({QStringLiteral("invalid"),QStringLiteral("block")})
        && client.preferencesRevision()==revision,"invalid preferences changed semantic revision");
    check(client.setPreferences({QStringLiteral("razer"),QStringLiteral("block")})
        && client.preferencesRevision()==revision+1,"real preferences change did not advance revision");
    (void)client.statusSnapshot();
    check(client.preferencesRevision()==revision+1,"observation changed semantic revision");
    check(client.setPreferences(original) && client.preferencesRevision()==revision+2,"restoring preferences revived a stale revision");
}
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
    for (bool failInit:{false,true}) for (bool allowFallback:{false,true}) {
        reset();
        QkmIbInputClient superseded;
        QkmIbInputClient::Preferences preferences; preferences.backend=QStringLiteral("logitech_ghub");
        preferences.unavailablePolicy=allowFallback ? QStringLiteral("sendinput_before_start") : QStringLiteral("block");
        check(superseded.setPreferences(preferences),"Superseded fixture preferences rejected");
        quint64 latest=0;
        initFails=failInit;
        duringInit=[&] { latest=superseded.requestRun(); };
        const auto stale=superseded.requestRun();
        check(!superseded.prepareRun(stale,required) && !superseded.statusSnapshot().connected
            && !superseded.statusSnapshot().fallback,"Superseded initialization stayed connected or fell back");
        duringInit={}; initFails=false;
        check(latest>stale && superseded.acceptingInput(),"Superseded initialization cancelled the newer intent");
        check(superseded.prepareRun(latest,required) && superseded.closeAfterDrain(latest),
            "New intent could not connect after superseded initialization");
    }
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
    client.cancelRun(token);
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
void supersededSendCase() {
    for (bool mouseInput:{false,true}) {
        reset(); QkmIbInputClient client;
        const auto required=needs(mouseInput ? QkmIbInputClient::Buttons : QkmIbInputClient::Keyboard);
        const auto old=client.requestRun(); check(client.prepareRun(old,required),"Superseded send fixture failed");
        quint64 latest=0;
        duringSend=[&] { latest=client.requestRun(); };
        sendFails=true;
        check(!client.sendEvent(mouseInput ? mouse(MOUSEEVENTF_LEFTDOWN) : key('F'),old),"Old send failure was hidden");
        duringSend={}; sendFails=false;
        check(latest>old && client.acceptingInput(),"Old send failure cancelled the newer intent");
        check(client.closeAfterDrain(old) && client.acceptingInput(),"Old failed send cleanup cancelled the newer intent");
        check(client.prepareRun(latest,required) && client.closeAfterDrain(latest),"New intent could not start after failed send");
    }
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
    QkmIbInputClient::Preferences manual; manual.backend=QStringLiteral("logitech_ghub");
    check(blocked.setPreferences(manual),"manual wheel fixture rejected");
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
void horizontalCase() {
    QkmIbInputClient client;
    const auto requirements=needs(QkmIbInputClient::Buttons|QkmIbInputClient::HorizontalWheel);
    const auto generation=client.requestRun();
    check(client.prepareRun(generation,requirements)
        && attempts==std::vector<Send::SendType>{Send::SendType::Razer},"Automatic horizontal wheel did not select Razer only");
    check(client.sendEvent(mouse(MOUSEEVENTF_LEFTDOWN),generation),"Horizontal fixture button DOWN failed");
    for (LONG delta:{LONG(-2*WHEEL_DELTA),LONG(2*WHEEL_DELTA)}) {
        const auto before=events.size();
        check(client.sendEvent(mouse(MOUSEEVENTF_HWHEEL,0,0,static_cast<DWORD>(delta)),generation),"Horizontal wheel rejected");
        check(events.size()==before+2,"Horizontal wheel unit count differs");
        for (size_t index=before;index<events.size();++index)
            check(events[index].mi.dwFlags==MOUSEEVENTF_HWHEEL
                && static_cast<LONG>(events[index].mi.mouseData)==(delta<0 ? -WHEEL_DELTA : WHEEL_DELTA),
                "Horizontal wheel axis/sign/unit changed");
    }
    client.cancelRun();
    check(!client.sendEvent(mouse(MOUSEEVENTF_HWHEEL,0,0,WHEEL_DELTA),generation),"Stopped horizontal wheel sent input");
    check(client.closeAfterDrain(generation) && events.size()==6 && events.back().mi.dwFlags==MOUSEEVENTF_LEFTUP,
        "Held button was not released after horizontal wheel");
    reset();
    QkmIbInputClient failed;
    const auto token=failed.requestRun();
    check(failed.prepareRun(token,requirements) && failed.sendEvent(mouse(MOUSEEVENTF_LEFTDOWN),token),"Horizontal failure fixture failed");
    failSendAt=3;
    check(!failed.sendEvent(mouse(MOUSEEVENTF_HWHEEL,0,0,3*WHEEL_DELTA),token) && events.size()==3
        && attempts.size()==1,"Horizontal partial failure continued or switched backend");
    check(failed.closeAfterDrain(token) && events.size()==4 && events.back().mi.dwFlags==MOUSEEVENTF_LEFTUP,
        "Horizontal failure lost held-button cleanup");
    const INPUT invalid[]={mouse(MOUSEEVENTF_HWHEEL,0,0,1),
        mouse(MOUSEEVENTF_WHEEL|MOUSEEVENTF_HWHEEL,0,0,WHEEL_DELTA),
        mouse(MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_HWHEEL,0,0,WHEEL_DELTA),
        mouse(MOUSEEVENTF_MOVE|MOUSEEVENTF_HWHEEL,1,0,WHEEL_DELTA)};
    for (const auto &event:invalid) {
        reset(); QkmIbInputClient bounded;
        const auto run=bounded.requestRun();
        check(bounded.prepareRun(run,needs(QkmIbInputClient::Buttons|QkmIbInputClient::RelativeMove
            |QkmIbInputClient::HorizontalWheel|QkmIbInputClient::VerticalWheel)),"Horizontal bounds fixture failed");
        check(!bounded.sendEvent(event,run) && events.empty() && bounded.closeAfterDrain(run),
            "Ambiguous horizontal event reached the native API");
    }
}
void absoluteCase() {
    QkmIbInputClient client;
    const auto required=needs(QkmIbInputClient::AbsoluteMove|QkmIbInputClient::Buttons);
    QkmIbInputClient::Preferences native; native.backend=QStringLiteral("razer");
    check(client.setPreferences(native),"Native absolute fixture failed");
    const auto generation=client.requestRun();
    check(client.prepareRun(generation,required)
        && attempts==std::vector<Send::SendType>{Send::SendType::Razer},"Absolute input did not select Razer only");
    const DWORD absolute=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;
    for (LONG coordinate:{LONG(0),LONG(32768),LONG(65535)}) {
        check(client.sendEvent(mouse(absolute,coordinate,coordinate),generation),"Normalized absolute range rejected");
        check(events.back().mi.dx==coordinate && events.back().mi.dy==coordinate
            && events.back().mi.dwFlags==absolute,"Absolute coordinates/flags changed");
    }
    for (const auto &button:mouseButtons) {
        check(client.sendEvent(mouse(absolute|button.down,40000,50000,button.data),generation),"Absolute button DOWN rejected");
        check(events.back().mi.dwFlags==(absolute|button.down) && events.back().mi.mouseData==button.data,
            "Point-click report lost button identity");
        check(client.sendEvent(mouse(absolute|button.up,1000,2000,button.data),generation),"Absolute button UP rejected");
        check(events.back().mi.dwFlags==(absolute|button.up) && events.back().mi.dx==1000 && events.back().mi.dy==2000,
            "Active point-click UP lost its coordinates");
    }
    check(client.sendEvent(mouse(absolute|MOUSEEVENTF_LEFTDOWN,1000,2000),generation),"Late-UP fixture DOWN failed");
    client.cancelRun();
    check(!client.sendEvent(mouse(absolute,3000,4000),generation),"Stopped absolute movement reached API");
    check(client.sendEvent(mouse(absolute|MOUSEEVENTF_LEFTUP,3000,4000),generation)
        && events.back().mi.dwFlags==MOUSEEVENTF_LEFTUP && events.back().mi.dx==0 && events.back().mi.dy==0,
        "Point-click cleanup moved the cursor");
    check(client.releaseOwnedInputs(generation) && client.beginStopAction(generation),"Absolute stop-action fixture failed");
    check(client.sendEvent(mouse(absolute|MOUSEEVENTF_LEFTDOWN,1000,2000),generation,true)
        && client.sendEvent(mouse(absolute|MOUSEEVENTF_LEFTUP,3000,4000),generation,true)
        && events.back().mi.dwFlags==(absolute|MOUSEEVENTF_LEFTUP) && events.back().mi.dx==3000,
        "Admitted stop-action point-click lost its position");
    check(client.closeAfterDrain(generation),"Absolute close failed");
    reset();
    QkmIbInputClient failed;
    check(failed.setPreferences(native),"Native failure fixture preference failed");
    const auto token=failed.requestRun(); check(failed.prepareRun(token,required),"Absolute failure fixture failed");
    failSendAt=1;
    check(!failed.sendEvent(mouse(absolute|MOUSEEVENTF_LEFTDOWN,1000,2000),token),"Unknown point-click DOWN was accepted");
    check(failed.closeAfterDrain(token) && events.size()==2 && events.back().mi.dwFlags==MOUSEEVENTF_LEFTUP,
        "Unknown point-click DOWN did not release on the same backend");
    const INPUT invalid[]={mouse(absolute,-1,0),mouse(absolute,0,65536),mouse(MOUSEEVENTF_ABSOLUTE,0,0),
        mouse(MOUSEEVENTF_MOVE|MOUSEEVENTF_VIRTUALDESK,0,0),mouse(absolute|MOUSEEVENTF_WHEEL,0,0,WHEEL_DELTA),
        mouse(absolute|MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_RIGHTDOWN,0,0)};
    for (const auto &event:invalid) {
        reset(); QkmIbInputClient bounded;
        const auto run=bounded.requestRun();
        check(bounded.prepareRun(run,needs(QkmIbInputClient::AbsoluteMove|QkmIbInputClient::RelativeMove
            |QkmIbInputClient::Buttons|QkmIbInputClient::VerticalWheel)),"Absolute bounds fixture failed");
        check(!bounded.sendEvent(event,run) && events.empty() && bounded.closeAfterDrain(run),
            "Invalid absolute representation reached native API");
    }
    reset(); QkmIbInputClient ghub;
    QkmIbInputClient::Preferences manual; manual.backend=QStringLiteral("logitech_ghub");
    check(ghub.setPreferences(manual),"Manual absolute fixture failed");
    const auto run=ghub.requestRun();
    check(ghub.prepareRun(run,required) && ghub.closeAfterDrain(run),"G HUB absolute candidate was not admitted");
}
void ghubPositionCase() {
    using namespace QkmIbInputSystem;
    const DWORD absolute=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;
    const auto required=needs(QkmIbInputClient::AbsoluteMove|QkmIbInputClient::Buttons|QkmIbInputClient::Keyboard);
    for (double scale:{1.0/32.0,0.125,0.5,1.0,2.6,3.5}) for (LONG destination:{LONG(5),LONG(83),LONG(650)}) {
        ::reset(); gainX=scale; gainY=scale*0.8;
        QkmIbInputClient client; const auto run=client.requestRun();
        check(client.prepareRun(run,required),"G HUB position fixture failed");
        const auto down=mouse(absolute|MOUSEEVENTF_LEFTDOWN);
        const bool reached=client.sendAbsoluteEvent(down,{destination,destination/2},run);
        if (!reached) std::cerr<<"scale="<<scale<<" target="<<destination<<" actual="<<cursor.x<<","<<cursor.y<<" sends="<<events.size()<<" ms="<<QkmIbInputSystem::clock<<" detail="<<client.statusSnapshot().detail.toStdString()<<'\n';
        check(reached,"Linear feedback did not converge");
        check(std::abs(cursor.x-destination)<=2 && std::abs(cursor.y-destination/2)<=2,"Position exceeded tolerance");
        check(!events.empty() && events.back().mi.dwFlags==MOUSEEVENTF_LEFTDOWN,"Point click was not separated from movement");
        for (size_t i=0;i+1<events.size();++i) check(events[i].mi.dwFlags==MOUSEEVENTF_MOVE,"Point button was pressed before movement completed");
        check(client.closeAfterDrain(run),"Position fixture did not release");
    }
    for (int lag:{0,1,3}) {
        ::reset(); nonlinear=true; delay=lag; left=-1200; top=-700;
        exactX=cursor.x=-1000; exactY=cursor.y=-600;
        QkmIbInputClient client; const auto run=client.requestRun(); check(client.prepareRun(run,required),"Delayed fixture failed");
        check(client.sendAbsoluteEvent(mouse(absolute),{-450,-280},run),"Nonlinear/delayed position did not converge");
        check(std::abs(cursor.x+450)<=2 && std::abs(cursor.y+280)<=2 && client.closeAfterDrain(run),"Negative-origin feedback failed");
    }
    ::reset();
    QkmIbInputClient normalized; const auto normal=normalized.requestRun();
    check(normalized.prepareRun(normal,needs(QkmIbInputClient::AbsoluteMove)),"Normalized feedback fixture failed");
    check(normalized.sendEvent(mouse(absolute,65535,65535),normal)
        && std::abs(cursor.x-(width-1))<=2 && std::abs(cursor.y-(height-1))<=2,"Normalized event bypassed feedback");
    check(normalized.closeAfterDrain(normal),"Normalized feedback close failed");
    for (int failure:{0,1,3,4}) {
        ::reset(); frozen=failure==0; failReadAt=failure==4 ? 0 : failure;
        if (failure==4) gainX=gainY=8;
        QkmIbInputClient client; const auto run=client.requestRun(); check(client.prepareRun(run,required),"Failure fixture failed");
        check(!client.sendAbsoluteEvent(mouse(absolute|MOUSEEVENTF_LEFTDOWN),failure==4 ? POINT{5,2} : POINT{400,250},run)
            && client.statusSnapshot().failed,"Failed positioning returned success");
        if (failure==1 || failure==3) check(client.statusSnapshot().detail.contains(QStringLiteral("5")),"Immediate cursor error was overwritten");
        for (const auto &event:events) check(event.mi.dwFlags==MOUSEEVENTF_MOVE,"Failed positioning pressed a button");
        check(events.size()<=32 && client.closeAfterDrain(run),"Position limit or cleanup failed");
    }
    ::reset();
    QkmIbInputClient cancelled; const auto run=cancelled.requestRun(); check(cancelled.prepareRun(run,required),"Cancel fixture failed");
    check(cancelled.sendEvent(mouse(MOUSEEVENTF_LEFTDOWN),run),"Cancel fixture button failed");
    onPause=[&] { cancelled.cancelRun(run); };
    check(!cancelled.sendAbsoluteEvent(mouse(absolute|MOUSEEVENTF_LEFTUP),{400,250},run)
        && !cancelled.statusSnapshot().failed,"Normal position cancellation became a fault");
    check(events.back().mi.dwFlags==MOUSEEVENTF_LEFTUP && cancelled.closeAfterDrain(run),"Cancelled point UP did not release without movement");
    ::reset();
    QkmIbInputClient stopped; const auto stop=stopped.requestRun(); check(stopped.prepareRun(stop,required),"Stop position fixture failed");
    stopped.cancelRun(stop);
    check(stopped.releaseOwnedInputs(stop) && stopped.beginStopAction(stop),"Stop position scope failed");
    check(stopped.sendAbsoluteEvent(mouse(absolute|MOUSEEVENTF_LEFTDOWN),{120,80},stop,true),"Admitted stop position failed");
    failSendAt=static_cast<int>(events.size())+1;
    check(!stopped.sendEvent(key('F'),stop,true) && !stopped.stopActionAllowed(stop),"Failed stop send retained stop scope");
    failSendAt=0; check(stopped.closeAfterDrain(stop),"Stop position did not release");
}
void ghubPositionBoundaryCase() {
    using namespace QkmIbInputSystem;
    const DWORD absolute=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;
    const auto required=needs(QkmIbInputClient::AbsoluteMove|QkmIbInputClient::Buttons|QkmIbInputClient::Keyboard);
    for (bool cancel:{false,true}) {
        ::reset(); QkmIbInputClient client; const auto run=client.requestRun();
        check(client.prepareRun(run,required) && client.sendEvent(mouse(MOUSEEVENTF_LEFTDOWN),run),"Concurrent position fixture failed");
        std::mutex gate; std::condition_variable condition; bool entered=false,release=false,expired=false;
        onPause=[&] {
            std::unique_lock<std::mutex> lock(gate); entered=true; condition.notify_all();
            if (!condition.wait_for(lock,std::chrono::seconds(3),[&] { return release; })) expired=true;
        };
        bool first=false,second=false;
        std::thread owner([&] {
            { std::lock_guard<std::mutex> lock(mutex); advancingThread=std::this_thread::get_id(); }
            first=client.sendAbsoluteEvent(mouse(absolute),{400,250},run);
            { std::lock_guard<std::mutex> lock(mutex); advancingThread={}; }
        });
        bool observed=false;
        { std::unique_lock<std::mutex> lock(gate); observed=condition.wait_for(lock,std::chrono::seconds(3),[&] { return entered; }); }
        bool waiting=false;
        onPause=[&] { std::lock_guard<std::mutex> lock(gate); waiting=true; condition.notify_all(); };
        std::thread waiter([&] { second=client.sendAbsoluteEvent(mouse(absolute|MOUSEEVENTF_RIGHTDOWN),{600,300},run); });
        bool waiterObserved=false;
        { std::unique_lock<std::mutex> lock(gate); waiterObserved=condition.wait_for(lock,std::chrono::seconds(3),[&] { return waiting; }); }
        const bool keyboard=client.sendEvent(key('F'),run);
        const bool released=client.sendEvent(mouse(MOUSEEVENTF_LEFTUP),run);
        if (cancel) client.cancelRun(run);
        { std::lock_guard<std::mutex> lock(gate); release=true; } condition.notify_all();
        owner.join(); waiter.join();
        check(observed && waiterObserved && !expired && keyboard && released,"Position wait blocked keyboard or pure UP");
        if (cancel) check(!first && !second && !client.statusSnapshot().failed,"Cancelled position transaction became a fault");
        else check(first && second && std::abs(cursor.x-600)<=2 && std::abs(cursor.y-300)<=2,"Concurrent positions did not serialize");
        check(client.closeAfterDrain(run),"Concurrent position close failed");
    }
    for (bool releaseFails:{false,true}) {
        ::reset(); QkmIbInputClient client; const auto run=client.requestRun();
        check(client.prepareRun(run,required) && client.sendEvent(mouse(MOUSEEVENTF_LEFTDOWN),run),"Query failure UP fixture failed");
        failReadAt=1; sendFails=releaseFails;
        check(!client.sendAbsoluteEvent(mouse(absolute|MOUSEEVENTF_LEFTUP),{400,250},run)
            && events.back().mi.dwFlags==MOUSEEVENTF_LEFTUP,"Position read failure omitted pure UP");
        check(client.closeAfterDrain(run)!=releaseFails,"Failed point-UP cleanup result was hidden");
        if (releaseFails) {
            check(client.statusSnapshot().releaseIncomplete && client.busy() && !client.setPreferences({}),"Failed point UP removed the release barrier");
        }
    }
    ::reset(); QkmIbInputClient draining; const auto drainingRun=draining.requestRun();
    check(draining.prepareRun(drainingRun,required),"Drain-only fixture failed");
    bool drained=false;
    onPause=[&] { drained=draining.releaseOwnedInputs(drainingRun); };
    check(!draining.sendAbsoluteEvent(mouse(absolute|MOUSEEVENTF_LEFTDOWN),{400,250},drainingRun)
        && drained && !draining.statusSnapshot().failed,"Stopping state without a new intent admitted a click");
    for (const auto &event:events) check(event.mi.dwFlags==MOUSEEVENTF_MOVE,"Draining position pressed a button");
    check(draining.closeAfterDrain(drainingRun),"Drain-only close failed");
    ::reset(); QkmIbInputClient timed; const auto run=timed.requestRun(); check(timed.prepareRun(run,required),"Budget fixture failed");
    onPause=[] { std::lock_guard<std::mutex> lock(mutex); QkmIbInputSystem::clock+=250; };
    check(!timed.sendAbsoluteEvent(mouse(absolute|MOUSEEVENTF_LEFTDOWN),{400,250},run)
        && events.size()==1 && events.front().mi.dwFlags==MOUSEEVENTF_MOVE,"Expired budget clicked or retried");
    check(timed.closeAfterDrain(run),"Budget fixture cleanup failed");
    ::reset(); QkmIbInputClient onlyAbsolute; const auto admitted=onlyAbsolute.requestRun();
    check(onlyAbsolute.prepareRun(admitted,needs(QkmIbInputClient::AbsoluteMove)),"Absolute-only fixture failed");
    check(onlyAbsolute.sendAbsoluteEvent(mouse(absolute),{100,80},admitted),"Internal relative step was not admitted");
    const auto count=events.size();
    check(!onlyAbsolute.sendEvent(mouse(MOUSEEVENTF_MOVE,1),admitted) && events.size()==count,"Internal step widened ordinary relative admission");
    check(onlyAbsolute.closeAfterDrain(admitted),"Absolute-only fixture cleanup failed");
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
    events.push_back(*inputs); if (duringSend) duringSend();
    const bool success=!sendFails && !(failSendAt && events.size()==static_cast<size_t>(failSendAt));
    if (success && inputs->type==INPUT_MOUSE && (inputs->mi.dwFlags & MOUSEEVENTF_MOVE)
        && !(inputs->mi.dwFlags & MOUSEEVENTF_ABSOLUTE)) QkmIbInputSystem::move(inputs->mi.dx,inputs->mi.dy);
    return success ? 1 : 0;
}
int demoGuardCase(int argc,char **argv) {
    const HDESK previous=GetThreadDesktop(GetCurrentThreadId());
    const QString name=QStringLiteral("QkmIbClientDemo_%1").arg(GetCurrentProcessId());
    const HDESK desktop=CreateDesktopW(reinterpret_cast<LPCWSTR>(name.utf16()),nullptr,nullptr,0,
        DESKTOP_CREATEWINDOW|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS,nullptr);
    if (!desktop || !SetThreadDesktop(desktop)) { if (desktop) CloseDesktop(desktop); return 2; }
    int result=0;
    try {
        check(QkmAutomation::initializeStartupPurpose(QkmAutomation::StartupPurpose::ConfigurationOnly),"private demo policy initialization failed");
        QCoreApplication app(argc,argv);
        QkmIbInputClient client;
        for (int scenario=0;scenario<6;++scenario) {
            reset();
            if (scenario==0) check(!client.prepareRun(1,needs(QkmIbInputClient::Keyboard)),"demo initialized a backend");
            if (scenario==1) check(!client.sendEvent(key('A'),1),"demo admitted keyboard");
            if (scenario==2) check(!client.sendAbsoluteEvent(mouse(MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE),POINT{10,10},1),"demo admitted pixel absolute");
            if (scenario==3) check(!client.sendEvent(mouse(MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE,32767,32767),1),"demo admitted normalized absolute");
            if (scenario==4) check(!client.sendEvent(mouse(MOUSEEVENTF_MOVE,1,1),1),"demo admitted relative stepping");
            if (scenario==5) check(!client.checkConnection() && !client.releaseOwnedInputs(1) && !client.closeAfterDrain(1),"demo admitted check/release");
            check(attempts.empty() && events.empty(),"demo reached a substituted native input API");
            std::cout<<"PASS private demo client scenario="<<scenario<<" IbSendInit="<<attempts.size()<<" IbSendInput="<<events.size()<<'\n';
        }
        check(client.requestRun()==0,"demo reserved an input run");
    } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; result=2; }
    if (!SetThreadDesktop(previous)) result=2;
    if (!CloseDesktop(desktop)) result=2;
    return result;
}

int main(int argc,char **argv) {
    if (argc==2 && QString::fromLocal8Bit(argv[1])==QStringLiteral("--demo-guards")) return demoGuardCase(argc,argv);
    QCoreApplication app(argc,argv);
    static_assert(sizeof(INPUT)==(sizeof(void *)==8 ? 40 : 28));
    try {
        preferencesRevisionCase();
#ifndef QKM_IBINPUT_VALIDATION_PROBE
        {
            check(QkmIbInputClient::capabilities(QStringLiteral("logitech_ghub"))==(QkmIbInputClient::RelativeMove|QkmIbInputClient::Keyboard|QkmIbInputClient::Buttons|QkmIbInputClient::VerticalWheel|QkmIbInputClient::AbsoluteMove),"product capabilities differ from implemented primitives");
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
            reset(); razerCase(); reset(); admissionCase(); reset(); restartCase(); reset(); stopActionCase(); reset(); failureCase(); reset(); supersededSendCase(); reset(); reportCase(); reset(); modifiersCase();
            reset(); absoluteCase(); reset(); ghubPositionCase(); reset(); ghubPositionBoundaryCase(); reset(); horizontalCase(); reset(); buttonsCase(); reset(); mouseFailureCase(); reset(); mouseBoundsCase();
            reset(); concurrentCase();
            std::cout<<"PASS simulated production gate: 115 keys, five buttons, wheel units, rejected representations, movement, cancellation, failures, slots and concurrent cleanup; no real input.\n"; return 0;
        }
#endif
        const std::pair<const char *,void(*)()> cases[]={{"settings",settingsCase},{"razer",razerCase},{"admission",admissionCase},{"restart",restartCase},{"stop_action",stopActionCase},{"fallback",fallbackCase},{"failure",failureCase},{"superseded_send",supersededSendCase},{"report",reportCase},{"modifiers",modifiersCase},{"mouse",mouseCase},{"capability",capabilityCase},{"absolute",absoluteCase},{"ghub_position",ghubPositionCase},{"ghub_position_boundaries",ghubPositionBoundaryCase},{"horizontal",horizontalCase},{"buttons",buttonsCase},{"mouse_failure",mouseFailureCase},{"mouse_bounds",mouseBoundsCase},{"concurrent",concurrentCase}};
        for (const auto &test:cases) { reset(); test.second(); std::cout<<"PASS simulated client "<<test.first<<'\n'; }
        std::cout<<"No driver initialization, hooks or actual input; formal C API substituted.\n"; return 0;
    } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 2; }
}
