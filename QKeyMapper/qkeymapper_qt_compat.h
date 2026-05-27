#ifndef QKEYMAPPER_QT_COMPAT_H
#define QKEYMAPPER_QT_COMPAT_H

#include <QtGlobal>
#include <QList>
#include <QString>
#include <QVariant>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QPoint>
#include <QTabBar>
#include <QTabWidget>

namespace QKeyMapperQtCompat {

namespace detail {

inline const char *tabVisibilityStatePropertyName()
{
    return "_qkm_tab_visibility_state";
}

inline const char *tabEnabledBeforeHideStatePropertyName()
{
    return "_qkm_tab_enabled_before_hide_state";
}

inline QTabWidget *owningTabWidget(QTabBar *tabBar)
{
    return (tabBar != Q_NULLPTR) ? qobject_cast<QTabWidget*>(tabBar->parentWidget()) : Q_NULLPTR;
}

inline QObject *tabVisibilityStateOwner(QTabBar *tabBar)
{
    if (QTabWidget *tabWidget = owningTabWidget(tabBar)) {
        return tabWidget;
    }

    return tabBar;
}

inline QWidget *tabPageWidget(QTabBar *tabBar, int index)
{
    if (QTabWidget *tabWidget = owningTabWidget(tabBar)) {
        if (index < 0 || index >= tabWidget->count()) {
            return Q_NULLPTR;
        }

        return tabWidget->widget(index);
    }

    return Q_NULLPTR;
}

inline QString tabPageStateKey(const QWidget *pageWidget)
{
    if (pageWidget == Q_NULLPTR) {
        return QString();
    }

    return QString::number(static_cast<qulonglong>(reinterpret_cast<quintptr>(pageWidget)), 16);
}

inline QVariantMap propertyMap(const QObject *object, const char *propertyName)
{
    return (object != Q_NULLPTR) ? object->property(propertyName).toMap() : QVariantMap();
}

inline void setTabEnabledCompat(QTabBar *tabBar, int index, bool enabled)
{
    if (QTabWidget *tabWidget = owningTabWidget(tabBar)) {
        tabWidget->setTabEnabled(index, enabled);
        return;
    }

    if (tabBar != Q_NULLPTR) {
        tabBar->setTabEnabled(index, enabled);
    }
}

} // namespace detail

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

// QMouseEvent::pos() is deprecated in Qt6; use position().toPoint() instead.
inline QPoint mouseEventLocalPos(const QMouseEvent *event)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    return event->position().toPoint();
#else
    return event->pos();
#endif
}

inline bool isFontFamilyAvailable(const QString &family)
{
    if (family.isEmpty()) {
        return false;
    }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    return QFontDatabase::families().contains(family, Qt::CaseInsensitive);
#else
    const QFontDatabase fontDatabase;
    return fontDatabase.families().contains(family, Qt::CaseInsensitive);
#endif
}

inline bool tabBarIsTabVisible(QTabBar *tabBar, int index)
{
    if (tabBar == Q_NULLPTR || index < 0 || index >= tabBar->count()) {
        return false;
    }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    return tabBar->isTabVisible(index);
#else
    const QObject *stateOwner = detail::tabVisibilityStateOwner(tabBar);
    const QWidget *pageWidget = detail::tabPageWidget(tabBar, index);
    const QString pageStateKey = detail::tabPageStateKey(pageWidget);
    if (stateOwner == Q_NULLPTR || pageStateKey.isEmpty()) {
        return true;
    }

    const QVariantMap visibilityState = detail::propertyMap(stateOwner, detail::tabVisibilityStatePropertyName());
    const QVariantMap::const_iterator stateIt = visibilityState.constFind(pageStateKey);
    return (stateIt == visibilityState.constEnd()) ? true : stateIt.value().toBool();
#endif
}

inline void tabBarSetTabVisible(QTabBar *tabBar, int index, bool visible)
{
    if (tabBar == Q_NULLPTR || index < 0 || index >= tabBar->count()) {
        return;
    }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    tabBar->setTabVisible(index, visible);
#else
    QObject *stateOwner = detail::tabVisibilityStateOwner(tabBar);
    QWidget *pageWidget = detail::tabPageWidget(tabBar, index);
    const QString pageStateKey = detail::tabPageStateKey(pageWidget);
    if (stateOwner == Q_NULLPTR || pageStateKey.isEmpty()) {
        return;
    }

    QVariantMap visibilityState = detail::propertyMap(stateOwner, detail::tabVisibilityStatePropertyName());
    QVariantMap enabledBeforeHideState = detail::propertyMap(stateOwner, detail::tabEnabledBeforeHideStatePropertyName());
    const bool wasVisible = !visibilityState.contains(pageStateKey) || visibilityState.value(pageStateKey).toBool();

    visibilityState.insert(pageStateKey, visible);
    stateOwner->setProperty(detail::tabVisibilityStatePropertyName(), visibilityState);

    if (!visible) {
        if (wasVisible && !enabledBeforeHideState.contains(pageStateKey)) {
            enabledBeforeHideState.insert(pageStateKey, tabBar->isTabEnabled(index));
        }

        stateOwner->setProperty(detail::tabEnabledBeforeHideStatePropertyName(), enabledBeforeHideState);
        detail::setTabEnabledCompat(tabBar, index, false);
        return;
    }

    const bool restoreEnabled = enabledBeforeHideState.contains(pageStateKey)
        ? enabledBeforeHideState.value(pageStateKey).toBool()
        : tabBar->isTabEnabled(index);
    enabledBeforeHideState.remove(pageStateKey);
    stateOwner->setProperty(detail::tabEnabledBeforeHideStatePropertyName(), enabledBeforeHideState);
    detail::setTabEnabledCompat(tabBar, index, restoreEnabled);
#endif
}

} // namespace QKeyMapperQtCompat

#endif // QKEYMAPPER_QT_COMPAT_H
