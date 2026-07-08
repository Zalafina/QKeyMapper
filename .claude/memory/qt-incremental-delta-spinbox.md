---
name: qt-incremental-delta-spinbox
description: QSpinBox valueChanged fires per-step; use incremental delta (track last-applied value), never cumulative delta from a backup snapshot.
metadata:
  type: project
  date: 2026-07-08
---

# Qt QSpinBox: Incremental Delta Pattern

## Problem

`QSpinBox::valueChanged(int)` fires on every step when the user clicks the arrow buttons. If you compute a **cumulative delta** from a backup/snapshot (e.g., `currentValue - backupValue`), the delta stacks incorrectly on each step.

**Example**: User clicks down-arrow 3 times (199→198→197→196):

| Step | Value | Cumulative delta (current - backup=199) | Applied to already-modified value | Actual effect |
|------|-------|----------------------------------------|-----------------------------------|---------------|
| 1 | 198 | -1 | 243-1=242 | ✓ expected -1 |
| 2 | 197 | -2 | 242-2=240 | ✗ expected -1, got -2 |
| 3 | 196 | -3 | 240-3=237 | ✗ expected -1, got -3 |

The cumulative -3 on step 3 produces -7 total instead of the expected -3.

## Solution

Track the **last-applied value** and use **incremental delta**:

```cpp
// In constructor / loadFromCurrentItem:
m_LastAppliedOffsetX = keymapdata.FloatingButton_X_Offset;
m_LastAppliedOffsetY = keymapdata.FloatingButton_Y_Offset;

// In valueChanged handler:
const int deltaX = m_OffsetXSpinBox->value() - m_LastAppliedOffsetX;
const int deltaY = m_OffsetYSpinBox->value() - m_LastAppliedOffsetY;
if (deltaX == 0 && deltaY == 0) return;

m_LastAppliedOffsetX = m_OffsetXSpinBox->value();
m_LastAppliedOffsetY = m_OffsetYSpinBox->value();

// Apply deltaX/Y to group members...
```

| Step | Value | Incremental delta | Applied | Result |
|------|-------|-------------------|---------|--------|
| 1 | 198 | 198-199=-1 | 243-1=242 | ✓ |
| 2 | 197 | 197-198=-1 | 242-1=241 | ✓ |
| 3 | 196 | 196-197=-1 | 241-1=240 | ✓ |

## When to use cumulative delta

Cumulative delta from backup is correct ONLY when the signal fires once (e.g., user types a value directly and presses Enter). But `valueChanged` fires per-step, so incremental is the safe default.

## Related

- [[floating-button-sync-group-session]] — original bug discovery context
