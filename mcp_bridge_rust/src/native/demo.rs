use std::path::{Path, PathBuf};

use serde_json::{Value, json};

use super::{
    Cancel, Connection, Discovery, NativeError, Result,
    discovery::{normalized, read_private_record},
    own_identity,
};
use crate::wire::{Object, decimal, lower_hex};

pub enum DemoRecord {
    Explicit(PathBuf),
    Product,
}

pub(crate) struct Binding {
    pub target: Value,
    pub configuration: String,
}

const PURPOSE: &str = "configuration_only_demo";
const DEMO_SCOPES: u16 = 1 | 2 | 128 | 256;

pub(crate) fn configuration_only(policy: &Value) -> bool {
    *policy
        == json!({"startup_purpose":PURPOSE, "mapping_allowed":false, "system_input_allowed":false,
        "device_initialization_allowed":false, "system_setting_writes_allowed":false})
}

impl Connection {
    pub(crate) fn demo_binding(&self, mode: &DemoRecord, executable: &Path) -> Result<Binding> {
        let target = self.pipe.target();
        let actual_record;
        let record = match mode {
            DemoRecord::Explicit(path) => path,
            DemoRecord::Product => {
                actual_record = self
                    .discovery_path
                    .with_file_name(format!("demo-{}.json", lower_hex(&target.instance)));
                &actual_record
            }
        };
        let own = own_identity()?;
        let contents = read_private_record(record, &own, 8192)
            .map_err(|_| NativeError::new("demo_record_rejected"))?;
        let mismatch = NativeError::new("demo_record_mismatch");
        let binding = bind(&contents, target, executable).ok_or(mismatch)?;
        self.pipe.verify()?;
        Ok(binding)
    }

    pub(crate) fn demo_state(
        &mut self,
        mode: &DemoRecord,
        executable: &Path,
        cancel: &Cancel,
    ) -> Result<Value> {
        let mut result = json!({"configured":true, "ready":false, "reference_scope":"preflight_observation_only"});
        let binding = match self.demo_binding(mode, executable) {
            Ok(binding) => binding,
            Err(error) if matches!(error.code, "demo_record_rejected" | "demo_record_mismatch") => {
                result["reason"] = error.code.into();
                return Ok(json!({"ok":true, "result":result}));
            }
            Err(error) => return Err(error),
        };
        let mut sample = Vec::with_capacity(3);
        for (method, arguments) in [
            ("qkm_query", json!({"entity":"windows"})),
            ("qkm_get_state", json!({})),
            (
                "qkm_query",
                json!({"entity":"mapping_sources", "limit":100}),
            ),
        ] {
            let mut response = self.call(method, arguments.as_object().unwrap().clone(), cancel)?;
            if response["ok"] != true {
                result["reason"] = response["error"]
                    .as_str()
                    .unwrap_or("invalid_demo_state")
                    .into();
                return Ok(json!({"ok":true, "result":result}));
            }
            sample.push(response["result"].take());
        }
        let [windows, state, sources] = sample.as_slice() else {
            unreachable!("exact sample count")
        };
        let reason = readiness(&binding, windows, state, sources, self.scopes(), false);
        result["reason"] = reason.into();
        if !matches!(reason, "demo_record_mismatch" | "invalid_demo_state") {
            result["ready"] = (reason == "ready").into();
            result["target"] = binding.target;
            result["configuration"] = binding.configuration.into();
            result["source_snapshot"] = sources
                .get("source_snapshot")
                .cloned()
                .unwrap_or_else(|| json!({}));
        }
        Ok(json!({"ok":true, "result":result}))
    }
}

