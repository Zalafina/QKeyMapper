# 第一方测试

EDIT-GUI-146：生命周期目标的`settings_original_conflict_gui`使用141样例及固定`normal_duplicate／normal_common／common_no／common_yes／common_disabled_no／common_disabled_yes`。从真实Item输入原键与映射草稿、点击Update和Common模态No／Yes，再Escape；比较确认期、最终全sources、dirty、通知、加载／INI与退出。Common组须让普通表包含Common，初始禁用组仅改合成INI的Common首行；normal_common预置Common原键E，其余用原样例。`showPopup=false`不抑制原键Common确认，不能直接调用原互斥助手建立MCP预览；确认期的内部观察不等于正式MCP查询证据。本用例不发系统输入或启动映射。

EDIT-GUI-144：生命周期目标的`settings_disabled_gui`复用141样例，`QKM_CONTEXT_VARIANT=normal／disabled／common／hidden`。准备各自独立运行副本，disabled只在合成INI设首行初始禁用，common设普通表包含Common，hidden设HideDisabled=true；所有实际开关均经真实Qt Item／表格复选框事件，不能用setChecked／setCheckState或直接调用槽替代。比较完整源／派生／其他行、即时dirty、Escape、隐藏后重新显示、INI／加载及退出排空。它是配置GUI基线，不新增正式MCP字段、映射或真实系统输入验收。

EDIT-143：[普通Release键值更新驱动](../../mcp_bridge/tests/test_product_mapping_update.py)复用独立Item GUI的`settings_edit_gui`／[合成样例](sendinput_lifecycle/mapping_edit_fixture.ini.example)完整sources基线。参数为`--executable`自有隔离产品、`--bridge-python`锁依赖随包桥接、`--gui-baseline`本机141结果目录及`--output`全新证据目录；先按公共入口登记精确EXE／助手摘要和固定GUI动作，不用于用户实例。普通SDK／桥接执行，原manifest产品在私有桌面，GUI只配Read／Configure，验证普通／合法抬起／禁用／Common、取消／重放／陈旧引用／no-op、完整保存冷读及权限拒绝。专属凭据须初始为空，结束后只撤销该轮唯一测试配对；失败先查原请求与实际退出，不盲重放。该驱动不启动映射、不发系统输入、不证明完整GUI／MCP或真实设备等价性。

EDIT-145沿同一驱动加`--disabled-gui-baseline`指向144结果及显式`--case flag_normal flag_cold flag_disabled flag_common flag_hidden flag_conflict flag_common_conflict flag_read`。旧默认键值场景保持；每种cold须在同次运行的对应normal之后，不能拿旧实例warm记录复用。disabled独立布尔预览／取消／提交与144的after_item比较，初始禁用／Common往返再比after_table；隐藏后不自动显示，互斥拒绝不得改他行。完整保存冷读和Read拒绝单列，按实际case输出结果；不要把维护GUI、普通Release或历史设备证据合并成完整运行验收。

IB-MCP-138接纳用例：[普通Release的GUI／正式MCP驱动](../../mcp_bridge/tests/ibinput_acceptance.py)支持gui、mcp、read、sources四组，使用已有公共缓存控制器及固定原生[GUI探针](mcp_control/ibinput_product_gui.pro)，专用EXE／目录、合成INI和凭据，禁止用于用户实例。sources样例见[合成Common／禁用行配置](mcp_control/ibinput_sources_fixture.ini.example)；隐藏冷读组只在自有产品正常退出后修改样例中的HideDisabled，另存原件。GUI基线使用真实窗口消息／UIA动作，不通过产品内设置控件值建立；维护product_check里的直接C++通知和控件设定只作组件验证。

生命周期目标可显式加`CONFIG+=ibinput_output_probe`，仅此维护配置替换IbSendInit／IbSendInput／IbSendDestroy、SendInput／SetCursorPos边界为逐项计数，不执行真实输出。`QKM_LIFECYCLE_CASE=ibinput_output_control`在普通策略下校准11个可达场景，须在读取结果后写同路径`.done`确认退出；只配置演示的settings_save_apply／demo_sequence沿真实入口及正式MCP验证10个有效事务场景和退出计数全零。准入事务由测试访问器播种以排除“陈旧代次导致零调用”；不得称为厂商真实故障、普通Release真实输入或驱动隔离证明。[选定证据](../../handoff/qkeymapper_mcp_control/ibinput_mcp_integration_validation.md)区分上述范围。

维护中的第一方测试按主题放在 `tests/<主题>/`，对应的 `.pro`、测试代码和专用脚本相邻存放。新增测试沿用这一约定，不在产品目录根部增加 `*_test.pro`。通用构建、静态分析和进程管理工具继续放在仓库 `scripts/`；第三方测试保持上游目录结构。

| 主题 | 工程与运行入口 | 用途 |
|---|---|---|
| [ibinput](ibinput/) | client_test.pro、validation_test.pro、ui_test.pro、product_check.pro、keyboard_receiver.pro、ghub_probe.pro | 客户端／能力替身、移动组纯规则、真实Popup组件、产品保存拒绝／Common生效上下文／冷读、显式授权的固定接收；manual_verification.ini无自动映射 |
| [ui_scale](ui_scale/) | `ui_scale_test.pro`、`test_ui_scale.ps1` | 缩放断言与状态恢复 |
| [sendinput_lifecycle](sendinput_lifecycle/) | `sendinput_lifecycle_test.pro` | 实际 SendInputTask 的等待、暂停、取消和退出 |
| [forza_auto_state](forza_auto_state/) | `forza_auto_state_test.pro` | Forza 阈值读取与按下／释放／状态重置的并发回归，无真实输入 |
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

## 共通备份保存与空表清除

`settings_common_save`／`settings_common_cold` 使用 [common_save_fixture.ini.example](sendinput_lifecycle/common_save_fixture.ini.example)。这是Qt自身编码的完整INI，正常启动加载Common76Seed：普通A→B及两条共通源行，备份故意是不同的F20→Z。备注嵌套列表、空注释位置、中／日／emoji、QColor和同步组已由两个Qt kit实际读回；复制时保留其转义和列表结构。

在自有out副本设置 `QKM_LIFECYCLE_COMMON_VARIANT` 为attached／detached／empty。attached直接另存Map76Common；detached经真实高级设定按钮和共通复选框关闭共通表；empty经Qt Tab点击、表格Ctrl+A及Delete删除全部共通行。名称编辑和Save均经Qt事件，不直接同步备份或调用保存／加载函数，不发送系统输入。所有组独立重建样例；保存进程退出后，用原INI和同一EXE运行cold，不重新复制初始夹具。

先保留未提取实现的完整GUI保存基线，再在同路径新EXE上设置父目录下的 `QKM_LIFECYCLE_SAVE_BASELINE` 对照。新纯序列化直接接收源列表，保留74字段和行顺序；用例检查Qt类型、重复性、disk／dirty／代次及保存前旧备份不变，再核对GUI实际备份全值或空列表。GUI自己的同步仍在原保存位置；此局部序列化不替代全部保存修订及其他草稿范围，结果和权限／桌面／退出核验沿用上述settingsGeneral约定。

## 通知样式条件保存

