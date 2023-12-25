# QKeyMapper
---------------
## A keyboard remap tool of Qt Widget + WinAPI which could working under Win10 & Win11. v1.2.8 (2022-12-24) has update the support for Qt6，v1.2.8 and later Releases will build by Qt6，v1.3.6(Build 20231125) add joystick keys support.
---------------
### Qt6 version is recommended for Win10 and Win11 OS, and the provision of Qt5 version is only for compatibility with Win7 OS. If the Qt6 version could not be used under Win7, please download the Qt5 version for it.
### Note: Visual C++ Redistributable for Visual Studio 2015 64-bit Runtime needs to be installed when using this software.<br>VC++ 2015 64-bit Runtime, Microsoft download link:<br>https://www.microsoft.com/en-us/download/confirmation.aspx?id=48145
### Note: Version 1.3.6 has changed the names of mouse buttons stored in the ini configuration file. If you select settings stored before version 1.3.6 that include mouse buttons, there may be loading errors. If the error tolerance handling added by the software does not take effect, you can try to manually replace L-Mouse, R-Mouse, M-Mouse, X1-Mouse, X2-Mouse with Mouse-L, Mouse-R, Mouse-M, Mouse-X1, Mouse-X2 in the ini file.
### Note: The newly added virtual game controller feature in version v1.3.6(Build 20231220) requires the installation of the ViGEmBus driver. Frequent installation and uninstallation of the ViGEmBus driver may lead to system blue screen restart risks, so please use it with caution and avoid repeated installation and uninstallation of the driver. If the "Install ViGEmBus" button on the software cannot install the driver normally, you can also manually install the ViGEmBus driver using the "ViGEmBus_1.22.0_x64_x86_arm64.exe" driver installer included in the software zip package.
---------------
### Feature details
1. Display a ProcessList of visible windows for select process name & title to match the current ForegroundWindow.
2. Use mouse leftbutton doubleclick a row to select it to the Icon/FileName/WindowTitle area.
3. Push "KeyMappingStart" to start KeyMapping, then it will check ForegroundWindow's process name/title cyclically, KeyMapping function only take effect when ForegroundWindow's process name/title matched.
4. Support add & delete KeyMapDataTable, when the KeyMap ComboBox has focus, push certain keyboardkey will set the ComboBox to the keyname you pressed.
5. Save button will write KeyMapData & ProcessInfo to ini setting file, then it will load it at program startup.
6. Press "Ctrl + `" to minimize the QKeyMapper to System tray.
7. Mapping multi keys to the same original key will display mappingkeys as "A + B" to support multi key mapping.
8. "Disable Win Key" checkbox could make Windows Key on the keyboard invalid in the game.
9. Burst checkbox could repeatly send mappingkeys by selected BurstPress and BurstRelease time.
10. Lock checkbox could lock the original key as pressed status, next time the original key pressed will set LockStatus to OFF. Lock checkbox could work wiht Burst checkbox to Lock a key in burst mode. "Lock" column will display ON & OFF to show original key LockStatus.
11. "Auto Start Mapping" checkbox could auto start mapping at program startup, and minimize it to System tray.
12. Add "SelectSetting" combobox to support multi settings selection, could save upto 10 different settings, and switch them by "SelectSetting" combobox.
13. Add support mouse left&right&middle button to mapping with keyborad keys.
14. v1.3.0 -> Start to support save multi different program's keymap settings to "keymapdata.ini" config file. If "Auto Start Mapping" is checked for the keymap setting, it will check foreground window's executable file name to match saved program keymap settings in "keymapdata.ini" config file automatically at "KeyMappingStart" state. If there is a matched keymap setting, it will load this setting and start key mapping automatically.
15. v1.3.1 -> "Auto Startup" checkbox could launch QKeyMapper program at Windows user logon, it could work with "Auto Start Mapping" checkbox.
16. v1.3.1 -> Add SingleApplication support, there could be only one QKeyMapper program instance at the same time. Launch second QKeyMapper program will only bring first QKeyMapper instance to Forgeground Window.
17. v1.3.2 -> When it is detected that the foreground window is matched with the current SelectSetting's mapping configuration at the "KeyMappingStart"(Cycle Checking) state, a sound effect will be played to prompt that keymapping is actived.
18. v1.3.3 -> Add a checkbox "»" next to the "ADD" button, which is used to add a key sequence, such as "Ctrl+X" then "Ctrl+S". The effect after setting can be seen in the screenshot displayed in the README.
19. v1.3.3 -> Add Mouse XButton1 & XButton2 to key list for the support of mouse side buttons.
20. v1.3.5 -> Add Chinese UI, use language select checkbox to change UI language between Chinese and English. Adapt to different resolutions(4K/2K/1K).
21. v1.3.5(Build 20230805) -> Added a shortcut key that allows you to switch the mapping start and stop states directly in any state. Press the "Ctrl + F6" shortcut key, and you can immediately switch between the mapping start and stop states whether in the foreground or in the tray display state.
22. v1.3.5(Build 20230806) -> The shortcut key for switching the mapping start and stop states can be customized through the KeySequenceEdit control. After clicking the KeySequenceEdit control with the mouse, press the desired shortcut key to set it. This custom shortcut key setting can save different values for each configuration.
23. v1.3.6(Build 20231125) -> Added the feature to map gamepad buttons as original keys to keyboard keys. Select the keys starting with "Joy" from the original key list, and choose the keyboard key you want to trigger as the mapped key from the mapping key list.
24. v1.3.6(Build 20231220) -> Added virtual gamepad functionality (implemented through ViGEmBus). Click "Install ViGEmBus", and once the green text "ViGEmBus Available" appears, check "Enable Virtual Gamepad". After that, you can map keyboard keys to virtual gamepad keys starting with "vJoy".
25. v1.3.6(Build 20231223) -> Added the feature to control the virtual gamepad's left/right stick via the mouse (Mouse2Joystick). This can be used by adding "vJoy-Mouse2LS" or "vJoy-Mouse2RS" to the mapping table. The sensitivity of the X and Y axes of the joystick controlled by the mouse ranges from "1 to 1000". The smaller the value, the more sensitive it is.
26. v1.3.6(Build 20231223) -> The "AutoMappingMinimize" button has been changed to a tri-state checkbox. When set to the middle state, the software only minimizes to the tray on startup and does not automatically start key mapping.
---------------
## Screenshot
![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_screenshot_02.png)
---------------
## VirScan result
![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_VirScan.png)
