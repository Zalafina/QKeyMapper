# QKeyMapper
A keyboard remap tool of Qt5 Widget + WinAPI which could working under Win10.

## Feature details
1. Display a ProcessList of visible windows for select process name & title to match the current ForegroundWindow.
2. Use mouse leftbutton doubleclick a row to select it to the Icon/FileName/WindowTitle area.
3. Push "KeyMappingStart" to start KeyMapping, then it will check ForegroundWindow's process name/title cyclically, KeyMapping function only take effect when ForegroundWindow's process name/title matched.
3. Support add & delete KeyMapDataTable, when the KeyMap ComboBox has focus, push certain keyboardkey will set the ComboBox to the keyname you pressed.
4. Save button will write KeyMapData & ProcessInfo to ini setting file, then it will load it at program startup.
5. Use "SAO UI" font for static strings display.
6. Press "Ctrl + `" to minimize the QKeyMapper to System tray.
7. Mapping multi keys to the same original key will display mappingkeys as "A + B" to support multi key mapping.

## Screenshot
![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_screenshot_02.png)