`settings_notification_save`／`settings_notification_cold` 仍用正式manifest、自有私有桌面和公共缓存登记。初始INI以profile_save_fixture.ini.example为基础，将Profile74Seed／Keep74改为Notify77Seed／Keep77；在Notify77Seed节添加CustomNotification_Enabled=false和CustomNotification_Position=2。设置 `QKM_LIFECYCLE_NOTIFICATION_VARIANT`：absent保持此样例；enabled只把Enabled改为true；loaded把 [notification_save_style.ini.example](sendinput_lifecycle/notification_save_style.ini.example) 的14项追加到同一节。target则保持源样例，并另加Map77Notify，含合法Tab名／G→H映射、AutoStartMappingChecked=0及单项CustomNotification_FontSize=99，用于验证已有目标样式触发条件，不能预先把它加载为当前配置。

每组由正常启动加载初始源，经Qt名称编辑和真实Save另存Map77Notify；只有target点击实际覆盖确认。保留保存后的INI，正常退出后启动独立cold进程。先建立旧GUI基线，再对提取后的同路径副本设置 `QKM_LIFECYCLE_SAVE_BASELINE` 比较完整INI；结果仍在settingsGeneral。验证无样式组保持14键全缺失，其余完整写入；启用旗标、初始化旗标和目标已有键分别覆盖，loaded另对全部14个非默认值给出独立期待。新收集显式接收const QSettings目标依赖，检查类型／顺序／重复性以及disk／dirty／加载／源代次和初始化旗标不变；这不证明正式MCP保存、运行通知显示或Default桌面效果。

## 剩余配置开关、输入标量与面板保存

`settings_options_save`／`settings_options_cold` 基于profile_save_fixture.ini.example，把Profile74Seed／Keep74改为Profile78Seed／Keep78，仅将源节原始键A改为 `VButton{Button1}`，映射仍为B。`QKM_LIFECYCLE_OPTIONS_VARIANT=default`直接使用；rich在Profile78Seed节、Keep78前添加 [profile_options_panel_nondefault.ini.example](sendinput_lifecycle/profile_options_panel_nondefault.ini.example)。保持StartupAutoMonitoring、VirtualGamepadEnable、MultiInputEnable、自动匹配及Windows系统过滤键关闭，不启动映射。缺省内部FilterKeys为true，rich显式false，它与EnableSystemFilterKeyChecked是不同设置。

rich通过产品已有过滤显示入口准备自有面板，仅对背景发送Qt Ctrl拖动，屏幕参考点从73／97移动到110／126，缓存设置仍73／97；不移动物理鼠标、不点击虚拟按钮，不走窗口参考点／外部窗口分支。Save仍经Qt名称编辑和真实按钮另存Map78Profile；此显示准备不等于普通用户全部显示入口或映射运行验收。记录panel_key_signals=0、鼠标捕获已释放、实际私有桌面及进程退出。冷读必须保留保存INI并用另一进程启动，不再准备或拖动。

提取前建立独立旧GUI基线；提取后同路径设置父级baseline-*.ini，通过完整INI字节及逐项写回对照验证9项开关、当前启用ViGEm构建的6项输入标量、21项面板值。检查Qt类型／顺序／重复性和disk／dirty／加载／源代次／可见性／缓存与实际偏移不变；面板缺失的回退另作组件检查。宏关闭时输入仅4项，此构建条件仍需按实际配置另验。两项通知模式、快捷键规范化、配置顺序与保存尾部没有因这组取值而移动，未来完整保存修订仍须包含它们。

公共登记更新范围前须用正式status取得当前scope及拥有关系；verified_controller只核对身份，其内部记录不能代替即时状态。仅移除已被新EXE替代的旧定义摘要，保留有效范围；不因登记容量或旧快照问题重启缓存或延长期限。

## 保存名称与覆盖目标

`settings_target_save`／`settings_target_cold` 使用 [save_target_fixture.ini.example](sendinput_lifecycle/save_target_fixture.ini.example)，在自有out中以正常启动加载Target79Seed。`QKM_LIFECYCLE_TARGET_VARIANT`为empty／invalid／reserved／no／cancel／yes／current／new／global；global另在启动前把SettingSelect和源节Target79Seed改为QKeyMapperGlobalSetting，保留Existing79。其他组原样准备，原始目标A→B、不同已存目标Q→R及其他配置保持独立。

Qt名称编辑输入空格、非法字符、全局保留名、现有或新名称；新名称含首尾空格。真实Save触发原校验，覆盖分别点击实际No／Yes或给实际模态框发送Escape。拒绝保持原INI、当前组、dirty及加载／源代次；成功保存并独立冷读目标A→B。cold不重建INI、不重复选择或保存。结果仍在settingsGeneral，并核对身份／实际私有桌面、High／同登录、native exit0及零尾部任务；无映射启动或系统输入。

提取前先记录每组旧GUI文件，提取后同路径通过父目录下baseline-*.ini核对完整字节。只读目标分析显式传入请求名、当前名称、全局选择及已存组，检查错误类别、trim、新建／覆盖／全局决定、重复性及不变性。中间换行、本地化全局名及当前组外部消失的原判断仅补组件检查，不冒充GUI可输入这些状态的基线，也不构成执行许可；正式MCP仍须校验实例／权限／完整修订／状态／到期。

多个固定case可共用一个已核实空闲原生登记槽；更新前用正式status核对拥有关系和原定义，原精确定义由新固定EXE／参数／环境／桌面／预算摘要取代，保留审计，不增加通用启动权限或改变缓存截止。

## 保存前运行态只读投影

`settings_prepare_save`／`settings_prepare_cold`复用[common_save_fixture.ini.example](sendinput_lifecycle/common_save_fixture.ini.example)，仅在启动前把Common76Seed／Keep76替换为Prepare80Seed／Keep80。`QKM_LIFECYCLE_PREPARE_VARIANT`为direct／active／last，Save目标固定Map80Prepared。正常启动加载普通A→B、共通两行及不同的旧F20备份；名称通过Qt编辑事件输入，真实Save后由独立进程冷读，不直接调用同步／加载／保存函数。

这组测试在IDLE人工准备Active／LastActive数据及源索引依赖，不启动映射或发送系统输入，不冒充真实运行态端到端验收。覆盖不同当前指针、另一未选列表、普通／共通源、重复绑定最后项、原键不符但绑定有效、空原键、负／越界Tab或行，以及行数与绑定数不等的原行为。旧GUI基线先运行；只读准备后比较同路径完整INI及全部97项Qt值和共通74字段，另核对LockState及派生列表，准备不改源／运行列表／备份／指针／磁盘／dirty／代次。

无效Tab及临时缺失共通源仅作组件边界，缺失时回退旧备份；不冒充GUI自然产生的空指针或真实脱离流程。GUI默认仍用原行编码，原运行同步与共通同步均未改，预览使用显式只读副本；不重复提取Tab／映射字段，也不构成完整保存修订或执行许可。

## 保存快捷键输入分析

`settings_hotkey_save`／`settings_hotkey_cold`复用[profile_save_fixture.ini.example](sendinput_lifecycle/profile_save_fixture.ini.example)，启动前把Profile74Seed／Keep74替换为Hotkey81Seed／Keep81。`QKM_LIFECYCLE_HOTKEY_VARIANT`为default／window／start／stop／invalid／committed／empty，目标Map81Hotkey。名称及快捷键均用Qt编辑事件；window输入带空格的$L-Ctrl+F17，start／stop为F18／F19，invalid为无效键，committed先移回名称编辑框触发真实离焦。empty只人工清空三个后台字符串检验原默认回退，不冒充普通GUI可到达状态。

