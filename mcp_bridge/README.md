# QKeyMapper MCP 桥接（开发阶段）

官方 Python MCP SDK 的 stdio 适配层。已有连接／GUI 配对、能力查询、只读状态、配置／Tab 目录、安全源表／源行分页及解析器帮助、基本映射的预览／内存提交／取消／结果查询；SAVE-97接通完整保存的预览与显式提交，P1-107接本实例窗口观察、P1-108接有界真实PNG捕获、P1-109／110接私有后台源行设定窗安全开关，DELETE-113接稳定源行删除预览／提交，实际范围见[唯一主计划](../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md)。**其他设置变更、启停及输入测试尚未接入，M1仍未完成**。P1-121后当前开发产品返回十九项业务能力、十七工具；未接入业务处理器的维护服务仍返回 `transport_only` 和空列表，须查询目标实际能力，不能只看工具枚举。

桥接面向 Win10／11。统一开发基线为 CPython 3.11.9 Windows x64、`mcp==2.3.0`、`pywin32==312`，C++ 对端分别使用 Qt 6.8.3／Qt 5.15.2。两台机器的逐轮验收分别记录，不能互相替代；完整运行依赖固定在 [requirements-windows.txt](requirements-windows.txt)。PKG-123提供下述免全局Python的桥接组件目录包；完整Release集成及干净双系统交付仍属M3-B，原venv入口继续用于开发和回归。

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
10. 能力包含 `windows.read` 时，`qkm_query({"entity":"windows"})` 默认观察当前已认证实例的main／item_setup两个固定角色；120另允许window_role="mapping_advanced"，只返回固定高级窗一项。仅接受entity／offset／limit及此可选角色，默认offset为0–2、显式高级窗0–1，limit为1–100。返回实际 EXE／PID／创建时间／GUI 线程桌面、Qt／原生可见、最小化、前台精确采样、逻辑尺寸／DPR及不透明 window_ref。未创建原生窗口时引用为 null，不为查询调用 winId 创建或显示窗口。该查询只需 Read，不授予或调用显示／截图，也不读取其他进程窗口。

窗口引用仅属于实例／配对权限代次及本角色窗口生命周期，关闭／隐藏重开、呈现／原生窗口重建、销毁或身份／权限改变后失效；正常返回 Agent 导致的焦点变化不使其失效。`foreground_exact` 和 `foreground_in_instance` 是本次采样，不代表已请求激活或已核实前台显示。引用不证明映射源／场景未变，也不授权操作；截图独立核对业务源及源Tab；109提供私有后台单项打开，121补固定高级窗；普通Default显示确认仍待。

管理窗口的“显示窗口”与“截取窗口图像”是各自独立的权限，默认不授予，也不包含在 Read／Configure／Run mapping 中。新配对可单独勾选；已有配对在接口启用后，选中实际客户端，点击“授权窗口权限”追加批准，已有权限和配对密钥保留。批准实际新增权限后权限代次递增、该客户端的旧连接断开；使用 qkm_connect 明确重连取得新权限，无需重新配对。取消或没有新增项不写盘；保存失败关闭 MCP，原授权未成功改变。旧引用／预览属于原权限代次，应重新查询。

DEMO-P1-105交付授权基础，107／108新增窗口观察和捕获，109接私有后台单项打开；Default显示确认仍待。不能因获得权限就假设接口存在，须查询实际能力。写有新权限的配对文件会被不认识这些范围位的旧产品拒绝，升级／部署应使用匹配版本，不自动删存储或降级权限恢复连接。

目录只提供配置名称和当前 Tab 元数据；display_name 来自下拉框条目文本，空白项可为空字符串，通过 kind=draft 区分，不能仅按显示名定位。saved_profile 的内容未加载查询，未附着共通表也有 attached_to_ui=false 的目录记录，这不表示屏幕可见性。selected_configuration／is_selected 只表示界面选择。catalog_ref 和目录修订号不是映射编辑目标或完整业务修订，不能据此提交变更；权限仍由认证与逐请求检查决定。

源查询要求 GUI idle、Worker stopped、最近加载成功，且读取基准为初始静止（read_basis=pristine_idle，cleanup_generation=null）或仍有效的完整清理许可（read_basis=completed_cleanup，cleanup_generation 为本实例清理代次字符串）；否则 context_busy，不主动停止映射。生命周期请求、按钮排队、任务构造和同步／包装在相关处理前记录活动修订，与在途复制互斥。只有完整停止协调、所有者／发送池及 GUI 尾部完成后，才在互斥内记录清理对应修订；任何后续活动或饱和值均拒绝该许可，不能仅凭 IDLE／STOPPED 或零计数恢复。相同映射跨启停后也必须重新查询引用。此许可只用于复制源数据，不能授权删除／修改源表或替代正式启停操作结果。source_snapshot 的配置选择和最近加载记录不能证明每行磁盘来源；未附着的共通备份可能保留自之前的配置，origin=source_memory 只证明当前内存来源。源修订仍不是完整业务修订。

