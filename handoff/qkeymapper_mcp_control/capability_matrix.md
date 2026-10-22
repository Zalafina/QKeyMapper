# 能力与 GUI／MCP 等价性矩阵

[返回唯一计划与进度](qkeymapper_mcp_automation_plan_codex.md) · [验证记录](validation.md) · [目录规则](../README.md)

## 当前状态与填写规则

日期：2026-10-22；源码基线：`d49a9f57`。**已建立逐控件入口清单，并追踪首个闭环及代表性设置的关键链路；MCP 能力全部未接入，等价性全部未执行。** 入口清单包含只读显示、编辑暂存和多个入口，不能把条目数当成业务能力数或全量链路完成数。

M0.2 必须把每个模块拆到实际设置和操作粒度，为每项能力维护稳定 ID、作用范围、类型／单位／约束、GUI 事件入口、共用业务操作、实际有效状态／缓存／通知、持久化／生效时机、依赖与风险、阶段、用例及结果。模块行中列出的文件用于导航；未逐项追踪的处理链路明确标为待核对。

同一功能存在多个 GUI 入口时，需要列出每个入口及其调用关系。测试必须比较实际数据、派生状态、联动、保存恢复及适用最终效果；不能只核对控件或将请求值原样回显。完成判据以主计划为准。

## 内部通信基础（M1.1a／b／c-1）

以下是内部组件检查，不是已对外开放的 MCP 能力。产品尚未链接这些文件；业务能力的 GUI／MCP 等价性状态仍为未执行。结果见验证记录 `IPC-20261022-07`／`11`／`12`。

| ID | 实现及源码 | 已证明范围 | 未证明范围 |
|---|---|---|---|
| INF-FRAME | [qkm_ipc_frame.cpp](../../QKeyMapper/automation/qkm_ipc_frame.cpp)：大端长度、分段读取、有界 UTF-8 JSON 对象、深度与失败终止 | Qt6.8.3／Qt5.15.2 指定边界用例通过；实际往返保留中文／日文／emoji、转义 NUL，错误不回显旧消息，超限头不分配大载荷 | 首包 schema／版本、读缓存和慢连接见 INF-LISTENER；认证后读写限额、鉴权、速率和业务原子提交由后续层实现 |
| INF-IDENTITY | [qkm_local_identity.cpp](../../QKeyMapper/automation/qkm_local_identity.cpp)：进程创建时间、主 Token SID／登录／会话及实际管道对端身份 | 两套 Qt 下同机独立进程双方查询通过；请求自报 PID 未被当作身份；合成字段变化及不可读取身份被拒绝 | 请求上下文组件见 INF-REQUEST-CONTEXT；完整集成／异常恢复、真正异用户／会话、管理员产品与普通桥接、配对／授权及撤销尚未验收；主 Token 快照不等于完整授权 |
| INF-LISTENER | [qkm_local_listener.cpp](../../QKeyMapper/automation/qkm_local_listener.cpp)：默认关闭的原生单实例管道、Qt 接管与清理、严格首包、绝对期限及请求 Token 门禁 | PC-W10 两套 Qt 普通配置各 59 通过；Qt6 故障配置 64 通过；匿名请求不得进入握手入口，在途检查全 listener 限一项，挂起检查超时／恢复失败关闭端点，旧结果不放行重启实例；见 IPC-20261022-11／13 | 无产品监听／凭据／ready／业务分发；配对／证明／撤销及认证后限额待 M1.1c；不等于 MCP 业务能力或 GUI 等价性 |
| INF-REQUEST-CONTEXT | [qkm_local_identity.cpp](../../QKeyMapper/automation/qkm_local_identity.cpp)：独立原生线程检查已读管道请求 Token，退出后回所属 Qt 线程报告 | 普通／匿名 SQOS、进程／会话绑定、析构回收通过；已接 listener；专用配置故障注入证明恢复失败、长期挂起及关闭交错不会认证或累积替代线程；见 IPC-20261022-12／13 | 故障注入不等于真实 OS API 故障；真正异用户／会话、管理员产品／普通桥接及完整鉴权尚未验收；不能当作业务权限 |

## 初始模块索引

下表是模块范围导航；各模块的 MCP 实现和等价性验收统一为 **未实现／未执行**。本轮已追踪的部分见后续关键链路表，其余逐项契约仍待核对。拟定用例 ID 是后续测试目标，不表示已存在测试脚本。

| ID | 能力范围 | GUI／源码导航 | 共用链路与有效状态重点 | 阶段／拟定用例 |
|---|---|---|---|---|
| CAP-001 | 能力、实例、运行状态及帮助 | [主窗口头文件](../../QKeyMapper/qkeymapper.h)、[主程序入口](../../QKeyMapper/main.cpp) | 当前配置、真实运行状态、停止快捷键、依赖与端点身份；接口待设计 | M1／API-STATUS-001 |
| MAP-001 | 普通映射新增／修改／删除 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)，`on_addmapdataButton_clicked()`；[映射项窗口](../../QKeyMapper/qitemsetupdialog.cpp) | 校验、数据来源、派生字段、冲突、刷新和 dirty；按操作拆行 | M1／EQ-MAP-001 |
| MAP-002 | 连发、锁定、长按／双击、时机、发送方式、抬起键 | [映射项窗口](../../QKeyMapper/qitemsetupdialog.cpp)、[映射引擎](../../QKeyMapper/qkeymapper_worker.cpp) | 所有参数与相互限制、实际有效数据、Worker 消费值、释放与保存 | M2-A／EQ-ITEM-001 |
| MAP-003 | 映射码、浮动按钮样式码 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)，`applyMappingCodeToSourceRow()` | 解析不等于完整业务提交；运行状态、禁用保留、冲突及忽略字段 | M2-A／EQ-CODE-001 |
| MAP-004 | 表格选择、分类／筛选、复制／移动／排序 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp) | 显示行与源数据、隐藏项、选区、插入模式、dirty；具体入口待盘点 | M2-A／EQ-TABLE-001 |
| MAP-005 | Tab、共通映射、属性及 Tab 快捷键 | [Tab 设定](../../QKeyMapper/qtablesetupdialog.cpp)、[qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp) | 自有／共通来源、互斥及优先级、运行表、跨 Tab 影响 | M2-A／EQ-COMMON-001 |
| MAC-001 | 普通宏、通用宏、分类、重命名／覆盖及导入导出 | [宏列表](../../QKeyMapper/qmacrolistdialog.cpp) | 实际有序宏表、引用解析、范围、依赖与覆盖选择；逐动作盘点 | M2-A／EQ-MACRO-001 |
| CFG-001 | 保存、另存、加载及配置切换 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)，`saveKeyMapSetting()`／`loadKeyMapSetting()` | 草稿、同名覆盖、加载副作用、sync／status、冷启动及失败保护 | M1／EQ-SAVE-001 |
| CFG-002 | 配置／单表导入导出、备份及恢复 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)、[Tab 设定](../../QKeyMapper/qtablesetupdialog.cpp) | 内存与磁盘来源、全局组范围、覆盖、校验和已有草稿 | M2-A／EQ-TRANSFER-001 |
| WIN-001 | 窗口条件、进程信息、自动匹配与回退 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp) | 实际匹配条件、前台目标、定时器／事件、加载或切换影响 | M2-B／EQ-MATCH-001 |
| WIN-002 | 忽略窗口规则、恢复默认及正则条件 | [忽略规则窗口](../../QKeyMapper/qignorewindowinfolistdialog.cpp) | 条件、禁用与全局范围、匹配缓存和保存；入口待逐项核对 | M2-B／EQ-IGNORE-001 |
| SET-001 | 语言 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)，`on_languageComboBox_currentIndexChanged()` | 语言重载、字体、进程表及列宽；不是仅改变选择索引 | M2-B／EQ-LANGUAGE-001 |
| SET-002 | 主题及显示相关设置 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)，`on_themeComboBox_currentIndexChanged()` | `setUITheme()`、Tab 显示及全表刷新，逐显示选项待盘点 | M2-B／EQ-THEME-001 |
| SET-003 | 实时缩放及启动缩放 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)，`initScaleMenu()`／`onScaleComboBoxActivated()` | `activated` 与索引变化区别；实际目标比例、补偿、布局、dirty、重载 | M2-B／EQ-SCALE-001 |
| SET-004 | 启动、托盘、窗口尺寸／位置及通用高级 | [通用高级窗口](../../QKeyMapper/qgeneraladvanceddialog.cpp)、[qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp) | 运行值、即时／保存／重启时机、系统启动项副作用 | M2-B／EQ-GENERAL-001 |
| SET-005 | 开始／停止快捷键、系统筛选键及映射高级 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)、[映射高级](../../QKeyMapper/qmappingadvanceddialog.cpp) | 停止入口、系统筛选键会话恢复、驱动筛选的区别；高风险参数逐项列出 | M2-B／EQ-FILTER-001 |
| UI-001 | 浮动按钮、样式及同步组 | [浮动按钮设定](../../QKeyMapper/qfloatingbuttonsetupdialog.cpp)、[qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp) | 应用／恢复、三态外观、派生布局、组状态及保存 | M2-B／EQ-FBUTTON-001 |
| UI-002 | 悬浮窗口、Tab 图片及虚拟按钮面板 | [悬浮窗口](../../QKeyMapper/qfloatingwindowsetupdialog.cpp)、[面板设定](../../QKeyMapper/qvbuttonpanelsetupdialog.cpp) | 显示对象、位置／参考点、鼠标穿透、刷新及配置；拆为独立能力 | M2-B／EQ-PANEL-001 |
| UI-003 | 准星、通知／自定义通知及托盘图标 | [准星](../../QKeyMapper/qcrosshairsetupdialog.cpp)、[通知](../../QKeyMapper/qnotificationsetupdialog.cpp)、[托盘图标](../../QKeyMapper/qtrayiconselectdialog.cpp) | 编辑值与实际叠加／提示／托盘行为、文件依赖及生效时机 | M2-B／EQ-OVERLAY-001 |
| UI-004 | 页面、编辑窗、最小化／显示及必要窗口操作 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)、各相关对话框 | 与常规 GUI 相同限制；实际显示、焦点影响及未提交编辑内容 | M2-B／EQ-UISESSION-001 |
| INPUT-001 | 原始键／宏录制、序列编辑及坐标采集 | [录制](../../QKeyMapper/qkeyrecord.cpp)、[序列编辑](../../QKeyMapper/qmappingsequenceedit.cpp)、[坐标采集](../../QKeyMapper/qpointpickerdialog.cpp) | 实际采集、编辑草稿、确认／取消、上层更新及真实输入边界 | M2-A／M2-B／EQ-EDITOR-001 |
| RUN-001 | 开始、停止、切表、任务释放及等待 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)，`handleManualMappingSwitchRequest()`／`setKeyUnHook()`；[Worker](../../QKeyMapper/qkeymapper_worker.cpp) | 切换与明确命令区别、停止发送动作、GUI 与 Worker 完成、释放任务 | M1／EQ-RUN-001 |
| DEV-001 | Interception、多键鼠、设备选择／禁用、驱动筛选 | [设备列表](../../QKeyMapper/qinputdevicelistwindow.cpp)、[Interception](../../QKeyMapper/interception_worker.cpp) | 设备身份／编号、禁用范围、依赖及严重输入失效风险；不自动安装 | M3-A／EQ-INTERCEPTION-001 |
| DEV-002 | FakerInput、ViGEm、虚拟手柄及发送后端 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)、[Worker](../../QKeyMapper/qkeymapper_worker.cpp) | 配置与实际连接／设备状态、创建／释放、权限及重启要求 | M3-A／EQ-VDEVICE-001 |
| DEV-003 | 物理手柄、陀螺仪、触摸板及直连参数 | [陀螺仪选项](../../QKeyMapper/qgyro2mouseoptiondialog.cpp)、[Worker](../../QKeyMapper/qkeymapper_worker.cpp) | 来源、玩家、方向、阈值、有效参数与实际设备能力 | M3-A／EQ-GAMEPAD-001 |
| GAME-001 | 极限竞速遥测及自动映射 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)、[Worker](../../QKeyMapper/qkeymapper_worker.cpp) | 地址／端口、阈值、映射内参数与页面参数、遥测前提和实际目标输出 | M3-A／EQ-FORZA-001 |
| SYS-001 | 启动项、更新、驱动安装／卸载及其他系统动作 | [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)、[更新模块](../../QKeyMapper/QSimpleUpdater/src/QSimpleUpdater.cpp) | 具体对象、系统副作用、重启及结果未知；本轮不执行 | M3-A／M3-B／AUTH-SYSTEM-001 |
| OBS-001 | 操作完成、运行观测与崩溃／卡死证据 | [现有诊断](../../QKeyMapper/diagnostics/README.md)、[维护测试](../../QKeyMapper/tests/README.md) | 相关操作及事件顺序、有界留存、事件丢失、外部诊断可用性 | M1／M3-A／OBS-001 |

