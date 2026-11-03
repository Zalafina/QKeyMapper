@echo off
chcp 65001 >nul
call "%~dp0manage_driver.bat" 2
exit /b %errorlevel%
