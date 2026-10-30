# QKeyMapper IbInputSimulator 集成计划：独立实施、能力验收与 MCP 合流

文档修订：R10，2026-10-30（Asia/Shanghai）。
本文是本功能唯一的设计、决策、实施状态和双 PC 接续来源，随功能分支提交；目录规则见 [handoff/README.md](../README.md)，依赖事实见 [依赖来源与本机制品](dependency_provenance.md)。旧 plan/ 文档仅保留导航，不维护第二份进度。

## 0. 最新交接卡片与本轮停止点

| 项目 | 当前记录 |
|---|---|
| 当前阶段 | IB-04-K1基础键盘人工验证通过；用户另确认基本长按／双击可用，未做复杂测试。开放键值不变；本轮只更新验收记录与筛选键基准，不进入后续开发或MCP合流 |
| 当前授权 | 2026-10-30用户要求记录人工通过、补充FakerInput／系统筛选键验收边界，并再创建一次本地commit；本轮不push、不运行产品或输入测试；用户随后明确允许amend本条尚未推送的文档提交，源码39126f15保持不变。此前K1输入与推送授权保留为历史记录 |
| 执行机器 | PC-W11，Windows 11 企业版 25H2 10.0.26200.9711 x64；G HUB 2026.7.992920，logi_joy_bus_enum／xlcore／vir_hid 只读采样为 Running |
| 实施仓库／分支 | D:/work/mygithub/QKeyMapper/QKeyMapper_dev_temp/QKeyMapper_dev_temp；codex/ibinput-simulator |
| 本轮源码检查点 | 已人工验证的交付源码39126f15ed64ac6dffc2383308387343817cf085，已推送；本次R10仅在其后增加文档本地提交，不改EXE／DLL／INI／TS／QM |
| 接收检查点 | HEAD=4e5c34f1794eda96701a183fa30312238fed496b；依赖=e7f7ab3fd0053ad2bf1eace743916418d9de4370；产品=69dc82ecea68e4008992e1c522e5c2362100675c；DOC-00=53f80dbdb2b0893289118fb76d6e566bc9beb393；bundle 验证通过，接收时工作区干净 |
| 工具链 | 本机 Qt6.8.3 x64，Qt5.15.2 x64／x86，VS2022 BuildTools 14.44 工具集，SDK 10.0.19041.0；实际配置和编译器版本随本机日志记录 |
| 当前验证 | 用户确认基础键盘人工通过，并报告基本长按Q 500ms→F／双击W 500ms→G可用，未做复杂测试。前轮Qt6／Qt5定向检查、Default输入与私有Popup自动证据见12.11；本次不重跑，新增确认与边界见12.12 |
| 原始材料 | out/ibinput/handoff 的 bundle 与 pc-w10-ghub-minimal-evidence-20261030.zip；Win10 运行结果只作历史证据，不冒充 Win11 通过 |
| 本切片范围／估算 | R10只维护主计划及两处配套文档的验收提示，完成文档差异／编码检查后本地提交即停止；K1开发实耗和原始验证记录保留在12.11及Git历史 |
| 桌面与权限 | 本次纯文档维护及普通权限Git操作，无需Default或私有桌面、驱动初始化、真实输入、系统筛选键更改或UAC；K1历史测试场景保留在12.11 |
| MCP 协调 | 本次不暂停或联系MCP线程、不修改其源码／计划；既有接口对齐与接纳顺序保留，完整功能及用户明确合流确认后由原线程重新冻结接纳检查点 |
| gsudo | 本次不查询或更改缓存、不登记测试。12.11中的PID2580／18:30为历史记录，不作为本轮操作依据 |
| 能力范围 | 鼠标指针移动、基础键盘和基本长按／双击已获用户人工确认；复杂场景未测。以FakerInput同条件效果为目标，推荐启用系统筛选键；关闭时两种方式类似的机制性问题由用户决定是否维持，详见1.2 |
| 稳定交付门槛 | 完整功能、正式范围与UI／文档一致后形成固定SHA并交付完整验收；按1.2以FakerInput及推荐筛选键条件判断，用户可接受共同机制限制。当前基础功能通过不自动触发MCP接纳 |
| 合流顺序 | 完整功能及独立验证 → 手册预览／审阅与用户最终验收 → 新固定SHA → MCP原线程检查点／独立集成验证／快进接纳 → 合流后手册复核及release note／发布包收尾；不在本轮执行 |
| 下一项动作 | 完成本次纯文档本地commit后停止，不push。后续研发另行继续：按1.1／1.2审查历史临时门控与实际差异，不把未测或FakerInput共有现象一律作为阻断 |
| 当前性质／未执行 | 独立提交切点（纯文档本地commit）；复杂长按／双击／组合、完整键鼠能力、Win10当前切片、发布级检查、手册／README正式更新及MCP合流仍未完成，本轮不扩大验证范围 |

Win10及Win11各切片历史保留在第11～12节，不重写当时事实。当前状态与停止点以本卡及12.12为准；此前“基本验证后即可合流”的阶段安排由R8的完整功能门槛替代。

状态统一使用：未开始／进行中／待验证／受阻／完成；暂停意图、用户验收和合流许可另行记录。构建、替身、API 接受与目标实际接收分别说明。

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
| 不支持能力 | 同一移动组混用属于结构冲突，保存及启动均拒绝；软件未安装／未连接仍可保存合法配置。其他未开放能力的既有保存策略保持，运行明确拒绝，不静默改写方式 |
| 自动化 | GUI／MCP 最终共用业务规则；当前 master 无需提前引入整套 MCP 代码 |
| 文档位置 | 按用户最新指令在 handoff/ibinput-simulator 维护唯一主计划；plan/ 留导航和本机材料，不修改忽略规则 |

2026-10-30用户已批准IB-04-K1固定输入验证，并在通过后创建新检查点、推送codex/ibinput-simulator；不合并master或MCP分支。后续驱动初始化、真实输入、系统设置副作用和需要 UAC 的场景，必须在对应阶段具体说明并按项目约定获得授权；沿用既有授权时不重复征求同一事项。

不以特定游戏、反作弊或硬件白名单作为功能可靠性保证。目标应用是否接受输入需要独立记录实际环境与结果，不能由驱动签名、初始化成功或 DLL 编译成功推导。

### 1.1 复用优先与准入依据（2026-10-30用户补充）

- 原始键识别、长按／双击／组合、发送时机和队列调度优先沿用现有QKeyMapper实现，尽量只在最终输出处替换发送通道。不能只因某组合未验收，就另写发送方式专属拦截或并行实现。
- 分开记录“已验证”“可由现有流程复用但待回归”“明确不支持／存在具体故障”。待回归通常进入测试清单；后端确实不能表达、已核实会回环、无效参数、释放失败或配置结构冲突才保留对应守卫，并记录依据与恢复条件。
- 风险决定验证：输出原语变化检查真实接收和释放；纯业务复用优先调用链审查及代表性回归；不能由基础键通过外推全部组合，也不因未跑完整矩阵而反向判为不可用。本切片仍只交付60个基础键值，其他表示按后续输出适配切片处理。
- 早期代码中以“await validation”为由设置的复杂序列／连发／锁定等临时门控，下一切片按实际调用链逐项审查：能直接复用则删除多余判断，有具体差异再最小修正；不把门控本身当作已完成适配。来源回环、持有状态／释放失败等具体保护不随意删除。

### 1.2 FakerInput对齐与系统筛选键验收基准（2026-10-30用户确认）

以现有FakerInput发送方式在同系统、同映射和同筛选键设置下的效果为对齐目标，不要求IbInput与SendInput的事件标记机制完全等价。README及使用手册已推荐系统筛选键，减少持续保持物理键时Windows反复产生同一按键的Key_Down；后续以“映射设定”→“系统筛选键”中勾选“启用系统筛选键”作为推荐使用和主要验收条件，未启用状态作为对照观察。

关于IbInput与FakerInput不能正确支持dwExtraInfo的判断，来源是用户此前调查及对通道机制的推断，当前作为待实测核对的工作假设，不写成所有后端／版本均不支持的确定结论。接口接受该参数、静态HID报告结构或应用层ExtraInfo队列，都不足以单独判定最终Hook所见的实际行为；允许在后续开发中再作一次定向验证，并据结果修正说明及实现判断。

后续可在现有LowLevelKeyboardHook入口增加小范围日志，对同一固定键记录发送前INPUT.ki.dwExtraInfo，以及Hook入口未经应用层补标的KBDLLHOOKSTRUCT.dwExtraInfo、vkCode／scanCode／flags和DOWN／UP；同条件对照SendInput、FakerInput和实际选定的IbInput后端，注明回退状态，区分底层原始值与应用层队列推断。复用自有窗口和固定输入用例即可，不要求完整矩阵或新增复杂探针；具体运行按届时输入授权执行，本次不启动该测试。最终仍以FakerInput同条件功能效果为目标，共有问题是否维持现状由用户决定。

