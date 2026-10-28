# 发送任务生命周期维护夹具

进度和授权以[唯一主计划](../../../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md)为准。本目标包含真实产品源码，用 `QKM_LIFECYCLE_TESTING` 替换入口并增加固定测试，不向正式产品提供测试控制命令。

使用本机正常 MSVC 开发环境及对应 Qt kit，在 `out/tests/sendinput_lifecycle/<kit>/` 构建 `sendinput_lifecycle_test.pro`，参数为 `CONFIG+=release CONFIG-=debug`，JOM 目标是 `Makefile.Release`；源码列表或继承的 `.pri` 变化后先重新运行 qmake。Qt6／Qt5 与 ASan 使用不同构建目录；编译、Git、控制器及观测保持普通权限，保留原 manifest。

测试控制器先创建并持有私有 Windows 桌面，再通过仓库 `scripts/test_process.ps1` 的 `Start-QkmTestProcess` 启动隔离 runtime。把以下变量作为该子进程的 `EnvironmentOverrides` 传入：

- `QKM_LIFECYCLE_CASE`：固定用例名。
- `QKM_LIFECYCLE_RESULT`：本仓库 `out/` 下独立 JSON 结果文件的绝对路径。

必要启动／关闭遵循已授权的 gsudo 控制器 PID 缓存，默认空闲五分钟、finally 关闭并核实；不提权开发流程或修改 manifest。产品初始化／退出仍可能连接或复位本机 FakerInput 并执行 ReleaseAll，不能把 NoOp 等同于完全没有既有驱动副作用。

WRITE-64 源上下文回归：

| 用例 | 检查 |
|---|---|
| `repeat_source_context` | 全局表为不同的 decoy，真实异步宏／重复子任务始终使用父源并完成规定轮数；数据保留至作业排空 |
| `hold_source_context` | 直接观察保持序列的派生提交及尾键释放，索引、原键和源指针一致，不提交真实输入 |
| `unlock_source_context` | 从父源解锁并读取同一来源的抬起内容，不改变 decoy 的锁状态 |

修复后要求进程 exit 0、`passed=true`、`sourceContextTrace` 全部来源匹配、`workAtExit=0`、`producersAtExit=0`；检查实际桌面／EXE／权限和退出，不能只看计数。修复前对照应单列预期 exit 2 与具体不匹配轨迹，崩溃或超时不是有效红例。相关回归包括 `repeat_macro_3`、`gui_stop_after_timeout`。

NoOp 和 ASan 结果用于指定生命周期／来源边界，不能替代 Default 真实输入效果、普通 Release 性能或其他系统／设备验收。

## UI-73 正式翻译与实际入口

`ui_localization` 仅验证实际QKeyMapper类的UI和嵌入资源，不开始映射或停止流程。额外环境变量：`QKM_UI_LANGUAGE`为0／1／2（中文／英文／日文），`QKM_UI_QM_DIRECTORY`指向本次正式QM目录（独立读回作为期望，不安装以遮蔽产品资源），`QKM_UI_CAPTURE_STATE`为自有out下的ready JSON路径。

用例通过实际语言选择刷新高级设定入口、托盘及控制窗口，检查4项新增提示、NoFocus／非默认按钮和Qt鼠标打开／关闭。停止与超时仅临时设置显示状态并立即还原，核对源代次、清理阶段和准入未改变；这不证明真实停止、超时清理或输入效果。结果在`uiLocalization`，包含逐项断言和`mapping_started=false`。

高级设定在MCP窗口激活后按现有逻辑自动关闭，因此截图分两阶段：先读取ready并捕获可见的control_hwnd，再由普通控制器写同路径`.request`文件；用例关闭MCP并重新显示高级设定，发布`.advanced` JSON后捕获可见的advanced_hwnd。保留PID／桌面／可见性／HWND所有权检查，不截图已经关闭的窗口。ready发布20秒后正常退出；未完成的截图不能据逻辑断言通过而记为视觉通过。

保留正式产品及构建EXE的requireAdministrator manifest。按workflow可仅为本UI用例使用out内临时asInvoker副本和普通私有桌面PrintWindow，实际检查资源与界面，不把它当作管理员权限／驱动验收；不得用该副本运行映射或其他驱动用例。所有编辑、构建、截图仍为普通权限，原始运行材料留out，不构成跨机依赖。

WRITE-64 停止态释放回归 `inactive_virtual_release`：直接回放键盘 A／鼠标左键的虚拟抬起结构体，不注入系统输入。`virtualReleaseTrace` 共 18 条，格式为 `phase:source_present:virtual_state:keyboard_result:mouse_result`：

