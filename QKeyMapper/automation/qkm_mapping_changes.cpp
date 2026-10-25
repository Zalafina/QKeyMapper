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
    const QString idField = query ? QStringLiteral("operation_id") : QStringLiteral("preview_id");
    const QSet<QString> allowed = preview
        ? QSet<QString>{QStringLiteral("source_revision"), QStringLiteral("source_tab_ref"),
            QStringLiteral("original_key"), QStringLiteral("mapping_key"), QStringLiteral("insert_row")}
        : apply ? QSet<QString>{idField, QStringLiteral("confirm_conflicts")} : QSet<QString>{idField};
    for (auto it = arguments.constBegin(); it != arguments.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return fail("Unknown mapping change argument.");
    }
    if (!preview) {
        const auto id = arguments.value(idField);
        if (!id.isString() || !isLowerHex(id.toString(), 32)) return fail("A canonical 32-character preview or operation identifier is required.");
        result.id = id.toString();
        const auto confirmation = arguments.value(QStringLiteral("confirm_conflicts"));
        if (!confirmation.isUndefined() && !confirmation.isBool()) return fail("Conflict confirmation must be a boolean.");
        result.confirmConflicts = confirmation.toBool();
        return result;
    }
    const QJsonObject sourceQuery{{QStringLiteral("entity"), QStringLiteral("mappings")},
        {QStringLiteral("source_revision"), arguments.value(QStringLiteral("source_revision"))},
        {QStringLiteral("source_tab_ref"), arguments.value(QStringLiteral("source_tab_ref"))}};
    const auto source = validateSourceQueryArguments(sourceQuery);
    if (!source.ok()) { result.errorCode = source.errorCode; result.message = source.message; return result; }
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
    const QJsonObject identifierSchema = string(QStringLiteral("^[0-9a-f]{32}$"), 32);
    const auto schema = [](const QJsonObject &properties, const QJsonArray &required) {
        return QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("additionalProperties"), false}, {QStringLiteral("properties"), properties},
            {QStringLiteral("required"), required}};
    };
    const QJsonObject preview = schema({{QStringLiteral("source_revision"), revision}, {QStringLiteral("source_tab_ref"), reference},
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
    QJsonObject operation = capability("operation.read", "qkm_get_operation", Read, true,
        schema({{QStringLiteral("operation_id"), identifierSchema}}, {QStringLiteral("operation_id")}));
    operation.insert(QStringLiteral("result_ttl_ms"), double(MappingChangeStore::ResultTtlMs));
    operation.insert(QStringLiteral("max_results"), MappingChangeStore::MaxResults);
    return {previewCapability, capability("mapping.add.apply", "qkm_apply_changes", Read | Configure, false, apply),
        capability("mapping.preview.cancel", "qkm_cancel_changes", Read | Configure, false,
            schema({{QStringLiteral("preview_id"), identifierSchema}}, {QStringLiteral("preview_id")})), operation};
}

qint64 MappingChangeStore::now()
{
    if (!m_clock.isValid()) m_clock.start();
    return m_clock.elapsed();
}

void MappingChangeStore::remember(const MappingChangePreview &preview, const QString &status,
    const QString &errorCode, const QString &message)
{
    if (!m_results.contains(preview.id) && m_results.size() >= MaxResults) {
        auto oldest = m_results.begin();
        for (auto it = m_results.begin(); it != m_results.end(); ++it) {
            if (it->created < oldest->created) oldest = it;
        }
        m_results.erase(oldest);
    }
    QJsonObject value{{QStringLiteral("operation_id"), preview.id}, {QStringLiteral("status"), status},
        {QStringLiteral("sampled_at_utc"), sampleTime()}, {QStringLiteral("result_known"), true},
        {QStringLiteral("persistence"), QStringLiteral("memory_only")}};
    if (!errorCode.isEmpty()) value.insert(QStringLiteral("error"), QJsonObject{
        {QStringLiteral("code"), errorCode}, {QStringLiteral("message"), message}});
    m_results.insert(preview.id, {preview.owner, value, now()});
}

void MappingChangeStore::prune()
{
    const qint64 tick = now();
    for (auto it = m_results.begin(); it != m_results.end();) {
        if (tick - it->created >= ResultTtlMs) it = m_results.erase(it); else ++it;
    }
    for (auto it = m_previews.begin(); it != m_previews.end();) {
        if (tick - it->created >= PreviewTtlMs) {
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

std::optional<MappingChangePreview> MappingChangeStore::begin(const MappingChangeOwner &owner, const QString &id)
{
    prune();
    const auto it = m_previews.find(id);
    if (it == m_previews.end() || !(it->owner == owner)) return std::nullopt;
    const auto preview = it.value();
    m_previews.erase(it);
    remember(preview, QStringLiteral("applying"));
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
        return {{QStringLiteral("operation_id"), id}, {QStringLiteral("status"), QStringLiteral("previewed")},
            {QStringLiteral("result_known"), true}, {QStringLiteral("sampled_at_utc"), sampleTime()},
            {QStringLiteral("remaining_ms"), double(qMax(qint64(0), PreviewTtlMs - (now() - preview->created)))}};
    }
    return {{QStringLiteral("operation_id"), id}, {QStringLiteral("status"), QStringLiteral("unknown")},
        {QStringLiteral("result_known"), false}, {QStringLiteral("sampled_at_utc"), sampleTime()},
        {QStringLiteral("recovery"), QStringLiteral("Query current state and source; do not blindly repeat a write. Results are bounded and do not survive interface reset or process exit.")}};
}

void MappingChangeStore::clear() { m_previews.clear(); m_results.clear(); }

} // namespace QkmAutomation
