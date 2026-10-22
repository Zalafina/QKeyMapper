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
