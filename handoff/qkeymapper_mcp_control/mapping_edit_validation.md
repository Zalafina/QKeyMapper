# 简单源行键值编辑：选定证据

[唯一计划与进度](qkeymapper_mcp_automation_plan_codex.md) · [能力矩阵](capability_matrix.md) · [证据索引](validation.md)

本页仅记录事实和证据边界；阶段、决策和下一步仍以唯一主计划为准。原始材料只在PC-W11本机忽略的out，不随Git或跨PC复制。

## EDIT-143：版本与契约

2026-11-10，PC-W11，实际Windows 11、同一Windows登录。源码为a42709513e76d9fba8145803e8a357705655ba1c加本片差异；141独立Item GUI基线及142共用值提取保留原范围。当前普通Release与维护组件分别记录，不把两者或历史设备结果相加为完整验收。

既有mapping.update支持original_key／mapping_key／key_up_mapping_key及原send_method，至少指定一项，省略保持。新键仅大写字母／数字；键值更新的已有原键／按下／抬起须为单个字母、数字或F1–F24。抬起键仅原发送时机2／3／4可编辑，空字符串沿用候选按下键并清备注；显式非空变更使用既有备注对齐。原键互斥变更若按GUI规则会禁用本行／他行或要求确认，明确不支持且无修改。仅发送方式调用保持原范围；完整语法、冲突编辑及非当前／隐藏行编辑未开放。

候选复用GUI静态校验、142值助手及原备注对齐；提交沿既有源／权限／运行／清理／编辑窗守卫、预览到期和执行前复核。真实变化才推进源epoch与dirty；不隐式保存、停止或开始。结果仍归原实例／配对／权限，重放保留原业务结果、原操作采样时间及目标，只新增重放标记和查询时间。17工具／23业务能力不变，无新权限、Qt可见文案或TS/QM修改。

## 构建、组件与分析

- Qt6 6.8.3 x64／MSVC2022普通Release b01：scripts/build_qt6.ps1增量native0、零编译／链接警告。源out/build_qt6/release/QKeyMapper.exe；部署到out/mcp_product/edit143_20261110_r02/runtime-中文/QKeyMapperMcpAcceptance_edit143_qt6.exe，SHA256为f5f02c261cd34c022bd91400ec3da111554fdfdd60e2bacab313d2a11f2a8b9e。保留正式管理员manifest；六项随包厂商DLL逐项与当前仓库核对，其余Qt DLL／plugins只取同PC自有137运行副本，没有复制INI／凭据。
- Qt6／Qt5 5.15.2 x64控制组件各native0；实际私有桌面PID42732／34740，ibInputChangeArguments、mappingChangeArguments、mappingChangeLifecycle、mappingChangeLimits四个槽及init／cleanup各6pass，零fail／skip。参数类型／组合、原发送方式0–3、空抬起与省略区别、缺字段／未知字段、源格式及预览存储归属通过。Qt5控制台的Windows 10版本横幅不是本机操作系统证据；本轮实际均Win11。
- Python正式stdio schema／错误结果组件8pass，包含旧mapping.add／save／发送方式／偏好调用及新增键字段组合与非法类型／换行；该端使用固定请求替身，不冒称产品结果。
- 锁依赖桥接组件bridge-b01：29项固定依赖、1614文件，当前九个桥接源码文件逐字与组件内核对。ZIP SHA256为ac1ba4c68ab93251904ae086b0a9b633c193f2b4b145eee1943865a014072887。产品验证使用其runtime/python.exe和正式stdio入口，不依赖全局Python；本机仍有开发环境，不冒称干净机、离线或正式三包发布验收。
- 定向Clang-Tidy／Clazy：主窗与qkm_mapping_changes.cpp两文件、五个实际编译上下文，工具非零退出0、项目诊断0。原始日志及summary在out/mcp_edit143_20261110/static-b01。未重跑未变Worker／驱动／ASan全矩阵或Qt5普通Release运行。

组件构建／QtTest原件在out/mcp_persist137/build-control-*-edit143-b01.log及control-*-edit143-b01.log.qtest.txt；其余构建、Python、组件ZIP及分析原件在out/mcp_edit143_20261110。

