import argparse
import asyncio
import json
import os
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Annotated, Any, Literal

from mcp.server import MCPServer
from mcp.server.mcpserver.tools import Tool
from mcp.types import CallToolResult, ImageContent, TextContent, ToolAnnotations
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
    "stale_window": ("窗口引用已失效。", "重新查询本实例窗口，核对角色及当前源后再请求。"),
    "window_unavailable": ("目标窗口未显示或仍最小化。", "通过获准的显示流程恢复目标，再重新查询窗口；截图不会自动激活。"),
    "capture_timeout": ("截图未在本次时限内完成。", "核对实例及窗口状态后重新查询；不会返回上一张图片。"),
    "image_too_large": ("窗口图片超过像素或PNG大小上限。", "缩小已核实的目标窗口或选择受支持页面，重新查询；不会自动缩放或截断。"),
    "capture_failed": ("本次窗口图像未能取得。", "核对已显示的受支持窗口，重新查询窗口和源。"),
    "invalid_image": ("图像结果未通过身份、格式或大小校验。", "未返回图片；核对目标及错误码，保留本次记录后调查。"),
    "stale_preview": ("预览已失效、被消费或结果不可用。", "先用 operation_id 查询结果；未知时重新核对实际源，勿盲目重复写入。"),
    "preview_expired": ("预览已过期，未执行本次修改。", "重新查询源并预览，核对后提交新的预览。"),
    "preview_superseded": ("较新的预览已替换原预览。", "核对当前预览内容及其 ID，勿提交旧预览。"),
    "preview_limit": ("待处理预览达到有界上限。", "取消自己的待处理预览或等待过期后重新预览。"),
    "operation_limit": ("进行中操作已占满结果存储。", "先查询已接受操作；等待完成后核对原预览是否仍有效，再由明确意图提交。"),
    "confirmation_required": ("尚未确认预览要求的完整范围或连带变化。", "向用户说明预览内容，取得同意后重新预览；按操作分别确认映射冲突，或完整保存范围及独立覆盖意图。"),
    "save_write_failed": ("配置写盘失败，内存准备可能已变化。", "先以同一操作ID查询分段结果及当前状态；不要盲目重发，核对后再决定是否重新预览保存。"),
    "save_reload_failed": ("配置已经写盘，但重新加载失败。", "查询同一操作结果和当前配置，按实际磁盘／加载结果恢复；不能视为完全回滚或盲目重发。"),
    "permission_denied": ("该操作尚未获授权。", "请用户在 QKeyMapper 管理窗口核对并授予所需权限。"),
    "invalid_arguments": ("请求参数不符合接口约束。", "核对工具参数、范围及示例后修正请求。"),
    "unsupported_capability": ("此实例尚未提供该业务能力。", "查询实际能力，选择已支持的操作。"),
    "unsupported_method": ("此实例不支持该方法。", "查询实际能力，使用声明的工具和参数。"),
    "unsupported_query": ("此实例不支持该查询。", "核对可查询对象及参数说明。"),
    "result_too_large": ("查询结果超过接口限制。", "核对能力限制；减少单页请求，若全量快照超限则报告该限制。"),
    "item_too_large": ("单个查询项超过接口限制。", "核对该项及能力限制；减小页数不能缩小单个查询项。"),
    "invalid_source_data": ("当前源数据不能安全读取。", "查询实际配置来源和加载状态，保留错误码用于调查。"),
    "cancelled": ("本端等待已取消，已接受的变更仍可能完成。", "重新核对连接，并查询原操作ID的结果；取消等待不代表回滚或停止，不盲目重发变更。"),
    "timeout": ("请求等待超时。", "核对连接及原操作结果，避免盲目重试变更。"),
    "connection_closed": ("连接已关闭。", "重新核验目标并连接，再核对原操作结果。"),
}
_PRODUCT_ERRORS = frozenset({"context_busy", "stale_catalog", "stale_source", "permission_denied",
    "invalid_arguments", "unsupported_capability", "unsupported_method", "unsupported_query",
    "result_too_large", "item_too_large", "invalid_source_data", "stale_preview", "preview_expired",
    "preview_superseded", "preview_limit", "operation_limit", "confirmation_required", "save_write_failed", "save_reload_failed",
    "stale_window", "window_unavailable", "capture_timeout", "image_too_large", "capture_failed"})


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


