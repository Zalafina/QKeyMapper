use super::*;

#[derive(Clone, Copy, Default)]
enum Fault {
    #[default]
    None,
    CaptureTimeout,
    Cancelled,
    MissingBinding,
    ForeignContext,
}

struct Product {
    target: Value,
    revision: u64,
    row: Option<Value>,
    pending: Option<Value>,
    open: bool,
    calls: Vec<&'static str>,
    fault: Fault,
}

impl Default for Product {
    fn default() -> Self {
        Self {
            target: json!({"instance":"a".repeat(32),"desktop":"QkmDemo"}),
            revision: 1,
            row: None,
            pending: None,
            open: false,
            calls: Vec::new(),
            fault: Fault::None,
        }
    }
}

impl Product {
    fn execute(&mut self, session: &mut DemoSession, view: &str, scopes: u16) -> Result<Value> {
        let binding = Binding {
            target: self.target.clone(),
            configuration: "Demo".into(),
        };
        let args = json!({"original_key":"A","mapping_key":"B","view":view});
        session.run(
            &mut |method, args| self.reply(method, args),
            &binding,
            scopes,
            args.as_object().unwrap(),
        )
    }

    fn reply(&mut self, method: &'static str, args: Object) -> Result<Value> {
        self.calls.push(method);
        let revision = format!("{:064x}", self.revision);
        let policy = json!({"startup_purpose":"configuration_only_demo","mapping_allowed":false,
            "system_input_allowed":false,"device_initialization_allowed":false,"system_setting_writes_allowed":false});
        let configuration =
            json!({"group":"Demo","kind":"saved_profile","has_unsaved_changes":self.revision > 1});
        let load = json!({"group":"Demo","outcome":"accepted"});
        let snapshot = |revision, config| {
            json!({"instance":self.target["instance"],"revision":revision,
            "read_basis":"pristine_idle","common_feature_enabled":false,"configuration_selection":config,"mapping_load_context":load})
        };
        let context = snapshot(&revision, &configuration);
        let tab = format!("{revision}:source:0");
        let row_ref = format!("{revision}:row:0:0");
        let window = "b".repeat(64);
        let args = Value::Object(args);
        let result = match method {
            "qkm_get_capabilities" => {
                let mut caps: Vec<_> = [
                    "mapping.add.preview",
                    "mapping.add.apply",
                    "mapping.delete.preview",
                    "mapping.delete.apply",
                    "mapping.editor.open",
                    "mapping.editor.close",
                    "mapping.settings.open",
                    "mapping.settings.close",
                    "demo.context.read",
                ]
                .iter()
                .map(|id| json!({"id":id,"source_binding_result":"commit_snapshot_or_null"}))
                .collect();
                caps.push(
                    json!({"id":"windows.capture","arguments_schema":{"properties":{"role":{
                    "enum":["main","item_setup","mapping_advanced"]}}}}),
                );
                json!({"business_capabilities":caps,"execution_policy":policy})
            }
            "qkm_get_state" => json!({"instance":self.target["instance"],"execution_policy":policy,
                "configuration":configuration,"mapping_load_context":load,
                "current_tab":{"index":0},"run":{"gui_state":"idle","worker_state_sample":"stopped"}}),
            "qkm_query" if args["entity"] == "windows" => json!({"target":self.target,
            "private_background_available":true,"items":[
                {"role":"main","qt_visible":true,"native_ready":true,"native_visible":true,"minimized":false,"window_ref":window},
                {"role":"item_setup","qt_visible":self.open}
            ]}),
            "qkm_query" if args["entity"] == "mapping_sources" => {
                json!({"source_snapshot":context,"offset":0,
                "total":2,"next_offset":null,"items":[
                    {"kind":"normal","attached_to_ui":true,"ui_index":0,"include_common":false,"source_tab_ref":tab,"row_count":u8::from(self.row.is_some())},
                    {"kind":"common","attached_to_ui":false,"ui_index":null,"source_tab_ref":format!("{revision}:source:1"),"row_count":0}
                ]})
            }
            "qkm_query" if args["entity"] == "mappings" => {
                let mut row = self.row.clone().expect("owned row");
                row["source_ref"] = row_ref.into();
                row["source_tab_ref"] = tab.into();
                row["source_row"] = 0.into();
                json!({"source_snapshot":context,"total":1,"next_offset":null,"items":[row]})
            }
            "qkm_query" if args["entity"] == "demo_context" => {
                let revision = if matches!(self.fault, Fault::ForeignContext) && self.row.is_some()
                {
                    "d"
                } else {
                    "c"
                };
                json!({"source_snapshot":context,"context_revision":revision.repeat(64),"reference_scope":"non_row_save_footprint_only",
                    "mapping_rows_excluded":true,"write_authority":false})
            }
            "qkm_preview_changes" => {
                let action = args["action"].as_str().unwrap_or("mapping.add");
                let change = if action == "mapping.add" {
                    let row = json!({"fields":{"KeyMapData_OriginalKeys":args["original_key"],
                        "KeyMapData_MappingKeys":[args["mapping_key"]]},"derived":{}});
                    self.pending = Some(row.clone());
                    json!({"kind":"insert_mapping","source_row":0,"mapping":row})
                } else {
                    assert_eq!(action, "mapping.delete");
                    assert_eq!(args["source_refs"], json!([row_ref]));
                    self.pending = None;
                    json!({"source_ref":row_ref,"mapping":self.row})
                };
                let id = format!("{:032x}", self.revision);
                json!({"preview_id":id,"operation_id":id,"description":{"action":action,"confirmation_required":false,"changes":[change]}})
            }
            "qkm_apply_changes" => {
                assert_eq!(args["confirm_delete"] == true, self.pending.is_none());
                self.row = self.pending.take();
                self.revision += 1;
                let revision = format!("{:064x}", self.revision);
                let mut configuration = configuration.clone();
                configuration["has_unsaved_changes"] = true.into();
                let context = snapshot(&revision, &configuration);
                let binding = if matches!(self.fault, Fault::MissingBinding) {
                    Value::Null
                } else {
                    json!({"reference_scope":"commit_snapshot_only","source_snapshot":context,
                        "source_tab_ref":format!("{revision}:source:0"),
                        "source_ref":self.row.as_ref().map(|_| format!("{revision}:row:0:0"))})
                };
                json!({"status":"applied","result":{"modified":true,"saved":false,"started_by_operation":false,
                    "source_binding_status":"captured","source_binding_after":binding}})
            }
            "qkm_open_mapping_editor" | "qkm_open_mapping_settings" => {
                self.open = true;
                json!({"target":self.target,"opened":true,"activation_requested":false,
                    "window":{"window_ref":window},"source_context":context,"source_tab_ref":tab})
            }
            "qkm_close_mapping_editor" | "qkm_close_mapping_settings" => {
                assert!(self.open);
                self.open = false;
                json!({"target":self.target,"closed":true})
            }
            "qkm_capture_window" => {
                match self.fault {
                    Fault::CaptureTimeout => {
                        return Ok(json!({"ok":false,"error":"capture_timeout"}));
                    }
                    Fault::Cancelled => return Err(NativeError::new("cancelled")),
                    _ => {}
                }
                json!({"target":self.target,"role":args["role"],"edit_target":{"source_ref":row_ref},
                    "settings_editor_unchanged":true,"settings_scope":"live_loaded_settings_not_committed_by_source_revision",
                    "source_context":context,"png_base64":"synthetic"})
            }
            _ => panic!("unexpected method {method}"),
        };
        Ok(json!({"ok":true,"result":result}))
    }
}

