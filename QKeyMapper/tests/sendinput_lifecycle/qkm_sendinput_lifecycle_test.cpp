#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QSemaphore>
#include <QTimer>
#include <QToolButton>
#include <QMouseEvent>
#include "../../qkeymapper_worker.h"
#include "../../qkeymapper.h"
#include "../../qkm_input_producer_work.h"
#include "../../qkm_input_owner_fence.h"
#include "../../interception_worker.h"

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
    bool stoppedBeforeSubmission = false;
    bool controllersEmptyBeforeDispatch = false;
    bool pendingAfterDispatch = false;
    int stopDownQueued = 0;
    int stopUpQueued = 0;
    quint64 workAtSubmission = 0;
    quint64 workAtBarrier = 0;
    quint64 workAtSecondBarrier = 0;
    quint64 workAfterQueueClear = 0;
    quint64 workAfterJoin = 0;
    bool sourceGateBefore = false;
    bool sourceGateAfter = false;
    int idleStatesChecked = 0;
    int callbackDownsTracked = 0;
    int callbackUpsTracked = 0;
    QStringList inputProducerTrace;
    int producerSubmissions = 0;
    quint64 producersWhileTaskWaiting = 0;
    quint64 fenceFirstGeneration = 0;
    quint64 fenceLastGeneration = 0;
    quint64 fenceCancelledGeneration = 0;
    int fenceReachedCount = 0;
    int fenceGuiTurns = 0;
    int fenceOwnersAtRelease = 0;
    bool fencePendingAtRelease = false;
    bool fenceCancelledHookIgnored = false;
    bool fenceInterceptionContextAvailable = false;
    QStringList ownerFenceTrace;
    bool burstCycleTimerDestroyed = false;
    bool burstPressTimerDestroyed = false;
    bool burstSourceContainsKeyAtCleanup = false;
    QAtomicInt drainGuiTurns{0};
    QStringList drainTrace;
    QAtomicInt guiStopDowns{0};
    QAtomicInt guiStopUps{0};
    int guiStopUnhooks = 0;
    int guiStopTimeouts = 0;
    int guiStopCompletions = 0;
    int guiStopTurns = 0;
    bool guiStopBlockedBeforeUnhook = false;
    bool guiStopBlockedAtTimeout = false;
    bool guiStopAdmissionRejected = false;
    QStringList guiStopTrace;
    bool drainResourcesIntact = false;
    bool drainFinalPoolsFinished = false;
    int buttonLockAfterDispatch = 0;
    bool observeRepeats = false;
    bool observeSingleRoot = false;
    QMutex traceMutex;
    QStringList repeatTrace;
    QAtomicInt repeatRounds{0};
    int repeatRuns = 0;
    QStringList lifecycleDispatchTrace;
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

void installOwnerFenceTest(bool cancel)
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), [cancel] {
        auto *fence = new InputOwnerFence(QCoreApplication::instance());
        auto *hook = QKeyMapper_Hook_Proc::getInstance();
        const auto entered = QSharedPointer<QSemaphore>::create();
        const auto blocked = QSharedPointer<QSemaphore>::create();
        result->fenceInterceptionContextAvailable = Interception_Worker::getInterceptionContext() != nullptr;
        QObject::connect(fence, &InputOwnerFence::ownerObserved, fence, [](quint64 generation, int owners) {
            result->ownerFenceTrace.append(QStringLiteral("%1:%2").arg(generation).arg(owners));
        });
        QObject::connect(fence, &InputOwnerFence::reached, fence, [fence, cancel](quint64 generation) {
            ++result->fenceReachedCount;
            const bool passed = generation == result->fenceLastGeneration && !fence->pending()
                && fence->observedOwners() == InputOwnerFence::All && result->fenceReachedCount == 1
                && result->fenceLastGeneration > result->fenceFirstGeneration && result->fenceGuiTurns > 0
                && result->fencePendingAtRelease
                && (!cancel || (result->fenceCancelledHookIgnored
                    && generation > result->fenceCancelledGeneration));
            fence->deleteLater();
            finish(passed);
        });
        if (cancel) {
            QObject::connect(hook, &QKeyMapper_Hook_Proc::inputFenceReached_Signal, fence,
                [fence](quint64 generation) {
                    if (generation == result->fenceCancelledGeneration) {
                        result->fenceCancelledHookIgnored = !fence->pending() && result->fenceReachedCount == 0;
                        result->fenceLastGeneration = fence->request();
                    }
                }, Qt::QueuedConnection);
        }
        QMetaObject::invokeMethod(hook, [entered, blocked] {
            entered->release();
            blocked->acquire();
        }, Qt::QueuedConnection);
        result->fenceFirstGeneration = fence->request();
        result->fenceLastGeneration = fence->request();
        auto *poll = new QTimer(fence);
        QObject::connect(poll, &QTimer::timeout, fence, [fence, poll, entered, blocked, cancel] {
            ++result->fenceGuiTurns;
            if (entered->available() == 0
                || fence->observedOwners() != (InputOwnerFence::Worker | InputOwnerFence::Interception)) { return; }
            poll->stop();
            result->fenceOwnersAtRelease = fence->observedOwners();
            result->fencePendingAtRelease = fence->pending() && result->fenceReachedCount == 0;
            if (cancel) {
                result->fenceCancelledGeneration = result->fenceLastGeneration;
                fence->cancel();
            }
            blocked->release();
        });
        poll->start(1);
        QTimer::singleShot(3000, fence, [fence, blocked] {
            fence->cancel();
            blocked->release();
            finish(false);
        });
    }, Qt::QueuedConnection);
}

void installSessionLockQueueTest(QKeyMapper_Worker *worker)
{
    result->sourceGateBefore = QKeyMapper_Worker::s_SourceReadAccess.runtimeObserved();
    result->passed = !result->sourceGateBefore && SendInputWorkItem::outstanding() == 0
        && QKeyMapper::getInstance()->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE
        && QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() == HOOKPROC_STATE_STOPPED
        && QKeyMapper_Worker::pressedRealKeysList.isEmpty();
    if (!result->passed) { finish(false); return; }
    QKeyMapper_Worker::pressedRealKeysList.append(QStringLiteral("QkmSessionProbe"));
    QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput.append(QStringLiteral("QkmSessionProbe"));
    emit worker->sessionLockStateChanged_Signal(true, QSharedPointer<SendInputWorkItem>::create());
    emit worker->sessionLockStateChanged_Signal(false, QSharedPointer<SendInputWorkItem>::create());
    result->workAtSubmission = SendInputWorkItem::outstanding();
    result->sourceGateAfter = QKeyMapper_Worker::s_SourceReadAccess.runtimeObserved();
    QMetaObject::invokeMethod(worker, [] {
        result->workAtBarrier = SendInputWorkItem::outstanding();
        finish(result->workAtSubmission == 2 && result->workAtBarrier == 0 && result->sourceGateAfter
            && QKeyMapper_Worker::pressedRealKeysList.isEmpty()
            && QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput.isEmpty()
            && QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() == HOOKPROC_STATE_STOPPED);
    }, Qt::QueuedConnection);
}

