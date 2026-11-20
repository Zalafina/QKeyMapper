#ifndef QKM_MAPPING_CHANGES_H
#define QKM_MAPPING_CHANGES_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QDeadlineTimer>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <optional>

namespace QkmAutomation {

constexpr int MappingChangeBudgetMs = 5000;

struct MappingAdditionArguments {
    QString sourceRevision;
    QString sourceTabRef;
    QString originalKey;
    QString mappingKey;
    int insertRow = -1;
    int sendMethod = 0;
};

struct MappingRemovalArguments {
    QString sourceRevision;
    QString sourceTabRef;
    QList<int> sourceRows;
};

enum class MappingRowMove { None, Up, Down, Top, Bottom };
const char *mappingRowMoveAction(MappingRowMove move);

struct MappingUpdateArguments {
    QString sourceReference;
    QString sourceRevision;
    QString sourceTabRef;
    int sourceRow = -1;
    int sendMethod = -1;
    std::optional<QString> originalKey;
    std::optional<QString> mappingKey;
    std::optional<QString> keyUpMappingKey;
    std::optional<bool> disabled;
    MappingRowMove move = MappingRowMove::None;
    int rowCount = 1;
    std::optional<QString> tabName;
    std::optional<QString> tabHotkey;
    bool addBlankTab = false;
    bool copyTab = false;
    bool deleteTab = false;
    bool clearTab = false;
    QString targetTabRef;
    bool selectTab = false;
    std::optional<bool> tabIncludeCommon;
    std::optional<bool> tabCommonCollapsed;
    std::optional<bool> commonFeatureEnabled;
    QByteArray tabSettingsDigest;
};

struct IbInputPreferencesArguments {
    QString revision;
    QString preferredBackend;
    QString unavailablePolicy;
};

enum class MappingChangeMethod { Preview, Apply, Cancel, Operation, Stop, Start };
enum class MappingChangeAction { AddMapping, SaveConfiguration, DeleteMapping, UpdateMapping, UpdateIbInputPreferences };
struct MappingChangeRequest {
    MappingChangeMethod method = MappingChangeMethod::Preview;
    MappingChangeAction action = MappingChangeAction::AddMapping;
    MappingAdditionArguments addition;
    MappingRemovalArguments removal;
    MappingUpdateArguments update;
    IbInputPreferencesArguments preferences;
    QString settingName;
    QString id;
    QString instance;
    QString startSourceRevision;
    QString startSourceTabRef;
    bool confirmConflicts = false;
    bool confirmScope = false;
    bool confirmOverwrite = false;
    bool confirmDelete = false;
    QString errorCode;
    QString message;
    bool ok() const { return errorCode.isEmpty(); }
};

bool isBasicMappingKey(const QString &key);
bool isMappingChangeMethod(const QString &method);
MappingChangeRequest validateMappingChangeRequest(const QString &method, const QJsonObject &arguments);
QJsonArray mappingChangeCapabilities(bool allowRunControls = true);

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
    std::optional<MappingRemovalArguments> removal;
    std::optional<MappingUpdateArguments> update;
    std::optional<IbInputPreferencesArguments> preferences;
    QString preferencesRevision;
};

// GUI-thread request state only; no source pointers, worker work, persistence or retries.
class MappingChangeStore {
public:
    static constexpr qint64 PreviewTtlMs = 120000;
    static constexpr qint64 ResultTtlMs = 600000;
    static constexpr int MaxPreviews = 32;
    static constexpr int MaxResults = 64;
    std::optional<MappingChangePreview> create(const MappingChangeOwner &owner,
        const MappingAdditionArguments &arguments, const QJsonObject &description, const QString &preferencesRevision = {});
    std::optional<MappingChangePreview> createSave(const MappingChangeOwner &owner,
        const ConfigurationSaveBinding &binding, const QJsonObject &description, const QString &preferencesRevision = {});
    std::optional<MappingChangePreview> createRemoval(const MappingChangeOwner &owner,
        const MappingRemovalArguments &arguments, const QJsonObject &description, const QString &preferencesRevision = {});
    std::optional<MappingChangePreview> createUpdate(const MappingChangeOwner &owner,
        const MappingUpdateArguments &arguments, const QJsonObject &description, const QString &preferencesRevision);
    std::optional<MappingChangePreview> createPreferences(const MappingChangeOwner &owner,
        const IbInputPreferencesArguments &arguments, const QJsonObject &description, const QString &preferencesRevision);
    // The caller must revalidate live source, permissions and expiry before committing.
    std::optional<MappingChangePreview> begin(const MappingChangeOwner &owner, const QString &id);
    bool expired(const MappingChangePreview &preview)
    { return now() - preview.created >= PreviewTtlMs || (preview.save && preview.save->expiresAt.hasExpired()); }
    void finish(const MappingChangeOwner &owner, const QString &id, const QString &status,
        const QJsonObject &result, const QString &errorCode = {}, const QString &message = {});
    bool cancel(const MappingChangeOwner &owner, const QString &id);
    enum class RunReservation { Reserved, Conflict, Full };
    RunReservation reserveStart(const MappingChangeOwner &owner, const QString &id, const QJsonObject &target);
    RunReservation reserveStop(const MappingChangeOwner &owner, const QString &id, bool idle, bool initiated,
        bool cleanupObserved);
    void bindStop(const QString &id, quint64 generation, bool timedOut);
    void observeStop(quint64 generation, bool completed, bool timedOut);
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
        quint64 stopGeneration = 0;
    };
    qint64 now();
    void prune();
    bool remember(const MappingChangePreview &preview, const QString &status,
        const QString &errorCode = {}, const QString &message = {});
    bool remember(const MappingChangeOwner &owner, const QString &id, QJsonObject value);
    RunReservation reserveRun(const MappingChangeOwner &owner, const QString &id, QJsonObject value);
    void updateStop(Operation &operation, bool completed, bool timedOut);
    QElapsedTimer m_clock;
    QHash<QString, MappingChangePreview> m_previews;
    QHash<QString, Operation> m_results;
};

} // namespace QkmAutomation
#endif
