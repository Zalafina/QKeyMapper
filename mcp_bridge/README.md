# QKeyMapper MCP 桥接（开发阶段）

官方 Python MCP SDK 的 stdio 适配层。已有连接／GUI 配对、能力查询、只读状态、配置／Tab 目录、安全源表／源行分页及解析器帮助、基本映射的预览／内存提交／取消／结果查询；SAVE-97接通完整保存的预览与显式提交，P1-107接本实例窗口观察、P1-108接有界真实PNG捕获、P1-109接私有后台源行设定窗打开，实际范围见[唯一主计划](../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md)。**其他设置变更、启停及输入测试尚未接入，M1仍未完成**。当前开发产品返回十三项业务能力、十四工具；未接入业务处理器的维护服务仍返回 `transport_only` 和空列表，须查询目标实际能力，不能只看工具枚举。

桥接面向 Win10／11。统一开发基线为 CPython 3.11.9 Windows x64、`mcp==2.3.0`、`pywin32==312`，C++ 对端分别使用 Qt 6.8.3／Qt 5.15.2。两台机器的逐轮验收分别记录，不能互相替代；完整运行依赖固定在 [requirements-windows.txt](requirements-windows.txt)。普通用户免安装 Python 的打包交付属于 M3，当前入口用于开发和回归测试。

## 启动与使用

开发／跨机接续统一使用本轮实测的 **Windows x64 CPython 3.11.9**（包含补丁版本），pip 24.0、setuptools 65.5.0；29 项运行依赖及其传递依赖已在 [requirements-windows.txt](requirements-windows.txt) 精确固定，开发工具由 [requirements-dev-windows.txt](requirements-dev-windows.txt) 补齐。两个 requirements 同时锁定选定 Windows x64／通用 wheel 的 SHA256，不接受源码包或其他平台 wheel。

