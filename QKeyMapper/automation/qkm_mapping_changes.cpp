#include "qkm_mapping_changes.h"
#include "qkm_auth_proof.h"
#include "qkm_macro_backup.h"
#include "qkm_pair_store.h"
#include "qkm_source_snapshot.h"
#include "../qkeymapper_constants.h"

#include <QDateTime>
#include <QJsonArray>
#include <QSet>
#include <QUuid>
#include <algorithm>
#include <limits>
#include <utility>

namespace QkmAutomation {
namespace {
constexpr MappingRowMove RowMoves[] = {MappingRowMove::Up, MappingRowMove::Down, MappingRowMove::Top, MappingRowMove::Bottom};

QString sampleTime() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }
bool operationPending(const QJsonObject &operation)
{
    const auto status = operation.value(QStringLiteral("status"));
    return status == QLatin1String("applying") || status == QLatin1String("waiting_cleanup");
}
QJsonObject saveOperationTarget(const MappingChangePreview &preview)
{
    return {{QStringLiteral("instance_id"), preview.owner.instance}, {QStringLiteral("group"), preview.save->settingName},
        {QStringLiteral("save_revision"), preview.save->revision}};
}
QJsonObject removalOperationTarget(const MappingChangePreview &preview)
{
    return {{QStringLiteral("instance_id"), preview.owner.instance},
        {QStringLiteral("source_revision"), preview.removal->sourceRevision},
        {QStringLiteral("source_tab_ref"), preview.removal->sourceTabRef}};
}

QByteArray decodeBackupDocument(const QJsonValue &value, int maxBytes)
{
    const auto encoded = value.toString();
    const int encodedLimit = ((maxBytes + 2) / 3) * 4;
    if (encoded.isEmpty() || encoded.size() > encodedLimit) return {};
    const auto bytes = encoded.toLatin1();
    auto decoded = QByteArray::fromBase64Encoding(bytes, QByteArray::AbortOnBase64DecodingErrors);
    if (!decoded || decoded.decoded.isEmpty() || decoded.decoded.size() > maxBytes || decoded.decoded.toBase64() != bytes)
        return {};
    return std::move(decoded.decoded);
}
}

const char *mappingRowMoveAction(MappingRowMove move)
{
    switch (move) {
    case MappingRowMove::Up: return "mapping.move_up";
    case MappingRowMove::Down: return "mapping.move_down";
    case MappingRowMove::Top: return "mapping.move_to_top";
    case MappingRowMove::Bottom: return "mapping.move_to_bottom";
    case MappingRowMove::None: break;
    }
    return "";
}

bool isBasicMappingKey(const QString &text)
{
    return text.size() == 1 && ((text.at(0) >= QLatin1Char('A') && text.at(0) <= QLatin1Char('Z'))
        || (text.at(0) >= QLatin1Char('0') && text.at(0) <= QLatin1Char('9')));
}

bool isMappingChangeMethod(const QString &method)
{
    return method == QStringLiteral("qkm_preview_changes") || method == QStringLiteral("qkm_apply_changes")
        || method == QStringLiteral("qkm_cancel_changes") || method == QStringLiteral("qkm_get_operation")
        || method == QStringLiteral("qkm_stop_mapping") || method == QStringLiteral("qkm_start_mapping");
}

