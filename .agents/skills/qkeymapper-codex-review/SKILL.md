---
name: qkeymapper-codex-review
description: QKeyMapper 仓库内 Gemini 与 Codex（或 Author 与 Reviewer）之间的本地未推送提交只读代码审查、复审与修复闭环工作流。支持 `$qkeymapper-codex-review`、`/qkeymapper-codex-review` 与跨会话／跨 PC 复用，固定提交范围与 `post_review_action` 终止边界。
---

# QKeyMapper 本地未推送提交双端评审工作流

本技能将 Gemini 完成本地提交后交由 Codex 只读评审、问题修复与复审收口的流程固化到仓库，使任意新会话或另一台同步本仓库的 PC 均可直接复用。

## 双层架构与文件归属

- **随 Git 跨机同步（只放规范、模板与脚手架）**：
  - [SKILL.md](SKILL.md)：本协议正文；
  - [agents/openai.yaml](agents/openai.yaml)：Codex 原生 `$qkeymapper-codex-review` 元数据；
  - [references/templates.md](references/templates.md)：送审单、评审/复审单、修复回复单、`LATEST.md` 模板与极简双端交互口令；
  - [references/mcp-handoff-checklist.md](references/mcp-handoff-checklist.md)：提交涉及 `handoff/` 主计划或能力矩阵时的专项附加检查表；
  - [scripts/review_helper.py](scripts/review_helper.py)：非提权零依赖辅助脚本（`init` / `check` / `status`）。
- **本机 `.gitignore` 忽略（不入 Git、不污染提交历史）**：
  - `out/codex_review/LATEST.md`：当前活跃轮次唯一指针；
  - `out/codex_review/<YYYYMMDD>_<tag>/`：本机该批次评审与修复原件。
- 用户仅询问用法或读取本技能时，不修改 `out/codex_review/` 下任何文件。

## 单向追加铁律与文件命名

每轮交互文件放在 `out/codex_review/<batch>/`，严格按序号单向追加，**禁止覆盖、改写或删改对方已写入的原件**：

```text
out/codex_review/
├── LATEST.md
└── <YYYYMMDD>_<tag>/
    ├── 01_gemini_submission.md   # [仅 Author / Gemini] 送审清单、固定 SHA 与证据索引
    ├── 02_codex_review_r1.md     # [仅 Reviewer / Codex] 第 1 轮只读评审结论与发现清单
    ├── 03_gemini_response_r1.md  # [仅 Author / Gemini] 逐条核实、修复 commit 与补验证据
    └── 04_codex_recheck_r1.md    # [仅 Reviewer / Codex] 第 1 轮复审（如需多轮按 05/06 递增）
```

- 每次写完自己的文件后，顺手更新 `out/codex_review/LATEST.md` 的当前状态、最新文件指针与下一步约束。
- **相对路径规范**：
  - 从 `out/codex_review/<batch>/` 引用仓库根路径文件（如 `AGENTS.md`、`QKeyMapper/`、`handoff/`）统一使用 **`../../../`**；
  - 从 `out/codex_review/<batch>/` 引用同级 `out/` 下原件使用 **`../../`**；
  - 从 `out/codex_review/LATEST.md` 引用仓库根路径文件使用 **`../../`**。

## 冻结提交范围与固定 SHA 三元组

- 每轮送审与评审必须写清完整固定引用，**严禁用移动的 `..HEAD` 作为冻结已审范围**：
  - `branch`：当前工作分支；
  - `remote_tracking`：远端跟踪引用及短 SHA（若存在），若本地领先存在未纳入本批审查的提前提交，必须在 `deferred_commits` 显式列出；
  - `review_range`：`base_sha..doc_head_sha`（具体 8 位或完整提交哈希）；
  - `code_head`：本批实际代码／测试／配置终点 SHA；
  - `doc_head`：本批文档／送审终点 SHA（纯代码与文档同提交时同 `code_head`）。
- 若复审期间分支有额外提交，需在新一轮元信息与 `LATEST.md` 显式固定新端点，不得继续引用旧范围覆盖新提交。

## 强制终止边界：`post_review_action`

`01_gemini_submission.md` 头部必须明确声明 `post_review_action`（`review_helper.py init` 默认设为 `pause_for_user`）：

| 取值 | 含义 | Reviewer 允许动作 |
|---|---|---|
| `pause_for_user`（默认） | 本批评审收口后直接暂停 | 退回 `CHANGES_REQUESTED` 时仅核实本批并指示按用户安排修复报复审；通过 `PASS` / `PASS_WITH_P2_P3` 时指示确认收口并暂停等待用户推送／后续指示；**禁止自行追加“通过后继续下一切片”** |
| `fix_only_then_pause` | 仅修完本批发现并复审收口后暂停 | 同上，仅围绕当前批次闭环 |
| `continue_after_pass` | 用户已明确授权通过后自动衔接下一项 | 仅当送审单显式声明且与用户指示一致时，方可在第 6 节写出既定下一项 |

## 双角色职责与红线

### Author（默认 Gemini）

