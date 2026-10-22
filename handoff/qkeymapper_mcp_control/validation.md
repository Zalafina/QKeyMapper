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

## IPC-20261022-20：官方 SDK stdio 桥接与原生跨语言配对

源码基线 `6124b35e` 加本切片同提交变更：mcp_bridge 开发入口、固定依赖、严格协议／Windows 发现及实际 peer、精确 FILETIME 身份、可取消原生管道、客户端 DPAPI／Windows 凭据管理器与官方 MCPServer。C++ 只增加无业务回调时的真实通信能力响应，及对应维护 IPC／GUI 夹具。Worker／驱动／个人 INI／TS／正式 manifest 未变。

正式 SDK 初始化／工具枚举在应用不可用时仍可完成；配对调用立即返回等待状态，真实 GUI 事件批准后才握手并 ready。正式工具只有连接状态、连接、申请配对、断开和能力查询；当前 C++ 明确返回 `stage=transport_only`、空业务能力、当前权限及帧限额，不接受未知参数。其他业务未接时拒绝，不开放通用方法／属性／INI／命令入口。

| 检查 | 本轮结果与证据 |
|---|---|
| Python 协议／原生单测 | 10 通过，0 失败，0.072 s，exit 0；out/mcp_bridge/unit_tests.log；固定独立 MAC 向量含 uint64 最大值、JSON／NUL 往返与错误 Unicode／重复键／深度／非有限值、规范整数、凭据字段、线程冒用拒绝、有界作业／重复关闭、挑战错误身份／schema／角色反射拒绝 |
| 正式 SDK＋Qt6 | 12 场景通过，exit 0；out/mcp_bridge/accepted-qt6.log 与 accepted-qt6/results.json；实际私有桌面 ID 在 JSON／fixture.json，父、GUI 及待批准桥接进程 TokenElevation=0 |
| 正式 SDK＋Qt5 | 同 12 场景通过，exit 0；accepted-qt5.log 与 accepted-qt5/results.json；与 Qt6 相同真实配对及 60 秒场景，独立维护 EXE／私有桌面／凭据目标 |
| IPC Qt6.8.3 普通 | 83 通过，0 失败／跳过，40,385 ms，exit 0 |
| IPC Qt5.15.2 普通 | 83 通过，0 失败／跳过，41,270 ms，exit 0 |
| IPC Qt6 故障 | 89 通过，0 失败／跳过，47,612 ms，exit 0 |
| GUI Qt6／Qt5 原有控件回归 | 各 11 通过，0 失败／跳过，1,402／8,403 ms，exit 0；out/mcp_bridge/gui-qt6-current／gui-qt5-current/results.txt 及 run_private.json |
| 产品 Qt6／Qt5 | 标准 out/build_qt6、out/build_qt5_5152 下 release/QKeyMapper.exe 本轮增量构建 exit 0；确实重新编译 qkm_session_server.cpp；Qt6 无编译／链接警告，Qt5 仅既定官方 qtmain 缺 PDB LNK4099；产品未启动 |
| 定向静态分析 | 三个改动 cpp／八个实际目标编译，16 次 Clang-Tidy／Clazy，所有 native exit 0、项目诊断 0；out/mcp_bridge/static-analysis/summary.txt。覆盖 main、sendinput-lifecycle、IPC 普通／故障和 GUI 的真实 qmake 参数，无新忽略 |
| 依赖／语法 | Python compileall exit 0；隔离 venv pip check 无损坏依赖；完整冻结 requirements-windows.txt，官方 SDK 2.3.0／pywin32 312，实际运行 Python 3.11.9 |

SDK 场景包括：无应用 initialize／list／call；无凭据时要求配对；GUI 批准前不认证且忙时拒绝能力作业；跨语言 Server／Client MAC 与配对确认；保存客户端 key／新 stdio 进程重连；错误 DPAPI 绑定拒绝且原凭据仍可读；撤销后现有会话断开、旧 key 拒绝且不自动重新配对；GUI 拒绝；待批准取消三秒内完成；stdio 退出五秒内清除待批准；错误预期 EXE；私有但创建时间／远程程序路径／NUL／额外字段被篡改的发现信息，均在申请 GUI 配对前拒绝；真正等待 60 秒仍未批准时失败；服务器存储被故意损坏时，客户端虽已保存 key 仍不能 ready，原损坏服务器文件保留、未增加授权。测试最后只删除自身随机凭据目标、关闭并等待自有私有桌面进程退出。

客户端使用 Windows 凭据管理器本机持久化，再用禁止 UI 的当前用户 DPAPI 加用途／机器／显式发现与 EXE 路径／SessionId 绑定。调查发现 pywin32 CredWrite 要求 Unicode，最初传 bytes 的 TypeError 在客户端保存前触发，服务器未 ready；改为仅把 DPAPI 密文十六进制转成文本。正式 SDK v2 的工具枚举／结构化输出也按固定 SDK 源码修正，不以内部函数或探针结果冒充正式调用成功。初次失败原始记录留在 out/mcp_bridge/interop-qt*-first 等目录／日志；最终通过版本是 accepted-qt6／accepted-qt5，不覆盖失败证据。

IPC 普通／故障 EXE 位于 out/tests/mcp_ipc/{qt6-release,qt5-release,qt6-faults}/release/QKeyMapperMcpIpcTest.exe；构建 build_bridge.log，运行 results_bridge.txt／test_bridge.log，stdout／stderr 空且未提权。新增 sessionTransportCapabilities 通过独立 native 子进程验证空业务能力、未知参数拒绝及未接业务拒绝；原生命周期／安全测试全量回归。GUI EXE 为 out/tests/mcp_control/{qt6-release,qt5-release}/release/QKeyMapperMcpControlTest.exe，构建 build_bridge_final.log；维护固定事件／保存失败夹具不进入产品。产品日志 out/mcp_bridge/product_qt6.log／product_qt5.log，环境 Git ignore 的权限提示不是编译器诊断，未更改个人 Git 配置。编译器、Qt、分析工具沿用 IPC-19 环境。

所有编辑／构建／分析保持默认开发沙箱；需要真实用户 DPAPI 的运行在已授权普通用户上下文且未提权。没有切换输入桌面、Default 焦点、系统键鼠、驱动、个人配置或已有用户凭据操作。UI 本轮无产品布局变化，三语言静态图复用 IPC-18；此项复用不是本轮截图。当前未增加 ASan，IPC-19 仅可用于不变的原生基础；不把它记为新增 Python 桥接的 ASan 验收。

未验证真正产品启动及高级设置／托盘入口、管理员产品到普通桥接／对象 MIC、实际异用户、Win11、打包、性能、映射／设置业务与 GUI／MCP 等价性。M1.1d 只形成通信组件的本地开发检查点，完整 M1 未完成；关闭时配对管理仍须在业务开放前补齐。原始材料均在忽略的 out，本轮随 Git 同步桥接源码、依赖锁、维护夹具及命令；接收 PC 须按本机环境重建并重新配对。

## IPC-20261022-21：关闭状态配对管理与目录独占租约

PC-W10，源码基线 `af1fd2ea` 加本记录所在提交：共享私有存储／会话／管理窗口及对应维护测试。桥接生产源码、Worker／驱动／个人 INI／TS／正式 manifest 未改。选定切片 M1.1c-4e-a 完成，完整 M1 及产品业务未完成。

接口关闭时打开管理窗口只加载 DPAPI 配对元数据，临时 key 清除，不创建目录／端点。撤销先取得私有目录和 share=0 的 control.lock 租约，再读取最新记录并原子保存；当前版本活跃服务器从加载到关闭清理全程持有同一租约，另一实例占用时返回 Busy，避免已撤销记录仍被活跃内存授权使用。根目录及锁文件不可替换，锁文件不写载荷、拒绝多硬链接，OS 在进程退出时释放；不承诺与未实现租约的旧原型兼容。