MappingChangeRequest validateMappingChangeRequest(const QString &method, const QJsonObject &arguments)
{
    MappingChangeRequest result;
    const auto fail = [&](const char *message) {
        result.errorCode = QStringLiteral("invalid_arguments");
        result.message = QString::fromLatin1(message);
        return result;
    };
    if (!isMappingChangeMethod(method)) { return fail("Unsupported mapping change method."); }
    const bool start = method == QLatin1String("qkm_start_mapping");
    if (start || method == QLatin1String("qkm_stop_mapping")) {
        result.method = start ? MappingChangeMethod::Start : MappingChangeMethod::Stop;
        result.id = arguments.value(QStringLiteral("request_id")).toString();
        result.instance = arguments.value(QStringLiteral("instance")).toString();
        if (!isLowerHex(result.id, 32) || !isLowerHex(result.instance, 32))
            return fail("Provide a lowercase hexadecimal instance and request_id, each 32 characters.");
        if (start) {
            result.startSourceRevision = arguments.value(QStringLiteral("source_revision")).toString();
            result.startSourceTabRef = arguments.value(QStringLiteral("source_tab_ref")).toString();
            const auto source = validateSourceQueryArguments({{QStringLiteral("entity"), QStringLiteral("mappings")},
                {QStringLiteral("source_revision"), result.startSourceRevision}, {QStringLiteral("source_tab_ref"), result.startSourceTabRef}});
            if (arguments.size() != 5 || arguments.value(QStringLiteral("save_policy")) != QLatin1String("start_only") || !source.ok())
                return fail("Provide the current source revision and Tab reference with save_policy=start_only.");
        } else if (arguments.size() != 2) {
            return fail("Stop accepts only instance and request_id.");
        }
        return result;
    }
    const bool preview = method == QStringLiteral("qkm_preview_changes");
    const bool apply = method == QStringLiteral("qkm_apply_changes");
    const bool query = method == QStringLiteral("qkm_get_operation");
    result.method = preview ? MappingChangeMethod::Preview : apply ? MappingChangeMethod::Apply
        : query ? MappingChangeMethod::Operation : MappingChangeMethod::Cancel;
    const auto action = arguments.value(QStringLiteral("action"));
    if (preview && action == QLatin1String("configuration.import")) {
        auto &binding = result.configurationImport;
        binding.catalogRevision = arguments.value(QStringLiteral("catalog_revision")).toString();
        const auto groups = arguments.value(QStringLiteral("groups")).toArray();
        const auto conflict = arguments.value(QStringLiteral("on_conflict"));
        if (arguments.size() != 5 || !isLowerHex(binding.catalogRevision, 64)
            || groups.isEmpty() || groups.size() > MaxConfigurationBackupGroups
            || (conflict != QLatin1String("keep") && conflict != QLatin1String("overwrite")))
            return fail("Select up to 100 ordinary groups, a current catalog revision, a backup, and a conflict policy.");
        QSet<QString> distinct;
        for (const auto &item : groups) {
            const QString group = item.toString();
            if (group.isEmpty() || group != group.trimmed() || group.size() > 32767 || group.contains(QLatin1Char('\0'))
                || group.contains(QLatin1Char('\r')) || group.contains(QLatin1Char('\n')) || distinct.contains(group))
                return fail("Use distinct, nonempty single-line group names.");
            distinct.insert(group);
            binding.groups.append(group);
        }
        binding.document = decodeBackupDocument(arguments.value(QStringLiteral("document_base64")), MaxConfigurationBackupBytes);
        if (binding.document.isEmpty()) return fail("Provide a valid bounded base64 configuration backup.");
        binding.overwrite = conflict == QLatin1String("overwrite");
        result.action = MappingChangeAction::ImportConfiguration;
        return result;
    }
    if (preview && action == QLatin1String("configuration.save_import")) {
        auto &binding = result.configurationImport;
        binding.draftId = arguments.value(QStringLiteral("import_draft_id")).toString();
        if (arguments.size() != 2 || !isLowerHex(binding.draftId, 32))
            return fail("Provide the exact configuration import draft ID.");
        result.action = MappingChangeAction::SaveConfigurationImport;
        return result;
    }
    if (preview && (action == QLatin1String("configuration.move") || action == QLatin1String("configuration.reset_order")
            || action == QLatin1String("configuration.remove"))) {
        auto &binding = result.configurationList;
        binding.catalogRevision = arguments.value(QStringLiteral("catalog_revision")).toString();
        const bool move = action == QLatin1String("configuration.move");
        const bool remove = action == QLatin1String("configuration.remove");
        if (move) binding.action = ConfigurationListAction::Move;
        else if (remove) binding.action = ConfigurationListAction::Remove;
        const int expectedFields = move ? 4 : remove ? 3 : 2;
        if (!isLowerHex(binding.catalogRevision, 64) || arguments.size() != expectedFields)
            return fail("Provide the current configuration catalog revision and the exact groups required by this action.");
        if (!binding.reset()) {
            const auto validName = [](const QString &name) {
                return !name.isEmpty() && name == name.trimmed() && name.size() <= 32767 && !name.contains(QChar::Null)
                    && !name.contains(QLatin1Char('\r')) && !name.contains(QLatin1Char('\n'));
            };
            binding.group = arguments.value(QStringLiteral("group")).toString();
            if (move) binding.targetGroup = arguments.value(QStringLiteral("target_group")).toString();
            if (!validName(binding.group) || (move && !validName(binding.targetGroup)))
                return fail("Provide the exact single-line group names.");
        }
        result.action = MappingChangeAction::ChangeConfigurationList;
        return result;
    }
    if (preview && action == QLatin1String("table.import")) {
        auto &table = result.update;
        table.sourceTabRef = arguments.value(QStringLiteral("source_tab_ref")).toString();
        table.sourceRevision = table.sourceTabRef.left(64);
        const auto source = validateSourceQueryArguments({{QStringLiteral("entity"), QStringLiteral("mappings")},
            {QStringLiteral("source_revision"), table.sourceRevision}, {QStringLiteral("source_tab_ref"), table.sourceTabRef}});
        if (arguments.size() != 3 || !source.ok())
            return fail("Provide the attached source Tab reference and a bounded table backup.");
        table.tableImportDocument = decodeBackupDocument(arguments.value(QStringLiteral("document_base64")), MaxTableBackupBytes);
        if (table.tableImportDocument.isEmpty()) return fail("Provide a valid bounded base64 document.");
        result.action = MappingChangeAction::UpdateMapping;
        return result;
    }
    const bool macroDelete = action == QLatin1String("macro.delete");
    const bool macroImport = action == QLatin1String("macro.import");
    if (preview && (macroDelete || macroImport || action == QLatin1String("macro.upsert"))) {
        const auto value = arguments.value(QStringLiteral("macro"));
        if (arguments.size() != 2 || !value.isObject()) return fail("Macro changes require only action and macro.");
        const auto fields = value.toObject();
        const auto allowed = macroDelete
            ? QSet<QString>{QStringLiteral("catalog_revision"), QStringLiteral("scope"), QStringLiteral("names")}
            : macroImport ? QSet<QString>{QStringLiteral("catalog_revision"), QStringLiteral("scope"),
                QStringLiteral("on_conflict"), QStringLiteral("document_base64")}
            : QSet<QString>{QStringLiteral("catalog_revision"), QStringLiteral("scope"), QStringLiteral("name"),
                QStringLiteral("content"), QStringLiteral("category"), QStringLiteral("note"), QStringLiteral("insert_at"),
                QStringLiteral("segment_comments")};
        for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
            if (!allowed.contains(it.key())) return fail("Unknown macro argument.");
        }
        auto &macro = result.macro;
        macro.catalogRevision = fields.value(QStringLiteral("catalog_revision")).toString();
        macro.scope = fields.value(QStringLiteral("scope")).toString();
        if (!isLowerHex(macro.catalogRevision, 64)
            || (macro.scope != QLatin1String("configuration") && macro.scope != QLatin1String("universal"))) {
            return fail("Provide the current macro catalog revision and an explicit scope.");
        }
        if (macroImport) {
            const auto policy = fields.value(QStringLiteral("on_conflict"));
            if (policy != QLatin1String("keep") && policy != QLatin1String("overwrite")) {
                return fail("Macro import requires keep or overwrite and a bounded backup document.");
            }
            macro.importDocument = decodeBackupDocument(fields.value(QStringLiteral("document_base64")), MaxMacroBackupBytes);
            if (macro.importDocument.isEmpty()) {
                return fail("Provide a valid bounded base64 document.");
            }
            macro.overwriteImportDuplicates = policy == QLatin1String("overwrite");
            result.action = MappingChangeAction::ImportMacro;
            return result;
        }
        if (macroDelete) {
            const auto names = fields.value(QStringLiteral("names")).toArray();
            if (names.isEmpty() || names.size() > 100) return fail("Provide one to 100 macro names to delete.");
            QSet<QString> distinct;
            for (const auto &value : names) {
                const auto name = value.toString();
                if (name.isEmpty() || name.size() > 32767 || name.contains(QChar::Null) || distinct.contains(name))
                    return fail("Macro names must be distinct, non-empty strings.");
                distinct.insert(name);
                macro.namesToDelete.append(name);
            }
            result.action = MappingChangeAction::DeleteMacro;
            return result;
        }
        macro.name = fields.value(QStringLiteral("name")).toString();
        macro.content = fields.value(QStringLiteral("content")).toString();
        const auto optionalText = [&](const QString &key, std::optional<QString> &output) {
            const auto text = fields.value(key);
            if (text.isUndefined() || text.isNull()) return true;
            if (!text.isString() || text.toString().size() > 32767 || text.toString().contains(QChar::Null)) return false;
            output = text.toString();
            return true;
        };
        if (macro.name.isEmpty() || macro.name.size() > 32767 || macro.name.contains(QChar::Null)
            || macro.content.isEmpty() || macro.content.size() > QKeyMapperConstants::MAPPINGKEY_LINE_EDIT_MAX_LENGTH
            || macro.content.contains(QChar::Null)
            || !optionalText(QStringLiteral("category"), macro.category) || !optionalText(QStringLiteral("note"), macro.note)) {
            return fail("Provide the macro catalog revision, scope, name and content. Category and note are optional strings.");
        }
        const auto placement = fields.value(QStringLiteral("insert_at"));
        if (!placement.isUndefined() && !placement.isNull()) {
            const auto object = placement.toObject();
            const auto position = object.value(QStringLiteral("position")).toString();
            macro.anchorName = object.value(QStringLiteral("anchor_name")).toString();
            if (object.size() != 2 || (position != QLatin1String("above") && position != QLatin1String("below"))
                || macro.anchorName.isEmpty() || macro.anchorName.size() > 32767 || macro.anchorName.contains(QChar::Null)) {
                return fail("Macro insertion needs above or below and an exact anchor name.");
            }
            macro.insertBelow = position == QLatin1String("below");
        }
        const auto comments = fields.value(QStringLiteral("segment_comments"));
        if (!comments.isUndefined() && !comments.isNull()) {
            const auto array = comments.toArray();
            if (array.isEmpty()) return fail("Segment comments require a non-empty array of strings.");
            QStringList parsed;
            parsed.reserve(array.size());
            for (const auto &item : array) {
                const auto text = item.toString();
                if (!item.isString() || text.size() > 32767 || text.contains(QChar::Null))
                    return fail("Invalid macro segment comment.");
                parsed.append(text);
            }
            macro.segmentComments = std::move(parsed);
        }
        result.action = MappingChangeAction::UpsertMacro;
        return result;
    }
    const bool savePreview = preview && action.isString() && action.toString() == QStringLiteral("configuration.save");
    const bool deletePreview = preview && action.isString() && action.toString() == QStringLiteral("mapping.delete");
    const bool updatePreview = preview && action == QJsonValue(QStringLiteral("mapping.update"));
    const bool duplicatePreview = preview && action == QLatin1String("mapping.duplicate");
    MappingRowMove rowMove = MappingRowMove::None;
    if (preview) {
        for (const auto move : RowMoves) {
            if (action == QLatin1String(mappingRowMoveAction(move))) {
                rowMove = move;
                break;
            }
        }
    }
    const bool movePreview = rowMove != MappingRowMove::None;
    const bool renamePreview = preview && action == QLatin1String("tab.rename");
    const bool tabHotkeyPreview = preview && action == QLatin1String("tab.set_hotkey");
    const bool tabNotificationPreview = preview && action == QLatin1String("tab.set_notification_mode");
    const bool tabNotificationColorsPreview = preview && action == QLatin1String("tab.set_notification_colors");
    const bool categoryFilterPreview = preview && action == QLatin1String("tab.set_category_filter");
    const bool disabledVisibilityPreview = preview && action == QLatin1String("configuration.set_show_disabled_rows");
    const bool mappingDisplayPreview = preview && action == QLatin1String("configuration.set_mapping_display");
    const bool insertModePreview = preview && action == QLatin1String("general.set_table_insert_mode");
    const bool editTriggerPreview = preview && action == QLatin1String("general.set_table_edit_trigger");
    const bool windowDisplayModePreview = preview && action == QLatin1String("configuration.set_window_display_mode");
    const bool saveWindowSizePreview = preview && action == QLatin1String("general.set_save_window_size");
    const bool saveSplitterPositionPreview = preview && action == QLatin1String("general.set_save_splitter_position");
    const bool startupPositionPreview = preview && action == QLatin1String("general.set_startup_position");
    const bool addBlankTabPreview = preview && action == QLatin1String("tab.add_blank");
    const bool copyTabPreview = preview && action == QLatin1String("tab.copy");
    const bool deleteTabPreview = preview && action == QLatin1String("tab.delete");
    const bool clearTabPreview = preview && action == QLatin1String("tab.clear");
    const bool moveTabPreview = preview && action == QLatin1String("tab.move");
    const bool selectTabPreview = preview && action == QLatin1String("tab.select");
    const bool tabCommonPreview = preview && action == QLatin1String("tab.set_common_inheritance");
    const bool tabCollapsePreview = preview && action == QLatin1String("tab.set_common_collapsed");
    const bool commonFeaturePreview = preview && action == QLatin1String("configuration.set_common_enabled");
    const bool tabActionPreview = addBlankTabPreview || copyTabPreview || deleteTabPreview
        || clearTabPreview || moveTabPreview || tabCommonPreview || tabCollapsePreview || commonFeaturePreview || selectTabPreview
        || tabHotkeyPreview || tabNotificationPreview || tabNotificationColorsPreview || categoryFilterPreview
        || disabledVisibilityPreview || mappingDisplayPreview || insertModePreview || editTriggerPreview
        || windowDisplayModePreview || saveWindowSizePreview || saveSplitterPositionPreview || startupPositionPreview;
    const bool preferencesPreview = preview && action == QJsonValue(QStringLiteral("ibinput.preferences.update"));
    if (preview && !action.isUndefined() && (!action.isString()
        || (action.toString() != QStringLiteral("mapping.add") && !savePreview && !deletePreview && !updatePreview && !movePreview && !duplicatePreview && !renamePreview && !tabActionPreview && !preferencesPreview))) return fail("Unsupported preview action.");
    const QString idField = query ? QStringLiteral("operation_id") : QStringLiteral("preview_id");
    const QSet<QString> allowed = savePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("setting_name")}
        : preferencesPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("preferences_revision"),
            QStringLiteral("preferred_backend"), QStringLiteral("unavailable_policy")}
        : renamePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("tab_name")}
        : tabHotkeyPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("tab_hotkey")}
        : tabNotificationPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("notification_mode")}
        : mappingDisplayPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("mapping_display")}
        : insertModePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("table_insert_mode")}
        : editTriggerPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("table_edit_trigger")}
        : windowDisplayModePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("window_display_mode")}
        : saveWindowSizePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("save_window_size")}
        : saveSplitterPositionPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("save_splitter_position")}
        : startupPositionPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("startup_position")}
        : disabledVisibilityPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("show_disabled_rows")}
        : categoryFilterPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("category_filter")}
        : tabNotificationColorsPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"),
            QStringLiteral("notification_font_color"), QStringLiteral("notification_background_color")}
        : (moveTabPreview || selectTabPreview) ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("target_tab_ref")}
        : tabCommonPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("include_common")}
        : tabCollapsePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("common_collapsed")}
        : commonFeaturePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("common_feature_enabled")}
        : tabActionPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref")}
        : movePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_ref"), QStringLiteral("source_refs")}
        : duplicatePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_ref"), QStringLiteral("source_refs"), QStringLiteral("copy_to")}
        : updatePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_ref"), QStringLiteral("source_refs"), QStringLiteral("send_method"),
            QStringLiteral("original_key"), QStringLiteral("mapping_key"), QStringLiteral("key_up_mapping_key"), QStringLiteral("disabled"), QStringLiteral("note"), QStringLiteral("category"), QStringLiteral("row_modes"), QStringLiteral("send_timing")}
        : deletePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_revision"),
            QStringLiteral("source_tab_ref"), QStringLiteral("source_refs")} : preview
        ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_revision"), QStringLiteral("source_tab_ref"),
            QStringLiteral("original_key"), QStringLiteral("mapping_key"), QStringLiteral("insert_row"), QStringLiteral("send_method")}
        : apply ? QSet<QString>{idField, QStringLiteral("confirm_conflicts"), QStringLiteral("confirm_scope"),
            QStringLiteral("confirm_overwrite"), QStringLiteral("confirm_delete")}
            : QSet<QString>{idField};
    for (auto it = arguments.constBegin(); it != arguments.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return fail("Unknown mapping change argument.");
    }
    const auto sendMethod = [&]() -> int {
        const auto value = arguments.value(QStringLiteral("send_method"));
        if (value.isUndefined()) return updatePreview ? -1 : 0;
        if (!value.isDouble() || value.toDouble() < 0 || value.toDouble() > 3
            || value.toDouble() != value.toInt(-1)) return -1;
        return value.toInt();
    };
    const auto basicKey = [](const QJsonValue &value) { return isBasicMappingKey(value.toString()); };
    if (preferencesPreview) {
        const auto revision = arguments.value(QStringLiteral("preferences_revision"));
        const auto backend = arguments.value(QStringLiteral("preferred_backend"));
        const auto policy = arguments.value(QStringLiteral("unavailable_policy"));
        if (!revision.isString() || !isLowerHex(revision.toString(), 64)
            || (backend.isUndefined() && policy.isUndefined())) return fail("A current preferences revision and at least one preference are required.");
        if (!backend.isUndefined() && (!backend.isString() || !QStringList{QStringLiteral("auto"), QStringLiteral("logitech_ghub"),
            QStringLiteral("logitech_legacy"), QStringLiteral("razer")}.contains(backend.toString()))) return fail("Unsupported preferred backend.");
        if (!policy.isUndefined() && (!policy.isString() || !QStringList{QStringLiteral("block"),
            QStringLiteral("sendinput_before_start")}.contains(policy.toString()))) return fail("Unsupported unavailable policy.");
        result.action = MappingChangeAction::UpdateIbInputPreferences;
        result.preferences = {revision.toString(), backend.toString(), policy.toString()};
        return result;
    }
    if (renamePreview || tabActionPreview) {
        const auto reference = arguments.value(QStringLiteral("source_tab_ref"));
        const auto name = arguments.value(QStringLiteral("tab_name"));
        const QString text = name.toString();
        if (renamePreview && (!name.isString() || text.size() > 32767 || text.contains(QLatin1Char('\0'))
            || text.contains(QLatin1Char('\r')) || text.contains(QLatin1Char('\n'))))
            return fail("A Tab name must be a single line of at most 32767 UTF-16 code units.");
        const QString revision = reference.toString().left(64);
        const auto source = validateSourceQueryArguments({{QStringLiteral("entity"), QStringLiteral("mappings")},
            {QStringLiteral("source_revision"), revision}, {QStringLiteral("source_tab_ref"), reference}});
        if (!source.ok()) return fail("A canonical current source Tab reference is required.");
        result.action = MappingChangeAction::UpdateMapping;
        result.update.sourceRevision = revision;
        result.update.sourceTabRef = reference.toString();
        if (renamePreview) result.update.tabName = text;
        if (insertModePreview) {
            const int mode = arguments.value(QStringLiteral("table_insert_mode")).toInt(-1);
            if (mode != QKeyMapperConstants::TABLE_INSERT_MODE_ABOVE
                && mode != QKeyMapperConstants::TABLE_INSERT_MODE_BELOW) {
                return fail("Table insert mode must be 0 (above) or 1 (below).");
            }
            result.update.tableInsertMode = mode;
        }
        if (editTriggerPreview) {
            const int trigger = arguments.value(QStringLiteral("table_edit_trigger")).toInt(-1);
            if (trigger != QKeyMapperConstants::EDITMODE_RIGHT_DOUBLECLICK
                && trigger != QKeyMapperConstants::EDITMODE_LEFT_DOUBLECLICK) {
                return fail("Table edit trigger must be 0 (right double-click) or 1 (left double-click).");
            }
            result.update.tableEditTrigger = trigger;
        }
        if (windowDisplayModePreview) {
            const int mode = arguments.value(QStringLiteral("window_display_mode")).toInt(-1);
            if (mode < QKeyMapperConstants::WINDOWINFO_DISPLAYMODE_MATCH_INDEX_MIN
                || mode > QKeyMapperConstants::WINDOWINFO_DISPLAYMODE_MATCH_INDEX_MAX) {
                return fail("Window display mode must be 0 (ignore), 1 (fullscreen), or 2 (windowed).");
            }
            result.update.windowDisplayMode = mode;
        }
        if (saveWindowSizePreview) {
            const auto value = arguments.value(QStringLiteral("save_window_size"));
            if (!value.isBool()) return fail("Provide an explicit boolean save_window_size.");
            result.update.saveWindowSize = value.toBool();
        }
        if (saveSplitterPositionPreview) {
            const auto value = arguments.value(QStringLiteral("save_splitter_position"));
            if (!value.isBool()) return fail("Provide an explicit boolean save_splitter_position.");
            result.update.saveSplitterPosition = value.toBool();
        }
        if (startupPositionPreview) {
            using namespace QKeyMapperConstants;
            const auto fields = arguments.value(QStringLiteral("startup_position")).toObject();
            const auto mode = fields.value(QStringLiteral("mode"));
            const auto point = fields.value(QStringLiteral("point"));
            if (fields.isEmpty() || fields.size() != int(!mode.isUndefined()) + int(!point.isUndefined()))
                return fail("Provide at least one startup position field, mode or point.");
            if (!mode.isUndefined()) {
                const int index = mode.toInt(-1);
                if (index < STARTUP_POSITION_MIN || index > STARTUP_POSITION_MAX)
                    return fail("Startup position mode must be 0 (default), 1 (last saved), or 2 (specified).");
                result.update.startupPositionMode = index;
            }
            if (!point.isUndefined()) {
                const auto coordinates = point.toObject();
                const int x = coordinates.value(QStringLiteral("x")).toInt(STARTUP_SPECIFY_POSITION_MIN_X - 1);
                const int y = coordinates.value(QStringLiteral("y")).toInt(STARTUP_SPECIFY_POSITION_MIN_Y - 1);
                if (coordinates.size() != 2 || x < STARTUP_SPECIFY_POSITION_MIN_X || x > STARTUP_SPECIFY_POSITION_MAX_X
                    || y < STARTUP_SPECIFY_POSITION_MIN_Y || y > STARTUP_SPECIFY_POSITION_MAX_Y)
                    return fail("Provide both integer coordinates within -50000 to 50000.");
                if (result.update.startupPositionMode.value_or(STARTUP_POSITION_SPECIFY) != STARTUP_POSITION_SPECIFY)
                    return fail("Coordinates require the specified startup position mode.");
                result.update.startupPositionPoint = QPoint(x, y);
            }
        }
        if (tabHotkeyPreview) {
            const auto hotkey = arguments.value(QStringLiteral("tab_hotkey"));
            const QString value = hotkey.toString();
            if (!hotkey.isString() || value.size() > 32767 || value.contains(QLatin1Char('\0'))
                || value.contains(QLatin1Char('\r')) || value.contains(QLatin1Char('\n')))
                return fail("A single-line tab_hotkey of at most 32767 UTF-16 code units is required.");
            result.update.tabHotkey = value;
        }
        if (tabNotificationPreview) {
            const auto value = arguments.value(QStringLiteral("notification_mode"));
            const int mode = value.toInt(-1);
            if (!value.isDouble() || value.toDouble() != mode || mode < Qt::Unchecked || mode > Qt::Checked)
                return fail("notification_mode must be 0, 1, or 2.");
            result.update.tabNotificationMode = static_cast<Qt::CheckState>(mode);
        }
        if (mappingDisplayPreview) {
            const auto change = arguments.value(QStringLiteral("mapping_display")).toObject();
            if (change.isEmpty()) return fail("Provide at least one mapping display preference.");
            for (auto it = change.constBegin(); it != change.constEnd(); ++it) {
                if (!it.value().isBool() || (it.key() != QStringLiteral("category_column")
                    && it.key() != QStringLiteral("notes") && it.key() != QStringLiteral("floating_column")))
                    return fail("Only category_column, notes and floating_column booleans are allowed.");
            }
            result.update.mappingDisplay = change;
        }
        if (disabledVisibilityPreview) {
            const auto value = arguments.value(QStringLiteral("show_disabled_rows"));
            if (!value.isBool()) return fail("A boolean show_disabled_rows is required.");
            result.update.showDisabledRows = value.toBool();
        }
        if (categoryFilterPreview) {
            const auto filter = arguments.value(QStringLiteral("category_filter"));
            if (filter != QJsonValue(QStringLiteral("all"))) {
                const auto names = filter.toArray();
                if (!filter.isArray() || names.size() > 100)
                    return fail("Use all or a list of up to 100 category names.");
                QSet<QString> selected;
                for (const auto &value : names) {
                    const QString name = value.toString();
                    if (!value.isString() || name.size() > 32767 || name.contains(QLatin1Char('\0'))
                        || name.contains(QLatin1Char('\r')) || name.contains(QLatin1Char('\n')) || selected.contains(name))
                        return fail("Category filter names must be distinct single-line strings.");
                    selected.insert(name);
                }
            }
            result.update.tabCategoryFilter = filter;
        }
        if (tabNotificationColorsPreview) {
            const auto font = arguments.value(QStringLiteral("notification_font_color"));
            const auto background = arguments.value(QStringLiteral("notification_background_color"));
            const auto colorValid = [](const QJsonValue &value, int digits) {
                if (value.isUndefined()) return true;
                if (!value.isString()) return false;
                const QString color = value.toString();
                return color == QStringLiteral("default")
                    || (color.startsWith(QLatin1Char('#')) && isLowerHex(color.mid(1).toLower(), digits));
            };
            if ((font.isUndefined() && background.isUndefined()) || !colorValid(font, 6) || !colorValid(background, 8))
                return fail("Provide a #RRGGBB font color, a #AARRGGBB background color, or default for either.");
            result.update.tabNotificationFontColor = font.toString();
            result.update.tabNotificationBackgroundColor = background.toString();
        }
        result.update.addBlankTab = addBlankTabPreview;
        result.update.copyTab = copyTabPreview;
        result.update.deleteTab = deleteTabPreview;
        result.update.clearTab = clearTabPreview;
        if (moveTabPreview || selectTabPreview) {
            const auto target = arguments.value(QStringLiteral("target_tab_ref"));
            const auto targetQuery = validateSourceQueryArguments({{QStringLiteral("entity"), QStringLiteral("mappings")},
                {QStringLiteral("source_revision"), revision}, {QStringLiteral("source_tab_ref"), target}});
            if (!targetQuery.ok() || (moveTabPreview && target == reference))
                return fail("A canonical destination from the same revision is required. Tab moves need a different destination.");
            result.update.targetTabRef = target.toString();
            result.update.selectTab = selectTabPreview;
        }
        if (tabCommonPreview) {
            const auto includeCommon = arguments.value(QStringLiteral("include_common"));
            if (!includeCommon.isBool()) return fail("A boolean include_common is required.");
            result.update.tabIncludeCommon = includeCommon.toBool();
        }
        if (tabCollapsePreview) {
            const auto collapsed = arguments.value(QStringLiteral("common_collapsed"));
            if (!collapsed.isBool()) return fail("A boolean common_collapsed is required.");
            result.update.tabCommonCollapsed = collapsed.toBool();
        }
        if (commonFeaturePreview) {
            const auto enabled = arguments.value(QStringLiteral("common_feature_enabled"));
            if (!enabled.isBool()) return fail("A boolean common_feature_enabled is required.");
            result.update.commonFeatureEnabled = enabled.toBool();
        }
        return result;
    }
    if (updatePreview || movePreview || duplicatePreview) {
        const auto singleReference = arguments.value(QStringLiteral("source_ref"));
        const auto batch = arguments.value(QStringLiteral("source_refs"));
        const auto references = batch.toArray();
        const auto category = arguments.value(QStringLiteral("category"));
        const auto rowModes = arguments.value(QStringLiteral("row_modes"));
        const bool triggerBatch = !batch.isUndefined() && rowModes.toObject().contains(QStringLiteral("original_trigger"));
        if (!batch.isUndefined() && ((!movePreview && !duplicatePreview && category.isUndefined() && !triggerBatch)
            || !singleReference.isUndefined() || references.size() < 2 || references.size() > 100))
            return fail("Only moves, duplication, category and trigger updates accept 2 to 100 source_refs instead of source_ref.");
        const auto reference = references.isEmpty() ? singleReference : references.first();
        const auto original = arguments.value(QStringLiteral("original_key"));
        const auto mapping = arguments.value(QStringLiteral("mapping_key"));
        const auto keyUp = arguments.value(QStringLiteral("key_up_mapping_key"));
        const auto disabled = arguments.value(QStringLiteral("disabled"));
        const auto note = arguments.value(QStringLiteral("note"));
        const auto timing = arguments.value(QStringLiteral("send_timing"));
        if (!timing.isUndefined()) {
            const int mode = timing.toInt(-1);
            if (arguments.size() != 3 || !singleReference.isString() || !timing.isDouble() || timing.toDouble() != mode
                || mode < QKeyMapperConstants::SENDTIMING_NORMAL || mode > QKeyMapperConstants::SENDTIMING_NORMAL_AND_KEYUP)
                return fail("Provide one source_ref and an integer send_timing from 0 to 4.");
        }
        if (!rowModes.isUndefined()) {
            const auto modes = rowModes.toObject();
            if (modes.isEmpty() || arguments.size() != 3 || (!singleReference.isString() && !triggerBatch))
                return fail("Provide row_modes and an eligible source reference, without other changed fields.");
            for (auto it = modes.constBegin(); it != modes.constEnd(); ++it) {
                const QString key = it.key();
                const auto value = it.value();
                if (key == QStringLiteral("burst") || key == QStringLiteral("lock")
                    || key == QStringLiteral("mapping_key_unlock") || key == QStringLiteral("disable_original_key_unlock")
                    || key == QStringLiteral("sequence_hold_down") || key == QStringLiteral("pass_through")
                    || key == QStringLiteral("check_combo_order") || key == QStringLiteral("unbreakable")
                    || key == QStringLiteral("disable_fn_key_switch")) {
                    if (!value.isBool()) return fail("Row toggles must be booleans.");
                } else if (key == QStringLiteral("burst_press_ms") || key == QStringLiteral("burst_release_ms")) {
                    const int ms = value.toInt(-1);
                    if (!value.isDouble() || value.toDouble() != ms || ms < QKeyMapperConstants::BURST_TIME_MIN
                        || ms > QKeyMapperConstants::BURST_TIME_MAX)
                        return fail("Burst timings must be integer milliseconds within the supported range.");
                } else if (key == QStringLiteral("repeat_mode") || key == QStringLiteral("repeat_times")) {
                    const int number = value.toInt(-1);
                    const bool mode = key == QStringLiteral("repeat_mode");
                    const int minimum = mode ? QKeyMapperConstants::REPEAT_MODE_NONE : QKeyMapperConstants::REPEAT_TIMES_MIN;
                    const int maximum = mode ? QKeyMapperConstants::REPEAT_MODE_BYTIMES : QKeyMapperConstants::REPEAT_TIMES_MAX;
                    if (!value.isDouble() || value.toDouble() != number || number < minimum || number > maximum)
                        return fail("Repeat mode and count must be integers within the supported range.");
                } else if (key == QStringLiteral("fixed_vkey_code") || key == QStringLiteral("paste_text_mode")) {
                    const int number = value.toInt(-1);
                    const bool fixed = key == QStringLiteral("fixed_vkey_code");
                    const int minimum = fixed ? QKeyMapperConstants::FIXED_VIRTUAL_KEY_CODE_MIN : QKeyMapperConstants::PASTETEXT_MODE_SHIFTINSERT;
                    const int maximum = fixed ? QKeyMapperConstants::FIXED_VIRTUAL_KEY_CODE_MAX : QKeyMapperConstants::PASTETEXT_MODE_CTRLV;
                    if (!value.isDouble() || value.toDouble() != number || number < minimum || number > maximum)
                        return fail("Fixed virtual key code and paste mode must be integers within the supported range.");
                } else if (key == QStringLiteral("original_trigger")) {
                    const auto trigger = value.toObject();
                    const QString type = trigger.value(QStringLiteral("type")).toString();
                    const auto duration = trigger.value(QStringLiteral("duration_ms"));
                    const bool normal = type == QStringLiteral("normal");
                    const int time = duration.toInt(-1);
                    if (modes.size() != 1 || trigger.size() != (duration.isUndefined() ? 1 : 2)
                        || (triggerBatch && !duration.isUndefined())
                        || (!normal && type != QStringLiteral("long_press") && type != QStringLiteral("double_press"))
                        || (!duration.isUndefined() && (normal || !duration.isDouble() || duration.toDouble() != time
                            || time < QKeyMapperConstants::PRESSTIME_MIN || time > QKeyMapperConstants::PRESSTIME_MAX)))
                        return fail("Provide only original_trigger with type normal, long_press or double_press and an optional timed duration from 1 to 9999 ms.");
                } else {
                    return fail("Unsupported row mode field.");
                }
            }
        }
        if (!note.isUndefined() || !category.isUndefined()) {
            const auto &value = note.isUndefined() ? category : note;
            const QString text = value.toString();
            if (!value.isString() || (!note.isUndefined() && !category.isUndefined())
                || text.size() > 32767 || text.contains(QLatin1Char('\0'))
                || text.contains(QLatin1Char('\r')) || text.contains(QLatin1Char('\n'))
                || !original.isUndefined() || !mapping.isUndefined() || !keyUp.isUndefined() || !disabled.isUndefined()
                || arguments.contains(QStringLiteral("send_method")))
                return fail("Provide one single-line note or category of at most 32767 UTF-16 code units without other changed mapping fields.");
        }
        if (!disabled.isUndefined() && (!disabled.isBool() || !original.isUndefined() || !mapping.isUndefined()
            || !keyUp.isUndefined() || arguments.contains(QStringLiteral("send_method"))))
            return fail("Disabled must be a boolean updated separately from key and send method fields.");
        if ((!original.isUndefined() && !basicKey(original)) || (!mapping.isUndefined() && !basicKey(mapping))
            || (!keyUp.isUndefined() && !(keyUp.isString() && keyUp.toString().isEmpty()) && !basicKey(keyUp))
            || (arguments.contains(QStringLiteral("send_method")) && sendMethod() < 0))
            return fail("Updates accept uppercase single keys, an optional empty key-up key, and send method 0 to 3.");
        if (updatePreview && original.isUndefined() && mapping.isUndefined() && keyUp.isUndefined() && sendMethod() < 0
            && disabled.isUndefined() && note.isUndefined() && category.isUndefined() && rowModes.isUndefined() && timing.isUndefined())
            return fail("At least one mapping field must be specified.");
        const auto parts = reference.toString().split(QLatin1Char(':'));
        bool tabOk = false, rowOk = false;
        const int tab = parts.size() == 4 ? parts.at(2).toInt(&tabOk) : -1;
        const int row = parts.size() == 4 ? parts.at(3).toInt(&rowOk) : -1;
        if (!reference.isString() || reference.toString().size() > 128 || parts.size() != 4
            || !isLowerHex(parts.at(0), 64) || parts.at(1) != QStringLiteral("row") || !tabOk || !rowOk
            || tab < 0 || row < 0 || QString::number(tab) != parts.at(2) || QString::number(row) != parts.at(3))
            return fail("A canonical current source row is required.");
        const int rowCount = qMax(1, int(references.size()));
        const QString prefix = reference.toString().section(QLatin1Char(':'), 0, 2) + QLatin1Char(':');
        QString copyPosition;
        QString copyAnchorRef;
        QString copyTargetTabRef;
        if (duplicatePreview) {
            const auto destination = arguments.value(QStringLiteral("copy_to")).toObject();
            const QString position = destination.value(QStringLiteral("position")).toString();
            const auto anchor = destination.value(QStringLiteral("anchor_ref"));
            const auto target = destination.value(QStringLiteral("target_tab_ref"));
            const bool relative = position == QLatin1String("above") || position == QLatin1String("below");
            if (destination.size() != (relative ? 2 : 1) + (target.isUndefined() ? 0 : 1)
                || (!relative && position != QLatin1String("top") && position != QLatin1String("bottom"))
                || (!relative && !anchor.isUndefined()))
                return fail("Provide copy_to position top, bottom, above or below. Only above and below require anchor_ref.");
            QString anchorPrefix = prefix;
            if (!target.isUndefined()) {
                const auto targetQuery = validateSourceQueryArguments({{QStringLiteral("entity"), QStringLiteral("mappings")},
                    {QStringLiteral("source_revision"), parts.at(0)}, {QStringLiteral("source_tab_ref"), target}});
                if (!targetQuery.ok()) return fail("The copy target must be a canonical Tab of the same revision.");
                copyTargetTabRef = target.toString();
                anchorPrefix = parts.at(0) + QStringLiteral(":row:") + copyTargetTabRef.section(QLatin1Char(':'), 2, 2) + QLatin1Char(':');
            }
            if (relative) {
                const QString anchorRef = anchor.toString();
                bool indexOk = false;
                const int index = anchorRef.mid(anchorPrefix.size()).toInt(&indexOk);
                if (!anchor.isString() || !indexOk || index < 0 || anchorRef != anchorPrefix + QString::number(index))
                    return fail("The insert anchor must be a canonical row of the target Tab and revision.");
            }
            copyPosition = position;
            copyAnchorRef = anchor.toString();
        }
        QList<int> batchRows;
        if (movePreview) {
            if (row > std::numeric_limits<int>::max() - (rowCount - 1))
                return fail("The contiguous source range exceeds the row index limit.");
            for (int i = 1; i < rowCount; ++i) {
                if (references.at(i) != prefix + QString::number(row + i))
                    return fail("Batch moves require contiguous, ascending row references of the same source and revision.");
            }
        } else if (!references.isEmpty()) {
            for (const auto &value : references) {
                const QString ref = value.toString();
                bool indexOk = false;
                const int index = ref.mid(prefix.size()).toInt(&indexOk);
                if (!indexOk || index < 0 || ref != prefix + QString::number(index) || batchRows.contains(index))
                    return fail("Batch references must be distinct rows of the same source and revision.");
                batchRows.append(index);
            }
            std::sort(batchRows.begin(), batchRows.end());
        }
        const int firstRow = batchRows.isEmpty() ? row : batchRows.constFirst();
        result.action = MappingChangeAction::UpdateMapping;
        result.update = {prefix + QString::number(firstRow), parts.at(0), parts.at(0) + QStringLiteral(":source:") + parts.at(2),
            firstRow, updatePreview ? sendMethod() : -1};
        result.update.move = rowMove;
        if (movePreview) result.update.rowCount = rowCount;
        result.update.batchRows = std::move(batchRows);
        result.update.copyPosition = copyPosition;
        result.update.copyAnchorRef = copyAnchorRef;
        result.update.copyTargetTabRef = copyTargetTabRef;
        if (!original.isUndefined()) result.update.originalKey = original.toString();
        if (!mapping.isUndefined()) result.update.mappingKey = mapping.toString();
        if (!keyUp.isUndefined()) result.update.keyUpMappingKey = keyUp.toString();
        if (!disabled.isUndefined()) result.update.disabled = disabled.toBool();
        if (!note.isUndefined()) result.update.note = note.toString();
        if (!category.isUndefined()) result.update.category = category.toString();
        result.update.rowModes = rowModes.toObject();
        if (!timing.isUndefined()) result.update.sendTiming = timing.toInt();
        return result;
    }
    if (savePreview) {
        const auto name = arguments.value(QStringLiteral("setting_name"));
        if (!name.isString() || name.toString().isEmpty() || name.toString().size() > 32767)
            return fail("An explicit setting name of 1 to 32767 UTF-16 code units is required.");
        result.action = MappingChangeAction::SaveConfiguration;
        result.settingName = name.toString();
        return result;
    }
    if (!preview) {
        const auto id = arguments.value(idField);
        if (!id.isString() || !isLowerHex(id.toString(), 32)) return fail("A canonical 32-character preview or operation identifier is required.");
        result.id = id.toString();
        const auto confirmation = arguments.value(QStringLiteral("confirm_conflicts"));
        if (!confirmation.isUndefined() && !confirmation.isBool()) return fail("Conflict confirmation must be a boolean.");
        result.confirmConflicts = confirmation.toBool();
        const auto scope = arguments.value(QStringLiteral("confirm_scope"));
        const auto overwrite = arguments.value(QStringLiteral("confirm_overwrite"));
        const auto deletion = arguments.value(QStringLiteral("confirm_delete"));
        if ((!scope.isUndefined() && !scope.isBool()) || (!overwrite.isUndefined() && !overwrite.isBool())
            || (!deletion.isUndefined() && !deletion.isBool()))
            return fail("Save and deletion confirmations must be booleans.");
        result.confirmScope = scope.toBool();
        result.confirmOverwrite = overwrite.toBool();
        result.confirmDelete = deletion.toBool();
        return result;
    }
    const QJsonObject sourceQuery{{QStringLiteral("entity"), QStringLiteral("mappings")},
        {QStringLiteral("source_revision"), arguments.value(QStringLiteral("source_revision"))},
        {QStringLiteral("source_tab_ref"), arguments.value(QStringLiteral("source_tab_ref"))}};
    const auto source = validateSourceQueryArguments(sourceQuery);
    if (!source.ok()) { result.errorCode = source.errorCode; result.message = source.message; return result; }
    if (deletePreview) {
        const auto references = arguments.value(QStringLiteral("source_refs"));
        if (!references.isArray() || references.toArray().isEmpty() || references.toArray().size() > 100)
            return fail("Deletion requires 1 to 100 source row references.");
        result.action = MappingChangeAction::DeleteMapping;
        result.removal.sourceRevision = arguments.value(QStringLiteral("source_revision")).toString();
        result.removal.sourceTabRef = arguments.value(QStringLiteral("source_tab_ref")).toString();
        const QString sourceIndex = result.removal.sourceTabRef.section(QLatin1Char(':'), 2, 2);
        const auto rows = references.toArray();
        for (const auto &value : rows) {
            if (!value.isString() || value.toString().size() > 128) return fail("Malformed deletion source reference.");
            const QStringList parts = value.toString().split(QLatin1Char(':'));
            bool valid = false;
            const int row = parts.size() == 4 ? parts.at(3).toInt(&valid) : -1;
            if (parts.size() != 4 || !isLowerHex(parts.at(0), 64) || parts.at(1) != QStringLiteral("row")
                || !valid || row < 0 || QString::number(row) != parts.at(3)) return fail("Malformed deletion source reference.");
            if (parts.at(0) != result.removal.sourceRevision || parts.at(2) != sourceIndex)
                return fail("All deletion references must belong to the same source and revision.");
            if (!result.removal.sourceRows.contains(row)) result.removal.sourceRows.append(row);
        }
        return result;
    }
    if (sendMethod() < 0) return fail("Send method must be an integer from 0 to 3.");
    result.addition.sendMethod = sendMethod();
    if (!basicKey(arguments.value(QStringLiteral("original_key"))) || !basicKey(arguments.value(QStringLiteral("mapping_key")))) {
        return fail("This capability accepts one uppercase letter or digit for each key; full mapping syntax is not connected yet.");
    }
    const auto row = arguments.value(QStringLiteral("insert_row"));
    if (!row.isUndefined() && !row.isNull()) {
        if (!row.isDouble() || !(row.toDouble() >= 0 && row.toDouble() <= 2147483647.0)) return fail("Insertion row is out of range.");
        result.addition.insertRow = row.toInt(-1);
        if (row.toDouble() != result.addition.insertRow) return fail("Insertion row must be an integer.");
    }
    result.addition.sourceRevision = arguments.value(QStringLiteral("source_revision")).toString();
    result.addition.sourceTabRef = arguments.value(QStringLiteral("source_tab_ref")).toString();
    result.addition.originalKey = arguments.value(QStringLiteral("original_key")).toString();
    result.addition.mappingKey = arguments.value(QStringLiteral("mapping_key")).toString();
    return result;
}

