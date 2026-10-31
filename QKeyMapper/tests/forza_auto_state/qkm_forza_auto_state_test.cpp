#include <QCoreApplication>
#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QSemaphore>
#include <QtEndian>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <io.h>
#include <thread>

#define QKM_FORZA_AUTO_STATE_TESTING
#include "../../qkeymapper_worker.cpp"

namespace {
using Worker = QKeyMapper_Worker;
QSemaphore readEntered;
QSemaphore continueRead;
std::atomic<bool> pauseAccelRead{false};
std::atomic<bool> clearAccelRead{false};
std::atomic<bool> verifySendUnlocked{false};
std::atomic<int> unlockedSends{0};
std::atomic<int> outputCalls{0};
XUSB_REPORT lastReport{};
int clientStorage = 0;
int targetStorage = 0;
const auto testClient = reinterpret_cast<PVIGEM_CLIENT>(&clientStorage);
const auto testTarget = reinterpret_cast<PVIGEM_TARGET>(&targetStorage);
std::atomic<int> checks{0};

void require(bool passed, const char *message)
{
    if (!passed) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::fflush(stderr);
        std::_Exit(2);
    }
    ++checks;
}

double forzaTestGlobalThreshold(bool isBrake)
{
    return isBrake ? 1.05 : 10.0;
}

void forzaTestThresholdReadCheckpoint(bool isBrake)
{
    if (!isBrake && clearAccelRead.exchange(false)) {
        forzaAutoSpecMap(0, false)->clear();
    }
    if (!isBrake && pauseAccelRead.exchange(false)) {
        readEntered.release();
        require(continueRead.tryAcquire(1, 5000), "reader checkpoint timed out");
    }
}

void forzaTestBeforeAutoAdjustSend()
{
    // No competing writer runs in this case, so failure means same-thread reentry.
    if (verifySendUnlocked.load()) {
        require(Worker::s_ViGEmClient_Mutex.tryLock(), "UDP send retained the nonrecursive mutex");
        Worker::s_ViGEmClient_Mutex.unlock();
        ++unlockedSends;
    }
}

QByteArray telemetry(quint32 timestamp, float slip = 0.0f)
{
    QByteArray packet(FORZA_HORIZON_DATA_LENGTH, '\0');
    qToLittleEndian<quint32>(timestamp, packet.data() + 4);
    quint32 bits;
    std::memcpy(&bits, &slip, sizeof(bits));
    for (int offset : {84, 88, 92, 96, 180, 184, 188, 192}) {
        qToLittleEndian<quint32>(bits, packet.data() + offset);
    }
    qToLittleEndian<quint32>(123, packet.data() + 212);
    return packet;
}

void press(const QString &key, const QString &token)
{
    Worker::ViGEmClient_PressButton(key, AUTO_ADJUST_NONE, 0, INITIAL_PLAYER_INDEX, token);
}

void release(const QString &key, const QString &token)
{
    Worker::ViGEmClient_ReleaseButton(key, 0, token);
}

void checkThreshold(bool brake, double expected, bool active)
{
    QMutexLocker locker(&Worker::s_ViGEmClient_Mutex);
    require(currentForzaAutoThreshold(0, brake) == expected, "active threshold changed");
    require(hasForzaAutoGroupActive(0, brake) == active, "active group state changed");
}

void lastReleaseInterleave(Worker &worker)
{
    const QString key = VJOY_RT_ACCEL_STR + QStringLiteral("[INIT=190,THR=3.5]");
    press(key, QStringLiteral("W"));
    pauseAccelRead = true;
    std::thread reader([&] { worker.processForzaFormatData(telemetry(100)); });
    require(readEntered.tryAcquire(1, 5000), "UDP threshold read was not reached");
    const bool readLocked = !Worker::s_ViGEmClient_Mutex.tryLock();
    if (!readLocked) { Worker::s_ViGEmClient_Mutex.unlock(); }
    QSemaphore writerStarted;
    QSemaphore writerDone;
    std::thread writer([&] {
        writerStarted.release();
        release(key, QStringLiteral("W"));
        writerDone.release();
    });
    require(writerStarted.tryAcquire(1, 5000), "release writer did not start");
    const bool releasedDuringRead = writerDone.tryAcquire(1, 100);
    continueRead.release();
    reader.join();
    writer.join();
    require(readLocked && !releasedDuringRead, "last release overlapped the UDP threshold read");
    checkThreshold(false, 10.0, false);
    require(lastReport.bRightTrigger == 0, "late UDP update re-pressed a released trigger");
    std::puts("PASS last_release_interleave");
}

