# MiniDump 实现、构建与验收

本目录实现基于 `9697808c` 的已确认改善计划。崩溃由相邻的原生辅助进程保存现场，疑似卡死默认自动采集。主程序的逐次键鼠、手柄、陀螺仪处理代码没有诊断埋点；普通日志的 `LOGOUT_TOFILE` 开关、同步和滚动策略保持原状。

当前交付的是实现与可执行检查工具。已有用户构建产物完成过静态符号核验，但**本次导入库调用调整仍须重新编译；故障注入、WinDbg 定位、实际输入体验和性能验收尚待完成，不能视为已通过发布验收**。编译由用户执行。

## 构建与部署

Qt Creator 打开原有 `QKeyMapper.pro`，重新运行 qmake 后构建。源码身份在 qmake 阶段生成，切换提交或需要刷新 dirty 标记时也应重新运行 qmake。新增依赖会先构建 `QKeyMapperCrashReporter.exe`，连同 PDB 复制到当前主 EXE 旁。辅助程序始终采用优化 Release、静态 CRT，不链接 Qt 或输入驱动库；使用同一个 qmake 和 MSVC 环境确定架构。Debug 主程序也使用该原生 Release 辅助程序。

在配置好相应架构 MSVC 环境的 PowerShell 中运行现有发布脚本，例如：

```powershell
& 'D:\work\mygithub\QKeyMapper\QKeyMapper\QKeyMapper\build_release.ps1' `
    -QMakePath 'C:\Qt\6.8.3\msvc2022_64\bin\qmake.exe' `
    -MakePath 'C:\Qt\Tools\QtCreator\bin\jom\jom.exe' `
    -Target All
