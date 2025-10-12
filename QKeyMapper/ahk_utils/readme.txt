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
