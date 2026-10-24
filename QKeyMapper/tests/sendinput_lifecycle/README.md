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