void installSendInputDrainTest(QKeyMapper_Worker *worker, const QString &mode)
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), [worker, mode] {
        auto *heartbeat = new QTimer(QCoreApplication::instance());
        QObject::connect(heartbeat, &QTimer::timeout, heartbeat, [] { result->drainGuiTurns.fetchAndAddRelaxed(1); });
        heartbeat->start(5);
        QMetaObject::invokeMethod(worker, [worker, mode, heartbeat] {
            auto *previousData = QKeyMapper::KeyMappingDataList;
            auto *data = new QList<MAP_KEYDATA>{MAP_KEYDATA()};
            auto &row = data->first();
            row.Original_Key = SENDON_MAPPINGSTOP_STR;
            row.Mapping_Keys = QStringList{QStringLiteral("QkmProbeNoOp⏱250")};
            row.Pure_OriginalKeys = QStringList{row.Original_Key};
            QKeyMapper::KeyMappingDataList = data;
            const auto finished = QSharedPointer<bool>::create(false);
            const auto finishTest = [worker, previousData, data, heartbeat, finished] {
                if (*finished) { return; }
                *finished = true;
                QMetaObject::invokeMethod(worker, [previousData, data, heartbeat] {
                    SendInputTask::clearSendInputTaskControllerMap();
                    QKeyMapper::KeyMappingDataList = previousData;
                    delete data;
                    QMetaObject::invokeMethod(heartbeat, [heartbeat] {
                        heartbeat->stop();
                        heartbeat->deleteLater();
                        finish(result->passed && SendInputWorkItem::outstanding() == 0);
                    }, Qt::QueuedConnection);
                }, Qt::QueuedConnection);
            };
            QObject::connect(worker, &QKeyMapper_Worker::sendInputDrainObserved_Signal, worker,
                [worker, mode, finishTest](quint64 generation, const QString &outcome) {
                    result->drainTrace.append(QStringLiteral("%1:%2").arg(generation).arg(outcome));
                    if (outcome == QStringLiteral("timeout") || outcome == QStringLiteral("superseded")
                        || outcome == QStringLiteral("shutdown")) {
                        const auto controller = SendInputTask::s_SendInputTaskControllerMap.constFind(SENDON_MAPPINGSTOP_STR);
                        result->drainResourcesIntact = SendInputWorkItem::outstanding() > 0
                            && controller != SendInputTask::s_SendInputTaskControllerMap.cend()
                            && !controller->task_stopping.loadAcquire() && !SendInputTask::poolsFinished();
                        result->passed = result->passed && result->drainResourcesIntact;
                        if (mode == QStringLiteral("drain_timeout") && generation == 1
                            && outcome == QStringLiteral("timeout")) {
                            worker->observeSendInputDrain(2, 2000);
                            return;
                        }
                        if ((mode == QStringLiteral("drain_supersede") && generation == 1
                                && outcome == QStringLiteral("superseded"))
                            || (mode == QStringLiteral("drain_shutdown") && generation == 1
                                && outcome == QStringLiteral("shutdown"))) { return; }
                        result->passed = false;
                        finishTest();
                        return;
                    }
                    const QStringList expected = mode == QStringLiteral("drain_wait")
                        ? QStringList{QStringLiteral("1:drained")}
                        : mode == QStringLiteral("drain_timeout")
                        ? QStringList{QStringLiteral("1:timeout"), QStringLiteral("2:drained")}
                        : QStringList{QStringLiteral("1:superseded"), QStringLiteral("2:drained")};
                    result->drainFinalPoolsFinished = SendInputTask::poolsFinished();
                    result->passed = result->passed && outcome == QStringLiteral("drained")
                        && result->drainTrace == expected && result->drainFinalPoolsFinished
                        && SendInputWorkItem::outstanding() == 0 && InputProducerWorkItem::outstanding() == 0
                        && result->drainGuiTurns.loadAcquire() > 0;
                    finishTest();
                }, Qt::DirectConnection);
            worker->emit_sendInputKeysSignal_Wrapper(0, row.Mapping_Keys, KEY_DOWN, row.Original_Key,
                SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL, data);
            worker->emit_sendInputKeysSignal_Wrapper(0, row.Mapping_Keys, KEY_UP, row.Original_Key,
                SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL, data);
            QMetaObject::invokeMethod(worker, [worker, mode, finishTest] {
                worker->observeSendInputDrain(1, mode == QStringLiteral("drain_timeout") ? 25 : 2000);
                if (mode == QStringLiteral("drain_supersede")) {
                    worker->observeSendInputDrain(2, 2000);
                    worker->observeSendInputDrain(1, 2000);
                }
                if (mode == QStringLiteral("drain_shutdown")) {
                    const bool invoked = QMetaObject::invokeMethod(worker, "shutdownSendInputTasks", Qt::DirectConnection);
                    result->passed = result->passed && invoked
                        && result->drainTrace == QStringList{QStringLiteral("1:shutdown")};
                    finishTest();
                }
            }, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

void clickMappingButton(QToolButton *button)
{
    const QPoint local(8, button->height() / 2);
    const QPointF window = button->mapTo(button->window(), local);
    const QPointF global = button->mapToGlobal(local);
    QMouseEvent press(QEvent::MouseButtonPress, local, window, global,
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, local, window, global,
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(button, &press);
    QCoreApplication::sendEvent(button, &release);
}

void installGuiStopTest(QKeyMapper_Worker *worker, const QString &mode)
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), [worker, mode] {
        auto *mapper = QKeyMapper::getInstance();
        auto *button = mapper->findChild<QToolButton *>(QStringLiteral("keymapButton"));
        auto *editor = mapper->findChild<QWidget *>(QStringLiteral("settingNameLineEdit"));
        const int tab = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
        if (!button || !editor || !button->isEnabled() || mapper->m_KeyMapStatus != QKeyMapper::KEYMAP_IDLE
            || tab < 0 || tab >= QKeyMapper::s_KeyMappingTabInfoList.size()
            || QKeyMapper::isCommonMappingTabIndex(tab)) { finish(false); return; }
        auto *source = QKeyMapper::s_KeyMappingTabInfoList.at(tab).KeyMappingData;
        const auto backup = QSharedPointer<QList<MAP_KEYDATA>>::create(*source);
        MAP_KEYDATA row;
        row.Original_Key = SENDON_MAPPINGSTOP_STR;
        row.Mapping_Keys = QStringList{QStringLiteral("QkmProbeNoOp⏱250")};
        // Product startup expects the same derived fields and table rows as a loaded mapping.
        row.MappingKeys_KeyUp = row.Mapping_Keys;
        row.Pure_OriginalKeys = QStringList{row.Original_Key};
        row.Pure_MappingKeys = QStringList{QStringLiteral("QkmProbeNoOp")};
        row.Pure_MappingKeys_KeyUp = row.Pure_MappingKeys;
        *source = QList<MAP_KEYDATA>{row};
        mapper->refreshKeyMappingDataTableByTabIndex(tab);
        auto *table = QKeyMapper::s_KeyMappingTabInfoList.at(tab).KeyMappingDataTable;
        if (!table || !table->item(0, LOCK_COLUMN)) { *source = *backup; finish(false); return; }
        auto *context = new QObject(mapper);
        const auto hold = QSharedPointer<QSemaphore>::create();
        const auto entered = QSharedPointer<QSemaphore>::create();
        const auto deadline = QSharedPointer<QElapsedTimer>::create();
        deadline->start();
        QObject::connect(worker, &QKeyMapper_Worker::sendInputKeys_Signal, context,
            [](int, const QStringList &, int direction, const QString &original, int, int, QList<MAP_KEYDATA> *) {
                if (original != SENDON_MAPPINGSTOP_STR) { return; }
                if (direction == KEY_DOWN) { result->guiStopDowns.fetchAndAddRelease(1); }
                else { result->guiStopUps.fetchAndAddRelease(1); }
            }, Qt::DirectConnection);
        QObject::connect(worker, &QKeyMapper_Worker::setKeyUnHook_Signal, context,
            [] { ++result->guiStopUnhooks; }, Qt::DirectConnection);
        QObject::connect(mapper, &QKeyMapper::mappingCleanupTimedOut_Signal, context,
            [mapper, button, editor, hold](quint64 operation) {
                ++result->guiStopTimeouts;
                result->guiStopTrace.append(QStringLiteral("%1:timeout").arg(operation));
                result->guiStopBlockedAtTimeout = mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_STOPPING
                    && QKeyMapper_Worker::s_MappingInputClosed.loadAcquire()
                    && !button->isEnabled() && !editor->isEnabled() && result->guiStopUnhooks == 0
                    && result->guiStopCompletions == 0;
                result->passed = result->passed && result->guiStopBlockedAtTimeout;
                QTimer::singleShot(20, mapper, [hold] { hold->release(); });
            });
        QObject::connect(mapper, &QKeyMapper::mappingCleanupCompleted_Signal, context,
            [mapper, button, editor, context, source, backup, mode](quint64 operation, bool timedOut) {
                ++result->guiStopCompletions;
                result->guiStopTrace.append(QStringLiteral("%1:completed:%2").arg(operation).arg(timedOut));
                result->passed = result->passed && mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE
                    && !QKeyMapper_Worker::s_MappingInputClosed.loadAcquire() && button->isEnabled() && editor->isEnabled()
                    && result->guiStopUnhooks == 1 && result->guiStopDowns.loadAcquire() == 1
                    && result->guiStopUps.loadAcquire() == 1 && result->guiStopCompletions == 1
                    && result->guiStopBlockedBeforeUnhook && result->guiStopAdmissionRejected
                    && result->guiStopTimeouts == (mode == QStringLiteral("gui_stop_timeout") ? 1 : 0)
                    && timedOut == (mode == QStringLiteral("gui_stop_timeout"))
                    && SendInputWorkItem::outstanding() == 0 && InputProducerWorkItem::outstanding() == 0
                    && SendInputTask::poolsFinished() && result->guiStopTurns > 1;
                *source = *backup;
                QKeyMapper::restoreKeyMappingDataListPointer();
                context->deleteLater();
                finish(result->passed);
            });
        // Use widget mouse events for the normal GUI entry, without injecting system input.
        clickMappingButton(button);
        auto *poll = new QTimer(context);
        QObject::connect(poll, &QTimer::timeout, context,
            [worker, mapper, button, editor, context, hold, entered, source, backup, mode, deadline] {
                ++result->guiStopTurns;
                if (deadline->elapsed() > 5000) {
                    hold->release();
                    finish(false);
                    return;
                }
                if (result->guiStopTrace.isEmpty()) {
                    if (QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() != HOOKPROC_STATE_STARTED) { return; }
                    result->guiStopTrace.append(QStringLiteral("started"));
                    QMetaObject::invokeMethod(QKeyMapper_Hook_Proc::getInstance(), [hold, entered] {
                        const InputProducerWorkItem inputWork;
                        entered->release();
                        hold->acquire();
                    }, Qt::QueuedConnection);
                    return;
                }
                if (result->guiStopTrace.size() != 1 || entered->available() == 0) { return; }
                result->guiStopTrace.append(QStringLiteral("stopping"));
                clickMappingButton(button);
                result->guiStopBlockedBeforeUnhook = mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_STOPPING
                    && QKeyMapper_Worker::s_MappingInputClosed.loadAcquire()
                    && !button->isEnabled() && !editor->isEnabled() && result->guiStopUnhooks == 0;
                // A queued repeat request must neither restart mapping nor duplicate stop actions.
                mapper->MappingSwitch(QKeyMapper::MAPPINGSTART_BUTTONCLICK);
                result->guiStopAdmissionRejected = !worker->queueVButtonKey(QStringLiteral("VButton1"), true);
                if (mode == QStringLiteral("gui_stop_exit")) {
                    result->passed = result->passed && result->guiStopBlockedBeforeUnhook
                        && result->guiStopAdmissionRejected && result->guiStopCompletions == 0;
                    *source = *backup;
                    // The pending operation has not submitted Worker stop or borrowed the synthetic row.
                    hold->release();
                    finish(result->passed);
                }
                else if (mode == QStringLiteral("gui_stop_timeout")) {
                    auto *timer = mapper->findChild<QTimer *>(QStringLiteral("mappingStopDeadline"));
                    if (!timer) { hold->release(); finish(false); return; }
                    timer->start(30);
                }
                else {
                    QTimer::singleShot(50, context, [hold] { hold->release(); });
                }
            });
        poll->start(5);
    }, Qt::QueuedConnection);
}

void installBurstTimerCleanupTest(QKeyMapper_Worker *worker, const QString &mode)
{
    if (!QKeyMapper_Worker::s_BurstKeyTimerMap.isEmpty() || !QKeyMapper_Worker::s_BurstKeyPressTimerMap.isEmpty()
        || QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() != HOOKPROC_STATE_STOPPED) { finish(false); return; }
    auto *previousData = QKeyMapper::KeyMappingDataList;
    auto *data = new QList<MAP_KEYDATA>{MAP_KEYDATA()};
    auto *otherData = new QList<MAP_KEYDATA>;
    const QString original = QStringLiteral("VButton1");
    const QStringList keys{QStringLiteral("QkmProbeNoOp")};
    auto &row = data->first();
    row.Original_Key = original;
    row.Mapping_Keys = keys;
    row.Pure_OriginalKeys = QStringList{original};
    row.BurstPressTime = 10000;
    row.BurstReleaseTime = 10000;
    QKeyMapper::KeyMappingDataList = data;
    worker->startBurstKeyTimer(original, 0, data);
    QPointer<QTimer> cycleTimer = QKeyMapper_Worker::s_BurstKeyTimerMap.value(original);
    QPointer<QTimer> pressTimer = QKeyMapper_Worker::s_BurstKeyPressTimerMap.value(original);
    result->passed = cycleTimer && pressTimer && cycleTimer->isActive() && pressTimer->isActive();
    if (mode == QStringLiteral("burst_timer_missing_source")) { QKeyMapper::KeyMappingDataList = otherData; }
    if (mode == QStringLiteral("burst_timer_removed_source")) { data->clear(); }
    result->burstSourceContainsKeyAtCleanup = QKeyMapper::findOriKeyInKeyMappingDataList(original) >= 0;
    worker->setWorkerKeyUnHook();
    result->burstCycleTimerDestroyed = cycleTimer.isNull();
    result->burstPressTimerDestroyed = pressTimer.isNull();
    result->passed = result->passed && result->burstCycleTimerDestroyed && result->burstPressTimerDestroyed
        && QKeyMapper_Worker::s_BurstKeyTimerMap.isEmpty() && QKeyMapper_Worker::s_BurstKeyPressTimerMap.isEmpty();
    // Reclaim a failing baseline's orphan before any timeout can touch the synthetic source.
    if (cycleTimer) { cycleTimer->stop(); delete cycleTimer.data(); }
    if (pressTimer) { pressTimer->stop(); delete pressTimer.data(); }
    QMetaObject::invokeMethod(worker, [worker, previousData, data, otherData, original, keys] {
        SendInputTask::clearSendInputTaskControllerMap();
        worker->sendInputKeys(-1, keys, KEY_UP, original, SENDMODE_FORCE_STOP,
            SendInputTask::s_GlobalSendInputTaskController);
        QKeyMapper::KeyMappingDataList = previousData;
        delete data;
        delete otherData;
        finish(result->passed && SendInputWorkItem::outstanding() == 0
            && QKeyMapper_Worker::pressedMappingKeysMap.isEmpty());
    }, Qt::QueuedConnection);
}

void installQueuedStopActionTest(QKeyMapper_Worker *worker, const QString &mode)
{
    const bool reject = mode == QStringLiteral("queued_admission");
    const bool discard = mode == QStringLiteral("discard_dispatch");
    if (reject) {
        result->passed = QMetaObject::invokeMethod(worker, "shutdownSendInputTasks", Qt::DirectConnection);
    }
    QObject *receiver = nullptr;
    if (discard) {
        QObject::disconnect(worker, &QKeyMapper_Worker::sendInputKeys_Signal, worker, &QKeyMapper_Worker::onSendInputKeys);
        receiver = new QObject(worker);
        QObject::connect(worker, &QKeyMapper_Worker::sendInputKeys_Signal, receiver, [] {}, Qt::QueuedConnection);
    }
    auto *previousData = QKeyMapper::KeyMappingDataList;
    auto *data = new QList<MAP_KEYDATA>{MAP_KEYDATA()};
    auto &row = data->first();
    row.Original_Key = SENDON_MAPPINGSTOP_STR;
    row.Mapping_Keys = QStringList{QStringLiteral("QkmProbeNoOp⏱5000")};
    row.Pure_OriginalKeys = QStringList{row.Original_Key};
    QKeyMapper::KeyMappingDataList = data;
    result->stoppedBeforeSubmission = QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() == HOOKPROC_STATE_STOPPED;
    const auto observation = QObject::connect(worker, &QKeyMapper_Worker::sendInputKeys_Signal, worker,
        [](int, const QStringList &, int direction, const QString &, int, int, QList<MAP_KEYDATA> *) {
            if (direction == KEY_DOWN) { ++result->stopDownQueued; }
            else { ++result->stopUpQueued; }
            if (InputProducerWorkItem::outstanding() > 0) { ++result->producerSubmissions; }
        }, Qt::DirectConnection);
    // Use the real stop-action submissions without invoking Hook or native device cleanup.
    worker->emit_sendInputKeysSignal_Wrapper(0, row.Mapping_Keys, KEY_DOWN, row.Original_Key,
        SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL, data);
    worker->emit_sendInputKeysSignal_Wrapper(0, row.Mapping_Keys, KEY_UP, row.Original_Key,
        SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL, data);
    QObject::disconnect(observation);
    result->workAtSubmission = SendInputWorkItem::outstanding();
    result->controllersEmptyBeforeDispatch = SendInputTask::s_SendInputTaskControllerMap.isEmpty();
    delete receiver;
    QMetaObject::invokeMethod(worker, [data, previousData, reject, discard]() {
        const auto found = SendInputTask::s_SendInputTaskControllerMap.constFind(SENDON_MAPPINGSTOP_STR);
        result->pendingAfterDispatch = found != SendInputTask::s_SendInputTaskControllerMap.cend()
            && !found->task_threadpool->waitForDone(0);
        result->workAtBarrier = SendInputWorkItem::outstanding();
        if (!reject && !discard) {
            QElapsedTimer started;
            started.start();
            while (InputProducerWorkItem::outstanding() == 0 && started.elapsed() < 1000) {
                QThread::msleep(1);
            }
        }
        result->producersWhileTaskWaiting = InputProducerWorkItem::outstanding();
        result->passed = result->passed && result->stoppedBeforeSubmission && result->controllersEmptyBeforeDispatch
            && result->workAtSubmission == 2
            && ((reject || discard) ? (!result->pendingAfterDispatch && result->workAtBarrier == 0)
                                    : (result->pendingAfterDispatch && result->workAtBarrier > 0))
            && result->stopDownQueued == 1 && result->stopUpQueued == 1
            && result->producerSubmissions == 2
            && result->producersWhileTaskWaiting == ((reject || discard) ? 0 : 1)
            && QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() == HOOKPROC_STATE_STOPPED;
        QElapsedTimer elapsed;
        elapsed.start();
        SendInputTask::clearSendInputTaskControllerMap();
        result->cleanupMs = elapsed.elapsed();
        result->workAfterJoin = SendInputWorkItem::outstanding();
        result->passed = result->passed && SendInputTask::s_SendInputTaskControllerMap.isEmpty()
            && result->workAfterJoin == 0;
        QKeyMapper::KeyMappingDataList = previousData;
        delete data;
        finish(result->passed);
    }, Qt::QueuedConnection);
}

void installQueuedButtonTest(QKeyMapper_Worker *worker, bool reject)
{
    result->sourceGateBefore = QKeyMapper_Worker::s_SourceReadAccess.runtimeObserved();
    result->passed = !result->sourceGateBefore && SendInputWorkItem::outstanding() == 0;
    if (reject) {
        result->passed = result->passed && QMetaObject::invokeMethod(worker, "shutdownSendInputTasks", Qt::DirectConnection);
    }
    auto *previousData = QKeyMapper::KeyMappingDataList;
    auto *data = new QList<MAP_KEYDATA>{MAP_KEYDATA()};
    auto &row = data->first();
    row.Original_Key = QStringLiteral("VButton1");
    row.Mapping_Keys = QStringList{QStringLiteral("QkmProbeNoOp⏱5000")};
    row.Pure_OriginalKeys = QStringList{row.Original_Key};
    row.Lock = reject;
    QKeyMapper::KeyMappingDataList = data;
    const bool downQueued = worker->queueVButtonKey(row.Original_Key, true);
    const bool upQueued = worker->queueVButtonKey(row.Original_Key, false);
    result->sourceGateAfter = QKeyMapper_Worker::s_SourceReadAccess.runtimeObserved();
    result->workAtSubmission = SendInputWorkItem::outstanding();
    result->passed = result->passed && downQueued && upQueued && result->sourceGateAfter
        && result->workAtSubmission == 2 && SendInputTask::s_SendInputTaskControllerMap.isEmpty();
    QMetaObject::invokeMethod(worker, [worker, data, previousData, reject] {
        result->workAtBarrier = SendInputWorkItem::outstanding();
        result->buttonLockAfterDispatch = data->first().LockState;
        result->passed = result->passed && result->workAtBarrier == (reject ? 0 : 2)
            && SendInputTask::s_SendInputTaskControllerMap.isEmpty()
            && result->buttonLockAfterDispatch == LOCK_STATE_LOCKOFF;
        // The button callbacks post sends behind this first queue barrier.
        QMetaObject::invokeMethod(worker, [data, previousData, reject] {
            const auto found = SendInputTask::s_SendInputTaskControllerMap.constFind(data->first().Original_Key);
            result->pendingAfterDispatch = found != SendInputTask::s_SendInputTaskControllerMap.cend()
                && !found->task_threadpool->waitForDone(0);
            result->workAtSecondBarrier = SendInputWorkItem::outstanding();
            result->passed = result->passed && (reject ? (!result->pendingAfterDispatch && result->workAtSecondBarrier == 0)
                : (result->pendingAfterDispatch && result->workAtSecondBarrier > 0));
            SendInputTask::clearSendInputTaskControllerMap();
            result->workAfterJoin = SendInputWorkItem::outstanding();
            result->passed = result->passed && result->workAfterJoin == 0
                && QKeyMapper_Worker::pressedVButtonKeysList.isEmpty()
                && QKeyMapper_Worker::pressedMappingKeysMap.isEmpty();
            QKeyMapper::KeyMappingDataList = previousData;
            delete data;
            finish(result->passed);
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

void installQueuedLifecycleWorkTest(QKeyMapper_Worker *worker, bool discard)
{
    auto *hook = QKeyMapper_Hook_Proc::getInstance();
    auto *workerReceiver = new QObject(worker);
    auto *hookReceiver = new QObject(worker);
    result->sourceGateBefore = QKeyMapper_Worker::s_SourceReadAccess.runtimeObserved();
    result->passed = !result->sourceGateBefore && SendInputWorkItem::outstanding() == 0;
    // Exercise the real signal envelopes without executing native start/stop actions.
    const auto replace = [](auto *sender, auto signal, auto slot, QObject *receiver) {
        result->passed = QObject::disconnect(sender, signal, sender, slot) && result->passed;
        QObject::connect(sender, signal, receiver, [] {
            result->lifecycleDispatchTrace.append(QString::number(SendInputWorkItem::outstanding()));
        }, Qt::QueuedConnection);
    };
    replace(worker, &QKeyMapper_Worker::setKeyHook_Signal, &QKeyMapper_Worker::setWorkerKeyHook, workerReceiver);
    replace(worker, &QKeyMapper_Worker::setKeyUnHook_Signal, &QKeyMapper_Worker::setWorkerKeyUnHook, workerReceiver);
    replace(worker, &QKeyMapper_Worker::setKeyMappingRestart_Signal, &QKeyMapper_Worker::setKeyMappingRestart, workerReceiver);
    replace(hook, &QKeyMapper_Hook_Proc::setKeyHook_Signal, &QKeyMapper_Hook_Proc::onSetHookProcKeyHook, hookReceiver);
    replace(hook, &QKeyMapper_Hook_Proc::setKeyUnHook_Signal, &QKeyMapper_Hook_Proc::onSetHookProcKeyUnHook, hookReceiver);
    replace(hook, &QKeyMapper_Hook_Proc::setKeyMappingRestart_Signal, &QKeyMapper_Hook_Proc::onSetHookProcKeyMappingRestart, hookReceiver);
    {
        const auto work = QSharedPointer<SendInputWorkItem>::create();
        emit worker->setKeyHook_Signal(work);
        emit hook->setKeyHook_Signal(work);
    }
    {
        const auto work = QSharedPointer<SendInputWorkItem>::create();
        emit worker->setKeyUnHook_Signal(work);
        emit hook->setKeyUnHook_Signal(work);
    }
    {
        const auto work = QSharedPointer<SendInputWorkItem>::create();
        emit worker->setKeyMappingRestart_Signal(work);
        emit hook->setKeyMappingRestart_Signal(work);
    }
    result->workAtSubmission = SendInputWorkItem::outstanding();
    result->sourceGateAfter = QKeyMapper_Worker::s_SourceReadAccess.runtimeObserved();
    result->passed = result->passed && result->workAtSubmission == 3 && result->sourceGateAfter
        && SendInputTask::s_SendInputTaskControllerMap.isEmpty();
    if (discard) {
        delete workerReceiver;
        workerReceiver = nullptr;
        result->workAfterQueueClear = SendInputWorkItem::outstanding();
        result->passed = result->passed && result->workAfterQueueClear == 3;
        delete hookReceiver;
        hookReceiver = nullptr;
        result->passed = result->passed && SendInputWorkItem::outstanding() == 0;
    }
    QMetaObject::invokeMethod(worker, [worker, hook, workerReceiver, hookReceiver, discard] {
        result->workAtBarrier = SendInputWorkItem::outstanding();
        const QStringList expected{QStringLiteral("3"), QStringLiteral("3"), QStringLiteral("2"),
            QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("1")};
        result->passed = result->passed && result->workAtBarrier == 0
            && result->lifecycleDispatchTrace == (discard ? QStringList{} : expected)
            && SendInputTask::s_SendInputTaskControllerMap.isEmpty();
        delete workerReceiver;
        delete hookReceiver;
        QObject::connect(worker, &QKeyMapper_Worker::setKeyHook_Signal, worker, &QKeyMapper_Worker::setWorkerKeyHook, Qt::QueuedConnection);
        QObject::connect(worker, &QKeyMapper_Worker::setKeyUnHook_Signal, worker, &QKeyMapper_Worker::setWorkerKeyUnHook, Qt::QueuedConnection);
        QObject::connect(worker, &QKeyMapper_Worker::setKeyMappingRestart_Signal, worker, &QKeyMapper_Worker::setKeyMappingRestart, Qt::QueuedConnection);
        QObject::connect(hook, &QKeyMapper_Hook_Proc::setKeyHook_Signal, hook, &QKeyMapper_Hook_Proc::onSetHookProcKeyHook, Qt::QueuedConnection);
        QObject::connect(hook, &QKeyMapper_Hook_Proc::setKeyUnHook_Signal, hook, &QKeyMapper_Hook_Proc::onSetHookProcKeyUnHook, Qt::QueuedConnection);
        QObject::connect(hook, &QKeyMapper_Hook_Proc::setKeyMappingRestart_Signal, hook, &QKeyMapper_Hook_Proc::onSetHookProcKeyMappingRestart, Qt::QueuedConnection);
        finish(result->passed);
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
            if (mode.startsWith(QStringLiteral("gui_stop_"))) {
                installGuiStopTest(worker, mode);
                return;
            }
            if (mode.startsWith(QStringLiteral("drain_"))) {
                installSendInputDrainTest(worker, mode);
                return;
            }
            if (mode.startsWith(QStringLiteral("burst_timer_"))) {
                installBurstTimerCleanupTest(worker, mode);
                return;
            }
            if (mode == QStringLiteral("owner_fence") || mode == QStringLiteral("owner_fence_cancel")) {
                installOwnerFenceTest(mode == QStringLiteral("owner_fence_cancel"));
                return;
            }
            if (mode == QStringLiteral("session_lock_queue")) {
                installSessionLockQueueTest(worker);
                return;
            }
            if (mode == QStringLiteral("queued_button") || mode == QStringLiteral("button_admission")) {
                installQueuedButtonTest(worker, mode == QStringLiteral("button_admission"));
                return;
            }
            if (mode == QStringLiteral("lifecycle_dispatch") || mode == QStringLiteral("lifecycle_discard")) {
                installQueuedLifecycleWorkTest(worker, mode == QStringLiteral("lifecycle_discard"));
                return;
            }
            if (mode == QStringLiteral("idle_input_tracking") || mode == QStringLiteral("closed_input_tracking")) {
                QMetaObject::invokeMethod(QCoreApplication::instance(), [mode] {
                    auto *mapper = QKeyMapper::getInstance();
                    const auto previousStatus = mapper->m_KeyMapStatus;
                    const int previousTab = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
                    const QString rawKey = QStringLiteral("A@0");
                    const QString pureKey = QStringLiteral("A");
                    if (!QKeyMapper::isInitialized() || previousStatus != QKeyMapper::KEYMAP_IDLE
                        || QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() != HOOKPROC_STATE_STOPPED
                        || !QKeyMapper_Worker::pressedRealKeysList.isEmpty()
                        || !QKeyMapper_Worker::pressedMultiKeyboardVKeyCodeList[0].isEmpty()
                        || QKeyMapper::s_MappingStartKeyString.contains(pureKey)
                        || QKeyMapper::s_MappingStopKeyString.contains(pureKey)
                        || QKeyMapper::s_WindowSwitchKeyString.contains(pureKey)) {
                        finish(false);
                        return;
                    }
                    const auto observation = QObject::connect(mapper,
                        &QKeyMapper::updateKeyLineEditWithRealKeyListChanged_Signal, mapper,
                        [pureKey](const QString &key, int) {
                            result->passed = result->passed && key == pureKey;
                            result->inputProducerTrace.append(QString::number(InputProducerWorkItem::outstanding()));
                        }, Qt::DirectConnection);
                    // Run on the GUI owner lane so the synthetic missing Tab cannot leak into UI work.
                    const bool closed = mode == QStringLiteral("closed_input_tracking");
                    const QList<QKeyMapper::KeyMapStatus> states = closed
                        ? QList<QKeyMapper::KeyMapStatus>{QKeyMapper::KEYMAP_STOPPING}
                        : QList<QKeyMapper::KeyMapStatus>{QKeyMapper::KEYMAP_IDLE, QKeyMapper::KEYMAP_CHECKING};
                    QKeyMapper_Worker::s_MappingInputClosed.storeRelease(closed);
                    if (closed) { QKeyMapper_Worker::s_AtomicHookProcState.storeRelease(HOOKPROC_STATE_STARTED); }
                    for (auto status : states) {
                        mapper->m_KeyMapStatus = status;
                        QKeyMapper::s_KeyMappingTabWidgetCurrentIndex = -1;
                        const int down = QKeyMapper_Worker::InterceptionKeyboardHookProc(0x1e, KEY_DOWN, 0, false, false, 0);
                        result->passed = result->passed && InputProducerWorkItem::outstanding() == 0;
                        if (QKeyMapper_Worker::pressedRealKeysList.contains(rawKey)
                            && QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput.contains(pureKey)
                            && QKeyMapper_Worker::pressedMultiKeyboardVKeyCodeList[0].contains(0x41)) {
                            ++result->callbackDownsTracked;
                        }
                        const int up = QKeyMapper_Worker::InterceptionKeyboardHookProc(0x1e, KEY_UP, 0, false, false, 0);
                        result->passed = result->passed && InputProducerWorkItem::outstanding() == 0;
                        if (!QKeyMapper_Worker::pressedRealKeysList.contains(rawKey)
                            && !QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput.contains(pureKey)
                            && !QKeyMapper_Worker::pressedMultiKeyboardVKeyCodeList[0].contains(0x41)) {
                            ++result->callbackUpsTracked;
                        }
                        result->passed = result->passed && down == INTERCEPTION_RETURN_NORMALSEND
                            && up == INTERCEPTION_RETURN_NORMALSEND;
                        ++result->idleStatesChecked;
                    }
                    QKeyMapper_Worker::s_MappingInputClosed.storeRelease(false);
                    QKeyMapper_Worker::s_AtomicHookProcState.storeRelease(HOOKPROC_STATE_STOPPED);
                    QKeyMapper::s_KeyMappingTabWidgetCurrentIndex = previousTab;
                    mapper->m_KeyMapStatus = previousStatus;
                    QObject::disconnect(observation);
                    finish(result->passed && result->idleStatesChecked == states.size()
                        && result->callbackDownsTracked == states.size() && result->callbackUpsTracked == states.size()
                        && result->inputProducerTrace == (closed ? QStringList{}
                            : QStringList{QStringLiteral("1"), QStringLiteral("1")})
                        && SendInputWorkItem::outstanding() == 0);
                }, Qt::QueuedConnection);
                return;
            }
            if (mode == QStringLiteral("queued_stop_action") || mode == QStringLiteral("queued_admission")
                || mode == QStringLiteral("discard_dispatch")) {
                installQueuedStopActionTest(worker, mode);
                return;
            }
            if (mode.startsWith(QStringLiteral("repeat_"))) {
                installRepeatTest(worker, mode);
                return;
            }
            if (mode == QStringLiteral("direct_task_gate")) {
                result->sourceGateBefore = QKeyMapper_Worker::s_SourceReadAccess.runtimeObserved();
                auto *task = new SendInputTask(-1, {QStringLiteral("QkmProbeNoOp")}, KEY_DOWN,
                    QStringLiteral("QkmDirect"), SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL);
                result->workAtSubmission = SendInputWorkItem::outstanding();
                result->sourceGateAfter = QKeyMapper_Worker::s_SourceReadAccess.runtimeObserved();
                delete task;
                SendInputTask::clearSendInputTaskControllerMap();
                result->workAfterJoin = SendInputWorkItem::outstanding();
                finish(!result->sourceGateBefore && result->sourceGateAfter && result->workAtSubmission == 1
                    && result->workAfterJoin == 0 && QKeyMapper_Worker::s_SourceReadAccess.runtimeObserved());
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
                } else if (mode == QStringLiteral("clear_queue")) {
                    submit({QStringLiteral("QkmProbeQueuedNoOp")}, KEY_DOWN, original);
                    result->workAtSubmission = SendInputWorkItem::outstanding();
                    current.task_threadpool->clear();
                    result->workAfterQueueClear = SendInputWorkItem::outstanding();
                    result->passed = result->passed && result->workAtSubmission == 2
                        && result->workAfterQueueClear == 1;
                    SendInputTask::clearSendInputTaskControllerMap();
                    result->workAfterJoin = SendInputWorkItem::outstanding();
                    result->passed = result->passed && result->workAfterJoin == 0;
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
        QStringLiteral("exit_wait"), QStringLiteral("exit_pause"), QStringLiteral("borrowers"),
        QStringLiteral("queued_stop_action"), QStringLiteral("queued_admission"),
        QStringLiteral("discard_dispatch"), QStringLiteral("clear_queue"), QStringLiteral("direct_task_gate"),
        QStringLiteral("idle_input_tracking"), QStringLiteral("queued_button"), QStringLiteral("button_admission"),
        QStringLiteral("lifecycle_dispatch"), QStringLiteral("lifecycle_discard"),
        QStringLiteral("owner_fence"), QStringLiteral("owner_fence_cancel"), QStringLiteral("session_lock_queue"),
        QStringLiteral("burst_timer_cleanup"), QStringLiteral("burst_timer_missing_source"),
        QStringLiteral("burst_timer_removed_source"), QStringLiteral("drain_wait"), QStringLiteral("drain_timeout"),
        QStringLiteral("drain_supersede"), QStringLiteral("drain_shutdown"),
        QStringLiteral("gui_stop_wait"), QStringLiteral("gui_stop_timeout"),
        QStringLiteral("gui_stop_exit"), QStringLiteral("closed_input_tracking")};
    bool knownCase = cases.contains(mode);
    for (const auto &spec : repeatSpecs()) { knownCase = knownCase || mode == spec.name; }
    if (!knownCase || path.isEmpty()) { return 2; }
    wchar_t desktop[256]{};
    const bool desktopObserved = GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()),
        UOI_NAME, desktop, sizeof(desktop), nullptr);
    result = new LifecycleResult;
    const int code = qkmProductMain(argc, argv);
    const bool passed = code == 0 && result->passed && SendInputTask::s_SendInputTaskControllerMap.isEmpty()
        && InputProducerWorkItem::outstanding() == 0
        && result->downStarted.loadAcquire() == result->downFinished.loadAcquire();
    QJsonObject evidence{{QStringLiteral("case"), mode}, {QStringLiteral("passed"), passed},
        {QStringLiteral("activeAtCleanup"), result->activeAtCleanup}, {QStringLiteral("pauseAtCleanup"), result->pauseAtCleanup},
        {QStringLiteral("cleanupMs"), result->cleanupMs}, {QStringLiteral("downStarted"), result->downStarted.loadAcquire()},
        {QStringLiteral("downFinished"), result->downFinished.loadAcquire()}, {QStringLiteral("upStarted"), result->upStarted.loadAcquire()},
        {QStringLiteral("upFinished"), result->upFinished.loadAcquire()}, {QStringLiteral("cancelledDown"), result->cancelledDown.loadAcquire()},
        {QStringLiteral("borrowerCalls"), result->borrowerCalls.loadAcquire()},
        {QStringLiteral("desktop"), desktopObserved ? QString::fromWCharArray(desktop) : QString()},
        {QStringLiteral("stoppedBeforeSubmission"), result->stoppedBeforeSubmission},
        {QStringLiteral("controllersEmptyBeforeDispatch"), result->controllersEmptyBeforeDispatch},
        {QStringLiteral("pendingAfterDispatch"), result->pendingAfterDispatch},
        {QStringLiteral("stopDownQueued"), result->stopDownQueued},
        {QStringLiteral("stopUpQueued"), result->stopUpQueued},
        {QStringLiteral("workAtSubmission"), QString::number(result->workAtSubmission)},
        {QStringLiteral("workAtBarrier"), QString::number(result->workAtBarrier)},
        {QStringLiteral("workAtSecondBarrier"), QString::number(result->workAtSecondBarrier)},
        {QStringLiteral("workAfterQueueClear"), QString::number(result->workAfterQueueClear)},
        {QStringLiteral("workAfterJoin"), QString::number(result->workAfterJoin)},
        {QStringLiteral("workAtExit"), QString::number(SendInputWorkItem::outstanding())},
        {QStringLiteral("sourceGateBefore"), result->sourceGateBefore},
        {QStringLiteral("sourceGateAfter"), result->sourceGateAfter},
        {QStringLiteral("idleStatesChecked"), result->idleStatesChecked},
        {QStringLiteral("callbackDownsTracked"), result->callbackDownsTracked},
        {QStringLiteral("callbackUpsTracked"), result->callbackUpsTracked},
        {QStringLiteral("inputProducerTrace"), QJsonArray::fromStringList(result->inputProducerTrace)},
        {QStringLiteral("producerSubmissions"), result->producerSubmissions},
        {QStringLiteral("producersWhileTaskWaiting"), QString::number(result->producersWhileTaskWaiting)},
        {QStringLiteral("producersAtExit"), QString::number(InputProducerWorkItem::outstanding())},
        {QStringLiteral("fenceFirstGeneration"), QString::number(result->fenceFirstGeneration)},
        {QStringLiteral("fenceLastGeneration"), QString::number(result->fenceLastGeneration)},
        {QStringLiteral("fenceCancelledGeneration"), QString::number(result->fenceCancelledGeneration)},
        {QStringLiteral("fenceReachedCount"), result->fenceReachedCount},
        {QStringLiteral("fenceGuiTurns"), result->fenceGuiTurns},
        {QStringLiteral("fenceOwnersAtRelease"), result->fenceOwnersAtRelease},
        {QStringLiteral("fencePendingAtRelease"), result->fencePendingAtRelease},
        {QStringLiteral("fenceCancelledHookIgnored"), result->fenceCancelledHookIgnored},
        {QStringLiteral("fenceInterceptionContextAvailable"), result->fenceInterceptionContextAvailable},
        {QStringLiteral("ownerFenceTrace"), QJsonArray::fromStringList(result->ownerFenceTrace)},
        {QStringLiteral("burstCycleTimerDestroyed"), result->burstCycleTimerDestroyed},
        {QStringLiteral("burstPressTimerDestroyed"), result->burstPressTimerDestroyed},
        {QStringLiteral("burstSourceContainsKeyAtCleanup"), result->burstSourceContainsKeyAtCleanup},
        {QStringLiteral("drainGuiTurns"), result->drainGuiTurns.loadAcquire()},
        {QStringLiteral("drainTrace"), QJsonArray::fromStringList(result->drainTrace)},
        {QStringLiteral("guiStopDowns"), result->guiStopDowns.loadAcquire()},
        {QStringLiteral("guiStopUps"), result->guiStopUps.loadAcquire()},
        {QStringLiteral("guiStopUnhooks"), result->guiStopUnhooks},
        {QStringLiteral("guiStopTimeouts"), result->guiStopTimeouts},
        {QStringLiteral("guiStopCompletions"), result->guiStopCompletions},
        {QStringLiteral("guiStopTurns"), result->guiStopTurns},
        {QStringLiteral("guiStopBlockedBeforeUnhook"), result->guiStopBlockedBeforeUnhook},
        {QStringLiteral("guiStopBlockedAtTimeout"), result->guiStopBlockedAtTimeout},
        {QStringLiteral("guiStopAdmissionRejected"), result->guiStopAdmissionRejected},
        {QStringLiteral("guiStopTrace"), QJsonArray::fromStringList(result->guiStopTrace)},
        {QStringLiteral("drainResourcesIntact"), result->drainResourcesIntact},
        {QStringLiteral("drainFinalPoolsFinished"), result->drainFinalPoolsFinished},
        {QStringLiteral("buttonLockAfterDispatch"), result->buttonLockAfterDispatch},
        {QStringLiteral("lifecycleDispatchTrace"), QJsonArray::fromStringList(result->lifecycleDispatchTrace)},
        {QStringLiteral("repeatRounds"), result->repeatRounds.loadAcquire()},
        {QStringLiteral("repeatRuns"), result->repeatRuns},
        {QStringLiteral("repeatTrace"), QJsonArray::fromStringList(result->repeatTrace)}};
    delete result;
    result = nullptr;
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(QJsonDocument(evidence).toJson()) < 0) { return 2; }
    return passed ? 0 : 2;
}
