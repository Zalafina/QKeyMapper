# 能力与 GUI／MCP 等价性矩阵

[返回唯一计划与进度](qkeymapper_mcp_automation_plan_codex.md) · [验证记录](validation.md) · [目录规则](../README.md)

## 当前状态与填写规则

日期：2026-10-28；初始源码基线：`d49a9f57`。**逐控件入口及68条关键链路已盘点。通信／配对、选定状态／目录／源查询／帮助已接；MAP-67-B1增加基本映射内存添加预览／提交／取消／结果查询，SAVE-95接保存只读预览、SAVE-97接完整范围／覆盖确认后的正式保存提交与分段结果，当前共15工具、14项业务能力（107增加windows.read，108增加windows.capture，109／110增加mapping.editor.open／close）。完整增改删、完整保存生命周期、MCP启停及全量GUI等价性仍未完成。** Python字节锁与PC-W11接续见ENV-39，缓存见CACHE-40／CACHE72；入口和链路数量不等于已支持能力数。

UI-73：新增MCP界面及4项主窗口提示已接英文tr／正式三语言QM，NoFocus／ClickFocus遵循用户选择；双Qt各36项、实际产品类双Qt三语言66项及私有截图见[本片证据](ui_localization_validation.md)。仅UI与资源，不新增工具、业务能力或运行态编辑许可；High／Default／Win11边界另列。

