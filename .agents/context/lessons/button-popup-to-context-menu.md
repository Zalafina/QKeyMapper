---
name: button-popup-to-context-menu
description: Migrating a button-triggered popup (ActionPopup/QMenu) to a right-click context menu submenu
metadata:
  type: project
---

# Button Popup → Context Menu Migration

## Context

The `macroListBackupButton` (QPushButton) showed an `ActionPopup` with Export/Import actions on click. Needed to move to the QTableWidget right-click context menu to prepare for button removal from .ui.

This differs from [[ui-button-to-menu-migration]] — that covers **toggle** buttons (checked/unchecked state). This covers **popup** buttons (click → show popup → user picks action).

## Pattern

1. **The popup's actions already dispatch to standalone functions** (`exportMacroListToFile()`, `importMacroListFromFile()`). Verify these exist — if the slot body is inline, extract it first.

2. **Add a submenu in contextMenuEvent** — connect QActions directly to the standalone functions, bypassing the popup entirely:
   ```cpp
   QMenu *backupMenu = contextMenu.addMenu(tr("Macro List Backup"));
   QAction *exportAction = backupMenu->addAction(tr("Macro List Export"));
   connect(exportAction, &QAction::triggered, dialog, &DialogClass::exportFunc);
   ```

3. **Consider empty-area clicks** — if the context menu previously showed nothing on empty area right-click, the new group should be added unconditionally so the menu always has at least that group.

4. **Delete**: button `.ui` refs (setFont, setText, setGeometry), popup creation function (ActionPopup init), popup action slot (the dispatch function), and popup member variable.

5. **Cleanup**: remove forward declarations (e.g., `class ActionPopup;`) if no longer referenced, remove popup member from header.

## Key differences from toggle button migration

| Aspect | Toggle Button | Popup Button |
|--------|--------------|--------------|
| State | `m_ShowXxx` bool + QAction checkable | No state needed |
| Menu integration | Top menu + context menu QAction | Context menu submenu only |
| Slot extraction | Extracted from `on_xxx_toggled` body | Already standalone functions |
| Net result | Fewer files changed | Fewer files changed |

**Why:** Extends the button migration pattern family; the popup variant is simpler since there's no state to manage.
**How to apply:** When a button only shows a popup (no toggle state), check if the popup actions already dispatch to standalone functions — if so, skip the ActionPopup entirely and wire QActions directly.
