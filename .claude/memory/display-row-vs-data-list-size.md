---
name: display-row-vs-data-list-size
description: QKeyMapper 中显示表格行数 vs 数据列表大小的不一致陷阱
metadata:
  type: pattern
  category: qkeymapper_data_model
  confidence: 0.95
  target_files:
    - QKeyMapper/qkeymapper.cpp
    - QKeyMapper/qkeymapper.h
---

## Pattern

在 QKeyMapper 中，当普通 Tab 启用"包含共通映射表"（`IncludeCommonMappingTable`）时：

- `m_KeyMappingDataTable->rowCount()` = 自有数据行数 + 共通数据行数
- `KeyMappingDataList->size()` = 自有数据行数（仅当前Tab的数据）

这两个值**不相等**。对自有数据的操作（append、索引、遍历边界）必须使用 `KeyMappingDataList->size()`，不能使用 `rowCount()`。

## Where this matters

| 场景 | 错误用法 | 正确用法 |
|------|---------|---------|
| 新追加项的显示行号 | `rowCount() - 1` | `KeyMappingDataList->size() - 1` |
| 遍历自有数据 | `for i < rowCount()` | `for i < KeyMappingDataList->size()` |
| 插入位置边界 | `qBound(0, row, rowCount())` | `qBound(0, row, KeyMappingDataList->size())` |

## Why

`resolveDisplayRowSource(displayRow)` 的逻辑：
- `displayRow < localRowCount` → 映射到当前Tab的自有数据
- `displayRow >= localRowCount` → 尝试映射到共通数据

如果用 `rowCount() - 1` 作为行号，会落入共通数据区间，导致操作错误的映射项。

## Source

[[add-new-mapping-row-target-bug]] — 2026-07-12 修复的实际 bug
