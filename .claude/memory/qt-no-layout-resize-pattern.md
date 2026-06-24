---
name: qt-no-layout-resize-pattern
description: Pattern for adding window resize to Qt dialogs using absolute widget positioning without layout managers
metadata:
  type: project
  source: session-2026-06-23
---

## Qt Absolute-Positioning Window Resize Pattern

When a QDialog uses absolute widget positioning (no QLayout managers), making it resizable requires a central `applyResizeLayout(dw, dh)` function called from `resizeEvent`.

### Core formula

```
dw = qMax(0, width() - WINDOW_BASE_WIDTH)  // width only increases
dh = height() - WINDOW_BASE_HEIGHT          // height can go both ways
boundaryShift = dw / 2                      // proportional split, independent of widget visibility
```

### Key principles

1. **Delta-from-base**: All calculations based on `dw`/`dh` offsets from base size, not absolute pixel values. When dw=0, dh=0, layout degenerates to original.

2. **boundaryShift is a window property**: The vertical split line should be independent of any widget's visibility state. Do NOT make it conditional on widget states — this caused a bug where wide mode got `boundaryShift=0`.

3. **Bottom controls shift Y by dh**: When window height changes, ALL controls at y>=370 should shift Y by dh to maintain relative positioning. Prevents overlap when height increases and clipping when height decreases.

4. **Right-anchored controls use `width() - margin`**: Use `this->width() - widgetWidth - rightMargin` instead of `originalX + dw` for cleaner intent.

5. **Avoid cascade growth**: When column-1 ComboBoxes grow wider, column-2 labels/inputs must shift extra to avoid being covered. Simplest fix: keep ComboBox widths at original values, only shift X positions.

### Related files
- [[qkeymapper-applyResizeLayout]]
- [[window-size-persistence]]
