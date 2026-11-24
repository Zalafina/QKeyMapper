use qkeymapper_mcp_rust::native::valid_macro_backup_name;
use qkeymapper_mcp_rust::state_wait::StateWait;
use qkeymapper_mcp_rust::wire::{InvalidMessage, Object, decimal, hex_bytes};
use serde::{Deserialize, Serialize, de::DeserializeOwned};
use serde_json::Value;

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum Topic {
    BasicMapping,
    OriginalKeys,
    MappingKeys,
}

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum Entity {
    Configurations,
    Tabs,
    MappingSources,
    Mappings,
    Windows,
    DemoContext,
    Macros,
}

#[derive(Default, Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum SettingsRole {
    #[default]
    MappingAdvanced,
    IbinputSettings,
}

impl SettingsRole {
    fn is_default(&self) -> bool {
        matches!(self, Self::MappingAdvanced)
    }
}

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum CaptureRole {
    Main,
    ItemSetup,
    MappingAdvanced,
    IbinputSettings,
}

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum Presentation {
    PrivateBackground,
}

fn default_limit() -> u8 {
    50
}

fn page(offset: u32, limit: u8) -> bool {
    offset <= i32::MAX as u32 && (1..=100).contains(&limit)
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Help {
    topic: Topic,
    #[serde(default)]
    offset: u32,
    #[serde(default = "default_limit")]
    limit: u8,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Query {
    entity: Entity,
    #[serde(skip_serializing_if = "Option::is_none")]
    catalog_revision: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    source_revision: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    source_tab_ref: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    window_role: Option<SettingsRole>,
    #[serde(default)]
    offset: u32,
    #[serde(default = "default_limit")]
    limit: u8,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Operation {
    operation_id: String,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Stop {
    instance: String,
    request_id: String,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Start {
    instance: String,
    request_id: String,
    source_revision: String,
    source_tab_ref: String,
    save_policy: String,
}

#[derive(Default, Deserialize, Serialize)]
enum Action {
    #[default]
    #[serde(rename = "mapping.add")]
    Add,
    #[serde(rename = "mapping.update")]
    Update,
    #[serde(rename = "mapping.delete")]
    Delete,
    #[serde(rename = "mapping.move_up")]
    MoveUp,
    #[serde(rename = "mapping.move_down")]
    MoveDown,
    #[serde(rename = "mapping.move_to_top")]
    MoveToTop,
    #[serde(rename = "mapping.move_to_bottom")]
    MoveToBottom,
    #[serde(rename = "mapping.duplicate")]
    DuplicateMapping,
    #[serde(rename = "tab.rename")]
    RenameTab,
    #[serde(rename = "tab.set_hotkey")]
    SetTabHotkey,
    #[serde(rename = "tab.set_notification_mode")]
    SetTabNotificationMode,
    #[serde(rename = "tab.set_category_filter")]
    SetCategoryFilter,
    #[serde(rename = "configuration.set_show_disabled_rows")]
    SetShowDisabledRows,
    #[serde(rename = "configuration.set_mapping_display")]
    SetMappingDisplay,
    #[serde(rename = "tab.set_notification_colors")]
    SetTabNotificationColors,
    #[serde(rename = "tab.add_blank")]
    AddBlankTab,
    #[serde(rename = "tab.copy")]
    CopyTab,
    #[serde(rename = "tab.delete")]
    DeleteTab,
    #[serde(rename = "tab.clear")]
    ClearTab,
    #[serde(rename = "tab.move")]
    MoveTab,
    #[serde(rename = "tab.select")]
    SelectTab,
    #[serde(rename = "tab.set_common_inheritance")]
    SetCommonInheritance,
    #[serde(rename = "tab.set_common_collapsed")]
    SetCommonCollapsed,
    #[serde(rename = "configuration.set_common_enabled")]
    SetCommonFeature,
    #[serde(rename = "configuration.save")]
    Save,
    #[serde(rename = "ibinput.preferences.update")]
    Preferences,
}

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum Backend {
    Auto,
    LogitechGhub,
    LogitechLegacy,
    Razer,
}

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum UnavailablePolicy {
    Block,
    SendinputBeforeStart,
}

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum MacroScope {
    Configuration,
    Universal,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct MacroExport {
    catalog_revision: String,
    scope: MacroScope,
    #[serde(skip_serializing_if = "Option::is_none")]
    names: Option<Vec<String>>,
    file_name: String,
    #[serde(default)]
    overwrite: bool,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct TableExport {
    source_tab_ref: String,
    file_name: String,
    #[serde(default)]
    overwrite: bool,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct ConfigurationExport {
    catalog_revision: String,
    groups: Vec<String>,
    file_name: String,
    #[serde(default)]
    overwrite: bool,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct TableImport {
    action: String,
    source_tab_ref: String,
    file_name: String,
}

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum ConfigurationImportConflict {
    Keep,
    Overwrite,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct ConfigurationImport {
    action: String,
    catalog_revision: String,
    groups: Vec<String>,
    file_name: String,
    on_conflict: ConfigurationImportConflict,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct SaveConfigurationImport {
    action: String,
    import_draft_id: String,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct ConfigurationMove {
    action: String,
    catalog_revision: String,
    group: String,
    target_group: String,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct ConfigurationResetOrder {
    action: String,
    catalog_revision: String,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct ConfigurationGroup {
    action: String,
    catalog_revision: String,
    group: String,
}

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum MacroInsertPosition {
    Above,
    Below,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct MacroInsertAt {
    position: MacroInsertPosition,
    anchor_name: String,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct MacroUpsert {
    catalog_revision: String,
    scope: MacroScope,
    name: String,
    content: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    category: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    note: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    insert_at: Option<MacroInsertAt>,
    #[serde(skip_serializing_if = "Option::is_none")]
    segment_comments: Option<Vec<String>>,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct MacroDelete {
    catalog_revision: String,
    scope: MacroScope,
    names: Vec<String>,
}

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum MacroConflictPolicy {
    Keep,
    Overwrite,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct MacroImport {
    catalog_revision: String,
    scope: MacroScope,
    file_name: String,
    on_conflict: MacroConflictPolicy,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct MacroPreview<T> {
    action: String,
    #[serde(rename = "macro")]
    data: T,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct TableInsertMode {
    action: String,
    source_tab_ref: String,
    table_insert_mode: u8,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct TableEditTrigger {
    action: String,
    source_tab_ref: String,
    table_edit_trigger: u8,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct WindowDisplayMode {
    action: String,
    source_tab_ref: String,
    window_display_mode: u8,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct SaveWindowSize {
    action: String,
    source_tab_ref: String,
    save_window_size: bool,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct SaveSplitterPosition {
    action: String,
    source_tab_ref: String,
    save_splitter_position: bool,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct StartupPoint {
    x: i32,
    y: i32,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct StartupPositionFields {
    #[serde(skip_serializing_if = "Option::is_none")]
    mode: Option<u8>,
    #[serde(skip_serializing_if = "Option::is_none")]
    point: Option<StartupPoint>,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct StartupPosition {
    action: String,
    source_tab_ref: String,
    startup_position: StartupPositionFields,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Preview {
    #[serde(default)]
    action: Action,
    source_revision: Option<String>,
    source_tab_ref: Option<String>,
    original_key: Option<String>,
    mapping_key: Option<String>,
    insert_row: Option<u32>,
    setting_name: Option<String>,
    source_refs: Option<Vec<String>>,
    send_method: Option<u8>,
    source_ref: Option<String>,
    preferences_revision: Option<String>,
    preferred_backend: Option<Backend>,
    unavailable_policy: Option<UnavailablePolicy>,
    key_up_mapping_key: Option<String>,
    disabled: Option<bool>,
    note: Option<String>,
    category: Option<String>,
    category_filter: Option<Value>,
    show_disabled_rows: Option<bool>,
    mapping_display: Option<Object>,
    row_modes: Option<Object>,
    send_timing: Option<u8>,
    copy_to: Option<Object>,
    tab_name: Option<String>,
    tab_hotkey: Option<String>,
    notification_mode: Option<u8>,
    notification_font_color: Option<String>,
    notification_background_color: Option<String>,
    target_tab_ref: Option<String>,
    include_common: Option<bool>,
    common_collapsed: Option<bool>,
    common_feature_enabled: Option<bool>,
}

fn macro_text(value: &str, max: usize) -> bool {
    !value.contains('\0') && value.encode_utf16().count() <= max
}

fn single_line(value: &str) -> bool {
    value.encode_utf16().count() <= 32767 && !value.contains(['\0', '\r', '\n'])
}

fn configuration_group_name(value: &str) -> bool {
    !value.is_empty() && value.trim() == value && single_line(value)
}

fn notification_color(value: &str, digits: usize) -> bool {
    value == "default"
        || value
            .strip_prefix('#')
            .is_some_and(|hex| hex.len() == digits && hex.bytes().all(|c| c.is_ascii_hexdigit()))
}

fn same_source_batch(rows: &[Value]) -> bool {
    let Some(first) = rows
        .first()
        .and_then(Value::as_str)
        .and_then(|row| row.rsplit_once(':'))
    else {
        return false;
    };
    (2..=100).contains(&rows.len())
        && rows.iter().enumerate().all(|(index, value)| {
            value.as_str().is_some_and(|row| {
                row_ref(row)
                    && row
                        .rsplit_once(':')
                        .is_some_and(|(prefix, _)| prefix == first.0)
            }) && !rows[..index].contains(value)
        })
}

fn original_trigger(value: &Value) -> bool {
    let Some(trigger) = value.as_object() else {
        return false;
    };
    let kind = trigger.get("type").and_then(Value::as_str);
    let duration = trigger.get("duration_ms");
    match kind {
        Some("normal") => trigger.len() == 1,
        Some("long_press" | "double_press") => {
            trigger.len() == if duration.is_some() { 2 } else { 1 }
                && duration
                    .is_none_or(|time| time.as_u64().is_some_and(|ms| (1..=9999).contains(&ms)))
        }
        _ => false,
    }
}

impl Preview {
    fn valid(&self) -> bool {
        self.source_revision.as_deref().is_none_or(revision)
            && self.source_tab_ref.as_deref().is_none_or(tab_ref)
            && self.target_tab_ref.as_deref().is_none_or(tab_ref)
            && self.original_key.as_deref().is_none_or(basic_key)
            && self.mapping_key.as_deref().is_none_or(basic_key)
            && self.insert_row.is_none_or(|row| row <= i32::MAX as u32)
            && self
                .setting_name
                .as_deref()
                .is_none_or(|name| (1..=32767).contains(&name.chars().count()))
            && self.source_refs.as_ref().is_none_or(|refs| {
                (1..=100).contains(&refs.len()) && refs.iter().all(|s| row_ref(s))
            })
            && self.send_method.is_none_or(|method| method <= 3)
            && self.source_ref.as_deref().is_none_or(row_ref)
            && self.preferences_revision.as_deref().is_none_or(revision)
            && self
                .key_up_mapping_key
                .as_deref()
                .is_none_or(|key| key.chars().count() <= 1)
            && self.tab_name.as_deref().is_none_or(single_line)
            && self.tab_hotkey.as_deref().is_none_or(single_line)
            && self.note.as_deref().is_none_or(single_line)
            && self.category.as_deref().is_none_or(single_line)
            && self.category_filter.as_ref().is_none_or(|filter| {
                filter.as_str() == Some("all")
                    || filter.as_array().is_some_and(|names| {
                        names.len() <= 100
                            && names.iter().enumerate().all(|(index, value)| {
                                value.as_str().is_some_and(single_line)
                                    && !names[..index].contains(value)
                            })
                    })
            })
            && self.mapping_display.as_ref().is_none_or(|display| {
                !display.is_empty()
                    && display.iter().all(|(name, value)| {
                        matches!(
                            name.as_str(),
                            "category_column" | "notes" | "floating_column"
                        ) && value.is_boolean()
                    })
            })
            && self.row_modes.as_ref().is_none_or(|modes| {
                !modes.is_empty()
                    && modes.iter().all(|(key, value)| match key.as_str() {
                        "burst"
                        | "lock"
                        | "mapping_key_unlock"
                        | "disable_original_key_unlock"
                        | "sequence_hold_down"
                        | "pass_through"
                        | "check_combo_order"
                        | "unbreakable"
                        | "disable_fn_key_switch" => value.is_boolean(),
                        "fixed_vkey_code" => value.as_u64().is_some_and(|vk| vk <= 255),
                        "paste_text_mode" => value.as_u64().is_some_and(|mode| mode <= 1),
                        "burst_press_ms" | "burst_release_ms" => {
                            value.as_u64().is_some_and(|ms| (1..=99_999).contains(&ms))
                        }
                        "repeat_mode" => value.as_u64().is_some_and(|mode| mode <= 2),
                        "repeat_times" => value.as_u64().is_some_and(|count| count <= 99_999_999),
                        "original_trigger" => modes.len() == 1 && original_trigger(value),
                        _ => false,
                    })
            })
            && self.send_timing.is_none_or(|timing| timing <= 4)
            && self.notification_mode.is_none_or(|mode| mode <= 2)
            && self
                .notification_font_color
                .as_deref()
                .is_none_or(|value| notification_color(value, 6))
            && self
                .notification_background_color
                .as_deref()
                .is_none_or(|value| notification_color(value, 8))
    }
}

#[derive(Default, Deserialize, Serialize)]
#[serde(default, deny_unknown_fields)]
struct Apply {
    preview_id: String,
    confirm_conflicts: bool,
    confirm_scope: bool,
    confirm_overwrite: bool,
    confirm_delete: bool,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Cancel {
    preview_id: String,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct OpenEditor {
    source_ref: String,
    presentation: Presentation,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct CloseEditor {
    source_ref: String,
    window_ref: String,
    presentation: Presentation,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct OpenSettings {
    source_tab_ref: String,
    presentation: Presentation,
    #[serde(default, skip_serializing_if = "SettingsRole::is_default")]
    window_role: SettingsRole,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct CloseSettings {
    source_tab_ref: String,
    window_ref: String,
    presentation: Presentation,
    #[serde(default, skip_serializing_if = "SettingsRole::is_default")]
    window_role: SettingsRole,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Capture {
    role: CaptureRole,
    window_ref: String,
    source_revision: String,
    source_tab_ref: String,
}

#[derive(Default, Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum DemoView {
    Main,
    #[default]
    ItemSetup,
    MappingAdvanced,
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct DemoMapping {
    original_key: String,
    mapping_key: String,
    #[serde(default)]
    view: DemoView,
}

fn revision(s: &str) -> bool {
    hex_bytes::<32>(s).is_ok()
}

fn id(s: &str) -> bool {
    hex_bytes::<16>(s).is_ok()
}

fn index(s: &str) -> bool {
    s.len() <= 10 && decimal(s, true).is_ok()
}

fn tab_ref(text: &str) -> bool {
    text.split_once(":source:")
        .is_some_and(|(value, tab)| revision(value) && index(tab))
}

fn row_ref(text: &str) -> bool {
    text.split_once(":row:").is_some_and(|(value, position)| {
        revision(value)
            && position
                .split_once(':')
                .is_some_and(|(tab, row)| index(tab) && index(row))
    })
}

fn basic_key(s: &str) -> bool {
    matches!(s.as_bytes(), [b'A'..=b'Z' | b'0'..=b'9'])
}

fn typed<T: DeserializeOwned + Serialize>(
    arguments: Object,
    valid: impl FnOnce(&T) -> bool,
) -> Result<Object, InvalidMessage> {
    let value: T = serde_json::from_value(Value::Object(arguments)).map_err(|_| InvalidMessage)?;
    if !valid(&value) {
        return Err(InvalidMessage);
    }
    match serde_json::to_value(value).map_err(|_| InvalidMessage)? {
        Value::Object(value) => Ok(value),
        _ => Err(InvalidMessage),
    }
}

pub fn parse(tool: &str, arguments: Object) -> Result<Object, InvalidMessage> {
    match tool {
        "qkm_get_help" => typed(arguments, |v: &Help| page(v.offset, v.limit)),
        "qkm_query" => typed(arguments, |v: &Query| {
            page(v.offset, v.limit)
                && v.catalog_revision.as_deref().is_none_or(revision)
                && v.source_revision.as_deref().is_none_or(revision)
                && v.source_tab_ref.as_deref().is_none_or(tab_ref)
        }),
        "qkm_export_macros" => {
            if arguments.get("names").is_some_and(Value::is_null) {
                return Err(InvalidMessage);
            }
            typed(arguments, |v: &MacroExport| {
                let mut seen = std::collections::HashSet::new();
                revision(&v.catalog_revision)
                    && valid_macro_backup_name(&v.file_name)
                    && v.names.as_ref().is_none_or(|names| {
                        (1..=100).contains(&names.len())
                            && names.iter().all(|name| {
                                !name.is_empty() && macro_text(name, 32767) && seen.insert(name)
                            })
                    })
            })
        }
        "qkm_export_mapping_table" => typed(arguments, |v: &TableExport| {
            tab_ref(&v.source_tab_ref) && valid_macro_backup_name(&v.file_name)
        }),
        "qkm_export_configurations" => typed(arguments, |v: &ConfigurationExport| {
            let mut seen = std::collections::HashSet::new();
            revision(&v.catalog_revision)
                && (1..=100).contains(&v.groups.len())
                && v.groups
                    .iter()
                    .all(|name| !name.is_empty() && single_line(name) && seen.insert(name))
                && valid_macro_backup_name(&v.file_name)
        }),
        "qkm_get_operation" => typed(arguments, |v: &Operation| id(&v.operation_id)),
        "qkm_stop_mapping" => typed(arguments, |v: &Stop| id(&v.instance) && id(&v.request_id)),
        "qkm_start_mapping" => typed(arguments, |v: &Start| {
            id(&v.instance)
                && id(&v.request_id)
                && revision(&v.source_revision)
                && tab_ref(&v.source_tab_ref)
                && v.source_tab_ref.starts_with(&v.source_revision)
                && v.save_policy == "start_only"
        }),
        "qkm_wait_state" => typed(arguments, StateWait::valid),
        "qkm_preview_changes" => {
            match arguments.get("action").and_then(Value::as_str) {
                Some("general.set_table_insert_mode") => {
                    return typed(arguments, |v: &TableInsertMode| {
                        tab_ref(&v.source_tab_ref) && v.table_insert_mode <= 1
                    });
                }
                Some("general.set_table_edit_trigger") => {
                    return typed(arguments, |v: &TableEditTrigger| {
                        tab_ref(&v.source_tab_ref) && v.table_edit_trigger <= 1
                    });
                }
                Some("configuration.set_window_display_mode") => {
                    return typed(arguments, |v: &WindowDisplayMode| {
                        tab_ref(&v.source_tab_ref) && v.window_display_mode <= 2
                    });
                }
                Some("general.set_save_window_size") => {
                    return typed(arguments, |v: &SaveWindowSize| tab_ref(&v.source_tab_ref));
                }
                Some("general.set_save_splitter_position") => {
                    return typed(arguments, |v: &SaveSplitterPosition| {
                        tab_ref(&v.source_tab_ref)
                    });
                }
                Some("general.set_startup_position") => {
                    if arguments
                        .get("startup_position")
                        .and_then(Value::as_object)
                        .is_some_and(|fields| fields.values().any(Value::is_null))
                    {
                        return Err(InvalidMessage);
                    }
                    return typed(arguments, |v: &StartupPosition| {
                        let fields = &v.startup_position;
                        tab_ref(&v.source_tab_ref)
                            && (fields.mode.is_some() || fields.point.is_some())
                            && fields.mode.is_none_or(|mode| mode <= 2)
                            && fields.point.as_ref().is_none_or(|point| {
                                fields.mode.unwrap_or(2) == 2
                                    && (-50000..=50000).contains(&point.x)
                                    && (-50000..=50000).contains(&point.y)
                            })
                    });
                }
                Some("table.import") => {
                    return typed(arguments, |v: &TableImport| {
                        tab_ref(&v.source_tab_ref) && valid_macro_backup_name(&v.file_name)
                    });
                }
                Some("configuration.import") => {
                    return typed(arguments, |v: &ConfigurationImport| {
                        let mut seen = std::collections::HashSet::new();
                        revision(&v.catalog_revision)
                            && (1..=100).contains(&v.groups.len())
                            && v.groups
                                .iter()
                                .all(|name| configuration_group_name(name) && seen.insert(name))
                            && valid_macro_backup_name(&v.file_name)
                    });
                }
                Some("configuration.save_import") => {
                    return typed(arguments, |v: &SaveConfigurationImport| {
                        id(&v.import_draft_id)
                    });
                }
                Some("configuration.move") => {
                    return typed(arguments, |v: &ConfigurationMove| {
                        revision(&v.catalog_revision)
                            && configuration_group_name(&v.group)
                            && configuration_group_name(&v.target_group)
                    });
                }
                Some("configuration.reset_order") => {
                    return typed(arguments, |v: &ConfigurationResetOrder| {
                        revision(&v.catalog_revision)
                    });
                }
                Some("configuration.remove" | "configuration.load") => {
                    return typed(arguments, |v: &ConfigurationGroup| {
                        revision(&v.catalog_revision) && configuration_group_name(&v.group)
                    });
                }
                Some("macro.upsert") => {
                    return typed(arguments, |v: &MacroPreview<MacroUpsert>| {
                        revision(&v.data.catalog_revision)
                            && !v.data.name.is_empty()
                            && macro_text(&v.data.name, 32767)
                            && !v.data.content.is_empty()
                            && macro_text(&v.data.content, 1_000_000)
                            && v.data
                                .category
                                .as_deref()
                                .is_none_or(|value| macro_text(value, 32767))
                            && v.data
                                .note
                                .as_deref()
                                .is_none_or(|value| macro_text(value, 32767))
                            && v.data.insert_at.as_ref().is_none_or(|at| {
                                !at.anchor_name.is_empty() && macro_text(&at.anchor_name, 32767)
                            })
                            && v.data.segment_comments.as_ref().is_none_or(|comments| {
                                !comments.is_empty()
                                    && comments.iter().all(|text| macro_text(text, 32767))
                            })
                    });
                }
                Some("macro.delete") => {
                    return typed(arguments, |v: &MacroPreview<MacroDelete>| {
                        let mut seen = std::collections::HashSet::new();
                        revision(&v.data.catalog_revision)
                            && (1..=100).contains(&v.data.names.len())
                            && v.data.names.iter().all(|name| {
                                !name.is_empty() && macro_text(name, 32767) && seen.insert(name)
                            })
                    });
                }
                Some("macro.import") => {
                    return typed(arguments, |v: &MacroPreview<MacroImport>| {
                        revision(&v.data.catalog_revision)
                            && valid_macro_backup_name(&v.data.file_name)
                    });
                }
                _ => {}
            }
            let duplicate =
                arguments.get("action").and_then(Value::as_str) == Some("mapping.duplicate");
            if duplicate || arguments.contains_key("copy_to") {
                let single = arguments.get("source_ref").and_then(Value::as_str);
                let batch = arguments.get("source_refs").and_then(Value::as_array);
                let single_valid = single.is_some_and(row_ref);
                let batch_valid = batch.is_some_and(|rows| same_source_batch(rows));
                let source =
                    single.or_else(|| batch.and_then(|rows| rows.first()).and_then(Value::as_str));
                let Some(destination) = arguments.get("copy_to").and_then(Value::as_object) else {
                    return Err(InvalidMessage);
                };
                let source_tab = source
                    .and_then(|row| row.rsplit_once(':'))
                    .and_then(|(prefix, _)| prefix.split_once(":row:"));
                let explicit_target = destination.get("target_tab_ref");
                let target_tab = match explicit_target {
                    Some(value) => {
                        let Some(target) = value.as_str().filter(|target| tab_ref(target)) else {
                            return Err(InvalidMessage);
                        };
                        let target_tab = target.split_once(":source:");
                        if target_tab.map(|(revision, _)| revision)
                            != source_tab.map(|(revision, _)| revision)
                        {
                            return Err(InvalidMessage);
                        }
                        target_tab
                    }
                    None => source_tab,
                };
                let target_fields = usize::from(explicit_target.is_some());
                let position = destination.get("position").and_then(Value::as_str);
                let placement_valid = match position {
                    Some("top" | "bottom") => destination.len() == 1 + target_fields,
                    Some("above" | "below") => {
                        let anchor = destination.get("anchor_ref").and_then(Value::as_str);
                        destination.len() == 2 + target_fields
                            && anchor.is_some_and(row_ref)
                            && anchor
                                .and_then(|row| row.rsplit_once(':'))
                                .and_then(|(prefix, _)| prefix.split_once(":row:"))
                                == target_tab
                    }
                    _ => false,
                };
                if !duplicate
                    || arguments.len() != 3
                    || single_valid == batch_valid
                    || !placement_valid
                {
                    return Err(InvalidMessage);
                }
            }
            if let Some(timing) = arguments.get("send_timing")
                && (!timing.is_u64()
                    || arguments.len() != 3
                    || !arguments.contains_key("source_ref")
                    || arguments.get("action").and_then(Value::as_str) != Some("mapping.update"))
            {
                return Err(InvalidMessage);
            }
            let modes = arguments.get("row_modes");
            let category = arguments.get("category");
            if modes.is_some() || category.is_some() {
                let single = arguments.get("source_ref").is_some_and(Value::is_string);
                let batch = arguments.get("source_refs").and_then(Value::as_array);
                let valid_batch = batch.is_some_and(|rows| same_source_batch(rows));
                if modes.is_some_and(|value| !value.is_object())
                    || category.is_some_and(|value| !value.is_string())
                    || arguments.get("action").and_then(Value::as_str) != Some("mapping.update")
                    || arguments.len() != 3
                    || single == valid_batch
                    || (valid_batch
                        && category.is_none()
                        && !modes
                            .and_then(|value| value.get("original_trigger"))
                            .is_some_and(|trigger| trigger.get("duration_ms").is_none()))
                {
                    return Err(InvalidMessage);
                }
            }
            let mut values = typed(arguments, Preview::valid)?;
            values.retain(|_, value| !value.is_null());
            let valid_move = match values.get("action").and_then(Value::as_str) {
                Some(
                    "mapping.move_up"
                    | "mapping.move_down"
                    | "mapping.move_to_top"
                    | "mapping.move_to_bottom",
                ) => {
                    values.len() == 2
                        && (values.contains_key("source_ref")
                            || values
                                .get("source_refs")
                                .and_then(Value::as_array)
                                .is_some_and(|rows| rows.len() >= 2))
                }
                _ => true,
            };
            let action = values.get("action").and_then(Value::as_str);
            if (values.contains_key("note")
                && (action != Some("mapping.update")
                    || values.len() != 3
                    || !values.contains_key("source_ref")))
                || (action == Some("mapping.update")
                    && values.contains_key("source_refs")
                    && !values.contains_key("category")
                    && !values.contains_key("row_modes"))
            {
                return Err(InvalidMessage);
            }
            let rename = action == Some("tab.rename");
            let set_hotkey = action == Some("tab.set_hotkey");
            let set_notification = action == Some("tab.set_notification_mode");
            let category_filter = action == Some("tab.set_category_filter");
            let show_disabled = action == Some("configuration.set_show_disabled_rows");
            let mapping_display = action == Some("configuration.set_mapping_display");
            let color_count = ["notification_font_color", "notification_background_color"]
                .into_iter()
                .filter(|field| values.contains_key(*field))
                .count();
            let common_inheritance = action == Some("tab.set_common_inheritance");
            let common_collapsed = action == Some("tab.set_common_collapsed");
            let common_feature = action == Some("configuration.set_common_enabled");
            let destination = values.get("target_tab_ref").and_then(Value::as_str);
            let valid_tab_change = match action {
                Some(
                    "tab.rename"
                    | "tab.set_hotkey"
                    | "tab.set_notification_mode"
                    | "tab.set_common_inheritance"
                    | "tab.set_common_collapsed"
                    | "configuration.set_common_enabled",
                ) => values.len() == 3 && values.contains_key("source_tab_ref"),
                Some("tab.move" | "tab.select") => {
                    values.len() == 3
                        && values
                            .get("source_tab_ref")
                            .and_then(Value::as_str)
                            .zip(destination)
                            .is_some_and(|(source, target)| {
                                (action == Some("tab.select") || source != target)
                                    && source.split(':').next() == target.split(':').next()
                            })
                }
                Some(
                    "tab.set_category_filter"
                    | "configuration.set_show_disabled_rows"
                    | "configuration.set_mapping_display",
                ) => values.len() == 3 && values.contains_key("source_tab_ref"),
                Some("tab.set_notification_colors") => {
                    color_count > 0
                        && values.len() == 2 + color_count
                        && values.contains_key("source_tab_ref")
                }
                Some("tab.add_blank" | "tab.copy" | "tab.delete" | "tab.clear") => {
                    values.len() == 2 && values.contains_key("source_tab_ref")
                }
                _ => true,
            };
            if !valid_move
                || !valid_tab_change
                || rename != values.contains_key("tab_name")
                || set_hotkey != values.contains_key("tab_hotkey")
                || set_notification != values.contains_key("notification_mode")
                || category_filter != values.contains_key("category_filter")
                || show_disabled != values.contains_key("show_disabled_rows")
                || mapping_display != values.contains_key("mapping_display")
                || (color_count > 0 && action != Some("tab.set_notification_colors"))
                || common_inheritance != values.contains_key("include_common")
                || common_collapsed != values.contains_key("common_collapsed")
                || common_feature != values.contains_key("common_feature_enabled")
                || (destination.is_some() && !matches!(action, Some("tab.move" | "tab.select")))
            {
                return Err(InvalidMessage);
            }
            // Keep the IPC defaults. Qt validates each action's field set.
            if values.get("action").and_then(Value::as_str) == Some("mapping.add") {
                values.remove("action");
                values.entry("insert_row".to_owned()).or_insert(Value::Null);
            }
            Ok(values)
        }
        "qkm_apply_changes" => {
            let mut values = typed(arguments, |v: &Apply| id(&v.preview_id))?;
            values
                .retain(|key, value| key == "confirm_conflicts" || value.as_bool() != Some(false));
            Ok(values)
        }
        "qkm_cancel_changes" => typed(arguments, |v: &Cancel| id(&v.preview_id)),
        "qkm_open_mapping_editor" => typed(arguments, |v: &OpenEditor| row_ref(&v.source_ref)),
        "qkm_close_mapping_editor" => typed(arguments, |v: &CloseEditor| {
            row_ref(&v.source_ref) && revision(&v.window_ref)
        }),
        "qkm_open_mapping_settings" => {
            typed(arguments, |v: &OpenSettings| tab_ref(&v.source_tab_ref))
        }
        "qkm_close_mapping_settings" => typed(arguments, |v: &CloseSettings| {
            tab_ref(&v.source_tab_ref) && revision(&v.window_ref)
        }),
        "qkm_capture_window" => typed(arguments, |v: &Capture| {
            revision(&v.window_ref) && revision(&v.source_revision) && tab_ref(&v.source_tab_ref)
        }),
        "qkm_demo_mapping" => typed(arguments, |v: &DemoMapping| {
            basic_key(&v.original_key) && basic_key(&v.mapping_key)
        }),
        _ if arguments.is_empty() => Ok(arguments),
        _ => Err(InvalidMessage),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn settings_window_omits_the_default_role_and_keeps_an_explicit_popup() {
        let mut arguments = json!({"source_tab_ref":format!("{}:source:0", "a".repeat(64)),
            "presentation":"private_background"});
        let default = arguments.as_object().unwrap().clone();
        arguments["window_role"] = "mapping_advanced".into();
        assert_eq!(
            parse(
                "qkm_open_mapping_settings",
                arguments.as_object().unwrap().clone()
            )
            .unwrap(),
            default
        );
        arguments["window_role"] = "ibinput_settings".into();
        assert_eq!(
            parse(
                "qkm_open_mapping_settings",
                arguments.as_object().unwrap().clone()
            )
            .unwrap(),
            *arguments.as_object().unwrap()
        );
    }

    #[test]
    fn configuration_arguments_keep_original_wire_shapes() {
        let id = "b".repeat(32);
        let revision = "a".repeat(64);
        let row = format!("{revision}:row:1:99");
        let cases = [
            (
                "qkm_preview_changes",
                json!({"action":"mapping.add", "original_key":"A", "mapping_key":"B", "send_method":null}),
                json!({"original_key":"A", "mapping_key":"B", "insert_row":null}),
            ),
            (
                "qkm_preview_changes",
                json!({"action":"mapping.delete", "source_refs":[row], "setting_name":null}),
                json!({"action":"mapping.delete", "source_refs":[row]}),
            ),
            (
                "qkm_preview_changes",
                json!({"action":"mapping.update", "source_ref":row, "send_method":3, "key_up_mapping_key":""}),
                json!({"action":"mapping.update", "source_ref":row, "send_method":3, "key_up_mapping_key":""}),
            ),
            (
                "qkm_preview_changes",
                json!({"action":"ibinput.preferences.update", "preferences_revision":revision,
                    "preferred_backend":"logitech_ghub", "unavailable_policy":"sendinput_before_start"}),
                json!({"action":"ibinput.preferences.update", "preferences_revision":revision,
                    "preferred_backend":"logitech_ghub", "unavailable_policy":"sendinput_before_start"}),
            ),
            (
                "qkm_preview_changes",
                json!({"action":"configuration.save", "setting_name":"合成 设置", "insert_row":null}),
                json!({"action":"configuration.save", "setting_name":"合成 设置"}),
            ),
            (
                "qkm_apply_changes",
                json!({"preview_id":id, "confirm_scope":true, "confirm_overwrite":false}),
                json!({"preview_id":id, "confirm_conflicts":false, "confirm_scope":true}),
            ),
            (
                "qkm_cancel_changes",
                json!({"preview_id":id}),
                json!({"preview_id":id}),
            ),
        ];
        for (tool, input, output) in cases {
            assert_eq!(
                parse(tool, input.as_object().unwrap().clone()).unwrap(),
                *output.as_object().unwrap()
            );
        }
    }

    #[test]
    fn configuration_list_requires_current_catalog_and_exact_fields() {
        let revision = "a".repeat(64);
        let move_group = json!({"action":"configuration.move", "catalog_revision":revision,
            "group":"配置甲", "target_group":"Second"});
        let reset = json!({"action":"configuration.reset_order", "catalog_revision":revision});
        let remove =
            json!({"action":"configuration.remove", "catalog_revision":revision, "group":"Other"});
        let load =
            json!({"action":"configuration.load", "catalog_revision":revision, "group":"Other"});
        let preview =
            |value: &Value| parse("qkm_preview_changes", value.as_object().unwrap().clone());
        for valid in [&move_group, &reset, &remove, &load] {
            assert_eq!(preview(valid).unwrap(), *valid.as_object().unwrap());
        }
        for field in ["group", "target_group", "catalog_revision"] {
            let mut absent = move_group.clone();
            absent.as_object_mut().unwrap().remove(field);
            assert!(preview(&absent).is_err());
        }
        let mut padded = move_group.clone();
        padded["target_group"] = " Second".into();
        assert!(preview(&padded).is_err());
        let mut multiline = move_group;
        multiline["group"] = "bad\nname".into();
        assert!(preview(&multiline).is_err());
        let mut extra = reset;
        extra["group"] = "配置甲".into();
        assert!(preview(&extra).is_err());
        let mut extra = remove;
        extra["target_group"] = "Other".into();
        assert!(preview(&extra).is_err());
        let mut load_with_path = load;
        load_with_path["file_name"] = "other.ini".into();
        assert!(preview(&load_with_path).is_err());
    }

    #[test]
    fn query_wire_arguments_omit_null_and_send_original_defaults() {
        let cases = [
            (
                "qkm_get_help",
                json!({"topic": "basic_mapping"}),
                json!({"topic": "basic_mapping", "offset": 0, "limit": 50}),
            ),
            (
                "qkm_query",
                json!({"entity":"windows", "source_revision":null, "catalog_revision":null,
                                "source_tab_ref":null, "window_role":"ibinput_settings", "offset": 3}),
                json!({"entity":"windows", "window_role":"ibinput_settings", "offset":3, "limit":50}),
            ),
            (
                "qkm_get_operation",
                json!({"operation_id": "abcd".repeat(8)}),
                json!({"operation_id":"abcd".repeat(8)}),
            ),
        ];
        for (tool, input, output) in cases {
            assert_eq!(
                parse(tool, input.as_object().unwrap().clone()).unwrap(),
                *output.as_object().unwrap()
            );
        }
        assert!(tab_ref(&format!("{}:source:0", "a".repeat(64))));
        assert!(tab_ref(&format!("{}:source:9999999999", "a".repeat(64))));
        assert!(!tab_ref(&format!("{}:source:01", "a".repeat(64))));
        assert!(!tab_ref(&format!("{}:source:3:row:4", "a".repeat(64))));
    }
}
