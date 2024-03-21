#include "qkeymapper.h"
#include "interception_worker.h"

InterceptionContext Interception_Worker::s_InterceptionContext = Q_NULLPTR;
int Interception_Worker::s_InterceptionStatus = 0;

Interception_Worker::Interception_Worker(QObject *parent) :
    QObject{parent}
{
    if (doLoad()) {
        s_InterceptionStatus = 1;
    }
}

Interception_Worker::~Interception_Worker()
{
    doUnload();
}

void Interception_Worker::InterceptionThreadStarted()
{
#ifdef DEBUG_LOGOUT_ON
    QString threadIdStr = QString("0x%1").arg(QString::number((qulonglong)QThread::currentThreadId(), 16).toUpper(), 8, '0');
    qDebug().nospace().noquote() << "[InterceptionThreadStarted] ThreadName:" << QThread::currentThread()->objectName() << ", ThreadID:" << threadIdStr;
#endif
}

bool Interception_Worker::doLoad()
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

void Interception_Worker::doUnload()
{
    if (s_InterceptionContext != Q_NULLPTR) {
        interception_destroy_context(s_InterceptionContext);
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
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[getKeyboardDeviceList] Keyboard[" << device << "] -> HardwareID: " << hardware_id_str << ", DeviceName: " << devicename;
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[getKeyboardDeviceList] No Keyboard[" << device << "]";
#endif
        }
        devicelist.append(input_device);
    }

    return devicelist;
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
        int mouse_index = device - INTERCEPTION_MAX_KEYBOARD;
        if(length > 0 && length < sizeof(hardware_id)) {
            QString hardware_id_str = QString::fromWCharArray(hardware_id);
            QString devicename = getDeviceNameByHardwareID(hardware_id_str);
            input_device.hardwareid = hardware_id_str;
            input_device.devicename = devicename;
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[getMouseDeviceList] Mouse[" << mouse_index << "] -> HardwareID: " << hardware_id_str << ", DeviceName: " << devicename;
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[getMouseDeviceList] No Mouse[" << mouse_index << "]";
#endif
        }
        devicelist.append(input_device);
    }

    return devicelist;
}
