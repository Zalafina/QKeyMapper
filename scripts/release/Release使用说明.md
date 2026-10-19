# QKeyMapper 一键本地 Release

## 长期保存与运行位置

四个长期维护文件为 `build_release.bat`、`build_release.ps1`、`test_build_release.ps1` 和本说明：

| 位置 | 用途 |
|---|---|
| 仓库 `scripts\release` | 脚本源码的长期保存位置，纳入 Git 版本管理；同目录 `.gitignore` 排除临时文件，`.gitattributes` 固定 BAT 使用 CRLF 换行 |
| `F:\work\code\mygit_hub\release\temp\release_packages` | 日常运行位置，四个文件放在这里，与三个配套目录并列；双击这里的 BAT |
| 各仓库的 `out`，以及旧 `out\release-script-development` | 可重新生成的构建/开发临时目录，不作为脚本长期保存位置；清理后不影响正式脚本的保存 |

仓库中的副本用于维护和备份；其目录没有打包所需的三个配套目录，不能直接作为正式 Release 运行目录。修改仓库副本并通过自检后，先备份运行目录中的四个旧文件，再将上述四个文件同步到运行位置并逐文件核对。同步本身不要求先推送，但正式运行前仍需提交并推送源码、清理未提交改动；同步脚本不会绕过仓库状态检查。示例：在仓库根目录执行：

```powershell
$releaseRuntime = 'F:\work\code\mygit_hub\release\temp\release_packages'
$scriptBackup = Join-Path $releaseRuntime ('script_backups/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $scriptBackup -ErrorAction Stop | Out-Null
foreach ($name in @('build_release.bat', 'build_release.ps1', 'test_build_release.ps1', 'Release使用说明.md')) {
    $sourceScript = Join-Path '.\scripts\release' $name
    $runtimeScript = Join-Path $releaseRuntime $name
    Copy-Item -LiteralPath $runtimeScript -Destination (Join-Path $scriptBackup $name) -ErrorAction Stop
    Copy-Item -LiteralPath $sourceScript -Destination $runtimeScript -Force -ErrorAction Stop
    if ((Get-FileHash -LiteralPath $sourceScript).Hash -ne (Get-FileHash -LiteralPath $runtimeScript).Hash) { throw "脚本同步不一致：$name" }
}
```

运行目录里还应长期保留三个 `QKeyMapper_Qt*` 配套目录；它们提供 Qt DLL、驱动、模板和其他资源，不是可任意删除的构建缓存。迁移电脑时，需要同时迁移这些目录，并核对脚本中的仓库和工具链路径；仅复制四个脚本或拉取 Git 不能替代这些配套文件。

正式 ZIP 保存在 `latest_release`，最新配套 EXE/PDB 保存在三个 `Qt*_dump_analyze` 目录。若需要保留历史版本供以后下载或分析崩溃，在下一次 Release 前将相应 ZIP 和匹配的 EXE/PDB 一起按版本归档；这两类固定目录会被后续成功发布更新。

本次验收报告和原始日志归档在运行目录的 `release_records\v1.5.0.20261016`。该目录独立于会轮换的 `_work\logs`，不是脚本运行依赖。

可以在没有 Release 任务运行、且不需要失败恢复时清理运行目录的 `_test_work`、`_work\staging`、`_work\pending`；`_work\logs` 按需保留，`_work\previous_release` 是上一套恢复备份，确认不再需要后再清理。`update_qt5_vendor_policy.ps1` 是本次补齐规则用的一次性开发辅助文件，不属于 Release 脚本长期维护文件。

## 日常使用

1. 提交需要发布的源码并推送到 `origin/master`，包括源码中正确的版本号。
2. 双击本目录的 `build_release.bat`。
3. 等待窗口显示 **Release 成功**，再使用 `latest_release` 中的三个 ZIP。

成功、条件不满足、执行失败都会显示结果并等待按键，窗口不会在脚本结束时自动关闭。也可以直接点击窗口关闭按钮。运行过程中不要关闭窗口；不要同时用 QtCreator 构建脚本专用目录。

可选命令：

