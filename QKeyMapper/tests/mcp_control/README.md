# MCP 管理窗口验证

按产品高 DPI／Fusion／字体基线，使用产品同一个 `ControlDialog`，通过 Qt 界面事件点击实际复选框指示区／按钮，再由独立 IPC 测试子进程完成正式配对证明。读取／批准／撤销断言检查真实控制器及私有凭据，不以控件值代替结果。没有产品业务回调；测试的 fixture 回复不代表产品能力。

在忽略的独立目录对 `mcp_control_test.pro` 运行相应 Qt 的 qmake 和 JOM。运行时 PATH 加入对应 Qt `bin`，`QKM_IPC_TEST_EXE` 指向相同 Qt 的 `mcp_ipc_test.pro` 产出的绝对 EXE。两个维护目标均为 asInvoker，无需修改产品 manifest。DPAPI 需要可用的当前用户环境；本机 Codex 默认沙箱的限制与批准的非提权运行方式见[验证记录](../../../handoff/qkeymapper_mcp_control/validation.md)。

测试窗口需要真实显示；优先在私有 Windows 桌面运行，**不调用 SwitchDesktop**。可以设置 `QKM_TEST_DESKTOP`，测试将检查自身线程实际桌面名。Qt 事件直接投递目标控件，不发送系统键鼠输入。关闭并清理自有窗口和子进程，不关闭用户实例。

覆盖默认关闭／不创建凭据目录、启用时发布并核实发现文件、中英日标题及明文标签、Enter 不默认批准、只读默认值、额外配置／启停授权、配对批准、拒绝、撤销、关闭窗口保留接口、关闭／Escape 时拒绝待批准请求和启用失败回显。关闭接口时通过真实控件查看／撤销，核对实际持久化和重新打开结果；只读保存失败与另一活跃实例占用不得假报成功。可见选中项与键盘导航 currentRow 不同时须撤销选中客户端，无选中不得撤销；通过独立读取存储核对剩余 pair ID。`queryProcessExecutable` 还检查真实自身路径、错误创建时间及无效身份拒绝。

`--preview <language:0|1|2> <off|pending|paired-off|busy-off> <absolute-state-json>` 仅用于维护目标的静态截图：真实显示同一控件，原子报告 PID／HWND／实际桌面、原生可见性、Qt／原生 DPI、样式／字体及接口／配对状态；20 秒后自动退出，也可从该私有桌面发送 WM_CLOSE。pending 含 `<b>` 和实际自身进程身份，用于检查纯文本；paired-off 使用假配对记录，busy-off 再取得真实租约并点击撤销制造占用提示。四种预览均不启用接口、批准或认证客户端，截图不能证明配对安全。使用仓库 `scripts/capture_ui_snapshots.ps1 -ProcessId ... -WindowHandle ... -PrintWindow` 后自主审阅。

已验证范围包括独立窗口、产品 Qt6／Qt5 编译、同源维护窗口 SDK 配对，以及真正管理员产品的入口／默认绑定、只读批准／RPC、重连、撤销／关闭和冷启动拒绝旧 key。状态／目录与加载上下文的新增场景按[验证记录](../../../handoff/qkeymapper_mcp_control/validation.md)逐轮列明，不能将旧通信结果当作全部业务验收。普通截图无法捕获 High 产品，该部分未算视觉通过；没有 Default 桌面、真实输入、驱动输出、托盘或性能结论。

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

已获本轮管理员启动／关闭及受限 GUI 助手授权、用户可以确认 UAC 时运行。开发工具、控制器、SDK、桥接和截图保持普通权限；不修改产品 manifest。`product_lifecycle.ps1` 只管理自有隔离产品，`product_ui_actions.ps1` 只允许高级设置／MCP、启用、只读批准、撤销、关闭的固定 GUI 动作，核对 EXE／PID／启动时间及实际私有桌面。OpenAdvanced 先选映射页读取实际停止快捷键／开始按钮、Tab1 和配置选择显示，再选通用页打开高级设置，避免读取隐藏控件；配置选择使用 UIA ValuePattern，Name 是可访问名称，不能当作当前选中内容。不更改配置、不启动映射或发送系统输入。正常产品启动仍包含对已安装 FakerInput 的连接尝试。

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

已明确获得临时缓存授权时，连续固定动作推荐在命令末尾加 `--gsudo-cache`，避免十多次逐条 UAC。它只允许当前普通测试控制器 PID 及其子进程使用缓存，空闲 5 分钟失效，并在 `finally` 中关闭及核对不可用；不改变全局 CacheMode，不提升 SDK／桥接／截图。默认不启用。结果及原始日志分别记录缓存模式、启用与关闭，真实产品仍独立核对 High／Medium、系统关联登录及实际私有桌面。PC-W11 的同 EXE／同合成 INI 无缓存与缓存 11 分项、状态／目录业务字段对照见 CACHE-40；这不证明逐次 UAC 提示／拒绝、焦点或等待耗时相同，UAC 交互场景须不使用缓存另验。

正式 SDK 通过固定公开工具验证未批准时不可用、只读权限、实际能力、状态／目录查询、凭据重连、撤销断线／拒绝旧 key；完整退出后再启动同一产品，核对默认关闭及撤销持久化。两次均要求正常退出 0，仅清理自身客户端凭据。Qt5 按控件实际支持的 SelectionItem／Invoke 无障碍模式选择固定目标；没有任意控件或命令入口。服务有 240 秒自有产品看门狗，控制器超时不自动重启。

可用 `--load-fixture blank|saved|invalid|missing` 验证启动映射加载记录。blank 为默认模板；其余要求全新运行目录且 INI 与仓库合成模板完全相同，再由普通控制器在启动前写入测试配置。saved 为一个 A→B 源映射，invalid 含固定非法原始键，missing 指向不存在的配置组；均核对实际加载结果／代次、GUI 选择的实际值、只读不变性和目录分页。

`--load-fixture warm` 额外准备 McpSaved（A→B）、McpSecond（C→D）和 McpInvalid。固定助手只在已核实的自有私有桌面窗口使用配置控件实际提供的 UIA ExpandCollapse.Expand 或 InvokePattern.Invoke 展开菜单，再向其真实弹窗发送 Up／Down／Enter／Escape，激活配置条目；不设置配置值或调用加载函数。展开后按本进程实际五项配置列表定位唯一弹窗，记录并排除没有该列表的 Qt 辅助窗口；不移动系统光标或修改产品焦点策略。先读取原配置，再温切换、空白重置、加载成功→失败→恢复及同项重载。每步独立读取 GUI 表格与配置实际值，正式 SDK 检查加载代次／结果／来源、旧目录失效与新分页，并比较只读请求前后的 GUI、状态、dirty 和整个 INI 字节。每步原始结果为 load_*.json；实际通过范围以主计划／验证记录为准，脚本或语法检查本身不表示运行通过。这里不启动映射，不证明 A→B 输入效果或运行时清理完成。

普通 PrintWindow 无法捕获 High 产品时明确记录未验证，不提升截图权限；功能断言继续独立检查。失败时先看日志、closed.json 和实际进程／配对状态，再决定接续动作；不得盲目重复配对或撤销。

桌面清理单独回归（普通权限，仅创建／关闭自有桌面，不启动产品）：

```powershell
out/mcp_bridge/venv/Scripts/python.exe -X utf8 mcp_bridge/tests/test_desktop_lifecycle.py -v
```

该回归检查显式关闭后必须解除 Python 对象对原生句柄的持有，旧对象释放不能误关新建桌面；正式 SDK 与产品脚本使用同一清理函数。
