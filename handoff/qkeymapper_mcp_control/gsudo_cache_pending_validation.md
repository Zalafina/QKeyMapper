# 缓存技能：已验范围、待补验收与接续提示词

[唯一主计划](qkeymapper_mcp_automation_plan_codex.md) · [技能入口](../../.agents/skills/qkeymapper-gsudo-cache/SKILL.md)

本文件仅列验证范围、证据和观察边界，不建立另一份任务进度。2026-10-25已完成69的本机补验；用户随后决定并要求实现日常日志观察，替代单独跨PC／长时补验排期。70的记录功能与说明已完成，未覆盖场景不改记通过。MCP开发仍暂停、B1草稿不应用，最新状态以唯一主计划为准。

## 基础检查点证据（6bfacd88，按不变路径复用）

- 来源：PC-W10／Windows 10 19045 x64，基线 `3a12a434`＋本轮技能、导航及记录；产品、桥接源码、依赖锁和 manifest 未改。
- 环境：项目 CPython 3.11.9 x64 venv、现有 pywin32、gsudo 2.6.1。Windows 进程实际映像是系统 Python，虚拟环境入口另行核对；不单按 python.exe 名称接管。
- 普通权限26项单元测试通过：命令／时长／日期、跨午夜和“份”兼容、非法及过大值、单调时钟、延长／缩短／恢复、忙及失败、重复／淘汰请求、旧代请求、PID复用／权限拒绝、范围及预算、活动固定作业期间调整。包含模拟 gsudo，不把它称为真实 UAC 结果。
- 实际普通进程验收通过：无控制器状态、两个客户端同时创建但只产生一个控制器、Medium身份、跨独立命令存活、无缓存状态和正常关闭。此组没有 UAC。
- 真实缓存 basic 通过：同一PID创建、缩短、延长、跨过旧截止时间仍可用、设置5小时、恢复剩余5分钟、固定只读High身份探针及同登录核对，最后缩短到5秒，到期关闭包括旧长窗口的全部自有缓存。**没有驻留5小时**，不能据此证明长时间持续可用。
- crash 通过：核对身份且无活动作业后，仅终止本轮自有普通控制器，观察对应缓存随PID退出清理。没有终止产品或用户进程。
- cancel-02 通过：实际UAC取消／原生999，返回needs_uac，没有有效窗口、没有自动重试，自有缓存清理。cancel-01因用户误点“是”未满足负例，脚本断言失败后正常关闭；失败及清理证据保留，不记为取消通过。
- skill-creator官方结构校验通过。其PyYAML原先缺失，随后仅在 `out/skill_validation/pyyaml` 准备6.0.2工具依赖；未修改全局环境或MCP固定依赖。

本机原始材料不随Git：`out/gsudo_cache_acceptance/20261025-{ordinary-01,basic-01,crash-01,cancel-01,cancel-02}/`、`20261025-unit-final.txt`、校验依赖日志；`out/gsudo_cache/` 为通信、控制器及gsudo日志／关闭记录。完整性、实例及范围按实际结果核对，不复用另一PC的PID或运行记录。该基础检查点当时未启动QKeyMapper，没有真实键鼠、驱动、产品GUI、产品性能或Qt构建验收。

## 69补充证据（b2dd1007，PC-W10，2026-10-25）

源码为6bfacd88＋本轮技能差异；测试助手已独立提交 `a5097898`。最终技能检查点见Git HEAD。环境仍为项目CPython 3.11.9 x64／pywin32、gsudo 2.6.1。产品复用MAP-67-A的Qt6.8.3／MSVC普通Release，原构建日志 `out/mcp_bridge/map67-product-qt6.txt`；实际EXE SHA-256为 `f19b26a803a2c4eb2889dfe710ac37ef0cb91a0ab60d0c5bcf57f7aa69de1064`，部署身份见下述artifacts.json。本轮未构建Qt，不将复用二进制称为最终全部源码的新构建。

