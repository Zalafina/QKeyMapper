---
name: block-input-device-index-session
description: Full session of extending Block-Keyboard/Block-Mouse with @0~9 per-device blocking
metadata:
  type: project
  tags: [block-input, interception, multi-device, bitmask, hook]
---

## Summary

Extended Block-Keyboard/Block-Mouse mapping keys to support @0~9 per-device blocking suffix, following the GamepadTouchpad@N pattern.

## Key Decisions

### Two-variable state: bool + bitmask (not pure bitmask)
- `s_BlockKeyboard` (QAtomicBool) — global "block all", controlled by Block-Keyboard without @N suffix
- `s_BlockKeyboardMask` (QAtomicInt) — per-device bitmask, controlled by Block-Keyboard@N
- Reason: pure bitmask would cause state interference when both "block all" and "block @N" keys are held simultaneously

### Regex: `[0-9]` not `(\d+)`
- `[0-9]` + `$` anchor naturally rejects `@10`, `@01`, `@` (no digit)
- No need for post-validation range/leading-zero checks

### Bitmask ignored when device index unknown
- LowLevel hooks: `keyboard_index >= 0 && isBlockDeviceMaskSet(...)` — no fallback to `mask != 0`
- Without Interception, per-device bitmask has no effect; only `s_BlockKeyboard` bool matters
- Per-device blocking only works through Interception path (which has device index)

## Bugs Fixed During Implementation

### 1. `else { ... if...else if... }` breaks if-else chains
**Why:** Adding `else { declarations... if...else if... }` inside an existing `if-else if` chain steals the closing `}` from the outer block, causing C2181 "illegal else without matching if".
**Fix:** Declare variables at the same scope as the if-else chain start, use standard `else if (parseFunc(...))`.

### 2. `QLatin1String` destroys non-Latin-1 characters
**Why:** Emoji ⌨ (U+2328) and 🖱 (U+1F5B1) are outside Latin-1 range; `QLatin1String` only handles 8-bit chars.
**Fix:** Use `QStringLiteral("⌨")` / `QStringLiteral("🖱")` for emoji comparisons.

### 3. Regex character class `[⌨🖱]` may not capture supplementary Unicode
**Why:** `🖱` (U+1F5B1) is outside BMP, surrogate pair in UTF-16 may not work in character class `[...]`.
**Status:** Debug shows emoji capture is empty; regex change to `(⌨|🖱)?` identified but not yet applied per user request.

### 4. Notification translation reuse
**Fix:** Changed `tr("Block Keyboard : @%1").arg(N)` to `tr("Block Keyboard") + QString(" : @%1").arg(N)` to reuse existing translations.

## Files Modified
- `qkeymapper_constants.h` — REGEX_PATTERN_BLOCK_INPUT_MAPPING_KEY
- `qkeymapper_worker.h` — s_BlockKeyboardMask / s_BlockMouseMask declarations
- `qkeymapper_worker.cpp` — parse function, helpers, sendInputKeys paths, hook blocking checks, clearCustomKeyFlags
- `interception_worker.cpp` — device-specific blocking checks
- `qkeymapper.cpp` — validation, notification, burst/lock disable
- `qkeymapper.h` — signal/slot signature with deviceIndex default param
