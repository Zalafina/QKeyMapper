import argparse
import asyncio
import os
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Any

from mcp.server import MCPServer
from mcp.server.mcpserver.tools import Tool
from mcp.types import ToolAnnotations

from . import __version__
from .bridge import Bridge


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
    def qkm_get_connection_state() -> dict[str, Any]:
        """返回桥接连接的最近核验状态；不代表映射引擎状态，也不会申请权限。"""
        return bridge.state()

    @tool(connection)
    def qkm_connect() -> dict[str, Any]:
        """使用已有本机配对连接。立即返回，随后查询连接状态；不会自动重新配对或提升权限。"""
        return bridge.connect()

    @tool(connection)
    def qkm_request_pairing() -> dict[str, Any]:
        """申请新的配对。须用户在 QKeyMapper 内核对真实 EXE 并批准；本工具不能自行批准。"""
        return bridge.connect(pair=True)

    @tool(connection)
    def qkm_disconnect() -> dict[str, Any]:
        """取消正在等待的连接或配对并断开，保留已保存配对；不会停止或启动映射。"""
        return bridge.disconnect()

    @tool(read)
    async def qkm_get_capabilities() -> dict[str, Any]:
        """通过已认证 IPC 查询此实例实际提供的能力。未实现能力不会报告为可用。"""
        return await bridge.get_capabilities()

    @tool(read)
    async def qkm_get_state() -> dict[str, Any]:
        """读取已认证产品的当前配置、GUI 状态、Worker 状态采样与实际停止快捷键；不改变设置。未观测的停止清理和不可用的修订号会明确报告。"""
        return await bridge.get_state()

    server = MCPServer("QKeyMapper", version=__version__, lifespan=lifespan, log_level="ERROR", tools=tools,
        instructions="连接本机 QKeyMapper。首次配对必须由用户在 QKeyMapper 窗口批准。按能力发现读取实际支持的状态；尚未开放映射或设置变更。连接状态是上次核验结果，业务状态须重新查询。IDLE 或 Worker STOPPED 均不代表停止清理已完成。")
    server.run(transport="stdio")


if __name__ == "__main__":
    main()
