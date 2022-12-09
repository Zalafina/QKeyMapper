#include "qkeymapper.h"
#include "qkeymapper_worker.h"

static const int KEY_UP = 0;
static const int KEY_DOWN = 1;

static const int SENDMODE_HOOK          = 0;
static const int SENDMODE_BURST_NORMAL  = 1;
static const int SENDMODE_BURST_STOP    = 2;

static const int SEND_INPUTS_MAX = 20;

static const ULONG_PTR VIRTUAL_KEYBOARD_PRESS = 0xACBDACBD;
static const ULONG_PTR VIRTUAL_MOUSE_CLICK = 0xCEDFCEDF;

QKeyMapper_Worker *QKeyMapper_Worker::m_instance = Q_NULLPTR;
QHash<QString, V_KEYCODE> QKeyMapper_Worker::VirtualKeyCodeMap = QHash<QString, V_KEYCODE>();
QHash<QString, V_MOUSECODE> QKeyMapper_Worker::VirtualMouseButtonMap = QHash<QString, V_MOUSECODE>();
QHash<WPARAM, QString> QKeyMapper_Worker::MouseButtonNameMap = QHash<WPARAM, QString>();
QStringList QKeyMapper_Worker::pressedRealKeysList = QStringList();
QStringList QKeyMapper_Worker::pressedVirtualKeysList = QStringList();
QStringList QKeyMapper_Worker::pressedLockKeysList = QStringList();
QMutex *QKeyMapper_Worker::sendinput_mutex = Q_NULLPTR;

QKeyMapper_Worker::QKeyMapper_Worker(QObject *parent) :
    m_KeyHook(Q_NULLPTR),
#ifdef SUPPORT_MOUSE_LL_HOOK
    m_MouseHook(Q_NULLPTR),
