# SAVE-20261025-67-C1：保存闭环分项证据

[唯一主计划与进度](qkeymapper_mcp_automation_plan_codex.md) · [能力矩阵](capability_matrix.md) · [证据索引](validation.md)

## C1-A：辅助序列化

PC-W10／Win10 19045，源码47238cad加本切片。只提取忽略规则、普通／通用宏及同步组说明的原序列化：三个只读值函数供原四个保存包装器共用。字段、Qt类型、OrderedMap顺序、同步组空值过滤、配置组空名早退、QSettings创建／codec／销毁时机及原调用顺序保留；未改Worker、输入、完整保存／重载或错误处理。

直接保存调用范围仍仅saveKeyMapSetting；未新增MCP能力或工具。未来保存准备可复用这些值函数，源访问、实例／会话／权限、状态、完整保存内容与磁盘修订、到期、用户对其他草稿的同意及提交重检仍由外层负责，单条映射相等／dirty不替代完整保存修订。

### 实际检查

| 检查 | 实际范围和结果 |
|---|---|
| 双Qt普通Release维护构建 | out/tests/sendinput_lifecycle/{qt6,qt5}-resume49/release/QKeyMapperSendInputLifecycleTest.exe。最终原生exit0；Qt6零编译／链接警告，Qt5仅已批准官方qtmain LNK4099。日志out/mcp_bridge/map67c1-20261025/build-qt*-final.log及exit；正式产品EXE本轮未重新部署 |
| 保存及边界 | 双Qt settings_serialization各14项通过：独立期望的所有规则字段／枚举／顺序，中日文／emoji／换行／引号／反斜杠，宏作用域、全部字段、QVariantList注释及空项，同步组ID／空值；纯生成不写盘／dirty且无可变源别名；实际旧包装器保存／加载，空组名无操作、空列表清旧数据、无关键及dirty保持 |
| 独立冷进程 | 同一合成INI保留，完整退出后启动settings_serialization_cold；双Qt各11项通过，磁盘实际内容及原加载器恢复均符合独立期望，检查本身不重写INI |
| 身份与清理 | 四进程exit0、实际私有桌面QkmLifecycle_*，测试控制器及维护副本均Medium、系统登录关联已核实，workAtExit／producersAtExit为0；无系统输入／映射启动／驱动验证 |

实际运行副本为out/mcp_lifecycle/map67c1-ordinary-20261025/runtime-{qt6,qt5}/QKeyMapperSave67_{qt6,qt5}.exe。仅这些副本由普通SDK mt工具改为asInvoker，原构建产物和正式manifest未改。原生manifest及本机脚本／INI／逐项结果／closed.json位于同目录，results.json与acceptance.log记录四组结果；这些原始材料不随Git同步。

初次High维护启动申请独立五分钟缓存，在90秒内未获UAC批准，未启动产品；普通PID7620的cache off后核实不可用，原始失败位于out/mcp_lifecycle/map67c1-20261025。未盲目重发或把失败记为通过。随后使用上述普通副本，只证明数据与文件路径；用户五小时受管PID29236不受影响，截止仍21:07:50 +08:00。

### 边界

本片不是GUI／正式MCP完整保存等价性、High权限、保存失败回滚或全局／全部配置保存的重验。原有失败保护逻辑未改，历史证据只按原版本复用；完整保存范围、其他草稿、保存后映射冷启动及MCP启停仍待后续接入。无新界面布局或输入改动，因此未重复Default截图／性能、ASan或全量静态分析；B1的DP观察结果仍按其原范围保留。

## C1-B1：全局保存内容与原GUI基线

基于b82b60bb。原54处全局setValue表达式及四种按键收藏／历史，移入只读collectGeneralSettingSaveValues(int languageIndex)，用现有OrderedMap保留原写入顺序及QVariant类型，原GUI保存仍逐项写同一QSettings。窗口位置、不可变尺寸基线、条件分隔条、通知／缩放／主题／托盘、全局高级选项及虚拟手柄列表仍取原来源；未扩大为MCP能力或任意INI写入。

