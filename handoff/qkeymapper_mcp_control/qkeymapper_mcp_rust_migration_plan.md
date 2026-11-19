# QKeyMapper MCP Sidecar：Rust 迁移子计划

整理日期 2026-11-17；2026-11-18 完成当前迁移范围并结项。

本文件保留 **Rust 迁移的实施决策和验收证据**。后续发行与宿主补验统一由 [MCP 主计划](qkeymapper_mcp_automation_plan_codex.md) 跟踪。[任务导航](../README.md) 提供入口。

本页记录结项时的历史策略。2026-11-19 起，[MCP 主计划](qkeymapper_mcp_automation_plan_codex.md) 采用 Rust 单一正式入口，历史对照和原回退设计保留原范围。

## 当前进度与接续卡

| 项目 | 当前记录 |
|---|---|
| 当前切片 | RUST-D4 完成本机选定验收，子计划结项 |
| 分支／源码检查点 | codex/qkeymapper-mcp-control。Rust 运行源码 f4cc8c7d，单 EXE 构建与 Win10 离线选验见 42f5744d，验收范围见 606c2292；正式打包接线及关闭记录见本提交。C++ 产品基线 3fefc26a846b8ad5db1ba381cecff870c570a670 |
| 实现／交付 | 正式 Qt6／Qt5 x64 Release 默认入口为 mcp/qkeymapper_mcp.exe，随附许可、清单及说明。Python 源码、锁和独立组件供开发／显式回退。Qt5 x86 包省略 MCP，Windows 7 x86 不支持 MCP |
| 当前验证 | 双 Qt x64 构建、实际本机解包及真实产品配对复用／权限／PNG／退出／回退通过；Codex 实际从中文及空格路径取得 Qt6 PNG；沿用同一单 EXE 的干净 Win10 x64 离线组件通过证据 |
| 清理 | 两个产品、桥接和固定助手正常退出，专属配对及凭据已清理；16:53:47 关闭本轮缓存 PID 31940，独立查询无控制器／原生会话为零 |
| 后续入口 | Antigravity 最终单 EXE 的中文／空格路径业务调用、真图及模型回文，以及 Win10 Release PC 的真实三包流程，已转入 [MCP 主计划交付项](qkeymapper_mcp_automation_plan_codex.md#rust-d4sidecar-迁移结项)。干净 Win11 离线复验已按用户决定免除 |
| 证据边界 | 本机原件 out/rust_sidecar/{a,b,c,d,d4}_20261118、out/mcp_product/rust_d4_20261118 等。合成、Qt 夹具、真实产品、独立离线组件及正式发行各保留实测范围 |
| 提交策略 | 聚焦本地提交，禁止推送 |

## 1. 结论、价值与固定边界

Rust 适合承接当前独立普通权限 Sidecar，值得先做有界原型。当前桥接有九个 Python 源模块、约1648行，普通模式17个工具，显式配置演示记录后另注册 qkm_get_demo_state，启用演示编排后另注册 qkm_demo_mapping。真正的配置、保存、权限、窗口和映射业务仍由 Qt/C++ 产品执行。

主要收益是减少发布文件、不再随包携带 Python 运行时，以及简化客户端 command／args。现有嵌入式 Python 目录包已不要求用户安装 Python；它没有采用 PyInstaller 单文件自解压。迁移不代表映射延迟、驱动兼容、安全性、冷启动或杀软结果自动改善。

2026-11-17只读复核的本机组件 out/mcp_disabled145_20261110/bridge-b01 为1615文件、63,000,546字节（60.08 MiB），ZIP为26,653,943字节（25.42 MiB）；九个模块与3fefc26a源码逐字节相同。这是组件样本，不是完整最终 Release 的增量证明。Rust 解压5–15 MiB／ZIP 3–10 MiB仅为待验证估算，不是门槛或承诺。

用户提供的 Rust 评估文档和《MCP兼容性指南》是背景材料，接续依据以本文件和实际源码为准。11-18 的 Rust 1.100.0 在 out/rust_sidecar/toolchain 独立安装。原镜像缺版本，改用 Rust 官方静态源。用户更新了本机默认工具链，实测其编译器提交 0ef1bc386059596dada8fb65b50425aab2ea637a 与项目工具链相同。

### 1.1 对外契约与业务职责

- Rust Sidecar 为独立二进制项目，Python 实现供开发与显式回退。正式 MCP 工具名、参数、默认值、可选字段、注解、权限、错误和业务结果保持兼容；内部有界 IPC v1不改。启动入口由包内 Python 改为 Rust EXE时，保留既有目标／演示参数，显式更新客户端配置，不静默改用户 workspace。
- 保留 Schema 和运行时两层校验：严格类型、范围、枚举、未知字段拒绝、空值／省略区别及现有字符串约束。Schema 生成不能代替实际验证；不得借迁移放宽参数或直接写 INI。
- 保留文本、structuredContent及isError语义；图片仍来自产品自身窗口。ImageContent与不重复包含Base64的结构化元数据分别返回，保持身份／窗口／源关联、PNG完整性和现有160 KiB／8M像素等边界，不生成示意图冒充截图。
- C++继续负责共用业务校验、完整保存事务、生命周期和执行前复核。Rust不新增业务引擎、任意命令、Qt属性、任意PID／HWND、文件读写接口、HTTP服务或全局实例管理器。

### 1.2 Windows身份、认证与资源生命周期

- 保留显式 discovery-file／可信EXE及persistent-target入口，逐项核验私有发现ACL／文件边界、真实进程与创建时间、当前用户／登录身份／Windows Session、EXE文件身份、管道对端及HMAC证明。发现记录自报路径和MCP客户端元数据均不构成信任或权限。
- 客户端密钥仍在Windows凭据管理器并受DPAPI保护；服务端保留现有私有DPAPI配对库。workspace仅保存非秘密关联。逐项复现v1／v2派生、路径规范化、熵、UTF-16LE密文编码和写失败语义，不扫描／迁移／覆盖旧或无关凭据，不暴露通用凭据访问工具。
- 当前服务端持久配对记录未存客户端EXE路径。Rust EXE变化不直接要求重新配对；先用专属凭据验证原合同是否可复用，同时用真实新PID／创建时间和登录身份重新认证。凭据缺失、撤销或确需新授权时仍经真实GUI机制，不自动批准或扩权。
- 保留每实例单客户端、每桥接单作业与明确busy／draining。使用专用原生工作线程，保持connect／pair／disconnect的受理和状态语义；取消须关联原作业，等待实际I/O完成／排空后再释放缓冲区、句柄及busy屏障，不能把future丢弃视为业务取消。
- 内部四字节大端帧、长度／握手上限、重复键、深度、Unicode、整数表示和证明字节按现有合同迁移，以跨语言向量验证，不依赖JSON库默认行为。原生异常不作为未审阅文本泄露给MCP调用者。
- 预览及operation继续按产品实例／配对／权限代次归属，由产品判断同一合法绑定下能否继续查询／使用；不能转交新实例。窗口开关另遵循认证会话和窗口生命周期，不能以同配对接管旧窗。演示本地回收凭证不能跨桥接进程搬用。
- 不自动重试已受理写入。断线／超时后核对原操作及实际状态；保存可能已有内存准备、已落盘或重载失败，不能报告完全回滚。Worker STOPPED、Rust任务退出或队列清空不代表产品清理完成。

### 1.3 协议兼容的明确选择

采用官方 rmcp 处理分帧、协议和调度。RUST-A 已用 Rust 1.100.0、rmcp=3.10.0、Windows x64 MSVC 构建，依赖见 Cargo.lock。查阅 3.11.0 发行说明并对比制品源码，本轮两个 stdio 问题仍存在，保留固定版本。

旧版本2024-11-05、2025-03-26、2025-06-18、2025-11-25使用initialize生命周期；2026-07-28使用发现和逐请求元数据，不能强塞旧握手。无共同版本须明确失败，不假定客户端一定接受降级；新协议不改变产品端的单客户端、配对和业务会话边界。

不硬编码统一的“-32002未初始化”。未知方法、未知通知、初始化前业务和元数据错误按相应规范与固定SDK分别验证：有效未知请求返回适当协议错误，未知通知不回请求式错误，拒绝路径业务零执行；可恢复探测后连接仍可用。不可恢复I/O或资源超限可明确关闭，不能挂死或无诊断地退出。

stdout在服务模式下仅输出UTF-8、正确分帧的MCP JSON-RPC并及时发送；日志／诊断走stderr，不输出密钥、敏感挑战或原始异常。固定版本服务端已有“发现不提前锁定生命周期”的逻辑，但Windows stdio仍可能等待阻塞stdin；必须实际验证退出，不能仅凭tools/list成功宣称兼容。

## 2. 分阶段实施与决策门槛

阶段状态以本节和接续卡为准。

| 阶段 | 状态／依赖 | 最小完成门槛 | 权限、预算与停止条件 |
|---|---|---|---|
| RUST-A：有界协议／分发原型 | 选定范围通过 | Windows x64 锁定 Release、原始报文与 Python SDK 11 组、Inspector 双协议、解包复验及实际发布样包测量 | 普通权限，合成数据。结果见 §2.1 |
| RUST-B：Windows IPC／身份／凭据 | 选定范围通过，见 §2.2 | 严格帧及证明、原生身份／ACL／对端、v1／v2 凭据、忙碌、部分帧、取消／超时和实际排空；Qt 私有夹具及普通产品 v1 Read | 两轮原 manifest 测试分别获批。无跨 Windows 登录、PC 重启及完整业务证据 |
| RUST-C：现有全部行为等价 | 本机选定范围通过，见 §2.3 | 全工具合同、实际只读及事务、四窗口 PNG 和两轮演示回收通过；未验配置变体、跨登录和宿主继续保留原范围 | 自有私有桌面及合成配置。C++ 业务与原 Python 维持，复用未受影响的 GUI 及引擎证据 |
| RUST-D：实际客户端与交付 | 本机选定范围完成，见 §3.5 | x64 单 EXE 接入正式打包，Qt6／Qt5 本机真实包内桥接、Codex 与 Win10 离线组件按各自范围实测；Antigravity 既有动态版工具／图片证据保留 | 最终 Antigravity 回合和 Win10 Release PC 发布流程回归主计划；干净 Win11 离线免验；QQ 渠道仍单列 |

A通过只证明原型选定范围。B–D的投入在A评审后校准，不并入原M1／M2／M3-B旧估算，不因讨论过Rust而自动承诺整迁。若现场推进，按实际源码及有效证据接续，不回退或覆盖工作区。

### 2.1 RUST-A 固定兼容用例

| 编号 | 场景及应核对结果 |
|---|---|
| COMPAT-01 | 四个旧版本的initialize→initialized→tools/list／夹具调用，核对实际版本及版本相应结果 |
| COMPAT-02 | 合法server/discover→带完整逐请求元数据调用，以及无需先discover的合法新版请求 |
| COMPAT-03 | 裸探测、正常发现后回到initialize，在同一进程／通道恢复，不提前锁定错误生命周期 |
| COMPAT-04 | 未知请求、未知通知、初始化前业务；错误及无回包语义正确、业务零执行，可恢复场景保持连接 |
| COMPAT-05 | 不支持版本、缺失／错误元数据、阶段混用，按规范拒绝，不伪报协商成功 |
| COMPAT-06 | 错误JSON、错误参数、过大输入、超时、EOF，资源有界、错误／原生退出状态可解释 |
| COMPAT-07 | UTF-8中文／日文、LF／CRLF、及时发送，stdout无日志、banner、空白杂字或残留未发送响应 |
| COMPAT-08 | Schema／默认值／严格参数、文本及structuredContent、isError；同协议版本下对照，不把Schema生成当运行校验 |
| COMPAT-09 | 固定PNG的ImageContent及元数据可消费；仅协议测试图片，不是产品截图／业务验收 |
| COMPAT-10 | 启动／关闭期间取消、输入EOF、输出断开及Windows阻塞stdin，记录有界退出和未排空情形 |
| COMPAT-11 | 独立原始报文客户端、现有Python SDK及Inspector交叉检查，不只用同语言SDK自测 |
| COMPAT-12 | 固定客户端／SDK／协议版本和启动方式记录，实际Codex／Antigravity业务及图片验收留在D，不将协议夹具当完成 |

2026-11-18 在中文及空格路径下运行锁定 Release。原始 stdio 与 Python SDK 2.3.0 共 11 组通过，旧版四个版本、现代发现及直连、探测后旧握手、未知消息、版本和元数据负例、64 KiB 丢弃后继续、UTF-8／LF／CRLF、Schema／文本／PNG、取消及 EOF 均有覆盖。Inspector 2.15.0 CLI、Node 24.22.1 分别选择 legacy 和 modern，列表及图片调用退出 0，固定业务错误保留 isError，Inspector 退出 5。fmt、Release Clippy 零警告及 ZIP 解包后二次矩阵通过。

原样 SDK 的未知首包通知导致进程退出。握手后断开 stdout 而保持 stdin 打开，1.5 秒内仍在运行。适配层忽略未知通知，在实际输出失败时停止服务，并给 Windows 阻塞 stdio 留 200 ms 的运行时关闭预算。裸 discover 返回元数据错误后可在同一进程完成旧握手，正确的 modern discover 同样保留旧握手选择。旧 initialize 对不受支持的版本可以返回可用旧版，现代不受支持版本返回 -32022。兼容用例按对应约定断言。

EOF 时 rmcp 会先给在途响应最多 5 秒排空，本轮首次 5 秒用例超时后分开验证了已取消请求与仍在途请求，后者在 7 秒门限内退出。stdout 断开须到实际输出时才可观测。B 需单独验证客户端取消与原生 I/O 排空，也需核对宿主等待时间。原型进程退出证据只覆盖 stdio 和合成工具。

发布样包 EXE 2,823,168 字节。含 VCRUNTIME140.dll、许可、使用说明及清单共 133 个文件，解压 5,200,825 字节（4.96 MiB），Deflate-9 ZIP 1,379,165 字节（1.32 MiB），PDB 2,617,344 字节单列。首轮样本启动到 tools/list 为 50.80 ms，后 10 次中位 37.20 ms、范围 34.96–41.33 ms。未控制 OS 缓存，未测认证耗时。Win10 最低版本、CRT 和最终完整同功能 ZIP 增量仍在 D 核对。

### 2.2 RUST-B 实施切片

RUST-B1 已通过 4 组 Rust 单测和原有 6 组 Python 协议测试。三种 HMAC 角色逐项匹配 Qt 和 Python 固定向量，角色、代次错配被拒。四字节大端帧、256 KiB／4 KiB 上限、重复及转义同名键、32 层容器、UTF-8 和规范整数／权限位使用独立测试。

独立 Rust 工作线程在发送前及接收后核验对端，每个作业的取消事件单独所有。已受理断开阻止新准入，异步等待取消后继续等待原生结果。真实管道组件覆盖空读取超时、完整帧及两字节前缀后取消，观察原生 ERROR_OPERATION_ABORTED（995）后才释放内存与句柄。普通测试通过 8 组，另显式运行专属凭据用例，真实 v1／v2 均完成 Rust 写、Python 读写、Rust 再读和删除后复核。DPAPI 错熵拒绝。v1 使用 Windows 不变区域小写规则，v2 用文件句柄规范路径，SessionId 仅进入 v2 发现目录。

普通 Medium 的新 Qt6.8.3 asInvoker 夹具运行在自有私有桌面。r02 通过 7 组，其中包括发现缺失、ACL／EXE／进程创建时间／管道负例，忙碌、原生取消、配对拒绝，双向 v1 重新认证、第二客户端拒绝、撤销以及 stdio 关闭时清理等待批准的请求。夹具只返回 transport_only 能力，配置写工具未注册。最后核实专属凭据删除，夹具实际退出 0。没有 QKeyMapper 普通产品、真实用户配对、Default、跨登录或重启证据。

首轮 Qt 编译在受限沙箱返回 C1902 和原生 2，同一目标改用普通 Medium 构建后退出 0，无编译及链接警告。初次互操作 r01 在未启动 Qt 夹具前发现 SDK 调用参数错误表示差异。Python 对已知工具的参数拒绝使用 isError 工具结果，Rust 候选调齐后 r02 通过。Rust 锁定 Release、fmt、全目标 Clippy 和 A 的 11 组 stdio 回归通过；原件见 out/rust_sidecar/b_20261118。

普通产品首轮为 Qt6.8.3／MSVC 真实 High Release，自有区隔 EXE、合成 INI 及私有桌面。Rust 在配对前拒绝身份，产品原生 0 退出。普通 Medium 红例确认 TokenLinkedToken 的长度探测返回实际 Windows 24，导致 Rust 丢失 Python 能取得的关联令牌。改为直接读取固定 TOKEN_LINKED_TOKEN，保留双向关联、账号、登录 SID、SessionId 和相反提权类型核验。Rust／Python 红绿对照、修正版 Release、fmt 和 Clippy 通过。

新隔离副本 read-r02 仅授 Read。Rust 完成真实管理窗配对，Python 及新 Rust 进程复用凭据并重新认证。只读状态匹配原 GUI 的 Tab 和停止快捷键，同一产品状态及读期间 INI 字节不变。撤销后旧配对被拒，专属凭据删除，产品 native 0，固定测试授权撤出，缓存仍截止 19:00。EXE、实际桌面及日志见 out/mcp_product/rust_b_20261118/read-r02。此项覆盖同一 Windows 登录中的普通产品 v1；v2 双向读写来自专属组件测试。普通产品的写入、真实跨登录／重启及终端发布仍待。

### 2.3 RUST-C 实施切片

C1 已对齐九项工具声明／注解／Schema 和已审核错误消息。Rust 分别校验参数、补齐原分页缺省值并省略未指定的查询字段，再向固定认证实例转发。新版及旧版 stdio 对照通过，包括未知工具、错误参数及合法调用未连接的不同结果层。锁定 Release、fmt、Clippy 和 9 组 Rust 测试通过（2 个专属用例保留原 B 结果）。Qt6 普通权限夹具补验 7 组通过，三项新增查询到达认证端并获得夹具声明的 unsupported_capability，待配对资源清理，夹具原生退出 0。原件在 out/rust_sidecar/c_20261118/read-interop-r01。普通产品的查询成功、分页内容及业务结果归属留在 C 产品对照。

C2 已接入预览、取消及提交。行操作、偏好与保存沿用原参数、确认旗标和 C++ 业务准入，发送只一次。公开 Schema 与运行时约束分开，Rust 单测核对五种预览的线协议形状。正式模式 stdio 行上限为 512 KiB，容纳原 Schema 的 32767 字符设定名及 JSON 转义，私有帧仍为 256 KiB。新版及旧版的 12 项合同、98 KiB 中文参数、严格布尔／整数及错误层对照通过。首轮测试用 Python 字典相等混淆 true 与 1，修正判据后两组通过。Rust 锁定 Release、10 组测试、fmt、Clippy 通过。复用 asInvoker Qt 私有夹具，三个事务方法在 Read-only 配对下均返回 permission_denied，合成凭据已删除且原生退出 0。原件在 out/rust_sidecar/c_20261118/transactions-interop-r01。

C2 普通产品对照通过。Qt6.8.3 原 manifest 普通 Release，产品 EXE SHA-256 前缀 211b13e19b40，Rust 3be1f611 EXE f4b7f38e5394，两个独立私有桌面，Read＋Configure 专属配对。Rust 与 Python 对照真实帮助分页及当前源，Python 取消 Rust 预览并建立 A→B 预览，Rust 提交后将发送方式改为 3。正常确认缺失被拒；共享读锁只阻断合成 INI 写入，失败时原 INI 及内存草稿保留；解除锁定后重新预览、保存及重新加载均成功。Python 能查询同一配对的成功／失败 operation。独立冷启动的 Rust 重新认证、真实 GUI 和源均读回 A→B，方式 3 读回；旧源及旧预览被拒，旧 operation 返回 unknown。两次产品原生退出 0，FilterKeys 未变，专属配对撤销、凭据删除，本轮固定测试授权撤出。原件 out/mcp_product/rust_c_20261118/transactions-r01。

本次未运行映射、Default、重载失败注入或完整配置变体；行删除／偏好沿原产品验收及本轮参数对照保留各自范围。

C3 增加五项正式窗口操作，保留 private_background、默认高级窗和显式 IbInput Popup、已认证产品的独立权限及窗口所有权。捕获复核实例／PID／创建时间／EXE、窗口引用、来源上下文、Qt UTC 时间和角色字段，限制 160 KiB／8M 像素并检查 PNG 的尺寸、块边界及 CRC。图片只在 ImageContent 返回一次，文本／结构化结果有校验摘要且省略 Base64。原生 IPC 保持 256 KiB。锁定 Release、fmt、Clippy、13 组 Rust 测试及新旧协议 17 项合同均通过。复用 asInvoker Qt 夹具，Read 拒绝全部五个窗口工具，专属测试凭据已删除，私有夹具原生退出 0。原件 out/rust_sidecar/c_20261118/windows-interop-r01。PNG 单测使用合成图片，真实窗口的允许／所有权／图片检查留待 C4 同批授权验收。

C4a 仅在显式 `--demo-record PATH` 或 `--product-demo-record` 模式注册 qkm_get_demo_state。首次核验在认证后，产品记录只按已认证实例在确定发现目录拼出文件名，沿原 Windows 私有文件检查及 8 KiB 记录上限。观察严格核对 v2 创建记录、EXE／PID／创建时间／登录会话、背景私有桌面、只配置策略、恰当权限及固定配置的普通、共通空源。锁定 Release、Clippy、fmt、15 组 Rust 测试，以及双协议两种记录模式的合同验证通过。共享私有读取实现的 asInvoker Qt 身份、ACL 和排空回归通过，专属凭据移除且夹具退出 0，见 out/rust_sidecar/c_20261118/demo-read-interop-r01。真实产品记录、就绪正例及写入演示留在 C4 后续。

C4b 只有显式带演示记录并加 `--enable-demo-mapping` 才注册 qkm_demo_mapping。单作业内固定调用产品预览、提交、窗口、捕获、按正式提交绑定删除。每次校验当前非行足迹、实例／源及窗口所有权；失败时可验证的已知错误至多尝试一次受限回收，取消／断线／无法确权不重放，只隔离现场。能够完成回收才返回图片及回收证明，dirty 保留。回收记录仅位于同一个 Sidecar 进程。18 组 Rust 测试（另 2 个 B 专属用例保留原证据）、fmt、Clippy、Release、新旧协议各模式全部合同和 11 组 stdio 回归通过。演示组件使用受控响应，覆盖三种视图、连续轮次、权限拒绝、缺绑定、异源、截图超时恢复和取消隔离。真实窗口、图片内容、原生回收及主机接线仍需实测。

C3/C4 的真实正例见 out/mcp_product/rust_c_demo_20261118/windows-demo-r01。Qt6.8.3 原 manifest 产品使用 `--demo-configuration-only` 在自有私有桌面启动一次。产品 EXE 摘要前缀 211b13e19b40，Rust f4cc8c7d 候选 e0a43aa76796。仅 Read＋Configure 时就绪明确拒绝；本专属配对经 GUI 加 ShowWindows、CaptureWindows 后重新认证。Python 读取明确指定的原生记录，与 Rust 认证后定位的产品记录核对相同目标及源。正式 Rust 调用产生四张 PNG，目视检查中文及布局，单项窗实显 A→B；旧 Popup 窗口引用拒绝且无旧图。单项／高级两轮分别建立、关闭及删除归属行，非行足迹、回收记录、操作归属及磁盘字节均核验。高级图用于真实设定展示，键值归属由正式源和操作记录检查。前后采样保持 Default 输入桌面且目标未成前台，FilterKeys 未变。产品原生 0，配对撤销、专属凭据删除，测试授权撤出。无需把新图像、私有运行或原 C++ 守卫证据扩大为 Default、全系统零输入矩阵及 QQ 送达。

### 2.4 B–D必须保留的业务验收

- 身份与凭据：错误EXE／PID创建时间／Windows登录／Session／pipe peer、发现ACL、缺失及损坏凭据、写入失败、撤销、权限不足和单客户端冲突均明确失败，不自动换目标。专属测试凭据验证跨语言兼容，不读取用户个人配对。
- 保存与归属：正式A→B预览、取消、提交、显式完整保存和独立冷读；旧源／预览、权限修订、重复提交、超时／断线后的结果、写失败／重载失败及部分效果按原合同。原Python预览／结果在相同合法归属下的接续也须核对，不因换语言擅自扩大或抹除。
- 只读及参数拒绝：状态刷新不写配置、不改变修订／磁盘或合法预览，不触发设备初始化；SDK参数拒绝和产品业务错误分别保持原语义。
- 窗口与图片：main、item_setup、mapping_advanced、ibinput_settings的显式发现／打开／捕获／关闭、独立ShowWindows／CaptureWindows权限、旧窗口／草稿保护、PNG身份和尺寸校验。Popup动作不含连接检查、偏好修改或通用控件操作。
- 演示：一个作业内复用内部正式请求建立真实场景→原GUI入口→真实PNG→Agent检查→安全回收，避免嵌套公共作业导致自锁。连续请求／失败隔离、旧场景污染和底层零输出按受影响范围核对；不能移植旧桥接的内存回收凭证，也不能自动清dirty或重载配置获得假基线。
- 交付：新客户端明确加载自己的workspace配置，Rust EXE保持普通权限；用户端无Python／Rust／venv／pip依赖，不后台下载。产品原manifest、明确启用MCP及权限批准保持。无环境的客户端／干净系统／QQ渠道如实列缺口，不宣称完成。

新C++业务、驱动行为或系统副作用不包含在迁移顺带改动中；确需改变时先提交具体方案及验证范围。gsudo仅在必要已授权测试启动／关闭时按技能和公共登记入口使用，缓存时长不增加范围、不接管共享或历史未知请求。

## 3. 同口径测量与Python回退

### 3.1 RUST-D1 完整组件候选

本机产物 out/rust_sidecar/d_20261118/component-r02/QKeyMapper MCP Rust 中文.zip，SHA-256 为 `3a1a8d27cb9d2f1ee2e210682df74a120c9602109e0dc809fd248b208d339c7b`。包含完整普通 17 项及条件演示 2 项的已验 Qt 对照候选（EXE 摘要前缀 e0a43aa76796）、应用目录 VCRUNTIME140.dll、项目许可、依赖许可／清单和用法。两次独立封装得到相同摘要。Windows x64 PE 导入只含系统库及所带 CRT，后者取自官方 MSVC 14.44.35112 x64 redistributable 并核对有效 Microsoft 签名。[再分发依据](https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution)

原生 PowerShell 7 及本机 Windows 自带 PowerShell 均完成解包、摘要、实际 app-local CRT 加载、stdio／结构化 PNG 和 EOF 退出 0 检查。受限沙箱首次不能直接加载 Windows PowerShell 安全模块或执行本机脚本；封装改为既有的精确模块加载，脚本以普通 Medium 复验通过。解包后的新旧 SDK 工具及启动模式对照通过。本机仍是开发 Win11，缩减子进程 PATH 及观察加载模块不等于干净机器试验。

两侧使用 Python ZIP Deflate-9 和同一个 QKeyMapper Qt6 部署样本，Python 的九个桥接模块与当前保留源码相同。体积保留包内许可和 DLL，PDB 单列。原始字节统计见 out/rust_sidecar/d_20261118/size-final-r02/result.json。

| Sidecar | 文件数 | 解压 | 同口径组件 ZIP | 同一 QKeyMapper 样本的 ZIP 增量 |
|---|---:|---:|---:|---:|
| Python | 1615 | 60.08 MiB | 25.41 MiB | 25.44 MiB |
| Rust | 128 | 4.65 MiB | 1.62 MiB | 1.62 MiB |

局部组件 ZIP 缩减约 93.6%。比较使用实际完整工具合同和同一普通 Qt6 部署样本，正式三包发布与 Win10 Release PC 验收沿原计划。

### 3.2 RUST-D2 本机宿主验证

实际 Codex CLI 0.169.0 App Server 在中文／空格解包路径加载原 Rust 二进制。显式会话覆盖关闭其他服务器，独立 SQLite 和记录均在 out。真实本地 MCP 列表及 text、ImageContent、structuredContent、isError 分别通过，EOF 后 App Server 退出 0。此次使用合成工具，未调用模型。早期轮次遇到 App Server 引导配置、沙箱本地状态和测试客户端 64 KiB 读取上限，修正隔离配置及测试客户端后确认通过。原件 out/rust_sidecar/d_20261118/codex-host-r07。

同一 Codex 以正式模式发现全部 17 项普通工具，断开时返回原结构化错误。经用户单独批准，复用合成 INI 和既有原 manifest Qt6 只配置产品启动一次。通过 GUI 向本次解包的 EXE 授予 Read＋CaptureWindows；正式 Codex 重新认证，读取真实产品状态、源和窗口，并取得 1130×722、56,622 字节的真实主窗 PNG，目视核对中文及预期窗口。产品和 App Server 原生退出 0，INI 及 FilterKeys 保持原值，目标未成为输入桌面前台，专属配对、凭据及固定授权均清理。原件 out/rust_sidecar/d_20261118/codex-product-r01 和 out/mcp_product/rust_c_demo_20261118/codex-read-r01。

实际 Antigravity CLI 1.5.10 独立 ACP 通过 initialize 和带显式本地 stdio 服务器参数的 session/new。r05 观察到独立解包 EXE 直启，r06 仅用开发诊断转发观察实际 `server/discover`、`tools/list` 和返回的合成工具名，两轮宿主退出 0。转发器只用于核验报文，不属于候选包。早期命令行列表未确认工作区发现，绝对配置目录被宿主拒绝。独立配置尚未登录，实际 tools/call、用户可见图片及活动工作区配置接线仍待授权账号验收。原件 out/rust_sidecar/d_20261118/agy-acp-r05、agy-acp-r06。[Antigravity CLI 官方配置](https://antigravity.google/docs/cli/gcli-migration?authuser=0)

同一 PC-W11 上的 Antigravity CLI 1.5.10 使用现有登录和独立临时工作区的 `.agents/mcp_config.json`，直接启动中文／空格路径中的解包 EXE，调用合成文本和图片工具。CLI 以流式 JSON 记录成功的工具步骤，保存的 2×2、75 字节 PNG 已核对。合成文本轮实得结果和模型回文，最终状态因早期 API EOF 为 ERROR；图片轮在 PNG 落盘后遇到模型服务 503 并超时。另在无产品的 ACP 会话中用诊断转发观察正式入口的 `server/discover`、`tools/list` 和全部 17 项工具，宿主退出 0。原件 out/rust_sidecar/d_20261118/agy-cli-fixture-r02、agy-cli-fixture-image-r01、agy-acp-formal-r01。[Antigravity CLI 的工作区配置](https://antigravity.google/docs/mcp?tab=cli)

本轮一次真实产品尝试仍使用上述 Qt6 合成 INI、只配置模式和自有私有桌面。固定助手仅向解包 EXE 的专属配对授 Read＋CaptureWindows。Antigravity CLI 在正式调用前尝试列资源，随后其 shell 请求按本次会话规则被拒绝；模型服务报告 503，CLI 退出码为 3。本轮没有 Antigravity 真产品 MCP 读取或 PNG。产品原生退出 0，专属配对已撤销、凭据已删除、接口已关闭；合成 INI、FilterKeys 和前／后输入桌面观测符合原基线，产品未成为所采样的前台。其后无产品预检在模型资格检查遇到 EOF，另一次显式 effort 与服务给出的模型不匹配。后续用完整工具名和单独的服务器名复查，结果见下。原件 out/rust_sidecar/d_20261118/agy-product-r01、agy-formal-r01 至 agy-formal-r03 及 out/mcp_product/rust_c_demo_20261118/agy-read-r01。

缓存关闭后的补查区分了启动与调用。CLI 的独立诊断转发实际发送 2026-07-28 版 `tools/call`，正式 `qkm_get_connection_state` 返回 `disconnected`、`busy=false`，Rust 退出 0；回文包含该状态，模型服务仍记录一次 EOF。CLI 换独立名称后直启的对照轮没有生成正式工具缓存或发出调用。ACP 显式配置能直接启动同一正式 EXE，已登录复查也生成了正式工具 Schema，但提示在 90 秒内没有返回工具调用。当前证据不足以定位 CLI 直启差异的原因。下次先完成无产品的原生直连正式调用，再沿同一入口申请私有真产品及截图复验。原件 out/rust_sidecar/d_20261118/agy-formal-r04 至 agy-formal-r06-direct、agy-acp-native-formal-r01 和 agy-acp-signed-formal-r01。

后续 r07 用相同 SHA-256 的动态 EXE 和 CRT 在 ASCII 目录启动，已登录 Antigravity CLI 完成正式 `qkm_get_connection_state`，模型轮返回 SUCCESS。用户随后批准的一次 r02 私有 Qt6 真产品试验使用该 ASCII 部署和 Read＋CaptureWindows，正式调用完成认证、状态、来源、窗口及截图，断开请求已受理。Antigravity 原生落盘的 1130×722 PNG 为 56,622 字节，SHA-256 为 `d06c7c68747faf98ac252f6feb943c075ec644754f2782ad0a23b51bea831669`，与 IPC 返回摘要一致，目视确认中文主窗。临时检查器原先读取错误的尺寸字段，随后以同一原件和正式像素字段通过复核。截图与工具结果已取得，最后模型回复超时，CLI 返回 3。产品退出 0，专属配对与凭据已清理，合成 INI、FilterKeys 和所采样的前后台状态符合基线。原件 out/rust_sidecar/d_20261118/agy-formal-r07-ascii、agy-product-r02、out/mcp_product/rust_c_demo_20261118/agy-read-r02。单 EXE 的 r08 与新 ZIP 中文／空格路径 r09 在模型会话超时前没有实际工具调用，原件同目录。

用户选择先在本机完成验证。VirtualBox 只读核对发现关机的 Win10_x64 已配置 Win10_x64_share，缓存来宾工具版本 7.2.16。安排 D3 时先核对实际来宾在线、登录、网络隔离、共享访问与可用执行入口，再传输已核验 SHA-256 的候选 ZIP 和原生 PowerShell 验证脚本；缓存的版本只是准备线索。这是进入本轮前的资源核对，实际来宾结果见 §3.3。[VirtualBox 来宾工具](https://docs.oracle.com/en/virtualization/virtualbox/7.2/user/guestadditions.html)

### 3.3 RUST-D3 单 EXE 候选与离线验收

用已有固定 Rust 1.100.0 和 Windows x64 MSVC 执行锁定离线 Release 构建，独立目标目录设置 `+crt-static`。同一正式 Rust 源编出的 `qkeymapper_mcp.exe` 为 4,131,328 字节，SHA-256 为 `e7517fd55a2ae730cea9e0d7a6d02a9cedef51bf7b52c8397d42fecd6cec9f8f`。PE 导入仅含 Windows 系统库，无单独 VC Redistributable DLL。Win11 开发机通过 11 项 stdio 定向测试，旧／新协议各自的 17 项正式工具合同和条件演示工具门槛均通过。[Rust C 运行时链接说明](https://doc.rust-lang.org/reference/linkage.html)

[候选打包和烟测入口](../../mcp_bridge_rust/README.md#windows-x64-component)按 PE 导入决定随包 CRT。动态分支仍核验和携带签名 CRT，单 EXE 分支输出 `qkeymapper_mcp.exe`，清单 `app_local` 为空。动态缺 CRT 和静态误带 CRT 均在落盘前拒绝。两次独立生成的单 EXE ZIP 摘要相同 `ec308f89723465d0b24c7fe180ab0790940c38daffde31e7fd4503a3de14e6c5`，1,690,757 字节，127 个文件中只有一个二进制运行文件，其余为许可证、清单和用法。Win11 开发机用 Windows PowerShell 验证原动态 ZIP、重打动态 ZIP 及新单 EXE ZIP 的清单、模块加载、PNG 和 EOF 退出。原件 out/rust_sidecar/d_20261118/static-crt-r02、component-shape-r01。

VirtualBox Win10 Pro x64 19045.2965，Windows PowerShell 5.1、Medium，虚拟链路断开且来宾观测无活动网络。常用程序入口、Python 注册和用户 Rust 工具链检查均未发现开发运行环境。r01 的动态旧协议及 app-local CRT 已通过，后续模块筛查误把系统 `msvcp_win.dll` 计作需分发的库。r02 校正后，动态组件和独立单 EXE 均通过中文／空格路径下的旧协议 PNG、新协议 17 项正式工具和断开状态、EOF 退出 0。r03 用上述同一单 EXE ZIP 解包复验通过。所有临时网线和共享均恢复，来宾测试材料保存在获准的 `C:\tools\QKeyMapper` 子目录。原件 out/rust_sidecar/d_20261118/d3-win10-r01 至 r03。上述为组件验收，完整 QKeyMapper Release 组装及真实产品业务仍按以下门槛核对。

本轮只对 x86 做理论评估。固定 Rust 工具链和当前开发环境均只安装了 x86_64-pc-windows-msvc 标准库，未安装 i686 目标。Rust 提供面向 Windows 10 及以上的 i686-pc-windows-msvc 路径，理论上可另建 32 位程序；Windows 7 的 i686-win7-windows-msvc 是独立 Tier 3 目标，现有工具链与构建脚本没有对应交付能力。本轮不安装、编译或试运行 x86 Sidecar。明确 **Windows 7 x86 不支持 QKeyMapper MCP 功能**，Qt5 x86 包不带 Sidecar，本限制写入[包内 MCP 使用说明](../../mcp_bridge_rust/BUNDLE_README.md)。QKeyMapper 的其他 Qt5 兼容范围另按产品发布说明。[Rust 平台支持](https://doc.rust-lang.org/rustc/platform-support.html)／[Windows 7 Rust 目标](https://doc.rust-lang.org/rustc/platform-support/win7-windows-msvc.html)

### 3.4 交付与回退

1. A的最小原型单列能力范围，不与完整Python组件直接得出全量节省比例；C／D再比较相同能力的完整组件。
2. 两侧用同一压缩器和选项，记录文件数、解压字节、组件ZIP及同一完整Release添加Sidecar前后的ZIP差值。许可、包清单及必要DLL计入，PDB单列；现有25.42 MiB组件ZIP不是完整Release增量。
3. 记录实际源码／相关差异、工具链／Cargo.lock、EXE路径／架构及必要依赖身份。冷／暖启动至可发现工具、首次认证查询分开测，产品启动与UAC等待不混入桥接性能。
4. 不为数字删除鉴权、资源上限、诊断或回收功能；不保证单文件、某一体积、毫秒启动或杀软不误报。
5. 用 A 的实际体积、部署及兼容修补成本判断继续投入。若后续出现不可靠的兼容／退出、需要放宽凭据或排空要求、需要常驻 Python，或完整收益不足，则保留 Python 正式实现并重新评估。默认入口切换决定见 §3.5。
6. 回退用显式启动配置完成；先停止新操作并核对已接受任务／结果，不自动切运行时、换目标或重发写入。旧入口、旧凭据和Python实现保留；不做无必要的第二套密钥存储。

### 3.5 RUST-D4 正式打包接线与迁移结项（2026-11-18）

正式发布脚本对同一 x64 组件依次核对验收 SHA-256、Rust 源树与依赖锁对象、解包清单、原生 stdio／PNG／EOF 和 PE 架构。Qt6 与 Qt5 x64 的 ZIP 默认使用 mcp/qkeymapper_mcp.exe，旧 Python 和别名从本次临时发布树移除，Qt5 x86 仍走原产品发布流程。操作者明确修改客户端项目配置，先断开旧桥接并保留原参数和独立回退组件；不改现有用户项目配置。[发布入口](../../scripts/release/Release使用说明.md)／[包内用户说明](../../mcp_bridge_rust/BUNDLE_README.md)。

在 PC-W11 以 606c2292 上的 C++ 源码（与 3fefc26a 相同）从空输出目录构建 Qt 6.8.3／Qt 5.15.2 x64 Release 和匹配 CrashReporter。Qt6 零警告、Qt5 仅既有 Qt 官方 LNK4099 例外，最早复用目录的 C1902 失败日志保留。用发布脚本实际组装、核对并解包两支本机 ZIP；组件 127 文件、解压 4,847,853 字节／ZIP 1,690,078 字节，唯一运行二进制 SHA-256 与 §3.3 的 Win10 离线样本相同。独立组件 ZIP SHA-256 为 c07ce03f82d2a2515fccf4a07538e079487dc3c500d69e39624a704ff3486b7c。发布脚本本机适配自检通过 154 项，含新摘要、源码身份、双包载荷及 Qt5 x86 包形态；原自检的一项固定 Win10 Qt 厂商库路径夹具保留在正式源码，本机适配副本跳过该项。本机 ZIP 使用现有 Qt／厂商 DLL 和更新器，缺正式站配套 AHK，完整三包流程仍由发布站验收。

双 Qt 使用已核对同一字节的隔离产品副本，在中文及空格路径下按 Python→Rust→Python 顺序走正式 MCP。各自通过真实 GUI 配对 Read，Rust 用同一专属配对重认证并拒绝未授权截图，增加 CaptureWindows 后取得 1130×722 真主窗，切回 Python 仍复用同一配对及许可。实际 Codex App Server 再用同一 Qt6 包内 Rust EXE 完成认证、读取、真图及正常退出。Qt5 与 Qt6 中文画面已查看。首轮 Qt5 固定助手的目录／EXE 命名安全检查拒绝，产品退出 0；后续用现有约束允许的独立副本通过。终轮两产品退出 0，专属配对、凭据和授权均清理，合成 INI／FilterKeys 及观测的输入桌面保持。全程私有桌面，只配置模式，未运行映射或发送系统输入。

最终 EXE 的 Antigravity 无产品预检在发出工具调用前超时（退出 3，空回文）。早期 ASCII 动态版真实图按 §3.2 保留，不据此推出最终单 EXE／完整模型回合通过。用户决定干净离线环境以已验 Win10 x64 样本为准，主要功能由本机双 Qt x64 支持；Windows 7 x86 的说明按 §3.3。当前适合完成的迁移工作结项，默认发布接线生效。Antigravity 定向补验和 Win10 Release PC 正式三包流程转入主计划。原件 out/rust_sidecar/d4_20261118、out/mcp_product/rust_d4_20261118，仅本机。

## 4. 与现有MCP任务及本轮收尾的关系

完整M1／M2、PERSIST-137真实跨登录／PC重启、普通用户多实例前台绑定、主动设备工具、完整M3-B及QQ渠道目标保持，原证据不扩大。原型通过不代表任何完整阶段完成。Python 环境锁继续服务开发、对照测试和显式回退。M3-B 默认运行包已改用 Rust x64 组件，按主计划完成正式发布。

普通用户的Default前台绑定流程不能套到后台演示；本轮没有新增Default测试。后续若需要实际前台／输入／原生UAC或重启场景，另说明具体动作、影响及验证方法，不从本计划或缓存时长推导授权。

33ebffc8 记录了 2026-11-17 的只读评估和文档检查。11-18 从该提交接续并完成 RUST-A 独立协议原型，随后按依赖推进 B。

本文件结项归档；后续仅在主计划推进。EXE、日志和截图留本机 out，复用时分别标明源码、产品／组件／替身、系统／桌面和权限。

## 官方核对入口

- [rmcp 3.10.0固定源码与内容类型](https://github.com/modelcontextprotocol/rust-sdk/tree/rmcp-v3.10.0)
- [固定版本的服务端生命周期](https://github.com/modelcontextprotocol/rust-sdk/blob/rmcp-v3.10.0/crates/rmcp/src/service/server.rs)／[stdio实现及Windows限制](https://github.com/modelcontextprotocol/rust-sdk/blob/rmcp-v3.10.0/crates/rmcp/src/transport/io.rs)
- [2025-11-25旧生命周期](https://modelcontextprotocol.io/specification/2025-11-25/basic/lifecycle)／[2026-07-28版本与逐请求规则](https://github.com/modelcontextprotocol/modelcontextprotocol/blob/main/docs/specification/2026-07-28/basic/versioning.mdx)
- [2026-07-28 stdio、取消和关闭](https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/stdio)／[rmcp 3.11.0 发行说明](https://github.com/modelcontextprotocol/rust-sdk/releases/tag/rmcp-v3.11.0)
- [Rust 1.100.0发布说明](https://blog.rust-lang.org/2026/11/12/Rust-1.100.0/)／[Windows官方Rust绑定](https://github.com/microsoft/windows-rs)
- [JSON-RPC错误及通知规则](https://www.jsonrpc.org/specification)
- [现有桥接及测试入口](../../mcp_bridge/README.md)／[现有目录包用法](../../mcp_bridge/BUNDLE_README.md)
