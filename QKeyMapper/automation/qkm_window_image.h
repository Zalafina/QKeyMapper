#ifndef QKM_WINDOW_IMAGE_H
#define QKM_WINDOW_IMAGE_H

#include <QByteArray>
#include <QSize>
#include <QString>
#include <QJsonObject>

class QWidget;

namespace QkmAutomation {

constexpr qint64 MaxWindowImagePixels = 8 * 1024 * 1024;
constexpr int MaxWindowImageBytes = 2 * 1024 * 1024;
constexpr int MaxTransportWindowImageBytes = 160 * 1024;

struct WindowCaptureArguments {
    QString role;
    QString windowReference;
    QString sourceRevision;
    QString sourceTabReference;
    bool ok() const { return !role.isEmpty(); }
};

WindowCaptureArguments parseWindowCaptureArguments(const QJsonObject &arguments);
QJsonObject windowCaptureCapability();

struct WindowImage {
    QByteArray png;
    QSize pixelSize;
    qreal devicePixelRatio = 1;
    QString errorCode;
    bool ok() const { return errorCode.isEmpty() && !png.isEmpty(); }
};

// Internal GUI-thread primitive; callers own role selection, source binding and authorization.
WindowImage captureOwnWindow(QWidget *window, int maxBytes = MaxWindowImageBytes);

} // namespace QkmAutomation

#endif
