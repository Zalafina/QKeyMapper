---
name: qmenu-null-action-comparison-pitfall
description: QMenu::exec() returns Q_NULLPTR on dismiss; comparing with conditionally-created QAction that is also Q_NULLPTR causes false match.
metadata:
  type: reference
---

# QMenu Null Action Pointer Comparison Pitfall

## Pattern

When using `QMenu::exec()` to show a context menu with conditionally-created actions:

```cpp
QAction *optionalAction = Q_NULLPTR;
if (someCondition) {
    optionalAction = menu.addAction("Optional Item");
}

QAction *selectedAction = menu.exec(QCursor::pos());

// ❌ BUG: both can be Q_NULLPTR → false match
if (selectedAction == optionalAction) { ... }

// ✅ FIX: always guard the optional action
if (optionalAction != Q_NULLPTR && selectedAction == optionalAction) { ... }
```

## Why It Fails

- `QMenu::exec()` returns `Q_NULLPTR` when user dismisses menu without selection
- Conditionally-created `QAction*` stays `Q_NULLPTR` when the condition is false
- `Q_NULLPTR == Q_NULLPTR` evaluates to `true`, causing the branch to execute incorrectly

## QKeyMapper Instance

File: [qkeymapper.cpp:15054](QKeyMapper/qkeymapper.cpp#L15054)

```cpp
QAction *syncGroupMoveAction = Q_NULLPTR;                        // stays Q_NULLPTR when SyncGroupId == 0
if (keymapdata.FloatingButton_SyncGroupId > 0) {
    syncGroupMoveAction = contextMenu.addAction(tr("Group synchronized move"));
}
// ...
QAction *selectedAction = contextMenu.exec(QCursor::pos());      // Q_NULLPTR on dismiss
// ...
else if (selectedAction == syncGroupMoveAction) {                // ← BUG: both Q_NULLPTR → true
    armFloatingButtonSyncGroupMoveState(sourceKey);
}
```

Caused `armFloatingButtonSyncGroupMoveState` to fire when any ungrouped button's context menu was dismissed. Also exposed latent bug with [[syncgroupid-zero-sentinel-guard]].

**Why:** QMenu::exec() returns null on dismiss; conditionally-created QAction may also be null. Add when: any new conditional QMenu::exec() usage.