def image_tool_result(value: dict[str, Any]) -> CallToolResult:
    if value.get("ok") is not True:
        return tool_result(value)
    payload = value.get("result")
    if type(payload) is not dict or type(payload.get("png_base64")) is not str:
        return tool_result({"ok": False, "error": "invalid_image"})
    metadata = dict(payload)
    encoded = metadata.pop("png_base64")
    result = tool_result({**value, "result": metadata})
    result.content.insert(0, ImageContent(type="image", data=encoded, mime_type="image/png"))
    return result


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
        """断开桥接并取消本端等待，保留已保存配对。已接受的业务仍可能完成，断开不代表撤销、回滚或停止；重连后先按原操作ID查询结果，不盲目重发变更。不会停止或启动映射。"""
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
    async def qkm_query(entity: Literal["configurations", "tabs", "mapping_sources", "mappings", "windows"],
            catalog_revision: Annotated[str | None, Field(min_length=64, max_length=64, pattern="^[0-9a-f]{64}$")] = None,
            source_revision: Annotated[str | None, Field(min_length=64, max_length=64, pattern="^[0-9a-f]{64}$")] = None,
            source_tab_ref: Annotated[str | None, Field(max_length=128, pattern="^[0-9a-f]{64}:source:(0|[1-9][0-9]{0,9})$")] = None,
            offset: Annotated[StrictInt, Field(ge=0, le=2147483647)] = 0,
            limit: Annotated[StrictInt, Field(ge=1, le=100)] = 50) -> ToolResult:
        """只读分页查询。configurations/tabs 使用 catalog_revision，catalog_ref 不能作为编辑目标。mapping_sources 返回当前实际普通/共通源表的不透明引用；mappings 必须同时传该 source_revision 和 source_tab_ref，返回源行及完整字段。源查询要求加载成功、GUI 空闲且 Worker 停止，并且是初始静止或已有仍有效的完整清理读取许可；新运行活动使许可失效，运行中和停止清理中拒绝，不主动停止。completed_cleanup 及清理代次仅证明只读上下文，不授予编辑权限或替代异步操作结果。来源只证明当前内存源，不证明磁盘保存或共通备份加载自哪组。源修订不是完整业务修订，引用不授予写入权限；变化后重新查询。windows 仅需 Read，观察本实例 main/item_setup 的身份、可见／最小化／前台采样和不透明窗口生命周期引用；不创建、显示或截取窗口，不证明映射源／场景未变。该实体仅接收 entity/offset/limit，目录／源参数须省略。未加载配置内容尚未接入。"""
        # Window observation shares pagination but accepts no catalog/source parameters.
        return tool_result(await bridge.query(entity, catalog_revision=catalog_revision, source_revision=source_revision,
                                             source_tab_ref=source_tab_ref, offset=offset, limit=limit))

    @tool(read)
    async def qkm_preview_changes(
            source_revision: Annotated[str | None, Field(min_length=64, max_length=64, pattern="^[0-9a-f]{64}$")] = None,
            source_tab_ref: Annotated[str | None, Field(max_length=128, pattern="^[0-9a-f]{64}:source:(0|[1-9][0-9]{0,9})$")] = None,
            original_key: Annotated[str | None, Field(min_length=1, max_length=1, pattern="^[A-Z0-9]$")] = None,
            mapping_key: Annotated[str | None, Field(min_length=1, max_length=1, pattern="^[A-Z0-9]$")] = None,
            insert_row: Annotated[StrictInt | None, Field(ge=0, le=2147483647)] = None,
            action: Literal["mapping.add", "configuration.save"] = "mapping.add",
            setting_name: Annotated[str | None, Field(min_length=1, max_length=32767)] = None) -> ToolResult:
        """只读预览，先查询实际能力。默认mapping.add保持基本映射调用：四个源／键字段必填，每键限单个大写字母或数字；先qkm_query(mapping_sources)获取真实源引用，insert_row从0开始、缺省追加。configuration.save仅传action及明确setting_name，要求Read＋Configure、加载成功／空闲且清理许可有效、无打开编辑窗；返回完整加载／共享草稿的保存足迹、名称效果及明确目标，不能理解为只保存一行。保存另须qkm_apply_changes明确确认完整范围及独立覆盖；不自动开始映射。两种预览最多120秒，新预览替换本客户端旧预览，不改配置、dirty或磁盘，不自动停止、切表或关闭编辑窗。"""
        return tool_result(await bridge.preview_changes(source_revision, source_tab_ref, original_key, mapping_key, insert_row,
            action=action, setting_name=setting_name))

    preview_definition = next(definition for definition in tools if definition.name == "qkm_preview_changes")
    preview_definition.parameters["oneOf"] = [
        {"required": ["source_revision", "source_tab_ref", "original_key", "mapping_key"],
         "properties": {"action": {"const": "mapping.add"}, "setting_name": {"type": "null"},
            **{name: {"type": "string"} for name in ("source_revision", "source_tab_ref", "original_key", "mapping_key")}}},
        {"required": ["action", "setting_name"], "properties": {
            "action": {"const": "configuration.save"}, "setting_name": {"type": "string"}, **{name: {"type": "null"} for name in
                ("source_revision", "source_tab_ref", "original_key", "mapping_key", "insert_row")}}}]

    @tool(write)
    async def qkm_apply_changes(
            preview_id: Annotated[str, Field(min_length=32, max_length=32, pattern="^[0-9a-f]{32}$")],
            confirm_conflicts: StrictBool = False, confirm_scope: StrictBool = False,
            confirm_overwrite: StrictBool = False) -> ToolResult:
        """提交本实例／配对内已审阅的预览，重新校验权限、完整内容、状态及原到期。mapping.add仅改内存；用户同意禁用冲突行后才传confirm_conflicts=true。configuration.save会写入全部预览范围并重新加载，用户同意全部加载／共享草稿和名称效果后才传confirm_scope=true；覆盖另须confirm_overwrite=true，不能用映射冲突确认替代。准入拒绝不写盘；执行中写失败可能有内存准备效果，重载失败可能已写盘，须查询同ID的分段结果。备份失败与主文件保存分别报告。不自动停止或启动映射。预览单次消费，重复成功ID只返回历史结果；失败需新预览，超时／断线先查qkm_get_operation，unknown不证明未发生，不盲目重试。"""
        return tool_result(await bridge.apply_changes(preview_id, confirm_conflicts,
            confirm_scope=confirm_scope, confirm_overwrite=confirm_overwrite))

    @tool(connection)
    async def qkm_cancel_changes(
            preview_id: Annotated[str, Field(min_length=32, max_length=32, pattern="^[0-9a-f]{32}$")]) -> ToolResult:
        """取消本配对尚未提交的预览，不撤销已经应用的修改、不停止映射。允许查询／取消期间状态变化；已消费或未知时先查询操作结果。"""
        return tool_result(await bridge.cancel_changes(preview_id))

    @tool(read)
    async def qkm_capture_window(
            role: Literal["main", "item_setup"],
            window_ref: Annotated[str, Field(min_length=64, max_length=64, pattern="^[0-9a-f]{64}$")],
            source_revision: Annotated[str, Field(min_length=64, max_length=64, pattern="^[0-9a-f]{64}$")],
            source_tab_ref: Annotated[str, Field(max_length=128, pattern="^[0-9a-f]{64}:source:(0|[1-9][0-9]{0,9})$")]) -> ToolResult:
        """截取本实例当前已显示、非最小化的受支持Qt窗口，必须另获CaptureWindows；Read、Configure或ShowWindows不能代替。先查询windows取得本角色window_ref，并查询mapping_sources取得当前source_revision及所选source_tab_ref；窗口须正展示该源，不自动切换Tab。绘制等待前后复核请求、窗口、完整源及安全停止上下文；单项窗存在未应用键文本、弹窗、陈旧或超时明确失败。不会显示、恢复、激活、开始映射或注入输入。返回160KiB内PNG原生图片和不含Base64的实例／源／窗口／尺寸元数据；仅Qt客户区，独立弹窗／原生边框不包含，实时UI可能显示其他未提交控件。没有文件路径参数，不落盘；图片不代表已保存，也不代表QQ已发送。"""
        return image_tool_result(await bridge.capture_window(role, window_ref, source_revision, source_tab_ref))

    @tool(read)
    async def qkm_get_operation(
            operation_id: Annotated[str, Field(min_length=32, max_length=32, pattern="^[0-9a-f]{32}$")]) -> ToolResult:
        """查询本实例／本配对及权限代次内的预览或提交结果，失败状态查询本身仍可成功。保存预览另带所属实例／配置组／保存修订。结果带采样时间，完成记录为历史事实，不代表当前运行／保存状态；最多64条，进行中记录保留，完成后保留10分钟并受容量限制，接口重置或进程退出后丢失。unknown时核对当前配置及源，不能假定未修改或盲目重发。"""
        return tool_result(await bridge.get_operation(operation_id))

    server = MCPServer("QKeyMapper", version=__version__, lifespan=lifespan, log_level="ERROR", tools=tools,
        instructions="连接本机 QKeyMapper。首次配对必须由用户在产品窗口批准。先查询实际能力和明确目标；修改和完整保存分别预览并按授权确认，映射添加不自动保存，保存不自动停止或开始。完整保存包含全部加载及共享草稿，不能当作单行保存。IDLE或Worker STOPPED不代表清理完成。写入失败、超时或断线先查询同一操作的分段结果。")
    server.run(transport="stdio")


if __name__ == "__main__":
    main()
