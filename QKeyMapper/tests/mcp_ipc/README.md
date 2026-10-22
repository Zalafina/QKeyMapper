# MCP 本机 IPC 基础测试

当前覆盖 `automation/qkm_ipc_frame.*`、`automation/qkm_local_identity.*` 和 `automation/qkm_local_listener.*`。这些基础代码由本测试目标编译，尚未加入产品工程或建立产品监听；MCP 配对、授权、桥接与业务操作仍按主计划接入。

## 覆盖与边界

- 帧格式：4 字节大端无符号长度＋无 BOM 的 UTF-8 JSON 对象。上限按载荷字节数计算：握手 4 KiB，认证后 256 KiB；容器深度上限 32。版本和命令 schema 由后续握手／分发层处理。
- 校验半包、逐字节、连续帧、精确边界、超限、非法 UTF-8／JSON／Unicode、过深嵌套、读失败、截断、失败后关闭及输出不残留上次消息。
- 从 Windows 进程和 Token 读取 PID、进程创建时间、用户 SID、logon SID、登录 AuthenticationId 及 SessionId；按真实数据比较进程／登录会话。身份读取失败返回空身份，不能使用请求自报字段作为后备值。
- 独立子进程通过随机本机命名管道连接；管道设置 logon SID DACL、远程拒绝及首实例限制。两端分别查询真实对端，测试报文故意自报另一个 PID，断言身份仍来自 Windows。
- listener 构造时关闭，显式启用生成新的实例／管道名；关闭可取消挂起 accept，重新启用不沿用旧实例。保留一个原生管道实例并复制句柄给 Qt，旧 socket 完整析构后才重新接受连接，避免 Qt5 断开信号早于底层清理造成重连破坏；关闭／重启交错时丢弃旧实例的清理回调。
- 初始限制一个连接，握手载荷 4096 字节，Qt 接管前读缓存限制为 4100 字节。接入起的绝对 3000 ms 期限不随碎片、有效首包或额外消息延长；未来 GUI 配对等待另行实现。当前没有输出接口或业务队列，输出字节数为零。
- 严格首包仅允许 `hello` 或 `pair.request`：共同字段为数值 `version: 1`、当前 `instance`、64 个小写十六进制字符的 `client_nonce`（32 字节）；`hello` 另须 32 个小写十六进制字符的 `pair_id`，`pair.request` 另须非空 `client_name`（UTF-8 最多 80 字节，无控制／格式字符）。未知／缺失字段、错误类型、版本／实例不匹配、业务请求、畸形／超大帧及额外数据均关闭。
- `handshakeReceived` **只代表首包语法及进程主 Token 身份初检，不代表配对、密钥证明或业务授权**。当前有效首包后仍处于未认证状态，到期关闭，不发送凭据／`ready`，不接受第二条请求。认证状态机在下一切片接入。
- 身份组件另有独立的异步 `PipeRequestVerifier`：先读取管道消息，再在一个专用原生线程中核对实际请求 Token 的用户／登录／会话与真实对端进程；检查前后核对 PID／创建时间。读取失败也恢复身份；只有线程退出后才向所属 Qt 线程报告结果，GUI 线程不模拟客户端。组件可复用、每对象只允许一个在途检查，析构不阻塞 Qt、线程独立保留数据／复制句柄直至退出。**该组件尚未接入 listener；集成时仍须限制总在途数、清理与过期回调，不能据此宣称请求安全上下文已被 listener 强制检查。**
- 请求上下文测试覆盖普通／匿名 SQOS 客户端、过期进程／会话预期、无效启动、重复／复用、调用线程 Token 不变及回调线程；32 次启动后立即销毁检查对象，再断开客户端，验证无晚到回调且管道引用全部释放。没有注入 `RevertToSelf` 自身失败或系统资源分配失败；未覆盖的恢复异常不能记为实际通过。
- 覆盖实际跨进程正常／异常退出、失败后重连、半包／静默／持续慢发期限、首包后业务请求、同名首实例抢占拒绝，以及实际管道 DACL、非继承 ACL 与 socket 读缓存／无输出检查。
- 不创建 QKeyMapper 窗口，不加载个人 INI，不使用 Hook、驱动、真实键鼠或 Default 焦点；测试和子进程均为 asInvoker。没有需要占用用户桌面的场景。

异用户／异会话的实际拒绝、远程访问、有效请求上下文检查与 listener 的完整集成、恢复 API 失败、管理员产品／普通桥接的连接、配对密钥与撤销、认证后资源限制、权限和 GUI／MCP 等价性不由本测试证明。合成身份不匹配用例只验证比较逻辑，不能记为真实 Windows 跨用户隔离验收。

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

完整运行正常约 20 秒（包含四个各 3 秒的绝对期限用例）；自动执行端另设 40 秒外部看门狗。测试中的连接／普通收发／子进程等待各限 3 秒，listener 子进程断线看门狗为 6 秒；失败路径清理自有子进程和管道，不操作用户实例。`--pipe-client` 和 `--listener-client` 是测试内部子进程入口，不是产品接口。

静态分析工具已登记此独立目标并在影子目录生成所需 `.moc`；`scripts/test_static_analysis.ps1` 维护覆盖及报告门禁。真实分析按 `scripts/run_static_analysis.ps1 -Files` 选择新增文件，沿用现有普通 Release 构建参数入口；报告门禁测试不等于 C++ 分析通过。

实际结果按源码、Qt、机器和构建配置记录在[任务验证记录](../../../handoff/qkeymapper_mcp_control/validation.md)。不同系统的结论分别验收。
