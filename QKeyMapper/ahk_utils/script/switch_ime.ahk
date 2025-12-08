; AutoHotkey v2 脚本 - 切换到指定输入法并设置输入模式
; 用法示例：
;   switch_ime.ahk ime=0x08040804                    ; 切换到中文拼音
;   switch_ime.ahk ime=0x04090409                    ; 切换到英文键盘
;   switch_ime.ahk ime=0x04110411                    ; 切换到日文输入法
;
;   中文输入法模式：
;   switch_ime.ahk ime=0x08040804 mode=chinese       ; 中文模式（可输入汉字）
;   switch_ime.ahk ime=0x08040804 mode=english       ; 英文模式（输入字母）
;
;   日文输入法模式：
;   switch_ime.ahk ime=0x04110411 mode=hiragana      ; 平假名模式
;   switch_ime.ahk ime=0x04110411 mode=katakana      ; 全角片假名模式
;   switch_ime.ahk ime=0x04110411 mode=katakana_half ; 半角片假名模式
;   switch_ime.ahk ime=0x04110411 mode=english       ; 英文模式
;
;   通用模式：
;   switch_ime.ahk ime=0x08040804 mode=on            ; 开启输入法（本地语言）
;   switch_ime.ahk ime=0x08040804 mode=off           ; 关闭输入法（英文）
;
;   其他参数：
;   switch_ime.ahk ime=0x08040804 target=activewin   ; 切换当前活动窗口（默认）
;   switch_ime.ahk ime=0x08040804 target=allwin      ; 切换所有窗口
;   switch_ime.ahk ime=0x08040804 debug=true         ; 启用调试信息
;
; 参考: https://github.com/Tebayaki/AutoHotkeyScripts/blob/main/lib/IME.ahk
#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon

; -------------------------------
; 全局变量
; -------------------------------
gDebugMode := false
gLogFile := A_ScriptDir "\switch_ime_debug.log"

; -------------------------------
; 工具函数
; -------------------------------
; 调试输出函数（支持文件日志）
DebugLog(msg) {
    if (gDebugMode) {
        timestamp := FormatTime(A_Now, "yyyy-MM-dd HH:mm:ss")
        logMsg := "[" timestamp "] " msg "`n"

        ; 输出到 OutputDebug
        OutputDebug(msg)

        ; 同时输出到文件
        try {
            FileAppend(logMsg, gLogFile, "UTF-8")
        }
    }
}

TrimQuotes(str) {
    if (StrLen(str) >= 2) && (SubStr(str, 1, 1) = '"') && (SubStr(str, -1) = '"') {
        return SubStr(str, 2, StrLen(str) - 2)
    }
    return str
}

; 激活键盘布局
ActivateKeyboardLayout(hkl, flags := 0) {
    return DllCall("ActivateKeyboardLayout", "UInt", hkl, "UInt", flags, "UInt")
}

; -------------------------------
; IME 控制函数（基于 ImmGetDefaultIMEWnd）
; -------------------------------

; 获取 IME 窗口句柄
GetIMEWnd(hwnd) {
    return DllCall("imm32\ImmGetDefaultIMEWnd", "ptr", hwnd, "ptr")
}

; 获取 IME 开启状态（中/英文模式）
GetIMEOpenStatus(hwnd) {
    imeWnd := GetIMEWnd(hwnd)
    if (imeWnd) {
        status := 0
        DllCall("SendMessageTimeoutW", "ptr", imeWnd, "uint", 0x283, "ptr", 0x5, "ptr", 0, "uint", 0, "uint", 200, "ptr*", &status)
        return status
    }
    return 0
}

; 设置 IME 开启状态
SetIMEOpenStatus(hwnd, status) {
    imeWnd := GetIMEWnd(hwnd)
    if (imeWnd) {
        DllCall("SendMessageTimeoutW", "ptr", imeWnd, "uint", 0x283, "ptr", 0x6, "ptr", status, "uint", 0, "uint", 200, "ptr*", 0)
        return true
    }
    return false
}

; 获取 IME 转换模式
GetIMEConversionMode(hwnd) {
    imeWnd := GetIMEWnd(hwnd)
    if (imeWnd) {
        mode := 0
        DllCall("SendMessageTimeoutW", "ptr", imeWnd, "uint", 0x283, "ptr", 0x1, "ptr", 0, "uint", 0, "uint", 200, "ptr*", &mode)
        return mode
    }
    return 0
}

; 设置 IME 转换模式
SetIMEConversionMode(hwnd, mode) {
    imeWnd := GetIMEWnd(hwnd)
    if (imeWnd) {
        DllCall("SendMessageTimeoutW", "ptr", imeWnd, "uint", 0x283, "ptr", 0x2, "ptr", mode, "uint", 0, "uint", 200, "ptr*", 0)
        return true
    }
    return false
}

