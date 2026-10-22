#ifndef QKEYMAPPER_AUTOMATION_PRIVATE_STORAGE_H
#define QKEYMAPPER_AUTOMATION_PRIVATE_STORAGE_H

#include <QByteArray>
#include <QString>
#include <QtCore/qt_windows.h>

class QSaveFile;

namespace QkmAutomation { namespace Storage {

HANDLE privateDirectory(const QString &path, const QByteArray &sid, bool create);
HANDLE privateFile(const QString &path, const QByteArray &sid, bool deleteAccess = false);
bool secureTemporaryFile(QSaveFile &file, const QByteArray &sid);

} }
#endif
