# Forza 自动油门／刹车状态回归

测试直接编译产品 `qkeymapper_worker.cpp`，运行实际按下、释放和遥测处理路径。
测试入口为无窗口、普通权限的控制台程序；不调用产品 main，不安装 Hook，不连接虚拟手柄总线，不发送真实输入。

测试专用宏只用于该翻译单元：全局阈值由固定的 1.05／10.0 提供，读取检查点由信号量控制。
三个 ViGEm 输出边界函数由测试实现，报告仅记录在内存中。普通产品构建继续读取原 GUI 阈值并使用原驱动接口。

## 覆盖

- `last_release_interleave`：在阈值非空检查后暂停 UDP 读取，另一线程释放最后一个自动油门状态；断言写入被同一互斥锁阻挡，完成后扳机归零。
- `threshold_stack_and_both_triggers`：旧式全局阈值、显式 INIT／THR、后按覆盖与释放回退、组内 INIT 保留、油门／刹车遥测调节和最终释放。
- `same_mutex_reentry_boundary`：在没有竞争写入者的场景中，直接断言 UDP 调用按下函数之前已释放普通非递归锁；阈值辅助函数在调用者持锁时正常完成。
- `empty_keys_fallback_and_invalid_indices`：空键快照兜底，以及无效虚拟手柄下标不产生报告。
- `concurrent_press_release_reset_and_reuse`：6000 帧遥测与 3000 轮双组按下／释放并发，穿插状态表重置，最后重新按下／释放。

状态表重置用例覆盖启停／切表所用容器的并发销毁和重新使用，不代表完整 GUI 启停、切表、退出或硬件验收。
测试不创建主窗口，因此不验证阈值控件、INI 持久化或真实游戏接收效果。ASan 检查内存错误，并非线程竞争检测器；线程排他性由可控交错断言验证。

## 构建

从仓库根目录，在已加载 x64 MSVC 环境的 PowerShell 中执行；Qt／JOM 路径按本机调整：

```powershell
$repoRoot = (Get-Location).Path
$qtRoot = 'C:\Qt\Qt6\6.8.3\msvc2022_64'
$jom = 'C:\Qt\Qt6\Tools\QtCreator\bin\jom\jom.exe'
$buildDirectory = Join-Path $repoRoot 'out/tests/forza_auto_state/qt6-asan'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
Push-Location $buildDirectory
try {
    & "$qtRoot/bin/qmake.exe" "$repoRoot/QKeyMapper/tests/forza_auto_state/forza_auto_state_test.pro" `
        -spec win32-msvc 'CONFIG+=release' 'CONFIG-=debug' 'CONFIG+=asan'
    if ($LASTEXITCODE -ne 0) { throw 'qmake failed.' }
    & $jom -j4
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
}
finally { Pop-Location }
```

普通 Release 测试使用另一个构建目录并省略 `CONFIG+=asan`。产品普通 Release 继续使用 `scripts/build_qt6.ps1`。

## 运行

保留开发者终端的 PATH，以找到匹配编译器的 ASan 运行库。以下依赖来自当前仓库的 x64 DLL 目录：

```powershell
. ./scripts/test_process.ps1
$nativeDirectories = @(
    'QKeyMapper/QJoysticks/lib/SDL/lib/x64', 'QKeyMapper/ViGEm/lib/x64',
    'QKeyMapper/Interception/lib/x64', 'QKeyMapper/libusb/lib/x64', 'QKeyMapper/FakerInput/lib/x64'
) | ForEach-Object { Join-Path $repoRoot $_ }
$logPath = Join-Path $buildDirectory 'run.log'
$test = Start-QkmTestProcess -ExecutablePath "$buildDirectory/release/QKeyMapperForzaAutoStateTest.exe" `
    -EnvironmentOverrides @{
        PATH = (@("$qtRoot/bin") + $nativeDirectories + @($env:PATH)) -join ';'
        ASAN_OPTIONS = 'halt_on_error=1'
        QKM_FORZA_TEST_LOG = $logPath
    }
$null = $test.Process.Handle
if (-not $test.Process.WaitForExit(40000)) { throw 'Test exceeded its watchdog; inspect the owned process.' }
$test.Process.Refresh()
Get-Content -LiteralPath $logPath
if ($test.Process.ExitCode -ne 0) { throw "Test failed: $($test.Process.ExitCode)" }
```

程序自带 30 秒看门狗，正常完成返回 0；断言失败返回 2，看门狗返回 3，日志打开失败返回 4。
运行记录应保留实际 EXE、PID、启动时间、原生退出码、输出与 ASan 报告。

## 功能分支集成

修复从 master 独立形成提交。MCP／IbInput 在各自稳定检查点合入同一修复分支或已包含修复的 master，保留共同历史。
集成时保留 MCP 的停止准入检查，复核启停／切表／退出中的锁顺序；持有虚拟手柄锁时不能等待发送任务，也不能再次调用会取得同一锁的函数。
对合并后的实际源码重跑本测试和相应生命周期用例；两个分支分别通过的历史结果不等于合流通过。
