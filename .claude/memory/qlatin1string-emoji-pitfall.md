---
name: qlatin1string-emoji-pitfall
description: QLatin1String only handles 8-bit Latin-1; emoji and non-BMP Unicode require QStringLiteral
metadata:
  type: reference
  tags: [qt, unicode, emoji, string, qlatin1string, qstringliteral]
---

## Problem

`QLatin1String("⌨")` silently corrupts emoji characters. Emoji like ⌨ (U+2328, outside Latin-1 range) and 🖱 (U+1F5B1, supplementary plane) are not representable in Latin-1 (8-bit).

## Rule

- `QLatin1String` — only for ASCII/Latin-1 characters (≤ U+00FF)
- `QStringLiteral` — for any Unicode, including emoji and CJK
- For comparisons: `deviceType == QLatin1String("Keyboard")` is fine (pure ASCII), but `emoji == QLatin1String("⌨")` is broken

## Fix

```cpp
// ❌ Broken — emoji corrupted
emoji == QLatin1String("⌨")

// ✅ Correct
emoji == QStringLiteral("⌨")
```

## Related

- [[regex-character-class-unicode-pitfall]] — regex `[...]` may also fail on non-BMP chars
