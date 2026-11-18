use serde_json::{Value, json};

#[cfg(test)]
mod tests;

use crate::{
    native::{
        NativeError, Result,
        demo::{Binding, configuration_only, readiness},
    },
    wire::{Object, hex_bytes, lower_hex},
};

#[derive(Default)]
pub(crate) struct DemoSession {
    receipt: Option<Value>,
    quarantined: bool,
    generation: u64,
    last: Object,
}

enum Failure {
    Business(String),
    Transport(NativeError),
}

type Outcome<T> = std::result::Result<T, Failure>;
type Request<'a> = dyn FnMut(&'static str, Object) -> Result<Value> + 'a;

fn check(condition: bool, code: &'static str) -> Outcome<()> {
    condition
        .then_some(())
        .ok_or_else(|| Failure::Business(code.to_owned()))
}

const INVALID: &str = "invalid_demo_state";
const CHANGED: &str = "demo_context_changed";

impl DemoSession {
    pub(crate) fn status(&self) -> Value {
        json!({"receipt_available":self.receipt.is_some(), "quarantined":self.quarantined,
            "generation":self.generation, "last":self.last})
    }

    pub(crate) fn run(
        &mut self,
        request: &mut Request<'_>,
        binding: &Binding,
        granted: u16,
        arguments: &Object,
    ) -> Result<Value> {
        if self.quarantined {
            return Ok(json!({"ok":false, "error":"demo_quarantined", "result":self.status()}));
        }
        let key = |name| {
            arguments
                .get(name)
                .and_then(Value::as_str)
                .filter(|text| matches!(text.as_bytes(), [b'A'..=b'Z' | b'0'..=b'9']))
        };
        let view = arguments.get("view").and_then(Value::as_str);
        let (
            Some(original),
            Some(mapped),
            Some(view @ ("main" | "item_setup" | "mapping_advanced")),
        ) = (key("original_key"), key("mapping_key"), view)
        else {
            return Ok(json!({"ok":false, "error":"invalid_arguments"}));
        };
        self.generation += 1;
        let mut id = [0u8; 16];
        getrandom::fill(&mut id).map_err(|_| NativeError::new("random_unavailable"))?;
        self.last = json!({"scene_id":lower_hex(&id), "generation":self.generation,
            "stage":"preflight", "reset_verified":false})
        .as_object()
        .unwrap()
        .clone();
        let had_receipt = self.receipt.is_some();
        let mut scene = Scene {
            session: self,
            request,
            binding,
            granted,
            view,
            initial_sources: Value::Null,
            baseline: Value::Null,
            template: Value::Null,
            owned: None,
            opened_ref: None,
            operations: Object::new(),
            mutating: false,
            reset_started: false,
            reset_verified: false,
        };
        let result = scene.execute(original, mapped, had_receipt);
        scene.finish(result, had_receipt)
    }
}

struct Scene<'a, 'client> {
    session: &'a mut DemoSession,
    request: &'a mut Request<'client>,
    binding: &'a Binding,
    granted: u16,
    view: &'a str,
    initial_sources: Value,
    baseline: Value,
    template: Value,
    owned: Option<Value>,
    opened_ref: Option<Value>,
    operations: Object,
    mutating: bool,
    reset_started: bool,
    reset_verified: bool,
}

impl Scene<'_, '_> {
    fn stage(&mut self, phase: &str) {
        self.session.last.insert("stage".into(), phase.into());
        self.session
            .last
            .insert("operations".into(), self.operations.clone().into());
    }

    fn call(&mut self, phase: &str, method: &'static str, args: Value) -> Outcome<Value> {
        self.stage(phase);
        let args = args
            .as_object()
            .expect("fixed scene arguments are objects")
            .clone();
        let mut response = (self.request)(method, args).map_err(Failure::Transport)?;
        if response["ok"] != true {
            return Err(Failure::Business(
                response["error"].as_str().unwrap_or(INVALID).to_owned(),
            ));
        }
        check(response["result"].is_object(), INVALID)?;
        Ok(response["result"].take())
    }

    fn observe(&mut self, dirty: bool) -> Outcome<(Value, Value)> {
        let windows = self.call("observe_windows", "qkm_query", json!({"entity":"windows"}))?;
        let state = self.call("observe_state", "qkm_get_state", json!({}))?;
        let sources = self.call(
            "observe_sources",
            "qkm_query",
            json!({"entity":"mapping_sources","limit":100}),
        )?;
        let reason = readiness(
            self.binding,
            &windows,
            &state,
            &sources,
            self.granted,
            dirty,
        );
        check(reason == "ready", reason)?;
        let context = self.call(
            "observe_context",
            "qkm_query",
            json!({"entity":"demo_context"}),
        )?;
        check(
            context["source_snapshot"] == sources["source_snapshot"],
            CHANGED,
        )?;
        check(
            context["reference_scope"] == "non_row_save_footprint_only"
                && context["mapping_rows_excluded"] == true
                && context["write_authority"] == false
                && context["context_revision"]
                    .as_str()
                    .is_some_and(|text| hex_bytes::<32>(text).is_ok()),
            INVALID,
        )?;
        Ok((sources, context["context_revision"].clone()))
    }

