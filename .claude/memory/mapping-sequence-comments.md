---
name: mapping-sequence-comments
description: 映射序列编辑窗口"备注"列功能：纯数据字段方案、两阶段提交数据流、reconcile对齐算法、已知陷阱
metadata:
  type: project
---

# 映射序列编辑窗口"备注"列

## 架构决策

**纯数据字段方案**：`MAP_KEYDATA.MappingKeys_Comments` / `MappingKeys_KeyUp_Comments` (QStringList)，
`MappingMacroData.MacroComments` (QStringList)。与段列表按索引对齐。

废弃字符串语法方案的理由：映射字符串保持纯净，
`splitMappingKeyString` / `validateMappingKeyString` / 运行时 `sendInputKeys` 全部零改动。

## 数据流（两阶段提交）

```
SeqEdit "确定" → m_PendingMappingComments + m_PendingMappingKeysSegments (暂存)
       ↓
   "Update" → updateAllMappingInfoFinally: reconcile(pendingSegs, pending, newSegs) → MAP_KEYDATA → clear pending
       ↓
   "Save Setting" → saveKeyMapSetting → INI (QVariantList 格式)
```

**三个入口的读取优先级：**
1. `m_PendingMappingComments` 非空 → 用 pending（最近一次 SeqEdit "确定" 未提交）
2. Pending 为空 → 用 `MAP_KEYDATA.MappingKeys_Comments`（已持久化）

**Pending 清空时机（保证无冲突）：**
- `updateAllMappingInfoFinally` 写入 MAP_KEYDATA 后立即 clear
- `closeEvent` 对话框关闭时 clear
- `addMacroToListInternal` 写入宏数据后立即 clear

## reconcileMappingKeyComments 算法

```
pass0: oldSegs == newSegs → fast path (含 size guard)
pass1: same position + same text → keep comment
pass2: unique segment text match in newSegs → migrate comment
       oi >= oldSegs.size() → position-based fallback
```

时间复杂度 O(n*m)，仅编辑器打开 / confirm / Update 时调用。

## 边界场景覆盖

| 场景 | 处理方式 |
|------|----------|
| SeqEdit "确定" 后不点 Update 重开 | pending 分支，用保存的 PendingKeysSegments 做 reconcile |
| 直接编辑 lineedit 后点 Update | else-if oldMappingKeys != newSegs 分支，reconcile 存储的 comments |
| SeqEdit 内新增段后加备注 | pass2 bounds guard + 同位 fallback |
| 复制粘贴多行 | `s_CopiedMappingCommentList` 并行剪贴板 |
| Mapping Code 复制/粘贴 | `MAPPINGCODE_STRINGLIST_FIELDS` X-macro 自动包含 Comments 字段 |

## 涉及文件

| 文件 | 改动要点 |
|---|---|
| `qkeymapper_worker.h` | MAP_KEYDATA +2 QStringList, MappingMacroData +1 |
| `qkeymapper_constants.h` | 常量 +3, TABLE_COLUMN_COUNT 1→2, COMMENT_COLUMN=1 |
| `qkeymapper.h/.cpp` | reconcileMappingKeyComments(), save/load 持久化, Mapping Code X-macro |
| `qmappingsequenceedit.h/.cpp` | m_MappingCommentList, s_CopiedMappingCommentList, 双列表格, 锁步编辑, 列宽自适应 |
| `qitemsetupdialog.h/.cpp` | m_PendingMappingComments + Segments, 两阶段提交, 三个边界分支 |
| `qmacrolistdialog.h/.cpp` | m_PendingMacroComments + Segments, addMacroToListInternal reconcile |
