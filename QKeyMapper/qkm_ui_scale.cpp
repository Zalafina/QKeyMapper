#include "qkm_ui_scale.h"
#include "qkeymapper_qt_compat.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QEvent>
#include <QGridLayout>
#include <QHeaderView>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QProxyStyle>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <QSet>
#include <QSpinBox>
#include <QSplitter>
#include <QStyleFactory>
#include <QStyleOption>
#include <QStringView>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <qt_windows.h>
#include <QtMath>
#include <vector>
#include <utility>
#include <algorithm>

namespace {
int scaled(int value, qreal ratio)
{
    return value <= 0 || value >= QWIDGETSIZE_MAX ? value : qMax(1, qRound(value * ratio));
}

QSize scaledSize(QSize size, qreal ratio)
{
    return QSize(scaled(size.width(), ratio), scaled(size.height(), ratio));
}

QMargins scaledMargins(const QMargins &m, qreal ratio)
{
    return QMargins(scaled(m.left(), ratio), scaled(m.top(), ratio),
                    scaled(m.right(), ratio), scaled(m.bottom(), ratio));
}

QFont scaledFont(QFont font, qreal ratio)
{
    if (font.pointSizeF() > 0) {
        font.setPointSizeF(font.pointSizeF() * ratio);
    } else if (font.pixelSize() > 0) {
        font.setPixelSize(scaled(font.pixelSize(), ratio));
    }
    return font;
}

QString scaledStyleSheet(const QString &sheet, qreal ratio)
{
    if (qFuzzyCompare(ratio, qreal(1))) { return sheet; }
    static const QRegularExpression dimensions(QStringLiteral("((?:[\\w-]*width|[\\w-]*height|padding[\\w-]*|margin[\\w-]*|border[\\w-]*)\\s*:[^;{}]*);"));
    static const QRegularExpression pixels(QStringLiteral("(-?\\d+)px"));
    QString adjusted;
    int last = 0;
    auto matches = dimensions.globalMatch(sheet);
    while (matches.hasNext()) {
        const auto match = matches.next();
        QKeyMapperQtCompat::appendStringSlice(adjusted, sheet, last, match.capturedStart() - last);
        const QString declaration = match.captured();
        int offset = 0;
        auto values = pixels.globalMatch(declaration);
        while (values.hasNext()) {
            const auto value = values.next();
            QKeyMapperQtCompat::appendStringSlice(adjusted, declaration, offset, value.capturedStart() - offset);
            adjusted += QString::number(qRound(value.captured(1).toInt() * ratio)) + QStringLiteral("px");
            offset = value.capturedEnd();
        }
        QKeyMapperQtCompat::appendStringSlice(adjusted, declaration, offset);
        last = match.capturedEnd();
    }
    QKeyMapperQtCompat::appendStringSlice(adjusted, sheet, last);
    return adjusted;
}

#ifdef DEBUG_LOGOUT_ON
void logSpinEditor(const QWidget *widget, quint64 sequence, qreal ratio, const char *phase)
{
    const auto *spin = qobject_cast<const QAbstractSpinBox *>(widget);
    if (!spin && qobject_cast<const QLineEdit *>(widget)) {
        spin = qobject_cast<const QAbstractSpinBox *>(widget->parentWidget());
    }
    if (!spin) { return; }
    const auto *edit = spin->findChild<QLineEdit *>(QStringLiteral("qt_spinbox_lineedit"),
                                                  Qt::FindDirectChildrenOnly);
    if (!edit) { return; }
    QStyleOptionSpinBox option;
    option.initFrom(spin);
    option.frame = spin->hasFrame();
    option.buttonSymbols = spin->buttonSymbols();
    option.subControls = QStyle::SC_SpinBoxEditField;
    const QRect expected = spin->style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                         QStyle::SC_SpinBoxEditField, spin);
    qDebug() << "[UI_SCALE] SPIN_EDITOR qkmSeq=" << sequence << "phase=" << phase
             << "R=" << ratio << "spin=" << spin->objectName() << "target=" << widget->objectName()
             << "size=" << spin->size() << "minimum=" << spin->minimumSize()
             << "maximum=" << spin->maximumSize() << "editor=" << edit->geometry()
             << "expected=" << expected << "matches=" << (edit->geometry() == expected)
             << "editorMinimum=" << edit->minimumSize() << "editorMaximum=" << edit->maximumSize()
             << "editorHint=" << edit->sizeHint() << "editorMinHint=" << edit->minimumSizeHint()
             << "fontPt=" << spin->font().pointSizeF() << "editorFontPt=" << edit->font().pointSizeF()
             << "fontHeight=" << edit->fontMetrics().height() << "textMargins=" << edit->textMargins()
             << "alignment=" << edit->alignment() << "DPR=" << spin->devicePixelRatioF()
             << "style=" << spin->style()->metaObject()->className();
}
#endif