- phase 0：GUI 空闲、Worker STOPPED、停止清理已结束；Force／BurstStop 均保护仍按住的原键，不依赖源中是否存在该键。
- phase 1：GUI STOPPING、Worker STARTED、准入关闭；保留冻结源判断。
- phase 2：GUI STOPPING、Worker STOPPED、准入仍关闭；同样保留冻结源，不能只按 Worker 状态当成清理完成。

三个阶段中明确 bypass 均放行，虚拟按键记账均清空、作业计数归零；用 `gui_stop_wait`／`gui_stop_after_timeout` 复核实际停止任务协调。ASan 副本须包含本次编译的测试 EXE，不能用普通版本替代。

MAP-67 共用新增操作维护用例（不代表正式 MCP 验收）：

| 用例 | 检查 |
|---|---|
| `mapping_add_plain` | 预选 Combo 值作为前置条件，再以 Qt 鼠标事件添加 A→B；对照共用提交的完整映射码、派生值、dirty、表格与选区；显式插入、解析负例及组合／长按／双击／序列／停止动作键 |
| `mapping_add_conflicts` | 两个普通源与共通新增冲突；纯分析无修改，GUI 的 No／Yes 按钮独立驱动，取消和拒绝确认均无部分修改；提交结果与通知一致 |
| `mapping_add_guards` | 注入 CHECKING／运行／STOPPING，关闭准入、在途生产者和加载；GUI 事件及共用操作均拒绝且不产生部分修改 |
| `mapping_add_stale` | 分别改变加载代次、结构、冲突、当前目标和运行活动；各测试独立保留其他前提，不让旧 epoch 掩盖实际断言 |
| `mapping_add_setup` | 固定 A→A 输入适配器打开新增本地行，避免误指向追加的共通行；这是适配器检查，不是该入口的完整 GUI 事件基线 |

结果在 `mappingAddition.checks`，所有检查、进程退出及零尾部作业均须通过。用例核对 INI 未变化，拒绝分析／提交时全源字段、派生值、dirty 及通知无修改。正式 MCP 两条独立路径、冷启动、Default 实际输入、打开编辑会话和保存范围仍须在业务接入时验证；源引用与内部决策戳不能作为编辑许可。

EQ-68 `mapping_config_equality`：`mappingEquality.checks` 覆盖同步组单独变化及 Qt 列表不等、三个 Pure 列表及 LockState 分别仍等、既有备注／映射差异、编辑窗刷新不新增 dirty，以及同步组 Apply／Revert。等待主窗口的 queued `settingsApplied` 后检查 dirty，不把即时赋值误当通知完成。修复前仅两项同步组比较失败（exit 2），修复后全部 16 项通过（exit 0）；同时检查实际私有桌面及退出作业清理。

本用例不复现同步组保存故障、不发送系统输入、不启动映射，不代表正式 MCP 陈旧预览通过。MCP 继续使用含同步组／派生／运行采样的完整源快照及外层身份、权限、状态和到期守卫，不能用配置行相等替代完整修订。

MAP-67-C1-A辅助保存用例：先在同一自有runtime运行`settings_serialization`，正常退出后保留该合成INI，再启动独立进程运行`settings_serialization_cold`。runtime必须在out且为EXE所在目录；不复用个人INI。结果位于`settingsSerialization.checks`，要求实际私有桌面、进程身份／权限、exit0及零尾部作业。此数据／文件检查可用明确记录的临时asInvoker维护副本，正式manifest不变；不能据此证明High／驱动或权限通信行为。

用例核对忽略规则的全部字段／枚举及顺序，普通／通用宏的作用域、Unicode、备注和含空项的QVariantList注释，同步组说明忽略空值及保留ID；纯序列化不写盘／dirty且不保留可变数据别名。复用原保存／加载入口验证精确内容、空组名无操作、空列表清除旧值、无关配置／全局键不变及独立冷进程读回。主进程为身份核对短暂保留两秒；没有系统输入、映射启动或正式MCP保存请求，不能把辅助字段读回当成完整保存闭环或原保存失败路径已重新通过。

C1-B1的`settings_general_save`在自有runtime准备合法A→B、四种按键收藏／历史及固定窗口位置，再通过Qt鼠标事件点击真实“保存设定”按钮。检查共用内容生成不写盘／dirty／加载或映射，语言采样的中英日／回退语义，以及实际保存、重载、dirty和几何基线。可用合成INI的SaveWindowSize／SaveSplitterPosition覆盖开启／关闭条件。

提取前先用旧生产保存代码生成基线INI；提取后通过`QKM_LIFECYCLE_SAVE_BASELINE`传入同一out任务目录下的baseline-*.ini。两份磁盘图像用独立QSettings解析后比较完整键集合／值，避免同路径共享缓存中的QStringList与磁盘解析表示造成假失败；原始INI字节也由控制器核对。不得用新共用函数代替旧GUI基线，不把本用例当作正式MCP保存或完整冷启动验收。

