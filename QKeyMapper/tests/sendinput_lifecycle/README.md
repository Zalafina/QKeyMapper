# 发送任务生命周期维护夹具

进度和授权以[唯一主计划](../../../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md)为准。本目标包含真实产品源码，用 `QKM_LIFECYCLE_TESTING` 替换入口并增加固定测试，不向正式产品提供测试控制命令。

使用本机正常 MSVC 开发环境及对应 Qt kit，在 `out/tests/sendinput_lifecycle/<kit>/` 构建 `sendinput_lifecycle_test.pro`，参数为 `CONFIG+=release CONFIG-=debug`，JOM 目标是 `Makefile.Release`；源码列表或继承的 `.pri` 变化后先重新运行 qmake。Qt6／Qt5 与 ASan 使用不同构建目录；编译、Git、控制器及观测保持普通权限，保留原 manifest。

测试控制器先创建并持有私有 Windows 桌面，再通过仓库 `scripts/test_process.ps1` 的 `Start-QkmTestProcess` 启动隔离 runtime。把以下变量作为该子进程的 `EnvironmentOverrides` 传入：

- `QKM_LIFECYCLE_CASE`：固定用例名。
- `QKM_LIFECYCLE_RESULT`：本仓库 `out/` 下独立 JSON 结果文件的绝对路径。

必要启动／关闭遵循已授权的 gsudo 控制器 PID 缓存，默认空闲五分钟、finally 关闭并核实；不提权开发流程或修改 manifest。产品初始化／退出仍可能连接或复位本机 FakerInput 并执行 ReleaseAll，不能把 NoOp 等同于完全没有既有驱动副作用。

WRITE-64 源上下文回归：

| 用例 | 检查 |
|---|---|
| `repeat_source_context` | 全局表为不同的 decoy，真实异步宏／重复子任务始终使用父源并完成规定轮数；数据保留至作业排空 |
| `hold_source_context` | 直接观察保持序列的派生提交及尾键释放，索引、原键和源指针一致，不提交真实输入 |
| `unlock_source_context` | 从父源解锁并读取同一来源的抬起内容，不改变 decoy 的锁状态 |

修复后要求进程 exit 0、`passed=true`、`sourceContextTrace` 全部来源匹配、`workAtExit=0`、`producersAtExit=0`；检查实际桌面／EXE／权限和退出，不能只看计数。修复前对照应单列预期 exit 2 与具体不匹配轨迹，崩溃或超时不是有效红例。相关回归包括 `repeat_macro_3`、`gui_stop_after_timeout`。

NoOp 和 ASan 结果用于指定生命周期／来源边界，不能替代 Default 真实输入效果、普通 Release 性能或其他系统／设备验收。

WRITE-64 停止态释放回归 `inactive_virtual_release`：直接回放键盘 A／鼠标左键的虚拟抬起结构体，不注入系统输入。`virtualReleaseTrace` 共 18 条，格式为 `phase:source_present:virtual_state:keyboard_result:mouse_result`：

- phase 0：GUI 空闲、Worker STOPPED、停止清理已结束；Force／BurstStop 均保护仍按住的原键，不依赖源中是否存在该键。
- phase 1：GUI STOPPING、Worker STARTED、准入关闭；保留冻结源判断。
- phase 2：GUI STOPPING、Worker STOPPED、准入仍关闭；同样保留冻结源，不能只按 Worker 状态当成清理完成。

三个阶段中明确 bypass 均放行，虚拟按键记账均清空、作业计数归零；用 `gui_stop_wait`／`gui_stop_after_timeout` 复核实际停止任务协调。ASan 副本须包含本次编译的测试 EXE，不能用普通版本替代。

MAP-67 共用新增操作维护用例（不代表正式 MCP 验收）：

| 用例 | 检查 |
|---|---|
| `mapping_add_plain` | 预选 Combo 值作为前置条件，再以 Qt 鼠标事件添加 A→B；对照共用提交的完整映射码、派生值、dirty、表格与选区；显式插入、解析负例及组合／长按／双击／序列／停止动作键 |
| `mapping_add_conflicts` | 两个普通源与共通新增冲突；纯分析无修改，GUI 的 No／Yes 按钮独立驱动，取消和拒绝确认均无部分修改；提交结果与通知一致 |
| `mapping_add_guards` | 注入 CHECKING／运行／STOPPING，关闭准入、在途生产者和加载；GUI 事件及共用操作均拒绝且不产生部分修改 |
| `mapping_add_stale` | 分别改变加载代次、结构、冲突、当前目标和运行活动；各测试独立保留其他前提，不让旧 epoch 掩盖实际断言 |
| `mapping_add_setup` | 固定 A→A 输入适配器打开新增本地行，避免误指向追加的共通行；这是适配器检查，不是该入口的完整 GUI 事件基线 |

结果在 `mappingAddition.checks`，所有检查、进程退出及零尾部作业均须通过。用例核对 INI 未变化，拒绝分析／提交时全源字段、派生值、dirty 及通知无修改。正式 MCP 两条独立路径、冷启动、Default 实际输入、打开编辑会话和保存范围仍须在业务接入时验证；源引用与内部决策戳不能作为编辑许可。
