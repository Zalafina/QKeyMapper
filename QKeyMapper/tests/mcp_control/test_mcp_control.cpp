#include "../../automation/qkm_control_dialog.h"
#include "../../automation/qkm_source_snapshot.h"
#include "../../automation/qkm_mapping_help.h"
#include "../../automation/qkm_mapping_changes.h"
#include "../../automation/qkm_window_access.h"
#include "../../automation/qkm_demo_mode.h"
#include "../../automation/qkm_demo_process.h"
#include "../../automation/qkm_demo_host.h"
#include "../../automation/qkm_window_image.h"
#include "../../qkm_mapping_source_access.h"
#include "../../qkeymapper_qt_compat.h"
#include "../../qkeymapper_constants.h"
#include "ui_qgeneraladvanceddialog.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSettings>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonArray>
#include <QItemSelectionModel>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QTranslator>
#include <QWindowStateChangeEvent>
#include <QVector>
#include <QtTest/QTest>
#include <thread>
#include <cstring>

using namespace QkmAutomation;

namespace {

QByteArray demoDeploymentSeed()
{
    return "[General]\nSettingSelect=Demo104Seed\nStartupAutoMonitoring=false\nStartupMinimized=false\n"
        "VirtualGamepadEnable=false\nMultiInputEnable=false\nEnableSystemFilterKeyChecked=false\nDisableFilterKeyClickSoundChecked=false\n"
        "CloseToSystemtray=false\nCloseToSystemtray_Decided=true\n[Demo104Seed]\nAutoStartMappingChecked=0\n"
        "MappingTable_Common_Enabled=false\nKeyMapData_OriginalKeys=\nKeyMapData_MappingKeys=\n";
}

ConfigurationSaveBinding saveBinding()
{
    ConfigurationSaveBinding binding;
    binding.settingName = QStringLiteral("Save94");
    binding.revision = QString(64, QLatin1Char('a'));
    binding.context = {{QStringLiteral("fixture"), QStringLiteral("private-context94")}};
    binding.scope = {{QStringLiteral("mode"), QStringLiteral("full_gui_save")}};
    binding.expiresAt.setRemainingTime(MappingChangeStore::PreviewTtlMs, Qt::PreciseTimer);
    return binding;
}

bool installUiTranslation(QTranslator &translator, int language)
{
    QCoreApplication::removeTranslator(&translator);
    const QStringList locales{QStringLiteral("zh_CN"), QStringLiteral("en_US"), QStringLiteral("ja_JP")};
    if (language < 0 || language >= locales.size()) return false;
    const QString directory = qEnvironmentVariable("QKM_TEST_TRANSLATIONS_DIR");
    return !directory.isEmpty()
        && translator.load(QDir(directory).filePath(QStringLiteral("QKeyMapper_%1.qm").arg(locales.at(language))))
        && QCoreApplication::installTranslator(&translator);
}

class Client : public QProcess {
public:
    bool launch(AutomationSessionServer &server, const QString &directory, const QString &mode)
    {
        const auto own = queryProcessIdentity(GetCurrentProcessId());
        if (!own.ok()) return false;
        start(qEnvironmentVariable("QKM_IPC_TEST_EXE"), {QStringLiteral("--session-client"), server.pipeName(),
            server.instanceId(), QString::number(own.identity.processId), QString::number(own.identity.creationTime), directory, mode});
        return waitForStarted(3000);
    }
    ~Client() override
    {
        if (state() != NotRunning) { kill(); waitForFinished(3000); }
    }
};

template<class T> T *widget(QWidget &dialog, const char *name)
{
    T *result = dialog.findChild<T *>(QLatin1String(name));
    Q_ASSERT(result);
    return result;
}

void fixture(AutomationSessionServer &server)
{
    server.setRequestHandler([&server](const AutomationRequest &request) {
        QVERIFY(server.isRequestCurrent(request));
        auto stale = request;
        stale.permissionEpoch += 1;
        QVERIFY(!server.isRequestCurrent(stale));
        stale = request;
        stale.requestId = QString(32, QLatin1Char('0'));
        QVERIFY(!server.isRequestCurrent(stale));
        stale = request;
        stale.connectionId.clear();
        QVERIFY(!server.isRequestCurrent(stale));
        stale = request;
        stale.scopes ^= Configure;
        QVERIFY(!server.isRequestCurrent(stale));
        QVERIFY(server.reply(request.connectionId, request.requestId, {{QStringLiteral("fixture"), true}}));
        QVERIFY(!server.isRequestCurrent(request));
    });
}

void clickCheckBox(QWidget &dialog, const char *name)
{
    auto *box = widget<QCheckBox>(dialog, name);
    QStyleOptionButton option;
    option.initFrom(box);
    option.text = box->text();
    const QRect indicator = box->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, box);
    QTest::mouseClick(box, Qt::LeftButton, Qt::NoModifier, indicator.center());
}

} // namespace

namespace QkmAutomation {
class MappingChangeStoreTestAccess {
public:
    static int previews(const MappingChangeStore &store) { return store.m_previews.size(); }
    static int results(const MappingChangeStore &store) { return store.m_results.size(); }
    static void expirePreview(MappingChangeStore &store, const QString &id)
    { store.m_previews[id].created -= MappingChangeStore::PreviewTtlMs + 1; }
    static void expireResult(MappingChangeStore &store, const QString &id)
    { store.m_results[id].created -= MappingChangeStore::ResultTtlMs + 1; }
    static void expireSave(MappingChangeStore &store, const QString &id)
    { store.m_previews[id].save->expiresAt = QDeadlineTimer(); }
};
}

class McpControlTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void defaultOffAndLanguage_data();
    void defaultOffAndLanguage();
    void startupChoice_data();
    void startupChoice();
    void startupChoiceFailures();
    void advancedEntryKeyboard();
    void controlFocusPolicy();
    void languageRefreshPreservesState();
    void pairingFromWidgets_data();
    void pairingFromWidgets();
    void windowPermissions_data();
    void windowPermissions();
    void windowObservation();
    void windowAdvancedObservation();
    void windowCaptureBoundaries();
    void mappingEditorArguments();
    void mappingSettingsArguments();
    void startupPurpose();
    void demoProcessLifecycle();
    void demoPairingPrompt();
    void privateBackgroundDesktopGuard();
    void editorDraftGuard();
    void rejectAndClose_data();
    void rejectAndClose();
    void enableFailure();
    void offlinePairings_data();
    void offlinePairings();
    void offlineFailure_data();
    void offlineFailure();
    void selectedPairing_data();
    void selectedPairing();
    void sourceSnapshotReferences();
    void sourceSnapshotBoundaries();
    void sourceSnapshotEncodingBoundaries();
    void sourceSnapshotQueryCost_data();
    void sourceSnapshotQueryCost();
    void sourceReadAdmission();
    void mappingHelpReference();
    void mappingHelpBoundaries();
    void mappingChangeArguments();
    void savePreviewArguments();
    void mappingRemovalArguments();
    void mappingRemovalLifecycle();
    void mappingChangeLifecycle();
    void mappingChangeLimits();
    void configurationSaveLifecycle();
    void configurationSaveExpiry();
    void configurationSaveLimits();
};

void McpControlTest::mappingChangeArguments()
{
    const QString revision(64, QLatin1Char('a'));
    const QJsonObject input{{QStringLiteral("source_revision"), revision},
        {QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0")},
        {QStringLiteral("original_key"), QStringLiteral("A")}, {QStringLiteral("mapping_key"), QStringLiteral("B")}};
    const auto request = validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), input);
    QVERIFY(request.ok());
    QCOMPARE(request.addition.originalKey, QStringLiteral("A"));
    QCOMPARE(request.addition.mappingKey, QStringLiteral("B"));
    QCOMPARE(request.addition.insertRow, -1);
    for (const QJsonValue &row : {QJsonValue(QJsonValue::Null), QJsonValue(0), QJsonValue(2147483647)}) {
        auto changed = input; changed.insert(QStringLiteral("insert_row"), row);
        QVERIFY(validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), changed).ok());
    }
    for (const QJsonValue &row : {QJsonValue(-1), QJsonValue(.5), QJsonValue(2147483648.0), QJsonValue(true), QJsonValue(QStringLiteral("1"))}) {
        auto changed = input; changed.insert(QStringLiteral("insert_row"), row);
        QVERIFY(!validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), changed).ok());
    }
    for (const QString &field : {QStringLiteral("original_key"), QStringLiteral("mapping_key")}) {
        for (const QJsonValue &key : {QJsonValue(QString()), QJsonValue(QStringLiteral("a")), QJsonValue(QStringLiteral("A+B")),
            QJsonValue(QStringLiteral("Func-Test")), QJsonValue(QStringLiteral("Ａ")), QJsonValue(1), QJsonValue(false)}) {
            auto changed = input; changed.insert(field, key);
            QVERIFY(!validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), changed).ok());
        }
    }
    auto changed = input; changed.insert(QStringLiteral("unknown"), true);
    QVERIFY(!validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), changed).ok());
    changed = input; changed.remove(QStringLiteral("source_revision"));
    QVERIFY(!validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), changed).ok());
    changed = input; changed.insert(QStringLiteral("source_tab_ref"), QString(64, QLatin1Char('b')) + QStringLiteral(":source:0"));
    QCOMPARE(validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), changed).errorCode, QStringLiteral("stale_source"));
    const QString id(32, QLatin1Char('a'));
    for (const QString &method : {QStringLiteral("qkm_apply_changes"), QStringLiteral("qkm_cancel_changes"), QStringLiteral("qkm_get_operation")}) {
        const QString field = method == QStringLiteral("qkm_get_operation") ? QStringLiteral("operation_id") : QStringLiteral("preview_id");
        QVERIFY(validateMappingChangeRequest(method, {{field, id}}).ok());
        QVERIFY(!validateMappingChangeRequest(method, {{field, QString(32, QLatin1Char('A'))}}).ok());
        QVERIFY(!validateMappingChangeRequest(method, {{field, id}, {QStringLiteral("extra"), true}}).ok());
    }
    QVERIFY(validateMappingChangeRequest(QStringLiteral("qkm_apply_changes"),
        {{QStringLiteral("preview_id"), id}, {QStringLiteral("confirm_conflicts"), true}}).ok());
    QVERIFY(!validateMappingChangeRequest(QStringLiteral("qkm_apply_changes"),
        {{QStringLiteral("preview_id"), id}, {QStringLiteral("confirm_conflicts"), 1}}).ok());
    const auto capabilities = mappingChangeCapabilities();
    QCOMPARE(capabilities.size(), 8);
    QCOMPARE(capabilities.first().toObject().value(QStringLiteral("preview_ttl_ms")).toInt(), 120000);
    QCOMPARE(capabilities.first().toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    const auto operation = capabilities.last().toObject();
    QCOMPARE(operation.value(QStringLiteral("required_scopes")).toInt(), int(Read));
    QVERIFY(!operation.value(QStringLiteral("limitations")).toArray().contains(QStringLiteral("idle_after_cleanup")));
}

void McpControlTest::savePreviewArguments()
{
    const QString method = QStringLiteral("qkm_preview_changes");
    const QJsonObject valid{{QStringLiteral("action"), QStringLiteral("configuration.save")}, {QStringLiteral("setting_name"), QStringLiteral("配置95日本語")}};
    const auto save = validateMappingChangeRequest(method, valid);
    QVERIFY(save.ok() && save.action == MappingChangeAction::SaveConfiguration);
    QCOMPARE(save.settingName, valid.value(QStringLiteral("setting_name")).toString());
    auto changed = valid; changed.insert(QStringLiteral("setting_name"), QString(32767, QLatin1Char('a')));
    QVERIFY(validateMappingChangeRequest(method, changed).ok());
    changed.insert(QStringLiteral("setting_name"), QString(32768, QLatin1Char('a')));
    QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    for (const QJsonValue &name : {QJsonValue(QString()), QJsonValue(true), QJsonValue(1), QJsonValue(QJsonValue::Null)}) {
        changed = valid; changed.insert(QStringLiteral("setting_name"), name);
        QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    }
    for (const QString &field : {QStringLiteral("source_revision"), QStringLiteral("source_tab_ref"), QStringLiteral("original_key"),
            QStringLiteral("mapping_key"), QStringLiteral("insert_row"), QStringLiteral("path"), QStringLiteral("confirm_scope")}) {
        changed = valid; changed.insert(field, QStringLiteral("not allowed"));
        QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    }
    changed = valid; changed.remove(QStringLiteral("action"));
    QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    for (const QJsonValue &action : {QJsonValue(QStringLiteral("configuration.delete")), QJsonValue(true), QJsonValue(QJsonValue::Null)}) {
        changed = valid; changed.insert(QStringLiteral("action"), action);
        QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    }
    const auto capabilities = mappingChangeCapabilities();
    const auto capability = capabilities.at(1).toObject();
    QCOMPARE(capability.value(QStringLiteral("id")).toString(), QStringLiteral("configuration.save.preview"));
    QCOMPARE(capability.value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(capability.value(QStringLiteral("read_only")).toBool());
    QVERIFY(capability.value(QStringLiteral("limitations")).toArray().contains(QStringLiteral("explicit_full_scope_confirmation")));
    const auto schema = capability.value(QStringLiteral("arguments_schema")).toObject();
    QVERIFY(!schema.value(QStringLiteral("additionalProperties")).toBool());
    QCOMPARE(schema.value(QStringLiteral("required")).toArray(), QJsonArray({QStringLiteral("action"), QStringLiteral("setting_name")}));
    QVERIFY(!schema.value(QStringLiteral("properties")).toObject().contains(QStringLiteral("source_revision")));
    const auto apply = validateMappingChangeRequest(QStringLiteral("qkm_apply_changes"),
        {{QStringLiteral("preview_id"), QString(32, QLatin1Char('a'))}, {QStringLiteral("confirm_scope"), true}, {QStringLiteral("confirm_overwrite"), true}});
    QVERIFY(apply.ok() && apply.confirmScope && apply.confirmOverwrite && !apply.confirmConflicts);
    for (const QString &flag : {QStringLiteral("confirm_scope"), QStringLiteral("confirm_overwrite")}) {
        for (const QJsonValue &wrong : {QJsonValue(1), QJsonValue(QStringLiteral("true")), QJsonValue(QJsonValue::Null)}) {
            QVERIFY(!validateMappingChangeRequest(QStringLiteral("qkm_apply_changes"),
                {{QStringLiteral("preview_id"), QString(32, QLatin1Char('a'))}, {flag, wrong}}).ok());
        }
    }
    const auto saveApply = capabilities.at(3).toObject();
    QCOMPARE(saveApply.value(QStringLiteral("id")).toString(), QStringLiteral("configuration.save.apply"));
    QVERIFY(!saveApply.value(QStringLiteral("read_only")).toBool());
    QCOMPARE(saveApply.value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
}

void McpControlTest::mappingRemovalArguments()
{
    const QString revision(64, QLatin1Char('a'));
    const QString reference = revision + QStringLiteral(":row:0:1");
    const QString method = QStringLiteral("qkm_preview_changes");
    const QJsonObject input{{QStringLiteral("action"), QStringLiteral("mapping.delete")},
        {QStringLiteral("source_revision"), revision}, {QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0")},
        {QStringLiteral("source_refs"), QJsonArray{reference, revision + QStringLiteral(":row:0:0"), reference}}};
    const auto request = validateMappingChangeRequest(method, input);
    QVERIFY(request.ok() && request.action == MappingChangeAction::DeleteMapping);
    QCOMPARE(request.removal.sourceRevision, revision);
    QCOMPARE(request.removal.sourceTabRef, revision + QStringLiteral(":source:0"));
    QCOMPARE(request.removal.sourceRows, QList<int>({1, 0}));
    for (const QString &field : {QStringLiteral("source_revision"), QStringLiteral("source_tab_ref"), QStringLiteral("source_refs")}) {
        auto changed = input; changed.remove(field);
        QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    }
    for (const QJsonValue &value : {QJsonValue(QJsonValue::Null), QJsonValue(true), QJsonValue(reference), QJsonValue(QJsonArray())}) {
        auto changed = input; changed.insert(QStringLiteral("source_refs"), value);
        QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    }
    for (const QString &suffix : {QStringLiteral("-1"), QStringLiteral("01"), QStringLiteral("2147483648"), QStringLiteral("0:1"), QStringLiteral("x")}) {
        auto changed = input; changed.insert(QStringLiteral("source_refs"), QJsonArray{revision + QStringLiteral(":row:0:") + suffix});
        QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    }
    for (const QJsonValue &referenceValue : {QJsonValue(1), QJsonValue(QString(64, QLatin1Char('b')) + QStringLiteral(":row:0:1")),
            QJsonValue(revision + QStringLiteral(":row:1:1")), QJsonValue(revision + QStringLiteral(":row:00:1"))}) {
        auto changed = input; changed.insert(QStringLiteral("source_refs"), QJsonArray{referenceValue});
        QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    }
    for (const QString &field : {QStringLiteral("original_key"), QStringLiteral("insert_row"), QStringLiteral("setting_name"), QStringLiteral("pid")}) {
        auto changed = input; changed.insert(field, 0);
        QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    }
    QJsonArray references;
    for (int i = 0; i < 100; ++i) references.append(reference);
    auto changed = input; changed.insert(QStringLiteral("source_refs"), references);
    QVERIFY(validateMappingChangeRequest(method, changed).ok());
    references.append(reference); changed.insert(QStringLiteral("source_refs"), references);
    QVERIFY(!validateMappingChangeRequest(method, changed).ok());
    const QString id(32, QLatin1Char('c'));
    const auto apply = validateMappingChangeRequest(QStringLiteral("qkm_apply_changes"),
        {{QStringLiteral("preview_id"), id}, {QStringLiteral("confirm_delete"), true}});
    QVERIFY(apply.ok() && apply.confirmDelete);
    QVERIFY(!validateMappingChangeRequest(QStringLiteral("qkm_apply_changes"),
        {{QStringLiteral("preview_id"), id}, {QStringLiteral("confirm_delete"), 1}}).ok());
    const auto capabilities = mappingChangeCapabilities();
    const auto preview = capabilities.at(4).toObject(), submit = capabilities.at(5).toObject();
    QCOMPARE(preview.value(QStringLiteral("id")).toString(), QStringLiteral("mapping.delete.preview"));
    QVERIFY(preview.value(QStringLiteral("read_only")).toBool());
    QCOMPARE(submit.value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(!submit.value(QStringLiteral("read_only")).toBool());
    QVERIFY(submit.value(QStringLiteral("limitations")).toArray().contains(QStringLiteral("explicit_delete_confirmation")));
}

void McpControlTest::mappingRemovalLifecycle()
{
    MappingChangeStore store;
    const MappingChangeOwner owner{QString(32, QLatin1Char('a')), QString(32, QLatin1Char('b')), 1};
    const MappingRemovalArguments arguments{QString(64, QLatin1Char('c')), QString(64, QLatin1Char('c')) + QStringLiteral(":source:1"), {1, 0}};
    const auto preview = store.createRemoval(owner, arguments, {}); QVERIFY(preview && preview->removal && !preview->save);
    const auto target = store.operation(owner, preview->id).value(QStringLiteral("target")).toObject();
    QCOMPARE(target.value(QStringLiteral("instance_id")).toString(), owner.instance);
    QCOMPARE(target.value(QStringLiteral("source_tab_ref")).toString(), arguments.sourceTabRef);
    QVERIFY(!store.begin({owner.instance, owner.pairId, 2}, preview->id));
    QVERIFY(!store.begin({QString(32, QLatin1Char('d')), owner.pairId, 1}, preview->id));
    const auto begun = store.begin(owner, preview->id); QVERIFY(begun && begun->removal);
    QCOMPARE(begun->removal->sourceRows, arguments.sourceRows);
    store.finish(owner, preview->id, QStringLiteral("applied"), {{QStringLiteral("deleted_count"), 2}});
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("target")).toObject(), target);
    QVERIFY(!store.begin(owner, preview->id));
    const auto expired = store.createRemoval(owner, arguments, {}); QVERIFY(expired);
    MappingChangeStoreTestAccess::expirePreview(store, expired->id);
    QCOMPARE(store.operation(owner, expired->id).value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("preview_expired"));
    QCOMPARE(store.operation(owner, expired->id).value(QStringLiteral("target")).toObject(), target);
    const auto cancelled = store.createRemoval(owner, arguments, {}); QVERIFY(cancelled && store.cancel(owner, cancelled->id));
    QCOMPARE(store.operation(owner, cancelled->id).value(QStringLiteral("target")).toObject(), target);
    store.clear();
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("unknown"));
}

void McpControlTest::mappingChangeLifecycle()
{
    MappingChangeStore store;
    const MappingChangeOwner owner{QString(32, QLatin1Char('a')), QString(32, QLatin1Char('b')), 1};
    const MappingChangeOwner other{owner.instance, QString(32, QLatin1Char('c')), 1};
    const MappingChangeOwner nextEpoch{owner.instance, owner.pairId, 2};
    MappingAdditionArguments arguments; arguments.originalKey = QStringLiteral("A"); arguments.mappingKey = QStringLiteral("B");
    const auto preview = store.create(owner, arguments, {{QStringLiteral("marker"), QStringLiteral("private-fixture")}});
    QVERIFY(preview.has_value());
    QVERIFY(isLowerHex(preview->id, 32));
    QVERIFY(!store.expired(*preview));
    auto oldPreview = *preview;
    oldPreview.created -= MappingChangeStore::PreviewTtlMs + 1;
    QVERIFY(store.expired(oldPreview));
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("previewed"));
    QVERIFY(!store.begin(other, preview->id));
    QVERIFY(!store.cancel(nextEpoch, preview->id));
    QCOMPARE(store.operation(other, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("unknown"));
    const auto begun = store.begin(owner, preview->id);
    QVERIFY(begun.has_value());
    QCOMPARE(begun->arguments.mappingKey, QStringLiteral("B"));
    QVERIFY(!store.begin(owner, preview->id));
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("applying"));
    const QJsonObject result{{QStringLiteral("row"), 0}, {QStringLiteral("saved"), false}};
    store.finish(owner, preview->id, QStringLiteral("applied"), result);
    const auto completed = store.operation(owner, preview->id);
    QCOMPARE(completed.value(QStringLiteral("status")).toString(), QStringLiteral("applied"));
    QCOMPARE(completed.value(QStringLiteral("result")).toObject(), result);
    QVERIFY(completed.value(QStringLiteral("historical_result")).toBool());
    QVERIFY(!completed.value(QStringLiteral("query_sampled_at_utc")).toString().isEmpty());
    QVERIFY(!store.begin(owner, preview->id) && !store.cancel(owner, preview->id));
    store.finish(owner, preview->id, QStringLiteral("rejected"), {}, QStringLiteral("bad"), QStringLiteral("must not overwrite"));
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("applied"));
    QCOMPARE(store.operation(nextEpoch, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("unknown"));
    const auto cancel = store.create(owner, arguments, {}); QVERIFY(cancel);
    QVERIFY(store.cancel(owner, cancel->id));
    QCOMPARE(store.operation(owner, cancel->id).value(QStringLiteral("status")).toString(), QStringLiteral("cancelled"));
    QVERIFY(!store.begin(owner, cancel->id));
    const auto old = store.create(owner, arguments, {}); QVERIFY(old);
    const auto replacement = store.create(owner, arguments, {}); QVERIFY(replacement);
    QCOMPARE(MappingChangeStoreTestAccess::previews(store), 1);
    QCOMPARE(store.operation(owner, old->id).value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("preview_superseded"));
    QVERIFY(store.begin(owner, replacement->id));
    store.finish(owner, replacement->id, QStringLiteral("rejected"), {}, QStringLiteral("stale_source"), QStringLiteral("fixed rejection"));
    QCOMPARE(store.operation(owner, replacement->id).value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("stale_source"));
    store.clear();
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("unknown"));
    QVERIFY(!store.begin(owner, preview->id));
}

