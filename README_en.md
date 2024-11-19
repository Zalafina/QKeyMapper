# QKeyMapper

<p align="left">
    <a href="./LICENSE"><img src="https://img.shields.io/github/license/Zalafina/QKeyMapper"></a>
    <a href="https://github.com/Zalafina/QKeyMapper/releases"><img src="https://img.shields.io/github/v/release/Zalafina/QKeyMapper?color=09e"></a>
    <a href="https://github.com/Zalafina/QKeyMapper/stargazers"><img src="https://img.shields.io/github/stars/Zalafina/QKeyMapper?color=ccf"></a>
    <a href="https://github.com/Zalafina/QKeyMapper/releases/latest" target="_blank"><img src="https://img.shields.io/github/downloads/Zalafina/QKeyMapper/total.svg"/></a>
</p>

## A key mapping tool that works well on Win10 and Win11, developed using Qt Widget + WinAPI. Support for Qt6 was updated starting from v1.2.8 (2022-12-24). The versions of the Release after v1.2.8 are compiled with Qt6. v1.3.6 (Build 20231220) added support for gamepad keys and virtual gamepads. v1.3.7 (Build 20240410) added support for multi-key mouse + multiple virtual gamepads.

---

### Download the latest Release version zip package:

### https://github.com/Zalafina/QKeyMapper/releases/latest

### https://gitee.com/asukavov/QKeyMapper/releases/latest

### ※ The ZIP packages starting with QKeyMapper_vX.Y.Z_x64/x86 are compiled executable file zip packages. Build_YYYYMMDD represents the compilation date, and the newer compilation date has corresponding new feature descriptions.

---

### License change: Please note that the software license has been changed from the MIT license to the GNU General Public License v3.0 (GPL v3). Before using, modifying, or distributing the software, please make sure you have viewed and understood the terms of the new license.

### Win10 and Win11 systems are recommended to use the Qt6 version, and the Qt5 version is provided just to be compatible with the Win7 system. If you cannot use the Qt6 version under Win7, please download the Qt5 version for use.

### Note: You may need to install the Visual C++ Redistributable 64-bit runtime library when using.`<br>`VC++ 2015-2022 64-bit runtime library, Microsoft download address:`<br>`[https://aka.ms/vs/17/release/vc_redist.x64.exe](https://aka.ms/vs/17/release/vc_redist.x64.exe)

