# QKeyMapper
## 在Win10和Win11下可以正常使用的键盘映射工具，使用Qt5 Widget + WinAPI开发。

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

## A keyboard remap tool of Qt5 Widget + WinAPI which could working under Win10.

### Feature details
1. Display a ProcessList of visible windows for select process name & title to match the current ForegroundWindow.
2. Use mouse leftbutton doubleclick a row to select it to the Icon/FileName/WindowTitle area.
3. Push "KeyMappingStart" to start KeyMapping, then it will check ForegroundWindow's process name/title cyclically, KeyMapping function only take effect when ForegroundWindow's process name/title matched.
4. Support add & delete KeyMapDataTable, when the KeyMap ComboBox has focus, push certain keyboardkey will set the ComboBox to the keyname you pressed.
5. Save button will write KeyMapData & ProcessInfo to ini setting file, then it will load it at program startup.
6. Use "SAO UI" font for static strings display.
7. Press "Ctrl + `" to minimize the QKeyMapper to System tray.
8. Mapping multi keys to the same original key will display mappingkeys as "A + B" to support multi key mapping.
9. Disable Win Key checkbox could make Windows Key on the keyboard invalid in the game.
10. Burst checkbox could repeatly send mappingkeys by selected BurstPress and BurstRelease time.
11. Lock checkbox could lock the original key as pressed status, next time the original key pressed will set LockStatus to OFF. Lock checkbox could work wiht Burst checkbox to Lock a key in burst mode. "Lock" column will display ON & OFF to show original key LockStatus.

## Screenshot
![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_screenshot_02.png)
