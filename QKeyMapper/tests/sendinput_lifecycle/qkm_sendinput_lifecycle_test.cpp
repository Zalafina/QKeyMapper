#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QSemaphore>
#include <QTimer>
#include <QToolButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QSettings>
#include <QScopeGuard>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTabBar>
#include <QScopedValueRollback>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QDir>
#include <QAction>
#include <QMenu>
#include <QTranslator>
#include "../../automation/qkm_control_dialog.h"
#include "../../qgeneraladvanceddialog.h"
#include "../../qfloatingbuttonsetupdialog.h"
#include "../../qmappingadvanceddialog.h"
#include "../../qnotificationsetupdialog.h"
#include "../../qitemsetupdialog.h"
#include "../../qkeymapper_worker.h"
#include "../../qkeymapper.h"
#include "../../qkeymapper_qt_compat.h"
#include "../../qkm_input_producer_work.h"
#include "../../qkm_input_owner_fence.h"
#include "../../interception_worker.h"

// Test fixtures observe coordinator state without adding a product control entry.
class QkmMappingStopTestAccess {
public:
    static void armReturn(QKeyMapper *mapper)
    {
        mapper->recordLastAutoMatchedSetting(QStringLiteral("McpReturn"));
        mapper->m_LastAutoMatchedSettingTimer.setInterval(10);
        mapper->startLastAutoMatchedSettingTimer();
    }
    static bool returnCaptured(QKeyMapper *mapper)
    {
        return mapper->m_mappingStopRestoreSetting == QStringLiteral("McpReturn")
            && !mapper->m_LastAutoMatchedSettingTimer.isActive();
    }
    static quint64 loadGeneration(QKeyMapper *mapper) { return mapper->m_MappingLoadGeneration; }
    static QString loadedGroup(QKeyMapper *mapper) { return mapper->m_LoadedMappingSettingGroup; }
    static bool loadAccepted(QKeyMapper *mapper) { return mapper->m_MappingSourceLoadAccepted; }
    static quint64 fenceGeneration(QKeyMapper *mapper) { return mapper->m_mappingStopFenceGeneration; }
    static void deliverOldTail(QKeyMapper *mapper, quint64 operation, quint64 generation)
    {
        mapper->finishMappingStop(operation, generation);
    }
};

class QkmMappingAdditionTestAccess {
public:
    static QJsonObject run(QKeyMapper *mapper, const QString &mode);
    static QJsonObject equality(QKeyMapper *mapper);
    static QJsonObject serialization(QKeyMapper *mapper, bool cold);
    static QJsonObject generalSave(QKeyMapper *mapper, bool rich = false);
    static QJsonObject profileSave(QKeyMapper *mapper, bool cold);
    static QJsonObject windowMatchSave(QKeyMapper *mapper, bool cold);
    static QJsonObject commonSave(QKeyMapper *mapper, bool cold);
    static QJsonObject notificationSave(QKeyMapper *mapper, bool cold);
    static QJsonObject tableState(QKeyMapper *mapper);
    static QJsonObject commonComments(QKeyMapper *mapper, const QString &mode);
    static QJsonObject uiLocalization(QKeyMapper *mapper);
    static void uiLocalizationWindows(QKeyMapper *mapper, QJsonObject &evidence)
    {
        evidence.insert(QStringLiteral("advanced_hwnd"), QString::number(quintptr(mapper->m_GeneralAdvancedDialog->winId())));
        evidence.insert(QStringLiteral("control_hwnd"), QString::number(quintptr(mapper->m_McpControlDialog->winId())));
        evidence.insert(QStringLiteral("advanced_visible"), mapper->m_GeneralAdvancedDialog->isVisible());
        evidence.insert(QStringLiteral("control_visible"), mapper->m_McpControlDialog->isVisible());
    }
    static void uiLocalizationAdvanced(QKeyMapper *mapper)
    {
        mapper->m_McpControlDialog->close();
        mapper->m_GeneralAdvancedDialog->show();
        mapper->m_GeneralAdvancedDialog->raise();
        mapper->m_GeneralAdvancedDialog->activateWindow();
    }
};

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
    QStringList sourceRevisionTrace;
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
    quint64 guiStopLastOperation = 0;
    quint64 guiStopLastFence = 0;
    bool guiStopStaleIgnored = false;
    bool guiStopReturnCaptured = false;
    bool guiStopReturnLoaded = false;
    int guiStopBusySourceChecks = 0;
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
    QStringList sourceContextTrace;
    QStringList virtualReleaseTrace;
    QJsonObject mappingAdditionEvidence;
    QJsonObject mappingEqualityEvidence;
    QJsonObject settingsSerializationEvidence;
    QJsonObject settingsGeneralEvidence;
    QJsonObject commonCommentsEvidence;
    QJsonObject uiLocalizationEvidence;
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

void clickMappingButton(QAbstractButton *button)
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

bool prepareGuiStopReturnFixture()
{
    QSettings ini(QString::fromLatin1(CONFIG_FILENAME), QSettings::IniFormat);
    ini.setValue(QString::fromLatin1(SETTINGSELECT), QString::fromLatin1(GROUPNAME_GLOBALSETTING));
    for (const QString &group : {QString::fromLatin1(GROUPNAME_GLOBALSETTING), QStringLiteral("McpReturn")}) {
        ini.beginGroup(group);
        ini.setValue(QString::fromLatin1(KEYMAPDATA_ORIGINALKEYS), QString());
        ini.setValue(QString::fromLatin1(KEYMAPDATA_MAPPINGKEYS), QString());
        ini.setValue(QStringLiteral("MappingTable_TabNameList"),
            QStringList{group == QLatin1String(GROUPNAME_GLOBALSETTING) ? QStringLiteral("GlobalTab") : QStringLiteral("ReturnTab")});
        ini.setValue(QString::fromLatin1(AUTOSTARTMAPPING_CHECKED),
            group == QLatin1String(GROUPNAME_GLOBALSETTING) ? int(Qt::Checked) : int(Qt::Unchecked));
        ini.setValue(QStringLiteral("EnableSystemFilterKeyChecked"), false);
        ini.setValue(QStringLiteral("DisableFilterKeyClickSoundChecked"), false);
        ini.endGroup();
    }
    ini.sync();
    return ini.status() == QSettings::NoError;
}

void installGuiStopTest(QKeyMapper_Worker *worker, const QString &mode, int cycle = 1)
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), [worker, mode, cycle] {
        auto *mapper = QKeyMapper::getInstance();
        auto *button = mapper->findChild<QToolButton *>(QStringLiteral("keymapButton"));
        auto *editor = mapper->findChild<QLineEdit *>(QStringLiteral("settingNameLineEdit"));
        const bool restore = mode == QStringLiteral("gui_stop_restore") || mode == QStringLiteral("gui_stop_restore_timeout");
        const bool expectTimeout = mode == QStringLiteral("gui_stop_timeout")
            || ((mode == QStringLiteral("gui_stop_after_timeout") || mode == QStringLiteral("gui_stop_restore_timeout")) && cycle == 1);
        const quint64 loadGeneration = QkmMappingStopTestAccess::loadGeneration(mapper);
        const int tab = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
        if (!button || !editor || !button->isEnabled() || mapper->m_KeyMapStatus != QKeyMapper::KEYMAP_IDLE
            || tab < 0 || tab >= QKeyMapper::s_KeyMappingTabInfoList.size()
            || QKeyMapper::isCommonMappingTabIndex(tab)
            || (restore && (!QKeyMapper::checkGlobalSettingAutoStart()
                || QkmMappingStopTestAccess::loadedGroup(mapper) != QLatin1String(GROUPNAME_GLOBALSETTING)))) { finish(false); return; }
        if (auto *timer = mapper->findChild<QTimer *>(QStringLiteral("mappingStopDeadline"))) { timer->setInterval(30000); }
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
        const QPointer<QTableWidget> sourceTable = table;
        auto *context = new QObject(mapper);
        const auto hold = QSharedPointer<QSemaphore>::create();
        const auto entered = QSharedPointer<QSemaphore>::create();
        const auto stage = QSharedPointer<int>::create(0);
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
            [mapper, button, editor, hold, cycle, restore, loadGeneration](quint64 operation) {
                ++result->guiStopTimeouts;
                result->guiStopTrace.append(QStringLiteral("%1:timeout").arg(operation));
                result->guiStopBlockedAtTimeout = mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_STOPPING
                    && QKeyMapper_Worker::s_MappingInputClosed.loadAcquire()
                    && !button->isEnabled() && !editor->isEnabled() && result->guiStopUnhooks == cycle - 1
                    && result->guiStopCompletions == cycle - 1
                    && (!restore || (QkmMappingStopTestAccess::returnCaptured(mapper)
                        && QkmMappingStopTestAccess::loadGeneration(mapper) == loadGeneration));
                result->passed = result->passed && result->guiStopBlockedAtTimeout;
                QTimer::singleShot(20, mapper, [hold] { hold->release(); });
            });
        QObject::connect(mapper, &QKeyMapper::mappingCleanupCompleted_Signal, context,
            [worker, mapper, button, editor, context, source, backup, mode, cycle, restore, expectTimeout, loadGeneration]
            (quint64 operation, bool timedOut) {
                ++result->guiStopCompletions;
                result->guiStopTrace.append(QStringLiteral("%1:completed:%2").arg(operation).arg(timedOut));
                result->passed = result->passed && mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE
                    && !QKeyMapper_Worker::s_MappingInputClosed.loadAcquire() && button->isEnabled() && editor->isEnabled()
                    && result->guiStopUnhooks == cycle && result->guiStopDowns.loadAcquire() == cycle
                    && result->guiStopUps.loadAcquire() == cycle && result->guiStopCompletions == cycle
                    && result->guiStopBlockedBeforeUnhook && result->guiStopAdmissionRejected
                    && result->guiStopTimeouts == ((expectTimeout || mode == QStringLiteral("gui_stop_after_timeout")) ? 1 : 0)
                    && timedOut == expectTimeout && operation > result->guiStopLastOperation
                    && (cycle == 1 || result->guiStopStaleIgnored)
                    && SendInputWorkItem::outstanding() == 0 && InputProducerWorkItem::outstanding() == 0
                    && SendInputTask::poolsFinished() && result->guiStopTurns > 1;
                result->guiStopLastOperation = operation;
                result->guiStopLastFence = QkmMappingStopTestAccess::fenceGeneration(mapper);
                if (restore) {
                    const int restoredTab = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
                    result->guiStopReturnLoaded = QkmMappingStopTestAccess::loadAccepted(mapper)
                        && QkmMappingStopTestAccess::loadedGroup(mapper) == QStringLiteral("McpReturn")
                        && QkmMappingStopTestAccess::loadGeneration(mapper) > loadGeneration
                        && editor->text() == QStringLiteral("McpReturn") && QKeyMapper::KeyMappingDataList->isEmpty()
                        && restoredTab >= 0 && restoredTab < QKeyMapper::s_KeyMappingTabInfoList.size()
                        && QKeyMapper::s_KeyMappingTabInfoList.at(restoredTab).TabName == QStringLiteral("ReturnTab");
                    result->passed = result->passed && result->guiStopReturnCaptured && result->guiStopReturnLoaded
                        && result->guiStopBusySourceChecks > 0;
                }
                else {
                    *source = *backup;
                    QKeyMapper::restoreKeyMappingDataListPointer();
                }
                context->deleteLater();
                if (result->passed && cycle == 1
                    && (mode == QStringLiteral("gui_stop_repeat") || mode == QStringLiteral("gui_stop_after_timeout"))) {
                    QTimer::singleShot(0, mapper, [worker, mode] { installGuiStopTest(worker, mode, 2); });
                }
                else { finish(result->passed); }
            });
        // Use widget mouse events for the normal GUI entry, without injecting system input.
        clickMappingButton(button);
        auto *poll = new QTimer(context);
        QObject::connect(poll, &QTimer::timeout, context,
            [worker, mapper, button, editor, context, hold, entered, stage, source, sourceTable, backup, mode, cycle, restore, expectTimeout, loadGeneration, deadline] {
                ++result->guiStopTurns;
                if (deadline->elapsed() > 5000) { hold->release(); finish(false); return; }
                if (restore && *stage == 2 && SendInputWorkItem::outstanding() > 0) {
                    ++result->guiStopBusySourceChecks;
                    result->passed = result->passed && QkmMappingStopTestAccess::loadGeneration(mapper) == loadGeneration
                        && QkmMappingStopTestAccess::loadedGroup(mapper) == QLatin1String(GROUPNAME_GLOBALSETTING)
                        && sourceTable && sourceTable->rowCount() > 0;
                }
                if (*stage == 0) {
                    if (QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() != HOOKPROC_STATE_STARTED) { return; }
                    *stage = 1;
                    result->guiStopTrace.append(QStringLiteral("started:%1").arg(cycle));
                    QMetaObject::invokeMethod(QKeyMapper_Hook_Proc::getInstance(), [hold, entered] {
                        const InputProducerWorkItem inputWork;
                        entered->release();
                        hold->acquire();
                    }, Qt::QueuedConnection);
                    return;
                }
                if (*stage != 1 || entered->available() == 0) { return; }
                *stage = 2;
                result->guiStopTrace.append(QStringLiteral("stopping:%1").arg(cycle));
                if (restore) { QkmMappingStopTestAccess::armReturn(mapper); }
                clickMappingButton(button);
                result->guiStopBlockedBeforeUnhook = mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_STOPPING
                    && QKeyMapper_Worker::s_MappingInputClosed.loadAcquire()
                    && !button->isEnabled() && !editor->isEnabled() && result->guiStopUnhooks == cycle - 1;
                if (restore) {
                    result->guiStopReturnCaptured = QkmMappingStopTestAccess::returnCaptured(mapper)
                        && QkmMappingStopTestAccess::loadGeneration(mapper) == loadGeneration;
                }
                // A queued repeat request must neither restart mapping nor duplicate stop actions.
                mapper->MappingSwitch(QKeyMapper::MAPPINGSTART_BUTTONCLICK);
                result->guiStopAdmissionRejected = !worker->queueVButtonKey(QStringLiteral("VButton1"), true);
                if (cycle > 1) {
                    QkmMappingStopTestAccess::deliverOldTail(mapper, result->guiStopLastOperation, result->guiStopLastFence);
                    emit worker->sendInputDrainObserved_Signal(result->guiStopLastFence, QStringLiteral("drained"));
                    result->guiStopStaleIgnored = mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_STOPPING
                        && QKeyMapper_Worker::s_MappingInputClosed.loadAcquire() && !button->isEnabled()
                        && result->guiStopCompletions == cycle - 1 && result->guiStopUnhooks == cycle - 1;
                }
                if (mode == QStringLiteral("gui_stop_exit")) {
                    result->passed = result->passed && result->guiStopBlockedBeforeUnhook
                        && result->guiStopAdmissionRejected && result->guiStopCompletions == 0;
                    *source = *backup;
                    // The pending operation has not submitted Worker stop or borrowed the synthetic row.
                    hold->release();
                    finish(result->passed);
                }
                else if (expectTimeout) {
                    auto *timer = mapper->findChild<QTimer *>(QStringLiteral("mappingStopDeadline"));
                    if (!timer) { hold->release(); finish(false); return; }
                    timer->start(30);
                }
                else { QTimer::singleShot(50, context, [hold] { hold->release(); }); }
            });
        poll->start(5);
    }, Qt::QueuedConnection);
}

