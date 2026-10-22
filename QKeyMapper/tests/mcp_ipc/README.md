# MCP 本机 IPC 基础测试

当前覆盖 `automation/qkm_ipc_frame.*` 和 `automation/qkm_local_identity.*`。这两组基础代码由本测试目标编译，尚未加入产品工程或建立产品监听；MCP 配对、授权、桥接与业务操作仍按主计划接入。

## 覆盖与边界

- 帧格式：4 字节大端无符号长度＋无 BOM 的 UTF-8 JSON 对象。上限按载荷字节数计算：握手 4 KiB，认证后 256 KiB；容器深度上限 32。版本和命令 schema 由后续握手／分发层处理。
- 校验半包、逐字节、连续帧、精确边界、超限、非法 UTF-8／JSON／Unicode、过深嵌套、读失败、截断、失败后关闭及输出不残留上次消息。
- 从 Windows 进程和 Token 读取 PID、进程创建时间、用户 SID、logon SID、登录 AuthenticationId 及 SessionId；按真实数据比较进程／登录会话。身份读取失败返回空身份，不能使用请求自报字段作为后备值。
- 独立子进程通过随机本机命名管道连接；管道设置 logon SID DACL、远程拒绝及首实例限制。两端分别查询真实对端，测试报文故意自报另一个 PID，断言身份仍来自 Windows。
- 不创建 QKeyMapper 窗口，不加载个人 INI，不使用 Hook、驱动、真实键鼠或 Default 焦点；测试和子进程均为 asInvoker。没有需要占用用户桌面的场景。

异用户／异会话的实际拒绝、远程访问、配对密钥与撤销、慢连接、正式 listener 生命周期、权限和 GUI／MCP 等价性不由本测试证明。合成身份不匹配用例只验证比较逻辑，不能记为真实 Windows 跨用户隔离验收。

## 构建和运行

从仓库根目录、已加载 x64 MSVC 环境的普通终端执行。选择本机 Qt kit 和独立构建目录；Qt5 使用 5.15.2 kit 并换成 `out/tests/mcp_ipc/qt5-release`：

```powershell
$qkmRepo = (Get-Location).Path
$qkmQtRoot = 'C:\Qt\Qt6\6.8.3\msvc2022_64'
$qkmMake = 'C:\Qt\Qt6\Tools\QtCreator\bin\jom\jom.exe'
$qkmBuild = Join-Path $qkmRepo 'out/tests/mcp_ipc/qt6-release'
New-Item -ItemType Directory -Force -Path $qkmBuild | Out-Null
Push-Location $qkmBuild
$qkmOriginalPath = $env:PATH
try {
    & (Join-Path $qkmQtRoot 'bin/qmake.exe') (Join-Path $qkmRepo 'QKeyMapper/tests/mcp_ipc/mcp_ipc_test.pro')
    if ($LASTEXITCODE -ne 0) { throw 'qmake failed' }
    & $qkmMake -j4
    if ($LASTEXITCODE -ne 0) { throw 'build failed' }
    $env:PATH = (Join-Path $qkmQtRoot 'bin') + ';' + $env:PATH
    $qkmReport = Join-Path $qkmBuild 'results.txt'
    & './release/QKeyMapperMcpIpcTest.exe' -o ($qkmReport + ',txt')
    if ($LASTEXITCODE -ne 0) { throw 'tests failed' }
    Get-Content -LiteralPath $qkmReport
}
finally { $env:PATH = $qkmOriginalPath; Pop-Location }
```

完整运行正常应在数秒内结束；自动执行端另设 30 秒外部看门狗。命名管道用例中的连接／收发／子进程等待各限 3 秒，失败路径清理自有子进程和管道，不操作用户实例。`--pipe-client` 是测试内部子进程入口，不是产品接口。

静态分析工具已登记此独立目标并在影子目录生成所需 `.moc`；`scripts/test_static_analysis.ps1` 维护覆盖及报告门禁。真实分析按 `scripts/run_static_analysis.ps1 -Files` 选择新增文件，沿用现有普通 Release 构建参数入口；报告门禁测试不等于 C++ 分析通过。

实际结果按源码、Qt、机器和构建配置记录在[任务验证记录](../../../handoff/qkeymapper_mcp_control/validation.md)。不同系统的结论分别验收。