```

保留原有 `-BuildRoot`、`-QMakePath`、`-MakePath` 和 `-Target` 语义。`-Target QKeyMapper` 只构建普通版，`-Target QKeyMapper_diagnostic` 只构建日志诊断版。切换 Qt/架构时使用各自的构建根目录和匹配的 MSVC 环境。脚本依旧清理选中目标的构建目录。

`force_debug_info` 保留。Qt6 的现有优化设置不重复添加。配置检查和用户提供的 Qt5 链接命令确认 Qt 5.15.2 Release 缺少显式 `/OPT:ICF`，因此只补足缺失项；没有关闭优化。

仅辅助程序按目标架构链接仓库 `win_lib/x86/DbgHelp.Lib` 或 `win_lib/x64/DbgHelp.Lib`，保留 `<dbghelp.h>` 并直接调用 `MiniDumpWriteDump`。主程序不链接 DbgHelp。该 `.Lib` 是导入库，仍由 Windows 在辅助程序进入入口函数前按 DLL 搜索规则加载 `dbghelp.dll`，不再固定系统目录绝对路径，也不使用 `LoadLibraryExW`/`GetProcAddress`。部署继续将匹配的辅助 EXE 放在主 EXE 旁，默认使用系统提供的 DLL，不分发 `.Lib`、辅助 PDB 或私有 `dbghelp.dll`；PDB 随构建归档。

发布检查要求两个 EXE 均有匹配 GUID/Age 的完整 PDB，主程序与辅助程序架构相同，编译保留 `/O2`、`/Zi`，链接保留 `/OPT:REF`、`/OPT:ICF`、`/INCREMENTAL:NO`、`/DEBUG`，拒绝 FASTLINK、关闭优化等配置。构建/归档脚本还使用同一 MSVC 环境中的 `link.exe /dump /imports` 检查辅助 EXE 导入 `dbghelp.dll` 和 `MiniDumpWriteDump`，主 EXE 不导入 DbgHelp；此命令只读取现有二进制，不重新链接。

归档位置为 `<BuildRoot>/symbols/<主程序 PDB GUID>-<Age>/<目标名>/`，包含两个 EXE、两个 PDB 及 `manifest.json`。清单记录源码身份（含 dirty 标志）、Qt/qmake、MSVC 探测版本、主/辅编译链接参数、文件 SHA256 和构建目录中已有的 DLL 身份。归档与会被清理的目标目录并列，已有归档不覆盖。部署后追加的 DLL 不会反向更新清单；发布包仍须保留与本次主 EXE 匹配的辅助 EXE。PDB 留在归档中供分析，不要求给终端用户分发。

## 启动、心跳和异常处理

管理员检查通过后、Qt 缩放与 `SingleApplication` 初始化前建立匿名共享内存、事件和受限继承句柄并启动辅助程序。Ready 最多等待 1 秒，失败后继续启动；健康 UI 就绪后一次性提示。诊断状态复用现有非阻塞通知的默认样式，不受映射通知的关闭设置影响，不弹出需要确认的模态窗口。次实例保留原有退出路径，由目标进程句柄驱动辅助程序自行退出。辅助程序无窗口、普通优先级，不继承主程序的高优先级策略。

创建辅助进程前临时在现有进程错误模式上增加 `SEM_FAILCRITICALERRORS`，让子进程在 DLL 加载阶段就继承该模式，避免加载失败的系统错误框。`CreateProcessW` 返回后立即恢复主程序原错误模式，再处理创建结果；不将此设置保留到 Qt/业务线程运行期。创建失败保存原始 Win32 错误，辅助程序在 Ready 前退出则保存原始退出码（可能为 NTSTATUS）；仍继续主程序启动并提示诊断不可用。

每个实际线程上的 2 秒 `Qt::CoarseTimer` 仅更新自己的对齐计数器。Interception 定时器在现有 `started` 槽前初始化，复用其已有 `processEvents()`，不修改驱动等待时间。辅助程序每 2 秒比较计数器；启动进度 120 秒、运行心跳默认 30 秒、退出进度 60 秒未前进才触发疑似卡死采集。

启动时一次性读取现有 `keymapdata.ini` 的根级键。通过 QSettings 表示时位于 `[General]`，不要新建名为 CrashDiagnostics 的映射配置分组：

```ini
[General]
CrashDiagnosticsHangEnabled=true
CrashDiagnosticsHangTimeoutSeconds=30
```

默认启用；`false` 或 `0` 关闭。阈值有效范围 10～600 秒，非法值恢复 30 秒。修改后重启生效；关闭后不创建四个心跳定时器，崩溃采集继续保留。辅助程序死亡或采集超时后，现有心跳定时器在下一次触发时停止。

计时使用 `QueryUnbiasedInterruptTime`，不计入系统休眠；与 `GetTickCount64` 的差值用于识别恢复并重给宽限期。调试器附加或状态无法确认时暂停自动卡死判定。一次卡死只尝试一次，必须实际看到各预期线程持续恢复心跳 60 秒才重新布防，采集间隔至少 10 分钟，每次运行最多尝试 3 次。崩溃独立于这些限制。

异常处理器原子占有首个现场，复制固定大小顶层异常记录和 `CONTEXT`，截断嵌套链并标记，通知辅助程序并有界等待；不调用 Qt、文件 API、日志、CRT 格式化、符号解析或映射清理。其他异常线程等待同一结果；同线程重入快速放弃。最多等待 30 秒后返回 `EXCEPTION_CONTINUE_SEARCH`。

协议核对版本、结构大小、架构、PID、创建时间和创建者关系。辅助程序只接受创建它的父进程传来的句柄，不接受外部 PID 或输出路径。辅助程序验证异常线程归属后，以本地异常记录和上下文副本重建指针，固定 `ClientPointers=FALSE`。

## 转储、失败与文件边界

默认标志为 `MiniDumpNormal | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules`。只有 DbgHelp 明确返回 `E_INVALIDARG` 时才允许一次降级到 `MiniDumpNormal`，同时记录请求/实际标志与首次错误。卡死不伪造异常流。一个监督线程协调一个串行写入线程，所有 DbgHelp 调用只发生在写入线程上。

卡死采集中发生崩溃时，请求协作取消后优先处理首个崩溃。采集超过 15 秒请求取消、发布超时并关闭后续卡死采集；回调不保证能打断所有底层调用。对仍存活的卡死目标，不强杀主程序，也不强杀正在采集它的辅助程序；目标死亡或明确进入崩溃终止流程后才允许辅助程序放弃未返回的写入。正常退出最多等待辅助程序 2 秒。没有进程内 Dump 回退、自动重启、自动结束或自动恢复映射。

**每次实际采集时**选择目录：已有 `log/` 目录则直接使用；确认不存在则创建；创建失败后再次检查，若已被其他实例建好则仍用 `log/`；只有确认仍不存在才尝试主程序目录。原目录的文件创建、写入、完成失败，不触发换目录。下次采集重新优先 `log/`，不持久化兜底选择。

使用 Unicode 绝对路径，首版明确限制完整诊断文件路径短于 `MAX_PATH`。路径截断、无法确认目录、目录重定向/重解析点或规范化路径不符均明确失败，不通过兜底掩盖。目录句柄在采集/清理期间保持打开，避免期间被改名替换。诊断路径策略不影响普通日志。

文件名：`QKM_Diag_v1_<UTC日期>_<时间>_<毫秒>_<PID>_<16位创建时间>_<序号>_<Crash或Hang>`。通过 `CREATE_NEW` 防覆盖，先写 `.partial`，成功转储、刷新、关闭并重命名后才将 `.dmp` 标为成功。`.txt` 是 UTF-8 伴随记录，三种扩展名使用同一选定目录。失败文件保留，不能因有文件或 `.partial` 非空就判断采集成功。

伴随记录与共享结果包含实际路径、兜底标志、`log/` 创建错误、最终错误和元数据错误。`failure_stage` 数值按 `crash_protocol.h` 的 `FailureStage` 枚举区分目录、路径、文件创建、异常验证、转储、刷新、关闭、重命名、元数据、取消及辅助内部错误。元数据还含构建身份、线程/阶段/阈值、异常信息、标志与耗时。元数据无法落盘时通过共享结果报告，不能承诺在两个目录都不可写时仍有磁盘记录。

正常启动进入 Running 后清理一次历史文件：合并主目录与 `log/`，按文件名 UTC 时间保留最近 10 个已确认组。仅处理严格命名且伴随记录具有一致归属标记的组，并在全部文件能独占取得删除权限后才删除。不递归、不跟随重解析点，不处理旧版 Dump、普通日志、EXE、PDB、INI 或其他文件。正在写入和无法确认归属的残留保留人工检查，因此这些组不受自动清理数量承诺约束；本次启动之后新生成的组在下一次正常启动清理。

诊断文件默认仅本地保存，无上传。Dump 和伴随记录可能包含路径、线程栈和内存中的敏感数据，分享前由用户确认内容。

## 隔离检查与符号定位

下列脚本由用户显式运行。默认只编译原生测试程序/辅助程序并执行心跳策略自检、正确和错误 EXE/PDB 配对检查及导入表检查，不启动正式 QKeyMapper，也不调用输入驱动：

```powershell
& 'D:\work\mygithub\QKeyMapper\QKeyMapper\QKeyMapper\diagnostics\tests\run_checks.ps1' `
    -QMakePath 'C:\Qt\6.8.3\msvc2022_64\bin\qmake.exe' `
    -MakePath 'C:\Qt\Tools\QtCreator\bin\jom\jom.exe'
