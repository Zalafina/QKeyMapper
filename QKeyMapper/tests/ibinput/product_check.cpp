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
#include <Windows.h>
#include <tlhelp32.h>
#include <QTimer>
#include <stdexcept>
#include "../../qkm_ibinput_client.h"
#include "../../qkm_ibinput_popup.h"

// Keep the real entry point, startup and orderly shutdown in this maintenance target.
#define main qkmIbProductMain
#include "../../main.cpp"
#undef main

namespace {
QString output;
QString selectedCase;
QString expectedDesktop;
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


int keyboardPhase=0;
quint64 keyboardGeneration=0;
HWND keyboardReceiver=nullptr;
void keyboardRecord(const QString &name,const QJsonObject &value) {
    QSaveFile file(output+QLatin1Char('/')+name);
    check(file.open(QIODevice::WriteOnly),"Cannot open keyboard evidence");
    check(file.write(QJsonDocument(value).toJson())>0 && file.commit(),"Cannot write keyboard evidence");
}
void verifyKeyboardProduct(QKeyMapper *window) {
    if (keyboardPhase!=6 && QFile::exists(output+QStringLiteral("/keyboard-abort"))) {
        keyboardPhase=6;
        check(QMetaObject::invokeMethod(window,"onTrayIconMenuQuitAction",Qt::DirectConnection),"Product abort quit entry is missing");
        return;
    }
    auto &client=QkmIbInputClient::instance();
    if (!keyboardPhase) {
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
        check(rows && rows->size()==3,"Keyboard fixture did not load three rows");
        const QStringList originals{QStringLiteral("W"),QStringLiteral("1"),QStringLiteral("F13")};
        const QStringList mapped{QStringLiteral("F"),QStringLiteral("2"),QStringLiteral("F24")};
        for (int index=0;index<3;++index)
            check(rows->at(index).Original_Key==originals.at(index) && rows->at(index).Mapping_Keys==QStringList{mapped.at(index)}
                && rows->at(index).SendMappingKeyMethod==QKeyMapperConstants::SENDMAPPINGKEY_METHOD_IBINPUT,"Keyboard fixture content changed");
        keyboardReceiver=reinterpret_cast<HWND>(qEnvironmentVariable("QKM_IBINPUT_KEYBOARD_HWND").toULongLong());
        DWORD owner=0; GetWindowThreadProcessId(keyboardReceiver,&owner);
        wchar_t title[128]{}; GetWindowTextW(keyboardReceiver,title,128);
        check(IsWindow(keyboardReceiver) && owner==static_cast<DWORD>(qEnvironmentVariableIntValue("QKM_IBINPUT_KEYBOARD_PID"))
            && QString::fromWCharArray(title)==QStringLiteral("IbInput basic keyboard receiver"),"Owned receiver identity differs");
        check(SetForegroundWindow(keyboardReceiver) || GetForegroundWindow()==keyboardReceiver,"Cannot activate owned keyboard receiver");
        window->hide();
        window->MappingSwitch(QKeyMapper::MAPPINGSTART_BUTTONCLICK);
        keyboardPhase=1;
    }
    check(IsWindow(keyboardReceiver),"Owned keyboard receiver disappeared");
    const auto status=client.statusSnapshot();
    check(!status.releaseIncomplete,"Keyboard release is incomplete");
    if ((keyboardPhase==1 || keyboardPhase==4) && status.connected
        && window->m_KeyMapStatus==QKeyMapper::KEYMAP_MAPPING_GLOBAL && qkmCurrentIbInputGeneration()!=0) {
        check(status.backend==QStringLiteral("logitech_ghub") && !status.fallback,"Keyboard backend differs");
        if (keyboardPhase==4) check(status.generation>keyboardGeneration,"Restart reused an old generation");
        keyboardGeneration=status.generation;
        keyboardRecord(keyboardPhase==1 ? QStringLiteral("keyboard-started.json") : QStringLiteral("keyboard-restarted.json"),
            {{"generation",static_cast<qint64>(keyboardGeneration)},{"pid",static_cast<int>(GetCurrentProcessId())},{"desktop",currentDesktop()}});
        keyboardPhase=keyboardPhase==1 ? 2 : 5;
    }
    if (keyboardPhase==2 && QFile::exists(output+QStringLiteral("/keyboard-stop"))) {
        window->MappingSwitch(QKeyMapper::MAPPINGSTART_BUTTONCLICK); keyboardPhase=3;
    }
    if (keyboardPhase==3 && window->m_KeyMapStatus==QKeyMapper::KEYMAP_IDLE && !client.busy()
        && !status.connected && qkmCurrentIbInputGeneration()==0) {
        keyboardRecord(QStringLiteral("keyboard-stopped.json"),{{"released",true},{"generation",static_cast<qint64>(keyboardGeneration)}});
        keyboardPhase=7;
    }
    if (keyboardPhase==7 && QFile::exists(output+QStringLiteral("/keyboard-restart"))) {
        check(GetForegroundWindow()==keyboardReceiver,"Owned receiver lost foreground before restart");
        window->MappingSwitch(QKeyMapper::MAPPINGSTART_BUTTONCLICK); keyboardPhase=4;
    }
    if (keyboardPhase==5 && QFile::exists(output+QStringLiteral("/keyboard-exit"))) {
        keyboardRecord(QStringLiteral("keyboard-exiting.json"),{{"exit_requested",true},{"generation",static_cast<qint64>(keyboardGeneration)}});
        keyboardPhase=6;
        check(QMetaObject::invokeMethod(window,"onTrayIconMenuQuitAction",Qt::DirectConnection),"Product quit entry is missing");
    }
}

void verifyProduct() {
    try {
        if (++polls>(selectedCase==QStringLiteral("keyboard") ? 900 : 180)) throw std::runtime_error("Product check exceeded 90 seconds");
        check(currentDesktop()==expectedDesktop,"Product left its selected private desktop");
        QKeyMapper *window=nullptr;
        for (QWidget *widget:QApplication::topLevelWidgets())
            if (auto *candidate=qobject_cast<QKeyMapper *>(widget)) { window=candidate; break; }
        if (!window || !QKeyMapper::s_AtomicIsInitialized.loadAcquire()) {
            QTimer::singleShot(500,verifyProduct); return;
        }
        if (selectedCase==QStringLiteral("keyboard")) {
            verifyKeyboardProduct(window); QTimer::singleShot(100,verifyProduct); return;
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
        if (selectedCase==QStringLiteral("save")) saveGuardCases(window);
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
            combos.at(0)->setCurrentIndex(1);
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
        const auto preferences=QkmIbInputClient::readPreferences(settings);
        check(preferences.backend==QStringLiteral("logitech_ghub") && preferences.unavailablePolicy==QStringLiteral("block"),
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
    const bool keyboard=selectedCase==QStringLiteral("keyboard");
    if (argc!=1 || !QDir(output).exists() || currentDesktop()!=expectedDesktop
        || (keyboard ? expectedDesktop!=QStringLiteral("Default")
            : ((selectedCase!=QStringLiteral("save") && selectedCase!=QStringLiteral("cold"))
                || !expectedDesktop.startsWith(QStringLiteral("QkmIbProduct_"))))) return 2;
    return qkmIbProductMain(argc,argv);
}
