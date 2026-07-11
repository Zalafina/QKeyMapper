---
name: drag-coordinate-label-memory-leak
description: VLD 检测到 DragCoordinateLabel 40 字节内存泄漏，析构函数遗漏 delete
metadata:
  type: feedback
  source: session
  severity: minor
  timestamp: 2026-07-11
---

# DragCoordinateLabel Memory Leak Fix

## Situation

VLD 报告 `qkeymapper.cpp:444` 处 `new DragCoordinateLabel()` 分配 40 字节从未释放。

## Root Cause

`ensureDragCoordinateLabel()` 创建 `DragCoordinateLabel`（Qt::Tool 顶层窗口，无 parent），存入 `m_FloatingButtonDragCoordinateLabel`。析构函数 `~QKeyMapper()` 已清理 `m_PopupNotification`、`m_FloatingIconWindow`、`m_VButtonPanel`、`m_PopupMessageLabel`、`m_PopupMessageAnimation` 等所有其他顶层 widget，但遗漏了此 label。

## Solution

在析构函数中添加 3 行（遵循现有 null-check → delete → Q_NULLPTR 模式）：

```cpp
if (m_FloatingButtonDragCoordinateLabel != Q_NULLPTR) {
    delete m_FloatingButtonDragCoordinateLabel;
    m_FloatingButtonDragCoordinateLabel = Q_NULLPTR;
}
```

**Why:** 顶层 Qt widget（无 parent）不会被 Qt 对象树自动清理，必须手动 delete。

**How to apply:** 每次新增 `new` 分配的顶层 Qt widget 成员时，务必在析构函数中添加对应的 delete 块。建议以 `grep "new.*Label\|new.*Widget\|new.*Window\|new.*Dialog" qkeymapper.cpp` 交叉对比析构函数中的 delete 列表。