    fn capabilities(&mut self) -> Outcome<()> {
        let response = self.call("capabilities", "qkm_get_capabilities", json!({}))?;
        check(
            configuration_only(&response["execution_policy"]),
            "demo_configuration_only_required",
        )?;
        let caps = response["business_capabilities"].as_array();
        let capability =
            |name: &str| caps.and_then(|items| items.iter().find(|item| item["id"] == name));
        let kind = if self.view == "mapping_advanced" {
            "settings"
        } else {
            "editor"
        };
        let available = [
            "mapping.add.preview",
            "mapping.add.apply",
            "mapping.delete.preview",
            "mapping.delete.apply",
            &format!("mapping.{kind}.open"),
            &format!("mapping.{kind}.close"),
            "windows.capture",
            "demo.context.read",
        ]
        .iter()
        .all(|name| capability(name).is_some());
        let bindings = ["mapping.add.apply", "mapping.delete.apply"]
            .iter()
            .all(|name| {
                capability(name)
                    .is_some_and(|item| item["source_binding_result"] == "commit_snapshot_or_null")
            });
        let role = capability("windows.capture").is_some_and(|capture| {
            capture["arguments_schema"]["properties"]["role"]["enum"]
                .as_array()
                .is_some_and(|roles| roles.iter().any(|role| role == self.view))
        });
        check(available && bindings && role, "unsupported_capability")
    }

    fn receipt(&self, snapshot: &Value) -> Value {
        json!({"target":self.binding.target, "source_snapshot":snapshot, "context_revision":self.baseline})
    }

    fn post_binding(&self, operation: &Value, row_added: bool) -> Outcome<Value> {
        check(
            operation["status"] == "applied",
            "demo_operation_not_applied",
        )?;
        let result = &operation["result"];
        check(
            result["modified"] == true
                && result["saved"] == false
                && result["started_by_operation"] == false,
            INVALID,
        )?;
        let bound = &result["source_binding_after"];
        check(
            result["source_binding_status"] == "captured" && bound.is_object(),
            "demo_source_binding_unavailable",
        )?;
        let snapshot = &bound["source_snapshot"];
        let revision = snapshot["revision"].as_str().unwrap_or_default();
        let row = if row_added {
            json!(format!("{revision}:row:0:0"))
        } else {
            Value::Null
        };
        check(
            hex_bytes::<32>(revision).is_ok()
                && snapshot["instance"] == self.binding.target["instance"]
                && bound["reference_scope"] == "commit_snapshot_only"
                && bound["source_tab_ref"] == format!("{revision}:source:0")
                && bound["source_ref"] == row,
            INVALID,
        )?;
        Ok(bound.clone())
    }

    fn verify_owned(&mut self) -> Outcome<()> {
        let owned = self.owned.clone().expect("checked commit binding");
        let snapshot = &owned["source_snapshot"];
        let revision = snapshot["revision"].as_str().expect("checked revision");
        let mut expected = self.initial_sources["items"]
            .as_array()
            .expect("checked source items")
            .clone();
        for (index, item) in expected.iter_mut().enumerate() {
            item["source_tab_ref"] = format!("{revision}:source:{index}").into();
            item["row_count"] = u64::from(index == 0).into();
        }
        let listing = self.call(
            "verify_owned_sources",
            "qkm_query",
            json!({"entity":"mapping_sources","source_revision":revision,"limit":100}),
        )?;
        check(
            listing["source_snapshot"] == *snapshot
                && listing["items"] == Value::Array(expected)
                && listing["total"] == 2
                && listing["next_offset"].is_null(),
            CHANGED,
        )?;
        let rows = self.call(
            "verify_owned_row",
            "qkm_query",
            json!({"entity":"mappings","limit":100,
            "source_revision":revision,"source_tab_ref":owned["source_tab_ref"]}),
        )?;
        let mut row = self.template.clone();
        row["source_ref"] = owned["source_ref"].clone();
        row["source_tab_ref"] = owned["source_tab_ref"].clone();
        row["source_row"] = 0.into();
        check(
            rows["source_snapshot"] == *snapshot
                && rows["items"] == json!([row])
                && rows["total"] == 1
                && rows["next_offset"].is_null(),
            CHANGED,
        )?;
        let context = self.call(
            "verify_owned_context",
            "qkm_query",
            json!({"entity":"demo_context"}),
        )?;
        check(
            context["source_snapshot"] == *snapshot && context["context_revision"] == self.baseline,
            CHANGED,
        )
    }