; 获取当前输入模式（true=中文，false=英文）
GetInputMode(hwnd) {
    if !GetIMEOpenStatus(hwnd) {
        return false
    }
    return GetIMEConversionMode(hwnd) & 1
}

; 设置输入模式（支持多种模式）
; 模式参数：
;   "off" / "english" / "0" - 关闭输入法（英文模式）
;   "on" / "native" / "1" - 开启输入法（本地语言模式）
;   对于中文输入法：
;     "chinese" / "1025" - 中文模式（简体拼音）
;   对于日文输入法（标准转换模式）：
;     "hiragana" / "9" - 平假名模式 (IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE)
;     "katakana" / "11" - 全角片假名模式 (IME_CMODE_NATIVE | IME_CMODE_KATAKANA | IME_CMODE_FULLSHAPE)
;     "katakana_half" / "3" - 半角片假名模式 (IME_CMODE_NATIVE | IME_CMODE_KATAKANA)
;     "alphanumeric" / "8" - 全角英数模式 (IME_CMODE_FULLSHAPE)
SetInputMode(hwnd, modeStr, hkl := 0, showDebug := false) {
    conversionMode := -1
    openStatus := -1

    ; 解析模式字符串
    switch modeStr {
        case "off", "english", "0":
            openStatus := 0

        case "on", "native", "1":
            openStatus := 1
            ; 根据输入法设置默认转换模式
            if (hkl = 0x08040804) {
                conversionMode := 1025  ; 中文拼音
            } else if (hkl = 0x04110411) {
                conversionMode := 9     ; 日文平假名
            } else {
                conversionMode := 1     ; 其他输入法
            }

        case "chinese", "1025":
            openStatus := 1
            conversionMode := 1025  ; 中文模式

        case "hiragana", "9":
            openStatus := 1
            conversionMode := 9     ; 平假名 (NATIVE=1 | FULLSHAPE=8)

        case "katakana", "11":
            openStatus := 1
            conversionMode := 11    ; 全角片假名 (NATIVE=1 | KATAKANA=2 | FULLSHAPE=8)

        case "katakana_half", "3":
            openStatus := 1
            conversionMode := 3     ; 半角片假名 (NATIVE=1 | KATAKANA=2)

        case "alphanumeric", "8":
            openStatus := 1
            conversionMode := 8     ; 全角英数 (FULLSHAPE=8)

        default:
            ; 尝试作为数字解析
            if IsInteger(modeStr) {
                conversionMode := Integer(modeStr)
                openStatus := (conversionMode > 0) ? 1 : 0
            }
    }

    ; 应用设置
    if (openStatus != -1) {
        SetIMEOpenStatus(hwnd, openStatus)
        Sleep(30)

        if (conversionMode != -1 && openStatus = 1) {
            SetIMEConversionMode(hwnd, conversionMode)
        }

        if (showDebug) {
            DebugLog("设置输入模式: " modeStr " OpenStatus=" openStatus " ConversionMode=" conversionMode)
        }
        return true
    }

    return false
}

; 切换指定窗口的输入法
SwitchWindowIME(hwnd, hkl, imeMode := "", showDebug := false) {
    try {
        if (hwnd && WinExist("ahk_id " hwnd)) {
            if (showDebug) {
                title := WinGetTitle("ahk_id " hwnd)
                DebugLog("切换窗口输入法: HWND=" hwnd " Title=" title " IME=" Format("0x{:08X}", hkl))
            }

            ; 发送输入法切换消息
            PostMessage(0x50, 0, hkl, , "ahk_id " hwnd)
            Sleep(50)

            ; 如果指定了输入模式,设置输入模式
            if (imeMode != "") {
                Sleep(50)  ; 等待输入法切换完成

                ; 调用统一的输入模式设置函数,支持所有模式(中文/日文/英文等)
                SetInputMode(hwnd, imeMode, hkl, showDebug)

                if (showDebug) {
                    openStatus := GetIMEOpenStatus(hwnd)
                    convMode := GetIMEConversionMode(hwnd)
                    DebugLog("当前 IME 状态: OpenStatus=" openStatus " ConversionMode=" convMode)
                }
            }

            return true
        }
        return false
    } catch as err {
        if (showDebug) {
            DebugLog("切换输入法错误: " err.Message)
        }
        return false
    }
}

; 切换当前活动窗口的输入法
SwitchActiveWindowIME(hkl, imeMode := "", showDebug := false) {
    hwnd := WinExist("A")
    if (hwnd) {
        result := SwitchWindowIME(hwnd, hkl, imeMode, showDebug)
        Sleep(30)
        ActivateKeyboardLayout(hkl)
        return result
    }
    return false
}

