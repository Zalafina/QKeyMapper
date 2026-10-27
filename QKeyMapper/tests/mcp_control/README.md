# MCP 管理窗口验证

按产品高 DPI／Fusion／字体基线，使用产品同一个 `ControlDialog`，通过 Qt 界面事件点击实际复选框指示区／按钮，再由独立 IPC 测试子进程完成正式配对证明。读取／批准／撤销断言检查真实控制器及私有凭据，不以控件值代替结果。没有产品业务回调；测试的 fixture 回复不代表产品能力。

在忽略的独立目录对 `mcp_control_test.pro` 运行相应 Qt 的 qmake 和 JOM。运行时 PATH 加入对应 Qt `bin`，`QKM_IPC_TEST_EXE` 指向相同 Qt 的 `mcp_ipc_test.pro` 产出的绝对 EXE。两个维护目标均为 asInvoker，无需修改产品 manifest。DPAPI 需要可用的当前用户环境；本机 Codex 默认沙箱的限制与批准的非提权运行方式见[验证记录](../../../handoff/qkeymapper_mcp_control/validation.md)。

测试窗口需要真实显示；优先在私有 Windows 桌面运行，**不调用 SwitchDesktop**。可以设置 `QKM_TEST_DESKTOP`，测试将检查自身线程实际桌面名。Qt 事件直接投递目标控件，不发送系统键鼠输入。关闭并清理自有窗口和子进程，不关闭用户实例。

语言用例实际安装 `QTranslator`，不再以整数切换硬编码文本。设置 `QKM_TEST_TRANSLATIONS_DIR` 指向三份本次 `.qm` 的目录；Python 入口支持 `--translations-dir`，默认使用仓库正式 translations。正式资源未补齐时可明确传入 out 中的临时译文，结果必须记为临时接线验证。`languageRefreshPreservesState` 核对语言刷新保留接口、配对选中项、待批准进程及权限选择；`advancedEntryKeyboard` 使用真实 `.ui` 生成代码验证 NoFocus、非默认按钮、Tab 跳过及鼠标点击，不发送系统输入。

`controlFocusPolicy` 核对MCP窗口按钮／复选框NoFocus，标签、路径输入框和客户端列表ClickFocus。使用独立Tab参照验证不进入产品控件；鼠标启用和关闭仍有效，路径可点击选择文字，Space／Enter不误切换开关。不写系统剪贴板，不把Qt直接事件等同于系统键鼠输入。

`sourceSnapshotEncodingBoundaries` 核对精确 4 MiB 前后边界及完整绑定的原修订字节，包含中日文、emoji、引号／反斜杠／换行。`sourceSnapshotQueryCost` 用 1／100／1,000 行合成不可变快照分别测目录和单行助手，QtTest 可用 `-iterations 32 -median 3` 固定比较。该数值不含产品源复制、锁占用、IPC／SDK 或 GUI 响应，不能当作完整查询或输入性能。

`mappingChangeArguments`／`mappingChangeLifecycle`／`mappingChangeLimits` 验证基本新增预览基础：严格参数、单字母／数字与源引用、有界120秒预览、实例／配对／权限代次归属、消费／取消／替换、10分钟／64条结果、未知结果及重置。存储本身不授权或修改映射；产品实际提交另外检查完整源、业务状态、当前请求／权限和到期。`sourceSnapshotReferences`验证同步组单改的完整修订；正式B1业务证据见[独立记录](../../../handoff/qkeymapper_mcp_control/map67b1_validation.md)。

上述纯边界可用普通权限 `QT_QPA_PLATFORM=offscreen` 运行，并通过 QtTest `-o <绝对结果文件>,txt` 保存报告；这不是私有 Windows 桌面或 GUI 验收。不得以没有 stdout 当作通过。产品／正式 SDK 接线和两条独立 GUI／MCP 路径另行验收。

覆盖默认关闭／不创建凭据目录、启用时发布并核实发现文件、中英日标题及明文标签、Enter 不默认批准、只读默认值、额外配置／启停授权、配对批准、拒绝、撤销、关闭窗口保留接口、关闭／Escape 时拒绝待批准请求和启用失败回显。关闭接口时通过真实控件查看／撤销，核对实际持久化和重新打开结果；只读保存失败与另一活跃实例占用不得假报成功。可见选中项与键盘导航 currentRow 不同时须撤销选中客户端，无选中不得撤销；通过独立读取存储核对剩余 pair ID。`queryProcessExecutable` 还检查真实自身路径、错误创建时间及无效身份拒绝。