| 检查 | 本轮结果与证据 |
|---|---|
| IPC Qt6.8.3 普通 | 87 通过，0 失败／跳过，40,463 ms，native exit 0 |
| IPC Qt5.15.2 普通 | 87 通过，0 失败／跳过，41,990 ms，native exit 0 |
| IPC Qt6 故障 | 93 通过，0 失败／跳过，48,130 ms，native exit 0 |
| 定向 Qt6 ASan＋故障 | 12 通过，0 失败／跳过，8,858 ms，exit 0、stderr 空，无 ASan 报告；实际导入 clang_rt.asan_dynamic-x86_64.dll |
| GUI Qt6／Qt5 | 各 16 通过，0 失败／跳过，2,022／19,920 ms，exit 0；out/mcp_bridge/offline-visual-qt6／offline-visual-qt5/results.txt、run_private.json |
| 正式 SDK Qt6／Qt5 | 各 12 个短场景通过，exit 0；out/mcp_bridge/offline-sdk-qt6.log／offline-sdk-qt5.log 和各同名目录 results.json；新增关闭接口 GUI 撤销、重新启用旧 key 拒绝及不自动配对 |
| 静态界面 | 每 kit 捕获三语言四状态共 12 张；本轮自主审阅新增 paired-off／busy-off 共 12 张，无截断／重叠、内容与按钮可读。原 off／pending 状态也重新捕获，其视觉结论复用 IPC-18，未声称本轮逐张审阅 |
| 产品 Qt6／Qt5 | out/build_qt6、out/build_qt5_5152 下 release/QKeyMapper.exe 本轮增量构建 exit 0；out/mcp_bridge/product_qt6_offline.log／product_qt5_offline.log；Qt6 无编译／链接警告，Qt5 仅既定官方 qtmain 缺 PDB LNK4099；产品未启动 |
| 定向静态分析 | 五个改动 cpp／16 个实际目标编译，32 次 Clang-Tidy／Clazy，全部 native exit 0、项目诊断 0；out/mcp_bridge/static-analysis-offline/summary.txt，未新增忽略 |

IPC EXE 为 out/tests/mcp_ipc/{qt6-release,qt5-release,qt6-faults}/release/QKeyMapperMcpIpcTest.exe；各 build_offline_final.log、results_offline.txt、test_offline.log 和 out/mcp_bridge/offline_cpp.log。新用例 directoryLease／directoryLeaseProcessExit／sessionOfflineManagement／sessionOfflineFailure 覆盖独占及根目录保护、子进程被结束后 OS 释放、无目录查看、不监听／不发布提示、活跃占用拒绝且原会话仍有效、关闭撤销／重启旧 key 拒绝、只读／损坏保存失败保留。初次子进程 stdout 探针因 Windows 文本模式 CRLF 与预期 LF 不同失败，改为比较 trimmed 内容后全量普通／故障及 ASan 通过；失败原件 results_offline_probe.txt／test_offline_probe.log、out/mcp_bridge/offline_probe.log 保留，不将其列为通过。

ASan EXE 为 out/tests/mcp_ipc/qt6-asan-1440/release/QKeyMapperMcpIpcTest.exe；MSVC BuildTools 14.40.33807，build_offline_final.log／results_offline_asan.txt／test_offline_asan.log 及 out/mcp_bridge/offline_asan.log。十个方法为四项新增、discoveryLifecycle、storeRoundTrip、storeAtomicFailure、storeUnsafePaths、sessionHandlerLifecycle、sessionPairAndReconnect，另含 init／cleanup。Qt DLL 未重插桩、detect_leaks=0；不把 12 项定向结果当作 93 项全量 ASan。

GUI EXE 为 out/tests/mcp_control/{qt6-release,qt5-release}/release/QKeyMapperMcpControlTest.exe，各 build_offline_final.log；新用例通过 Qt 真实控件事件验证关闭状态三语言查看／撤销、实际 DPAPI 结果、重新打开为空、无端点和提示；只读保存失败及另一活跃实例占用显示失败，条件解除后成功并清除错误。普通构建／运行无编译警告，父／子 TokenElevation=0。最终截图运行实际私有桌面 Qt6 为 QkmMcp_77eeb1d465b04056927c2ce467be6acb，Qt5 为 QkmMcp_7d1a6c65542d4076b9d8a01f0b0633d9。使用 capture_ui_snapshots.ps1／PrintWindow，Fusion、Microsoft YaHei UI 9 pt、原生 DPI 144、Qt DPR 1.5；静态预览始终接口关闭，假记录不能证明认证。截图、状态及捕获日志仅在上述 offline-visual 目录。

SDK 使用正式 mcp 2.3.0 Client／stdio／固定五项工具，桥接生产源码未改；Qt6 实际私有桌面 QkmSdk_4949af6ea0de42a1a8a69d06d53b60d6，Qt5 为 QkmSdk_131939cae95a4539a44e3e37c0d1d3ed。父、GUI 及待批准桥接均 TokenElevation=0，只清理自有随机凭据及夹具。本轮省略 --approval-timeout，12 短场景不含实际 60 秒等待；计时未改，复用 IPC-20 两套 Qt 实际 60 秒通过证据。Python 单测 10 与依赖检查同样沿用 IPC-20，不写为本轮重跑。

编辑／Git／构建／分析均非提权；需真实用户 DPAPI 的运行仍使用已授权沙箱外普通用户。未切换输入桌面、占用 Default 焦点、发送系统键鼠或安装驱动。用户返回后已批准仅测试启动／关闭的 gsudo 管理员维护验收，随后要求为开启 fast 在当前检查点暂停；**本轮尚未启动任何管理员进程**，不把批准当作通过。恢复后无需重复请求同范围批准，UAC 仍可能需要用户确认。

实际产品高级设置／托盘入口、管理员产品到普通桥接／MIC、真实异用户、Win11、打包／性能、业务操作及 GUI／MCP 等价性均未验证。当前形成本地开发检查点并按用户指令提交、不推送；原始证据不随 Git 同步，精简记录及可移植测试随源码同步。

## IPC-20261022-22：同用户 UAC 关联与管理员维护服务互操作

PC-W10，源码 `199b5e37` 加本记录所在提交。实际用户已批准所需 UAC，只有固定启动助手及自有维护服务提权；控制器、官方 SDK／stdio 桥接、源码／Git／构建／分析仍未提权。未改正式 manifest、协议／文件格式、管道 DACL／MIC、Worker／驱动或个人 INI。

首次高权限实测 out/mcp_bridge/elevated-first-qt6.log 在发现核验阶段返回 identity_rejected：双方为同用户、同 logon SID、同 SessionId，原始 AuthenticationId 不同。普通用户只读系统 TokenLinkedToken 探针确认 full／limited 互相关联。依据 [TOKEN_LINKED_TOKEN](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-token_linked_token)、[提升类型](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ne-winnt-token_elevation_type) 及 [GetTokenInformation](https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-gettokeninformation)，双端只增加 OS 核实的关联 AuthenticationId；须同用户／logon SID／SessionId、相反 full／limited 类型且双方关联互指。缺失／查询失败不降级为仅比较 SID／SessionId，原始 ID 保留。sameProcess 仍严格比较原 AuthenticationId；实际请求 Token 读取先清除关联字段，不能从主 Token 继承或经 UAC 关联绕过请求上下文检查。关联信息不由 IPC 输入提供。

| 检查 | 当前结果及原始索引 |
|---|---|
| SDK High 服务／Medium 桥接 Qt6 | 13 场景通过，native exit 0；out/mcp_bridge/accepted-high-qt6.log、同名目录 results.json／topology.json／fixture.log／launch.log |
| SDK High 服务／Medium 桥接 Qt5 | 同 13 场景通过，native exit 0；accepted-high-qt5.log 及同名目录；两套均真正等待 60 秒未批准并拒绝 |
| SDK 全普通权限 Qt6／Qt5 | 各 12 短场景通过，exit 0；accepted-linked-medium-qt6.log／accepted-linked-medium-qt5.log 与同名目录；本轮普通模式不含 60 秒等待 |
| Python 单测 | 12 通过，0 失败，0.097 s，exit 0；out/mcp_bridge/unit_linked_final.log；新增互指／单边／错误用户／logon／SessionId 拒绝和关联查询失败无关联结果 |
| IPC Qt6／Qt5 普通 | 各 88 通过，0 失败／跳过，40,433／42,305 ms，exit 0；新增 linkedLogonSession 证明关联只允许同登录，不允许变成同进程 |
| IPC Qt6 故障 | 94 通过，0 失败／跳过，48,360 ms，exit 0；逐帧请求／冒用恢复及既有故障回归 |
| GUI Qt6／Qt5 | 各 16 通过，0 失败／跳过，1,773／18,816 ms，exit 0；linked-gui-qt6／linked-gui-qt5/results.txt、run_private.json |
| 定向 ASan＋故障 Qt6 | 13 通过，0 失败／跳过，10,074 ms，exit 0、stderr 空，无 ASan 日志；out/mcp_bridge/linked_asan.log 及下述实际 EXE／原始文件 |
| 产品构建 Qt6／Qt5 | 两套 native exit 0，Qt6 无编译／链接警告，Qt5 仅既定官方 qtmain 缺 PDB LNK4099；out/mcp_bridge/product_qt6_linked.log／product_qt5_linked.log；正式产品未启动 |
| 定向静态分析 | 两个 cpp、七个实际目标编译、14 次 Clang-Tidy／Clazy，全部 native exit 0、项目诊断 0；out/mcp_bridge/static-analysis-linked/summary.txt，无新增忽略 |

