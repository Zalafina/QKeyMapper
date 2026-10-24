---
name: qkeymapper-gsudo-cache
description: 在 QKeyMapper 工作区查看、创建或调整专用普通测试控制器的 gsudo PID 缓存窗口，支持中英文时长、截止时间及恢复五分钟；用于临时离开电脑时减少已授权测试的重复 UAC。
---

# QKeyMapper 临时缓存

使用本技能处理缓存状态、时长、截止时间、恢复默认及关闭。缓存指令不恢复暂停的开发，也不新增真实输入、驱动或 GUI 授权。技能被加载、用户引用用法或只问格式时，不执行缓存变更。

## 使用入口

显式入口为 `$qkeymapper-gsudo-cache …`；兼容实际送达 Agent 的 `/qkeymapper-gsudo-cache …` 文本和清晰的自然语言请求。不会注册 App 原生斜杠命令；若界面拦截 `/`，使用 `$`。

从仓库根目录用本机已有的普通权限 Python 运行脚本，优先使用 `out/mcp_bridge/venv/Scripts/python.exe`（既定 CPython 3.11.9 x64＋pywin32）。缺少环境时按主计划的环境恢复规则处理，不安装全局包。脚本位置为 `.agents/skills/qkeymapper-gsudo-cache/scripts/cache_control.py`。

```powershell
& out/mcp_bridge/venv/Scripts/python.exe -B -X utf8 .agents/skills/qkeymapper-gsudo-cache/scripts/cache_control.py status
```

把用户请求作为独立参数传给脚本，不拼接为 shell 代码：

| 请求 | 行为 |
|---|---|
| 空参数、status、当前状态、状态 | 只读查询；不存在时明确显示，不创建控制器 |
| 5m、5h、5分钟、5小时、10s | 从现在起重设窗口，可延长或缩短；纯数字按分钟 |
| until 20261025 19:30、直到今天19:30、直到明天0点50份 | 北京时间截止；该位置的份兼容分 |
| restore、恢复默认 | 当前重设为剩余五分钟；不存在时不创建 |
| off、关闭缓存 | 关闭本技能管理的缓存，停止派发新测试，保留既有清理 |
| help、帮助 | 显示示例 |

正整数窗口最长24小时；兼容大小写、空格和全角数字／冒号。不接受过去时间、无穷、零／负值或自动滚到明天；不截断。含义不明确时只问一个短问题，保留当前设置。第一次开启或原缓存失效可能出现 UAC，须用户实际批准；没有确认生效就不能报告已延长。

## 反馈与失败

以脚本实际结果说明：新建了控制器，或复用同一 PID 延长／缩短／重新启用，或恢复默认、关闭、等待审批／失败。给出实际 PID、北京时间旧／新截止时间、剩余时间、当前固定作业和已批准范围。未管理的本机缓存不猜测期限、范围或擅自接管。

只查询状态不会保活或延长窗口。gsudo 的原生 duration 是空闲超时，实际最终限制由控制器单调时钟截止监视完成，到期关闭本 PID 全部缓存；持续执行命令也不延长授权。缩短不假定原生 cache on 覆盖了旧服务。

长命令使用可返回会话 ID 的工具执行并持续报告进展，不让工具等待阻塞沟通。请求超时会返回 request_id；用 `result <request_id>` 查询保存结果，不重发变更。结果为历史事实，当前有效状态须另查 status；控制器退出／结果淘汰后未知，不假定操作未发生。UAC 取消（999）不自动重试或替用户批准。

## 让后续测试复用该控制器

调整时间只保留原范围，新控制器默认范围为空。Agent 根据会话中已经批准的具体作业登记范围；授权依据不足时询问缺少的范围，而不是要求用户重复已经批准的事项。reason 是授权证据说明，不是创造授权的手段。

内部管理接口（非时间指令）：

- `authorize --scope-file <JSON>`：以完整范围对象替换已登记的固定入口。键为 product、source、input_regression 或 identity_probe，值含明确的 reason；输入回归还可含已批准的布尔项 allow_default、allow_focus_click、allow_late_forced_up。普通产品／源查询不得包含真实输入权限。状态会显示该范围。
- `run --job-file <JSON>`：内容仅为 kind 与 arguments。kind 对应已登记入口；固定适配器组装现有脚本参数，禁止任意命令／shell／脚本。产品及源查询参数沿现有 CLI，输入回归使用显式授权标志，不自动追加。
- `identity_probe` 的 arguments 必须为空，只读取固定进程令牌，供本技能验收。其他固定入口复用现有正式产品、源查询和输入回归；单次预算含清理分别为630／630／1230秒，窗口不足明确拒绝，不自行延长。

已授权测试应经本控制器运行，子脚本不加 `--gsudo-cache`，以继承父控制器的缓存；不要每个短命令另建 PID 缓存。开发、Git、构建、SDK、桥接、注入、采样及截图始终普通权限，仅原有必要启动／关闭及获准固定 GUI 动作经 gsudo。scope 不能用于运行这些开发命令提权。

## 生命周期与交接

每工作区／当前用户登录只允许一个受管普通控制器。核对实际 PID、创建时间、解释器和 Medium 权限；不按进程名或旧 PID 猜测。运行记录、通信和日志在忽略的 `out/gsudo_cache/`，不随 Git 或跨 PC 继承；技能和测试源码随 Git。

任务正式结束或用户要求关闭时执行 off，并确认 closed／缓存不可用；普通消息、Fast 切换或短暂停顿不擅自重启、延长缓存。到期不再派发新测试；活动测试仍按既有逻辑清理，异常清理可能需要用户 UAC，不能谎报已清理或强杀用户实例。首次创建、缓存失效和异常恢复不能保证无需 UAC。

更新进度／验收到本任务唯一主计划，不在技能内维护任务进度。MCP 主计划入口：`handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md`，缓存边界见 CACHE-40。不要恢复 `out` 中已经结束的旧临时控制器，不修改全局 CacheMode／CacheDuration，不使用 PID 0、无限期或全局 -k。

本轮实测为Win10／gsudo2.6.1，仅完成基础检查点；三个固定入口的真实复用、Win11、长期驻留及更多异常待补，见 `handoff/qkeymapper_mcp_control/gsudo_cache_pending_validation.md`。换PC或gsudo版本变化后先核对环境并补短时原生场景，不直接沿用本机结果。
