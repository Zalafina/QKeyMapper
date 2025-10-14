#Requires AutoHotkey v2.0
#SingleInstance Force ; 允许多次调用，通过参数控制
#NoTrayIcon           ; 不显示托盘图标

; ====================================================================
; 窗口拖动脚本 - 可被外部软件调用
; 
; 使用方法：
;   启动拖动: move_window.ahk start
;   停止拖动: move_window.ahk stop
;
; 外部软件在按下自定义快捷键时调用 "move_window.ahk start"
; 释放快捷键时调用 "move_window.ahk stop"
; ====================================================================

; 全局变量
global EWD_MouseStartX := 0
global EWD_MouseStartY := 0
global EWD_MouseWin := 0
global EWD_OriginalPosX := 0
global EWD_OriginalPosY := 0
global EWD_TimerRunning := false

; 获取命令行参数
args := A_Args

; 如果没有参数，显示使用说明并退出
if (args.Length = 0) {
    MsgBox("窗口拖动脚本 - 使用方法:`n`n" .
           "启动拖动: move_window.ahk start`n" .
           "停止拖动: move_window.ahk stop`n`n" .
           "说明: 外部软件可通过快捷键调用此脚本`n" .
           "按下快捷键时调用 start 参数`n" .
           "释放快捷键时调用 stop 参数", 
           "窗口拖动脚本", "Icon!")
    ExitApp
}

; 解析参数
command := StrLower(args[1])

; 处理 start 命令 - 开始拖动
if (command = "start") {
    StartWindowDrag()
}
; 处理 stop 命令 - 停止拖动
else if (command = "stop") {
    StopWindowDrag()
}
else {
    MsgBox("未知的命令: " . command . "`n`n有效命令: start, stop", "错误", "Icon!")
    ExitApp
}

; ====================================================================
; 函数: 开始窗口拖动
; ====================================================================
StartWindowDrag()
{
    global EWD_MouseStartX, EWD_MouseStartY, EWD_MouseWin
    global EWD_OriginalPosX, EWD_OriginalPosY, EWD_TimerRunning
    
    ; 如果定时器已经在运行，不重复启动
    if (EWD_TimerRunning) {
        return
    }
    
    ; 设置鼠标坐标模式为屏幕坐标
    CoordMode "Mouse", "Screen"
    
    ; 获取鼠标当前位置和所在窗口
    MouseGetPos &EWD_MouseStartX, &EWD_MouseStartY, &EWD_MouseWin
    
    ; 验证窗口句柄是否有效，如果失败则尝试获取前台窗口
    if (!EWD_MouseWin || EWD_MouseWin = 0) {
        EWD_MouseWin := WinExist("A")
        if (!EWD_MouseWin || EWD_MouseWin = 0) {
            ; 无法获取窗口，静默退出（不影响外部软件体验）
            ExitApp
        }
    }
    
    ; 获取窗口原始位置（用于 Esc 恢复）
    WinGetPos &EWD_OriginalPosX, &EWD_OriginalPosY, , , EWD_MouseWin
    
    ; 只对非最小化窗口启动拖动
    if (!WinGetMinMax(EWD_MouseWin)) {
        EWD_TimerRunning := true
        SetTimer WatchMouseMovement, 10
    }
    else {
        ; 窗口最小化或最大化，不进行拖动
        ExitApp
    }
}

; ====================================================================
; 函数: 停止窗口拖动
; ====================================================================
StopWindowDrag()
{
    global EWD_TimerRunning
    
    ; 停止定时器
    if (EWD_TimerRunning) {
        SetTimer WatchMouseMovement, 0
        EWD_TimerRunning := false
    }
    
    ; 退出脚本
    ExitApp
}

; ====================================================================
; 函数: 监视鼠标移动并拖动窗口
; ====================================================================
WatchMouseMovement()
{
    global EWD_MouseStartX, EWD_MouseStartY, EWD_MouseWin
    global EWD_OriginalPosX, EWD_OriginalPosY
    
    ; 按下 Esc → 恢复窗口原始位置并退出
    if GetKeyState("Escape", "P") {
        SetTimer , 0
        WinMove EWD_OriginalPosX, EWD_OriginalPosY, , , EWD_MouseWin
        ExitApp
    }
    
    ; 获取当前鼠标位置
    CoordMode "Mouse", "Screen"
    MouseGetPos &EWD_MouseX, &EWD_MouseY
    
    ; 获取窗口当前位置
    try {
        WinGetPos &EWD_WinX, &EWD_WinY, , , EWD_MouseWin
    }
    catch {
        ; 窗口可能已关闭，停止拖动
        SetTimer , 0
        ExitApp
    }
    
    ; 计算鼠标移动偏移量
    deltaX := EWD_MouseX - EWD_MouseStartX
    deltaY := EWD_MouseY - EWD_MouseStartY
    
    ; 只有当鼠标实际移动时才更新窗口位置（避免不必要的操作）
    if (deltaX != 0 || deltaY != 0) {
        SetWinDelay -1
        
        ; 移动窗口到新位置
        try {
            WinMove EWD_WinX + deltaX, EWD_WinY + deltaY, , , EWD_MouseWin
        }
        catch {
            ; 移动失败，可能窗口已关闭
            SetTimer , 0
            ExitApp
        }
        
        ; 更新起始坐标为当前坐标
        EWD_MouseStartX := EWD_MouseX
        EWD_MouseStartY := EWD_MouseY
    }
}