若启用系统筛选键时IbInput功能正常，而关闭时出现异常，且FakerInput在相同条件下表现类似，记录最小复现条件、开关状态及双方现象，交由用户判定是否接受并维持现状。未经用户判断不擅自宣称“无法解决”；也不默认把这类共有现象列为IbInput完成或MCP合流的阻断项，不自动追加拦截／规避代码或复杂专项工程。IbInput独有差异、推荐条件下仍不正常的功能或释放故障，应按其具体问题评估。

本节是验收与后续开发边界，不要求本次重跑对照实验，也不改变个人配置、现有筛选键会话管理或历史测试INI。本次人工反馈未逐项注明筛选键开关状态，不据此补造开／关两组通过记录。

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
| R08 | 当前 FakerInput 消费端过滤不等于新通道已经有完整防回环机制 | 按真实执行路径复用消费端规则；核实具体回环／屏蔽差异后最小修正，代表组合列为回归任务，不由未测推定全部不可用 |
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

正式发布验收必须移除“此检查点”等开发阶段用语；用户只看到当前真实支持范围、连接／检查结果和可操作建议。能力限制不能随开发用语一并删除或虚报扩大；三语正式文案随当时实际已验收能力更新，并按TS/QM审阅流程核对。开发提交、验证阶段和剩余任务只保留在handoff及开发记录中。发布前同步对应正式使用说明。

### 4.3 偏好编辑与语言

修改选择立即更新内存偏好并触发既有未保存提示；关闭面板不隐式保存、不连接后端。正式保存仍走现有保存入口。运行或清理期间允许查看，禁用偏好修改和连接检查，明确提示停止映射后调整。

不自动安装、启动、重启或卸载 G HUB／Synapse。不增加任意 DLL 路径、任意命令、按行选择不同厂商或通用优先级编辑器。

界面采用英文 tr／可翻译 UI 源，中文优先，维护英语和日语及运行时刷新。正式 TS／QM 更新前先展示 context、英文源、中文／日文建议、占位符和换行供用户审阅；批准后由 Agent 按项目 lupdate-pro／lrelease 流程完成，不能让用户承担工具执行。本次10/30前62项及追加U01～U10均已获用户批准，按正式提取／限定合并／lrelease流程更新并验证QM；三语相同品牌／固定模板依用户决定不翻译。用户最终人工验收仍待反馈。

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

保存前执行纯结构校验：检查本次保存的每个普通映射表及其实际附加Common，复用现有Common优先／原始键分组去重和禁用行规则；不把互不同时运行的普通表相互比较。同一移动组合中方法集合含3且含另一方式时拒绝整个保存，定位原表／源行；不落盘、不连接驱动、不替换运行映射指针。“保存并开始”在任何保存／末次表名写入前先校验，即使没有dirty标记也不能绕过；单独开始对实际有效映射使用同一判据。安装／连接状态不是保存条件；其他未开放能力的保存策略本轮不扩大。

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

- 同一合成组存在IbInput与其他发送方式混用时，保存前和启动前共用纯规则，指出相关原始源行并拒绝。分组为Key2Mouse、摇杆（左右摇杆共享）、陀螺仪及触摸板；原始触发键不同不拆分Key2Mouse组。保存范围按5.1覆盖各独立运行上下文。
- 不含 IbInput 时保持现有优先规则；不借本功能重写旧发送方式选择机制。
- 同键连发、输出再成为原始触发键、Mouse2vJoy／锁光标／鼠标或键盘屏蔽的相关组合，必须有单独证据才能开放。
- 移动组混用不得通过保存写入；已载入的历史／外部配置保留在内存供修正，启动仍受拦截。其余尚未开放能力继续沿既有保存策略并显示限制；运行准入使用与UI一致的规则。识别不到可证明安全的组合时保守拒绝，并给出受限项，不能影响无关普通映射。

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

#### 7.1.1 IbInput新增部署文件与正式包门槛

IbInput专属的运行包增量为根目录IbInputSimulator.dll及licenses/IbInputSimulator/下四份许可；.lib与头文件只用于开发，不作为用户运行依赖。Qt6 x64、Qt5 x64复制lib/x64的配套DLL，Qt5 x86复制lib/x86；不能混放架构。

| 运行包相对路径 | 源位置／用途 |
|---|---|
| IbInputSimulator.dll | QKeyMapper/IbInputSimulator/lib/<x64或x86>/IbInputSimulator.dll，进程装载所需 |
| licenses/IbInputSimulator/IbInputSimulator-LICENSE.txt | 同组件licenses目录，IbInputSimulator许可 |
| licenses/IbInputSimulator/IbWinCppLib-LICENSE.txt | 同组件licenses目录，IbWinCppLib许可 |
| licenses/IbInputSimulator/Detours-LICENSE.txt | 同组件licenses目录，Detours许可 |
| licenses/IbInputSimulator/rapidjson-LICENSE.txt | 同组件licenses目录，RapidJSON完整声明 |

2026-10-30只读核对用户指定的out/ibinput/manual-w11-20261030与MCP仓库plan/bin_release/QKeyMapper_Qt6_x64：上述DLL和四份许可与本仓库来源一致。测试目录另有INI、build-identity、MCP提示词、验证说明，以及Qt插件／运行库和既有许可的差异；这些不全部属于IbInput新依赖。测试目录也未带齐原发布包的驱动工具、PDF、模板和辅助工具，不能整体替换正式部署基底。更完整的依赖事实见dependency_provenance.md。

ibinput.pri已有构建后复制DLL；本轮未完成正式发布脚本对新增DLL／许可的显式接入和部署回归，不把手工测试目录当作发布完成。IB-06需在scripts/release/build_release.ps1及其既有测试中补齐三组合清单、缺文件／错架构拦截、许可内容和ZIP实际内容核对；保留原依赖／驱动工具及从本次源码生成的使用手册。G HUB／Synapse是用户环境中的外部软件，不打入本程序运行包。隐式链接下缺少或装错IbInputSimulator.dll会阻止程序装载，需由部署阶段发现。

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

### 7.4 使用手册：IB-04功能稳定后编写，IB-05合流后复核

按用户指定，在功能／界面／正式支持范围稳定后使用[qkeymapper-manual-update](../../.agents/skills/qkeymapper-manual-update/SKILL.md)。维护源是readme/manual/，发布文件是readme/QKeyMapper使用手册.html；修改chapters.html、必要真实截图、coverage.md和维护说明，再由build.py生成，不能直接手改生成HTML。

内容至少包括：发送方式的选择入口；G HUB／Synapse前置条件和已验证后端；无实体罗技设备／有GPW等场景的区别；自动／手动选择、启动前回退；安装／进程／驱动／当前连接及最近结果的含义；刷新与连接检查的不同作用；W→F及小幅移动等已开放示例；保存、Common／禁用／混用规则、停止与退出、失败排查和真实能力限制。用户正文不写“检查点”或验证工具细节。

先在IB-04-DOC生成out/user_manual/中的预览，检查新增章节、索引／搜索、链接、离线内嵌图片和深浅主题；截图使用实际中文深色默认比例界面，按需捕获，不伪造设备／游戏结果。提交预览供用户审阅；依该技能，发布HTML及对应本地提交在获批后执行，当前计划安排不代替届时具体内容审阅。

IB-05保留并融合MCP对同一手册源和稳定ID的修改，按合流后的实际UI／能力复核，再生成同源发布HTML。IB-06确认压缩包包含当前手册，并按已有在线发布流程核对在线内容／入口；单HTML内嵌资源，不增加独立截图目录依赖。本轮不改或发布手册。

### 7.5 Release note与暂定发布版本：IB-06发布准备

功能和MCP集成结果稳定、手册说明可引用后，使用[qkeymapper-release-notes](../../.agents/skills/qkeymapper-release-notes/SKILL.md)，默认联动qkeymapper-readme-en-sync。按实际正式release tag到固定HEAD收集已提交更新，展示中英完整草稿，获批后限定范围更新并分两阶段本地提交；保留MCP及其他功能的有效更新，不以本分支README整文件覆盖。

暂定将当前待发布的v1.5.0(Build 20261024)标题调整为v1.5.1(Build 20261122)。这是计划目标，不承诺该日前完成；若未完成，Build日期在发布准备时按实际进度重新确定。已发布的历史条目按正式tag边界保留，不覆盖；本轮不修改README标题、程序版本或发布配置。

IbInput说明保持简短：写清新增发送方式、实际已支持的后端与必要前置，并引导“详细使用方式请参考《QKeyMapper使用手册》离线或在线文档”。沿用现有README的手册入口；发版前核对在线链接和离线文件，不把计划能力写成已发布能力。程序版本／Build、包名和更新元数据届时按正式发布流程统一核对。README编辑／本地提交授权不自动包含push、tag、GitHub release或master合并。

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
| 保存 | 两个全局键进入非行设置足迹、保存失败恢复、取消及冷启动验证；移动组混用在预览／提交／GUI保存前共用结构校验，失败无落盘或驱动副作用，不能仅改INI写入点 |
| 启停 | 共用实际准入、运行代次、停止任务排空及后端释放结果；停止未完成时不开放配置修改 |
| 权限 | Read 用于被动查询，Configure 用于偏好／行配置，ManageDevices 用于主动连接检查，TestInput 用于明确测试发送；正常映射沿用 RunMapping |
| 配置演示 | configuration_only_demo 在产品／客户端调用入口拒绝设备初始化和系统输入；允许配置展示，不以 UI 禁用替代底层守卫 |
| Popup 自动化 | 作为后续受限窗口角色单独接入发现／打开／捕获／关闭；不自动扩大通用控件访问 |
| 返回结果 | 区分配置已改、已保存、实际后端已选、输入调用已接受及清理已完成；未知结果不得报成功 |

