---
name: first-time-prompt-decided-flag
description: Use a separate "UserDecided" INI flag for first-time prompts instead of checking if the setting key exists
metadata:
  type: project
  category: ui-pattern
---

## Pattern: First-Time Prompt with Separate "Decided" Flag

### Problem

When a feature has a first-time prompt (e.g., "What should happen when you close the window?"), using `settingFile.contains(KEY)` to gate the prompt is fragile. The setting key may be written early by "保存设定" (Save Settings), which writes all UI control values including the new CheckBox (default unchecked = false). The real first close then never triggers the prompt.

### Solution

Use a separate `_Decided` flag:

```cpp
// constants
inline constexpr const char CLOSETO_SYSTEMTRAY[] = "CloseToSystemtray";
inline constexpr const char CLOSETO_SYSTEMTRAY_DECIDED[] = "CloseToSystemtray_Decided";
```

```cpp
bool isCloseToSystemtray()
{
    bool closeto_systemtray = false;
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);

    // Always read the setting value
    if (settingFile.contains(CLOSETO_SYSTEMTRAY))
        closeto_systemtray = settingFile.value(CLOSETO_SYSTEMTRAY).toBool();

    // Gate on "decided", not on "key exists"
    bool user_decided = false;
    if (settingFile.contains(CLOSETO_SYSTEMTRAY_DECIDED))
        user_decided = settingFile.value(CLOSETO_SYSTEMTRAY_DECIDED).toBool();

    if (!user_decided) {
        // Show first-time dialog
        // ... user makes choice ...
        settingFile.setValue(CLOSETO_SYSTEMTRAY, closeto_systemtray);
        settingFile.setValue(CLOSETO_SYSTEMTRAY_DECIDED, true);
        ui->checkBox->setChecked(closeto_systemtray);  // sync UI
    }

    return closeto_systemtray;
}
```

Set `_Decided = true` from **two** sources:
1. First-time MessageBox button click (in the function itself)
2. CheckBox `clicked` signal — user actively toggles the setting in preferences

```cpp
// Use QCheckBox::clicked (NOT toggled) — clicked only fires on user interaction
QObject::connect(ui->checkBox, &QCheckBox::clicked, this, [this]() {
    QSettings settingFile(CONFIG_FILENAME, QSettings::IniFormat);
    settingFile.setValue(CLOSETO_SYSTEMTRAY_DECIDED, true);
});
```

### Why `QCheckBox::clicked`, not `toggled`

| Signal | Fires on `setChecked()` | Fires on user click |
|--------|------------------------|---------------------|
| `toggled` | ✅ Yes | ✅ Yes |
| `clicked` | ❌ No | ✅ Yes |

`connectCheckable` uses `toggled` for dirty-state tracking (wants to catch all changes). The "decided" flag needs `clicked` to exclude programmatic `setChecked()` during settings load.

### Edge case handled

| Scenario | Result |
|----------|--------|
| First run, close immediately | ✅ Prompt shown |
| First run, save settings, then close | ✅ Prompt shown (decided flag not set) |
| First run, toggle CheckBox, then close | ✅ No prompt (user already decided) |
| First run, close → choose in dialog → close again | ✅ No prompt (decided flag set) |
| Non-first run, close | ✅ No prompt (reads decided flag from INI) |

**Why:** "Save Settings" writes all control values to INI including unchecked CheckBox → `CloseToSystemtray=false`. Without the separate flag, checking `settingFile.contains(CLOSETO_SYSTEMTRAY)` would see the key exists and skip the prompt. The decided flag is only set by deliberate user action.