QJsonArray mappingChangeCapabilities(bool allowRunControls)
{
    const auto string = [](const QString &pattern, int maximum) {
        return QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
            {QStringLiteral("pattern"), pattern}, {QStringLiteral("maxLength"), maximum}};
    };
    const QJsonObject revision = string(QStringLiteral("^[0-9a-f]{64}$"), 64);
    const QJsonObject reference = string(QStringLiteral("^[0-9a-f]{64}:source:(0|[1-9][0-9]{0,9})$"), 128);
    const QJsonObject key = string(QStringLiteral("^[A-Z0-9]$"), 1);
    const QJsonObject rowReference = string(QStringLiteral("^[0-9a-f]{64}:row:(0|[1-9][0-9]{0,9}):(0|[1-9][0-9]{0,9})$"), 128);
    const QJsonObject identifierSchema = string(QStringLiteral("^[0-9a-f]{32}$"), 32);
    const auto schema = [](const QJsonObject &properties, const QJsonArray &required) {
        return QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("additionalProperties"), false}, {QStringLiteral("properties"), properties},
            {QStringLiteral("required"), required}};
    };
    auto preview = schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("mapping.add")}}},
        {QStringLiteral("source_revision"), revision}, {QStringLiteral("source_tab_ref"), reference},
        {QStringLiteral("original_key"), key}, {QStringLiteral("mapping_key"), key},
        {QStringLiteral("insert_row"), QJsonObject{{QStringLiteral("type"), QJsonArray{QStringLiteral("integer"), QStringLiteral("null")}},
            {QStringLiteral("minimum"), 0}, {QStringLiteral("maximum"), 2147483647}}}},
        {QStringLiteral("source_revision"), QStringLiteral("source_tab_ref"), QStringLiteral("original_key"), QStringLiteral("mapping_key")});
    const QJsonObject sendMethodSchema{{QStringLiteral("type"), QStringLiteral("integer")},
        {QStringLiteral("minimum"), 0}, {QStringLiteral("maximum"), 3}};
    auto addProperties = preview.value(QStringLiteral("properties")).toObject();
    addProperties.insert(QStringLiteral("send_method"), sendMethodSchema);
    preview.insert(QStringLiteral("properties"), addProperties);
    const QJsonObject apply = schema({{QStringLiteral("preview_id"), identifierSchema},
        {QStringLiteral("confirm_conflicts"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}, {QStringLiteral("default"), false}}}},
        {QStringLiteral("preview_id")});
    const auto capability = [](const char *id, const char *method, quint32 scopes, bool readOnly, const QJsonObject &arguments) {
        QJsonArray limits{QStringLiteral("memory_only_no_save_or_start"), QStringLiteral("bounded_volatile_results")};
        if (QLatin1String(method) == QStringLiteral("qkm_preview_changes") || QLatin1String(method) == QStringLiteral("qkm_apply_changes")) {
            for (const char *limit : {"current_attached_source_only", "single_uppercase_letter_or_digit", "idle_after_cleanup",
                "no_open_mapping_editor_session", "preview_is_not_write_permission"}) { limits.append(QString::fromLatin1(limit)); }
        }
        return QJsonObject{{QStringLiteral("id"), QString::fromLatin1(id)}, {QStringLiteral("method"), QString::fromLatin1(method)},
            {QStringLiteral("required_scopes"), int(scopes)}, {QStringLiteral("read_only"), readOnly},
            {QStringLiteral("arguments_schema"), arguments}, {QStringLiteral("limitations"), limits}};
    };
    QJsonObject previewCapability = capability("mapping.add.preview", "qkm_preview_changes", Read | Configure, true, preview);
    previewCapability.insert(QStringLiteral("preview_ttl_ms"), double(MappingChangeStore::PreviewTtlMs));
    previewCapability.insert(QStringLiteral("max_previews"), MappingChangeStore::MaxPreviews);
    previewCapability.insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    const QJsonObject saveSchema = schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("configuration.save")}}},
        {QStringLiteral("setting_name"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
            {QStringLiteral("minLength"), 1}, {QStringLiteral("maxLength"), 32767}}}},
        {QStringLiteral("action"), QStringLiteral("setting_name")});
    auto saveCapability = capability("configuration.save.preview", "qkm_preview_changes", Read | Configure, true, saveSchema);
    const QJsonArray saveLimits{QStringLiteral("explicit_full_scope_confirmation"), QStringLiteral("separate_overwrite_confirmation"),
        QStringLiteral("full_loaded_and_shared_save_scope"), QStringLiteral("explicit_setting_name"),
        QStringLiteral("idle_after_cleanup"), QStringLiteral("no_open_editors"), QStringLiteral("no_automatic_stop_or_start"),
        QStringLiteral("bounded_volatile_results")};
    saveCapability.insert(QStringLiteral("limitations"), saveLimits);
    saveCapability.insert(QStringLiteral("preview_ttl_ms"), double(MappingChangeStore::PreviewTtlMs));
    saveCapability.insert(QStringLiteral("max_previews"), MappingChangeStore::MaxPreviews);
    saveCapability.insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    const QJsonObject saveApply = schema({{QStringLiteral("preview_id"), identifierSchema},
        {QStringLiteral("confirm_scope"), QJsonObject{{QStringLiteral("const"), true}}},
        {QStringLiteral("confirm_overwrite"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}, {QStringLiteral("default"), false}}},
        {QStringLiteral("confirm_conflicts"), QJsonObject{{QStringLiteral("const"), false}}}},
        {QStringLiteral("preview_id"), QStringLiteral("confirm_scope")});
    auto saveApplyCapability = capability("configuration.save.apply", "qkm_apply_changes", Read | Configure, false, saveApply);
    saveApplyCapability.insert(QStringLiteral("limitations"), saveLimits);
    saveApplyCapability.insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    const QJsonObject deleteSchema = schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("mapping.delete")}}},
        {QStringLiteral("source_revision"), revision}, {QStringLiteral("source_tab_ref"), reference},
        {QStringLiteral("source_refs"), QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
            {QStringLiteral("minItems"), 1}, {QStringLiteral("maxItems"), 100}, {QStringLiteral("items"), rowReference}}}},
        {QStringLiteral("action"), QStringLiteral("source_revision"), QStringLiteral("source_tab_ref"), QStringLiteral("source_refs")});
    const QJsonArray deleteLimits{QStringLiteral("memory_only_no_save_or_start"), QStringLiteral("current_attached_visible_rows_only"),
        QStringLiteral("single_source_batch"), QStringLiteral("duplicate_references_deduplicated"), QStringLiteral("idle_after_cleanup"),
        QStringLiteral("no_open_mapping_editor_session"), QStringLiteral("explicit_delete_confirmation"), QStringLiteral("bounded_volatile_results")};
    auto deleteCapability = capability("mapping.delete.preview", "qkm_preview_changes", Read | Configure, true, deleteSchema);
    deleteCapability.insert(QStringLiteral("limitations"), deleteLimits);
    deleteCapability.insert(QStringLiteral("preview_ttl_ms"), double(MappingChangeStore::PreviewTtlMs));
    deleteCapability.insert(QStringLiteral("max_previews"), MappingChangeStore::MaxPreviews);
    deleteCapability.insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    const auto deleteApplySchema = schema({{QStringLiteral("preview_id"), identifierSchema}, {QStringLiteral("confirm_delete"), QJsonObject{{QStringLiteral("const"), true}}},
            {QStringLiteral("confirm_conflicts"), QJsonObject{{QStringLiteral("const"), false}}},
            {QStringLiteral("confirm_scope"), QJsonObject{{QStringLiteral("const"), false}}},
            {QStringLiteral("confirm_overwrite"), QJsonObject{{QStringLiteral("const"), false}}}},
        {QStringLiteral("preview_id"), QStringLiteral("confirm_delete")});
    auto deleteApply = capability("mapping.delete.apply", "qkm_apply_changes", Read | Configure, false, deleteApplySchema);
    deleteApply.insert(QStringLiteral("limitations"), deleteLimits);
    deleteApply.insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    deleteApply.insert(QStringLiteral("source_binding_result"), QStringLiteral("commit_snapshot_or_null"));
    auto addApply = capability("mapping.add.apply", "qkm_apply_changes", Read | Configure, false, apply);
    addApply.insert(QStringLiteral("source_binding_result"), QStringLiteral("commit_snapshot_or_null"));
    QJsonObject operation = capability("operation.read", "qkm_get_operation", Read, true,
        schema({{QStringLiteral("operation_id"), identifierSchema}}, {QStringLiteral("operation_id")}));
    operation.insert(QStringLiteral("result_ttl_ms"), double(MappingChangeStore::ResultTtlMs));
    operation.insert(QStringLiteral("max_results"), MappingChangeStore::MaxResults);
    const QJsonObject toggle{{QStringLiteral("type"), QStringLiteral("boolean")}};
    const QJsonObject burstTiming{{QStringLiteral("type"), QStringLiteral("integer")},
        {QStringLiteral("minimum"), QKeyMapperConstants::BURST_TIME_MIN}, {QStringLiteral("maximum"), QKeyMapperConstants::BURST_TIME_MAX}};
    auto triggerSchema = schema({{QStringLiteral("type"), QJsonObject{{QStringLiteral("enum"),
            QJsonArray{QStringLiteral("normal"), QStringLiteral("long_press"), QStringLiteral("double_press")}}}},
        {QStringLiteral("duration_ms"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
            {QStringLiteral("minimum"), QKeyMapperConstants::PRESSTIME_MIN}, {QStringLiteral("maximum"), QKeyMapperConstants::PRESSTIME_MAX}}}},
        {QStringLiteral("type")});
    triggerSchema.insert(QStringLiteral("if"), QJsonObject{{QStringLiteral("properties"),
        QJsonObject{{QStringLiteral("type"), QJsonObject{{QStringLiteral("const"), QStringLiteral("normal")}}}}}});
    triggerSchema.insert(QStringLiteral("then"), QJsonObject{{QStringLiteral("not"),
        QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("duration_ms")}}}}});
    auto rowModesSchema = schema({{QStringLiteral("burst"), toggle}, {QStringLiteral("lock"), toggle},
        {QStringLiteral("burst_press_ms"), burstTiming}, {QStringLiteral("burst_release_ms"), burstTiming},
        {QStringLiteral("mapping_key_unlock"), toggle}, {QStringLiteral("disable_original_key_unlock"), toggle},
        {QStringLiteral("sequence_hold_down"), toggle}, {QStringLiteral("pass_through"), toggle},
        {QStringLiteral("check_combo_order"), toggle}, {QStringLiteral("unbreakable"), toggle},
        {QStringLiteral("disable_fn_key_switch"), toggle}, {QStringLiteral("original_trigger"), triggerSchema},
        {QStringLiteral("fixed_vkey_code"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
            {QStringLiteral("minimum"), QKeyMapperConstants::FIXED_VIRTUAL_KEY_CODE_MIN}, {QStringLiteral("maximum"), QKeyMapperConstants::FIXED_VIRTUAL_KEY_CODE_MAX}}},
        {QStringLiteral("paste_text_mode"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
            {QStringLiteral("minimum"), QKeyMapperConstants::PASTETEXT_MODE_SHIFTINSERT}, {QStringLiteral("maximum"), QKeyMapperConstants::PASTETEXT_MODE_CTRLV}}},
        {QStringLiteral("repeat_mode"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
            {QStringLiteral("minimum"), QKeyMapperConstants::REPEAT_MODE_NONE}, {QStringLiteral("maximum"), QKeyMapperConstants::REPEAT_MODE_BYTIMES}}},
        {QStringLiteral("repeat_times"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
            {QStringLiteral("minimum"), QKeyMapperConstants::REPEAT_TIMES_MIN}, {QStringLiteral("maximum"), QKeyMapperConstants::REPEAT_TIMES_MAX}}}}, {});
    rowModesSchema.insert(QStringLiteral("minProperties"), 1);
    rowModesSchema.insert(QStringLiteral("dependentSchemas"), QJsonObject{
        {QStringLiteral("original_trigger"), QJsonObject{{QStringLiteral("maxProperties"), 1}}}});
    auto updateSchema = schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("mapping.update")}}},
        {QStringLiteral("row_modes"), rowModesSchema},
        {QStringLiteral("send_timing"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
            {QStringLiteral("minimum"), QKeyMapperConstants::SENDTIMING_NORMAL},
            {QStringLiteral("maximum"), QKeyMapperConstants::SENDTIMING_NORMAL_AND_KEYUP}}},
        {QStringLiteral("source_ref"), rowReference}, {QStringLiteral("send_method"), sendMethodSchema},
        {QStringLiteral("source_refs"), QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
            {QStringLiteral("minItems"), 2}, {QStringLiteral("maxItems"), 100}, {QStringLiteral("uniqueItems"), true},
            {QStringLiteral("items"), rowReference}}},
        {QStringLiteral("disabled"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}},
        {QStringLiteral("note"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}, {QStringLiteral("maxLength"), 32767}}},
        {QStringLiteral("category"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}, {QStringLiteral("maxLength"), 32767}}},
        {QStringLiteral("original_key"), key}, {QStringLiteral("mapping_key"), key},
        {QStringLiteral("key_up_mapping_key"), QJsonObject{{QStringLiteral("anyOf"), QJsonArray{key,
            QJsonObject{{QStringLiteral("const"), QString()}}}}}}}, {QStringLiteral("action")});
    updateSchema.insert(QStringLiteral("oneOf"), QJsonArray{
        QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("source_ref")}}},
        QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("source_refs")}}}});
    QJsonArray updateFields;
    for (const auto *name : {"send_method", "original_key", "mapping_key", "key_up_mapping_key"})
        updateFields.append(QJsonObject{{QStringLiteral("required"), QJsonArray{QString::fromLatin1(name)}}});
    const QJsonObject disabledRequired{{QStringLiteral("required"), QJsonArray{QStringLiteral("disabled")}}};
    QJsonArray noteConflicts = updateFields;
    noteConflicts.append(disabledRequired);
    const QJsonObject noteRequired{{QStringLiteral("required"), QJsonArray{QStringLiteral("note")}}};
    const QJsonObject categoryRequired{{QStringLiteral("required"), QJsonArray{QStringLiteral("category")}}};
    QJsonArray categoryConflicts = noteConflicts;
    categoryConflicts.append(noteRequired);
    QJsonArray rowModeConflicts = categoryConflicts;
    rowModeConflicts.append(categoryRequired);
    const QJsonObject rowModesRequired{{QStringLiteral("required"), QJsonArray{QStringLiteral("row_modes")}}};
    const QJsonObject timingRequired{{QStringLiteral("required"), QJsonArray{QStringLiteral("send_timing")}}};
    QJsonArray timingConflicts = rowModeConflicts;
    timingConflicts.append(QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("source_refs")}}});
    timingConflicts.append(rowModesRequired);
    const QJsonObject noDuration{{QStringLiteral("not"),
        QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("duration_ms")}}}}};
    const QJsonObject batchTriggerMode{{QStringLiteral("required"), QJsonArray{QStringLiteral("original_trigger")}},
        {QStringLiteral("properties"), QJsonObject{{QStringLiteral("original_trigger"), noDuration}}}};
    const QJsonObject batchTriggerSchema{{QStringLiteral("required"), QJsonArray{QStringLiteral("row_modes")}},
        {QStringLiteral("properties"), QJsonObject{{QStringLiteral("row_modes"), batchTriggerMode}}}};
    updateSchema.insert(QStringLiteral("dependentSchemas"), QJsonObject{
        {QStringLiteral("disabled"), QJsonObject{{QStringLiteral("not"), QJsonObject{{QStringLiteral("anyOf"), updateFields}}}}},
        {QStringLiteral("note"), QJsonObject{{QStringLiteral("not"), QJsonObject{{QStringLiteral("anyOf"), noteConflicts}}}}},
        {QStringLiteral("category"), QJsonObject{{QStringLiteral("not"), QJsonObject{{QStringLiteral("anyOf"), categoryConflicts}}}}},
        {QStringLiteral("source_refs"), QJsonObject{{QStringLiteral("oneOf"), QJsonArray{categoryRequired, batchTriggerSchema}}}},
        {QStringLiteral("row_modes"), QJsonObject{{QStringLiteral("not"), QJsonObject{{QStringLiteral("anyOf"), rowModeConflicts}}}}},
        {QStringLiteral("send_timing"), QJsonObject{{QStringLiteral("not"), QJsonObject{{QStringLiteral("anyOf"), timingConflicts}}}}}});
    updateFields.append(disabledRequired);
    updateFields.append(noteRequired);
    updateFields.append(categoryRequired);
    updateFields.append(rowModesRequired);
    updateFields.append(timingRequired);
    updateSchema.insert(QStringLiteral("anyOf"), updateFields);
    auto updatePreview = capability("mapping.update.preview", "qkm_preview_changes", Read | Configure, true, updateSchema);
    updatePreview.insert(QStringLiteral("key_up_send_timings"), QJsonArray{2, 3, 4});
    // Keep the legacy flag for general conflict resolution.
    updatePreview.insert(QStringLiteral("original_key_conflicts_supported"), false);
    QJsonObject preferencesSchema = schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("ibinput.preferences.update")}}},
        {QStringLiteral("preferences_revision"), revision},
        {QStringLiteral("preferred_backend"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
            {QStringLiteral("enum"), QJsonArray{QStringLiteral("auto"), QStringLiteral("logitech_ghub"), QStringLiteral("logitech_legacy"), QStringLiteral("razer")}}}},
        {QStringLiteral("unavailable_policy"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
            {QStringLiteral("enum"), QJsonArray{QStringLiteral("block"), QStringLiteral("sendinput_before_start")}}}}},
        {QStringLiteral("action"), QStringLiteral("preferences_revision")});
    preferencesSchema.insert(QStringLiteral("anyOf"), QJsonArray{
        QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("preferred_backend")}}},
        QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("unavailable_policy")}}}});
    auto preferencesPreview = capability("ibinput.preferences.update.preview", "qkm_preview_changes", Read | Configure, true, preferencesSchema);
    const auto settingsApplySchema = schema({{QStringLiteral("preview_id"), identifierSchema},
        {QStringLiteral("confirm_conflicts"), QJsonObject{{QStringLiteral("const"), false}}},
        {QStringLiteral("confirm_scope"), QJsonObject{{QStringLiteral("const"), false}}},
        {QStringLiteral("confirm_overwrite"), QJsonObject{{QStringLiteral("const"), false}}},
        {QStringLiteral("confirm_delete"), QJsonObject{{QStringLiteral("const"), false}}}}, {QStringLiteral("preview_id")});
    auto updateApplySchema = settingsApplySchema;
    auto updateConfirmations = updateApplySchema.value(QStringLiteral("properties")).toObject();
    updateConfirmations.insert(QStringLiteral("confirm_conflicts"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}});
    updateConfirmations.insert(QStringLiteral("confirm_scope"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}});
    updateApplySchema.insert(QStringLiteral("properties"), updateConfirmations);
    auto updateApply = capability("mapping.update.apply", "qkm_apply_changes", Read | Configure, false, updateApplySchema);
    auto preferencesApply = capability("ibinput.preferences.update.apply", "qkm_apply_changes", Read | Configure, false, settingsApplySchema);
    for (auto *item : {&updatePreview, &updateApply, &preferencesPreview, &preferencesApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("memory_only_no_save_or_start"),
            QStringLiteral("idle_after_completed_cleanup"), QStringLiteral("semantic_preferences_revision_bound"),
            QStringLiteral("no_open_editor_or_popup"), QStringLiteral("declared_fields_only")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    for (auto *item : {&updatePreview, &updateApply}) {
        item->insert(QStringLiteral("original_key_conflict_policy"), QStringLiteral("disable_current_row_or_confirm_common_changes"));
        auto limits = item->value(QStringLiteral("limitations")).toArray();
        limits.append(QStringLiteral("key_updates_single_keyboard_row_only"));
        limits.append(QStringLiteral("common_update_requires_confirmation_to_disable_normal_rows"));
        limits.append(QStringLiteral("key_up_edit_requires_send_timing_2_3_4"));
        limits.append(QStringLiteral("disabled_updates_are_separate_from_key_and_method_updates"));
        limits.append(QStringLiteral("disabled_updates_require_no_exclusive_side_effects"));
        limits.append(QStringLiteral("hidden_rows_require_explicit_gui_reveal"));
        item->insert(QStringLiteral("limitations"), limits);
    }
    const QJsonObject copyPositions{{QStringLiteral("enum"), QJsonArray{QStringLiteral("top"), QStringLiteral("bottom"),
        QStringLiteral("above"), QStringLiteral("below")}}};
    const QJsonObject relativePositions{{QStringLiteral("enum"), QJsonArray{QStringLiteral("above"), QStringLiteral("below")}}};
    const QJsonObject anchorRequired{{QStringLiteral("required"), QJsonArray{QStringLiteral("anchor_ref")}}};
    auto copyDestination = schema({{QStringLiteral("position"), copyPositions}, {QStringLiteral("anchor_ref"), rowReference},
        {QStringLiteral("target_tab_ref"), reference}}, {QStringLiteral("position")});
    copyDestination.insert(QStringLiteral("if"), QJsonObject{{QStringLiteral("properties"),
        QJsonObject{{QStringLiteral("position"), relativePositions}}}});
    copyDestination.insert(QStringLiteral("then"), anchorRequired);
    copyDestination.insert(QStringLiteral("else"), QJsonObject{{QStringLiteral("not"), anchorRequired}});
    auto duplicateSchema = schema({
        {QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("mapping.duplicate")}}},
        {QStringLiteral("source_ref"), rowReference}, {QStringLiteral("copy_to"), copyDestination},
        {QStringLiteral("source_refs"), QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
            {QStringLiteral("minItems"), 2}, {QStringLiteral("maxItems"), 100}, {QStringLiteral("uniqueItems"), true},
            {QStringLiteral("items"), rowReference}}}}, {QStringLiteral("action"), QStringLiteral("copy_to")});
    duplicateSchema.insert(QStringLiteral("oneOf"), QJsonArray{
        QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("source_ref")}}},
        QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("source_refs")}}}});
    auto duplicatePreview = capability("mapping.duplicate.preview", "qkm_preview_changes", Read | Configure, true, duplicateSchema);
    auto duplicateApplySchema = settingsApplySchema;
    auto duplicateConfirmations = duplicateApplySchema.value(QStringLiteral("properties")).toObject();
    duplicateConfirmations.insert(QStringLiteral("confirm_conflicts"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}});
    duplicateApplySchema.insert(QStringLiteral("properties"), duplicateConfirmations);
    auto duplicateApply = capability("mapping.duplicate.apply", "qkm_apply_changes", Read | Configure, false, duplicateApplySchema);
    for (auto *item : {&duplicatePreview, &duplicateApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("current_attached_target_tab"),
            QStringLiteral("common_target_requires_confirmation_to_disable_normal_rows"),
            QStringLiteral("one_visible_row_or_one_contiguous_visible_selection_and_same_revision_anchor"), QStringLiteral("idle_after_completed_cleanup"),
            QStringLiteral("memory_only_no_save_or_start")});
    }
    duplicateApply.insert(QStringLiteral("source_binding_result"), QStringLiteral("commit_snapshot_or_null"));
    QJsonArray capabilities{previewCapability, saveCapability, addApply,
        saveApplyCapability, deleteCapability, deleteApply, updatePreview, updateApply, preferencesPreview, preferencesApply,
        capability("mapping.preview.cancel", "qkm_cancel_changes", Read | Configure, false,
            schema({{QStringLiteral("preview_id"), identifierSchema}}, {QStringLiteral("preview_id")}))};
    if (allowRunControls) {
        auto stop = capability("mapping.stop", "qkm_stop_mapping", Read | RunMapping, false,
            schema({{QStringLiteral("instance"), identifierSchema}, {QStringLiteral("request_id"), identifierSchema}},
                {QStringLiteral("instance"), QStringLiteral("request_id")}));
        stop.insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("monitoring_with_stable_sources_or_verified_mcp_started_match"),
            QStringLiteral("no_configured_stop_actions"), QStringLiteral("existing_cleanup_is_observable"),
            QStringLiteral("result_is_bounded_and_instance_local")});
        capabilities.append(stop);
        auto start = capability("mapping.start", "qkm_start_mapping", Read | RunMapping, false,
            schema({{QStringLiteral("instance"), identifierSchema}, {QStringLiteral("request_id"), identifierSchema},
                {QStringLiteral("source_revision"), revision}, {QStringLiteral("source_tab_ref"), reference},
                {QStringLiteral("save_policy"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                    {QStringLiteral("enum"), QJsonArray{QStringLiteral("start_only")}}}}},
                {QStringLiteral("instance"), QStringLiteral("request_id"), QStringLiteral("source_revision"),
                    QStringLiteral("source_tab_ref"), QStringLiteral("save_policy")}));
        start.insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("current_normal_source"),
            QStringLiteral("simple_keyboard_rows_only"), QStringLiteral("automatic_profile_switching_disabled"),
            QStringLiteral("monitoring_acceptance_not_mapping_active")});
        capabilities.append(start);
    }
    for (const auto move : RowMoves) {
        const QByteArray action = mappingRowMoveAction(move);
        auto moveSchema = schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QString::fromLatin1(action)}}},
            {QStringLiteral("source_ref"), rowReference},
            {QStringLiteral("source_refs"), QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
                {QStringLiteral("minItems"), 2}, {QStringLiteral("maxItems"), 100}, {QStringLiteral("items"), rowReference}}}},
            {QStringLiteral("action")});
        moveSchema.insert(QStringLiteral("oneOf"), QJsonArray{
            QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("source_ref")}}},
            QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("source_refs")}}}});
        auto movePreview = capability((action + ".preview").constData(), "qkm_preview_changes", Read | Configure, true, moveSchema);
        auto moveApply = capability((action + ".apply").constData(), "qkm_apply_changes", Read | Configure, false, settingsApplySchema);
        const auto allowedMove = move == MappingRowMove::Up ? QStringLiteral("one_or_contiguous_rows_one_position_up")
            : move == MappingRowMove::Down ? QStringLiteral("one_or_contiguous_rows_one_position_down")
            : QStringLiteral("one_or_contiguous_rows_to_table_edge");
        for (auto *item : {&movePreview, &moveApply}) {
            item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("memory_only_no_save_or_start"),
                allowedMove, QStringLiteral("all_displaced_rows_in_preview"),
                QStringLiteral("current_unfiltered_normal_source_only"), QStringLiteral("idle_after_completed_cleanup"),
                QStringLiteral("no_open_editor_or_popup")});
            item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
        }
        movePreview.insert(QStringLiteral("preview_ttl_ms"), double(MappingChangeStore::PreviewTtlMs));
        moveApply.insert(QStringLiteral("source_binding_result"), QStringLiteral("commit_snapshot_or_null"));
        capabilities.append(movePreview);
        capabilities.append(moveApply);
    }
    const auto renameSchema = schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("tab.rename")}}},
        {QStringLiteral("source_tab_ref"), reference},
        {QStringLiteral("tab_name"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}, {QStringLiteral("maxLength"), 32767}}}},
        {QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("tab_name")});
    auto renamePreview = capability("tab.rename.preview", "qkm_preview_changes", Read | Configure, true, renameSchema);
    auto renameApply = capability("tab.rename.apply", "qkm_apply_changes", Read | Configure, false, settingsApplySchema);
    for (auto *item : {&renamePreview, &renameApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("current_attached_normal_source_only"),
            QStringLiteral("memory_only_no_save_or_start"), QStringLiteral("no_open_editor_or_popup"),
            QStringLiteral("idle_after_completed_cleanup"), QStringLiteral("gui_tab_name_rules")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    renamePreview.insert(QStringLiteral("preview_ttl_ms"), double(MappingChangeStore::PreviewTtlMs));
    renameApply.insert(QStringLiteral("source_binding_result"), QStringLiteral("commit_snapshot_or_null"));
    capabilities.append(renamePreview);
    capabilities.append(renameApply);
    for (const char *action : {"tab.add_blank", "tab.copy", "tab.delete", "tab.clear", "tab.move",
        "tab.set_common_inheritance", "tab.set_common_collapsed", "configuration.set_common_enabled", "tab.select", "tab.set_hotkey", "tab.set_notification_mode", "tab.set_notification_colors",
        "tab.set_category_filter", "configuration.set_show_disabled_rows", "configuration.set_mapping_display"}) {
        const QByteArray actionId(action);
        const bool deleting = actionId == "tab.delete";
        const bool clearing = actionId == "tab.clear";
        const bool removesData = deleting || clearing;
        const bool moving = actionId == "tab.move";
        const bool selecting = actionId == "tab.select";
        const bool settingHotkey = actionId == "tab.set_hotkey";
        const bool settingNotification = actionId == "tab.set_notification_mode";
        const bool settingNotificationColors = actionId == "tab.set_notification_colors";
        const bool categoryFilter = actionId == "tab.set_category_filter";
        const bool disabledVisibility = actionId == "configuration.set_show_disabled_rows";
        const bool mappingDisplay = actionId == "configuration.set_mapping_display";
        const bool includingCommon = actionId == "tab.set_common_inheritance";
        const bool collapsingCommon = actionId == "tab.set_common_collapsed";
        const bool commonFeature = actionId == "configuration.set_common_enabled";
        QJsonObject tabProperties{{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QString::fromLatin1(action)}}},
            {QStringLiteral("source_tab_ref"), reference}};
        QJsonArray tabRequired{QStringLiteral("action"), QStringLiteral("source_tab_ref")};
        if (moving || selecting) {
            tabProperties.insert(QStringLiteral("target_tab_ref"), reference);
            tabRequired.append(QStringLiteral("target_tab_ref"));
        }
        if (includingCommon || collapsingCommon || commonFeature) {
            const auto key = includingCommon ? QStringLiteral("include_common")
                : collapsingCommon ? QStringLiteral("common_collapsed") : QStringLiteral("common_feature_enabled");
            tabProperties.insert(key, QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}});
            tabRequired.append(key);
        }
        if (settingHotkey) {
            tabProperties.insert(QStringLiteral("tab_hotkey"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                {QStringLiteral("maxLength"), 32767}});
            tabRequired.append(QStringLiteral("tab_hotkey"));
        }
        if (settingNotification) {
            tabProperties.insert(QStringLiteral("notification_mode"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
                {QStringLiteral("enum"), QJsonArray{0, 1, 2}}});
            tabRequired.append(QStringLiteral("notification_mode"));
        }
        if (mappingDisplay) {
            const auto field = QStringLiteral("mapping_display");
            const QJsonObject toggle{{QStringLiteral("type"), QStringLiteral("boolean")}};
            auto displaySchema = schema({{QStringLiteral("category_column"), toggle}, {QStringLiteral("notes"), toggle},
                {QStringLiteral("floating_column"), toggle}}, {});
            displaySchema.insert(QStringLiteral("minProperties"), 1);
            tabProperties.insert(field, displaySchema);
            tabRequired.append(field);
        }
        if (disabledVisibility) {
            const auto field = QStringLiteral("show_disabled_rows");
            tabProperties.insert(field, QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}});
            tabRequired.append(field);
        }
        if (categoryFilter) {
            const auto field = QStringLiteral("category_filter");
            tabProperties.insert(field, QJsonObject{{QStringLiteral("oneOf"), QJsonArray{
                QJsonObject{{QStringLiteral("const"), QStringLiteral("all")}},
                QJsonObject{{QStringLiteral("type"), QStringLiteral("array")}, {QStringLiteral("maxItems"), 100},
                    {QStringLiteral("uniqueItems"), true}, {QStringLiteral("items"), QJsonObject{
                        {QStringLiteral("type"), QStringLiteral("string")}, {QStringLiteral("maxLength"), 32767}}}}}}});
            tabRequired.append(field);
        }
        const QString fontField = QStringLiteral("notification_font_color");
        const QString backgroundField = QStringLiteral("notification_background_color");
        if (settingNotificationColors) {
            tabProperties.insert(fontField, QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                {QStringLiteral("pattern"), QStringLiteral("^(default|#[0-9a-fA-F]{6})$")}});
            tabProperties.insert(backgroundField, QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                {QStringLiteral("pattern"), QStringLiteral("^(default|#[0-9a-fA-F]{8})$")}});
        }
        auto tabSchema = schema(tabProperties, tabRequired);
        if (settingNotificationColors) {
            tabSchema.insert(QStringLiteral("anyOf"), QJsonArray{
                QJsonObject{{QStringLiteral("required"), QJsonArray{fontField}}},
                QJsonObject{{QStringLiteral("required"), QJsonArray{backgroundField}}}});
        }
        auto tabPreview = capability((actionId + ".preview").constData(), "qkm_preview_changes", Read | Configure, true, tabSchema);
        auto tabApply = capability((actionId + ".apply").constData(), "qkm_apply_changes", Read | Configure, false,
            removesData ? deleteApplySchema : commonFeature ? updateApplySchema : settingsApplySchema);
        const auto behavior = removesData ? QStringLiteral("explicit_delete_confirmation")
            : moving ? QStringLiteral("same_revision_normal_destination")
            : selecting ? QStringLiteral("same_revision_attached_destination_selection_only")
            : settingHotkey ? QStringLiteral("gui_hotkey_validation_and_shared_targets")
            : settingNotification ? QStringLiteral("tab_start_and_switch_notifications")
            : mappingDisplay || disabledVisibility ? QStringLiteral("all_mapping_tables_view_preference")
            : categoryFilter ? QStringLiteral("visible_category_column_view_only")
            : settingNotificationColors ? QStringLiteral("notification_colors_with_inherited_defaults")
            : includingCommon ? QStringLiteral("common_feature_must_be_enabled")
            : collapsingCommon ? QStringLiteral("inherited_common_separator_required")
            : commonFeature ? QStringLiteral("explicit_conflict_confirmation_if_needed")
            : actionId == "tab.copy" ? QStringLiteral("gui_copy_preserves_current_tab") : QStringLiteral("gui_default_name_and_activation");
        for (auto *item : {&tabPreview, &tabApply}) {
            QJsonArray limits{clearing || commonFeature || selecting || categoryFilter || disabledVisibility || mappingDisplay
                    ? QStringLiteral("current_attached_source_only") : QStringLiteral("current_attached_normal_source_only"),
                QStringLiteral("memory_only_no_save_or_start"), behavior,
                QStringLiteral("idle_after_completed_cleanup"), QStringLiteral("no_open_editor_or_popup")};
            if (deleting) limits.append(QStringLiteral("last_normal_tab_protected"));
            if (clearing) limits.append(QStringLiteral("all_local_rows_including_hidden"));
            item->insert(QStringLiteral("limitations"), limits);
            item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
        }
        tabPreview.insert(QStringLiteral("preview_ttl_ms"), double(MappingChangeStore::PreviewTtlMs));
        tabApply.insert(QStringLiteral("source_binding_result"), QStringLiteral("commit_snapshot_or_null"));
        capabilities.append(tabPreview);
        capabilities.append(tabApply);
    }
    capabilities.append(duplicatePreview);
    capabilities.append(duplicateApply);
    const QJsonObject macroText{{QStringLiteral("type"), QJsonArray{QStringLiteral("string"), QStringLiteral("null")}},
        {QStringLiteral("maxLength"), 32767}};
    const auto macroPlacement = schema({{QStringLiteral("position"), QJsonObject{{QStringLiteral("enum"),
        QJsonArray{QStringLiteral("above"), QStringLiteral("below")}}}},
        {QStringLiteral("anchor_name"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
            {QStringLiteral("minLength"), 1}, {QStringLiteral("maxLength"), 32767}}}},
        {QStringLiteral("position"), QStringLiteral("anchor_name")});
    const auto macroFields = schema({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("scope"), QJsonObject{{QStringLiteral("enum"), QJsonArray{QStringLiteral("configuration"), QStringLiteral("universal")}}}},
        {QStringLiteral("name"), string(QStringLiteral("^[\\s\\S]+$"), 32767)},
        {QStringLiteral("content"), string(QStringLiteral("^[\\s\\S]+$"), QKeyMapperConstants::MAPPINGKEY_LINE_EDIT_MAX_LENGTH)},
        {QStringLiteral("category"), macroText}, {QStringLiteral("note"), macroText},
        {QStringLiteral("insert_at"), QJsonObject{{QStringLiteral("anyOf"), QJsonArray{macroPlacement,
            QJsonObject{{QStringLiteral("type"), QStringLiteral("null")}}}}}},
        {QStringLiteral("segment_comments"), QJsonObject{{QStringLiteral("type"), QJsonArray{QStringLiteral("array"), QStringLiteral("null")}},
            {QStringLiteral("minItems"), 1}, {QStringLiteral("items"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                {QStringLiteral("maxLength"), 32767}}}}}},
        {QStringLiteral("catalog_revision"), QStringLiteral("scope"), QStringLiteral("name"), QStringLiteral("content")});
    auto macroPreview = capability("macro.upsert.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("macro.upsert")}}},
            {QStringLiteral("macro"), macroFields}}, {QStringLiteral("action"), QStringLiteral("macro")}));
    auto macroApplySchema = settingsApplySchema;
    auto macroConfirmation = macroApplySchema.value(QStringLiteral("properties")).toObject();
    macroConfirmation.insert(QStringLiteral("confirm_overwrite"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}});
    macroApplySchema.insert(QStringLiteral("properties"), macroConfirmation);
    auto macroApply = capability("macro.upsert.apply", "qkm_apply_changes", Read | Configure, false, macroApplySchema);
    for (auto *item : {&macroPreview, &macroApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("current_loaded_macro_catalog"),
            QStringLiteral("insert_at_named_anchor_or_append_replace_in_place"), QStringLiteral("confirm_overwrite_for_existing_name"),
            QStringLiteral("basic_letter_or_digit_sequence_only"), QStringLiteral("one_explicit_comment_per_key_step"),
            QStringLiteral("idle_after_completed_cleanup"), QStringLiteral("memory_only_no_save_or_start")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    capabilities.append(macroPreview);
    capabilities.append(macroApply);
    const auto deleteMacroFields = schema({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("scope"), macroFields.value(QStringLiteral("properties")).toObject().value(QStringLiteral("scope"))},
        {QStringLiteral("names"), QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
            {QStringLiteral("minItems"), 1}, {QStringLiteral("maxItems"), 100}, {QStringLiteral("uniqueItems"), true},
            {QStringLiteral("items"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                {QStringLiteral("minLength"), 1}, {QStringLiteral("maxLength"), 32767}}}}}},
        {QStringLiteral("catalog_revision"), QStringLiteral("scope"), QStringLiteral("names")});
    auto macroDeletePreview = capability("macro.delete.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("macro.delete")}}},
            {QStringLiteral("macro"), deleteMacroFields}}, {QStringLiteral("action"), QStringLiteral("macro")}));
    auto macroDeleteApply = capability("macro.delete.apply", "qkm_apply_changes", Read | Configure, false, deleteApplySchema);
    for (auto *item : {&macroDeletePreview, &macroDeleteApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("current_loaded_macro_catalog"),
            QStringLiteral("one_scope_exact_distinct_names"), QStringLiteral("existing_references_preserved"),
            QStringLiteral("idle_after_completed_cleanup"), QStringLiteral("memory_only_no_save_or_start")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    capabilities.append(macroDeletePreview);
    capabilities.append(macroDeleteApply);
    const auto importMacroFields = schema({{QStringLiteral("catalog_revision"), revision},
        {QStringLiteral("scope"), macroFields.value(QStringLiteral("properties")).toObject().value(QStringLiteral("scope"))},
        {QStringLiteral("file_name"), string(QStringLiteral("^[^<>:\"/\\\\|?*]+\\.[iI][nN][iI]$"), 255)},
        {QStringLiteral("on_conflict"), QJsonObject{{QStringLiteral("enum"),
            QJsonArray{QStringLiteral("keep"), QStringLiteral("overwrite")}}}}},
        {QStringLiteral("catalog_revision"), QStringLiteral("scope"), QStringLiteral("file_name"), QStringLiteral("on_conflict")});
    auto macroImportPreview = capability("macro.import.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("macro.import")}}},
            {QStringLiteral("macro"), importMacroFields}}, {QStringLiteral("action"), QStringLiteral("macro")}));
    auto macroImportApply = capability("macro.import.apply", "qkm_apply_changes", Read | Configure, false, macroApplySchema);
    for (auto *item : {&macroImportPreview, &macroImportApply}) {
        item->insert(QStringLiteral("max_macros"), MaxMacroBackupEntries);
        item->insert(QStringLiteral("max_document_bytes"), MaxMacroBackupBytes);
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("sidecar_local_backup_directory"),
            QStringLiteral("current_loaded_macro_catalog"), QStringLiteral("one_scope_per_import"),
            QStringLiteral("basic_letter_or_digit_sequence_only"), QStringLiteral("confirm_overwrite_for_existing_name"),
            QStringLiteral("idle_after_completed_cleanup"), QStringLiteral("memory_only_no_save_or_start")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    capabilities.append(macroImportPreview);
    capabilities.append(macroImportApply);
    auto insertModePreview = capability("general.set_table_insert_mode.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("general.set_table_insert_mode")}}},
            {QStringLiteral("source_tab_ref"), reference},
            {QStringLiteral("table_insert_mode"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
                {QStringLiteral("enum"), QJsonArray{0, 1}}}}},
            {QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("table_insert_mode")}));
    auto insertModeApply = capability("general.set_table_insert_mode.apply", "qkm_apply_changes", Read | Configure, false, settingsApplySchema);
    for (auto *item : {&insertModePreview, &insertModeApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("application_preferences"),
            QStringLiteral("current_source_guard"), QStringLiteral("gui_insertion_default"),
            QStringLiteral("idle_after_completed_cleanup"), QStringLiteral("memory_only_no_save_or_start")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    capabilities.append(insertModePreview);
    capabilities.append(insertModeApply);
    auto editTriggerPreview = capability("general.set_table_edit_trigger.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("general.set_table_edit_trigger")}}},
            {QStringLiteral("source_tab_ref"), reference},
            {QStringLiteral("table_edit_trigger"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
                {QStringLiteral("enum"), QJsonArray{0, 1}}}}},
            {QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("table_edit_trigger")}));
    auto editTriggerApply = capability("general.set_table_edit_trigger.apply", "qkm_apply_changes", Read | Configure, false, settingsApplySchema);
    for (auto *item : {&editTriggerPreview, &editTriggerApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("application_preferences"),
            QStringLiteral("current_source_guard"), QStringLiteral("gui_inline_edit_trigger"),
            QStringLiteral("idle_after_completed_cleanup"), QStringLiteral("memory_only_no_save_or_start")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    capabilities.append(editTriggerPreview);
    capabilities.append(editTriggerApply);
    auto windowDisplayModePreview = capability("configuration.set_window_display_mode.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("configuration.set_window_display_mode")}}},
            {QStringLiteral("source_tab_ref"), reference},
            {QStringLiteral("window_display_mode"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
                {QStringLiteral("enum"), QJsonArray{0, 1, 2}}}}},
            {QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("window_display_mode")}));
    auto windowDisplayModeApply = capability("configuration.set_window_display_mode.apply", "qkm_apply_changes", Read | Configure, false, settingsApplySchema);
    for (auto *item : {&windowDisplayModePreview, &windowDisplayModeApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("current_loaded_configuration"),
            QStringLiteral("requires_editable_display_condition"), QStringLiteral("idle_after_completed_cleanup"),
            QStringLiteral("memory_only_no_save_or_start")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    capabilities.append(windowDisplayModePreview);
    capabilities.append(windowDisplayModeApply);
    auto tableImportPreview = capability("table.import.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("table.import")}}},
            {QStringLiteral("source_tab_ref"), reference},
            {QStringLiteral("file_name"), string(QStringLiteral("^[^<>:\"/\\\\|?*]+\\.[iI][nN][iI]$"), 255)}},
            {QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("file_name")}));
    auto tableImportApply = capability("table.import.apply", "qkm_apply_changes", Read | Configure, false, updateApplySchema);
    for (auto *item : {&tableImportPreview, &tableImportApply}) {
        item->insert(QStringLiteral("max_rows"), MaxTableBackupRows);
        item->insert(QStringLiteral("max_document_bytes"), MaxTableBackupBytes);
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("sidecar_local_backup_directory"),
            QStringLiteral("attached_source_tab"), QStringLiteral("append_at_end"),
            QStringLiteral("confirm_disabling_existing_rows"), QStringLiteral("idle_after_completed_cleanup"),
            QStringLiteral("memory_only_no_save_or_start")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    capabilities.append(tableImportPreview);
    capabilities.append(tableImportApply);
    auto windowSizePreview = capability("general.set_save_window_size.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("general.set_save_window_size")}}},
            {QStringLiteral("source_tab_ref"), reference},
            {QStringLiteral("save_window_size"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}}},
            {QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("save_window_size")}));
    auto windowSizeApply = capability("general.set_save_window_size.apply", "qkm_apply_changes", Read | Configure, false, settingsApplySchema);
    for (auto *item : {&windowSizePreview, &windowSizeApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("application_preferences"),
            QStringLiteral("startup_window_size_restore_option"), QStringLiteral("idle_after_completed_cleanup"),
            QStringLiteral("memory_only_no_save_or_start")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    capabilities.append(windowSizePreview);
    capabilities.append(windowSizeApply);
    auto splitterPreview = capability("general.set_save_splitter_position.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("general.set_save_splitter_position")}}},
            {QStringLiteral("source_tab_ref"), reference},
            {QStringLiteral("save_splitter_position"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}}},
            {QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("save_splitter_position")}));
    auto splitterApply = capability("general.set_save_splitter_position.apply", "qkm_apply_changes", Read | Configure, false, settingsApplySchema);
    for (auto *item : {&splitterPreview, &splitterApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("application_preferences"),
            QStringLiteral("startup_splitter_position_restore_option"), QStringLiteral("idle_after_completed_cleanup"),
            QStringLiteral("memory_only_no_save_or_start")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    capabilities.append(splitterPreview);
    capabilities.append(splitterApply);
    const auto boundedInteger = [](int minimum, int maximum) {
        return QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
            {QStringLiteral("minimum"), minimum}, {QStringLiteral("maximum"), maximum}};
    };
    using namespace QKeyMapperConstants;
    const auto point = schema({{QStringLiteral("x"), boundedInteger(STARTUP_SPECIFY_POSITION_MIN_X, STARTUP_SPECIFY_POSITION_MAX_X)},
        {QStringLiteral("y"), boundedInteger(STARTUP_SPECIFY_POSITION_MIN_Y, STARTUP_SPECIFY_POSITION_MAX_Y)}},
        {QStringLiteral("x"), QStringLiteral("y")});
    auto startupPosition = schema({{QStringLiteral("mode"), boundedInteger(STARTUP_POSITION_MIN, STARTUP_POSITION_MAX)},
        {QStringLiteral("point"), point}}, {});
    startupPosition.insert(QStringLiteral("minProperties"), 1);
    startupPosition.insert(QStringLiteral("if"), QJsonObject{{QStringLiteral("required"),
        QJsonArray{QStringLiteral("mode"), QStringLiteral("point")}}});
    startupPosition.insert(QStringLiteral("then"), QJsonObject{{QStringLiteral("properties"),
        QJsonObject{{QStringLiteral("mode"), QJsonObject{{QStringLiteral("const"), STARTUP_POSITION_SPECIFY}}}}}});
    auto startupPreview = capability("general.set_startup_position.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("general.set_startup_position")}}},
            {QStringLiteral("source_tab_ref"), reference}, {QStringLiteral("startup_position"), startupPosition}},
            {QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("startup_position")}));
    auto startupApply = capability("general.set_startup_position.apply", "qkm_apply_changes", Read | Configure, false, settingsApplySchema);
    for (auto *item : {&startupPreview, &startupApply}) {
        item->insert(QStringLiteral("limitations"), QJsonArray{QStringLiteral("application_preferences"),
            QStringLiteral("point_editable_in_specified_mode"), QStringLiteral("idle_after_completed_cleanup"),
            QStringLiteral("memory_only_no_save_or_move")});
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    }
    capabilities.append(startupPreview);
    capabilities.append(startupApply);
    auto stage = capability("configuration.import.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("configuration.import")}}},
            {QStringLiteral("catalog_revision"), revision},
            {QStringLiteral("groups"), QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
                {QStringLiteral("minItems"), 1}, {QStringLiteral("maxItems"), MaxConfigurationBackupGroups},
                {QStringLiteral("uniqueItems"), true},
                {QStringLiteral("items"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                    {QStringLiteral("minLength"), 1}, {QStringLiteral("maxLength"), 32767}}}}},
            {QStringLiteral("file_name"), string(QStringLiteral("^[^<>:\"/\\\\|?*]+\\.[iI][nN][iI]$"), 255)},
            {QStringLiteral("on_conflict"), QJsonObject{{QStringLiteral("enum"),
                QJsonArray{QStringLiteral("keep"), QStringLiteral("overwrite")}}}}},
            {QStringLiteral("action"), QStringLiteral("catalog_revision"), QStringLiteral("groups"),
                QStringLiteral("file_name"), QStringLiteral("on_conflict")}));
    stage.insert(QStringLiteral("max_document_bytes"), MaxConfigurationBackupBytes);
    stage.insert(QStringLiteral("max_groups"), MaxConfigurationBackupGroups);
    const QJsonArray importLimits{QStringLiteral("ordinary_groups_no_external_assets"), QStringLiteral("idle_after_completed_cleanup"),
        QStringLiteral("preserve_loaded_configuration"), QStringLiteral("same_name_merge_only_supplied_fields"),
        QStringLiteral("require_explicit_save_of_owned_draft")};
    auto stageApply = capability("configuration.import.apply", "qkm_apply_changes", Read | Configure, false, macroApplySchema);
    auto importSave = capability("configuration.save_import.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("configuration.save_import")}}},
            {QStringLiteral("import_draft_id"), identifierSchema}}, {QStringLiteral("action"), QStringLiteral("import_draft_id")}));
    auto saveImportSchema = settingsApplySchema;
    auto saveImportProperties = saveImportSchema.value(QStringLiteral("properties")).toObject();
    saveImportProperties.insert(QStringLiteral("confirm_scope"), QJsonObject{{QStringLiteral("const"), true}});
    saveImportSchema.insert(QStringLiteral("properties"), saveImportProperties);
    saveImportSchema.insert(QStringLiteral("required"), QJsonArray{QStringLiteral("preview_id"), QStringLiteral("confirm_scope")});
    auto importSaveApply = capability("configuration.save_import.apply", "qkm_apply_changes", Read | Configure, false, saveImportSchema);
    for (auto *item : {&stage, &stageApply, &importSave, &importSaveApply}) {
        item->insert(QStringLiteral("limitations"), importLimits);
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
        capabilities.append(*item);
    }
    const auto groupName = string(QStringLiteral("^[^\\r\\n]+$"), 32767);
    auto moveOrder = capability("configuration.move.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("configuration.move")}}},
            {QStringLiteral("catalog_revision"), revision}, {QStringLiteral("group"), groupName},
            {QStringLiteral("target_group"), groupName}},
            {QStringLiteral("action"), QStringLiteral("catalog_revision"), QStringLiteral("group"), QStringLiteral("target_group")}));
    auto resetOrder = capability("configuration.reset_order.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("configuration.reset_order")}}},
            {QStringLiteral("catalog_revision"), revision}}, {QStringLiteral("action"), QStringLiteral("catalog_revision")}));
    auto moveOrderApply = capability("configuration.move.apply", "qkm_apply_changes", Read | Configure, false, saveImportSchema);
    auto resetOrderApply = capability("configuration.reset_order.apply", "qkm_apply_changes", Read | Configure, false, saveImportSchema);
    const QJsonArray orderLimits{QStringLiteral("idle_after_completed_cleanup"), QStringLiteral("ordinary_groups_only"),
        QStringLiteral("write_only_configuration_order"), QStringLiteral("preserve_selected_configuration_and_drafts")};
    for (auto *item : {&moveOrder, &moveOrderApply, &resetOrder, &resetOrderApply}) {
        item->insert(QStringLiteral("limitations"), orderLimits);
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
        capabilities.append(*item);
    }
    auto removeConfiguration = capability("configuration.remove.preview", "qkm_preview_changes", Read | Configure, true,
        schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("configuration.remove")}}},
            {QStringLiteral("catalog_revision"), revision}, {QStringLiteral("group"), groupName}},
            {QStringLiteral("action"), QStringLiteral("catalog_revision"), QStringLiteral("group")}));
    auto removeConfigurationApply = capability("configuration.remove.apply", "qkm_apply_changes", Read | Configure, false, deleteApplySchema);
    const QJsonArray removeLimits{QStringLiteral("idle_after_completed_cleanup"), QStringLiteral("non_current_ordinary_group_only"),
        QStringLiteral("preserve_selected_configuration_and_drafts")};
    for (auto *item : {&removeConfiguration, &removeConfigurationApply}) {
        item->insert(QStringLiteral("limitations"), removeLimits);
        item->insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
        capabilities.append(*item);
    }
    capabilities.append(operation);
    return capabilities;
}

