# QTableWidget 重建刷新期间保持选区与焦点的设计模式

## 背景与问题

在 QKeyMapper 中，`refreshKeyMappingDataTable` 负责根据当前数据模型重新构建表格。为了渲染最新状态，函数会调用 `setRowCount(0)` 清空表格后再逐行插入 `QTableWidgetItem`。
这导致了几个典型问题：
1. **焦点与高亮丢失**：清空表格触发 Qt 原生信号（如 `itemSelectionChanged`），导致焦点单元格被重置为 `(-1, -1)`，高亮选区被清除；
2. **结构重排后错位**：当公共映射项在公共 Tab 中被上移/下移/拖拽重排时，由于公共映射总行数未变，直接按旧行号恢复会导致引用公共项的普通 Tab 选区指向错误的映射项；
3. **隐藏行幽灵选区**：Qt 的 `setRangeSelected` 和 `setCurrentCell` 对隐藏行（如开启“隐藏禁用”或分类筛选）依然生效，若盲目恢复旧选区，隐藏行会被选中并设为当前项，导致回车快捷键误操作看不见的行。

---

## 核心设计模式

### 1. 信号屏蔽与状态快照
在调用 `setRowCount(0)` 前，使用 `QSignalBlocker` 屏蔽控件信号，并保存选区、焦点及滚动条：
```cpp
const int previousRowCount = mappingDataTable->rowCount();
const int savedCurrentRow = mappingDataTable->currentRow();
const int savedCurrentColumn = mappingDataTable->currentColumn();
const QList<QTableWidgetSelectionRange> savedRanges = mappingDataTable->selectedRanges();
const int savedVScroll = mappingDataTable->verticalScrollBar() ? mappingDataTable->verticalScrollBar()->value() : -1;
const int savedHScroll = mappingDataTable->horizontalScrollBar() ? mappingDataTable->horizontalScrollBar()->value() : -1;

QSignalBlocker blocker(mappingDataTable);
mappingDataTable->setRowCount(0);
```

### 2. 行数守卫（Guard with Row Count Equality）
仅在总行数完全一致（`previousRowCount == newRowCount && newRowCount > 0`）时才尝试恢复选区。删除行或新增行会天然破坏行数相等条件，交由调用方（如 `deleteSelectedMappingData` 或添加函数）根据新结构显式定位新选区。

### 3. 结构重排感知与区域裁剪（Structural Reorder Exclusion）
当父源头（如公共映射 Tab）发生纯顺序移动（上移、下移、置顶、置底、拖拽）时，总行数未变，但旧行号对应的映射内容已变：
- 传递 `commonReordered = true` 标记给联动的普通 Tab；
- 联动的普通 Tab 恢复时，检查 `commonStartDisplayRow`：
  - 凡是属于公共追加区（`top >= commonStartDisplayRow`）的选区一律丢弃；
  - 跨本地与公共的选区截断至本地部分（`bottom = qMin(bottom, commonStartDisplayRow - 1)`）；
  - `effectiveCurrentRow >= commonStartDisplayRow` 时重置为 `-1`；
  - 本地行的选区完整保留。

### 4. 连续可见分段恢复（Contiguous Visible Segments Filtering）
解决 Qt `setRangeSelected` 作用于包含隐藏行范围的问题：
- 遍历旧选区范围 `[top..bottom]`，逐行检查 `!mappingDataTable->isRowHidden(r)`；
- 将连续的可见行拆分为独立的 `QTableWidgetSelectionRange(segTop, left, segBottom, right)` 分段应用，确保没有一行隐藏行被选中；
- 原焦点单元格 `effectiveCurrentRow` 仅在未隐藏时恢复；若已隐藏，安全回退到首个可见高亮行；若选区全被隐藏，则调用 `setCurrentItem(Q_NULLPTR)` 清空焦点。

### 5. 交互入口双重守卫
在触发具体业务行为的入口进行二次防御：
- **回车/快捷键入口**：`highlightSelectOpenItemSetup()` 检查 `if (m_KeyMappingDataTable->isRowHidden(topRow)) return;`；
- **对话框关闭 fallback**：`setupDialogClosed()` 检查 `!m_KeyMappingDataTable->isRowHidden(reselectrow)`；
- **批量/删除操作**：统一使用 `collectVisibleSelectedRows()` 提取操作目标，天然排除隐藏行。
