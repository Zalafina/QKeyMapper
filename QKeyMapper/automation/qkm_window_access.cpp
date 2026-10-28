#include "qkm_window_access.h"
#include "qkm_auth_proof.h"
#include "qkm_local_identity.h"
#include "qkm_pair_store.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QThread>
#include <QWindowStateChangeEvent>
#include <QtMath>
#include <limits>

namespace QkmAutomation {
namespace {

const char *const Roles[] = {"main", "item_setup"};

bool guiThread()
{
    return qApp && QThread::currentThread() == qApp->thread();
}

bool validOwner(const QString &instance, const QString &pairId, quint64 permissionEpoch)
{
    return isLowerHex(instance, 32) && isLowerHex(pairId, 32) && permissionEpoch != 0;
}

WindowQueryResult fail(const char *code, const char *message)
{
    return {{}, QString::fromLatin1(code), QString::fromLatin1(message)};
}

} // namespace

WindowAccess::WindowAccess(QWidget *main, QWidget *itemSetup)
    : QObject(main), m_windows{{main, itemSetup}}
{
    for (const auto &window : m_windows) if (window) window->installEventFilter(this);
}

bool WindowAccess::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowStateChange: {
        const auto *window = qobject_cast<QWidget *>(watched);
        const Qt::WindowStates presentation = Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen;
        if (window && (static_cast<QWindowStateChangeEvent *>(event)->oldState() & presentation)
            == (window->windowState() & presentation)) return false;
        Q_FALLTHROUGH();
    }
    case QEvent::Show: case QEvent::Hide: case QEvent::Destroy:
    case QEvent::WinIdChange: case QEvent::Resize:
        for (size_t index = 0; index < m_windows.size(); ++index) {
            if (m_windows[index].data() == watched && m_generations[index] != (std::numeric_limits<quint64>::max)())
                ++m_generations[index];
        }
        break;
    default: break;
    }
    return false;
}

QJsonObject WindowAccess::describe(int index, const QString &instance, const QString &pairId, quint64 permissionEpoch) const
{
    const auto &window = m_windows.at(size_t(index));
    QJsonObject result{{QStringLiteral("role"), QString::fromLatin1(Roles[index])},
        {QStringLiteral("exists"), !window.isNull()}, {QStringLiteral("window_ref"), QJsonValue(QJsonValue::Null)},
        {QStringLiteral("window_generation"), QString::number(m_generations.at(size_t(index)))}};
    if (!window || window->thread() != qApp->thread() || !window->isWindow()) return result;
    // internalWinId observes the existing HWND without creating a native window.
    const HWND hwnd = reinterpret_cast<HWND>(window->internalWinId());
    DWORD process = 0;
    const bool native = hwnd && GetWindowThreadProcessId(hwnd, &process) == GetCurrentThreadId()
        && process == GetCurrentProcessId();
    const qreal ratio = window->devicePixelRatioF();
    result.insert(QStringLiteral("native_ready"), native);
    result.insert(QStringLiteral("qt_visible"), window->isVisible());
    result.insert(QStringLiteral("native_visible"), native && IsWindowVisible(hwnd));
    result.insert(QStringLiteral("minimized"), window->isMinimized() || (native && IsIconic(hwnd)));
    result.insert(QStringLiteral("foreground_exact"), native && GetForegroundWindow() == hwnd);
    result.insert(QStringLiteral("logical_width"), window->width());
    result.insert(QStringLiteral("logical_height"), window->height());
    result.insert(QStringLiteral("device_pixel_ratio"), qIsFinite(ratio) && ratio > 0 ? QJsonValue(ratio) : QJsonValue(QJsonValue::Null));
    if (!native || !qIsFinite(ratio) || ratio <= 0
        || m_generations.at(size_t(index)) == (std::numeric_limits<quint64>::max)()) return result;
    QJsonObject binding = result;
    // Foreground changes are normal while returning to the Agent; they do not invalidate identity.
    binding.remove(QStringLiteral("foreground_exact"));
    binding.insert(QStringLiteral("instance"), instance);
    binding.insert(QStringLiteral("pair"), pairId);
    binding.insert(QStringLiteral("permission_epoch"), QString::number(permissionEpoch));
    binding.insert(QStringLiteral("native_window"), QString::number(quintptr(hwnd)));
    result.insert(QStringLiteral("window_ref"), QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(binding).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex()));
    return result;
}

