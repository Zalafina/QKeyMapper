; AutoHotkey v2 脚本 - 显示当前活动窗口的输入法/键盘布局 HKL
; 用法：
;   ahk.exe show_ime_hkl.ahk
;
; 说明：
; - HKL 是“窗口线程的键盘布局句柄”，用于 switch_ime.ahk 的 ime=0xXXXXXXXX 参数。
; - 本脚本读取“当前前台窗口所属线程”的 HKL，因此在编辑器里打字的状态下运行也能正确反映该窗口的输入法布局。

#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon

GetActiveWindowHKL(&hwnd := 0, &tid := 0, &pid := 0) {
    hwnd := WinExist("A")
    if (!hwnd) {
        hwnd := DllCall("GetForegroundWindow", "Ptr")
    }
    if (!hwnd) {
        return 0
    }

    pidLocal := 0
    tid := DllCall("GetWindowThreadProcessId", "Ptr", hwnd, "UInt*", &pidLocal, "UInt")
    pid := pidLocal

    if (!tid) {
        return 0
    }

    ; GetKeyboardLayout 返回 HKL（在 AHK v2 里用 Ptr 接收最稳妥）
    return DllCall("GetKeyboardLayout", "UInt", tid, "Ptr")
}

hkl := GetActiveWindowHKL(&hwnd, &tid, &pid)
if (!hkl) {
    MsgBox("错误：未能获取当前活动窗口的 HKL。", "show_ime_hkl", "Icon!")
    ExitApp(1)
}

try {
    title := WinGetTitle("ahk_id " hwnd)
} catch {
    title := ""
}

msg := "HKL: " Format("0x{:08X}", hkl) "`n"
msg .= "HWND: " hwnd "`n"
msg .= "TID: " tid "  PID: " pid
if (title != "") {
    msg .= "`nTitle: " title
}

MsgBox(msg, "show_ime_hkl", "Iconi")
ExitApp(0)