// Normalize style queries and painting together, including styles with hard-coded
// button/arrow sizes. The original style is restored at ratio 1.
class ScaleStyle final : public QProxyStyle
{
public:
    ScaleStyle(QStyle *base, QObject *owner) : QProxyStyle(base) { setParent(owner); }
    qreal ratio = 1;
    int contentWidth = 0;

    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override
    {
        if (delegating) { return QProxyStyle::pixelMetric(metric, option, widget); }
        QScopedValueRollback<bool> guard(delegating, true);
        return scaled(QProxyStyle::pixelMetric(metric, option, widget), ratio);
    }

    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &contents, const QWidget *widget) const override
    {
        if (delegating || !option) {
            return QProxyStyle::sizeFromContents(type, option, contents, widget);
        }
        QScopedValueRollback<bool> guard(delegating, true);
        QSize result;
        visit(option, widget, [&](const QStyleOption *base) {
            QSize normalized = scaledSize(contents, 1 / ratio);
            if (type == CT_ComboBox && contents.height() > 0) {
                // QComboBox adds a 14px floor and 2px padding before consulting
                // the style. Recompute those in baseline units, not twice scaled.
                normalized.setHeight(qMax(base->fontMetrics.height(), 14) + 2);
                if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(base)) {
                    normalized.setHeight(qMax(normalized.height(), combo->iconSize.height() + 2));
                }
            }
            result = QProxyStyle::sizeFromContents(type, base, normalized, widget);
#ifdef DEBUG_LOGOUT_ON
            if (widget && ratio < 1 && (type == CT_ComboBox || type == CT_SpinBox)
                && (widget->objectName() == QStringLiteral("waitTimeSpinBox")
                || widget->objectName() == QStringLiteral("keyPressTypeComboBox"))) {
                qDebug() << "[UI_SCALE] CONTENTS" << widget->objectName() << type
                         << "input=" << contents << "baseline=" << normalized << "result=" << result
                         << "font=" << widget->font().pointSizeF() << "baselineFontHeight=" << base->fontMetrics.height();
            }
#endif
        });
        QSize adjusted = scaledSize(result, ratio);
        if (contentWidth > 0 && (type == CT_SpinBox || type == CT_ComboBox)) {
            adjusted.setWidth(contentWidth);
        }
        return adjusted;
    }

    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                         SubControl sub, const QWidget *widget) const override
    {
        if (delegating || !option) {
            return QProxyStyle::subControlRect(control, option, sub, widget);
        }
        QScopedValueRollback<bool> guard(delegating, true);
        QRect result;
        visit(option, widget, [&](const QStyleOption *base) {
            result = QProxyStyle::subControlRect(control, static_cast<const QStyleOptionComplex *>(base), sub, widget);
        });
        return restoreRect(result, option->rect.topLeft());
    }

    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const override
    {
        if (delegating || !option) {
            return QProxyStyle::subElementRect(element, option, widget);
        }
        QScopedValueRollback<bool> guard(delegating, true);
        QRect result;
        visit(option, widget, [&](const QStyleOption *base) {
            result = QProxyStyle::subElementRect(element, base, widget);
        });
        return restoreRect(result, option->rect.topLeft());
    }

    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                     const QPoint &point, const QWidget *widget) const override
    {
        if (delegating || !option) {
            return QProxyStyle::hitTestComplexControl(control, option, point, widget);
        }
        QScopedValueRollback<bool> guard(delegating, true);
        SubControl result = SC_None;
        const QPoint p = point - option->rect.topLeft();
        visit(option, widget, [&](const QStyleOption *base) {
            result = QProxyStyle::hitTestComplexControl(control, static_cast<const QStyleOptionComplex *>(base),
                                                       QPoint(qRound(p.x() / ratio), qRound(p.y() / ratio)), widget);
        });
        return result;
    }

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override
    {
        paint(option, painter, widget, [&](const QStyleOption *base, QPainter *p) {
            QProxyStyle::drawPrimitive(element, base, p, widget);
        });
    }

    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override
    {
        paint(option, painter, widget, [&](const QStyleOption *base, QPainter *p) {
            QProxyStyle::drawControl(element, base, p, widget);
        });
    }

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override
    {
        paint(option, painter, widget, [&](const QStyleOption *base, QPainter *p) {
            QProxyStyle::drawComplexControl(control, static_cast<const QStyleOptionComplex *>(base), p, widget);
        });
    }