DEMO-P1-105：独立ShowWindows／CaptureWindows权限及新配对默认不授予、既有配对GUI追加批准已接；复用原updatePermissions保留密钥、递增权限代次并断开旧连接，取消／无新增不写存储，保存失败关闭接口。双Qt各51项组件／5项IPC、7项Python及12幅正式三语资源私有桌面截图见[同一C1证据](map67c1_validation.md#demo-p1-105独立窗口权限及追加授权2026-10-28pc-w11)。仅授权基础，仍12工具／10业务能力；正式显示／捕获、目标绑定、产物／预热及QQ未交付。

DEMO-P1-107：windows.read／qkm_query(entity=windows)正式只读观察固定main／item_setup角色及真实身份，窗口引用绑定实例／配对权限代次和生命周期；不接任意原生目标，不创建或显示／捕获窗口。双Qt组件及正式SDK真实源码维护产品通过，见[同一C1证据](map67c1_validation.md#demo-p1-107正式窗口只读查询及引用2026-10-28pc-w11)。107当时十二工具／十一能力，旧105的10项保留其当时范围；后续正式捕获及源绑定见108，显示／完整场景仍待。

M0.2 必须把每个模块拆到实际设置和操作粒度，为每项能力维护稳定 ID、作用范围、类型／单位／约束、GUI 事件入口、允许状态、共用业务状态校验、实际有效状态／缓存／通知及副作用、持久化／生效时机、依赖与风险、阶段、用例及结果。模块行中列出的文件用于导航；未逐项追踪的处理链路明确标为待核对，不在状态限制未核实前声明支持。

同一功能存在多个 GUI 入口时，需要列出每个入口及其调用关系。测试必须比较实际数据、派生状态、联动、保存恢复及适用最终效果；不能只核对控件或将请求值原样回显。完成判据以主计划为准。

EQ-68：MAP_KEYDATA配置相等已补同步组，三Pure列表／LockState排除不变；四处Item编辑变化检测继续共用。双Qt私有各16项见EQ-20261024-68。B1的Qt6 Default另验GUI仅改同步组使正式预览陈旧、提交零部分修改及还原。MCP仍复用已含同步组的完整快照／哈希，无新增operator==调用；身份、权限、内容、到期、状态和代次检查保留，Qt5／Win11该正式分支未验。

MAP-67-B0：cac101be的纯模块参数／预览及结果存储完成选定双Qt检查；当时未接产品，内部schema和结果存储本身不授权写入。后续B1正式接线证据如下，不能倒填为B0当时已验。

MAP-67-A：两条GUI新增入口共用准备／提交和明确插入源行，状态／加载／目标／冲突提交前重检；选定双Qt私有用例见MAP-20261024-67-A。该内部决策本身不是完整修订或MCP许可；B1复用它并补外层守卫，保存冷启动另行接入。

### 基本映射内存添加：MAP-67-B1

选定证据见[MAP-20261025-67-B1](map67b1_validation.md)。当前仅大写字母／数字的单项新增和当前已附着源表；不自动停止、切Tab、关闭编辑窗、保存或开始。完整内容修订继续覆盖全部源及目标上下文，实际提交复核会话／权限／请求、运行修订、5秒写前预算及120秒预览到期；预算不强制打断已开始的同步提交。

| 操作／GUI入口 | 允许状态与共用校验 | 实际副作用／持久化 | 选定验证 |
|---|---|---|---|
| mapping.add.preview；对应GUI键选择与添加意图 | Read+Configure；现有mappingAdditionAllowed和prepareMappingAddition，IDLE／完整清理、当前源、安全访问、无编辑窗 | 仅产生有界预览；不改源／dirty／INI | 双Qt正式产品：独立GUI基线、权限、目标切换／陈旧、取消／替换／到期；源／共通联动预告 |
| mapping.add.apply；GUI添加按钮／既有菜单 | 上述守卫及完整修订提交前重检，consume-once；共用commitMappingAddition | 主数据、派生列表、冲突禁用、界面和dirty更新；saved=false／started=false | 双Qt全部字段／默认值、共通禁用确认和拒绝零部分修改、重放不重复；Qt6 Default实际A→B／释放、运行中GUI禁用与MCP拒绝、同步组陈旧、独立悬浮编辑窗拒绝 |
| mapping.preview.cancel；GUI无同名入口 | Read+Configure及预览归属；无需为了取消停止或关闭编辑窗 | 仅撤销未执行预览，不撤销已提交映射／停止动作 | 双Qt取消／替换／到期，Qt6编辑窗打开时可取消；未知或已消费结果按明确语义返回 |
| operation.read；GUI无同名入口 | Read及实例／配对／权限代次归属 | 仅查询本进程有界历史，失败operation是查询成功；不写盘 | 双Qt断线重连、重复提交、失败结果及重新配对旧结果拒绝 |

运行状态含sampled_at_utc，比较只排除此已校验时间；编辑源、已保存配置和运行态不混用。基本添加不触发整个GUI输入采集历史的写入；全量保存须独立说明其范围并保护其他草稿。Qt5／Win11真实输入、完整编辑会话、STOPPING／超时正式写请求及其他操作仍待验，不能将B1标为M1完成。

### 完整保存：SAVE-95预览／SAVE-97提交

| 操作／GUI对应 | 允许状态与共用校验 | 实际结果／限制 | 已选证据 |
|---|---|---|---|
| configuration.save.preview；GUI完整Save意图的预告 | Read＋Configure，明确setting_name，成功加载及pristine_idle或有效completed_cleanup，无未结算编辑；完整候选／磁盘／缓存及预算复核 | 全部目标／共享键、其他草稿范围、名称效果及实例／组／修订；预览只存有界元数据，不写模型或INI；SAVE-97后apply_supported=true | PC-W11双Qt正式stdio到维护目标生产处理器，普通／Read／全局六场景；完整不变性、目标／新建／覆盖、替换／取消及误提交拒绝，见[同一C1记录](map67c1_validation.md) |

SAVE-95时保存仅预览，误提交按unsupported_capability单次消费；该历史结果范围保留。SAVE-97正式提交须confirm_scope及必要的独立confirm_overwrite，复用SAVE-96重采集与写入栅栏，按原目标记录主文件／重载／备份与失败结果；保存和映射确认不能混用。当前／新建／覆盖／全局、写失败恢复、备份告警、成功重放及添加后独立冷读经双Qt正式stdio维护接线通过；12工具／10能力。原映射线格式兼容，满进行中槽仍保留预览、原截止不续期。完整生命周期和全量等价性待验，受限配对不替代普通Release GUI批准。DEMO专用会话／窗口／捕获／产物为[主计划新增待实现范围](qkeymapper_mcp_automation_plan_codex.md#demo-session基于预置-release-的私有演示与截图会话2026-10-27-新增实施目标)，当前没有图片或QQ发送能力；Win10渠道末期单列。

SAVE-96只补私有保存执行协调：存储元数据重采集，真实授权／原到期／总验证预算及清理许可在写入前复核，源互斥保持到实际重载。双Qt五协调器／两重校验各199项，14进程398项通过；新建／覆盖／全局及拒绝不变性、实际旧表销毁处互斥探针已选验，见同一C1记录。未开放正式保存提交或增加能力，不能据此称C1-C／M1完成；实际输入竞态与正式业务生命周期待后续。

SAVE-97证据：28项Python、双Qt七组件槽，18个正式stdio维护进程／130项CPP检查／443次含连接轮询的SDK调用／14次保存。八组独立GUI基线全组／键／编码值相等，仅当前目标两组原字节相等；范围与键序边界见[同一C1记录](map67c1_validation.md)。不增加运行、窗口捕获或QQ能力。

SAVE-98仅补维护证据：双Qt全局／覆盖真实GUI Save／Yes及四独立冷读，四对完整INI值相等（全局两对原字节相等）；正式重载失败明确已写盘、不备份、加载invalid_mapping_data和重复ID不再保存。最终10进程／52项通过，首轮助手失败与原退出码缺失另列，见同一C1记录。产品仍SAVE-97，12工具／10能力不变；并发、断线和完整生命周期仍待。

SAVE-99：预览后真实Qt名称输入／全局选择改变上下文，旧保存确认拒绝且完整GUI后状态不变；新预览确认才保存。产品一行将旧目标不兼容归stale_preview，保留Qt6红例exit2；四个有效双Qt场景44项（Qt6名称复用未受影响原结果）。能力／权限不变，见同一C1证据。

SAVE-100：已接受且写盘后的正式timeout／qkm_disconnect，经同实例同SDK进程原配对重连，原ID结果可查且只保存／重载一次。双Qt四进程36项、7项工具结果通过；每场一次配对，取消只结束本端等待，已接受操作可能继续。产品执行／权限／原10秒时限不改，完整生命周期与跨重启恢复仍待；见同一C1证据。

SAVE-101：双Qt实际GUI启停／超时后完成清理，再正式保存四场44项通过。Worker STOPPED但所有者仍在途时拒绝源读取／保存，完成后新确认只保存一次并销毁旧表；旧源与未消费预览失效，FilterKeys完整状态不变。仅维护夹具／证据，首次preview_superseded脚本失败原件保留，未改产品。物理输入、设备及更广生命周期仍待；见同一C1证据。

SAVE-102：真实清理后正式新增X→Y、全范围确认保存／源重建及独立冷读，双Qt四场42项通过。新增不写INI，旧修订在新增／重载后失效，暖冷实际A→B／X→Y及完整INI／备份一致；旧实例结果未知。只补维护断言，产品不改，首次SDK结果结构错误原件保留。物理按键、配对跨重启复用与完整生命周期仍待；见同一C1证据。

SAVE-103：实际停止回切Global→McpReturn后新确认保存Q→R，双Qt24项通过。产品一行对称化保留Global目标守卫，旧预览stale_preview、新非法目标invalid_arguments；停止／写盘／权限不改。原产品红例及主表复用维护断言失败两份exit2保留，修正后通过；Widget可复用，业务引用仍按修订失效。实际前台自动匹配／输入与完整生命周期仍待；见同一C1证据。

DEMO-P0-104：新增内部自身Qt窗口PNG捕获底层，Win11双Qt正式MCP从空白建立A→B后真实GUI main／单项窗采样；两进程12项、四图中文与A／B已由Agent检查，DPR1／约45–51KB。捕获与Esc关闭保持源／INI，关闭／错误线程拒绝。无正式截图端点、授权或服务交付，工具仍12／能力10；独立权限、绑定／产物策略、免Python预热与QQ均待。首SDK小记录读取器上限失败exit2原件保留，见同一C1证据。

### 评审接续：REVIEW-59

MAP-67-C1-A：原saveKeyMapSetting调用的四个辅助保存包装器共用三个只读序列化函数；未新增GUI或MCP写盘入口，允许状态／保存时机沿原调用，不能据纯函数放宽业务门禁。字段／Qt类型、OrderedMap顺序、空组名早退、空列表清旧值及同步组空说明过滤保持。双Qt普通私有维护保存各14／独立冷读各11通过，含无关键／dirty不变，见[SAVE-20261025-67-C1-A](map67c1_validation.md)。临时asInvoker副本仅证明数据／文件逻辑，完整保存、High行为和其他草稿保护后续验收；该基础切片当时仍12工具／8能力。

C1-B1：GUI保存的全局／显示／按键收藏历史值共用只读生成，OrderedMap保持写入顺序与Qt类型，保留原语言采样时机。双Qt三种条件的独立旧GUI基线与当前GUI共六对：每个当前场景9项、252键及INI字节完全相同；见同一C1证据。配置／映射及其他保存依赖、失败／正式MCP／冷启动仍待后续覆盖，不增加工具或放宽运行态门禁。

SAVE-71：共通独立备份补两类序列备注的四行保存／读取；旧文件缺字段仍为空。双Qt各两项预期红例，修复后每套GUI保存／冷启动／附着源／旧格式共22项通过，见同一C1证据。其他配置比较仅在维护测试复用operator==，MCP生产继续完整快照／哈希；未开放保存或备注编辑，Pure／运行态语义不变。

唯一主计划记录[评审附件](review_and_improvement_recommendations.md)的采纳决定与进度。BRIDGE-59 已修复 R1 取消恢复；BRIDGE-60 已修复 R2 MCP 错误语义，协议／线程 20 项、工具结果 6 项、双 Qt SDK 各 12、当前 Qt6 正式产品 11／源查询 9 场景通过，具体范围见验证记录。公共停止及输入路径在接口关闭时仍受新增逻辑影响，ENGINE-61 原功能回归与普通 Release 四状态性能对照尚未完成，不得扩大公共引擎改动或据 NoOp 宣称无退化。QUERY-62 保留完整源内容／身份／权限修订与陈旧引用拒绝，先做参数前置校验，目录拆分／缓存待独立契约和测量。

首个写入能力同时交付去重、取消／失败和断线结果查询，并区分编辑／已保存／运行配置与采样状态；保存范围不能连带吞入其他草稿。按键名称、参数范围／单位／默认值、依赖、示例、当前允许操作和错误下一步随能力接入。当前12工具含基本Read+Configure新增，source.read的初始静止及完整清理许可见STOP-57；未开放RunMapping工具。

BRIDGE-60：现有调用失败和 accepted=false 使用 isError=true，保留 ok／error 并提供受控 message／next_steps；成功查询失败状态或失败 operation 数据不误标错。SDK schema 错误仍由 SDK 拒绝，产品业务错误保留有界解释（UTF-8 ≤1024 字节，错误码 1–64 个限定字符），原生／未知错误不泄漏异常文本；七工具与输出字典 schema 不变。真实 Qt6 产品验证 stale_catalog、stale_source、invalid_arguments、context_busy 等；permission_denied 和失败 operation 数据仅由正式 stdio 固定端点证明封装语义，未新增 operation 工具或实际产品权限覆盖。

ENGINE-61-A：Win10 普通 Qt6 Release 四状态各三轮 A→B、6,000 个正式事件、F24 停止后 A 恢复及 GUI 空闲／清理通过，9 项维护工具保护自测通过，见 ENGINE-20261024-61A。仅新增原功能基础证据，不新增 MCP Run／Configure 能力；source.read=pristine_idle 不变。连发／锁定／序列等 ENGINE-61-B 及唤醒、其他系统／设备等缺口继续作为相关接入／发布门槛。新工具复用固定 UIA／原进程管理，Default 仅显式允许且受 engine61 路径／EXE 名限制，不扩展任意窗口控制接口。

ENGINE-61-B1：Win10 普通 Qt6 Release 的七类原功能基线／当前共 14 组通过，完整目标按键轨迹、原键恢复及停止尾部一致，14 项维护自测通过；见 ENGINE-20261024-61B1。覆盖按住停止、连发时序与释放、锁定、等待／取消和定向停止发送；不等同 B2 回切／重启／退出或设备等全范围验收。TEMP-CACHE-61 只改变本机当天测试缓存期限，不改变产品权限、MCP 能力或业务状态门禁。

ENGINE-61-B2：Tab 往返、自动匹配→通用→停止恢复配置、四轮按住／停止／快速重启共 6 组基线／当前对照通过，完整 key/down 轨迹一致，工具自测 15 项通过。退出等待严格断言未通过：当前及基线复核均有 C 多一次抬起，无迟到 E、产品均退出 0；保留为原功能／发布门槛，不放宽断言或称为 MCP 特有退化。见 ENGINE-20261024-61B2；不新增 MCP 能力，不放宽 pristine_idle。

QUERY-62-A：产品采集源表前与直接快照调用方共用纯参数校验，未知字段、类型／范围、字段依赖及引用／修订不一致可在采集前拒绝；真实源索引／分页边界仍在安全快照后校验。双 Qt 各三组维护、Qt6 普通产品构建及私有桌面正式 MCP 10 场景通过；见 QUERY-20261024-62A。source.read 仍为 pristine_idle，完整内容／身份／授权修订和七工具不变；有效请求的重复编码／哈希未解决，未验证新的运行后读取能力。

EXIT-63：公共退出保留 Hook 释放记账直到 Worker 排空，退出关闭新准入，正常启停及 MCP 开关不分支。普通 Qt6 六次退出、三类直接回归、正反四态和一次夹持的 11 组完整性能／清理通过，Qt5 编译通过；见 EXIT-20261024-63。仅为已测 Win10／Qt6 键盘边界，未强制覆盖所有取消调度／多键／设备，不把诊断或 NoOp 当性能或完整无退化。MCP 能力仍为原七工具／只读，运行后读取及写入继续待实现。

STOP-57：在完整清理及必要回切末尾记录根清理代次／活动修订，加载成功且 IDLE／STOPPED 时可用 completed_cleanup 读取；新活动或修订饱和拒绝，安全源复制继续持互斥。快照新增 cleanup_generation，并参与完整修订，所以相同内容跨启停也拒绝旧引用。双 Qt 各三组组件／产品构建、Win10 Qt6 私有桌面正式源查询 10 场景、Default 按住停止／序列取消／四轮重启／实际配置回切，以及后两场景的正式只读 SDK 检查通过；见 STOP-20261024-57。不是源写入许可或完整停止 operation，不新增启停工具。

QUERY-62-B：查询助手复用一次完整绑定编码，原完整哈希字节和精确 4 MiB／帧上限不变；没有缓存、目录拆分或源复制锁变化。双 Qt 前后基准、精确大小／Unicode 绑定／原修订／陈旧引用边界与双 Qt 产品构建、正式 Qt6 私有源查询 10 场景通过，见 QUERY-20261024-62B。2.44 MB 合成快照的助手耗时减少约 36%–43%，不包含产品采集／锁／IPC／GUI，也不代表整个 R4 或输入性能完成；后续更大优化暂缓，先推进业务闭环。

WRITE-64-A／B：A 父源修复提交 7b188d5c；B 在真正空闲时短路源查找、保护按住原键，清理期间保留冻结源，明确 bypass／FakerInput 及虚拟键记账不变。B 三阶段前红／双 Qt 与 ASan 九绿、Default 标签和四原功能回归通过，详见 WRITE-20261024-64。仅为局部公共修复，原 CPU 波动／失败记录保留，不标记完整 R3、源写入安全或新增 MCP 能力。

### 只读语法与键名帮助：HELP-66

qkm_get_help／help.read 使用既有 Read 权限。没有另造 GUI 编辑入口：original_keys／mapping_keys 直接读取 QItemSetupDialog 初始化并供现有校验器使用的有效列表；basic_mapping 复用现有常量，提供 A→B 示例、分隔符、等待／连发的毫秒范围／默认值及映射项重复含义。名称去空／去重、大小写敏感排序，offset／limit 严格校验，帧超限拒绝；不读取映射源、不写 dirty／INI、不产生输入。允许初始化后的只读查询，本轮实测停止态；运行态及完整语法／设备依赖后续补验。

双 Qt 各两组功能边界（含 init／cleanup 各 4 passed）、正式 SDK 各 12 场景，Qt6 原 manifest 私有产品 12 场景及源查询 10 场景通过。实际键名数为原始 233／映射 244，仅描述该版本；分页与错误、状态及整份 INI 不变性已核对。reference_scope=parser_help_only，不把存在于解析器列表当作设备已连接或完整映射已校验，也不授编辑许可；M1／全部操作等价性仍未完成。详见 HELP-20261024-66。

### 逐项操作状态与拒绝保护：DESIGN-58（待实现／待验证）

GUI／MCP 采用同一逐项状态限制。现有 GUI 要求空闲的配置及映射编辑，须 IDLE 且相关停止清理完成；运行中 Agent 说明停止及修改内容，由用户选择，获同意才停止、等待清理、重校验和提交，修改后保持停止。拒绝／取消、停止失败或超时不修改，迟到完成也不擅自继续原请求。实际提交复用业务校验，不能只检查 GUI 控件；预览后运行／STOPPING／配置或目标变化、批量拒绝均须保证无部分映射、派生值、dirty 或磁盘修改。

即时显示操作不一律套用编辑门禁，其他显示设置亦不因名称自动获准。下表仅是本轮设计要求与静态入口，不新增已盘点业务链路计数或已开放能力；允许状态仍须按完整 GUI 入口逐项验收。

| 操作／阶段 | GUI 入口 | 允许状态与共用校验要求 | 实际副作用、持久化及用例 |
|---|---|---|---|
| UI-WINDOW-MOVE／M2-B | 主窗口标题栏／系统移动；当前 pos() 由 saveKeyMapSetting 保存 | 映射运行期间与 GUI 一样允许即时移动；其余窗口／加载条件按 GUI 核实。共用明确位置操作，不开放任意 Qt 属性或外部 HWND | 仅改变实际窗口位置并沿用 LAST_WINDOWPOSITION 保存语义；不编辑映射或设置“启动位置”选项。显示／焦点导致既有匹配变化另记。EQ-OPSTATE-01／04 |
| UI-WINDOW-RESIZE／M2-B | 原生窗口边框拖动 → resizeEvent → applyResizeLayout；用户手势更新 m_normalWindowBaseSize | 映射运行期间与 GUI 一样允许即时调整尺寸；保持原有最小尺寸、最大化及 DPI 条件。共用操作包含用户尺寸基线和布局处理，不能仅 resize 后成功回显 | 实际窗口／表列布局及正常用户尺寸基线；保留 LAST_WINDOW_SIZE／SAVE_WINDOW_SIZE 既有保存恢复，不自动保存映射。程序化改变几何不能遗漏 GUI 手势会更新的基线。EQ-OPSTATE-01／04 |
| SET-STARTUP-POSITION／M2-B | 通用高级设定的启动位置模式／指定坐标及 updateStartupSpecifyPositionState | 这是设定编辑，按该 GUI 实际允许状态及联动校验；不能继承即时移动的运行中许可 | 启动位置 getter、模式／坐标、dirty 及保存／下一次启动语义；不通过移动窗口代替设置提交。EQ-SET-GENERAL、EQ-OPSTATE-02／03 |
| SET-REMEMBER-SIZE／M2-B | 通用高级设定“保存窗口尺寸”选项／getSaveWindowSize | 这是设定编辑，按该 GUI 实际允许状态校验；不能继承即时缩放的运行中许可 | SAVE_WINDOW_SIZE、dirty 和冷启动恢复条件；不改变当前尺寸来伪造选项效果。EQ-SET-GENERAL、EQ-OPSTATE-02／03 |

MAP-ADD／MAP-EDIT／MAP-DELETE 及 SET-SCALE 等现有 GUI 要求空闲的操作继续保留原限制，在其共用业务入口补齐状态与清理守卫；具体参数／链路仍见下文。既有所有设置和操作接入时都按本表字段逐项记录，不以模块级一句“运行中禁止”代替。

| 待验组 | 独立 GUI／正式 MCP 验收判据 |
|---|---|
| EQ-OPSTATE-01 运行中显示 | 从等价隔离状态启动，GUI 窗口移动／缩放与正式 MCP 各自独立驱动均获允许，不请求停止；核对实际位置／尺寸、布局和尺寸基线，不直接调用共用函数充当 GUI 基线 |
| EQ-OPSTATE-02 状态拒绝 | 监视等待、实际映射和 STOPPING 中均拒绝要求空闲的设置及映射增删改；覆盖预览后启动／停止／切配置／改目标及批量拒绝，无部分数据、派生状态、dirty 或写盘 |
| EQ-OPSTATE-03 停止后修改 | 用户同意、拒绝、取消；正常停止及必要任务／池／GUI 尾部清理，失败、超时和迟到完成；仅同意且当前请求成功清理、重新校验后提交，修改后停止，不自动开始 |
| EQ-OPSTATE-04 显示效果隔离 | 显示调整不改映射内容／引擎配置；比较既有尺寸／位置保存与冷启动恢复。固定焦点／匹配条件对照执行效果，显示／激活引起的既有匹配变化另记；真实拖动／DPI 另按 Default 范围实测 |

状态／拒绝保护纳入 M1，纯显示能力逐项接入及完整对照纳入 M2-B；物理操作可与主计划 TARGET-07 的 Default 时段合并。上述均待实现／待验证，不将当前 source.read 或 STOP-56 组件证据当作此组通过。

### 当前只读能力：M1.2a-1

`state.read`／`qkm_get_state` 为固定无参数、Read 权限的只读摘要；从 GUI 线程读取界面选择对应的实际配置组、当前 Tab、dirty、GUI 状态、Worker 原子采样和实际停止键。配置选择不是运行／编辑数据来源证明；业务修订号为 null，Tab index 不能充当稳定编辑目标。初始静止或当前读取许可无效时 stop_completion=unobserved；STOP-57 的当前完整清理只读上下文才为 observed，不是异步 operation 结果。加载／初始化未就绪返回 context_busy，未知参数与超大结果拒绝。

STATE-34：14 项桥接单测，两套正式 SDK 各 12、相关会话各 3 组，Qt6／Qt5 产品各 8 分项通过。独立 GUI 基线先选择可见映射页，读到实际停止快捷键 L-Ctrl+F6 和“映射开始”按钮；正式 MCP 核对空白配置／Tab1／dirty、停止态及重复读取不变，原生负例独立核对 C++ 参数拒绝。未以 MCP 或业务函数替代 GUI 基线。

STATE-34 当时的证明范围为隔离空白配置／停止态摘要；后续加载场景及源快照见 LOAD-37／41、SOURCE-42。全局配置、运行态、加载／初始化忙及超大摘要错误仍待对应场景。完整业务修订、稳定编辑授权、写操作及停止完成等待未实现；下文历史通信记录只适用于其原版本／范围。

### 配置／Tab 目录：M1.2a-2a

`catalog.read`／`qkm_query` 接 configurations／tabs，返回配置选择目录、当前 Tab 元数据和未附着共通表记录。offset／limit 严格检查类型和范围，分页带 catalog_revision；版本按实例、配对、权限代次和实际目录内容计算，每页重核，无缓存／定时器。catalog_ref 仅为目录引用，不是编辑目标或完整业务修订。selected_configuration／is_selected 不代表实际加载身份，attached_to_ui 不表示屏幕可见性。

CAT-36：15 单测、两套 SDK 各 12、两套真正产品各 10 分项通过。独立 GUI 读到 Tab1、空白选择文本和停止键；正式 MCP 返回对应目录，分两页与整页一致，重复读取／同配对重连引用稳定，错误参数／修订拒绝，新 GUI 配对不能沿用旧修订。未通过 MCP 驱动 GUI 基线。

CAT-36 当时是空白配置／停止态选定范围；后续已保存／温切换及实际源表见 LOAD-37／41、SOURCE-42。加载忙／产品大结果、运行／重启及性能仍待对应用例。目录引用本身不是源行或编辑引用，不用本目录读流替代完整等价性。

### 映射加载记录：M1.2a-2b-1

state.read 新增 mapping_load_context，目录中也返回并绑定它：最近加载／空白重置的 generation、既有加载器 outcome、实际解析的 group；scope 仅为 last_mapping_load。加载未完成拒绝查询，非法映射加载失败时 group=null。它不追踪后续行编辑、不证明持久化或共通备份归属，不是业务修订或编辑目标；稳定源行身份及安全读取／写入仍在 2b 后续。

LOAD-37：PC-W10 的 Qt6 空白／已保存／非法／缺失四组，Qt5 已保存／非法两组，各 11 分项通过并正常退出；真实 GUI ValuePattern 与目录／来源记录对照。首次非空配置揭示旧测试助手读取 ComboBox Name 的缺陷，失败保留并清理已观察到的自有配对；修正未改变产品加载逻辑。

LOAD-41：PC-W11 的 Qt6／Qt5 各独立 GUI 七步和正式 MCP 18 分项通过。配置控件按实际支持的 ExpandCollapse／Invoke 展开真实弹窗，窗口 Up／Down／Enter 激活；GUI 实际表格为 A→B、C→D、空表、恢复 A→B、非法加载空表及再次恢复。同内容重选仍增加加载代次；正式 MCP generation=1…7，失败 outcome=invalid_mapping_data／group=null，空白重置 accepted／空组，各步旧目录拒绝、新分页恢复。只读请求前后 GUI、摘要／dirty、整个 INI 字节不变；同项重载摘要除 generation 外不变。空白重置的 dirty=true 来自原 GUI 行为，未被读取清除。首次／冷启动四个产品正常退出，缓存关闭；助手校准失败均保留。没有改产品加载逻辑，也没有提供源行／稳定编辑目标。保存失败／缓存恢复、多个普通 Tab／筛选／共通来源及真正停止完成仍须对应场景。

### 源快照：M1.2a-2b-2／STOP-57

source.read／qkm_query 的 mapping_sources、mappings 返回当前实际普通／共通源表及源行，不读取合成运行表或为查询触发同步回写。mappings 要求 source_revision 与 source_tab_ref；源行含 source_ref、源行号、现有映射码完整字段、派生缓存及 LockState 采样。未附着共通备份独立列出，origin=source_memory 不证明其磁盘组来源。引用绑定实例、配对／权限、源内容、结构代次及加载上下文，仅为 source_snapshot_only；不是完整业务修订或编辑授权。快照 4 MiB、单页原帧预算及严格参数／分页边界见 SOURCE-42。

SOURCE-42：PC-W11 两套产品各 9 分项通过，源码及最终 EXE／日志见验证记录。独立 GUI 的两个普通 Tab 与共通源先于 MCP 建立基线；正式 MCP 验证非当前源、共通重复显示去重、读取／dirty／INI 不变性、实际 GUI 上移和相同内容往返失效、重载／切配置、未附着共通来源范围、失败／恢复和新配对拒绝旧引用。每套源维护 3 组、协议 15、SDK 各 12 通过；助手校准及一次 UAC 取消保留，13 个自有产品均退出 0，最终配对／接口／缓存已清理。

STOP-44 在真实 SendInputTask 构造前补上同一首次活动门禁，直接构造／删除任务也不重新开放。Qt6／Qt5 Release 与 Qt6 ASan 各 9 项无输出生命周期维护用例通过，含队列／runnable 连续存续、拒收／销毁／取消及派生序列；内部计数未作为 MCP 清理完成或运行后查询的放行条件。STOP-43／44 均不是完整停止、设备释放或 GUI／MCP 启停等价验收。

STOP-46 在三处 GUI 按钮调用的共同排队入口创建存续标记并关闭首次门禁，业务处理仍排队到 Worker；退出专用拒收在 LockState 修改前检查。Qt6／Qt5 Release、Qt6 ASan 各两个组件用例通过：排队按钮至真实发送的标记连续，拒收后锁定状态不变。它未增加 MCP 能力或完整停止完成条件，日常停止代次、所有生产者与 GUI 尾部仍待协调。

STOP-49 在 PC-W10 为三个 GUI 生命周期入口及六个 Worker／Hook 排队信号加入同操作共享标记，保留原槽与业务顺序。Qt6／Qt5 Release、Qt6 ASan 各 4 项通过：三对共享请求六次派发计数为 3/3/2/2/1/1，丢弃一个接收器仍为 3、全部丢弃后为 0，既有排队停止动作／按钮预期保留；12 个自有进程均退出 0、ASan 4 项无报告。这不是跨所有者完整栅栏或 GUI／MCP 等价性通过。source.read 门禁与 stop_completion=unobserved 不变，详见主计划及 STOP-49 记录。

STOP-50 为四个原生输入回调、发送包装／同步发送及 Interception 成功接收后至转发结束的区间增加独立栈作用域计数。三配置各 5 项通过：IDLE 回调内真实记账信号计数 1/1、返回后 0，DOWN／UP 记账保持；包装提交两次均被观察，NoOp 同步发送等待为 1、拒收／丢弃负例为 0，退出均归零。原生接收／转发与鼠标／低层 Hook 实际效果本轮只核对源码和编译，未冒充物理输入验证。嵌套作用域不等于线程数，仍不是停止完成条件；新增计数尚未开放任何 MCP 放行能力。

STOP-51 内部三所有者队列确认具有独立代次及取消；Interception 将确认延至本轮迭代退出，会话通知也持有原共享标记并关闭首次门禁。三配置各 5 项通过：延后 Hook 时其余端确认不放行、GUI 推进，旧代被忽略，取消后的晚到确认不完成旧作业，新一代完成一次；合成锁定／解锁计数 2→0，原停止发送／生命周期回归保持。确认本身没有准入、子队列、池或 GUI 清理语义，目前尚未接入产品停止完成或新增 MCP 能力。

STOP-52 修正共用连发清理函数：原始键已不在当前源表时，剩余循环定时器也必须销毁，不能只清空索引。Qt6 基线经真实 Worker 停止入口复现循环定时器未销毁、按压定时器已销毁；两套 Release／Qt6 ASan 各三个原表／异表／清空源表用例均通过，QPointer 确认实际销毁。无系统输出用例不是 GUI 切表／删行或物理连发基线；本修复不新增停止完成声明。

STOP-53 Worker 异步发送观察仅在活动请求时检查两类作用域和真实池排空，带代次及超时／替代／退出通知，不自行停止或删除任务。三配置各 4 项通过：正常等待和超时后新代均排空，替代后忽略旧代，三类非成功通知时任务／控制器仍完整；等待期间 GUI 心跳推进，退出后所有计数归零。它不封闭输入、不含前后所有者栅栏／GUI 尾部，Worker 阻塞时也不代替 GUI 总超时，未开放新的 MCP 完成／编辑能力。

STOP-54 将准入关闭、前后所有者确认、Worker 异步排空、末尾所有者与 GUI 尾部接入既有 GUI 停止入口；GUI 独立总超时仍锁定编辑和重启，继续观察而不重发停止动作，延后配置回切。GUI 状态增加 stopping；摘要并不声明 MCP 停止完成。2026-10-24 PC-W10 三配置各 8 项通过：NoOp 按钮启停／等待超时／退出，Worker 仍 STARTED 时关闭准入且无 Tab 的原始 A DOWN／UP 记账，以及四项相关回归。GUI 完成两例各发一次停止及 DOWN／UP，超时后同一作业完成并保留 timedOut；退出由原 main 清理，无 GUI 完成通知。初轮夹具漏填抬起序列引发启动崩溃，ASan 定位后仅修正测试派生字段与表格刷新，失败不计通过。未做正式 MCP 对照、真实输入／设备、配置回切或循环启停验证；source.read 与编辑授权仍不放行。详情见 STOP-20261024-54。

STOP-55 维护目标专用夹具验证连续两轮 GUI NoOp 启停、首轮超时后重启、正常／超时后的延后回切，并重跑正常停止，三配置共 15 项通过。根作业 1→4；晚到旧 GUI 尾部／drained 通知不提前放行，新轮不保留旧超时；在途发送期间实际源表和加载代次保持，排空后原加载器加载 McpReturn／ReturnTab。测试 friend 只在维护目标启用，不新增产品入口；构造回切条件不是实际前台匹配／正式 MCP 等价性证明。仍未解除任何读取／编辑门禁，见 STOP-20261024-55。

STOP-56 运行历史由不回绕活动修订推导；任务每次构造及非空同步／包装入口在处理前标记，避免“曾运行”后新活动无区别。两个普通组件 kit 各三项边界用例通过（含 init/cleanup 各 5 passed），三配置引擎各四项共 12 项通过，构造／同步／包装／执行修订实录均为 0/1/2/3/4/5/6/10，回切和超时后重启保持。忙期测试改用 GUI 所有者的加载信息／受保护表格，完成后才采实际源。新增互斥未做性能测量；修订不是完整业务版本或停止／写入许可。同步清理／序列控制同样会关闭原初始只读门禁，接口仍为 pristine_idle，见 STOP-20261024-56。

当前读取要求最近加载成功、GUI idle／Worker stopped，且 read_basis 为 pristine_idle 或与当前活动修订相符的 completed_cleanup。后者必须经过既有完整停止清理及必要配置回切；新启停／排队／发送活动失效，不凭 STOPPED 或零计数重开。复制与活动准入互斥，cleanup_generation 初始为 null、完成后为根代次字符串。GUI 入口为原开始／停止按钮及实际快捷键；MCP 仅查询，不代替 GUI 启停。实际副作用只生成快照，正式测试核对源字段／dirty／INI 不变、运行中拒绝且不停止、再次清理旧引用失效。STOPPING／超时的既有 NoOp 证据按原版本保留，本轮未新跑其正式 MCP 路径；Qt5／Win11 运行后源读取、英日源 GUI、大结果／性能、编辑／保存仍待对应验收，不将只读引用升级为稳定编辑授权。

### 普通用户目标选择：TARGET-DESIGN-47（待实现／待验证）

完整设计与 TARGET-01～07 待验场景只在[唯一主计划](qkeymapper_mcp_automation_plan_codex.md)维护。原设计提交 1c3dc6d746409d479d3952983bf4c94659094874，DESIGN-58 本轮补整窗黄色闪烁框，均未实现／未验证。已有显式 discovery-file／可信 EXE、进程／管道身份核验、认证连接及私有存储独占租约；同一 EXE 路径、同一 Windows 会话目前只允许一个实例启用 MCP。管理窗口的可复制连接路径不等于已交付候选选择／导入引导，开发测试助手也不能作为普通用户功能。

最小待补能力为有界候选及目标摘要、已认证且具有独立显示权限的受限窗口显示、只观测显示状态、客户端／MCP 会话绑定确认与主动切换。候选来自已配置／显式导入连接及当前用户默认私有发现根；不自动信任发现文件自报路径，不以标题／前台／顺序选目标。业务摘要需 Read；显示不自动复用 Read，已有配对保留、缺权时 GUI 明确增授。普通连续操作不重复激活，确认输入时只重核实例身份，不要求仍在前台。

| 新增操作 | 入口／允许状态及共用守卫 | 副作用／存续及验收 |
|---|---|---|
| TARGET-HIGHLIGHT／M1，待实现 | 已认证且独立显示授权的待绑定展示，复用 showQKeyMapperWindowToTop；遵循原 GUI 显示限制，不为画框停止映射；只作用本实例主窗口或有效所属模态窗 | 整窗含标题栏，透明提示窗不全局置顶、不接收输入／焦点或进入任务栏；默认 #FFD400、约 2 逻辑像素、每 500ms 亮灭，跟随几何／DPI，隐藏无残留；不修改映射、dirty 或 INI。确切存续／异常清理只在主计划维护，TARGET-03／07 待验 |

展示记录按实例／会话／代次绑定，每实例同时仅一条待确认记录，其他会话不得覆盖或清除；五分钟到期及确认／否认／取消、断线、撤权、关闭／退出清理，旧回调不清新提示。提示框状态／失败原因与实际前台观测分别报告，提示窗 HWND 不充当前台证明。完整规则以唯一主计划为准；出现黄色框不授予绑定或业务权限，不启动窗口捕获。

新目标确认成功后才替换原会话绑定，失败／取消保留原绑定并报告其有效性；旧引用／预览和已接受任务始终归原实例，不迁移或自动重发。上述目标选择能力、Default前台／托盘核验及多实例业务隔离仍未实现或未验证，不列入当前12工具的支持范围；免Python交付归M3-B。STOP-46及此前记录仅按原范围保留。

### 保存准备C1-B2-TABLE（完成本局部切片）

GUI Save复用私有97项有序Qt载荷，原表达式／顺序及前后业务处理保持；完整保存仍未接MCP。双Qt各52项旧基线／60项提取后保存冷读，四对252键／类型和值／INI字节一致，源／派生键／锁／dirty／加载及磁盘只读检查通过。共通附着／脱离、多Tab及空Tab、序列备注和样式覆盖见[map67c1_validation.md](map67c1_validation.md)。无图片时像素选择清空属于原加载规范化；剩余保存范围、其他草稿、修订及失败语义仍待C1-B2-PROFILE／C1-C，不作为新产品能力完成。

### 开发测试公共接入：CACHE72

缓存技能新增登记原生Start／拥有关系Close／固定Gui，正式产品／源查询／输入run和SDK高权限夹具已接公共入口；新目标／case由独立授权绑定精确定义，不新增MCP产品能力。PC-W10普通87、公共助手及维护18、技能结构、新维护25／SDK12、最终整套run26及实际缩短清理／硬到期通过；覆盖及失败证据见[cache72_validation.md](cache72_validation.md)。其他系统、长时、Default输入／性能及产品完整保存闭环不因此算通过。

### 跨机 Python 开发基线：M0.3c

ENV-38 保留为 PC-W10 的版本／漂移检查历史。ENV-39 已在 PC-W11 核实 CPython 3.11.9 x64 安装文件身份、31 项 wheel SHA256 锁和独立 venv 实际哈希重装，环境检查／pip check、6 环境用例及选定协议／SDK／两套产品接续通过；CACHE-40 的同 EXE／同 INI 无缓存与缓存 11 分项、状态及目录业务字段相等。ENV-48 完成 PC-W10 新锁下载／哈希重装、环境检查／pip check、6 环境／15 协议用例、Qt6 原生 IPC 88 项及普通正式 SDK 12 项；未复验最新产品源路径。它们是开发验证基础，不是新的 MCP 业务能力，不补全 UAC 交互、High 截图、真实输入或稳定源行身份。后续换机仍须核对当前锁及本机实际安装；打包工具锁／交付仍待 M3-B。

## 内部通信基础（M1.1a／b／c）

以下为内部组件及通信历史检查，不代表完整用户业务能力；表内未接／未验项以对应历史版本为准。默认关闭的管理窗口和正式 SDK 已接，旧 transport_only 范围保留；当前新增三项 Read 能力及其证明边界见上文。历史结果见 `IPC-20261022-07` 至 `31`，后续状态／目录／加载／源快照分别记录。

| ID | 实现及源码 | 已证明范围 | 未证明范围 |
|---|---|---|---|
| INF-FRAME | [qkm_ipc_frame.cpp](../../QKeyMapper/automation/qkm_ipc_frame.cpp)：大端长度、分段读取、有界 UTF-8 JSON 对象、深度与失败终止 | Qt6.8.3／Qt5.15.2 指定边界用例通过；实际往返保留中文／日文／emoji、转义 NUL，错误不回显旧消息，超限头不分配大载荷 | 首包 schema／版本、读缓存和慢连接见 INF-LISTENER；认证后读写限额、鉴权、速率和业务原子提交由后续层实现 |
| INF-IDENTITY | [qkm_local_identity.cpp](../../QKeyMapper/automation/qkm_local_identity.cpp)：进程创建时间、主 Token SID／登录／会话及实际管道对端身份 | 两套 Qt 下同机独立进程双方查询通过；IPC-22 验证 High／Medium 系统 UAC 互指登录，保留进程／请求原 AuthenticationId 严格检查；不采信自报 PID | 请求上下文组件见 INF-REQUEST-CONTEXT；完整集成／异常恢复、真正异用户／会话、管理员产品与普通桥接、配对／授权及撤销尚未验收；主 Token 快照不等于完整授权 |
| INF-LISTENER | [qkm_local_listener.cpp](../../QKeyMapper/automation/qkm_local_listener.cpp)：默认关闭的原生单实例管道、Qt 接管与清理、严格首包、绝对期限及请求 Token 门禁 | PC-W10 两套 Qt 普通配置各 59 通过；Qt6 故障配置 64 通过；匿名请求不得进入握手入口，在途检查全 listener 限一项，挂起检查超时／恢复失败关闭端点，旧结果不放行重启实例；见 IPC-20261022-11／13 | listener 单独不授权；产品管理窗口、控制器和 SDK 已接入，只读摘要见 STATE-34；本行组件结果不等于业务能力或 GUI 等价性 |
| INF-REQUEST-CONTEXT | [qkm_local_identity.cpp](../../QKeyMapper/automation/qkm_local_identity.cpp)：独立原生线程检查已读管道请求 Token，退出后回所属 Qt 线程报告 | 普通／匿名 SQOS、进程／会话绑定、析构回收通过；已接 listener；专用配置故障注入证明恢复失败、长期挂起及关闭交错不会认证或累积替代线程；见 IPC-20261022-12／13 | 故障注入不等于真实 OS API 故障；真正异用户／会话、管理员产品／普通桥接及完整鉴权尚未验收；不能当作业务权限 |
| INF-AUTH-PROOF | [qkm_auth_proof.cpp](../../QKeyMapper/automation/qkm_auth_proof.cpp)：系统随机、固定格式双向 HMAC-SHA256 和一次性挑战 | 三个方向／用途的固定输入及 MAC 与独立 Python 结果一致；绑定字段逐项变化、版本／类型／长度、32 个错误字节位置、反射／重放／旧 nonce／取消及随机源失败通过；见 IPC-20261022-14 | 已在 INF-SESSION／SDK 接握手及 GUI 授权；本行仅证明组件语义，不单独作为认证或业务权限证据；未测侧信道时序 |
| INF-PAIR-STORE | [qkm_pair_store.cpp](../../QKeyMapper/automation/qkm_pair_store.cpp)：类型化权限／凭据、机器与应用绑定、当前用户 DPAPI、私有 ACL、原子保存 | PC-W10 非提权运行，普通 Qt6／Qt5 各 71、故障 Qt6 77 通过；独立进程读取、错误上下文／损坏／非法权限拒绝、只读失败保留原文件、私有目录与硬链接拒绝，见 IPC-20261022-15 | 已接内部控制器与管理窗口；IPC-22 已证明 High 维护服务写盘、普通桥接配对／重连；真实异用户／跨机器及完整产品未测，不能算业务授权或完整安全边界；默认执行沙箱不具备可用 DPAPI 环境，运行测试使用获准非提权沙箱外上下文 |
| INF-SESSION | [qkm_session_server.cpp](../../QKeyMapper/automation/qkm_session_server.cpp)：连接代次、GUI 权威批准入口、证明及保存后 ready、最小权限、代次更新／撤销、分阶段限额 | IPC-22／31 原范围；STATE-34 相关会话两套各三组；CAT-36 未变会话代码复用，正式产品两套各十分项 | 只读状态／目录选定范围见上；不能把通信结果当作写操作业务等价性，异用户／Win11、写权限和实际效果待对应场景 |
| INF-GUI | [ControlDialog](../../QKeyMapper/automation/qkm_control_dialog.cpp)、[通用高级设置](../../QKeyMapper/qgeneraladvanceddialog.cpp)、[托盘入口](../../QKeyMapper/qkeymapper.cpp)：本次运行启用、只读默认、实际程序／PID、权限批准、拒绝／撤销、窗口关闭与 Escape | IPC-23 修正后两套 GUI 各 18；IPC-31 两套真正产品六分项完整通过，含默认关闭、只读批准／RPC、重连、撤销及冷启动拒绝旧 key | 实际托盘／High 静态截图未验，普通 PrintWindow 失败；三语言同源静态图复用 IPC-21。固定助手 Qt5 选择模式兼容仅属测试工具，无业务等价性结论 |
| INF-DISCOVERY | [实例发现](../../QKeyMapper/automation/qkm_instance_discovery.cpp)、[共享私有存储](../../QKeyMapper/automation/qkm_private_storage.cpp)、[会话启停](../../QKeyMapper/automation/qkm_session_server.cpp)：私有原子 JSON、活跃文件租约、真实进程／登录／预期 EXE、按句柄清理 | IPC-19 组件与 IPC-27 两套 SDK 原结果；IPC-31 两套正式管理员产品默认绑定／发现、关闭清理及冷启动重新启用通过 | 只是连接提示，实际管道身份／新证明仍必需；实际异用户及 Win11 未测，不算业务能力 |
| INF-SDK | [桥接入口](../../mcp_bridge/src/qkeymapper_mcp/__main__.py)、[连接与证明](../../mcp_bridge/src/qkeymapper_mcp/bridge.py)、[Windows I/O](../../mcp_bridge/src/qkeymapper_mcp/windows.py)、[客户端凭据](../../mcp_bridge/src/qkeymapper_mcp/credentials.py) | 官方 mcp 2.3.0；CAT-36 七工具严格参数、15 单测，两套正式 SDK 各 12、产品各 10；High 维护／60 秒范围仍仅复用 IPC-22 | 映射行／写入／启停等未接。历史一次维护退出原因未知，保留 IPC-31 失败索引；打包、异用户、Win11、性能及写操作等价性未验 |
| INF-OFFLINE | [共享私有存储](../../QKeyMapper/automation/qkm_private_storage.cpp)、[会话](../../QKeyMapper/automation/qkm_session_server.cpp)、[窗口](../../QKeyMapper/automation/qkm_control_dialog.cpp)：关闭接口查看／撤销，原生目录及 control.lock 租约 | IPC-21 占用拒绝／只读／损坏／冷启动及 ASan 原范围；IPC-22 High 维护离线撤销，IPC-30 实际产品离线撤销持久化观察，IPC-31 两套产品默认关闭／零配对／冷启动旧 key 拒绝通过 | 不承诺无租约旧原型兼容；实际异用户／Win11 未测，无业务等价性结论 |

## 初始模块索引

下表是模块范围导航；除 CAP-001 的状态摘要子集外，各模块 MCP 业务实现／等价性验收仍为 **未实现／未执行**。已追踪的部分见后续关键链路表，其余逐项契约仍待核对。拟定用例 ID 是后续测试目标，不表示已存在测试脚本。

| ID | 能力范围 | GUI／源码导航 | 共用链路与有效状态重点 | 阶段／拟定用例 |
|---|---|---|---|---|
| CAP-001 | 能力、实例、运行状态、目录／源快照及帮助 | [主窗口头文件](../../QKeyMapper/qkeymapper.h)、[主程序入口](../../QKeyMapper/main.cpp) | 摘要／目录／加载见 STATE-34／CAT-36／LOAD-37／41；初始静止源表／行见 SOURCE-42。其他配置／运行态／产品大结果待验，帮助及稳定编辑授权未接 | M1／API-STATUS-001 |
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
| 允许状态与拒绝保护 | 各 GUI 入口的实际允许状态、相关停止清理条件及共用业务校验；覆盖预览后状态变化和批量拒绝，不能只依赖控件禁用，拒绝不产生部分修改 |
| 有效状态及副作用 | 主数据、派生字段、缓存、Worker／系统接收值、异步完成条件及通知；必要处理不漏执行、不重复执行 |
| 生效及持久化 | 当前内存／即时持久化／保存／下一次开始／重启及取消、失败、恢复规则 |
| 风险与授权 | 停止要求、依赖、实际停止快捷键、外部或系统副作用、恢复边界 |
| 独立用例 | GUI 基线、正式 MCP 路径、相同期望；实际数据、状态、保存恢复和最终效果断言 |
| 分系统结果 | Win10 与 Win11、Qt／构建／源码、通过／失败／未执行、验证记录 ID及必要证据 |

本节静态盘点不声明下表编辑／设置操作已经接入；当前已验只读范围见上文。下列结论用于决定提取位置及验收要求，不替代运行证据；未追踪的参数、持久化分支及动态入口仍须补齐。

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

### M0.2b 第一批补充：Tab、插入及宏（源码 `2b9215ef`）

以下 14 条只完成静态链路核对，连同上表共 30 条；不是 30 项已开放能力。具体约束、GUI／MCP 独立运行和所有后续动态动作仍待逐项补齐。`prepare`、`sync` 或 bool 返回值不自动代表只读／写盘完成。

| ID／阶段 | GUI 入口与已确认链路 | 有效状态、持久化与风险 | 最小共用操作及独立验收要求 |
|---|---|---|---|
| MAP-INSERT-PLAN／M1、M2-A | 粘贴及单表导入 → `prepareMappingDataForInsertion`；目标 Tab、候选行及 `allowCommonConflictPrompt` | **并非纯预览**：目标为共通表时会直接禁用其他普通 Tab 冲突行并发通知；false 只跳过询问，仍执行修改。还逐候选维护已启用互斥组、自动禁用重复／共通冲突新行，返回修复摘要 | 先分离纯分析与实际提交，GUI 询问后与 MCP 提交调用同一操作；不得把现函数或 false 当 dry-run。`EQ-INSERT-PREVIEW` 断言预览前后所有源表、Disabled、dirty、通知／派生状态及磁盘不变；取消／过期后同样不改 |
| TAB-ADD／M2-A | 菜单 Ctrl+N／表格入口 → `addTabToKeyMappingTabWidget(customName, activate)` | 有默认名及重名后缀搜索上限；在共通表之前插入新的源 QList 和真实表格，初始化 Tab 默认字段，维护当前／上一表索引、布局、刷新及 dirty。只有 activate=true 的入口显式守卫 IDLE，加载也使用该函数 | 共用明确新增操作，区分用户新增与加载；预览返回规范名和源位置，不临时切换 UI。`EQ-TAB-ADD` 比较源列表独立性、共通位置、全部默认值、可选激活、索引失效及保存恢复 |
| TAB-COPY／M2-A | Ctrl+D／菜单 → `copyCurrentTabToKeyMappingTabWidget` | IDLE 限制；拒绝共通源；自动命名，新 QList 值拷贝映射及 Tab 属性，复制快捷键／共通包含与折叠／图片及浮窗设定，清空内部名，保持普通类型；刷新显示／索引及 dirty。不是只复制表格文字 | 提取明确源 Tab 的复制操作，保持 GUI 命名和所有复制字段；不得以改变当前表确定源。`EQ-TAB-COPY` 比较全部行／备注／同步组、属性、快捷键重复语义、新旧源隔离及冷启动；运行临时字段的预期另行验证 |
| TAB-REMOVE／M2-A | 菜单／Tab 设置删除 → `deleteMappingTableByTabIndex` 确认 → `removeTabFromKeyMappingTabWidget` | 外层先按索引取名称，内层才验证索引；保护共通及最后一普通表；关闭相关编辑窗，删除实际 QList／表格，调整当前索引、切表／重连事件及 dirty。菜单状态不能替代业务守卫 | 先解析并核实源引用，再由共用操作检查状态／保护项，提交不弹模态框；GUI 另收集确认。`EQ-TAB-REMOVE` 覆盖非当前表、取消、过期目标、共通／最后一表、打开编辑窗、热键缓存及保存恢复 |
| TAB-ORDER／M2-A | `QTabBar::tabMoved` → `KeyMappingTabWidget::onTabMoved`／顺序恢复检查 → `onKeyMappingTabWidgetTabOrderChanged` → `moveTabInKeyMappingTabWidget(from,to)` | 回调时 Qt 页序已经移动；末端函数处理数据侧 IDLE／索引／共通检查、移动 TabInfo、当前／上一表跟踪、切表／重连、热键缓存及 dirty。直接调用末端函数并不包含 Qt 页序移动 | 共用完整的明确源／目标顺序操作及结构修订，避免回调重复提交；`EQ-TAB-ORDER` 比较实际页序与源信息、逻辑当前表保持、源引用失效、热键实际切表、共通固定位置及冷启动顺序 |
| TAB-CLEAR／M2-A | 菜单清空 → `clearCurrentMappingTable` 确认 | 清空当前实际源表、刷新依赖显示、清除 Screen／WindowMousePointsList 并 dirty；存在编辑窗关闭分支。不能仅清空可见行或遗漏被筛选／禁用行 | 共用明确源表清空，GUI 另确认；`EQ-TAB-CLEAR` 覆盖筛选、共通表、空表、坐标缓存、未保存草稿、取消及冷启动；状态守卫随共享入口补齐 |
| TAB-NAME／M2-A | Tab 设置文本编辑／更新 → `on_tabNameUpdateButton_clicked` → `updateKeyMappingTabWidgetTabName` | 校验非空、重复及保留共通显示名；写 TabInfo.TabName、更新真实 Tab 显示并 dirty；不以显示文字作为永久身份 | 复用校验／提交，输入框只采集参数；`EQ-TAB-NAME` 检查重复／保留名拒绝、非当前源、提示和图片组合显示、名称变化后源身份及保存恢复 |
| TAB-HOTKEY／M2-A | Tab 快捷键更新 → 规范空白／提取包装内组合键 → `validateCombinationKey` → `updateKeyMappingTabInfoHotkey` | 保存原规范串、更新显示并调用 `collectMappingTableTabHotkeys`，Worker `detectMappingTableTabHotkeys` 消费该缓存和各 Tab 字段。多个 Tab 的同一快捷键对应有序索引列表，不能自行改成唯一限制 | 共用解析、清除、校验、赋值及重建缓存；`EQ-TAB-HOTKEY` 比较显示串与有效键、相同键多目标、清空／非法值、排序后实际切换及冷启动；不只核对 LineEdit |
| TABLE-EXPORT／M2-A | 菜单／Tab 设置 → `exportMappingTableByTabIndex` 文件选择 → `exportKeyMappingDataToFile` | 从指定 Tab 的**内存源行**序列化映射、备注／分类／逐段备注和浮动字段；不导出整个配置／宏定义。现函数 setValue 后直接 true，未显式 sync/status；调用方据此提示成功，不能当写盘证据 | 共用序列化及写盘完成检查，导出明确草稿来源和宏引用依赖；`EQ-TABLE-EXPORT` 用新进程读取实际文件、测试无法写入／覆盖及 Qt5 Unicode。外部路径采用普通权限文件层，不把管理员保存函数变成任意路径写入口 |
| TABLE-IMPORT／M2-A | 菜单／Tab 设置 → `importMappingTableByTabIndex` → `importKeyMappingDataFromFile` → MAP-INSERT-PLAN | 读取旧格式、默认值及字段边界；检查原始／主映射，跳过无效行，空／无效抬起键回退主映射；构造派生字段后追加，冲突可影响其他 Tab。外层才刷新相关表、dirty 和摘要；不等于磁盘配置已保存 | 共用解析、规范化和计划／提交，报告接收／跳过／回退／冲突及依赖，不用弹窗等待 Agent。`EQ-TABLE-IMPORT` 覆盖全无效／部分有效、非当前表、旧格式、同批互斥、共通连动、失败草稿及后续保存恢复 |
| MACRO-UPSERT／M2-A | 按钮／快捷键 → `addMacroToListInternal(allowInsertBySelection)` | 目标取普通／通用 OrderedMap；去首尾空白及换行、非空／名称不含右括号、完整映射串校验，同名需确认。新项可按 Shift／全局插入模式插入；同名更新保持位置。消费待提交逐段备注或按段对齐旧备注，再刷新／dirty；改变输入名称不自动重写旧引用 | 共用明确 scope／名称／内容／备注／插入位置／覆盖选择；预览不能消费编辑暂存。`EQ-MACRO-UPSERT` 比较两种作用域、顺序、同名取消、备注对齐／暂存、嵌套引用与运行依赖；普通宏保存于配置组，通用宏在根设置 |
| MACRO-DELETE／M2-A | Delete／菜单 → `deleteMacroSelectedItems` | 从当前宏表首个连续选区的名称删除相应 OrderedMap 键，刷新／重选并 dirty；未在该路径重写引用该名称的映射／其他宏 | 共用明确作用域和名称集合；`EQ-MACRO-DELETE` 比较筛选下目标、两个命名空间同名隔离、无选择、引用失效及实际展开结果／保存恢复。引用风险预览应说明现有行为，不静默删除关联映射 |
| MACRO-EXPORT／M2-A | `exportMacroListToFile` 从选区或全部构造有序列表，再选择文件 | 来源为内存普通／通用宏；现导出字段为名称／内容／分类／备注，**未含 MacroComments**；调用 sync 后不检查 status 即提示成功。完整配置保存包含逐段备注，不能把两种格式视为相同覆盖 | 两种入口共用规范序列化／完成检查，逐段备注差异在接入时统一修正并保持旧文件兼容。`EQ-MACRO-EXPORT` 比较选区／筛选／顺序、逐段备注往返、无法写入、Qt5 Unicode 和真正新进程读回 |
| MACRO-IMPORT／M2-A | `importMacroListFromFile` 读取 MacroList_Export → QVariantList／Map → OrderedMap → 同名覆盖选择 | 跳过非 Map／空名称；按当前 scope 导入。与手工添加的名称／DSL 校验不同；同名覆盖使用 insert 移到末尾，保留同名时跳过；只解析四个导出字段，刷新／dirty 后仍未保存配置。导入中重复名称也受 OrderedMap 规则影响 | 复用同一文件解析与覆盖／顺序计划，区分旧格式容错与正式 typed 参数拒绝未知字段；不能只给 MCP 另写严格逻辑形成隐性差异。`EQ-MACRO-IMPORT` 覆盖部分忽略、重名保留／替换顺序、空／非法内容、作用域、备注格式和保存恢复 |

后续接口接入必须额外核对：这些内部函数有的依赖当前表／当前编辑框，有的只靠 GUI 启用状态限制运行中操作；不能直接按函数名透传。列表操作会改变源引用，源 Tab、配置修订和编辑会话须在提交前再次核实。

### M0.2b 第二批补充：通用高级与映射高级（源码 `d8e435f4`）

新增 21 条静态链路，连同前两表共 51 条。各行尚未实现 MCP／运行等价性；多轴、四组阈值等共享同一规则的字段在行内逐一列明。GUI 构造及加载默认值不是接口允许静默夹紧参数的理由。

通用高级字段在 `saveKeyMapSetting` 写根设置，由 `loadKeyMapSetting` 的初始／load_all 分支及 `loadGeneralSetting` 读取；普通配置组加载不能一概当作全局重载。映射高级字段则写入当前配置组。两类均须通过现有保存结果检查及独立冷启动确认；关闭这两个窗口不会撤销已经编辑的内存值。

| ID／阶段 | GUI 入口、参数及现有处理 | 实际有效状态／生效／持久化 | 共用操作与独立验收要求 |
|---|---|---|---|
| GEN-STARTUP-MODE／M2-B | 通用高级 startupPositionComboBox 的 currentIndexChanged → updateStartupSpecifyPositionState／dirty；枚举 Default=0、LastSaved=1、Specify=2，缺省 LastSaved | getter 为当前索引；指定坐标控件启用联动。根 StartupPosition_Index；只改模式不会立即移动当前主窗，首次 loadKeyMapSetting 的空 settingtext 分支才选择启动坐标并做屏幕相交检查／回退 | 共用明确枚举校验、联动及 dirty，不借改变模式执行移动。EQ-GEN-STARTUP-MODE 比较三模式、当前窗口不意外移动、冷启动落点、屏幕外回退和全局重载差异 |
| GEN-STARTUP-POINT／M2-B | X／Y valueChanged → dirty；各轴整数 -50000…50000，默认 (0,0)，仅 Specify 时界面可编辑 | getSpecifyStartupPosition 读 QPoint；根 StartupPosition_SpecifyPoint。模式切换保留值；加载非法点整体回退默认，启动时还有可见屏幕回退 | 明确模式及坐标更新策略，拒绝正式参数越界；不可按控件禁用就丢失已有坐标。EQ-GEN-STARTUP-POINT 比较负坐标、两轴边界、切换保留、加载兼容及实际启动位置 |
| GEN-SAVE-SIZE／M2-B | saveWindowSizeCheckBox toggled → dirty；bool | 根 SaveWindowSize；保存仍写 LastWindowSize 的 m_normalWindowBaseSize，即逻辑基准尺寸而非当前物理 size。该开关控制加载恢复，取消勾选不删除旧尺寸；resizeEvent 只在实际用户调整且非 live scale／加载／最大化时更新基准 | 共用开关与原尺寸保存／恢复链路，不能把开关 setter 当作 resize。EQ-GEN-SAVE-SIZE 比较勾选／取消、真实调整与缩放、最大化、冷启动和 DPI 换算；无真实 DPI 输入时只记已覆盖部分 |
| GEN-SAVE-SPLITTER／M2-B | saveSplitterPositionCheckBox toggled → dirty；bool | 根 SaveSplitterPosition；仅开关开启、进程表可见且两分区正数时写 LastSplitterPosition。加载解析两正整数并 setSizes；关闭开关不清除旧比例，也不立即重排 | 共用保存条件及恢复；EQ-GEN-SAVE-SPLITTER 覆盖进程表隐藏、无效／旧值、开关切换、有效实际分区尺寸和冷启动，不只比复选框 |
| GEN-EDIT-TRIGGER／M2-B | tableEditModeTriggerComboBox currentIndexChanged → dirty；右双击=0／左双击=1，默认右双击 | 根 EditModeTrigger（实际键以 EDITMODE_TRIGGER 常量为准）。主表 keyMappingTableItemDoubleClicked、序列编辑通过 getTableEditModeTriggerIndex 读取，另有列／Alt／鼠标键规则，不是全表双击直接调用一个编辑动作 | 共用枚举设置；独立界面事件验证主表及序列编辑的两种触发、列规则与 Alt，再比保存恢复；MCP 的“打开编辑”操作另有明确目标，不能伪造鼠标修饰状态 |
| GEN-INSERT-MODE／M2-B | tableInsertModeComboBox currentIndexChanged → dirty；Above=0／Below=1，默认 Above | 根 TABLE_INSERTMODE；主表 insertNewMappingData、宏及序列编辑在各自插入／Shift／选区分支读取 getTableInsertModeIndex。没有选区或未请求相应插入模式的路径仍可追加 | 共用全局枚举；具体新增操作先从 GUI 意图解析为明确位置。EQ-GEN-INSERT-MODE 覆盖各调用者、上下界、无选择、筛选／共通来源及冷启动，不将该开关强加到所有 MCP 新增请求 |
| GEN-SOUND／M2-B | soundEffectCheckBox toggled → dirty；bool，默认 true | 根 PlaySoundEffect；isSoundEffectEnabled 被 playStartSound／playStopSound 在事件时读取，控制对应 WAV 的异步 PlaySound；修改开关本身不播放预览，不代表所有系统声音 | 共用设置及 dirty；EQ-GEN-SOUND 比较下次开始／停止声音入口、关闭开关和缺失声音文件、保存恢复；自动脚本检查调用不能冒充实际音频输出验收 |
| GEN-GLOBAL-SWITCH／M2-B | globalSettingSwitchTimerSpinBox valueChanged → dirty；整数毫秒 0…999999999，默认 2000 | 根 GlobalSettingSwitchTimeout；加载同时 setInterval。scheduleGlobalSettingSwitchCheck 停掉旧计时并重新读值，0 为 queued invoke，非阻塞立即重入；界面编辑信号本身只 dirty，不重排已经等待的 timer。全屏拦截重试另用 500 ms | 共用值和既有调度语义，不凭名称宣称即时重置。EQ-GEN-GLOBAL-SWITCH 覆盖 0、边界、已有等待、CHECKING／非 CHECKING、全屏重试及冷启动；最终状态需检查实际配置匹配 |
| GEN-FULLSCREEN／M2-B | disableGlobalMappingInFullscreenCheckBox toggled → updateGlobalMappingFullscreenState／dirty；bool，默认 false | 根 GlobalMappingDisableInFullscreen；联动白名单编辑框。shouldBlockGlobalMappingForFullscreen 读取开关，结合实际 HWND／进程判断；handleGlobalMappingBlockedByFullscreen 可发停止、转 CHECKING、隐藏浮窗／提示，仍需等待真正清理 | 共用设置与联动，实际副作用沿现有匹配入口发生；EQ-GEN-FULLSCREEN 比较普通／全屏目标、允许进程、已全局映射及停止完成，不能只验开关／计时器 |
| GEN-FULLSCREEN-PROCESSES／M2-B | fullscreenGlobalMappingProcessLineEdit textEdited → dirty；QString，分号分隔；setGlobalMappingFullscreenAllowedProcesses 仅 setText | 根 GlobalMappingFullscreenAllowedProcesses；isGlobalMappingFullscreenProcessAllowed 跳过空段、trim，按进程文件名 contains／CaseInsensitive 比较，不是正则、通配符或严格全名。程序 setText 不发 textEdited，会漏掉用户编辑的 dirty 语义 | 共用明确文本设置、dirty 和同一匹配解析；加载路径保留非用户编辑语义。EQ-GEN-FULLSCREEN-PROCESSES 检查多段／空白／大小写／子串及不匹配、开关关闭后保留、保存恢复与实际目标匹配 |
| ADV-MOUSE-SPEED／M2-B | 映射高级 mouseXSpeed／mouseYSpeedSpinBox valueChanged → dirty；两轴整数 1…99，默认 3 | 配置组 KEY2MOUSE_X_SPEED／Y_SPEED；主窗口 getter 直读对话框，Worker 的摇杆／按键转鼠标周期用两轴速度计算位移。不是 OS 鼠标速度设置；无单独 setter 通知刷新缓存 | 共用双轴校验及设置，停止后提交；EQ-ADV-MOUSE-SPEED 对比两轴独立值、边界、受控轨迹最终位移与保存恢复。Worker 跨线程读取边界随实现确认，不能以直读视为线程安全证明 |
| ADV-MOUSE-POLL／M2-B | mousePollingIntervalSpinBox valueChanged → dirty；整数毫秒 1…99，默认 2 | 配置组 KEY2MOUSE_POLLING_INTERVAL；Worker setWorkerKeyHook／setKeyMappingRestart 在有相关映射时用 getMouseMovePollingInterval 启动 PreciseTimer。当前编辑路径不对活动 timer setInterval | 共用值及生效时机，停止后提交、下次开始使用；EQ-ADV-MOUSE-POLL 检查实际 timer interval、未使用相关映射不误启动、重启及冷启动，不能只比 getter |
| ADV-TOUCHPAD-SPEED／M3-A | gamepadTouchpadXSpeed／YSpeedSpinBox valueChanged → dirty；各轴 0…200，2 位小数、步进 0.10、默认 0.5 | 配置组 GAMEPAD_TOUCHPAD_X_SPEED／Y_SPEED；gamepadTouchpad2MouseMoveProc 读取速度，以归一化触点差 × 屏幕尺寸换算并保留小数余量；0 清该轴余量。需要实际触摸板与对应映射状态 | 共用参数精度／范围及提交，预览注明规范值；EQ-ADV-TOUCHPAD-SPEED 覆盖零、两轴、余量、屏幕维度、有效输出及冷启动；模拟轨迹与真实设备分记 |
| ADV-TOUCHPAD-INVERT／M3-A | gamepadTouchpadInvertX／InvertYCheckBox toggled → dirty；两轴 bool | 配置组 GAMEPAD_TOUCHPAD_INVERT_X／Y；同一触摸板处理在位移计算前按轴取反，保留速度及小数余量逻辑 | 共用两轴设置；EQ-ADV-TOUCHPAD-INVERT 同轨迹比较方向／幅度、单轴／双轴、停止重启及冷启动，不把 UI 选中当作鼠标实际反向 |
| ADV-VIRTUAL-INPUT／M3-A | acceptVirtualGamepadInputCheckBox toggled → dirty；bool，加载缺省 false | 配置组 ACCEPTVIRTUALGAMEPADINPUT_CHECKED，现存键名含 Gamdpad 拼写，接入不能顺手改格式；Worker POV／Axis／Button／Sensor／Touchpad 事件对 blacklisted 设备按该值决定是否继续处理。此项不安装或创建虚拟手柄；接受本软件输出设备可能带来反馈影响 | 共用值、风险说明及停止限制；EQ-ADV-VIRTUAL-INPUT 按五类事件验证实体／虚拟过滤、已有设备与保存恢复，硬件缺失不能记为真实设备通过 |
| ADV-PROCESS-TRAY／M2-B | ProcessIconAsTrayIconCheckBox toggled → dirty；bool。全局配置加载分支强制 false 并禁用，普通配置恢复可编辑 | 配置组 PROCESSICON_AS_TRAYICON_CHECKED；updateSystemTrayDisplay 在实际映射时按有效 Tab 图片优先、随后可用进程图标、再状态图标选择。setter 本身不直接刷新系统托盘 | 共用范围守卫与既有刷新链路；EQ-ADV-PROCESS-TRAY 检查全局／普通配置、图标缺失与 Tab 图片优先、实际托盘和冷启动。托盘最终效果须在 Default，现未执行 |
| ADV-POINT-KEYS／M2-B | showWindowPointKey／showScreenPointKeyComboBox currentIndexChanged → dirty；各为 None 或 MultiKeyboardInputList 中键名；两个 getter 返回 currentText | 配置组 SHOW_WINDOW_POINT_KEY／SHOW_SCREEN_POINT_KEY；两个键盘处理入口对 down／up 分别维护坐标显示标记并发显示／隐藏信号。None 的界面翻译不应作为公共枚举；旧 setter 对非法串回退默认键，不能将此作为正式请求成功 | 共用规范键名／none、dirty 和既有显示处理；EQ-ADV-POINT-KEYS 检查三语言 None、非法串、两个键相同优先级、按住／释放与叠加显示清理及冷启动；真实输入范围按计划执行 |
| ADV-THRESHOLDS／M3-A | LT／RT 的 Release≤LightPress≤Press；LS／RS 的 Release≤LightPush≤Push。12 个整数百分比 0…100；initGamepadThresholdSpinBoxes 为四组三项建立动态 min／max 及 valueChanged／dirty | 配置组各 GAMEPAD_*_THRESHOLD_*；Worker joystickLTRTButtonProc 和四个摇杆轴处理读取三阈值，按当前已按状态判断轻／重／释放，触发器还按事件轴范围换算。setValue 会被当时 min／max 夹紧；约束连接在首次 loadKeyMapSetting 后才安装，后续配置切换的应用顺序必须专门检查 | 共用完整三元组校验／应用次序，预览报告连带值，不逐字段盲目 setValue。EQ-ADV-THRESHOLDS 覆盖四组、相等及 0／100、跨配置不同三元组、迟滞状态、实际 down／up 和冷启动；静态顺序风险尚未作为运行缺陷复现 |
| ADV-COMMON／M2-A | enableCommonMappingTableCheckBox toggled → commonMappingFeatureEnabledChanged／dirty；bool | 配置组 MAPPINGTABLE_COMMON_ENABLED；主窗口启用时 reconcileCommonPriorityConflicts 会禁用各相关普通源表冲突行并发通知，再 updateCommonMappingTabVisibility 执行 attach／detach、必要切表、全表刷新及 Tab 设置联动。加载临时屏蔽对话框信号，尾部另修复／刷新并保护共通备份 | 共用纯冲突预览＋明确提交，保留加载区别；EQ-ADV-COMMON 比较实际 Disabled、源表／共通备份、当前索引／缓存、提示／dirty、取消、关闭再开启和冷启动；仅隐藏页签不算关闭功能 |
| ADV-NOTIFY-ENABLE／M2-B | customNotificationEnableCheckBox toggled → updateCustomNotificationState → customNotificationEnabledChanged → onCustomNotificationEnabledChanged；bool | 配置组 CUSTOM_NOTIFICATION_ENABLED；主窗口 dirty，关闭时关闭专属编辑窗；首次启用可从全局拷贝位置及完整样式并设初始化标记。getCurrentMappingNotificationStyleSettings 根据开关选择实际样式；load 的信号屏蔽路径另载持久化样式 | 共用启用及初始化／关闭链路，不能只设置 bool；EQ-ADV-NOTIFY-ENABLE 比较首次／再次启用、已有自定义样式保留、编辑窗状态、实际通知和保存恢复；不把未启用的首次复制误记为用户显式样式 |
| ADV-NOTIFY-POSITION／M2-B | customNotificationPositionComboBox currentIndexChanged → dirty；None=0，左／中／右的上、下共六位置=1…6；disabled 时保留值 | 配置组 CUSTOM_NOTIFICATION_POSITION；通知使用自定义样式时取此位置，None 不显示；未启用自定义则使用全局位置。加载缺失／非法值按全局通知位置处理，setter 非法值另回退默认，正式参数应先拒绝 | 共用明确位置枚举及作用范围；EQ-ADV-NOTIFY-POSITION 比较全部位置／None、全局回退、专属启用状态、实际通知窗口及冷启动 |

这一批设置进一步确认：若现有有效值本来存于控件，共用业务操作仍须负责校验、dirty／初始化、派生联动、加载区别及生效时机，再将控件作为现有存储／回显使用；“数据存于控件”不能成为只调用 setter 就返回成功的例外。不在本轮重写整个设置模型，Worker 读取一致性按接入项处理。

### M0.2b 第三批补充：配置、窗口匹配和忽略规则（源码 `f73f0db0`）

新增 17 条静态链路，累计 68 条；没有新增 MCP 业务能力。此处特别区分编辑草稿、MAP_PROCESSINFO、自动匹配读取的磁盘配置和实际 HWND／Worker 状态。拟定用例仍未运行；下列静态差异不能当作已复现故障或已完成修正。

| ID／阶段 | GUI 入口与实际处理 | 状态、持久化及已知边界 | 共用操作及独立验收要求 |
|---|---|---|---|
| CFG-LOAD／M2-B | 配置组合框及自定义弹出列表 applyPopupSelection → 设置索引 → on_settingselectComboBox_textActivated → loadSetting_flag → loadKeyMapSetting → 名称／flushPendingCommonPriorityRepairAfterLoad。Qt6.8.3／Qt5.15.2 均使用 textActivated，非旧 currentTextChanged 分支 | 显示描述与实际组名列表／item role 分开；仅 setCurrentIndex 不执行用户加载链路。加载会关闭相关编辑窗、重建 Tab／源表／宏、更新进程缓存及有效设置，可能执行排队界面尾部；load_all 另包含全局项。返回字符串须核对真实目标，失败前可能已改变部分状态，不能假定事务回滚 | 共用明确组名、草稿处理选择、加载作用域、停止／清理守卫及完成报告；GUI 只采集索引。EQ-CFG-LOAD 覆盖描述相同、同组重载、非法数据、未保存草稿、打开编辑窗、源引用失效、保存后的同路径重载及 GUI 尾部 |
| CFG-EMPTY／M2-B | 用户选择空白配置 → 组合框槽的空文本分支 → loadEmptyMapSetting | 清理／重建当前 Tab、宏／同步组备注、MAP_PROCESSINFO 和窗口条件，重置列出的映射参数／快捷键；不是把当前 INI 所有组删除，也不是所有全局设置恢复默认。已有初始参数和后续重置函数覆盖范围需逐项保持 | 共用明确“新建空白草稿”，保护现有草稿意图；EQ-CFG-EMPTY 比较内存源、宏、停止快捷键、共通备份及配置范围，不将其当作软件全量恢复出厂；不借空字符串隐式删除 |
| CFG-REMOVE／M2-B | 配置弹出列表删除 → removeSettingByName → removeSettingByIndex → 确认 | name 包装有 IDLE 守卫；空白／GlobalSetting 受保护。删除真实配置组、顺序项及必要 SettingSelect，sync／status 成功后才改列表并加载后继配置。sync 失败时没有该路径的显式缓存回滚；不能把返回 false 解释为磁盘必然不变 | 共用明确组名、确认决策和保存失败语义，保留固定配置目标；EQ-CFG-REMOVE 覆盖当前／非当前、保护组、取消、失败缓存／冷启动、后继选择及源引用失效，禁止静默丢弃其他草稿 |
| CFG-ORDER／M2-B | 弹出列表拖动 → movePopupSelectionToUserIndex → moveSettingSelectOrderEntry | IDLE、真实普通组守卫；targetUserIndex 当前会 qBound。写根 SettingSelect_OrderList，屏蔽组合框信号移项、同步实际组名列表／role 并按原组恢复当前选择；没有在返回 true 前显式 sync／status。此显示顺序不被 matchAutoStartSaveSettings 用作匹配优先级 | 共用顺序规划／提交及完成检查，正式参数不静默夹紧；EQ-CFG-ORDER 比较显示与真实组顺序、保留当前草稿／选择、边界／幂等／写盘失败及冷启动，不能顺带加载或改变匹配优先级 |
| CFG-EXPORT／M2-A | 导出按钮 → SettingTransferDialog::ExportMode 采集组／文件 → exportSelectedGroups | 数据来自固定磁盘 CONFIG_FILENAME，未保存草稿不在导出中；只拷选择组，General 仅显式选择时包含。通过 QSettings childKeys 逐组复制，未知组跳过；目标已有多余键不清除。sync 后未检查 status 就报告完成，不能直接复用为可靠提交结果 | 共用规范格式／范围与结果检查，普通桥接负责外部文件权限；EQ-CFG-EXPORT 比较磁盘与草稿差异、General／GlobalSetting／普通组、已有目标文件、未知组、无法写入、Qt5 Unicode 和独立新进程读回。未支持数据不得借导出宣称完整覆盖 |
| CFG-IMPORT／M2-A | 导入窗口采集组／文件 → importSelectedGroups → 同名组确认 → 写固定 CONFIG_FILENAME → 重载及界面／设备状态刷新 | 逐组 childKeys 合并，目标未在源中出现的键保留，不是整个组替换；General 包含全部所选根键，缺少 LanguageIndex 时其存在判定与选择窗口按任意 root key 判定不同。sync 未检查 status 即报告成功；顺序规范化代码现位于 copied==0 失败分支内，成功路径不执行该块。后续 load_all 可改变全局设置／有效状态并刷新设备相关状态 | 提取同一格式校验、覆盖／保留规则、跨能力影响预览和可靠写盘／重载结果；外部内容由普通桥接读，不能把未支持或未经风险许可的根项当任意 INI 写入口。EQ-CFG-IMPORT 覆盖取消、部分忽略、General 判定、顺序、未知／旧格式、写盘失败及冷启动；先复现上述静态差异，再在共同路径修正 |
| WIN-CRITERIA／M2-B | 主窗进程／标题／类名文本 textEdited 与三种模式 currentIndexChanged → dirty；confirmProcess／WindowTitle／ClassNameLineEdit 对 RegexMatch 的非法表达式仅提示 | 模式 Ignore／Equals／Contains／StartsWith／EndsWith／RegexMatch=0…5。实际 matchForegroundWindow／updateHWNDListProc 使用 MAP_PROCESSINFO 缓存串与当前模式；手工编辑框不直接写缓存，saveKeyMapSetting 写组内字符串／模式后重载。大小写按现有 QString 默认，不能套用全屏白名单的不区分大小写规则；非法 regex 不匹配 | 共用草稿编辑及相同校验／提示，保存／加载再更新有效缓存；若改即时生效，两入口必须共同调整。EQ-WIN-CRITERIA 分别检查草稿、缓存、磁盘及实际目标，覆盖六模式、空值／全忽略、正则、大小写、非法值及运行限制 |
| WIN-DISPLAY／M2-B | checkDisplayModeComboBox currentIndexChanged → dirty；Ignore=0／Fullscreen=1／Windowed=2 | 配置组 PROCESSINFO_DISPLAYMODE_MATCH_INDEX；displayModeMatches 用实际前台窗口的 isWindowFullscreen 结果判断；当前匹配、自动选配置及进程表双击选择都涉及此条件，后两者的候选规则不同。不是只按窗口大小文本或控件值判断全屏 | 共用模式校验及既有消费者；EQ-WIN-DISPLAY 比较实际 HWND 变化、显式条件与 Ignore、其他条件为空、候选并列及保存恢复。受控实际前台目标需要 Default，私有桌面静态显示不能替代 |
| WIN-PROCESS-SELECT／M2-B | 进程表真实双击 → on_processinfoTable_doubleClicked，IDLE／表格启用守卫；从行取 PID、HWND、标题、类名，查询实际路径和显示模式 | 未按物理左 Ctrl 时先 matchSavedSettings，可能加载已有配置；没有匹配时设置空白选择及默认条件，再 setMapProcessInfo、图标及文本／tooltip；dirty 由既有控件信号决定，setText 本身不发 textEdited。空标题另把编辑串转为空串正则并切 RegexMatch，此时缓存原值和编辑串仍不同。必要时既有逻辑临时尝试 SeDebugPrivilege，不能据旧行直接信任已复用 PID／HWND；全函数不是单纯设置文本 | 共用经核实的目标进程／窗口快照和“尝试加载已有配置”明确意图，GUI 采集 Ctrl，MCP 不伪造键态。EQ-WIN-PROCESS-SELECT 覆盖失效行／进程退出、同 PID 新进程、无路径、空标题、已匹配及未匹配、草稿保护与实际缓存／图标；扩大权限或前台操作另按既定规则 |
| WIN-RESTORE-PATH／M2-B | restoreProcessPathButton → on_restoreProcessPathButton_clicked → 路径非空且与编辑串不同 → 确认后 setText | 使用 m_MapProcessInfo.FilePath 恢复进程条件编辑框；没有直接更新 FileName 缓存或持久化。此 setText 不发主窗 dirty 所监听的 textEdited；显示、缓存和未保存状态应独立断言 | 将明确恢复目标／确认与共同草稿提交相连，核对是否需共同修正 dirty；EQ-WIN-RESTORE-PATH 覆盖空／相同路径、取消、与当前匹配串不同、dirty、保存及实际匹配。当前只读发现信号差异，未作产品运行缺陷复现 |
| WIN-AUTOSTART／M2-B | autoStartMappingCheckBox stateChanged → 联动禁用／启用“发送到相同窗口”，另 toggled → dirty；此 UI 文案为“自动匹配前台” | 配置组 AUTOSTARTMAPPING_CHECKED 存 CheckState；matchAutoStartSaveSettings 从磁盘读取且只认 Qt::Checked，按三条件具体程度优先、并列偏显式显示模式，再受现有组遍历顺序影响。全局自动回退另读 s_GlobalSettingAutoStart；并非此开关一勾就启动引擎，也不是进程启动自动运行开关 | 共用明确语义、check state／范围、联动与缓存更新时机；EQ-WIN-AUTOSTART 比较只改草稿／保存后磁盘、同优先级、无条件组跳过、全局回退、部分选中旧值及冷启动；启动动作仍由明确接口执行 |
| WIN-SAME-WINDOWS／M2-B | sendToSameTitleWindowsCheckBox toggled → dirty；autoStartMappingCheckBox 勾选或全局配置下界面禁用，底层勾选值可保留 | 配置组 SENDTOSAMEWINDOWS_CHECKED；getSendToSameTitleWindowsStatus 同时要求“未自动匹配”且本项勾选。updateHWNDListProc 按实际条件枚举 s_last_HWNDList／当前目标；Worker 相应后台键鼠发送路径遍历该列表，不等于所有输入后端都广播 | 共用联动、原始值／有效值和状态守卫；EQ-WIN-SAME-WINDOWS 比较禁用后保留与实际 false、两个受控目标、窗口退出／匹配变化、不同发送方式、释放清理及冷启动。真实目标效果不能由列表数量替代 |
| WIN-LAST-MATCH／M2-B | 自动匹配 recordLastAutoMatchedSetting；进入全局回退时 startLastAutoMatchedSettingTimer；停止可 switchBackToLastMatchedSetting | 临时内存组名及 timer；不记录空白／GlobalSetting，恢复前先清记录以免递归，按真实列表找组并完整 load／flush；超时清除。停止可能切换配置，故“停止完成后继续编辑原目标”仍须重新核对配置／修订 | 复用既有恢复而非新增另一套记录；EQ-WIN-LAST-MATCH 覆盖计时有效／过期、配置删除、停止恢复、重复停止不重复恢复、重连及旧目标提交拒绝。完成边界与 RUN-STOP／SRC-26 联验 |
| IGNORE-UPSERT／M2-B | 忽略规则“添加／更新” → on_saveRuleButton_clicked；trim 非空名称、进程／标题／类名／描述、三模式 0…5 及 disabled → s_IgnoreWindowInfoMap[ruleName] | 按名称覆盖保持 OrderedMap 位置，内容变化才 dirty；编辑其他名称会新增，非自动重命名。名称之外该入口不阻止无条件或非法 regex；isWindowInIgnoreList 跳过 disabled／全空条件，启用条件 AND、各规则 OR，区分大小写，非法 regex 不匹配。保存到根 IGNOREWINDOWINFO_RULES 的九字段列表，完整配置保存时写盘 | 共用明确规则值、作用域、已有校验／诊断和 dirty；EQ-IGNORE-UPSERT 比较新增／同名／无变化、禁用、无有效条件、六模式及非法 regex、规则顺序、当前与已保存状态及实际忽略效果，不只检查列表文字 |
| IGNORE-DELETE／M2-B | 删除按钮从 currentItem 取名称 → 确认 → OrderedMap.remove → 刷新／dirty；若编辑框正在显示该名则清编辑草稿 | 不存在／无 currentItem 不修改；未直接持久化。currentItem 与可见选择可能不同，显式 MCP 目标须核对相同业务名称，GUI 选择一致性另列回归，不能照搬此前 MCP 管理窗选择逻辑就宣称此处通过 | 共用明确名称／确认及结果；EQ-IGNORE-DELETE 覆盖当前与选中项不同、取消、最后一条、编辑草稿保持／清理、实际忽略恢复及冷启动 |
| IGNORE-DEFAULTS／M2-B | 恢复默认按钮 → 全量替换确认 → initIgnoreWindowInfoList → 列表／编辑区刷新及 dirty | 清掉全部现有规则并重建九条内建项，不是仅补缺失；内建描述还随 updateIgnoreWindowInfoListDescriptionTranslation 语言刷新。缺失／无法转换的根规则值加载时走默认，合法空列表则保留空；部分非法元素跳过 | 共用明确全量替换及影响预览，保存仍由意图决定；EQ-IGNORE-DEFAULTS 比较取消、自定义项删除、九条默认／禁用状态、空列表与缺失区别、三语言、有效忽略及冷启动 |
| IGNORE-EDITOR／M2-B | 规则双击 → loadRuleToUI；清空按钮可确认后 initRuleWindowInfoArea | 仅编辑区草稿，不改变 OrderedMap 或磁盘。默认模式为进程／标题 Contains、类名 Ignore，清除五段文本并取消 disabled；名称 textChanged 只改变“添加／更新”文案 | 仅在必要 UI 会话能力中暴露明确选中／清草稿；业务 upsert 不依赖用户当前编辑框。EQ-IGNORE-EDITOR 比较清空确认、未提交内容、列表源未变、关闭再打开及与 MCP 业务更新并发 |

配置迁移、组级导入及恢复不能绕过能力覆盖／停止守卫：General 或普通组可能包含驱动、输入、通知、窗口和全局项，必须先通过共用解析形成已声明能力的完整变更清单，拒绝或明确报告尚未支持的内容，不能将现有 INI 复制函数直接变成高权限远程写任意键的入口。

### 首个编辑闭环的参数补充：SRC-65

PC-W10／a8b8a6fa 的解析及 GUI 链路，当前 WRITE-64 候选不改这些函数。本节补充 MAP-ADD／MAP-EDIT 的参数事实，不新增关键链路或已开放能力计数；帮助／schema 尚待产品接入。状态限制仍由共用操作执行，下面的键串校验函数本身不能代替 IDLE／清理守卫。

| 参数／入口 | 当前源码约束及实际处理 | MCP 接入及待验要求 |
|---|---|---|
| 键名与组合；主窗口录入／映射项更新 | validateOriginalKeyString／validateMappingKeyString 共用 QItemSetupDialog 的有效键名集合；集合来自真实主窗口列表，映射集合另移除仅可作语法的分隔符／前缀／函数占位项。原始组合以 + 拆分并拒绝重复；特殊原始键、SendOn 及 VButton 不可直接加入组合 | 按当前构建的有效集合发现键名，结构化语法另说明，不把下拉框占位词当可直接发送的键。不另写 MCP 解析器；校验须接受明确源上下文，不能临时切 Tab 或替换全局表。EQ-KEY-NAMES／EQ-KEY-SYNTAX 待验 |
| 原始键设备／按压后缀；键盘／鼠标选择、按压类型与 pressTimeSpinBox | 单键解析接受一位 @0～@9；设备语法不证明驱动或设备可用。⏲N／✖N 为 1～9999 ms、默认按压框 500 ms，拒绝前导 0。VButton 的组合／按压有专用拒绝；设备后缀存在下述待复现差异，不能仅按注释宣称已拒绝。部分触摸板原始键另有限制 | 单位明确为 ms，能力发现区分语法与实际设备依赖；先做普通 A→B，相关设备／特殊键按阶段接入。EQ-KEY-TRIGGER／DEVICE 待验 |
| 映射等待；waitTimeSpinBox → 添加槽／键串解析 | GUI 范围 0～999999 ms；0 表示不追加等待后缀，字面 B⏱0 及前导 0 被拒绝。实际后缀 B⏱N 或 B⏱(min~max) 两端为 1～999999 ms、min≤max；部分功能键不追加该后缀 | 帮助与正规化保留“无后缀”和非法 ⏱0 的区别，不把控件下限直接当 DSL 下限。EQ-KEY-WAIT 的 0／1／最大值／越界／随机反向待验 |
| 序列／宏；映射键列表、validateSingleMappingKey 及展开 | 空段、重复组合（含去修饰后重复）拒绝；当前完整序列校验要求段数严格小于 KEY_SEQUENCE_MAX=1000000。宏语法通过不检查其存在，存在性在运行展开处理；包装嵌套另按原上限处理 | 预览报告语法、依赖解析与可执行性各自结果，不能以语法成功保证运行效果；引用、展开预算和嵌套沿既有实现核实。EQ-KEY-SEQUENCE／MACRO 待验 |
| 连发／重复；映射项窗口的 SpinBox／更新入口 | 连发按住／间隔各 1～99999 ms，默认 40／20；重复模式 0 无／1 按键／2 次数，次数 0～99999999、默认 1，次数模式 0 为无限。Macro(name)xN 的语法重复与本字段不同 | 字段类型、单位、模式依赖、无限含义与默认值随首个相应能力输出；不将不同重复机制混为同一上限。EQ-MAP-TIMING／REPEAT 待验 |
| 添加位置与副作用；on_addmapdataButton_clicked → insertNewMappingData | 添加槽包含键名生成、等待／触发后缀和互斥选择；insertNewMappingData 读取物理 Shift／当前选区决定插入位置，刷新、重选及可能打开编辑窗。两者不能作为纯预览 | 共用提交接收明确目标和位置，GUI 仅采集手势；预览不调用插入或冲突修复函数。比较数据／Pure／dirty／通知及磁盘，继续 EQ-MAP-ADD／EQ-INSERT-PREVIEW |

待复现的局部差异：VBUTTON_REGEX_PATTERN 为带首尾锚点的 ^VButton\{(.+)\}$；VButton{Button1}@0 不能进入外层的设备后缀拒绝分支，而单键辅助校验先拆 @0 后又以 VButton 结构接受，静态路径可能放行与注释意图不符的格式。未运行验证、未修改解析器，相关 VButton 接入前在共用校验中复现并处理，不据此改变首个普通键闭环的范围或宣称产品崩溃。

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
| `U-general/startupPositionComboBox` | QComboBox | `connect currentIndexChanged @75` | 静态见 GEN-STARTUP-MODE；业务／运行未验 |
| `U-general/startupSpecifyPositionXSpinBox` | QSpinBox | `connect valueChanged @85` | 静态见 GEN-STARTUP-POINT；业务／运行未验 |
| `U-general/startupSpecifyPositionYSpinBox` | QSpinBox | `connect valueChanged @87` | 静态见 GEN-STARTUP-POINT；业务／运行未验 |
| `U-general/saveWindowSizeCheckBox` | QCheckBox；Save Window Size | `connect toggled @95` | 静态见 GEN-SAVE-SIZE；业务／运行未验 |
| `U-general/saveSplitterPositionCheckBox` | QCheckBox；Save Window Splitter Position | `connect toggled @97` | 静态见 GEN-SAVE-SPLITTER；业务／运行未验 |
| `U-general/tableEditModeTriggerComboBox` | QComboBox | `connect currentIndexChanged @89` | 静态见 GEN-EDIT-TRIGGER；业务／运行未验 |
| `U-general/tableInsertModeComboBox` | QComboBox | `connect currentIndexChanged @91` | 静态见 GEN-INSERT-MODE；业务／运行未验 |
| `U-general/disableGlobalMappingInFullscreenCheckBox` | QCheckBox；Disable GlobalKeyMapping in Fullscreen | `connect toggled @80` | 静态见 GEN-FULLSCREEN；业务／运行未验 |
| `U-general/fullscreenGlobalMappingProcessLineEdit` | QLineEdit | `connect textEdited @93` | 静态见 GEN-FULLSCREEN-PROCESSES；业务／运行未验 |
| `U-general/soundEffectCheckBox` | QCheckBox；Notification Sound | `connect toggled @99` | 静态见 GEN-SOUND；业务／运行未验 |
| `U-general/globalSettingSwitchTimerSpinBox` | QSpinBox | `connect valueChanged @101` | 静态见 GEN-GLOBAL-SWITCH；业务／运行未验 |

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
| `U-ignore_rules/ruleNameLineEdit` | QLineEdit | `on_ruleNameLineEdit_textChanged @358` | 静态见 IGNORE-UPSERT；业务／运行未验 |
| `U-ignore_rules/saveRuleButton` | QPushButton；Add | `on_saveRuleButton_clicked @188` | 静态见 IGNORE-UPSERT；业务／运行未验 |
| `U-ignore_rules/ruleProcessNameMatchTypeComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 静态见 IGNORE-UPSERT；业务／运行未验 |
| `U-ignore_rules/ruleProcessNameLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 静态见 IGNORE-UPSERT；业务／运行未验 |
| `U-ignore_rules/ruleWindowTitleMatchTypeComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 静态见 IGNORE-UPSERT；业务／运行未验 |
| `U-ignore_rules/ruleWindowTitleLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 静态见 IGNORE-UPSERT；业务／运行未验 |
| `U-ignore_rules/clearRuleButton` | QPushButton；Clear | `on_clearRuleButton_clicked @257` | 静态见 IGNORE-EDITOR；业务／运行未验 |
| `U-ignore_rules/ruleClassNameMatchTypeComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 静态见 IGNORE-UPSERT；业务／运行未验 |
| `U-ignore_rules/ruleClassNameLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 静态见 IGNORE-UPSERT；业务／运行未验 |
| `U-ignore_rules/ruleDescriptionLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 静态见 IGNORE-UPSERT；业务／运行未验 |
| `U-ignore_rules/ruleDisabledCheckBox` | QCheckBox；Disabled | 手动／间接绑定待逐项定位 | 静态见 IGNORE-UPSERT；业务／运行未验 |
| `U-ignore_rules/ruleListWidget` | QListWidget | `on_ruleListWidget_itemDoubleClicked @365` | 静态见 IGNORE-EDITOR；业务／运行未验 |
| `U-ignore_rules/deleteRuleButton` | QPushButton；Delete | `on_deleteRuleButton_clicked @286` | 静态见 IGNORE-DELETE；业务／运行未验 |
| `U-ignore_rules/restoreDefaultRulesButton` | QPushButton；RestoreDefault | `on_restoreDefaultRulesButton_clicked @333` | 静态见 IGNORE-DEFAULTS；业务／运行未验 |

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
| `U-main/settingselectComboBox` | SettingSelectComboBox | `on_settingselectComboBox_textActivated @43446`<br>`on_settingselectComboBox_currentTextChanged @46050` | 静态见 CFG-LOAD；业务／运行未验 |
| `U-main/savemaplistButton` | QPushButton；SaveSetting | `on_savemaplistButton_clicked @31908` | 运行约束待核 |
| `U-main/settingTabWidget` | QTabWidget | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/checkProcessComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 静态见 WIN-CRITERIA；业务／运行未验 |
| `U-main/processLineEdit` | QLineEdit | `connect returnPressed @4034` | 静态见 WIN-CRITERIA；业务／运行未验 |
| `U-main/restoreProcessPathButton` | QPushButton；Restore | `on_restoreProcessPathButton_clicked @49702` | 静态见 WIN-RESTORE-PATH；业务／运行未验 |
| `U-main/checkWindowTitleComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 静态见 WIN-CRITERIA；业务／运行未验 |
| `U-main/windowTitleLineEdit` | QLineEdit | `connect returnPressed @4035` | 静态见 WIN-CRITERIA；业务／运行未验 |
| `U-main/checkClassNameComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 静态见 WIN-CRITERIA；业务／运行未验 |
| `U-main/classNameLineEdit` | QLineEdit | `connect returnPressed @4036` | 静态见 WIN-CRITERIA；业务／运行未验 |
| `U-main/checkDisplayModeComboBox` | QComboBox | 手动／间接绑定待逐项定位 | 静态见 WIN-DISPLAY；业务／运行未验 |
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
| `U-main/autoStartMappingCheckBox` | QCheckBox；Auto Match Foreground | `on_autoStartMappingCheckBox_stateChanged @49333` | 静态见 WIN-AUTOSTART；业务／运行未验 |
| `U-main/mappingStopKeyLineEdit` | QLineEdit | 手动／间接绑定待逐项定位 | 运行约束待核 |
| `U-main/enableSystemFilterKeyButton` | QPushButton；SystemFilterKey | `connect clicked @34357` | 运行约束待核 |
| `U-main/mappingAdvancedSettingButton` | QPushButton；Mapping Advanced | `on_mappingAdvancedSettingButton_clicked @49940` | 运行约束待核 |
| `U-main/sendToSameTitleWindowsCheckBox` | QCheckBox；Send To Same Windows | 手动／间接绑定待逐项定位 | 静态见 WIN-SAME-WINDOWS；业务／运行未验 |
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
| `U-mapping_advanced/mouseXSpeedSpinBox` | QSpinBox | `connect valueChanged @121` | 静态见 ADV-MOUSE-SPEED；业务／运行未验 |
| `U-mapping_advanced/mouseYSpeedSpinBox` | QSpinBox | `connect valueChanged @123` | 静态见 ADV-MOUSE-SPEED；业务／运行未验 |
| `U-mapping_advanced/mousePollingIntervalSpinBox` | QSpinBox | `connect valueChanged @125` | 静态见 ADV-MOUSE-POLL；业务／运行未验 |
| `U-mapping_advanced/ProcessIconAsTrayIconCheckBox` | QCheckBox；ProcessIcon as TrayIcon | `connect toggled @135` | 静态见 ADV-PROCESS-TRAY；业务／运行未验 |
| `U-mapping_advanced/acceptVirtualGamepadInputCheckBox` | QCheckBox；Accept Virtual Gamepad Input | `connect toggled @137` | 静态见 ADV-VIRTUAL-INPUT；业务／运行未验 |
| `U-mapping_advanced/enableCommonMappingTableCheckBox` | QCheckBox；Enable Common Mapping Table | `connect toggled @139` | 静态见 ADV-COMMON；业务／运行未验 |
| `U-mapping_advanced/showWindowPointKeyComboBox` | QComboBox | `connect currentIndexChanged @144` | 静态见 ADV-POINT-KEYS；业务／运行未验 |
| `U-mapping_advanced/showScreenPointKeyComboBox` | QComboBox | `connect currentIndexChanged @146` | 静态见 ADV-POINT-KEYS；业务／运行未验 |
| `U-mapping_advanced/customNotificationEnableCheckBox` | QCheckBox；Enable Custom Notification | `connect toggled @114` | 静态见 ADV-NOTIFY-ENABLE；业务／运行未验 |
| `U-mapping_advanced/customNotificationPositionComboBox` | QComboBox | `connect currentIndexChanged @148` | 静态见 ADV-NOTIFY-POSITION；业务／运行未验 |
| `U-mapping_advanced/customNotificationSetupButton` | QPushButton；Custom Notification Setup | `connect clicked @119` | 静态见 ADV-NOTIFY-ENABLE；业务／运行未验 |
| `U-mapping_advanced/LT_Threshold_PressSpinBox` | QSpinBox | `connect valueChanged @256` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/LT_Threshold_LightPressSpinBox` | QSpinBox | `connect valueChanged @247`<br>`connect valueChanged @251` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/LT_Threshold_ReleaseSpinBox` | QSpinBox | `connect valueChanged @239` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/RT_Threshold_PressSpinBox` | QSpinBox | `connect valueChanged @283` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/RT_Threshold_LightPressSpinBox` | QSpinBox | `connect valueChanged @274`<br>`connect valueChanged @278` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/RT_Threshold_ReleaseSpinBox` | QSpinBox | `connect valueChanged @266` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/LS_Threshold_PushSpinBox` | QSpinBox | `connect valueChanged @310` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/LS_Threshold_LightPushSpinBox` | QSpinBox | `connect valueChanged @301`<br>`connect valueChanged @305` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/LS_Threshold_ReleaseSpinBox` | QSpinBox | `connect valueChanged @293` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/RS_Threshold_PushSpinBox` | QSpinBox | `connect valueChanged @337` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/RS_Threshold_LightPushSpinBox` | QSpinBox | `connect valueChanged @328`<br>`connect valueChanged @332` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/RS_Threshold_ReleaseSpinBox` | QSpinBox | `connect valueChanged @320` | 静态见 ADV-THRESHOLDS；业务／运行未验 |
| `U-mapping_advanced/gamepadTouchpadXSpeedSpinBox` | QDoubleSpinBox | `connect valueChanged @127` | 静态见 ADV-TOUCHPAD-SPEED；业务／运行未验 |
| `U-mapping_advanced/gamepadTouchpadYSpeedSpinBox` | QDoubleSpinBox | `connect valueChanged @129` | 静态见 ADV-TOUCHPAD-SPEED；业务／运行未验 |
| `U-mapping_advanced/gamepadTouchpadInvertXCheckBox` | QCheckBox；Invert X | `connect toggled @131` | 静态见 ADV-TOUCHPAD-INVERT；业务／运行未验 |
| `U-mapping_advanced/gamepadTouchpadInvertYCheckBox` | QCheckBox；Invert Y | `connect toggled @133` | 静态见 ADV-TOUCHPAD-INVERT；业务／运行未验 |

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

## DEMO-P1-108：正式有界窗口PNG

windows.capture／qkm_capture_window固定main／item_setup、window_ref、source_revision及source_tab_ref，Read+CaptureWindows独立准入，不接受原生目标／路径；GUI短绘制等待前后及抓图后复核同一请求／窗口／完整源和实际展示Tab，单项另核对源行与三个键文本。只捕获已显示且恢复的自身Qt客户区，不调用显示／激活，不从Worker STOPPED推定安全源。上限160KiB PNG／8M像素及完整帧，普通桥接核验准确认证对端／请求／PNG结构与CRC并返回原生ImageContent及无重复Base64的元数据／SHA256，无正式文件输出。

Win11双Qt组件各3、IPC各3及Python图片7项通过；正式原manifest产品三场18项CPP，最终109次记录调用＋7次schema拒绝。两套有Capture取得六张MCP图片，Agent原尺寸查看中文／A→B，旧源／旧窗／错Tab、未应用C草稿、最小化／关闭及无Capture拒绝，源／INI保持且清理五项零。当前十三工具／十二能力；详见[同一C1证据](map67c1_validation.md#demo-p1-108正式有界窗口png2026-10-28pc-w11)。仅DPR1私有桌面，单项打开仍经维护真实GUI；正式展示、其他页面、实际超时／撤权交错、串行重置、预热／免Python及QQ仍待，完整P1／M1未完成。

## DEMO-P1-109：私有后台源行设定窗打开

mapping.editor.open／qkm_open_mapping_editor只接稳定source_ref与private_background，固定本实例单项窗、Read+ShowWindows=129。执行层核验非Default且非输入的实际WinSta0桌面、当前安全源／可见Tab／非隐藏显示行到真实源的对应，复用原showItemSetupDialog；已有编辑窗／草稿、表格编辑或模态／弹窗拒绝，临时禁止激活后恢复原属性，不改映射／INI或切Tab。返回目标／源／新window引用，不收原生目标或路径。

Win11双Qt私有组件各4、Default只读拒绝各3、IPC权限各3通过（含init／cleanup）；三场正式原manifest产品18项CPP、77次记录调用＋12次schema拒绝，八张MCP图由Agent查看，Qt6正式打开／真实GUI双击两角色PNG字节相同。源／dirty／加载／INI不变，重复打开／C草稿守卫及无Show拒绝，逐场清理五项零；两正例各三次Default前台采样非本测试PID，不当作全程因果证明。当前十四工具／十三能力；详见[同一C1证据](map67c1_validation.md#demo-p1-109私有后台正式打开源行窗口2026-10-28pc-w11)。正式安全关闭／串行重置、普通Default显示确认、其他页面、预热／免Python和QQ仍待，完整P1／M1未完。

## DEMO-P1-110：正式打开记录与安全关闭

mapping.editor.close／qkm_close_mapping_editor要求Read+ShowWindows、稳定source_ref／window_ref及private_background；仅本owner正式open记录且完整安全源／全部当前输入与pending备注摘要未变可关闭。固定本实例单项窗，不接discard或原生目标；可见子窗、其他编辑、模态／popup／源或草稿变化拒绝，原GUI close后核实隐藏／源不变，不清空映射。摘要内存单记录，不返回原文／控件指针；默认hasOpenMappingEditor语义保持。

Win11双Qt最终私有组件各4、IPC各3通过（含init／cleanup）；正式双Qt各两轮安全关闭，连同复用无Show负例为18项CPP／94次调用＋15次schema拒绝。GUI未归属窗口、C草稿及旧源／旧窗拒绝，恢复B后关闭，源／dirty／加载／INI保持、逐场清理零。十张图审阅，最终正例与子窗前图字节一致；两套GUI／正式／重开单项图字节相同。详见[同一C1证据](map67c1_validation.md#demo-p1-110正式打开记录及安全关闭2026-10-28pc-w11)。当前十五工具／十四能力；实际备注／数值／子窗业务交错、P2源重置、预热／免Python及QQ仍待，完整P1／M1未完。

## DELETE-GUI-111：删除原GUI基线，尚无正式MCP删除

P1-110之上仅增加固定维护观察，产品deleteSelectedMappingData及头文件未改。Win11双Qt普通最后行／共通首行／共通全行六场，各7项共42项、native exit0，真实Qt选表／选行和Delete／Ctrl+A；完整源剩余fields／derived、其他快照源保持，dirty、加载代次、选择与INI已核对，正常退出及清理五项零。见[同一C1证据](map67c1_validation.md#delete-gui-111源行删除原gui基线2026-10-28pc-w11)。仅原GUI基线，未覆盖过滤／附加共通／分隔行／混合来源、独立备份或正式MCP引用／删除；工具15／能力14不变，完整EQ-MAP-DELETE／M1未完成。

## DELETE-112：共用单源删除，正式MCP删除待接通

111之上提取私有removeMappingRows，GUI仍负责可见选行／混合源限制；共用入口全部预检后倒序去重，原刷新／鼠标点／接替选区／dirty保留。双Qt八场66项通过，其中六场GUI完整记录与111精确相等，另两场共用边界覆盖无效参数零修改及逆序重复行。原manifest／实际私有桌面／正常退出／逐场清理全零。见[同一C1证据](map67c1_validation.md#delete-112单源删除共用业务2026-10-29pc-w11)。15工具／14能力不变；未开放正式MCP删除／重置，过滤／附加Common等扩展矩阵仍待，不标完整EQ-MAP-DELETE／M1完成。

## DELETE-113：稳定源行删除已接通，完整删除／重置矩阵待验

mapping.delete.preview／apply复用现有工具，Read+Configure及独立confirm_delete，完整owner源修订、当前附着可见单源1–100引用、去重／预检、无编辑窗／表格编辑／弹出菜单、完整清理及runtime互斥后调用112原业务。仅改内存／dirty，结果归原实例，旧引用失效、成功重放不再删除；当前15工具／16能力。双Qt十二场96项native、450 handler／471 SDK＋84 schema，八次正式删除，原GUI剩余全字段／派生对照及其他源保持；hidden为组件注入，原GUI基线111独立保留。版本／复用／原件及未验边界见[同一C1证据](map67c1_validation.md#delete-113稳定源行删除2026-10-29pc-w11)。不支持普通Tab间接删除附加Common或隐藏／分隔行，不自动切表／关闭／保存／启停；完整EQ-MAP-DELETE、真实筛选／派生／运行清理／其他实例、Win10及P2重置未标完成。

## DEMO-P2-114：可选宿主记录及只读预检

--demo-record仅额外启用无参数qkm_get_demo_state；严格私有记录与固定可信EXE／认证实例／实际进程及桌面匹配，Read+Configure+Show+Capture、固定已加载单一普通空源及空Common备份、完整空闲读取基准、主窗可见／无编辑窗／无dirty才报告ready。返回仅观察，不授权写入／租约／重置或预热；普通15工具／产品16能力保持。双Qt八场64项native、104 handler／176 SDK＋8 schema，27项Python及双Qt各4项窗口组件通过；全部源INI及状态保持、正常退出／五项清理零，见[同一C1证据](map67c1_validation.md#demo-p2-114只读专用实例预检2026-10-29pc-w11)。测试宿主记录不冒充正式预热，非映射上下文／串行回收／免Python和QQ仍待，未标完整P2。
