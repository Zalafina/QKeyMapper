# MCP 本机 IPC 基础测试

当前覆盖 `automation/qkm_ipc_frame.*`、`automation/qkm_local_identity.*`、`automation/qkm_local_listener.*`、`automation/qkm_auth_proof.*`、`automation/qkm_pair_store.*` 和 `automation/qkm_session_server.*`。这些基础代码由本维护目标及产品共用。产品已接入默认关闭的管理窗口；正式 MCP 桥接与业务操作仍按主计划接入。管理窗口用例见 [mcp_control](../mcp_control/README.md)。

## 覆盖与边界

- 帧格式：4 字节大端无符号长度＋无 BOM 的 UTF-8 JSON 对象。上限按载荷字节数计算：握手 4 KiB，认证后 256 KiB；容器深度上限 32。版本和命令 schema 由后续握手／分发层处理。
- 校验半包、逐字节、连续帧、精确边界、超限、非法 UTF-8／JSON／Unicode、过深嵌套、读失败、截断、失败后关闭及输出不残留上次消息。
- 从 Windows 进程和 Token 读取 PID、进程创建时间、用户 SID、logon SID、登录 AuthenticationId 及 SessionId；按真实数据比较进程／登录会话。身份读取失败返回空身份，不能使用请求自报字段作为后备值。
- 独立子进程通过随机本机命名管道连接；管道设置 logon SID DACL、远程拒绝及首实例限制。两端分别查询真实对端，测试报文故意自报另一个 PID，断言身份仍来自 Windows。
- listener 构造时关闭，显式启用生成新的实例／管道名；关闭可取消挂起 accept，重新启用不沿用旧实例。保留一个原生管道实例并复制句柄给 Qt，旧 socket 完整析构后才重新接受连接，避免 Qt5 断开信号早于底层清理造成重连破坏；关闭／重启交错时丢弃旧实例的清理回调。
- 初始限制一个连接，握手载荷 4096 字节，Qt 接管前读缓存限制为 4100 字节。接入起的绝对 3000 ms 期限不随碎片、有效首包或额外消息延长；独立 listener 在该阶段不发输出；装配会话控制器后采用下文的配对等待／认证后限制。
- 严格首包仅允许 `hello` 或 `pair.request`：共同字段为数值 `version: 1`、当前 `instance`、64 个小写十六进制字符的 `client_nonce`（32 字节）；`hello` 另须 32 个小写十六进制字符的 `pair_id`，`pair.request` 另须非空 `client_name`（UTF-8 最多 80 字节，无控制／格式字符）。未知／缺失字段、错误类型、版本／实例不匹配、业务请求、畸形／超大帧及额外数据均关闭。
- `handshakeReceived` **只代表首包语法、进程主 Token 与实际管道请求 Token 检查，不代表配对、密钥证明或业务授权**。独立 listener 不发送凭据／`ready` 或放行业务；下文会话控制器在此后执行证明、GUI 批准及逐帧授权。
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

`ServerChallenge` 不可复制，每次 begin 产生新 server nonce，不保存密钥；只允许一次 client 证明尝试，失败也消耗本次挑战，重放／反射／旧 nonce／取消均拒绝。私有预期证明在完成、重建、取消及析构时清零后释放；Qt HMAC 和调用方可能存在临时内存副本，不能承诺全进程零残留。证明组件本身不负责凭据落盘或消息收发，由会话／存储层承担；密钥不进入此输入格式、日志、命令行或测试证据。

故障配置额外模拟随机源失败，确认不能生成秘密或进入待证明状态。普通配置没有该测试钩子。

## 配对凭据文件

`PairCredentialStore` 只接受最多 32 条完整记录。每条包含唯一的 32 个小写十六进制字符的 pair ID、合规 client name、非零 uint64 授权代次、已知权限位和 32 字节 secret。Read 为必需权限；其余为 Configure、RunMapping、Observe、TestInput、ManageDevices、ExecuteCommands，当前只是后续业务权限的类型定义，尚未授权任何产品操作。

