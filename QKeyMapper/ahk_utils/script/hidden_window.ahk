#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon
SetTitleMatchMode(2)
SetControlDelay(-1)
SetWinDelay(-1)
SetMouseDelay(-1)
DetectHiddenWindows(false)

; ===================== 固定配置项（非进程相关） =====================
; 隐藏操作延时（最小化→隐藏之间的等待时间）
MinimizeHideDelay := 10
; 恢复操作延时（显示→恢复→激活之间的等待时间）
RestoreShowDelay := 10
; 仅当此值为true时，才会隐藏仅最小化的窗口；false时仅处理可见窗口
HideMinimizedWindows := true
; 窗口排序模式配置项（可选值：
;   "z_order"     - 保持默认Z序收集顺序；
;   "hwnd"        - 按窗口句柄降序，句柄大的先隐藏；
;   "creation_time" - 按窗口创建时间降序，后创建的窗口先隐藏
SortMode := "creation_time"  ; 可改为 "hwnd" 或 "z_order" 切换模式
HiddenWindowStateProp := "QKM.HiddenWindow.State"
HiddenWindowFocusProp := "QKM.HiddenWindow.Focus"
WindowStateNormal := 1
WindowStateMaximized := 2
WindowStateTaskbarMinimized := 3

; ===================== 1. 所有函数定义（先定义后调用，避免函数不存在） =====================

TrimOuterQuotes(str) {
    if (StrLen(str) >= 2) && (SubStr(str, 1, 1) = '"') && (SubStr(str, -1) = '"') {
        return SubStr(str, 2, StrLen(str) - 2)
    }
    return str
}

ParseCommandArgs() {
    argMap := Map()

    for _, arg in A_Args {
        arg := Trim(arg)
        if !arg
            continue

        eqPos := InStr(arg, '=')
        if !eqPos
            continue

        key := StrLower(Trim(SubStr(arg, 1, eqPos - 1)))
        value := TrimOuterQuotes(Trim(SubStr(arg, eqPos + 1)))
        argMap.Set(key, value)
    }

    return argMap
}

ParseBooleanValue(value, argName) {
    lowerValue := StrLower(Trim(value))

    if (lowerValue = 'true' || lowerValue = '1' || lowerValue = 'yes' || lowerValue = 'on')
        return true
    if (lowerValue = 'false' || lowerValue = '0' || lowerValue = 'no' || lowerValue = 'off')
        return false

    throw Error('布尔参数 ' argName ' 的值无效：' value)
}

GetBooleanArg(argMap, keyName, defaultValue, fallbackKeyName := '') {
    if argMap.Has(keyName)
        return ParseBooleanValue(argMap[keyName], keyName)
    if (fallbackKeyName != '' && argMap.Has(fallbackKeyName))
        return ParseBooleanValue(argMap[fallbackKeyName], fallbackKeyName)
    return defaultValue
}

SetWindowProperty(hwnd, propertyName, propertyValue) {
    return !!DllCall('SetPropW', 'Ptr', hwnd, 'Str', propertyName, 'Ptr', propertyValue, 'Int')
}

GetWindowProperty(hwnd, propertyName) {
    return DllCall('GetPropW', 'Ptr', hwnd, 'Str', propertyName, 'Ptr')
}

RemoveWindowProperty(hwnd, propertyName) {
    DllCall('RemovePropW', 'Ptr', hwnd, 'Str', propertyName, 'Ptr')
}

