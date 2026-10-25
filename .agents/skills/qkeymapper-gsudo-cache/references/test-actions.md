# 公共测试接入与新增目标

本页供Agent维护测试使用。用户仍只需用缓存技能设置时长；Agent核对本次已有授权、生成定义并登记，用户不必手写JSON。实施进度只见[唯一主计划](../../../../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md)。

## 入口与权限

| 场景 | 接入 |
|---|---|
| 普通Qt维护、SDK主体、桥接、构建、采样、截图 | 直接普通权限，不进入gsudo |
| 原manifest或驱动权限确有需要的自有原生EXE | 公共Start／Close，或Python的managed_test.perform |
| 正式产品固定GUI动作 | Product.action，经同一控制器调用既有product_ui_actions.ps1 |
| SDK高权限服务端夹具 | Fixture(elevated=True)，仅原生夹具由控制器启动；SDK仍Medium |
| 产品／源查询／输入整套验收 | 受管run及原参数，子脚本省略旧--gsudo-cache |
| 临时普通脚本、新维护目标／新case | 调用公共函数并登记目标／cases，不新增高权限脚本服务 |
| 专门测试UAC出现／拒绝 | 独立无继承缓存调用链，显式UncachedGsudo；不能从受管run内验证 |

先检查源码、构建日志及实际EXE来源，确认是本次自有原生测试目标。不能把解释器、cmd、安装器或任意shell当测试EXE登记，也不能从工作区路径或MZ文件头推导信任。完整定义摘要只绑定审核结果，不替代授权及来源核查。

## 定义

每个目标最多64个精确cases；最多32个登记目标。字段全部显式提供：

| 字段 | 约束 |
|---|---|
| id | 稳定英文数字／下划线／连字符，首字母英文，最多64字 |
| executable／sha256 | out内自有原生EXE的绝对路径与实际内容摘要 |
| cases | arguments为原生进程参数字符串；environment为精确键值对象，均有长度／数量限制。调用不允许额外参数 |
| desktop | private或Default；private仅Qkm前缀专用桌面，Default必须在已有授权范围内 |
| output_root | out内本次输出根；GUI结果必须是其下的新JSON文件 |
| maximum_seconds | 15–1800秒，包含整个目标用例与清理；启动时剩余窗口必须足够 |
| gui_actions | 已有固定助手中本次获准的动作名；不需要时为空 |
| client_executable | GUI配对实际进程映像路径，不是venv启动器路径；不需要时null |
| helpers | test_process.ps1及固定Product GUI助手的实际SHA-256，随定义一起审核 |

所有材料仅留本机out，不含凭据、私人配置、令牌或任意命令。环境只声明测试所需的项。DLL部署及Qt工具链沿既有构建恢复规则核对，EXE摘要不能证明整个部署环境一致。

Agent确认已有授权及来源后，可在普通项目Python中按下例生成并登记；路径按实际构建调整：

```python
from pathlib import Path
import sys

repo = Path.cwd().resolve()
sys.path.insert(0, str(repo / '.agents/skills/qkeymapper-gsudo-cache/scripts'))
import cache_control as cache
from cache_actions import digest, file_hash, helper_hashes

exe = repo / 'out/tests/mcp_control/qt6-release/release/QKeyMapperMcpControlTest.exe'
output = repo / 'out/tests/mcp_control/owned-case'
output.mkdir(parents=True, exist_ok=False)
value = {
    'id': 'owned-interop', 'executable': str(exe), 'sha256': file_hash(exe),
    'cases': [{'arguments': '--interop-fixture "' + str(output / 'fixture.json') + '"',
               'environment': {}}],
    'desktop': 'private', 'output_root': str(output), 'maximum_seconds': 90,
    'gui_actions': [], 'client_executable': None, 'helpers': helper_hashes(repo),
}
cache.write_json(output / 'definition.json', value)
state = cache.verified_controller()
assert state is not None and state['format_version'] == 2
scope = dict(state['scope'])
approved = set(scope.get('test_actions', {}).get('definitions', []))
approved.add(digest(value))
scope['test_actions'] = {
    'reason': '本会话已批准的自有私有桌面互操作夹具，无真实输入；已核对实际构建和部署。',
    'definitions': sorted(approved),
}
assert cache.send(state, {'op': 'authorize', 'scope': scope})['ok']
assert cache.send(state, {'op': 'register', 'definition': value})['ok']
```

