#include "qkm_macro_backup.h"
#include "../qkeymapper_constants.h"

#include <QDataStream>
#include <QIODevice>

namespace QkmAutomation {
namespace {

using Status = MacroBackupStatus;
constexpr char VariantPrefix[] = "@Variant(";
constexpr int VariantPrefixLength = sizeof(VariantPrefix) - 1;
constexpr quint32 NullDataLength = 0xffffffffU;

int hexDigit(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

bool appendIniEscape(const QByteArray &value, int &index, QByteArray &token)
{
    if (++index == value.size()) return false;
    const char ch = value.at(index);
    switch (ch) {
    case 'a': token.append('\a'); return true;
    case 'b': token.append('\b'); return true;
    case 'f': token.append('\f'); return true;
    case 'n': token.append('\n'); return true;
    case 'r': token.append('\r'); return true;
    case 't': token.append('\t'); return true;
    case 'v': token.append('\v'); return true;
    case '"': case '\\': token.append(ch); return true;
    default: break;
    }

    const bool hex = ch == 'x';
    int number = hex ? 0 : ch - '0';
    if (!hex && (number < 0 || number > 7)) return false;
    int digits = hex ? 0 : 1;
    while (index + 1 < value.size()) {
        const int digit = hexDigit(value.at(index + 1));
        if (digit < 0 || (!hex && digit > 7)) break;
        number = number * (hex ? 16 : 8) + digit;
        if (++digits > (hex ? 2 : 3) || number > 255) return false;
        ++index;
    }
    if (digits == 0) return false;
    token.append(char(number));
    return true;
}

Status splitIniValue(const QByteArray &value, QList<QByteArray> &tokens)
{
    bool quoted = false;
    QByteArray token;
    for (int index = 0; index < value.size(); ++index) {
        const char ch = value.at(index);
        if (ch == '\\') {
            if (!appendIniEscape(value, index, token)) return Status::InvalidFormat;
        } else if (ch == '"') {
            quoted = !quoted;
        } else if (!quoted && ch == ',') {
            tokens.append(token.trimmed());
            if (tokens.size() == MaxMacroBackupEntries) return Status::TooManyEntries;
            token.clear();
        } else if (!quoted && ch == ';') {
            break;
        } else {
            token.append(ch);
        }
    }
    if (quoted) return Status::InvalidFormat;
    tokens.append(token.trimmed());
    return Status::Ok;
}

bool skipBytes(QDataStream &stream, quint32 size)
{
    return size <= quint64(stream.device()->bytesAvailable()) && stream.skipRawData(int(size)) == int(size);
}

bool skipSizedValue(QDataStream &stream, bool utf16)
{
    quint32 size = 0;
    stream >> size;
    return stream.status() == QDataStream::Ok
        && (size == NullDataLength || ((!utf16 || size % 2 == 0) && skipBytes(stream, size)));
}

// Check Qt 4.0 types and lengths before QVariant is allowed to construct them.
Status checkVariant(QDataStream &stream, int depth, bool listDocument)
{
    if (depth > 3) return Status::InvalidFormat;
    quint32 type = 0;
    stream >> type;
    if (stream.status() != QDataStream::Ok) return Status::InvalidFormat;
    if (listDocument && type != QMetaType::QVariantList) return Status::InvalidFormat;

    switch (type) {
    case QMetaType::UnknownType:
    case QMetaType::QString:
        return skipSizedValue(stream, true) ? Status::Ok : Status::InvalidFormat;
    case QMetaType::QByteArray:
        return skipSizedValue(stream, false) ? Status::Ok : Status::InvalidFormat;
    case QMetaType::Bool:
        return skipBytes(stream, 1) ? Status::Ok : Status::InvalidFormat;
    case QMetaType::Int: case QMetaType::UInt:
        return skipBytes(stream, 4) ? Status::Ok : Status::InvalidFormat;
    case QMetaType::LongLong: case QMetaType::ULongLong: case QMetaType::Double:
        return skipBytes(stream, 8) ? Status::Ok : Status::InvalidFormat;
    case QMetaType::QVariantMap: case QMetaType::QVariantList: case QMetaType::QStringList:
        break;
    default:
        return Status::InvalidFormat;
    }

    quint32 count = 0;
    stream >> count;
    if (stream.status() != QDataStream::Ok) return Status::InvalidFormat;
    if (listDocument && count > MaxMacroBackupEntries) return Status::TooManyEntries;
    const bool map = type == QMetaType::QVariantMap;
    if (count > quint64(stream.device()->bytesAvailable()) / (map ? 8 : 4)) return Status::InvalidFormat;
    for (quint32 i = 0; i < count; ++i) {
        if ((map || type == QMetaType::QStringList) && !skipSizedValue(stream, true)) return Status::InvalidFormat;
        if (type != QMetaType::QStringList && checkVariant(stream, depth + 1, false) != Status::Ok)
            return Status::InvalidFormat;
    }
    return Status::Ok;
}

Status decodeVariant(const QByteArray &token, bool listDocument, QVariant &value)
{
    if (!token.startsWith(VariantPrefix)) return listDocument ? Status::InvalidFormat : Status::Ok;
    if (!token.endsWith(')')) return Status::InvalidFormat;
    const auto payload = token.mid(VariantPrefixLength, token.size() - VariantPrefixLength - 1);
    QDataStream checked(payload);
    checked.setVersion(QDataStream::Qt_4_0);
    const auto status = checkVariant(checked, 0, listDocument);
    if (status != Status::Ok) return status;
    if (!checked.atEnd()) return Status::InvalidFormat;

    QDataStream stream(payload);
    stream.setVersion(QDataStream::Qt_4_0);
    stream >> value;
    return stream.status() == QDataStream::Ok && stream.atEnd() ? Status::Ok : Status::InvalidFormat;
}

} // namespace

bool appendBackupIniValue(QByteArray &document, const QByteArray &key, const QVariant &value, int maxBytes)
{
    const QByteArray prefix("=\"@Variant(");
    const QByteArray suffix(")\"\n");
    const qsizetype available = maxBytes - document.size() - key.size() - prefix.size() - suffix.size();
    if (available <= 0) return false;

    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_0);
    stream << value;
    if (stream.status() != QDataStream::Ok || payload.size() > available / 4) return false;

