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

## DELETE-GUI-111 原GUI删除基线

`settings_remove_gui`配合`QKM_CONTEXT_VARIANT=normal_last／common_first／common_all`，期望普通启动载入固定Common76Seed合成INI：普通A→B及共通F13／F14两行，共通功能启用但普通表不附加共通。每场独立副本／INI，MCP保持默认关闭；不使用个人配置。实际Qt Tab／表格鼠标和Ctrl+A／Delete走原产品删除，禁止设置选中控件值或直接调用删除／加载函数代替基线。

结果在settingsGeneral，记录完整源before／after、目标源、实际current_row／column及加载代次，七项检查包含全剩余行／其他快照源、dirty、选择与INI保持；退出／实际桌面／权限和工作计数仍由原生命周期结果及普通控制器核对。111版本产品删除函数未改，后续提取后的同场GUI事件可与其完整快照及选择精确对照，formal_mcp_delete=false；本用例不是MCP删除或串行重置，也不覆盖过滤／附加共通／混合来源完整矩阵。当前具体结果和停止位置只维护在唯一主计划／C1证据。

## DELETE-112 共用单源删除边界

`settings_remove_shared`独立验证私有共用入口，沿同一合成INI启动。无效Tab／空行／负行／越界／有效与越界混合列表、缺失源数据均须拒绝且完整源、dirty、加载、INI及当前单元格不变；逆序重复行只删除一次，保留非目标源，执行原刷新、鼠标点更新、接替选区及dirty。这里直接调用共用函数只证明其参数和效果边界，不能代替`settings_remove_gui`的独立真实GUI基线，也不证明MCP权限、稳定引用、预览提交或串行重置。

## DELETE-113 正式源行删除

`settings_save_apply`配合`delete_normal／delete_common_first／delete_common_all／delete_stale／delete_read／delete_hidden`复用87的真实认证请求夹具，启动同一Common76Seed合成INI；Common及stale变体通过实际Qt Tab点击选择Common。普通SDK调用维护模块 [mapping_deletion_acceptance.py](../../../mcp_bridge/tests/mapping_deletion_acceptance.py)，批准仅Read+Configure（read变体只Read），不授Run。模块的`fixture_bytes()`从既有common_save_fixture.ini.example去除Note、Category、两类Comments和CommonMappingData五个可选键行，精确重建111的1171字节种子，不另复制样例；每场必须将该内容复制到自有隔离runtime/keymapdata.ini，SDK拒绝其他INI。`.client`必须匹配实际SDK peer PID／创建时间／EXE；`.done`明确报SDK结果、successful_saves=0。正常关闭核对exit0及全部尾部工作，不能只看STOPPED。

固定`.delete-open-editor／delete-close-editor`仅真实Qt双击／Esc，`.delete-gui-remove`在stale变体沿原Qt Delete改变实际源，不调用加载／删除函数替代GUI基线。`.delete-table-edit／delete-table-cancel`用原Qt editItem形成未提交Z草稿再Esc取消，`.delete-popup／delete-popup-close`形成受控QMenu，hidden变体的`.delete-hide-row／delete-show-row`注入及恢复表格隐藏状态；三者明确是准入保护组件注入，不是独立GUI删除／筛选基线。SDK应验证这些状态下预览／提交拒绝，恢复后源／dirty／INI不变，并检查正式删除的完整剩余字段、去重／旧引用、取消／确认／历史结果。其他普通Tab附加Common、真实筛选及独立备份等仍另验，串行重置未接通。

## DEMO-P2-114 只读就绪预检

`settings_save_apply`／`demo_ready、demo_ready_read、demo_ready_nonempty、demo_ready_dirty`复用同一真实认证夹具；批准387，read变体只Read。使用P0的安全Demo104Seed单表空源INI，nonempty只在合成INI预置A→B，dirty在建立观察基线前以真实Qt键事件编辑保存名称为DemoReady114Draft，不用setText或直接设置dirty。之后SDK只读预检，Native独立核对完整源、GUI名称、dirty、加载／sourceEpoch和INI均不变，无保存或映射动作。

维护模块 [demo_readiness_acceptance.py](../../../mcp_bridge/tests/demo_readiness_acceptance.py)要求普通宿主依据已核验Start记录／实际PID创建当前用户私有ACL的`host-record/demo.json`，并用显式--demo-record启动正式stdio桥接；不复制配对／缓存PID，不以该测试记录冒充正式预热已交付。验证就绪／权限／非空／dirty、严格无参schema、错误创建时间／实例／Session／EXE／桌面／额外字段、损坏记录、恢复及原连接保持。原manifest启动／必要关闭走公共登记，SDK与记录写入均普通权限，无Default、系统输入或驱动变更。

