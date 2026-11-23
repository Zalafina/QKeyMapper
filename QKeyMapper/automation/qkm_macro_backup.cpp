#include "qkm_macro_backup.h"
#include "../qkeymapper_constants.h"

#include <QColor>
#include <QDataStream>
#include <QHash>
#include <QIODevice>
#include <QPoint>
#include <QSet>
#include <QSize>
#include <algorithm>
#include <utility>

namespace QkmAutomation {
namespace {

using Status = BackupDecodeStatus;
constexpr char VariantPrefix[] = "@Variant(";
constexpr int VariantPrefixLength = sizeof(VariantPrefix) - 1;
constexpr quint32 NullDataLength = 0xffffffffU;
constexpr quint32 Qt4VariantColorType = 67;

int hexDigit(ushort ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

bool appendIniEscape(const QString &value, qsizetype &index, QString &token)
{
    if (++index == value.size()) return false;
    const ushort ch = value.at(index).unicode();
    switch (ch) {
    case 'a': token.append(QLatin1Char('\a')); return true;
    case 'b': token.append(QLatin1Char('\b')); return true;
    case 'f': token.append(QLatin1Char('\f')); return true;
    case 'n': token.append(QLatin1Char('\n')); return true;
    case 'r': token.append(QLatin1Char('\r')); return true;
    case 't': token.append(QLatin1Char('\t')); return true;
    case 'v': token.append(QLatin1Char('\v')); return true;
    case '"': case '\\': case '\'': case '?': token.append(QChar(ch)); return true;
    default: break;
    }

    const bool hex = ch == 'x';
    int number = hex ? 0 : ch - '0';
    if (!hex && (number < 0 || number > 7)) return false;
    int digits = hex ? 0 : 1;
    while (index + 1 < value.size()) {
        const int digit = hexDigit(value.at(index + 1).unicode());
        if (digit < 0 || (!hex && digit > 7)) break;
        number = number * (hex ? 16 : 8) + digit;
        if (++digits > (hex ? 4 : 6) || number > 0xffff) return false;
        ++index;
    }
    if (digits == 0) return false;
    token.append(QChar(ushort(number)));
    return true;
}

Status splitIniValue(const QByteArray &bytes, QStringList &tokens, int maxEntries)
{
    const auto value = QString::fromUtf8(bytes.constData(), bytes.size());
    if (value.toUtf8() != bytes) return Status::InvalidFormat;
    bool quoted = false;
    QString token;
    qsizetype trailingSpaces = 0;
    for (qsizetype index = 0; index < value.size(); ++index) {
        const auto ch = value.at(index);
        if (ch == QLatin1Char('\\')) {
            if (!appendIniEscape(value, index, token)) return Status::InvalidFormat;
            trailingSpaces = 0;
        } else if (ch == QLatin1Char('"')) {
            quoted = !quoted;
            trailingSpaces = 0;
        } else if (!quoted && ch == QLatin1Char(',')) {
            token.chop(trailingSpaces);
            tokens.append(token);
            if (tokens.size() == maxEntries) return Status::TooManyEntries;
            token.clear();
            trailingSpaces = 0;
        } else if (!quoted && ch == QLatin1Char(';')) {
            break;
        } else if (!quoted && (ch == QLatin1Char(' ') || ch == QLatin1Char('\t'))) {
            if (!token.isEmpty()) {
                token.append(ch);
                ++trailingSpaces;
            }
        } else {
            token.append(ch);
            trailingSpaces = 0;
        }
    }
    if (quoted) return Status::InvalidFormat;
    token.chop(trailingSpaces);
    tokens.append(token);
    return Status::Ok;
}

template<typename FieldReader>
Status readBackupIniFields(const QByteArray &document, FieldReader &&readField, bool strict = false)
{
    QByteArray section("General");
    for (auto line : document.split('\n')) {
        if (line.startsWith("\xef\xbb\xbf")) line.remove(0, 3);
        line = line.trimmed();
        if (line.startsWith('[')) {
            const auto end = line.indexOf(']');
            section = end >= 0 ? line.mid(1, end - 1).trimmed() : QByteArray();
            if (strict && section.isEmpty()) return Status::InvalidFormat;
        } else if (!section.isEmpty() && !line.startsWith(';') && !line.startsWith('#')) {
            const auto equals = line.indexOf('=');
            if (equals < 0) {
                if (strict && !line.isEmpty()) return Status::InvalidFormat;
                continue;
            }
            if (const auto status = readField(section, line.left(equals).trimmed(), line.mid(equals + 1)); status != Status::Ok)
                return status;
        }
    }
    return Status::Ok;
}

template<typename FieldReader>
Status readRootIniFields(const QByteArray &document, FieldReader &&readField)
{
    return readBackupIniFields(document, [&](const QByteArray &section, const QByteArray &key, const QByteArray &value) {
        return section.compare("General", Qt::CaseInsensitive) == 0 ? readField(key, value) : Status::Ok;
    });
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
Status checkVariant(QDataStream &stream, int depth, bool listDocument, int maxRootEntries, bool profileValues)
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
    case QMetaType::QPoint: case QMetaType::QSize:
        return profileValues && skipBytes(stream, 8) ? Status::Ok : Status::InvalidFormat;
    case Qt4VariantColorType: {
        if (!profileValues) return Status::InvalidFormat;
        qint8 spec = 0;
        stream >> spec;
        // Common rows store the RGB colors from the pickers or an invalid color.
        return stream.status() == QDataStream::Ok && (spec == QColor::Invalid || spec == QColor::Rgb)
            && skipBytes(stream, 5 * sizeof(quint16)) ? Status::Ok : Status::InvalidFormat;
    }
    case QMetaType::QVariantMap: case QMetaType::QVariantList: case QMetaType::QStringList:
        break;
    default:
        return Status::InvalidFormat;
    }

    quint32 count = 0;
    stream >> count;
    if (stream.status() != QDataStream::Ok) return Status::InvalidFormat;
    if (depth == 0 && maxRootEntries > 0 && count > quint32(maxRootEntries)) return Status::TooManyEntries;
    const bool map = type == QMetaType::QVariantMap;
    if (count > quint64(stream.device()->bytesAvailable()) / (map ? 8 : 4)) return Status::InvalidFormat;
    for (quint32 i = 0; i < count; ++i) {
        if ((map || type == QMetaType::QStringList) && !skipSizedValue(stream, true)) return Status::InvalidFormat;
        if (type != QMetaType::QStringList && checkVariant(stream, depth + 1, false, 0, profileValues) != Status::Ok)
            return Status::InvalidFormat;
    }
    return Status::Ok;
}

Status decodeVariant(const QString &token, bool listDocument, int maxRootEntries, QVariant &value, bool profileValues = false)
{
    if (!token.startsWith(QLatin1String(VariantPrefix))) return listDocument ? Status::InvalidFormat : Status::Ok;
    if (!token.endsWith(QLatin1Char(')'))) return Status::InvalidFormat;
    const auto text = token.mid(VariantPrefixLength, token.size() - VariantPrefixLength - 1);
    for (const auto ch : text) {
        if (ch.unicode() > 255) return Status::InvalidFormat;
    }
    const auto payload = text.toLatin1();
    QDataStream checked(payload);
    checked.setVersion(QDataStream::Qt_4_0);
    const auto status = checkVariant(checked, 0, listDocument, maxRootEntries, profileValues);
    if (status != Status::Ok) return status;
    if (!checked.atEnd()) return Status::InvalidFormat;

    QDataStream stream(payload);
    stream.setVersion(QDataStream::Qt_4_0);
    stream >> value;
    return stream.status() == QDataStream::Ok && stream.atEnd() ? Status::Ok : Status::InvalidFormat;
}

Status decodeTableText(QStringList tokens, QVariant &value)
{
    if (tokens.size() == 1 && tokens.first() == QLatin1String("@Invalid()")) return Status::Ok;
    if (tokens.size() == 1 && tokens.first().startsWith(QLatin1String(VariantPrefix))) {
        const auto status = decodeVariant(tokens.first(), false, MaxTableBackupRows, value);
        if (status != Status::Ok) return status;
        return value.userType() == QMetaType::QStringList || value.userType() == QMetaType::QString
            ? Status::Ok : Status::InvalidFormat;
    }

    for (auto &token : tokens) {
        if (token.startsWith(QLatin1String("@@"))) {
            token.remove(0, 1);
        } else if (token.startsWith(QLatin1String("@String(")) && token.endsWith(QLatin1Char(')'))) {
            token = token.mid(8, token.size() - 9);
        } else if (token.startsWith(QLatin1String(VariantPrefix)) || token.startsWith(QLatin1String("@ByteArray("))
                   || token.startsWith(QLatin1String("@DateTime(")) || token == QLatin1String("@Invalid()")) {
            return Status::InvalidFormat;
        }
    }
    value = tokens;
    return Status::Ok;
}

Status decodeTableComments(const QStringList &tokens, QVariant &value)
{
    if (tokens.size() == 1 && tokens.first() == QLatin1String("@Invalid()")) return Status::Ok;

    QVariantList rows;
    if (tokens.size() == 1) {
        const auto status = decodeVariant(tokens.first(), true, MaxTableBackupRows, value);
        if (status != Status::Ok) return status;
        rows = value.toList();
    } else {
        rows.reserve(tokens.size());
        for (const auto &token : tokens) {
            QVariant row;
            const auto status = decodeVariant(token, true, 0, row);
            if (status != Status::Ok) return status;
            rows.append(row);
        }
    }
    for (const auto &row : std::as_const(rows)) {
        if (row.userType() != QMetaType::QVariantList && row.userType() != QMetaType::QStringList)
            return Status::InvalidFormat;
        const auto comments = row.toList();
        for (const auto &comment : comments) {
            if (comment.userType() != QMetaType::QString) return Status::InvalidFormat;
        }
    }
    value = rows;
    return Status::Ok;
}

QString decodeIniIdentifier(const QByteArray &bytes)
{
    const auto token = QString::fromUtf8(bytes.constData(), bytes.size());
    if (token.toUtf8() != bytes) return {};
    QString name;
    for (qsizetype i = 0; i < token.size(); ++i) {
        if (token.at(i) != QLatin1Char('%')) {
            name.append(token.at(i));
            continue;
        }
        const bool wide = i + 1 < token.size() && token.at(i + 1) == QLatin1Char('U');
        const qsizetype start = i + (wide ? 2 : 1);
        const int digits = wide ? 4 : 2;
        if (start + digits > token.size()) return {};
        int code = 0;
        for (int j = 0; j < digits; ++j) {
            const int digit = hexDigit(token.at(start + j).unicode());
            if (digit < 0) return {};
            code = code * 16 + digit;
        }
        name.append(QChar(ushort(code)));
        i = start + digits - 1;
    }
    if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\')) || name.contains(QLatin1Char('\0'))
        || name.contains(QLatin1Char('\r')) || name.contains(QLatin1Char('\n'))) return {};
    return name;
}

Status decodeIniGeometry(const QString &token, QVariant &value)
{
    if (!token.endsWith(QLatin1Char(')'))) return Status::InvalidFormat;
    const auto start = token.indexOf(QLatin1Char('(')) + 1;
    const auto parts = token.mid(start, token.size() - start - 1).split(QLatin1Char(' '));
    if (parts.size() != 2) return Status::InvalidFormat;
    bool firstOk = false, secondOk = false;
    const int first = parts.at(0).toInt(&firstOk), second = parts.at(1).toInt(&secondOk);
    if (!firstOk || !secondOk) return Status::InvalidFormat;
    if (token.startsWith(QLatin1String("@Point("))) value = QPoint(first, second);
    else value = QSize(first, second);
    return Status::Ok;
}

Status decodeConfigurationToken(const QString &token, QVariant &value)
{
    if (token.startsWith(QLatin1String(VariantPrefix))) return decodeVariant(token, false, 0, value, true);
    if (token == QLatin1String("@Invalid()")) return Status::Ok;
    if (token.startsWith(QLatin1String("@DateTime(")) || token.startsWith(QLatin1String("@Rect("))) return Status::InvalidFormat;
    if (token.startsWith(QLatin1String("@Point(")) || token.startsWith(QLatin1String("@Size("))) return decodeIniGeometry(token, value);
    if (token.startsWith(QLatin1String("@ByteArray("))) {
        if (!token.endsWith(QLatin1Char(')'))) return Status::InvalidFormat;
        const auto content = token.mid(11, token.size() - 12);
        const auto bytes = content.toLatin1();
        if (QString::fromLatin1(bytes.constData(), bytes.size()) != content) return Status::InvalidFormat;
        value = bytes;
    } else if (token.startsWith(QLatin1String("@String(")) && token.endsWith(QLatin1Char(')'))) {
        value = token.mid(8, token.size() - 9);
    } else {
        value = token.startsWith(QLatin1String("@@")) ? token.mid(1) : token;
    }
    return Status::Ok;
}

Status decodeConfigurationField(const QByteArray &raw, QVariant &value)
{
    QStringList tokens;
    const auto split = splitIniValue(raw, tokens, MaxConfigurationBackupBytes / 4);
    if (split != Status::Ok) return split;
    if (tokens.size() == 1) return decodeConfigurationToken(tokens.first(), value);

    const bool typedList = std::any_of(tokens.cbegin(), tokens.cend(), [](const QString &token) {
        return token.startsWith(QLatin1Char('@')) && !token.startsWith(QLatin1String("@@"));
    });
    if (!typedList) {
        for (auto &token : tokens) {
            if (token.startsWith(QLatin1String("@@"))) token.remove(0, 1);
        }
        value = tokens;
        return Status::Ok;
    }
    QVariantList entries;
    entries.reserve(tokens.size());
    for (const auto &token : std::as_const(tokens)) {
        QVariant entry;
        const auto status = decodeConfigurationToken(token, entry);
        if (status != Status::Ok) return status;
        entries.append(entry);
    }
    value = entries;
    return Status::Ok;
}

} // namespace

