---
name: nested-brace-pitfalls
description: Common bugs introduced when modifying deeply nested save/load code, especially with Qt serialization patterns
metadata:
  type: project
  source: session-2026-07-01
---

## Common Pitfalls When Modifying Save/Load Code With Nested Braces

QKeyMapper's `saveKeyMapSetting()` and `loadKeyMapSetting()` are deeply nested functions (hundreds of lines, multiple levels of scope). Modifying them introduces characteristic bugs.

### 1. Extra brace terminates function early

The `saveKeyMapSetting` function has many brace levels: outer if-blocks for settings groups, for-loops over data, QVariantMap construction blocks. An **extra closing brace `}`** will silently terminate the function body early, causing all code after it to be parsed as global/namespace scope.

**Symptoms**:
- Compiler errors on `"unknown type"` for variables that were local
- "does not name a type" errors on subsequent function calls
- Constructor/destructor defined outside class errors

**Root cause**: When a function body ends earlier than expected, subsequent lines that were inside the function are now parsed as file-scope declarations, which fail because they reference types or use statement syntax.

**Prevention**:
- Always add a comment after closing braces in deep nesting, e.g. `} // end for each item` or `} // end if !empty`
- Count brace pairs before and after an edit region
- When adding a new loop or block, write both braces first, then fill the body

### 2. Save guard prevents clearing stale data

A common pattern is wrapping the save code in `if (!data.isEmpty())`:

```cpp
// BUG: if data is empty, stale INI data is NEVER overwritten
if (!m_CommonMappingData.isEmpty()) {
    QVariantList commonList;
    for (...) { commonList.append(...); }
    settingFile.setValue(KEY, commonList);
}
```

**Fix**: Always write the QVariantList (even empty) to overwrite stale data:

```cpp
// CORRECT: empty list still written to clear old data
QVariantList commonList;
if (!m_CommonMappingData.isEmpty()) {
    for (...) { commonList.append(...); }
}
settingFile.setValue(KEY, commonList);  // unconditional write
```

### 3. Early return skips data restoration

When adding an early return for an optimization (e.g., tab already exists), ensure it doesn't skip the data restoration step:

```cpp
// BUG: early return skips m_CommonMappingData restoration
int existingIndex = findCommonMappingTabIndex();
if (existingIndex >= 0) {
    ensureCommonMappingTabIsLast();
    return;  // OOPS: restored data block after this is skipped!
}

// ... create new tab ...
if (!m_CommonMappingData.isEmpty()) {
    *commonData = m_CommonMappingData;  // never reached if tab existed
}
```

**Fix**: Move the restoration inside the early-return block:

```cpp
if (existingIndex >= 0) {
    ensureCommonMappingTabIsLast();
    if (!m_CommonMappingData.isEmpty()) {
        // restore even for existing tab (e.g., after re-enable with fresh empty data)
        if (commonData && commonData->isEmpty()) {
            *commonData = m_CommonMappingData;
        }
    }
    return;
}
```

### 4. Scope shadowing in serialization loops

When building QVariantMap entries inside a for-loop, the `m` variable name is used per-iteration. Avoid reusing `m` or `d` at outer scopes:

```cpp
for (const MAP_KEYDATA &d : std::as_const(m_CommonMappingData)) {  // OK: scoped to loop
    QVariantMap m;  // OK: fresh per iteration
    m["key"] = d.SomeField;
    // ...
    commonList.append(m);
}
```

But if there's an outer loop using the same variable name, inner code may accidentally reference the wrong scope.

### 5. Brace mismatch in conditional compilation blocks

When `#ifdef` blocks surround sections of serialization code, the brace count can mismatch between enabled and disabled configurations:

```cpp
if (condition) {
    // ... lots of code ...
#ifdef FEATURE_X
    settingFile.setValue(KEY, value);
}  // BUG: closes the if, but #ifdef may have created extra braces
#endif
```

Always compile-test both configurations when adding conditional compilation to save/load code.

### Prevention checklist

When modifying saveKeyMapSetting or loadKeyMapSetting:
- [ ] Count braces before and after edit (use `grep -n '{'` / `'}'` over region)
- [ ] Write QVariantList unconditionally (even empty)
- [ ] Check early returns don't skip data restoration
- [ ] Verify variable names don't shadow outer scope
- [ ] For each `if (!data.isEmpty())` guard, verify it doesn't prevent clearing stale data
- [ ] Compile after edit (ask user unless they opt out)
