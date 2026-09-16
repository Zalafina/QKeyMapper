---
name: qkeymapper-readme-en-sync
description: 将 QKeyMapper 中文版 README.md 的新增和变更内容同步翻译到英文版 README_en.md，比对章节与更新日志差异，沿用英文版固定术语，先展示完整草稿供审阅，批准实施后仅更新并本地提交 README_en.md。用于英文 README 同步、英文更新日志补齐、按键对照表维护。
---

# QKeyMapper README English Sync

以仓库根目录 `README.md`（中文版）为准，将其新增和变更内容翻译补充到 `README_en.md`（英文版）的对应现有章节中。翻译沿用英文版当前使用的固定 QKeyMapper 专用英文名词。遵守仓库指令，不增加脚本、依赖或全局配置。

## 先审阅，再实施

- 首次调用只读比对并展示完整翻译草稿（含差异清单），不写文件、不暂存、不提交。用户已明确批准当前草稿并要求实施时，直接进入实施阶段，不重复索要确认。
- 建议用户在计划模式调用 `$qkeymapper-readme-en-sync` 或 `/qkeymapper-readme-en-sync`；处于计划模式时展示计划等待批准，普通模式下展示同样的草稿，等待明确批准。
- 草稿批准同时授权本次 README_en.md 本地提交，计划中必须明确这一点。用户明确要求仅草稿、不提交或调整范围时，遵从其要求。
- 需要等待审阅时，引用本技能路径及"首次调用只读比对并展示完整翻译草稿，不写文件、不暂存、不提交。"，简短说明这是用户要求的审阅步骤。

## 固定同步策略

- **章节集合固定**：英文版章节以当前状态为准（含 📣 Known Issues）。原则上不对英文版新增章节；中文版出现英文版没有的章节时，先列出差异并向用户确认后再决定是否翻译补充。
- **永不补充**：Wiki 链接、Bilibili 教学视频、QQ 群三个中国特有资源链接章节；「正则表达式匹配规则说明」整章（英文版名词表 RegexMatch 行已引用软件目录下的 PDF 文档）。
- **英文版独有不动**：⭐ Star Trend 章节；截图、XBox 布局图（英文版仅引英文布局图）、VirScan 章节保持英文版现状。
- **错位随修**：发现英文版老旧条目日期或内容错位、整条缺失、要点遗漏时，随本次同步一并修复，不只补最新段。先例：2024 年初条目整体错位一版、20241124 整条缺失、20250926 漏 2 个要点。
- **单向同步**：只以 README.md 为准更新 README_en.md，不反向修改 README.md。发现中文版自身笔误时不传播到英文版，按正确含义翻译。先例：中文版名词表"抬起映射"重复行（英文版用 KeyUpMapping + SendTiming 两行）、20260212 条目 `vJoy-RT-Max[50]` 说明误写"左扳机键"（应为 Right trigger）。

## 比对收集差异

0. **确认 Git 状态与工作区检查**（前置防呆）：
   - 确认 Git 仓库根目录、分支、HEAD 完整哈希、工作区与暂存区状态。
   - 检查并记录 `README_en.md` 是否存在已有修改（staged/unstaged），并在草稿审阅阶段明确展示其状态与提交处理边界；若有未提交的中文版源码或文档变动仅作提示，不夹带。
1. **增量线索定位（日常增量同步推荐先做）**：
   - 查看自上次英文版同步提交以来，中文版 `README.md` 有哪些具体变动，秒级定位修改段落与表格：
     ```powershell
     $lastEnCommit = git log -1 --format="%h" -- README_en.md
     git diff "$lastEnCommit..HEAD" -- README.md
     ```
2. **Build 号全量比对**：
   ```powershell
   $cn = rg -o "Build 20\d{6}" README.md | Sort-Object -Unique
   $en = rg -o "Build 20\d{6}" README_en.md | Sort-Object -Unique
   Compare-Object $cn $en
   ```
   中文版多出的每个 Build 号都是一个待翻译条目；英文版多出的需人工判断（历史原因）。
   若比对后发现双向完全一致且内容完整无更新，直接向用户说明“README_en.md 与 README.md 已完全同步，无需更新”，不生成空计划，不执行写操作或空提交。
3. **条目级逐 bullet 比对**：Build 号相同不代表内容一致。对每个共有条目逐要点核对，补译遗漏要点。
4. **章节与表格清单核对**（固定核对表，逐项过）：
   - 软件简介 / 免责声明 / 功能特性表 / 常用名词解释列表 / Release 下载说明 / 使用须知（注意1~4）/ 使用建议 / Known Issues
   - 特殊原始按键对照表、特殊映射按键对照表（逐行对，含表下 ##### 附注）
   - Xbox 手柄表及 @0~9 附注、特殊物理手柄按键表、特殊功能手柄原始按键表（含下方 Gyro2Mouse/GamepadTouchpad 映射键小表）、扩展手柄按键表、PS4-Dualshock 表、Xbox360 虚拟手柄表及 @0~3 与 20ms 两条附注、特殊功能虚拟手柄表（含下方 Mouse2vJoy 小表）
   - PS4 表曾整表过期（旧 Joy-Key14~18 错误映射），按键映射变化时整表替换而非局部修补。

