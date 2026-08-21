---
name: qt-header-name-collision-windows
description: Windows 下项目头文件不得与 Qt 头同名或复用其 include guard
metadata:
  type: reference
---

## Qt Header Name Collision on Windows

项目头文件不要使用 Qt 已有头文件的名称，即使项目代码通过双引号包含它。
Windows 文件系统通常不区分大小写，而且项目 include 路径位于 Qt include 路径之前，
因此同名项目头会遮蔽 Qt 头。

`qinputdevice.h` 是已验证的冲突名称：Qt 6 提供
`QtGui/qinputdevice.h`，并使用 `QINPUTDEVICE_H` include guard。项目文件若同时使用
这个名称和 guard，会导致项目的 `InputDevice` 类型与 Qt 的 `QInputDevice` 类型互相屏蔽，
产生大量看似无关的级联编译错误。

共享类型头应使用项目限定名称和唯一 guard，例如：

```cpp
#ifndef QKEYMAPPER_INPUT_DEVICE_TYPES_H
#define QKEYMAPPER_INPUT_DEVICE_TYPES_H
```

新增公共头之前，应检查 Qt include 目录是否已有同名文件，并避免复用通用 guard。
移除大型传递 include 时，还要给每个 `.cpp` 补齐它直接使用的 Qt、WinAPI 和第三方头。