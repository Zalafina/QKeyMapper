---
name: sectionpressed-vs-sectionclicked-qmenu-toggle
description: Why sectionPressed works for QMenu toggle on header click but sectionClicked doesn't
metadata:
  type: project
---

# sectionPressed vs sectionClicked: QMenu Toggle Timing

## Problem

Clicking a `QHeaderView` section to toggle a `QMenu` popup (show if hidden, close if visible) does NOT work with `QHeaderView::sectionClicked`.

**Why**: `QMenu::popup()` creates a popup window. When the user clicks outside the menu (e.g., on the table header), Qt auto-closes the menu on **mouse-press**. `sectionClicked` fires on **mouse-release** — by then, `QMenu::isVisible()` is already `false`, so the toggle check always sees a closed menu and re-opens it.

Sequence:
```
mouse-press → QMenu detects outside click → auto-close → isVisible() = false
mouse-release → sectionClicked fires → isVisible() == false → opens popup (WRONG!)
```

## Failed Approaches

### approach 1: `isVisible()` in sectionClicked handler
Fails because menu auto-closes on mouse-press before handler runs.

### approach 2: `m_CategoryFilterMenuOpen` flag + `aboutToHide` signal
`aboutToHide` fires synchronously during auto-close on mouse-press. The flag is cleared before `sectionClicked` fires. Same timing problem.

### approach 3: `QTimer::singleShot(0, ...)` to defer flag clearing
Race condition: if user holds mouse button down, the timer may fire before mouse-release, clearing the flag prematurely.

## Solution

Use `QHeaderView::sectionPressed` instead of `sectionClicked`.

`sectionPressed` is emitted **synchronously** from `QHeaderView::mousePressEvent()`, BEFORE Qt's popup auto-close mechanism runs. At that point `QMenu::isVisible()` is still accurate.

```cpp
// ✅ Works: sectionPressed fires before QMenu auto-close
connect(table->horizontalHeader(), &QHeaderView::sectionPressed,
        this, &QKeyMapper::onCategoryColumnHeaderClicked);

void onCategoryColumnHeaderClicked(int logicalIndex)
{
    if (logicalIndex != TARGET_COLUMN || !m_Menu) return;
    if (m_Menu->isVisible()) {
        m_Menu->close();   // toggle: close the open menu
        return;
    }
    // ... show popup
}
```

No flags, no timers, no `aboutToHide` — just `isVisible()` directly.

## Applies To

Any scenario where a `QHeaderView` header click needs to toggle a `QMenu` (or any popup that auto-closes on outside click).

**Why:** Non-obvious Qt event ordering. Three failed attempts before finding the right signal.
**How to apply:** Always use `sectionPressed` (not `sectionClicked`) when the handler needs to check popup visibility state.