## 普通Release实际场景

维护驱动为[mcp_bridge/tests/test_product_mapping_update.py](../../mcp_bridge/tests/test_product_mapping_update.py)。每次使用独立私有QkmProduct_*桌面、合成INI、自有EXE目录和专属v2配对。固定GUI实际打开管理窗、勾选启用并批准Read／Configure；不授RunMapping。普通SDK及随包桥接Medium8192，产品原manifest High12288，记录PID、创建时间、EXE及实际桌面；没有切Default输入桌面。独立GUI基线来自141真实Qt双击／键事件／Update，不能用本轮MCP请求或控件赋值替代。

v3的normal及cold已完整结束：普通行完整sources前后与141的qt6-normal-02一致；预览／取消不改源、dirty或INI，提交E→C、原抬起B保留，其他行／源／字段／派生采样相同。重放不重改，旧引用拒绝；同值no-op不改修订。完整保存明确confirm_scope，成功saved／reloaded后另启PID32324冷读，配置字段保留、新实例纯原键按实际加载变E、旧源stale_source／旧operation unknown，Read配置操作拒绝。normal PID7192及cold PID32324均exit0，专属配对撤销、客户端凭据删除，FilterKeys前后相同。

其余选验为v4合法抬起、禁用行及v6四场；八场实际业务请求共179次，独立复核23次未发生真实修改时的完整source_snapshot相等，包含dirty／加载上下文／原修订，不能只比较行数或控件。八产品均为上述F5制品、不同私有桌面且exit0：

| 原件场次／PID | 实际结果 |
|---|---|
| v3 normal／7192、cold／32324 | 普通更新及独立完整保存冷读，如上；cold新Read会话拒绝修改，旧引用／结果不转交 |
| v4 normal_clear_up／25008 | 完整前后与141 qt6-normal_clear_up-02相等；空抬起沿用C，显式D→C使用原备注对齐，再清空恢复完整基线；预览／取消／重放／陈旧引用／no-op、INI不变 |
| v4 disabled／23548 | 可见禁用行的完整前后与141 qt6-disabled-02相等；Disabled及发送方式3保持，其余协议／GUI检查通过；Disable未发布及普通权限凭据补清理见失败6，不伪报整轮无异常 |
| v6 common／37340 | 原F13→E／按下B→C／原抬起H保留，完整sources与141 qt6-common-02相等，其他普通／Common行和字段不变 |
| v6 common_conflict／3316 | Common原键拟改C会影响已启用普通行，unsupported_capability；所有源、上下文、INI不变，不弹确认或自动禁用 |
| v6 complex／31472 | 初始实际按下列表B、D两段，键值修改明确unsupported_capability；源／上下文／INI不变 |
| v6 read／37440 | GUI仅授Read=1，正式键值预览permission_denied；源／上下文／INI不变 |

前三组正例及Common均以全sources比较独立GUI，不仅核对请求字段；返回原GUI上仍是“映射开始”，没有请求启动或注入输入。本片没有原生输出计数器，不能据此补造逐API零调用或设备隔离证明。四组完整前后GUI比较及独立结果复核在各场gui-comparison.json、sdk-calls.json和out/mcp_edit143_20261110/product-evidence.json；v3／v4只复用已通过子场，不把其整体失败改成通过。v6最终四场原始passed.json及exit0保留；其旧通用stdout含cold用语，实际覆盖以场次／调用记录为准，维护驱动已改按本次选中case输出。

每场GUI只撤销本轮初始为空且结束唯一的测试配对；最终客户端凭据不存在。scope-release.json核实资源／未定／待处理均零，仅撤出本片固定定义摘要；保留原控制器42036至19:00和其他登记，不关闭共享窗口、不重建缓存或处理历史未知请求。该选择仅供持续开发，缓存并非正式交付依赖。

## 失败轮次与诊断边界