; 切换所有窗口的输入法
SwitchAllWindowsIME(hkl, imeMode := "", showDebug := false) {
    count := 0
    hwndList := WinGetList()

    for hwnd in hwndList {
        if (SwitchWindowIME(hwnd, hkl, imeMode, false)) {
            count++
        }
    }

    ; 最后切换全局布局
    Sleep(50)
    ActivateKeyboardLayout(hkl)

    if (showDebug) {
        DebugLog("已切换 " count " 个窗口的输入法")
    }

    return count
}

; -------------------------------
; 参数解析
; -------------------------------
imeCode := ""
target := "activewin"  ; 默认切换当前活动窗口 (activewin | allwin)
imeMode := ""          ; 输入模式 (chinese | english)
showDebug := false

for arg in A_Args {
    if InStr(arg, "ime=") = 1 {
        imeCode := TrimQuotes(SubStr(arg, 5))
    } else if InStr(arg, "target=") = 1 {
        target := TrimQuotes(SubStr(arg, 8))
    } else if InStr(arg, "mode=") = 1 {
        imeMode := TrimQuotes(SubStr(arg, 6))
    } else if (arg = "debug=true") {
        showDebug := true
        gDebugMode := true
    }
}

; -------------------------------
; 参数验证
; -------------------------------
if (imeCode = "") {
    MsgBox("错误：缺少 ime 参数`n`n用法示例：`n  switch_ime.ahk ime=0x08040804`n  switch_ime.ahk ime=0x04090409", "参数错误", "Icon!")
    ExitApp(1)
}

; 转换输入法代码（支持十六进制字符串）
try {
    if InStr(imeCode, "0x") = 1 {
        hkl := Integer(imeCode)
    } else {
        hkl := Integer("0x" imeCode)
    }
} catch {
    MsgBox("错误：无效的输入法代码 '" imeCode "'`n`n输入法代码必须是十六进制格式，例如：0x08040804", "参数错误", "Icon!")
    ExitApp(1)
}

; 验证 mode 参数（支持多种模式）
validModes := ["off", "on", "english", "native", "chinese", "hiragana", "katakana", "katakana_half", "alphanumeric", "0", "1"]
if (imeMode != "" && !HasValue(validModes, imeMode) && !IsInteger(imeMode)) {
    helpText := "错误：无效的 mode 参数 '" imeMode "'`n`n"
    helpText .= "通用模式：`n"
    helpText .= "  mode=off / english - 关闭输入法（英文）`n"
    helpText .= "  mode=on / native - 开启输入法（本地语言）`n`n"
    helpText .= "中文输入法：`n"
    helpText .= "  mode=chinese - 中文模式`n`n"
    helpText .= "日文输入法：`n"
    helpText .= "  mode=hiragana - 平假名`n"
    helpText .= "  mode=katakana - 全角片假名`n"
    helpText .= "  mode=katakana_half - 半角片假名`n"
    helpText .= "  mode=alphanumeric - 全角英数`n`n"
    helpText .= "或使用数字转换模式值"
    MsgBox(helpText, "参数错误", "Icon!")
    ExitApp(1)
}

; 辅助函数：检查数组是否包含值
HasValue(arr, val) {
    for item in arr {
        if (item = val)
            return true
    }
    return false
}

; -------------------------------
; 执行切换
; -------------------------------
if (showDebug) {
    ; 清空之前的日志文件
    try {
        FileDelete(gLogFile)
    }

    modeText := imeMode != "" ? " 模式: " imeMode : ""
    DebugLog("======================================")
    DebugLog("开始切换输入法: " Format("0x{:08X}", hkl) " 目标: " target modeText)
}

success := false

switch target {
    case "activewin":
        success := SwitchActiveWindowIME(hkl, imeMode, showDebug)

    case "allwin":
        count := SwitchAllWindowsIME(hkl, imeMode, showDebug)
        success := (count > 0)

    default:
        if (showDebug) {
            DebugLog("错误：未知的目标类型 '" target "'")
        }
        ExitApp(1)
}

; -------------------------------
; 退出
; -------------------------------
if (showDebug) {
    if (success) {
        DebugLog("输入法切换成功")
    } else {
        DebugLog("输入法切换失败")
    }
    DebugLog("======================================")

    ; 显示日志文件位置
    MsgBox("调试信息已保存到:`n" gLogFile "`n`n点击确定后将打开日志文件", "调试信息", "Icon!")
    Run("notepad.exe " gLogFile)
}

ExitApp(success ? 0 : 1)
