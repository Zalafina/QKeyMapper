# qkeymapper-gsudo-cache 使用与日常运行复查

本技能管理本工作区专用普通测试控制器的 gsudo PID 缓存，减少已授权固定测试的重复UAC。默认五分钟，单次窗口最多24小时；缓存时长不增加测试权限，也不恢复暂停的MCP开发。

[技能入口](../SKILL.md) · [控制脚本](../scripts/cache_control.py) · [历史日志实现](../scripts/cache_history.py) · [唯一主计划](../../../../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md) · [已有验证与观察边界](../../../../handoff/qkeymapper_mcp_control/gsudo_cache_pending_validation.md)

本页是说明文档，不维护第二份开发进度。具体检查点和覆盖结论只在唯一主计划及其验证索引维护。

## 日常使用

在Codex聊天中使用：

| 输入 | 含义 |
|---|---|
| `$qkeymapper-gsudo-cache status` 或 `当前状态` | 只读查看控制器、截止时间、原生观测及日志状态；不存在时不创建 |
| `$qkeymapper-gsudo-cache 30m` | 从现在起设为30分钟；已有控制器则复用PID，可能延长也可能缩短 |
| `$qkeymapper-gsudo-cache 5h` 或 `5小时` | 同理设置五小时窗口，首次或失效后可能需要UAC |
| `$qkeymapper-gsudo-cache until 今天19:30` | 按北京时间解释，必须是未来24小时内的时间 |
| `$qkeymapper-gsudo-cache 直到明天0点50分` | 支持中文日期和时分，该位置的“份”也兼容为“分” |
| `$qkeymapper-gsudo-cache restore` 或 `恢复默认` | 当前窗口重设为剩余五分钟；无控制器时不创建 |
| `$qkeymapper-gsudo-cache off` 或 `关闭缓存` | 停止新任务准入并关闭本技能缓存，保留既有安全清理 |

时长支持正整数和s／m／h、秒／分钟／小时；纯数字按分钟。明确日期可写 `until YYYYMMDD HH:MM` 或 `until YYYY-MM-DD HH:MM`。不接受过去时间、零／负值、超过24小时或自动顺延；表达含糊时先澄清。

兼容真正送到Agent的 `/qkeymapper-gsudo-cache ...` 文本，但技能没有注册App原生斜杠命令；界面拦截时使用 `$`。引用示例、询问格式或加载技能本身都不表示授权创建缓存。

开发、Git、构建、SDK、桥接、注入和截图保持普通权限。只有已有明确授权的必要测试启动／关闭及固定GUI动作使用gsudo。新控制器默认测试范围为空；时间调整不自动登记真实输入、驱动或GUI授权。

## 日志在哪里、何时记录

每台PC在其当前仓库根目录下独立保存：

```text
out/gsudo_cache/
├─ state.json                    最近状态，会覆盖
├─ closed.json                   最近关闭记录，会覆盖
├─ requests/ 和 results/          控制通信与有界请求结果
├─ logs/                         原有原生／固定任务日志
└─ history/
   ├─ events-YYYYMMDD-0000.jsonl  精简日常历史，一行一个JSON对象
   └─ events-YYYYMMDD-0001.jsonl  同一天达到大小上限后换文件
```

正常使用受管控制器时自动记录，无需另开监控进程或定时任务。无控制器的status／help不会为了记录而创建控制器或历史目录。尚未启动记录功能的旧控制器需要自然结束后由新控制器加载新实现；不为启用日志擅自重启活动测试。

正常status不逐条写历史；关键状态变更即时记录，控制器存活时每60秒至多一条heartbeat，不补造间隔内的心跳。摘要只读取控制器自身时间与资源，**不调用gsudo状态／提权探针、不续期、不保活**。原生缓存状态来自既有操作或状态查询，并保留实际观测时间。

每个摘要文件最多约1 MiB；只管理严格匹配自身命名的JSONL文件。保留期限30天、总量20 MiB，以先达到的约束淘汰最旧摘要。轮转在写入时进行，控制器停止后不另开清理服务。原有logs／results及其他out材料不由本功能清理；调查前先保留所需时间段，复查不能忽略已轮转的历史缺口。

日志留在本机忽略的out，不提交Git或自动上传，也不跨PC继承缓存、PID或凭据。

## 关键记录如何理解