先从 [Python 官方 3.11.9 发布页](https://www.python.org/downloads/release/python-3119/) 下载 Windows installer (64-bit)，不使用 install manager、x86 或 ARM64 文件。本轮核实的文件身份为：

| 项目 | 固定身份 |
|---|---|
| 文件 | [python-3.11.9-amd64.exe](https://www.python.org/ftp/python/3.11.9/python-3.11.9-amd64.exe)，26,216,840 字节 |
| SHA256 | `5ee42c4eee1e6b4464bb23722f90b45303f79442df63083f05322f1785f5fdde` |
| 发布者 | Authenticode 状态必须为 Valid，签名发布者为 Python Software Foundation |

下载后先检查身份；文件保留在 Git 外。不能只对下载结果重新计算哈希就把该结果当作可信基准：

```powershell
$qkmInstaller = '<本机下载的 python-3.11.9-amd64.exe 绝对路径>'
$qkmSignature = Get-AuthenticodeSignature -LiteralPath $qkmInstaller
if ($qkmSignature.Status -ne 'Valid' -or $qkmSignature.SignerCertificate.Subject -notmatch 'O=Python Software Foundation') { throw 'Python installer signature mismatch.' }
if ((Get-FileHash -LiteralPath $qkmInstaller -Algorithm SHA256).Hash -ine '5ee42c4eee1e6b4464bb23722f90b45303f79442df63083f05322f1785f5fdde') { throw 'Python installer SHA256 mismatch.' }
```

已有解释器须核对实际版本、x64 架构、`python.exe` 与 `python311.dll` 的有效 PSF 签名；本机可复用符合要求的安装，不为重建 venv 改动全局 Python。安装文件校验不追溯证明已有安装的全部文件来自该安装包。使用明确的解释器路径，在每台 PC 仓库内各自创建环境：

```powershell
$qkmPython = '<本机已核验的 CPython 3.11.9 x64 python.exe 绝对路径>'
& $qkmPython -m pip --isolated download --index-url https://pypi.org/simple --no-cache-dir --require-hashes --only-binary=:all: --no-deps -r mcp_bridge/requirements-dev-windows.txt --dest out/mcp_bridge/wheels-cp311-win_amd64
& $qkmPython -m venv out/mcp_bridge/venv
out/mcp_bridge/venv/Scripts/python.exe -m pip --isolated install --no-index --find-links out/mcp_bridge/wheels-cp311-win_amd64 --require-hashes --only-binary=:all: --no-deps --force-reinstall -r mcp_bridge/requirements-dev-windows.txt
out/mcp_bridge/venv/Scripts/python.exe -X utf8 mcp_bridge/check_environment.py
out/mcp_bridge/venv/Scripts/python.exe -m pip check
```

每条命令都须 exit 0。新建环境须使用尚不存在的 venv 目录；已有环境先校验，只有已有哈希安装证据且校验通过才复用，否则在本机重建或修复后重验，不覆盖不明目录。`--force-reinstall` 确保 venv 自带的同版本 pip／setuptools 也实际经过哈希安装，而非以 already satisfied 跳过。`check_environment.py` 只读检查解释器／平台／隔离状态和全部安装包，并拒绝缺失／非法哈希的锁条目；它不替代 pip 对下载字节的验证。对应反向用例为 `tests/test_environment.py`。使用 venv 内解释器的绝对路径启动 SDK、回归及桥接，不依赖 PATH 或激活脚本。venv、个人凭据和 out 产物不随 Git 复制到另一台 PC；虚拟环境应在目标路径重新创建，见 [Python venv 说明](https://docs.python.org/3/library/venv.html)。

上述版本是当前已验证基线，升级须在同一检查点更新锁、哈希和校验入口并重跑相关验收。wheel 哈希来自对应 PyPI 发布文件元数据，再由实际 hash-checking 下载／安装核对；原始文件和日志留在本机 out，不提交包文件。M3-B 打包工具锁及交付尚未完成，打包前固定打包器与相关构建工具，并在独立环境离线重建／启动验收。采用 pip 的[精确版本及哈希锁机制](https://pip.pypa.io/en/stable/topics/repeatable-installs/)，不引入新的环境管理框架。环境及文件身份通过不能替代两系统业务、设备及实际输入验收。

在 QKeyMapper 的 MCP 管理窗口启用接口，复制显示的 `instance.json` 路径。MCP 宿主以以下参数启动 stdio 进程；具体宿主配置格式按其要求填写：

```text
<checkout>/out/mcp_bridge/venv/Scripts/python.exe
<checkout>/mcp_bridge/run_bridge.py
--discovery-file <管理窗口显示的 instance.json 绝对路径>
--executable <本机信任的 QKeyMapper.exe 绝对路径>
```

`--executable` 是用户／宿主明确选择的信任目标，不能自动采用发现文件中的 EXE 路径。两台 PC 分别填写本机路径并重新配对，不复制客户端凭据或构建产物。不要把密钥加入参数、环境变量或仓库。

1. `qkm_connect` 使用已有配对。没有凭据时状态变为 `pairing_required`。
2. 首次连接调用 `qkm_request_pairing`，随后用户在 QKeyMapper 核对系统验证的程序路径、PID 和权限并批准。客户端名称仅为自报信息，不能批准自身请求。
3. 上述调用立即返回，`accepted` 仅表示已接受连接作业。通过 `qkm_get_connection_state` 等待 `busy=false` 和 `status=connected`；失败检查 `error`。待批准期间 SDK 查询仍可响应。
4. `qkm_get_capabilities` 经正式认证管道重新查询实际能力。响应含 `ok`；`ok=false` 是业务请求失败，不能把 SDK 已收到工具回复当成操作成功。
5. 能力包含 `state.read` 时，`qkm_get_state` 读取本次 GUI 线程快照：界面选择对应的配置组／类型、dirty、当前 Tab、GUI 监视状态、Worker 原子状态采样和实际停止快捷键。另含 `mapping_load_context`：最近映射加载／空白重置的代次（十进制字符串）、结果和实际解析的配置组；加载未完成时 context_busy，非法映射导致加载失败时 outcome=invalid_mapping_data、group=null，不沿用旧来源。它记录最后一次加载，不能证明后续编辑未变、全部数据来自同一磁盘组或已保存；configuration 仍只表示界面选择。`schema_version=1`；不可用的 `business_revision=null`。初始静止或当前清理读取许可无效时 `stop_completion=unobserved`；仅在 IDLE／STOPPED、加载成功且清理代次与当前活动修订匹配时报告 observed，它是当前只读清理上下文，不是异步操作结果或编辑权限。Tab index 只作观察值，不能用作稳定编辑目标；GUI idle 或 Worker stopped 也不能证明停止清理完成。 GUI 停止协调期间摘要可报告 gui_state=stopping、monitoring_enabled=false；这表示前台窗口监视已停止，不表示 Worker／发送任务已经排空，stop_completion 仍为 unobserved。
6. 能力包含 `catalog.read` 时，`qkm_query` 的 configurations／tabs 使用目录分页。首轮 offset=0，limit 为 1–100（默认 50），后续页携带 catalog.revision 为 catalog_revision，并使用实际返回的 next_offset；next_offset=null 表示结束。相关目录、映射加载代次／结果或授权上下文变化返回 stale_catalog，应从首轮重新查询。相同目录／上下文的重复读取及同配对重连保持修订号稳定；不保留目录缓存或过期定时器。
7. 能力包含 `source.read` 时，先查询 mapping_sources，后续页传返回的 source_snapshot.revision 为 source_revision；查询 mappings 必须同时传该 source_revision 和所选 source_tab_ref。源表来自实际普通／共通内存表，包含非当前 Tab，附加显示的共通行不会重复列为普通源行。每个源行含 source_ref、source_row、fields、derived 和 lock_state_sample；fields 沿用现有映射码字段名及类型，派生字段只采样，不重新计算或回写。引用属于本次源快照，不能解析为写入授权；源内容、加载／结构代次、实例或配对权限改变时返回 stale_source，须重新查询。完整快照上限 4 MiB，单页还受现有通信帧限制，使用实际 next_offset 分页；单项超限明确拒绝。
8. 能力包含 `help.read` 时，`qkm_get_help` 的 topic 为 `basic_mapping`、`original_keys` 或 `mapping_keys`。基础帮助给出 A→B 语法示例、组合／序列分隔符、等待及连发的毫秒范围／默认值、映射项重复模式及次数含义（与宏 xN 的约束分开）；等待控件的 0 表示不追加后缀，字面 ⏱0 非法。键名直接来自产品解析器的有效列表，去空／去重并按大小写敏感顺序分页；offset 从 0 开始，limit 为 1–100、默认 50，使用 next_offset 续页。返回 reference_scope=parser_help_only，不校验完整映射或设备是否可用，不授予编辑权限；实际可调用操作以能力发现为准。此帮助不读取可变映射源，也不停止映射。
9. `qkm_disconnect` 有界取消等待并断开，保留已存配对。它不启动／停止映射，也不撤销服务器权限。撤销使用 QKeyMapper 的管理窗口；接口关闭时也能查看及撤销持久化配对，无需临时启用接口。
10. 能力包含 `windows.read` 时，`qkm_query({"entity":"windows"})` 只观察当前已认证实例的 main／item_setup 两个固定角色；仅接受 entity／offset／limit，offset 为 0–2，limit 为 1–100。返回实际 EXE／PID／创建时间／GUI 线程桌面、Qt／原生可见、最小化、前台精确采样、逻辑尺寸／DPR及不透明 window_ref。未创建原生窗口时引用为 null，不为查询调用 winId 创建或显示窗口。该查询只需 Read，不授予或调用显示／截图，也不读取其他进程窗口。

窗口引用仅属于实例／配对权限代次及本角色窗口生命周期，关闭／隐藏重开、呈现／原生窗口重建、销毁或身份／权限改变后失效；正常返回 Agent 导致的焦点变化不使其失效。`foreground_exact` 和 `foreground_in_instance` 是本次采样，不代表已请求激活或已核实前台显示。引用不证明映射源／场景未变，也不授权操作；截图独立核对业务源及源Tab；109提供私有后台单项打开，普通Default显示确认和其他角色仍待。

管理窗口的“显示窗口”与“截取窗口图像”是各自独立的权限，默认不授予，也不包含在 Read／Configure／Run mapping 中。新配对可单独勾选；已有配对在接口启用后，选中实际客户端，点击“授权窗口权限”追加批准，已有权限和配对密钥保留。批准实际新增权限后权限代次递增、该客户端的旧连接断开；使用 qkm_connect 明确重连取得新权限，无需重新配对。取消或没有新增项不写盘；保存失败关闭 MCP，原授权未成功改变。旧引用／预览属于原权限代次，应重新查询。

DEMO-P1-105交付授权基础，107／108新增窗口观察和捕获，109接私有后台单项打开；Default显示确认仍待。不能因获得权限就假设接口存在，须查询实际能力。写有新权限的配对文件会被不认识这些范围位的旧产品拒绝，升级／部署应使用匹配版本，不自动删存储或降级权限恢复连接。

目录只提供配置名称和当前 Tab 元数据；display_name 来自下拉框条目文本，空白项可为空字符串，通过 kind=draft 区分，不能仅按显示名定位。saved_profile 的内容未加载查询，未附着共通表也有 attached_to_ui=false 的目录记录，这不表示屏幕可见性。selected_configuration／is_selected 只表示界面选择。catalog_ref 和目录修订号不是映射编辑目标或完整业务修订，不能据此提交变更；权限仍由认证与逐请求检查决定。

源查询要求 GUI idle、Worker stopped、最近加载成功，且读取基准为初始静止（read_basis=pristine_idle，cleanup_generation=null）或仍有效的完整清理许可（read_basis=completed_cleanup，cleanup_generation 为本实例清理代次字符串）；否则 context_busy，不主动停止映射。生命周期请求、按钮排队、任务构造和同步／包装在相关处理前记录活动修订，与在途复制互斥。只有完整停止协调、所有者／发送池及 GUI 尾部完成后，才在互斥内记录清理对应修订；任何后续活动或饱和值均拒绝该许可，不能仅凭 IDLE／STOPPED 或零计数恢复。相同映射跨启停后也必须重新查询引用。此许可只用于复制源数据，不能授权删除／修改源表或替代正式启停操作结果。source_snapshot 的配置选择和最近加载记录不能证明每行磁盘来源；未附着的共通备份可能保留自之前的配置，origin=source_memory 只证明当前内存来源。源修订仍不是完整业务修订。

连接状态是最近一次核验结果；产品状态须显式重新查询，Worker 字段是跨线程的单次采样。当前十四个工具的 schema 均声明 `additionalProperties=false`，参数严格检查类型与范围，未知字段在进入工具函数前拒绝；C++ 仍独立检查业务请求参数。应用未启动／接口未启用时，MCP 初始化和工具枚举仍能完成，连接报告不可用；桥接不自行启动应用、提升权限、安装驱动或重新申请被撤销的授权。当前只有一个原生工作线程和一个在途请求，忙时拒绝叠加作业；没有写操作自动重试。

## 基本映射的预览与提交

使用 `qkm_preview_changes` → 审阅完整 description → `qkm_apply_changes`。先查询 mapping_sources 的完整修订和 source_tab_ref；目前只支持当前已附着源表、每个键为一个大写字母或数字（例如 A→B），insert_row 为零起始源行，省略则追加。须 Read＋Configure、IDLE 且停止清理完成、无打开的映射编辑窗；不会自动停止、切换目标或关闭编辑窗。完整冲突联动会列出；只有用户同意禁用所列旧行时才传 confirm_conflicts=true。

预览保留120秒，新预览替换同配对的旧预览；取消使用 qkm_cancel_changes。实际提交重新检查完整内容、实例／权限／目标和运行修订，在现有源互斥内复用GUI业务提交；验证超过5秒也在写入前拒绝。该预算不是已开始提交后的强制中断，不承诺回复必在5秒到达。成功只改内存／dirty，保持停止，明确 saved=false。无自动重试；重复已应用的同一预览只返回已知历史结果，不再添加一行。

超时或断线后先按同一ID调用 qkm_get_operation。结果属于实例／配对／权限代次，完成记录是带采样时间的历史事实；最多64条，进行中记录保留，完成后保留10分钟并受容量限制，接口重置或进程退出后丢失。全部结果槽为进行中时operation_limit拒绝消费新预览；先查询原操作和预览，不自动重试。unknown不证明写入未发生。运行状态另含run.sampled_at_utc，变化不代表映射内容修订变化。完整保存／冷启动和实际运行等价性分别验收，不把基础添加称为完整闭环。

`qkm_disconnect`只结束本端连接及等待，保留已存配对；已经接受的保存仍可能完成，不代表撤销、回滚或停止。超时／断线后显式连接原目标，再query原操作ID，核对分段结果后再决定后续动作。SAVE-100已选验同一存活实例及同一SDK进程重连，不据此保证进程重启或接口重置后仍有历史。

## 完整保存的预览与提交

先核对目标声明`configuration.save.preview`，再调用`qkm_preview_changes`，例如`{"action":"configuration.save","setting_name":"Demo"}`；setting_name是明确配置组名，其他映射字段省略。普通／新建／覆盖目标及当前已选全局组均由产品校验，不切换GUI配置或改名称控件决定目标。须Read＋Configure、成功加载及初始静止或仍有效的完整清理许可、无打开编辑窗或未结算热键输入；Worker STOPPED本身不够。schema以oneOf保留原映射四个必填字段与保存分支，SDK拒绝未知字段，C++另独立校验组合及业务规则。

description返回所属实例／组／save_revision、实际源快照和完整scope：全部目标／共享写入键、其他已加载及共享草稿、新组排序、启动选择，以及成功后GUI名称效果。它是完整写入足迹，`change_attribution=unavailable`，不是精确差异或“只保存刚加的一行”；始终报告全范围确认要求。原单调截止最多120秒，创建存储不会续期；保存与映射预览共用单配对替换／取消及结果归属。

当目标实际声明`configuration.save.apply`且`apply_supported=true`，用户同意完整范围后提交`qkm_apply_changes({"preview_id":"实际预览ID","confirm_scope":true})`；覆盖已有其他组还须独立`confirm_overwrite=true`。默认均false，不代用户批准；`confirm_conflicts`只用于映射，不能代替保存确认。提交重新采集真实内容、权限、原截止及完整清理许可，在写入前最终核对并持有源互斥完成同步／重载，不自动停止或启动。5秒是写入前总验证预算，不中断部分保存或承诺执行必在5秒结束。

操作结果分别报告`saved`、`reloaded`、`backup_attempted`、`backup_succeeded`及`outcome`，target保留原实例／组／save_revision，新的加载代次与源epoch另列。主文件／重载成功但备份失败仍为applied，`outcome=saved_backup_failed`，须向用户说明。写失败返回save_write_failed，query中saved=false、modified=null、in_memory_effects_possible=true，不能视为内存完全未变；save_reload_failed说明磁盘已经保存但重载失败，未做全局回滚。失败后先查询同一ID再决定恢复，重复成功ID只返回历史结果，不再次保存；完整并发／断线和生命周期验收仍按主计划推进。

## 安全与失败语义

先检查私有目录／文件 owner、单用户 DACL、目录保护／继承、reparse 和硬链接，再按严格 UTF-8 JSON schema 读取有界发现信息。核对原生进程创建时间、主 Token 用户／登录身份／桌面会话、实际 OS 程序路径和显式预期 EXE 的 Windows 文件身份；在打开提示中的路径前先与 OS 路径比较。登录默认要求 AuthenticationId 相同；UAC full／limited 仅接受系统 TokenLinkedToken 核实的互指 AuthenticationId，用户／logon SID／SessionId 仍须全部相同，查询失败不降级。打开管道后再次验证实际 Server PID／会话和活跃进程，随后使用与 C++ 一致的固定 HMAC-SHA256 transcript 双向证明。发现文件本身不能授权。

每次工作检查线程未冒用其他 Token。管道采用最小访问掩码和 Identification SQOS。分帧、认证与请求使用绝对截止时间；取消在发起 I/O 的同一工作线程调用 CancelIo，并在释放 OVERLAPPED／缓冲区前排空完成结果。没有常驻查询轮询。

作业准入由当前原生 Future 绑定，完成回调统一释放忙状态，包括执行前取消；回调在状态锁外注册。协程迟到取消仅影响仍属自己的当前作业，不污染下一请求；执行中仍须等原生排空结束后才能接收新 I/O。提交失败恢复准入，断开／关闭沿同一个工作线程串行，不自动重试。

MCP 调用自身失败使用原生 CallToolResult 的 isError=true，并在 structuredContent 和对应 JSON 文本中保留 ok=false／error 错误码，增加受控 message 和按码生成的 next_steps。产品错误码限制为 1–64 个小写字母／数字／下划线（字母开头），消息须为 UTF-8 不超过 1024 字节的字符串；只保留已审阅业务错误的端点解释，原生异常及未知错误用固定通用说明。连接申请 accepted=false 也作为调用失败；accepted=true 仅表示已接收，不承诺随后连接成功。

成功查询到 status=failed 的连接状态或 outcome=failed 的 operation 数据仍是成功查询，不因数据含 error 字段误置 isError。SDK 参数 schema 校验错误保留 SDK 标准 isError 通道，产品返回的 invalid_arguments 则带上述业务错误结构；未知字段、类型及范围检查不放宽。下一步是解释信息，不会自动重试、停映射、重新配对或授予权限。

客户端密钥保存在当前用户的 Windows 凭据管理器，`CRED_PERSIST_LOCAL_MACHINE`；另以禁止 UI 的当前用户 DPAPI 绑定机器、用途、显式发现／EXE 路径及 Windows SessionId。pywin32 的 CredWrite 接收 Unicode，写入的是 **DPAPI 密文的十六进制文本**，不是明文密钥。密钥不进入 MCP 回复或日志，但不承诺清除 Python／Qt 所有临时内存副本。[Windows 凭据持久化定义](https://learn.microsoft.com/en-us/windows/win32/api/wincred/ns-wincred-credentialw)、[CredWrite 用户凭据语义](https://learn.microsoft.com/en-us/windows/win32/api/wincred/nf-wincred-credwritew)。

首次配对先保存客户端密钥再发送确认；C++ 保存服务器授权后才发送 ready。任一端保存失败都不算已连接。若客户端已保存而服务器失败，可能留下无效客户端记录，下次连接仍须服务器认证；不会因本地存在密钥而假定授权有效。再次申请配对需要新的 GUI 批准，不覆盖损坏的服务器存储。测试只删除自身随机目标，不枚举或修改用户其他凭据。

同一配对目录由一个当前版本的活跃服务器独占。关闭状态的撤销先取得同一租约，重新读取后原子保存；另一实例占用时显示原因并拒绝修改，避免活跃实例保留已撤销权限。查看不创建监听或存储目录。只读／损坏存储保存失败时仍保留原记录，不能视为撤销成功；进程退出后 Windows 自动释放租约。

管理员维护服务与普通桥接的同用户 UAC 路径，以及 Win10 两套真正产品的入口／默认绑定、只读批准、RPC、重连、撤销／关闭和冷启动拒绝旧 key 已实测，见[正式产品验收](../QKeyMapper/tests/mcp_control/README.md)。实际其他用户连接拒绝、Win11、托盘、业务等价性与性能仍须分别验收。使用另一个管理员账号提权不属于同用户 UAC 关联路径，不会因此放宽异用户边界。

## 回归验证

从仓库根目录执行协议／原生工作线程单测：

```powershell
$env:PYTHONPATH = (Resolve-Path mcp_bridge/src).Path
out/mcp_bridge/venv/Scripts/python.exe -m unittest discover -s mcp_bridge/tests -p test_protocol.py -v
```

构建 [MCP 管理维护目标](../QKeyMapper/tests/mcp_control/README.md) 后运行正式 SDK 互操作测试。按机器填写 Qt bin；每次选新的 output，必须在本仓库 `out/` 下：

```powershell
out/mcp_bridge/venv/Scripts/python.exe mcp_bridge/tests/test_sdk_interop.py --test-exe out/tests/mcp_control/qt6-release/release/QKeyMapperMcpControlTest.exe --qt-bin <Qt6-bin> --output out/mcp_bridge/interop-qt6 --approval-timeout
```

Qt5 使用其维护 EXE、Qt bin 和独立 output。`--approval-timeout` 实际等待一次 60 秒；省略则只运行较短场景。脚本拒绝 Python `-O`，默认检查父进程、GUI 和待批准桥接进程非提权。创建私有 Windows 桌面，始终不调用 SwitchDesktop、不占用 Default 焦点、不发送系统键鼠、不操作驱动。

已获用户授权管理员测试且能确认 UAC 时，加 `--elevated-server`。它仅用 gsudo 启动固定的短生命周期助手和自有维护 EXE；控制器、正式 SDK／stdio 桥接及全部断言仍为普通权限。记录双方实际完整性、原始 AuthenticationId 是否相同、系统互指关联和目标实际桌面，不记录 SID／登录 ID。此模式不修改 manifest、不改变 gsudo 配置，启动等待超时不自动重启。维护进程由固定退出事件关闭并核对 exit 0，保留原有 180 秒看门狗。

测试通过官方Client／stdio执行initialize、list和工具调用。12个短场景覆盖12工具严格schema、新增预览／提交／取消／结果查询的参数及未连接错误，还有既有配对证明、重连、拒绝、撤销、取消／退出、发现信息与保存失败；加--approval-timeout共13场景。维护EXE的固定GUI事件／失败夹具不进入产品，transport_only不冒充业务可用；实际映射等价性另由正式产品验证。原始日志及自有清理结果在out，具体版本与覆盖见验证记录。

正式产品的--load-fixture warm现在还运行基本映射等价性套件，需明确授权私有桌面Read+Configure批准与固定GUI添加A→B，不能沿用仅只读助手范围。test_source_interop.py末段同样需要Read+Configure及共通C→B／禁用普通C的固定变更授权；不启动映射、写INI或发送系统输入。完整保存／冷启动和MCP启停尚未接入；Default真实输入使用独立input_regression入口及其授权。

此切片是本地开发检查点，完整进度见 [主计划](../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md)。SDK API 依据 [官方 Python SDK](https://github.com/modelcontextprotocol/python-sdk) 及本环境固定版本的已安装源码核对。

## 受限真实窗口图片（P1-108）

先查询 `mapping_sources`／`mappings` 取得当前 `source_revision`、所选 `source_tab_ref`，再查询 `windows` 取得非空 `window_ref`。能力包含 `windows.capture` 且配对有 **Read+CaptureWindows** 时，调用 `qkm_capture_window(role, window_ref, source_revision, source_tab_ref)`；role仅main／item_setup，四项均必填，无PID／HWND／输出路径。main须显示所选源Tab，单项窗须对应实际源Tab／行；窗口必须已显示且非最小化。截图工具不打开、恢复或激活窗口；单项窗可由下述私有后台正式入口或原GUI打开。

成功的 `CallToolResult.content` 含一项原生 `ImageContent(type="image", mime_type="image/png", data=<Base64>)` 及不重复图片Base64的JSON文本；`structured_content.result`为相同元数据。包括准确实例／进程创建身份、实际桌面、角色／窗口引用、源及源Tab／单项行、像素／DPR／PNG字节数、UTC时间和桥接计算的SHA256。Agent可直接查看原生图像；gateway应按其SDK取出image内容或解码成自己的受控附件，再关联QQ会话发送。MCP返回图片不证明QQ发送或送达，本机没有gateway联调环境。

仅实时Qt客户区，最多160KiB PNG／8M像素，整体受通信帧限制；超限拒绝，不缩放／截断。原生边框、独立弹窗／原生子窗及滚动隐藏内容不包含。源修订证明底层内存，`presentation=live_qt_client_ui_drafts_may_be_visible`提醒其他控件草稿可能可见；不证明所有UI已提交或已保存。单项三个键文本未应用变更时明确拒绝，先通过获准业务流程处理草稿。完整专用演示重置仍待P2。

失败返回is_error及文本／结构化错误，无image内容：常见permission_denied、stale_window、stale_source、context_busy、window_unavailable、capture_timeout、image_too_large、invalid_image。关闭导致原生句柄消失时window_ref为null，不可提交；重新观察。显示后焦点转回Agent本身不使引用失效，但生命周期／源／权限变化必须重新查询。接口五秒截止并在短绘制等待前后复核；损坏图片被桥接拒绝且连接关闭，不返回旧图或自动重试。

正式工具只传内存内容，不创建图片文件或历史缓存；若gateway落盘，须由普通权限接线端限制目录、文件名、大小、数量、保留时间及清理归属。开发SDK在out保存的PNG仅是证据，不是已交付文件产物服务。测试入口 `tests/test_images.py`（7项）和 `tests/window_checks.py`；Win11双Qt实际图片及拒绝范围见同一主计划／C1证据。Win10、高DPI、Default因果、代表高级页、正式预热、免Python交付及QQ渠道仍待。

## 私有后台打开源行设定窗（P1-109）

能力包含 `mapping.editor.open` 且配对另有 **Read+ShowWindows** 时，先查询当前 `mappings` 的稳定 `source_ref`，调用 `qkm_open_mapping_editor(source_ref, presentation="private_background")`。只有这两个参数，角色固定本实例单项设定窗；无PID／HWND／路径，不接受foreground模式。C++实际核验WinSta0后台私有桌面，拒绝Default、当前输入桌面或无法核验的桌面，不能凭客户端自称演示实例放行。

源须属于当前可见Tab／行且仍安全；不会切Tab、停止映射或关闭既有编辑窗。已有编辑窗／草稿、表格编辑或模态／弹窗时明确拒绝；保持原普通GUI行为。复用原单项展示入口，只在本次显示临时禁止激活并恢复原属性，不调用置顶、SwitchDesktop或系统输入，不改映射／保存。

成功结果含target、role=item_setup、presentation、activation_requested=false、源引用／源Tab／完整源上下文及新window对象。用返回的window_ref、source_revision、source_tab_ref即可在另获CaptureWindows后调用 `qkm_capture_window`。失败／超时或断线后先query windows确认是否已打开，勿盲目重复；重复打开会被拒绝，防止覆盖草稿。只处理后台私有实例，不代替普通多实例的Default前台展示／用户确认和绑定。

当前可在已准备的开发实例完成正式MCP建A→B→正式打开单项窗→原生图片，但正式安全关闭、串行场景重置及Release预热／免Python包仍待；不能直接把维护程序或gsudo缓存部署为咨询服务。Win11双Qt正式路径、无Show拒绝及独立GUI对照见唯一主计划；已有Read／Configure／Capture不能自动扩成Show权限，沿管理窗口明确批准后重新连接。