void thresholdStack(Worker &worker)
{
    const QString legacy = VJOY_RT_ACCEL_STR;
    const QString custom = legacy + QStringLiteral("[INIT=50,THR=3.5]");
    press(custom, QStringLiteral("W"));
    require(lastReport.bRightTrigger == 50, "custom INIT was not applied");
    press(legacy, QStringLiteral("Up"));
    checkThreshold(false, 10.0, true);
    require(lastReport.bRightTrigger == 50, "second press reset the active INIT");
    release(legacy, QStringLiteral("Up"));
    checkThreshold(false, 3.5, true);
    worker.processForzaFormatData(telemetry(116, 20.0f));
    require(Worker::s_Auto_Accel < 50, "telemetry did not adjust the active accel");
    release(custom, QStringLiteral("W"));
    checkThreshold(false, 10.0, false);
    require(lastReport.bRightTrigger == 0, "final release left the accel pressed");
    require(Worker::s_Auto_Accel == AUTO_ACCEL_DEFAULT, "final release did not reset accel state");

    const QString brake = VJOY_LT_BRAKE_STR + QStringLiteral("[INIT=190,THR=1.2]");
    press(brake, QStringLiteral("H"));
    press(custom, QStringLiteral("W"));
    checkThreshold(true, 1.2, true);
    checkThreshold(false, 3.5, true);
    worker.processForzaFormatData(telemetry(132, 20.0f));
    worker.processForzaFormatData(telemetry(148, 20.0f));
    require(Worker::s_Auto_Brake < 190, "confirmed brake slip did not adjust brake");
    release(brake, QStringLiteral("H"));
    release(custom, QStringLiteral("W"));
    require(lastReport.bLeftTrigger == 0 && lastReport.bRightTrigger == 0, "release did not neutralize both triggers");
    std::puts("PASS threshold_stack_and_both_triggers");
}

void emptyKeysAndIndices()
{
    const QString key = VJOY_RT_ACCEL_STR + QStringLiteral("[INIT=50,THR=3.5]");
    press(key, QStringLiteral("W"));
    {
        QMutexLocker locker(&Worker::s_ViGEmClient_Mutex);
        clearAccelRead = true;
        require(currentForzaAutoThreshold(0, false) == 10.0, "empty key snapshot did not use the global threshold");
    }
    release(key, QStringLiteral("W"));
    const int before = outputCalls.load();
    for (int index : {-1, 1}) {
        Worker::ViGEmClient_PressButton(key, AUTO_ADJUST_NONE, index, INITIAL_PLAYER_INDEX);
        Worker::ViGEmClient_ReleaseButton(key, index);
    }
    require(outputCalls.load() == before, "invalid gamepad index reached the output boundary");
    std::puts("PASS empty_keys_fallback_and_invalid_indices");
}

void concurrentTransitions(Worker &worker)
{
    const QString accel = VJOY_RT_ACCEL_STR + QStringLiteral("[INIT=190,THR=3.5]");
    const QString brake = VJOY_LT_BRAKE_STR + QStringLiteral("[INIT=190,THR=1.2]");
    std::thread reader([&] {
        for (quint32 i = 0; i < 6000; ++i) {
            worker.processForzaFormatData(telemetry(200 + i * 16, (i % 2) ? 20.0f : 0.0f));
        }
    });
    std::thread writer([&] {
        for (int i = 0; i < 3000; ++i) {
            press(accel, QStringLiteral("W"));
            press(brake, QStringLiteral("H"));
            release(accel, QStringLiteral("W"));
            release(brake, QStringLiteral("H"));
            if (i % 50 == 0) {
                QMutexLocker locker(&Worker::s_ViGEmClient_Mutex);
                resetForzaAutoSpecLists();
            }
        }
    });
    reader.join();
    writer.join();
    checkThreshold(false, 10.0, false);
    checkThreshold(true, 1.05, false);
    require(lastReport.bLeftTrigger == 0 && lastReport.bRightTrigger == 0, "concurrent release left a trigger pressed");
    press(accel, QStringLiteral("W"));
    checkThreshold(false, 3.5, true);
    release(accel, QStringLiteral("W"));
    std::puts("PASS concurrent_press_release_reset_and_reuse");
}