1. 首部署脚本误查组件旧app目录，在公共请求前FileNotFoundError；首准备目录保留，另用_r02部署，不删除或覆盖原件。
2. 产品v1／PID41276：OpenAdvanced助手固定查Tab1，与本片Normal夹具不符，原GUI请求15deb7d13c584b31a7dce19b7f47e58a有效失败，无配对／MCP修改。native_exit_code=1、native_pid25832、7.062秒、未超时／未由控制器终止；产品正常exit0。助手只在既有固定名称中补Normal，先核缓存无在途／未定后重登记摘要；未重放旧请求。
3. v2／PID32704：普通键值前后已与GUI相同；测试错误要求重放整个响应相等，未接受协议既有replayed与新查询时间，断言失败。finally又未先重开高级窗口，OpenMcp原请求1c8313e0a62446c880ac1ef85e112235有效失败，native_exit_code=1、native_pid42276、5.375秒、无超时／控制器终止；产品exit0。新独立清理PID32112经真实GUI只撤本轮唯一配对、删除其客户端凭据并正常exit0，不接管未知请求。测试改按原业务字段／目标／采样时间比较，并在清理前重开高级窗口。
4. v3合法抬起组／PID28392：清空后的完整sources已与141清空基线相同；额外显式D更新实际生成原备注对齐规则的[""]，测试误期望[]而失败。仅修测试预期，并增加空值恢复后完整回到GUI基线的检查；产品未因此修改。该进程正常exit0，GUI撤销及客户端凭据删除完成，FilterKeys不变。normal／cold有效证据保留，仅补跑受影响及未运行场景。
5. Python b01缺PYTHONPATH，在测试前导入失败；b02旧负例把本片合法original_key=A当非法，7pass／1error；只修预期，b03为8pass。均不是产品拒绝或成功证据。
6. v4禁用行／PID23548：完整GUI对照、取消／重放／旧引用／no-op及GUI撤销均已完成，pair_count=0；随后Disable在send发布请求前的控制器复核返回stale_controller，未取得底层OS错误码。原控制器42036／instance2c169a62c9c54748a1b1ddfc10fdda40后续仍按原19:00期限有效；历史和实际关闭结果确认该产品exit0、资源0。没有10_Disable请求或原生动作，不重放；只在普通权限删除本轮残留的专属客户端凭据。业务证据与清理异常分别保留，不伪造该轮完整passed文件，不将瞬时复核失败诊断为已证实的缓存关闭／更换或网络故障。
7. v5 Common／PID42512：驱动误把attached_to_ui=true当当前选择，引用了Normal源，产品按现有规则context_busy拒绝；实际GUI已选Common，原Common／Normal源均未改。产品exit0、撤销及凭据删除完成。仅修驱动按明确normal／common身份选当前源，并检查真实GUI首行；attached_to_ui仅表示源表已挂接界面。新轮次保留旧失败，未改产品或放宽当前源守卫。

上述原生失败均取得有效响应；未取得OS错误码的字段保持null，不据文字推算。缓存未因测试断言伪报未知；旧历史未知记录保留且未重放。全部失败日志、初始夹具和实际退出结果留本机。

## 未覆盖范围

不证明完整M1／M2、任意映射语法、原键冲突确认、附加Common行／非当前或隐藏源行编辑、主表内联编辑、全部运行后编辑或实际映射输出。未新增Default激活／真实系统输入、Hook／驱动专项、跨Windows登录／PC重启、Win10／高DPI、QQ或干净机交付验收。137／138／139和141／142未受影响的结果仅按原版本／机器／路径复用。

原GUI原键改动后Pure_OriginalKeys仍暂保留A／F13，本片保持这一配置观察；冷读后按实际加载变E。没有启动映射，不据此宣布运行错误或已修复。

## EDIT-GUI-144：禁用开关原GUI基线

2026-11-10 PC-W11，产品源码仍c35aa57de522b445c7531c0fe890e314ee0ad195，仅维护用例增加settings_disabled_gui及必要登记；未修改产品。复用141合成INI，四组normal／disabled／common／hidden，其中Common组启用普通表的Common附加，隐藏组启用HideDisabled。初始值经独立INI正常启动，业务动作均为真实Qt双击、复选框鼠标事件和Escape；未用控件赋值或直接槽建立GUI结果。

