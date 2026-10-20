#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSemaphore>
#include <QTimer>
#include "../../qkeymapper_worker.h"
#include "../../qkeymapper.h"

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
    bool observeRepeats = false;
    bool observeSingleRoot = false;
    QMutex traceMutex;
    QStringList repeatTrace;
    QAtomicInt repeatRounds{0};
    int repeatRuns = 0;
};
LifecycleResult *result = nullptr;

class ObservedTask : public SendInputTask {
public:
    using SendInputTask::SendInputTask;
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
        if (result->observeRepeats) {
            if (m_keyupdown == KEY_DOWN && (m_sendmode == SENDMODE_KEYSEQ_NORMAL
                || (result->observeSingleRoot && m_sendmode == SENDMODE_NORMAL))) {
                QMutexLocker locker(&result->traceMutex);
                result->repeatTrace.append(expandRepeatKeys(expandMacroKeys(m_inputKeys)));
            }
            if (m_keyupdown == KEY_UP && m_original_key.endsWith(QStringLiteral(":") + KEYSEQUENCE_FINAL_STR)) {
                result->repeatRounds.fetchAndAddRelease(1);
            }
        }
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

struct RepeatSpec {
    QString name, mapping, macro, first, following;
    int repeatMode = REPEAT_MODE_BYTIMES;
    int times = 3;
    int cycles = 3;
    QString action;
    int seedCount = 0;
    bool unbreakable = false;
};

QList<RepeatSpec> repeatSpecs()
{
    const QString macro = QStringLiteral("Macro(测试宏1)");
    const QString twice = macro + QStringLiteral("x2");
    const QString a = QStringLiteral("QkmProbeA⏱2");
    const QString b = QStringLiteral("QkmProbeB⏱3");
    const QString body = a + QStringLiteral("+") + b + QStringLiteral("»QkmProbeC»QkmProbeD⏱4");
    const QString pair = body + QStringLiteral("»") + body;
    const QString once = QStringLiteral("OnlyOnce{") + a + QStringLiteral("}»") + b;
    const QString ab = a + QStringLiteral("»") + b;
    const QString abab = ab + QStringLiteral("»") + ab;
    const QString bb = b + QStringLiteral("»") + b;
    return {
        {QStringLiteral("repeat_macro_3"), macro, body, body, body},
        {QStringLiteral("repeat_macro_x2_off"), twice, body, pair, pair, REPEAT_MODE_NONE, 1, 1},
        {QStringLiteral("repeat_macro_x2_1"), twice, body, pair, pair, REPEAT_MODE_BYTIMES, 1, 1},
        {QStringLiteral("repeat_macro_x2_3"), twice, body, pair, pair},
        {QStringLiteral("repeat_macro_x2_unlimited"), twice, body, pair, pair, REPEAT_MODE_BYTIMES, 0, 5, QStringLiteral("break")},
        {QStringLiteral("repeat_universal"), QStringLiteral("UniversalMacro(测试宏1)x2"), body, pair, pair},
        {QStringLiteral("repeat_single"), macro, a, a, a},
        {QStringLiteral("repeat_single_unlimited"), macro, a, a, a, REPEAT_MODE_BYTIMES, 0, 5, QStringLiteral("retrigger")},
        {QStringLiteral("repeat_single_off"), macro, a, a, a, REPEAT_MODE_NONE, 1, 1},
        {QStringLiteral("repeat_max"), macro, body, body, body, REPEAT_MODE_BYTIMES, REPEAT_TIMES_MAX, 1, {}, REPEAT_TIMES_MAX - 1},
        {QStringLiteral("repeat_onlyonce"), twice, once, abab, bb},
        {QStringLiteral("repeat_onlyonce_unlimited"), macro, once, ab, b, REPEAT_MODE_BYTIMES, 0, 5, QStringLiteral("break")},
        {QStringLiteral("repeat_nested_onlyonce"), twice, QStringLiteral("Macro(内层)"), abab, bb},
        {QStringLiteral("repeat_nested_repeat"), macro, QStringLiteral("Repeat{") + once + QStringLiteral("}x2"), abab, bb},
        {QStringLiteral("repeat_filtered_single"), once, body, ab, b},
        {QStringLiteral("repeat_all_onlyonce"), twice, QStringLiteral("OnlyOnce{") + a + QStringLiteral("}"), a + QStringLiteral("»") + a, {}, REPEAT_MODE_BYTIMES, 0, 1},
        {QStringLiteral("repeat_top_onlyonce"), QStringLiteral("OnlyOnce{") + ab + QStringLiteral("}"), body, ab, {}, REPEAT_MODE_BYTIMES, 0, 1},
        {QStringLiteral("repeat_missing"), QStringLiteral("Macro(不存在)"), body, {}, {}, REPEAT_MODE_BYTIMES, 0, 0},
        {QStringLiteral("repeat_empty"), macro, {}, {}, {}, REPEAT_MODE_BYTIMES, 0, 0},
        {QStringLiteral("repeat_recursive"), macro, macro, {}, {}, REPEAT_MODE_BYTIMES, 0, 0},
        {QStringLiteral("repeat_overflow"), macro, QStringLiteral("Repeat{Repeat{QkmProbeA}x%1}x%2").arg(REPEAT_COUNT_MAX).arg(KEY_SEQUENCE_MAX / REPEAT_COUNT_MAX + 1), {}, {}, REPEAT_MODE_BYTIMES, 0, 0},
        {QStringLiteral("repeat_direct"), body, body, body, body},
        {QStringLiteral("repeat_bykey"), twice, body, pair, pair, REPEAT_MODE_BYKEY, 0, 5, QStringLiteral("release")},
        {QStringLiteral("repeat_retrigger"), twice, body, pair, pair, REPEAT_MODE_BYTIMES, 0, 5, QStringLiteral("retrigger")},
        {QStringLiteral("repeat_stop"), twice, body, pair, pair, REPEAT_MODE_BYTIMES, 0, 5, QStringLiteral("stop")},
        {QStringLiteral("repeat_restart"), twice, body, pair, pair, REPEAT_MODE_BYTIMES, 0, 5, QStringLiteral("restart")},
        {QStringLiteral("repeat_unbreakable"), twice, body, pair, pair, REPEAT_MODE_BYTIMES, 0, 5, QStringLiteral("unbreakable"), 0, true}
    };
}

void installRepeatTest(QKeyMapper_Worker *worker, const QString &mode)
{
    RepeatSpec spec;
    for (const auto &candidate : repeatSpecs()) {
        if (candidate.name == mode) { spec = candidate; break; }
    }
    auto *previousData = QKeyMapper::KeyMappingDataList;
    const int previousHookState = QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire();
    // Exercise running mappings without installing hardware hooks.
    QKeyMapper_Worker::s_AtomicHookProcState.storeRelease(HOOKPROC_STATE_STARTED);
    auto *data = new QList<MAP_KEYDATA>{MAP_KEYDATA()};
    auto &row = data->first();
    row.Original_Key = QStringLiteral("QkmRepeatProbe");
    row.Mapping_Keys = splitMappingKeyString(spec.mapping, SPLIT_WITH_NEXT);
    row.Pure_OriginalKeys = QStringList{row.Original_Key};
    row.RepeatMode = spec.repeatMode;
    row.RepeatTimes = spec.times;
    row.Unbreakable = spec.unbreakable;
    QKeyMapper::KeyMappingDataList = data;
    MappingMacroData macro;
    macro.MappingMacro = spec.macro;
    QKeyMapper::s_MappingMacroList.insert(QStringLiteral("测试宏1"), macro);
    QKeyMapper::s_UniversalMappingMacroList.insert(QStringLiteral("测试宏1"), macro);
    macro.MappingMacro = QStringLiteral("OnlyOnce{QkmProbeA⏱2}»QkmProbeB⏱3");
    QKeyMapper::s_MappingMacroList.insert(QStringLiteral("内层"), macro);
    result->observeRepeats = true;
    result->observeSingleRoot = mode == QStringLiteral("repeat_single_off");
    // Observe the real runnable, including sequence tasks generated by other tasks.
    QObject::disconnect(worker, &QKeyMapper_Worker::sendInputKeys_Signal, worker, &QKeyMapper_Worker::onSendInputKeys);
    QObject::connect(worker, &QKeyMapper_Worker::sendInputKeys_Signal, worker,
        [](int index, const QStringList &keys, int direction, const QString &original, int sendmode, int state, QList<MAP_KEYDATA> *mappingData) {
            auto *task = new ObservedTask(index, keys, direction, original, sendmode, state, mappingData);
            SendInputTask::s_SendInputTaskControllerMap[task->m_real_originalkey].task_threadpool->start(task);
        }, Qt::QueuedConnection);
    auto trigger = [worker, data](bool release) {
        auto &mapping = data->first();
        worker->emit_sendInputKeysSignal_Wrapper(0, mapping.Mapping_Keys, KEY_DOWN, mapping.Original_Key,
            SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL, data);
        if (release) {
            worker->emit_sendInputKeysSignal_Wrapper(0, mapping.Mapping_Keys, KEY_UP, mapping.Original_Key,
                SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL, data);
        }
    };
    if (spec.repeatMode == REPEAT_MODE_BYKEY) {
        QKeyMapper_Worker::pressedRealKeysList.append(row.Original_Key);
    }
    trigger(spec.repeatMode != REPEAT_MODE_BYKEY);
    if (spec.seedCount > 0) {
        QKeyMapper_Worker::s_KeySequenceRepeatCount[row.Original_Key] = spec.seedCount;
    }
    auto *timer = new QTimer(worker);
    QObject::connect(timer, &QTimer::timeout, worker,
        [worker, timer, data, previousData, previousHookState, spec, trigger, attempts = 0, quiet = 0, phase = 0, extraRounds = 0]() mutable {
            const QString original = data->first().Original_Key;
            const int rounds = result->repeatRounds.loadAcquire();
            ++attempts;
            bool idle = !QKeyMapper_Worker::s_runningKeySequenceOrikeyList.contains(original);
            for (const auto &entry : SendInputTask::s_SendInputTaskControllerMap) {
                idle = idle && entry.task_threadpool->activeThreadCount() == 0;
            }
            quiet = idle ? quiet + 1 : 0;
            if (phase == 0 && !spec.action.isEmpty() && rounds >= spec.cycles) {
                QStringList trace;
                { QMutexLocker locker(&result->traceMutex); trace = result->repeatTrace; }
                QString expected = spec.first;
                for (int cycle = 1; cycle < spec.cycles; ++cycle) { expected += QStringLiteral("»") + spec.following; }
                const auto prefix = splitMappingKeyString(expected, SPLIT_WITH_NEXT);
                result->passed = result->passed && trace.mid(0, prefix.size()) == prefix;
                if (spec.repeatMode == REPEAT_MODE_BYTIMES) {
                    result->passed = result->passed && QKeyMapper_Worker::s_KeySequenceRepeatCount.value(original, -2) == 0;
                }
                if (spec.action == QStringLiteral("unbreakable")) {
                    trigger(true);
                    extraRounds = rounds + 2;
                    phase = 1;
                    return;
                }
                if (spec.action == QStringLiteral("retrigger")) { trigger(true); }
                else if (spec.action == QStringLiteral("stop")) { worker->setWorkerKeyUnHook(); }
                else if (spec.action == QStringLiteral("restart")) { worker->setKeyMappingRestart(); }
                else if (spec.action == QStringLiteral("release")) {
                    QKeyMapper_Worker::pressedRealKeysList.removeAll(original);
                    worker->emit_sendInputKeysSignal_Wrapper(0, data->first().Mapping_Keys, KEY_UP, data->first().Original_Key,
                        SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL, data);
                } else { QKeyMapper_Worker::breakRunningKeySequence(original); }
                phase = 2;
                quiet = 0;
                return;
            }
            if (phase == 1 && rounds >= extraRounds) {
                QKeyMapper_Worker::breakRunningKeySequence(original);
                phase = 2;
                quiet = 0;
                return;
            }
            if (phase == 2 && quiet >= 3) {
                result->passed = result->passed && !QKeyMapper_Worker::s_KeySequenceRepeatCount.contains(original)
                    && QKeyMapper_Worker::pressedMappingKeysMap.isEmpty();
                QKeyMapper::KeyMappingDataList = data;
                QKeyMapper_Worker::s_AtomicHookProcState.storeRelease(HOOKPROC_STATE_STARTED);
                data->first().RepeatMode = REPEAT_MODE_BYTIMES;
                data->first().RepeatTimes = 3;
                data->first().Unbreakable = false;
                { QMutexLocker locker(&result->traceMutex); result->repeatTrace.clear(); }
                result->repeatRounds.storeRelease(0);
                ++result->repeatRuns;
                phase = 3;
                quiet = 0;
                trigger(true);
                return;
            }
            if (quiet < 3 && attempts < 600) { return; }
            timer->stop();
            const bool timedOut = attempts >= 600;
            QString expected = spec.first;
            const int cycles = phase == 3 ? 3 : spec.cycles;
            for (int cycle = 1; cycle < cycles; ++cycle) {
                if (!spec.following.isEmpty()) { expected += QStringLiteral("»") + spec.following; }
            }
            if (phase < 2 && !spec.action.isEmpty()) { result->passed = false; }
            QKeyMapper_Worker::breakAllRunningKeySequence();
            SendInputTask::clearSendInputTaskControllerMap();
            {
                QMutexLocker locker(&result->traceMutex);
                result->passed = result->passed && !timedOut
                    && result->repeatTrace == splitMappingKeyString(expected, SPLIT_WITH_NEXT);
            }
            result->passed = result->passed && !QKeyMapper_Worker::s_KeySequenceRepeatCount.contains(original)
                && !QKeyMapper_Worker::s_runningKeySequenceOrikeyList.contains(original)
                && QKeyMapper_Worker::pressedMappingKeysMap.isEmpty();
            QKeyMapper_Worker::pressedRealKeysList.removeAll(original);
            QKeyMapper::KeyMappingDataList = previousData;
            QKeyMapper_Worker::s_AtomicHookProcState.storeRelease(previousHookState);
            delete data;
            timer->deleteLater();
            finish(result->passed);
        });
    timer->start(20);
}

void installLifecycleTest()
{
    QTimer::singleShot(0, QCoreApplication::instance(), []() {
        auto *worker = QKeyMapper_Worker::getInstance();
        QMetaObject::invokeMethod(worker, [worker]() {
            const QString mode = qEnvironmentVariable("QKM_LIFECYCLE_CASE");
            if (mode.startsWith(QStringLiteral("repeat_"))) {
                installRepeatTest(worker, mode);
                return;
            }
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
    bool knownCase = cases.contains(mode);
    for (const auto &spec : repeatSpecs()) { knownCase = knownCase || mode == spec.name; }
    if (!knownCase || path.isEmpty()) { return 2; }
    result = new LifecycleResult;
    const int code = qkmProductMain(argc, argv);
    const bool passed = code == 0 && result->passed && SendInputTask::s_SendInputTaskControllerMap.isEmpty()
        && result->downStarted.loadAcquire() == result->downFinished.loadAcquire();
    QJsonObject evidence{{QStringLiteral("case"), mode}, {QStringLiteral("passed"), passed},
        {QStringLiteral("activeAtCleanup"), result->activeAtCleanup}, {QStringLiteral("pauseAtCleanup"), result->pauseAtCleanup},
        {QStringLiteral("cleanupMs"), result->cleanupMs}, {QStringLiteral("downStarted"), result->downStarted.loadAcquire()},
        {QStringLiteral("downFinished"), result->downFinished.loadAcquire()}, {QStringLiteral("upStarted"), result->upStarted.loadAcquire()},
        {QStringLiteral("upFinished"), result->upFinished.loadAcquire()}, {QStringLiteral("cancelledDown"), result->cancelledDown.loadAcquire()},
        {QStringLiteral("borrowerCalls"), result->borrowerCalls.loadAcquire()},
        {QStringLiteral("repeatRounds"), result->repeatRounds.loadAcquire()},
        {QStringLiteral("repeatRuns"), result->repeatRuns},
        {QStringLiteral("repeatTrace"), QJsonArray::fromStringList(result->repeatTrace)}};
    delete result;
    result = nullptr;
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(QJsonDocument(evidence).toJson()) < 0) { return 2; }
    return passed ? 0 : 2;
}
