import argparse
import asyncio
import json
import os
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Annotated, Any, Literal

from mcp.server import MCPServer
from mcp.server.mcpserver.tools import Tool
from mcp.types import CallToolResult, TextContent, ToolAnnotations
from pydantic import Field, StrictInt

from . import __version__
from .bridge import Bridge


ToolResult = Annotated[CallToolResult, dict[str, Any]]

_ERROR_FEEDBACK = {
    "not_connected": ("尚未连接 QKeyMapper。", "查询连接状态，再使用已有配对连接；需要配对时由用户在产品内批准。"),
    "busy_or_closed": ("桥接正在处理或关闭请求。", "查询连接状态，等待当前作业清理；已关闭的桥接需由客户端重新启动。"),
    "context_busy": ("当前状态不能安全完成此操作。", "查询产品状态和受阻原因；只读查询不会自动停止映射。"),
    "stale_catalog": ("配置或 Tab 目录已变化。", "重新查询目录，使用新引用核对原操作目标。"),
    "stale_source": ("源数据或其身份／授权上下文已变化。", "重新查询实际源，核对目标后重新预览，勿直接重发旧写入。"),
    "permission_denied": ("该操作尚未获授权。", "请用户在 QKeyMapper 管理窗口核对并授予所需权限。"),
    "invalid_arguments": ("请求参数不符合接口约束。", "核对工具参数、范围及示例后修正请求。"),
    "unsupported_capability": ("此实例尚未提供该业务能力。", "查询实际能力，选择已支持的操作。"),
    "unsupported_method": ("此实例不支持该方法。", "查询实际能力，使用声明的工具和参数。"),
    "unsupported_query": ("此实例不支持该查询。", "核对可查询对象及参数说明。"),
    "result_too_large": ("查询结果超过接口限制。", "核对能力限制；减少单页请求，若全量快照超限则报告该限制。"),
    "item_too_large": ("单个查询项超过接口限制。", "核对该项及能力限制；减小页数不能缩小单个查询项。"),
    "invalid_source_data": ("当前源数据不能安全读取。", "查询实际配置来源和加载状态，保留错误码用于调查。"),
    "cancelled": ("请求已取消。", "核对连接及原操作结果，再决定是否重新发起请求。"),
    "timeout": ("请求等待超时。", "核对连接及原操作结果，避免盲目重试变更。"),
    "connection_closed": ("连接已关闭。", "重新核验目标并连接，再核对原操作结果。"),
}
_PRODUCT_ERRORS = frozenset({"context_busy", "stale_catalog", "stale_source", "permission_denied",
    "invalid_arguments", "unsupported_capability", "unsupported_method", "unsupported_query",
    "result_too_large", "item_too_large", "invalid_source_data"})


def tool_result(value: dict[str, Any]) -> CallToolResult:
    failed = value.get("ok") is False or value.get("accepted") is False
    if failed:
        code = value.get("error", "busy_or_closed")
        message, next_step = _ERROR_FEEDBACK.get(code,
            ("操作未完成。", "核对连接状态与实际能力；保留错误码用于调查。"))
        # Only reviewed product errors carry endpoint text; native exception text stays private.
        if code in _PRODUCT_ERRORS and value.get("message"):
            message = value["message"]
        value = {**value, "ok": False, "error": code, "message": message, "next_steps": [next_step]}
    return CallToolResult(content=[TextContent(type="text", text=json.dumps(value, ensure_ascii=False))],
                          structured_content=value, is_error=failed)


