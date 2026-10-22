# 验证记录与证据索引

[返回唯一计划与进度](qkeymapper_mcp_automation_plan_codex.md) · [能力与等价性矩阵](capability_matrix.md) · [目录规则](../README.md)

## 记录原则

本文件仅记录当前实际检查与必要证据索引；进度和下一步只在主计划维护。实际结果按来源机器、源码、构建及场景区分，不将 Win10 结果当成 Win11 结果。

运行记录包含实际 EXE、相关源码与差异、配置、操作桌面／输入方法和完成范围。原始日志、PNG、转储等留在本机 `out/`，未随 Git 传输时明确注明。日志摘录先脱敏；证据缺失不写“通过”。

## DOC-20261022-01：M0.1 文档迁移与初始化

- 日期：2026-10-22。
- 机器：PC-W10；PC-W11 未接入检查。
- 源码基线：`d49a9f57d280d3b3cecc8204372ae7dc13bf6e0d`。
- 任务分支：`codex/qkeymapper-mcp-control`。
- 变更范围：本任务四份 Markdown、`AGENTS.md` 一条导航，以及移除忽略目录内的旧 Codex 计划正文；原 Gemini 方案保留。
- 构建／EXE／桌面：本轮均不适用，没有编译、启动产品、输入注入或 Default 场景。
- 提交／同步：尚未创建本任务本地 commit，未 push；当前只是本地开发检查点。
- 本轮原始产品证据：无。文档检查使用只读 Python 标准库断言和 Git 检查，结果如下；没有新建维护测试或 JSON 回执。

| 检查 ID | 内容 | 结果 |
|---|---|---|
| DOC-01 | 四份文档及 AGENTS 导航、唯一主计划和旧副本迁移 | 通过；新位置唯一正文，旧 Codex 文件已移除，原 Gemini 文件保留 |
| DOC-02 | 仓库相对链接、源文件和文档锚点 | 通过；AGENTS 与新文档共 72 个本地链接／引用检查，2 个外部参考沿用本会话已核对内容 |
| DOC-03 | UTF-8 无 BOM、LF、尾随空白及 Git diff 空白 | 通过；5 个变更文档字节／文本检查；git diff --check 退出 0，新文件另由断言覆盖 |
| DOC-04 | Git 分支、可跟踪／未被忽略、变更范围及无关文件保留 | 通过；任务分支已创建，4 个新文件未被忽略但尚未加入索引，AGENTS 仅增加一条导航，无关本地配置未触碰 |
| DOC-05 | 单文件 256 KiB、任务目录 2 MiB 建议及敏感／二进制材料排除 | 通过；仅 4 份 Markdown，全部低于建议上限，未加入运行材料、个人配置或凭据 |
| DOC-06 | 已批准目标、等价性、阶段时间、双机交接、授权及初始状态一致性 | 通过；有效状态／最终效果硬门槛、48–84／60–105 小时估算、用户确认提交并手工推送、模块骨架及未实施状态均明确保留 |

Git 状态和忽略规则检查会提示无法读取当前沙箱账户下的用户级 Git ignore 文件；仓库级规则、实际新文件状态及差异已核对，没有修改全局 Git 配置或扩大文件范围。此检查不代表文件已提交或已同步远端。

以上 DOC 检查即使通过，也仅证明文档组织和一致性。M0 完整能力盘点、协议、安全实现、模型／开发环境配置，以及所有产品行为均未验收。

## 产品与双机验收状态

| 验收范围 | PC-W10 | PC-W11 | 说明 |
|---|---|---|---|
| 开发依赖与实际测试环境 | 基础依赖已核对；隔离 Qt 管道探针通过 | 未核验 | ENV-20261022-03、IPC-PROBE-20261022-04；MCP 依赖、产品及驱动／输入环境未验收 |
| Qt6／Qt5 产品构建 | 未执行 | 未执行 | 不为文档修改运行产品构建 |
| MCP 协议、配对、访问控制 | 未实现／未执行 | 未实现／未执行 | 能力接口仍处于计划阶段 |
| GUI／MCP 有效状态及最终效果等价性 | 未执行 | 未执行 | 控件相同不构成通过 |
| 停止完成、释放与实际输入 | 未执行 | 未执行 | 现有测试的历史结果不计入本任务当前验收 |
| 保存失败、持久化及冷启动 | 未执行 | 未执行 | 未来核对实际磁盘和新进程 |
| 真正跨机推送／拉取及恢复 | 未执行 | 未执行 | 当前文档未提交、未推送，不能视为已完成交接 |
| 系统／驱动副作用和恢复 | 未执行 | 未执行 | 本切片未操作驱动、FilterKeys、实际映射或系统输入 |
| 性能、普通 Release 与打包交付 | 未执行 | 未执行 | 没有零开销或固定延迟结论 |

## 后续单次验证记录

后续在本文件追加精简记录：记录 ID、日期／机器／系统、源码与相关变化、Qt／构建、实际 EXE、桌面／输入、能力与用例、预期／实际、结果、未覆盖项及必要证据位置。

失败后保留失败结论与原因；修复后追加新结果，不覆盖原失败或把未执行项改成通过。没有变化的历史输入可明确复用，并指出对应版本和边界。编译、内部轨迹、GUI、实际输入及目标程序效果分开表述。

## SRC-20261022-02：M0.2 首轮源码盘点

- 机器／基线：PC-W10，`d49a9f57d280d3b3cecc8204372ae7dc13bf6e0d`；产品源码未修改。
- 方法：Python 标准库 ElementTree 读取根目录 16 个 `.ui`，按常见输入、颜色与交互容器分类；搜索手动连接、自动槽、C++ 构造、动态菜单，再针对关键函数阅读校验／写入／派生值／保存和 Worker 消费路径。
- 结果：316 个 UI 定义定位项（294 个常见控件、12 个颜色控件、10 个交互容器），71 个 C++ 构造／按钮定位项，合计 **387 个唯一定位 ID**。其中包含只读项、暂存编辑和重复创建入口，不等于 387 项业务能力。
- 在所选菜单相关源码的 136 处 `addAction` 匹配中，归并记录 **38 行动态动作族／入口**；重复位置、动作组、注释与多个同意图入口不能相加为能力数。
- 记录 **16 条关键链路**：普通映射添加／编辑／删除、映射码、保存、开始、停止、数据来源、缩放、通用高级、陀螺仪来源、阈值联动、通知、面板、浮动按钮和托盘。详见能力矩阵；未追踪部分保留待办。
- 关键发现：输入采集对 Shift／Ctrl／焦点及当前 Tab 的依赖；`activated`／`textEdited` 与程序赋值差异；原始／主／抬起键及备注的派生维护；GUI IDLE、Worker STOPPED 和真正清理完成的区别；保存后的表重建及共享 QSettings 缓存。
- 边界：入口候选和静态链路检查通过；**完整逐项业务盘点未完成，任何 MCP 接入及 GUI／MCP 运行等价性均未执行**。未启动 QKeyMapper、未修改个人 INI、未使用真实输入或驱动。

## ENV-20261022-03：PC-W10 基础环境核对

| 项目 | 实际结果 | 边界 |
|---|---|---|
| OS／架构 | Windows 10 10.0.19045，AMD64 | 本机实测；不能代替 Win11 |
| Qt6 | `C:/Qt/Qt6/6.8.3/msvc2022_64/bin/qmake.exe -query QT_VERSION` → 6.8.3，退出 0 | 后续隔离探针已实际编译运行，未构建产品 |
| Qt5 | `C:/Qt/Qt6/5.15.2/msvc2019_64/bin/qmake.exe -query QT_VERSION` → 5.15.2，退出 0 | 同上；共享源兼容不等于完整产品兼容验收 |
| MSVC | vswhere 定位 Community 2022 17.14.36811.4；后续通过 `vcvars64.bat` 构建探针 | 两套 Qt 使用本机 x64 MSVC；路径只作本机证据，接收 PC 重新定位 |
| Python | 当前命令的 Python 3.11.9；该解释器未装 MCP、Pydantic、AnyIO、PyInstaller、pytest | 未安装或修改全局依赖；不代表其他解释器或机器没有这些包 |
| PC-W11／设备／权限 | 尚未探测 | 未操作驱动和真实输入，未检查实际映射条件 |

