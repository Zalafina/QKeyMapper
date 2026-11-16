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

01是独立本地提交切点；全范围普通Release／分析、Bug02、MCP与Default双后端仍待本轮后段，不用本表提前宣称通过。缓存13752／instance dabbebabad784963b85fa4c860b8cc4e至11-16 19:00，十维护结束各次owned/unresolved/pending均0；保留窗口供已批准的02继续，不接管历史未知请求。
