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
}

impl Preview {
    fn valid(&self) -> bool {
        self.source_revision.as_deref().is_none_or(revision)
            && self.source_tab_ref.as_deref().is_none_or(tab_ref)
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
            let mut values = typed(arguments, Preview::valid)?;
            values.retain(|_, value| !value.is_null());
            if matches!(
                values.get("action").and_then(Value::as_str),
                Some("mapping.move_up" | "mapping.move_down")
            ) && (values.len() != 2 || !values.contains_key("source_ref"))
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