; 函数0：获取窗口创建时间（用于排序）
GetWindowCreationTime(hwnd) {
    static FILETIME_to_UNIX_offset := 116444736000000000
    
    ; 1. 获取窗口所属线程ID
    threadId := DllCall("GetWindowThreadProcessId", "Ptr", hwnd, "Ptr", 0, "UInt")
    
    ; 2. 打开线程（需要 THREAD_QUERY_INFORMATION 权限）
    static THREAD_QUERY_INFORMATION := 0x0040
    hThread := DllCall("OpenThread", "UInt", THREAD_QUERY_INFORMATION, "Int", false, "UInt", threadId, "Ptr")
    
    if (!hThread)
        return 0
    
    ; 3. 获取线程时间（窗口创建时间≈线程创建时间）
    creationTime := Buffer(8, 0)
    exitTime := Buffer(8, 0)
    kernelTime := Buffer(8, 0)
    userTime := Buffer(8, 0)
    
    success := DllCall("GetThreadTimes", "Ptr", hThread, 
        "Ptr", creationTime, "Ptr", exitTime, 
        "Ptr", kernelTime, "Ptr", userTime, "Int")
    
    DllCall("CloseHandle", "Ptr", hThread)
    
    if (!success)
        return 0
    
    ; 4. 将FILETIME转换为可比较的整数
    ; FILETIME是一个64位值，表示自1601年1月1日以来的100纳秒间隔数
    low := NumGet(creationTime, 0, "UInt")
    high := NumGet(creationTime, 4, "UInt")
    return (high << 32) | low
}

; 函数1：解析外部多进程传参
ParseProcessConfig(argMap) {
    configs := []
    static defaultRule := { 
        MatchRule: 'contains', 
        WindowTitleContains: '',
        ClassMatchRule: 'contains',
        WindowClassContains: ''
    }

    processKeys := []
    for key in argMap {
        if RegExMatch(key, '^process\d*$') {
            processKeys.Push(key)
        }
    }
    if !processKeys.Length {
        MsgBox('未传入进程名！`n多进程使用示例：`n脚本名.ahk process1="notepad.exe" title1="临时" class1="Notepad" process2="explorer.exe" rule2="exclude" title2="桌面" classrule2="exact"`n单进程无后缀示例：`n脚本名.ahk process="notepad.exe" rule="exact" title="新建文本文档" class="Notepad"', '参数错误', 0x10)
        ExitApp()
    }

    for _, procKey in processKeys {
        if RegExMatch(procKey, 'process(\d*)', &m) {
            groupNum := m[1] ? m[1] : 1
        } else {
            groupNum := 1
        }
        
        currentConfig := { 
            ProcessName: '', 
            MatchRule: defaultRule.MatchRule, 
            WindowTitleContains: defaultRule.WindowTitleContains,
            ClassMatchRule: defaultRule.ClassMatchRule,
            WindowClassContains: defaultRule.WindowClassContains
        }
        
        currentConfig.ProcessName := argMap.Get(procKey, '')
        ruleKey := (procKey = 'process') ? 'rule' : ('rule' groupNum)
        currentConfig.MatchRule := argMap.Get(ruleKey, argMap.Get('rule' groupNum, defaultRule.MatchRule))
        titleKey := (procKey = 'process') ? 'title' : ('title' groupNum)
        currentConfig.WindowTitleContains := argMap.Get(titleKey, argMap.Get('title' groupNum, defaultRule.WindowTitleContains))
        classRuleKey := (procKey = 'process') ? 'classrule' : ('classrule' groupNum)
        currentConfig.ClassMatchRule := argMap.Get(classRuleKey, argMap.Get('classrule' groupNum, defaultRule.ClassMatchRule))
        classKey := (procKey = 'process') ? 'class' : ('class' groupNum)
        currentConfig.WindowClassContains := argMap.Get(classKey, argMap.Get('class' groupNum, defaultRule.WindowClassContains))
        
        if currentConfig.ProcessName {
            configs.Push(currentConfig)
        }
    }

    return configs
}