WindowQueryResult WindowAccess::query(const QString &instance, const QString &pairId, quint64 permissionEpoch,
    const QJsonObject &arguments) const
{
    if (!guiThread() || !validOwner(instance, pairId, permissionEpoch))
        return fail("context_busy", "Window observation requires the instance GUI thread and a current authenticated owner.");
    if (arguments.value(QStringLiteral("entity")) != QJsonValue(QStringLiteral("windows")))
        return fail("invalid_arguments", "Use the windows query entity.");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
        if (it.key() != QStringLiteral("entity") && it.key() != QStringLiteral("offset") && it.key() != QStringLiteral("limit"))
            return fail("invalid_arguments", "Only entity, offset and limit are accepted for windows.");
    }
    const auto integer = [&](const char *key, int fallback, int low, int high) {
        const auto value = arguments.value(QLatin1String(key));
        if (value.isUndefined()) return fallback;
        return value.isDouble() && qIsFinite(value.toDouble()) && value.toDouble() >= low
            && value.toDouble() <= high && value.toDouble() == value.toInt(-1) ? value.toInt() : -1;
    };
    const int offset = integer("offset", 0, 0, 2), limit = integer("limit", 50, 1, 100);
    if (offset < 0 || limit < 0) return fail("invalid_arguments", "Window pagination is outside the supported range.");
    wchar_t desktop[256]{};
    DWORD needed = 0;
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    const QString executable = own.ok() ? queryProcessExecutable(own.identity) : QString();
    if (executable.isEmpty() || !GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME,
        desktop, sizeof(desktop), &needed)) return fail("context_busy", "The current instance identity or desktop could not be verified.");
    DWORD foregroundPid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
    QJsonArray items;
    int next = offset;
    for (; next < 2 && items.size() < limit; ++next) items.append(describe(next, instance, pairId, permissionEpoch));
    return {{{QStringLiteral("schema_version"), 1}, {QStringLiteral("entity"), QStringLiteral("windows")},
        {QStringLiteral("sampled_at_utc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("target"), QJsonObject{{QStringLiteral("instance"), instance},
            {QStringLiteral("pid"), QString::number(own.identity.processId)},
            {QStringLiteral("created"), QString::number(own.identity.creationTime)},
            {QStringLiteral("executable"), executable}, {QStringLiteral("desktop"), QString::fromWCharArray(desktop)}}},
        {QStringLiteral("reference_scope"), QStringLiteral("instance_window_lifetime_only")},
        {QStringLiteral("foreground_in_instance"), foregroundPid == GetCurrentProcessId()},
        {QStringLiteral("offset"), offset}, {QStringLiteral("total"), 2}, {QStringLiteral("items"), items},
        {QStringLiteral("next_offset"), next < 2 ? QJsonValue(next) : QJsonValue(QJsonValue::Null)}}, {}, {}};
}

QWidget *WindowAccess::resolve(const QString &role, const QString &reference, const QString &instance,
    const QString &pairId, quint64 permissionEpoch) const
{
    if (!guiThread() || !validOwner(instance, pairId, permissionEpoch) || !isLowerHex(reference, 64)) return nullptr;
    for (int index = 0; index < 2; ++index) {
        if (role == QLatin1String(Roles[index])
            && describe(index, instance, pairId, permissionEpoch).value(QStringLiteral("window_ref")) == QJsonValue(reference))
            return m_windows.at(size_t(index));
    }
    return nullptr;
}

QJsonObject windowQueryCapability()
{
    return {{QStringLiteral("id"), QStringLiteral("windows.read")}, {QStringLiteral("method"), QStringLiteral("qkm_query")},
        {QStringLiteral("required_scopes"), int(Read)}, {QStringLiteral("read_only"), true},
        {QStringLiteral("arguments_schema"), QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("additionalProperties"), false}, {QStringLiteral("required"), QJsonArray{QStringLiteral("entity")}},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("entity"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                    {QStringLiteral("const"), QStringLiteral("windows")}}},
                {QStringLiteral("offset"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
                    {QStringLiteral("minimum"), 0}, {QStringLiteral("maximum"), 2}}},
                {QStringLiteral("limit"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
                    {QStringLiteral("minimum"), 1}, {QStringLiteral("maximum"), 100}}}}}}},
        {QStringLiteral("limitations"), QJsonArray{QStringLiteral("fixed_main_and_item_setup_roles"),
            QStringLiteral("window_reference_is_not_a_source_or_scene_revision"), QStringLiteral("does_not_show_or_capture_windows"),
            QStringLiteral("foreground_exact_is_observation_not_activation_proof")}}};
}

} // namespace QkmAutomation