void installRuntimeRevisionTest(QKeyMapper_Worker *worker)
{
    auto &access = QKeyMapper_Worker::s_SourceReadAccess;
    const auto record = [&access] { result->sourceRevisionTrace.append(QString::number(access.runtimeRevision())); };
    result->passed = !access.runtimeObserved() && access.runtimeRevision() == 0;
    record();
    const QString original = QStringLiteral("QkmRuntimeRevision");
    const QStringList keys{QStringLiteral("QkmProbeNoOp")};
    for (int i = 0; i < 2; ++i) {
        auto *task = new SendInputTask(-1, keys, KEY_DOWN, original, SENDMODE_NORMAL, SENDVIRTUALKEY_STATE_NORMAL);
        record();
        delete task;
    }
    worker->sendInputKeys(-1, keys, KEY_DOWN, original, SENDMODE_NORMAL, SendInputTask::s_GlobalSendInputTaskController);
    record();
    worker->sendInputKeys(-1, keys, KEY_UP, original, SENDMODE_NORMAL, SendInputTask::s_GlobalSendInputTaskController);
    record();
    QString queuedOriginal = QStringLiteral("QkmQueuedRevision");
    QStringList queuedKeys = keys;
    worker->emit_sendInputKeysSignal_Wrapper(-1, queuedKeys, KEY_DOWN, queuedOriginal, SENDMODE_NORMAL);
    record();
    worker->emit_sendInputKeysSignal_Wrapper(-1, queuedKeys, KEY_UP, queuedOriginal, SENDMODE_NORMAL);
    record();
    result->workAtSubmission = SendInputWorkItem::outstanding();
    QMetaObject::invokeMethod(worker, [] {
        for (const auto &controller : std::as_const(SendInputTask::s_SendInputTaskControllerMap)) {
            controller.task_threadpool->waitForDone();
        }
        result->sourceRevisionTrace.append(QString::number(QKeyMapper_Worker::s_SourceReadAccess.runtimeRevision()));
        for (int i = 1; i < result->sourceRevisionTrace.size(); ++i) {
            result->passed = result->passed && result->sourceRevisionTrace.at(i).toULongLong()
                > result->sourceRevisionTrace.at(i - 1).toULongLong();
        }
        SendInputTask::clearSendInputTaskControllerMap();
        finish(result->passed && result->sourceRevisionTrace.size() == 8 && result->workAtSubmission == 2
            && SendInputWorkItem::outstanding() == 0 && InputProducerWorkItem::outstanding() == 0
            && QKeyMapper_Worker::pressedMappingKeysMap.isEmpty());
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
        {QStringLiteral("repeat_source_context"), macro, body, body, body},
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
    auto *decoy = mode == QStringLiteral("repeat_source_context") ? new QList<MAP_KEYDATA>(*data) : nullptr;
    if (decoy) { decoy->first().RepeatMode = REPEAT_MODE_NONE; }
    QKeyMapper::KeyMappingDataList = decoy ? decoy : data;
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
    const auto dispatcher = QObject::connect(worker, &QKeyMapper_Worker::sendInputKeys_Signal, worker,
        [data, decoy](int index, const QStringList &keys, int direction, const QString &original, int sendmode, int state, QList<MAP_KEYDATA> *mappingData) {
            if (decoy) {
                const bool matches = mappingData == data;
                result->sourceContextTrace.append(QStringLiteral("%1:%2:%3").arg(original).arg(direction).arg(matches));
                result->passed = result->passed && matches;
            }
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
        [worker, timer, data, decoy, dispatcher, previousData, previousHookState, spec, trigger, attempts = 0, quiet = 0, phase = 0, extraRounds = 0]() mutable {
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
            QObject::disconnect(dispatcher);
            QObject::connect(worker, &QKeyMapper_Worker::sendInputKeys_Signal, worker,
                &QKeyMapper_Worker::onSendInputKeys, Qt::QueuedConnection);
            QKeyMapper::KeyMappingDataList = previousData;
            QKeyMapper_Worker::s_AtomicHookProcState.storeRelease(previousHookState);
            delete data;
            delete decoy;
            timer->deleteLater();
            finish(result->passed);
        });
    timer->start(20);
}

void installSourceContextTest(QKeyMapper_Worker *worker, bool holdSequence)
{
    auto *previous = QKeyMapper::KeyMappingDataList;
    QList<MAP_KEYDATA> source{MAP_KEYDATA(), MAP_KEYDATA()};
    source[0].Original_Key = QStringLiteral("QkmContextSource");
    source[0].Mapping_Keys = QStringList{QStringLiteral("QkmProbeNoOp")};
    source[0].Lock = !holdSequence;
    source[0].LockState = LOCK_STATE_LOCKON;
    source[1].Original_Key = QStringLiteral("QkmContextUnlock");
    source[1].Mapping_Keys = QStringList{QStringLiteral("Unlock(QkmContextSource)")};
    QList<MAP_KEYDATA> decoy = source;
    decoy[0].Original_Key = QStringLiteral("QkmContextDecoy");
    decoy[0].Mapping_Keys = QStringList{QStringLiteral("QkmDecoyNoOp")};
    QKeyMapper::KeyMappingDataList = &decoy;
    int submissions = 0;
    if (!QObject::disconnect(worker, &QKeyMapper_Worker::sendInputKeys_Signal,
        worker, &QKeyMapper_Worker::onSendInputKeys)) {
        QKeyMapper::KeyMappingDataList = previous;
        finish(false);
        return;
    }
    const auto observer = QObject::connect(worker, &QKeyMapper_Worker::sendInputKeys_Signal, worker,
        [&source, &submissions](int index, const QStringList &keys, int direction, const QString &original,
            int, int, QList<MAP_KEYDATA> *mappingData) {
            ++submissions;
            const bool matches = mappingData == &source && index == 0
                && original.startsWith(source.first().Original_Key)
                && !keys.isEmpty() && keys.first().startsWith(QStringLiteral("QkmProbeNoOp"));
            result->sourceContextTrace.append(QStringLiteral("%1:%2:%3").arg(original).arg(direction).arg(matches));
            result->passed = result->passed && matches && InputProducerWorkItem::outstanding() > 0;
        }, Qt::DirectConnection);
    if (holdSequence) {
        QStringList keys{QStringLiteral("QkmProbeNoOpA"), QStringLiteral("QkmProbeNoOpB")};
        worker->sendInputKeys(0, keys, KEY_DOWN, source.first().Original_Key, SENDMODE_KEYSEQ_HOLDDOWN,
            SendInputTask::s_GlobalSendInputTaskController, &source);
        const QString finalOriginal = source.first().Original_Key + QLatin1Char(':') + KEYSEQUENCE_STR + HOLDDOWN_STR;
        QKeyMapper_Worker::pressedMappingKeysMap.insert(finalOriginal, keys);
        worker->sendInputKeys(0, keys, KEY_UP, source.first().Original_Key, SENDMODE_KEYSEQ_HOLDDOWN,
            SendInputTask::s_GlobalSendInputTaskController, &source);
        QKeyMapper_Worker::pressedMappingKeysMap.remove(finalOriginal);
    } else {
        QKeyMapper_Worker::pressedLockKeysMap.insert(source.first().Original_Key, 0);
        worker->sendInputKeys(1, source.at(1).Mapping_Keys, KEY_DOWN, source.at(1).Original_Key, SENDMODE_NORMAL,
            SendInputTask::s_GlobalSendInputTaskController, &source);
        result->passed = result->passed && source.first().LockState == LOCK_STATE_LOCKOFF
            && !QKeyMapper_Worker::pressedLockKeysMap.contains(source.first().Original_Key)
            && decoy.first().LockState == LOCK_STATE_LOCKON;
    }
    QObject::disconnect(observer);
    QObject::connect(worker, &QKeyMapper_Worker::sendInputKeys_Signal, worker,
        &QKeyMapper_Worker::onSendInputKeys, Qt::QueuedConnection);
    QKeyMapper::KeyMappingDataList = previous;
    finish(result->passed && submissions == (holdSequence ? 5 : 1)
        && SendInputWorkItem::outstanding() == 0 && InputProducerWorkItem::outstanding() == 0);
}

void installInactiveVirtualReleaseTest()
{
    auto *mapper = QKeyMapper::getInstance();
    auto *previousSource = QKeyMapper::KeyMappingDataList;
    const auto previousStatus = mapper->m_KeyMapStatus;
    const int previousHook = QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire();
    const bool previousClosed = QKeyMapper_Worker::s_MappingInputClosed.loadAcquire();
    const auto previousReal = QKeyMapper_Worker::pressedRealKeysList;
    const auto previousPure = QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput;
    const auto previousVirtual = QKeyMapper_Worker::pressedVirtualKeysList;
    const auto previousBlocked = QKeyMapper_Worker::blockedKeysList;
    QList<MAP_KEYDATA> source{MAP_KEYDATA(), MAP_KEYDATA()}, empty;
    const auto restore = qScopeGuard([=] {
        QKeyMapper::KeyMappingDataList = previousSource;
        mapper->m_KeyMapStatus = previousStatus;
        QKeyMapper_Worker::s_AtomicHookProcState.storeRelease(previousHook);
        QKeyMapper_Worker::s_MappingInputClosed.storeRelease(previousClosed);
        QKeyMapper_Worker::pressedRealKeysList = previousReal;
        QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput = previousPure;
        QKeyMapper_Worker::pressedVirtualKeysList = previousVirtual;
        QKeyMapper_Worker::blockedKeysList = previousBlocked;
    });
    source[0].Original_Key = QStringLiteral("A");
    source[1].Original_Key = QStringLiteral("Mouse-L");
    QKeyMapper_Worker::pressedRealKeysList = QStringList{QStringLiteral("A"), QStringLiteral("Mouse-L")};
    QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput = QKeyMapper_Worker::pressedRealKeysList;
    QKeyMapper_Worker::blockedKeysList.clear();
    // Replay both callbacks without injecting system input, including the late cleanup tail.
    for (int phase : {0, 1, 2}) {
        const bool stopped = phase == 0;
        mapper->m_KeyMapStatus = stopped ? QKeyMapper::KEYMAP_IDLE : QKeyMapper::KEYMAP_STOPPING;
        QKeyMapper_Worker::s_AtomicHookProcState.storeRelease(phase == 1 ? HOOKPROC_STATE_STARTED : HOOKPROC_STATE_STOPPED);
        QKeyMapper_Worker::s_MappingInputClosed.storeRelease(!stopped);
        for (bool present : {true, false}) {
            QKeyMapper::KeyMappingDataList = present ? &source : &empty;
            for (int state : {SENDVIRTUALKEY_STATE_FORCE, SENDVIRTUALKEY_STATE_BURST_STOP,
                SENDVIRTUALKEY_STATE_FORCE_KEYUP_BYPASS_REALKEY}) {
                QKeyMapper_Worker::pressedVirtualKeysList = QKeyMapper_Worker::pressedRealKeysList;
                KBDLLHOOKSTRUCT key{};
                key.vkCode = 0x41;
                key.scanCode = 0x1e;
                key.flags = LLKHF_INJECTED | LLKHF_UP;
                key.dwExtraInfo = QKeyMapper_Worker::VIRTUAL_KEY_SEND | ULONG_PTR(state);
                MSLLHOOKSTRUCT mouse{};
                mouse.flags = LLMHF_INJECTED;
                mouse.dwExtraInfo = key.dwExtraInfo;
                const LRESULT keyboardResult = QKeyMapper_Worker::LowLevelKeyboardHookProc(HC_ACTION, WM_KEYUP, LPARAM(&key));
                const LRESULT mouseResult = QKeyMapper_Worker::LowLevelMouseHookProc(HC_ACTION, WM_LBUTTONUP, LPARAM(&mouse));
                const bool block = state != SENDVIRTUALKEY_STATE_FORCE_KEYUP_BYPASS_REALKEY && (stopped || !present);
                result->virtualReleaseTrace.append(QStringLiteral("%1:%2:%3:%4:%5")
                    .arg(phase).arg(present).arg(state).arg(keyboardResult).arg(mouseResult));
                result->passed = result->passed && (keyboardResult != 0) == block && (mouseResult != 0) == block
                    && QKeyMapper_Worker::pressedVirtualKeysList.isEmpty()
                    && InputProducerWorkItem::outstanding() == 0 && SendInputWorkItem::outstanding() == 0;
            }
        }
    }
    finish(result->passed && result->virtualReleaseTrace.size() == 18);
}

void installLifecycleTest()
{
    QTimer::singleShot(0, QCoreApplication::instance(), []() {
        const QString guiMode = qEnvironmentVariable("QKM_LIFECYCLE_CASE");
        if (guiMode == QStringLiteral("ui_localization")) {
            // Capture separately: the advanced dialog closes on deactivation when MCP opens.
            QTimer::singleShot(1000, QCoreApplication::instance(), [] {
                result->uiLocalizationEvidence = QkmMappingAdditionTestAccess::uiLocalization(QKeyMapper::getInstance());
                QTimer::singleShot(500, QCoreApplication::instance(), [] {
                    auto *mapper = QKeyMapper::getInstance();
                    QkmMappingAdditionTestAccess::uiLocalizationWindows(mapper, result->uiLocalizationEvidence);
                    QSaveFile ready(qEnvironmentVariable("QKM_UI_CAPTURE_STATE"));
                    const QByteArray payload = QJsonDocument(result->uiLocalizationEvidence).toJson();
                    const bool written = ready.open(QIODevice::WriteOnly) && ready.write(payload) == payload.size() && ready.commit();
                    if (!written) { finish(false); return; }
                    auto *stage = new QTimer(QCoreApplication::instance());
                    QObject::connect(stage, &QTimer::timeout, stage, [stage] {
                        const QString path = qEnvironmentVariable("QKM_UI_CAPTURE_STATE");
                        if (!QFileInfo::exists(path + QStringLiteral(".request"))) return;
                        stage->stop();
                        QkmMappingAdditionTestAccess::uiLocalizationAdvanced(QKeyMapper::getInstance());
                        QTimer::singleShot(500, QCoreApplication::instance(), [path] {
                            QkmMappingAdditionTestAccess::uiLocalizationWindows(QKeyMapper::getInstance(), result->uiLocalizationEvidence);
                            QSaveFile advanced(path + QStringLiteral(".advanced"));
                            const QByteArray data = QJsonDocument(result->uiLocalizationEvidence).toJson();
                            if (!advanced.open(QIODevice::WriteOnly) || advanced.write(data) != data.size() || !advanced.commit()) finish(false);
                        });
                    });
                    stage->start(50);
                    QTimer::singleShot(20000, QCoreApplication::instance(), [] {
                        finish(result->uiLocalizationEvidence.value(QStringLiteral("passed")).toBool());
                    });
                });
            });
            return;
        }
        if (guiMode.startsWith(QStringLiteral("common_comments"))) {
            result->commonCommentsEvidence = QkmMappingAdditionTestAccess::commonComments(QKeyMapper::getInstance(), guiMode);
            QTimer::singleShot(2000, QCoreApplication::instance(), [] {
                finish(result->commonCommentsEvidence.value(QStringLiteral("passed")).toBool());
            });
            return;
        }
        if (guiMode == QStringLiteral("settings_notification_save") || guiMode == QStringLiteral("settings_notification_cold")) {
            result->settingsGeneralEvidence = QkmMappingAdditionTestAccess::notificationSave(
                QKeyMapper::getInstance(), guiMode.endsWith(QStringLiteral("_cold")));
            QTimer::singleShot(2000, QCoreApplication::instance(), [] {
                finish(result->settingsGeneralEvidence.value(QStringLiteral("passed")).toBool());
            });
            return;
        }
        if (guiMode == QStringLiteral("settings_common_save") || guiMode == QStringLiteral("settings_common_cold")) {
            result->settingsGeneralEvidence = QkmMappingAdditionTestAccess::commonSave(
                QKeyMapper::getInstance(), guiMode.endsWith(QStringLiteral("_cold")));
            QTimer::singleShot(2000, QCoreApplication::instance(), [] {
                finish(result->settingsGeneralEvidence.value(QStringLiteral("passed")).toBool());
            });
            return;
        }
        if (guiMode == QStringLiteral("settings_match_save") || guiMode == QStringLiteral("settings_match_cold")) {
            result->settingsGeneralEvidence = QkmMappingAdditionTestAccess::windowMatchSave(
                QKeyMapper::getInstance(), guiMode.endsWith(QStringLiteral("_cold")));
            QTimer::singleShot(2000, QCoreApplication::instance(), [] {
                finish(result->settingsGeneralEvidence.value(QStringLiteral("passed")).toBool());
            });
            return;
        }
        if (guiMode == QStringLiteral("settings_profile_save") || guiMode == QStringLiteral("settings_profile_cold")) {
            result->settingsGeneralEvidence = QkmMappingAdditionTestAccess::profileSave(
                QKeyMapper::getInstance(), guiMode.endsWith(QStringLiteral("_cold")));
            QTimer::singleShot(2000, QCoreApplication::instance(), [] {
                finish(result->settingsGeneralEvidence.value(QStringLiteral("passed")).toBool());
            });
            return;
        }
        if (guiMode == QStringLiteral("settings_general_save") || guiMode == QStringLiteral("settings_table_save")
            || guiMode == QStringLiteral("settings_table_cold")) {
            result->settingsGeneralEvidence = guiMode == QStringLiteral("settings_table_cold")
                ? QkmMappingAdditionTestAccess::tableState(QKeyMapper::getInstance())
                : QkmMappingAdditionTestAccess::generalSave(QKeyMapper::getInstance(), guiMode == QStringLiteral("settings_table_save"));
            QTimer::singleShot(2000, QCoreApplication::instance(), [] {
                finish(result->settingsGeneralEvidence.value(QStringLiteral("passed")).toBool());
            });
            return;
        }
        if (guiMode.startsWith(QStringLiteral("settings_serialization"))) {
            result->settingsSerializationEvidence = QkmMappingAdditionTestAccess::serialization(
                QKeyMapper::getInstance(), guiMode.endsWith(QStringLiteral("_cold")));
            // Keep the process observable by the owned launcher before normal exit.
            QTimer::singleShot(2000, QCoreApplication::instance(), [] {
                finish(result->settingsSerializationEvidence.value(QStringLiteral("passed")).toBool());
            });
            return;
        }
        if (guiMode == QStringLiteral("mapping_config_equality")) {
            result->mappingEqualityEvidence = QkmMappingAdditionTestAccess::equality(QKeyMapper::getInstance());
            finish(result->mappingEqualityEvidence.value(QStringLiteral("passed")).toBool());
            return;
        }
        if (guiMode.startsWith(QStringLiteral("mapping_add_"))) {
            result->mappingAdditionEvidence = QkmMappingAdditionTestAccess::run(QKeyMapper::getInstance(), guiMode);
            finish(result->mappingAdditionEvidence.value(QStringLiteral("passed")).toBool());
            return;
        }
        auto *worker = QKeyMapper_Worker::getInstance();
        QMetaObject::invokeMethod(worker, [worker]() {
            const QString mode = qEnvironmentVariable("QKM_LIFECYCLE_CASE");
            if (mode == QStringLiteral("inactive_virtual_release")) {
                QMetaObject::invokeMethod(QCoreApplication::instance(), installInactiveVirtualReleaseTest, Qt::QueuedConnection);
                return;
            }
            if (mode == QStringLiteral("hold_source_context") || mode == QStringLiteral("unlock_source_context")) {
                installSourceContextTest(worker, mode == QStringLiteral("hold_source_context"));
                return;
            }
            if (mode == QStringLiteral("runtime_revision")) {
                installRuntimeRevisionTest(worker);
                return;
            }
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

QJsonObject QkmMappingAdditionTestAccess::run(QKeyMapper *mapper, const QString &mode)
{
    using namespace QKeyMapperConstants;
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    auto *tabs = mapper->findChild<QTabWidget *>(QStringLiteral("keyMappingTabWidget"));
    auto *original = mapper->findChild<QComboBox *>(QStringLiteral("orikeyComboBox"));
    auto *mapping = mapper->findChild<QComboBox *>(QStringLiteral("mapkeyComboBox"));
    auto *record = mapper->findChild<QLineEdit *>(QStringLiteral("originalKeyRecordLineEdit"));
    auto *add = mapper->findChild<QPushButton *>(QStringLiteral("addmapdataButton"));
    if (!tabs || !original || !mapping || !record || !add || !mapper->m_MappingAdvancedDialog) {
        return {{QStringLiteral("passed"), false}, {QStringLiteral("reason"), QStringLiteral("Missing GUI fixture controls")}};
    }
    const bool plain = mode == QStringLiteral("mapping_add_plain");
    const bool setup = mode == QStringLiteral("mapping_add_setup");
    mapper->m_MappingAdvancedDialog->setCommonMappingTableEnabled(!plain);
    mapper->updateCommonMappingTabVisibility();
    if (!plain && !mapper->addTabToKeyMappingTabWidget(QStringLiteral("Map67Other"), false)) {
        return {{QStringLiteral("passed"), false}, {QStringLiteral("reason"), QStringLiteral("Cannot create second fixture Tab")}};
    }
    const int normalIndex = QKeyMapper::firstNormalMappingTabIndex();
    const int commonIndex = QKeyMapper::findCommonMappingTabIndex();
    if (normalIndex < 0 || (!plain && commonIndex < 0)) {
        return {{QStringLiteral("passed"), false}, {QStringLiteral("reason"), QStringLiteral("Missing fixture source")}};
    }
    QList<QList<MAP_KEYDATA>> seed;
    for (int index = 0; index < QKeyMapper::s_KeyMappingTabInfoList.size(); ++index) {
        auto &tab = QKeyMapper::s_KeyMappingTabInfoList[index];
        tab.KeyMappingData->clear();
        if (!QKeyMapper::isCommonMappingTabIndex(index)) {
            tab.IncludeCommonMappingTable = !plain;
            tab.CommonMappingCollapsed = false;
            tab.KeyMappingData->append(mapper->createDefaultKeyMappingData(
                plain || setup ? QStringLiteral("C") : QStringLiteral("A"), QStringLiteral("D"), false));
        } else if (setup) {
            tab.KeyMappingData->append(mapper->createDefaultKeyMappingData(QStringLiteral("Z"), QStringLiteral("E"), false));
        }
        seed.append(*tab.KeyMappingData);
    }
    const int target = plain || setup ? normalIndex : commonIndex;
    const auto reset = [&] {
        mapper->closeItemSetupDialog();
        for (int index = 0; index < seed.size(); ++index) { *QKeyMapper::s_KeyMappingTabInfoList.at(index).KeyMappingData = seed.at(index); }
        mapper->forceSwitchKeyMappingTabWidgetIndex(target);
        mapper->refreshTabsForSourceTabChange(target);
        mapper->clearSaveSettingDirty();
    };
    reset();
    const auto snapshot = [&] {
        QJsonArray sources;
        for (const auto &tab : std::as_const(QKeyMapper::s_KeyMappingTabInfoList)) {
            QJsonArray rows;
            for (const auto &entry : *tab.KeyMappingData) {
                rows.append(QJsonObject{{QStringLiteral("code"), QKeyMapper::generateMappingCode(entry)},
                    {QStringLiteral("pure_original"), QJsonArray::fromStringList(entry.Pure_OriginalKeys)},
                    {QStringLiteral("pure_mapping"), QJsonArray::fromStringList(entry.Pure_MappingKeys)},
                    {QStringLiteral("pure_key_up"), QJsonArray::fromStringList(entry.Pure_MappingKeys_KeyUp)},
                    {QStringLiteral("lock"), int(entry.LockState)}});
            }
            sources.append(QJsonObject{{QStringLiteral("name"), tab.TabInternalName}, {QStringLiteral("rows"), rows}});
        }
        return QJsonObject{{QStringLiteral("sources"), sources}, {QStringLiteral("dirty"), mapper->m_SaveSettingDirty},
            {QStringLiteral("display_rows"), mapper->m_KeyMappingDataTable->rowCount()},
            {QStringLiteral("current_row"), mapper->m_KeyMappingDataTable->currentRow()}};
    };
    QFile ini(QString::fromLatin1(CONFIG_FILENAME));
    const auto disk = [&] { if (!ini.open(QIODevice::ReadOnly)) return QByteArray(); const auto bytes = ini.readAll(); ini.close(); return bytes; };
    const auto beforeDisk = disk();
    check(!beforeDisk.isEmpty(), "owned fixture INI is readable");
    check(mapper->mappingAdditionAllowed(target), "GUI lane is idle and cleanup complete");
    int notifications = 0;
    const auto observer = QObject::connect(mapper, &QKeyMapper::keyMappingTableItemCheckStateChanged_Signal,
        mapper, [&](int, int, bool) { ++notifications; });
    const auto disconnect = qScopeGuard([=] { QObject::disconnect(observer); });
    const auto prepare = [&] { return mapper->prepareMappingAddition(target, QKeyMapper::KeyMappingDataList->size(),
        QStringLiteral("A"), QStringLiteral("B")); };
    const auto before = snapshot();
    const auto plan = prepare();
    check(plan.ok() && snapshot() == before && notifications == 0 && disk() == beforeDisk,
        "pure analysis leaves all source fields, derived values, dirty, view, notifications and disk unchanged");
    // Combo values are fixture preconditions; adding is driven only by Qt mouse events.
    original->setCurrentText(QStringLiteral("A"));
    mapping->setCurrentText(QStringLiteral("B"));
    record->clear();
    if (plain) {
        clickMappingButton(add);
        check(QKeyMapper::KeyMappingDataList->size() == 2 && mapper->m_SaveSettingDirty, "GUI added one dirty row");
        if (QKeyMapper::KeyMappingDataList->size() == 2) {
            const auto row = QKeyMapper::KeyMappingDataList->last();
            check(row.Original_Key == QStringLiteral("A") && row.Mapping_Keys == QStringList{QStringLiteral("B")}
                && row.MappingKeys_KeyUp == row.Mapping_Keys && row.Pure_OriginalKeys == QStringList{QStringLiteral("A")}
                && row.Pure_MappingKeys == row.Mapping_Keys && row.Pure_MappingKeys_KeyUp == row.Mapping_Keys
                && !row.Disabled && !row.Burst && !row.Lock && row.RepeatMode == REPEAT_MODE_NONE
                && row.SendTiming == SENDTIMING_NORMAL, "independent ordinary-row and derived-state expectations");
        }
        const auto gui = snapshot();
        reset();
        check(mapper->commitMappingAddition(prepare(), true).isValid && snapshot() == gui,
            "shared commit matches independent GUI-event result including full serialized and derived fields");
        reset();
        const auto insertion = mapper->prepareMappingAddition(target, 0, QStringLiteral("A"), QStringLiteral("B"));
        check(insertion.ok() && mapper->commitMappingAddition(insertion, true).isValid
            && QKeyMapper::KeyMappingDataList->first().Original_Key == QStringLiteral("A")
            && QKeyMapper::KeyMappingDataList->last().Original_Key == QStringLiteral("C")
            && mapper->m_KeyMappingDataTable->currentRow() == 0, "explicit insertion ignores ambient selection");
        reset();
        check(!mapper->prepareMappingAddition(target, 0, QStringLiteral("QkmInvalidOriginal"), QStringLiteral("B")).ok()
            && !mapper->prepareMappingAddition(target, 0, QStringLiteral("A"), QStringLiteral("B»")).ok()
            && !mapper->prepareMappingAddition(target, -1, QStringLiteral("A"), QStringLiteral("B")).ok()
            && snapshot() == before, "invalid keys, empty sequence segment and insertion bounds do not change data");
        for (const auto &keys : {qMakePair(QStringLiteral("A+B"), QStringLiteral("C+D")),
            qMakePair(QStringLiteral("A⏲200"), QStringLiteral("B⏱20")),
            qMakePair(QStringLiteral("A✖300"), QStringLiteral("B»C")),
            qMakePair(QStringLiteral("SendOnMappingStop"), QStringLiteral("E"))}) {
            reset();
            const auto parsed = mapper->prepareMappingAddition(target, 1, keys.first, keys.second);
            check(parsed.ok() && mapper->commitMappingAddition(parsed, true).isValid
                && QKeyMapper::KeyMappingDataList->last().Original_Key == keys.first
                && QKeyMapper::KeyMappingDataList->last().Mapping_Keys.join(QStringLiteral("»")) == keys.second,
                "shared parser retains combinations, time suffixes, sequences and stop-action keys");
        }
    } else if (mode == QStringLiteral("mapping_add_conflicts")) {
        check(plan.disableRows.size() == 2 && !plan.confirmationMessage.isEmpty(), "two normal-source conflicts are explicit");
        const auto guiChoice = [&](QMessageBox::StandardButton choice) {
            int replies = 0;
            QTimer answer;
            QObject::connect(&answer, &QTimer::timeout, mapper, [&] {
                auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                if (!box || !box->button(choice)) return;
                answer.stop();
                ++replies;
                clickMappingButton(box->button(choice));
            });
            answer.start(10);
            clickMappingButton(add);
            answer.stop();
            return replies == 1;
        };
        check(guiChoice(QMessageBox::No) && snapshot() == before && notifications == 0, "GUI conflict cancellation is a zero-change operation");
        check(!mapper->commitMappingAddition(plan, false).isValid && snapshot() == before && notifications == 0,
            "shared commit refuses unconfirmed conflicts without partial disables");
        check(guiChoice(QMessageBox::Yes), "GUI conflict confirmation is handled through its modal button");
        const auto gui = snapshot();
        check(QKeyMapper::KeyMappingDataList->size() == 1 && !QKeyMapper::KeyMappingDataList->first().Disabled
            && notifications == 2 && mapper->m_SaveSettingDirty, "GUI common addition applies both conflict disables");
        for (const auto &row : plan.disableRows) {
            check(QKeyMapper::s_KeyMappingTabInfoList.at(row.first).KeyMappingData->at(row.second).Disabled,
                "conflicting normal row is disabled");
        }
        reset(); notifications = 0;
        check(mapper->commitMappingAddition(prepare(), true).isValid && snapshot() == gui && notifications == 2,
            "shared conflict commit matches full GUI result and notifications");
    } else if (mode == QStringLiteral("mapping_add_guards")) {
        for (auto state : {QKeyMapper::KEYMAP_CHECKING, QKeyMapper::KEYMAP_MAPPING_GLOBAL,
            QKeyMapper::KEYMAP_MAPPING_MATCHED, QKeyMapper::KEYMAP_STOPPING}) {
            const QScopedValueRollback<decltype(mapper->m_KeyMapStatus)> status(mapper->m_KeyMapStatus, state);
            check(!prepare().ok() && !mapper->commitMappingAddition(plan, true).isValid
                && snapshot() == before && notifications == 0, "non-idle state rejects analysis and commit without partial changes");
            clickMappingButton(add);
            check(snapshot() == before && notifications == 0, "GUI events cannot bypass the shared non-idle guard");
        }
        QKeyMapper_Worker::s_MappingInputClosed.storeRelease(true);
        check(!prepare().ok() && !mapper->commitMappingAddition(plan, true).isValid && snapshot() == before,
            "Worker stopped alone does not bypass cleanup admission");
        QKeyMapper_Worker::s_MappingInputClosed.storeRelease(false);
        {
            InputProducerWorkItem producer;
            check(!prepare().ok() && !mapper->commitMappingAddition(plan, true).isValid && snapshot() == before,
                "active producer prevents partial source modification");
        }
        {
            const QScopedValueRollback<bool> loading(mapper->m_MappingSourceLoading, true);
            check(!prepare().ok() && !mapper->commitMappingAddition(plan, true).isValid && snapshot() == before,
                "load in progress refuses edits");
        }
        check(mapper->mappingAdditionAllowed(target), "guard is restored after temporary blocking conditions");
    } else if (mode == QStringLiteral("mapping_add_stale")) {
        {
            const QScopedValueRollback<quint64> generation(mapper->m_MappingLoadGeneration, mapper->m_MappingLoadGeneration + 1);
            check(!mapper->commitMappingAddition(plan, true).isValid && snapshot() == before, "changed load generation rejects the old decision");
        }
        {
            const QScopedValueRollback<quint64> epoch(mapper->m_AutomationSourceEpoch, mapper->m_AutomationSourceEpoch + 1);
            check(!mapper->commitMappingAddition(plan, true).isValid && snapshot() == before, "changed source structure rejects the old decision");
        }
        // Keep the epoch unchanged so recomputed conflict decisions must reject this edit themselves.
        QKeyMapper::s_KeyMappingTabInfoList[normalIndex].KeyMappingData->first().Original_Key = QStringLiteral("F1");
        const auto changed = snapshot();
        check(!mapper->commitMappingAddition(plan, true).isValid && snapshot() == changed && notifications == 0,
            "changed conflicts are refused before disabling any remaining row");
        reset();
        const auto tabPlan = prepare();
        mapper->forceSwitchKeyMappingTabWidgetIndex(normalIndex);
        const auto otherTab = snapshot();
        check(tabPlan.ok() && !mapper->commitMappingAddition(tabPlan, true).isValid && snapshot() == otherTab,
            "changing the current GUI target rejects the old decision without modifying either source");
        reset();
        const auto runtimePlan = prepare();
        QKeyMapper_Worker::s_SourceReadAccess.observeRuntimeLifecycle();
        check(runtimePlan.ok() && !mapper->commitMappingAddition(runtimePlan, true).isValid && snapshot() == before,
            "runtime activity invalidates the old decision even when GUI is idle again");
    } else if (setup) {
        mapper->addFixedDefaultMappingAndOpenSetup();
        check(QKeyMapper::KeyMappingDataList->size() == 2 && mapper->m_ItemSetupDialog->isVisible()
            && mapper->m_ItemSetupDialog->getTabIndex() == normalIndex && mapper->m_ItemSetupDialog->getItemRow() == 1
            && QKeyMapper::KeyMappingDataList->at(1).Original_Key == QStringLiteral("A")
            && QKeyMapper::KeyMappingDataList->at(1).Mapping_Keys == QStringList{QStringLiteral("A")}
            && mapper->m_KeyMappingDataTable->rowCount() > QKeyMapper::KeyMappingDataList->size(),
            "fixed GUI adapter opens the new local A row, not the appended Common row");
        mapper->closeItemSetupDialog();
    }
    check(disk() == beforeDisk, "no analysis or memory commit silently persists other drafts");
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks},
        {QStringLiteral("notifications"), notifications}, {QStringLiteral("desktop_input"), false},
        {QStringLiteral("formal_mcp"), false}};
}

QJsonObject QkmMappingAdditionTestAccess::equality(QKeyMapper *mapper)
{
    using namespace QKeyMapperConstants;
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    MAP_KEYDATA base = mapper->createDefaultKeyMappingData(QStringLiteral("A"), QStringLiteral("B"), false);
    base.FloatingButton_SyncGroupId = 7;
    auto changed = base;
    changed.FloatingButton_SyncGroupId = 8;
    check(!(base == changed) && !(changed == base), "sync-group-only configuration change is unequal");
    check(!(QList<MAP_KEYDATA>{base} == QList<MAP_KEYDATA>{changed}), "Qt list equality includes the row sync group");
    check(QKeyMapper::generateMappingCode(base) != QKeyMapper::generateMappingCode(changed),
        "existing mapping serialization includes the sync group independently of equality");
    changed = base; changed.Pure_OriginalKeys.append(QStringLiteral("C"));
    check(base == changed && changed == base, "pure original list stays outside configuration equality");
    changed = base; changed.Pure_MappingKeys.append(QStringLiteral("D"));
    check(base == changed && changed == base, "pure mapping list stays outside configuration equality");
    changed = base; changed.Pure_MappingKeys_KeyUp.append(QStringLiteral("E"));
    check(base == changed && changed == base, "pure key-up list stays outside configuration equality");
    changed = base; changed.LockState = LOCK_STATE_LOCKON;
    check(base == changed && changed == base, "runtime lock state stays outside configuration equality");
    changed = base; changed.Note = QStringLiteral("configuration change");
    check(!(base == changed), "existing note configuration change is still unequal");
    changed = base; changed.Mapping_Keys = QStringList{QStringLiteral("C")};
    check(!(base == changed), "existing mapping configuration change is still unequal");
    const int normal = QKeyMapper::firstNormalMappingTabIndex();
    if (normal < 0) return {{QStringLiteral("passed"), false}, {QStringLiteral("checks"), checks}};
    mapper->forceSwitchKeyMappingTabWidgetIndex(normal);
    *QKeyMapper::KeyMappingDataList = QList<MAP_KEYDATA>{base};
    mapper->refreshTabsForSourceTabChange(normal);
    mapper->clearSaveSettingDirty();
    mapper->showItemSetupDialog(normal, 0);
    mapper->m_ItemSetupDialog->refreshFromCurrentItem();
    check(QKeyMapper::KeyMappingDataList->first() == base && !mapper->m_SaveSettingDirty,
        "opening and refreshing the item editor does not dirty an unchanged valid row");
    auto *floating = mapper->m_ItemSetupDialog->findChild<QFloatingButtonSetupDialog *>();
    if (!floating) return {{QStringLiteral("passed"), false}, {QStringLiteral("checks"), checks}};
    floating->setItemRow(0);
    floating->show();
    floating->refreshFromCurrentItem();
    check(QKeyMapper::KeyMappingDataList->first() == base && !mapper->m_SaveSettingDirty,
        "opening and refreshing floating settings preserves the configuration");
    QSpinBox *group = nullptr;
    int matches = 0;
    for (auto *spin : floating->findChildren<QSpinBox *>()) {
        if (spin->minimum() == FLOATINGBUTTON_SYNCGROUPID_MIN && spin->maximum() == FLOATINGBUTTON_SYNCGROUPID_MAX
            && !spin->specialValueText().isEmpty()) { group = spin; ++matches; }
    }
    auto *buttons = floating->findChild<QDialogButtonBox *>();
    QAbstractButton *apply = nullptr, *revert = nullptr;
    if (buttons) {
        for (auto *button : buttons->buttons()) {
            if (buttons->buttonRole(button) == QDialogButtonBox::ApplyRole) apply = button;
            if (buttons->buttonRole(button) == QDialogButtonBox::ResetRole) revert = button;
        }
    }
    check(matches == 1 && group && apply && revert, "sync group and Apply/Revert controls are identified unambiguously");
    if (matches == 1 && group && apply && revert) {
        group->stepUp();
        QCoreApplication::sendPostedEvents(mapper, QEvent::MetaCall);
        check(QKeyMapper::KeyMappingDataList->first().FloatingButton_SyncGroupId == 8 && mapper->m_SaveSettingDirty,
            "group editing reaches actual configuration and dirty handling");
        clickMappingButton(apply);
        const MAP_KEYDATA applied = QKeyMapper::KeyMappingDataList->first();
        group->stepUp();
        check(QKeyMapper::KeyMappingDataList->first().FloatingButton_SyncGroupId == 9,
            "subsequent group edit remains live before revert");
        clickMappingButton(revert);
        QCoreApplication::sendPostedEvents(mapper, QEvent::MetaCall);
        check(group->value() == 8 && QKeyMapper::KeyMappingDataList->first() == applied,
            "Revert restores the last Apply snapshot including its sync group");
    }
    floating->close();
    mapper->clearSaveSettingDirty();
    mapper->m_ItemSetupDialog->refreshFromCurrentItem();
    check(!mapper->m_SaveSettingDirty, "refreshing after group Apply/Revert does not invent another edit");
    mapper->closeItemSetupDialog();
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks},
        {QStringLiteral("saved_fault_reproduced"), false}, {QStringLiteral("formal_mcp"), false},
        {QStringLiteral("system_input"), false}};
}

QJsonObject QkmMappingAdditionTestAccess::serialization(QKeyMapper *mapper, bool cold)
{
    using namespace QKeyMapperConstants;
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    if (!QDir::currentPath().contains(QStringLiteral("/out/"))
        || QDir::currentPath() != QCoreApplication::applicationDirPath()) {
        return {{QStringLiteral("passed"), false}, {QStringLiteral("reason"), QStringLiteral("An owned out runtime is required")}};
    }
    const QString group = QStringLiteral("Map67Save");
    const QString macroKey = group + QLatin1Char('/') + QLatin1String(MACROLIST);
    const QString notesKey = group + QLatin1Char('/') + QLatin1String(SYNCGROUPNOTES);
    const auto disk = [] {
        QFile ini(QString::fromLatin1(CONFIG_FILENAME));
        return ini.open(QIODevice::ReadOnly) ? ini.readAll() : QByteArray();
    };
    const auto beforeDisk = disk();
    check(!beforeDisk.isEmpty(), "owned INI exists before serialization");
    const bool beforeDirty = mapper->m_SaveSettingDirty;
    const auto keepRules = qScopeGuard([rules = QKeyMapper::s_IgnoreWindowInfoMap] { QKeyMapper::s_IgnoreWindowInfoMap = rules; });
    const auto keepMacros = qScopeGuard([macros = QKeyMapper::s_MappingMacroList] { QKeyMapper::s_MappingMacroList = macros; });
    const auto keepUniversal = qScopeGuard([macros = QKeyMapper::s_UniversalMappingMacroList] { QKeyMapper::s_UniversalMappingMacroList = macros; });
    const auto keepNotes = qScopeGuard([mapper, notes = mapper->m_FloatingButtonSyncGroupNotes] { mapper->m_FloatingButtonSyncGroupNotes = notes; });
    IgnoreWindowInfo rule;
    rule.ruleName = QStringLiteral("Z-规则😀");
    rule.processName = QStringLiteral("example.exe");
    rule.windowTitle = QStringLiteral("日本語\n\"窗口\"");
    rule.className = QStringLiteral("TestClass");
    rule.description = QStringLiteral("note\\path");
    rule.processNameMatchType = WindowInfoMatchType::RegexMatch;
    rule.windowTitleMatchType = WindowInfoMatchType::Equals;
    rule.classNameMatchType = WindowInfoMatchType::EndsWith;
    rule.disabled = true;
    OrderedMap<QString, IgnoreWindowInfo> rules;
    rules.insert(rule.ruleName, rule);
    IgnoreWindowInfo second;
    second.ruleName = QStringLiteral("A-second");
    rules.insert(second.ruleName, second);
    const QVariantList expectedRules{
        QVariantMap{{"ruleName", QStringLiteral("Z-规则😀")}, {"processName", "example.exe"},
            {"windowTitle", QStringLiteral("日本語\n\"窗口\"")}, {"className", "TestClass"},
            {"description", "note\\path"}, {"processNameMatchType", 5}, {"windowTitleMatchType", 1},
            {"classNameMatchType", 4}, {"disabled", true}},
        QVariantMap{{"ruleName", "A-second"}, {"processName", ""}, {"windowTitle", ""}, {"className", ""},
            {"description", ""}, {"processNameMatchType", 2}, {"windowTitleMatchType", 2},
            {"classNameMatchType", 0}, {"disabled", false}}};
    MappingMacroData macro;
    macro.MappingMacro = QStringLiteral("B⏱25»C");
    macro.Category = QStringLiteral("分类／Category");
    macro.Note = QStringLiteral("日本語😀\n\"note\"");
    macro.MacroComments = QStringList{QStringLiteral("前"), QString(), QStringLiteral("後\\tail")};
    OrderedMap<QString, MappingMacroData> macros;
    macros.insert(QStringLiteral("Z-宏"), macro);
    macros.insert(QStringLiteral("A-empty"), MappingMacroData());
    const QVariantList expectedMacros{
        QVariantMap{{MACROLIST_FIELD_MACRONAME, QStringLiteral("Z-宏")}, {MACROLIST_FIELD_MACROCONTENT, QStringLiteral("B⏱25»C")},
            {MACROLIST_FIELD_MACROCATEGORY, QStringLiteral("分类／Category")}, {MACROLIST_FIELD_MACRONOTE, QStringLiteral("日本語😀\n\"note\"")},
            {MACROLIST_FIELD_MACROCOMMENTS, QVariantList{QStringLiteral("前"), QString(), QStringLiteral("後\\tail")}}},
        QVariantMap{{MACROLIST_FIELD_MACRONAME, "A-empty"}, {MACROLIST_FIELD_MACROCONTENT, ""},
            {MACROLIST_FIELD_MACROCATEGORY, ""}, {MACROLIST_FIELD_MACRONOTE, ""}, {MACROLIST_FIELD_MACROCOMMENTS, QVariantList()}}};
    OrderedMap<QString, MappingMacroData> universal;
    universal.insert(QStringLiteral("A-empty"), MappingMacroData());
    const QVariantList expectedUniversal{expectedMacros.last()};
    const QHash<int, QString> notes{{7, QStringLiteral("联动😀\n日本語")}, {999, QStringLiteral("末尾\\\"")}, {8, QString()}};
    const auto expectedNotes = [](const QVariantList &values) {
        QHash<int, QString> actual;
        for (const auto &value : values) {
            const auto map = value.toMap();
            if (map.size() != 2 || map.value(SYNCGROUPNOTES_FIELD_NOTE).toString().isEmpty()) return false;
            actual.insert(map.value(SYNCGROUPNOTES_FIELD_GROUPID).toInt(), map.value(SYNCGROUPNOTES_FIELD_NOTE).toString());
        }
        return values.size() == 2 && actual == QHash<int, QString>{{7, QStringLiteral("联动😀\n日本語")}, {999, QStringLiteral("末尾\\\"")}};
    };
    check(mapper->serializeIgnoreRules(rules) == expectedRules, "ignore rules preserve every field, enum, Unicode and insertion order");
    const auto capturedMacros = mapper->serializeMappingMacros(macros);
    check(capturedMacros == expectedMacros
        && capturedMacros.first().toMap().value(MACROLIST_FIELD_MACROCOMMENTS).userType() == QMetaType::QVariantList,
        "macro fields, insertion order and QVariantList comments retain empty entries");
    check(expectedNotes(mapper->serializeSyncGroupNotes(notes)), "notes preserve group IDs and omit only empty notes");
    macros[QStringLiteral("Z-宏")].MacroComments[0] = QStringLiteral("changed");
    check(capturedMacros == expectedMacros, "serialized values do not retain mutable source aliases");
    macros[QStringLiteral("Z-宏")] = macro;
    check(disk() == beforeDisk && mapper->m_SaveSettingDirty == beforeDirty, "pure serialization does not change disk or dirty");
    const auto seed = [&] {
        QKeyMapper::s_IgnoreWindowInfoMap = rules;
        QKeyMapper::s_MappingMacroList = macros;
        QKeyMapper::s_UniversalMappingMacroList = universal;
        mapper->m_FloatingButtonSyncGroupNotes = notes;
    };
    const auto save = [&] {
        mapper->saveIgnoreRulesToINI();
        mapper->saveMacroListToINI(group);
        mapper->saveUniversalMacroListToINI();
        mapper->saveSyncGroupNotesToINI(group);
    };
    const auto load = [&] {
        mapper->loadIgnoreRulesFromINI();
        mapper->loadMacroListFromINI(group);
        mapper->loadUniversalMacroListFromINI();
        mapper->loadSyncGroupNotesFromINI(group);
    };
    QSettings ini(QString::fromLatin1(CONFIG_FILENAME), QSettings::IniFormat);
    QKeyMapperQtCompat::setIniCodecUtf8(ini);
    const auto otherValues = [&] {
        const QSet<QString> ownKeys{QString::fromLatin1(IGNOREWINDOWINFO_RULES), macroKey,
            QString::fromLatin1(MACROLIST_UNIVERSAL), notesKey};
        QVariantMap values;
        const auto keys = ini.allKeys();
        for (const auto &key : keys) if (!ownKeys.contains(key)) values.insert(key, ini.value(key));
        return values;
    };
    const auto beforeOtherValues = otherValues();
    if (!cold) { seed(); save(); }
    ini.sync();
    check(ini.status() == QSettings::NoError && ini.value(IGNOREWINDOWINFO_RULES).toList() == expectedRules
        && ini.value(macroKey).toList() == expectedMacros && ini.value(MACROLIST_UNIVERSAL).toList() == expectedUniversal
        && expectedNotes(ini.value(notesKey).toList()), cold ? "cold process reads actual persisted payloads" : "existing save wrappers write exact expected payloads");
    QKeyMapper::s_IgnoreWindowInfoMap.clear(); QKeyMapper::s_MappingMacroList.clear();
    QKeyMapper::s_UniversalMappingMacroList.clear(); mapper->m_FloatingButtonSyncGroupNotes.clear();
    load();
    check(mapper->serializeIgnoreRules(QKeyMapper::s_IgnoreWindowInfoMap) == expectedRules
        && mapper->serializeMappingMacros(QKeyMapper::s_MappingMacroList) == expectedMacros
        && mapper->serializeMappingMacros(QKeyMapper::s_UniversalMappingMacroList) == expectedUniversal
        && expectedNotes(mapper->serializeSyncGroupNotes(mapper->m_FloatingButtonSyncGroupNotes)),
        "existing loaders restore rules, both macro scopes and group notes");
    if (!cold) {
        const auto savedDisk = disk();
        mapper->saveMacroListToINI(QString()); mapper->saveSyncGroupNotesToINI(QString());
        ini.sync();
        check(disk() == savedDisk, "empty profile names remain no-ops");
        QKeyMapper::s_IgnoreWindowInfoMap.clear(); QKeyMapper::s_MappingMacroList.clear();
        QKeyMapper::s_UniversalMappingMacroList.clear(); mapper->m_FloatingButtonSyncGroupNotes.clear();
        save(); ini.sync();
        check(ini.status() == QSettings::NoError && ini.contains(IGNOREWINDOWINFO_RULES) && ini.contains(macroKey)
            && ini.contains(MACROLIST_UNIVERSAL) && ini.contains(notesKey)
            && ini.value(IGNOREWINDOWINFO_RULES).toList().isEmpty() && ini.value(macroKey).toList().isEmpty()
            && ini.value(MACROLIST_UNIVERSAL).toList().isEmpty() && ini.value(notesKey).toList().isEmpty(),
            "empty lists overwrite stale data in all four save paths");
        load();
        check(QKeyMapper::s_IgnoreWindowInfoMap.isEmpty() && QKeyMapper::s_MappingMacroList.isEmpty()
            && QKeyMapper::s_UniversalMappingMacroList.isEmpty() && mapper->m_FloatingButtonSyncGroupNotes.isEmpty(),
            "empty payloads restore as empty, without default rules or stale rows");
        seed(); save(); ini.sync();
        check(ini.status() == QSettings::NoError, "final payload is flushed for an independent cold process");
    } else check(disk() == beforeDisk, "cold verification does not rewrite the INI");
    check(otherValues() == beforeOtherValues, "auxiliary saving changes no unrelated profile or global key");
    check(mapper->m_SaveSettingDirty == beforeDirty, "auxiliary persistence retains the existing dirty behavior");
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks},
        {QStringLiteral("cold"), cold}, {QStringLiteral("formal_mcp"), false}, {QStringLiteral("system_input"), false}};
}

QJsonObject QkmMappingAdditionTestAccess::uiLocalization(QKeyMapper *mapper)
{
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    const int language = qEnvironmentVariableIntValue("QKM_UI_LANGUAGE");
    if (language < 0 || language > 2 || mapper->m_KeyMapStatus != QKeyMapper::KEYMAP_IDLE
        || !QDir::currentPath().contains(QStringLiteral("/out/"))) return {{QStringLiteral("passed"), false}};
    const QStringList locales{QStringLiteral("zh_CN"), QStringLiteral("en_US"), QStringLiteral("ja_JP")};
    QTranslator expected;
    if (!expected.load(QDir(qEnvironmentVariable("QKM_UI_QM_DIRECTORY")).filePath(
            QStringLiteral("QKeyMapper_%1.qm").arg(locales.at(language))))) return {{QStringLiteral("passed"), false}};
    const auto translation = [&](const char *context, const char *source) {
        const QString text = expected.translate(context, source);
        return text.isEmpty() ? QString::fromUtf8(source) : text;
    };
    auto *selector = mapper->findChild<QComboBox *>(QStringLiteral("languageComboBox"));
    auto *entry = mapper->m_GeneralAdvancedDialog->findChild<QPushButton *>(QStringLiteral("mcpControlButton"));
    auto *button = mapper->findChild<QToolButton *>(QStringLiteral("keymapButton"));
    if (!selector || !entry || !button) return {{QStringLiteral("passed"), false}};
    if (selector->currentIndex() == language) mapper->reloadUILanguage();
    else selector->setCurrentIndex(language);
    const QString title = translation("QkmAutomation::ControlDialog", "Agent / MCP Control");
    check(entry->text() == title && mapper->m_McpControlDialog->windowTitle() == title,
        "actual advanced entry and control title use the embedded formal QM");
    // Tray captions are refreshed lazily by the existing context-menu activation handler.
    emit mapper->m_SysTrayIcon->activated(QSystemTrayIcon::Context);
    check(mapper->m_TrayIconMenu_McpAction && mapper->m_TrayIconMenu_McpAction->text() == title,
        "actual tray action uses the same formal translation");
    mapper->m_SysTrayIconMenu->hide();
    check(entry->focusPolicy() == Qt::NoFocus && !entry->autoDefault() && !entry->isDefault(),
        "actual advanced entry has the selected focus and default policy");
    const quint64 sourceEpoch = mapper->m_AutomationSourceEpoch;
    const auto phase = mapper->m_mappingStopPhase;
    const bool inputClosed = QKeyMapper_Worker::s_MappingInputClosed.loadAcquire();
    const quint64 work = SendInputWorkItem::outstanding();
    const QString tooltip = button->toolTip();
    {
        QScopedValueRollback<decltype(mapper->m_KeyMapStatus)> state(mapper->m_KeyMapStatus, QKeyMapper::KEYMAP_STOPPING);
        QScopedValueRollback<bool> timeout(mapper->m_mappingStopTimedOut, false);
        mapper->updateMappingStopButtonTooltip();
        check(button->toolTip() == translation("QKeyMapper", "Mapping cleanup is pending. Editing and restarting remain disabled."),
            "pending stop display uses the formal QM without beginning a stop");
        const auto busy = mapper->prepareMappingAddition(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex, 0, QStringLiteral("A"), QStringLiteral("B"));
        check(busy.errorCode == QStringLiteral("context_busy") && busy.errorMessage == translation("QKeyMapper",
            "Mapping edits require idle state and completed stop cleanup. Close the current modal dialog and try again."),
            "existing busy guard reports its formal translation without an edit");
        mapper->m_mappingStopTimedOut = true;
        mapper->setUILanguage(language);
        check(button->toolTip() == translation("QKeyMapper", "Stop wait timed out. Cleanup continues; editing and restarting remain disabled."),
            "language refresh retains the timeout display state");
    }
    button->setToolTip(tooltip);
    check(mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE && mapper->m_mappingStopPhase == phase
        && mapper->m_AutomationSourceEpoch == sourceEpoch && SendInputWorkItem::outstanding() == work
        && QKeyMapper_Worker::s_MappingInputClosed.loadAcquire() == inputClosed,
        "display checks restore state and do not alter source or work admission");
    const auto stale = mapper->prepareMappingAddition(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex, -1, QStringLiteral("A"), QStringLiteral("B"));
    check(stale.errorCode == QStringLiteral("invalid_arguments") && stale.errorMessage == translation("QKeyMapper",
        "The mapping target or conflicts changed. Review the operation again."), "stale-context explanation uses its formal QM");
    mapper->m_GeneralAdvancedDialog->show();
    clickMappingButton(entry);
    check(mapper->m_McpControlDialog->isVisible() && !mapper->m_McpControlDialog->server().isEnabled(),
        "actual advanced entry opens the disabled control window by Qt mouse events");
    auto *close = mapper->m_McpControlDialog->findChild<QPushButton *>(QStringLiteral("mcpClose"));
    check(close && close->text() == translation("QkmAutomation::ControlDialog", "Close"), "actual Close caption uses its formal QM");
    if (close) clickMappingButton(close);
    check(!mapper->m_McpControlDialog->isVisible() && !mapper->m_McpControlDialog->server().isEnabled(),
        "actual Close button closes the window without toggling the interface");
    clickMappingButton(entry);
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks}, {QStringLiteral("language"), language},
        {QStringLiteral("pid"), int(GetCurrentProcessId())}, {QStringLiteral("advanced_hwnd"), QString::number(quintptr(mapper->m_GeneralAdvancedDialog->winId()))},
        {QStringLiteral("control_hwnd"), QString::number(quintptr(mapper->m_McpControlDialog->winId()))},
        {QStringLiteral("mapping_started"), false}, {QStringLiteral("system_input"), false}};
}

