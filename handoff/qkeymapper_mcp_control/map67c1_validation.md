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