基础环境读取为非提权操作。Windows 工具本地化输出用其 UTF-8 选项读取；定位成功不代表全部产品构建、MCP 打包和开发权限已就绪。

## IPC-PROBE-20261022-04：本机 Windows 管道接管可行性

- 目的：验证 `CreateNamedPipeW` 配合 Qt `QLocalSocket::setSocketDescriptor` 的最小路线，避免在 `QLocalServer::listen(qintptr)` 不支持 Windows 的情况下开始错误集成。
- 源码：本机一次性探针 `out/mcp_m0_20261022/ipc_probe.cpp` 和 `.pro`，**未随 Git 同步**。它们不引用或启动产品，不算维护中的 MCP 实现；接续验收应使用后续正式传输组件及维护测试，不能把此本机原始材料当成已到达另一台 PC。
- 产物：`out/mcp_m0_20261022/qt6/release/qkm_ipc_probe.exe`、`out/mcp_m0_20261022/qt5/release/qkm_ipc_probe.exe`；均为 x64 console Release，QtCore＋QtNetwork、Win32＋Advapi32，未提权。
- 操作范围：随机命名的单个本机管道；logon SID DACL；`PIPE_REJECT_REMOTE_CLIENTS` 与 `FILE_FLAG_FIRST_PIPE_INSTANCE`；明确最小读写权限的原生客户端，随后将双方已连接 HANDLE 交由 Qt 接管。均在同一测试进程、同一当前用户／会话内完成，无产品 Hook、GUI 窗口、焦点／键鼠或驱动操作。
- 看门狗：每个接收等待最多 2 秒，外部进程最长 10 秒，失败返回非零；当前两次最终运行均正常返回。

| 检查 | Qt6.8.3 | Qt5.15.2 | 能证明的范围 |
|---|---|---|---|
| qmake／JOM 构建 | 退出 0，最终构建日志无 warning／error | 退出 0，构建日志无 warning／error | 此独立探针在两套指定 Qt 下可编译链接 |
| logon SID 描述符及客户端连接 | 通过 | 通过 | 当前 logon 授予的明确访问掩码可建立测试连接；未执行其他用户或会话拒绝测试 |
| 首实例冲突 | 第二次同名创建被拒绝 | 同左 | 当前管道存在时不接受同名首实例创建；不等于已证明完整抗冒名服务端能力 |
| Qt 接管＋双方身份 API | 通过 | 通过 | 两端可读取实际 PID／会话，且等于本测试进程和会话；未测试跨进程 Token 身份或认证 |
| 大于管道缓冲的往返 | 65,537 字节精确接收；回复精确匹配 | 同左 | Qt 异步字节传输在该条件下可行，不代表 MCP 帧／鉴权／业务协议实现 |
| 关闭与同名重建 | 双端 abort／析构后可重新创建首实例 | 同左 | 本次正常路径释放了阻止重建的管道引用；未覆盖慢连接／半包／异常关闭矩阵 |
| 最终进程结果 | 退出 0，stdout PASS，stderr 空 | 同左 | 本机探针通过；产品／Win11／完整安全边界未验收 |

原始证据位于各 Qt 子目录的 `configure.log`、`build.log`、`run.log`，只在 PC-W10 的忽略目录中。Qt6 初次构建出现探针自带 `/W4` 被 qmake 的 `/W3` 覆盖告警，已移除重复警告参数并重新编译；初次运行把 `flush()` 返回值误当作传输成功条件而失败，已改用实际接收字节断言。原始失败分别保留在 `qt6/build_initial.log`、`qt6/run_initial.log`；最终结果记录在新日志，未覆盖成历史通过。

**未证明项**：远程客户端实际连接拒绝、其他用户／登录会话拒绝、Token／创建时间身份校验、配对／撤销、凭据保护、长度帧与请求限额、慢连接、跨进程故障、产品 GUI／Worker 生命周期、普通映射及任何实际输入效果。`PIPE_REJECT_REMOTE_CLIENTS` 已用于创建探针，但不能把设置该标志写成已完成远程攻击场景验收。

## DOC-20261022-05：本轮检查点一致性

- 五个变更 Markdown（AGENTS 及四份 handoff 文档）：UTF-8 无 BOM、LF、文件末尾换行、无尾随空白、代码围栏成对；每份及任务目录低于建议大小上限。
- 本地链接／引用 108 个均存在；387 个候选定位 ID 唯一，UI 定义数 316、动态项 71、动作族／入口行 38、关键链路 16，与进度卡一致；71 个动态项的构造行、成员名及所属类逐一核对。计数为当前静态清单范围，未冒充业务能力覆盖数。
- `git diff --check` 退出 0；现有已跟踪差异仅 `AGENTS.md` 一条导航；新交接文档未被忽略但尚未提交；一次性探针及产物仍受 `out/` 忽略规则约束。
- HEAD 仍为 `d49a9f57d280d3b3cecc8204372ae7dc13bf6e0d`，任务分支未变；原 Gemini 文件保持 10,716 字节、旧 Codex 正文不存在，既有 `.claude/settings.local.json` 保留。
- 本轮只形成**本地开发检查点**，没有创建提交、推送或建立正式 MCP 监听；Qt 探针结果与产品、GUI 等价性和 Win11 未执行项分开记录。
- 两个探针进程均已退出，最终进程检查没有残留 `qkm_ipc_probe`；本轮没有输入或驱动状态需要恢复。

## GIT-20261022-06：文档基线独立提交切点

- 用户已明确授权：由 Agent 判断是否适合本地提交，适合则提交后继续计划；不授权自动推送。
- 判断：当前五份 Markdown 构成独立切点，保存 M0 首轮设计、源码入口和可行性证据边界；不包含产品改动、个人配置、一次性探针及产物。
- 拟提交范围：`AGENTS.md`、`handoff/README.md` 及本任务三份 Markdown；提交前核对索引文件集合、编码、链接、空白与大小限制。
- 提交消息：`docs(mcp): establish automation plan and handoff baseline`；实际提交号在提交后由主计划接续卡片记录，不在提交内容中递归写入自身哈希。
- 历史文档／探针检查按原记录复用；本提交不新增产品、GUI、输入或 Win11 验收结论。既有 `.claude/settings.local.json` 不进入索引。
- 实际结果：已提交 `c6ca3388a2de3141539cca2621f45a93e3636ab6`，五份 Markdown、1,180 行新增；未推送。提交后继续 M0.3b／M1.1a。

## IPC-20261022-07：M1.1a 消息帧与真实进程身份

- 机器：PC-W10，Windows 10 19045 x64；PC-W11 未运行。
- 源码：以文档基线 `c6ca3388a2de3141539cca2621f45a93e3636ab6` 为起点，新增 `QKeyMapper/automation/qkm_ipc_frame.*`、`qkm_local_identity.*`、`tests/mcp_ipc/`，补充测试导航和任务记录；未改产品 `.pro`、主窗、配置、Worker、驱动或 `.ts`。
- 构建：同一维护测试目标 `QKeyMapper/tests/mcp_ipc/mcp_ipc_test.pro`，x64 console Release＋调试符号、asInvoker，MSVC 2022 工具链；分别链接 Qt6.8.3／Qt5.15.2。
- 实际 EXE：`out/tests/mcp_ipc/qt6-release/release/QKeyMapperMcpIpcTest.exe` 与 `out/tests/mcp_ipc/qt5-release/release/QKeyMapperMcpIpcTest.exe`。
- 桌面／权限：QCoreApplication 无界面测试及自有子进程，未提权；没有 GUI 窗口、Default 焦点、Hook、键鼠或驱动操作，未加载个人 INI。
- 原始证据：上述两个构建目录中的 `configure.log`、`build.log`、`test.log`、`results.txt`；仅在 PC-W10 的忽略目录，不随 Git 同步。维护源码、测试及说明随 Git 同步，接收 PC 可按说明重新验证。

