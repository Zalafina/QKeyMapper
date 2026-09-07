#include "crash_monitor.h"
#include <QThread>
#include <QTimer>
#include <QWinEventNotifier>

using namespace QkmDiagnostics;
QkmCrashMonitor::QkmCrashMonitor(QObject *parent) : QObject(parent)
{
    if (reporterHandle()) {
        auto *death = new QWinEventNotifier(reporterHandle(), this);
        connect(death, &QWinEventNotifier::activated, this, [this, death]() {
            death->setEnabled(false);
            disableHang();
            if (uiReady) announce();
        });
        auto *result = new QWinEventNotifier(notificationHandle(), this);
        connect(result, &QWinEventNotifier::activated, this, [this]() { receiveResult(); });
    }
    if (hangEnabled()) {
        expectThread(Ui);
        beginHeartbeat(Ui);
        auto *timer = new QTimer(this);
        timer->setTimerType(Qt::CoarseTimer);
        connect(timer, &QTimer::timeout, this, [timer]() {
            if (hangEnabled()) heartbeat(Ui);
            else timer->stop();
        });
        timer->start(HeartbeatIntervalMs);
    }
}
void QkmCrashMonitor::watch(QThread *thread, Slot slot)
{
    if (!hangEnabled()) return;
    expectThread(slot);
    auto *timer = new QTimer;
    timer->setTimerType(Qt::CoarseTimer);
    timer->moveToThread(thread);
    // Connect before the driver's started slot, which runs its own event loop.
    connect(thread, &QThread::started, timer, [timer, slot]() {
        beginHeartbeat(slot);
        timer->start(HeartbeatIntervalMs);
    }, Qt::DirectConnection);
    connect(timer, &QTimer::timeout, timer, [timer, slot]() {
        if (hangEnabled()) heartbeat(slot);
        else timer->stop();
    });
    connect(thread, &QThread::finished, timer, &QObject::deleteLater);
}
void QkmCrashMonitor::announce()
{
    uiReady = true;
    DWORD error = startupError();
    if (!error && reporterHandle() && WaitForSingleObject(reporterHandle(), 0) == WAIT_OBJECT_0) {
        if (!GetExitCodeProcess(reporterHandle(), &error) || !error) error = ERROR_PROCESS_ABORTED;
    }
    if (error && !unavailableReported) {
        unavailableReported = true;
        emit diagnosticMessage(tr("Crash diagnostics unavailable (error %1).").arg(error));
    }
    if (pendingResult) { pendingResult = false; receiveResult(); }
}
void QkmCrashMonitor::receiveResult()
{
    if (!uiReady) { pendingResult = true; return; }
    Result result = {};
    if (!readResult(result)) return;
    if (result.kind == Cleanup) {
        if (result.error) emit diagnosticMessage(tr("Diagnostic file cleanup failed (error %1).").arg(result.error));
    } else if (result.status == Complete) {
        QString message = tr("Diagnostic dump saved: %1").arg(QString::fromWCharArray(result.path));
        if (result.metadataError)
            message += QLatin1Char('\n') + tr("Diagnostic metadata failed (error %1).").arg(result.metadataError);
        emit diagnosticMessage(message);
    } else {
        emit diagnosticMessage(tr("Diagnostic dump failed (error %1, directory error %2).").arg(result.error).arg(result.directoryError));
    }
}