```

增加 `-Runtime` 才进行故障注入：在唯一的测试目录复制测试 EXE/辅助程序，检查启动、模拟 UI/工作线程崩溃、同时异常、四种线程停滞、辅助死亡与目标直接退出。也检查目录创建失败兜底、权限恢复、已有 log 不可写、两处都不可写，以及两目录合并保留规则。目录 ACL 仅在该测试目录临时改变，并在 finally 中恢复。超时回收仅针对显式启动的测试实例及其独有目录中的辅助进程。全部测试产物保留，脚本不递归删除。完整运行约需数分钟；增加 `-LongLifecycleTests` 覆盖真实 120/60 秒启动/退出阈值。CIM/PowerShell 命令要求测试机具备相应组件（建议 Windows PowerShell 5.1）；Win7 上原生目标兼容性与测试脚本环境分别验证。

其中 `missing-import` 用例只将全新隔离辅助 EXE 副本的 `dbghelp.dll` 导入名称替换为等长随机 DLL 名称，保留 PE 布局，不改构建产物或系统 DLL。显式启动的原生测试程序验证启动结果为 `STATUS_DLL_NOT_FOUND` 或 `ERROR_MOD_NOT_FOUND`，原错误模式已恢复、诊断已停用且目标继续执行；正常成功启动的用例也检查错误模式恢复。运行时仍需观察没有系统错误框，并在真实 Qt 程序中验证健康 UI 仅提示一次。修改后的辅助副本仅用于加载失败测试，不能部署或用其 PDB 做正常符号验证。

测试程序通过原生计数器模拟四个槽，不等价于验证实际 Qt 事件循环。脚本检查 Dump 结构、异常流是否符合类型、元数据与符号配对；**仍须使用匹配 EXE/PDB 定位预期源码、线程和调用栈**，不能仅以 Dump 能打开判定合格。例如在 WinDbg 中打开故障文件，设置本次归档目录后：

```text
.sympath+ D:\path\to\this-build-symbols
.reload /f
lmv m QKeyMapperDiagnosticsTest
!analyze -v
.ecxr
k
```

预期原生异常码为 `0xE0424B4D`，栈应能定位 `crashHere` 和预期测试线程；真实程序测试须定位真正注入点。疑似卡死文件使用 `~*kb`、`!analyze -hang`，结合伴随记录的线程 ID、stalled_mask、阶段及最后心跳分析，不使用不存在的异常上下文。Windows/Qt/第三方模块需要各自匹配符号才能继续深入其内部。错误 PDB 必须被校验脚本拒绝。

仍需专项人工验收：实际 UI、Hook、Worker、Interception 分别阻塞及恢复；主窗口构造崩溃；卡死写入尚未返回时发生崩溃；处理器自身重入；辅助缺失/错误架构；实际转储超过 15 秒及不响应取消；磁盘不足；创建期间其他实例建好 log；不支持路径与目录重定向。故障只在隔离复制目录及明确启动的测试实例中注入，不能向正在使用的主程序随机注入异常。

## 性能与兼容发布门槛

相同工具链/优化/符号配置下，对比 `9697808c` 基线、改善版关闭心跳、改善版默认开启心跳；每种场景至少三轮。普通版应与普通版比较，诊断日志版另作对照，不混淆日志 I/O 与本次诊断开销。

可对已经显式启动的目标进程进行只读 CPU/内存采样（PID 用该轮实际值替换）：

```powershell
& 'D:\work\mygithub\QKeyMapper\QKeyMapper\QKeyMapper\diagnostics\tests\measure_overhead.ps1' `
    -MainProcessId 1234 -Seconds 120 -Label 'default-idle-round1' |
    Export-Csv -LiteralPath '.\default-idle-round1.csv' -NoTypeInformation
```

