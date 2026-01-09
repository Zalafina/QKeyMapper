; AutoHotkey v2 脚本
; 用法示例：
;   active_window.ahk process="D:\Tools\NPP\npp.8.6.7\notepad++.exe" title="新文件" launch=true loopSwitch=true
;   active_window.ahk process="D:\Tools\NPP\npp.8.6.7\notepad++.exe" title="新文件" launch=true loopSwitch=false
#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon

SetTitleMatchMode 2  ; 标题部分匹配

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
    if (WinGetMinMax("ahk_id " hwnd) = -1) {  ; 仅最小化才恢复
        WinRestore("ahk_id " hwnd)
    }
    WinActivate("ahk_id " hwnd)
}

; -------------------------------
; 参数解析
; -------------------------------
title        := ""
titleProvided := false
processInput := ""
processName  := ""
launch       := false
loopSwitch   := true  ; 默认启用循环切换

for arg in A_Args {
    if InStr(arg, "title=") = 1 {
        titleProvided := true
        title := TrimQuotes(SubStr(arg, 7))
    } else if InStr(arg, "process=") = 1 {
        processInput := TrimQuotes(SubStr(arg, 9))
    } else if (arg = "launch=true") {
        launch := true
    } else if (arg = "loopSwitch=true") {
        loopSwitch := true
    } else if (arg = "loopSwitch=false") {
        loopSwitch := false
    }
}

; 如果指定了 process 参数，提取 exe 文件名
if (processInput != "") {
    if InStr(processInput, "\") || InStr(processInput, "/") {
        SplitPath processInput, &processName
    } else {
        processName := processInput
    }
}

; -------------------------------
; 主逻辑（支持多窗口循环切换）
; -------------------------------
found := false

; 构建搜索条件
criteria := ""
if (titleProvided) {
    ; 关键点：允许 title=""（空字符串）作为“显式条件”。
    ; AHK 的 WinTitle 参数无法直接表达“仅匹配空标题”，因此采用：先按 process 取列表，再按 WinGetTitle() 精确过滤。
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

; 如果有搜索条件（或显式传入了 title=），进行智能窗口切换
if (criteria != "" || titleProvided) {
    ; 获取所有匹配的窗口列表
    matchingWindows := []
    hwndList := WinGetList(criteria)

    for hwnd in hwndList {
        ; 如果显式指定 title=""，则只保留标题为空的窗口
        if (titleProvided && title = "") {
            if (WinGetTitle("ahk_id " hwnd) != "")
                continue
        }
        ; 过滤掉不可见或最小化的窗口（可选）
        ; if WinGetMinMax("ahk_id " hwnd) = -1  ; 跳过最小化窗口
        ;     continue
        matchingWindows.Push(hwnd)
    }

    ; 如果找到匹配的窗口
    if (matchingWindows.Length > 0) {
        currentHwnd := 0
        try {
            currentHwnd := WinGetID("A")  ; 获取当前活动窗口的 HWND
        } catch {
            currentHwnd := 0  ; 没有活动窗口时，按“不在列表中”处理
        }
        targetHwnd := 0

        if (matchingWindows.Length = 1) {
            ; 只有一个匹配窗口，直接激活
            targetHwnd := matchingWindows[1]
        } else {
            if (!loopSwitch) {
                ; 禁用循环切换时，始终激活第一个窗口
                targetHwnd := matchingWindows[1]
            } else {
                ; 多个匹配窗口，实现循环切换
                currentIndex := 0

                ; 查找当前窗口在列表中的位置
                for index, hwnd in matchingWindows {
                    if (hwnd = currentHwnd) {
                        currentIndex := index
                        break
                    }
                }

                ; 如果当前窗口在匹配列表中，切换到下一个
                if (currentIndex > 0) {
                    ; 循环到下一个窗口
                    nextIndex := Mod(currentIndex, matchingWindows.Length) + 1
                    targetHwnd := matchingWindows[nextIndex]
                } else {
                    ; 当前窗口不在匹配列表中，激活第一个匹配窗口
                    targetHwnd := matchingWindows[1]
                }
            }
        }

        ; 激活目标窗口
        if (targetHwnd != 0) {
            ActivateWindow(targetHwnd)
            found := true
        }
    }
}

; 如果没找到，且允许启动
if (!found && launch && processInput != "") {
    Run processInput
}