def main() -> None:
    parser = argparse.ArgumentParser(description="QKeyMapper local MCP bridge (stdio).")
    parser.add_argument("--discovery-file", type=Path, required=True)
    parser.add_argument("--executable", type=Path, required=True)
    args = parser.parse_args()
    if os.name != "nt":
        parser.error("Windows 10 or 11 is required.")
    # Resolve no untrusted discovery contents; these paths are explicit host configuration.
    discovery = Path(os.path.abspath(args.discovery_file))
    executable = Path(os.path.abspath(args.executable))
    if discovery.name != "instance.json":
        parser.error("--discovery-file must name instance.json from QKeyMapper's MCP window.")
    bridge = Bridge(discovery, executable)

    @asynccontextmanager
    async def lifespan(_server):
        try:
            yield bridge
        finally:
            await asyncio.to_thread(bridge.close)

    read = ToolAnnotations(readOnlyHint=True, destructiveHint=False, openWorldHint=False)
    connection = ToolAnnotations(readOnlyHint=False, destructiveHint=False, openWorldHint=False)
    tools = []

    def tool(annotations):
        def register(fn):
            definition = Tool.from_function(fn, annotations=annotations, structured_output=True)
            arguments = definition.fn_metadata.arg_model
            # The SDK otherwise drops unknown fields before calling the tool.
            arguments.model_config["extra"] = "forbid"
            arguments.model_rebuild(force=True)
            definition.parameters = arguments.model_json_schema(by_alias=True)
            tools.append(definition)
            return fn
        return register

    @tool(read)
    def qkm_get_connection_state() -> ToolResult:
        """返回桥接连接的最近核验状态；不代表映射引擎状态，也不会申请权限。"""
        return tool_result(bridge.state())

    @tool(connection)
    def qkm_connect() -> ToolResult:
        """使用已有本机配对连接。立即返回，随后查询连接状态；不会自动重新配对或提升权限。"""
        return tool_result(bridge.connect())

    @tool(connection)
    def qkm_request_pairing() -> ToolResult:
        """申请新的配对。须用户在 QKeyMapper 内核对真实 EXE 并批准；本工具不能自行批准。"""
        return tool_result(bridge.connect(pair=True))

    @tool(connection)
    def qkm_disconnect() -> ToolResult:
        """取消正在等待的连接或配对并断开，保留已保存配对；不会停止或启动映射。"""
        return tool_result(bridge.disconnect())

    @tool(read)
    async def qkm_get_capabilities() -> ToolResult:
        """通过已认证 IPC 查询此实例实际提供的能力。未实现能力不会报告为可用。"""
        return tool_result(await bridge.get_capabilities())

    @tool(read)
    async def qkm_get_state() -> ToolResult:
        """读取已认证产品的配置选择、最近映射加载的实际结果／代次、GUI 状态、Worker 状态采样与实际停止快捷键。加载记录不证明磁盘已保存或后续数据未变；未观测的停止清理和不可用的业务修订号会明确报告。"""
        return tool_result(await bridge.get_state())

    @tool(read)
    async def qkm_query(entity: Literal["configurations", "tabs", "mapping_sources", "mappings"],
            catalog_revision: Annotated[str | None, Field(min_length=64, max_length=64, pattern="^[0-9a-f]{64}$")] = None,
            source_revision: Annotated[str | None, Field(min_length=64, max_length=64, pattern="^[0-9a-f]{64}$")] = None,
            source_tab_ref: Annotated[str | None, Field(max_length=128, pattern="^[0-9a-f]{64}:source:(0|[1-9][0-9]{0,9})$")] = None,
            offset: Annotated[StrictInt, Field(ge=0, le=2147483647)] = 0,
            limit: Annotated[StrictInt, Field(ge=1, le=100)] = 50) -> ToolResult:
        """只读分页查询。configurations/tabs 使用 catalog_revision，catalog_ref 不能作为编辑目标。mapping_sources 返回当前实际普通/共通源表的不透明引用；mappings 必须同时传该 source_revision 和 source_tab_ref，返回源行及完整字段。源查询当前仅允许本进程尚无映射启停、重启或浮动/虚拟按钮发送活动的初始空闲状态；运行后清理尚未核实则拒绝，不主动停止。来源只证明当前内存源，不证明磁盘保存或共通备份加载自哪组。源修订不是完整业务修订，引用不授予写入权限；变化后重新查询。未加载配置内容尚未接入。"""
        return tool_result(await bridge.query(entity, catalog_revision=catalog_revision, source_revision=source_revision,
                                             source_tab_ref=source_tab_ref, offset=offset, limit=limit))

    server = MCPServer("QKeyMapper", version=__version__, lifespan=lifespan, log_level="ERROR", tools=tools,
        instructions="连接本机 QKeyMapper。首次配对必须由用户在 QKeyMapper 窗口批准。按能力发现读取实际支持的状态；尚未开放映射或设置变更。连接状态是上次核验结果，业务状态须重新查询。IDLE 或 Worker STOPPED 均不代表停止清理已完成。")
    server.run(transport="stdio")


if __name__ == "__main__":
    main()