fn bind(value: &Object, discovery: &Discovery, expected: &Path) -> Option<Binding> {
    let text = |key| value.get(key)?.as_str();
    let instance = lower_hex(&discovery.instance);
    if value.len() != 9
        || value.get("version")?.as_u64() != Some(2)
        || text("startup_purpose")? != PURPOSE
        || text("instance")? != instance
        || decimal(text("pid")?, false).ok()? != u64::from(discovery.server.stamp.pid.get())
        || decimal(text("created")?, false).ok()? != discovery.server.stamp.created.get()
        || decimal(text("session")?, true).ok()? != u64::from(discovery.server.session)
    {
        return None;
    }
    let executable = text("executable")?;
    let normalized_path = normalized(Path::new(executable)).ok()?;
    if normalized_path != normalized(&discovery.server.executable).ok()?
        || normalized_path != normalized(expected).ok()?
    {
        return None;
    }
    let desktop = text("desktop")?;
    if !(1..=120).contains(&desktop.len())
        || desktop.eq_ignore_ascii_case("Default")
        || !desktop
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || b"_.-".contains(&byte))
    {
        return None;
    }
    let configuration = text("configuration")?;
    if !(1..=1024).contains(&configuration.chars().count()) || configuration.contains('\0') {
        return None;
    }
    Some(Binding {
        target: json!({"instance":instance, "pid":text("pid")?, "created":text("created")?,
            "executable":executable, "desktop":desktop}),
        configuration: configuration.to_owned(),
    })
}