| 检查 | Qt6.8.3 | Qt5.15.2 | 结论范围 |
|---|---|---|---|
| qmake／JOM | 退出 0；最终构建日志零 warning／error | 同左 | 两个新组件及测试目标可编译链接；不是产品构建 |
| 完整 QtTest | **30 passed，0 failed，0 skipped**；退出 0、stderr 空 | 同左 | 含 init／cleanup；数据行数不等于业务能力数 |
| 消息结构 | 半包所有切点、逐字节、连续帧、4 KiB／256 KiB 精确边界、无效／超大头、32 层边界通过 | 同左 | 长度核对后才分配当前帧，错误后不重新同步；socket 本身和请求队列仍需限额 |
| 数据正确性及失败 | UTF-8、多语／emoji、转义 NUL、畸形 JSON／代理字符／BOM／非有限数值、读失败、截断、旧输出清除通过 | 同左 | 合法内容不被隐式截断／转换；不能代替具体命令参数校验 |
| 真实管道对端 | 两个独立进程通过本机随机管道连接，双向核对 PID／创建时间及登录／会话 | 同左 | 报文故意自报父 PID，而 Windows 读取的真实对端仍为子进程 |
| 拒绝／比较 | 空身份、不可用 PID／管道、合成 SID／AuthId／SessionId／PID／创建时间变化、已退出子进程通过 | 同左 | 比较逻辑和所测 API 失败路径通过，不等于真实异用户／异会话验收 |

Qt5 首次完整运行是 **28 通过、1 失败**：合法字符串中内嵌 NUL 被 Unicode 往返校验误拒绝。已核对两套本机 `qstring.h`：Qt5 的 QByteArray 重载使用 `qstrnlen`，Qt6 不同。修复为共用的指针＋显式长度重载，并在映射对象之外直接断言键和值中 NUL 前后的内容。原 Qt5 失败保留于 `qt5-release/results_before_qt5_nul_fix.txt`；当前两版结果是修复后重新编译并运行所得。没有修改 Qt 或将缺失测试计为通过。

风险及未覆盖：这些代码尚未加入产品，不产生新的产品入口；正式 listener、管道请求安全上下文、配对／授权／撤销、普通与高权限连接、慢连接／队列、MCP SDK、业务操作、GUI 对照、真实输入、Win11 和完整产品兼容均未完成。初始选择两套 Qt 的边界／跨进程测试与源码审查，后因新增测试目标需要登记工具链，追加了工具回归及三个新增 C++ 文件的定向分析，见 TOOLS-20261022-10。未运行产品构建、ASan 或性能验收；不能证明完整接口安全或零开销。

## DOC-20261022-08／GIT-20261022-09：M1.1a 独立切点

- 当前切片共 13 个源码／工程／说明／工具／交接文件；UTF-8 无 BOM、LF、结尾换行及无尾随空白检查通过。相关 7 份 Markdown 共 118 个本地链接／引用存在；handoff 文档单文件及目录大小符合上限。
- 代码审查重点：长度先验后分配、固定大小头的部分读取、错误状态不恢复／不返回旧消息、Unicode 与数值不隐式转换、32 层递归边界、SID 长度及 TokenGroups 容量校验、失败路径空身份、只读进程／Token 权限、句柄所有权与子进程清理。具体命令 schema 和权限检查留给下一层，不能由帧结构校验代替。
- `git diff --check` 通过；产品 `.pro`、主窗口、Worker、驱动和翻译均未改变。当前新增源文件只由测试工程显式编译；Git 索引仅加入本切片的 13 个文件，不含 `.claude/settings.local.json` 或 `out/`。
- 判断：有明确边界的基础组件、两套 Qt 实际维护测试及相应记录构成独立提交切点。依照用户授权创建 `feat(mcp): add bounded IPC frames and local peer identity` 本地提交；提交结果由 Git 核对，未推送。
- 验证基线为 `c6ca3388` 加本切片相关变化；最终 Qt 运行后，C++ 仅补充身份头文件的前置条件注释，不改变有效测试输入；两份 PowerShell 工具的登记与准备步骤另外通过 TOOLS-10 验证。文档更新不误记为产品构建或额外运行验收。

## TOOLS-20261022-10：新测试目标登记与定向分析

- 新增第一方 `.cpp` 后，现有工具的全量源文件清单会发现它们；必须把独立 MCP 测试目标登记到 `scripts/run_static_analysis.ps1`，使用它自己的 QtCore／QtNetwork／QtTest、asInvoker 和 CRT 参数。未将新组件冒充产品主目标，也未让测试继承 QWidget／SDL 参数。
- 同步 `scripts/test_static_analysis.ps1`：检查三个新增源文件各自被覆盖、目标参数独立、meta-object 输入已生成，并继续验证 vendor 提示排除、第一方提示拒绝和 native 工具非零退出拒绝。
- 首次真实定向分析：两个基础组件的 Clang-Tidy／Clazy 均退出 0、无第一方诊断；测试文件因影子目录未生成 `test_mcp_ipc.moc` 失败。修复为在分析影子目录仅生成该输入（qmake／MSVC nmake／moc），没有编译或运行产品；维护回归加入文件存在断言。
- 修复后工具链回归通过：`out/tests/mcp_ipc/static_analysis_regression_final.log`，明细在 `out/tests/static_analysis/c28c83d5dd084f7e9be05167c1cf50c4/`。回归中的提示和非零退出是故意注入的报告门禁用例，不能据此宣称全产品 C++ 分析通过。
- 实际定向分析共三个新增 C++ 文件、各 Clang-Tidy／Clazy 检查。两个未变的基础组件沿用首次逐文件通过结果；修复后仅重跑受影响测试文件，双方工具退出 0、第一方诊断 0。原记录在 `out/tests/mcp_ipc/static-analysis-before-moc/`，最终测试文件结果在 `static-analysis-final-test/`，均未随 Git 同步。
- 工具：LLVM 22.1.8、Clazy 1.16，Clang-Tidy 使用现有 `-*,clang-*`，Clazy 保留现有 52 项检查；分析环境按现有脚本选择 MSVC BuildTools 14.40.33807，普通维护测试构建使用 Community 14.44.35207，均为 x64 MSVC 2022。没有更改产品编译标志或诊断规则。
- Git 安全目录仅对工具进程及其子进程配置，不改全局 Git 设置。首次工具回归的历史日志包含未设置该进程配置时的 qmake Git 元数据读取报错；最终回归和实际分析使用受控仓库路径成功取到 `c6ca3388`。用户级 ignore 文件读权限提示仍属既有沙箱边界，仓库与新文件状态另外核对。

## IPC-20261022-11：M1.1b listener 与认证前边界

- 基线：本地 `6aee2a39` 加本切片 `qkm_local_listener.*`、维护测试／工程、分析工具覆盖断言及说明／交接变化。PC-W10，非提权，x64 console Release／asInvoker；组件仍只由维护测试编译，产品工程、主窗口、Worker、驱动和个人配置未改。
- EXE：`out/tests/mcp_ipc/{qt6-release,qt5-release}/release/QKeyMapperMcpIpcTest.exe`。沿用各自 Qt6.8.3／Qt5.15.2 kit，Community MSVC 14.44.35207。qmake 与最终 JOM 均退出 0，最终 `configure_listener.log`／`build_listener.log` 无编译／链接警告和错误。
- 最终运行：两套 Qt **各 54 通过、0 失败、0 跳过**，进程退出 0，stderr 空；Qt6 19,283 ms，Qt5 19,106 ms。原始 `results_listener.txt`、`test_listener.log` 在各构建目录，不随 Git 同步；外部看门狗 40 秒。

