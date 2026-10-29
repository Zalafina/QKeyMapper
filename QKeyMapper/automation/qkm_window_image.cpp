#include "qkm_window_image.h"
#include "qkm_auth_proof.h"
#include "qkm_pair_store.h"
#include <QApplication>
#include <QBuffer>
#include <QPixmap>
#include <QJsonArray>
#include <QThread>
#include <QWidget>
#include <QtMath>
#include <qt_windows.h>

namespace QkmAutomation {

WindowCaptureArguments parseWindowCaptureArguments(const QJsonObject &arguments)
{
    if (arguments.size() != 4 || !arguments.value(QStringLiteral("role")).isString()
        || !arguments.value(QStringLiteral("window_ref")).isString()
        || !arguments.value(QStringLiteral("source_revision")).isString()
        || !arguments.value(QStringLiteral("source_tab_ref")).isString()) return {};
    const QString role = arguments.value(QStringLiteral("role")).toString();
    const QString window = arguments.value(QStringLiteral("window_ref")).toString();
    const QString source = arguments.value(QStringLiteral("source_revision")).toString();
    const QString tab = arguments.value(QStringLiteral("source_tab_ref")).toString();
    const auto parts = tab.split(QLatin1Char(':'));
    bool validIndex = false;
    const int index = parts.size() == 3 ? parts.at(2).toInt(&validIndex) : -1;
    if ((role != QStringLiteral("main") && role != QStringLiteral("item_setup") && role != QStringLiteral("mapping_advanced"))
        || !isLowerHex(window, 64) || !isLowerHex(source, 64) || tab.size() > 128
        || parts.size() != 3 || !isLowerHex(parts.at(0), 64) || parts.at(1) != QStringLiteral("source")
        || !validIndex || index < 0 || QString::number(index) != parts.at(2)) return {};
    return {role, window, source, tab};
}

QJsonObject windowCaptureCapability()
{
    const QJsonObject revision{{QStringLiteral("type"), QStringLiteral("string")},
        {QStringLiteral("pattern"), QStringLiteral("^[0-9a-f]{64}$")}};
    return {{QStringLiteral("id"), QStringLiteral("windows.capture")},
        {QStringLiteral("method"), QStringLiteral("qkm_capture_window")},
        {QStringLiteral("required_scopes"), int(Read | CaptureWindows)}, {QStringLiteral("read_only"), true},
        {QStringLiteral("arguments_schema"), QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("additionalProperties"), false},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("role"), QStringLiteral("window_ref"), QStringLiteral("source_revision"), QStringLiteral("source_tab_ref")}},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("role"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                    {QStringLiteral("enum"), QJsonArray{QStringLiteral("main"), QStringLiteral("item_setup"), QStringLiteral("mapping_advanced")}}}},
                {QStringLiteral("window_ref"), revision}, {QStringLiteral("source_revision"), revision},
                {QStringLiteral("source_tab_ref"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                    {QStringLiteral("maxLength"), 128},
                    {QStringLiteral("pattern"), QStringLiteral("^[0-9a-f]{64}:source:(0|[1-9][0-9]{0,9})$")}}}}}}},
        {QStringLiteral("max_png_bytes"), MaxTransportWindowImageBytes},
        {QStringLiteral("max_pixels"), double(MaxWindowImagePixels)},
        {QStringLiteral("limitations"), QJsonArray{QStringLiteral("visible_restored_own_qt_client_only"),
            QStringLiteral("advanced_settings_not_committed_by_source_revision"),
            QStringLiteral("source_revision_does_not_commit_ui_drafts"), QStringLiteral("does_not_show_or_activate"),
            QStringLiteral("requires_current_safe_source_context"), QStringLiteral("independent_popups_not_supported")}}};
}

WindowImage captureOwnWindow(QWidget *window, int maxBytes)
{
    const auto fail = [](const char *code) {
        WindowImage result;
        result.errorCode = QString::fromLatin1(code);
        return result;
    };
    if (maxBytes <= 0 || maxBytes > MaxWindowImageBytes) return fail("invalid_window_context");
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
    if (result.png.size() > maxBytes) return fail("image_too_large");
    result.pixelSize = image.size();
    result.devicePixelRatio = image.devicePixelRatioF();
    return result;
}

} // namespace QkmAutomation
