# QKeyMapper
## 在Win10和Win11下可以正常使用的键盘映射工具，使用Qt Widget + WinAPI开发，v1.2.8(2022-12-24)开始更新了对Qt6的支持，v1.2.8及之后的Release中使用Qt6编译的版本。
## 注意使用时需要 Visual C++ Redistributable for Visual Studio 2015 64位运行库。
## VC++ 2015 64位运行库，微软下载网址： https://www.microsoft.com/zh-cn/download/confirmation.aspx?id=48145

### 具体功能
1. 左侧显示当前正在运行的可见的窗口程序列表，进程可执行exe文件的文件名和对应的窗口标题名都会显示出来，信息会用于匹配当前处于前台的窗口。
2. 鼠标左键双击左侧窗口程序列表的某一行可以将选定的窗口程序的可自行文件名和窗口标题设定到图标/文件名/窗口标题区域。
3. 按下"KeyMappingStart"按钮可以开始执行键盘映射处理流程，映射工具会定时循环检测当前处于前台的窗口与设定的可自行文件名和窗口标题是否匹配，键盘映射功能可以只在前台窗口与当前进程可执行文件名和窗口标题都匹配的情况才生效，前台窗口不匹配的情况下键盘映射自动失效。
4. 支持在右侧添加和删除键盘映射表，下拉列表获取焦点时按下特定键盘按键可以自动将按键名设置到下拉列表上。
5. "SaveMapData"按钮会将键盘映射表中的信息和进程及窗口标题信息保存到同一路径下的ini设定文件中，之后键盘映射工具下次启动时可以自动加载同一路径下的"keymapdata.ini"文件读取已保存的设定值。
6. 使用了"SAO UI"字体来显示程序界面中的静态字符串。
7. 按下"Ctrl + `"组合键会自动将QKeyMapper最小化显示到任务栏的系统程序托盘上，左键双击程序托盘图标会恢复显示程序窗口。
8. 将多个按键映射到同一个键盘初始按键会按照"A + B"的形式来显示再键盘映射表中，以此来支持用一个键盘初始按键来同时映射多个按键。
9. 加入了"Disable WIN Key"功能可以让键盘上的Windows按键在游戏中无效。
10. 右侧键盘映射表中加入了连发(Burst)选择框，可以按照设定的连发按下时间(BurstPress)和连发抬起时间(BurstRelease)设定来反复发送映射表中的按键。
11. 右侧键盘映射表中加入了锁定(Lock)选择框，选择后第一次按下对应的键盘初始按键后会保持此按键的按下状态，再次按下此键盘初始按键后接触锁定状态，支持与连发(Burst)选择框来组合使用，映射表中的Lock列的ON和OFF表示当前按键锁定状态。
12. 加入了"Auto Start Mapping"功能可以让程序启动后自动根据加载的配置文件开始键盘映射，并且最小化到系统托盘, 。
13. 增加了SelectSetting下拉列表，可以保存最多10组不同的设定，并且通过下拉列表随时切换。
14. 增加了支持将鼠标左键&右键&中键与键盘按键进行互相映射。
15. v1.3.0 -> 版本开始支持"SaveMapData"可以将多个程序的不同键盘映射配置都保存到"keymapdata.ini"文件中。如果配置了"Auto Start Mapping"功能，那么在"KeyMappingStart"状态下会根据当前前台窗口的进程可执行文件名进行自动匹配切换到对应的键盘映射配置。
16. v1.3.1 -> "Auto Startup"勾选框可以让QKeyMapper键盘映射程序在Windows用户登录的时候自动启动，配合"Auto Start Mapping"勾选框开机启动后可以自动最小化到系统托盘图标，取消勾选框则取消开机登录自动启动。
17. v1.3.1 -> 加入单例进程支持，只能同时运行一个QKeyMapper程序, 运行第二个QKeyMapper程序时会将第一个运行的实例程序窗口显示到前台。
18. v1.3.2 -> 在"KeyMappingStart"(循环检测)状态下，如果检测到到前台窗口与当前SelectSetting的键盘映射配置设定一致时候会播放一个音效提示进入到键盘映射生效状态。
19. v1.3.3 -> 在ADD按键右侧增加了"»"勾选框，用于添加按键序列，例如: Ctrl + X 之后 Ctrl + S，设置后效果参考README中显示的截图。
20. v1.3.3 -> 按键列表中添加了对鼠标侧键XButton1和XButton2的支持。
21. v1.3.5 -> 添加中文界面，可以使用语言切换下拉列表进行中英文界面切换，适配4K/2K/1K分辨率。