双Qt维护b01均native0，Qt6零警告、Qt5仅官方qtmain缺PDB的精确LNK4099例外。自有EXE分别为out/mcp_disabled_gui_20261110/{qt6,qt5}-runtime-b01/QKeyMapperDisabledGui144_<kit>_runtime.exe；SHA256分别63aad426235df0d9735602817d2f4370351221ab3c6e762a1c20603879ece365、ab59b1a20d2c2845dfeda7220fbc03b3baa112f95d035728b2e8f3ba6a158395。保留正式manifest，六项厂商DLL逐项核对，各Qt按对应kit部署，不复制个人INI／凭据。

| 场景 | Qt6 PID／项数 | Qt5 PID／项数 | 实际结果 |
|---|---|---|---|
| normal | 25336／9 | 39608／9 | Item禁用立即只改首行Disabled及dirty；Escape不回滚；表格重新启用恢复完整源数据 |
| disabled | 32016／9 | 37360／9 | 初始禁用行由Item启用、Escape保留，再由表格禁用；完整其他字段／派生／源保持 |
| common | 21836／9 | 26188／9 | 真实选择Common后同样两入口往返；附加Common的普通表源数据未被修改 |
| hidden | 18840／10 | 9120／10 | Item禁用立即隐藏首行；仍打开的Item可重新启用并重显；关闭后表格禁用再次隐藏，完整非Disabled字段保持 |

各kit37项／合计74项，八个实际不同QkmDisabledGui144_*私有桌面，High12288产品／Medium8192控制器、同登录、正常exit0。两类复选框按实际style指标区域点击，所有源逐字段／派生比较；加载代次及INI不变，已接受开关保留dirty，退出work／producers0。FilterKeys前后全字段相同，两次Default前台采样均非本轮产品（不是连续监测）。本片没有配对／客户端凭据或失败轮次；没有截图、普通Release新运行、正式MCP禁用字段、冲突确认／自动禁用、其他实例、真实输入、驱动、Win10或全量ASan／静态结论。

