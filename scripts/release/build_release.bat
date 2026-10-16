@echo off
setlocal
chcp 65001 >nul
set "releaseExitCode=1"
set "releasePowerShell=%ProgramFiles%\PowerShell\7\pwsh.exe"
if exist "%releasePowerShell%" goto run
where pwsh.exe >nul 2>&1
if errorlevel 1 goto missingPowerShell
set "releasePowerShell=pwsh.exe"
:run
if not exist "%~dp0build_release.ps1" goto missingScript
"%releasePowerShell%" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_release.ps1" %*
set "releaseExitCode=%ERRORLEVEL%"
if not "%releaseExitCode%"=="0" echo Release 已中止，退出码 %releaseExitCode%。请查看上方原因及脚本显示的日志位置。
goto finish
:missingPowerShell
echo Release 已中止：找不到 PowerShell 7.2 或更高版本（pwsh.exe）。
echo 请安装 PowerShell 7，或将已有 pwsh.exe 加入 PATH 后重试。
goto finish
:missingScript
echo Release 已中止：缺少脚本 "%~dp0build_release.ps1"。
:finish
echo.
echo 按任意键关闭窗口，也可点击窗口关闭按钮。
pause >nul
exit /b %releaseExitCode%