连接状态是最近一次核验结果；产品状态须显式重新查询，Worker 字段是跨线程的单次采样。当前十七个工具的 schema 均声明 `additionalProperties=false`，参数严格检查类型与范围，未知字段在进入工具函数前拒绝；C++ 仍独立检查业务请求参数。应用未启动／接口未启用时，MCP 初始化和工具枚举仍能完成，连接报告不可用；桥接不自行启动应用、提升权限、安装驱动或重新申请被撤销的授权。当前只有一个原生工作线程和一个在途请求，忙时拒绝叠加作业；没有写操作自动重试。

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

先查询 `mapping_sources`／`mappings` 取得当前 `source_revision`、所选 `source_tab_ref`，再查询 `windows` 取得非空 `window_ref`。能力包含 `windows.capture` 且配对有 **Read+CaptureWindows** 时，调用 `qkm_capture_window(role, window_ref, source_revision, source_tab_ref)`；role固定main／item_setup／mapping_advanced，四项均必填，无PID／HWND／输出路径。main须显示所选源Tab，单项窗须对应实际源Tab／行；窗口必须已显示且非最小化。截图工具不打开、恢复或激活窗口；单项窗可由下述私有后台正式入口或原GUI打开。

成功的 `CallToolResult.content` 含一项原生 `ImageContent(type="image", mime_type="image/png", data=<Base64>)` 及不重复图片Base64的JSON文本；`structured_content.result`为相同元数据。包括准确实例／进程创建身份、实际桌面、角色／窗口引用、源及源Tab／单项行、像素／DPR／PNG字节数、UTC时间和桥接计算的SHA256。Agent可直接查看原生图像；gateway应按其SDK取出image内容或解码成自己的受控附件，再关联QQ会话发送。MCP返回图片不证明QQ发送或送达，本机没有gateway联调环境。

仅实时Qt客户区，最多160KiB PNG／8M像素，整体受通信帧限制；超限拒绝，不缩放／截断。原生边框、独立弹窗／原生子窗及滚动隐藏内容不包含。源修订证明底层内存，`presentation=live_qt_client_ui_drafts_may_be_visible`提醒其他控件草稿可能可见；不证明所有UI已提交或已保存。单项三个键文本未应用变更时明确拒绝，先通过获准业务流程处理草稿。完整专用演示重置仍待P2。

失败返回is_error及文本／结构化错误，无image内容：常见permission_denied、stale_window、stale_source、context_busy、window_unavailable、capture_timeout、image_too_large、invalid_image。关闭导致原生句柄消失时window_ref为null，不可提交；重新观察。显示后焦点转回Agent本身不使引用失效，但生命周期／源／权限变化必须重新查询。接口五秒截止并在短绘制等待前后复核；损坏图片被桥接拒绝且连接关闭，不返回旧图或自动重试。

正式工具只传内存内容，不创建图片文件或历史缓存；若gateway落盘，须由普通权限接线端限制目录、文件名、大小、数量、保留时间及清理归属。开发SDK在out保存的PNG仅是证据，不是已交付文件产物服务。测试入口 `tests/test_images.py` 和 `tests/window_checks.py`；Win11双Qt实际图片及拒绝范围见同一主计划／C1证据，代表高级页见120–122。Win10、高DPI、Default因果、正式预热、免Python交付及QQ渠道仍待。

## 私有后台打开源行设定窗（P1-109）

能力包含 `mapping.editor.open` 且配对另有 **Read+ShowWindows** 时，先查询当前 `mappings` 的稳定 `source_ref`，调用 `qkm_open_mapping_editor(source_ref, presentation="private_background")`。只有这两个参数，角色固定本实例单项设定窗；无PID／HWND／路径，不接受foreground模式。C++实际核验WinSta0后台私有桌面，拒绝Default、当前输入桌面或无法核验的桌面，不能凭客户端自称演示实例放行。

源须属于当前可见Tab／行且仍安全；不会切Tab、停止映射或关闭既有编辑窗。已有编辑窗／草稿、表格编辑或模态／弹窗时明确拒绝；保持原普通GUI行为。复用原单项展示入口，只在本次显示临时禁止激活并恢复原属性，不调用置顶、SwitchDesktop或系统输入，不改映射／保存。

成功结果含target、role=item_setup、presentation、activation_requested=false、源引用／源Tab／完整源上下文及新window对象。用返回的window_ref、source_revision、source_tab_ref即可在另获CaptureWindows后调用 `qkm_capture_window`。失败／超时或断线后先query windows确认是否已打开，勿盲目重复；重复打开会被拒绝，防止覆盖草稿。只处理后台私有实例，不代替普通多实例的Default前台展示／用户确认和绑定。

