#include "../../automation/qkm_control_dialog.h"
#include "../../automation/qkm_source_snapshot.h"
#include "../../automation/qkm_mapping_help.h"
#include "../../automation/qkm_mapping_changes.h"
#include "../../automation/qkm_macro_backup.h"
#include "../../automation/qkm_window_access.h"
#include "../../automation/qkm_demo_mode.h"
#include "../../automation/qkm_demo_process.h"
#include "../../automation/qkm_demo_host.h"
#include "../../automation/qkm_window_image.h"
#include "../../automation/qkm_safe_image_asset.h"
#include "../../qkm_mapping_source_access.h"
#include "../../qkeymapper_qt_compat.h"
#include "../../qkeymapper_constants.h"
#include "../../orderedmap/orderedmap.h"
#include "ui_qgeneraladvanceddialog.h"

#include <QApplication>
#include <QBuffer>
#include <QCloseEvent>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QImage>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
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
    void orderedMapMoveAssignment_data();
    void orderedMapMoveAssignment();
    void orderedMapCopyAndMoveConstruction();
    void defaultOffAndLanguage_data();
    void defaultOffAndLanguage();
    void startupChoice_data();
    void startupChoice();
    void startupChoiceFailures();
    void startupChoiceWithProductApplicationName();
    void persistentStorageDirectories();
    void advancedEntryKeyboard();
    void controlFocusPolicy();
    void languageRefreshPreservesState();
    void pairingFromWidgets_data();
    void pairingFromWidgets();
    void windowPermissions_data();
    void windowPermissions();
    void windowObservation();
    void windowAdvancedObservation();
    void windowIbInputObservation();
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
    void macroBackupDecodesQtExports();
    void macroBackupChecksLimitsAndFormat();
    void mappingTableBackupDecodesQtExports();
    void mappingTableBackupChecksLimitsAndShape();
    void configurationBackupEscapesIniIdentifiers();
    void configurationBackupDecodesQtProfiles();
    void configurationBackupChecksBoundsAndScope();
    void configurationImportPlansSelectedGroups();
    void configurationImportPreservesScope();
    void configurationImportDraftLifecycle();
    void configurationImportArguments();
    void configurationListArgumentsAndOwnership();
    void ibInputChangeArguments();
    void savePreviewArguments();
    void mappingRemovalArguments();
    void mappingRemovalLifecycle();
    void mappingChangeLifecycle();
    void mappingChangeLimits();
    void mappingStopLifecycle();
    void configurationSaveLifecycle();
    void configurationSaveExpiry();
    void configurationSaveLimits();
    void safeImageAssetAndTrayScanSecurity();
};

void McpControlTest::ibInputChangeArguments()
{
    const QString revision(64, QLatin1Char('a'));
    const QString source = revision + QStringLiteral(":row:0:1");
    const auto validate = [](const QJsonObject &arguments) {
        return validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), arguments);
    };
    const QJsonObject update{{QStringLiteral("action"), QStringLiteral("mapping.update")},
        {QStringLiteral("source_ref"), source}, {QStringLiteral("send_method"), 3}};
    const auto parsed = validate(update);
    QVERIFY(parsed.ok());
    QCOMPARE(parsed.action, MappingChangeAction::UpdateMapping);
    QCOMPARE(parsed.update.sourceRow, 1);
    QCOMPARE(parsed.update.sendMethod, 3);
    for (const auto &bad : QJsonArray{true, -1, 4, 1.5, QStringLiteral("3"), QJsonValue(QJsonValue::Null)}) {
        auto arguments = update; arguments.insert(QStringLiteral("send_method"), bad);
        QVERIFY(!validate(arguments).ok());
    }
    for (const auto &bad : {QStringLiteral(":row:00:1"), QStringLiteral(":row:0:-1"), QStringLiteral(":source:0")}) {
        auto arguments = update; arguments.insert(QStringLiteral("source_ref"), revision + bad);
        QVERIFY(!validate(arguments).ok());
    }
    auto extra = update; extra.insert(QStringLiteral("original_key"), QStringLiteral("B"));
    QVERIFY(validate(extra).ok());
    QCOMPARE(*validate(extra).update.originalKey, QStringLiteral("B"));
    const QJsonObject keys{{QStringLiteral("action"), QStringLiteral("mapping.update")},
        {QStringLiteral("source_ref"), source}, {QStringLiteral("mapping_key"), QStringLiteral("C")}};
    const auto keyUpdate = validate(keys);
    QVERIFY(keyUpdate.ok());
    QCOMPARE(keyUpdate.update.sendMethod, -1);
    QVERIFY(!keyUpdate.update.originalKey.has_value());
    QCOMPARE(*keyUpdate.update.mappingKey, QStringLiteral("C"));
    QVERIFY(!keyUpdate.update.keyUpMappingKey.has_value());
    QVERIFY(!keyUpdate.update.disabled.has_value());
    const QJsonObject disabled{{QStringLiteral("action"), QStringLiteral("mapping.update")},
        {QStringLiteral("source_ref"), source}, {QStringLiteral("disabled"), true}};
    for (const bool choice : {true, false}) {
        auto arguments = disabled; arguments.insert(QStringLiteral("disabled"), choice);
        const auto flagUpdate = validate(arguments);
        QVERIFY(flagUpdate.ok());
        QVERIFY(flagUpdate.update.disabled.has_value());
        QCOMPARE(*flagUpdate.update.disabled, choice);
        QCOMPARE(flagUpdate.update.sendMethod, -1);
    }
    for (const auto &bad : QJsonArray{0, 1, QStringLiteral("true"), QJsonValue(QJsonValue::Null)}) {
        auto arguments = disabled; arguments.insert(QStringLiteral("disabled"), bad);
        QVERIFY(!validate(arguments).ok());
    }
    for (const auto *name : {"original_key", "mapping_key", "key_up_mapping_key", "send_method"}) {
        auto arguments = disabled;
        arguments.insert(QString::fromLatin1(name), qstrcmp(name, "send_method") == 0 ? QJsonValue(0) : QJsonValue(QStringLiteral("B")));
        QVERIFY(!validate(arguments).ok());
    }
    auto keyUp = keys; keyUp.insert(QStringLiteral("key_up_mapping_key"), QString());
    QVERIFY(validate(keyUp).ok());
    QVERIFY(validate(keyUp).update.keyUpMappingKey->isEmpty());
    for (const auto *name : {"original_key", "mapping_key", "key_up_mapping_key"}) {
        for (const auto &bad : QJsonArray{true, 1, QStringLiteral("AB"), QStringLiteral("a"), QStringLiteral("\n"),
                                        QJsonValue(QJsonValue::Null)}) {
            auto invalidKey = keys; invalidKey.insert(QString::fromLatin1(name), bad);
            QVERIFY(!validate(invalidKey).ok());
        }
    }
    auto noFields = keys; noFields.remove(QStringLiteral("mapping_key"));
    QVERIFY(!validate(noFields).ok());
    auto unknown = keys; unknown.insert(QStringLiteral("field"), QStringLiteral("C"));
    QVERIFY(!validate(unknown).ok());
    auto wrongAction = keys; wrongAction.insert(QStringLiteral("action"), QStringLiteral("mapping.add"));
    wrongAction.insert(QStringLiteral("key_up_mapping_key"), QString());
    QVERIFY(!validate(wrongAction).ok());
    const QJsonObject preferences{{QStringLiteral("action"), QStringLiteral("ibinput.preferences.update")},
        {QStringLiteral("preferences_revision"), revision}, {QStringLiteral("preferred_backend"), QStringLiteral("razer")}};
    const auto settings = validate(preferences);
    QVERIFY(settings.ok());
    QCOMPARE(settings.action, MappingChangeAction::UpdateIbInputPreferences);
    QCOMPARE(settings.preferences.preferredBackend, QStringLiteral("razer"));
    auto invalid = preferences; invalid.remove(QStringLiteral("preferred_backend"));
    QVERIFY(!validate(invalid).ok());
    invalid.insert(QStringLiteral("unavailable_policy"), QStringLiteral("sendinput_before_start"));
    QVERIFY(validate(invalid).ok());
    invalid.insert(QStringLiteral("unavailable_policy"), QStringLiteral("always_sendinput"));
    QVERIFY(!validate(invalid).ok());
    invalid = preferences; invalid.insert(QStringLiteral("preferred_backend"), QStringLiteral("AnyDriver"));
    QVERIFY(!validate(invalid).ok());
    MappingChangeStore store;
    const MappingChangeOwner owner{QString(32, QLatin1Char('b')), QString(32, QLatin1Char('c')), 1};
    const auto preview = store.createUpdate(owner, parsed.update,
        {{QStringLiteral("target"), QJsonObject{{QStringLiteral("source_ref"), source}}}}, revision);
    QVERIFY(preview.has_value());
    const auto begun = store.begin(owner, preview->id);
    QVERIFY(begun.has_value());
    QCOMPARE(begun->preferencesRevision, revision);
    QCOMPARE(begun->update->sourceReference, source);
    store.finish(owner, preview->id, QStringLiteral("applied"), {{QStringLiteral("modified"), true}});
    QCOMPARE(store.operation(owner, preview->id).value(QStringLiteral("target")).toObject().value(QStringLiteral("source_ref")).toString(), source);
}

void McpControlTest::macroBackupDecodesQtExports()
{
    using namespace QKeyMapperConstants;
    const QVariantMap legacy{
        {QLatin1String(MACROLIST_FIELD_MACRONAME), QStringLiteral("旧 宏")},
        {QLatin1String(MACROLIST_FIELD_MACROCONTENT), QStringLiteral("A»B")},
        {QLatin1String(MACROLIST_FIELD_MACROCATEGORY), QStringLiteral("分類")},
        {QLatin1String(MACROLIST_FIELD_MACRONOTE), QStringLiteral("note,;=\r\n\"\\🙂")}
    };
    QVariantMap current = legacy;
    current[QLatin1String(MACROLIST_FIELD_MACRONAME)] = QStringLiteral("新 宏");
    current[QLatin1String(MACROLIST_FIELD_MACROCOMMENTS)] = QVariantList{QStringLiteral(" 段一 "), QString()};
    current[QStringLiteral("unknownField")] = true;
    QTemporaryDir files;
    QVERIFY(files.isValid());

    const QList<QVariantList> fixtures{{legacy}, {current}, {legacy, QStringLiteral("ignore"), current}};
    for (qsizetype i = 0; i < fixtures.size(); ++i) {
        const auto &entries = fixtures.at(int(i));
        const auto path = files.filePath(QString::number(i) + QStringLiteral(".ini"));
        {
            QSettings settings(path, QSettings::IniFormat);
            QKeyMapperQtCompat::setIniCodecUtf8(settings);
            settings.setValue(QLatin1String(MACROLIST_EXPORT), entries);
            settings.sync();
            QCOMPARE(settings.status(), QSettings::NoError);
        }
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto fromGui = decodeMacroBackup(file.readAll());
        QCOMPARE(fromGui.status, BackupDecodeStatus::Ok);
        QCOMPARE(fromGui.entries.size(), entries.size());
        QCOMPARE(fromGui.entries.first(), entries.first());
        QCOMPARE(fromGui.entries.last(), entries.last());
        if (entries.size() > 1) QVERIFY(fromGui.entries.at(1).toMap().isEmpty());

        const auto document = encodeMacroBackup(entries);
        QVERIFY(!document.isEmpty());
        const auto fromMcp = decodeMacroBackup(document);
        QCOMPARE(fromMcp.status, BackupDecodeStatus::Ok);
        QCOMPARE(fromMcp.entries, entries);

        const auto mcpPath = files.filePath(QStringLiteral("mcp-%1.ini").arg(i));
        QFile output(mcpPath);
        QVERIFY(output.open(QIODevice::WriteOnly));
        QCOMPARE(output.write(document), qint64(document.size()));
        output.close();
        QSettings readback(mcpPath, QSettings::IniFormat);
        QKeyMapperQtCompat::setIniCodecUtf8(readback);
        QCOMPARE(readback.value(QLatin1String(MACROLIST_EXPORT)).toList(), entries);
    }
}

void McpControlTest::macroBackupChecksLimitsAndFormat()
{
    QVariantList entries;
    for (int i = 0; i < MaxMacroBackupEntries; ++i) entries.append(QVariantMap{});
    const auto document = encodeMacroBackup(entries);
    QCOMPARE(decodeMacroBackup(document).status, BackupDecodeStatus::Ok);
    QCOMPARE(decodeMacroBackup("\xef\xbb\xbf; backup\r\n" + document).status, BackupDecodeStatus::Ok);
    QCOMPARE(decodeMacroBackup(document + document).status, BackupDecodeStatus::InvalidFormat);
    QCOMPARE(decodeMacroBackup(document.left(document.size() - 4)).status, BackupDecodeStatus::InvalidFormat);
    QCOMPARE(decodeMacroBackup("[Other]\nMacroList_Export=hello\n").status, BackupDecodeStatus::MissingRequiredField);
    QCOMPARE(decodeMacroBackup("[General]\nMacroList_Export=hello\n").status, BackupDecodeStatus::InvalidFormat);
    QCOMPARE(decodeMacroBackup(QByteArray(MaxMacroBackupBytes + 1, 'x')).status, BackupDecodeStatus::TooLarge);
    entries.append(QVariantMap{});
    QCOMPARE(decodeMacroBackup(encodeMacroBackup(entries)).status, BackupDecodeStatus::TooManyEntries);
    QCOMPARE(decodeMacroBackup(encodeMacroBackup({QDateTime::currentDateTimeUtc()})).status, BackupDecodeStatus::InvalidFormat);

    QByteArray multiField = "[General]\n";
    const auto keys = QStringList{QStringLiteral("A"), QStringLiteral("B")};
    QVERIFY(appendBackupIniValue(multiField, "KeyMapData_OriginalKeys", keys, MaxMacroBackupBytes));
    const auto before = multiField;
    QVERIFY(!appendBackupIniValue(multiField, "KeyMapData_MappingKeys", keys, int(before.size())));
    QCOMPARE(multiField, before);
    QVERIFY(appendBackupIniValue(multiField, "KeyMapData_MappingKeys", keys, MaxMacroBackupBytes));
    QTemporaryDir files;
    QVERIFY(files.isValid());
    QFile output(files.filePath(QStringLiteral("mapping-table.ini")));
    QVERIFY(output.open(QIODevice::WriteOnly));
    QCOMPARE(output.write(multiField), qint64(multiField.size()));
    output.close();
    QSettings settings(output.fileName(), QSettings::IniFormat);
    QCOMPARE(settings.value(QStringLiteral("KeyMapData_OriginalKeys")).toStringList(), keys);
    QCOMPARE(settings.value(QStringLiteral("KeyMapData_MappingKeys")).toStringList(), keys);
    QCOMPARE(settings.status(), QSettings::NoError);
}

void McpControlTest::mappingTableBackupDecodesQtExports()
{
    using namespace QKeyMapperConstants;
    const QString originals = QLatin1String(KEYMAPDATA_ORIGINALKEYS);
    const QString mappings = QLatin1String(KEYMAPDATA_MAPPINGKEYS);
    const QString notes = QLatin1String(KEYMAPDATA_NOTE);
    const QString categories = QLatin1String(KEYMAPDATA_CATEGORY);
    const QString comments = QLatin1String(KEYMAPDATA_MAPPINGKEYS_COMMENTS);
    const QString upComments = QLatin1String(KEYMAPDATA_MAPPINGKEYS_KEYUP_COMMENTS);
    const QStringList fields{originals, mappings, notes, categories, comments, upComments};
    QTemporaryDir files;
    QVERIFY(files.isValid());

    const QString note = QStringLiteral("  中文, 日本語; = \"\\\n段落  ");
    const QVariantList segmentNotes{QStringLiteral("段"), QString()};
    const QVariantList commentRows{QVariant(segmentNotes), QVariant(QVariantList{QStringLiteral("二")})};
    const QVariantMap multipleRows{{originals, QStringList{QStringLiteral("A"), QStringLiteral("B")}},
        {mappings, QStringList{QStringLiteral("C"), QStringLiteral("D")}}, {notes, QStringList{note, QString()}},
        {categories, QStringList{}}, {comments, commentRows}, {upComments, QVariantList{}}};
    const QVariantMap singleRow{{originals, QStringList{QStringLiteral("A")}}, {mappings, QStringList{QStringLiteral("C")}},
        {notes, QStringList{note}}, {comments, QVariantList{QVariant(segmentNotes)}}};

    int fileIndex = 0;
    for (const auto &values : {singleRow, multipleRows}) {
        const auto path = files.filePath(QString::number(fileIndex++) + QStringLiteral(".ini"));
        {
            QSettings settings(path, QSettings::IniFormat);
            QKeyMapperQtCompat::setIniCodecUtf8(settings);
            for (auto it = values.cbegin(); it != values.cend(); ++it) settings.setValue(it.key(), it.value());
            settings.setValue(QStringLiteral("Other/KeyMapData_OriginalKeys"), QStringLiteral("Z"));
            settings.sync();
            QCOMPARE(settings.status(), QSettings::NoError);
        }
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto fromGui = decodeMappingTableBackup(file.readAll(), fields);
        QByteArray encoded("[General]\n");
        for (auto it = values.cbegin(); it != values.cend(); ++it)
            QVERIFY(appendBackupIniValue(encoded, it.key().toLatin1(), it.value(), MaxTableBackupBytes));
        const auto fromMcp = decodeMappingTableBackup(encoded, fields);
        QSettings readback(path, QSettings::IniFormat);
        QKeyMapperQtCompat::setIniCodecUtf8(readback);
        for (const auto &decoded : {fromGui, fromMcp}) {
            QCOMPARE(decoded.status, BackupDecodeStatus::Ok);
            QCOMPARE(decoded.values.keys(), values.keys());
            for (const auto &field : values.keys()) {
                if (field == comments || field == upComments) {
                    QCOMPARE(decoded.values.value(field).toList(), readback.value(field).toList());
                } else {
                    QCOMPARE(decoded.values.value(field).toStringList(), readback.value(field).toStringList());
                }
            }
            QCOMPARE(decoded.values.value(notes).toStringList().first(), note);
        }
    }
}