#endif
    m_BurstTimerMap(),
    m_BurstKeyUpTimerMap()
{
    qRegisterMetaType<HWND>("HWND");
    qRegisterMetaType<V_KEYCODE>("V_KEYCODE");
    qRegisterMetaType<V_MOUSECODE>("V_MOUSECODE");

    Q_UNUSED(parent);
    sendinput_mutex = new QMutex(QMutex::Recursive);

    QObject::connect(this, SIGNAL(setKeyHook_Signal(HWND)), this, SLOT(setWorkerKeyHook(HWND)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(setKeyUnHook_Signal()), this, SLOT(setWorkerKeyUnHook()), Qt::QueuedConnection);

    QObject::connect(this, SIGNAL(startBurstTimer_Signal(QString,int)), this, SLOT(startBurstTimer(QString,int)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(stopBurstTimer_Signal(QString,int)), this, SLOT(stopBurstTimer(QString,int)), Qt::QueuedConnection);

    QObject::connect(this, SIGNAL(sendKeyboardInput_Signal(V_KEYCODE,int)), this, SLOT(sendKeyboardInput(V_KEYCODE,int)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(sendMouseInput_Signal(V_MOUSECODE,int)), this, SLOT(sendMouseInput(V_MOUSECODE,int)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(sendInputKeys_Signal(QStringList,int,QString,int)), this, SLOT(sendInputKeys(QStringList,int,QString,int)), Qt::QueuedConnection);

    initVirtualKeyCodeMap();
    initVirtualMouseButtonMap();
}

QKeyMapper_Worker::~QKeyMapper_Worker()
{
    setWorkerKeyUnHook();

    delete sendinput_mutex;
    sendinput_mutex = Q_NULLPTR;

    delete m_instance;
    m_instance = Q_NULLPTR;
}

void QKeyMapper_Worker::sendKeyboardInput(V_KEYCODE vkeycode, int keyupdown)
{
    QMutexLocker locker(sendinput_mutex);

    INPUT keyboard_input;
    DWORD extenedkeyflag = 0;
    if (true == vkeycode.ExtenedFlag){
        extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
    }
    else{
        extenedkeyflag = 0;
    }
    keyboard_input.type = INPUT_KEYBOARD;
    keyboard_input.ki.wScan = 0;
    keyboard_input.ki.time = 0;
    keyboard_input.ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
    keyboard_input.ki.wVk = vkeycode.KeyCode;
    if (KEY_DOWN == keyupdown) {
        keyboard_input.ki.dwFlags = extenedkeyflag | 0;
    }
    else {
        keyboard_input.ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
    }
    UINT uSent = SendInput(1, &keyboard_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("sendKeyboardInput(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::sendMouseInput(V_MOUSECODE vmousecode, int keyupdown)
{
    QMutexLocker locker(sendinput_mutex);

    INPUT mouse_input;
    mouse_input.type = INPUT_MOUSE;
    mouse_input.mi.dx = 0;
    mouse_input.mi.dy = 0;
    mouse_input.mi.mouseData = 0;
    mouse_input.mi.time = 0;
    mouse_input.mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
    if (KEY_DOWN == keyupdown) {
        mouse_input.mi.dwFlags = vmousecode.MouseDownCode;
    }
    else {
        mouse_input.mi.dwFlags = vmousecode.MouseUpCode;
    }
    UINT uSent = SendInput(1, &mouse_input, sizeof(INPUT));
    if (uSent != 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("sendMouseInput(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::sendInputKeys(QStringList inputKeys, int keyupdown, QString original_key, int sendmode)
{
    int keycount = inputKeys.size();
    if (keycount <= 0) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("sendInputKeys(): no input keys, size error(%d)!!!", keycount);
#endif
        return;
    }
    else if (keycount > SEND_INPUTS_MAX) {
#ifdef DEBUG_LOGOUT_ON
        qWarning("sendInputKeys(): too many input keys(%d)!!!", keycount);
#endif
        return;
    }

    QMutexLocker locker(sendinput_mutex);

    int index = 0;
    QStringList moustButtons;
    INPUT inputs[SEND_INPUTS_MAX] = { 0 };

    if (KEY_UP == keyupdown) {
        for(auto it = inputKeys.crbegin(); it != inputKeys.crend(); ++it) {
            QString key = (*it);

            /* special hook key process */
            if (SENDMODE_HOOK == sendmode) {
                if ((original_key == key) && (keycount == 1)) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug("Mapping the same key, do not skip send mapping VirtualKey \"%s\" KEYUP!", key.toStdString().c_str());
#endif
                }
                else {
                    if (pressedRealKeysList.contains(key)){
                        pressedVirtualKeysList.removeAll(key);
#ifdef DEBUG_LOGOUT_ON
                        qDebug("RealKey \"%s\" is pressed down on keyboard, skip send mapping VirtualKey \"%s\" KEYUP!", key.toStdString().c_str(), key.toStdString().c_str());
                        qDebug("Remove \"%s\" in pressedVirtualKeysList.", key.toStdString().c_str());
#endif
                        continue;
                    }
                }
            }
            else if (SENDMODE_BURST_STOP == sendmode) {
                if (pressedRealKeysList.contains(key)){
                    pressedVirtualKeysList.removeAll(key);
#ifdef DEBUG_LOGOUT_ON
                    qDebug("stopBurstTimer()->sendBurstKeyUp()->sendInputKeys(): RealKey \"%s\" is pressed down on keyboard, skip send mapping VirtualKey \"%s\" KEYUP!", key.toStdString().c_str(), key.toStdString().c_str());
#endif
                    continue;
                }
            }

            INPUT *input_p = &inputs[index];
            if (true == VirtualMouseButtonMap.contains(key)) {
                if (false == pressedVirtualKeysList.contains(key)) {
                    qCritical("sendInputKeys(): Mouse Button Up -> \"%s\" do not exist!!!", key.toStdString().c_str());
                }

                moustButtons.append(key);
                V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);
                input_p->type = INPUT_MOUSE;
                input_p->mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
                if (KEY_DOWN == keyupdown) {
                    input_p->mi.dwFlags = vmousecode.MouseDownCode;
                }
                else {
                    input_p->mi.dwFlags = vmousecode.MouseUpCode;
                }
            }
            else if (true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(key)) {
                V_KEYCODE vkeycode = QKeyMapper_Worker::VirtualKeyCodeMap.value(key);
                DWORD extenedkeyflag = 0;
                if (true == vkeycode.ExtenedFlag){
                    extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
                }
                else{
                    extenedkeyflag = 0;
                }
                input_p->type = INPUT_KEYBOARD;
                input_p->ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
                input_p->ki.wVk = vkeycode.KeyCode;
                if (KEY_DOWN == keyupdown) {
                    input_p->ki.dwFlags = extenedkeyflag | 0;
                }
                else {
                    input_p->ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("sendInputKeys(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
            }
            index++;
        }
    }
    else {
        for (const QString &key : inputKeys){
            INPUT *input_p = &inputs[index];
            if (true == VirtualMouseButtonMap.contains(key)) {
                if (true == pressedVirtualKeysList.contains(key)) {
                    qCritical("sendInputKeys(): Mouse Button Down -> \"%s\" already exist!!!", key.toStdString().c_str());
                }

                moustButtons.append(key);
                V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(key);
                input_p->type = INPUT_MOUSE;
                input_p->mi.dwExtraInfo = VIRTUAL_MOUSE_CLICK;
                if (KEY_DOWN == keyupdown) {
                    input_p->mi.dwFlags = vmousecode.MouseDownCode;
                }
                else {
                    input_p->mi.dwFlags = vmousecode.MouseUpCode;
                }
            }
            else if (true == QKeyMapper_Worker::VirtualKeyCodeMap.contains(key)) {
                V_KEYCODE vkeycode = QKeyMapper_Worker::VirtualKeyCodeMap.value(key);
                DWORD extenedkeyflag = 0;
                if (true == vkeycode.ExtenedFlag){
                    extenedkeyflag = KEYEVENTF_EXTENDEDKEY;
                }
                else{
                    extenedkeyflag = 0;
                }
                input_p->type = INPUT_KEYBOARD;
                input_p->ki.dwExtraInfo = VIRTUAL_KEYBOARD_PRESS;
                input_p->ki.wVk = vkeycode.KeyCode;
                if (KEY_DOWN == keyupdown) {
                    input_p->ki.dwFlags = extenedkeyflag | 0;
                }
                else {
                    input_p->ki.dwFlags = extenedkeyflag | KEYEVENTF_KEYUP;
                }
            }
            else {
#ifdef DEBUG_LOGOUT_ON
                qWarning("sendInputKeys(): VirtualMap do not contains \"%s\" !!!", key.toStdString().c_str());
#endif
            }
            index++;
        }
    }

    UINT uSent = SendInput(keycount, inputs, sizeof(INPUT));
    if (uSent != keycount) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("sendInputKeys(): SendInput failed: 0x%X\n", HRESULT_FROM_WIN32(GetLastError()));
#endif
    }
}

void QKeyMapper_Worker::sendBurstKeyDown(const QString &burstKey)
{
    int findindex = QKeyMapper::findInKeyMappingDataList(burstKey);

    if (findindex >=0){
        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
        QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
        sendInputKeys(mappingKeyList, KEY_DOWN, original_key, SENDMODE_BURST_NORMAL);
    }
}

void QKeyMapper_Worker::sendBurstKeyUp(const QString &burstKey, bool stop)
{
    int findindex = QKeyMapper::findInKeyMappingDataList(burstKey);

    if (findindex >=0){
        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
        QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
        int sendmode = SENDMODE_BURST_NORMAL;
        if (true == stop) {
            sendmode = SENDMODE_BURST_STOP;
        }
        sendInputKeys(mappingKeyList, KEY_UP, original_key, sendmode);
    }
}

void QKeyMapper_Worker::sendSpecialVirtualKeyDown(const QString &virtualKey)
{
    if (true == VirtualMouseButtonMap.contains(virtualKey)) {
        V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(virtualKey);
        sendMouseInput(vmousecode, KEY_DOWN);
    }
    else {
        V_KEYCODE map_vkeycode = QKeyMapper_Worker::VirtualKeyCodeMap.value(virtualKey);
        sendKeyboardInput(map_vkeycode, KEY_DOWN);
    }
}

void QKeyMapper_Worker::sendSpecialVirtualKeyUp(const QString &virtualKey)
{
    if (true == VirtualMouseButtonMap.contains(virtualKey)) {
        V_MOUSECODE vmousecode = VirtualMouseButtonMap.value(virtualKey);
        sendMouseInput(vmousecode, KEY_UP);
    }
    else {
        V_KEYCODE map_vkeycode = QKeyMapper_Worker::VirtualKeyCodeMap.value(virtualKey);
        sendKeyboardInput(map_vkeycode, KEY_UP);
    }
}

void QKeyMapper_Worker::timerEvent(QTimerEvent *event)
{
    int timerID = event->timerId();

    if (true == m_BurstKeyUpTimerMap.values().contains(timerID)) {
        QString burstKey = m_BurstKeyUpTimerMap.key(timerID);
        if (false == burstKey.isEmpty()) {
            killTimer(timerID);
            m_BurstKeyUpTimerMap.remove(burstKey);
            sendBurstKeyUp(burstKey, false);
//#ifdef DEBUG_LOGOUT_ON
//            qDebug("timerEvent(): Key \"%s\" BurstPress timeout.", burstKey.toStdString().c_str());
//#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("timerEvent(): Could not find TimerID(%d) in m_BurstKeyUpTimerMap!!!", timerID);
#endif
        }
    }
    else if (true == m_BurstTimerMap.values().contains(timerID)) {
        QString burstKey = m_BurstTimerMap.key(timerID);
        if (false == burstKey.isEmpty()) {
//#ifdef DEBUG_LOGOUT_ON
//            qDebug("timerEvent(): Key \"%s\" BurstTimer timeout.", burstKey.toStdString().c_str());
//#endif
            sendBurstKeyDown(burstKey);

            if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
                int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
                qDebug("timerEvent(): Key \"%s\" BurstKeyUpTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
                killTimer(existTimerID);
                m_BurstKeyUpTimerMap.remove(burstKey);
            }
            int burstpressTime = QKeyMapper::getBurstPressTime();
            int keyupTimerID = startTimer(burstpressTime, Qt::PreciseTimer);
            m_BurstKeyUpTimerMap.insert(burstKey, keyupTimerID);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("timerEvent(): Could not find TimerID(%d) in BurstTimerMap!!!", timerID);
#endif
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning("timerEvent(): Could not find TimerID(%d) in both m_BurstKeyUpTimerMap & BurstTimerMap!!!", timerID);
#endif
    }
}

void QKeyMapper_Worker::threadStarted()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("threadStarted() -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
#endif
}

void QKeyMapper_Worker::setWorkerKeyHook(HWND hWnd)
{
    if(TRUE == IsWindowVisible(hWnd)){
        clearAllBurstTimersAndLockKeys();
        pressedRealKeysList.clear();
        pressedVirtualKeysList.clear();
        m_BurstTimerMap.clear();
        m_BurstKeyUpTimerMap.clear();
        pressedLockKeysList.clear();
        m_KeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, QKeyMapper_Worker::LowLevelKeyboardHookProc, GetModuleHandle(Q_NULLPTR), 0);
#ifdef SUPPORT_MOUSE_LL_HOOK
        m_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, QKeyMapper_Worker::LowLevelMouseHookProc, GetModuleHandle(Q_NULLPTR), 0);
#endif
        qInfo("[setKeyHook] Normal Key Hook & Mouse Hook Started.");
    }
    else{
        qWarning("[setKeyHook] Error: Invisible Window Handle!!!");
    }
}

void QKeyMapper_Worker::setWorkerKeyUnHook()
{
    if (m_KeyHook != Q_NULLPTR){
        clearAllBurstTimersAndLockKeys();
        pressedRealKeysList.clear();
        pressedVirtualKeysList.clear();
        m_BurstTimerMap.clear();
        m_BurstKeyUpTimerMap.clear();
        pressedLockKeysList.clear();
        UnhookWindowsHookEx(m_KeyHook);
        m_KeyHook = Q_NULLPTR;
        qInfo("[setKeyUnHook] Normal Key Hook & Mouse Hook Released.");
    }

#ifdef SUPPORT_MOUSE_LL_HOOK
    if (m_MouseHook != Q_NULLPTR) {
        UnhookWindowsHookEx(m_MouseHook);
        m_MouseHook = Q_NULLPTR;
    }
#endif
}

void QKeyMapper_Worker::startBurstTimer(const QString &burstKey, int mappingIndex)
{
    Q_UNUSED(mappingIndex);
    if (true == m_BurstTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("startBurstTimer(): Key \"%s\" BurstTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstTimerMap.remove(burstKey);
    }
    if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("startBurstTimer(): Key \"%s\" BurstKeyUpTimer(%d) already started!!!", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstKeyUpTimerMap.remove(burstKey);
    }
    int burstpressTime = QKeyMapper::getBurstPressTime();
    int burstreleaseTime = QKeyMapper::getBurstReleaseTime();
    int burstTime = burstpressTime + burstreleaseTime;
    int timerID = startTimer(burstTime, Qt::PreciseTimer);
    m_BurstTimerMap.insert(burstKey, timerID);

    int keyupTimerID = startTimer(burstpressTime, Qt::PreciseTimer);
    m_BurstKeyUpTimerMap.insert(burstKey, keyupTimerID);

#ifdef DEBUG_LOGOUT_ON
    qDebug("startBurstTimer(): Key \"%s\", Timer:%d, ID:%d", burstKey.toStdString().c_str(), burstTime, timerID);
#endif
}

void QKeyMapper_Worker::stopBurstTimer(const QString &burstKey, int mappingIndex)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug("stopBurstTimer(): Key \"%s\"", burstKey.toStdString().c_str());
#endif

    if (true == m_BurstTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstTimerMap.value(burstKey);
        killTimer(existTimerID);
        m_BurstTimerMap.remove(burstKey);

        if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
            sendBurstKeyUp(burstKey, true);
#ifdef DEBUG_LOGOUT_ON
            qDebug("stopBurstTimer(): sendBurstKeyUp(\"%s\"), BurstTimer(%d) stoped.", burstKey.toStdString().c_str(), existTimerID);
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug("stopBurstTimer(): Do not need to sendBurstKeyUp(\"%s\"), BurstTimer(%d) stoped.", burstKey.toStdString().c_str(), existTimerID);
#endif
        }

        QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(mappingIndex).Mapping_Keys;
        for (const QString &key : pressedRealKeysList){
            if ((true == mappingKeyList.contains(key))
                && (burstKey != key)
                && (false == pressedVirtualKeysList.contains(key))) {
#ifdef DEBUG_LOGOUT_ON
                qDebug("stopBurstTimer() : RealKey \"%s\" is still pressed down on keyboard, call sendSpecialVirtualKeyDown(\"%s\").", key.toStdString().c_str(), key.toStdString().c_str());
#endif
                sendSpecialVirtualKeyDown(key);
            }
        }
    }

    if (true == m_BurstKeyUpTimerMap.contains(burstKey)) {
        int existTimerID = m_BurstKeyUpTimerMap.value(burstKey);
#ifdef DEBUG_LOGOUT_ON
        qDebug("stopBurstTimer(): Key \"%s\" kill BurstKeyUpTimer(%d)", burstKey.toStdString().c_str(), existTimerID);
#endif
        killTimer(existTimerID);
        m_BurstKeyUpTimerMap.remove(burstKey);
    }
}

LRESULT QKeyMapper_Worker::LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;

    bool returnFlag = false;
    V_KEYCODE vkeycode;
    vkeycode.KeyCode = (quint8)pKeyBoard->vkCode;
    if (LLKHF_EXTENDED == (pKeyBoard->flags & LLKHF_EXTENDED)){
        vkeycode.ExtenedFlag = EXTENED_FLAG_TRUE;
    }
    else{
        vkeycode.ExtenedFlag = EXTENED_FLAG_FALSE;
    }

    QString keycodeString = VirtualKeyCodeMap.key(vkeycode);

//#ifdef DEBUG_LOGOUT_ON
//    qDebug("\"%s\" (0x%02X),  wParam(0x%04X), scanCode(0x%08X), flags(0x%08X), ExtenedFlag(%s)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, wParam, pKeyBoard->scanCode, pKeyBoard->flags, vkeycode.ExtenedFlag==EXTENED_FLAG_TRUE?"true":"false");
//#endif

    if ((false == keycodeString.isEmpty())
        && (WM_KEYDOWN == wParam || WM_KEYUP == wParam)){
#ifdef DEBUG_LOGOUT_ON
        if (pKeyBoard->scanCode != 0){
            if (WM_KEYDOWN == wParam){
                qDebug("RealKey: \"%s\" (0x%02X) KeyDown, scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
            }
            else if (WM_KEYUP == wParam){
                qDebug("RealKey: \"%s\" (0x%02X) KeyUp, scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
            }
            else{
            }
        }
        else {
            if (WM_KEYDOWN == wParam){
                qDebug("VirtualMapKey: \"%s\" (0x%02X) KeyDown, scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
            }
            else if (WM_KEYUP == wParam){
                qDebug("VirtualMapKey: \"%s\" (0x%02X) KeyUp, scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, pKeyBoard->scanCode, pKeyBoard->flags);
            }
            else{
            }
        }
#endif

        if (pKeyBoard->scanCode != 0){
            int findindex = QKeyMapper::findInKeyMappingDataList(keycodeString);

            if (WM_KEYDOWN == wParam){
                if (false == pressedRealKeysList.contains(keycodeString)){
                    if (findindex >=0 && true == QKeyMapper::KeyMappingDataList.at(findindex).Burst) {
                        if (true == QKeyMapper::KeyMappingDataList.at(findindex).Lock) {
                            if (true == QKeyMapper::KeyMappingDataList.at(findindex).LockStatus) {
                                returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                                qDebug("Lock ON & Burst ON(KeyDown) -> Key \"%s\" LockStatus is ON, skip startBurstTimer()!", keycodeString.toStdString().c_str());
#endif
                            }
                            else {
                                emit QKeyMapper_Worker::getInstance()->startBurstTimer_Signal(keycodeString, findindex);
                            }
                        }
                        else {
                            emit QKeyMapper_Worker::getInstance()->startBurstTimer_Signal(keycodeString, findindex);
                        }
                    }
                    pressedRealKeysList.append(keycodeString);
                }

                if (findindex >=0 && true == QKeyMapper::KeyMappingDataList.at(findindex).Lock) {
                    if (true == pressedLockKeysList.contains(keycodeString)){
                        QKeyMapper::KeyMappingDataList[findindex].LockStatus = false;
                        pressedLockKeysList.removeAll(keycodeString);
                        emit QKeyMapper::getInstance()->updateLockStatus_Signal();
#ifdef DEBUG_LOGOUT_ON
                        qDebug("Key \"%s\" KeyDown LockStatus -> OFF", keycodeString.toStdString().c_str());
#endif
                    }
                    else {
                        QKeyMapper::KeyMappingDataList[findindex].LockStatus = true;
                        pressedLockKeysList.append(keycodeString);
                        emit QKeyMapper::getInstance()->updateLockStatus_Signal();
#ifdef DEBUG_LOGOUT_ON
                        qDebug("Key \"%s\" KeyDown LockStatus -> ON", keycodeString.toStdString().c_str());
#endif
                    }
                }
            }
            else if (WM_KEYUP == wParam){
                if (true == pressedRealKeysList.contains(keycodeString)){
                    if (findindex >=0) {
                        if (true == QKeyMapper::KeyMappingDataList.at(findindex).Lock) {
                            /* Lock ON &  Burst ON */
                            if (true == QKeyMapper::KeyMappingDataList.at(findindex).Burst) {
                                if (true == QKeyMapper::KeyMappingDataList.at(findindex).LockStatus) {
                                    returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                                    qDebug("Lock ON & Burst ON(KeyUp) -> Key \"%s\" LockStatus is ON, skip stopBurstTimer()!", keycodeString.toStdString().c_str());
#endif
                                }
                                else {
                                    emit QKeyMapper_Worker::getInstance()->stopBurstTimer_Signal(keycodeString, findindex);
                                    returnFlag = true;
                                }
                            }
                            /* Lock ON &  Burst OFF */
                            else {
                                if (true == QKeyMapper::KeyMappingDataList.at(findindex).LockStatus) {
                                    returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                                    qDebug("Lock ON & Burst OFF -> Key \"%s\" LockStatus is ON, skip KeyUp!", keycodeString.toStdString().c_str());
#endif
                                }
                            }
                        }
                        else {
                            /* Lock OFF &  Burst ON */
                            if (true == QKeyMapper::KeyMappingDataList.at(findindex).Burst) {
                                emit QKeyMapper_Worker::getInstance()->stopBurstTimer_Signal(keycodeString, findindex);
                                returnFlag = true;
                            }
                            /* Lock OFF &  Burst OFF do nothing */
                        }
                    }
                    pressedRealKeysList.removeAll(keycodeString);
                }
            }

            if (true == QKeyMapper::getDisableWinKeyStatus()) {
                if (WM_KEYDOWN == wParam) {
                    if ("D" == keycodeString && pressedRealKeysList.contains("L-Win")) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug("\"L-Win + D\" pressed!");
#endif
                        V_KEYCODE map_vkeycode = VirtualKeyCodeMap.value("L-Win");
                        QKeyMapper_Worker::getInstance()->sendKeyboardInput_Signal(map_vkeycode, KEY_DOWN);
                    }
                }

                if (WM_KEYUP == wParam) {
                    if (("D" == keycodeString && pressedRealKeysList.contains("L-Win"))
                            || ("L-Win" == keycodeString && pressedRealKeysList.contains("D"))) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug("\"L-Win + D\" released by \"%s\" keyup!", keycodeString.toStdString().c_str());
#endif
                        V_KEYCODE map_vkeycode = VirtualKeyCodeMap.value("L-Win");
                        QKeyMapper_Worker::getInstance()->sendKeyboardInput_Signal(map_vkeycode, KEY_UP);
                    }
                }

                if ((WM_KEYDOWN == wParam)
                     || (WM_KEYUP == wParam)) {
                    if (("L-Win" == keycodeString)
                        || ("R-Win" == keycodeString)
                        || ("Application" == keycodeString)) {
                        qDebug("Disable \"%s\" (0x%02X), wParam(0x%04X), scanCode(0x%08X), flags(0x%08X)", keycodeString.toStdString().c_str(), pKeyBoard->vkCode, wParam, pKeyBoard->scanCode, pKeyBoard->flags);
                        returnFlag = true;
                    }
                }
            }

            if (WM_KEYUP == wParam && false == returnFlag){
                if (findindex >=0 && (QKeyMapper::KeyMappingDataList.at(findindex).Original_Key == keycodeString)) {
                }
                else {
                    if (pressedVirtualKeysList.contains(keycodeString)) {
                        returnFlag = true;
#ifdef DEBUG_LOGOUT_ON
                        qDebug("VirtualKey \"%s\" is pressed down, skip RealKey \"%s\" KEYUP!", keycodeString.toStdString().c_str(), keycodeString.toStdString().c_str());
#endif
                    }
                }
            }

            if (false == returnFlag) {
                if (findindex >=0){
                    QStringList mappingKeyList = QKeyMapper::KeyMappingDataList.at(findindex).Mapping_Keys;
                    QString original_key = QKeyMapper::KeyMappingDataList.at(findindex).Original_Key;
                    if (WM_KEYDOWN == wParam){
                        QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_DOWN, original_key, SENDMODE_HOOK);
                        returnFlag = true;
                    }
                    else if (WM_KEYUP == wParam){
                        QKeyMapper_Worker::getInstance()->sendInputKeys_Signal(mappingKeyList, KEY_UP, original_key, SENDMODE_HOOK);
                        returnFlag = true;
                    }
                }
            }
        }
        else {
            if (WM_KEYDOWN == wParam){
                if (false == pressedVirtualKeysList.contains(keycodeString)){
                    pressedVirtualKeysList.append(keycodeString);
                }
            }
            else if (WM_KEYUP == wParam){
                pressedVirtualKeysList.removeAll(keycodeString);
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "LowLevelKeyboardHookProc():" << (wParam == WM_KEYDOWN?"KEY_DOWN":"KEY_UP") << " : pressedVirtualKeysList -> " << pressedVirtualKeysList;
#endif
        }
    }
    else{
#ifdef DEBUG_LOGOUT_ON
        qDebug("UnknownKey (0x%02X) Input, scanCode(0x%08X), wParam(0x%08X), flags(0x%08X)", pKeyBoard->vkCode, pKeyBoard->scanCode, wParam, pKeyBoard->flags);
#endif
    }

    if (true == returnFlag){
#ifdef DEBUG_LOGOUT_ON
        qDebug("LowLevelKeyboardHookProc() -> return TRUE");
#endif
        return (LRESULT)TRUE;
    }
    else{
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }
}

LRESULT QKeyMapper_Worker::LowLevelMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0) {
        return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
    }

    MSLLHOOKSTRUCT *pMouse = (MSLLHOOKSTRUCT *)lParam;

    if ((wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP)
        || (wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP)
        || (wParam == WM_MBUTTONDOWN || wParam == WM_MBUTTONUP)) {
        int keyupdown;
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN) {
            keyupdown = KEY_DOWN;
        }
        else {
            keyupdown = KEY_UP;
        }
        ULONG_PTR extraInfo = pMouse->dwExtraInfo;
        Q_UNUSED(extraInfo);

        if (true == MouseButtonNameMap.contains(wParam)) {
            QString keycodeString = MouseButtonNameMap.value(wParam);
            if(VIRTUAL_MOUSE_CLICK == extraInfo) {
                if (KEY_DOWN == keyupdown) {
                    if (false == pressedVirtualKeysList.contains(keycodeString)){
                        pressedVirtualKeysList.append(keycodeString);
                    }
                }
                else {
                    pressedVirtualKeysList.removeAll(keycodeString);
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug("Virtual \"%s\" %s", MouseButtonNameMap.value(wParam).toStdString().c_str(), (keyupdown == KEY_DOWN?"Button Down":"Button Up"));
#endif
            }
            else {
                if (KEY_DOWN == wParam){
                    if (false == pressedRealKeysList.contains(keycodeString)){
                        pressedRealKeysList.append(keycodeString);
                    }
                }
                else {
                    pressedRealKeysList.removeAll(keycodeString);
                }
#ifdef DEBUG_LOGOUT_ON
                qDebug("Real \"%s\" %s", MouseButtonNameMap.value(wParam).toStdString().c_str(), (keyupdown == KEY_DOWN?"Button Down":"Button Up"));
#endif
//#ifdef DEBUG_LOGOUT_ON
//                qDebug("LowLevelMouseHookProc() -> Name:%s, ID:0x%08X", QThread::currentThread()->objectName().toLatin1().constData(), QThread::currentThreadId());
//#endif
            }
        }
    }

    return CallNextHookEx(Q_NULLPTR, nCode, wParam, lParam);
}

void QKeyMapper_Worker::initVirtualKeyCodeMap()
{
    // US 104 Keyboard Main Area
    // Row 1
    VirtualKeyCodeMap.insert        ("`~",          V_KEYCODE(VK_OEM_3,         EXTENED_FLAG_FALSE));   // 0xC0
    VirtualKeyCodeMap.insert        ("1!",          V_KEYCODE(VK_1,             EXTENED_FLAG_FALSE));   // 0x31
    VirtualKeyCodeMap.insert        ("2@",          V_KEYCODE(VK_2,             EXTENED_FLAG_FALSE));   // 0x32
    VirtualKeyCodeMap.insert        ("3#",          V_KEYCODE(VK_3,             EXTENED_FLAG_FALSE));   // 0x33
    VirtualKeyCodeMap.insert        ("4$",          V_KEYCODE(VK_4,             EXTENED_FLAG_FALSE));   // 0x34
    VirtualKeyCodeMap.insert        ("5%",          V_KEYCODE(VK_5,             EXTENED_FLAG_FALSE));   // 0x35
    VirtualKeyCodeMap.insert        ("6^",          V_KEYCODE(VK_6,             EXTENED_FLAG_FALSE));   // 0x36
    VirtualKeyCodeMap.insert        ("7&",          V_KEYCODE(VK_7,             EXTENED_FLAG_FALSE));   // 0x37
    VirtualKeyCodeMap.insert        ("8*",          V_KEYCODE(VK_8,             EXTENED_FLAG_FALSE));   // 0x38
    VirtualKeyCodeMap.insert        ("9(",          V_KEYCODE(VK_9,             EXTENED_FLAG_FALSE));   // 0x39
    VirtualKeyCodeMap.insert        ("0)",          V_KEYCODE(VK_0,             EXTENED_FLAG_FALSE));   // 0x30
    VirtualKeyCodeMap.insert        ("-_",          V_KEYCODE(VK_OEM_MINUS,     EXTENED_FLAG_FALSE));   // 0xBD
    VirtualKeyCodeMap.insert        ("=+",          V_KEYCODE(VK_OEM_PLUS,      EXTENED_FLAG_FALSE));   // 0xBB
    VirtualKeyCodeMap.insert        ("Backspace",   V_KEYCODE(VK_BACK,          EXTENED_FLAG_FALSE));   // 0x08
    // Row 2
    VirtualKeyCodeMap.insert        ("Tab",         V_KEYCODE(VK_TAB,           EXTENED_FLAG_FALSE));   // 0x09
    VirtualKeyCodeMap.insert        ("Q",           V_KEYCODE(VK_Q,             EXTENED_FLAG_FALSE));   // 0x51
    VirtualKeyCodeMap.insert        ("W",           V_KEYCODE(VK_W,             EXTENED_FLAG_FALSE));   // 0x57
    VirtualKeyCodeMap.insert        ("E",           V_KEYCODE(VK_E,             EXTENED_FLAG_FALSE));   // 0x45
    VirtualKeyCodeMap.insert        ("R",           V_KEYCODE(VK_R,             EXTENED_FLAG_FALSE));   // 0x52
    VirtualKeyCodeMap.insert        ("T",           V_KEYCODE(VK_T,             EXTENED_FLAG_FALSE));   // 0x54
    VirtualKeyCodeMap.insert        ("Y",           V_KEYCODE(VK_Y,             EXTENED_FLAG_FALSE));   // 0x59
    VirtualKeyCodeMap.insert        ("U",           V_KEYCODE(VK_U,             EXTENED_FLAG_FALSE));   // 0x55
    VirtualKeyCodeMap.insert        ("I",           V_KEYCODE(VK_I,             EXTENED_FLAG_FALSE));   // 0x49
    VirtualKeyCodeMap.insert        ("O",           V_KEYCODE(VK_O,             EXTENED_FLAG_FALSE));   // 0x4F
    VirtualKeyCodeMap.insert        ("P",           V_KEYCODE(VK_P,             EXTENED_FLAG_FALSE));   // 0x50
    VirtualKeyCodeMap.insert        ("[{",          V_KEYCODE(VK_OEM_4,         EXTENED_FLAG_FALSE));   // 0xDB
    VirtualKeyCodeMap.insert        ("]}",          V_KEYCODE(VK_OEM_6,         EXTENED_FLAG_FALSE));   // 0xDD
    VirtualKeyCodeMap.insert        ("\\|",         V_KEYCODE(VK_OEM_5,         EXTENED_FLAG_FALSE));   // 0xDC
    // Row 3
    VirtualKeyCodeMap.insert        ("CapsLock",    V_KEYCODE(VK_CAPITAL,       EXTENED_FLAG_FALSE));   // 0x14
    VirtualKeyCodeMap.insert        ("A",           V_KEYCODE(VK_A,             EXTENED_FLAG_FALSE));   // 0x41
    VirtualKeyCodeMap.insert        ("S",           V_KEYCODE(VK_S,             EXTENED_FLAG_FALSE));   // 0x53
    VirtualKeyCodeMap.insert        ("D",           V_KEYCODE(VK_D,             EXTENED_FLAG_FALSE));   // 0x44
    VirtualKeyCodeMap.insert        ("F",           V_KEYCODE(VK_F,             EXTENED_FLAG_FALSE));   // 0x46
    VirtualKeyCodeMap.insert        ("G",           V_KEYCODE(VK_G,             EXTENED_FLAG_FALSE));   // 0x47
    VirtualKeyCodeMap.insert        ("H",           V_KEYCODE(VK_H,             EXTENED_FLAG_FALSE));   // 0x48
    VirtualKeyCodeMap.insert        ("J",           V_KEYCODE(VK_J,             EXTENED_FLAG_FALSE));   // 0x4A
    VirtualKeyCodeMap.insert        ("K",           V_KEYCODE(VK_K,             EXTENED_FLAG_FALSE));   // 0x4B
    VirtualKeyCodeMap.insert        ("L",           V_KEYCODE(VK_L,             EXTENED_FLAG_FALSE));   // 0x4C
    VirtualKeyCodeMap.insert        (";:",          V_KEYCODE(VK_OEM_1,         EXTENED_FLAG_FALSE));   // 0xBA
    VirtualKeyCodeMap.insert        ("'\"",         V_KEYCODE(VK_OEM_7,         EXTENED_FLAG_FALSE));   // 0xDE
    VirtualKeyCodeMap.insert        ("Enter",       V_KEYCODE(VK_RETURN,        EXTENED_FLAG_FALSE));   // 0x0D
    // Row 4
    VirtualKeyCodeMap.insert        ("L-Shift",     V_KEYCODE(VK_LSHIFT,        EXTENED_FLAG_FALSE));   // 0xA0
    VirtualKeyCodeMap.insert        ("Z",           V_KEYCODE(VK_Z,             EXTENED_FLAG_FALSE));   // 0x5A
    VirtualKeyCodeMap.insert        ("X",           V_KEYCODE(VK_X,             EXTENED_FLAG_FALSE));   // 0x58
    VirtualKeyCodeMap.insert        ("C",           V_KEYCODE(VK_C,             EXTENED_FLAG_FALSE));   // 0x43
    VirtualKeyCodeMap.insert        ("V",           V_KEYCODE(VK_V,             EXTENED_FLAG_FALSE));   // 0x56
    VirtualKeyCodeMap.insert        ("B",           V_KEYCODE(VK_B,             EXTENED_FLAG_FALSE));   // 0x42
    VirtualKeyCodeMap.insert        ("N",           V_KEYCODE(VK_N,             EXTENED_FLAG_FALSE));   // 0x4E
    VirtualKeyCodeMap.insert        ("M",           V_KEYCODE(VK_M,             EXTENED_FLAG_FALSE));   // 0x4D
    VirtualKeyCodeMap.insert        (",<",          V_KEYCODE(VK_OEM_COMMA,     EXTENED_FLAG_FALSE));   // 0xBC
    VirtualKeyCodeMap.insert        (".>",          V_KEYCODE(VK_OEM_PERIOD,    EXTENED_FLAG_FALSE));   // 0xBE
    VirtualKeyCodeMap.insert        ("/?",          V_KEYCODE(VK_OEM_2,         EXTENED_FLAG_FALSE));   // 0xBF
    VirtualKeyCodeMap.insert        ("R-Shift",     V_KEYCODE(VK_RSHIFT,        EXTENED_FLAG_TRUE ));   // 0xA1 + E
    // Row 5
    VirtualKeyCodeMap.insert        ("L-Ctrl",      V_KEYCODE(VK_LCONTROL,      EXTENED_FLAG_FALSE));   // 0xA2
    VirtualKeyCodeMap.insert        ("L-Win",       V_KEYCODE(VK_LWIN,          EXTENED_FLAG_TRUE ));   // 0x5B + E
    VirtualKeyCodeMap.insert        ("L-Alt",       V_KEYCODE(VK_LMENU,         EXTENED_FLAG_FALSE));   // 0xA4
    VirtualKeyCodeMap.insert        ("Space",       V_KEYCODE(VK_SPACE,         EXTENED_FLAG_FALSE));   // 0x20
    VirtualKeyCodeMap.insert        ("R-Alt",       V_KEYCODE(VK_RMENU,         EXTENED_FLAG_TRUE ));   // 0xA5 + E
    VirtualKeyCodeMap.insert        ("Application", V_KEYCODE(VK_APPS,          EXTENED_FLAG_TRUE ));   // 0x5D + E
    VirtualKeyCodeMap.insert        ("R-Ctrl",      V_KEYCODE(VK_RCONTROL,      EXTENED_FLAG_TRUE ));   // 0xA3 + E
    VirtualKeyCodeMap.insert        ("R-Win",       V_KEYCODE(VK_RWIN,          EXTENED_FLAG_TRUE ));   // 0x5C + E
    // Old special keys
    VirtualKeyCodeMap.insert        ("Shift",       V_KEYCODE(VK_SHIFT,         EXTENED_FLAG_FALSE));   // 0x10
    VirtualKeyCodeMap.insert        ("Ctrl",        V_KEYCODE(VK_CONTROL,       EXTENED_FLAG_FALSE));   // 0x11
    VirtualKeyCodeMap.insert        ("Alt",         V_KEYCODE(VK_MENU,          EXTENED_FLAG_FALSE));   // 0x12

    // Function Keys
    VirtualKeyCodeMap.insert        ("Esc",         V_KEYCODE(VK_ESCAPE,        EXTENED_FLAG_FALSE));   // 0x1B
    VirtualKeyCodeMap.insert        ("F1",          V_KEYCODE(VK_F1,            EXTENED_FLAG_FALSE));   // 0x70
    VirtualKeyCodeMap.insert        ("F2",          V_KEYCODE(VK_F2,            EXTENED_FLAG_FALSE));   // 0x71
    VirtualKeyCodeMap.insert        ("F3",          V_KEYCODE(VK_F3,            EXTENED_FLAG_FALSE));   // 0x72
    VirtualKeyCodeMap.insert        ("F4",          V_KEYCODE(VK_F4,            EXTENED_FLAG_FALSE));   // 0x73
    VirtualKeyCodeMap.insert        ("F5",          V_KEYCODE(VK_F5,            EXTENED_FLAG_FALSE));   // 0x74
    VirtualKeyCodeMap.insert        ("F6",          V_KEYCODE(VK_F6,            EXTENED_FLAG_FALSE));   // 0x75
    VirtualKeyCodeMap.insert        ("F7",          V_KEYCODE(VK_F7,            EXTENED_FLAG_FALSE));   // 0x76
    VirtualKeyCodeMap.insert        ("F8",          V_KEYCODE(VK_F8,            EXTENED_FLAG_FALSE));   // 0x77
    VirtualKeyCodeMap.insert        ("F9",          V_KEYCODE(VK_F9,            EXTENED_FLAG_FALSE));   // 0x78
    VirtualKeyCodeMap.insert        ("F10",         V_KEYCODE(VK_F10,           EXTENED_FLAG_FALSE));   // 0x79
    VirtualKeyCodeMap.insert        ("F11",         V_KEYCODE(VK_F11,           EXTENED_FLAG_FALSE));   // 0x7A
    VirtualKeyCodeMap.insert        ("F12",         V_KEYCODE(VK_F12,           EXTENED_FLAG_FALSE));   // 0x7B
    VirtualKeyCodeMap.insert        ("F13",         V_KEYCODE(VK_F13,           EXTENED_FLAG_FALSE));   // 0x7C
    VirtualKeyCodeMap.insert        ("F14",         V_KEYCODE(VK_F14,           EXTENED_FLAG_FALSE));   // 0x7D
    VirtualKeyCodeMap.insert        ("F15",         V_KEYCODE(VK_F15,           EXTENED_FLAG_FALSE));   // 0x7E
    VirtualKeyCodeMap.insert        ("F16",         V_KEYCODE(VK_F16,           EXTENED_FLAG_FALSE));   // 0x7F
    VirtualKeyCodeMap.insert        ("F17",         V_KEYCODE(VK_F17,           EXTENED_FLAG_FALSE));   // 0x80
    VirtualKeyCodeMap.insert        ("F18",         V_KEYCODE(VK_F18,           EXTENED_FLAG_FALSE));   // 0x81
    VirtualKeyCodeMap.insert        ("F19",         V_KEYCODE(VK_F19,           EXTENED_FLAG_FALSE));   // 0x82
    VirtualKeyCodeMap.insert        ("F20",         V_KEYCODE(VK_F20,           EXTENED_FLAG_FALSE));   // 0x83
    VirtualKeyCodeMap.insert        ("F21",         V_KEYCODE(VK_F21,           EXTENED_FLAG_FALSE));   // 0x84
    VirtualKeyCodeMap.insert        ("F22",         V_KEYCODE(VK_F22,           EXTENED_FLAG_FALSE));   // 0x85
    VirtualKeyCodeMap.insert        ("F23",         V_KEYCODE(VK_F23,           EXTENED_FLAG_FALSE));   // 0x86
    VirtualKeyCodeMap.insert        ("F24",         V_KEYCODE(VK_F24,           EXTENED_FLAG_FALSE));   // 0x87

    VirtualKeyCodeMap.insert        ("PrintScrn",   V_KEYCODE(VK_SNAPSHOT,      EXTENED_FLAG_TRUE ));   // 0x2C + E
    VirtualKeyCodeMap.insert        ("ScrollLock",  V_KEYCODE(VK_SCROLL,        EXTENED_FLAG_FALSE));   // 0x91
    VirtualKeyCodeMap.insert        ("Pause",       V_KEYCODE(VK_PAUSE,         EXTENED_FLAG_FALSE));   // 0x13

    VirtualKeyCodeMap.insert        ("Insert",      V_KEYCODE(VK_INSERT,        EXTENED_FLAG_TRUE ));   // 0x2D + E
    VirtualKeyCodeMap.insert        ("Delete",      V_KEYCODE(VK_DELETE,        EXTENED_FLAG_TRUE ));   // 0x2E + E
    VirtualKeyCodeMap.insert        ("Home",        V_KEYCODE(VK_HOME,          EXTENED_FLAG_TRUE ));   // 0x24 + E
    VirtualKeyCodeMap.insert        ("End",         V_KEYCODE(VK_END,           EXTENED_FLAG_TRUE ));   // 0x23 + E
    VirtualKeyCodeMap.insert        ("PageUp",      V_KEYCODE(VK_PRIOR,         EXTENED_FLAG_TRUE ));   // 0x21 + E
    VirtualKeyCodeMap.insert        ("PageDown",    V_KEYCODE(VK_NEXT,          EXTENED_FLAG_TRUE ));   // 0x22 + E

    VirtualKeyCodeMap.insert        ("Up",          V_KEYCODE(VK_UP,            EXTENED_FLAG_TRUE ));   // 0x26 + E
    VirtualKeyCodeMap.insert        ("Down",        V_KEYCODE(VK_DOWN,          EXTENED_FLAG_TRUE ));   // 0x28 + E
    VirtualKeyCodeMap.insert        ("Left",        V_KEYCODE(VK_LEFT,          EXTENED_FLAG_TRUE ));   // 0x25 + E
    VirtualKeyCodeMap.insert        ("Right",       V_KEYCODE(VK_RIGHT,         EXTENED_FLAG_TRUE ));   // 0x27 + E

    //NumberPad Keys
    VirtualKeyCodeMap.insert        ("NumLock",     V_KEYCODE(VK_NUMLOCK,       EXTENED_FLAG_TRUE ));   // 0x90 + E
    VirtualKeyCodeMap.insert        ("Num /",       V_KEYCODE(VK_DIVIDE,        EXTENED_FLAG_TRUE ));   // 0x6F + E
    VirtualKeyCodeMap.insert        ("Num *",       V_KEYCODE(VK_MULTIPLY,      EXTENED_FLAG_FALSE));   // 0x6A
    VirtualKeyCodeMap.insert        ("Num -",       V_KEYCODE(VK_SUBTRACT,      EXTENED_FLAG_FALSE));   // 0x6D
    VirtualKeyCodeMap.insert        ("Num +",       V_KEYCODE(VK_ADD,           EXTENED_FLAG_FALSE));   // 0x6B
    VirtualKeyCodeMap.insert        ("Num .",       V_KEYCODE(VK_DECIMAL,       EXTENED_FLAG_FALSE));   // 0x6E
    VirtualKeyCodeMap.insert        ("Num 0",       V_KEYCODE(VK_NUMPAD0,       EXTENED_FLAG_FALSE));   // 0x60
    VirtualKeyCodeMap.insert        ("Num 1",       V_KEYCODE(VK_NUMPAD1,       EXTENED_FLAG_FALSE));   // 0x61
    VirtualKeyCodeMap.insert        ("Num 2",       V_KEYCODE(VK_NUMPAD2,       EXTENED_FLAG_FALSE));   // 0x62
    VirtualKeyCodeMap.insert        ("Num 3",       V_KEYCODE(VK_NUMPAD3,       EXTENED_FLAG_FALSE));   // 0x63
    VirtualKeyCodeMap.insert        ("Num 4",       V_KEYCODE(VK_NUMPAD4,       EXTENED_FLAG_FALSE));   // 0x64
    VirtualKeyCodeMap.insert        ("Num 5",       V_KEYCODE(VK_NUMPAD5,       EXTENED_FLAG_FALSE));   // 0x65
    VirtualKeyCodeMap.insert        ("Num 6",       V_KEYCODE(VK_NUMPAD6,       EXTENED_FLAG_FALSE));   // 0x66
    VirtualKeyCodeMap.insert        ("Num 7",       V_KEYCODE(VK_NUMPAD7,       EXTENED_FLAG_FALSE));   // 0x67
    VirtualKeyCodeMap.insert        ("Num 8",       V_KEYCODE(VK_NUMPAD8,       EXTENED_FLAG_FALSE));   // 0x68
    VirtualKeyCodeMap.insert        ("Num 9",       V_KEYCODE(VK_NUMPAD9,       EXTENED_FLAG_FALSE));   // 0x69
    VirtualKeyCodeMap.insert        ("Num Enter",   V_KEYCODE(VK_RETURN,        EXTENED_FLAG_TRUE ));   // 0x0D + E
    //NumLock Off NumberPadKeys
    VirtualKeyCodeMap.insertMulti   ("Num .",       V_KEYCODE(VK_DELETE,        EXTENED_FLAG_FALSE));   // 0x2E
    VirtualKeyCodeMap.insertMulti   ("Num 0",       V_KEYCODE(VK_INSERT,        EXTENED_FLAG_FALSE));   // 0x2D
    VirtualKeyCodeMap.insertMulti   ("Num 1",       V_KEYCODE(VK_END,           EXTENED_FLAG_FALSE));   // 0x23
    VirtualKeyCodeMap.insertMulti   ("Num 2",       V_KEYCODE(VK_DOWN,          EXTENED_FLAG_FALSE));   // 0x28
    VirtualKeyCodeMap.insertMulti   ("Num 3",       V_KEYCODE(VK_NEXT,          EXTENED_FLAG_FALSE));   // 0x22
    VirtualKeyCodeMap.insertMulti   ("Num 4",       V_KEYCODE(VK_LEFT,          EXTENED_FLAG_FALSE));   // 0x25
    VirtualKeyCodeMap.insertMulti   ("Num 5",       V_KEYCODE(VK_CLEAR,         EXTENED_FLAG_FALSE));   // 0x0C
    VirtualKeyCodeMap.insertMulti   ("Num 6",       V_KEYCODE(VK_RIGHT,         EXTENED_FLAG_FALSE));   // 0x27
    VirtualKeyCodeMap.insertMulti   ("Num 7",       V_KEYCODE(VK_HOME,          EXTENED_FLAG_FALSE));   // 0x24
    VirtualKeyCodeMap.insertMulti   ("Num 8",       V_KEYCODE(VK_UP,            EXTENED_FLAG_FALSE));   // 0x26
    VirtualKeyCodeMap.insertMulti   ("Num 9",       V_KEYCODE(VK_PRIOR,         EXTENED_FLAG_FALSE));   // 0x21
}

void QKeyMapper_Worker::initVirtualMouseButtonMap()
{
    VirtualMouseButtonMap.insert("L-Mouse",     V_MOUSECODE(MOUSEEVENTF_LEFTDOWN,       MOUSEEVENTF_LEFTUP  ));   // Left Mouse Button
    VirtualMouseButtonMap.insert("R-Mouse",     V_MOUSECODE(MOUSEEVENTF_RIGHTDOWN,      MOUSEEVENTF_RIGHTUP ));   // Right Mouse Button
    VirtualMouseButtonMap.insert("M-Mouse",     V_MOUSECODE(MOUSEEVENTF_MIDDLEDOWN,     MOUSEEVENTF_MIDDLEUP));   // Middle Mouse Button

    MouseButtonNameMap.insert(WM_LBUTTONDOWN,   "L-Mouse");
    MouseButtonNameMap.insert(WM_LBUTTONUP,     "L-Mouse");
    MouseButtonNameMap.insert(WM_RBUTTONDOWN,   "R-Mouse");
    MouseButtonNameMap.insert(WM_RBUTTONUP,     "R-Mouse");
    MouseButtonNameMap.insert(WM_MBUTTONDOWN,   "M-Mouse");
    MouseButtonNameMap.insert(WM_MBUTTONUP,     "M-Mouse");
}

void QKeyMapper_Worker::clearAllBurstTimersAndLockKeys()
{
    QList<QString> burstKeyUpKeys = m_BurstKeyUpTimerMap.keys();
    for (const QString &key : burstKeyUpKeys) {
        int timerID = m_BurstTimerMap.value(key, 0);
        if (timerID > 0) {
            killTimer(timerID);
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qWarning("startBurstTimer(): Key \"%s\" could not find in m_BurstKeyUpTimerMap!!!", key.toStdString().c_str());
#endif
        }
    }

    QList<QString> burstKeys = m_BurstTimerMap.keys();
    for (const QString &key : burstKeys) {
        int timerID = m_BurstTimerMap.value(key, 0);
        if (timerID > 0) {
            int findindex = QKeyMapper::findInKeyMappingDataList(key);
            if (findindex >= 0) {
                if (true == QKeyMapper::KeyMappingDataList.at(findindex).Lock) {
                    if (true == pressedLockKeysList.contains(key)){
                        QKeyMapper::KeyMappingDataList[findindex].LockStatus = false;
                        pressedLockKeysList.removeAll(key);
#ifdef DEBUG_LOGOUT_ON
                        qDebug("clearAllBurstTimersAndLockKeys() : Key \"%s\" KeyDown LockStatus -> OFF", key.toStdString().c_str());
#endif
                    }
                }

                emit stopBurstTimer_Signal(key, findindex);
            }
        }
    }

    for (int index = 0; index < QKeyMapper::KeyMappingDataList.size(); index++) {
        QKeyMapper::KeyMappingDataList[index].LockStatus = false;
    }
}
