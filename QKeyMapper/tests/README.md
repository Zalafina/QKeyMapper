# 第一方测试

维护中的第一方测试按主题放在 `tests/<主题>/`，对应的 `.pro`、测试代码和专用脚本相邻存放。新增测试沿用这一约定，不在产品目录根部增加 `*_test.pro`。通用构建、静态分析和进程管理工具继续放在仓库 `scripts/`；第三方测试保持上游目录结构。

| 主题 | 工程与运行入口 | 用途 |
|---|---|---|
| [ibinput](ibinput/) | client_test.pro、validation_test.pro、ui_test.pro、product_check.pro、keyboard_receiver.pro、ghub_probe.pro | 客户端／能力替身、移动组纯规则、真实Popup组件、产品保存拒绝／Common生效上下文／冷读、显式授权的固定接收；manual_verification.ini无自动映射 |
| [ui_scale](ui_scale/) | `ui_scale_test.pro`、`test_ui_scale.ps1` | 缩放断言与状态恢复 |
| [sendinput_lifecycle](sendinput_lifecycle/) | `sendinput_lifecycle_test.pro` | 实际 SendInputTask 的等待、暂停、取消和退出 |
| [forza_auto_state](forza_auto_state/) | `forza_auto_state_test.pro` | Forza 阈值读取与按下／释放／状态重置的并发回归，无真实输入 |
| [diagnostics](diagnostics/) | `diagnostics_test.pro`、`run_checks.ps1`、`measure_overhead.ps1` | 原生诊断策略、符号、故障注入及进程开销采样 |

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

性能基准使用非 diagnostic 的普通 Release。生命周期测试的普通版保留原等待与 cleanup 耗时门槛；diagnostic 写盘和 ASan 用于完成／销毁及内存安全检查，不用于判断日常性能。运行器须保留每项30秒看门狗；diagnostic 的 `borrowers` 在内部等待全部调用完成。

源码、Git、构建和分析保持非管理员。只有保留正式 manifest 的必要测试启动／关闭使用已授权 gsudo，并由用户确认 UAC。无输出生命周期用例不证明硬件、Hook 或驱动释放。窗口测试和完整阶段门禁按仓库工作流另行验收。

静态报告回归入口仍为 `scripts/test_static_analysis.ps1`，只验证目标发现和报告门禁，不代替真实 Clang-Tidy／Clazy。


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

## IbInput Razer绝对坐标

product_check的absolute固定用例使用razer_absolute_verification.ini：Q屏幕移动、W屏幕左键点按、E保存、R第二落点、T恢复、A窗口客户区移动、S窗口左键点按；方式3、偏好auto并断言实际razer。坐标含逗号的INI值须加引号；Qt6／Qt5均读取原完整字符串。协议沿absolute-started/stopped/restarted/exiting.json及absolute-stop/restart/exit/abort。

维护目标核验接收PID／HWND／标题，只为本用例绑定该自有窗口，并断开自身周期进程列表刷新以免其清空测试绑定；断言目标不漂移。这是发送路径夹具，不证明自动进程匹配。absolute-clear-target只清空此绑定并写target-cleared.json，用于检验缺失目标后的UP，不等同实际窗口销毁或PID复用。原始键仍由获准的普通固定运行器生成，真实输出来自正式Worker和Razer DLL。

运行器先确认屏幕及ClientToScreen后的落点均在接收客户区；首个移动实际落点通过后才点击。定位／保存恢复用GetCursorPos与接收移动核对，点按用接收DOWN／UP计数；保持时小幅自有移动后检查缺失目标UP、停止／重启／退出释放且无清理跳转。客户端替身覆盖归一化边界、五按钮同包、晚到UP及已准入停止源保留坐标；报告替身覆盖实际Razer Flags／LastX/Y，不替代真实接收。范围／机器／EXE／结果见主计划30节。

## IbInput Popup 显示维护

ui_test.pro生成普通权限QKeyMapperIbInputUiTest，显式使用--ibinput-ui-check运行；QKM_IBINPUT_UI_ROOT指定现有结果目录，QKM_IBINPUT_QM_ROOT指定三语QM目录。程序在创建QApplication前进入自有QkmIbInputUi_<PID>私有Windows桌面，Qt5需要该桌面的DESKTOP_HOOKCONTROL以建立自身线程WH_GETMESSAGE消息机制；不切换Default，不运行产品映射Hook。

十五个场景覆盖三语、100%／150%浅深主题、可见期间的语言／调色板切换、Razer接口及G HUB登记缺失／查询失败、长错误换行与超长错误滚动、详情选择／滚动保持，以及自动顺序、G HUB／Razer检查结果、运行与停止、偏好变化及延迟结果失效。上下两区都是只读QTextEdit；详情按ibInputDetails查找，反馈沿用ibInputFeedbackLabel对象名。显示／被动刷新不得改变偏好或客户端状态采样，也不得调用初始化／发送API。错误通过既有Requirements.rejection入口在初始化前构造；自动检查／运行状态通过公开客户端入口和C API替身构造，固定夹具初始化与意外初始化分别统计，发送仍为0，不打开实际驱动。

每场写scene.json，捕获工作进程须通过STARTUPINFO.lpDesktop在同一私有桌面启动，再调用scripts/capture_ui_snapshots.ps1的-ProcessId／-WindowHandle／-PrintWindow／-NoActivate；捕获后写captured-<序号>应答，最终检查原生退出码和result.json。截图线程留在Default会无法访问目标HWND，不能为此静默改用Default测试；私有Popup截图跳过前台激活可避免激活请求引起的原生关闭事件。NoActivate是可选参数，省略时保留原截图激活行为。此入口只证明真实Popup组件和资源，不证明完整主程序、映射输入或驱动释放；具体运行证据只记在唯一主计划。
