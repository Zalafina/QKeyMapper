; AutoHotkey v2 脚本
; 用法示例：
;   active_window.ahk process="D:\Tools\NPP\npp.8.6.7\notepad++.exe" title="新文件" launch=true
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
    WinRestore("ahk_id " hwnd)
    WinActivate("ahk_id " hwnd)
}

; -------------------------------
; 参数解析
; -------------------------------
title        := ""
processInput := ""
processName  := ""
launch       := false

for arg in A_Args {
    if InStr(arg, "title=") = 1 {
        title := TrimQuotes(SubStr(arg, 7))
    } else if InStr(arg, "process=") = 1 {
        processInput := TrimQuotes(SubStr(arg, 9))
    } else if (arg = "launch=true") {
        launch := true
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
if (title != "" && processName != "") {
    criteria := title " ahk_exe " processName
} else if (title != "") {
    criteria := title
} else if (processName != "") {
    criteria := "ahk_exe " processName
}

; 如果有搜索条件，进行智能窗口切换
if (criteria != "") {
    ; 获取所有匹配的窗口列表
    matchingWindows := []
    hwndList := WinGetList(criteria)
    
    for hwnd in hwndList {
        ; 过滤掉不可见或最小化的窗口（可选）
        ; if WinGetMinMax("ahk_id " hwnd) = -1  ; 跳过最小化窗口
        ;     continue
        matchingWindows.Push(hwnd)
    }
    
    ; 如果找到匹配的窗口
    if (matchingWindows.Length > 0) {
        currentHwnd := WinGetID("A")  ; 获取当前活动窗口的 HWND
        targetHwnd := 0
        
        if (matchingWindows.Length = 1) {
            ; 只有一个匹配窗口，直接激活
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
