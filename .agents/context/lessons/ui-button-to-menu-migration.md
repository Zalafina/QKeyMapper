---
name: ui-button-to-menu-migration
description: Pattern for migrating .ui QPushButton/QToolButton dependencies to menu actions + member state variables
metadata:
  type: project
---

# .ui Button → Menu + State Member Migration

## Problem

`.ui`-defined buttons (`QPushButton`, `QToolButton`) use auto-connected slots (`on_xxx_toggled`) and direct `ui->xxx->isChecked()/setChecked()` access spread across the codebase. This makes the button hard to remove from the `.ui` file — its logic is scattered across ~20 call sites.

## Pattern (参照: `m_ShowFloating`)

In QKeyMapper, `processListButton`, `showNotesButton`, `hideDisabledButton`, and `showFloatingButton` were previously migrated. `showCategoryButton` was the last, migrated 2026-07-09.

### Step-by-step:

1. **Add state member + getter + setter** in `QKeyMapper.h`:
   ```cpp
   bool m_ShowXxx = false;                          // near m_ShowFloating
   bool isShowXxxEnabled() const { return m_ShowXxx; }  // inline getter
   void setXxxVisible(bool visible);                     // setter declaration
   QAction *m_ActionShowXxx = Q_NULLPTR;                 // menu action
   ```

2. **Implement setter** — extract from `on_showXxxButton_toggled` body:
   - Set `m_ShowXxx = visible`
   - Sync the button: `if (ui->xxxButton && ui->xxxButton->isChecked() != visible) ui->xxxButton->setChecked(visible);`
   - Apply the state change (table column visibility, filter updates, etc.)
   - ⚠️ Be aware of signal recursion: button `setChecked` → `toggled` → auto-slot → calls setter again. Solution: keep the auto-slot as a one-line delegation, OR remove `setChecked` from setter and let callers sync manually, OR use guard bool.

3. **Replace all `ui->xxxButton->isChecked()`** → `m_ShowXxx` (~20 locations)
4. **Replace all `ui->xxxButton->setChecked(state)`** → `setXxxVisible(state)` 
5. **Keep** button UI ops (setStyle, setFont, setEnabled, setGeometry, setText) until button deletion

6. **Add menu items** (in constructor, line ~3174):
   - Top menu `m_MenuMappingTableView`: addAction before existing items, connect toggled → setter
   - `aboutToShow` sync: add `m_ActionShowXxx->setChecked(m_ShowXxx)`
   - Context menu `viewMenu`: addAction with same pattern (local QAction, rebuilt each time)
   - `retranslateUi`: add `m_ActionShowXxx->setText(tr("..."))`

7. **After verification** — delete button refs from .cpp, delete auto-slot from .h/.cpp, delete button from .ui

## Common pitfalls

- **Signal recursion**: `setChecked(visible)` in setter → auto-slot → setter again. Idempotent but wasteful. Use guard bool or sync only when state differs.
- **Missing `markSaveSettingDirty`**: Menu actions don't auto-mark dirty like `connectCheckable` did. Ensure settings save path reads `m_ShowXxx` directly.
- **all-or-nothing migration**: If `isChecked()` reads are partially replaced, stale button references cause compile errors after .ui widget deletion.

**Why:** Standardizes on a proven pattern; enables .ui cleanup without breaking menu/context-menu functionality.
**How to apply:** Follow this exact sequence for any future .ui button removal.