当前可在已准备的开发实例完成正式MCP建A→B→正式打开单项窗→原生图片，110补正式安全关闭，117接下述专用单行串行回收；Release预热／免Python包仍待，不能直接把维护程序或gsudo缓存部署为咨询服务。Win11双Qt正式路径、无Show拒绝及独立GUI对照见唯一主计划；已有Read／Configure／Capture不能自动扩成Show权限，沿管理窗口明确批准后重新连接。

## 不丢草稿的受限关闭（P1-110）

能力 `mapping.editor.close` 要求 **Read+ShowWindows**，调用 `qkm_close_mapping_editor(source_ref, window_ref, presentation="private_background")`。source_ref来自本owner正式open，window_ref必须是同实例当前角色有效引用；只有这三个参数，不接discard、PID／HWND／路径。仅实际后台私有非输入桌面，继续复用当前源／清理门槛；不提供普通Default关闭、映射清空或保存。

正式open在产品内存中保留当前输入摘要／源记录，关闭只接受源和输入未变、且本owner正式打开的窗口。仅GUI打开的窗口为editor_not_owned；键文本、数值未提交文本、Combo／勾选、待应用备注／segments或记录状态有变化时为editor_draft_changed，存在可见子窗也拒绝。先保留草稿并在原GUI处理；本工具不自动应用、舍弃或刷新控件绕过。语言／控件重建可能保守拒绝；新增草稿控件类型须扩展内部守卫。摘要不含任何对外可读控件地址或草稿原文。

成功含closed=true、目标／源及已核实不可见的window；源和INI保持其既有状态，不代表演示场景已重置。Show／Hide／原生句柄／销毁使旧打开记录失效，实例／pair权限变化由原引用机制拒绝；正常焦点返回Agent不使记录失效。Resize后旧window_ref失效，重新query当前引用且源／摘要仍匹配才可继续；窗口新生命周期不能沿用旧引用。失败／断线后先query实际窗口，不猜测未执行或自动重发。

Win11双Qt已验证GUI窗口拒绝、C草稿保护／恢复B后关闭、连续两轮正式开关及旧引用拒绝；当前无Show负例复用子窗保护前同准入路径结果，原版本与材料分开记录。源行删除见下节；演示初始状态、真实高级子窗交错、Win10／高DPI、预热／免Python与QQ仍待，见唯一主计划。

## 稳定源行删除（DELETE-113）

能力包含 `mapping.delete.preview`／`mapping.delete.apply` 且有 **Read+Configure** 时，先查询 `mapping_sources` 和 `mappings`，调用 `qkm_preview_changes(action="mapping.delete", source_revision=..., source_tab_ref=..., source_refs=[...])`。只接受同一当前附着源、同一完整修订的 1–100 个规范源引用；重复引用去重，不接受显示行号、跨源批次、任意 PID／路径或增添／保存字段。目标行不得被表格筛选隐藏，不能是分隔行；不要求窗口占据前台，也不自动切表或显示窗口。

新增／删除 apply 能力声明 `source_binding_result="commit_snapshot_or_null"` 时，成功结果额外含 `source_binding_status` 和 `source_binding_after`。`captured` 绑定真实提交时、原源互斥释放前的完整 owner 源快照；其 `source_snapshot`／`source_tab_ref` 可用于后续查询，新增还含新行 `source_ref`，删除的 `source_ref` 为 null。`reference_scope="commit_snapshot_only"` 表示历史提交事实，后续 GUI 或其他操作仍可立即使引用失效。历史重放／operation 返回原绑定，不重新采样。

后续操作应使用该提交绑定并接受 `stale_source`，不能重查当前源后认领其中的相似行。绑定捕获失败时为 `unavailable`／null，原操作仍如实报告 `applied`／`modified=true`；不能据此推断未修改或盲目重试。旧版没有此能力合同时，不应提供依赖提交归属证明的自动回收。

预览逐条返回完整删除字段及派生采样、实例和源摘要，不改映射、dirty或INI。用户审阅并同意删除后，才调用 `qkm_apply_changes(preview_id=..., confirm_delete=true)`；冲突、保存范围或覆盖确认不能替代删除确认。提交再次核验实际认证owner／权限、完整源、当前目标、原120秒到期、无编辑窗／表格编辑／弹出菜单及完整空闲清理许可，再复用GUI共用删除和显示刷新、鼠标点更新、接替选区、dirty流程。

