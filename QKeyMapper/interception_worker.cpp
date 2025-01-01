#include <QCoreApplication>
#include "interception_worker.h"
#include "qkeymapper_constants.h"

InterceptionContext Interception_Worker::s_InterceptionContext = Q_NULLPTR;
QAtomicBool Interception_Worker::s_RebootRequired = QAtomicBool();
bool Interception_Worker::s_libusb_available = false;
QAtomicBool Interception_Worker::s_InterceptStart = QAtomicBool();
QAtomicBool Interception_Worker::s_InterceptLoopbreak = QAtomicBool();
QAtomicBool Interception_Worker::s_FilterKeys = QAtomicBool(true);
QList<InputDevice> Interception_Worker::KeyboardDeviceList = QList<InputDevice>();
QList<InputDevice> Interception_Worker::MouseDeviceList = QList<InputDevice>();
InterceptionDevice Interception_Worker::lastOperateKeyboardDevice = INITIAL_KEYBOARD_INDEX;
InterceptionDevice Interception_Worker::lastOperateMouseDevice = INITIAL_MOUSE_INDEX;
QHash<QString, USBDeviceInfo> Interception_Worker::s_USBIDsMap;
QStringList Interception_Worker::disabledKeyboardList;
QStringList Interception_Worker::disabledMouseList;