盘点参考：[产品手册](../../readme/QKeyMapper使用手册.html)、[手册功能覆盖表](../../readme/manual/coverage.md)、[主窗口 UI](../../QKeyMapper/qkeymapper.ui)、[常量](../../QKeyMapper/qkeymapper_constants.h)。手册旧验证及截图不是本任务当前运行证据。

## 逐能力验收记录格式

M0.2 按稳定能力 ID 扩充以下信息，不重复主计划的任务进度：

| 字段 | 要求 |
|---|---|
| GUI 操作与参数 | 鼠标／键盘／菜单入口；作用域、前置配置、字段类型、单位、约束和默认值 |
| 现有处理链路 | 确认实际调用者、校验、依赖、联动及返回处理，不凭函数名假定安全 |
| 共用操作 | 现有可复用函数，或最小必要提取；明确业务提交与界面回显边界 |
| 有效状态及副作用 | 主数据、派生字段、缓存、Worker／系统接收值、异步完成条件及通知；必要处理不漏执行、不重复执行 |
| 生效及持久化 | 当前内存／即时持久化／保存／下一次开始／重启及取消、失败、恢复规则 |
| 风险与授权 | 停止要求、依赖、实际停止快捷键、外部或系统副作用、恢复边界 |
| 独立用例 | GUI 基线、正式 MCP 路径、相同期望；实际数据、状态、保存恢复和最终效果断言 |
| 分系统结果 | Win10 与 Win11、Qt／构建／源码、通过／失败／未执行、验证记录 ID及必要证据 |

当前没有任何能力达到“已实现”或“等价性通过”。下列静态结论用于决定提取位置及验收要求，不替代运行证据；未追踪的参数、持久化分支及动态入口仍须补齐。

## M0.2 已核对的关键业务链路

本节 ID 在实现时保留。源码位置以函数名定位，括号行号仅用于 `d49a9f57` 导航。拟定测试 ID 的 GUI 与 MCP 驱动分别使用 `-G`／`-M` 后缀；目前均未建立运行结果。

| ID／阶段 | GUI 入口与已确认链路 | 有效状态、持久化与风险 | 最小共用操作及独立验收要求 |
|---|---|---|---|
| MAP-ADD／M1 | 主窗口添加按钮 `on_addmapdataButton_clicked`（39033）读取录入键／列表、设备及按压选项；`validateOriginalKeyString`、`validateMappingKeyString`；互斥／共通冲突；`createDefaultKeyMappingData` → `insertNewMappingData` | 新项初始化完整 `MAP_KEYDATA`，同原始键可能自动禁用；新增共通项可禁用其他 Tab 的行。插入位置受 Shift、选区和全局插入模式影响；刷新相关 Tab 并 dirty | 将已采集的明确原始键、映射键、源 Tab、插入位置及冲突选择送入同一提交操作；GUI 保留采集逻辑。不得调用按钮槽后依赖物理修饰键。`EQ-MAP-ADD` 比较默认字段、禁用／联动、源表、位置、草稿、保存恢复及 A→B 按下／释放 |
| MAP-EDIT／M1 | 映射项窗口更新按钮／各字段更新进入 `updateMappingInfoByOrder`（3174）；按焦点／明确顺序正规化并校验三种键串；`updateAllMappingInfoFinally`（3468）最终写入 | 原始键冲突可能取消或禁用其他行；主／抬起键更新同时维护 `Pure_MappingKeys*`、序列拆分及备注对齐；空抬起键回退主映射；刷新相关表和编辑窗并 dirty。代码含对当前全局 `KeyMappingDataList` 的依赖 | 提取接受源表、源行、明确字段与验证顺序的提交操作，保留未指定字段和备注对齐；移除操作目标对当前 UI 的隐含依赖。`EQ-MAP-EDIT` 覆盖非当前源表、同时改原始／映射键、抬起键回退、取消冲突和已有编辑暂存 |
| MAP-DELETE／M1 | 表格 Delete／右键删除 → `deleteSelectedMappingData`（40848）；收集可见选中行并转 `DisplayRowSourceInfo`；拒绝跨普通／共通源表混合批次 | 按源表、去重后的源行逆序删除；`refreshTabsForSourceTabChange`、`updateMousePointsList`、重选最近可见行、dirty；显示行号不能作数据身份 | GUI 采集选区后调用明确源行批量删除；MCP 按查询得到的源引用操作，同样处理混合来源限制。`EQ-MAP-DELETE` 覆盖筛选、分隔行、共通来源、重复引用、最后一行和未提交编辑窗 |
| MAP-CODE／M2-A | 右键／映射项窗口粘贴映射码 → `applyMappingCodeToSourceRow`（10694）→ `parseAndApplyMappingCode`（9751） | 解析器允许忽略未知字段并报告部分解析，不能直接作为严格 MCP 写入口；不等价于正常键串校验；保留 Disabled、更新部分派生键、冲突联动、刷新及 dirty；函数本身没有完整停机守卫 | 正式 MCP 类型边界先拒绝未知字段；映射码导入仍复用与 GUI 一致的业务提交。不能把放宽的解析成功冒充参数全部生效。`EQ-MAP-CODE` 断言被忽略内容、范围校验、冲突及运行中拒绝 |
| CFG-SAVE／M1 | 保存按钮 → `saveKeyMapSetting`（21062）；同步运行态；校验名称／覆盖；收集映射及各设置窗口 getter；`QSettings::sync/status`（22686）→ `loadKeyMapSetting` 重载 | 保存范围不限单条映射；可能包括全局设置及宏。失败恢复本次共享 pending cache；成功仍需确认重载结果。重载会重建 Tab／表格并改变引用；同进程 QSettings 读回不能证明磁盘和冷启动 | 从名称／覆盖交互中提取明确保存决策，沿用序列化及失败分支，报告“内存已修改／磁盘已保存／重载已完成”各自结果。`EQ-CFG-SAVE` 比较全作用域、失败草稿保护、真正新进程恢复、Qt5 编码及保存后引用失效 |
| RUN-START／M1 | 开始按钮下拉、托盘及快捷键 → `handleManualMappingSwitchRequest`（16659）→ `MappingSwitch`（16725） | 开始可能先切离共通 Tab；按钮模式／物理 Ctrl 会决定是否先保存。进入 `KEYMAP_CHECKING` 后启动窗口匹配及事件 Hook；监视已开始不代表目标窗口已匹配或 Worker 已启用 | 共用明确的开始操作，GUI 另行采集保存意图／Ctrl；MCP 提供显式保存策略及目标，不能直接盲调 toggle。`EQ-RUN-START` 比较监视、实际匹配、Worker 生效、配置／Tab 与保存影响；重复开始不停止 |
| RUN-STOP／M1 | 相同 GUI 入口停止 → `MappingSwitch` → `setKeyUnHook`（5369）向 Hook／Worker 发排队信号；GUI 随后设 IDLE | Worker `setWorkerKeyUnHook`（9942）释放、清理控制器、重置设备、执行 `emitSendOnMappingStopKeys`（10131），最后设置 Hook 原子状态 STOPPED（10145）；部分控制器清理和停止发送任务仍可能异步。停止还可能回切配置 | 共用明确停止请求，增加可证实的清理完成边界；GUI IDLE 或 Hook STOPPED 单一指标均不足以放行写入。`EQ-RUN-STOP` 覆盖按键未释放、停止发送、延迟任务、重复停止、超时后禁止修改及回切后目标重新核对 |
| STATE-SOURCE／M1 | 普通／共通显示使用 `DisplayRowSourceInfo`，运行时使用合成的 `s_ActiveKeyMappingDataList` | 显示行包含筛选及共通分隔，运行数据不等于正在编辑的自有表；配置加载／保存会重建源表。设备枚举、目标 HWND 等动态数据另属运行上下文 | 查询必须区分源数据、合成有效配置和运行状态，不读取屏幕文字当真值；查询引用只在对应实例和配置快照有效。`API-STATE-SOURCE` 检查共通重复显示、非当前 Tab、重载及 stale 引用 |
| SET-SCALE／M2-B | `scaleComboBox::activated`（3768）、缩放菜单 → `onScaleComboBoxActivated`（36530） | 运行中拒绝；调用实际缩放、字体／几何与菜单刷新并 dirty。`setCurrentIndex` 不会发 activated；加载恢复有独立路径 | 可复用处理函数但先提取返回结果／错误的明确业务操作；只在回显时屏蔽重复信号。`EQ-SET-SCALE` 比较实际缩放比例、主窗及已打开编辑窗尺寸、保存／冷启动和运行中拒绝 |
| SET-GENERAL／M2-B | 通用高级窗口手动连接 valueChanged／toggled；启动位置联动 `updateStartupSpecifyPositionState`，全屏规则联动 `updateGlobalMappingFullscreenState`；文本用 `textEdited` | 部分设置直接以控件作为主存储，由 `getStartupPosition`、`getSaveWindowSize`、`getTableInsertMode` 等 getter 被保存／运行逻辑消费；`setText` 不触发 textEdited／dirty。启动位置为下一次启动语义 | 不强迫全局改成新模型；逐项保留既有 getter 与完整 setter／联动，业务提交显式 dirty。`EQ-SET-GENERAL` 对照位置模式／指定坐标、插入模式、全屏白名单实际匹配、重启及保存范围 |
| SET-GYRO／M2-B | 陀螺仪高级的两个来源、两个反转信号 → `settingsChanged` → 主窗口 `requestSaveSettingDirty`（37159）；Worker 在 `Gyro2Mouse` 相关处理（13057）直接读取四个 getter | 保存四个 `GYRO2MOUSE_*` 字段（22426）；真实运行消费值来自对话框，不是独立缓存。跨线程访问和一致快照边界仍需实现前专门核对，不在此静态盘点中宣称安全 | 先确认既有读取边界，再共用明确的四字段设置操作；模拟输入轨迹与真实手柄证据分开。`EQ-SET-GYRO` 检查来源枚举、反向、保存恢复与同轨迹输出 |
| SET-THRESHOLD／M2-B | 映射高级 `initGamepadThresholdSpinBoxes`（212）连接 LT／RT／LS／RS 每组 Release、Light、Press 三向区间联动及 dirty | 每组须 Release ≤ Light ≤ Press。按任意顺序 setValue 可能被旧 min/max 静默夹紧，造成最终值与请求不同；当前有效运行消费路径、四组保存字段仍待细化 | 预览校验完整目标组，以同一业务操作协调最终值及允许范围；禁止成功回显请求但实际被夹紧。`EQ-SET-THRESHOLD` 覆盖成组上调／下调、非法顺序、边界、getter 和 Worker 的实际值 |
| UI-NOTIFY／M2-B | 通知设定普通／自定义两个实例共用 `QNotificationSetupDialog`；颜色、字体、时间、偏移等手动连接 dirty；字体通过 `m_FontFamily` 表示自定义／默认 | 字体列表当前显示不等于实际字体来源；默认按钮 clear 成员后 `syncFontFamilyControls`。普通／自定义设置不可因同类／同 objectName 混淆；显示消费者和保存具体字段待逐项补齐 | 操作明确目标实例、字体来源与各字段；复用现有字体同步逻辑。`EQ-UI-NOTIFY` 核对实际通知、默认继承／自定义、透明度／持续时间、双实例隔离及冷启动 |
| UI-PANEL／M2-B | `QVButtonPanelSetupDialog::onAnyControlChanged`（311）→ `settingsApplied` → `onVButtonPanelSettingsAccepted`（46583）；恢复按钮重新 load backup 并应用 | 更新 `m_VButtonPanelSettings`、实际面板布局／位置／颜色、Worker 的 VButton 列表和 defaultShow；保留可见性、同步面板、dirty。仅 loadSettings 的 loading 保护会跳过应用信号 | 共用“设置值＋完整应用”的明确操作；不得停在 loadSettings。`EQ-UI-PANEL` 比较面板真实窗口、按钮列表、Worker 默认显示、备份恢复及保存 |
| UI-FLOAT／M2-B | C++ 构造控件；`onAnyControlChanged`／Apply／Revert 更新目标映射项并发 `settingsApplied`；主窗排队接收 `onFloatingButtonSettingsApplied`（47117） | 依源 Tab／行同步运行数据、刷新表格／浮动列、实际显示及 dirty；同步组偏移为增量操作，可影响其他 Tab 的行，不能逐字段机械调用导致重复累计 | 先追踪应用与组移动的完整路径，再提取明确目标设置；完成需要等待必要排队回调。`EQ-UI-FLOAT` 比较组成员／增量、源表及真实窗口；静态图不能替代拖动验证 |
| UI-TRAY／M2-B | 四个图标组合框只在 `activated` 手动通知 dirty；外部 ICO 导入／删除为额外动作 | 程序化 index setter 不等价于用户选择；图标文件来源、实际系统托盘刷新与保存时机仍待细化；不得把只读图标列表当已应用 | 复用完整选择动作，并对外部文件作显式授权和错误报告。`EQ-UI-TRAY` 比较当前真实状态图标、默认恢复、文件失败及新进程恢复；需 Default 桌面时另记范围 |

