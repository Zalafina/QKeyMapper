#include "../../automation/qkm_control_dialog.h"
#include "../../automation/qkm_source_snapshot.h"
#include "../../automation/qkm_mapping_help.h"
#include "../../automation/qkm_mapping_changes.h"
#include "../../qkm_mapping_source_access.h"
#include "../../qkeymapper_qt_compat.h"
#include "../../qkeymapper_constants.h"
#include "ui_qgeneraladvanceddialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
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
#include <QProcess>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QTranslator>
#include <QVector>
#include <QtTest/QTest>
#include <thread>

using namespace QkmAutomation;

namespace {

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

template<class T> T *widget(ControlDialog &dialog, const char *name)
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

void clickCheckBox(ControlDialog &dialog, const char *name)
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
    void advancedEntryKeyboard();
    void controlFocusPolicy();
    void languageRefreshPreservesState();
    void pairingFromWidgets_data();
    void pairingFromWidgets();
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
    QCOMPARE(capabilities.size(), 5);
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
    QVERIFY(capability.value(QStringLiteral("limitations")).toArray().contains(QStringLiteral("preview_only_apply_unavailable")));
    const auto schema = capability.value(QStringLiteral("arguments_schema")).toObject();
    QVERIFY(!schema.value(QStringLiteral("additionalProperties")).toBool());
    QCOMPARE(schema.value(QStringLiteral("required")).toArray(), QJsonArray({QStringLiteral("action"), QStringLiteral("setting_name")}));
    QVERIFY(!schema.value(QStringLiteral("properties")).toObject().contains(QStringLiteral("source_revision")));
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
    QTest::newRow("read-only") << false;
    QTest::newRow("configure-run") << true;
}

void McpControlTest::pairingFromWidgets()
{
    QFETCH(bool, writable);
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
    QCOMPARE(pairs.at(0).scopes, quint32(writable ? Read | Configure | RunMapping : Read));
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

int main(int argc, char **argv)
{
    // Match the product's baseline DPI, style and font without loading a personal INI.
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setFont(QFont(QString::fromLatin1(QKeyMapperConstants::FONTNAME_ENGLISH), 9));
    app.setQuitOnLastWindowClosed(false);
    if (app.arguments().value(1) == QStringLiteral("--source-fixture") && app.arguments().size() == 3)
        return writeSourceFixture(app.arguments().at(2));
    if (app.arguments().value(1) == QStringLiteral("--interop-fixture") && app.arguments().size() == 3)
        return runInteropFixture(app, app.arguments().at(2));
    if (app.arguments().value(1) == QStringLiteral("--preview") && app.arguments().size() == 5) {
        QTemporaryDir root(QDir::currentPath() + QStringLiteral("/preview-XXXXXX"));
        if (!root.isValid()) return 20;
        const QString mode = app.arguments().at(3);
        if (mode != QStringLiteral("off") && mode != QStringLiteral("pending")
            && mode != QStringLiteral("paired-off") && mode != QStringLiteral("busy-off")) return 24;
        Storage::DirectoryLease previewLease;
        if (mode == QStringLiteral("paired-off") || mode == QStringLiteral("busy-off")) {
            PairCredentialStore store(root.path() + QStringLiteral("/server"), "test-preview");
            const PairCredential pair{QString(32, QLatin1Char('a')), QStringLiteral("预览客户端 Preview 日本語"),
                1, Read | Configure | RunMapping, QByteArray(AuthSecretBytes, 'k')};
            PairStoreError error;
            if (!store.save({pair}, error)) return 25;
        }
        QTranslator translator;
        if (!installUiTranslation(translator, app.arguments().at(2).toInt())) return 27;
        ControlDialog dialog(root.path() + QStringLiteral("/server"), "test-preview");
        if (mode == QStringLiteral("pending")) {
            const auto own = queryProcessIdentity(GetCurrentProcessId());
            if (!own.ok()) return 21;
            // A UI-only fixture deliberately includes markup; no client is approved or authenticated.
            emit dialog.server().pairingRequested({QStringLiteral("preview"), QStringLiteral("preview"),
                QStringLiteral("<b>测试客户端</b> 日本語"), own.identity});
        }
        dialog.show();
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
            wchar_t desktop[256]{};
            DWORD needed = 0;
            if (!GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, desktop, sizeof(desktop), &needed)) { app.exit(22); return; }
            const QJsonObject state{{QStringLiteral("pid"), int(GetCurrentProcessId())},
                {QStringLiteral("hwnd"), QString::number(quintptr(dialog.winId()))},
                {QStringLiteral("qt_dpr"), dialog.devicePixelRatioF()},
                {QStringLiteral("qt_logical_dpi"), dialog.logicalDpiX()},
                {QStringLiteral("native_dpi"), int(GetDpiForWindow(reinterpret_cast<HWND>(dialog.winId())))},
                {QStringLiteral("native_visible"), bool(IsWindowVisible(reinterpret_cast<HWND>(dialog.winId())))},
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
