#include "qkm_source_snapshot.h"
#include "qkm_auth_proof.h"
#include "qkm_ipc_frame.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>

namespace QkmAutomation {

SourceQueryResult validateSourceQueryArguments(const QJsonObject &arguments)
{
    const auto fail = [](const char *code, const char *message) {
        return SourceQueryResult{{}, QString::fromLatin1(code), QString::fromLatin1(message)};
    };
    const QSet<QString> allowed{QStringLiteral("entity"), QStringLiteral("source_revision"),
        QStringLiteral("source_tab_ref"), QStringLiteral("offset"), QStringLiteral("limit")};
    for (auto it = arguments.constBegin(); it != arguments.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return fail("invalid_arguments", "Unknown source query argument.");
    }
    const auto integer = [&](const QString &name, int fallback, int minimum, int maximum, int &output) {
        const QJsonValue value = arguments.value(name);
        if (value.isUndefined()) { output = fallback; return true; }
        if (!value.isDouble()) return false;
        const double number = value.toDouble();
        if (!(number >= minimum && number <= maximum)) return false;
        output = static_cast<int>(number);
        return number == output;
    };
    const QJsonValue entityValue = arguments.value(QStringLiteral("entity"));
    const QString entity = entityValue.toString();
    const QJsonValue revisionValue = arguments.value(QStringLiteral("source_revision"));
    const QJsonValue tabValue = arguments.value(QStringLiteral("source_tab_ref"));
    const bool fresh = revisionValue.isUndefined();
    int offset = 0, limit = 50;
    if (!entityValue.isString()
        || (!fresh && (!revisionValue.isString() || !isLowerHex(revisionValue.toString(), 64)))
        || !integer(QStringLiteral("offset"), 0, 0, 2147483647, offset)
        || !integer(QStringLiteral("limit"), 50, 1, 100, limit)) {
        return fail("invalid_arguments", "Invalid source query types, ranges or revision.");
    }
    if (entity != QStringLiteral("mapping_sources") && entity != QStringLiteral("mappings")) {
        return fail("unsupported_query", "Unsupported source query entity.");
    }
    int sourceIndex = -1;
    if (entity == QStringLiteral("mapping_sources")) {
        if (!tabValue.isUndefined() || (fresh && offset != 0)) {
            return fail("invalid_arguments", "Source listing needs its revision for later pages and accepts no source target.");
        }
    } else {
        if (fresh || !tabValue.isString() || tabValue.toString().size() > 128) {
            return fail("invalid_arguments", "Mapping rows require the source revision and source Tab reference.");
        }
        const QStringList parts = tabValue.toString().split(QLatin1Char(':'));
        bool validIndex = false;
        if (parts.size() == 3) sourceIndex = parts.at(2).toInt(&validIndex);
        if (parts.size() != 3 || !isLowerHex(parts.at(0), 64) || parts.at(1) != QStringLiteral("source")
            || !validIndex || sourceIndex < 0 || QString::number(sourceIndex) != parts.at(2)) {
            return fail("invalid_arguments", "Malformed source Tab reference.");
        }
        if (parts.at(0) != revisionValue.toString()) {
            return fail("stale_source", "Source reference and revision differ; query again.");
        }
    }
    return {};
}

SourceQueryResult querySourceSnapshot(const QJsonObject &snapshot, const QString &instance,
    const QString &pairId, quint64 permissionEpoch, const QJsonObject &arguments)
{
    const auto validation = validateSourceQueryArguments(arguments);
    if (!validation.ok()) return validation;
    const auto fail = [](const char *code, const char *message) {
        return SourceQueryResult{{}, QString::fromLatin1(code), QString::fromLatin1(message)};
    };
    const QString entity = arguments.value(QStringLiteral("entity")).toString();
    const QJsonValue revisionValue = arguments.value(QStringLiteral("source_revision"));
    const bool fresh = revisionValue.isUndefined();
    const int offset = arguments.value(QStringLiteral("offset")).toInt(0);
    const int limit = arguments.value(QStringLiteral("limit")).toInt(50);
    const int sourceIndex = entity == QStringLiteral("mappings")
        ? arguments.value(QStringLiteral("source_tab_ref")).toString().section(QLatin1Char(':'), 2, 2).toInt() : -1;
    if (snapshot.isEmpty() || !snapshot.value(QStringLiteral("sources")).isArray()) {
        return fail("context_busy", "A safe source snapshot is not available.");
    }
    const QByteArray bytes = QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
    if (bytes.size() > MaxSourceSnapshotBytes) {
        return fail("result_too_large", "Source snapshot exceeds its bounded size.");
    }
    const QJsonObject binding{{QStringLiteral("instance"), instance}, {QStringLiteral("pair_id"), pairId},
        {QStringLiteral("permission_epoch"), QString::number(permissionEpoch)},
        {QStringLiteral("snapshot"), snapshot}};
    const QString revision = QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(binding).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
    if (!fresh && revisionValue.toString() != revision) {
        return fail("stale_source", "Source data, identity or authorization changed; query again.");
    }
    const QJsonArray sources = snapshot.value(QStringLiteral("sources")).toArray();
    const auto sourceReference = [&](int index) {
        return revision + QStringLiteral(":source:") + QString::number(index);
    };
    const auto sourceMetadata = [&](int index) {
        QJsonObject metadata = sources.at(index).toObject();
        metadata.insert(QStringLiteral("row_count"), metadata.value(QStringLiteral("rows")).toArray().size());
        metadata.remove(QStringLiteral("rows"));
        metadata.insert(QStringLiteral("source_tab_ref"), sourceReference(index));
        return metadata;
    };
    QJsonArray rows;
    QJsonObject selectedSource;
    if (entity == QStringLiteral("mapping_sources")) {
        for (int index = 0; index < sources.size(); ++index) rows.append(sourceMetadata(index));
    } else {
        if (sourceIndex >= sources.size()) return fail("invalid_arguments", "Source Tab index exceeds the snapshot.");
        rows = sources.at(sourceIndex).toObject().value(QStringLiteral("rows")).toArray();
        selectedSource = sourceMetadata(sourceIndex);
    }
    if (offset > rows.size()) return fail("invalid_arguments", "Offset exceeds the source result size.");
    QJsonObject result{{QStringLiteral("schema_version"), 1}, {QStringLiteral("entity"), entity},
        {QStringLiteral("source_snapshot"), QJsonObject{{QStringLiteral("revision"), revision},
            {QStringLiteral("instance"), instance}, {QStringLiteral("reference_scope"), QStringLiteral("source_snapshot_only")},
            {QStringLiteral("scope"), QStringLiteral("current_memory_sources")},
            {QStringLiteral("read_basis"), snapshot.value(QStringLiteral("read_basis"))},
            {QStringLiteral("cleanup_generation"), snapshot.value(QStringLiteral("cleanup_generation"))},
            {QStringLiteral("configuration_selection"), snapshot.value(QStringLiteral("configuration_selection"))},
            {QStringLiteral("mapping_load_context"), snapshot.value(QStringLiteral("mapping_load_context"))},
            {QStringLiteral("common_feature_enabled"), snapshot.value(QStringLiteral("common_feature_enabled"))}}},
        {QStringLiteral("offset"), offset}, {QStringLiteral("total"), int(rows.size())},
        {QStringLiteral("items"), QJsonArray()}, {QStringLiteral("next_offset"), QJsonValue(QJsonValue::Null)}};
    if (!selectedSource.isEmpty()) result.insert(QStringLiteral("source"), selectedSource);
    qsizetype size = QJsonDocument(result).toJson(QJsonDocument::Compact).size();
    if (size > MaxFramePayload - 256) return fail("result_too_large", "Source response metadata is too large.");
    QJsonArray items;
    int next = offset;
    for (; next < rows.size() && items.size() < limit; ++next) {
        QJsonObject item = rows.at(next).toObject();
        if (entity == QStringLiteral("mappings")) {
            item.insert(QStringLiteral("source_ref"), revision + QStringLiteral(":row:")
                + QString::number(sourceIndex) + QLatin1Char(':') + QString::number(next));
            item.insert(QStringLiteral("source_tab_ref"), sourceReference(sourceIndex));
            item.insert(QStringLiteral("source_row"), next);
        }
        const qsizetype itemSize = QJsonDocument(item).toJson(QJsonDocument::Compact).size() + 1;
        if (size + itemSize > MaxFramePayload - 256) {
            if (items.isEmpty()) return fail("item_too_large", "One source item exceeds the response size.");
            break;
        }
        items.append(item);
        size += itemSize;
    }
    result.insert(QStringLiteral("items"), items);
    if (next < rows.size()) result.insert(QStringLiteral("next_offset"), next);
    return {result, {}, {}};
}

} // namespace QkmAutomation