待编辑组先用项目现有QWidget::activateWindow建立自有私有桌面的Qt应用内激活，并断言实际编辑框hasFocus；不切换输入桌面、不发送系统按键、不验证Default前台或物理焦点。Save真实clearFocus触发现有editingFinished，成功后另进程冷读三项键及完整INI。最初没有应用内激活的失败另存，不能计通过；Qt6已弃用的setActiveWindow试编译警告也另存，正式选定版本只用activateWindow。

提取前先保存原GUI基线，提取后同路径核对完整字节。共享analyzeHotKeyLineEditText仅返回规范化字符串和原validateCombinationKey结果，保留美元前缀、空白清理、错误及翻译context；不改后台字符串、编辑器文本／焦点／选区／modified、dirty／代次／磁盘，也不发完成／文本信号。空白及带换行输入只作组件边界。该函数不预测Qt内部edited标志，不把hasFocus／isModified当成保存许可；完整保存准备仍须处理未提交编辑依赖与其他草稿。

## 完整Qt保存载荷组合

`settings_payload_save`／`settings_payload_cold`使用[full_save_fixture.ini.example](sendinput_lifecycle/full_save_fixture.ini.example)，由普通QtCore-only准备器对合成数据编码，正常启动加载Payload82Seed。包含普通／共通源、不同旧备份、规则、普通／通用宏、非空同步说明、14项非默认通知样式与模式、待规范化配置顺序及其他配置。`QKM_LIFECYCLE_PAYLOAD_VARIANT`为current／new／overwrite／global／pending；global启动前把Payload82Seed全部替换为QKeyMapperGlobalSetting，其他组原样，目标分别当前组／A82Saved／Existing82／全局组／A82Saved。

先运行原GUI基线，再组合既有Qt值函数。名称、Save、覆盖Yes与pending快捷键均通过Qt事件；pending沿既有activateWindow建立私有Qt焦点，实际Save仍按原规则处理F17。只读组合在输入文本与后台快捷键不一致时明确返回HotKeyInputNotSettled和空载荷，不猜Qt私有edited标志、不提交编辑；这是内部生成约束，MCP草稿守卫尚未接线。

有效载荷保留Qt类型和绝对键，声明是否还需原appendSettingSelectOrderIfMissing操作，不临时写INI模拟其结果。GUI保持原实现；检查每项生成值实际保存、所有变化／删除键均属于载荷或明确的顺序操作、全文件字节等于旧基线、独立冷读不改盘。组合不改映射、旧备份、规则／宏／说明、文本／焦点、dirty及代次，非法目标不产生部分载荷。完整磁盘／业务修订、其他编辑窗草稿、权限／状态／到期和真正提交／失败恢复仍属后续，不因组合成功开放保存工具。

## 保存候选编辑窗与依赖指纹

`settings_save_dependencies`正常启动加载[full_save_fixture.ini.example](sendinput_lifecycle/full_save_fixture.ini.example)，真实Qt按钮打开高级映射窗，应返回EditorBusy且无部分载荷；Qt Escape正常关闭后恢复。旧实现保留一项失败复现；只读INI，不调用加载／保存、不启动映射或发系统输入。按原manifest、公共登记、自有私有桌面执行。

用显式路径、存在、完整字节和合成64字符源修订检查指纹：改变Qt类型、值、目标、顺序、磁盘描述或源应区分；非法／超4MiB拒绝。核对INI原始字节、dirty、加载／源代次不变，记录实际桌面、身份及native退出。真实文件一致性、授权／身份接线、跨Qt指纹一致性、其余编辑窗动态分支和正式MCP提交不在该组件用例证明范围；无需Default。

## 完整磁盘快照保存准备

`settings_save_snapshot`正常加载[full_save_fixture.ini.example](sendinput_lifecycle/full_save_fixture.ini.example)，在自有私有桌面运行。以合成源修订检查当前／新建／覆盖、非法目标和源缺失；原目标路径通过载荷省略保留，不期待重复写入。临时同路径QSettings待写组不能成为磁盘覆盖目标，准备不冲刷或修改其缓存。

测试只在本用例out INI放置换行、坏格式、超4MiB、缺失及目录等负向夹具，再恢复原始字节；这是组件依赖输入，不冒充GUI保存基线。高级映射窗由真实Qt按钮打开／Escape关闭，准备拒绝后恢复；核对最终INI、dirty、加载／源代次及载入组不变。记录原manifest、身份、实际私有桌面及native退出；不启动映射或发系统输入，无需Default。调用中并发写盘、文件身份／权限／事务和正式MCP预览／提交另验。

## 保存阶段结果与失败恢复

`settings_result_save`／`settings_result_cold`正常启动加载[full_save_fixture.ini.example](sendinput_lifecycle/full_save_fixture.ini.example)，`QKM_LIFECYCLE_RESULT_VARIANT`为current／new／no／invalid／format／write_current／write_new／reload／backup。旧基线先用Qt名称输入、真实Save按钮及实际No保存；结果版再调用内部结果入口对照，不能用它自造GUI基线。冷读仅current／new／write_current／write_new／backup，保留前次INI另进程正常启动。

format只改本用例INI；write分支用只读共享句柄短时阻止自有INI写入／替换，返回后释放，再由读者sync及冷启动核对无失败缓存补写。reload人工注入非法源键，仅证明内部阶段分离，非普通GUI可输入；backup只在自有latest路径建空目录并及时移除。所有目标在out内，原manifest／公共登记／私有桌面，无Default、映射或系统输入。结果按磁盘同步、重载和备份分别核对，并比较完整旧GUI基线字节；内部方法仍读GUI目标，不等于MCP保存已交付或并发／权限安全已验。

保存末段共用切片复用上述维护case：current／new／write_current／write_new／reload／backup，冷读取其原允许五组。若复用SAVE-86独立旧GUI基线，先核对旧GUI case／native退出及归档EXE身份，并证明当前改动前GUI保存体与原基线无业务差异；新结果不得自造为GUI基线。新EXE路径及所复用原始材料分别记录，比较全部INI字节，保留写锁解除后共享缓存与独立冷读、已写但重载失败、备份失败的分段期待。未变的早退／取消与设备场景按原范围复用，不作为本片新增通过。

## 真实请求绑定的保存候选

`settings_save_context`正常加载同一[完整夹具](sendinput_lifecycle/full_save_fixture.ini.example)。维护目标使用本用例out内的独立私有配对目录；`QKM_CONTEXT_CLIENT_PID`／`QKM_CONTEXT_CLIENT_CREATED`／`QKM_CONTEXT_CLIENT_EXE`必须同时匹配实际桥接进程，测试代码才批准固定权限。普通Python用实际桥接完成认证并发送一次固定`qkm_get_state`，维护handler验证私有候选后只返回fixture标记；没有生产保存方法或新增MCP工具。

