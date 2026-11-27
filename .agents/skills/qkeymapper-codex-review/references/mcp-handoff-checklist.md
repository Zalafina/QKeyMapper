# `handoff/` 任务主计划与 MCP 专项附加评审清单

当送审批次声明 `review_profile=handoff-plan` 或提交范围涉及 `handoff/<任务目录>/` 下任务主计划、`capability_matrix.md`、`validation.md` 时（仅顶层导航 `handoff/README.md` 除外），Author 送审前与 Reviewer 评审时额外核对以下事项：

1. **交接卡片固定锚点防漂移（`PLAN-R1-ANCHOR` 规则）**
   - 核对主计划顶部卡片是否明确记录**上次已审／远端跟踪基线 SHA**、**当前分支已审或待审切片的固定代码／文档 SHA**，且未用会自动滑动的裸 `HEAD` 伪造“已冻结送审状态”。
   - 核对 `待 Codex 验收` 列表是否完整保留尚未终审的历史切片记录，未擅自清除未获用户或 Codex 确认的待验项。

2. **目录体量、编码与锚点完整性**
   - 单文件建议不超过 `256 KiB`（`262,144` B），单任务目录建议不超过 `2 MiB`（沿用 [handoff/README.md](../../../../handoff/README.md) 建议口径；若当前切片另有明确验收约定阈值，以该约定为准）；
   - UTF-8 无 BOM、纯 LF（`CR=0`）、`git diff --check` 零输出；
   - 主计划变更处出站链接、跨文件入站 `#anchor` 锚点及外部文档引用的章节标题均有效可解析；涉及精简压缩时，核实存在可恢复全文的固定 Git commit SHA。

3. **能力矩阵与阶段表口径一致性**
   - 新增或修改的能力、控件行与里程碑摘要是否与源码实现（Qt 控件、`qkm_mapping_changes`、`tool_catalog.json`、`tool_args.rs`）一致；
   - 是否如实区分「设置/配置子项已选验」与「实际物理外设/屏幕视觉/驱动运行时效果待验」，未把仅配置验证写成整模块完全闭环。

4. **`out/` 原件来源机、跨机重取与 FakerInput 口径**
   - 所有指向 `.gitignore` 忽略目录 `out/` 的证据索引，须明确说明所属执行机（如 `PC-W11` / `PC-W10`）、不随 Git 同步，以及「有随仓脚本的可重跑，仅存原机的夹具或日志按交接规则向原机索取」的真实重取路径；
   - 若隔离验收副本为保护本机安全而禁用了 FakerInput 分配与连接，须显式写明「运行副本禁用 FakerInput 分配与连接，开启 FakerInput 连接运行待验」，禁止写为已验证带连接运行。

5. **构建、测试与隐私边界**
   - 检查声明的 Qt 6.8.3 零警告与受影响范围 Qt 5.15.2 兼容结论是否同原日志文件一致（Qt5 仅允许官方 `qtmain.lib(qtmain_win.obj)` LNK4099 Standing Exception）；
   - 确认 `handoff/` 文档与 `out/codex_review/` 文本未夹带令牌、凭据、私人窗口标题或真实敏感输入文本。