void McpControlTest::mappingChangeLimits()
{
    MappingChangeStore store;
    const MappingChangeOwner owner{QStringLiteral("instance"), QStringLiteral("pair"), 1};
    const auto expired = store.create(owner, {}, {}); QVERIFY(expired);
    MappingChangeStoreTestAccess::expirePreview(store, expired->id);
    const auto result = store.operation(owner, expired->id);
    QCOMPARE(result.value(QStringLiteral("status")).toString(), QStringLiteral("cancelled"));
    QCOMPARE(result.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("preview_expired"));
    QVERIFY(!store.begin(owner, expired->id));
    MappingChangeStoreTestAccess::expireResult(store, expired->id);
    QCOMPARE(store.operation(owner, expired->id).value(QStringLiteral("status")).toString(), QStringLiteral("unknown"));
    for (int index = 0; index < MappingChangeStore::MaxResults + 5; ++index) {
        const auto preview = store.create(owner, {}, {}); QVERIFY(preview);
        QVERIFY(store.begin(owner, preview->id));
        store.finish(owner, preview->id, QStringLiteral("applied"), {});
        QVERIFY(MappingChangeStoreTestAccess::results(store) <= MappingChangeStore::MaxResults);
    }
    store.clear();
    for (int index = 0; index < MappingChangeStore::MaxPreviews; ++index) {
        QVERIFY(store.create({owner.instance, QString::number(index), 1}, {}, {}));
    }
    QVERIFY(!store.create(owner, {}, {}));
    QCOMPARE(MappingChangeStoreTestAccess::previews(store), MappingChangeStore::MaxPreviews);
    QVERIFY(store.create({owner.instance, QStringLiteral("0"), 2}, {}, {}));
    QCOMPARE(MappingChangeStoreTestAccess::previews(store), MappingChangeStore::MaxPreviews);
}

void McpControlTest::configurationSaveLifecycle()
{
    MappingChangeStore store;
    const MappingChangeOwner owner{QStringLiteral("instance94"), QStringLiteral("pair94"), 1};
    const MappingChangeOwner other{QStringLiteral("other-instance94"), owner.pairId, owner.permissionEpoch};
    const MappingChangeOwner nextEpoch{owner.instance, owner.pairId, 2};
    const auto mapping = store.create(owner, MappingAdditionArguments(), {}); QVERIFY(mapping);
    const auto binding = saveBinding();
    const auto preview = store.createSave(owner, binding, {{QStringLiteral("fixture"), QStringLiteral("private-description94")}});
    QVERIFY(preview && preview->save && preview->arguments.originalKey.isEmpty());
    QCOMPARE(preview->save->expiresAt, binding.expiresAt);
    QCOMPARE(store.operation(owner, mapping->id).value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("preview_superseded"));
    const QJsonObject target{{QStringLiteral("instance_id"), owner.instance}, {QStringLiteral("group"), binding.settingName},
        {QStringLiteral("save_revision"), binding.revision}};
    const auto pending = store.operation(owner, preview->id);
    QCOMPARE(pending.value(QStringLiteral("target")).toObject(), target);
    QVERIFY(!QJsonDocument(pending).toJson().contains("private-context94"));
    QVERIFY(!QJsonDocument(pending).toJson().contains("private-description94"));
    QVERIFY(!store.begin(other, preview->id) && !store.cancel(nextEpoch, preview->id));
    QCOMPARE(store.operation(other, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("unknown"));
    const auto begun = store.begin(owner, preview->id); QVERIFY(begun && begun->save);
    QCOMPARE(begun->save->context, binding.context);
    QCOMPARE(begun->save->scope, binding.scope);
    QVERIFY(!store.begin(owner, preview->id));
    auto nextBinding = binding; nextBinding.settingName = QStringLiteral("Next94");
    const auto next = store.createSave(owner, nextBinding, {}); QVERIFY(next);
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("applying"));
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("target")).toObject(), target);
    const auto replacement = store.create(owner, MappingAdditionArguments(), {}); QVERIFY(replacement && !replacement->save);
    QCOMPARE(store.operation(owner, next->id).value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("preview_superseded"));
    QVERIFY(store.cancel(owner, replacement->id));
    const QJsonObject result{{QStringLiteral("disk_saved"), true}, {QStringLiteral("reloaded"), false}};
    store.finish(owner, preview->id, QStringLiteral("applied"), result);
    const auto completed = store.operation(owner, preview->id);
    QCOMPARE(completed.value(QStringLiteral("result")).toObject(), result);
    QCOMPARE(completed.value(QStringLiteral("target")).toObject(), target);
    QVERIFY(!store.begin(owner, preview->id) && !store.cancel(owner, preview->id));
    store.finish(owner, preview->id, QStringLiteral("rejected"), {}, QStringLiteral("stale_source"), QStringLiteral("late result"));
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("applied"));
    QCOMPARE(store.operation(nextEpoch, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("unknown"));
    store.clear();
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("status")).toString(), QStringLiteral("unknown"));
}

void McpControlTest::configurationSaveExpiry()
{
    MappingChangeStore store;
    const MappingChangeOwner owner{QStringLiteral("instance94"), QStringLiteral("pair94"), 1};
    const auto original = store.create(owner, MappingAdditionArguments(), {}); QVERIFY(original);
    for (int variant = 0; variant < 7; ++variant) {
        auto invalid = saveBinding();
        if (variant == 0) invalid.settingName.clear();
        if (variant == 1) invalid.revision = QString(64, QLatin1Char('A'));
        if (variant == 2) invalid.context = {};
        if (variant == 3) invalid.scope = {};
        if (variant == 4) invalid.expiresAt = QDeadlineTimer();
        if (variant == 5) invalid.expiresAt = QDeadlineTimer(QDeadlineTimer::Forever);
        if (variant == 6) invalid.expiresAt.setRemainingTime(MappingChangeStore::PreviewTtlMs + 1000, Qt::PreciseTimer);
        QVERIFY(!store.createSave(owner, invalid, {}));
        QCOMPARE(store.operation(owner, original->id).value(QStringLiteral("status")).toString(), QStringLiteral("previewed"));
    }
    auto binding = saveBinding();
    binding.expiresAt.setRemainingTime(10000, Qt::PreciseTimer);
    const auto preview = store.createSave(owner, binding, {}); QVERIFY(preview);
    const auto remaining = store.operation(owner, preview->id).value(QStringLiteral("remaining_ms")).toDouble();
    QVERIFY(remaining > 0 && remaining <= 10000);
    QCOMPARE(preview->save->expiresAt, binding.expiresAt);
    auto expiredCopy = *preview;
    expiredCopy.save->expiresAt = QDeadlineTimer();
    QVERIFY(store.expired(expiredCopy));
    MappingChangeStoreTestAccess::expireSave(store, preview->id);
    const auto expired = store.operation(owner, preview->id);
    QCOMPARE(expired.value(QStringLiteral("status")).toString(), QStringLiteral("cancelled"));
    QCOMPARE(expired.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("preview_expired"));
    QCOMPARE(expired.value(QStringLiteral("target")).toObject().value(QStringLiteral("group")).toString(), binding.settingName);
    QVERIFY(!store.begin(owner, preview->id));
    const auto active = store.createSave(owner, saveBinding(), {}); QVERIFY(active && store.begin(owner, active->id));
    MappingChangeStoreTestAccess::expireResult(store, active->id);
    QCOMPARE(store.operation(owner, active->id).value(QStringLiteral("status")).toString(), QStringLiteral("applying"));
    store.finish(owner, active->id, QStringLiteral("applied"), {});
    QCOMPARE(store.operation(owner, active->id).value(QStringLiteral("status")).toString(), QStringLiteral("applied"));
    MappingChangeStoreTestAccess::expireResult(store, active->id);
    QCOMPARE(store.operation(owner, active->id).value(QStringLiteral("status")).toString(), QStringLiteral("unknown"));
}

void McpControlTest::configurationSaveLimits()
{
    MappingChangeStore store;
    QVector<MappingChangePreview> active;
    for (int index = 0; index < MappingChangeStore::MaxResults; ++index) {
        const MappingChangeOwner owner{QStringLiteral("instance94"), QString::number(index), 1};
        const auto preview = store.createSave(owner, saveBinding(), {}); QVERIFY(preview);
        const auto begun = store.begin(owner, preview->id); QVERIFY(begun);
        active.append(*begun);
    }
    const MappingChangeOwner waiting{QStringLiteral("instance94"), QStringLiteral("waiting94"), 1};
    const auto pending = store.createSave(waiting, saveBinding(), {}); QVERIFY(pending);
    QVERIFY(!store.begin(waiting, pending->id));
    QCOMPARE(store.operation(waiting, pending->id).value(QStringLiteral("status")).toString(), QStringLiteral("previewed"));
    QCOMPARE(MappingChangeStoreTestAccess::results(store), MappingChangeStore::MaxResults);
    for (const auto &preview : active)
        QCOMPARE(store.operation(preview.owner, preview.id).value(QStringLiteral("status")).toString(), QStringLiteral("applying"));
    store.finish(active.first().owner, active.first().id, QStringLiteral("applied"), {});
    QVERIFY(store.begin(waiting, pending->id));
    QCOMPARE(MappingChangeStoreTestAccess::results(store), MappingChangeStore::MaxResults);
    for (int index = 1; index < active.size(); ++index)
        QCOMPARE(store.operation(active.at(index).owner, active.at(index).id).value(QStringLiteral("status")).toString(), QStringLiteral("applying"));
    store.clear();
    for (int index = 0; index < MappingChangeStore::MaxPreviews; ++index)
        QVERIFY(store.createSave({waiting.instance, QString::number(index), 1}, saveBinding(), {}));
    QVERIFY(!store.createSave(waiting, saveBinding(), {}));
    QCOMPARE(MappingChangeStoreTestAccess::previews(store), MappingChangeStore::MaxPreviews);
    QVERIFY(store.createSave({waiting.instance, QStringLiteral("0"), 2}, saveBinding(), {}));
    QCOMPARE(MappingChangeStoreTestAccess::previews(store), MappingChangeStore::MaxPreviews);
}

void McpControlTest::mappingHelpReference()
{
    const auto capability = mappingHelpCapability();
    QCOMPARE(capability.value(QStringLiteral("id")).toString(), QStringLiteral("help.read"));
    QCOMPARE(capability.value(QStringLiteral("required_scopes")).toInt(), 1);
    QVERIFY(capability.value(QStringLiteral("read_only")).toBool());
    const auto help = queryMappingHelp({{QStringLiteral("topic"), QStringLiteral("basic_mapping")}}, {}, {});
    QVERIFY(help.ok());
    QCOMPARE(help.page.value(QStringLiteral("reference_scope")).toString(), QStringLiteral("parser_help_only"));
    QVERIFY(help.page.value(QStringLiteral("next_offset")).isNull());
    const auto item = help.page.value(QStringLiteral("items")).toArray().first().toObject();
    QCOMPARE(item.value(QStringLiteral("example_syntax")).toObject(), (QJsonObject{
        {QStringLiteral("original_key"), QStringLiteral("A")}, {QStringLiteral("mapping_key"), QStringLiteral("B")}}));
    QCOMPARE(item.value(QStringLiteral("sequence_separator")).toString(), QStringLiteral("»"));
    QCOMPARE(item.value(QStringLiteral("sequence_max_items_exclusive")).toInt(), 1000000);
    QVERIFY(item.value(QStringLiteral("mcp_edit_requires")).toArray().contains(QStringLiteral("no_pending_input_cleanup")));
    const auto wait = item.value(QStringLiteral("wait")).toObject();
    QCOMPARE(wait.value(QStringLiteral("unit")).toString(), QStringLiteral("ms"));
    QCOMPARE(wait.value(QStringLiteral("maximum")).toInt(), 999999);
    QCOMPARE(wait.value(QStringLiteral("literal_suffix_minimum")).toInt(), 1);
    QVERIFY(wait.value(QStringLiteral("zero_omits_suffix")).toBool());
    const auto burst = item.value(QStringLiteral("burst")).toObject();
    QCOMPARE(burst.value(QStringLiteral("press_default")).toInt(), 40);
    QCOMPARE(burst.value(QStringLiteral("release_default")).toInt(), 20);
    QCOMPARE(item.value(QStringLiteral("repeat")).toObject().value(QStringLiteral("count_maximum")).toInt(), 99999999);
}

void McpControlTest::mappingHelpBoundaries()
{
    const QStringList originals{QStringLiteral("B"), QStringLiteral("A"), QString(), QStringLiteral("A"),
        QStringLiteral("中"), QStringLiteral("a")};
    const QStringList mappings{QStringLiteral("Mouse-L"), QStringLiteral("B")};
    const auto originalCopy = originals;
    const QJsonObject query{{QStringLiteral("topic"), QStringLiteral("original_keys")}, {QStringLiteral("limit"), 2}};
    const auto first = queryMappingHelp(query, originals, mappings);
    QVERIFY(first.ok());
    QCOMPARE(first.page.value(QStringLiteral("total")).toInt(), 4);
    QCOMPARE(first.page.value(QStringLiteral("next_offset")).toInt(), 2);
    QCOMPARE(first.page.value(QStringLiteral("items")).toArray(), (QJsonArray{QStringLiteral("A"), QStringLiteral("B")}));
    auto next = query;
    next.insert(QStringLiteral("offset"), 2);
    const auto last = queryMappingHelp(next, originals, mappings);
    QVERIFY(last.ok());
    QCOMPARE(last.page.value(QStringLiteral("items")).toArray(), (QJsonArray{QStringLiteral("a"), QStringLiteral("中")}));
    QVERIFY(last.page.value(QStringLiteral("next_offset")).isNull());
    next.insert(QStringLiteral("offset"), 4);
    QVERIFY(queryMappingHelp(next, originals, mappings).page.value(QStringLiteral("items")).toArray().isEmpty());
    const auto mapping = queryMappingHelp({{QStringLiteral("topic"), QStringLiteral("mapping_keys")}}, originals, mappings);
    QCOMPARE(mapping.page.value(QStringLiteral("items")).toArray(), (QJsonArray{QStringLiteral("B"), QStringLiteral("Mouse-L")}));
    QCOMPARE(originals, originalCopy);
    for (const QJsonObject &bad : QList<QJsonObject>{
        {{QStringLiteral("unknown"), true}}, {{QStringLiteral("topic"), true}},
        {{QStringLiteral("offset"), true}}, {{QStringLiteral("offset"), 0.5}},
        {{QStringLiteral("offset"), -1}}, {{QStringLiteral("offset"), 2147483648.0}},
        {{QStringLiteral("offset"), 5}}, {{QStringLiteral("limit"), 0}}, {{QStringLiteral("limit"), 101}},
        {{QStringLiteral("limit"), QStringLiteral("1")}}, {{QStringLiteral("limit"), QJsonValue(QJsonValue::Null)}}}) {
        auto arguments = query;
        for (auto it = bad.constBegin(); it != bad.constEnd(); ++it) arguments.insert(it.key(), it.value());
        QCOMPARE(queryMappingHelp(arguments, originals, mappings).errorCode, QStringLiteral("invalid_arguments"));
    }
    QCOMPARE(queryMappingHelp({}, originals, mappings).errorCode, QStringLiteral("invalid_arguments"));
    QCOMPARE(queryMappingHelp({{QStringLiteral("topic"), QStringLiteral("unknown")}}, originals, mappings).errorCode,
        QStringLiteral("unsupported_query"));
    QCOMPARE(queryMappingHelp(query, {}, mappings).errorCode, QStringLiteral("context_busy"));
    QCOMPARE(queryMappingHelp(query, {QString(MaxFramePayload, QLatin1Char('X'))}, {}).errorCode,
        QStringLiteral("result_too_large"));
}

void McpControlTest::sourceReadAdmission()
{
    MappingSourceAccess access;
    QMutexLocker reader(&access.mutex());
    using ReadBasis = MappingSourceAccess::ReadBasis;
    QVERIFY(access.readBasis(0) == ReadBasis::PristineIdle);
    QVERIFY(access.readBasis(1) == ReadBasis::Unavailable);
    QVERIFY(!access.runtimeObserved());
    QCOMPARE(access.runtimeRevision(), quint64(0));
    QSemaphore entered;
    std::thread producer([&] {
        entered.release();
        access.observeRuntimeLifecycle();
    });
    const auto cleanup = qScopeGuard([&] {
        reader.unlock();
        if (producer.joinable()) producer.join();
    });
    QVERIFY(entered.tryAcquire(1, 3000));
    QVERIFY(!access.runtimeObserved());
    QCOMPARE(access.runtimeRevision(), quint64(0));
    reader.unlock();
    producer.join();
    QVERIFY(access.runtimeObserved());
    QCOMPARE(access.runtimeRevision(), quint64(1));
    QVERIFY(access.readBasis(0) == ReadBasis::Unavailable);
    QVERIFY(access.readBasis(1) == ReadBasis::CompletedCleanup);
    access.observeRuntimeLifecycle();
    QCOMPARE(access.runtimeRevision(), quint64(2));
    QVERIFY(access.readBasis(1) == ReadBasis::Unavailable);
    QVERIFY(access.readBasis(2) == ReadBasis::CompletedCleanup);
    reader.relock();
    producer = std::thread([&] {
        entered.release();
        access.observeRuntimeLifecycle();
    });
    QVERIFY(entered.tryAcquire(1, 3000));
    QCOMPARE(access.runtimeRevision(), quint64(2));
    QVERIFY(access.readBasis(2) == ReadBasis::CompletedCleanup);
    reader.unlock();
    producer.join();
    QCOMPARE(access.runtimeRevision(), quint64(3));
    QVERIFY(access.readBasis(2) == ReadBasis::Unavailable);

    MappingSourceAccess queuedAccess;
    QPushButton request;
    bool queuedWriterRan = false;
    QObject::connect(&request, &QPushButton::clicked, &request,
        [&] { queuedAccess.observeRuntimeLifecycle(); }, Qt::DirectConnection);
    QObject::connect(&request, &QPushButton::clicked, &request,
        [&] { queuedWriterRan = true; }, Qt::QueuedConnection);
    request.click();
    QVERIFY(queuedAccess.runtimeObserved());
    QVERIFY(queuedAccess.readBasis(0) == ReadBasis::Unavailable);
    QCOMPARE(queuedAccess.runtimeRevision(), quint64(1));
    QVERIFY(!queuedWriterRan);
    QTRY_VERIFY(queuedWriterRan);
    queuedWriterRan = false;
    request.click();
    QCOMPARE(queuedAccess.runtimeRevision(), quint64(2));
    QVERIFY(queuedAccess.readBasis(1) == ReadBasis::Unavailable);
    QVERIFY(!queuedWriterRan);
    QTRY_VERIFY(queuedWriterRan);
}

