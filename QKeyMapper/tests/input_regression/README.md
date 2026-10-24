# ENGINE-61 原功能及 Release 对照

唯一进度与授权范围见 [主计划](../../../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md)。本目录是维护测试，不是产品控制接口；不修改产品或 manifest，不安装驱动。使用仓库锁定的 x64 Python 3.11.9／venv。

自测（私有桌面，仅窗口消息，不注入系统输入、不启动产品或 UAC）：

```powershell
& out/mcp_bridge/venv/Scripts/python.exe -B -m unittest discover -s QKeyMapper/tests/input_regression -p test_probe.py -v
```

正式测试必须已获 Default 前台／真实键盘和固定 High 助手范围授权。先将同工具链的普通 Qt6 Release 与依赖部署到以下独立模板目录；baseline 是固定 d49a9f57，current 是当前产品。每态将模板复制到新输出目录内的独立 runtime，写入相同合成 INI；不复用失败测试的配对命名空间，不能使用个人运行目录。

```powershell
& out/mcp_bridge/venv/Scripts/python.exe -B QKeyMapper/tests/input_regression/run_acceptance.py `
  --baseline out/mcp_product/engine61/runtime-baseline/QKeyMapperMcpAcceptance_engine61_baseline.exe `
  --current out/mcp_product/engine61/runtime-current/QKeyMapperMcpAcceptance_engine61_current.exe `
  --output out/mcp_product/engine61/accept-unique `
  --allow-default --allow-focus-click --gsudo-cache
```

默认依次测 baseline／off／idle／query，每态三轮 250 对 A 按下／释放；idle 表示 MCP 已启用且配对、没有查询，四态均运行相同 A→B 映射。`--states off --rounds 1 --pairs 10` 可做先行检查。结果核对实际 B 计数／顺序／释放、停止后 A 恢复；端到端时延包含调度、Hook、产品和接收窗口，不是纯引擎耗时。运行态压力仅交替读取状态／Tab，最高约 20 次／秒，不访问被 pristine_idle 限制的源表。

进程匹配使用 `python.exe` 路径后缀加精确接收窗口标题；产品的“等于”比较完整进程路径。停止使用隔离配置的实际 F24 快捷键，核对 A 恢复后再独立观察 GUI 空闲；不据按钮名称断言完成。

每次发送前核对自有接收窗口身份、前台和修饰键；失焦／关闭／Esc 终止发送，清理只释放测试成功按下的源键，不强占焦点重试。`--allow-focus-click` 需另获自有接收区真实左键点击授权；仅阶段开始且普通激活失败时，对经过身份及命中检查的固定接收区点击一次，恢复原光标位置，绝不用于运行中失焦后抢回。固定助手在启停观察后最小化自有产品，记录实际结果。产品使用原 manifest 和 SendInput 后端，实际启动／停止仍可能执行原有驱动复位及 FakerInput ReleaseAll。控制器、SDK、接收、采样、注入和截图保持普通权限；gsudo 五分钟缓存仅绑定此独立控制器 PID，finally 关闭并检查不可用，不使用全局缓存关闭。

原始事件及采样留在 out，不同步 Git。线程数不代表唤醒次数；基础 A→B 不能替代连发／锁定／序列／设备等验收，也不能代表完整 ENGINE-61 通过。

固定原功能用例（当前仅对照 baseline／MCP 关闭；需同一 Default／键盘授权）：

```powershell
& out/mcp_bridge/venv/Scripts/python.exe -B QKeyMapper/tests/input_regression/run_acceptance.py `
  --baseline out/mcp_product/engine61/runtime-baseline/QKeyMapperMcpAcceptance_engine61_baseline.exe `
  --current out/mcp_product/engine61/runtime-current/QKeyMapperMcpAcceptance_engine61_current.exe `
  --output out/mcp_product/engine61/features-unique --states baseline off `
  --cases hold_stop burst_release burst_stop lock_toggle_stop sequence_complete sequence_cancel stop_message `
  --allow-default --allow-focus-click --gsudo-cache
```

每个用例使用新的产品和接收窗口：先验证 A→B，再检查固定功能的实际输出及停止后 A 恢复。连发核对 400 ms 按住／100 ms 释放的实际时序；锁定核对原键释放后仍按住、再次切换及停止释放；序列检查 200 ms 等待，取消后观察超过原 2 秒期限。停止发送仅用 SendMessage E⏱250，按实际接收进程独有的窗口类精确匹配，检查没有重复停止输出。仍按住原键的既有恢复事件全部保留，不能过滤掉后冒称没有输出差异。

`feature-events.json` 留存原始输入、清理释放的源键和全部接收事件，`stop-tail.json` 单列完成用例后的停止清理。开始前要求测试源键和 B–E 均未按住；每个接收窗口至多一次获批的焦点点击。若通过主计划当天批准的普通父控制器继承 PID 缓存，子测试不传 `--gsudo-cache`，不自行创建或关闭父缓存；其进程、输入与窗口清理仍照常执行。
