#![forbid(unsafe_code)]

#[cfg(windows)]
mod connection;
mod stdio;
#[cfg(windows)]
mod tool_args;

use std::{
    ffi::OsStr,
    process::ExitCode,
    sync::atomic::{AtomicU32, Ordering},
    time::Duration,
};

use rmcp::{
    ErrorData, RoleServer, ServerHandler,
    model::{
        CallToolRequestParams, CallToolResponse, CallToolResult, ContentBlock, Implementation,
        ListToolsResult, PaginatedRequestParams, ServerCapabilities, ServerConfig, Tool,
    },
    service::RequestContext,
};
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};
use serde_json::{from_value, json};

const FIXTURE_TOOL: &str = "qkm_protocol_fixture";
const MAX_LINE_BYTES: usize = 64 * 1024;
const FIXTURE_PNG: &str = "iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAAEklEQVR4nGP4z8DAAMIM/4EAAB/uBfsL2WiLAAAAAElFTkSuQmCC";

#[derive(Default, Deserialize, JsonSchema, Serialize)]
#[serde(rename_all = "snake_case")]
enum Mode {
    #[default]
    Text,
    Error,
    Image,
    Wait,
}

fn default_text() -> String {
    "QKeyMapper 协议 / 確認".to_owned()
}

#[derive(Deserialize, JsonSchema)]
#[serde(deny_unknown_fields)]
struct FixtureArgs {
    #[serde(default)]
    mode: Mode,
    #[serde(default = "default_text")]
    #[schemars(length(max = 256))]
    text: String,
}

#[derive(Default)]
struct Fixture {
    calls: AtomicU32,
}

impl ServerHandler for Fixture {
    fn get_info(&self) -> ServerConfig {
        ServerConfig::new(ServerCapabilities::builder().enable_tools().build())
            .with_server_info(Implementation::new(
                env!("CARGO_PKG_NAME"),
                env!("CARGO_PKG_VERSION"),
            ))
            .with_instructions("QKeyMapper Rust protocol fixture. No product connection.")
    }

    async fn list_tools(
        &self,
        _: Option<PaginatedRequestParams>,
        _: RequestContext<RoleServer>,
    ) -> Result<ListToolsResult, ErrorData> {
        let schema = schemars::schema_for!(FixtureArgs).to_value();
        Ok(ListToolsResult::with_all_items(vec![Tool::new(
            FIXTURE_TOOL,
            "Return fixed protocol test data.",
            schema.as_object().expect("fixture schema object").clone(),
        )]))
    }

    async fn call_tool(
        &self,
        request: CallToolRequestParams,
        context: RequestContext<RoleServer>,
    ) -> Result<CallToolResponse, ErrorData> {
        if request.name != FIXTURE_TOOL {
            return Err(ErrorData::method_not_found::<
                rmcp::model::CallToolRequestMethod,
            >());
        }
        let parsed = from_value::<FixtureArgs>(json!(request.arguments.unwrap_or_default()));
        let Ok(args) = parsed else {
            return Err(ErrorData::invalid_params("Invalid fixture arguments", None));
        };
        if args.text.chars().count() > 256 {
            return Err(ErrorData::invalid_params(
                "Fixture text exceeds 256 characters",
                None,
            ));
        }

        let call = self.calls.fetch_add(1, Ordering::Relaxed) + 1;
        let result = match args.mode {
            Mode::Text => {
                CallToolResult::structured(json!({"ok": true, "text": args.text, "call": call}))
            }
            Mode::Error => CallToolResult::structured_error(
                json!({"ok": false, "error": "fixture_rejected", "call": call}),
            ),
            Mode::Image => {
                let mut result = CallToolResult::structured(
                    json!({"ok": true, "fixture": true, "width": 2, "height": 2, "call": call}),
                );
                result
                    .content
                    .push(ContentBlock::image(FIXTURE_PNG, "image/png"));
                result
            }
            Mode::Wait => {
                tokio::select! {
                    _ = context.ct.cancelled() => {
                        return Err(ErrorData::internal_error("Fixture request cancelled", None));
                    }
                    _ = tokio::time::sleep(Duration::from_secs(10)) => {}
                }
                CallToolResult::structured(json!({"ok": true, "call": call}))
            }
        };
        Ok(result.into())
    }
}

fn main() -> ExitCode {
    let args: Vec<_> = std::env::args_os().skip(1).collect();
    if args == [OsStr::new("--protocol-fixture")] {
        return finish(stdio::run(Fixture::default(), MAX_LINE_BYTES, || {}));
    }
    #[cfg(windows)]
    if let Some((target, demo)) = connection::parse_target(&mut args.into_iter()) {
        return finish(connection::run(target, demo));
    }
    eprintln!("Usage: qkeymapper-mcp-rust --protocol-fixture");
    eprintln!(
        "   or: qkeymapper-mcp-rust --executable PATH (--discovery-file PATH | --persistent-target)"
    );
    eprintln!("       [--demo-record PATH | --product-demo-record] [--enable-demo-mapping]");
    ExitCode::from(2)
}

fn finish(result: Result<(), &'static str>) -> ExitCode {
    match result {
        Ok(()) => ExitCode::SUCCESS,
        Err(message) => {
            eprintln!("{message}");
            ExitCode::FAILURE
        }
    }
}