    fn remember_operation(&mut self, name: &str, preview: &Value) -> Outcome<()> {
        check(
            ["operation_id", "preview_id"].iter().all(|key| {
                preview[*key]
                    .as_str()
                    .is_some_and(|text| hex_bytes::<16>(text).is_ok())
            }),
            INVALID,
        )?;
        self.operations
            .insert(name.into(), preview["operation_id"].clone());
        Ok(())
    }

    fn open_or_observe(&mut self, owned: &Value) -> Outcome<Value> {
        if self.view == "main" {
            let windows = self.call(
                "observe_capture_window",
                "qkm_query",
                json!({"entity":"windows"}),
            )?;
            check(
                windows["target"] == self.binding.target
                    && windows["private_background_available"] == true,
                INVALID,
            )?;
            let main = windows["items"]
                .as_array()
                .and_then(|items| items.iter().find(|item| item["role"] == "main"));
            let Some(main) = main.filter(|item| {
                item["minimized"] == false
                    && ["qt_visible", "native_ready", "native_visible"]
                        .iter()
                        .all(|key| item[*key] == true)
            }) else {
                return Err(Failure::Business("window_unavailable".into()));
            };
            return Ok(main["window_ref"].clone());
        }
        let advanced = self.view == "mapping_advanced";
        let (stage, method, key) = if advanced {
            (
                "open_settings",
                "qkm_open_mapping_settings",
                "source_tab_ref",
            )
        } else {
            ("open_editor", "qkm_open_mapping_editor", "source_ref")
        };
        let opened = self.call(
            stage,
            method,
            json!({(key):owned[key],"presentation":"private_background"}),
        )?;
        let reference = &opened["window"]["window_ref"];
        if opened["target"] != self.binding.target
            || !reference
                .as_str()
                .is_some_and(|value| hex_bytes::<32>(value).is_ok())
            || (advanced
                && (opened["opened"] != true
                    || opened["activation_requested"] != false
                    || opened["source_context"] != owned["source_snapshot"]
                    || opened["source_tab_ref"] != owned["source_tab_ref"]))
        {
            // An untrusted success record cannot establish ownership for cleanup.
            return Err(Failure::Transport(NativeError::new("invalid_message")));
        }
        self.opened_ref = Some(reference.clone());
        Ok(reference.clone())
    }

    fn reset(&mut self) -> Outcome<Value> {
        self.reset_started = true;
        let owned = self.owned.clone().expect("checked commit binding");
        if let Some(reference) = &self.opened_ref {
            let (stage, method, key) = if self.view == "mapping_advanced" {
                (
                    "close_settings",
                    "qkm_close_mapping_settings",
                    "source_tab_ref",
                )
            } else {
                ("close_editor", "qkm_close_mapping_editor", "source_ref")
            };
            let args = json!({(key):owned[key],"window_ref":reference,"presentation":"private_background"});
            let closed = self.call(stage, method, args)?;
            check(
                closed["closed"] == true && closed["target"] == self.binding.target,
                INVALID,
            )?;
            self.opened_ref = None;
        }
        self.verify_owned()?;
        let preview = self.call("preview_cleanup", "qkm_preview_changes", json!({"action":"mapping.delete",
            "source_revision":owned["source_snapshot"]["revision"],"source_tab_ref":owned["source_tab_ref"],
            "source_refs":[owned["source_ref"]]}))?;
        let description = &preview["description"];
        check(description["action"] == "mapping.delete" && description["changes"].as_array()
            .is_some_and(|changes| matches!(changes.as_slice(), [change] if change["source_ref"] == owned["source_ref"]
                && change["mapping"] == self.template)), INVALID)?;
        self.remember_operation("cleanup", &preview)?;
        let deleted = self.call(
            "apply_cleanup",
            "qkm_apply_changes",
            json!({"preview_id":preview["preview_id"],"confirm_delete":true}),
        )?;
        let final_binding = self.post_binding(&deleted, false)?;
        let (sources, context) = self.observe(true)?;
        let snapshot = &final_binding["source_snapshot"];
        check(
            sources["source_snapshot"] == *snapshot && context == self.baseline,
            CHANGED,
        )?;
        self.session.receipt = Some(self.receipt(snapshot));
        self.reset_verified = true;
        Ok(
            json!({"verified":true,"source_snapshot":snapshot,"context_revision":context,
            "saved":false,"started_by_operation":false,"dirty_retained":true}),
        )
    }

