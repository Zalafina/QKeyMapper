#Requires AutoHotkey v2.0
#SingleInstance Force
#NoTrayIcon

; 配置文件路径（与脚本同目录）
configFile := A_ScriptDir "\mouse_positions.ini"

; 遍历命令行参数
for arg in A_Args {
    if RegExMatch(arg, "i)^save=(.+)$", &m) {
        SaveMousePos(m[1])
    } else if RegExMatch(arg, "i)^restore=(.+)$", &m) {
        RestoreMousePos(m[1])
    }
}

; 保存鼠标位置
SaveMousePos(name) {
    global configFile
    MouseGetPos &x, &y
    IniWrite x, configFile, name, "X"
    IniWrite y, configFile, name, "Y"
}

; 恢复鼠标位置
RestoreMousePos(name) {
    global configFile
    x := IniRead(configFile, name, "X", "")
    y := IniRead(configFile, name, "Y", "")
    if (x != "" && y != "") {
        MouseMove x, y, 0
    }
}
