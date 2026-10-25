# CACHE-SKILL-72 公共测试入口验证

本页只保存证据与覆盖边界；状态及接续动作见[唯一主计划](qkeymapper_mcp_automation_plan_codex.md)。PC-W10，2026-10-25，Win10 19045 x64。基于4b0f2b58加本切片缓存技能、公共进程助手及Python测试调用方；C++产品、Worker、依赖锁、正式manifest未改。项目CPython3.11.9 x64／pywin32、gsudo2.6.1.0保持原环境，开发及所有SDK主体普通权限。

## 实现与入口核对

- 复用原文件请求队列、身份／登录校验、窗口、去重、历史和普通控制器。增加登记的Start／Close／Gui动作；必要gsudo在该控制器的实际子进程调用链中执行。普通脚本经scripts/test_process.ps1或scripts/managed_test.py请求，未接入不另建临时缓存。
- 正式产品／源查询／输入run保留固定参数适配，内部启动和GUI请求串行处理，不等待自己所在run结束。SDK高权限夹具仅提权原生服务端启动，移除原High Python整段启动辅助路径；普通Qt维护、源夹具写入、SDK主体、桥接、采样和截图保持Medium。
- 定义绑定原生EXE内容、精确cases／环境、桌面、输出、预算及固定助手摘要。独立scope批准定义摘要，元数据本身不能授权；新增普通case和同类目标不修改核心。旧--gsudo-cache明确拒绝，原有三个CLI不再创建独立／嵌套缓存。
- Close／Gui仅针对本控制器拥有的PID、创建时间、EXE和桌面；固定助手再次核对。清理提前预留20秒，缩短时按新截止重新安排，硬到期不因活动命令延长。身份结果不明保留记录，拒绝盲目重放。
- 日志增加入口类型、定义摘要、归属及请求／结果关联，沿用带时间的原生观测和有界去私人化字段，不据原生exit推断弹窗次数。说明及可复制接入例见[技能引用页](../../.agents/skills/qkeymapper-gsudo-cache/references/test-actions.md)。

## 选定结果

| 检查 | 实际结果与材料 |
|---|---|
| 普通确定性回归 | 原67＋新增20，共87项通过。覆盖新case／新原生目标、摘要独立批准、非法参数／环境／桌面、撤权、助手变化后重新登记、并发／去重、run内部与外部请求、实际解释器映像、PID复用、未知存活、缩短／到期／off、迟到结果及升级保留上限 |
| 公共进程助手 | scripts/test_test_process.ps1两次选定普通私有桌面通过：空桌面、原生启动、实际desktop、字面参数／环境、身份守卫、正常exit0及缺失desktop拒绝；最终材料out/cache72/public-process-final |
| 原普通输入维护自测 | 18项通过，仅私有接收窗口／窗口消息，无系统输入。测试中的预期显示观测失败打印UNVERIFIED，不据此判断Default或物理显示器 |
| 技能结构 | skill-creator quick_validate通过；项目锁内无PyYAML，校验器使用out/cache72/validator解包的6.0.2，不安装到项目venv、不改依赖锁 |
| SDK高权限夹具 | 既有Qt6维护EXE out/tests/mcp_control/qt6-release/release/QKeyMapperMcpControlTest.exe；正式SDK12项通过，实际服务端High／同关联登录，SDK和桥接Medium。父控制器13512；out/gsudo_cache_acceptance/cache72-20261025/sdk-high/results.json |
| 新维护目标及新case | 原manifest的out/gsudo_cache_acceptance/cache72-20261025/maintenance-runtime/QKeyMapperCache72Maintenance.exe，来自最新Qt6 Release维护构建，SHA e2b17109b2b7e36f03ddeb233eaa8830a28fde63ad33488740b4eb07129292db。普通公共Start、相同控制器13512，两种case保存14／独立冷读11均通过，实际High及Win32原生exit0；maintenance-results.json和*-win32目录 |
| 原整套run及内部公共请求 | 最终控制器27744，普通Python＋普通PowerShell guardian＋High自有产品／固定GUI＋普通正式SDK。out/mcp_product/cache72-final-runtime/QKeyMapperMcpAcceptance_cache72_qt6.exe，三个独立私有桌面实例正常退出，26项通过；覆盖GUI配对／配置切换／冷启动、正式MCP基本添加和真实120秒预览到期。out/mcp_product/cache72-warm-final/results.json及4acc890086ad4986aa9d09a18d955d61-job.log。未使用--gsudo-cache，无自锁或嵌套缓存 |
| 真实缩短与拥有关系清理 | 用户明确允许为验收调整窗口后，27744对仅本次自有空闲产品的窗口缩为35秒；约15秒进入正常关闭，原生exit0，硬截止后cache_available=false，owned_targets与unresolved_starts为空，last_error=null。expiry-results.json、shortened.json及对应历史；没有真实输入或Default场景 |
| 恢复原窗口 | 测试后新普通控制器6920，创建134374054638392661，instance f0feb7a8f8e04d58aa5b7ebeb3727640，实际缓存可用；恢复到原2026-10-25 21:07:50.203266 +08:00，未重算五小时。restored-window.json；本机当前状态应再查status，不跨机继承 |