| 验证项 | 当前结果及边界 |
|---|---|
| 默认关闭／显式启停 | 未启用没有端点；20 次启停、挂起 accept 的取消及析构均通过；重复 start 保持原实例，停止后新 start 使用新实例 |
| 真实独立进程与重连 | `hello`／`pair.request` 首包字段及真实 PID／创建时间对照；两种各四轮同名端点重连；非法请求拒绝后再连接正常首包；未认证首包不发送 `ready` 或任何输出 |
| OS／Qt 边界 | 实际 pipe 的单条允许 ACE、logon SID、明确读写掩码 `0x0012019b`、protected DACL 和接管后的 Qt 4100 字节读缓存断言通过；重连期间保留首实例，额外同名首实例创建被拒绝 |
| 首包拒绝 | 错误数值／字符串版本、实例、未知／缺失字段、nonce／pair ID 类型、名称控制字符／长度、未认证业务请求、连续额外数据、超大长度、非法 JSON／UTF-8 与 64 KiB 流量请求通过拒绝用例；失败不分发业务 |
| 非延长期限 | 静默、持续 200 ms 碎片、截断头、有效首包后等待均在固定 3000 ms 期限关闭（测试允许调度容差）；没有通过每收到一字节重置计时器 |
| 关闭及回调 | 杀死自有客户端、首包回调内停止、关闭后立即重启、旧清理回调不影响新实例、新连接继续成功；最终没有遗留自有测试进程 |

