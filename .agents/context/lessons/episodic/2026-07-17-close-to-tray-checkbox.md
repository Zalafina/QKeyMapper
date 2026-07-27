---
name: close-to-tray-checkbox-session
description: Adding "Close to System Tray" CheckBox to General tab, simplifying closeEvent, and fixing first-time prompt timing
metadata:
  type: episodic
  date: 2026-07-17
  files:
    - QKeyMapper/qkeymapper.ui
    - QKeyMapper/qkeymapper.h
    - QKeyMapper/qkeymapper.cpp
    - QKeyMapper/qkeymapper_constants.h
---

## What happened

Added a `closeToSystemTrayCheckBox` in the General tab (y=130, x=20 below startupAutoMonitoringCheckBox).
Replaced the old Ctrl-key-gated MessageBox flow with a simpler design:

1. **Removed** Ctrl key detection in closeEvent — no more holding L-Ctrl to force the dialog.
2. **Rewrote** `isCloseToSystemtray()` signature from `(bool force_showdialog)` to `()`.
3. **Replaced** `showMessageBoxWithCheckbox` with standard `QMessageBox::question` (Minimize to Tray / Exit Directly buttons).
4. **Added** `CloseToSystemtray_Decided` flag to fix the "save settings before first close" edge case.

## Key insights

### Bug 1: CheckBox not updated after MessageBox choice
After the first-time MessageBox, the INI was saved but `closeToSystemTrayCheckBox` wasn't calling `setChecked()`. Fixed by adding `ui->closeToSystemTrayCheckBox->setChecked(closeto_systemtray)` after the INI save.

### Bug 2: "保存设定" writes the key early
The original logic used `settingFile.contains(CLOSETO_SYSTEMTRAY)` to gate the first-time dialog. But if the user clicked "保存设定" first, the key was already written (default unchecked = false), so the real first close wouldn't show the dialog.

**Root cause**: "Key exists in INI" ≠ "User has decided."

**Fix**: Added `CloseToSystemtray_Decided` flag. Set to `true` only when:
- User clicks a button in the first-time MessageBox, OR
- User clicks the CheckBox (`QCheckBox::clicked` signal — user-interaction only, not programmatic)

See [[first-time-prompt-decided-flag]] for the reusable pattern.

### Pattern followed
Used `startupAutoMonitoringCheckBox` as the template: .ui, loadKeyMapSetting, loadGeneralSetting, saveKeyMapSetting, setUILanguage, setFont×3, connectCheckable.