```powershell
# 仅检查本地环境，不联网、不拉取、不写日志、不构建。
.\build_release.bat -CheckOnly

# 设置每套构建的并行编译任务数，默认 4；三套配置始终串行执行。
.\build_release.bat -Jobs 4

# 在已有终端中直接运行，不附加关闭前等待，退出码 0=成功，1=失败。
pwsh -NoProfile -File .\build_release.ps1
```

需要 PowerShell 7.2 或更新版本、Git，以及本机已有的 Qt/MSVC/JOM。整个流程使用普通用户权限，无须以管理员身份运行。

## 自动完成的操作

检查三个仓库：

| 配置 | 仓库 | 工具链 |
|---|---|---|
| Qt6_x64 | `F:\work\code\mygit_hub\QKeyMapper` | Qt 6.8.3 / MSVC2022 BuildTools x64 |
| Qt5_x64 | `F:\work\code\mygit_hub\QKeyMapper_Qt5` | Qt 5.15.2 / MSVC2019 BuildTools x64 |
| Qt5_x86 | `F:\work\code\mygit_hub\QKeyMapper_Qt5_32bit\QKeyMapper_Qt5_x86` | Qt 5.15.2 / MSVC2019 BuildTools x86 |

工具链从 `C:\Qt\Qt6` 和 `C:\Program Files (x86)\Microsoft Visual Studio` 中固定选择，与当前 QtCreator 配置对应，不使用 PATH 中任意版本的 qmake 或编译器。

- 全部仓库必须干净、位于 `master`、使用相同的 `origin`。暂存、未暂存、未忽略的未跟踪文件都会阻止运行。无法可靠检查 Git 状态时也中止。
- 分别执行 `git pull --ff-only origin master`；拒绝自动合并。随后要求三个 HEAD 一致且等于各自的 `origin/master`，防止发布本地领先提交或不同源码。
- 拉取并确认源码一致后、构建前，检查三个仓库各自的 `readme\QKeyMapper使用手册.html`；缺少则中止。拉取前不要求旧仓库已经具有新手册，避免阻止正常更新。
- 版本自动读取拉取后 `QKeyMapper\QKeyMapper.rc` 的 `ProductVersion`，例如 `v1.5.0.20261016`，不再修改旧 BAT 中的版本。版本的八位日期必须有效。
- 每个仓库均在 **`out\release_package_build`** 完整重建。每次删除并重建这个固定目录，包括 CrashReporter 中间产物，不复用上次的对象文件。原 QtCreator 构建目录不变。
- 检查命令退出码、构建警告、主程序版本、两个 EXE 的架构及 EXE/PDB 配对。普通 Release 不启用诊断或 ASan。仅接受官方 Qt 5.15.2 的 `msvc2019_64` 和 `msvc2019` 套件中 `qtmain.lib(qtmain_win.obj)` 缺失厂商 PDB 的 LNK4099 精确例外；x64 沿用项目既有授权，x86 为用户于 2026-10-16 对本脚本明确批准的扩展。仍核对实际链接库路径，保留全部原始诊断，其余警告阻止发布。
- 构建后和发布前再次检查源码，拒绝运行期间被修改的源码。

`-CheckOnly` 只检查本地已有的远端跟踪信息及手册文件，不代表已经获取远端最新提交；本地旧仓库没有手册时会报告缺少文件，不会联网补齐。正常运行会实际联网拉取；遇到认证或网络错误就中止。

## 配套目录与打包内容

本目录已有的 `QKeyMapper_Qt6_x64`、`QKeyMapper_Qt5_x64`、`QKeyMapper_Qt5_x86` 是配套文件来源，继续由您维护 Qt DLL、驱动、模板、其他文档和工具。脚本不会运行 windeployqt，也不会更新或清理这些原目录。

临时包位于 `_work\staging\QKeyMapper_Qt*`，复制配套目录后：