`QKM_CONTEXT_VARIANT=configure`验证实际Read＋Configure请求、完整源及实例绑定、显式当前／新建／覆盖目标、非法目标、五种陈旧请求、真实编辑窗拒绝及恢复。`global`仅在本用例INI把Payload82Seed替换为QKeyMapperGlobalSetting，正常启动加载全局源，验证明确全局保存及拒绝另名；不是普通GUI切换基线。仅由夹具改变并恢复派生源字段，核对未入INI的内容仍使完整修订失效；最后用既有运行修订观察器模拟新活动，核对Worker STOPPED采样不能单独放行。`read`使用真实Read配对，确认不返回候选载荷或范围；回复完成后的原请求同样失效。外部控制器保持Medium，目标沿原manifest在公共登记的自有私有桌面运行，核对原始INI、dirty、加载／源代次及GUI名称文本不变；无需Default或系统输入。

SAVE-88另核对候选scope为full_gui_save，目标／共享键精确分割全部准备载荷且无重复，包含Tab／共通备份／普通宏／同步说明及全局／忽略规则／通用宏／启动选择；新配置排序动作独立声明，保留原规范化操作，不预测最终QSettings分组写入。完整范围始终要求确认，草稿归属明确unavailable；夹具临时把dirty设false再恢复，核对它不能免除共享范围或确认。描述是写入足迹，不是精确内容diff；实际确认与执行守卫仍由后续实现。

SAVE-90在同一认证用例中临时持有QSettings待写对象，分别加入无关组、改目标原键、删除共享键、设置持久化表示相同的原生bool及超过4MiB的额外值；检查候选按实际内容拒绝或接受，同时真实INI字节及原待写对象不被冲刷／消耗。测试自己恢复全部临时值并sync，再恢复原始字节，核对同一候选恢复；这段fixture清理不是产品只读准备。`configure_saved`先用真实Save按钮完成当前配置保存，再以保存后磁盘／源代次作为候选观察基线，用来覆盖正常GUI保存后的原生缓存类型；候选本身仍须不改该基线。缓存比较只编码已存在磁盘与缓存值到私有临时文件，不写候选或操作真实INI，不代替独立GUI业务基线。

候选只使用GUI既有读取入口之后建立的const QSettings观察器，不在预览中新构造真实INI读者；缺失观察器时拒绝，错误观察器只在GUI读取边界重新建立。维护用例在待写状态临时移走观察器，验证拒绝时不回退构造；另以私有坏格式观察器复现并恢复错误状态，这个直接初始化是组件检查，不冒充GUI加载基线。曾以Qt6阶段探针确认：新建同路径读者在构造当下就可能冲刷其他对象的pending；该诊断及失败版本只保留本机out，不保留危险预览路径。新的观察器寿命另用原六组保存／五组冷读及独立GUI完整INI基线回归；投影自身编码也受4MiB限制，不承诺无执行期竞态或跨进程事务。

SAVE-91在非全局源用Qt键事件先后输入Draft91甲／Draft91乙，固定dirty=true，核对源快照不变、显式保存目标不变，但保存候选修订不同。scope的gui_save_name分别报告before／on_success／changes_on_success；这只描述可核实的文本效果，不按dirty推断所有草稿归属。恢复原文本／dirty后应恢复候选；当前配置和全局实际译名另核对成功后文本。只读准备不改INI、加载／源代次，测试自身名称输入与恢复不计为产品不变性；不执行新的保存／加载基线。

这是认证组件接线及只读候选验证；不代替独立GUI保存基线、真正运行／停止验收、调用中并发变更、预算超时或正式MCP提交。SAVE-87按受影响输入回归settings_save_snapshot；SAVE-88不重跑未变的快照／GUI保存路径，沿各自原版本有效范围复用。

## 明确目标与准备载荷的私有持久化

`settings_prepared_save`沿用上述真实认证夹具和同一完整INI。SAVE-92的before在产品8327d7bd不变时用实际Qt名称输入／Save按钮建立独立GUI基线；after通过真实请求调用候选准备，直接传明确目标及完整载荷给私有savePreparedSettingWithResult，不改GUI名称决定目标。它是内部持久化原语，生产协议无调用方；真正权限、范围／覆盖确认、到期、生命周期与执行时重检由后续协调层负责，基础状态采样不是提交栅栏。

选择current／new／write_current／write_new／reload／backup及原五组冷读；检查全部INI节、键和原始编码值，排序键也必须相同；另记原始字节是否相同。新目标的MacroList／SyncGroupNotes排列会因写入批次与旧GUI不同，不能把文本排列差异误报成全部字节一致。比较器严格拒绝键／值变化及重复键；正常冷启动另验证目标、加载／dirty及不改盘。写锁、人工无效源、备份目录负向范围沿用，均在自有out路径，原manifest／公共登记／私有桌面，无Default或系统输入。

当前目标另以无效准备、改向路径、组件STOPPING和变化磁盘字节检查写入前拒绝；组件状态只作分支覆盖，不代表真实停止。写失败保留原GUI名称，重载失败沿共享收尾清空名称。仍缺提交中竞态／文件身份、真实权限撤销／清理、预览消费／去重及正式MCP业务闭环，不能由该私有测试推定通过。

SAVE-93在settings_save_context增加私有重校验：QDeadlineTimer保留原精确单调截止，范围和覆盖分别确认；当前授权及完整候选重新采集，匹配原修订／context／scope才返回新载荷。可控过期／无限截止、旧修订／缩窄范围、旧载荷注入及旧覆盖位清除均定向覆盖；不实际等待120秒，也不声称校验中撤权／到期已验。原实际Qt名称、源字段、磁盘字节、pending、编辑窗及活动修订夹具同时核对重校验拒绝，Read与已消费请求不返回可用载荷。此调用不写盘，未把新持久化入口接入；configure_saved仍仅以既有真实GUI Save建立观察基线。

## 正式保存只读预览的维护接线

`settings_save_preview`使用隔离ControlDialog／私有存储、实际SDK子进程身份白名单及Read／Read＋Configure配对，随后把请求交给真实handleAutomationRequest。接口启用／配对是维护设置，不替代普通Release用户GUI批准。官方stdio检查当前／新建／覆盖／全局范围、取消／替换、原映射兼容、保存误提交拒绝及最终状态不变；`case.json.done`只让本测试记录SDK完成并正常退出，不能执行产品命令。最终CPP核对完整INI、dirty、加载／源、名称和派生数据。尚无实际保存、运行、截图或QQ回传，不能由该模式推定完整业务验收。

`settings_save_execution`是SAVE-96的固定认证维护模式：QKM_CONTEXT_VARIANT限选configure／new／overwrite／global／read，调用私有执行协调器，验证元数据确认与拒绝、全INI／源不变性，以及实际重载的互斥和分段结果。旧表destroyed回调中的普通QThread仅tryLock，不发输入；末尾观察运行修订使旧许可失效，不能当作实际停止。settings_save_context已改为只交元数据重校验，原载荷注入不进入执行参数；两模式均不是正式MCP保存工具。固定启用／配对与原权限边界不变，结果及未验范围见唯一主计划与同一C1证据。

`settings_save_apply`沿用准确SDK peer白名单和生产handleAutomationRequest，固定本轮普通／新建／覆盖／全局、Read、写失败恢复、备份失败、添加后保存及独立冷读场景。正式保存通过工具执行，维护代码只在预览时独立采集候选供最终全键比较，并核对实际加载代次／源、名称和dirty。完成文件仅记录SDK结果和固定0／1保存计数，不能执行产品命令；冷读使用独立产品进程及新认证，只读检查实际源与旧引用失效。此为正式业务接线验收，仍不等于普通Release配对GUI、完整输入或所有生命周期已验。

