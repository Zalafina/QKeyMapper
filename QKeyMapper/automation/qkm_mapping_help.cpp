#include "qkm_mapping_help.h"
#include "qkm_pair_store.h"
#include "qkm_ipc_frame.h"
#include "../qkeymapper_constants.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>

namespace QkmAutomation {
namespace {

QJsonArray topics()
{
    return {QStringLiteral("basic_mapping"), QStringLiteral("original_keys"), QStringLiteral("mapping_keys")};
}

QJsonArray limitations()
{
    return {QStringLiteral("syntax_reference_not_full_mapping_validation"),
        QStringLiteral("key_names_do_not_prove_device_availability"), QStringLiteral("help_does_not_authorize_operations")};
}

QJsonObject basicMappingHelp()
{
    using namespace QKeyMapperConstants;
    return {{QStringLiteral("id"), QStringLiteral("basic_mapping")},
        {QStringLiteral("example_syntax"), QJsonObject{{QStringLiteral("original_key"), QStringLiteral("A")},
            {QStringLiteral("mapping_key"), QStringLiteral("B")}}},
        {QStringLiteral("key_names"), QJsonObject{{QStringLiteral("original"), QStringLiteral("original_keys")},
            {QStringLiteral("mapping"), QStringLiteral("mapping_keys")}, {QStringLiteral("case_sensitive"), true}}},
        {QStringLiteral("combination_separator"), QStringLiteral("+")},
        {QStringLiteral("sequence_separator"), QString::fromUtf8(SEPARATOR_NEXTARROW)},
        {QStringLiteral("sequence_max_items_exclusive"), KEY_SEQUENCE_MAX},
        {QStringLiteral("wait"), QJsonObject{{QStringLiteral("unit"), QStringLiteral("ms")},
            {QStringLiteral("minimum"), MAPPING_WAITTIME_MIN}, {QStringLiteral("maximum"), MAPPING_WAITTIME_MAX},
            {QStringLiteral("suffix"), QString::fromUtf8(SEPARATOR_WAITTIME)}, {QStringLiteral("zero_omits_suffix"), true},
            {QStringLiteral("literal_suffix_minimum"), 1}}},
        {QStringLiteral("burst"), QJsonObject{{QStringLiteral("unit"), QStringLiteral("ms")},
            {QStringLiteral("minimum"), BURST_TIME_MIN}, {QStringLiteral("maximum"), BURST_TIME_MAX},
            {QStringLiteral("press_default"), BURST_PRESS_TIME_DEFAULT},
            {QStringLiteral("release_default"), BURST_RELEASE_TIME_DEFAULT},
            {QStringLiteral("requires"), QStringLiteral("burst_enabled")}}},
        {QStringLiteral("repeat"), QJsonObject{{QStringLiteral("modes"), QJsonObject{
            {QStringLiteral("none"), REPEAT_MODE_NONE}, {QStringLiteral("while_pressed"), REPEAT_MODE_BYKEY},
            {QStringLiteral("count"), REPEAT_MODE_BYTIMES}}},
            {QStringLiteral("count_minimum"), REPEAT_TIMES_MIN}, {QStringLiteral("count_maximum"), REPEAT_TIMES_MAX},
            {QStringLiteral("count_default"), REPEAT_TIMES_DEFAULT},
            {QStringLiteral("zero_count_meaning"), QStringLiteral("unlimited_in_count_mode")}}},
        {QStringLiteral("mcp_edit_requires"), QJsonArray{QStringLiteral("idle"), QStringLiteral("no_pending_input_cleanup"),
            QStringLiteral("current_target_validation"), QStringLiteral("configure_permission")}},
        {QStringLiteral("operation_discovery"), QStringLiteral("qkm_get_capabilities")}};
}

} // namespace

QJsonObject mappingHelpCapability()
{
    return {{QStringLiteral("id"), QStringLiteral("help.read")},
        {QStringLiteral("method"), QStringLiteral("qkm_get_help")},
        {QStringLiteral("required_scopes"), int(Read)}, {QStringLiteral("read_only"), true},
        {QStringLiteral("arguments_schema"), QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("additionalProperties"), false}, {QStringLiteral("required"), QJsonArray{QStringLiteral("topic")}},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("topic"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                    {QStringLiteral("enum"), topics()}}},
                {QStringLiteral("offset"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
                    {QStringLiteral("minimum"), 0}, {QStringLiteral("maximum"), 2147483647}}},
                {QStringLiteral("limit"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
                    {QStringLiteral("minimum"), 1}, {QStringLiteral("maximum"), 100}}}}}}},
        {QStringLiteral("limitations"), limitations()}};
}

MappingHelpResult queryMappingHelp(const QJsonObject &arguments,
    const QStringList &originalKeys, const QStringList &mappingKeys)
{
    const auto fail = [](const char *code, const char *message) {
        return MappingHelpResult{{}, QString::fromLatin1(code), QString::fromLatin1(message)};
    };
    const QSet<QString> allowed{QStringLiteral("topic"), QStringLiteral("offset"), QStringLiteral("limit")};
    for (auto it = arguments.constBegin(); it != arguments.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return fail("invalid_arguments", "Unknown help argument.");
    }
    const auto integer = [&](const QString &name, int fallback, int minimum, int maximum, int &out) {
        const QJsonValue value = arguments.value(name);
        if (value.isUndefined()) { out = fallback; return true; }
        if (!value.isDouble()) return false;
        const double number = value.toDouble();
        if (!(number >= minimum && number <= maximum)) return false;
        out = static_cast<int>(number);
        return number == out;
    };
    const auto topicValue = arguments.value(QStringLiteral("topic"));
    int offset = 0, limit = 50;
    if (!topicValue.isString() || !integer(QStringLiteral("offset"), 0, 0, 2147483647, offset)
        || !integer(QStringLiteral("limit"), 50, 1, 100, limit))
        return fail("invalid_arguments", "Invalid help topic, offset or limit type/range.");
    if (!topics().contains(topicValue)) return fail("unsupported_query", "Unsupported help topic.");
    const QString topic = topicValue.toString();
    QJsonArray items;
    int total = 1;
    if (topic == QStringLiteral("basic_mapping")) {
        if (offset == 0) items.append(basicMappingHelp());
    } else {
        QStringList keys = topic == QStringLiteral("original_keys") ? originalKeys : mappingKeys;
        keys.removeAll(QString());
        keys.removeDuplicates();
        keys.sort(Qt::CaseSensitive);
        if (keys.isEmpty()) return fail("context_busy", "Parser key names are not initialized.");
        total = int(keys.size());
        for (int i = offset; i < total && i - offset < limit; ++i) items.append(keys.at(i));
    }
    if (offset > total) return fail("invalid_arguments", "Help offset exceeds the topic size.");
    const int next = offset + int(items.size());
    QJsonObject page{{QStringLiteral("schema_version"), 1}, {QStringLiteral("topic"), topic},
        {QStringLiteral("reference_scope"), QStringLiteral("parser_help_only")}, {QStringLiteral("total"), total},
        {QStringLiteral("offset"), offset}, {QStringLiteral("items"), items},
        {QStringLiteral("next_offset"), next < total ? QJsonValue(next) : QJsonValue(QJsonValue::Null)},
        {QStringLiteral("limitations"), limitations()}};
    if (QJsonDocument(page).toJson(QJsonDocument::Compact).size() > MaxFramePayload - 256)
        return fail("result_too_large", "Help page exceeds the response limit; request fewer items.");
    return {page, {}, {}};
}

} // namespace QkmAutomation
