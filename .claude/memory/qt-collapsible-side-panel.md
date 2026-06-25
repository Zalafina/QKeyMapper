---
name: qt-collapsible-side-panel
description: Pattern for a collapsible right-side button panel that extends the window width
metadata:
  type: project
  source: session-2026-06-24
---

## Qt Collapsible Side Panel Pattern

When adding a toggleable side panel to a QDialog with absolute positioning:

### Design: Window Extension, NOT Content Shrinking

A child `QFrame` cannot paint beyond the parent dialog's bounds. To make a panel appear to "extend" the window:

- **Do**: `resize(width + panelWidth, height)` on expand — window grows, content stays same
- **Do NOT**: Just position a child QFrame at the right edge — it clips inside the window and shrinks content area

### Critical: `setMinimumWidth` / `resize()` ordering

Qt's `setMinimumWidth` triggers auto-resize if the current width is below the new minimum. When both `setMinimumWidth` and `resize()` are called in a toggle handler, ORDER matters:

| Direction | Step 1 | Step 2 | Why |
|-----------|--------|--------|-----|
| **Expand** | `resize(+90)` | `setMinWidth(1160)` | Width >= old min, resize safe; then lock min |
| **Collapse** | `setMinWidth(1070)` | `resize(-90)` | Lower min first or Qt clamps resize to old min → window won't shrink |

**Bug experienced**: `setMinWidth(1160)` while window=1070 → Qt auto-resizes to 1160, then `resize(1160+90)` → double expansion to 1250 (+180).

### Architecture

```
toggle handler:
  m_rightPanelExpanded = checked
  if expand: resize(+90) then setMinWidth(1160)
  if collapse: setMinWidth(1070) then resize(-90)
  → resizeEvent → applyResizeLayout(dw, dh)

applyResizeLayout:
  panelWidth = expanded ? 90 : 0
  contentWidth = windowWidth - panelWidth
  contentDw = max(0, dw - panelWidth)
  boundaryShift = contentDw / 2
  → all table/control widths use contentDw, not dw
```

### UI: Toggle button in .ui file

- Checkable QPushButton in .ui (~`x=990, y=35, 71x25`)
- Text convention: show the ACTION (" >>" expands, "<<" collapses)
- Reparent 8 right-column buttons into panel QVBoxLayout after `setupUi()`
- Panel needs `setAutoFillBackground(true)` + stylesheet with `background-color: palette(window)`

### Related
- [[qt-no-layout-resize-pattern]] — the base `applyResizeLayout(dw, dh)` pattern
- [[window-size-persistence]] — INI persistence for settings like panel state
