# `qkeymapper-codex-review` 模板与极简双端交互口令

本文件供 Author（Gemini）与 Reviewer（Codex）在每轮评审时直接套用，统一元信息字段、链接层级与回传格式。

## 一、日常三句极简双端交互口令

1. **发起送审（对 Gemini 说）**：
   `使用 qkeymapper-codex-review 为 <base..head 或当前切片> 发起送审（默认 post_review_action=pause_for_user）。`
2. **开始只读评审／复审（对 Codex 说）**：
   `使用 $qkeymapper-codex-review 按 out/codex_review/LATEST.md 执行只读评审（或复审）。`
3. **回传修复／收口（将 Codex 第 6 节提示词直接复制发给 Gemini）**：
   见下文第 6 节模板；必须明确保留 `post_review_action` 终止条件。

---

## 二、`out/codex_review/LATEST.md` 模板

```markdown
# Codex 本地提交 Review 交互入口

- **当前活跃评审批次**：`<YYYYMMDD_tag>`（`<摘要>`）
- **当前分支与远端基线**：`<branch>`；远端 `<origin/branch>` @ `<upstream_sha>`（未纳入本批的提前提交：`<无 或 deferred_range>`）
- **本轮冻结审查范围**：`<base_sha>..<doc_head_sha>`（代码终点 `<code_head_sha>`，文档终点 `<doc_head_sha>`）
- **终止边界（`post_review_action`）**：`pause_for_user`
- **评审模式（`review_profile`）**：`general`（或 `handoff-plan`）
- **当前评审状态**：**<WAITING_R1_REVIEW / CHANGES_REQUESTED / WAITING_R1_RECHECK / PASS / PASS_WITH_P2_P3>**
- **最新送审／回复文档**：[<YYYYMMDD_tag>/01_gemini_submission.md](<YYYYMMDD_tag>/01_gemini_submission.md)
- **最新 Codex 评审／复审**：<暂无 或 `[<YYYYMMDD_tag>/02_codex_review_r1.md](<YYYYMMDD_tag>/02_codex_review_r1.md)`>
- **仓库权威协议**：[qkeymapper-codex-review SKILL](../../.agents/skills/qkeymapper-codex-review/SKILL.md)
- **当前约束（必读）**：
  1. Reviewer 默认严格只读：仅核对真实 `git log`/`git diff`、源码与已有 `out/` 产物，严禁修改任何 Git 跟踪文件、严禁 `git commit` / `git push`、严禁触发 UAC/gsudo。
  2. 结论写入 `<YYYYMMDD_tag>/02_codex_review_r1.md`（或对应 `04/06` 复审文件）并更新本页状态，严禁覆盖既有奇数送审／回复原文；将 2–4 句「回传给 Gemini 的确认提示词」同时附在评审文件第 6 节及对用户的聊天回复末尾。
  3. 严格遵守 `post_review_action=pause_for_user`：收口或发回修复指示后等待，不擅自追加“通过后推进后续切片”。
```

---

## 三、`01_gemini_submission.md` 模板

从轮次子目录引用仓库文件使用 `../../../`，引用同级 `out/` 原件使用 `../../`。

```markdown
# Gemini 本地提交送审单：`<YYYYMMDD_tag>`

| 字段 | 值 |
|---|---|
| `branch` | `<branch>` |
| `remote_tracking` | `<origin/branch>` @ `<upstream_sha>` |
| `deferred_commits` | `<无 或 upstream_sha..base_sha 延后范围及原因>` |
| `review_range` | `<base_sha>..<doc_head_sha>` |
| `code_head` | `<code_head_sha>` |
| `doc_head` | `<doc_head_sha>` |
| `post_review_action` | `pause_for_user` |
| `review_profile` | `general`（仅顶层 `handoff/README.md` 亦属此项；涉及 `handoff/<任务>/` 下主计划／矩阵／验证记录时必为 `handoff-plan`） |

- **现场核对**：`git status --short` 为空；`git diff --check <base_sha>..<doc_head_sha>` 退出 0；本批未执行 `git push`。
- **评审约束**：请 Codex 按 [qkeymapper-codex-review SKILL](../../../.agents/skills/qkeymapper-codex-review/SKILL.md) 执行只读评审，将结论写入 `02_codex_review_r1.md` 并更新 [LATEST.md](../LATEST.md)；收口或发回时严格继承 `post_review_action`，并将简短「回传给 Gemini 的确认提示词」同时附在评审文件第 6 节及对用户的聊天回复末尾。

---

## 1. 待审本地提交清单

| 顺序 | SHA (8位) | 类型／切片 | 提交信息 |
|---|---|---|---|
| 1 | `<sha>` | `<scope>` | `<subject>` |

## 2. 变更摘要与重点审阅项

- 列出核心改动动机、架构/边界取舍与希望 Codex 重点核验的风险点（精确到仓库相对链接 `../../../...`）。

## 3. 验证与原始证据索引

- 区分本次实跑构建/测试产物路径、SHA256 前 8 位、退出码与日志位置（相对链接 `../../...`）。
- 明确说明 `handoff/` 专项核对项（若 `review_profile=handoff-plan`，对照 [mcp-handoff-checklist.md](../../../.agents/skills/qkeymapper-codex-review/references/mcp-handoff-checklist.md)）。

## 4. 明确未验范围

- 诚实列出本轮未执行构建、平台、外设或外部环境项目。
```