## A keyboard remap tool of Qt Widget + WinAPI which could working under Win10 & Win11. v1.2.8 (2022-12-24) has update the support for Qt6, v1.2.8 and later Releases will build by Qt6.

### Feature details
1. Display a ProcessList of visible windows for select process name & title to match the current ForegroundWindow.
2. Use mouse leftbutton doubleclick a row to select it to the Icon/FileName/WindowTitle area.
3. Push "KeyMappingStart" to start KeyMapping, then it will check ForegroundWindow's process name/title cyclically, KeyMapping function only take effect when ForegroundWindow's process name/title matched.
4. Support add & delete KeyMapDataTable, when the KeyMap ComboBox has focus, push certain keyboardkey will set the ComboBox to the keyname you pressed.
5. Save button will write KeyMapData & ProcessInfo to ini setting file, then it will load it at program startup.
6. Use "SAO UI" font for static strings display.
7. Press "Ctrl + `" to minimize the QKeyMapper to System tray.
8. Mapping multi keys to the same original key will display mappingkeys as "A + B" to support multi key mapping.
9. "Disable Win Key" checkbox could make Windows Key on the keyboard invalid in the game.
10. Burst checkbox could repeatly send mappingkeys by selected BurstPress and BurstRelease time.
11. Lock checkbox could lock the original key as pressed status, next time the original key pressed will set LockStatus to OFF. Lock checkbox could work wiht Burst checkbox to Lock a key in burst mode. "Lock" column will display ON & OFF to show original key LockStatus.
12. "Auto Start Mapping" checkbox could auto start mapping at program startup, and minimize it to System tray.
13. Add "SelectSetting" combobox to support multi settings selection, could save upto 10 different settings, and switch them by "SelectSetting" combobox.
14. Add support mouse left&right&middle button to mapping with keyborad keys.
15. v1.3.0 -> Start to support save multi different program's keymap settings to "keymapdata.ini" config file. If "Auto Start Mapping" is checked for the keymap setting, it will check foreground window's executable file name to match saved program keymap settings in "keymapdata.ini" config file automatically at "KeyMappingStart" state. If there is a matched keymap setting, it will load this setting and start key mapping automatically.
16. v1.3.1 -> "Auto Startup" checkbox could launch QKeyMapper program at Windows user logon, it could work with "Auto Start Mapping" checkbox.
17. v1.3.1 -> Add SingleApplication support, there could be only one QKeyMapper program instance at the same time. Launch second QKeyMapper program will only bring first QKeyMapper instance to Forgeground Window.
18. v1.3.2 -> When it is detected that the foreground window is matched with the current SelectSetting's mapping configuration at the "KeyMappingStart"(Cycle Checking) state, a sound effect will be played to prompt that keymapping is actived.
19. v1.3.3 -> Add a checkbox "»" next to the "ADD" button, which is used to add a key sequence, such as "Ctrl+X" then "Ctrl+S". The effect after setting can be seen in the screenshot displayed in the README.
20. v1.3.3 -> Add Mouse XButton1 & XButton2 to key list for the support of mouse side buttons.
21. v1.3.5 -> Add Chinese UI, use language select checkbox to change UI language between Chinese and English. Adapt to different resolutions(4K/2K/1K).

## Screenshot
![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_screenshot_03.png)
![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_screenshot_02.png)
