---
name: qvariantmap-saveload-pattern
description: Pattern for saving/loading struct data to QSettings using QVariantMap per entry and QVariantList as the container
metadata:
  type: project
  source: session-2026-07-01
---

## QVariantMap/QVariantList Save/Load Pattern for Struct Data in QSettings

When you need to persist a list of struct objects (e.g., `QList<MAP_KEYDATA>`) to QSettings INI files, use QVariantMap per entry wrapped in a QVariantList. This avoids writing custom serialization and is a first-class Qt type that QSettings can natively handle.

### Save pattern

```cpp
// Always write the list (even if empty) to clear stale data from previous saves
QVariantList list;
if (!myData.isEmpty()) {
    for (const MyStruct &item : std::as_const(myData)) {
        QVariantMap m;
        m["FieldName1"] = item.field1;
        m["FieldName2"] = item.field2;
        m["FieldName3"] = QString::number(item.field3);  // int stored as string
        // ... map all struct fields ...
        list.append(m);
    }
}
settingFile.setValue(settingKey, list);
```

### Load pattern

```cpp
QVariantList list = settingFile.value(settingKey).toList();
if (!list.isEmpty()) {
    myData.clear();
    myData.reserve(list.size());
    for (const QVariant &v : std::as_const(list)) {
        QVariantMap m = v.toMap();
        MyStruct item;
        item.field1 = m.value("FieldName1").toString();
        item.field2 = m.value("FieldName2").toString();
        item.field3 = m.value("FieldName3").toInt();
        // ... restore all struct fields ...
        myData.append(item);
    }
}
```

### Key rules

1. **Always write even if empty**: Writing an empty QVariantList overwrites stale INI data from previous saves. Without this, old entries persist forever since QSettings does not automatically clear keys.

2. **Use string constants as keys**: Define the key strings as `inline constexpr const char` in the constants header. Example from existing code:
   ```cpp
   inline constexpr const char KEYMAPDATA_ORIGINALKEYS[] = "KeyMapData_OriginalKeys";
   inline constexpr const char KEYMAPDATA_MAPPINGKEYS[] = "KeyMapData_MappingKeys";
   ```

3. **reserve() for performance**: When loading, call `myData.reserve(list.size())` before the loop to avoid repeated reallocations.

4. **Use std::as_const in range-for**: The pattern `for (const T &v : std::as_const(container))` prevents accidental detach of Qt's implicit-sharing containers.

5. **Type mapping**:
   | C++ type | QVariant method |
   |----------|----------------|
   | QString | `m.value(key).toString()` |
   | QStringList | `m.value(key).toStringList()` |
   | int | `m.value(key).toInt()` |
   | bool | `m.value(key).toBool()` |
   | double | `m.value(key).toDouble()` |
   | QColor | `m.value(key).value<QColor>()` |

### Existing references in codebase

- `QKeyMapper/qkeymapper.cpp:20787-20865` — save of `COMMONMAPPINGDATA` (full MAP_KEYDATA serialization with ~50 fields)
- `QKeyMapper/qkeymapper.cpp:25609-25689` — load of `COMMONMAPPINGDATA` (full MAP_KEYDATA deserialization)
- `QKeyMapper/qmacrolistdialog.cpp:1002-1060` — macro list export using same pattern (over ~15 fields per entry)

### Related files
- `QKeyMapper/qkeymapper_constants.h` — KEYMAPDATA_* string constants
- `QKeyMapper/qkeymapper.cpp` — saveKeyMapSetting, loadKeyMapSetting
- `QKeyMapper/qmacrolistdialog.cpp` — exportMacroListToFile
