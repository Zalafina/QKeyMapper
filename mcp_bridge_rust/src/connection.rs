use std::{
    collections::{BTreeMap, BTreeSet},
    ffi::OsString,
    path,
};

use qkeymapper_mcp_rust::{
    bridge::{Bridge, Worker},
    native::{NativeError, Target},
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

#[derive(Deserialize)]
struct Contracts {
    tools: Vec<Tool>,
    feedback: BTreeMap<String, (String, String)>,
    reviewed_product_errors: BTreeSet<String>,
}

pub fn parse_target(args: &mut impl Iterator<Item = OsString>) -> Option<Target> {
    let (mut executable, mut discovery, mut persistent) = (None, None, false);
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
            _ => return None,
        }
    }
    if discovery.is_none() && !persistent {
        return None;
    }
    Some(Target {
        executable: executable?,
        discovery,
    })
}

struct Handler {
    bridge: Bridge,
    contracts: Contracts,
}

impl ServerHandler for Handler {
    fn get_info(&self) -> ServerConfig {
        ServerConfig::new(ServerCapabilities::builder().enable_tools().build())
            .with_server_info(Implementation::new(
                "QKeyMapper Rust bridge",
                env!("CARGO_PKG_VERSION"),
            ))
            .with_instructions(
                "QKeyMapper Rust IPC candidate. Connection, pairing, and read tools.",
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
            crate::read_args::parse(&request.name, request.arguments.unwrap_or_default())
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
            "qkm_get_capabilities" => self.read("qkm_get_capabilities", arguments, context).await,
            "qkm_get_state" => self.read("qkm_get_state", arguments, context).await,
            "qkm_get_help" => self.read("qkm_get_help", arguments, context).await,
            "qkm_query" => self.read("qkm_query", arguments, context).await,
            "qkm_get_operation" => self.read("qkm_get_operation", arguments, context).await,
            _ => return Ok(CallToolResult::error(vec![ContentBlock::text("未知工具。")]).into()),
        };
        Ok(self.contracts.tool_result(value).into())
    }
}

impl Handler {
    async fn read(
        &self,
        method: &'static str,
        arguments: Object,
        context: RequestContext<RoleServer>,
    ) -> Value {
        if context.ct.is_cancelled() {
            return json!({"ok": false, "error": "cancelled"});
        }
        let mut job = match self.bridge.read(method, arguments) {
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

pub fn run(target: Target) -> Result<(), &'static str> {
    let contracts = serde_json::from_str(include_str!("tool_catalog.json"))
        .map_err(|_| "Invalid embedded MCP tool catalog")?;
    let worker = Worker::start(target).map_err(|_| "Could not start the IPC worker")?;
    let client = worker.client.clone();
    let handler = Handler {
        bridge: worker.client.clone(),
        contracts,
    };
    let outcome = crate::stdio::run(handler, 64 * 1024, move || client.stop());
    worker.join().map_err(|_| "IPC worker did not finish")?;
    outcome
}