void lifecycleTransitions(int argc, char *argv[])
{
    wchar_t desktop[128]{};
    DWORD bytes = 0;
    require(GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME,
        desktop, sizeof(desktop), &bytes), "cannot identify the actual desktop");
    require(QString::fromWCharArray(desktop).startsWith(QStringLiteral("QkmForza_")),
        "lifecycle checks require an owned private desktop");
    QApplication app(argc, argv);
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSettings settings(QString::fromLatin1(CONFIG_FILENAME), QSettings::IniFormat);
    require(settings.value(QStringLiteral("ForzaLifecycleFixture")).toBool()
        && !settings.value(QString::fromLatin1(STARTUP_AUTOMONITORING), true).toBool()
        && !settings.value(QString::fromLatin1(VIRTUALGAMEPAD_ENABLE), true).toBool(),
        "an isolated, non-starting fixture INI is required");
    auto *worker = Worker::getInstance();
    worker->threadStarted();
    {
        QKeyMapper mapper;
        require(Worker::s_ViGEmTargetList.isEmpty(), "fixture unexpectedly created a native target");
        auto *tabs = mapper.findChild<KeyMappingTabWidget *>(QStringLiteral("keyMappingTabWidget"));
        require(tabs && !QKeyMapper::s_KeyMappingTabInfoList.isEmpty(), "real Tab UI was not initialized");
        require(mapper.addTabToKeyMappingTabWidget(QStringLiteral("ForzaLifecycleB"), false), "cannot add the second Tab");
        const int second = QKeyMapper::s_KeyMappingTabInfoList.size() - 1;
        for (int index : {0, second}) {
            auto *data = QKeyMapper::s_KeyMappingTabInfoList[index].KeyMappingData;
            require(data != nullptr, "Tab has no source list");
            data->clear();
            for (bool brake : {false, true}) {
                MAP_KEYDATA row;
                row.Original_Key = QStringLiteral("QkmForza%1%2").arg(index).arg(brake ? 'B' : 'A');
                row.Pure_OriginalKeys = QStringList{row.Original_Key};
                row.Mapping_Keys = QStringList{brake ? VJOY_LT_BRAKE_STR : VJOY_RT_ACCEL_STR};
                row.Pure_MappingKeys = row.Mapping_Keys;
                row.MappingKeys_KeyUp = row.Mapping_Keys;
                row.Pure_MappingKeys_KeyUp = row.Mapping_Keys;
                data->append(row);
            }
        }
        const auto nativeClient = Worker::s_ViGEmClient;
        const auto nativeState = Worker::s_ViGEmClient_ConnectState;
        const auto nativeFaker = Worker::s_FakerInputClient;
        Worker::s_FakerInputClient = Q_NULLPTR;
        Worker::s_ViGEmClient = testClient;
        Worker::s_ViGEmClient_ConnectState = Worker::VIGEMCLIENT_CONNECT_SUCCESS;
        Worker::s_ViGEmTargetList.append(testTarget);
        Worker::s_ViGEmTarget_ReportList.append(ViGEm_ReportData());
        const auto cleared = [] {
            QMutexLocker locker(&Worker::s_ViGEmClient_Mutex);
            require(!hasForzaAutoGroupActive(0, false) && !hasForzaAutoGroupActive(0, true), "lifecycle retained Forza groups");
            require(Worker::s_Auto_Accel == AUTO_ACCEL_DEFAULT && Worker::s_Auto_Brake == AUTO_BRAKE_DEFAULT,
                "lifecycle retained automatic levels");
            require(!Worker::s_HasLastForzaTimestamp && Worker::s_LastForzaTimestampMs == 0,
                "lifecycle retained telemetry timing");
            require(lastReport.bLeftTrigger == 0 && lastReport.bRightTrigger == 0, "lifecycle left a trigger pressed");
        };
        const auto activeTab = [](int index) {
            require(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex == index, "selected Tab identity changed");
            require(QKeyMapper::KeyMappingDataList == &QKeyMapper::s_ActiveKeyMappingDataList,
                "worker did not select the active list");
            require(QKeyMapper::s_ActiveKeyMappingDataList.size() == 2
                && QKeyMapper::s_ActiveKeyMappingDataList.first().Original_Key == QStringLiteral("QkmForza%1A").arg(index),
                "worker used another Tab's rows");
        };
        const auto reuse = [worker](int index) {
            const QString accel = VJOY_RT_ACCEL_STR + (index == 0 ? QStringLiteral("[INIT=190,THR=3.5]") : QStringLiteral("[INIT=110,THR=6.25]"));
            const QString brake = VJOY_LT_BRAKE_STR + QStringLiteral("[INIT=190,THR=1.2]");
            press(accel, QStringLiteral("QkmForzaAccel"));
            press(brake, QStringLiteral("QkmForzaBrake"));
            checkThreshold(false, index == 0 ? 3.5 : 6.25, true);
            checkThreshold(true, 1.2, true);
            worker->processForzaFormatData(telemetry(4321, 20.0f));
            require(Worker::s_HasLastForzaTimestamp, "lifecycle did not process telemetry");
            release(accel, QStringLiteral("QkmForzaAccel"));
            release(brake, QStringLiteral("QkmForzaBrake"));
            require(lastReport.bLeftTrigger == 0 && lastReport.bRightTrigger == 0, "reused state did not release");
            press(accel, QStringLiteral("QkmForzaAccel"));
            press(brake, QStringLiteral("QkmForzaBrake"));
        };
        tabs->setCurrentIndex(0);
        worker->setWorkerKeyHook();
        activeTab(0);
        cleared();
        reuse(0);
        worker->setWorkerKeyUnHook();
        require(Worker::s_AtomicHookProcState.loadAcquire() == HOOKPROC_STATE_STOPPED, "stop did not finish");
        cleared();
        require(QKeyMapper::KeyMappingDataList == QKeyMapper::s_KeyMappingTabInfoList[0].KeyMappingData,
            "stop did not restore the editable source");
        worker->setWorkerKeyHook();
        activeTab(0);
        cleared();
        reuse(0);
        worker->setKeyMappingRestart();
        activeTab(0);
        cleared();
        reuse(0);
        tabs->setCurrentIndex(second);
        worker->setKeyMappingRestart();
        activeTab(second);
        cleared();
        require(QKeyMapper::s_LastActiveKeyMappingDataList.first().Original_Key == QStringLiteral("QkmForza0A"),
            "Tab restart lost its previous source");
        reuse(second);
        worker->setWorkerKeyUnHook();
        cleared();
        require(SendInputTask::s_SendInputTaskControllerMap.isEmpty(), "lifecycle left input controllers");
        Worker::s_ViGEmTargetList.clear();
        Worker::s_ViGEmTarget_ReportList.clear();
        Worker::s_ViGEmClient = nativeClient;
        Worker::s_ViGEmClient_ConnectState = nativeState;
        Worker::s_FakerInputClient = nativeFaker;
        require(QMetaObject::invokeMethod(worker, "shutdownSendInputTasks", Qt::DirectConnection), "worker shutdown failed");
        std::printf("PASS lifecycle_stop_start_restart_tab_switch desktop=%ls HWND=%llu\n", desktop,
            static_cast<unsigned long long>(mapper.winId()));
    }
    worker->threadFinished();
}

}

