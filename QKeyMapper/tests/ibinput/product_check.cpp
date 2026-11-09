#include <QCheckBox>
#include <QLabel>
#include <QMenu>
#include <QSignalBlocker>
#include <QToolButton>
#include "../../qmappingadvanceddialog.h"
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QSettings>
#include <QSaveFile>
#include <QSemaphore>
#include <QSharedPointer>
#include <atomic>
#include <Windows.h>
#include <tlhelp32.h>
#include <QTimer>
#include <stdexcept>
#include "../../qkm_ibinput_client.h"
#include "../../qkm_ibinput_popup.h"
#include "../../qkm_ibinput_validation.h"

// Keep the real entry point, startup and orderly shutdown in this maintenance target.
#define main qkmIbProductMain
#include "../../main.cpp"
#undef main

namespace {
QString output;
QString selectedCase;
QString expectedDesktop;
QString expectedBackend;
bool checked=false;
int polls=0;
void check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
QString currentDesktop() {
    wchar_t name[128]{}; DWORD bytes=0;
    return GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()),UOI_NAME,name,sizeof(name),&bytes)
        ? QString::fromWCharArray(name) : QString();
}
void record(const QJsonObject &value) {
    QFile file(output+QStringLiteral("/result.json"));
    check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"Cannot write product result");
    check(file.write(QJsonDocument(value).toJson())>0,"Cannot persist product result");
}

int rejectedSaves=0;
QByteArray savedIni() {
    QFile file(QCoreApplication::applicationDirPath()+QStringLiteral("/keymapdata.ini"));
    check(file.open(QIODevice::ReadOnly),"Cannot read isolated product INI");
    return file.readAll();
}
void queuedFaultCase(QKeyMapper *window) {
    auto &client=QkmIbInputClient::instance();
    QPopupMessageLabel *message=nullptr;
    for (QWidget *widget:QApplication::topLevelWidgets())
        if (auto *candidate=qobject_cast<QPopupMessageLabel *>(widget)) { message=candidate; break; }
    check(message && !client.busy(),"Queued fault fixture is not idle");
    const auto old=client.requestRun();
    const auto latest=client.requestRun();
    const QString stale=QStringLiteral("Owned stale IbInput fault");
    emit QKeyMapper_Worker::getInstance()->ibInputFault_Signal(stale,old);
    QCoreApplication::sendPostedEvents(window,QEvent::MetaCall);
    check(message->text()!=stale && client.requestedGeneration()==latest && client.acceptingInput()
        && window->m_KeyMapStatus==QKeyMapper::KEYMAP_IDLE,"Stale queued fault reached the newer UI intent");
    const QString current=QStringLiteral("Owned current IbInput fault");
    emit QKeyMapper_Worker::getInstance()->ibInputFault_Signal(current,latest);
    QCoreApplication::sendPostedEvents(window,QEvent::MetaCall);
    check(message->text()==current,"Current queued fault was incorrectly suppressed");
    client.cancelRun(latest);
    message->hide(); message->clear();
}
void saveGuardCases(QKeyMapper *window) {
    const QString normalName=QKeyMapper::s_KeyMappingTabInfoList.at(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex).TabName;
    const auto normal=[&]() -> KeyMappingTab_Info & {
        for (auto &tab:QKeyMapper::s_KeyMappingTabInfoList)
            if (!QKeyMapper::isCommonMappingTab(tab) && tab.TabName==normalName) return tab;
        throw std::runtime_error("Original normal table is missing");
    };
    const QList<MAP_KEYDATA> originalRows=*normal().KeyMappingData;
    const bool originalInclude=normal().IncludeCommonMappingTable;
    check(window->saveKeyMapSetting(false),"Clean baseline save failed");
    auto *start=window->findChild<QToolButton *>(QStringLiteral("keymapButton"));
    check(start && start->menu() && start->menu()->actions().size()==2,"Mapping start actions are unavailable");
    const auto actions=start->menu()->actions();
    QAction *originalAction=actions.at(0)->isChecked() ? actions.at(0) : actions.at(1);
    actions.at(1)->trigger();
    const QStringList arrows{QStringLiteral("Up"),QStringLiteral("Down"),QStringLiteral("Left"),QStringLiteral("Right")};
    for (int index=0;index<4;++index) {
        auto row=originalRows.at(index); row.Original_Key=arrows.at(index); row.Pure_OriginalKeys=QStringList{arrows.at(index)};
        row.SendMappingKeyMethod=QKeyMapperConstants::SENDMAPPINGKEY_METHOD_SENDINPUT;
        normal().KeyMappingData->append(row);
    }
    QByteArray before=savedIni();
    const quint64 generation=QkmIbInputClient::instance().requestedGeneration();
    start->click();
    check(savedIni()==before && QKeyMapper::getInstance()->m_KeyMapStatus==QKeyMapper::KEYMAP_IDLE
        && QkmIbInputClient::instance().requestedGeneration()==generation,"Save-and-start wrote or requested a run for a conflict");
    ++rejectedSaves;
    const auto reject=[&]() {
        const QByteArray bytes=savedIni();
        auto *mappingPointer=QKeyMapper::KeyMappingDataList;
        check(!window->saveKeyMapSetting(false),"Conflicting configuration was saved");
        check(savedIni()==bytes,"Rejected save changed INI bytes");
        check(QKeyMapper::KeyMappingDataList==mappingPointer && !QkmIbInputClient::instance().busy()
            && QkmIbInputClient::instance().requestedGeneration()==generation,"Save validation changed runtime state");
        ++rejectedSaves;
    };
    reject();
    *normal().KeyMappingData=originalRows;
    originalAction->trigger();
    check(window->addTabToKeyMappingTabWidget(QStringLiteral("IbSaveOther"),false),"Could not create inactive validation table");
    int otherIndex=-1;
    for (int index=0;index<QKeyMapper::s_KeyMappingTabInfoList.size();++index)
        if (QKeyMapper::s_KeyMappingTabInfoList.at(index).TabName==QStringLiteral("IbSaveOther")) otherIndex=index;
    check(otherIndex>=0,"Inactive validation table is missing");
    auto *other=QKeyMapper::s_KeyMappingTabInfoList[otherIndex].KeyMappingData;
    *other=originalRows; other->last().SendMappingKeyMethod=QKeyMapperConstants::SENDMAPPINGKEY_METHOD_SENDINPUT;
    reject(); other->clear();
    auto *advanced=window->findChild<QMappingAdvancedDialog *>();
    check(advanced,"Mapping advanced dialog is missing");
    auto *commonBox=advanced->findChild<QCheckBox *>(QStringLiteral("enableCommonMappingTableCheckBox"));
    check(commonBox,"Common feature control is missing");
    const bool originalCommon=commonBox->isChecked();
    const QSignalBlocker commonSignals(commonBox);
    commonBox->setChecked(true);
    window->ensureCommonMappingTabExists();
    const auto common=[]() -> KeyMappingTab_Info & {
        const int index=QKeyMapper::findCommonMappingTabIndex();
        check(index>=0,"Common source table is missing");
        return QKeyMapper::s_KeyMappingTabInfoList[index];
    };
    auto shared=originalRows.front(); shared.Original_Key=QStringLiteral("F12");
    shared.Pure_OriginalKeys=QStringList{QStringLiteral("F12")};
    shared.SendMappingKeyMethod=QKeyMapperConstants::SENDMAPPINGKEY_METHOD_SENDINPUT;
    *common().KeyMappingData=QList<MAP_KEYDATA>{shared};
    normal().IncludeCommonMappingTable=true;
    reject();
    common().KeyMappingData->first().Disabled=true;
    check(window->saveKeyMapSetting(false),"Disabled Common row blocked saving");
    common().KeyMappingData->first().Disabled=false;
    normal().IncludeCommonMappingTable=false;
    check(window->saveKeyMapSetting(false),"Non-appended Common row blocked an independent table");
    *common().KeyMappingData=QList<MAP_KEYDATA>{originalRows.front()};
    auto shadowed=originalRows.front(); shadowed.SendMappingKeyMethod=QKeyMapperConstants::SENDMAPPINGKEY_METHOD_SENDINPUT;
    *normal().KeyMappingData=QList<MAP_KEYDATA>{shadowed};
    normal().IncludeCommonMappingTable=true;
    check(window->saveKeyMapSetting(false),"Common-priority shadowed row was incorrectly counted");
    *normal().KeyMappingData=originalRows;
    normal().IncludeCommonMappingTable=originalInclude;
    common().KeyMappingData->clear();
    commonBox->setChecked(originalCommon);
    check(window->saveKeyMapSetting(false),"Restored valid configuration was not saved");
}