SDK actual High 服务完整性 RID 12288／TokenElevation=1，控制器为 Medium 8192／0；每次待批准桥接也检查 TokenElevation=0。两端原 AuthenticationId 不同、系统关联互指、same_login=true，身份摘要只记录这些布尔值，不存账号／SID／登录 ID。实际私有桌面 High Qt6 为 QkmSdk_357f9db78235423bb0811fe70d985ed8，High Qt5 为 QkmSdk_993452d26c224c5d819db9bb36bfc5d9；普通 Qt6／Qt5 为 QkmSdk_65addfc3f0754ac485c21fc1495b7d16／QkmSdk_bf60ada823094cbf85361cb07a6a9540。High 目标仍为同源维护 EXE out/tests/mcp_control/{qt6-release,qt5-release}/release/QKeyMapperMcpControlTest.exe，不能当作真正 QKeyMapper.exe 验收。

高权限场景通过正式 SDK initialize／list／固定五工具：批准前不 ready、只读批准、跨语言双向证明、服务器高权限 DPAPI／私有存储保存、真实能力 RPC、新 stdio 凭据重连、启用／关闭撤销和旧 key 拒绝、GUI 拒绝、取消、stdio 退出、错误预期 EXE、篡改私有发现、实际 60 秒超时以及客户端已存 key 但服务器保存失败不 ready。未给桥接或测试断言提权，也未放宽对象安全标签。所有自有维护进程关闭并等待 exit 0，清理仅自身随机凭据；不安装驱动、不切换输入桌面、不发送系统键鼠、不占用 Default 焦点。

第二轮 out/mcp_bridge/elevated-linked-qt6.log 已完成配对／RPC／重连／撤销，但读测试 fixture.json 遇到与 QSaveFile 原子替换竞争的 PermissionError。测试读取改为 Windows 共享读／写／删除句柄、128 KiB 上限；仅对短暂共享／访问竞争有界重试，PID／实际桌面仍须匹配，异常不当作成功。之后 elevated-reader-qt6 12 短场景及最终 accepted-high 两套完整场景通过。失败记录未覆盖，不把该测试观察竞争记为产品配对失败。

IPC 实际 EXE 为 out/tests/mcp_ipc/{qt6-release,qt5-release,qt6-faults}/release/QKeyMapperMcpIpcTest.exe，各 build_linked.log／results_linked.txt／test_linked.log，汇总 out/mcp_bridge/linked_cpp.log；stdout／stderr 空、父进程未提权。ASan 为 out/tests/mcp_ipc/qt6-asan-1440/release/QKeyMapperMcpIpcTest.exe，BuildTools MSVC 14.40.33807，实际导入 clang_rt.asan_dynamic-x86_64.dll；build_linked.log／results_linked_asan.txt／test_linked_asan.log。覆盖 IPC-21 十个方法及新增 linkedLogonSession，另含 init／cleanup；Qt DLL 未重插桩、detect_leaks=0，不等于 94 项全量 ASan。

GUI 普通维护目标实际桌面分别为 QkmMcp_333dcc99037848219b549e008027a6df／QkmMcp_901bf4e91264487f8a36a36cc6d53222，无 UI 布局／文字变更，三语言静态效果复用 IPC-21，未重拍。所有构建和分析在原开发沙箱，实际用户 DPAPI 运行按既有授权；测试 gsudo 不更改缓存／系统策略。产品 EXE 仍为 out/build_qt6／out/build_qt5_5152 下 release/QKeyMapper.exe，原始材料只在忽略的 out，未随 Git 同步。

本切片形成身份兼容性的本地开发检查点；M1.1c-4e-b 的真正产品入口、默认应用绑定／存储及实际产品高低权限流程仍待验收。真实异用户／异登录／Win11、业务等价性、性能和打包没有当前结论，未计完成。

## IPC-20261022-23：实际产品入口初验与配对选中项修正

PC-W10，起点 `b1f3ddf7`。本记录对应的产品修改仅在 ControlDialog 的选择／撤销目标，不改通信、凭据格式、Worker、驱动、TS 或 manifest。用户明确批准固定管理员 GUI 助手；随后外出用餐，要求其返回前延后新的 UAC 作业。所有已启动的自有产品均已正常关闭，最后固定名称进程清查为空，原用户 PID 5420／启动时间未变；无 Default 场景、真实输入或驱动安装。

### 选择修正的独立提交切点

实际 Qt6 产品 UIAutomation 的 SelectionItemPattern 已报告列表项 selected=true，撤销按钮仍禁用。进一步增加两个普通 Qt GUI 回归：清除选中而保留 currentRow，以及可见选中第二项而 currentRow 仍为第一项。旧实现分别允许无选中撤销、错误撤销第一客户端；不能用视觉一致替代实际目标检查。

窗口改为按 selectedItems 恰为一项启用／执行撤销，并按同一选择保留刷新状态；复用原服务器撤销及私有原子持久化路径。无选中不修改；选中第二项时保留第一项，并独立从真实存储解密读取剩余 pair ID。

| 当前检查 | 结果／原始索引 |
|---|---|
| Qt6 修正前回归 | 16 通过、2 失败、0 跳过，native exit 2，2,035 ms；out/mcp_product/selection-red-qt6/results.txt／run_private.json，实际桌面 QkmMcp_7f812f12b9a84daebbe3be64f5beecbc |
| Qt6 修正后 GUI | 18 通过、0 失败／跳过，native exit 0，1,872 ms；selection-green-qt6 同名文件，实际桌面 QkmMcp_86070b4f4037443ab84b90cf4d690c3a |
| Qt5 修正后 GUI | 18 通过、0 失败／跳过，native exit 0，23,039 ms；selection-green-qt5 同名文件，实际桌面 QkmMcp_a497b0d1c9324eada33455e8eb907d0a |
| 维护目标增量构建 | 两套 native exit 0、无编译／链接诊断；out/tests/mcp_control/{qt6-release,qt5-release}/build_selection.log，实际 EXE 为各 release/QKeyMapperMcpControlTest.exe |
| 产品增量构建 | 两套 native exit 0；out/mcp_product/build_qt6_selection_verified.log／build_qt5_selection_verified.log；Qt6 无编译／链接警告，Qt5 仅既有官方 qtmain 缺 PDB 例外；实际 EXE 为 out/build_qt6／out/build_qt5_5152 下 release/QKeyMapper.exe |

首轮产品构建曾遇 qmake 的 Git 元数据读取被沙箱所有权拒绝；保留 build_qt6_selection.log／build_qt5_selection.log，不作为最终构建身份凭据。随后仅对构建进程传入 safe.directory，重新构建成功。最终日志保留沙箱不能读取个人 Git ignore 的警告；这是 Git 环境诊断，不是编译／链接诊断，未改变全局 Git 或忽略配置。共享通信／原生身份／ASan／SDK 故障套件代码未改，IPC-22 结果按原范围复用；本次未重跑整个 IPC、ASan 或严格静态分析。

### 真正产品初验：已验证部分与待验证部分

以正式 Release EXE 的同字节副本运行，不改 manifest。入口实际按主界面的“通用设定 → 通用高级设定 → Agent / MCP 控制”驱动；辅助脚本只允许固定动作，验证 PID／EXE／精确启动时间、目标及助手的实际私有桌面。普通控制器／桥接为 Medium 8192／TokenElevation=0，产品为 High 12288／1，同用户系统 UAC 登录关联通过。产品启动按原逻辑尝试连接本机 FakerInput；本次授权已包含该启动影响，不启动映射或安装驱动。

源码 b1f3ddf7 对应 out/mcp_product/stock-qt6-owned-dialogs：实际桌面 QkmProduct_bd213665897348a8915eb3f8c766fb6e。已通过默认关闭／连接框为空、GUI 启用、默认应用绑定与私有发现、普通官方 SDK 配对前不 ready、只读批准、凭据写入、真实 transport_only RPC、新 stdio 凭据重连及关闭管理窗口保留接口。`business_capabilities=[]`，没有产品业务。随后重开入口的助手流程未完成，完整测试返回非零；该轮只证明日志中的三个已完成分项，**不是整轮通过**。

后续 stock-qt6-offline-revoke／focus／selection 从同一隔离产品路径冷启动，接口关闭且实际读到先前一个配对，证明此产品默认应用绑定可恢复记录；其中 selection 记录 selected=true 但按钮禁用，促成本节修正。修正后的 stock-qt6-offline-revoke-fixed 使用重新构建的同字节正式 EXE，实际桌面 QkmProduct_22ebcc4969d5428b9932adf0730012aa；GUI 撤销按钮已能调用，但助手紧接的界面观察仍见一项，脚本因此失败。**最终撤销／持久化结果待冷启动核实，不能当作已成功，也不能盲目重试**。助手现已补有界完成观察，未再执行 UAC。

