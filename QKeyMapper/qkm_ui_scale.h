#ifndef QKM_UI_SCALE_H
#define QKM_UI_SCALE_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include "qkeymapper_qt_compat.h"
#include <QPointer>
#include <QSize>
#include <functional>
#include <memory>

class QWidget;
class QStyle;

// Private editor presentation state. Runtime overlay coordinates are excluded.
class QkmUiScale final : public QObject, public QAbstractNativeEventFilter
{
public:
    explicit QkmUiScale(QWidget *window);
    ~QkmUiScale() override;
    void capture(QWidget *authoredRoot = nullptr);
    void apply(qreal ratio, QWidget *subtree = nullptr);
    void refreshLayouts();
    void refreshTheme();
    QSize defaultWindowMinimum() const;
    void setStyleFactory(std::function<QStyle *(QStyle *, QWidget *)> factory);
    void setStyleResolver(std::function<QStyle *(QWidget *)> resolver);
    void manageWindow();
    void applyWindow(qreal ratio);
    void setWindowSizeMode(int mode);
    bool isApplying() const;
    qreal ratio() const;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    bool nativeEventFilter(const QByteArray &eventType, void *message,
                           QKeyMapperQtCompat::NativeEventResult *result) override;

private:
    struct Data;
    std::unique_ptr<Data> d;
};

#endif
