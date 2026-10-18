@echo off
setlocal
chcp 65001 >nul
set "releaseExitCode=1"
set "releasePowerShell=%ProgramFiles%\PowerShell\7\pwsh.exe"
if not exist "%releasePowerShell%" (
    where pwsh.exe >nul 2>&1
    if errorlevel 1 (
        set "releasePowerShell="
    ) else (
        set "releasePowerShell=pwsh.exe"
    )
)
set "releaseScript=%~dp0build_release.ps1"
if not exist "%releaseScript%" set "releaseScript="
if defined releasePowerShell if defined releaseScript "%releasePowerShell%" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%releaseScript%" %*
if defined releasePowerShell if defined releaseScript set "releaseExitCode=%ERRORLEVEL%"
rem Use blocks for UTF-8 messages; CMD can misread skipped inline commands.
if not defined releasePowerShell (
    echo Release 已中止：找不到 PowerShell 7.2 或更高版本（pwsh.exe）。请安装 PowerShell 7，或将已有 pwsh.exe 加入 PATH 后重试。
)
if not defined releaseScript (
    echo Release 已中止：缺少脚本 "%~dp0build_release.ps1"。
)
if not "%releaseExitCode%"=="0" (
    echo Release 已中止，退出码 %releaseExitCode%。请查看上方原因及脚本显示的日志位置。
)
echo.
echo 按任意键关闭窗口，也可点击窗口关闭按钮。
pause >nul
exit /b %releaseExitCode%
