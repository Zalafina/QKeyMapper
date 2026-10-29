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
#include <QLineEdit>
#include <QComboBox>
#include <QAbstractButton>
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

MappingEditorArguments parseMappingEditorArguments(const QJsonObject &arguments)
{
    if (arguments.size() != 2 || !arguments.value(QStringLiteral("source_ref")).isString()
        || arguments.value(QStringLiteral("presentation")) != QJsonValue(QStringLiteral("private_background"))) return {};
    const QString reference = arguments.value(QStringLiteral("source_ref")).toString();
    const auto parts = reference.split(QLatin1Char(':'));
    if (reference.size() > 128 || parts.size() != 4 || !isLowerHex(parts.at(0), 64)
        || parts.at(1) != QStringLiteral("row")) return {};
    bool tabOk = false, rowOk = false;
    const int tab = parts.at(2).toInt(&tabOk), row = parts.at(3).toInt(&rowOk);
    if (!tabOk || !rowOk || tab < 0 || row < 0 || QString::number(tab) != parts.at(2)
        || QString::number(row) != parts.at(3)) return {};
    return {reference, parts.at(0), tab, row};
}

bool privateBackgroundDesktopAvailable()
{
    if (!guiThread()) return false;
    wchar_t station[256]{}, current[256]{}, input[256]{};
    DWORD needed = 0;
    if (!GetUserObjectInformationW(GetProcessWindowStation(), UOI_NAME, station, sizeof(station), &needed)
        || QString::fromWCharArray(station).compare(QStringLiteral("WinSta0"), Qt::CaseInsensitive) != 0
        || !GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, current, sizeof(current), &needed)
        || QString::fromWCharArray(current).compare(QStringLiteral("Default"), Qt::CaseInsensitive) == 0) return false;
    const HDESK inputDesktop = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
    if (!inputDesktop) return false;
    const bool known = GetUserObjectInformationW(inputDesktop, UOI_NAME, input, sizeof(input), &needed) != FALSE;
    CloseDesktop(inputDesktop);
    return known && QString::fromWCharArray(current).compare(QString::fromWCharArray(input), Qt::CaseInsensitive) != 0;
}

MappingEditorCloseArguments parseMappingEditorCloseArguments(const QJsonObject &arguments)
{
    if (arguments.size() != 3 || !arguments.value(QStringLiteral("window_ref")).isString()
        || !isLowerHex(arguments.value(QStringLiteral("window_ref")).toString(), 64)) return {};
    auto opening = arguments;
    const QString window = opening.take(QStringLiteral("window_ref")).toString();
    const auto source = parseMappingEditorArguments(opening);
    return source.ok() ? MappingEditorCloseArguments{source, window} : MappingEditorCloseArguments{};
}

QByteArray windowInputFingerprint(const QWidget *window)
{
    if (!guiThread() || !window || window->thread() != qApp->thread()) return {};
    for (const auto *child : window->findChildren<QWidget *>())
        if (child->isWindow() && child->isVisible()) return {};
    QJsonObject values;
    const auto key = [](const char *kind, const QObject *object) {
        return QString::fromLatin1(kind) + QLatin1Char(':') + QString::number(quintptr(object), 16);
    };
    // Internal digests only; include control identity so duplicate Qt object names cannot hide a draft.
    for (const auto *edit : window->findChildren<QLineEdit *>())
        values.insert(key("line", edit), edit->text());
    for (const auto *combo : window->findChildren<QComboBox *>())
        values.insert(key("combo", combo), QJsonArray{combo->currentIndex(), combo->currentText()});
    for (const auto *button : window->findChildren<QAbstractButton *>())
        if (button->isCheckable()) values.insert(key("button", button), button->isChecked());
    // shortcut: these cover the current item dialog; add guards when a new draft control type is introduced.
    return QCryptographicHash::hash(QJsonDocument(values).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256);
}

QJsonObject mappingEditorCapability()
{
    const QJsonObject properties{
        {QStringLiteral("source_ref"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
            {QStringLiteral("maxLength"), 128},
            {QStringLiteral("pattern"), QStringLiteral("^[0-9a-f]{64}:row:(0|[1-9][0-9]{0,9}):(0|[1-9][0-9]{0,9})$")}}},
        {QStringLiteral("presentation"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
            {QStringLiteral("const"), QStringLiteral("private_background")}}}};
    return {{QStringLiteral("id"), QStringLiteral("mapping.editor.open")},
        {QStringLiteral("method"), QStringLiteral("qkm_open_mapping_editor")},
        {QStringLiteral("required_scopes"), int(Read | ShowWindows)}, {QStringLiteral("read_only"), false},
        {QStringLiteral("arguments_schema"), QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("additionalProperties"), false},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("source_ref"), QStringLiteral("presentation")}},
            {QStringLiteral("properties"), properties}}},
        {QStringLiteral("limitations"), QJsonArray{QStringLiteral("private_non_input_desktop_only"),
            QStringLiteral("current_visible_source_tab_only"), QStringLiteral("existing_editor_is_not_replaced"),
            QStringLiteral("does_not_request_activation_or_change_mapping")}}};
}

QJsonObject mappingEditorCloseCapability()
{
    auto result = mappingEditorCapability();
    result[QStringLiteral("id")] = QStringLiteral("mapping.editor.close");
    result[QStringLiteral("method")] = QStringLiteral("qkm_close_mapping_editor");
    auto schema = result.value(QStringLiteral("arguments_schema")).toObject();
    auto properties = schema.value(QStringLiteral("properties")).toObject();
    properties[QStringLiteral("window_ref")] = QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
        {QStringLiteral("pattern"), QStringLiteral("^[0-9a-f]{64}$")}};
    auto required = schema.value(QStringLiteral("required")).toArray();
    required.append(QStringLiteral("window_ref"));
    schema[QStringLiteral("properties")] = properties;
    schema[QStringLiteral("required")] = required;
    result[QStringLiteral("arguments_schema")] = schema;
    auto limitations = result.value(QStringLiteral("limitations")).toArray();
    limitations.append(QStringLiteral("unchanged_editor_opened_by_this_owner_only"));
    limitations.append(QStringLiteral("no_draft_discard_or_mapping_reset"));
    result[QStringLiteral("limitations")] = limitations;
    return result;
}

WindowAccess::WindowAccess(QWidget *main, QWidget *itemSetup)
    : QObject(main), m_windows{{main, itemSetup}}
{
    for (const auto &window : m_windows) if (window) window->installEventFilter(this);
}

bool WindowAccess::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_windows[1].data() && (event->type() == QEvent::Show || event->type() == QEvent::Hide
        || event->type() == QEvent::Destroy || event->type() == QEvent::WinIdChange)) {
        m_editorSourceReference.clear();
        m_editorDraftFingerprint.clear();
    }
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
        {QStringLiteral("private_background_available"), privateBackgroundDesktopAvailable()},
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

void WindowAccess::rememberEditor(const QString &sourceReference, const QByteArray &draftFingerprint)
{
    if (!guiThread()) return;
    m_editorSourceReference = sourceReference;
    m_editorDraftFingerprint = draftFingerprint;
}

bool WindowAccess::ownsEditor(const QString &sourceReference) const
{
    return guiThread() && m_windows[1] && m_windows[1]->isVisible()
        && !m_editorSourceReference.isEmpty() && m_editorSourceReference == sourceReference;
}

bool WindowAccess::editorDraftMatches(const QByteArray &draftFingerprint) const
{
    return guiThread() && !m_editorSourceReference.isEmpty() && draftFingerprint.size() == 32
        && m_editorDraftFingerprint == draftFingerprint;
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