qint64 MappingChangeStore::now()
{
    if (!m_clock.isValid()) m_clock.start();
    return m_clock.elapsed();
}

bool MappingChangeStore::remember(const MappingChangePreview &preview, const QString &status,
    const QString &errorCode, const QString &message)
{
    QJsonObject value{{QStringLiteral("status"), status}, {QStringLiteral("persistence"), QStringLiteral("memory_only")}};
    if (preview.save) value.insert(QStringLiteral("target"), saveOperationTarget(preview));
    if (preview.removal) value.insert(QStringLiteral("target"), removalOperationTarget(preview));
    if (preview.update || preview.preferences || preview.macro || preview.configurationImport || preview.configurationList)
        value.insert(QStringLiteral("target"), preview.description.value(QStringLiteral("target")));
    if (!errorCode.isEmpty()) value.insert(QStringLiteral("error"), QJsonObject{
        {QStringLiteral("code"), errorCode}, {QStringLiteral("message"), message}});
    return remember(preview.owner, preview.id, std::move(value));
}

bool MappingChangeStore::remember(const MappingChangeOwner &owner, const QString &id, QJsonObject value)
{
    if (!m_results.contains(id) && m_results.size() >= MaxResults) {
        auto oldest = m_results.end();
        for (auto it = m_results.begin(); it != m_results.end(); ++it) {
            if (!operationPending(it->value) && (oldest == m_results.end() || it->created < oldest->created))
                oldest = it;
        }
        if (oldest == m_results.end()) return false;
        m_results.erase(oldest);
    }
    value.insert(QStringLiteral("operation_id"), id);
    value.insert(QStringLiteral("sampled_at_utc"), sampleTime());
    value.insert(QStringLiteral("result_known"), true);
    m_results.insert(id, {owner, std::move(value), now()});
    return true;
}

