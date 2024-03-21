#ifndef INTERCEPTION_WORKER_H
#define INTERCEPTION_WORKER_H

#include <QObject>
#include <interception.h>

struct InputDevice
{
    QString hardwareid;
    QString devicename;
};

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

signals:

public slots:
    void InterceptionThreadStarted(void);

public:
    bool doLoad(void);
    void doUnload(void);

    static QString getDeviceNameByHardwareID(const QString& hardwareID);
    static InterceptionContext getInterceptionContext(void);
    static QList<InputDevice> getKeyboardDeviceList(void);
    static QList<InputDevice> getMouseDeviceList(void);

    static InterceptionContext s_InterceptionContext;
    static int s_InterceptionStatus;
};

#endif // INTERCEPTION_WORKER_H
