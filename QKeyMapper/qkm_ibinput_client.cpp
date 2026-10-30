#include "qkm_ibinput_client.h"
#include <QCoreApplication>
#include <QMutexLocker>
#include <QSettings>
#ifdef IBINPUT_SUPPORT
#include <IbInputSimulator/InputSimulator.hpp>
#endif

namespace {
const QString backendKey=QStringLiteral("IbInputPreferredBackend");
const QString policyKey=QStringLiteral("IbInputUnavailablePolicy");
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
#ifndef QKM_IBINPUT_VALIDATION_PROBE
    // shortcut: other key representations await product reception and release acceptance.
    return basic;
#else
    return basic || (key>=VK_NUMPAD0 && key<=VK_DIVIDE && key!=VK_SEPARATOR)
        || (key>=VK_LSHIFT && key<=VK_RMENU) || key==VK_LWIN || key==VK_RWIN
        || key==VK_BACK || key==VK_TAB || key==VK_RETURN || key==VK_ESCAPE || key==VK_SPACE
        || (key>=VK_PRIOR && key<=VK_DOWN) || key==VK_INSERT || key==VK_DELETE || key==VK_CAPITAL
        || key==VK_NUMLOCK || key==VK_SCROLL || key==VK_SNAPSHOT;
#endif
}
bool QkmIbInputClient::isSupportedButton(quint16 button) {
    // shortcut: expand after right/middle/X1/X2 reception and release are recorded on a real backend.
    return button==VK_LBUTTON;
}
unsigned QkmIbInputClient::capabilities(const QString &backend) {
    // shortcut: other backend output awaits its recorded acceptance before opening it.
    if (backend==QStringLiteral("logitech_ghub")) {
#ifdef QKM_IBINPUT_VALIDATION_PROBE
        return RelativeMove|Keyboard|Buttons|VerticalWheel;
#else
        // shortcut: mouse buttons and wheel await product reception and release acceptance.
        return RelativeMove|Keyboard;
#endif
    }
    return 0;
}
QkmIbInputClient::Preferences QkmIbInputClient::preferences() const { QMutexLocker lock(&m_statusMutex); return m_preferences; }
bool QkmIbInputClient::setPreferences(const Preferences &preferences) {
    QMutexLocker lock(&m_statusMutex);
    if (m_published.connected || m_published.stopping || m_published.releaseIncomplete || (!m_cancelled.load() && m_requested.load()!=0)) return false;
    if (candidates(preferences.backend).isEmpty() || (preferences.unavailablePolicy!=QStringLiteral("block") && preferences.unavailablePolicy!=QStringLiteral("sendinput_before_start"))) return false;
    m_preferences=preferences; return true;
}
QkmIbInputClient::Status QkmIbInputClient::statusSnapshot() const { QMutexLocker lock(&m_statusMutex); return m_published; }
bool QkmIbInputClient::busy() const { const auto state=statusSnapshot(); return state.connected || state.stopping || state.releaseIncomplete || (!m_cancelled.load() && m_requested.load()!=0); }
void QkmIbInputClient::publish() { m_status.sampledAt=QDateTime::currentDateTimeUtc(); QMutexLocker lock(&m_statusMutex); m_published=m_status; }
bool QkmIbInputClient::fail(const QString &detail) { m_status.detail=detail; m_cancelled.store(true); publish(); return false; }
quint64 QkmIbInputClient::requestRun() { const quint64 generation=m_requested.fetch_add(1)+1; m_cancelled.store(false); return generation; }
void QkmIbInputClient::cancelRun() { m_cancelled.store(true); }
bool QkmIbInputClient::candidateSupports(const QString &backend,const Requirements &requirements) const {
    if ((capabilities(backend)&requirements.capabilities)!=requirements.capabilities) return false;
    for (quint16 key:requirements.keys) if (!isSupportedKey(key)) return false;
    for (quint16 button:requirements.mouseButtons) if (!isSupportedButton(button)) return false;
    // shortcut: button combinations and dragging await recorded real-backend acceptance.
    if (backend.startsWith(QStringLiteral("logitech")) && (requirements.mouseButtons.size()>1
        || requirements.buttonStateOverlap)) return false;
    return true;
}
bool QkmIbInputClient::prepareRun(quint64 generation,const Requirements &requirements) {
    QMutexLocker calls(&m_callMutex);
    if (!generation || generation!=m_requested.load() || m_cancelled.load()) return false;
    if (m_status.connected || m_status.stopping || m_status.releaseIncomplete) return fail(QCoreApplication::translate("QkmIbInputClient","Previous IbInput cleanup is incomplete."));
    m_status=Status{}; m_status.generation=generation; m_runCapabilities=0;
    if (!requirements.rejection.isEmpty()) return fail(requirements.rejection);
    const auto prefs=preferences();
    QStringList eligible;
    for (const QString &backend:candidates(prefs.backend)) if (candidateSupports(backend,requirements)) eligible.append(backend);
    if (eligible.isEmpty()) return fail(QCoreApplication::translate("QkmIbInputClient","The selected IbInput backend does not support this mapping or still awaits validation."));
#ifndef IBINPUT_SUPPORT
    return fail(QCoreApplication::translate("QkmIbInputClient","IbInput support is not included in this build."));
#else
    QStringList failures;
    for (const QString &backend:eligible) {
        if (generation!=m_requested.load() || m_cancelled.load()) return fail(QCoreApplication::translate("QkmIbInputClient","IbInput startup was cancelled."));
        IbSendDestroy();
        const Send::SendType type=backend==QStringLiteral("logitech_ghub") ? Send::SendType::LogitechGHubNew
            : (backend==QStringLiteral("logitech_legacy") ? Send::SendType::Logitech : Send::SendType::Razer);
        const Send::Error error=IbSendInit(type,0,nullptr);
        if (error==Send::Error::Success) {
            m_status.connected=true; m_status.backend=backend; m_runCapabilities=requirements.capabilities;
            if (generation!=m_requested.load() || m_cancelled.load()) { IbSendDestroy(); m_status.connected=false; return fail(QCoreApplication::translate("QkmIbInputClient","IbInput startup was cancelled.")); }
            m_status.detail=QCoreApplication::translate("QkmIbInputClient","Connected; input reception has not been verified."); publish(); return true;
        }
        IbSendDestroy(); failures.append(QStringLiteral("%1: %2").arg(backend).arg(static_cast<quint32>(error)));
    }
    m_status.detail=QCoreApplication::translate("QkmIbInputClient","IbInput connection failed: %1").arg(failures.join(QStringLiteral(", ")));
    if (prefs.unavailablePolicy==QStringLiteral("sendinput_before_start") && !m_cancelled.load()) {
        m_status.fallback=true; m_status.detail+=QStringLiteral(" ")+QCoreApplication::translate("QkmIbInputClient","SendInput is frozen for this run; configuration is unchanged."); publish(); return true;
    }
    return fail(m_status.detail);
#endif
}
bool QkmIbInputClient::sendOne(const INPUT &input) {
#ifdef IBINPUT_SUPPORT
    INPUT event=input;
    return IbSendInput(1,&event,sizeof(INPUT))==1;
#else
    Q_UNUSED(input); return false;
#endif
}
bool QkmIbInputClient::sendEvent(const INPUT &input,quint64 generation) {
    QMutexLocker calls(&m_callMutex);
    if (!m_status.connected || generation!=m_status.generation || generation!=m_requested.load()
        || m_status.fallback || ((m_cancelled.load() || m_status.stopping) && !isRelease(input))) return false;
    unsigned needed=0;
    if (input.type==INPUT_KEYBOARD) needed=Keyboard;
    else if (input.type==INPUT_MOUSE) {
        if (input.mi.dwFlags & MOUSEEVENTF_MOVE) needed|=RelativeMove;
        if (input.mi.dwFlags & MOUSEEVENTF_WHEEL) needed|=VerticalWheel;
        if (buttonId(input.mi)) needed|=Buttons;
    }
    if ((needed & m_runCapabilities)!=needed || (needed & capabilities(m_status.backend))!=needed)
        return fail(QCoreApplication::translate("QkmIbInputClient","This IbInput event was not admitted for the current run."));
    if (input.type==INPUT_KEYBOARD) {
        if (!isSupportedKey(input.ki.wVk) || (input.ki.dwFlags & (KEYEVENTF_UNICODE|KEYEVENTF_SCANCODE))) return fail(QCoreApplication::translate("QkmIbInputClient","Unsupported IbInput keyboard event."));
        const bool up=(input.ki.dwFlags & KEYEVENTF_KEYUP)!=0;
        if (!up) {
            if (m_ownedKeys.contains(input.ki.wVk)) return true;
            int ordinary=0; for (quint16 key:m_ownedKeys) if (!(key>=VK_LSHIFT && key<=VK_RMENU) && key!=VK_LWIN && key!=VK_RWIN) ++ordinary;
            if (ordinary>=6 && !(input.ki.wVk>=VK_LSHIFT && input.ki.wVk<=VK_RMENU) && input.ki.wVk!=VK_LWIN && input.ki.wVk!=VK_RWIN) return fail(QCoreApplication::translate("QkmIbInputClient","The IbInput keyboard report is full."));
            m_ownedKeys.insert(input.ki.wVk);
        }
        if (!sendOne(input)) return fail(QCoreApplication::translate("QkmIbInputClient","IbInput send failed; mapping must stop without retrying another backend."));
        if (up) m_ownedKeys.remove(input.ki.wVk);
    } else if (input.type==INPUT_MOUSE) {
        const auto &mouse=input.mi;
        const DWORD supported=MOUSEEVENTF_MOVE|MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_LEFTUP|MOUSEEVENTF_RIGHTDOWN|MOUSEEVENTF_RIGHTUP|MOUSEEVENTF_MIDDLEDOWN|MOUSEEVENTF_MIDDLEUP|MOUSEEVENTF_XDOWN|MOUSEEVENTF_XUP|MOUSEEVENTF_WHEEL;
        if ((mouse.dwFlags & ~supported) || (mouse.dwFlags==0)
            || ((mouse.dwFlags & (MOUSEEVENTF_XDOWN|MOUSEEVENTF_XUP)) && mouse.mouseData!=XBUTTON1 && mouse.mouseData!=XBUTTON2)
            || ((mouse.dwFlags & MOUSEEVENTF_WHEEL) && (static_cast<LONG>(mouse.mouseData)%WHEEL_DELTA)!=0)
            || mouse.dx>32767 || mouse.dx< -32767 || mouse.dy>32767 || mouse.dy< -32767) return fail(QCoreApplication::translate("QkmIbInputClient","Unsupported IbInput mouse event."));
        const DWORD buttonFlags=mouse.dwFlags & (MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_LEFTUP|MOUSEEVENTF_RIGHTDOWN|MOUSEEVENTF_RIGHTUP|MOUSEEVENTF_MIDDLEDOWN|MOUSEEVENTF_MIDDLEUP|MOUSEEVENTF_XDOWN|MOUSEEVENTF_XUP);
        if ((buttonFlags && (buttonFlags & (buttonFlags-1))) || (buttonFlags && (mouse.dwFlags & (MOUSEEVENTF_MOVE|MOUSEEVENTF_WHEEL)))
            || ((mouse.dwFlags & MOUSEEVENTF_MOVE) && (mouse.dwFlags & MOUSEEVENTF_WHEEL)))
            return fail(QCoreApplication::translate("QkmIbInputClient","Unsupported combined IbInput mouse event."));
        const quint16 button=buttonId(mouse);
        if (button && !isSupportedButton(button)) return fail(QCoreApplication::translate("QkmIbInputClient","This IbInput mouse button still awaits backend validation."));
        const bool up=isRelease(input);
        if ((!m_ownedButtons.isEmpty() && (mouse.dwFlags & (MOUSEEVENTF_MOVE|MOUSEEVENTF_WHEEL)))
            || (button && !up && !m_ownedButtons.isEmpty() && !m_ownedButtons.contains(button))) return fail(QCoreApplication::translate("QkmIbInputClient","This IbInput button combination still awaits backend validation."));
        if (button && !up) m_ownedButtons.insert(button);
        if (mouse.dwFlags & MOUSEEVENTF_WHEEL) {
            const LONG delta=static_cast<LONG>(mouse.mouseData);
            INPUT step=input; step.mi.mouseData=static_cast<DWORD>(delta>0 ? WHEEL_DELTA : -WHEEL_DELTA);
            for (LONG left=delta/WHEEL_DELTA;left!=0;left+=left>0 ? -1 : 1) if (!sendOne(step)) return fail(QCoreApplication::translate("QkmIbInputClient","IbInput wheel send failed; mapping must stop."));
        } else if (!sendOne(input)) return fail(QCoreApplication::translate("QkmIbInputClient","IbInput send failed; mapping must stop without retrying another backend."));
        if (button && up) m_ownedButtons.remove(button);
    } else return fail(QCoreApplication::translate("QkmIbInputClient","Unsupported IbInput event type."));
    return true;
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
    QMutexLocker calls(&m_callMutex);
    if (generation!=m_status.generation) return false;
    m_status.stopping=true;
    const bool result=m_status.connected ? releaseLocked() : !m_status.releaseIncomplete;
    publish(); return result;
}
bool QkmIbInputClient::closeAfterDrain(quint64 generation) {
    QMutexLocker calls(&m_callMutex);
    if (generation!=m_status.generation) return false;
    m_cancelled.store(true); m_status.stopping=true;
    const bool released=m_status.connected ? releaseLocked() : !m_status.releaseIncomplete;
#ifdef IBINPUT_SUPPORT
    if (m_status.connected) IbSendDestroy();
#endif
    m_status.connected=false; m_status.stopping=false; publish(); return released;
}
bool QkmIbInputClient::checkConnection() {
    if (busy()) return false;
    const auto generation=requestRun();
    const bool connected=prepareRun(generation,Requirements{});
    const bool actual=statusSnapshot().connected;
    closeAfterDrain(generation);
    return connected && actual;
}
