#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon

args := A_Args
if (args.Length = 0)
    ExitApp

; Parse named parameters (e.g., wintitle="Notepad++" x=100 y=200)
; If first argument doesn't contain "=", treat as positional parameters (backward compatibility)
isNamedParams := (args.Length > 0 && InStr(args[1], "="))

if (isNamedParams) {
    ; Named parameters mode
    params := Map()
    params.CaseSense := false  ; Case-insensitive parameter names
    centerMode := ""  ; Store center mode if found
    
    for arg in args {
        ; Check if this is a center parameter (standalone, not key=value)
        if (RegExMatch(arg, "^(center)(:\d+)?$", &m)) {
            centerMode := arg
            continue
        }
        
        if (RegExMatch(arg, "^(\w+)=(.*)$", &m)) {
            paramName := StrLower(m[1])
            paramValue := m[2]
            
            ; Remove quotes if present
            if (RegExMatch(paramValue, '^"(.*)"$', &quoted)) {
                paramValue := quoted[1]
            }
            
            params[paramName] := paramValue
        }
    }
    
    ; Extract parameters with default empty values
    WinTitle     := params.Has("wintitle") ? params["wintitle"] : ""
    WinText      := params.Has("wintext") ? params["wintext"] : ""
    X            := params.Has("x") ? params["x"] : ""
    Y            := params.Has("y") ? params["y"] : ""
    W            := params.Has("w") ? params["w"] : ""
    H            := params.Has("h") ? params["h"] : ""
    ExcludeTitle := params.Has("extitle") ? params["extitle"] : ""
    ExcludeText  := params.Has("extext") ? params["extext"] : ""
    
    ; Check for center mode
    if (centerMode != "" && RegExMatch(centerMode, "^center(:\d+)?$", &m)) {
        try {
            ; Get window position and size
            WinGetPos(&wx, &wy, &ww, &wh, WinTitle, WinText != "" ? WinText : unset, ExcludeTitle != "" ? ExcludeTitle : unset, ExcludeText != "" ? ExcludeText : unset)
            
            if (m[1] = "") {
                ; center → current screen
                hWnd := WinExist(WinTitle)
                monIndex := MonitorFromWindow(hWnd)
            } else {
                ; center:N → specific screen
                monIndex := Integer(SubStr(m[1], 2))
            }
            
            ; Get target screen work area
            if MonitorGetWorkArea(monIndex, &mx, &my, &mw, &mh) {
                newX := mx + (mw - ww) // 2
                newY := my + (mh - wh) // 2
                WinMove(newX, newY, ww, wh, WinTitle, WinText != "" ? WinText : unset, ExcludeTitle != "" ? ExcludeTitle : unset, ExcludeText != "" ? ExcludeText : unset)
            }
        }
        ExitApp
    }
}
else {
    ; Positional parameters mode (backward compatibility)
    WinTitle     := args.Length >= 1 ? args[1] : ""
    WinText      := args.Length >= 2 ? args[2] : ""
    X            := args.Length >= 3 ? args[3] : ""
    Y            := args.Length >= 4 ? args[4] : ""
    W            := args.Length >= 5 ? args[5] : ""
    H            := args.Length >= 6 ? args[6] : ""
    ExcludeTitle := args.Length >= 7 ? args[7] : ""
    ExcludeText  := args.Length >= 8 ? args[8] : ""
    
    ; Check for center mode in positional parameters
    if (args.Length >= 2 && RegExMatch(args[2], "^center(:\d+)?$", &m)) {
        try {
            ; Get window position and size
            WinGetPos(&wx, &wy, &ww, &wh, WinTitle, WinText, ExcludeTitle, ExcludeText)
            
            if (m[1] = "") {
                ; center → current screen
                hWnd := WinExist(WinTitle " " WinText)
                monIndex := MonitorFromWindow(hWnd)
            } else {
                ; center:N → specific screen
                monIndex := Integer(SubStr(m[1], 2))
            }
            
            ; Get target screen work area
            if MonitorGetWorkArea(monIndex, &mx, &my, &mw, &mh) {
                newX := mx + (mw - ww) // 2
                newY := my + (mh - wh) // 2
                WinMove(newX, newY, ww, wh, WinTitle, WinText, ExcludeTitle, ExcludeText)
            }
        }
        ExitApp
    }
}

; 默认调用 WinMove
; Convert empty strings to unset for all optional parameters
; In AHK v2, empty string parameters may behave differently from omitted parameters
CallWinMove(X, Y, W, H, WinTitle, WinText, ExcludeTitle, ExcludeText) {
    ; Check which parameters are provided (non-empty)
    hasX := (X != "")
    hasY := (Y != "")
    hasW := (W != "")
    hasH := (H != "")
    hasWinText := (WinText != "")
    hasExcludeTitle := (ExcludeTitle != "")
    hasExcludeText := (ExcludeText != "")
    
    ; Convert numeric strings to integers if provided
    numX := hasX ? Integer(X) : unset
    numY := hasY ? Integer(Y) : unset
    numW := hasW ? Integer(W) : unset
    numH := hasH ? Integer(H) : unset
    
    ; Convert string parameters to unset if empty
    strWinText := hasWinText ? WinText : unset
    strExcludeTitle := hasExcludeTitle ? ExcludeTitle : unset
    strExcludeText := hasExcludeText ? ExcludeText : unset
    
    ; Call WinMove with appropriate parameters based on what's provided
    ; We need to handle all combinations of optional trailing parameters
    if (hasExcludeText) {
        WinMove(numX?, numY?, numW?, numH?, WinTitle, strWinText?, strExcludeTitle?, strExcludeText)
    }
    else if (hasExcludeTitle) {
        WinMove(numX?, numY?, numW?, numH?, WinTitle, strWinText?, strExcludeTitle)
    }
    else if (hasWinText) {
        ; WinMove(numX?, numY?, numW?, numH?, WinTitle, strWinText)
        WinMove(numX?, numY?, numW?, numH?, WinTitle, strWinText)
    }
    else {
        WinMove(numX?, numY?, numW?, numH?, WinTitle)
    }
}

CallWinMove(X, Y, W, H, WinTitle, WinText, ExcludeTitle, ExcludeText)
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
