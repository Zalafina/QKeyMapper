#ifndef QKEYMAPPER_INPUT_DEVICE_TYPES_H
#define QKEYMAPPER_INPUT_DEVICE_TYPES_H

#include <QAtomicInteger>
#include <QString>
#include <QtGlobal>
#include <interception.h>

struct InputDeviceInfo
{
    QString hardwareid;
    QString devicedesc;
    ushort vendorid;
    ushort productid;
    QString VendorStr;
    QString ManufacturerStr;
    QString ProductStr;
};

struct InputDevice
{
    InterceptionDevice device;
    InputDeviceInfo deviceinfo;
    QAtomicInteger<bool> disabled;
};

struct USBDeviceInfo {
    QString vendorName;
    QString productName;
};

#endif // QKEYMAPPER_INPUT_DEVICE_TYPES_H