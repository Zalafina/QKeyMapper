---
name: qt-tabbar-settabvisible-bug
description: Qt 6 QTabBar::setTabVisible(false) scroll button calculation bug and the removeTab/insertTab workaround
metadata:
  type: project
  source: session-2026-07-01
---

## Qt 6 QTabBar setTabVisible Scroll Button Bug

QTabBar::setTabVisible(false) on Qt 6 has a bug where scroll button states are incorrectly calculated after hiding a tab. All workarounds that try to force QTabBar to relayout (resize widget, resize tab bar, temporary tab switch) FAIL because the underlying layout calculation is broken after setTabVisible.

### Root cause

The internal QTabBar layout logic that computes whether scroll (arrow) buttons are needed does not properly account for tabs hidden via setTabVisible(false). After hiding a tab, the scroll button state calculation can get stuck with incorrect enabled/disabled states, making some tabs inaccessible.

### Solution: removeTab/insertTab workaround

Instead of `setTabVisible(false)`, physically remove the tab with `QTabWidget::removeTab()` and re-insert it later with `insertTab()`. The removeTab path uses a different, well-tested Qt code path that correctly recalculates layout and scroll buttons.

```cpp
// BAD (broken on Qt 6):
tabBar->setTabVisible(index, false);

// GOOD:
tabWidget->removeTab(index);          // physically remove
// ... later ...
tabWidget->insertTab(index, widget, text);  // re-insert
```

### Data preservation pattern

Since removeTab deletes the tab page widget and its associated data, any data that needs to survive the detach must be stored separately.

1. **Dedicated storage**: A member variable (e.g., `QList<MAP_KEYDATA> m_CommonMappingData`) holds the data independently of the tab's existence.

2. **Sync on detach**: Before removeTab, copy data from the tab's data pointer to the dedicated storage:
   ```cpp
   if (commonData) {
       m_CommonMappingData = *commonData;
   }
   ```

3. **Sync on save**: Before serializing, copy from the live tab data to dedicated storage (in case the tab is attached at save time):
   ```cpp
   void syncCommonMappingDataForSave() {
       const int commonIndex = findCommonMappingTabIndex();
       if (commonIndex >= 0) {
           QList<MAP_KEYDATA> *commonData = ...;
           if (commonData) m_CommonMappingData = *commonData;
       }
   }
   ```

4. **Restore on attach**: After re-creating the tab via addTab, restore data back:
   ```cpp
   if (!m_CommonMappingData.isEmpty()) {
       *commonData = m_CommonMappingData;
   }
   ```

### Failed attempts (do not try)

| Attempt | Why it fails |
|---------|-------------|
| Resize QTabWidget after setTabVisible | Triggers main window's resizeEvent -> applyResizeLayout -> catastrophic side effects (widget reposition, layout skew) |
| Resize QTabBar directly | Also breaks scroll state; disables previously-enabled arrow buttons |
| setCurrentIndex + setUpdatesEnabled(false) | Hides flicker but makeVisible() is also broken after setTabVisible |
| setTabVisible(true) on another tab first | Doesn't fix the underlying broken layout calculation |

### Reordering tabs with removeTab/insertTab

Same pattern works for reordering tabs (e.g., moving a tab to the last position):

```cpp
// Save all tab metadata before removal
QWidget *widget = tabWidget->widget(oldIndex);
QString text = tabWidget->tabText(oldIndex);
QIcon icon = tabWidget->tabIcon(oldIndex);
// ... save tooltip, enabled, visible, colors, data ...

tabWidget->blockSignals(true);
tabWidget->removeTab(oldIndex);
tabWidget->insertTab(newIndex, widget, icon, text);
// ... restore tooltip, enabled, visible, colors, data ...
tabWidget->blockSignals(false);
```

Always save ALL tab properties before removeTab, as insertTab only restores widget + icon + text.

### Related files
- `QKeyMapper/qkeymapper_qt_compat.h` — contains the broken `tabBarSetTabVisible` wrapper
- `QKeyMapper/qkeymapper.cpp` — `detachCommonMappingTab()`, `attachCommonMappingTab()`, `ensureCommonMappingTabIsLast()`
- `QKeyMapper/qkeymapper.h` — `m_CommonMappingData` member variable
