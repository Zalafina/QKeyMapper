# MCP 本机 IPC 基础测试

当前覆盖 `automation/qkm_ipc_frame.*`、`automation/qkm_local_identity.*`、`automation/qkm_local_listener.*` 和 `automation/qkm_auth_proof.*`。这些基础代码由本测试目标编译，尚未加入产品工程或建立产品监听；MCP 配对、授权、桥接与业务操作仍按主计划接入。

## 覆盖与边界

- 帧格式：4 字节大端无符号长度＋无 BOM 的 UTF-8 JSON 对象。上限按载荷字节数计算：握手 4 KiB，认证后 256 KiB；容器深度上限 32。版本和命令 schema 由后续握手／分发层处理。
- 校验半包、逐字节、连续帧、精确边界、超限、非法 UTF-8／JSON／Unicode、过深嵌套、读失败、截断、失败后关闭及输出不残留上次消息。
- 从 Windows 进程和 Token 读取 PID、进程创建时间、用户 SID、logon SID、登录 AuthenticationId 及 SessionId；按真实数据比较进程／登录会话。身份读取失败返回空身份，不能使用请求自报字段作为后备值。
- 独立子进程通过随机本机命名管道连接；管道设置 logon SID DACL、远程拒绝及首实例限制。两端分别查询真实对端，测试报文故意自报另一个 PID，断言身份仍来自 Windows。
- listener 构造时关闭，显式启用生成新的实例／管道名；关闭可取消挂起 accept，重新启用不沿用旧实例。保留一个原生管道实例并复制句柄给 Qt，旧 socket 完整析构后才重新接受连接，避免 Qt5 断开信号早于底层清理造成重连破坏；关闭／重启交错时丢弃旧实例的清理回调。
- 初始限制一个连接，握手载荷 4096 字节，Qt 接管前读缓存限制为 4100 字节。接入起的绝对 3000 ms 期限不随碎片、有效首包或额外消息延长；未来 GUI 配对等待另行实现。当前没有输出接口或业务队列，输出字节数为零。
- 严格首包仅允许 `hello` 或 `pair.request`：共同字段为数值 `version: 1`、当前 `instance`、64 个小写十六进制字符的 `client_nonce`（32 字节）；`hello` 另须 32 个小写十六进制字符的 `pair_id`，`pair.request` 另须非空 `client_name`（UTF-8 最多 80 字节，无控制／格式字符）。未知／缺失字段、错误类型、版本／实例不匹配、业务请求、畸形／超大帧及额外数据均关闭。
- `handshakeReceived` **只代表首包语法、进程主 Token 与实际管道请求 Token 检查，不代表配对、密钥证明或业务授权**。当前有效首包后仍处于未认证状态，到期关闭，不发送凭据／`ready`，不接受第二条请求。挑战证明和配对状态机在下一切片接入。
- listener 通过唯一的异步 `PipeRequestVerifier` 检查已读取的请求：专用原生线程核对实际请求 Token 的用户／登录／会话与真实对端进程，检查前后核对 PID／创建时间。只有线程退出后才向所属 Qt 线程报告结果，GUI 线程不模拟客户端。断开后既要等 socket 完整析构，也要等检查退出，才重新接受连接；超时但检查未退出时关闭整个接口，不等待或强杀线程，禁止在存续检查期间重新启用。
- 请求上下文测试覆盖普通／匿名 SQOS、过期进程／会话预期、无效启动、重复／复用、调用线程 Token 不变及回调线程；32 次启动后立即销毁检查对象，再断开客户端，验证无晚到回调且管道引用全部释放。专用故障配置还覆盖保持原生线程挂起、关闭／超时／客户端异常退出、恢复失败关闭入口及迟到结果不得放行新实例。
- 覆盖实际跨进程正常／异常退出、失败后重连、半包／静默／持续慢发期限、首包后业务请求、同名首实例抢占拒绝，以及实际管道 DACL、非继承 ACL 与 socket 读缓存／无输出检查。
- 不创建 QKeyMapper 窗口，不加载个人 INI，不使用 Hook、驱动、真实键鼠或 Default 焦点；测试和子进程均为 asInvoker。没有需要占用用户桌面的场景。