GUI在原位置采样语言并传入，不因共用提取改到覆盖确认之后；实际保存的UI_SCALE诊断仍在GUI路径，不在纯生成时重复输出。原忽略规则／宏／说明包装器、配置／映射写入、失败回滚、重载及dirty／latest语义未修改。本片不涉及输入热路径，没有重复做输入性能或设备验收。

### 独立基线与实际结果

- 提取前：b82b60bb生产保存代码加仅GUI按钮驱动的settings_general_save夹具。双Qt普通维护构建后，以自有临时asInvoker副本在私有桌面保存合法A→B、新配置Map67General及四种按键收藏／历史；默认、开启尺寸／分隔条、关闭尺寸／分隔条共六组，每组5项、252个保存键通过。旧生产保存函数尚未调用新的内容生成。
- 提取后：双Qt普通Release维护构建原生exit0，Qt6零警告、Qt5仅已批准官方qtmain LNK4099例外。相同三种前置条件／同路径副本，各9项、完整252键通过；独立磁盘解析的键集合／值及原始INI字节均与对应旧GUI基线相同。只读生成可重复、不改disk／dirty／加载代次／映射；语言参数0／1／2及非法值回退语义通过。保存后实际加载组、dirty清除、四种历史、位置／尺寸基线和A→B均另有预期断言。
- 六组旧GUI与六组当前GUI均真实私有桌面、Medium且同系统登录、exit0及零尾部作业。未发系统输入、未启动映射；普通副本不证明High或驱动行为。

构建日志：out/mcp_bridge/map67c1-20261025/build-qt*-general-{baseline,after,final}.log。实际EXE为out/mcp_lifecycle/map67c1-general-20261025/runtime-{qt6,qt5}/QKeyMapperGeneral67_{qt6,qt5}.exe；旧基线运行后被同路径部署的当前副本替换，不能把当前EXE当作旧二进制。原始baseline-{splitter-,nosplitter-,空}{qt6,qt5}.ini、before*结果、after*-r2结果及closed.json／results.json仅留本机；before为原生产代码的该轮基线，非历史其他场景冒用。

首轮after Qt6两条夹具断言失败：语言期望用OrderedMap.insert移动了已有键的顺序；实际INI的QSettings共享缓存与独立磁盘解析产生列表表示差异。该轮保存的17,937字节已与旧基线逐字节一致，未发现产品内容差异。修正夹具用原位赋值、独立临时磁盘图像解析后重验六组通过；失败after-qt6及failed.ini保留，未覆盖为通过。

### 仍由外层完成

此有序值集只是完整保存的一部分，不能单独授权提交或判定整个保存预览有效；QPoint／QSize／QStringList等类型须保留，不能用丢失Qt类型的通用JSON转换代替内容修订。配置、全部Tab／映射、共通来源、宏／说明、面板、保存前默认化及磁盘依赖仍待合并，实例／会话／权限／到期／状态和实际提交重检保留。保存失败、正式MCP保存／映射冷启动／启停仍未完成；高权限产品及其他系统按相关接入选择验证，不据本片成功保存推定失败路径通过。

## SAVE-20261025-71：共通序列备注的独立原功能修复

基于46d2759e。盘点C1-B2时发现，CommonMappingData独立备份遗漏MappingKeys_Comments及MappingKeys_KeyUp_Comments的写入／读取。双Qt复现：含两段按下／抬起序列的共通行，关闭共通表后经真实GUI保存／重载，两类备注均丢失；其余配置及正常退出保留。不是MCP特有问题，也不以GUI／MCP共同丢失为等价通过。

生产改动仅四行：在原共通QVariantMap中保存两个QStringList，并在原加载器读回；旧文件缺字段时默认空列表。未改normal Tab数组、映射字符串、引擎、分组、撤销、EQ-68比较或Qt codec／持久化时机。未把这项数据修复与后续Tab序列化提取混在一起。