运行目录 out/mcp_product/runtime-b1f3ddf7-qt6（文件名保留基线，内容已更新至本节修正），EXE 为 QKeyMapperMcpAcceptance_b1f3ddf7.exe。Qt5 的 runtime-selection-qt5 已部署同字节产品、对应 Qt 依赖和隔离 INI，尚未启动。独立路径／文件名用于隔离 SingleApplication；main.cpp 会按 argv[0] 设置 applicationName，日志模式也保留目录差异，不能只假定相同文件名副本一定独立。

私有桌面启动／关闭复用 scripts/test_process.ps1 的新待提交扩展：CreateProcess 指定 lpDesktop；关闭在新原生线程绑定该桌面后发送 WM_CLOSE。PowerShell 已使用的线程上 SetThreadDesktop 返回资源在使用中，现改为创建时选桌面；GUI 对话框按同桌面原生 HWND／PID／标题定位，不能假定属于 UIAutomation RootElement 的直接子项。首轮 stock-qt6-first 遗留的自有 PID 27900 已经原身份核验后正常关闭，exit 0；后续 closed.json 均记录 0。普通窗体自检 helper-private-close-thread 已验证参数／Unicode 环境、实际桌面、错误启动时间拒绝关闭、正常退出 0 和不存在桌面拒绝启动；该测试不等于完整高权限产品验收。

普通权限 scripts/capture_ui_snapshots.ps1 在目标实际私有桌面附着 High 产品时 PrintWindow 失败，未保存截图；见 stock-qt6-accessibility／stock-qt6-owned-dialogs/main.log。未给截图提权、不切换输入桌面；实际产品静态视觉仍未验证。ControlDialog 布局／文本未改，中英日同源维护窗口视觉只复用 IPC-21。

本节仅提交已通过的产品选择修正、两个回归及跟踪记录；新测试进程工具、product_lifecycle.ps1／product_ui_actions.ps1／test_product_interop.py 仍在工作区，待完整验收再形成检查点。原始日志、截图、INI／EXE 和保护存储均不随 Git 同步，不记录密钥。实际产品完整撤销／旧 key 冷启动拒绝、Qt5 产品高低权限、托盘、Win11、业务等价性及性能仍待验收；用户离开期间只推进不需要 UAC 的工作。

## SRC-20261022-24：M0.2b 第一批操作追踪与共用提取边界

源码 `2b9215ef`，PC-W10；本轮只修改三份跟踪文档，没有产品源码或运行操作，不发起 UAC。

新增 14 条静态链路：插入计划，Tab 新增／复制／删除／排序／清空／名称／快捷键，单表导入／导出，普通及通用宏新增更新／删除／导入／导出。连同原 16 条共 30 条；候选控件仍为 387、动态动作族／入口行 38，数量不是已实现或已通过等价性的能力数。

本轮读源确认的关键差异：prepareMappingDataForInsertion 在共通冲突时会直接修改其他源表，false 提示参数并非 dry-run；Tab 移动末端函数是 Qt 页序已经变化后的数据回调，前面还有共通顺序恢复；单表导出未显式 sync/status，宏导出 sync 后未检查 status；宏文件四字段不含配置保存中的逐段备注，手工宏添加和文件导入在校验、覆盖排序方面不同。除实际读源外，没有将这些差异作为当前运行缺陷复现或修正。

主计划据此细化最小共用校验／纯分析／提交职责，并约束外部文件在普通桥接权限下读写，管理员业务端处理有界内容及受控内部目标。未开放接口、未增加新权限或改变现有格式。

`out/mcp_product/audit_m0_2b.py`／同名 .log：检查 30 个链路 ID 唯一、14 个新增 ID、四份 cpp 中 20 个函数锚点、本任务三份文档的本地链接与 256 KiB 限制，exit 0；`git diff --check` 通过。新增链路的 GUI／MCP 对照、冷启动、真实输入和系统效果仍未执行。产品构建／GUI 结果复用 IPC-23，通信／ASan 等复用 IPC-22 的原范围，本轮文档检查不替代任何运行证明。

## TOOL-20261022-25：可复用的私有桌面进程工具

源码 `3e5274b9` 加本记录所在检查点，PC-W10。用户外出后本轮只执行普通权限测试，未运行 gsudo 或发起 UAC。

scripts/test_process.ps1 新增可选 DesktopName，未指定时保持现有 Process.Start／EnumWindows 路径。指定时先确认桌面存在，以 CreateProcess 的 lpDesktop 启动；环境按 Windows 规则排序并拒绝 NUL；在释放创建句柄前取得托管进程句柄，避免 PID 被复用后误认目标。按指定桌面枚举主窗口；关闭在新原生线程绑定该桌面发送 WM_CLOSE，再恢复线程桌面。保留原来的精确 EXE／启动时间检查、gsudo 只启动／关闭及正常等待退出约束，不改 manifest，也没有 SwitchDesktop。

新增 scripts/test_test_process.ps1 以自有 WinForms 窗体核对实际桌面／PID、带空格参数、中文日文及 shell 元字符的字面环境值、错误启动时间拒绝关闭、恢复正确身份后 exit 0、不存在桌面拒绝启动。最终命令 `pwsh -NoProfile -File scripts/test_test_process.ps1 -OutputDirectory out/mcp_product/helper-private-retained-handle`，退出 0；日志在同名 .log，状态在目录内 state with spaces.json；实际桌面 QkmProcess_2625704add0c4daf83a13103d5138ddc，自有 PID 13412 已退出。仅验证 GUI 测试进程，未声明极短命令的启动语义或 PowerShell 5.1 兼容通过。

同时验证原截图调用者：out/mcp_product/check_capture_helper.ps1 经当前工具在私有桌面启动 Qt6 同源维护预览（paired-off），使用原 scripts/capture_ui_snapshots.ps1，经现有 capture_mcp_control.ps1 捕获／关闭。控制器、预览及截图均非管理员；exit 0，实际桌面 QkmCapture_25cb1ceaf158477b9a7fea953f634e6e，预览 PID 31120，native DPI 144／Qt DPR 1.5／Qt logical DPI 96，Fusion／Microsoft YaHei UI 9pt，接口关闭、假配对 1。自主审阅 capture-helper-current/paired-off.png：中文说明、列表、撤销及关闭按钮可见无截断。这只证明当前普通权限截图调用兼容和单张同源控件静态效果，不能代替 High 产品截图或运行配对证明。

PowerShell 解析、C# 实际编译运行、git diff --check 通过。未改产品源码；Qt6／Qt5 产品及 GUI 结果复用 IPC-23。当前托管句柄保留改动未重跑管理员启动／关闭，新增 UAC 验收继续等待用户返回；更早实际 High 产品的私有桌面启动和正常关闭记录仍仅按 IPC-23 的原工具版本引用。product_lifecycle.ps1／product_ui_actions.ps1／test_product_interop.py 仍待完整验收、尚未提交。本切片只提交基础进程工具、独立自检及跟踪文档，不推送。

## SRC-20261022-26：停止完成边界的调用链核对

源码 `d38cf8dc`，只读核对 qkeymapper.cpp 的 setKeyUnHook／MappingSwitch、qkeymapper_worker.cpp 的停止／任务控制器／信号连接，以及现有 sendinput_lifecycle 维护目标。未启动产品、未发送输入、未运行 UAC 或改引擎代码。

核对结果写入主计划“明确启停与清理完成”：区分请求停止、真正等待池排空、停止中新任务、KEY_UP 记账与 native 发送、永久关闭发送入口的退出函数、常驻 Hook 回调及 GUI／配置尾部。尤其停止动作经排队信号可在 STOPPED 后建任务；把完成信号放在该状态赋值旁边仍不足以证明清理完成。复用了已有架构作为提取位置，没有将其当作新增运行证明。

另外确认键串校验族在多处通过当前 KeyMappingDataList 的 size／at 取源字段，部分 matched 参数仍受当前表行数限制；单独传候选键还不足以支持非当前源表。共同提取应贯穿原始键／单键／映射键和必要递归路径，不能临时切表或换全局指针。只读提取时 Python 控制台曾因默认 GBK 无法输出源码的时钟符号而中断，改用 -X utf8 后完整输出；这是工具输出编码问题，无产品变化。

本轮验证仅为源锚点／调用者阅读和跟踪文档一致性；新增完成代次／栅栏、独立 GUI／MCP 对照、线程／按键释放、超时及性能全部待实现／验证。用户要求延后 UAC 仍生效，未把该等待记为整个计划受阻。

