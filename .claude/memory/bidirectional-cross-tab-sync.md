---
name: bidirectional-cross-tab-sync
description: When syncing data across tabs, always verify both directions. A unidirectional check (A→B) must also handle the reverse (B→A) using the current display context.
metadata:
  type: project
  date: 2026-07-08
---

# Bidirectional Cross-Tab Sync

## Pattern

When synchronizing data between two tabs where one is conditionally appended to the other (e.g., QKeyMapper's "共通映射表" appended to regular mapping tabs), the sync check must be **bidirectional**:

```cpp
const bool sourceIsCommon = isCommonMappingTabIndex(sourceTabIndex);

if (!sourceIsCommon && shouldIncludeCommonMappingRows(sourceTabIndex)) {
    // Direction A: regular tab → also sync common tab
    syncToTab(commonTabIndex);
} else if (sourceIsCommon) {
    // Direction B: common tab → also sync the regular display tab
    const int displayTab = s_KeyMappingTabWidgetCurrentIndex;
    if (shouldIncludeCommonMappingRows(displayTab)) {
        syncToTab(displayTab);
    }
}
```

## Why unidirectional fails

The original code only checked `shouldAppendCommonMappingRows(sourceTabIndex)`, which works for regular→common but fails for common→regular because `isCommonMappingTab()` returns true for the common tab, causing `shouldAppendCommonMappingRows` to return false.

**Result**: Operating from a common tab button only syncs common tab members, missing the regular tab members that share the same GroupId and are visible on the same screen.

## Key insight

Use `s_KeyMappingTabWidgetCurrentIndex` (the current display tab) as the **display context** to find which regular tab is currently showing the common rows. This represents what the user actually sees on screen.

## Guard conditions

- `isCommonMappingFeatureEnabled()` — global feature flag
- `!isCommonMappingTab(tabInfo)` — don't append common to itself
- `tabInfo.IncludeCommonMappingTable` — per-tab checkbox
- `otherTabIndex != sourceTabIndex` — don't double-scan

## Related

- [[floating-button-sync-group-session]] — original bug discovery context