void McpControlTest::sourceSnapshotReferences()
{
    const QJsonObject firstRow{{QStringLiteral("fields"), QJsonObject{
        {QStringLiteral("original_key"), QStringLiteral("A")},
        {QStringLiteral("mapping_keys"), QJsonArray{QStringLiteral("B")}},
        {QStringLiteral("KeyMapData_FloatingButton_SyncGroupId"), 7},
        {QStringLiteral("note"), QStringLiteral("中文 日本語 😀")}}}};
    QJsonObject snapshot{{QStringLiteral("source_epoch"), QStringLiteral("1")},
        {QStringLiteral("read_basis"), QStringLiteral("pristine_idle")},
        {QStringLiteral("mapping_load_context"), QJsonObject{{QStringLiteral("generation"), QStringLiteral("1")}}},
        {QStringLiteral("sources"), QJsonArray{
            QJsonObject{{QStringLiteral("name"), QStringLiteral("Tab1")}, {QStringLiteral("kind"), QStringLiteral("normal")},
                {QStringLiteral("rows"), QJsonArray{firstRow, firstRow}}},
            QJsonObject{{QStringLiteral("name"), QStringLiteral("Tab2")}, {QStringLiteral("kind"), QStringLiteral("normal")},
                {QStringLiteral("rows"), QJsonArray{QJsonObject{{QStringLiteral("fields"), QJsonObject{
                    {QStringLiteral("original_key"), QStringLiteral("C")}}}}}}},
            QJsonObject{{QStringLiteral("name"), QStringLiteral("Common")}, {QStringLiteral("kind"), QStringLiteral("common")},
                {QStringLiteral("attached_to_ui"), false}, {QStringLiteral("rows"), QJsonArray{firstRow}}}}}};
    const QJsonObject original = snapshot;
    const QJsonObject list{{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("limit"), 1}};
    const auto query = [&](const QJsonObject &arguments) { return querySourceSnapshot(snapshot, "instance", "pair", 1, arguments); };
    const auto first = query(list);
    QVERIFY(first.ok());
    QCOMPARE(first.page.value(QStringLiteral("total")).toInt(), 3);
    QCOMPARE(first.page.value(QStringLiteral("next_offset")).toInt(), 1);
    const QString revision = first.page.value(QStringLiteral("source_snapshot")).toObject().value(QStringLiteral("revision")).toString();
    QVERIFY(isLowerHex(revision, 64));
    QCOMPARE(query(list).page, first.page);
    auto groupOnly = snapshot;
    auto groupSources = groupOnly.value(QStringLiteral("sources")).toArray();
    auto groupSource = groupSources.first().toObject();
    auto groupRows = groupSource.value(QStringLiteral("rows")).toArray();
    auto groupRow = groupRows.first().toObject();
    auto groupFields = groupRow.value(QStringLiteral("fields")).toObject();
    groupFields.insert(QStringLiteral("KeyMapData_FloatingButton_SyncGroupId"), 8);
    groupRow.insert(QStringLiteral("fields"), groupFields); groupRows[0] = groupRow;
    groupSource.insert(QStringLiteral("rows"), groupRows); groupSources[0] = groupSource;
    groupOnly.insert(QStringLiteral("sources"), groupSources);
    QCOMPARE(querySourceSnapshot(groupOnly, "instance", "pair", 1,
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("source_revision"), revision}}).errorCode,
        QStringLiteral("stale_source"));
    QCOMPARE(snapshot, original);
    QJsonObject rest = list;
    rest.insert(QStringLiteral("source_revision"), revision);
    rest.insert(QStringLiteral("offset"), 1);
    rest.insert(QStringLiteral("limit"), 100);
    const auto tail = query(rest);
    QVERIFY(tail.ok());
    const QJsonArray sources = tail.page.value(QStringLiteral("items")).toArray();
    QCOMPARE(sources.size(), 2);
    QVERIFY(tail.page.value(QStringLiteral("next_offset")).isNull());
    QCOMPARE(sources.at(1).toObject().value(QStringLiteral("kind")).toString(), QStringLiteral("common"));
    QCOMPARE(sources.at(1).toObject().value(QStringLiteral("row_count")).toInt(), 1);
    QJsonObject rows{{QStringLiteral("entity"), QStringLiteral("mappings")}, {QStringLiteral("source_revision"), revision},
        {QStringLiteral("source_tab_ref"), sources.first().toObject().value(QStringLiteral("source_tab_ref"))}};
    const auto nonCurrent = query(rows);
    QVERIFY(nonCurrent.ok());
    QCOMPARE(nonCurrent.page.value(QStringLiteral("items")).toArray().first().toObject()
        .value(QStringLiteral("fields")).toObject().value(QStringLiteral("original_key")).toString(), QStringLiteral("C"));
    rows.insert(QStringLiteral("source_tab_ref"), first.page.value(QStringLiteral("items")).toArray().first().toObject().value(QStringLiteral("source_tab_ref")));
    const auto duplicates = query(rows);
    QVERIFY(duplicates.ok());
    const QJsonArray items = duplicates.page.value(QStringLiteral("items")).toArray();
    QCOMPARE(items.size(), 2);
    QCOMPARE(items.first().toObject().value(QStringLiteral("fields")), items.last().toObject().value(QStringLiteral("fields")));
    QVERIFY(items.first().toObject().value(QStringLiteral("source_ref")) != items.last().toObject().value(QStringLiteral("source_ref")));
    QVERIFY(querySourceSnapshot(snapshot, "new-instance", "pair", 1, rows).errorCode == QStringLiteral("stale_source"));
    QVERIFY(querySourceSnapshot(snapshot, "instance", "new-pair", 1, rows).errorCode == QStringLiteral("stale_source"));
    QVERIFY(querySourceSnapshot(snapshot, "instance", "pair", 2, rows).errorCode == QStringLiteral("stale_source"));
    snapshot.insert(QStringLiteral("source_epoch"), QStringLiteral("2"));
    QCOMPARE(query(rows).errorCode, QStringLiteral("stale_source"));
    snapshot = original;
    snapshot.insert(QStringLiteral("mapping_load_context"), QJsonObject{{QStringLiteral("generation"), QStringLiteral("2")}});
    QCOMPARE(query(rows).errorCode, QStringLiteral("stale_source"));
    snapshot = original;
    QJsonArray changedSources = snapshot.value(QStringLiteral("sources")).toArray();
    QJsonObject changedSource = changedSources.first().toObject();
    changedSource.insert(QStringLiteral("rows"), QJsonArray{QJsonObject{{QStringLiteral("fields"), QJsonObject{
        {QStringLiteral("original_key"), QStringLiteral("Z")}}}}});
    changedSources.replace(0, changedSource);
    snapshot.insert(QStringLiteral("sources"), changedSources);
    QCOMPARE(query(rows).errorCode, QStringLiteral("stale_source"));
    snapshot = original;
    QCOMPARE(query(rows).page, duplicates.page);
    QCOMPARE(snapshot, original);
    snapshot.insert(QStringLiteral("read_basis"), QStringLiteral("completed_cleanup"));
    snapshot.insert(QStringLiteral("cleanup_generation"), QStringLiteral("1"));
    const auto cleaned = query(list);
    QVERIFY(cleaned.ok());
    QCOMPARE(cleaned.page.value(QStringLiteral("source_snapshot")).toObject()
        .value(QStringLiteral("cleanup_generation")).toString(), QStringLiteral("1"));
    QCOMPARE(query(rows).errorCode, QStringLiteral("stale_source"));
    rows.insert(QStringLiteral("source_revision"), cleaned.page.value(QStringLiteral("source_snapshot")).toObject().value(QStringLiteral("revision")));
    rows.insert(QStringLiteral("source_tab_ref"), cleaned.page.value(QStringLiteral("items")).toArray().first().toObject().value(QStringLiteral("source_tab_ref")));
    QVERIFY(query(rows).ok());
    snapshot.insert(QStringLiteral("cleanup_generation"), QStringLiteral("2"));
    QCOMPARE(query(rows).errorCode, QStringLiteral("stale_source"));
}

void McpControlTest::sourceSnapshotBoundaries()
{
    QJsonObject snapshot{{QStringLiteral("sources"), QJsonArray{QJsonObject{{QStringLiteral("rows"), QJsonArray{QJsonObject{}}}}}}};
    const auto query = [&](const QJsonObject &arguments) { return querySourceSnapshot(snapshot, "instance", "pair", 1, arguments); };
    QJsonObject list{{QStringLiteral("entity"), QStringLiteral("mapping_sources")}};
    const auto first = query(list);
    QVERIFY(first.ok());
    const QString revision = first.page.value(QStringLiteral("source_snapshot")).toObject().value(QStringLiteral("revision")).toString();
    const QString source = first.page.value(QStringLiteral("items")).toArray().first().toObject().value(QStringLiteral("source_tab_ref")).toString();
    const QList<QJsonObject> invalid{
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("unknown"), true}},
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("offset"), 1}},
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("limit"), true}},
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("offset"), 0.5}},
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("offset"), -1}},
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("offset"), 2147483648.0}},
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("limit"), 0}},
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("limit"), 101}},
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("source_revision"), QStringLiteral("bad")}},
        {{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("source_tab_ref"), source}},
        {{QStringLiteral("entity"), QStringLiteral("mappings")}},
        {{QStringLiteral("entity"), QStringLiteral("mappings")}, {QStringLiteral("source_revision"), revision}}};
    QVERIFY(validateSourceQueryArguments(list).ok());
    for (const QJsonObject &arguments : invalid) {
        const auto validation = validateSourceQueryArguments(arguments);
        QCOMPARE(validation.errorCode, QStringLiteral("invalid_arguments"));
        QCOMPARE(query(arguments).errorCode, validation.errorCode);
        const auto unavailable = querySourceSnapshot({}, "instance", "pair", 1, arguments);
        QCOMPARE(unavailable.errorCode, validation.errorCode);
        QCOMPARE(unavailable.message, validation.message);
    }
    QCOMPARE(validateSourceQueryArguments({{QStringLiteral("entity"), QStringLiteral("unsupported")}}).errorCode,
        QStringLiteral("unsupported_query"));
    QJsonObject rows{{QStringLiteral("entity"), QStringLiteral("mappings")}, {QStringLiteral("source_revision"), revision}};
    for (const QString &suffix : {QStringLiteral("-1"), QStringLiteral("01"), QStringLiteral("2147483648"), QStringLiteral("x")}) {
        rows.insert(QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:") + suffix);
        QCOMPARE(validateSourceQueryArguments(rows).errorCode, QStringLiteral("invalid_arguments"));
        QCOMPARE(query(rows).errorCode, QStringLiteral("invalid_arguments"));
    }
    rows.insert(QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:9"));
    QVERIFY(validateSourceQueryArguments(rows).ok()); // The index bound requires the real snapshot.
    QCOMPARE(query(rows).errorCode, QStringLiteral("invalid_arguments"));
    rows.insert(QStringLiteral("source_tab_ref"), source);
    QVERIFY(validateSourceQueryArguments(rows).ok());
    QJsonObject mismatch = rows;
    mismatch.insert(QStringLiteral("source_revision"), QString(64, QLatin1Char('0')));
    QCOMPARE(validateSourceQueryArguments(mismatch).errorCode, QStringLiteral("stale_source"));
    rows.insert(QStringLiteral("offset"), 2);
    QCOMPARE(query(rows).errorCode, QStringLiteral("invalid_arguments"));
    rows.insert(QStringLiteral("offset"), 1);
    QVERIFY(query(rows).ok());
    QVERIFY(query(rows).page.value(QStringLiteral("items")).toArray().isEmpty());
    QCOMPARE(querySourceSnapshot({}, "instance", "pair", 1, list).errorCode, QStringLiteral("context_busy"));
    snapshot.insert(QStringLiteral("oversized"), QString(MaxSourceSnapshotBytes, QLatin1Char('x')));
    QCOMPARE(query(list).errorCode, QStringLiteral("result_too_large"));
    snapshot.remove(QStringLiteral("oversized"));
    snapshot.insert(QStringLiteral("sources"), QJsonArray{QJsonObject{{QStringLiteral("rows"), QJsonArray{
        QJsonObject{{QStringLiteral("note"), QString(MaxFramePayload, QLatin1Char('x'))}}}}}});
    const auto largeList = query(list);
    QVERIFY(largeList.ok());
    rows.insert(QStringLiteral("offset"), 0);
    rows.insert(QStringLiteral("source_revision"), largeList.page.value(QStringLiteral("source_snapshot")).toObject().value(QStringLiteral("revision")));
    rows.insert(QStringLiteral("source_tab_ref"), largeList.page.value(QStringLiteral("items")).toArray().first().toObject().value(QStringLiteral("source_tab_ref")));
    QCOMPARE(query(rows).errorCode, QStringLiteral("item_too_large"));
    const QJsonObject halfFrame{{QStringLiteral("note"), QString(MaxFramePayload / 2, QLatin1Char('x'))}};
    snapshot.insert(QStringLiteral("sources"), QJsonArray{QJsonObject{
        {QStringLiteral("rows"), QJsonArray{halfFrame, halfFrame, halfFrame}}}});
    const auto boundedList = query(list);
    QVERIFY(boundedList.ok());
    rows.insert(QStringLiteral("source_revision"), boundedList.page.value(QStringLiteral("source_snapshot")).toObject().value(QStringLiteral("revision")));
    rows.insert(QStringLiteral("source_tab_ref"), boundedList.page.value(QStringLiteral("items")).toArray().first().toObject().value(QStringLiteral("source_tab_ref")));
    for (int offset = 0; offset < 3; ++offset) {
        rows.insert(QStringLiteral("offset"), offset);
        const auto page = query(rows);
        QVERIFY(page.ok());
        QCOMPARE(page.page.value(QStringLiteral("items")).toArray().size(), 1);
        QVERIFY(QJsonDocument(page.page).toJson(QJsonDocument::Compact).size() <= MaxFramePayload - 256);
        if (offset < 2) QCOMPARE(page.page.value(QStringLiteral("next_offset")).toInt(), offset + 1);
        else QVERIFY(page.page.value(QStringLiteral("next_offset")).isNull());
    }
}

void McpControlTest::sourceSnapshotEncodingBoundaries()
{
    QJsonObject snapshot{{QStringLiteral("sources"), QJsonArray{QJsonObject{{QStringLiteral("rows"), QJsonArray()}}}},
        {QStringLiteral("padding"), QString()}};
    const int emptySize = QJsonDocument(snapshot).toJson(QJsonDocument::Compact).size();
    const QJsonObject arguments{{QStringLiteral("entity"), QStringLiteral("mapping_sources")}};
    for (int delta : {-1, 0, 1}) {
        snapshot.insert(QStringLiteral("padding"), QString(MaxSourceSnapshotBytes - emptySize + delta, QLatin1Char('x')));
        QCOMPARE(QJsonDocument(snapshot).toJson(QJsonDocument::Compact).size(), MaxSourceSnapshotBytes + delta);
        for (const QString &instance : {QStringLiteral("instance"), QStringLiteral("中文 日本語 😀 \"\\\n")}) {
            const QString pair = QStringLiteral("pair \"\\\n");
            const auto result = querySourceSnapshot(snapshot, instance, pair, 42, arguments);
            if (delta > 0) {
                QCOMPARE(result.errorCode, QStringLiteral("result_too_large"));
            } else {
                QVERIFY(result.ok());
                const QJsonObject binding{{QStringLiteral("instance"), instance}, {QStringLiteral("pair_id"), pair},
                    {QStringLiteral("permission_epoch"), QStringLiteral("42")}, {QStringLiteral("snapshot"), snapshot}};
                const QString expected = QString::fromLatin1(QCryptographicHash::hash(
                    QJsonDocument(binding).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
                QCOMPARE(result.page.value(QStringLiteral("source_snapshot")).toObject()
                    .value(QStringLiteral("revision")).toString(), expected);
            }
        }
    }
}

void McpControlTest::sourceSnapshotQueryCost_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<bool>("mappingPage");
    for (int count : {1, 100, 1000}) {
        for (bool page : {false, true}) {
            QTest::newRow(qPrintable(QStringLiteral("%1_%2").arg(count).arg(page ? "mapping" : "sources"))) << count << page;
        }
    }
}

void McpControlTest::sourceSnapshotQueryCost()
{
    QFETCH(int, rowCount);
    QFETCH(bool, mappingPage);
    QJsonObject fields{{QStringLiteral("original_key"), QStringLiteral("A")},
        {QStringLiteral("mapping_keys"), QJsonArray{QStringLiteral("B")}},
        {QStringLiteral("note"), QString(512, QChar(0x4e2d))}};
    for (int index = 0; index < 48; ++index) fields.insert(QStringLiteral("parameter_%1").arg(index), index);
    const QJsonObject row{{QStringLiteral("fields"), fields}};
    QJsonArray rows;
    for (int index = 0; index < rowCount; ++index) rows.append(row);
    const QJsonObject snapshot{{QStringLiteral("sources"), QJsonArray{QJsonObject{{QStringLiteral("rows"), rows}}}}};
    qInfo() << "Query helper snapshot bytes:" << QJsonDocument(snapshot).toJson(QJsonDocument::Compact).size();
    QJsonObject arguments{{QStringLiteral("entity"), QStringLiteral("mapping_sources")}, {QStringLiteral("limit"), 1}};
    const auto first = querySourceSnapshot(snapshot, "instance", "pair", 1, arguments);
    QVERIFY(first.ok());
    if (mappingPage) {
        arguments.insert(QStringLiteral("entity"), QStringLiteral("mappings"));
        arguments.insert(QStringLiteral("source_revision"), first.page.value(QStringLiteral("source_snapshot")).toObject().value(QStringLiteral("revision")));
        arguments.insert(QStringLiteral("source_tab_ref"), first.page.value(QStringLiteral("items")).toArray().first().toObject().value(QStringLiteral("source_tab_ref")));
    }
    SourceQueryResult result;
    QBENCHMARK { result = querySourceSnapshot(snapshot, "instance", "pair", 1, arguments); }
    QVERIFY(result.ok());
    QCOMPARE(result.page.value(QStringLiteral("items")).toArray().size(), 1);
}

void McpControlTest::initTestCase()
{
    wchar_t desktop[256]{};
    DWORD needed = 0;
    QVERIFY(GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, desktop, sizeof(desktop), &needed));
    const QString actualDesktop = QString::fromWCharArray(desktop);
    qInfo().noquote() << "Actual desktop:" << actualDesktop;
    if (!qEnvironmentVariableIsEmpty("QKM_TEST_DESKTOP")) QCOMPARE(actualDesktop, qEnvironmentVariable("QKM_TEST_DESKTOP"));
    QVERIFY2(QFileInfo::exists(qEnvironmentVariable("QKM_IPC_TEST_EXE")), "Set QKM_IPC_TEST_EXE to the matching built IPC test executable.");
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    QVERIFY(own.ok());
    QCOMPARE(QFileInfo(queryProcessExecutable(own.identity)).canonicalFilePath(), QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath());
    ProcessIdentity changed = own.identity;
    ++changed.creationTime;
    QVERIFY(queryProcessExecutable(changed).isEmpty());
    QVERIFY(queryProcessExecutable({}).isEmpty());
}

void McpControlTest::defaultOffAndLanguage_data()
{
    QTest::addColumn<int>("language");
    QTest::newRow("Chinese") << 0;
    QTest::newRow("English") << 1;
    QTest::newRow("Japanese") << 2;
}

void McpControlTest::defaultOffAndLanguage()
{
    QFETCH(int, language);
    QTranslator translator;
    QVERIFY(installUiTranslation(translator, language));
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/control-XXXXXX"));
    QVERIFY(root.isValid());
    ControlDialog dialog(root.path() + QStringLiteral("/private"), "test-control");
    dialog.retranslateUi();
    QVERIFY(!dialog.server().isEnabled());
    QVERIFY(!QFileInfo::exists(root.path() + QStringLiteral("/private")));
    const QString source = QStringLiteral("Agent / MCP Control");
    const QString translated = translator.translate("QkmAutomation::ControlDialog", "Agent / MCP Control");
    if (language != 1) QVERIFY2(!translated.isEmpty(), "The selected QM does not contain the MCP title.");
    QCOMPARE(dialog.windowTitle(), translated.isEmpty() ? source : translated);
    QCOMPARE(ControlDialog::title(), dialog.windowTitle());
    QVERIFY(!widget<QCheckBox>(dialog, "mcpEnabled")->isChecked());
    QVERIFY(!widget<QPushButton>(dialog, "mcpApprove")->autoDefault());
    QVERIFY(!widget<QPushButton>(dialog, "mcpApprove")->isDefault());
    const auto labels = dialog.findChildren<QLabel *>();
    for (const QLabel *label : labels) QCOMPARE(label->textFormat(), Qt::PlainText);
}

void McpControlTest::startupChoice_data()
{
    defaultOffAndLanguage_data();
}

void McpControlTest::startupChoice()
{
    QFETCH(int, language);
    QTranslator translator;
    QVERIFY(installUiTranslation(translator, language));
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/startup-XXXXXX"));
    QVERIFY(root.isValid());
    const QString server = root.path() + QStringLiteral("/server"), startup = root.path() + QStringLiteral("/choice");
    const auto prepare = [](ControlDialog &dialog) {
        if (configurationOnlyDemo()) QVERIFY(dialog.server().setDemoConfiguration(QStringLiteral("Demo136Seed")));
        fixture(dialog.server());
        dialog.restoreStartupChoice();
    };
    const auto cold = [&](bool enabled, bool wrongMode = false) {
        QProcess child;
        auto environment = QProcessEnvironment::systemEnvironment();
        if (wrongMode) environment.insert(QStringLiteral("QKM_TEST_CONFIGURATION_ONLY"), configurationOnlyDemo() ? QStringLiteral("0") : QStringLiteral("1"));
        child.setProcessEnvironment(environment);
        child.setWorkingDirectory(root.path());
        child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--startup-choice-probe"), server, startup,
            enabled ? QStringLiteral("1") : QStringLiteral("0"), wrongMode ? QStringLiteral("1") : QStringLiteral("0")});
        QVERIFY(child.waitForStarted(3000));
        QVERIFY(child.waitForFinished(10000));
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QCOMPARE(child.exitCode(), 0);
        const auto result = QJsonDocument::fromJson(child.readAllStandardOutput()).object();
        QCOMPARE(result.value(QStringLiteral("desktop")).toString(), qEnvironmentVariable("QKM_TEST_DESKTOP"));
        QCOMPARE(result.value(QStringLiteral("enabled")).toBool(), enabled);
    };
    {
        ControlDialog dialog(server, "startup136", nullptr, startup);
        prepare(dialog);
        QVERIFY(!dialog.server().isEnabled());
        QVERIFY(!QFileInfo::exists(startup));
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        auto *save = widget<QPushButton>(dialog, "mcpSaveStartup");
        QVERIFY(!save->autoDefault());
        QCOMPARE(save->focusPolicy(), Qt::NoFocus);
        QCOMPARE(save->text(), ControlDialog::tr("Save startup choice"));
        QCOMPARE(widget<QLabel>(dialog, "mcpStartupStatus")->text(), ControlDialog::tr("Startup choice: not saved (default: disabled)."));
        clickCheckBox(dialog, "mcpEnabled");
        QVERIFY(dialog.server().isEnabled());
        QVERIFY(!QFileInfo::exists(startup));
        QTest::mouseClick(save, Qt::LeftButton);
        QCOMPARE(widget<QLabel>(dialog, "mcpStartupStatus")->text(), ControlDialog::tr("Saved startup choice: enabled."));
        clickCheckBox(dialog, "mcpEnabled");
        QVERIFY(!dialog.server().isEnabled());
        // An unsaved runtime change cannot alter the remembered startup choice.
    }
    cold(true);
    cold(false, true);
    {
        ControlDialog dialog(server, "startup136", nullptr, startup);
        prepare(dialog);
        QVERIFY(dialog.server().isEnabled());
        const QString firstInstance = dialog.server().instanceId();
        dialog.retranslateUi();
        dialog.restoreStartupChoice();
        QCOMPARE(dialog.server().instanceId(), firstInstance);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        clickCheckBox(dialog, "mcpEnabled");
        QTest::mouseClick(widget<QPushButton>(dialog, "mcpSaveStartup"), Qt::LeftButton);
        QCOMPARE(widget<QLabel>(dialog, "mcpStartupStatus")->text(), ControlDialog::tr("Saved startup choice: disabled."));
    }
    cold(false);
}