* (For 32-bit systems, download and install the 32-bit runtime library [https://aka.ms/vs/17/release/vc_redist.x86.exe](https://aka.ms/vs/17/release/vc_redist.x86.exe) )

### [Note: Win7 system needs to install ViGEMBus v1.16.116 by itself to use the virtual gamepad function](https://github.com/Zalafina/ViGEmBus_v1.16.116_Win7_InstallFiles)

* [https://github.com/Zalafina/ViGEmBus_v1.16.116_Win7_InstallFiles](https://github.com/Zalafina/ViGEmBus_v1.16.116_Win7_InstallFiles)

---

## For tips on using the key mapping tool, refer to the Wiki page:

* [QKeyMapper Tips and More Wiki](https://github.com/Zalafina/QKeyMapper/wiki)

---

## For a collection of tutorial videos, click the image below

## If you have any questions during use, you can also join the Q group for consultation (Group number: 906963961)

### New features list (sorted in descending order of update time)
* v1.3.7(Build 20241120)
    * Added the "CheckCombinationKeyOrder" checkbox in the mapping settings window. When enabled, if the original key is a combination, the order in which the keys are pressed must match the sequence of the original key combination for it to trigger. For example, for the original key combination "A+B" with "CheckCombKeyOrder" enabled, pressing A first and then B will trigger the combination. The default value for "CheckCombKeyOrder" is enabled. If you do not want to check the order, uncheck this option.

* v1.3.7(Build 20241116)
    * Added a "Record Keys" button in the mapping settings window. Pressing this button opens the key recording dialog. In the recording dialog, press the "F11" key to start recording and the "F12" key to stop recording. You can also click the "Start Record"/"Stop Record" buttons with the left mouse button to control the recording process. After recording, the captured key sequence is displayed in the lineedit box and automatically copied to the clipboard. You can directly paste the recorded keys into the "MappingKey" lineedit box or other fields.

* v1.3.7 (Build 20241112)
    * **New Feature: "SendTiming" Options**:  
        * Added a **"SendTiming"** dropdown in the mapping configuration window, allowing users to choose when mapped keys are sent based on the state of the original key. Options include:  
          - **Normal**  
          - **KeyDown**  
          - **KeyUp**  
          - **KeyDown+KeyUp**  
          - **Normal+KeyUp**  
        * The previous **"KeyUp Action"** checkbox has been replaced and removed.  

    * **Separate KeyUpMapping**:  
        * Introduced a **"KeyUpMapping"** field in the configuration window to specify a separate key mapping triggered when the original key is released.  
        * By default, the **"KeyUpMapping"** inherits the same value as the **"MappingKey"** field but can be customized independently.  
          - **MappingKey**: Sent when the original key is pressed.  
          - **KeyUpMapping**: Sent when the original key is released.

    * **"MappingKeyUnlock" Option**:  
        * Added a **"MappingKeyUnlock"** checkbox, available when the **"Lock"** option is selected.  
        * Functionality:  
          - While a key is locked (e.g., holding **L-Shift** locks it in a pressed state), pressing it again will unlock and release the key.  
          - If **"MappingKeyUnlock"** is also checked, a short press of any mapped key can unlock and release the locked key.  
          - Example: Mapping **L-Shift⏲500** to **L-Shift** with **Lock** enabled will lock **L-Shift** when long-pressed. Unlock it with either another long press or a short press of the mapped key if **MappingKeyUnlock** is enabled.

    * **Extended "！" Prefix Functionality**:  
        * Enhanced the **"！" prefix** in the mapping configuration to support **Input after override(SOCD - Last Input Priority) for both physical and virtual keys**.
        * Input after override(SOCD - Last Input Priority) is now available for both keyboard keys and mouse buttons, ensuring seamless key remapping across physical and virtual devices.
* v1.3.7 (Build 20241030)

  * **Crash Fix for Burst Key Presses**:
    * Resolved a potential crash issue when holding burst keys.
  * **Save Settings Hotkey**:
    * While mapping is inactive, pressing **L-Ctrl+S** with the main window in focus now executes the **"Save Settings"** action.
  * **Miscellaneous Fixes**:
    * Addressed other issues affecting user experience.
* v1.3.7 (Build 20241024)

  * **Key Sequence Holddown Issue Fix**:
    * Fixed a bug in the **"Key Sequence Holddown"** feature to ensure consistent behavior.
* v1.3.7 (Build 20241022)

  * **New Key Mapping: KeySequenceBreak**:
    * Added a new mapping key **"KeySequenceBreak"** to interrupt ongoing key sequences connected by "»".
    * Example: If the original key **Q** is mapped to **"KeySequenceBreak"**, pressing **Q** immediately halts all ongoing key sequences in the active mapping table.
  * **Optimized Key Sequence and Burst Handling**:
    * Improved the method of sending rapid-fire keys for smoother performance.
  * **Key State Synchronization**:
    * When stopping burst or key sequence sends, the program now checks for physically pressed keys and reissues their press messages to ensure their states remain consistent.
  * **New Launch Parameter: `--scale=`**:
    * Added the `--scale=` launch parameter to enforce a specific UI scaling factor.
    * Example: Setting the shortcut as `QKeyMapper.exe --scale=1.0` will start the program at a 1.0 scaling factor.
    * If no `--scale=` parameter is passed and the system scaling is set to 1.0, the program defaults to a 1.25 scaling factor.
* v1.3.7 (Build 20241012)

  * **Gamepad Detection Update**:
    * Updated the SDL2 library for gamepad detection to **SDL2-2.30.8** (released on 2024-10-02).
    * Updated the gamepad detection database to support more devices.
* v1.3.7 (Build 20241006)

  * **Mapping Item Description**:
    * Added a **"Mapping Item Description"** text box to the "Mapping Table Settings" dialog.
    * You can add remarks or notes for a specific key mapping. When hovering over the "OriginalKey" field in the mapping list, the description is displayed as a tooltip at the mouse pointer location.
  * **ShowNotes Button**:
    * A new **"ShowNotes"** toggle button has been added to the top-right corner of the main window.
    * By default, this button is unchecked (released), and remarks are not appended to the original key list.
    * Clicking the button toggles it to a checked (pressed) state, displaying the original key in the format:
      `Original Key【Mapping Item Note】`.
  * **Optimized Concurrent Key Processing**:
    * Improved the handling of simultaneous key mapping sends for better performance and stability during high-frequency input scenarios.
* v1.3.7 (Build 20240928)

  * Added "RepeatByKey" and "RepeatTimes" checkboxes in the mapping settings window. Only one of these checkboxes can be selected at a time:
    - **RepeatByKey**: Sends the key sequence repeatedly while the original key remains pressed.
    - **RepeatTimes**: Specifies how many times the key sequence will be repeated for a single press of the original key.
  * Introduced the "！" prefix for the "Mapped Key List" in the mapping settings window. This prefix can be added before a normal keyboard or mouse mapped key to implement input after override(SOCD - Last Input Priority). For example:
    - If the "D" key is mapped to "！A," pressing the "D" key after pressing the "A" key will automatically send a release signal for "A."
    - When the "D" key is released, if the "A" key's physical button is still pressed, it will automatically send an "A key pressed" message.
    - (Note: This character is not the exclamation mark on the keyboard and must be added through the "Mapped Key List" in the mapping settings window or copied from elsewhere.)
  * Added a "Mapping Description" text box below the "Save Settings" button in the main window. This allows users to add descriptive notes for the saved settings corresponding to processes and titles. The description is displayed in the settings selection list as `process.exe|TitleX【Mapping Description】`.
  * Moved the language selection dropdown to the "General" settings tab.
  * Added a "Process List" toggle button in the top-right corner of the main window. By default, the "Process List" button is in the checked state, displaying the process list on the left side of the main window. Clicking the button toggles it to the unchecked state, hiding the process list and expanding the mapping table on the right to occupy the space previously used by the process list.
* v1.3.7 (Build 20240916)

  * Support for adding multiple mapping tables under a single configuration option (process name + title). The default mapping table tab name is "Tab1." Double-click the "+" tab button with the left mouse button to add a blank mapping table. Tab names are automatically numbered sequentially as "Tab2," "Tab3," and so on.
  * Double-click an existing mapping table tab name with the left mouse button to open the "Mapping Table Settings" dialog. Here, you can modify the "Mapping Table Name" and "Mapping Table Hotkey." Changes take effect after clicking the "Update" button. To save the updated "Mapping Table Name" and "Mapping Table Hotkey" to the configuration file, you must click the "Save Settings" button.
  * "Mapping Table Hotkey" allows you to switch the currently active mapping table while the mapping is enabled in the game, enabling you to switch between multiple key mappings during gameplay. If the hotkey is prefixed with the `$` character, the hotkey will not be intercepted and will still be sent to the application.
  * The "Mapping Table Settings" dialog now includes an "Export Mapping Table" button, which can export all key mapping data from a specific mapping table to an INI file. The default file name for export is "mapdatatable.ini," but you can set a custom file name in the save file dialog.
  * The "Mapping Table Settings" dialog now includes an "Import Mapping Table" button, allowing you to import previously exported mapping table data from an INI file. The imported data is appended to the end of the selected mapping table. Since duplicate mappings with the same original key are not allowed in a single table, imported data with original keys already existing in the table will be filtered out.
  * Physical gamepad key input detection now supports detecting virtual gamepad input. This allows you to combine virtual gamepad key mappings with physical gamepad key mappings. For example, you can map `vJoy-Mouse2RS` along with `Joy-RS-Up`, `Joy-RS-Down`, `Joy-RS-Left`, and `Joy-RS-Right` to keyboard keys, enabling functionality where mouse movements trigger keyboard keys.
* v1.3.7 (Build 20240812)

  * In the "Original Keys List," physical gamepad keys prefixed with `Joy-` can now include `@` followed by a numeric player index (`0` to `9`) to distinguish mappings based on the gamepad's player number. A total of 10 player indices are supported. If no `@` + numeric player index is added, the mapping will apply to inputs from all physical gamepads.
  * Added a "Gamepad" dropdown combo box to assist with selecting the player index (PlayerIndex) for mapping original keys. When selecting a gamepad player index, the `@` + numeric index (`0~9`) is automatically appended to the `Joy-` prefixed original key mapping. For example, `"Joy-Key1(A/×)@0"` represents the `A/×` key of gamepad player `0`.
  * Removed the feature allowing deletion of the last `»` or `+` connected mapping key by pressing the `Backspace` key after selecting a mapping item via mouse click. Instead, mapping keys can be modified through the "Mapped Keys" editor in the mapping settings window.
* v1.3.7 (Build 20240711)

  * Added `SendText` mapping to the "Mapped Keys List." When `SendText` is selected, you can input text in the "Text" single-line editor to send the string inside `SendText()` to the current foreground window or a specified window when the original key is pressed.
  * You can now prepend the `$` character to the "Display Toggle Key" and "Mapping Toggle Key" settings. This indicates that these keys will not be intercepted and will still be sent to the application after being triggered.
  * Added a "Notification" combo box in the "General" tab. You can choose whether to display notifications on the screen for mapping state changes. Notification display positions include "Top Left," "Top Center," "Top Right," "Bottom Left," "Bottom Center," and "Bottom Right."
  * Updated to allow multiple instances of the QKeyMapper program to run simultaneously on the same system. By copying the unzipped QKeyMapper directory, each instance can maintain its own independent configuration files.
  * Split the "Mapping Toggle Key" into separate "MappingStart" and "MappingStop" keys, allowing different shortcut keys for start and stop the mapping.
  * Added `↓`, `↑`, and `⇵` prefixes to the "MappingKeyList" in the mapping settings window. These prefixes can be added before normal mapped keys to specify that the mapping sends only the key press, key release, or both key press and release simultaneously.
* v1.3.7 (Build 20240622)

  * Added "WindowPoint" mapping to the mapping list. Use `L-Alt + Left Mouse Button` to mark relative coordinates within the corresponding window for the current mapping configuration. Selecting `Mouse-L|R|M|X1|X2_WindowPoint` allows sending mouse clicks to the relative coordinates within the window. Pressing the `F8` key displays the WindowPoint coordinates for the currently mapped window. The display rules, including color, are consistent with the previously added `F9` key, which shows fullscreen coordinate points.
  * Added a "Send To Same Windows" checkbox in the "Mapping" tab. When enabled, mapped keys will be sent to all windows with the same title as the current "Title" display name, even if the windows are minimized to the taskbar and not shown in the foreground.

* v1.3.7 (Build 20240610)
  * Added a "Settings" tab, organizing all software settings into separate categorized tabs. Tabs include "General","Mapping","Virtual GamePad","Multi-Input Devices", and "Forza".
  * Enhanced "OriginalKey" and "MappingKey" editing functionality in the mapping settings dialog:
    * In the mapping settings window (opened by double-clicking an item in the mapping list), the "OriginalKey" and "MappingKey" input boxes now allow editing of the respective keys. After editing, click the "Update" button at the end of the input box or press the Enter key while the input box is focused to update the "OriginalKey" or "MappingKey." On successful update, a green-font popup message "Update Successful" will appear in the center of the window and disappear after 3 seconds. If the update fails, a red-font popup message will appear in the center of the window, indicating the reason for the failure, and will also disappear after 3 seconds.
    * Right-clicking the "OriginalKeyList" or "MappingKeyList" allows you to add the content at the current cursor position in the list to the "OriginalKey" or "MappingKey" input boxes.

* v1.3.7 (Build 20240604)
  * Double-clicking any row in the key mapping list will pop up the mapping settings dialog for that key in the center of the window. In this dialog, you can modify settings such as "Rapid Fire," "Lock," "Action on Key Release," and "Original Key Passthrough."
  * The "Rapid Fire Down" and "Rapid Fire Up" time adjustment boxes, previously located at the bottom of the key mapping list, have been moved to the mapping settings dialog. Each key mapping's "Rapid Fire Down" and "Rapid Fire Up" times can now be individually configured and saved.
  * A right-click context menu with "Show" and "Hide" options has been added to the system tray icon, allowing you to display or hide the main window. This feature is disabled while the input device list window is open.
  * A "Quit" option has been added to the system tray icon's right-click menu, enabling you to directly exit the application.
  * Drag-and-drop functionality has been added to the items in the key mapping list.
  * When you click the "Save Settings" button and successfully save the current mapping settings, a green-font popup message "Save Successful: [Setting Name]" will appear in the center of the window and gradually fade out over 3 seconds.
  * The "Auto Map and Minimize" checkbox setting has been split into two separate checkboxes: "Auto Match and Enable Mapping" and "Auto Minimize After Startup."
  * The last successfully saved settings file will be backed up in the same directory as `QKeyMapper.exe` with the filename `keymapdata_latest.ini`. If mapping data issues occur, you can try restoring by replacing `keymapdata.ini` with this file.
  * Supports executable file process names with extensions other than `.exe`.
  * A "Hold Key Sequence" checkbox has been added to the mapping settings dialog. When checked, the final set of mapped keys in the key sequence (the part after the last "»") will remain pressed while the original key is held down. When the original key is released, the release action for the final set of mapped keys will be triggered.

* v1.3.7(Build 20240519)
  * Added "Long Press" and "Double Click" time value adjustment boxes (select the type of press through the drop-down list in front of the value box). You can map keys by long pressing for a specific time (within the range of 1~9999 milliseconds), or by double clicking twice within a specific time interval.
  * The switch key for the PassThrough mode of key mapping has been changed from the "F2" key to the "F12" key.
  * Added the KeyUp_Action mode for mapping. Select a mapping in the mapping table, press the "F2" key, and the original key string will be displayed in underscore font. This mapping will not be triggered when the original key is pressed, but when the original key is released.
  * Right-click the "original key" list control, you can append the currently displayed "original key" key name to the "original combination key" edit box, which is more convenient than manually filling in or copying and filling in the combination key name.
  * After clicking the mouse to highlight and select a mapping table item, pressing the keyboard "Backspace" key can delete the last mapped key connected by "»" or "+". It will not be deleted when there is only one mapping key.
  * Fixed the support of "long press" and "double click" mapping for "continuous firing" and "locking" functions (Build 20240519).
* v1.3.7(Build 20240416)

  * Supports adding up to 4 virtual gamepads and each can be controlled independently.
    * Multi-virtual gamepad feature 1: A "spin box" has been added behind the virtual gamepad type combo box to increase or decrease the number of gamepads. The range is between 1 and 4. Click the up arrow of the spin box to increase the number of gamepads (the type of gamepad added is determined by the gamepad type selection box X360/DS4), and click the down arrow to delete the last added gamepad each time.
    * Multi-virtual gamepad feature 2: The added gamepads can be viewed and selected through the virtual gamepad list combo box. When adding a mapped virtual gamepad key, "@+ digital number 0~3" is added after the gamepad key according to the currently selected virtual gamepad, indicating which virtual gamepad key is mapped. When the virtual gamepad selection combo box is empty, no "@+ digital number" is added after the mapping, and it is mapped to the 0th virtual gamepad key by default.
  * When multiple input devices are enabled, you can use the "Filter Key" checkbox to turn on and off whether to filter out the repeated sending of keys when the same key on the same keyboard is continuously pressed. This is the same as the filter key switch function of the Windows Control Panel, but the filter key built into Windows will fail when different keys on multiple keyboards connected to the same PC are pressed at the same time. This multiple input device filter key solves this problem. The filter key setting is saved in each mapping configuration file, and different filter key switch states can be set for different application windows. The corresponding filter key settings will be loaded when the mapping configuration is automatically switched according to the foreground window.
  * When multiple input devices are enabled, the Interception driver is used to listen for keyboard and mouse input, which is a lower-level interception than the WinAPI low-level keyboard and mouse hook function, and can map keys to games that cannot intercept input with low-level keyboard and mouse hooks.
  * Added the PassThrough mode for mapping. Select a mapping in the mapping table, press the "F2" key, and the color of the original key string display will become "orange", indicating that this mapping has become PassThrough mode, that is, after the original key is pressed, the mapped key will be sent, but the original key will not be intercepted and will be triggered with the mapped key.
* v1.3.7(Build 20240330)

  * Supports multi-input device differentiation function (up to 10 different keyboard and 10 different mouse devices can be received).
    * Multi-input device function 1: The "Install Driver" and "Uninstall Driver" buttons in the multi-input device GroupBox are used to uninstall and install the multi-input device support driver (the driver needs to be restarted to take effect after installation and uninstallation).
    * Multi-input device function 2: The "Enable" checkbox in the multi-input device GroupBox will distinguish the input of different keyboard and mouse devices when checked, and will not distinguish input device operations when not checked.
    * Multi-input device function 3: The "Device List" button in the multi-input device GroupBox pops up the keyboard and mouse input device list, which will display the keyboard and mouse devices currently connected in the system, and display the "Device Description", "Hardware ID", "VID", "PID", "Manufacturer", "Product Name", "Manufacturer" information of each device. When the multi-input device is enabled, the last operated keyboard and mouse device will be highlighted in the device list.
    * Multi-input device function 4: The disable checkbox in the keyboard and mouse input device list popped up by the "Device List" button can disable keyboard and mouse devices. After checking the disable and clicking the confirm button to exit the device list, the disable takes effect and saves the disabled keyboard and mouse devices to the disable list. The next time the program starts, it will automatically load the previously saved disable device list. (When a device is disabled, any input from this device cannot be received, so use it with caution. Do not disable the input device currently in use and cannot operate)
    * Multi-input device function 5: The "Keyboard" and "Mouse" drop-down combo boxes are used in conjunction with the "Original Key" list to select the input device number of the original key mapping. According to the selected device, the "@+ digital number 0~9" will be appended after adding the original key mapping. For example: "A@0" represents the A key of the 0th keyboard, and "Mouse-L@1" represents the left key of the 1st mouse.
    * Multi-input device function 6: You can directly input the combination key of a specific input device number in the "Original Combination Key" text edit box. For example: "L-Ctrl@2+F@2" represents the combination key of the LCtrl key and F key of the 2nd keyboard, and "L-Alt+3@1" represents the combination key of the L-Alt key of any keyboard and the 3 key of the 1st keyboard.
    * Multi-input device function 7: When the multi-input device is enabled, if there are multiple mappings in the key mapping list that match the current key, the first matching mapping entry from top to bottom will take effect, and others will not. For example: There are mappings for "W@2" and "W" in the mapping list. When the W key of the 2nd keyboard is pressed, if "W@2" is on top, "W@2" will be triggered, and if "W" is on top, "W" will be triggered.

#### ※ Note: After installing the multi-device driver, do not repeatedly plug and unplug the USB device connected to the system. The keyboard/mouse device ID upper limit of the Interception driver is 10. Each time the device is plugged and unplugged, the device ID upper limit will increase by 1. Exceeding the upper limit will cause the device to be unable to detect input, and only restarting the operating system can reset it. The system enters the sleep state and then returns, which is similar to the increase in device ID caused by plugging and unplugging devices, and the problem of increasing the device ID to more than the upper limit of 10 will also occur, which needs to be avoided. It is recommended to use the multi-device driver, first connect all the input devices that need to be used, restart the system for use, and do not plug and unplug the device during use.

* v1.3.6(Build 20240320)
  * Fixed the problem that the virtual key that is locked and pressed down is not released when the key mapping stops.
  * Fixed the problem that the delay setting of the mapped key is greater than 1000 milliseconds and cannot take effect. The upper limit of the delay setting of the mapped key is increased to 9999 milliseconds.
* v1.3.6(Build 20240316)
  * The UI control overall layout changes.
  * Added support for virtual gamepad type selection (X360/DS4), if you use the settings of the previous version, it prompts "Invalid settings data loaded from INI file", after backing up keymapdata.ini in the program path, use a text editing tool to replace text in keymapdata.ini to continue using the old configuration file. Replace content: (A) -> (A/×), (B) -> (B/○), (X) -> (X/□), (Y) -> (Y/△)
* v1.3.6(Build 20240131)
  * UI control position fine-tuning, setting selection list is placed under the process name and title name text box. Added a GroupBox frame for virtual gamepad settings.
  * When the window is visible, the process list displayed on the left is automatically refreshed every 3 seconds.
  * ADD button right side "»" checkbox added key sequence can support "vJoy" prefix virtual gamepad keys.
  * "Save Settings" button can save the current window position, after hiding and then displaying the window, it will be displayed at the window position before hiding. The program starts to display according to the window position when the last saved setting is saved.
  * Use the "»" checkbox to add a key sequence to use the delay function.
  * Window title added Build Number display.
  * Added "sound effect" checkbox, after checking, start mapping and stop mapping will play sound effects.
  * Added the function of controlling the virtual gamepad left/right joystick with the mouse (Mouse2Joystick).
* v1.3.6(Build 20240125)
  * Added a fixed global mapping setting item (QKeyMapperGlobalSetting). If the global mapping is checked to automatically start mapping, after exiting from the matching window in the start mapping state, it will automatically switch to the global mapping setting and enable mapping after a few seconds. A special tray icon is added for the global mapping state.
  * The "original shortcut key" edit box has been added, and the combination keys containing Ctrl, Shift, and Alt keys can be set as original input (when the original key drop-down box is empty, the setting key in the original shortcut key edit box will be added).
  * Added sound effect when stopping key mapping.
* v1.3.6(Build 20240112)
  * Added "Delay" numeric adjustment box for mapped keys, which can appropriately increase the waiting time between pressing and releasing of combination keys.
* v1.3.6(Build 20240106)
  * Added "Lock Cursor" checkbox. When checked, when the mouse controls the virtual gamepad joystick, the mouse cursor is locked at the lower right corner of the screen (please confirm that the mapping switch combination key is available before using this function to avoid the mouse being unable to move and the key mapping cannot be turned off to restore mouse movement).
  * Added support for mouse wheel scrolling up and down mapping function.
* v1.3.6(Build 20231230)
  * Added "Delay" numeric adjustment box for mapped keys, which can appropriately increase the waiting time between pressing and releasing of combination keys.
* v1.3.6(Build 20231225)
  * Added "Lock Cursor" checkbox. When checked, when the mouse controls the virtual gamepad joystick, the mouse cursor is locked at the lower right corner of the screen (please confirm that the mapping switch combination key is available before using this function to avoid the mouse being unable to move and the key mapping cannot be turned off to restore mouse movement).
  * Added support for mouse wheel scrolling up and down mapping function.
* v1.3.6(Build 20231223)
  * Added the function of controlling the virtual gamepad left/right joystick with the mouse (Mouse2Joystick). It can be used by adding "vJoy-Mouse2LS" or "vJoy-Mouse2RS" in the mapping table. The sensitivity of the X-axis and Y-axis of the joystick controlled by the mouse is in the range of "1~1000", and the smaller the value, the more sensitive.
  * The "Auto Mapping and Minimize" button has been changed to a tri-state checkbox. When set to the middle state, the software only minimizes to the tray when it starts, and does not automatically start key mapping.
* v1.3.6(Build 20231220)
  * Added virtual gamepad function (implemented through ViGEmBus). Click "Install ViGEmBus", after the green word "ViGEmBus available" is displayed, check "Enable Virtual Gamepad", then you can map keyboard keys to "vJoy" prefixed virtual gamepad keys.
* v1.3.6(Build 20231125)
  * Added the function of mapping gamepad keys as original keys to keyboard keys. Select keys starting with "Joy" in the original key list, then select the keyboard key you want to trigger in the mapped keys.
* v1.3.5(Build 20230806)
  * The shortcut key to switch between start and stop mapping can be customized through the KeySequenceEdit widget. Click the KeySequenceEdit widget with the mouse and press the shortcut key you want to set. This custom shortcut key setting can save different values for each configuration.
* v1.3.5(Build 20230805)
  * Added a shortcut key that can directly switch between start and stop mapping in any state. Press "Ctrl + F6", no matter whether it is displayed in the foreground or the tray, you can immediately switch between start mapping and stop mapping.
* v1.3.5
  * Added Chinese interface, you can use the language switch drop-down list to switch between Chinese and English interfaces, supporting 4K/2K/1K resolution.
* v1.3.3
  * Added support for mouse side keys XButton1 and XButton2 in the key list.
* v1.3.3
  * Added "»" checkbox to the right of the ADD button for adding key sequences, for example: Ctrl + X then Ctrl + S, the effect after setting is referenced in the screenshot displayed in README.
* v1.3.2
  * In the "KeyMappingStart" (loop detection) state, if it is detected that the foreground window matches the key mapping configuration setting of the current SelectSetting, a sound effect will be played to prompt the entry into the effective state of key mapping.
* v1.3.1
  * Added support for single-instance process, only one QKeyMapper program can run at the same time, when running the second QKeyMapper program, the window of the first running instance program will be displayed in the foreground.
* v1.3.1
  * The "Auto Startup" checkbox allows the QKeyMapper key mapping program to automatically start when the Windows user logs in. With the "Auto Start Mapping" checkbox, it can automatically minimize to the system tray icon after startup, unchecking the checkbox cancels automatic startup when logging in.
* v1.3.0
  * Started to support "SaveMapData" which can save different key mapping configurations of multiple programs to the "keymapdata.ini" file. If the "Auto Start Mapping" function is configured, then in the "KeyMappingStart" state, it will automatically match and switch to the corresponding key mapping configuration based on the executable file name of the current foreground window.

---

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

---

## Screenshot

## VirScan result

![Screenshot](https://raw.githubusercontent.com/Zalafina/QKeyMapper/master/screenshot/QKeyMapper_VirScan.png)
