# ENGINE-61 原功能及 Release 对照

唯一进度与授权范围见 [主计划](../../../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md)。本目录是维护测试，不是产品控制接口；不修改产品或 manifest，不安装驱动。使用仓库锁定的 x64 Python 3.11.9／venv，不使用 `-O`；正式入口会在解析参数和启动任何测试前拒绝优化模式，以免安全断言被移除。

自测（私有桌面，仅窗口消息，不注入系统输入、不启动产品或 UAC）：

```powershell
& out/mcp_bridge/venv/Scripts/python.exe -B -m unittest discover -s QKeyMapper/tests/input_regression -p test_probe.py -v
```

正式测试必须已获 Default 前台／真实键盘和固定 High 助手范围授权，并经[受管run或公共目标登记](../../../.agents/skills/qkeymapper-gsudo-cache/references/test-actions.md)复用工作区缓存。旧--gsudo-cache已拒绝，不再创建自己的控制器。先将同工具链的普通 Qt6 Release 与依赖部署到以下独立模板目录；baseline 是固定 d49a9f57，current 是当前产品。每态将模板复制到新输出目录内的独立 runtime，写入相同合成 INI；不复用失败测试的配对命名空间，不能使用个人运行目录。

```powershell
& out/mcp_bridge/venv/Scripts/python.exe -B QKeyMapper/tests/input_regression/run_acceptance.py `
  --baseline out/mcp_product/engine61/runtime-baseline/QKeyMapperMcpAcceptance_engine61_baseline.exe `
  --current out/mcp_product/engine61/runtime-current/QKeyMapperMcpAcceptance_engine61_current.exe `
  --output out/mcp_product/engine61/accept-unique `
  --allow-default --allow-focus-click
```

默认依次测 baseline／off／idle／query，每态三轮 250 对 A 按下／释放；idle 表示 MCP 已启用且配对、没有查询，四态均运行相同 A→B 映射。`--states off --rounds 1 --pairs 10` 可做先行检查。结果核对实际 B 计数／顺序／释放、停止后 A 恢复；端到端时延包含调度、Hook、产品和接收窗口，不是纯引擎耗时。运行态压力仅交替读取状态／Tab，最高约 20 次／秒，不访问要求初始静止或当前完整清理许可的源表。

MAP-67-B1之后，query态还需Read+Configure及下述固定GUI变更授权：停止态GUI删除唯一合成A→B，再由正式MCP预览／提交恢复；完整字段／派生值相同，INI保持未保存。独立悬浮编辑窗阻止预览／提交而允许取消；GUI仅改同步组0→1使旧预览陈旧且零部分修改，再还原0。正式输入收到实际B并确认Worker started后，核对GUI添加禁用、MCP写入拒绝及停止清理后的源不变。RunMapping未授予，开始／停止仍用获批GUI动作。此设置阶段位于性能采样之外；不能拿GUI启停当MCP启停闭环通过。

query态用普通capture_ui_snapshots.ps1进行Default屏幕复制，自主检查A／B及未保存界面。display-*.json仅记录UTC、实际输入桌面、Windows逻辑视图／主模式及自有窗口DPI／前台，不判断物理显示器电源。记录失败标明UNVERIFIED，不妨碍输入或进程清理；不改变显示设置或唤醒显示器。

进程匹配使用 `python.exe` 路径后缀加精确接收窗口标题；产品的“等于”比较完整进程路径。停止使用隔离配置的实际 F24 快捷键，核对 A 恢复后再独立观察 GUI 空闲；不据按钮名称断言完成。

每次发送前核对自有接收窗口身份、前台和修饰键；失焦／关闭／Esc 终止发送，清理只释放测试成功按下的源键，不强占焦点重试。`--allow-focus-click` 需另获自有接收区真实左键点击授权；仅阶段开始且普通激活失败时，对经过身份及命中检查的固定接收区点击一次，恢复原光标位置，绝不用于运行中失焦后抢回。固定助手在启停观察后最小化自有产品，记录实际结果。产品使用原 manifest 和 SendInput 后端，实际启动／停止仍可能执行原有驱动复位及 FakerInput ReleaseAll。控制器、SDK、接收、采样、注入和截图保持普通权限；必要启动／关闭和固定GUI经工作区受管控制器的实际调用链；默认五分钟、最长24小时和最终硬截止由技能管理，不创建嵌套缓存或使用全局关闭。