MappingChangeStore::RunReservation MappingChangeStore::reserveStart(const MappingChangeOwner &owner,
    const QString &id, const QJsonObject &target)
{
    return reserveRun(owner, id, {{QStringLiteral("action"), QStringLiteral("mapping.start")},
        {QStringLiteral("status"), QStringLiteral("applying")}, {QStringLiteral("target"), target},
        {QStringLiteral("persistence"), QStringLiteral("memory_only")}});
}

MappingChangeStore::RunReservation MappingChangeStore::reserveStop(const MappingChangeOwner &owner,
    const QString &id, bool idle, bool initiated, bool cleanupObserved)
{
    QJsonObject value{{QStringLiteral("action"), QStringLiteral("mapping.stop")},
        {QStringLiteral("status"), idle ? QStringLiteral("already_idle") : QStringLiteral("waiting_cleanup")},
        {QStringLiteral("target"), QJsonObject{{QStringLiteral("instance_id"), owner.instance}}},
        {QStringLiteral("stop_initiated"), initiated},
        {QStringLiteral("stop_completion"), cleanupObserved ? QStringLiteral("observed") : QStringLiteral("unobserved")},
        {QStringLiteral("cleanup_timed_out"), false}, {QStringLiteral("persistence"), QStringLiteral("memory_only")}};
    if (!idle) value.insert(QStringLiteral("pending_steps"), QJsonArray{QStringLiteral("stop_cleanup")});
    return reserveRun(owner, id, std::move(value));
}

