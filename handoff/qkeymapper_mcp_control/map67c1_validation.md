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

## SAVE-76：共通备份74字段纯编码（2026-10-26，PC-W11）

基线为SAVE-75本地7dd63fc6d729bd7ee62d3bd0ee04324adaaac644，当前未推送。在生产保存实现未变时添加settings_common_save／settings_common_cold和完整INI夹具，先通过旧GUI基线；再把74项QVariantMap字段／QVariantList行序提为私有静态serializeCommonMappingData(const QList<MAP_KEYDATA>&)。syncCommonMappingDataForSave仍在原位置执行，编码之后的QSettings总写保留，包括空列表清除旧值；未改加载器或已吸收的SAVE-71修复。

唯一锚点、74个唯一字段／表达式、完整逆变换、dry run／numstat及应用后源码／私有声明逐字核对通过。初始difflib对长文件给出扩大上下文的补丁未应用，改用两处局部hunk后为cpp87增／82删、header一项私有声明；未改其他提取、Worker、manifest、MCP或依赖。

同机环境沿SAVE-74，Qt6.8.3／Qt5.15.2官方x64、MSVC19.44；提取前／后四次普通Release维护构建native exit0，Qt6零警告，Qt5仅原qtmain例外。仍使用out/tests/sendinput_lifecycle/{qt6,qt5}-profile74-release；测试EXE为out/mcp_save_common_w11_20261026/{qt6,qt5}-{attached,detached,empty}/QKeyMapperCommon76_*.exe，前／后同路径，保留旧二进制及identity记录，原requireAdministrator manifest不变。

完整初始INI由本机普通权限QtCore-only准备器以Qt自己的QSettings编码嵌套列表，准备器没有启动产品或申请UAC；成品common_save_fixture.ini.example随维护测试保存，Qt5与Qt6都独立读取同一夹具。包含A→B普通表、两条F13／F14共通源行、上下序列备注／空项／中日emoji、非默认QColor／偏移／宽度／同步组，以及不同的旧F20→Z备份。准备器首次cmd引号错误发生在编译／生成前，保留build-fixture-failed1.log，改用既有ProcessStartInfo方式后成功；不计为产品运行通过或失败。

| variant | 独立GUI效果 | 每Qt旧GUI保存／冷读 | 每Qt提取后保存／冷读 | 完整INI键 |
|---|---|---|---|---|
| attached | 正常启动显示两条源，与单条旧备份不同，GUI另存时按原同步写回两行 | 7＋7 | 11＋8 | 277 |
| detached | 真实高级设定入口、共通复选框及Escape关闭窗口，GUI脱离后保存两行备份 | 9＋7 | 13＋8 | 277 |
| empty | 实际Tab点击、表格Ctrl+A／Delete删除全部共通行，GUI保存空列表清除旧备份 | 6＋4 | 10＋5 | 276 |

每Qt旧40项／6进程、提取后55项／6进程；总24进程passed=true、native exit0，workAtExit／producersAtExit均为0。六对完整INI字节与旧GUI相同，独立冷进程全程不改盘。新编码74字段Qt类型／重复行序相等，disk／dirty／加载代次／源epoch及编码前旧备份不变；原Save同步后的全部备份值与只读源编码相同（含空表）。额外明确期待序列行序、上下备注、空位置、转义、分类、颜色、宽度、同步组与其他字段；不以单行operator==替代完整MCP修订。

正常产品启动加载样例；GUI名称编辑、保存、脱离和删除均通过Qt控件事件，不直接调用加载／保存／同步。全部产品主线程实际桌面为独立QkmCommon76_<UUID>、非Default；产品High12288、普通控制器8192、same_login=true，PID／创建时间／实际桌面与退出保留在各case的start／closed.json和evidence-summary.json。仍是原产品初始化／退出设备清理；未启动映射或发系统输入，不能据此判定完整停止或设备效果。

原始材料为本机out/mcp_save_common_w11_20261026的before／after、六份baseline INI、identity、old-executables、四份build-qt*-*.log、运行日志、局部patch／review、实际内容校对及汇总；不随Git。公共登记仍由本机Medium控制器23316，登记前只移除内容已被新EXE替代的旧定义摘要，保留有效目标及历史；缓存核心与期限不变。cache-after.json为active／cache_available=true、零owned／unresolved、current_action=null、last_error=null，保持用户窗口至15:44:30继续开发。

本片为独立本地提交切点，不推送；继续条件通知样式、面板、剩余开关和完整保存范围。未来预览必须只读选择实际附着源或脱离备份并绑定完整依赖，不能提前执行同步。MCP仍12工具／8能力，完整保存／失败恢复／其他草稿／正式MCP与启停闭环未完成；未新增普通产品全构建、ASan、严格静态分析、Default／真实输入／设备、三语言布局或性能结论，原PC-W10证据不冒充本机复验。

## SAVE-77：通知14项条件样式（2026-10-26，PC-W11）

基线为SAVE-76本地41f40fe7918d5bd4627e889188c346ce45c8c482。先添加settings_notification_save／settings_notification_cold，生产原保存代码未变时建立四组独立GUI基线；再提取私有collectCustomNotificationStyleSaveValues(const QSettings&, const QString&) const。它显式读取目标已存样式，保留原两个对话框空指针、初始化、启用和目标旧键的短路条件；不创建或写QSettings。原saveCustomNotificationStyleSettings包装器在原位置按原顺序写入。14项表达式／颜色格式／类型保持，完整逆变换、dry run及实际源码／私有声明逐字校对通过。

环境与构建树沿SAVE-74：Qt6.8.3／Qt5.15.2官方x64、MSVC19.44，双Qt前／后四次普通Release维护构建native exit0；Qt6零警告，Qt5仅原qtmain例外。没有重做无关环境门禁、TABLE／UI-73或普通产品全量构建，未改Worker、manifest、协议、依赖或翻译。

| variant | 源／目标独立设置与期待 | 每Qt旧GUI保存／冷读 | 每Qt提取后保存／冷读 | 完整INI键 |
|---|---|---|---|---|
| absent | 源未启用、未初始化、目标没有样式，保存后14键仍全缺失 | 6＋5 | 10＋6 | 264 |
| enabled | 源已启用但未初始化、无旧样式，保存全部当前有效样式 | 7＋6 | 11＋7 | 278 |
| loaded | 源未启用但已载入14项非默认值，保存完整保留 | 8＋7 | 12＋8 | 292 |
| target | 源未启用／未初始化，已有目标只有FontSize=99，真实覆盖确认后写完整当前有效样式 | 7＋6 | 11＋7 | 277 |

每Qt旧52项／8进程、提取后72项／8进程，两Qt共32进程passed=true、native exit0、workAtExit／producersAtExit为0。八对完整INI字节等于旧GUI基线；另一个进程冷启动至退出不改盘。loaded对RGB字体色、ARGB背景、字体、字号／粗细／斜体、三项时长、圆角／padding、浮点透明度及正负偏移全部14项有独立期待。其他组区分当前初始化旗标与目标旧键，不将禁用误解为必不保存，也不把目标旧值当成当前有效样式。

新收集的条件数量0／14、重复顺序／Qt类型及GUI写回值通过；disk、dirty、加载代次、源epoch、样式初始化和启用旗标不变。源／目标INI只在启动前准备，以正常产品启动加载，名称编辑、保存和target模态Yes均经Qt控件事件，不直接调用加载／保存函数。完整基础准备步骤和14项非默认样例已维护在tests/README.md及notification_save_style.ini.example。保留原空指针分支的源码证明，不把正常已构建对话框的GUI运行当成缺失对话框验收。

原requireAdministrator EXE在out/mcp_save_notification_w11_20261026/{qt6,qt5}-{absent,enabled,loaded,target}/QKeyMapperNotify77_*.exe，同路径前／后身份与旧二进制分别保留identity及old-executables。全部经普通Medium控制器23316公共登记，产品High12288、父8192、same_login=true；主线程实际独立QkmNotify77_<UUID>桌面、PID／创建时间和native退出在before／after各case的start／closed.json及evidence-summary.json。未使用Default、系统输入或映射启动；仍保留原main初始化／退出设备清理，不证明输入或设备效果。

原始材料在同一本机out目录：四份build-qt*-*.log、run-before／after.log、八份baseline INI、case／身份／退出、14字段局部patch／review、应用预期内容及汇总；不随Git。cache-after.json确认active／cache_available=true、零owned／unresolved、current_action=null、last_error=null，沿用用户6小时期限15:44:30，不重启／延长。仅移除已被新EXE替代的旧登记摘要，仍有效范围及历史保留，缓存核心不变。

本片为独立本地提交切点，不推送，继续面板及剩余开关。const QSettings依赖仍须纳入完整目标磁盘／业务修订和执行前重检，不能用局部类型一致开放保存权限。MCP仍12工具／8能力，完整保存／其他草稿／失败恢复和启停闭环仍待实现；未验证通知实际显示、Default、真实输入／设备、性能、ASan、严格静态分析或三语言布局，PC-W10此前证据只按其原版本范围复用。

## SAVE-78：剩余开关、输入标量与面板（2026-10-26，PC-W11）

基线为SAVE-77本地eb684f8961efca0445345fa25a82dd7cd198cdbb。先增加settings_options_save／settings_options_cold与独立INI样例，在生产保存实现未变时建立有效旧GUI基线，再提取私有collectProfileMappingOptionSaveValues／collectProfileInputSaveValues／collectVButtonPanelSaveValues，分别9／6／21有效键，均在原阶段写回。保持Qt checkState／bool、VIGEM条件、两项阈值原round精度、面板实时偏移优先和ARGB。唯一锚点、字段／条件审查、完整逆变换、dry run及应用后实际CPP／header逐字核对通过；通知模式两项、快捷键默认化／焦点、配置顺序、辅助包装器和尾部未移动。没有改Worker、manifest、协议、依赖、翻译或缓存核心。

