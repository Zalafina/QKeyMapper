#ifndef QKM_SOURCE_SNAPSHOT_H
#define QKM_SOURCE_SNAPSHOT_H

#include <QJsonObject>
#include <QString>

namespace QkmAutomation {

inline constexpr int MaxSourceSnapshotBytes = 4 * 1024 * 1024;

struct SourceQueryResult {
    QJsonObject page;
    QString errorCode;
    QString message;
    bool ok() const { return errorCode.isEmpty(); }
};

SourceQueryResult querySourceSnapshot(const QJsonObject &snapshot, const QString &instance,
    const QString &pairId, quint64 permissionEpoch, const QJsonObject &arguments);

} // namespace QkmAutomation

#endif
