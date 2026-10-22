# MCP 管理窗口验证

按产品高 DPI／Fusion／字体基线，使用产品同一个 `ControlDialog`，通过 Qt 界面事件点击实际复选框指示区／按钮，再由独立 IPC 测试子进程完成正式配对证明。读取／批准／撤销断言检查真实控制器及私有凭据，不以控件值代替结果。没有产品业务回调；测试的 fixture 回复不代表产品能力。

在忽略的独立目录对 `mcp_control_test.pro` 运行相应 Qt 的 qmake 和 JOM。运行时 PATH 加入对应 Qt `bin`，`QKM_IPC_TEST_EXE` 指向相同 Qt 的 `mcp_ipc_test.pro` 产出的绝对 EXE。两个维护目标均为 asInvoker，无需修改产品 manifest。DPAPI 需要可用的当前用户环境；本机 Codex 默认沙箱的限制与批准的非提权运行方式见[验证记录](../../../handoff/qkeymapper_mcp_control/validation.md)。

测试窗口需要真实显示；优先在私有 Windows 桌面运行，**不调用 SwitchDesktop**。可以设置 `QKM_TEST_DESKTOP`，测试将检查自身线程实际桌面名。Qt 事件直接投递目标控件，不发送系统键鼠输入。关闭并清理自有窗口和子进程，不关闭用户实例。

覆盖默认关闭／不创建凭据目录、启用时发布并核实发现文件、中英日标题及明文标签、Enter 不默认批准、只读默认值、额外配置／启停授权、配对批准、拒绝、撤销、关闭窗口保留接口、关闭／Escape 时拒绝待批准请求和启用失败回显。关闭接口时通过真实控件查看／撤销，核对实际持久化和重新打开结果；只读保存失败与另一活跃实例占用不得假报成功。`queryProcessExecutable` 还检查真实自身路径、错误创建时间及无效身份拒绝。

`--preview <language:0|1|2> <off|pending|paired-off|busy-off> <absolute-state-json>` 仅用于维护目标的静态截图：真实显示同一控件，原子报告 PID／HWND／实际桌面、原生可见性、Qt／原生 DPI、样式／字体及接口／配对状态；20 秒后自动退出，也可从该私有桌面发送 WM_CLOSE。pending 含 `<b>` 和实际自身进程身份，用于检查纯文本；paired-off 使用假配对记录，busy-off 再取得真实租约并点击撤销制造占用提示。四种预览均不启用接口、批准或认证客户端，截图不能证明配对安全。使用仓库 `scripts/capture_ui_snapshots.ps1 -ProcessId ... -WindowHandle ... -PrintWindow` 后自主审阅。

当前验证覆盖独立窗口、产品 Qt6／Qt5 编译及同源维护窗口的正式 SDK 配对；产品启动、通用高级设置／托盘入口的实际操作、管理员产品到普通桥接及业务等价性仍须后续集成验收。没有 Default 桌面、真实输入、驱动或性能结论。

## 可移植执行入口

`test_mcp_control.py` 只依赖 Python 3.10+ 标准库；不切换输入桌面，检查父／子进程均未提权，报告目标自身实际桌面。输出必须位于本仓库 `out/`。`capture_mcp_control.ps1` 复用仓库截图脚本，在同一私有桌面核对 HWND 所有者并关闭自有预览窗口。不要用 `python -O`；脚本拒绝关闭断言的优化模式。

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