### 动态菜单与事件入口补充

对主窗口、宏列表、序列编辑、Tab 设定、VButton 面板及忽略规则源码进行 `addAction` 静态搜索得到 136 处匹配，其中包含重复入口、动作组、注释或辅助加入操作；**不是 136 个独立能力**。以下按用户意图归并，全部尚待逐操作完整链路核对；不能只扫描 `.ui` 就声明全覆盖。

| 稳定 ID | 实际入口／用户意图 | 源码锚点／阶段／验收关注 |
|---|---|---|
| ACT-TAB-ADD | 菜单、快捷键、表格右键添加空白 Tab | 主窗口构造及表格 `contextMenuEvent`；M2-A；命名、默认字段、dirty 与源引用失效 |
| ACT-TAB-COPY | 复制当前 Tab | 同上；M2-A；有序数据、共通类型和宏引用 |
| ACT-TAB-DELETE | 删除当前／指定 Tab | `deleteMappingTableByTabIndex`；M2-A；最后一表、共通、取消和当前表切换 |
| ACT-TABLE-CLEAR | 清空当前表 | `clearCurrentMappingTable`；M2-A；确认、来源范围及已有草稿 |
| ACT-TABLE-EXPORT | 导出映射表 | `exportMappingTableByTabIndex`／`exportKeyMappingDataToFile`；M2-A；导出内存还是磁盘、覆盖与文件权限 |
| ACT-TABLE-IMPORT | 导入映射表 | `importMappingTableByTabIndex`／`importKeyMappingDataFromFile`；M2-A；插入位置、校验、互斥联动、宏及失败保护 |
| ACT-VIEW-PROCESS | 显示／隐藏进程列表 | `m_ActionShowProcessList`；M2-B；真实布局、尺寸和保存范围 |
| ACT-VIEW-POINT | 显示／隐藏坐标拾取 | `m_ActionShowPointPicker`；M2-B／M3-A；实际采集和真实输入权限 |
| ACT-VIEW-CATEGORY | 显示／隐藏分类列 | `setCategoryColumnVisible`；M2-A；筛选、列宽及草稿 |
| ACT-VIEW-NOTES | 显示／隐藏备注列 | `m_ActionShowNotes` 及表格右键；M2-A；列显示和保存 |
| ACT-VIEW-FLOATING | 显示／隐藏浮动列 | `m_ActionShowFloating`；M2-B；实际浮动按钮与表格显示的区别 |
| ACT-VIEW-DISABLED | 显示／隐藏禁用行 | `m_ActionHideDisabled`；M2-A；动作名称与实际布尔语义、选区恢复 |
| ACT-CATEGORY-FILTER | 分类多选／全部／空白分类 | `initCategoryFilterControls`／`showCategoryFilterPopup`；M2-A；视图筛选与业务数据分离 |
| ACT-CATEGORY-ASSIGN | 选定分类／输入新分类／批量分类 | 表格 `contextMenuEvent` 的内嵌文本和 QWidgetAction；M2-A；可见行转源行、混合来源限制 |
| ACT-MAP-STATE | 单项／批量启用禁用、连发开关、锁定开关、浮动按钮开关 | 表格状态菜单与 `keyMappingTableItemCheckStateChanged`；M2-A／M2-B；与映射项窗口逐项等价 |
| ACT-MAP-TRIGGER | 普通／长按／双击及时间的单项／批量更改 | 表格触发类型菜单、内嵌 combo／spin；M2-A；完整原始键解析与互斥组变化 |
| ACT-MAP-ORDER | 上移、下移、置顶、置底及拖拽 | `selectedItemsMove*`／`keyMappingTableDragDropMove`；M2-A；隐藏行、源行和共通分隔 |
| ACT-MAP-COPYPASTE | 复制／向顶部、尾部、上方、下方插入 | 表格 `contextMenuEvent`；M2-A；剪贴板、内部拷贝缓冲、位置与批量校验 |
| ACT-MAP-CODE | 复制映射码／应用剪贴板映射码 | 表格／映射项窗口；M2-A；字符串解析与业务提交不能混同 |
| ACT-MAP-EDIT | 双击／右键编辑、打开单项／表格／浮动按钮设定 | 表格 `contextMenuEvent` 和事件处理；M1／M2；焦点、未提交编辑与明确源目标 |
| ACT-COMMON-JUMP | 从显示的共通映射跳转其真实源表 | 表格 `jumpToCommonAction`；M2-A；不把显示位置作为来源身份 |
| ACT-KEYLIST-COLLECTION | 键名复制／追加、收藏添加删除、收藏管理／清空、最近项目清空／搜索／返回 | `KeyListComboBoxPopup::show*Menu` 及构造；M2-B；作用范围、立即持久化与隐私 |
| ACT-CONFIG-ORDER | 配置上移、下移、置顶、置底、删除、排序模式／搜索 | `SettingSelectComboBoxPopup::showMainListMenu` 及构造；M2-B；列表显示名与配置真实名称、草稿及持久化 |
| ACT-CONFIG-ICON | 选择配置自定义图标／恢复默认 | `showSelectSettingCustomIconButtonContextMenu`；M2-B；外部文件、图标缓存和保存范围 |
| ACT-TAB-IMAGE | 选择 Tab 图片／恢复默认 | `QTableSetupDialog::showSelectCustomImageButtonContextMenu`；M2-B；路径、图像加载、实际显示 |
| ACT-START-MODE | 仅开始／保存并开始模式选择 | `initMappingStartActionMenu`；M1／M2-B；模式设置与一次开始意图、Ctrl 覆写区别 |
| ACT-SCALE-MENU | 菜单中的各缩放选项 | `initScaleMenu`；M2-B；必须汇入 SET-SCALE 真实处理 |
| ACT-TRAY | 托盘开始停止／显示隐藏／退出 | `initSysTrayIcon` 及各 handler；M1／M2-B；复用明确状态语义，退出不作为强制结束接口 |
| ACT-FLOAT-STYLE | 浮动按钮设定、复制样式码、应用剪贴板样式、保存 | 主窗口 `eventFilter`（15936）；M2-B；源行、组联动、授权及保存 |
| ACT-FLOAT-MOVE | 浮动按钮移动／同步组移动 | 同上；M2-B／M3-A；实际拖动与偏移、输入范围需单列 |
| ACT-FLOAT-FLAGS | 穿透／置顶／提示开关、全局拖动坐标开关 | 同上；M2-B；瞬时窗口属性、映射行和全局字段的不同范围 |
| ACT-FLOAT-VISIBILITY | 隐藏单个／全部、显示全部浮动按钮 | 同上；M2-B；手工隐藏标记与自动开始／停止显示规则 |
| ACT-PANEL | VButton 面板设定／保存／移动 | `QVButtonPanel::showPanelContextMenu`；M2-B；运行面板状态与保存分离 |
| ACT-MACRO | 普通／通用宏编辑、载入、序列编辑；排序／复制／四种插入／删除；宏表导入导出及分类筛选 | `MacroListDataTableWidget::contextMenuEvent`、`QMacroListDialog`；M2-A；同名覆盖、范围、引用及递归风险，每项还须拆出正式命令 |
| ACT-SEQUENCE | 序列编辑／载入／排序／复制／替换／四种插入／四种空白插入／删除／撤销重做／确认取消 | `MappingSequenceEditTableWidget::contextMenuEvent`、`QMappingSequenceEdit`；M2-A；编辑草稿与最终提交、分段备注 |
| ACT-PROCESS-COPY | 进程列表信息复制和选中目标载入窗口条件 | `ProcessInfoTableWidget::contextMenuEvent` 及主窗点击事件；M2-B；快照与窗口销毁竞态 |
| ACT-COLOR-RESTORE | 颜色选择器右键恢复默认 | `ColorPickerWidget`；M2-B；预览／提交／取消和消费者联动 |
| ACT-CONFIG-TRANSFER | 配置导入／导出文件选择、配置组多选及确认／取消 | `SettingTransferDialog`／`GroupSelectionWidget`；M2-A；路径框只读、导出来自磁盘 INI、覆盖／部分失败及组范围需逐项追踪 |

后续还须对照键盘快捷键、拖拽、表格直接编辑、首选项即时保存、状态／设备按钮及手册覆盖表补漏。宏／序列行暂以动作族归并，不计作逐动作契约完成。原始键／宏录制、真实设备枚举与运行期 DSL 的完整动作域均属于最终覆盖范围，不能用 GUI 控件扫描代替。

## 逐控件候选入口清单（源码静态快照）

扫描范围：根目录下 16 份 `.ui`。原先按常见输入控件得到 294 项；本次另补 12 个颜色控件和 10 个列表／表格／Tab 等交互容器，共 316 个 UI 定义入口。另列 C++ 构造的动态控件。它们是覆盖核对待办，不是正式 MCP 能力，也不是业务链路已完成。

稳定定位 ID 的前缀表示界面归属，后缀沿用 objectName／成员名。显示名可随语言变化，ID 不依赖翻译和行号。一个业务能力可以有多个入口；一个设置也可能需要几个控件共同提交。普通／自定义通知复用同一个 UI 定义，正式命令另带明确实例作用域。

“绑定线索”来自自动槽命名和手动 connect 静态定位，可能同时包含调用位置、兼容分支或停用代码，需结合现有信号、编译条件及调用者核对；未找到线索不代表没有实现。XML 原始值可能被构造代码、加载或联动覆盖，不得直接发布为 API 参数约束。只读状态也可能在 C++ 动态设置。所有未在前述关键链路覆盖的项目，其业务提交、有效状态、持久化和用例仍为待追踪。

### crosshair（14 项）

