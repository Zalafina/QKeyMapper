#include "qstyle_singletons.h"

#include <QApplication>
#include <QMutex>
#include <QPointer>
#include <QStyleFactory>

namespace QKeyMapperStyle {

static QStyle *ensureStyle(QPointer<QStyle> &cached, const char *key)
{
    static QMutex s_styleMutex;
    QMutexLocker locker(&s_styleMutex);

    if (cached) {
        return cached;
    }

    QStyle *style = QStyleFactory::create(QString::fromLatin1(key));
    if (!style) {
        return nullptr;
    }

    // Own the style for the full application lifetime.
    // QWidget::setStyle() does not take ownership, so without an owner this would leak.
    style->setParent(qApp);
    cached = style;
    return style;
}

QStyle *windowsStyle()
{
    static QPointer<QStyle> s_windowsStyle;
    return ensureStyle(s_windowsStyle, "Windows");
}

QStyle *fusionStyle()
{
    static QPointer<QStyle> s_fusionStyle;
    return ensureStyle(s_fusionStyle, "Fusion");
}

} // namespace QKeyMapperStyle