---

## 四、`02_codex_review_r1.md` / `04_codex_recheck_r*.md` 模板

> `check --strict-meta --check-latest` 与 `sync-latest` 会核对第 1 节（`## 1. 评审元信息` / `## 1. 复审元信息`）中的冻结 `<base>..<doc_head>`（及 `code_head != doc_head` 时显式声明的 `code_head`）和第 2 节（`## 2. 总体结论` / `## 2. 结论`）中的规范结论（`**PASS**` / `**PASS_WITH_P2_P3**` / `**CHANGES_REQUESTED**`），以此自动推导并校验 `LATEST.md` 的状态与评审文件指针。

```markdown
# QKeyMapper Codex 本地提交评审 r1（或复审 rN）

## 1. 评审元信息

- 日期与机器、实际分支 `<branch>`、实际远端基线 `<upstream_sha>`、冻结核对范围 `<base_sha>..<doc_head_sha>`（代码 `<code_head_sha>`，文档 `<doc_head_sha>`）、`post_review_action` `<原样继承>`。
- 工作区与暂存区清洁度、`git diff --check` 结果；明确声明只读复核，未修改跟踪文件、未提交或推送、未调用 UAC/gsudo。
- 参考：[送审单](01_gemini_submission.md) · [仓库协议](../../../.agents/skills/qkeymapper-codex-review/SKILL.md)

## 2. 总体结论

**<PASS / PASS_WITH_P2_P3 / CHANGES_REQUESTED>。** 总结本批结论、遗留项情况及终止动作（若 `post_review_action=pause_for_user`，明确“收口后暂停等待用户推送与后续指示”）。

## 3. 发现清单（按 P0 → P3 排序）

### P0
（无，或按 `[P0] <ID> · <标题> · 原因/影响/建议与精确行号链接` 逐条列出）

### P1
无（或同上）

### P2
无（或同上）

### P3
无（或同上）

## 4. 证据抽查与未验范围

- 列明核对过的真实 diff/源码、复算产物哈希、测试报告与日志原件，以及与送审单一致或需收紧的未验边界。

## 5. 协作／规范核对（按需）

- 当 `review_profile=handoff-plan` 或送审提出流程协作问题时，逐条答复防跑偏、锚点与边界结论；普通 `general` 提交可简写“不涉及主计划变更”。

## 6. 回传给 Gemini 的确认提示词（同时附在评审文件第 6 节及对用户的聊天回复末尾）

> 请读 `out/codex_review/<YYYYMMDD_tag>/02_codex_review_r1.md`。本轮对 `<base_sha>..<doc_head_sha>` 的结论为 `<PASS / PASS_WITH_P2_P3 / CHANGES_REQUESTED>`（<简述 P0–P3 发现或无阻断项>）。请逐条核实第 3 节：<若需修改则修复并聚焦提交后更新 `03_gemini_response_r1.md` 报复审；若已通过且 `post_review_action=pause_for_user` 则确认收口并直接暂停，等待我推送与后续指示>。
```

---

## 五、`03_gemini_response_r*.md` 修复回复模板

> `check --strict-meta --check-latest` 与 `sync-latest` 会严格解析并链式绑定下方 `前置基线 → 新增本地提交`（须为固定十六进制 `<prev>..<new>`）与 `post_review_action` 两行，请保留精确前缀且不要重复填写。`code_head` 默认绑定为 `<new_head_sha>`；仅当末尾存在纯文档提交导致 `code_head != doc_head` 时，才额外添加一行 `- **\`code_head\`**：\`<code_head_sha>\`` 显式固定。

```markdown
# Gemini 评审修复与确认回复 r1：`out/codex_review/<YYYYMMDD_tag>`

- **回应对象**：[02_codex_review_r1.md](02_codex_review_r1.md)
- **前置基线 → 新增本地提交**：`<prev_head_sha>..<new_head_sha>`（仅包含修复本轮发现的聚焦提交，未执行 `git push`）
- **`post_review_action`**：`<继承原设定>`

## 1. 逐条核实与处理结果

| 发现 ID / 级别 | 核实结论 | 处理方式／拒绝理由 | 源码／提交证据 |
|---|---|---|---|
| `<ID>` (`P?`) | 已核实 / 部分复现 / 不成立 | `<修复措施与边界>` | `../../../<file>#L..` · `<commit_sha>` |

## 2. 比例补验与编码核对结果

- 写明修复后重新运行的构建/测试输出、SHA256、退出码、`git diff --check` 与 `review_helper.py check` 结果。

## 3. 请 Codex 复审／收口边界

- 请按 [LATEST.md](../LATEST.md) 对 `<base_sha>..<new_head_sha>` 执行只读复审并写入 `04_codex_recheck_r1.md`，遵守 `post_review_action`。
```