// Replace only report output; the test target never connects to the bus.
extern "C" BOOL vigem_target_is_attached(PVIGEM_TARGET target)
{
    return target == testTarget;
}

extern "C" VIGEM_TARGET_TYPE vigem_target_get_type(PVIGEM_TARGET target)
{
    require(target == testTarget, "unexpected virtual target");
    return Xbox360Wired;
}

extern "C" VIGEM_ERROR vigem_target_x360_update(PVIGEM_CLIENT client, PVIGEM_TARGET target, XUSB_REPORT report)
{
    require(client == testClient && target == testTarget, "unexpected output target");
    ++outputCalls;
    lastReport = report;
    return VIGEM_ERROR_NONE;
}

int main(int argc, char *argv[])
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    const QByteArray logPath = qgetenv("QKM_FORZA_TEST_LOG");
    if (!logPath.isEmpty()) {
        FILE *log = nullptr;
        if (freopen_s(&log, logPath.constData(), "w", stdout) != 0
            || _dup2(_fileno(stdout), _fileno(stderr)) < 0) { return 4; }
    }
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    QSemaphore finished;
    std::thread watchdog([&] {
        if (!finished.tryAcquire(1, 30000)) { std::_Exit(3); }
    });
    if (qEnvironmentVariableIsSet("QKM_FORZA_TEST_LIFECYCLE")) {
        lifecycleTransitions(argc, argv);
    } else {
        QCoreApplication app(argc, argv);
        Worker worker;
        Worker::s_ViGEmClient = testClient;
        Worker::s_ViGEmClient_ConnectState = Worker::VIGEMCLIENT_CONNECT_SUCCESS;
        Worker::s_ViGEmTargetList.append(testTarget);
        Worker::s_ViGEmTarget_ReportList.append(ViGEm_ReportData());
        lastReleaseInterleave(worker);
        verifySendUnlocked = true;
        thresholdStack(worker);
        verifySendUnlocked = false;
        require(unlockedSends.load() > 0, "send lock boundary was not exercised");
        std::puts("PASS same_mutex_reentry_boundary");
        emptyKeysAndIndices();
        concurrentTransitions(worker);
        Worker::s_ViGEmTargetList.clear();
        Worker::s_ViGEmTarget_ReportList.clear();
        Worker::s_ViGEmClient = Q_NULLPTR;
        Worker::s_ViGEmClient_ConnectState = Worker::VIGEMCLIENT_DISCONNECTED;
        require(QMetaObject::invokeMethod(&worker, "shutdownSendInputTasks", Qt::DirectConnection), "worker shutdown failed");
    }
    finished.release();
    watchdog.join();
    std::printf("PASS %d checks\n", checks.load());
    return 0;
}
