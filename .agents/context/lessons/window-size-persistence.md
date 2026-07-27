---
name: window-size-persistence
description: Pattern for persisting window size/position in INI settings with conditional restore
metadata:
  type: project
  source: session-2026-06-24
---

## Window Size Persistence via INI Settings

Pattern for saving QDialog window size to QSettings INI file, with a user-facing checkbox to control whether size is restored on next launch.

### Flow

1. **Save**: In `saveKeyMapSetting()`, always save `size()` alongside `pos()`. Also save checkbox state.
   - `settingFile.setValue(LAST_WINDOW_SIZE, size())`
   - `settingFile.setValue(SAVE_WINDOW_SIZE, checkboxValue)`
   - Do NOT call `markSaveSettingDirty()` on resize — too frequent.

2. **Load**: In `loadKeyMapSetting()`, restore size only if checkbox is checked AND saved size >= minimum.
   - `if (SAVE_WINDOW_SIZE && contains(LAST_WINDOW_SIZE) && size >= min) resize(savedSize)`

3. **Dialog state**: In both `loadKeyMapSetting()` and `loadGeneralSetting()`, restore checkbox UI state.
   - `m_GeneralAdvancedDialog->setSaveWindowSize(value)`

### UI: Checkbox in GeneralAdvancedDialog

- Place in `startupPositionGroupBox` with left spacer (150px fixed, like `soundEffectCheckBox` pattern)
- Default unchecked
- Window height increased by ~30px to accommodate new row
- Must add `tr("Save Window Size")` to `setUILanguage()` for i18n

### Constants needed
```cpp
inline constexpr const char SAVE_WINDOW_SIZE[] = "SaveWindowSize";
inline constexpr const char LAST_WINDOW_SIZE[] = "LastWindowSize";
```

### Related files
- `qkeymapper_constants.h` — SAVE_WINDOW_SIZE, LAST_WINDOW_SIZE
- `qgeneraladvanceddialog.ui` — saveWindowSizeCheckBox
- `qgeneraladvanceddialog.h/.cpp` — getSaveWindowSize/setSaveWindowSize
- `qkeymapper.cpp` — saveKeyMapSetting, loadKeyMapSetting, loadGeneralSetting
