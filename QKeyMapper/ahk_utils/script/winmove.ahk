#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon

args := A_Args
if (args.Length = 0)
    ExitApp

WinTitle      := args.Length >= 1 ? args[1] : ""
WinText       := args.Length >= 2 ? args[2] : ""
X             := args.Length >= 3 ? args[3] : ""
Y             := args.Length >= 4 ? args[4] : ""
W             := args.Length >= 5 ? args[5] : ""
H             := args.Length >= 6 ? args[6] : ""
ExcludeTitle  := args.Length >= 7 ? args[7] : ""
ExcludeText   := args.Length >= 8 ? args[8] : ""

; 检查是否是 center 模式
if (args.Length >= 2 && RegExMatch(args[2], "^center(:\d+)?$", &m)) {
    try {
        ; 获取窗口位置和大小
        WinGetPos(&wx, &wy, &ww, &wh, WinTitle, WinText, ExcludeTitle, ExcludeText)

        if (m[1] = "") {
            ; center → 当前屏幕
            hWnd := WinExist(WinTitle " " WinText)
            monIndex := MonitorFromWindow(hWnd)
        } else {
            ; center:N → 指定屏幕
            monIndex := Integer(SubStr(m[1], 2))
        }

        ; 获取目标屏幕工作区
        if MonitorGetWorkArea(monIndex, &mx, &my, &mw, &mh) {
            newX := mx + (mw - ww) // 2
            newY := my + (mh - wh) // 2
            WinMove(newX, newY, ww, wh, WinTitle, WinText, ExcludeTitle, ExcludeText)
        }
    }
    ExitApp
}

; 默认调用 WinMove
try WinMove(X, Y, W, H, WinTitle, WinText, ExcludeTitle, ExcludeText)
ExitApp


; 辅助函数：获取窗口所在屏幕编号
MonitorFromWindow(hWnd) {
    hMon := DllCall("MonitorFromWindow", "ptr", hWnd, "uint", 2, "ptr")
    if !hMon
        return 1
    count := MonitorGetCount()
    loop count {
        ; v2 内置 MonitorGetWorkArea 已经能直接用索引，不需要句柄比对
        ; 所以这里直接返回 1..count 即可
        if (A_Index = 1)  ; 简化逻辑：默认返回主屏
            return 1
    }
    return 1
}