此脚本只计主程序和已确认的相邻子辅助程序，不包含采样脚本自身；输出的是两进程 CPU/私有内存总量，**开销须由同场景各轮差值计算**，CPU 以单核 100% 为基准。采样结果不能代替输入延迟或磁盘写入验证。

| 验收项 | 方法及合格条件 | 当前状态 |
|---|---|---|
| 空闲、托盘、无输入、停止映射、普通对话框 | 长时间观察，无误采集 | 待用户运行 |
| 休眠唤醒、调试暂停 | 超过阈值后恢复，重新给予宽限，不立即误采集 | 待用户运行 |
| 正常磁盘与输入路径 | 用进程 I/O 跟踪区分启动清理与稳态；稳态无诊断周期写入；逐输入无新增分配、锁或系统调用 | 源码确认未改输入处理，运行待验证 |
| 常驻资源 | 三轮以上同场景差值；新增私有内存合计 ≤16 MiB，平均 CPU 增量 ≤单核 0.1% | 预算，尚未实测 |
| 输入延迟和行为 | 用同一外部输入/时间测量方法覆盖持续键鼠、连发/序列、手柄/陀螺仪；比较 p95/p99 与基线轮次波动 | 待用户实测；不在正式逐输入路径新增测量代码 |
| 采集及取消后行为 | 记录实际采集耗时，解除可恢复阻塞后检查仍可运行的输入线程；无新增丢键、粘键、热键失效、Hook 失效、遗留挂起 | 待用户实测 |
| Qt 6.8.3 / Win10、Win11 x64 | 完整编译、故障、符号、功能、性能验证 | qmake 配置已检查，其余待验证 |
| Qt 5.15.2 / Win7 x86、x64 | 匹配架构辅助程序及系统 DbgHelp；保持仓库 Qt5 API 边界 | 仅 Qt5 x64 qmake 配置已检查，其余待验证 |

原方案曾完成 Qt5/Qt6 qmake 配置、PowerShell 语法、既有样本符号身份和归档流程检查。本次导入库调整另行检查辅助目标链接配置、脚本语法、既有样本的导入解析/检查与副本改名流程，以及源码差异/编码/空白；这些文件检查不运行样本。尚未为本次调整运行编译器、正式或隔离目标，也没有故障或性能实测。旧的显式加载辅助 EXE 不满足新增导入检查，需要重新构建后再执行完整验收。发布前必须解决验收中发现的体验回退。

用户态转储提供原因分析线索，不承诺定位所有死锁，也不覆盖断电、内核死锁、强制结束、处理器安装前故障或所有 fail-fast/栈损坏情况。

## 新增提示的翻译建议

没有修改 `.ts` 文件。以下提示由 `QkmCrashMonitor` 翻译上下文发出：

| 语言 | 文本 |
|---|---|
| English | Crash diagnostics unavailable (error %1). |
| 中文 | 异常诊断不可用（错误 %1）。 |
| 日本語 | 異常診断を利用できません（エラー %1）。 |

| 语言 | 文本 |
|---|---|
| English | Diagnostic file cleanup failed (error %1). |
| 中文 | 诊断文件清理失败（错误 %1）。 |
| 日本語 | 診断ファイルの削除に失敗しました（エラー %1）。 |

| 语言 | 文本 |
|---|---|
| English | Diagnostic dump saved: %1 |
| 中文 | 诊断转储已保存：%1 |
| 日本語 | 診断ダンプを保存しました：%1 |

| 语言 | 文本 |
|---|---|
| English | Diagnostic metadata failed (error %1). |
| 中文 | 诊断元数据保存失败（错误 %1）。 |
| 日本語 | 診断メタデータの保存に失敗しました（エラー %1）。 |

| 语言 | 文本 |
|---|---|
| English | Diagnostic dump failed (error %1, directory error %2). |
| 中文 | 诊断转储失败（错误 %1，目录错误 %2）。 |
| 日本語 | 診断ダンプに失敗しました（エラー %1、ディレクトリエラー %2）。 |
