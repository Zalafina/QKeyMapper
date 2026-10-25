# UI-73：MCP 用户界面翻译与按钮行为验证

本页只记录证据和覆盖边界，进度及下一步以[唯一主计划](qkeymapper_mcp_automation_plan_codex.md)为准。用户已按[英中日对照](ui_translation_recommendations.md)完成正式 `.ts/.qm`，正式验收通过后与本片代码共同提交；以本页最终验收记录及主计划交接卡为准。以下前置／待验文字保留其原发生阶段，不作为当前未完成状态。

## 2026-10-26 前置验证（临时译文）

- 源码：`codex/qkeymapper-mcp-control`，HEAD `2514ca50` + UI-73工作区。用户的 `.ui` 改动为 MCP 入口 `NoFocus` 与 `autoDefault=false`，已审阅保留。
- 机器：PC-W10，Win10 19045 x64；当前普通用户执行编辑、构建、维护、截图。未启动产品、不使用Default、不注入系统输入、不运行映射或安装驱动，无UAC。
- 生产范围：31项英文tr源；ControlDialog删除手工三语言分支，title()由窗口／高级设定／托盘共用，retranslateUi()保留状态后刷新。主窗口两个上下文错误改为QKeyMapper::tr，停止和超时提示共用只改文字的刷新；无Worker／manifest／协议／依赖锁变化。

| 检查 | 实际结果及边界 |
|---|---|
| Qt6普通Release产品 | `scripts/build_qt6.ps1 -Jobs 2`，原生exit0，零编译／链接警告；`out/build_qt6/release/QKeyMapper.exe`。正式QM仍是旧资源，只构建未启动，不算正式翻译验收 |
| 维护构建 | Qt6.8.3 msvc2022_64／Qt5.15.2 msvc2019_64，Release／asInvoker，两者exit0、零编译／链接警告。VS2022 Community vcvars64，原构建目录及对应Qt DLL |
| 双Qt词条提取 | 两版本lupdate均将31项准确提取到QkmAutomation::ControlDialog（27项）和QKeyMapper（4项），含换行与%1/%2/%3；未生成或改写正式TS/QM |
| 临时资源 | `out/mcp_ui_localization_20261026/temporary-{qt6,qt5}`，分别由对应lrelease生成三份QM；使用建议译文，非正式交付资源 |
| Qt6维护运行 | 实际私有桌面QkmMcp_b2f51137282e404ba5b3e4d9d159e3a9，普通PID11296，35 passed／0 failed／0 skipped，native exit0 |
| Qt5维护运行 | 实际私有桌面QkmMcp_bac80dee808d42619145275ccc736a44，普通PID32752，35 passed／0 failed／0 skipped，native exit0 |
| 按钮行为 | 使用本次真实.ui生成代码，NoFocus、非默认、Tab跳过；鼠标点击仍产生clicked，之后Space／Enter不再点击。仅Qt事件，无系统键盘／鼠标。增加独立Tab参照，不修改产品焦点策略；不将生成UI测试冒称完整产品入口点击 |
| 语言及状态 | 实际安装QM／QTranslator；独立读取译文对照标题。切换中文→日文→英文时，接口／发现路径、配对记录／选中项、待配对进程及额外权限选择不变，不意外批准或拒绝；显式拒绝后客户端正常退出 |
| 静态截图 | 每Qt三语言×off/pending/paired-off/busy-off共12张，合计24张。通过capture_ui_snapshots.ps1 PrintWindow，实际私有桌面、原生DPI144及Qt DPR1.5写入逐图JSON；没有切换输入桌面 |
| 自主视觉审阅 | 已用view_image审阅两Qt各三语言的pending及busy-off共12张：文字、状态后缀和操作按钮可读，无新裁剪，长EXE为既有可选择滚动行。其余off／paired-off已捕获但不声称本轮逐张视觉审阅；正式资源后按实际影响再验 |
| 规则 | AGENTS、workflow及Claude镜像已更新；两份技能正文完全一致。skill-creator quick_validate通过，使用既有out/cache72/validator/PyYAML，不安装包／不改锁依赖 |

维护EXE：`out/tests/mcp_control/{qt6,qt5}-release/release/QKeyMapperMcpControlTest.exe`；IPC：`out/tests/mcp_ipc/{qt6,qt5}-release/release/QKeyMapperMcpIpcTest.exe`。Qt bin按上述匹配版本。执行入口为本仓库 `QKeyMapper/tests/mcp_control/test_mcp_control.py --translations-dir <本次QM目录> --screenshots`，测试读取 `QKM_TEST_TRANSLATIONS_DIR`；普通运行默认仓库正式translations目录。

原始证据全在本机忽略的 `out/mcp_ui_localization_20261026/`，不随Git同步：`build-product-qt6.log`、`build-control-qt{6,5}-r3.log`、`extracted*.ts`、`extraction-result.json`、`temporary-*`及`test-qt{6,5}-r2/`（results.txt、runtime.log、run_private.json及各图JSON/PNG）。源码及维护目标对应本片工作区，不把旧二进制当本轮通过。

