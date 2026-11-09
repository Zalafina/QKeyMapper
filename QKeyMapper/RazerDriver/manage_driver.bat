@echo off
chcp 65001 >nul
setlocal
if "%~1"=="" (
    "%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0manage_driver.ps1"
) else (
    "%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0manage_driver.ps1" -Action "%~1"
)
exit /b %errorlevel%