| 事件／字段 | 解释 |
|---|---|
| `controller_start` | 普通控制器进入运行；不代表已经创建缓存 |
| `request`、`cache_request` | 收到变更／提出原生申请；不代表申请获准或窗口已应用 |
| `request_result` | 请求结果，关联request_id；set的ok／action／generation描述实际应用结果 |
| `job_start`、run的`request_result` | 固定任务种类、预算及退出结果，不含完整命令、配置或输入内容 |
| `cache_off_checked` | 原关闭操作后的实际原生观测；只在ok=true且cache_available=false时确认该次检查不可用 |
| `controller_finish` | 已走到控制器finally清理末段；cleanup_complete要求原生不可用、无未结任务和既有异常。不是事后采集的进程exit 0证明 |
| `cleanup_failed` | 清理未获确认，不生成成功finish；需结合状态和原有日志调查 |
| `heartbeat` | 会话存活、剩余窗口、当前任务及可获得的自身资源；不证明原生缓存在整个间隔持续可用 |
| `instance`、`pid`、`process_created`、`sequence` | 区分会话及PID复用，序号仅在同一instance中比较 |
| `machine_id`、`login_tag` | 机器名／登录身份的摘要标记，辅助分组；不是授权凭据或全局硬件身份，合并日志还应保留来源PC说明 |
| `os_version`、`python_version`、`gsudo_version` | 实际系统／Python及gsudo文件版本；缺失时不能猜测环境相同 |
| `code_version` | 三个实现脚本cache_control／cache_core／cache_history的内容SHA-256，仅在启动时读取；不代表整个仓库或Agent行为版本 |
| `timestamp`、`elapsed_seconds` | 北京时间记录时间与单调时钟会话经过秒数，避免仅凭系统时间判断长时覆盖 |
| `generation`、`deadline`、`previous_deadline`、`remaining_seconds` | 实际窗口代次和截止；requested_deadline只是请求，不等于生效 |
| `cache_available`、`native_checked_at` | 最近原生观测及其时间；没有观测时为null，旧观测不得冒充当前事实 |
| `native_exit_code`、`requested_window_expired` | 原生申请结束及是否已错过请求窗口；原生exit 0不等于窗口安装成功，也不能推出出现过几次UAC |
| `close_reason`、`deadline_expired` | 关闭／超时原因与实际到期事实分别记录 |
| `scope`、`input_flags` | 已登记固定入口和布尔权限摘要，不含授权reason的私人文本 |

不记录令牌、SID／用户名、完整私人路径、异常对象／完整消息、映射内容或通用命令。采用固定字段白名单；详细产品调查仍看对应固定测试证据，健康摘要不能替代实际输入效果验收。

日志写入失败不会改变缓存准入、时限或清理逻辑；`status`／关闭记录中的 `history.available`、`missed_events`、`last_error` 显示可观察到的记录失败。后续写入恢复会保留累计missed_events和事件序号，不能因此把此前缺口算完整。available只表示最近写入状态，最初尚未尝试时为null。整段无法写入、进程骤停及轮转可能没有完整记录，零missed_events也不证明全部历史完整。

客户端在参数解析阶段拒绝、未送达Agent的文本、客户端等待超时及UAC弹窗次数不由这份控制器摘要完整观测；需要时结合原请求ID、CLI结果和已有证据。摘要不是授权来源、去重数据库或自动补执行依据。

## 日常观察替代独立长时排期

已有专项测试仍保留；其他PC、长时及自然休眠恢复改为日常使用中积累、之后按实际覆盖复查，不为凑样本专门开启长缓存。只在发现异常或关键证据不足影响使用时做最小必要补验。

- Win11覆盖必须来自该PC实际OS记录、窗口成功应用、相关固定任务及关闭核实，不能沿用Win10结果或只看启动／status。
- 设置五小时后很快restore／off不能算五小时驻留；按generation之间的实际经过时间、自然任务结果和原生观测判断。
- 心跳空档可能是休眠、进程繁忙、异常或记录缺失，不能单独证明休眠。需要时由用户提供自然休眠／恢复的大致时间，对照恢复后的操作及截止行为。
- 缺finish可能是仍在运行、异常退出或历史被轮转，先区分；不要因当前PID不存在补记“上次清理通过”，也不接管旧PID缓存。
- 最终结论使用“在所列已覆盖场景中符合预期”“需要调查”“未覆盖／证据不足”，不使用“全部场景保证正常”。

## 可直接复制的运行状况复查提示词

```text
请只读复查当前QKeyMapper工作区 qkeymapper-gsudo-cache 技能的日常运行状况。
读取技能SKILL.md、references/usage.md和MCP唯一主计划；先列本机out/gsudo_cache/history的现存时间范围、格式及记录完整性，再按来源PC／machine_id、系统、代码版本、登录标记和instance分组。先汇总，再定向读取异常，不全量粘贴日志。
核对缓存创建／窗口应用、延长、缩短、restore、到期、off、任务结果和清理。重点检查到期后是否新派发任务（既有安全清理可以继续）、关闭是否有实际核实、任务／控制器是否遗留或结果未知，以及真实驻留时长和资源趋势。区分正常取消、预期拒绝、业务失败、异常及日志不足。
不要把cache_request、原生exit 0、旧native_checked_at、心跳或没有报错等同于缓存持续可用／全部正常；不要推断UAC弹窗次数。按代次识别提前restore／off。没有真实Win11／长时／休眠证据就列未覆盖；心跳空档不是休眠证明。
检查序号跳跃、missed_events、缺开始／结束、损坏行及轮转缺口；正在追加的尾行可稍后重读一次，不直接当成事故。现有state／closed只能作为带观测时间的补充。不同进程的CacheAvailable不能证明目标控制器已关闭。
给出“有充分证据符合预期／需要调查／未覆盖或证据不足”三类结论。异常引用文件、时间、instance、request_id和最小必要补验建议，不复制私人数据，不因日志缺失推定操作没发生或补执行。
本轮不向控制器提交新请求，不创建／延长缓存、不申请UAC、不启动产品、不修改源码或系统配置、不接管旧缓存，也不恢复MCP开发。若日志尚未启用或资料不完整，明确说明结论限制。
```

实现变更需要本地普通回归时，从仓库根目录使用已有项目Python：

```powershell
& out/mcp_bridge/venv/Scripts/python.exe -B -X utf8 -m unittest discover -s .agents/skills/qkeymapper-gsudo-cache/scripts -p 'test_cache*.py' -v
```

该命令使用模拟缓存及普通文件，不执行UAC、产品或真实输入。日常复查不默认重跑它，更不默认进行完整产品／Qt构建验收。