以上是合流目标，不是当前 master 或现有 MCP 已提供的能力声明。具体工具／action 名称在 IB-05 对齐当时正式协议；优先扩展既有查询和预览／提交能力，不凭本计划增设任意命令、任意 Qt 属性或任意 INI 写入接口。

### 8.3 合流检查点与流程

1. IB-01 记录 master 基线、依赖接口、配置值和后端状态语义；建立双方共享修改面清单。
2. IB-03 接入启动／停止／退出前，只读复核 MCP 最新稳定检查点与相关差异；调整 IbInput 接入位置，公共生命周期修复单列，不擅自修改 MCP 仓库。
3. **IB-04完成后才进入正式IB-05**：对照现有FakerInput的实际功能建立完成矩阵，目标功能实现／开放、停止释放及必要兼容检查通过；确实受上游限制的差异须由用户确认，不能把长期门控当作完整实现。手册预览已按稳定功能准备，用户完成完整功能验收并明确同意合流后，形成新的聚焦固定提交。当前鼠标移动人工通过不满足该门槛，旧版接纳提示词不再直接使用。
4. MCP原线程先完成当前最小切片并正常清理自有测试，形成本地合流前检查点；从该SHA创建独立集成工作树／codex/ibinput-mcp-integration，经本地Git获取上述固定IbInput提交并保留共同历史合并。逐项处理共享源码、TS／QM、手册源和部署冲突，不整文件覆盖。
5. 在集成工作树验证配置、生命周期、权限、configuration_only_demo底层零驱动／零输入守卫及受影响真实后端；复核合流后的手册和实际部署。只验证合流后的实际源码／EXE，不能将双方各自通过相加。必要静态、实际ASan及Qt5两架构兼容按接纳计划完成。
6. 原MCP分支仍停在约定检查点时，由原线程以快进方式接纳到D:/work/mygithub/QKeyMapper/QKeyMapper的codex/qkeymapper-mcp-control；分支已推进则先对齐复验，不强制覆盖。更新两项唯一计划及公共导航，然后继续MCP开发。
7. 手册最终复核、release note及三组合发布包在IB-06收尾。当前不通知／暂停MCP线程、不合流、不push、不合并master；届时按明确指令执行。

## 9. 阶段计划、能力状态与验收记录

### 9.1 阶段表

| 阶段 | 状态 | 内容 | 完成标准／暂停点 |
|---|---|---|---|
| DOC-00 | 完成 | 审查、确认决策、新分支、handoff 文档及本轮推送 | 文档／提交白名单／远端一致后停止；不开始开发 |
| IB-01 | 待验证 | 固定依赖、API／能力差异、必要第三方修正决策、最小客户端骨架和三目标构建接入 | 来源／许可／ABI／架构及选定构建通过；未作真实输入结论 |
| IB-02 | 待验证 | 全局偏好、枚举往返、Popup 与被动状态 | 配置／保存／错误状态和私有 UI 通过；不自动连接驱动 |
| IB-03 | 待验证 | 鼠标指针移动已获用户人工通过；其余移动来源、候选／回退及完整生命周期仍分项验收 | 不从指针移动外推四类来源、真实故障释放或整阶段完成；MCP差异在IB-05统一 |
| IB-04 | 进行中 | 对齐现有FakerInput实际功能：键盘、按钮、滚轮、定位及复杂序列；本轮基础键盘输出自动验证通过 | 按能力矩阵复用既有路径、适配实际差异并分项验收；不由未测新增禁令，最终固定SHA经用户完整功能验收 |
| IB-04-DOC | 未开始 | 稳定功能对应的使用手册源、截图、覆盖表和预览 | 使用qkeymapper-manual-update，预览审阅后更新生成发布HTML及聚焦本地提交；见7.4 |
| IB-05 | 未开始 | 已有只读接口对齐作为历史准备；IB-04完整功能及用户合流确认后由MCP原线程正式接纳 | 独立工作树合并／验证，配置、停止排空、权限／演示及文档融合通过后快进更新MCP功能分支；见8.3 |
| IB-06 | 未开始 | 双PC、最终三个构建组合部署、合流后手册复核与交付 | DLL／四份许可／当前手册及ZIP内容完整，实际包与验收能力一致；见7.1.1、7.4 |
| IB-06-NOTES | 未开始 | README中英release note及暂定v1.5.1(Build 20261122) | 使用qkeymapper-release-notes审阅／实施；简述IbInput并指向离线／在线手册，日期按进度再定；见7.5 |

每阶段按可验证切片推进。开发开始前填写本阶段选定范围、估算和必要 Default／权限场景；完成后记录实耗与剩余。阶段编号不是自动执行后续阶段或全套门禁的授权。

### 9.2 当前能力状态表

当前以IB-04-K1为准；具体历史证据见第11～12节，未改变路径的历史检查按原配置复用。

| 能力 | 当前实现／证据 | 剩余事项 |
|---|---|---|
| IbInput方式／配置／Popup | 方式3、偏好、保存／冷读及Popup绑定见12.8；U11正式三语资源及私有Popup五场景本轮通过 | 用户最终UI体验、其他配置入口和完整发布矩阵继续验收 |
| 自动／手动及启动前回退 | 候选顺序、能力筛选和回退冻结已有替身及相关产品检查 | 多厂商／缺失依赖实际组合待补；旧Logitech／Razer未作为本轮可用后端交付 |
| 相对移动 | G HUB Key2Mouse指针移动用户人工通过；其他相对来源已接发送点 | Joy／Gyro／触摸板、边界坐标及相应停止场景分项回归 |
| 基础键盘输出 | G HUB的A-Z／0-9／F1-F24正式开放；60键替身与代表路径自动通过，用户已确认本轮基础键盘人工验证通过 | 其余键值／表示分后续适配；人工反馈未逐键枚举，不外推全部组合 |
| 长按／双击等原始触发 | 沿用既有流程，未增加专用触发守卫；用户确认基本长按Q 500ms→F、双击W 500ms→G可用 | 未做复杂测试；复杂时序、取消／停止组合、设备编号及特殊发送时机按实际风险补回归，不重复要求基础功能验收 |
| 左键／其他按钮、垂直滚轮、拖拽 | 公共接口和既有适配保留；本轮未扩大正式能力 | 审查临时门控与具体状态差异，按可复用路径及实际接收／保持／释放补齐 |
| 水平滚轮 | 当前Logitech后端没有实现，属于实际能力差异 | 核对Razer后端及表示／单位，不以接口同名推定支持 |
| 绝对定位／点击／位置恢复 | 当前适配未提供等价能力 | 评估复用现有坐标处理后的最小发送适配，再检查副屏／虚拟桌面坐标和释放 |
| 复杂序列／连发／锁定／反馈 | 历史临时准入仍在；本轮未将其宣称为不可用或完整实现 | 按1.1审查并清理无具体依据的限制；具体反馈循环、槽位和释放问题保留保护 |
| 生命周期 | 代次／任务快照、停止排空、同后端UP、失败阻止重启已接；本轮持有键停止、代次1→2、退出后接收UP通过 | 完整故障／阻塞／快速切表等验收仍未完成；旧G HUB错误5及0004修正历史见12.4，不继续列为当前未解决复连结论 |
| MCP | 保留接口对齐及正式接纳计划，当前未合流 | 完整功能／用户确认后由原线程独立集成并统一生命周期、保存、权限和演示守卫 |
| 其他OS／厂商／应用 | 本轮仅PC-W11当前G HUB环境 | Win10＋GPW及其他实际组合单列，不从本机外推 |

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
- 用户请求的 3h gsudo PID 缓存只影响既定测试启动／关闭，未扩大授权。控制器 PID8104（Medium），硬截止 2026-10-30 02:10:13；本轮普通探针未使用提权。已于 2026-10-30 00:31:22 按技能 off，返回 closed／user_off、generation 2、cache_available=false、native_sessions_observed=0，拥有／未知／在途／当前动作／作业均空；独立 status 确认无受管控制器、原生会话 0。不继承到 Win11。

### 11.7 聚焦本地提交顺序

先提交获批补丁、固定正式依赖、许可、重建脚本、报告／参数替身、Python 缓存忽略和主计划／依赖事实。该依赖提交不包含尚在工作区的产品接线、客户端／Popup、其定向测试或正式翻译，不能将计划描述的工作区进度当作该单个提交的源码内容。

再提交产品、客户端、配置／Popup、Worker／生命周期、固定验证探针、定向测试、批准的 TS/QM 及文案稿，并更新本计划。提交前核对暂存清单／编码／差异；均是本地开发检查点，提交不代表阶段或完整功能验收。完整 SHA 于提交产生后记录，HEAD 自引用由 Git 实际日志和交接回复识别。