SAVE-98增加settings_save_gui的global／overwrite固定GUI基线，Save按钮及Yes对话框独立于私有执行助手，settings_result_cold另进程读回。同一settings_save_apply仅在reload_failure维护变体注入已知坏源并走正式SDK，核对磁盘已保存、加载invalid_mapping_data、未备份及源不可用；该变体不是正常GUI输入来源。普通控制器必须先保留native退出码再解析case，缺失退出码不能由case通过推算。产品实现未变，详见同一C1证据。

SAVE-99的settings_save_apply固定stale_name／stale_global变体，由完成预览后的.gui-change触发一次真实Qt名称输入或配置弹窗筛选／Enter；.gui-changed只记录实际结果。CPP在旧提交拒绝后检查完整GUI后状态，再核对新预览确认的一次保存；触发器不接受任意动作／文本。配置GUI加载与MCP保存代次分别计数。点击助手仅把接收类型拓宽为QWidget以复用相同Qt鼠标事件，原按钮事件构造不变，不产生系统输入。

SAVE-100的settings_save_apply固定timeout_after_accept／disconnect_after_accept变体，在实际同步后旧表销毁点写accepted-reload，暂停本实例重载至固定release文件或15秒上界，不处理GUI事件。正式SDK观察原10秒timeout或主动qkm_disconnect的cancelled，再原配对重连query同ID，不能把等待取消当撤销保存；CPP要求一次配对、一次加载、完整实际值匹配。每次控制器调用只选一个场景，逐场保留原始退出／登记／尾状态。此为测试夹具，无普通Release延迟入口，不作性能／停止完成结论。

SAVE-101在settings_save_apply增加cleanup_stop／cleanup_timeout：固定.gui-run／.gui-stop文件仅触发真实Qt按钮事件，setKeyUnHook后的Hook回调以InputProducerWorkItem有界等待.release-cleanup最多5秒；后者只把原GUI观察定时器缩为30ms。正式SDK核对Worker STOPPED但清理pending仍拒绝，实际完成后新预览保存一次。旧尾部／排空处理函数直接组件回放不代替真实迟到事件。系统过滤键选项关闭并核对完整前后状态，无系统输入／Default。普通场当前预览在pending中被拒绝，超时场保留旧预览至完成后核对陈旧；同一客户端新预览会取代旧预览，不能先创建两个再期望首个仍可提交。

SAVE-102复用cleanup_stop及cold_addsave入口，正式SDK在实际completed_cleanup后新增X→Y、确认全量保存，并由独立新进程Read冷读。完成记录added_mapping=true时CPP另查实际源X→Y及GUI表行数；原有场景不报该字段则不执行此断言。新增与保存历史结构不同：mapping.add用source_revision_before／source_tab_ref_before，target仅属于保存结果。暖冷必须保留同一已保存INI和最新备份，旧实例操作unknown不能解释为此前未写；每次公共控制器只运行一个固定场景。

SAVE-103的cleanup_return使用合法Global A→B及McpReturn Q→R初始INI，Global AutoStart=2但StartupAutoMonitoring=false；GUI停止前仅通过既有armReturn准备匹配记录，实际停止尾部回切。正式SDK旧Global预览必须陈旧拒绝，新非法Global目标invalid_arguments，新确认才保存返回源，回切与保存各一次代次。原Global动态表销毁，单Tab主表Widget复用是既有行为，不能要求主控件销毁或用其地址充当源身份。此夹具不证明真实前台自动匹配，不启动系统输入。

DEMO-P0-104在settings_save_apply的demo_capture固定变体中复用正式SDK建例框架：空白合法Demo104Seed源、只内存新增A→B；.capture-main／.capture-item仅触发自身窗口采样和表格真实Qt双击，.capture-close发送Esc关闭单项窗。内部qkm_window_image只返回有界PNG字节，无文件或激活参数；固定.frame-main／.frame-item_setup是维护记录，普通SDK按3MiB记录／2MiB PNG上限验证后写自有图片。该记录含图片，不能误用默认小容量缓存记录读取器；其invalid_record不等于gsudo启动故障。Agent需实际查看原图，SDK结构校验不代替看图。当前仅main／item_setup中文DPR1客户区可行性，关闭／隐藏和错误线程为空失败；不提供正式捕获权限或工具、不改变Read，当前源码底层不代表Release可直接服务QQ。

## IbInput 基础键盘接收

keyboard_receiver.pro生成普通权限接收窗，观察常用键盘VK的DOWN／UP及扫描码／修饰状态，不安装Hook、不调用发送API；Escape保留为中止。--self-check只向组件派发Qt事件；正式接收需QKM_IBINPUT_KEYBOARD_OUTPUT现有输出目录及实际Default桌面，180秒退出保护，receiver-close文件触发正常关闭，Escape中止。合成manual_keyboard_verification.ini不自动开始映射。

product_check.pro的keyboard维护用例继续使用真实main.cpp与正式manifest，QKM_IBINPUT_PRODUCT_CASE=keyboard、QKM_IBINPUT_PRODUCT_DESKTOP=Default，QKM_IBINPUT_PRODUCT_OUTPUT指定输出目录，QKM_IBINPUT_KEYBOARD_HWND／QKM_IBINPUT_KEYBOARD_PID须精确绑定已启动的自有接收进程。只接受三条固定映射W→F、1→2、F13→F24；keyboard-stop／keyboard-restart／keyboard-exit／keyboard-abort文件依次请求真实产品入口，状态写入keyboard-*.json。它本身不生成原始输入；固定输入测试器须另按具体授权执行，并核对前台、实际桌面、按键持有及自有清理，不可仅凭编译成功直接运行。

本机K1已授权的受管启动记录、接收结果与普通截图见handoff/ibinput-simulator主计划12.11；MCP工作区的临时定义和缓存不随Git接续，不作为另一台PC的必备环境。正常运行和人工验证以主计划及独立目录说明为准。

## IbInput 鼠标固定接收与软件检查

client_test.pro默认替换正式C API，生产能力模式使用CONFIG+=ibinput_production_capabilities；两者均不链接IbInput DLL。覆盖五按钮身份、整格滚轮、持有期间移动／滚轮、代次及停止拒绝、部分失败后同后端释放和释放失败禁止重启。logitech_report_test.cpp编译固定上游＋已批准补丁的实际实现，以WinAPI替身检查Mouse8／Mouse16报告位、拆包、正负轮单位和失败传播；这些均不是驱动接收验证。

ghub_probe.pro只链接生产客户端、Qt及正式x64 IbInput DLL，普通权限，不启动产品主窗口。--ghub-connect-cycle仅连接／关闭／复连，--ghub-smoke保留基本移动／F24／左键／滚轮，--mouse-smoke检查五按钮各一次、左键保持的±8相对移动／上下各一格滚轮及停止释放；三种模式必须单选。配置QKM_IBINPUT_PROBE_LOG为可写日志绝对路径，确保正式DLL可加载、G HUB版本和设备／驱动条件已核实。新PC按实际Qt6工具链构建该pro即可，不复制另一PC的out缓存。

