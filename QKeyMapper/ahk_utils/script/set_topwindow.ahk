#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon

; 获取当前前台窗口
hwnd := WinExist("A")

; 切换置顶状态
WinSetAlwaysOnTop -1, "ahk_id " hwnd

; 脚本执行完毕后自动退出
ExitApp