2026-10-30 故障收尾复核：释放失败后保持 GUI 清理锁定，客户端 busy／偏好修改同样受 releaseIncomplete 约束；不自动重试或假报清理完成。补充同一客户端 failure 模拟断言，重跑受影响目标和最终 Release 后再提交产品；无新 Default 场景。依赖检查点为 e7f7ab3fd0053ad2bf1eace743916418d9de4370，18 个明确白名单文件；产品代码、正式翻译和文案稿随后已纳入 69dc82ecea68e4008992e1c522e5c2362100675c；受影响的最终 Release 和三配置模拟复查通过。

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

### 11.10 最终本地检查点与手工传输

| 提交 | 包含内容／性质 |
|---|---|
| e7f7ab3fd0053ad2bf1eace743916418d9de4370 | 固定获批依赖、许可、0001／0002、两架构正式 DLL／导入库、重建与无驱动报告测试、缓存忽略、主计划／依赖事实；依赖独立提交切点 |
| 69dc82ecea68e4008992e1c522e5c2362100675c | 方式 3、客户端、全局偏好与 Popup、移动／键钮轮／代次排空释放、定向测试／固定探针、批准的 62 项正式 TS/QM、文案稿及测试方法／合流说明；本地开发检查点，包含未验收／未开放实现 |
| 本段所在后续文档封存 HEAD | 如实记录上述完整 SHA、最终工作区、缓存关闭和交接方法；用 git rev-parse HEAD 取得实际完整 SHA，避免正文递归自引用 |

当前功能分支 codex/ibinput-simulator，未 push、未合并 master。产品／依赖提交后工作区干净；本次文档封存只修改本计划。MCP 仍为 20c0ea1f02b8accbc2ca31838c436260ee340c43，未修改其源码／主计划，原无关 .claude/settings.local.json 保留。Git／构建保持普通权限；受限沙箱曾无法核实缓存控制器，不能将那个空查询当成已关闭。随后以同一普通登录身份核实原 instance／PID8104，再正常 off 并独立确认原生会话 0；未接管其他缓存或扩大测试。

已在忽略 out/ibinput/handoff/pc-w10-ghub-minimal-evidence-20261030.zip 封存 02/03 两份真实抽查日志和两张已查看的自有窗截图，CRC 检查通过。这四份原件可由用户随 bundle 单独复制；不包含个人 INI、凭据、用户桌面、完整构建日志或缓存。旧探针 EXE 后来被新版构建替换，不能拿当前 EXE 代替旧场次的二进制身份或声称新版通过；接收 PC 需按所选场景重新构建并注明当前／历史证据。

文档封存后，在本机 out/ibinput/handoff/qkeymapper-ibinput-simulator-20261030.bundle 创建仅此功能分支的 Git bundle，并执行 git bundle verify；前置提交为 DOC-00 53f80dbdb2b0893289118fb76d6e566bc9beb393。该 bundle 为本地派生传输物，不随 Git。用户自行复制到 Win11，不自动网络传送或 push。接收步骤：

~~~powershell
git status --short
git bundle verify X:\transfer\qkeymapper-ibinput-simulator-20261030.bundle
git fetch X:\transfer\qkeymapper-ibinput-simulator-20261030.bundle codex/ibinput-simulator
# Existing feature branch: preserve local work and allow only a fast-forward.
git switch codex/ibinput-simulator
git merge --ff-only FETCH_HEAD
# If the local feature branch does not exist, use instead:
# git switch -c codex/ibinput-simulator FETCH_HEAD
~~~

出现本地改动、分叉或 bundle 前置提交缺失时保留现场并核实，不 reset／clean。可改由用户手工推送功能分支再在 Win11 fetch；Agent 如需 push 必须另获授权。bundle 不含 MCP 分支的未推送历史；IB-05 前须确保 Win11 另有实际 MCP 稳定检出，按其自身任务流程传输，不能把参考 SHA 当作已同步。

Win11 接续顺序与完成标准仍见 11.5，测试恢复和桌面选择见 11.8，TS/QM 合流见 11.9。未执行的产品路径、完整 UI、Qt5 全产品、其他后端／系统、ASan／静态分析／发布包与 MCP 合流仍未通过；本地提交不改变这些状态。

## 12. Win11 当前执行记录

### W11-01：接收、本机恢复及首次真实初始化

2026-10-30 接收 HEAD4e5c34f1794eda96701a183fa30312238fed496b，分支codex/ibinput-simulator，接收时干净。完整提交链、bundle及四份Git内DLL/LIB与依赖记录一致；未重复导入。系统Win11企业版25H2 26200.9711 x64，G HUB2026.7.992920；MSVC19.44.35228.0、SDK10.0.19041.0、Qt6.8.3 x64。Win10两份原日志从ZIP只读复核，保留为历史。

| 检查 | 当前结果 | 证据与边界 |
|---|---|---|
| Qt6普通Release初始构建 | 原生0、零错误／零警告 | out/ibinput/logs/w11-qt6-product-initial.log；out/build_qt6/release/QKeyMapper.exe；产品未启动。后续Popup修正尚需增量更新 |
| Qt6客户端 | 八组替身全部exit0；正式能力门控exit0 | w11-qt6-client-*及w11-qt6-production-gates-*；无真实DLL／驱动／Hook／输入 |
| 新固定探针构建 | 原生0，无编译警告；已部署正式x64 DLL | w11-ghub-probe-*；添加连接／关闭／一次复连模式、Medium／并发实例守卫和逐动作接收计数 |
| Default连接探针01 | FAIL，首个IbSendInit即返回DeviceCreateFailed(5)，后续复连及全部键鼠发送未执行，正常关闭exit2 | PID18176、Medium、实际线程／输入桌面Default；w11-ghub-connect-cycle-01.log及snapshot_ibinput_w11_connect_cycle_01.png，已查看。窗口有Wheel2/0、delta240/0观察，来源不明，不作为G HUB输出证明 |
| MCP运行冲突前检 | 当时0个QKeyMapper进程，控制器无current_job/action/owned/unresolved | 10:04:52只读采样，原线程正在构建；只代表当时状态，不锁定后续时段 |

初始化失败后没有重试，没有提权或改变厂商软件／服务。错误5是IbInput枚举；真实Windows底层错误仍未知，不宣称权限原因或连接／复连通过。

### W11-02：私有Popup与最小诊断提案（进行中）

实际Popup组件测试使用正式QM、真实私有桌面及C API替身，不构造完整产品，不证明产品主窗口／真实映射。初次截图从Default执行失败，旧关闭助手无法定位私有窗口；该fixture按90秒超时退出，无遗留。失败原件在out/ibinput/ui-checks/w11-01，不计通过。

随后只读复用MCP现有普通权限STARTUPINFO.lpDesktop助手，在同一自有私有桌面运行本仓库capture_ui_snapshots.ps1。PID40228，桌面QkmIbInputUi_40228；中文／英文／日文100%浅色及中文／日文150%深色五场，组件断言、偏好读回、被动刷新不变性、运行禁用、子下拉／Escape／隐藏停刷均通过，init_calls=send_calls=0，正常exit0。五张原图已自主查看；这是组件及程序化交互，不冒充Default物理操作或完整产品通过。

图中发现G HUB版本数字字段显示2026.7.2920.0而字符串为2026.7.992920，另有深色链接低对比；已局部修正为字符串版本优先、固定字段后备和仅帮助链接的深色颜色。无翻译文案修改、无输入行为变化；修正后构建／截图尚待执行。

最小0003提案及说明在[out/ibinput/diagnostics/ghub-native-error-0003](../../out/ibinput/diagnostics/ghub-native-error-0003/README.md)，仅保存／恢复失败点GetLastError，不改变错误5或设备调用。两架构记录替身的获批基线错误被清理覆盖、提案保留原错误及五组回归已通过；数字仅为模拟。固定普通权限一次初始化诊断目标已/W4 /WX编译，尚未执行。正式patches／DLL／LIB均未应用提案，等待用户评审；不因提案编译成功宣称真实原生错误已确认。


### W11-DIAG-0003：已批准的原生错误诊断（进行中）

用户2026-10-30明确批准0003最小诊断补丁、配套依赖重建及一次普通权限固定初始化诊断。只保留失败点原生错误，不改变错误5、设备请求或成功判据；不发送键鼠、不主动取焦点、不启停厂商软件，失败后不重试。新增补丁已进入正式patches，重建脚本只追加dllmain.cpp到恢复列表；DLL／LIB待实际构建结果记录。本次批准不覆盖后续改变驱动行为的修正、提权或其他真实输入。

### W11-DIAG-0003 当前实际结果与0004待评审提案

0003两架构原版／补丁对照、五组报告、参数返回、PE／导入库检查及部署已完成，原生均符合选定门槛；原版预期失败单列。x86已包含维护测试新加的即时错误保留断言。正式x64 DLL SHA256=F0B13BE01A684A1A6C7870C7B3B05BF27EE5D094DF558FB8F061012F9AB0F096，x86 DLL=6F435DE5EFDE6D51AC80255E95319380E089936E364267EC2A7D0E0BB1A09894；最终依赖记录在切片检查点时统一更新。