Interception_Worker::Interception_Worker(QObject *parent) :
    QObject{parent}
{
    if (doLoadInterception()) {
        int result = libusb_init_context(NULL, NULL, 0);
        if (result < 0) {
#ifdef DEBUG_LOGOUT_ON
            qDebug("[Interception_Worker] Failed to initialise libusb %d - %s", result, libusb_strerror(result));
#endif
            s_libusb_available = false;
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug("[Interception_Worker] Success to initialise libusb : %d", result);
#endif
            s_libusb_available = true;
        }

        s_USBIDsMap = parseUSBIDs(USBIDS_QRC);

        /* Must do KeyboardDeviceList & MouseDeviceList Initialize before Interception Start */
        (void)getRefreshedKeyboardDeviceList();
        (void)getRefreshedMouseDeviceList();
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
    InterceptionStroke stroke;

    while(s_InterceptLoopbreak == false)
    {
        int receive_ret = interception_receive(s_InterceptionContext, device = interception_wait_with_timeout(s_InterceptionContext, 1), &stroke, 1);

        if (s_InterceptLoopbreak) {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[KeyInterceptionWorker] Interception Loop Breaked!";
#endif
            break;
        }

        QCoreApplication::processEvents();
        if (receive_ret <= 0) {
            continue;
        }

        if (!s_InterceptStart) {
            if(interception_is_mouse(device))
            {
                lastOperateMouseDevice = device;
            }
            else {
                lastOperateKeyboardDevice = device;
            }
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[KeyInterceptionWorker] Intercept is stopped!";
#endif
            interception_send(s_InterceptionContext, device, (InterceptionStroke *)&stroke, 1);
            continue;
        }

        if(interception_is_mouse(device))
        {
            lastOperateMouseDevice = device;
            int index = device - INTERCEPTION_MOUSE(0);
            if (MouseDeviceList.at(index).disabled) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[KeyInterceptionWorker] Mouse[" << index << "] is disabled!";
#endif
            }
            else {
                InterceptionMouseStroke &mstroke = *(InterceptionMouseStroke *) &stroke;
                mstroke.information = INTERCEPTION_EXTRA_INFO + device;
                // QKeyMapper_Worker::s_Mouse2vJoy_delta_interception.rx() += mstroke.x;
                // QKeyMapper_Worker::s_Mouse2vJoy_delta_interception.ry() += mstroke.y;
                // {
                //     QMutexLocker locker(&QKeyMapper_Worker::s_MouseMove_delta_List_Mutex);
                //     QKeyMapper_Worker::s_Mouse2vJoy_delta_List[index].rx() += mstroke.x;
                //     QKeyMapper_Worker::s_Mouse2vJoy_delta_List[index].ry() += mstroke.y;
                // }

                int delta_x = mstroke.x;
                int delta_y = mstroke.y;
                short delta_wheel = mstroke.rolling;
                unsigned short flags = mstroke.flags;
                ULONG_PTR extraInfo = mstroke.information;
                QKeyMapper_Worker::MouseEvent mouse_event = QKeyMapper_Worker::EVENT_NONE;
                if (mstroke.state != 0) {
                    if ((mstroke.state & INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN) == INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN) {
                        mouse_event = QKeyMapper_Worker::EVENT_LBUTTONDOWN;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_LEFT_BUTTON_UP) == INTERCEPTION_MOUSE_LEFT_BUTTON_UP) {
                        mouse_event = QKeyMapper_Worker::EVENT_LBUTTONUP;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN) == INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN) {
                        mouse_event = QKeyMapper_Worker::EVENT_RBUTTONDOWN;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_RIGHT_BUTTON_UP) == INTERCEPTION_MOUSE_RIGHT_BUTTON_UP) {
                        mouse_event = QKeyMapper_Worker::EVENT_RBUTTONUP;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_MIDDLE_BUTTON_DOWN) == INTERCEPTION_MOUSE_MIDDLE_BUTTON_DOWN) {
                        mouse_event = QKeyMapper_Worker::EVENT_MBUTTONDOWN;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_MIDDLE_BUTTON_UP) == INTERCEPTION_MOUSE_MIDDLE_BUTTON_UP) {
                        mouse_event = QKeyMapper_Worker::EVENT_MBUTTONUP;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_BUTTON_4_DOWN) == INTERCEPTION_MOUSE_BUTTON_4_DOWN) {
                        mouse_event = QKeyMapper_Worker::EVENT_X1BUTTONDOWN;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_BUTTON_4_UP) == INTERCEPTION_MOUSE_BUTTON_4_UP) {
                        mouse_event = QKeyMapper_Worker::EVENT_X1BUTTONUP;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_BUTTON_5_DOWN) == INTERCEPTION_MOUSE_BUTTON_5_DOWN) {
                        mouse_event = QKeyMapper_Worker::EVENT_X2BUTTONDOWN;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_BUTTON_5_UP) == INTERCEPTION_MOUSE_BUTTON_5_UP) {
                        mouse_event = QKeyMapper_Worker::EVENT_X2BUTTONUP;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_WHEEL) == INTERCEPTION_MOUSE_WHEEL) {
                        mouse_event = QKeyMapper_Worker::EVENT_MOUSEWHEEL;
                    }
                    else if ((mstroke.state & INTERCEPTION_MOUSE_HWHEEL) == INTERCEPTION_MOUSE_HWHEEL) {
                        mouse_event = QKeyMapper_Worker::EVENT_MOUSEHWHEEL;
                    }
                }
                else {
                    if (mstroke.x != 0 || mstroke.y != 0) {
                        mouse_event = QKeyMapper_Worker::EVENT_MOUSEMOVE;
                    }
                }

                int intercept = QKeyMapper_Worker::InterceptionMouseHookProc(mouse_event, delta_x, delta_y, delta_wheel, flags, extraInfo, index);
                if (INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION == intercept) {
                    /* Do not call interception_send */
                }
                else if (INTERCEPTION_RETURN_BLOCKEDBY_LOWLEVELHOOK == intercept) {
                    mstroke.information = INTERCEPTION_EXTRA_INFO_BLOCKED;
                    interception_send(s_InterceptionContext, device, (InterceptionStroke *)&stroke, 1);
                }
                else {
                    interception_send(s_InterceptionContext, device, (InterceptionStroke *)&stroke, 1);
                }

#ifdef INTERCEPTION_VERBOSE_LOG
                QString stateStr = QString("0x%1").arg(QString::number(mstroke.state, 16).toUpper(), 8, '0');
                qDebug().nospace() << "[KeyInterceptionWorker] Mouse[" << index << "] " << "State=" << stateStr << ", mouse_event=" << mouse_event << ", x=" << mstroke.x << ", y=" << mstroke.y <<", rolling=" << mstroke.rolling;
