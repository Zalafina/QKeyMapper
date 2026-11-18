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

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum Role {
    MappingAdvanced,
    IbinputSettings,
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
    window_role: Option<Role>,
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

fn tab_ref(text: &str) -> bool {
    text.split_once(":source:")
        .is_some_and(|(revision, index)| {
            hex_bytes::<32>(revision).is_ok() && index.len() <= 10 && decimal(index, true).is_ok()
        })
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
                && v.catalog_revision
                    .as_deref()
                    .is_none_or(|s| hex_bytes::<32>(s).is_ok())
                && v.source_revision
                    .as_deref()
                    .is_none_or(|s| hex_bytes::<32>(s).is_ok())
                && v.source_tab_ref.as_deref().is_none_or(tab_ref)
        }),
        "qkm_get_operation" => typed(arguments, |v: &Operation| {
            hex_bytes::<16>(&v.operation_id).is_ok()
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
