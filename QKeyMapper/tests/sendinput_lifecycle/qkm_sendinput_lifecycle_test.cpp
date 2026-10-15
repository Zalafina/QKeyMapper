#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSemaphore>
#include <QTimer>
#include "../../qkeymapper_worker.h"

namespace {
using namespace QKeyMapperConstants;

struct LifecycleResult {
    QAtomicInt downStarted{0};
    QAtomicInt downFinished{0};
    QAtomicInt upStarted{0};
    QAtomicInt upFinished{0};
    QAtomicInt cancelledDown{0};
    QAtomicInt borrowerCalls{0};
    bool passed = true;
    int activeAtCleanup = 0;
    int pauseAtCleanup = 0;
    qint64 cleanupMs = 0;
};
LifecycleResult *result = nullptr;

class ObservedTask : public SendInputTask {
public:
    ObservedTask(const QStringList &keys, int direction, const QString &original)
        : SendInputTask(-1, keys, direction, original, SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL) {}

    void run() override
    {
        if (m_keyupdown == KEY_DOWN) {
            result->downStarted.fetchAndAddRelaxed(1);
            if (s_SendInputTaskControllerMap[m_real_originalkey].task_stopping.loadAcquire()) {
                result->cancelledDown.fetchAndAddRelaxed(1);
            }
        } else {
            result->upStarted.fetchAndAddRelaxed(1);
        }
        SendInputTask::run();
        if (m_keyupdown == KEY_DOWN) {
            result->downFinished.fetchAndAddRelaxed(1);
        } else {
            result->upFinished.fetchAndAddRelaxed(1);
        }
    }
};

void submit(const QStringList &keys, int direction, const QString &original)
{
    auto *task = new ObservedTask(keys, direction, original);
    auto &controller = SendInputTask::s_SendInputTaskControllerMap[task->m_real_originalkey];
    controller.task_threadpool->start(task);
}

void finish(bool passed)
{
    result->passed = result->passed && passed;
    QMetaObject::invokeMethod(QCoreApplication::instance(), []() {
        QCoreApplication::exit(result->passed ? 0 : 2);
    }, Qt::QueuedConnection);
}

void installLifecycleTest()
{
    QTimer::singleShot(0, QCoreApplication::instance(), []() {
        auto *worker = QKeyMapper_Worker::getInstance();
        QMetaObject::invokeMethod(worker, [worker]() {
            const QString mode = qEnvironmentVariable("QKM_LIFECYCLE_CASE");
            const bool paused = mode.startsWith(QStringLiteral("pause"))
                || mode == QStringLiteral("exit_pause") || mode.startsWith(QStringLiteral("break"));
            const QString original = paused ? QStringLiteral("QkmProbe:KeySequence2") : QStringLiteral("QkmProbe");
            const QStringList keys{paused ? QStringLiteral("QkmProbeNoOp") : QStringLiteral("QkmProbeNoOp⏱5000")};
            auto *task = new ObservedTask(keys, KEY_DOWN, original);
            auto &controller = SendInputTask::s_SendInputTaskControllerMap[task->m_real_originalkey];
            if (paused) {
                controller.task_pause_state->storeRelease(1);
                QKeyMapper_Worker::s_runningKeySequenceOrikeyList.append(QStringLiteral("QkmProbe"));
            }
            controller.task_threadpool->start(task);
            auto *timer = new QTimer(worker);
            QObject::connect(timer, &QTimer::timeout, worker, [worker, timer, mode, paused, original, keys, attempts = 0]() mutable {
                auto &current = SendInputTask::s_SendInputTaskControllerMap[QStringLiteral("QkmProbe")];
                ++attempts;
                if ((paused && current.task_pause_state->loadAcquire() != 2)
                    || (!paused && (attempts < 5 || result->downStarted.loadAcquire() == 0))) {
                    if (attempts > 250) { timer->stop(); finish(false); }
                    return;
                }
                timer->stop();
                timer->deleteLater();
                result->activeAtCleanup = current.task_threadpool->activeThreadCount();
                result->pauseAtCleanup = current.task_pause_state->loadAcquire();
                result->passed = result->passed && result->activeAtCleanup == 1;
                QElapsedTimer elapsed;
                elapsed.start();
                if (mode.startsWith(QStringLiteral("exit"))) {
                    finish(true);
                    return;
                }
                if (mode == QStringLiteral("borrowers")) {
#ifdef LOGOUT_TOFILE
                    // Diagnostic disk logging is not a Release performance benchmark; the runner has a watchdog.
                    constexpr int joinTimeout = -1;
#else
                    constexpr int joinTimeout = 1000;
#endif
                    QThreadPool borrowers;
                    QSemaphore started;
                    borrowers.start([worker, original, &started]() {
                        QStringList input{QStringLiteral("QkmProbeNoOp")};
                        QString key = original;
                        started.release();
                        for (int count = 0; count < 500; ++count) {
                            worker->emit_sendInputKeysSignal_Wrapper(-1, input, KEY_DOWN, key, SENDMODE_NORMAL);
                            result->borrowerCalls.fetchAndAddRelaxed(1);
                        }
                    });
                    result->passed = result->passed && started.tryAcquire(1, 1000);
                    SendInputTask::clearSendInputTaskControllerMap();
                    result->passed = result->passed && borrowers.waitForDone(joinTimeout)
                        && result->borrowerCalls.loadAcquire() == 500;
                } else if (mode == QStringLiteral("queue_keyup")) {
                    submit({QStringLiteral("QkmProbeQueuedNoOp")}, KEY_DOWN, original);
                    submit(keys, KEY_UP, original);
                    SendInputTask::clearSendInputTaskControllerMap();
                    result->passed = result->passed && result->cancelledDown.loadAcquire() == 1
                        && result->upStarted.loadAcquire() == 1 && result->upFinished.loadAcquire() == 1;
                } else if (mode == QStringLiteral("pause_continue")) {
                    QKeyMapper_Worker::continueRunningKeySequence(QStringLiteral("QkmProbe"));
                    result->passed = result->passed && current.task_threadpool->waitForDone(1000);
                    SendInputTask::clearSendInputTaskControllerMap();
                } else if (mode.startsWith(QStringLiteral("break"))) {
                    if (mode == QStringLiteral("break_one")) {
                        QKeyMapper_Worker::breakRunningKeySequence(QStringLiteral("QkmProbe"));
                    } else {
                        QKeyMapper_Worker::breakAllRunningKeySequence();
                    }
                    result->passed = result->passed && current.task_threadpool->waitForDone(1000);
                    SendInputTask::clearSendInputTaskControllerMap();
                } else if (mode == QStringLiteral("stop") || mode == QStringLiteral("restart")) {
                    if (mode == QStringLiteral("stop")) { worker->setWorkerKeyUnHook(); }
                    else { worker->setKeyMappingRestart(); }
                    for (auto &entry : SendInputTask::s_SendInputTaskControllerMap) {
                        result->passed = result->passed && entry.task_threadpool->waitForDone(1000);
                    }
                    SendInputTask::clearSendInputTaskControllerMap();
                } else {
                    SendInputTask::clearSendInputTaskControllerMap();
                    if (mode == QStringLiteral("cancel_reset")) {
                        submit({QStringLiteral("QkmProbeNoOp")}, KEY_DOWN, QStringLiteral("QkmProbe"));
                        submit({QStringLiteral("QkmProbeNoOp")}, KEY_UP, QStringLiteral("QkmProbe"));
                        auto &fresh = SendInputTask::s_SendInputTaskControllerMap[QStringLiteral("QkmProbe")];
                        result->passed = result->passed && fresh.task_stopping.loadAcquire() == 0
                            && fresh.task_threadpool->waitForDone(1000);
                        SendInputTask::clearSendInputTaskControllerMap();
                    } else if (mode == QStringLiteral("admission")) {
                        result->passed = result->passed && QMetaObject::invokeMethod(worker, "shutdownSendInputTasks", Qt::DirectConnection);
                        worker->onSendInputKeys(-1, keys, KEY_DOWN, QStringLiteral("QkmLate"), SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL, nullptr);
                        worker->onSendInputKeys(-1, keys, KEY_UP, QStringLiteral("QkmLate"), SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL, nullptr);
                    }
                }
                result->cleanupMs = elapsed.elapsed();
#ifndef LOGOUT_TOFILE
                result->passed = result->passed && result->cleanupMs < 1500;
#endif
                result->passed = result->passed && SendInputTask::s_SendInputTaskControllerMap.isEmpty()
                    && result->downStarted.loadAcquire() == result->downFinished.loadAcquire();
                finish(result->passed);
            });
            timer->start(20);
        }, Qt::QueuedConnection);
    });
}
}