#endif
            }
        }
        else
        {
            lastOperateKeyboardDevice = device;
            int index = device - INTERCEPTION_KEYBOARD(0);
            if (KeyboardDeviceList.at(index).disabled) {
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[KeyInterceptionWorker] Keyboard[" << index << "] is disabled!";
#endif
            }
            else {
                InterceptionKeyStroke &kstroke = *(InterceptionKeyStroke *) &stroke;
                kstroke.information = INTERCEPTION_EXTRA_INFO + device;
                UINT scancode = kstroke.code;
                int keyupdown;
                ULONG_PTR extraInfo = kstroke.information;
                bool ExtenedFlag_e0 = false;
                bool ExtenedFlag_e1 = false;
                if ((kstroke.state & INTERCEPTION_KEY_UP) == INTERCEPTION_KEY_UP) {
                    keyupdown = KEY_UP;
                }
                else {
                    keyupdown = KEY_DOWN;
                }

                if ((kstroke.state & INTERCEPTION_KEY_E0) == INTERCEPTION_KEY_E0) {
                    ExtenedFlag_e0 = true;
                }
                if ((kstroke.state & INTERCEPTION_KEY_E1) == INTERCEPTION_KEY_E1) {
                    ExtenedFlag_e1 = true;
                }

                int intercept = QKeyMapper_Worker::InterceptionKeyboardHookProc(scancode, keyupdown, extraInfo, ExtenedFlag_e0, ExtenedFlag_e1, index);
                if (INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION == intercept) {
                    /* Do not call interception_send */
                }
                else if (INTERCEPTION_RETURN_BLOCKEDBY_LOWLEVELHOOK == intercept) {
                    kstroke.information = INTERCEPTION_EXTRA_INFO_BLOCKED;
                    interception_send(s_InterceptionContext, device, (InterceptionStroke *)&stroke, 1);
                }
                else {
                    interception_send(s_InterceptionContext, device, (InterceptionStroke *)&stroke, 1);
                }

#ifdef INTERCEPTION_VERBOSE_LOG
                QString keyupdownStr;
                if (KEY_UP == keyupdown) {
                    keyupdownStr = "KEY_UP";
                }
                else {
                    keyupdownStr = "KEY_DOWN";
                }
                QString codeStr = QString("0x%1").arg(QString::number(scancode, 16).toUpper(), 4, '0');
                QString keystateStr = QString("0x%1").arg(QString::number(kstroke.state, 16).toUpper(), 4, '0');
                qDebug().nospace() << "[KeyInterceptionWorker] Keyboard[" << index << "] " << codeStr << " " << keyupdownStr << ", Intercept = " << intercept << ", E0 = " << ExtenedFlag_e0 << ", E1 = " << ExtenedFlag_e1 << ", State =" << keystateStr;
#endif
            }
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[KeyInterceptionWorker] Call stopInterception after interception_receive while loop breaked.";
#endif
    stopInterception();
}

bool Interception_Worker::doLoadInterception()
{
    bool result = false;
    s_InterceptionContext = interception_create_context();
    if (s_InterceptionContext == Q_NULLPTR) {
        result = false;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[doLoadInterception] interception_create_context() Failed!!!";
#endif
    }
    else {
        result = true;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[doLoadInterception] interception_create_context() Success.";
#endif
    }

    return result;
}

void Interception_Worker::doUnloadInterception()
{
    if (s_InterceptionContext != Q_NULLPTR) {
        stopInterception();
        interception_destroy_context(s_InterceptionContext);
        s_InterceptionContext = Q_NULLPTR;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[doUnloadInterception] interception_destroy_context()";
#endif
    }
}

