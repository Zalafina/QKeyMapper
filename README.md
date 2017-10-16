# QKeyMapper
A keyboard remap tool of Qt5 Widget + WinAPI which could working under Win10.

## Feature details
1. Display a ProcessList of visible windows for select process name & title to match the current ForegroundWindow.  
2. After start KeyMapping it will check ForegroundWindow's process name & title in cyclic, KeyMapping only valid under matched process name & title.
3. Support add & delete KeyMapDataTable.
4. Save button will write KeyMapData to ini setting file, then it will load it at program startup.

## Screenshot
![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_screenshot.png)
