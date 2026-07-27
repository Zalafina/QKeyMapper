---
name: qtablewidget-context-menu-stale-pointer
description: QTableWidgetItem pointers captured before QMenu::exec() become stale if background refresh runs during the menu loop
metadata:
  type: reference
---

# QTableWidget Context Menu Stale Pointer After Background Refresh

## Pattern

When a `QTableWidget` subclass has a context menu AND a background timer that refreshes the table data:

```cpp
// ❌ BUG: item pointer may be stale after exec() if refresh fired
void FooTable::contextMenuEvent(QContextMenuEvent *event)
{
    QTableWidgetItem *item = currentItem();   // captured before exec
    QAction *act = menu.exec(...);           // timer may fire here, destroying item
    if (act == copyAction) {
        copyCellContentToClipboard(item);     // DANGEROUS: dangling pointer
    }
}

// ✅ FIX: re-fetch currentItem() after exec()
void FooTable::contextMenuEvent(QContextMenuEvent *event)
{
    QTableWidgetItem *item = currentItem();   // used only for pre-menu decisions
    QAction *act = menu.exec(...);           // timer may fire here
    if (act == copyAction) {
        QTableWidgetItem *current = currentItem();  // re-fetch after exec
        if (current) {
            copyCellContentToClipboard(current);
        }
    }
}
```

## Why It Fails

`QMenu::exec()` 使用本地事件循环 — 调用栈被阻塞，但 Qt 事件循环继续处理定时器、paint、socket 等事件。如果定时器回调触发了 `setRowCount(0)`（销毁所有 item），`exec()` 前捕获的 `QTableWidgetItem*` 就变成了悬空指针。

## Required Companion Fix in Refresh Function

表格刷新函数需要同时恢复两个东西，不能只恢复一个：

```cpp
// ❌ INCOMPLETE: only restores visual selection
setRangeSelected(selection, true);

// ✅ COMPLETE: also restores currentItem for context menu/keyboard focus
setRangeSelected(selection, true);
QTableWidgetItem *newItem = table->item(row, savedColumn);
if (newItem) {
    table->setCurrentItem(newItem);
}
```

`setRangeSelected` 恢复的是可视化选择（蓝色高亮行），`setCurrentItem` 恢复的是键盘焦点单元格（`currentItem()` 返回值）。右键菜单依赖的是后者。

## When Selection Row Disappears

如果之前选中的行已消失（进程退出等），应关闭正在显示的右键菜单：

```cpp
if (reselectrow == -1) {
    QWidget *popup = QApplication::activePopupWidget();
    if (popup) {
        popup->close();  // dismisses context menu, exec() returns nullptr
    }
}
```

## Related Memory

- [[qmenu-null-action-comparison-pitfall]] — another QMenu::exec() pitfall with null action comparison
- [[processinfo-context-menu-refresh-race]] — the specific QKeyMapper instance of this pattern

**Why:** QMenu::exec() uses a local event loop; timers fire during it. Add when: any QWidget with a context menu and background refresh timer.