QByteArray encodeBackupIniIdentifier(const QString &identifier)
{
    QByteArray encoded;
    for (const auto character : identifier) {
        const auto code = character.unicode();
        if ((code >= 'A' && code <= 'Z') || (code >= 'a' && code <= 'z') || (code >= '0' && code <= '9')
            || code == '_' || code == '-' || code == '.') {
            encoded.append(char(code));
        } else {
            encoded.append(code > 0xff ? "%U" : "%");
            encoded.append(QByteArray::number(uint(code), 16).toUpper().rightJustified(code > 0xff ? 4 : 2, '0'));
        }
    }
    return encoded;
}

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
    bool found = false;
    const auto fields = readRootIniFields(document, [&](const QByteArray &key, const QByteArray &rawValue) {
        if (key.compare(QKeyMapperConstants::MACROLIST_EXPORT, Qt::CaseInsensitive) != 0) return Status::Ok;
        if (found) return Status::InvalidFormat;
        value = rawValue;
        found = true;
        return Status::Ok;
    });
    if (fields != Status::Ok) return {fields, {}};
    if (!found) return {Status::MissingRequiredField, {}};

    QStringList tokens;
    const auto split = splitIniValue(value, tokens, MaxMacroBackupEntries);
    if (split != Status::Ok) return {split, {}};
    if (tokens.size() == 1) {
        QVariant list;
        const auto status = decodeVariant(tokens.first(), true, MaxMacroBackupEntries, list);
        return {status, list.toList()};
    }
    QVariantList entries;
    entries.reserve(tokens.size());
    for (const auto &token : tokens) {
        QVariant entry;
        const auto status = decodeVariant(token, false, 0, entry);
        if (status != Status::Ok) return {status, {}};
        entries.append(entry);
    }
    return {Status::Ok, entries};
}