发现并修复两个明确问题：首次编译因新 Windows ACL 头定义 `small` 宏与旧测试变量同名，已只重命名该测试变量；初始 Qt6 54 通过，但 Qt5 异常退出后重连失败（53 通过、1 失败）。根因为 Qt5 在释放管道之前发出 `disconnected`，立即重新接受连接会被旧 socket 后续清理破坏。现等待旧 socket 完整析构后的排队回调再重新接受连接，并将清理回调绑定启用实例，避免关闭／重启交错影响新实例。依据 [Qt5.15.2 源码](https://raw.githubusercontent.com/qt/qtbase/v5.15.2/src/network/socket/qlocalsocket_win.cpp) 与本机 Qt6.8.3 源码；复用原生实例遵循 [ConnectNamedPipe](https://learn.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-connectnamedpipe)／[DisconnectNamedPipe](https://learn.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-disconnectnamedpipe) 生命周期。失败报告保留于各目录 `*_before_qt5_cleanup_fix.*`，首次宏冲突日志为 Qt6 的 `build_listener_before_rpc_name_fix.log`。

定向分析：`qkm_local_listener.cpp` 和受影响维护测试各执行 Clang-Tidy／Clazy（LLVM 22.1.8、Clazy 1.16、原有规则），最终全部 native exit 0、项目诊断 0，记录在 `out/tests/mcp_ipc/static-analysis-listener-final/`。初始测试中通过 `QVERIFY(pointer)` 后的解引用被分析器报告潜在空指针，改为显式空分支 `QFAIL`，保留实际失败断言，不添加规则豁免；初始记录在 `static-analysis-listener/`。工具库存断言从三个改为四个真实源文件，报告门禁回归通过：`static_analysis_listener_regression.log`，明细 `out/tests/static_analysis/7c8cbbdff4204bec8352617011e45dd1/`；其中的故意非零／诊断不算真实 C++ 分析失败。

本切片不创建窗口、不占用 Default／焦点或键鼠、不加载个人配置、不触发驱动。未执行产品构建、ASan、Win11、真实异用户／会话／远程客户端、普通桥接到管理员产品、请求线程 Token／模拟恢复、配对／证明／撤销、MCP SDK、GUI 等价性或性能验收。`handshakeReceived` 仅表示首包和进程主 Token 初检，不能记为身份认证成功或可用 MCP 能力；后续桥接还须对接受连接准备期间的忙／断线进行有界重试。

## IPC-20261022-12：M1.1c-1 隔离线程的请求安全上下文检查

- 基线：`0deeea86` 加 `qkm_local_identity.*`、维护测试和本切片说明。组件仍只由维护测试编译；产品工程、GUI、Worker、驱动、翻译和个人配置未改。共用 Token 字段提取从原进程查询中局部抽出，原进程身份及 listener 回归继续在同一目标执行。
- `PipeRequestVerifier` 在读取消息之后，复制管道 HANDLE，使用 `_beginthreadex` 的独立短线程。核对真实对端 PID／创建时间／主 Token，确认该工作线程原本没有模拟身份；调用 `ImpersonateNamedPipeClient`，仅查询请求 Token 的用户 SID、logon SID、AuthenticationId、SessionId；全部读取路径由作用域守卫执行 `RevertToSelf`，正常返回前再次确认没有线程 Token、实际对端未变。Token 提取先清空原 logon SID，不能把缺失请求组信息误当成进程旧值。
- 工作线程不运行 QObject、事件循环、信号或业务回调；失败返回空身份。恢复失败返回 `ContextRestoreFailed`，不继续查询或执行应用动作；结果只有在 Windows 线程退出后才发布。线程隔离旨在防止异常恢复影响 GUI 线程，并不将恢复 API 自身失败写成已测；这是依据 Windows 线程身份范围作出的实现选择，集成时该错误还须关闭认证入口，不能忽略错误继续。
- 依据：[ImpersonateNamedPipeClient](https://learn.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-impersonatenamedpipeclient) 读取最近已读消息的安全上下文；[RevertToSelf](https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-reverttoself) 的失败不得继续以客户端身份工作；[线程句柄生命周期](https://learn.microsoft.com/en-us/windows/win32/procthread/creating-threads)。析构仅注销 notifier／关闭线程 HANDLE，工作线程通过独立共享数据保留复制管道，结束时自行释放，不等待／强杀线程，不访问已销毁 QObject。

| 用例 | 实测 |
|---|---|
| 普通独立进程的请求 Token | 每套 Qt 三次复用检查均通过；结果与真实客户端进程一致，回调在调用 Qt 线程，调用线程没有获得客户端 Token |
| 匿名 SQOS | 对端主 Token 仍可查询，但请求 Token 不可识别时返回 `RequestUnavailable` 和空身份；读取失败后的恢复检查通过 |
| 过期进程／会话预期 | 分别改变创建时间、SessionId，明确拒绝；没有实际创建异登录会话或另一个用户 |
| 无效／重复调用 | 空／无效管道、空预期身份不启动；每对象已有检查时不启动第二个；完成后可复用 |
| 提前销毁 | 32 次启动后立即析构检查对象，客户端退出、listener 停止后，等待原管道不再存在；无晚到回调，无遗留管道引用或子进程 |

Qt6.8.3／Qt5.15.2 x64 console Release／asInvoker，原 EXE 路径不变，已重新 qmake 和编译新增 meta-object 输入。最终构建均 native exit 0、零编译／链接警告；两套维护测试 **各 59 通过、0 失败、0 跳过**，退出 0、stderr 空；Qt6 18,035 ms、Qt5 19,056 ms。证据分别为 `out/tests/mcp_ipc/{qt6-release,qt5-release}/{configure_request.log,build_request.log,results_request.txt,test_request.log}`。40 秒外部看门狗。

实际定向 Clang-Tidy／Clazy 对受影响 `qkm_local_identity.cpp` 和测试执行，全部 native exit 0、项目诊断 0，见 `out/tests/mcp_ipc/static-analysis-request/`；新增代码没有增加源文件／目标数量，前一切片的工具库存及报告回归未变化，复用 IPC-11 的维护回归证据。最终运行后仅补身份头文件的线程使用约束注释和文档，不改变有效测试输入。

边界：组件尚未在 listener 内强制调用；下一步须将它纳入认证状态机，限制整个 listener 的在途检查数量和期限，处理停止／断开／重启时尚未完成的检查及过期回调，再接配对证明。没有注入 `RevertToSelf` 失败、内存／线程创建失败或检查长期不返回；这些分支只完成安全路径审查，不能作为已运行恢复验收。未执行产品／ASan／Win11／管理员连接、配对、MCP SDK、业务或 GUI／MCP 等价性测试；不创建窗口，不操作键鼠、驱动或 Default 焦点。

## IPC-20261022-13：M1.1c-2 请求 Token 门禁、单在途与故障恢复

- 基线 `f9a47bb0` 加 listener／identity、维护测试／工程、双配置分析登记及文档变化。产品工程／GUI／Worker／驱动／翻译仍未改；PC-W10，全部非提权、无窗口／Default／真实输入。
- listener 的首包语法和真实进程初检通过后，必须完成请求 Token 检查才发出握手入口信号；匿名 SQOS 现在由 listener 拒绝。每个 listener 生命周期只保留一个 verifier；断开后等待 socket 完整析构及原生检查退出才重新接受连接。关闭接口立即释放 Qt／原生监听资源，不等待／强杀检查；存续检查期间 `start()` 返回失败。检查长期不退出而达到原绝对期限时关闭整个端点，恢复异常也关闭端点，迟到结果不会自动重新启用或发出握手信号。
- 故障控制仅在独立 `CONFIG+=mcp_fault_injection` 构建定义 `QKM_AUTOMATION_TESTING` 时存在，普通 Qt6／Qt5 编译不含钩子。测试门事件由检查独立复制；测试结束释放门，模拟恢复失败时故意不执行恢复，验证识别级模拟身份随原生线程退出消失，Qt 调用线程未被模拟。没有提供 IPC 参数或产品配置去启用这些钩子。

| 配置／EXE | 构建 | 最终运行 |
|---|---|---|
| Qt6.8.3 普通分支，`out/tests/mcp_ipc/qt6-release/release/QKeyMapperMcpIpcTest.exe` | qmake／JOM 退出 0，零警告／错误 | 59 通过、0 失败／跳过，19,732 ms |
| Qt5.15.2 普通分支，`out/tests/mcp_ipc/qt5-release/release/QKeyMapperMcpIpcTest.exe` | 同上 | 59 通过、0 失败／跳过，20,618 ms |
| Qt6.8.3 故障分支，`out/tests/mcp_ipc/qt6-faults/release/QKeyMapperMcpIpcTest.exe` | 同上，独立构建，测试宏显式开启 | 64 通过、0 失败／跳过，27,047 ms |

三个最终进程均 exit 0、stderr 空，x64 console Release／asInvoker，Community MSVC 14.44.35207；外部看门狗 50 秒。原始 `configure_context.log`、`build_context.log`、`results_context.txt`、`test_context.log` 在各自构建目录，不随 Git 同步。

故障用例独立断言：恢复失败组件返回 `ContextRestoreFailed` 和空身份；listener 恢复失败关闭端点、不发送握手；工作线程被门事件保持时确有一个原生线程，关闭在 1 秒容差内返回且不能重启；连接仍在／客户端已异常退出两种挂起状态均到原期限关闭接口；释放门后旧结果不会通过，显式再次启用才产生新实例；普通新客户端可再次通过身份门禁。没有用“未发生崩溃”替代这些状态与回调断言。

分析工具分别读取普通／故障两个真实 qmake 目标，生成各自 `.moc` 和编译参数；维护回归断言每个目标四个源文件及测试宏只出现在故障配置。回归通过，`out/tests/mcp_ipc/static_analysis_context_regression.log`，明细 `out/tests/static_analysis/c2c30dba225a4ef8987cc3babb40bb03/`。实际定向分析三个改动 C++ 文件、两个配置共六次目标编译／十二次 Clang-Tidy／Clazy，全部 native exit 0、项目诊断 0；`out/tests/mcp_ipc/static-analysis-context/`。未重跑无关产品分析。

故障注入验证指定分支，未制造真实系统 `RevertToSelf`／线程分配／内存分配故障，不能称为全部 OS 异常验收。仍未接配对、挑战证明、撤销、GUI 权限、MCP SDK 或业务；未运行产品、ASan、Win11、高低权限连接、真实异用户／会话和 GUI／MCP 等价性。当前信号只代表首包语法及 OS 身份门禁，不能进入业务权限。

## IPC-20261022-14：M1.1c-3 双向证明基础组件

- 基线 `67aaf9fc` 加 `qkm_auth_proof.*`、listener 的共用十六进制／版本常量、维护工程／测试、分析库存断言和记录。新组件仍未链接产品；没有配对、`ready`、凭据收发或业务授权。
- 随机源使用 Windows [BCryptGenRandom](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom) 的系统首选提供者并检查返回值，失败不使用弱随机后备；没有将 Worker 的普通随机逻辑改作秘密来源。HMAC 使用现有 [Qt QMessageAuthenticationCode](https://doc.qt.io/qt-6/qmessageauthenticationcode.html) 的 SHA256；本机 Qt5.15.2 头文件与两套实际构建确认共用 API。算法背景见 [RFC 4231](https://www.rfc-editor.org/rfc/rfc4231)。
- 固定证明输入为 153 字节，精确格式见[维护测试说明](../../QKeyMapper/tests/mcp_ipc/README.md#挑战证明-v1)。绑定版本、实例、pair、非零授权代次、双方实际 PID／创建时间、两个 32 字节 nonce 与用途；不使用 JSON 成员顺序或浮点数。server／client／pairing confirmation 分别使用 1／2／3，Invalid=0 明确拒绝。
- 独立 Python `struct.pack('>I', version)`、原始 ID 字节、`struct.pack('>QIQIQ', epoch, clientPid, clientCreated, serverPid, serverCreated)` 与 `hmac.new(key, transcript, hashlib.sha256)` 计算三个完整输入／MAC；C++ 固定断言这些字节及摘要，样例包括最大 uint64 和高位 PID。逐项改变绑定、方向、key、nonce 后拒绝；逐个改变 32 个 MAC 位置均拒绝。不能把两端同一个实现自比当成通过。
- 一次性挑战不可复制；生成新 nonce 后，成功／失败都会消耗一次尝试，反射 server 证明、旧 nonce、重放、取消和无效重建均拒绝。私有预期证明清零后释放，不长存 key；Qt HMAC 内部和调用方的临时副本未作全内存残留保证。固定长度比较无按首个差异早退，未进行时序／侧信道测试。

| 配置 | 最终构建 | 最终运行 |
|---|---|---|
| Qt6.8.3，普通 | qmake／JOM exit 0，零 warning／error | 66 通过、0 失败／跳过，19,954 ms |
| Qt5.15.2，普通 | 同上 | 66 通过、0 失败／跳过，20,530 ms |
| Qt6.8.3，故障 | 同上 | 71 通过、0 失败／跳过，27,089 ms；额外验证随机源失败不产生秘密／待证明状态 |

EXE 沿用 `out/tests/mcp_ipc/{qt6-release,qt5-release,qt6-faults}/release/QKeyMapperMcpIpcTest.exe`，x64 console Release／asInvoker、Community MSVC 14.44.35207。三个进程均 exit 0、stderr 空，外部看门狗 50 秒；当前 `build_proof.log`、`results_proof.txt`、`test_proof.log` 留在各目录，不随 Git 同步。

初始定向分析在测试故意把整数 0 转成未命名 ProofRole 时报告枚举越界。已命名 `Invalid=0` 并继续断言拒绝，没有删除负例或加规则豁免；全部相关配置重新构建并运行。两个不变的 listener 逐配置分析沿用初始 native exit 0／无诊断结果；证明组件和测试在两个配置重跑，四次目标编译／八次工具运行均 native exit 0、项目诊断 0。原分析 `static-analysis-proof/`，最终 `static-analysis-proof-final/`，均在 `out/tests/mcp_ipc/`。

工具库存回归更新为普通／故障目标各五个源文件，验证实际参数和测试宏隔离，通过记录 `static_analysis_proof_regression.log`，明细 `out/tests/static_analysis/587cc59b967a4e6aa3b5b91d5a844ce7/`。后续仅枚举命名及负例表达变化，目标及脚本不变，未重复回归。

未运行产品、ASan、Win11、完整握手／配对／持久化／撤销、高低权限或 GUI／MCP 等价性。无窗口、真实输入、驱动、个人配置或 Default 操作。本组件证明格式和验证行为，不表示正式连接已认证。

## IPC-20261022-15：M1.1c-4a 配对凭据存储

- 基线 `f5f74284` 加 `qkm_pair_store.*`、共用 client name 校验、维护工程／测试／库存和文档。尚未链接产品、GUI 或握手控制器。
- 类型化记录保护 pair ID、client name、非零 uint64 授权代次、已知权限位与 32 字节 secret；最多 32 条、ID 不重复、Read 必需。64 位值为规范十进制字符串。当前权限只是类型定义，不声明产品执行支持。
- 全部记录一起装入有界帧，附魔数／版本及 SHA256 后调用当前用户 DPAPI，使用 UI_FORBIDDEN、不使用 LOCAL_MACHINE。附加上下文绑定固定用途、本机 64 位视图 MachineGuid 和应用绑定。读取检查原生结果、封套完整性、帧、字段类型、权限及全部记录后才返回；错误时输出为空。
- 最终目录与文件 owner 为实际当前用户 SID、DACL 仅该 SID 的 FA；目录 protected／可继承，文件替换前显式设置 protected DACL 与 owner。读取通过原生句柄拒绝最终 reparse point／目录／多硬链接，并检查 ACL、owner 与大小；持有目录句柄阻止该目录在操作中被重命名。只读目标保存失败、错误上下文／损坏既有文件都保留原数据，正常保存不静默覆盖这些异常。使用 QSaveFile 禁用 direct-write fallback，不另写自制原子替换框架。
- 初始默认沙箱的五个 store 用例失败在 DPAPI，独立 Python ctypes 的最小 CryptProtectData（有／无附加 entropy）均返回 Win32 2、输出为空。未因此降级保护或将产品判错。同一 asInvoker 测试获准在沙箱外运行，明确查询 TokenElevation=0，五个 store 用例／独立子进程读盘均通过；没有 UAC／gsudo。仅这些无界面运行及随后完整运行在该上下文，源编辑／构建／分析始终非提权且留在原沙箱。记录 `qt6-release/store_smoke.txt`（默认沙箱失败）和 `store_smoke_user.txt`（普通用户通过）。不推断失败对应哪个未观测到的用户配置文件。

| 配置 | 构建／运行上下文 | 最终运行 |
|---|---|---|
| Qt6.8.3 普通 | 构建在原沙箱；运行在获准沙箱外、TokenElevation=0 | 71 通过，0 失败／跳过，20,091 ms |
| Qt5.15.2 普通 | 同上 | 71 通过，0 失败／跳过，20,489 ms |
| Qt6.8.3 故障 | 同上；测试宏只在故障配置 | 77 通过，0 失败／跳过，27,876 ms |

均为 x64 console Release／asInvoker、Community MSVC 14.44.35207，qmake／JOM exit 0、零编译／链接警告，三个运行 exit 0／stderr 空；外部看门狗 60 秒。EXE 在既有 `out/tests/mcp_ipc/{qt6-release,qt5-release,qt6-faults}/release/`。各自 `configure_store.log`、`build_store.log`、`results_store.txt`、`test_store.log` 是本机原始记录，最后一个记录未提权标志；没有把默认沙箱运行算作通过。

当前用例：不创建未使用目录、有效记录与最大 uint64、文件不含明文 key／标签／pair ID、新对象及独立子进程读取、空集持久化；非法 ID／名称／secret／权限／代次、重复或过多记录拒绝且文件不变；错误应用上下文、零长／损坏／截断／过大文件拒绝且不能正常覆盖；只读目标保存失败后原记录仍可读，再解除只读可成功清空；不安全目录和实际 NTFS 硬链接拒绝。

专用故障配置以测试专用 DPAPI 包装器构造“有效加密但非法内容”：未知格式／字段、JSON 数字或非规范／溢出 uint64、越界／非整数权限、错误 key 长度、重复 pair 和错误内层校验和，全部拒绝且不返回部分凭据。该入口不编译进普通配置。真实密钥不进入日志／命令行；固定 dummy key 属于公开测试样例，子进程命令只含测试路径与非秘密应用绑定。

四个改动 C++ 文件、普通／故障两配置共八次目标编译／十六次 Clang-Tidy／Clazy，全部 native exit 0、项目诊断 0：`out/tests/mcp_ipc/static-analysis-store/`。工具库存断言每目标六个源码，报告回归通过：`static_analysis_store_regression.log`，明细 `out/tests/static_analysis/93a24c3851234507866403f090b16573/`。新 helper 已由真实 qmake 参数覆盖，不冒充产品目标。

依据：[DPAPI 保护](https://learn.microsoft.com/en-us/windows/win32/api/dpapi/nf-dpapi-cryptprotectdata)、[DPAPI 解密及完整性注意](https://learn.microsoft.com/en-us/windows/win32/api/dpapi/nf-dpapi-cryptunprotectdata)；本机 Qt6 的 QFileDevice::handle 文档及 QFSFileEnginePrivate::nativeHandle 确认 Win32 句柄须由 CRT descriptor 经 `_get_osfhandle` 取得，Qt5／Qt6 均已实际编译运行验证。没有更改全局 ACL、用户个人设置或系统驱动。

边界：Qt JSON／HMAC／调用方可能保留短期内存副本，只清零本组件自有明文与 DPAPI 解密缓冲，不承诺全进程零残留。没有真实异用户／跨机器拒绝、管理员产品／普通桥接、真实资源耗尽、产品、ASan、GUI／MCP 等价性或性能验收。没有创建窗口／占焦点或发送输入；凭据还未接入 GUI 批准、正式证明消息、授权或撤销控制器，不能记为可用 MCP 能力。

## IPC-20261022-16：M1.1c-4b 配对、认证与权限会话

基线 `f52cad39` 加 `qkm_session_server.*`、listener 分阶段收发、证明用途选择及维护测试；未链接产品。默认关闭，批准只接受当前连接的 GUI 请求 ID；客户端批准布尔值、未知方法、错误／反射证明不能取得业务入口。用途 3 配对回证且 DPAPI 保存完成后才 ready；重连用用途 2 新挑战。权限更新递增代次／撤销先保存再断开，失败关闭接口并报告未保存。

每帧核对真实对端／请求 Token；认证前 4 KiB、3 秒，GUI 批准等待 60 秒；认证后 256 KiB、空闲无固定断开、首字节起 10 秒期限；输出最多两个最大帧，一个在途请求，回包绑定连接／请求 ID。无产品配置修改、映射执行或业务去重保证。

| 配置 | 最终构建／运行 | 结果 |
|---|---|---|
| Qt6.8.3 普通 | Community MSVC 14.44.35207，x64 Release／asInvoker | 80 通过，0 失败／跳过，38,740 ms |
| Qt5.15.2 普通 | 同上 | 80 通过，0 失败／跳过，39,310 ms |
| Qt6.8.3 故障 | 同上，仅故障目标含测试钩子 | 86 通过，0 失败／跳过，45,813 ms |

三版 qmake／JOM exit 0、编译／链接零警告；运行在获准沙箱外且 TokenElevation=0，exit 0、stdout／stderr 空。EXE 在 `out/tests/mcp_ipc/{qt6-release,qt5-release,qt6-faults}/release/QKeyMapperMcpIpcTest.exe`；各目录 `build_session.log`、`results_session_final.txt`、`test_session_final.log` 为本机原始证据，不随 Git 同步。

独立进程覆盖配对／重连、只读拒绝 apply、授权代次更新、撤销活跃连接和旧 key、谎报批准、GUI 拒绝、错误／缺失确认、迟到批准不能作用于新连接、保存失败无 ready、8 KiB 双向收发、认证空闲超过 3 秒仍可用、实际 10 秒慢帧，以及处理器禁用／deleteLater 控制器。key 不进 CLI／日志；撤销测试等待客户端已收业务回包标志，避免把调度延迟当成断线通过。

定向分析证明、listener、session server、测试四个 cpp，普通／故障共八次目标编译、十六次工具调用 native exit 0。初次测试有两处 Clazy detaching-temporary，改 `.first()` 为 `.at(0)` 后仅重跑已改测试两配置，最终项目诊断 0；未变三组件沿用本次逐文件零诊断。原报告 `out/tests/mcp_ipc/static-analysis-session/`，最终 `static-analysis-session-final-test/`。库存回归确认两真实 qmake 目标各七个 cpp，通过 `static_analysis_session_regression.log`，明细 `out/tests/static_analysis/15193f9c3f0a41f6875c904720bb3c1c/`。

未验收产品 GUI／正式 SDK、60 秒 GUI 超时、真实高低权限／异用户／Win11、业务等价性、真实输入和性能。无窗口、Default 焦点或驱动操作。

## IPC-20261022-17：会话与原生验证生命周期 ASan

同 IPC-16 源码，维护目标 `CONFIG+=asan CONFIG+=mcp_fault_injection`。Community 14.44 链接 LNK1104，缺 ASan import library，未安装／修改系统工具链；改用已有 BuildTools 14.40.33807，在新 `out/tests/mcp_ipc/qt6-asan-1440/` 重建避免混用对象，qmake／JOM exit 0、零警告。失败日志保留 `qt6-asan/`，不算通过。

确认 EXE 导入 `clang_rt.asan_dynamic-x86_64.dll`，PATH 仅进程内加入对应 kit。实际非提权运行完整故障测试：**86 通过、0 失败／跳过，53,820 ms，exit 0，stderr 空，无 ASan 报告**。证据为该目录 `build_session.log`、`results_session_final.txt`、`test_session_final.log`。包含原生验证滞留／恢复故障、对象销毁、listener 重启、会话关闭／处理器销毁及 DPAPI／ACL 负例。

插桩仅本目标 first-party 组件／测试，Qt DLL 未插桩；`detect_leaks=0`，无泄漏检测结论；OS 恢复故障为受控注入。未覆盖产品 GUI、映射引擎、高低权限或 Win11。构建／编辑／分析未提权，无 UAC 或驱动操作。

## IPC-20261022-18：产品管理窗口与可移植私有桌面验证

基线 `001bf61d` 加 `ControlDialog`／automation.pri、真实进程路径读取、通用高级设置和独立托盘入口、语言／退出清理及现有 editor UI scale 注册。Worker／驱动／TS／产品 manifest 未改。端点仍默认关闭且只在 GUI 明确启用后监听；当前没有产品业务回调，正式 MCP SDK 也尚未连接，不能算普通映射／软件设置能力完成。

管理窗口默认读取权限，配置及启停须另选；所有已存权限位均展示。程序路径由真实 PID／创建时间核实并用只读单行框显示；自报 client name 明文显示，不解释富文本。Enter 不默认批准／撤销。关闭窗口保留已经启用的接口；待批准请求在拒绝、X 关闭或 Escape 时拒绝，统一使用 QDialog 的 done 边界，避免 Escape 绕过 closeEvent。

| 目标／检查 | 当前结果 |
|---|---|
| 产品 Qt6.8.3 x64 Release | qmake／JOM exit 0，零编译／链接警告；EXE `out/build_qt6/release/QKeyMapper.exe` |
| 产品 Qt5.15.2 x64 Release | exit 0；官方 msvc2019_64 的既定 qtmain.lib(qtmain_win.obj) 缺 PDB LNK4099 例外，其余无警告；EXE `out/build_qt5_5152/release/QKeyMapper.exe` |
| 管理窗口 Qt6 Release／asInvoker | 最终 `final-qt6/results.txt`：11 通过、0 失败／跳过，1,085 ms，无运行告警，native exit 0 |
| 管理窗口 Qt5 Release／asInvoker | `baseline-qt5/results.txt`：11 通过、0 失败／跳过，7,893 ms，无运行告警，native exit 0；此后只改预览元数据分支，普通用例有效输入未变，最终 EXE 另跑了中文预览元数据 |
| 可移植脚本 | 两 Qt 的真实私有桌面运行通过；原生可见性／DPI 与所属 PID 核对，截图后关闭自有窗口。缺参数、output 越出 out、Python -O 三种入口均 exit 2 拒绝 |

产品最终日志 `out/tests/mcp_control/product_qt{6,5}_baseline.log`。维护 EXE 是 `out/tests/mcp_control/{qt6-release,qt5-release}/release/QKeyMapperMcpControlTest.exe`，Community 14.44.35207 编译零警告；产品使用原构建脚本选择的 BuildTools kit。运行在获准沙箱外以使用真实用户 DPAPI，父／子 TokenElevation=0；编辑、构建和分析在原沙箱且未提权，无 UAC／gsudo。

维护目标使用产品同一 ControlDialog，经 Qt 真实控件事件驱动，并由独立 `QKeyMapperMcpIpcTest --session-client` 子进程完成真实管道及证明。断言核对默认不监听／不创建目录、真实 EXE 路径与 PID、只读及附加权限的真实凭据、批准／拒绝、撤销、关闭保留接口、X／Escape 取消待批准请求、失败回显；业务响应是明确测试 fixture，不代替实际 QKeyMapper 业务或正式 MCP 调用。

初次驱动未显示窗口／点击复选框空白中心，未触发完整用户操作；修正为显示窗口、等待暴露、点击 style 提供的实际指示区，并断言按钮效果。Qt5 初始测试还因未采用产品 DPI／字体／Fusion 基线而有几何告警；对照 main.cpp，测试设置相同的高 DPI 启用、PassThrough、Fusion 与 FONTNAME_ENGLISH 9pt 后，最终 Qt5／Qt6 均无运行告警。没有修改产品 DPI／几何逻辑或屏蔽告警；旧记录留在 portable-*／initial_*，不算最终验收。

三语言静态审阅：`out/tests/mcp_control/baseline-qt6/` 和 `baseline-qt5/` 各关闭／待批准六图，共十二图已逐张审阅；文字／按钮／权限控件完整，路径可水平滚动，`<b>` 仍是文字。最终 Qt6 元数据／脚本补充运行与六图在 `final-qt6/`，样式和控件呈现未变；Qt5 中文最终预览在 `mcp_qt5-release_0_pending_fff6c2.*`。pending 图是 UI fixture，未启用接口或批准客户端，不作为安全证明。

实际桌面：最终 Qt6 `QkmMcp_21a01079e6854afb850499b13948c2d9`，Qt5 控件 `QkmMcp_77b14cf5726e4d07b2d93f7ff737b13d`；目标自身报告并与 launcher 比较。元数据中两 Qt 均 Qt DPR 1.5／logical DPI 96、原生 DPI 144、IsWindowVisible=true、Fusion／Microsoft YaHei UI 9pt，详情在本机 JSON。通过 capture_ui_snapshots.ps1 的 PrintWindow 抓图，截图程序也在相同私有桌面核对 HWND／PID 后关闭窗口；未切换输入桌面，无 Default、SendInput、驱动或个人 INI 操作。

定向分析 identity、ControlDialog、维护测试：初始三个文件共九个真实目标编译（main、sendinput-lifecycle、IPC 普通／故障、control），工具 native exit 0，只有测试的模块包含及遗漏 emit 两项诊断；改用 QtTest/QTest 与显式 emit 后修复。后续按实际变化只重跑 ControlDialog 的三个配置和测试配置；最终 `static-analysis-portable/`、`static-analysis-metadata-test/` 均 native exit 0、项目诊断 0，identity 沿用本次初始四配置零诊断。库存／报告回归通过，`static_analysis_regression.log`，明细 `out/tests/static_analysis/30e8d0c123b14d57905a189d03b1ee20/`，确认 control 目标八个 cpp、真实 Widgets 参数且无故障宏。

边界：没有启动正式产品、验证实际入口／主题／动态缩放、管理员产品与普通桥接、60 秒 GUI 批准超时、Win11、真实键鼠、业务等价性或性能；本切片未选 ASan，IPC-17 仅覆盖当时不变的会话／原生验证组件。所有日志／EXE／JSON／截图仅本机 out，不随 Git 同步；可移植测试源码／命令随仓库同步，另一台 PC 需重建并重新验证。

## IPC-20261022-19：私有实例发现、活跃租约与关闭清理

基线 `6acbc7b7` 加 qkm_instance_discovery.*／qkm_private_storage.*、会话启停／GUI 路径接入及维护用例。私有目录、文件与 QSaveFile 临时安全处理从凭据存储原样提取供两者共用；文件打开仅增加显式 DELETE 访问选项供安全清理，原 owner／DACL／reparse／硬链接校验未减。统一 PipeNamePrefix，证明格式及业务无变化。

发现文件为严格七字段、无 BOM UTF-8 JSON，最多 128 KiB；不含 key。整数 PID／创建时间／session 为规范十进制字符串。读取先检查私有目录／文件／大小／完整 schema，再核对实际进程、登录会话、OS 程序路径和调用者预期 EXE 的 volume serial／file index；不打开提示中尚未与 OS 比较的远程路径。提示不授权，实际连接仍须核对管道 peer 并完成新证明。

活跃服务器保留已核实发现文件的只读句柄，拒绝写／删除共享；发布后再核对自身实例／PID，防止并发覆盖后同时开始服务。其他活跃发布者拒绝替换，已退出／PID 复用的提示可替换；损坏／不安全数据不正常覆盖。禁用先关闭端点并释放租约，再校验 ID／自身 PID／创建时间，以带 DELETE 权限的验证句柄清理；旧 ID 不能删新文件，失败保留并报告。

| 检查 | 当前结果 |
|---|---|
| IPC Qt6.8.3 普通 | 82 通过，0 失败／跳过，39,258 ms，exit 0 |
| IPC Qt5.15.2 普通 | 82 通过，0 失败／跳过，39,967 ms，exit 0 |
| IPC Qt6 故障 | 88 通过，0 失败／跳过，46,795 ms，exit 0 |
| GUI Qt6／Qt5 | 各 11 通过，0 失败／跳过，1,350／8,559 ms，exit 0；实际私有桌面 |
| 产品 Qt6／Qt5 | 两套构建 exit 0；Qt6 零 warning／error，Qt5 仅既定官方 qtmain 缺 PDB LNK4099 例外；产品未启动 |
| 定向 ASan | 8 通过，0 失败／跳过，1,771 ms，exit 0、stderr 空、无报告；已确认导入 ASan runtime |

维护普通／故障 EXE 沿用 out/tests/mcp_ipc/{qt6-release,qt5-release,qt6-faults}/release/；构建日志 build_discovery_final.log，运行 results_discovery.txt／test_discovery.log，stdout／stderr 空、TokenElevation=0，Community 14.44.35207。GUI 在 out/tests/mcp_control/{qt6-release,qt5-release}/release/，证据 discovery-qt6／discovery-qt5，实际桌面分别 QkmMcp_173001727fd6490d842e3a943b2ebe40／QkmMcp_27903aca9aa5464f9947ebe48972b04d。产品日志 product_qt{6,5}_discovery.log，实际 EXE 仍在标准 out/build_qt6、out/build_qt5_5152 的 release/。

回归核对发布／读取／无 secret、路径别名及错误预期 EXE、禁用清理／重启新 ID、旧清理不删新文件、同进程第二服务器不能覆盖活跃租约、只读清理／重新发布失败保留；字段类型／额外字段／代次／远程管道／假程序路径、空文件／过大／非法 UTF-8 拒绝且不返回旧 descriptor。独立活跃子进程占用提示时发布 Busy，退出后才替换。凭据原子写盘／私有 ACL／损坏拒绝等原用例全部回归；会话保存失败用例改为启用后放入损坏凭据，断言不覆盖且不 ready，因为启用现在会创建私有发现目录。

七个改动 cpp 共 26 个实际目标编译、52 次 Clang-Tidy／Clazy，native exit 全部 0；初始仅测试 invalid QList 遍历有 range-loop-detach，改为 std::as_const 并包含 utility 后，两测试配置重跑 native exit 0、诊断 0。生产／GUI 其余配置源未变，沿用本轮各文件零诊断；报告 static-analysis-discovery/ 和 static-analysis-discovery-final-test/。全量运行在该测试 const 遍历修正前；修正不改变测试输入／生产路径，定向 ASan 使用修正后的源码。库存／报告回归通过 static_analysis_discovery_regression.log，明细 out/tests/static_analysis/2f297fca94fd4f8db6c17eba7a92339f/，确认普通／故障各九个 cpp、GUI 十个。

ASan 使用既有 BuildTools 14.40.33807 完整 kit，Qt6 故障＋asan，EXE out/tests/mcp_ipc/qt6-asan-1440/release/QKeyMapperMcpIpcTest.exe；configure_discovery.log／build_discovery.log／results_discovery_asan.txt／test_discovery_asan.log。覆盖 discoveryLifecycle、discoveryRejectsMetadata、storeRoundTrip、storeAtomicFailure、storeUnsafePaths、sessionHandlerLifecycle 加 init／cleanup。Qt DLL 未重插桩，detect_leaks=0；不把八项定向结果写成全量 88 项 ASan 通过。

全部运行在获准普通用户上下文、未提权；编辑／构建／分析仍在默认开发沙箱。只读查询确认开发沙箱与交互用户是不同账号，但不把这当成异用户连接拒绝已经实测，不记录账号／SID。交互用户当前 Medium、gsudo 缓存不可用，未启动 UAC；值需下次重新核对。Windows MIC 独立于 DACL，实际管理员产品与普通桥接仍须验收：[Microsoft 文档](https://learn.microsoft.com/en-us/windows/win32/secauthz/mandatory-integrity-control)。本轮未改管道完整性标签。

GUI 只有连接框内容／提示变化，控件布局／字体／三语言呈现沿用 IPC-18 已审阅图；本轮私有桌面断言增加实际发现文件与进程核实，无 Default／真实输入／驱动操作。没有正式 SDK 往返、产品运行、真实高低权限／异用户／Win11、60 秒批准超时、业务／性能结论。关闭状态的配对查看／撤销需在业务开放前补齐。原始材料仅在 out，不随 Git 同步；当前为本地开发检查点，完整 M1 尚未完成。
