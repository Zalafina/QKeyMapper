1. 窗口切换工具：
active_window.exe process="notepad.exe" title="新建文本文档" launch=true
process= 可传入可执行文件名或者路径，带空格路径注意外面加双引号。
title= 可传入希望切换窗口的完整标题或部分标题，带空格标题注意外面加双引号。
launch=true 配合process= 的可选参数，如果窗口不存在，则尝试通过Run命令启动process=传入的可执行文件名或文件路径。
映射按键用法示例：Run(utils\active_window title=ABC)

2. 鼠标指针位置保存恢复工具：
mouse_position.exe save=XXX       (将当前鼠标指针位置存储为XXX名称，XXX名称可以自定义)
mouse_position.exe restore=XXX    (从之前存储的鼠标指针位置中查找XXX名称对应的坐标点，将鼠标指针移动到此坐标点)
注意：保存鼠标指针位置操作需要等待一定时间后再移动鼠标指针，避免存储操作未能正确执行完毕。
映射按键用法示例：Run(utils\mouse_position save=mousepos)⏱50»Mouse-L(500,600)»Run(utils\mouse_position restore=mousepos)

3. 窗口状态更改工具：
config_window.exe opacity=XXX     (改变鼠标指针指向的窗口或者当前前台窗口的不透明度，可以增加减少也可以设定确定值，范围20~255)
config_window.exe alwaysontop     (当前鼠标指针指向的窗口或者当前前台窗口切换置顶状态)
config_window.exe passthrough     (当前鼠标指针指向的窗口或者当前前台窗口切换鼠标穿透状态)
注意：如果置顶和鼠标穿透切换无效，尝试鼠标点击任务栏选择此窗口让此窗口变为前台窗口后再进行切换通常会有效。
映射按键用法示例：
Run(utils\config_window opacity=+2)
Run(utils\config_window opacity=-2)
Run(utils\config_window opacity=150)
Run(utils\config_window alwaysontop)
Run(utils\config_window passthrough)


4. 窗口位置移动工具
move_window.exe start  (启动窗口位置拖动，启动后当前鼠标指针指向的窗口跟随鼠标指针移动进行位置移动)
move_window.exe stop   (停止窗口位置拖动，停止之后窗口不再跟随鼠标指针位置移动)
启动窗口位置拖动过程中，按下 Esc 键会回到开始拖动之前的窗口位置。
映射按键用法示例：
原始按键： L-Ctrl+Mouse-L
映射按键： Run(utils\move_window start)
抬起映射： Run(utils\move_window stop)
发送时机： 正常+抬起
