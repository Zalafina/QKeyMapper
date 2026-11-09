#include "qkm_ibinput_client.h"
#include "automation/qkm_demo_mode.h"
#include <QCoreApplication>
#include <QMutexLocker>
#include <QSettings>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <utility>
#ifdef IBINPUT_SUPPORT
#include <IbInputSimulator/InputSimulator.hpp>
#endif
#ifdef QKM_IBINPUT_OUTPUT_PROBE
#include "tests/ibinput/native_output_probe.h"
#endif

namespace QkmIbInputSystem {
#if defined(QKM_IBINPUT_CLIENT_TEST) || defined(QKM_IBINPUT_OUTPUT_PROBE)
BOOL cursorPosition(LPPOINT point);
int metric(int index);
quint64 ticks();
void pause();
#else
BOOL cursorPosition(LPPOINT point) { return GetCursorPos(point); }
int metric(int index) { return GetSystemMetrics(index); }
quint64 ticks() { return GetTickCount64(); }
void pause() { QThread::msleep(1); }
#endif
}

namespace {
constexpr char backendKey[]="IbInputPreferredBackend";
constexpr char policyKey[]="IbInputUnavailablePolicy";
bool isRelease(const INPUT &input) {
    return (input.type==INPUT_KEYBOARD && (input.ki.dwFlags & KEYEVENTF_KEYUP))
        || (input.type==INPUT_MOUSE && (input.mi.dwFlags & (MOUSEEVENTF_LEFTUP|MOUSEEVENTF_RIGHTUP|MOUSEEVENTF_MIDDLEUP|MOUSEEVENTF_XUP)));
}
quint16 buttonId(const MOUSEINPUT &mouse) {
    if (mouse.dwFlags & (MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_LEFTUP)) return VK_LBUTTON;
    if (mouse.dwFlags & (MOUSEEVENTF_RIGHTDOWN|MOUSEEVENTF_RIGHTUP)) return VK_RBUTTON;
    if (mouse.dwFlags & (MOUSEEVENTF_MIDDLEDOWN|MOUSEEVENTF_MIDDLEUP)) return VK_MBUTTON;
    if (mouse.dwFlags & (MOUSEEVENTF_XDOWN|MOUSEEVENTF_XUP)) return mouse.mouseData==XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2;
    return 0;
}
QString cursorPositionError() {
    const DWORD error=GetLastError();
    return QCoreApplication::translate("QkmIbInputClient","IbInput could not read the cursor position (Windows error %1). Mapping must stop.").arg(error);
}
INPUT releaseButton(quint16 button) {
    INPUT input{}; input.type=INPUT_MOUSE;
    if (button==VK_LBUTTON) input.mi.dwFlags=MOUSEEVENTF_LEFTUP;
    else if (button==VK_RBUTTON) input.mi.dwFlags=MOUSEEVENTF_RIGHTUP;
    else if (button==VK_MBUTTON) input.mi.dwFlags=MOUSEEVENTF_MIDDLEUP;
    else { input.mi.dwFlags=MOUSEEVENTF_XUP; input.mi.mouseData=button==VK_XBUTTON1 ? XBUTTON1 : XBUTTON2; }
    return input;
}
}

