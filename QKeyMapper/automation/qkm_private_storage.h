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

class DirectoryLease {
public:
    DirectoryLease() = default;
    ~DirectoryLease();
    bool acquire(const QString &directory, const QByteArray &sid, bool createDirectory);
    void release();
    DWORD lastError() const { return m_error; }

private:
    Q_DISABLE_COPY_MOVE(DirectoryLease)
    HANDLE m_directory = INVALID_HANDLE_VALUE;
    HANDLE m_file = INVALID_HANDLE_VALUE;
    DWORD m_error = ERROR_SUCCESS;
};

} }
#endif
