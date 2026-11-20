#include "qkm_mapping_changes.h"
#include "qkm_auth_proof.h"
#include "qkm_pair_store.h"
#include "qkm_source_snapshot.h"

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
    const bool savePreview = preview && action.isString() && action.toString() == QStringLiteral("configuration.save");
    const bool deletePreview = preview && action.isString() && action.toString() == QStringLiteral("mapping.delete");
    const bool updatePreview = preview && action == QJsonValue(QStringLiteral("mapping.update"));
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
        || disabledVisibilityPreview || mappingDisplayPreview;
    const bool preferencesPreview = preview && action == QJsonValue(QStringLiteral("ibinput.preferences.update"));
    if (preview && !action.isUndefined() && (!action.isString()
        || (action.toString() != QStringLiteral("mapping.add") && !savePreview && !deletePreview && !updatePreview && !movePreview && !renamePreview && !tabActionPreview && !preferencesPreview))) return fail("Unsupported preview action.");
    const QString idField = query ? QStringLiteral("operation_id") : QStringLiteral("preview_id");
    const QSet<QString> allowed = savePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("setting_name")}
        : preferencesPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("preferences_revision"),
            QStringLiteral("preferred_backend"), QStringLiteral("unavailable_policy")}
        : renamePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("tab_name")}
        : tabHotkeyPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("tab_hotkey")}
        : tabNotificationPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("notification_mode")}
        : mappingDisplayPreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_tab_ref"), QStringLiteral("mapping_display")}
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
        : updatePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_ref"), QStringLiteral("source_refs"), QStringLiteral("send_method"),
            QStringLiteral("original_key"), QStringLiteral("mapping_key"), QStringLiteral("key_up_mapping_key"), QStringLiteral("disabled"), QStringLiteral("note"), QStringLiteral("category"), QStringLiteral("row_modes")}
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
    if (updatePreview || movePreview) {
        const auto singleReference = arguments.value(QStringLiteral("source_ref"));
        const auto batch = arguments.value(QStringLiteral("source_refs"));
        const auto references = batch.toArray();
        const auto category = arguments.value(QStringLiteral("category"));
        if (!batch.isUndefined() && ((!movePreview && category.isUndefined())
            || !singleReference.isUndefined() || references.size() < 2 || references.size() > 100))
            return fail("Only moves and category updates accept 2 to 100 source_refs instead of source_ref.");
        const auto reference = references.isEmpty() ? singleReference : references.first();
        const auto original = arguments.value(QStringLiteral("original_key"));
        const auto mapping = arguments.value(QStringLiteral("mapping_key"));
        const auto keyUp = arguments.value(QStringLiteral("key_up_mapping_key"));
        const auto disabled = arguments.value(QStringLiteral("disabled"));
        const auto note = arguments.value(QStringLiteral("note"));
        const auto rowModes = arguments.value(QStringLiteral("row_modes"));
        if (!rowModes.isUndefined()) {
            const auto modes = rowModes.toObject();
            if (modes.isEmpty() || arguments.size() != 3 || !singleReference.isString())
                return fail("Provide row_modes and one source_ref, without other changed fields.");
            for (auto it = modes.constBegin(); it != modes.constEnd(); ++it) {
                if (!it.value().isBool() || (it.key() != QStringLiteral("burst") && it.key() != QStringLiteral("lock")))
                    return fail("Only burst and lock booleans are allowed in row_modes.");
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
            && disabled.isUndefined() && note.isUndefined() && category.isUndefined() && rowModes.isUndefined())
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
        QList<int> categoryRows;
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
                if (!indexOk || index < 0 || ref != prefix + QString::number(index) || categoryRows.contains(index))
                    return fail("Category references must be distinct rows of the same source and revision.");
                categoryRows.append(index);
            }
            std::sort(categoryRows.begin(), categoryRows.end());
        }
        const int firstRow = categoryRows.isEmpty() ? row : categoryRows.constFirst();
        result.action = MappingChangeAction::UpdateMapping;
        result.update = {prefix + QString::number(firstRow), parts.at(0), parts.at(0) + QStringLiteral(":source:") + parts.at(2),
            firstRow, updatePreview ? sendMethod() : -1};
        result.update.move = rowMove;
        if (movePreview) result.update.rowCount = rowCount;
        result.update.categoryRows = std::move(categoryRows);
        if (!original.isUndefined()) result.update.originalKey = original.toString();
        if (!mapping.isUndefined()) result.update.mappingKey = mapping.toString();
        if (!keyUp.isUndefined()) result.update.keyUpMappingKey = keyUp.toString();
        if (!disabled.isUndefined()) result.update.disabled = disabled.toBool();
        if (!note.isUndefined()) result.update.note = note.toString();
        if (!category.isUndefined()) result.update.category = category.toString();
        result.update.rowModes = rowModes.toObject();
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
    auto rowModesSchema = schema({{QStringLiteral("burst"), toggle}, {QStringLiteral("lock"), toggle}}, {});
    rowModesSchema.insert(QStringLiteral("minProperties"), 1);
    auto updateSchema = schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("mapping.update")}}},
        {QStringLiteral("row_modes"), rowModesSchema},
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
    rowModeConflicts.append(QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("source_refs")}}});
    updateSchema.insert(QStringLiteral("dependentSchemas"), QJsonObject{
        {QStringLiteral("disabled"), QJsonObject{{QStringLiteral("not"), QJsonObject{{QStringLiteral("anyOf"), updateFields}}}}},
        {QStringLiteral("note"), QJsonObject{{QStringLiteral("not"), QJsonObject{{QStringLiteral("anyOf"), noteConflicts}}}}},
        {QStringLiteral("category"), QJsonObject{{QStringLiteral("not"), QJsonObject{{QStringLiteral("anyOf"), categoryConflicts}}}}},
        {QStringLiteral("source_refs"), categoryRequired},
        {QStringLiteral("row_modes"), QJsonObject{{QStringLiteral("not"), QJsonObject{{QStringLiteral("anyOf"), rowModeConflicts}}}}}});
    updateFields.append(disabledRequired);
    updateFields.append(noteRequired);
    updateFields.append(categoryRequired);
    updateFields.append(QJsonObject{{QStringLiteral("required"), QJsonArray{QStringLiteral("row_modes")}}});
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
    if (preview.update || preview.preferences) value.insert(QStringLiteral("target"), preview.description.value(QStringLiteral("target")));
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
    if (it == m_previews.end() || !(it->owner == owner)) return false;
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
        return value;
    }
    return {{QStringLiteral("operation_id"), id}, {QStringLiteral("status"), QStringLiteral("unknown")},
        {QStringLiteral("result_known"), false}, {QStringLiteral("sampled_at_utc"), sampleTime()},
        {QStringLiteral("recovery"), QStringLiteral("Query current state and source; do not blindly repeat a write. Results are bounded and do not survive interface reset or process exit.")}};
}

void MappingChangeStore::clear() { m_previews.clear(); m_results.clear(); }

} // namespace QkmAutomation
