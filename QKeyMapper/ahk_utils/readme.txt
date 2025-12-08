1. 窗口切换工具：
ahk.exe active_window.ahk process="notepad.exe" title="新建文本文档" launch=true
process= 可传入可执行文件名或者路径，带空格路径注意外面加双引号。
title= 可传入希望切换窗口的完整标题或部分标题，带空格标题注意外面加双引号。
launch=true 配合process= 的可选参数，如果窗口不存在，则尝试通过Run命令启动process=传入的可执行文件名或文件路径。
映射按键用法示例：Run(utils\ahk utils\script\active_window.ahk title=ABC)

2. 鼠标指针位置保存恢复工具：
ahk.exe mouse_position.ahk save=XXX       (将当前鼠标指针位置存储为XXX名称，XXX名称可以自定义)
ahk.exe mouse_position.ahk restore=XXX    (从之前存储的鼠标指针位置中查找XXX名称对应的坐标点，将鼠标指针移动到此坐标点)
注意：保存鼠标指针位置操作需要等待一定时间后再移动鼠标指针，避免存储操作未能正确执行完毕。
映射按键用法示例：Run(utils\ahk utils\script\mouse_position.ahk save=mousepos)⏱50»Mouse-L(500,600)»Run(utils\ahk utils\script\mouse_position.ahk restore=mousepos)

3. 窗口状态更改工具：
ahk.exe config_window.ahk opacity=XXX     (改变鼠标指针指向的窗口或者当前前台窗口的不透明度，可以增加减少也可以设定确定值，范围20~255)
ahk.exe config_window.ahk alwaysontop     (当前鼠标指针指向的窗口或者当前前台窗口切换置顶状态)
ahk.exe config_window.ahk passthrough     (当前鼠标指针指向的窗口或者当前前台窗口切换鼠标穿透状态)
注意：如果置顶和鼠标穿透切换无效，尝试鼠标点击任务栏选择此窗口让此窗口变为前台窗口后再进行切换通常会有效。
映射按键用法示例：
Run(utils\ahk utils\script\config_window.ahk opacity=+2)
Run(utils\ahk utils\script\config_window.ahk opacity=-2)
Run(utils\ahk utils\script\config_window.ahk opacity=150)
Run(utils\ahk utils\script\config_window.ahk alwaysontop)
Run(utils\ahk utils\script\config_window.ahk passthrough)

4. 窗口位置跟随鼠标移动工具
move_window.exe start  (启动窗口位置拖动，启动后当前鼠标指针指向的窗口跟随鼠标指针移动进行位置移动)
move_window.exe stop   (停止窗口位置拖动，停止之后窗口不再跟随鼠标指针位置移动)
启动窗口位置拖动过程中，按下 Esc 键会回到开始拖动之前的窗口位置。
映射按键用法示例：
原始按键： L-Ctrl+Mouse-L
映射按键： Run(utils\ahk utils\script\move_window.ahk start)
抬起映射： Run(utils\ahk utils\script\move_window.ahk stop)
发送时机： 正常+抬起

5. 窗口大小和位置调整工具
winmove.exe wintitle="窗口标题" [其他参数...]
可用参数->
wintitle="..." : 窗口标题 (必需)
wintext="..." : 窗口文本 (可选,用于进一步匹配窗口)
x=数字 : X坐标
y=数字 : Y坐标
w=数字 : 宽度
h=数字 : 高度
extitle="..." : 排除标题
extext="..." : 排除文本
center : 将窗口居中到当前屏幕 (独立参数)
center:N : 将窗口居中到第N个屏幕 (独立参数)
工具命令示例->
ahk.exe winmove.ahk wintitle="记事本" w=800 h=600				(窗口大小调整为 800 x 600)
ahk.exe winmove.ahk wintitle="记事本" x=100 y=200				(窗口左上角坐标位置移动到x=100, y=200)
ahk.exe winmove.ahk wintitle="记事本" x=100 y=200 w=800 h=600	(窗口左上角坐标位置移动到x=100, y=200的同时窗口大小调整为 宽度=800, 高度=600)
ahk.exe winmove.ahk wintitle="记事本" center						(窗口在当前屏幕居中显示)
映射按键用法示例：
Run(utils\ahk utils\script\winmove.ahk wintitle="记事本" w=800 h=600)
Run(utils\ahk utils\script\winmove.ahk wintitle="记事本" x=100 y=200)
Run(utils\ahk utils\script\winmove.ahk wintitle="记事本" x=100 y=200 w=800 h=600)
Run(utils\ahk utils\script\winmove.ahk wintitle="记事本" center)

