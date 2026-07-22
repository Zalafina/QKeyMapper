---
name: 2026-07-22-mapping-sequence-comment
description: 映射序列编辑窗口添加"备注"列的完整实施与修复记录
metadata:
  type: episodic
  date: 2026-07-22
  skill: qkeymapper-workflow
---

# 映射序列编辑窗口"备注"列 — 实施与修复全记录

## 实施概览

在 QMappingSequenceEdit 单列表格后追加第二列"Note"，用户可为每个按键序列段添加备注。
纯数据字段方案，同时支持 QItemSetupDialog 和 QMacroListDialog。

## 发现并修复的 Bug（按时间顺序）

### Bug 1: 备注在"确定"→重新打开后丢失
**根因**：SeqEdit handler 只读 MAP_KEYDATA，忽略 m_PendingMappingComments
**修**：Handler 先检查 pending，为空才读 MAP_KEYDATA；Update 后 clear pending

### Bug 2: 列宽 resize 横向滚动条
**根因**：setColumnWidth+setStretch 顺序与 resizeMacroListTableColumnWidth 不一致
**修**：重构为先设两列宽度→Stretch→读回锁定，commentWidthMin=50

### Bug 3: 列头 UI 和 debug log
**修**：tr("Comment")→tr("Note")；多处添加 DEBUG_LOGOUT_ON 日志

### Bug 4: Undo/Redo 后列宽不更新
**修**：restoreHistorySnapshot() 末尾加 resizeMappingSequenceEditTableColumnWidth()

### Bug 5: 手动编辑 lineedit 后点 Update 导致备注串位
**根因**：updateAllMappingInfoFinally 用 newSegs 同时作 oldSegs 和 newSegs → pass0 截断
**修**：保存 m_PendingMappingKeysSegments；无 pending 时用 oldMappingKeys reconcile 存储的 comments

### Bug 6: Crash — reconcile oldSegs 越界
**根因**：用户在 SeqEdit 内新增段，MAP_KEYDATA 仅 2 段但 pending 有 5 项，pass2 oldSegs[oi] 越界
**修**：pass2 加 oi >= oldSegs.size() bounds guard + 同位 fallback

### Bug 7: 复制粘贴备注丢失
**根因**：仅有 s_CopiedMappingSequenceList，无并行 comment 剪贴板
**修**：新增 s_CopiedMappingCommentList，同步到 copy/insert/replace

### Bug 8: 宏列表直接编辑 lineedit 后 Add Macro 备注丢失
**根因**：addMacroToListInternal 在无 pending 时未 reconcile 存储的 comments
**修**：isUpdate 时若 pending 为空且 oldSegs!=newSegs 则 reconcile previousMacroData.MacroComments

## 验证状态

- [x] 序列编辑窗口两列显示、备注就地编辑
- [x] 备注随 Save Setting 持久化、重启恢复
- [x] 删段后备注正确迁移/丢弃（三种场景全覆盖）
- [x] QItemSetupDialog 和 QMacroListDialog 双入口支持
- [x] 复制粘贴备注同步
- [x] Mapping Code 包含/恢复备注
- [x] 列宽自适应无横向滚动条
- [x] undo/redo 后列宽自动更新