QJsonObject QkmMappingAdditionTestAccess::generalSave(QKeyMapper *mapper, bool rich)
{
    using namespace QKeyMapperConstants;
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    if (!QDir::currentPath().contains(QStringLiteral("/out/"))
        || QDir::currentPath() != QCoreApplication::applicationDirPath()) {
        return {{QStringLiteral("passed"), false}, {QStringLiteral("reason"), QStringLiteral("An owned out runtime is required")}};
    }
    auto *name = mapper->findChild<QLineEdit *>(QStringLiteral("settingNameLineEdit"));
    auto *save = mapper->findChild<QPushButton *>(QStringLiteral("savemaplistButton"));
    auto *language = mapper->findChild<QComboBox *>(QStringLiteral("languageComboBox"));
    auto *startup = mapper->findChild<QCheckBox *>(QStringLiteral("startupAutoMonitoringCheckBox"));
    int target = QKeyMapper::firstNormalMappingTabIndex();
    if (!name || !save || !language || !startup || target < 0) return {{QStringLiteral("passed"), false}};
    mapper->forceSwitchKeyMappingTabWidgetIndex(target);
    *QKeyMapper::KeyMappingDataList = QList<MAP_KEYDATA>{mapper->createDefaultKeyMappingData(QStringLiteral("A"), QStringLiteral("B"), false)};
    if (rich) {
        const bool attached = qEnvironmentVariableIntValue("QKM_LIFECYCLE_COMMON_ATTACHED") == 1;
        mapper->m_MappingAdvancedDialog->setCommonMappingTableEnabled(attached);
        mapper->updateCommonMappingTabVisibility();
        target = QKeyMapper::firstNormalMappingTabIndex();
        if (target < 0) return {{QStringLiteral("passed"), false}};
        mapper->forceSwitchKeyMappingTabWidgetIndex(target);
        *QKeyMapper::KeyMappingDataList = QList<MAP_KEYDATA>{mapper->createDefaultKeyMappingData(QStringLiteral("A"), QStringLiteral("B"), false)};
        if (!mapper->addTabToKeyMappingTabWidget(QStringLiteral("Map67序列"), false)
            || !mapper->addTabToKeyMappingTabWidget(QStringLiteral("Map67空"), false)) return {{QStringLiteral("passed"), false}};
        MAP_KEYDATA sequence = mapper->createDefaultKeyMappingData(QStringLiteral("F13"), QStringLiteral("C»D"), false);
        sequence.MappingKeys_KeyUp = QStringList{QStringLiteral("E"), QStringLiteral("F")};
        sequence.Pure_MappingKeys_KeyUp = sequence.MappingKeys_KeyUp;
        sequence.MappingKeys_Comments = QStringList{QStringLiteral("中文😀\npress"), QString()};
        sequence.MappingKeys_KeyUp_Comments = QStringList{QStringLiteral("release\\0"), QStringLiteral("日本語")};
        sequence.Note = QStringLiteral("note\n第二行");
        sequence.Category = QStringLiteral("类别α");
        sequence.FloatingButton_Label = QStringLiteral("labelα");
        sequence.FloatingButton_ButtonColor = QColor(QStringLiteral("#123456"));
        sequence.FloatingButton_BorderWidth = 3;
        sequence.FloatingButton_Width = 180;
        sequence.FloatingButton_Height = 48;
        sequence.FloatingButton_FontFamily = QStringLiteral("Segoe UI");
        sequence.FloatingButton_NormalOpacity = 0.25;
        sequence.FloatingButton_Opacity = 0.25;
        sequence.FloatingButton_PressedOpacity = 0.5;
        sequence.FloatingButton_LockedOpacity = 0.75;
        sequence.FloatingButton_SyncGroupId = 23;
        sequence.Crosshair_CenterSize = 7;
        sequence.Crosshair_CenterColor = QColor(QStringLiteral("#11aa55"));
        sequence.FloatingButton_Enable = false;
        auto &first = QKeyMapper::s_KeyMappingTabInfoList[target];
        first.KeyMappingData->append(sequence);
        first.TabFontColor = QColor(QStringLiteral("#234567"));
        first.TabBackgroundColor = QColor(QStringLiteral("#80345678"));
        first.TabHotkey = QStringLiteral("F17");
        first.IncludeCommonMappingTable = attached;
        first.CommonMappingCollapsed = attached;
        first.FloatingWindow_Position = QPoint(-27, 96);
        first.FloatingWindow_Size = QSize(288, 144);
        first.FloatingWindow_BackgroundColor = QColor(QStringLiteral("#a0112233"));
        first.FloatingWindow_Opacity = 0.75;
        first.FloatingWindow_Radius = 13;
        first.TabCustomImage_Padding = 7;
        first.TabCustomImage_TrayIconPixel = QSize(24, 24);
        first.TabCustomImage_ShowAsTrayIcon = false;
        first.TabCustomImage_ShowAsFloatingWindow = false;
        MAP_KEYDATA common = mapper->createDefaultKeyMappingData(QStringLiteral("G"), QStringLiteral("H"), false);
        common.MappingKeys_Comments = QStringList{QStringLiteral("common备注")};
        common.FloatingButton_SyncGroupId = 17;
        if (attached) {
            const int commonIndex = QKeyMapper::findCommonMappingTabIndex();
            if (commonIndex < 0) return {{QStringLiteral("passed"), false}};
            *QKeyMapper::s_KeyMappingTabInfoList.at(commonIndex).KeyMappingData = QList<MAP_KEYDATA>{common};
        }
        else mapper->m_CommonMappingData = QList<MAP_KEYDATA>{common};
        for (auto &tab : QKeyMapper::s_KeyMappingTabInfoList) {
            if (tab.TabName == QStringLiteral("Map67序列")) {
                auto row = mapper->createDefaultKeyMappingData(QStringLiteral("X"), QStringLiteral("Y"), false);
                row.Disabled = true;
                *tab.KeyMappingData = QList<MAP_KEYDATA>{row};
            }
            if (tab.TabName == QStringLiteral("Map67空")) tab.KeyMappingData->clear();
        }
        mapper->forceSwitchKeyMappingTabWidgetIndex(target);
    }
    mapper->refreshTabsForSourceTabChange(target);
    mapper->m_FavoriteOriginalKeys = QStringList{QStringLiteral("A")};
    mapper->m_RecentOriginalKeys = QStringList{QStringLiteral("A")};
    mapper->m_FavoriteMappingKeys = QStringList{QStringLiteral("B")};
    mapper->m_RecentMappingKeys = QStringList{QStringLiteral("B")};
    mapper->move(64, 96);
    name->setText(QStringLiteral("Map67General"));
    mapper->markSaveSettingDirty();
    check(mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE && save->isEnabled() && !startup->isChecked()
        && language->currentIndex() == LANGUAGE_CHINESE, "owned Chinese GUI fixture is idle with automatic mapping disabled");
    const QPoint beforePosition = mapper->pos();
    const QSize beforeBase = mapper->m_normalWindowBaseSize;
    QFile beforeFile(QString::fromLatin1(CONFIG_FILENAME));
    if (!beforeFile.open(QIODevice::ReadOnly)) return {{QStringLiteral("passed"), false}};
    const auto beforeBytes = beforeFile.readAll();
    beforeFile.close();
    const quint64 beforeLoad = mapper->m_MappingLoadGeneration;
    const auto sourceState = [&] {
        QJsonArray tabs;
        for (const auto &tab : std::as_const(QKeyMapper::s_KeyMappingTabInfoList)) {
            QJsonArray rows;
            for (const auto &row : *tab.KeyMappingData) {
                rows.append(QJsonObject{{QStringLiteral("configuration"), QKeyMapper::generateMappingCode(row)},
                    {QStringLiteral("original"), QJsonArray::fromStringList(row.Pure_OriginalKeys)},
                    {QStringLiteral("mapping"), QJsonArray::fromStringList(row.Pure_MappingKeys)},
                    {QStringLiteral("key_up"), QJsonArray::fromStringList(row.Pure_MappingKeys_KeyUp)},
                    {QStringLiteral("lock"), int(row.LockState)}});
            }
            tabs.append(QJsonObject{{QStringLiteral("name"), tab.TabName}, {QStringLiteral("rows"), rows}});
        }
        return tabs;
    };
    const auto beforeSources = sourceState();
    const auto mappingValues = mapper->collectMappingTableSaveValues();
    const auto repeatedMappingValues = mapper->collectMappingTableSaveValues();
    const auto generalValues = mapper->collectGeneralSettingSaveValues(language->currentIndex());
    const auto repeatValues = mapper->collectGeneralSettingSaveValues(language->currentIndex());
    const auto sameValues = [](const auto &left, const auto &right) {
        if (left.keys() != right.keys()) return false;
        for (auto it = left.begin(); it != left.end(); ++it) if (it.value() != right.value(it.key())) return false;
        return true;
    };
    if (!beforeFile.open(QIODevice::ReadOnly)) return {{QStringLiteral("passed"), false}};
    check(sameValues(generalValues, repeatValues) && beforeFile.readAll() == beforeBytes && mapper->m_SaveSettingDirty
        && mapper->m_MappingLoadGeneration == beforeLoad && QKeyMapper::KeyMappingDataList->first().Original_Key == QStringLiteral("A"),
        "general value collection is repeatable without disk, dirty, load or mapping mutation");
    beforeFile.close();
    bool languagesMatch = true;
    for (int value : {LANGUAGE_CHINESE, LANGUAGE_ENGLISH, LANGUAGE_JAPANESE, -1}) {
        auto expected = generalValues;
        expected[QString::fromLatin1(LANGUAGE_INDEX)] = value == LANGUAGE_ENGLISH || value == LANGUAGE_JAPANESE ? value : LANGUAGE_CHINESE;
        languagesMatch = languagesMatch && sameValues(mapper->collectGeneralSettingSaveValues(value), expected);
    }
    check(languagesMatch, "captured language follows the existing three-language and fallback save semantics");
    check(mappingValues.size() == 97 && sameValues(mappingValues, repeatedMappingValues)
        && sourceState() == beforeSources && mapper->m_SaveSettingDirty && mapper->m_MappingLoadGeneration == beforeLoad,
        "mapping payload has all ordered fields without changing source, derived lists, lock, dirty or load state");
    if (rich) {
        check(mappingValues.value(QString::fromLatin1(MAPPINGTABLE_FLOATINGWINDOW_POSITIONLIST)).toList().first().toPoint() == QPoint(-27, 96)
            && mappingValues.value(QString::fromLatin1(MAPPINGTABLE_FLOATINGWINDOW_SIZELIST)).toList().first().toSize() == QSize(288, 144)
            && mappingValues.value(QString::fromLatin1(MAPPINGTABLE_TABCUSTOMIMAGE_TRAYICON_PIXELLIST)).toList().first().toSize() == QSize(24, 24),
            "mapping payload preserves Qt point and size types before the existing reload normalization");
    }
    // The baseline is driven through the real Save button, not the serialization helper.
    clickMappingButton(save);
    QSettings ini(QString::fromLatin1(CONFIG_FILENAME), QSettings::IniFormat);
    QKeyMapperQtCompat::setIniCodecUtf8(ini);
    ini.sync();
    bool valuesPersisted = true;
    for (auto it = generalValues.begin(); it != generalValues.end(); ++it) {
        valuesPersisted = valuesPersisted && ini.contains(it.key()) && ini.value(it.key()) == it.value();
    }
    check(valuesPersisted, "GUI Save persists every collected general value without type or content changes");
    bool mappingValuesPersisted = true;
    for (auto it = mappingValues.begin(); it != mappingValues.end(); ++it) {
        mappingValuesPersisted = mappingValuesPersisted && ini.contains(QStringLiteral("Map67General/") + it.key())
            && ini.value(QStringLiteral("Map67General/") + it.key()) == it.value();
    }
    check(mappingValuesPersisted, "GUI Save persists every collected mapping value with original Qt types and content");
    check(ini.status() == QSettings::NoError && ini.value(SETTINGSELECT).toString() == QStringLiteral("Map67General")
        && mapper->m_LoadedMappingSettingGroup == QStringLiteral("Map67General") && !mapper->m_SaveSettingDirty,
        "GUI Save flushes the named profile and completes existing reload and dirty clearing");
    check(ini.value(LANGUAGE_INDEX).toInt() == LANGUAGE_CHINESE && !ini.value(STARTUP_AUTOMONITORING).toBool()
        && ini.value(LAST_WINDOWPOSITION).toPoint() == beforePosition && ini.value(LAST_WINDOW_SIZE).toSize() == beforeBase,
        "GUI preserves language, startup safety and original geometry base semantics");
    check(ini.value(KEYLISTCOMBOBOX_FAVORITE_ORIGINAL_KEYS).toStringList() == QStringList{QStringLiteral("A")}
        && ini.value(KEYLISTCOMBOBOX_RECENT_ORIGINAL_KEYS).toStringList() == QStringList{QStringLiteral("A")}
        && ini.value(KEYLISTCOMBOBOX_FAVORITE_MAPPING_KEYS).toStringList() == QStringList{QStringLiteral("B")}
        && ini.value(KEYLISTCOMBOBOX_RECENT_MAPPING_KEYS).toStringList() == QStringList{QStringLiteral("B")},
        "all four existing key collections are persisted with the GUI save scope");
    check(QKeyMapper::KeyMappingDataList->size() == (rich ? 2 : 1) && QKeyMapper::KeyMappingDataList->first().Original_Key == QStringLiteral("A")
        && QKeyMapper::KeyMappingDataList->first().Mapping_Keys == QStringList{QStringLiteral("B")},
        "GUI save and reload retain the valid mapping");
    if (rich) {
        const auto state = tableState(mapper);
        passed = passed && state.value(QStringLiteral("passed")).toBool();
        for (const auto &entry : state.value(QStringLiteral("checks")).toArray()) checks.append(entry);
    }
    const auto values = [](QSettings &settings) {
        QVariantMap map;
        const auto keys = settings.allKeys();
        for (const auto &key : keys) map.insert(key, settings.value(key));
        return map;
    };
    const QString baseline = qEnvironmentVariable("QKM_LIFECYCLE_SAVE_BASELINE");
    if (!baseline.isEmpty()) {
        QDir parent(QCoreApplication::applicationDirPath());
        parent.cdUp();
        if (!QFileInfo(baseline).isAbsolute() || QFileInfo(baseline).absolutePath() != parent.absolutePath()
            || !QFileInfo(baseline).fileName().startsWith(QStringLiteral("baseline-")) || !QFile::exists(baseline)) {
            return {{QStringLiteral("passed"), false}, {QStringLiteral("reason"), QStringLiteral("Owned baseline path mismatch")}};
        }
        QSettings expected(baseline, QSettings::IniFormat);
        QKeyMapperQtCompat::setIniCodecUtf8(expected);
        // Parse both disk images independently of the live writer's shared pending cache.
        QFile persisted(QString::fromLatin1(CONFIG_FILENAME));
        QTemporaryFile currentCopy(QDir::currentPath() + QStringLiteral("/general-current-XXXXXX.ini"));
        if (!persisted.open(QIODevice::ReadOnly) || !currentCopy.open()) return {{QStringLiteral("passed"), false}};
        const auto bytes = persisted.readAll();
        if (persisted.error() != QFile::NoError || currentCopy.write(bytes) != bytes.size() || !currentCopy.flush()) {
            return {{QStringLiteral("passed"), false}};
        }
        currentCopy.close();
        QSettings diskValues(currentCopy.fileName(), QSettings::IniFormat);
        QKeyMapperQtCompat::setIniCodecUtf8(diskValues);
        const auto actualValues = values(diskValues);
        const auto expectedValues = values(expected);
        check(diskValues.status() == QSettings::NoError && expected.status() == QSettings::NoError && actualValues == expectedValues,
            "complete GUI saved key set and typed values match the pre-extraction baseline");
        QJsonArray differences;
        QSet<QString> keys;
        for (auto it = actualValues.cbegin(); it != actualValues.cend(); ++it) keys.insert(it.key());
        for (auto it = expectedValues.cbegin(); it != expectedValues.cend(); ++it) keys.insert(it.key());
        for (const auto &key : std::as_const(keys)) if (!actualValues.contains(key) || !expectedValues.contains(key)
            || actualValues.value(key) != expectedValues.value(key)) differences.append(key);
        return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks},
            {QStringLiteral("different_keys"), differences}, {QStringLiteral("saved_key_count"), actualValues.size()},
            {QStringLiteral("formal_mcp"), false}, {QStringLiteral("system_input"), false}};
    }
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks},
        {QStringLiteral("saved_key_count"), ini.allKeys().size()}, {QStringLiteral("formal_mcp"), false}, {QStringLiteral("system_input"), false}};
}