成功只改内存，返回 `modified=true, saved=false, started_by_operation=false`、删除数及 `query_source_again=true`；没有保存、自动停止／启动或演示重置。所有旧源引用／预览须按修订重新查询。取消／拒绝不删除，已消费失败需新预览；成功ID重复提交只返回历史结果，不再次删除。结果绑定原实例／pair权限代次和原源摘要，超时／断线先查同ID的 `qkm_get_operation`，unknown不证明未执行，禁止盲目重试或改连其他实例。

当前仅普通当前源及直接选中的Common源；从普通Tab操作附加Common、隐藏行删除、跨Tab及多源批次尚不支持。Win11双Qt正式stdio的普通最后行、Common首行／全批及重复引用、无确认／错误确认、Read拒绝、编辑窗／表格草稿／菜单保护、GUI删除后的陈旧预览及历史重放已定向核对；117仅在专用空源下回收本轮归属行，完整删除矩阵、实际Win10／高DPI及运行后清理仍待。维护SDK用例见 [mapping_deletion_acceptance.py](tests/mapping_deletion_acceptance.py)，具体证据仍以唯一主计划／C1记录为准。

## 专用演示实例只读预检（P2-114）

宿主可在原`--discovery-file`／可信`--executable`外显式指定`--demo-record <私有创建记录>`，该桥接才额外提供无参数、只读的`qkm_get_demo_state`；P1-121后普通入口17工具，此参数增加为18工具；115另增下述非行足迹查询，产品当前19项业务能力。此参数不创建进程、申请UAC或配对，不改变正常默认关闭；MCP客户端不能传入记录路径、任意PID或自己选择目标。当前是开发桥接入口，完整Release预热／免Python交付仍待P3／M3-B。

P3-133另提供显式`--product-demo-record`，与旧记录路径互斥。仅用已认证管道的实例ID推导该固定discovery目录内`demo-<instance>.json`，未认证不读取；ACL／文件及实际身份／只配置策略照旧逐次验证，不信任发现自报路径、不枚举或回退其他实例。它同样增加就绪工具，另加`--enable-demo-mapping`才有串行演示。重启后可用相同客户端配置启动新桥接，但须显式连接、核验新实例；旧回收记录／引用／预览／结果不迁移。无新权限、自动配对或产品启动。

复用限同一可信EXE路径和Windows会话；目录／会话改变需重新复制管理窗连接路径并核验，不跨会话寻找目标或复制凭据。

记录是受信启动链的目标约束，最多8KiB，严格包含`version=2`、`startup_purpose=configuration_only_demo`、`executable`绝对路径、十进制字符串`pid/created/session`、32位小写hex的`instance`、`desktop`及初始保存配置组`configuration`；没有密钥。目录和文件须为当前用户的私有ACL、非重解析对象／单链接普通文件。记录自报路径不产生信任：仍沿显式可信EXE、实际进程／创建时间／登录会话、认证管道核验，再与产品报告的实际桌面和实例逐项匹配。旧version1开发记录明确拒绝，不自动迁移；126起只配置产品可在启用MCP时发布自身version2记录，正式预热宿主与完整部署流程尚未交付；历史维护宿主手写记录只保留其测试范围。记录自报用途仍须与已认证state／capabilities的严格只配置策略一致，普通模式、策略缺失／值不符或有效Run均拒绝，不能只凭私有桌面放行。

预检在既有单worker内串行读取，要求已有Read+Configure+ShowWindows+CaptureWindows、后台私有非输入桌面、固定配置加载已接受、完整空闲清理读取基准、唯一附着普通空源及空的未附着Common备份、主窗已恢复／可见、无单项窗口且初始无dirty。产品`windows`观察新增保守布尔`private_background_available`，缺失／无法确认均不放行；不会显示、激活或切换桌面。

成功数据含`configured/ready/reason`、已核实目标及`source_snapshot`；`reference_scope=preflight_observation_only`明确它不是写入许可、场景租约或完整重置证明。权限不足、非空／dirty、配置或记录不符、窗口未就绪分别报告；错误记录不改连其他目标、不返回未经核实的目标摘要，不破坏现有配对。当前工具只观察，不建例／截图／关窗／删除；不能仅凭ready=true向QQ宣称服务已准备完毕。非行足迹见115，显式串行例程见117，QQ渠道由gateway另联调。

## 非行保存足迹观察（P2-115）

产品能力`demo.context.read`要求Read+Configure，调用`qkm_query(entity="demo_context")`；只支持实际后台私有桌面的当前单一普通源、无Common数据及完整空闲清理读取基准。只接受entity/offset/limit，offset必须0，不接受目录／源引用或路径。不创建保存预览，不改变现有预览槽，不保存、重载、自动关闭窗口或启动映射。

它复用C1完整保存载荷和临时Qt序列化核验，在副本中仅排除74项明确的源行存储键；保留其他Tab元数据、加载与共享草稿、宏、原始INI字节及GUI保存名称，再绑定实例、pair权限代次、加载／runtime代次。返回`context_revision`、`gui_save_name`、完整`source_snapshot`和`write_authority=false`。内部临时解析不等于写入产品INI；只读不表示没有临时序列化文件。

