# QKeyMapper MCP Rust Sidecar Windows x64 候选组件

将整个目录解压到本机。若清单的 `app_local` 列出 `VCRUNTIME140.dll`，请将它与 Sidecar EXE 放在同一目录。单 EXE 候选的 `app_local` 为空。本组件由普通权限客户端通过 stdio 启动。运行端需要 64 位 Windows 10／11 和包含 MCP 接口的 QKeyMapper，Sidecar 自身无需 Python、Rust 或开发工具。Windows 7 x86 不支持 QKeyMapper MCP 功能，Qt5 x86 Release 不包含 Sidecar。

在目标 QKeyMapper 的 MCP 控制窗启用接口并复制连接信息文件路径。客户端配置使用本目录的绝对 EXE 路径，将 `--executable` 指向同一个实际目标，并将 `--discovery-file` 指向它的 `instance.json`。

## Codex 项目配置

```toml
[mcp_servers.qkeymapper]
command = "D:/Tools/QKeyMapper MCP Rust/qkeymapper-mcp-rust.exe"
args = ["--executable", "D:/Tools/QKeyMapper/QKeyMapper.exe", "--discovery-file", "C:/Path/from/management/instance.json"]
```

## Antigravity CLI 工作区配置 `.agents/mcp_config.json`

```json
{
  "mcpServers": {
    "qkeymapper": {
      "command": "D:/Tools/QKeyMapper MCP Rust/qkeymapper-mcp-rust.exe",
      "args": ["--executable", "D:/Tools/QKeyMapper/QKeyMapper.exe", "--discovery-file", "C:/Path/from/management/instance.json"]
    }
  }
}
```

## 连接及配对

用 `qkm_connect` 复用有效配对并查询 `qkm_get_connection_state`。需要新配对时调用 `qkm_request_pairing`，在目标 QKeyMapper 内确认实际客户端及权限。连接忙碌时由占用方主动断开。项目文件只包含连接参数，客户端配对密钥由 Windows 凭据管理器保存。

`--persistent-target` 是 `--discovery-file` 的显式替代选项，沿用产品现有的持久配对及当前登录发现范围。专用只配置演示部署可选 `--product-demo-record` 或 `--demo-record PATH`；只有宿主明确允许写入演示时才附加 `--enable-demo-mapping`。演示图片由支持 MCP 图片的客户端消费，渠道发送由对应 gateway 处理。

此包用于 Rust 迁移阶段验收。可通过显式切回原 Python 组件的启动配置使用既有正式实现。完整宿主、干净系统和正式发布状态以迁移计划为准。

`bundle-manifest.json` 包含依赖清单及组件文件摘要。第三方许可原件位于 `third-party`，主项目许可见 `QKeyMapper-LICENSE.txt`。Microsoft Visual C++ 运行库适用对应的 Visual Studio 许可和再分发条款。