private:
    mutable bool delegating = false;

    QRect restoreRect(const QRect &rect, QPoint origin) const
    {
        return QRect(origin + QPoint(qRound(rect.x() * ratio), qRound(rect.y() * ratio)),
                     scaledSize(rect.size(), ratio));
    }

    template<class Option, class Fn>
    void copyOption(const QStyleOption *option, const QWidget *widget, Fn fn) const
    {
        // A newer payload must not retain its version in a smaller sliced copy.
        if (option->version > Option::Version) { fn(option); return; }
        Option copy = *static_cast<const Option *>(option);
        copy.rect = QRect(QPoint(), scaledSize(option->rect.size(), 1 / ratio));
        copy.fontMetrics = QFontMetrics(scaledFont(widget ? widget->font() : QApplication::font(), 1 / ratio));
        normalize(copy);
        fn(&copy);
    }

    void normalize(QStyleOption &) const {}
    void normalize(QStyleOptionButton &o) const { o.iconSize = scaledSize(o.iconSize, 1 / ratio); }
    void normalize(QStyleOptionComboBox &o) const { o.iconSize = scaledSize(o.iconSize, 1 / ratio); }
    void normalize(QStyleOptionToolButton &o) const
    {
        o.iconSize = scaledSize(o.iconSize, 1 / ratio);
        o.font = scaledFont(o.font, 1 / ratio);
    }
    void normalize(QStyleOptionViewItem &o) const
    {
        o.decorationSize = scaledSize(o.decorationSize, 1 / ratio);
        o.font = scaledFont(o.font, 1 / ratio);
        o.fontMetrics = QFontMetrics(o.font);
    }
    void normalize(QStyleOptionMenuItem &o) const
    {
        o.font = scaledFont(o.font, 1 / ratio);
        o.maxIconWidth = scaled(o.maxIconWidth, 1 / ratio);
        QKeyMapperQtCompat::scaleMenuShortcutWidth(o, 1 / ratio);
    }
    void normalize(QStyleOptionTab &o) const
    {
        o.iconSize = scaledSize(o.iconSize, 1 / ratio);
        o.leftButtonSize = scaledSize(o.leftButtonSize, 1 / ratio);
        o.rightButtonSize = scaledSize(o.rightButtonSize, 1 / ratio);
    }
    void normalize(QStyleOptionFrame &o) const
    {
        o.lineWidth = scaled(o.lineWidth, 1 / ratio);
        o.midLineWidth = scaled(o.midLineWidth, 1 / ratio);
    }
    void normalize(QStyleOptionTabWidgetFrame &o) const
    {
        o.lineWidth = scaled(o.lineWidth, 1 / ratio);
        o.midLineWidth = scaled(o.midLineWidth, 1 / ratio);
        o.tabBarSize = scaledSize(o.tabBarSize, 1 / ratio);
        o.rightCornerWidgetSize = scaledSize(o.rightCornerWidgetSize, 1 / ratio);
        o.leftCornerWidgetSize = scaledSize(o.leftCornerWidgetSize, 1 / ratio);
        o.tabBarRect = QRect(o.tabBarRect.topLeft() / ratio, scaledSize(o.tabBarRect.size(), 1 / ratio));
        o.selectedTabRect = QRect(o.selectedTabRect.topLeft() / ratio, scaledSize(o.selectedTabRect.size(), 1 / ratio));
    }
    void normalize(QStyleOptionTabBarBase &o) const
    {
        o.tabBarRect = QRect(o.tabBarRect.topLeft() / ratio, scaledSize(o.tabBarRect.size(), 1 / ratio));
        o.selectedTabRect = QRect(o.selectedTabRect.topLeft() / ratio, scaledSize(o.selectedTabRect.size(), 1 / ratio));
    }

    template<class Fn>
    void visit(const QStyleOption *o, const QWidget *w, Fn fn) const
    {
        switch (o->type) {
        case QStyleOptionButton::Type: copyOption<QStyleOptionButton>(o, w, fn); break;
        case QStyleOptionToolButton::Type: copyOption<QStyleOptionToolButton>(o, w, fn); break;
        case QStyleOptionComboBox::Type: copyOption<QStyleOptionComboBox>(o, w, fn); break;
        case QStyleOptionSpinBox::Type: copyOption<QStyleOptionSpinBox>(o, w, fn); break;
        case QStyleOptionSlider::Type: copyOption<QStyleOptionSlider>(o, w, fn); break;
        case QStyleOptionViewItem::Type: copyOption<QStyleOptionViewItem>(o, w, fn); break;
        case QStyleOptionMenuItem::Type: copyOption<QStyleOptionMenuItem>(o, w, fn); break;
        case QStyleOptionHeader::Type:
            if (o->version >= QKeyMapperQtCompat::UiScaleHeaderOption::Version) {
                copyOption<QKeyMapperQtCompat::UiScaleHeaderOption>(o, w, fn);
            } else {
                copyOption<QStyleOptionHeader>(o, w, fn);
            }
            break;
        case QStyleOptionTab::Type: copyOption<QStyleOptionTab>(o, w, fn); break;
        case QStyleOptionTabWidgetFrame::Type: copyOption<QStyleOptionTabWidgetFrame>(o, w, fn); break;
        case QStyleOptionTabBarBase::Type: copyOption<QStyleOptionTabBarBase>(o, w, fn); break;
        case QStyleOptionFrame::Type: copyOption<QStyleOptionFrame>(o, w, fn); break;
        case QStyleOptionFocusRect::Type: copyOption<QStyleOptionFocusRect>(o, w, fn); break;
        case QStyleOptionGroupBox::Type: copyOption<QStyleOptionGroupBox>(o, w, fn); break;
        case QStyleOptionProgressBar::Type: copyOption<QStyleOptionProgressBar>(o, w, fn); break;
        case QStyleOptionRubberBand::Type: copyOption<QStyleOptionRubberBand>(o, w, fn); break;
        case QStyleOptionToolBox::Type: copyOption<QStyleOptionToolBox>(o, w, fn); break;
        case QStyleOptionSizeGrip::Type: copyOption<QStyleOptionSizeGrip>(o, w, fn); break;
        case QStyleOptionDockWidget::Type: copyOption<QStyleOptionDockWidget>(o, w, fn); break;
        case QStyleOptionTitleBar::Type: copyOption<QStyleOptionTitleBar>(o, w, fn); break;
        case QStyleOption::Type: copyOption<QStyleOption>(o, w, fn); break;
        case QStyleOptionComplex::Type: copyOption<QStyleOptionComplex>(o, w, fn); break;
        default: fn(o); break;
        }
    }

    template<class Fn>
    void paint(const QStyleOption *option, QPainter *painter, const QWidget *widget, Fn fn) const
    {
        if (delegating || !option) {
            fn(option, painter);
            return;
        }
        if (option->rect.isEmpty()) { return; }
        QScopedValueRollback<bool> guard(delegating, true);
        // Keep glyph rasterization on the final paint device. An intermediate
        // transparent pixmap changes text blending and pixel alignment.
        painter->save();
        painter->setClipRect(option->rect, Qt::IntersectClip);
        painter->translate(option->rect.topLeft());
        painter->scale(ratio, ratio);
        painter->setFont(scaledFont(widget ? widget->font() : painter->font(), 1 / ratio));
        visit(option, widget, [&](const QStyleOption *base) { fn(base, painter); });
        painter->restore();
    }
};
}