例中的原生夹具需事先部署匹配Qt DLL，或把实际需要的Qt PATH精确登记到environment。源文件及工具链本身不提权。不要把示例reason当作未经核对的授权模板。

也可把审核后的范围／定义写为本机JSON，经内部CLI：

```powershell
& out/mcp_bridge/venv/Scripts/python.exe -B -X utf8 .agents/skills/qkeymapper-gsudo-cache/scripts/cache_control.py authorize --scope-file out/owned-test/scope.json
& out/mcp_bridge/venv/Scripts/python.exe -B -X utf8 .agents/skills/qkeymapper-gsudo-cache/scripts/cache_control.py register --definition-file out/owned-test/definition.json
```

## 普通脚本启动与关闭

```powershell
. ./scripts/test_process.ps1
$child = $null
try {
    $child = Start-QkmTestProcess -ExecutablePath $exe -Arguments $arguments -EnvironmentOverrides $testEnvironment -DesktopName $privateDesktop -UseGsudo
    # Run the ordinary test and wait for its documented completion condition.
} finally {
    if ($null -ne $child) { $code = Close-QkmTestProcess $child }
}
```

参数／环境必须与一个已登记case精确匹配。未指定DesktopName的原有Default行为会转换为明确Default，仍需对应定义授权。普通Start不加UseGsudo时保持原路径。

Python普通脚本可导入repo/scripts/managed_test.py并调用perform，Start请求字段为Action、Executable、Arguments、Environment、DesktopName；Close改为Action、Executable、ProcessId、StartTicks、DesktopName。StartTicks来自实际Start结果，不自行猜测。GUI还带Name、ResultFile及按需ClientExecutable。不接受任意脚本或shell字段。

Close只操作登记且仍由当前控制器拥有的同一进程，核对PID、创建时间、EXE及桌面。Qt产品优先关闭已知主窗口，其他原生夹具才回退本进程的caption窗口；不强杀其他进程。自退出目标应由普通测试持有Win32 QUERY_LIMITED_INFORMATION／SYNCHRONIZE句柄核对原生退出码；PowerShell包装器ExitCode为空不能记为0。

## 并发、更新和失败

- 控制器同一时间仅执行一个必要提权动作。整套run继续运行时，其普通内部请求使用本次作业归属令牌进入动作队列，不等待run完成。该令牌不产生测试授权；实际gsudo仍由缓存控制器发起，调用方实际Medium及同登录身份另行核对。
- 新普通case只改cases；新同类目标另登记；不改控制器核心。活跃目标须先清理，再修改其定义或助手。固定助手代码更新后重新审核helpers及定义摘要，同一控制器即可使用；不要为此另建缓存。
- 每次新启动需要整个预算。缩短窗口会提前安排自有目标清理；保留20秒清理时间，活动目标不能直接缩到不足20秒。到期关闭本PID所有缓存，不因持续命令延长授权。
- 缓存缺失／失效、target_not_registered、scope_not_authorized、busy、helper_changed等分别处理；不会自动改为直接gsudo。结果未知先查result及本机记录，不用新ID重放。未知存活状态不会被记录为已清理；PID复用拒绝关闭。
- 控制器升级仅在无作业／自有进程时运行upgrade；核实旧缓存关闭，新PID沿用旧截止及剩余上限，可能一次UAC。失败不重新计算时长，先确认原审批／结果；跨PC不继承PID、状态、定义或拥有关系。
- 发生控制器异常退出、无GUI的挂起目标或原生服务故障时，报告精确未清理范围，按已有安全恢复规则处理；不自动接管旧PID或用全局-k清除别人的缓存。

## 覆盖例外与日志

首次缓存开启、已失效后的恢复、系统独立RunAs／安装器、正式产品自己发起的提权，以及专门UAC交互验收仍可能需要人工确认。原生服务在可用性检查之后异常失效也不能保证无弹窗。不会改变产品manifest、产品提权逻辑或自动安装驱动。尚未使用公共函数的旧临时脚本先接入，不为其另开缓存。

status显示registered_targets、owned_targets、current_action及unresolved_starts。历史按控制器instance／PID和request_id关联入口类型、定义摘要、可用性观测及错误／清理结果。只读复查方法见[使用说明](usage.md)；缓存摘要不能证明实际弹窗次数、映射性能或真实输入效果。
