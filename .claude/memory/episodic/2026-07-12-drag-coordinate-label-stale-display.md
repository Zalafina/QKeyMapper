---
name: drag-coordinate-label-stale-display
description: updateDragCoordinateLabel 的 isVisible 守卫导致 MouseButtonPress 时 label 显示上次拖拽的残留坐标
metadata:
  type: feedback
  source: session
  severity: minor
  timestamp: 2026-07-12
---

# DragCoordinateLabel 拖拽开始时不更新坐标

## Situation

用户 Ctrl+点击悬浮按钮 "B" 时，坐标 label 显示在上次拖拽的按钮 "E" 上方，显示 "E" 的残留坐标。移动鼠标后才正确更新为 "B" 的坐标。

## Root Cause

`updateDragCoordinateLabel()` 在 [qkeymapper.cpp:495](QKeyMapper/qkeymapper.cpp#L495) 有 `!label->isVisible()` 守卫：

```cpp
if (label == Q_NULLPTR || !label->isVisible()) return;
```

事件时序：
1. 上次拖拽结束 → `label->hide()` — label 隐藏但残留旧文本/位置
2. 新拖拽开始（MouseButtonPress）→ `updateDragCoordinateLabel()` → label 不可见 → 直接 return
3. `label->show()` → 显示的是步骤 1 残留的旧数据
4. MouseMove → label 已可见 → 正常更新

## Solution

移除 `!label->isVisible()` 守卫：

```cpp
if (label == Q_NULLPTR) return;
```

**Why:** 更新一个隐藏的 widget 没有副作用，不应被可见性守卫阻止。守卫反而导致 show() 时暴露残留数据。

**How to apply:** 编写 `updateXxx()` 类函数时，避免添加 `isVisible()` 守卫。可见性不应是更新的前置条件，调用方知道自己是否需要在显示前更新。如果确实需要性能优化，应该在调用方判断而非在更新函数内部。