QJsonObject QkmMappingAdditionTestAccess::profileSave(QKeyMapper *mapper, bool cold)
{
    using namespace QKeyMapperConstants;
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    if (!QDir::currentPath().contains(QStringLiteral("/out/"))
        || QDir::currentPath() != QCoreApplication::applicationDirPath()) return {{QStringLiteral("passed"), false}};
    const auto disk = [] {
        QFile file(QString::fromLatin1(CONFIG_FILENAME));
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    const auto allValues = [](QSettings &ini) {
        QVariantMap values;
        for (const auto &key : ini.allKeys()) values.insert(key, ini.value(key));
        return values;
    };
    const QByteArray before = disk();
    const bool nondefault = qEnvironmentVariableIntValue("QKM_LIFECYCLE_PROFILE_NONDEFAULT") == 1;
    const bool dirtyBefore = mapper->m_SaveSettingDirty;
    const quint64 loadBefore = mapper->m_MappingLoadGeneration;
    const quint64 sourceBefore = mapper->m_AutomationSourceEpoch;
    const auto profileValues = mapper->collectProfileParameterSaveValues();
    const auto repeatedValues = mapper->collectProfileParameterSaveValues();
    bool repeated = profileValues.keys() == repeatedValues.keys();
    for (auto it = profileValues.begin(); it != profileValues.end(); ++it) {
        repeated = repeated && it.value() == repeatedValues.value(it.key())
            && it.value().userType() == repeatedValues.value(it.key()).userType();
    }
    check(profileValues.size() == 36 && repeated && profileValues.keys().first() == QString::fromLatin1(KEY2MOUSE_X_SPEED)
        && profileValues.keys().last() == QString::fromLatin1(MOUSE2VJOY_RECENTER_TIMEOUT),
        "profile parameter collection retains all ordered Qt scalar values and types");
    check(disk() == before && mapper->m_SaveSettingDirty == dirtyBefore && mapper->m_MappingLoadGeneration == loadBefore
        && mapper->m_AutomationSourceEpoch == sourceBefore,
        "pure parameter collection changes no disk, dirty, load generation or source epoch");
    const QString group = cold ? QStringLiteral("Map74Profile") : QStringLiteral("Profile74Seed");
    auto *name = mapper->findChild<QLineEdit *>(QStringLiteral("settingNameLineEdit"));
    auto *save = mapper->findChild<QPushButton *>(QStringLiteral("savemaplistButton"));
    auto *startup = mapper->findChild<QCheckBox *>(QStringLiteral("startupAutoMonitoringCheckBox"));
    check(name && save && startup && !before.isEmpty() && !startup->isChecked()
        && mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE && mapper->m_LoadedMappingSettingGroup == group
        && QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() == HOOKPROC_STATE_STOPPED,
        "owned profile is loaded by product startup and mapping remains stopped");
    if (!name || !save || !startup) return {{QStringLiteral("passed"), false}, {QStringLiteral("checks"), checks}};
    check(QKeyMapper::KeyMappingDataList && QKeyMapper::KeyMappingDataList->size() == 1
        && QKeyMapper::KeyMappingDataList->first().Original_Key == QStringLiteral("A")
        && QKeyMapper::KeyMappingDataList->first().Mapping_Keys == QStringList{QStringLiteral("B")},
        "startup restores the independently seeded valid mapping");
    if (nondefault) {
        check(mapper->m_MappingAdvancedDialog->getMouseXSpeed() == 13
            && mapper->m_MappingAdvancedDialog->getMouseYSpeed() == 17
            && mapper->m_MappingAdvancedDialog->getMousePollingInterval() == 8,
            "startup restores the nondefault key-to-mouse parameters");
        check(mapper->m_MappingAdvancedDialog->getLeftTriggerPressThreshold() == 80
            && mapper->m_MappingAdvancedDialog->getLeftTriggerLightPressThreshold() == 50
            && mapper->m_MappingAdvancedDialog->getLeftTriggerReleaseThreshold() == 20,
            "startup restores the ordered trigger threshold triple");
        check(qFuzzyCompare(mapper->m_MappingAdvancedDialog->getGamepadTouchpadXSpeed(), 1.5)
            && mapper->m_MappingAdvancedDialog->getGamepadTouchpadInvertX(),
            "startup restores floating-point and boolean touchpad parameters");
    }
    QSettings ini(QString::fromLatin1(CONFIG_FILENAME), QSettings::IniFormat);
    QKeyMapperQtCompat::setIniCodecUtf8(ini);
    ini.beginGroup(QStringLiteral("Keep74"));
    const QVariantMap otherProfile = allValues(ini);
    ini.endGroup();
    if (!cold) {
        // Exercise the ordinary name editor and Save button without calling the save operation.
        name->setFocus();
        QKeyEvent select(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier);
        QKeyEvent selected(QEvent::KeyRelease, Qt::Key_A, Qt::ControlModifier);
        QCoreApplication::sendEvent(name, &select);
        QCoreApplication::sendEvent(name, &selected);
        for (const QChar character : QStringLiteral("Map74Profile")) {
            QKeyEvent press(QEvent::KeyPress, 0, Qt::NoModifier, QString(character));
            QKeyEvent release(QEvent::KeyRelease, 0, Qt::NoModifier, QString(character));
            QCoreApplication::sendEvent(name, &press);
            QCoreApplication::sendEvent(name, &release);
        }
        check(name->text() == QStringLiteral("Map74Profile") && save->isEnabled(),
            "Qt editor events prepare a distinct profile before GUI Save");
        clickMappingButton(save);
    }
    ini.sync();
    const QString prefix = QStringLiteral("Map74Profile/");
    check(ini.status() == QSettings::NoError && ini.value(SETTINGSELECT).toString() == QStringLiteral("Map74Profile")
        && mapper->m_LoadedMappingSettingGroup == QStringLiteral("Map74Profile") && !mapper->m_SaveSettingDirty,
        "GUI save or independent startup confirms the selected persisted profile and clean state");
    bool parametersPersisted = true;
    for (auto it = profileValues.begin(); it != profileValues.end(); ++it) {
        parametersPersisted = parametersPersisted && ini.contains(prefix + it.key()) && ini.value(prefix + it.key()) == it.value();
    }
    check(parametersPersisted, "GUI save or independent cold read retains every collected profile parameter");
    if (nondefault) {
        check(ini.value(prefix + KEY2MOUSE_X_SPEED).toInt() == 13
            && ini.value(prefix + KEY2MOUSE_Y_SPEED).toInt() == 17
            && ini.value(prefix + KEY2MOUSE_POLLING_INTERVAL).toInt() == 8
            && ini.value(prefix + GAMEPAD_LT_THRESHOLD_PRESS).toInt() == 80
            && ini.value(prefix + GAMEPAD_LT_THRESHOLD_LIGHTPRESS).toInt() == 50
            && ini.value(prefix + GAMEPAD_LT_THRESHOLD_RELEASE).toInt() == 20
            && qFuzzyCompare(ini.value(prefix + GAMEPAD_TOUCHPAD_X_SPEED).toDouble(), 1.5)
            && ini.value(prefix + GAMEPAD_TOUCHPAD_INVERT_X).toBool(),
            "saved scalar values retain independently specified nondefault expectations");
    }
    ini.beginGroup(QStringLiteral("Keep74"));
    check(!otherProfile.isEmpty() && allValues(ini) == otherProfile, "saving does not modify the other stored profile");
    ini.endGroup();
    const QByteArray after = disk();
    if (cold) check(after == before, "independent cold verification does not write the INI");
    const QString baseline = qEnvironmentVariable("QKM_LIFECYCLE_SAVE_BASELINE");
    if (!baseline.isEmpty()) {
        QDir parent(QCoreApplication::applicationDirPath());
        parent.cdUp();
        if (!QFileInfo(baseline).isAbsolute() || QFileInfo(baseline).absolutePath() != parent.absolutePath()
            || !QFileInfo(baseline).fileName().startsWith(QStringLiteral("baseline-"))) return {{QStringLiteral("passed"), false}};
        QSettings expected(baseline, QSettings::IniFormat);
        QKeyMapperQtCompat::setIniCodecUtf8(expected);
        QTemporaryFile copy(QDir::currentPath() + QStringLiteral("/profile-current-XXXXXX.ini"));
        if (!copy.open() || copy.write(after) != after.size() || !copy.flush()) return {{QStringLiteral("passed"), false}};
        copy.close();
        QSettings actual(copy.fileName(), QSettings::IniFormat);
        QKeyMapperQtCompat::setIniCodecUtf8(actual);
        QFile original(baseline);
        check(original.open(QIODevice::ReadOnly) && original.readAll() == after
            && allValues(actual) == allValues(expected) && actual.status() == QSettings::NoError && expected.status() == QSettings::NoError,
            "complete key set, Qt values and INI bytes match the independent old GUI baseline");
    }
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks},
        {QStringLiteral("saved_key_count"), ini.allKeys().size()}, {QStringLiteral("cold"), cold},
        {QStringLiteral("nondefault"), nondefault}, {QStringLiteral("formal_mcp"), false}, {QStringLiteral("system_input"), false}};
}

