---
name: qt-queued-connection-pointer-toctou
description: Qt::QueuedConnection with raw pointer arguments causes TOCTOU crashes when the pointed object is deleted before the slot executes
metadata:
  type: pattern
---

# Qt QueuedConnection Raw Pointer TOCTOU Pattern

**Pattern**: When a Qt signal passes a raw pointer via `Qt::QueuedConnection` across threads, the pointed-to object can be deleted by the sender thread before the queued slot executes, causing use-after-free crashes.

**Root cause**: Qt's queued connection mechanism copies the argument — for a `T*`, it copies the pointer VALUE, not the pointed-to object. If the object's lifetime is managed by the sender thread, the receiver gets a dangling pointer.

**Detection signal**: Crashes only during rapid connect/disconnect or start/stop cycles. Non-reproducible in normal-speed usage. The `joystickRemoved` path never crashes but `joystickAdded` sometimes does — look for signal signature asymmetry (one passes pointer, the other passes value).

**Why**: The sender emits the signal (pointer is valid), the event is queued, then the sender thread continues and may delete the object before the receiver thread processes the queued slot.

## Fix Options (ranked by preference)

### 1. Change signal to pass by value (best)
If the type is copyable, change `signal(T* ptr)` → `signal(const T val)`. Qt serializes the full copy, the receiver gets an independent object. Requires updating all consumers.

### 2. Copy before queuing (when can't change signal)
Use a lambda connection that copies the needed fields on the sender thread:
```cpp
QObject::connect(sender, &Sender::signalWithPointer,
    receiver, [receiver](T *ptr) {
        if (!ptr) return;
        // Copy needed data immediately (on sender thread, pointer valid)
        QMetaObject::invokeMethod(receiver, [copy = *ptr]() {
            // Use copy safely on receiver thread
        }, Qt::QueuedConnection);
    }, Qt::DirectConnection);  // Direct to copy on sender thread
```

### 3. Share ownership (use when object must stay alive)
Wrap in `QSharedPointer<T>` — Qt serializes smart pointers correctly.

## Related Bugs in This Project
- [[joystick-hotplug-toctou-crash]] — This exact pattern caused gamepad hot-plug crashes
- [[qtablewidget-context-menu-stale-pointer]] — Similar stale pointer but different root cause (event loop re-entrancy vs cross-thread queue)

**Why**: Raw pointers in signal arguments silently become dangling across `Qt::QueuedConnection` — the compiler gives no warning. **How to apply**: When reviewing cross-thread signal-slot connections, check every `T*` signal parameter. If the sender thread owns the object and the connection is `Qt::QueuedConnection`, it's a latent crash.