原件仅本机out/mcp_disabled_gui_20261110，包括build／deploy／definition／registration、两份gui日志、runs/*-01完整before／after_item／after_table及closed／INI、summary.json和scope-release.json。后者核实两份固定授权已撤出、owned／unresolved／pending0，原42036缓存仍至19:00；无其他授权被关闭或未知请求重放。主计划141／142仅压缩重复详录，完整压缩前文字在Git c35aa57d，所有本机原件保留。

## EDIT-145：独立禁用字段正式接线

2026-11-10 PC-W11，源码0db06e9f57529394a0bc0441bd73474e682c4489加本片；既有mapping.update新增独立disabled布尔值，不与键值／发送方式混用，不加工具／权限（仍17／23、Read＋Configure）。候选使用原完整行和结构校验，启用前复用原互斥／有效Common分析；需改其他行、Common确认或优先回退时unsupported_capability，零修改。本片没有调用带showPopup=false的互斥提交函数充预览，也未新建字段写入框架。合法提交只改Disabled、原显示刷新及dirty，不保存或开始；同值不刷新／推修订，隐藏后继续写需显式显示。原键值及发送方式合同保持。

生产主窗16增／2删，唯一锚点生成补丁经git apply --check后一次应用；最终main为2424898字节／53263行，UTF-8无BOM／LF，无截断。Qt6普通Release b01增量native0、零警告；实际EXE为out/mcp_product/disabled145_20261110/runtime-中文/QKeyMapperMcpAcceptance_disabled145_qt6.exe，SHA256为3caaafd7880b7d40e6ea3c4a9e15d0d1ae0a947abc88438d9c6f5162e1acd66a，原manifest。六项厂商DLL与当前仓库核对，其余DLL／plugins只取同PC自有137副本，没有INI／凭据复制。随包桥接bridge-b01仍29固定依赖／1614文件，九个源码与组件逐字核对，ZIP SHA256为2b3f13e5a5f74ecedd4351142b6999be1c7061899948be4373dd82c0daf5db14；实际SDK用其独立runtime/python.exe，不是仓库venv充交付证明。

双Qt控制组件构建native0；普通私有进程PID20328／19200，四槽加init／cleanup各6pass、零fail／skip。新增true／false保留、非布尔拒绝及与四类旧字段混用拒绝；原参数／预览存储边界保持。Python正式stdio schema／错误结果组件8pass，含两布尔值、混用／null／数字／字符串拒绝及旧调用回归；固定请求替身不当产品。Clang-Tidy／Clazy对主窗及协议两文件、五实际编译上下文native0、项目诊断0；大型维护上下文实际分析完成后才收敛，没有省略或把未完成当通过。未新增Qt5普通Release运行、ASan或未改Worker／驱动完整门禁。

正式产品八场共167次SDK调用，四组全sources与144独立GUI对照；逐份原始调用另核22次未真实修改时的完整source_snapshot（含dirty、加载上下文及修订）相等。均为实际不同QkmProduct_*私有桌面、原manifest产品High12288／SDK与随包桥接Medium8192、同一登录；GUI实际启用并配对，每场初始专属凭据为空，只授Read／Configure=3或Read=1，不授RunMapping。

| v1场景／PID | 实际结果 |
|---|---|
| flag_normal／21356 | 禁用首行，完整前后与144 normal的before／after_item相等；预览／取消、重放、旧引用、no-op及INI不变，随后显式完整保存saved／reloaded |
| flag_cold／38820 | 新产品及新Read桥接独立冷读保留Disabled=true及整行配置，原键实际加载仍A；旧源stale_source／旧operation unknown，Read修改拒绝 |
| flag_disabled／16196 | 初始禁用行启用，完整after_item相等；同值修订不变，反向提交再与144 after_table全源相等 |
| flag_common／30632 | 实际选Common后禁用／启用往返，两入口全源结果均与144相等；普通附加源及其他行／字段／派生不变 |
| flag_hidden／39044 | 禁用后全源与144隐藏基线相等，新行引用继续写invalid_target，源／上下文／INI不变，不自动改筛选 |
| flag_conflict／7636 | 合成Normal首行C禁用、另一行C启用；拟启用首行会改他行，unsupported_capability且全源／上下文／INI不变 |
| flag_common_conflict／42732 | Common C初始禁用，包含Common的Normal C启用；拟启用Common需处理普通冲突，unsupported_capability且零修改／不弹确认 |
| flag_read／16688 | 独立Read=1对disabled正式预览permission_denied，全源／上下文／INI不变 |

各场GUI仍显示“映射开始”，未请求映射或系统输入；没有逐原生API计数器，不据此补造输出零调用或设备隔离证明。本片未运行只配置演示整轮，底层模式／输入守卫未改，138／139原结果仅按原范围复用；未扩展到完整互斥确认、混合字段、任意映射语法、其他实例、Default、Win10／高DPI、跨Windows登录／PC重启、QQ或干净机三包交付。143原键值路径未变，复用其原版本证据，不称本轮重跑。

八产品均实际exit0；GUI各撤本轮唯一配对，最终客户端凭据不存在。八普通桥接原PID均不再存活；取得不了原退出码时明确null，进程查询的实际87另记，没有推算exit0。FilterKeys全字段前后相等，无本轮测试保持键需要额外注入释放。先前历史未知请求未重放或接管。145选定产品／组件没有失败轮次；只读进程观察的Get-Process无匹配不被记成产品／分析失败。

原件仅本机out/mcp_disabled145_20261110与out/mcp_product/disabled145_20261110/v1，含main.patch、构建／锁组件／分析报告、部署／固定定义、每场topology／GUI／sdk-calls／closed／FilterKeys／完整保存INI、product-evidence.json、bridge-cleanup.json和scope-release.json。双Qt控制原件在out/mcp_persist137/*disabled145-b01*。资源owned／unresolved／pending及授权均0后，仅关闭本任务管理的42036缓存；off返回closed／generation2／native0，后续status再次确认不可用。关闭前无其他活动授权，保留登记元数据及全部历史；closed中的旧last_error来自143，不是本轮off失败。未删除环境、构建、缓存文件或原件。

收尾文档核对UTF-8无BOM／LF、相对链接、主窗完整字节／行数和git diff --check；当前主计划及本事实页在256KiB建议内，任务目录在2MiB内。既有validation.md在本片前已超过256KiB建议，本次仅加短索引，不为本次停止压缩其历史或宣称全部旧文档均符合大小建议。关闭后status实际返回controller=null／native0，普通检查先误按活动控制器形状取cache_available而KeyError；按实际关闭形状复核通过，没有再次off或启动缓存，不是原生关闭失败。

## EDIT-GUI-146：原键互斥与Common确认GUI基线

2026-11-11 PC-W11，从808f7e0b92fdc004874d3ae5be5a6459fbd6572b同机轻量恢复，工作区干净；环境及145等证据沿用，未重建venv或重复完整门槛。仅维护目标增加settings_original_conflict_gui，产品实现／MCP合同／TS/QM未改；工具及业务能力仍17／23，正式MCP仍拒绝原键冲突。

先查Item Update／Enter、主表原键编辑和映射代码三个调用方：原键互斥助手会写行、发通知、刷新；Common即使showPopup=false仍询问。Item把临时原键先写入，再询问，No由调用方恢复原键并保留草稿；不能把该函数直接当预览或仅凭参数false称无副作用。此片以真实Item入口验证，不将主表内联或映射代码入口倒填成运行通过。

双Qt维护b01增量native0，Qt6零警告／Qt5仅官方qtmain缺PDB的精确LNK4099例外。实际EXE为out/mcp_original_conflict_gui_20261111/{qt6,qt5}-runtime-b01/QKeyMapperOriginalConflict146_<kit>_runtime.exe，SHA256分别fa2b234cc4980817e3fb0fb312e4671e642656a0a9691a2a076d5b6cb9fa1e3b、ec8e7eb42e05be2fb9721d7942b80429ba0a845e8f926164383692f5ff3c5f99。原manifest，六项厂商DLL逐项与本机仓库核对，其余Qt按对应kit部署，没有个人INI／配对复制；普通启动前检查FakerInput缺席，未扩大原启动／关闭边界。

样例来自141合成INI；除同表重复组外让Normal包含Common，normal_common组预置Common原键E，禁用Common组预置其首行Disabled=true。正常启动后真实Qt双击、Ctrl+A／字符输入、Update、已有模态按钮和Escape，未用控件赋值／直接业务函数代替GUI。目标原键改C（normal_common改E），映射草稿统一Z，原抬起B／H保留。

| 场景 | Qt6 PID／项数 | Qt5 PID／项数 | 实际行为 |
|---|---|---|---|
| normal_duplicate | 6200／9 | 15748／9 | 同表已有C，提交新C→Z后仅当前行自动禁用，不询问；另一C保持启用 |
| normal_common | 26580／9 | 14964／9 | 有效Common E优先，普通行改E→Z后仅当前行禁用，不询问 |
| common_no | 26472／11 | 22856／11 | Common F13改C触发一个默认No确认；No恢复完整模型／dirty、保留C／Z草稿，零禁用通知 |
| common_yes | 29708／10 | 5624／10 | Yes提交C→Z并禁用Normal中冲突C，Common当前行保持启用，一个禁用通知 |
| common_disabled_no | 25764／11 | 23336／11 | 原Common禁用仍询问；No保持全部原源及禁用状态／dirty，保留草稿 |
| common_disabled_yes | 13628／10 | 25652／10 | 原Common禁用仍询问；Yes仍禁用普通冲突C，当前Common保持禁用，一个通知 |

各kit60项／共120项，十二个不同QkmOriginalConflict146_*实际私有桌面，High12288产品维护进程／Medium8192观察器，同登录，全部正常exit0。Common八个确认均核对本实例父窗、目标／Normal文字及默认No，并实际点击按钮。Qt内内部观察确认期仅临时Original_Key已变，映射草稿尚未提交、其他行和dirty未变；不是正式MCP查询／外部并发可见性验收。No后整份原快照相等；Yes后全部sources逐字段／派生精确对照，只保留原GUI实际变化和通知。Pure_OriginalKeys仍为原A／F13，未修正或外推运行行为。

所有场景INI字节／加载代次及FilterKeys前后全字段相同，Escape保持已接受或拒绝后的数据，退出work／producers0；两次Default前台归属采样非本轮PID，非连续监测。没有失败轮次、配对或系统输入／映射请求；无普通Release新运行、截图／三语视觉、Default操作、驱动、真实输入、Win10／高DPI、跨登录或全量静态／ASan结论，不把维护结果当完整M1。

原件仅本机out/mcp_original_conflict_gui_20261111：build／deploy及实际EXE、definition／registration、两份gui日志、runs/*-01完整before／during_confirmation／after、confirmation／notifications／closed／INI，另summary.json和scope-release.json。十二进程实退后仅撤本片两份固定授权，owned／unresolved／pending0；保留新普通30560／instance3c8b0380f4fb423587f1a67bfb617d86至2026-11-11 19:00，未接管／重放历史未知。后续纯分析及正式冲突合同仍由唯一主计划决定，本页不另记进度。

## EDIT-147：共用纯冲突分析

2026-11-11，PC-W11、同一Windows登录。源码为66a5270ecfca8f945e27be3734c197cd90b076bb加本片差异。只增加私有analyzeOriginalKeyConflict及结果（源有效性、当前行禁用、其他行位置、确认文字），由既有两处MCP原键／启用冲突拒绝共用；原GUI autoDisableRowIfExclusiveGroupConflict及三个调用方逐字未改。分析结果不是授权／提交计划，真正执行仍经过现有源、权限、生命周期、全候选校验及执行前复核。本片不开放冲突写入，不改17工具／23能力、桥接运行码／schema／TS/QM或保存结构。

### 147 构建与独立GUI对照

- Qt6 6.8.3 x64／MSVC2022普通Release及双Qt维护目标b01增量通过；Qt6编译／链接零警告，Qt5 5.15.2官方msvc2019_64仅精确qtmain.lib(qtmain_win.obj)缺PDB LNK4099例外。普通build脚本子进程ExitCode非零会抛错，本轮报告Build succeeded且PowerShell进程exit0；外层LASTEXITCODE未取得（空），不冒称取得另一个原生码。
- 维护副本out/mcp_original_analysis147_20261111/{qt6,qt5}-runtime-b01/QKeyMapperOriginalAnalysis147_<kit>_runtime.exe；Qt6 SHA256=29b5edd05f787b127c61644075d0ce512dd50d0eded10d8bd4b4dc6b6322cdf6，Qt5=36f46a56c3429cf3fadc29345918d67bdce9a03d8f205d158f9db8cd1f33c54d。六厂商DLL逐项核对，Qt依赖用对应kit部署，保留manifest，不复制个人INI／凭据。
- 十二实际私有High进程均exit0，双Qt各84项共168通过；每场新增四项纯分析断言，原146的完整before／after／确认期间源、dirty、实际文字／默认No／回答、通知及目标逐项相等，共十二全量对照。分析前后完整源、epoch、dirty、加载、磁盘不变且无模态；无效索引不可用。初始禁用Common依然预测影响普通行，符合原GUI实际行为。六场原始键冲突均走真实Qt事件／按钮；纯助手调用仅是额外预测，不冒充GUI基线。

| Qt | 场景 | PID | 断言 |
|---|---|---:|---:|
| qt6 | common_disabled_no | 28360 | 15 |
| qt6 | common_disabled_yes | 10644 | 14 |
| qt6 | common_no | 4304 | 15 |
| qt6 | common_yes | 6368 | 14 |
| qt6 | normal_common | 30604 | 13 |
| qt6 | normal_duplicate | 23532 | 13 |
| qt5 | common_disabled_no | 26972 | 15 |
| qt5 | common_disabled_yes | 11540 | 14 |
| qt5 | common_no | 15108 | 15 |
| qt5 | common_yes | 7352 | 14 |
| qt5 | normal_common | 20920 | 13 |
| qt5 | normal_duplicate | 2940 | 13 |

实际桌面均QkmOriginalAnalysis147_*，普通观察8192／产品12288、同登录；INI逐字不变、FilterKeys前后相等、两次前台采样非自有PID（非连续监测）、退出work／producers0。无失败轮次，无配对、映射启动或系统输入。构建／部署／GUI／对照／汇总原件在本机out/mcp_original_analysis147_20261111，禁止由此推断Default干扰因果、完整冲突操作或设备验收。

### 147 正式MCP与分析

普通Release独立运行副本out/mcp_product/original_analysis147_20261111/runtime-中文/QKeyMapperMcpAcceptance_analysis147_qt6.exe，SHA256=e4b30b46dcce084a48bbef60694aec1f697b1b4e7da54ed1a6c086560f0d8e85。六厂商DLL与仓库匹配；仅取同PC自有137运行副本的DLL／Qt插件，不复制INI或凭据。正式SDK／桥接Medium8192；产品High12288，真实QkmProduct_*私有桌面，同登录。每轮GUI启用并批准专属Read／Configure，无RunMapping；随包145组件九个运行源码与当前逐字核对，沿用ZIP SHA256=2b3f13e5a5f74ecedd4351142b6999be1c7061899948be4373dd82c0daf5db14，不重打未改依赖。

| 正式场景 | PID | 调用 | 结果 |
|---|---:|---:|---|
| normal | 5716 | 31 | 预览／取消／提交与141原GUI全源一致，重放／旧引用／no-op及完整保存成功 |
| key_normal_conflict | 31524 | 9 | 同表原键重复，unsupported_capability、零修改 |
| key_common_priority | 30292 | 9 | 普通原键受有效Common约束，unsupported_capability、零修改 |
| common_conflict | 29852 | 9 | Common原键影响普通行，unsupported_capability、零修改 |
| flag_conflict | 30732 | 9 | 启用影响同表行，unsupported_capability、零修改 |
| flag_common_conflict | 24704 | 9 | 启用Common影响普通行，unsupported_capability、零修改 |

六场76正式调用、一次141独立全源对照、九次完整source_snapshot上下文（含dirty／加载）不变性通过；五拒绝全源／磁盘保持，normal保存明确完整范围，无隐式开始。产品全部exit0，专属唯一配对GUI撤销至0、停接口、CredentialStore.forget后load为空；六普通桥接不再存活，未取得退出码的明确null，不从进程缺失推算。FilterKeys每场前后相等。实际拓扑／调用／GUI动作／closed／bridge-peers原件在out/mcp_product/original_analysis147_20261111/v1，核对汇总product-evidence.json／bridge-cleanup.json在147本机证据目录。

定向Clang-Tidy／Clazy仅主qkeymapper.cpp、普通／sendinput-lifecycle两个真实编译上下文；脚本exit0、各工具非零退出0、项目诊断0。原件static-b01含summary.txt；未增加未变协议／Worker全矩阵或ASan。原GUI冲突函数及主文件两处调用、Item第三处调用逐字保持。本轮GUI／产品无失败轮次；普通事后桥接核对初次用未安装项目包的开发venv报ModuleNotFoundError，随后用原已核验随包Python -I检查成功，没有重启或重放产品动作。最初GUI调用计数核对把两个主文件调用和一个外部Item调用误按三个主文件调用计数，已按实际2＋1修正；不构成产品失败。

普通控制器30560／instance3c8b0380f4fb423587f1a67bfb617d86保持既有2026-11-11 19:00期限，147三份固定权限已撤出，owned／unresolved／pending及action／job为0／空，剩余授权0。缓存继续开发未off，历史未知不接管或重放。未执行新的Default／系统输入／映射／驱动／全局设置、Win10／高DPI／截图或三语视觉、Qt5普通Release正式MCP、冷读重启／跨登录、完整M1或设备验收；保存冷读／schema等复用145原范围，不能相加称本轮通过。