定义：[qcrosshairsetupdialog.ui](../../QKeyMapper/qcrosshairsetupdialog.ui)；绑定：[qcrosshairsetupdialog.cpp](../../QKeyMapper/qcrosshairsetupdialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-crosshair/centerColorPicker` | ColorPickerWidget | `connect colorChanged @56` | 运行约束待核 |
| `U-crosshair/centerSizeSpinBox` | QSpinBox | `on_centerSizeSpinBox_editingFinished @309` | 运行约束待核 |
| `U-crosshair/centerOpacitySpinBox` | QSpinBox | `on_centerOpacitySpinBox_editingFinished @326` | 运行约束待核 |
| `U-crosshair/showCenterCheckBox` | QCheckBox；Show Center | `on_showCenterCheckBox_stateChanged @224` | 运行约束待核 |
| `U-crosshair/crosshairColorPicker` | ColorPickerWidget | `connect colorChanged @57` | 运行约束待核 |
| `U-crosshair/showTopCheckBox` | QCheckBox；Show Top | `on_showTopCheckBox_stateChanged @241` | 运行约束待核 |
| `U-crosshair/crosshairLineWidthSpinBox` | QSpinBox | `on_crosshairLineWidthSpinBox_editingFinished @343` | 运行约束待核 |
| `U-crosshair/showBottomCheckBox` | QCheckBox；Show Bottom | `on_showBottomCheckBox_stateChanged @258` | 运行约束待核 |
| `U-crosshair/crosshairLineLengthSpinBox` | QSpinBox | `on_crosshairLineLengthSpinBox_editingFinished @360` | 运行约束待核 |
| `U-crosshair/showLeftCheckBox` | QCheckBox；Show Left | `on_showLeftCheckBox_stateChanged @275` | 运行约束待核 |
| `U-crosshair/crosshairOpacitySpinBox` | QSpinBox | `on_crosshairOpacitySpinBox_editingFinished @377` | 运行约束待核 |
| `U-crosshair/showRightCheckBox` | QCheckBox；Show Right | `on_showRightCheckBox_stateChanged @292` | 运行约束待核 |
| `U-crosshair/x_offsetSpinBox` | QSpinBox | `on_x_offsetSpinBox_editingFinished @394` | 运行约束待核 |
| `U-crosshair/y_offsetSpinBox` | QSpinBox | `on_y_offsetSpinBox_editingFinished @411` | 运行约束待核 |

### floating_window（9 项）

定义：[qfloatingwindowsetupdialog.ui](../../QKeyMapper/qfloatingwindowsetupdialog.ui)；绑定：[qfloatingwindowsetupdialog.cpp](../../QKeyMapper/qfloatingwindowsetupdialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-floating_window/floatingWindow_BGColorPicker` | ColorPickerWidget | `connect colorChanged @86` | 运行约束待核 |
| `U-floating_window/windowSizeSpinBox` | QSpinBox | `on_windowSizeSpinBox_valueChanged @249` | 运行约束待核 |
| `U-floating_window/windowRadiusSpinBox` | QSpinBox | `on_windowRadiusSpinBox_valueChanged @350` | 运行约束待核 |
| `U-floating_window/windowOpacitySpinBox` | QDoubleSpinBox | `on_windowOpacitySpinBox_valueChanged @310` | 运行约束待核 |
| `U-floating_window/referencePointComboBox` | QComboBox | `on_referencePointComboBox_currentIndexChanged @369` | 运行约束待核 |
| `U-floating_window/windowPositionXSpinBox` | QSpinBox | `on_windowPositionXSpinBox_valueChanged @270` | 运行约束待核 |
| `U-floating_window/windowPositionYSpinBox` | QSpinBox | `on_windowPositionYSpinBox_valueChanged @290` | 运行约束待核 |
| `U-floating_window/mousePassThroughCheckBox` | QCheckBox；MousePassThrough | `on_mousePassThroughCheckBox_stateChanged @330` | 运行约束待核 |
| `U-floating_window/mousePassThroughSwitchKeyComboBox` | QComboBox | `on_mousePassThroughSwitchKeyComboBox_currentIndexChanged @388` | 运行约束待核 |

### general（11 项）

定义：[qgeneraladvanceddialog.ui](../../QKeyMapper/qgeneraladvanceddialog.ui)；绑定：[qgeneraladvanceddialog.cpp](../../QKeyMapper/qgeneraladvanceddialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-general/startupPositionComboBox` | QComboBox | `connect currentIndexChanged @75` | 运行约束待核 |
| `U-general/startupSpecifyPositionXSpinBox` | QSpinBox | `connect valueChanged @85` | 运行约束待核 |
| `U-general/startupSpecifyPositionYSpinBox` | QSpinBox | `connect valueChanged @87` | 运行约束待核 |
| `U-general/saveWindowSizeCheckBox` | QCheckBox；Save Window Size | `connect toggled @95` | 运行约束待核 |
| `U-general/saveSplitterPositionCheckBox` | QCheckBox；Save Window Splitter Position | `connect toggled @97` | 运行约束待核 |
| `U-general/tableEditModeTriggerComboBox` | QComboBox | `connect currentIndexChanged @89` | 运行约束待核 |
| `U-general/tableInsertModeComboBox` | QComboBox | `connect currentIndexChanged @91` | 运行约束待核 |
| `U-general/disableGlobalMappingInFullscreenCheckBox` | QCheckBox；Disable GlobalKeyMapping in Fullscreen | `connect toggled @80` | 运行约束待核 |
| `U-general/fullscreenGlobalMappingProcessLineEdit` | QLineEdit | `connect textEdited @93` | 运行约束待核 |
| `U-general/soundEffectCheckBox` | QCheckBox；Notification Sound | `connect toggled @99` | 运行约束待核 |
| `U-general/globalSettingSwitchTimerSpinBox` | QSpinBox | `connect valueChanged @101` | 运行约束待核 |

### gyro（4 项）

定义：[qgyro2mouseoptiondialog.ui](../../QKeyMapper/qgyro2mouseoptiondialog.ui)；绑定：[qgyro2mouseoptiondialog.cpp](../../QKeyMapper/qgyro2mouseoptiondialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-gyro/mouseXInputSourceComboBox` | QComboBox | `connect currentIndexChanged @40` | 运行约束待核 |
| `U-gyro/mouseXRevertCheckBox` | QCheckBox；X Revert | `connect toggled @52` | 运行约束待核 |
| `U-gyro/mouseYInputSourceComboBox` | QComboBox | `connect currentIndexChanged @46` | 运行约束待核 |
| `U-gyro/mouseYRevertCheckBox` | QCheckBox；Y Revert | `connect toggled @55` | 运行约束待核 |

### ignore_rules（14 项）

定义：[qignorewindowinfolistdialog.ui](../../QKeyMapper/qignorewindowinfolistdialog.ui)；绑定：[qignorewindowinfolistdialog.cpp](../../QKeyMapper/qignorewindowinfolistdialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-ignore_rules/ruleNameLineEdit` | QLineEdit | `on_ruleNameLineEdit_textChanged @358` | 运行约束待核 |
| `U-ignore_rules/saveRuleButton` | QPushButton；Add | `on_saveRuleButton_clicked @188` | 运行约束待核 |
| `U-ignore_rules/ruleProcessNameMatchTypeComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-ignore_rules/ruleProcessNameLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-ignore_rules/ruleWindowTitleMatchTypeComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-ignore_rules/ruleWindowTitleLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-ignore_rules/clearRuleButton` | QPushButton；Clear | `on_clearRuleButton_clicked @257` | 运行约束待核 |
| `U-ignore_rules/ruleClassNameMatchTypeComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-ignore_rules/ruleClassNameLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-ignore_rules/ruleDescriptionLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-ignore_rules/ruleDisabledCheckBox` | QCheckBox；Disabled | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-ignore_rules/ruleListWidget` | QListWidget | `on_ruleListWidget_itemDoubleClicked @365` | 运行约束待核 |
| `U-ignore_rules/deleteRuleButton` | QPushButton；Delete | `on_deleteRuleButton_clicked @286` | 运行约束待核 |
| `U-ignore_rules/restoreDefaultRulesButton` | QPushButton；RestoreDefault | `on_restoreDefaultRulesButton_clicked @333` | 运行约束待核 |

### devices（4 项）

定义：[qinputdevicelistwindow.ui](../../QKeyMapper/qinputdevicelistwindow.ui)；绑定：[qinputdevicelistwindow.cpp](../../QKeyMapper/qinputdevicelistwindow.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-devices/keyboardDeviceTable` | QTableWidget | `connect cellChanged @34` | 运行约束待核 |
| `U-devices/mouseDeviceTable` | QTableWidget | `connect cellChanged @35` | 运行约束待核 |
| `U-devices/confirmButton` | QPushButton；OK | `on_confirmButton_clicked @532` | 运行约束待核 |
| `U-devices/cancelButton` | QPushButton；Cancel | `on_cancelButton_clicked @539` | 运行约束待核 |

### item（46 项）

定义：[qitemsetupdialog.ui](../../QKeyMapper/qitemsetupdialog.ui)；绑定：[qitemsetupdialog.cpp](../../QKeyMapper/qitemsetupdialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-item/mappingKey_KeyUpSequenceEditButton` | QPushButton；SeqEdit | `on_mappingKey_KeyUpSequenceEditButton_clicked @4407` | 运行约束待核 |
| `U-item/originalKeyLineEdit` | QLineEdit | `connect returnPressed @179` | 运行约束待核 |
| `U-item/indexValueLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | readOnly=true |
| `U-item/updateMappingInfoButton` | QPushButton；Mapping Update | `on_updateMappingInfoButton_clicked @4465` | 运行约束待核 |
| `U-item/SetupDialog_MappingKey_KeyUpLineEdit` | KeyStringLineEdit | `connect returnPressed @181` | 运行约束待核 |
| `U-item/mappingKey_SequenceEditButton` | QPushButton；SeqEdit | `on_mappingKey_SequenceEditButton_clicked @4361` | 运行约束待核 |
| `U-item/SetupDialog_MappingKeyLineEdit` | KeyStringLineEdit | `connect returnPressed @180` | 运行约束待核 |
| `U-item/oriList_SelectKeyboardButton` | QPushButton | `connect toggled @518` | checkable=true |
| `U-item/oriList_SelectMouseButton` | QPushButton | `connect toggled @520` | checkable=true |
| `U-item/oriList_SelectGamepadButton` | QPushButton | `connect toggled @522` | checkable=true |
| `U-item/oriList_SelectFunctionButton` | QPushButton | `connect toggled @524` | checkable=true |
| `U-item/mapList_SelectKeyboardButton` | QPushButton | `connect toggled @550`<br>`connect toggled @569`<br>`connect toggled @591` | checkable=true |
| `U-item/mapList_SelectMouseButton` | QPushButton | `connect toggled @552`<br>`connect toggled @571`<br>`connect toggled @593` | checkable=true |
| `U-item/mapList_SelectGamepadButton` | QPushButton | `connect toggled @554`<br>`connect toggled @573`<br>`connect toggled @595` | checkable=true |
| `U-item/mapList_SelectFunctionButton` | QPushButton | `connect toggled @556`<br>`connect toggled @575`<br>`connect toggled @597` | checkable=true |
| `U-item/recordKeysButton` | QPushButton；Record Keys | `on_recordKeysButton_clicked @3975` | 运行约束待核 |
| `U-item/SetupDialog_OriginalKeyListComboBox` | KeyListComboBox | `connect currentTextChanged @177` | 运行约束待核 |
| `U-item/SetupDialog_MappingKeyListComboBox` | KeyListComboBox | `connect currentTextChanged @178` | 运行约束待核 |
| `U-item/crosshairSetupButton` | QPushButton；CrosshairSetup | `on_crosshairSetupButton_clicked @4185` | 运行约束待核 |
| `U-item/keyRecordLineEdit` | QLineEdit | `on_keyRecordLineEdit_textChanged @4302` | 运行约束待核 |
| `U-item/keyRecordEditModeButton` | QPushButton；Edit | `on_keyRecordEditModeButton_clicked @4254` | 运行约束待核 |
| `U-item/keyRecordCopyButton` | QPushButton；Copy | `on_keyRecordCopyButton_clicked @4282` | 运行约束待核 |
| `U-item/floatingButtonSetupButton` | QPushButton；FloatingButton | `on_floatingButtonSetupButton_clicked @4204` | 运行约束待核 |
| `U-item/itemNoteLineEdit` | QLineEdit | `on_itemNoteLineEdit_textChanged @4324`<br>`connect returnPressed @182` | 运行约束待核 |
| `U-item/burstCheckBox` | QCheckBox；Burst | `on_burstCheckBox_stateChanged @3721` | 运行约束待核 |
| `U-item/burstpressSpinBox` | QSpinBox | `on_burstpressSpinBox_valueChanged @3684` | 运行约束待核 |
| `U-item/burstreleaseSpinBox` | QSpinBox | `on_burstreleaseSpinBox_valueChanged @3703` | 运行约束待核 |
| `U-item/lockCheckBox` | QCheckBox；Lock | `on_lockCheckBox_stateChanged @3744` | 运行约束待核 |
| `U-item/mappingKeyUnlockCheckBox` | QCheckBox；MappingKeyUnlock | `on_mappingKeyUnlockCheckBox_stateChanged @4099` | 运行约束待核 |
| `U-item/disableOriginalKeyUnlockCheckBox` | QCheckBox；DisableOriKeyUnlock | `on_disableOriginalKeyUnlockCheckBox_stateChanged @4116` | 运行约束待核 |
| `U-item/disableFnKeySwitchCheckBox` | QCheckBox；DisableFnKeySwitch | `on_disableFnKeySwitchCheckBox_stateChanged @4133` | 运行约束待核 |
| `U-item/sendTimingComboBox` | QComboBox | `on_sendTimingComboBox_currentIndexChanged @3797` | 运行约束待核 |
| `U-item/sendMappingKeyMethodComboBox` | QComboBox | `on_sendMappingKeyMethodComboBox_currentIndexChanged @3845` | UI选项=SendInput, SendMessage, FakerInput |
| `U-item/fixedVKeyCodeSpinBox` | QSpinBox | `on_fixedVKeyCodeSpinBox_valueChanged @4231` | 运行约束待核 |
| `U-item/checkCombKeyOrderCheckBox` | QCheckBox；CheckCombKeyOrder | `on_checkCombKeyOrderCheckBox_stateChanged @3862` | 运行约束待核 |
| `U-item/unbreakableCheckBox` | QCheckBox；Unbreakable | `on_unbreakableCheckBox_stateChanged @4168` | 运行约束待核 |
| `U-item/passThroughCheckBox` | QCheckBox；PassThrough | `on_passThroughCheckBox_stateChanged @3879` | 运行约束待核 |
| `U-item/keySeqHoldDownCheckBox` | QCheckBox；KeySeqHoldDown | `on_keySeqHoldDownCheckBox_stateChanged @3902` | 运行约束待核 |
| `U-item/pasteTextModeComboBox` | QComboBox | `on_pasteTextModeComboBox_currentIndexChanged @3828` | UI选项=Shift+Insert, Ctrl+V |
| `U-item/repeatByKeyCheckBox` | QCheckBox；RepeatByKey | `on_repeatByKeyCheckBox_stateChanged @3990` | 运行约束待核 |
| `U-item/repeatByTimesCheckBox` | QCheckBox；RepeatTimes | `on_repeatByTimesCheckBox_stateChanged @4019` | 运行约束待核 |
| `U-item/repeatTimesSpinBox` | QSpinBox | `on_repeatTimesSpinBox_valueChanged @4047` | 运行约束待核 |
| `U-item/disabledCheckBox` | QCheckBox；Disabled | `on_disabledCheckBox_stateChanged @3768` | 运行约束待核 |
| `U-item/mappingCodeLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | readOnly=true |
| `U-item/copyMappingCodeButton` | QPushButton；Copy KMC | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-item/applyClipboardMappingCodeButton` | QPushButton；Apply KMC | 手动／间接绑定待逐项定位 | 运行约束待核 |

### main（93 项）

定义：[qkeymapper.ui](../../QKeyMapper/qkeymapper.ui)；绑定：[qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-main/processinfoTable` | ProcessInfoTableWidget | `on_processinfoTable_doubleClicked @38789`<br>`on_processinfoTable_clicked @49720` | 运行约束待核 |
| `U-main/keyMappingTabWidget` | KeyMappingTabWidget | `connect currentChanged @3978`<br>`connect tabOrderChanged @3980` | 运行约束待核 |
| `U-main/settingNameLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/backupSettingButton` | QPushButton；Backup | `on_backupSettingButton_clicked @49945` | 运行约束待核 |
| `U-main/settingselectComboBox` | SettingSelectComboBox | `on_settingselectComboBox_textActivated @43446`<br>`on_settingselectComboBox_currentTextChanged @46050` | 运行约束待核 |
| `U-main/savemaplistButton` | QPushButton；SaveSetting | `on_savemaplistButton_clicked @31908` | 运行约束待核 |
| `U-main/settingTabWidget` | QTabWidget | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/checkProcessComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/processLineEdit` | QLineEdit | `connect returnPressed @4034` | 运行约束待核 |
| `U-main/restoreProcessPathButton` | QPushButton；Restore | `on_restoreProcessPathButton_clicked @49702` | 运行约束待核 |
| `U-main/checkWindowTitleComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/windowTitleLineEdit` | QLineEdit | `connect returnPressed @4035` | 运行约束待核 |
| `U-main/checkClassNameComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/classNameLineEdit` | QLineEdit | `connect returnPressed @4036` | 运行约束待核 |
| `U-main/checkDisplayModeComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/descriptionLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/selectSettingCustomIconButton` | QPushButton；Select Custom Icon | `on_selectSettingCustomIconButton_clicked @50032`<br>`connect customContextMenuRequested @3375` | 运行约束待核 |
| `U-main/ignoreRulesListButton` | QPushButton；Ignore Rules List | `on_ignoreRulesListButton_clicked @50001` | 运行约束待核 |
| `U-main/windowswitchkeyLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/languageComboBox` | QComboBox | `on_languageComboBox_currentIndexChanged @46285` | UI选项=中文, English, 日本語 |
| `U-main/selectTrayIconButton` | QPushButton；Select Tray Icon | `on_selectTrayIconButton_clicked @49591` | 运行约束待核 |
| `U-main/autoStartupCheckBox` | QCheckBox；Auto Startup | `on_autoStartupCheckBox_stateChanged @46215` | 运行约束待核 |
| `U-main/notificationComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/notificationAdvancedSettingButton` | QPushButton；Noti Advanced | `on_notificationAdvancedSettingButton_clicked @49596` | 运行约束待核 |
| `U-main/startupMinimizedCheckBox` | QCheckBox；Startup Minimized | `on_startupMinimizedCheckBox_stateChanged @49316` | 运行约束待核 |
| `U-main/updateSiteComboBox` | QComboBox | 手动／间接绑定待逐项定位 | UI选项=Github, Gitee |
| `U-main/checkUpdateButton` | QPushButton；Check Updates | `on_checkUpdateButton_clicked @49445` | 运行约束待核 |
| `U-main/startupAutoMonitoringCheckBox` | QCheckBox；Startup AutoMonitoring | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/scaleComboBox` | QComboBox | `connect activated @3767`<br>`connect currentIndexChanged @16595` | 运行约束待核 |
| `U-main/generalAdvancedButton` | QPushButton；General Advanced | `on_generalAdvancedButton_clicked @49935` | 运行约束待核 |
| `U-main/closeToSystemTrayCheckBox` | QCheckBox；Minimize to tray on close | `connect clicked @37182` | 运行约束待核 |
| `U-main/themeComboBox` | QComboBox | `on_themeComboBox_currentIndexChanged @49896` | 运行约束待核 |
| `U-main/mappingStartKeyLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/autoStartMappingCheckBox` | QCheckBox；Auto Match Foreground | `on_autoStartMappingCheckBox_stateChanged @49333` | 运行约束待核 |
| `U-main/mappingStopKeyLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/enableSystemFilterKeyButton` | QPushButton；SystemFilterKey | `connect clicked @34357` | 运行约束待核 |
| `U-main/mappingAdvancedSettingButton` | QPushButton；Mapping Advanced | `on_mappingAdvancedSettingButton_clicked @49940` | 运行约束待核 |
| `U-main/sendToSameTitleWindowsCheckBox` | QCheckBox；Send To Same Windows | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/mappingMacroListButton` | QPushButton；Mapping MacroList | `on_mappingMacroListButton_clicked @50083` | 运行约束待核 |
| `U-main/vButtonPanelSetupButton` | QPushButton；VButton Panel | `on_vButtonPanelSetupButton_clicked @30502` | 运行约束待核 |
| `U-main/installFakerInputButton` | QPushButton；Install FakerInput | `on_installFakerInputButton_clicked @46493` | 运行约束待核 |
| `U-main/enableVirtualJoystickCheckBox` | QCheckBox；Enable Virtual Gamepad | `on_enableVirtualJoystickCheckBox_stateChanged @46308` | 运行约束待核 |
| `U-main/virtualGamepadTypeComboBox` | QComboBox | 手动／间接绑定待逐项定位 | UI选项=X360, DS4 |
| `U-main/virtualGamepadNumberSpinBox` | QSpinBox | `on_virtualGamepadNumberSpinBox_valueChanged @47310` | 运行约束待核 |
| `U-main/virtualGamepadListComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/installViGEmBusButton` | QPushButton；InstallViGEm | `on_installViGEmBusButton_clicked @46408` | 运行约束待核 |
| `U-main/directModeCheckBox` | QCheckBox；Direct Mode | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/lockCursorCheckBox` | QCheckBox；Lock Cursor | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/vJoyRecenterSpinBox` | QSpinBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/vJoyXSensSpinBox` | QSpinBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/vJoyInvertXCheckBox` | QCheckBox；InvertX | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/vJoyYSensSpinBox` | QSpinBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/vJoyInvertYCheckBox` | QCheckBox；InvertY | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/Gyro2MouseXSpeedSpinBox` | QDoubleSpinBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/Gyro2MouseMinXSensSpinBox` | QDoubleSpinBox | `connect valueChanged @34724` | decimals=1 |
| `U-main/Gyro2MouseMaxXSensSpinBox` | QDoubleSpinBox | `connect valueChanged @34727` | decimals=1 |
| `U-main/Gyro2MouseYSpeedSpinBox` | QDoubleSpinBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/Gyro2MouseMinYSensSpinBox` | QDoubleSpinBox | `connect valueChanged @34732` | decimals=1 |
| `U-main/Gyro2MouseMaxYSensSpinBox` | QDoubleSpinBox | `connect valueChanged @34735` | decimals=1 |
| `U-main/Gyro2MouseAdvancedSettingButton` | QPushButton；Advanced | `on_Gyro2MouseAdvancedSettingButton_clicked @49586` | 运行约束待核 |
| `U-main/Gyro2MouseMinThresholdSpinBox` | QDoubleSpinBox | `connect valueChanged @34740` | decimals=1 |
| `U-main/Gyro2MouseMaxThresholdSpinBox` | QDoubleSpinBox | `connect valueChanged @34743` | decimals=1 |
| `U-main/multiInputEnableCheckBox` | QCheckBox；Enable Multi-Device Support | `on_multiInputEnableCheckBox_stateChanged @47283` | 运行约束待核 |
| `U-main/multiInputDeviceListButton` | QPushButton；DeviceList | `on_multiInputDeviceListButton_clicked @47278` | 运行约束待核 |
| `U-main/installInterceptionButton` | QPushButton；Install Driver | `on_installInterceptionButton_clicked @47199` | 运行约束待核 |
| `U-main/filterKeysCheckBox` | QCheckBox；FilterKeys | `on_filterKeysCheckBox_stateChanged @47373` | 运行约束待核 |
| `U-main/brakeThresholdDoubleSpinBox` | QDoubleSpinBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/accelThresholdDoubleSpinBox` | QDoubleSpinBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/dataPortSpinBox` | QSpinBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/oriList_SelectKeyboardButton` | QPushButton | `on_oriList_SelectKeyboardButton_toggled @49623` | checkable=true |
| `U-main/oriList_SelectMouseButton` | QPushButton | `on_oriList_SelectMouseButton_toggled @49633` | checkable=true |
| `U-main/oriList_SelectGamepadButton` | QPushButton | `on_oriList_SelectGamepadButton_toggled @49643` | checkable=true |
| `U-main/oriList_SelectFunctionButton` | QPushButton | `on_oriList_SelectFunctionButton_toggled @49653` | checkable=true |
| `U-main/mapList_SelectKeyboardButton` | QPushButton | `on_mapList_SelectKeyboardButton_toggled @49663` | checkable=true |
| `U-main/mapList_SelectMouseButton` | QPushButton | `on_mapList_SelectMouseButton_toggled @49673` | checkable=true |
| `U-main/mapList_SelectGamepadButton` | QPushButton | `on_mapList_SelectGamepadButton_toggled @49683` | checkable=true |
| `U-main/mapList_SelectFunctionButton` | QPushButton | `on_mapList_SelectFunctionButton_toggled @49693` | checkable=true |
| `U-main/addmapdataButton` | QPushButton；ADD | `on_addmapdataButton_clicked @39033` | 运行约束待核 |
| `U-main/orikeyComboBox` | KeyListComboBox | `connect currentTextChanged @4018` | 运行约束待核 |
| `U-main/mapkeyComboBox` | KeyListComboBox | `connect currentTextChanged @4019` | 运行约束待核 |
| `U-main/originalKeyRecordLineEdit` | QLineEdit | `on_originalKeyRecordLineEdit_textChanged @49874` | 运行约束待核 |
| `U-main/originalKeyEditModeButton` | QPushButton；Edit | `on_originalKeyEditModeButton_clicked @49828` | 运行约束待核 |
| `U-main/originalKeyRecordCopyButton` | QPushButton；Copy | `on_originalKeyRecordCopyButton_clicked @49856` | 运行约束待核 |
| `U-main/keyPressTypeComboBox` | QComboBox | `connect currentIndexChanged @3444` | 运行约束待核 |
| `U-main/pressTimeSpinBox` | QSpinBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/waitTimeSpinBox` | QSpinBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/keyboardSelectComboBox` | QComboBox | `on_keyboardSelectComboBox_currentIndexChanged @49808` | 运行约束待核 |
| `U-main/pushLevelSlider` | QSlider | `connect valueChanged @32001` | 运行约束待核 |
| `U-main/pushLevelSpinBox` | QSpinBox | `connect valueChanged @32002` | 运行约束待核 |
| `U-main/mouseSelectComboBox` | QComboBox | `on_mouseSelectComboBox_currentIndexChanged @49815` | 运行约束待核 |
| `U-main/sendTextPlainTextEdit` | QPlainTextEdit | `on_sendTextPlainTextEdit_textChanged @49906` | 运行约束待核 |
| `U-main/gamepadSelectComboBox` | QComboBox | `on_gamepadSelectComboBox_currentIndexChanged @49822` | 运行约束待核 |
| `U-main/keymapButton` | MappingStartToolButton；MappingStart | `on_keymapButton_clicked @16372` | 运行约束待核 |

### record（4 项）

定义：[qkeyrecord.ui](../../QKeyMapper/qkeyrecord.ui)；绑定：[qkeyrecord.cpp](../../QKeyMapper/qkeyrecord.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-record/keyRecordLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | readOnly=true |
| `U-record/mergeKeyActionsCheckBox` | QCheckBox；Merge key actions | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-record/ignoreWaitTimeCheckBox` | QCheckBox；Ignore waittime | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-record/recordStartStopButton` | QPushButton；Start Record | `on_recordStartStopButton_clicked @686` | 运行约束待核 |

### macro（15 项）

定义：[qmacrolistdialog.ui](../../QKeyMapper/qmacrolistdialog.ui)；绑定：[qmacrolistdialog.cpp](../../QKeyMapper/qmacrolistdialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-macro/macroListTabWidget` | MacroListTabWidget | `connect currentChanged @116` | 运行约束待核 |
| `U-macro/macrolistTable` | MacroListDataTableWidget | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-macro/universalmacrolistTable` | MacroListDataTableWidget | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-macro/macroNameLineEdit` | QLineEdit | `connect returnPressed @124` | 运行约束待核 |
| `U-macro/categoryLineEdit` | QLineEdit | `connect returnPressed @126` | 运行约束待核 |
| `U-macro/macroContentLineEdit` | KeyStringLineEdit | `connect returnPressed @125` | 运行约束待核 |
| `U-macro/macroContent_SequenceEditButton` | QPushButton；SeqEdit | `on_macroContent_SequenceEditButton_clicked @848` | 运行约束待核 |
| `U-macro/macroNoteLineEdit` | QLineEdit | `connect returnPressed @127` | 运行约束待核 |
| `U-macro/mapList_SelectKeyboardButton` | QPushButton | 手动／间接绑定待逐项定位 | checkable=true |
| `U-macro/mapList_SelectMouseButton` | QPushButton | 手动／间接绑定待逐项定位 | checkable=true |
| `U-macro/mapList_SelectGamepadButton` | QPushButton | 手动／间接绑定待逐项定位 | checkable=true |
| `U-macro/mapList_SelectFunctionButton` | QPushButton | 手动／间接绑定待逐项定位 | checkable=true |
| `U-macro/MacroList_MappingKeyListComboBox` | KeyListComboBox | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-macro/clearButton` | QPushButton；Clear Editing | `on_clearButton_clicked @820` | 运行约束待核 |
| `U-macro/addMacroButton` | QPushButton；Add Macro | `on_addMacroButton_clicked @814` | 运行约束待核 |

### mapping_advanced（27 项）

定义：[qmappingadvanceddialog.ui](../../QKeyMapper/qmappingadvanceddialog.ui)；绑定：[qmappingadvanceddialog.cpp](../../QKeyMapper/qmappingadvanceddialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-mapping_advanced/mouseXSpeedSpinBox` | QSpinBox | `connect valueChanged @121` | 运行约束待核 |
| `U-mapping_advanced/mouseYSpeedSpinBox` | QSpinBox | `connect valueChanged @123` | 运行约束待核 |
| `U-mapping_advanced/mousePollingIntervalSpinBox` | QSpinBox | `connect valueChanged @125` | 运行约束待核 |
| `U-mapping_advanced/ProcessIconAsTrayIconCheckBox` | QCheckBox；ProcessIcon as TrayIcon | `connect toggled @135` | 运行约束待核 |
| `U-mapping_advanced/acceptVirtualGamepadInputCheckBox` | QCheckBox；Accept Virtual Gamepad Input | `connect toggled @137` | 运行约束待核 |
| `U-mapping_advanced/enableCommonMappingTableCheckBox` | QCheckBox；Enable Common Mapping Table | `connect toggled @139` | 运行约束待核 |
| `U-mapping_advanced/showWindowPointKeyComboBox` | QComboBox | `connect currentIndexChanged @144` | 运行约束待核 |
| `U-mapping_advanced/showScreenPointKeyComboBox` | QComboBox | `connect currentIndexChanged @146` | 运行约束待核 |
| `U-mapping_advanced/customNotificationEnableCheckBox` | QCheckBox；Enable Custom Notification | `connect toggled @114` | 运行约束待核 |
| `U-mapping_advanced/customNotificationPositionComboBox` | QComboBox | `connect currentIndexChanged @148` | 运行约束待核 |
| `U-mapping_advanced/customNotificationSetupButton` | QPushButton；Custom Notification Setup | `connect clicked @119` | 运行约束待核 |
| `U-mapping_advanced/LT_Threshold_PressSpinBox` | QSpinBox | `connect valueChanged @256` | 运行约束待核 |
| `U-mapping_advanced/LT_Threshold_LightPressSpinBox` | QSpinBox | `connect valueChanged @247`<br>`connect valueChanged @251` | 运行约束待核 |
| `U-mapping_advanced/LT_Threshold_ReleaseSpinBox` | QSpinBox | `connect valueChanged @239` | 运行约束待核 |
| `U-mapping_advanced/RT_Threshold_PressSpinBox` | QSpinBox | `connect valueChanged @283` | 运行约束待核 |
| `U-mapping_advanced/RT_Threshold_LightPressSpinBox` | QSpinBox | `connect valueChanged @274`<br>`connect valueChanged @278` | 运行约束待核 |
| `U-mapping_advanced/RT_Threshold_ReleaseSpinBox` | QSpinBox | `connect valueChanged @266` | 运行约束待核 |
| `U-mapping_advanced/LS_Threshold_PushSpinBox` | QSpinBox | `connect valueChanged @310` | 运行约束待核 |
| `U-mapping_advanced/LS_Threshold_LightPushSpinBox` | QSpinBox | `connect valueChanged @301`<br>`connect valueChanged @305` | 运行约束待核 |
| `U-mapping_advanced/LS_Threshold_ReleaseSpinBox` | QSpinBox | `connect valueChanged @293` | 运行约束待核 |
| `U-mapping_advanced/RS_Threshold_PushSpinBox` | QSpinBox | `connect valueChanged @337` | 运行约束待核 |
| `U-mapping_advanced/RS_Threshold_LightPushSpinBox` | QSpinBox | `connect valueChanged @328`<br>`connect valueChanged @332` | 运行约束待核 |
| `U-mapping_advanced/RS_Threshold_ReleaseSpinBox` | QSpinBox | `connect valueChanged @320` | 运行约束待核 |
| `U-mapping_advanced/gamepadTouchpadXSpeedSpinBox` | QDoubleSpinBox | `connect valueChanged @127` | decimals=2 |
| `U-mapping_advanced/gamepadTouchpadYSpeedSpinBox` | QDoubleSpinBox | `connect valueChanged @129` | decimals=2 |
| `U-mapping_advanced/gamepadTouchpadInvertXCheckBox` | QCheckBox；Invert X | `connect toggled @131` | 运行约束待核 |
| `U-mapping_advanced/gamepadTouchpadInvertYCheckBox` | QCheckBox；Invert Y | `connect toggled @133` | 运行约束待核 |

### sequence（13 项）

定义：[qmappingsequenceedit.ui](../../QKeyMapper/qmappingsequenceedit.ui)；绑定：[qmappingsequenceedit.cpp](../../QKeyMapper/qmappingsequenceedit.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-sequence/mappingSequenceEditTable` | MappingSequenceEditTableWidget | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-sequence/MappingSequenceEdit_MappingKeyLineEdit` | KeyStringLineEdit | `connect returnPressed @72` | 运行约束待核 |
| `U-sequence/mapList_SelectKeyboardButton` | QPushButton | 手动／间接绑定待逐项定位 | checkable=true |
| `U-sequence/mapList_SelectMouseButton` | QPushButton | 手动／间接绑定待逐项定位 | checkable=true |
| `U-sequence/mapList_SelectGamepadButton` | QPushButton | 手动／间接绑定待逐项定位 | checkable=true |
| `U-sequence/mapList_SelectFunctionButton` | QPushButton | 手动／间接绑定待逐项定位 | checkable=true |
| `U-sequence/MappingSequenceEdit_MappingKeyListComboBox` | KeyListComboBox | `connect currentTextChanged @75` | 运行约束待核 |
| `U-sequence/insertMappingKeyButton` | QPushButton；Insert | `on_insertMappingKeyButton_clicked @1159` | 运行约束待核 |
| `U-sequence/confirmButton` | QPushButton；OK | `on_confirmButton_clicked @1168` | 运行约束待核 |
| `U-sequence/cancelButton` | QPushButton；Cancel | `on_cancelButton_clicked @1232` | 运行约束待核 |
| `U-sequence/deleteButton` | QPushButton；Delete | `on_deleteButton_clicked @1237` | 运行约束待核 |
| `U-sequence/undoButton` | QPushButton；Undo | `on_undoButton_clicked @1244` | 运行约束待核 |
| `U-sequence/redoButton` | QPushButton；Redo | `on_redoButton_clicked @1251` | 运行约束待核 |

### notification（15 项）

定义：[qnotificationsetupdialog.ui](../../QKeyMapper/qnotificationsetupdialog.ui)；绑定：[qnotificationsetupdialog.cpp](../../QKeyMapper/qnotificationsetupdialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-notification/fontColorPicker` | ColorPickerWidget | `connect colorChanged @81` | 运行约束待核 |
| `U-notification/fontFamilyComboBox` | QFontComboBox | `connect currentFontChanged @83` | 运行约束待核 |
| `U-notification/fontFamilyDefaultButton` | QPushButton；Default | `connect clicked @89` | 运行约束待核 |
| `U-notification/fontSizeSpinBox` | QSpinBox | `connect valueChanged @95` | 运行约束待核 |
| `U-notification/fontWeightComboBox` | QComboBox | `connect currentIndexChanged @97` | 运行约束待核 |
| `U-notification/fontItalicCheckBox` | QCheckBox；Italic | `connect toggled @99` | 运行约束待核 |
| `U-notification/displayDurationSpinBox` | QSpinBox | `connect valueChanged @101` | 运行约束待核 |
| `U-notification/fadeinDurationSpinBox` | QSpinBox | `connect valueChanged @103` | 运行约束待核 |
| `U-notification/fadeoutDurationSpinBox` | QSpinBox | `connect valueChanged @105` | 运行约束待核 |
| `U-notification/backgroundColorPicker` | ColorPickerWidget | `connect colorChanged @82` | 运行约束待核 |
| `U-notification/borderRadiusSpinBox` | QSpinBox | `connect valueChanged @107` | 运行约束待核 |
| `U-notification/paddingSpinBox` | QSpinBox | `connect valueChanged @109` | 运行约束待核 |
| `U-notification/opacitySpinBox` | QDoubleSpinBox | `connect valueChanged @111` | 运行约束待核 |
| `U-notification/x_offsetSpinBox` | QSpinBox | `connect valueChanged @113` | 运行约束待核 |
| `U-notification/y_offsetSpinBox` | QSpinBox | `connect valueChanged @115` | 运行约束待核 |

### tab（18 项）

定义：[qtablesetupdialog.ui](../../QKeyMapper/qtablesetupdialog.ui)；绑定：[qtablesetupdialog.cpp](../../QKeyMapper/qtablesetupdialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-tab/tabNameLineEdit` | QLineEdit | `connect returnPressed @65` | 运行约束待核 |
| `U-tab/tabNameUpdateButton` | QPushButton；Update | `on_tabNameUpdateButton_clicked @586` | 运行约束待核 |
| `U-tab/tabHotkeyLineEdit` | QLineEdit | `connect returnPressed @66` | 运行约束待核 |
| `U-tab/tabHotkeyUpdateButton` | QPushButton；Update | `on_tabHotkeyUpdateButton_clicked @629` | 运行约束待核 |
| `U-tab/exportTableButton` | QPushButton；ExportTable | `on_exportTableButton_clicked @694` | 运行约束待核 |
| `U-tab/importTableButton` | QPushButton；ImportTable | `on_importTableButton_clicked @700` | 运行约束待核 |
| `U-tab/removeTableButton` | QPushButton；RemoveTable | `on_removeTableButton_clicked @705` | 运行约束待核 |
| `U-tab/notificationFontColorPicker` | ColorPickerWidget | `connect colorChanged @69` | 运行约束待核 |
| `U-tab/notificationBackgroundColorPicker` | ColorPickerWidget | `connect colorChanged @70` | 运行约束待核 |
| `U-tab/hideNotificationCheckBox` | QCheckBox；Hide Notification | `on_hideNotificationCheckBox_stateChanged @930` | 运行约束待核 |
| `U-tab/appendCommonMappingTableCheckBox` | QCheckBox；Append Common Mapping Table | `on_appendCommonMappingTableCheckBox_stateChanged @947` | 运行约束待核 |
| `U-tab/selectCustomImageButton` | QPushButton；Select Custom Image | `on_selectCustomImageButton_clicked @710`<br>`connect customContextMenuRequested @67` | 运行约束待核 |
| `U-tab/customImageShowPositionComboBox` | QComboBox | `on_customImageShowPositionComboBox_currentIndexChanged @831` | 运行约束待核 |
| `U-tab/customImagePaddingSpinBox` | QSpinBox | `on_customImagePaddingSpinBox_valueChanged @848` | 运行约束待核 |
| `U-tab/customImageShowAsTrayIconCheckBox` | QCheckBox；Show as TrayIcon | `on_customImageShowAsTrayIconCheckBox_stateChanged @865` | 运行约束待核 |
| `U-tab/customImageTrayIconPixelComboBox` | QComboBox | `on_customImageTrayIconPixelComboBox_currentIndexChanged @883` | 运行约束待核 |
| `U-tab/customImageShowAsFloatingWindowCheckBox` | QCheckBox；Show as FloatingWindow | `on_customImageShowAsFloatingWindowCheckBox_stateChanged @893` | 运行约束待核 |
| `U-tab/floatingWindowSetupButton` | QPushButton；FloatingWindow Setup | `on_floatingWindowSetupButton_clicked @911` | 运行约束待核 |

### tray（5 项）

定义：[qtrayiconselectdialog.ui](../../QKeyMapper/qtrayiconselectdialog.ui)；绑定：[qtrayiconselectdialog.cpp](../../QKeyMapper/qtrayiconselectdialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-tray/idleStateTrayIconSelectComboBox` | QComboBox | `connect activated @44` | 运行约束待核 |
| `U-tray/monitoringStateTrayIconSelectComboBox` | QComboBox | `connect activated @46` | 运行约束待核 |
| `U-tray/globalStateTrayIconSelectComboBox` | QComboBox | `connect activated @48` | 运行约束待核 |
| `U-tray/matchedStateTrayIconSelectComboBox` | QComboBox | `connect activated @50` | 运行约束待核 |
| `U-tray/addCustomTrayiconsButton` | QPushButton；Add custom trayicons | `on_addCustomTrayiconsButton_clicked @327` | 运行约束待核 |

### vbutton_panel（24 项）

定义：[qvbuttonpanelsetupdialog.ui](../../QKeyMapper/qvbuttonpanelsetupdialog.ui)；绑定：[qvbuttonpanelsetupdialog.cpp](../../QKeyMapper/qvbuttonpanelsetupdialog.cpp)。

| 稳定定位 ID | 控件类型／原始文案 | 绑定线索（该 cpp 的基线行号） | UI 声明线索 |
|---|---|---|---|
| `U-vbutton_panel/btnColorPicker` | ColorPickerWidget | `connect colorChanged @132`<br>`connect previewColorChanged @138` | 运行约束待核 |
| `U-vbutton_panel/bgColorPicker` | ColorPickerWidget | `connect colorChanged @131`<br>`connect previewColorChanged @137` | 运行约束待核 |
| `U-vbutton_panel/pressedColorPicker` | ColorPickerWidget | `connect colorChanged @133`<br>`connect previewColorChanged @139` | 运行约束待核 |
| `U-vbutton_panel/lockedColorPicker` | ColorPickerWidget | `connect colorChanged @134`<br>`connect previewColorChanged @140` | 运行约束待核 |
| `U-vbutton_panel/textColorPicker` | ColorPickerWidget | `connect colorChanged @135`<br>`connect previewColorChanged @141` | 运行约束待核 |
| `U-vbutton_panel/columnsSpinBox` | QSpinBox | `connect valueChanged @106` | minimum=1；maximum=20 |
| `U-vbutton_panel/radiusSpinBox` | QSpinBox | `connect valueChanged @112` | minimum=0；maximum=100 |
| `U-vbutton_panel/maxRowsSpinBox` | QSpinBox | `connect valueChanged @107` | minimum=2；maximum=999 |
| `U-vbutton_panel/opacitySpinBox` | QDoubleSpinBox | `connect valueChanged @110` | minimum=0.100000000000000；maximum=1.000000000000000；decimals=2 |
| `U-vbutton_panel/marginSpinBox` | QSpinBox | `connect valueChanged @111` | minimum=0；maximum=50 |
| `U-vbutton_panel/defaultShowCheckBox` | QCheckBox；Show on Mapping Start | `connect checkStateChanged @100`<br>`connect stateChanged @104` | 运行约束待核 |
| `U-vbutton_panel/btnWidthSpinBox` | QSpinBox | `connect valueChanged @108` | minimum=4；maximum=640 |
| `U-vbutton_panel/dragEnabledCheckBox` | QCheckBox；Drag Move | `connect checkStateChanged @99`<br>`connect stateChanged @103` | 运行约束待核 |
| `U-vbutton_panel/btnHeightSpinBox` | QSpinBox | `connect valueChanged @109` | minimum=4；maximum=640 |
| `U-vbutton_panel/alwaysOnTopCheckBox` | QCheckBox；Always On Top | `connect checkStateChanged @98`<br>`connect stateChanged @102` | 运行约束待核 |
| `U-vbutton_panel/btnFontSizeSpinBox` | QSpinBox | `connect valueChanged @116` | minimum=1；maximum=72 |
| `U-vbutton_panel/btnFontWeightComboBox` | QComboBox | `connect currentIndexChanged @117` | 运行约束待核 |
| `U-vbutton_panel/btnFontFamilyComboBox` | QFontComboBox | `connect currentFontChanged @85`<br>`connect currentFontChanged @118` | 运行约束待核 |
| `U-vbutton_panel/btnFontFamilyDefaultButton` | QPushButton；Default | `connect clicked @90` | 运行约束待核 |
| `U-vbutton_panel/referencePointComboBox` | QComboBox | `connect currentIndexChanged @113` | 运行约束待核 |
| `U-vbutton_panel/offsetXSpinBox` | QSpinBox | `connect valueChanged @114` | minimum=-50000；maximum=50000 |
| `U-vbutton_panel/offsetYSpinBox` | QSpinBox | `connect valueChanged @115` | minimum=-50000；maximum=50000 |
| `U-vbutton_panel/revertButton` | QPushButton；Revert | `on_revertButton_clicked @301` | 运行约束待核 |
| `U-vbutton_panel/okButton` | QPushButton；Apply | `on_okButton_clicked @293` | 运行约束待核 |

### 动态 floating_button（48 项）

定义和连接：[qfloatingbuttonsetupdialog.cpp](../../QKeyMapper/qfloatingbuttonsetupdialog.cpp)。局部循环变量（例如 `cb`、`btn`）代表按数据重复创建的一类入口；重命名时保留此定位 ID 或明确记录迁移。

| 稳定定位 ID | 类型 | 定位／绑定线索 |
|---|---|---|
| `D-QFloatingButtonSetupDialog/m_ItemOriginalKeyLineEdit` | QLineEdit | 构造 @192；手动／间接绑定待逐项定位 |
| `D-QFloatingButtonSetupDialog/m_ItemNoteLineEdit` | QLineEdit | 构造 @194；手动／间接绑定待逐项定位 |
| `D-QFloatingButtonSetupDialog/m_ItemIndexLineEdit` | QLineEdit | 构造 @196；手动／间接绑定待逐项定位 |
| `D-QFloatingButtonSetupDialog/m_EnableCheckBox` | QCheckBox | 构造 @197；`connect checkStateChanged @582` |
| `D-QFloatingButtonSetupDialog/m_LabelLineEdit` | QLineEdit | 构造 @199；`connect textChanged @586` |
| `D-QFloatingButtonSetupDialog/m_ShowOnStartCheckBox` | QCheckBox | 构造 @200；`connect checkStateChanged @588`<br>`connect stateChanged @597` |
| `D-QFloatingButtonSetupDialog/m_ShowToolTipCheckBox` | QCheckBox | 构造 @201；`connect checkStateChanged @589`<br>`connect stateChanged @598` |
| `D-QFloatingButtonSetupDialog/m_SyncPressedLockedStateCheckBox` | QCheckBox | 构造 @202；`connect checkStateChanged @590`<br>`connect stateChanged @599` |
| `D-QFloatingButtonSetupDialog/m_AlwaysOnTopCheckBox` | QCheckBox | 构造 @203；`connect checkStateChanged @591`<br>`connect stateChanged @600` |
| `D-QFloatingButtonSetupDialog/m_MousePassThroughCheckBox` | QCheckBox | 构造 @204；`connect checkStateChanged @592`<br>`connect stateChanged @601` |
| `D-QFloatingButtonSetupDialog/m_DragToMoveCheckBox` | QCheckBox | 构造 @205；`connect checkStateChanged @593`<br>`connect stateChanged @602` |
| `D-QFloatingButtonSetupDialog/m_EnableGradientFillCheckBox` | QCheckBox | 构造 @206；`connect checkStateChanged @594`<br>`connect stateChanged @603` |
| `D-QFloatingButtonSetupDialog/m_EnableHoverAnimationCheckBox` | QCheckBox | 构造 @207；`connect checkStateChanged @595`<br>`connect stateChanged @604` |
| `D-QFloatingButtonSetupDialog/m_MousePassThroughSwitchKeyComboBox` | QComboBox | 构造 @209；`connect currentIndexChanged @606` |
| `D-QFloatingButtonSetupDialog/m_WidthSpinBox` | QSpinBox | 构造 @225；`connect valueChanged @607` |
| `D-QFloatingButtonSetupDialog/m_HeightSpinBox` | QSpinBox | 构造 @226；`connect valueChanged @608` |
| `D-QFloatingButtonSetupDialog/m_FontSizeSpinBox` | QSpinBox | 构造 @227；`connect valueChanged @609` |
| `D-QFloatingButtonSetupDialog/m_FontWeightComboBox` | QComboBox | 构造 @228；`connect currentIndexChanged @610` |
| `D-QFloatingButtonSetupDialog/m_FontFamilyComboBox` | QFontComboBox | 构造 @229；`connect currentFontChanged @611` |
| `D-QFloatingButtonSetupDialog/m_FontFamilyDefaultButton` | QPushButton | 构造 @230；`connect clicked @617` |
| `D-QFloatingButtonSetupDialog/m_RadiusSpinBox` | QSpinBox | 构造 @231；`connect valueChanged @623` |
| `D-QFloatingButtonSetupDialog/m_BorderWidthSpinBox` | QSpinBox | 构造 @232；`connect valueChanged @624` |
| `D-QFloatingButtonSetupDialog/m_NormalOpacitySpinBox` | QDoubleSpinBox | 构造 @233；`connect valueChanged @625` |
| `D-QFloatingButtonSetupDialog/m_PressedOpacitySpinBox` | QDoubleSpinBox | 构造 @234；`connect valueChanged @626` |
| `D-QFloatingButtonSetupDialog/m_LockedOpacitySpinBox` | QDoubleSpinBox | 构造 @235；`connect valueChanged @627` |
| `D-QFloatingButtonSetupDialog/m_HoverEffectStrengthSpinBox` | QSpinBox | 构造 @236；`connect valueChanged @628` |
| `D-QFloatingButtonSetupDialog/m_HoverGlowStrengthSpinBox` | QSpinBox | 构造 @237；`connect valueChanged @629` |
| `D-QFloatingButtonSetupDialog/m_HoverContrastModeComboBox` | QComboBox | 构造 @238；`connect currentIndexChanged @631` |
| `D-QFloatingButtonSetupDialog/m_HoverAnimationDurationSpinBox` | QSpinBox | 构造 @239；`connect valueChanged @630` |
| `D-QFloatingButtonSetupDialog/m_StyleCodeLineEdit` | QLineEdit | 构造 @240；手动／间接绑定待逐项定位 |
| `D-QFloatingButtonSetupDialog/m_CopyStyleCodeButton` | QPushButton | 构造 @241；`connect clicked @578` |
| `D-QFloatingButtonSetupDialog/m_ApplyClipboardStyleCodeButton` | QPushButton | 构造 @242；`connect clicked @579` |
| `D-QFloatingButtonSetupDialog/m_ReferencePointComboBox` | QComboBox | 构造 @246；`connect currentIndexChanged @636` |
| `D-QFloatingButtonSetupDialog/m_OffsetXSpinBox` | QSpinBox | 构造 @247；`connect valueChanged @637` |
| `D-QFloatingButtonSetupDialog/m_OffsetYSpinBox` | QSpinBox | 构造 @248；`connect valueChanged @642` |
| `D-QFloatingButtonSetupDialog/m_SyncGroupIdSpinBox` | QSpinBox | 构造 @250；`connect valueChanged @647` |
| `D-QFloatingButtonSetupDialog/m_ShowDragCoordinateCheckBox` | QCheckBox | 构造 @251；`connect checkStateChanged @672`<br>`connect toggled @676` |
| `D-QFloatingButtonSetupDialog/m_SyncGroupMoveCheckBox` | QCheckBox | 构造 @252；`connect checkStateChanged @667` |
| `D-QFloatingButtonSetupDialog/m_ButtonCountLineEdit` | QLineEdit | 构造 @254；手动／间接绑定待逐项定位 |
| `D-QFloatingButtonSetupDialog/m_SyncGroupNoteLineEdit` | QLineEdit | 构造 @256；`connect textChanged @653` |
| `D-QFloatingButtonSetupDialog/m_ButtonColorPicker` | ColorPickerWidget | 构造 @260；`connect colorChanged @689`<br>`connect previewColorChanged @696` |
| `D-QFloatingButtonSetupDialog/m_PressedColorPicker` | ColorPickerWidget | 构造 @261；`connect colorChanged @690`<br>`connect previewColorChanged @697` |
| `D-QFloatingButtonSetupDialog/m_LockedColorPicker` | ColorPickerWidget | 构造 @262；`connect colorChanged @691`<br>`connect previewColorChanged @698` |
| `D-QFloatingButtonSetupDialog/m_TextColorPicker` | ColorPickerWidget | 构造 @263；`connect colorChanged @692`<br>`connect previewColorChanged @699` |
| `D-QFloatingButtonSetupDialog/m_BorderColorPicker` | ColorPickerWidget | 构造 @264；`connect colorChanged @693`<br>`connect previewColorChanged @700` |
| `D-QFloatingButtonSetupDialog/m_HoverCustomColorPicker` | ColorPickerWidget | 构造 @265；`connect colorChanged @694`<br>`connect previewColorChanged @701` |
| `D-QFloatingButtonSetupDialog/m_ApplyButton` | QDialogButtonBox 按钮 | addButton @525；`connect clicked @576` |
| `D-QFloatingButtonSetupDialog/m_RevertButton` | QDialogButtonBox 按钮 | addButton @526；`connect clicked @577` |

### 动态 main（20 项）

定义和连接：[qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)。局部循环变量（例如 `cb`、`btn`）代表按数据重复创建的一类入口；重命名时保留此定位 ID 或明确记录迁移。

| 稳定定位 ID | 类型 | 定位／绑定线索 |
|---|---|---|
| `D-QKeyMapper/cb` | QCheckBox | 构造 @33264；`connect toggled @33278` |
| `D-QKeyMapper/m_EnableSystemFilterKeyPopupCheckBox` | QCheckBox | 构造 @34346；`connect checkStateChanged @34360` |
| `D-QKeyMapper/m_DisableFilterKeyClickSoundPopupCheckBox` | QCheckBox | 构造 @34347；手动／间接绑定待逐项定位 |
| `D-QKeyMapper/m_CategoryFilterAllCheckBox` | QCheckBox | 构造 @34521；`connect checkStateChanged @34544`<br>`connect stateChanged @34567` |
| `D-KeyListComboBoxPopup/m_SearchLineEdit` | QLineEdit | 构造 @41884；`connect textChanged @41972` |
| `D-KeyListComboBoxPopup/m_FavoritesToolButton` | QToolButton | 构造 @41886；`connect clicked @41973`<br>`connect customContextMenuRequested @41993` |
| `D-KeyListComboBoxPopup/m_RecentToolButton` | QToolButton | 构造 @41887；`connect clicked @41983`<br>`connect customContextMenuRequested @41994` |
| `D-KeyListComboBoxPopup/m_BackToolButton` | QToolButton | 构造 @41891；`connect clicked @41998` |
| `D-SettingSelectComboBoxPopup/m_SearchLineEdit` | QLineEdit | 构造 @43513；`connect textChanged @43590` |
| `D-SettingSelectComboBoxPopup/m_MoveTopToolButton` | QToolButton | 构造 @43515；`connect clicked @43596` |
| `D-SettingSelectComboBoxPopup/m_MoveUpToolButton` | QToolButton | 构造 @43516；`connect clicked @43597` |
| `D-SettingSelectComboBoxPopup/m_MoveDownToolButton` | QToolButton | 构造 @43517；`connect clicked @43598` |
| `D-SettingSelectComboBoxPopup/m_MoveBottomToolButton` | QToolButton | 构造 @43518；`connect clicked @43599` |
| `D-SettingSelectComboBoxPopup/m_ReorderToolButton` | QToolButton | 构造 @43519；`connect clicked @43600` |
| `D-KeyMappingDataTableWidget/inputCategoryLineEdit` | QLineEdit | 构造 @48474；`connect textChanged @48554`<br>`connect returnPressed @48572` |
| `D-KeyMappingDataTableWidget/applyCategoryButton` | QPushButton | 构造 @48486；`connect clicked @48571` |
| `D-KeyMappingDataTableWidget/triggerTypeComboBox` | QComboBox | 构造 @48775；`connect currentIndexChanged @48891` |
| `D-KeyMappingDataTableWidget/pressTimeSpinBox` | QSpinBox | 构造 @48778；`connect valueChanged @48905` |
| `D-SettingTransferDialog/filePathEdit` | QLineEdit | 构造 @50593；`connect textChanged @50640` |
| `D-SettingTransferDialog/browseBtn` | QPushButton | 构造 @50597；`connect clicked @50637` |

### 动态 macro（2 项）

定义和连接：[qmacrolistdialog.cpp](../../QKeyMapper/qmacrolistdialog.cpp)。局部循环变量（例如 `cb`、`btn`）代表按数据重复创建的一类入口；重命名时保留此定位 ID 或明确记录迁移。

| 稳定定位 ID | 类型 | 定位／绑定线索 |
|---|---|---|
| `D-QMacroListDialog/m_CategoryFilterAllCheckBox` | QCheckBox | 构造 @334；`connect checkStateChanged @356`<br>`connect stateChanged @378` |
| `D-QMacroListDialog/cb` | QCheckBox | 构造 @439；`connect toggled @451` |

### 动态 panel_runtime（1 项）

定义和连接：[qvbuttonpanel.cpp](../../QKeyMapper/qvbuttonpanel.cpp)。局部循环变量（例如 `cb`、`btn`）代表按数据重复创建的一类入口；重命名时保留此定位 ID 或明确记录迁移。

| 稳定定位 ID | 类型 | 定位／绑定线索 |
|---|---|---|
| `D-QVButtonPanel/btn` | QToolButton | 构造 @205；`connect pressed @227` |

本清单合计 387 个定义／构造定位项（UI 316，动态 71），不含运行时重复实例数。动态动作族见上节；DSL、设备／驱动能力及快捷键事件仍须独立盘点。