void Interception_Worker::interceptionLoopBreak()
{
    s_InterceptLoopbreak = true;

//     if(s_InterceptionContext == Q_NULLPTR) {
//         return;
//     }

//     InterceptionDeviceArray device_array = (InterceptionDeviceArray)s_InterceptionContext;
//     QList<HANDLE> wait_handles;
//     for(int i = 0; i < INTERCEPTION_MAX_DEVICE; ++i)
//     {
//         if (device_array[i].unempty) {
//             wait_handles.append(device_array[i].unempty);
//         }
//     }

//     bool setevent_result = SetEvent(wait_handles.constFirst());
//     Q_UNUSED(setevent_result);

// #ifdef DEBUG_LOGOUT_ON
//     qDebug() << "[interceptionLoopBreak]" << "wait_handles.size() =" << wait_handles.size() << ", setevent_result =" << setevent_result;
// #endif
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
    fileDescription = QString::fromUtf16(reinterpret_cast<Utf16Pointer>(value), length);
    fileDescription.remove(QChar('\0'));
    if (!VerQueryValue(data.data(), L"\\StringFileInfo\\040904b0\\ProductName", &value, &length)) {
        return false;
    }
    productName = QString::fromUtf16(reinterpret_cast<Utf16Pointer>(value), length);
    productName.remove(QChar('\0'));
    if (!VerQueryValue(data.data(), L"\\StringFileInfo\\040904b0\\OriginalFilename", &value, &length)) {
        return false;
    }
    originalFilename = QString::fromUtf16(reinterpret_cast<Utf16Pointer>(value), length);
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
    if (s_RebootRequired) {
        return INTERCEPTION_REBOOTREQUIRED;
    }

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

void Interception_Worker::setRebootRequiredFlag()
{
    s_RebootRequired = true;
}

void Interception_Worker::startInterception()
{
    if (s_InterceptionContext == Q_NULLPTR) {
        return;
    }

    QKeyMapper_Worker::pressedMultiKeyboardVKeyCodeList.clear();
    for (int i = 0; i < INTERCEPTION_MAX_KEYBOARD; ++i) {
        QKeyMapper_Worker::pressedMultiKeyboardVKeyCodeList.append(QList<quint8>());
    }
    s_InterceptStart = true;

#ifdef QT_DEBUG
    if (!IsDebuggerPresent()) {
        interception_set_filter(s_InterceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);
        interception_set_filter(s_InterceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_ALL);
    }
#else
    interception_set_filter(s_InterceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);
    interception_set_filter(s_InterceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_ALL);
#endif
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[startInterception] Start Keyboard & Mouse Interception.";
#endif
}

void Interception_Worker::stopInterception()
{
    s_InterceptStart = false;
    QKeyMapper_Worker::pressedMultiKeyboardVKeyCodeList.clear();
    for (int i = 0; i < INTERCEPTION_MAX_KEYBOARD; ++i) {
        QKeyMapper_Worker::pressedMultiKeyboardVKeyCodeList.append(QList<quint8>());
    }

    if (s_InterceptionContext == Q_NULLPTR) {
        return;
    }

    interception_set_filter(s_InterceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_NONE);
    interception_set_filter(s_InterceptionContext, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_NONE);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[stopInterception] Stop Keyboard & Mouse Interception.";
#endif
}

bool Interception_Worker::getUSBDeviceDescriptor(ushort vendor_id, ushort product_id, QString &iManufacturer, QString &iProduct)
{
    if (!s_libusb_available) {
        return false;
    }

    libusb_device_handle *handle;
    libusb_device *dev;
    struct libusb_device_descriptor dev_desc;
    char string[256];

    handle = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id);
    if (handle == NULL) {
#ifdef DEBUG_LOGOUT_ON
        QString vendorIdStr = QString("0x%1").arg(QString::number(vendor_id, 16).toUpper(), 4, '0');
        QString productIdStr = QString("0x%1").arg(QString::number(product_id, 16).toUpper(), 4, '0');
        qDebug().nospace().noquote() << "[getUSBDeviceDescriptor] Failed to open USB device -> " << "VendorID = " << vendorIdStr << ", ProductID = " << productIdStr;
#endif
        return false;
    }

    dev = libusb_get_device(handle);
    int ret = libusb_get_device_descriptor(dev, &dev_desc);
    if (ret != LIBUSB_SUCCESS) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[getUSBDeviceDescriptor] Failed to get device descriptor :" << ret;
#endif
        libusb_close(handle);
        return false;
    }

    if (dev_desc.iManufacturer) {
        ret = libusb_get_string_descriptor_ascii(handle, dev_desc.iManufacturer, (unsigned char*)string, sizeof(string));
        if (ret > 0) {
            iManufacturer = QString::fromLatin1(string, ret);
        } else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[getUSBDeviceDescriptor] Failed to get iManufacturer string descriptor :" << ret;
#endif
        }
    }

    if (dev_desc.iProduct) {
        ret = libusb_get_string_descriptor_ascii(handle, dev_desc.iProduct, (unsigned char*)string, sizeof(string));
        if (ret > 0) {
            iProduct = QString::fromLatin1(string, ret);
        } else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[getUSBDeviceDescriptor] Failed to get iProduct string descriptor :" << ret;
#endif
        }
    }

    libusb_close(handle);
    return true;
}