QJsonObject QkmMappingAdditionTestAccess::windowMatchSave(QKeyMapper *mapper, bool cold)
{
    using namespace QKeyMapperConstants;
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    const QString variant = qEnvironmentVariable("QKM_LIFECYCLE_MATCH_VARIANT");
    const QStringList variants{QStringLiteral("metadata"), QStringLiteral("keep_path"), QStringLiteral("draft_fallback"),
        QStringLiteral("global"), QStringLiteral("missing_icon")};
    if (!variants.contains(variant) || !QDir::currentPath().contains(QStringLiteral("/out/"))
        || QDir::currentPath() != QCoreApplication::applicationDirPath()) return {{QStringLiteral("passed"), false}};
    const bool global = variant == QStringLiteral("global");
    const bool overwrite = variant == QStringLiteral("keep_path") || variant == QStringLiteral("missing_icon");
    const QString target = global ? QString::fromLatin1(GROUPNAME_GLOBALSETTING) : QStringLiteral("Map75Profile");
    const auto disk = [] { QFile file(QString::fromLatin1(CONFIG_FILENAME)); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray(); };
    const QByteArray before = disk();
    auto *name = mapper->findChild<QLineEdit *>(QStringLiteral("settingNameLineEdit"));
    auto *process = mapper->findChild<QLineEdit *>(QStringLiteral("processLineEdit"));
    auto *save = mapper->findChild<QPushButton *>(QStringLiteral("savemaplistButton"));
    auto *selector = mapper->findChild<QComboBox *>(QStringLiteral("settingselectComboBox"));
    if (!name || !process || !save || !selector) return {{QStringLiteral("passed"), false}};
    check(!before.isEmpty() && mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE
        && QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() == HOOKPROC_STATE_STOPPED,
        "window-match fixture uses an owned loaded idle product");
    const auto enterText = [](QLineEdit *editor, const QString &text) {
        editor->setFocus();
        QKeyEvent select(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier);
        QKeyEvent selected(QEvent::KeyRelease, Qt::Key_A, Qt::ControlModifier);
        QCoreApplication::sendEvent(editor, &select);
        QCoreApplication::sendEvent(editor, &selected);
        for (const QChar character : text) {
            QKeyEvent press(QEvent::KeyPress, 0, Qt::NoModifier, QString(character));
            QKeyEvent release(QEvent::KeyRelease, 0, Qt::NoModifier, QString(character));
            QCoreApplication::sendEvent(editor, &press);
            QCoreApplication::sendEvent(editor, &release);
        }
    };
    QSettings ini(QString::fromLatin1(CONFIG_FILENAME), QSettings::IniFormat);
    QKeyMapperQtCompat::setIniCodecUtf8(ini);
    const QString prefix = target + QStringLiteral("/");
    const QString oldPath = ini.value(prefix + PROCESSINFO_FILEPATH).toString();
    const QString oldIcon = ini.value(prefix + PROCESSINFO_CUSTOMICONPATH).toString();
    const QString incomingIcon = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("incoming75.png"));
    const QString keptIcon = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("kept75.png"));
    QVariantMap expected;
    const QList<QPair<const char *, const char *>> textFields{{PROCESSINFO_FILENAME, "processLineEdit"},
        {PROCESSINFO_WINDOWTITLE, "windowTitleLineEdit"}, {PROCESSINFO_CLASSNAME, "classNameLineEdit"}, {PROCESSINFO_DESCRIPTION, "descriptionLineEdit"}};
    const QList<QPair<const char *, const char *>> indexFields{{PROCESSINFO_FILENAME_MATCH_INDEX, "checkProcessComboBox"},
        {PROCESSINFO_WINDOWTITLE_MATCH_INDEX, "checkWindowTitleComboBox"}, {PROCESSINFO_CLASSNAME_MATCH_INDEX, "checkClassNameComboBox"},
        {PROCESSINFO_DISPLAYMODE_MATCH_INDEX, "checkDisplayModeComboBox"}};
    if (!cold) {
        if (variant == QStringLiteral("draft_fallback")) {
            QKeyEvent home(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier);
            QKeyEvent homeReleased(QEvent::KeyRelease, Qt::Key_Home, Qt::NoModifier);
            QCoreApplication::sendEvent(selector, &home);
            QCoreApplication::sendEvent(selector, &homeReleased);
            check(selector->currentIndex() == EMPTYSETTING_INDEX && mapper->m_MapProcessInfo.FilePath.isEmpty(),
                "GUI combo keyboard activation resets to a blank draft without a synthetic load call");
            enterText(process, QStringLiteral("  fixture75.exe  "));
        }
        if (variant == QStringLiteral("missing_icon")) {
            check(mapper->m_MapProcessInfo.CustomIconPath == incomingIcon && QFile::exists(incomingIcon)
                && QFile::remove(incomingIcon), "only the owned incoming icon disappears after normal startup loaded it");
        }
        if (!global) enterText(name, target);
    }
    for (const auto &field : textFields) {
        auto *editor = mapper->findChild<QLineEdit *>(QLatin1String(field.second));
        if (!editor) return {{QStringLiteral("passed"), false}};
        expected.insert(QString::fromLatin1(field.first), global ? ini.value(prefix + field.first) : QVariant(editor->text()));
    }
    for (const auto &field : indexFields) {
        auto *combo = mapper->findChild<QComboBox *>(QLatin1String(field.second));
        if (!combo) return {{QStringLiteral("passed"), false}};
        expected.insert(QString::fromLatin1(field.first), global ? ini.value(prefix + field.first) : QVariant(combo->currentIndex()));
    }
    OrderedMap<QString, QVariant> matchValues;
    if (!cold) {
        const bool dirty = mapper->m_SaveSettingDirty;
        const quint64 load = mapper->m_MappingLoadGeneration;
        const quint64 source = mapper->m_AutomationSourceEpoch;
        if (!global) matchValues = mapper->collectWindowMatchSaveValues(oldPath);
        const auto repeatedValues = global ? OrderedMap<QString, QVariant>() : mapper->collectWindowMatchSaveValues(oldPath);
        bool repeated = matchValues.keys() == repeatedValues.keys();
        for (auto it = matchValues.begin(); it != matchValues.end(); ++it) {
            repeated = repeated && it.value() == repeatedValues.value(it.key())
                && it.value().userType() == repeatedValues.value(it.key()).userType();
        }
        const int count = global ? 0 : (variant == QStringLiteral("missing_icon") ? 8
            : (variant == QStringLiteral("keep_path") ? 9 : 10));
        check(matchValues.size() == count && repeated,
            "window-match collection retains conditional key presence, order and Qt value types");
        check(disk() == before && mapper->m_SaveSettingDirty == dirty && mapper->m_MappingLoadGeneration == load
            && mapper->m_AutomationSourceEpoch == source,
            "window-match collection changes no disk, dirty, load generation or source epoch");
        int replies = 0;
        QTimer answer;
        QObject::connect(&answer, &QTimer::timeout, mapper, [&] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (box && box->button(QMessageBox::Yes)) { answer.stop(); ++replies; clickMappingButton(box->button(QMessageBox::Yes)); }
        });
        answer.start(10);
        clickMappingButton(save);
        answer.stop();
        check(replies == (overwrite ? 1 : 0), "existing-profile overwrite uses the real modal confirmation only when needed");
    }
    ini.sync();
    bool fieldsMatch = true;
    for (auto it = expected.cbegin(); it != expected.cend(); ++it) fieldsMatch = fieldsMatch && ini.value(prefix + it.key()) == it.value();
    check(fieldsMatch, "all window matching text and index fields follow the GUI or global no-write scope");
    check(ini.status() == QSettings::NoError && ini.value(SETTINGSELECT).toString() == target
        && mapper->m_LoadedMappingSettingGroup == target && !mapper->m_SaveSettingDirty,
        "GUI save or independent cold startup completes the exact target profile");
    const QString expectedPath = global ? oldPath : (overwrite ? QStringLiteral("C:/QkmProfile75/keep.exe")
        : (variant == QStringLiteral("draft_fallback") ? QStringLiteral("fixture75.exe") : QStringLiteral("C:/QkmProfile75/current.exe")));
    const QString expectedIcon = global ? oldIcon : (variant == QStringLiteral("metadata") ? incomingIcon
        : (variant == QStringLiteral("missing_icon") ? keptIcon : QString()));
    check(ini.value(prefix + PROCESSINFO_FILEPATH).toString() == expectedPath, "absolute path, preserved old path or trimmed draft fallback follows the original GUI rule");
    check(ini.contains(prefix + PROCESSINFO_CUSTOMICONPATH) && ini.value(prefix + PROCESSINFO_CUSTOMICONPATH).toString() == expectedIcon,
        "existing, empty and disappeared custom icon paths preserve their distinct write rules");
    if (!cold) {
        bool persisted = true;
        for (auto it = matchValues.begin(); it != matchValues.end(); ++it) {
            persisted = persisted && ini.contains(prefix + it.key()) && ini.value(prefix + it.key()) == it.value();
        }
        check(persisted, "ordinary GUI Save persists every collected window-match value");
    }
    if (cold) check(disk() == before, "cold metadata verification does not rewrite the INI");
    const QString baseline = qEnvironmentVariable("QKM_LIFECYCLE_SAVE_BASELINE");
    if (!baseline.isEmpty()) {
        QDir parent(QCoreApplication::applicationDirPath()); parent.cdUp();
        if (QFileInfo(baseline).absolutePath() != parent.absolutePath() || !QFileInfo(baseline).fileName().startsWith(QStringLiteral("baseline-"))) return {{QStringLiteral("passed"), false}};
        QFile original(baseline);
        check(original.open(QIODevice::ReadOnly) && original.readAll() == disk(), "complete metadata GUI save bytes match the independent old baseline");
    }
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks}, {QStringLiteral("variant"), variant},
        {QStringLiteral("saved_key_count"), ini.allKeys().size()}, {QStringLiteral("cold"), cold},
        {QStringLiteral("formal_mcp"), false}, {QStringLiteral("system_input"), false}};
}