    const QByteArray hex = payload.toHex();
    document.reserve(document.size() + key.size() + prefix.size() + 2 * hex.size() + suffix.size());
    document.append(key).append(prefix);
    for (qsizetype index = 0; index < hex.size(); index += 2) {
        document.append("\\x").append(hex.constData() + index, 2);
    }
    document.append(suffix);
    return true;
}

QByteArray encodeMacroBackup(const QVariantList &macros)
{
    QByteArray document("[General]\n");
    // Wrap a single macro in a list so QSettings still reads the value as a list.
    if (!appendBackupIniValue(document, QKeyMapperConstants::MACROLIST_EXPORT, QVariant(macros), MaxMacroBackupBytes))
        return {};
    return document;
}

MacroBackupDocument decodeMacroBackup(const QByteArray &document)
{
    if (document.size() > MaxMacroBackupBytes) return {Status::TooLarge, {}};
    QByteArray value;
    bool general = true;
    bool found = false;
    const auto lines = document.split('\n');
    for (auto line : lines) {
        if (line.startsWith("\xef\xbb\xbf")) line.remove(0, 3);
        line = line.trimmed();
        if (line.startsWith('[')) {
            const auto end = line.indexOf(']');
            general = end >= 0 && line.mid(1, end - 1).compare("General", Qt::CaseInsensitive) == 0;
        } else if (general && !line.startsWith(';') && !line.startsWith('#')) {
            const auto equals = line.indexOf('=');
            if (equals < 0 || line.left(equals).trimmed().compare(QKeyMapperConstants::MACROLIST_EXPORT,
                    Qt::CaseInsensitive) != 0) continue;
            if (found) return {Status::InvalidFormat, {}};
            value = line.mid(equals + 1);
            found = true;
        }
    }
    if (!found) return {Status::MissingList, {}};

    QList<QByteArray> tokens;
    const auto split = splitIniValue(value, tokens);
    if (split != Status::Ok) return {split, {}};
    if (tokens.size() == 1) {
        QVariant list;
        const auto status = decodeVariant(tokens.first(), true, list);
        return {status, list.toList()};
    }
    QVariantList entries;
    entries.reserve(tokens.size());
    for (const auto &token : tokens) {
        QVariant entry;
        const auto status = decodeVariant(token, false, entry);
        if (status != Status::Ok) return {status, {}};
        entries.append(entry);
    }
    return {Status::Ok, entries};
}

}