| 实际验证 | 结果与边界 |
|---|---|
| 修复前双Qt | 各5项中仅两个指定备注检查失败，其他3项通过，进程exit2且尾部作业归零；有效红例，无崩溃或超时 |
| 修复后双Qt普通Release维护构建 | 原生exit0，Qt6零警告，Qt5仅原官方qtmain LNK4099例外；build-qt*-common71-{before,after}.log在out/mcp_bridge/map67c1-20261025 |
| 修复后每个Qt | GUI保存5项、独立冷启动5项、共通源附着6项、无备注字段旧格式6项，共22项通过；8个实际进程均exit0、零尾部作业，冷读／旧格式的整个INI前后字节不变 |
| 配置内容 | 中日文、emoji、换行、反斜杠和对齐空项，两类备注分别保留；其他单行配置共用已修复的operator==核对，包含同步组17。该比较按原语义排除Pure和LockState，不据此宣称执行态或整个保存预览有效 |

实际EXE：out/mcp_lifecycle/common71-20261025/runtime-{qt6,qt5}/QKeyMapperCommon71_{qt6,qt5}.exe，均为明确记录的临时asInvoker维护副本；控制器／目标Medium、系统登录关联及实际QkmLifecycle_*私有桌面已核实。before*／after*的逐项结果、closed.json、旧格式legacy-qt*.ini与原始日志仅在本机out。旧副本随后由修复后副本同路径替换，不冒称当前文件仍是修复前EXE。

MCP生产仍使用已有完整源快照／哈希及外层守卫，未新增operator==调用，也未开放保存或备注编辑工具。正式MCP保存等价性、High／真实输入、性能和Win11未在此片运行；本片只证明所选共通备注保存与恢复，不扩大为整个C1完成。用户随后要求在此提交检查点插入CACHE-SKILL-72，之后回到C1-B2。

## C1-B2-TABLE：Tab／映射载荷提取与双Qt对照（2026-10-25，PC-W10）

基于75d5a5e6＋本片生产／维护变化。生产仅QKeyMapper的私有collectMappingTableSaveValues与原GUI写回：97个唯一相对键，保留原表达式、循环、清洗及插入顺序，原位置仍用同一QSettings和配置前缀写盘。tabIndexBeforeSaveReload、保存范围／时机、清理、失败回滚、重载和dirty处理均留在原流程；未改Worker、manifest、依赖锁、协议或MCP工具。完整文件逆变换审阅确认除本片提取外qkeymapper.cpp与75d5a5e6一致，97项表达式及顺序一致，证据scope-review.json／extraction.json。

现有维护夹具新增settings_table_save／settings_table_cold，仍由真实Qt鼠标事件点击GUI Save，经原槽函数保存。三普通Tab含非当前／空Tab，分别覆盖附着及脱离共通表；包含按下／抬起序列、中文／emoji／日文／空备注、分类、非默认颜色／尺寸／透明度、同步组和禁用行。按钮／浮动图片保持不显示，不启动映射、不注入系统键鼠、不用Default桌面；原manifest启动／退出仍有已授权的FakerInput连接及复位等原有副作用。

| 验证 | Qt6.8.3 x64 | Qt5.15.2官方msvc2019_64 kit |
|---|---|---|
| 提取前Release构建及基线 | exit0；零编译警告 | exit0；仅既有qtmain缺PDB例外 |
| 提取前GUI保存／冷读 | 两种共通状态各17＋9，合计52项 | 同52项 |
| 提取后Release构建 | exit0；零编译警告 | exit0；同既有例外 |
| 提取后GUI保存／冷读 | 两种状态各21＋9，合计60项，四个进程exit0 | 同60项／四进程exit0 |
| 完整写盘对照 | 两对完整252键、Qt类型和值相同，INI字节精确一致 | 同两对 |
| 独立冷启动 | 两组恢复预期字段，冷读前后INI字节不变 | 同两组 |

额外断言97项有序载荷可重复生成；不改主数据、三个派生键列表、LockState、dirty、加载代次或磁盘；QPoint／QSize保持Qt类型，GUI逐项写入值等于收集值。保存前指定无图片的托盘像素24×24仍按旧编码写盘，重载后依现有规则清为空；这项既有规范化明确保留，不能把编辑值、已保存值和有效值混为一谈。

