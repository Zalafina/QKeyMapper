---
name: settingtabwidget-internal-resize
description: How to handle QTabWidget width changes when internal pages use absolute positioning
metadata:
  type: project
  source: session-2026-06-23
---

## QTabWidget Width Change with Absolute-Positioned Internal Pages

When `settingTabWidget` width increases, its internal tab pages also get wider. Widgets inside those pages need explicit geometry updates since they use absolute positioning.

### Strategy

1. **Targeted stretching**: Only stretch widgets that benefit from extra width (LineEdits). Leave buttons and labels at fixed positions.

2. **Container-only for other tabs**: For tabs where no internal widgets need stretching (General, Mapping, VirtualGamepad, etc.), just expanding the tab widget container is sufficient. Internal widgets keep their left-relative X positions automatically.

### WindowInfo tab specifics

The WindowInfo tab has 4 LineEdits (processLineEdit, windowTitleLineEdit, classNameLineEdit, descriptionLineEdit) at x=153, original width 281. New width = `281 + boundaryShift`.

Buttons (selectSettingCustomIconButton, ignoreRulesListButton) keep original positions relative to the left edge.

The restoreProcessPathButton must follow processLineEdit's right edge: `x = 440 + boundaryShift` (original gap: 6px).

### Caution

- Widget coordinates inside tab pages are page-relative, NOT dialog-relative
- `ui->processLineEdit->setGeometry(x, y, w, h)` uses page coordinates
- Expanding `settingTabWidget` geometry on the dialog auto-expands the tab page area

### Related
- [[qt-no-layout-resize-pattern]]
