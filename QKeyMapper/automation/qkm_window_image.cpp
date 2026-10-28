#include "qkm_window_image.h"
#include <QApplication>
#include <QBuffer>
#include <QPixmap>
#include <QThread>
#include <QWidget>
#include <QtMath>
#include <qt_windows.h>

namespace QkmAutomation {

WindowImage captureOwnWindow(QWidget *window)
{
    const auto fail = [](const char *code) {
        WindowImage result;
        result.errorCode = QString::fromLatin1(code);
        return result;
    };
    if (!window || !qApp || QThread::currentThread() != qApp->thread()
        || window->thread() != qApp->thread()) return fail("invalid_window_context");
    if (!window->isWindow() || !window->isVisible() || window->isMinimized()) return fail("window_unavailable");
    DWORD process = 0;
    const DWORD thread = GetWindowThreadProcessId(reinterpret_cast<HWND>(window->winId()), &process);
    if (thread != GetCurrentThreadId() || process != GetCurrentProcessId()) return fail("wrong_window_owner");
    const qreal ratio = window->devicePixelRatioF();
    const double pixels = double(window->width()) * window->height() * ratio * ratio;
    if (!qIsFinite(ratio) || ratio <= 0 || !qIsFinite(pixels) || pixels <= 0 || pixels > MaxWindowImagePixels)
        return fail("image_too_large");
    const QPixmap image = window->grab();
    if (image.isNull()) return fail("capture_failed");
    if (qint64(image.width()) * image.height() > MaxWindowImagePixels) return fail("image_too_large");
    WindowImage result;
    QBuffer buffer(&result.png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) return fail("capture_failed");
    if (result.png.size() > MaxWindowImageBytes) return fail("image_too_large");
    result.pixelSize = image.size();
    result.devicePixelRatio = image.devicePixelRatioF();
    return result;
}

} // namespace QkmAutomation
