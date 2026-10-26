# 第一方测试

维护中的第一方测试按主题放在 `tests/<主题>/`，对应的 `.pro`、测试代码和专用脚本相邻存放。新增测试沿用这一约定，不在产品目录根部增加 `*_test.pro`。通用构建、静态分析和进程管理工具继续放在仓库 `scripts/`；第三方测试保持上游目录结构。

| 主题 | 工程与运行入口 | 用途 |
|---|---|---|
| [ui_scale](ui_scale/) | `ui_scale_test.pro`、`test_ui_scale.ps1` | 缩放断言与状态恢复 |
| [sendinput_lifecycle](sendinput_lifecycle/) | `sendinput_lifecycle_test.pro` | 实际 SendInputTask 的等待、暂停、取消和退出 |
| [diagnostics](diagnostics/) | `diagnostics_test.pro`、`run_checks.ps1`、`measure_overhead.ps1` | 原生诊断策略、符号、故障注入及进程开销采样 |
| [mcp_ipc](mcp_ipc/) | `mcp_ipc_test.pro` | MCP 本机 IPC 消息边界、真实进程身份、listener 生命周期／读缓存／认证前期限与拒绝验证 |

所有构建、生成文件、日志和运行副本放在仓库忽略的 `out/tests/`。不同 Qt、配置和 ASan 使用独立子目录；一次性调查探针留在 `out/`，不加入维护测试。Qt Creator 使用影子构建，个人 `.pro.user*` 不提交。

## 构建与运行

以下命令从仓库根目录执行。qmake/JOM 构建命令需要已加载 x64 MSVC 环境；可先在开发者终端执行 `vcvars64.bat`。只构建不会运行测试：

```powershell
$repoRoot = (Get-Location).Path
$qmake = 'C:\Qt\6.8.3\msvc2022_64\bin\qmake.exe'
$jom = 'C:\Qt\Tools\QtCreator\bin\jom\jom.exe'
$buildDirectory = Join-Path $repoRoot 'out/tests/sendinput_lifecycle/qt6-release'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
Push-Location $buildDirectory
try {
    & $qmake (Join-Path $repoRoot 'QKeyMapper/tests/sendinput_lifecycle/sendinput_lifecycle_test.pro') 'CONFIG+=release' 'CONFIG-=debug'
    if ($LASTEXITCODE -ne 0) { throw 'qmake failed.' }
    & $jom -j4
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
}
finally { Pop-Location }
```

另两个目标使用同样方法，替换工程路径并分别使用 `out/tests/ui_scale/qt6-release`、`out/tests/diagnostics/qt6-release`。生命周期测试继续包含真实产品入口并保留正式管理员 manifest；其工程在本地解析继承的产品文件路径，不复制产品源码清单。

完整运行 UI 缩放断言（会创建测试窗口）：

```powershell
& .\QKeyMapper\tests\ui_scale\test_ui_scale.ps1
# Qt5 或 ASan：沿用 -QtRoot、-AddressSanitizer 参数，并使用独立构建目录。
```

诊断检查（默认包括原生自检、符号和导入检查；故障注入需显式 `-Runtime`）：

```powershell
& .\QKeyMapper\tests\diagnostics\run_checks.ps1 `
    -QMakePath 'C:\Qt\6.8.3\msvc2022_64\bin\qmake.exe' `
    -MakePath 'C:\Qt\Tools\QtCreator\bin\jom\jom.exe' `
    -BuildDirectory '.\out\tests\diagnostics\qt6-release'