同机Qt6.8.3／Qt5.15.2官方x64、MSVC19.44和原锁环境；维护构建树沿out/tests/sendinput_lifecycle/{qt6,qt5}-profile74-release。有效旧基线预期校准后及提取后四次Release构建native exit0，Qt6零警告，Qt5仅原qtmain例外；当前日志为这些选定构建。初次未校准构建的日志随后由校准构建覆盖，不能补称仍有独立原始日志；其EXE和失败运行记录另行保留，不计入选定通过。

基础INI由profile_save_fixture派生，正常启动载入VButton{Button1}→B；非默认31项样例含开关、端口／阈值和面板21值，Windows系统过滤键、启动监视、自动匹配、全局虚拟手柄和多设备输入均关闭。内部FilterKeys缺省true、非默认显式false，两者与系统过滤键不同。名称编辑及真实Save另存Map78Profile，不直接调用加载／保存或用控件赋值代替基线。

rich使用产品现有过滤显示入口准备已加载的自有面板，确认只有一按钮且无映射目标HWND；随后Qt背景Ctrl拖动、屏幕参考点0，将实际偏移73／97移到110／126，缓存仍73／97。保存按实际偏移，重载及冷进程恢复相同值；鼠标捕获释放，panel_key_signals始终0。此显示准备不是普通用户完整显示入口验收；没有物理鼠标／系统按键、窗口参考点或外部窗口控制，未开始映射。

| 组 | 每Qt旧GUI保存／冷读 | 每Qt提取后保存／冷读 | 全部INI键／对照 |
|---|---|---|---|
| default | 6＋7 | 10＋8 | 262，完整字节相同 |
| rich | 10＋9 | 15＋10 | 293，完整字节相同 |

每Qt旧32项／4进程、提取后43项／4进程；两Qt共16个选定进程passed=true、native exit0，workAtExit／producersAtExit为0。四对完整INI字节一致，独立冷启动至退出不改INI。新9／6／21项收集保持顺序／Qt类型／重复性，GUI逐项写回相等；disk、dirty、加载代次、源epoch、缓存／实际偏移、可见性和按钮派发不变。非默认布局／字体／5个ARGB色、true／false开关、5307端口及1.23457／2.98765阈值有独立期待。临时空面板指针回退仅组件分支检查，不冒充真实GUI缺失面板；VIGEM关闭时4项路径仅源码核对，未运行该构建。

最初Qt6 default仅一项内部FilterKeys预期失败、exit2，其余保存／重载／停止态及零尾部作业通过；源码确认缺省true后修正测试期待，不改产品。保留failed-initial-before、failed-initial-qt6-default.ini、failed-initial-executables及run-before-failed1.log，不能计为有效基线。随后Qt6两组及Qt5 default成功，最后Qt5 rich在启动前authorize因32摘要上限拒绝；只读核对发现普通脚本从verified_controller的内部旧快照恢复了一个已替换EXE摘要。改为每组先正式status，清理仅该过期摘要，再只补未启动的Qt5 rich两进程；未重跑成功组、重启缓存或重试UAC。

有效旧基线证据由before各case的原生退出与检查汇总、run-before-partial-r2.log（含6个成功进程及最后启动前拒绝）和run-before-complete-r3.log（补2个进程）共同组成。root的run-before.log保留启动前拒绝，不把整次控制脚本exit1改称exit0；16个选定产品native进程均有独立exit0记录。提取后run-after.log整轮成功，汇总evidence-summary.json逐项检查通过。

实际原requireAdministrator EXE为out/mcp_save_options_panel_w11_20261026/{qt6,qt5}-{default,rich}/QKeyMapperOptions78_*.exe，前／后同路径，identity及old-executables保留有效旧二进制。全部经普通Medium控制器23316的公共登记，产品High12288、父8192、same_login=true；主线程实际独立QkmOptions78_<UUID>私有桌面、PID／创建时间／退出在各case的start／closed.json。原main初始化／退出设备清理仍保留，不能据计数认定设备效果或完整停止。

本机原始材料在同一out目录的before／after、四份baseline INI、有效／失败EXE、构建／运行与登记失败日志、局部patch／review、预期内容及汇总，不随Git。cache-after.json为active／cache_available=true、零owned／unresolved、current_action=null、last_error=null；32登记槽／32定义，保留用户6小时截止15:44:30继续开发。后续复用空闲目标或固定case并先正式status，不因容量重启／延长缓存，不恢复旧scope。

本片为独立本地提交切点，不推送。下一项是完整范围／修订和只读准备收口，再按C1-C接入保存预览／提交；两项通知模式、快捷键／目标／顺序／全部辅助集合、其他草稿及保存尾部仍须纳入。MCP仍12工具／8能力，C1-B2及M1未整体完成。未新增普通产品全构建、ASan、严格静态分析、Default／真实输入／设备、性能或三语言布局结论；PC-W10原证据只按原版本范围保留。

## SAVE-79：保存名称与目标只读分析（2026-10-26，PC-W11）

基线为SAVE-78本地bc393e6cb8a12809b4a5c841385f62f7c097534a。在生产保存未变时增加settings_target_save／settings_target_cold，先建立九组独立旧GUI基线，再提取私有SettingSaveTarget／analyzeSettingSaveTarget。请求名称、当前名称、全局选择及已存组显式传入；保留trim、空值、非法字符／换行、规范／翻译后全局保留名、当前组优先、新建及覆盖决定。原QKeyMapper翻译context和消息不变；QMessageBox、STOPPING守卫、分类恢复／运行同步、格式检查、所有写盘和尾部留在原GUI。唯一锚点、完整逆变换、dry run／numstat、实际CPP／header与预期字节校对通过；未改Worker、manifest、协议、依赖或缓存核心。

同机环境和构建树沿SAVE-74，Qt6.8.3／Qt5.15.2官方x64、MSVC19.44。双Qt前／后四次普通Release维护构建native exit0，Qt6零警告，Qt5仅原qtmain例外。实际原requireAdministrator EXE在out/mcp_save_target_w11_20261026/{qt6,qt5}-runtime/QKeyMapperTargets79_*_runtime.exe，前／后同路径同名称，identity-before／after及old-executables保留来源和旧二进制。没有使用另一PC的out或个人INI。

| variant | 原GUI行为／独立期待 | 每Qt旧GUI保存／冷读 | 每Qt提取后保存／冷读 | 完整INI键 |
|---|---|---|---|---|
| empty | 名称仅空格，拒绝、不写盘 | 6＋4 | 9＋5 | 29 |
| invalid | 实际输入全部五个非法字符，拒绝 | 6＋4 | 10＋5 | 29 |
| reserved | 非全局选择输入规范全局名，拒绝 | 6＋4 | 9＋5 | 29 |
| no | 另一个已存目标，实际模态No | 6＋4 | 9＋5 | 29 |
| cancel | 同一覆盖模态实际Escape取消 | 6＋4 | 9＋5 | 29 |
| yes | 实际模态Yes，覆盖目标Q→R为当前A→B | 5＋4 | 8＋5 | 261 |
| current | 同名当前组，无额外覆盖确认 | 5＋4 | 8＋5 | 261 |
| new | 带首尾空格新名称，trim后另存 | 5＋4 | 8＋5 | 268 |
| global | 全局选择直接保存规范全局组 | 4＋4 | 7＋5 | 252 |

每Qt旧85项／18进程、提取后122项／18进程；总72个选定进程passed=true、native exit0、workAtExit／producersAtExit为0。18对完整INI字节相同；独立冷读全程不改盘。拒绝／取消保持原文件、已有不同目标Q→R、当前组、dirty、加载代次和源epoch，成功实际保存及冷进程恢复目标A→B。分析重复错误／结果一致且无上述状态变化。中间换行、本地化全局名、当前组从显式组列表消失仅组件边界，不冒充GUI可输入／可复现基线。该分析不是完整修订或执行许可。

夹具save_target_fixture.ini.example在启动前准备；global只在准备时替换源节和SettingSelect。正常产品启动加载，名称、Save及模态回复通过Qt编辑／点击事件，不直接调用加载或保存函数、不用控件赋值代替旧GUI基线。各目标主线程实际桌面为独立QkmTarget79_<UUID>，父Medium8192、产品High12288、same_login=true，PID／创建时间／桌面／native退出见各case的start／closed.json。未用Default、系统输入或映射启动；原main设备初始化／清理仍执行，零计数不证明完整停止或设备效果。

旧基线首次控制脚本完成Qt6后在Qt5启动前状态门禁失败，未保存该次具体状态，因此原因不补造；正式状态随后健康，只补未启动的Qt5。提取后Qt6全部完成，Qt5前状态门禁捕获最后一个已由句柄确认exit0的目标仍短暂列在owned，cache仍可用。普通控制脚本增加最多五次、间隔0.5秒的只读状态等待，不改缓存核心或清理权限，保留原门禁日志。根run-before／after.log的整体失败不改为0，选定产品退出由独立记录核对。

12:50左右，Qt5已完成empty保存／冷读和invalid保存；下一invalid冷读的公共Start请求5a52d237b3234b0b8f9d04738bd70ca6缺少响应，原生日志为Setting token failed／Failed to tighten security descriptor／Failed to substitute token。原Medium控制器23316以start_result_unknown保护性关闭，历史unresolved_starts保留；只读实际进程审计未发现两个精确测试EXE存活，不能据此证明该请求未启动或完整清理。保存cache-error-closed.json、unknown-start-action.log及unknown-start-result.json，此未知尝试不计入72个通过进程。