struct QkmUiScale::Data
{
    struct Widget {
        QPointer<QWidget> target;
        QSize minimum, maximum, icon;
        QSize naturalMinimum;
        QSizePolicy policy;
        QMargins margins;
        QFont font;
        QPointer<QStyle> original;
        QPointer<ScaleStyle> proxy;
        int preferredHeight = 0;
        int defaultHeight = 0;
        int preferredWidth = 0;
        int spinAffixWidth = 0;
        int splitterWidth = 0;
        int headerMinimum = -1;
        int headerDefault = -1;
        std::vector<int> fixedColumns;
        QPixmap authoredPixmap;
        qint64 appliedPixmapKey = 0;
        QString sheet;
        bool hasSheetMetrics = false;
        bool explicitStyle = false;
        bool spinEditor = false;
    };
    struct Layout {
        QPointer<QLayout> target;
        QMargins margins;
        int spacing, horizontal, vertical;
        std::vector<QSize> spacers;
    };
    QPointer<QWidget> window;
    QPointer<QTabWidget> settingsTabs;
    std::vector<Widget> widgets;
    std::vector<Layout> layouts;
    std::function<QStyle *(QStyle *, QWidget *)> factory;
    std::function<QStyle *(QWidget *)> resolver;
    QString theme;
    qreal ratio = 1;
    bool handlingPopup = false;
    bool restoredDefaultHints = false;
    bool managedWindow = false;
    bool applying = false;
    bool userResizing = false;
    bool scaledWindow = false;
    int sizeMode = 0;
    QSizeF normalSizes[2];
    QTimer settleTimer;
    QSet<int> pendingDefaultPages;
#ifdef DEBUG_LOGOUT_ON
    quint64 sequence = 0;
#endif

    bool owned(const QWidget *w) const
    {
        for (const QWidget *parent = w; parent; parent = parent->parentWidget()) {
            if (parent == window) { return true; }
            if (parent->isWindow() && parent->windowType() != Qt::Popup) { return false; }
        }
        return false;
    }
};

QkmUiScale::QkmUiScale(QWidget *window) : QObject(window), d(new Data)
{
    d->window = window;
}
QkmUiScale::~QkmUiScale()
{
    if (qApp) {
        qApp->removeEventFilter(this);
        qApp->removeNativeEventFilter(this);
    }
    if (d->window) { d->window->removeEventFilter(this); }
    d->applying = true;
    // A manager can be retired before its still-visible editor is destroyed.
    for (const auto &s : d->widgets) {
        if (s.target && s.proxy) { s.target->setStyle(s.explicitStyle ? s.original.data() : nullptr); }
    }
}

void QkmUiScale::manageWindow()
{
    if (!d->window || d->managedWindow) { return; }
    d->managedWindow = true;
    d->normalSizes[0] = d->window->size();
    capture();
    d->window->installEventFilter(this);
    d->settleTimer.setSingleShot(true);
    connect(&d->settleTimer, &QTimer::timeout, this, [this]() {
        if (!d->window) { return; }
        if (!d->scaledWindow) {
            d->normalSizes[d->sizeMode] = d->window->size();
            return;
        }
        applyWindow(d->ratio);
    });
    if (qApp) { qApp->installNativeEventFilter(this); }
}

qreal QkmUiScale::ratio() const { return d->ratio; }
bool QkmUiScale::isApplying() const { return d->applying; }

void QkmUiScale::setWindowSizeMode(int mode)
{
    Q_ASSERT(mode == 0 || mode == 1);
    if (mode < 0 || mode > 1 || !d->window) { return; }
    d->sizeMode = mode;
    // The floating-button editor derives height from its current layout, not a fixed root floor.
    for (auto &s : d->widgets) { if (s.target == d->window) { s.minimum.setHeight(0); break; } }
    if (d->normalSizes[mode].isEmpty()) {
        d->normalSizes[mode] = QSizeF(d->window->size()) / d->ratio;
    }
}

bool QkmUiScale::nativeEventFilter(const QByteArray &eventType, void *message,
                                 QKeyMapperQtCompat::NativeEventResult *result)
{
    Q_UNUSED(result);
    if (eventType != "windows_generic_MSG" || !d->window || !d->window->effectiveWinId()) { return false; }
    const auto *msg = static_cast<const MSG *>(message);
    if (!msg || msg->hwnd != reinterpret_cast<HWND>(d->window->effectiveWinId())) { return false; }
    if (msg->message == WM_ENTERSIZEMOVE) { d->userResizing = true; }
    else if (msg->message == WM_EXITSIZEMOVE && d->userResizing) {
        d->userResizing = false;
        const qreal exitRatio = d->ratio;
        if (!d->window->isMaximized() && !d->window->isMinimized()) {
            d->normalSizes[d->sizeMode] = QSizeF(d->window->size()) / exitRatio;
        }
        // The dialog's native handler may finish its layout after this filter.
        QTimer::singleShot(0, this, [this, exitRatio]() {
            if (d->window && !d->applying && qFuzzyCompare(d->ratio, exitRatio)
                && !d->window->isMaximized() && !d->window->isMinimized()) {
                d->normalSizes[d->sizeMode] = QSizeF(d->window->size()) / d->ratio;
            }
        });
    }
    return false;
}

void QkmUiScale::applyWindow(qreal ratio)
{
    if (!d->managedWindow || !d->window || ratio <= 0 || d->applying) { return; }
    if (!d->scaledWindow && qFuzzyCompare(ratio, qreal(1))) { return; }
    d->scaledWindow = true;
    QScopedValueRollback<bool> guard(d->applying, true);
    capture();
    apply(ratio);
    for (const auto &s : d->widgets) {
        if (s.target != d->window) { continue; }
        d->window->setMinimumSize(scaledSize(s.minimum, ratio));
        d->window->setMaximumSize(scaledSize(s.maximum, ratio));
        d->window->setContentsMargins(scaledMargins(s.margins, ratio));
        break;
    }
    refreshLayouts();
    if (!d->window->isMaximized() && !d->window->isMinimized()) {
        const QSize target = (d->normalSizes[d->sizeMode] * ratio).toSize()
            .expandedTo(d->window->minimumSizeHint()).expandedTo(d->window->minimumSize());
        d->window->resize(target);
    }
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[UI_SCALE] EDITOR root=" << d->window->metaObject()->className()
             << "R=" << ratio << "mode=" << d->sizeMode
             << "base=" << d->normalSizes[d->sizeMode] << "size=" << d->window->size();
#endif
}