void McpControlTest::startupChoiceFailures()
{
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/startup-fail-XXXXXX"));
    QVERIFY(root.isValid());
    const QString server = root.path() + QStringLiteral("/server"), startup = root.path() + QStringLiteral("/choice");
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    QVERIFY(own.ok());
    ControlDialog dialog(server, "startup136", nullptr, startup);
    if (configurationOnlyDemo()) QVERIFY(dialog.server().setDemoConfiguration(QStringLiteral("Demo136Seed")));
    fixture(dialog.server());
    dialog.restoreStartupChoice();
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    clickCheckBox(dialog, "mcpEnabled");
    QVERIFY(dialog.server().isEnabled());
    auto *save = widget<QPushButton>(dialog, "mcpSaveStartup");
    QTest::mouseClick(save, Qt::LeftButton);
    QFile saved(QDir(startup).filePath(QStringLiteral("startup.json")));
    QVERIFY(saved.open(QIODevice::ReadOnly));
    const QByteArray original = saved.readAll();
    saved.close();
    clickCheckBox(dialog, "mcpEnabled");
    {
        Storage::DirectoryLease held;
        QVERIFY(held.acquire(startup, own.identity.userSid, false));
        QTest::mouseClick(save, Qt::LeftButton);
        QVERIFY(widget<QLabel>(dialog, "mcpStartupStatus")->text().contains(QStringLiteral("busy")));
    }
    {
        HANDLE held = Storage::privateFile(saved.fileName(), own.identity.userSid);
        QVERIFY(held != INVALID_HANDLE_VALUE);
        const auto close = qScopeGuard([held]() { CloseHandle(held); });
        QTest::mouseClick(save, Qt::LeftButton);
        QVERIFY(widget<QLabel>(dialog, "mcpStartupStatus")->text().contains(QStringLiteral("io_error")));
    }
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(saved.readAll(), original);
    saved.close();
    {
        AutomationSessionServer blocker(server, "startup136");
        if (configurationOnlyDemo()) QVERIFY(blocker.setDemoConfiguration(QStringLiteral("Demo136Seed")));
        QVERIFY(blocker.enable());
        ControlDialog failed(server, "startup136", nullptr, startup);
        if (configurationOnlyDemo()) QVERIFY(failed.server().setDemoConfiguration(QStringLiteral("Demo136Seed")));
        failed.restoreStartupChoice();
        QVERIFY(!failed.server().isEnabled());
        QVERIFY(widget<QLabel>(failed, "mcpStartupStatus")->text().contains(ControlDialog::tr("MCP could not be restored at startup.\nThe saved choice remains enabled. Check the current status.")));
        failed.show();
        QVERIFY(QTest::qWaitForWindowExposed(&failed));
        QVERIFY(widget<QLabel>(failed, "mcpStatus")->text().contains(QStringLiteral("pair_store_busy")));
        QVERIFY(widget<QLabel>(failed, "mcpStatus")->text().contains(ControlDialog::tr(" Another instance owns this pairing store; manage it there or disable its interface first.")));
        blocker.disable();
        failed.restoreStartupChoice();
        QVERIFY(!failed.server().isEnabled());
    }
    QVERIFY(saved.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(saved.write("{"), qint64(1));
    saved.close();
    ControlDialog broken(server, "startup136", nullptr, startup);
    broken.restoreStartupChoice();
    QVERIFY(!broken.server().isEnabled());
    QVERIFY(widget<QLabel>(broken, "mcpStartupStatus")->text().contains(QStringLiteral("invalid_data")));
    broken.show();
    QVERIFY(QTest::qWaitForWindowExposed(&broken));
    QTest::mouseClick(widget<QPushButton>(broken, "mcpSaveStartup"), Qt::LeftButton);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(saved.readAll(), QByteArray("{"));
}

void McpControlTest::advancedEntryKeyboard()
{
    QDialog dialog;
    Ui::QGeneralAdvancedDialog form;
    form.setupUi(&dialog);
    QSignalSpy clicked(form.mcpControlButton, &QPushButton::clicked);
    QCOMPARE(form.mcpControlButton->focusPolicy(), Qt::NoFocus);
    QVERIFY(!form.mcpControlButton->autoDefault());
    QVERIFY(!form.mcpControlButton->isDefault());
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto *edit = form.fullscreenGlobalMappingProcessLineEdit;
    edit->setFocus();
    QCOMPARE(dialog.focusWidget(), edit);
    QTest::keyClick(edit, Qt::Key_Return);
    QTest::keyClick(edit, Qt::Key_Space);
    QCOMPARE(clicked.count(), 0);
    // The existing form uses ClickFocus/NoFocus, so provide an independent Tab stop.
    QLineEdit tabTarget(&dialog);
    tabTarget.show();
    QWidget *previous = dialog.focusWidget();
    bool focusMoved = false;
    for (int index = 0; index < 20; ++index) {
        QTest::keyClick(&dialog, Qt::Key_Tab);
        QVERIFY(dialog.focusWidget() != form.mcpControlButton);
        focusMoved |= dialog.focusWidget() != previous;
        previous = dialog.focusWidget();
    }
    QVERIFY(focusMoved);
    QTest::mouseClick(form.mcpControlButton, Qt::LeftButton);
    QCOMPARE(clicked.count(), 1);
    QVERIFY(dialog.focusWidget() != form.mcpControlButton);
    QTest::keyClick(&dialog, Qt::Key_Space);
    QTest::keyClick(&dialog, Qt::Key_Return);
    QCOMPARE(clicked.count(), 1);
}

void McpControlTest::controlFocusPolicy()
{
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/focus-XXXXXX"));
    QVERIFY(root.isValid());
    ControlDialog dialog(root.path() + QStringLiteral("/server"), "test-focus");
    for (const auto *button : dialog.findChildren<QPushButton *>()) {
        QCOMPARE(button->focusPolicy(), Qt::NoFocus);
        QVERIFY(!button->autoDefault());
        QVERIFY(!button->isDefault());
    }
    for (const auto *check : dialog.findChildren<QCheckBox *>()) QCOMPARE(check->focusPolicy(), Qt::NoFocus);
    for (const auto *label : dialog.findChildren<QLabel *>()) QCOMPARE(label->focusPolicy(), Qt::ClickFocus);
    for (const auto *edit : dialog.findChildren<QLineEdit *>()) QCOMPARE(edit->focusPolicy(), Qt::ClickFocus);
    QCOMPARE(widget<QListWidget>(dialog, "mcpPairings")->focusPolicy(), Qt::ClickFocus);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto *connection = widget<QLineEdit>(dialog, "mcpConnection");
    const QString value = QStringLiteral("Copyable connection path");
    connection->setText(value);
    QTest::mouseClick(connection, Qt::LeftButton);
    QCOMPARE(dialog.focusWidget(), connection);
    QVERIFY(connection->isReadOnly());
    QTest::keyClick(connection, Qt::Key_A, Qt::ControlModifier);
    QCOMPARE(connection->selectedText(), value);
    auto *enabled = widget<QCheckBox>(dialog, "mcpEnabled");
    QSignalSpy toggled(enabled, &QCheckBox::toggled);
    clickCheckBox(dialog, "mcpEnabled");
    QVERIFY(dialog.server().isEnabled());
    QCOMPARE(dialog.focusWidget(), connection);
    QTest::keyClick(connection, Qt::Key_Space);
    QTest::keyClick(connection, Qt::Key_Return);
    QCOMPARE(toggled.count(), 1);
    // A separate Tab stop proves navigation works without changing any product control.
    QLineEdit tabTarget(&dialog);
    tabTarget.show();
    QTest::keyClick(connection, Qt::Key_Tab);
    QCOMPARE(dialog.focusWidget(), &tabTarget);
    for (int index = 0; index < 5; ++index) {
        QTest::keyClick(&tabTarget, Qt::Key_Tab);
        QCOMPARE(dialog.focusWidget(), &tabTarget);
    }
    QSignalSpy closed(&dialog, &QDialog::finished);
    QTest::mouseClick(widget<QPushButton>(dialog, "mcpClose"), Qt::LeftButton);
    QCOMPARE(closed.count(), 1);
    QVERIFY(!dialog.isVisible());
    QVERIFY(dialog.server().isEnabled());
}

void McpControlTest::languageRefreshPreservesState()
{
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/language-XXXXXX"));
    QVERIFY(root.isValid());
    const QString directory = root.path() + QStringLiteral("/server");
    PairCredentialStore store(directory, "test-language");
    const PairCredential credential{QString(32, QLatin1Char('a')), QStringLiteral("Existing client"),
        1, Read | Configure, QByteArray(AuthSecretBytes, 'k')};
    PairStoreError error;
    QVERIFY(store.save({credential}, error));
    QTranslator translator;
    ControlDialog dialog(directory, "test-language");
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto *list = widget<QListWidget>(dialog, "mcpPairings");
    list->setCurrentRow(0);
    clickCheckBox(dialog, "mcpEnabled");
    Client child;
    QVERIFY(child.launch(dialog.server(), root.path() + QStringLiteral("/client"), QStringLiteral("pair-rejected")));
    QTRY_VERIFY_WITH_TIMEOUT(!widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty(), 3000);
    clickCheckBox(dialog, "mcpConfigure");
    clickCheckBox(dialog, "mcpRunMapping");
    const QString executable = widget<QLineEdit>(dialog, "mcpClientExecutable")->text();
    const QString discovery = dialog.server().discoveryPath();
    QSignalSpy enabledChanged(&dialog.server(), &AutomationSessionServer::enabledChanged);
    for (int language : {0, 2, 1}) {
        QVERIFY(installUiTranslation(translator, language));
        dialog.retranslateUi();
        QVERIFY(dialog.server().isEnabled());
        QCOMPARE(enabledChanged.count(), 0);
        QCOMPARE(dialog.server().discoveryPath(), discovery);
        QCOMPARE(dialog.server().pairings().size(), 1);
        QCOMPARE(dialog.server().pairings().first().scopes, quint32(Read | Configure));
        QCOMPARE(list->selectedItems().size(), 1);
        QCOMPARE(list->selectedItems().first()->data(Qt::UserRole).toString(), credential.pairId);
        QVERIFY(widget<QCheckBox>(dialog, "mcpConfigure")->isChecked());
        QVERIFY(widget<QCheckBox>(dialog, "mcpRunMapping")->isChecked());
        QVERIFY(widget<QPushButton>(dialog, "mcpApprove")->isEnabled());
        QCOMPARE(widget<QLineEdit>(dialog, "mcpClientExecutable")->text(), executable);
        QVERIFY(widget<QLabel>(dialog, "mcpPairRequest")->text().contains(QString::number(child.processId())));
        QCOMPARE(child.state(), QProcess::Running);
    }
    QTest::mouseClick(widget<QPushButton>(dialog, "mcpReject"), Qt::LeftButton);
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
    QVERIFY(widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty());
}

void McpControlTest::pairingFromWidgets_data()
{
    QTest::addColumn<bool>("writable");
    QTest::addColumn<quint32>("windowScopes");
    QTest::newRow("read-only") << false << quint32(0);
    QTest::newRow("configure-run") << true << quint32(0);
    QTest::newRow("show-only") << false << quint32(ShowWindows);
    QTest::newRow("capture-only") << false << quint32(CaptureWindows);
    QTest::newRow("both-windows") << false << quint32(ShowWindows | CaptureWindows);
}

void McpControlTest::pairingFromWidgets()
{
    QFETCH(bool, writable);
    QFETCH(quint32, windowScopes);
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/control-XXXXXX"));
    QVERIFY(root.isValid());
    ControlDialog dialog(root.path() + QStringLiteral("/server"), "test-control");
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    fixture(dialog.server());
    clickCheckBox(dialog, "mcpEnabled");
    QVERIFY(dialog.server().isEnabled());
    QVERIFY(QFileInfo::exists(dialog.server().discoveryPath()));
    InstanceDescriptor descriptor;
    DiscoveryError discoveryError;
    QVERIFY(readInstanceDiscovery(QFileInfo(dialog.server().discoveryPath()).absolutePath(),
        QCoreApplication::applicationFilePath(), descriptor, discoveryError));
    QCOMPARE(descriptor.processId, quint32(GetCurrentProcessId()));
    Client child;
    QVERIFY(child.launch(dialog.server(), root.path() + QStringLiteral("/client"), QStringLiteral("pair")));
    QTRY_VERIFY_WITH_TIMEOUT(!widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty(), 3000);
    QVERIFY(widget<QLabel>(dialog, "mcpPairRequest")->text().contains(QString::number(child.processId())));
    const auto *executable = widget<QLineEdit>(dialog, "mcpClientExecutable");
    QVERIFY(executable->isReadOnly());
    QCOMPARE(QFileInfo(executable->text()).canonicalFilePath(), QFileInfo(qEnvironmentVariable("QKM_IPC_TEST_EXE")).canonicalFilePath());
    QVERIFY(!widget<QCheckBox>(dialog, "mcpConfigure")->isChecked());
    QVERIFY(!widget<QCheckBox>(dialog, "mcpRunMapping")->isChecked());
    QVERIFY(!widget<QCheckBox>(dialog, "mcpShowWindows")->isChecked());
    QVERIFY(!widget<QCheckBox>(dialog, "mcpCaptureWindows")->isChecked());
    if (windowScopes & ShowWindows) clickCheckBox(dialog, "mcpShowWindows");
    if (windowScopes & CaptureWindows) clickCheckBox(dialog, "mcpCaptureWindows");
    if (writable) {
        clickCheckBox(dialog, "mcpConfigure");
        clickCheckBox(dialog, "mcpRunMapping");
    }
    QTest::mouseClick(widget<QPushButton>(dialog, "mcpApprove"), Qt::LeftButton);
    QVERIFY(!widget<QPushButton>(dialog, "mcpApprove")->isEnabled());
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
    QTRY_COMPARE(widget<QListWidget>(dialog, "mcpPairings")->count(), 1);
    const auto pairs = dialog.server().pairings();
    QCOMPARE(pairs.at(0).scopes, quint32(writable ? Read | Configure | RunMapping : Read) | windowScopes);
    QVERIFY(dialog.close());
    QVERIFY(dialog.server().isEnabled());
    auto *list = widget<QListWidget>(dialog, "mcpPairings");
    list->setCurrentRow(0);
    QTest::mouseClick(widget<QPushButton>(dialog, "mcpRevoke"), Qt::LeftButton);
    QVERIFY(dialog.server().pairings().isEmpty());
    QCOMPARE(list->count(), 0);
    clickCheckBox(dialog, "mcpEnabled");
    QVERIFY(!dialog.server().isEnabled());
    QVERIFY(!QFileInfo::exists(dialog.server().discoveryPath()));
}

void McpControlTest::windowPermissions_data()
{
    QTest::addColumn<quint32>("existing");
    QTest::addColumn<quint32>("additional");
    QTest::addColumn<bool>("cancel");
    QTest::addColumn<bool>("failSave");
    QTest::newRow("show") << quint32(0) << quint32(ShowWindows) << false << false;
    QTest::newRow("capture") << quint32(0) << quint32(CaptureWindows) << false << false;
    QTest::newRow("both") << quint32(0) << quint32(ShowWindows | CaptureWindows) << false << false;
    QTest::newRow("cancel") << quint32(0) << quint32(ShowWindows | CaptureWindows) << true << false;
    QTest::newRow("unchanged") << quint32(0) << quint32(0) << false << false;
    QTest::newRow("append-capture") << quint32(ShowWindows) << quint32(CaptureWindows) << false << false;
    QTest::newRow("already-granted") << quint32(ShowWindows | CaptureWindows) << quint32(0) << false << false;
    QTest::newRow("save-failed") << quint32(0) << quint32(ShowWindows) << false << true;
}

void McpControlTest::windowPermissions()
{
    QFETCH(quint32, existing);
    QFETCH(quint32, additional);
    QFETCH(bool, cancel);
    QFETCH(bool, failSave);
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/window-permissions-XXXXXX"));
    QVERIFY(root.isValid());
    const QString directory = root.path() + QStringLiteral("/server");
    PairCredentialStore store(directory, "test-window-permissions");
    const PairCredential first{QString(32, QLatin1Char('a')), QStringLiteral("First client"),
        1, Read, QByteArray(AuthSecretBytes, 'k')};
    const PairCredential selected{QString(32, QLatin1Char('b')), QStringLiteral("<b>演示客户端</b> 日本語"),
        7, quint32(127) | existing, QByteArray(AuthSecretBytes, 'l')};
    PairStoreError error;
    QVERIFY(store.save({first, selected}, error));
    QFile saved(store.filePath());
    QVERIFY(saved.open(QIODevice::ReadOnly));
    const QByteArray before = saved.readAll();
    saved.close();
    const QString nativePath = QDir::toNativeSeparators(store.filePath());
    const auto restore = qScopeGuard([&]() {
        SetFileAttributesW(reinterpret_cast<LPCWSTR>(nativePath.utf16()), FILE_ATTRIBUTE_NORMAL);
    });
    ControlDialog dialog(directory, "test-window-permissions");
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto *list = widget<QListWidget>(dialog, "mcpPairings");
    auto *button = widget<QPushButton>(dialog, "mcpWindowPermissions");
    auto *approval = widget<QDialog>(dialog, "mcpWindowApproval");
    list->setCurrentRow(0);
    QVERIFY(!button->isEnabled());
    clickCheckBox(dialog, "mcpEnabled");
    list->clearSelection();
    QVERIFY(!button->isEnabled());
    QTest::mouseClick(button, Qt::LeftButton);
    QVERIFY(!approval->isVisible());
    list->setCurrentRow(0);
    list->selectionModel()->select(list->model()->index(1, 0), QItemSelectionModel::ClearAndSelect);
    QCOMPARE(list->currentRow(), 0);
    QVERIFY(button->isEnabled());
    if (failSave) QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(nativePath.utf16()), FILE_ATTRIBUTE_READONLY));
    QTranslator translator;
    QTimer::singleShot(0, &dialog, [&]() {
        const auto close = qScopeGuard([&]() { approval->reject(); });
        QVERIFY(approval->isVisible());
        auto *show = widget<QCheckBox>(*approval, "mcpWindowShow");
        auto *capture = widget<QCheckBox>(*approval, "mcpWindowCapture");
        QCOMPARE(show->isChecked(), bool(existing & ShowWindows));
        QCOMPARE(capture->isChecked(), bool(existing & CaptureWindows));
        QCOMPARE(show->isEnabled(), !(existing & ShowWindows));
        QCOMPARE(capture->isEnabled(), !(existing & CaptureWindows));
        if (additional & ShowWindows) clickCheckBox(*approval, "mcpWindowShow");
        if (additional & CaptureWindows) clickCheckBox(*approval, "mcpWindowCapture");
        for (int language : {0, 2, 1}) {
            QVERIFY(installUiTranslation(translator, language));
            dialog.retranslateUi();
            const auto *label = widget<QLabel>(*approval, "mcpWindowPair");
            QCOMPARE(label->textFormat(), Qt::PlainText);
            QCOMPARE(label->text(), ControlDialog::tr("Selected pairing: %1\nPair ID: %2").arg(selected.clientName, selected.pairId));
            QCOMPARE(label->text().count(QLatin1Char('\n')), 1);
            QCOMPARE(show->isChecked(), bool((existing | additional) & ShowWindows));
            QCOMPARE(capture->isChecked(), bool((existing | additional) & CaptureWindows));
            QCOMPARE(dialog.server().pairings().at(1).permissionEpoch, quint64(7));
            for (const auto *push : approval->findChildren<QPushButton *>()) {
                QCOMPARE(push->focusPolicy(), Qt::NoFocus);
                QVERIFY(!push->autoDefault());
                QVERIFY(!push->isDefault());
            }
        }
        QTest::mouseClick(widget<QPushButton>(*approval, cancel ? "mcpWindowClose" : "mcpWindowApprove"), Qt::LeftButton);
    });
    QTest::mouseClick(button, Qt::LeftButton);
    QVERIFY(!approval->isVisible());
    QList<PairCredential> records;
    QVERIFY(store.load(records, error));
    QCOMPARE(records.size(), 2);
    QCOMPARE(records.first().pairId, first.pairId);
    QCOMPARE(records.first().scopes, first.scopes);
    QCOMPARE(records.first().permissionEpoch, first.permissionEpoch);
    QVERIFY(records.first().secret == first.secret);
    const bool changed = additional && !cancel && !failSave;
    QCOMPARE(records.at(1).pairId, selected.pairId);
    QCOMPARE(records.at(1).scopes, selected.scopes | (changed ? additional : quint32(0)));
    QCOMPARE(records.at(1).permissionEpoch, quint64(changed ? 8 : 7));
    QVERIFY(records.at(1).secret == selected.secret);
    QCOMPARE(dialog.server().isEnabled(), !failSave);
    if (!changed) {
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), before);
    }
    if (failSave) QVERIFY(widget<QLabel>(dialog, "mcpStatus")->text().contains(
        ControlDialog::tr("The window permission update did not complete. MCP is disabled if saving failed.")));
    dialog.server().disable();
}