## IPC-20261022-27：选择修正后普通权限正式 SDK 再验

源码 `d38cf8dc`；产品 C++ 最近变更为 `2b9215ef`，此后只改文档和基础测试工具。PC-W10；普通控制器与同源维护服务均为 Medium／TokenElevation=0，未运行 gsudo、UAC、真实输入或驱动操作。

以 out/mcp_bridge/venv/Scripts/python.exe 执行 mcp_bridge/tests/test_sdk_interop.py，指定两套 out/tests/mcp_control/{qt6-release,qt5-release}/release/QKeyMapperMcpControlTest.exe 和对应 Qt bin。Qt6／Qt5 各 12 个短场景通过，脚本 native exit 0；日志为 out/mcp_product/sdk-selection-qt6.log／sdk-selection-qt5.log，同名目录内 results.json／topology.json／fixture.log。实际私有桌面分别为 QkmSdk_44a4b09aee4645e6844f5e3c69fae6e8／QkmSdk_08169c2a691e4a55b5cb0bc1ffedd0ab；两端完整性 RID 8192，同用户／登录／AuthenticationId／SessionId。官方 SDK mcp 2.3.0，五个正式工具。

覆盖无应用、GUI 批准／双向证明／凭据保存／真实 RPC、新 stdio 凭据重连、断开幂等、撤销断线且不静默重配对、GUI 拒绝、取消读及 stdio 退出、关闭接口时撤销及重新启用后旧 key 拒绝、错误预期 EXE、私有发现篡改、服务器持久化失败不 ready。所有自有维护进程正常退出 0，仅清理自身随机凭据。该短场景集合不含 60 秒等待，后者复用 IPC-22。

这是正式 SDK 到同源普通维护服务的当前集成证明，不能替代 IPC-23 尚未完成的实际管理员产品验收，也不证明任何映射业务等价性。UAC 后续仍按用户要求延后；没有重跑无关 IPC／ASan／产品构建。

## SRC-20261022-28：通用高级／映射高级设置全字段静态追踪

源码 `d8e435f4`，PC-W10；只读 qgeneraladvanceddialog、qmappingadvanceddialog、qkeymapper 及 Worker 的实际信号、getter、加载／保存和消费者，未修改产品源码或启动产品。新增 10 条通用高级与 11 条映射高级链路，累计 51 条静态追踪；38 个既有 UI 入口已关联对应链路，仍明确业务／运行未验。

通用高级记录启动模式／坐标、窗口尺寸／分割恢复、编辑／插入方式、声音、全局切换延时、全屏开关及允许进程，核对参数范围与根设置。映射高级记录速度／轮询、触摸板两轴速度／反转、虚拟手柄输入过滤、进程托盘图标、坐标显示键、四组三阈值、共通开关、自定义通知启用／位置，核对配置组存储、Worker 实际消费者和既有生效时机。

需要接入时保留的区别：白名单 textEdited 才 dirty，setText 不等价；延时编辑不重排既有 timer，0 走 queued invoke；鼠标轮询只在映射开始／重启启动对应 PreciseTimer；阈值 setValue 受动态 min／max 和应用顺序影响；共通启用会修改其他源表 Disabled；自定义通知首次初始化会复制完整全局样式，加载信号屏蔽另有处理。这些为静态事实／待验风险，不是运行缺陷复现，也不证明 Worker 直接读控件线程安全。

`out/mcp_product/audit_m0_settings.py`／同名 .log：51 个唯一链路、21 个新增链路、两份真实 UI XML 的 38 个关联入口、四份 cpp 的 47 个函数定义、12 个实际常量值、本地链接及单文件 256 KiB 限制，native exit 0。另执行 git diff --check。原始审阅／审计在忽略的 out，不随 Git 同步；精简结论随本记录同步。

没有 UAC／gsudo、输入、驱动或系统设置操作；未运行产品构建、真实 GUI／MCP 业务对照、实际计时／鼠标／手柄／声音／托盘效果及冷启动。这些在能力矩阵逐项保留为未来验收要求，不用文档检查代替。当前 Qt6／Qt5 GUI／SDK 复用 IPC-23／27 的原范围，实际管理员产品剩余验收继续等用户返回。

## SRC-20261022-29：配置切换／传输、窗口匹配和忽略规则

源码 `f73f0db0`，PC-W10；仅源审阅与文档。新增 17 条链路：配置加载／空白／删除／排序／组导出／组导入，窗口条件／显示模式／进程选择／恢复路径／自动匹配／相同窗口发送／上次匹配恢复，忽略规则新增更新／删除／恢复默认／编辑会话。累计 68 条静态追踪、63 个既有 UI 入口关联；MCP 产品业务仍未开放。

读源确认 Qt6.8.3／Qt5.15.2 配置选择走 textActivated，单纯 setCurrentIndex 不加载。窗口匹配的草稿、MAP_PROCESSINFO 与自动选配置的磁盘来源不同；当前显示顺序不是自动匹配优先级。组传输只拷选择组的 childKeys 并合并目标，sync 后未检查 status；导入顺序规范化代码实际在 copied==0 分支。忽略规则以九字段有序表存根设置，内存编辑不等于已保存；缺失／非法根值回退默认与合法空列表不同。均为当前静态事实或待验风险，不是运行故障复现。

`out/mcp_product/audit_m0_context.py`／同名 .log：68 个唯一链路、17 个新增、四份实际 UI XML 的 63 个关联、两份 cpp 中 47 个函数定义、九条默认规则、本地链接／文档大小，native exit 0；git diff --check 通过。原始审计在 out，不随 Git 同步。

没有产品构建／启动、UAC／gsudo、真实输入、设备或系统设置操作；实际 GUI／MCP、写盘失败、冷启动、前台／后台目标效果全部待后续按项验证。此前 GUI／SDK 结果仍按 IPC-23／27 原范围引用。本切片结束时用户通知已返回，可恢复原获准的受限 UAC 产品验收；新通知不改变本节只读检查范围。

## IPC-20261022-30：实际 Qt6 撤销核实、五分项通过及冷启动助手失败

PC-W10，源码 `3fd3deb3`，产品 C++ 仍为 `2b9215ef` 构建的同字节隔离副本 out/mcp_product/runtime-b1f3ddf7-qt6/QKeyMapperMcpAcceptance_b1f3ddf7.exe。用户返回后明确恢复原受限 UAC 范围，未修改产品或 manifest；管理员仅自有产品启动／关闭及固定 GUI 动作，官方 SDK／桥接／截图／断言为普通权限。未启动映射、发送真实输入或安装驱动，无 Default 场景。

首先 out/mcp_product/observe_product_pair.py 只启动并打开管理窗口观察；stock-qt6-revoke-observation.log／同名目录 observation.json：cold_start_pair_count=0、接口关闭、连接框为空，证明 IPC-23 那次撤销已持久化，**没有再次执行撤销**。真实桌面 QkmProduct_8be4cac9d45d4ca498c69546e6801ae8，产品 PID 30832、High RID 12288，普通控制器 Medium 8192／同登录，产品正常关闭 exit 0。

随后正式 test_product_interop.py 指定上述 EXE 及新输出 out/mcp_product/accepted-stock-qt6。日志 accepted-stock-qt6.log 的五个已完成分项：实际高级设置／MCP 入口及默认绑定／发现；只读 GUI 批准／真实凭据／正式 RPC 且关闭窗口保留接口；新 stdio 保存凭据重连；实际 GUI 撤销断线及旧 key 拒绝；关闭接口移除发现。08_Revoke.json 记录 enabled=true、pair_count=0，正式 MCP 另核对权限已失效。实际桌面 QkmProduct_14fccdffee044bdc943bd1ff8517aba3，PID 7920，High 12288；普通控制器／SDK Medium 8192／同登录。产品随后正常关闭 exit 0，客户端自身凭据 finally 已清理。

该轮 **native exit 1，未生成完整 results.json**：冷启动的新生命周期助手调用 StartPrivate 返回系统找不到指定文件，未产生 started.json；关闭阶段 PyHDESK.CloseDesktop 又报 Win32 6 无效句柄。桌面对象显式关闭／析构／句柄复用是待用普通权限最小复现核对的假设，尚未确定根因，不当作产品冷启动故障或已通过。冷启动后保留旧 key 拒绝、Qt5 正式产品仍未执行通过。

原截图脚本保持非提权，在 High 产品 PrintWindow 失败，日志明确 UNVERIFIED，未保存图、未改权限；不能以同源维护图替代本轮静态视觉。两份 PowerShell 及 Python 语法检查通过；产品未重建，普通 Qt6／Qt5 SDK／GUI 仍按 IPC-23／27 原范围引用。