获批的一次真实诊断PID26480，Medium／Default，IbInputError=5、ActivationWin32Error=87，InitCalls=1、InputSendApiCalls=0、ForegroundUnchanged=1、CursorDelta=0,0，正常exit2。证据out/ibinput/logs/w11-ghub-native-error-0003-once.log。仅证明当前激活请求参数被拒绝，不是权限不足，也不是连接成功；没有第二次尝试或gsudo。

Popup修正后PID34084／QkmIbInputUi_34084的五场再次通过，正式版本字符串断言2026.7.992920、init/send为0、正常exit0；中文默认及日文放大深色新图已查看，其余场景原图保留。Qt5 x64／x86实际Popup组件均构建0，客户端各八组替身0。中途一次本机编排把日志Path传成str，未执行相应客户端；修正调用后只续跑未完成项，不记成产品失败或省略覆盖。

只读PnP确认G HUB总线已有正常的虚拟鼠标后代。同类参考实现对87的“已存在”说明仅作线索，不据此直接放行。0004提案用实际选择总线的HardwareID／鼠标类／DN_STARTED／故障位及有界树查询判定；两架构六组记录替身与负例通过，固定只读目标在阻断实际驱动打开时同一判据返回true。详见[out/ibinput/diagnostics/ghub-existing-mouse-0004/README.md](../../out/ibinput/diagnostics/ghub-existing-mouse-0004/README.md)。尚未应用0004或执行相应初始化，等待用户对这项驱动准入变化的独立评审。


### W11-FIX-0004：已批准复用正常虚拟鼠标及兼容矩阵（进行中）

2026-10-30用户明确批准0004及继续既定最小验证。正式patches已加入0004，现有报告维护测试接入CfgMgr替身和六组记录回归；不引入新测试框架。先前0003不等于本项批准，分别记录。

新增兼容设计决定：实体罗技鼠标／键盘不是G HUB后端准入条件。判据只使用本次选择的G HUB虚拟总线及其已启动、无故障Mouse后代；存在则复用，不存在则走原创建路径，未知／故障继续拒绝。实体设备型号只作为测试环境维度，不把某个型号硬编码到驱动选择或准入。

| 环境维度 | 设计行为 | 当前证据／后续 |
|---|---|---|
| 仅安装G HUB，无罗技实体设备 | 根据虚拟总线／鼠标状态判断，不因没有实体硬件拒绝 | 用户确认当前Win11正属此类；0004只读判据在本机成立，真实连接／输出待执行 |
| 安装G HUB并连接GPW等实体设备 | 使用同一虚拟后端，实体设备不改变自动／手动选择规则 | 用户确认Win10使用GPW；其旧版短移及失败仅历史，本修正版需在Win10独立复核 |
| 虚拟鼠标已正常启动 | 复用同一总线的现存节点，避免重复创建 | 两架构替身＋本机只读判据已通过，真实复连待验 |
| 总线存在但虚拟鼠标未创建 | 保留原有激活请求；仅已证实的创建竞争可复用 | 无驱动分支／负例已验证；本机当前不是该状态，不擅自移除设备造场景 |
| 软件未装、驱动未启动、节点故障、查询未知 | 状态如实显示；默认阻止，已选启动前SendInput回退按原规则处理 | 无驱动负例覆盖部分；不安装／卸载、停服务或改设备来补矩阵 |
| 不同版本、实体设备插拔、共存其他厂商、重启后恢复 | 沿相同虚拟后端与冻结／清理规则；软件／实体／虚拟驱动／实际接收分层记录 | 后续按具体版本及权限分批验收，不从单一PC外推 |

目前正式仍仅开放G HUB相对移动；键盘、按钮、滚轮的固定探针不自动扩大产品能力。


### 12.4 W11-02 最新验证与产品收尾（前述提案／待执行状态的后续）

0003和0004均已分别得到用户明确批准并配套重建；前文提案时记录仅作过程证据，当前正式制品以下表及dependency_provenance.md为准。MSVC19.44.35228.0、SDK10.0.19041.0、CMake3.31.12／NMake；Qt6.8.3 x64及Qt5.15.2两架构。未改变公共头／导出ABI／IOCTL报告布局，未安装卸载驱动、启停厂商软件或提权运行探针。

| 当前检查 | 结果／证据（本机out，不随Git） |
|---|---|
| Qt6最终普通Release | 原生0，零编译／链接错误及警告；w11-qt6-product-final.log；out/build_qt6/release/QKeyMapper.exe。产品尚未启动 |
| 两架构0004配套 | 原版对照、六组报告／就绪负例、参数边界、PE／导入库及部署通过；w11-dependency-*-0004-driver-free.log、logs/x64与x86/review-*；修正版无编译警告 |
| 修正后私有Popup | w11-03五场均exit0、init/send=0；版本2026.7.992920断言通过。中文／英文／日文100%浅色及中文／日文150%深色五张原图均已查看，深色链接可读；仅组件及程序化交互 |
| Default连接、正常关闭及一次复连 | PID17108／Medium／实际线程与输入桌面Default；两次prepare/close成功，released=1、incomplete=0、输入计数全0、正常exit0。ibinput-w11-connect-0004-activation.log及自有窗截图已查看 |
| 同代次基本接收 | PID38740／Medium／Default；四次X/Y正反方向及窗口事件正确，F24=1/1、左键=1/1、轮=1/1且delta120/-120；released=1、incomplete=0、正常exit0。ibinput-w11-smoke-0004.log及截图已查看 |
| 独立运行目录 | out/ibinput/manual-w11-20261030；本仓库正式EXE、当前x64 DLL、Qt6及既有依赖／许可、合成keymapdata.ini已部署，未启动。w11-manual-runtime-deploy.log |
| 产品配置固定目标 | product_check.pro／.cpp已编译，待新增启动范围批准；无映射启动或真实输入。w11-product-check-build.log |

0004初场在前台守卫中止，两次后台焦点准备亦被Windows拒绝；这些场次均未prepare，不是驱动连接失败。随后固定探针等待正常激活，最多90秒；逐动作仍检查Default、前台和冲突。自有窗口临时置顶避免遮挡，没有放宽守卫、注入按键强抢焦点或为焦点提权。最终连接及接收通过如上；无遗留测试进程。

用户确认本机仅安装G HUB、没有罗技实体键鼠：现已通过0004连接／复连及固定接收。实体设备不是后端准入条件，不提示用户购买罗技设备。Win10＋GPW仍待0004独立复验，不能引用旧版短移作本版本通过。不存在／故障／未启动／错误总线／查询失败及其他原生错误有两架构负例；不删除设备或停服务造场景。其他版本、实体插拔、共存厂商和重启恢复仍待按环境补验。

被动Popup当前显示文件版本、进程名和驱动登记；登记不等于设备就绪，未知不改称未安装。后续细化服务／设备状态时仍需三语文案审阅，不把连接检查放进自动刷新。正式产品仍只开放G HUB相对移动，F24／左键／滚轮探针通过不自动扩展正式能力。

#### 下一项固定产品验证：尚未批准的新范围

实际main入口的product_check保留正式管理员manifest、正常析构和线程排空；manual_verification.ini仅有F8/F9/F10/F11四条方式3 Key2Mouse，关闭自动开始、开机启动、虚拟手柄和多设备。

拟在独立QkmIbProduct_私有桌面执行save和cold两次启动，核对真实配置加载、方式3保存／重启读回、全局偏好和主窗口Popup绑定；只程序化Qt控件操作及PrintWindow，不开始映射、不点IbInput连接检查、不发送键鼠。实际入口仍经过既有Hook／Interception／FakerInput等初始化，因此普通客户端探针授权不足。预计约2分钟，无Default窗口／焦点／键鼠影响；缓存首次建立可能有UAC。记录实际桌面、PID、捕获及原生退出；捕获不足则停止，不擅自切Default。

拟复用MCP现有普通受管控制器，仅固定原生目标Start／Close；须在原MCP忽略的out/ibinput_acceptance/w11-20261030暂存经核对副本，保留原允许项，不修改MCP源码／计划／缓存时长。新增范围尚未批准，未暂存／登记或启动。若用户选择自行验证，产品路径如实标未执行，不将组件与探针结果改称产品通过。

稳定提交、人工步骤和固定SHA合流提示词在收尾结果明确后填写；当前尚未进入用户基本验证停止点，未向MCP线程发消息或合流。


#### 12.4 产品验证已获批；首场调度异常（未计通过）

用户已批准前述两次私有产品验证及MCP独立out暂存／固定登记。复用控制器PID25336时，首场原生启动回执明确Success=true、PID9904；维护目标在QkmIbProduct_私有桌面完成方式3／偏好保存断言，写出ready.json。控制器却在publish原子替换state.json时WinError5退出，原请求c7e8fdd081b74b1b93cc5bc75449c6b5未完成。没有重放；产品等不到截图确认而按90秒超时写passed=false，冷读未启动。不能宣称该产品用例通过。