QkmIbInputClient &QkmIbInputClient::instance() { static QkmIbInputClient client; return client; }
QkmIbInputClient::Preferences QkmIbInputClient::readPreferences(QSettings &settings, QString *diagnostic) {
    if (diagnostic) diagnostic->clear();
    Preferences result;
    const QString backend=settings.value(backendKey,result.backend).toString();
    const QString policy=settings.value(policyKey,result.unavailablePolicy).toString();
    if (candidates(backend).isEmpty() || (policy!=QStringLiteral("block") && policy!=QStringLiteral("sendinput_before_start"))) {
        if (diagnostic) *diagnostic=QCoreApplication::translate("QkmIbInputClient","Invalid IbInput preferences; defaults were used.");
    }
    if (!candidates(backend).isEmpty()) result.backend=backend;
    if (policy==QStringLiteral("block") || policy==QStringLiteral("sendinput_before_start")) result.unavailablePolicy=policy;
    return result;
}
void QkmIbInputClient::writePreferences(QSettings &settings,const Preferences &preferences) {
    settings.setValue(backendKey,preferences.backend); settings.setValue(policyKey,preferences.unavailablePolicy);
}
QStringList QkmIbInputClient::candidates(const QString &preference) {
    const QStringList order{QStringLiteral("logitech_ghub"),QStringLiteral("logitech_legacy"),QStringLiteral("razer")};
    if (preference==QStringLiteral("auto")) return order;
    return order.contains(preference) ? QStringList{preference} : QStringList{};
}
bool QkmIbInputClient::isSupportedKey(quint16 key) {
    const bool basic=(key>='0' && key<='9') || (key>='A' && key<='Z') || (key>=VK_F1 && key<=VK_F24);
    return basic || (key>=VK_NUMPAD0 && key<=VK_DIVIDE && key!=VK_SEPARATOR)
        || (key>=VK_LSHIFT && key<=VK_RMENU) || key==VK_LWIN || key==VK_RWIN
        || key==VK_BACK || key==VK_TAB || key==VK_RETURN || key==VK_ESCAPE || key==VK_SPACE
        || (key>=VK_PRIOR && key<=VK_DOWN) || key==VK_INSERT || key==VK_DELETE || key==VK_CAPITAL
        || key==VK_NUMLOCK || key==VK_SCROLL || key==VK_SNAPSHOT || key==VK_PAUSE || key==VK_APPS
        || (key>=VK_OEM_1 && key<=VK_OEM_3) || (key>=VK_OEM_4 && key<=VK_OEM_7);
}
bool QkmIbInputClient::isSupportedButton(quint16 button) {
    return button==VK_LBUTTON || button==VK_RBUTTON || button==VK_MBUTTON
        || button==VK_XBUTTON1 || button==VK_XBUTTON2;
}
unsigned QkmIbInputClient::capabilities(const QString &backend) {
    if (backend==QStringLiteral("logitech_ghub") || backend==QStringLiteral("razer")) {
        return RelativeMove|Keyboard|Buttons|VerticalWheel|AbsoluteMove|(backend==QStringLiteral("razer") ? HorizontalWheel : 0);
    }
    return 0;
}
QkmIbInputClient::Preferences QkmIbInputClient::preferences() const { QMutexLocker lock(&m_statusMutex); return m_preferences; }
bool QkmIbInputClient::setPreferences(const Preferences &preferences) {
    QMutexLocker lock(&m_statusMutex);
    if (m_published.connected || m_published.stopping || m_published.releaseIncomplete || (acceptingInput() && m_requested.load()!=0)) return false;
    if (candidates(preferences.backend).isEmpty() || (preferences.unavailablePolicy!=QStringLiteral("block") && preferences.unavailablePolicy!=QStringLiteral("sendinput_before_start"))) return false;
    if (m_preferences.backend!=preferences.backend || m_preferences.unavailablePolicy!=preferences.unavailablePolicy) {
        m_preferences=preferences;
        ++m_preferencesRevision;
    }
    return true;
}
QkmIbInputClient::Status QkmIbInputClient::statusSnapshot() const { QMutexLocker lock(&m_statusMutex); return m_published; }
quint64 QkmIbInputClient::preferencesRevision() const { QMutexLocker lock(&m_statusMutex); return m_preferencesRevision; }
bool QkmIbInputClient::busy() const { const auto state=statusSnapshot(); return state.connected || state.stopping || state.releaseIncomplete || (acceptingInput() && m_requested.load()!=0); }
void QkmIbInputClient::publish() { m_status.sampledAt=QDateTime::currentDateTimeUtc(); QMutexLocker lock(&m_statusMutex); m_published=m_status; }
bool QkmIbInputClient::fail(const QString &detail,quint64 generation) { m_status.detail=detail; m_status.failed=true; cancelRun(generation); publish(); return false; }
quint64 QkmIbInputClient::requestRun() { if (QkmAutomation::denyDemoEffect()) return 0; m_stopGeneration.store(0); const quint64 generation=m_requested.fetch_add(1)+1; m_acceptingGeneration.store(generation); return generation; }
void QkmIbInputClient::cancelRun(quint64 generation) {
    if (!generation) { m_acceptingGeneration.store(0); m_stopGeneration.store(0); }
    else {
        auto stopping=generation;
        m_acceptingGeneration.compare_exchange_strong(generation,0);
        m_stopGeneration.compare_exchange_strong(stopping,0);
    }
}
bool QkmIbInputClient::candidateSupports(const QString &backend,const Requirements &requirements) const {
    if ((capabilities(backend)&requirements.capabilities)!=requirements.capabilities) return false;
    for (quint16 key:requirements.keys) if (!isSupportedKey(key)) return false;
    for (quint16 button:requirements.mouseButtons) if (!isSupportedButton(button)) return false;
    return true;
}
bool QkmIbInputClient::prepareRun(quint64 generation,const Requirements &requirements) {
    if (QkmAutomation::denyDemoEffect()) return false;
    QMutexLocker calls(&m_callMutex);
    if (!generation || generation!=m_requested.load() || !acceptingInput()) return false;
    if (m_status.connected || m_status.stopping || m_status.releaseIncomplete) return fail(QCoreApplication::translate("QkmIbInputClient","Previous IbInput cleanup is incomplete."),generation);
    m_status=Status{}; m_status.generation=generation; m_runCapabilities=0;
    if (!requirements.rejection.isEmpty()) return fail(requirements.rejection,generation);
    const auto prefs=preferences();
    const auto ordered=candidates(prefs.backend);
    QStringList eligible;
    for (const QString &backend:ordered) if (candidateSupports(backend,requirements)) eligible.append(backend);
    if (eligible.isEmpty()) return fail(QCoreApplication::translate("QkmIbInputClient","The selected IbInput backend does not support this mapping or still awaits validation."),generation);
#ifndef IBINPUT_SUPPORT
    return fail(QCoreApplication::translate("QkmIbInputClient","IbInput support is not included in this build."),generation);
#else
    QStringList failures;
    for (const QString &backend:std::as_const(eligible)) {
        if (generation!=m_requested.load() || !acceptingInput()) return fail(QCoreApplication::translate("QkmIbInputClient","IbInput startup was cancelled."),generation);
        IbSendDestroy();
        const Send::SendType type=backend==QStringLiteral("logitech_ghub") ? Send::SendType::LogitechGHubNew
            : (backend==QStringLiteral("logitech_legacy") ? Send::SendType::Logitech : Send::SendType::Razer);
        const Send::Error error=IbSendInit(type,0,nullptr);
        if (error==Send::Error::Success) {
            m_status.connected=true; m_status.backend=backend; m_runCapabilities=requirements.capabilities;
            if (generation!=m_requested.load() || !acceptingInput()) { IbSendDestroy(); m_status.connected=false; return fail(QCoreApplication::translate("QkmIbInputClient","IbInput startup was cancelled."),generation); }
            m_status.detail=QCoreApplication::translate("QkmIbInputClient","Connected; input reception has not been verified."); publish(); return true;
        }
        IbSendDestroy(); failures.append(QStringLiteral("%1: %2").arg(backend).arg(static_cast<quint32>(error)));
    }
    if (generation!=m_requested.load() || !acceptingInput()) return fail(QCoreApplication::translate("QkmIbInputClient","IbInput startup was cancelled."),generation);
    m_status.detail=QCoreApplication::translate("QkmIbInputClient","IbInput connection failed: %1").arg(failures.join(QStringLiteral(", ")));
    if (prefs.unavailablePolicy==QStringLiteral("sendinput_before_start") && acceptingInput()) {
        m_status.fallback=true; m_status.detail+=QStringLiteral(" ")+QCoreApplication::translate("QkmIbInputClient","SendInput is frozen for this run; configuration is unchanged."); publish(); return true;
    }
    return fail(m_status.detail,generation);
#endif
}
bool QkmIbInputClient::sendOne(const INPUT &input) {
    if (QkmAutomation::denyDemoEffect()) return false;
#ifdef IBINPUT_SUPPORT
    INPUT event=input;
    return IbSendInput(1,&event,sizeof(INPUT))==1;
#else
    Q_UNUSED(input); return false;
#endif
}
bool QkmIbInputClient::runActive(quint64 generation,bool stopAction) const {
    return generation && generation==m_requested.load()
        && (acceptingInput() || (stopAction && stopActionAllowed(generation)));
}
bool QkmIbInputClient::sendEvent(const INPUT &input,quint64 generation,bool stopAction) {
    return sendEventImpl(input,generation,stopAction,nullptr);
}
bool QkmIbInputClient::sendAbsoluteEvent(const INPUT &input,POINT screenPoint,quint64 generation,bool stopAction) {
    return sendEventImpl(input,generation,stopAction,&screenPoint);
}
bool QkmIbInputClient::sendEventImpl(const INPUT &input,quint64 generation,bool stopAction,const POINT *screenPoint) {
    if (QkmAutomation::denyDemoEffect()) return false;
    const bool up=isRelease(input);
    std::unique_lock<QMutex> mouseCall(m_mouseMutex,std::defer_lock);
    if (input.type==INPUT_MOUSE && (!up || (input.mi.dwFlags & MOUSEEVENTF_MOVE))) {
        while (!mouseCall.try_lock()) {
            if (!runActive(generation,stopAction)) { if (!up) return false; break; }
            QkmIbInputSystem::pause();
        }
    }
    QMutexLocker calls(&m_callMutex);
    if (!m_status.connected || generation!=m_status.generation || generation!=m_requested.load()
        || m_status.fallback || ((!acceptingInput() || m_status.stopping) && !isRelease(input)
            && !(stopAction && stopActionAllowed(generation)))) return false;
    if (screenPoint && (input.type!=INPUT_MOUSE
        || (input.mi.dwFlags & (MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE))!=(MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE)))
        return fail(QCoreApplication::translate("QkmIbInputClient","Unsupported IbInput mouse event."),generation);
    unsigned needed=0;
    if (input.type==INPUT_KEYBOARD) needed=Keyboard;
    else if (input.type==INPUT_MOUSE) {
        if (input.mi.dwFlags & MOUSEEVENTF_MOVE) needed|=(input.mi.dwFlags & MOUSEEVENTF_ABSOLUTE) ? AbsoluteMove : RelativeMove;
        if (input.mi.dwFlags & MOUSEEVENTF_WHEEL) needed|=VerticalWheel;
        if (input.mi.dwFlags & MOUSEEVENTF_HWHEEL) needed|=HorizontalWheel;
        if (buttonId(input.mi)) needed|=Buttons;
    }
    if ((needed & m_runCapabilities)!=needed || (needed & capabilities(m_status.backend))!=needed)
        return fail(QCoreApplication::translate("QkmIbInputClient","This IbInput event was not admitted for the current run."),generation);
    if (input.type==INPUT_KEYBOARD) {
        if (!isSupportedKey(input.ki.wVk) || (input.ki.dwFlags & (KEYEVENTF_UNICODE|KEYEVENTF_SCANCODE))) return fail(QCoreApplication::translate("QkmIbInputClient","Unsupported IbInput keyboard event."),generation);
        if (!up) {
            if (m_ownedKeys.contains(input.ki.wVk)) return true;
            int ordinary=0; for (quint16 key:std::as_const(m_ownedKeys)) if (!(key>=VK_LSHIFT && key<=VK_RMENU) && key!=VK_LWIN && key!=VK_RWIN) ++ordinary;
            if (ordinary>=6 && !(input.ki.wVk>=VK_LSHIFT && input.ki.wVk<=VK_RMENU) && input.ki.wVk!=VK_LWIN && input.ki.wVk!=VK_RWIN) return fail(QCoreApplication::translate("QkmIbInputClient","The IbInput keyboard report is full."),generation);
            m_ownedKeys.insert(input.ki.wVk);
        }
        if (!sendOne(input)) return fail(QCoreApplication::translate("QkmIbInputClient","IbInput send failed; mapping must stop without retrying another backend."),generation);
        if (up) m_ownedKeys.remove(input.ki.wVk);
    } else if (input.type==INPUT_MOUSE) {
        const auto &mouse=input.mi;
        const DWORD wheelFlags=MOUSEEVENTF_WHEEL|MOUSEEVENTF_HWHEEL;
        const DWORD supported=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK|MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_LEFTUP|MOUSEEVENTF_RIGHTDOWN|MOUSEEVENTF_RIGHTUP|MOUSEEVENTF_MIDDLEDOWN|MOUSEEVENTF_MIDDLEUP|MOUSEEVENTF_XDOWN|MOUSEEVENTF_XUP|wheelFlags;
        if ((mouse.dwFlags & ~supported) || (mouse.dwFlags==0)
            || ((mouse.dwFlags & (MOUSEEVENTF_XDOWN|MOUSEEVENTF_XUP)) && mouse.mouseData!=XBUTTON1 && mouse.mouseData!=XBUTTON2)
            || ((mouse.dwFlags & wheelFlags) && (static_cast<LONG>(mouse.mouseData)%WHEEL_DELTA)!=0)
            || (mouse.dwFlags & wheelFlags)==wheelFlags
            || ((mouse.dwFlags & MOUSEEVENTF_ABSOLUTE) && !(mouse.dwFlags & MOUSEEVENTF_MOVE))
            || ((mouse.dwFlags & MOUSEEVENTF_VIRTUALDESK) && !(mouse.dwFlags & MOUSEEVENTF_ABSOLUTE))
            || ((mouse.dwFlags & MOUSEEVENTF_ABSOLUTE)
                ? (mouse.dx<0 || mouse.dx>65535 || mouse.dy<0 || mouse.dy>65535)
                : (mouse.dx>32767 || mouse.dx< -32767 || mouse.dy>32767 || mouse.dy< -32767))) return fail(QCoreApplication::translate("QkmIbInputClient","Unsupported IbInput mouse event."),generation);
        const DWORD buttonFlags=mouse.dwFlags & (MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_LEFTUP|MOUSEEVENTF_RIGHTDOWN|MOUSEEVENTF_RIGHTUP|MOUSEEVENTF_MIDDLEDOWN|MOUSEEVENTF_MIDDLEUP|MOUSEEVENTF_XDOWN|MOUSEEVENTF_XUP);
        if ((buttonFlags && (buttonFlags & (buttonFlags-1)))
            || (buttonFlags && ((mouse.dwFlags & wheelFlags) || ((mouse.dwFlags & MOUSEEVENTF_MOVE) && !(mouse.dwFlags & MOUSEEVENTF_ABSOLUTE))))
            || ((mouse.dwFlags & MOUSEEVENTF_MOVE) && (mouse.dwFlags & wheelFlags)))
            return fail(QCoreApplication::translate("QkmIbInputClient","Unsupported combined IbInput mouse event."),generation);
        const quint16 button=buttonId(mouse);
        if (button && !isSupportedButton(button)) return fail(QCoreApplication::translate("QkmIbInputClient","This IbInput mouse button still awaits backend validation."),generation);
        INPUT event=input;
        if (up && (!acceptingInput() || m_status.stopping) && !(stopAction && stopActionAllowed(generation))) {
            // A late point-click UP must release without introducing movement during cleanup.
            event.mi.dwFlags &= ~(MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK);
            event.mi.dx=event.mi.dy=0;
        }
        if (m_status.backend==QStringLiteral("logitech_ghub") && (event.mi.dwFlags & MOUSEEVENTF_ABSOLUTE)) {
            const bool virtualDesktop=(event.mi.dwFlags & MOUSEEVENTF_VIRTUALDESK)!=0;
            const int left=virtualDesktop ? QkmIbInputSystem::metric(SM_XVIRTUALSCREEN) : 0;
            const int top=virtualDesktop ? QkmIbInputSystem::metric(SM_YVIRTUALSCREEN) : 0;
            const int width=QkmIbInputSystem::metric(virtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
            const int height=QkmIbInputSystem::metric(virtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
            if (width<=1 || height<=1)
                return fail(QCoreApplication::translate("QkmIbInputClient","Unsupported IbInput mouse event."),generation);
            const auto pixel=[](LONG normalized,int origin,int extent) {
                return static_cast<LONG>(qint64(origin)+(qint64(normalized)*(extent-1)+32767)/65535);
            };
            POINT target=screenPoint ? *screenPoint : POINT{pixel(mouse.dx,left,width),pixel(mouse.dy,top,height)};
            target.x=static_cast<LONG>(qBound<qint64>(qint64(left),qint64(target.x),qint64(left)+width-1));
            target.y=static_cast<LONG>(qBound<qint64>(qint64(top),qint64(target.y),qint64(top)+height-1));
            calls.unlock();
            const auto result=moveGHubTo(target,generation,stopAction);
            calls.relock();
            if (result!=MoveResult::Reached || !m_status.connected || m_status.generation!=generation
                || !runActive(generation,stopAction) || (m_status.stopping && !(stopAction && stopActionAllowed(generation)))) {
                if (up && m_status.connected && m_status.generation==generation && generation==m_requested.load()) {
                    if (sendOne(releaseButton(button))) m_ownedButtons.remove(button);
                    else return fail(QCoreApplication::translate("QkmIbInputClient","IbInput send failed; mapping must stop without retrying another backend."),generation);
                }
                return false;
            }
            // Move without a new DOWN, then apply the button at the observed destination.
            event.mi.dwFlags &= ~(MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK);
            event.mi.dx=event.mi.dy=0;
            if (!button) return true;
        }
        if (button && !up) m_ownedButtons.insert(button);
        if (mouse.dwFlags & wheelFlags) {
            const LONG delta=static_cast<LONG>(mouse.mouseData);
            INPUT step=event; step.mi.mouseData=static_cast<DWORD>(delta>0 ? WHEEL_DELTA : -WHEEL_DELTA);
            for (LONG left=delta/WHEEL_DELTA;left!=0;left+=left>0 ? -1 : 1) if (!sendOne(step)) return fail(QCoreApplication::translate("QkmIbInputClient","IbInput wheel send failed; mapping must stop."),generation);
        } else if (!sendOne(event)) return fail(QCoreApplication::translate("QkmIbInputClient","IbInput send failed; mapping must stop without retrying another backend."),generation);
        if (button && up) m_ownedButtons.remove(button);
    } else return fail(QCoreApplication::translate("QkmIbInputClient","Unsupported IbInput event type."),generation);
    return true;
}
QkmIbInputClient::MoveResult QkmIbInputClient::positionFailure(const QString &detail,quint64 generation,bool stopAction) {
    QMutexLocker calls(&m_callMutex);
    if (!runActive(generation,stopAction) || m_status.generation!=generation || !m_status.connected
        || (m_status.stopping && !(stopAction && stopActionAllowed(generation))))
        return MoveResult::Cancelled;
    fail(detail,generation); return MoveResult::Failed;
}
QkmIbInputClient::MoveResult QkmIbInputClient::moveGHubTo(POINT target,quint64 generation,bool stopAction) {
    // shortcut: synchronous vendor I/O can outlive this budget; cancellation needs upstream support.
    const quint64 started=QkmIbInputSystem::ticks();
    const auto position=[](POINT &point) {
        SetLastError(ERROR_SUCCESS);
        return QkmIbInputSystem::cursorPosition(&point)!=FALSE;
    };
    POINT current{};
    if (!runActive(generation,stopAction)) return MoveResult::Cancelled;
    if (!position(current)) return positionFailure(cursorPositionError(),generation,stopAction);
    double gainX=3.2,gainY=3.2;
    int sends=0;
    while (QkmIbInputSystem::ticks()-started<200) {
        if (!runActive(generation,stopAction)) return MoveResult::Cancelled;
        const qint64 dx=qint64(target.x)-current.x,dy=qint64(target.y)-current.y;
        if (std::abs(dx)<=2 && std::abs(dy)<=2) {
            QkmIbInputSystem::pause();
            if (!runActive(generation,stopAction)) return MoveResult::Cancelled;
            if (!position(current)) return positionFailure(cursorPositionError(),generation,stopAction);
            if (QkmIbInputSystem::ticks()-started<200
                && std::abs(qint64(target.x)-current.x)<=2 && std::abs(qint64(target.y)-current.y)<=2)
                return MoveResult::Reached;
            continue;
        }
        if (sends>=32) break;
        const auto step=[&](qint64 error,double gain) {
            if (std::abs(error)<=2) return LONG{0};
            const qint64 value=std::max<qint64>(1,static_cast<qint64>(std::llround(std::abs(error)/(gain*1.25))));
            const qint64 limit=sends==0 ? 128 : 32767;
            return static_cast<LONG>((error<0 ? -1 : 1)*qMin(value,limit));
        };
        INPUT move{}; move.type=INPUT_MOUSE; move.mi.dwFlags=MOUSEEVENTF_MOVE;
        move.mi.dx=step(dx,gainX); move.mi.dy=step(dy,gainY);
        const POINT before=current;
        {
            QMutexLocker calls(&m_callMutex);
            if (!m_status.connected || m_status.generation!=generation || !runActive(generation,stopAction)
                || (m_status.stopping && !(stopAction && stopActionAllowed(generation))))
                return MoveResult::Cancelled;
            // This relative report belongs to the already-admitted absolute transaction.
            if (!sendOne(move)) {
                fail(QCoreApplication::translate("QkmIbInputClient","IbInput send failed; mapping must stop without retrying another backend."),generation);
                return MoveResult::Failed;
            }
        }
        ++sends;
        for (int sample=0;sample<4;++sample) {
            QkmIbInputSystem::pause();
            if (!runActive(generation,stopAction)) return MoveResult::Cancelled;
            if (!position(current)) return positionFailure(cursorPositionError(),generation,stopAction);
            if (current.x!=before.x || current.y!=before.y || QkmIbInputSystem::ticks()-started>=200) break;
        }
        const auto observe=[](LONG command,qint64 actual,qint64 oldError,qint64 newError,double &gain) {
            if (!command) return;
            if (!actual) { gain=qMax(1.0/32.0,gain*0.5); return; }
            const double observed=std::abs(static_cast<double>(actual)/command);
            gain=std::clamp((gain+observed)*0.5,1.0/32.0,16.0);
            if ((oldError<0 && newError>0) || (oldError>0 && newError<0))
                gain=qMin(16.0,qMax(gain,observed)*1.5);
        };
        observe(move.mi.dx,qint64(current.x)-before.x,dx,qint64(target.x)-current.x,gainX);
        observe(move.mi.dy,qint64(current.y)-before.y,dy,qint64(target.y)-current.y,gainY);
    }
    return positionFailure(QCoreApplication::translate("QkmIbInputClient","Logitech G HUB could not reach (%1, %2); last position: (%3, %4). Mapping must stop.")
        .arg(target.x).arg(target.y).arg(current.x).arg(current.y),generation,stopAction);
}
bool QkmIbInputClient::releaseLocked() {
    const auto keys=m_ownedKeys;
    for (quint16 key:keys) { INPUT input{}; input.type=INPUT_KEYBOARD; input.ki.wVk=key; input.ki.dwFlags=KEYEVENTF_KEYUP; if (sendOne(input)) m_ownedKeys.remove(key); }
    const auto buttons=m_ownedButtons;
    for (quint16 button:buttons) if (sendOne(releaseButton(button))) m_ownedButtons.remove(button);
    m_status.releaseIncomplete=!m_ownedKeys.isEmpty() || !m_ownedButtons.isEmpty();
    if (m_status.releaseIncomplete) m_status.detail=QCoreApplication::translate("QkmIbInputClient","IbInput release is incomplete; automatic restart is blocked.");
    return !m_status.releaseIncomplete;
}
bool QkmIbInputClient::releaseOwnedInputs(quint64 generation) {
    if (QkmAutomation::denyDemoEffect()) return false;
    QMutexLocker calls(&m_callMutex);
    if (generation!=m_status.generation) return false;
    m_status.stopping=true;
    const bool result=m_status.connected ? releaseLocked() : !m_status.releaseIncomplete;
    publish(); return result;
}
bool QkmIbInputClient::beginStopAction(quint64 generation) {
    if (QkmAutomation::denyDemoEffect()) return false;
    QMutexLocker calls(&m_callMutex);
    if (!generation || generation!=m_status.generation || generation!=m_requested.load() || acceptingInput()
        || !m_status.stopping || m_status.releaseIncomplete || (!m_status.connected && !m_status.fallback)) return false;
    m_stopGeneration.store(generation);
    return true;
}
bool QkmIbInputClient::closeAfterDrain(quint64 generation) {
    if (QkmAutomation::denyDemoEffect()) return false;
    QMutexLocker calls(&m_callMutex);
    if (generation!=m_status.generation) return false;
    cancelRun(generation); quint64 stopping=generation; m_stopGeneration.compare_exchange_strong(stopping,0); m_status.stopping=true;
    const bool released=m_status.connected ? releaseLocked() : !m_status.releaseIncomplete;
#ifdef IBINPUT_SUPPORT
    if (m_status.connected) IbSendDestroy();
#endif
    m_status.connected=false; m_status.stopping=false; publish(); return released;
}
bool QkmIbInputClient::checkConnection() {
    if (QkmAutomation::denyDemoEffect()) return false;
    if (busy()) return false;
    const auto generation=requestRun();
    const bool connected=prepareRun(generation,Requirements{});
    const bool actual=statusSnapshot().connected;
    closeAfterDrain(generation);
    return connected && actual;
}