## 翻译规则

1. **术语以英文版现有用词为准**：翻译新内容前先通读 README_en.md 挖掘既有固定译法；拿不准的界面词优先在英文版中找先例。核心固定术语：
   - 原始按键 Original Key｜映射按键 Mapped Key｜抬起映射 Key Release Mapping（名词表用 KeyUpMapping）｜发送时机 Send Timing｜连发 Burst｜锁定 Lock｜长按 Long Press｜双击 Double Click｜连发按下/抬起时间 Burst Press/Release Time
   - 映射项设定窗口 Mapping Item Settings window｜映射表设定窗口 Mapping Table Settings window｜通用设定标签页 General Settings tab｜映射设定标签页 Mapping Settings tab｜映射高级设定 Advanced Mapping Settings｜通用高级设定 General Advanced Settings｜窗口信息标签页 Window Info tab
   - 悬浮按钮 Floating Button｜悬浮按钮设定窗口 Floating Button Settings window｜虚拟按钮面板 Virtual Button Panel｜共通映射表 Common Mapping Table｜映射宏列表 Macro List｜通用宏 Universal Macro｜序列编辑 Sequence Edit
   - 显示切换 Display Switch｜筛选键 Filter Keys｜鼠标穿透 Mouse Through｜轻推/重推 Light/Hard Push｜轻按/重按 Light/Hard Press｜阈值 Threshold｜回中延时 Recenter Delay｜直控模式 Direct Control Mode｜缩放比例 Scaling Ratio｜保存设定 Save Settings
2. **不翻译**：所有按键名、映射键名（含参数形式，如 `vJoy-Key11(LT)_BRAKE[INIT=150,THR=2.0]`、`KeySequenceToggle(OriginalKey)`、`{{CLIPBOARD_TEXT}}`）、命令行、文件路径、驱动和库名称。
3. **示例本地化**：示例中的中文窗口标题、中文说明文字翻译为英文；示例命令本身保持原样。先例：ahk 工具示例 `title="工作"` → `title="work"`、`记事本` → `Notepad`。
4. **格式约定**（与中文版不同点，必须遵守）：
   - 条目标题 `* v1.3.8 (Build YYYYMMDD)` —— 英文版 Build 前有空格（中文版无空格）
   - 要点 4 空格缩进 `*`；子项 6 空格 `-`；示例标题 6 空格前缀加 `##### `；示例代码块 10 空格缩进；注意事项 `**※ ...**`
   - 每个 Build 条目之间不空行，紧接下一条（沿用英文版现有紧凑排版，与中文版一致）
5. **日志条目写法**：每条通常一两句话，写操作、效果、必要限制；合并同一功能的多次修复；不暴露函数、宏、线程等实现细节。

## 写入和本地提交

1. 只有当前草稿获批准且实际处于允许写入的模式，才能实施。写入前复核两个 README 与暂存区；内容变化会影响草稿或提交范围时重新展示修订稿等待批准。
2. 只改批准的同步内容，保留其他内容、UTF-8 无 BOM 编码和换行风格。README_en.md 原有 staged/unstaged 修改必须先展示并明确处理边界，不默认包含在本次提交中，不 stash、reset 或覆盖用户改动。
3. 执行"验证清单"全部通过后提交。README_en.md 原先无用户改动时，使用限定路径的提交：单次指定版本同步推荐使用 `git commit --only -m "docs: sync README_en.md for Build YYYYMMDD" -- README_en.md`（与 release notes 风格对齐）；多版本合并或全量校对同步使用 `git commit --only -m "docs: sync README_en.md with README.md" -- README_en.md`。避免夹带其他已暂存文件；严禁使用 `git add .`、`git commit -a` 或不限定范围的提交。
4. 不 push、不打 tag、不创建远端 release、不 amend。提交后检查提交实际只含批准的 README_en.md 修改，并核对其他文件的 staged/unstaged 状态未被改变。报告 commit ID 和检查结果。

## 验证清单

写入后必做，全部通过才可提交：

1. 运行 `git diff --check`，确认无行尾空白错误或合并冲突标记。
2. Build 号双向比对无差异（命令同"比对收集差异"第 2 步）。
3. 本次新增按键名在英文版中可检索到（`rg -c "按键名1|按键名2|..." README_en.md` 抽查）。
4. Markdown 表格列数一致性检查（逐表比对 `|` 计数，新增行列数与表头一致）。
5. 文件仍为 UTF-8 无 BOM（前 3 字节非 EF BB BF）。
6. 抽查新旧条目衔接处与表格上下文，确认无格式破坏且各条目间无多余空行。

## 验收要点

- 正常调用：草稿前零写入；批准实施后只有 README_en.md 的一个本地 commit。
- 重复调用/无差异：不重复条目、不空提交。
- 共有 Build 条目：逐要点核对补漏，不仅比对日期。
- 中文版笔误：不传播，按正确含义翻译；中文版缺章节：先确认再补充，永不补充清单内章节直接跳过。
- README_en.md 有用户改动：先确定边界；审阅后两个 README 或暂存区改变：复核，影响草稿则重新审阅。
