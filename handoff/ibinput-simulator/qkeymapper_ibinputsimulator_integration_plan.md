# QKeyMapper IbInputSimulator 集成计划：独立实施、能力验收与 MCP 合流

文档修订：R5，2026-10-30（Asia/Shanghai）。
本文是本功能唯一的设计、决策、实施状态和双 PC 接续来源，随功能分支提交；目录规则见 [handoff/README.md](../README.md)，依赖事实见 [依赖来源与本机制品](dependency_provenance.md)。旧 plan/ 文档仅保留导航，不维护第二份进度。

## 0. 最新交接卡片与本轮停止点

| 项目 | 当前记录 |
|---|---|
| 当前阶段 | IB-01～04 主体代码已落地，形成待验收的本地开发检查点；阶段未全部完成；IB-05 已只读对齐接口，尚未试合并 |
| 当前授权 | 2026-10-29 恢复主要开发及聚焦本地提交；0001／0002 均明确批准。G HUB 最小连接／短移／F24／左键／滚轮已授权，另准一次复测；不 push、不合并 master，不启动／关闭厂商软件、提权、重启或故障设备测试 |
| 本轮执行机器 | PC-W10，Windows 10 Pro 22H2 19045.6093 x64；G HUB 2026.7.992920，安装／进程观察与实际结果分别记录 |
| 实施仓库 | F:/work/code/mygit_hub/temp_test/QKeyMapper_temp_debug/QKeyMapper；另一台 PC 以实际检出路径为准 |
| 分支／起点 | codex/ibinput-simulator；DOC-00=53f80dbdb2b0893289118fb76d6e566bc9beb393，产品基线=d49a9f57d280d3b3cecc8204372ae7dc13bf6e0d；开始时干净；本轮聚焦提交见末尾检查点记录 |
| 唯一计划 | 本文件；双层 handoff 路径不存在，沿用仓库单层入口，未创建第二份进度 |
| 工具链 | Qt6.8.3 x64／Qt5.15.2 x64、x86，MSVC 19.44.35222.0，SDK 10.0.19041.0，CMake 3.28.6；Qt6 产品 Release 及定向客户端／ABI 检查见第 11 节 |
| 依赖 | 四个固定提交、完整许可、正式公共头文件、获批两补丁、配套 x64／x86 DLL／导入库均随 Git；重建命令及真实制品身份见 dependency_provenance.md |
| MCP 只读参考 | F:/work/code/mygit_hub/QKeyMapper，codex/qkeymapper-mcp-control，20c0ea1f02b8accbc2ca31838c436260ee340c43；IB-03 前及收尾再次核对；未修改其源码／计划，无关 .claude/settings.local.json 保留 |
| 当前范围／实耗 | 依赖、客户端、配置／Popup、四类移动和键／钮／轮、代次及排空释放主要代码；原估算 P1 2–4h、C1/C2 3–5h、C3/C4 4–8h。本轮跨中断累计工时未连续计时，不虚报实耗；构建和探针以原始时间戳为准，收尾切片 23:42 起，预估 30–60min |
| 权限／桌面 | 编辑／Git／构建／截图均普通权限。三个自有探针在 Default，短时焦点；未接管用户实例，均正常关闭。只有第二个探针发送四次短移；其他动作未发送 |
| 当前结果／性质 | 当前是本地开发检查点，不能代表功能／阶段验收完成。G HUB 首次短移收到并正常关闭；随后重新初始化及独立一次复测均返回 DeviceCreateFailed(5)，完整抽查失败 |
| 能力门控 | 正式产品暂开放 G HUB 相对移动；四条产品映射路径仍待实测。键／左键／垂直轮实现保留在固定验证目标，待真实接收及释放证据后开放；其他后端、按钮组合／拖拽／定位／复杂序列保守拒绝且保留配置 |
| 翻译 | 英文源及运行时刷新已接线；用户 10/30 已批准非三语相同的 62 项；正式三语 TS/QM 已更新并逐条回读核对；纯品牌／固定模板不翻译。审阅稿 translation_review.md 不维护进度；实际产品三语视觉／刷新待验 |
| 下一步／完成标准 | Win11 核实 G HUB 实际版本与设备；先检查连接／关闭／复连失败，再用单代次固定探针验证 F24、左键、±1格滚轮及释放，之后逐条验四类产品映射路径；达到第 11 节条件才开放相应能力 |
| 同步 | 仅本地提交；由用户传输或另行授权 push。out、日志、截图、缓存、个人 INI／凭据不随 Git；恢复方式见第 11 节 |

此前 DOC-00 的停止／仅文档授权已被用户本次恢复指令替代；历史事实保留在 Git。本文后续沿用的阶段与能力开放门槛不变。

状态统一使用：未开始／进行中／待验证／受阻／完成。执行暂停或恢复意图另写在交接卡，不把“已接通”或“已编译”当作阶段完成。

## 1. 目标与已确认决策

新增发送方式 IbInput，通过 IbInputSimulator 的 Logitech／Razer 后端提供另一种输入发送选择。最终目标覆盖与现有 FakerInput 对应的键盘、鼠标按钮、滚轮及移动用途；根据实际能力分阶段开放，未验证或不支持的组合明确说明原因。

| 决策 | 已确认内容 |
|---|---|
| 工作隔离 | 在新仓库独立实施；原 MCP 仓库保持其自身进度，后续通过专门集成分支融合 |
| 分支 | 从新仓库 master 的 d49a9f57 创建 codex/ibinput-simulator |
| 交付节奏 | 完整通道目标，分阶段交付；按已验证能力开放，不为达到“完整”字样掩盖限制 |
| 用户界面 | 主界面单行状态＋类似“设定备份”的 PopupFrame；单项编辑窗提供同一面板快捷入口 |
| 偏好范围 | 应用在本机使用的全局偏好；单条映射只保存 IbInput 发送方式，不保存厂商后端 |
| 自动／手动 | 默认自动选择，允许手动指定 G HUB 新通道、Logitech 旧通道或 Razer |
| 失败策略 | 默认无法连接时拒绝启动；可选在本轮开始前改用 SendInput；运行中不跨后端切换 |
| 链接方式 | 保留导入库隐式链接方案；运行时依赖随产品部署的 DLL，不改为手写动态加载函数表 |
| 不支持能力 | 可保存配置并显示限制；运行准入明确拒绝，不静默丢输入或擅自改写发送方式 |
| 自动化 | GUI／MCP 最终共用业务规则；当前 master 无需提前引入整套 MCP 代码 |
| 文档位置 | 按用户最新指令在 handoff/ibinput-simulator 维护唯一主计划；plan/ 留导航和本机材料，不修改忽略规则 |

DOC-00 曾授权文档分支推送；本次最新授权为聚焦本地提交，不 push，不合并 master。后续驱动初始化、真实输入、系统设置副作用和需要 UAC 的场景，必须在对应阶段具体说明并按项目约定获得授权；沿用既有授权时不重复征求同一事项。

不以特定游戏、反作弊或硬件白名单作为功能可靠性保证。目标应用是否接受输入需要独立记录实际环境与结果，不能由驱动签名、初始化成功或 DLL 编译成功推导。

## 2. Review 结论：原提案必须修正的内容