用户随后要求在合适位置暂停以关闭 fast。收尾核对：上述两产品 closed.json 均为 exit 0；没有 QKeyMapperMcpAcceptance_* 进程。普通实际用户 CIM 核对相关 product_lifecycle／product_ui_actions／test_product_interop／observe_product_pair 助手数为 0，原用户 PID 5420／启动时间 2026-10-22 10:17:55 未变。默认沙箱 CIM 访问被拒绝后只切换普通 Medium 只读查询，没有新 gsudo／UAC。三份正式产品测试脚本仍未提交，暂停卡片记录恢复动作；本轮只形成文档本地开发检查点，不推送。

## IPC-20261022-31：桌面句柄修正及两套正式产品六分项

PC-W10，起点 `26ecb9e5` 加本记录所在测试切片；产品 C++ 仍为 `2b9215ef`，未改 manifest、引擎、驱动或业务接口。用户通知继续后恢复原固定 GUI／UAC 范围。产品启动按既有逻辑连接已安装 FakerInput；无映射启动、系统输入、驱动安装、Default 或焦点占用。

### 冷启动助手根因与修正

普通权限真实探针复现：PyHDESK.CloseDesktop 后对象仍持有旧非零值，新的 CreateDesktop 复用该值，释放旧对象后新桌面原生名称查询失败。与 [pywin32 桌面绑定](https://github.com/mhammond/pywin32/blob/main/win32/src/win32service.i) 的显式关闭／析构及 [Detach 实现](https://github.com/mhammond/pywin32/blob/main/win32/src/PyHANDLE.cpp) 一致；不是产品冷启动失败。SDK／产品测试共用 close_desktop，在成功 CloseDesktop 后 Detach，保留失败异常，防止析构重复关闭。

新增 test_desktop_lifecycle.py：普通权限先失败（desktop-lifecycle-red.log，保留句柄值断言失败／exit 1），修正后 desktop-lifecycle-green-final.log，1 条真实桌面用例内连续 8 轮通过，0.205 s／exit 0。初版观察误用空桌面 EnumDesktopWindows，其错误不证明句柄失效；已用原生 GetUserObjectInformation 名称／身份查询替代。只创建／关闭自有桌面，无产品或 UAC。

### 真正产品及正式 SDK

| 当前检查 | 结果／原始索引 |
|---|---|
| Qt6 正式产品 | 六分项／script native exit 0；out/mcp_product/accepted-stock-qt6-desktop.log 及目录 results.json、两轮 topology.json／closed.json。首次 PID 15448、冷启动 PID 5964，两者 exit 0；实际桌面 QkmProduct_2d7013d2ee7a4f4a8773287032ca3df5／QkmProduct_907eb1a4762e4482b30537797c2da4af |
| Qt5 正式产品 | 六分项／script native exit 0；accepted-stock-qt5-patterns.log 及同名结果目录。首次 PID 12008、冷启动 PID 31488，两者 exit 0；实际桌面 QkmProduct_a3cb4ea7824a4d1eb21ed83fdb425035／QkmProduct_ff52b2c73bfb438bb83054a9f5acde49 |
| SDK 普通 Qt6 | sdk-desktop-fix-native-qt6.log 各 12 短场景通过／exit 0；随后带夹具诊断的 sdk-native-diag-qt6-1／2／3 各 12 通过／exit 0，当前三个完整序列未重现下述首次失败。结果／fixture.log／topology.json 位于同名目录 |
| 维护诊断构建 | out/tests/mcp_control/qt6-release/build_desktop_diagnostics.log，native exit 0、无编译／链接警告。只给 interop 维护入口的主动失败加固定原因及 SDK 原生退出码，不改产品行为 |

六分项分别验证真实高级设置／MCP 入口和默认私有绑定、GUI 只读批准及凭据／真实 transport_only RPC、关闭管理窗保持接口及新 stdio 重连、撤销导致断线／旧 key 拒绝、关闭接口移除发现，以及产品完整退出后重新启动仍默认关闭、零配对并拒绝保留的旧客户端 key。每项经官方 mcp 2.3.0 Client／五工具，C++ 产品为 High／RID 12288，控制器／SDK／桥接为 Medium／8192，同登录关联。退出只清理自身客户端凭据。

Qt6 实际 EXE 仍为 runtime-b1f3ddf7-qt6/QKeyMapperMcpAcceptance_b1f3ddf7.exe（名称保留旧基线，内容为选择修正后的产品）；Qt5 为 runtime-selection-qt5/QKeyMapperMcpAcceptance_selection_qt5.exe。两者源自 IPC-23 已核实的当前产品副本，不复用另一 PC 产物。跨机准备步骤和合成 INI 模板已放入 tests/mcp_control，不复制个人 INI。

Qt5 首次 accepted-stock-qt5-desktop 在 OpenAdvanced 因“不支持的模式”退出，产品 exit 0。固定 GUI 助手选择操作现按实际支持的 SelectionItem／Invoke 模式处理，目标／权限／动作范围不变，修正后完整流程通过。Qt6 已通过轮使用原 SelectionItem；该正向路径与新助手的可用 SelectionItem 分支等价，复用其结果，没有为了无影响的 fallback 增补重复 UAC。

SDK 的 sdk-desktop-fix-qt6 首次短回归在 disable 分项前维护进程已终止，整轮 exit 1；当时 fixture.log 为空且未打印原生退出码，**原生退出原因仍未知，不能追认该轮通过**。新增主动失败诊断并完成上述有界三轮后未重现；保留失败原始记录，如再次发生先根据原生码／夹具原因调查，不盲目重启。此处不将未重现当作已修复的产品故障。

两产品的普通 PrintWindow 仍失败，results.json 的 stock_product_snapshots=false；未提权截图，静态视觉只按 IPC-21／TOOL-25 同源维护证据引用。CPP UTF-8／diff、Python 语法及 PowerShell 解析按本切片核对；未重建产品、未重跑无关 IPC／ASan／严格分析，原结果仅按 IPC-22／23 范围复用。SDK Qt5 普通 12 场景复用 IPC-27；实际 Qt5 管理员产品是本轮新验。

结束后普通实际用户 CIM 核对自有产品助手数 0，四个 closed.json 均 exit 0，原用户 PID 5420／启动时间未变。原始材料均在 out，不随 Git；测试脚本、模板及精简记录形成可同步的本地开发检查点。托盘、实际异用户／Win11、性能、打包和业务等价性仍未验；business_capabilities 仍为空。

## STATE-20261022-32：首批只读摘要实现、局部检查与用户暂停

PC-W10，HEAD `3511deae` 加工作区 10 个实现／测试文件及本轮跟踪文档；2026-10-22 21:24（UTC+8）按用户请求在已启动构建结束后暂停，以便关闭 fast 模式。本轮没有启动新产品、副本或 GUI 助手，没有 UAC／gsudo／真实输入／驱动操作。未创建新的提交、未推送；源码和记录均已保存，但未提交部分不能假定已同步到另一 PC。

产品接入固定 Read 请求 `qkm_get_state` 和相应 state.read 能力声明：真实配置组／类型、当前 Tab、dirty、GUI 监视状态、Worker 原子采样和实际停止快捷键；不是控件请求值回显。修订号为 null、停止完成为 unobserved，不把 IDLE／STOPPED 当作清理完成或允许写入的证明。会话共用能力响应仍保留无业务处理器的 transport_only 后备；引擎、驱动、manifest 和 TS 未改。桥接增加固定无参数工具并复用已有有界请求执行，不提供任意方法透传。

当前检查：out/mcp_product/state-protocol-unit.log 记录 14 tests／OK（0.106 秒），覆盖固定 RPC／空参数、非法响应、应用错误保留连接及忙状态；它不证明正式 SDK 对未知参数的处理或 C++ 业务路径。Qt6.8.3 x64 Release 使用 scripts/build_qt6.ps1 -Jobs 4，out/mcp_product/build_state_qt6.log 有 Build succeeded，out/build_qt6/release/QKeyMapper.exe 于 21:20:12 生成；日志扫描无 C／LNK 编译链接警告及 fatal error。原工具会话已结束，最终退出码未能恢复，故不补写原生 exit 0。暂停时未见 jom／cl／link／msbuild；该 EXE 未部署、未启动。此前 UTF-8／LF 与 git diff --check 通过，暂停文档补记后再次检查差异。

已写入但未运行的验收扩展：正式 SDK 工具列表／未知参数拒绝，真正产品状态与独立 GUI 停止快捷键／按钮对照、重复读取不改变状态。OpenAdvanced 中只增加读取观察，没有新增高权限写动作；停止快捷键在隐藏 Mapping 页，是否能通过现有 UIA 读取尚未知，若失败先调查实际可访问性，不静默删去独立对照。后端未知参数／加载中／超限、相关共享会话回归、Qt5 构建与产品运行全部未执行。新能力不能据此标记支持或 M1 完成；IPC-31 的旧两套产品结果仍仅用于原通信功能。

接续先检查正式 SDK 是否严格拒绝未知字段（新断言尚未运行）；再完成选定 Qt5／核心检查、新隔离运行副本及产品 GUI／MCP 对照。源数据查询、修订号／稳定引用、纯预览提交、保存及停止完成继续按原计划实施。SDK 维护夹具先前一次提前退出的未知原因、High 产品截图／托盘、实际异用户／Win11 等既有缺口不变。

## TOOL-20261022-33：私有空桌面窗口查找误判的独立修正

PC-W10；基线 `3511deae`，本节对应 scripts/test_process.ps1、scripts/test_test_process.ps1 的局部修正。M1.2a-1 产品／桥接实现仍在工作区另行验收，不混入本工具修正的已验证范围。

out/mcp_product/accept-state-validated-qt5 在创建窗口前遇到 MainWindow 异常“操作成功完成”，该轮 script exit 1；closed.json 确认自有产品 PID 30520 正常 exit 0，普通 CIM 核实没有遗留自有产品，因此不是已经证实的 Qt5 产品崩溃。独立空桌面断言先运行：helper-empty-red.log，native exit 1，未启动产品／GUI 夹具就复现相同异常。

本机空桌面 EnumDesktopWindows 返回 FALSE、GetLastError 为 0。按 [Windows API 返回语义](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumdesktopwindows) 及实际复现，查找前清零线程错误；未找到窗口且错误为 0 时返回空句柄，让调用者原有有界等待决定是否超时，非零错误继续抛出，已找到窗口的回调终止保持成功。没有扩大桌面权限、切换输入桌面或弱化 EXE／PID／启动时间守卫。影响调用者包括私有桌面产品生命周期／GUI 助手、截图窗口查找及关闭工具；产品 C++、manifest、驱动和输入逻辑均未改。

修正后 pwsh -NoProfile -File scripts/test_test_process.ps1 -OutputDirectory out/mcp_product/helper-empty-green，native exit 0，helper-empty-green.log 与同目录 state with spaces.json 保留。本轮普通权限覆盖空桌面未找到、实际 WinForms 私有桌面／PID、中文日文／shell 字面环境值、错误启动时间拒绝关闭、正常退出 0 以及不存在桌面拒绝启动。没有 gsudo／UAC、真实输入或驱动操作。Qt6 已通过的产品正向路径没有改动其有效输入，按原范围复用；Qt5 正在使用修正工具重新验收，结果另记。Win11／PowerShell 5.1 未执行，不将本机观察表述为双系统验证。

## STATE-20261022-34：首批状态摘要与双 Qt 正式产品停止态验收

PC-W10；产品源码为 `3511deae` 加本检查点的主窗口只读快照与共用能力响应；空桌面工具为 `1a64eade`，正式桥接／回归为本检查点。没有修改引擎、驱动、manifest、INI 格式或 TS。当前只开放 state.read 的实际摘要：schema_version=1，配置组／类型／dirty、当前 Tab、GUI 状态、Worker 原子采样及实际停止快捷键。业务修订号仍为 null、停止清理仍为 unobserved；不提供编辑目标引用、配置写入或停止完成承诺。

在固定 mcp 2.3.0 中实际复现了额外字段被默认忽略：state-sdk-unknown-red.log，script exit 1，qkm_get_state(unknown=true) 进入函数后返回 not_connected，SDK 并未拒绝参数。随后仅在注册的参数模型设 extra=forbid、重建并同步发布 schema；没有修改依赖或绕过 SDK。state-sdk-unknown-green.log，exit 0，六工具的 additionalProperties=false、实际额外字段拒绝及拒绝后仍 disconnected 通过；永久 SDK 回归中保留对应六个调用。

| 当前检查 | 结果／证据（均在本机 out，不随 Git） |
|---|---|
| Qt6／Qt5 x64 普通 Release | scripts/build_qt6.ps1／build_qt5.ps1 -Jobs 4；build_state_qt6.log／build_state_qt5.log 初轮有成功标记，但中断后工具退出码丢失。恢复后 build_state_qt6_finish.log／build_state_qt5_finish.log 的增量完成命令均 native exit 0；Qt6 无编译链接警告，Qt5 只有既有官方 qtmain LNK4099 例外，无新增警告 |
| 维护／IPC 目标 | out/tests/mcp_control/{qt6-release,qt5-release} 与 out/tests/mcp_ipc/{qt6-release,qt5-release} 的 build_state.log，四个增量构建 native exit 0、无编译链接警告 |
| Python 协议／工作线程 | state-protocol-final.log，14 tests／OK，0.108 秒，native exit 0；固定 RPC、空参数、畸形响应／应用错误、忙状态等 |
| 两套正式 SDK | sdk-state-qt6.log／sdk-state-qt5.log，同名目录内 results.json／topology.json／fixture.log；各 12 个短场景、native exit 0，含六工具严格参数及维护端未接业务时 get_state 拒绝。GUI 服务、控制器及桥接均 Medium；60 秒批准超时本轮未重跑，仍仅复用 IPC-22 原范围 |
| 两套相关原生会话 | out/tests/mcp_ipc/{qt6-release,qt5-release}/state-session.txt；sessionPairAndReconnect、sessionTransportCapabilities、sessionHandlerLifecycle 三组均通过，含 init／cleanup 各 5 passed／0 failed，native exit 0。只选与能力响应、权限及处理器生命周期相关的范围，未重跑全套故障／ASan／严格分析 |
| 正式 Qt6 产品 | accept-state-validated-qt6.log 与同名目录 results.json，8 分项／native exit 0；runtime-state-qt6/QKeyMapperMcpAcceptance_state_qt6.exe 与本机产品字节一致，部署时 SHA256 98A81F7BD22549363B40AD73EC23EC2227BC51BB0BFFE9B67B4A244186F88ADF |
| 正式 Qt5 产品 | accept-state-window-qt5.log 与同名目录 results.json，8 分项／native exit 0；runtime-state-qt5/QKeyMapperMcpAcceptance_state_qt5.exe 与本机产品字节一致，部署时 SHA256 185AFF3E6D74B56519B3EB2C075554EC00915671749988459331B5480A0CAEB3 |

产品八分项包括：真实高级／MCP 入口与发现；独立 GUI 停止快捷键／按钮与正式 MCP 状态对照；只读批准／保护凭据及关闭窗口保持接口；经已认证原生负请求独立核对 C++ 拒绝 get_state／capabilities 的未知参数且会话／状态不变；新 stdio 凭据重连；GUI 撤销断线／旧 key 拒绝；关闭清理发现；完整产品退出／冷启动保持关闭及撤销持久化。正向摘要始终通过正式 MCP，原生负例不替代正向路径或 GUI 基线。

GUI 助手在私有桌面选中映射设定页后，通过 UIA 读取 L-Ctrl+F6 和实际中文“映射开始”，再选通用页打开高级窗口；未使用 MCP／业务函数驱动 GUI 基线。合成空白 INI 的配置应为 group 空／kind draft／dirty false、当前 Tab1／index 0／is_common false，MCP 均符合。Qt5 本轮直接运行新增完整默认值断言；Qt6 同轮 state_summary.json 在该断言加入后再次独立核对，未为只新增结果断言重复产品运行。重复查询及非法原生参数后摘要不变。Worker stopped 是单次采样，未作为无在途任务的证据。

Qt6 首轮 accept-state-qt6 因隐藏页控件不可访问而 exit 1，自有 PID 30144 正常 exit 0；调整助手先选映射页。下一轮 accept-state-visible-qt6 读到状态和快捷键，但测试误写按钮预期“开始映射”，实际／源码翻译为“映射开始”，该轮仍保留 exit 1，PID 30080 正常 exit 0。其唯一自有配对通过 cleanup-state-qt6-pair.log 固定 GUI 离线撤销，pair_count=0／enabled=false，清理进程正常退出后才运行通过轮。Qt5 先前一次空桌面助手失败和独立修正详见 TOOL-33，不将这些失败轮追认为通过。

最终产品拓扑均 High／RID 12288，控制器／SDK／桥接 Medium／RID 8192，same_login=true。Qt6 实际桌面 QkmProduct_d81f18a0af21447780662acb4c959e4b／冷启动 QkmProduct_07a2e71596b0482ca9173743f9ea58bc，PID 32284／4320；Qt5 为 QkmProduct_cc0f43bd737f4dc2b7120f347daa289e／QkmProduct_c71cf4118b2641f6aca891be7bb2f5d7，PID 29204／31580。四个 closed.json 均 exit 0。原用户 PID 5420／10:17:55 实例保持运行。除原已授权产品启动／关闭和固定 GUI 助手外未提权；无 Default 场景、真实输入、映射启动或驱动安装。

两套产品普通 PrintWindow 仍失败，stock_product_snapshots=false，截图未提权；新接口不改布局／翻译，静态视觉仍仅按 IPC-21 的同源原范围引用。当前只证明空白配置／停止态子切片；全局／保存配置、运行态、加载／初始化忙与超大摘要错误、稳定引用、写操作、停止清理及实际效果尚未验证或尚未实现。异用户／Win11、性能、打包和 SDK 旧一次维护退出原因仍待后续。编码／LF、差异及相关 Markdown 链接在本检查点核对，不把源码审阅或省略检查记为产品通过。

## SRC-20261022-35：目录查询与映射行快照的实现边界

PC-W10，基线 `95d9ed46`；本节为 M1.2a-2a 工作区实现前／实现中的源审阅，产品验收另记，不算完整源查询通过。buildActiveKeyMappingDataList 使用正常／共通源表构建独立运行列表；Worker 停止／重启会恢复指针并通过 syncRuntimeKeyMappingStatesToCurrentTab 写回 Burst／Lock／LockState，不能直接复制运行指针或假定所有源数据只由 GUI 修改。映射码已有六类显式字段白名单，后续映射行投影应复用该注册表；分页缓存也不能因共享可变运行数据而改变引擎的复制行为。

先实现 configurations／tabs 目录：只读取实际配置选择列表、Tab 元数据和共通附着状态，不读映射行、运行有效映射或未加载配置内容。使用绑定实例／配对／权限代次和目录内容的修订号，每页重新核对；不保留缓存或定时器，相同目录与授权上下文重复查询保持引用稳定，目录／授权变化返回 stale_catalog。attached_to_ui 表示是否附着到 Tab 控件，不承诺屏幕可见。catalog_ref 不是编辑目标，目录修订不是完整业务修订；完整映射源、相关修订及停止完成仍为后续实现。

currentSettingSelectGroupName 读取选择框索引对应的实际组名；当前类没有独立“已加载源配置”身份字段。因此目录使用 selected_configuration／is_selected，状态能力增加 configuration_group_is_ui_selection 限制及源码注释，不把选择当作运行或编辑数据的来源证明。该来源身份需要在映射行／共用操作切片中建立并验证。另补齐两个已有 Worker 重启状态的名称（restart_stopping／restart_starting），没有更改 Worker 状态转换；其真实运行场景仍待启停切片验收。

初期局部证据：catalog-protocol.log 15 tests／OK／native exit 0；catalog-sdk-boundaries-green.log 的早期快照参数草案，七工具未知字段／八类目录参数类型范围及未连接错误语义通过。首次 catalog-sdk-boundaries.log 的额外状态断言失败：有效未连接业务请求按既有逻辑将连接状态标为 failed／not_connected；修正测试顺序／预期，未改变桥接失败语义。sdk-catalog-qt6.log／sdk-catalog-qt5.log 的早期适配各 12 通过。最终收敛为无状态 catalog_revision，以减少缓存／定时器和读取之间的相互失效；这些草案检查不作为最终协议或产品结果，最终参数与产品必须重新验证。写入预览的 120 秒上限按原计划保留。

## CAT-20261022-36：无状态目录修订与两套正式产品读流

PC-W10，基线 `95d9ed46` 加本检查点。C++ 在 GUI 线程读取配置选择目录和当前 Tab 元数据；不读取映射行／运行数据列表，不改 Worker、驱动、manifest、INI 格式或 TS。配置目录使用实际组名和下拉框条目文本；空白项 display_name 是空字符串，通过 kind=draft 识别。selected_configuration／is_selected 不承诺已加载数据身份，attached_to_ui 不承诺屏幕可见性。状态能力补充配置选择限制及两个已有重启状态名称，不改变引擎转换。

最终接口为 qkm_query(entity=configurations|tabs, catalog_revision?, offset=0, limit=50)，limit 1–100，offset 为非负整数，非首页必须携带 64 个小写十六进制字符的修订号。C++ 从实例、配对、权限代次和完整目录 JSON 生成 SHA256 修订，每页重核；不保存缓存、快照或定时器。目录引用不构成授权或编辑目标，仍须原认证／逐请求权限检查，完整业务修订仍未实现。没有每个输入事件的配置哈希。早期 32 位快照／两分钟过期设计未交付，其日志不作为最终验证；预览的 120 秒上限保留给写操作阶段。

| 当前检查 | 结果与本机证据 |
|---|---|
| Qt6／Qt5 x64 普通 Release | build_catalog_revision_qt6.log／build_catalog_revision_qt5.log，均 native exit 0；Qt6 无编译链接警告，Qt5 唯一官方 qtmain LNK4099 例外，无新增警告 |
| Python 协议／有界线程 | catalog-revision-protocol.log，15 tests／OK，0.194 秒，native exit 0；增加固定 query 分发、默认／分页参数保留和忙状态 |
| 两套正式 SDK | sdk-catalog-revision-qt6.log／sdk-catalog-revision-qt5.log，同名目录 results.json／topology.json；各 12 短场景／native exit 0，含七工具未知字段、八类目录参数类型范围拒绝、未连接查询语义、维护端未接业务时 query 拒绝；均非提权私有桌面 |
| Qt6 正式产品 | accept-catalog-revision-qt6.log 与同名目录 results.json／catalog.json／state_summary.json／topology.json；10 分项、native exit 0。runtime-catalog-qt6/QKeyMapperMcpAcceptance_catalog_qt6.exe；部署字节与本机构建一致，SHA256 0B09111591CAAC2EE35AA3882B9E00E8DEAE28D0FB0A7DFD92FB4339F601D291 |
| Qt5 正式产品 | accept-catalog-revision-qt5.log 与同名目录同类证据；10 分项、native exit 0。runtime-catalog-qt5/QKeyMapperMcpAcceptance_catalog_qt5.exe；部署字节一致，SHA256 B5CF0E5436046D5A87B512D63F2437B331AA02F597D761DE50B5F9BB0DDD5D62 |

十个产品分项在原状态／连接／撤销／冷启动流程上增加目录和跨配对上下文检查。GUI 助手从真实控件读 Tab1、空白配置显示和 L-Ctrl+F6／“映射开始”；正式 MCP 查询对应值。Tab 目录第一页 1 项、次页未附着共通表，合并与整页相同；配置目录为当前空白项和未选的全局项。多次新查询和同配对新 stdio 重连保持修订及引用稳定。改动一位的修订拒绝，明确重新配对后旧配对的修订拒绝而新修订可读，状态／dirty 不变。

正式 MCP 的 typed 参数拒绝 bool／字符串 offset、负 offset、0／101／小数 limit、非法修订和不支持 entity；原生负例独立验证 C++ 拒绝 state／capabilities 额外参数，以及 query 的未知字段、bool／小数 limit、缺少分页修订、不支持的 mappings 和错误修订。后续有效状态／目录请求仍成功，未把业务失败当作连接授权丢失。正向目录仍从正式 SDK 读取，没有使用原生负例或同一业务函数替代 GUI 基线。

实际拓扑：产品 High／RID 12288，控制器／SDK／桥接 Medium／RID 8192，same_login=true。Qt6 私有桌面 QkmProduct_c776a3cfd0e7491d97fb1467dc524383／冷启动 QkmProduct_7bb7af84453a425983612b244991facf，PID 4080／15680；Qt5 为 QkmProduct_b4ac578346b2426da0352aefb44554fe／QkmProduct_307f76b4b8a64e02a1717273f0076508，PID 31640／30140。四个 closed.json 均 exit 0；普通 CIM 最终核对自有测试进程 0，原用户 PID 5420／10:17:55 保持运行。gsudo 仅原批准启动／关闭及固定 GUI 助手；无 Default 场景、真实输入、映射启动、驱动安装或配置业务变更。

普通 PrintWindow 仍失败，stock_product_snapshots=false，未提权截图；不据此宣称 High 产品静态视觉通过。当前只验空白配置／停止态读流，GUI 实际目录变更引起失效、多个普通 Tab／用户配置、加载忙／超大响应及运行／重启状态仍待后续相应用例。映射行、实际源身份、业务修订、编辑／保存和停止完成尚未实现；Win11／异用户、性能、打包及旧 SDK 一次维护退出原因缺口保留。共享会话／控制器 C++ 未变，原生会话三组和管理窗口旧证据仅按 STATE-34 原范围复用，未重复全量 IPC、严格分析或 ASan。