void McpControlTest::windowObservation()
{
    QWidget main;
    auto *item = new QDialog(&main);
    WindowAccess access(&main, item);
    const QString instance(32, QLatin1Char('a')), pair(32, QLatin1Char('b'));
    const QJsonObject arguments{{QStringLiteral("entity"), QStringLiteral("windows")}};
    const auto query = [&]() { return access.query(instance, pair, 1, arguments); };
    QCOMPARE(main.internalWinId(), WId(0));
    QCOMPARE(item->internalWinId(), WId(0));
    auto result = query();
    QVERIFY(result.ok());
    QCOMPARE(main.internalWinId(), WId(0));
    QCOMPARE(item->internalWinId(), WId(0));
    QCOMPARE(result.page.value(QStringLiteral("total")).toInt(), 2);
    QVERIFY(result.page.value(QStringLiteral("private_background_available")).isBool());
    QCOMPARE(result.page.value(QStringLiteral("private_background_available")).toBool(), privateBackgroundDesktopAvailable());
    QVERIFY(result.page.value(QStringLiteral("items")).toArray().at(0).toObject().value(QStringLiteral("window_ref")).isNull());
    const auto target = result.page.value(QStringLiteral("target")).toObject();
    QCOMPARE(target.value(QStringLiteral("instance")).toString(), instance);
    QCOMPARE(target.value(QStringLiteral("pid")).toString(), QString::number(GetCurrentProcessId()));
    QCOMPARE(QFileInfo(target.value(QStringLiteral("executable")).toString()).canonicalFilePath(),
        QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath());
    QVERIFY(!target.value(QStringLiteral("desktop")).toString().isEmpty());
    if (!qEnvironmentVariableIsEmpty("QKM_TEST_DESKTOP"))
        QCOMPARE(target.value(QStringLiteral("desktop")).toString(), qEnvironmentVariable("QKM_TEST_DESKTOP"));
    main.show();
    QVERIFY(QTest::qWaitForWindowExposed(&main));
    const auto reference = [&](int index) {
        return query().page.value(QStringLiteral("items")).toArray().at(index).toObject().value(QStringLiteral("window_ref")).toString();
    };
    QString mainRef = reference(0);
    QCOMPARE(mainRef.size(), 64);
    QCOMPARE(reference(0), mainRef);
    QEvent activation(QEvent::ActivationChange);
    QCoreApplication::sendEvent(&main, &activation);
    QWindowStateChangeEvent activeOnly(main.windowState() ^ Qt::WindowActive);
    QCoreApplication::sendEvent(&main, &activeOnly);
    QCOMPARE(reference(0), mainRef);
    QCOMPARE(access.resolve(QStringLiteral("main"), mainRef, instance, pair, 1), &main);
    QVERIFY(!access.resolve(QStringLiteral("item_setup"), mainRef, instance, pair, 1));
    QVERIFY(!access.resolve(QStringLiteral("arbitrary"), mainRef, instance, pair, 1));
    QVERIFY(!access.resolve(QStringLiteral("main"), mainRef, QString(32, QLatin1Char('c')), pair, 1));
    QVERIFY(!access.resolve(QStringLiteral("main"), mainRef, instance, QString(32, QLatin1Char('c')), 1));
    QVERIFY(!access.resolve(QStringLiteral("main"), mainRef, instance, pair, 2));
    QVERIFY(!access.resolve(QStringLiteral("main"), QString(64, QLatin1Char('F')), instance, pair, 1));
    main.resize(main.width() + 10, main.height() + 10);
    QVERIFY(!access.resolve(QStringLiteral("main"), mainRef, instance, pair, 1));
    mainRef = reference(0);
    main.hide();
    QVERIFY(!access.resolve(QStringLiteral("main"), mainRef, instance, pair, 1));
    main.show();
    QVERIFY(QTest::qWaitForWindowExposed(&main));
    QVERIFY(reference(0) != mainRef);
    mainRef = reference(0);
    main.showMinimized();
    QTRY_VERIFY(main.isMinimized());
    QVERIFY(query().page.value(QStringLiteral("items")).toArray().at(0).toObject().value(QStringLiteral("minimized")).toBool());
    QVERIFY(!access.resolve(QStringLiteral("main"), mainRef, instance, pair, 1));
    main.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&main));
    item->show();
    QVERIFY(QTest::qWaitForWindowExposed(item));
    const QString itemRef = reference(1);
    QCOMPARE(itemRef.size(), 64);
    QCOMPARE(access.resolve(QStringLiteral("item_setup"), itemRef, instance, pair, 1), item);
    QVERIFY(item->close());
    QVERIFY(!access.resolve(QStringLiteral("item_setup"), itemRef, instance, pair, 1));
    item->show();
    QVERIFY(QTest::qWaitForWindowExposed(item));
    QVERIFY(reference(1) != itemRef);
    const QString reopened = reference(1);
    delete item;
    QVERIFY(!query().page.value(QStringLiteral("items")).toArray().at(1).toObject().value(QStringLiteral("exists")).toBool());
    QVERIFY(!access.resolve(QStringLiteral("item_setup"), reopened, instance, pair, 1));
    auto page = access.query(instance, pair, 1, {{QStringLiteral("entity"), QStringLiteral("windows")},
        {QStringLiteral("offset"), 0}, {QStringLiteral("limit"), 1}});
    QVERIFY(page.ok());
    QCOMPARE(page.page.value(QStringLiteral("items")).toArray().size(), 1);
    QCOMPARE(page.page.value(QStringLiteral("next_offset")).toInt(), 1);
    page = access.query(instance, pair, 1, {{QStringLiteral("entity"), QStringLiteral("windows")},
        {QStringLiteral("offset"), 2}});
    QVERIFY(page.ok());
    QVERIFY(page.page.value(QStringLiteral("items")).toArray().isEmpty());
    for (const auto &extra : QList<QJsonObject>{
            {{QStringLiteral("offset"), true}}, {{QStringLiteral("offset"), -1}}, {{QStringLiteral("offset"), 3}},
            {{QStringLiteral("offset"), 1.5}}, {{QStringLiteral("limit"), 0}}, {{QStringLiteral("limit"), 101}},
            {{QStringLiteral("limit"), true}}, {{QStringLiteral("pid"), 1}}, {{QStringLiteral("hwnd"), QStringLiteral("1")}},
            {{QStringLiteral("source_revision"), QString(64, QLatin1Char('a'))}},
            {{QStringLiteral("catalog_revision"), QString(64, QLatin1Char('a'))}}, {{QStringLiteral("role"), QStringLiteral("main")}}}) {
        auto invalid = arguments;
        for (auto it = extra.begin(); it != extra.end(); ++it) invalid.insert(it.key(), it.value());
        QCOMPARE(access.query(instance, pair, 1, invalid).errorCode, QStringLiteral("invalid_arguments"));
    }
    QVERIFY(!access.query(instance, pair, 0, arguments).ok());
    WindowQueryResult wrongThread;
    std::thread worker([&]() { wrongThread = query(); });
    worker.join();
    QCOMPARE(wrongThread.errorCode, QStringLiteral("context_busy"));
}

void McpControlTest::mappingEditorArguments()
{
    const QString revision(64, QLatin1Char('a'));
    const QJsonObject valid{{QStringLiteral("source_ref"), revision + QStringLiteral(":row:2:3")},
        {QStringLiteral("presentation"), QStringLiteral("private_background")}};
    const auto parsed = parseMappingEditorArguments(valid);
    QVERIFY(parsed.ok());
    QCOMPARE(parsed.sourceRevision, revision);
    QCOMPARE(parsed.sourceTab, 2);
    QCOMPARE(parsed.sourceRow, 3);
    for (const auto &extra : QList<QJsonObject>{
            {{QStringLiteral("source_ref"), 1}}, {{QStringLiteral("source_ref"), revision + QStringLiteral(":row:02:3")}},
            {{QStringLiteral("source_ref"), revision + QStringLiteral(":row:2:-1")}},
            {{QStringLiteral("source_ref"), revision + QStringLiteral(":row:2147483648:0")}},
            {{QStringLiteral("source_ref"), revision + QStringLiteral(":source:2")}},
            {{QStringLiteral("presentation"), QStringLiteral("foreground")}},
            {{QStringLiteral("pid"), 1}}, {{QStringLiteral("hwnd"), QStringLiteral("1")}},
            {{QStringLiteral("path"), QStringLiteral("test.exe")}}}) {
        auto changed = valid;
        for (auto it = extra.begin(); it != extra.end(); ++it) changed.insert(it.key(), it.value());
        QVERIFY(!parseMappingEditorArguments(changed).ok());
    }
    for (const auto &key : valid.keys()) {
        auto changed = valid; changed.remove(key);
        QVERIFY(!parseMappingEditorArguments(changed).ok());
    }
    const auto capability = mappingEditorCapability();
    QCOMPARE(capability.value(QStringLiteral("required_scopes")).toInt(), int(Read | ShowWindows));
    QCOMPARE(capability.value(QStringLiteral("read_only")), QJsonValue(false));
    auto closing = valid;
    closing[QStringLiteral("window_ref")] = QString(64, QLatin1Char('b'));
    const auto parsedClose = parseMappingEditorCloseArguments(closing);
    QVERIFY(parsedClose.ok());
    QCOMPARE(parsedClose.source.sourceReference, parsed.sourceReference);
    for (const auto &extra : QList<QJsonObject>{
            {{QStringLiteral("window_ref"), 1}}, {{QStringLiteral("window_ref"), QString(63, QLatin1Char('b'))}},
            {{QStringLiteral("window_ref"), QString(64, QLatin1Char('B'))}},
            {{QStringLiteral("discard"), true}}, {{QStringLiteral("pid"), 1}}, {{QStringLiteral("hwnd"), QStringLiteral("1")}}}) {
        auto changed = closing;
        for (auto it = extra.begin(); it != extra.end(); ++it) changed.insert(it.key(), it.value());
        QVERIFY(!parseMappingEditorCloseArguments(changed).ok());
    }
    QVERIFY(!parseMappingEditorCloseArguments(valid).ok());
    const auto closeCapability = mappingEditorCloseCapability();
    QCOMPARE(closeCapability.value(QStringLiteral("required_scopes")).toInt(), int(Read | ShowWindows));
    QCOMPARE(closeCapability.value(QStringLiteral("read_only")), QJsonValue(false));
}

