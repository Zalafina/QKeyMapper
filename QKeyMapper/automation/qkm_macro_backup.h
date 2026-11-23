#pragma once

#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QVariant>

namespace QkmAutomation {

constexpr int MaxMacroBackupBytes = 128 * 1024;
constexpr int MaxMacroBackupEntries = 100;
constexpr int MaxTableBackupBytes = 128 * 1024;
constexpr int MaxTableBackupRows = 100;
constexpr int MaxConfigurationBackupBytes = 128 * 1024;
constexpr int MaxConfigurationBackupGroups = 100;

enum class BackupDecodeStatus { Ok, MissingRequiredField, InvalidFormat, TooManyEntries, TooLarge };

struct MacroBackupDocument {
    BackupDecodeStatus status;
    QVariantList entries;
};

struct MappingTableBackupDocument {
    BackupDecodeStatus status;
    QVariantMap values;
};

struct ConfigurationBackupGroup {
    QString name;
    QVariantMap values;
};

struct ConfigurationBackupDocument {
    BackupDecodeStatus status;
    QList<ConfigurationBackupGroup> groups;
};

QByteArray encodeMacroBackup(const QVariantList &macros);
bool appendBackupIniValue(QByteArray &document, const QByteArray &key, const QVariant &value, int maxBytes);
QByteArray encodeBackupIniIdentifier(const QString &identifier);
MacroBackupDocument decodeMacroBackup(const QByteArray &document);
MappingTableBackupDocument decodeMappingTableBackup(const QByteArray &document, const QStringList &fieldNames);
ConfigurationBackupDocument decodeConfigurationBackup(const QByteArray &document);

}
