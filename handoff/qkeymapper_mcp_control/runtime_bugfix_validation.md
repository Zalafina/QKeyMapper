# 两项运行时缺陷：独立审阅与选定验证事实

[唯一主计划](qkeymapper_mcp_automation_plan_codex.md) · [能力矩阵](capability_matrix.md)

本页只记录事实，阶段／决策／下一步见唯一主计划。当前基线5a80dd0649668ed59fd9f22591433954002264dd，PC-W11、2026-11-16。外部材料只读，不把Gemini结果当当前分支新验收；原件仅本机out/runtime_bugfix_149_20261116。

## 原始材料与符号边界

独立读取PE CodeView、PDB MSF信息流及五份完整minidump模块记录：EXE与五dump均GUID748063ee-0ba8-453b-a812-c9d36073121f、Age57，所给PDB为8d1999f1-e341-4daf-81d4-4a2b13bc3721、Age4，均0xc0000005；另五partial为0字节，不能独立确认同一生产栈。用户说明PDB来自v1.3.8.20260926 tag重建，原PDB丢失；允许辅助符号／反汇编尝试，不阻塞源码与当前分支复现，不称精确生产行号。补丁SHA256见本机reference-patches.json；01独立git apply --check通过，01→02内存逐段61／54段全匹配，不据此宣称逻辑安全。

## 本轮执行

### BUG01-149：实键／Timed列表叶锁

源码基线5a80dd06，四产品文件采用已独立审阅的01预适配补丁；两非递归叶锁分别保护实键双列表与长按／双击列表。生产读写均经锁内容器操作／值快照，UI两录键读者用快照；信号、计时器及发送在叶锁外。保留两处STOPPED且输入未关闭的原冻结条件及多设备去索引的既有首次UP语义，不引入引用计数。维护测试中的直接写入仅作队列开始前或排空后的合成状态布置，不是生产入口或并发写入证明。

外部复现临时复制到out/repro_pressedrealkeys_bug149，实际编译当前Worker。发现realkeys_full会从原键恢复直达SendInput，故本机副本先安装仅自身映像的IAT不转发截获，缺失即退出；F24 DOWN/UP正对照计数2后才进入模式，截获保留到进程结束，无真实输入。各模式依次realkeys_only／realkeys_full／longpress_race／doublepress_race／timedkeys_full，每项5秒，父级25秒、内部12秒看门狗。

| 构建／执行 | 修前实际结果 | 修后实际结果 |
|---|---|---|
| Qt6.8.3 x64 Release | 0xc0000374／0xc0000005／0xc0000005／0xc0000409／0xc0000005 | 五项native0／PASS，无超时或控制器终止 |
| Qt5.15.2 x64 Release | 0xc0000409／0xc0000005／0xc0000374／0xc0000005／0xc0000005 | 五项native0／PASS，无超时或控制器终止 |
| Qt6实际MSVC ASan | double-free／heap-buffer-overflow／negative-size-param／negative-size-param／heap-use-after-free，native3／1／3／3／3 | 五项native0／PASS，每进程观测到clang_rt.asan_dynamic-x86_64.dll，无ASan报告 |

复现均普通Medium、asInvoker、QCoreApplication／offscreen，无产品GUI或驱动验收含义。修前ASan的exit3来自实际报告而非看门狗；启动早期模块查询的真实299原样保留，不推算为其他错误。实际EXE为out/runtime_bugfix_149_20261116/build-{baseline01,fixed01}-{qt6,qt5,asan}/release/QKeyMapperReproPressedRealKeys.exe；各EXE身份／PID／native及日志在run-*对应summary.json。