6. IME切换工具
ahk.exe switch_ime.ahk ime=输入法编码 [其他参数...]
可用参数->
ime=0xXXXXXXXX : 输入法编码(HKL值，必需，十六进制格式)
mode=模式 : 设置输入法模式 (可选)
  通用模式:
    mode=off / english : 关闭输入法(英文模式)
    mode=on / native : 开启输入法(本地语言模式)
  中文输入法:
    mode=chinese : 中文模式(可输入汉字)
  日文输入法:
    mode=hiragana : 平假名模式(ひらがな)
    mode=katakana : 全角片假名模式(カタカナ)
    mode=katakana_half : 半角片假名模式(ｶﾀｶﾅ)
    mode=alphanumeric : 全角英数模式(ＡＢＣ)
target=activewin : 切换当前活动窗口的输入法 (默认)
target=allwin : 切换所有窗口的输入法
debug=true : 启用调试信息输出 (可选)

工具命令示例->
ahk.exe switch_ime.ahk ime=0x04090409                          (切换到英文键盘)
ahk.exe switch_ime.ahk ime=0x08040804                          (切换到中文拼音-保持之前中英文模式)
ahk.exe switch_ime.ahk ime=0x08040804 mode=chinese             (中文拼音-中文模式)
ahk.exe switch_ime.ahk ime=0x08040804 mode=english             (中文拼音-英文模式)
ahk.exe switch_ime.ahk ime=0x04110411                          (切换到日文输入法-保持之前输入模式)
ahk.exe switch_ime.ahk ime=0x04110411 mode=hiragana            (日文输入法-平假名)
ahk.exe switch_ime.ahk ime=0x04110411 mode=katakana            (日文输入法-全角片假名)
ahk.exe switch_ime.ahk ime=0x04110411 mode=katakana_half       (日文输入法-半角片假名)
ahk.exe switch_ime.ahk ime=0x04110411 mode=alphanumeric        (日文输入法-全角字母数字)
ahk.exe switch_ime.ahk ime=0x04110411 mode=english             (日文输入法-直接输入/半角字母数字)
ahk.exe switch_ime.ahk ime=0x08040804 target=allwin            (切换所有窗口)

映射按键用法示例：
Run(utils\ahk utils\script\switch_ime.ahk ime=0x08040804)
Run(utils\ahk utils\script\switch_ime.ahk ime=0x08040804 mode=chinese)
Run(utils\ahk utils\script\switch_ime.ahk ime=0x08040804 mode=english)
Run(utils\ahk utils\script\switch_ime.ahk ime=0x04110411 mode=hiragana)
Run(utils\ahk utils\script\switch_ime.ahk ime=0x04110411 mode=katakana)
Run(utils\ahk utils\script\switch_ime.ahk ime=0x04110411 mode=katakana_half)
Run(utils\ahk utils\script\switch_ime.ahk ime=0x04110411 mode=alphanumeric)
Run(utils\ahk utils\script\switch_ime.ahk ime=0x04110411 mode=english)

常见输入法编码(HKL值)参考：
┌─────────────────────────────────┬──────────────┬────────────────────────────┐
│ 语言/输入法                      │ 编码值(Hex)  │ 说明                       │
├─────────────────────────────────┼──────────────┼────────────────────────────┤
│ 英文 (美式键盘)                │ 0x04090409   │ English (United States)    │
│ 英文 (英国键盘)                │ 0x08090409   │ English (United Kingdom)   │
│ 中文 (简体 - 微软拼音)         │ 0x08040804   │ Microsoft Pinyin IME       │
│ 中文 (简体 - 新加坡拼音)       │ 0x10040804   │ Singapore Chinese IME      │
│ 中文 (繁体 - 注音)             │ 0x04090404   │ Traditional Chinese Zhuyin │
│ 中文 (繁体 - 仓颉)             │ 0x0C090404   │ Traditional Chinese Cangjie│
│ 日文 IME                        │ 0x04110411   │ Microsoft Japanese IME     │
│ 韩文 IME                        │ 0x04120412   │ Microsoft Korean IME       │
│ 德文 (德国键盘)                │ 0x04070407   │ German (Germany)           │
│ 法文 (法国键盘)                │ 0x040C040C   │ French (France)            │
│ 西班牙文 (西班牙键盘)          │ 0x040A040A   │ Spanish (Spain)            │
└─────────────────────────────────┴──────────────┴────────────────────────────┘
注意：
- HKL编码可能因系统版本和安装的语言包不同而有所差异
- 切换的输入法必须已在系统中安装，否则切换无效
- Windows 10/11 系统可在 设置 -> 时间和语言 -> 语言 中添加输入法