控制器closed.json记录cache_available=false、native_sessions_observed=0，但仍current_action=Start，不能以缓存关闭冒充完整动作清理证明。随后实际QKeyMapper固定目标／CrashReporter均已无进程；未开始映射，无测试持有输入。客户端未及时取得启动回执而没保存原生退出句柄，本场原生退出码未验证。原生回执、controller.log和产品结果保留在各自out。

本机运行器另有PyHDESK.Close方法错误，已按现有MCP方法改为CloseDesktop后Detach，仅修正本任务out脚本。MCP缓存源码未改，也未自动重建／关闭其他缓存或扩大授权。需用户明确重新调用缓存技能后继续已获批范围；新结果使用retry-02子目录保留首场。正常验证仍不轮询读取共享state.json文件；所有失败证据保留。


### 12.5 W11-02 稳定检查点的实际证据与剩余边界

| 项目 | 最终本轮结果 |
|---|---|
| 依赖独立提交切点 | 5258b37988f21efe4bdbdb57d7553196bd816350；只含0003／0004、四份配套DLL/LIB、恢复脚本／报告替身及依赖事实。补丁数据中的单空格空上下文是unified diff必需标记；其余暂存内容空白检查通过，补丁已按构建脚本实际apply检查／构建 |
| 实际产品入口保存 | retry-04/save，PID34132，实际QkmIbProduct_私有桌面；四条Key2Mouse方式3、切换自动／手动及失败偏好、真实saveKeyMapSetting、主窗口按钮到实际Popup绑定通过；正常托盘退出／main线程排空，原生0 |
| 实际产品入口冷读 | retry-04/cold，PID39548，另一自有QkmIbProduct_私有桌面；重新启动读回3#3#3#3及logitech_ghub／block，Popup绑定通过，正常退出0 |
| 私有产品图像限制 | 普通High私有PrintWindow明确失败；未提权截图、未把该图像项算通过。五场已查看的Medium真实Popup组件截图仍有效。retry-03/save/capture.log保留 |
| 调度修正与历史 | retry-02因截图阻塞而超时；retry-03配置断言true但普通quit被关闭到托盘行为阻止。维护目标改用现有onTrayIconMenuQuitAction，retry-04两项退出0。前三场保留，不改作通过；产品源代码未为测试改动 |
| 运行目录及样例 | out/ibinput/manual-w11-20261030，正式Qt6 x64 Release和当前DLL、Qt／既有依赖与许可；合成INI位于QKeyMapper/tests/ibinput/manual_verification.ini，操作说明见manual_verification.md，不复制个人INI |
| 正式能力 | 仍只开放G HUB相对移动；F24／左键／垂直轮实现保留，客户端最小接收通过不等于产品全路径验收 |
| 未执行覆盖 | 真实产品Hook→Key2Mouse、其他三类移动路径、完整配置复制／映射码／共通／导入导出／保存失败矩阵、真实故障释放、Qt5全产品、实际ASan、定向静态分析、干净系统／发布包及MCP合流均未通过 |

实际产品维护目标复用main.cpp、正式manifest和原启动／退出；不开始映射、不点连接检查、不发送键鼠。源码QKeyMapper/tests/ibinput/product_check.pro／.cpp，使用QKM_IBINPUT_PRODUCT_CASE=save或cold、QKM_IBINPUT_PRODUCT_OUTPUT和QKM_IBINPUT_PRODUCT_DESKTOP，后者须为实际QkmIbProduct_私有桌面。外部inspection-complete只声明普通截图结果，不决定配置断言。构建为Qt6 Release，最终恢复至已通过的私有配置版本；维护构建零警告。本机运行器和登记材料位于本任务out及MCP独立out/ibinput_acceptance/w11-20261030/retry-04，不是产品依赖或第二份进度。

用户后来明确允许13:10前午休期间按推荐自主使用Default。曾准备额外产品Hook→相对移动用例；第一次空闲守卫拒绝，未启动；第二次固定原生helper返回Success=true、PID10928，但控制器在identity_check阶段失败，原请求5cc569f6588d44ad868e75722bf1e814保留为unresolved_start。没有用例ready／result，不计产品启动或输入通过；原因未定位，不能称G HUB失败。控制器随后closed，cache_available=false／native_sessions_observed=0仍不代表原动作完整清理。实际抽查无本次QKeyMapper或CrashReporter进程存活，未盲目重放、恢复缓存或修改MCP核心。

未验证的Default测试扩展已从提交源码撤下，仅在本机out/ibinput/diagnostics/product-relative-unverified.cpp保留；正式产品未被改动。两次维护目标新增期间出现的switch作用域和SDK头顺序编排错误原日志保留，不当作产品失败。实际产品Hook→移动留在用户基本验证，不能用客户端接收或配置结果补签。需要后续缓存诊断时由原MCP线程按自己的任务处理，当前不代发消息。

本轮没有修改正式TS／QM或新增可见文案，没有改动MCP源码／主计划，只按用户具体批准使用其忽略out及普通共享控制器。当前不push、不合并master或MCP分支。

#### 用户基本验证与合流停止点

请按[人工验证步骤](manual_verification.md)在独立目录执行；需要暂避MCP真实输入时由用户通知原线程。反馈若不通过，保留本检查点、修复后创建新提交／新交付，不amend已交付版本。

状态：待用户基本验证。当前是本地开发检查点，不是IB-01～04完整验收。稳定提交后本线程停止修改对应源码，直到用户给出结果。只有用户明确“基本验证通过，可以合流”后，才将交付的固定SHA提示词送给MCP原线程。最终位置仍为D:/work/mygithub/QKeyMapper/QKeyMapper的codex/qkeymapper-mcp-control；原线程做自己的合流前检查点、独立集成验证及快进接纳，再继续开发。


### 12.6 本轮提交与文档封存

| 检查点 | 完整SHA／范围 |
|---|---|
| 依赖独立提交切点 | 5258b37988f21efe4bdbdb57d7553196bd816350；0003／0004与两架构配套依赖／定向验证 |
| 稳定源码检查点 | 208a717262567c00b0694af726aac22e578e9b5b；产品版本／深色显示、固定探针与已验证私有配置／UI测试、合成INI、人工步骤及真实覆盖边界 |
| 文档封存 | 本节所在后续提交，仅本计划登记上述完整SHA；与稳定源码检查点之间没有源代码或二进制变化。完整交付SHA从git rev-parse HEAD、独立目录build-identity.txt及交付回复取得，避免递归自引用 |

当前分支codex/ibinput-simulator，本轮均为本地提交，不push、不合并master或MCP。交付性质：本地开发检查点，待用户基本验证；不是完整输入能力或IB-01～04验收。正式EXE SHA256=9722F521437F7439591D08415151FAA62A435BBE83B0EFBC9BBC1C9A2D450EF6；依赖身份见dependency_provenance.md。独立目录out/ibinput/manual-w11-20261030包含实际EXE、当前依赖、合成keymapdata.ini、人工说明及已填完整交付SHA的MCP_RECEIVE_PROMPT.md。后者仅供用户确认通过后发送，不提前触发合流。

W11-02封存时缓存状态为关闭／native_sessions_observed=0，并保留5cc569f6588d44ad868e75722bf1e814未知启动记录；不是仍生效至14:51的3h窗口。本条是历史状态，本轮现有新控制器复用结果见顶部卡和12.8。当前无本轮自有产品／CrashReporter进程。不重放原请求、不接管MCP缓存恢复。原MCP线程未被通知、暂停或修改代码。


### 12.7 W11-UX-01：用户基本验证反馈与已批准修订

原稳定交付c98303bb5777124e69bd8a1400383ad366136529保留。用户反馈了八行Key2Mouse混用、主窗口按钮／状态样式、Popup操作无反馈及开发提示；尚未确认“基本验证通过，可以合流”。本轮修订完成后重新交付，不能沿用旧版的待验说明作为通过证据。

1. **保存规则变更已批准**：旧版c98303bb允许先保存再在开始阶段拒绝混用，截图红字不代表当时保存失败。用户认为这种结构冲突应在保存时阻止，已明确批准5.1边界；当前实现已改为保存／保存并开始／开始共用判据。所保存各普通表独立与实际附加Common形成有效上下文，使用原有去重／禁用规则；不放宽共享移动输出限制，不因缺少厂商软件阻止保存。
2. **布局和状态已修改**：主入口去掉省略号，沿相邻按钮141×22基准和同一缩放规则；状态Label复用FakerInput的WinPanel／Sunken、居中、字体／内边距。显示“未连接”、可读品牌、停止／清理未完成及活动回退，不把断开后的历史后端ID当作当前连接。
3. **操作反馈已修改**：刷新只观察安装／进程／驱动登记，手动操作显示本地完成时间；被动刷新不覆盖最近手动反馈、不修改配置。连接检查原有bool结果传回Popup，显示检查中及成功／失败；成功明确临时连接已关闭，不再把残留“已连接”作为当前连接。实际连接／关闭顺序、驱动调用、能力和回退规则不变。提示色适配深浅主题。
4. **正式文案已批准并实施**：U01～U10按项目Qt5 lupdate-pro成功提取，再限定合并已审阅词条、lrelease生成三语QM并round-trip核对；保留无关词条和旧资源历史。用户可见“检查点”已改为“目前仅支持 G HUB 鼠标相对移动”，真实能力限制继续保留。最终人工验收仍待用户进行。