记录及权限元数据一起序列化为有界帧，64 位代次使用规范十进制字符串。明文封套为含 NUL 的 `QKM-PAIR-STORE-1`、32 字节 SHA256 与帧；随后以当前用户 DPAPI、`CRYPTPROTECT_UI_FORBIDDEN` 加密，不使用 machine-wide 解密标志。附加上下文为固定用途标签、本机 64 位注册表视图 MachineGuid 的 RFC4122 字节和调用方应用绑定的 SHA256，避免漫游用户凭据使跨机器文件自动成为本机授权。读取缺失机器标识或应用上下文时失败，不降级。

存储目录须为显式绝对路径；父目录由调用方准备。仅首次成功保存才创建最终私有目录，读取不存在的存储不会写文件。目录和文件 owner 为当前用户 SID，DACL 只允许该 SID 的完全访问；拒绝公开／异常 ACL、reparse point、目录冒充文件及多个硬链接。读取持有原生句柄并限制文件 64 KiB／明文载荷 32 KiB，不跟随最终文件链接；不静默修复不安全的既有目录。

写入复用 `QSaveFile`，禁止直接写回后备；临时文件在替换前设置并验证 owner／DACL，适用于默认 owner 可能不同的管理员进程，但当前实测仍为非管理员。已有文件须先在相同上下文成功读取；上下文错误、损坏或不安全文件不会被正常保存覆盖。空记录集也采用原子加密保存，可作为撤销全部配对的持久化结果。保存失败不得被调用方当作授权已落地／已撤销。

测试验证无明文 key／标签／pair ID 落盘、独立子进程读取、最大 uint64、权限和重复 ID 拒绝、错误上下文、损坏／过大文件、只读目标保存失败保留原文件、私有目录及硬链接检查。故障配置还提供仅测试可见的 DPAPI 封套生成入口，构造有效加密但格式、代次、权限、key、校验和错误的文件，验证全部拒绝而不返回部分凭据。它不编译进普通配置。

DPAPI 测试必须以可用的当前 Windows 用户配置运行；普通 Codex 执行沙箱的最小 `CryptProtectData` 在本机返回 `ERROR_FILE_NOT_FOUND`，同一 asInvoker 测试在沙箱外且 `TokenElevation=0` 时通过。因此仅无界面运行测试使用获准的非提权沙箱外执行；编辑、构建、分析继续在原沙箱。没有 UAC／gsudo 或产品启动。不能把沙箱失败写成产品 DPAPI 失败，也不能用弱保护替代。

DPAPI 原生解密缓冲及本组件自有明文 QByteArray 在使用后清零；Qt JSON／HMAC 和调用方仍可能持有临时副本，不保证全内存无残留。同一 Windows 用户及管理员具备的文件／进程访问能力不属于进程间恶意软件隔离保证。真正异用户、跨机器、管理员到普通桥接的实际拒绝及 GUI 配对仍待系统验收。

实际结果按源码、Qt、机器和构建配置记录在[任务验证记录](../../../handoff/qkeymapper_mcp_control/validation.md)。不同系统的结论分别验收。

## 配对及授权会话

`AutomationSessionServer` 默认关闭；真实身份和请求 Token 门禁后，显式 `pair.request` 产生只交给本地 GUI 的请求 ID。只有本地 `approvePairing` 才生成 secret；客户端自报批准值拒绝。批准绑定原连接／实际进程，等待最长 60 秒，期间不得发送其他消息。证明阶段限 3 秒；GUI 提示每秒最多一次，凭据上限 32 条。

`pair.grant` 含新 key、权限、代次、实例、双方 PID／创建时间、nonce 和 server proof；客户端先核对实际服务进程再验证 proof，以用途 3 的 `pair.confirm` 回证。服务器成功落盘才 ready。重连使用 hello／challenge／用途 2 的 proof，key 不再发送；uint64 为十进制字符串。错误／反射／重放证明、已撤销 pair、旧批准、保存失败均不得 ready。