`reference_scope=non_row_save_footprint_only`必须与完整源修订及窗口生命周期分别使用；不覆盖行数据、不授权修改，不是场景租约或完整业务修订。单项或其他设定编辑窗、未稳定快捷键、文件／共享缓存无法核验时拒绝。dirty已true时，GUI保存名或磁盘字节仍能改变该签名；仅新增／删除已知源行时可保持不变，为117受限回收提供依据。跨退出恢复或QQ交付仍待，不能仅凭两个签名相等宣称所有外部系统行为已复原。

## 专用单行串行演示（P2-117）

部署端已准备独立运行副本／合成初始INI、受信创建记录和实际私有桌面实例时，可在现有桥接参数中同时指定 `--demo-record <私有记录> --enable-demo-mapping`。增加非只读、非幂等工具 `qkm_demo_mapping(original_key="A", mapping_key="B")`，只接收单个大写字母／数字；P1-121后普通入口17工具，仅demo-record为18工具，两个参数同时启用为19工具，产品19项业务能力。显式开启不代替原GUI配对与Read+Configure+ShowWindows+CaptureWindows批准，不增加Run权限。

一次调用在原单worker内建立一个内存映射，按真实提交绑定核对完整行／其余空源及非行足迹，复用原后台单项窗入口和有界PNG，安全关闭后只删除本轮创建的行。返回单张原生MCP ImageContent；结构化结果保留实例／窗口／源／尺寸／SHA256及`demo_scene`场景ID、代次、业务键、原操作ID和`cleanup`证明。捕获源修订是历史图片上下文，回收源修订是后续空源，两者不得混用。桥接不写图片文件；Agent／gateway可以直接消费ImageContent的PNG，测试宿主在普通权限out保存供看图。QQ发送由gateway另行完成，`delivery`明确仅MCP图像、渠道送达未验证。

首次必须符合114干净空源基线。回收后dirty按原GUI行为保留，绝不强制清零或保存；只有同一桥接内上一轮完整回收记录的目标、全源修订和非行签名都仍匹配，才允许在已知dirty下继续。只读demo_state因此仍可报告`demo_baseline_dirty`，另含`demo_session`最近阶段／操作ID、`receipt_available`及`quarantined`；这是存储记录观察，实际下一轮仍须完整核验。连接状态查询也提供该摘要，便于在传输失败后查看原操作ID而不自动重连。

已知业务失败时仅在原源／窗口／非行归属仍可证明时尝试一次受限回收；草稿、源／设定变化或未知执行结果保留现场并隔离。超时／取消不猜测未执行、不自动重试或改连；新桥接没有旧内存记录，遇到遗留dirty／场景拒绝，交由部署端处理。正常配对持久化策略保持；本工具不提供清空任意草稿、撤销外部操作或启动映射的接口。

Win11双Qt各两轮A→B／C→D正式stdio完成，四张真实820×480单项窗图片经Agent查看，源／INI／加载及无运行输入已定向核对。模拟测试覆盖已知失败回收和未知／外来变化隔离，不能替代实际进程退出／撤权／超时等矩阵。118补下述主窗视图，122补代表高级页；其他实例对照、Win10／高DPI、正式预热和免Python包仍待，现有开发命令不作为无开发环境Release部署承诺。详见唯一主计划及 [demo_scene_acceptance.py](tests/demo_scene_acceptance.py)。

### 主窗口视图（P2-118）

`qkm_demo_mapping(original_key="A", mapping_key="B", view="main")`使用相同固定部署／权限和归属行回收；view允许main、item_setup及122新增的mapping_advanced，省略仍走原单项窗。写入前核对实际windows.capture角色声明。main仅查询并捕获已存在、可见／恢复的本实例主窗，不打开单项窗、不切换设定页、不请求显示或激活；未就绪按原失败／回收规则处理，不能自动改选其他窗口。`demo_scene.view`与原图像role对应。

Win11双Qt各实际主窗A→B、随后默认单项C→D及Read负例通过，主窗1130×722 PNG可见真实A/B行，四张图片经Agent查看；复用117完全相同的本机Native二进制，未重编产品或测试夹具。主窗横向滚动表格当前仅可见部分列，不宣称全列内容或高级设定页已覆盖；完整源字段仍由正式业务查询核对。新增视图不改变正常工具或配对／实例选择规则。


## 映射高级窗只读查询／捕获（P1-120）

`qkm_query(entity="windows", window_role="mapping_advanced")`仅观察固定本实例QMappingAdvancedDialog，需Read；默认不带window_role仍返回原main／item_setup两项。其他查询实体不接受该选择器，不扫描或选择任意进程／窗口；查询不创建原生窗口、显示或激活。窗口隐藏／关闭后旧引用失效。

