---
name: add-new-mapping-row-target-bug
description: Ctrl+click "添加新映射" opened wrong item setup dialog when normal tab has common mappings appended
metadata:
  type: episodic
  date: 2026-07-12
  files_changed:
    - QKeyMapper/qkeymapper.cpp
    - QKeyMapper/qkeymapper.h
---

## Situation

用户报告：普通映射表中按 Ctrl 点击"添加新映射"按钮，虽然先添加了 A→A 映射项，但打开的"映射项设定"窗口显示的是追加在映射表末尾的最后一个共通映射项。

## Root Cause

`insertNewMappingData()` 中非 insertBySelection 路径计算行号时使用了：
```cpp
m_KeyMappingDataTable->rowCount() - 1
```
当普通Tab启用了"包含共通映射表"时，显示表格行数 = 自有数据行数 + 共通数据行数。`rowCount() - 1` 指向最后一行（共通映射行），而刚 append 的 A→A 项在 `KeyMappingDataList->size() - 1` 位置。

`showItemSetupDialog()` 调用 `resolveDisplayRowSource()`，当 displayRow >= localRowCount 时解析为共通映射表的行，导致打开了错误的映射项设定。

## Solution

1. **Fix row calc** (line 37570): `m_KeyMappingDataTable->rowCount() - 1` → `KeyMappingDataList->size() - 1`
2. **Safety check**: 打开设置窗口前验证 boundedRow 指向的项与 `newKeyMappingData` 匹配，不匹配时在 KeyMappingDataList 中回退查找
3. **Context menu**: 在右键菜单 Edit 组中添加"Add New Mapping"项，始终可见

## Compilation fixes needed

- `const int boundedRow` → `int boundedRow`（安全校验回退查找需要重新赋值）
- `addFixedDefaultMappingAndOpenSetup()` 从 private 移到 public（右键菜单 lambda 需要访问）

## Lesson

QKeyMapper 中 `m_KeyMappingDataTable->rowCount()` 不等于 `KeyMappingDataList->size()` 当共通映射被追加显示时。操作自有数据项时应始终用 `KeyMappingDataList->size()` 而非 `rowCount()` 作为索引边界。

**Why:** 显示层和数据层的行数在共通映射场景下不一致。`resolveDisplayRowSource()` 依赖 `displayRow < localRowCount` 判断来区分自有数据和共通数据，用错行号会导致路由到错误的源数据列表。

**How to apply:** 在 `insertNewMappingData()` 和类似操作中，使用 `KeyMappingDataList->size()` 作为新追加项的行号，而非 `rowCount()`。

Related: [[display-row-vs-data-list-size-pitfall]]