void McpControlTest::mappingSettingsArguments()
{
    const QString revision(64, QLatin1Char('a'));
    const QJsonObject valid{{QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:2")},
        {QStringLiteral("presentation"), QStringLiteral("private_background")}};
    const auto parsed = parseMappingSettingsArguments(valid);
    QVERIFY(parsed.ok()); QCOMPARE(parsed.sourceRevision, revision); QCOMPARE(parsed.sourceTab, 2); QCOMPARE(parsed.sourceRow, -1);
    QVERIFY(!parseMappingEditorArguments(valid).ok());
    for (const auto &extra : QList<QJsonObject>{
            {{QStringLiteral("source_tab_ref"), 1}}, {{QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:02")}},
            {{QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:-1")}},
            {{QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:2147483648")}},
            {{QStringLiteral("source_tab_ref"), revision + QStringLiteral(":row:2:0")}},
            {{QStringLiteral("presentation"), QStringLiteral("foreground")}}, {{QStringLiteral("discard"), true}},
            {{QStringLiteral("pid"), 1}}, {{QStringLiteral("hwnd"), QStringLiteral("1")}}, {{QStringLiteral("path"), QStringLiteral("test.exe")}}}) {
        auto invalid = valid;
        for (auto it = extra.begin(); it != extra.end(); ++it) invalid.insert(it.key(), it.value());
        QVERIFY(!parseMappingSettingsArguments(invalid).ok());
    }
    auto closing = valid; closing.insert(QStringLiteral("window_ref"), QString(64, QLatin1Char('b')));
    QVERIFY(parseMappingSettingsCloseArguments(closing).ok());
    QVERIFY(!parseMappingSettingsCloseArguments(valid).ok());
    closing.insert(QStringLiteral("window_ref"), QString(64, QLatin1Char('B')));
    QVERIFY(!parseMappingSettingsCloseArguments(closing).ok());
    for (bool close : {false, true}) {
        const auto capability = mappingSettingsCapability(close);
        QCOMPARE(capability.value(QStringLiteral("required_scopes")).toInt(), int(Read | ShowWindows));
        QCOMPARE(capability.value(QStringLiteral("read_only")), QJsonValue(false));
        QCOMPARE(capability.value(QStringLiteral("arguments_schema")).toObject().value(QStringLiteral("additionalProperties")), QJsonValue(false));
    }
}

void McpControlTest::startupPurpose()
{
    QCOMPARE(parseStartupPurpose({}), StartupPurpose::Standard);
    QCOMPARE(parseStartupPurpose({QStringLiteral("--scale=1.0")}), StartupPurpose::Standard);
    QCOMPARE(parseStartupPurpose({QStringLiteral("--scale=1.0"), QStringLiteral("--demo-configuration-only")}), StartupPurpose::ConfigurationOnly);
    QCOMPARE(parseStartupPurpose({QStringLiteral("--demo-host")}), StartupPurpose::DemoHost);
    for (const auto &arguments : QList<QStringList>{
            {QStringLiteral("--demo-configuration-only"), QStringLiteral("--demo-configuration-only")},
            {QStringLiteral("--demo-configuration-only=true")}, {QStringLiteral("--demo-unknown")},
            {QStringLiteral("--demo-host"), QStringLiteral("--demo-configuration-only")},
            {QStringLiteral("--demo-host"), QStringLiteral("--demo-host")},
            {QStringLiteral("--demo-host"), QStringLiteral("--scale=1.0")}})
        QCOMPARE(parseStartupPurpose(arguments), StartupPurpose::Invalid);
    const bool mode = configurationOnlyDemo();
    QCOMPARE(startupAllowedScopes(), mode ? quint32(Read | Configure | ShowWindows | CaptureWindows) : KnownScopes);
    QVERIFY(!initializeStartupPurpose(StartupPurpose::Standard));
    QCOMPARE(configurationOnlyDemo(), mode);
    const quint64 before = demoEffectDenialsForTest();
    QCOMPARE(denyDemoEffect(), mode);
    QCOMPARE(demoEffectDenialsForTest(), before + (mode ? 1 : 0));
    if (mode) QCOMPARE(configurationOnlyPolicy().value(QStringLiteral("system_input_allowed")), QJsonValue(false));
}

void McpControlTest::demoProcessLifecycle()
{
    DemoProcess owner;
    QVERIFY(owner.close());
    if (!configurationOnlyDemo()) {
        DemoSetupEndpoint endpoint([] { return false; }, qApp);
        QVERIFY(!endpoint.ready());
        QVERIFY(!owner.start());
        QCOMPARE(owner.failure(), QStringLiteral("configuration_only_required"));
        QVERIFY(!owner.nativeError());
        return;
    }
    const QString runtime = qEnvironmentVariable("QKM_TEST_OWNER_RUNTIME");
    {
        HANDLE request = CreateEventW(nullptr, FALSE, TRUE, nullptr), ack = CreateEventW(nullptr, FALSE, TRUE, nullptr);
        QVERIFY(request && ack);
        const auto release = qScopeGuard([request, ack] { CloseHandle(request); CloseHandle(ack); });
        const auto own = queryProcessIdentity(GetCurrentProcessId());
        QVERIFY(own.ok());
        qputenv(DemoSetupRequestEnv, QByteArray::number(quintptr(request)));
        qputenv(DemoSetupAckEnv, QByteArray::number(quintptr(ack)));
        qputenv(DemoSetupParentEnv, QByteArray::number(own.identity.processId));
        qputenv(DemoSetupCreatedEnv, QByteArray::number(own.identity.creationTime));
        prepareInheritedDemoSetup();
        DemoSetupEndpoint rejected([] { return false; }, qApp);
        QVERIFY(!rejected.ready());
        QVERIFY(WaitForSingleObject(request, 0) == WAIT_OBJECT_0 && WaitForSingleObject(ack, 0) == WAIT_OBJECT_0);
        QVERIFY(qEnvironmentVariableIsEmpty(DemoSetupRequestEnv) && qEnvironmentVariableIsEmpty(DemoSetupAckEnv)
            && qEnvironmentVariableIsEmpty(DemoSetupParentEnv) && qEnvironmentVariableIsEmpty(DemoSetupCreatedEnv));
    }
    QVERIFY(QDir::fromNativeSeparators(runtime).contains(QStringLiteral("/out/")));
    QCOMPARE(QDir(runtime).absolutePath(), QCoreApplication::applicationDirPath());
    const auto releaseFixture = [&](DemoProcess &process) {
        if (!process.running()) return;
        QFile release(QDir(runtime).filePath(QStringLiteral("release-child")));
        if (release.open(QIODevice::WriteOnly)) release.close();
        if (!process.close()) qWarning() << "Fixture cleanup failed" << process.failure();
    };
    const auto finishOwner = qScopeGuard([&] { releaseFixture(owner); });
    const QString iniPath = QDir(runtime).filePath(QStringLiteral("keymapdata.ini"));
    QVERIFY(!QFileInfo::exists(iniPath));
    QVERIFY(!owner.start());
    QCOMPARE(owner.failure(), QStringLiteral("deployment_unavailable"));
    QVERIFY(owner.nativeError().has_value());
    const QByteArray seed = demoDeploymentSeed();
    const auto writeIni = [&](const QByteArray &bytes) {
        QSaveFile file(iniPath);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
    };
    QVERIFY(writeIni(seed));
    QVERIFY(!owner.start());
    QCOMPARE(owner.failure(), QStringLiteral("deployment_not_prepared"));
    QVERIFY(!owner.nativeError());
    QByteArray prepared = seed;
    prepared.replace("[General]\n", "[General]\nMcpDemoDeploymentVersion=1\n");
    QByteArray unsafe = prepared;
    unsafe.replace("StartupAutoMonitoring=false", "StartupAutoMonitoring=true");
    QVERIFY(writeIni(unsafe));
    QVERIFY(!owner.start());
    QCOMPARE(owner.failure(), QStringLiteral("deployment_not_prepared"));
    QVERIFY(writeIni(prepared));
    {
        QProcess peer;
        peer.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--demo-configuration-only")});
        QVERIFY(peer.waitForStarted(5000));
        const QString releasePath = QDir(runtime).filePath(QStringLiteral("release-child"));
        const auto finishPeer = qScopeGuard([&] {
            QFile release(releasePath);
            if (release.open(QIODevice::WriteOnly)) release.close();
            peer.waitForFinished(5000);
            QFile::remove(releasePath);
        });
        const auto identity = queryProcessIdentity(DWORD(peer.processId()));
        QVERIFY(identity.ok());
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(QDir(runtime).filePath(
            QStringLiteral("child-%1.json").arg(peer.processId()))), 10000);
        QVERIFY(!owner.start());
        QCOMPARE(owner.failure(), QStringLiteral("deployment_has_live_instance"));
        QVERIFY(!owner.nativeError() && owner.processHandle() == nullptr);
        QVERIFY(queryProcessIdentity(identity.identity.processId).ok());
        QFile release(releasePath);
        QVERIFY(release.open(QIODevice::WriteOnly));
        release.close();
        QVERIFY(peer.waitForFinished(5000));
        QCOMPARE(peer.exitCode(), 0);
    }
    QString previousDesktop;
    const QString unrelatedName = QStringLiteral("Local\\QkmDemoUnrelated_%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    SECURITY_ATTRIBUTES inherited{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE unrelated = CreateEventW(&inherited, FALSE, FALSE, reinterpret_cast<LPCWSTR>(unrelatedName.utf16()));
    QVERIFY(unrelated);
    const auto removeUnrelated = qScopeGuard([unrelated] {
        CloseHandle(unrelated);
        qunsetenv("QKM_TEST_UNRELATED_HANDLE"); qunsetenv("QKM_TEST_UNRELATED_EVENT");
    });
    qputenv("QKM_TEST_UNRELATED_HANDLE", QByteArray::number(quintptr(unrelated)));
    qputenv("QKM_TEST_UNRELATED_EVENT", unrelatedName.toUtf8());
    for (int round = 0; round != 2; ++round) {
        qputenv("QKM_TEST_OWNER_IGNORE_CLOSE", round ? "1" : "0");
        const auto restore = qScopeGuard([] { qunsetenv("QKM_TEST_OWNER_IGNORE_CLOSE"); });
        QVERIFY2(owner.start(), qPrintable(owner.failure()));
        QVERIFY(owner.running());
        const auto target = owner.target();
        DemoProcess competing;
        QVERIFY(!competing.start());
        QCOMPARE(competing.failure(), QStringLiteral("deployment_in_use"));
        QVERIFY(competing.nativeError().has_value());
        QVERIFY(target.isValid() && target.processId != GetCurrentProcessId());
        const QString desktop = owner.desktop();
        QVERIFY(desktop != previousDesktop && desktop.startsWith(QStringLiteral("QkmDemoOwned_")));
        QVERIFY(!owner.start());
        QCOMPARE(owner.failure(), QStringLiteral("already_started"));
        QVERIFY(sameProcess(target, owner.target()));
        const QString report = QDir(runtime).filePath(QStringLiteral("child-%1.json").arg(target.processId));
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(report), 10000);
        QFile file(report);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto actual = QJsonDocument::fromJson(file.readAll()).object();
        file.close();
        QCOMPARE(actual.value(QStringLiteral("desktop")).toString(), desktop);
        QCOMPARE(actual.value(QStringLiteral("created")).toString(), QString::number(target.creationTime));
        QVERIFY(actual.value(QStringLiteral("configuration_only")).toBool());
        QVERIFY(actual.value(QStringLiteral("setup_ready")).toBool());
        QVERIFY(actual.value(QStringLiteral("setup_environment_cleared")).toBool());
        QVERIFY(!actual.value(QStringLiteral("unrelated_inherited")).toBool());
        QVERIFY2(actual.value(QStringLiteral("unrelated_probe_valid")).toBool(), qPrintable(
            QString::fromUtf8(QJsonDocument(actual.value(QStringLiteral("unrelated_probe")).toObject()).toJson(QJsonDocument::Compact))));
        for (int shown = 1; shown != 3; ++shown) {
            const auto result = owner.requestManagement();
            QVERIFY2(result.ok(), qPrintable(result.failure));
            QVERIFY(!result.nativeError && owner.running() && sameProcess(target, owner.target()));
            QFile changed(report);
            QVERIFY(changed.open(QIODevice::ReadOnly));
            QCOMPARE(QJsonDocument::fromJson(changed.readAll()).object().value(QStringLiteral("management_requests")).toInt(), shown);
        }
        if (round) {
            QVERIFY(!owner.close());
            QCOMPARE(owner.failure(), QStringLiteral("close_timeout"));
            QVERIFY(!owner.nativeError() && owner.running() && sameProcess(target, owner.target()));
            QFile release(QDir(runtime).filePath(QStringLiteral("release-child")));
            QVERIFY(release.open(QIODevice::WriteOnly));
            release.close();
        }
        QVERIFY2(owner.close(), qPrintable(owner.failure()));
        QVERIFY(!owner.running() && owner.processHandle() == nullptr && owner.exitCode().has_value());
        QCOMPARE(owner.exitCode().value(), DWORD(0));
        previousDesktop = desktop;
    }
    {
        QVERIFY(QFile::remove(QDir(runtime).filePath(QStringLiteral("release-child"))));
        qputenv("QKM_TEST_OWNER_REJECT_MANAGEMENT", "1");
        const auto restore = qScopeGuard([] { qunsetenv("QKM_TEST_OWNER_REJECT_MANAGEMENT"); });
        QVERIFY(owner.start());
        const auto target = owner.target();
        const auto request = owner.requestManagement();
        QCOMPARE(request.failure, QStringLiteral("setup_timeout"));
        QVERIFY(!request.nativeError && owner.running() && sameProcess(target, owner.target()));
        QFile report(QDir(runtime).filePath(QStringLiteral("child-%1.json").arg(target.processId)));
        QVERIFY(report.open(QIODevice::ReadOnly));
        QCOMPARE(QJsonDocument::fromJson(report.readAll()).object().value(QStringLiteral("management_requests")).toInt(), 1);
        report.close();
        QVERIFY(owner.close());
        QVERIFY(owner.exitCode() && owner.exitCode().value() == 0);
    }
    QFile finalIni(iniPath);
    QVERIFY(finalIni.open(QIODevice::ReadOnly));
    QCOMPARE(finalIni.readAll(), prepared);
    QVERIFY(!QFileInfo::exists(QDir(runtime).filePath(QStringLiteral("release-child"))));
    for (int round = 0; round != 2; ++round) {
        qputenv("QKM_TEST_OWNER_IGNORE_CLOSE", round ? "1" : "0");
        const auto restore = qScopeGuard([] { qunsetenv("QKM_TEST_OWNER_IGNORE_CLOSE"); });
        DemoHostDialog host;
        const auto finishHost = qScopeGuard([&] { releaseFixture(host.process()); });
        QVERIFY(host.process().running());
        const auto target = host.process().target();
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(QDir(runtime).filePath(
            QStringLiteral("child-%1.json").arg(target.processId))), 10000);
        host.show();
        QVERIFY(host.isVisible());
        auto *button = host.findChild<QPushButton *>(QStringLiteral("demoHostClose"));
        QVERIFY(button && !button->autoDefault() && button->focusPolicy() == Qt::NoFocus);
        if (round) {
            QVERIFY(!host.close());
            QVERIFY(host.isVisible() && host.process().running() && sameProcess(host.process().target(), target));
            QCOMPARE(host.process().failure(), QStringLiteral("close_timeout"));
        }
        if (!qEnvironmentVariableIsEmpty("QKM_TEST_TRANSLATIONS_DIR")) {
            QTranslator translator;
            for (int language = 0; language != 3; ++language) {
                QVERIFY(installUiTranslation(translator, language));
                QCoreApplication::processEvents();
                QCOMPARE(host.windowTitle(), DemoHostDialog::tr("QKeyMapper Demo Host"));
                auto *status = host.findChild<QLabel *>(QStringLiteral("demoHostStatus"));
                QVERIFY(status && status->text().count(QLatin1Char('\n')) == 1);
                if (!round) QVERIFY(status->text().contains(QString::number(target.processId)) && status->text().contains(host.process().executable()));
                else QVERIFY(status->text().contains(QStringLiteral("close_timeout")));
                QVERIFY(sameProcess(host.process().target(), target) && host.process().running());
                const QString captures = qEnvironmentVariable("QKM_HOST_CAPTURE_DIR");
                if (!round && !captures.isEmpty()) {
                    QVERIFY(QDir::fromNativeSeparators(captures).contains(QStringLiteral("/out/")));
                    const QString path = QDir(captures).filePath(QStringLiteral("host-%1.json").arg(language));
                    QSaveFile file(path);
                    const QByteArray bytes = QJsonDocument(QJsonObject{{QStringLiteral("pid"), int(GetCurrentProcessId())},
                        {QStringLiteral("hwnd"), QString::number(quintptr(host.winId()))}, {QStringLiteral("title"), host.windowTitle()}}).toJson();
                    QVERIFY(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit());
                    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(path + QStringLiteral(".done")), 15000);
                }
            }
        }
        if (!round) QTest::keyClick(&host, Qt::Key_Escape);
        else {
            QFile release(QDir(runtime).filePath(QStringLiteral("release-child")));
            QVERIFY(release.open(QIODevice::WriteOnly));
            release.close();
        }
        QTRY_VERIFY_WITH_TIMEOUT(!host.isVisible(), 5000);
        QVERIFY(!host.process().running() && host.process().exitCode());
        QCOMPARE(host.process().exitCode().value(), DWORD(0));
    }
    QVERIFY(finalIni.seek(0));
    QCOMPARE(finalIni.readAll(), prepared);
    finalIni.close();
    const auto unavailable = owner.requestManagement();
    QCOMPARE(unavailable.failure, QStringLiteral("setup_unavailable"));
    QVERIFY(!unavailable.nativeError);
    QVERIFY(writeIni(unsafe));
    {
        const auto restore = qScopeGuard([&] { writeIni(prepared); });
        DemoHostDialog failed;
        failed.show();
        QVERIFY(!failed.process().running() && !failed.process().target().isValid());
        QCOMPARE(failed.process().failure(), QStringLiteral("deployment_not_prepared"));
        auto *status = failed.findChild<QLabel *>(QStringLiteral("demoHostStatus"));
        QVERIFY(status && status->text().contains(QStringLiteral("deployment_not_prepared")) && status->text().count(QLatin1Char('\n')) == 1);
        QTest::keyClick(&failed, Qt::Key_Escape);
        QVERIFY(!failed.isVisible());
    }
}

void McpControlTest::demoPairingPrompt()
{
    DemoProcess owner;
    DemoPairingSession session;
    const QString title = DemoHostDialog::tr("QKeyMapper Demo Host");
    if (!configurationOnlyDemo()) {
        QVERIFY(!session.start(owner, title, QString(), QString()));
        QCOMPARE(session.result().failure, QStringLiteral("setup_unavailable"));
        return;
    }
    const QString runtime = qEnvironmentVariable("QKM_TEST_OWNER_RUNTIME");
    QVERIFY(QDir::fromNativeSeparators(runtime).contains(QStringLiteral("/out/")));
    const QString releasePath = QDir(runtime).filePath(QStringLiteral("release-child"));
    QFile::remove(releasePath);
    const auto cleanup = qScopeGuard([&] {
        session.cancel();
        if (owner.running()) {
            QFile release(releasePath);
            if (release.open(QIODevice::WriteOnly)) release.close();
            owner.close();
        }
        QFile::remove(releasePath);
    });
    QByteArray prepared = demoDeploymentSeed();
    prepared.replace("[General]\n", "[General]\nMcpDemoDeploymentVersion=1\n");
    QSaveFile ini(QDir(runtime).filePath(QStringLiteral("keymapdata.ini")));
    QVERIFY(ini.open(QIODevice::WriteOnly) && ini.write(prepared) == prepared.size() && ini.commit());
    QVERIFY(owner.start());
    const auto target = owner.target();
    QVERIFY(!session.start(owner, title, QString(), QString()));
    QCOMPARE(session.result().failure, QStringLiteral("default_input_required"));
    QVERIFY(!session.result().nativeError && !session.isRunning() && owner.running() && sameProcess(owner.target(), target));
    const auto promptWindow = [](const DemoProcess &process) {
        HDESK desktop = OpenDesktopW(reinterpret_cast<LPCWSTR>(process.desktop().utf16()), 0, FALSE, DESKTOP_ENUMERATE | DESKTOP_READOBJECTS);
        if (!desktop) return HWND(nullptr);
        const auto close = qScopeGuard([desktop] { CloseDesktop(desktop); });
        HWND found = nullptr;
        EnumDesktopWindows(desktop, [](HWND window, LPARAM argument) -> BOOL {
            DWORD pid = 0;
            GetWindowThreadProcessId(window, &pid);
            wchar_t name[32]{};
            if (pid != GetCurrentProcessId() || !GetClassNameW(window, name, 32) || QString::fromWCharArray(name) != QStringLiteral("#32770")) return TRUE;
            *reinterpret_cast<HWND *>(argument) = window;
            return FALSE;
        }, reinterpret_cast<LPARAM>(&found));
        return found;
    };
    QSignalSpy completed(&session, &DemoPairingSession::finished);
    const auto waitForPrompt = [&] {
        const bool done = !session.isRunning() || completed.wait(5000);
        if (!done) {
            QJsonObject observed = session.workerForTest();
            const HWND window = promptWindow(owner);
            observed.insert(QStringLiteral("dialog_found"), window != nullptr);
            observed.insert(QStringLiteral("ok_button_found"), window && GetDlgItem(window, IDOK));
            QSaveFile file(QDir(runtime).filePath(QStringLiteral("prompt-timeout-observation.json")));
            const auto bytes = QJsonDocument(observed).toJson();
            if (file.open(QIODevice::WriteOnly)) { file.write(bytes); file.commit(); }
            if (window) PostMessageW(window, WM_CLOSE, 0, 0);
        }
        return done && !session.isRunning();
    };
    const auto capture = [&](HWND window, const QString &desktop, const QString &label) {
        const QString directory = qEnvironmentVariable("QKM_PAIRING_CAPTURE_DIR");
        if (directory.isEmpty()) return;
        QVERIFY(QDir::fromNativeSeparators(directory).contains(QStringLiteral("/out/")));
        const QString path = QDir(directory).filePath(label + QStringLiteral(".json"));
        QSaveFile file(path);
        const QByteArray bytes = QJsonDocument(QJsonObject{{QStringLiteral("pid"), int(GetCurrentProcessId())},
            {QStringLiteral("hwnd"), QString::number(quintptr(window))}, {QStringLiteral("desktop"), desktop}}).toJson();
        QVERIFY(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit());
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(path + QStringLiteral(".done")), 15000);
    };
    QVERIFY(session.startPromptForTest(owner, title, QStringLiteral("timeout fixture"), 250));
    QVERIFY(waitForPrompt());
    QCOMPARE(session.result().failure, QStringLiteral("setup_timeout"));
    QVERIFY(!session.result().nativeError && owner.running());
    QVERIFY(session.startPromptForTest(owner, title, QStringLiteral("cancel fixture"), 10000));
    QTRY_VERIFY_WITH_TIMEOUT(promptWindow(owner) != nullptr, 5000);
    QVERIFY(!session.startPromptForTest(owner, title, QString(), 250));
    QCOMPARE(session.result().failure, QStringLiteral("setup_busy"));
    session.cancel();
    QVERIFY(waitForPrompt());
    QCOMPARE(session.result().failure, QStringLiteral("setup_cancelled"));
    QVERIFY(!session.result().nativeError && owner.running());
    QTranslator translator;
    for (int language = 0; language != 3; ++language) {
        QVERIFY(installUiTranslation(translator, language));
        for (bool recovery : {false, true}) {
            const QString text = recovery
                ? DemoHostDialog::tr("The original desktop could not be restored.\nClick OK to try returning again.\nReason: %1").arg(QStringLiteral("synthetic restore failure"))
                : DemoHostDialog::tr("Finish MCP pairing in the QKeyMapper management window, then click OK to return to your desktop.");
            QCOMPARE(text.count(QLatin1Char('\n')), recovery ? 2 : 0);
            QVERIFY(session.startPromptForTest(owner, DemoHostDialog::tr("QKeyMapper Demo Host"), text, 20000));
            QTRY_VERIFY_WITH_TIMEOUT(promptWindow(owner) != nullptr, 5000);
            HWND window = promptWindow(owner);
            struct Text { QString expected, caption; bool found = false; DWORD error = 0; } observed{text, QString()};
            const QString desktopName = owner.desktop();
            std::thread reader([&] {
                HDESK desktop = OpenDesktopW(reinterpret_cast<LPCWSTR>(desktopName.utf16()), 0, FALSE, DESKTOP_READOBJECTS | DESKTOP_ENUMERATE);
                if (!desktop) { observed.error = GetLastError(); return; }
                const auto close = qScopeGuard([desktop] { CloseDesktop(desktop); });
                const HDESK original = GetThreadDesktop(GetCurrentThreadId());
                if (!SetThreadDesktop(desktop)) { observed.error = GetLastError(); return; }
                const auto restore = qScopeGuard([original] { SetThreadDesktop(original); });
                wchar_t caption[256]{};
                GetWindowTextW(window, caption, 256);
                observed.caption = QString::fromWCharArray(caption);
                EnumChildWindows(window, [](HWND child, LPARAM argument) -> BOOL {
                    auto &text = *reinterpret_cast<Text *>(argument);
                    wchar_t value[4096]{};
                    GetWindowTextW(child, value, 4096);
                    if (QString::fromWCharArray(value).remove(QLatin1Char('\r')) == text.expected) text.found = true;
                    return TRUE;
                }, reinterpret_cast<LPARAM>(&observed));
            });
            reader.join();
            QCOMPARE(observed.error, DWORD(0));
            QCOMPARE(observed.caption, DemoHostDialog::tr("QKeyMapper Demo Host"));
            QVERIFY(observed.found);
            capture(window, owner.desktop(), QStringLiteral("pairing-%1-%2").arg(language).arg(recovery ? QStringLiteral("recovery") : QStringLiteral("prompt")));
            QVERIFY(PostMessageW(window, WM_CLOSE, 0, 0));
            QVERIFY(waitForPrompt());
            QVERIFY2(session.result().ok(), qPrintable(session.result().failure));
            QVERIFY(!IsWindow(window));
            QVERIFY(!session.workerForTest().value(QStringLiteral("running")).toBool());
        }
    }
    QVERIFY(session.startPromptForTest(owner, title, QStringLiteral("target exit fixture"), 10000));
    QTRY_VERIFY_WITH_TIMEOUT(promptWindow(owner) != nullptr, 5000);
    QFile release(releasePath);
    QVERIFY(release.open(QIODevice::WriteOnly)); release.close();
    QVERIFY(waitForPrompt());
    QCOMPARE(session.result().failure, QStringLiteral("target_exited"));
    QVERIFY(owner.close() && owner.exitCode() && *owner.exitCode() == 0);
    QVERIFY(QFile::remove(releasePath));
    QVERIFY(!session.start(owner, title, QString(), QString()));
    QCOMPARE(session.result().failure, QStringLiteral("setup_unavailable"));
    DemoHostDialog host;
    QVERIFY(host.process().running());
    host.show();
    auto *setup = host.findChild<QPushButton *>(QStringLiteral("demoHostPairing"));
    auto *status = host.findChild<QLabel *>(QStringLiteral("demoHostStatus"));
    QVERIFY(setup && status && !setup->autoDefault() && setup->focusPolicy() == Qt::NoFocus && setup->isEnabled());
    QTest::mouseClick(setup, Qt::LeftButton);
    QVERIFY(status->text().contains(QStringLiteral("default_input_required")));
    const auto hostTarget = host.process().target();
    for (int language = 0; language != 3; ++language) {
        QVERIFY(installUiTranslation(translator, language));
        QCoreApplication::processEvents();
        QCOMPARE(setup->text(), DemoHostDialog::tr("Set Up Pairing"));
        QCOMPARE(status->text().count(QLatin1Char('\n')), 1);
        QCOMPARE(status->text(), DemoHostDialog::tr("Pairing setup could not be shown.\nReason: %1").arg(QStringLiteral("default_input_required")));
        wchar_t desktop[256]{}; DWORD needed = 0;
        QVERIFY(GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, desktop, sizeof(desktop), &needed));
        capture(reinterpret_cast<HWND>(host.winId()), QString::fromWCharArray(desktop), QStringLiteral("pairing-%1-host").arg(language));
        QVERIFY(host.process().running() && sameProcess(hostTarget, host.process().target()));
    }
    QVERIFY(host.pairingForTest().startPromptForTest(host.process(), title, QStringLiteral("host close fixture"), 10000));
    QTRY_VERIFY_WITH_TIMEOUT(promptWindow(host.process()) != nullptr, 5000);
    QVERIFY(!host.close());
    QTRY_VERIFY_WITH_TIMEOUT(!host.isVisible(), 10000);
    QVERIFY(!host.pairingForTest().isRunning() && host.process().exitCode() && *host.process().exitCode() == 0);
    QFile disk(QDir(runtime).filePath(QStringLiteral("keymapdata.ini")));
    QVERIFY(disk.open(QIODevice::ReadOnly));
    QCOMPARE(disk.readAll(), prepared);
}

void McpControlTest::editorDraftGuard()
{
    QWidget main, item;
    QLineEdit first(&item), second(&item);
    first.setObjectName(QStringLiteral("duplicate")); second.setObjectName(QStringLiteral("duplicate"));
    first.setText(QStringLiteral("B")); second.setText(QStringLiteral("A"));
    QSpinBox spin(&item);
    spin.setRange(-100, 100);
    QComboBox combo(&item);
    combo.addItems({QStringLiteral("A"), QStringLiteral("B")});
    QCheckBox check(&item);
    QCOMPARE(item.internalWinId(), WId(0));
    const auto original = windowInputFingerprint(&item);
    QCOMPARE(original.size(), 32);
    QCOMPARE(item.internalWinId(), WId(0));
    first.setText(QStringLiteral("C")); QVERIFY(windowInputFingerprint(&item) != original);
    first.setText(QStringLiteral("B")); QCOMPARE(windowInputFingerprint(&item), original);
    second.setText(QStringLiteral("C")); QVERIFY(windowInputFingerprint(&item) != original);
    second.setText(QStringLiteral("A")); QCOMPARE(windowInputFingerprint(&item), original);
    auto *number = spin.findChild<QLineEdit *>();
    QVERIFY(number);
    const QString numberBefore = number->text();
    number->setText(QStringLiteral("-"));
    QCOMPARE(spin.value(), 0);
    QVERIFY(windowInputFingerprint(&item) != original);
    number->setText(numberBefore); QCOMPARE(windowInputFingerprint(&item), original);
    combo.setCurrentIndex(1); QVERIFY(windowInputFingerprint(&item) != original);
    combo.setCurrentIndex(0); QCOMPARE(windowInputFingerprint(&item), original);
    check.setChecked(true); QVERIFY(windowInputFingerprint(&item) != original);
    check.setChecked(false); QCOMPARE(windowInputFingerprint(&item), original);
    QByteArray fromWorker;
    std::thread worker([&]() { fromWorker = windowInputFingerprint(&item); });
    worker.join(); QVERIFY(fromWorker.isEmpty());
    QDialog advanced(&main);
    WindowAccess access(&main, &item, &advanced);
    item.show(); QVERIFY(QTest::qWaitForWindowExposed(&item));
    const QString source = QString(64, QLatin1Char('a')) + QStringLiteral(":row:0:0");
    const auto opened = windowInputFingerprint(&item);
    QVERIFY(!access.ownsEditor(source));
    access.rememberEditor(source, opened);
    QVERIFY(access.ownsEditor(source)); QVERIFY(access.editorDraftMatches(opened));
    QVERIFY(!access.ownsEditor(source + QLatin1Char('1')));
    QVERIFY(!access.editorDraftMatches(QByteArray(32, 'x')));
    QWidget child(&item, Qt::Tool);
    child.show();
    QVERIFY(child.isVisible());
    QVERIFY(windowInputFingerprint(&item).isEmpty());
    child.close();
    QVERIFY(access.editorDraftMatches(windowInputFingerprint(&item)));
    item.resize(item.width() + 20, item.height() + 20);
    QVERIFY(access.ownsEditor(source)); QVERIFY(access.editorDraftMatches(windowInputFingerprint(&item)));
    item.hide(); QVERIFY(!access.ownsEditor(source)); QVERIFY(!access.editorDraftMatches(opened));
    item.show(); QVERIFY(!access.ownsEditor(source));
    access.rememberEditor(source, windowInputFingerprint(&item));
    QEvent nativeChange(QEvent::WinIdChange);
    QCoreApplication::sendEvent(&item, &nativeChange);
    QVERIFY(!access.ownsEditor(source));
    item.close();
    advanced.show(); QVERIFY(QTest::qWaitForWindowExposed(&advanced));
    const QString settingsSource = QString(64, QLatin1Char('a')) + QStringLiteral(":source:0");
    const QString role = QStringLiteral("mapping_advanced");
    access.rememberEditor(settingsSource, windowInputFingerprint(&advanced), role);
    QVERIFY(access.ownsEditor(settingsSource, role));
    QVERIFY(!access.ownsEditor(settingsSource));
    advanced.hide(); QVERIFY(!access.ownsEditor(settingsSource, role));
    advanced.show(); QVERIFY(!access.ownsEditor(settingsSource, role));
    advanced.close();
}

void McpControlTest::privateBackgroundDesktopGuard()
{
    const QString expected = qEnvironmentVariable("QKM_EXPECT_PRIVATE_BACKGROUND");
    if (!expected.isEmpty()) {
        QVERIFY(expected == QStringLiteral("0") || expected == QStringLiteral("1"));
        QCOMPARE(privateBackgroundDesktopAvailable(), expected == QStringLiteral("1"));
    } else {
        wchar_t desktop[256]{};
        DWORD needed = 0;
        QVERIFY(GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, desktop, sizeof(desktop), &needed));
        if (QString::fromWCharArray(desktop).compare(QStringLiteral("Default"), Qt::CaseInsensitive) == 0)
            QVERIFY(!privateBackgroundDesktopAvailable());
    }
    bool fromWorker = true;
    std::thread worker([&]() { fromWorker = privateBackgroundDesktopAvailable(); });
    worker.join();
    QVERIFY(!fromWorker);
}

void McpControlTest::windowAdvancedObservation()
{
    QWidget main;
    QDialog item(&main), advanced(&main);
    WindowAccess access(&main, &item, &advanced);
    const QString instance(32, QLatin1Char('a')), pair(32, QLatin1Char('b'));
    const QJsonObject arguments{{QStringLiteral("entity"), QStringLiteral("windows")},
        {QStringLiteral("window_role"), QStringLiteral("mapping_advanced")}};
    const auto query = [&] { return access.query(instance, pair, 1, arguments); };
    QCOMPARE(access.query(instance, pair, 1, {{QStringLiteral("entity"), QStringLiteral("windows")}}).page.value(QStringLiteral("total")).toInt(), 2);
    QVERIFY(query().ok());
    QCOMPARE(advanced.internalWinId(), WId(0));
    QCOMPARE(query().page.value(QStringLiteral("total")).toInt(), 1);
    QCOMPARE(query().page.value(QStringLiteral("items")).toArray().first().toObject().value(QStringLiteral("role")).toString(), QStringLiteral("mapping_advanced"));
    auto invalid = arguments; invalid.insert(QStringLiteral("offset"), 2);
    QCOMPARE(access.query(instance, pair, 1, invalid).errorCode, QStringLiteral("invalid_arguments"));
    for (const QJsonValue value : {QJsonValue(QStringLiteral("other")), QJsonValue(QStringLiteral("main")), QJsonValue(true), QJsonValue(QJsonValue::Null)}) {
        invalid = arguments; invalid.insert(QStringLiteral("window_role"), value);
        QCOMPARE(access.query(instance, pair, 1, invalid).errorCode, QStringLiteral("invalid_arguments"));
    }
    advanced.show();
    QVERIFY(QTest::qWaitForWindowExposed(&advanced));
    const auto reference = [&] { return query().page.value(QStringLiteral("items")).toArray().first().toObject().value(QStringLiteral("window_ref")).toString(); };
    const auto first = reference();
    QCOMPARE(first.size(), 64);
    QCOMPARE(access.resolve(QStringLiteral("mapping_advanced"), first, instance, pair, 1), &advanced);
    QVERIFY(!access.resolve(QStringLiteral("main"), first, instance, pair, 1));
    QVERIFY(!access.resolve(QStringLiteral("mapping_advanced"), first, instance, pair, 2));
    advanced.hide();
    QVERIFY(!access.resolve(QStringLiteral("mapping_advanced"), first, instance, pair, 1));
    advanced.show();
    QVERIFY(QTest::qWaitForWindowExposed(&advanced));
    QVERIFY(reference() != first);
}

void McpControlTest::windowCaptureBoundaries()
{
    const QJsonObject valid{{QStringLiteral("role"), QStringLiteral("main")},
        {QStringLiteral("window_ref"), QString(64, QLatin1Char('a'))},
        {QStringLiteral("source_revision"), QString(64, QLatin1Char('b'))},
        {QStringLiteral("source_tab_ref"), QString(64, QLatin1Char('b')) + QStringLiteral(":source:0")}};
    QVERIFY(parseWindowCaptureArguments(valid).ok());
    auto item = valid;
    item[QStringLiteral("role")] = QStringLiteral("item_setup");
    QVERIFY(parseWindowCaptureArguments(item).ok());
    item[QStringLiteral("role")] = QStringLiteral("mapping_advanced");
    QVERIFY(parseWindowCaptureArguments(item).ok());
    for (const auto &extra : QList<QJsonObject>{
            {{QStringLiteral("role"), QStringLiteral("other")}}, {{QStringLiteral("role"), true}},
            {{QStringLiteral("window_ref"), QString(63, QLatin1Char('a'))}},
            {{QStringLiteral("window_ref"), QString(64, QLatin1Char('A'))}},
            {{QStringLiteral("source_revision"), 1}}, {{QStringLiteral("pid"), 1}},
            {{QStringLiteral("hwnd"), QStringLiteral("1")}}, {{QStringLiteral("path"), QStringLiteral("image.png")}}}) {
        auto invalid = valid;
        for (auto it = extra.begin(); it != extra.end(); ++it) invalid.insert(it.key(), it.value());
        QVERIFY(!parseWindowCaptureArguments(invalid).ok());
    }
    item.remove(QStringLiteral("source_revision"));
    QVERIFY(!parseWindowCaptureArguments(item).ok());
    QWidget window;
    window.resize(64, 48);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const auto image = captureOwnWindow(&window, MaxTransportWindowImageBytes);
    QVERIFY(image.ok());
    QVERIFY(image.png.startsWith(QByteArray::fromHex("89504e470d0a1a0a")));
    QCOMPARE(image.pixelSize, QSize(qRound(window.width() * image.devicePixelRatio), qRound(window.height() * image.devicePixelRatio)));
    QVERIFY(image.png.size() > 1);
    const auto limited = captureOwnWindow(&window, int(image.png.size()) - 1);
    QCOMPARE(limited.errorCode, QStringLiteral("image_too_large"));
    QVERIFY(limited.png.isEmpty());
    QVERIFY(captureOwnWindow(&window, int(image.png.size())).ok());
    QCOMPARE(captureOwnWindow(&window, 0).errorCode, QStringLiteral("invalid_window_context"));
    QCOMPARE(captureOwnWindow(&window, MaxWindowImageBytes + 1).errorCode, QStringLiteral("invalid_window_context"));
    window.resize(4096, 2049);
    QVERIFY(double(window.width()) * window.height() * window.devicePixelRatioF() * window.devicePixelRatioF() > MaxWindowImagePixels);
    const auto large = captureOwnWindow(&window, MaxTransportWindowImageBytes);
    QCOMPARE(large.errorCode, QStringLiteral("image_too_large"));
    QVERIFY(large.png.isEmpty());
    window.hide();
    QCOMPARE(captureOwnWindow(&window).errorCode, QStringLiteral("window_unavailable"));
    WindowImage denied;
    std::thread worker([&]() { denied = captureOwnWindow(&window); });
    worker.join();
    QCOMPARE(denied.errorCode, QStringLiteral("invalid_window_context"));
    QVERIFY(denied.png.isEmpty());
}

void McpControlTest::rejectAndClose_data()
{
    QTest::addColumn<int>("closeMethod");
    QTest::newRow("reject-button") << 0;
    QTest::newRow("close-window") << 1;
    QTest::newRow("escape") << 2;
}

void McpControlTest::rejectAndClose()
{
    QFETCH(int, closeMethod);
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/control-XXXXXX"));
    QVERIFY(root.isValid());
    ControlDialog dialog(root.path() + QStringLiteral("/server"), "test-control");
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    clickCheckBox(dialog, "mcpEnabled");
    Client child;
    QVERIFY(child.launch(dialog.server(), root.path() + QStringLiteral("/client"), QStringLiteral("pair-rejected")));
    QTRY_VERIFY_WITH_TIMEOUT(!widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty(), 3000);
    if (closeMethod == 1) QVERIFY(dialog.close());
    else if (closeMethod == 2) QTest::keyClick(&dialog, Qt::Key_Escape);
    else QTest::mouseClick(widget<QPushButton>(dialog, "mcpReject"), Qt::LeftButton);
    QVERIFY(widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
    QVERIFY(dialog.server().isEnabled());
    QVERIFY(dialog.server().pairings().isEmpty());
    QVERIFY(widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty());
}

void McpControlTest::enableFailure()
{
    ControlDialog dialog(QString(), "test-control");
    clickCheckBox(dialog, "mcpEnabled");
    QVERIFY(!dialog.server().isEnabled());
    QVERIFY(!widget<QCheckBox>(dialog, "mcpEnabled")->isChecked());
    QVERIFY(widget<QLabel>(dialog, "mcpStatus")->text().contains(ControlDialog::tr("; the last operation failed. A revocation that was not saved did not succeed.")));
}

void McpControlTest::offlinePairings_data()
{
    QTest::addColumn<int>("language");
    QTest::newRow("Chinese") << 0;
    QTest::newRow("English") << 1;
    QTest::newRow("Japanese") << 2;
}

void McpControlTest::offlinePairings()
{
    QFETCH(int, language);
    QTranslator translator;
    QVERIFY(installUiTranslation(translator, language));
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/offline-XXXXXX"));
    QVERIFY(root.isValid());
    const QString directory = root.path() + QStringLiteral("/server");
    PairCredentialStore store(directory, "test-offline-control");
    const PairCredential credential{QString(32, QLatin1Char('a')), QStringLiteral("测试配对 Test 日本語"),
        1, Read | Configure | RunMapping, QByteArray(AuthSecretBytes, 'k')};
    PairStoreError error;
    QVERIFY(store.save({credential}, error));
    ControlDialog dialog(directory, "test-offline-control");
    dialog.retranslateUi();
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QVERIFY(!dialog.server().isEnabled());
    QVERIFY(dialog.server().pipeName().isEmpty());
    QVERIFY(!QFileInfo::exists(dialog.server().discoveryPath()));
    auto *list = widget<QListWidget>(dialog, "mcpPairings");
    QCOMPARE(list->count(), 1);
    QCOMPARE(list->item(0)->data(Qt::UserRole).toString(), credential.pairId);
    list->setCurrentRow(0);
    QVERIFY(widget<QPushButton>(dialog, "mcpRevoke")->isEnabled());
    QTest::mouseClick(widget<QPushButton>(dialog, "mcpRevoke"), Qt::LeftButton);
    QCOMPARE(list->count(), 0);
    QVERIFY(!dialog.server().isEnabled());
    QVERIFY(dialog.server().pipeName().isEmpty());
    QVERIFY(!QFileInfo::exists(dialog.server().discoveryPath()));
    QList<PairCredential> records;
    QVERIFY(store.load(records, error));
    QVERIFY(records.isEmpty());
    ControlDialog reopened(directory, "test-offline-control");
    reopened.show();
    QVERIFY(QTest::qWaitForWindowExposed(&reopened));
    QCOMPARE(widget<QListWidget>(reopened, "mcpPairings")->count(), 0);
    QVERIFY(!reopened.server().isEnabled());
}

void McpControlTest::selectedPairing_data()
{
    QTest::addColumn<bool>("hasSelection");
    QTest::newRow("no-selection") << false;
    QTest::newRow("selection-differs-from-current") << true;
}

void McpControlTest::selectedPairing()
{
    QFETCH(bool, hasSelection);
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/selection-XXXXXX"));
    QVERIFY(root.isValid());
    const QString directory = root.path() + QStringLiteral("/server");
    PairCredentialStore store(directory, "test-selection");
    const PairCredential first{QString(32, QLatin1Char('a')), QStringLiteral("First client"),
        1, Read, QByteArray(AuthSecretBytes, 'k')};
    const PairCredential second{QString(32, QLatin1Char('b')), QStringLiteral("Second client"),
        1, Read, QByteArray(AuthSecretBytes, 'l')};
    PairStoreError error;
    QVERIFY(store.save({first, second}, error));
    ControlDialog dialog(directory, "test-selection");
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto *list = widget<QListWidget>(dialog, "mcpPairings");
    auto *revoke = widget<QPushButton>(dialog, "mcpRevoke");
    list->setCurrentRow(0);
    if (hasSelection) {
        list->selectionModel()->select(list->model()->index(1, 0), QItemSelectionModel::ClearAndSelect);
        QCOMPARE(list->currentRow(), 0);
        QCOMPARE(list->selectedItems().size(), 1);
        QCOMPARE(list->selectedItems().first()->data(Qt::UserRole).toString(), second.pairId);
        QVERIFY(revoke->isEnabled());
        QTest::mouseClick(revoke, Qt::LeftButton);
        QCOMPARE(list->count(), 1);
        QCOMPARE(list->item(0)->data(Qt::UserRole).toString(), first.pairId);
    } else {
        list->clearSelection();
        QCOMPARE(list->currentRow(), 0);
        QVERIFY(!revoke->isEnabled());
        QTest::mouseClick(revoke, Qt::LeftButton);
        QCOMPARE(list->count(), 2);
    }
    QList<PairCredential> records;
    QVERIFY(store.load(records, error));
    QCOMPARE(records.size(), hasSelection ? 1 : 2);
    QCOMPARE(records.first().pairId, first.pairId);
    QVERIFY(!dialog.server().isEnabled());
    QVERIFY(!QFileInfo::exists(dialog.server().discoveryPath()));
}

void McpControlTest::offlineFailure_data()
{
    QTest::addColumn<bool>("competing");
    QTest::newRow("read-only") << false;
    QTest::newRow("active-owner") << true;
}

void McpControlTest::offlineFailure()
{
    QFETCH(bool, competing);
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/offline-XXXXXX"));
    QVERIFY(root.isValid());
    const QString directory = root.path() + QStringLiteral("/server");
    PairCredentialStore store(directory, "test-offline-control");
    const PairCredential credential{QString(32, QLatin1Char('a')), QStringLiteral("Stored client"),
        1, Read, QByteArray(AuthSecretBytes, 'k')};
    PairStoreError error;
    QVERIFY(store.save({credential}, error));
    AutomationSessionServer active(directory, "test-offline-control");
    if (competing) QVERIFY(active.enable());
    const QString native = QDir::toNativeSeparators(store.filePath());
    const auto restore = qScopeGuard([&]() { SetFileAttributesW(reinterpret_cast<LPCWSTR>(native.utf16()), FILE_ATTRIBUTE_NORMAL); });
    if (!competing) QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(native.utf16()), FILE_ATTRIBUTE_READONLY));
    ControlDialog dialog(directory, "test-offline-control");
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto *list = widget<QListWidget>(dialog, "mcpPairings");
    QCOMPARE(list->count(), 1);
    list->setCurrentRow(0);
    QTest::mouseClick(widget<QPushButton>(dialog, "mcpRevoke"), Qt::LeftButton);
    QVERIFY(!dialog.server().isEnabled());
    QCOMPARE(list->count(), 1);
    QVERIFY(widget<QLabel>(dialog, "mcpStatus")->text().contains(ControlDialog::tr("; the last operation failed. A revocation that was not saved did not succeed.")));
    QList<PairCredential> records;
    QVERIFY(store.load(records, error));
    QCOMPARE(records.size(), 1);
    if (competing) {
        QCOMPARE(dialog.server().lastStoreError(), PairStoreError::Busy);
        QVERIFY(active.isEnabled());
        QVERIFY(widget<QLabel>(dialog, "mcpStatus")->text().contains(ControlDialog::tr(" Another instance owns this pairing store; manage it there or disable its interface first.")));
        active.disable();
    } else {
        QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(native.utf16()), FILE_ATTRIBUTE_NORMAL));
    }
    QTest::mouseClick(widget<QPushButton>(dialog, "mcpRevoke"), Qt::LeftButton);
    QCOMPARE(list->count(), 0);
    QVERIFY(!dialog.server().isEnabled());
    QVERIFY(!QFileInfo::exists(dialog.server().discoveryPath()));
    QVERIFY(!widget<QLabel>(dialog, "mcpStatus")->text().contains(ControlDialog::tr("; the last operation failed. A revocation that was not saved did not succeed.")));
}

