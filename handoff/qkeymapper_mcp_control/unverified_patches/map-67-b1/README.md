# MAP-67-B1 未验证接线草稿

本目录仅保存收尾时撤回的实现草稿，供本机或另一 PC 接续审阅复用。它不是已经接入或验证的产品功能；进度、取舍及验收只维护在[唯一主计划](../../qkeymapper_mcp_automation_plan_codex.md)。

## 来源与范围

- 补丁：[integration-draft.patch](integration-draft.patch)，原样保留 25,370 字节，五文件、219 行新增／8 行删除。原始本机副本在 out/mcp_bridge/map67b-integration-wip-014ef2eb.patch，后续不以该副本作为交接入口。
- 生成基准：`014ef2ebd35b8421fc3b2aad43e3d287de044909`。该补丁不包含 B0 纯模块，不能只在生成基准上应用后就假定依赖齐全。
- 接续依赖：`cac101be33a7b30a03cb4c1d2335e08c44a520cf` 已提交 qkm_mapping_changes 参数／有界状态模块及定向测试；本次在该检查点核实五个目标文件与生成基准相同，补丁 `git apply --check` 通过。
- 草稿范围：C++ 的取消权限路由、基本映射预览／提交及结果查询、运行状态采样时间；Python 桥接及四个工具声明。仍不包含显式保存／冷启动或开始／停止工具实现。
- 五个目标文件：QKeyMapper/automation/qkm_session_server.cpp、QKeyMapper/qkeymapper.cpp、QKeyMapper/qkeymapper.h、mcp_bridge/src/qkeymapper_mcp/__main__.py、mcp_bridge/src/qkeymapper_mcp/bridge.py。
- 未编译、未过正式产品／SDK、GUI 等价或原子拒绝验收。归档和补丁格式检查不改变这一事实；当前正式源码仍为八工具／四项只读业务能力，没有这些新写入口。

## 继续作业时如何使用

1. 先读主计划最新卡片，按 handoff/README 核对分支、已同步检查点和本地改动。另一 PC 必须取得包含本目录的提交；本机 out、构建及配对凭据不随 Git 自动同步。
2. 在仓库根目录阅读补丁及目标文件的当前实现。优先逐块审阅复用，允许采用更小、更可靠的实现，不要求照搬草稿。若相关代码已有后续修改，重新判断适用部分。
3. 以下命令只查看范围并检查是否可应用，不会修改源码：

```powershell
git status --short
git apply --stat -- handoff/qkeymapper_mcp_control/unverified_patches/map-67-b1/integration-draft.patch
git apply --check --whitespace=error -- handoff/qkeymapper_mcp_control/unverified_patches/map-67-b1/integration-draft.patch
```

4. 确认相关本地改动已保留、适用性已审阅并决定整包复用时，才执行以下命令。它只恢复未验证源码，不会构建、提交或取得业务验收：

```powershell
git apply -- handoff/qkeymapper_mcp_control/unverified_patches/map-67-b1/integration-draft.patch
git diff --stat
git diff --check
```

检查失败或已应用时停止重复应用，保留当前改动并比较差异；不使用强制覆盖、--reject 或硬重置绕过冲突。也可以只按设计逐块接入，不整包应用。

5. 继续主计划 B1：复核源访问安全、目标／实例／会话／权限／到期及完整内容修订，保留 GUI 共用操作，验证拒绝零部分修改、取消／去重／断线结果查询，且不自动停止、保存或开始。尤其审阅草稿的写入预算、提交前到期检查、结果生命周期和接口关闭清理。
6. 草稿未同步既有测试对八工具／四能力的预期，也未补新工具正式 SDK 用例或 Configure 产品验收助手。须随实际接入更新调用方与测试，按主计划选定 Qt 构建、正式 SDK／产品及独立 GUI／MCP 对照；仅能应用补丁不能把 B1 标为完成。

## 何时移除

相关实现已吸收并通过选定验收，或明确采用其他实现、确认该草稿不再需要后，删除本目录的补丁及说明；同一提交更新主计划中的引用、实际实现提交和验证记录。不要仅因应用成功就删除，不建立多份“最终补丁”；历史内容由 Git 保留。不得在此放二进制、日志、配置、凭据或其他本机敏感材料。