MappingChangeStore::RunReservation MappingChangeStore::reserveRun(const MappingChangeOwner &owner,
    const QString &id, QJsonObject value)
{
    prune();
    if (m_previews.contains(id) || m_results.contains(id)) return RunReservation::Conflict;
    return remember(owner, id, std::move(value)) ? RunReservation::Reserved : RunReservation::Full;
}

void MappingChangeStore::bindStop(const QString &id, quint64 generation, bool timedOut)
{
    const auto it = m_results.find(id);
    if (it == m_results.end() || it->value.value(QStringLiteral("status")) != QLatin1String("waiting_cleanup")) return;
    it->stopGeneration = generation;
    if (timedOut) updateStop(it.value(), false, true);
}

void MappingChangeStore::observeStop(quint64 generation, bool completed, bool timedOut)
{
    if (generation == 0) return;
    for (auto it = m_results.begin(); it != m_results.end(); ++it) {
        if (it->stopGeneration == generation && it->value.value(QStringLiteral("status")) == QLatin1String("waiting_cleanup"))
            updateStop(it.value(), completed, timedOut);
    }
}

void MappingChangeStore::updateStop(Operation &operation, bool completed, bool timedOut)
{
    auto &value = operation.value;
    value.insert(QStringLiteral("sampled_at_utc"), sampleTime());
    value.insert(QStringLiteral("cleanup_timed_out"), timedOut || value.value(QStringLiteral("cleanup_timed_out")).toBool());
    if (completed) {
        value.insert(QStringLiteral("status"), QStringLiteral("completed"));
        value.insert(QStringLiteral("stop_completion"), QStringLiteral("observed"));
        value.remove(QStringLiteral("error"));
        value.remove(QStringLiteral("pending_steps"));
        operation.created = now();
    } else if (timedOut) {
        value.insert(QStringLiteral("error"), QJsonObject{{QStringLiteral("code"), QStringLiteral("stop_timeout")},
            {QStringLiteral("message"), QStringLiteral("Stop cleanup is still pending.")}});
    }
}