bool QkmUiScale::eventFilter(QObject *object, QEvent *event)
{
    if (d->managedWindow && object == d->window && event->type() == QEvent::Show) {
        d->settleTimer.start(0);
    }
    if (d->managedWindow && !d->applying && event->type() == QEvent::Paint
        && !qFuzzyCompare(d->ratio, qreal(1)) && qobject_cast<QLabel *>(object)) {
        for (auto &s : d->widgets) {
            if (s.target != object) { continue; }
            auto *label = static_cast<QLabel *>(object);
            const QPixmap current = QKeyMapperQtCompat::labelPixmap(label);
            if (!current.isNull() && current.cacheKey() != s.appliedPixmapKey) {
                // A newly selected image is authored content, even at a non-default R.
                s.authoredPixmap = current;
                const QPixmap adjusted = current.scaled(scaledSize(current.size(), d->ratio),
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
                s.appliedPixmapKey = adjusted.cacheKey();
                label->setPixmap(adjusted);
            }
            break;
        }
    }
    if (!d->handlingPopup && event->type() == QEvent::Polish && !qFuzzyCompare(d->ratio, qreal(1))) {
        auto *popup = qobject_cast<QWidget *>(object);
        if (popup && popup->isWindow() && popup->windowType() == Qt::Popup && d->owned(popup)) {
            QScopedValueRollback<bool> guard(d->handlingPopup, true);
            capture();
            apply(d->ratio, popup);
        }
    }
    return QObject::eventFilter(object, event);
}

QSize QkmUiScale::defaultWindowMinimum() const
{
    for (const auto &s : d->widgets) {
        if (s.target == d->window) { return s.minimum; }
    }
    return QSize();
}

void QkmUiScale::setStyleFactory(std::function<QStyle *(QStyle *, QWidget *)> factory)
{
    d->factory = std::move(factory);
}

void QkmUiScale::setStyleResolver(std::function<QStyle *(QWidget *)> resolver)
{
    d->resolver = std::move(resolver);
}

void QkmUiScale::capture(QWidget *authoredRoot)
{
    if (!d->window) { return; }
    // Transient menus may be created on the stack. Discard their expired
    // baselines and private styles before capturing the next popup.
    d->widgets.erase(std::remove_if(d->widgets.begin(), d->widgets.end(), [](Data::Widget &s) {
        if (s.target) { return false; }
        delete s.proxy.data();
        return true;
    }), d->widgets.end());
    d->layouts.erase(std::remove_if(d->layouts.begin(), d->layouts.end(), [](const Data::Layout &s) {
        return s.target.isNull();
    }), d->layouts.end());
    QSet<QWidget *> known;
    for (const auto &s : d->widgets) { if (s.target) { known.insert(s.target); } }
    auto all = d->window->findChildren<QWidget *>();
    all.prepend(d->window);
    for (QWidget *w : std::as_const(all)) {
        if (!d->owned(w) || known.contains(w)) { continue; }
        Data::Widget s;
        s.target = w;
        s.minimum = w->minimumSize();
        s.maximum = w->maximumSize();
        s.policy = w->sizePolicy();
        s.naturalMinimum = w->minimumSizeHint();
        if (w->objectName() == QStringLiteral("settingTabWidget") && !d->settingsTabs) {
            d->settingsTabs = qobject_cast<QTabWidget *>(w);
            connect(d->settingsTabs, &QTabWidget::currentChanged, this, [this](int index) {
                if (index < 0 || !d->settingsTabs || !d->pendingDefaultPages.contains(index)
                    || !qFuzzyCompare(d->ratio, qreal(1))) { return; }
                QWidget *page = d->settingsTabs->widget(index);
                for (const auto &baseline : d->widgets) {
                    QWidget *target = baseline.target;
                    if (target && (target == d->settingsTabs || (page && page->isAncestorOf(target)))) {
                        target->setMinimumSize(baseline.minimum);
                        target->setMaximumSize(baseline.maximum);
                        target->updateGeometry();
                    }
                }
                refreshLayouts();
                d->pendingDefaultPages.remove(index);
                d->restoredDefaultHints = false;
            });
        }
        s.margins = w->contentsMargins();
        const bool authored = authoredRoot && (w == authoredRoot || authoredRoot->isAncestorOf(w));
        const QWidget *popupWindow = w->window();
        const bool applicationMenuFont = qobject_cast<const QMenu *>(popupWindow)
            && !popupWindow->testAttribute(Qt::WA_SetFont)
            && !popupWindow->testAttribute(Qt::WA_WindowPropagation);
        s.font = scaledFont(w->font(), authored || applicationMenuFont ? 1 : 1 / d->ratio);
        s.original = d->resolver ? d->resolver(w) : w->style();
        s.explicitStyle = w->testAttribute(Qt::WA_SetStyle);
        s.sheet = w->styleSheet();
        s.hasSheetMetrics = scaledStyleSheet(s.sheet, qreal(0.5)) != s.sheet;
        if (auto *button = qobject_cast<QAbstractButton *>(w)) { s.icon = button->iconSize(); }
        if (auto *combo = qobject_cast<QComboBox *>(w)) { s.icon = combo->iconSize(); }
        if (auto *view = qobject_cast<QAbstractItemView *>(w)) { s.icon = view->iconSize(); }
        if (auto *splitter = qobject_cast<QSplitter *>(w)) { s.splitterWidth = splitter->handleWidth(); }
        if (auto *header = qobject_cast<QHeaderView *>(w)) {
            s.headerMinimum = header->minimumSectionSize();
            s.headerDefault = header->defaultSectionSize();
            if (d->managedWindow && header->orientation() == Qt::Horizontal) {
                for (int section = 0; section < header->count(); ++section) {
                    if (header->sectionResizeMode(section) == QHeaderView::Fixed) {
                        s.headerMinimum = qMin(s.headerMinimum, header->sectionSize(section));
                    }
                }
            }
        }
        if (d->managedWindow) {
            if (auto *table = qobject_cast<QTableWidget *>(w)) {
                for (int column = 0; column < table->columnCount(); ++column) {
                    s.fixedColumns.push_back(table->horizontalHeader()->sectionResizeMode(column) == QHeaderView::Fixed
                        ? table->columnWidth(column) : -1);
                }
            }
        }
        s.spinEditor = qobject_cast<QLineEdit *>(w)
            && qobject_cast<QAbstractSpinBox *>(w->parentWidget());
        // A spin box owns its editor geometry. Its size hint is not an
        // independent height constraint and must not outlive a scale change.
        if ((qobject_cast<QLineEdit *>(w) && !s.spinEditor) || qobject_cast<QAbstractSpinBox *>(w)) {
            if (w->window() == d->window && s.minimum.height() != s.maximum.height()) {
                s.preferredHeight = w->sizeHint().height();
                s.defaultHeight = s.preferredHeight;
            }
        }
        if (auto *spin = qobject_cast<QAbstractSpinBox *>(w)) {
            if (w->window() == d->window && s.minimum.width() != s.maximum.width()) {
                s.preferredWidth = w->sizeHint().width();
                if (auto *integer = qobject_cast<QSpinBox *>(spin)) {
                    s.spinAffixWidth = QFontMetrics(s.font).horizontalAdvance(integer->prefix() + integer->suffix());
                } else if (auto *decimal = qobject_cast<QDoubleSpinBox *>(spin)) {
                    s.spinAffixWidth = QFontMetrics(s.font).horizontalAdvance(decimal->prefix() + decimal->suffix());
                }
            }
        } else if (auto *combo = qobject_cast<QComboBox *>(w)) {
            if (combo->sizeAdjustPolicy() == QComboBox::AdjustToMinimumContentsLengthWithIcon
                && s.minimum.width() != s.maximum.width()) {
                s.preferredWidth = s.naturalMinimum.width();
            }
        }
        d->widgets.push_back(s);
#ifdef DEBUG_LOGOUT_ON
        logSpinEditor(w, d->sequence, d->ratio, "CAPTURE");
#endif
    }
    QSet<QLayout *> knownLayouts;
    for (const auto &s : d->layouts) { if (s.target) { knownLayouts.insert(s.target); } }
    const auto layouts = d->window->findChildren<QLayout *>();
    for (QLayout *layout : layouts) {
        if (knownLayouts.contains(layout) || !layout->parentWidget() ||
            (!d->managedWindow && layout->parentWidget() == d->window) || !d->owned(layout->parentWidget())) { continue; }
        Data::Layout s;
        s.target = layout;
        s.margins = layout->contentsMargins();
        s.spacing = layout->spacing();
        const auto *grid = qobject_cast<QGridLayout *>(layout);
        s.horizontal = grid ? grid->horizontalSpacing() : -1;
        s.vertical = grid ? grid->verticalSpacing() : -1;
        for (int i = 0; i < layout->count(); ++i) {
            QSpacerItem *spacer = layout->itemAt(i)->spacerItem();
            s.spacers.push_back(spacer ? spacer->sizeHint() : QSize(-1, -1));
        }
        d->layouts.push_back(s);
    }
}

void QkmUiScale::apply(qreal ratio, QWidget *subtree)
{
    if (!d->window || ratio <= 0) { return; }
#ifdef DEBUG_LOGOUT_ON
    ++d->sequence;
#endif
    // Style changes may synchronously polish popup children. Keep the baseline
    // registry stable until the current traversal has finished.
    QScopedValueRollback<bool> popupGuard(d->handlingPopup, true);
    if (!subtree && qFuzzyCompare(d->ratio, qreal(1)) && !qFuzzyCompare(ratio, qreal(1))
        && !d->restoredDefaultHints) {
        // Font/constraint baselines are immutable. The default layout cache also
        // depends on which pages the user has visited; preserve that presentation.
        for (auto &s : d->widgets) {
            QWidget *w = s.target;
            if (!w) { continue; }
            if (w == d->settingsTabs) { s.naturalMinimum = w->minimumSizeHint(); }
            if (s.preferredHeight > 0 && d->settingsTabs && d->settingsTabs->isAncestorOf(w)
                && !w->isVisible() && w->parentWidget()->layout()
                && !w->parentWidget()->layout()->geometry().isEmpty()) { s.defaultHeight = w->height(); }
        }
    }
    d->ratio = ratio;
    const bool original = qFuzzyCompare(ratio, qreal(1));
    if (!subtree) {
        d->restoredDefaultHints = original;
        d->pendingDefaultPages.clear();
        if (original && d->settingsTabs) {
            for (int i = 0; i < d->settingsTabs->count(); ++i) { d->pendingDefaultPages.insert(i); }
        }
    }
    if (!subtree && qApp) {
        if (original) { qApp->removeEventFilter(this); }
        else { qApp->installEventFilter(this); }
    }
    // Prepare child constraints before native style/resize handling lays out
    // the parent spin box. Never impose a synthetic height on these editors.
    for (const auto &s : d->widgets) {
        QWidget *w = s.target;
        if (!w || !s.spinEditor) { continue; }
        if (subtree && w != subtree && !subtree->isAncestorOf(w)) { continue; }
        w->setMinimumSize(scaledSize(s.minimum, ratio));
        w->setMaximumSize(scaledSize(s.maximum, ratio));
#ifdef DEBUG_LOGOUT_ON
        logSpinEditor(w, d->sequence, ratio, "EDITOR_CONSTRAINTS_READY");
#endif
    }
    if (!subtree) { refreshTheme(); }
    for (auto &s : d->widgets) {
        QWidget *w = s.target;
        if (!w) { continue; }
        if (subtree && w != subtree && !subtree->isAncestorOf(w)) { continue; }
#ifdef DEBUG_LOGOUT_ON
        logSpinEditor(w, d->sequence, ratio, "BEFORE_PROPERTIES");
#endif
        w->setFont(scaledFont(s.font, ratio));
        if (w != d->window) {
            // Local menu/header dimensions override inherited theme rules. Scale
            // their authored templates too, while leaving dynamic color rules alone.
            if (s.hasSheetMetrics) {
                const QString localSheet = scaledStyleSheet(s.sheet, ratio);
                if (w->styleSheet() != localSheet) { w->setStyleSheet(localSheet); }
            }
            if (!original && !s.proxy) {
                QStyle *base = d->factory ? d->factory(s.original, w) : nullptr;
                if (!base) {
                    const QString name = s.original ? s.original->objectName() : QString();
                    base = QStyleFactory::create(name.compare(QStringLiteral("windows"), Qt::CaseInsensitive) == 0
                                                ? QStringLiteral("Windows") : QStringLiteral("Fusion"));
                }
                s.proxy = new ScaleStyle(base, this);
            }
            int preferredWidth = 0;
            if (!original && s.preferredWidth > 0) {
                int affixWidth = 0;
                if (auto *integer = qobject_cast<QSpinBox *>(w)) {
                    affixWidth = QFontMetrics(s.font).horizontalAdvance(integer->prefix() + integer->suffix());
                } else if (auto *decimal = qobject_cast<QDoubleSpinBox *>(w)) {
                    affixWidth = QFontMetrics(s.font).horizontalAdvance(decimal->prefix() + decimal->suffix());
                }
                preferredWidth = qMax(scaled(s.minimum.width(), ratio),
                                     scaled(s.preferredWidth + affixWidth - s.spinAffixWidth, ratio));
            }
            if (s.proxy) {
                s.proxy->ratio = ratio;
                s.proxy->contentWidth = preferredWidth;
            }
            if (original && s.proxy) {
                w->setStyle(s.explicitStyle ? s.original.data() : nullptr);
                if (!s.explicitStyle) {
                    // Rejoin the original parent stylesheet inheritance. Setting
                    // a null style alone creates a wrapper around the app style.
                    const QString sheet = w->styleSheet();
                    w->setStyleSheet(QString());
                    if (!sheet.isEmpty()) { w->setStyleSheet(sheet); }
                }
            }
            else if (s.proxy) { w->setStyle(s.proxy); }
            QSize minimum = s.minimum;
            if (!original && w->objectName() == QStringLiteral("bottomContainerWidget")) { minimum.setHeight(0); }
            w->setMinimumSize(scaledSize(minimum, ratio));
            w->setMaximumSize(scaledSize(s.maximum, ratio));
            w->setSizePolicy(s.policy);
            w->setContentsMargins(scaledMargins(s.margins, ratio));
            if (!original && s.preferredHeight > 0) {
                w->setFixedHeight(qMax(scaled(s.preferredHeight, ratio), w->minimumSizeHint().height()));
            } else if (original && s.preferredHeight > 0 && s.policy.verticalPolicy() == QSizePolicy::Fixed
                       && d->settingsTabs && d->settingsTabs->isAncestorOf(w) && !w->isVisible()) {
                // Restore the original preferred height even when Qt has discarded
                // a hidden page's native size-hint cache during style changes.
                w->setMinimumHeight(qMax(s.minimum.height(), s.defaultHeight));
            }
            if (preferredWidth > 0) {
                // Keep a nonzero layout hint: Ignored can allocate zero width in
                // a nested box layout even when the widget has a minimum width.
                w->setMinimumWidth(preferredWidth);
            }
            if (original && w->objectName() == QStringLiteral("settingTabWidget")) {
                // Preserve the pre-scaling implicit floor from all seven pages.
                w->setMinimumWidth(qMax(s.minimum.width(), s.naturalMinimum.width()));
            }
            if (auto *splitter = qobject_cast<QSplitter *>(w)) { splitter->setHandleWidth(scaled(s.splitterWidth, ratio)); }
            if (auto *header = qobject_cast<QHeaderView *>(w)) {
                header->setMinimumSectionSize(scaled(s.headerMinimum, ratio));
                header->setDefaultSectionSize(scaled(s.headerDefault, ratio));
            }
            if (s.icon.isValid()) {
                const QSize icon = scaledSize(s.icon, ratio);
                if (auto *button = qobject_cast<QAbstractButton *>(w)) { button->setIconSize(icon); }
                if (auto *combo = qobject_cast<QComboBox *>(w)) { combo->setIconSize(icon); }
                if (auto *view = qobject_cast<QAbstractItemView *>(w)) { view->setIconSize(icon); }
            }
        }
        w->updateGeometry();
        w->update();
#ifdef DEBUG_LOGOUT_ON
        logSpinEditor(w, d->sequence, ratio, "AFTER_PROPERTIES");
#endif
    }
    // Header minima must be ready before fixed sections are resized (Qt5 clamps early).
    for (auto &s : d->widgets) {
        QWidget *w = s.target;
        if (!w || (subtree && w != subtree && !subtree->isAncestorOf(w))) { continue; }
        if (auto *table = qobject_cast<QTableWidget *>(w)) {
            for (int column = 0; column < int(s.fixedColumns.size()) && column < table->columnCount(); ++column) {
                if (s.fixedColumns[column] > 0) { table->setColumnWidth(column, scaled(s.fixedColumns[column], ratio)); }
            }
        }
        if (d->managedWindow) {
            if (auto *label = qobject_cast<QLabel *>(w)) {
                const QPixmap current = QKeyMapperQtCompat::labelPixmap(label);
                if (current.isNull()) { s.authoredPixmap = QPixmap(); s.appliedPixmapKey = 0; continue; }
                if (current.cacheKey() != s.appliedPixmapKey) { s.authoredPixmap = current; }
                const QPixmap adjusted = original ? s.authoredPixmap
                    : s.authoredPixmap.scaled(scaledSize(s.authoredPixmap.size(), ratio), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                label->setPixmap(adjusted);
                s.appliedPixmapKey = adjusted.cacheKey();
            }
        }
    }
    for (const auto &s : d->layouts) {
        QLayout *layout = s.target;
        if (!layout) { continue; }
        if (subtree && layout->parentWidget() != subtree && !subtree->isAncestorOf(layout->parentWidget())) { continue; }
        layout->setContentsMargins(scaledMargins(s.margins, ratio));
        layout->setSpacing(scaled(s.spacing, ratio));
        if (auto *grid = qobject_cast<QGridLayout *>(layout)) {
            grid->setHorizontalSpacing(scaled(s.horizontal, ratio));
            grid->setVerticalSpacing(scaled(s.vertical, ratio));
        }
        const int count = qMin(layout->count(), int(s.spacers.size()));
        for (int i = 0; i < count; ++i) {
            QSpacerItem *spacer = layout->itemAt(i)->spacerItem();
            if (spacer && s.spacers[i].isValid()) {
                const QSize size = scaledSize(s.spacers[i], ratio);
                spacer->changeSize(size.width(), size.height(), spacer->sizePolicy().horizontalPolicy(),
                                   spacer->sizePolicy().verticalPolicy());
            }
        }
    }
    // A popup's own stylesheet may have just been restored from its template.
    // Reapply the inherited theme dimensions after all local styles are ready.
    refreshTheme();
    refreshLayouts();
}

void QkmUiScale::refreshTheme()
{
    if (!d->window) { return; }
    if (d->managedWindow) {
        QString authored;
        for (const auto &s : d->widgets) { if (s.target == d->window) { authored = s.sheet; break; } }
        QWidget *owner = d->window->parentWidget();
        while (owner && owner->parentWidget()) { owner = owner->parentWidget(); }
        QString theme = owner ? owner->styleSheet() : QString();
        QString adjusted = authored;
        if (!qFuzzyCompare(d->ratio, qreal(1))) {
            adjusted = scaledStyleSheet(theme, d->ratio) + QLatin1Char('\n') + scaledStyleSheet(authored, d->ratio);
            const int indicator = scaled(QFontMetrics(d->widgets.front().font).height(), d->ratio);
            adjusted += QStringLiteral("QCheckBox::indicator, QTableView::indicator, QListWidget::indicator "
                                       "{ width: %1px; height: %1px; }").arg(indicator);
        }
        if (d->window->styleSheet() != adjusted) { d->window->setStyleSheet(adjusted); }
        return;
    }
    QWidget *central = d->window->findChild<QWidget *>(QStringLiteral("centralwidget"));
    if (!central) { return; }
    const QString theme = d->window->styleSheet();
    const bool original = qFuzzyCompare(d->ratio, qreal(1));
    for (const auto &s : d->widgets) {
        QWidget *target = s.target;
        if (!target || (target != central &&
            !(target->isWindow() && target->windowType() == Qt::Popup && d->owned(target)))) { continue; }
        QString adjusted = s.sheet;
        if (!original) {
            adjusted = scaledStyleSheet(theme, d->ratio);
            if (!s.sheet.isEmpty()) { adjusted += QLatin1Char('\n') + scaledStyleSheet(s.sheet, d->ratio); }
            // Stylesheet em lengths use the unchanged application font.
            const int indicator = scaled(QFontMetrics(s.font).height(), d->ratio);
            adjusted += QStringLiteral("QCheckBox::indicator, QTableView::indicator, QListWidget::indicator "
                                       "{ width: %1px; height: %1px; }").arg(indicator);
        }
        if (target->styleSheet() != adjusted) { target->setStyleSheet(adjusted); }
    }
}

void QkmUiScale::refreshLayouts()
{
    // Hidden tab pages need explicit invalidation: updateGeometry alone is a no-op.
    for (auto it = d->layouts.rbegin(); it != d->layouts.rend(); ++it) {
        if (it->target) { it->target->invalidate(); it->target->activate(); }
    }
#ifdef DEBUG_LOGOUT_ON
    for (const auto &baseline : d->widgets) {
        if (baseline.target && qobject_cast<QAbstractSpinBox *>(baseline.target.data())) {
            logSpinEditor(baseline.target, d->sequence, d->ratio, "LAYOUT_REFRESHED");
        }
    }
#endif
}