MappingTableBackupDocument decodeMappingTableBackup(const QByteArray &document, const QStringList &fieldNames)
{
    using namespace QKeyMapperConstants;
    if (document.size() > MaxTableBackupBytes) return {Status::TooLarge, {}};
    QHash<QByteArray, QString> names;
    for (const auto &field : fieldNames) names.insert(field.toLatin1().toLower(), field);

    QVariantMap values;
    const auto fields = readRootIniFields(document, [&](const QByteArray &key, const QByteArray &rawValue) {
        const auto name = names.constFind(key.toLower());
        if (name == names.cend()) return Status::Ok;
        if (values.contains(*name)) return Status::InvalidFormat;

        QStringList tokens;
        const auto split = splitIniValue(rawValue, tokens, MaxTableBackupRows);
        if (split != Status::Ok) return split;
        const bool comments = *name == QLatin1String(KEYMAPDATA_MAPPINGKEYS_COMMENTS)
            || *name == QLatin1String(KEYMAPDATA_MAPPINGKEYS_KEYUP_COMMENTS);
        QVariant value;
        const auto status = comments ? decodeTableComments(tokens, value) : decodeTableText(std::move(tokens), value);
        if (status == Status::Ok) values.insert(*name, value);
        return status;
    });
    if (fields != Status::Ok) return {fields, {}};

    const auto originalKeys = QLatin1String(KEYMAPDATA_ORIGINALKEYS);
    const auto mappingKeys = QLatin1String(KEYMAPDATA_MAPPINGKEYS);
    if (!values.contains(originalKeys) || !values.contains(mappingKeys)) return {Status::MissingRequiredField, {}};
    const auto count = values.value(originalKeys).toStringList().size();
    if (count == 0 || count != values.value(mappingKeys).toStringList().size())
        return {Status::InvalidFormat, {}};
    return {Status::Ok, values};
}