永久回归放在既有sendinput_lifecycle目标的pressed_state149：两个真实生产者各1000次DOWN/UP并发成对快照，锁外回调重入，主窗／Item录键组件读取，以及Timed DOWN/UP释放。Qt6／Qt5各七断言通过，成对快照19528／10610次，实键／Timed回调均4／4；另各idle_input_tracking、closed_input_tracking、hold_source_context、unlock_source_context通过。十个自有High原manifest进程在独立私有桌面通过公共登记入口运行，native0，尾部work/producers0、INI字节不变、FilterKeys不变、前后前台PID均非本实例；离散前后台采样不等于连续无干扰证明。EXE为本轮{qt6,qt5}-runtime-149-b04/QKeyMapperPressedState149_{qt6,qt5}_runtime.exe，部署身份见deployment-*.json，原件在runs/*-r02。录键验证不是独立GUI键入基线。

失败轮次保留：临时截获helper的slots触发Qt宏编译失败，改名后b02构建通过；fixed01三构建首轮C1041写PDB失败，普通权限将该本机JOM并发降为1、增量b02通过，未终止共享编译服务。永久夹具b01错误假定长按map值类型且触发已弃用API警告，修正类型／API；r01又因排队信号未推进及长按／双击样本语法不符导致native2（三断言失败），修夹具后b04及r02通过，产品补丁未为迎合测试改变。Qt6维护构建native0零警告；Qt5 native0，仅官方qtmain缺PDB LNK4099精确例外。

本轮实际使用x86 CDB读取五份x64完整dump：显式本地PDB并标注忽略不匹配，native均0、无超时，辅助帧含updatePressedRealKeysList和VCRUNTIME拷贝路径。保留系统模块校验／时间戳警告及缺原VCRUNTIME映像的缺口；仍不是精确生产符号。日志dump-01至05-rebuilt-pdb-aux.log及dump-aux-summary.json，不运行原EXE、不改源资料。

01提交时是独立本地提交切点，其后段普通Release／分析、Bug02、MCP及Default结果见下节，不改写01当时验收范围。149时保留普通控制器13752供02继续；最终关闭事实另列，不接管历史未知请求。


### BUG02-150：选定完成，收尾停止

基线为BUG01提交ec2daf695fb7eeb3dc65625e77b7030eb8cc35fc；02为本提交，分支codex/qkeymapper-mcp-control，以实际Git HEAD定位。本轮不切／合master、不push、不amend。配置接口仍为17个普通工具／23项业务能力，没有增加MCP运行入口或权限。

#### 采纳与适配

虚拟按键生产访问统一经叶锁／值快照，释放反向遍历本地副本。SendInputWorkItem携带原序列身份；控制器区分最新接受与实际运行的序列，根、展开步骤及重复请求传递同一身份，重复发射／消费均先拒绝旧轮次。首次根在排队前经共用创建入口登记控制器，避免带身份的新请求被空控制器拒绝。controller值复制仍依赖原池排空后的销毁顺序，不是存续租约。

按成功发送记录当前步DOWN及尚待UP，补偿Hook回声延迟；SendMessage、滚轮、BOTH／排除及不对应普通保持的固定覆盖分支不纳入这项补偿。成功UP从待释放记录移除，取消前未发DOWN不补UP，晚到取消UP不重放；其他源行的组合借键和普通SendInput物理原键保护保留。IbInput按原源行／运行代次逆序释放，不与虚拟列表求交；其dwExtraInfo=0的实键回声也不能作为释放所有权判断，其他行借用仍保护。FakerInput仍沿既有报告释放和来源识别边界，本轮未做真实FakerInput专项。

清理尾任务纳入SendInputWorkItem，投递时固定原控制器、源行／源副本和运行／IbInput代次，执行前核实身份；丢弃只归还工作计数，不声称释放完成。暂停唤醒、清队列、根UP、步内打断、重触发和重复边界统一走该上下文，只消费匹配的中断标志；HoldDown不能绕过取消检查。清理／实际池排空和原有IbInput释放屏障继续决定可写及可重启，不以STOPPED或标志归零代替。

#### 复现及失败轮次

修前当前01源码运行外部02：Qt6 Case1/2/3为native1/1/1，300轮native0（该轮未复现）；Qt5为0/1/0xc0000005，压力186轮失败；实际ASan为1/1/3、压力失败。Case3 ASan包含外部局部源表遭晚到任务访问的夹具生命周期缺口，不能当作精确生产崩溃栈。临时副本三处旧七参调用先导致编译失败，适配当前IbInput代次后b03构建通过，不据此改产品入口。

修后r01的手工Case3缺序列身份，补夹具契约；r02三套Case1/2/3通过，但Qt6／Qt5压力在22／103轮抓到DOWN1、UP2，ASan该轮300通过；r03三套第1轮又抓到未发DOWN却补UP。r04／r05中间通过仅保留当时范围。最终b09、r06三套Case1、2、3a–3d及各300轮全部native0／ALL_PASSED，无超时或控制器终止；ASan进程均观测到实际运行时。外部工具自有映像SendInput不转发截获及F24 DOWN/UP正对照先成功才执行，无真实系统输入。build-fixed02-*与run-fixed02-*-r06保留实际EXE身份、原生码及日志；Bug01最终ASan b05／final-r03五模式也全过。

repeat r01先过26项，restart夹具丢弃了实际运行代次／work，且未完成冻结运行就编辑下一控制源；修夹具后r02首项真实抓到首次根身份未登记导致零执行。共用创建路径修复后r03的repeatSpecs()全28项通过，观察器保留正式work并检查首次根身份。静态首轮七项Worker诊断（新裸指针计数及死存储）经控制器原类型值保存、去掉无用途赋值修正，未新增抑制。

sequence_release150 r01在STOPPED状态直接执行发送任务，旧allMappingKeysReleased排队回调清理控制器而使夹具崩溃，无case.json。实际dump记录0xc0000005；当时本机PDB辅助定位至取消UP，但未取得该目标进程原生退出码，不能拿gsudo启动exit0或dump异常码替代。夹具改为无Hook的运行态标记，源表销毁前排空控制器；b07／r02通过，产品未为迎合夹具放宽。观察器现于解析结果前先保存已取得的原生退出码。失败EXE／dump／CDB日志留final3/log及sequence-release-r01-*。

普通产品MCP-01因target_not_registered在启动前拒绝；登记精确GUI动作后使用新轮次MCP-02通过，不重放旧请求。Control首轮缺QKM_IPC_TEST_EXE，native1；转既有私有运行入口并传匹配IPC后r02通过。Python首轮缺本地PYTHONPATH导致导入失败，限定本次进程路径后通过，未改venv或依赖锁。其余首轮编译／夹具错误保留本机原日志，不删除或重写成通过。

#### 最终选定门槛

| 范围 | 本轮实际结果及原件 |
|---|---|
| 普通Release构建 | Qt6.8.3 x64、Qt5.15.2 x64，MSVC BuildTools14.44.35207；build-ordinary-*-b05成功，Qt6零编译／链接警告，Qt5仅官方qtmain缺PDB的精确LNK4099例外；原manifest保持。两个普通EXE为out/build_qt6/release/QKeyMapper.exe及out/build_qt5_5152/release/QKeyMapper.exe |
| 定向Clang-Tidy／Clazy | Main与Item用本轮未受后续语义改动影响的两个实际上下文零诊断；Worker最终static-worker-first-owner两个上下文native0、项目诊断0／工具非零0。首轮static-final失败保留，不说每次分析都通过 |
| Worker运行态／重复 | runs/qt6-runtime-*-r03全47项，qt6-repeat-*-r03全28项；含break、pause、queue、borrowers、Stop／restart／exit、HoldDown、关闭输入、GUI停止、源修订及新身份／丢弃回归，native0、work/producers0 |
| 实际Qt6 ASan | 两外部复现及runs/asan-asan-*-r01的26项排空、取消、上下文、GUI停止和重复用例native0；各进程明确asan_runtime_loaded=true，部署实际clang_rt.asan_dynamic-x86_64.dll，无ASan报告；不是环境启动失败或真实厂商I/O故障验收 |
| Qt5必要兼容 | runs/qt5-compat-*-r01七项全过，含新sequence_context150、149、源上下文、虚拟释放与重复；非Qt5 Default完整运行验收 |
| 确定性释放 | output-runtime-150-final4／runs/output-release-sequence_release150-r02 native0；七项布尔均真：借键／物理保护、自有释放、最后借用者、延迟UP不重复、未发DOWN不补UP、最后借用上下文释放、晚到取消UP拒绝。原生边界为编译期替身 |
| Forza | 最终forza150 b03／*-r02，状态31910断言及实际Worker生命周期141断言，native0／真实私有桌面；ViGEm报告仍为替身，启动枚举CreateFile错误5原样保留，不是游戏或实体硬件验收 |
| Python及MCP组件 | protocol22、tool_results8、demo9项通过；Control r02为71项、SDK interop12项通过，普通权限实际私有Qt组件／正式stdio认证；原生／脚本exit0，无60秒approval-timeout新增长测 |
| 普通产品正式MCP | out/mcp_product/runtimebug150_20261116/runtime/QKeyMapperMcpAcceptance_runtimebug150_qt6.exe，SHA256 c7bdef215d246d50c6c4c899dda1e197d011a7226b0cf7faa6478fc7f67f4988；mapping-02三产品正常退出，51次SDK调用，normal／cold／read通过，预览／取消／提交、旧引用与预览、保存及独立冷读、Read拒绝。GUI数据基线复用out/mcp_edit_gui_20261110/runs/qt6-normal-02原范围；本轮不是新独立GUI全等价验收 |
| 运行及清理期正式写拒绝 | formal-cleanup150两场cleanup_stop／cleanup_timeout，真实私有GUI启停与SDK共45次调用、Native各11检查；运行时context_busy，真实生产者队列仍占用且Worker已STOPPED时继续拒绝源／预览／提交，释放后实际完成清理才保存。尾部work/producers0、产品native0；30ms超时为明确夹具注入，不是厂商真实故障 |
| 只配置演示 | formal-demo150，普通策略正对照11场确认计数接线；正式只配置入口25检查／27次SDK调用、两场真实配置和图片／回收通过，10场逐项及退出IbSendInit／IbSendInput／IbSendDestroy／SendInput／SetCursorPos各0。中文高级页及C→D单项PNG自主查看。其他资源仍检查未初始化，不把五项计数扩大成任意API全局监控；未向QQ发送 |

维护EXE身份与部署分别在deployment-{qt6,qt5,asan}.json、deployment-output-b07.json；最终非Probe为{qt6,qt5,asan}-runtime-150-final3，Probe为output-runtime-150-final4。随包普通桥接复用本机out/mcp_disabled145_20261110/bridge-b01/runtime/python.exe，九个桥接源文件与当前仓库逐字节相同；没有复制其他PC产物或凭据。保存、组件、替身和实际输入范围独立，不相加宣称普通Release全运行或完整MCP阶段通过。

#### Default双后端代表范围

启动前发现用户自有Release PID1468／其报告器4380，路径exe_release/QKeyMapper_Qt6_x64，11-11起运行。本任务只读核实；用户自行正常退出并回复后才执行，不接管个人INI或实例。公共控制器当时无在途／拥有／未决资源，两后端串行。原计划预留前台20–30分钟；实际成功轮驱动分别8.328／8.422秒（含启动、截图、回收），前置等待／定位排错另计，不把短测说成长期压力。

| 后端 | 实际本机结果 |
|---|---|
| G HUB | default-sequence150-logitech_ghub-r01：维护产品PID37540、普通接收器38108，实际Default，backend=logitech_ghub且无回退；7场通过，代次1→2，产品／接收器native0 |
| Razer | 首轮r01已连接razer，观察器读到接收器刷新中的暂时空JSON，未完成第三场即停止后续输入并正常清理：产品25420／接收器37352 native0，接收器键／按钮归零。只修普通观察器有界读取重试并在读取后复核前台，不重试原生动作；r02新轮产品29504／接收器29864，7场通过，代次1→2、真实Razer无回退，native0 |

固定Mouse-L→Num1+Num2⏱400»Num0⏱25、UP BLOCKED、方式3／按键循环／透传，覆盖400ms等待内松开、观察第二步DOWN后松开、仍持原键时显式break、10ms快速重触发、持有停止并实际排空、重启新代次、持有退出。普通控制器只向本轮前台接收窗注入固定原Mouse-L；每次DOWN前及轮询检查实际HWND/PID/创建时间、Default、修饰键／其他鼠标按钮和指针在接收窗内。该轮未触发失焦中断；不能声称新增实测失焦故障矩阵。步界以实际接收事件为依据，不声称亚毫秒调度保证。

NumLock保持0，实际观察为End/Down/Insert（VK35/40/45，scan0x4f/0x50/0x52），两成功轮各DOWN 8/8/1、UP 8/8/1，最终held keys及mouse buttons0。原Mouse-L透传／恢复语义保留，接收记录Mouse DOWN12／UP8而最终buttons0，不把它当成原鼠标事件严格一一对应；重复UP原生次数由确定性截获回归补证，接收器状态会去重。stop记录work/producers0、pools_finished=true、connected=false、release_incomplete=false；实际main返回后的closed再核计数0／controllers_empty，随后核进程退出，不拿STOPPED替代。

两轮指针速度10、鼠标参数6/10/1、FilterKeys flags58且时间参数全0、NumLock0均前后相同；未安装／卸载驱动或修改系统参数。G HUB指针位置[1846,736]→[1838,724]，Razer[1654,854]不变；位置小幅变化不作来源归因，脚本仅在接收窗内必要定位且满足条件才恢复，不强制覆盖用户移动。仓库capture_ui_snapshots.ps1的PrintWindow／NoActivate生成两张接收器PNG，已自主查看为实际正确窗口且最终保持0。此处是正式维护目标的真实main／Hook／Worker／厂商后端路径，普通Release本轮只验上表配置／MCP，未新增普通产品全运行或游戏结论。

#### 清理、提交与未验

本任务产品与桥接均按各自原生结束／SDK关闭记录核对，专用普通产品配对经GUI撤销、接口关闭、客户端凭据删除；三维护正式场景另只读确认各自CredentialStore为空，不枚举无关Windows凭据。维护夹具私有材料按原始证据留out，不冒称已销毁所有服务端历史密钥。两份外部临时CPP／PRO副本在核实绝对路径为out直接子目录、内容仅预期文件且无reparse后删除；构建、失败dump、辅助PDB原件、日志及所有其他缓存／环境保留。

16:54:45对13752／instance dabbebabad784963b85fa4c860b8cc4e执行off，事前16项登记均核实属本任务、无其他共享scope或活动资源；off脚本exit0，closed／cache_available=false、native_sessions_observed=0、owned/unresolved/pending0。独立status脚本exit0、controller=null／native_sessions_observed0。缓存关闭与目标清理分别证明；历史未知请求b588469d172e4e4bbff2b4fc1797792a保留、不重放，不因本次零资源改写历史未知。用户恢复办公后的全局按键采样不判作本任务保持，也不发送额外全局UP。

完成必要差异／编码／相对链接和文档大小检查后，BUG02-150形成第二个聚焦本地提交。本轮停止，等待用户明确安排，不自动恢复MCP。原始证据只在本机上述out；Win10实机、Qt5 x86、游戏／实体手柄全范围、真实FakerInput专项、厂商真实I/O故障注入和精确生产PDB仍未覆盖；这些不因当前选定通过补造。