`--preview <language:0|1|2> <off|pending|paired-off|busy-off> <absolute-state-json>` 仅用于维护目标的静态截图：真实显示同一控件，原子报告 PID／HWND／实际桌面、原生可见性、Qt／原生 DPI、样式／字体及接口／配对状态；20 秒后自动退出，也可从该私有桌面发送 WM_CLOSE。pending 含 `<b>` 和实际自身进程身份，用于检查纯文本；paired-off 使用假配对记录，busy-off 再取得真实租约并点击撤销制造占用提示。四种预览均不启用接口、批准或认证客户端，截图不能证明配对安全。使用仓库 `scripts/capture_ui_snapshots.ps1 -ProcessId ... -WindowHandle ... -PrintWindow` 后自主审阅。

已验证范围按[验证记录](../../../handoff/qkeymapper_mcp_control/validation.md)的版本／场景分别列明，不能将旧通信结果当作全部业务验收。B1另覆盖Read+Configure及基本内存添加；Default实际输入和性能由独立input_regression入口验证。本目录私有High产品的普通PrintWindow未算视觉通过，不能拿Default屏幕复制替代这一缺口。

## 可移植执行入口

`test_mcp_control.py` 的语言／标准库要求为 Python 3.10+；本任务的统一开发／跨机验收基线为 CPython 3.11.9 x64，环境锁和校验见[桥接说明](../../../mcp_bridge/README.md)。它不切换输入桌面，检查父／子进程均未提权，报告目标自身实际桌面。输出必须位于本仓库 `out/`。`capture_mcp_control.ps1` 复用仓库截图脚本，在同一私有桌面核对 HWND 所有者并关闭自有预览窗口。不要用 `python -O`；脚本拒绝关闭断言的优化模式。

构建两个维护目标后，从仓库根目录执行（Qt 路径按本机调整）：

```powershell
$qtBin = 'C:/Qt/Qt6/6.8.3/msvc2022_64/bin'
python QKeyMapper/tests/mcp_control/test_mcp_control.py --test-exe out/tests/mcp_control/qt6-release/release/QKeyMapperMcpControlTest.exe --ipc-exe out/tests/mcp_ipc/qt6-release/release/QKeyMapperMcpIpcTest.exe --qt-bin $qtBin --output out/tests/mcp_control/portable-qt6 --screenshots
```

Qt5 改用对应 Qt5 的两个 EXE、Qt bin 和独立 output。省略 `--screenshots` 只运行控件／配对测试；截图需要 PowerShell，自动查找 pwsh／powershell，也可显式传 `--powershell`。脚本运行失败返回非零；结果、运行身份及截图为本机材料，不进入 Git。

管理窗口连接框为 `instance.json` 的可复制路径，文件不含配对 key。启用失败或发现信息无法验证时，复选框回到关闭；关闭后实际文件清理也由 IPC 维护用例核对。关闭状态可查看／撤销既有配对；撤销失败保留实际记录及原因，不为撤销临时开启接口。`--screenshots` 每个 Qt kit 生成中英日四种状态共 12 张图；实际审阅范围以验证记录为准。

## 正式 SDK 互操作

维护 EXE 的 `--interop-fixture <state-file>` 创建真实管理窗口、启用同源服务，通过固定 Qt 事件批准／拒绝／撤销及注入测试保存失败；状态与事件文件仅在维护进程自有 out 路径使用，产品 EXE 不包含此入口。有限生命周期 180 秒，关闭清理服务；不安装驱动，不发送系统输入。

[桥接回归说明](../../../mcp_bridge/README.md) 中的独立官方 SDK 客户端启动 stdio 桥接，核对实际 native pipe peer／EXE、双向 HMAC、凭据持久化、撤销、拒绝、取消、退出、篡改发现、真实 60 秒批准超时和保存失败。GUI 批准前不得 ready，客户端保存了 key 也不代表服务器授权成功。默认检查全部非提权；经授权加 --elevated-server 可仅通过 gsudo 启动管理员维护服务，桥接／控制器仍为普通权限。两种模式使用真实私有桌面；该结果不能替代真正产品入口与完整业务验收。