## 失败及夹具校准

- 首次维护编译缺少新增QSignalSpy显式头文件，两Qt编译失败；补齐后-r2／-r3构建通过。原始build-control-qt*.log保留，不计通过。
- 首轮运行两Qt各34通过／1失败：原高级设定其余控件使用ClickFocus／NoFocus，无法满足夹具额外的“Tab必须移走”假设。测试增加独立、正常Tab焦点参照后，确认驱动有效且MCP入口仍跳过；产品焦点和控件规则未为测试放宽。原test-qt6／test-qt5记录保留。
- 技能验证初次PYTHONPATH指向缓存解压目录的父目录，找不到yaml；改用既有PyYAML子目录后通过。无新依赖或验证器修改。

## 正式资源后待验

1. 用户更新的31项正式词条context、占位符、空格／拼接后缀、翻译状态及本片之外的意外变更；三份正式QM及其实际加载。
2. 重建受正式资源变化影响的目标，验证产品中高级设定入口、共用托盘标题及MCP对话框的正式三语言显示。停止／超时和映射错误的正式取值／刷新仍待相应受限夹具确认；当前提取／编译不冒充运行覆盖。
3. 使用正式资源在私有桌面检查相关状态及布局；临时译文截图不替代此项。必要启动原manifest产品须走既有公共受管入口；产品原有FakerInput连接／复位副作用按既有授权处理，开发、SDK和截图保持普通权限。
4. 通过后更新本页与主计划，代码／用户UI／正式TS/QM／规则／测试／文档共同形成一个独立本地提交，不推送并停止；跨机接续仍为C1-B2-PROFILE。

未执行Win11、Default、真实输入、驱动、完整引擎性能、ASan或静态分析；本片不影响其既有验收门槛。当前无受管控制器或原生缓存会话（status只读核实），无本片遗留产品或维护进程。

## 同轮追加：MCP 对话框焦点约定

用户进一步明确MCP对话框的控件也应以NoFocus／ClickFocus为主、默认不启用Tab导航。按钮／复选框NoFocus，两个只读路径输入框、可选择文本标签及客户端列表ClickFocus；保留点击后选择、Ctrl+A／复制及列表导航，不加键盘拦截。新增controlFocusPolicy用例，前述35项及截图属于该追加前的有效翻译证据；焦点变更不改变其词条、布局或未聚焦预览状态，不重复捕获作为正式资源证据。

- 包含焦点变更的普通Qt6 Release产品及双Qt维护构建均exit0，零编译／链接警告；原始日志为build-product-qt6-focus.log及build-control-qt{6,5}-focus.log。
- Qt6私有桌面QkmMcp_f06dcf6e64cd4283be440c75bbeb5282、普通PID13616；Qt5为QkmMcp_be64a97b13b4481e8221253c4a7c1f9e、普通PID29416。两者各36 passed／0 failed／0 skipped、native exit0，结果在test-qt6-focus／test-qt5-focus；仍使用对应temporary-Qt版本的临时QM。
- controlFocusPolicy确认全部按钮／复选框、标签／路径／列表实际策略，ClickFocus路径可鼠标聚焦并Ctrl+A选择（不写系统剪贴板），鼠标启用后仍保留路径焦点，Space／Enter不再切换开关；独立Tab参照验证导航有效且不进入产品控件，鼠标关闭窗口保留接口。其他配对／拒绝／撤销／失败及语言刷新回归同轮通过。
- 两份workflow镜像保持一致，技能结构重新校验通过；正式TS/QM仍未修改，资源最终验收与共同提交继续等待用户。

## 用户译文审阅后的三条调整

用户已更新其他TS词条，尚未生成正式QM；Agent未修改这些TS。中文状态建议改为“MCP 接口已关闭／已启用”，指代更明确；英文仍MCP is disabled／enabled，日文仍MCP は無効です／有効です。关闭按钮源统一为Close，译作关闭／閉じる，原“关闭窗口不关闭接口”的说明保持。CloseMcp固定助手同步为mcpClose ID及新三语言文本，行为和授权范围不变；以后登记助手须使用其新摘要。

此前双Qt36项及构建是Close window文案版本；焦点／状态保持证据继续适用，新Close源和助手待正式资源到齐后构建及运行验证，不把旧截图当新文案已经验收。用户需重新lupdate补齐三条并lrelease，最终一起提交，不提前提交。

## 2026-10-26 正式资源最终验收

来源为2514ca50及本UI-73提交前工作区（与本记录所在提交的代码／资源一致）。用户更新正式三份TS/QM，Agent未改译文；旧位置引用由lupdate刷新。按context/source/comment比较，三个TS各新增31项，既有翻译修改0、删除0；31项无unfinished、占位符完整且QM值逐项一致。完整QM还核对全部完成译文：英文1144、中文1147、日文1147，无缺失或值变化；允许与源文本相同的条目使用原文回退。