高级窗已经由原GUI显示且源安全可读时，用所查window_ref及当前source_revision／source_tab_ref调用`qkm_capture_window(role="mapping_advanced", ...)`，仍需独立Read+CaptureWindows。高级窗须对应当前源Tab，绘制前后复核与160KiB／8M像素上限保持；返回真实ImageContent。高级控件是实时加载设定，`settings_scope="live_loaded_settings_not_committed_by_source_revision"`明确其值未由源行修订提交，不能据此宣称完整配置已保存或未变化。未显示、旧窗／旧源、模态／popup或权限不足明确失败，不自动打开或切换目标。

Win11双Qt正式图片与同次原GUI客户区PNG字节相同，Agent已查看；默认两角色兼容、无Capture（Read+Configure+Show）拒绝、开关前后源及非行足迹／INI保持已定向核对。无Capture用例中的固定GUI帧是测试夹具观察材料，MCP未返回图片。高级窗正式显示／归属关闭见下述121；串行高级view见122；不提供高级设置编辑。其他系统／Default／高DPI、独立子窗与完整交付缺口见唯一主计划。


## 私有后台高级窗安全开关（P1-121）

先查询实际能力mapping.settings.open／close及当前mapping_sources，使用当前附着源的source_tab_ref调用`qkm_open_mapping_settings(source_tab_ref=..., presentation="private_background")`。仅Read+ShowWindows，在实际非输入私有桌面复用原GUI入口显示固定高级窗，不切主窗设置页或请求激活；不提供任意PID／HWND／role／路径或设置编辑。已有编辑窗／子窗／popup、非空闲或不稳定数字拒绝。普通Default前台显示与人工确认仍属于另一个待交付流程。

成功返回目标／源／窗口摘要；用返回window_ref和同一source_tab_ref调用`qkm_close_mapping_settings(..., presentation="private_background")`。只关闭本owner正式打开、完整源及全部输入摘要未变化的窗；GUI打开的窗为editor_not_owned，数值或未提交数字变化为editor_draft_changed，保留现场，不提供discard。旧源／旧窗及权限／身份变化拒绝；失败／断线先查询实际状态，不能自动重发、清草稿或换目标。

截图仍另需CaptureWindows。已归属高级窗的图片元数据可能包含`settings_editor_unchanged`布尔，表示捕获前后输入摘要是否仍与本次打开记录相同；false的实时图片不能作为该演示已确认的初始场景，缺失也不能推定true。它不代替源／非行足迹核验或安全关闭，普通GUI实时图片仍可能展示草稿。正式串行高级view见下述122。无高级参数编辑、预热、免Python或QQ送达承诺。


### 固定高级串行视图（P2-122）

`qkm_demo_mapping(original_key="A", mapping_key="B", view="mapping_advanced")`在相同专用实例和四项权限下，先正式建例，沿固定高级窗原入口后台显示／捕获，再安全关闭和回收本轮行。写前要求实际mapping.settings.open／close和capture角色能力；捕获须有settings_editor_unchanged=true，随后完整非行足迹与源再次一致才交付图片。未知结果或草稿变化保留隔离，不替用户丢弃；标记缺失／false不返回场景图片，仍按原证明决定是否能安全回收。

高级图片显示当前真实参数，本窗口本身不含A/B，行内容由正式源字段核验并关联demo_scene；不提供高级参数编辑或保存。Win11双Qt连续高级A→B后默认单项C→D、Read负例及四张图片已定向核对；普通工具及配对规则保持。Agent／gateway可直接消费原生ImageContent，QQ发送仍须渠道联调。Release预热／免Python包、Win10／高DPI及实际连接异常矩阵仍待。

## 随包嵌入运行时（PKG-123）

[目录包用法](BUNDLE_README.md)随产物复制为README。Windows x64组件包含专用CPython3.11.9、现有29项哈希锁运行依赖和桥接；用户不需安装Python、venv、pip或源码。客户端直接配置包内runtime/python.exe的绝对路径及`-I -B -X utf8 -m qkeymapper_mcp`，._pth只导入包内标准库／依赖。原私有发现、显式可信EXE、身份核验、配对及权限保持；桥接组件不启动产品或申请UAC，不能代替P3预热。

