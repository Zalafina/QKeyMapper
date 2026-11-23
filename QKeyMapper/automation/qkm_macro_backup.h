#pragma once

#include <QByteArray>
#include <QVariant>

namespace QkmAutomation {

constexpr int MaxMacroBackupBytes = 128 * 1024;
constexpr int MaxMacroBackupEntries = 100;

enum class MacroBackupStatus { Ok, MissingList, InvalidFormat, TooManyEntries, TooLarge };

struct MacroBackupDocument {
    MacroBackupStatus status;
    QVariantList entries;
};

QByteArray encodeMacroBackup(const QVariantList &macros);
bool appendBackupIniValue(QByteArray &document, const QByteArray &key, const QVariant &value, int maxBytes);
MacroBackupDocument decodeMacroBackup(const QByteArray &document);

}