固定接收必须另获对应真实输入授权：仅Default自有接收窗，确认无其他QKeyMapper及物理持有，单击标题栏激活后才初始化；焦点／桌面／光标区域／进程冲突或Escape中止并释放同后端的自有状态。不要以连接或API成功代替窗口事件、方向、轮单位和DOWN／UP平衡结果。用项目capture_ui_snapshots.ps1核对自有PID／HWND并截图，正常关闭后确认原生退出码；失败保留日志，不自动重放。测试方法在此，实际系统／版本／结果仅更新唯一主计划。


## IbInput 产品鼠标映射维护用例

复用product_check.pro的mouse用例和keyboard_receiver.pro的--mouse模式；保留现有keyboard协议。接收器仍普通权限、无发送API或驱动依赖，--self-check在Qt组件内检查五按钮配对、保持标志、正负轮单位及移动观察，运行时可设置QT_QPA_PLATFORM=offscreen。mouse模式沿用QKM_IBINPUT_KEYBOARD_OUTPUT作为接收输出目录，发布receiver-ready.json／receiver-state.json。

产品环境为QKM_IBINPUT_PRODUCT_CASE=mouse、QKM_IBINPUT_PRODUCT_DESKTOP=Default、QKM_IBINPUT_PRODUCT_OUTPUT；接收窗身份沿用QKM_IBINPUT_KEYBOARD_HWND／PID。manual_mouse_verification.ini提供九条固定原始键映射，不自动开始；产品校验内容后调用既有开始／停止／退出。mouse-started.json、mouse-stopped.json、mouse-restarted.json、mouse-exiting.json为观察点，mouse-stop／mouse-restart／mouse-exit／mouse-abort为外部请求。原始输入仍由另获具体授权的普通固定测试器产生；不以运行维护目标自行获得输入权限。

接收mouse_down／mouse_up以Qt按钮数值1／2／4／8／16分别对应左／右／中／X1／X2；mouse_buttons是当前按钮位，move_buttons／wheel_buttons记录对应接收事件的保持位，wheel_delta记录实际angleDelta.y()。测试器核对真正的接收与系统按住状态、实际后端及代次；不能只看marker文件或API返回。真实执行／未执行范围以唯一主计划第14节记录为准。


接收器自检如需显示核对，可显式设置QKM_IBINPUT_RECEIVER_SNAPSHOT为PNG绝对路径；同时输出<PNG>.desktop记录自身线程桌面。offscreen只证明逻辑，字体缺失时不得记为视觉通过；可在普通私有桌面使用windows平台渲染，具体原生STARTUPINFO启动方式沿现有私有截图经验，不通过切换Default或提权解决。该可选渲染不发送系统输入。


## IbInput 通用组合／序列／连发／锁定

product_check.pro新增patterns固定用例，沿既有输入状态机及正式manifest，控制文件前缀为patterns-。manual_patterns_verification.ini提供Q→F+G、W→H»I、E→J连发60/60ms、R→K及T→L锁定。维护目标会核对映射、发送方式及连发／锁定标志，再调用真正开始／停止／退出入口；不自行生成原始键。

普通接收器键盘模式观察常用键盘键，receiver-state.json的key_events（vk及up）记录实际按住状态转换，最多128项，events_truncated标记超界；DOWN／UP完整计数不受观察截断影响。观察仅限自有接收窗收到的事件，未安装全局Hook。自检验证重复DOWN不计两次、顺序及128边界后计数继续完整；真实组合顺序／连发／锁定仍须具体授权和实际窗口接收，不以组件自检代替。


## IbInput 序列保持／循环

product_check.pro的loops固定用例复用既有开始、停止、重启与退出状态机，控制文件前缀loops-；manual_loops_verification.ini包含最后键保持、有限三轮、按住循环及键鼠中途取消场景。普通接收器--mouse同时记录基础键盘集与鼠标事件，仍不调用发送API或驱动；--self-check补验混合模式普通字母的DOWN／UP。按住状态转换轨迹最多128项，超过后完整计数继续，不能将截断轨迹当作完整顺序。

驱动释放修正需验证“中断等待后同代再次触发出现新DOWN”，只看SendInput的UP或停止marker不能证明IbInput持有报告已清空。实际测试仍先获得具体程序、初始化、Default前台／光标、固定输入和清理范围授权；本用例本身不授予输入权限。当前及后续Razer切片的系统、版本、授权和实际结果仅更新唯一主计划。


## IbInput 宏实际输出检查

product_check.pro的macros用例复用原输入状态机，前缀macros-；manual_macros_verification.ini包含普通／通用宏、Repeat／OnlyOnce、键鼠中断释放及宏Key2Mouse六行，宏列表使用正式QSettings QVariant格式。测试原始键仍由另获授权的普通固定测试器产生；无新输入授权不得直接启动。

开始前元数据断言核对派生纯键、源行不变、宏隐藏的混用及禁用规则、反馈键、嵌套／循环引用；开始及重启后额外核对Key2Mouse合成方式为IbInput。实际输出、循环次数／OnlyOnce、鼠标保持及停止退出仍由自有接收窗口计数／轨迹和系统按住状态证明。validation_test.cpp另补“仅抬起映射含方向”不能绕过混用的纯规则断言。源码／纯规则／实际接收／人工验收分别记录在主计划第18节。


C3实际纯键派生通过现有展开器的collectOnly=true仅收集原语，普通发送默认false；维护断言覆盖默认x2仍4步、收集2步、99999×99999只收集F/G而不实际发送，以及“宏名含方向但正文不移动”不能影响移动分类。宏数量与嵌套上限沿既有规则。

接收器状态文件是测试遥测，使用QFile覆盖／flush；并发读取必须沿现有runner重试空／不完整JSON。Windows上即便读句柄共享删除，QSaveFile覆盖仍可与读句柄冲突，不能把此遥测协议套用到需要事务性的产品配置保存。主计划18.3记录无输入复现和20000轮修正验证。


## IbInput 常用键盘表示

product_check.pro的standard用例沿既有状态机和正式manifest，前缀standard-；manual_standard_verification.ini固定八行Q/W/E/R/T/A/S/D，包含编辑／导航／数字区／OEM标点序列、左右Shift／Ctrl+F24，不自动开始。维护目标核对原文、方式3及无连发／锁定／循环等标志；原始键仍由另获具体授权的固定运行器生成，Default自有前台及同后端停止／重启／退出释放必须观察。

receiver-state.json保留原key_events的vk／up格式，并增加逐项对应的key_observations（vk／up／scan／modifiers）。两者最多128项，events_truncated必须为false才能用作完整轨迹；Tab由自有接收器事件入口直接接收，避免Qt焦点导航吞掉。数字区在NumLock关时可呈现导航VK，应核对扫描码和Qt::KeypadModifier，不为测试更改锁定状态。左右修饰同时核对系统侧状态及F24的Qt修饰位。

当前G HUB及FakerInput均按VK-only发送Return，NumEnter不表示独立数字区回车。client_test正式／替身115键门控及槽位断言、真实G HUB头文件的WinAPI替身报告检查与产品实际接收属于不同层次；Win／Alt／菜单／锁定／媒体等未进行相应真实副作用验证时不能声称已完成。实际机器、范围、EXE及结果只记录在唯一主计划第19节。


## IbInput 串行切表