原始材料均在本机忽略的out，不随Git同步。最后普通检查记录及控制器历史在out/cache72-stage和out/gsudo_cache/history。原始日志可能包含本机路径，交接只保留必要摘要。

## 发现、修正和失败证据

1. 首次升级旧29236前已确认无作业，旧缓存正常关闭。新开启返回原生exit1且空日志；用户说明当时不在PC旁并明确要求再试。未认作成功或自动重试，后续13512开启成功，截止仍为原时刻。证据out/cache72-stage/upgrade.json、retry-upgrade.json；不把该尝试当作UAC拒绝专项通过。
2. 首轮维护观察器使用PowerShell包装器ExitCode，读到null，虽然C++结果已成功仍未计通过。改用既有普通Win32 QUERY_LIMITED_INFORMATION／SYNCHRONIZE句柄后复跑，两个原生exit0。首轮日志和结果保留在settings_serialization，成功在*-win32。
3. 首轮整套run揭示注册时绑定venv启动路径与产品观测实际Python映像不一致；关闭还可能命中辅助caption窗口。失败产品经拥有关系核对后正常清理。修正为实际控制器进程映像及优先既有QKeyMapper主窗口，增加针对性回归，在27744的最终26项及真实到期组复核。首轮aad2f316f7824753a7d569913a90ae6e-job.log和out/mcp_product/cache72-warm保留，不能计为通过。
4. 13512到27744升级及真实到期后的6920恢复均重新确认原生缓存可用；旧PID缓存先关闭再创建新PID，保留原截止及剩余上限。定义／拥有关系不迁移，恢复后测试定义需按当前产物重新登记。

SDK12及维护25先于最后的run注册映像／Close窗口选择修正；其独立SDK注册、正常quit及自退出维护路径未受两项修正影响，按该范围复用，不冒称最终二次复跑。最后run和到期组覆盖两项实际修正。

## 保留边界

未重跑完整源查询组、Default真实输入／性能、Qt5产品、Win11、长时或休眠；未改变这些已有证据及门槛。High私有桌面PrintWindow仍无法可靠截图，正式产品脚本保留UNVERIFIED，没有给截图或显示器效果新增通过结论。UAC提示／拒绝专项、独立系统RunAs及原生缓存服务异常时的弹窗次数不由本组证明。

关闭仅尝试自有进程正常退出；无GUI挂起、控制器异常退出或无法核实身份时可能需要显式恢复，不自动接管旧进程或强杀。登记前仍由Agent核对实际构建来源、部署和已有授权。CACHE72是开发工具的独立切片，MCP显式保存／映射冷启动／启停完整闭环及其余能力仍按主计划推进。