- 覆盖本次 `QKeyMapper.exe` 和 `QKeyMapperCrashReporter.exe`。
- 完整替换 `utils\script`，同步 `utils\readme.txt`，防止旧 AHK 脚本残留。
- 从对应源码仓库的 `readme\QKeyMapper使用手册.html` 更新包根目录同名离线手册；直接使用已提交的发布HTML，不在Release时运行手册生成器。
- 从对应源码仓库的 `QKeyMapper_Keyname.pdf` 更新并保留 `QKeyMapper按键名称说明.pdf`；不再要求或复制参数速查PDF。
- 仅从临时包根目录排除四个废弃文档：`QKeyMapper使用说明.pdf`、`QKeyMapper映射键参数速查表.pdf`、`QKeyMapper正则匹配规则说明.pdf`、`组合键支持按键列表.pdf`。即使长期配套目录仍有旧副本，也不会带进ZIP；长期配套目录及仓库参考文档不受影响。
- 删除包根目录的 INI、WAV、LOG、DMP、PDB，以及明确的 `log`、`crash_dumps` 运行目录；排除包内所有 PDB。**保留“映射表模版”中的 INI。**
- 其他配套内容原样保留。

ZIP 保持一个 `QKeyMapper_Qt*` 根目录，HTML位于该根目录内。发布前检查 ZIP 文件清单、长度和主程序/CrashReporter/HTML手册内容哈希，并用当前更新器的文件名正则校验：

```text
QKeyMapper_v1.5.0.20261016_Qt6_x64.zip
QKeyMapper_v1.5.0.20261016_Qt5_x64.zip
QKeyMapper_v1.5.0.20261016_Qt5_x86.zip
```

ZIP 不附加时间戳、后缀或其他标记。脚本不上传 GitHub，也不改源码版本。

## 日志、磁盘与失败恢复

固定路径会在重复运行时复用，不为每次 Release 新建构建目录：

| 路径 | 内容 / 保留规则 |
|---|---|
| 各仓库 `out\release_package_build` | 最新一次完整构建及 EXE/PDB；下次先清空 |
| `_work\staging` | 当前三套临时部署目录；下次打包前清空 |
| `_work\pending` | 已检查的待发布 ZIP 和符号；下次打包前清空 |
| `_work\logs\latest` | 最近一次执行的控制台、摘要、Git/工具命令及构建日志；`*.stdout.bin`、`*.stderr.bin` 保留编译命令未经解码的原始字节，文本日志统一为 UTF-8 |
| `_work\logs\previous` | 上次日志；再下一次替换 |
| `_work\previous_release` | 正式替换前的一套 ZIP 和对应 EXE/PDB 备份；下次发布替换 |
| `latest_release` | 最新一套正式 ZIP；保留其他文件和子目录 |
| `Qt*_dump_analyze` | 最新主程序、CrashReporter 及各自 PDB；保留原有分析工具 |

当前三个配套目录合计约 194 MiB。还需要三套完整构建、临时 ZIP、符号和一套发布备份的空间；实际大小随版本变化。

任何一套构建或包检查失败，都不会进入正式替换。替换 ZIP/符号过程中发生可捕获错误时，会恢复上一套文件；恢复若失败，会明确提示并保留 `_work\previous_release`。强制结束进程、关闭运行中的窗口或断电可能打断恢复，此时请根据该目录的原相对路径恢复，或重新运行完成发布。

一次只能运行一个正式 Release；独占锁随进程结束释放，残留的 `release.lock` 文件本身不代表仍有任务。不要手工删除正在使用的锁。

Git 拉取失败时，已经完成的其他仓库拉取不会被自动回滚；解决原因后重新运行即可。脚本不会自动 stash、reset、提交代码或调整 Git 忽略规则。

## 脚本自检

```powershell
pwsh -NoProfile -File .\test_build_release.ps1
```

仅在本目录 `_test_work` 中创建测试 Git 仓库和文件，验证脏工作区、分叉、版本日期、原生命令退出码、目录清理边界、模板保留、ZIP 内容及发布失败恢复，不连接真实远端。该目录为固定测试目录，重复运行先清空。

正式构建和压缩包检查不等于映射、驱动、界面或完整产品回归通过；本脚本不启动 QKeyMapper、不触发驱动或真实输入。
