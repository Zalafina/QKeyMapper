#include "interception_worker.h"

InterceptionContext Interception_Worker::s_InterceptionContext = Q_NULLPTR;
QAtomicBool Interception_Worker::s_InterceptStart = QAtomicBool();
QList<InputDevice> Interception_Worker::KeyboardDeviceList = QList<InputDevice>();
QList<InputDevice> Interception_Worker::MouseDeviceList = QList<InputDevice>();

Interception_Worker::Interception_Worker(QObject *parent) :
    QObject{parent}
{
    if (doLoadInterception()) {
        (void)getKeyboardDeviceList();
        (void)getMouseDeviceList();
        startInterception();
    }
}

Interception_Worker::~Interception_Worker()
{
    doUnloadInterception();
}

void Interception_Worker::InterceptionThreadStarted()
{
#ifdef DEBUG_LOGOUT_ON
    QString threadIdStr = QString("0x%1").arg(QString::number((qulonglong)QThread::currentThreadId(), 16).toUpper(), 8, '0');
    qDebug().nospace().noquote() << "[InterceptionThreadStarted] ThreadName:" << QThread::currentThread()->objectName() << ", ThreadID:" << threadIdStr;
#endif

    if (s_InterceptionContext == Q_NULLPTR) {
        return;
    }

    InterceptionDevice device;
    InterceptionKeyStroke stroke;

    if (s_InterceptStart) {
        interception_set_filter(s_InterceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);
#ifdef QT_DEBUG
        if (!IsDebuggerPresent()) {
            interception_set_filter(s_InterceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_ALL);
        }
#else
        interception_set_filter(s_InterceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_ALL);
#endif
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[KeyInterceptionWorker] Start intercept Keyboard&Mouse devices.";
#endif
    }

    while(s_InterceptStart && interception_receive(s_InterceptionContext, device = interception_wait(s_InterceptionContext), (InterceptionStroke *)&stroke, 1) > 0)
    {
        if(interception_is_mouse(device))
        {
            // int index = device - INTERCEPTION_MOUSE(0);
            // if (QKeyMapper::MouseDeviceList.at(index)) {
                unsigned int device_index = 1;
                InterceptionMouseStroke &mstroke = *(InterceptionMouseStroke *) &stroke;
                device_index += device - INTERCEPTION_KEYBOARD(0);
                mstroke.information = INTERCEPTION_EXTRA_INFO + device_index;
                interception_send(s_InterceptionContext, device, (InterceptionStroke *)&stroke, 1);
            // }
        }
        else
        {
            int index = device - INTERCEPTION_KEYBOARD(0);
            if (false == KeyboardDeviceList.isEmpty() && KeyboardDeviceList.at(index).disabled) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[KeyInterceptionWorker] Keyboard[" << device << "] is disabled!";
#endif
            }
            else {
                unsigned int device_index = 1;
                InterceptionKeyStroke &kstroke = *(InterceptionKeyStroke *) &stroke;
                // unsigned int ori_information = kstroke.information;
                device_index += device - INTERCEPTION_KEYBOARD(0);
                kstroke.information = INTERCEPTION_EXTRA_INFO + device_index;
                interception_send(s_InterceptionContext, device, (InterceptionStroke *)&stroke, 1);
// #ifdef DEBUG_LOGOUT_ON
//                 QString ori_extraInfoStr = QString("0x%1").arg(QString::number(ori_information, 16).toUpper(), 8, '0');
//                 QString extraInfoStr = QString("0x%1").arg(QString::number(kstroke.information, 16).toUpper(), 8, '0');
//                 qDebug().nospace() << "[KeyInterceptionWorker] Keyboard[" << device << "] extraInfo (" << ori_extraInfoStr << " -> " << extraInfoStr << ")";
// #endif
            }
        }
    }

    interception_set_filter(s_InterceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_NONE);
    interception_set_filter(s_InterceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_NONE);
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[KeyInterceptionWorker] Stop intercept Keyboard&Mouse devices.";
#endif
}

bool Interception_Worker::doLoadInterception()
{
    bool result = false;
    s_InterceptionContext = interception_create_context();
    if (s_InterceptionContext == Q_NULLPTR) {
        result = false;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[Interception_Worker] interception_create_context Failed!!!";
#endif
    }
    else {
        result = true;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[Interception_Worker] interception_create_context Success.";
#endif
    }

    return result;
}

void Interception_Worker::doUnloadInterception()
{
    if (s_InterceptionContext != Q_NULLPTR) {
        interception_set_filter(s_InterceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_NONE);
        interception_set_filter(s_InterceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_NONE);
        interception_destroy_context(s_InterceptionContext);
    }
}

