---
name: processinfo-context-menu-refresh-race
description: Fix race condition between ProcessInfoTableWidget context menu and auto-refresh timer causing dangling QTableWidgetItem pointer
metadata:
  type: feedback
---

# ProcessInfoTable Context Menu + Auto-Refresh Race Condition Fix

## Situation

Commit `5a5e14b` 为进程列表（ProcessInfoTableWidget）添加了右键菜单复制功能，但没有考虑自动刷新定时器（3秒间隔）与右键菜单的并发问题。

## Root Cause

`contextMenuEvent` 中 `contextMenu.exec()` 是阻塞调用，但 Qt 事件循环在此期间继续处理定时器事件。定时器触发 `refreshProcessInfoTable(false)` → `setRowCount(0)` 销毁所有 QTableWidgetItem，导致 `exec()` 前捕获的 `item` 指针变成悬空指针。

同时 `refreshProcessInfoTable` 虽然通过 `setRangeSelected` 恢复了可视化选择高亮，但从未调用 `setCurrentItem` 恢复当前项，导致 `currentItem()` 在刷新后返回 nullptr 或错误单元格。

## Solution (3 coordinated fixes in `qkeymapper.cpp`)

1. **`refreshProcessInfoTable`**: 保存当前列号 → `setRangeSelected` 后调用 `setCurrentItem` 恢复当前项；选中行消失时通过 `QApplication::activePopupWidget()->close()` 关闭右键菜单
2. **`contextMenuEvent`**: `exec()` 返回后重新调用 `currentItem()` 替代悬空指针
3. **`keyPressEvent`**: 无需修改（同步执行无竞态）

## Key Insight

`QMenu::exec()` 不等于"阻塞整个世界" — 它只阻塞调用栈，事件循环继续运行。任何在 `exec()` 前从 QTableWidget 获取的 item 指针，在 `exec()` 后都可能已经失效。

**Why:** Qt 模态对话框/菜单使用本地事件循环，定时器事件在此期间照常分发。

**How to apply:** 任何 QTableWidget 子类的 contextMenuEvent 中，如果存在后台定时器可能触发表格刷新，必须在 `exec()` 后重新获取 `currentItem()`，不能依赖之前缓存的指针。同时表格刷新函数应同时恢复 `setRangeSelected` 和 `setCurrentItem`。