QHash<QString, USBDeviceInfo> Interception_Worker::parseUSBIDs(const QString &filePath)
{
    QHash<QString, USBDeviceInfo> usbIDs;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open USB IDs file.";
        return usbIDs;
    }

    QTextStream in(&file);
    QString line;
    QString currentVendorId;
    QString currentVendorName;
    while (!in.atEnd()) {
        line = in.readLine();

        if (line.startsWith('#') || line.trimmed().isEmpty())
            continue;

        if (line.startsWith('\t')) {
            // Product line
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            QStringList fields = line.trimmed().split("  ", Qt::SkipEmptyParts);
#else
            QStringList fields = line.trimmed().split("  ", QString::SkipEmptyParts);
#endif
            if (fields.size() < 2)
                continue;

            QString productId = fields[0];
            QString productName = fields[1];
            USBDeviceInfo deviceInfo;
            deviceInfo.vendorName = currentVendorName;
            deviceInfo.productName = productName;
            usbIDs[currentVendorId + productId] = deviceInfo;
        } else {
            // Vendor line
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            QStringList fields = line.trimmed().split("  ", Qt::SkipEmptyParts);
#else
            QStringList fields = line.trimmed().split("  ", QString::SkipEmptyParts);
#endif
            if (fields.size() < 2)
                continue;

            QString productId = QString("0000");
            currentVendorId = fields[0];
            currentVendorName = fields[1];
            USBDeviceInfo deviceInfo;
            deviceInfo.vendorName = currentVendorName;
            usbIDs[currentVendorId + productId] = deviceInfo;
        }
    }

    file.close();
    return usbIDs;
}

QString Interception_Worker::getDeviceDescriptionByHardwareID(const QString &hardwareID)
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

QList<InputDevice> Interception_Worker::getRefreshedKeyboardDeviceList()
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

        input_device.device = device;
        if(length > 0 && length < sizeof(hardware_id)) {
            QString hardware_id_str = QString::fromWCharArray(hardware_id);
            QString devicedesc = getDeviceDescriptionByHardwareID(hardware_id_str);
            input_device.deviceinfo.hardwareid = hardware_id_str;
            input_device.deviceinfo.devicedesc = devicedesc;

            ushort vendorID = 0;
            ushort productID = 0;
            QString vendorIDStr;
            QString productIDStr;
            QString iManufacturerStr;
            QString iProductStr;
            static QRegularExpression regex(R"(VID_(\w+)&PID_(\w+))");
            QRegularExpressionMatch match = regex.match(hardware_id_str);
            if (match.hasMatch()) {
                vendorIDStr = match.captured(1);
                productIDStr = match.captured(2);
                bool ok;
                vendorID = vendorIDStr.toUShort(&ok, 16);
                if (ok) {
                    input_device.deviceinfo.vendorid = vendorID;
                }
                productID = productIDStr.toUShort(&ok, 16);
                if (ok) {
                    input_device.deviceinfo.productid = productID;
                }
            }

            if (vendorID && productID) {
                if (getUSBDeviceDescriptor(vendorID, productID, iManufacturerStr, iProductStr)) {
                    input_device.deviceinfo.ManufacturerStr = iManufacturerStr;
                    input_device.deviceinfo.ProductStr = iProductStr;
                }

                QString vendor_key = vendorIDStr.toLower() + QString("0000");
                if (s_USBIDsMap.contains(vendor_key)) {
                    USBDeviceInfo deviceInfo = s_USBIDsMap.value(vendor_key);
                    input_device.deviceinfo.VendorStr = deviceInfo.vendorName;
                }

                QString product_key = vendorIDStr.toLower() + productIDStr.toLower();
                if (s_USBIDsMap.contains(product_key)) {
                    USBDeviceInfo deviceInfo = s_USBIDsMap.value(product_key);
                    if (input_device.deviceinfo.ManufacturerStr.isEmpty()) {
                        input_device.deviceinfo.ManufacturerStr = deviceInfo.vendorName;;
                    }
                    input_device.deviceinfo.ProductStr = deviceInfo.productName;
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[getRefreshedKeyboardDeviceList] USB device not found in USB IDs database -> " << "VendorID =" << vendorIDStr << ", ProductID =" << productIDStr;
#endif
                }

            }
#ifdef DEBUG_LOGOUT_ON
            int index = device - INTERCEPTION_KEYBOARD(0);
            if (vendorIDStr.isEmpty() && productIDStr.isEmpty()) {
                qDebug().nospace().noquote() << "[getRefreshedKeyboardDeviceList] [" << index << "]Keyboard -> HardwareID=" << input_device.deviceinfo.hardwareid << ", DeviceDescription=" << input_device.deviceinfo.devicedesc;
            }
            else {
                qDebug().nospace().noquote() << "[getRefreshedKeyboardDeviceList] [" << index << "]Keyboard -> HardwareID=" << input_device.deviceinfo.hardwareid << ", Vendor=" << input_device.deviceinfo.VendorStr << ", Manufacturer=" << input_device.deviceinfo.ManufacturerStr << ", ProductStr=" << input_device.deviceinfo.ProductStr << ", DeviceDescription=" << input_device.deviceinfo.devicedesc << ", VendorID=0x" << vendorIDStr << ", ProductID=0x" << productIDStr;
            }
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            int index = device - INTERCEPTION_KEYBOARD(0);
            qDebug().nospace() << "[getRefreshedKeyboardDeviceList] [" << index << "]No Keyboard Device";
#endif
        }

        devicelist.append(input_device);
    }

    KeyboardDeviceList = devicelist;

    return KeyboardDeviceList;
}