- **普通回归51项通过**：新增异常／非对象状态、请求结果未知与历史查询不重放、旧实例／用户／登录／解释器拒绝、真实目录联接越界、source正向及参数边界、审批迟到、关闭失败、任务失败／超时和清理预算等。可控时钟复现并修复“到期后、下一轮轮询前新请求续建缓存”的问题；请求处理前即进入关闭，不能复活旧窗口。模拟用例不充作真实UAC证据。
- **最小可靠性修正**：严格校验原生UTF-8 JSON及缓存字段；损坏状态记录不接管；关闭原因与实际到期分开记录，超时和到期重叠时仍保留到期事实，迟到申请失败不覆盖原关闭原因。失败结果补 `native_exit_code`／`requested_window_expired`。相应红例保留，最终51项绿色；另验证-O禁用断言时在任何原生动作前拒绝，不能误报验收通过。
- **真实活动任务边界通过**：固定只读High探针输出身份后保留5秒可观测期；off与缩短到期时，缓存先失效、探针继续正常退出，控制器等待结果后结束，不强杀任务。active-off／active-expiry-01为本轮先完成的原生证据，后续未改变其正常输入路径，按该范围复用；不是最终源码全套原生重跑。
- **真实迟到批准通过**：pending-off／pending-expiry-02明确记录原生exit 0，但分别因已关闭／请求窗口已过期而拒绝安装，最终无缓存。01组未保存完整原生观测，核对时还发现关闭原因被覆盖；由02组补齐并复核，不按用户是否心算等待5秒判定通过。
- **未管理缓存共存通过**：只创建本轮自有普通等待进程作对照。无受管控制器时status／off不接管该缓存；新受管缓存关闭后，对照缓存仍在；最后分别清理，原生会话归零。没有清除其他用户缓存或改变全局配置。
- **三个实际固定入口通过**：同一普通控制器PID 21496创建90秒窗口、延长40分钟；110秒后仍可用，跨过初始原生空闲期限。product 12场景、source 10场景、input_regression的query单态10对A→B及F24停止通过，包含既有暖身／停止后恢复检查；SDK／桥接／接收／注入保持普通权限，只有既定产品和GUI助手High，同登录及实际桌面核实。子脚本均未加 `--gsudo-cache`，采样时仅一个原生会话。一次故意不匹配夹具在产品启动前exit 1，结果可查询、忙状态恢复，后续任务通过。
- **实际产品失去缓存后的清理通过**：source_cleanup在最后撤销／关闭阶段主动off；现场确认cache不可用但source任务仍活动，后续必要关闭使用原有UAC。10场景、产品和任务exit 0，结果可查询，控制器正常结束；没有映射启动或真实输入。
- **独立测试助手修正**：source初次及新副本复核均在ReloadCurrent出现UIA ElementNotAvailable，产品仍exit 0。改为已有受限窗口鼠标消息打开Qt自定义弹窗，保留HWND／PID／桌面／范围检查和原断言；source-click及source-cleanup均10场景通过。产品代码未改，助手修正为a5097898。Default短测的query路径不使用该配置弹窗；没有冒称Default配置切换或Qt5此助手重验。
- **短时稳定性／使用边界**：控制器混合任务驻留约14分钟，两次间隔610秒采样的工作集仅差24 KiB、私有提交相同，不据此宣布长期无泄漏或产品性能通过。官方技能结构校验、当前会话技能可发现性、元数据与中英文确定性命令路径已核对；status不创建、不保活，引用用法不视为变更授权。不宣称做过独立Agent评估或选择器UI点击。

新增原始材料仅本机out，不随Git：

- `out/gsudo_cache_acceptance/20261025-supplement-unit-51.txt`；红／绿探针在 `out/skill_validation/supplement/`。
- `out/gsudo_cache_acceptance/20261025-{active-off-01,active-expiry-01,pending-off-01,pending-expiry-01,pending-off-02,pending-expiry-02,coexist-01,source-cleanup-01}/`。
- `out/gsudo_cache_acceptance/20261025-adapters-01/`：固定job／scope、artifacts、同PID延长及关闭、资源、manager结果；相应job日志在 `out/gsudo_cache/logs/`。
- `out/mcp_product/cache69-20261025/{product-result,source-result,source-result-retry,source-result-click,source-result-cleanup}/`：前两组source失败保留，后两组通过；含topology／GUI／RPC／closed。`out/mcp_product/engine61/cache69-20261025-input/` 为Default输入、查询及清理。