void McpControlTest::mappingTableBackupChecksLimitsAndShape()
{
    using namespace QKeyMapperConstants;
    const QStringList fields{QLatin1String(KEYMAPDATA_ORIGINALKEYS), QLatin1String(KEYMAPDATA_MAPPINGKEYS),
        QLatin1String(KEYMAPDATA_MAPPINGKEYS_COMMENTS)};
    const QByteArray legacy("\xef\xbb\xbf; legacy\r\n[General]\r\nkeymapdata_originalkeys=A\r\nKeyMapData_MappingKeys=B\r\n");
    const auto read = [&](const QByteArray &bytes) { return decodeMappingTableBackup(bytes, fields).status; };
    QCOMPARE(read(legacy), BackupDecodeStatus::Ok);
    QCOMPARE(read("[Other]\nKeyMapData_OriginalKeys=A\nKeyMapData_MappingKeys=B\n"), BackupDecodeStatus::MissingRequiredField);
    QCOMPARE(read("[General]\nKeyMapData_OriginalKeys=A, C\nKeyMapData_MappingKeys=B\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(read(legacy + "KeyMapData_OriginalKeys=C\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(read(legacy + "KeyMapData_MappingKeys_Comments=\"unterminated\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(read(legacy + "KeyMapData_MappingKeys_Comments=@Variant(\\0\\0\\0\\t\\xff\\xff\\xff\\xff)\n"),
             BackupDecodeStatus::TooManyEntries);
    QCOMPARE(read(legacy + "KeyMapData_MappingKeys_Comments=@Variant(\\0\\0\\0\\x7f)\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(read(legacy + "Future=any ignored representation\n"), BackupDecodeStatus::Ok);
    QCOMPARE(read(QByteArray(MaxTableBackupBytes + 1, 'x')), BackupDecodeStatus::TooLarge);

    const auto many = QByteArray("A, ").repeated(MaxTableBackupRows - 1) + 'A';
    QCOMPARE(read("KeyMapData_OriginalKeys=" + many + "\nKeyMapData_MappingKeys=" + many + '\n'), BackupDecodeStatus::Ok);
    QCOMPARE(read("KeyMapData_OriginalKeys=" + many + ", A\nKeyMapData_MappingKeys=" + many + ", A\n"),
             BackupDecodeStatus::TooManyEntries);

    QByteArray wrongType("[General]\n");
    QVERIFY(appendBackupIniValue(wrongType, KEYMAPDATA_ORIGINALKEYS, 42, MaxTableBackupBytes));
    wrongType.append("KeyMapData_MappingKeys=B\n");
    QCOMPARE(read(wrongType), BackupDecodeStatus::InvalidFormat);
}

void McpControlTest::configurationBackupEscapesIniIdentifiers()
{
    const auto group = QStringLiteral("配置甲[01]%");
    const auto key = QStringLiteral("附注.é %");
    const auto text = QStringLiteral("已保存的内容");
    QByteArray document = '[' + encodeBackupIniIdentifier(group) + "]\n";
    QVERIFY(appendBackupIniValue(document, encodeBackupIniIdentifier(key), text, 128 * 1024));
    QTemporaryDir files;
    QVERIFY(files.isValid());
    QFile file(files.filePath(QStringLiteral("configuration-backup.ini")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(document), qint64(document.size()));
    file.close();
    QSettings readback(file.fileName(), QSettings::IniFormat);
    QKeyMapperQtCompat::setIniCodecUtf8(readback);
    QCOMPARE(readback.childGroups(), QStringList{group});
    QCOMPARE(readback.value(group + QLatin1Char('/') + key).toString(), text);
    QCOMPARE(readback.status(), QSettings::NoError);
}

void McpControlTest::configurationBackupDecodesQtProfiles()
{
    const QString first = QStringLiteral("配置甲[1]%");
    const QString second = QStringLiteral("Other");
    const QVariantList notes{QStringLiteral("段一"), QStringLiteral("段二")};
    const QVariantList macros{QVariantMap{{QStringLiteral("notes"), notes}}};
    const QVariantList common{QVariantMap{{QStringLiteral("color"), QColor(17, 34, 51, 68)}},
        QVariantMap{{QStringLiteral("color"), QColor()}}};
    const QVariantMap previous{{QStringLiteral("label"), QStringLiteral(" \t中文,; \" \\ \n")},
        {QStringLiteral("keys"), QStringList{QStringLiteral("A"), QStringLiteral("@B"), QStringLiteral("C")}},
        {QStringLiteral("position"), QPoint(-20, 300)}, {QStringLiteral("size"), QSize(1050, 722)},
        {QStringLiteral("mixed"), QVariantList{QStringLiteral("first"), QPoint(1, -2), QByteArray("b\0", 2)}},
        {QStringLiteral("macros"), macros}, {QStringLiteral("common"), common}, {QStringLiteral("blank"), QVariant()}};
    QTemporaryDir files;
    QVERIFY(files.isValid());
    const auto sourcePath = files.filePath(QStringLiteral("legacy.ini"));
    {
        QSettings file(sourcePath, QSettings::IniFormat);
        QKeyMapperQtCompat::setIniCodecUtf8(file);
        file.beginGroup(first);
        for (auto it = previous.cbegin(); it != previous.cend(); ++it) file.setValue(it.key(), it.value());
        file.endGroup();
        file.setValue(second + QStringLiteral("/KeyMapData_OriginalKeys"), QStringLiteral("A"));
        file.setValue(QStringLiteral("QKeyMapperGlobalSetting/Ignored"), QDateTime::currentDateTimeUtc());
        file.setValue(QStringLiteral("Ignored"), QDateTime::currentDateTimeUtc());
        file.sync();
        QCOMPARE(file.status(), QSettings::NoError);
    }
    QFile file(sourcePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto bytes = file.readAll();
    const auto legacy = decodeConfigurationBackup(bytes);
    QCOMPARE(legacy.status, BackupDecodeStatus::Ok);
    QCOMPARE(legacy.groups.size(), 2);
    QVERIFY(QFile::copy(sourcePath, files.filePath(QStringLiteral("reference.ini"))));
    QSettings reference(files.filePath(QStringLiteral("reference.ini")), QSettings::IniFormat);
    QKeyMapperQtCompat::setIniCodecUtf8(reference);
    for (const auto &group : legacy.groups) {
        QVERIFY(group.name == first || group.name == second);
        reference.beginGroup(group.name);
        QCOMPARE(group.values.keys(), reference.childKeys());
        for (auto it = group.values.cbegin(); it != group.values.cend(); ++it) {
            QCOMPARE(it.value().userType(), reference.value(it.key()).userType());
            QCOMPARE(it.value(), reference.value(it.key()));
        }
        reference.endGroup();
    }

    QByteArray mcp;
    for (const auto &name : {second, first}) {
        mcp.append('[').append(encodeBackupIniIdentifier(name)).append("]\n");
        reference.beginGroup(name);
        const auto keys = reference.childKeys();
        for (const auto &key : keys)
            QVERIFY(appendBackupIniValue(mcp, encodeBackupIniIdentifier(key), reference.value(key), MaxConfigurationBackupBytes));
        reference.endGroup();
    }
    const auto current = decodeConfigurationBackup(mcp);
    QCOMPARE(current.status, BackupDecodeStatus::Ok);
    QCOMPARE(current.groups.size(), 2);
    QCOMPARE(current.groups.at(0).name, second);
    QCOMPARE(current.groups.at(1).name, first);
    QCOMPARE(current.groups.at(1).values.value(QStringLiteral("position")).toPoint(), QPoint(-20, 300));
    QCOMPARE(current.groups.at(1).values.value(QStringLiteral("size")).toSize(), QSize(1050, 722));
    for (const auto &group : legacy.groups) {
        const auto &decoded = current.groups.at(group.name == second ? 0 : 1);
        QCOMPARE(decoded.values, group.values);
    }
}

void McpControlTest::configurationBackupChecksBoundsAndScope()
{
    const auto status = [](const QByteArray &bytes) { return decodeConfigurationBackup(bytes).status; };
    QCOMPARE(status("[General]\nLanguageIndex=1\n[QKeyMapperGlobalSetting]\nKey=A\n"), BackupDecodeStatus::MissingRequiredField);
    QCOMPARE(status("[Primary]\nValue=A\nValue=B\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(status("[Primary]\nValue=A\nvalue=B\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(status("[Primary]\nValue=A\n[primary]\nOther=B\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(status("[Primary%2FChild]\nValue=A\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(status("[Primary]\nBad%U=Q\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(status("[Primary]\nValue=@Point(1 unknowable)\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(status("[Primary]\nValue=\"unterminated\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(status("[Primary]\nValue=A\n[Broken\nOther=B\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(status("[Primary]\nValue=A\nBroken field\n"), BackupDecodeStatus::InvalidFormat);
    QCOMPARE(status(QByteArray(MaxConfigurationBackupBytes + 1, 'x')), BackupDecodeStatus::TooLarge);
    QByteArray groups;
    for (int index = 0; index < MaxConfigurationBackupGroups; ++index)
        groups.append("[Profile-").append(QByteArray::number(index)).append("]\nValue=A\n");
    QCOMPARE(status(groups), BackupDecodeStatus::Ok);
    QCOMPARE(status(groups + "[Overflow]\nValue=Z\n"), BackupDecodeStatus::TooManyEntries);

    QByteArray unsupported("[Profile]\n");
    QVERIFY(appendBackupIniValue(unsupported, "Value", QRect(0, 0, 2, 2), MaxConfigurationBackupBytes));
    QCOMPARE(status(unsupported), BackupDecodeStatus::InvalidFormat);
    QByteArray unknownColorModel("[Profile]\n");
    QVERIFY(appendBackupIniValue(unknownColorModel, "Value", QColor::fromHsv(0, 255, 255), MaxConfigurationBackupBytes));
    QCOMPARE(status(unknownColorModel), BackupDecodeStatus::InvalidFormat);
}

void McpControlTest::configurationImportPlansSelectedGroups()
{
    using Action = ConfigurationImportAction;
    using Conflict = ConfigurationImportConflict;
    using Status = ConfigurationImportStatus;
    const auto document = decodeConfigurationBackup("[General]\nLanguageIndex=0\n"
        "[Existing]\nKeyMapData_OriginalKeys=N\n"
        "[Unselected]\nKeyMapData_OriginalKeys=B\n"
        "[新组]\nKeyMapData_OriginalKeys=A\n");
    QCOMPARE(document.status, BackupDecodeStatus::Ok);
    const QString existing = QStringLiteral("Existing");
    const QString created = QStringLiteral("新组");
    const QString loaded = QStringLiteral("Loaded");
    const QStringList saved{loaded, existing};
    const QStringList selected{created, existing};

    const auto keep = planConfigurationImport(document, selected, saved, loaded, Conflict::Keep);
    QCOMPARE(keep.status, Status::Ok);
    QCOMPARE(keep.groups.size(), 2);
    QCOMPARE(keep.groups.first().name, created);
    QCOMPARE(keep.groups.first().action, Action::Create);
    QCOMPARE(keep.groups.first().values.value(QStringLiteral("KeyMapData_OriginalKeys")).toString(), QStringLiteral("A"));
    QCOMPARE(keep.groups.last().name, existing);
    QCOMPARE(keep.groups.last().action, Action::Keep);
    QVERIFY(keep.groups.last().values.isEmpty());

    const auto overwrite = planConfigurationImport(document, selected, saved, loaded, Conflict::Overwrite);
    QCOMPARE(overwrite.status, Status::Ok);
    QCOMPARE(overwrite.groups.size(), 2);
    QCOMPARE(overwrite.groups.last().action, Action::Merge);
    QCOMPARE(overwrite.groups.last().values, document.groups.first().values);
    QVERIFY(!overwrite.groups.last().values.contains(QStringLiteral("ExistingOnly")));

    const auto missing = planConfigurationImport(document, {created, QStringLiteral("General")}, saved, loaded, Conflict::Keep);
    QCOMPARE(missing.status, Status::UnknownGroup);
    QCOMPARE(missing.group, QStringLiteral("General"));
    QVERIFY(missing.groups.isEmpty());
    QCOMPARE(planConfigurationImport(document, {created, created}, saved, loaded, Conflict::Keep).status, Status::InvalidSelection);
    QCOMPARE(planConfigurationImport(document, {}, saved, loaded, Conflict::Keep).status, Status::InvalidSelection);
}

void McpControlTest::configurationImportPreservesScope()
{
    using Conflict = ConfigurationImportConflict;
    using Status = ConfigurationImportStatus;
    const QString loaded = QStringLiteral("Loaded");
    const auto document = decodeConfigurationBackup("[Loaded]\nPROCESSINFO_CUSTOMICONPATH=asset.ico\n"
        "[UnsafeTab]\nMappingTable_TabCustomImage_PathList=, asset.png\n"
        "[UnsafeProcess]\nProcessInfo_FilePath=C:/Synthetic/Example.exe\n"
        "[Clear]\nProcessInfo_FilePath=\nProcessInfo_CustomIconPath=\nMappingTable_TabCustomImage_PathList=, , \n");
    QCOMPARE(document.status, BackupDecodeStatus::Ok);
    const auto keepLoaded = planConfigurationImport(document, {loaded}, {loaded}, loaded, Conflict::Keep);
    QCOMPARE(keepLoaded.status, Status::Ok);
    QCOMPARE(keepLoaded.groups.first().action, ConfigurationImportAction::Keep);
    const auto overwriteLoaded = planConfigurationImport(document, {loaded}, {loaded}, loaded, Conflict::Overwrite);
    QCOMPARE(overwriteLoaded.status, Status::ActiveDestination);
    QCOMPARE(overwriteLoaded.group, loaded);
    const auto caseCollision = planConfigurationImport(document, {loaded}, {QStringLiteral("loaded")}, {}, Conflict::Keep);
    QCOMPARE(caseCollision.status, Status::AmbiguousDestination);
    const auto twoMatches = planConfigurationImport(document, {loaded}, {loaded, QStringLiteral("LOADED")}, {}, Conflict::Keep);
    QCOMPARE(twoMatches.status, Status::AmbiguousDestination);
    const auto icons = planConfigurationImport(document, {QStringLiteral("Clear"), loaded}, {}, {}, Conflict::Keep);
    QCOMPARE(icons.status, Status::ExternalPath);
    QCOMPARE(icons.field, QStringLiteral("PROCESSINFO_CUSTOMICONPATH"));
    QVERIFY(icons.groups.isEmpty());
    const auto tab = planConfigurationImport(document, {QStringLiteral("UnsafeTab")}, {}, {}, Conflict::Keep);
    QCOMPARE(tab.status, Status::ExternalPath);
    QCOMPARE(tab.field, QStringLiteral("MappingTable_TabCustomImage_PathList"));
    const auto process = planConfigurationImport(document, {QStringLiteral("UnsafeProcess")}, {}, {}, Conflict::Overwrite);
    QCOMPARE(process.status, Status::ExternalPath);
    QCOMPARE(process.field, QStringLiteral("ProcessInfo_FilePath"));
    const auto clear = planConfigurationImport(document, {QStringLiteral("Clear")}, {}, {}, Conflict::Overwrite);
    QCOMPARE(clear.status, Status::Ok);
    QCOMPARE(clear.groups.first().action, ConfigurationImportAction::Create);
}

void McpControlTest::configurationImportDraftLifecycle()
{
    MappingChangeStore store;
    const MappingChangeOwner owner{QStringLiteral("profile-instance"), QStringLiteral("original-pair"), 1};
    const MappingChangeOwner otherPair{owner.instance, QStringLiteral("other-pair"), owner.permissionEpoch};
    const MappingChangeOwner otherInstance{QStringLiteral("other-instance"), owner.pairId, owner.permissionEpoch};
    const MappingChangeOwner nextEpoch{owner.instance, owner.pairId, 2};
    const QString preferences(64, QLatin1Char('d'));
    const ConfigurationImportBinding binding{QString(64, QLatin1Char('a')), {QStringLiteral("Incoming")},
        QByteArray("[Incoming]\nNote=private-draft-value\n"), QString(64, QLatin1Char('b')), {}, true};
    const QJsonObject result{{QStringLiteral("saved"), false}};
    const QJsonObject description{{QStringLiteral("target"), QJsonObject{{QStringLiteral("instance_id"), owner.instance}}}};
    const auto prepare = [&] { return store.createConfigurationImport(owner, binding, description, preferences); };
    const auto preview = prepare();
    QVERIFY(preview);
    QVERIFY(!store.configurationImportDraft(owner, preview->id));
    QVERIFY(!store.stageConfigurationImport(*preview, result));
    QVERIFY(!store.begin(otherPair, preview->id));
    QVERIFY(!store.cancel(nextEpoch, preview->id));
    QVERIFY(store.cancel(owner, preview->id));
    QVERIFY(!store.begin(owner, preview->id));

    const auto accepted = prepare();
    QVERIFY(accepted);
    const auto begun = store.begin(owner, accepted->id);
    QVERIFY(begun && store.stageConfigurationImport(*begun, result));
    const auto draft = store.configurationImportDraft(owner, accepted->id);
    QVERIFY(draft);
    QCOMPARE(draft->document, binding.document);
    QCOMPARE(draft->groups, binding.groups);
    QVERIFY(!store.stageConfigurationImport(*begun, result));
    for (const auto &different : {otherPair, otherInstance, nextEpoch}) {
        QVERIFY(!store.configurationImportDraft(different, accepted->id));
        QVERIFY(!store.cancel(different, accepted->id));
    }
    const auto staged = store.operation(owner, accepted->id);
    QVERIFY(!QJsonDocument(staged).toJson().contains("private-draft-value"));
    QCOMPARE(staged.value(QStringLiteral("result")).toObject().value(QStringLiteral("draft_state")), QJsonValue(QStringLiteral("pending")));
    ConfigurationImportBinding save;
    save.contextRevision = binding.contextRevision;
    save.draftId = accepted->id;
    QVERIFY(!store.createConfigurationImport(otherPair, save, description, preferences));
    auto stale = save;
    stale.contextRevision = QString(64, QLatin1Char('f'));
    QVERIFY(!store.createConfigurationImport(owner, stale, description, preferences));
    const auto pendingSave = store.createConfigurationImport(owner, save, description, preferences);
    QVERIFY(pendingSave && store.cancel(owner, pendingSave->id));
    QVERIFY(store.configurationImportDraft(owner, accepted->id));
    const auto retry = store.createConfigurationImport(owner, save, description, preferences);
    QVERIFY(retry && store.begin(owner, retry->id));
    store.finish(owner, retry->id, QStringLiteral("failed"), {}, QStringLiteral("save_write_failed"));
    QVERIFY(store.configurationImportDraft(owner, accepted->id));
    store.completeConfigurationImportSave(owner, accepted->id);
    QVERIFY(!store.configurationImportDraft(owner, accepted->id));
    QVERIFY(!store.createConfigurationImport(owner, save, description, preferences));
    QCOMPARE(store.operation(owner, accepted->id).value(QStringLiteral("result")).toObject()
        .value(QStringLiteral("draft_state")), QJsonValue(QStringLiteral("saved")));

    const auto cancelled = prepare();
    QVERIFY(cancelled);
    const auto cancelling = store.begin(owner, cancelled->id);
    QVERIFY(cancelling && store.stageConfigurationImport(*cancelling, result));
    QVERIFY(store.cancel(owner, cancelled->id) && store.cancel(owner, cancelled->id));
    QVERIFY(!store.configurationImportDraft(owner, cancelled->id));
    QCOMPARE(store.operation(owner, cancelled->id).value(QStringLiteral("result")).toObject()
        .value(QStringLiteral("draft_state")), QJsonValue(QStringLiteral("cancelled")));

    const auto expired = prepare();
    QVERIFY(expired);
    const auto expiring = store.begin(owner, expired->id);
    QVERIFY(expiring && store.stageConfigurationImport(*expiring, result));
    MappingChangeStoreTestAccess::expireResult(store, expired->id);
    QVERIFY(!store.configurationImportDraft(owner, expired->id));
    QVERIFY(!store.operation(owner, expired->id).value(QStringLiteral("result_known")).toBool());
}

void McpControlTest::configurationImportArguments()
{
    const QString revision(64, QLatin1Char('a'));
    const QByteArray document("[配置甲]\nKeyMapData_OriginalKeys=A\nKeyMapData_MappingKeys=B\n");
    const QString action = QStringLiteral("action");
    const QString method = QStringLiteral("qkm_preview_changes");
    const QJsonObject arguments{{action, QStringLiteral("configuration.import")},
        {QStringLiteral("catalog_revision"), revision}, {QStringLiteral("groups"), QJsonArray{QStringLiteral("配置甲")}},
        {QStringLiteral("on_conflict"), QStringLiteral("overwrite")},
        {QStringLiteral("document_base64"), QString::fromLatin1(document.toBase64())}};
    const auto parsed = validateMappingChangeRequest(method, arguments);
    QVERIFY(parsed.ok());
    QCOMPARE(parsed.action, MappingChangeAction::ImportConfiguration);
    QCOMPARE(parsed.configurationImport.document, document);
    QCOMPARE(parsed.configurationImport.groups, QStringList{QStringLiteral("配置甲")});
    QVERIFY(parsed.configurationImport.overwrite);
    const auto invalid = [&](const QString &key, const QJsonValue &value) {
        auto candidate = arguments;
        candidate.insert(key, value);
        return !validateMappingChangeRequest(method, candidate).ok();
    };
    QVERIFY(invalid(QStringLiteral("file_name"), QStringLiteral("source.ini")));
    QVERIFY(invalid(QStringLiteral("groups"), QJsonArray()));
    QVERIFY(invalid(QStringLiteral("groups"), QJsonArray{QStringLiteral("Same"), QStringLiteral("Same")}));
    QVERIFY(invalid(QStringLiteral("on_conflict"), QStringLiteral("replace")));
    QVERIFY(invalid(QStringLiteral("document_base64"), QStringLiteral("@")));
    QVERIFY(invalid(QStringLiteral("catalog_revision"), revision.toUpper()));
    const QString draft(32, QLatin1Char('b'));
    const QJsonObject save{{action, QStringLiteral("configuration.save_import")}, {QStringLiteral("import_draft_id"), draft}};
    const auto saveRequest = validateMappingChangeRequest(method, save);
    QVERIFY(saveRequest.ok());
    QCOMPARE(saveRequest.action, MappingChangeAction::SaveConfigurationImport);
    QCOMPARE(saveRequest.configurationImport.draftId, draft);
    QVERIFY(!validateMappingChangeRequest(method, {{action, QStringLiteral("configuration.save_import")},
        {QStringLiteral("import_draft_id"), draft.toUpper()}}).ok());
}

void McpControlTest::configurationListArgumentsAndOwnership()
{
    const QString revision(64, QLatin1Char('a'));
    const QString method = QStringLiteral("qkm_preview_changes");
    const QJsonObject move{{QStringLiteral("action"), QStringLiteral("configuration.move")},
        {QStringLiteral("catalog_revision"), revision}, {QStringLiteral("group"), QStringLiteral("配置甲")},
        {QStringLiteral("target_group"), QStringLiteral("Second")}};
    const auto parsed = validateMappingChangeRequest(method, move);
    QVERIFY(parsed.ok());
    QCOMPARE(parsed.action, MappingChangeAction::ChangeConfigurationList);
    QCOMPARE(parsed.configurationList.group, QStringLiteral("配置甲"));
    QCOMPARE(parsed.configurationList.targetGroup, QStringLiteral("Second"));
    QVERIFY(!parsed.configurationList.reset());

    auto malformed = move;
    malformed.insert(QStringLiteral("group"), QStringLiteral("bad\nname"));
    QVERIFY(!validateMappingChangeRequest(method, malformed).ok());
    malformed = move;
    malformed.remove(QStringLiteral("target_group"));
    QVERIFY(!validateMappingChangeRequest(method, malformed).ok());
    malformed = move;
    malformed.insert(QStringLiteral("save"), false);
    QVERIFY(!validateMappingChangeRequest(method, malformed).ok());
    const QJsonObject reset{{QStringLiteral("action"), QStringLiteral("configuration.reset_order")},
        {QStringLiteral("catalog_revision"), revision}};
    const auto resetRequest = validateMappingChangeRequest(method, reset);
    QVERIFY(resetRequest.ok());
    QVERIFY(resetRequest.configurationList.reset());
    QJsonObject remove = move;
    remove.insert(QStringLiteral("action"), QStringLiteral("configuration.remove"));
    QVERIFY(!validateMappingChangeRequest(method, remove).ok());
    remove.remove(QStringLiteral("target_group"));
    const auto removal = validateMappingChangeRequest(method, remove);
    QVERIFY(removal.ok());
    QVERIFY(removal.configurationList.removesGroup());
    QJsonObject load = remove;
    load.insert(QStringLiteral("action"), QStringLiteral("configuration.load"));
    const auto selection = validateMappingChangeRequest(method, load);
    QVERIFY(selection.ok());
    QVERIFY(selection.configurationList.loadsGroup());
    auto globalLoad = load;
    globalLoad.insert(QStringLiteral("group"), QStringLiteral("QKeyMapperGlobalSetting"));
    const auto globalSelection = validateMappingChangeRequest(method, globalLoad);
    QVERIFY(globalSelection.ok());
    QVERIFY(globalSelection.configurationList.loadsGroup());
    QCOMPARE(globalSelection.configurationList.group, QStringLiteral("QKeyMapperGlobalSetting"));
    auto emptyNameLoad = load;
    emptyNameLoad.insert(QStringLiteral("group"), QString());
    QVERIFY(!validateMappingChangeRequest(method, emptyNameLoad).ok());
    load.insert(QStringLiteral("file_name"), QStringLiteral("other.ini"));
    QVERIFY(!validateMappingChangeRequest(method, load).ok());
    QJsonObject loadEmpty = reset;
    loadEmpty.insert(QStringLiteral("action"), QStringLiteral("configuration.load_empty"));
    const auto emptyRequest = validateMappingChangeRequest(method, loadEmpty);
    QVERIFY(emptyRequest.ok());
    QVERIFY(emptyRequest.configurationList.loadsEmpty());
    QVERIFY(!emptyRequest.configurationList.loadsGroup());
    loadEmpty.insert(QStringLiteral("group"), QStringLiteral("Other"));
    QVERIFY(!validateMappingChangeRequest(method, loadEmpty).ok());

    auto binding = parsed.configurationList;
    MappingChangeStore store;
    const MappingChangeOwner owner{QString(32, QLatin1Char('a')), QString(32, QLatin1Char('b')), 1};
    QVERIFY(!store.createConfigurationList(owner, binding, {}, {}));
    binding.contextRevision = QString(64, QLatin1Char('c'));
    const auto preview = store.createConfigurationList(owner, binding, {}, {});
    QVERIFY(preview);
    auto other = owner;
    ++other.permissionEpoch;
    QVERIFY(!store.begin(other, preview->id));
    const auto applying = store.begin(owner, preview->id);
    QVERIFY(applying && applying->configurationList);
    QCOMPARE(applying->configurationList->contextRevision, binding.contextRevision);
    QVERIFY(!store.begin(owner, preview->id));
    auto removalBinding = removal.configurationList;
    removalBinding.contextRevision = binding.contextRevision;
    const auto removalPreview = store.createConfigurationList(owner, removalBinding, {}, {});
    QVERIFY(removalPreview && removalPreview->configurationList->removesGroup());
    auto loadBinding = selection.configurationList;
    loadBinding.contextRevision = binding.contextRevision;
    const auto loadPreview = store.createConfigurationList(owner, loadBinding, {}, {});
    QVERIFY(loadPreview && loadPreview->configurationList->loadsGroup());
    auto emptyBinding = emptyRequest.configurationList;
    emptyBinding.contextRevision = binding.contextRevision;
    const auto emptyPreview = store.createConfigurationList(owner, emptyBinding, {}, {});
    QVERIFY(emptyPreview && emptyPreview->configurationList->loadsEmpty());
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static bool previewRequestValid(const QString &previewMethod, const QJsonObject &arguments)
{
    return validateMappingChangeRequest(previewMethod, arguments).ok();
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyGamepadValidation(const QString &previewMethod, const QJsonObject &input)
{
    const QJsonValue sourceTabRef = input.value(QStringLiteral("source_tab_ref"));
    const auto validateGyro2MouseOptions = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_gyro2mouse_options")},
                {QStringLiteral("source_tab_ref"), sourceTabRef},
                {QStringLiteral("gyro2mouse_options"), fields}});
    };
    const auto gyro2MouseOptionsValid = [&](const QJsonObject &fields) {
        return previewRequestValid(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_gyro2mouse_options")},
                {QStringLiteral("source_tab_ref"), sourceTabRef},
                {QStringLiteral("gyro2mouse_options"), fields}});
    };
    {
        const auto gyroAll = validateGyro2MouseOptions({
            {QStringLiteral("mouse_x_source"), 0},
            {QStringLiteral("mouse_y_source"), 2},
            {QStringLiteral("mouse_x_revert"), true},
            {QStringLiteral("mouse_y_revert"), false},
        });
        QVERIFY(gyroAll.ok() && gyroAll.update.gyro2MouseXSource == 0 && gyroAll.update.gyro2MouseYSource == 2
            && gyroAll.update.gyro2MouseXRevert == true && gyroAll.update.gyro2MouseYRevert == false);
    }
    {
        const auto gyroSubset = validateGyro2MouseOptions({{QStringLiteral("mouse_y_revert"), true}});
        QVERIFY(gyroSubset.ok() && !gyroSubset.update.gyro2MouseXSource.has_value()
            && !gyroSubset.update.gyro2MouseYSource.has_value() && !gyroSubset.update.gyro2MouseXRevert.has_value()
            && gyroSubset.update.gyro2MouseYRevert == true);
    }
    QVERIFY(gyro2MouseOptionsValid({{QStringLiteral("mouse_x_source"), 1}, {QStringLiteral("mouse_y_source"), 1}}));
    for (const QJsonObject &badGyro : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("mouse_x_source"), -1}},
        QJsonObject{{QStringLiteral("mouse_x_source"), 3}},
        QJsonObject{{QStringLiteral("mouse_y_source"), -1}},
        QJsonObject{{QStringLiteral("mouse_y_source"), 3}},
        QJsonObject{{QStringLiteral("mouse_x_source"), 1.5}},
        QJsonObject{{QStringLiteral("mouse_y_source"), true}},
        QJsonObject{{QStringLiteral("mouse_x_source"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("mouse_x_revert"), 1}},
        QJsonObject{{QStringLiteral("mouse_y_revert"), QStringLiteral("true")}},
        QJsonObject{{QStringLiteral("mouse_x_revert"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("mouse_x_source"), 1}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!gyro2MouseOptionsValid(badGyro));
    }

    const auto validateGamepadThresholds = [&](const QJsonObject &groups) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_gamepad_thresholds")},
                {QStringLiteral("source_tab_ref"), sourceTabRef},
                {QStringLiteral("gamepad_thresholds"), groups}});
    };
    const auto gamepadThresholdsValid = [&](const QJsonObject &groups) {
        return previewRequestValid(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_gamepad_thresholds")},
                {QStringLiteral("source_tab_ref"), sourceTabRef},
                {QStringLiteral("gamepad_thresholds"), groups}});
    };
    {
        const auto threshAll = validateGamepadThresholds({
            {QStringLiteral("lt"), QJsonObject{{QStringLiteral("press"), 80}, {QStringLiteral("light_press"), 50}, {QStringLiteral("release"), 20}}},
            {QStringLiteral("rt"), QJsonObject{{QStringLiteral("press"), 100}, {QStringLiteral("light_press"), 100}, {QStringLiteral("release"), 100}}},
            {QStringLiteral("ls"), QJsonObject{{QStringLiteral("push"), 0}, {QStringLiteral("light_push"), 0}, {QStringLiteral("release"), 0}}},
            {QStringLiteral("rs"), QJsonObject{{QStringLiteral("push"), 75}, {QStringLiteral("light_push"), 40}, {QStringLiteral("release"), 10}}},
        });
        QVERIFY(threshAll.ok() && threshAll.update.gamepadLtPress == 80 && threshAll.update.gamepadLtLightPress == 50
            && threshAll.update.gamepadLtRelease == 20 && threshAll.update.gamepadRtPress == 100
            && threshAll.update.gamepadLsPush == 0 && threshAll.update.gamepadRsRelease == 10);
    }
    {
        const auto threshSubset = validateGamepadThresholds({
            {QStringLiteral("rs"), QJsonObject{{QStringLiteral("push"), 85}, {QStringLiteral("light_push"), 45}}},
        });
        QVERIFY(threshSubset.ok() && !threshSubset.update.gamepadLtPress.has_value()
            && threshSubset.update.gamepadRsPush == 85 && threshSubset.update.gamepadRsLightPush == 45
            && !threshSubset.update.gamepadRsRelease.has_value());
    }
    for (const QJsonObject &badThresholds : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("lt"), QJsonObject{}}},
        QJsonObject{{QStringLiteral("unknown"), QJsonObject{{QStringLiteral("press"), 50}}}},
        QJsonObject{{QStringLiteral("lt"), QJsonObject{{QStringLiteral("push"), 50}}}},
        QJsonObject{{QStringLiteral("ls"), QJsonObject{{QStringLiteral("press"), 50}}}},
        QJsonObject{{QStringLiteral("lt"), QJsonObject{{QStringLiteral("press"), -1}}}},
        QJsonObject{{QStringLiteral("rt"), QJsonObject{{QStringLiteral("press"), 101}}}},
        QJsonObject{{QStringLiteral("ls"), QJsonObject{{QStringLiteral("push"), 1.5}}}},
        QJsonObject{{QStringLiteral("rs"), QJsonObject{{QStringLiteral("push"), true}}}},
        QJsonObject{{QStringLiteral("lt"), QJsonObject{{QStringLiteral("press"), QJsonValue::Null}}}},
        QJsonObject{{QStringLiteral("lt"), QJsonObject{{QStringLiteral("light_press"), 60}, {QStringLiteral("release"), 65}}}},
        QJsonObject{{QStringLiteral("rt"), QJsonObject{{QStringLiteral("press"), 40}, {QStringLiteral("light_press"), 45}}}},
        QJsonObject{{QStringLiteral("ls"), QJsonObject{{QStringLiteral("push"), 30}, {QStringLiteral("release"), 35}}}},
    }) {
        QVERIFY(!gamepadThresholdsValid(badThresholds));
    }

    const auto validateGamepadTouchpad = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_gamepad_touchpad")},
                {QStringLiteral("source_tab_ref"), sourceTabRef},
                {QStringLiteral("gamepad_touchpad"), fields}});
    };
    const auto gamepadTouchpadValid = [&](const QJsonObject &fields) {
        return previewRequestValid(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_gamepad_touchpad")},
                {QStringLiteral("source_tab_ref"), sourceTabRef},
                {QStringLiteral("gamepad_touchpad"), fields}});
    };
    {
        const auto touchpadValid = validateGamepadTouchpad({
            {QStringLiteral("x_speed"), 0},
            {QStringLiteral("y_speed"), 200},
            {QStringLiteral("invert_x"), true},
            {QStringLiteral("invert_y"), false},
        });
        QVERIFY(touchpadValid.ok() && touchpadValid.update.gamepadTouchpadXSpeed == 0.0
            && touchpadValid.update.gamepadTouchpadYSpeed == 200.0
            && touchpadValid.update.gamepadTouchpadInvertX == true
            && touchpadValid.update.gamepadTouchpadInvertY == false);
    }
    {
        const auto touchpadDecimal = validateGamepadTouchpad({{QStringLiteral("x_speed"), 12.34}});
        QVERIFY(touchpadDecimal.ok() && qRound(*touchpadDecimal.update.gamepadTouchpadXSpeed * 100.0) == 1234);
    }
    for (const QJsonObject &badTouchpad : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("x_speed"), -0.01}},
        QJsonObject{{QStringLiteral("y_speed"), 200.01}},
        QJsonObject{{QStringLiteral("x_speed"), 1.234}},
        QJsonObject{{QStringLiteral("y_speed"), 0.005}},
        QJsonObject{{QStringLiteral("x_speed"), QStringLiteral("0.5")}},
        QJsonObject{{QStringLiteral("x_speed"), true}},
        QJsonObject{{QStringLiteral("x_speed"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("invert_x"), 1}},
        QJsonObject{{QStringLiteral("invert_y"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("invert_x"), true}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!gamepadTouchpadValid(badTouchpad));
    }

    const auto acceptVirtualGamepadValid = [&](const QJsonValue &value) {
        return previewRequestValid(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_accept_virtual_gamepad_input")},
                {QStringLiteral("source_tab_ref"), sourceTabRef},
                {QStringLiteral("accept_virtual_gamepad_input"), value}});
    };
    for (bool value : {true, false}) {
        const auto result = validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_accept_virtual_gamepad_input")},
                {QStringLiteral("source_tab_ref"), sourceTabRef},
                {QStringLiteral("accept_virtual_gamepad_input"), value}});
        QVERIFY(result.ok() && result.update.acceptVirtualGamepadInput.has_value());
        QCOMPARE(*result.update.acceptVirtualGamepadInput, value);
    }
    for (const auto &value : {QJsonValue(1), QJsonValue(QStringLiteral("true")), QJsonValue(QJsonValue::Null)})
        QVERIFY(!acceptVirtualGamepadValid(value));
    QVERIFY(!previewRequestValid(previewMethod,
        {{QStringLiteral("action"), QStringLiteral("configuration.set_accept_virtual_gamepad_input")},
            {QStringLiteral("source_tab_ref"), sourceTabRef}}));
    QVERIFY(!previewRequestValid(previewMethod,
        {{QStringLiteral("action"), QStringLiteral("configuration.set_accept_virtual_gamepad_input")},
            {QStringLiteral("source_tab_ref"), sourceTabRef},
            {QStringLiteral("accept_virtual_gamepad_input"), true},
            {QStringLiteral("extra"), 1}}));
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyVButtonPanelValidation(const QString &previewMethod, const QJsonObject &input)
{
    const QJsonValue sourceTabRef = input.value(QStringLiteral("source_tab_ref"));
    const auto validateVButtonPanel = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_vbutton_panel_settings")},
                {QStringLiteral("source_tab_ref"), sourceTabRef},
                {QStringLiteral("vbutton_panel"), fields}});
    };
    const auto vbuttonPanelValid = [&](const QJsonObject &fields) {
        return previewRequestValid(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_vbutton_panel_settings")},
                {QStringLiteral("source_tab_ref"), sourceTabRef},
                {QStringLiteral("vbutton_panel"), fields}});
    };
    {
        const auto panelValid = validateVButtonPanel({
            {QStringLiteral("columns"), 1},
            {QStringLiteral("max_rows"), 999},
            {QStringLiteral("button_width"), 4},
            {QStringLiteral("button_height"), 640},
            {QStringLiteral("opacity"), 0.75},
            {QStringLiteral("always_on_top"), false},
            {QStringLiteral("default_show"), true},
            {QStringLiteral("margin"), 0},
            {QStringLiteral("radius"), 100},
            {QStringLiteral("drag_enabled"), false},
            {QStringLiteral("reference_point"), 11},
            {QStringLiteral("offset_x"), -50000},
            {QStringLiteral("offset_y"), 50000},
            {QStringLiteral("font_size"), 72},
            {QStringLiteral("font_weight"), 2},
            {QStringLiteral("font_family"), QStringLiteral("  Consolas  ")},
            {QStringLiteral("bg_color"), QStringLiteral("#AA112233")},
            {QStringLiteral("button_color"), QStringLiteral("#224466")},
            {QStringLiteral("pressed_color"), QStringLiteral("#ABCDEF")},
            {QStringLiteral("locked_color"), QStringLiteral("#445566")},
            {QStringLiteral("text_color"), QStringLiteral("#DDEEFF")},
        });
        QVERIFY(panelValid.ok() && panelValid.update.vbuttonPanel.has_value());
        QCOMPARE(panelValid.update.vbuttonPanel->columns, std::optional<int>(1));
        QCOMPARE(panelValid.update.vbuttonPanel->fontFamily, std::optional<QString>(QStringLiteral("Consolas")));
        QCOMPARE(panelValid.update.vbuttonPanel->bgColor, std::optional<QString>(QStringLiteral("#aa112233")));
        QCOMPARE(panelValid.update.vbuttonPanel->pressedColor, std::optional<QString>(QStringLiteral("#abcdef")));
    }
    {
        const auto panelEmptyFont = validateVButtonPanel({{QStringLiteral("font_family"), QString()}});
        QVERIFY(panelEmptyFont.ok() && panelEmptyFont.update.vbuttonPanel->fontFamily == std::optional<QString>(QString()));
    }
    for (const QJsonObject &badPanel : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("columns"), 0}},
        QJsonObject{{QStringLiteral("columns"), 21}},
        QJsonObject{{QStringLiteral("max_rows"), 1}},
        QJsonObject{{QStringLiteral("max_rows"), 1000}},
        QJsonObject{{QStringLiteral("button_width"), 3}},
        QJsonObject{{QStringLiteral("button_height"), 641}},
        QJsonObject{{QStringLiteral("opacity"), 0.09}},
        QJsonObject{{QStringLiteral("opacity"), 1.01}},
        QJsonObject{{QStringLiteral("opacity"), 0.555}},
        QJsonObject{{QStringLiteral("margin"), -1}},
        QJsonObject{{QStringLiteral("radius"), 101}},
        QJsonObject{{QStringLiteral("reference_point"), -1}},
        QJsonObject{{QStringLiteral("reference_point"), 12}},
        QJsonObject{{QStringLiteral("offset_x"), -50001}},
        QJsonObject{{QStringLiteral("offset_y"), 50001}},
        QJsonObject{{QStringLiteral("font_size"), 0}},
        QJsonObject{{QStringLiteral("font_size"), 73}},
        QJsonObject{{QStringLiteral("font_weight"), 3}},
        QJsonObject{{QStringLiteral("font_family"), QStringLiteral("Consolas\n")}},
        QJsonObject{{QStringLiteral("font_family"), QString(256, QLatin1Char('A'))}},
        QJsonObject{{QStringLiteral("bg_color"), QStringLiteral("#112233")}},
        QJsonObject{{QStringLiteral("button_color"), QStringLiteral("#ff112233")}},
        QJsonObject{{QStringLiteral("columns"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("columns"), 3}, {QStringLiteral("unknown"), 1}},
    }) {
        QVERIFY(!vbuttonPanelValid(badPanel));
    }
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyExtendedMappingChangeCapabilities(const QJsonArray &capabilities)
{
    QCOMPARE(capabilities.size(), 154);
    const auto demoCapabilities = mappingChangeCapabilities(false);
    QCOMPARE(demoCapabilities.size(), 146);
    QCOMPARE(capabilities.at(85).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_sound_effect.preview"));
    QCOMPARE(capabilities.at(86).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_sound_effect.apply"));
    QCOMPARE(capabilities.at(87).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_global_switch_timeout.preview"));
    QCOMPARE(capabilities.at(88).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_global_switch_timeout.apply"));
    QCOMPARE(capabilities.at(89).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_fullscreen_global_mapping.preview"));
    QCOMPARE(capabilities.at(90).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_fullscreen_global_mapping.apply"));
    QCOMPARE(capabilities.at(91).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_key2mouse_settings.preview"));
    QCOMPARE(capabilities.at(92).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_key2mouse_settings.apply"));
    QCOMPARE(capabilities.at(93).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_process_icon_as_tray_icon.preview"));
    QCOMPARE(capabilities.at(94).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_process_icon_as_tray_icon.apply"));
    QCOMPARE(capabilities.at(95).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_point_display_keys.preview"));
    QCOMPARE(capabilities.at(96).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_point_display_keys.apply"));
    QCOMPARE(capabilities.at(97).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_custom_notification.preview"));
    QCOMPARE(capabilities.at(98).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_custom_notification.apply"));
    QCOMPARE(capabilities.at(99).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_window_match_criteria.preview"));
    QCOMPARE(capabilities.at(100).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_window_match_criteria.apply"));
    QCOMPARE(capabilities.at(101).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_send_to_same_windows.preview"));
    QCOMPARE(capabilities.at(102).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_send_to_same_windows.apply"));
    QCOMPARE(capabilities.at(103).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("ignore_rules.upsert.preview"));
    QCOMPARE(capabilities.at(104).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("ignore_rules.upsert.apply"));
    QCOMPARE(capabilities.at(105).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("ignore_rules.delete.preview"));
    QCOMPARE(capabilities.at(106).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("ignore_rules.delete.apply"));
    QCOMPARE(capabilities.at(107).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("ignore_rules.restore_defaults.preview"));
    QCOMPARE(capabilities.at(108).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("ignore_rules.restore_defaults.apply"));
    QCOMPARE(capabilities.at(109).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.restore_process_path.preview"));
    QCOMPARE(capabilities.at(110).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.restore_process_path.apply"));
    QCOMPARE(capabilities.at(111).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_display_scale.preview"));
    QCOMPARE(capabilities.at(112).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_display_scale.apply"));
    QCOMPARE(capabilities.at(113).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_gyro2mouse_options.preview"));
    QCOMPARE(capabilities.at(114).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_gyro2mouse_options.apply"));
    QCOMPARE(capabilities.at(115).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_gamepad_thresholds.preview"));
    QCOMPARE(capabilities.at(116).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_gamepad_thresholds.apply"));
    QCOMPARE(capabilities.at(117).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_gamepad_touchpad.preview"));
    QCOMPARE(capabilities.at(118).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_gamepad_touchpad.apply"));
    QCOMPARE(capabilities.at(119).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_accept_virtual_gamepad_input.preview"));
    QCOMPARE(capabilities.at(120).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_accept_virtual_gamepad_input.apply"));
    QCOMPARE(capabilities.at(121).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_vbutton_panel_settings.preview"));
    QCOMPARE(capabilities.at(122).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_vbutton_panel_settings.apply"));
    QCOMPARE(capabilities.at(123).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_notification_style.preview"));
    QCOMPARE(capabilities.at(124).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_notification_style.apply"));
    QCOMPARE(capabilities.at(125).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_custom_notification_style.preview"));
    QCOMPARE(capabilities.at(126).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_custom_notification_style.apply"));
    QCOMPARE(capabilities.at(127).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_tray_icons.preview"));
    QCOMPARE(capabilities.at(128).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_tray_icons.apply"));
    QCOMPARE(capabilities.at(129).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_floating_window.preview"));
    QCOMPARE(capabilities.at(130).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_floating_window.apply"));
    QCOMPARE(capabilities.at(131).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_custom_image.preview"));
    QCOMPARE(capabilities.at(131).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(capabilities.at(131).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(132).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_custom_image.apply"));
    QCOMPARE(capabilities.at(132).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(!capabilities.at(132).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(133).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.set_crosshair.preview"));
    QCOMPARE(capabilities.at(133).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(capabilities.at(133).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(134).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.set_crosshair.apply"));
    QCOMPARE(capabilities.at(134).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(!capabilities.at(134).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(135).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.set_floating_button.preview"));
    QCOMPARE(capabilities.at(135).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(capabilities.at(135).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(136).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.set_floating_button.apply"));
    QCOMPARE(capabilities.at(136).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(!capabilities.at(136).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(137).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_floating_button_drag_coordinate.preview"));
    QCOMPARE(capabilities.at(137).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(capabilities.at(137).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(138).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_floating_button_drag_coordinate.apply"));
    QCOMPARE(capabilities.at(138).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(!capabilities.at(138).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(139).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_floating_button_sync_group_note.preview"));
    QCOMPARE(capabilities.at(139).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(capabilities.at(139).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(140).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_floating_button_sync_group_note.apply"));
    QCOMPARE(capabilities.at(140).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(!capabilities.at(140).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(141).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.move_floating_button_sync_group.preview"));
    QCOMPARE(capabilities.at(141).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(capabilities.at(141).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(142).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.move_floating_button_sync_group.apply"));
    QCOMPARE(capabilities.at(142).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(!capabilities.at(142).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(143).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_ui_preferences.preview"));
    QCOMPARE(capabilities.at(143).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(capabilities.at(143).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(144).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_ui_preferences.apply"));
    QCOMPARE(capabilities.at(144).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(!capabilities.at(144).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(145).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_behavior.preview"));
    QCOMPARE(capabilities.at(145).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(capabilities.at(145).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(146).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_behavior.apply"));
    QCOMPARE(capabilities.at(146).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(!capabilities.at(146).toObject().value(QStringLiteral("read_only")).toBool());
    QCOMPARE(capabilities.at(151).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_auto_match_foreground.preview"));
    QCOMPARE(capabilities.at(152).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_auto_match_foreground.apply"));
    for (const char *demoId : {
            "general.set_floating_button_drag_coordinate.apply",
            "configuration.set_floating_button_sync_group_note.apply",
            "mapping.move_floating_button_sync_group.apply",
            "general.set_ui_preferences.apply",
            "general.set_behavior.apply",
        }) {
        QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [demoId](const QJsonValue &item) {
            return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String(demoId);
        }));
    }
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("tab.set_custom_image.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_gyro2mouse_options.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_gamepad_thresholds.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_gamepad_touchpad.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_accept_virtual_gamepad_input.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_vbutton_panel_settings.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("general.set_notification_style.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_custom_notification_style.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("general.set_tray_icons.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("tab.set_floating_window.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("mapping.set_crosshair.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("mapping.set_floating_button.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("ignore_rules.upsert.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("ignore_rules.delete.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("ignore_rules.restore_defaults.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.restore_process_path.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("general.set_display_scale.apply");
    }));
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyNotificationStyleValidation(const QString &previewMethod, const QJsonObject &input)
{
    for (const QString &action : {QStringLiteral("general.set_notification_style"),
            QStringLiteral("configuration.set_custom_notification_style")}) {
        const auto validateStyle = [&](const QJsonObject &fields) {
            return validateMappingChangeRequest(previewMethod,
                {{QStringLiteral("action"), action},
                    {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                    {QStringLiteral("notification_style"), fields}});
        };
        const auto validAll = validateStyle({
            {QStringLiteral("font_color"), QStringLiteral("#AB1234")},
            {QStringLiteral("background_color"), QStringLiteral("#88112233")},
            {QStringLiteral("font_family"), QStringLiteral("  Consolas  ")},
            {QStringLiteral("font_size"), 72},
            {QStringLiteral("font_weight"), 0},
            {QStringLiteral("font_italic"), true},
            {QStringLiteral("display_duration"), 0},
            {QStringLiteral("fade_in_duration"), 99999},
            {QStringLiteral("fade_out_duration"), 500},
            {QStringLiteral("border_radius"), 1000},
            {QStringLiteral("padding"), 0},
            {QStringLiteral("opacity"), -0.0},
            {QStringLiteral("x_offset"), -5000},
            {QStringLiteral("y_offset"), 5000},
        });
        QVERIFY(validAll.ok());
        const auto &opt = (action == QLatin1String("general.set_notification_style"))
            ? validAll.update.generalNotificationStyle
            : validAll.update.customNotificationStyle;
        QVERIFY(opt.has_value());
        QCOMPARE(opt->fontColor, std::optional<QString>(QStringLiteral("#ab1234")));
        QCOMPARE(opt->backgroundColor, std::optional<QString>(QStringLiteral("#88112233")));
        QCOMPARE(opt->fontFamily, std::optional<QString>(QStringLiteral("Consolas")));
        QVERIFY(opt->opacity.has_value() && *opt->opacity == 0.0 && !std::signbit(*opt->opacity));

        const auto emptyFont = validateStyle({{QStringLiteral("font_family"), QString()}});
        QVERIFY(emptyFont.ok());
        for (const QJsonObject &badStyle : {
            QJsonObject{},
            QJsonObject{{QStringLiteral("font_color"), QStringLiteral("#88112233")}},
            QJsonObject{{QStringLiteral("background_color"), QStringLiteral("#112233")}},
            QJsonObject{{QStringLiteral("font_size"), 0}},
            QJsonObject{{QStringLiteral("font_size"), 73}},
            QJsonObject{{QStringLiteral("font_weight"), 3}},
            QJsonObject{{QStringLiteral("font_italic"), 1}},
            QJsonObject{{QStringLiteral("display_duration"), -1}},
            QJsonObject{{QStringLiteral("fade_in_duration"), 100000}},
            QJsonObject{{QStringLiteral("border_radius"), 1001}},
            QJsonObject{{QStringLiteral("padding"), -1}},
            QJsonObject{{QStringLiteral("opacity"), -0.01}},
            QJsonObject{{QStringLiteral("opacity"), 1.01}},
            QJsonObject{{QStringLiteral("opacity"), 0.123}},
            QJsonObject{{QStringLiteral("x_offset"), -5001}},
            QJsonObject{{QStringLiteral("y_offset"), 5001}},
            QJsonObject{{QStringLiteral("font_family"), QStringLiteral("Consolas\n")}},
            QJsonObject{{QStringLiteral("font_family"), QString(256, QLatin1Char('A'))}},
            QJsonObject{{QStringLiteral("font_size"), QJsonValue::Null}},
            QJsonObject{{QStringLiteral("font_size"), 16}, {QStringLiteral("extra"), 1}},
        }) {
            QVERIFY(!validateStyle(badStyle).ok());
        }
    }
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyWindowAndTrayArgumentsValidation(const QString &previewMethod, const QJsonObject &input)
{
    const auto validateProcessIconTray = [&](const QJsonValue &value) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_process_icon_as_tray_icon")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("process_icon_as_tray_icon"), value}});
    };
    for (bool value : {true, false}) {
        const auto result = validateProcessIconTray(value);
        QVERIFY(result.ok() && result.update.processIconAsTrayIcon.has_value());
        QCOMPARE(*result.update.processIconAsTrayIcon, value);
    }
    for (const auto &value : {QJsonValue(1), QJsonValue(QStringLiteral("true")), QJsonValue(QJsonValue::Null)})
        QVERIFY(!validateProcessIconTray(value).ok());

    const auto validateAutoMatchForeground = [&](const QJsonValue &value) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_auto_match_foreground")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("auto_match_foreground"), value}});
    };
    for (bool value : {true, false}) {
        const auto result = validateAutoMatchForeground(value);
        QVERIFY(result.ok() && result.update.autoMatchForeground.has_value());
        QCOMPARE(*result.update.autoMatchForeground, value);
    }
    for (const auto &value : {QJsonValue(1), QJsonValue(QStringLiteral("true")), QJsonValue(QJsonValue::Null)})
        QVERIFY(!validateAutoMatchForeground(value).ok());
    QVERIFY(!validateMappingChangeRequest(previewMethod,
        {{QStringLiteral("action"), QStringLiteral("configuration.set_auto_match_foreground")},
            {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))}}).ok());
    QVERIFY(!validateMappingChangeRequest(previewMethod,
        {{QStringLiteral("action"), QStringLiteral("configuration.set_auto_match_foreground")},
            {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
            {QStringLiteral("auto_match_foreground"), true},
            {QStringLiteral("extra"), 1}}).ok());

    const auto validateSendToSameWindows = [&](const QJsonValue &value) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_send_to_same_windows")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("send_to_same_windows"), value}});
    };
    for (bool value : {true, false}) {
        const auto result = validateSendToSameWindows(value);
        QVERIFY(result.ok() && result.update.sendToSameWindows.has_value());
        QCOMPARE(*result.update.sendToSameWindows, value);
    }
    for (const auto &value : {QJsonValue(1), QJsonValue(QStringLiteral("true")), QJsonValue(QJsonValue::Null)})
        QVERIFY(!validateSendToSameWindows(value).ok());
    QVERIFY(!validateMappingChangeRequest(previewMethod,
        {{QStringLiteral("action"), QStringLiteral("configuration.set_send_to_same_windows")},
            {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))}}).ok());
    QVERIFY(!validateMappingChangeRequest(previewMethod,
        {{QStringLiteral("action"), QStringLiteral("configuration.set_send_to_same_windows")},
            {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
            {QStringLiteral("send_to_same_windows"), true},
            {QStringLiteral("extra"), 1}}).ok());

    const auto validatePointDisplayKeys = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_point_display_keys")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("point_display_keys"), fields}});
    };
    const auto pointKeysBoth = validatePointDisplayKeys({{QStringLiteral("window_point_key"), QStringLiteral("None")},
        {QStringLiteral("screen_point_key"), QStringLiteral("F10")}});
    QVERIFY(pointKeysBoth.ok() && pointKeysBoth.update.showWindowPointKey == QStringLiteral("None")
        && pointKeysBoth.update.showScreenPointKey == QStringLiteral("F10"));
    QVERIFY(validatePointDisplayKeys({{QStringLiteral("window_point_key"), QStringLiteral("F8")}}).ok());
    QVERIFY(validatePointDisplayKeys({{QStringLiteral("screen_point_key"), QString(64, QLatin1Char('A'))}}).ok());
    QVERIFY(!validatePointDisplayKeys({}).ok());
    QVERIFY(!validatePointDisplayKeys({{QStringLiteral("window_point_key"), QStringLiteral("")}}).ok());
    QVERIFY(!validatePointDisplayKeys({{QStringLiteral("screen_point_key"), QString(65, QLatin1Char('A'))}}).ok());
    QVERIFY(!validatePointDisplayKeys({{QStringLiteral("window_point_key"), QStringLiteral("A\nB")}}).ok());
    QVERIFY(!validatePointDisplayKeys({{QStringLiteral("window_point_key"), QStringLiteral("A\rB")}}).ok());
    QVERIFY(!validatePointDisplayKeys({{QStringLiteral("window_point_key"), QStringLiteral("A") + QChar(0)}}).ok());
    QVERIFY(!validatePointDisplayKeys({{QStringLiteral("window_point_key"), 123}}).ok());
    QVERIFY(!validatePointDisplayKeys({{QStringLiteral("window_point_key"), QJsonValue::Null}}).ok());
    QVERIFY(!validatePointDisplayKeys({{QStringLiteral("window_point_key"), QStringLiteral("F8")},
        {QStringLiteral("extra"), QStringLiteral("F9")}}).ok());

    const auto validateCustomNotification = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_custom_notification")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("custom_notification"), fields}});
    };
    const auto customNotifyBoth = validateCustomNotification({{QStringLiteral("enabled"), true},
        {QStringLiteral("position"), 6}});
    QVERIFY(customNotifyBoth.ok() && customNotifyBoth.update.customNotificationEnabled == std::optional<bool>(true)
        && customNotifyBoth.update.customNotificationPosition == std::optional<int>(6));
    QVERIFY(validateCustomNotification({{QStringLiteral("enabled"), false}}).ok());
    QVERIFY(validateCustomNotification({{QStringLiteral("position"), 0}}).ok());
    QVERIFY(!validateCustomNotification({}).ok());
    QVERIFY(!validateCustomNotification({{QStringLiteral("enabled"), 1}}).ok());
    QVERIFY(!validateCustomNotification({{QStringLiteral("position"), true}}).ok());
    QVERIFY(!validateCustomNotification({{QStringLiteral("position"), 0.5}}).ok());
    QVERIFY(!validateCustomNotification({{QStringLiteral("position"), -1}}).ok());
    QVERIFY(!validateCustomNotification({{QStringLiteral("position"), 7}}).ok());
    QVERIFY(!validateCustomNotification({{QStringLiteral("enabled"), false}, {QStringLiteral("position"), 3}}).ok());
    QVERIFY(!validateCustomNotification({{QStringLiteral("enabled"), true}, {QStringLiteral("extra"), 1}}).ok());
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyWindowMatchAndStartupValidation(const QString &previewMethod, const QJsonObject &input)
{
    const auto validateWindowMatchCriteria = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_window_match_criteria")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("window_match_criteria"), fields}});
    };
    const auto winMatchAll = validateWindowMatchCriteria({
        {QStringLiteral("process_name"), QStringLiteral("DemoApp.exe")},
        {QStringLiteral("process_match_mode"), 1},
        {QStringLiteral("window_title"), QString()},
        {QStringLiteral("window_title_match_mode"), 5},
        {QStringLiteral("class_name"), QStringLiteral("Qt6QWindow")},
        {QStringLiteral("class_name_match_mode"), 3}});
    QVERIFY(winMatchAll.ok()
        && winMatchAll.update.windowProcessName == std::optional<QString>(QStringLiteral("DemoApp.exe"))
        && winMatchAll.update.windowProcessMatchMode == std::optional<int>(1)
        && winMatchAll.update.windowTitleText == std::optional<QString>(QString())
        && winMatchAll.update.windowTitleMatchMode == std::optional<int>(5)
        && winMatchAll.update.windowClassName == std::optional<QString>(QStringLiteral("Qt6QWindow"))
        && winMatchAll.update.windowClassMatchMode == std::optional<int>(3));
    QVERIFY(validateWindowMatchCriteria({{QStringLiteral("process_name"), QString(1024, QLatin1Char('A'))}}).ok());
    QVERIFY(!validateWindowMatchCriteria({}).ok());
    QVERIFY(!validateWindowMatchCriteria({{QStringLiteral("process_name"), QString(1025, QLatin1Char('A'))}}).ok());
    QVERIFY(!validateWindowMatchCriteria({{QStringLiteral("window_title"), QStringLiteral("A\nB")}}).ok());
    QVERIFY(!validateWindowMatchCriteria({{QStringLiteral("class_name"), QStringLiteral("A\rB")}}).ok());
    QVERIFY(!validateWindowMatchCriteria({{QStringLiteral("process_name"), QStringLiteral("A") + QChar(0)}}).ok());
    QVERIFY(!validateWindowMatchCriteria({{QStringLiteral("process_name"), 123}}).ok());
    QVERIFY(!validateWindowMatchCriteria({{QStringLiteral("process_match_mode"), true}}).ok());
    QVERIFY(!validateWindowMatchCriteria({{QStringLiteral("window_title_match_mode"), 0.5}}).ok());
    QVERIFY(!validateWindowMatchCriteria({{QStringLiteral("class_name_match_mode"), -1}}).ok());
    QVERIFY(!validateWindowMatchCriteria({{QStringLiteral("process_match_mode"), 6}}).ok());
    QVERIFY(!validateWindowMatchCriteria({{QStringLiteral("process_name"), QStringLiteral("a")}, {QStringLiteral("extra"), 1}}).ok());

    const auto normalBounded = boundAutomationText(QString(1024, QLatin1Char('A')));
    QVERIFY(!normalBounded.truncated && normalBounded.value.size() == 1024 && normalBounded.fullCodeUnits == 1024);
    const QString surrogateBoundary = QString(1023, QLatin1Char('a')) + QString::fromUtf8("\xF0\x9F\x98\x80");
    const auto boundedSurrogate = boundAutomationText(surrogateBoundary);
    QVERIFY(boundedSurrogate.truncated && boundedSurrogate.value.size() == 1023
        && boundedSurrogate.fullCodeUnits == 1025 && boundedSurrogate.fullSha256.size() == 64);
    const QString longCjk1 = QString(16000, QChar(0x4E2D));
    const QString longCjk2 = QString(16000, QChar(0x6587));
    const QString longCjk3 = QString(16000, QChar(0x7A97));
    const QJsonObject boundedState = buildWindowMatchCriteriaState(
        true, longCjk1, 1, longCjk2, 2, longCjk3, 3, longCjk1, longCjk2, longCjk3);
    QVERIFY(boundedState.value(QStringLiteral("text_truncated")).toBool());
    QCOMPARE(boundedState.value(QStringLiteral("text_max_code_units")).toInt(), WindowMatchStateTextMaxCodeUnits);
    const QJsonObject truncatedMap = boundedState.value(QStringLiteral("truncated_fields")).toObject();
    QCOMPARE(truncatedMap.size(), 6);
    for (const char *key : {"process_name", "window_title", "class_name",
            "cached.file_name", "cached.window_title", "cached.class_name"}) {
        const QJsonObject meta = truncatedMap.value(QLatin1String(key)).toObject();
        QCOMPARE(meta.value(QStringLiteral("full_code_units")).toInt(), 16000);
        QCOMPARE(meta.value(QStringLiteral("returned_code_units")).toInt(), 1024);
        QCOMPARE(meta.value(QStringLiteral("full_sha256")).toString().size(), 64);
    }
    QVERIFY(QJsonDocument(boundedState).toJson(QJsonDocument::Compact).size() < 24 * 1024);
    const QJsonObject previewA = buildWindowMatchCriteriaFields(longCjk1, 1, QString(), 2, QString(), 0);
    const QString longCjk1Tail = QString(15999, QChar(0x4E2D)) + QChar(0x4E2E);
    const QJsonObject previewB = buildWindowMatchCriteriaFields(longCjk1Tail, 1, QString(), 2, QString(), 0);
    QVERIFY(previewA.value(QStringLiteral("truncated_fields")).toObject().value(QStringLiteral("process_name"))
        != previewB.value(QStringLiteral("truncated_fields")).toObject().value(QStringLiteral("process_name")));

    const auto validateStartupPosition = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("general.set_startup_position")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("startup_position"), fields}});
    };
    const QJsonObject point{{QStringLiteral("x"), -50000}, {QStringLiteral("y"), 50000}};
    for (int mode : {0, 1, 2}) {
        const auto request = validateStartupPosition({{QStringLiteral("mode"), mode}});
        QVERIFY(request.ok() && request.update.startupPositionMode.has_value());
        QCOMPARE(*request.update.startupPositionMode, mode);
        QVERIFY(!request.update.startupPositionPoint.has_value());
    }
    const auto startup = validateStartupPosition({{QStringLiteral("mode"), 2}, {QStringLiteral("point"), point}});
    QVERIFY(startup.ok() && startup.update.startupPositionPoint.has_value());
    QCOMPARE(*startup.update.startupPositionPoint, QPoint(-50000, 50000));
    QVERIFY(validateStartupPosition({{QStringLiteral("point"), point}}).ok());
    QVERIFY(!validateStartupPosition({}).ok());
    QVERIFY(!validateStartupPosition({{QStringLiteral("mode"), true}}).ok());
    QVERIFY(!validateStartupPosition({{QStringLiteral("mode"), 0.5}}).ok());
    QVERIFY(!validateStartupPosition({{QStringLiteral("mode"), 3}}).ok());
    QVERIFY(!validateStartupPosition({{QStringLiteral("mode"), 1}, {QStringLiteral("point"), point}}).ok());
    QVERIFY(!validateStartupPosition({{QStringLiteral("point"), QJsonObject{{QStringLiteral("x"), 0}}}}).ok());
    QVERIFY(!validateStartupPosition({{QStringLiteral("point"), QJsonObject{{QStringLiteral("x"), 0},
        {QStringLiteral("y"), -50001}}}}).ok());
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyCapabilityListContracts()
{
    const auto capabilities = mappingChangeCapabilities();
    verifyExtendedMappingChangeCapabilities(capabilities);
    if (QTest::currentTestFailed()) return;
    const auto demoCapabilities = mappingChangeCapabilities(false);
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("general.set_global_switch_timeout.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("general.set_fullscreen_global_mapping.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_key2mouse_settings.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_process_icon_as_tray_icon.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_point_display_keys.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_custom_notification.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_window_match_criteria.apply");
    }));
    QVERIFY(std::any_of(demoCapabilities.begin(), demoCapabilities.end(), [](const QJsonValue &item) {
        return item.toObject().value(QStringLiteral("id")).toString() == QLatin1String("configuration.set_send_to_same_windows.apply");
    }));
    for (const auto &id : {QStringLiteral("configuration.load.preview"), QStringLiteral("configuration.load.apply"),
            QStringLiteral("configuration.load_empty.preview"), QStringLiteral("configuration.load_empty.apply"),
            QStringLiteral("configuration.set_auto_match_foreground.preview"), QStringLiteral("configuration.set_auto_match_foreground.apply")}) {
        const auto matches = [&id](const QJsonValue &item) {
            return item.toObject().value(QStringLiteral("id")).toString() == id;
        };
        QVERIFY(std::any_of(capabilities.begin(), capabilities.end(), matches));
        QVERIFY(std::none_of(demoCapabilities.begin(), demoCapabilities.end(), matches));
    }
    QCOMPARE(capabilities.at(75).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.import.preview"));
    QCOMPARE(capabilities.at(76).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.import.apply"));
    QCOMPARE(capabilities.at(77).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.save_import.preview"));
    QCOMPARE(capabilities.at(78).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.save_import.apply"));
    QCOMPARE(capabilities.at(73).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_startup_position.preview"));
    QCOMPARE(capabilities.at(74).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_startup_position.apply"));
    QCOMPARE(capabilities.at(71).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_save_splitter_position.preview"));
    QCOMPARE(capabilities.at(72).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_save_splitter_position.apply"));
    QCOMPARE(capabilities.at(69).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_save_window_size.preview"));
    QCOMPARE(capabilities.at(70).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_save_window_size.apply"));
    QCOMPARE(capabilities.at(67).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("table.import.preview"));
    QCOMPARE(capabilities.at(68).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("table.import.apply"));
    QCOMPARE(capabilities.at(65).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_window_display_mode.preview"));
    QCOMPARE(capabilities.at(66).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_window_display_mode.apply"));
    QCOMPARE(capabilities.at(63).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_table_edit_trigger.preview"));
    QCOMPARE(capabilities.at(64).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_table_edit_trigger.apply"));
    QCOMPARE(capabilities.at(61).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_table_insert_mode.preview"));
    QCOMPARE(capabilities.at(62).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("general.set_table_insert_mode.apply"));
    QCOMPARE(capabilities.at(59).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("macro.import.preview"));
    QCOMPARE(capabilities.at(60).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("macro.import.apply"));
    QCOMPARE(capabilities.at(57).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("macro.delete.preview"));
    QCOMPARE(capabilities.at(58).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("macro.delete.apply"));
    QCOMPARE(capabilities.at(55).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("macro.upsert.preview"));
    QCOMPARE(capabilities.at(56).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("macro.upsert.apply"));
    QCOMPARE(capabilities.at(53).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.duplicate.preview"));
    const auto duplicateApply = capabilities.at(54).toObject();
    QCOMPARE(duplicateApply.value(QStringLiteral("id")).toString(), QStringLiteral("mapping.duplicate.apply"));
    QCOMPARE(duplicateApply.value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    const auto duplicateConfirmations = duplicateApply.value(QStringLiteral("arguments_schema")).toObject()
        .value(QStringLiteral("properties")).toObject();
    QCOMPARE(duplicateConfirmations.value(QStringLiteral("confirm_conflicts")).toObject()
        .value(QStringLiteral("type")).toString(), QStringLiteral("boolean"));
    QCOMPARE(duplicateConfirmations.value(QStringLiteral("confirm_scope")).toObject()
        .value(QStringLiteral("const")).toBool(true), false);
    QCOMPARE(capabilities.at(51).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_mapping_display.preview"));
    QCOMPARE(capabilities.at(52).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_mapping_display.apply"));
    QCOMPARE(capabilities.at(49).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_show_disabled_rows.preview"));
    QCOMPARE(capabilities.at(50).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_show_disabled_rows.apply"));
    QCOMPARE(capabilities.at(47).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_category_filter.preview"));
    QCOMPARE(capabilities.at(48).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_category_filter.apply"));
    QCOMPARE(capabilities.at(45).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_notification_colors.preview"));
    QCOMPARE(capabilities.at(46).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_notification_colors.apply"));
    QCOMPARE(capabilities.at(43).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_notification_mode.preview"));
    QCOMPARE(capabilities.at(44).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_notification_mode.apply"));
    QCOMPARE(capabilities.at(41).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_hotkey.preview"));
    QCOMPARE(capabilities.at(42).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_hotkey.apply"));
    QCOMPARE(capabilities.at(39).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.select.preview"));
    QCOMPARE(capabilities.at(40).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.select.apply"));
    QCOMPARE(capabilities.at(37).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_common_enabled.preview"));
    QCOMPARE(capabilities.at(38).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("configuration.set_common_enabled.apply"));
    QCOMPARE(capabilities.at(35).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_common_collapsed.preview"));
    QCOMPARE(capabilities.at(36).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_common_collapsed.apply"));
    QCOMPARE(capabilities.at(33).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_common_inheritance.preview"));
    QCOMPARE(capabilities.at(34).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.set_common_inheritance.apply"));
    QCOMPARE(capabilities.at(31).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.move.preview"));
    QCOMPARE(capabilities.at(32).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.move.apply"));
    QCOMPARE(capabilities.at(29).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.clear.preview"));
    const auto clearTabApply = capabilities.at(30).toObject();
    QCOMPARE(clearTabApply.value(QStringLiteral("id")).toString(), QStringLiteral("tab.clear.apply"));
    QCOMPARE(clearTabApply.value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QVERIFY(clearTabApply.value(QStringLiteral("arguments_schema")).toObject().value(QStringLiteral("required")).toArray()
        .contains(QJsonValue(QStringLiteral("confirm_delete"))));
    QCOMPARE(capabilities.at(27).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.delete.preview"));
    const auto deleteTabApply = capabilities.at(28).toObject();
    QCOMPARE(deleteTabApply.value(QStringLiteral("id")).toString(), QStringLiteral("tab.delete.apply"));
    QVERIFY(deleteTabApply.value(QStringLiteral("arguments_schema")).toObject().value(QStringLiteral("required")).toArray()
        .contains(QJsonValue(QStringLiteral("confirm_delete"))));
    QCOMPARE(capabilities.at(25).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.copy.preview"));
    QCOMPARE(capabilities.at(25).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QCOMPARE(capabilities.at(26).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.copy.apply"));
    QCOMPARE(capabilities.at(23).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.add_blank.preview"));
    QCOMPARE(capabilities.at(23).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QCOMPARE(capabilities.at(24).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.add_blank.apply"));
    QCOMPARE(capabilities.at(21).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.rename.preview"));
    QCOMPARE(capabilities.at(21).toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QCOMPARE(capabilities.at(22).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("tab.rename.apply"));
    QCOMPARE(capabilities.at(17).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.move_to_top.preview"));
    QCOMPARE(capabilities.at(18).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.move_to_top.apply"));
    QCOMPARE(capabilities.at(19).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.move_to_bottom.preview"));
    QCOMPARE(capabilities.at(20).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.move_to_bottom.apply"));
    const auto downPreview = capabilities.at(15).toObject();
    QCOMPARE(downPreview.value(QStringLiteral("id")).toString(), QStringLiteral("mapping.move_down.preview"));
    QCOMPARE(downPreview.value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    QCOMPARE(capabilities.at(16).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("mapping.move_down.apply"));
    const auto movePreview = capabilities.at(13).toObject();
    QCOMPARE(movePreview.value(QStringLiteral("id")).toString(), QStringLiteral("mapping.move_up.preview"));
    QCOMPARE(movePreview.value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    const auto moveApply = capabilities.at(14).toObject();
    QCOMPARE(moveApply.value(QStringLiteral("id")).toString(), QStringLiteral("mapping.move_up.apply"));
    QVERIFY(!moveApply.value(QStringLiteral("read_only")).toBool());
    const auto stop = capabilities.at(11).toObject();
    QCOMPARE(stop.value(QStringLiteral("id")).toString(), QStringLiteral("mapping.stop"));
    QCOMPARE(stop.value(QStringLiteral("required_scopes")).toInt(), int(Read | RunMapping));
    const auto updatePreview = capabilities.at(6).toObject();
    const auto updateApply = capabilities.at(7).toObject();
    QCOMPARE(updatePreview.value(QStringLiteral("id")).toString(), QStringLiteral("mapping.update.preview"));
    QCOMPARE(updateApply.value(QStringLiteral("id")).toString(), QStringLiteral("mapping.update.apply"));
    QCOMPARE(updatePreview.value(QStringLiteral("original_key_conflicts_supported")), QJsonValue(false));
    const auto conflictSchema = [](const QJsonValue &entry) {
        return entry.toObject().value(QStringLiteral("arguments_schema")).toObject()
            .value(QStringLiteral("properties")).toObject().value(QStringLiteral("confirm_conflicts")).toObject();
    };
    QCOMPARE(conflictSchema(updateApply).value(QStringLiteral("type")).toString(), QStringLiteral("boolean"));
    QCOMPARE(conflictSchema(capabilities.at(9)).value(QStringLiteral("const")), QJsonValue(false));
    for (const auto &update : {updatePreview, updateApply}) {
        QCOMPARE(update.value(QStringLiteral("original_key_conflict_policy")).toString(),
            QStringLiteral("disable_current_row_or_confirm_common_changes"));
        QVERIFY(update.value(QStringLiteral("limitations")).toArray().contains(
            QStringLiteral("common_update_requires_confirmation_to_disable_normal_rows")));
    }
    QCOMPARE(capabilities.first().toObject().value(QStringLiteral("preview_ttl_ms")).toInt(), 120000);
    QCOMPARE(capabilities.first().toObject().value(QStringLiteral("required_scopes")).toInt(), int(Read | Configure));
    const auto start = capabilities.at(12).toObject();
    QCOMPARE(start.value(QStringLiteral("id")).toString(), QStringLiteral("mapping.start"));
    QCOMPARE(start.value(QStringLiteral("required_scopes")).toInt(), int(Read | RunMapping));
    const auto operation = capabilities.last().toObject();
    QCOMPARE(operation.value(QStringLiteral("required_scopes")).toInt(), int(Read));
    QVERIFY(!operation.value(QStringLiteral("limitations")).toArray().contains(QStringLiteral("idle_after_cleanup")));
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyTrayIconsValidation(const QString &previewMethod, const QJsonObject &input)
{
    const auto validateTray = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("general.set_tray_icons")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("tray_icons"), fields}});
    };
    const auto valid = validateTray({
        {QStringLiteral("idle"), QStringLiteral(":/Black.ico")},
        {QStringLiteral("monitoring"), QStringLiteral("default")},
        {QStringLiteral("global"), QStringLiteral("custom_trayicons/mcp.ico")},
        {QStringLiteral("matched"), QStringLiteral(":/Yellow.ico")},
    });
    QVERIFY(valid.ok() && valid.update.trayIcons.has_value());
    QCOMPARE(valid.update.trayIcons->idle, std::optional<QString>(QStringLiteral(":/Black.ico")));
    QCOMPARE(valid.update.trayIcons->monitoring, std::optional<QString>(QStringLiteral("default")));
    QCOMPARE(valid.update.trayIcons->global, std::optional<QString>(QStringLiteral("custom_trayicons/mcp.ico")));
    QCOMPARE(valid.update.trayIcons->matched, std::optional<QString>(QStringLiteral(":/Yellow.ico")));
    for (const QJsonObject &bad : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("idle"), QString()}},
        QJsonObject{{QStringLiteral("monitoring"), QStringLiteral("a\nb")}},
        QJsonObject{{QStringLiteral("global"), QString(256, QLatin1Char('a'))}},
        QJsonObject{{QStringLiteral("matched"), 123}},
        QJsonObject{{QStringLiteral("idle"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("idle"), QStringLiteral("default")}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!validateTray(bad).ok());
    }
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyTabFloatingWindowValidation(const QString &previewMethod, const QJsonObject &input)
{
    const auto validateFWin = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("tab.set_floating_window")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("floating_window"), fields}});
    };
    const auto valid = validateFWin({
        {QStringLiteral("background_color"), QStringLiteral("#AA112233")},
        {QStringLiteral("size"), 128},
        {QStringLiteral("radius"), 24},
        {QStringLiteral("opacity"), 0.65},
        {QStringLiteral("reference_point"), 11},
        {QStringLiteral("position_x"), -120},
        {QStringLiteral("position_y"), 240},
        {QStringLiteral("mouse_pass_through"), true},
        {QStringLiteral("mouse_pass_through_switch_key"), QStringLiteral("None")},
    });
    QVERIFY(valid.ok() && valid.update.tabFloatingWindow.has_value());
    QCOMPARE(valid.update.tabFloatingWindow->backgroundColor, std::optional<QString>(QStringLiteral("#aa112233")));
    QCOMPARE(valid.update.tabFloatingWindow->size, std::optional<int>(128));
    QCOMPARE(valid.update.tabFloatingWindow->radius, std::optional<int>(24));
    QVERIFY(valid.update.tabFloatingWindow->opacity.has_value()
        && qRound(*valid.update.tabFloatingWindow->opacity * 100.0) == 65);
    QCOMPARE(valid.update.tabFloatingWindow->referencePoint, std::optional<int>(11));
    QCOMPARE(valid.update.tabFloatingWindow->positionX, std::optional<int>(-120));
    QCOMPARE(valid.update.tabFloatingWindow->positionY, std::optional<int>(240));
    QCOMPARE(valid.update.tabFloatingWindow->mousePassThrough, std::optional<bool>(true));
    QCOMPARE(valid.update.tabFloatingWindow->mousePassThroughSwitchKey, std::optional<QString>(QStringLiteral("None")));

    const auto defBg = validateFWin({{QStringLiteral("background_color"), QStringLiteral("default")}});
    QVERIFY(defBg.ok() && defBg.update.tabFloatingWindow->backgroundColor == QStringLiteral("default"));

    for (const QJsonObject &bad : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("background_color"), QStringLiteral("#112233")}},
        QJsonObject{{QStringLiteral("size"), 15}},
        QJsonObject{{QStringLiteral("size"), 641}},
        QJsonObject{{QStringLiteral("radius"), -1}},
        QJsonObject{{QStringLiteral("radius"), 321}},
        QJsonObject{{QStringLiteral("opacity"), 0.09}},
        QJsonObject{{QStringLiteral("opacity"), 1.01}},
        QJsonObject{{QStringLiteral("opacity"), 0.555}},
        QJsonObject{{QStringLiteral("reference_point"), -1}},
        QJsonObject{{QStringLiteral("reference_point"), 12}},
        QJsonObject{{QStringLiteral("position_x"), -50001}},
        QJsonObject{{QStringLiteral("position_y"), 50001}},
        QJsonObject{{QStringLiteral("mouse_pass_through"), 1}},
        QJsonObject{{QStringLiteral("mouse_pass_through_switch_key"), QString()}},
        QJsonObject{{QStringLiteral("mouse_pass_through_switch_key"), QStringLiteral("F3\n")}},
        QJsonObject{{QStringLiteral("mouse_pass_through_switch_key"), QString(65, QLatin1Char('A'))}},
        QJsonObject{{QStringLiteral("size"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("size"), 64}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!validateFWin(bad).ok());
    }
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyTabCustomImageValidation(const QString &previewMethod, const QJsonObject &input)
{
    const auto validateCustomImage = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("tab.set_custom_image")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("custom_image"), fields}});
    };
    const auto validFull = validateCustomImage({
        {QStringLiteral("image_path"), QStringLiteral("icons/tab_01.ico")},
        {QStringLiteral("show_position"), 2},
        {QStringLiteral("padding"), 12},
        {QStringLiteral("show_as_tray_icon"), true},
        {QStringLiteral("show_as_floating_window"), true},
        {QStringLiteral("tray_icon_pixel"), QStringLiteral("32x32")}});
    QVERIFY(validFull.ok() && validFull.update.tabCustomImage.has_value());
    QCOMPARE(validFull.update.tabCustomImage->imagePath, std::optional<QString>(QStringLiteral("icons/tab_01.ico")));
    QCOMPARE(validFull.update.tabCustomImage->showPosition, std::optional<int>(2));
    QCOMPARE(validFull.update.tabCustomImage->padding, std::optional<int>(12));
    QCOMPARE(validFull.update.tabCustomImage->showAsTrayIcon, std::optional<bool>(true));
    QCOMPARE(validFull.update.tabCustomImage->showAsFloatingWindow, std::optional<bool>(true));
    QCOMPARE(validFull.update.tabCustomImage->trayIconPixel, std::optional<QString>(QStringLiteral("32x32")));

    QVERIFY(validateCustomImage({{QStringLiteral("image_path"), QString()}, {QStringLiteral("tray_icon_pixel"), QStringLiteral("default")}}).ok());
    QVERIFY(validateCustomImage({{QStringLiteral("image_path"), QStringLiteral("a.png")}, {QStringLiteral("show_position"), 0}, {QStringLiteral("padding"), 0}}).ok());
    QVERIFY(validateCustomImage({{QStringLiteral("image_path"), QStringLiteral("sub/b.SVG")}, {QStringLiteral("padding"), 1000}}).ok());

    for (const auto &bad : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("image_path"), 123}},
        QJsonObject{{QStringLiteral("image_path"), QStringLiteral("C:/temp/a.ico")}},
        QJsonObject{{QStringLiteral("image_path"), QStringLiteral("C:\\temp\\a.ico")}},
        QJsonObject{{QStringLiteral("image_path"), QStringLiteral("../a.ico")}},
        QJsonObject{{QStringLiteral("image_path"), QStringLiteral("./a.ico")}},
        QJsonObject{{QStringLiteral("image_path"), QStringLiteral("/a.ico")}},
        QJsonObject{{QStringLiteral("image_path"), QStringLiteral("\\\\server\\share\\a.ico")}},
        QJsonObject{{QStringLiteral("image_path"), QStringLiteral("icons/a.bmp")}},
        QJsonObject{{QStringLiteral("show_position"), -1}},
        QJsonObject{{QStringLiteral("show_position"), 3}},
        QJsonObject{{QStringLiteral("show_position"), 1.5}},
        QJsonObject{{QStringLiteral("padding"), -1}},
        QJsonObject{{QStringLiteral("padding"), 1001}},
        QJsonObject{{QStringLiteral("show_as_tray_icon"), 1}},
        QJsonObject{{QStringLiteral("show_as_floating_window"), QStringLiteral("true")}},
        QJsonObject{{QStringLiteral("tray_icon_pixel"), QStringLiteral("32\u00D732")}},
        QJsonObject{{QStringLiteral("tray_icon_pixel"), QStringLiteral("0x32")}},
        QJsonObject{{QStringLiteral("tray_icon_pixel"), QStringLiteral("032x32")}},
        QJsonObject{{QStringLiteral("tray_icon_pixel"), QStringLiteral("32x")}},
        QJsonObject{{QStringLiteral("padding"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("padding"), 4}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!validateCustomImage(bad).ok());
    }
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyCrosshairValidation(const QString &previewMethod, const QString &revision)
{
    const QString rowRef = revision + QStringLiteral(":row:0:0");
    const auto validateCrosshair = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("mapping.set_crosshair")},
                {QStringLiteral("source_ref"), rowRef},
                {QStringLiteral("crosshair"), fields}});
    };
    const auto valid = validateCrosshair({
        {QStringLiteral("center_color"), QStringLiteral("#AABBCC")},
        {QStringLiteral("center_size"), 500},
        {QStringLiteral("center_opacity"), 255},
        {QStringLiteral("crosshair_color"), QStringLiteral("default")},
        {QStringLiteral("crosshair_width"), 1},
        {QStringLiteral("crosshair_length"), 900},
        {QStringLiteral("crosshair_opacity"), 0},
        {QStringLiteral("show_center"), false},
        {QStringLiteral("show_top"), true},
        {QStringLiteral("show_bottom"), false},
        {QStringLiteral("show_left"), true},
        {QStringLiteral("show_right"), false},
        {QStringLiteral("x_offset"), -5000},
        {QStringLiteral("y_offset"), 5000},
    });
    QVERIFY(valid.ok() && valid.update.crosshair.has_value());
    QCOMPARE(valid.update.crosshair->centerColor, std::optional<QString>(QStringLiteral("#aabbcc")));
    QCOMPARE(valid.update.crosshair->centerSize, std::optional<int>(500));
    QCOMPARE(valid.update.crosshair->centerOpacity, std::optional<int>(255));
    QCOMPARE(valid.update.crosshair->crosshairColor, std::optional<QString>(QStringLiteral("default")));
    QCOMPARE(valid.update.crosshair->crosshairWidth, std::optional<int>(1));
    QCOMPARE(valid.update.crosshair->crosshairLength, std::optional<int>(900));
    QCOMPARE(valid.update.crosshair->crosshairOpacity, std::optional<int>(0));
    QCOMPARE(valid.update.crosshair->showCenter, std::optional<bool>(false));
    QCOMPARE(valid.update.crosshair->showTop, std::optional<bool>(true));
    QCOMPARE(valid.update.crosshair->showBottom, std::optional<bool>(false));
    QCOMPARE(valid.update.crosshair->showLeft, std::optional<bool>(true));
    QCOMPARE(valid.update.crosshair->showRight, std::optional<bool>(false));
    QCOMPARE(valid.update.crosshair->xOffset, std::optional<int>(-5000));
    QCOMPARE(valid.update.crosshair->yOffset, std::optional<int>(5000));

    QVERIFY(!validateMappingChangeRequest(previewMethod, {
        {QStringLiteral("action"), QStringLiteral("mapping.set_crosshair")},
        {QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0")},
        {QStringLiteral("crosshair"), QJsonObject{{QStringLiteral("center_size"), 5}}}}).ok());

    for (const QJsonObject &bad : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("center_color"), QStringLiteral("#11223344")}},
        QJsonObject{{QStringLiteral("crosshair_color"), QStringLiteral("70a1ff")}},
        QJsonObject{{QStringLiteral("center_size"), 0}},
        QJsonObject{{QStringLiteral("center_size"), 501}},
        QJsonObject{{QStringLiteral("center_opacity"), -1}},
        QJsonObject{{QStringLiteral("center_opacity"), 256}},
        QJsonObject{{QStringLiteral("crosshair_width"), 0}},
        QJsonObject{{QStringLiteral("crosshair_width"), 501}},
        QJsonObject{{QStringLiteral("crosshair_length"), 0}},
        QJsonObject{{QStringLiteral("crosshair_length"), 901}},
        QJsonObject{{QStringLiteral("crosshair_opacity"), 256}},
        QJsonObject{{QStringLiteral("show_center"), 1}},
        QJsonObject{{QStringLiteral("x_offset"), -5001}},
        QJsonObject{{QStringLiteral("y_offset"), 5001}},
        QJsonObject{{QStringLiteral("x_offset"), 1.5}},
        QJsonObject{{QStringLiteral("center_size"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("center_size"), 2}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!validateCrosshair(bad).ok());
    }
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyFloatingButtonValidation(const QString &previewMethod, const QString &revision)
{
    const QString rowRef = revision + QStringLiteral(":row:0:0");
    const auto validateFloatingButton = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("mapping.set_floating_button")},
                {QStringLiteral("source_ref"), rowRef},
                {QStringLiteral("floating_button"), fields}});
    };
    const auto valid = validateFloatingButton({
        {QStringLiteral("enable"), true},
        {QStringLiteral("label"), QStringLiteral("Attack-01")},
        {QStringLiteral("show_on_mapping_start"), false},
        {QStringLiteral("show_tool_tip"), true},
        {QStringLiteral("sync_pressed_locked_state"), false},
        {QStringLiteral("always_on_top"), false},
        {QStringLiteral("mouse_pass_through"), true},
        {QStringLiteral("drag_to_move"), false},
        {QStringLiteral("sync_group_id"), 999},
        {QStringLiteral("button_color"), QStringLiteral("#80AABBCC")},
        {QStringLiteral("pressed_color"), QStringLiteral("#112233")},
        {QStringLiteral("locked_color"), QStringLiteral("default")},
        {QStringLiteral("text_color"), QStringLiteral("#FFEEDD")},
        {QStringLiteral("border_color"), QStringLiteral("#40010203")},
        {QStringLiteral("border_width"), 50},
        {QStringLiteral("width"), 640},
        {QStringLiteral("height"), 4},
        {QStringLiteral("font_size"), 72},
        {QStringLiteral("font_weight"), 2},
        {QStringLiteral("font_family"), QStringLiteral(" Consolas ")},
        {QStringLiteral("radius"), 1000},
        {QStringLiteral("normal_opacity"), 0.01},
        {QStringLiteral("pressed_opacity"), 0.55},
        {QStringLiteral("locked_opacity"), 1.0},
        {QStringLiteral("enable_gradient_fill"), false},
        {QStringLiteral("enable_hover_animation"), true},
        {QStringLiteral("hover_effect_strength"), 200},
        {QStringLiteral("hover_glow_strength"), 0},
        {QStringLiteral("hover_contrast_mode"), 3},
        {QStringLiteral("hover_custom_color"), QStringLiteral("#CC556677")},
        {QStringLiteral("hover_animation_duration"), 500},
        {QStringLiteral("reference_point"), 11},
        {QStringLiteral("x_offset"), -50000},
        {QStringLiteral("y_offset"), 50000},
    });
    QVERIFY(valid.ok() && valid.update.floatingButton.has_value());
    QCOMPARE(valid.update.floatingButton->buttonColor, std::optional<QString>(QStringLiteral("#80aabbcc")));
    QCOMPARE(valid.update.floatingButton->pressedColor, std::optional<QString>(QStringLiteral("#ff112233")));
    QCOMPARE(valid.update.floatingButton->lockedColor, std::optional<QString>(QStringLiteral("default")));
    QCOMPARE(valid.update.floatingButton->fontFamily, std::optional<QString>(QStringLiteral("Consolas")));
    QCOMPARE(valid.update.floatingButton->syncGroupId, std::optional<int>(999));
    QCOMPARE(valid.update.floatingButton->xOffset, std::optional<int>(-50000));
    QCOMPARE(valid.update.floatingButton->yOffset, std::optional<int>(50000));

    const auto withStyleCode = validateFloatingButton({
        {QStringLiteral("style_code"), QStringLiteral("FBS1;w=120;h=40")},
        {QStringLiteral("enable"), true},
    });
    QVERIFY(withStyleCode.ok() && withStyleCode.update.floatingButton.has_value());
    QCOMPARE(withStyleCode.update.floatingButton->styleCode, std::optional<QString>(QStringLiteral("FBS1;w=120;h=40")));
    QCOMPARE(withStyleCode.update.floatingButton->enable, std::optional<bool>(true));

    QVERIFY(!previewRequestValid(previewMethod, {
        {QStringLiteral("action"), QStringLiteral("mapping.set_floating_button")},
        {QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0")},
        {QStringLiteral("floating_button"), QJsonObject{{QStringLiteral("enable"), true}}}}));

    for (const QJsonObject &bad : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("style_code"), QStringLiteral("   ")}},
        QJsonObject{{QStringLiteral("style_code"), QStringLiteral("FBS1;w=1\n2")}},
        QJsonObject{{QStringLiteral("style_code"), QString(1025, QLatin1Char('A'))}},
        QJsonObject{{QStringLiteral("label"), QStringLiteral("bad#label")}},
        QJsonObject{{QStringLiteral("label"), QStringLiteral("bad|label")}},
        QJsonObject{{QStringLiteral("label"), QStringLiteral("bad\nlabel")}},
        QJsonObject{{QStringLiteral("label"), QString(256, QLatin1Char('A'))}},
        QJsonObject{{QStringLiteral("font_family"), QStringLiteral("Consolas\n")}},
        QJsonObject{{QStringLiteral("font_family"), QString(256, QLatin1Char('A'))}},
        QJsonObject{{QStringLiteral("sync_group_id"), -1}},
        QJsonObject{{QStringLiteral("sync_group_id"), 1000}},
        QJsonObject{{QStringLiteral("button_color"), QStringLiteral("#12345")}},
        QJsonObject{{QStringLiteral("border_width"), -1}},
        QJsonObject{{QStringLiteral("border_width"), 51}},
        QJsonObject{{QStringLiteral("width"), 3}},
        QJsonObject{{QStringLiteral("width"), 641}},
        QJsonObject{{QStringLiteral("height"), 3}},
        QJsonObject{{QStringLiteral("height"), 641}},
        QJsonObject{{QStringLiteral("font_size"), 0}},
        QJsonObject{{QStringLiteral("font_size"), 73}},
        QJsonObject{{QStringLiteral("font_weight"), -1}},
        QJsonObject{{QStringLiteral("font_weight"), 3}},
        QJsonObject{{QStringLiteral("radius"), -1}},
        QJsonObject{{QStringLiteral("radius"), 1001}},
        QJsonObject{{QStringLiteral("normal_opacity"), 0.0}},
        QJsonObject{{QStringLiteral("normal_opacity"), 0.009}},
        QJsonObject{{QStringLiteral("pressed_opacity"), 1.01}},
        QJsonObject{{QStringLiteral("locked_opacity"), 0.123}},
        QJsonObject{{QStringLiteral("hover_effect_strength"), -1}},
        QJsonObject{{QStringLiteral("hover_effect_strength"), 201}},
        QJsonObject{{QStringLiteral("hover_glow_strength"), -1}},
        QJsonObject{{QStringLiteral("hover_glow_strength"), 201}},
        QJsonObject{{QStringLiteral("hover_contrast_mode"), -1}},
        QJsonObject{{QStringLiteral("hover_contrast_mode"), 4}},
        QJsonObject{{QStringLiteral("hover_animation_duration"), 79}},
        QJsonObject{{QStringLiteral("hover_animation_duration"), 501}},
        QJsonObject{{QStringLiteral("reference_point"), -1}},
        QJsonObject{{QStringLiteral("reference_point"), 12}},
        QJsonObject{{QStringLiteral("x_offset"), -50001}},
        QJsonObject{{QStringLiteral("y_offset"), 50001}},
        QJsonObject{{QStringLiteral("x_offset"), 1.5}},
        QJsonObject{{QStringLiteral("enable"), 1}},
        QJsonObject{{QStringLiteral("enable"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("enable"), true}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!previewRequestValid(previewMethod, {
            {QStringLiteral("action"), QStringLiteral("mapping.set_floating_button")},
            {QStringLiteral("source_ref"), rowRef},
            {QStringLiteral("floating_button"), bad}}));
    }
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyFloatingButtonSyncValidation(const QString &previewMethod, const QJsonObject &input)
{
    const QJsonValue tabRef = input.value(QStringLiteral("source_tab_ref"));
    {
        const auto dragReq = validateMappingChangeRequest(previewMethod, {
            {QStringLiteral("action"), QStringLiteral("general.set_floating_button_drag_coordinate")},
            {QStringLiteral("source_tab_ref"), tabRef},
            {QStringLiteral("show_drag_coordinate"), false}});
        QVERIFY(dragReq.ok() && dragReq.update.floatingButtonShowDragCoordinate == std::optional<bool>(false));
    }
    QVERIFY(!previewRequestValid(previewMethod, {
        {QStringLiteral("action"), QStringLiteral("general.set_floating_button_drag_coordinate")},
        {QStringLiteral("source_tab_ref"), tabRef}}));
    QVERIFY(!previewRequestValid(previewMethod, {
        {QStringLiteral("action"), QStringLiteral("general.set_floating_button_drag_coordinate")},
        {QStringLiteral("source_tab_ref"), tabRef},
        {QStringLiteral("show_drag_coordinate"), 1}}));
    QVERIFY(!previewRequestValid(previewMethod, {
        {QStringLiteral("action"), QStringLiteral("general.set_floating_button_drag_coordinate")},
        {QStringLiteral("source_tab_ref"), tabRef},
        {QStringLiteral("show_drag_coordinate"), true},
        {QStringLiteral("extra"), 1}}));

    {
        const auto noteReq = validateMappingChangeRequest(previewMethod, {
            {QStringLiteral("action"), QStringLiteral("configuration.set_floating_button_sync_group_note")},
            {QStringLiteral("source_tab_ref"), tabRef},
            {QStringLiteral("sync_group_note"), QJsonObject{
                {QStringLiteral("group_id"), 12},
                {QStringLiteral("note"), QStringLiteral("同步组😀")}}}});
        QVERIFY(noteReq.ok() && noteReq.update.floatingButtonSyncGroupNote.has_value());
        QCOMPARE(noteReq.update.floatingButtonSyncGroupNote->groupId, 12);
        QCOMPARE(noteReq.update.floatingButtonSyncGroupNote->note, QStringLiteral("同步组😀"));
    }
    {
        const auto clearNoteReq = validateMappingChangeRequest(previewMethod, {
            {QStringLiteral("action"), QStringLiteral("configuration.set_floating_button_sync_group_note")},
            {QStringLiteral("source_tab_ref"), tabRef},
            {QStringLiteral("sync_group_note"), QJsonObject{
                {QStringLiteral("group_id"), 999},
                {QStringLiteral("note"), QString()}}}});
        QVERIFY(clearNoteReq.ok() && clearNoteReq.update.floatingButtonSyncGroupNote.has_value());
        QCOMPARE(clearNoteReq.update.floatingButtonSyncGroupNote->groupId, 999);
        QCOMPARE(clearNoteReq.update.floatingButtonSyncGroupNote->note, QString());
    }
    for (const QJsonObject &badNote : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("group_id"), 0}, {QStringLiteral("note"), QStringLiteral("ok")}},
        QJsonObject{{QStringLiteral("group_id"), 1000}, {QStringLiteral("note"), QStringLiteral("ok")}},
        QJsonObject{{QStringLiteral("group_id"), 1.5}, {QStringLiteral("note"), QStringLiteral("ok")}},
        QJsonObject{{QStringLiteral("group_id"), 1}, {QStringLiteral("note"), QStringLiteral("line1\nline2")}},
        QJsonObject{{QStringLiteral("group_id"), 1}, {QStringLiteral("note"), QString(1025, QLatin1Char('A'))}},
        QJsonObject{{QStringLiteral("group_id"), 1}},
        QJsonObject{{QStringLiteral("group_id"), 1}, {QStringLiteral("note"), QStringLiteral("ok")}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!previewRequestValid(previewMethod, {
            {QStringLiteral("action"), QStringLiteral("configuration.set_floating_button_sync_group_note")},
            {QStringLiteral("source_tab_ref"), tabRef},
            {QStringLiteral("sync_group_note"), badNote}}));
    }

    {
        const auto moveReq = validateMappingChangeRequest(previewMethod, {
            {QStringLiteral("action"), QStringLiteral("mapping.move_floating_button_sync_group")},
            {QStringLiteral("source_tab_ref"), tabRef},
            {QStringLiteral("sync_group_move"), QJsonObject{
                {QStringLiteral("group_id"), 1},
                {QStringLiteral("delta_x"), -100000},
                {QStringLiteral("delta_y"), 100000}}}});
        QVERIFY(moveReq.ok() && moveReq.update.floatingButtonSyncGroupMove.has_value());
        QCOMPARE(moveReq.update.floatingButtonSyncGroupMove->groupId, 1);
        QCOMPARE(moveReq.update.floatingButtonSyncGroupMove->deltaX, -100000);
        QCOMPARE(moveReq.update.floatingButtonSyncGroupMove->deltaY, 100000);
    }
    {
        const auto moveXOnly = validateMappingChangeRequest(previewMethod, {
            {QStringLiteral("action"), QStringLiteral("mapping.move_floating_button_sync_group")},
            {QStringLiteral("source_tab_ref"), tabRef},
            {QStringLiteral("sync_group_move"), QJsonObject{
                {QStringLiteral("group_id"), 999},
                {QStringLiteral("delta_x"), 15}}}});
        QVERIFY(moveXOnly.ok() && moveXOnly.update.floatingButtonSyncGroupMove.has_value());
        QCOMPARE(moveXOnly.update.floatingButtonSyncGroupMove->deltaX, 15);
        QCOMPARE(moveXOnly.update.floatingButtonSyncGroupMove->deltaY, 0);
    }
    for (const QJsonObject &badMove : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("group_id"), 0}, {QStringLiteral("delta_x"), 1}},
        QJsonObject{{QStringLiteral("group_id"), 1000}, {QStringLiteral("delta_y"), 1}},
        QJsonObject{{QStringLiteral("group_id"), 1}},
        QJsonObject{{QStringLiteral("group_id"), 1}, {QStringLiteral("delta_x"), 0}},
        QJsonObject{{QStringLiteral("group_id"), 1}, {QStringLiteral("delta_x"), 0}, {QStringLiteral("delta_y"), 0}},
        QJsonObject{{QStringLiteral("group_id"), 1}, {QStringLiteral("delta_x"), 100001}},
        QJsonObject{{QStringLiteral("group_id"), 1}, {QStringLiteral("delta_y"), -100001}},
        QJsonObject{{QStringLiteral("group_id"), 1}, {QStringLiteral("delta_x"), 1.5}},
        QJsonObject{{QStringLiteral("group_id"), 1}, {QStringLiteral("delta_x"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("group_id"), 1}, {QStringLiteral("delta_x"), 1}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!previewRequestValid(previewMethod, {
            {QStringLiteral("action"), QStringLiteral("mapping.move_floating_button_sync_group")},
            {QStringLiteral("source_tab_ref"), tabRef},
            {QStringLiteral("sync_group_move"), badMove}}));
    }
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyGeneralUiPreferencesValidation(const QString &previewMethod, const QJsonObject &input)
{
    const QString tabRef = input.value(QStringLiteral("source_tab_ref")).toString();
    QVERIFY(previewRequestValid(previewMethod, {
        {QStringLiteral("action"), QStringLiteral("general.set_ui_preferences")},
        {QStringLiteral("source_tab_ref"), tabRef},
        {QStringLiteral("general_ui_preferences"), QJsonObject{
            {QStringLiteral("language"), 1},
            {QStringLiteral("theme"), 2},
            {QStringLiteral("notification_position"), 5},
            {QStringLiteral("show_process_list"), false},
            {QStringLiteral("update_site"), 1},
            {QStringLiteral("startup_minimized"), true},
            {QStringLiteral("window_switch_key"), QStringLiteral("L-Ctrl+F7")},
        }}}));
    for (const QJsonObject &badPrefs : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("language"), -1}},
        QJsonObject{{QStringLiteral("language"), 3}},
        QJsonObject{{QStringLiteral("language"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("theme"), -1}},
        QJsonObject{{QStringLiteral("theme"), 3}},
        QJsonObject{{QStringLiteral("notification_position"), -1}},
        QJsonObject{{QStringLiteral("notification_position"), 7}},
        QJsonObject{{QStringLiteral("show_process_list"), QStringLiteral("false")}},
        QJsonObject{{QStringLiteral("update_site"), -1}},
        QJsonObject{{QStringLiteral("update_site"), 2}},
        QJsonObject{{QStringLiteral("startup_minimized"), 1}},
        QJsonObject{{QStringLiteral("window_switch_key"), QString()}},
        QJsonObject{{QStringLiteral("window_switch_key"), QStringLiteral("   ")}},
        QJsonObject{{QStringLiteral("window_switch_key"), QStringLiteral("L-Ctrl+A\nB")}},
        QJsonObject{{QStringLiteral("window_switch_key"), QString(257, QLatin1Char('A'))}},
        QJsonObject{{QStringLiteral("language"), 1}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!previewRequestValid(previewMethod, {
            {QStringLiteral("action"), QStringLiteral("general.set_ui_preferences")},
            {QStringLiteral("source_tab_ref"), tabRef},
            {QStringLiteral("general_ui_preferences"), badPrefs}}));
    }
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyGeneralBehaviorValidation(const QString &previewMethod, const QJsonObject &input)
{
    const QString tabRef = input.value(QStringLiteral("source_tab_ref")).toString();
    QVERIFY(previewRequestValid(previewMethod, {
        {QStringLiteral("action"), QStringLiteral("general.set_behavior")},
        {QStringLiteral("source_tab_ref"), tabRef},
        {QStringLiteral("general_behavior"), QJsonObject{
            {QStringLiteral("startup_auto_monitoring"), true},
            {QStringLiteral("close_to_system_tray"), false},
            {QStringLiteral("mapping_start_action_mode"), 1},
        }}}));
    QVERIFY(previewRequestValid(previewMethod, {
        {QStringLiteral("action"), QStringLiteral("general.set_behavior")},
        {QStringLiteral("source_tab_ref"), tabRef},
        {QStringLiteral("general_behavior"), QJsonObject{
            {QStringLiteral("close_to_system_tray"), true},
        }}}));
    for (const QJsonObject &badBehavior : {
        QJsonObject{},
        QJsonObject{{QStringLiteral("startup_auto_monitoring"), 1}},
        QJsonObject{{QStringLiteral("startup_auto_monitoring"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("close_to_system_tray"), QStringLiteral("false")}},
        QJsonObject{{QStringLiteral("mapping_start_action_mode"), -1}},
        QJsonObject{{QStringLiteral("mapping_start_action_mode"), 2}},
        QJsonObject{{QStringLiteral("mapping_start_action_mode"), QJsonValue::Null}},
        QJsonObject{{QStringLiteral("close_to_system_tray"), true}, {QStringLiteral("extra"), 1}},
    }) {
        QVERIFY(!previewRequestValid(previewMethod, {
            {QStringLiteral("action"), QStringLiteral("general.set_behavior")},
            {QStringLiteral("source_tab_ref"), tabRef},
            {QStringLiteral("general_behavior"), badBehavior}}));
    }
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyTabManagementArgumentsValidation(const QString &previewMethod, const QString &revision, const QString &sourceRef)
{
    QJsonObject changed;
    const QJsonObject rename{{QStringLiteral("action"), QStringLiteral("tab.rename")},
        {QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0")},
        {QStringLiteral("tab_name"), QStringLiteral("测试 Tab")}};
    const auto renameRequest = validateMappingChangeRequest(previewMethod, rename);
    QVERIFY(renameRequest.ok() && renameRequest.update.tabName == QStringLiteral("测试 Tab"));
    QCOMPARE(renameRequest.update.sourceRevision, revision);
    changed = rename; changed.insert(QStringLiteral("source_tab_ref"), sourceRef);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = rename; changed.insert(QStringLiteral("source_ref"), sourceRef);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = rename; changed.remove(QStringLiteral("tab_name"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = rename; changed.insert(QStringLiteral("tab_name"), QString(32767, QLatin1Char('A')));
    QVERIFY(validateMappingChangeRequest(previewMethod, changed).ok());
    for (const auto &badName : {QStringLiteral("Line\nBreak"), QStringLiteral("A\rB"), QString(QChar(0)),
        QString(32768, QLatin1Char('A'))}) {
        changed = rename; changed.insert(QStringLiteral("tab_name"), badName);
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    const QJsonObject addTab{{QStringLiteral("action"), QStringLiteral("tab.add_blank")},
        {QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0")}};
    const auto addTabRequest = validateMappingChangeRequest(previewMethod, addTab);
    QVERIFY(addTabRequest.ok() && addTabRequest.update.addBlankTab && !addTabRequest.update.tabName);
    QCOMPARE(addTabRequest.update.sourceRevision, revision);
    changed = addTab; changed.insert(QStringLiteral("tab_name"), QStringLiteral("Custom name"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = addTab; changed.insert(QStringLiteral("source_tab_ref"), sourceRef);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = addTab; changed.remove(QStringLiteral("source_tab_ref"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject copyTab = addTab;
    copyTab.insert(QStringLiteral("action"), QStringLiteral("tab.copy"));
    const auto copyTabRequest = validateMappingChangeRequest(previewMethod, copyTab);
    QVERIFY(copyTabRequest.ok() && copyTabRequest.update.copyTab && !copyTabRequest.update.addBlankTab);
    QCOMPARE(copyTabRequest.update.sourceRevision, revision);
    changed = copyTab; changed.insert(QStringLiteral("source_tab_ref"), sourceRef);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = copyTab; changed.insert(QStringLiteral("tab_name"), QStringLiteral("New name"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject deleteTab = addTab;
    deleteTab.insert(QStringLiteral("action"), QStringLiteral("tab.delete"));
    const auto deleteTabRequest = validateMappingChangeRequest(previewMethod, deleteTab);
    QVERIFY(deleteTabRequest.ok() && deleteTabRequest.update.deleteTab && !deleteTabRequest.update.copyTab);
    changed = deleteTab; changed.insert(QStringLiteral("source_refs"), QJsonArray{sourceRef});
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = deleteTab; changed.remove(QStringLiteral("source_tab_ref"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject clearTab = addTab;
    clearTab.insert(QStringLiteral("action"), QStringLiteral("tab.clear"));
    const auto clearTabRequest = validateMappingChangeRequest(previewMethod, clearTab);
    QVERIFY(clearTabRequest.ok() && clearTabRequest.update.clearTab && !clearTabRequest.update.deleteTab);
    changed = clearTab; changed.insert(QStringLiteral("confirm_delete"), true);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = clearTab; changed.insert(QStringLiteral("source_tab_ref"), sourceRef);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject moveTab = addTab;
    moveTab.insert(QStringLiteral("action"), QStringLiteral("tab.move"));
    const QString destinationTabRef = revision + QStringLiteral(":source:2");
    moveTab.insert(QStringLiteral("target_tab_ref"), destinationTabRef);
    const auto moveTabRequest = validateMappingChangeRequest(previewMethod, moveTab);
    QVERIFY(moveTabRequest.ok());
    QCOMPARE(moveTabRequest.update.targetTabRef, destinationTabRef);
    changed = moveTab; changed.insert(QStringLiteral("target_tab_ref"), moveTab.value(QStringLiteral("source_tab_ref")));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = moveTab; changed.insert(QStringLiteral("target_tab_ref"), QString(64, QLatin1Char('b')) + QStringLiteral(":source:2"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = moveTab; changed.remove(QStringLiteral("target_tab_ref"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject commonInheritance = addTab;
    commonInheritance.insert(QStringLiteral("action"), QStringLiteral("tab.set_common_inheritance"));
    commonInheritance.insert(QStringLiteral("include_common"), false);
    const auto inheritanceRequest = validateMappingChangeRequest(previewMethod, commonInheritance);
    QVERIFY(inheritanceRequest.ok() && inheritanceRequest.update.tabIncludeCommon == std::optional<bool>(false));
    changed = commonInheritance; changed.remove(QStringLiteral("include_common"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = commonInheritance; changed.insert(QStringLiteral("include_common"), 1);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject commonCollapsed = addTab;
    commonCollapsed.insert(QStringLiteral("action"), QStringLiteral("tab.set_common_collapsed"));
    commonCollapsed.insert(QStringLiteral("common_collapsed"), true);
    const auto collapseRequest = validateMappingChangeRequest(previewMethod, commonCollapsed);
    QVERIFY(collapseRequest.ok() && collapseRequest.update.tabCommonCollapsed == std::optional<bool>(true));
    changed = commonCollapsed; changed.remove(QStringLiteral("common_collapsed"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = commonCollapsed; changed.insert(QStringLiteral("common_collapsed"), QStringLiteral("true"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject commonFeature = addTab;
    commonFeature.insert(QStringLiteral("action"), QStringLiteral("configuration.set_common_enabled"));
    commonFeature.insert(QStringLiteral("common_feature_enabled"), false);
    const auto featureRequest = validateMappingChangeRequest(previewMethod, commonFeature);
    QVERIFY(featureRequest.ok() && featureRequest.update.commonFeatureEnabled == std::optional<bool>(false));
    changed = commonFeature; changed.remove(QStringLiteral("common_feature_enabled"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = commonFeature; changed.insert(QStringLiteral("common_feature_enabled"), QStringLiteral("false"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = commonFeature; changed.insert(QStringLiteral("include_common"), true);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject selectTab = moveTab;
    selectTab.insert(QStringLiteral("action"), QStringLiteral("tab.select"));
    const auto selectRequest = validateMappingChangeRequest(previewMethod, selectTab);
    QVERIFY(selectRequest.ok() && selectRequest.update.selectTab);
    QCOMPARE(selectRequest.update.targetTabRef, destinationTabRef);
    changed = selectTab; changed.insert(QStringLiteral("target_tab_ref"), selectTab.value(QStringLiteral("source_tab_ref")));
    QVERIFY(validateMappingChangeRequest(previewMethod, changed).ok());
    changed = selectTab; changed.insert(QStringLiteral("target_tab_ref"), QString(64, QLatin1Char('b')) + QStringLiteral(":source:2"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = selectTab; changed.remove(QStringLiteral("target_tab_ref"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject hotkey = addTab;
    hotkey.insert(QStringLiteral("action"), QStringLiteral("tab.set_hotkey"));
    hotkey.insert(QStringLiteral("tab_hotkey"), QString());
    const auto clearHotkey = validateMappingChangeRequest(previewMethod, hotkey);
    QVERIFY(clearHotkey.ok() && clearHotkey.update.tabHotkey.has_value() && clearHotkey.update.tabHotkey->isEmpty());
    hotkey.insert(QStringLiteral("tab_hotkey"), QStringLiteral(" $ F9 "));
    QCOMPARE(validateMappingChangeRequest(previewMethod, hotkey).update.tabHotkey.value(), QStringLiteral(" $ F9 "));
    changed = hotkey; changed.remove(QStringLiteral("tab_hotkey"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = hotkey; changed.insert(QStringLiteral("tab_hotkey"), 9);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = hotkey; changed.insert(QStringLiteral("tab_hotkey"), QStringLiteral("A\nB"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject notification = addTab;
    notification.insert(QStringLiteral("action"), QStringLiteral("tab.set_notification_mode"));
    for (const auto state : {Qt::Unchecked, Qt::PartiallyChecked, Qt::Checked}) {
        notification.insert(QStringLiteral("notification_mode"), int(state));
        const auto modeRequest = validateMappingChangeRequest(previewMethod, notification);
        QVERIFY(modeRequest.ok() && modeRequest.update.tabNotificationMode == state);
    }
    for (const auto &invalid : {QJsonValue(-1), QJsonValue(3), QJsonValue(true), QJsonValue(1.5), QJsonValue(QStringLiteral("1"))}) {
        changed = notification; changed.insert(QStringLiteral("notification_mode"), invalid);
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    changed = notification; changed.remove(QStringLiteral("notification_mode"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    QJsonObject colors = addTab;
    colors.insert(QStringLiteral("action"), QStringLiteral("tab.set_notification_colors"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, colors).ok());
    colors.insert(QStringLiteral("notification_font_color"), QStringLiteral("#123ABC"));
    auto colorRequest = validateMappingChangeRequest(previewMethod, colors);
    QVERIFY(colorRequest.ok() && colorRequest.update.tabNotificationFontColor == QStringLiteral("#123ABC"));
    colors.insert(QStringLiteral("notification_background_color"), QStringLiteral("#80405060"));
    QVERIFY(validateMappingChangeRequest(previewMethod, colors).ok());
    changed = colors; changed.insert(QStringLiteral("notification_font_color"), QStringLiteral("default"));
    QVERIFY(validateMappingChangeRequest(previewMethod, changed).ok());
    changed = colors; changed.insert(QStringLiteral("notification_font_color"), QStringLiteral("#ff123456"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = colors; changed.insert(QStringLiteral("notification_background_color"), QStringLiteral("#405060"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = colors; changed.insert(QStringLiteral("notification_background_color"), QJsonValue(QJsonValue::Null));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyRowUpdateAndCopyArgumentsValidation(const QString &previewMethod, const QString &revision, const QString &sourceRef)
{
    QJsonObject changed;
    const QJsonObject noteUpdate{{QStringLiteral("action"), QStringLiteral("mapping.update")},
        {QStringLiteral("source_ref"), sourceRef}, {QStringLiteral("note"), QStringLiteral("  测试#分组|備考😀  ")}};
    const auto noteRequest = validateMappingChangeRequest(previewMethod, noteUpdate);
    QVERIFY(noteRequest.ok() && noteRequest.update.note == std::optional<QString>(QStringLiteral("  测试#分组|備考😀  ")));
    changed = noteUpdate; changed.insert(QStringLiteral("note"), QString());
    QVERIFY(validateMappingChangeRequest(previewMethod, changed).ok());
    changed = noteUpdate; changed.insert(QStringLiteral("note"), QStringLiteral("first\nsecond"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = noteUpdate; changed.insert(QStringLiteral("note"), true);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = noteUpdate; changed.insert(QStringLiteral("disabled"), false);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = noteUpdate; changed.insert(QStringLiteral("send_method"), 0);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    auto categoryUpdate = noteUpdate;
    categoryUpdate.remove(QStringLiteral("note"));
    categoryUpdate.insert(QStringLiteral("category"), QStringLiteral("  组 #A|分類  "));
    const auto categoryRequest = validateMappingChangeRequest(previewMethod, categoryUpdate);
    QVERIFY(categoryRequest.ok());
    QCOMPARE(categoryRequest.update.category.value(), QStringLiteral("  组 #A|分類  "));
    for (const auto &invalid : {QJsonValue(1), QJsonValue(), QJsonValue(QStringLiteral("two\nlines"))}) {
        changed = categoryUpdate; changed.insert(QStringLiteral("category"), invalid);
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    changed = categoryUpdate; changed.insert(QStringLiteral("note"), QString());
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = categoryUpdate; changed.insert(QStringLiteral("disabled"), false);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = categoryUpdate; changed.insert(QStringLiteral("category"), QString());
    QVERIFY(validateMappingChangeRequest(previewMethod, changed).ok());
    categoryUpdate.remove(QStringLiteral("source_ref"));
    const QString rowThree = sourceRef.section(QLatin1Char(':'), 0, 2) + QStringLiteral(":3");
    categoryUpdate.insert(QStringLiteral("source_refs"), QJsonArray{rowThree, sourceRef});
    const auto batchCategory = validateMappingChangeRequest(previewMethod, categoryUpdate);
    QVERIFY(batchCategory.ok());
    QCOMPARE(batchCategory.update.batchRows, QList<int>({1, 3}));
    QCOMPARE(batchCategory.update.sourceReference, sourceRef);
    changed = categoryUpdate; changed.insert(QStringLiteral("source_ref"), sourceRef);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = categoryUpdate; changed.remove(QStringLiteral("category"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    for (const auto &badRef : {sourceRef, sourceRef.section(QLatin1Char(':'), 0, 1) + QStringLiteral(":1:0")}) {
        changed = categoryUpdate; changed.insert(QStringLiteral("source_refs"), QJsonArray{sourceRef, badRef});
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    const QJsonObject categoryFilter{{QStringLiteral("action"), QStringLiteral("tab.set_category_filter")},
        {QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0")},
        {QStringLiteral("category_filter"), QJsonArray{QString(), QStringLiteral("分类#|")}}};
    const auto filter = validateMappingChangeRequest(previewMethod, categoryFilter);
    QVERIFY(filter.ok());
    QCOMPARE(filter.update.tabCategoryFilter.value(), categoryFilter.value(QStringLiteral("category_filter")));
    for (const auto &value : {QJsonValue(QStringLiteral("all")), QJsonValue(QJsonArray{})}) {
        changed = categoryFilter; changed.insert(QStringLiteral("category_filter"), value);
        QVERIFY(validateMappingChangeRequest(previewMethod, changed).ok());
    }
    for (const auto &value : {QJsonValue(), QJsonValue(QStringLiteral("none")),
        QJsonValue(QJsonArray{QString(), QString()}), QJsonValue(QJsonArray{QStringLiteral("two\nlines")}),
        QJsonValue(QJsonArray{false})}) {
        changed = categoryFilter; changed.insert(QStringLiteral("category_filter"), value);
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    const QJsonObject visibility{{QStringLiteral("action"), QStringLiteral("configuration.set_show_disabled_rows")},
        {QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0")}, {QStringLiteral("show_disabled_rows"), false}};
    const auto hidden = validateMappingChangeRequest(previewMethod, visibility);
    QVERIFY(hidden.ok() && hidden.update.showDisabledRows.has_value());
    QVERIFY(!hidden.update.showDisabledRows.value());
    changed = visibility; changed.insert(QStringLiteral("show_disabled_rows"), true);
    QVERIFY(validateMappingChangeRequest(previewMethod, changed).update.showDisabledRows.value());
    changed = visibility; changed.insert(QStringLiteral("show_disabled_rows"), 0);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    const QJsonObject displayChange{{QStringLiteral("action"), QStringLiteral("configuration.set_mapping_display")},
        {QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0")}, {QStringLiteral("mapping_display"),
            QJsonObject{{QStringLiteral("notes"), true}, {QStringLiteral("category_column"), false}}}};
    const auto display = validateMappingChangeRequest(previewMethod, displayChange);
    QVERIFY(display.ok());
    QCOMPARE(display.update.mappingDisplay, displayChange.value(QStringLiteral("mapping_display")).toObject());
    for (const auto &value : {QJsonObject{}, QJsonObject{{QStringLiteral("notes"), 1}}, QJsonObject{{QStringLiteral("arbitrary"), true}}}) {
        changed = displayChange; changed.insert(QStringLiteral("mapping_display"), value);
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    const QJsonObject rowModes{{QStringLiteral("action"), QStringLiteral("mapping.update")},
        {QStringLiteral("source_ref"), sourceRef}, {QStringLiteral("row_modes"),
            QJsonObject{{QStringLiteral("burst"), true}, {QStringLiteral("lock"), false}}}};
    const auto modeConfig = validateMappingChangeRequest(previewMethod, rowModes);
    QVERIFY(modeConfig.ok());
    QCOMPARE(modeConfig.update.rowModes, rowModes.value(QStringLiteral("row_modes")).toObject());
    changed = rowModes; changed.insert(QStringLiteral("row_modes"), QJsonObject{
        {QStringLiteral("mapping_key_unlock"), true}, {QStringLiteral("disable_original_key_unlock"), false}});
    const auto unlock = validateMappingChangeRequest(previewMethod, changed);
    QVERIFY(unlock.ok());
    QCOMPARE(unlock.update.rowModes, changed.value(QStringLiteral("row_modes")).toObject());
    changed = rowModes; changed.insert(QStringLiteral("row_modes"), QJsonObject{{QStringLiteral("mapping_key_unlock"), 1}});
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = rowModes; changed.insert(QStringLiteral("row_modes"), QJsonObject{
        {QStringLiteral("sequence_hold_down"), false}, {QStringLiteral("repeat_mode"), 2}, {QStringLiteral("repeat_times"), 0}});
    const auto sequence = validateMappingChangeRequest(previewMethod, changed);
    QVERIFY(sequence.ok());
    QCOMPARE(sequence.update.rowModes, changed.value(QStringLiteral("row_modes")).toObject());
    for (const auto &invalid : {QJsonObject{{QStringLiteral("sequence_hold_down"), 1}},
            QJsonObject{{QStringLiteral("repeat_mode"), 3}}, QJsonObject{{QStringLiteral("repeat_times"), 100000000}},
            QJsonObject{{QStringLiteral("repeat_times"), 0.25}}, QJsonObject{{QStringLiteral("repeat_mode"), QJsonValue::Null}}}) {
        changed = rowModes; changed.insert(QStringLiteral("row_modes"), invalid);
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    changed = rowModes; changed.insert(QStringLiteral("row_modes"), QJsonObject{
        {QStringLiteral("pass_through"), true}, {QStringLiteral("check_combo_order"), false}, {QStringLiteral("unbreakable"), true}});
    const auto inputOptions = validateMappingChangeRequest(previewMethod, changed);
    QVERIFY(inputOptions.ok());
    QCOMPARE(inputOptions.update.rowModes, changed.value(QStringLiteral("row_modes")).toObject());
    changed.insert(QStringLiteral("row_modes"), QJsonObject{{QStringLiteral("check_combo_order"), 0}});
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = rowModes; changed.insert(QStringLiteral("row_modes"), QJsonObject{
        {QStringLiteral("fixed_vkey_code"), 255}, {QStringLiteral("paste_text_mode"), 0}, {QStringLiteral("disable_fn_key_switch"), true}});
    const auto advanced = validateMappingChangeRequest(previewMethod, changed);
    QVERIFY(advanced.ok());
    QCOMPARE(advanced.update.rowModes, changed.value(QStringLiteral("row_modes")).toObject());
    for (const auto &invalid : {QJsonObject{{QStringLiteral("fixed_vkey_code"), 256}},
            QJsonObject{{QStringLiteral("fixed_vkey_code"), -1}}, QJsonObject{{QStringLiteral("paste_text_mode"), 2}},
            QJsonObject{{QStringLiteral("paste_text_mode"), 0.5}}, QJsonObject{{QStringLiteral("disable_fn_key_switch"), 1}}}) {
        changed.insert(QStringLiteral("row_modes"), invalid);
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    QJsonObject copyRow{{QStringLiteral("action"), QStringLiteral("mapping.duplicate")}, {QStringLiteral("source_ref"), sourceRef},
        {QStringLiteral("copy_to"), QJsonObject{{QStringLiteral("position"), QStringLiteral("above")},
            {QStringLiteral("anchor_ref"), revision + QStringLiteral(":row:0:3")}}}};
    const auto duplicate = validateMappingChangeRequest(previewMethod, copyRow);
    QVERIFY(duplicate.ok());
    QCOMPARE(duplicate.update.copyPosition, QStringLiteral("above"));
    QCOMPARE(duplicate.update.copyAnchorRef, revision + QStringLiteral(":row:0:3"));
    auto crossCopy = copyRow;
    crossCopy.insert(QStringLiteral("copy_to"), QJsonObject{{QStringLiteral("position"), QStringLiteral("below")},
        {QStringLiteral("target_tab_ref"), revision + QStringLiteral(":source:1")},
        {QStringLiteral("anchor_ref"), revision + QStringLiteral(":row:1:0")}});
    const auto acrossTabs = validateMappingChangeRequest(previewMethod, crossCopy);
    QVERIFY(acrossTabs.ok());
    QCOMPARE(acrossTabs.update.copyTargetTabRef, revision + QStringLiteral(":source:1"));
    QCOMPARE(acrossTabs.update.copyAnchorRef, revision + QStringLiteral(":row:1:0"));
    crossCopy.insert(QStringLiteral("copy_to"), QJsonObject{{QStringLiteral("position"), QStringLiteral("below")},
        {QStringLiteral("target_tab_ref"), revision + QStringLiteral(":source:1")}, {QStringLiteral("anchor_ref"), sourceRef}});
    QVERIFY(!validateMappingChangeRequest(previewMethod, crossCopy).ok());
    auto copyBatch = copyRow;
    copyBatch.remove(QStringLiteral("source_ref"));
    copyBatch.insert(QStringLiteral("source_refs"), QJsonArray{revision + QStringLiteral(":row:0:3"), sourceRef});
    const auto duplicates = validateMappingChangeRequest(previewMethod, copyBatch);
    QVERIFY(duplicates.ok());
    QCOMPARE(duplicates.update.batchRows, QList<int>({1, 3}));
    copyBatch.insert(QStringLiteral("source_refs"), QJsonArray{sourceRef, sourceRef});
    QVERIFY(!validateMappingChangeRequest(previewMethod, copyBatch).ok());
    copyRow.insert(QStringLiteral("copy_to"), QJsonObject{{QStringLiteral("position"), QStringLiteral("below")}});
    QVERIFY(!validateMappingChangeRequest(previewMethod, copyRow).ok());
    copyRow.insert(QStringLiteral("copy_to"), QJsonObject{{QStringLiteral("position"), QStringLiteral("above")},
        {QStringLiteral("anchor_ref"), revision + QStringLiteral(":row:1:3")}});
    QVERIFY(!validateMappingChangeRequest(previewMethod, copyRow).ok());
    const auto triggerModes = [](const QJsonObject &trigger) {
        return QJsonObject{{QStringLiteral("original_trigger"), trigger}};
    };
    changed = rowModes; changed.insert(QStringLiteral("row_modes"), triggerModes(
        {{QStringLiteral("type"), QStringLiteral("long_press")}, {QStringLiteral("duration_ms"), 9999}}));
    QVERIFY(validateMappingChangeRequest(previewMethod, changed).ok());
    for (const auto &invalid : {QJsonObject{{QStringLiteral("type"), QStringLiteral("normal")}, {QStringLiteral("duration_ms"), 1}},
            QJsonObject{{QStringLiteral("type"), QStringLiteral("double_press")}, {QStringLiteral("duration_ms"), 10000}},
            QJsonObject{{QStringLiteral("type"), QStringLiteral("long_press")}, {QStringLiteral("extra"), true}},
            QJsonObject{{QStringLiteral("type"), QStringLiteral("long_press")}, {QStringLiteral("duration_ms"), QJsonValue::Null}}}) {
        changed.insert(QStringLiteral("row_modes"), triggerModes(invalid));
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    QJsonObject triggerBatch{{QStringLiteral("action"), QStringLiteral("mapping.update")},
        {QStringLiteral("source_refs"), QJsonArray{sourceRef.section(QLatin1Char(':'), 0, 2) + QStringLiteral(":3"), sourceRef}},
        {QStringLiteral("row_modes"), triggerModes({{QStringLiteral("type"), QStringLiteral("double_press")}})}};
    const auto batchTrigger = validateMappingChangeRequest(previewMethod, triggerBatch);
    QVERIFY(batchTrigger.ok());
    QCOMPARE(batchTrigger.update.batchRows, QList<int>({1, 3}));
    triggerBatch.insert(QStringLiteral("row_modes"), triggerModes(
        {{QStringLiteral("type"), QStringLiteral("double_press")}, {QStringLiteral("duration_ms"), 500}}));
    QVERIFY(!validateMappingChangeRequest(previewMethod, triggerBatch).ok());
    auto mixedTrigger = triggerModes({{QStringLiteral("type"), QStringLiteral("normal")}});
    mixedTrigger.insert(QStringLiteral("lock"), true);
    changed.insert(QStringLiteral("row_modes"), mixedTrigger);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    for (const int ms : {1, 99999}) {
        changed = rowModes; changed.insert(QStringLiteral("row_modes"), QJsonObject{{QStringLiteral("burst_press_ms"), ms}});
        QVERIFY(validateMappingChangeRequest(previewMethod, changed).ok());
    }
    for (const auto &value : {QJsonValue(0), QJsonValue(100000), QJsonValue(12.5), QJsonValue(QStringLiteral("40"))}) {
        changed = rowModes; changed.insert(QStringLiteral("row_modes"), QJsonObject{{QStringLiteral("burst_release_ms"), value}});
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    for (const auto &value : {QJsonObject{}, QJsonObject{{QStringLiteral("burst"), 0}}, QJsonObject{{QStringLiteral("floating"), true}}}) {
        changed = rowModes; changed.insert(QStringLiteral("row_modes"), value);
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    changed = rowModes; changed.insert(QStringLiteral("send_method"), 0);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    const QJsonObject sendTiming{{QStringLiteral("action"), QStringLiteral("mapping.update")},
        {QStringLiteral("source_ref"), sourceRef}, {QStringLiteral("send_timing"), 0}};
    for (int timing = 0; timing <= 4; ++timing) {
        changed = sendTiming; changed.insert(QStringLiteral("send_timing"), timing);
        const auto result = validateMappingChangeRequest(previewMethod, changed);
        QVERIFY(result.ok());
        QCOMPARE(result.update.sendTiming.value(), timing);
    }
    for (const auto &value : {QJsonValue(-1), QJsonValue(5), QJsonValue(2.5), QJsonValue(true), QJsonValue()}) {
        changed = sendTiming; changed.insert(QStringLiteral("send_timing"), value);
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    changed = sendTiming; changed.insert(QStringLiteral("key_up_mapping_key"), QStringLiteral("B"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyMacroAndTableArgumentsValidation(const QString &previewMethod, const QString &revision, const QString &sourceRef, const QJsonObject &input)
{
    QJsonObject changed;
    const QJsonObject macroFields{{QStringLiteral("catalog_revision"), revision}, {QStringLiteral("scope"), QStringLiteral("universal")},
        {QStringLiteral("name"), QStringLiteral("Shared macro")}, {QStringLiteral("content"), QStringLiteral("A")},
        {QStringLiteral("note"), QStringLiteral("")}};
    const QJsonObject macroUpsert{{QStringLiteral("action"), QStringLiteral("macro.upsert")}, {QStringLiteral("macro"), macroFields}};
    const auto macroRequest = validateMappingChangeRequest(previewMethod, macroUpsert);
    QVERIFY(macroRequest.ok() && macroRequest.action == MappingChangeAction::UpsertMacro);
    QCOMPARE(macroRequest.macro.scope, QStringLiteral("universal"));
    QVERIFY(!macroRequest.macro.category && macroRequest.macro.note && macroRequest.macro.note->isEmpty());
    auto relativeMacro = macroFields;
    relativeMacro.insert(QStringLiteral("insert_at"), QJsonObject{{QStringLiteral("position"), QStringLiteral("below")},
        {QStringLiteral("anchor_name"), QStringLiteral("Existing macro")}});
    changed = macroUpsert;
    changed.insert(QStringLiteral("macro"), relativeMacro);
    const auto insertion = validateMappingChangeRequest(previewMethod, changed);
    QVERIFY(insertion.ok() && insertion.macro.insertBelow);
    QCOMPARE(insertion.macro.anchorName, QStringLiteral("Existing macro"));
    auto notedMacro = macroFields;
    notedMacro.insert(QStringLiteral("segment_comments"), QJsonArray{QStringLiteral(""), QStringLiteral("second")});
    changed = macroUpsert;
    changed.insert(QStringLiteral("macro"), notedMacro);
    const auto notes = validateMappingChangeRequest(previewMethod, changed);
    QVERIFY(notes.ok() && notes.macro.segmentComments.has_value());
    QCOMPARE(*notes.macro.segmentComments, QStringList({QStringLiteral(""), QStringLiteral("second")}));
    notedMacro.insert(QStringLiteral("segment_comments"), QJsonArray{true});
    changed.insert(QStringLiteral("macro"), notedMacro);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    relativeMacro.insert(QStringLiteral("insert_at"), QJsonObject{{QStringLiteral("position"), QStringLiteral("append")}});
    changed.insert(QStringLiteral("macro"), relativeMacro);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = macroUpsert;
    changed.insert(QStringLiteral("source_ref"), sourceRef);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    auto invalidMacro = macroFields;
    invalidMacro.insert(QStringLiteral("scope"), QStringLiteral("other"));
    changed = macroUpsert;
    changed.insert(QStringLiteral("macro"), invalidMacro);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    invalidMacro = macroFields;
    invalidMacro.remove(QStringLiteral("name"));
    invalidMacro.remove(QStringLiteral("content"));
    invalidMacro.remove(QStringLiteral("note"));
    invalidMacro.insert(QStringLiteral("names"), QJsonArray{QStringLiteral("A"), QStringLiteral("B")});
    QJsonObject macroDelete{{QStringLiteral("action"), QStringLiteral("macro.delete")}, {QStringLiteral("macro"), invalidMacro}};
    const auto macroDeleteRequest = validateMappingChangeRequest(previewMethod, macroDelete);
    QVERIFY(macroDeleteRequest.ok() && macroDeleteRequest.action == MappingChangeAction::DeleteMacro);
    QCOMPARE(macroDeleteRequest.macro.namesToDelete, QStringList({QStringLiteral("A"), QStringLiteral("B")}));
    invalidMacro.insert(QStringLiteral("names"), QJsonArray{QStringLiteral("A"), QStringLiteral("A")});
    macroDelete.insert(QStringLiteral("macro"), invalidMacro);
    QVERIFY(!validateMappingChangeRequest(previewMethod, macroDelete).ok());
    const auto document = encodeMacroBackup({QVariantMap{{QStringLiteral("macroName"), QStringLiteral("Test")}}});
    QJsonObject importFields{{QStringLiteral("catalog_revision"), revision}, {QStringLiteral("scope"), QStringLiteral("configuration")},
        {QStringLiteral("on_conflict"), QStringLiteral("keep")},
        {QStringLiteral("document_base64"), QString::fromLatin1(document.toBase64())}};
    const auto importRequest = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod, {{QStringLiteral("action"), QStringLiteral("macro.import")},
            {QStringLiteral("macro"), fields}});
    };
    const auto parsedImport = importRequest(importFields);
    QVERIFY(parsedImport.ok() && parsedImport.action == MappingChangeAction::ImportMacro);
    QCOMPARE(parsedImport.macro.importDocument, document);
    QVERIFY(!parsedImport.macro.overwriteImportDuplicates);
    importFields.insert(QStringLiteral("on_conflict"), QStringLiteral("overwrite"));
    QVERIFY(importRequest(importFields).macro.overwriteImportDuplicates);
    auto invalidImport = importFields;
    invalidImport.insert(QStringLiteral("file_name"), QStringLiteral("file.ini"));
    QVERIFY(!importRequest(invalidImport).ok());
    invalidImport = importFields;
    invalidImport.remove(QStringLiteral("on_conflict"));
    QVERIFY(!importRequest(invalidImport).ok());
    invalidImport = importFields;
    invalidImport.insert(QStringLiteral("document_base64"), QStringLiteral("invalid?"));
    QVERIFY(!importRequest(invalidImport).ok());
    const auto validateTablePreference = [&](const QString &field, const QJsonValue &mode) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("general.set_") + field},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {field, mode}});
    };
    for (const int mode : {0, 1}) {
        const auto parsed = validateTablePreference(QStringLiteral("table_insert_mode"), mode);
        QVERIFY(parsed.ok() && parsed.action == MappingChangeAction::UpdateMapping);
        QVERIFY(parsed.update.tableInsertMode.has_value());
        QCOMPARE(*parsed.update.tableInsertMode, mode);
        QVERIFY(!parsed.update.tableEditTrigger.has_value());
        const auto trigger = validateTablePreference(QStringLiteral("table_edit_trigger"), mode);
        QVERIFY(trigger.ok() && trigger.action == MappingChangeAction::UpdateMapping);
        QVERIFY(trigger.update.tableEditTrigger.has_value());
        QCOMPARE(*trigger.update.tableEditTrigger, mode);
        QVERIFY(!trigger.update.tableInsertMode.has_value());
    }
    for (const auto &mode : {QJsonValue(-1), QJsonValue(2), QJsonValue(0.5), QJsonValue(true), QJsonValue(QStringLiteral("1"))}) {
        QVERIFY(!validateTablePreference(QStringLiteral("table_insert_mode"), mode).ok());
        QVERIFY(!validateTablePreference(QStringLiteral("table_edit_trigger"), mode).ok());
    }
    const auto validateWindowDisplayMode = [&](const QJsonValue &mode) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_window_display_mode")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("window_display_mode"), mode}});
    };
    for (int mode : {0, 1, 2}) {
        const auto result = validateWindowDisplayMode(mode);
        QVERIFY(result.ok() && result.update.windowDisplayMode.has_value());
        QCOMPARE(*result.update.windowDisplayMode, mode);
    }
    for (const auto &mode : {QJsonValue(-1), QJsonValue(3), QJsonValue(1.5), QJsonValue(true), QJsonValue(QStringLiteral("2"))})
        QVERIFY(!validateWindowDisplayMode(mode).ok());
    const QJsonObject tableImport{{QStringLiteral("action"), QStringLiteral("table.import")},
        {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
        {QStringLiteral("document_base64"), QStringLiteral("QUJD")}};
    const auto validTableImport = validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), tableImport);
    QVERIFY(validTableImport.ok());
    QCOMPARE(validTableImport.update.sourceRevision, revision);
    QCOMPARE(validTableImport.update.tableImportDocument, QByteArray("ABC"));
    for (const auto &invalidDocument : {QString(), QStringLiteral("QUJD="), QStringLiteral("--"),
             QString::fromLatin1(QByteArray(MaxTableBackupBytes + 1, 'a').toBase64())}) {
        auto rejected = tableImport;
        rejected.insert(QStringLiteral("document_base64"), invalidDocument);
        QVERIFY(!validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), rejected).ok());
    }
    auto tablePath = tableImport;
    tablePath.insert(QStringLiteral("file_name"), QStringLiteral("backup.ini"));
    QVERIFY(!validateMappingChangeRequest(QStringLiteral("qkm_preview_changes"), tablePath).ok());
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyGeneralAndKey2MouseArgumentsValidation(const QString &previewMethod, const QJsonObject &input)
{
    const auto validateSaveWindowSize = [&](const QJsonValue &value) {
        return validateMappingChangeRequest(previewMethod, {{QStringLiteral("action"), QStringLiteral("general.set_save_window_size")},
            {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
            {QStringLiteral("save_window_size"), value}});
    };
    for (bool value : {true, false}) {
        const auto result = validateSaveWindowSize(value);
        QVERIFY(result.ok() && result.update.saveWindowSize.has_value());
        QCOMPARE(*result.update.saveWindowSize, value);
    }
    for (const auto &value : {QJsonValue(1), QJsonValue(QStringLiteral("true")), QJsonValue(QJsonValue::Null)})
        QVERIFY(!validateSaveWindowSize(value).ok());

    const auto validateSaveSplitterPosition = [&](const QJsonValue &value) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("general.set_save_splitter_position")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("save_splitter_position"), value}});
    };
    for (bool value : {true, false}) {
        const auto result = validateSaveSplitterPosition(value);
        QVERIFY(result.ok() && result.update.saveSplitterPosition.has_value());
        QCOMPARE(*result.update.saveSplitterPosition, value);
    }
    for (const auto &value : {QJsonValue(1), QJsonValue(QStringLiteral("true")), QJsonValue(QJsonValue::Null)})
        QVERIFY(!validateSaveSplitterPosition(value).ok());

    const auto validatePlaySoundEffect = [&](const QJsonValue &value) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("general.set_sound_effect")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("play_sound_effect"), value}});
    };
    for (bool value : {true, false}) {
        const auto result = validatePlaySoundEffect(value);
        QVERIFY(result.ok() && result.update.playSoundEffect.has_value());
        QCOMPARE(*result.update.playSoundEffect, value);
    }
    for (const auto &value : {QJsonValue(1), QJsonValue(QStringLiteral("true")), QJsonValue(QJsonValue::Null)})
        QVERIFY(!validatePlaySoundEffect(value).ok());

    const auto validateGlobalSwitchTimeout = [&](const QJsonValue &value) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("general.set_global_switch_timeout")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("timeout_ms"), value}});
    };
    for (int value : {0, 2000, 999999999}) {
        const auto result = validateGlobalSwitchTimeout(value);
        QVERIFY(result.ok() && result.update.globalSwitchTimeoutMs.has_value());
        QCOMPARE(*result.update.globalSwitchTimeoutMs, value);
    }
    for (const auto &value : {QJsonValue(-1), QJsonValue(1000000000), QJsonValue(1.5),
            QJsonValue(QStringLiteral("2000")), QJsonValue(true), QJsonValue(QJsonValue::Null)})
        QVERIFY(!validateGlobalSwitchTimeout(value).ok());

    const auto validateFullscreenGlobalMapping = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("general.set_fullscreen_global_mapping")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("fullscreen_global_mapping"), fields}});
    };
    const auto fullscreenBoth = validateFullscreenGlobalMapping({{QStringLiteral("disable_in_fullscreen"), true},
        {QStringLiteral("allowed_processes"), QStringLiteral("demo.exe;notepad.exe")}});
    QVERIFY(fullscreenBoth.ok() && fullscreenBoth.update.disableGlobalMappingInFullscreen == true
        && fullscreenBoth.update.fullscreenAllowedProcesses == QStringLiteral("demo.exe;notepad.exe"));
    const auto fullscreenDisableOnly = validateFullscreenGlobalMapping({{QStringLiteral("disable_in_fullscreen"), false}});
    QVERIFY(fullscreenDisableOnly.ok() && fullscreenDisableOnly.update.disableGlobalMappingInFullscreen == false
        && !fullscreenDisableOnly.update.fullscreenAllowedProcesses.has_value());
    const auto fullscreenClearOnly = validateFullscreenGlobalMapping({{QStringLiteral("allowed_processes"), QStringLiteral("")}});
    QVERIFY(fullscreenClearOnly.ok() && !fullscreenClearOnly.update.disableGlobalMappingInFullscreen.has_value()
        && fullscreenClearOnly.update.fullscreenAllowedProcesses == QStringLiteral(""));
    QVERIFY(validateFullscreenGlobalMapping({{QStringLiteral("allowed_processes"), QString(32767, QLatin1Char('a'))}}).ok());
    QVERIFY(!validateFullscreenGlobalMapping({}).ok());
    QVERIFY(!validateFullscreenGlobalMapping({{QStringLiteral("disable_in_fullscreen"), false},
        {QStringLiteral("allowed_processes"), QStringLiteral("x.exe")}}).ok());
    QVERIFY(!validateFullscreenGlobalMapping({{QStringLiteral("disable_in_fullscreen"), 1}}).ok());
    QVERIFY(!validateFullscreenGlobalMapping({{QStringLiteral("allowed_processes"), 123}}).ok());
    QVERIFY(!validateFullscreenGlobalMapping({{QStringLiteral("allowed_processes"), QStringLiteral("a\nb")}}).ok());
    QVERIFY(!validateFullscreenGlobalMapping({{QStringLiteral("allowed_processes"), QStringLiteral("a\rb")}}).ok());
    QVERIFY(!validateFullscreenGlobalMapping({{QStringLiteral("allowed_processes"), QStringLiteral("a") + QChar(0)}}).ok());
    QVERIFY(!validateFullscreenGlobalMapping({{QStringLiteral("allowed_processes"), QString(32768, QLatin1Char('a'))}}).ok());
    QVERIFY(!validateFullscreenGlobalMapping({{QStringLiteral("disable_in_fullscreen"), true},
        {QStringLiteral("extra"), true}}).ok());

    const auto validateKey2Mouse = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("configuration.set_key2mouse_settings")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("key2mouse"), fields}});
    };
    const auto key2MouseBounds = validateKey2Mouse({{QStringLiteral("x_speed"), 1},
        {QStringLiteral("y_speed"), 99}, {QStringLiteral("polling_interval_ms"), 1}});
    QVERIFY(key2MouseBounds.ok() && key2MouseBounds.update.mouseXSpeed == 1
        && key2MouseBounds.update.mouseYSpeed == 99 && key2MouseBounds.update.mousePollingIntervalMs == 1);
    const auto key2MousePollOnly = validateKey2Mouse({{QStringLiteral("polling_interval_ms"), 99}});
    QVERIFY(key2MousePollOnly.ok() && !key2MousePollOnly.update.mouseXSpeed.has_value()
        && !key2MousePollOnly.update.mouseYSpeed.has_value() && key2MousePollOnly.update.mousePollingIntervalMs == 99);
    QVERIFY(validateKey2Mouse({{QStringLiteral("x_speed"), 15}}).ok());
    QVERIFY(!validateKey2Mouse({}).ok());
    QVERIFY(!validateKey2Mouse({{QStringLiteral("x_speed"), 0}}).ok());
    QVERIFY(!validateKey2Mouse({{QStringLiteral("y_speed"), 100}}).ok());
    QVERIFY(!validateKey2Mouse({{QStringLiteral("polling_interval_ms"), 0}}).ok());
    QVERIFY(!validateKey2Mouse({{QStringLiteral("polling_interval_ms"), 100}}).ok());
    QVERIFY(!validateKey2Mouse({{QStringLiteral("x_speed"), 1.5}}).ok());
    QVERIFY(!validateKey2Mouse({{QStringLiteral("y_speed"), true}}).ok());
    QVERIFY(!validateKey2Mouse({{QStringLiteral("polling_interval_ms"), QJsonValue::Null}}).ok());
    QVERIFY(!validateKey2Mouse({{QStringLiteral("x_speed"), 3}, {QStringLiteral("extra"), 1}}).ok());
}

#ifdef Q_CC_MSVC
__declspec(noinline)
#endif
static void verifyDisplayScaleAndIgnoreRulesValidation(const QString &previewMethod, const QString &revision, const QJsonObject &input)
{
    for (const int scale : {0, 1, 2, 5, 6, 10}) {
        const auto validScale = validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("general.set_display_scale")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("display_scale"), scale}});
        QVERIFY(validScale.ok() && validScale.action == MappingChangeAction::UpdateMapping);
        QVERIFY(validScale.update.displayScale.has_value());
        QCOMPARE(*validScale.update.displayScale, scale);
    }
    for (const QJsonValue &bad : QJsonArray{-1, 11, 1.5, true, QStringLiteral("2"), QJsonValue(QJsonValue::Null)}) {
        QVERIFY(!validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("general.set_display_scale")},
                {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
                {QStringLiteral("display_scale"), bad}}).ok());
    }
    QVERIFY(!validateMappingChangeRequest(previewMethod,
        {{QStringLiteral("action"), QStringLiteral("general.set_display_scale")},
            {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))}}).ok());

    const auto restorePathValid = validateMappingChangeRequest(previewMethod,
        {{QStringLiteral("action"), QStringLiteral("configuration.restore_process_path")},
            {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))}});
    QVERIFY(restorePathValid.ok() && restorePathValid.action == MappingChangeAction::UpdateMapping);
    QVERIFY(restorePathValid.update.restoreProcessPath);
    QVERIFY(!validateMappingChangeRequest(previewMethod,
        {{QStringLiteral("action"), QStringLiteral("configuration.restore_process_path")}}).ok());
    QVERIFY(!validateMappingChangeRequest(previewMethod,
        {{QStringLiteral("action"), QStringLiteral("configuration.restore_process_path")},
            {QStringLiteral("source_tab_ref"), input.value(QStringLiteral("source_tab_ref"))},
            {QStringLiteral("extra"), true}}).ok());

    const auto validateIgnoreRestoreDefaults = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("ignore_rules.restore_defaults")},
                {QStringLiteral("ignore_rule"), fields}});
    };
    const auto ignoreDefaultsValid = validateIgnoreRestoreDefaults({{QStringLiteral("catalog_revision"), revision}});
    QVERIFY(ignoreDefaultsValid.ok() && ignoreDefaultsValid.action == MappingChangeAction::RestoreDefaultIgnoreRules);
    QVERIFY(ignoreDefaultsValid.ignoreRule.restoreDefaults);
    QVERIFY(!validateIgnoreRestoreDefaults({}).ok());
    QVERIFY(!validateIgnoreRestoreDefaults({{QStringLiteral("catalog_revision"), QStringLiteral("bad")}}).ok());
    QVERIFY(!validateIgnoreRestoreDefaults({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QStringLiteral("RuleA")}}).ok());
    QVERIFY(!validateIgnoreRestoreDefaults({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_names"), QJsonArray{QStringLiteral("RuleA")}}}).ok());

    const auto validateIgnoreDelete = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("ignore_rules.delete")},
                {QStringLiteral("ignore_rule"), fields}});
    };
    const auto ignoreDeleteValid = validateIgnoreDelete({
        {QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_names"), QJsonArray{QStringLiteral("RuleA"), QStringLiteral("RuleB")}}});
    QVERIFY(ignoreDeleteValid.ok() && ignoreDeleteValid.action == MappingChangeAction::DeleteIgnoreRule);
    QCOMPARE(ignoreDeleteValid.ignoreRule.namesToDelete, QStringList({QStringLiteral("RuleA"), QStringLiteral("RuleB")}));
    QVERIFY(!validateIgnoreDelete({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_names"), QJsonArray{}}}).ok());
    QVERIFY(!validateIgnoreDelete({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_names"), QJsonArray{QStringLiteral("RuleA"), QStringLiteral("RuleA")}}}).ok());
    QJsonArray tooManyRules;
    for (int i = 0; i < 101; ++i) {
        tooManyRules.append(QStringLiteral("Rule%1").arg(i));
    }
    QVERIFY(!validateIgnoreDelete({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_names"), tooManyRules}}).ok());
    QVERIFY(!validateIgnoreDelete({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_names"), QJsonArray{QString()}}}).ok());
    QVERIFY(!validateIgnoreDelete({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_names"), QJsonArray{QStringLiteral("Bad\nName")}}}).ok());
    QVERIFY(!validateIgnoreDelete({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_names"), QJsonArray{QString(1025, QLatin1Char('A'))}}}).ok());
    QVERIFY(!validateIgnoreDelete({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_names"), QJsonArray{QStringLiteral("RuleA")}},
        {QStringLiteral("rule_name"), QStringLiteral("RuleA")}}).ok());

    const auto validateIgnoreUpsert = [&](const QJsonObject &fields) {
        return validateMappingChangeRequest(previewMethod,
            {{QStringLiteral("action"), QStringLiteral("ignore_rules.upsert")},
                {QStringLiteral("ignore_rule"), fields}});
    };
    const auto ignoreValid = validateIgnoreUpsert({
        {QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QStringLiteral("  CustomRule  ")},
        {QStringLiteral("process_name"), QStringLiteral("Demo.exe")},
        {QStringLiteral("process_match_mode"), 1},
        {QStringLiteral("window_title"), QStringLiteral("^Test$")},
        {QStringLiteral("window_title_match_mode"), 5},
        {QStringLiteral("class_name"), QStringLiteral("WndClass")},
        {QStringLiteral("class_name_match_mode"), 3},
        {QStringLiteral("description"), QStringLiteral("Note")},
        {QStringLiteral("disabled"), true}});
    QVERIFY(ignoreValid.ok() && ignoreValid.action == MappingChangeAction::UpsertIgnoreRule);
    QCOMPARE(ignoreValid.ignoreRule.ruleName, QStringLiteral("CustomRule"));
    QCOMPARE(ignoreValid.ignoreRule.processMatchMode.value_or(-1), 1);
    QCOMPARE(ignoreValid.ignoreRule.windowTitleMatchMode.value_or(-1), 5);
    QCOMPARE(ignoreValid.ignoreRule.classNameMatchMode.value_or(-1), 3);
    QVERIFY(ignoreValid.ignoreRule.disabled.value_or(false));
    QVERIFY(validateIgnoreUpsert({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QString(1024, QLatin1Char('A'))}}).ok());
    QVERIFY(!validateIgnoreUpsert({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QStringLiteral("   ")}}).ok());
    QVERIFY(!validateIgnoreUpsert({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QString(1025, QLatin1Char('A'))}}).ok());
    QVERIFY(!validateIgnoreUpsert({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QStringLiteral("Bad\nName")}}).ok());
    QVERIFY(!validateIgnoreUpsert({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QStringLiteral("Rule")},
        {QStringLiteral("process_match_mode"), 6}}).ok());
    QVERIFY(!validateIgnoreUpsert({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QStringLiteral("Rule")},
        {QStringLiteral("window_title_match_mode"), true}}).ok());
    QVERIFY(!validateIgnoreUpsert({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QStringLiteral("Rule")},
        {QStringLiteral("disabled"), 1}}).ok());
    QVERIFY(!validateIgnoreUpsert({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QStringLiteral("Rule")},
        {QStringLiteral("description"), QStringLiteral("Line1\rLine2")}}).ok());
    QVERIFY(!validateIgnoreUpsert({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("rule_name"), QStringLiteral("Rule")},
        {QStringLiteral("extra"), 1}}).ok());
    MappingChangeStore ignoreStore;
    const auto ignorePreview = ignoreStore.createIgnoreRule(
        {QStringLiteral("inst"), QStringLiteral("pair"), 1},
        ignoreValid.ignoreRule,
        {{QStringLiteral("action"), QStringLiteral("ignore_rules.upsert")},
            {QStringLiteral("target"), QJsonObject{{QStringLiteral("rule_name"), QStringLiteral("CustomRule")}}}},
        {});
    QVERIFY(ignorePreview.has_value());
    QCOMPARE(ignoreStore.operation({QStringLiteral("inst"), QStringLiteral("pair"), 1}, ignorePreview->id)
        .value(QStringLiteral("target")).toObject().value(QStringLiteral("rule_name")).toString(),
        QStringLiteral("CustomRule"));
}

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
    const QString sourceRef = revision + QStringLiteral(":row:0:1");
    const QJsonObject move{{QStringLiteral("action"), QStringLiteral("mapping.move_up")},
        {QStringLiteral("source_ref"), sourceRef}};
    const auto previewMethod = QStringLiteral("qkm_preview_changes");
    const auto moveRequest = validateMappingChangeRequest(previewMethod, move);
    QVERIFY(moveRequest.ok() && moveRequest.update.move == MappingRowMove::Up);
    auto down = move;
    down.insert(QStringLiteral("action"), QStringLiteral("mapping.move_down"));
    const auto downRequest = validateMappingChangeRequest(previewMethod, down);
    QVERIFY(downRequest.ok() && downRequest.update.move == MappingRowMove::Down);
    down.insert(QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, down).ok());
    QCOMPARE(moveRequest.update.sourceRow, 1);
    QCOMPARE(moveRequest.update.sourceTabRef, revision + QStringLiteral(":source:0"));
    changed = move; changed.remove(QStringLiteral("source_ref"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = move; changed.insert(QStringLiteral("source_ref"), revision + QStringLiteral(":row:0:01"));
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    changed = move; changed.insert(QStringLiteral("insert_row"), 0);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    for (const auto direction : {MappingRowMove::Top, MappingRowMove::Bottom}) {
        auto edge = move;
        edge.insert(QStringLiteral("action"), QString::fromLatin1(mappingRowMoveAction(direction)));
        const auto request = validateMappingChangeRequest(previewMethod, edge);
        QVERIFY(request.ok() && request.update.move == direction);
        edge.insert(QStringLiteral("insert_row"), 0);
        QVERIFY(!validateMappingChangeRequest(previewMethod, edge).ok());
    }
    const QString adjacent = revision + QStringLiteral(":row:0:2");
    const QJsonObject batch{{QStringLiteral("action"), QStringLiteral("mapping.move_to_top")},
        {QStringLiteral("source_refs"), QJsonArray{sourceRef, adjacent}}};
    const auto batchRequest = validateMappingChangeRequest(previewMethod, batch);
    QVERIFY(batchRequest.ok() && batchRequest.update.move == MappingRowMove::Top);
    QCOMPARE(batchRequest.update.sourceRow, 1);
    QCOMPARE(batchRequest.update.rowCount, 2);
    changed = batch; changed.insert(QStringLiteral("source_ref"), sourceRef);
    QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    for (const auto direction : {MappingRowMove::Up, MappingRowMove::Down, MappingRowMove::Bottom}) {
        changed = batch; changed.insert(QStringLiteral("action"), QString::fromLatin1(mappingRowMoveAction(direction)));
        const auto request = validateMappingChangeRequest(previewMethod, changed);
        QVERIFY(request.ok() && request.update.move == direction);
        QCOMPARE(request.update.rowCount, 2);
    }
    for (const auto &invalid : {QJsonArray{sourceRef}, QJsonArray{sourceRef, sourceRef}, QJsonArray{adjacent, sourceRef},
        QJsonArray{sourceRef, revision + QStringLiteral(":row:0:3")}, QJsonArray{sourceRef, revision + QStringLiteral(":row:1:2")},
        QJsonArray{sourceRef, QString(64, QLatin1Char('b')) + QStringLiteral(":row:0:2")},
        QJsonArray{revision + QStringLiteral(":row:0:2147483647"), revision + QStringLiteral(":row:0:2147483648")}}) {
        changed = batch; changed.insert(QStringLiteral("source_refs"), invalid);
        QVERIFY(!validateMappingChangeRequest(previewMethod, changed).ok());
    }
    verifyTabManagementArgumentsValidation(previewMethod, revision, sourceRef);
    verifyRowUpdateAndCopyArgumentsValidation(previewMethod, revision, sourceRef);
    verifyMacroAndTableArgumentsValidation(previewMethod, revision, sourceRef, input);
    verifyGeneralAndKey2MouseArgumentsValidation(previewMethod, input);
    if (QTest::currentTestFailed()) return;

    verifyGamepadValidation(previewMethod, input);
    verifyVButtonPanelValidation(previewMethod, input);
    if (QTest::currentTestFailed()) return;

    verifyNotificationStyleValidation(previewMethod, input);
    verifyTrayIconsValidation(previewMethod, input);
    verifyTabFloatingWindowValidation(previewMethod, input);
    verifyTabCustomImageValidation(previewMethod, input);
    verifyCrosshairValidation(previewMethod, revision);
    verifyFloatingButtonValidation(previewMethod, revision);
    verifyFloatingButtonSyncValidation(previewMethod, input);
    verifyGeneralUiPreferencesValidation(previewMethod, input);
    verifyGeneralBehaviorValidation(previewMethod, input);
    verifyWindowAndTrayArgumentsValidation(previewMethod, input);
    verifyWindowMatchAndStartupValidation(previewMethod, input);
    if (QTest::currentTestFailed()) return;

    verifyDisplayScaleAndIgnoreRulesValidation(previewMethod, revision, input);
    if (QTest::currentTestFailed()) return;

    verifyCapabilityListContracts();
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

void McpControlTest::mappingStopLifecycle()
{
    MappingChangeStore store;
    const QString id(32, QLatin1Char('a'));
    const QString instance(32, QLatin1Char('b'));
    const MappingChangeOwner owner{instance, QStringLiteral("pair"), 1};
    const QString method = QStringLiteral("qkm_stop_mapping");
    const QJsonObject arguments{{QStringLiteral("instance"), instance}, {QStringLiteral("request_id"), id}};
    auto extra = arguments;
    extra.insert(QStringLiteral("force"), true);
    QVERIFY(!validateMappingChangeRequest(method, extra).ok());
    QVERIFY(!validateMappingChangeRequest(method, {{QStringLiteral("instance"), instance}}).ok());
    const auto request = validateMappingChangeRequest(method, arguments);
    QVERIFY(request.ok() && request.method == MappingChangeMethod::Stop);
    QCOMPARE(request.instance, instance);
    QCOMPARE(request.id, id);
    const QString revision(64, QLatin1Char('c'));
    auto startArguments = arguments;
    startArguments.insert(QStringLiteral("source_revision"), revision);
    startArguments.insert(QStringLiteral("source_tab_ref"), revision + QStringLiteral(":source:0"));
    startArguments.insert(QStringLiteral("save_policy"), QStringLiteral("start_only"));
    const auto start = validateMappingChangeRequest(QStringLiteral("qkm_start_mapping"), startArguments);
    QVERIFY(start.ok() && start.method == MappingChangeMethod::Start);
    QCOMPARE(start.startSourceRevision, revision);
    startArguments.insert(QStringLiteral("save_policy"), QStringLiteral("save"));
    QVERIFY(!validateMappingChangeRequest(QStringLiteral("qkm_start_mapping"), startArguments).ok());

    using Reservation = MappingChangeStore::RunReservation;
    const QString status = QStringLiteral("status");
    const QString timedOut = QStringLiteral("cleanup_timed_out");
    const QString waiting = QStringLiteral("waiting_cleanup");
    const auto preview = store.create(owner, {}, {});
    QVERIFY(preview);
    QVERIFY(store.reserveStop(owner, preview->id, true, false, false) == Reservation::Conflict);
    QVERIFY(store.reserveStop(owner, id, false, true, false) == Reservation::Reserved);
    QVERIFY(store.reserveStop(owner, id, false, true, false) == Reservation::Conflict);
    QVERIFY(store.reserveStart(owner, id, {}) == Reservation::Conflict);
    QVERIFY(store.reserveStop({instance, QStringLiteral("pair"), 2}, id, false, true, false) == Reservation::Conflict);
    QCOMPARE(store.operation({instance, QStringLiteral("other"), 1}, id).value(status).toString(), QStringLiteral("unknown"));
    QCOMPARE(store.operation({instance, owner.pairId, 2}, id).value(status).toString(), QStringLiteral("unknown"));

    store.bindStop(id, 100, false);
    store.observeStop(99, true, false);
    QCOMPARE(store.operation(owner, id).value(status).toString(), waiting);
    store.observeStop(100, false, true);
    const auto timed = store.operation(owner, id);
    QCOMPARE(timed.value(status).toString(), waiting);
    QCOMPARE(timed.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("stop_timeout"));

    MappingChangeStoreTestAccess::expireResult(store, id);
    for (int index = 1; index < MappingChangeStore::MaxResults; ++index) {
        const QString duplicate = QString::number(index);
        QVERIFY(store.reserveStop(owner, duplicate, false, false, false) == Reservation::Reserved);
        store.bindStop(duplicate, 100, true);
    }
    QVERIFY(store.reserveStop(owner, QStringLiteral("extra"), true, false, false) == Reservation::Full);
    QCOMPARE(store.operation(owner, id).value(status).toString(), waiting);
    store.observeStop(100, true, false);
    const QString queryTime = QStringLiteral("query_sampled_at_utc");
    auto done = store.operation(owner, id);
    QVERIFY(done.value(queryTime).isString());
    done.remove(queryTime);
    QCOMPARE(done.value(status).toString(), QStringLiteral("completed"));
    QVERIFY(done.value(timedOut).toBool());
    QVERIFY(!done.contains(QStringLiteral("error")) && !done.contains(QStringLiteral("pending_steps")));
    QVERIFY(done.value(QStringLiteral("stop_initiated")).toBool());
    QVERIFY(!store.operation(owner, QStringLiteral("1")).value(QStringLiteral("stop_initiated")).toBool());
    store.observeStop(100, false, true);
    auto late = store.operation(owner, id);
    late.remove(queryTime);
    QCOMPARE(late, done);
    MappingChangeStoreTestAccess::expireResult(store, id);
    QCOMPARE(store.operation(owner, id).value(status).toString(), QStringLiteral("unknown"));
    QVERIFY(store.reserveStop(owner, QStringLiteral("idle"), true, false, false) == Reservation::Reserved);
    QCOMPARE(store.operation(owner, QStringLiteral("idle")).value(status).toString(), QStringLiteral("already_idle"));
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

void McpControlTest::orderedMapMoveAssignment_data()
{
    QTest::addColumn<bool>("sourceHasValues");
    QTest::addColumn<bool>("targetHasValues");
    QTest::newRow("empty-to-empty") << false << false;
    QTest::newRow("empty-to-populated") << false << true;
    QTest::newRow("populated-to-empty") << true << false;
    QTest::newRow("populated-to-populated") << true << true;
}

void McpControlTest::orderedMapMoveAssignment()
{
    QFETCH(bool, sourceHasValues);
    QFETCH(bool, targetHasValues);
    OrderedMap<int, QString> target;
    if (targetHasValues) {
        target[10] = QStringLiteral("old shared key");
        target[99] = QStringLiteral("old target only");
    }
    {
        OrderedMap<int, QString> source;
        if (sourceHasValues) {
            source[30] = QStringLiteral("first");
            source[10] = QStringLiteral("second");
            source[20] = QStringLiteral("third");
        }
        auto &assigned = (target = std::move(source));
        QCOMPARE(&assigned, &target);

        source.clear();
        source[99] = QStringLiteral("reused source");
        QCOMPARE(source.take(99), QStringLiteral("reused source"));
        QVERIFY(source.isEmpty());
    }

    if (!sourceHasValues) {
        QVERIFY(target.isEmpty());
        QCOMPARE(target.keys(), QList<int>{});
        return;
    }
    QCOMPARE(target.keys(), (QList<int>{30, 10, 20}));
    QVERIFY(!target.contains(99));
    const auto &observed = target;
    const auto found = observed.find(10);
    QVERIFY(found != observed.end());
    QCOMPARE(found.key(), 10);
    QCOMPARE(found.value(), QStringLiteral("second"));

    auto &same = target;
    target = std::move(same);
    QCOMPARE(target.keys(), (QList<int>{30, 10, 20}));
    target.insert(10, QStringLiteral("replaced"));
    QCOMPARE(target.keys(), (QList<int>{30, 20, 10}));
    QCOMPARE(target.remove(30), 1);
    QCOMPARE(target.take(20), QStringLiteral("third"));
    target.insertAt(0, 40, QStringLiteral("inserted"));
    QCOMPARE(target.keys(), (QList<int>{40, 10}));
    QCOMPARE(target.find(40).value(), QStringLiteral("inserted"));

    OrderedMap<int, QString> last;
    {
        OrderedMap<int, QString> intermediate;
        intermediate = std::move(target);
        last = std::move(intermediate);
    }
    QCOMPARE(last.keys(), (QList<int>{40, 10}));
    QCOMPARE(last.take(10), QStringLiteral("replaced"));
    last.insertAt(1, 50, QStringLiteral("tail"));
    QCOMPARE(last.keys(), (QList<int>{40, 50}));
}

void McpControlTest::orderedMapCopyAndMoveConstruction()
{
    OrderedMap<int, QString> source;
    source[4] = QStringLiteral("first");
    source[2] = QStringLiteral("second");
    OrderedMap<int, QString> moved(std::move(source));
    source.clear();
    source[42] = QStringLiteral("reused source");
    QCOMPARE(source.take(42), QStringLiteral("reused source"));

    OrderedMap<int, QString> copy(moved);
    OrderedMap<int, QString> assigned;
    assigned[99] = QStringLiteral("unrelated old value");
    assigned = copy;
    moved.clear();
    QCOMPARE(copy.find(2).key(), 2);
    QCOMPARE(copy.take(2), QStringLiteral("second"));
    QCOMPARE(assigned.keys(), (QList<int>{4, 2}));
    assigned.insert(4, QStringLiteral("updated"));
    QCOMPARE(assigned.keys(), (QList<int>{2, 4}));
    QCOMPARE(copy.value(4), QStringLiteral("first"));
    QVERIFY(!assigned.contains(99));
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

void McpControlTest::startupChoiceWithProductApplicationName()
{
    const QString previousName = QCoreApplication::applicationName();
    const bool previousMode = QStandardPaths::isTestModeEnabled();
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QCoreApplication::applicationFilePath());
    const auto restore = qScopeGuard([&]() {
        QCoreApplication::setApplicationName(previousName);
        QStandardPaths::setTestModeEnabled(previousMode);
    });
    qInfo() << "Application-name-derived path:" << QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const auto prepare = [](ControlDialog &dialog) {
        if (configurationOnlyDemo()) QVERIFY(dialog.server().setDemoConfiguration(QStringLiteral("Demo136ASeed")));
        fixture(dialog.server());
        dialog.restoreStartupChoice();
    };
    {
        ControlDialog dialog;
        prepare(dialog);
        QVERIFY(!dialog.server().isEnabled());
        const auto status = widget<QLabel>(dialog, "mcpStartupStatus")->text();
        QVERIFY(status == ControlDialog::tr("Startup choice: not saved (default: disabled).")
                || status == ControlDialog::tr("Saved startup choice: disabled."));
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        clickCheckBox(dialog, "mcpEnabled");
        QVERIFY(dialog.server().isEnabled());
        QTest::mouseClick(widget<QPushButton>(dialog, "mcpSaveStartup"), Qt::LeftButton);
        QCOMPARE(widget<QLabel>(dialog, "mcpStartupStatus")->text(), ControlDialog::tr("Saved startup choice: enabled."));
    }
    {
        ControlDialog dialog;
        prepare(dialog);
        QVERIFY(dialog.server().isEnabled());
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        clickCheckBox(dialog, "mcpEnabled");
        QTest::mouseClick(widget<QPushButton>(dialog, "mcpSaveStartup"), Qt::LeftButton);
        QCOMPARE(widget<QLabel>(dialog, "mcpStartupStatus")->text(), ControlDialog::tr("Saved startup choice: disabled."));
    }
    ControlDialog final;
    prepare(final);
    QVERIFY(!final.server().isEnabled());
}

void McpControlTest::persistentStorageDirectories()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/persistent-directories-XXXXXX"));
    QVERIFY(temporary.isValid());
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    QVERIFY(own.ok());
    const QString pairs = temporary.path() + QStringLiteral("/stable");
    ControlDialog first(pairs + QStringLiteral("/sessions/1"), "target137", nullptr, {}, pairs);
    ControlDialog second(pairs + QStringLiteral("/sessions/2"), "target137", nullptr, {}, pairs);
    for (auto *dialog : {&first, &second}) {
        if (configurationOnlyDemo()) QVERIFY(dialog->server().setDemoConfiguration(QStringLiteral("Persist137Seed")));
        fixture(dialog->server());
    }
    QVERIFY(!QFileInfo::exists(pairs));
    first.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first));
    QVERIFY(!QFileInfo::exists(pairs));
    clickCheckBox(first, "mcpEnabled");
    QVERIFY(first.server().isEnabled());
    for (const auto &path : {pairs, pairs + QStringLiteral("/sessions")}) {
        HANDLE directory = Storage::privateDirectory(path, own.identity.userSid, false);
        QVERIFY(directory != INVALID_HANDLE_VALUE);
        CloseHandle(directory);
    }
    second.show();
    QVERIFY(QTest::qWaitForWindowExposed(&second));
    clickCheckBox(second, "mcpEnabled");
    QVERIFY(second.server().isEnabled());
    QVERIFY(first.server().isEnabled());
    first.server().disable(); second.server().disable();
    for (bool unsafeSessions : {false, true}) {
        const QString root = temporary.path() + (unsafeSessions ? QStringLiteral("/unsafe-sessions") : QStringLiteral("/unsafe-root"));
        if (unsafeSessions) {
            HANDLE directory = Storage::privateDirectory(root, own.identity.userSid, true);
            QVERIFY(directory != INVALID_HANDLE_VALUE);
            CloseHandle(directory);
        }
        const QString unsafe = unsafeSessions ? root + QStringLiteral("/sessions") : root;
        QVERIFY(QDir().mkpath(unsafe));
        HANDLE unsafeHandle = Storage::privateDirectory(unsafe, own.identity.userSid, false);
        if (unsafeHandle != INVALID_HANDLE_VALUE) CloseHandle(unsafeHandle);
        QVERIFY(unsafeHandle == INVALID_HANDLE_VALUE);
        ControlDialog rejected(root + QStringLiteral("/sessions/1"), "target137", nullptr, {}, root);
        if (configurationOnlyDemo()) QVERIFY(rejected.server().setDemoConfiguration(QStringLiteral("Persist137Seed")));
        rejected.show();
        QVERIFY(QTest::qWaitForWindowExposed(&rejected));
        clickCheckBox(rejected, "mcpEnabled");
        QVERIFY(!rejected.server().isEnabled());
        QVERIFY(!widget<QCheckBox>(rejected, "mcpEnabled")->isChecked());
        unsafeHandle = Storage::privateDirectory(unsafe, own.identity.userSid, false);
        if (unsafeHandle != INVALID_HANDLE_VALUE) CloseHandle(unsafeHandle);
        QVERIFY(unsafeHandle == INVALID_HANDLE_VALUE);
    }
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
    const QByteArray closeInput = QJsonDocument(closing).toJson(QJsonDocument::Compact);
    const auto parsedClose = parseMappingEditorCloseArguments(closing);
    QVERIFY(parsedClose.ok());
    QCOMPARE(QJsonDocument(closing).toJson(QJsonDocument::Compact), closeInput);
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
    for (const auto &role : {QStringLiteral("mapping_advanced"), QStringLiteral("ibinput_settings")}) {
        auto opening = valid; opening.insert(QStringLiteral("window_role"), role);
        const QByteArray original = QJsonDocument(opening).toJson(QJsonDocument::Compact);
        QVERIFY(parseMappingSettingsArguments(opening).ok());
        QCOMPARE(QJsonDocument(opening).toJson(QJsonDocument::Compact), original);
        opening.insert(QStringLiteral("window_ref"), QString(64, QLatin1Char('b')));
        auto withoutWindow = opening;
        QCOMPARE(withoutWindow.value(QStringLiteral("window_ref")).toString(), QString(64, QLatin1Char('b')));
        withoutWindow.remove(QStringLiteral("window_ref"));
        QVERIFY2(parseMappingSettingsArguments(withoutWindow).ok(), QJsonDocument(withoutWindow).toJson(QJsonDocument::Compact).constData());
        const QByteArray closeInput = QJsonDocument(opening).toJson(QJsonDocument::Compact);
        QVERIFY2(parseMappingSettingsCloseArguments(opening).ok(), QJsonDocument(opening).toJson(QJsonDocument::Compact).constData());
        QCOMPARE(QJsonDocument(opening).toJson(QJsonDocument::Compact), closeInput);
        QVERIFY(parseMappingSettingsCloseArguments(opening).ok());
    }
    for (const auto &role : {QJsonValue(true), QJsonValue(QJsonValue::Null), QJsonValue(QStringLiteral("main"))}) {
        auto opening = valid; opening.insert(QStringLiteral("window_role"), role);
        QVERIFY(!parseMappingSettingsArguments(opening).ok());
        opening.insert(QStringLiteral("window_ref"), QString(64, QLatin1Char('b')));
        QVERIFY(!parseMappingSettingsCloseArguments(opening).ok());
    }
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

void McpControlTest::windowIbInputObservation()
{
    QWidget main, other;
    QDialog item(&main);
    WindowAccess access(&main, &item);
    const QString instance(32, QLatin1Char('a')), pair(32, QLatin1Char('b'));
    const QString role = QStringLiteral("ibinput_settings"), source = QStringLiteral("owned-source");
    const auto query = [&] { return access.query(instance, pair, 1,
        {{QStringLiteral("entity"), QStringLiteral("windows")}, {QStringLiteral("window_role"), role}}); };
    const auto window = [&] { return query().page.value(QStringLiteral("items")).toArray().first().toObject(); };
    QVERIFY(query().ok());
    QVERIFY(!window().value(QStringLiteral("exists")).toBool());
    auto *popup = new QDialog(&other, Qt::Popup);
    access.trackIbInputSettings(popup);
    QVERIFY(!window().value(QStringLiteral("exists")).toBool());
    delete popup;
    popup = new QDialog(&main, Qt::Popup);
    access.trackIbInputSettings(popup);
    QVERIFY(window().value(QStringLiteral("exists")).toBool());
    QCOMPARE(popup->internalWinId(), WId(0));
    main.show();
    popup->show();
    QVERIFY(QTest::qWaitForWindowExposed(popup));
    const auto reference = window().value(QStringLiteral("window_ref")).toString();
    QCOMPARE(reference.size(), 64);
    QCOMPARE(access.resolve(role, reference, instance, pair, 1), popup);
    QVERIFY(!access.resolve(role, reference, instance, QString(32, QLatin1Char('c')), 1));
    QVERIFY(!access.resolve(role, reference, instance, pair, 2));
    access.rememberEditor(source, QByteArray(32, 'k'), role, QStringLiteral("current-connection"));
    QVERIFY(access.ownsEditor(source, role, QStringLiteral("current-connection")));
    QVERIFY(!access.ownsEditor(source, role));
    QVERIFY(!access.ownsEditor(source, role, QStringLiteral("other-connection")));
    QVERIFY(access.editorDraftMatches(QByteArray(32, 'k')));
    popup->close();
    QVERIFY(!access.resolve(role, reference, instance, pair, 1));
    QVERIFY(!access.ownsEditor(source, role));
    popup->show();
    QVERIFY(QTest::qWaitForWindowExposed(popup));
    QVERIFY(window().value(QStringLiteral("window_ref")).toString() != reference);
    delete popup;
    QVERIFY(!window().value(QStringLiteral("exists")).toBool());
    QCOMPARE(access.query(instance, pair, 1, {{QStringLiteral("entity"), QStringLiteral("windows")}})
        .page.value(QStringLiteral("total")).toInt(), 2);
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
    item[QStringLiteral("role")] = QStringLiteral("ibinput_settings");
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
    int waitSamples = 0;
    QString waitObservation;
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
                } else if (action == QStringLiteral("wait-idle") || action == QStringLiteral("wait-checking")
                    || action == QStringLiteral("wait-active") || action == QStringLiteral("wait-cleaned")
                    || action == QStringLiteral("wait-stall")) {
                    waitObservation = action;
                    dialog.server().setRequestHandler([&](const AutomationRequest &request) {
                        auto &server = dialog.server();
                        if (request.method != QStringLiteral("qkm_get_state") || !request.arguments.isEmpty()) {
                            server.replyError(request.connectionId, request.requestId, QStringLiteral("unsupported_capability"),
                                QStringLiteral("Synthetic wait-state samples only."));
                            return;
                        }
                        ++waitSamples;
                        if (waitObservation == QStringLiteral("wait-stall")) return;
                        const bool active = waitObservation == QStringLiteral("wait-active");
                        const bool monitoring = active || waitObservation == QStringLiteral("wait-checking");
                        server.reply(request.connectionId, request.requestId, {{QStringLiteral("schema_version"), 1},
                            {QStringLiteral("instance"), server.instanceId()}, {QStringLiteral("run"), QJsonObject{
                                {QStringLiteral("monitoring_enabled"), monitoring},
                                {QStringLiteral("gui_state"), QString::fromLatin1(active ? "mapping_matched" : monitoring ? "checking" : "idle")},
                                {QStringLiteral("worker_state_sample"), QString::fromLatin1(monitoring ? "started" : "stopped")},
                                {QStringLiteral("stop_completion"), QString::fromLatin1(waitObservation == QStringLiteral("wait-cleaned")
                                    ? "observed" : "unobserved")}}}});
                    });
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
            {QStringLiteral("wait_samples"), waitSamples}, {QStringLiteral("client_pid"), int(clientPid)},
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

void McpControlTest::safeImageAssetAndTrayScanSecurity()
{
    QTemporaryDir tempRoot;
    QVERIFY(tempRoot.isValid());
    const QString appRoot = tempRoot.filePath(QStringLiteral("app"));
    const QString outsideRoot = tempRoot.filePath(QStringLiteral("outside"));
    const QString subIcons = appRoot + QStringLiteral("/icons");
    const QString trayRealDir = appRoot + QStringLiteral("/custom_trayicons");
    QVERIFY(QDir().mkpath(subIcons));
    QVERIFY(QDir().mkpath(outsideRoot));

    auto writeAll = [](const QString &path, const QByteArray &bytes) {
        QFile f(path);
        return f.open(QIODevice::WriteOnly | QIODevice::Truncate) && f.write(bytes) == bytes.size();
    };
    auto buildTestIco = [](const QList<QSize> &sizes) -> QByteArray {
        const quint16 count = static_cast<quint16>(sizes.size());
        QByteArray header;
        header.append('\0'); header.append('\0');
        header.append(char(1)); header.append('\0');
        header.append(char(count & 0xFF)); header.append(char((count >> 8) & 0xFF));
        QList<QByteArray> dibs;
        quint32 offset = 6 + 16 * quint32(count);
        QByteArray entries;
        for (const QSize &sz : sizes) {
            const quint32 w = quint32(sz.width());
            const quint32 h = quint32(sz.height());
            const quint32 xorBytes = w * h * 4;
            const quint32 andBytes = ((w + 31) / 32) * 4 * h;
            QByteArray dib(40 + int(xorBytes + andBytes), '\0');
            auto put32 = [&](int pos, quint32 v) { std::memcpy(dib.data() + pos, &v, 4); };
            auto put16 = [&](int pos, quint16 v) { std::memcpy(dib.data() + pos, &v, 2); };
            put32(0, 40);
            put32(4, w);
            put32(8, h * 2);
            put16(12, 1);
            put16(14, 32);
            put32(20, xorBytes + andBytes);
            for (quint32 p = 0; p < w * h; ++p) {
                dib[40 + int(p * 4) + 0] = char(0xFF);
                dib[40 + int(p * 4) + 1] = char(0x80);
                dib[40 + int(p * 4) + 2] = char(0x20);
                dib[40 + int(p * 4) + 3] = char(0xFF);
            }
            QByteArray entry(16, '\0');
            entry[0] = char(w < 256 ? w : 0);
            entry[1] = char(h < 256 ? h : 0);
            entry[4] = char(1);
            entry[6] = char(32);
            const quint32 dibSize = quint32(dib.size());
            std::memcpy(entry.data() + 8, &dibSize, 4);
            std::memcpy(entry.data() + 12, &offset, 4);
            offset += dibSize;
            entries.append(entry);
            dibs.append(dib);
        }
        QByteArray full = header + entries;
        for (const QByteArray &dib : dibs) full.append(dib);
        return full;
    };

    const QByteArray multiIcoBytes = buildTestIco({QSize(16, 16), QSize(32, 32)});
    const QByteArray singleIcoBytes = buildTestIco({QSize(16, 16)});
    QImage pngImg(24, 24, QImage::Format_ARGB32);
    pngImg.fill(QColor(40, 140, 240, 255));
    QByteArray pngBytes;
    {
        QBuffer pngBuf(&pngBytes);
        QVERIFY(pngBuf.open(QIODevice::WriteOnly));
        QVERIFY(pngImg.save(&pngBuf, "PNG"));
    }
    const QByteArray svgBytes = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\">"
        "<defs><linearGradient id=\"g1\"><stop offset=\"0%\" stop-color=\"#2080ff\"/></linearGradient></defs>"
        "<rect id=\"r1\" width=\"32\" height=\"32\" fill=\"url(#g1)\"/>"
        "<use href=\"#r1\"/></svg>");
    const QByteArray evilSvgImageBytes = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\">"
        "<image href=\"../outside/ext.ico\" width=\"32\" height=\"32\"/></svg>");
    const QByteArray evilSvgXlinkBytes = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"32\" height=\"32\">"
        "<image xlink:href=\"C:/Windows/win.ini\" width=\"32\" height=\"32\"/></svg>");
    const QByteArray evilSvgFeImageBytes = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\">"
        "<defs><filter id=\"f\"><feImage href=\"single.png\"/></filter></defs>"
        "<rect width=\"32\" height=\"32\" filter=\"url(#f)\"/></svg>");
    const QByteArray evilSvgDtdBytes = QByteArrayLiteral(
        "<!DOCTYPE svg SYSTEM \"file:///C:/outside/ext.dtd\">"
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\"/>");
    const QByteArray evilSvgStyleImportBytes = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\">"
        "<style>@import url(\"ext.css\");</style><rect width=\"32\" height=\"32\"/></svg>");
    const QByteArray evilSvgAttrUrlBytes = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\">"
        "<rect width=\"32\" height=\"32\" style=\"fill: url(ext.svg#g)\"/></svg>");
    const QByteArray evilSvgUseExtBytes = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\">"
        "<use href=\"other.svg#id\"/></svg>");
    const QByteArray evilSvgzBytes("\x1f\x8b\x08\x00\x00\x00\x00\x00", 8);

    QVERIFY(writeAll(subIcons + QStringLiteral("/multi.ico"), multiIcoBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/single.png"), pngBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/badge.svg"), svgBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/evil_image.svg"), evilSvgImageBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/evil_xlink.svg"), evilSvgXlinkBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/evil_feimage.svg"), evilSvgFeImageBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/evil_dtd.svg"), evilSvgDtdBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/evil_import.svg"), evilSvgStyleImportBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/evil_url.svg"), evilSvgAttrUrlBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/evil_use.svg"), evilSvgUseExtBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/evil_gz.svg"), evilSvgzBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/mismatched.ico"), pngBytes));
    QVERIFY(writeAll(subIcons + QStringLiteral("/empty.ico"), QByteArray()));
    QVERIFY(writeAll(outsideRoot + QStringLiteral("/ext.ico"), singleIcoBytes));

    const auto multiRes = loadSafeInAppImageAsset(QStringLiteral("icons\\MULTI.ICO"), appRoot);
    QVERIFY(multiRes.ok && multiRes.relativePath == QStringLiteral("icons/multi.ico"));
    QCOMPARE(multiRes.availableSizes, (QList<QSize>{QSize(32, 32), QSize(16, 16)}));
    const auto pngRes = loadSafeInAppImageAsset(QStringLiteral("icons/single.png"), appRoot);
    QVERIFY(pngRes.ok);
    QCOMPARE(pngRes.availableSizes, (QList<QSize>{QSize(24, 24)}));
    const auto svgRes = loadSafeInAppImageAsset(QStringLiteral("icons/badge.svg"), appRoot);
    QVERIFY(svgRes.ok);
    QVERIFY(!svgRes.icon.isNull());
    QVERIFY(svgRes.availableSizes.isEmpty());
    for (const char *badSvg : {
             "icons/evil_image.svg", "icons/evil_xlink.svg", "icons/evil_feimage.svg",
             "icons/evil_dtd.svg", "icons/evil_import.svg", "icons/evil_url.svg",
             "icons/evil_use.svg", "icons/evil_gz.svg" }) {
        QVERIFY2(!loadSafeInAppImageAsset(QLatin1String(badSvg), appRoot).ok, badSvg);
    }

    QVERIFY(!loadSafeInAppImageAsset(QStringLiteral("icons/mismatched.ico"), appRoot).ok);
    QVERIFY(!loadSafeInAppImageAsset(QStringLiteral("icons/empty.ico"), appRoot).ok);
    QVERIFY(!loadSafeInAppImageAsset(QStringLiteral("../outside/ext.ico"), appRoot).ok);
    QVERIFY(!loadSafeInAppImageAsset(QStringLiteral("icons/multi.ico:stream"), appRoot).ok);

    // NTFS directory junction tests for both subdirs and custom_trayicons root.
    QCOMPARE(QProcess::execute(QStringLiteral("cmd"),
        {QStringLiteral("/c"), QStringLiteral("mklink"), QStringLiteral("/J"),
         QDir::toNativeSeparators(appRoot + QStringLiteral("/linked_icons")),
         QDir::toNativeSeparators(outsideRoot)}), 0);
    QVERIFY(!loadSafeInAppImageAsset(QStringLiteral("linked_icons/ext.ico"), appRoot).ok);

    QCOMPARE(QProcess::execute(QStringLiteral("cmd"),
        {QStringLiteral("/c"), QStringLiteral("mklink"), QStringLiteral("/J"),
         QDir::toNativeSeparators(trayRealDir),
         QDir::toNativeSeparators(outsideRoot)}), 0);
    const auto juncScan = scanSafeCustomTrayIcons(appRoot);
    QVERIFY(juncScan.customIcons.isEmpty());
    QVERIFY(!loadSafeCustomTrayIconAsset(QStringLiteral("custom_trayicons/ext.ico"), appRoot).ok);
    SafeTrayIconCatalogCache trayCache;
    const auto juncCached = trayCache.snapshot(appRoot);
    QVERIFY(juncCached.customIcons.isEmpty());

    // Remove junction and populate real custom_trayicons directory.
    QCOMPARE(QProcess::execute(QStringLiteral("cmd"),
        {QStringLiteral("/c"), QStringLiteral("rmdir"), QDir::toNativeSeparators(trayRealDir)}), 0);
    QVERIFY(QDir().mkpath(trayRealDir));
    QVERIFY(writeAll(trayRealDir + QStringLiteral("/a.ico"), singleIcoBytes));
    QVERIFY(writeAll(trayRealDir + QStringLiteral("/b.ico"), singleIcoBytes));
    QVERIFY(writeAll(trayRealDir + QStringLiteral("/c.ico"), singleIcoBytes));
    QVERIFY(writeAll(trayRealDir + QStringLiteral("/bad.ico"), QByteArray()));
    QVERIFY(writeAll(trayRealDir + QStringLiteral("/note.txt"), QByteArrayLiteral("txt")));

    const auto smallScan = scanSafeCustomTrayIcons(appRoot);
    QCOMPARE(smallScan.customIcons, (QStringList{
        QStringLiteral("custom_trayicons/a.ico"),
        QStringLiteral("custom_trayicons/b.ico"),
        QStringLiteral("custom_trayicons/c.ico")}));
    QVERIFY(!smallScan.truncated && smallScan.countExact);
    QCOMPARE(smallScan.totalCount, std::optional<int>(3));
    QCOMPARE(smallScan.minCount, 3);

    const auto firstSnap = trayCache.snapshot(appRoot);
    QVERIFY(!firstSnap.cacheHit && firstSnap.countExact && firstSnap.totalCount == std::optional<int>(3));
    const auto secondSnap = trayCache.snapshot(appRoot);
    QVERIFY(secondSnap.cacheHit && secondSnap.customIcons == firstSnap.customIcons);

    // Populate 300 long-named .ico files to verify scan budget, truncation lower bound & out-of-window direct lookup.
    const QString pad(100, QLatin1Char('x'));
    QStringList allLarge;
    allLarge.reserve(300);
    for (int i = 0; i < 300; ++i) {
        const QString rel = QStringLiteral("custom_trayicons/icon_%1_%2.ico").arg(i, 3, 10, QLatin1Char('0')).arg(pad);
        QVERIFY(writeAll(appRoot + QLatin1Char('/') + rel, singleIcoBytes));
        allLarge.append(rel);
    }
    const auto largeScan = trayCache.snapshot(appRoot);
    QVERIFY(!largeScan.cacheHit);
    QVERIFY(largeScan.truncated);
    QVERIFY(!largeScan.countExact);
    QVERIFY(!largeScan.totalCount.has_value());
    QCOMPARE(largeScan.minCount, largeScan.customIcons.size());
    QVERIFY(largeScan.customIcons.size() > 0 && largeScan.customIcons.size() <= MaxTrayIconStateItems);
    int totalUtf8 = 0;
    for (const QString &s : largeScan.customIcons) totalUtf8 += s.toUtf8().size();
    QVERIFY(totalUtf8 <= MaxTrayIconStateUtf8Bytes);
    QVERIFY(largeScan.entriesInspected <= MaxTrayIconScanEntries);
    QVERIFY(largeScan.filesDecoded <= MaxTrayIconScanDecodes);
    QVERIFY(trayCache.snapshot(appRoot).cacheHit);

    QString omitted;
    for (const QString &candidate : allLarge) {
        if (!largeScan.customIcons.contains(candidate)) {
            omitted = candidate;
            break;
        }
    }
    QVERIFY(!omitted.isEmpty());
    QVERIFY(loadSafeCustomTrayIconAsset(omitted, appRoot).ok);

    // GUI untruncated scan lists all 303 valid icons while MCP snapshot stays bounded.
    const auto guiFullScan = scanSafeCustomTrayIcons(appRoot, 0, 0, 0, 0);
    QVERIFY(!guiFullScan.truncated && guiFullScan.countExact);
    QCOMPARE(guiFullScan.totalCount, std::optional<int>(303));
    QCOMPARE(guiFullScan.customIcons.size(), 303);
    QVERIFY(guiFullScan.customIcons.contains(omitted));

    // Tab image cache: external path skips disk probe, failed decode caches until file changes.
    SafeTabImageSizeCache tabCache;
    const auto extProbe = tabCache.probeSafeInAppImageSizes(outsideRoot + QStringLiteral("/ext.ico"), appRoot);
    QVERIFY(!extProbe.probed && extProbe.external && extProbe.availableSizes.isEmpty());
    const auto uncProbe = tabCache.probeSafeInAppImageSizes(QStringLiteral("\\\\server\\share\\x.ico"), appRoot);
    QVERIFY(!uncProbe.probed && uncProbe.external);
    const auto missingLocalProbe = tabCache.probeSafeInAppImageSizes(QStringLiteral("missing_sub/a.ico"), appRoot);
    QVERIFY(!missingLocalProbe.probed && !missingLocalProbe.external && missingLocalProbe.availableSizes.isEmpty());
    const auto inProbe1 = tabCache.probeSafeInAppImageSizes(QStringLiteral("icons/multi.ico"), appRoot);
    QVERIFY(inProbe1.probed && !inProbe1.external && !inProbe1.cacheHit);
    QCOMPARE(inProbe1.availableSizes, (QList<QSize>{QSize(32, 32), QSize(16, 16)}));
    QVERIFY(tabCache.probeSafeInAppImageSizes(QStringLiteral("icons/multi.ico"), appRoot).cacheHit);

    const auto svgProbe = tabCache.probeSafeInAppImageSizes(QStringLiteral("icons/badge.svg"), appRoot);
    QVERIFY(svgProbe.probed && !svgProbe.external && svgProbe.availableSizes.isEmpty());
    const auto evilSvgProbe1 = tabCache.probeSafeInAppImageSizes(QStringLiteral("icons/evil_image.svg"), appRoot);
    QVERIFY(!evilSvgProbe1.probed && !evilSvgProbe1.external && !evilSvgProbe1.cacheHit);
    const auto evilSvgProbe2 = tabCache.probeSafeInAppImageSizes(QStringLiteral("icons/evil_image.svg"), appRoot);
    QVERIFY(!evilSvgProbe2.probed && !evilSvgProbe2.external && evilSvgProbe2.cacheHit);

    // File mutable transition: valid -> corrupt (caches failure) -> repaired (invalidates cache cleanly).
    const QString mutablePath = subIcons + QStringLiteral("/mutable_probe.png");
    QVERIFY(writeAll(mutablePath, pngBytes));
    const auto mutValid = tabCache.probeSafeInAppImageSizes(QStringLiteral("icons/mutable_probe.png"), appRoot);
    QVERIFY(mutValid.probed && !mutValid.cacheHit);
    QCOMPARE(mutValid.availableSizes, (QList<QSize>{QSize(24, 24)}));

    QVERIFY(writeAll(mutablePath, QByteArrayLiteral("corrupt-png-bytes")));
    const auto mutBad1 = tabCache.probeSafeInAppImageSizes(QStringLiteral("icons/mutable_probe.png"), appRoot);
    QVERIFY(!mutBad1.probed && !mutBad1.external && !mutBad1.cacheHit && mutBad1.availableSizes.isEmpty());
    const auto mutBad2 = tabCache.probeSafeInAppImageSizes(QStringLiteral("icons/mutable_probe.png"), appRoot);
    QVERIFY(!mutBad2.probed && !mutBad2.external && mutBad2.cacheHit && mutBad2.availableSizes.isEmpty());

    QImage repairedImg(16, 16, QImage::Format_ARGB32);
    repairedImg.fill(QColor(80, 200, 120, 255));
    QByteArray repairedBytes;
    {
        QBuffer repBuf(&repairedBytes);
        QVERIFY(repBuf.open(QIODevice::WriteOnly));
        QVERIFY(repairedImg.save(&repBuf, "PNG"));
    }
    QVERIFY(writeAll(mutablePath, repairedBytes));
    const auto mutFixed = tabCache.probeSafeInAppImageSizes(QStringLiteral("icons/mutable_probe.png"), appRoot);
    QVERIFY(mutFixed.probed && !mutFixed.external && !mutFixed.cacheHit);
    QCOMPARE(mutFixed.availableSizes, (QList<QSize>{QSize(16, 16)}));

    // Derive pixel size helper parity across normal & Common Tabs.
    QCOMPARE(deriveTabCustomImagePixelAfterImageChange(QStringLiteral("icons/multi.ico"), QSize(16, 16), QString(), {}), QSize());
    QCOMPARE(deriveTabCustomImagePixelAfterImageChange(QStringLiteral("icons/multi.ico"), QSize(16, 16), QStringLiteral("icons/multi.ico"), {QSize(32, 32)}), QSize(16, 16));
    QCOMPARE(deriveTabCustomImagePixelAfterImageChange(QStringLiteral("icons/multi.ico"), QSize(16, 16), QStringLiteral("icons/other.ico"), {QSize(32, 32), QSize(16, 16)}), QSize(16, 16));
    QCOMPARE(deriveTabCustomImagePixelAfterImageChange(QStringLiteral("icons/multi.ico"), QSize(16, 16), QStringLiteral("icons/single.png"), {QSize(24, 24)}), QSize());

    // TAB-ASSET-R2-GUI-PREFIX: sibling same-prefix dirs stay external absolute paths, while true children become relative.
    const QString cleanAppRoot = QDir::cleanPath(QDir::fromNativeSeparators(QDir(appRoot).absolutePath()));
    const QString siblingAssetsDir = cleanAppRoot + QStringLiteral("-assets");
    const QString siblingExtraDir = cleanAppRoot + QStringLiteral("_extra");
    QVERIFY(QDir().mkpath(siblingAssetsDir));
    QVERIFY(QDir().mkpath(siblingExtraDir));
    QVERIFY(writeAll(siblingAssetsDir + QStringLiteral("/ext.png"), pngBytes));
    QVERIFY(writeAll(siblingExtraDir + QStringLiteral("/ext.png"), pngBytes));

    QCOMPARE(classifyTabCustomImagePathForGui(appRoot + QStringLiteral("/icons/multi.ico"), appRoot), QStringLiteral("icons/multi.ico"));
    QCOMPARE(classifyTabCustomImagePathForGui(appRoot + QStringLiteral("\\icons\\multi.ico"), appRoot), QStringLiteral("icons/multi.ico"));

    const QString sibling1 = classifyTabCustomImagePathForGui(siblingAssetsDir + QStringLiteral("/ext.png"), appRoot);
    QCOMPARE(sibling1, siblingAssetsDir + QStringLiteral("/ext.png"));
    QVERIFY(QDir::isAbsolutePath(sibling1) && !sibling1.startsWith(QStringLiteral("..")));

    const QString sibling2 = classifyTabCustomImagePathForGui(siblingExtraDir + QStringLiteral("/ext.png"), appRoot);
    QCOMPARE(sibling2, siblingExtraDir + QStringLiteral("/ext.png"));

    const QString traversalSibling = classifyTabCustomImagePathForGui(
        appRoot + QStringLiteral("/icons/../../") + QFileInfo(cleanAppRoot).fileName() + QStringLiteral("-assets/ext.png"),
        appRoot);
    QCOMPARE(traversalSibling, siblingAssetsDir + QStringLiteral("/ext.png"));
}

#include "test_mcp_control.moc"