    fn execute(&mut self, original: &str, mapped: &str, had_receipt: bool) -> Outcome<Value> {
        self.capabilities()?;
        (self.initial_sources, self.baseline) = self.observe(had_receipt)?;
        if had_receipt {
            check(
                self.session.receipt.as_ref()
                    == Some(&self.receipt(&self.initial_sources["source_snapshot"])),
                CHANGED,
            )?;
        }
        self.session.receipt = None;
        let preview = self.call("preview_mapping", "qkm_preview_changes", json!({
            "original_key":original,"mapping_key":mapped,"source_revision":self.initial_sources["source_snapshot"]["revision"],
            "source_tab_ref":self.initial_sources["items"][0]["source_tab_ref"]}))?;
        let description = &preview["description"];
        let changes = description["changes"].as_array().map(Vec::as_slice);
        check(
            description["action"] == "mapping.add"
                && description["confirmation_required"] == false
                && matches!(changes, Some([change]) if change["kind"] == "insert_mapping" && change["source_row"] == 0),
            INVALID,
        )?;
        self.template = description["changes"][0]["mapping"].clone();
        check(
            self.template["fields"]["KeyMapData_OriginalKeys"] == original
                && self.template["fields"]["KeyMapData_MappingKeys"] == json!([mapped]),
            INVALID,
        )?;
        self.remember_operation("create", &preview)?;
        self.mutating = true;
        let added = self.call(
            "apply_mapping",
            "qkm_apply_changes",
            json!({"preview_id":preview["preview_id"]}),
        )?;
        let owned = self.post_binding(&added, true)?;
        self.owned = Some(owned.clone());
        self.verify_owned()?;
        let reference = self.open_or_observe(&owned)?;
        let stage = match self.view {
            "main" => "capture_main",
            "mapping_advanced" => "capture_settings",
            _ => "capture_editor",
        };
        let mut image = self.call(stage, "qkm_capture_window", json!({"role":self.view,"window_ref":reference,
            "source_revision":owned["source_snapshot"]["revision"],"source_tab_ref":owned["source_tab_ref"]}))?;
        check(
            image["target"] == self.binding.target && image["role"] == self.view,
            INVALID,
        )?;
        if self.view == "item_setup" {
            check(
                image["edit_target"]["source_ref"] == owned["source_ref"],
                INVALID,
            )?;
        } else if self.view == "mapping_advanced" {
            check(
                image["settings_editor_unchanged"] == true
                    && image["settings_scope"]
                        == "live_loaded_settings_not_committed_by_source_revision"
                    && image["source_context"] == owned["source_snapshot"],
                "demo_settings_changed",
            )?;
        }
        let cleanup = self.reset()?;
        self.stage("completed");
        self.session
            .last
            .insert("reset_verified".into(), true.into());
        let mut summary = Value::Object(self.session.last.clone());
        summary["original_key"] = original.into();
        summary["mapping_key"] = mapped.into();
        summary["view"] = self.view.into();
        summary["cleanup"] = cleanup;
        summary["delivery"] = "mcp_image_only_gateway_delivery_unverified".into();
        image["demo_scene"] = summary;
        Ok(json!({"ok":true,"result":image}))
    }

    fn quarantine_if_unproved(&mut self, had_receipt: bool) {
        if (self.mutating || had_receipt) && !self.reset_verified {
            self.session.quarantined = true;
            self.session.receipt = None;
        }
    }

    fn finish(&mut self, result: Outcome<Value>, had_receipt: bool) -> Result<Value> {
        let code = match result {
            Ok(image) => return Ok(image),
            Err(Failure::Transport(error)) => {
                self.quarantine_if_unproved(had_receipt);
                self.session
                    .last
                    .insert("transport_or_internal_failure".into(), true.into());
                return Err(error);
            }
            Err(Failure::Business(code)) => code,
        };
        let failed = self
            .session
            .last
            .get("stage")
            .cloned()
            .unwrap_or(Value::Null);
        let mut cleanup_error = Value::Null;
        if self.owned.is_some() && !self.reset_started {
            match self.reset() {
                Ok(_) => {}
                Err(Failure::Business(error)) => cleanup_error = error.into(),
                Err(Failure::Transport(error)) => {
                    self.quarantine_if_unproved(had_receipt);
                    self.session.last.insert("failed_stage".into(), failed);
                    self.session.last.insert("error".into(), code.into());
                    self.session
                        .last
                        .insert("transport_or_internal_failure".into(), true.into());
                    return Err(error);
                }
            }
        }
        self.quarantine_if_unproved(had_receipt);
        self.stage("failed");
        self.session.last.extend([
            ("failed_stage".into(), failed),
            ("error".into(), code.clone().into()),
            ("cleanup_error".into(), cleanup_error),
            ("reset_verified".into(), self.reset_verified.into()),
        ]);
        Ok(json!({"ok":false,"error":code,"result":self.session.status()}))
    }
}