QJsonObject QkmMappingAdditionTestAccess::commonSave(QKeyMapper *mapper, bool cold)
{
    using namespace QKeyMapperConstants;
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    const QString variant = qEnvironmentVariable("QKM_LIFECYCLE_COMMON_VARIANT");
    const bool detached = variant == QStringLiteral("detached");
    const bool empty = variant == QStringLiteral("empty");
    if ((!detached && !empty && variant != QStringLiteral("attached")) || !QDir::currentPath().contains(QStringLiteral("/out/"))
        || QDir::currentPath() != QCoreApplication::applicationDirPath()) return {{QStringLiteral("passed"), false}};
    const auto disk = [] { QFile file(QString::fromLatin1(CONFIG_FILENAME)); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray(); };
    const QByteArray before = disk();
    const QString target = QStringLiteral("Map76Common");
    check(!before.isEmpty() && mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE
        && QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() == HOOKPROC_STATE_STOPPED
        && mapper->m_LoadedMappingSettingGroup == (cold ? target : QStringLiteral("Common76Seed")),
        "common fixture is independently loaded by normal startup while idle");
    const auto key = [](QObject *object, int code, Qt::KeyboardModifiers modifiers = Qt::NoModifier, const QString &text = QString()) {
        QKeyEvent press(QEvent::KeyPress, code, modifiers, text);
        QKeyEvent release(QEvent::KeyRelease, code, modifiers, text);
        QCoreApplication::sendEvent(object, &press); QCoreApplication::sendEvent(object, &release);
    };
    QVariantList encoded;
    if (!cold) {
        int common = QKeyMapper::findCommonMappingTabIndex();
        if (common < 0) return {{QStringLiteral("passed"), false}, {QStringLiteral("checks"), checks}};
        const auto *source = QKeyMapper::s_KeyMappingTabInfoList.at(common).KeyMappingData;
        check(source && source->size() == 2 && source->first().Original_Key == QStringLiteral("F13")
            && source->last().Original_Key == QStringLiteral("F14") && mapper->m_CommonMappingData.size() == 1
            && mapper->m_CommonMappingData.first().Original_Key == QStringLiteral("F20"),
            "attached two-row source and deliberately stale standalone backup remain distinct before Save");
        if (detached) {
            auto *entry = mapper->findChild<QPushButton *>(QStringLiteral("mappingAdvancedSettingButton"));
            auto *enabled = mapper->m_MappingAdvancedDialog->findChild<QCheckBox *>(QStringLiteral("enableCommonMappingTableCheckBox"));
            if (!entry || !enabled || !enabled->isChecked()) return {{QStringLiteral("passed"), false}, {QStringLiteral("checks"), checks}};
            clickMappingButton(entry);
            check(mapper->m_MappingAdvancedDialog->isVisible(), "real mapping-advanced entry displays the common feature control");
            clickMappingButton(enabled);
            key(mapper->m_MappingAdvancedDialog, Qt::Key_Escape);
            check(!enabled->isChecked() && QKeyMapper::findCommonMappingTabIndex() < 0 && mapper->m_CommonMappingData.size() == 2,
                "GUI checkbox detaches the common source and retains its full backup");
        } else if (empty) {
            auto *tabs = mapper->findChild<QTabWidget *>(QStringLiteral("keyMappingTabWidget"));
            if (!tabs) return {{QStringLiteral("passed"), false}};
            auto *bar = tabs->tabBar();
            const QPoint local = bar->tabRect(tabs->indexOf(QKeyMapper::s_KeyMappingTabInfoList.at(common).KeyMappingDataTable)).center();
            QMouseEvent press(QEvent::MouseButtonPress, local, bar->mapTo(bar->window(), local), bar->mapToGlobal(local), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QMouseEvent release(QEvent::MouseButtonRelease, local, bar->mapTo(bar->window(), local), bar->mapToGlobal(local), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(bar, &press); QCoreApplication::sendEvent(bar, &release);
            check(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex == common && QKeyMapper::KeyMappingDataList->size() == 2,
                "Qt tab click selects the actual common source");
            key(mapper->m_KeyMappingDataTable, Qt::Key_A, Qt::ControlModifier);
            key(mapper->m_KeyMappingDataTable, Qt::Key_Delete);
            check(QKeyMapper::KeyMappingDataList->isEmpty(), "ordinary table selection and Delete remove all common rows");
        }
        auto *name = mapper->findChild<QLineEdit *>(QStringLiteral("settingNameLineEdit"));
        auto *save = mapper->findChild<QPushButton *>(QStringLiteral("savemaplistButton"));
        if (!name || !save || !save->isEnabled()) return {{QStringLiteral("passed"), false}};
        name->setFocus();
        key(name, Qt::Key_A, Qt::ControlModifier);
        for (const QChar character : target) key(name, 0, Qt::NoModifier, QString(character));
        common = QKeyMapper::findCommonMappingTabIndex();
        const auto data = common >= 0 ? *QKeyMapper::s_KeyMappingTabInfoList.at(common).KeyMappingData : mapper->m_CommonMappingData;
        const QByteArray beforeCollection = disk();
        const bool dirty = mapper->m_SaveSettingDirty;
        const quint64 load = mapper->m_MappingLoadGeneration, sourceEpoch = mapper->m_AutomationSourceEpoch;
        const auto backup = mapper->m_CommonMappingData;
        encoded = mapper->serializeCommonMappingData(data);
        const auto repeated = mapper->serializeCommonMappingData(data);
        bool typed = encoded == repeated && encoded.size() == data.size();
        for (int i = 0; i < encoded.size(); ++i) {
            const auto fields = encoded.at(i).toMap(), repeatedFields = repeated.at(i).toMap();
            typed = typed && fields.size() == 74;
            for (auto it = fields.cbegin(); it != fields.cend(); ++it) typed = typed && it.value().userType() == repeatedFields.value(it.key()).userType();
        }
        check(typed, "common serialization retains all 74 typed fields and repeated row order");
        check(disk() == beforeCollection && mapper->m_SaveSettingDirty == dirty && mapper->m_MappingLoadGeneration == load
            && mapper->m_AutomationSourceEpoch == sourceEpoch && mapper->m_CommonMappingData == backup,
            "pure common serialization leaves disk, dirty, generations and the pre-sync backup unchanged");
        clickMappingButton(save);
    }
    QSettings ini(QString::fromLatin1(CONFIG_FILENAME), QSettings::IniFormat);
    QKeyMapperQtCompat::setIniCodecUtf8(ini);
    const auto stored = ini.value(target + QLatin1Char('/') + COMMONMAPPINGDATA).toList();
    check(ini.status() == QSettings::NoError && ini.contains(target + QLatin1Char('/') + COMMONMAPPINGDATA)
        && stored.size() == (empty ? 0 : 2) && mapper->m_CommonMappingData.size() == stored.size(),
        "standalone backup is always written, including an empty list that clears stale data");
    check(mapper->m_LoadedMappingSettingGroup == target && !mapper->m_SaveSettingDirty
        && ini.value(SETTINGSELECT).toString() == target && (QKeyMapper::findCommonMappingTabIndex() < 0) == detached,
        "saved or cold-loaded profile retains the intended common attachment");
    if (!cold) check(stored == encoded, "ordinary GUI Save writes the exact full common payload, including empty lists");
    if (!empty && stored.size() == 2) {
        const auto first = stored.first().toMap(), second = stored.last().toMap();
        const auto &loaded = mapper->m_CommonMappingData.first();
        check(first.value(KEYMAPDATA_ORIGINALKEYS).toString() == QStringLiteral("F13")
            && second.value(KEYMAPDATA_ORIGINALKEYS).toString() == QStringLiteral("F14")
            && first.value(KEYMAPDATA_MAPPINGKEYS).toStringList() == QStringList{QStringLiteral("C"), QStringLiteral("D")}
            && first.value(KEYMAPDATA_MAPPINGKEYS_KEYUP).toStringList() == QStringList{QStringLiteral("E"), QStringLiteral("F")},
            "row and down/up sequence order survives common serialization and reload");
        check(loaded.MappingKeys_Comments == QStringList{QStringLiteral("中文😀\npress"), QString()}
            && loaded.MappingKeys_KeyUp_Comments == QStringList{QStringLiteral("release\\0"), QStringLiteral("日本語")}
            && first.value(KEYMAPDATA_MAPPINGKEYS_COMMENTS).toStringList() == loaded.MappingKeys_Comments
            && first.value(KEYMAPDATA_MAPPINGKEYS_KEYUP_COMMENTS).toStringList() == loaded.MappingKeys_KeyUp_Comments,
            "optional sequence comments retain Unicode, empty positions and escaped text");
        check(loaded.Note == QStringLiteral("Common note") && loaded.Category == QStringLiteral("分类")
            && loaded.FloatingButton_SyncGroupId == 17 && loaded.FloatingButton_Width == 93
            && loaded.Crosshair_CenterColor == QColor(QStringLiteral("#123456")) && loaded.Crosshair_X_Offset == -7,
            "independently seeded notes, category, sync group and typed styles survive");
    }
    if (cold) check(disk() == before, "independent common cold read does not rewrite the INI");
    const QString baseline = qEnvironmentVariable("QKM_LIFECYCLE_SAVE_BASELINE");
    if (!baseline.isEmpty()) {
        QDir parent(QCoreApplication::applicationDirPath()); parent.cdUp();
        if (QFileInfo(baseline).absolutePath() != parent.absolutePath() || !QFileInfo(baseline).fileName().startsWith(QStringLiteral("baseline-"))) return {{QStringLiteral("passed"), false}};
        QFile original(baseline);
        check(original.open(QIODevice::ReadOnly) && original.readAll() == disk(), "complete common save INI matches the independent old GUI baseline");
    }
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks}, {QStringLiteral("variant"), variant},
        {QStringLiteral("saved_key_count"), ini.allKeys().size()}, {QStringLiteral("cold"), cold},
        {QStringLiteral("formal_mcp"), false}, {QStringLiteral("system_input"), false}};
}