QList<InputDevice> Interception_Worker::getRefreshedMouseDeviceList()
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

        input_device.device = device;
        if(length > 0 && length < sizeof(hardware_id)) {
            QString hardware_id_str = QString::fromWCharArray(hardware_id);
            QString devicedesc = getDeviceDescriptionByHardwareID(hardware_id_str);
            input_device.deviceinfo.hardwareid = hardware_id_str;
            input_device.deviceinfo.devicedesc = devicedesc;

            ushort vendorID = 0;
            ushort productID = 0;
            QString vendorIDStr;
            QString productIDStr;
            QString iManufacturerStr;
            QString iProductStr;
            static QRegularExpression regex(R"(VID_(\w+)&PID_(\w+))");
            QRegularExpressionMatch match = regex.match(hardware_id_str);
            if (match.hasMatch()) {
                vendorIDStr = match.captured(1);
                productIDStr = match.captured(2);
                bool ok;
                vendorID = vendorIDStr.toUShort(&ok, 16);
                if (ok) {
                    input_device.deviceinfo.vendorid = vendorID;
                }
                productID = productIDStr.toUShort(&ok, 16);
                if (ok) {
                    input_device.deviceinfo.productid = productID;
                }
            }

            if (vendorID && productID) {
                if (getUSBDeviceDescriptor(vendorID, productID, iManufacturerStr, iProductStr)) {
                    input_device.deviceinfo.ManufacturerStr = iManufacturerStr;
                    input_device.deviceinfo.ProductStr = iProductStr;
                }

                QString vendor_key = vendorIDStr.toLower() + QString("0000");
                if (s_USBIDsMap.contains(vendor_key)) {
                    USBDeviceInfo deviceInfo = s_USBIDsMap.value(vendor_key);
                    input_device.deviceinfo.VendorStr = deviceInfo.vendorName;
                }

                QString product_key = vendorIDStr.toLower() + productIDStr.toLower();
                if (s_USBIDsMap.contains(product_key)) {
                    USBDeviceInfo deviceInfo = s_USBIDsMap.value(product_key);
                    if (input_device.deviceinfo.ManufacturerStr.isEmpty()) {
                        input_device.deviceinfo.ManufacturerStr = deviceInfo.vendorName;;
                    }
                    input_device.deviceinfo.ProductStr = deviceInfo.productName;
                }
                else {
#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[getRefreshedMouseDeviceList] USB device not found in USB IDs database -> " << "VendorID =" << vendorIDStr << ", ProductID =" << productIDStr;
#endif
                }

            }
#ifdef DEBUG_LOGOUT_ON
            int index = device - INTERCEPTION_MOUSE(0);
            if (vendorIDStr.isEmpty() && productIDStr.isEmpty()) {
                qDebug().nospace().noquote() << "[getRefreshedMouseDeviceList] [" << index << "]Mouse -> HardwareID=" << input_device.deviceinfo.hardwareid << ", DeviceDescription=" << input_device.deviceinfo.devicedesc;
            }
            else {
                qDebug().nospace().noquote() << "[getRefreshedMouseDeviceList] [" << index << "]Mouse -> HardwareID=" << input_device.deviceinfo.hardwareid << ", Vendor=" << input_device.deviceinfo.VendorStr << ", Manufacturer=" << input_device.deviceinfo.ManufacturerStr << ", ProductStr=" << input_device.deviceinfo.ProductStr << ", DeviceDescription=" << input_device.deviceinfo.devicedesc << ", VendorID=0x" << vendorIDStr << ", ProductID=0x" << productIDStr;
            }
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            int index = device - INTERCEPTION_MOUSE(0);
            qDebug().nospace() << "[getRefreshedMouseDeviceList] [" << index << "]No Mouse Device";
#endif
        }

        devicelist.append(input_device);
    }

    MouseDeviceList = devicelist;

    return MouseDeviceList;
}

