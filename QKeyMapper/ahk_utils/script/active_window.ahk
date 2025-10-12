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
; 主逻辑
; -------------------------------
found := false

if (title != "" && processName != "") {
    ; 标题 + 进程双重条件
    criteria := title " ahk_exe " processName
    if hwnd := WinExist(criteria) {
        ActivateWindow(hwnd)
        found := true
    }
} else if (title != "") {
    if hwnd := WinExist(title) {
        ActivateWindow(hwnd)
        found := true
    }
} else if (processName != "") {
    if hwnd := WinExist("ahk_exe " processName) {
        ActivateWindow(hwnd)
        found := true
    }
}

; 如果没找到，且允许启动
if (!found && launch && processInput != "") {
    Run processInput
}
