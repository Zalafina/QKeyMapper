use std::{
    collections::{BTreeMap, BTreeSet},
    ffi::OsString,
    path,
};

use qkeymapper_mcp_rust::{
    bridge::{Bridge, Worker},
    native::{DemoOptions, DemoRecord, NativeError, Target},
    wire::Object,
};
use rmcp::{
    ErrorData, RoleServer, ServerHandler,
    model::{
        CallToolRequestParams, CallToolResponse, CallToolResult, ContentBlock, Implementation,
        ListToolsResult, PaginatedRequestParams, ServerCapabilities, ServerConfig, Tool,
    },
    service::RequestContext,
};
use serde::Deserialize;
use serde_json::{Value, json};

// The largest setting name can occupy roughly 384 KiB with JSON Unicode escapes.
const MAX_REQUEST_LINE: usize = 512 * 1024;

#[derive(Deserialize)]
struct Contracts {
    tools: Vec<Tool>,
    feedback: BTreeMap<String, (String, String)>,
    reviewed_product_errors: BTreeSet<String>,
}

pub fn parse_target(
    args: &mut impl Iterator<Item = OsString>,
) -> Option<(Target, Option<DemoOptions>)> {
    let (mut executable, mut discovery, mut persistent) = (None, None, false);
    let mut demo = None;
    let mut enable_mapping = false;
    while let Some(flag) = args.next() {
        match flag.to_str()? {
            "--executable" if executable.is_none() => {
                executable = Some(path::absolute(args.next()?).ok()?)
            }
            "--discovery-file" if discovery.is_none() && !persistent => {
                let name = path::absolute(args.next()?).ok()?;
                if name.file_name()? != "instance.json" {
                    return None;
                }
                discovery = Some(name);
            }
            "--persistent-target" if discovery.is_none() && !persistent => persistent = true,
            "--demo-record" if demo.is_none() => {
                demo = Some(DemoRecord::Explicit(path::absolute(args.next()?).ok()?));
            }
            "--product-demo-record" if demo.is_none() => demo = Some(DemoRecord::Product),
            "--enable-demo-mapping" if !enable_mapping => enable_mapping = true,
            _ => return None,
        }
    }
    if (discovery.is_none() && !persistent) || (enable_mapping && demo.is_none()) {
        return None;
    }
    Some((
        Target {
            executable: executable?,
            discovery,
        },
        demo.map(|record| DemoOptions {
            record,
            enable_mapping,
        }),
    ))
}

struct Handler {
    bridge: Bridge,
    contracts: Contracts,
    demo_configured: bool,
    demo_mapping: bool,
}

impl ServerHandler for Handler {
    fn get_info(&self) -> ServerConfig {
        ServerConfig::new(ServerCapabilities::builder().enable_tools().build())
            .with_server_info(Implementation::new(
                "QKeyMapper Rust bridge",
                env!("CARGO_PKG_VERSION"),
            ))
            .with_instructions(
                "QKeyMapper Rust IPC candidate. Explicitly connect to the configured target before querying or editing.",
            )
    }

    async fn list_tools(
        &self,
        _: Option<PaginatedRequestParams>,
        _: RequestContext<RoleServer>,
    ) -> Result<ListToolsResult, ErrorData> {
        Ok(ListToolsResult::with_all_items(
            self.contracts.tools.clone(),
        ))
    }

    async fn call_tool(
        &self,
        request: CallToolRequestParams,
        context: RequestContext<RoleServer>,
    ) -> Result<CallToolResponse, ErrorData> {
        let Ok(arguments) =
            crate::tool_args::parse(&request.name, request.arguments.unwrap_or_default())
        else {
            return Ok(
                CallToolResult::error(vec![ContentBlock::text("请求参数不符合接口约束。")]).into(),
            );
        };
        let value = match request.name.as_ref() {
            "qkm_get_connection_state" => self.bridge.state(),
            "qkm_connect" => self.bridge.connect(false),
            "qkm_request_pairing" => self.bridge.connect(true),
            "qkm_disconnect" => self.bridge.disconnect(),
            "qkm_get_capabilities" => {
                self.request("qkm_get_capabilities", arguments, context)
                    .await
            }
            "qkm_get_state" => self.request("qkm_get_state", arguments, context).await,
            "qkm_wait_state" => self.request("qkm_wait_state", arguments, context).await,
            "qkm_get_help" => self.request("qkm_get_help", arguments, context).await,
            "qkm_query" => self.request("qkm_query", arguments, context).await,
            "qkm_get_operation" => self.request("qkm_get_operation", arguments, context).await,
            "qkm_preview_changes" => {
                self.request("qkm_preview_changes", arguments, context)
                    .await
            }
            "qkm_apply_changes" => self.request("qkm_apply_changes", arguments, context).await,
            "qkm_cancel_changes" => self.request("qkm_cancel_changes", arguments, context).await,
            "qkm_open_mapping_editor" => {
                self.request("qkm_open_mapping_editor", arguments, context)
                    .await
            }
            "qkm_close_mapping_editor" => {
                self.request("qkm_close_mapping_editor", arguments, context)
                    .await
            }
            "qkm_open_mapping_settings" => {
                self.request("qkm_open_mapping_settings", arguments, context)
                    .await
            }
            "qkm_close_mapping_settings" => {
                self.request("qkm_close_mapping_settings", arguments, context)
                    .await
            }
            "qkm_capture_window" => self.request("qkm_capture_window", arguments, context).await,
            "qkm_get_demo_state" if self.demo_configured => {
                self.request("qkm_get_demo_state", arguments, context).await
            }
            "qkm_demo_mapping" if self.demo_mapping => {
                self.request("qkm_demo_mapping", arguments, context).await
            }
            _ => return Ok(CallToolResult::error(vec![ContentBlock::text("未知工具。")]).into()),
        };
        let result = if matches!(
            request.name.as_ref(),
            "qkm_capture_window" | "qkm_demo_mapping"
        ) {
            self.contracts.image_tool_result(value)
        } else {
            self.contracts.tool_result(value)
        };
        Ok(result.into())
    }
}