void MappingChangeStore::prune()
{
    const qint64 tick = now();
    for (auto it = m_results.begin(); it != m_results.end();) {
        if (!operationPending(it->value) && tick - it->created >= ResultTtlMs)
            it = m_results.erase(it);
        else ++it;
    }
    for (auto it = m_previews.begin(); it != m_previews.end();) {
        if (tick - it->created >= PreviewTtlMs || (it->save && it->save->expiresAt.hasExpired())) {
            remember(it.value(), QStringLiteral("cancelled"), QStringLiteral("preview_expired"),
                QStringLiteral("Preview expired without applying any changes."));
            it = m_previews.erase(it);
        } else ++it;
    }
}

std::optional<MappingChangePreview> MappingChangeStore::create(const MappingChangeOwner &owner,
    const MappingAdditionArguments &arguments, const QJsonObject &description, const QString &preferencesRevision)
{
    prune();
    for (auto it = m_previews.begin(); it != m_previews.end();) {
        if (it->owner.instance == owner.instance && it->owner.pairId == owner.pairId) {
            remember(it.value(), QStringLiteral("cancelled"), QStringLiteral("preview_superseded"),
                QStringLiteral("A new preview replaced this client's earlier preview without applying it."));
            it = m_previews.erase(it);
        } else ++it;
    }
    if (m_previews.size() >= MaxPreviews) return std::nullopt;
    QString id;
    do { id = QUuid::createUuid().toString(QUuid::Id128); } while (m_previews.contains(id) || m_results.contains(id));
    MappingChangePreview preview{owner, arguments, description, id, now()};
    preview.preferencesRevision = preferencesRevision;
    m_previews.insert(id, preview);
    return preview;
}