过长源行红字继续沿现有格式；简短组名摘要／明细查看是后续UX任务，需要新增文案时另行审阅，本轮未扩展。未修改MCP源码／计划或第三方依赖，不合流、不推送；实际权限和验证见12.8。

### 12.8 W11-UX-01：本轮定向证据与新版交付

源码基线为c98303bb5777124e69bd8a1400383ad366136529；新的稳定源码／资源／维护测试检查点为b0ac332eb2834488829ffbe52402d520a2aa4065。当前文档封存仅登记该完整SHA与停止点，没有源码／QM或二进制差异；完整交付SHA由git rev-parse HEAD、独立目录build-identity.txt和最终回复给出，避免自引用。本轮没有修改0001～0004或依赖制品，继续使用5258b37988f21efe4bdbdb57d7553196bd816350的x64 DLL／LIB。不是IB-01～04全部验收。

| 当前检查 | 实际范围与结果 |
|---|---|
| 共用纯规则 | validation_test.pro使用真实MAP_KEYDATA和共用头文件；八行不同原始键混用、源位置、禁用排除、统一方式、旧方式组合、四类移动组、独立组及普通键盘通过。Qt6 x64／Qt5 x64／Qt5 x86构建及运行均原生0，无厂商DLL、窗口或输入 |
| 构建 | Qt6.8.3 x64普通Release：out/build_qt6/release/QKeyMapper.exe；同配置product_check目标；Qt6／Qt5 x64／Qt5 x86 Popup目标最终增量构建均0、无新警告。没有重跑Qt5完整产品或依赖重建 |
| 三语资源 | w11-ux01-lupdate-qt5.log提取原生0，无模块／编译器提取错误；三语lrelease与QM反向转换全部0。逐项核对U01～U10、context、占位符和无关翻译不变；三个被替代源仅标vanished，不从历史TS清除 |
| Popup实窗 | 最终w11-06：PID35908，实际QkmIbInputUi_35908私有桌面；中文100%浅色、英文100%浅色、日文100%浅色、中文150%深色、日文150%深色五场通过，原生0；正式QM、刷新／检查中／成功／失败反馈、控件边界、运行禁改和语言刷新断言及截图已检查。C API为替身，init_calls=0、send_calls=0 |
| 实际保存入口 | 已批准受管原manifest产品目标，PID18160，QkmIbProduct_f71e41bbdca14a2ea3a6365dd94f308c；四场拒绝：无dirty标记的保存并开始、单独保存八行混用、非当前普通表冲突、实际附加Common冲突。每场拒绝后INI逐字节不变；单独保存还核对运行指针／busy／requestedGeneration不变 |
| 不误拦截／冷读 | 同一产品save用例中，禁用Common、未附加Common、被Common优先规则遮蔽的行均可保存；恢复四行方式3及全局偏好后正常退出。第二次PID20316，QkmIbProduct_209dcb6a5a1a4b2fbfe421b973214b36，实际冷读方式3及偏好、主窗口Popup绑定通过，两次原生退出0 |
| 主窗口布局属性 | 产品用例核对按钮与高级设置同宽、正式翻译、状态Label与FakerInput的frameStyle／alignment／margins一致。High私有主窗口截图仍未验证；不将Qt属性断言冒充完整主窗口视觉通过 |
| 系统副作用 | 无Default桌面场景、桌面切换或真实输入；没有开始映射、点真实IbInput连接检查、安装／启停厂商软件。产品原入口保留既有后端初始化；仅固定原生启动／正常退出使用现有缓存，开发／构建／截图普通权限 |

原始证据仅在本机忽略目录：out/ibinput/logs/w11-ux01-*，out/ibinput/ui-checks/w11-06/result.json及test_snapshots/snapshot_ibinput_w11-06_popup_*；产品隔离副本／登记／逐场结果在MCP仓库out/ibinput_acceptance/w11-20261030/ux01。另一PC应按源码复验或明确缺少本机证据，不能把这些路径当作已随Git传输。

早先w11-04组件场景2曾在“运行期间控件冻结”检查失败；结果和零init/send计数保留。之后改用明确objectName定位控件并在检查前显式重开Popup，w11-05及最终w11-06通过，没有放宽产品的运行禁改条件。纯规则目标初次缺少Qt Widgets／已有宏声明的编译尝试保留在日志，修正目标声明后才记三配置通过。

本轮未验：Default主窗口整体视觉／焦点、真实产品Hook→移动及用户手感、真实连接按钮成功／失败反馈的新版端到端路径、Qt5完整产品、ASan／定向静态分析、干净未安装环境、Win10＋GPW下0004、其他移动来源和键钮轮完整产品能力。旧客户端／驱动探针证据只按12.5原范围复用。

新版独立目录：out/ibinput/manual-w11-ux01-20261030，使用新正式EXE、相同依赖和重新复制的合成INI，附README-验证.md／build-identity.txt／MCP_RECEIVE_PROMPT.md。旧out/ibinput/manual-w11-20261030及用户已经编辑的INI完全保留，不覆盖、不迁移其测试状态。新EXE SHA256=00B83EA14E0CE5C82279545DCB422BB5195325ED1E4F3E439B0B679DDD597E8A，x64 IbInputSimulator.dll SHA256=13BDF841A5B15057A939BC1DC5C41FC1848FDE433DCC5B0941248D16FDB9D548。

停止点仍是“待用户基本验证”。按manual_verification.md复验本轮反馈；用户最终人工验收未发生，本线程不自判通过、不发送MCP接纳消息。失败时修复并形成新提交，不amend已交付检查点；明确“基本验证通过，可以合流”后才由原MCP线程接纳。

本轮缓存最终只读快照见out/ibinput/logs/w11-ux01-cache-final.json：PID2580仍active／截止18:30，已无IbInput定义授权及自有目标／未知启动；保留MCP其余范围和原截止。本线程未执行off，未创建／恢复缓存；等待人工验收期间不派发新的测试。该快照只说明本轮时点，不保证另一会话后续缓存状态。


### 12.9 R8：鼠标移动人工通过、部署核对与后续阶段

2026-10-30用户明确：“已经进行了IbInput鼠标指针移动的功能的人工验收，验收OK”。记录该功能人工通过，不扩展为键盘、按钮／滚轮、其他移动来源、完整UI／配置或整项功能完成。本轮引用的manual-w11-20261030用于deploy比较，不据此推定所有后续版本／修订均已获人工验收。

本轮只读比较该目录与MCP仓库plan/bin_release/QKeyMapper_Qt6_x64，确认新增IbInputSimulator.dll为本仓库当前x64配套版本，四份licenses/IbInputSimulator文本与来源逐字节一致；Qt5两组合尚未做发布包运行验收。目录其他差异及三组合正式部署门槛见7.1.1；既有计划有DLL／许可要求，本次已补明确路径及发布测试任务。

用户要求的手册／release note已安排在7.4／7.5及阶段表：稳定功能后用指定技能编写手册，合流后复核；发布准备时用指定技能形成简短中英release note，并引导手册。v1.5.1(Build 20261122)为暂定目标，未完成则重新确定Build日期，当前不改产品版本或正式发布文档。

合流门槛已由“相对移动检查点基本验证后可接纳”调整为“目标功能完整、独立验证及用户完整验收／明确合流确认后正式IB-05”。MCP原线程继续其工作，届时负责暂停、检查点、独立集成及快进接纳；最终目标路径／分支不变。旧运行目录中的MCP_RECEIVE_PROMPT.md仅是历史草案，届时须按新的固定SHA、能力及验证范围重新生成，不能直接触发当前合流。

R8计划维护结束时的状态：本地开发检查点上的未提交文档修订；当时HEAD为8bafc07cbcab4b6226f814bc55b0e77b09fd589c，产品源码检查点仍b0ac332eb2834488829ffbe52402d520a2aa4065。仅修改本计划、dependency_provenance.md及manual_verification.md中的旧合流提示，无源码／EXE／INI／TS／QM、手册／README或MCP检出改动，无构建、GUI、输入、gsudo、提交、推送或合流。后续功能开发需另按阶段继续；本轮到计划更新后停止。


### 12.10 R8文档检查点与远端同步

用户随后要求核对是否适合创建检查点并push；本轮按该新指令形成三份handoff文档的独立提交切点：本计划、dependency_provenance.md、manual_verification.md。前一节“不提交／推送”是当时停止边界，本轮同步授权仅覆盖本功能分支，不恢复后续开发或MCP接纳。

