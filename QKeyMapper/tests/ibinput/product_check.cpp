#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QSettings>
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
void verifyProduct() {
    try {
        if (++polls>180) throw std::runtime_error("Product check exceeded 90 seconds");
        check(currentDesktop()==expectedDesktop,"Product left its selected private desktop");
        QKeyMapper *window=nullptr;
        for (QWidget *widget:QApplication::topLevelWidgets())
            if (auto *candidate=qobject_cast<QKeyMapper *>(widget)) { window=candidate; break; }
        if (!window || !QKeyMapper::s_AtomicIsInitialized.loadAcquire()) {
            QTimer::singleShot(500,verifyProduct); return;
        }
        if (checked) {
            if (!QFile::exists(output+QStringLiteral("/inspection-complete"))) {
                QTimer::singleShot(500,verifyProduct); return;
            }
            QFile observation(output+QStringLiteral("/inspection-complete"));
            check(observation.open(QIODevice::ReadOnly),"Cannot read external capture status");
            const auto capture=QJsonDocument::fromJson(observation.readAll()).object();
            record({{"passed",true},{"case",selectedCase},{"desktop",currentDesktop()},
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
        auto *button=window->findChild<QPushButton *>(QStringLiteral("ibInputSettingsButton"));
        check(button && button->isEnabled(),"Product IbInput settings entry is missing or locked");
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
            ==QStringLiteral("3#3#3#3"),"Method 3 did not survive product save/cold-read");
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
    if (argc!=1 || !QDir(output).exists()
        || (selectedCase!=QStringLiteral("save") && selectedCase!=QStringLiteral("cold"))
        || !expectedDesktop.startsWith(QStringLiteral("QkmIbProduct_"))
        || currentDesktop()!=expectedDesktop) return 2;
    return qkmIbProductMain(argc,argv);
}
