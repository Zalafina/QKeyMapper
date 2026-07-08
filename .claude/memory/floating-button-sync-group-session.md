---
name: floating-button-sync-group-session
description: Full session record of implementing Floating Button Sync Group feature in QKeyMapper — 6 files modified, 5 bugs fixed, key design decisions and lessons learned.
metadata:
  type: project
  date: 2026-07-08
---

# Floating Button Sync Group — Session Record

## Feature Summary

Added synchronized movement for floating button groups. Buttons sharing the same `FloatingButton_SyncGroupId` move together when dragged (Alt+drag or context menu) or when X/Y offsets are adjusted in the setup dialog. Includes common (共通) mapping tab bidirectional support and Group Note feature.

## Files Modified (6)

| File | Changes |
|------|---------|
| `qkeymapper_constants.h` | Added `KEYMAPDATA_FLOATINGBUTTON_SYNCGROUPID`, range constants, `SYNCGROUPNOTES` serialization keys |
| `qkeymapper_worker.h` | Added `FloatingButton_SyncGroupId` field to `MAP_KEYDATA` |
| `QKeyMapper.h` | Added armed key, drag hashes ×2, notes hash, 9 method declarations + 2 accessors |
| `qkeymapper.cpp` | Serialization (2 paths × 2 keys), drag sync (press/move/release/cleanup), context menu, runtime sync, armed state methods, sync group notes save/load |
| `qfloatingbuttonsetupdialog.h` | Added 6 control members + 3 method declarations |
| `qfloatingbuttonsetupdialog.cpp` | UI layout, load/save, SpinBox real-time delta sync, Group Note read/write, common tab helpers |

## Bugs Fixed (5)

1. **Double-move with SpinBox arrows** — cumulative delta from backup stacked on each valueChanged step. Fixed with [[qt-incremental-delta-spinbox]] pattern.
2. **Common tab unidirectional sync** — `shouldAppendCommonMappingRows` only checked regular→common. Fixed with [[bidirectional-cross-tab-sync]] pattern.
3. **Common tab button delayed ~1s** — QueuedConnection `settingsApplied` handler triggered heavy `refreshTabsForSourceTabChange`. Fixed by adding synchronous `refreshFloatingButtonPositionForSource` for current button.
4. **SaveSettingDirty on dialog open** — `syncGroupNoteFromData()` set `QLineEdit` text during `loadFromCurrentItem`, triggering `textChanged→notifySaveSettingDirty`. Fixed with `m_isLoading` guard.
5. **Button count off by 1** — `updateGroupMemberCountLabel` ran before `applyToCurrentItem`, so current row's SyncGroupId was stale. Fixed by adding 1 if current row's SyncGroupId != target groupId.

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| `SyncGroupId` as int (0=no group) | Simplest serialization, O(1) lookup, follows existing int field patterns |
| Opt-in sync via checkbox + context menu | User explicitly requested; avoids unexpected group movement |
| Alt+drag parallel to Ctrl+drag | Follows existing UX pattern, intuitive |
| Group Notes via QVariantList (same as MacroList) | Reuses proven [[qvariantmap-saveload-pattern]], QHash for O(1) GroupId lookup |
| Bidirectional common tab sync via `s_KeyMappingTabWidgetCurrentIndex` | Current display tab represents what user actually sees |

## Other Lessons

- **Qt signal order races**: In dialogs, `valueChanged`/`textChanged` signals may fire before `applyToCurrentItem` updates backing data. Always check with `m_isLoading` guard during init, and account for pending changes when reading from data model in signal handlers.
- **QueuedConnection + heavy handler = visible lag**: When a `Qt::QueuedConnection` handler triggers expensive operations, supplement with synchronous UI updates for the visible widget. The data model was already updated, so the synchronous move is safe.
- **Follow existing patterns exactly**: Armed state helpers, `QCoreApplication::translate` for translatable text, QVariantList serialization — all reused from existing code. Before adding anything new, find the closest existing pattern and trace all its code paths.
