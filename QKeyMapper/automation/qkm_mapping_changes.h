#ifndef QKM_MAPPING_CHANGES_H
#define QKM_MAPPING_CHANGES_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QDeadlineTimer>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QPoint>
#include <QStringList>
#include <optional>

namespace QkmAutomation {

constexpr int MappingChangeBudgetMs = 5000;
inline constexpr int WindowMatchStateTextMaxCodeUnits = 1024;

struct BoundedAutomationText {
    QString value;
    bool truncated = false;
    int fullCodeUnits = 0;
    QString fullSha256;
};

BoundedAutomationText boundAutomationText(const QString &text, int maxCodeUnits = WindowMatchStateTextMaxCodeUnits);
QJsonObject buildWindowMatchCriteriaFields(const QString &processName, int processMode,
    const QString &windowTitle, int titleMode, const QString &className, int classMode,
    int maxCodeUnits = WindowMatchStateTextMaxCodeUnits);
QJsonObject buildWindowMatchCriteriaState(bool editable, const QString &processName, int processMode,
    const QString &windowTitle, int titleMode, const QString &className, int classMode,
    const QString &cachedProcess, const QString &cachedTitle, const QString &cachedClass,
    int maxCodeUnits = WindowMatchStateTextMaxCodeUnits);

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

struct VButtonPanelUpdate {
    std::optional<int> columns;
    std::optional<int> maxRows;
    std::optional<int> buttonWidth;
    std::optional<int> buttonHeight;
    std::optional<double> opacity;
    std::optional<bool> alwaysOnTop;
    std::optional<bool> defaultShow;
    std::optional<int> margin;
    std::optional<int> radius;
    std::optional<bool> dragEnabled;
    std::optional<int> referencePoint;
    std::optional<int> offsetX;
    std::optional<int> offsetY;
    std::optional<int> fontSize;
    std::optional<int> fontWeight;
    std::optional<QString> fontFamily;
    std::optional<QString> bgColor;
    std::optional<QString> buttonColor;
    std::optional<QString> pressedColor;
    std::optional<QString> lockedColor;
    std::optional<QString> textColor;
};

struct NotificationStyleUpdate {
    std::optional<QString> fontColor;
    std::optional<QString> backgroundColor;
    std::optional<QString> fontFamily;
    std::optional<int> fontSize;
    std::optional<int> fontWeight;
    std::optional<bool> fontItalic;
    std::optional<int> displayDuration;
    std::optional<int> fadeInDuration;
    std::optional<int> fadeOutDuration;
    std::optional<int> borderRadius;
    std::optional<int> padding;
    std::optional<double> opacity;
    std::optional<int> xOffset;
    std::optional<int> yOffset;
};

struct TrayIconsUpdate {
    std::optional<QString> idle;
    std::optional<QString> monitoring;
    std::optional<QString> global;
    std::optional<QString> matched;
};

struct TabFloatingWindowUpdate {
    std::optional<QString> backgroundColor;
    std::optional<int> size;
    std::optional<int> radius;
    std::optional<double> opacity;
    std::optional<int> referencePoint;
    std::optional<int> positionX;
    std::optional<int> positionY;
    std::optional<bool> mousePassThrough;
    std::optional<QString> mousePassThroughSwitchKey;
};

struct MappingCrosshairUpdate {
    std::optional<QString> centerColor;
    std::optional<int> centerSize;
    std::optional<int> centerOpacity;
    std::optional<QString> crosshairColor;
    std::optional<int> crosshairWidth;
    std::optional<int> crosshairLength;
    std::optional<int> crosshairOpacity;
    std::optional<bool> showCenter;
    std::optional<bool> showTop;
    std::optional<bool> showBottom;
    std::optional<bool> showLeft;
    std::optional<bool> showRight;
    std::optional<int> xOffset;
    std::optional<int> yOffset;
};

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
    std::optional<QString> note;
    std::optional<QString> category;
    QList<int> batchRows;
    QJsonObject rowModes;
    std::optional<int> sendTiming;
    MappingRowMove move = MappingRowMove::None;
    int rowCount = 1;
    QString copyPosition;
    QString copyAnchorRef;
    QString copyTargetTabRef;
    std::optional<QString> tabName;
    std::optional<QString> tabHotkey;
    std::optional<Qt::CheckState> tabNotificationMode;
    std::optional<QJsonValue> tabCategoryFilter;
    std::optional<bool> showDisabledRows;
    QJsonObject mappingDisplay;
    std::optional<int> tableInsertMode;
    std::optional<int> tableEditTrigger;
    std::optional<int> displayScale;
    std::optional<int> windowDisplayMode;
    std::optional<bool> saveWindowSize;
    std::optional<bool> saveSplitterPosition;
    std::optional<bool> playSoundEffect;
    std::optional<int> globalSwitchTimeoutMs;
    std::optional<bool> disableGlobalMappingInFullscreen;
    std::optional<QString> fullscreenAllowedProcesses;
    std::optional<int> mouseXSpeed;
    std::optional<int> mouseYSpeed;
    std::optional<int> mousePollingIntervalMs;
    std::optional<int> gyro2MouseXSource;
    std::optional<int> gyro2MouseYSource;
    std::optional<bool> gyro2MouseXRevert;
    std::optional<bool> gyro2MouseYRevert;
    std::optional<int> gamepadLtPress;
    std::optional<int> gamepadLtLightPress;
    std::optional<int> gamepadLtRelease;
    std::optional<int> gamepadRtPress;
    std::optional<int> gamepadRtLightPress;
    std::optional<int> gamepadRtRelease;
    std::optional<int> gamepadLsPush;
    std::optional<int> gamepadLsLightPush;
    std::optional<int> gamepadLsRelease;
    std::optional<int> gamepadRsPush;
    std::optional<int> gamepadRsLightPush;
    std::optional<int> gamepadRsRelease;
    std::optional<double> gamepadTouchpadXSpeed;
    std::optional<double> gamepadTouchpadYSpeed;
    std::optional<bool> gamepadTouchpadInvertX;
    std::optional<bool> gamepadTouchpadInvertY;
    std::optional<bool> acceptVirtualGamepadInput;
    std::optional<VButtonPanelUpdate> vbuttonPanel;
    std::optional<NotificationStyleUpdate> generalNotificationStyle;
    std::optional<NotificationStyleUpdate> customNotificationStyle;
    std::optional<TrayIconsUpdate> trayIcons;
    std::optional<TabFloatingWindowUpdate> tabFloatingWindow;
    std::optional<MappingCrosshairUpdate> crosshair;
    std::optional<bool> processIconAsTrayIcon;
    std::optional<QString> showWindowPointKey;
    std::optional<QString> showScreenPointKey;
    std::optional<bool> customNotificationEnabled;
    std::optional<int> customNotificationPosition;
    std::optional<QString> windowProcessName;
    std::optional<int> windowProcessMatchMode;
    std::optional<QString> windowTitleText;
    std::optional<int> windowTitleMatchMode;
    std::optional<QString> windowClassName;
    std::optional<int> windowClassMatchMode;
    std::optional<bool> autoMatchForeground;
    std::optional<bool> sendToSameWindows;
    std::optional<int> startupPositionMode;
    std::optional<QPoint> startupPositionPoint;
    QByteArray tableImportDocument;
    QString tabNotificationFontColor;
    QString tabNotificationBackgroundColor;
    bool addBlankTab = false;
    bool copyTab = false;
    bool deleteTab = false;
    bool clearTab = false;
    bool restoreProcessPath = false;
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

struct MacroChangeArguments {
    QString catalogRevision;
    QString scope;
    QString name;
    QString content;
    std::optional<QString> category;
    std::optional<QString> note;
    QStringList namesToDelete;
    QString anchorName;
    bool insertBelow = false;
    std::optional<QStringList> segmentComments;
    QByteArray importDocument;
    bool overwriteImportDuplicates = false;
};

struct IgnoreRuleChangeArguments {
    QString catalogRevision;
    QString ruleName;
    QStringList namesToDelete;
    bool restoreDefaults = false;
    std::optional<QString> processName;
    std::optional<int> processMatchMode;
    std::optional<QString> windowTitle;
    std::optional<int> windowTitleMatchMode;
    std::optional<QString> className;
    std::optional<int> classNameMatchMode;
    std::optional<QString> description;
    std::optional<bool> disabled;
};

struct ConfigurationImportBinding {
    QString catalogRevision;
    QStringList groups;
    QByteArray document;
    QString contextRevision;
    QString draftId;
    bool overwrite = false;
};

enum class ConfigurationListAction { Move, ResetOrder, Remove, Load, LoadEmpty };
struct ConfigurationListBinding {
    ConfigurationListAction action = ConfigurationListAction::ResetOrder;
    QString catalogRevision;
    QString group;
    QString targetGroup;
    QString contextRevision;
    bool reset() const { return action == ConfigurationListAction::ResetOrder; }
    bool removesGroup() const { return action == ConfigurationListAction::Remove; }
    bool loadsGroup() const { return action == ConfigurationListAction::Load; }
    bool loadsEmpty() const { return action == ConfigurationListAction::LoadEmpty; }
};

enum class MappingChangeMethod { Preview, Apply, Cancel, Operation, Stop, Start };
enum class MappingChangeAction { AddMapping, SaveConfiguration, DeleteMapping, UpdateMapping, UpdateIbInputPreferences,
    UpsertMacro, DeleteMacro, ImportMacro, UpsertIgnoreRule, DeleteIgnoreRule, RestoreDefaultIgnoreRules, ImportConfiguration, SaveConfigurationImport, ChangeConfigurationList };
struct MappingChangeRequest {
    MappingChangeMethod method = MappingChangeMethod::Preview;
    MappingChangeAction action = MappingChangeAction::AddMapping;
    MappingAdditionArguments addition;
    MappingRemovalArguments removal;
    MappingUpdateArguments update;
    IbInputPreferencesArguments preferences;
    MacroChangeArguments macro;
    IgnoreRuleChangeArguments ignoreRule;
    ConfigurationImportBinding configurationImport;
    ConfigurationListBinding configurationList;
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
QJsonArray mappingChangeCapabilities(bool standardMode = true);

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
    std::optional<MacroChangeArguments> macro;
    std::optional<IgnoreRuleChangeArguments> ignoreRule;
    QString preferencesRevision;
    std::optional<ConfigurationImportBinding> configurationImport;
    std::optional<ConfigurationListBinding> configurationList;
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
    std::optional<MappingChangePreview> createMacro(const MappingChangeOwner &owner,
        const MacroChangeArguments &arguments, const QJsonObject &description, const QString &preferencesRevision);
    std::optional<MappingChangePreview> createIgnoreRule(const MappingChangeOwner &owner,
        const IgnoreRuleChangeArguments &arguments, const QJsonObject &description, const QString &preferencesRevision);
    std::optional<MappingChangePreview> createConfigurationImport(const MappingChangeOwner &owner,
        const ConfigurationImportBinding &binding, const QJsonObject &description, const QString &preferencesRevision);
    std::optional<MappingChangePreview> createConfigurationList(const MappingChangeOwner &owner,
        const ConfigurationListBinding &binding, const QJsonObject &description, const QString &preferencesRevision);
    bool stageConfigurationImport(const MappingChangePreview &preview, QJsonObject result);
    std::optional<ConfigurationImportBinding> configurationImportDraft(const MappingChangeOwner &owner, const QString &id);
    void completeConfigurationImportSave(const MappingChangeOwner &owner, const QString &id);
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
        std::optional<ConfigurationImportBinding> configurationImportDraft;
    };
    qint64 now();
    void prune();
    bool remember(const MappingChangePreview &preview, const QString &status,
        const QString &errorCode = {}, const QString &message = {});
    bool remember(const MappingChangeOwner &owner, const QString &id, QJsonObject value);
    bool releaseConfigurationImportDraft(const MappingChangeOwner &owner, const QString &id, const QString &state);
    RunReservation reserveRun(const MappingChangeOwner &owner, const QString &id, QJsonObject value);
    void updateStop(Operation &operation, bool completed, bool timedOut);
    QElapsedTimer m_clock;
    QHash<QString, MappingChangePreview> m_previews;
    QHash<QString, Operation> m_results;
};

} // namespace QkmAutomation
#endif