QList<InputDevice> Interception_Worker::getKeyboardDeviceList()
{
    return KeyboardDeviceList;
}

QList<InputDevice> Interception_Worker::getMouseDeviceList()
{
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

QString Interception_Worker::getDeviceDescription(InterceptionDevice device)
{
    QString hardware_id_str;
    QString devicedesc;
    WCHAR hardware_id[MAX_PATH];
    size_t length = interception_get_hardware_id(s_InterceptionContext, device, hardware_id, sizeof(hardware_id));
    if(length > 0 && length < sizeof(hardware_id)) {
        hardware_id_str = QString::fromWCharArray(hardware_id);
        devicedesc = getDeviceDescriptionByHardwareID(hardware_id_str);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[getDeviceDescription] Device[" << device << "] -> HardwareID: " << hardware_id_str << ", DeviceDescription: " << devicedesc;
        if (devicedesc.isEmpty()) {
            qDebug().nospace() << "[getDeviceDescription] No DeviceDescription[" << device << "]";
        }
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[getDeviceDescription] No HardwareId[" << device << "]";
#endif
    }

    return devicedesc;

}

void Interception_Worker::setInputDeviceDisabled(InterceptionDevice device, bool disabled)
{
    if (interception_is_invalid(device)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[setInputDeviceDisabled] Invalid InputDevice[" << device << "]";
#endif
        return;
    }

    if (interception_is_keyboard(device))
    {
        if (!KeyboardDeviceList.isEmpty()) {
            int index = device - INTERCEPTION_KEYBOARD(0);
            KeyboardDeviceList[index].disabled = disabled;
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[setInputDeviceDisabled] Keyboard[" << index << "] disabled = " << disabled;
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[setInputDeviceDisabled]" << " KeyboardDeviceList is Empty!";
#endif
        }
    }
    else if(interception_is_mouse(device))
    {
        if (!MouseDeviceList.isEmpty()) {
            int index = device - INTERCEPTION_MOUSE(0);
            MouseDeviceList[index].disabled = disabled;
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[setInputDeviceDisabled] Mouse[" << index << "] disabled = " << disabled;
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[setInputDeviceDisabled]" << " MouseDeviceList is Empty!";
#endif
        }
    }
}

void Interception_Worker::updateDisabledKeyboardList(const QString &disabled_device, bool disabled)
{
    if (disabled) {
        if (!disabledKeyboardList.contains(disabled_device)) {
            disabledKeyboardList.append(disabled_device);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[updateDisabledKeyboardList]" << " Add disabled Keyboard -> " << disabled_device;
#endif
        }
    }
    else {
        if (disabledKeyboardList.contains(disabled_device)) {
            disabledKeyboardList.removeAll(disabled_device);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[updateDisabledKeyboardList]" << " Remove disabled Keyboard -> " << disabled_device;
#endif
        }
    }
}

void Interception_Worker::updateDisabledMouseList(const QString &disabled_device, bool disabled)
{
    if (disabled) {
        if (!disabledMouseList.contains(disabled_device)) {
            disabledMouseList.append(disabled_device);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[updateDisabledMouseList]" << " Add disabled Mouse -> " << disabled_device;
#endif
        }
    }
    else {
        if (disabledMouseList.contains(disabled_device)) {
            disabledMouseList.removeAll(disabled_device);
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[updateDisabledMouseList]" << " Remove disabled Mouse -> " << disabled_device;
#endif
        }
    }
}

void Interception_Worker::syncDisabledKeyboardList()
{
    for (int i = 0; i < KeyboardDeviceList.size(); ++i) {
        InputDevice& inputDevice = KeyboardDeviceList[i];
        QString hardwareid = inputDevice.deviceinfo.hardwareid;

        // Check if hardwareid is empty
        if (!hardwareid.isEmpty()) {
            QString disabledDevice = inputDevice.deviceinfo.devicedesc + JOIN_DEVICE + hardwareid;

            // Check if disabledKeyboardList contains disabledDevice
            if (disabledKeyboardList.contains(disabledDevice)) {
                if (inputDevice.disabled != true) {
                    inputDevice.disabled = true;
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "[syncDisabledKeyboardList][Changed]" << " KeyboardDeviceList[" << i << "] disabled set TRUE";
#endif
                }
            } else {
                if (inputDevice.disabled != false) {
                    inputDevice.disabled = false;
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "[syncDisabledKeyboardList][Changed]" << " KeyboardDeviceList[" << i << "] disabled set FALSE";
#endif
                }
            }
        }
    }

#ifdef DEBUG_LOGOUT_ON
    /* Output All MouseDeviceList disabled status */
    for (int i = 0; i < KeyboardDeviceList.size(); ++i) {
        qDebug().nospace() << "[syncDisabledKeyboardList] KeyboardDeviceList[" << i << "] disabled: " << KeyboardDeviceList.at(i).disabled;
    }
#endif
}

void Interception_Worker::syncDisabledMouseList()
{
    for (int i = 0; i < MouseDeviceList.size(); ++i) {
        InputDevice& inputDevice = MouseDeviceList[i];
        QString hardwareid = inputDevice.deviceinfo.hardwareid;

        // Check if hardwareid is empty
        if (!hardwareid.isEmpty()) {
            QString disabledDevice = inputDevice.deviceinfo.devicedesc + JOIN_DEVICE + hardwareid;

            // Check if disabledMouseList contains disabledDevice
            if (disabledMouseList.contains(disabledDevice)) {
                if (inputDevice.disabled != true) {
                    inputDevice.disabled = true;
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "[syncDisabledMouseList][Changed]" << " MouseDeviceList[" << i << "] disabled set TRUE";
#endif
                }
            } else {
                if (inputDevice.disabled != false) {
                    inputDevice.disabled = false;
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "[syncDisabledMouseList][Changed]" << " MouseDeviceList[" << i << "] disabled set FALSE";
#endif
                }
            }
        }
    }

#ifdef DEBUG_LOGOUT_ON
    /* Output All MouseDeviceList disabled status */
    for (int i = 0; i < MouseDeviceList.size(); ++i) {
        qDebug().nospace() << "[syncDisabledMouseList] MouseDeviceList[" << i << "] disabled: " << MouseDeviceList.at(i).disabled;
    }
#endif
}

void Interception_Worker::saveDisabledKeyboardList()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[saveDisabledKeyboardList] Save disabledKeyboardList ->" << disabledKeyboardList;
#endif

    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    settingFile.setValue(DISABLED_KEYBOARDLIST , disabledKeyboardList);
}

void Interception_Worker::saveDisabledMouseList()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[saveDisabledMouseList] Save disabledMouseList ->" << disabledMouseList;
#endif

    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    settingFile.setValue(DISABLED_MOUSELIST , disabledMouseList);
}

void Interception_Worker::loadDisabledKeyboardList(const QStringList &disabledlist)
{
    disabledKeyboardList = disabledlist;
}

void Interception_Worker::loadDisabledMouseList(const QStringList &disabledlist)
{
    disabledMouseList = disabledlist;
}