1. 完成当前切片全部既定开发与比例验证，在工作区干净且形成聚焦本地 commit 后（**严禁 `git push`**），用 `review_helper.py init` 生成送审目录与 `01_gemini_submission.md`，补全变更摘要、重点审阅项、证据索引与明确未验范围，并运行 `review_helper.py check --strict-meta`。
2. 收到 Reviewer 的 `CHANGES_REQUESTED` 后，按顺序读审阅报告，**先逐条核实真实源码再判断采纳**，修复 P0–P2 问题（必要 P3 按需处理）并完成比例补验，形成聚焦本地提交后写入 `03_gemini_response_r1.md`（或 r2+）并更新 `LATEST.md` 报复审。
3. 收到 `PASS` / `PASS_WITH_P2_P3` 时，确认并记录是否采纳或延后非阻断建议，按 `post_review_action` 停止并等待用户推送与指示。

### Reviewer（默认 Codex）

1. 读取 `out/codex_review/LATEST.md` 与当前 `01_gemini_submission.md`（或最新 `03/05` 回复单），**默认严格只读**：
   - 以真实 `git --no-optional-locks log <base>..<head>`、`git --no-optional-locks diff`、工作区源码及已有 `out/` 产物／SHA256 为最终事实依据，`01_gemini_submission.md` 仅作导航；
   - **禁止修改任何 Git 跟踪文件**，禁止执行 `git commit` / `git push`；
   - **禁止触发 UAC、调用 gsudo 修改状态或启停需要提权的产品进程**；如仅需非提权静态核验，或用户单独授权额外只读检查，须在评审单中如实写明命令与范围。
2. 当送审批次声明 `review_profile=handoff-plan`，或提交范围涉及 `handoff/<任务目录>/` 下的主计划、`capability_matrix.md` 或验证记录时（仅改顶层导航 `handoff/README.md` 除外），额外按 [references/mcp-handoff-checklist.md](references/mcp-handoff-checklist.md) 逐项复核。
3. 将结果写入当前批次下一个偶数文件（`02_codex_review_r1.md`、`04_codex_recheck_r1.md`...）并更新 `LATEST.md`；严格遵循模板输出，并将可直接复制回传给 Author 的 2–4 句确认提示词**同时附在评审文件第 6 节及对用户的聊天回复末尾**（严格继承 `post_review_action`）。

## 问题严重级与总体结论定义

- **`[P0]` 阻断**：崩溃、数据损坏、安全／权限／路径穿越绕过、输入热路径回归或破坏 QKeyMapper 原有核心功能。
- **`[P1]` 高危**：MCP／GUI 操作等价性错误、核心边界漏校验、状态／dirty／冷启动持久化不一致、Qt 6.8.3 警告或受影响路径 Qt 5.15.2 兼容破损。
- **`[P2]` 中危**：局部持久化或异常分支边缘缺陷、文档／产物哈希／证据口径与事实不符、局部可维护性或脚本边界缺陷。
- **`[P3]` 建议**：非阻断优化建议，不影响推送与收口。
- **总体结论**：`PASS`（无遗留 P0–P3）、`PASS_WITH_P2_P3`（无 P0/P1，仅含非阻断项，写明可延后还是建议即修）、`CHANGES_REQUESTED`（含 P0/P1 或阻断收口的 P2，必须修复后复审）。

## 辅助脚手架用法（非提权 Python 3）

从仓库根目录运行（本机首选 `out/mcp_bridge/venv/Scripts/python.exe -B -X utf8`，其他同步仓库且无该 venv 的 PC 可改用 `python -B -X utf8`）；脚本全程使用 `--no-optional-locks` 只读查 Git、`O_CREAT | O_EXCL` 独占创建 `01` 并原子更新 `LATEST.md`（不覆盖既有 `01` 原件，拒绝符号链接／NTFS 联接／硬链接及工作区外链接；作者后续写入 `03/05` 回复单时须沿用 [templates.md](references/templates.md) 的冻结范围与 `post_review_action` 头字段以供 `--strict-meta --check-latest` 绑定校验）：

```powershell
# 1. 查看当前评审指针与 Git 本地待审状态
& out/mcp_bridge/venv/Scripts/python.exe -B -X utf8 .agents/skills/qkeymapper-codex-review/scripts/review_helper.py status

# 2. 基于真实 git log 初始化送审目录与 LATEST.md（要求工作区干净，否则需 --allow-dirty；涉及 handoff/<任务>/ 时必选 handoff-plan）
& out/mcp_bridge/venv/Scripts/python.exe -B -X utf8 .agents/skills/qkeymapper-codex-review/scripts/review_helper.py init `
  --batch 20261127_example_tag --base <base_sha> --head <head_sha> --post-action pause_for_user --profile general

# 3. 写入 03/05 回复单后，或 init 更新 LATEST.md 失败后，在不触碰已有原件前提下原子同步 LATEST.md
& out/mcp_bridge/venv/Scripts/python.exe -B -X utf8 .agents/skills/qkeymapper-codex-review/scripts/review_helper.py sync-latest `
  --batch 20261127_example_tag --status WAITING_R1_RECHECK

# 4. 校验送审单／回复单／评审单／LATEST.md 的编码（UTF-8 无 BOM / LF）、命名顺序、相对链接与元信息绑定
& out/mcp_bridge/venv/Scripts/python.exe -B -X utf8 .agents/skills/qkeymapper-codex-review/scripts/review_helper.py check `
  --batch 20261127_example_tag --check-latest --strict-meta
```

完整模板与双端标准启动口令见 [references/templates.md](references/templates.md)；涉及 `handoff/` 主计划任务时配合 [references/mcp-handoff-checklist.md](references/mcp-handoff-checklist.md)。
