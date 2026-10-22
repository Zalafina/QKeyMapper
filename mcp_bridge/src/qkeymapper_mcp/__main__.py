import argparse
import asyncio
import os
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Any

from mcp.server import MCPServer
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

    server = MCPServer("QKeyMapper", version=__version__, lifespan=lifespan, log_level="ERROR",
        instructions="连接本机 QKeyMapper。首次配对必须由用户在 QKeyMapper 窗口批准。当前仅提供连接和能力发现，尚未开放映射或设置操作。连接状态是上次核验结果，查询能力会重新核验连接。")
    read = ToolAnnotations(readOnlyHint=True, destructiveHint=False, openWorldHint=False)
    connection = ToolAnnotations(readOnlyHint=False, destructiveHint=False, openWorldHint=False)

    @server.tool(annotations=read, structured_output=True)
    def qkm_get_connection_state() -> dict[str, Any]:
        """返回桥接连接的最近核验状态；不代表映射引擎状态，也不会申请权限。"""
        return bridge.state()

    @server.tool(annotations=connection, structured_output=True)
    def qkm_connect() -> dict[str, Any]:
        """使用已有本机配对连接。立即返回，随后查询连接状态；不会自动重新配对或提升权限。"""
        return bridge.connect()

    @server.tool(annotations=connection, structured_output=True)
    def qkm_request_pairing() -> dict[str, Any]:
        """申请新的配对。须用户在 QKeyMapper 内核对真实 EXE 并批准；本工具不能自行批准。"""
        return bridge.connect(pair=True)

    @server.tool(annotations=connection, structured_output=True)
    def qkm_disconnect() -> dict[str, Any]:
        """取消正在等待的连接或配对并断开，保留已保存配对；不会停止或启动映射。"""
        return bridge.disconnect()

    @server.tool(annotations=read, structured_output=True)
    async def qkm_get_capabilities() -> dict[str, Any]:
        """通过已认证 IPC 查询此实例实际提供的能力。未实现能力不会报告为可用。"""
        return await bridge.get_capabilities()

    server.run(transport="stdio")


if __name__ == "__main__":
    main()
