---
name: qt-ifelse-chain-variable-declaration
description: Pattern for adding local variable declarations before an existing if-else if chain without breaking brace structure
metadata:
  type: reference
  tags: [qt, c++, syntax, if-else, scope]
---

## Problem

When adding a new `else if` branch that needs local variables (e.g., regex match results), wrapping the new branch in `else { vars... if... }` steals the closing `}` from the outer block, causing C2181 "illegal else without matching if".

## Wrong approach

```cpp
else if (existingCheck) { ... }
else {                          // ❌ This breaks the chain
    QString result;
    if (newCheck(key, &result)) { ... }
    else if (anotherCheck) { ... }  // ❌ orphaned else-if
}
```

## Correct approach

Declare variables at the same scope as the if-else chain start, BEFORE the first `if`:

```cpp
// Declare vars at chain scope level
QString result;

if (firstCheck) { ... }
else if (secondCheck) { ... }
else if (newCheck(key, &result)) { ... }  // ✅ standard else-if
else if (anotherCheck) { ... }
```

## When to use

Any time you add a new `else if` branch that needs `QRegularExpressionMatch` results or other output parameters from a parse/check function, inside an existing long if-else if chain.