首次旧GUI基线仅图标尺寸聚合预期失败（其余16项通过），定位为无有效图片时加载器主动清空像素选择；未修改产品迁就夹具，校准预期后才建立有效基线。保留before.log、failed-initial-qt6-detached.ini。重试准备时遇到已有wav文件不可覆盖及读取内部state快照过旧的断言，均在启动产品前失败；普通脚本改为复用未变部署并通过正式status刷新，未创建额外控制器或提权开发工具。成功基线为before-r2-final.log／before-r2-results.json，前述尝试不计通过。

实际EXE为out/mcp_lifecycle/map67c1-tables-20261025/runtime-{qt6,qt5}-{attached,detached}/QKeyMapperTables67_{qt6,qt5}.exe；原requireAdministrator，未修改manifest。全部由普通父控制器6920、经已登记公共入口启动，测试主体／构建／Git普通权限；每个独立QkmTables桌面、创建时间、EXE内容身份、High／同关联登录、原生退出及检查记录在各save／cold下closed.json和before-r2／after-definitions.json。新目标／case只新增定义，未改缓存核心，未另建控制器。

原始材料本机out/mcp_lifecycle/map67c1-tables-20261025；Release日志out/mcp_bridge/map67c1-tables-20261025/build-qt*-{before-r2,after}.log。完整252键对照及基线INI仅本机out，不随Git。脚本和机器路径为本机取证材料，另一PC按现有维护pro、case环境及公共登记规则恢复，不复制out或假设已有二进制。

本次仅GUI保存与只读载荷基础的独立提交切点。MCP未消费此载荷，仍12工具／8能力；该载荷不能代替完整内容修订、目标身份、其他草稿范围或提交授权。剩余配置／高级设置／面板等编码、完整保存预览与提交、失败回滚及正式MCP保存／冷启动／启停闭环仍待接续。未新增普通产品构建、ASan、严格静态分析、Win11、真实输入／设备、三语言布局或性能结论；按本片纯序列化影响选择双Qt Release维护及完整文件对照，没有越过原R3门槛。

用户要求本片通过后收尾。曾按明确3小时请求把6920延长至23:46:49；本轮原生用例已实际跨过旧21:07截止，但不代表三小时驻留完整通过。正式收尾已off并核实closed、cache_available=false、零拥有／未定记录及last_error=null，见cache-closed.json；不跨PC继承窗口、PID或拥有关系。

## SAVE-74：C1-B2-PROFILE依赖盘点与36项纯参数（2026-10-26，PC-W11）

本机接续基线6d33db2758fa5b43194efe87542d21fa76e95ea9（UI-73），获取远端后仍一致，开始时无本地改动。没有重复Tab／映射提取或应用旧B1补丁。先增加独立GUI保存／冷读维护用例，在生产saveKeyMapSetting尚未变化时建立基线，再提取连续36项键转鼠／手柄阈值／触摸板／陀螺仪／鼠标转虚拟摇杆参数为私有collectProfileParameterSaveValues，原位置以同一前缀及QSettings写回。

Windows 11企业版10.0.26200 x64；本机CPython3.11.9 x64、python.exe／python311.dll的有效PSF签名及旧ENV-39哈希安装日志已核对。31项实际锁依赖与旧安装条目／hash一致，环境及pip check、6项环境／20项协议接续通过，复用本机原独立venv，未复制外机环境或变更全局安装。首次整文件比较因说明注释差异拒绝，改为逐条requirement及SHA256核对后确认一致，不把该准备失败记为运行通过。原始环境材料为out/mcp_bridge/bootstrap-w11-20261023，本轮日志另存新目录。

Qt6.8.3 msvc2022_64／Qt5.15.2官方msvc2019_64，两者qconfig的QT_ARCH=x86_64，MSVC19.44 x64。新维护构建树out/tests/sendinput_lifecycle/{qt6,qt5}-profile74-release；提取前、后四次native build均exit0，Qt6零警告，Qt5仅既有官方qtmain缺PDB例外。生产cpp唯一锚点、36项键／表达式／顺序和完整逆变换均通过，除该提取外还原为原cpp；header仅一项私有声明，无Worker、manifest、协议、锁或已完成载荷变更。

