#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon

; 获取命令行参数
args := A_Args

; 如果没有参数，直接退出
if (args.Length = 0) {
    ExitApp
}

; ====================================================================
; 窗口跳过名单 - 用于过滤不应该被操作的系统窗口
; 可以根据需要继续添加更多规则
; ====================================================================
; 跳过名单结构：每条规则包含 进程名(ProcessName)、窗口标题(WindowTitle) 和 窗口类名(ClassName)
; 所有非空字段都匹配时才跳过，空字符串表示不检查该项
global SkipList := [
    ; Windows 桌面
    {ProcessName: "explorer.exe", WindowTitle: "", ClassName: "Progman"},
    ; Windows 任务栏
    {ProcessName: "explorer.exe", WindowTitle: "", ClassName: "Shell_TrayWnd"},
    ; 可以继续添加更多规则，例如：
    ; {ProcessName: "explorer.exe", WindowTitle: "", ClassName: "WorkerW"},
    ; {ProcessName: "dwm.exe", WindowTitle: "", ClassName: ""},
    ; {ProcessName: "", WindowTitle: "特定标题", ClassName: ""},
    ; {ProcessName: "", WindowTitle: "", ClassName: "SomeClassName"}
]

; ====================================================================
; 函数: 检查窗口是否在跳过名单中
; 参数: hwnd - 窗口句柄
; 返回: true=应该跳过, false=不跳过
; ====================================================================
IsWindowInSkipList(hwnd) {
    global SkipList

    ; 获取窗口信息
    try {
        processName := WinGetProcessName("ahk_id " hwnd)
        windowTitle := WinGetTitle("ahk_id " hwnd)
        className := WinGetClass("ahk_id " hwnd)
    }
    catch {
        ; 无法获取窗口信息，不跳过
        return false
    }

    ; 遍历跳过名单
    for rule in SkipList {
        ; 检查进程名是否匹配（如果规则中指定了进程名）
        processMatch := (rule.ProcessName = "" || StrLower(processName) = StrLower(rule.ProcessName))

        ; 检查窗口标题是否匹配（如果规则中指定了标题）
        ; 使用 InStr 进行包含匹配，支持部分匹配
        titleMatch := (rule.WindowTitle = "" || InStr(windowTitle, rule.WindowTitle))

        ; 检查类名是否匹配（如果规则中指定了类名）
        classMatch := (rule.ClassName = "" || className = rule.ClassName)

        ; 如果所有非空字段都匹配，则应该跳过此窗口
        if (processMatch && titleMatch && classMatch) {
            return true
        }
    }

    ; 不在跳过名单中
    return false
}

; ====================================================================
; 获取目标窗口句柄
; ====================================================================
; 获取鼠标当前位置下的窗口句柄
MouseGetPos , , &hwnd

; 验证窗口句柄是否有效，如果失败则尝试获取前台窗口
; 这个 fallback 很重要：当窗口设置了点击穿透后，MouseGetPos 无法获取到它
; 此时使用前台窗口作为目标，可以解除穿透状态
if (!hwnd) {
    hwnd := WinExist("A")
    if (!hwnd) {
        ExitApp
    }

    ; 检查前台窗口是否在跳过名单中
    if (IsWindowInSkipList(hwnd)) {
        ; 前台窗口是系统窗口（如桌面、任务栏），不进行操作
        ExitApp
    }
}
else {
    ; 检查鼠标位置的窗口是否在跳过名单中
    if (IsWindowInSkipList(hwnd)) {
        ; 尝试使用前台窗口作为备选
        hwnd := WinExist("A")
        if (!hwnd || IsWindowInSkipList(hwnd)) {
            ; 前台窗口也不可用，退出
            ExitApp
        }
    }
}

; 透明度的最小值和最大值限制
OPACITY_MIN := 20
OPACITY_MAX := 255

; 解析并处理参数
alwaysOnTopFlag := false
passthroughFlag := false
opacityFlag := false
opacityValue := 0

for arg in args {
    ; 处理 alwaysontop 参数
    if (InStr(arg, "alwaysontop")) {
        alwaysOnTopFlag := true
    }
    ; 处理 passthrough 参数
    else if (InStr(arg, "passthrough")) {
        passthroughFlag := true
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

; 切换窗口鼠标点击穿透状态
if (passthroughFlag) {
    ; 获取窗口当前的扩展样式
    currentExStyle := WinGetExStyle("ahk_id " hwnd)

    ; WS_EX_TRANSPARENT (0x20) 和 WS_EX_LAYERED (0x80000) 用于实现点击穿透
    WS_EX_TRANSPARENT := 0x20
    ; WS_EX_LAYERED := 0x80000

    ; 检查是否已经启用了点击穿透
    if (currentExStyle & WS_EX_TRANSPARENT) {
        ; 已启用，移除点击穿透效果
        newExStyle := currentExStyle & ~WS_EX_TRANSPARENT
        WinSetExStyle newExStyle, "ahk_id " hwnd
    }
    else {
        ; 未启用，添加点击穿透效果（需要同时设置 WS_EX_LAYERED 和 WS_EX_TRANSPARENT）
        ; newExStyle := currentExStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT
        newExStyle := currentExStyle | WS_EX_TRANSPARENT
        WinSetExStyle newExStyle, "ahk_id " hwnd
    }
}

; 设置窗口透明度
if (opacityFlag) {
    WinSetTransparent opacityValue, "ahk_id " hwnd
}

; 操作完成，退出脚本
ExitApp