## 正式产品入口与管理员／普通桥接

已获本轮管理员启动／关闭及受限 GUI 助手授权、用户可以确认 UAC 时运行。开发工具、控制器、SDK、桥接和截图保持普通权限；不修改产品 manifest。`product_lifecycle.ps1` 只管理自有隔离产品，`product_ui_actions.ps1` 提供固定管理窗口动作及下述合成配置的加载／源表操作，核对 EXE／PID／启动时间及实际私有桌面。OpenAdvanced 先选映射页读取实际停止快捷键／开始按钮、Tab1 和配置选择显示，再选通用页打开高级设置，避免读取隐藏控件；配置选择使用 UIA ValuePattern，Name 是可访问名称，不能当作当前选中内容。不启动映射或发送系统输入。正常产品启动仍包含对已安装 FakerInput 的连接尝试。

先构建对应 x64 Release 产品，并按本机 Qt kit 部署到全新目录。从仓库根目录执行，Qt5 改用其产品构建目录、Qt bin 和不同名称；不复制个人 INI 或另一台 PC 的产物／凭据：

```powershell
$qkmProductBuild = 'out/build_qt6/release'
$qkmKitBin = '<本机 Qt6 bin 绝对路径>'
$qkmRuntime = 'out/mcp_product/runtime-qt6'
New-Item -ItemType Directory -Path $qkmRuntime -ErrorAction Stop | Out-Null
$qkmProductExe = Join-Path $qkmRuntime 'QKeyMapperMcpAcceptance_qt6.exe'
Copy-Item -LiteralPath (Join-Path $qkmProductBuild 'QKeyMapper.exe') -Destination $qkmProductExe
Copy-Item -LiteralPath (Join-Path $qkmProductBuild 'QKeyMapperCrashReporter.exe') -Destination $qkmRuntime
foreach ($qkmLibrary in @('QKeyMapper/QJoysticks/lib/SDL/lib/x64/SDL2.dll',
    'QKeyMapper/Interception/lib/x64/interception.dll', 'QKeyMapper/FakerInput/lib/x64/FakerInputDll.dll',
    'QKeyMapper/libusb/lib/x64/libusb-1.0.dll', 'QKeyMapper/ViGEm/lib/x64/ViGEmClient.dll')) {
    Copy-Item -LiteralPath $qkmLibrary -Destination $qkmRuntime
}
& (Join-Path $qkmKitBin 'windeployqt.exe') --release --no-compiler-runtime --no-translations $qkmProductExe
if ($LASTEXITCODE -ne 0) { throw 'windeployqt failed.' }
Copy-Item -LiteralPath 'QKeyMapper/tests/mcp_control/product_fixture.ini.example' -Destination (Join-Path $qkmRuntime 'keymapdata.ini')
out/mcp_bridge/venv/Scripts/python.exe -X utf8 mcp_bridge/tests/test_product_interop.py --executable $qkmProductExe --output out/mcp_product/accept-product-qt6
if ($LASTEXITCODE -ne 0) { throw 'Product acceptance failed; inspect its logs before retrying.' }
```

运行依赖桥接的固定 Windows requirements、PowerShell 7 和 gsudo；具体安装见桥接说明，不自动安装驱动。启动未改正式 EXE 的同字节副本，用独立路径／文件名隔离 SingleApplication 和应用绑定。输出目录必须全新且在 out/mcp_product 下。产品初始化为接口关闭、自动监视／多键鼠／虚拟手柄关闭；脚本仍独立验证实际状态，不能只依赖模板。

连续固定动作使用[工作区缓存技能](../../../.agents/skills/qkeymapper-gsudo-cache/SKILL.md)及[公共测试接入](../../../.agents/skills/qkeymapper-gsudo-cache/references/test-actions.md)。通过受管run执行原产品／源参数，或先审核登记定义后直接普通运行；启动、关闭及固定GUI由同一缓存控制器派发。子脚本省略旧--gsudo-cache，该选项已明确拒绝，不创建独立或嵌套PID缓存。默认窗口五分钟、最长24小时及硬截止由技能管理，SDK／桥接／截图保持Medium。历史CACHE-40无缓存对照仅按原版本保留，不能替代真实UAC提示／拒绝、焦点及等待耗时验收。

