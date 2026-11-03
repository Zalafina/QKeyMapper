@echo off
chcp 65001 >nul
call "%~dp0manage_driver.bat" 1
exit /b %errorlevel%