认证后请求仅 `{id, method, arguments}`；ID 为 32 个小写十六进制字符，arguments 为对象。命令白名单与最低权限在 C++ 校验；未接业务回调返回 unsupported_capability；未知命令、任意 Qt 属性写入不分发。权限变化先保存并递增代次，再关原连接；撤销保存失败时关闭接口，不能声称撤销成功。

认证前载荷／读缓存 4 KiB／4100 字节，认证后 256 KiB／262148 字节；输出待写最多两个最大帧。每次一个完整请求，逐帧核对真实进程和请求 Token；认证后空闲不因 3 秒断开，但首字节起 10 秒须完整并完成身份核验。回复绑定连接／请求 ID；处理器可禁用或 deleteLater 会话。业务重试／去重保证尚未实现，须在实际写能力前完成。

独立子进程 key 只通过管道／私有 DPAPI 文件传递，不进入 CLI／日志。覆盖完整配对／重连、权限更新、撤销旧 key、迟到批准、新连接、8 KiB 双向收发、慢帧、保存失败、处理器销毁。同源维护 GUI／正式 SDK 与实际 60 秒等待见 IPC-20；真正产品高低权限及业务效果尚未验收。

目标支持 `CONFIG+=asan CONFIG+=mcp_fault_injection`，须使用带 ASan runtime／import library 的 MSVC kit，在独立目录构建；核对 EXE 导入、实际退出与报告。当前用 BuildTools 14.40，Qt DLL 未重新插桩，不宣称泄漏检测；结果见 IPC-20261022-16／17。

## 私有实例发现

启用会话后在同一私有目录原子发布 `instance.json`：严格七字段 `version`、`instance`、`pipe`、`pid`、`created`、`session`、`executable`。整数身份使用规范十进制字符串，version 为数值 1；文件为无 BOM UTF-8 JSON，最多 128 KiB，以容纳长程序路径，不含 secret／授权数据。目录、文件及临时文件复用凭据存储的 owner／DACL／reparse／硬链接守卫，相关逻辑仅提取到 qkm_private_storage.*，没有降低检查。

发现信息是提示。读取还核对实际进程及创建时间、当前登录会话、系统返回的程序路径，并以 Windows volume serial／file index 与调用方明确指定的预期 EXE 比较。先比较 OS 程序路径再处理提示中的路径，避免打开不可信远程路径。真正连接仍必须核对实际管道对端并完成新挑战；文件内容不能授权操作。

活跃服务器保留发现文件的只读句柄，禁止写／删除共享，并复核当前发布者，避免并发实例覆盖后都开始服务。其他活跃发布者拒绝覆盖；已退出／PID 复用的记录可替换；损坏／不安全文件不静默覆盖。禁用先关闭端点并释放租约，再按实例 ID＋自身 PID／创建时间核对，使用有 DELETE 权限的已验证句柄清理。旧实例清理不能删新文件；只读或失败会报告，不声称已清理。

当前回归包括发布／读取、预期 EXE、字段／UTF-8／大小拒绝、活跃发布者／同进程第二服务器拒绝、退出后的替换、旧 ID 清理、只读失败保留与原存储用例。GUI 连接框现在是可复制的发现文件路径；正式 SDK 集成见 [桥接说明](../../../mcp_bridge/README.md)。高低权限、实际异用户、产品业务及 Win11 仍需后续验收。详情见 IPC-20261022-19／20。

未安装业务处理器时，`qkm_get_capabilities` 只接受空参数，返回 `stage=transport_only`、空 `business_capabilities`、当前权限与帧上限；其余业务仍 `unsupported_capability`。`sessionTransportCapabilities` 独立子进程验证该响应、未知参数拒绝及业务未接拒绝。后续正式业务处理器负责真实完整能力表，不能把内部方法白名单当成已支持业务。
