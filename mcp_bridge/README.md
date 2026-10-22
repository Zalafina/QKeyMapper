# QKeyMapper MCP 桥接（开发阶段）

官方 Python MCP SDK 的 stdio 适配层。当前支持连接状态、申请 GUI 配对、使用既有凭据连接、断开、能力查询、只读状态摘要和配置／Tab 目录分页。**尚未提供映射行查询、映射编辑、设置变更、启停或输入测试操作**。当前产品返回 `stage=business`、`state.read` 与 `catalog.read`；未接入业务处理器的维护服务仍返回 `transport_only` 和空业务能力列表，以实际响应为准。

桥接面向 Win10／11。当前维护验收为 Win10 x64、Python 3.11、`mcp==2.3.0`、`pywin32==312`，C++ 对端分别使用 Qt 6.8.3／Qt 5.15.2。完整运行依赖固定在 [requirements-windows.txt](requirements-windows.txt)。普通用户免安装 Python 的打包交付属于 M3，当前入口用于开发和回归测试。

## 启动与使用

开发／跨机接续统一使用本轮实测的 **Windows x64 CPython 3.11.9**（包含补丁版本），pip 24.0、setuptools 65.5.0；29 项运行依赖及其传递依赖已在 [requirements-windows.txt](requirements-windows.txt) 精确固定，开发工具由 [requirements-dev-windows.txt](requirements-dev-windows.txt) 补齐。使用明确的解释器路径，在每台 PC 仓库内各自创建环境：

```powershell
& '<本机已核验的 CPython 3.11.9 x64 python.exe 绝对路径>' -m venv out/mcp_bridge/venv
out/mcp_bridge/venv/Scripts/python.exe -m pip install --only-binary=:all: --no-deps -r mcp_bridge/requirements-dev-windows.txt
out/mcp_bridge/venv/Scripts/python.exe -X utf8 mcp_bridge/check_environment.py
out/mcp_bridge/venv/Scripts/python.exe -m pip check
```