产品／source在随机真实私有桌面QkmProduct_*验收，输入在Default。原有普通PrintWindow不能捕获High产品，仍记录未验像素，不提升截图权限。未做产品Qt重建、Qt5／Win11系统实测、全源静态分析、ASan或产品性能复测；本轮任务不依赖这些门槛。

最终普通只读核对7个原产品身份均已退出，status为controller=null、原生会话0；编码／新增链接／语法及官方技能结构检查通过，见 `out/gsudo_cache_acceptance/20261025-final-review.json`。测试源码、Git、SDK和截图始终未提权。

## 70日常记录功能的选定验证（PC-W10，2026-10-25）

基线b2dd1007＋本轮日志实现；只修改缓存技能／测试／说明，不修改C++产品、SDK、依赖锁、manifest、cache on/off参数或既有状态限制。环境仍为项目CPython3.11.9 x64＋pywin32、gsudo文件版本2.6.1.0，开发／检查均普通权限。

- 原51项＋16项日志回归共67项通过：请求／异步结果关联、字段过滤与去私人信息、心跳节流且无缓存探测／续期、资源读取失败、写入和最终记录失败仍清理、异常观察器不逃逸、记录恢复留缺口、30天／大小轮转只删自身文件、半行恢复、目录联接越界／文件硬链接拒绝、实现摘要变化及清理失败不记成功finish。
- 实际无缓存的普通控制器验收通过：Medium／同工作区身份核对，五条精简事件，包含完整OS／Python／gsudo文件版本与匿名标记／代码摘要；状态查询不写额外历史，off后实际控制器结束、原生会话仍为零。没有cache on、UAC、High探针、产品／GUI／Default或输入动作。
- 实际读取自身内存和CPU累计值成功。60秒心跳、长期／系统时钟等由可控时钟测试覆盖，本轮不称真实长时／休眠通过；原生产品证据仍只属于69的实际版本，未无条件重跑Qt／产品／输入验收。

本机证据不随Git：`out/gsudo_cache_acceptance/20261025-history70-unit-final67.txt`、`out/gsudo_cache_acceptance/20261025-history70-ordinary-final/summary.json`；初期65／66项、硬链接红例及工作副本位于`out/skill_validation/history70/`和原unit记录。日常历史位于`out/gsudo_cache/history/`，按30天／总20MiB轮转。

## 后续自然观察与复查

不再单独安排Win11／2–5小时等待以凑样本，日常使用后根据实际来源PC、OS、实现版本、会话／代次与原生观测列覆盖。换机环境／身份准入与授权仍保持；缓存、PID和凭据不跨机继承。缺失／轮转、心跳空档、旧cache_available、设置过长窗口或没有报错都不等于实际场景通过；休眠需实际时间或相应补充证据。只有异常或关键证据不足影响使用时选择最小必要补验，新增真实输入／驱动／GUI范围仍须相应授权。

详细字段、解释边界及可复制的[只读运行复查提示词](../../.agents/skills/qkeymapper-gsudo-cache/references/usage.md#可直接复制的运行状况复查提示词)放在技能references/usage.md，不重复维护两份正文。本文件保留验证证据，唯一主计划保留进度，说明文件不作为另一份计划。

## 72公共测试接入的选定验证（PC-W10，2026-10-25）

普通87、公共助手、输入维护18及skill-creator结构检查通过。相同控制器覆盖新增原manifest维护保存14／冷读11及SDK High夹具12；最终整套run26证明内部公共请求无自锁，真实35秒组确认拥有进程正常exit0、硬到期缓存不可用及零拥有／未定记录。两处真实接线问题修复及失败尝试均保留，SDK／维护只按未受最后两项修正影响的原路径复用。[精简证据、源码边界和路径](cache72_validation.md)。

首次开启／恢复仍可能UAC；系统独立RunAs、专门UAC提示／拒绝、控制器异常恢复不保证自动覆盖。不新增Win11、长时、休眠或真实输入性能结论，日常观察安排不变。当前状态及接续只见唯一主计划。
