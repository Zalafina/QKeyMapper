#ifndef QKM_IBINPUT_VALIDATION_H
#define QKM_IBINPUT_VALIDATION_H

#include "qkeymapper_worker.h"
#include <QCoreApplication>
#include <QSet>

namespace QkmIbInputValidation {
// shortcut: output follows FakerInput's VK-only HID; revise source aliases if backend key distinctions change.
inline quint16 keyboardSourceCode(const V_KEYCODE &key) {
    if (key.KeyCode==VK_RETURN && key.ExtenedFlag) return VK_RETURN|0x100;
    if (!key.ExtenedFlag) {
        switch (key.KeyCode) {
        case VK_INSERT: return VK_NUMPAD0;
        case VK_END: return VK_NUMPAD1;
        case VK_DOWN: return VK_NUMPAD2;
        case VK_NEXT: return VK_NUMPAD3;
        case VK_LEFT: return VK_NUMPAD4;
        case VK_CLEAR: return VK_NUMPAD5;
        case VK_RIGHT: return VK_NUMPAD6;
        case VK_HOME: return VK_NUMPAD7;
        case VK_UP: return VK_NUMPAD8;
        case VK_PRIOR: return VK_NUMPAD9;
        case VK_DELETE: return VK_DECIMAL;
        default: break;
        }
    }
    return key.KeyCode;
}

QList<MAP_KEYDATA> expandedRows(const QList<MAP_KEYDATA> &source);

inline int movementGroup(const MAP_KEYDATA &row) {
    for (const QString &key:row.Pure_MappingKeys+row.Pure_MappingKeys_KeyUp)
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