SAVE-71共通备注用例同样只操作自有out runtime：`common_comments`准备含按下／抬起序列备注的共通行，关闭共通表后用真实GUI保存；保留INI再独立运行`common_comments_cold`。`common_comments_attached`覆盖共通表开启时的源行和备份；`common_comments_legacy`读取修复前、没有两个可选备注字段的合成旧文件，应保持空备注且不重写文件。冷读需保留对应INI，不能重新复制空模板。

结果位于`commonComments.checks`。修复前只接受两条备注保存／加载检查为预期红例，其他配置、加载完成、停止态和零尾部作业必须通过；修复后全部通过。单行其他配置比较复用MAP_KEYDATA::operator==，其Pure／LockState排除语义不变；不能据单行比较替代MCP的完整修订、身份、权限、状态及到期校验。临时普通副本不证明High／输入效果，也不是GUI备注编辑与MCP编辑的完整对照。

## DEMO-P1-108 窗口捕获维护范围

`settings_save_apply` 的固定连接变体 `demo_mcp_capture` 沿P0演示夹具，只在空白合法隔离INI／私有桌面上经正式MCP内存新增A→B；固定配对Read+Configure+CaptureWindows，无ShowWindows／RunMapping。`demo_capture`保留Read+Configure，用于正式捕获权限拒绝。维护用例的Qt表格双击／Esc打开关闭真实单项窗，不是正式MCP展示接口。

新增固定GUI标记仅向本实例单项映射键行发送Qt Ctrl+A／C／B事件，回读实际草稿与源仍B，并恢复；自有主窗固定Qt最小化／恢复标记检验截图拒绝且不自动恢复。没有setText、系统输入、Default、映射启动或驱动变更。公开登记必须绑定实际维护EXE及固定case／输出根，普通SDK消费原生MCP ImageContent并写自有out证据，源／INI／dirty／代次及退出清理分别核对；测试探针PNG不替代正式MCP图片。关闭后Qt5／Qt6可能分别保留／销毁原生句柄，应按实际window_ref可用性检查业务拒绝或SDK schema拒绝，不把null当有效引用。

图片字段／范围与用法见[mcp_bridge说明](../../../mcp_bridge/README.md#受限真实窗口图片p1-108)；具体执行及缺口只在唯一主计划／C1证据记录，不将本用例称完整演示服务或QQ送达。

## DEMO-P1-109 正式打开与独立GUI对照

`settings_save_apply`／`demo_mcp_editor`沿P0固定演示夹具，配对仅Read+Configure+ShowWindows+CaptureWindows（387），无RunMapping。普通SDK经正式业务MCP建例，再用qkm_open_mapping_editor打开源行；本变体的capture-item标记只观察已经显示的真实单项窗和WA_ShowWithoutActivating已恢复，不再投递双击。`demo_mcp_capture`不授ShowWindows，正式open应拒绝，再按既有Qt真实双击形成独立GUI基线。不得把两条路径混写为正式打开通过。

同实例Qt Ctrl+A／C／B与Esc用于草稿保护／关闭，均沿原固定标记；不得用setText或调用加载函数替代独立GUI基线。图像通过正式MCP ImageContent消费，窗口来源、源／INI／dirty／代次与退出分别核实；普通Default控制器只采样是否本测试PID在前台，不把用户自行换焦点判为产品干扰。正式安全关闭／串行重置和预热仍待，维护退出／Esc不替代产品接口。用法见[桥接说明](../../../mcp_bridge/README.md#私有后台打开源行设定窗p1-109)。

## DEMO-P1-110 安全关闭及连续窗口

`settings_save_apply`／`demo_mcp_close`固定配对387=Read+Configure+ShowWindows+CaptureWindows、无Run。先按旧Qt双击／Esc形成只GUI打开的拒绝基线，然后正式MCP open／close执行两轮；旧window_ref不能关闭新窗口，C草稿需保留，恢复B后才允许关闭，最终由正式query核实不可见／源INI不变。`demo_mcp_capture`缺Show，正式close须permission_denied，再由原GUI回收；不把此GUI回收计为正式关闭。

本变体GUI基线关闭后固定轮询继续处理C／B标记，stage4不再次发出Esc；最终case由SDK明确done和原有清理退出，不因STOPPED或先前关闭标记推断当前窗口已关闭。输入赋值的摘要组件与真实GUI业务基线分开记录。源行删除／演示重置及服务预热仍待；用法见[桥接说明](../../../mcp_bridge/README.md#不丢草稿的受限关闭p1-110)。