原始事件及采样留在 out，不同步 Git。线程数不代表唤醒次数；基础 A→B 不能替代连发／锁定／序列／设备等验收，也不能代表完整 ENGINE-61 通过。

CPU 归因可显式加 `--thread-cpu`，每秒以普通查询权限记录自有产品线程的累计 CPU（100 ns）、创建时间和有界名称；读取失败单列，不当成零。使用 TID＋创建时间区分线程复用，结果还提供 GUI 线程 ID；退出／新建线程间的遗漏和各次采样时刻差异需与进程总 CPU 对照。默认关闭此观测，前后对照使用相同选项；它不采集实际线程唤醒，也不调整优先级、亲和性或系统设置。

固定原功能用例（当前仅对照 baseline／MCP 关闭；需同一 Default／键盘授权）：

```powershell
& out/mcp_bridge/venv/Scripts/python.exe -B QKeyMapper/tests/input_regression/run_acceptance.py `
  --baseline out/mcp_product/engine61/runtime-baseline/QKeyMapperMcpAcceptance_engine61_baseline.exe `
  --current out/mcp_product/engine61/runtime-current/QKeyMapperMcpAcceptance_engine61_current.exe `
  --output out/mcp_product/engine61/features-unique --states baseline off `
  --cases hold_stop burst_release burst_stop lock_toggle_stop sequence_complete sequence_cancel stop_message `
  --allow-default --allow-focus-click
```

每个用例使用新的产品和接收窗口：先验证 A→B，再检查固定功能的实际输出及停止后 A 恢复。连发核对 400 ms 按住／100 ms 释放的实际时序；锁定核对原键释放后仍按住、再次切换及停止释放；序列检查 200 ms 等待，取消后观察超过原 2 秒期限。停止发送仅用 SendMessage E⏱250，按实际接收进程独有的窗口类精确匹配，检查没有重复停止输出。仍按住原键的既有恢复事件全部保留，不能过滤掉后冒称没有输出差异。

`feature-events.json` 留存原始输入、清理释放的源键和全部接收事件，`stop-tail.json` 单列完成用例后的停止清理。开始前要求测试源键和 B–E 均未按住；每个接收窗口至多一次获批的焦点点击。经受管run时，普通子测试通过公共请求复用控制器，不传 `--gsudo-cache`，不自行创建或关闭缓存；其进程、输入与窗口清理仍照常执行。

`hold_stop` 默认不执行新增标签输入。仅在另获 WRITE-64 明确授权后传 `--cases hold_stop --allow-late-forced-up`：正常停止并核实 GUI 空闲后，重新按住自有测试 A，发送一次带该实例实际 B 输出标签的 A 强制抬起，要求 A 仍保持按住；最后释放 A 并核对事件。标签必须来自该轮 warmup，失焦中止、不增加焦点点击额度；结果无论成功或失败均保存于 `late-force-release.json`。该标志只允许配合单个 hold_stop 用例，函数入口也默认拒绝；普通原功能回归和仅批准 Default 输入不能替代此新增标签授权。

`--cases tab_return profile_return quick_restart exit_wait` 补充回切与生命周期：F18／F17 逐次切入 A→C／A→B 的两个 Tab；仅改变自有接收窗口标题，验证自动匹配配置进入通用 A→C 后 F24 恢复 Engine61；F17／F24 连续四轮按住、停止和重启，逐轮核对实际 B 释放。`exit_wait` 在 F16 的 C⏱2000»E 已按下 C 时正常关闭产品，接收器继续观察超过 2 秒，要求一次抬起、无 E、进程正常退出及退出后 A 直通。F17 是这些夹具明确配置的开始／Tab 快捷键，不注入基础夹具的 F23。

当前版 `off_quick_restart`／`off_profile_return` 在上述 MCP 关闭的真实输入阶段完成后，另外启用接口并批准只读配对，执行 STOP-57 正式 SDK 检查：完整清理及配置回切后读取 A→B、只读批次不变性、GUI 再次启动时拒绝源查询、再次停止后清理代次更新及旧引用失效。此后段只复用已批准的固定 GUI 启停／配对动作，不再注入系统输入；不能把后段当成 MCP 关闭状态的测试。结果在 `sources-after-stop.json`，最终撤销配对并关闭接口；只读许可不代表编辑许可或正式异步停止操作结果。

运行失败须保留原始轨迹和退出记录，不过滤重复抬起或因基线也失败而放宽断言；当前待处理问题和实际覆盖以唯一主计划为准。这些固定用例不会修改用户配置或替代正式 MCP 业务等价性测试。