| 检查 | 正式结果 |
|---|---|
| 普通Qt6产品 | build-product-qt6-formal.log，exit0、零编译／链接警告；out/build_qt6/release/QKeyMapper.exe。正式requireAdministrator manifest不改 |
| 双Qt控制维护 | build-control-qt{6,5}-formal.log均exit0、零编译／链接警告。使用仓库正式QM，各36 passed／0 failed／0 skipped，进程exit0，覆盖语言刷新、配对身份／权限保持、焦点、鼠标关闭、失败与撤销 |
| 实际产品类维护 | build-ui-product-qt{6,5}-final.log均exit0；Qt6零警告，Qt5仅既定官方qtmain缺PDB LNK4099例外。复用原产品类、真实入口及完整嵌入资源；6次各11项，共66项通过，进程原生exit0 |
| 临时UI副本边界 | 原构建EXE保留requireAdministrator；只在out/actual-*-r5/runtime副本改为asInvoker，实际Medium8192、elevated=false。无映射启动／真实系统输入／驱动安装，不算High权限或驱动验证；初始化／退出仍沿原产品逻辑 |
| 实际入口与提示 | 高级设定入口、ControlDialog及托盘动作的同一正式标题，入口NoFocus／非默认；Qt鼠标打开／关闭，Close新文案。停止／超时仅设置并还原显示状态，真实业务守卫返回本次正式翻译，源代次／清理阶段／准入不变；未执行真实停止 |
| 托盘边界 | 通过既有QSystemTrayIcon Context信号进入文字刷新处理并隐藏菜单；没有Default系统托盘点击或遮挡验收。其标题本来在Context激活时懒刷新，不要求隐藏菜单的旧缓存提前变化 |
| 截图 | 正式控制维护24张（双Qt×三语言×四状态）；实际高级设定／控制窗口12张，共36张。capture_ui_snapshots.ps1 PrintWindow，普通私有桌面、PID和HWND所有权核对，实际可见窗口分阶段捕获 |
| 自主视觉审阅 | 两Qt三语言的正式pending／busy-off共12张，加实际高级设定／控制窗口12张，共24张已审阅。MCP入口、标题、状态及新按钮完整可读；off／paired-off其余12张已捕获，不冒称逐张审阅 |

实际QKeyMapper类的每次运行：

| Qt／语言 | PID | 实际桌面 | 检查／退出 |
|---|---:|---|---|
| qt5:0 | 17220 | QkmUi73_9c7985255e7d4efc89c762dd00ac70ba | 11通过／0 |
| qt5:1 | 28644 | QkmUi73_770fea6b260e4de7b192970abb99ecb5 | 11通过／0 |
| qt5:2 | 21400 | QkmUi73_287cd3f3d6f94d37abe5c1dfa5bd5fe2 | 11通过／0 |
| qt6:0 | 23236 | QkmUi73_29822c650eff44519d93f71c067485f6 | 11通过／0 |
| qt6:1 | 24704 | QkmUi73_076c4336d44e490094bc3f7b0ca497ff | 11通过／0 |
| qt6:2 | 32456 | QkmUi73_a5e40af1020345a7ae54064b1db3f3a0 | 11通过／0 |

正式控制维护：Qt6为PID19252、QkmMcp_61b1655585d04eb0b18bbc1e85d95626；Qt5为PID31372、QkmMcp_8dc4dc4937c442059b386103c968b51b。完整运行／逐图状态与DPI在formal-control-qt{6,5}/。实际界面的ready、.advanced、result、observed及两个PNG在actual-*-r5/；observed保留原构建EXE摘要以区分修改manifest的UI副本。目录均位于本机out/mcp_ui_localization_20261026，不随Git同步；恢复参数和分阶段契约已纳入维护README。

校准失败保留：UI维护初次缺少QSaveFile显式头文件，修正后编译通过；actual-qt6-0／-r2／-r3先试图在MCP激活后截图已经按既有逻辑关闭的高级设定窗口，身份／可见性守卫拒绝，未放宽守卫。-r4中文成功后英文仅托盘旧缓存断言失败，查明原实现按Context激活刷新，最终用例走该信号入口；不是生产托盘翻译修复。最终-r5全部通过，前述失败不计通过。正式产品除本片UI文字／焦点外未为测试改动。

范围外观察：两Qt实际日文高级设定截图中，未改动的旧fullscreenGlobalMappingProcessLabel说明末尾有固定宽度裁剪；该词条／既有译文未修改，MCP入口和控制窗口日文不受影响。本片只验收新增MCP区域，不视为整个高级设定旧控件的三语言全量验收或用户已接受该裁剪。

收尾确认：无本片自有进程／窗口遗留；受管gsudo controller=null、native_sessions_observed=0，本片无UAC。UI-73形成独立本地提交，不推送并停止；C1-B2-PROFILE、完整保存及MCP启停仍留后续。未新增Win11、High GUI助手运行、Default输入／真实托盘、引擎性能、ASan或静态分析结论。CloseMcp固定助手已通过语法检查，其稳定mcpClose ID和实际按钮已在普通UI副本验证；没有把它冒称本轮High助手实跑。