void Interception_Worker::startInterception()
{
    s_InterceptStart = true;
}

void Interception_Worker::stopInterception()
{
    s_InterceptStart = false;
}

bool Interception_Worker::isInterceptionDriverFileExist()
{
    wchar_t system32Path[MAX_PATH];
    UINT result = GetSystemDirectory(system32Path, MAX_PATH);
    if (result == 0)
    {
        return false;
    }

    QString driverFilePath = QString::fromWCharArray(system32Path) + QString("\\drivers\\keyboard.sys");
    DWORD dummy;
    DWORD size = GetFileVersionInfoSize((LPCWSTR)driverFilePath.utf16(), &dummy);
    if (size == 0) {
        return false;
    }

    QByteArray data(size, 0);
    if (!GetFileVersionInfo((LPCWSTR)driverFilePath.utf16(), 0, size, data.data())) {
        return false;
    }

    void *value = nullptr;
    UINT length;
    QString fileDescription;
    QString productName;
    QString originalFilename;
    if (!VerQueryValue(data.data(), L"\\StringFileInfo\\040904b0\\FileDescription", &value, &length)) {
        return false;
    }
    fileDescription = QString::fromUtf16((ushort *)value, length);
    fileDescription.remove(QChar('\0'));
    if (!VerQueryValue(data.data(), L"\\StringFileInfo\\040904b0\\ProductName", &value, &length)) {
        return false;
    }
    productName = QString::fromUtf16((ushort *)value, length);
    productName.remove(QChar('\0'));
    if (!VerQueryValue(data.data(), L"\\StringFileInfo\\040904b0\\OriginalFilename", &value, &length)) {
        return false;
    }
    originalFilename = QString::fromUtf16((ushort *)value, length);
    originalFilename.remove(QChar('\0'));

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[isInterceptionDriverFileExist]" << "FileDescription:" << fileDescription;
    qDebug() << "[isInterceptionDriverFileExist]" << "ProductName:" << productName;
    qDebug() << "[isInterceptionDriverFileExist]" << "originalFilename:" << originalFilename;
#endif

    if (fileDescription == "Keyboard Upper Filter Driver"
        && productName == "Interception"
        && originalFilename == "keyboard.sys") {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[isInterceptionDriverFileExist]" << "Interception driver file exists.";
#endif
        return true;
    }

    return false;
}

Interception_Worker::Interception_State Interception_Worker::getInterceptionState()
{
    if (s_InterceptionContext != Q_NULLPTR) {
        return INTERCEPTION_AVAILABLE;
    }
    else {
        if (isInterceptionDriverFileExist()) {
            return INTERCEPTION_REBOOTREQUIRED;
        }
        else {
            return INTERCEPTION_UNAVAILABLE;
        }
    }
}

QString Interception_Worker::getDeviceNameByHardwareID(const QString &hardwareID)
{
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        return QString();
    }

    SP_DEVINFO_DATA deviceInfoData;
    ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    DWORD index = 0;
    while (SetupDiEnumDeviceInfo(deviceInfoSet, index, &deviceInfoData))
    {
        DWORD dataType;
        WCHAR buffer[MAX_PATH];
        DWORD bufferSize = sizeof(buffer);

        if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID,
                                             &dataType, reinterpret_cast<PBYTE>(buffer), bufferSize, nullptr))
        {
            QString hardwareIdStr = QString::fromWCharArray(buffer);
            if (hardwareIdStr == hardwareID)
            {
                if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC,
                                                     &dataType, reinterpret_cast<PBYTE>(buffer), bufferSize, nullptr))
                {
                    QString deviceName = QString::fromWCharArray(buffer);
                    SetupDiDestroyDeviceInfoList(deviceInfoSet);
                    return deviceName;
                }
            }
        }

        index++;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return QString();
}

InterceptionContext Interception_Worker::getInterceptionContext()
{
    return s_InterceptionContext;
}

QList<InputDevice> Interception_Worker::getKeyboardDeviceList()
{
    QList<InputDevice> devicelist;
    if (s_InterceptionContext == Q_NULLPTR) {
        return devicelist;
    }

    WCHAR hardware_id[MAX_PATH];
    for(InterceptionDevice device = INTERCEPTION_KEYBOARD(0); device <= INTERCEPTION_KEYBOARD(INTERCEPTION_MAX_KEYBOARD - 1); ++device)
    {
        InputDevice input_device = InputDevice();
        size_t length = interception_get_hardware_id(s_InterceptionContext, device, hardware_id, sizeof(hardware_id));

        if(length > 0 && length < sizeof(hardware_id)) {
            QString hardware_id_str = QString::fromWCharArray(hardware_id);
            QString devicename = getDeviceNameByHardwareID(hardware_id_str);
            input_device.hardwareid = hardware_id_str;
            input_device.devicename = devicename;
            input_device.device = device;
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[getKeyboardDeviceList] Keyboard[" << device << "] -> HardwareID: " << hardware_id_str << ", DeviceName: " << devicename;
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[getKeyboardDeviceList] No Keyboard[" << device << "]";
#endif
        }

        if (!KeyboardDeviceList.isEmpty()) {
            input_device.disabled = KeyboardDeviceList[device-INTERCEPTION_KEYBOARD(0)].disabled;
        }
        devicelist.append(input_device);
    }

    KeyboardDeviceList = devicelist;

    return KeyboardDeviceList;
}

