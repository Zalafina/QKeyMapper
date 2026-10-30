#ifndef QKM_IBINPUT_VALIDATION_H
#define QKM_IBINPUT_VALIDATION_H

#include "qkeymapper_worker.h"
#include <QCoreApplication>
#include <QSet>

namespace QkmIbInputValidation {
inline int movementGroup(const MAP_KEYDATA &row) {
    for (const QString &key:row.Pure_MappingKeys)
        if (key.startsWith(QStringLiteral("Key2Mouse-"))) return 0;
    if (row.Original_Key.startsWith(QStringLiteral("Joy-LS2Mouse"))
        || row.Original_Key.startsWith(QStringLiteral("Joy-RS2Mouse"))) return 1;
    if (row.Original_Key.contains(QLatin1String(QKeyMapperConstants::JOY_GYRO2MOUSE_STR))) return 2;
    if (row.Original_Key.contains(QLatin1String(QKeyMapperConstants::JOY_TOUCHPAD2MOUSE_STR))) return 3;
    return -1;
}

inline QString mixedMovementError(const QList<MAP_KEYDATA> &rows,const QStringList &locations) {
    QHash<int,QSet<int>> methods;
    QHash<int,QStringList> sources;
    for (int index=0;index<rows.size();++index) {
        const auto &row=rows.at(index);
        if (row.Disabled) continue;
        const int group=movementGroup(row);
        if (group<0) continue;
        methods[group].insert(row.SendMappingKeyMethod);
        sources[group].append(locations.value(index,
            QCoreApplication::translate("QkmIbInputClient","Row %1").arg(index+1)));
    }
    for (int group=0;group<4;++group)
        if (methods.value(group).contains(QKeyMapperConstants::SENDMAPPINGKEY_METHOD_IBINPUT)
            && methods.value(group).size()>1)
            return QCoreApplication::translate("QkmIbInputClient",
                "IbInput is mixed with another send method in one mouse movement group: %1")
                .arg(sources.value(group).join(QStringLiteral(", ")));
    return {};
}
}
#endif