Q_COREAPP_STARTUP_FUNCTION(installLifecycleTest)
#define main qkmProductMain
#include "../../main.cpp"
#undef main

int main(int argc, char *argv[])
{
    const QString mode = qEnvironmentVariable("QKM_LIFECYCLE_CASE");
    const QString path = qEnvironmentVariable("QKM_LIFECYCLE_RESULT");
    const QStringList cases{QStringLiteral("wait_clear"), QStringLiteral("pause_clear"), QStringLiteral("queue_keyup"),
        QStringLiteral("cancel_reset"), QStringLiteral("pause_continue"), QStringLiteral("break_one"),
        QStringLiteral("break_all"), QStringLiteral("stop"), QStringLiteral("restart"), QStringLiteral("admission"),
        QStringLiteral("exit_wait"), QStringLiteral("exit_pause"), QStringLiteral("borrowers")};
    if (!cases.contains(mode) || path.isEmpty()) { return 2; }
    result = new LifecycleResult;
    const int code = qkmProductMain(argc, argv);
    const bool passed = code == 0 && result->passed && SendInputTask::s_SendInputTaskControllerMap.isEmpty()
        && result->downStarted.loadAcquire() == result->downFinished.loadAcquire();
    QJsonObject evidence{{QStringLiteral("case"), mode}, {QStringLiteral("passed"), passed},
        {QStringLiteral("activeAtCleanup"), result->activeAtCleanup}, {QStringLiteral("pauseAtCleanup"), result->pauseAtCleanup},
        {QStringLiteral("cleanupMs"), result->cleanupMs}, {QStringLiteral("downStarted"), result->downStarted.loadAcquire()},
        {QStringLiteral("downFinished"), result->downFinished.loadAcquire()}, {QStringLiteral("upStarted"), result->upStarted.loadAcquire()},
        {QStringLiteral("upFinished"), result->upFinished.loadAcquire()}, {QStringLiteral("cancelledDown"), result->cancelledDown.loadAcquire()},
        {QStringLiteral("borrowerCalls"), result->borrowerCalls.loadAcquire()}};
    delete result;
    result = nullptr;
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(QJsonDocument(evidence).toJson()) < 0) { return 2; }
    return passed ? 0 : 2;
}