正式 SDK 通过固定公开工具验证未批准时不可用、只读权限、实际能力、状态／目录查询、凭据重连、撤销断线／拒绝旧 key；完整退出后再启动同一产品，核对默认关闭及撤销持久化。两次均要求正常退出 0，仅清理自身客户端凭据。Qt5 按控件实际支持的 SelectionItem／Invoke 无障碍模式选择固定目标；没有任意控件或命令入口。服务有 240 秒自有产品看门狗，控制器超时不自动重启。

可用 `--load-fixture blank|saved|invalid|missing` 验证启动映射加载记录。blank 为默认模板；其余要求全新运行目录且 INI 与仓库合成模板完全相同，再由普通控制器在启动前写入测试配置。saved 为一个 A→B 源映射，invalid 含固定非法原始键，missing 指向不存在的配置组；均核对实际加载结果／代次、GUI 选择的实际值、只读不变性和目录分页。

`--load-fixture warm`准备McpSaved（A→B）、McpSecond（C→D）和McpInvalid。固定助手仅操作自有私有桌面：Qt6使用UIA ExpandCollapse，Qt5在配置控件投递成对窗口消息，再向经身份核对的唯一弹窗投递Up／Down／Enter／Escape，激活真实条目；不调用加载函数或移动系统光标。独立GUI读取实际行／选择，SDK核对温切换、空白重置、失败／恢复、同项重载及旧引用失效，只读批次检查dirty及整个INI不变。原始load_*.json保留。

warm随后调用mapping_changes_acceptance.py，**需要Read+Configure配对及固定GUI添加授权**：键选择弹窗／搜索／选中后点击真实添加按钮，最多两条A→B；RunMapping保持未授予。两条独立路径比较完整字段、派生值、dirty及未保存INI；覆盖权限、取消／替换、单次提交／重放、重连查结果、GUI编辑／切配置陈旧、新配对隔离及实际120秒到期。该产品阶段有420秒自有看门狗；不启动映射，不证明保存冷启动或实际输入效果。仅获只读授权时不要运行warm全套。

普通 PrintWindow 无法捕获 High 产品时明确记录未验证，不提升截图权限；功能断言继续独立检查。失败时先看日志、closed.json 和实际进程／配对状态，再决定接续动作；不得盲目重复配对或撤销。

## 实际源表与只读引用

`validateSourceQueryArguments` 是共用纯参数校验：产品必须先校验再采集源表，`querySourceSnapshot` 也独立调用它保护直接调用方。维护目标的 `sourceSnapshotBoundaries` 验证空快照时同样保留参数错误／解释、字段依赖、引用与修订不一致，以及只能拿到真实快照后才能检查的索引／分页边界；`sourceSnapshotReferences` 和 `sourceReadAdmission` 继续验证内容／身份／授权失效及读取准入。参数通过不代表内存安全许可；完整快照和完整内容哈希保留。读取基准包括初始静止与仍匹配当前活动修订的完整清理；sourceReadAdmission 覆盖已完成修订、后续／排队活动失效及复制互斥，sourceSnapshotReferences 覆盖相同内容改变清理代次也拒绝旧引用。完整运行后正式 SDK 用例见 input_regression 的 quick_restart 后置阶段，不把组件或无输入静止检查当成实际输入验收。

`test_source_interop.py` 使用全新部署、未经改动的合成 INI，准备两张普通源表 A→B／G→H、C→D 和一张共通源表 E→F。先独立从真实 GUI 读取三个 Tab 的实际行，之后才启用 MCP 并通过正式 SDK 分页读取非当前源表及每个源行；区分共通源与普通 Tab 中附加的共通显示行。表格 UIA 可能把合并分隔行的 ColumnSpan 报为 1，固定助手还核对分隔行的明确中英日文字，不把任意空行当成分隔行。