; 函数2：深层不可见窗口过滤
IsValidUserWindow(hwnd) {
    ; 1. 基础有效性检查：窗口必须存在且不是桌面/任务栏等系统窗口
    if !WinExist(hwnd) 
        return false
    
    ; 排除桌面窗口（Program Manager）
    if WinGetClass(hwnd) = "Progman" 
        return false
    
    ; 排除任务栏相关窗口
    if WinGetClass(hwnd) = "Shell_TrayWnd" 
        return false
    
    ; 2. 检查窗口是否为用户可见的顶层窗口
    ; GetWindowLongPtr 常量：GWL_EXSTYLE = -20, WS_EX_TOOLWINDOW = 0x00000080
    static GWL_EXSTYLE := -20, WS_EX_TOOLWINDOW := 0x80, WS_EX_APPWINDOW := 0x40000
    exStyle := DllCall("GetWindowLongPtr", "Ptr", hwnd, "Int", GWL_EXSTYLE, "UInt")
    
    ; 排除工具窗口（通常是后台小窗口，如托盘提示）
    if (exStyle & WS_EX_TOOLWINDOW) && !(exStyle & WS_EX_APPWINDOW)
        return false
    
    ; 3. 检查窗口是否有实际尺寸（排除0尺寸的无效窗口）
    WinGetPos(&x, &y, &width, &height, hwnd)
    if (width <= 0 || height <= 0)
        return false
    
    ; 4. 检查窗口是否可交互（排除消息窗口、后台工作窗口）
    if !DllCall("IsWindowEnabled", "Ptr", hwnd, "UInt")
        return false
    
    return true
}

