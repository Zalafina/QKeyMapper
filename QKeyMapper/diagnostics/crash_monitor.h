#ifndef QKM_CRASH_MONITOR_H
#define QKM_CRASH_MONITOR_H
#include <QObject>
#include <QString>
#include "crash_client.h"
class QThread;

class QkmCrashMonitor : public QObject {
    Q_OBJECT
public:
    explicit QkmCrashMonitor(QObject *parent = nullptr);
    void watch(QThread *thread, QkmDiagnostics::Slot slot);
    void announce();
signals:
    void diagnosticMessage(const QString &message);
private:
    void receiveResult();
    bool unavailableReported = false;
    bool uiReady = false;
    bool pendingResult = false;
};
#endif
