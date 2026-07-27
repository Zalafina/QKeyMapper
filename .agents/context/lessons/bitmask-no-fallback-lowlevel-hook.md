---
name: bitmask-no-fallback-lowlevel-hook
description: When device index is unknown in LowLevel hooks, per-device bitmask must be ignored — no fallback to "mask != 0"
metadata:
  type: reference
  tags: [hook, interception, bitmask, lowlevel, blocking]
---

## Problem

In LowLevel hook blocking checks, when `extraInfo` lacks Interception device info (no driver installed, or non-Interception path), the device index is -1. A fallback like `(device_index >= 0 ? checkBit(mask, idx) : (mask != 0))` causes per-device bitmask bits to unpredictably block ALL input.

## Correct pattern

```cpp
// ❌ Wrong — any bit set blocks all when device unknown
(device_index >= 0 ? isBitSet(mask, device_index) : (mask != 0))

// ✅ Correct — ignore bitmask when device unknown
(device_index >= 0 && isBitSet(mask, device_index))
```

Only `s_BlockKeyboard` (global QAtomicBool) should control blocking when device identity is unavailable.

## Context

Used in [[block-input-device-index-session]]. The Interception worker path (interception_worker.cpp) always has device index and does NOT need this guard.
