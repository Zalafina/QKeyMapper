use std::{ffi::OsString, path};

use qkeymapper_mcp_rust::{
    bridge::{Bridge, Worker},
    native::{NativeError, Target},
};
use rmcp::{
    ErrorData, RoleServer, ServerHandler,
    model::{
        CallToolRequestMethod, CallToolRequestParams, CallToolResponse, CallToolResult,
        ContentBlock, Implementation, ListToolsResult, PaginatedRequestParams, ServerCapabilities,
        ServerConfig, Tool,
    },
    service::RequestContext,
};
use serde_json::{Value, json};

const TOOLS: [(&str, &str); 6] = [
    ("qkm_get_connection_state", "查询桥接连接状态。"),
    (
        "qkm_connect",
        "使用已批准并保存的配对连接指定的 QKeyMapper。",
    ),
    (
        "qkm_request_pairing",
        "发起由 QKeyMapper 管理窗批准的配对。",
    ),
    ("qkm_disconnect", "断开本桥接的连接。"),
    ("qkm_get_capabilities", "查询当前已认证实例的实际能力。"),
    ("qkm_get_state", "读取当前已认证实例的状态。"),
];

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

struct Handler(Bridge);

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
        let schema = json!({"type": "object", "additionalProperties": false});
        let schema = schema.as_object().expect("fixed object schema");
        Ok(ListToolsResult::with_all_items(
            TOOLS
                .iter()
                .map(|&(name, description)| Tool::new(name, description, schema.clone()))
                .collect(),
        ))
    }

    async fn call_tool(
        &self,
        request: CallToolRequestParams,
        context: RequestContext<RoleServer>,
    ) -> Result<CallToolResponse, ErrorData> {
        if request.arguments.is_some_and(|args| !args.is_empty()) {
            return Ok(
                CallToolResult::error(vec![ContentBlock::text("Unexpected arguments")]).into(),
            );
        }
        let value = match request.name.as_ref() {
            "qkm_get_connection_state" => self.0.state(),
            "qkm_connect" => self.0.connect(false),
            "qkm_request_pairing" => self.0.connect(true),
            "qkm_disconnect" => self.0.disconnect(),
            "qkm_get_capabilities" => self.read("qkm_get_capabilities", context).await,
            "qkm_get_state" => self.read("qkm_get_state", context).await,
            _ => return Err(ErrorData::method_not_found::<CallToolRequestMethod>()),
        };
        Ok(tool_result(value).into())
    }
}

impl Handler {
    async fn read(&self, method: &'static str, context: RequestContext<RoleServer>) -> Value {
        if context.ct.is_cancelled() {
            return json!({"ok": false, "error": "cancelled"});
        }
        let mut job = match self.0.read(method) {
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

fn tool_result(mut value: Value) -> CallToolResult {
    if value["ok"].as_bool() != Some(false) && value["accepted"].as_bool() != Some(false) {
        return CallToolResult::structured(value);
    }
    let code = value["error"]
        .as_str()
        .unwrap_or("busy_or_closed")
        .to_owned();
    let (message, step) = match code.as_str() {
        "busy_or_closed" => (
            "桥接正在处理或关闭请求。",
            "查询连接状态，等待当前作业清理；已关闭的桥接需由客户端重新启动。",
        ),
        "connection_closed" => ("连接已关闭。", "重新核验目标并连接，再核对原操作结果。"),
        "not_connected" => ("尚未连接 QKeyMapper。", "核对目标后连接或请求配对。"),
        "unsupported_capability" => (
            "此实例尚未提供该业务能力。",
            "查询实际能力，选择已支持的操作。",
        ),
        "permission_denied" => (
            "该操作尚未获授权。",
            "请用户在 QKeyMapper 管理窗口核对并授予所需权限。",
        ),
        _ => (
            "操作未完成。",
            "核对连接状态与实际能力；保留错误码用于调查。",
        ),
    };
    let reviewed = matches!(
        code.as_str(),
        "unsupported_capability" | "permission_denied"
    )
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

pub fn run(target: Target) -> Result<(), &'static str> {
    let worker = Worker::start(target).map_err(|_| "Could not start the IPC worker")?;
    let client = worker.client.clone();
    let outcome = crate::stdio::run(Handler(worker.client.clone()), 64 * 1024, move || {
        client.stop()
    });
    worker.join().map_err(|_| "IPC worker did not finish")?;
    outcome
}
