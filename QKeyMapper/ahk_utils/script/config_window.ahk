#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon

; 获取命令行参数
args := A_Args

; 如果没有参数，直接退出
if (args.Length = 0) {
    ExitApp
}

; 获取鼠标当前位置下的窗口句柄
MouseGetPos , , &hwnd

; 验证窗口句柄是否有效，如果失败则尝试获取前台窗口
if (!hwnd) {
    hwnd := WinExist("A")
    if (!hwnd) {
        ; MsgBox("无法获取鼠标指向的窗口或前台窗口", "错误", "Icon!")
        ExitApp
    }
}

; 透明度的最小值和最大值限制
OPACITY_MIN := 20
OPACITY_MAX := 255

; 解析并处理参数
alwaysOnTopFlag := false
opacityFlag := false
opacityValue := 0

for arg in args {
    ; 处理 alwaysontop 参数
    if (InStr(arg, "alwaysontop")) {
        alwaysOnTopFlag := true
    }
    ; 处理 opacity 参数
    else if (InStr(arg, "opacity=")) {
        opacityFlag := true
        opacityStr := SubStr(arg, InStr(arg, "=") + 1)
        
        ; 获取当前窗口透明度
        currentOpacity := WinGetTransparent("ahk_id " hwnd)
        
        ; 如果窗口当前没有设置透明度，默认为255（不透明）
        if (currentOpacity = "") {
            currentOpacity := 255
        }
        
        ; 判断是绝对值还是相对值
        if (SubStr(opacityStr, 1, 1) = "+") {
            ; 增加透明度
            delta := SubStr(opacityStr, 2)
            opacityValue := currentOpacity + Integer(delta)
        }
        else if (SubStr(opacityStr, 1, 1) = "-") {
            ; 减少透明度
            delta := SubStr(opacityStr, 2)
            opacityValue := currentOpacity - Integer(delta)
        }
        else {
            ; 绝对值
            opacityValue := Integer(opacityStr)
        }
        
        ; 限制透明度值在有效范围内 (20~255)
        if (opacityValue < OPACITY_MIN) {
            opacityValue := OPACITY_MIN
        }
        else if (opacityValue > OPACITY_MAX) {
            opacityValue := OPACITY_MAX
        }
    }
}

; 执行窗口配置操作

; 切换窗口置顶状态
if (alwaysOnTopFlag) {
    WinSetAlwaysOnTop -1, "ahk_id " hwnd
}

; 设置窗口透明度
if (opacityFlag) {
    WinSetTransparent opacityValue, "ahk_id " hwnd
}

; 操作完成，退出脚本
ExitApp