pub(crate) fn readiness(
    binding: &Binding,
    windows: &Value,
    state: &Value,
    sources: &Value,
    granted: u16,
    expected_dirty: bool,
) -> &'static str {
    if ![windows, state, sources]
        .iter()
        .all(|value| value.is_object())
    {
        return "invalid_demo_state";
    }
    if windows["target"] != binding.target {
        return "demo_record_mismatch";
    }
    if granted & DEMO_SCOPES != DEMO_SCOPES {
        return "permission_denied";
    }
    if granted != DEMO_SCOPES || !configuration_only(&state["execution_policy"]) {
        return "demo_configuration_only_required";
    }
    if windows["private_background_available"] != true {
        return "unsupported_desktop";
    }
    if state["instance"] != binding.target["instance"] {
        return "demo_record_mismatch";
    }
    let (configuration, load) = (&state["configuration"], &state["mapping_load_context"]);
    if !configuration.is_object() || !load.is_object() {
        return "invalid_demo_state";
    }
    if configuration["group"] != binding.configuration
        || configuration["kind"] != "saved_profile"
        || load["group"] != binding.configuration
        || load["outcome"] != "accepted"
    {
        return "demo_configuration_mismatch";
    }
    if configuration["has_unsaved_changes"] != expected_dirty {
        return "demo_baseline_dirty";
    }
    let (run, snapshot) = (&state["run"], &sources["source_snapshot"]);
    if !run.is_object() || !snapshot.is_object() {
        return "invalid_demo_state";
    }
    if run["gui_state"] != "idle"
        || run["worker_state_sample"] != "stopped"
        || !matches!(
            snapshot["read_basis"].as_str(),
            Some("pristine_idle" | "completed_cleanup")
        )
    {
        return "context_busy";
    }
    let Some([source, common]) = sources["items"].as_array().map(Vec::as_slice) else {
        return "demo_baseline_not_empty";
    };
    if snapshot["instance"] != binding.target["instance"]
        || snapshot["common_feature_enabled"] != false
        || sources["total"].as_u64() != Some(2)
        || sources["offset"].as_u64() != Some(0)
        || !sources["next_offset"].is_null()
        || !source.is_object()
        || !common.is_object()
    {
        return "demo_baseline_not_empty";
    }
    if snapshot["configuration_selection"] != *configuration
        || snapshot["mapping_load_context"] != *load
    {
        return "context_busy";
    }
    if source["attached_to_ui"] != true
        || source["kind"] != "normal"
        || source["ui_index"].as_u64() != Some(0)
        || source["row_count"].as_u64() != Some(0)
        || source["include_common"] != false
        || state["current_tab"]["index"].as_u64() != Some(0)
        || common["attached_to_ui"] != false
        || common["kind"] != "common"
        || !common["ui_index"].is_null()
        || common["row_count"].as_u64() != Some(0)
    {
        return "demo_baseline_not_empty";
    }
    let Some([main, editor]) = windows["items"].as_array().map(Vec::as_slice) else {
        return "window_unavailable";
    };
    if main["role"] != "main"
        || editor["role"] != "item_setup"
        || !["qt_visible", "native_ready", "native_visible"]
            .iter()
            .all(|key| main[*key] == true)
        || main["minimized"] != false
        || editor["qt_visible"] != false
    {
        return "window_unavailable";
    }
    "ready"
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn creation_record_matches_only_the_authenticated_target() {
        let server = own_identity().unwrap();
        let discovery = Discovery {
            instance: [0xab; 16],
            server,
            pipe: String::new(),
        };
        let path = &discovery.server.executable;
        let mut record = json!({"version":2, "startup_purpose":PURPOSE, "executable":path,
            "pid":discovery.server.stamp.pid.to_string(), "created":discovery.server.stamp.created.to_string(),
            "session":discovery.server.session.to_string(), "instance":lower_hex(&discovery.instance),
            "desktop":"QkmPrivate_001", "configuration":"示例"});
        let original = record.as_object().unwrap().clone();
        let binding = bind(&original, &discovery, path).unwrap();
        assert_eq!(binding.target["desktop"], "QkmPrivate_001");
        assert_eq!(binding.configuration, "示例");
        for (key, substitute) in [
            ("desktop", json!("dEfAuLt")),
            ("executed", json!(true)),
            ("startup_purpose", json!("normal")),
            ("version", json!(1)),
            ("configuration", json!("\u{0}")),
            ("pid", json!("0")),
            ("created", json!("1")),
            ("executable", json!("relative.exe")),
            ("instance", json!("a".repeat(32))),
        ] {
            record[key] = substitute;
            assert!(
                bind(record.as_object().unwrap(), &discovery, path).is_none(),
                "{key}"
            );
            record = Value::Object(original.clone());
        }
    }

    #[test]
    fn demo_policy_is_proved_before_interpreting_an_empty_source() {
        let instance = "a".repeat(32);
        let binding = Binding {
            target: json!({"instance":instance,"desktop":"QkmPrivate"}),
            configuration: "Demo".to_owned(),
        };
        let windows = json!({"target":binding.target,"private_background_available":true,"items":[
            {"role":"main","qt_visible":true,"native_ready":true,"native_visible":true,"minimized":false},
            {"role":"item_setup","qt_visible":false}
        ]});
        let mut state = json!({"instance":instance,"execution_policy":{
            "startup_purpose":PURPOSE,"mapping_allowed":false,"system_input_allowed":false,
            "device_initialization_allowed":false,"system_setting_writes_allowed":false},
            "configuration":{"group":"Demo","kind":"saved_profile","has_unsaved_changes":false},
            "mapping_load_context":{"group":"Demo","outcome":"accepted"},
            "run":{"gui_state":"idle","worker_state_sample":"stopped"},"current_tab":{"index":0}});
        let sources = json!({"source_snapshot":{"instance":instance,"common_feature_enabled":false,
            "read_basis":"pristine_idle","configuration_selection":state["configuration"],
            "mapping_load_context":state["mapping_load_context"]}, "total":2,"offset":0,"next_offset":null,
            "items":[{"attached_to_ui":true,"kind":"normal","ui_index":0,"row_count":0,"include_common":false},
                     {"attached_to_ui":false,"kind":"common","ui_index":null,"row_count":0}]});
        assert_eq!(
            readiness(&binding, &windows, &state, &sources, DEMO_SCOPES, false),
            "ready"
        );
        assert_eq!(
            readiness(&binding, &windows, &state, &sources, 1, false),
            "permission_denied"
        );
        assert_eq!(
            readiness(&binding, &windows, &state, &sources, DEMO_SCOPES | 4, false),
            "demo_configuration_only_required"
        );
        state["configuration"]["has_unsaved_changes"] = true.into();
        assert_eq!(
            readiness(&binding, &windows, &state, &sources, DEMO_SCOPES, false),
            "demo_baseline_dirty"
        );
        state["execution_policy"]["mapping_allowed"] = true.into();
        assert_eq!(
            readiness(&binding, &windows, &state, &sources, DEMO_SCOPES, true),
            "demo_configuration_only_required"
        );
    }
}
