# 第一方测试

维护中的第一方测试按主题放在 `tests/<主题>/`，对应的 `.pro`、测试代码和专用脚本相邻存放。新增测试沿用这一约定，不在产品目录根部增加 `*_test.pro`。通用构建、静态分析和进程管理工具继续放在仓库 `scripts/`；第三方测试保持上游目录结构。

| 主题 | 工程与运行入口 | 用途 |
|---|---|---|
| [ui_scale](ui_scale/) | `ui_scale_test.pro`、`test_ui_scale.ps1` | 缩放断言与状态恢复 |
| [sendinput_lifecycle](sendinput_lifecycle/) | `sendinput_lifecycle_test.pro` | 实际 SendInputTask 的等待、暂停、取消和退出 |
| [diagnostics](diagnostics/) | `diagnostics_test.pro`、`run_checks.ps1`、`measure_overhead.ps1` | 原生诊断策略、符号、故障注入及进程开销采样 |
| [mcp_ipc](mcp_ipc/) | `mcp_ipc_test.pro` | MCP 本机 IPC 消息边界、真实进程／Token 身份及独立进程管道基础验证 |

所有构建、生成文件、日志和运行副本放在仓库忽略的 `out/tests/`。不同 Qt、配置和 ASan 使用独立子目录；一次性调查探针留在 `out/`，不加入维护测试。Qt Creator 使用影子构建，个人 `.pro.user*` 不提交。

## 构建与运行

以下命令从仓库根目录执行。qmake/JOM 构建命令需要已加载 x64 MSVC 环境；可先在开发者终端执行 `vcvars64.bat`。只构建不会运行测试：

```powershell
$repoRoot = (Get-Location).Path
$qmake = 'C:\Qt\6.8.3\msvc2022_64\bin\qmake.exe'
$jom = 'C:\Qt\Tools\QtCreator\bin\jom\jom.exe'
$buildDirectory = Join-Path $repoRoot 'out/tests/sendinput_lifecycle/qt6-release'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
Push-Location $buildDirectory
try {
    & $qmake (Join-Path $repoRoot 'QKeyMapper/tests/sendinput_lifecycle/sendinput_lifecycle_test.pro') 'CONFIG+=release' 'CONFIG-=debug'
    if ($LASTEXITCODE -ne 0) { throw 'qmake failed.' }
    & $jom -j4
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
}
finally { Pop-Location }
```

另两个目标使用同样方法，替换工程路径并分别使用 `out/tests/ui_scale/qt6-release`、`out/tests/diagnostics/qt6-release`。生命周期测试继续包含真实产品入口并保留正式管理员 manifest；其工程在本地解析继承的产品文件路径，不复制产品源码清单。

完整运行 UI 缩放断言（会创建测试窗口）：

```powershell
& .\QKeyMapper\tests\ui_scale\test_ui_scale.ps1
# Qt5 或 ASan：沿用 -QtRoot、-AddressSanitizer 参数，并使用独立构建目录。
```

诊断检查（默认包括原生自检、符号和导入检查；故障注入需显式 `-Runtime`）：

```powershell
& .\QKeyMapper\tests\diagnostics\run_checks.ps1 `
    -QMakePath 'C:\Qt\6.8.3\msvc2022_64\bin\qmake.exe' `
    -MakePath 'C:\Qt\Tools\QtCreator\bin\jom\jom.exe' `
    -BuildDirectory '.\out\tests\diagnostics\qt6-release'
```

生命周期目标使用 `QKM_LIFECYCLE_CASE` 选择用例、`QKM_LIFECYCLE_RESULT` 指定结果 JSON，缺少这两个值会拒绝运行。现有用例为 `wait_clear`、`pause_clear`、`queue_keyup`、`cancel_reset`、`pause_continue`、`break_one`、`break_all`、`stop`、`restart`、`admission`、`exit_wait`、`exit_pause`、`borrowers`。在 `out/tests/` 准备带依赖和隔离 INI 的运行副本后，通过已有进程工具启动；核对 PID、EXE、启动时间及实际桌面，不操作个人实例。

宏循环回归使用同一目标的 `repeat_*` 用例，完整列表及固定预期见测试源码 `repeatSpecs()`。覆盖宏自身 `x2` 与外层 1／3／无上限循环、通用宏、单步宏、OnlyOnce 嵌套与空结果、异常宏、有限次数上界、按住循环和中断后重新触发。测试以无输出按键运行真实 SendInputTask；`repeatTrace` 记录已执行步骤，`repeatRounds` 记录整轮完成，`repeatRuns` 记录中断后的重新触发。有限次数上界通过预置接近上限的计数验证，不实际执行数千万轮。此测试不证明实际键盘、Hook 或驱动输出。

性能基准使用非 diagnostic 的普通 Release。生命周期测试的普通版保留原等待与 cleanup 耗时门槛；diagnostic 写盘和 ASan 用于完成／销毁及内存安全检查，不用于判断日常性能。运行器须保留每项30秒看门狗；diagnostic 的 `borrowers` 在内部等待全部调用完成。

源码、Git、构建和分析保持非管理员。只有保留正式 manifest 的必要测试启动／关闭使用已授权 gsudo，并由用户确认 UAC。无输出生命周期用例不证明硬件、Hook 或驱动释放。窗口测试和完整阶段门禁按仓库工作流另行验收。

静态报告回归入口仍为 `scripts/test_static_analysis.ps1`，只验证目标发现和报告门禁，不代替真实 Clang-Tidy／Clazy。
