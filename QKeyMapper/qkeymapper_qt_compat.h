#ifndef QKEYMAPPER_QT_COMPAT_H
#define QKEYMAPPER_QT_COMPAT_H

#include <QtGlobal>
#include <QList>
#include <QString>
#include <QMouseEvent>
#include <QPoint>

namespace QKeyMapperQtCompat {

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
using SplitBehavior = Qt::SplitBehavior;
constexpr SplitBehavior KeepEmptyParts = Qt::KeepEmptyParts;
constexpr SplitBehavior SkipEmptyParts = Qt::SkipEmptyParts;
#else
using SplitBehavior = QString::SplitBehavior;
constexpr SplitBehavior KeepEmptyParts = QString::KeepEmptyParts;
constexpr SplitBehavior SkipEmptyParts = QString::SkipEmptyParts;
#endif

template <typename T>
inline void resizeQList(QList<T> &list, int newSize, const T &fillValue)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    list.resize(newSize, fillValue);
#else
    if (newSize < 0) {
        newSize = 0;
    }

    while (list.size() < newSize) {
        list.append(fillValue);
    }
    while (list.size() > newSize) {
        list.removeLast();
    }
#endif
}

template <typename T>
inline void resizeQList(QList<T> &list, int newSize)
{
    resizeQList(list, newSize, T());
}

// QMouseEvent::globalPos() is deprecated in Qt6; use globalPosition().toPoint() instead.
inline QPoint mouseEventGlobalPos(const QMouseEvent *event)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}

} // namespace QKeyMapperQtCompat

#endif // QKEYMAPPER_QT_COMPAT_H