QJsonObject QkmMappingAdditionTestAccess::notificationSave(QKeyMapper *mapper, bool cold)
{
    using namespace QKeyMapperConstants;
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    const QString variant = qEnvironmentVariable("QKM_LIFECYCLE_NOTIFICATION_VARIANT");
    const bool enabled = variant == QStringLiteral("enabled"), loaded = variant == QStringLiteral("loaded"), overwrite = variant == QStringLiteral("target");
    if ((!enabled && !loaded && !overwrite && variant != QStringLiteral("absent")) || !QDir::currentPath().contains(QStringLiteral("/out/"))
        || QDir::currentPath() != QCoreApplication::applicationDirPath() || !mapper->m_CustomNotificationSetupDialog) return {{QStringLiteral("passed"), false}};
    const auto disk = [] { QFile file(QString::fromLatin1(CONFIG_FILENAME)); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray(); };
    const QByteArray before = disk();
    const QString target = QStringLiteral("Map77Notify"), prefix = target + QLatin1Char('/');
    const QList<const char *> styleKeys{CUSTOM_NOTIFICATION_FONTCOLOR, CUSTOM_NOTIFICATION_BACKGROUNDCOLOR, CUSTOM_NOTIFICATION_FONTFAMILY,
        CUSTOM_NOTIFICATION_FONTSIZE, CUSTOM_NOTIFICATION_FONTWEIGHT, CUSTOM_NOTIFICATION_FONTITALIC, CUSTOM_NOTIFICATION_DISPLAYDURATION,
        CUSTOM_NOTIFICATION_FADEINDURATION, CUSTOM_NOTIFICATION_FADEOUTDURATION, CUSTOM_NOTIFICATION_BORDERRADIUS,
        CUSTOM_NOTIFICATION_PADDING, CUSTOM_NOTIFICATION_OPACITY, CUSTOM_NOTIFICATION_X_OFFSET, CUSTOM_NOTIFICATION_Y_OFFSET};
    check(!before.isEmpty() && mapper->m_LoadedMappingSettingGroup == (cold ? target : QStringLiteral("Notify77Seed"))
        && mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE && QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() == HOOKPROC_STATE_STOPPED,
        "notification fixture is independently loaded by ordinary startup while idle");
    check(mapper->m_MappingAdvancedDialog->getCustomNotificationEnabled() == enabled
        && mapper->m_CustomNotificationStyleInitialized == (cold ? variant != QStringLiteral("absent") : loaded),
        "initialization and enabled state follow the independent current profile");
    OrderedMap<QString, QVariant> styleValues;
    const auto settings = mapper->m_CustomNotificationSetupDialog->getNotificationSettings();
    if (loaded) check(settings.fontColor == QColor(QStringLiteral("#123456")) && settings.backgroundColor == QColor(QStringLiteral("#80336699"))
        && settings.fontFamily == QStringLiteral("Arial") && settings.fontSize == 27 && settings.fontWeight == 2 && settings.fontItalic
        && settings.displayDuration == 1234 && settings.fadeInDuration == 111 && settings.fadeOutDuration == 222
        && settings.borderRadius == 13 && settings.padding == 9 && qFuzzyCompare(settings.opacity, 0.63)
        && settings.xOffset == -17 && settings.yOffset == 23,
        "all fourteen nondefault notification style fields restore from the independent INI");
    QSettings ini(QString::fromLatin1(CONFIG_FILENAME), QSettings::IniFormat);
    QKeyMapperQtCompat::setIniCodecUtf8(ini);
    if (!cold) {
        check(mapper->hasStoredCustomNotificationStyle(ini, prefix) == overwrite,
            "target stored-style dependency is distinct from the current profile initialization");
        auto *name = mapper->findChild<QLineEdit *>(QStringLiteral("settingNameLineEdit"));
        auto *save = mapper->findChild<QPushButton *>(QStringLiteral("savemaplistButton"));
        if (!name || !save || !save->isEnabled()) return {{QStringLiteral("passed"), false}};
        name->setFocus();
        QKeyEvent select(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier), selected(QEvent::KeyRelease, Qt::Key_A, Qt::ControlModifier);
        QCoreApplication::sendEvent(name, &select); QCoreApplication::sendEvent(name, &selected);
        for (const QChar character : target) {
            QKeyEvent press(QEvent::KeyPress, 0, Qt::NoModifier, QString(character)), release(QEvent::KeyRelease, 0, Qt::NoModifier, QString(character));
            QCoreApplication::sendEvent(name, &press); QCoreApplication::sendEvent(name, &release);
        }
        const bool dirty = mapper->m_SaveSettingDirty, initialized = mapper->m_CustomNotificationStyleInitialized;
        const quint64 load = mapper->m_MappingLoadGeneration, source = mapper->m_AutomationSourceEpoch;
        styleValues = mapper->collectCustomNotificationStyleSaveValues(ini, prefix);
        const auto repeated = mapper->collectCustomNotificationStyleSaveValues(ini, prefix);
        bool same = styleValues.keys() == repeated.keys();
        for (auto it = styleValues.begin(); it != styleValues.end(); ++it) {
            same = same && it.value() == repeated.value(it.key()) && it.value().userType() == repeated.value(it.key()).userType();
        }
        check(same && styleValues.size() == (variant == QStringLiteral("absent") ? 0 : 14),
            "notification collection preserves conditional ordered keys and all Qt value types");
        check(disk() == before && mapper->m_SaveSettingDirty == dirty && mapper->m_MappingLoadGeneration == load
            && mapper->m_AutomationSourceEpoch == source && mapper->m_CustomNotificationStyleInitialized == initialized
            && mapper->m_MappingAdvancedDialog->getCustomNotificationEnabled() == enabled,
            "notification collection changes no disk, dirty, generations, initialization or enabled state");
        int replies = 0;
        QTimer answer;
        QObject::connect(&answer, &QTimer::timeout, mapper, [&] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (box && box->button(QMessageBox::Yes)) { answer.stop(); ++replies; clickMappingButton(box->button(QMessageBox::Yes)); }
        });
        answer.start(10);
        clickMappingButton(save);
        answer.stop();
        check(replies == (overwrite ? 1 : 0), "only the existing target uses the actual overwrite confirmation");
    }
    ini.sync();
    int count = 0;
    for (const auto *key : styleKeys) count += ini.contains(prefix + key) ? 1 : 0;
    check(count == (variant == QStringLiteral("absent") ? 0 : 14),
        "style omission and all-fourteen-field writing retain the original conditional scope");
    if (!cold) {
        bool persisted = true;
        for (auto it = styleValues.begin(); it != styleValues.end(); ++it) persisted = persisted && ini.value(prefix + it.key()) == it.value();
        check(persisted, "GUI Save writes every collected notification style value");
    }
    check(ini.status() == QSettings::NoError && mapper->m_LoadedMappingSettingGroup == target && !mapper->m_SaveSettingDirty
        && ini.value(SETTINGSELECT).toString() == target && ini.value(prefix + CUSTOM_NOTIFICATION_ENABLED).toBool() == enabled
        && ini.value(prefix + CUSTOM_NOTIFICATION_POSITION).toInt() == 2,
        "GUI Save or cold load completes the exact target, enabled flag and notification position");
    if (count == 14) check(ini.value(prefix + CUSTOM_NOTIFICATION_FONTFAMILY).toString() == settings.fontFamily
        && ini.value(prefix + CUSTOM_NOTIFICATION_FONTSIZE).toInt() == settings.fontSize
        && ini.value(prefix + CUSTOM_NOTIFICATION_BACKGROUNDCOLOR).toString() == settings.backgroundColor.name(QColor::HexArgb)
        && qFuzzyCompare(ini.value(prefix + CUSTOM_NOTIFICATION_OPACITY).toDouble(), settings.opacity),
        "stored style uses the current effective values, not an overwritten target's stale style");
    if (cold) check(disk() == before, "independent notification cold read does not rewrite the INI");
    const QString baseline = qEnvironmentVariable("QKM_LIFECYCLE_SAVE_BASELINE");
    if (!baseline.isEmpty()) {
        QDir parent(QCoreApplication::applicationDirPath()); parent.cdUp();
        if (QFileInfo(baseline).absolutePath() != parent.absolutePath() || !QFileInfo(baseline).fileName().startsWith(QStringLiteral("baseline-"))) return {{QStringLiteral("passed"), false}};
        QFile original(baseline);
        check(original.open(QIODevice::ReadOnly) && original.readAll() == disk(), "complete notification INI matches the independent old GUI baseline");
    }
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks}, {QStringLiteral("variant"), variant},
        {QStringLiteral("saved_key_count"), ini.allKeys().size()}, {QStringLiteral("cold"), cold},
        {QStringLiteral("formal_mcp"), false}, {QStringLiteral("system_input"), false}};
}

