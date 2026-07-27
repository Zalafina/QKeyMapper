---
name: joystick-hotplug-toctou-crash
description: Fixed joystick rapid hot-plug crash by changing joystickAdded signal from raw pointer to value copy + callback-based virtual gamepad detection
metadata:
  type: project
---

# Joystick Rapid Hot-Plug Crash Fix (2026-07-17)

## Problem
Rapidly connecting/disconnecting gamepads caused low-probability crashes. Root cause: `SDL_Joysticks::joystickAdded(QJoystickDevice*)` passed a raw pointer via `Qt::QueuedConnection` to `QKeyMapper_Worker::onJoystickAdded` running on a different thread. The `QJoystickDevice*` could be deleted by the main thread before the worker thread processed the queued slot.

## Solution
Changed `joystickAdded` signal signature from `QJoystickDevice*` to `const QJoystickDevice` (value copy), matching the already-safe `joystickRemoved` signal. Added a `std::function<bool(uint16_t, uint16_t, const QString&)>` callback to `SDL_Joysticks` so virtual gamepad detection (X360 ViGEm, DS4 ViGEm) happens on the main thread BEFORE the signal is emitted, setting `blacklisted = true` safely.

## Files Changed (6 files, +46/-56 lines)
| File | Change |
|------|--------|
| `SDL_Joysticks.h` | Signal `*` → value; added `VirtualGamepadDetector` callback type + setter + member |
| `SDL_Joysticks.cpp` | Callback before emit; `emit joystickAdded(*joystick)` |
| `QJoysticks.h` | Signal + slot signatures synced |
| `QJoysticks.cpp` | `onJoystickAdded` slot signature synced |
| `qkeymapper_worker.h` | Slot declaration synced |
| `qkeymapper_worker.cpp` | Registered callback in constructor; simplified `onJoystickAdded` (removed null check, blacklisting logic, pointer derefs) |

## Key Design Decisions
- Did NOT fix `QJoysticks::updateInterfaces()` UAF (library internal, separate issue, low trigger probability)
- Used `std::function` callback instead of modifying library core logic for virtual gamepad detection
- Kept debug logging in `onJoystickAdded` but reading from the safe value copy (now includes `blacklisted` field for verification)
- New-style connect syntax auto-adjusted to signature change — no manual connect statement updates needed

## Related Patterns
- [[qt-queued-connection-pointer-toctou]] — Generic pattern for this class of bug

**Why**: The `joystickAdded`/`joystickRemoved` signal signature asymmetry (pointer vs value) was the root cause — this is a code smell worth checking for in other cross-thread signal connections. **How to apply**: When reviewing Qt cross-thread signals, check that every `T*` parameter's lifetime extends past the queued delivery window. Prefer value types or `QSharedPointer<T>`.