## DEMO-P2-115 非行保存足迹

`settings_save_apply`／`demo_context、demo_context_read`沿相同安全Demo104Seed空源INI、真实认证和隔离私有桌面，分别批准387／Read。SDK模块 [demo_context_acceptance.py](../../../mcp_bridge/tests/demo_context_acceptance.py)验证上下文只读参数／权限、正式A→B新增后签名稳定、实际Qt保存名称变化／恢复、普通宿主合成INI注释字节变化／恢复、正式单项窗开关及删除后签名稳定。固定`.demo-name-change／demo-name-restore`只向保存名称投递Qt键事件，不设置控件值或dirty；此时dirty已true，源修订保持而非行签名须变化。

第一次单行上下文请求另对真实C1候选副本做10项组件检查：74项源字段排除，Tab元数据／GUI名／磁盘／pair／epoch／加载改变检出，无效候选无签名及原全保存载荷不受副本删除影响。组件副本不是GUI修改基线。Native独立验证源完全复原、原INI／加载保持、只有两次源修改、名称恢复、dirty按原行为保留和窗口已关闭／未启动映射；不把本用例或签名比较记成已交付串行服务。

## DEMO-GUI-119 映射高级设定原GUI基线

固定settings_save_apply／advanced_gui沿独立合成INI和真实认证SDK；维护模块[advanced_gui_acceptance.py](../../../mcp_bridge/tests/advanced_gui_acceptance.py)先正式MCP建立A→B，再由实际Qt Tab／原按钮打开高级窗，自身Qt客户区抓图，Escape关闭及真实Tab点击恢复原页，最后正式删除本轮归属行。完整源、非行足迹、输入指纹、INI及生命周期核验保持；不编辑任何设置值、不启动映射、发送系统输入或改变驱动。

这里只形成独立GUI显示证据，尚非正式高级窗MCP接口。仅沿已登记私有桌面／原manifest测试入口执行；SDK、构建及图像解码保持普通权限，原生GUI线程只生成固定out证据。结果与缺口仍只记唯一主计划。


## DEMO-P1-120 高级角色只读捕获

同一SDK模块以capture=True运行advanced_gui／advanced_gui_no_capture，分别批准387／131；沿119原GUI入口取得自身帧，并调用正式显式高级角色查询／PNG捕获，验证默认两角色兼容、旧源／旧窗、非windows实体参数拒绝及无Capture不返回MCP图片。固定GUI证据文件与正式MCP图像分别计数，不把测试观察材料当成权限授予。最后仍核对原GUI恢复／完整设定与源，正式删除归属行、正常退出及公共登记清理。


## DEMO-P1-121 高级窗归属与数值草稿

`settings_save_apply`／`settings_window`批准387，`settings_no_show`批准259（无Show）；普通SDK模块[mapping_settings_acceptance.py](../../../mcp_bridge/tests/mapping_settings_acceptance.py)先正式建A→B，原Qt Tab／按钮与Escape形成GUI拒绝接管基线，再正式高级open／capture／close两轮。固定.settings-edit／settings-restore以真实Qt Ctrl+A和字符事件将鼠标X倍率3→4→3，核对草稿关闭拒绝、图片未变化标记、恢复及旧window_ref；不运行映射或系统输入。无Show零GUI／MCP图，正式回收本轮行。

未提交数字另在隐藏高级窗组件中临时关闭原keyboardTracking，以相同Qt事件形成text≠value，检查只读指纹拒绝，再恢复文本和原属性；它是明确的准入组件注入，不替代独立GUI基线。另验非GUI线程拒绝，保留原ActivationChange。最后核对完整源／非行足迹、主窗页、输入指纹、INI／加载／dirty及正常退出和公共登记五项清理；产品启动／关闭才用已有授权缓存，SDK／构建／图片消费保持普通权限。


## DEMO-P2-122 高级串行view

沿demo_sequence／demo_sequence_read及现有demo_scene_acceptance.py，views固定(mapping_advanced,item_setup)，Read负例也明确请求高级view。C++夹具不变，双Qt复用121已验本机制品及安全初始INI，普通SDK正式建例／高级图／归属安全关闭／源行回收后接默认单项。高级图片输入未变标记必须true，SDK另核对固定高级窗已隐藏；Native继续独立核对完整空源／INI／dirty／代次和无runtime输入，实际目标退出及公共登记清理分别留证。原GUI基线119及归属／草稿定向121保留其原范围，不用串行助手冒充独立GUI。