提交前fetch仅更新origin/codex/ibinput-simulator，远端为4e5c34f1794eda96701a183fa30312238fed496b，本地无落后、领先5个提交；待推送内容包括已获批的0003／0004配套依赖、Win11及UX源码检查点和文档。本次再加入R8文档提交，采用普通快进push，仅更新refs/heads/codex/ibinput-simulator，不强推、不改master或MCP分支。完整新SHA和实际远端核对结果以Git与最终交付回复为准，不在本文件递归自引用。

本轮复用此前源码／构建／输入证据，只对文档、范围、编码和远端祖先关系做检查；没有新增产品构建或输入测试。out中的运行目录、个人INI、截图／日志／缓存不会随Git推送；另一PC按当前提交、主计划及本机环境恢复构建。当前仍是“鼠标移动人工通过，完整功能待补齐”，不是正式release，也未进入MCP合流。


### 12.11 IB-04-K1：2026-10-30基础键盘切片（自动验证通过，待用户验收）

用户目标为18:20前完成下一合适切片并再推送一次，已明确批准本切片和固定真实输入范围、U11三语。基线11eceaa5fd5a47e35c78959f2e5041fc36f99145；旧检查点及运行INI保留，不amend。

本轮范围：G HUB基础单键输出A-Z、0-9、F1-F24。原始触发与发送时机保留既有流程，不为长按／双击／组合／设备编号等新增专用准入；本轮实际输入只验普通单键。修饰／符号／其他键值表示、按钮／滚轮等完整输出能力未扩展；复杂输出的历史门控按1.1继续审查，不作为永久能力结论。原Key2Mouse混用／Common规则与其他发送方式不变；不修改第三方依赖或MCP源码。

选定验证：Qt6 Release；Qt6及Qt5两架构的生产门控／纯规则，Qt6既有客户端替身；基础输出60个键值／本轮范围外表示、停止后UP、失败释放、代次与并发断言。实际产品保留main.cpp和正式manifest，在Default自有接收窗验证W→F、数字及F键代表路径、按住后停止、重新开始及退出释放；独立普通接收进程用于观察产品退出后的UP。原始输入由普通测试控制器固定发送，目标／前台／桌面改变立即禁止新DOWN并仅释放本轮自有键；不操作其他窗口、鼠标或系统组合快捷键。

Default确有必要：厂商驱动真实输入及全局Hook路由／退出后释放接收不能由私有桌面组件证明。前台占用预计3～5min，原始键不涉及Ctrl／Alt／Win／Shift组合；运行前公告，核对无用户实例、MCP测试空闲、接收窗身份、Default输入桌面及物理保持。普通截图用项目脚本；High捕获不足不提权截图或假报通过。其余构建／源编辑／Git／采样均普通权限，只有原manifest原生启动／关闭复用已批准受管入口；不重建缓存、不扩大其既有其他范围。

U11来源及三语记录见translation_review.md。先完成真实功能验证，再交付与能力一致的正式资源／新独立运行目录；用户人工复验与完整功能／MCP门槛继续保留。本轮时间边界不替代通过标准，若有未解决故障则保留原门控、记录未完成范围，不以赶18:20宣称通过。


**最新设计修正**：用户指出“未验证”不应自动变为“不允许”。已撤回本轮新增的isBasicKeyboardRow及worker专用检查和镜像测试，相关三个文件精确回到基线内容；不修改既有触发识别或发送时机。调用链核对：长按／双击均进入emit_sendInputKeysSignal_Wrapper，键盘最终在sendInputKeys／单键发送分支按方式选择FakerInput或IbInput。可自然复用，但尚无这类触发的实际接收验收。该原则同样适用于后续各部分，已写入1.1与能力表。

**源码范围**：生产变更仅qkm_ibinput_client.cpp的60键白名单与G HUB Keyboard能力、qkm_ibinput_popup.cpp的U11说明及三语TS/QM；既有业务处理、保存规则、厂商初始化／销毁、第三方DLL均不改。新增／扩展的维护测试、合成INI和本文档随同提交；完整SHA见Git及交付回复。

**当前自动检查**：

- Qt6.8.3 x64普通Release及维护目标构建原生exit 0，交付构建日志为out/ibinput/logs/k1-product-delivery-build.log、k1-product-check-delivery-build.log；Qt6选定构建无项目警告。开发／构建均普通Medium。
- Qt6／Qt5 x64／Qt5 x86生产客户端替身核对0～255键值中的完整60键集合、120次DOWN/UP、未开放表示在初始化前拒绝、移动能力不误放键盘及失败／并发释放。三组合纯移动规则通过；Qt6既有客户端8组替身通过。分别见k1-<kit>-client_test-*、k1-<kit>-validation_test-*与k1-client-probe-*日志；后续只撤回新行守卫，未改变客户端或旧纯规则的有效输入。
- 实际产品测试保留main.cpp与正式管理员manifest，原生测试EXE为D:/work/mygithub/QKeyMapper/QKeyMapper/out/ibinput_acceptance/w11-20261030/k1-02/runtime/QKeyMapperIbInputProductCheck.exe，SHA256为0713fbe1f0f6ac7f30249dba8ed5712aab7906b5ab5eba736387a8377f219893。PID11452，普通接收窗PID32868，实际桌面Default。普通测试器仅固定发送W／1／F13，产品经真实LL Hook、实际G HUB输出F／2／F24；短按、保持W后停止释放F、重新开始代次1→2、保持1后托盘正常退出并收到2抬起均通过。产品及接收窗exit 0，最终pressed_count=0，无鼠标／系统组合；首次k1-01同范围亦通过。
- 上述实际输入发生在撤回专用行守卫前；三条固定普通行在前后均准入，客户端／发送／排空路径完全相同，故复用该接收证据而不额外占用Default。撤回守卫后的交付Release重新构建。不能由此声称长按／双击／组合已实测通过。
- U11经Qt5 lupdate-pro QKeyMapper.pro原生成功提取，限定合并单项批准词条，旧U08保留vanished历史，lrelease三语及QM反向转换核对通过。Qt6私有Popup最终五场景通过：zh_CN／en_US／ja_JP 100%浅色、zh_CN／ja_JP 150%深色；实际桌面QkmIbInputUi_21400，初始化／发送API计数均为0，正常退出。中文优先检查换行／按钮／反馈；150%已滚动到能力说明并目视检查，无遮挡。日志k1-private-ui-final.log，PNG为test_snapshots/snapshot_ibinput_w11-08_popup_*。
- 构建准备曾出现测试工程$$PWD转义及Windows头顺序错误，均已修正，最终相关构建exit 0；失败原始日志保留，不删除或描述为首次即通过。

**交付与停止点**：独立运行目录out/ibinput/manual-w11-k1-20261030，正式QKeyMapper.exe、合成keymapdata.ini、原依赖及四份许可；不覆盖旧运行目录或个人INI。步骤见manual_verification.md及目录内README-验证.md。普通EXE只供用户按UAC启动，本线程不自动再占用Default。依赖仍为5258b37988f21efe4bdbdb57d7553196bd816350，x64 DLL哈希仍13BDF841A5B15057A939BC1DC5C41FC1848FDE433DCC5B0941248D16FDB9D548。

未执行：用户键盘人工验收、实体键盘触发的本轮专项记录、长按／双击等实际回归、其他键值／按钮／滚轮／全FakerInput对齐、Win10当前切片、全产品Qt5／严格静态分析／实际ASan、手册HTML／README发布说明、发布包及MCP合流。它们不是本切片通过项。本轮为获授权的独立提交切点，正常推送当前功能分支后停止，用户验收与完整功能／合流门槛保留；本地原始日志／运行目录不随Git同步。


### 12.12 R10：人工通过与FakerInput／筛选键验收边界

2026-10-30用户明确“人工验证通过”，对应上一交付源码39126f15ed64ac6dffc2383308387343817cf085及out/ibinput/manual-w11-k1-20261030。随后补充：最基本的双击和长按也验证IbInput可用，没有进行复杂测试。用户截图示例为Q⏲500→F、W✖500→G，另有普通E→H行；记录基本长按／双击可用，不据截图额外推定复杂组合、全部键值、系统筛选键两种状态或所有三语场景均已验收。

用户确认：QKeyMapper已在README和使用手册推荐系统筛选键；IbInput达到现有FakerInput的效果即可。启用筛选键正常、未启用时与FakerInput类似的机制性问题，可由用户判定是否维持现状。已据此加入1.2并更新当前交接卡片／能力表；不把未测或共有机制差异自动升级为额外禁令、补偿工程或合流阻断。

用户随后澄清dwExtraInfo判断来自既有调查推断，允许后续通过LowLevelKeyboardHook原始日志再核实一次，已在1.2记录为待验证假设与最小观察方案。本次仅修订文档并按用户授权amend最后一条未推送的文档提交，不改变人工通过的源码／程序或启动新测试。

本次独立提交切点仅更新主计划及配套文档中的旧待验提示。检查文档范围、差异和UTF-8无BOM／LF；无源码、资源、EXE、DLL、INI、正式README／手册或MCP检出改动，无新构建、GUI、输入、筛选键更改或gsudo操作。创建新的本地commit，不amend已验39126f15、不push；完成即停止。完整功能和后续MCP接纳仍遵守1.2及既定用户确认门槛。