```

生命周期目标使用 `QKM_LIFECYCLE_CASE` 选择用例、`QKM_LIFECYCLE_RESULT` 指定结果 JSON，缺少这两个值会拒绝运行。现有用例为 `wait_clear`、`pause_clear`、`queue_keyup`、`cancel_reset`、`pause_continue`、`break_one`、`break_all`、`stop`、`restart`、`admission`、`exit_wait`、`exit_pause`、`borrowers`。在 `out/tests/` 准备带依赖和隔离 INI 的运行副本后，通过已有进程工具启动；核对 PID、EXE、启动时间及实际桌面，不操作个人实例。

宏循环回归使用同一目标的 `repeat_*` 用例，完整列表及固定预期见测试源码 `repeatSpecs()`。覆盖宏自身 `x2` 与外层 1／3／无上限循环、通用宏、单步宏、OnlyOnce 嵌套与空结果、异常宏、有限次数上界、按住循环和中断后重新触发。测试以无输出按键运行真实 SendInputTask；`repeatTrace` 记录已执行步骤，`repeatRounds` 记录整轮完成，`repeatRuns` 记录中断后的重新触发。有限次数上界通过预置接近上限的计数验证，不实际执行数千万轮。此测试不证明实际键盘、Hook 或驱动输出。

`queued_stop_action` 在 STOPPED 状态通过停止动作使用的真实 wrapper／排队接收入口提交一对无输出 DOWN／UP；先确认派发前控制器表为空，再用 `waitForDone(0)` 确认队列派发后仍有未完成任务，最后沿原回收路径等待后释放合成源数据。它证明 STOPPED／空表不能作为清理完成条件；用例主体不调用 Hook 启停或原生设备清理，真实产品 main 正常退出仍保留原 UnHook／shutdown 路径。不以此代替 GUI 停止、实际设备释放及完整生命周期协调验收。结果同时记录产品主线程实际桌面。

连续存续计数通过同一目标验证：`queued_stop_action` 观察排队到 runnable 的标记交接；`queued_admission` 覆盖退出专用入口拒收已排队请求；`discard_dispatch` 覆盖接收器销毁时丢弃排队事件；`clear_queue` 覆盖未运行任务被线程池删除；`direct_task_gate` 验证直接构造任务也会关闭首次静止源读取门禁，删除任务后不重开。`workAtSubmission`／`workAtBarrier`／`workAtSecondBarrier`／`workAfterQueueClear`／`workAfterJoin` 只在对应分支采样，`sourceGateBefore`／`sourceGateAfter` 在 direct_task_gate 和两个按钮排队用例采样，其他分支的初始值不代表相应事实；`workAtExit` 是所有用例退出后的实际计数。计数不是完整停止完成条件，仍需生产者封闭、线程池排空、设备及 GUI 尾部证明。

`queued_button` 从 queueVButtonKey 排队一对无输出按钮事件，入队即关闭首次源读取门禁并持两个存续标记；第一 Worker 栅栏观察按钮处理产生的待派发发送，第二栅栏观察真实未完成发送任务，沿原路径 join 后才释放合成源表。`button_admission` 先开启退出专用拒收，再排队带 Lock 的按钮，确认两个栅栏均无发送、LockState 保持关闭、标记归还。`buttonLockAfterDispatch` 仅在这两个用例采样；组件调用不是独立 GUI 点击／实际输出基线，也不证明日常停止或快速重启已安全。

`lifecycle_dispatch`／`lifecycle_discard` 验证 Worker 和 Hook 所有者的六种启停／重启信号；每对同类请求共享一个存续标记，三对排队时计数为 3。用例临时将原接收槽换成同一 Worker 线程中两个接收器的观察槽，前者确认最后一个相关事件处理前标记持续存在、全部派发后归零；后者确认只销毁其中一个接收器仍保持计数 3，再销毁另一个才归零，随后恢复原连接。`lifecycleDispatchTrace` 在这些分支采样，`workAfterQueueClear` 在此丢弃分支表示只删除首个接收器后的计数。这是实际 Qt 信号封装／丢弃验证，主体不执行启停业务或 Hook／设备操作，不是独立 GUI 基线、停止代次或完整清理完成证明；真实产品 main 正常退出仍保留原清理。

`owner_fence`／`owner_fence_cancel` 在实际 Worker／Hook／Interception 所有者上验证带代次的队列确认。用例延后 Hook 队列，连续请求两代，等待其余两端实际确认并由 GUI 定时器推进，再释放 Hook；旧代不完成新请求，全部确认仅完成一次。取消分支先取消第二代，观察晚到的 Hook 确认被忽略，再请求第三代并完成。`fence*`／`ownerFenceTrace` 仅在这两项采样；InterceptionContextAvailable 只证明 context 是否存在，不能据此声称收到真实 stroke 或转发已经验证。3 秒失败看门狗释放自有测试阻塞，不启动映射、不安装驱动或发送输入。

InputOwnerFence 只确认各队列到达对应位置；Interception 接收循环中的确认延到当次迭代退出、收到的 stroke 作用域释放及原转发调用返回后。未运行该路径时仅为源码／构建证据。确认不包括子队列、发送池、GUI 尾部或输入准入封闭，不能当完整停止完成。`session_lock_queue` 在 GUI IDLE／Worker STOPPED 下排队一对合成锁定／解锁通知，检查提交计数 2、处理后 0、原始输入记账清除及首次源读取门禁关闭；不锁定 Windows、不验证映射中的锁屏恢复或 FilterKeys 系统效果。

`burst_timer_cleanup`／`burst_timer_missing_source`／`burst_timer_removed_source` 创建一对真实连发定时器，分别保留原表、改用其他表、清空原表，再经原 Worker 停止入口核对两个 QPointer 均已失效且两个定时器表为空。`burst*` 字段仅在这三项采样；索引为空不算销毁证明。采用无系统输出的 NoOp 映射、长于用例的定时器间隔；失败基线也主动回收孤立定时器，排空真实发送任务并补测例 KEY_UP 后才释放合成数据。它不是实际 GUI 切表／删行或物理连发释放基线；Worker 停止主体和真实 main 退出仍保留原有设备清理。

`drain_wait`／`drain_timeout`／`drain_supersede`／`drain_shutdown` 使用真实排队的 250 ms NoOp DOWN／UP 测试异步发送排空观察：完成须同时看到两类作用域为零、所有单键和全局池 waitForDone(0) 成功；等待时 GUI 心跳继续。25 ms 超时不停止或删除任务／控制器，随后新代继续观察到排空；替代请求报告旧代 superseded、忽略旧请求，退出专用关闭先报告 shutdown 再按原逻辑释放资源。`drainTrace` 是实际代次／结果轨迹，`drainResourcesIntact` 只在三个非成功通知时采样，`drainFinalPoolsFinished` 只在 drained 通知时采样；shutdown 不把默认值当作 drained 通过。观察本身不封闭新输入、不修改资源，也不包含所有者／GUI 边界，不能独立作为业务停止完成或编辑授权。

观察期限在 Worker 事件循环中检查，不抢占原有阻塞调用；整体停止作业仍需 GUI 协调器独立计时并保持超时禁写。

`idle_input_tracking` 在 GUI 所有者线程临时设置合成 IDLE／CHECKING 和缺失 Tab，直接调用 Interception 键盘处理函数回放一对 A DOWN／UP，核对设备、原始及去设备记账均正确添加／清除，并立即恢复索引和状态。IDLE 的真实记账信号直接观察 `inputProducerTrace=1/1`，每次回调返回后计数为 0；CHECKING 不发该信号。`queued_stop_action`／`queued_admission`／`discard_dispatch` 也在真实发送信号处核对包装函数的在途作用域（`producerSubmissions=2`），前者在 NoOp 发送任务等待期间观察 `producersWhileTaskWaiting=1`，两个负例为 0。`producersAtExit` 是所有分支实际退出后计数，其他新字段仅在上述分支采样；嵌套同步作用域计数不等于线程数或停止完成。

这些用例不经 Interception 驱动接收／转发、不发送系统输入；合成状态不是 GUI 监视或真实硬件基线。Interception 接收循环的作用域位置及空接收不计数通过源码核对，鼠标／低层 Hook 回调及实际驱动转发仍须相应输入场景验证，不能由此直接记为运行通过。

性能基准使用非 diagnostic 的普通 Release。生命周期测试的普通版保留原等待与 cleanup 耗时门槛；diagnostic 写盘和 ASan 用于完成／销毁及内存安全检查，不用于判断日常性能。运行器须保留每项30秒看门狗；diagnostic 的 `borrowers` 在内部等待全部调用完成。

源码、Git、构建和分析保持非管理员。只有保留正式 manifest 的必要测试启动／关闭使用已授权 gsudo，并由用户确认 UAC。无输出生命周期用例不证明硬件、Hook 或驱动释放。窗口测试和完整阶段门禁按仓库工作流另行验收。

静态报告回归入口仍为 `scripts/test_static_analysis.ps1`，只验证目标发现和报告门禁，不代替真实 Clang-Tidy／Clazy。

## 配置参数保存及独立冷读

`settings_profile_save`／`settings_profile_cold` 使用同一生命周期目标和正式manifest。仅在新建的自有 `out/` 运行目录中部署匹配的EXE／Qt依赖，通过缓存技能公共登记入口启动；不改变正式manifest或调用任意高权限脚本。基础样例为 [profile_save_fixture.ini.example](sendinput_lifecycle/profile_save_fixture.ini.example)，复制为运行目录的keymapdata.ini，首次由正常产品启动加载Profile74Seed，不直接调用加载函数。

非默认组把 [profile_save_nondefault.ini.example](sendinput_lifecycle/profile_save_nondefault.ini.example) 的36项放入该样例的Profile74Seed节末、Keep74节之前，并设置 `QKM_LIFECYCLE_PROFILE_NONDEFAULT=1`；默认组不添加且设为0。准备发生在产品启动前，禁止覆盖个人INI。保存用例通过Qt键盘事件输入新名称、真实鼠标事件点击Save，核对原GUI写盘／重载与其他配置保持；冷读必须在保存进程正常退出后，以同一EXE和已保存INI启动另一进程，省略任何重新准备／覆盖INI步骤。

每次设置 `QKM_LIFECYCLE_CASE` 为对应case，`QKM_LIFECYCLE_RESULT` 为新的绝对JSON路径；结果在既有 `settingsGeneral` 容器，不是正式MCP结果。另检查主结果passed、实际桌面、PID／EXE／创建时间、High／同登录关联、原生exit0、workAtExit／producersAtExit为0，并在冷启动前后从进程外比较INI字节。没有映射启动或系统输入，正常main退出仍保留既有设备清理，不证明实际驱动释放。

为提取前／后对照，先在未改GUI保存实现的二进制上建立基线，保存其INI为运行目录父级下的 `baseline-*.ini`。提取后在同路径／同名称运行副本重新从相同样例开始，设置 `QKM_LIFECYCLE_SAVE_BASELINE` 为该已有绝对路径；用例独立解析完整键值、比较INI字节，并验证36项收集的类型／顺序／重复性及disk／dirty／加载／源代次不变。默认与非默认、两个Qt kit分别保留证据，不用其他PC的out代替本机基线。选定执行及未覆盖范围见主计划与C1验证记录。

## 窗口匹配保存及条件键保留

`settings_match_save`／`settings_match_cold` 复用上述目标、正式manifest、私有桌面和公共缓存入口。使用 [window_match_save_fixture.ini.example](sendinput_lifecycle/window_match_save_fixture.ini.example)，先把 `__RUNTIME__` 替换为自有运行目录的正斜杠绝对路径，并将仓库 `QKeyMapper/image/button_image/gamepad_32px.png` 复制为该目录的 `incoming75.png`、`kept75.png`。在产品未启动时按下表准备每个独立运行目录的INI，设置 `QKM_LIFECYCLE_MATCH_VARIANT`；不修改个人文件。

| variant | 样例调整与期望 |
|---|---|
| metadata | 原样加载；保存为Map75Profile，写入当前完整路径与存在的图标 |
| keep_path | 删除Profile75Seed的FilePath及CustomIconPath；另加Map75Profile，保留同样的六项映射／安全键，FilePath设为C:/QkmProfile75/keep.exe，CustomIconPath设为运行目录的kept75.png。GUI覆盖确认后保留旧FilePath，显式清空图标 |
| draft_fallback | 只删除Profile75Seed的FilePath及CustomIconPath；用例以真实ComboBox键盘激活回到空白草稿，再输入带首尾空格的fixture75.exe，验证仅回退路径去空格 |
| global | SettingSelect及Profile75Seed节名都改为QKeyMapperGlobalSetting；保存全局配置，原窗口匹配键保持 |
| missing_icon | 只删除Profile75Seed的FilePath；另加与keep_path相同的Map75Profile。用例在正常加载后仅删除自有incoming75.png，覆盖保存保留目标旧路径与图标 |

每个warm用例通过Qt事件输入名称并点击真实Save；已存在目标时点击真实模态Yes按钮。cold必须在保存进程正常退出后，用同一EXE／INI启动另一进程，禁止再次准备INI或图片。两种Qt分别建立未提取的GUI基线；提取后同路径重建初始夹具，设置 `QKM_LIFECYCLE_SAVE_BASELINE` 比较完整INI字节，并验证窗口匹配收集的重复性、Qt类型、条件键数量和磁盘／dirty／代次不变。这里不启用映射、不发送系统输入，不证明正式MCP保存或设备释放完成。
