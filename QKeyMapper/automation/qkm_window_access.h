#ifndef QKM_WINDOW_ACCESS_H
#define QKM_WINDOW_ACCESS_H

#include <QObject>
#include <QPointer>
#include <QWidget>
#include <QJsonObject>
#include <array>

namespace QkmAutomation {

struct WindowQueryResult {
    QJsonObject page;
    QString errorCode;
    QString message;
    bool ok() const { return errorCode.isEmpty(); }
};

struct MappingEditorArguments {
    QString sourceReference;
    QString sourceRevision;
    int sourceTab = -1;
    int sourceRow = -1;
    bool ok() const { return !sourceReference.isEmpty(); }
};

MappingEditorArguments parseMappingEditorArguments(const QJsonObject &arguments);
bool privateBackgroundDesktopAvailable();
QJsonObject mappingEditorCapability();

// Tracks only the product's two explicitly supplied window roles.
class WindowAccess : public QObject {
public:
    WindowAccess(QWidget *main, QWidget *itemSetup);
    WindowQueryResult query(const QString &instance, const QString &pairId, quint64 permissionEpoch,
        const QJsonObject &arguments) const;
    QWidget *resolve(const QString &role, const QString &reference, const QString &instance,
        const QString &pairId, quint64 permissionEpoch) const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QJsonObject describe(int index, const QString &instance, const QString &pairId, quint64 permissionEpoch) const;
    std::array<QPointer<QWidget>, 2> m_windows;
    std::array<quint64, 2> m_generations{{1, 1}};
};

QJsonObject windowQueryCapability();

} // namespace QkmAutomation
#endif