每条命令都须 exit 0。已有环境先校验，通过则复用；失败时在本机重建或修复后重验，不能忽略差异后继续验收。`check_environment.py` 只读检查解释器／平台／隔离状态和全部安装包，拒绝缺失、版本变化、重复或额外依赖；对应反向用例为 `tests/test_environment.py`。使用 venv 内解释器的绝对路径启动 SDK、回归及桥接，不依赖 PATH 或激活脚本。venv、个人凭据和 out 产物不随 Git 复制到另一台 PC；虚拟环境应在目标路径重新创建，见 [Python venv 说明](https://docs.python.org/3/library/venv.html)。

上述版本是当前已验证基线，升级须在同一检查点更新锁和校验入口并重跑相关验收。**包文件哈希锁、Python 安装文件身份及打包工具版本尚未补齐**：首次 PC-W11 接续前须落实安装文件校验及 Windows x64 wheel 哈希锁／安装校验；M3-B 打包前固定打包器与相关构建工具，并在独立环境离线重建／启动验收。采用 pip 的[精确版本及哈希锁机制](https://pip.pypa.io/en/stable/topics/repeatable-installs/)，不引入新的环境管理框架。版本校验通过不能替代两系统业务、设备及实际输入验收。

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
5. 能力包含 `state.read` 时，`qkm_get_state` 读取本次 GUI 线程快照：界面选择对应的配置组／类型、dirty、当前 Tab、GUI 监视状态、Worker 原子状态采样和实际停止快捷键。另含 `mapping_load_context`：最近映射加载／空白重置的代次（十进制字符串）、结果和实际解析的配置组；加载未完成时 context_busy，非法映射导致加载失败时 outcome=invalid_mapping_data、group=null，不沿用旧来源。它记录最后一次加载，不能证明后续编辑未变、全部数据来自同一磁盘组或已保存；configuration 仍只表示界面选择。`schema_version=1`；不可用的 `business_revision=null`，`stop_completion=unobserved`。Tab index 只作观察值，不能用作稳定编辑目标；GUI idle 或 Worker stopped 也不能证明停止清理完成。
6. 能力包含 `catalog.read` 时，`qkm_query` 的 entity 只接受 configurations／tabs。首轮 offset=0，limit 为 1–100（默认 50），后续页携带 catalog.revision 为 catalog_revision，并使用实际返回的 next_offset；next_offset=null 表示结束。相关目录、映射加载代次／结果或授权上下文变化返回 stale_catalog，应从首轮重新查询。相同目录／上下文的重复读取及同配对重连保持修订号稳定；不保留目录缓存或过期定时器。
7. `qkm_disconnect` 有界取消等待并断开，保留已存配对。它不启动／停止映射，也不撤销服务器权限。撤销使用 QKeyMapper 的管理窗口；接口关闭时也能查看及撤销持久化配对，无需临时启用接口。

目录只提供配置名称和当前 Tab 元数据；display_name 来自下拉框条目文本，空白项可为空字符串，通过 kind=draft 区分，不能仅按显示名定位。saved_profile 的内容未加载查询，未附着共通表也有 attached_to_ui=false 的目录记录，这不表示屏幕可见性。selected_configuration／is_selected 只表示界面选择。catalog_ref 和目录修订号不是映射编辑目标或完整业务修订，不能据此提交变更；权限仍由认证与逐请求检查决定。

连接状态是最近一次核验结果；产品状态须显式重新查询，Worker 字段是跨线程的单次采样。当前七个工具的 schema 均声明 `additionalProperties=false`，目录参数严格检查类型与范围，未知字段在进入工具函数前拒绝；C++ 仍独立检查业务请求参数。应用未启动／接口未启用时，MCP 初始化和工具枚举仍能完成，连接报告不可用；桥接不自行启动应用、提升权限、安装驱动或重新申请被撤销的授权。当前只有一个原生工作线程和一个在途请求，忙时拒绝叠加作业；没有写操作自动重试。

## 安全与失败语义

先检查私有目录／文件 owner、单用户 DACL、目录保护／继承、reparse 和硬链接，再按严格 UTF-8 JSON schema 读取有界发现信息。核对原生进程创建时间、主 Token 用户／登录身份／桌面会话、实际 OS 程序路径和显式预期 EXE 的 Windows 文件身份；在打开提示中的路径前先与 OS 路径比较。登录默认要求 AuthenticationId 相同；UAC full／limited 仅接受系统 TokenLinkedToken 核实的互指 AuthenticationId，用户／logon SID／SessionId 仍须全部相同，查询失败不降级。打开管道后再次验证实际 Server PID／会话和活跃进程，随后使用与 C++ 一致的固定 HMAC-SHA256 transcript 双向证明。发现文件本身不能授权。

每次工作检查线程未冒用其他 Token。管道采用最小访问掩码和 Identification SQOS。分帧、认证与请求使用绝对截止时间；取消在发起 I/O 的同一工作线程调用 CancelIo，并在释放 OVERLAPPED／缓冲区前排空完成结果。没有常驻查询轮询。

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

测试通过官方 `Client`／stdio 执行 initialize、list、工具调用；维护 EXE 中的 `--interop-fixture` 只提供固定 Qt 界面事件及失败夹具，产品 EXE 不含此入口。12 个短场景覆盖七工具未知字段／目录参数拒绝、GUI 批准前不 ready、跨语言证明、密钥重连、错误 DPAPI 上下文、无业务处理器时状态／目录请求拒绝、启用／关闭状态撤销及旧 key 拒绝、GUI 拒绝、取消、stdio 退出、错误预期 EXE、私有但被篡改／过期的提示和服务器保存失败；加 `--approval-timeout` 时共 13 场景。退出清理自有窗口和测试凭据。原始日志、状态和结果在 out，随 Git 同步的是测试源码；各轮实际执行及复用范围见验证记录。

此切片是本地开发检查点，完整进度见 [主计划](../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md)。SDK API 依据 [官方 Python SDK](https://github.com/modelcontextprotocol/python-sdk) 及本环境固定版本的已安装源码核对。