void configurationRoundTripCases(QKeyMapper *window) {
    const auto original=*QKeyMapper::KeyMappingDataList;
    const auto generation=QkmIbInputClient::instance().requestedGeneration();
    for (const auto &row:original) {
        const QString code=QKeyMapper::generateMappingCode(row);
        auto current=row; current.SendMappingKeyMethod=QKeyMapperConstants::SENDMAPPINGKEY_METHOD_SENDINPUT;
        MAP_KEYDATA decoded=current;
        const auto result=QKeyMapper::applyMappingCode(code,current,decoded);
        check(result.success && decoded.SendMappingKeyMethod==QKeyMapperConstants::SENDMAPPINGKEY_METHOD_IBINPUT
            && decoded.Mapping_Keys==row.Mapping_Keys,"Method 3 did not round-trip through a mapping code");
    }
    const QString exported=output+QStringLiteral("/method3-export.ini");
    check(!QFile::exists(exported),"Configuration export would overwrite evidence");
    check(QKeyMapper::exportKeyMappingDataToFile(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex,exported),
        "Method-3 export failed");
    check(window->addTabToKeyMappingTabWidget(QStringLiteral("IbRoundTrip"),false),"Cannot create isolated import target");
    int target=-1;
    for (int index=0;index<QKeyMapper::s_KeyMappingTabInfoList.size();++index)
        if (QKeyMapper::s_KeyMappingTabInfoList.at(index).TabName==QStringLiteral("IbRoundTrip")) target=index;
    check(target>=0 && QKeyMapper::importKeyMappingDataFromFile(target,exported),"Method-3 import failed");
    const auto imported=*QKeyMapper::s_KeyMappingTabInfoList.at(target).KeyMappingData;
    check(imported.size()==original.size(),"Imported method-3 row count differs");
    for (int index=0;index<original.size();++index)
        check(imported.at(index).SendMappingKeyMethod==QKeyMapperConstants::SENDMAPPINGKEY_METHOD_IBINPUT
            && imported.at(index).Original_Key==original.at(index).Original_Key
            && imported.at(index).Mapping_Keys==original.at(index).Mapping_Keys,"Import changed the method or mapping");
    const auto before=savedIni();
    const QString ini=QCoreApplication::applicationDirPath()+QStringLiteral("/keymapdata.ini");
    const HANDLE locked=CreateFileW(reinterpret_cast<LPCWSTR>(ini.utf16()),GENERIC_READ,FILE_SHARE_READ,
        nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    check(locked!=INVALID_HANDLE_VALUE,"Cannot lock the isolated INI for the save failure test");
    const bool saved=window->saveKeyMapSetting(false);
    CloseHandle(locked);
    check(!saved && savedIni()==before,"Failed method-3 save changed the file or reported success");
    check(!QkmIbInputClient::instance().busy()
        && QkmIbInputClient::instance().requestedGeneration()==generation,"Configuration checks initialized a run");
}

void verifyMacroRules(const QList<MAP_KEYDATA> &source) {
    const auto before=source;
    const auto rows=QkmIbInputValidation::expandedRows(source);
    const QList<QStringList> expected{
        {QStringLiteral("F"),QStringLiteral("G")},{QStringLiteral("H"),QStringLiteral("I")},
        {QStringLiteral("J"),QStringLiteral("K")},{QStringLiteral("L"),QStringLiteral("M")},
        {QStringLiteral("Mouse-L"),QStringLiteral("Mouse-R")},{QStringLiteral("Key2Mouse-Right")}};
    check(rows.size()==expected.size(),"Expanded macro row count changed");
    for (int index=0;index<rows.size();++index) {
        if (rows.at(index).Pure_MappingKeys!=expected.at(index) || rows.at(index).Pure_MappingKeys_KeyUp!=expected.at(index))
            throw std::runtime_error(QStringLiteral("Macro primitives row %1: DOWN=[%2], UP=[%3], expected=[%4], ordinary=%5, universal=%6")
                .arg(index+1).arg(rows.at(index).Pure_MappingKeys.join(QLatin1Char(',')))
                .arg(rows.at(index).Pure_MappingKeys_KeyUp.join(QLatin1Char(','))).arg(expected.at(index).join(QLatin1Char(',')))
                .arg(QKeyMapper::s_MappingMacroList.size()).arg(QKeyMapper::s_UniversalMappingMacroList.size()).toStdString());
    }
    check(source==before,"Macro inspection changed source rows");
    check(expandMacroKeys(QStringList{QStringLiteral("Macro(C3Pair)x2")}).size()==4,
        "Default macro expansion changed its repeat count");
    check(expandMacroKeys(QStringList{QStringLiteral("Macro(C3Pair)x2")},0,nullptr,false,true).size()==2,
        "Requirement collection materialized macro repetitions");
    MappingMacroData large;
    large.MappingMacro=QStringLiteral("Repeat{F⏱60»G⏱60}x99999");
    QKeyMapper::s_MappingMacroList.insert(QStringLiteral("C3CollectionProbe"),large);
    auto bounded=source;
    bounded.first().Mapping_Keys=QStringList{QStringLiteral("Macro(C3CollectionProbe)x99999")};
    const auto collected=QkmIbInputValidation::expandedRows(bounded);
    QKeyMapper::s_MappingMacroList.remove(QStringLiteral("C3CollectionProbe"));
    check(collected.first().Pure_MappingKeys==expected.first(),
        "Large nested macro requirements did not stay compact");
    QKeyMapper::s_MappingMacroList.insert(QStringLiteral("Key2Mouse-Right"),QKeyMapper::s_MappingMacroList.value(QStringLiteral("C3Pair")));
    auto namedLikeDirection=source;
    namedLikeDirection.first().Mapping_Keys=QStringList{QStringLiteral("Macro(Key2Mouse-Right)")};
    const auto namedRows=QkmIbInputValidation::expandedRows(namedLikeDirection);
    QKeyMapper::s_MappingMacroList.remove(QStringLiteral("Key2Mouse-Right"));
    check(namedRows.first().Pure_MappingKeys==expected.first() && QkmIbInputValidation::movementGroup(namedRows.first())<0,
        "A macro name was mistaken for actual mouse movement");
    auto mixed=source;
    auto other=source.last(); other.SendMappingKeyMethod=QKeyMapperConstants::SENDMAPPINGKEY_METHOD_SENDINPUT;
    other.Original_Key=QStringLiteral("F12"); other.Pure_OriginalKeys=QStringList{QStringLiteral("F12")};
    mixed.append(other);
    check(!QkmIbInputValidation::mixedMovementError(QkmIbInputValidation::expandedRows(mixed),{}).isEmpty(),
        "A macro hid mixed movement methods");
    mixed.last().Disabled=true;
    check(QkmIbInputValidation::mixedMovementError(QkmIbInputValidation::expandedRows(mixed),{}).isEmpty(),
        "Disabled macro movement blocked a valid table");
    auto probe=source;
    probe.first().Mapping_Keys=QStringList{QStringLiteral("Macro(C3Feedback)")};
    check(QkmIbInputValidation::expandedRows(probe).first().Pure_MappingKeys==QStringList{QStringLiteral("Q")},
        "Macro hid feedback to an original key");
    probe.first().Mapping_Keys=QStringList{QStringLiteral("Macro(C3Nested)")};
    check(QkmIbInputValidation::expandedRows(probe).first().Pure_MappingKeys==expected.first(),
        "Nested Repeat/Macro primitives were not resolved");
    probe.first().Mapping_Keys=QStringList{QStringLiteral("Macro(C3Cycle)")};
    check(QkmIbInputValidation::expandedRows(probe).first().Pure_MappingKeys.isEmpty(),
        "Recursive macro did not follow existing expansion semantics");
}

int inputPhase=0;
int tableStage=0;
int tableRaceRound=0,tableRaceStablePolls=0;
quint64 tableRaceIntent=0;
struct TableRaceGate { QSemaphore release; std::atomic_bool timedOut{false}; };
QSharedPointer<TableRaceGate> tableRaceGate;
bool absoluteTargetCleared=false;
QString absoluteOriginalTable;
int absoluteSwitches=0;
bool absoluteSwitchPending=false;
struct MotionProbe { std::atomic_bool done{false}; bool valid=false; };
QSharedPointer<MotionProbe> motionProbe;
QSharedPointer<QJoystickDevice> motionDevice;
int motionStep=0;
quint64 motionTimestamp=1000000;
quint64 inputGeneration=0;
HWND inputReceiver=nullptr;
void inputRecord(const QString &name,const QJsonObject &value) {
    QSaveFile file(output+QLatin1Char('/')+selectedCase+QLatin1Char('-')+name);
    check(file.open(QIODevice::WriteOnly),"Cannot open keyboard evidence");
    check(file.write(QJsonDocument(value).toJson())>0 && file.commit(),"Cannot write keyboard evidence");
}
void verifyMotionStep(const QString &prefix,int phase) {
    if (motionProbe) {
        if (!motionProbe->done.load(std::memory_order_acquire)) return;
        check(motionProbe->valid,"Motion source settings differ from the running product");
        inputRecord(QStringLiteral("step-%1.json").arg(motionStep+1),
            {{"step",motionStep+1},{"active",motionStep!=9},
                {"generation",static_cast<qint64>(qkmCurrentIbInputGeneration())}});
        motionProbe.clear(); ++motionStep;
    }
    const bool allowed=(phase==2 && motionStep<9) || (phase==7 && motionStep==9)
        || (phase==5 && motionStep>=10 && motionStep<13);
    if (!allowed || !QFile::exists(prefix+QStringLiteral("step-%1").arg(motionStep+1))) return;
    const auto probe=QSharedPointer<MotionProbe>::create(); motionProbe=probe;
    const auto device=motionDevice;
    const int step=motionStep;
    auto *worker=QKeyMapper_Worker::getInstance();
    check(QMetaObject::invokeMethod(worker,[probe,device,worker,step] {
        using namespace QKeyMapperConstants;
        const bool active=step!=9;
        probe->valid=active ? (QKeyMapper_Worker::s_Joy2Mouse_SendMethod==SENDMAPPINGKEY_METHOD_IBINPUT
            && QKeyMapper_Worker::s_Gyro2Mouse_SendMethod==SENDMAPPINGKEY_METHOD_IBINPUT
            && QKeyMapper_Worker::s_GamepadTouchpad2Mouse_SendMethod==SENDMAPPINGKEY_METHOD_IBINPUT
            && !QKeyMapper_Worker::s_Joy2Mouse_EnableStateMap.isEmpty()
            && QKeyMapper_Worker::s_GameControllerSensor_EnableState
            && QKeyMapper_Worker::s_GamepadTouchpad2Mouse_EnableState)
            : (!qkmCurrentIbInputGeneration() && QKeyMapper_Worker::s_Joy2Mouse_EnableStateMap.isEmpty()
                && !QKeyMapper_Worker::s_GameControllerSensor_EnableState
                && !QKeyMapper_Worker::s_GamepadTouchpad2Mouse_EnableState);
        if (probe->valid) {
            const int family=step<6 ? step/2 : step<9 ? step-6 : step==9 ? -1 : step-10;
            const int direction=(step<6 && step%2) ? -1 : 1;
            const auto dispatch=[&](int kind) {
                if (kind==0) {
                    QJoystickAxisEvent event{JOYSTICK_AXIS_LS_HORIZONTAL,qreal(direction),GameControllerEvent,device.data()};
                    worker->onJoystickAxisEvent(event);
                    worker->onKey2MouseCycleTimeout();
                    event.value=0; worker->onJoystickAxisEvent(event);
                } else if (kind==1) {
                    QJoystickSensorEvent event{};
                    event.joystick=device.data(); event.accelZ=1.0f;
                    event.timestamp=motionTimestamp; motionTimestamp+=20000;
                    worker->onJoystickSensorEvent(event);
                    event.gyroY=-15.0f*direction; event.timestamp=motionTimestamp; motionTimestamp+=20000;
                    worker->onJoystickSensorEvent(event);
                    event.gyroY=0; event.timestamp=motionTimestamp; motionTimestamp+=20000;
                    worker->onJoystickSensorEvent(event);
                } else {
                    QJoystickTouchpadEvent event{0,0,0.5,0.5,1.0,100,TouchpadEventDown,device.data()};
                    worker->onJoystickTouchpadEvent(event);
                    event.x+=0.01*direction; event.eventType=TouchpadEventMotion; event.timestamp=120;
                    worker->onJoystickTouchpadEvent(event);
                    event.eventType=TouchpadEventUp; event.timestamp=140;
                    worker->onJoystickTouchpadEvent(event);
                }
            };
            if (family<0) for (int kind=0;kind<3;++kind) dispatch(kind);
            else dispatch(family);
        }
        probe->done.store(true,std::memory_order_release);
    },Qt::QueuedConnection),"Cannot queue owned motion source events");
}

void verifyInputProduct(QKeyMapper *window) {
    const bool fakerPause=selectedCase==QStringLiteral("pause_faker");
    const bool pause=fakerPause || selectedCase==QStringLiteral("pause");
    const bool stopActions=selectedCase==QStringLiteral("stopactions");
    const bool tables=selectedCase==QStringLiteral("tables");
    const bool standard=selectedCase==QStringLiteral("standard");
    const bool loops=selectedCase==QStringLiteral("loops");
    const bool macros=selectedCase==QStringLiteral("macros");
    const bool motion=selectedCase==QStringLiteral("motion");
    const bool absolute=selectedCase==QStringLiteral("absolute");
    const bool horizontal=selectedCase==QStringLiteral("horizontal");
    const bool mouse=selectedCase==QStringLiteral("mouse") || loops || macros || horizontal || absolute || motion;
    const bool patterns=selectedCase==QStringLiteral("patterns");
    const QString prefix=output+QLatin1Char('/')+selectedCase+QLatin1Char('-');
    if (inputPhase!=6 && QFile::exists(prefix+QStringLiteral("abort"))) {
        inputPhase=6;
        check(QMetaObject::invokeMethod(window,"onTrayIconMenuQuitAction",Qt::DirectConnection),"Product abort quit entry is missing");
        return;
    }
    auto &client=QkmIbInputClient::instance();
    if (!inputPhase) {
        const HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
        check(snapshot!=INVALID_HANDLE_VALUE,"Cannot check other mapper instances");
        bool conflict=false;
        PROCESSENTRY32W entry{}; entry.dwSize=sizeof(entry);
        if (Process32FirstW(snapshot,&entry)) do {
            const QString name=QString::fromWCharArray(entry.szExeFile);
            if (entry.th32ProcessID!=GetCurrentProcessId() && name.startsWith(QStringLiteral("QKeyMapper"),Qt::CaseInsensitive)
                && name!=QStringLiteral("QKeyMapperIbInputKeyboardReceiver.exe") && name!=QStringLiteral("QKeyMapperCrashReporter.exe")) conflict=true;
        } while (Process32NextW(snapshot,&entry));
        CloseHandle(snapshot);
        check(!conflict,"Another mapper appeared; no mapping started");
        const auto *rows=QKeyMapper::KeyMappingDataList;
        const QStringList originals=motion ? QStringList{QStringLiteral("Joy-LS2Mouse"),QStringLiteral("Joy-Gyro2Mouse"),
            QStringLiteral("Joy-Touchpad2Mouse"),QStringLiteral("Q")} : absolute ? QStringList{QStringLiteral("Q"),QStringLiteral("W"),QStringLiteral("E"),
            QStringLiteral("R"),QStringLiteral("T"),QStringLiteral("A"),QStringLiteral("S")} : horizontal ? QStringList{QStringLiteral("Q"),QStringLiteral("W"),QStringLiteral("E")} : pause ? QStringList{QStringLiteral("Q")} : stopActions
            ? QStringList{QStringLiteral("Q"),QStringLiteral("SendOnMappingStart"),QStringLiteral("SendOnMappingStop")} : tables ? QStringList{QStringLiteral("Q"),QStringLiteral("W")} : standard
            ? QStringList{QStringLiteral("Q"),QStringLiteral("W"),QStringLiteral("E"),QStringLiteral("R"),
                QStringLiteral("T"),QStringLiteral("A"),QStringLiteral("S"),QStringLiteral("D")} : macros
            ? QStringList{QStringLiteral("Q"),QStringLiteral("W"),QStringLiteral("E"),QStringLiteral("R"),QStringLiteral("T"),QStringLiteral("A")}
            : (patterns || loops) ? QStringList{QStringLiteral("Q"),QStringLiteral("W"),QStringLiteral("E"),QStringLiteral("R"),QStringLiteral("T")} : mouse
            ? QStringList{QStringLiteral("Q"),QStringLiteral("W"),QStringLiteral("E"),QStringLiteral("R"),QStringLiteral("T"),
                QStringLiteral("A"),QStringLiteral("S"),QStringLiteral("D"),QStringLiteral("F")}
            : QStringList{QStringLiteral("W"),QStringLiteral("1"),QStringLiteral("F13")};
        const QStringList mapped=motion ? QStringList{QStringLiteral("Joy-LS2Mouse"),QStringLiteral("Joy-Gyro2Mouse"),
            QStringLiteral("Joy-Touchpad2Mouse"),QStringLiteral("Mouse-L")} : absolute ? QStringList{QStringLiteral("Mouse-Move(300,300)"),QStringLiteral("Mouse-L(320,310)"),
            QStringLiteral("Mouse-PosSave"),QStringLiteral("Mouse-Move(340,320)"),QStringLiteral("Mouse-PosRestore"),
            QStringLiteral("Mouse-Move:W(210,180)"),QStringLiteral("Mouse-L:W(230,190)")} : horizontal ? QStringList{QStringLiteral("Mouse-WheelLeft"),QStringLiteral("Mouse-WheelRight"),QStringLiteral("Mouse-L")} : pause ? QStringList{QStringLiteral("Pause")} : stopActions
            ? QStringList{QStringLiteral("F"),QStringLiteral("H⏱20»I⏱20"),QStringLiteral("Repeat{F⏱20»G⏱20}x2")} : tables ? QStringList{QStringLiteral("F"),QStringLiteral("SwitchTab(IbTableB)")} : standard
            ? QStringList{QString::fromUtf8(R"QKM(Backspace⏱20»Tab⏱20»Space⏱20»Enter)QKM"),
                QString::fromUtf8(R"QKM(Insert⏱20»Delete⏱20»Home⏱20»End⏱20»PageUp⏱20»PageDown⏱20»Up⏱20»Down⏱20»Left⏱20»Right)QKM"),
                QString::fromUtf8(R"QKM(Num0⏱20»Num1⏱20»Num5⏱20»Num9⏱20»Num/⏱20»Num*⏱20»Num-⏱20»Num＋⏱20»Num.⏱20»NumEnter)QKM"),
                QString::fromUtf8(R"QKM(;⏱20»'⏱20»[⏱20»]⏱20»\⏱20»-⏱20»=⏱20»,⏱20».⏱20»/⏱20»`)QKM"),
                QString::fromUtf8(R"QKM(L-Shift+F24)QKM"),
                QString::fromUtf8(R"QKM(R-Shift+F24)QKM"),
                QString::fromUtf8(R"QKM(L-Ctrl+F24)QKM"),
                QString::fromUtf8(R"QKM(R-Ctrl+F24)QKM")} : macros
            ? QStringList{QStringLiteral("Macro(C3Pair)x2"),QStringLiteral("UniversalMacro(C3Once)"),
                QStringLiteral("Repeat{OnlyOnce{J⏱60}»K⏱60}x2"),QStringLiteral("Macro(C3Hold)"),
                QStringLiteral("Macro(C3Buttons)"),QStringLiteral("Macro(C3Move)")} : loops
            ? QStringList{QStringLiteral("F⏱60»G"),QStringLiteral("H⏱60»I⏱60"),QStringLiteral("J⏱500»K⏱60"),
                QStringLiteral("L⏱500»M⏱60"),QStringLiteral("Mouse-L⏱500»Mouse-R⏱60")} : patterns ? QStringList{QStringLiteral("F+G"),QStringLiteral("H»I"),QStringLiteral("J"),QStringLiteral("K"),QStringLiteral("L")} : mouse
            ? QStringList{QStringLiteral("Mouse-L"),QStringLiteral("Mouse-R"),QStringLiteral("Mouse-M"),QStringLiteral("Mouse-X1"),
                QStringLiteral("Mouse-X2"),QStringLiteral("Mouse-WheelUp"),QStringLiteral("Mouse-WheelDown"),
                QStringLiteral("Key2Mouse-Right"),QStringLiteral("Key2Mouse-Left")}
            : QStringList{QStringLiteral("F"),QStringLiteral("2"),QStringLiteral("F24")};
        check(rows && rows->size()==originals.size(),"Input fixture row count changed");
        for (int index=0;index<originals.size();++index) {
            const auto &row=rows->at(index);
            check(row.Original_Key==originals.at(index)
                && row.Mapping_Keys.join(QString::fromUtf8(QKeyMapperConstants::SEPARATOR_NEXTARROW))==mapped.at(index)
                && row.SendMappingKeyMethod==(fakerPause ? QKeyMapperConstants::SENDMAPPINGKEY_METHOD_FAKERINPUT
                    : QKeyMapperConstants::SENDMAPPINGKEY_METHOD_IBINPUT),"Input fixture content changed");
            if (patterns) check(row.Burst==(index==2) && row.Lock==(index==3 || index==4)
                && (index!=2 || (row.BurstPressTime==60 && row.BurstReleaseTime==60)),"Pattern fixture timing/flags changed");
            if (loops) check(!row.Burst && !row.Lock && row.KeySeqHoldDown==(index==0)
                && row.RepeatMode==(index==0 ? QKeyMapperConstants::REPEAT_MODE_NONE
                    : index==1 ? QKeyMapperConstants::REPEAT_MODE_BYTIMES : QKeyMapperConstants::REPEAT_MODE_BYKEY)
                && (index!=1 || row.RepeatTimes==3),"Loop fixture timing/flags changed");
            if (macros) check(!row.Burst && !row.Lock && !row.KeySeqHoldDown
                && row.RepeatMode==((index==1 || index==2) ? QKeyMapperConstants::REPEAT_MODE_BYTIMES
                    : (index==3 || index==4) ? QKeyMapperConstants::REPEAT_MODE_BYKEY : QKeyMapperConstants::REPEAT_MODE_NONE)
                && (index!=1 || row.RepeatTimes==3) && (index!=2 || row.RepeatTimes==2),"Macro fixture timing/flags changed");
            if (standard || stopActions || pause || horizontal || absolute || motion) check(!row.Burst && !row.Lock && !row.KeySeqHoldDown
                && row.RepeatMode==QKeyMapperConstants::REPEAT_MODE_NONE,"Standard fixture flags changed");
        }
        if (tables) {
            const QStringList names{QStringLiteral("IbTableA"),QStringLiteral("IbTableB"),QStringLiteral("SendInputTable")};
            const QList<QStringList> originalTables{{QStringLiteral("Q"),QStringLiteral("W")},
                {QStringLiteral("Q"),QStringLiteral("W"),QStringLiteral("SendOnSwitchTab")},
                {QStringLiteral("Q"),QStringLiteral("W")}};
            const QList<QStringList> mappedTables{{QStringLiteral("F"),QStringLiteral("SwitchTab(IbTableB)")},
                {QStringLiteral("G"),QStringLiteral("SwitchTab(SendInputTable)"),QStringLiteral("L⏱20»M⏱20")},
                {QStringLiteral("J"),QStringLiteral("SwitchTab(IbTableA)")}};
            check(QKeyMapper::s_KeyMappingTabInfoList.size()==3 && QKeyMapper::s_KeyMappingTabWidgetCurrentIndex==0,
                "Three-table fixture selection changed");
            for (int tab=0;tab<3;++tab) {
                const auto &info=QKeyMapper::s_KeyMappingTabInfoList.at(tab);
                check(info.TabName==names.at(tab) && !info.IncludeCommonMappingTable
                    && info.KeyMappingData->size()==originalTables.at(tab).size(),"Three-table metadata changed");
                for (int row=0;row<info.KeyMappingData->size();++row) {
                    const auto &item=info.KeyMappingData->at(row);
                    check(item.Original_Key==originalTables.at(tab).at(row)
                        && item.Mapping_Keys.join(QStringLiteral("»"))==mappedTables.at(tab).at(row)
                        && item.MappingKeys_KeyUp.join(QStringLiteral("»"))==mappedTables.at(tab).at(row)
                        && item.SendMappingKeyMethod==(tab==2 ? QKeyMapperConstants::SENDMAPPINGKEY_METHOD_SENDINPUT
                            : QKeyMapperConstants::SENDMAPPINGKEY_METHOD_IBINPUT)
                        && !item.Disabled && !item.Burst && !item.Lock,"Three-table mapping changed");
                }
            }
        }
        if (macros) verifyMacroRules(*rows);
        inputReceiver=reinterpret_cast<HWND>(qEnvironmentVariable("QKM_IBINPUT_KEYBOARD_HWND").toULongLong());
        DWORD owner=0; GetWindowThreadProcessId(inputReceiver,&owner);
        wchar_t title[128]{}; GetWindowTextW(inputReceiver,title,128);
        check(IsWindow(inputReceiver) && owner==static_cast<DWORD>(qEnvironmentVariableIntValue("QKM_IBINPUT_KEYBOARD_PID"))
            && QString::fromWCharArray(title)==(mouse ? QStringLiteral("IbInput product mouse receiver")
                : QStringLiteral("IbInput basic keyboard receiver")),"Owned receiver identity differs");
        check(SetForegroundWindow(inputReceiver) || GetForegroundWindow()==inputReceiver,"Cannot activate owned keyboard receiver");
        window->hide();
        if (absolute) {
            // Keep the owned HWND fixture independent of periodic process-list matching.
            int detached=0;
            for (auto *timer:window->findChildren<QTimer *>(QString(),Qt::FindDirectChildrenOnly)) {
                if (QObject::disconnect(timer,&QTimer::timeout,window,&QKeyMapper::cycleRefreshProcessInfoTableProc)) {
                    timer->stop(); ++detached;
                }
            }
            check(detached==1,"Owned HWND fixture could not detach process-list refresh");
        }
        if (horizontal) check(client.preferences().backend==QStringLiteral("auto") && expectedBackend==QStringLiteral("razer"),
            "Horizontal fixture must test automatic Razer selection");
        if (absolute) {
            check(client.preferences().backend==(expectedBackend==QStringLiteral("logitech_ghub") ? QStringLiteral("auto") : QStringLiteral("razer")),
                "Absolute fixture preference differs from its expected backend");
            if (expectedBackend==QStringLiteral("logitech_ghub")) {
                const auto copy=*rows;
                const int original=QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
                absoluteOriginalTable=QKeyMapper::s_KeyMappingTabInfoList.at(original).TabName;
                check(window->addTabToKeyMappingTabWidget(QStringLiteral("IbAbsoluteOther"),false),"Cannot add owned coordinate table");
                *QKeyMapper::s_KeyMappingTabInfoList.last().KeyMappingData=copy;
                check(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex==original,"Coordinate fixture changed the initial table");
            }
        }
        if (motion) {
            check(client.preferences().backend==expectedBackend,"Motion fixture must use its explicit backend");
            motionDevice=QSharedPointer<QJoystickDevice>::create();
            motionDevice->id=-10001; motionDevice->instanceID=-10001; motionDevice->playerindex=0;
            motionDevice->name=QStringLiteral("IbInput owned synthetic device");
            motionDevice->blacklisted=false; motionDevice->has_gyro=true; motionDevice->has_accel=true;
            motionDevice->sensor_disabled=false;
            check(QKeyMapper::getJoystick2MouseSpeedX()==4 && QKeyMapper::getJoystick2MouseSpeedY()==4
                && QKeyMapper::getGyro2MouseXSpeed()==2 && QKeyMapper::getGyro2MouseYSpeed()==2
                && qAbs(QKeyMapper::getGamepadTouchpadXSpeed()-0.2)<0.001
                && qAbs(QKeyMapper::getGamepadTouchpadYSpeed()-0.2)<0.001
                && QKeyMapper::getGyro2MouseMinXSensitivity()==7 && QKeyMapper::getGyro2MouseMaxXSensitivity()==7
                && QKeyMapper::getGyro2MouseMinYSensitivity()==7 && QKeyMapper::getGyro2MouseMaxYSensitivity()==7
                && QKeyMapper::getGyro2MouseMaxThreshold()==2,"Motion fixture speed/sensitivity changed");
        }
        window->MappingSwitch(QKeyMapper::MAPPINGSTART_BUTTONCLICK);
        inputPhase=1;
    }
    check(IsWindow(inputReceiver),"Owned input receiver disappeared");
    const auto status=client.statusSnapshot();
    check(!status.releaseIncomplete,"Input release is incomplete");
    const bool channelReady=fakerPause
        ? (!status.connected && qkmCurrentIbInputGeneration()==0
            && QKeyMapper_Worker::s_AtomicHookProcState==QKeyMapperConstants::HOOKPROC_STATE_STARTED)
        : (status.connected && qkmCurrentIbInputGeneration()!=0);
    if ((inputPhase==1 || inputPhase==4) && channelReady
        && window->m_KeyMapStatus==QKeyMapper::KEYMAP_MAPPING_GLOBAL) {
        if (fakerPause) {
#ifdef FAKERINPUT_SUPPORT
            check(QKeyMapper_Worker::s_FakerInputClient
                && QKeyMapper_Worker::s_FakerInputClient_ConnectState==QKeyMapper_Worker::FAKERINPUT_CONNECT_SUCCESS,
                "FakerInput comparison driver is not connected");
#else
            check(false,"FakerInput is not compiled");
#endif
        } else check(status.backend==expectedBackend && !status.fallback,"Input backend differs");
        const auto *backendLabel=window->findChild<QLabel *>(QStringLiteral("ibInputStatusLabel"));
        check(backendLabel,"Main backend status label is missing");
        if (!fakerPause && !backendLabel->text().contains(expectedBackend==QStringLiteral("razer")
            ? QStringLiteral("Razer") : QStringLiteral("Logitech G HUB"))) {
            if (!QFile::exists(prefix+QStringLiteral("ui-waiting.json")))
                inputRecord(QStringLiteral("ui-waiting.json"),{{"text",backendLabel->text()},{"backend",status.backend},
                    {"generation",static_cast<qint64>(status.generation)}});
            return;
        }
        if (inputPhase==4 && !fakerPause) check(status.generation>inputGeneration,"Restart reused an old generation");
        inputGeneration=status.generation;
        QJsonObject started{{"generation",static_cast<qint64>(inputGeneration)},{"backend",fakerPause ? QStringLiteral("fakerinput") : status.backend},{"backend_label",backendLabel->text()},{"pid",static_cast<int>(GetCurrentProcessId())},{"desktop",currentDesktop()}};
        if (horizontal || absolute) started.insert(QStringLiteral("preferred_backend"),client.preferences().backend);
        if (absolute) {
            // Supply only this owned HWND to exercise window-coordinate sending, not process matching.
            QKeyMapper::s_CurrentMappingHWND=inputReceiver;
            absoluteTargetCleared=false;
            started.insert(QStringLiteral("window_target"),QString::number(reinterpret_cast<quintptr>(inputReceiver)));
        }
        if (macros) {
            check(QKeyMapper_Worker::s_Key2Mouse_EnableState
                && QKeyMapper_Worker::s_Key2Mouse_SendMethod==QKeyMapperConstants::SENDMAPPINGKEY_METHOD_IBINPUT,
                "Macro movement did not select IbInput");
            started.insert(QStringLiteral("key2mouse_method"),QKeyMapper_Worker::s_Key2Mouse_SendMethod);
            started.insert(QStringLiteral("macro_rules_passed"),true);
        }
        inputRecord(inputPhase==1 ? QStringLiteral("started.json") : QStringLiteral("restarted.json"),started);
        inputPhase=inputPhase==1 ? 2 : 5;
    }
    if (absolute && expectedBackend==QStringLiteral("logitech_ghub") && inputPhase==2 && absoluteSwitches<2) {
        if (!absoluteSwitchPending && QFile::exists(prefix+QStringLiteral("switch-%1").arg(absoluteSwitches+1))) {
            window->MappingTableSwitchByTabName(absoluteSwitches==0 ? QStringLiteral("IbAbsoluteOther") : absoluteOriginalTable,false);
            absoluteSwitchPending=true;
            return;
        }
        if (absoluteSwitchPending) {
            if (status.connected && status.backend==expectedBackend && !status.fallback
                && status.generation==client.requestedGeneration() && status.generation>inputGeneration
                && QKeyMapper_Worker::s_AtomicHookProcState==QKeyMapperConstants::HOOKPROC_STATE_STARTED
                && window->findChild<QToolButton *>(QStringLiteral("keymapButton"))->isEnabled()) {
                const auto old=inputGeneration;
                inputGeneration=status.generation; QKeyMapper::s_CurrentMappingHWND=inputReceiver;
                emit QKeyMapper_Worker::getInstance()->ibInputFault_Signal(QStringLiteral("Owned stale coordinate-table fault"),old);
                inputRecord(QStringLiteral("switched-%1.json").arg(++absoluteSwitches),
                    {{"generation",static_cast<qint64>(inputGeneration)},{"backend",status.backend},
                        {"table",QKeyMapper::s_KeyMappingTabWidgetCurrentIndex},{"released_previous",!status.releaseIncomplete}});
                absoluteSwitchPending=false;
            }
            return;
        }
    }
    if (absolute && inputPhase==2 && !absoluteTargetCleared && QFile::exists(prefix+QStringLiteral("clear-target"))) {
        QKeyMapper::s_CurrentMappingHWND=nullptr; absoluteTargetCleared=true;
        inputRecord(QStringLiteral("target-cleared.json"),{{"generation",static_cast<qint64>(inputGeneration)},
            {"connected",status.connected},{"target",QStringLiteral("0")}});
    }
    if (absolute && (inputPhase==2 || inputPhase==5))
        check(QKeyMapper::s_CurrentMappingHWND==(absoluteTargetCleared ? nullptr : inputReceiver),
            "Owned coordinate target changed unexpectedly");
    if (motion) verifyMotionStep(prefix,inputPhase);
    if (tables && inputPhase==2 && tableRaceRound<3) {
        const QStringList commands{QStringLiteral("race-sendinput"),QStringLiteral("race-latest"),QStringLiteral("race-stop")};
        if (QFile::exists(prefix+commands.at(tableRaceRound))) {
            const auto gate=QSharedPointer<TableRaceGate>::create(); tableRaceGate=gate;
            // Hold queued worker restarts until the UI has reserved every intent in this fixed burst.
            check(QMetaObject::invokeMethod(QKeyMapper_Worker::getInstance(),[gate] {
                if (!gate->release.tryAcquire(1,5000)) gate->timedOut.store(true);
            },Qt::QueuedConnection),"Cannot queue table race gate");
            if (tableRaceRound==1) window->MappingTableSwitchByTabName(QStringLiteral("IbTableA"),false);
            window->MappingTableSwitchByTabName(QStringLiteral("IbTableB"),false);
            window->MappingTableSwitchByTabName(tableRaceRound==1 ? QStringLiteral("IbTableA") : QStringLiteral("SendInputTable"),false);
            if (tableRaceRound==2) window->MappingTableSwitchByTabName(QStringLiteral("IbTableA"),false);
            tableRaceIntent=client.requestedGeneration();
            if (tableRaceRound==2) window->MappingSwitch(QKeyMapper::MAPPINGSTART_BUTTONCLICK);
            gate->release.release();
            inputRecord(QStringLiteral("race-requested-%1.json").arg(++tableRaceRound),
                {{"intent",static_cast<qint64>(tableRaceIntent)},{"accepting",client.acceptingInput()},
                    {"table",QKeyMapper::s_KeyMappingTabWidgetCurrentIndex}});
            inputPhase=tableRaceRound==3 ? 3 : 9;
        }
    }
    if (tables && inputPhase==9) {
        check(tableRaceGate && !tableRaceGate->timedOut.load(),"Table race gate timed out");
        const int target=tableRaceRound==1 ? 2 : 0;
        const auto current=qkmCurrentIbInputGeneration();
        const bool ready=target==2 ? (!status.connected && !client.busy() && current==0)
            : (status.connected && !status.fallback && status.backend==expectedBackend && current==tableRaceIntent);
        if (QKeyMapper::s_KeyMappingTabWidgetCurrentIndex==target && ready
            && QKeyMapper_Worker::s_AtomicHookProcState==QKeyMapperConstants::HOOKPROC_STATE_STARTED
            && window->m_KeyMapStatus==QKeyMapper::KEYMAP_MAPPING_GLOBAL
            && window->findChild<QToolButton *>(QStringLiteral("keymapButton"))->isEnabled()) {
            check(client.requestedGeneration()==tableRaceIntent,"Queued restart consumed a different intent");
            inputGeneration=current;
            inputRecord(QStringLiteral("race-ready-%1.json").arg(tableRaceRound),
                {{"generation",static_cast<qint64>(current)},{"intent",static_cast<qint64>(tableRaceIntent)},
                    {"table",target},{"backend",status.backend},{"connected",status.connected}});
            inputPhase=2;
        }
    }
    if (tables && inputPhase==2 && tableStage<3
        && QFile::exists(prefix+QStringLiteral("switch-%1").arg(tableStage+1))) inputPhase=8;
    if (tables && inputPhase==8) {
        const int target=(tableStage+1)%3;
        const auto current=qkmCurrentIbInputGeneration();
        const bool backendReady=target==2 ? (!status.connected && !client.busy() && current==0)
            : (status.connected && !status.fallback && status.backend==expectedBackend
                && current>inputGeneration && status.generation==current);
        if (QKeyMapper::s_KeyMappingTabWidgetCurrentIndex==target && backendReady
            && QKeyMapper_Worker::s_AtomicHookProcState==QKeyMapperConstants::HOOKPROC_STATE_STARTED
            && window->m_KeyMapStatus==QKeyMapper::KEYMAP_MAPPING_GLOBAL
            && window->findChild<QToolButton *>(QStringLiteral("keymapButton"))->isEnabled()) {
            const auto *active=QKeyMapper::KeyMappingDataList;
            check(active && !active->isEmpty()
                && active->first().SendMappingKeyMethod==(target==2 ? QKeyMapperConstants::SENDMAPPINGKEY_METHOD_SENDINPUT
                    : QKeyMapperConstants::SENDMAPPINGKEY_METHOD_IBINPUT),"New table snapshot has the wrong channel");
            inputGeneration=current;
            inputRecord(QStringLiteral("switched-%1.json").arg(++tableStage),
                {{"table",target},{"generation",static_cast<qint64>(current)},
                    {"intent",static_cast<qint64>(client.requestedGeneration())},{"backend",status.backend},
                    {"connected",status.connected},{"released_previous",!status.releaseIncomplete}});
            inputPhase=2;
        }
    }
    if (inputPhase==2 && QFile::exists(prefix+QStringLiteral("stop"))) {
        window->MappingSwitch(QKeyMapper::MAPPINGSTART_BUTTONCLICK); inputPhase=3;
    }
    if (inputPhase==3 && window->m_KeyMapStatus==QKeyMapper::KEYMAP_IDLE && !client.busy()
        && !status.connected && qkmCurrentIbInputGeneration()==0) {
        inputRecord(QStringLiteral("stopped.json"),{{"released",true},{"generation",static_cast<qint64>(inputGeneration)}});
        inputPhase=7;
    }
    if (tables && inputPhase==7 && tableRaceRound==3) {
        check(tableRaceGate && !tableRaceGate->timedOut.load(),"Stop race gate timed out");
        check(window->m_KeyMapStatus==QKeyMapper::KEYMAP_IDLE && !client.busy() && !status.connected
            && !client.acceptingInput() && qkmCurrentIbInputGeneration()==0
            && QKeyMapper_Worker::s_AtomicHookProcState==QKeyMapperConstants::HOOKPROC_STATE_STOPPED,
            "A queued table restart revived stopped mapping");
        if (++tableRaceStablePolls==5) inputRecord(QStringLiteral("race-stable.json"),
            {{"stopped",true},{"cancelled_intent",static_cast<qint64>(tableRaceIntent)},{"connected",false}});
    }
    if (inputPhase==7 && QFile::exists(prefix+QStringLiteral("restart"))) {
        check(GetForegroundWindow()==inputReceiver,"Owned receiver lost foreground before restart");
        window->MappingSwitch(QKeyMapper::MAPPINGSTART_BUTTONCLICK); inputPhase=4;
    }
    if (inputPhase==5 && QFile::exists(prefix+QStringLiteral("exit"))) {
        inputRecord(QStringLiteral("exiting.json"),{{"exit_requested",true},{"generation",static_cast<qint64>(inputGeneration)}});
        inputPhase=6;
        check(QMetaObject::invokeMethod(window,"onTrayIconMenuQuitAction",Qt::DirectConnection),"Product quit entry is missing");
    }
}

void verifyProduct() {
    try {
        if (++polls>((selectedCase==QStringLiteral("keyboard") || selectedCase==QStringLiteral("mouse") || selectedCase==QStringLiteral("patterns") || selectedCase==QStringLiteral("loops") || selectedCase==QStringLiteral("macros") || selectedCase==QStringLiteral("standard") || selectedCase==QStringLiteral("tables") || selectedCase==QStringLiteral("stopactions") || selectedCase==QStringLiteral("pause") || selectedCase==QStringLiteral("pause_faker") || selectedCase==QStringLiteral("horizontal") || selectedCase==QStringLiteral("absolute") || selectedCase==QStringLiteral("motion")) ? 900 : 180)) throw std::runtime_error("Product check exceeded 90 seconds");
        check(currentDesktop()==expectedDesktop,"Product left its selected desktop");
        QKeyMapper *window=nullptr;
        for (QWidget *widget:QApplication::topLevelWidgets())
            if (auto *candidate=qobject_cast<QKeyMapper *>(widget)) { window=candidate; break; }
        if (!window || !QKeyMapper::s_AtomicIsInitialized.loadAcquire()) {
            QTimer::singleShot(500,verifyProduct); return;
        }
        if (selectedCase==QStringLiteral("keyboard") || selectedCase==QStringLiteral("mouse") || selectedCase==QStringLiteral("patterns") || selectedCase==QStringLiteral("loops") || selectedCase==QStringLiteral("macros") || selectedCase==QStringLiteral("standard") || selectedCase==QStringLiteral("tables") || selectedCase==QStringLiteral("stopactions") || selectedCase==QStringLiteral("pause") || selectedCase==QStringLiteral("pause_faker") || selectedCase==QStringLiteral("horizontal") || selectedCase==QStringLiteral("absolute") || selectedCase==QStringLiteral("motion")) {
            verifyInputProduct(window); QTimer::singleShot(100,verifyProduct); return;
        }
        if (checked) {
            if (!QFile::exists(output+QStringLiteral("/inspection-complete"))) {
                QTimer::singleShot(500,verifyProduct); return;
            }
            QFile observation(output+QStringLiteral("/inspection-complete"));
            check(observation.open(QIODevice::ReadOnly),"Cannot read external capture status");
            const auto capture=QJsonDocument::fromJson(observation.readAll()).object();
            record({{"passed",true},{"case",selectedCase},{"desktop",currentDesktop()},{"rejected_saves_unchanged",rejectedSaves},
                {"external_capture",capture.value(QStringLiteral("captured")).toBool()},
                {"capture_detail",capture.value(QStringLiteral("detail")).toString()},
                {"scope",QStringLiteral("Real product entry, four method-3 rows, global preferences, save/cold-read and Popup binding; mapping was not started")}});
            check(QMetaObject::invokeMethod(window,"onTrayIconMenuQuitAction",Qt::DirectConnection),"Product quit entry is missing"); return;
        }
        check(!QkmIbInputClient::instance().busy(),"Product unexpectedly started an IbInput run");
        const auto *rows=QKeyMapper::KeyMappingDataList;
        check(rows && rows->size()==4,"Isolated product INI did not load four rows");
        const QStringList originals{QStringLiteral("F8"),QStringLiteral("F9"),QStringLiteral("F10"),QStringLiteral("F11")};
        const QStringList mappings{QStringLiteral("Key2Mouse-Right"),QStringLiteral("Key2Mouse-Left"),
            QStringLiteral("Key2Mouse-Down"),QStringLiteral("Key2Mouse-Up")};
        for (int index=0;index<rows->size();++index) {
            const auto &row=rows->at(index);
            check(row.Original_Key==originals.at(index) && row.Mapping_Keys==QStringList{mappings.at(index)}
                && row.SendMappingKeyMethod==QKeyMapperConstants::SENDMAPPINGKEY_METHOD_IBINPUT,
                "Method-3 mapping changed during product load");
        }
        if (selectedCase==QStringLiteral("save")) {
            queuedFaultCase(window);
            saveGuardCases(window);
            configurationRoundTripCases(window);
        }
        auto *button=window->findChild<QPushButton *>(QStringLiteral("ibInputSettingsButton"));
        check(button && button->isEnabled(),"Product IbInput settings entry is missing or locked");
        auto *aligned=window->findChild<QPushButton *>(QStringLiteral("mappingAdvancedSettingButton"));
        auto *status=window->findChild<QLabel *>(QStringLiteral("ibInputStatusLabel"));
        auto *faker=window->findChild<QLabel *>(QStringLiteral("FakerInputStatusLabel"));
        check(aligned && button->width()==aligned->width(),"IbInput button is not aligned with the existing column");
        check(button->text()==QCoreApplication::translate("QKeyMapper","IbInput settings"),"Product caption was not translated");
        check(status && faker && status->frameStyle()==faker->frameStyle()
            && status->alignment()==faker->alignment() && status->contentsMargins()==faker->contentsMargins(),"Status label did not reuse FakerInput presentation");
        button->click();
        auto *popup=window->findChild<QkmIbInputPopup *>();
        check(popup && popup->isVisible(),"Product button did not open its Popup");
        const auto combos=popup->findChildren<QComboBox *>();
        check(combos.size()==2,"Product Popup controls changed");
        if (selectedCase==QStringLiteral("save")) {
            combos.at(0)->setCurrentIndex(0);
            combos.at(0)->setCurrentIndex(expectedBackend==QStringLiteral("razer") ? 3 : 1);
            combos.at(1)->setCurrentIndex(1);
            combos.at(1)->setCurrentIndex(0);
            check(window->saveKeyMapSetting(false),"Product save returned failure");
        }
        const QString ini=QCoreApplication::applicationDirPath()+QStringLiteral("/keymapdata.ini");
        QSettings settings(ini,QSettings::IniFormat);
        QKeyMapperQtCompat::setIniCodecUtf8(settings);
        settings.sync();
        check(settings.status()==QSettings::NoError,"Saved product INI could not be read");
        check(settings.value(QStringLiteral("QKeyMapperGlobalSetting/KeyMapData_SendMappingKeyMethod")).toString()
            .split(QLatin1Char('|')).value(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex)==QStringLiteral("3#3#3#3"),"Method 3 did not survive product save/cold-read");
        if (selectedCase==QStringLiteral("cold")) {
            bool found=false;
            for (const auto &tab:QKeyMapper::s_KeyMappingTabInfoList) {
                if (tab.TabName!=QStringLiteral("IbRoundTrip")) continue;
                found=true;
                check(tab.KeyMappingData->size()==4,"Cold imported table row count changed");
                for (const auto &row:*tab.KeyMappingData)
                    check(row.SendMappingKeyMethod==QKeyMapperConstants::SENDMAPPINGKEY_METHOD_IBINPUT,
                        "Cold imported table lost method 3");
            }
            check(found,"Cold process did not load the imported table");
        }
        const auto preferences=QkmIbInputClient::readPreferences(settings);
        check(preferences.backend==expectedBackend && preferences.unavailablePolicy==QStringLiteral("block"),
            "Global preferences did not survive product save/cold-read");
        check(!QkmIbInputClient::instance().busy(),"Popup binding initialized an IbInput run");
        QFile ready(output+QStringLiteral("/ready.json"));
        check(ready.open(QIODevice::WriteOnly),"Cannot write product capture request");
        ready.write(QJsonDocument(QJsonObject{{"pid",static_cast<int>(GetCurrentProcessId())},
            {"desktop",currentDesktop()},{"hwnd",QString::number(static_cast<qulonglong>(popup->winId()))},
            {"case",selectedCase}}).toJson());
        checked=true;
        QTimer::singleShot(500,verifyProduct);
    } catch (const std::exception &error) {
        record({{"passed",false},{"case",selectedCase},{"desktop",currentDesktop()},
            {"error",QString::fromUtf8(error.what())}});
        QCoreApplication::exit(2);
    }
}
void scheduleCheck() { QTimer::singleShot(3000,verifyProduct); }
}
Q_COREAPP_STARTUP_FUNCTION(scheduleCheck)

