---
name: loadsetting-flag-startup-dirty-guard
description: loadSetting_flag must cover entire constructor init, and connectSettingDirtySignals must be last
metadata:
  type: project
---

# loadSetting_flag Constructor Initialization Order

`loadSetting_flag` is a guard boolean that suppresses `markSaveSettingDirty()` during programmatic state changes. Its correct usage is critical to prevent false dirty-flag during startup.

## Pattern

In the constructor, the order must be:

```
1. loadSetting_flag = true;              // guard ON before any UI init
2. All UI initialization code            // under guard
3. loadSetting_flag = false;             // guard OFF
4. connectSettingDirtySignals();         // connect dirty signals LAST
5. clearSaveSettingDirty();              // clear any false positives
6. flushPendingCommonPriorityRepairAfterLoad();  // apply repairs (may re-dirty)
```

## Why

- `loadSetting_flag` must be set BEFORE any init function that may directly or indirectly call `markSaveSettingDirty()` (e.g., `initKeyMappingTabWidget()`, any function that modifies UI widgets)
- `connectSettingDirtySignals()` must be called AFTER all UI widget modifications — it connects `QSpinBox::valueChanged`, `QComboBox::currentIndexChanged`, `QCheckBox::toggled`, etc. to lambdas that call `markSaveSettingDirty()`. Any widget setter called AFTER these connections will trigger dirty marking.
- `loadSetting_flag` must remain true through ALL constructor code (including after `loadKeyMapSetting()`) because late-stage init functions like `updateVirtualGamepadListDisplay()`, `refreshAllKeyMappingTabWidget()`, etc. also modify UI widgets.

## Common pitfalls

1. Setting `loadSetting_flag = true` after some init functions have already run
2. Calling `connectSettingDirtySignals()` before all UI widget initialization is complete
3. Assuming that `loadSetting_flag` only needs to cover `loadKeyMapSetting()` — it needs to cover the ENTIRE constructor

## Related

- `markSaveSettingDirty()` at `qkeymapper.cpp:35174` — guarded by `if (loadSetting_flag || m_SaveSettingDirty) return;`
- `connectSettingDirtySignals()` at `qkeymapper.cpp:35030` — connects 6 categories of widget signals to `markSaveSettingDirty()`
- Dialog constructors use "set initial values first, then connect notifyDirty signals" — this is the safe pattern for dialog-level dirty tracking.

**Why:** Two bugs were found (2026-07-11) where `markSaveSettingDirty` was incorrectly called during startup:
1. `initKeyMappingTabWidget()` called before `loadSetting_flag = true`  
2. `updateVirtualGamepadListDisplay()` → `QSpinBox::setValue()` triggered dirty signal because `connectSettingDirtySignals()` was called too early

**How to apply:** When adding new init code to the constructor, ensure it runs under `loadSetting_flag = true` and before `connectSettingDirtySignals()`. When adding new dirty-tracking signal connections, add them inside `connectSettingDirtySignals()` (not elsewhere) so they benefit from the deferred-connection pattern.