std::optional<MappingChangePreview> MappingChangeStore::createSave(const MappingChangeOwner &owner,
    const ConfigurationSaveBinding &binding, const QJsonObject &description, const QString &preferencesRevision)
{
    if (binding.settingName.isEmpty() || !isLowerHex(binding.revision, 64) || binding.context.isEmpty() || binding.scope.isEmpty()
        || binding.expiresAt.isForever() || binding.expiresAt.hasExpired() || binding.expiresAt.remainingTime() > PreviewTtlMs)
        return std::nullopt;
    auto preview = create(owner, MappingAdditionArguments(), description, preferencesRevision);
    if (preview) {
        preview->save = binding;
        m_previews[preview->id] = *preview;
    }
    return preview;
}

std::optional<MappingChangePreview> MappingChangeStore::createRemoval(const MappingChangeOwner &owner,
    const MappingRemovalArguments &arguments, const QJsonObject &description, const QString &preferencesRevision)
{
    auto preview = create(owner, MappingAdditionArguments(), description, preferencesRevision);
    if (preview) {
        preview->removal = arguments;
        m_previews[preview->id] = *preview;
    }
    return preview;
}

std::optional<MappingChangePreview> MappingChangeStore::createUpdate(const MappingChangeOwner &owner,
    const MappingUpdateArguments &arguments, const QJsonObject &description, const QString &preferencesRevision)
{
    auto preview = create(owner, MappingAdditionArguments(), description, preferencesRevision);
    if (preview) { preview->update = arguments; m_previews[preview->id] = *preview; }
    return preview;
}

std::optional<MappingChangePreview> MappingChangeStore::createPreferences(const MappingChangeOwner &owner,
    const IbInputPreferencesArguments &arguments, const QJsonObject &description, const QString &preferencesRevision)
{
    auto preview = create(owner, MappingAdditionArguments(), description, preferencesRevision);
    if (preview) { preview->preferences = arguments; m_previews[preview->id] = *preview; }
    return preview;
}

std::optional<MappingChangePreview> MappingChangeStore::createMacro(const MappingChangeOwner &owner,
    const MacroChangeArguments &arguments, const QJsonObject &description, const QString &preferencesRevision)
{
    auto preview = create(owner, MappingAdditionArguments(), description, preferencesRevision);
    if (preview) { preview->macro = arguments; m_previews[preview->id] = *preview; }
    return preview;
}

std::optional<MappingChangePreview> MappingChangeStore::createConfigurationImport(const MappingChangeOwner &owner,
    const ConfigurationImportBinding &binding, const QJsonObject &description, const QString &preferencesRevision)
{
    if (!isLowerHex(binding.contextRevision, 64)) return std::nullopt;
    if (binding.draftId.isEmpty()) {
        if (!isLowerHex(binding.catalogRevision, 64) || binding.groups.isEmpty() || binding.groups.size() > MaxConfigurationBackupGroups
            || binding.document.isEmpty() || binding.document.size() > MaxConfigurationBackupBytes) return std::nullopt;
    } else {
        if (!isLowerHex(binding.draftId, 32) || !binding.catalogRevision.isEmpty() || !binding.groups.isEmpty()
            || !binding.document.isEmpty() || binding.overwrite) return std::nullopt;
        const auto draft = configurationImportDraft(owner, binding.draftId);
        if (!draft || draft->contextRevision != binding.contextRevision) return std::nullopt;
    }
    auto preview = create(owner, MappingAdditionArguments{}, description, preferencesRevision);
    if (preview) {
        preview->configurationImport = binding;
        m_previews[preview->id] = *preview;
    }
    return preview;
}

std::optional<MappingChangePreview> MappingChangeStore::createConfigurationList(const MappingChangeOwner &owner,
    const ConfigurationListBinding &binding, const QJsonObject &description, const QString &preferencesRevision)
{
    const bool targetMatches = binding.action == ConfigurationListAction::Move
        ? !binding.targetGroup.isEmpty() : binding.targetGroup.isEmpty();
    if (!isLowerHex(binding.catalogRevision, 64) || !isLowerHex(binding.contextRevision, 64)
        || binding.reset() != binding.group.isEmpty() || !targetMatches) return std::nullopt;
    auto preview = create(owner, MappingAdditionArguments{}, description, preferencesRevision);
    if (preview) {
        preview->configurationList = binding;
        m_previews[preview->id] = *preview;
    }
    return preview;
}

bool MappingChangeStore::stageConfigurationImport(const MappingChangePreview &preview, QJsonObject result)
{
    if (!preview.configurationImport || !preview.configurationImport->draftId.isEmpty() || expired(preview)) return false;
    const auto operation = m_results.find(preview.id);
    if (operation == m_results.end() || !(operation->owner == preview.owner)
        || operation->value.value(QStringLiteral("status")) != QLatin1String("applying")) return false;
    operation->configurationImportDraft = preview.configurationImport;
    result.insert(QStringLiteral("draft_id"), preview.id);
    result.insert(QStringLiteral("draft_state"), QStringLiteral("pending"));
    finish(preview.owner, preview.id, QStringLiteral("applied"), result);
    return true;
}

std::optional<ConfigurationImportBinding> MappingChangeStore::configurationImportDraft(const MappingChangeOwner &owner, const QString &id)
{
    prune();
    const auto operation = m_results.constFind(id);
    if (operation == m_results.cend() || !(operation->owner == owner)) return std::nullopt;
    return operation->configurationImportDraft;
}

bool MappingChangeStore::releaseConfigurationImportDraft(const MappingChangeOwner &owner, const QString &id, const QString &state)
{
    const auto operation = m_results.find(id);
    if (operation == m_results.end() || !(operation->owner == owner)) return false;
    auto result = operation->value.value(QStringLiteral("result")).toObject();
    if (!operation->configurationImportDraft) return result.value(QStringLiteral("draft_state")) == state;
    operation->configurationImportDraft.reset();
    result.insert(QStringLiteral("draft_state"), state);
    operation->value.insert(QStringLiteral("result"), result);
    operation->value.insert(QStringLiteral("sampled_at_utc"), sampleTime());
    return true;
}

void MappingChangeStore::completeConfigurationImportSave(const MappingChangeOwner &owner, const QString &id)
{
    releaseConfigurationImportDraft(owner, id, QStringLiteral("saved"));
}

std::optional<MappingChangePreview> MappingChangeStore::begin(const MappingChangeOwner &owner, const QString &id)
{
    prune();
    const auto it = m_previews.find(id);
    if (it == m_previews.end() || !(it->owner == owner)) return std::nullopt;
    const auto preview = it.value();
    // Never accept work without retaining its result slot, or evict an in-flight operation.
    if (!remember(preview, QStringLiteral("applying"))) return std::nullopt;
    m_previews.erase(it);
    return preview;
}

void MappingChangeStore::finish(const MappingChangeOwner &owner, const QString &id, const QString &status,
    const QJsonObject &result, const QString &errorCode, const QString &message)
{
    const auto it = m_results.find(id);
    if (it == m_results.end() || !(it->owner == owner) || it->value.value(QStringLiteral("status")) != QStringLiteral("applying")) return;
    it->value.insert(QStringLiteral("status"), status);
    it->value.insert(QStringLiteral("result"), result);
    it->value.insert(QStringLiteral("sampled_at_utc"), sampleTime());
    if (!errorCode.isEmpty()) it->value.insert(QStringLiteral("error"), QJsonObject{
        {QStringLiteral("code"), errorCode}, {QStringLiteral("message"), message}});
    it->created = now();
}

bool MappingChangeStore::cancel(const MappingChangeOwner &owner, const QString &id)
{
    prune();
    const auto it = m_previews.find(id);
    if (it == m_previews.end()) return releaseConfigurationImportDraft(owner, id, QStringLiteral("cancelled"));
    if (!(it->owner == owner)) return false;
    remember(it.value(), QStringLiteral("cancelled"));
    m_previews.erase(it);
    return true;
}

QJsonObject MappingChangeStore::operation(const MappingChangeOwner &owner, const QString &id)
{
    prune();
    const auto result = m_results.constFind(id);
    if (result != m_results.constEnd() && result->owner == owner) {
        QJsonObject value = result->value;
        value.insert(QStringLiteral("query_sampled_at_utc"), sampleTime());
        value.insert(QStringLiteral("historical_result"), true);
        return value;
    }
    const auto preview = m_previews.constFind(id);
    if (preview != m_previews.constEnd() && preview->owner == owner) {
        qint64 remaining = PreviewTtlMs - (now() - preview->created);
        if (preview->save) remaining = qMin(remaining, preview->save->expiresAt.remainingTime());
        QJsonObject value{{QStringLiteral("operation_id"), id}, {QStringLiteral("status"), QStringLiteral("previewed")},
            {QStringLiteral("result_known"), true}, {QStringLiteral("sampled_at_utc"), sampleTime()},
            {QStringLiteral("remaining_ms"), double(qMax(qint64(0), remaining))}};
        if (preview->save) value.insert(QStringLiteral("target"), saveOperationTarget(*preview));
        if (preview->removal) value.insert(QStringLiteral("target"), removalOperationTarget(*preview));
        if (preview->macro || preview->configurationImport || preview->configurationList)
            value.insert(QStringLiteral("target"), preview->description.value(QStringLiteral("target")));
        return value;
    }
    return {{QStringLiteral("operation_id"), id}, {QStringLiteral("status"), QStringLiteral("unknown")},
        {QStringLiteral("result_known"), false}, {QStringLiteral("sampled_at_utc"), sampleTime()},
        {QStringLiteral("recovery"), QStringLiteral("Query current state and source; do not blindly repeat a write. Results are bounded and do not survive interface reset or process exit.")}};
}

void MappingChangeStore::clear() { m_previews.clear(); m_results.clear(); }

} // namespace QkmAutomation
