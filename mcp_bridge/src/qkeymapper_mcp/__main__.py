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
from pydantic import Field, StrictBool, StrictInt

from . import __version__
from .bridge import Bridge


ToolResult = Annotated[CallToolResult, dict[str, Any]]

_ERROR_FEEDBACK = {
    "not_connected": ("尚未连接 QKeyMapper。", "查询连接状态，再使用已有配对连接；需要配对时由用户在产品内批准。"),
    "busy_or_closed": ("桥接正在处理或关闭请求。", "查询连接状态，等待当前作业清理；已关闭的桥接需由客户端重新启动。"),
    "context_busy": ("当前状态不能安全完成此操作。", "查询产品状态和受阻原因；操作不会自动停止映射。需要空闲的修改须取得停止同意、等待清理后重新预览。"),
    "stale_catalog": ("配置或 Tab 目录已变化。", "重新查询目录，使用新引用核对原操作目标。"),
    "stale_source": ("源数据或其身份／授权上下文已变化。", "重新查询实际源，核对目标后重新预览，勿直接重发旧写入。"),
    "stale_preview": ("预览已失效、被消费或结果不可用。", "先用 operation_id 查询结果；未知时重新核对实际源，勿盲目重复写入。"),
    "preview_expired": ("预览已过期，未执行本次修改。", "重新查询源并预览，核对后提交新的预览。"),
    "preview_superseded": ("较新的预览已替换原预览。", "核对当前预览内容及其 ID，勿提交旧预览。"),
    "preview_limit": ("待处理预览达到有界上限。", "取消自己的待处理预览或等待过期后重新预览。"),
    "confirmation_required": ("此次新增还会禁用预览中列出的冲突行。", "向用户说明完整连带变化，取得同意后重新预览并显式确认冲突。"),
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
    "result_too_large", "item_too_large", "invalid_source_data", "stale_preview", "preview_expired",
    "preview_superseded", "preview_limit", "confirmation_required"})


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
    write = ToolAnnotations(readOnlyHint=False, destructiveHint=True, idempotentHint=True, openWorldHint=False)
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
    async def qkm_get_help(topic: Literal["basic_mapping", "original_keys", "mapping_keys"],
            offset: Annotated[StrictInt, Field(ge=0, le=2147483647)] = 0,
            limit: Annotated[StrictInt, Field(ge=1, le=100)] = 50) -> ToolResult:
        """读取基础映射语法、毫秒单位／范围／默认值和 A→B 示例，或分页查询产品解析器的真实按键名称。repeat 指映射项的重复设置，宏的 xN 语法另行校验。名称区分大小写；列表不证明设备可用，帮助不代替完整校验或编辑授权，实际可用操作仍以能力查询为准。offset 从 0 开始，limit 为 1–100；使用 next_offset 续页。只读，不停止映射。"""
        return tool_result(await bridge.get_help(topic, offset=offset, limit=limit))

    @tool(read)
    async def qkm_query(entity: Literal["configurations", "tabs", "mapping_sources", "mappings"],
            catalog_revision: Annotated[str | None, Field(min_length=64, max_length=64, pattern="^[0-9a-f]{64}$")] = None,
            source_revision: Annotated[str | None, Field(min_length=64, max_length=64, pattern="^[0-9a-f]{64}$")] = None,
            source_tab_ref: Annotated[str | None, Field(max_length=128, pattern="^[0-9a-f]{64}:source:(0|[1-9][0-9]{0,9})$")] = None,
            offset: Annotated[StrictInt, Field(ge=0, le=2147483647)] = 0,
            limit: Annotated[StrictInt, Field(ge=1, le=100)] = 50) -> ToolResult:
        """只读分页查询。configurations/tabs 使用 catalog_revision，catalog_ref 不能作为编辑目标。mapping_sources 返回当前实际普通/共通源表的不透明引用；mappings 必须同时传该 source_revision 和 source_tab_ref，返回源行及完整字段。源查询要求加载成功、GUI 空闲且 Worker 停止，并且是初始静止或已有仍有效的完整清理读取许可；新运行活动使许可失效，运行中和停止清理中拒绝，不主动停止。completed_cleanup 及清理代次仅证明只读上下文，不授予编辑权限或替代异步操作结果。来源只证明当前内存源，不证明磁盘保存或共通备份加载自哪组。源修订不是完整业务修订，引用不授予写入权限；变化后重新查询。未加载配置内容尚未接入。"""
        return tool_result(await bridge.query(entity, catalog_revision=catalog_revision, source_revision=source_revision,
                                             source_tab_ref=source_tab_ref, offset=offset, limit=limit))

    @tool(read)
    async def qkm_preview_changes(
            source_revision: Annotated[str, Field(min_length=64, max_length=64, pattern="^[0-9a-f]{64}$")],
            source_tab_ref: Annotated[str, Field(max_length=128, pattern="^[0-9a-f]{64}:source:(0|[1-9][0-9]{0,9})$")],
            original_key: Annotated[str, Field(min_length=1, max_length=1, pattern="^[A-Z0-9]$")],
            mapping_key: Annotated[str, Field(min_length=1, max_length=1, pattern="^[A-Z0-9]$")],
            insert_row: Annotated[StrictInt | None, Field(ge=0, le=2147483647)] = None) -> ToolResult:
        """预览一条基本映射，例如 A→B；每个键限单个大写字母或数字。须 Read＋Configure、当前已加载源表、空闲且停止清理完成、无打开的映射编辑窗。先 qkm_query(mapping_sources) 获取完整源修订及目标引用；insert_row 为从 0 开始的源行位置，省略则追加，不受筛选或共通显示行影响。返回全部连带变化、冲突确认要求及 120 秒预览 ID；新预览替换本客户端旧预览。不写配置、dirty 或磁盘，不自动停止、切表、保存或开始。"""
        return tool_result(await bridge.preview_changes(source_revision, source_tab_ref, original_key, mapping_key, insert_row))

    @tool(write)
    async def qkm_apply_changes(
            preview_id: Annotated[str, Field(min_length=32, max_length=32, pattern="^[0-9a-f]{32}$")],
            confirm_conflicts: StrictBool = False) -> ToolResult:
        """提交已审阅的基本映射预览，重新校验身份、权限、完整源、状态及到期；拒绝不产生部分配置修改。只有用户已同意预览列出的禁用冲突行时才传 confirm_conflicts=true。预览单次消费，重复同 ID 返回已知历史结果而不再添加；失败需重新预览。只改内存并保持停止，不保存或启动。超时／断线后先用同 ID 查询 qkm_get_operation；unknown 不代表未发生，不盲目重试。"""
        return tool_result(await bridge.apply_changes(preview_id, confirm_conflicts))

    @tool(connection)
    async def qkm_cancel_changes(
            preview_id: Annotated[str, Field(min_length=32, max_length=32, pattern="^[0-9a-f]{32}$")]) -> ToolResult:
        """取消本配对尚未提交的预览，不撤销已经应用的修改、不停止映射。允许查询／取消期间状态变化；已消费或未知时先查询操作结果。"""
        return tool_result(await bridge.cancel_changes(preview_id))

    @tool(read)
    async def qkm_get_operation(
            operation_id: Annotated[str, Field(min_length=32, max_length=32, pattern="^[0-9a-f]{32}$")]) -> ToolResult:
        """查询本实例／本配对及权限代次内的预览或提交结果，失败状态查询本身仍可成功。结果带采样时间，完成记录为历史事实，不代表当前运行／保存状态。结果最多 64 条、保留 10 分钟；接口重置或进程退出后丢失。unknown 时核对当前配置及源，不能假定未修改或盲目重发。"""
        return tool_result(await bridge.get_operation(operation_id))

    server = MCPServer("QKeyMapper", version=__version__, lifespan=lifespan, log_level="ERROR", tools=tools,
        instructions="连接本机 QKeyMapper。首次配对必须由用户在产品窗口批准。先查询实际能力和明确目标；基本映射须先预览完整变化再提交，按授权分别停止、保存和开始，当前工具不自动执行这些动作。连接状态须重新核验，IDLE 或 Worker STOPPED 不代表清理完成。写入超时或断线后先查询操作结果。")
    server.run(transport="stdio")


if __name__ == "__main__":
    main()
