# QKeyMapper MCP 桥接目录包

Windows 10／11 x64，内含专用 CPython 3.11.9 和固定运行依赖。用户无需安装 Python、pip、源码或开发环境；完整解压后保留 runtime 目录结构。Python、依赖和项目许可随包保留，包清单为 bundle-manifest.json。该组件由普通权限客户端启动。

在 MCP 客户端设置绝对路径，例如：

```json
{
  "mcpServers": {
    "qkeymapper": {
      "command": "D:\\QKeyMapper\\mcp_bridge\\runtime\\python.exe",
      "args": ["-I", "-B", "-X", "utf8", "-m", "qkeymapper_mcp",
               "--discovery-file", "<从目标QKeyMapper管理窗口复制的instance.json绝对路径>",
               "--executable", "D:\\QKeyMapper\\QKeyMapper.exe"]
    }
  }
}
```

替换示例路径；不要只复制 python.exe 或依赖本机 PATH。保持 -I 和 -B，._pth 限定包内模块、禁用全局 Python 和工作目录导入；stdio 客户端直接运行上述程序，不通过管理员终端。可用同一程序的 `-I -B -X utf8 -m qkeymapper_mcp --help` 查看参数。运行端不执行 pip 或联网下载依赖。

目标 QKeyMapper 须为匹配版本、由用户明确启用 MCP，并沿产品管理窗口批准客户端配对及所需权限。发现文件和指定 EXE、进程身份与认证仍逐项核验。未启动或未启用时桥接可初始化并枚举工具，但连接明确失败；它不自动启动产品、申请 UAC 或扩大权限。不要复制另一台 PC 的配对凭据。

普通入口提供已实现的连接、查询、预览／提交、保存及受限窗口能力，实际范围以目标能力发现为准。私有后台演示须另有version2可信创建记录和已认证只配置模式专用实例（startup_purpose=configuration_only_demo），旧version1／普通模式拒绝，显式 --demo-record／--enable-demo-mapping 才增加相关工具；main、item_setup、mapping_advanced 返回真实 PNG。新版只配置产品可在MCP启用后自行发布记录，并在已认证capabilities中给出demo_record路径；普通Release预热宿主尚未交付，桥接本身不生成创建记录、不提供自动管理员启动；开发 gsudo 缓存不随包也不是服务依赖。

图片以原生 MCP ImageContent 交给 Agent／gateway，桥接不创建图片文件。QQ 渠道发送由 gateway 另行负责。此包组件的本机隔离验证不能替代干净 Win10／Win11、完整产品／预热及 QQ 联调；产品默认关闭、正常配对和实例选择规则继续适用。