QList<InputDevice> Interception_Worker::getMouseDeviceList()
{
    QList<InputDevice> devicelist;
    if (s_InterceptionContext == Q_NULLPTR) {
        return devicelist;
    }

    WCHAR hardware_id[MAX_PATH];
    for(InterceptionDevice device = INTERCEPTION_MOUSE(0); device <= INTERCEPTION_MOUSE(INTERCEPTION_MAX_MOUSE - 1); ++device)
    {
        InputDevice input_device = InputDevice();
        size_t length = interception_get_hardware_id(s_InterceptionContext, device, hardware_id, sizeof(hardware_id));
        int mouse_index = device - INTERCEPTION_MOUSE(0) + 1;
        Q_UNUSED(mouse_index);
        if(length > 0 && length < sizeof(hardware_id)) {
            QString hardware_id_str = QString::fromWCharArray(hardware_id);
            QString devicename = getDeviceNameByHardwareID(hardware_id_str);
            input_device.hardwareid = hardware_id_str;
            input_device.devicename = devicename;
            input_device.device = device;
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[getMouseDeviceList] Mouse[" << mouse_index << "] -> HardwareID: " << hardware_id_str << ", DeviceName: " << devicename;
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[getMouseDeviceList] No Mouse[" << mouse_index << "]";
#endif
        }

        if (!MouseDeviceList.isEmpty()) {
            input_device.disabled = MouseDeviceList[device - INTERCEPTION_MOUSE(0)].disabled;
        }
        devicelist.append(input_device);
    }

    MouseDeviceList = devicelist;

    return MouseDeviceList;
}

QString Interception_Worker::getHardwareId(InterceptionDevice device)
{
    QString hardware_id_str;
    WCHAR hardware_id[MAX_PATH];
    size_t length = interception_get_hardware_id(s_InterceptionContext, device, hardware_id, sizeof(hardware_id));
    if(length > 0 && length < sizeof(hardware_id)) {
        hardware_id_str = QString::fromWCharArray(hardware_id);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[getHardwareId] Device[" << device << "] -> HardwareID: " << hardware_id_str;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[getHardwareId] No HardwareId[" << device << "]";
#endif
    }

    return hardware_id_str;
}

QString Interception_Worker::getDeviceName(InterceptionDevice device)
{
    QString hardware_id_str;
    QString devicename;
    WCHAR hardware_id[MAX_PATH];
    size_t length = interception_get_hardware_id(s_InterceptionContext, device, hardware_id, sizeof(hardware_id));
    if(length > 0 && length < sizeof(hardware_id)) {
        hardware_id_str = QString::fromWCharArray(hardware_id);
        devicename = getDeviceNameByHardwareID(hardware_id_str);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[getDeviceName] Device[" << device << "] -> HardwareID: " << hardware_id_str << ", DeviceName: " << devicename;
        if (devicename.isEmpty()) {
            qDebug().nospace() << "[getDeviceName] No DeviceName[" << device << "]";
        }
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[getDeviceName] No HardwareId[" << device << "]";
#endif
    }

    return devicename;

}

void Interception_Worker::setInputDeviceDisabled(InterceptionDevice device, bool disabled)
{
    if (interception_is_invalid(device)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[setInputDeviceDisabled] Invalid Device[" << device << "]";
#endif
        return;
    }

    if (interception_is_keyboard(device))
    {
        KeyboardDeviceList[device - INTERCEPTION_KEYBOARD(0)].disabled = disabled;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[setInputDeviceDisabled] Keyboard[" << device << "] disabled = " << disabled;
#endif
    }
    else if(interception_is_mouse(device))
    {
        MouseDeviceList[device - INTERCEPTION_MOUSE(0)].disabled = disabled;
#ifdef DEBUG_LOGOUT_ON
        int mouse_index = device - INTERCEPTION_MOUSE(0) + 1;
        qDebug().nospace() << "[setInputDeviceDisabled] Mouse[" << mouse_index << "] disabled = " << disabled;
#endif
    }
}