开发者先取得[PSF 3.11.9发布归档](https://www.python.org/ftp/python/3.11.9/python-3.11.9-embed-amd64.zip)及原requirements哈希选定wheel，使用独立的3.11.9 x64无pip环境离线组装：

```powershell
& '<已核验的Python3.11.9 x64绝对路径>' -m venv --without-pip out/mcp_bundle_builder
& out/mcp_bundle_builder/Scripts/python.exe -I -B -X utf8 mcp_bridge/tools/build_windows_bundle.py `
  --runtime-zip '<python-3.11.9-embed-amd64.zip绝对路径>' `
  --wheel-directory out/mcp_bridge/wheels-cp311-win_amd64 `
  --output out/mcp_bundle/QKeyMapperMcpBridge
```

组装只读本地固定输入、不下载依赖、不调用运行端pip；固定归档SHA256／大小、逐wheel版本与SHA256、关键Python PE有效PSF签名在复制应用前验证，Windows签名核验沿系统信任策略。路径穿越、设备／ADS／歧义名称、归档symlink、文件碰撞、实际Windows重解析路径及超预算拒绝。目标目录及同名ZIP均须全新，失败材料保留；不删除／覆盖旧包。签名检查明确使用Windows内置安全模块，不依赖PowerShell7或用户模块路径。保留Python／依赖／项目许可，产物清单列相对路径、大小及SHA256；目录包和ZIP均含同一载荷，普通产品Release仍沿既有发布脚本。

归档基准11249023B、SHA256 `009d6bf7e3b2ddca3d784fa09f90fe54336d5b60f0e0f305c37f400bf83cfd3b`，来源为官方HTTPS、发布页MD5、.sigstore消息摘要及关键PE Authenticode；未执行Sigstore证书链／透明日志完整验证，不把摘要对照称为签名验证。标准库打包器随3.11.9固定，无新增第三方打包器依赖或bootloader。

维护检查为`tests/test_windows_bundle.py`；官方SDK端可运行`tests/bundle_acceptance.py --bundle <解压目录> --output <全新out目录>`，需要原开发测试环境，仅测试端使用SDK。PKG-123在Win11验证ZIP独立解压、中文／空格路径、与包无关的cwd和被污染的Python环境／仅系统PATH、29项metadata与原生依赖、17工具及异步连接不存在产品的失败。再用包内实际Medium客户端与121原manifest Qt6私有目标完成高级／默认单项两轮，两张正式图片及源／INI／回收保持；运行前后载荷不变。本机未断网，没有把路径隔离当作无全局Python的干净Win10／Win11或断网启动验收；普通Release集成／P3／QQ仍待。


### 当前契约组件刷新（PKG-130）

按同一固定输入重新组装当前version2演示契约的桥接源码，目录包使用方式不变。Win11普通权限完成全新ZIP独立解压／1614项载荷和当前Python源码一致性检查，污染环境／中文空格路径／无关cwd下包内运行时及29项依赖保持隔离。普通／仅记录／串行演示三种配置分别枚举17／18／19工具；未连接的演示查询／执行明确not_connected并保持失败状态，没有启动产品、配对或申请UAC。首扩展测试误期望disconnected，按既有failed/not_connected行为修正测试后通过，桥接行为未改。

随后用同一独立解压包内runtime连接当前129只配置Qt6产品：实际核验Medium客户端、High父子身份及不同私有桌面，消费产品自身version2记录，完成高级设定／单条映射两场景、两张真实中文图片和回收。Agent已查看，场景／操作关联、完整INI不变、产品记录删除、实际父子0退出及本轮清理零；1614项载荷保持不变。固定维护拥有者与配对夹具仅用于验收，不等于正式部署GUI；本轮未新验Qt5包内产品、干净系统／断网、完整Release／预热或QQ。原PKG-123结果仍保持旧版本范围。

### 只配置演示启动策略（P3-124）

新版产品新增固定`--demo-configuration-only`，在Application／所有单例前确定且不能经MCP切换；仅实际WinSta0非Default且非当前输入桌面可启动。它硬拒绝映射／系统输入、Hook／Interception／libusb／SDL控制器和虚拟设备初始化、驱动操作、外部命令／更新、Windows开机任务及FilterKeys写入。真实配置、解析器、窗口和业务共用；普通模式、正式管理员manifest及MCP默认关闭保持。

已认证模式的state／capabilities附`execution_policy`，明确startup_purpose及四项禁止效果。有效权限上限为Read+Configure+ShowWindows+CaptureWindows（387）；新Run授权拒绝，旧持久配对不自动删权限或重复配对，但当前连接只取允许交集。只配置模式仍需原GUI启用／配对与独立窗口权限，不能将该参数或Skill当授权。

当前已完成启动门禁、产品自身version2记录和固定`--demo-host`的独立状态／正常关闭入口，并完成选定双Qt私有验证；配对显示／返回已接线并通过私有原生提示／三语定向验证，真实Default／人工配对、部署引导及完整Release仍待验收。宿主只创建同EXE的专用私有子实例，不自动启用MCP或批准配对。不要自行将普通用户多实例Default确认流程用于后台实例，也不要用开发gsudo缓存作为正式服务依赖。一次原生UAC、临时桌面切换／人工配对的验收另行授权；本节不是完整免开发环境部署步骤。


P3-125 起上述演示入口仅接受version2和已认证只配置产品；缺少产品策略时返回`demo_configuration_only_required`，在预览、业务写入和窗口显示前失败，普通非演示MCP不受该限制。`false`须为布尔值，不接受0／空值或额外策略字段。旧记录／用途不符为`demo_record_mismatch`；只读预检拒绝后保留当前认证连接，恢复正确记录可重新预检，不自动配对、改连或迁移旧引用。正常串行回收及未知结果隔离规则不变。124之前未加模式的开发证据保留原范围，不视为新版演示准入通过。


### 产品自身演示记录（P3-126）

只配置模式在初始映射加载成功后固定启动配置声明；显式启用MCP时，沿已有每目标私有目录及独占租约发布`demo-<instance>.json`。普通模式不发布，MCP仍默认关闭。原发现文件保持七项连接元数据，已认证且有Read权限的`qkm_get_capabilities`另返回`demo_record`路径；该路径不代替显式可信EXE、实际进程／创建时间、登录会话、认证或权限核验。记录含九项version2字段、不含配对密钥，最多8KiB。

优先把产品返回的记录路径用于`--demo-record`；桥接只读消费，不代写或修复。产品启用期间持有只读句柄，拒绝外部写删；关闭时仅核对本次实际身份及原始发布字节一致的记录并按句柄删除。异常退出或清理失败可能留下旧记录，仍须按实例／PID创建时间及认证判无效，不自动改连或删除不明文件。初始配置声明不随用户后来选组自动改绑，不一致时预检失败。

首次GUI批准时私有桌面暂为输入桌面的发布路径仅有源码设计，本片没有切桌面验收；正常演示仍要求后台私有。Win11双Qt组件及Qt6原manifest正式SDK消费产品记录／两图／回收／正常退出删除已定向验证，Qt5本片正式SDK、完整预热宿主、Default／Win10／QQ尚待。现有123历史ZIP不含125新契约；正式交付须重建匹配版本桥接与产品，不能把旧组件包当作当前整包。

### 固定宿主配对入口（P3-131／132，选定Win11流程已验证）

只在已确认支持该模式的完整独立Release副本、固定安全演示INI下显式运行`QKeyMapper.exe --demo-host`。原管理员manifest保持，开发gsudo不是部署依赖；不复制个人INI或凭据。132普通Qt6已走原生产品UAC／真实GUI配对及正常返回；完整发布组装、干净系统运行库前提和故障矩阵仍待，不能只对旧EXE添加参数便假定安全。

首次预热由操作者与Agent配合完成；宿主不会自动启动桥接、启用MCP或批准客户端：

1. Agent先准备包内普通桥接，固定可信产品EXE和管理窗提供的`instance.json`；以`--product-demo-record --enable-demo-mapping`启用固定产品记录与串行工具，或保留旧显式记录路径。不能从发现文件自报EXE推导信任。
2. 操作者批准产品原生UAC，点击宿主“设置配对”。黑色背景且看不到Agent／办公应用是私有桌面隔离的正常表现；只需操作本实例管理窗与返回提示，可将提示框拖到旁边。
3. 在“MCP 控制”勾选“启用 MCP 接口”。这只开启接口。Agent应准备就绪并在进入后调用`qkm_request_pairing`，避免在阅读步骤时耗尽约60秒人工批准窗口；不会自动批准或重发。
4. 明确申请标志是新的批准区域出现客户端名、PID、已核验EXE及权限按钮。核对随包`bridge/runtime/python.exe`的实际路径；读取必选，额外勾选配置、显示窗口、捕获窗口，运行映射保持不选，再点击批准。若启用后约10秒还未见申请，使用返回提示“确定”回Agent排查，保留宿主；不要一直盲等。
5. 批准后用私有桌面提示的“确定”返回。Agent核验认证状态及387权限后才执行业务；OK本身不证明已配对。后续后台咨询不再显示／激活Default或逐次批准。
6. 保持宿主运行，使用完成后点击“关闭演示”。它先取消提示并等返回，再正常关闭已核实子实例，实际退出前不把STOPPED视为回收完成。

已存配对可在同一可信路径／登录上下文重连；MCP仍需用户显式启用，新实例必须重新核验，旧引用／预览不能复用。只批准Read时不会自动扩大为配置权限；须操作者明确处理新的配对，窗口追加权限仍沿原独立批准入口。

132实际两图在cf56普通产品完成，随后修正返回线程的多余桌面迁移并单独复验返回／重连，未重复图片。真实返回失败／15分钟实等／锁屏／宿主意外死亡与完整Release／Win10／QQ仍待；异常宿主死亡或恢复界面不可用仍可能需要人工Windows桌面恢复。