| ID | 核实事实／风险 | 计划处理 |
|---|---|---|
| R01 | 上游 AnyDriver 会尝试 Logitech、Razer、DD、MouClassInputInjection，未纳入 LogitechGHubNew | QKeyMapper 显式控制候选；不调用 AnyDriver，不启用 DD／MCII |
| R02 | IbSendInit 使用强类型 SendType／Error；IbSendMouseMove 有 x、y、MoveMode 三个参数 | 使用固定版本正式头文件；删除不能编译的两参数示例和 int 直接互转示例 |
| R03 | IbSendInput 签名近似 Win32 不等于行为等价；当前 Base::send_input 分组后设置 i=j，随后 for 再递增，会跳过下一组首事件 | 适配层逐事件调用并检查返回值；不依赖上游混合类型批量行为 |
| R04 | Logitech 每次发送新建鼠标报告，移动／滚轮可能不保留已按按钮；垂直滚轮仅看符号，水平滚轮未实现 | 先确定最小第三方修正及可表达能力；保持按钮状态、按单位处理滚轮；不能承诺现有 DLL 仅靠 QKeyMapper 包装即可完整修复 |
| R05 | 新 G HUB 初始化会发送设备激活请求，且当前实现未传播该请求的失败 | 被动检测与实际连接分离；连接成功不宣称实际输出已验证，不把初始化用于后台只读刷新 |
| R06 | 上游发送对象为进程级共享对象，发送入口不保护未初始化访问；初始化／销毁不是与发送天然安全并发 | 所有库调用收敛到一个客户端，明确准入、串行保护、运行代次和销毁顺序 |
| R07 | 原方案把无 ExtraInfo 的移动描述为天然无回环；当前 QKeyMapper 鼠标移动仍参与 Mouse2vJoy／屏蔽／锁光标路径 | 分别验收这些组合；不能声称 USB 驱动事件标志必定如何，也不能伪造可靠来源标记 |
| R08 | 当前 FakerInput 消费端过滤不等于新通道已经有完整防回环机制 | 按真实执行路径复用消费端规则；同键连发、链式触发和相关屏蔽组合未经验证不开放 |
| R09 | 配置保存／加载、单项回显存在多个分支；还有映射码、复制、共通映射、导入导出等路径 | 逐入口闭环，不只修改一处白名单或下拉项 |
| R10 | 多类鼠标移动以共享发送方式合成；简单追加判断会产生顺序依赖 | 同一合成组含 IbInput 与其他发送方式时明确拒绝并指出源行，不改变旧配置原语义 |
| R11 | 原示例把 x86 DLL 部署到现有 Qt5 x64 目录，且头文件路径不一致 | 按实际目标架构部署，使用 IbInputSimulator/InputSimulator.hpp |
| R12 | “零风险”“100% 稳定”“彻底修复”和任一家软件安装即有效，均超过现有证据 | 删除保证性措辞；软件、驱动、连接、发送、目标接收分层记录 |
| R13 | Logitech 大位移拆包在填入按钮位之前发送中间报告，并忽略这些报告的返回值 | 拆包、按钮保持和部分失败作为一组评审；IbSendInput 返回 1 不能证明内部所有报告成功 |

上游源码证据均固定在 0aba4f4f2b2053f106cee4c3be0a6201d76ca585：