基线及当前均用正式requireAdministrator维护目标，不使用asInvoker副本。两个维护case为settings_profile_save／settings_profile_cold，默认及非默认样例随测试源码维护。初始配置只在产品启动前写入独立自有INI；正常产品启动恢复，测试通过Qt键盘事件编辑名称和真实鼠标事件点击Save，不直接调用保存／加载函数。准备一个合法A→B和独立Keep74配置，非默认组设置36项标量；不启动映射、不发系统输入，仍保留原main初始化及退出设备清理，不能据此证明驱动效果。

| 检查 | Qt6 | Qt5 |
|---|---|---|
| 提取前旧GUI保存／独立冷读 | 默认5＋5、非默认9＋9，共28项／4进程 | 同28项／4进程 |
| 提取后保存／冷读 | 默认9＋9、非默认13＋13，共44项／4进程 | 同44项／4进程 |
| 完整落盘对照 | 默认262键、非默认298键，两对完整键集合／Qt解析值及INI字节相同 | 同两对 |
| 独立冷启动 | 四个前／后冷进程均恢复预期且全程INI字节不变 | 同四个 |
| 纯收集 | 36项Qt类型／插入顺序／重复性保留；disk、dirty、加载代次、源epoch不变；实际保存逐项相等 | 同预期 |

所有16进程passed=true、原生exit0、退出workAtExit／producersAtExit均为0。默认／非默认的独立鼠标参数、阈值三元组、浮点／bool期待和其他配置保持均另行断言，不仅比较两个路径彼此一致。当前8个进程的实际桌面／PID：

| 组 | 保存／冷读PID | 产品主线程实际桌面 |
|---|---|---|
| Qt6默认 | 2236／35028 | QkmProfile74_a98c93296d0d4cdc8897d11cb4bb89ef |
| Qt6非默认 | 21532／2192 | QkmProfile74_f44c87febebd4ec29b9163756e125c27 |
| Qt5默认 | 27240／32448 | QkmProfile74_29410cd747e3408e89c412b46d4c3501 |
| Qt5非默认 | 9112／28232 | QkmProfile74_144c9e81e59d425089ba578ea0ea34d1 |

实际EXE为out/mcp_save_profile_w11_20261026/{qt6,qt5}-{default,nondefault}/QKeyMapperProfile74_*.exe，同路径前／后字节身份分别在identity-before／after.json，旧二进制保留old-executables。原始before／after的case、start／closed记录、完整基线INI、build-qt*-before／after.log、run-before／after.log、extraction-review.json及evidence-summary在同一本机目录；不随Git或跨PC复制。

用户明确要求6小时，普通Medium控制器23316于本轮创建，截止2026-10-26 15:44:30北京时间。目标内容及精确环境经技能公共authorize／register登记；仅必要自有原生启动／收尾提权，产品High RID12288、父Medium8192、same_login=true。当前全部目标已退出，status为active／cache_available=true、零owned／unresolved及current_action=null；保持用户所定窗口继续下一切片，不另建缓存或延长。原manifest正常退出仍有原有设备清理，不等于设备释放验收。

本片为独立本地提交切点，继续工作、不推送。剩余窗口匹配的旧路径／图标条件、通知样式是否写入、共通备份同步、面板和保存前后副作用已在唯一主计划盘点，不能用36项或当前源修订代替全部保存范围。MCP仍12工具／8业务能力，完整保存／其他草稿／失败回滚／正式MCP及启停闭环未完成。未新增普通产品全构建、严格静态分析、ASan、真实输入／设备、Default／三语言布局或性能覆盖；已完成UI-73／TABLE及PC-W10材料只按原版本范围复用，不冒充本次Win11结果。

## SAVE-75：窗口匹配条件载荷（2026-10-26，PC-W11）

