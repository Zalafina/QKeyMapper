#ifndef INTERCEPTION_WORKER_H
#define INTERCEPTION_WORKER_H

#include <QObject>
#include <interception.h>

#include "qkeymapper.h"
#include "qkeymapper_worker.h"

typedef struct
{
    void *handle;
    void *unempty;
} *InterceptionDeviceArray;

class Interception_Worker : public QObject
{
    Q_OBJECT
public:
    explicit Interception_Worker(QObject *parent = Q_NULLPTR);
    ~Interception_Worker();

    static Interception_Worker *getInstance()
    {
        static Interception_Worker m_instance;
        return &m_instance;
    }

    enum Interception_State
    {
        INTERCEPTION_INIT = 0,
        INTERCEPTION_UNAVAILABLE,
        INTERCEPTION_REBOOTREQUIRED,
        INTERCEPTION_AVAILABLE,
    };
    Q_ENUM(Interception_State)

signals:

public slots:
    void InterceptionThreadStarted(void);

public:
    static bool doLoadInterception(void);
    static void doUnloadInterception(void);
    static void interceptionLoopBreak(void);
    static bool isInterceptionDriverFileExist(void);
    static Interception_State getInterceptionState(void);
    static void setRebootRequiredFlag(void);
    static void startInterception(void);
    static void stopInterception(void);
    static bool getUSBDeviceDescriptor(ushort vendor_id, ushort product_id, QString& iManufacturer, QString& iProduct);
    static QHash<QString, USBDeviceInfo> parseUSBIDs(const QString& filePath);
    static QString getDeviceDescriptionByHardwareID(const QString& hardwareID);
    static InterceptionContext getInterceptionContext(void);
    static QList<InputDevice> getRefreshedKeyboardDeviceList(void);
    static QList<InputDevice> getRefreshedMouseDeviceList(void);
    static QList<InputDevice> getKeyboardDeviceList(void);
    static QList<InputDevice> getMouseDeviceList(void);
    static QString getHardwareId(InterceptionDevice device);
    static QString getDeviceDescription(InterceptionDevice device);
    static void setInputDeviceDisabled(InterceptionDevice device, bool disabled);
    static void updateDisabledKeyboardList(const QString& disabled_device, bool disabled);
    static void updateDisabledMouseList(const QString& disabled_device, bool disabled);
    static void syncDisabledKeyboardList(void);
    static void syncDisabledMouseList(void);
    static void saveDisabledKeyboardList(void);
    static void saveDisabledMouseList(void);
    static void loadDisabledKeyboardList(const QStringList& disabledlist);
    static void loadDisabledMouseList(const QStringList& disabledlist);

    static InterceptionContext s_InterceptionContext;
    static QAtomicBool s_RebootRequired;
    static bool s_libusb_available;
    static QAtomicBool s_InterceptStart;
    static QAtomicBool s_InterceptLoopbreak;
    static QAtomicBool s_FilterKeys;
    static QList<InputDevice> KeyboardDeviceList;
    static QList<InputDevice> MouseDeviceList;
    static InterceptionDevice lastOperateKeyboardDevice;
    static InterceptionDevice lastOperateMouseDevice;
    static QHash<QString, USBDeviceInfo> s_USBIDsMap;
    static QStringList disabledKeyboardList;
    static QStringList disabledMouseList;
};

#endif // INTERCEPTION_WORKER_H