异用户／异会话的实际拒绝、远程访问、真实 OS 恢复／分配 API 故障、管理员产品／普通桥接的连接、配对密钥与撤销、认证后资源限制、权限和 GUI／MCP 等价性不由本测试证明。合成身份不匹配用例只验证比较逻辑，不能记为真实 Windows 跨用户隔离验收。

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

## 专用故障配置

默认工程不编译测试钩子，使用实际生产分支。只有显式给 qmake 传入 `CONFIG+=mcp_fault_injection`，才定义 `QKM_AUTOMATION_TESTING`；使用独立目录 `out/tests/mcp_ipc/qt6-faults`，不把这个定义加到产品工程。该配置完整运行约 27 秒，外部看门狗为 50 秒。

测试钩子只在测试 Qt 线程中设置，分别在下一次检查复制一个等待门事件，或跳过恢复调用来模拟失败，并计算原生工作线程数。故障测试故意留下识别级模拟身份至专用线程退出，确认 Qt 线程未改变、失败后不认证／不继续业务。钩子不进入普通构建，不作为 IPC 参数；测试结束释放门事件、恢复钩子并回收自有进程。它证明指定异常路径，不代表真实系统 `RevertToSelf` 已返回失败或已覆盖所有系统故障。

分析工具分别读取普通和故障配置的实际 qmake 参数，回归断言两者源清单、meta-object 输入及测试宏隔离，避免只分析其中一个分支。

## 挑战证明 v1

证明组件复用 Qt 的 HMAC-SHA256；密钥、nonce、MAC 均为 32 字节。随机数直接调用 `BCryptGenRandom` 的系统首选提供者，失败返回空值，不降级到伪随机数。全部 ID 在校验后从 32 个小写十六进制字符解码为 16 字节。

规范化输入固定为 153 字节，所有整数使用无符号大端表示，不经过 JSON 数字或字符串拼接：

| 偏移 | 字节数 | 内容 |
|---:|---:|---|
| 0 | 20 | ASCII `QKeyMapper-MCP-AUTH` 加一个 NUL |
| 20 | 1 | 方向／用途：server=1、client=2、pairing confirmation=3 |
| 21 | 4 | 接口版本 1 |
| 25 | 16 | instance ID |
| 41 | 16 | pair ID |
| 57 | 8 | 非零授权代次 |
| 65 | 4 | 实际 client PID |
| 69 | 8 | 实际 client 创建时间 |
| 77 | 4 | 实际 server PID |
| 81 | 8 | 实际 server 创建时间 |
| 89 | 32 | client nonce |
| 121 | 32 | server nonce |

身份参数必须来自已核对的本机实际进程；辅助函数还检查双方用户／登录／会话相同及身份完整。SID／登录元数据由 OS 门禁校验，未复制进此固定证明格式。未来 JSON 传输使用十六进制 nonce／MAC 和十进制字符串形式的 64 位值，不能使用 JSON 浮点数承载这些整数。

三个用途的完整输入和 MAC 与 Python `struct.pack`、`hmac`、`hashlib.sha256` 独立计算结果比较，固定样例含高于 2^53 的值及最大 uint64。测试逐项改变实例、pair、代次、双方 PID／时间、双 nonce、密钥及方向，验证不再匹配；还拒绝无效／跨登录身份、错误版本／字段长度。固定 32 字节比较逐字节累积差异且不因首个错误早退；语义测试不等于运行时间／侧信道测量。

`ServerChallenge` 不可复制，每次 begin 产生新 server nonce，不保存密钥；只允许一次 client 证明尝试，失败也消耗本次挑战，重放／反射／旧 nonce／取消均拒绝。私有预期证明在完成、重建、取消及析构时清零后释放；Qt HMAC 和调用方可能存在临时内存副本，不能承诺全进程零残留。此处未实现凭据落盘或证明消息收发；密钥不进入此输入格式、日志、命令行或测试证据。

故障配置额外模拟随机源失败，确认不能生成秘密或进入待证明状态。普通配置没有该测试钩子。

实际结果按源码、Qt、机器和构建配置记录在[任务验证记录](../../../handoff/qkeymapper_mcp_control/validation.md)。不同系统的结论分别验收。