; 函数3：核心逻辑（状态判断+批量操作）
ToggleWindowStatePrecise() {
    global MinimizeHideDelay, RestoreShowDelay, HideMinimizedWindows, SortMode
    global HiddenWindowStateProp, HiddenWindowFocusProp
    global WindowStateNormal, WindowStateMaximized, WindowStateTaskbarMinimized
    try {
        argMap := ParseCommandArgs()
        hideMinimizedWindows := GetBooleanArg(argMap, 'hideminimizedwindows', HideMinimizedWindows, 'hideonlyminimizedwindows')
        AllowedProcesses := ParseProcessConfig(argMap)

        ; ===== 第一步：收集所有符合条件的窗口及状态 =====
        matchedWindows := []
        hasWindowToHide := false
        hasWindowToRestore := false
        activeHwnd := 0
        try activeHwnd := WinExist('A')

        for _, processConfig in AllowedProcesses {
            targetProcess := StrLower(Trim(processConfig.ProcessName))
            titleMatchRule := HasProp(processConfig, 'MatchRule') ? StrLower(Trim(processConfig.MatchRule)) : 'contains'
            titleContent := HasProp(processConfig, 'WindowTitleContains') ? Trim(processConfig.WindowTitleContains) : ''
            classMatchRule := HasProp(processConfig, 'ClassMatchRule') ? StrLower(Trim(processConfig.ClassMatchRule)) : 'contains'
            classContent := HasProp(processConfig, 'WindowClassContains') ? Trim(processConfig.WindowClassContains) : ''
            
            if (targetProcess = '')
                continue
            
            processFilter := 'ahk_exe ' targetProcess
            DetectHiddenWindows(true)
            winHwndList := WinGetList(processFilter)
            DetectHiddenWindows(false)
            
            if (winHwndList.Length = 0)
                continue
            
            for _, hwnd in winHwndList {
                ; 第一步先过滤深层无效窗口
                if !IsValidUserWindow(hwnd)
                    continue
                
                winTitle := WinGetTitle(hwnd)
                winClass := WinGetClass(hwnd)
                
                ; explorer 特殊过滤：仅处理文件管理器窗口
                if (targetProcess = 'explorer.exe') {
                    if (winTitle = '' || winClass != 'CabinetWClass')
                        continue
                }

                ; 标题匹配逻辑
                isTitleMatch := true
                if (titleContent != '') {
                    switch titleMatchRule {
                        case 'contains': isTitleMatch := InStr(StrLower(winTitle), StrLower(titleContent))
                        case 'exact':    isTitleMatch := (winTitle = titleContent)
                        case 'regex':    isTitleMatch := RegExMatch(winTitle, 'i)' titleContent)
                        case 'exclude':  isTitleMatch := !InStr(StrLower(winTitle), StrLower(titleContent))
                        default:         isTitleMatch := InStr(StrLower(winTitle), StrLower(titleContent))
                    }
                }
                
                ; 类名匹配逻辑
                isClassMatch := true
                if (classContent != '') {
                    switch classMatchRule {
                        case 'contains': isClassMatch := InStr(StrLower(winClass), StrLower(classContent))
                        case 'exact':    isClassMatch := (winClass = classContent)
                        case 'regex':    isClassMatch := RegExMatch(winClass, 'i)' classContent)
                        case 'exclude':  isClassMatch := !InStr(StrLower(winClass), StrLower(classContent))
                        default:         isClassMatch := InStr(StrLower(winClass), StrLower(classContent))
                    }
                }
                
                if (!isTitleMatch || !isClassMatch)
                    continue

                ; 获取窗口状态
                winStyle := WinGetStyle(hwnd)
                isVisible := !!(winStyle & 0x10000000)
                minMaxState := WinGetMinMax(hwnd)
                isMinimized := (minMaxState = -1)
                isMaximized := (minMaxState = 1)

                ; 补充可见性二次验证
                isActuallyVisible := DllCall("IsWindowVisible", "Ptr", hwnd, "UInt")
                if !isActuallyVisible
                    isVisible := false

                isHidden := !isVisible
                isNormal := isVisible && !isMinimized && !isMaximized
                isTaskbarMinimized := isMinimized && isVisible
                storedState := GetWindowProperty(hwnd, HiddenWindowStateProp)
                storedFocus := !!GetWindowProperty(hwnd, HiddenWindowFocusProp)

                if (storedState && !isHidden) {
                    RemoveWindowProperty(hwnd, HiddenWindowStateProp)
                    RemoveWindowProperty(hwnd, HiddenWindowFocusProp)
                    storedState := 0
                    storedFocus := false
                }

                ; 过滤无效窗口
                if (!isVisible && !isMinimized && !storedState)
                    continue

                canHide := isNormal || isMaximized || (isTaskbarMinimized && hideMinimizedWindows)

                ; 标记需要隐藏的窗口
                if canHide {
                    hasWindowToHide := true
                } else if storedState {
                    hasWindowToRestore := true
                }

                ; 加入待处理列表
                matchedWindows.Push({
                    hwnd: hwnd,
                    isNormal: isNormal,
                    isMaximized: isMaximized,
                    isMinimized: isMinimized,
                    isHidden: isHidden,
                    isTaskbarMinimized: isTaskbarMinimized,
                    canHide: canHide,
                    storedState: storedState,
                    storedFocus: storedFocus,
                    creationTime: GetWindowCreationTime(hwnd)
                })
            }
        }

        ; ===== 手动实现冒泡排序，完全兼容旧版AHK v2 =====
        ; 判断条件为 hwnd 或 creation_time 时
        if ((SortMode = "hwnd" || SortMode = "creation_time") && matchedWindows.Length > 1) {
            ; 冒泡排序
            n := matchedWindows.Length
            Loop (n - 1) {
                Loop (n - A_Index) {
                    i := A_Index
                    swap := false
                    
                    if (SortMode = "hwnd") {
                        ; 按窗口句柄降序排列（句柄大的先隐藏）
                        if (matchedWindows[i].hwnd < matchedWindows[i+1].hwnd)
                            swap := true
                    } else if (SortMode = "creation_time") {
                        ; 按创建时间降序排列（后创建的窗口先隐藏）
                        if (matchedWindows[i].creationTime < matchedWindows[i+1].creationTime)
                            swap := true
                    }
                    
                    if (swap) {
                        ; 交换两个元素的位置
                        temp := matchedWindows[i]
                        matchedWindows[i] := matchedWindows[i+1]
                        matchedWindows[i+1] := temp
                    }
                }
            }
        }
        ; z_order模式保持原有收集顺序，无需额外处理

        ; ===== 第二步：根据状态判断执行隐藏 or 恢复 =====
        if (hasWindowToHide) {
            focusTargetHwnd := 0
            if activeHwnd {
                for _, w in matchedWindows {
                    if (w.canHide && w.hwnd = activeHwnd) {
                        focusTargetHwnd := w.hwnd
                        break
                    }
                }
            }

            ; 有需要隐藏的窗口 → 执行批量隐藏（按排序后的顺序）
            for w in matchedWindows {
                RemoveWindowProperty(w.hwnd, HiddenWindowFocusProp)
            }

            for w in matchedWindows {
                hwnd := w.hwnd
                ; 隐藏前再次验证窗口有效性
                if !IsValidUserWindow(hwnd)
                    continue

                if !w.canHide
                    continue

                if (hwnd = focusTargetHwnd && !SetWindowProperty(hwnd, HiddenWindowFocusProp, 1))
                    throw Error('无法保存窗口焦点状态，hwnd=' hwnd)

                if w.isNormal {
                    if !SetWindowProperty(hwnd, HiddenWindowStateProp, WindowStateNormal)
                        throw Error('无法保存窗口状态，hwnd=' hwnd)
                    WinMinimize(hwnd)
                    if (MinimizeHideDelay > 0)
                        Sleep(MinimizeHideDelay)
                    WinHide(hwnd)
                }
                else if w.isMaximized {
                    if !SetWindowProperty(hwnd, HiddenWindowStateProp, WindowStateMaximized)
                        throw Error('无法保存窗口状态，hwnd=' hwnd)
                    WinMinimize(hwnd)
                    if (MinimizeHideDelay > 0)
                        Sleep(MinimizeHideDelay)
                    WinHide(hwnd)
                }
                else if (w.isTaskbarMinimized && hideMinimizedWindows) {
                    if !SetWindowProperty(hwnd, HiddenWindowStateProp, WindowStateTaskbarMinimized)
                        throw Error('无法保存窗口状态，hwnd=' hwnd)
                    if (MinimizeHideDelay > 0)
                        Sleep(MinimizeHideDelay)
                    WinHide(hwnd)
                }
            }
        } else if hasWindowToRestore {
            ; 所有匹配窗口都已最小化+隐藏 → 执行批量恢复（倒序，后隐藏的先恢复）
            focusRestoreHwnd := 0
            Loop matchedWindows.Length {
                index := matchedWindows.Length - (A_Index - 1)
                w := matchedWindows[index]
                hwnd := w.hwnd

                ; 恢复前再次验证窗口有效性
                if !IsValidUserWindow(hwnd)
                    continue

                if !w.storedState
                    continue

                WinShow(hwnd)
                if (RestoreShowDelay > 0)
                    Sleep(RestoreShowDelay)

                switch w.storedState {
                    case WindowStateNormal:
                        WinRestore(hwnd)
                        if (RestoreShowDelay > 0)
                            Sleep(RestoreShowDelay)
                    case WindowStateMaximized:
                        WinRestore(hwnd)
                        if (RestoreShowDelay > 0)
                            Sleep(RestoreShowDelay)
                        WinMaximize(hwnd)
                        if (RestoreShowDelay > 0)
                            Sleep(RestoreShowDelay)
                    case WindowStateTaskbarMinimized:
                        if (WinGetMinMax(hwnd) != -1) {
                            WinMinimize(hwnd)
                            if (RestoreShowDelay > 0)
                                Sleep(RestoreShowDelay)
                        }
                }

                if (w.storedFocus && !focusRestoreHwnd && w.storedState != WindowStateTaskbarMinimized) {
                    focusRestoreHwnd := hwnd
                }

                RemoveWindowProperty(hwnd, HiddenWindowStateProp)
                RemoveWindowProperty(hwnd, HiddenWindowFocusProp)
            }

            if focusRestoreHwnd {
                try {
                    if WinExist(focusRestoreHwnd) {
                        WinActivate(focusRestoreHwnd)
                    }
                }
            }
        } else {
            for w in matchedWindows {
                if !w.isHidden {
                    RemoveWindowProperty(w.hwnd, HiddenWindowStateProp)
                    RemoveWindowProperty(w.hwnd, HiddenWindowFocusProp)
                }
            }
        }

    } catch as e {
        MsgBox('脚本执行错误`n原因：' e.Message '`n行号：' e.Line, '错误提示', 0x10)
    }
}

; ===================== 2. 执行核心逻辑（所有函数已定义） =====================
; 脚本启动后立即执行
ToggleWindowStatePrecise()

; 操作完成，退出脚本
ExitApp
