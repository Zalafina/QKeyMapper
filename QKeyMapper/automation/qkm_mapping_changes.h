#ifndef QKM_MAPPING_CHANGES_H
#define QKM_MAPPING_CHANGES_H

#include <QElapsedTimer>
#include <QDeadlineTimer>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>
#include <optional>

namespace QkmAutomation {

constexpr int MappingChangeBudgetMs = 5000;

struct MappingAdditionArguments {
    QString sourceRevision;
    QString sourceTabRef;
    QString originalKey;
    QString mappingKey;
    int insertRow = -1;
};

enum class MappingChangeMethod { Preview, Apply, Cancel, Operation };
enum class MappingChangeAction { AddMapping, SaveConfiguration };
struct MappingChangeRequest {
    MappingChangeMethod method = MappingChangeMethod::Preview;
    MappingChangeAction action = MappingChangeAction::AddMapping;
    MappingAdditionArguments addition;
    QString settingName;
    QString id;
    bool confirmConflicts = false;
    QString errorCode;
    QString message;
    bool ok() const { return errorCode.isEmpty(); }
};

bool isMappingChangeMethod(const QString &method);
MappingChangeRequest validateMappingChangeRequest(const QString &method, const QJsonObject &arguments);
QJsonArray mappingChangeCapabilities();

struct MappingChangeOwner {
    QString instance;
    QString pairId;
    quint64 permissionEpoch = 0;
    bool operator==(const MappingChangeOwner &other) const
    { return instance == other.instance && pairId == other.pairId && permissionEpoch == other.permissionEpoch; }
};

struct ConfigurationSaveBinding {
    QString settingName;
    QString revision;
    QJsonObject context;
    QJsonObject scope;
    QDeadlineTimer expiresAt;
};

struct MappingChangePreview {
    MappingChangeOwner owner;
    MappingAdditionArguments arguments;
    QJsonObject description;
    QString id;
    qint64 created = 0;
    std::optional<ConfigurationSaveBinding> save;
};

// GUI-thread request state only; no source pointers, worker work, persistence or retries.
class MappingChangeStore {
public:
    static constexpr qint64 PreviewTtlMs = 120000;
    static constexpr qint64 ResultTtlMs = 600000;
    static constexpr int MaxPreviews = 32;
    static constexpr int MaxResults = 64;
    std::optional<MappingChangePreview> create(const MappingChangeOwner &owner,
        const MappingAdditionArguments &arguments, const QJsonObject &description);
    std::optional<MappingChangePreview> createSave(const MappingChangeOwner &owner,
        const ConfigurationSaveBinding &binding, const QJsonObject &description);
    // The caller must revalidate live source, permissions and expiry before committing.
    std::optional<MappingChangePreview> begin(const MappingChangeOwner &owner, const QString &id);
    bool expired(const MappingChangePreview &preview)
    { return now() - preview.created >= PreviewTtlMs || (preview.save && preview.save->expiresAt.hasExpired()); }
    void finish(const MappingChangeOwner &owner, const QString &id, const QString &status,
        const QJsonObject &result, const QString &errorCode = {}, const QString &message = {});
    bool cancel(const MappingChangeOwner &owner, const QString &id);
    QJsonObject operation(const MappingChangeOwner &owner, const QString &id);
    void clear();
private:
#ifdef QKM_AUTOMATION_TESTING
    friend class MappingChangeStoreTestAccess;
#endif
    struct Operation {
        MappingChangeOwner owner;
        QJsonObject value;
        qint64 created = 0;
    };
    qint64 now();
    void prune();
    bool remember(const MappingChangePreview &preview, const QString &status,
        const QString &errorCode = {}, const QString &message = {});
    QElapsedTimer m_clock;
    QHash<QString, MappingChangePreview> m_previews;
    QHash<QString, Operation> m_results;
};

} // namespace QkmAutomation
#endif