基线为本机SAVE-74提交e7b73960005c83721d946918e3c6aba9b54954c4。先增加settings_match_save／settings_match_cold，在未改生产保存实现时记录五组真实GUI结果；再提取12处写入、10个条件字段为私有collectWindowMatchSaveValues。全局组跳过分支不动，旧FilePath仍在原GUI位置且仅当前路径为空时读取，图标空值写空／文件存在写入／消失保留旧键等规则不动。唯一锚点、完整逆变换及逆向补丁检查通过；未改此前36项、97项、Worker、manifest、MCP协议、依赖或翻译。

环境复用本机SAVE-74已核实的CPython3.11.9 x64、锁依赖、Qt6.8.3／Qt5.15.2官方x64与MSVC19.44。双Qt前／后四次Release维护构建均native exit0；Qt6零警告，Qt5仅原官方qtmain例外。构建树仍为out/tests/sendinput_lifecycle/{qt6,qt5}-profile74-release，没有从其他PC复制out或再次执行无关环境门禁。

| variant | 独立GUI及磁盘期待 | 每Qt旧GUI保存／冷读 | 每Qt提取后保存／冷读 | 全部INI键 |
|---|---|---|---|---|
| metadata | 正常启动载入路径、图标及中／日／希腊文字段，另存新配置 | 6＋6 | 10＋7 | 276 |
| keep_path | 覆盖已有目标，保留旧FilePath、显式清空图标 | 6＋6 | 10＋7 | 273 |
| draft_fallback | Qt ComboBox键盘激活回到空白草稿、输入带空格进程名，仅路径回退trim | 7＋6 | 11＋7 | 274 |
| global | 保存全局组，原窗口匹配键完全不写 | 6＋6 | 10＋7 | 259 |
| missing_icon | 加载自有图标后删除该文件，真实覆盖确认，保留目标旧路径／图标键 | 7＋6 | 11＋7 | 274 |

每Qt旧GUI62项／10进程，提取后87项／10进程；两Qt总40进程，passed=true、native exit0、workAtExit／producersAtExit均为0。十对完整INI字节与旧基线相同；冷启动由另一进程恢复，启动至退出不改INI。仅warm收集使用原旧路径依赖，检查重复键顺序／Qt类型、条件数量10／9／10／0／8、磁盘／dirty／加载代次／源epoch不变，并逐项核对GUI实际保存值。全局不调用收集器，明确保留其原不写语义。

自有目录在启动前准备夹具，通过产品正常启动加载；名称／空白重置／保存／模态Yes均使用Qt编辑和点击事件，不直接调用加载或保存函数。原requireAdministrator manifest保持；实际EXE为out/mcp_save_match_w11_20261026/{qt6,qt5}-{metadata,keep_path,draft_fallback,global,missing_icon}/QKeyMapperMatch75_*.exe，前／后同路径同名称，旧EXE另存old-executables。可复用样例与五组准备规则在tests/README.md及window_match_save_fixture.ini.example，样例与实际旧基线生成器相等检查通过。

全部目标由同一普通Medium控制器23316经公共authorize／register／perform启动，产品High12288、父8192、same_login=true。各组实际桌面为独立QkmProfile74_<UUID>（沿用本轮控制脚本名称），不是Default；PID／创建时间／实际桌面、native退出在before／after各case的start／closed.json及evidence-summary.json。正式manifest的原设备初始化／退出清理仍执行，本片不启动映射或发系统输入，不据此证明设备效果或清理闭环。

本机原始材料在out/mcp_save_match_w11_20261026：build-qt*-before／after.log、run-before／after.log、十份baseline INI、identity-before／after、源变换patch／review、40组case与身份／退出记录及汇总。cache-after.json确认active／cache_available=true、零owned／unresolved、current_action=null、last_error=null，保留用户6小时窗口至15:44:30继续后续开发，不延长或另建缓存。这些out不随Git，PC-W10证据仍仅按原版本／范围保留。

本片是独立本地提交切点，不推送，继续共通备份／通知／面板和完整保存范围。MCP仍12工具／8业务能力，窗口匹配局部载荷不是完整保存修订、目标／权限／状态或执行前重检的替代。未重新全验TABLE／UI-73、普通产品构建、ASan、严格静态分析、三语言布局、性能、Default或真实输入／设备；新增MCP保存、失败恢复、其他草稿和启停闭环仍待实现／验证。
