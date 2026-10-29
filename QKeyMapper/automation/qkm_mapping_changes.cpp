#include "qkm_mapping_changes.h"
#include "qkm_auth_proof.h"
#include "qkm_pair_store.h"
#include "qkm_source_snapshot.h"

#include <QDateTime>
#include <QJsonArray>
#include <QSet>
#include <QUuid>

namespace QkmAutomation {
namespace {
QString sampleTime() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }
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

bool isMappingChangeMethod(const QString &method)
{
    return method == QStringLiteral("qkm_preview_changes") || method == QStringLiteral("qkm_apply_changes")
        || method == QStringLiteral("qkm_cancel_changes") || method == QStringLiteral("qkm_get_operation");
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
    const bool preview = method == QStringLiteral("qkm_preview_changes");
    const bool apply = method == QStringLiteral("qkm_apply_changes");
    const bool query = method == QStringLiteral("qkm_get_operation");
    result.method = preview ? MappingChangeMethod::Preview : apply ? MappingChangeMethod::Apply
        : query ? MappingChangeMethod::Operation : MappingChangeMethod::Cancel;
    const auto action = arguments.value(QStringLiteral("action"));
    const bool savePreview = preview && action.isString() && action.toString() == QStringLiteral("configuration.save");
    const bool deletePreview = preview && action.isString() && action.toString() == QStringLiteral("mapping.delete");
    if (preview && !action.isUndefined() && (!action.isString()
        || (action.toString() != QStringLiteral("mapping.add") && !savePreview && !deletePreview))) return fail("Unsupported preview action.");
    const QString idField = query ? QStringLiteral("operation_id") : QStringLiteral("preview_id");
    const QSet<QString> allowed = savePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("setting_name")}
        : deletePreview ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_revision"),
            QStringLiteral("source_tab_ref"), QStringLiteral("source_refs")} : preview
        ? QSet<QString>{QStringLiteral("action"), QStringLiteral("source_revision"), QStringLiteral("source_tab_ref"),
            QStringLiteral("original_key"), QStringLiteral("mapping_key"), QStringLiteral("insert_row")}
        : apply ? QSet<QString>{idField, QStringLiteral("confirm_conflicts"), QStringLiteral("confirm_scope"),
            QStringLiteral("confirm_overwrite"), QStringLiteral("confirm_delete")}
            : QSet<QString>{idField};
    for (auto it = arguments.constBegin(); it != arguments.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return fail("Unknown mapping change argument.");
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
        for (const auto &value : references.toArray()) {
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
    const auto basicKey = [](const QJsonValue &value) {
        if (!value.isString()) return false;
        const QString text = value.toString();
        return text.size() == 1 && ((text.at(0) >= QLatin1Char('A') && text.at(0) <= QLatin1Char('Z'))
            || (text.at(0) >= QLatin1Char('0') && text.at(0) <= QLatin1Char('9')));
    };
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

QJsonArray mappingChangeCapabilities()
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
    const QJsonObject preview = schema({{QStringLiteral("action"), QJsonObject{{QStringLiteral("const"), QStringLiteral("mapping.add")}}},
        {QStringLiteral("source_revision"), revision}, {QStringLiteral("source_tab_ref"), reference},
        {QStringLiteral("original_key"), key}, {QStringLiteral("mapping_key"), key},
        {QStringLiteral("insert_row"), QJsonObject{{QStringLiteral("type"), QJsonArray{QStringLiteral("integer"), QStringLiteral("null")}},
            {QStringLiteral("minimum"), 0}, {QStringLiteral("maximum"), 2147483647}}}},
        {QStringLiteral("source_revision"), QStringLiteral("source_tab_ref"), QStringLiteral("original_key"), QStringLiteral("mapping_key")});
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
    auto deleteApply = capability("mapping.delete.apply", "qkm_apply_changes", Read | Configure, false,
        schema({{QStringLiteral("preview_id"), identifierSchema}, {QStringLiteral("confirm_delete"), QJsonObject{{QStringLiteral("const"), true}}},
            {QStringLiteral("confirm_conflicts"), QJsonObject{{QStringLiteral("const"), false}}},
            {QStringLiteral("confirm_scope"), QJsonObject{{QStringLiteral("const"), false}}},
            {QStringLiteral("confirm_overwrite"), QJsonObject{{QStringLiteral("const"), false}}}},
            {QStringLiteral("preview_id"), QStringLiteral("confirm_delete")}));
    deleteApply.insert(QStringLiteral("limitations"), deleteLimits);
    deleteApply.insert(QStringLiteral("validation_budget_ms"), MappingChangeBudgetMs);
    deleteApply.insert(QStringLiteral("source_binding_result"), QStringLiteral("commit_snapshot_or_null"));
    auto addApply = capability("mapping.add.apply", "qkm_apply_changes", Read | Configure, false, apply);
    addApply.insert(QStringLiteral("source_binding_result"), QStringLiteral("commit_snapshot_or_null"));
    QJsonObject operation = capability("operation.read", "qkm_get_operation", Read, true,
        schema({{QStringLiteral("operation_id"), identifierSchema}}, {QStringLiteral("operation_id")}));
    operation.insert(QStringLiteral("result_ttl_ms"), double(MappingChangeStore::ResultTtlMs));
    operation.insert(QStringLiteral("max_results"), MappingChangeStore::MaxResults);
    return {previewCapability, saveCapability, addApply,
        saveApplyCapability, deleteCapability, deleteApply,
        capability("mapping.preview.cancel", "qkm_cancel_changes", Read | Configure, false,
            schema({{QStringLiteral("preview_id"), identifierSchema}}, {QStringLiteral("preview_id")})), operation};
}

qint64 MappingChangeStore::now()
{
    if (!m_clock.isValid()) m_clock.start();
    return m_clock.elapsed();
}

bool MappingChangeStore::remember(const MappingChangePreview &preview, const QString &status,
    const QString &errorCode, const QString &message)
{
    if (!m_results.contains(preview.id) && m_results.size() >= MaxResults) {
        auto oldest = m_results.end();
        for (auto it = m_results.begin(); it != m_results.end(); ++it) {
            if (it->value.value(QStringLiteral("status")) != QStringLiteral("applying")
                && (oldest == m_results.end() || it->created < oldest->created)) oldest = it;
        }
        if (oldest == m_results.end()) return false;
        m_results.erase(oldest);
    }
    QJsonObject value{{QStringLiteral("operation_id"), preview.id}, {QStringLiteral("status"), status},
        {QStringLiteral("sampled_at_utc"), sampleTime()}, {QStringLiteral("result_known"), true},
        {QStringLiteral("persistence"), QStringLiteral("memory_only")}};
    if (preview.save) value.insert(QStringLiteral("target"), saveOperationTarget(preview));
    if (preview.removal) value.insert(QStringLiteral("target"), removalOperationTarget(preview));
    if (!errorCode.isEmpty()) value.insert(QStringLiteral("error"), QJsonObject{
        {QStringLiteral("code"), errorCode}, {QStringLiteral("message"), message}});
    m_results.insert(preview.id, {preview.owner, value, now()});
    return true;
}

void MappingChangeStore::prune()
{
    const qint64 tick = now();
    for (auto it = m_results.begin(); it != m_results.end();) {
        if (it->value.value(QStringLiteral("status")) != QStringLiteral("applying") && tick - it->created >= ResultTtlMs)
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
    const MappingAdditionArguments &arguments, const QJsonObject &description)
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
    const MappingChangePreview preview{owner, arguments, description, id, now()};
    m_previews.insert(id, preview);
    return preview;
}

std::optional<MappingChangePreview> MappingChangeStore::createSave(const MappingChangeOwner &owner,
    const ConfigurationSaveBinding &binding, const QJsonObject &description)
{
    if (binding.settingName.isEmpty() || !isLowerHex(binding.revision, 64) || binding.context.isEmpty() || binding.scope.isEmpty()
        || binding.expiresAt.isForever() || binding.expiresAt.hasExpired() || binding.expiresAt.remainingTime() > PreviewTtlMs)
        return std::nullopt;
    auto preview = create(owner, MappingAdditionArguments(), description);
    if (preview) {
        preview->save = binding;
        m_previews[preview->id] = *preview;
    }
    return preview;
}

std::optional<MappingChangePreview> MappingChangeStore::createRemoval(const MappingChangeOwner &owner,
    const MappingRemovalArguments &arguments, const QJsonObject &description)
{
    auto preview = create(owner, MappingAdditionArguments(), description);
    if (preview) {
        preview->removal = arguments;
        m_previews[preview->id] = *preview;
    }
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