product_check.pro的tables用例沿既有状态机，前缀tables-；manual_tables_verification.ini的A/B两表使用IbInput、C表使用SendInput，Q分别输出F/G/J，W通过真实SwitchTab命令形成A→B→C→A；B另有SendOnSwitchTab的L⏱20»M⏱20。维护目标只观察真实业务切换，tables-switch-1/2/3文件开启对应观察，实际原始W仍由另获授权的固定运行器发送。收到新表已启用、通道／新代次／停止按钮可用后写tables-switched-1/2/3.json；正常停止／重启／退出仍沿tables-stop/restart/exit。

客户端替身restart断言覆盖预留新意图、旧关闭不得取消新意图、特定旧代取消不得影响新代以及用户停止后不能复活请求，不能替代实际池排空和窗口输出。实际产品须验证保持输出时切换的旧UP与新DOWN、Ib→Ib／Ib→SendInput／SendInput→Ib、自动切表输出及正常停止／退出释放；串行通过不等同于快速／并发切表、切换中停止及各初始化失败已验。源码及结果只更新唯一主计划第20节。


tables另支持三个依次触发的固定T3控制文件：tables-race-sendinput（B→C，最新为SendInput）、tables-race-latest（A→B→A，最新为IbInput）、tables-race-stop（B→C→A后立即正常停止）。维护目标用有5秒上限的工作队列门闩先排完UI切换，走真正MappingTableSwitchByTabName，写race-requested-1/2/3、race-ready-1/2及race-stable.json；后者在实际停止完成后继续观察5次轮询，不能以静态停止marker替代。旧中间B不应执行SendOnSwitchTab，外部原始Q实际输出F/J并检查旧UP、重启及退出释放。串行协议保持；此场景证明确定性排队交错，不等同实体快速W或驱动I/O故障。方法和原始键／Default／清理授权边界沿上文，实际结果见主计划28节。

## IbInput 配置往返与保存失败

product_check的私有save用例同时走真实generateMappingCode／applyMappingCode，将其他方式覆盖为方式3；随后用真实导出／导入API把四行导入新IbRoundTrip表。保存失败用例只对隔离运行目录的keymapdata.ini持只读共享句柄，调用正常保存，核对返回false、原字节不变且未请求运行；释放句柄后沿既有Popup偏好和正常保存继续。该句柄不接触个人配置或系统设置。

cold必须复用这次save产生的同一运行目录INI，核对新增导入表及原表的方式3、偏好和Popup绑定；不能把旧版save结果与新版cold混用。两个程序在独立QkmIbProduct_*私有桌面，保持正式管理员manifest及原入口初始化，不开始映射／发送输入。配置检查与GUI控件断言不等于新视觉验收；当前版本及机器结果见主计划33节。真实文件锁失败也不替代任意断电／磁盘满或合流后MCP保存事务验收。

## IbInput 开始／停止动作

product_check.pro的stopactions用例沿既有状态机，前缀stopactions-；manual_stopactions_verification.ini含Q→F、SendOnMappingStart→H⏱20»I⏱20、SendOnMappingStop→Repeat{F⏱20»G⏱20}x2，全为方式3，无自动开始。原始键须另按具体授权由固定运行器产生；产品本身只核对配置并调用真实开始／停止／重启／退出入口。

停止时先取消普通Ib输入并排空／释放旧保持，再为不可变快照中的SendOnMappingStop源行授予本代内部阶段；普通DOWN和周期输入仍被拒绝，释放失败不能打开阶段。client_test的stop_action断言覆盖权限边界、键鼠原语、取消及新代／失败不继承。真实接收重点核对旧F先UP再出现停止F/G完整两轮、开始／重启H/I及退出保持释放；退出动作沿原有析构语义，不能仅凭关闭marker认为停止动作已发送。实际版本／机器／范围仅记录唯一主计划第21节。


停止序列的父任务会继续排队子步骤，池暂空不能单独证明完成。关闭检查同时观察既有running序列及各池waitForDone(0)，通过工作线程10ms上下文定时继续处理消息，完成后关闭；停止期间旧allMappingKeysReleased回调由统一排空负责清理。IbInput序列子步骤沿实际源快照查找行和循环信息，避免恢复原表后用错索引。首次缺失和修正后实际完整Repeat x2及当前源T1回归见主计划21.4～21.5。

## IbInput Razer代表路径与手动驱动脚本

product_check沿用原固定用例，QKM_IBINPUT_PRODUCT_BACKEND只接受logitech_ghub（默认）或razer；用razer_*.ini独立合成配置测试，不替换个人INI。启动marker记录实际后端，主界面异步状态标签等待刷新后才发布started；mouse／standard／patterns／loops／macros／tables／stopactions的控制文件协议不变。razer_keyboard_verification.ini保留基础键盘入口。

pause使用razer_pause_verification.ini（Q→Pause、方式3），pause_faker使用faker_pause_comparison.ini（相同映射、方式2）并断言FakerInput真实连接及IbInput未启动；前缀分别pause-和pause_faker-。维护目标不生成原始输入，真实运行仍须固定范围授权。停止映射会恢复仍保持的原始键，应先核对映射输出已UP，再抬起测试原始键检查全部清空；不能把合法原始键保持算成输出释放失败。

logitech_report_test新增Razer基础报告、右修饰／除号／菜单E0和Pause完整E1序列及两段失败传播，所有设备WinAPI均替身；不是实际驱动接收。ghub_probe的--razer-connect-only／--razer-connect-cycle-only只连接／关闭（后者一次复连），无窗口、不取焦点、send=0，仍按驱动初始化授权使用。

razer_driver_script_test.ps1通过Windows自带PowerShell 5.1运行，替代CIM、权限、用户确认和原生命令，验证精确实例、失败／重启传播及保留驱动包；不操作本机设备。真实驱动安装／卸载未由此证明。Razer当前运行证据和未覆盖范围只维护在主计划23节；正式Release打包测试仅Win10专用PC执行。


## IbInput Razer 水平滚轮

product_check.pro的horizontal固定用例复用开始／停止／重启／退出，前缀horizontal-；razer_horizontal_verification.ini为Q→Mouse-WheelLeft、W→Mouse-WheelRight、E→Mouse-L，方式3、首选auto。维护目标检查实际偏好auto，started／restarted记录preferred_backend及实际razer，不能把连接成功当窗口收到滚轮。

接收器--mouse保留wheel_up/down、wheel_delta(y)及wheel_buttons，新增wheel_left/right和wheel_delta_x；按Qt Windows适配约定，Qt横向角度正为左、负为右，与WM_MOUSEHWHEEL原生符号相反。--self-check通过Qt组件派发两个轴各正负120，不能当真实输入；固定实际运行器另核对原始Q/W/E、左右各一格、左键保持的wheel_buttons、停止／退出UP及FilterKeys不变。源码范围、机器、实际EXE及结果仅见主计划29节。

## IbInput G HUB／Razer绝对坐标

product_check的absolute固定用例使用razer_absolute_verification.ini：Q屏幕移动、W屏幕左键点按、E保存、R第二落点、T恢复、A窗口客户区移动、S窗口左键点按；方式3、显式偏好razer并断言实际razer。ghub_absolute_verification.ini复用相同七行，偏好auto并断言实际logitech_ghub；不是把G HUB旧单次换算作为通过依据。坐标含逗号的INI值须加引号；Qt6／Qt5均读取原完整字符串。协议沿absolute-started/stopped/restarted/exiting.json及absolute-stop/restart/exit/abort。