int main(int argc,char **argv) {
    output=qEnvironmentVariable("QKM_IBINPUT_PRODUCT_OUTPUT");
    selectedCase=qEnvironmentVariable("QKM_IBINPUT_PRODUCT_CASE");
    expectedDesktop=qEnvironmentVariable("QKM_IBINPUT_PRODUCT_DESKTOP");
    expectedBackend=qEnvironmentVariable("QKM_IBINPUT_PRODUCT_BACKEND",QStringLiteral("logitech_ghub"));
    if (expectedBackend!=QStringLiteral("logitech_ghub") && expectedBackend!=QStringLiteral("razer")) return 2;
    const bool input=selectedCase==QStringLiteral("keyboard") || selectedCase==QStringLiteral("mouse") || selectedCase==QStringLiteral("patterns") || selectedCase==QStringLiteral("loops") || selectedCase==QStringLiteral("macros") || selectedCase==QStringLiteral("standard") || selectedCase==QStringLiteral("tables") || selectedCase==QStringLiteral("stopactions") || selectedCase==QStringLiteral("pause") || selectedCase==QStringLiteral("pause_faker") || selectedCase==QStringLiteral("horizontal") || selectedCase==QStringLiteral("absolute") || selectedCase==QStringLiteral("motion");
    if (argc!=1 || !QDir(output).exists() || currentDesktop()!=expectedDesktop
        || (input ? expectedDesktop!=QStringLiteral("Default")
            : ((selectedCase!=QStringLiteral("save") && selectedCase!=QStringLiteral("cold"))
                || !expectedDesktop.startsWith(QStringLiteral("QkmIbProduct_"))))) return 2;
    return qkmIbProductMain(argc,argv);
}