#[test]
fn same_session_can_reuse_only_a_proved_cleanup() {
    let mut product = Product::default();
    let mut demo = DemoSession::default();
    for (index, view) in ["item_setup", "mapping_advanced", "main"]
        .into_iter()
        .enumerate()
    {
        let outcome = product.execute(&mut demo, view, 387).unwrap();
        assert_eq!(outcome["ok"], true, "{outcome}");
        assert_eq!(outcome["result"]["demo_scene"]["view"], view);
        assert_eq!(outcome["result"]["demo_scene"]["generation"], index + 1);
        assert_eq!(outcome["result"]["demo_scene"]["reset_verified"], true);
        assert!(product.row.is_none() && !product.open);
    }
    assert_eq!(
        product
            .calls
            .iter()
            .filter(|&&method| method == "qkm_apply_changes")
            .count(),
        6
    );
    assert_eq!(
        product
            .calls
            .iter()
            .filter(|&&method| method.starts_with("qkm_open"))
            .count(),
        2
    );
    assert_eq!(demo.status()["receipt_available"], true);
    let fresh = product
        .execute(&mut DemoSession::default(), "item_setup", 387)
        .unwrap();
    assert_eq!(fresh["error"], "demo_baseline_dirty");
}

#[test]
fn known_capture_error_recovers_while_cancellation_blocks_reuse() {
    let mut product = Product {
        fault: Fault::CaptureTimeout,
        ..Product::default()
    };
    let mut demo = DemoSession::default();
    let outcome = product.execute(&mut demo, "item_setup", 387).unwrap();
    assert_eq!(outcome["error"], "capture_timeout");
    assert_eq!(outcome["result"]["last"]["reset_verified"], true);
    assert!(product.row.is_none() && !product.open);
    product.fault = Fault::Cancelled;
    let error = product
        .execute(&mut demo, "mapping_advanced", 387)
        .unwrap_err();
    assert_eq!(error.code, "cancelled");
    assert!(product.row.is_some() && product.open);
    let calls = product.calls.len();
    assert_eq!(
        product.execute(&mut demo, "item_setup", 387).unwrap()["error"],
        "demo_quarantined"
    );
    assert_eq!(product.calls.len(), calls);
}

#[test]
fn missing_commit_binding_or_foreign_context_cannot_authorize_deletion() {
    for fault in [Fault::MissingBinding, Fault::ForeignContext] {
        let mut product = Product {
            fault,
            ..Product::default()
        };
        let result = product
            .execute(&mut DemoSession::default(), "item_setup", 387)
            .unwrap();
        assert_eq!(result["ok"], false);
        assert_eq!(result["result"]["quarantined"], true);
        assert!(product.row.is_some());
        assert_eq!(
            product
                .calls
                .iter()
                .filter(|&&method| method == "qkm_apply_changes")
                .count(),
            1
        );
        assert!(!product.calls.contains(&"qkm_capture_window"));
    }
    let mut product = Product::default();
    let denied = product
        .execute(&mut DemoSession::default(), "item_setup", 1)
        .unwrap();
    assert_eq!(denied["error"], "permission_denied");
    assert!(!product.calls.contains(&"qkm_preview_changes"));
}
