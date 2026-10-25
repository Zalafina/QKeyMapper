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