第一次按原窗口恢复请求d8731dc037e140b5a0f4e7b3dbf7421b未确认成功、native_exit_code=1，控制器以cache_result_unknown关闭；其原生日志为空，不能推断为用户拒绝或批准。用户午休返回后明确有空审批，再次恢复请求3224df5631944c55b8aa676aa470fdd4获UAC并生效：新Medium控制器25608、创建时间134374652141697846，截止15:44，早于原15:44:30。只在新空scope登记剩余固定Qt5十五项，不迁移旧PID、凭据或拥有关系，不重放原请求。再次审计无存活测试目标且保留的invalid INI等于旧基线后，执行一次独立冷读与其余十四项；它们有自己的Start／退出记录，原未知结果仍未知。

有效证据组合为before各case、run-before-partial-r1／run-before-qt5-r2、after各case、run-after-partial-r1／run-after-qt5-r2及run-after-recovered-qt5.log；最后补十五项的普通脚本exit0。evidence-summary.json核对全部72个有效产品进程、前170／后244检查及18对INI。cache-after.json确认新控制器active／cache_available=true、零owned／unresolved、current_action=null、last_error=null；该当前事实不消除旧控制器的历史未知启动。继续用户已授权开发，不延长截止、不修改全局gsudo配置。

本机原始材料为同一out目录中的构建、前／后结果、18份baseline、身份／旧EXE、局部patch／review／预期字节、状态门禁、未知原生请求与恢复及汇总，不随Git；PC-W10旧结果只按原版本／范围保留。本片为独立本地提交切点，不推送，继续完整范围／修订和只读准备，再进C1-C。MCP仍12工具／8能力，C1-B2／M1未整体完成；未新增普通产品全构建、ASan、严格静态分析、Default／真实输入／设备、性能或三语言布局覆盖，原日文旧标签观察保留。

## SAVE-80：运行态只读投影与共通源选择（2026-10-26，PC-W11）

基线为SAVE-79本地35bdd689。先只增加settings_prepare_save／settings_prepare_cold，保留生产原保存建立独立GUI基线；再增加两个私有const值函数collectMappingRowsForSave／collectCommonRowsForSave。前者只在副本按当前数据指针选择Active或LastActive，保留原qMin、空原键跳过、绑定Tab／行索引及重复绑定最后项覆盖，仅投影Burst／Lock／LockState；后者优先有效附着共通源，否则返回旧备份。既有97字段编码器增加显式projectRuntimeState参数供只读准备，GUI默认false仍取原源行；原运行同步／共通同步及保存前后时机未动，未重新提取Tab字段。

生产CPP两处局部hunk41增／2删，header三处声明／签名3增／1删，唯一锚点、dry run／numstat、完整逆变换及实际源码与预期字节一致。未修改Worker、manifest、MCP协议、依赖、翻译或缓存核心；没有新增MCP保存工具、线程或输入热路径。

环境复用SAVE-74同机锁依赖、Qt6.8.3／Qt5.15.2官方x64、MSVC19.44。最初误在工具沙箱运行两项基线构建，均C1902／native exit2，保留build-qt*-before-sandbox-failed.log，尚未启动产品测试；本机编译器／PDB工具版本匹配，仓库已有同样沙箱限制记录。改回沙箱外普通Medium构建，未改编译选项或工具链。有效前／后四次Release维护构建均exit0，Qt6零警告、Qt5仅原qtmain例外；构建树沿out/tests/sendinput_lifecycle/{qt6,qt5}-profile74-release。

复用common_save_fixture.ini.example，只在启动前替换源节和保留节名为Prepare80Seed／Keep80；正常加载普通A→B、共通F13／F14两行和不同的旧F20备份。Qt编辑名称后，在IDLE人工准备运行列表和源绑定依赖，未开始映射。direct当前指针仍为普通源且忽略人工Active／LastActive；active选Active；last选LastActive且另一Active内容不同。包含负／越界Tab、越界行、空原键、数量不等、同一源重复记录及原键不同但索引绑定有效。人工状态只检验该准备算法与原Save，不冒充实际Hook／停止／运行态端到端验收。

| 组 | 每Qt旧GUI保存／冷读 | 每Qt准备后保存／冷读 | 完整INI |
|---|---|---|---|
| direct | 6＋5 | 13＋6 | 277键，字节相同 |
| active | 6＋5 | 12＋6 | 277键，字节相同 |
| last | 6＋5 | 12＋6 | 277键，字节相同 |

每Qt旧33项／6进程、准备后55项／6进程；共24个进程passed=true、native exit0、workAtExit／producersAtExit为0，六对完整INI字节相等。原Save通过真实按钮完成原同步／写盘／重载，独立冷进程恢复相同Burst／Lock与共通备份且全程不改INI。原始保存和准备后两个普通脚本都exit0；原运行同步未由测试直接调用，也不直接调用加载／保存或用控件赋值替代GUI基线。

只读准备按独立期待核对普通和共通完整74字段，额外核对LockState及三组Pure派生列表；97字段编码的顺序、Qt类型、重复性和原GUI实际全部写回一致。准备后disk、dirty、加载／源代次、当前数据指针、普通／共通源、Active／LastActive列表及旧备份均不变。负／越界Tab返回空；临时把共通数据指针置空后检查返回旧备份并立即恢复，仅属组件分支，不冒充真实GUI缺失指针或脱离流程。脱离／空表实际GUI结果仍仅按SAVE-76原范围保留，完整保存接入需按相关风险补验。

实际EXE为out/mcp_save_prepare_w11_20261026/{qt6,qt5}-runtime/QKeyMapperPrepared80_*_runtime.exe，原requireAdministrator manifest保持、前后同路径，旧EXE在old-executables。全部经普通控制器25608的公共定义审核／登记／perform启动，父8192、产品High12288、same_login=true；主线程实际QkmPrepare80_<UUID>私有桌面，PID／创建时间及退出见case的start／closed.json。无Default、系统输入、映射启动；原设备初始化／退出清理仍执行，不以零计数证明完整设备释放。

原始build-qt*-before／after.log、沙箱失败日志、run-before／after.log、前／后case、六份baseline、部署身份／旧EXE、局部patch／projection-review／预期字节及evidence-summary.json都在本机同一out目录，不随Git。cache-after.json确认25608仍active／cache_available=true、零owned／unresolved、current_action=null、last_error=null，截止15:44不变；SAVE-79旧控制器的未知启动不因此改记成功或已清理。

本片为独立本地提交切点，不推送，继续快捷键焦点提交、通知模式、顺序、其他草稿和完整Qt磁盘／业务修订，再进C1-C。C1-B2／M1未整体完成，工具12／业务能力8。未新增普通产品全构建、严格静态分析、ASan、Default／真实输入／设备、性能或三语言布局验收；PC-W10历史只按原版本范围保留。

## SAVE-81：快捷键只读输入分析与GUI复用（2026-10-26，PC-W11）

基线为SAVE-80本地d461d783。生产仅将onHotKeyLineEditEditingFinished原空白清理、美元前缀分离和validateCombinationKey调用提为私有静态analyzeHotKeyLineEditText，返回规范化字符串及原ValidationResult；原slot复用结果。三组后台字符串更新、成功／失败popup、setText和dirty逻辑，以及Save空值默认化、clearFocus、写入和重载时序均不动。CPP局部20增／13删、header一项声明，唯一锚点、dry run、完整逆变换及实际字节校对通过；未改Worker、manifest、协议、依赖、翻译或缓存核心。

