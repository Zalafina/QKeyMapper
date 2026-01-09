#ifndef QKEYMAPPER_QT_COMPAT_H
#define QKEYMAPPER_QT_COMPAT_H

#include <QtGlobal>
#include <QList>

namespace QKeyMapperQtCompat {

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

} // namespace QKeyMapperQtCompat

#endif // QKEYMAPPER_QT_COMPAT_H