维护目标核验接收PID／HWND／标题，只为本用例绑定该自有窗口，并断开自身周期进程列表刷新以免其清空测试绑定；断言目标不漂移。这是发送路径夹具，不证明自动进程匹配。absolute-clear-target只清空此绑定并写target-cleared.json，用于检验缺失目标后的UP，不等同实际窗口销毁或PID复用。原始键仍由获准的普通固定运行器生成，真实输出来自正式Worker和指定后端DLL。G HUB固定absolute-switch-1／2请求通过正常MappingTableSwitchByTabName业务入口切换两张自有表，确认代次更新和旧故障失效；此入口不替代T1的原始键SwitchTab指令证据。

运行器先确认屏幕及ClientToScreen后的落点均在接收客户区；首个移动实际落点通过后才点击。定位／保存恢复用GetCursorPos与接收移动核对，点按用接收DOWN／UP计数及mouse_observations中的实际Windows像素位置；接收器以Qt本地坐标、DPR和ClientToScreen转换，自检仅核对合成事件记录，不当成实际点击。保持时小幅自有移动后检查缺失目标UP、停止／重启／退出释放且无清理跳转。客户端替身覆盖归一化边界、五按钮同包、晚到UP及已准入停止源保留坐标；报告替身覆盖实际Razer Flags／LastX/Y，不替代真实接收。范围／机器／EXE／结果见主计划30节。

logitech_report_test的logitech_absolute组将GetSystemMetrics及GetCursorPos也替换为固定查询：验证负原点、尺寸改变、主屏归一化端点、持有按钮和查询失败／非法边界；不读取或修改实际系统设置。该组证明0009报告数值契约，不证明Windows实际落点。A2单次换算的实际失败及当时收敛决定保留在主计划31节。A3在第一方客户端新增像素反馈事务，达到各轴≤2px并再次采样确认后才点击；0009本身和Razer原生路径不变。不能把替身结果当作Windows实际落点，也不要为使测试通过自动调整指针设置。当前实际G HUB／Razer结果与环境边界见主计划34.2／34.3。

## IbInput 手柄移动来源

product_check的motion用例使用manual_motion_sources_verification.ini或razer_motion_sources_verification.ini：Joy-LS2Mouse／Joy-Gyro2Mouse／Joy-Touchpad2Mouse同名控制标记及Q→Mouse-L，均方式3，显式选择对应后端。控制标记不直接产生物理键鼠，不应被当作普通按键自映射；validation_test只对四种确切标记和单个@0～9编号放行，普通键鼠和畸形／组合负例继续受保护。

协议沿motion-started/stopped/restarted/exiting.json及motion-stop/restart/exit/abort。motion-step-1～13按序创建并等待同名.json：1/2左摇杆轴正反、3/4陀螺仪正反、5/6触点正反、7～9左键保持期间三源、10停止后无输出、11～13重启后三源。原始Q仍由获准普通运行器经Hook产生，维护入口只在Worker线程排队有限合成事件，使用正常加载的方式／开关和算法，合成设备对象通过共享所有权覆盖排空；不直接调用发送函数替代映射。

摇杆每样本调用一次正常周期处理入口并立即回中，不证明原周期调度器的持续手感。Gyro用有限单调时间戳及正常算法，固定灵敏度7／阈值2、采样15与速度2；启动前核对实际加载值，不能将被UI范围钳位的INI原值当作实际值。触摸板使用Down／Motion／Up及正常累计。接收端核对各方向实际位置与事件、小幅范围、保持按钮、停止后无移动及停止／退出UP；不是SDL枚举、物理采样或硬件校准通过。Win10实体补验完全可选，所有源码／版本／实际结果和失败历史见主计划32节。

## IbInput Popup 显示维护

ui_test.pro生成普通权限QKeyMapperIbInputUiTest，显式使用--ibinput-ui-check运行；QKM_IBINPUT_UI_ROOT指定现有结果目录，QKM_IBINPUT_QM_ROOT指定三语QM目录。程序在创建QApplication前进入自有QkmIbInputUi_<PID>私有Windows桌面，Qt5需要该桌面的DESKTOP_HOOKCONTROL以建立自身线程WH_GETMESSAGE消息机制；不切换Default，不运行产品映射Hook。

十五个场景覆盖三语、100%／150%浅深主题、可见期间的语言／调色板切换、Razer接口及G HUB登记缺失／查询失败、长错误换行与超长错误滚动、详情选择／滚动保持，以及自动顺序、G HUB／Razer检查结果、运行与停止、偏好变化及延迟结果失效。上下两区都是只读QTextEdit；详情按ibInputDetails查找，反馈沿用ibInputFeedbackLabel对象名。显示／被动刷新不得改变偏好或客户端状态采样，也不得调用初始化／发送API。错误通过既有Requirements.rejection入口在初始化前构造；自动检查／运行状态通过公开客户端入口和C API替身构造，固定夹具初始化与意外初始化分别统计，发送仍为0，不打开实际驱动。

每场写scene.json，捕获工作进程须通过STARTUPINFO.lpDesktop在同一私有桌面启动，再调用scripts/capture_ui_snapshots.ps1的-ProcessId／-WindowHandle／-PrintWindow／-NoActivate；捕获后写captured-<序号>应答，最终检查原生退出码和result.json。截图线程留在Default会无法访问目标HWND，不能为此静默改用Default测试；私有Popup截图跳过前台激活可避免激活请求引起的原生关闭事件。NoActivate是可选参数，省略时保留原截图激活行为。此入口只证明真实Popup组件和资源，不证明完整主程序、映射输入或驱动释放；具体运行证据只记在唯一主计划。

客户端superseded_send在真实C API替身内让旧发送遇到新requestRun再失败，检查旧清理不取消新意图；不是真实驱动故障。product_check的save另在私有桌面验证排队故障代次过滤及当前故障仍可显示，只预留／取消客户端意图，不启动映射或发送输入。结果见IbInput主计划34.1。


A3客户端ghub_position／ghub_position_boundaries使用固定位置、尺寸和时钟替身，覆盖不同增益、非线性、延迟、过冲、负原点、规范化端点、32次／200ms软件预算、查询／发送失败、失败不点击、持有按钮、并发串行、键盘与纯UP旁路、正常取消和释放失败重启屏障。固定8px步长对特定目标无法达到±2px时必须失败，不放宽成功条件。QKM_IBINPUT_CLIENT_TEST仅用于维护构建，正式目标仍调用真实系统查询。

sendinput_lifecycle维护目标的重启调用显式预留代次，晚到admission用例传0表示非IbInput源。A3接纳只选wait_clear、queue_keyup、cancel_reset、exit_wait、borrowers五个实际任务／排空／退出ASan用例，不发送真实输入；客户端ASan使用C API替身。两者不等同真实驱动输入ASan或Forza全套验收。同步修复Forza状态测试中两处旧重启签名，仅作调用兼容，不修改Forza算法。

U42／U43的六个三语错误场景追加在ui_test原十五场之后，使用负目标坐标与Windows错误5经Requirements.rejection进入真实反馈区；检查QM翻译、占位符、换行及窗口边界，不打开驱动。当前共21场景，证据见主计划34.3。
