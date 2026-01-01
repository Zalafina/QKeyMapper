#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon

SetTitleMatchMode 2  ; 标题部分匹配

; ------------------------------------------------------------
; WinMoveEx: 基于 HWND + WinAPI 的窗口移动/缩放工具
;
; 兼容 winmove.ahk 原有参数，并新增：
; - process=  : 可执行文件名或路径（路径会提取 exe 名用于 ahk_exe 匹配）
; - title=    : wintitle= 的别名（两者同义，若都提供则“最后出现覆盖”）
; - active=true : 可选，移动/缩放后激活窗口；默认不抢焦点
;
; 默认使用 SetWindowPos（更可控），如需切换可改为 "MoveWindow"
; ------------------------------------------------------------
MOVE_API := "SetWindowPos"  ; "SetWindowPos" | "MoveWindow"


; -------------------------------
; 工具函数
; -------------------------------
TrimQuotes(str) {
    if (StrLen(str) >= 2) && (SubStr(str, 1, 1) = '"') && (SubStr(str, -1) = '"') {
        return SubStr(str, 2, StrLen(str) - 2)
    }
    return str
}

ActivateWindow(hwnd) {
    WinShow("ahk_id " hwnd)
    WinRestore("ahk_id " hwnd)
    WinActivate("ahk_id " hwnd)
}

EnsureVisibleWithoutActivate(hwnd) {
    ; 默认策略：尽量不打扰焦点，不主动 WinActivate。
    ; 这里仅在窗口被隐藏时显示出来；不恢复最小化（避免视觉打扰）。
    try {
        WinShow("ahk_id " hwnd)
    }
}

GetWorkAreaForWindow(hwnd, &left, &top, &right, &bottom) {
    ; 通过 WinAPI 获取窗口所在显示器的工作区（rcWork）
    hMon := DllCall("MonitorFromWindow", "ptr", hwnd, "uint", 2, "ptr")  ; MONITOR_DEFAULTTONEAREST=2
    if (!hMon)
        return false

    ; MONITORINFO: cbSize(4) + rcMonitor(16) + rcWork(16) + dwFlags(4) = 40 bytes
    mi := Buffer(40, 0)
    NumPut("uint", 40, mi, 0)
    ok := DllCall("GetMonitorInfoW", "ptr", hMon, "ptr", mi, "int")
    if (!ok)
        return false

    left   := NumGet(mi, 20, "int")
    top    := NumGet(mi, 24, "int")
    right  := NumGet(mi, 28, "int")
    bottom := NumGet(mi, 32, "int")
    return true
}

ApiMoveWindow(hwnd, x, y, w, h, repaint := true) {
    return DllCall("MoveWindow", "ptr", hwnd, "int", x, "int", y, "int", w, "int", h, "int", repaint ? 1 : 0, "int")
}

ApiSetWindowPos(hwnd, x, y, w, h, flags) {
    return DllCall("SetWindowPos", "ptr", hwnd, "ptr", 0, "int", x, "int", y, "int", w, "int", h, "uint", flags, "int")
}


; -------------------------------
; 参数解析（兼容 winmove.ahk：具名/位置两种模式）
; -------------------------------
args := A_Args
if (args.Length = 0)
    ExitApp

isNamedParams := (args.Length > 0 && InStr(args[1], "="))

title := ""
titleProvided := false
processInput := ""
processName := ""
winText := ""
excludeTitle := ""
excludeText := ""

active := false
centerMode := false
centerMonitorIndex := 0  ; 0 表示“窗口所在屏幕”

xSet := false
ySet := false
wSet := false
hSet := false
x := 0
y := 0
w := 0
h := 0

if (isNamedParams) {
    for arg in args {
        ; standalone center / center:N
        if (RegExMatch(arg, "^center(?::(\d+))?$", &mCenter)) {
            centerMode := true
            centerMonitorIndex := (mCenter[1] != "") ? Integer(mCenter[1]) : 0
            continue
        }

        if (RegExMatch(arg, "^(\w+)=(.*)$", &m)) {
            paramName := StrLower(m[1])
            paramValue := TrimQuotes(m[2])

            switch paramName {
                case "process":
                    processInput := paramValue
                case "title", "wintitle":
                    ; 两者同义，按“最后出现覆盖”
                    titleProvided := true
                    title := paramValue
                case "wintext":
                    winText := paramValue
                case "extitle":
                    excludeTitle := paramValue
                case "extext":
                    excludeText := paramValue
                case "x":
                    if (paramValue != "") {
                        xSet := true
                        x := Integer(paramValue)
                    }
                case "y":
                    if (paramValue != "") {
                        ySet := true
                        y := Integer(paramValue)
                    }
                case "w":
                    if (paramValue != "") {
                        wSet := true
                        w := Integer(paramValue)
                    }
                case "h":
                    if (paramValue != "") {
                        hSet := true
                        h := Integer(paramValue)
                    }
                case "active":
                    if (StrLower(paramValue) = "true" || paramValue = "1")
                        active := true
            }
        }
    }
} else {
    ; 位置参数模式（向后兼容 winmove.ahk）：
    ; WinTitle, WinText, X, Y, W, H, ExcludeTitle, ExcludeText
    ; 以及：第二个参数可为 center/center:N
    title := TrimQuotes(args.Length >= 1 ? args[1] : "")
    titleProvided := (title != "")  ; 位置模式下通常要求 WinTitle，不提供则无意义

    if (args.Length >= 2 && RegExMatch(args[2], "^center(?::(\d+))?$", &mCenter2)) {
        centerMode := true
        centerMonitorIndex := (mCenter2[1] != "") ? Integer(mCenter2[1]) : 0
        winText := ""
    } else {
        winText := TrimQuotes(args.Length >= 2 ? args[2] : "")
    }

    if (args.Length >= 3 && args[3] != "") {
        xSet := true
        x := Integer(args[3])
    }
    if (args.Length >= 4 && args[4] != "") {
        ySet := true
        y := Integer(args[4])
    }
    if (args.Length >= 5 && args[5] != "") {
        wSet := true
        w := Integer(args[5])
    }
    if (args.Length >= 6 && args[6] != "") {
        hSet := true
        h := Integer(args[6])
    }

    excludeTitle := TrimQuotes(args.Length >= 7 ? args[7] : "")
    excludeText := TrimQuotes(args.Length >= 8 ? args[8] : "")
}