int runInteropFixture(QApplication &app, const QString &statePath)
{
    QTemporaryDir root(QDir::currentPath() + QStringLiteral("/interop-XXXXXX"));
    if (!root.isValid()) return 24;
    ControlDialog dialog(root.path() + QStringLiteral("/server"), "test-stdio-interop");
    dialog.show();
    clickCheckBox(dialog, "mcpEnabled");
    if (!dialog.server().isEnabled()) return 25;
    int sequence = 0;
    int authenticated = 0;
    quint32 clientPid = 0;
    QObject::connect(&dialog.server(), &AutomationSessionServer::authenticated, &dialog, [&]() { ++authenticated; });
    QObject::connect(&dialog.server(), &AutomationSessionServer::pairingRequested, &dialog,
        [&](const PairingRequest &request) { clientPid = request.peer.processId; });
    QByteArray previous;
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &dialog, [&]() {
        QFile command(statePath + QStringLiteral(".command"));
        if (command.open(QIODevice::ReadOnly)) {
            const QJsonObject message = QJsonDocument::fromJson(command.read(257)).object();
            if (message.value(QStringLiteral("sequence")).toInt() > sequence) {
                sequence = message.value(QStringLiteral("sequence")).toInt();
                const QString action = message.value(QStringLiteral("command")).toString();
                // Maintenance-only fixed GUI events; this mode is absent from the product executable.
                if (action == QStringLiteral("approve")) QTest::mouseClick(widget<QPushButton>(dialog, "mcpApprove"), Qt::LeftButton);
                else if (action == QStringLiteral("reject")) QTest::mouseClick(widget<QPushButton>(dialog, "mcpReject"), Qt::LeftButton);
                else if (action == QStringLiteral("revoke")) {
                    widget<QListWidget>(dialog, "mcpPairings")->setCurrentRow(0);
                    QTest::mouseClick(widget<QPushButton>(dialog, "mcpRevoke"), Qt::LeftButton);
                } else if (action == QStringLiteral("disable") || action == QStringLiteral("enable")) {
                    const bool wanted = action == QStringLiteral("enable");
                    if (dialog.server().isEnabled() != wanted) clickCheckBox(dialog, "mcpEnabled");
                } else if (action == QStringLiteral("corrupt-store")) {
                    QFile store(root.path() + QStringLiteral("/server/pairings.dat"));
                    if (!store.open(QIODevice::WriteOnly) || store.write("invalid") != 7) { app.exit(29); return; }
                } else if (action == QStringLiteral("quit")) app.quit();
                else { qWarning("Unexpected interop fixture command"); app.exit(26); return; }
            }
        }
        wchar_t desktop[256]{};
        DWORD needed = 0;
        if (!GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, desktop, sizeof(desktop), &needed)) {
            qWarning("Interop fixture desktop query failed: %lu", GetLastError());
            app.exit(27); return;
        }
        const QJsonObject state{{QStringLiteral("pid"), int(GetCurrentProcessId())},
            {QStringLiteral("desktop"), QString::fromWCharArray(desktop)},
            {QStringLiteral("sequence"), sequence}, {QStringLiteral("authenticated"), authenticated},
            {QStringLiteral("client_pid"), int(clientPid)},
            {QStringLiteral("pending"), !widget<QLabel>(dialog, "mcpPairRequest")->text().isEmpty()},
            {QStringLiteral("pair_count"), dialog.server().pairings().size()},
            {QStringLiteral("enabled"), dialog.server().isEnabled()},
            {QStringLiteral("discovery"), dialog.server().discoveryPath()}};
        const QByteArray bytes = QJsonDocument(state).toJson();
        if (bytes == previous) return;
        QSaveFile output(statePath);
        output.setDirectWriteFallback(false);
        if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit()) {
            qWarning("Interop fixture state write failed: %s", qPrintable(output.errorString()));
            app.exit(28); return;
        }
        previous = bytes;
    });
    poll.start(50);
    QTimer::singleShot(180000, &app, &QCoreApplication::quit);
    return app.exec();
}