QJsonObject QkmMappingAdditionTestAccess::tableState(QKeyMapper *mapper)
{
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    QFile file(QString::fromLatin1(CONFIG_FILENAME));
    if (!file.open(QIODevice::ReadOnly)) return {{QStringLiteral("passed"), false}};
    const auto before = file.readAll(); file.close();
    const bool attached = qEnvironmentVariableIntValue("QKM_LIFECYCLE_COMMON_ATTACHED") == 1;
    const int firstIndex = QKeyMapper::firstNormalMappingTabIndex();
    const int commonIndex = QKeyMapper::findCommonMappingTabIndex();
    check(firstIndex >= 0 && QKeyMapper::s_KeyMappingTabInfoList.size() == (attached ? 4 : 3)
        && (commonIndex >= 0) == attached && mapper->m_LoadedMappingSettingGroup == QStringLiteral("Map67General")
        && !mapper->m_SaveSettingDirty, "three normal tabs, common attachment, profile and clean state restore");
    if (firstIndex < 0 || (attached && commonIndex < 0)) return {{QStringLiteral("passed"), false}, {QStringLiteral("checks"), checks}};
    const auto &first = QKeyMapper::s_KeyMappingTabInfoList.at(firstIndex);
    check(first.TabHotkey == QStringLiteral("F17") && first.IncludeCommonMappingTable == attached
        && first.CommonMappingCollapsed == attached && first.TabFontColor == QColor(QStringLiteral("#234567"))
        && first.TabBackgroundColor == QColor(QStringLiteral("#80345678")), "tab metadata, include and collapsed rules retain their values");
    check(first.FloatingWindow_Position == QPoint(-27, 96) && first.FloatingWindow_Size == QSize(288, 144)
        && first.FloatingWindow_BackgroundColor == QColor(QStringLiteral("#a0112233"))
        && first.FloatingWindow_Opacity == 0.75 && first.FloatingWindow_Radius == 13
        && first.TabCustomImage_Padding == 7 && first.TabCustomImage_TrayIconPixel.isEmpty()
        && !first.TabCustomImage_ShowAsTrayIcon && !first.TabCustomImage_ShowAsFloatingWindow,
        "typed tab geometry and color restore; missing icon clears its unavailable pixel size");
    check(first.KeyMappingData->size() == 2 && first.KeyMappingData->first().Original_Key == QStringLiteral("A")
        && first.KeyMappingData->first().Mapping_Keys == QStringList{QStringLiteral("B")}, "first row and row ordering restore");
    if (first.KeyMappingData->size() == 2) {
        const auto &row = first.KeyMappingData->at(1);
        check(row.Original_Key == QStringLiteral("F13") && row.Mapping_Keys == QStringList{QStringLiteral("C"), QStringLiteral("D")}
            && row.MappingKeys_KeyUp == QStringList{QStringLiteral("E"), QStringLiteral("F")}
            && row.MappingKeys_Comments == QStringList{QStringLiteral("中文😀\npress"), QString()}
            && row.MappingKeys_KeyUp_Comments == QStringList{QStringLiteral("release\\0"), QStringLiteral("日本語")}
            && row.Note == QStringLiteral("note\n第二行") && row.Category == QStringLiteral("类别α"),
            "both sequences, Unicode comments, blank segments, note and category restore");
        check(row.FloatingButton_Label == QStringLiteral("labelα") && row.FloatingButton_ButtonColor == QColor(QStringLiteral("#123456"))
            && row.FloatingButton_BorderWidth == 3 && row.FloatingButton_Width == 180 && row.FloatingButton_Height == 48
            && row.FloatingButton_FontFamily == QStringLiteral("Segoe UI") && row.FloatingButton_NormalOpacity == 0.25
            && row.FloatingButton_PressedOpacity == 0.5 && row.FloatingButton_LockedOpacity == 0.75
            && row.FloatingButton_SyncGroupId == 23 && row.Crosshair_CenterSize == 7
            && row.Crosshair_CenterColor == QColor(QStringLiteral("#11aa55")) && !row.FloatingButton_Enable,
            "mapping visual parameters and sync group retain configuration without displaying a button");
    }
    bool second = false, empty = false;
    for (const auto &tab : std::as_const(QKeyMapper::s_KeyMappingTabInfoList)) {
        if (tab.TabName == QStringLiteral("Map67序列")) second = tab.KeyMappingData->size() == 1
            && tab.KeyMappingData->first().Original_Key == QStringLiteral("X") && tab.KeyMappingData->first().Disabled;
        if (tab.TabName == QStringLiteral("Map67空")) empty = tab.KeyMappingData->isEmpty();
    }
    check(second && empty, "non-current and empty tabs survive the saved table lists");
    const auto &common = attached ? *QKeyMapper::s_KeyMappingTabInfoList.at(commonIndex).KeyMappingData : mapper->m_CommonMappingData;
    check(common.size() == 1 && common.first().Original_Key == QStringLiteral("G")
        && common.first().Mapping_Keys == QStringList{QStringLiteral("H")}
        && common.first().MappingKeys_Comments == QStringList{QStringLiteral("common备注")}
        && common.first().FloatingButton_SyncGroupId == 17, "attached or detached common content retains comments and sync group");
    if (!file.open(QIODevice::ReadOnly)) return {{QStringLiteral("passed"), false}};
    check(file.readAll() == before, "inspection does not write the persisted profile");
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks},
        {QStringLiteral("formal_mcp"), false}, {QStringLiteral("system_input"), false}};
}

QJsonObject QkmMappingAdditionTestAccess::commonComments(QKeyMapper *mapper, const QString &mode)
{
    using namespace QKeyMapperConstants;
    bool passed = true;
    QJsonArray checks;
    const auto check = [&](bool value, const char *name) {
        checks.append(QJsonObject{{QStringLiteral("check"), QString::fromLatin1(name)}, {QStringLiteral("passed"), value}});
        passed = passed && value;
    };
    if (!QDir::currentPath().contains(QStringLiteral("/out/")) || QDir::currentPath() != QCoreApplication::applicationDirPath()) {
        return {{QStringLiteral("passed"), false}};
    }
    const bool cold = mode.endsWith(QStringLiteral("_cold")) || mode.endsWith(QStringLiteral("_legacy"));
    const bool legacy = mode.endsWith(QStringLiteral("_legacy"));
    const bool attached = mode.endsWith(QStringLiteral("_attached"));
    MAP_KEYDATA expected = mapper->createDefaultKeyMappingData(QStringLiteral("F13"), QStringLiteral("A»B"), false);
    expected.MappingKeys_KeyUp = QStringList{QStringLiteral("C"), QStringLiteral("D")};
    expected.Pure_MappingKeys_KeyUp = expected.MappingKeys_KeyUp;
    expected.MappingKeys_Comments = QStringList{QStringLiteral("中文😀\npress"), QString()};
    expected.MappingKeys_KeyUp_Comments = QStringList{QStringLiteral("release\\0"), QStringLiteral("日本語")};
    expected.Note = QStringLiteral("Common note");
    expected.FloatingButton_SyncGroupId = 17;
    if (!cold) {
        auto *name = mapper->findChild<QLineEdit *>(QStringLiteral("settingNameLineEdit"));
        auto *save = mapper->findChild<QPushButton *>(QStringLiteral("savemaplistButton"));
        if (!name || !save || mapper->m_KeyMapStatus != QKeyMapper::KEYMAP_IDLE || !save->isEnabled()) {
            return {{QStringLiteral("passed"), false}};
        }
        mapper->m_MappingAdvancedDialog->setCommonMappingTableEnabled(true);
        mapper->updateCommonMappingTabVisibility();
        const int common = QKeyMapper::findCommonMappingTabIndex();
        const int normal = QKeyMapper::firstNormalMappingTabIndex();
        if (common < 0 || normal < 0) return {{QStringLiteral("passed"), false}};
        *QKeyMapper::s_KeyMappingTabInfoList.at(common).KeyMappingData = QList<MAP_KEYDATA>{expected};
        *QKeyMapper::s_KeyMappingTabInfoList.at(normal).KeyMappingData = QList<MAP_KEYDATA>{
            mapper->createDefaultKeyMappingData(QStringLiteral("G"), QStringLiteral("H"), false)};
        mapper->forceSwitchKeyMappingTabWidgetIndex(normal);
        mapper->refreshTabsForSourceTabChange(normal);
        if (!attached) {
            mapper->m_MappingAdvancedDialog->setCommonMappingTableEnabled(false);
            mapper->updateCommonMappingTabVisibility();
        }
        name->setText(QStringLiteral("Common71"));
        mapper->markSaveSettingDirty();
        clickMappingButton(save);
    }
    check(mapper->m_LoadedMappingSettingGroup == QStringLiteral("Common71") && !mapper->m_SaveSettingDirty
        && mapper->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE && QKeyMapper_Worker::s_AtomicHookProcState.loadAcquire() == HOOKPROC_STATE_STOPPED,
        "GUI save or cold load completes while mapping remains stopped");
    check((QKeyMapper::findCommonMappingTabIndex() >= 0) == attached, "common source attachment matches the selected fixture");
    if (mapper->m_CommonMappingData.size() != 1) return {{QStringLiteral("passed"), false}, {QStringLiteral("checks"), checks}};
    const auto actual = mapper->m_CommonMappingData.first();
    QSettings ini(QString::fromLatin1(CONFIG_FILENAME), QSettings::IniFormat);
    QKeyMapperQtCompat::setIniCodecUtf8(ini);
    const auto stored = ini.value(QStringLiteral("Common71/") + COMMONMAPPINGDATA).toList();
    if (stored.size() != 1) return {{QStringLiteral("passed"), false}, {QStringLiteral("checks"), checks}};
    const auto fields = stored.first().toMap();
    if (legacy) { expected.MappingKeys_Comments.clear(); expected.MappingKeys_KeyUp_Comments.clear(); }
    check(actual.MappingKeys_Comments == expected.MappingKeys_Comments
        && fields.value(KEYMAPDATA_MAPPINGKEYS_COMMENTS).toStringList() == expected.MappingKeys_Comments,
        "common down-sequence comments survive standalone saving and loading");
    check(actual.MappingKeys_KeyUp_Comments == expected.MappingKeys_KeyUp_Comments
        && fields.value(KEYMAPDATA_MAPPINGKEYS_KEYUP_COMMENTS).toStringList() == expected.MappingKeys_KeyUp_Comments,
        "common up-sequence comments survive standalone saving and loading");
    auto otherFields = actual;
    otherFields.MappingKeys_Comments = expected.MappingKeys_Comments;
    otherFields.MappingKeys_KeyUp_Comments = expected.MappingKeys_KeyUp_Comments;
    check(otherFields == expected, "all other existing common configuration fields retain their values");
    if (attached) {
        const auto *source = QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::findCommonMappingTabIndex()).KeyMappingData;
        check(source && source->size() == 1 && source->first() == expected, "attached common source retains the same configuration");
    }
    if (legacy) check(!fields.contains(KEYMAPDATA_MAPPINGKEYS_COMMENTS) && !fields.contains(KEYMAPDATA_MAPPINGKEYS_KEYUP_COMMENTS),
        "old files without optional comment fields keep empty comments");
    return {{QStringLiteral("passed"), passed}, {QStringLiteral("checks"), checks}, {QStringLiteral("cold"), cold},
        {QStringLiteral("formal_mcp"), false}, {QStringLiteral("system_input"), false}};
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
        QStringLiteral("gui_stop_exit"), QStringLiteral("closed_input_tracking"),
        QStringLiteral("gui_stop_repeat"), QStringLiteral("gui_stop_after_timeout"),
        QStringLiteral("gui_stop_restore"), QStringLiteral("gui_stop_restore_timeout"), QStringLiteral("runtime_revision"),
        QStringLiteral("hold_source_context"), QStringLiteral("unlock_source_context"), QStringLiteral("inactive_virtual_release"),
        QStringLiteral("mapping_add_plain"), QStringLiteral("mapping_add_conflicts"), QStringLiteral("mapping_add_guards"),
        QStringLiteral("mapping_add_stale"), QStringLiteral("mapping_add_setup"), QStringLiteral("mapping_config_equality"),
        QStringLiteral("settings_serialization"), QStringLiteral("settings_serialization_cold"), QStringLiteral("settings_general_save"),
        QStringLiteral("common_comments"), QStringLiteral("common_comments_cold"), QStringLiteral("common_comments_legacy"),
        QStringLiteral("common_comments_attached"), QStringLiteral("settings_table_save"), QStringLiteral("settings_table_cold"),
        QStringLiteral("ui_localization"), QStringLiteral("settings_profile_save"), QStringLiteral("settings_profile_cold"),
        QStringLiteral("settings_match_save"), QStringLiteral("settings_match_cold"),
        QStringLiteral("settings_common_save"), QStringLiteral("settings_common_cold"),
        QStringLiteral("settings_notification_save"), QStringLiteral("settings_notification_cold")};
    bool knownCase = cases.contains(mode);
    for (const auto &spec : repeatSpecs()) { knownCase = knownCase || mode == spec.name; }
    if (!knownCase || path.isEmpty()) { return 2; }
    wchar_t desktop[256]{};
    const bool desktopObserved = GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()),
        UOI_NAME, desktop, sizeof(desktop), nullptr);
    result = new LifecycleResult;
    if ((mode == QStringLiteral("gui_stop_restore") || mode == QStringLiteral("gui_stop_restore_timeout"))
        && !prepareGuiStopReturnFixture()) { return 2; }
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
        {QStringLiteral("sourceRevisionTrace"), QJsonArray::fromStringList(result->sourceRevisionTrace)},
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
        {QStringLiteral("guiStopLastOperation"), QString::number(result->guiStopLastOperation)},
        {QStringLiteral("guiStopLastFence"), QString::number(result->guiStopLastFence)},
        {QStringLiteral("guiStopStaleIgnored"), result->guiStopStaleIgnored},
        {QStringLiteral("guiStopReturnCaptured"), result->guiStopReturnCaptured},
        {QStringLiteral("guiStopReturnLoaded"), result->guiStopReturnLoaded},
        {QStringLiteral("guiStopBusySourceChecks"), result->guiStopBusySourceChecks},
        {QStringLiteral("drainResourcesIntact"), result->drainResourcesIntact},
        {QStringLiteral("drainFinalPoolsFinished"), result->drainFinalPoolsFinished},
        {QStringLiteral("buttonLockAfterDispatch"), result->buttonLockAfterDispatch},
        {QStringLiteral("lifecycleDispatchTrace"), QJsonArray::fromStringList(result->lifecycleDispatchTrace)},
        {QStringLiteral("sourceContextTrace"), QJsonArray::fromStringList(result->sourceContextTrace)},
        {QStringLiteral("virtualReleaseTrace"), QJsonArray::fromStringList(result->virtualReleaseTrace)},
        {QStringLiteral("mappingAddition"), result->mappingAdditionEvidence},
        {QStringLiteral("mappingEquality"), result->mappingEqualityEvidence},
        {QStringLiteral("settingsSerialization"), result->settingsSerializationEvidence},
        {QStringLiteral("settingsGeneral"), result->settingsGeneralEvidence},
        {QStringLiteral("commonComments"), result->commonCommentsEvidence},
        {QStringLiteral("uiLocalization"), result->uiLocalizationEvidence},
        {QStringLiteral("repeatRounds"), result->repeatRounds.loadAcquire()},
        {QStringLiteral("repeatRuns"), result->repeatRuns},
        {QStringLiteral("repeatTrace"), QJsonArray::fromStringList(result->repeatTrace)}};
    delete result;
    result = nullptr;
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(QJsonDocument(evidence).toJson()) < 0) { return 2; }
    return passed ? 0 : 2;
}