- [正式接口 InputSimulator.hpp](https://github.com/Chaoses-Ib/IbInputSimulator/blob/0aba4f4f2b2053f106cee4c3be0a6201d76ca585/Simulator/include/IbInputSimulator/InputSimulator.hpp)。
- [初始化、自动候选、共享发送对象及 DLL 入口](https://github.com/Chaoses-Ib/IbInputSimulator/blob/0aba4f4f2b2053f106cee4c3be0a6201d76ca585/Simulator/source/dllmain.cpp)。
- [批量分组、坐标转换及基类语义](https://github.com/Chaoses-Ib/IbInputSimulator/blob/0aba4f4f2b2053f106cee4c3be0a6201d76ca585/Simulator/include/IbInputSimulator/SendTypes/base.hpp)。
- [Logitech 报告、键盘和设备激活](https://github.com/Chaoses-Ib/IbInputSimulator/blob/0aba4f4f2b2053f106cee4c3be0a6201d76ca585/Simulator/include/IbInputSimulator/SendTypes/Logitech.hpp)。
- [Razer 键鼠发送实现](https://github.com/Chaoses-Ib/IbInputSimulator/blob/0aba4f4f2b2053f106cee4c3be0a6201d76ca585/Simulator/include/IbInputSimulator/SendTypes/Razer.hpp)。
- [新 G HUB 设备激活修复 PR #52](https://github.com/Chaoses-Ib/IbInputSimulator/pull/52)。合入事实不等于本机重启复连已经通过。

这里的源码结论来自本次审阅，不是本产品运行测试。后端内部缺陷若需要修改第三方源码，先形成最小、可复现补丁和对应验证，再单独评审，不能把既有二进制直接当作修正版。

## 3. 源码与依赖接入边界

### 3.1 独立基线与影响范围

实施依据新仓库的 master 基线，不照搬 MCP 工作区中的行号或新增接口。关键接入面：

| 子系统 | 当前入口／应覆盖范围 |
|---|---|
| 发送选择与编辑 | qkeymapper_constants、MAP_KEYDATA、QItemSetupDialog 的各回显入口及发送方式变更 |
| 配置和复制 | qkeymapper 的配置／全局设置保存加载、共通映射、映射码、导入导出、备份恢复与复制粘贴 |
| 运行时发送 | sendInputKeys 的键盘／按钮／滚轮，鼠标坐标点击、定位及位置恢复 |
| 连续移动 | Key2Mouse、Joy2Mouse、Gyro2Mouse、GamepadTouchpad2Mouse 的选择和实际发送 |
| 停止与退出 | 启停、切表／重启、序列取消、释放任务、线程池排空、程序析构 |
| 展示 | 映射设定页新增一行；PopupFrame；单项发送方式旁快捷入口；字体／缩放／主题／语言 |
| 构建部署 | 产品 qmake、实际包含客户端的维护目标、三架构运行目录和正式发布脚本 |
| 后续 MCP | 查询、预览／提交、保存足迹、运行代次、只配置演示及窗口能力 |

开始每个代码切片前核对实际调用方和相邻分支。大文件使用唯一锚点、一次性受控修改、范围 diff／统计和编码检查，避免无关重排；本任务不重构整个输入框架。

### 3.2 依赖制品与链接

本机 plan/ibinput_simulator/BUILD_INFO.md 是前序构建记录，本轮不重写它或重复编译。Git 内的 [依赖来源与本机制品](dependency_provenance.md)保存固定版本、现有文件身份、历史构建参数与未验证边界，另一台 PC 不必依赖本机 plan/ 才能理解来源。

固定源码与依赖：

| 组件 | 固定提交 |
|---|---|
| IbInputSimulator | 0aba4f4f2b2053f106cee4c3be0a6201d76ca585 |
| IbWinCppLib | f630f0e3a2c757652298c626fcd52469e5d5bc81 |
| Microsoft Detours | 1dbd4ea281d73d478aa8d09a8b83fbd265f715f7 |
| RapidJSON | 24b5e7a8b27f42fa16b96fc70aade9106cf7102f |

IB-01 核对完整源获取／检出命令、工具链、SDK、CMake 参数、头文件和许可证，不能用 clone 默认分支代替固定版本。DLL 的 C++20 构建要求不自动要求修改 QKeyMapper 全局语言标准。

未来产品目录使用 QKeyMapper/IbInputSimulator/include/IbInputSimulator/InputSimulator.hpp 和 lib/x64、lib/x86。开发目录保存配套头文件／导入库；用户运行包部署正确架构 DLL 及必要许可，不必随运行包分发开发用 .lib。

隐式链接意味着缺少或装错 DLL 会在进程加载阶段失败，GUI 无法再解释该缺包。因此三架构部署清单、独立运行目录和发布包检查是交付门槛；“未安装厂商软件仍可用其他模式”不涵盖缺少程序依赖 DLL。

支持目标为 Qt 6.8.3 x64、Qt 5.15.2 x64／x86。新通道先按 Win10／Win11 验收；不承诺未测系统兼容。若新增依赖会破坏产品既有最低系统启动要求，需在 IB-01 暴露并重新评审，不能默认改变整个产品支持范围。

## 4. UI 与用户体验

### 4.1 入口及面板结构

映射设定页在现有 FakerInput 区域之后新增一行，左侧为“IbInput 设置…”按钮，右侧为简短文字状态；保留现有 FakerInput 按钮和状态行为。

单项编辑窗发送方式增加简短的 IbInput，旁边提供设置快捷入口。说明和厂商状态放在 Popup 内，避免使用长供应商名称拉宽发送方式下拉框。

采用 QFrame＋Qt::Popup，参考现有 ActionPopup／系统筛选键面板：Box／Raised 边框、8px 基准边距、6px 基准控件间距，沿现有字体、QkmUiScale 和主题规则缩放。不是网页或新增独立设置框架。

面板由主窗口拥有，两个入口使用同一偏好与状态；以实际入口为锚点，按内容 sizeHint 和目标屏幕工作区定位。不得复制“备份”面板固定 120×80 尺寸。点击外部／Escape 关闭；打开 ComboBox 的子弹出列表不能被误判为点击面板外。

| 面板区域 | 内容 |
|---|---|
| 首选后端 | 自动（推荐）／Logitech G HUB／Logitech 旧版通道／Razer Synapse |
| 无法连接时 | 不启动映射（默认）／启动前改用 SendInput |
| 本轮状态 | 当前偏好、实际后端、是否回退、最近结果和采样时间 |
| Logitech 行 | 软件版本、相关后台进程／服务观察、驱动检测或连接结果 |
| Razer 行 | 软件版本、相关后台进程／服务观察、驱动检测或连接结果 |
| 操作区 | 刷新状态、连接检查、官方安装／帮助链接 |
| 解释区 | 未检测到／未知／未连接／连接失败／能力不支持的原因及下一步 |

普通按钮 autoDefault=false，沿用 NoFocus／ClickFocus 约定；不额外引入 Tab 导航或宽泛键盘拦截。布局优先保证中文默认尺寸可读，再验证英文／日文长文本、深浅主题及高 DPI。

### 4.2 状态语义与刷新

安装、进程／服务、设备检测、客户端连接、实际输入是不同事实，独立显示：

- 安装检测读取已核实的软件注册信息和版本；后台观察使用明确的进程／服务身份，失败显示未知。
- 驱动存在可通过只读设备／服务信息观察，不能使用 IbSendInit 冒充被动检测。
- 前端进程未运行不直接判定驱动不可用；存在安装项也不直接显示“可用”。
- 初始化成功只显示“已连接／输出未验证”；最近发送返回成功表示该调用被接受，不代表目标应用已收到。
- 演示模式或尚未连接时显示真实原因，不伪造绿色状态。
- 状态不能只用红绿颜色；说明文字可复制，错误详情保持简短并按需展开。

打开面板刷新一次；面板可见时每五秒进行一次被动刷新；关闭后停止周期刷新。检测不进入键鼠发送热路径，不修改配置 dirty 或源修订。异步结果绑定对象存活和本次刷新代次，关闭、重开或退出后不访问失效窗口。

“连接检查”只在 IDLE 且清理完成时可用，说明它可能激活厂商虚拟设备，但不发送键鼠测试输入。检查在受控客户端中串行执行并结束临时连接，保留最近结果；运行时不销毁或替换正在使用的后端。只读刷新不自动重连。

### 4.3 偏好编辑与语言

修改选择立即更新内存偏好并触发既有未保存提示；关闭面板不隐式保存、不连接后端。正式保存仍走现有保存入口。运行或清理期间允许查看，禁用偏好修改和连接检查，明确提示停止映射后调整。

不自动安装、启动、重启或卸载 G HUB／Synapse。不增加任意 DLL 路径、任意命令、按行选择不同厂商或通用优先级编辑器。

界面采用英文 tr／可翻译 UI 源，中文优先，维护英语和日语及运行时刷新。正式 TS／QM 更新前先展示 context、英文源、中文／日文建议、占位符和换行供用户审阅；批准后由 Agent 按项目 lupdate-pro／lrelease 流程完成，不能让用户承担工具执行。本次 10/30 已获用户明确批准的 62 项文案按正式流程更新；三语相同品牌／固定模板依用户决定不翻译。

## 5. 配置、后端选择与失败策略

### 5.1 配置契约

~~~text
SendMappingKeyMethod: 保留 0=SendInput、1=SendMessage、2=FakerInput；追加 3=IbInput
行序列化沿用 KeyMapData_SendMappingKeyMethod，不另建并行行配置键。

应用全局设置（不是单条映射或每游戏配置的厂商覆盖）：
IbInputPreferredBackend = auto | logitech_ghub | logitech_legacy | razer
IbInputUnavailablePolicy = block | sendinput_before_start

缺省：auto、block。
~~~

稳定配置值由 QKeyMapper 自己解释，不直接持久化上游 SendType 数字。缺省／非法全局值按上述默认处理，并保留可解释诊断。安装位置、版本、当前活动后端、最近错误和检测时间不写入偏好配置。

全局设置沿应用当前的 INI、dirty、保存失败恢复和重置规则；普通配置切换不改变本机偏好。整份个人 INI 若被人为复制，偏好可能随文件移动，不能声称“全局”天然等于硬件绑定；双 PC 开发接续不复制个人 INI。

映射方式 3 必须完整往返：普通与共通源、禁用行、复制／粘贴、映射码、导入导出、恢复和编辑回显。驱动缺失或编译未启用 IBINPUT_SUPPORT 时，仍保留合法值 3 并解释不可用，不能保存成 0。现有旧版本不认识 3，降级使用前保留配置副本。

### 5.2 自动／手动与运行冻结

1. 只有本次有效映射需要 IbInput，才在实际开始前准备后端；普通应用启动、加载／编辑配置和被动查询不连接驱动。
2. 根据本轮有效映射建立所需能力，先拒绝结构冲突和已知不支持组合；不通过发送测试事件试探。
3. 自动模式按 G HUB 新通道 → Logitech 旧通道 → Razer 尝试，跳过已知不满足能力的候选。候选初始化前后保持客户端无旧活动对象；不调用 AnyDriver。
4. 手动模式只尝试选定后端，不隐式换另一厂商。
5. 连接成功后冻结本轮实际后端及运行代次。软件版本信息只作判断辅助，不能替代库连接结果或实测输出证据。
6. 无法连接时，block 阻止本轮开始并显示原因；sendinput_before_start 在任何本轮输入发送前，将本轮所有 IbInput 行的有效方式统一定为 SendInput，界面明确提示，不修改源配置。
7. 无效配置、聚合冲突和不支持功能不借“无法连接回退”绕过。原有特殊命令的明确发送语义见第 6 节。
8. 运行中发送失败只触发一次故障通知和停止清理；禁止改用另一后端重发，避免重复输入和不同设备间 DOWN／UP 分离。
9. 切表或配置重启必须先完成旧代次清理，再对新代次重新准入；不在清理尚未完成时重新初始化。

## 6. 客户端、发送与生命周期

### 6.1 最小实现形态

增加小型 qkm_ibinput_client.h/.cpp，集中处理连接、能力、单事件发送、释放和关闭。保留现有 Worker 的分流点，不为三个旧发送方式建立新的通用插件架构。

建议内部接口职责为：prepareRun、sendEvent、releaseOwnedInputs、closeAfterDrain、statusSnapshot。命名可以按局部代码习惯调整，职责和调用边界不变；实际 SendType／Error 映射集中在客户端内。

- 使用正式头文件和枚举；初始化保留位使用 0、额外参数为空，若固定上游版本有明确要求再按来源校正。
- 不使用 IbSendInputHook，不改变全局 SendInput／GetAsyncKeyState。
- 进程内单活动后端；所有库调用受同一客户端生命周期保护，禁止并发初始化／销毁。
- 对 GUI 发送不可变状态副本，不在持有客户端锁时同步回调 GUI。
- 发送调用检查初始化状态、能力和所属运行代次；库入口收到单个有效事件，必须检查返回计数等于 1。
- 不把厂商查询／重试、格式化日志或常驻状态轮询放进高频输入路径；错误日志按状态变化记录。

当前独立 master 尚无 MCP 新增的 SourceReadAccess、配置演示启动策略及清理观察接口。这里先接当前实际停止／排空路径；MCP 的具体准入和只配置守卫留到 IB-05 合流。不得编写不存在的接口调用或用无效占位成功掩盖缺失能力。

### 6.2 能力表与发送细节

下表均为目标或已知限制，不代表当前产品已实现。

| 能力 | 独立实现要求／开放门槛 |
|---|---|
| 相对移动 | 覆盖正负／零／大位移，遵守上游范围；验证 Key2Mouse、Joy2Mouse、Gyro2Mouse、触摸板全部实际发送点 |
| 键盘 | 根据现有键码映射构造后端所需 VK，不照搬 wVk=0 的扫描码输入；验左右修饰键、扩展键、无效键码、按下重复与释放 |
| 鼠标按钮 | 支持已验证的左右中键／X1／X2；保持本后端按钮状态，移动／滚轮期间不误抬起 |
| 垂直滚轮 | 定义为既有 QKeyMapper 滚轮单位；Logitech 多单位按单位拆分，不丢失幅度 |
| 水平滚轮 | Razer 按能力实测后开放；当前 Logitech 不支持，明确拒绝 |
| 绝对定位／点击／恢复 | 核对坐标单位、负坐标副屏、虚拟桌面原点与屏幕变化；未解决前不开放该组合 |
| 组合键／序列／连发／锁定 | 同步覆盖正常结束、取消、强制停止和退出；未经验证的同键回环组合不开放 |
| 特殊命令 | 保留现有固定 VK 强制 SendInput、文本／剪贴板、后台 SendMessage 等既有明确语义，并在帮助中说明覆盖范围 |

Logitech 鼠标公开接口不等于任意完整报告接口：单个 MOUSEINPUT 的 mouseData 无法同时表达滚轮和 X 按钮信息；当前实现也不能靠连续补发 DOWN 保证多按钮及大位移拆包过程保持正确状态。IB-01 必须确定可行的最小第三方修正或明确能力限制，记录补丁与重建制品身份；不绕过 DLL 新建第二套驱动发送实现。修正未验证前，相应按钮组合／拖拽／大位移能力不开放。

上游 Logitech 键盘报告只有有限普通键槽位，重复 DOWN、同时多键和满报告必须处理并验证；不能因驱动调用返回成功就忽略未进入报告的按键。Unicode、E1 等没有明确支持证据的输入不伪装为已支持。

不通过时间窗 FIFO 推定事件来源，不声称所有驱动事件的 ExtraInfo 或 injected 标志必定为固定值。复用现有 FakerInput 消费端过滤思路时，逐处核对真实来源与发送方式，避免简单新增一个无法可靠赋值的 isIbInput。

### 6.3 合成移动与反馈限制

当前 Key2Mouse 等路径会以共享发送方式输出合成结果。沿各路径的实际共享范围检查所有参与的有效源行（含附加 Common，排除禁用行）：

- 同一合成组存在 IbInput 与其他发送方式混用时，启动前指出相关原始源行并拒绝。
- 不含 IbInput 时保持现有优先规则；不借本功能重写旧发送方式选择机制。
- 同键连发、输出再成为原始触发键、Mouse2vJoy／锁光标／鼠标或键盘屏蔽的相关组合，必须有单独证据才能开放。
- 配置可以保留并显示限制；运行准入使用与 UI 一致的规则。识别不到可证明安全的组合时保守拒绝，并给出具体受限项，不能影响无关普通映射。

### 6.4 停止、故障与退出

在当前控制器排空点完成以下顺序：关闭新 DOWN／移动准入 → 取消不再执行的工作并保留必要 UP → 等待已接纳发送调用结束 → 对本后端持有或发送结果未知的键／按钮尝试释放 → 记录释放结果 → 关闭客户端。

使用现有源和任务生命周期机制，不凭定时等待、Worker STOPPED 或“句柄关闭”推定实际释放完成。必要的公共停止逻辑修正作为聚焦切片审阅，避免与 MCP 的新实现各自发展成两套停止框架。

运行失败后禁止跨后端发送补偿 UP；它不能证明原厂商设备已经释放。释放失败保留不完整状态和错误信息，禁止自动重启；正常退出也不能把未知释放结果记为通过。

同步厂商 I/O 可能阻塞。超时只能报告未完成，不能删除仍被调用访问的对象或同步资源；实际可靠超时／取消能力需要单独验证，首版不承诺强制终止可安全恢复。

## 7. 构建、部署与验证方案

### 7.1 构建与发布

使用新仓库自己的 out/，不复用 MCP 仓库的二进制、部署目录或个人 INI。

| 目标 | 依赖与验证 |
|---|---|
| Qt6.8.3 x64 | x64 导入库／DLL，普通 Release 原生退出 0、零错误／零警告 |
| Qt5.15.2 x64 | x64 导入库／DLL，现有 build_qt5.ps1 是此目标 |
| Qt5.15.2 x86 | x86 导入库／DLL、x86 Qt／MSVC 和独立输出目录；不能把现有 x64 脚本当 x86 验证 |
| 发布／维护目标 | 实际链接客户端的 qmake 目标都补依赖；正式发布脚本显式部署正确 DLL 与许可并核对架构 |

Qt5 仅接受项目既定、来源精确匹配的官方 qtmain.lib(qtmain_win.obj) 缺失 PDB LNK4099 例外，保留原日志。既有发布脚本依赖发布机目录及 master 检查，功能分支阶段不直接运行整套正式发布流程；采用本地独立运行目录验证，最终集成／发布时再接正确流程。

### 7.2 分层验证矩阵

| 层级 | 必要场景 | 证明范围 |
|---|---|---|
| 文档／配置 | 枚举往返、全局默认／非法值、复制／导出／共通源、保存取消／失败、冷进程读回 | 配置不会被静默改写；不证明驱动输出 |
| 客户端定向 | 明确候选顺序、手动模式、未初始化拒绝、单事件返回值、启动前回退冻结、并发关闭／失败释放 | 适配与生命周期；模拟故障不冒充真驱动异常 |
| 真实后端 | 无依赖、仅 Logitech、仅 Razer、两者共存；记录版本和实际候选 | 各环境连接及实际发送结果 |
| 输入效果 | 移动、按钮保持／拖拽、滚轮、组合键、重复 DOWN、报告满、序列取消、切表、快速启停、退出 | 只覆盖实际执行能力，不由日志成功代替接收窗口效果 |
| 交互 | Popup、下拉子窗口、外部点击／Escape、运行中禁用、语言刷新和错误状态 | 真实 Qt 窗口显示／交互 |
| 内存／并发 | 客户端与受影响停止路径的定向静态分析、实际 ASan 生命周期场景 | 内存／所有权风险；不代替真实驱动释放 |
| 集成 | GUI／MCP 同规则、预览过期、保存足迹、停止等待、配置演示无设备初始化 | 合流后软件整体契约 |
| 部署 | 三架构依赖包、清洁运行目录、无厂商软件负例 | 包完整性与启动；不承诺未测 OS／厂商版本 |

开发测试采用最小可复现断言或既有维护目标，模拟与真实执行分别记录。没有设备或软件环境的格子标为未验证，不能安装／卸载厂商软件来擅自补齐。

针对新 G HUB 的系统重启后复连，只能在用户安排或授权重启后实测；PR 合入不能代替此结果。目标游戏／其他应用作为单独选定场景记录，不以“突破限制”作为通用验收标准。

### 7.3 桌面选择、影响与授权

本轮 DOC-00 不需要 Default 桌面、产品启动或输入。

后续私有 Windows 桌面优先覆盖真实窗口字体、布局、状态、语言与程序化缩放。截图使用项目 capture_ui_snapshots.ps1，并由 Agent 自行查看；UI-only 临时 asInvoker 副本不证明原 manifest／驱动行为。

| 必要 Default 场景 | 原因及方法 | 影响与估时 |
|---|---|---|
| 实际厂商驱动输出 | 自有输入接收窗口观察短距离移动、受控键／按钮、拖拽和滚轮；核对接收结果与正常停止 | 前台窗口、焦点和真实键鼠短时受控；每个已具备后端约 10–15 分钟 |
| Popup 的真实焦点／关闭与跨屏行为 | 短时鼠标／键盘操作，验证外部点击、Escape、子下拉及实际跨 DPI 屏幕 | 焦点及鼠标短时受控；约 3–5 分钟 |
| 故障后的真实释放 | 仅在已批准的明确故障场景检查本轮持有输入；注入返回失败与真实设备失效区分 | 并入相应输入场次；不默认停服务、拔设备或卸载驱动 |

执行前按具体阶段说明场景、EXE／INI、停止方式和必要 UAC；获得该范围授权后宣布开始并执行，不重复询问时段。新增输入／前台或驱动操作范围另行说明。PrintWindow 在 Default 上仍可能影响焦点，不能描述为完全静默。

两个仓库不能隔离同机全局 Hook、厂商驱动、实际输入或单实例行为。真实输入／驱动测试与 MCP 相关运行测试错开；核对独立实例身份、绝对 EXE／INI／PID，保持用户实例和原 MCP 实例不受接管。无需为文档工作开启 gsudo 缓存。

## 8. MCP 适配与最终融合

### 8.1 当前参考与重叠风险

参考会话：[恢复 QKeyMapper MCP 开发](codex://threads/01a16695-16dc-7bc0-8589-538e7e1fe85a)。
MCP 仓库内的 handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md 是该任务自己的唯一进度来源。本文件只保存双方接口约定和 IbInput 合流状态，不复制或修改其进度，不发送跨会话指令。

当次只读比较显示，MCP 相比 d49a9f57 已修改配置保存／读取、源快照、异步发送排空、运行代次和只配置演示。输入相关提交还包括原始按键保持、派生发送来源和取消／释放协调。不能在最后通过整文件覆盖消除差异。

**独立功能代码尽量保持与 MCP 无关；最终合流必须补齐产品级约束。** 客户端提供清晰状态、配置入口和释放结果，不预先搬入整套 MCP，也不建立与其竞争的通用停止框架。

### 8.2 融合契约

| 能力 | 约定 |
|---|---|
| 查询／发现 | 在现有能力和状态机制中暴露配置发送方式、实际后端、支持能力、回退原因、最近错误／采样时间 |
| 行配置 | 新方式值 3 纳入源快照和受限编辑；GUI 与 MCP 调用同一校验，不绕过不支持组合 |
| 全局偏好 | 进入现有预览／提交和保存范围；相关配置变化使旧预览失效，被动状态刷新不改变配置源修订 |
| 保存 | 两个全局键进入非行设置足迹、保存失败恢复、取消及冷启动验证，不能仅改 INI 写入点 |
| 启停 | 共用实际准入、运行代次、停止任务排空及后端释放结果；停止未完成时不开放配置修改 |
| 权限 | Read 用于被动查询，Configure 用于偏好／行配置，ManageDevices 用于主动连接检查，TestInput 用于明确测试发送；正常映射沿用 RunMapping |
| 配置演示 | configuration_only_demo 在产品／客户端调用入口拒绝设备初始化和系统输入；允许配置展示，不以 UI 禁用替代底层守卫 |
| Popup 自动化 | 作为后续受限窗口角色单独接入发现／打开／捕获／关闭；不自动扩大通用控件访问 |
| 返回结果 | 区分配置已改、已保存、实际后端已选、输入调用已接受及清理已完成；未知结果不得报成功 |

以上是合流目标，不是当前 master 或现有 MCP 已提供的能力声明。具体工具／action 名称在 IB-05 对齐当时正式协议；优先扩展既有查询和预览／提交能力，不凭本计划增设任意命令、任意 Qt 属性或任意 INI 写入接口。

### 8.3 合流检查点与流程

1. IB-01 记录 master 基线、依赖接口、配置值和后端状态语义；建立双方共享修改面清单。
2. IB-03 接入启动／停止／退出前，只读复核 MCP 最新稳定检查点与相关差异；调整 IbInput 接入位置，公共生命周期修复单列，不擅自修改 MCP 仓库。
3. 独立功能形成获授权的聚焦提交后，在单独集成工作树／集成分支进行试合并；MCP 工作区继续自身任务。
4. 采用保留共同历史的合并，逐项审查共享源码、翻译和部署冲突；不以复制整文件覆盖。
5. 先跑受影响配置、生命周期和配置演示用例，再做真实后端与发布包验收。只验证整合后的实际源码／EXE，不能把两边各自通过相加视为合流通过。
6. 当前只授权本地检查点；不 push 或合并 master。后续发布／正式合入另按届时明确指令执行。

## 9. 阶段计划、能力状态与验收记录

### 9.1 阶段表

| 阶段 | 状态 | 内容 | 完成标准／暂停点 |
|---|---|---|---|
| DOC-00 | 完成 | 审查、确认决策、新分支、handoff 文档及本轮推送 | 文档／提交白名单／远端一致后停止；不开始开发 |
| IB-01 | 待验证 | 固定依赖、API／能力差异、必要第三方修正决策、最小客户端骨架和三目标构建接入 | 来源／许可／ABI／架构及选定构建通过；未作真实输入结论 |
| IB-02 | 待验证 | 全局偏好、枚举往返、Popup 与被动状态 | 配置／保存／错误状态和私有 UI 通过；不自动连接驱动 |
| IB-03 | 待验证 | 相对移动、候选选择、启动前回退与生命周期 | 四类移动发送、聚合冲突、故障／停止通过；先完成 MCP 生命周期差异检查点 |
| IB-04 | 进行中 | 键盘、按钮、滚轮、定位及复杂序列 | 按能力逐项通过才开放；不支持项明确保留 |
| IB-05 | 进行中 | 只读接口对齐完成，独立集成分支试合并未执行 | 配置／保存／权限／运行代次／只配置演示／窗口契约通过 |
| IB-06 | 未开始 | 双 PC、最终三架构部署与交付 | 能力表、环境覆盖、实际包和未验证限制一致；不能以单后端代表全部 |

每阶段按可验证切片推进。开发开始前填写本阶段选定范围、估算和必要 Default／权限场景；完成后记录实耗与剩余。阶段编号不是自动执行后续阶段或全套门禁的授权。

### 9.2 当前能力状态表

| 能力 | 当前实现／证据 | 开放条件／剩余 |
|---|---|---|
| IbInput 方式／配置／Popup | 值 3 已接四个保存／读取白名单、四个编辑回显、全局偏好及共享 Popup；全局偏好模拟往返通过；行全路径仅源码审查 | 产品配置往返、中文优先三语／主题／DPI／焦点待验；批准的 TS/QM 已完成，不能据此宣称视觉通过 |
| 自动／手动及启动前回退 | 明确候选顺序、能力筛选、回退冻结及发送失败不切换；模拟通过 | 缺失依赖／共存后端与产品准入负例待验；旧 Logitech／Razer 输入仍未开放 |
| 四类相对移动及相对坐标命令 | 全部发送点已接线、合成组混用拒绝；正式仅 G HUB 相对移动；自有客户端探针四次小幅正反移动已收到 | Key2Mouse／Joy2Mouse／Gyro／触摸板与实际产品映射、零／大位移、停止待独立验证 |
| 键盘／左键／垂直滚轮 | 单事件、槽位、分格、同后端持有／释放代码及模拟已做；真实抽查在初始化中止 | 仅验证目标可用；Win11 固定单代次实际接收 DOWN/UP、±1格及停止释放通过再开放；F24 抽查不能替代所有键 |
| 右／中／X1／X2、多按钮、拖拽 | 正式公共 API／补丁保留；客户端与产品准入明确拒绝 | 逐按钮、保持、拆包、取消／停止真实证据后扩大守卫；不靠关闭句柄认定释放 |
| 水平滚轮 | Logitech 明确不支持；Razer 未开放 | 对应 Razer 版本、设备及真实单位／释放用例通过后实现／开放 |
| 绝对定位／点击／位置恢复 | 识别并明确拒绝；不会默默走原生绝对输入 | 负坐标副屏、虚拟桌面原点、坐标／按钮保持及生命周期证据后实现／开放 |
| 复杂序列／连发／锁定／反馈 | 有配置保留及准入拒绝；未宣称实现完整 | 同键回环、停止任务、取消、退出和真实设备证据后逐项开放 |
| 生命周期 | 本轮代次、任务快照、阻止迟到 DOWN、实际池排空、同后端 UP／关闭及释放失败阻止重启已实现；客户端并发／失败模拟通过 | 产品快速切表暂停止并说明，完整退出／故障／阻塞仍待验；G HUB 复连错误 5 未解决 |
| MCP | 只读对齐最新稳定生命周期／源读取／保存／演示入口，接口清单见第 11 节 | 未搬入 MCP；合流后共用代次、实际停止观察、权限与 configuration_only_demo 底层守卫 |
| 特定应用、其他 OS／厂商／版本 | 未验证 | 每个实际环境单列，不从客户端或单系统结果外推 |


### 9.3 验证记录格式

每个可验证切片填写一行或一小段，至少包含：

~~~text
记录 ID／日期／文档修订：
阶段及源码提交／相关未提交差异：
机器／OS／Qt／架构／厂商软件和驱动版本：
实际 EXE 绝对路径／独立 INI／PID／桌面（涉及运行时）：
操作与捕获方法／测试场景／实际结果：
当前检查／历史复用／未执行覆盖：
原始证据相对位置／来源 PC／是否已传输：
失败原件及修正说明／剩余问题：
本地开发检查点或独立提交切点／实际提交状态：
~~~

本次 DOC-00 只记录分支、交接文档、聚焦提交与远端核对；具体自引用提交号从本文件 Git 历史读取，避免为回填同一文档提交号制造循环提交。源码审查与哈希比对不替代构建，构建不替代输入，组件／模拟负例不替代真实驱动释放。证据保留必要日志和图片，不默认制作全源码指纹或重复收据。

## 10. 双 PC 接续与文档同步

### 10.1 Git 内外边界

- 随 Git 同步：handoff/README.md、本文、dependency_provenance.md；以后获授权的功能源码和维护测试按其真实目录提交。
- 本机忽略材料：plan/ibinput_simulator/include、x64／x86 制品、原 BUILD_INFO.md，以及 out/ 中日志、图片和构建目录。
- 原 plan/ibinput_simulator/qkeymapper_ibinputsimulator_integration_plan.md 仅留指向本文的导航。缺少它不影响新克隆的接续，也不再需要传输另一份主计划。
- handoff/ 不存放 DLL／LIB／EXE／PDB、凭据或完整日志。缺少本机制品时按 dependency_provenance.md 的固定源码重建，或显式传输已有制品后核对身份；本轮不上传二进制。
- 公共 README 复用 MCP 的通用交接规则。本分支只列实际存在的 IbInput 入口；最终合流合并两项任务导航，保留 MCP 专属工具说明，不整文件覆盖，也不制造指向不存在文件的链接。

### 10.2 源 PC

同一任务同一时间只有一台 PC 写入。换机前：

1. 完成或明确停在当前切片；若运行过测试，释放本轮输入并正常关闭自有目标，不关闭用户或 MCP 实例。
2. 更新第 0 节交接卡、阶段表、能力表及必要验证记录，列出未提交材料和下一项明确动作。
3. 本次用户已授权 DOC-00 的聚焦文档提交／GitHub 推送；核对暂存文件仅为三份 handoff Markdown，不强制添加忽略的 plan/ 或依赖制品。
4. 后续提交／推送按届时明确授权执行。推送只针对 codex/ibinput-simulator，核对远端完整 SHA，不强制推送、不合并 master。
5. 告知接收 PC 的分支及检查点，列明本机独有材料。未提交、未推送的内容不能假定已同步。
6. 源 PC 停止写入后，才交由接收 PC 写入同一任务。跨机传输的制品仅在身份需要时校验，不默认制作全源码指纹。

### 10.3 接收 PC

1. 以本机实际仓库路径作为 REPO_ROOT，不依赖 PC-A 的盘符。保留本地改动，核对功能分支与指定检查点；已有本地分支只允许可快进同步，分叉或冲突先核实，不 reset／clean 覆盖。
2. 阅读本机 AGENTS.md、工作流、handoff/README.md、本计划顶部交接卡和依赖记录；核对实际代码与文档提交一致。
3. 新 PC 可以从 Git 内的文档恢复；不要求原 plan/、原 MCP 工作目录、个人 Codex 数据库或聊天转录存在。
4. 探测本机工具链、软件／驱动和桌面，独立构建必要目标。不复制另一 PC 的 out、个人 INI、凭据、gsudo 缓存或配对材料。
5. 只执行接续和当前切片所需检查；Win10／Win11、不同后端／版本结果分别记录。
6. 缺少原始材料的结果标记为历史复用记录，需重新证明的场景复跑；不能写成当前已核实。
7. 更新同一主计划顶部卡片，不创建“最终版”、第二套进度表或长期聊天转录。

MCP 仓库路径仅为本机参考位置，另一 PC 可不同。需要对齐时读取其实际稳定提交与主计划；不复制整套正在开发的 MCP 工作区来补齐 IbInput 环境。

### 10.4 后续 Agent 接续指令

~~~text
先阅读本仓库 AGENTS.md、.agents/skills/qkeymapper-workflow/SKILL.md、
.agents/context/project-memory.md、handoff/README.md 和本计划顶部交接卡。
核对实际仓库、分支、源码／文档检查点，保留已有改动。
2026-10-29 用户已明确恢复开发并授权聚焦本地提交。
从本文 R5 顶部卡和第 11 节实际检查点继续；不能把本地提交当作阶段验收完成。
只在独立仓库执行本次授权的 IbInput 切片，不改动原 MCP 工作区。
到输入生命周期和最终合流检查点时，按第 8 节只读对齐 MCP。
完成选定验证后更新本计划，准确区分当前、复用和未执行证据。
本轮允许聚焦本地产品提交；不 push、不合并 master；
不自行扩大真实输入、提权或 master 合并范围。
~~~

## 11. 当前检查点、验证与 Win11 接续

### 11.1 已实现代码与检查点性质

这是“本地开发检查点”，包括待验证实现，IB-01～04 尚未全部验收。依赖来源／许可证／正式 API／PE 机器类型与导出导入已经核对，用户分别批准 0001／0002；配套 x64／x86 重建和原版对照模拟通过。未使用 AnyDriver、DLL 手写函数表或全局 SendInput Hook。

客户端统一持有与调用锁、只读状态副本、候选、冻结回退、逐事件能力／代次准入、6 个普通键槽、停止释放和不完整结果；连接检查的零能力连接不能发送输入。正式仅 G HUB 相对移动；QKM_IBINPUT_VALIDATION_PROBE 只由固定探针／替身测试工程定义，不在产品 pro 中定义，不是用户配置开关。键／左键／垂直滚轮代码保留待验证，未永久删除。

配置值 3 的普通／共通保存读取、导入导出及四个编辑回显已接线；映射码／拷贝沿用既有整数成员。全局偏好走现有 dirty、全局加载与保存失败恢复；Popup 被动刷新不初始化驱动，主动检查在 Worker 串行，运行／清理时不改偏好。需翻译的英文 Qt 源可由正式提取器识别。用户批准的 62 项已更新正式三语 TS/QM；纯品牌及三语相同固定模板使用 QStringLiteral／notr，保留内部状态文字翻译。

Worker 覆盖四类连续移动和相对坐标、键／按钮／滚轮的既有分流点；保留固定 VK 强制 SendInput、文本／剪贴板及原本后台语义。IbInput 只加本轮快照、代次、停止准入和现有线程池实际排空，未复制 MCP 框架。绝对恢复不会回落原生定位。快速切表暂正常停止并提示待验证。

### 11.2 本机当前检查（PC-W10，相关源码相对 DOC-00 的本任务差异）

| ID | 当前执行与结果 | 证据／限制 |
|---|---|---|
| DEP-P1/P2 | 两架构原版五组问题逐项 exit 2，补丁五组 exit 0；参数非法模式原版 exit 2、补丁 exit 0；配套 DLL、LIB、正式头及许可证核对 | out/ibinput/logs/x64、x86；原版 C4715 和早期配置失败保留，修正版构建未检出警告；无真实设备 |
| BUILD-Q6 | 最终 Qt6.8.3 x64 Release 增量构建，原生退出 0；零编译／链接错误及警告，正式 EXE 未启动 | out/ibinput/logs/qt6-product-build-final-with-translations.log；EXE=out/build_qt6/release/QKeyMapper.exe。后续若再改源码／资源须按实际受影响范围更新证据 |
| CLIENT | Qt6 x64、Qt5 x64／x86 各 8 组客户端替身：偏好往返、候选、未初始化／代次拒绝、回退冻结、发送／释放失败、槽满、滚轮与停止组合、能力和并发；均原生 0、无编译警告 | out/ibinput/logs/qt*-client-*.log；正式 C API 替身，不是驱动验证；Qt6 单独编译正式能力门控目标并通过 |
| ABI/UI | Qt5 两架构正式头 C++17 /W4 /WX 链接真实导入库通过；两份修改 UI 的 Qt5 uic 均 0 | out/ibinput/logs/qt5-*-formal-abi-link.log、*-uic.log；未运行 ABI EXE，不代表 Qt5 全产品构建或 UI 通过 |
| TEXT | Qt5.15.2 x64 lupdate-pro QKeyMapper.pro、三语 lrelease 均 0；仅应用批准的 62 项；Linguist lconvert 从每个正式 QM 回读，与 TS／建议逐项一致；未相关旧条目保持不变 | out/ibinput/logs/translations-*-lrelease.log、*-qm-roundtrip.log、translations-formal-lupdate-qt5.log；Qt6 提取虽退出 0 但报 winextras 模块错误，判无效并保留原件；Qt5 提取无模块／编译器错误。实际产品运行时翻译与视觉仍未执行 |
| SOURCE | 实际 diff、编码／LF、配置白名单、正式依赖与 Qt5 API 使用检查 | 本轮 staged 与 working tree 分开核对；out、__pycache__、个人 INI 不提交。__pycache__/ 已加入 .gitignore |

模拟只证明受测软件契约。Qt5 两架构产品 qmake 生成均原生 0，Makefile 已核对匹配架构的导入库与 DLL 复制规则（qt5-*-product-configure.log），未做全产品编译。

正式产品构建不证明其完整映射路径、目标应用接收或无回归。早期客户端 6／7 组及较早构建只作历史执行记录，不代替最终源码的上述检查。曾有测试启动环境缺少 Qt DLL 路径（0xC0000135），已只修测试 PATH 后重跑正式能力门控为 0，失败原件保留。依赖脚本现为选定测试通过后才部署，并拒绝不明 staged／untracked 来源改动。

### 11.3 Default 桌面的真实 G HUB 最小抽查

普通权限、手动 logitech_ghub，正式 x64 DLL 加本次客户端的固定自有接收窗口，不构造产品主窗口、不安装产品 Hook、无个人 INI。EXE 为 out/ibinput/ghub-probe/release/QKeyMapperIbInputGhubProbe.exe；源码在 QKeyMapper/tests/ibinput/ghub_probe.cpp。前台／线程桌面／输入桌面均按探针实际记录核对；Escape、失焦或指针离窗中止，不接管用户实例。原用户 PID21704 由用户退出后，再确认无 QKeyMapper.exe 才运行。

| 场次／本机时间 | 身份与动作 | 实际结果 |
|---|---|---|
| 01 | PID17372，Medium／Default，自有窗 | 指针不在接收区，初始化前拒绝，无输入，正常关闭 exit 2；仅修探针窗位置检查／一次移窗，不放宽前台守卫 |
| 02 23:34 | PID30280，Medium／Default；初始化成功，(+8,0)、(-8,0)、(0,+6)、(0,-6) | 自有窗口收到 4 次移动、方向符合；generation 1 正常关闭，released=1、incomplete=0。紧接着键盘前重新初始化返回 5，整场 FAIL／正常退出 2；F24、左键、滚轮均未发送 |
| 03 23:42（用户明确要求一次复测） | PID5372，普通权限／Default；新进程按相同固定用例重试 | 首次初始化即返回 5，Moves=0、F24=0/0、Left=0/0、Wheel=0/0；FAIL／正常关闭 exit 2，无输入；不继续反复尝试 |

证据：out/ibinput/logs/ghub-smoke-02.log、ghub-smoke-03-retry.log 及 test_snapshots/snapshot_ibinput_ghub_smoke_02.png、snapshot_ibinput_ghub_smoke_03_retry.png；均已自主查看，不随 Git。第二轮失败点是正式 API 的 DeviceCreateFailed(5)，独立复测相同返回，无法归因为模型 capacity；未记录更底层原生错误，具体驱动原因尚不确定。不自行重启 G HUB／服务、重启 PC、提权或撤回失败传播补丁。

固定探针现改为一次准入、同代次顺序执行所有动作，最后释放关闭，与本轮冻结契约一致；当前新探针只编译，不再在本机运行。历史 02/03 使用旧的分动作重连探针，不能当作新版探针通过。复连失败另列生命周期问题，不能靠单代次通过掩盖。

### 11.4 MCP 只读对齐与未执行合流

IB-03 前及收尾均读取 MCP 稳定提交 20c0ea1f02b8accbc2ca31838c436260ee340c43。未修改其源码、计划、个人文件；只有按用户独立请求管理的缓存写其忽略 out。当前功能分支缺少 MCP SourceReadAccess、权限和 configuration_only_demo，本轮未假造接口。

| 共享面 | 合流时具体动作 |
|---|---|
| QKeyMapper 行结构／源快照与编辑 | 值 3 纳入 MCP 正式 schema 与同一校验；保留共通源行身份，不绕过 active snapshot |
| 全局偏好与保存事务 | 两键纳入非行设置足迹、preview/commit、源修订失效及失败恢复／冷启动；GUI 与 MCP 共用 |
| Worker 生命周期 | 把当前 IbInput request/cancel/generation/closeAfterDrain 接到 MCP beginMappingStop／finishMappingStop 和真实 producer/task 排空；不能让两套代次各自判断完成 |
| 底层演示守卫 | 在客户端准备／发送及 Worker 正式入口拒绝 configuration_only_demo 初始化／输入；保留可读配置与状态 |
| 权限／Popup | Read 被动查询、Configure 修改、ManageDevices 连接检查、TestInput 测试发送、RunMapping 正常映射；独立受限 Popup 角色，不能开放任意 Qt 属性 |
| 错误／结果 | 返回实际后端、回退／不支持原因、采样时间和 releaseIncomplete；连接／调用接受／目标收到分开 |

未进行独立集成工作树试合并。当前 G HUB 复连问题、产品排空边界、配置／演示／权限的整合验证仍未满足，不能把本地代码与 MCP 各自构建成功相加为融合通过。先以本轮聚焦提交为稳定输入，Win11 完成必要生命周期证据后，从 MCP 稳定点建立单独集成分支，保留共同历史逐块合并并验证上述契约；不覆盖文件、不合并 master、不修改 MCP 工作区。

### 11.5 Win11 下一切片与完成标准

先核对实际系统、G HUB 版本／相关驱动与进程、普通权限和厂商设备。固定源码／配套 x64 DLL 已随 Git；不需要原 PC out 或历史 plan 目录。先用正式探针检查一次初始化／关闭／复连，保留每次原生返回和接收计数；如仍为错误 5，获取能定位激活失败的最小诊断并先形成独立可审阅补丁／验证。厂商软件启停、提权、重启或设备故障操作需新授权。

得到可靠连接后，执行固定单代次的小幅正反移动、F24 DOWN/UP、左键 DOWN/UP、上下各一格，确认接收窗口的各计数、前台／Default、停止后本轮持有为空和 released=true。它只开放对应基本能力；普通键／左右修饰／扩展表示、右中X按钮、拖拽／拆包／多钮／复杂序列／真实故障释放仍需逐项证据。正式门控的扩大须与记录同时提交，不能设置永久关闭来冒充实现完成。

然后完成四类产品实际映射路径及聚合冲突／反馈负例；值 3 在普通／共通／禁用／复制／映射码／导入导出／编辑／保存失败／冷读完整往返；中文优先三语、主题、DPI、Popup 焦点及运行时语言刷新。完整 Qt5 产品三配置、无依赖启动／DLL 架构部署、ASan、必要静态分析、发布许可部署及 MCP 整体契约留待对应切片。当前 Win10 结果不代表 Win11，更不代表 Win7、其他厂商或其他版本。

### 11.6 Git 外材料与传输

- Git 已含：获批补丁、固定源码版本／许可证、正式公共头、配套 x64／x86 DLL 与导入库、重建脚本、源码／定向测试、文案提案及本计划。
- 不随 Git：out/ibinput/sources 和所有构建缓存／原始日志、test_snapshots 截图、gsudo 本机记录。固定依赖可按 dependency_provenance.md 两条普通权限命令恢复；选定软件检查及探针可在新 PC 重跑，不复制 CMake 缓存。
- 若需复核本机失败原件，由用户单独复制上述 02/03 日志及两张自有窗截图；缺少原件时标记历史记录，不能声称新 PC 已复核。个人 INI／凭据／用户设备身份不传。
- 用户可手工 push 此功能分支，再在 Win11 fetch／只快进同步；或创建 Git bundle 传输。Agent 未 push，也未操作 master。实际完整提交 SHA 和 bundle 基准在最后检查点条目及交接回复给出。
- 用户请求的 3h gsudo PID 缓存只影响既定测试启动／关闭，未扩大授权。控制器 PID8104（Medium），硬截止 2026-10-30 02:10:13；本轮普通探针未使用提权。正式结束时按技能 off 并只读确认，结果补到本计划；不继承到 Win11。

### 11.7 聚焦本地提交顺序

先提交获批补丁、固定正式依赖、许可、重建脚本、报告／参数替身、Python 缓存忽略和主计划／依赖事实。该依赖提交不包含尚在工作区的产品接线、客户端／Popup、其定向测试或正式翻译，不能将计划描述的工作区进度当作该单个提交的源码内容。

再提交产品、客户端、配置／Popup、Worker／生命周期、固定验证探针、定向测试、批准的 TS/QM 及文案稿，并更新本计划。提交前核对暂存清单／编码／差异；均是本地开发检查点，提交不代表阶段或完整功能验收。完整 SHA 于提交产生后记录，HEAD 自引用由 Git 实际日志和交接回复识别。

2026-10-30 故障收尾复核：释放失败后保持 GUI 清理锁定，客户端 busy／偏好修改同样受 releaseIncomplete 约束；不自动重试或假报清理完成。补充同一客户端 failure 模拟断言，重跑受影响目标和最终 Release 后再提交产品；无新 Default 场景。依赖检查点为 e7f7ab3fd0053ad2bf1eace743916418d9de4370，18 个明确白名单文件；产品代码、正式翻译和文案稿仍是本任务相关工作区差异，随下一检查点。

回退状态的主窗口单行明确显示实际 SendInput，配置值仍为 3；“IbInput: %1”为三语相同固定模板，不新增翻译词条。最后 Release 以全部这些工作区差异及正式 QM 为输入。

### 11.8 可复用的测试方法与环境

这是本轮实际方法的恢复说明，不另建进度。接续先按源码和本机变化选择必要项，不因已有用例就默认重跑完整矩阵。REPO_ROOT 使用 Win11 实际独立检出路径；不复制原 PC 的 out、个人 INI 或 gsudo PID。

| Git 内材料／方法 | 恢复与使用 | 桌面／边界 |
|---|---|---|
| 固定依赖、两补丁、许可证和重建脚本 | QKeyMapper/IbInputSimulator、scripts/build_ibinput_dependency.py；版本、两架构命令见 dependency_provenance.md | 普通权限；构建／记录替身，不加载后端 |
| 客户端固定模拟 | QKeyMapper/tests/ibinput/client_test.pro／.cpp，8 组配置、候选、准入、回退、停止、报告及并发；正式 API 是替身，没有导入真实 IbInput DLL | 控制台，无 GUI／Hook／驱动／真实输入，不需要 Default |
| 正式能力守卫模拟 | 同一 client_test.pro 加 CONFIG+=ibinput_production_capabilities，确认未开放输出不进入 API | 同上；不能将验证探针宏带入产品 pro |
| 正式头／导入库 ABI | 两架构使用 C++17、/W4 /WX，取 IbSendInit／IbSendInput／IbSendDestroy 地址链接对应 LIB；PE／导出证据见依赖日志 | 仅编译／链接，不运行 ABI EXE；依赖变化时按需重做 |
| 固定 G HUB 接收探针 | ghub_probe.pro／.cpp，只链接本客户端、正式 x64 DLL 和 Qt Widgets；必须 --ghub-smoke 及 QKM_IBINPUT_PROBE_LOG | 真实接收必须 Default；自有窗口，核对用户实例／物理保持、实际桌面／前台；普通权限，短时焦点、小幅移动、F24、左键、滚轮 |
| 截图和正常关闭 | scripts/capture_ui_snapshots.ps1 -ProcessId ... -WindowHandle ... -PrintWindow；scripts/test_process.ps1 的 Start-QkmTestProcess／Close-QkmTestProcess 保存核对 EXE／PID／StartTicks | 自主查看快照；失败也在 finally 中仅正常关闭自有进程；不按名称关闭用户实例 |
| 后续纯 UI／语言／主题／程序缩放 | 优先实际私有桌面＋PrintWindow及实际线程桌面核对；沿用 qkm_ui_scale、正式 QM 与固定捕获方法 | 私有桌面不切换用户输入桌面。当前独立检出的旧 test_process.ps1 不支持 -DesktopName，不能假称已支持；MCP 稳定 20c0ea1f02b8accbc2ca31838c436260ee340c43 已有 scripts/test_process.ps1、scripts/test_test_process.ps1、mcp_bridge/tests/test_product_interop.py，可复用其中 STARTUPINFO.lpDesktop、身份与捕获方法，先只读对齐，再在独立测试／合流切片接入，不整套搬入产品 |
| 后续 Default 必要场景 | 厂商真实输入和目标接收、实际前台／焦点、物理拖拽／跨屏 DPI、托盘／遮挡交互 | 在本计划列出原因、范围、自有接收进程、输入／截图方式、可见影响、预计用时和授权；静态布局、翻译、程序移窗本身不要求 Default |

本轮纯软件检查只需要普通 Python 3.11 标准库，不需要 pip、MCP 服务或厂商软件。构建需要 VS C++、SDK 10.0.19041.0、Git，依赖重建另需 CMake。Qt 套件和 JOM 本机位于 C:/Qt/Qt6；接续按实际安装调整，不混用 x86／x64 编译环境。Git 已包含配套 DLL，不要求无条件重建全部依赖。

下面示例在仓库根目录复用已有环境和日志函数，不运行真实探针：

~~~powershell
$ibChecks = @"
import sys
from pathlib import Path
sys.path.insert(0, str(Path("scripts").resolve()))
from build_ibinput_dependency import ROOT, OUT, environment, run
qt = Path("C:/Qt/Qt6/6.8.3/msvc2022_64/bin")  # Use this PC's actual kit.
env = environment("x64")
for name in list(env):
    if name.upper() == "PATH":
        env[name] = str(qt) + ";" + env[name]
jom = Path("C:/Qt/Qt6/Tools/QtCreator/bin/jom/jom.exe")
for label, extra in [("qt6-x64", []), ("qt6-production-gates", ["CONFIG+=ibinput_production_capabilities"])]:
    build = OUT / "client-tests" / label
    build.mkdir(parents=True, exist_ok=True)
    run([qt / "qmake.exe", ROOT / "QKeyMapper/tests/ibinput/client_test.pro",
         "CONFIG-=debug", "CONFIG+=release", *extra],
        OUT / "logs" / (label + "-client-configure.log"), cwd=build, env=env)
    run([jom, "-j4"], OUT / "logs" / (label + "-client-build.log"), cwd=build, env=env)
    run([build / "release/QKeyMapperIbInputClientTest.exe"],
        OUT / "logs" / (label + "-client-tests.log"), cwd=build, env=env)
"@
python -B -X utf8 -c $ibChecks
~~~

Qt5 按需使用相同工程，分别选择 5.15.2/msvc2019_64＋environment("x64")、5.15.2/msvc2019＋environment("x86")，用独立输出目录。UI 编译使用对应 kit 的 uic；产品 qmake 只生成 Makefile 并核对 IbInputSimulator/lib/x64 或 x86，不代表全产品编译。Qt6 产品沿用 scripts/build_qt6.ps1 -Jobs 4，以外部 PowerShell -File 调用记录真实退出码，零警告门槛不变。

真实探针先在 out/ibinput/ghub-probe 配置 ghub_probe.pro（Qt6 x64、CONFIG-=debug CONFIG+=release），JOM 编译后复制 Git 内 lib/x64 的正式 DLL 至 release，并用同一 kit 的 windeployqt --release --no-translations --no-opengl-sw --no-compiler-runtime 部署 EXE。不启动产品主窗口替代探针，以免初始化旧后端。探针为普通 asInvoker；权限不足时停止，针对确切启动／关闭场景另行授权，不更改产品 manifest 或提权开发工具。

得到新 PC 对应场景授权、公告开始并核对冲突后，可复用如下方法；这会产生真实输入，不作为恢复环境时的自动步骤。新版探针只初始化一次，在同代次顺序执行固定动作；本机旧 02/03 的分动作重连结果不代表新版通过。

~~~powershell
. .\scripts\test_process.ps1
if (Get-Process -Name QKeyMapper,QKeyMapperIbInputGhubProbe -ErrorAction SilentlyContinue) {
    throw 'Conflicting user/test process; no input started.'
}
$ibExe = (Resolve-Path 'out/ibinput/ghub-probe/release/QKeyMapperIbInputGhubProbe.exe').Path
$ibLog = Join-Path (Get-Location) 'out/ibinput/logs/ghub-smoke-this-pc.log'
$ibOwned = $null
try {
    $ibOwned = Start-QkmTestProcess -ExecutablePath $ibExe -Arguments '--ghub-smoke' -EnvironmentOverrides @{QKM_IBINPUT_PROBE_LOG=$ibLog}
    for ($ibPoll=0; $ibPoll -lt 16; $ibPoll++) {
        Start-Sleep -Milliseconds 700
        if ((Test-Path $ibLog) -and (Select-String -LiteralPath $ibLog -Pattern 'Result=' -Quiet)) { break }
    }
    $ibOwned.Process.Refresh()
    $ibWindow = $ibOwned.Process.MainWindowHandle.ToInt64()
    if (!$ibWindow) { throw 'Owned receiver window missing.' }
    & .\scripts\capture_ui_snapshots.ps1 -Label 'ibinput_ghub_this_pc' -ProcessId $ibOwned.Process.Id -WindowHandle $ibWindow -PrintWindow
    Get-Content -LiteralPath $ibLog
} finally {
    if ($null -ne $ibOwned) {
        $ibExit = Close-QkmTestProcess $ibOwned
        Write-Output ('NativeExitCode=' + $ibExit)
    }
}
~~~

通过须同时满足 Result=PASS、规定接收计数／方向、本轮 released=true／incomplete=false 和正常退出 0；仅 DLL 装载、Prepare 成功、截图或 API 接受都不够。探针核对 Default、前台和实际光标位置；用户允许的助手指针定位只是准备，不算 G HUB 输出证据。复连单独列用例，不混进每类基本动作之间。

### 11.9 与 MCP 的 TS/QM 合流提示

本功能已按批准文本更新 QKeyMapper/translations/QKeyMapper_{en_US,zh_CN,ja_JP}.ts 及各自 QM。MCP 同样修改这些资源，是预期共享面，不能由最后修改者覆盖另一边。

- TS 按 context＋source＋comment 匹配，保留两边新增词条和旧翻译；line／location 变化不构成删除另一功能词条的理由。相同键但译文实质冲突时展示差异审阅，不擅自改批准含义。
- IbInput、IbInput...、IbInput: %1、Logitech G HUB、Razer Synapse 按用户决定为固定文字／notr；不要合流时重新加入冗余翻译。其余 62 项 context、英文、中／日建议、占位符和 HTML 换行见 translation_review.md。
- QM 是生成制品，二进制冲突不单选 ours／theirs；先合并三份 TS，再用集成源码的实际可用 kit 重新 lrelease，保留 translations.qrc 三语资源。
- 合流后执行 lupdate-pro，检查原生退出和模块／编译器诊断；本机 Qt6 工具曾有 winextras 提取错误、Qt5 工具成功，不能只凭 exit 0 判断有效。仅保留相关词条，避免无关重排、位置大改或 obsolete 清理。
- 核对双方词条、占位符及实际 LF／HTML 语义，再做 QM 回读和集成产品三语页面／语言刷新（中文优先）。双方分别通过不等于合流通过；结果回填两任务各自的唯一计划，不创建第三份进度来源。