维护目标的 `--source-fixture <absolute-keymapdata.ini>` 只允许仓库 out/mcp_product 下已存在的固定源夹具，通过对应 Qt 的 QSettings 写入合成 CommonMappingData QVariantList。该普通权限准备发生在产品启动之前，保留跨配置切换后的未附着共通内存场景；不写个人配置或提供产品加载入口。

```powershell
out/mcp_bridge/venv/Scripts/python.exe -X utf8 mcp_bridge/tests/test_source_interop.py --executable $qkmProductExe --output out/mcp_product/accept-source-qt6 --test-exe out/tests/mcp_control/qt6-release/release/QKeyMapperMcpControlTest.exe --qt-bin $qkmKitBin
if ($LASTEXITCODE -ne 0) { throw 'Source acceptance failed; inspect logs and owned pairing state.' }
```

先经受管run或按公共接入说明登记已有授权，省略旧`--gsudo-cache`。Qt5 使用其匹配的产品、维护 EXE、Qt bin 和全新 output。固定助手通过 UIA 选择 Tab／行，在已核实单元格内投递成对右键窗口消息、调用真实菜单“上移”，再以实际 GUI 行及正式 MCP 源行验证效果；不调用业务函数或设置配置值。单纯查看其他 Tab 不应改变源引用；重排往返、重载、配置切换／失败／恢复及重新配对必须拒绝旧引用。读取批次核对 GUI、实际源内容、dirty／运行摘要与整个 INI 字节不变。

本套件末段现在需要私有桌面Read+Configure及固定共通变更授权：再次配对后预览共通C→B及禁用普通C的联动，先不确认并验证零部分修改，再确认、重放并从GUI读取实际结果；非当前源提交须拒绝且不切Tab。前面的只读批次仍检查dirty／INI不变，新增写批次单独核对预期dirty和INI未写，不能统一声称全程只读。

不使用Default、不移动系统光标或发送系统输入，目标及助手实际桌面写入身份记录；本套件未启动映射，源查询始于初始静止。运行后清理、实际A→B、正式MCP启停及保存冷启动另验。

桌面清理单独回归（普通权限，仅创建／关闭自有桌面，不启动产品）：

```powershell
out/mcp_bridge/venv/Scripts/python.exe -X utf8 mcp_bridge/tests/test_desktop_lifecycle.py -v
```

该回归检查显式关闭后必须解除 Python 对象对原生句柄的持有，旧对象释放不能误关新建桌面；正式 SDK 与产品脚本使用同一清理函数。

## 保存预览存储组件（SAVE-94）

定向运行mappingChangeArguments／mappingChangeLifecycle／mappingChangeLimits及configurationSaveLifecycle／configurationSaveExpiry／configurationSaveLimits。后三项验证保存绑定仅存明确组、修订、context／scope及原单调截止，创建无效绑定不替换旧预览；保存／映射共用替换、取消和单次消费，operation只报告所属实例／组／修订，不泄漏内部context／description。到期用测试访问器设置截止，不实等120秒。

容量夹具创建64个合成applying记录，核对无空结果槽时begin拒绝且保留待消费预览，原记录不被淘汰；模拟历史期经过仍保留进行中结果，完成后重新起算结果期限；另核对32预览上限。此为无业务动作的内存组件，不冒充正式保存、断线恢复或实际异步任务。clear仍按既有接口显式重置全部内存，调用方不能把它当任务取消或完成。

仅上述纯存储槽可普通权限在私有桌面运行，无需gsudo、产品或IPC子进程；initTestCase仍要求QKM_IPC_TEST_EXE指向匹配Qt的既有EXE并验证当前进程身份。完整UI用例仍遵循本文件的真实窗口规则，不由纯组件通过推定已验。正式保存接线前还须分派动作类型、解释结果槽不足、重新校验真实授权／业务内容并协调生命周期。

SAVE-95另选`savePreviewArguments`，与原映射／保存存储六槽合跑：明确configuration.save／setting_name、类型／未知字段／混合参数、长度及只读能力声明。生产SDK接线见第一方测试README与唯一主计划；组件通过不替代正式产品业务或实际保存。SDK发布的oneOf／原映射兼容由桥接侧固定端点和真实stdio维护接线分别验证。
