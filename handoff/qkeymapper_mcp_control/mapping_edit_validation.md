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
