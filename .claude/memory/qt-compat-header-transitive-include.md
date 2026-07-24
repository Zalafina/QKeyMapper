---
name: qt-compat-header-transitive-include
description: qkeymapper_qt_compat.h 通过 qkeymapper.h 间接包含，大多数文件无需手动 include
metadata:
  type: reference
---

## Qt Compat Header — Transitive Include

`qkeymapper_qt_compat.h` 在 `qkeymapper.h:80` 已包含。项目中几乎所有的 `.cpp` 文件都引用了 `qkeymapper.h`，因此 `QKeyMapperQtCompat` 命名空间已经可用，**无需额外 `#include "qkeymapper_qt_compat.h"`**。

### 添加新的 compat helper 时

1. 只在 `qkeymapper_qt_compat.h` 的 `QKeyMapperQtCompat` 命名空间内添加函数
2. 调用点直接使用 `QKeyMapperQtCompat::functionName()` 即可
3. 不要添加 `#include "qkeymapper_qt_compat.h"` —— 除非该文件不在 `qkeymapper.h` 的传递包含链上（极少见）

### 验证包含链

```bash
grep -r "qkeymapper_qt_compat.h" --include="*.h"
```