; 如果指定了 process 参数，提取 exe 文件名
if (processInput != "") {
    if InStr(processInput, "\\") || InStr(processInput, "/") {
        SplitPath processInput, &processName
    } else {
        processName := processInput
    }
}

; 至少需要一个窗口匹配条件：title/wintitle 或 process
if (!titleProvided && processName = "")
    ExitApp


; -------------------------------
; 目标窗口匹配（参考 active_window.ahk 的 process/title 组合规则）
; -------------------------------
criteria := ""
if (titleProvided) {
    ; 允许 title=""（空字符串）作为显式条件：通过 WinGetTitle() 二次过滤实现
    if (title != "" && processName != "") {
        criteria := title " ahk_exe " processName
    } else if (title != "") {
        criteria := title
    } else if (processName != "") {
        criteria := "ahk_exe " processName
    } else {
        criteria := ""  ; 仅 title=""，则先枚举所有窗口再过滤空标题
    }
} else if (title != "" && processName != "") {
    criteria := title " ahk_exe " processName
} else if (title != "") {
    criteria := title
} else if (processName != "") {
    criteria := "ahk_exe " processName
}

matchingWindows := []
hwndList := WinGetList(criteria
    , winText != "" ? winText : unset
    , excludeTitle != "" ? excludeTitle : unset
    , excludeText != "" ? excludeText : unset)

for hwnd in hwndList {
    ; 如果显式指定 title=""，则只保留标题为空的窗口
    if (titleProvided && title = "") {
        if (WinGetTitle("ahk_id " hwnd) != "")
            continue
    }
    matchingWindows.Push(hwnd)
}

if (matchingWindows.Length = 0)
    ExitApp

currentHwnd := WinGetID("A")
targetHwnd := 0

if (matchingWindows.Length = 1) {
    targetHwnd := matchingWindows[1]
} else {
    currentIndex := 0
    for index, hwnd in matchingWindows {
        if (hwnd = currentHwnd) {
            currentIndex := index
            break
        }
    }

    if (currentIndex > 0) {
        nextIndex := Mod(currentIndex, matchingWindows.Length) + 1
        targetHwnd := matchingWindows[nextIndex]
    } else {
        targetHwnd := matchingWindows[1]
    }
}

if (targetHwnd = 0)
    ExitApp


; -------------------------------
; 计算目标位置/尺寸（center 支持按“新尺寸”居中）
; -------------------------------
WinGetPos(&curX, &curY, &curW, &curH, "ahk_id " targetHwnd)

doMove := (centerMode || xSet || ySet)
doSize := (wSet || hSet)

if (!doMove && !doSize)
    ExitApp

newW := wSet ? w : curW
newH := hSet ? h : curH
newX := xSet ? x : curX
newY := ySet ? y : curY

if (centerMode) {
    wl := 0, wt := 0, wr := 0, wb := 0

    if (centerMonitorIndex > 0) {
        try {
            MonitorGetWorkArea(centerMonitorIndex, &wl, &wt, &wr, &wb)
        } catch {
            ; 无效屏幕编号，直接退出
            ExitApp
        }
    } else {
        if (!GetWorkAreaForWindow(targetHwnd, &wl, &wt, &wr, &wb))
            ExitApp
    }

    workW := wr - wl
    workH := wb - wt

    newX := wl + (workW - newW) // 2
    newY := wt + (workH - newH) // 2
}


; -------------------------------
; 执行移动/缩放（WinAPI）
; 默认不抢焦点；active=true 才激活
; -------------------------------
if (active) {
    ; 用户显式要求激活：先确保窗口可见/非最小化，再移动缩放，最后激活
    WinShow("ahk_id " targetHwnd)
    if (WinGetMinMax("ahk_id " targetHwnd) = -1)
        WinRestore("ahk_id " targetHwnd)
} else {
    EnsureVisibleWithoutActivate(targetHwnd)
}

if (MOVE_API = "MoveWindow") {
    ; MoveWindow 必须同时传入 x/y/w/h，所以用当前值补齐。
    ApiMoveWindow(targetHwnd, newX, newY, newW, newH, true)
} else {
    ; SetWindowPos 支持分别只移动/只改尺寸，并可通过 flags 控制是否激活
    SWP_NOSIZE := 0x0001
    SWP_NOMOVE := 0x0002
    SWP_NOZORDER := 0x0004
    SWP_NOACTIVATE := 0x0010
    SWP_NOOWNERZORDER := 0x0200
    SWP_NOSENDCHANGING := 0x0400

    flags := SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING
    if (!active)
        flags |= SWP_NOACTIVATE
    if (!doMove)
        flags |= SWP_NOMOVE
    if (!doSize)
        flags |= SWP_NOSIZE

    ApiSetWindowPos(targetHwnd
        , doMove ? newX : 0
        , doMove ? newY : 0
        , doSize ? newW : 0
        , doSize ? newH : 0
        , flags)
}

if (active)
    WinActivate("ahk_id " targetHwnd)

ExitApp