ConfigurationBackupDocument decodeConfigurationBackup(const QByteArray &document)
{
    if (document.size() > MaxConfigurationBackupBytes) return {Status::TooLarge, {}};
    QList<ConfigurationBackupGroup> groups;
    QList<QSet<QString>> fieldNames;
    QHash<QString, int> indexByName;
    const auto status = readBackupIniFields(document, [&](const QByteArray &section, const QByteArray &rawKey, const QByteArray &rawValue) {
        if (section.compare("General", Qt::CaseInsensitive) == 0
            || section.compare("%General", Qt::CaseInsensitive) == 0) return Status::Ok;
        const auto name = decodeIniIdentifier(section);
        if (name.isEmpty()) return Status::InvalidFormat;
        if (name.compare(QLatin1String(QKeyMapperConstants::CONFIG_FILE_TOPLEVEL_GROUPNAME), Qt::CaseInsensitive) == 0
            || name.compare(QLatin1String(QKeyMapperConstants::GROUPNAME_GLOBALSETTING), Qt::CaseInsensitive) == 0) return Status::Ok;

        const auto foldedName = name.toCaseFolded();
        int index = indexByName.value(foldedName, -1);
        if (index < 0) {
            if (groups.size() >= MaxConfigurationBackupGroups) return Status::TooManyEntries;
            index = int(groups.size());
            indexByName.insert(foldedName, index);
            groups.append(ConfigurationBackupGroup{name, {}});
            fieldNames.append(QSet<QString>());
        } else if (groups.at(index).name != name) {
            return Status::InvalidFormat;
        }
        const auto key = decodeIniIdentifier(rawKey);
        const auto foldedKey = key.toCaseFolded();
        if (key.isEmpty() || fieldNames.at(index).contains(foldedKey)) return Status::InvalidFormat;
        QVariant value;
        const auto decoded = decodeConfigurationField(rawValue, value);
        if (decoded == Status::Ok) {
            groups[index].values.insert(key, value);
            fieldNames[index].insert(foldedKey);
        }
        return decoded;
    }, true);
    if (status != Status::Ok) return {status, {}};
    if (groups.isEmpty()) return {Status::MissingRequiredField, {}};
    return {Status::Ok, groups};
}

}