int writeSourceFixture(const QString &path)
{
    const QFileInfo file(path);
    const QString root = QDir::cleanPath(QDir::current().absoluteFilePath(QStringLiteral("out/mcp_product"))) + QLatin1Char('/');
    const QString canonical = file.canonicalFilePath();
    if (canonical.isEmpty() || !canonical.startsWith(root, Qt::CaseInsensitive)
        || file.fileName() != QStringLiteral("keymapdata.ini")) return 31;
    QSettings settings(canonical, QSettings::IniFormat);
    QKeyMapperQtCompat::setIniCodecUtf8(settings);
    if (settings.value(QStringLiteral("McpSaved/KeyMapData_OriginalKeys")).toString() != QStringLiteral("A#G|C|E")
        || settings.contains(QStringLiteral("McpSaved/CommonMappingData"))) return 32;
    const QVariantMap common{{QString::fromLatin1(QKeyMapperConstants::KEYMAPDATA_ORIGINALKEYS), QStringLiteral("E")},
        {QString::fromLatin1(QKeyMapperConstants::KEYMAPDATA_MAPPINGKEYS), QStringList{QStringLiteral("F")}},
        {QString::fromLatin1(QKeyMapperConstants::KEYMAPDATA_MAPPINGKEYS_KEYUP), QStringList{QStringLiteral("F")}},
        {QString::fromLatin1(QKeyMapperConstants::KEYMAPDATA_NOTE), QStringLiteral("共通保留")}};
    settings.setValue(QStringLiteral("McpSaved/CommonMappingData"), QVariantList{common});
    settings.sync();
    return settings.status() == QSettings::NoError ? 0 : 33;
}

class DemoOwnerTestWindow : public QWidget {
protected:
    void closeEvent(QCloseEvent *event) override
    {
        if (qEnvironmentVariable("QKM_TEST_OWNER_IGNORE_CLOSE") == QStringLiteral("1")
            && !QFileInfo::exists(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("release-child"))))
            event->ignore();
        else QWidget::closeEvent(event);
    }
};

int main(int argc, char **argv)
{
    const bool ownerChild = argc == 2 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--demo-configuration-only");
    bool unrelatedInherited = false, unrelatedProbeValid = true;
    QJsonObject unrelatedProbe;
    // Inspect the inherited slot before Qt starts allocating handles or worker threads.
    const quintptr unrelated = ownerChild ? quintptr(qEnvironmentVariable("QKM_TEST_UNRELATED_HANDLE").toULongLong()) : 0;
    if (unrelated) {
        DWORD flags = 0;
        HANDLE candidate = reinterpret_cast<HANDLE>(unrelated);
        if (!GetHandleInformation(candidate, &flags)) {
            const DWORD error = GetLastError();
            unrelatedProbeValid = error == ERROR_INVALID_HANDLE;
            unrelatedProbe = {{QStringLiteral("stage"), QStringLiteral("handle_information")}, {QStringLiteral("native_error"), int(error)}};
        }
        else {
            const QString name = qEnvironmentVariable("QKM_TEST_UNRELATED_EVENT");
            HANDLE expected = OpenEventW(SYNCHRONIZE, FALSE, reinterpret_cast<LPCWSTR>(name.utf16()));
            const DWORD openError = expected ? ERROR_SUCCESS : GetLastError();
            const FARPROC address = GetProcAddress(GetModuleHandleW(L"kernelbase.dll"), "CompareObjectHandles");
            const DWORD resolveError = address ? ERROR_SUCCESS : GetLastError();
            BOOL (WINAPI *compare)(HANDLE, HANDLE) = nullptr;
            static_assert(sizeof(compare) == sizeof(address), "Windows function pointer size");
            std::memcpy(&compare, &address, sizeof(compare));
            unrelatedProbeValid = expected && compare;
            if (unrelatedProbeValid) {
                unrelatedInherited = compare(candidate, expected);
                const DWORD error = unrelatedInherited ? ERROR_SUCCESS : GetLastError();
                unrelatedProbeValid = unrelatedInherited || error == ERROR_NOT_SAME_OBJECT;
                unrelatedProbe = {{QStringLiteral("stage"), QStringLiteral("compare")}, {QStringLiteral("native_error"), int(error)}};
            }
            else unrelatedProbe = {{QStringLiteral("stage"), expected ? QStringLiteral("resolve_compare") : QStringLiteral("open_event")},
                {QStringLiteral("native_error"), int(expected ? resolveError : openError)}};
            if (expected) CloseHandle(expected);
        }
    }
    if ((ownerChild || qEnvironmentVariable("QKM_TEST_CONFIGURATION_ONLY") == QStringLiteral("1"))
        && !initializeStartupPurpose(StartupPurpose::ConfigurationOnly)) return 25;
    if (ownerChild) prepareInheritedDemoSetup();
    // Match the product's baseline DPI, style and font without loading a personal INI.
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setFont(QFont(QString::fromLatin1(QKeyMapperConstants::FONTNAME_ENGLISH), 9));
    app.setQuitOnLastWindowClosed(false);
    if (app.arguments().size() == 6 && app.arguments().at(1) == QStringLiteral("--startup-choice-probe")) {
        ControlDialog dialog(app.arguments().at(2), "startup136", nullptr, app.arguments().at(3));
        if (configurationOnlyDemo() && !dialog.server().setDemoConfiguration(QStringLiteral("Demo136Seed"))) return 40;
        fixture(dialog.server());
        dialog.restoreStartupChoice();
        wchar_t desktop[256]{};
        DWORD needed = 0;
        if (!GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, desktop, sizeof(desktop), &needed)) return 41;
        const bool enabled = dialog.server().isEnabled();
        const bool failed = widget<QLabel>(dialog, "mcpStartupStatus")->text().contains(QStringLiteral("invalid_data"));
        const QByteArray result = QJsonDocument(QJsonObject{{QStringLiteral("enabled"), enabled},
            {QStringLiteral("desktop"), QString::fromWCharArray(desktop)}}).toJson(QJsonDocument::Compact);
        QFile output;
        if (!output.open(stdout, QIODevice::WriteOnly) || output.write(result) != result.size()) return 42;
        return enabled == (app.arguments().at(4) == QStringLiteral("1"))
            && failed == (app.arguments().at(5) == QStringLiteral("1")) ? 0 : 43;
    }
    if (ownerChild) {
        app.setQuitOnLastWindowClosed(true);
        DemoOwnerTestWindow window;
        window.setWindowTitle(QStringLiteral("QkmDemoOwnerTest"));
        window.setAttribute(Qt::WA_ShowWithoutActivating, true);
        window.show();
        const auto own = queryProcessIdentity(GetCurrentProcessId());
        wchar_t desktop[256]{};
        DWORD needed = 0;
        if (!own.ok() || !GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME,
                desktop, sizeof(desktop), &needed)) return 34;
        int managementRequests = 0;
        bool setupReady = false;
        const auto report = [&] {
            QSaveFile file(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("child-%1.json").arg(GetCurrentProcessId())));
            const QByteArray bytes = QJsonDocument(QJsonObject{{QStringLiteral("desktop"), QString::fromWCharArray(desktop)},
                {QStringLiteral("created"), QString::number(own.identity.creationTime)},
                {QStringLiteral("configuration_only"), configurationOnlyDemo()}, {QStringLiteral("setup_ready"), setupReady},
                {QStringLiteral("unrelated_inherited"), unrelatedInherited}, {QStringLiteral("management_requests"), managementRequests},
                {QStringLiteral("unrelated_probe_valid"), unrelatedProbeValid},
                {QStringLiteral("unrelated_probe"), unrelatedProbe},
                {QStringLiteral("setup_environment_cleared"), qEnvironmentVariableIsEmpty(DemoSetupRequestEnv)
                    && qEnvironmentVariableIsEmpty(DemoSetupAckEnv) && qEnvironmentVariableIsEmpty(DemoSetupParentEnv)
                    && qEnvironmentVariableIsEmpty(DemoSetupCreatedEnv)}}).toJson();
            return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
        };
        DemoSetupEndpoint setup([&] {
            ++managementRequests;
            if (qEnvironmentVariable("QKM_TEST_OWNER_REJECT_MANAGEMENT") == QStringLiteral("1")) { report(); return false; }
            window.show();
            return report();
        }, &window);
        setupReady = setup.ready();
        if (!report()) return 35;
        QTimer poll;
        QObject::connect(&poll, &QTimer::timeout, &app, [&] {
            if (QFileInfo::exists(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("release-child")))) window.close();
        });
        poll.start(20);
        QTimer::singleShot(30000, &app, &QCoreApplication::quit);
        return app.exec();
    }
    if (app.arguments().value(1) == QStringLiteral("--source-fixture") && app.arguments().size() == 3)
        return writeSourceFixture(app.arguments().at(2));
    if (app.arguments().value(1) == QStringLiteral("--interop-fixture") && app.arguments().size() == 3)
        return runInteropFixture(app, app.arguments().at(2));
    if (app.arguments().value(1) == QStringLiteral("--preview") && app.arguments().size() == 5) {
        QTemporaryDir root(QDir::currentPath() + QStringLiteral("/preview-XXXXXX"));
        if (!root.isValid()) return 20;
        const QString mode = app.arguments().at(3);
        if (mode != QStringLiteral("off") && mode != QStringLiteral("pending")
            && mode != QStringLiteral("paired-off") && mode != QStringLiteral("busy-off")
            && mode != QStringLiteral("startup-saved") && mode != QStringLiteral("startup-failed")
            && mode != QStringLiteral("startup-write-failed")
            && mode != QStringLiteral("window-approval")) return 24;
        const QString startup = mode.startsWith(QStringLiteral("startup-"))
            ? root.path() + QStringLiteral("/choice") : QString();
        Storage::DirectoryLease previewLease;
        if (mode == QStringLiteral("paired-off") || mode == QStringLiteral("busy-off") || mode == QStringLiteral("window-approval")) {
            PairCredentialStore store(root.path() + QStringLiteral("/server"), "test-preview");
            const PairCredential pair{QString(32, QLatin1Char('a')), QStringLiteral("预览客户端 Preview 日本語"),
                1, Read | Configure | RunMapping, QByteArray(AuthSecretBytes, 'k')};
            PairStoreError error;
            if (!store.save({pair}, error)) return 25;
        }
        QTranslator translator;
        if (!installUiTranslation(translator, app.arguments().at(2).toInt())) return 27;
        if (!startup.isEmpty()) {
            ControlDialog initial(root.path() + QStringLiteral("/server"), "test-preview", nullptr, startup);
            if (configurationOnlyDemo() && !initial.server().setDemoConfiguration(QStringLiteral("Demo136Seed"))) return 29;
            initial.restoreStartupChoice();
            initial.show();
            if (!QTest::qWaitForWindowExposed(&initial)) return 30;
            clickCheckBox(initial, "mcpEnabled");
            if (!initial.server().isEnabled()) return 31;
            QTest::mouseClick(widget<QPushButton>(initial, "mcpSaveStartup"), Qt::LeftButton);
        }
        if (mode == QStringLiteral("startup-failed")) {
            const auto own = queryProcessIdentity(GetCurrentProcessId());
            if (!own.ok() || !previewLease.acquire(root.path() + QStringLiteral("/server"), own.identity.userSid, false)) return 32;
        }
        ControlDialog dialog(root.path() + QStringLiteral("/server"), "test-preview", nullptr, startup);
        if (!startup.isEmpty()) {
            if (configurationOnlyDemo() && !dialog.server().setDemoConfiguration(QStringLiteral("Demo136Seed"))) return 33;
            dialog.restoreStartupChoice();
        }
        if (mode == QStringLiteral("pending")) {
            const auto own = queryProcessIdentity(GetCurrentProcessId());
            if (!own.ok()) return 21;
            // A UI-only fixture deliberately includes markup; no client is approved or authenticated.
            emit dialog.server().pairingRequested({QStringLiteral("preview"), QStringLiteral("preview"),
                QStringLiteral("<b>测试客户端</b> 日本語"), own.identity});
        }
        dialog.show();
        if (mode == QStringLiteral("startup-write-failed")) {
            const auto own = queryProcessIdentity(GetCurrentProcessId());
            if (!own.ok() || !previewLease.acquire(startup, own.identity.userSid, false)) return 34;
            clickCheckBox(dialog, "mcpEnabled");
            QTest::mouseClick(widget<QPushButton>(dialog, "mcpSaveStartup"), Qt::LeftButton);
        }
        if (mode == QStringLiteral("window-approval")) {
            clickCheckBox(dialog, "mcpEnabled");
            if (!dialog.server().isEnabled()) return 28;
            widget<QListWidget>(dialog, "mcpPairings")->setCurrentRow(0);
            auto *approval = widget<QDialog>(dialog, "mcpWindowApproval");
            QObject::connect(approval, &QDialog::finished, &app, &QCoreApplication::quit);
            QTimer::singleShot(0, &dialog, [&]() {
                QTest::mouseClick(widget<QPushButton>(dialog, "mcpWindowPermissions"), Qt::LeftButton);
            });
        }
        if (mode == QStringLiteral("paired-off") || mode == QStringLiteral("busy-off")) {
            widget<QListWidget>(dialog, "mcpPairings")->setCurrentRow(0);
            if (mode == QStringLiteral("busy-off")) {
                const auto own = queryProcessIdentity(GetCurrentProcessId());
                if (!own.ok() || !previewLease.acquire(root.path() + QStringLiteral("/server"), own.identity.userSid, false)) return 26;
                QTest::mouseClick(widget<QPushButton>(dialog, "mcpRevoke"), Qt::LeftButton);
            }
        }
        QObject::connect(&dialog, &QDialog::finished, &app, &QCoreApplication::quit);
        QTimer::singleShot(300, &dialog, [&]() {
            QWidget *target = mode == QStringLiteral("window-approval")
                ? widget<QDialog>(dialog, "mcpWindowApproval") : &dialog;
            wchar_t desktop[256]{};
            DWORD needed = 0;
            if (!GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, desktop, sizeof(desktop), &needed)) { app.exit(22); return; }
            const QJsonObject state{{QStringLiteral("pid"), int(GetCurrentProcessId())},
                {QStringLiteral("hwnd"), QString::number(quintptr(target->winId()))},
                {QStringLiteral("qt_dpr"), target->devicePixelRatioF()},
                {QStringLiteral("qt_logical_dpi"), target->logicalDpiX()},
                {QStringLiteral("native_dpi"), int(GetDpiForWindow(reinterpret_cast<HWND>(target->winId())))},
                {QStringLiteral("native_visible"), bool(IsWindowVisible(reinterpret_cast<HWND>(target->winId())))},
                {QStringLiteral("style"), dialog.style()->objectName()},
                {QStringLiteral("font"), dialog.font().family()},
                {QStringLiteral("font_points"), dialog.font().pointSizeF()},
                {QStringLiteral("interface_enabled"), dialog.server().isEnabled()},
                {QStringLiteral("pair_count"), dialog.server().pairings().size()},
                {QStringLiteral("revoke_enabled"), widget<QPushButton>(dialog, "mcpRevoke")->isEnabled()},
                {QStringLiteral("store_busy"), dialog.server().lastStoreError() == PairStoreError::Busy},
                {QStringLiteral("desktop"), QString::fromWCharArray(desktop)}};
            QSaveFile output(app.arguments().at(4));
            output.setDirectWriteFallback(false);
            const QByteArray bytes = QJsonDocument(state).toJson();
            if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit()) app.exit(23);
        });
        QTimer::singleShot(20000, &app, &QCoreApplication::quit);
        return app.exec();
    }
    McpControlTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mcp_control.moc"