impl Handler {
    async fn request(
        &self,
        method: &'static str,
        arguments: Object,
        context: RequestContext<RoleServer>,
    ) -> Value {
        if context.ct.is_cancelled() {
            return json!({"ok": false, "error": "cancelled"});
        }
        let mut job = match self.bridge.request(method, arguments) {
            Ok(job) => job,
            Err(error) => return json!({"ok": false, "error": error.code}),
        };
        let result = tokio::select! {
            response = &mut job.reply => response,
            _ = context.ct.cancelled() => {
                let _ = job.cancel.signal();
                // The reply is sent only after the native worker has drained the call.
                job.reply.await
            }
        };
        result
            .unwrap_or(Err(NativeError {
                code: "internal_error",
                win32: None,
            }))
            .unwrap_or_else(|error| json!({"ok": false, "error": error.code}))
    }
}

impl Contracts {
    fn image_tool_result(&self, mut value: Value) -> CallToolResult {
        if value["ok"].as_bool() != Some(true) {
            return self.tool_result(value);
        }
        let Some(Value::String(encoded)) = value
            .get_mut("result")
            .and_then(Value::as_object_mut)
            .and_then(|result| result.remove("png_base64"))
        else {
            return self.tool_result(json!({"ok": false, "error": "invalid_image"}));
        };
        let mut result = self.tool_result(value);
        result
            .content
            .insert(0, ContentBlock::image(encoded, "image/png"));
        result
    }

    fn tool_result(&self, mut value: Value) -> CallToolResult {
        if value["ok"].as_bool() != Some(false) && value["accepted"].as_bool() != Some(false) {
            return CallToolResult::structured(value);
        }
        let code = value["error"]
            .as_str()
            .unwrap_or("busy_or_closed")
            .to_owned();
        let (message, step) = self
            .feedback
            .get(&code)
            .map(|(message, step)| (message.as_str(), step.as_str()))
            .unwrap_or((
                "操作未完成。",
                "核对连接状态与实际能力；保留错误码用于调查。",
            ));
        let reviewed = self
            .reviewed_product_errors
            .contains(&code)
            .then(|| value["message"].as_str())
            .flatten()
            .filter(|message| !message.is_empty())
            .unwrap_or(message)
            .to_owned();
        value["ok"] = false.into();
        value["error"] = code.into();
        value["message"] = reviewed.into();
        value["next_steps"] = json!([step]);
        CallToolResult::structured_error(value)
    }
}

pub fn run(target: Target, demo: Option<DemoOptions>) -> Result<(), &'static str> {
    let mut contracts: Contracts = serde_json::from_str(include_str!("tool_catalog.json"))
        .map_err(|_| "Invalid embedded MCP tool catalog")?;
    let demo_configured = demo.is_some();
    let demo_mapping = demo.as_ref().is_some_and(|options| options.enable_mapping);
    contracts.tools.retain(|tool| match tool.name.as_ref() {
        "qkm_get_demo_state" => demo_configured,
        "qkm_demo_mapping" => demo_mapping,
        _ => true,
    });
    let worker = Worker::start(target, demo).map_err(|_| "Could not start the IPC worker")?;
    let client = worker.client.clone();
    let handler = Handler {
        bridge: worker.client.clone(),
        contracts,
        demo_configured,
        demo_mapping,
    };
    let outcome = crate::stdio::run(handler, MAX_REQUEST_LINE, move || client.stop());
    worker.join().map_err(|_| "IPC worker did not finish")?;
    outcome
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn capture_image_is_returned_once_and_errors_have_no_picture() {
        let contracts: Contracts = serde_json::from_str(include_str!("tool_catalog.json")).unwrap();
        let result = serde_json::to_value(
            contracts
                .image_tool_result(json!({"ok": true, "result":{"width": 2, "png_base64":"cG5n"}})),
        )
        .unwrap();
        assert_eq!(
            result["content"][0],
            json!({"type":"image","data":"cG5n","mimeType":"image/png"})
        );
        assert_eq!(
            result["structuredContent"],
            json!({"ok": true, "result":{"width": 2}})
        );
        assert_eq!(
            serde_json::from_str::<Value>(result["content"][1]["text"].as_str().unwrap()).unwrap(),
            result["structuredContent"]
        );
        for value in [
            json!({"ok": true, "result":{}}),
            json!({"ok": false, "error":"stale_window"}),
        ] {
            let result = serde_json::to_value(contracts.image_tool_result(value)).unwrap();
            assert_eq!(result["isError"], true);
            assert_eq!(result["content"].as_array().unwrap().len(), 1);
            assert_eq!(result["content"][0]["type"], "text");
        }
    }
}