[Qt6.8.3源码](https://github.com/qt/qtbase/blob/v6.8.3/src/widgets/widgets/qlineedit.cpp)和[Qt5.15.2源码](https://github.com/qt/qtbase/blob/v5.15.2/src/widgets/widgets/qlineedit.cpp)均由内部edited标志决定离焦完成，isModified和hasFocus不能完整代替。这里只共享输入分析，没有新增猜测Qt私有状态的收集器，也未依赖Qt私有头文件。后续完整保存准备仍须捕获待提交编辑与其他草稿，按既定editor_busy等明确拒绝不安全状态；未把局部输入分析当成完整保存许可。

环境沿同机SAVE-74的锁依赖、Qt6.8.3／Qt5.15.2官方x64与MSVC19.44；全部构建和普通控制脚本为Medium、沙箱外。最初Qt5构建的工具自动审批审查超时、进程未创建，按允许对原命令重试一次后成功，非UAC取消。试用测试前置QApplication::setActiveWindow时Qt6出现C4996弃用警告；保留build-qt*-before-deprecated-activation.log，该中间版本未启动产品且EXE未单独归档。改为项目已有QWidget::activateWindow后，有效前／后四次普通Release维护构建native exit0，Qt6零警告，Qt5仅原qtmain例外；构建树仍为out/tests/sendinput_lifecycle/{qt6,qt5}-profile74-release。

夹具复用profile_save_fixture.ini.example，启动前仅替换源／保留组为Hotkey81Seed／Keep81。原GUI基线使用正常启动、Qt名称／快捷键编辑事件及真实Save，目标Map81Hotkey；不直接调用输入完成slot、加载或保存函数，不设置控件值代替基线。待编辑组先用activateWindow建立私有桌面Qt应用内激活并核实实际hasFocus；没有切换输入桌面或发送系统按键，不代替Default物理／前台焦点验收。

| variant | 独立原GUI期待 | 每Qt旧GUI保存／冷读 | 每Qt提取后保存／冷读 |
|---|---|---|---|
| default | 三项原默认，不模拟未提交文本 | 4＋4 | 8＋5 |
| window | 带空格美元前缀待编辑，Save离焦后$L-Ctrl+F17 | 4＋4 | 7＋5 |
| start | 带空格待编辑，Save后F18 | 4＋4 | 7＋5 |
| stop | 带空格待编辑，Save后F19 | 4＋4 | 7＋5 |
| invalid | stop无效键，原校验恢复原默认并继续原保存 | 4＋4 | 7＋5 |
| committed | 合法window编辑后先离焦回名称，Save前已规范化 | 5＋4 | 8＋5 |
| empty | 仅人工清空三项后台字符串，检查原Save默认回退 | 4＋4 | 7＋5 |

每Qt旧57项／14进程、提取后86项／14进程；两Qt56个选定进程passed=true、native exit0、workAtExit／producersAtExit为0。14对完整262键INI字节相等，独立冷读不改盘。只读分析可重复、原错误和美元前缀不变；不改disk、dirty、加载／源代次、后台字符串、编辑器文本／焦点／modified／光标／选区，也不发editingFinished／textChanged。额外空白及带换行前缀为组件边界；empty后台空值也仅组件准备，不冒充正常GUI可达到路径。

初次无Qt应用内激活时Qt6 default保存／冷读通过，window保存exit2：未取得hasFocus，预期离焦提交未发生。保存失败case、实际INI及原EXE／identity在before-failed-focus、initial-focus-executables和run-before-focus-inactive.log，不计通过。只在测试前置改用现有activateWindow后补其余26个基线进程，生产未变；最初两个default进程的有效输入／分支未改变，按原版本复用，不重复运行。旧基线由原default记录及run-before-focus-ready.log组合，根run-before.log仍保留整体exit1，不改记成功。

提取后先完成Qt6十四项及Qt5十项。下一Qt5 committed保存的Start请求a52555646051469cadbd82abd54fa254缺少响应，原生gsudo日志为Setting token failed／Failed to substitute token，旧控制器25608以start_result_unknown关闭并保留该unresolved。原结果查询仍invalid_record；定点只读进程审计无本轮两项测试EXE存活，隔离INI仅CRLF与仓库LF不同，规范化后等于启动准备夹具，未发现结果JSON。保存cache-native-closed.json、unknown-start-action／result及unknown-start-qt5.ini；不能据这些当前观察证明请求从未启动或旧清理已经核实，此请求不计通过。

用户获告知后批准恢复UAC，请求34baa0cdb16949a886876d3beb146bef创建新Medium控制器32464、创建时间134374683000215513，截止15:44不延长。新空scope仅登记剩余四项，独立结果目录after/qt5-{committed,empty}-independent，保留原目录且不重放原请求。四个独立产品进程均exit0，run-after-recovered-qt5.log的普通脚本exit0；run-after.log仍保留原整体exit1。evidence-summary.json显式映射独立目录，核对有效前114／后172检查、56进程和14对INI，不混用历史未知结果。

实际原requireAdministrator EXE为out/mcp_save_hotkey_w11_20261026/{qt6,qt5}-runtime/QKeyMapperHotkey81_*_runtime.exe，前后同路径，原始及有效旧EXE、identity与定义分别保留。经普通控制器25608／恢复后的32464公共登记，父8192、产品High12288、same_login=true；主线程实际独立QkmHotkey81_<UUID>私有桌面，PID／创建时间／退出在各case的start／closed.json。原设备初始化／退出清理仍执行，但无映射启动、系统输入或Default，不以零计数证明完整停止或设备释放。

所有原始构建、前／后case、14份baseline、失败／未知材料、部署／旧EXE、patch／analysis-review／预期字节和汇总均在同一本机out目录，不随Git。cache-after.json确认新32464 active／cache_available=true、零owned／unresolved、current_action=null、last_error=null；不消除SAVE-79／81旧控制器的历史未知。缓存原生可靠性故障已记录，未扩大到缓存核心或系统配置修改。

本片为独立本地提交切点，不推送，继续完整Qt保存范围、通知模式、顺序、全部辅助集合、其他草稿及磁盘／业务修订，再进C1-C。工具12／业务能力8，C1-B2／M1未整体完成。未新增普通产品全构建、严格静态分析、ASan、Default物理焦点／真实输入／设备、性能或三语言布局覆盖；PC-W10历史仅按原版本范围保留。

## SAVE-82：完整Qt载荷组合与顺序意图（2026-10-26，PC-W11）

基线为SAVE-81本地f612bafe。生产只增加私有SettingSaveValues／collectSettingSaveValues：复用C1-A、TABLE及SAVE-74至81的Qt类型生成，组合全局、全部Tab／映射、配置参数、条件窗口匹配、实际共通源、通知两项模式及条件样式、输入、快捷键默认值、面板、规则／普通与通用宏／同步说明。新配置另声明原appendSettingSelectOrderIfMissing动作，保留其依赖实际已写目标组的规则，不写临时INI模拟结果、不重写分组／大小写／排序算法。GUI Save、辅助包装器、所有写盘和焦点／规范化／重载／失败流程完全未变；尚未消费新载荷或新增MCP工具。

非法目标返回InvalidTarget；快捷键编辑文本与当前后台字符串不一致时保守返回HotKeyInputNotSettled，载荷和顺序动作均为空，不靠Qt私有edited、hasFocus或isModified猜测提交。这只是内部只读生成约束，其他草稿／MCP editor_busy守卫和执行许可仍待后续。CPP50行及header7行局部增加，唯一锚点、dry run／numstat、完整逆变换及应用后实际字节一致；未改Worker、manifest、协议、依赖、翻译或缓存核心。

普通QtCore-only准备器复用SAVE-76模式，以Qt原生QSettings编码合成规则、宏和说明到out；没有启动产品、提权或安装依赖。成品full_save_fixture.ini.example保留普通A→B、共通F13／F14、旧F20备份、其他Keep82配置、Existing82覆盖目标旧路径、非默认通知14样式／模式和待规范化顺序。规则／普通宏／通用宏各一项，说明仅17非空、21空项加载时过滤；用单个非空说明避免无关QHash随机遍历顺序干扰，不设置全局或测试QT_HASH_SEED。

同机锁环境、Qt6.8.3／Qt5.15.2官方x64及MSVC19.44沿SAVE-74；普通控制、准备、构建均Medium。双Qt前／后四次Release维护构建native exit0，Qt6零警告，Qt5仅原qtmain例外；构建树继续out/tests/sendinput_lifecycle/{qt6,qt5}-profile74-release，无本片编译／运行失败。

| variant | 原GUI与组合独立期待 | 每Qt旧GUI保存／冷读 | 每Qt组合后保存／冷读 | 完整INI键 |
|---|---|---|---|---|
| current | 保存当前组，保留原含重复／失效项的顺序值 | 6＋6 | 12＋7 | 275 |
| new | 另存A82Saved，原顺序操作过滤旧项并补全组 | 6＋6 | 12＋7 | 314 |
| overwrite | 真实Yes覆盖Existing82，保留其旧FilePath | 7＋7 | 13＋8 | 310 |
| global | 启动前替换源节为规范全局组；跳过窗口匹配，顺序值不写 | 6＋6 | 12＋7 | 266 |
| pending | 私有Qt激活后真实未提交F17，组合拒绝且不动；GUI仍按原Save完成离焦提交 | 7＋6 | 11＋7 | 314 |

每Qt旧63项／10进程、组合后96项／10进程；两Qt40个选定进程passed=true、native exit0、workAtExit／producersAtExit为0，十对完整INI字节相等。每项有效组合值均由原GUI实际保存，全部实际新增／变化／删除键均属于组合或明确的顺序操作；不以单行或只比较数量替代完整值。pending只证明内部生成拒绝／不变性与GUI原行为保持，不冒充该状态的完整MCP等价保存。独立冷进程恢复全部集合作用范围和通知值，全程不改INI。

组合重复键顺序和Qt类型一致；非法目标不产生部分载荷。disk、dirty、样式初始化、加载／源代次、当前指针、原映射载荷、旧共通备份、规则／普通宏／通用宏／说明、文本及焦点不变。新配置实际顺序为Keep82、Payload82Seed、A82Saved、Existing82，过滤Missing82与重复项；当前／覆盖／全局按原分支不重写旧顺序。Keep82未改，覆盖旧FilePath保持C:/QkmPayload82/keep.exe；共通实际两行取代旧备份，附带通知模式2及非默认字号27等值。

所有源夹具在启动前准备并由正常产品启动加载；名称、Save、覆盖Yes及pending编辑通过Qt事件，不直接调用加载／保存／同步函数，不设置控件值代替基线。pending复用既有activateWindow并核实真实hasFocus，仅自有私有桌面Qt应用内焦点，不切换Default或发送系统输入。原requireAdministrator EXE为out/mcp_save_payload_w11_20261026/{qt6,qt5}-runtime/QKeyMapperPayload82_*_runtime.exe，前后同路径，旧EXE在old-executables。主线程实际独立QkmPayload82_<UUID>，父Medium8192、产品High12288、same_login=true，PID／创建时间／退出在case的start／closed.json。

本片均复用普通控制器32464的公共审核登记／perform，定义只替换同一空闲目标的旧摘要并保留其他当前范围。cache-after.json确认active／cache_available=true、零owned／unresolved、current_action=null、last_error=null，15:44截止不变；SAVE-79／81旧控制器未知请求不因此改记成功或清理已核实。原main设备初始化／退出清理保持，但本片未启动映射、未发系统输入，零计数不证明完整停止或设备效果。

本机out同一目录保留QtCore准备器源码／构建及生成前后INI、产品前／后四份构建、两份成功运行日志、十份baseline、各case／身份、旧EXE、聚合patch／review／预期字节及evidence-summary.json，不随Git。维护夹具及中文步骤随源码；PC-W10原证据仅按原版本范围保留。

本片为独立本地提交切点，不推送。组合仅覆盖已盘点Qt写入值和原顺序意图，仍须完整磁盘／业务修订、其他编辑窗草稿、目标／会话／权限／状态／到期及实际执行重检，之后C1-C处理真实保存、重载／备份和失败恢复。工具12／业务能力8，C1-B2／M1未整体完成；未新增普通产品全构建、严格静态分析、ASan、VIGEM关闭配置、Default／真实输入／设备、性能或三语言布局覆盖。

## SAVE-83：编辑窗拒绝与显式依赖候选指纹（2026-10-26，PC-W11）

基于本地8f65976d。hasOpenMappingEditor原样提取B1已有表达式，保留mappingAdditionAllowed短路、拒绝位置及错误。保存组合另拒绝活动模态和已列配置编辑窗，返回EditorBusy／空载荷／无顺序动作，不自动关闭或提交草稿。GUI Save及调用者、运行同步、Worker、manifest、协议、依赖和缓存核心不改。

私有指纹以Qt_5_15 QDataStream框定Qt QVariant类型、有序键值、目标名／全局／新建／覆盖决定、原顺序意图、显式配置路径／存在／完整字节SHA256及源修订，沿既有4MiB上限。非法／忙碌／空载荷、无路径／源、缺失却有字节或超限均不给指纹。不采集文件、不证明来源或授权，不是完整saveRevision，不承诺跨Qt指纹相同；真实实例／配对／权限和执行前复核仍待接线。

旧Qt6用settings_save_dependencies正常启动加载full_save_fixture.ini.example，经真实mappingAdvancedSettingButton Qt事件打开高级映射窗：五项中仅open-editor save preparation is rejected without a partial payload失败，native exit2，保留为缺口复现。最终双Qt各21项通过、native exit0：打开拒绝，Qt Escape正常关闭后恢复；改变数值相同的Qt类型、值、顺序动作、目标四字段、路径、全文换行、存在或源修订均区分，非法及超限拒绝。源修订合成64字符，仅组件检查；其余编辑窗仅静态核对。INI全64键原始字节、dirty、加载／源代次不变；未保存、未开始映射、未发系统输入。

同机既有锁环境、Qt6.8.3／Qt5.15.2官方x64和MSVC19.44。旧Qt6及最终双Qt普通Release维护构建exit0，Qt6零警告、Qt5仅原qtmain例外。新测试最初因OrderedMap显式构造的聚合初始化及缺少上限头文件，两Qt编译exit2；只修测试，保留build-qt*-after-test-compile-failed.log后重建。一次Qt5构建自动审批超时、未创建进程，重试一次成功；工具沙箱HRESULT 0x80070020期间使用沙箱外普通Medium，未提升开发工具。

EXE在out/mcp_save_dependencies_w11_20261026/{qt6,qt5}-runtime/QKeyMapperDependencies83_*_runtime.exe，原requireAdministrator，旧Qt6 EXE保留。公共审核／登记／perform启动：父Medium8192、产品High12288、same_login=true，实际QkmDependencies83_<UUID>私有桌面；PID／创建时间／退出在各case start／closed.json。零workAtExit／producersAtExit不证明完整清理或设备效果；无需Default。

用户把现控制器32464截止由15:44延至18:44，status核实同PID、generation2、Medium、active／cache_available=true且零拥有／未决；时间调整不扩大测试范围，SAVE-79／81历史未知启动不因此解决。本机原目录保留三份原始用例（1复现＋2通过）、构建、原INI／旧EXE、精确生产补丁／完整逆变换／实际预期字节及evidence-summary.json，不随Git。

独立本地提交切点，不推送；继续一致磁盘／真实上下文、其他草稿、权限／状态／到期／提交复核和C1-C分段保存／重载／备份与失败恢复。工具12／能力8，C1-B2／M1未完成。不新增普通产品全构建、严格静态分析、ASan、VIGEM关闭、Default／输入／设备、性能或三语言布局；PC-W10历史按原范围保留。

## SAVE-85：私有磁盘副本上的完整候选准备（2026-10-26，PC-W11）

基于b56d736c，产品仍沿SAVE-83。私有SettingSavePreparation／prepareSettingSave仅新增CPP52行、header14行：固定CONFIG_FILENAME绝对路径，区分存在与缺失，拒绝非文件／读取失败及超过既有4MiB界限的数据；显式安全模板的QTemporaryFile仅放磁盘副本，沿Qt codec和allKeys校验格式，目标分析／条件旧值／全部Qt载荷均从同一副本取得。返回前再读原路径，存在或全字节不同拒绝；有效时复用SAVE-83指纹。失败清空载荷、顺序意图和修订，不提供部分候选。完整逆变换及实际预期字节通过；原GUI保存、B1、Worker、manifest、协议、依赖和缓存核心未改。

settings_save_snapshot正常启动加载full_save_fixture.ini.example，双Qt各17项通过，两个最终进程passed=true／native exit0。正常、新建、覆盖、非法目标、无源输入、完整文件换行变化、格式错误（先由同路径读者读取）、超限、缺失及路径为目录均有定向期待；待写缓存临时加入Pending85组仍被私有磁盘副本判新组，旧目标保留键、缓存值和真实字节不被准备冲刷。真实高级映射窗经Qt按钮打开时准备拒绝，正常Escape关闭后恢复同一修订。源输入为合成64字符，仅组件覆盖，不是正式MCP身份或授权证明。

测试的外部行为仅在自有out INI用QSaveFile放置负向夹具，临时待写缓存值由测试恢复并sync；不是GUI保存基线或MCP业务保存。每项准备不写真实INI，最终64键原始字节恢复，dirty、加载／源代次和实际载入组不变。没有通过控件赋值或直接加载／保存函数代替GUI；该内部新接口无需重跑原GUI Save，SAVE-82实际保存／冷读证据只按原版本范围复用，不据此宣布完整业务等价。

首轮Qt6 native exit2，仅“overwrite dependency uses the actual saved target path”断言失败，其余16项通过；错误是期待旧目标路径出现在写入载荷，原语义实际通过省略键保留。只改测试为检查磁盘旧值存在且载荷不写此键，生产实现不变。first-test-expectation-failed、failed-qt6.ini、首轮EXE／身份和构建日志保留；Qt5首轮已部署但未运行，不计失败或通过。修正后双Qt重新增量编译并各跑一个完整组件用例通过，最终两次脚本正常退出。

既有同机锁环境、Qt6.8.3／Qt5.15.2官方x64和MSVC19.44；首轮及修测试后四份普通Release维护构建exit0，Qt6零警告、Qt5仅原qtmain例外。第二轮只用已配置树jom，不重复qmake／环境恢复。EXE为out/mcp_save_snapshot_w11_20261026/{qt6,qt5}-runtime/QKeyMapperSnapshot85_*_runtime.exe，原requireAdministrator、同路径；公共审核／登记／perform经新Medium控制器10520，父8192、产品High12288、same_login=true，实际QkmSnapshot85_<UUID>私有桌面。PID／创建时间／退出在case start／closed.json，零尾部任务不代表完整停止或设备效果；无Default或系统输入。

cache-after.json核实10520仍active／cache_available=true，零owned／unresolved／current_action，18:50截止不变；首轮正常失败退出的目标已核实退出，旧控制器23316／25608的未知请求不因此解决。本机out/mcp_save_snapshot_w11_20261026含三份原运行结果、四份构建、旧EXE、INI、精确补丁／逆变换／预期字节和evidence-summary.json，均不随Git。

本片为独立本地提交切点，不推送。调用过程中恰好并发写盘的DiskChanged分支仅静态核对，未测真实锁定拒绝、文件身份替换或跨进程事务；重读比较不承诺原子执行。4MiB为内部有界候选限制，公开前须说明并按实际规模校准。真实实例／配对／权限／状态／到期及源修订、其他草稿范围与实际提交、保存／重载／备份分段和失败恢复仍待接续。工具12／能力8、C1-B2／M1未完成；未新增普通产品全构建、严格静态分析、ASan、VIGEM关闭、Default／输入／设备、性能或三语言布局，PC-W10按原范围保留。

## SAVE-86：保存、重载与备份的内部结果（2026-10-26，PC-W11）

基于edd55101。保留bool saveKeyMapSetting接口及所有旧调用者，薄包装仍返回reloaded，与原saveSucceeded定义相同。原函数体移至私有saveKeyMapSettingWithResult，仅新增SettingSaveResult：Failure在原拒绝／取消／配置不可用／sync失败／重载失败返回点赋值；接受目标名、diskSaved、reloaded、backupAttempted、backupSucceeded按原实际阶段记录。diskSaved在sync/status成功后设置，重载失败仍保留true；备份失败保留原GUI成功及bool true，并单独记录false。未移动任何原写入／缓存回滚／重载／界面恢复／popup动作；完整逆变换、唯一锚点及实际预期字节通过。未改Worker、manifest、协议、依赖或缓存核心。

新增settings_result_save／settings_result_cold，正常加载full_save_fixture.ini.example。提取前先编译真实原GUI版本，通过Qt名称输入、原Save按钮和实际覆盖No建立独立基线；结果版使用同样Qt输入及实际No，再由测试调用同一原函数体的新结果入口并断言阶段，不用该内部调用替代旧GUI基线。两版前后同EXE路径；冷读是独立正常启动的进程，不直接调用加载函数或设置控件值。

| variant | 期待实际行为与结果 | 完整INI键 |
|---|---|---|
| current／new | 同步、重载及latest备份成功，全部true | 275／314 |
| no／invalid | 实际No取消／非法名称拒绝，三个阶段未执行 | 64／64 |
| format | 正常加载后仅自有INI改为坏格式，原新鲜校验拒绝且原坏字节保留 | 1 |
| write_current／write_new | 仅自有INI用CreateFileW的FILE_SHARE_READ阻止写入／替换，sync失败；三个阶段false | 64／64 |
| reload | 仅组件人工将首行原键置为INVALID_RELOAD86；实际保存后原加载校验失败，diskSaved=true、reloaded=false、未备份 | 314 |
| backup | 仅在自有latest路径创建空目录；实际写盘及重载成功，backupAttempted=true、backupSucceeded=false，原bool语义不变 | 314 |

每Qt旧GUI67项／14进程、结果版85项／14进程，共56个选定进程passed=true／native exit0；18对完整INI字节一致，current／new／write_current／write_new／backup各有独立冷读并且不改盘。两个写锁分支在返回后关闭自有句柄，再用同路径新读者sync核对全部旧键值，退出及冷启动也没有意外补写失败目标。正常保存的latest完整字节等于INI；备份负向目录在finally范围移除。人工无效源是结果阶段故障注入，普通GUI编辑器不接受，不冒充正式MCP该故障路径或普通可到达输入。StateRejected只保留原STOPPING早退并静态核对，本片未启动映射或实际停止。

结果版九组阶段／接受目标均单独核对；reload失败Failure为ReloadFailed且diskSaved仍true，backup失败主Failure保持None但备份字段false，不能把这些组合压成saved=false或推断所有内存准备均未发生。结果方法仍采集GUI目标及执行原CategoryFilterStateGuard／运行同步等准备，不是显式MCP目标执行入口或身份／权限许可。下一层仍须真实完整源修订、实例／配对／权限／状态／到期、目标与其他草稿范围、执行重检和去重／结果查询。

既有同机锁环境、Qt6.8.3／Qt5.15.2官方x64及MSVC19.44；前／后四份普通Release维护增量构建native exit0，Qt6零警告、Qt5仅原qtmain例外；本片无意外编译或产品失败，九组负向是预期。EXE为out/mcp_save_result_w11_20261026/{qt6,qt5}-runtime/QKeyMapperSaveResult86_*_runtime.exe，原requireAdministrator、前后同路径，old-executables保留旧EXE／身份。普通Medium控制器10520经公共审核／登记／perform启动，父8192、产品High12288、same_login=true，实际QkmSaveResult86_<UUID>私有桌面；PID／创建时间／退出在各case start／closed.json。零workAtExit／producersAtExit不是完整清理或设备证明；无Default、系统输入或映射启动。

cache-after.json核实10520仍active／cache_available=true、零owned／unresolved／current_action，用户最新18:30截止未改；旧23316／25608未知请求不因此解决。本机out/mcp_save_result_w11_20261026保留18份基线、56份原case与身份、四份构建、旧EXE、局部patch／逆变换／预期字节、运行日志及evidence-summary.json，均不随Git。PC-W10结果按历史范围保留，不冒充本机复验。

独立本地提交切点，不推送。未开放MCP保存工具，12工具／8能力不变，C1-B2／M1未整体完成；未新增普通产品全构建、严格静态分析、ASan、VIGEM关闭、Default／真实输入／设备、性能或三语言布局。SAVE-85的调用内并发磁盘变化／文件身份／跨进程事务及真实业务授权缺口仍保留，选定写锁回滚不能代替所有失败和并发场景。

## SAVE-87：真实请求与完整源绑定的私有候选（2026-10-27，PC-W11）

基于45dac487，同机明确恢复，沿用既有锁环境／构建与有效证据，不重做环境恢复。新增私有prepareAutomationSettingSave，核对实际在途请求的Read＋Configure及server.isRequestCurrent；源沿现有完整快照／实例／pair／权限修订算法取得，另将当前Tab与运行修订纳入上下文哈希，再复用SAVE-85磁盘候选。显式请求目标不采集名称控件；全局源只接受明确全局目标，不静默忽略另一个名称。返回前重核权限、既有安全读取许可、运行修订及既有五秒验证预算；失败清空可用载荷、顺序意图、修订和上下文。

完整载荷中的映射／共通读取改在已有SourceReadAccess锁内复制／编码，保持OrderedMap插入位置与Qt类型，磁盘／图标存在检查在锁外；没有改Worker或新建同步机制。原saveKeyMapSettingWithResult函数体与SAVE-86逐字相同，生产补丁可完整逆向检查；未改GUI Save、B1、manifest、协议、依赖或缓存核心。候选不是执行许可，尚未开放保存工具。

维护case settings_save_context在自有out创建独立私有配对目录，只对实际普通桥接的PID／创建时间／映像三项匹配批准固定Read或Read＋Configure。普通桥接完成真实认证，固定qkm_get_state由维护handler验证内部候选并仅返回fixture标记；这是组件接线，不是生产MCP保存或完整SDK业务验收。双Qt各Configure20项／Read4项、受影响settings_save_snapshot17项，六进程均passed=true／native exit0，共82项。源／实例／授权上下文来自实际产品及请求；显式当前／新建／覆盖、非法及全局目标拒绝、五种陈旧请求与回复完成后失效通过。

夹具仅人工改变后恢复一个Pure_OriginalKeys派生字段，证明未序列化到INI的完整源内容仍影响候选；不是普通GUI编辑输入。实际Qt按钮打开高级映射窗后拒绝，正常Escape关闭后恢复同一修订；最后调用已有运行观察器模拟新活动，Worker采样仍STOPPED时也拒绝，不把它当作实际启动／停止或清理验收。候选调用不改变原INI完整字节、dirty、加载／源代次、实际载入组或GUI目标文本；SAVE-85负向磁盘夹具自行恢复，受影响回归通过。未变GUI保存结果只按SAVE-86原版本／范围复用，无需重复其56个进程。

同机Qt6.8.3／Qt5.15.2官方x64及MSVC19.44；首轮双Qt新增值成员缺QJsonObject完整包含而编译exit2，日志build-qt*-first-missing-include.log保留，补包含后最终两份普通Release维护增量构建native exit0，Qt6零警告、Qt5仅原qtmain例外。EXE为out/mcp_save_context_w11_20261027/{qt6,qt5}-runtime/QKeyMapperSaveContext87_*_runtime.exe，原requireAdministrator；公共登记／perform由普通Medium控制器24956启动，父8192、目标High12288、same_login=true，实际QkmSaveContext87_<UUID>私有桌面。PID／创建时间／native退出见各case的start／closed.json；无Default、映射或系统输入。

用户新授权至今日18:30，技能创建24956、instance4f384b60c3ca43e1a9c2bfd768020809／generation1，最终status仍active／cache_available=true且零owned／unresolved／pending／current_action／current_job。历史23316／25608未知请求保留、不重放或接管。原始六份case、认证fixture返回、构建／部署、EXE、最终补丁／逆向检查及evidence-summary.json均在本机out/mcp_save_context_w11_20261027，不随Git。

独立本地提交切点，不推送。当前全局源动态分支、调用中并发变化／权限丢失、五秒预算超时、其他草稿范围／确认、实际显式提交／到期／去重及操作查询仍待接续；SAVE-85文件身份／跨进程事务缺口保留。工具12／能力8，C1-B2／M1未完成。未增加普通产品全构建、ASan、严格静态分析、VIGEM关闭、Default／设备／性能或三语言布局；PC-W10证据保持历史范围。

## SAVE-88：完整候选写入范围与草稿确认声明（2026-10-27，PC-W11）

基于2957ef49，仅在私有AutomationSettingSavePreparation增加scope，按实际OrderedMap载荷分为目标相对键及完整共享键，保留源／目标配置种类、新建／覆盖确认意图、实际SettingSelect值及原appendSettingSelectOrderIfMissing动作／键。排序操作有原规范化语义，不模拟临时INI写入或预测最后值；它与直接值载荷分别声明。范围由已绑定的完整源、目标、载荷和顺序意图确定，不接受调用方自报范围。

明确mode=full_gui_save、key_semantics=write_footprint_not_diff、draft_policy=all_loaded_configuration_and_shared_drafts、change_attribution=unavailable、confirmation_required=true；列全范围不等于已识别每个草稿的来源，也不表示无其他草稿。dirty=false不能免除范围确认。scope在原候选失败路径清空；实际预览同意／覆盖同意和提交前复核须由后续执行层强制，当前没有保存工具或自动写盘。GUI保存体仍与SAVE-87／86逐字相同，未改Worker、权限、manifest、协议、依赖或缓存核心。

沿settings_save_context真实认证维护夹具，双Qt各configure30项／read4项／global28项，六进程均passed=true／native exit0，共124项。global只在本用例INI将Payload82Seed换为QKeyMapperGlobalSetting后正常启动，不直接调用加载函数，验证接受明确全局目标、拒绝新建／覆盖另名；不是普通GUI切换基线。目标／共享键排序后与全部候选键精确相等，包含所有Tab、共通备份、普通宏／同步说明和共享全局／规则／通用宏／启动选择；当前目标无排序，新建排序与覆盖确认分别核对。夹具临时清dirty并恢复，准备仍要求全范围确认；原INI字节、dirty、加载／源代次、载入组及名称文本不变，源派生／真实编辑窗／新运行修订拒绝沿受影响认证用例重验。SAVE-85快照及未变GUI保存按各自原版本范围复用，没有重复其已过验收。

同机既有锁环境、双Qt官方x64／MSVC19.44；两份普通Release维护增量构建native exit0，Qt6零警告、Qt5仅原qtmain例外，本片无编译或原生用例失败。EXE在out/mcp_save_scope_w11_20261027/{qt6,qt5}-runtime/QKeyMapperSaveScope88_*_runtime.exe，原requireAdministrator，经同一Medium24956和公共定义登记／perform启动；父8192、产品12288、same_login=true，实际QkmSaveScope88_<UUID>私有桌面，退出／创建身份见各start／closed.json，无Default、映射或系统输入。

最终缓存仍active／cache_available=true，18:30截止不变、零owned／unresolved／pending／current_action；原始case、真实认证fixture响应、构建／部署／EXE、完整补丁／逆向检查和evidence-summary.json保留在本机out/mcp_save_scope_w11_20261027，不随Git。运行器最后一行继承了snapshot regression文字标签，实际六个原始case为configure／read／global，未执行snapshot；原日志不改写，摘要按实际记录计数，后续本地辅助脚本的末行已改正。

独立本地提交切点，不推送。这是范围描述及确认要求，不是正式预览／同意或执行守卫的验收；不宣称精确字段变化归属、真实映射／停止、并发变化／权限丢失／预算超时、跨进程磁盘事务、到期／去重／结果查询或完整MCP闭环通过。工具12／能力8，C1-B2／M1未完成；未增加普通产品全构建、ASan、严格静态分析、VIGEM关闭、Default／设备／性能或三语言布局，PC-W10保留历史范围。

## SAVE-89：保存收尾阶段的原样共用（2026-10-27，PC-W11）

基于205a407f，把原saveKeyMapSettingWithResult末段116行逐字提为私有finalizeSettingSave；原调用显式传原QSettings引用、valuesBeforeSave／keysBeforeSave、已接受SettingSaveResult、Tab索引和popup选项。savedSettingName取已通过原名称校验的result.settingName，与原去除斜杠后的名称一致；所有写入前阶段、覆盖确认、原写入顺序及CategoryFilterStateGuard／QSettings析构顺序保持。changed／removed键计算、sync/status、只回滚本保存pending、重载、滚动／共通页恢复、dirty、popup与latest备份代码不改。唯一补丁、完整逆变换和实际预期字节通过；CPP净增9行、header2行，无Worker、manifest、协议、依赖、缓存或测试代码改动。

选择既有settings_result_save／cold的current／new／write_current／write_new／reload／backup六组，冷读保留原五组。复用SAVE-86 before独立旧GUI原生结果及完整基线，逐项核对case passed／native exit0、归档EXE与原identity摘要；本片改动前GUI保存体和SAVE-86逐字相同。原始GUI操作来自当时真实Qt名称输入／Save，不由当前内部结果调用自造；旧EXE在out/mcp_save_result_w11_20261026/old-executables，新EXE路径另记，不声称前后同路径。来源及本机副本在reused-gui-baselines.json及12份baseline-*.ini，未改动原SAVE-86材料。

双Qt各64项、22个当前原生进程全部passed=true／native exit0；12对完整INI字节相等，各Qt依次275／314／64／64／314／314键。独立冷读保持原始字节；两种写锁失败后解除句柄、另一个同路径读者sync及冷读都无失败pending补写。人工非法源仍明确已写盘／重载失败，备份目录负向仍明确已写且重载／备份失败，latest目录按原finally移除。六组的结果阶段、目标与备份按原独立期待核对，不把内部调用当作新的GUI基线、正式MCP保存或真实映射故障。

既有同机锁环境、Qt6.8.3／Qt5.15.2官方x64及MSVC19.44；双Qt普通Release维护增量构建native exit0，Qt6零警告、Qt5仅原qtmain例外，本片无编译或原生用例失败。EXE为out/mcp_save_finish_w11_20261027/{qt6,qt5}-runtime/QKeyMapperSaveFinish89_*_runtime.exe，原requireAdministrator；经同一Medium24956／公共定义登记及perform，父8192、目标12288、same_login=true，实际QkmSaveFinish89_<UUID>私有桌面。各PID／创建时间／退出见原start／closed.json；无需Default，无映射或系统输入。

最终status仍active／cache_available=true、18:30截止不变，零owned／unresolved／pending／current_action／current_job；历史未知请求不因此解决。22份原case、12份旧基线副本／来源、构建／部署／EXE、原尾段、完整补丁／逆变换／预期字节和evidence-summary.json在本机out/mcp_save_finish_w11_20261027，不随Git。

独立本地提交切点，不推送。本片只共用既有GUI收尾，不提供显式候选写入、配对／范围确认或磁盘与生命周期事务；普通取消／非法／坏格式早退未改，按SAVE-86原范围复用；共通Tab恢复及全局名称显示尾部分支逐字保留、未作本片动态通过声明。实际输入／停止／设备、并发／权限丢失、共享缓存任意pending、超时／到期／去重及正式结果查询仍待后续。工具12／能力8、C1-B2／M1未完成；未增加普通产品全构建、ASan、严格静态分析、VIGEM关闭、Default／性能或三语言布局，PC-W10保持历史范围。

## SAVE-90：候选缓存持久化内容预检（2026-10-27，PC-W11）

基于77a55753，私有compareSettingSaveCache接入认证候选准备；从同一磁盘字节副本读取Qt原值，并将磁盘／缓存已有值分别经当前Qt原生INI编码，比较完整持久化表示，拒绝CacheContentChanged。不是QVariant严格类型比较，也不向临时文件写入候选、替换CONFIG_FILENAME／cwd或调用GUI保存模拟预览。投影沿既有4MiB上限，失败清空可用候选及范围；原GUI保存／共用收尾、Worker、协议、manifest与gsudo核心不变。只检查当前快照与缓存内容，不证明没有内部pending标志或持有执行事务。

首轮双Qt维护构建成功，但仅首个Qt6 configure运行即native exit2：正常候选SnapshotFailed，38项中发生连带失败，其余七个预定场景未执行；Qt5当时只有构建／部署，不计运行通过。单个Qt6错误观测版再现candidate_error=3／save_preparation_failed；最小公共API探针QTemporaryFile打开true、QSettings状态1（AccessError）、原file.error=0且文件0字节。未取得底层Windows错误码，不根据错误文字推算。初始和观测进程最终原INI／dirty／源代次不变的断言通过，但不能据此把待写拒绝／正常候选算通过。

修正为QTemporaryDir安全模板下的新values.ini后，首个Qt6正常候选恢复，但五项待写断言仍失败。再用单例阶段探针记录：before_constructor=true、after_constructor=false，原INI在新建同路径QSettings时即变化，随后codec／allKeys／比较／析构均仍为false；这是实际文件字节观测，不依据报错文字归因。目录修正版及阶段探针各native exit2，共四份已知Qt6失败／诊断进程，不能算通过或未知启动。构造冲刷直接阶段观测仅Qt6；Qt5初期只构建／部署，无对应失败运行声明。

最终采用只读const QSettings观察器：在loadKeyMapSetting／loadGeneralSetting的既有原读者之后创建，正常时持续复用，错误时只在同一GUI读边界重建；候选不创建、sync或修改真实INI对象，缺失即context_busy。观察器只保留现有Qt共享缓存的观察入口，不另建配置模型、定时器或工作线程。私有compareSettingSaveCache只向独立临时目录编码已有磁盘／缓存值，类型差异按实际持久化表示判断；4MiB也约束投影编码。GUI Save函数体及共用收尾与77a55753逐字相同，加载入口只增加观察器初始化；对象寿命变化另选原保存回归。完整生产补丁、逆向检查及预期字节通过。

最终observer阶段双Qt各configure41项、read4项、global39项、configure_saved42项，八进程全部passed=true／native exit0，共252项。无关待写组、目标待写值及删除返回CacheContentChanged；等价原生bool被接受且修订不变，超限投影返回FileTooLarge；各步骤原INI字节和原待写对象不被准备冲刷／消耗。临时移走观察器时拒绝且不回退构造；私有坏格式观察器实际产生FormatError，在组件复现的GUI读取边界恢复，非真实GUI加载基线。正常关闭编辑窗、派生源绑定、新运行修订及全范围说明沿受影响用例重验。fixture自己恢复pending并sync／恢复原字节，与只读准备分开；configure_saved先由真实Save按钮完成业务保存，后续观察以其新磁盘和代次为基线，候选不改变它。

由于增加只读对象寿命，regression阶段另跑原current／new／write_current／write_new／reload／backup六组结果和五组冷读，双Qt各64项、22进程native exit0，共128项；12对275／314／64／64／314／314键的完整INI字节与已核实的SAVE-86独立旧GUI基线一致。原始GUI材料经SAVE-89来源索引复用，不把内部结果调用当作新GUI基线。两种写锁解除、后来读者及独立冷读没有失败缓存补写；重载失败与备份失败仍保留各阶段语义。最终两组共30个成功原生进程／380项，四份早期失败单列，不混计。

同机既有锁环境、Qt6.8.3／Qt5.15.2官方x64及MSVC19.44；最终双Qt普通Release维护增量构建native exit0，Qt6零警告、Qt5仅原qtmain例外，前期／探针构建分别保留。EXE为out/mcp_save_cache_w11_20261027/{qt6,qt5}-runtime/QKeyMapperSaveCache90_*_runtime.exe，原requireAdministrator；同一Medium24956和公共登记／perform，父8192、目标12288、same_login=true，实际QkmSaveCache90_<UUID>私有桌面，PID／创建时间／退出在原start／closed.json，无Default、映射或系统输入。最终status仍active／cache_available=true、18:30截止不变，零owned／unresolved／pending／current_action／current_job。

全部材料仅在本机out/mcp_save_cache_w11_20261027：after／probe／final／stages保留四份失败及诊断，observer／regression保留30份成功结果；first-failed-executables／probe-executable／directory-first-failed-executables／constructor-stage-probe保留相关EXE和identity，另有构建、部署、12份旧GUI基线副本／来源及evidence-summary.json。最终补丁及构造阶段探针源快照可追溯；directory-first-failed-source.patch的测试部分含后来追加的阶段观测，不作为该失败测试的逐字源码证明，生产CPP仍为当时目录方案。原始证据不随Git，历史未知请求保持原记录、不重放或接管。

独立本地提交切点，不推送。当前只有候选预检，未开放MCP保存；执行期仍须在任何真实INI构造前完成权限／状态／范围／目标／到期及缓存复核并协调生命周期，预检不是独占锁或跨进程事务。调用中竞态／预算超时、文件身份替换、显式提交／去重／结果查询、对象新增内存／加载性能及完整映射／停止均未验；错误观察器恢复只作组件覆盖，不冒充GUI成功→失败→恢复全链路。工具12／能力8、C1-B2／M1未完成。未加普通产品全构建、ASan、严格静态分析、VIGEM关闭、Default／设备／性能或三语言布局，PC-W10保留历史范围。

## SAVE-91：保存重载前的GUI名称依赖（2026-10-27，PC-W11）

基于49e303a7。原成功保存收尾会把settingNameLineEdit改为规范目标名／全局译名，重载失败则保留原清空行为；源快照和dirty不包含名称本身，不能识别dirty已经true时另一个名称草稿。只在私有保存context加入gui_save_name再计算原上下文哈希，scope另报before／on_success／changes_on_success。目标依旧取明确参数，不设置名称控件决定操作；名称效果不混入INI写入键列表，也不据此推断所有草稿归属。GUI保存体、只读观察器、Worker、协议、权限、manifest与依赖不变；CPP净增7行、删1行，精确预期字节及完整生产逆向检查通过。

旧产品Qt6单例经实际Qt Ctrl+A及文本事件输入等长Unicode的Draft91甲、Draft91乙，dirty固定true；两次源快照相同，45项中仅“名称变化使修订失效”和“范围描述将被替换的名称”两项失败，native exit2；其他43项通过。INI原始字节、加载／源代次、载入组及最后恢复的名称不变。原before case／native身份／退出与旧EXE／identity保留，不用直接赋值控件或调用加载函数替代独立GUI基线。此用例只证明候选依赖缺口，不是保存业务／GUI加载验收。

修正后八个实际认证维护进程全部native exit0：各Qt configure46项、read4项、global40项、configure_saved47项，合计274项。两种名称草稿仍共享相同源快照／dirty，候选修订应不同，显式目标仍为传入配置；恢复名称／dirty后恢复原候选。当前配置及全局实际译名报告正确，原只读权限／陈旧请求、缓存pending隔离、观察器缺失／错误恢复、编辑窗／新运行修订、完整范围等受影响检查通过；configure_saved仍由实际Save按钮建立本场景观察基线。红例之后补充了目标／全局译名检查，并将描述规范为changes_on_success，未声称红／绿测试源码逐字相同。

同机既有锁环境及官方Qt6.8.3／Qt5.15.2 x64／MSVC19.44。旧Qt6及最终双Qt普通Release维护增量构建native exit0，Qt6零警告、Qt5仅原qtmain例外。EXE在out/mcp_save_name_w11_20261027/{qt6,qt5}-runtime/QKeyMapperSaveName91_*_runtime.exe，原requireAdministrator；同一Medium24956及公共登记／perform，父8192、目标12288、same_login=true，实际QkmSaveName91_<UUID>私有桌面，原PID／创建时间／native退出可查，无Default、映射或系统输入。最后status仍active／cache_available=true，18:30截止不变、零owned／unresolved／pending／current_action／current_job。

本机out/mcp_save_name_w11_20261027保留before／after原case、旧EXE、身份、构建／部署、各补丁／逆向检查／预期字节及evidence-summary.json，均不随Git；前序证据不覆盖。原GUI保存／冷读、只读观察器寿命沿SAVE-90当前有效范围复用，不重复其22进程。此片为独立本地提交切点，不推送；没有新增MCP工具，12工具／8能力、C1-B2／M1仍未整体完成。实际显式写入、范围／覆盖确认、执行时生命周期／权限／到期、去重／结果查询及文件事务仍待后续；不把名称依赖补齐当作全部编辑草稿已归属或全部副作用已验。未加普通产品全构建、ASan、严格静态分析、VIGEM关闭、Default／设备／性能或三语言布局，PC-W10保留历史范围。

## SAVE-92：明确目标／完整准备载荷的私有持久化（2026-10-27，PC-W11）

基于8327d7bd，CPP仅新增51行savePreparedSettingWithResult及一条私有声明，原GUI保存体／共用尾部逐字不变。方法拒绝无效准备、非固定配置路径、不允许的当前编辑／停止状态、变化原始磁盘字节或缓存内容，再写全部准备载荷，执行原新组排序和finalizeSettingSave。默认热键／失焦、运行行同步、共通备份同步及CategoryFilterStateGuard保持既有语义；不会临时设置名称控件或调用加载函数伪造GUI输入。它是生产尚无调用方的持久化原语，不是自动化授权或并发提交许可；无Worker、协议、manifest或gsudo核心变更。大文件唯一锚点补丁、dry-run／numstat、精确预期字节及逆向检查通过。

before在产品8327d7bd未修改时，以实际Qt名称输入及Save按钮分别执行current／new／write_current／write_new／reload／backup；六组中除reload外均有独立冷读，双Qt各58项／11进程，共116项／22个native exit0。旧EXE及identity归档并核对实际SHA。after在相同EXE路径加载新鲜完整夹具，真实桥接认证的固定get_state维护handler只准备显式目标并调用私有方法，不输入目标名称；六组保存及五组冷读各82项，共164项／22个native exit0。两阶段共44进程／280项，formal_mcp=false，不冒充正式MCP保存。

after首轮三个产品进程（Qt6当前warm／cold、新目标warm）均native exit0，控制脚本在新目标完整字节比较时exit1，后续未派发。实际差异仅新组MacroList／SyncGroupNotes从组首移到组末，314个键及编码值相同；不修改产品或EXE，保留失败脚本和当时产物，改为全节／键／原始编码值严格对照，并单记raw_bytes_equal。比较器对改值、缺键和重复键的普通权限自检通过；只忽略排列、空行／注释及换行形式，不忽略任何键值或排序键。三个已完成进程未重跑，继续新目标冷读及剩余固定场景。最终12组275／314／64／64／314／314键全部编码值一致；双Qtnew／reload／backup六组保留两键排列差异，其他六组字节相同。Qt6 current的原字节断言在首次脚本推进到new前已完成，其当时完整文件未单独另存，不能补造原材料。

after核对实际结果中的diskSaved／reloaded／backupAttempted／backupSucceeded；两种写锁返回WriteFailed，原INI／加载代次不变，解锁后新QSettings读者及独立冷读均无失败pending补写。人工INVALID_RELOAD86仍为已写盘但重载失败、不备份，非普通GUI可达输入；备份目录负向仍为已写且重载成功／备份失败，临时目录及时移除。当前目标另检查无效准备、改向路径、组件STOPPING和变化磁盘字节在写入前拒绝；STOPPING只直接设置并恢复本夹具状态，未执行映射启停。写失败保留提交前GUI名称，重载失败沿既有共享尾部清空；成功后名称／dirty及冷读分别检查。

既有同机锁环境、官方Qt6.8.3／Qt5.15.2 x64、MSVC19.44；最终before／after双Qt普通Release维护增量native exit0，Qt6零警告、Qt5仅原qtmain例外。最初after构建期间补了定向测试断言，保留initial日志并在最终部署前重新增量编译测试文件，不混称最初EXE包含后来断言。EXE为out/mcp_save_prepared_w11_20261027/{qt6,qt5}-runtime/QKeyMapperSavePrepared92_*_runtime.exe，原requireAdministrator；普通Medium24956经公共登记／perform，父8192、目标12288、same_login=true，实际QkmSavePrepared92_<UUID>私有桌面。各PID／创建时间／native退出在原start／closed.json，无Default、系统输入或映射启动。

最后cache-after仍active／cache_available=true、18:30截止不变，零owned／unresolved／pending／current_action／current_job；23316／25608历史未知请求不因此解决。before／after原case、旧EXE／身份、INI基线／产物及比较、首次脚本／差异、构建部署、源码补丁／预期字节和evidence-summary.json均在本机out/mcp_save_prepared_w11_20261027，不随Git。原SAVE-86基线元数据另轻量核对18份shared_execution=false、save_result为空且passed，纠正早期ignored助手检查不存在execution字段的弱断言；原实际GUI路径及EXE来源未因此变化，不重跑旧验收。

独立本地提交切点，不推送。实际用户权限／完整范围及覆盖确认、到期／预览消费、源与草稿执行重检、生命周期写入栅栏、文件身份／跨进程事务、去重和结果查询仍待C1-C；私有方法的普通状态／缓存预检不能替代这些门槛。工具12／能力8、C1-B2／M1未整体完成。全局源、共通Tab恢复和缺省热键空输入的本片动态覆盖未增加；原字段等价证据沿原范围复用。未增普通产品全构建、ASan、严格静态分析、Default／真实输入／设备、性能或三语言布局；PC-W10保留原历史范围。
