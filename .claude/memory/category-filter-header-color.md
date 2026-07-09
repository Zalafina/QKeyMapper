---
name: category-filter-header-color
description: Visual filter state indicator via QHeaderView section foreground color, replacing button text summary
metadata:
  type: project
---

# Category Filter: Header Color as Filter State Indicator

## Context

After removing `categoryFilterToolButton` (2026-07-09), the visual feedback for "is filtering active?" was lost. The old button showed "All" / "None" / selected items as text.

## Solution

Set `CATEGORY_COLUMN` header section's `QTableWidgetItem::setForeground()` to `CATEGORY_FILTER_ACTIVE_HEADER_COLOR` when filtering is active, or `QBrush()` (default) when showing all rows.

**Color**: `QColor(0, 168, 138)` — same as `HOTKEY_TABBAR_TEXT_COLOR` in `qkeymapper_constants.h`.

**Filter state logic** (in `updateCategoryFilterHeaderAppearance`):
- `filters.isEmpty()` → **All** selected → default color
- `filters.contains(kNoneToken)` → **None** selected → special color (was a bug: initially excluded)
- otherwise → partial selection → special color

Simple check: `const bool isFiltering = !filters.isEmpty();`

💡 Also restored: `m_CategoryFilterAllCheckBox->setText(tr("All"))` — keeps popup checkbox text in sync with current UI language.

## Related: Popup Extraction Pattern

The category filter popup was originally triggered only by `categoryFilterToolButton::clicked`. Extracted into `showCategoryFilterPopup(const QPoint &globalAnchorPos, const QRect &anchorGlobalRect)` — now reusable from:
- Button click (via simplified lambda → `showCategoryFilterPopup(btnTopRight, btnRect)`)
- Category column Header click (via `QHeaderView::sectionClicked` → `showCategoryFilterPopup(headerTopRight, sectionRect)`)

Both use "top-right" anchor alignment with the same screen-fitting fallback logic.

**Why:** Prevents UI regression when removing filter-summary button; header color is always visible when the column is shown.
**How to apply:** When removing a filter-state indicator button, consider using the associated header's appearance to convey state. Always verify all filter states (All/None/Partial) are correctly represented.
