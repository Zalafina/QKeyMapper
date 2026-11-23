#pragma once

#include <QByteArray>
#include <QStringList>
#include <QVariant>

namespace QkmAutomation {

constexpr int MaxMacroBackupBytes = 128 * 1024;
constexpr int MaxMacroBackupEntries = 100;
constexpr int MaxTableBackupBytes = 128 * 1024;
constexpr int MaxTableBackupRows = 100;

enum class BackupDecodeStatus { Ok, MissingRequiredField, InvalidFormat, TooManyEntries, TooLarge };

struct MacroBackupDocument {
    BackupDecodeStatus status;
    QVariantList entries;
};

struct MappingTableBackupDocument {
    BackupDecodeStatus status;
    QVariantMap values;
};

QByteArray encodeMacroBackup(const QVariantList &macros);
bool appendBackupIniValue(QByteArray &document, const QByteArray &key, const QVariant &value, int maxBytes);
MacroBackupDocument decodeMacroBackup(const QByteArray &document);
MappingTableBackupDocument decodeMappingTableBackup(const QByteArray &document, const QStringList &fieldNames);

}
