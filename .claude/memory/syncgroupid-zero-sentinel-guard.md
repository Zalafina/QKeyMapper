---
name: syncgroupid-zero-sentinel-guard
description: FloatingButton_SyncGroupId 0 means "not grouped" and must be guarded in group operations like snapshotSyncGroupMembers.
metadata:
  type: reference
---

# SyncGroupId 0 — Sentinel Value Must Be Guarded

## Context

`FloatingButton_SyncGroupId` defaults to `0` (`FLOATINGBUTTON_SYNCGROUPID_DEFAULT = 0`), meaning "not assigned to any group". Valid group IDs are positive integers (>0).

## Problem

In `snapshotSyncGroupMembers()` ([qkeymapper.cpp:11639](QKeyMapper/qkeymapper.cpp#L11639)), the function iterates all mapping entries and collects those matching the given `groupId`:

```cpp
for (int row = 0; row < sourceDataList->size(); ++row) {
    if (data.FloatingButton_SyncGroupId != groupId) {
        continue;  // ← groupId=0 matches ALL ungrouped buttons!
    }
    // ...add to sync move list
}
```

When `groupId == 0`, ALL buttons with default Group ID 0 match — effectively treating all ungrouped buttons as one giant group. This is incorrect: Group ID 0 is not a valid group, it's the "no group" sentinel.

## Fix

Add an early guard at function entry:

```cpp
if (groupId <= 0 || sourceDataList == Q_NULLPTR) {
    return;
}
```

The existing `altDrag` condition already guards against Group ID 0:
```cpp
const bool altDrag = (keymapdata.FloatingButton_SyncGroupId > 0) && (modifiers & Qt::AltModifier);
```

But `snapshotSyncGroupMembers` can be called from other paths (e.g., context menu → `armFloatingButtonSyncGroupMoveState`), so the defense must be inside the function itself.

## Related

- Was triggered by [[qmenu-null-action-comparison-pitfall]] which bypassed the `altDrag` guard
- See [[floating-button-sync-group-session]] for full sync group implementation context

**Why:** Group ID 0 is the default/unassigned sentinel, not a valid group. Add when: any new group operation that iterates by groupId without checking >0.
