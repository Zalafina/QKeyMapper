# QKeyMapper MCP 开发计划：操作等价性、阶段排期与双机交接

更新日期：2026-11-27（前序验收保留实际发生日期）。初始源码核对基线：`d49a9f57d280d3b3cecc8204372ae7dc13bf6e0d`。

本文件是本任务唯一的计划、进度、决策及最新交接状态来源。相关入口：[目录规则](../README.md)、[能力与等价性矩阵](capability_matrix.md)、[验证记录](validation.md)。

Rust 是唯一受支持的 Sidecar。Python 实现与对照证据原地封存，正式回退入口撤销；普通 Python SDK 客户端、夹具和打包工具继续按需使用。工具合同以独立审阅的 Rust 目录、参数负例和真实产品结果验收。用户自行选择何时改客户端命令，沿用已核验目标和凭据。历史见[迁移归档](qkeymapper_mcp_rust_migration_plan.md)。

## 当前进度与最新交接卡片

[IbInput 合流证据](ibinput_mcp_integration_validation.md)保留 IB-MCP-138 固定输入和 139 Popup 选验。

历史同机暂停与双机接续见本文件 Git 历史及正文。10-30 PERSIST-136 为 PC-W11，10-31 Forza 同步为 PC-W10。

| 项目 | 当前记录 |
|---|---|
| 当前切片 | `REVIEW-FIX-267-272-R1 [Gemini]`（基于 `ee661de3`，完成 `20261127_267_272` r1 热键草稿保留、`style_code` 4096 往返上限与主计划锚点／FakerInput 口径修复及定向补验，待 Codex 复审） |
| 同步与评审锚点 | **上次已审／远端基线**：`af36398f`（`origin/codex/qkeymapper-mcp-control`：含 `f591483f..af36398f` 4 条通用评审技能提交经 `20261127_codex_review_skill` r3 `PASS_WITH_P2_P3` 收口并已推送；MCP 前序 `dd21d790..8cbf5eb3` 18 条 r3 PASS 与 `f591483f` 纯文档精简 `PASS_WITH_P2_P3` 均已推送，MCP 代码终点 `9560d278`，历史全文固定锚点 `8cbf5eb3`／`af36398f`）。<br>**待 Codex 验收（Gemini 切片）**：`DOC-TRIM-266-P2`（`92b5dd67`）、`UI-FLOAT-267`（`e661bfff`）、`UI-FLOAT-SYNC-268`（`4d6f4974`）、`SET-GENERAL-UI-269`（`fdf44854`）、`SET-GENERAL-BEHAVIOR-270`（`f95e419c`）、`DOC-TRIM-270`（`f4d910e6`）、`SET-MAPPING-HOTKEYS-271`（`7febd7f9`）、`SET-GYRO-PARAMS-272`（`ee661de3`）、`REVIEW-FIX-267-272-R1`（本轮 r1 修复提交，具体 code/doc 终点以 `out/codex_review/LATEST.md` 与 `03_gemini_response_r1.md` 冻结范围为准）；此前 `257～266` 与 `DOC-TRIM-266` 均已在远端基线收口 |
| 执行机器 | `PC-W11`（Qt 6.8.3／Qt 5.15.2、MSVC、Rust Sidecar）。Win10 正式 Release 与跨系统验收仍待 |
| 目标分支／接纳点 | `codex/qkeymapper-mcp-control`，`D:/work/mygithub/QKeyMapper/QKeyMapper`；仅本地 commit，严禁推送 |
| 核心基线 | `RUST-D4` `349d889c`；`154` `8f902531`；`155` `c9283438`；`IbInput` `e73f719b`（`MCP 94f0bb84` ＋ 固定输入 `b5b484a6`）；**历史全文固定回溯基线 `8cbf5eb3`**（代码终点 `9560d278`） |
| 当前已完成 | 按里程碑汇总（维持 **M2-A 进行中**；`UI-001` 配置子项及 `UI-002 3/3`、`UI-003 3/3` 均仅计设置能力，视觉与外设单列；终验时按 [capability_matrix.md](capability_matrix.md) 逐项核对本地代码完成度）：<br>- **M1／DEMO／PERSIST 已选验基线**：`104～133`、`135～137B`（同登录）、`149～161`、`UI-73`（详见下方与锚点 `8cbf5eb3`）；<br>- **M2-A（进行中）**：排序 `162～167`、Tab 与行属性 `168～194`、触发 `195～196`、复制 `197～201`、宏与导入 `202～212`、单表／普通组传输 `215/216/218～220/222～224/226/228～231`；<br>- **M2-B（进行中）**：设置 `213/214/217/221/225/227/232/234～257`、`261`（面板）、`262`（双通知）、`263`（托盘）、`264`（悬浮窗）、`265`（准星 `UI-003`）、`266`（Tab 图片 `UI-002`，含 r1～r3 修复 `d2c742aa`/`5db982a9`/`9560d278`）、`267`（单行浮动按钮）、`268`（浮动按钮同步组与拖动坐标）、`269`（主窗口通用界面偏好）、`270`（主窗口通用行为设置）、`271`（启停热键与配置说明）、`272`（主窗口 Gyro2Mouse 8 项参数）；含 `267～272` r1 修复 `REVIEW-FIX-267-272-R1`；<br>- **M3-A（进行中，仅配置子项）**：阈值 `258`、触摸板 `259`、接受虚拟手柄输入 `260` |
| 当前验证／证据 | `REVIEW-FIX-267-272-R1` 与 `SET-GYRO-PARAMS-272`（`ee661de3`）本机选验完成；隔离产品运行副本统一禁用 FakerInput 分配与连接，开启 FakerInput 连接运行待验（见 `out/mcp_review_fix_267_272_r1_20261127/` 与 `out/mcp_product/review_fix_267_272_r1_20261127/run3/`） |
| 当前控制器 | 外部活跃受管控制器 PID `45148`（截止 `2026-11-27T19:00:00+08:00`），`run3` 后复查 `owned_targets=[]`、`unresolved_starts=[]`、`registered_targets=[]`、`scope={}` |
| 下一项动作 | 等待 Codex 对批次 `20261127_267_272`（前序功能 `af36398f..ee661de3` + 本轮 r1 修复，冻结范围与本地 commit SHA 见 [LATEST.md](../../out/codex_review/LATEST.md) 与 `03_gemini_response_r1.md`）执行 r1 复审；评审收口后再规划 `SET-VJOY-MOUSE-273` |
| 剩余估算与外部待办 | 剩余有效作业时间沿用第 3 节（原 60–106 小时，预留 75–135 小时 ＋ DEMO 待估）。外部待办：**PC-W11**：开启 FakerInput 连接、OS 文件级 symlink、实际视觉绘制与托盘/通知/悬浮窗/面板/准星/浮动按钮渲染及拖动/穿透热键、原生文件选择器、Qt5 全量组件及整体产品、真实物理外设与虚拟手柄反馈、`PERSIST-137` 跨登录／重启、`TARGET-DESIGN-47`/`DESIGN-58` 多实例前台绑定与整窗提示、`IbInput` 主动连接检查／试发、`AGY-160` 整包与同轮图像消费；**PC-W10**：Win10 正式 Release／三包与 PE 烟测、跨系统等价性复验、hermes／QQBot gateway 联调 |

### REVIEW-FIX-267-272-R1 [Gemini]：批次 `20261127_267_272` r1 发现修复与定向补验

- **源码基线**：`ee661de3`；按 `out/codex_review/20261127_267_272/02_codex_review_r1.md` 第 3 节完成 3 项修复：
  1. **`[P1]` 热键草稿覆盖**：在 [qkeymapper.h](../../QKeyMapper/qkeymapper.h) / [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp) 为 `applyGeneralUiPreferencesSettings()`（`includeWindowSwitchKey = true`）与 `applyMappingHotkeysSettings()`（`includeStartKey = true, includeStopKey = true`）增加按字段条件写回开关，MCP apply 分支按 `.has_value()` 仅写回显式请求的对应热键输入框，省略的热键草稿文本与未提交状态完全保留，同时保留显式请求与聚焦草稿冲突时的 `context_busy` 守卫。
  2. **`[P2]` 浮动按钮样式码往返上限**：因 `font_family` 255 UTF-16 代码单元（三字节汉字 UTF-8 百分号编码 2,295 字符）叠加其余 style token 最坏可达 ~2,560 字符，统一将 [qkm_mapping_changes.cpp](../../QKeyMapper/automation/qkm_mapping_changes.cpp)、[tool_args.rs](../../mcp_bridge_rust/src/tool_args.rs)、[tool_catalog.json](../../mcp_bridge_rust/src/tool_catalog.json)、[test_mcp_control.cpp](../../QKeyMapper/tests/mcp_control/test_mcp_control.cpp) 与 [test_tool_contract.py](../../mcp_bridge_rust/tests/test_tool_contract.py) 的 `style_code` 有界输入上限调整为 `4096` UTF-16 代码单元（`mergeFloatingButtonSettings()` 解码后仍校验 `font_family <= 255` UTF-16）。
  3. **`[P2]` 主计划锚点与口径**：固定 `SET-GYRO-PARAMS-272` 提交哈希为 `ee661de3`，移除交接卡片中浮动 `af36398f..HEAD` 表述并将复审范围绑定至 [LATEST.md](../../out/codex_review/LATEST.md) 与 `03_gemini_response_r1.md` 冻结端点；交接卡片验证口径同步补全为“运行副本禁用 FakerInput 分配与连接，开启 FakerInput 连接运行待验”。
- **变更文件**：`QKeyMapper/qkeymapper.{h,cpp}`、`QKeyMapper/automation/qkm_mapping_changes.cpp`、`QKeyMapper/tests/mcp_control/test_mcp_control.cpp`、`mcp_bridge_rust/src/{tool_args.rs,tool_catalog.json}`、`mcp_bridge_rust/tests/test_tool_contract.py`、`handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md`。
- **构建与定向补验（PC-W11）**：
  - 官方 Qt6 Release `out/build_qt6/release/QKeyMapper.exe` `3E065B8D`（零警告）；部署 Rust Sidecar `QKeyMapper/build/mcp_bridge/x86_64-pc-windows-msvc/release/qkeymapper-mcp-rust.exe` `63B49E5A`；`cargo +stable test` lib `23 passed; 0 failed; 1 ignored` + bin `36 passed; 0 failed; 0 ignored`，`test_tool_contract.py --exe` 6/6 PASS。
  - Qt6 全量组件 `70B806DB`（零警告，私有桌面 `QkmMcp_6c75a8a986a7461b992ee207c00076b7`）：`90 passed, 0 failed, 0 skipped, 7684ms`；Qt5 定向组件 `8347E1D6`（零警告，私有桌面 `QkmMcp_2123b4240f194f299a43cc8cd2b205cd`）：`5 passed, 0 failed, 0 skipped, 380ms`。
  - 隔离产品定向补验 `QKeyMapperMcpAcceptance_ReviewFixR1.exe` `324A69A1`（零警告，`ALL_NON_SYSTEM_DLLS_PRESENT`，禁用 FakerInput 分配与连接）在 `out/mcp_product/review_fix_267_272_r1_20261127/run3/`（PID `45148`，私有桌面 `QkmProduct_c3fb1f11c8474840af9d92fa138f2434`）全通过：Scenario A（省略 `window_switch_key` 保留 `"DraftWindowKey"`，显式请求拒 `context_busy`）、Scenario B1/B2（仅改 `mapping_start_key` 保留 `"DraftStopKey"`，仅改 `mapping_stop_key` 保留 `"DraftStartKey"`，对应显式请求均拒 `context_busy`）、Scenario C（255 汉字 `font_family` 生成 2,465 字符 `style_code` 并往返应用至启用可见行 `row:0:2` 读回一致，4,097 字符被 Schema/Rust 拒接，seed INI 零改盘）。另记 `run1`/`run2` 仅因验证脚本自身状态遗漏（未在 B2 前还原 `committed_stop`）及目标行/查询结构误配（`row:0:1` 禁用隐藏行及旧 `floating_button` 键路径）失败，期间 C++ 与 Rust 产品源码零二次修改。
- **未验范围**：开启 FakerInput 分配与连接运行、屏幕视觉渲染与拖动热键交互、Qt5 全量组件及整体产品、Win10 正式 Release／三包。本机选验完成，待 Codex 复核。

### SET-GYRO-PARAMS-272 [Gemini]：主窗口 `Gyro2Mouse` 8 项参数 `configuration.set_gyro2mouse_params` 与 `get_state.gyro2mouse_params`

- **源码基线**：`7febd7f9`；在 `QKeyMapper.h`/`.cpp` 新增 `struct Gyro2MouseParamsSettings`（8 项 per-profile 浮点参数：`xSpeed`/`ySpeed` `[0.0, 99.99]` 2 位小数、`minXSensitivity`/`maxXSensitivity`/`minYSensitivity`/`maxYSensitivity` `[0.0, 200.0]` 1 位小数、`minThreshold`/`maxThreshold` `[0.0, 500.0]` 1 位小数）及共用辅助 `getGyro2MouseParamsSettings()`、`applyGyro2MouseParamsSettings()`（写入前临时解除 3 对 `min`/`max` 互锁边界并恢复，按实际变化标 dirty）、`resetGyro2MouseParamsToDefaults()`，并重构 `loadKeyMapSetting()` 与 `loadEmptyMapSetting()` 复用同一原子 setter，顺带修复冷启／切组先写高段 `Min*` 值被默认上限 `15.0` 静默截断的隐患；在 `qkm_mapping_changes.{h,cpp}` 新增 `Gyro2MouseParamsUpdate`、`__declspec(noinline)` `parseGyro2MouseParamsUpdate()` 与 `appendGyro2MouseParamsCapabilities()`、`configuration.set_gyro2mouse_params.{preview,apply}`（`160` 标准 / `152` 演示，允许 Common Tab、检测 `hasFocus() && !hasAcceptableInput()` 拒为 `context_busy`、合并当前值后成对校验 `min <= max` 拒绝静默 clamp）并在 `collectAutomationState()` 零磁盘 I/O 暴露 `gyro2mouse_params`；同步更新 Rust Sidecar（`tool_args.rs`、`tool_catalog.json`、`test_tool_contract.py`）与 `test_mcp_control.cpp`（`verifyGyro2MouseParamsValidation()`）。
- **变更文件**：`QKeyMapper/qkeymapper.{h,cpp}`、`QKeyMapper/automation/qkm_mapping_changes.{h,cpp}`、`QKeyMapper/tests/mcp_control/test_mcp_control.cpp`、`mcp_bridge_rust/src/{tool_args.rs,tool_catalog.json}`、`mcp_bridge_rust/tests/test_tool_contract.py`、`handoff/qkeymapper_mcp_control/{qkeymapper_mcp_automation_plan_codex.md,capability_matrix.md}`。
- **构建产物（SHA256 前 8 位）**：Qt6 Release `out/build_qt6/release/QKeyMapper.exe` `72910580`（零警告）、部署 Rust Sidecar `QKeyMapper/build/mcp_bridge/x86_64-pc-windows-msvc/release/qkeymapper-mcp-rust.exe` `847E6FEC`、Qt6 组件 `651F9794`（零警告）、Qt5 组件 `930C880F`（零警告）、隔离产品 `QKeyMapperMcpAcceptance_SetGyro2MouseParams.exe` `71ED4085`（零警告，`ALL_NON_SYSTEM_DLLS_PRESENT`；**禁用 FakerInput 分配与连接，开启 FakerInput 连接运行待验**）、双 Qt INI 读回器 `read_gyro2mouse_params.exe`（Qt 6.8.3 `59EE1FAE`／Qt 5.15.2 `345132FD`，零警告）。
- **测试与验证覆盖**：`cargo +stable test`（lib `23 passed; 0 failed; 1 ignored` + bin `36 passed; 0 failed; 0 ignored`）与 `test_tool_contract.py --exe` 6/6 PASS；Qt6 全量组件 `90 passed, 0 failed, 0 skipped, 8223ms`（`QkmMcp_843941f8adf64062ac968549c6eb6ddb`）、Qt5 定向组件 `5 passed, 0 failed, 0 skipped, 445ms`（`QkmMcp_22748c2717584d88b864b9b1825dc451`）；私有桌面验收 `out/mcp_product/set_gyro2mouse_params_272_20261127/run4/`（PID `45148`，5 桌面：`gui_baseline` `QkmProduct_a9a2eed0f6c34769b43b372a44d38155`、`readonly` `QkmProduct_313dc334ed9349508a470aead975048d`、`standard_flow` `QkmProduct_e41575f63e9b4c61ad65e31c6e26dd2b`、`cold_gyro2mouse_params` `QkmProduct_c030588d60bc4b57bfcf03dae0d52635`、`demo` `QkmProduct_a4682b3668d34b5e9abc8af3d9cfce29`）全 6 阶段通过：覆盖 GUI 8 个 SpinBox 编辑与 `Min<=Max` 联动及 dirty 置位（`ini_unchanged=true`）、160 项 Read 守卫、Schema/超小数位/越界/同请求与合并现值后 `min > max` 拒接、预览取消、`-0.0` 归零与重复 `modified=false`、高区间跃升（`min_x=30.5 > 默认max 15.0` 等不被截断）／低区间回落（`max < 旧min` 不被截断）、`QKeyMapperGlobalSetting` 与 Common Tab 各自保存隔离参数、冷重启无截断回读、双 Qt INI 读回（`McpSaved` 与 `QKeyMapperGlobalSetting` 均 `contains_all=true`, `changed_other_keys=0`, `diff_keys=[]`, `unchanged_other_keys=422`）及 `--demo-configuration-only`（152 项不写盘）。
- **失败重试与恢复记录**：`run1` 因夹具在 `confirm_conflicts=true` 拒绝（已消费预览）后复用同 `preview_id` 调 `qkm_cancel_changes` 报错；`run2` 因 `formal-seed.ini` 初态无 `QKeyMapperGlobalSetting` 节导致基线比对多出 165 项全球默认键；`run3` 遇 `out/gsudo_cache/results/*.json` 瞬时共享 `PermissionError` 并在清理遗留授权 `scope` 后重跑；C++ 与 Rust 产品源码零二次修改，`run4` 全部 6 阶段一次通过。
- **未验范围**：实体手柄陀螺仪映射运行态鼠标位移、未提交 SpinBox 焦点冲突 `context_busy` 实时触发、开启 FakerInput 连接运行、Qt5 全量组件与整体产品、Win10 正式 Release／三包。本机选验完成，待 Codex 复核。

### SET-MAPPING-HOTKEYS-271 [Gemini]：映射启停热键 `configuration.set_mapping_hotkeys` 与配置说明 `configuration.set_description`

- **源码基线**：`f4d910e6`（代码承接 `f95e419c`）；在 `QKeyMapper.h`/`.cpp` 新增 `struct MappingHotkeysSettings` 与共用辅助 `getMappingHotkeysSettings()`、`applyMappingStartKeySetting()`、`applyMappingStopKeySetting()`、`applyMappingHotkeysSettings()`、`isProfileDescriptionEditable()`、`getProfileDescriptionSetting()`、`applyProfileDescriptionSetting()`，重构 `onHotKeyLineEditEditingFinished()` 中 `MAPPINGSTARTKEY_LINEEDIT_NAME`/`MAPPINGSTOPKEY_LINEEDIT_NAME` 复用共用 setter（同步 `updateSysTrayIconMenuText()` 与 dirty 状态）；在 `qkm_mapping_changes.{h,cpp}` 新增 `MappingHotkeysUpdate`、`parseMappingHotkeysUpdate()`、`appendProfileHotkeysAndDescriptionCapabilities()`、`configuration.set_mapping_hotkeys.{preview,apply}`（`mapping_start_key`/`mapping_stop_key` 经 `analyzeHotKeyLineEditText()` 校验与规范化，聚焦未提交态拒为 `context_busy`，普通/空白/Common/`GlobalSetting` 均可编辑）与 `configuration.set_description.{preview,apply}`（单行 ≤1024 UTF-16，允许 `""` 清除；`QKeyMapperGlobalSetting` 拒为 `unsupported_capability`）（`158` 标准 / `150` 演示），并在 `collectAutomationState()` 零磁盘 I/O 暴露 `mapping_hotkeys` 与 `profile_description`；同步更新 Rust Sidecar（`tool_args.rs`、`tool_catalog.json`、`test_tool_contract.py`）与 `test_mcp_control.cpp`（`verifyProfileHotkeysAndDescriptionValidation()`）。
- **变更文件**：`QKeyMapper/qkeymapper.{h,cpp}`、`QKeyMapper/automation/qkm_mapping_changes.{h,cpp}`、`QKeyMapper/tests/mcp_control/test_mcp_control.cpp`、`mcp_bridge_rust/src/{tool_args.rs,tool_catalog.json}`、`mcp_bridge_rust/tests/test_tool_contract.py`、`handoff/qkeymapper_mcp_control/{qkeymapper_mcp_automation_plan_codex.md,capability_matrix.md}`。
- **构建产物（SHA256 前 8 位）**：Qt6 Release `out/build_qt6/release/QKeyMapper.exe` `0D2D89DB`（零警告）、Rust Sidecar `QKeyMapper/build/mcp_bridge/x86_64-pc-windows-msvc/release/qkeymapper-mcp-rust.exe` `3ED57176`、Qt6 组件 `CD8F1C8D`（零警告）、Qt5 组件 `4EAEC9A5`（零警告）、隔离产品 `QKeyMapperMcpAcceptance_SetMappingHotkeys.exe` `ABFCBC1B`（零警告，`ALL_NON_SYSTEM_DLLS_PRESENT`；**禁用 FakerInput 分配与连接，开启 FakerInput 连接运行待验**）、双 Qt INI 读回器 `read_hotkeys.exe`（Qt 6.8.3 `4F65CFD2`／Qt 5.15.2 `610E02AC`，零警告）。
- **测试与验证覆盖**：`cargo +stable test`（lib `23 passed; 0 failed; 1 ignored` + bin `35 passed; 0 failed; 0 ignored`）与 `test_tool_contract.py` 6/6 PASS；Qt6 全量组件 `90 passed, 0 failed, 0 skipped, 8324ms`（`QkmMcp_323c32877f834683b4c7dfacf4f17223`）、Qt5 定向组件 `5 passed, 0 failed, 0 skipped, 386ms`（`QkmMcp_241c2b78ed024132bd2986b7edee9392`）；私有桌面验收 `out/mcp_product/set_mapping_hotkeys_271_20261127/run3/`（PID `45148`，5 桌面：`gui_baseline` `QkmProduct_39a6c581b2a643c6a6219637741dbd55`、`readonly` `QkmProduct_743e5ceeccc242c8b7240329a66c2d54`、`standard_flow` `QkmProduct_944f6fe8b7bd4c9cb68ce505e583aea4`、`cold_hotkeys` `QkmProduct_c33480c9d57249009bd21de05c5347b5`、`demo` `QkmProduct_9caf40f19adc4c738e2cb3924c94724d`）全 6 阶段通过：覆盖 GUI 非法热键回滚（`dirty=false`）、`" L-Ctrl + F8 "`/`" L-Shift + F9 "` 规范化与 `descriptionLineEdit` 编辑（`after_dirty=true`, `ini_unchanged=true`）、158 项 Read 守卫、Schema/非法热键拒接、预览取消、零盘写入、`run.actual_stop_shortcut` 同步、重复 `modified=false`、Unicode/emoji 说明设/清与 Common Tab 编辑、全局组禁改说明 `unsupported_capability` 与改热键、冷启与双 Qt INI 读回（`contains_all=true`, `mapping_start_key="L-Ctrl+F8"`, `mapping_stop_key="L-Alt+F11"`, `description="FinalDesc说明🎮"`, `changed_other_keys=0`, `diff_keys=[]`, `unchanged_other_keys=259`）与 `--demo-configuration-only`（150 项不写盘）。`run1`/`run2` 为夹具默认键值（`L-Ctrl+F6`/`Synthetic shared description`）及 `read_hotkeys` 键名拼写误用 `MappingStart_Key`；产品源码零二次修改，`run3` 一次全过。
- **未验范围**：真实映射中开始/停止全局快捷键物理触发、未提交输入框焦点冲突 `context_busy`、托盘气泡/菜单热键实际提示展示、开启 FakerInput 连接运行、Qt5 全量组件与整体产品、Win10 正式 Release／三包。本机选验完成，待 Codex 复核。

### SET-GENERAL-BEHAVIOR-270 [Gemini]：主窗口通用行为 `general.set_behavior` 与 `get_state.general_behavior`

- **源码基线**：`fdf44854`；在 `QKeyMapper.h`/`.cpp` 新增 `struct GeneralBehaviorSettings`（`startupAutoMonitoring`、`closeToSystemTray`、只读 `closeToSystemTrayDecided`、`mappingStartActionMode` `0..1`；`autoStartupCheckBox` 因调用 `schtasks` 系统级计划任务副作用明确排除）及共用函数 `getGeneralBehaviorSettings()`、`applyGeneralBehaviorSettings()`；为 `setMappingStartActionMode()` 与 `closeToSystemTrayCheckBox` 点击连接补 `!QkmAutomation::denyDemoEffect()`，并同步 `m_CloseToSystemTrayDecided`/`m_PendingCloseToSystemTrayDecided`（加载清零、仅真变更写回内存并待显式保存写入 `CLOSETO_SYSTEMTRAY_DECIDED=true`）；在 `qkm_mapping_changes.{h,cpp}` 新增 `general.set_behavior.{preview,apply}`（`154` 标准 / `146` 演示）与 `get_state.general_behavior`（零磁盘 I/O）；同步更新 Rust Sidecar（`tool_args.rs`、`tool_catalog.json`、`test_tool_contract.py`）与 `test_mcp_control.cpp`。
- **变更文件**：`QKeyMapper/qkeymapper.{h,cpp}`、`QKeyMapper/automation/qkm_mapping_changes.{h,cpp}`、`QKeyMapper/tests/mcp_control/test_mcp_control.cpp`、`mcp_bridge_rust/src/{tool_args.rs,tool_catalog.json}`、`mcp_bridge_rust/tests/test_tool_contract.py`、`handoff/qkeymapper_mcp_control/{qkeymapper_mcp_automation_plan_codex.md,capability_matrix.md}`。
- **构建产物（SHA256 前 8 位）**：Qt6 Release `out/build_qt6/release/QKeyMapper.exe` `314B1ACA`（零警告）、Rust Sidecar `QKeyMapper/build/mcp_bridge/x86_64-pc-windows-msvc/release/qkeymapper-mcp-rust.exe` `00F24C90`、Qt6 组件 `742A90E8`（零警告）、Qt5 组件 `5ED25E66`（零非豁免警告）、隔离产品 `QKeyMapperMcpAcceptance_SetGeneralBehavior.exe` `2098B377`（零警告，`ALL_NON_SYSTEM_DLLS_PRESENT`；**禁用 FakerInput 分配与连接，开启 FakerInput 连接运行待验**）、双 Qt INI 读回器 `read_general_behavior.exe`（Qt 6.8.3 `257A16B1`／Qt 5.15.2 `F71F8F54`，零警告）。
- **测试与验证覆盖**：`cargo +stable test`（lib `23 passed; 0 failed; 1 ignored` + bin `34 passed; 0 failed; 0 ignored`）与 `test_tool_contract.py` 6/6 PASS；Qt6 全量组件 `90 passed, 0 failed, 0 skipped, 8026ms`、Qt5 定向组件 `5 passed, 0 failed, 0 skipped, 576ms`；私有桌面验收 `out/mcp_product/set_general_behavior_270_20261127/run3/`（PID `45148`，5 桌面：`gui_baseline` `QkmProduct_6aee1ecb57084d36a2cf971e52513af6`、`readonly` `QkmProduct_43995b8fc032403899fab3df902dc2da`、`standard_flow` `QkmProduct_83e0b9af2a4e4b02b09092d9e196ddb1`、`cold_general_behavior` `QkmProduct_fa048fc4151d4e10b578a6d81c6f7bad`、`demo` `QkmProduct_eac0037c600f48e898cd686930c955ac`）全 6 阶段通过：覆盖 GUI 控件与启动模式文案切换（`0/1`，`after_dirty=true`）、154 项 Read 守卫、Schema 拒接、预览取消、未决→已决不写盘与重复 `modified=false`、Common Tab 三项联动（`true,true,1`）、显式保存与 `final`（`false,false,1`）冷启、`QKeyMapperGlobalSetting` 切换、双 Qt INI 读回（`automon_true` 与 `final` 均 `contains_all=true`, `changed_other_keys=0`, `diff_keys=[]`, `unchanged_other_keys=258`）与 `--demo-configuration-only`（146 项不写盘）。`run1`/`run2` 为验收夹具未决种子/运行期写盘触发 `context_busy` 及 `CloseToSystemtray_Decided=false` 弹窗卡住 `WM_CLOSE`（用户 `taskkill /F /PID 43836` 并注销槽位）；产品源码零二次修改，`run3` 一次全过。
- **未验范围**：`autoStartupCheckBox`（`schtasks`）、首次关闭弹窗交互、开启自监控冷启自动监测、`CloseToSystemtray=true` 实际隐入托盘、开启 FakerInput 连接运行、Qt5 全量组件与整体产品、Win10 正式 Release／三包。本机选验完成，待 Codex 复核。

### SET-GENERAL-UI-269 [Gemini]：主窗口通用界面偏好 `general.set_ui_preferences` 与 `get_state.general_ui_preferences`

- **源码基线**：`4d6f4974`；在 `QKeyMapper.h`/`.cpp` 新增 `struct GeneralUiPreferencesSettings` 与共用辅助 `getGeneralUiPreferencesSettings()`、`applyProcessListVisibleSetting()`、`applyWindowSwitchKeySetting()`、`applyGeneralUiPreferencesSettings()`，重构 `onHotKeyLineEditEditingFinished()` 的 `WINDOWSWITCHKEY_LINEEDIT_NAME` 分支复用同一写回与 dirty 逻辑；在 `qkm_mapping_changes.{h,cpp}` 新增 `GeneralUiPreferencesUpdate`、`parseGeneralUiPreferencesUpdate()`、`appendGeneralUiPreferencesCapabilities()` 与 `general.set_ui_preferences.{preview,apply}`（`152` 标准 / `144` 演示，`language` `0..2`、`theme` `0..2`、`notification_position` `0..6`、`show_process_list` bool、`update_site` `0..1`、`startup_minimized` bool、`window_switch_key` 经 `analyzeHotKeyLineEditText()` 校验与规范化；允许在 Common Tab 编辑，检测 `windowswitchkeyLineEdit` 未提交编辑态防 `context_busy`，真实变化递增 `m_AutomationSourceEpoch` 并标 dirty），并在 `collectAutomationState()` 零磁盘 I/O 暴露 `general_ui_preferences`；同步更新 Rust Sidecar（`tool_args.rs`、`tool_catalog.json`、`test_tool_contract.py`）与 `test_mcp_control.cpp`（新增 MSVC `noinline` `verifyGeneralUiPreferencesValidation()`）。
- **变更文件**：`QKeyMapper/qkeymapper.{h,cpp}`、`QKeyMapper/automation/qkm_mapping_changes.{h,cpp}`、`QKeyMapper/tests/mcp_control/test_mcp_control.cpp`、`mcp_bridge_rust/src/{tool_args.rs,tool_catalog.json}`、`mcp_bridge_rust/tests/test_tool_contract.py`、`handoff/qkeymapper_mcp_control/{qkeymapper_mcp_automation_plan_codex.md,capability_matrix.md}`。
- **构建产物（SHA256 前 8 位）**：Qt6 Release `out/build_qt6/release/QKeyMapper.exe` `82C7A474`（零警告）、部署 Rust Sidecar `QKeyMapper/build/mcp_bridge/x86_64-pc-windows-msvc/release/qkeymapper-mcp-rust.exe` `DEC4B419`、Qt6 组件 `QKeyMapperMcpControlTest.exe` `A3772351`（零警告）、Qt5 组件 `QKeyMapperMcpControlTest.exe` `923B02E4`（零警告）、隔离产品 `out/mcp_product/set_general_ui_269_20261127/runtime/QKeyMapperMcpAcceptance_SetGeneralUi.exe` `545F8FC5`（零警告，`ALL_NON_SYSTEM_DLLS_PRESENT`；**运行副本禁用 FakerInput 分配与连接，开启 FakerInput 连接运行待验**）、双 Qt INI 读回器 `read_general_ui.exe`（Qt 6.8.3 `53F167DC`／Qt 5.15.2 `CFC14A11`，零警告）。
- **测试与验证覆盖**：`cargo +stable test`（lib `23 passed; 0 failed; 1 ignored` + bin `33 passed; 0 failed; 0 ignored`）与 `test_tool_contract.py` 6/6 PASS；Qt6 全量组件（私有桌面 `QkmMcp_6b4a1ceaf68d48a494705ce6cb6cd81e`）**90 passed, 0 failed, 0 skipped, 8701ms**，Qt5 定向组件（私有桌面 `QkmMcp_a7ec4e0f3d1f449faa6584ccacb210a9`）**5 passed, 0 failed, 0 skipped, 498ms**；私有桌面验收 `out/mcp_product/set_general_ui_269_20261127/run6/`（控制器 PID `45148`，5 桌面：`gui_baseline` `QkmProduct_89351bc9aa8d4e348989b7048e64a41e`、`readonly` `QkmProduct_1e94b8ce5a8d49b393ab9252f678d2b2`、`standard_flow` `QkmProduct_fc0eabd57ab44f79b7894be6423ee258`、`cold_general_ui` `QkmProduct_01927672b98c4997b8f8e6bf50fca0f4`、`demo` `QkmProduct_d755dd7c2cde4c9283fd7aaab233afd6`）全 6 阶段退出 0：覆盖 GUI 7 控件与中→英→中 Common Tab 名刷新及非法热键回滚（`ini_unchanged=true`）、152 项 Read 守卫、Schema/非法热键拒接、预览取消、`0->1->2->1` 语言与 7 项组合应用（`" L-Ctrl + F7 "`→`"L-Ctrl+F7"`）、无变更 `modified=false`、Common Tab 与 `QKeyMapperGlobalSetting` 切换编辑、显式保存与冷启读回、双 Qt INI 读回 `min_true`/`final` 两份快照（`contains_all=true`, `language=1`, `theme=2`, `notification_position=5`, `show_process_list=false`, `update_site=1`, `startup_minimized=true/false`, `window_switch_key="L-Ctrl+F7"`, 仅预期两项语言联动键 `IgnoreWindowInfo_Rules` 与 `McpSaved/MappingTable_TabNameList` (`Tab1, Tab2, Tab3, Common`) 变化，`unchanged_other_keys=253`）及 `--demo-configuration-only`（144 项不写盘）。
- **失败重试与恢复记录**：`test_mcp_control.cpp` 初次编译因测试签名类型 `const QJsonObject &` 误写修正后通过；产品验收 `run1～run5` 分别修正脚本键名与加固 INI 读回器 `--out` 落盘通道（`53F167DC`/`CFC14A11` 通过 20×2 压测），产品 C++/Rust 源码首次构建后未做二次改动，`run6` 全 6 阶段一次通过。
- **未验范围**：实际系统主题视觉观感、开机启动最小化真实托盘落态、在线更新网络请求、`windowswitchkeyLineEdit` 未提交焦点冲突 `context_busy` 实时触发、开启 FakerInput 连接运行、Qt5 非定向全部用例与整体产品、Win10 正式 Release／三包。本机选验完成，待 Codex 复核。

### UI-FLOAT-SYNC-268 [Gemini]：浮动按钮同步组说明、拖动坐标开关与同步组偏移联动

- **源码基线**：`e661bfff`；在 `QKeyMapper.h`/`.cpp` 新增 `struct FloatingButtonSyncGroupOffsetChange` 及共用函数 `applyFloatingButtonDragCoordinateSetting()`、`setFloatingButtonSyncGroupNoteSetting()`（UTF-16 ≤1024、禁止换行、空串清除）、`planFloatingButtonSyncGroupOffsetDelta()` 与 `applyFloatingButtonSyncGroupOffsetDelta()`（`group_id` `1..999`、`delta_x`/`delta_y` `-100000..100000` 且至少一项非零、严格复刻 GUI 与生效 Common Tab 双向联动及 `-50000..50000` 夹紧，真实坐标变化方递增 `m_AutomationSourceEpoch` 并标 dirty）；重构 `toggleDragCoordAction` 与 `QFloatingButtonSetupDialog` 的 `m_ShowDragCoordinateCheckBox`（加 `m_isLoading` 守卫）、`m_SyncGroupNoteLineEdit`、`syncGroupNoteFromData()`（加 `QSignalBlocker`）与 `applySyncGroupOffsetDelta()` 汇入同一共用逻辑；在 `qkm_mapping_changes.{h,cpp}` 新增 `general.set_floating_button_drag_coordinate.{preview,apply}`、`configuration.set_floating_button_sync_group_note.{preview,apply}`（需已加载配置组 match）与 `mapping.move_floating_button_sync_group.{preview,apply}`（`150` 标准 / `142` 演示，无匹配成员拒为 `invalid_target`），并在 `collectAutomationState()` 零磁盘 I/O 暴露排序后的 `floating_button_settings`；同步更新 Rust Sidecar（`tool_args.rs`、`tool_catalog.json`、`test_tool_contract.py`）与 `test_mcp_control.cpp`（新增 MSVC `noinline` `verifyFloatingButtonSyncValidation()`）。
- **变更文件**：`QKeyMapper/qkeymapper.{h,cpp}`、`QKeyMapper/qfloatingbuttonsetupdialog.cpp`、`QKeyMapper/automation/qkm_mapping_changes.{h,cpp}`、`QKeyMapper/tests/mcp_control/test_mcp_control.cpp`、`mcp_bridge_rust/src/{tool_args.rs,tool_catalog.json}`、`mcp_bridge_rust/tests/test_tool_contract.py`、`handoff/qkeymapper_mcp_control/{qkeymapper_mcp_automation_plan_codex.md,capability_matrix.md}`。
- **构建产物（SHA256 前 8 位）**：Qt6 Release `out/build_qt6/release/QKeyMapper.exe` `FC0CA4C6`（零警告）、部署 Rust Sidecar `QKeyMapper/build/mcp_bridge/x86_64-pc-windows-msvc/release/qkeymapper-mcp-rust.exe` `FCB17C75`、Qt6 组件 `QKeyMapperMcpControlTest.exe` `4E4D8A75`（零警告）、Qt5 组件 `QKeyMapperMcpControlTest.exe` `555B03F4`（零警告）、隔离产品 `out/mcp_product/set_floating_button_sync_268_20261127/runtime/QKeyMapperMcpAcceptance_SetFloatingButtonSync.exe` `8F5E9206`（零警告，`ALL_NON_SYSTEM_DLLS_PRESENT`；**运行副本禁用 FakerInput 分配与连接，开启 FakerInput 连接运行待验**）、双 Qt INI 读回器 `read_floating_button_sync.exe`（Qt 6.8.3 `F9D9FB50`／Qt 5.15.2 `DCFFE969`，零警告）。
- **测试与验证覆盖**：`cargo +stable test`（lib `23 passed; 0 failed; 1 ignored` + bin `32 passed; 0 failed; 0 ignored`）与 `test_tool_contract.py` 双协议 6/6 PASS；Qt6 全量组件在私有桌面 `QkmMcp_4561885435ad48b19c4c9380f6cafea6` **90 passed, 0 failed, 0 skipped, 8614ms**，Qt5 定向组件在私有桌面 `QkmMcp_3afd1378c12b42eca99445f97e15c508` **5 passed, 0 failed, 0 skipped, 482ms**；隔离产品验收 `out/mcp_product/set_floating_button_sync_268_20261127/run4/`（外部控制器 PID `45148`，5 个私有桌面：`gui_baseline` `QkmProduct_08685eb181cc48809fdd919338b892a0`、`readonly` `QkmProduct_cba5ec97145a4209bd0aa78dce102360`、`standard_flow` `QkmProduct_e4101dfc26fc400f98d5becf90b95e51`、`cold_floating_button_sync` `QkmProduct_ee39cc31d6a14d7ea537299bae61c02a`、`demo` `QkmProduct_cdeee04d8de44774a1a31479d0d465ca`）全 6 阶段通过：覆盖 GUI 对话框初始零误脏与组 5 说明/全局拖动开关/同步组偏移编辑（`delta=(25,-10)`, `ini_unchanged=true`）、150 项 Read 守卫、Schema/缺失组 `invalid_target`/预览取消/`confirm_conflicts=true` 拒绝、重复无变更 `modified=false`、emoji 同步组说明有序读写与清除、Tab 0 ↔ Common 双向同步组移动与 `-50000..50000` 夹紧、关闭共通继承隔离、Common Tab 与 Tab 1 切换移动/夹紧无操作、全局组 `QKeyMapperGlobalSetting` 同步组说明、显式保存冷重启、双 Qt 6.8.3／5.15.2 INI 读回（`contains_all=true`, `show_drag_coordinate=false`, `sync_group_notes=[{"group_id":12,"note":"同步组😀"}]`, `tab0_rows[0]=(-49990,49995)`, `tab0_rows[2]=(80,220)`, `tab1_rows[0]=(7,-50000,50000)`, `common_rows[0]=(12,260,-370)`, `unchanged_other_keys=256`, `changed_other_keys=0`, `diff_keys=[]`）与 `--demo-configuration-only`（142 项能力且不写盘）。
- **失败重试与恢复记录**：`run1`（循环内提前缓存 `row2_ref` 致 `stale_source`）、`run2`（脚本误取 `cat["catalog_revision"]` 致 `KeyError`）与 `run3`（INI 读回器误加 `"General/"` 前缀且未链接 `Qt*Gui.dll` 导致 `[General]`/`CommonMappingData` 读回失败）均属验证脚本与读回器夹具缺陷，产品 C++ 与 Rust Sidecar 代码未改动；修正夹具后 `run4` 全部 6 阶段一次通过。
- **未验范围**：屏幕浮动按钮实际拖动坐标浮标视觉渲染、鼠标右键菜单触发、多屏 DPI 与边界拖动、开启 FakerInput 连接运行、Qt5 非定向全部用例与整体产品、Win10 正式 Release／三包。本机选验完成，待 Codex 复核。

### UI-FLOAT-267 [Gemini]：单条映射浮动按钮设定 `mapping.set_floating_button`

- **源码基线**：`92b5dd67`（代码承接 `9560d278`）；在 `QKeyMapper.h`/`.cpp` 新增 `struct MappingFloatingButtonSettings`（34 项持久化字段）及共用读写/校验/比较函数，同步写入 `FloatingButton_Opacity` 以兼容旧配置；重构 `QFloatingButtonSetupDialog::applyToCurrentItem()` 走同一共用写回路径并在 `hover_contrast_mode != CUSTOM` 时保护 `HoverCustomColor`；在 `qkm_mapping_changes.{h,cpp}` 新增 `MappingFloatingButtonUpdate`、`parseMappingFloatingButtonUpdate()`、`validateRowAppearancePreview()` 与 `mapping.set_floating_button.{preview,apply}`（`144` 标准 / `136` 演示，支持 34 字段子集与可选 `style_code` `FBS1;...` 解析叠加，拒绝 `source_tab_ref`、隐藏行、`confirm_conflicts=true` 及非自定义对比模式下传入 `hover_custom_color`；复用 `qkm_query(mappings)` 既有 35 项 `KeyMapData_FloatingButton_*` 读回）；`onFloatingButtonSettingsApplied()` 在 Common Tab 修改时递增 `m_AutomationSourceEpoch` 并刷新关联表；同步更新 Rust Sidecar（`tool_args.rs`、`tool_catalog.json`、`test_tool_contract.py`）与 `test_mcp_control.cpp`（并将 `McpControlTest::mappingChangeArguments()` 拆分为 5 个 MSVC `noinline` 辅助函数消除测试线程 1 MB 栈溢出 `0xC00000FD`）。
- **变更文件**：`QKeyMapper/qkeymapper.{h,cpp}`、`QKeyMapper/qfloatingbuttonsetupdialog.cpp`、`QKeyMapper/automation/qkm_mapping_changes.{h,cpp}`、`QKeyMapper/tests/mcp_control/test_mcp_control.cpp`、`mcp_bridge_rust/src/{tool_args.rs,tool_catalog.json}`、`mcp_bridge_rust/tests/test_tool_contract.py`、`handoff/qkeymapper_mcp_control/{qkeymapper_mcp_automation_plan_codex.md,capability_matrix.md}`。
- **构建产物（SHA256 前 8 位）**：Qt6 Release `out/build_qt6/release/QKeyMapper.exe` `6BFCDAB5`（零警告）、Rust Sidecar `QKeyMapper/build/mcp_bridge/x86_64-pc-windows-msvc/release/qkeymapper-mcp-rust.exe` `14BA7806`（`mcp_bridge_rust/target/release/qkeymapper-mcp-rust.exe` `0D6FD102`）、Qt6 组件 `QKeyMapperMcpControlTest.exe` `86697B36`（零警告）、Qt5 组件 `QKeyMapperMcpControlTest.exe` `C73A5576`（零警告）、隔离产品 `out/mcp_product/set_floating_button_267_20261127/runtime/QKeyMapperMcpAcceptance_SetFloatingButton.exe` `36C352B3`（零警告，`ALL_NON_SYSTEM_DLLS_PRESENT`；**运行副本禁用 FakerInput 分配与连接，开启 FakerInput 连接运行待验**）、双 Qt INI 读回器 `read_floating_button.exe`（Qt 6.8.3 `F3D02269`／Qt 5.15.2 `81B65528`，零警告）。
- **测试与验证覆盖**：`cargo +stable test`（lib `23 passed; 0 failed; 1 ignored` + bin `31 passed; 0 failed; 0 ignored`）与 `test_tool_contract.py` 双协议 6/6 PASS；Qt6 全量组件在私有桌面 `QkmMcp_fdf9781f71f3466d96eaefb8e0561ce3` **90 passed, 0 failed, 0 skipped, 7051ms**，Qt5 定向组件在私有桌面 `QkmMcp_2d032e02b433470e9a1cd89d7ecd58c3` **5 passed, 0 failed, 0 skipped, 337ms**；隔离产品验收 `out/mcp_product/set_floating_button_267_20261127/run1/`（外部控制器 PID `45148`，5 个私有桌面：`gui_baseline` `QkmProduct_e788f10caa9b49f8ae726deee3feda8a`、`readonly` `QkmProduct_ca02b009d1104c35867e60dac5bee4b3`、`standard_flow` `QkmProduct_7abe961125ac4b3f9da14c646b106f15`、`cold_floating_button` `QkmProduct_4ec6eeed70d94b8780403f9ab54d26e5`、`demo` `QkmProduct_61ab151056d7423f81e47c322b27f583`）全部 6 阶段一次通过：覆盖 `QFloatingButtonSetupDialog` 加载/编辑/Revert/dirty 基线（不写盘）、144 项 Read 守卫、34 字段边界/`"default"` 色值/`hover_contrast_mode!=3` 保护/`style_code` `FBS1;...` 叠加/无变更 `modified=false`/`source_tab_ref` 与 `confirm_conflicts=true` 拒绝、`Common` Tab 与全局组、显式保存冷重启、双 Qt 6.8.3／5.15.2 INI 读回（`contains_all=true`, `common_count=1`, `unchanged_other_keys=226`, `changed_other_keys=0`, `diff_keys=[]`）与 `--demo-configuration-only`（136 项能力且不写盘）。
- **失败重试与恢复记录**：组件单测初期在 Qt6 Release 下于 `McpControlTest::mappingChangeArguments()` 连续两轮触发 `0xC00000FD`（`verifyGamepadAndPanelValidation` 与 `verifyCapabilityListContracts -> mappingChangeCapabilities`）；经 `dumpbin /disasm` 实测产品函数 `mappingChangeCapabilities` 仅 `77.9 KiB`、原测试函数达 `945,160 B`，将测试辅助拆为 `previewRequestValid` 与 5 个 `noinline` 分段函数后双 Qt 组件一次全通；产品 `run1` 全 6 阶段一次通过。
- **未验范围**：屏幕浮动按钮实际渲染/悬停动画/拖动/穿透热键（其中鼠标穿透切换热键已在 `UI-FWINDOW-264` 归 Tab 级设置）、同步组说明/全局拖动坐标开关与多行同步组联动（归下一切片 `UI-FLOAT-SYNC-268`）、开启 FakerInput 连接运行、Qt5 非定向全部用例与整体产品、Win10 正式 Release／三包。本机选验完成，待 Codex 复核。

### 已推远端切片压缩索引（IGNORE-BUILTIN-DESC-FIX ／ SET-GYRO-257 ～ DOC-TRIM-266，全文固定锚点 `af36398f`）

> `DOC-TRIM-270 [Gemini]` 将已推远端基线 `af36398f` 收口的 `257～266`（含 `REVIEW-FIX-257-266` r1～r3 与 `DOC-TRIM-266`）压缩为此结构化索引，保持本文件留有充足 `≤ 256 KiB` 空间；原件逐行正文随时可通过 `git show af36398f:handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md` 完整取回。下列各项均在 `PC-W11` 完成选验，`out/` 路径均属本机 `.gitignore` 忽略产物，隔离产品副本均因启动前提示拦截而禁用了 FakerInput 分配与连接。

| 切片 | 基线 / 产物要点 | 接入能力／核心变更与关键验证结论（PC-W11 选验，已在远端 `af36398f` 收口） | 剩余未闭环 |
|---|---|---|---|
| `DOC-TRIM-266` | `8cbf5eb3` / `f591483f` | 压缩 `138～256`、`DEMO-104～133` 与 `PERSIST-135～136A` 并经 r1 P2 口径校准（区分 `PC-W10` 与 `PC-W11` 原件机，还原 `238`/`PERSIST-136A` 证据层级） | —（纯文档切片） |
| `REVIEW-FIX-257-266`（r1/r2/r3） | `d2c742aa` / `5db982a9` / `9560d278` | 句柄级 `qkm_safe_image_asset`（防 `FILE_ATTRIBUTE_REPARSE_POINT`/穿越/损坏及自包含 SVG 纯内存 `QXmlStreamReader` 预检）、`SafeTrayIconCatalogCache`（MCP ≤64 项／GUI 无截断全量）、`TabCustomImage_AvailableSizes`、同前缀目录防误判与统一 `loadTabCustomImageForDisplay()`；Qt6 Release `71E69EE6`、Qt6 全量 `90` / Qt5 定向 `5`、`run3` 3 阶段与双 Qt INI（252 不变）通过 | OS 文件级 symlink 实测、开启 FakerInput、实际渲染、Qt5 全量/整体、Win10 |
| `UI-TAB-IMAGE-266` | `deeb6a74` / `34E292CF` | `tab.set_custom_image`（142/134）、`get_state.tab_custom_image`、`TabCustomImageSettings`、`QTableSetupDialog` 防误脏、MSVC `noinline` 防栈溢出；Qt6/Qt5 单测与 `run1` 双 Qt INI（256 不变）通过 | 原生外部任意路径图、真实托盘/悬浮窗绘制、开启 FakerInput、Win10 |
| `UI-CROSSHAIR-265` | `9fa0ddac` / `61BB5E30` | `mapping.set_crosshair`（140/132）、14 字段 `MappingCrosshairSettings` 与 `QCrosshairSetupDialog` 防误脏；`run6` 含 `QItemSetupDialog`→准星探针、Common/`Global` 与双 Qt INI（247 不变）通过 | 实际叠加绘制、隐藏透明度渲染、开启 FakerInput、Win10 |
| `UI-FWINDOW-264` | `7c106d74` / `C3A78F96` | `tab.set_floating_window`（138/130）、`get_state.tab_floating_window`、8 字段 `TabFloatingWindowSettings`；`run3` 含 GUI 探针、Common 拒错、新建/删 Tab 隔离与双 Qt INI（254 不变）通过 | 实际悬浮窗绘制与鼠标穿透热键交互、开启 FakerInput、Win10 |
| `UI-TRAY-263` | `39d53898` / `6C035026` | `general.set_tray_icons`（136/128）、`get_state.tray_icons`（57 内置＋`custom_trayicons/`）、`TrayIconSelections`；`run2` 含有效/损坏 `.ico`、`modified=false` 与双 Qt INI（258 不变）通过 | 外部 `.ico` 导入、系统托盘真实渲染、开启 FakerInput、Win10 |
| `UI-NOTIFY-262` | `f4ebea21` / `2F517B41` | `general.set_notification_style`＋`configuration.set_custom_notification_style`（134/126）、`get_state.notification_styles`；`run1` 双套 14 字段双向隔离、冷启与双 Qt INI（246 不变）通过 | 悬浮通知视觉/动画渲染、开启 FakerInput、Win10 |
| `UI-PANEL-261` | `78f181c2` / `ACED5F9C` | `configuration.set_vbutton_panel_settings`（130/122）、`get_state.vbutton_panel`（21 字段）、`applyVButtonPanelSettings()`；`run2` 含 `context_busy`、`worker_default_show` 与双 Qt INI（241 不变）通过 | 面板真实拖拽/按钮触发、开启 FakerInput、Win10 |
| `ADV-VIRTUAL-INPUT-260` | `4d64169c` / `3500965D` | `configuration.set_accept_virtual_gamepad_input`（128/120）、`get_state.accept_virtual_gamepad_input`（保留 `AcceptVirtualGamdpadInputChecked`）；`run1` 与双 Qt INI（261 不变）通过 | Worker 5 类虚拟手柄事件过滤、开启 FakerInput、Win10 |
| `ADV-TOUCHPAD-259` | `eb579c4c` / `BF682275` | `configuration.set_gamepad_touchpad`（126/118）、`get_state.gamepad_touchpad`；`run1` 两位小数/`-0.0` 规范、Common/`Global` 与双 Qt INI（258 不变）通过 | 实体触摸板归一化轨迹、开启 FakerInput、Win10 |
| `SET-THRESHOLD-258` | `de448596` / `25816E98` | `configuration.set_gamepad_thresholds`（124/116）、`get_state.gamepad_thresholds`（12 阈值原子三元）、`CapabilitySchemaContext` `noinline`；`run2` 与双 Qt INI（250 不变）通过 | 实体手柄按下/轻按/释放滞后、开启 FakerInput、Win10 |
| `SET-GYRO-257` | `dd21d790` / `C70A1D3A` | `configuration.set_gyro2mouse_options`（122/114）、`get_state.gyro2mouse_options`、`setGyro2MouseOptions()`；`run2` 与双 Qt INI（258 不变）通过 | 实体手柄陀螺仪轨迹、开启 FakerInput、Win10 |
| `IGNORE-BUILTIN-DESC-FIX` | `e8c05c2f` / `87AAA15A` | `applyDefaultIgnoreWindowInfoDescriptions(..., preserveCustom)`＋三语言 `.qm` 默认集合缓存，保留内置规则自定义与清空说明；`run7_builtin_desc` 三语言冷启与双 Qt INI（261 不变）通过 | 其余 5 条内置规则同循环逐一抽查、开启 FakerInput、Win10 |

### 历史切片压缩索引（FORZA-MERGE ～ SET-SCALE-256，全文固定锚点 `8cbf5eb3`）

> 本节压缩 `SET-SCALE-256` 往前至 `FORZA-MERGE-20261031` 的历史已选验段落；完整逐行细节固定保存在 Git `8cbf5eb3`，可通过 `git show 8cbf5eb3:handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md` 取回全文。表中各行除 `FORZA-MERGE-20261031` 在 `PC-W10` 执行外，其余切片均在 `PC-W11` 执行；所涉 `out/` 路径均为各自执行机本机 `.gitignore` 忽略产物（不随 Git 同步），已有随仓脚本的用例可在本机复跑重取，仅存原机的夹具或日志按[目录规则](../README.md#L31)向原机索取；**从 `238` 起的隔离运行副本均因启动前自动审批阶段提示风险拦截而禁用了 FakerInput 分配与连接**，开启连接运行持续列为待验项。

| 切片范围 | 代码基线 / 关键修复 | 接入能力／核心变更与关键验证结论（PC-W11 选验） | 剩余未闭环与依赖约束 |
|---|---|---|---|
| `SET-SCALE-256` | `e4fb73d6` | `general.set_display_scale`（120/112）、`get_state.display_scale`、`setDisplayScaleEnum(int)`；Qt6 Release `4811ED24`、Rust/双 Qt 组件/私有产品/双 Qt INI `display_scale=9`（261 不变）通过 | 开启 FakerInput、Win10 Release、跨 DPI 多屏拖拽 |
| `WIN-RESTORE-PATH-255` | `65c33ac0` | `configuration.restore_process_path`（118/110）、`get_state.restore_process_path`、共用 `restoreProcessPathToProcessLineEdit()` 修复 `markSaveSettingDirty()`；`9B606FB3`、双 Qt INI（261 不变）通过 | 开启 FakerInput、真进程窗口匹配、>1024 超长路径展示、Win10 |
| `IGNORE-DEFAULTS-254` | `20695a69` | `ignore_rules.restore_defaults`（116/108）、`confirm_scope=true`、`buildDefaultIgnoreWindowInfoList()` 三语言即时描述；`6390162F`、双 Qt 9 条/三语言实窗/INI（261 不变）通过 | 开启 FakerInput、空表增删后语言热切换实测、Win10 |
| `IGNORE-DELETE-253` | `922887e7` | `ignore_rules.delete`（114/106）、`confirm_delete=true`、修正 `loadIgnoreRulesFromINI()` 按 `userType()` 区分空/单 Map/损坏标量；`B854C19D`、双 Qt 0/1/7 条冷读与损坏回退通过 | 开启 FakerInput、GUI 可见选区同步、运行中/多实例删除、Win10 |
| `IGNORE-UPSERT-252` | `dc0f4609` | `ignore_rules` 目录查询与 `ignore_rules.upsert`（112/104）、`QKeyMapper.pro` `/bigobj` 解除 MSVC `C1128`、1024 UTF-16 有界截断；`8B957AD4`、匹配探针/超长/双 Qt INI 通过 | 开启 FakerInput、编辑器草稿/打开列表删除恢复已补 `253/254`；系统/多开真实枚举过滤、Win10 |
| `WIN-SAME-WINDOWS-251` | `05360895` | `configuration.set_send_to_same_windows`（110/102）、`effective_enabled` 与全局/自动匹配锁定状态；`99573006`、双 Qt INI（260 不变）全绿零重试通过 | 开启 FakerInput、运行态同标题多窗口广播发送与释放、Win10 |
| `WIN-AUTOSTART-250` | `5db38020` | `configuration.set_auto_match_foreground`（标准 108/演示禁）、修 `loadKeyMapSetting()`/`loadEmptyMapSetting()` 跨组切换滞后启用 `sendToSameTitleWindowsCheckBox`；`CE824DEF`、双 Qt 普通/全局/半选通过 | 开启 FakerInput、运行期多前台窗口优先级/自动回退命中、Win10 |
| `TRAY-MCP-FIX` | `02626cc8` | 补 `SYSTRAY_MENU_ITEM_PRESSED_MCP=4`、`initSysTrayIcon()` 分隔线、`showMcpControlDialog()` 恢复最小化；`FBE93090`、私有点击/隐藏恢复/截图通过 | 开启 FakerInput、Default 桌面真实托盘右键弹出、Qt5 产品/Win10 |
| `REVIEW-FIX-241-249` / `RESET` / `ROOT-THRESHOLDS` | `1a2e1c58` / `9aec8be5` / `781d0965` | Codex 复核 `241/249` 三轮收口：空白失败恢复干净态、1024 截断元数据、补全 `loadEmptyMapSetting()` 重置与 `brakeThresholdDoubleSpinBox` 笔误、根区 `[General]` 重载与 12 项阈值原子加载；`DF6302B2`/`5C386AB7`/`79D85D7C` 全通 | 开启 FakerInput、阈值/触板/刹车前端仅在后续 `258/259` 补验；Qt5 产品/Win10 |
| `WIN-CRITERIA-249` | `57238d01` | `configuration.set_window_match_criteria`（106/100）、6 窗口条件＋`RegexMatch` 校验；`649498A0`、双 Qt INI（256 不变）与全局锁定通过 | 开启 FakerInput、6 模式实窗正则/大小写运行时匹配、Default 切换、Win10 |
| `ADV-NOTIFY-248` | `205afd0e` | `configuration.set_custom_notification`（104/98）、预览预测全局样式初始化（标志 `QMappingAdvancedDialog` 7/7 控件完成）；`EB81F143`、截图/双 Qt INI 通过 | 开启 FakerInput、通知实框定位渲染、仅 `Enabled=true` 无样式键惰性分支、Win10 |
| `ADV-POINT-KEYS-247` | `3cae0d65` | `configuration.set_point_display_keys`（102/96）、规范 `"None"` getter 与严格键名校验；`87EF0BDF`、双 Qt INI 通过 | 开启 FakerInput、三语言 `"None"` 展示与实际热键坐标叠加层渲染、Win10 |
| `ADV-PROCESS-TRAY-246` | `88393171` | `configuration.set_process_icon_as_tray_icon`（100/94）、全局配置禁用守卫；`49FD0DA4`、双 Qt INI（86 基线不变）通过 | 开启 FakerInput、运行期 Default 托盘图标优先级渲染、Win10 |
| `ADV-MOUSE-245` | `f9e0aa0b` | `configuration.set_key2mouse_settings`（98/92）、X/Y/Polling `1..99`；`19BC2741`、GUI/MCP/冷读/双 Qt INI 通过 | 开启 FakerInput、键鼠映射真实位移与定时间隔、Win10 |
| `GEN-FULLSCREEN-244` | `fa5536cd` | `general.set_fullscreen_global_mapping`（96/90，标志 `QGeneralAdvancedDialog` 11/11 控件完成）；`BE942A43`、联动/白名单/双 Qt INI 通过 | 开启 FakerInput、实际全屏前台拦截放行、Win10 |
| `GEN-GLOBAL-SWITCH-243` | `f4ce85fc` | `general.set_global_switch_timeout`（94/88）、同步 `m_CheckGlobalSettingSwitchTimer` 间隔；`2FFAD5FD`、0..999999999/双 Qt INI 通过 | 开启 FakerInput、待切换计时触发切换、Win10 |
| `GEN-SOUND-242` | `3281446d` | `general.set_sound_effect`（92/86）、沿用 `toggled -> dirty`；`F0EEA999`、双 Qt INI（85 不变）通过 | 开启 FakerInput、实际 WAV 音频播放输出、Win10 |
| `CFG-LOAD-GLOBAL-241` / `CFG-EMPTY-240` / `CFG-REMOVE-STARTUP-239` | `16796c96` / `e4e95fe5` / `d0342f70` | `241` 扩展普通/全局/空白互切与坏组回退；`240` 新增 `configuration.load_empty`（90/84）与 `selectEmptySettingGroup()`；`239` 补测删除启动组及清根 `SettingSelect`；`99059D0B`/`62F02619`/`8e325d23`、双 Qt INI 通过 | 开启 FakerInput、监视/映射中切换、当前组删除与右键入口、Qt5 产品/Win10 |
| `CFG-ORDER-GUI-232` / `CFG-ORDER-GUI-234` / `CFG-ORDER-235` / `CFG-REMOVE-GUI-236` / `CFG-REMOVE-237` / `CFG-LOAD-238` | `e8a38d90`..`b4ed0c89` | `232/234/236` 修复排序/复位/删组写盘占用误报成功及同名组/草稿损坏；`235` `configuration.move/reset_order`；`237` `configuration.remove`；`238` `configuration.load` 并确立 FakerInput 禁用隔离夹具基线 | 开启 FakerInput、当前组删除、GUI 右键菜单、物理拖拽、监视/映射中切换、Qt5 产品/Win10 |
| `CACHE-REGISTRY-233` | `81449ea9` / `1ff96fa1` | 受管测试登记容量 128／摘要 32、按实例显式标记及 `release-ref <target_id>` 淘汰后注销；普通 Medium 技能组件 104/108 项＋`quick_validate.py` 通过 | 仅控制器替身，未以新接口直驱运行态缓存或 UAC |
| `CONFIG-IMPORT-228～231` / `CONFIG-EXPORT-222～223` / `CONFIG-IMPORT-FORMAT-226` | `ced74823`..`a8e2f1a2` | `222/223` 普通组导出与占盘恢复；`226/228/229/230` 128 KiB/100 组解码、同名合并草稿、`configuration.import`/`save_import`（74+配置字段）；`231` 验证 `CHECKING -> waiting_cleanup` 门禁 | 监视中、全局/`General`/当前已加载组覆盖、带外部路径/自动匹配/FilterKeys 配置导入、Win10 |
| `GEN-WINDOW-SIZE-221` / `GEN-SAVE-SPLITTER-225` / `GEN-STARTUP-POSITION-227` | `6559db5e` / `19c6b28f` / `3c51e399` | `general.set_save_window_size`、`general.set_save_splitter_position`、`general.set_startup_position`（±50000 成对坐标）；真实 GUI＋Rust＋双 Qt INI 冷启通过 | 物理缩放/拖动/移动、隐藏进程表/混合 DPI、Win10 |
| `TABLE-EXPORT-215～216` / `TABLE-IMPORT-218～220` | `16744d19`..`3c5ee116` | `215/216` 74 字段单表导出及占盘回退（`qkm_export_mapping_table`）；`218/219/220` 128 KiB/100 行解码、共用行重建与 `table.import`（含共通冲突确认）；补齐 218 常量命名空间 | 活动态输入输出、产品超限/宏引用专案、Qt5 真产品/Win10 |
| `GEN-INSERT-213` / `GEN-EDIT-214` / `WIN-DISPLAY-217` | `fa59e86a` / `aea370ed` / `bad1f0f8` | `general.set_table_insert_mode`、`general.set_table_edit_trigger`、`configuration.set_window_display_mode`（0/1/2） | 物理点击、宏/序列窗内联编辑、真实前台 HWND 分类、Win10 |
| `MACRO-202～208` / `ORDERED-MAP-209` / `MACRO-210～212` | `bfed0071`..`86104662` | `macro` 只读目录、`macro.upsert`（追加/替换/插位/逐段备注）、`macro.delete`、五字段编解码与 `qkm_export_macros`/`macro.import`、Schema 补齐（212）；`209` 修复 `OrderedMap::operator=(OrderedMap&&)` 迭代器失效（ASan 复现并验证） | 超越字母数字序列的完整 DSL、宏实际运行执行、监视中导入与 Qt5 私有 GUI/Win10 |
| `DUPLICATE-197～201` / `TRIGGER-195～196` / `ROW-182～194` | `240625a3`..`48ce9159` | 单行/批量/跨普通表/共通源与目标复制（197～201）；行与批量触发时长（195～196）；备注/分类/显示/连发/锁定/时机/透传顺序/按住重复/固定键码（182～194） | 实际键鼠发送、触发与粘贴/悬浮开关运行效果 |
| `TAB-168～181` / `ROW-ORDER-162～167` | `9c87f5b9`..`c32f4eee` | Tab 重命名/增删/复制/清空/重排/共通继承/折叠/选择/热键/通知三态与双色（168～181，含 172 复制刷新热键索引修复、177 共通总开关冲突与关态修复）；单行与连续 2–100 行置顶置底与相邻移动（162～167） | 活动态切页/通知展示、实体快捷键触发、物理拖拽与拖拽边界 |
| `WAIT-156` / `STOP-157` / `START-158` / `ACTIVE-STOP-159` / `AGY-160` / `RUN-WRITE-161` | `5b293017`..`7d1bfeb6` | 有界只读等待、`stop`/`start`、活动源显式停止（`09c3b60`）、停止排空后恢复写入（`7d1bfeb6`）；`AGY-160`（`f9ffccde`）验证 Antigravity 中文路径调用取得主窗 PNG 与 ACP 内联识读 | `159/161` 普通产品活动态及真实键鼠物理输出、`160` 整包与同回合图像消费 |
| `EDIT-CAP-153` / `EDIT-COMMON-154` / `EDIT-COMMON-155` | `349d889c` / `8f902531` / `c9283438` | `original_key_conflict_policy=disable_current_normal_row_only`（153）；Common 原键更新连带禁用普通表行（[154 事实记录](mapping_edit_validation.md#edit-common-154common-原键更新的连带禁用)）；Common 同表原键冲突（[155 事实记录](mapping_edit_validation.md#edit-common-155common-同表原键冲突)） | 禁用行联动（disabled row cascading）、完整冲突与运行态生效 |
| `RUNTIME-BUG01-149` / `BUG02-150` / `BUG02-FOLLOWUP-151` | `ec2daf69` / `fe2eb674` / `3fefc26a` | 实键双列表与两叶锁（149）、虚拟列表与待释放 DOWN（150）、序列身份/FakerInput 释放隔离/UP 记账（151）；双 Qt/ASan/维护目标 Default G HUB & Razer 通过，详见[runtime_bugfix_validation.md](runtime_bugfix_validation.md) 与 [151 边界](runtime_bugfix_validation.md#bug02-followup-151三项运行时边界补充2026-11-17选定完成) | 同 VK、松开受阻、真实物理输入与普通 Release 全设备验收 |
| `EDIT-143～148`（不含 `141/142` 单独锚点段） | `c35aa57d`..`5a80dd06` | `143` 简单键值、`144` 禁用 GUI 基线（[144 证据](mapping_edit_validation.md#edit-gui-144禁用开关原gui基线)）、`145` `disabled` 字段（[145 证据](mapping_edit_validation.md#edit-145独立禁用字段正式接线)）、`146` 原键互斥 GUI（[146 证据](mapping_edit_validation.md#edit-gui-146原键互斥与common确认gui基线)）、`147` 共用冲突分析（[147 证据](mapping_edit_validation.md#edit-147共用纯冲突分析)）、`148` 普通冲突禁本行（[148 证据](mapping_edit_validation.md#edit-148普通原键冲突仅禁用本行)）；详表另见 [mapping_edit_validation.md](mapping_edit_validation.md) | 复杂原键扩展、真实输入输出、Default/Win10 |
| `DOC-140` / `CACHE-139A` / `IB-MCP-138～139` | `987e840f` / `395e684d` / `e73f719b` | `140` 文档压缩；`139A` `write_json` winerror=5 有限退避替换（99 项单测通过）；`138` IbInput 合流 `MCP 94f0bb84`＋固定输入 `b5b484a6`；`139` `window_role=ibinput_settings` 观察/捕获/关闭；详见[选定证据页](ibinput_mcp_integration_validation.md) | 主动连接检查／试发、普通 PrintWindow High 读、长期并发锁竞争、Default/Win10/ASan |
| `FORZA-MERGE-20261031` | `97963bf4` / `0d053c17` | `PC-W10` 选验：将 master 仅含维护测试的 `0d053c17` 并入 MCP 分支（保持停止准入/排空），补建 [现有Forza维护测试](../../QKeyMapper/tests/forza_auto_state/README.md) 实际 Worker 入口 147 项 ASan 无报错；FH4/FH6 沿 `85422ea9` 实测（该轮原件存 `PC-W10`） | 合并版未重跑实车、Qt5、性能、完整停止协调与驱动 |

#### 历史区间跨切片持续有效约束与未闭环汇总

- **FakerInput 隔离测试口径（`238～266` 共同约束）**：因 `238` 在启动前自动审批阶段提示“关闭含原驱动连接的新版副本可能触发键鼠中性报告”并予以拦截（属启动前风险拦截，非实机运行证实），随后 `238～266` 所有隔离回归运行副本均在夹具 Worker 禁用 FakerInput 分配与连接；**开启 FakerInput 连接的真实运行持续作为待验门槛**，不以禁用驱动副本替代完整硬件/驱动验收。
- **局部能力待扩展或待补验子项**：`configuration.remove` 仅支持非当前组（当前组与 GUI 右键待补）；`configuration.import` 首轮排除含外部路径、自动匹配前台、FilterKeys、当前已加载组覆盖、全局/General 组；`macro.upsert`/`import` 首轮限字母/数字序列且宏真实运行时执行仍待；`EDIT-COMMON-154/155` 禁用行连带、完整冲突与独立开关组合待补；`AGY-160` 整包与同回合图像读取待完成；`IB-MCP-138/139` 主动连接检查与试发待接；`RUN-WRITE-161` 与 `ACTIVE-STOP-159` 普通产品活动态及真实键鼠物理输出、`BUG02-FOLLOWUP-151` 同 VK/受阻 UP 待验。

### RUST-D4：Sidecar 迁移结项

Rust 是唯一受支持的 Sidecar（`349d889c`），采用 x64 单运行文件 `qkeymapper_mcp.exe`，组件附说明、清单与合并许可（Windows 7 x86 不支持 MCP；原 Python 独立组件按顶部策略归档，双 Qt 与 Codex 选验见[迁移归档](qkeymapper_mcp_rust_migration_plan.md)）。基于 `3a442ab1` 的 Release 布局以 `QKM_BUILD_MCP_BRIDGE=0` 关闭联动、共用 Rust 1.100.0 静态 CRT 产物与 `licenses/QKeyMapper-MCP-Licenses.txt`（[发布用法](../../scripts/release/Release使用说明.md)）；PC-W11 已完成 7 组 qmake 与 67 crate / 123 原件许可组装，实际联动编译、PE／烟测、Win10 三包与 `AGY-160` 消费留归 M3-B。

### EDIT-GUI-141：普通单行键值修改原GUI基线（选定完成）

`1d0281f3`（2026-11-10，PC-W11，产品 `0ce665c0` 未改；完整记录保留 Git `8cbf5eb3`／`5a80dd06`，本机原件 `out/mcp_edit_gui_20261110` 不随 Git）：双 Qt 八个 High 私有进程各 41 项／共 82 项经真实 Item 双击、字符/Backspace、Update/Escape 核对普通键值与抬起清空（b02 修正合成夹具为 `KEYDOWN_AND_KEYUP` 后全通），无 Default、普通 Release、正式 MCP 或完整 M1 证明。

### EDIT-142：键值候选的最小共用数据提取（选定完成）

`a4270951`（2026-11-10，PC-W11；完整记录保留 Git `8cbf5eb3`／`5a80dd06`，本机原件 `out/mcp_edit_values_20261110` 不随 Git）：保留原 setter 边界并提取共用 `updateMappingKeyValues`（保持分段/纯键去重与赋值顺序，不改原键样本、备注或 dirty），双 Qt 八个 High 私有进程各 45 项／共 90 项对比 141 一致（Qt6 零警告、Qt5 仅已知 `qtmain` LNK4099 例外），后续正式接线见 `143～148`，无正式 MCP、Default 或输入设备结论。


### UI-73：MCP 用户界面翻译与按钮行为修正

UI-73在6d33db27封存代码／正式TS/QM，10-26后按顶部继续C1-B2-PROFILE，不重复Tab／映射提取。[31项三语建议及决策](ui_translation_recommendations.md)、[失败／实际验收](ui_localization_validation.md)保留详细原范围。ControlDialog统一翻译／语言刷新及原按钮焦点、关闭不禁用接口行为；AGENTS／workflow及Claude镜像同步，技能基础校验通过。

TS新增31有效项，旧译文语义不变；QM 1144／1147／1147完成并一致。Qt6零警告，双Qt控制各36项；实际类／资源三语6次66项通过。实际私有桌面、无输入／UAC，asInvoker UI副本不替代High／驱动／Default。中文区域可读，旧日文进程说明固定宽度裁剪单列范围外，不称全页通过。当时12工具／8能力，M1未完成；后续以当前卡片为准。

## 1. 目标、架构与固定原则

建立同时服务**用户自然语言操作**和**开发调查、问题复现、修复验证**的统一自动化接口。最终覆盖现有全部用户可配置项和可执行操作，开发测试从首个闭环就参与验收。

**Agent／回归脚本 → Rust Sidecar → 本机 IPC → 共用配置、GUI 与映射业务。**

- C++ 端负责业务规则、权限、状态限制和实际操作；小型本机通信组件接入 Qt 事件循环，优先复用可满足身份及远程拒绝要求的 Qt 能力，不在输入热路径增加常驻轮询。Rust Sidecar 负责正式协议适配，x64 用户无需 Python 或 Rust。
- 公共接口覆盖能力发现、查询、帮助、变更预览与提交、明确启停、操作完成查询、条件等待及按需诊断观测；真实输入、设备和驱动操作使用专用入口。
- 首次未选择时MCP默认关闭；用户明确保存的启动选择按PERSIST-136一次恢复。本地配对／实际用户与登录身份／权限仍强制，不把管道名称或关联文件当授权，不开放任意Qt属性、INI键或通用执行。
- GUI 与 MCP 对每项操作采用相同的状态限制。设定调整、映射内容的添加／修改／删除等现有 GUI 要求空闲才能执行的操作，必须在 IDLE 且相关停止清理完成后才能执行。
- 映射运行中需要执行上述操作时，Agent 先说明将停止映射以及具体修改内容，由用户选择是否停止后修改。用户同意后，等待停止及相关任务清理完成，再重新校验并提交修改；用户未同意、停止失败或超时，均不得继续修改。修改后按原计划保持停止。
- 窗口大小、窗口位置等现有 GUI 在映射运行期间仍允许的纯显示操作，MCP 同样允许，无需为此停止映射。其他显示操作应逐项遵循 GUI 的实际限制，保持现有 GUI 行为。
- 修改后默认保持停止。保存按用户意图及选项既有持久化语义执行，完成报告区分“已修改、已保存、已生效”，并提示输入 **开始映射**。
- 高风险操作提前解释影响、当前实际停止快捷键及适用恢复办法。普通键鼠映射不为补齐依赖而自动安装驱动；不得承诺快捷键、重启、安全模式或卸载一定能修复驱动失效。
- 权限由 QKeyMapper 执行，不只依赖 Skill、工具注解或 Agent 传入的确认布尔值。已授权测试场景内可连续执行，不逐步重复询问；测试用途不绕过业务限制。
- MCP 接入面向 Win10／11，不将 Win7 纳入验收；共享源码保持 Qt 6.8.3／Qt 5.15.2 兼容。中文优先，并维护受影响的英文、日文行为。
- 自然语言理解由外部 Agent 完成；QKeyMapper 内不引入模型或聊天系统。Skill 只作可选操作指南，接口自身提供足够的说明和风险信息。
- M0.1 的计划迁移已完成；用户随后授权开始执行计划。按阶段推进，先完成当前静态盘点与首个闭环契约，不能把已批准的整个目标误记成产品能力已经实现。

### 评审吸收与实施门槛（REVIEW-59）

参考[评审附件](review_and_improvement_recommendations.md)，其原文仅作基线证据，不在附件维护整改进度。2026-10-24 核实 HEAD 仍为 c32e004b，附件的 R1／R2 探针与 15 项原测试按同版本／原范围复用，没有为核实而重跑；R3／R4 的源码事实成立，不等于已测得性能退化。

| 评审项 | 当前结论与采纳决定 | 验收／接续门槛 |
|---|---|---|
| R1／BRIDGE-59 | 已修复并完成选定回归：原生 Future 统一完成归属，以当前 Future 身份限定取消 | 旧实现五项新增测试四失败，修复后完整 20 项、夹具 3 项及 Qt6 SDK 12 场景通过；实际原生读取取消／断开仍小于既有 3 秒门槛，不提前放行 I/O |
| R2／BRIDGE-60 | 已修复并完成选定验收：固定 SDK 原生 CallToolResult，保留 ok／error、受控解释及按码下一步，拒绝连接申请同样标错 | 协议 20／工具结果 6、双 Qt SDK 各 12、Qt6 产品 11／源 9 通过；权限拒绝及失败 operation 数据由正式 stdio 固定端点验证，不冒充真实产品该路径。SDK schema 错误保留原标准通道 |
| R3／ENGINE-61 | Win10／Qt6 的已选键盘原功能及普通 Release 四态证据已建立，EXIT-63 修正公共退出顺序；其他系统／设备／高频／唤醒缺口保留 | 后续只读许可可在已证边界推进，不新增输入热路径；源写入、相关设备／系统接入及发布仍受剩余验收约束，不据计数或 NoOp 宣称完整无退化 |
| R4／QUERY-62 | A 纯参数前置和 B 一次完整编码复用已通过选定验收；B 助手合成基准已测，产品全量采集／锁内逐行编码仍存在 | 完整快照、原修订字节及内容／身份／权限绑定不变，只读门禁沿 STOP-57。暂缓更大缓存／拆分，先打通业务闭环；全链路成本与后续分离仍按实际需要测量 |

优先顺序为 **R1／R2 → 公共引擎原功能与性能验收 → 安全读取及首个真实业务闭环 → 完整能力扩展**。STOP-57 的清理代次／活动修订只读许可设计保留，须先经过 ENGINE-61 门槛；不以 MCP 开关维持两套停止语义，不进行全局配置模型或映射引擎重写。必要公共安全小改动独立审阅、验证和提交，已有相关提交作为起点，不重写历史。桥接切片预计 3–6 小时有效作业时间，纳入 M1 分账，不重复计算原已包含验证；ENGINE-61 细化场景后再校准剩余预算。

ENGINE-61 固定主分支 d49a9f57、当前分支 MCP 关闭、启用但空闲、持续查询四种状态，使用同机同普通 Release 工具链、配置和输入场景，重复记录输入延迟分布、CPU、内存、线程／唤醒及 GUI 响应，区分噪声与稳定退化。覆盖普通按下／释放、连发／锁定、序列等待、停止发送、切表／延后回切、快速启停及退出；设备与 Qt5 按实际受影响路径验收。持续查询仅使用运行态允许的状态／目录能力，初始静止源查询压力单列，不放宽 pristine_idle。接口关闭、零计数和 NoOp 不能代替实际输入或性能结论；ASan 与普通 Release 性能分别报告。

首个闭环按 **能力发现 → 查询明确目标 → 预览 A→B → 提交 → 显式保存 → 冷启动读回 → 按用户意图开始 → 实际效果验证 → 停止** 验收，同时具备取消、冲突、过期预览、保存失败、去重与断线结果查询。区分编辑配置／修订、已保存配置／结果、实际运行配置及带采样时间的运行状态；运行采样不无故使配置预览失效，保存前说明作用范围，避免连带保存其他草稿。按键名称、参数范围／单位、默认值、依赖、示例、允许操作和可解释错误随首个能力提供，复用现有解析与守卫；基础使用不依赖特定 Agent／Skill。完整目标不缩减；先用已核验显式目标完成开发闭环，多实例及黄色框仍是独立能力，首个闭环后尽早试用免 Python 包，完整交付保留 M3-B。

QUERY-62 暂缓快照缓存及目录／详情协议拆分，是因为当前源引用绑定完整内容和实例／授权上下文；静默改用元数据版本会削弱逐页复核。后续独立设计有界快照、采样时间、失效、执行预算与限流；不得携带可变源指针越过安全复制边界，历史快照不冒充实时状态，缓存引用不授编辑权限。

QUERY-62-A 提前拒绝纯参数错误，双 Qt 源维护和 Qt6 私有正式 SDK 10 场景通过。共用校验仍保护直接调用，源读取边界和修订不变。[证据](validation.md#query-20261024-62a采集前的共用纯参数校验)。

QUERY-62-B 共用一次完整绑定编码，双 Qt 核对 4 MiB 边界和原修订。合成助手基准及正式 Qt6 私有十场景见[证据](validation.md#query-20261024-62b一次完整编码与原修订预算保持)。全量源复制、锁与每请求哈希仍在，R4 未完成。

EXIT-63 在退出时关闭准入，经过 Hook 线程边界并完成 Worker 取消／抬起后再结束两线程。普通 Qt6 六次仅一次 C 抬起、无 E、正常退出。四态与夹持性能见[事实](validation.md#exit-20261024-63保留-hook-到退出发送排空与性能复查)，持续约21 Hz 查询增加 CPU 与约4 MiB 工作集。其他设备、时序、系统及构建配置保留原验收范围。

STOP-57 仅恢复安全只读：既有停止流程完成前后所有者／池／GUI 尾部及必要配置回切后，在 SourceAccess 互斥内重核在途任务并记录当前活动修订和根清理代次；状态或代次已变化不提交该许可。源复制须加载成功、IDLE／STOPPED，且初始静止或当前修订匹配完整清理。运行活动使旧许可无效，饱和拒绝。源快照增加 cleanup_generation（初始为 null）；因此相同内容跨启停也拒绝旧引用。state 的 observed 只描述当前只读清理上下文，不是完整异步 operation、业务修订或编辑许可。正式 SDK 与实际 GUI／输入结果见 STOP-20261024-57；完整写入安全缺口保留。

### CACHE-SKILL-69：工作区专用缓存技能及本机补验

2026-10-25批准工作区普通 PID 缓存，默认及 restore 5分钟、上限24小时。6bfacd88基础26项、Win10最终51项，a5097898修正源组夹具。原 UAC 关闭 source_cleanup 后实际退出0。短驻留和模拟按原范围，细节见[技能证据](gsudo_cache_pending_validation.md)及本页 Git 历史。

### CACHE-LOG-84：固定动作失败诊断补充（2026-10-26，PC-W11）

SAVE-83 后补充原生失败与清理字段，未知保留。普通模拟19项通过，见 `out/cache_diagnostics_w11_20261026` 与 Git 原文。

### CACHE-SKILL-70：日常摘要与自然覆盖复查

普通67项及无缓存 Medium 五事件通过，周期、到期和故障主要为模拟。见[证据](gsudo_cache_pending_validation.md)和[日志用法](../../.agents/skills/qkeymapper-gsudo-cache/references/usage.md)。

### MAP-67-B0 检查点与接续方式

B0 固定预览／结果容量、归属、期限及查询语义，unknown 不证明未执行。B0 草稿已于 `4c5d04f3` 吸收进[B1](map67b1_validation.md)，不得再次应用。双 Qt 见 MAP-20261024-67-B0，旧暂停见 Git 原文。

### MAP-67-B1 的 Default 补验范围

2026-10-25当次批准自有engine61隔离产品／普通接收窗Default集中10–15分钟，Read+Configure配对、GUI启停，未授RunMapping。真实GUI删A→B后正式MCP恢复同配置／派生值且不落盘；运行时GUI禁用、MCP拒绝／modified=false。仅核实前台／PID／输入桌面的接收窗发送A／F24，检查B按下抬起及停止A恢复，失焦／Esc停止，仅清理自有保持。普通截图用仓库脚本，不全局置顶、不提权截图或改变显示配置；DP显示器物理电源不由API推断。四态三轮250对比较须同INI／工具链，CPU／尾部及查询成本不作零开销证明；实际唤醒／跨屏／设备／Win11当次未验。29236及21:07:50仅历史缓存，不能接管；结果见[B1证据](map67b1_validation.md)。

### MAP-67-B1：选定验收与保存接续

4c5d04f3上补全源互斥、运行修订／请求／权限复核及5秒写前预算，GUI／MCP共用prepare／commit；仅当前源大写字母／数字内存添加和dirty，不自动保存／开始。双Qt维护／SDK／warm／共通通过；Qt6 Default证实独立GUI／实际输入、运行中拒绝、同步组陈旧和零部分修改，以及独立悬浮编辑窗拒绝／取消。其他编辑窗／系统范围未扩大。四态正反三轮250对功能通过，p50约0.89–1.05ms，有CPU／尾部波动／查询成本；正序曾与分析重叠，反序无构建分析。EQ-68仍完整快照／哈希；R3设备／唤醒等保留，详[B1](map67b1_validation.md)。完整保存须覆盖全局、宏、说明、面板及辅助QSettings／其他草稿，不能只用源修订或dirty。

### MAP-67-C1：完整保存的分步准备

C1-A仅提取三辅助值函数，四包装器仍同路径／codec／原时机写入；双Qt保存14／冷读11见[C1证据](map67c1_validation.md)。C1-B1只读全局／显示／收藏历史保留OrderedMap与语言采样时机，双Qt默认及尺寸／分隔条开关六对252键／类型值／INI字节一致，生成不改状态。SAVE-71修复Common上下序列备注，双Qt各22项；旧格式读取不改盘。以上未开放MCP保存；后续TABLE／PROFILE及C1-C已接，不重复提取。

共用保存必须保留快捷键默认化／clearFocus、分类／运行同步、配置顺序、辅助QSettings共享缓存回滚、确认／重载／dirty／备份，以及完整目标／磁盘／依赖／其他草稿确认；写盘与生效分段，不能直接调用GUI Save冒充闭环。准备不得替换CONFIG_FILENAME／真实INI／工作目录来模拟。

### C1-B2-TABLE：当前独立切片完成与交接

75d5a5e6后只提取collectMappingTableSaveValues，返回OrderedMap并原位置／前缀／顺序写回；97唯一键、清洗、Tab恢复、前后规范化及失败／重载／dirty不变，逆变换通过。双Qt普通维护、私有原manifest旧GUI各52／新各60，四对252键类型值及INI字节一致，冷读不改盘；含非当前／空Tab、Unicode／备注／样式／同步组及Common附脱。无图时重载清空像素选择保留，首夹具／准备失败分列[C1证据](map67c1_validation.md)。此载荷不是完整保存修订／编辑许可，当时12工具／8能力；PROFILE后续已完成见下。旧6920三小时窗口及跨旧截止用例已正常off／零拥有未定，不接管旧PID，不以存活时间充开发实耗或系统验收。

### C1-B2-PROFILE：剩余保存依赖盘点（SAVE-74，2026-10-26／PC-W11）

以下为已完成历史保存准备与C1-C索引，源码起点6d33db27；所有Tab／全局提取、SAVE-71和B1草稿均已吸收，不再应用。详细独立GUI基线、字段、失败轮次／EXE／桌面及复用边界统一见[原C1证据](map67c1_validation.md)，压缩前原文可用Git 0ce665c081e49abfab311082ac157c0dc36a1f91的本文件查看，不是新进度来源。

持续约束：GUI可变准备不进入只读预览；显式目标不读名称控件，完整范围含全局／共享宏／顺序／Common备份／其他草稿，dirty不是归属证明。输入／图标／通知须绑定原条件及磁盘依赖。候选不构造／sync真实INI的QSettings，避免共享pending被构造触发冲盘。写盘失败只回退本次pending；diskSaved／reloaded／backup分别报告。状态／授权／完整内容／磁盘／上下文／到期及真实清理均执行前复核，已提取字段不再搬动。

| 切片 | 已选结果与必要边界（双Qt，除非另注） |
|---|---|
| SAVE-74 | 36纯参数原阶段写回；旧GUI28／新44，16进程0、四对262／298键及字节相同；冷读／采集不改状态。仅保存准备，未验输入设备 |
| SAVE-75 | 窗口匹配10条件字段／12写点，全局跳过、旧路径及图标空／存在规则保留；旧62／新87，40进程0、十对完整INI相同，逆变换至e7b73960 |
| SAVE-76 | Common74字段编码，原同步后总写，含上下备注及空表清旧备份；旧40／新55，24进程0、六对277／277／276键相同；当时尚未绑定完整保存 |
| SAVE-77 | 通知14条件样式，显式const目标QSettings与原短路；旧52／新72，32进程0、八对INI相同；不证明运行通知显示 |
| SAVE-78 | 9开关／当前VIGEM6标量／21面板键原顺序及round；旧32／新43，16进程0、四对相同。真实Qt背景拖动保存实际偏移、零按钮派发；缺面板仅组件、VIGEM关闭未跑；错误FilterKeys预期／旧scope拒绝保留 |
| SAVE-79 | 目标只读分析保留trim／全局／新建覆盖；旧85／新122，72进程0、18对INI相同。拒绝取消不改状态；换行名／消失组等只组件，缓存未知启动不计成功 |
| SAVE-80 | 只读Active／LastActive源副本、qMin／空原键／后项覆盖，Burst／Lock仅副本投影；旧33／新55，24进程0、六对277键相同。人工运行依赖不是实际映射 |
| SAVE-81 | 共用热键纯分析，GUI更新／clearFocus顺序不变；旧57／新86，56进程0、14对262键相同。焦点／弃用API／未知启动失败保留；edited／hasFocus／isModified不能推断有效保存文本 |
| SAVE-82 | 完整值组合及显式新配置顺序动作；目标非法或未提交热键返空载荷，不提交编辑。旧63／新96，40进程0、十对INI相同；所有变化键属足迹或顺序动作 |
| SAVE-83 | 复用编辑窗拒绝，完整Qt_5_15候选指纹含类型／顺序／目标／磁盘／源，限4MiB；真实高级窗旧红例后各21项通过，其他窗当时仅静态；合成源不授权限 |
| SAVE-85 | 固定路径读取≤4MiB完整磁盘候选，临时副本解析／分析／载荷一致、返回重读比较；各17项，覆盖pending隔离、坏格式／缺失／目录／超限及编辑窗。调用中并发与执行未由此证明 |
| SAVE-86 | 原bool只回reloaded，结构化阶段保留拒绝／取消／写盘／重载／备份；旧67／新85，56进程0、18对INI相同。写锁解锁后冷读无补写；已写但未重载和备份失败独立报告 |
| SAVE-87 | 实际Read+Configure请求、实例／pair／epoch／完整源及当前Tab／运行修订绑定，锁外I/O；各41项、六原manifest私有进程0，陈旧请求／派生源／编辑窗／新运行拒绝；含85回归，不是正式工具 |
| SAVE-88 | 足迹明确目标／共享键／启动选择／排序、write_footprint_not_diff及全范围确认；各62项／六进程0，清dirty仍确认；描述不是执行许可 |
| SAVE-89 | 原116行收尾逐字共用finalizeSettingSave，逆变换通过；复用86旧基线，各64项、22进程0、12对INI相同；锁后无补写，重载／备份分段；共通页恢复等未扩大 |
| SAVE-90 | Qt6阶段探针证实新构造同路径QSettings可冲刷pending，改复用GUI只读观察器／原生投影；各候选126＋保存回归64，共380项／30进程0、12对INI相同；四份早期失败保留，不证明执行期无竞态 |
| SAVE-91 | GUI名称文本进入context，独立报告重载会改变名称，仍非目标来源；旧Qt6两红例后各137项／八进程0；不改GUI Save或观察器 |
| SAVE-92 | 明确候选执行保留热键／Common／顺序及原收尾；8327d7bd旧GUI冷读22进程116项、新执行22进程164项。12组全键／编码值相等，新目标宏／说明排序有六组字节不等；不是外层授权／并发许可 |
| SAVE-93 | 120秒单调截止，重采当前授权／候选／context／足迹／独立确认，不续期或消费旧载荷；各201项、八进程0。两编译轮及四失败日志保留，不当产品运行故障 |
| SAVE-94 | 有界MappingChangeStore保存元数据，单pair替换／取消／消费，32／64限额；applying不淘汰、无槽不消费、完成再计保留期。各8项含init／cleanup；clear不是实际任务取消 |
| SAVE-95 | 正式configuration.save.preview接显式action／setting_name、完整范围／绑定，时点apply_supported=false；Python28、双Qt七槽／六stdio维护进程42项不变性通过。时点12工具9能力，夹具配对不冒充普通产品用户批准 |
| SAVE-96 | 重采元数据候选、磁盘／pending后复核权限／原截止／预算／清理／运行代次，源互斥持至写盘和重载；14真实认证进程398项，含旧表销毁跨线程tryLock。探针非真实停止竞态 |
| SAVE-97 | 正式save.apply独立范围／覆盖确认、一次消费及阶段结果；Python28、双Qt七槽／18stdio进程130CPP／443SDK、14保存／两冷读。八组与旧GUI键值相等，仅当前目标两组字节相等；写锁恢复／备份失败／旧引用覆盖，时点12工具10能力 |
| SAVE-98 | 仅测试，产品43a2a150；真实Save覆盖Yes／四冷读及正式reload_failed，十进程52项。全局两组266键字节相同，覆盖310键仍有已知序差；坏源已写／未重载／无备份，重复ID不重写；首助手失败／缺退出码保留 |
| SAVE-99 | 实际名称编辑／全局暖切使旧确认拒绝且不改新GUI状态；Qt6红例安全拒绝但error过泛，一行改stale_preview后四场44项通过（未受影响Qt6名称复用）。旧定义仅撤授权，不动历史未知 |
| SAVE-100 | 实际慢重载后桥接10秒timeout／disconnect不取消已开始保存；重连查原ID仅一次重载／回放，双Qt四进程36项＋工具结果7项。非客户端／产品重启或撤销／停止完整证明 |
| SAVE-101 | 真实GUI停止＋受控Hook生产者最多5秒，Worker STOPPED仍拒源读／保存且INI不变；排空及GUI尾部后新预览一次保存。双Qt普通／30ms观察超时四场44项0；旧代函数回放非设备迟到事件，首双预览脚本失败保留，无真实输入 |
| SAVE-102 | 101真实清理后正式X→Y新增仅内存dirty、保存及独立冷读A→B／X→Y；四进程42CPP0，INI／备份字节相等、旧实例引用失效。首SDK KeyError在保存后发生并保留exit2；非物理键或跨进程配对证明 |
| SAVE-103 | 真实GUI停止尾部Global→McpReturn后新确认只保存Q→R，各加载一次、双Qt24CPP／两进程0；旧Global引用／预览拒绝。安全拒绝分类一行对称化；旧动态表销毁但主表复用，不能以地址判断源身份。armReturn注入初始匹配记录非真实前台匹配 |

这些历史用例均保留各自原manifest、自有合成INI／实际私有桌面及公共提权登记边界；未授Default／真实系统输入／驱动变更。失败与缺失原码不改写为成功，注入／组件／正式MCP维护目标与普通Release分列。完整M1启停、并发／设备／跨系统等剩余项继续原计划；保存分段结果和选定冷读不等于全量GUI等价性。

### CACHE-SKILL-72：统一公共测试提权接入（用户插入）

已完成独立工具切片，详[公共入口证据](cache72_validation.md)。复用scripts/test_process.ps1和scripts/managed_test.py：只有绑定内容身份／参数／环境／桌面／输出／预算的Start、拥有关系Close及固定Gui；定义须独立授权，不能任意shell／解释器脚本／Qt属性。普通子脚本RPC实际由缓存普通控制器派发；作业令牌只分归属，实际Medium／同登录另核，不另开／嵌套缓存。run等待内部请求不自锁，新case可登记，活跃目标清理后才改定义或助手；PID复用／未知不重放。普通Git／构建／SDK／截图不提权，时限与模式不扩权。

当次普通87／助手维护18及结构检查通过，SDK12／新维护25复用；首次映像／Close选窗缺陷修正后26项／到期组通过。真实35秒组产品退出0、缓存到期不可用／零拥有未定；原截止21:07:50未重算，审批未完成及ExitCode观察失败保留。升级需空闲、先off再按原剩余时间，失效／原生独立UAC仍需用户；不声称源全组／输入性能／Qt5／Win11／长时通过。当前缓存以顶部卡片为准，旧入口不接管。

### EQ-68：同步组配置比较的独立修复

按用户要求在 MAP-67-B 中插入独立小修复，仅向 MAP_KEYDATA::operator== 增加 FloatingButton_SyncGroupId；三个 Pure 列表和 LockState 继续排除，不改引擎、序列化、分组或撤销逻辑。当前四处直接调用均为 Item 编辑变化检测；原缺口是配置比较遗漏，不等于已复现保存／恢复故障。

修复后 74 个比较字段与现有映射码字段集合一致，可供单条配置相等／局部变更检测共用。现有 MCP automationSourceRow 已包含同步组、三个派生列表及锁状态，完整快照／哈希保持；本次不为使用 operator== 增加重复判断，也不把本地 QColor／double 等相等与 JSON 规范化视为相同契约。完整修订仍覆盖所有依赖及目标身份，实例、配对／权限、到期、状态、目标代次和陈旧引用守卫由外层负责。

原选定验证为双Qt产品源码维护夹具：同步组单改、Qt列表比较、四项排除、原配置差异、编辑刷新及既有Apply／Revert。修复前Qt6两项预期红例、其他14项通过；修复后双Qt各16项通过，见EQ-20261024-68及独立提交014ef2eb。该轮未用Default或真实输入。B1另经Qt6 Default GUI仅改同步组，正式MCP旧预览返回stale_source、无部分修改并还原；复用完整源哈希，无新增operator==调用，其他外层守卫不变。完整范围见MAP-20261025-67-B1，不冒称Qt5／Win11该正式路径已验。

### ENGINE-61 执行切片与必要桌面范围

历史61-A、B1／B2及EXIT-63基础验收，C++9af1b260／桥接a0ee90be及Qt6主基线d49a9f57范围见[原验证索引](validation.md)。普通Release同INI／工具链：主基线、MCP关、配对空闲、持续状态／Tab查询四态各三轮250对；自有A→B接收、F24停止后A恢复。p50约0.91–0.94ms、单核CPU约2.46–3.31%，GUI WM_NULL有16.62ms样本；不承诺零成本／改进。B1十四组原功能对照＋十四工具项；B2回切／四轮快速启停六组及工具十五项通过。退出三次基线／当前均出现C多UP（无E、正常退出），非MCP特有；EXIT-63后选验另记，不抹旧失败。真实唤醒／设备／Win11／Qt5／其他场景仍待，ENGINE整体不完成。

当次Default批准仅engine61自有baseline／current原manifest副本及普通接收窗，15–20分钟，固定High GUI助手启停／只读配对，其余Medium。源键A、F13–F18／F24，产品B／C／D／E；不向其他窗输入、不测鼠标改键／安装驱动。每次核前台HWND／PID／修饰键，失焦／Esc即停，不自动抢回。Windows拒激活时曾另批仅阶段开始自有接收窗中心一次左点击并恢复光标，核命中且无保持；不使用AttachThreadInput。原启动／停止可能连接或ReleaseAll FakerInput，同驱动既有状态可能受影响，不操作用户实例。普通仓库截图、Qt自身UIA恢复；源查询压力只在pristine_idle，不放宽门禁。该历史授权不扩新范围；性能与实际输入／组件分别记录，REVIEW-59剩余发布门槛保留。

### 临时无人值守缓存例外（TEMP-CACHE-61，2026-10-24 当日）

已结束的当日特例：普通控制器6816仅固定engine／product／source，截止2026-10-24 19:30、19:15停新派发，模式／默认配置不改。用户19:23返回后关闭，closed核缓存不可用、PID不存在／native0；后续13152／3044各五分钟并关闭。job001–014为B1、015–023为B2；020／021业务断言失败但自有产品及接收器清理正常。原件仅本机out/mcp_product/engine61/unattended-20261024，不复活旧PID、不把普通开发提权。身份／去重／未知不重放等当前规则见缓存技能，历史例外不授新输入或永久期限。

### 公共接口职责

下表为完整接口职责与 v0.1 设计基线，不等同于全部已经实现；当前只开放顶部卡片与能力矩阵明确记录的能力，其余 schema 及业务仍须按 M1／后续阶段实现和验证。

| 接口组 | 职责 |
|---|---|
| `qkm_get_capabilities` | 软件／接口版本、实际支持能力、参数约束、依赖、权限、作用范围、生效及持久化时机 |
| `qkm_get_state`／`qkm_query` | 实例身份、运行状态、配置、Tab、映射、宏、设置、设备及当前实际停止快捷键 |
| `qkm_get_help` | 功能说明、相关风险及手册内容 |
| `qkm_preview_changes` | 校验并规范化变更，说明冲突、连带影响、风险及停止要求；不修改状态 |
| `qkm_apply_changes` | 提交与预览一致的配置变更，返回实际修改、未保存及持久化结果 |
| `qkm_start_mapping`／`qkm_stop_mapping` | 明确、可重复调用的目标状态操作，避免直接复用带切换含义的按钮命令 |
| `qkm_get_operation`／`qkm_wait_state` | 查询异步进度，等待真实条件成立，区分已接受、完成、失败、超时及结果未知 |
| 观测与专用操作 | 有界、按需的诊断事件及相关操作关联；界面会话、真实输入测试、设备和驱动采用明确参数及权限 |

写操作使用明确类型，覆盖新增、更新、删除、排序、复制和导入导出等既有能力；更新保留未指定字段。未知、拼错、不支持或越界参数拒绝执行。操作目标关联实例、配置和真实数据来源，不只使用显示行号。

预览绑定相关配置版本，GUI 或其他客户端改变条件后失效。GUI／MCP 复用业务状态校验；写操作串行提交，实际提交时再次检查状态、停止清理及目标，覆盖预览后开始运行、进入 STOPPING、切配置或修改目标。原预览已失效时重新生成；修改内容或影响超出用户原选择时重新征求选择。冲突返回结构化选择，避免请求停在不可见模态窗口中。请求具有去重及结果查询能力；重连或超时先核对结果，实例已重启而结果不明时不盲目重试。

纯配置变更先整体校验并保护已有草稿；因状态或预览失效拒绝时，不得部分修改映射、派生数据、dirty 或磁盘，批量请求也适用。不能原子完成的跨阶段操作明确报告部分结果，不承诺撤销系统副作用；已发生的停止动作与被拒绝的配置提交分别报告。停止超时后即使清理随后完成，也不得擅自执行原修改。保存复用现有序列化、写盘检查及失败处理，包含冷启动验收。需重启的设置如实记录生效时机，不能伪装成动态生效。

正常停止可能执行“停止映射时发送”动作，预览应包含相关影响。原停止入口不依赖 MCP 或 Agent 在线，也不被新增授权流程阻塞。等待不得阻塞 GUI 事件循环，超时不能成为删除活动同步资源或强杀用户实例的理由。

### 普通用户多实例目标选择、可视确认与会话绑定（TARGET-DESIGN-47）

以下47／58文字保留当时设计基线；现状以本节增补及顶部卡片为准：105已交付独立ShowWindows／CaptureWindows及GUI追加授权，107–121／139已交付选定私有窗口观察／捕获／开关，PKG-123／137-B已有免Python桥接组件选验。普通多实例候选／Default前台核实确认／会话主动切换和黄色提示仍待，不能把私有后台角色或组件ZIP当成这些能力或完整Release交付。

**状态：设计补充完成，新增能力全部待实现／待验证。** 多实例原设计对应提交 1c3dc6d746409d479d3952983bf4c94659094874，依据 STOP-46 检查点 309133b5 核对；DESIGN-58 在产品源码 9af1b260 上补充整窗黄色提示的静态可行性及待验要求，不表示新增工具已经可用。普通用户只有带 MCP 的 Release 解压包和配套桥接，没有源码、开发环境或 venv；正式交付不得依赖用户安装 Python。当前桥接仍为开发入口，免 Python 打包、离线启动及干净 Win10／Win11 验收仍待 M3-B。

#### 已实现的绑定与现有限制

| 现有机制／源码锚点 | 当前事实与边界 |
|---|---|
| [桥接入口](../../mcp_bridge/src/qkeymapper_mcp/__main__.py)与 [Bridge](../../mcp_bridge/src/qkeymapper_mcp/bridge.py) | 启动必须显式传 --discovery-file 和 --executable，每个 Bridge 固定对应一个目标。已有认证连接绑定，尚无普通用户多候选选择、显示确认或动态切换工具；开发测试助手不属于交付功能 |
| [私有发现及管道核验](../../mcp_bridge/src/qkeymapper_mcp/windows.py) | instance.json 仅含 version／instance／pipe／pid／created／session／executable 七项连接元数据。检查私有目录／文件 ACL、结构及边界，核对系统进程路径、创建时间、登录／Windows 会话和显式可信 EXE 文件身份，再核验实际管道对端；发送前、接收后重新核验。发现记录只是提示，不是授权、程序签名或业务状态证明 |
| [会话服务](../../QKeyMapper/automation/qkm_session_server.cpp)与 [凭据上下文](../../mcp_bridge/src/qkeymapper_mcp/credentials.py) | GUI 配对批准、双向证明及保存成功后才 ready，逐请求核对权限和授权代次。客户端凭据以机器、显式发现／EXE 路径和 SessionId 绑定，保存在当前用户凭据管理器并用 DPAPI 保护；配对不等于已确认本次操作对象 |
| [产品管理窗口](../../QKeyMapper/automation/qkm_control_dialog.cpp) | 只读连接路径可复制；默认存储根为当前用户 GenericDataLocation 下 QKeyMapper/MCP，子目录绑定 canonical EXE 路径与 SessionId。目录独占租约、活跃发现文件租约及发布复核使同一 EXE 路径、同一 Windows 会话目前只允许一个实例启用 MCP；不同目录可分别发布。首次未保存关闭；136按已保存选择一次恢复，配对按既有策略保留 |

#### 候选来源、职责与最小待补能力

候选首先来自客户端已配置的每目标桥接连接，或用户从目标管理窗口复制后显式导入的连接信息。另补当前用户默认私有发现根下各绑定目录的有界枚举；只读取固定 instance.json，不扫描全盘、不以进程枚举顺序选目标、不删除过期记录。过期、PID 复用、身份／会话不符、结构或 ACL 不安全的记录不能成为可操作候选；显式指定的失败项保留可解释原因。无法核验的连接状态报告未知或不可用，不能仅凭文件存在称为已连接。默认根外的目标只经显式配置／导入，不承诺发现所有运行中的 QKeyMapper；未启用接口时引导用户到目标管理窗口启用，不代为启用或启动产品。

候选摘要展示系统核实的 EXE 路径、PID／创建时间／Windows 会话、实例标识及连接核验状态。instance ID 在握手前仅是发现记录声明，认证后才与端点证明关联；已配置连接的状态带最近核验时间，不冒充实时在线。发现文件自报 EXE 不自动成为可信 EXE；用户明确选择或导入的目标仍须走既有身份／文件核验和认证。相同标题、配置名、前台窗口或枚举顺序都不作为选择依据。当前配置、映射状态等业务信息仅在认证且获 Read 权限后查询；没有权限或尚未取得时省略，不读取其他实例补全。

| 最小职责（均待实现，未冻结工具名或 wire schema） | 执行边界 |
|---|---|
| 候选与目标摘要 | 桥接返回有界候选、核验状态／失败原因及明确目标身份；复用现有私有发现与每目标 Bridge，不引入全局实例管理服务 |
| 受限目标显示 | 产品在已认证、具有独立显示权限的连接上，仅恢复显示并请求激活自身主窗口或所属模态窗。采用独立最小权限，不把 Read／Configure／RunMapping 自动视为显示授权；保留已有配对，缺少权限时由 GUI 明确增授，不无故删除重配。权限及其 GUI 增授流程均待实现；显示工具不标为 readOnly，不接受任意 PID／HWND／外部程序控制参数 |
| 显示状态观测 | 分别返回请求是否已接受、目标是否可见／恢复、是否实际核实前台归属、提示框状态、观测时间／关联记录及失败原因。只观测入口不得再次激活，供手动显示后核对；不返回其他前台应用的业务内容，提示窗 HWND 不能充当目标前台窗口 |
| 会话绑定确认 | 将有效展示记录、具体实例身份和当前客户端会话关联，复核后建立操作目标绑定；待确认整窗提示与展示记录共用存续边界，选择／展示中与已绑定状态分开，不能用 confirmed 布尔值或提示框出现替代身份、配对或权限检查 |

QKeyMapper/C++ 与桥接强制执行身份、权限、连接绑定、执行前复核和结果关联；MCP 提供清楚的结构化信息及失败原因。可选 Skill 只指导 Agent 展示候选、询问、解释和识别切换意图。无 Skill 客户端也必须能通过同一接口安全完成选择和绑定，不能把安全守卫只写进 Skill。

#### 首次绑定与可视确认

已有有效绑定且目标身份未变时直接沿用；明确指定连接／路径时跳过候选询问。首次多实例绑定仍须完成可视确认，明确路径不把该步骤视为已完成。存在多个候选且意图不明确时，展示可核实摘要，请用户选择一次；无法区分时保留“从目标管理窗口复制／导入连接信息”的路径。

流程固定为：**确定候选 → 完成认证和所需权限核验 → 恢复显示并请求激活所选窗口、启动待确认外框 → 独立核实显示结果 → 用户确认 → 建立本次操作目标绑定并清除外框**。绑定前只允许连接、权限、展示核验及有权限的必要目标摘要读取，不据此执行配置变更、启停或其他业务操作。

显示复用 [主窗口](../../QKeyMapper/qkeymapper.cpp) 的 showQKeyMapperWindowToTop()，保留其最小化恢复、隐藏显示及所属模态窗处理，不能改用含切换含义的 show/hide 入口。以实际窗口可见／非最小化状态、GetForegroundWindow、GetWindowThreadProcessId、窗口所属关系及目标 PID／创建时间核实；调用返回、activateWindow 或 SetForegroundWindow 已被请求均不等于显示成功。目标退出、激活失败、前台归属不符等要明确反馈，不自动换选；允许用户手动显示后通过只观测入口重新核实。

核实后使用如下询问；配置行仅在有权限且实际取得时显示：

> 请确认：刚刚显示到前台的 QKeyMapper，是否就是本次要操作的实例？
>
> 目标路径：……
>
> 当前配置：……（仅在有权限且已取得时显示）

确认关联刚才已展示并核实的具体实例、当前会话和展示记录。**用户返回 Agent 输入会正常改变前台窗口，不因此判定确认失败。** 接收确认时复核该实例仍存活、身份未变且展示记录仍有效，不要求 QKeyMapper 在确认输入瞬间仍占前台。候选更换、身份变化、旧展示记录失效或目标退出时重新展示核对，不沿用陈旧确认；“不是”返回选择流程，取消结束本次选择，均不执行配置变更。这里确认的是操作对象，不代替产品 GUI 配对或权限批准。

#### 待确认的整窗黄色闪烁提示（DESIGN-58，待实现／待验证）

目标外观为整窗黄色细框，包含标题栏，而非仅在内容区画框。静态核对发现仓库已有透明提示窗、鼠标穿透和不激活显示的做法；两套本地 Qt 头文件均包含 WindowTransparentForInput／WindowDoesNotAcceptFocus。采用本进程的透明无边框提示窗绘制，复用现有窗口及事件机制，不引入新框架，不修改原生边框或正式 manifest。Qt 的[透明窗口](https://doc.qt.io/qt-6.8/qwidget.html#creating-translucent-windows)及[输入穿透／焦点标志](https://doc.qt.io/qt-6.8/qt.html#WindowType-enum)支持该路径，实际系统效果仍须验收。

- 完成原有认证和独立显示权限核验后，复用 showQKeyMapperWindowToTop()，为当前实际展示的主窗口或有效所属模态窗口绘制外框；不接受任意 PID／HWND，不因提示请求启动或停止映射。
- 提示窗归属目标进程，透明、鼠标穿透、不接收焦点、不进入任务栏；遵循目标窗口层级，不全局置顶。闪烁只重绘，不重复调用窗口激活／置顶，不成为前台或身份核验的替代证据。
- 默认黄色 #FFD400、约 2 个逻辑像素宽，每 500 毫秒切换亮灭；边线沿实际可见整窗边界绘制。跟随移动、缩放、最大化和 DPI 变化，统一坐标系；屏幕边缘／最大化不丢失可见边线，最小化／隐藏时不留悬空外框。恢复可见时仅在原待确认记录仍有效时恢复提示。
- 记录关联具体实例、客户端会话及展示代次，每实例同时仅一条待确认展示记录；其他会话请求不得覆盖，返回可解释的忙状态。结束回调只清除匹配代次，旧取消／超时不能清掉新提示，不建立全局当前操作目标。
- 用户返回 Agent 时待确认记录继续有效；确认成功、否认、取消、断线、权限撤销、接口关闭、实例身份变化或退出时清除对应提示。记录默认五分钟绝对期限，到期清除并要求重新展示核对，不自动绑定；查询／闪烁不延期。该期限与 gsudo PID 缓存没有关联，不延长其授权。
- 显示观测单独返回提示框状态及失败原因；边框出现不等于前台核验成功、已绑定或已授业务权限。显示／绘制失败明确反馈，保留原手动显示后核验及切换失败保留原绑定的规则，不伪报提示已出现。

Windows 窗口捕获也会由系统绘制黄色提示边框，见[微软说明](https://learn.microsoft.com/en-us/windows/apps/develop/media-authoring-processing/screen-capture)；用户截图只作为外观参考，不据此推断 Codex 内部机制。本设计不为画框启动截图／录屏或新增捕获权限。实现与新增针对性验收另计约 4–8 小时有效作业时间，纳入 M1，复用已有显示／会话及双系统测试，避免重复计时。

#### 会话绑定、主动切换与恢复

绑定结果明确告知：**“已绑定：办公版 QKeyMapper（目标路径……）。后续操作将使用此实例。如需更换，请输入‘切换 QKeyMapper’。”** “办公版”等别名仅辅助辨认，真实路径及身份是核验依据。正式说明统一使用“切换 QKeyMapper”，Skill 可识别相近自然语言表达。

绑定作用域是当前客户端／MCP 会话，关联每目标连接、可信 EXE／发现位置、instance ID、实际 PID／创建时间及登录／Windows 会话。执行层按该绑定复核目标；不建立能被其他会话改写的全局“当前 QKeyMapper”，也不原地改写现有 Bridge 的目标路径来承接旧请求。待确认候选和原已绑定目标分开维护。查询、预览、提交、operation 及结果均关联同一目标身份并提供摘要；旧目标引用／预览不得提交给新实例。

用户主动切换时重新选择、显示、核实和确认，新目标确认成功后才替换会话绑定；失败或取消保留原绑定，原目标已失效则明确报告，不称为仍可操作。普通查询及连续操作不重复激活或确认。切换只改变后续操作目标，原实例的映射、接口与配对按原状态保留；已经接受的任务和结果继续归原实例，保留原目标结果查询关联，不迁移、不重发、不按新目标解释。无法确认的旧任务结果如实报告未知。

目标退出、连接失效或身份变化时明确失败，不自动改连其他实例执行原操作。连接短暂失效但仍是同一存活实例，重新认证并复核身份／权限后可沿用已有绑定，无需重复窗口确认。重启、PID 复用或接口重建导致 instance ID／进程身份变化时，重新发现与核验并再次显示确认；旧引用／预览失效。持久化配对在机器、发现／EXE 上下文、会话及授权仍适用时复用，不因重启无故要求重配；接口未启用／恢复失败或授权失效时按既有机制处理，不自动批准。

#### 后续实施与待验场景

当前优先顺序见 REVIEW-59；M1.2a-2b-3 的 STOP-57 保留，先经过公共引擎验收门槛。本设计归入后续 M1 普通用户连接／会话切片，按“候选及结构化摘要 → 独立显示权限、整窗提示与观测 → 会话绑定／切换和跨实例引用守卫 → 正式 MCP／GUI 对照”实施，在普通用户闭环交付前验收；完整界面能力仍按 M2-B 扩展，免 Python 包及干净双系统交付归 M3-B。DESIGN-58 当轮仅更新文档，不改变 STOP-46／56 原验证范围。

| 待验证组 | 必须覆盖的场景与判据 |
|---|---|
| TARGET-01 候选与选择 | 不同目录多个实例、相同标题／配置名和前台变化；明确连接／路径或有效绑定不重复选择；同一路径／会话只能一个实例启用 MCP；无法辨认时从目标管理窗口复制／导入 |
| TARGET-02 发现与权限 | 未启用、未配对、过期记录、ACL／身份不符、连接不可核验及显示权限不足；不自动启用／改连、不泄露未授权业务信息，增授显示权限不扩大 Read 或丢弃原配对 |
| TARGET-03 显示与人工确认 | 最小化、托盘隐藏、显示成功、激活失败、前台归属不符、手动显示后核对；提示与前台归属分别核实，提示窗不能冒充目标；确认／否认／取消及返回 Agent 输入引起的正常焦点变化，后者不能造成误拒绝 |
| TARGET-04 身份与恢复 | 展示后目标退出、PID 复用、重启、接口重建或身份变化；陈旧确认／引用／预览拒绝，同一存活身份重连不反复确认，适用的持久化配对不重复批准 |
| TARGET-05 切换与会话 | 主动切换成功、取消及失败，原目标失效、旧引用／预览误用、进行中任务结果仍归原实例、两个客户端／会话选择互不改写；普通连续操作不反复激活 |
| TARGET-06 实际业务隔离 | 正式 MCP 对选定实例产生预期业务结果，以独立 GUI／实际数据及适用最终效果核对；其他实例未收到该操作，不以标题、前台或助手参数回显冒充证明 |
| TARGET-07 整窗提示生命周期 | 标题栏在内的边线／亮灭、鼠标穿透、无新增焦点／任务栏、遮挡层级、移动／缩放、所属模态、最大化／屏幕边缘及跨屏 DPI；确认、否认、取消、五分钟到期、断线、撤权、禁用和退出清除；隐藏后恢复、旧代回调和多会话不得覆盖；正常查询不重激活或延长提示 |

**必要 Default 桌面场景：TARGET-03 及首次多实例确认、主动切换、展示后身份变化的代表交互，每个系统集中预计 15–20 分钟。** 前台与托盘属于真实用户桌面事实，私有桌面成功不能替代。后续实施时先告知，再在明确的自有实例上使用产品受限显示入口、窗口可见／最小化状态、前台 HWND／PID／创建时间及所属关系核验；让用户返回 Agent 确认、否认或取消，并用 scripts/capture_ui_snapshots.ps1 留图后自主检查。截图工具自身可能尝试激活，须先记录显示核验结果并注明拍摄焦点影响，不能以拍摄造成的前台变化倒推显示接口成功。会短时恢复窗口并改变焦点，允许有限鼠标／键盘确认，不发送映射测试按键或操作其他实例；实际桌面、方法和中断影响逐项记录。所需 High 截图或托盘证据无法取得时明确保留未验证，不扩大提权边界。

TARGET-07 的状态／绘制逻辑优先在私有桌面验证；真实前台、遮挡、标题栏／边框鼠标穿透、物理拖动及跨屏 DPI 使用 Default 上自有实例，每系统合并原 TARGET 场景额外预留约 10–15 分钟（合计约 25–35 分钟）。使用有限真实鼠标拖动／调整尺寸及返回 Agent 确认，临时影响焦点和输入；不发送映射测试按键、不操作用户实例。截图使用现有脚本并自主审阅，实屏画面须实际包含辅助外框和目标窗口，亮／灭分别留证；仅主窗口 PrintWindow 不能证明独立外框已正确合成或遮挡正确。非提权截图不足时保留未验证，不为补图提升截图权限。

身份／失效／会话组件可先隔离验证；TARGET-06 的具体业务沿对应能力既定 GUI／MCP 等价性范围执行。Win10／Win11 分别记录，英日维护受影响的文字／交互但中文优先。本轮只补要求，未执行任何 Default、产品或运行测试，也不把文档批准当成此刻启动测试的授权。

### DEMO-SESSION：基于预置 Release 的私有演示与截图会话（2026-10-27 新增实施目标）

用户明确要求追加实现及验收，不是仅补设计。本节窗口观察／截图／私有单项开关已在107／108／109／110完成选定范围，其余显示及完整会话交付仍**待实现／待验证**；P0内部捕获底层和选定可行性结果见下文，现有测试用私有桌面、截图脚本和开发gsudo控制器不构成已交付服务。保留当前SAVE-95与此前源／权限／保存结果证据，沿当前C1-C保存闭环节奏继续，在保存执行安全及相关窗口／会话工作的自然检查点安排DEMO-P0，不为截图验证临时切换开发队列；不恢复环境、不重置现场，也不因新目标放弃M1完整保存／启停。新增进度只在本计划维护。

目标是Windows执行端上的专用QKeyMapper演示实例：hermes QQBot的Agent按咨询建立真实示例、展示对应窗口、取得可检查图片及说明，再由gateway按原QQ会话回传。当前PC-W11没有hermes／QQBot gateway环境，在本机实现产品／桥接／本地Agent可消费闭环；**QQ渠道联调另排开发末期PC-W10**。本机图片产生、MCP返回ImageContent或本地文件均不等于QQ已发送／已送达，缺gateway源码、指定测试对象或发送授权时只报告可接线产物和待联调项。

本轮自然切点核对（2026-10-28）：SAVE-101选验真实清理／观察超时后保存，SAVE-102选验清理后新增／保存／冷读，SAVE-103选验停止回切与目标重核，均已聚焦本地提交；保存接口的范围确认、文件／派生修订、分段失败、超时／断线结果恢复沿SAVE-87至100原证据保留。C1-C／M1未整体标完，物理输入、设备／迟到事件、撤权交错、4MiB性能与完整GUI仍待。现进入与源／窗口会话衔接的DEMO-P0，不重跑上述已验矩阵。

#### DEMO-104～133 历史实施与选验压缩索引（全文固定锚点 `8cbf5eb3`）

> 本表汇总 `DEMO-P0-104` 至 `DEMO-P3-133` 的历史实施契约、选验结果与未闭环边界；完整全文固定保存于 `git show 8cbf5eb3:handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md`，详证与失败轮次见 [map67c1_validation.md](map67c1_validation.md)（表中提及的 `out/mcp_*` 目录仅存 `PC-W11` 本机，不随 Git 同步）。

| 切片范围 | 核心交付／契约要点 | 选验结论与证据入口（PC-W11） | 遗留边界与跨机待办 |
|---|---|---|---|
| `DEMO-P0-104` | `captureOwnWindow(QWidget*)` 无激活／无文件 I/O 内部截图，正式 P1 约定 PNG 上限 `160 KiB / 8M px` | 双 Qt 私有采样 4 图（main `1130×722` / item `820×480`）通过，源/INI/dirty 不变 | 原生边框/未支持子窗不截、HighDPI/Win10/长期服务另验 |
| `DEMO-P1-105` / `DOC-L10N-106` | `ShowWindows=128`／`CaptureWindows=256`、`updatePermissions` 追加入口、`W1–W8` 获批三语；`106` 固化经审阅 TS/QM 工作流 | 双 Qt 组件／IPC／12 张截图、`W7` Linguist 复核、QM `1152/1155/1155` 一致；[105 证据](map67c1_validation.md#demo-p1-105独立窗口权限及追加授权2026-10-28pc-w11) | 正式窗口端点与 High 实测接续至 `107～110` |
| `DEMO-P1-107` / `108` / `109` / `110` | `windows` 只读与窗口引用（`107`）、`160 KiB/8M px` 有界 PNG（`108`）、`qkm_open_mapping_editor` `WA_ShowWithoutActivating` 私有后台打开（`109`）、`qkm_close_mapping_editor` 不丢草稿关闭（`110`） | 双 Qt／正式 SDK／实图与子窗拒绝通过；[107](map67c1_validation.md#demo-p1-107正式窗口只读查询及引用2026-10-28pc-w11) · [108](map67c1_validation.md#demo-p1-108正式有界窗口png2026-10-28pc-w11) · [110](map67c1_validation.md#demo-p1-110正式打开记录及安全关闭2026-10-28pc-w11) | 跨 Tab/过滤/撤权退出/HighDPI/Win10、P2/P3/QQ 待续 |
| `DELETE-GUI-111` / `DELETE-112` / `DELETE-113` | `111` GUI 原事件删除基线；`112` `removeMappingRows` 共用提取；`113` `mapping.delete`（1–100 `source_refs`、`confirm_delete=true`、编辑窗/表格/菜单/隐藏行守卫） | 双 Qt 私有真实 GUI 对照、组件与 12 场正式 stdio（471 SDK 调用、INI 不变）全通 | 真实筛选 GUI 触发、运行后清理、撤权/过期/断线删除、Win10 |
| `DEMO-P2-114` / `115` / `116` / `117` / `118` | `qkm_get_demo_state` 就绪预检（`114`）、`demo_context` H0 非行保存足迹（`115`）、`source_binding_after` 提交快照（`116`）、`qkm_demo_mapping` `item_setup`/`main` 串行建例—截图—回收（`117～118`） | 双 Qt 组件／正式 stdio／PNG 核验；[115](map67c1_validation.md#demo-p2-115非行保存足迹2026-10-29pc-w11) · [117](map67c1_validation.md#demo-p2-117正式串行单行演示2026-10-29pc-w11) | 真实 PID 复用/退出/撤权/断线/超时、`125` 后仅新只配置模式准入、Win10 |
| `DEMO-GUI-119` / `P1-120` / `P1-121` / `P2-122` | `QMappingAdvancedDialog` 原 GUI 基线（`119`）、`mapping_advanced` 只读观察/捕获（`120`）、`qkm_open/close_mapping_settings` 归属开关与 `settings_editor_unchanged`（`121`）、串行 `view=mapping_advanced`（`122`） | 双 Qt 组件／真实高级窗 `770×390` PNG／草稿保护与回收集通过；[120](map67c1_validation.md#demo-p1-120高级窗只读角色观察捕获2026-10-29pc-w11) · [121](map67c1_validation.md#demo-p1-121高级窗私有归属开关2026-10-29pc-w11) · [122](map67c1_validation.md#demo-p2-122固定高级窗串行视图2026-10-29pc-w11) | 通知子窗显示、实际撤权/断线/跨实例、Default/Win10/HighDPI/QQ |
| `PKG-123` / `PKG-130`（已由 `RUST-D4` 归档替代） | 历史嵌入 [PSF 3.11.9 x64嵌入归档](https://www.python.org/downloads/release/python-3119/) ＋29 lock wheel 免安装 Python 桥接包组装与 `version2` 契约刷新 | 独立 ZIP/解压/PSF 签名/中文空格隔离/Qt6 私有实跑通过；[123](map67c1_validation.md#pkg-123固定嵌入运行时桥接组件包2026-10-29pc-w11) · [130](map67c1_validation.md#pkg-130version2契约的组件包刷新2026-10-30pc-w11) | 正式发行改以 `RUST-D4` Rust 单运行文件在 M3-B 收口 |
| `DEMO-P3-124` | 不可变 `--demo-configuration-only` 门禁（前置盘点 [main](../../QKeyMapper/main.cpp)、[Worker](../../QKeyMapper/qkeymapper_worker.cpp)、[Interception](../../QKeyMapper/interception_worker.cpp)、[GUI](../../QKeyMapper/qkeymapper.cpp)）、拦截 Hook/设备/驱动/FilterKeys/Run、`execution_policy`、旧 511 运行时降交集 `387` | 双 Qt 40 私有组件＋6 场正式 stdio＋8 图字节核对，确认设备/Hook/libusb/SDL 未初始化；[124](map67c1_validation.md#demo-p3-124只配置门禁2026-10-29pc-w11) | 危险 INI 全组合、Win10/HighDPI/ASan、与正式发行包集成 |
| `DEMO-P3-125` / `126` | 严格 `version2`＋`startup_purpose=configuration_only_demo` 契约（`125`）；只配置产品自有发现目录自发布/句柄验删 `version2` 记录（`126`） | Python 34/私有 48 组件＋Qt6 正式 stdio 消费产品记录与两图回收通过；[125](map67c1_validation.md#demo-p3-125正式演示绑定契约2026-10-29pc-w11) · [126](map67c1_validation.md#demo-p3-126产品自身私有创建记录2026-10-29pc-w11) | Qt5 SDK 自发布消费、删除竞争/异常退出矩阵 |
| `DEMO-P3-127` / `128` / `129` / `131` | `--demo-host` 同 EXE 私有子进程与租约（`127～128`）、仅继承两匿名事件的 `STARTUPINFOEX` 父子管理窗通道与 `H1–H9` 三语（`129/131`） | 双 Qt 组件／正式宿主创建、存活关闭、父子 0 与私有提示通过；[127](map67c1_validation.md#demo-p3-127固定宿主进程归属核心2026-10-30pc-w11) · [128](map67c1_validation.md#demo-p3-128正式宿主入口与存活产品关闭2026-10-30pc-w11) · [129](map67c1_validation.md#demo-p3-129父子专用管理窗通道2026-10-30pc-w11) · [131](map67c1_validation.md#demo-p3-131人工配对入口及私有原生提示2026-10-30pc-w11) | 15 分钟实等超时、真实桌面恢复失败/锁屏/宿主崩溃、Win10 |
| `DEMO-P3-132` / `133` | 真实 Default 原生 Shell/UAC 人工切入配对 `387` 与去除多余 `SetThreadDesktop` 修复 `thread_restore_failed 170`（`132`）；`--product-demo-record` 固定按实例 ID 绑定产品记录（`133`） | 真实 High/Medium 双图与回收、新版 Default 重连返回、随包引导实测通过；[132](map67c1_validation.md#demo-p3-132真实gui预热及一次性线程收束2026-10-30pc-w11) · [133](map67c1_validation.md#demo-p3-133固定产品记录与随包引导2026-10-30pc-w11) | 跨登录/PC 重启、Default 整屏/Qt5 普通/Win10/QQBot gateway 联调 |

#### 复用、隔离及专用目标

- 接受Agent可访问工作区中的完整Release，先核对实际EXE／DLL／桥接及能力发现；不能只按版本号假设新接口存在。接口缺失在当前仓库实现并交付新版。演示运行端最终只用完整Release及随包桥接，不编译、不依赖仓库／venv／用户全局Python；当前免Python交付仍待M3-B，演示交付所需打包试用提前作为依赖推进，未完成不得称运行端可直接使用。
- 复用每目标桥接、私有发现、实际进程／Windows登录会话核验、认证、独占租约、持久化配对和现有GUI／业务入口；不增加全局实例管理服务或通用远程控制框架。最小版仅一个专用预授权实例串行处理咨询，每次先恢复已核实的演示初始状态，完成后重置／回收；繁忙明确返回，暂不做实例池或并发多租户调度。
- 普通权限执行端准备独立运行副本和固定演示INI，**仅改cwd不足隔离，产品会切到EXE目录**。不复制用户个人INI、配对凭据或开发缓存PID。可信创建记录绑定实际EXE路径／必要制品身份、PID／创建时间、登录会话、实际私有桌面及MCP实例；退出／PID复用／重启／接口重建或身份变化使旧场景、引用、预览及窗口捕获令牌失效，不自动换到其他实例。
- 专用后台实例依据可信创建记录完成目标核验，不套用TARGET-DESIGN-47的Default前台展示／人工确认流程；普通用户多实例的候选、前台确认、独立显示权限和切换规则保持原约束。普通用户实例不能由客户端自行声明为“演示模式”而获得重置／丢弃草稿许可。

#### 一次预热授权与正常咨询边界

正式最小方向是**一次明确部署／预热、一个持续存活的专用实例**：用户核对专用Release／运行目录、私有桌面及普通桥接身份，完成必要原manifest UAC和本地配对／最小权限批准，正常咨询仅复用既有认证实例。不承诺跨退出／重启自动保留管理员令牌；预热实例或授权失效时返回“演示端未就绪”，由明确预热流程恢复，咨询请求不能自行弹UAC或激活Default。具体私有桌面启动／可信创建与生命周期方案先经DEMO-P0源码可行性和具体方案评审；新启动模式、权限或系统行为超出现授权时，列明影响、失败恢复和必要测试，由用户决策后执行。

开发gsudo缓存仅供已授权测试，不进入Release服务依赖、不作为正式管理员会话或任意客户端自动批准方案；不注册计划任务／服务、改全局UAC或改正式manifest来省略批准。普通桥接、Agent、Git、构建、图片落盘和开发工具保持普通权限。显示沿TARGET-DESIGN-47的独立最小权限设计；截图也须明确最小权限和隐私边界，不把已有Read／显示批准自动扩成屏幕捕获，配对持久化沿原机制复用。

默认只配置示例，RunMapping不授予；不开始映射、不注入系统输入、不安装或修改驱动。逐项审阅产品启动、配置装载、设置联动及重置中的FakerInput／ViGEm／Interception／Hook／FilterKeys等副作用，使用安全演示初始项并明确不支持的设置。私有桌面不证明驱动或Hook隔离，状态／零计数也不能代替实际清理；若后续设置涉及系统作用或需要新增Default／提权输入，先交具体方案，禁止在咨询中静默扩大权限。

#### 受限展示、捕获及图像交付

窗口参数只取本实例已知角色／受控页面和绑定业务源的窗口引用，例如main、单条映射设定、代表性映射高级页面；先核验实例及窗口生命周期、源引用／场景代次和权限。复用真实GUI展示入口并按明确策略区分普通前台展示与私有后台无激活展示；后台路径不得调用SwitchDesktop、主动激活Default、发送系统输入或改用户实例。不得开放任意PID／HWND、Qt属性读写、脚本／命令或管理员任意文件路径。

优先小范围验证产品GUI线程内QWidget::grab()/render()的自身窗口捕获。场景必须先经正式MCP业务操作建立，窗口由共用GUI入口展示；独立GUI基线通过真实控件事件形成，不能直接设值、拼图或示意图代替软件截图。捕获前等候必要绘制并重核对象／源／场景，关闭、隐藏／最小化、模态、超时或身份变化明确失败；不返回上一场景图片。说明Qt客户区与原生边框、独立弹窗／原生子窗、DPI／像素大小和绘制完成范围，未支持角色拒绝。传输大小、像素和编码上限由P0实测校准，超限明确错误，不截断；先选最小可行传输，不预设通用截图服务或分块缓存。

图片来自产品自身已核验窗口，返回PNG／受控图像内容及instance、scene、window角色／引用、源绑定、尺寸／DPI、时间、格式和大小等必要元数据。桥接只对配置的普通权限产物根写入服务生成文件名，校验根目录／重解析、大小、数量、保留时间和清理归属，不接受QQ提问中的任意路径；C++不接收图片落盘路径。MCP可提供原生ImageContent及／或受控产物引用，具体以Agent／gateway实际消费能力选择；结构化元数据不重复夹带整幅base64。接口安全不依赖Skill，Skill只解释场景、展示结果和指导自然语言。

同一专用实例的场景、捕获和重置须串行关联当前认证owner与场景代次；旧源／窗口／图片引用不能用于新实例或新场景。清理只作用于本演示实例、临时编辑状态和自有产物，不以STOPPED或进程不存在代替清理证明。普通用户草稿不适用演示重置策略。失败后无安全初始状态则隔离该会话并报告，不能把部分场景继续给下一位QQ用户；gateway另负责把图片及说明关联到正确QQ会话，不能由MCP场景ID推定QQ身份认证已完成。

#### 实施顺序与验收切点

| 切片 | 依赖及具体交付 | 验收与当前边界 |
|---|---|---|
| DEMO-P0：小范围可行性 | DEMO-P0-104已完成选定底层／采样：Win11双Qt正式MCP建例、真实GUI main／单项窗、四张PNG与Agent看图、无激活捕获函数、场景保持及两类拒绝；预热授权具体方案继续细化 | 仅维护可行性与当前DPR1结果，不是正式接口或完整无打扰／服务验收；P1至P3依赖和未验边界见本节，Qt／Win32新启动／权限范围先给方案，不自行实施 |
| DEMO-P1：正式窗口与图片能力 | 固化最小权限、角色／页面与源／窗口引用；在C++和普通桥接实现受限展示／查询／捕获及有界产物消费，保留首次默认关闭、136用户保存选择及配对规则 | 正式SDK调用；验证图片真实、中文清晰及元数据一致；无Skill客户端安全。新增权限UI／三语资源沿UI-73流程，未获授权不自动改TS，未完成范围明确保留 |
| DEMO-P2：最小串行演示闭环 | 复用已验mapping.add预览／提交及必要源生命周期协调；专用初始状态、场景绑定、重置／回收；不以未实现保存为隐含前提，也不绕过真正重置的源写入安全门槛 | 独立演示实例→正式MCP建立A→B→打开单条映射设定→获取真实图→Agent自主查看→重置／回收。连续咨询和失败后恢复不污染后续场景，其他实例／INI不变 |
| DEMO-P3：预置Release部署／预热 | 具体授权方案获准后实现正式私有启动／创建记录及预授权实例复用；提前复用M3-B免Python包与说明；复用已验main／单项及按依赖补齐的代表高级页，其他编辑场景依赖M1／M2逐项开放 | 仅完整Release／随包桥接的执行端可用，不依赖源码／venv／编译器；一次预热后正常咨询无逐次UAC、Default确认或开发缓存。实例失效不静默提权 |
| DEMO-P4：Win10渠道联调 | MCP开发末期，PC-W10具备指定hermes／QQBot gateway源码或接线对象、Windows执行端及受控QQ测试会话／发送授权后单独安排 | 核对Agent实际收到图像／文件、gateway正确取图及回传／失败报告。PC-W11缺该环境，当前不执行或声明QQ送达；渠道缺口不阻塞本机产品与可接线图像交付 |

P1／P2须从P0继续正式实现，不停在out临时助手或文档。P2与P3依赖交叉时先完成已有授权的配置／截图组件，再处理经批准的启动和免Python交付；“可用闭环完成”必须同时具备正式端使用方式、重置和普通咨询无打扰证据，不能用开发缓存冒充。

验收至少覆盖主窗、单条映射和代表性高级页；最小化／托盘／独立弹窗、实际DPI与绘制等待；关闭窗口、错误角色、身份改变／PID复用／重启、超时、连续请求、过期引用、满额／落盘失败及失败清理；核实截图中的A／B和已提交源一致、无黑图／错误窗／旧场景污染。Agent须实际查看原图，功能查询及计数不能代替视觉检查。原业务、正常配对、普通多实例选择、权限不足及未支持设置按受影响范围回归；Qt6／Qt5及Win11／Win10分别记录，不倒填历史结果。

**正常咨询无必要Default输入／前台操作；优先真实私有桌面验证。** 被动记录实际桌面及产品／桥接有无请求激活、切桌面／输入注入，并按窗口归属及事件来源区分用户主动前台切换，不能仅因前后前台不同认定干扰。正式一次预热／UAC与Default授权交互属于另列必要场景，待具体方案批准后每系统集中预计5–10分钟；说明焦点／输入影响再执行，不沿用TARGET前台确认测试替代。开发缓存的启动成功不算正式预热已验。QQ联调只在具备环境的PC-W10末期执行。

新增范围不计入下述旧60–106小时剩余基线；P0完成后按实际捕获、启动与打包方案校准新增有效作业时间，并把可复用M1／M2／M3-B分账，避免重复计算或把未知QQ环境等待计为已验。继续自选聚焦本地commit，不推送；没有用户收尾指令就沿依赖推进可执行工作。

### M1 首个闭环契约：v0.1 设计基线

本轮静态链路依据见能力矩阵的 `MAP-ADD`、`MAP-EDIT`、`MAP-DELETE`、`CFG-SAVE`、`RUN-START`、`RUN-STOP` 和 `STATE-SOURCE`。下列决定约束首个实现，未通过运行测试前不标记为冻结或支持。GUI 和 MCP 共用这些业务语义；GUI 的选择、键盘修饰键及弹窗属于输入采集。

#### 范围和接口分工

首个可用版本只声明已经通过等价性验证的普通键盘映射增删改、当前配置保存、状态查询与明确启停／等待；完整目标仍包含全部设置。高级 DSL、宏、设备及系统操作按能力逐项扩展，未支持内容返回明确错误，不能静默忽略或夹紧参数。

桥接只维护已知命令映射和 MCP 结果封装，不判断映射规则，不透传任意方法／属性／INI。正式回归脚本从官方 MCP SDK 的客户端连接正式桥接进程，通过 `initialize`、`tools/list`、`tools/call` 执行；直接 IPC 单测只证明内部协议，不能替代 MCP 路径验收。SDK 版本在隔离环境建立时固定并记录，打包前验证依赖及离线启动。

| 工具 | 最小输入／输出约束 |
|---|---|
| `qkm_get_capabilities` | 返回接口版本、已实现／已验证能力和参数 schema、依赖、当前授权、读取分页限制、持久化及生效语义；“计划支持”不混入可调用能力 |
| `qkm_get_state` | 返回实例身份、业务修订号、当前配置及真实源表、dirty、监视／匹配／Worker 状态、清理是否完成和实际停止快捷键；敏感文字按授权返回 |
| `qkm_query` | 首轮使用明确枚举的配置／Tab／映射查询，分页且有界；源行与共通显示行分开，编辑数据与运行有效数据分开；不接受任意对象路径或脚本表达式 |
| `qkm_preview_changes` | 接受实例／配置快照引用和带类型的命令；调用共用校验、规范化、冲突规划与风险分类，返回预览 ID、实际连带变化、是否需要停止及过期条件；不写主数据、dirty 或磁盘，也不采集真实输入 |
| `qkm_apply_changes` | 只接受预览 ID 与去重请求 ID；按预览绑定的明确目标、内容、冲突选择和保存意图提交，重新校验目标、权限、状态和修订号；不能在此临时改变预览内容 |
| `qkm_start_mapping` | 明确目标和保存策略，重复开始不会停止；目标是“开启监视”，窗口匹配及实际映射是否生效另报，不能把未匹配当成实际映射成功 |
| `qkm_stop_mapping` | 明确停止；重复停止不重新启动或重复执行停止动作；有停止发送或其他风险时先走有效授权。操作接受后返回 operation ID，真正完成由清理边界决定 |
| `qkm_get_operation` | 查询同实例的去重请求／operation ID，包括已接受、执行中、等待清理、完成、失败、部分完成；返回实际结果，不因客户端断线删除仍在执行的任务 |
| `qkm_wait_state` | 有界等待已命名的条件，例如 `monitoring_enabled`、`mapping_active`、`stopped_and_quiescent`；超时只报告观察结果，不发新操作，不阻塞 GUI 事件循环 |

`qkm_get_help` 和诊断工具沿用总体接口职责，按实际内容接入；它们不是首轮普通映射闭环的完成替代物。MCP 的 `readOnlyHint` 等注解只用于客户端理解，真正的权限与风险检查在 C++ 执行，依据 [MCP 工具规范](https://modelcontextprotocol.io/specification/2025-06-18/server/tools) 的注解语义。

BRIDGE-60 已统一现有七工具的结果语义：调用自身失败返回原生 isError=true，结构化内容及其 JSON 文本包含 ok=false／error、受控 message、固定错误码对应 next_steps；产品错误码限制 1–64 个小写字母／数字／下划线且字母开头，消息 UTF-8 不超过 1024 字节。只有已审阅产品错误保留端点解释，原生异常或未知错误不直接透出文本。accepted=false 是调用拒绝，accepted=true 仅为接收；成功查询到失败状态／失败 operation 仍是成功查询。SDK schema 校验沿原标准错误通道，C++ 业务 invalid_arguments 使用结构化业务错误；不绕过未知字段和类型／范围检查，不把解释性下一步自动执行。当前没有新增 operation 或写工具。

#### 目标引用、修订与预览

目录查询先作 M1.2a-2a：qkm_query 只接受 configurations／tabs，返回配置目录和当前 Tab 元数据，包含未附着共通表的目录记录，不读取映射行或未加载配置内容。分页 limit 为 1–100，offset>0 必须携带 catalog_revision。目录修订号按实例、配对、权限代次和实际目录内容生成，每页重新核对，相关目录／授权上下文变化返回 stale_catalog；不保留目录缓存或定时器，响应仍受原帧大小限制。写入预览的 120 秒上限不变。

catalog_ref 只表示对应目录修订中的条目，**不是映射编辑目标，也不是完整业务修订号**；映射行变化不由此宣称已经跟踪。selected_configuration 明确表示界面选择，不充当已加载数据来源证明。稳定源行引用、实际来源身份、相关配置修订和提交复核在 2b／预览提交切片闭环。此分步处理不缩减完整源查询、GUI／MCP 等价性或开发复现测试目标。

- 业务目标包括 `instance_id`、配置快照、源 Tab／源行引用，区分自有和共通来源。首先采用实例内、绑定查询修订号的不透明引用；不为第一版修改现有 INI 格式增加永久 UUID。加载、重建、重排等使引用失效时要求重新查询，不猜测相同显示行就是原目标。
- GUI 采集显示选区后立即转换成源引用；MCP 从查询取得引用。业务函数接受明确源目标和位置，不能借助临时切 Tab、模拟 Shift／Ctrl 或覆盖用户编辑框内容决定操作意图。
- 一次变更在 GUI 线程内串行校验／提交；加载、退出、正在录制或存在不允许合并的编辑会话时返回可解释状态。不能依赖模态循环维持原目标有效。
- 修订号覆盖相关主数据、派生配置及授权条件变化；不能只监听 dirty 信号，因为部分即时保存、动态 UI 或加载路径有不同语义。提交前还比较规范化后的相关状态快照，防止漏接 GUI 变更。具体挂接点必须随能力盘点核对，不引入每个输入事件的配置哈希。
- 初始预览有效期上限拟为 120 秒，并绑定实例、配对客户端、作用域、修订及权限代次；过期／条件改变返回 `stale_preview`，不先写一部分再要求确认。
- 冲突规划返回“新增项禁用”“指定其他项禁用”“取消”等结构化结果及影响清单。GUI 弹窗与 MCP 明确选择进入同一规划／提交路径，不能让 MCP 远程请求停在不可见模态确认中。

#### 变更与保存结果

命令采用区分类型的字段，例如 `mapping.add`、`mapping.update`、`mapping.delete`、`configuration.save`；每种类型只接受已声明的参数。未指定字段保持原值；原始键、主映射、抬起键、Disabled、备注等不借空字符串表达多种不同含义。

新增使用完整默认构造和已有互斥规则；更新复用键串正规化／校验、Pure 字段、备注对齐及联动；删除复用源行转换后的删除、鼠标点及显示刷新。初始化／加载 setter 不得冒充用户提交入口。共用提交返回业务结果，界面各自显示提示，避免逻辑只存在于 popup／槽函数里。

`configuration.save` 是显式意图，保存范围按原 GUI 定义报告，不能让用户误以为只保存刚添加的一行。单纯内存批次先整体校验，再一次提交；包含保存的请求分成内存提交、磁盘同步、重载确认三个可报告步骤。磁盘失败时保留可恢复草稿，返回 `modified=true, saved=false` 等实际结果，不能承诺整个跨步骤操作原子回滚。

完成结果至少分别说明：已修改哪些实际字段、未保存状态、磁盘保存结果、实际生效状态／条件、运行状态及后续动作。修改后默认停止，并提示输入 **开始映射**；等待用户开始意图，不因保存或断线恢复而自动开始。

#### M0.3b 共用操作提取边界（待实现，不是已开放接口）

源码 `2b9215ef` 的第一批 M0.2b 追踪补充在能力矩阵，共 30 条静态链路。新增事实约束首个闭环：`prepareMappingDataForInsertion` 会改变其他表，`allowCommonConflictPrompt=false` 也不是只读；Tab 移动末端回调则假定 Qt 页序已经变化。提取时先确认入口前已经发生了什么，不能按函数名直接复用作预览或完整操作。

第二批通用／映射高级设置再补 21 条，总计 51 条，见 SRC-28。有效值存在控件中的项目也必须共用完整提交：textEdited 与 setText 的 dirty 差别、共通开关的跨表 Disabled 修复、自定义通知首次初始化、阈值的三元组联动，以及只在启动／重启读取的 Worker timer 都属于业务语义。初始化／加载的信号屏蔽和全局／配置作用域应保留；不为接入而全局重写设置模型。现有 Worker 直接读控件的项还需核对线程一致性，静态定位不证明安全。

第三批配置／窗口／忽略规则再补 17 条，总计 68 条，见 SRC-29。配置选择与缩放同样不能仅 setCurrentIndex；窗口条件编辑串、MAP_PROCESSINFO 和自动匹配读取的磁盘组必须分别报告。组导入仍需能力覆盖、覆盖／保留规则和风险分类，不能透传任意根键；现有传输缺少 status 检查及导入顺序处理位置的静态差异列入接入前复现／共同修正。显示顺序不等于自动匹配优先级。

SRC-65 在上述 MAP-ADD／EDIT 内补首闭环参数事实：键名来自真实有效列表，语法占位符另处理；设备与按压、等待 0 不追加后缀但字面 ⏱0 非法、连发／重复的模式及单位、序列空段／宏存在性和插入手势副作用。GUI／MCP 仍复用现有解析与明确源上下文；本补充只供后续帮助／schema 和等价用例设计，不新增关键链路计数或已支持能力，详能力矩阵。

HELP-66 先交付其中不依赖源写许可的只读帮助：qkm_get_help 的 basic_mapping／original_keys／mapping_keys，经既有 Read 认证查询真实解析器有效键名及现有常量。名称区分大小写，去空／去重后分页，参数及帧大小独立校验；不复制一套 Python 键表或解析器，不接触可变源表。基础帮助的 repeat 为映射项参数，与宏 xN 分开。帮助不证明设备可用、不校验整条映射、不授权编辑；完整语法、设备依赖及未来预览参数随相应能力补齐。双 Qt 组件／SDK及 Qt6 私有产品选定验收见 HELP-20261024-66，不据此宣称运行中帮助、全部语法或完整 M1 已验。

MAP-67 当前候选先覆盖正在显示的源 Tab：两条 GUI 新增入口复用纯分析／提交，GUI 采集 Shift／选区后传明确源行位置；分析只计算互斥／共通禁用补丁，提交前重算状态、目标、加载／结构／运行活动及冲突决策，拒绝在任何模型变更或通知之前完成。复用原键串解析、默认行构造和界面刷新，不新增全局配置模型或引擎计数。内部决策戳仅用于这项 GUI 操作的重新核对，不是完整源／业务修订或 MCP 编辑许可；正式预览仍须绑定实例、配对权限、完整内容修订、到期及可查询的提交结果。源快照、已修改标记和保存范围分别处理，MAP_KEYDATA 相等只用于单条配置语义，不能代替全量修订。非当前源、其他字段及完整功能继续按原目标扩展，尚未开放写工具。

以下只确定最小职责，不引入通用命令框架；具体 C++ 签名随首个实现和测试冻结：

| 共用职责 | 参数／结果及现有提取点 |
|---|---|
| 校验源上下文 | 键串校验族接受明确源表与候选匹配键；GUI 当前表包装可保留，内部不临时替换全局 KeyMappingDataList，不切用户当前 Tab 决定 MCP 目标。宏作用域和引用上下文随实际 DSL 能力接入 |
| 纯插入分析 | 从 prepareMappingDataForInsertion 提取规范候选、互斥／共通冲突和其他表补丁的计算；不写任何源行、不发修改通知、不消费编辑暂存、不弹窗。GUI 收集选择后与 MCP 预览／提交使用相同结果 |
| 插入提交 | 接受核实后的源目标、位置、规范候选及冲突决策；在同一 GUI 线程内重新检查修订和停止清理条件，应用跨表补丁及插入，完成相关刷新／坐标缓存／dirty。GUI 保留选区和物理修饰键采集；MCP 传明确位置 |
| 单项更新／删除 | 更新从 updateMappingInfoByOrder／updateAllMappingInfoFinally 提取；删除从已转换源行的流程提取。明确未指定字段、验证顺序、备注对齐、混合来源限制和编辑会话处理，返回真实结果，不依赖 popup 是否出现 |
| 保存与明确启停 | 保存复用现有固定配置存储及同步／失败恢复，名称／覆盖由输入层决定；启停接收明确目标和保存策略，原有 toggle 入口只采集意图。停止完成仍按下节的实际生命周期边界判定 |

首轮 `EQ-INSERT-PREVIEW` 必须在存在跨表共通冲突时比较所有源数据／Disabled、dirty、有效配置、修改通知和磁盘，覆盖取消及过期提交。不能只测无冲突空表，或只证明预览返回了合理文本。

#### 文件传输与管理员权限（M2-A 实施约束）

外部文件读写优先由**普通权限桥接**承担；C++ 共用业务层处理有界文档内容、既有格式、字段校验和业务应用。导出由共用序列化产生内容，再在普通权限下写入用户指定文件；导入在普通权限下读取并传递有界内容。固定配置保存继续使用现有产品持久化路径。若现有序列化必须借助临时文件，使用服务器生成的受控临时目标并核实生命周期，外部路径不作为高权限任意文件读写参数透传。

该分工保持 GUI／MCP 业务规则共用，并避免低权限客户端借导入／导出获得管理员文件访问。公开能力需说明大小／数量上限、覆盖意图、路径来源与完成状态；不使用提权补齐普通文件权限，不自动访问远程共享。导出写盘错误必须返回失败，真实新进程读回才能证明文件恢复结果。

207 修正宏 GUI 备份遗漏逐段备注和写盘误报，210～212 验证受限正式宏导入。215 修正 GUI 单表同步结果，216／218～220 验证有界单表导出与导入。222／223 验证 GUI 和正式普通配置组导出，224／226／228～230 验证 GUI 失败保护、普通组有界解码、草稿及显式保存。正式 typed 请求拒绝未知字段，旧备份沿声明的兼容规则处理。

#### 明确启停与清理完成

`MappingSwitch` 的现有 toggle 由 GUI 入口采集意图后转为明确目标状态；开始的保存模式不再由 MCP 所在时刻的物理 Ctrl 或按钮下拉状态隐式决定。业务仍复用开始／停止原有必要处理，不能给 MCP 另建无通知的启停分支。

首轮至少暴露三类不同事实：

| 事实 | 含义 |
|---|---|
| 开启监视 | 自动匹配和对应事件／计时状态已启用 |
| 实际映射有效 | 目标条件匹配、实际 Worker 配置已接收并启用；说明生效配置来源 |
| 停止且清理完成 | 停止后的任务、释放、恢复及必要跨线程操作已结束，允许安全修改配置 |

源码已确认：GUI 置 IDLE 早于 Worker 完成；Worker 把 Hook 状态置 STOPPED 前还会发起停止发送，其他任务也可能延后清理。因此仅轮询任一现有状态位不得放行。M1 需在原生命周期上增加最小的完成通知／代次边界，覆盖相关 Hook、Worker、发送任务及适用设备清理；读 GUI 状态、锁住互斥量或固定 sleep 都不能代替完成证明。

等待超时返回 `stop_timeout` 和仍未完成的步骤，保持禁止修改，不自动终止实例、不释放还被借用的资源。停止可能回切配置，清理后须再次核对原变更目标和预览；不适用时重新预览。停止行为的风险分类包含当前配置中实际会执行的“停止映射时发送”，而不是仅看工具名。

以下补充依据源码 `d38cf8dc` 的只读核对，**清理完成接口尚未实现**：

| 现有边界 | 源码事实及对实现的约束 |
|---|---|
| GUI 停止入口 | setKeyUnHook 在 GUI 线程恢复 FilterKeys 基线并排队发送 Hook／Worker 信号；MappingSwitch 随即置 IDLE、允许控件并可能回切配置。GUI 方法返回与业务可安全写入分开记录 |
| Hook 回调 | onSetHookProcKeyUnHook 当前正文只有诊断，不是 UnhookWindowsHookEx 的成功回执。应核对常驻 Hook 的停止旁路和在途生产者，不能把此槽调用当“所有输入回调结束” |
| 停止态输入旁路 | STOP-45 已将 InterceptionKeyboardHookProc 的当前 Tab 透传热键读取移至映射状态短路后，与低层 Hook 对齐。Qt6／Qt5 Release、Qt6 ASan 的 IDLE／CHECKING 缺失 Tab 合成回放均返回正常并保留 DOWN／UP 记账；不证明真实输入线程并发或驱动效果。不能一律丢弃停止态回调 |
| 直接按钮生产者 | STOP-46 将浮动按钮两处调用和面板信号统一经 queueVButtonKey，标记与首次门禁在排队前生效，原 triggerVButtonKey 仍在 Worker 排队执行；退出专用拒收先于 LockState／发送处理。三套各两项组件回放确认两个排队按钮与后续发送的计数连续、迟到事件不修改锁定状态。尚未实现日常停止提交封闭／代次、连发及 GUI 尾部完成协调，不能据此放行 |
| 旧发送任务停止 | stopSendInputTaskController 设置 task_stopping／停止旗标、清暂停并唤醒等待；名称含 ThreadPool 的两种 clear 函数只请求停止。待执行 KEY_DOWN 在 run 的 admission 处退出，KEY_UP 仍须允许完成 |
| 池排空／资源释放 | clearSendInputTaskControllerMap 与全局 reset／clear 才实际 waitForDone，再释放／重置；其等待没有本业务超时，不能在 GUI 线程调用或从超时路径强行删资源。只取空闲线程数也不是排空证明 |
| 停止动作中新任务 | emitSendOnMappingStopKeys 在置 HOOKPROC_STATE_STOPPED 之前发出 DOWN／UP；sendInputKeys_Signal 使用排队连接，onSendInputKeys 可稍后建立新任务。完成条件须包含这些预期任务和它们派生的后续任务 |
| 排队／任务标记 | STOP-44 的共享标记随排队事件保留，真实 SendInputTask 从构造到析构另持标记；STOP-46 将按钮排队也纳入，交接和派生提交无计数空窗。拒收、事件销毁及未运行任务删除会退还。它仅覆盖这些提交及 runnable 存续，不包含全部同步发送、在途输入生产者、设备和 GUI；仍需原池 waitForDone 及其他完成边界 |
| 按键记账与真实释放 | KEY_UP 会先移除 pressedMappingKeysMap、排队 allMappingKeysReleased，再继续逐键发送；空记账表或该信号到达不能单独证明 native 释放及所有 runnable 已结束 |
| 退出专用入口 | shutdownSendInputTasks 把 m_sendInputTasksStopping 永久设 true，该旗标使后续 onSendInputKeys 返回；现有启动路径不复位。该函数保留退出语义，不能直接复用作可再次开始的日常停止 |
| 真实 main 退出 | app.exec 返回后先停止并 join Interception／Hook 线程，再 BlockingQueued 调用 Worker UnHook／shutdownSendInputTasks，最后 join Worker。此时 GUI 事件循环不再处理回执；新增协调不能在该路径等待 GUI，须先取消其观察定时器并保留原排队释放／drain 次序。此前维护用例的正常退出也保留原设备清理，未验证实际设备结果 |
| 配置与界面尾部 | 映射指针恢复、浮窗／面板隐藏等有跨线程回调，GUI 也可能回切配置；写入前再次解析源引用、修订和必要显示／运行缓存，旧停止通知不能完成新一轮操作 |

优先沿用现有停止／释放流程，增加带运行／停止代次的有界异步协调：先停止普通生产者，保留正确 KEY_UP 及已授权停止动作；观察 Worker／相关队列与任务池的最终排空，再确认 GUI 尾部并发完成通知。GUI 保持可响应；只有存在停止作业时才做必要条件观察，不在每个输入事件上增加常驻轮询。确切计数／队列栅栏位置需结合实际提交路径验证，不能只在 setWorkerKeyUnHook 末尾发一个“完成”信号。

STOP-53 接续约束：前置栅栏须在停止正文改写运行缓存／源指针前确认旧输入所有者回调已退出；不能先排队 UnHook，再把后置通知误当作这些改写已受保护。前置阶段不等待全部发送任务自然结束，存量发送仍交由原停止／唤醒／释放流程处理。新停止态回调继续 raw DOWN／UP 记账，但不得重新进入映射数据处理；排队按钮／连发和旧开始／重启请求也需准入守卫。后置阶段包含子队列、真实池排空和 GUI 尾部，总超时由 GUI 独立计时，不能依赖可能被旧阻塞调用占用的 Worker 定时器。

停止动作可以产生有意保留的输出或 GUI 命令，不能用全局按键表清零代替完成，也不能因拒收普通输入而丢弃这些已授权效果。保持其源数据／宏上下文，配置回切须在相关任务不再借用旧配置后执行；SwitchTab 等有意 GUI 效果须纳入尾部核对。以上是待实施约束，不表示当前已解决全部回调／界面并发。

共用开始、编辑／加载入口也须遵守清理未完成的限制；单独给 MCP 加限制不足以避免 GUI 并发修改／快速重启影响旧任务。当前先做源码核对，没有改变这些原有行为。

新增待实施边界用例：旧延迟／暂停任务与排队 KEY_UP；停止动作含等待和派生序列；无按键记账仍有排队 runnable；重复停止不重复停止发送；停止后立即 GUI 重启／改表；配置回切导致目标失效；旧代次完成晚到；超时、断线／取消后仍禁止写入。复用现有 sendinput_lifecycle 目标的真实任务机制，在相关系统／输入授权范围内再做实际效果验证；维护用例或历史结果都不代替新增边界通过。

#### 去重、断线与错误

写请求的 `request_id` 在配对客户端和实例内唯一；相同 ID、相同内容查询或返回原 operation，不重复追加映射、写盘、开始或停止。同 ID 不同内容返回 `request_id_conflict`。结果保留期和有界记录数由能力接口公布；不能通过移除运行中记录满足容量限制。

首次开始／停止也必须去重，不能把“目标状态相同”误当成会重复发送停止动作仍属幂等。客户端超时／断线后先查询原结果；实例 ID 变化、结果过期或在崩溃后不能判定时返回 `outcome_unknown`，不盲目重试。MCP 取消请求不等于业务事务已经撤销。

最小错误集合：`invalid_argument`、`unsupported_capability`、`permission_denied`、`wrong_instance`、`stale_target`、`stale_preview`、`mapping_running`、`cleanup_pending`、`editor_busy`、`conflict_requires_choice`、`save_failed`、`reload_failed`、`stop_timeout`、`request_id_conflict`、`outcome_unknown`。错误包含可执行的下一步，不包含凭据、完整私人映射文本或无限日志。

#### 本机 IPC 与配对门槛

首次默认关闭；明确保存后的启动恢复已按PERSIST-136接通选定范围，完整系统矩阵仍待。读取／配置／运行／观测及高风险权限仍独立，撤销／禁用拒绝新请求而必要清理继续。启动恢复不批准客户端或扩大权限，不开放任意执行／设备屏蔽／驱动安装。

Qt 本地管道组件与 Windows 身份检查共同组成传输层；不只设置一个可预测名称或 `UserAccessOption` 就宣称安全。首个传输切片先验证：

1. 本机和对应登录会话限制、最小 DACL、对端真实进程／Token 用户和登录会话；失败一律拒绝。客户端提供的 PID、用户名、session ID 只作提示，身份须由系统取得。服务端和客户端相互核对实际进程、创建时间、实例随机标识与配对凭据。
2. Qt 6.8.3／5.15.2 实际命名管道实现的远程拒绝能力及可扩展点。若不能可靠使用 `PIPE_REJECT_REMOTE_CLIENTS`，评估最小 Windows 管道适配接入 Qt 事件循环；不打补丁重编 Qt，也不将“本机名字”视为已经拒绝远程客户端。
3. GUI 创建／确认配对，凭据按本机用户保护并独立于可同步的 QKeyMapper INI／Git；支持撤销。不允许 Agent 自报 `confirmed=true` 获得额外能力，不把 token 放命令行、stdout、日志或交接文档。
4. 高权限 QKeyMapper 不给未获对应授权的低权限客户端提供任意执行通道；相同用户／会话校验不能替代能力授权。启动／停止的风险检查也要追踪共通项、宏引用及 `Run(...)` 等实际内容；无法可靠分类的活动配置拒绝该操作，不能按“普通映射工具”放行。
5. 使用有界、明确长度的 UTF-8 消息帧，处理半包／粘包、无效 JSON、过大／过深请求、慢连接和断线；握手前限制更严。初始建议单消息 256 KiB、握手 4 KiB、待认证连接与查询分页均有上限，确切值随测试固定并在能力接口公开。
6. MCP SDK 处理标准 MCP 传输；桥接 stdio 的 stdout 只输出协议消息，诊断走 stderr。IPC 版本与 MCP 协议版本独立；协议不兼容时拒绝，不猜测降级写语义。

Windows 身份与权限实现参考 [命名管道权限](https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-security-and-access-rights)、[真实客户端 PID](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getnamedpipeclientprocessid) 和 [客户端会话](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getnamedpipeclientsessionid)。本轮一次性探针验证了同进程当前会话中的管道创建、Qt 接管及 PID／会话读取；完整鉴权、异用户／会话和产品监听尚未验证，原始证据见 IPC-PROBE-20261022-04。

#### MCP持久使用：PERSIST-134（设计已补；实现／新增验收待分片）

133 a1e265e2后用户确认：**客户端密钥仅Windows凭据管理器＋原DPAPI，服务端保留私有DPAPI pairings.dat，workspace无密钥／密文副本。** 历史结果不变。

134时点源码：[credentials.py](../../mcp_bridge/src/qkeymapper_mcp/credentials.py)的v1目标名／DPAPI熵包含MachineGuid、规范化discovery／可信EXE路径及SessionId；当时[ControlDialog](../../QKeyMapper/automation/qkm_control_dialog.cpp)的applicationBinding为canonical EXE＋SessionId，默认目录和[PairStore](../../QKeyMapper/automation/qkm_pair_store.cpp)的DPAPI绑定都随Session变化。132仅证明同一Windows登录内重连。137-A/B现已分离稳定v2配对上下文与当前Session运行目录，旧显式v1算法保留；真实跨登录／PC重启仍未验。Agent对话、Windows登录、产品运行实例须分别处理；运行期的SID／logon SID／AuthenticationId（含OS验证的双向UAC链接）、Session、PID／创建时间／EXE、管道及认证证明始终复核。

**workspace最小关联。** 优先直接复用客户端已有MCP配置：包内command／固定argv里的可信EXE、明确discovery及133产品记录模式已足以定位当前一个凭据上下文，不新增第二份关联格式或任意CredentialTarget参数。客户端须明确加载／导入经审阅的workspace配置，再把argv交桥接；桥接不假定Agent自动读文件。当前常见workspace入口未配置QKM，真实客户端／gateway加载待验；SDK显式加载样例不能冒充实际Agent新对话联调。首次实现只支持同机／用户／目标上下文的先后会话复用，新对话不重配；不同workspace须明确选择共享同一关联／配对，复制两个配置文件不是权限隔离。确需独立撤销时才另设计独立配对命名空间，首版不加。

关联仅含非秘密启动／目标上下文，不保存动态instance／PID、预览、operation或场景回收记录。缺失／损坏／目标不符／凭据缺失或撤销即明确失败，不枚举凭据／目录换选。文件不授业务权限；客户端须审阅目标变更，桥接仍做全套身份／配对／权限检查，无任意Windows凭据读取工具。若本仓库需要写配置，须有界严格格式、目标路径／重解析及安全原子写，保留无关客户端项与原文件；先无此写入器。配置与系统凭据不是单事务：任一失败均不报配对成功，不自动删共享密钥或重配，实际认证才证明成功。删除配置只移除后续关联，不撤销现有连接；删本机凭据不撤销产品授权／其他已认证连接；产品GUI撤销才撤该配对。无文件删除监控。

**用户保存启用选择。** 首次／无明确保存仍关闭；GUI启用并明确保存后下次产品启动尝试恢复，取消选择并保存则不恢复。复用管理窗开关和最小明确保存入口，作用当前Windows用户＋可信EXE副本，与映射配置／Tab及其保存隔离，Agent配置保存不能隐式提交启用选择。保存意愿、保存结果、实际接口状态分开；初始化／演示配置就绪后只尝试一次，失败保留已保存意愿、实际关闭并给原因，不静默重试／改目标。存储位置及按钮文案于136按现有安全保存模式定稿，三语先审。普通模式按已保存选择恢复监听，演示模式还须固定宿主／安全INI／只配置门禁；不绕UAC、自动启动产品、建服务／计划任务、激活Default或批准新客户端。

136已选验，存储／实际语义及构建／组件／截图见下文；S1–S9已批准，S2／S7／S8真实LF。无新Default／UAC／系统输入；完整产品冷启及Windows／PC重启、客户端接线另列，不由组件结果推导。

**跨Windows登录依赖。** 137-A/B已将版本化稳定存储身份（同机同用户／可信EXE）与Session运行发现／独占租约分开，同步覆盖客户端熵／目标名及产品目录／DPAPI绑定，保持上述实时同登录身份检查。旧入口／凭据不迁移、覆盖或复制；首次v2明确重新GUI配对。短事务／撤销由组件与同登录产品选验，真实跨登录和PC重启仍须另验，不能据稳定命名宣布通过。133动态产品记录只用于当前认证实例，旧引用／结果不作永久连接身份。当前一实例仅一客户端；135的先后复用／占用失败按未变合同复用，不踢掉其他会话、不加代理／管理服务。

**PERSIST-137接续方案（2026-11-10，用户已批准137-A/B及私有验证）。** 在139检查点核对：默认applicationBinding同时决定会话私有目录和服务端DPAPI熵，客户端v1熵还包含显式discovery路径／SessionId；服务端启用期的DirectoryLease锁住同一目录。workspace旧固定instance路径也随Session改变。故不能只删SessionId或将整个独占目录改成共享目录。

最小建议复用现有MCP workspace的command／argv，不新增关联文件或管理服务：增加显式`--persistent-target --executable <可信EXE>`模式，按同一规范化EXE和当前用户本机数据根定位唯一v2目标及本登录Session的instance.json，不枚举候选或扫描凭据。C++／Python使用同一Windows文件句柄规范化路径合同并做跨语言测试；文件身份／实际进程／同登录Token／管道／认证仍完整执行。旧`--discovery-file`及v1凭据算法保持，缺失或失败不自动改模式／换目标。

产品v2根分为稳定的同机同用户／EXE配对库与`sessions/<当前SessionId>`运行发现／独占租约。同EXE同Session仍最多一个启用，不把稳定库锁扩成跨Session终身独占。服务端仍为原DPAPI pairings.dat、客户端仅Windows凭据管理器＋DPAPI，v2命名空间与旧v1隔离；旧文件／密钥不迁移、覆盖、复制或扫描。首次使用v2须一次真实GUI新配对；这项升级行为需明确说明，不能把原v1密钥静默当成新v2。启动启用选择沿136现有独立稳定保存范围，不由普通映射保存改变。

跨Session一致性复用DirectoryLease做稳定库的短事务：配对追加、权限更新、撤销均先锁／重新读最新／验证预期epoch／原子保存，不能用旧内存列表覆盖别人变更。新认证和业务执行前／异步复核读取最新授权；撤销／权限变化或存储忙／损坏则拒绝并结束旧认证，不将旧权限自动升级。无常驻文件监控，不强断其他会话以抢占同一实例；已接受结果仍归原实例，取消／部分效果按原契约。

实施分片：137-A共享库／运行目录、事务／撤销和双Qt独立子进程选验已完成；137-B正式v2目标、固定argv／凭据与默认路径，以及自有私有Qt6普通产品GUI配对、同登录产品／桥接重启、业务／旧引用和拒绝路径已完成选验，详下。真实跨Windows登录／PC重启、Win10和实际Agent／QQ加载仍未执行，须另报授权；组件模拟会话编号不记成真实跨登录。无新驱动／输入／Default动作，无新增Qt文案或TS/QM变化，不将完整137或M1标完成。

### PERSIST-137-A

2026-11-10，PC-W11，选定底层实现／组件完成。AutomationSessionServer增加可选独立pairDirectory；旧构造仍将运行／配对放同目录，正式ControlDialog尚未切v2。运行期独占租约保持，独立共享库只按读取／配对追加／权限修改／撤销持有原DirectoryLease短事务；重新加载最新库，选定pair的epoch／scopes冲突返回Conflict，锁忙返回Busy，无自动重试。v1配对文件格式、DPAPI保护、默认关闭与保存选择不变，不迁移旧凭据。

新认证／证明完成、业务分发、异步执行复核和回复前重核最新授权；外部撤销、库损坏／锁忙关闭当前认证，不升级旧权限。旧connection／request／pair／epoch先比对，再允许存储复核产生影响；迟到旧回调在共享库正忙时也不能误断新连接。两个运行目录共享、同运行目录仍互斥、关闭状态撤销及独立子进程旧快照修改不同配对均验证；同pair旧epoch和真实锁争用不落盘。

Qt6最终IPC构建05、Qt5构建05均native0／零编译链接警告。Qt6私有进程20340的原选验20项与37352新增复核8项分别通过；Qt5私有进程27820整组26项通过，横幅windows10不作为Win10实机证据。实际桌面为各QkmPersist137_*，无产品、UAC、驱动、映射或系统输入。六新增场景在证明完成前、请求前、回复前、异步复核前撤销，以及请求前库损坏／锁忙均拒绝且无后续业务；现有配对、独立窗口权限、关闭管理、存储损坏／写失败和租约边界复用同次选验。服务端文件Clang-Tidy／Clazy五个编译上下文工具exit0／项目诊断0。

失败原件保留：首个普通包装器因PyHANDLE不支持with，在创建测试进程前退出；Qt6 IPC-01为16过／4失败，其中新子进程的CRLF确认行被夹具按LF比较，旧拒绝用例仍把合法ShowWindows=128当未知位。02补诊断及未知位修正为KnownScopes+1，取得子进程确认原字节6c6f616465640d0a；按有界固定行处理后03原20项、04新8项通过。未将夹具失败记成产品故障或推算缺失原生码。

原件仅本机原MCP树out/mcp_persist137（build-ipc-*-01至05、ipc-qt6-01至04／ipc-qt5-01、static-server-01）；测试EXE为out/tests/mcp_ipc/{qt6,qt5}-release/release/QKeyMapperMcpIpcTest.exe。它们是asInvoker Qt Core真实管道／DPAPI／文件锁组件及独立子进程，不是普通Release GUI／跨Windows登录／PC重启验收。A时点正式v2路径尚待B接线，后续结果见下；137完整阶段不标完成。源基线8cc889fb加本片差异，A实耗约一小时。

### PERSIST-137-B

2026-11-10，PC-W11，源基线bd8474d6e62be97a41fd69c8a401ef50d398df45加本片差异；正式v2入口及选定私有验证完成，未push。C++／Python通过Windows文件句柄取得同一规范化可信EXE路径及严格Unicode，派生稳定applicationBinding；服务端私有targets-v2根保存原DPAPI配对库，sessions/<当前SessionId>保存发现／运行独占租约。创建时先验证或建立稳定根及sessions的受保护ACL，既有不安全目录拒绝且不修复；只读取／显示管理窗不创建配对根。不同模拟Session可共享稳定库，同EXE同实际Session仍最多一个启用。

桥接显式`--persistent-target --executable <可信EXE>`复用workspace现有command／argv，精确定位当前用户、目标及当前登录记录；无第二份关联文件、枚举、自动换目标或全局服务。v2客户端CredMgr目标／DPAPI熵稳定绑定同机、当前用户及EXE，独立于Session／动态发现路径；服务端仍用受保护DPAPI库。旧`--discovery-file`及v1算法保留，两模式互斥，不迁移／覆盖／复制旧密钥；首次v2须新GUI配对。正常与演示同EXE使用同一目标配对根；专用演示仍须独立EXE副本／INI，136的普通／演示角色启动选择继续分开，不因此获得新权限或绕过演示守卫。

| 本片检查 | 实际结果与范围 |
|---|---|
| Qt6普通Release | b02连贯重编受ControlDialog布局影响的直接使用者及moc，native0／零警告后部署；b01构建期间接口曾变化，未部署或运行，不能作有效产品证据。最终b03 native0／零警告 |
| 双Qt目录／启动组件 | Qt6 PID8768／Qt5 PID18520各11项通过，真实私有QkmPersist137_*桌面；受保护目录、拒绝既有不安全目录、默认关闭／保存选择及原三语恢复用例。Qt5横幅windows10不作为Win10实机结果 |
| C++／Python派生合同 | Qt6／Qt5各7项无skip；包含真实组件与Unicode目标、0／1／2／最大Session编号的跨语言比较。首次未提供原生EXE的7项中1项skip单列，不倒填为通过；选定Python回归74项通过 |
| 原生与静态 | 两Qt最终IPC各自target-context及demoRecordLifecycle分组各3项通过（均含init／cleanup）；后者仅配置模式组件。控制窗3编译上下文项目0复用，派生文件最终5上下文工具0／项目0；137-A服务端未变结果按原范围复用 |
| 离线桥接组件 | 29项原锁依赖、1614载荷文件，内置CPython 3.11.9 x64以-I／-B导入及v2派生成功；无新依赖。bridge-b01.zip SHA256 c2947097453af03acb7a18454bd4f43bf358b6b5252e1d73e21daf7290eea5e0，不代表完整Release三包／干净系统交付 |
| 真实普通产品与正式MCP | v2-01四阶段first／paired／reused／disabled完成：PID8128／42324／38000／42632均原manifest High、实际自有私有桌面并正常exit0，包内新桥接PID35288／3060／3656为Medium。无选择关闭→真实GUI启用并保存→冷启恢复；一次Read+Configure新v2配对→产品及桥接重启重认证／复用；正式A→B预览／提交／显式保存／冷读；旧源／预览拒绝、旧operation未知；真实GUI关闭保存后冷启关闭 |
| 旧入口／拒绝与清理 | 在末阶段临时启用而不保存，独立GUI新建Read-only v1配对，显式发现入口读取成功、配置保存预览permission_denied。该配对与v2各自撤销／忘记，未复制密钥；结束后独立核对两份专属CredStore为空、四产品退出0、控制器拥有／未定／在途／待处理均0。仅撤出persist137b-qt6授权，原42036缓存留至19:00 |

真实产品位于原MCP树`out/mcp_product/persist137_20261110/runtime-中文/QKeyMapperMcpAcceptance_persist137_qt6.exe`，对应b02 SHA256 7a881460c5aef7ae407f62f2a46c3c0f7d8619d6771b54ef36c418be7dd0340a；合成INI／同机依赖隔离部署，没有复制个人INI／旧凭据或别机out。最终out/build_qt6/release/QKeyMapper.exe为b03，SHA256 6a0d8a14d626c617ffe4e742d79d5d49fdd5ce5f5dbe50fa3a648eae33bee277；b02后的唯一行为相关差异为demo记录发布函数缓存相同正则，普通模式不经过该函数。故四阶段沿用未受影响的b02证据，不称已在b03重跑；最终双Qt demoRecord组件专门覆盖该处，另加UTF-16注释不改变执行。

失败保留：Control组件首轮目录／启动关键5项通过，6项三语用例因包装器遗漏QKM_TEST_TRANSLATIONS_DIR失败，补环境后双Qt各11项通过，无TS/QM修改。严格静态首轮发现原demo记录函数临时QRegularExpression诊断，改同模式static const后项目0；不将原静态轮记成零诊断。没有产品业务失败或未知启动，本片没有重放历史请求。

原件只在本机原MCP树out/mcp_persist137（build-product-b01至b03、build-control／ipc双Qt、control-*-b01／b02、persistent-target-python、bridge-regression-b01、static-b01／b02及bridge-b01）和out/mcp_product/persist137_20261110（deployment、v2-01四轮身份／桌面／GUI／MCP／FilterKeys及scope-release）。这里只证明同一实际Windows登录、同一EXE路径的普通产品／新桥接会话；无RunMapping权限、系统输入、驱动安装、Default切换或全局设置改动。独立GUI业务基线／保存失败／忙碌和Worker／ASan复用未变的136A／135／138原范围，未重复完整矩阵。真实跨Windows登录／PC重启、Win10、实际Agent新对话／workspace自动加载、QQ和新v2演示宿主整轮预热仍待；不能由模拟Session、稳定名字或组件ZIP推导通过。B实耗约两小时，含部署、夹具修正与文档；后续先整理主计划历史重复，实际系统验收另行安排。

| 顺序 | 聚焦实施及验收（状态按各切片） |
|---|---|
| PERSIST-135 | 已选验：现有格式显式SDK加载、同登录先后认证／共享及忙碌拒绝；真实客户端／gateway加载仍待，详下 |
| PERSIST-136 | 已选验：GUI独立保存／组件冷进程恢复与关闭／失败、角色隔离、双Qt／获批三语资源及截图；完整Release与系统矩阵待 |
| PERSIST-137 | A/B稳定上下文、旧入口兼容、共享库事务／撤销及同登录产品已选验；真实跨Windows登录／PC重启仍待 |
| 后续系统验收 | PC重启／登录变化、新UAC／Default另报方法、影响和时长获准后做；Win10／QQ末期另排 |

待验矩阵：首次关闭、启用保存／取消保存／恢复失败；同workspace新会话与跨workspace共享；关联缺失／损坏／目标不符、凭据缺失／写失败／产品撤销；桥接及产品重启、新实例／旧引用失效；用户／机器／Windows会话／路径变化与连接冲突；选定目标有预期业务、其他实例未收到操作、产物无密钥。复用133配置／认证／图像／回收证据，不重复完整验收；未实现／未验证保持待办。

#### PERSIST-135～136A 历史选验压缩索引（全文固定锚点 `8cbf5eb3`）

> `PERSIST-134` 契约与 `PERSIST-137-A/B` 正文保留于上下段落；以下 `135～136A` 历史明细固定保存于 `git show 8cbf5eb3:handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md`（表中本机 `out/` 原件仅存 `PC-W11`，不随 Git 同步）。

| 切片范围 | 核心改动／契约要点 | 选验结论与证据入口（PC-W11） | 遗留边界 |
|---|---|---|---|
| `PERSIST-135` | 复用 `mcpServers`/固定 `command`/`argv` 与 `231` 三秒等待→`pipe_busy`（未认证前不宣称占用者），显式共享配对 | 5 管道＋7 结果＋3 独立桥接 `A→B→A` 认证与 `B` 并发 `pipe_busy` 通过；[135 证据](map67c1_validation.md#persist-135workspace先后复用与明确忙碌2026-10-30pc-w11) | 真实 Agent 自动加载、产品重启、撤权/写失败、跨登录/重启（续 `137`） |
| `PERSIST-136` | 管理窗新增明确保存开关、`startup.json`（canonical EXE＋角色）、`S1–S9` 三语；用户批准豁免 Qt6 `lupdate-pro` 确切 `winextras` 提示 | 双 Qt 28+7 组件（含真实冷进程）、9+3 三语图、QM `1193` 往返一致；[136 证据](map67c1_validation.md#persist-136用户明确保存启动选择2026-10-30pc-w11) | 仅组件冷进程；`136A` 修复并补普通 Release 默认路径实测 |
| `PERSIST-136A` | 修正 `applicationName` 拼入完整 EXE 路径导致保存 `io_error`：改为 `GenericDataLocation/QKeyMapper/MCP/startup-v1/<hash>` | 普通 Qt6 Release 4 阶段（未选关闭→启用保存重启→配对 `Read+Configure` 建例保存→重启复用冷读→关闭保存，8 次退出 0）通过；[136A 证据](map67c1_validation.md#persist-136a真实普通release启动选择与同登录复用2026-11-02pc-w11) | 本轮未检测到 FakerInput 设备（检测到在场时拒启属测试预检规则，非在场实测）；开启 FakerInput 连接运行、跨登录/PC 重启/`137`/Win10/Qt5/QQ |


#### M0.3b 鉴权与传输细化（历史设计，当前实现以顶部为准）

本段原为认证尚未实现时的草案，后续M1及PERSIST结果以本文件当前卡片／相应切片为准；原逐阶段说明保留Git历史，不把历史中间态当成当前缺口。基础[帧](../../QKeyMapper/automation/qkm_ipc_frame.h)／[身份](../../QKeyMapper/automation/qkm_local_identity.h)及[独立测试](../../QKeyMapper/tests/mcp_ipc/README.md)固定4字节大端长度＋无BOM UTF-8对象，4KiB／256KiB、32层边界；先验长再分配，错误不重同步，合法转义NUL／字节长度及非法代理／非有限值规则保持。

未启用→实际对端身份→GUI配对／客户端落地→双向挑战→ready→逐请求权限／业务守卫；发现文件／自报确认不能授权。32字节随机密钥不进INI／Git／输出／日志。HMAC绑定方向／版本／实例／配对／代次／nonce及双方PID／创建时间，64位标识用受限十进制字符串，不依赖JSON顺序。未知／撤销、错误／反射／重放证明均拒绝。运行期身份严格按SID／logon SID／AuthenticationId／Session与PID／创建时间检查；UAC只认OS核验的互指TokenLinkedToken，失败不降级，查询仅TOKEN_QUERY、不改Token。IPC-22当时只验证维护目标，后续产品证据另列。

listener使用logon SID DACL／远程拒绝／首实例及有限连接／缓冲／绝对截止。M1.1b历史上为一个未认证连接、4096B握手／4100B读缓存、3000ms绝对期限，当时首包信号不代表认证；后续配对／业务限额按实际实现。PipeRequestVerifier在专用原生线程核请求Token并恢复，退出后报告；Qt线程不模拟。双Qt普通／匿名及故障配置曾覆盖恢复失败、挂起、迟到与关闭／重启交错；故障注入不是实际OS故障，不由asInvoker或合成SID推导High／跨用户／会话验收。真实跨登录及系统矩阵仍待。当前双边凭据存储／持久边界见PERSIST-134。

#### 首个闭环必备用例

| 用例组 | 必须能发现的失败 |
|---|---|
| API-CONNECT | 默认未启用、未配对、身份／会话不匹配、错误实例、撤销、版本错误、坏帧和超限；不能在验证失败后仍执行命令 |
| EQ-MAP | A→B 新增、修改、删除；GUI／正式 MCP 的源数据、默认值、Pure 字段、禁用／冲突、dirty、独立保存恢复和实际按下／释放一致 |
| API-PREVIEW | 预览不修改任何业务或持久状态；GUI 并发改值／重排／切配置／重载、过期预览和陈旧引用均被拒绝 |
| EQ-CFG-SAVE | 显式保存、取消覆盖、不可写路径、写盘成功但重载失败；旧草稿保护和真正冷启动；不能用同进程缓存冒充落盘 |
| EQ-RUN | 重复开始／停止、监视但未匹配、停止释放、停止发送、停止超时、回切配置；清理前和超时后都禁止修改 |
| EQ-OPSTATE | GUI／正式 MCP 运行中均允许即时移动／缩放窗口，均拒绝要求空闲的配置及映射增删改；覆盖 STOPPING、用户同意／拒绝／取消、停止失败／超时／迟到完成、预览后状态变化和批量拒绝零部分修改；对应矩阵 EQ-OPSTATE-01～04 |
| API-RETRY | 接受前／提交后断线、重连查结果、同 ID 相同／不同内容、实例重启及结果未知；新增／保存／停止不重复执行 |
| API-CONTEXT | 共通显示、筛选和非当前源表、已打开有暂存的编辑窗；不依赖当前焦点、Ctrl／Shift 或显示行号 |

状态和配置测试优先私有桌面；Default 首轮仍限原计划的代表性 GUI 操作、受控 A→B 按下／释放及实际停止快捷键，每台 10–15 分钟。后续 EQ-OPSTATE 的物理移动／缩放可合并上述 TARGET-07 桌面时段，避免重复占用；纯显示能力按 M2-B 接入，M1 先落实共用状态守卫与停止后提交保护。新增接口的安全帧／身份测试先在不含产品 Hook 和真实输入的隔离测试目标验证，再进入产品集成；不为此假定真实输入等价性已经通过。

## 2. 将 GUI／MCP 操作等价性设为硬门槛

**同样的前置条件和操作意图，必须产生相同业务状态、持久化结果和实际执行效果。仅控件显示一致不算通过。**

当前源码的具体风险例子：缩放选择通过 `QComboBox::activated` 进入 `onScaleComboBoxActivated()`，仅调用 `setCurrentIndex()` 不会执行完整缩放逻辑。详见 [qkeymapper.cpp](../../QKeyMapper/qkeymapper.cpp)，相关连接、`initScaleMenu()` 及实际处理函数。

### 共用实现

接入每项功能前追踪：

**用户输入事件 → 槽函数／动作 → 校验 → 主数据及派生状态 → 缓存与通知 → 运行效果 → 保存／恢复。**

GUI 与 MCP 分别采集输入，之后汇入同一个接受明确参数的业务操作。现有函数可完整承担职责时直接复用，否则只提取该功能必要逻辑，不进行全局架构重写。

共用操作包含校验、状态守卫、冲突与联动规则、实际数据、派生字段、有效配置、缓存、Worker 通知、界面刷新、未保存标记、既有持久化语义，以及正确执行顺序、完成条件和失败处理。

每项操作在能力矩阵明确 GUI 入口、允许状态、共用校验、实际副作用、持久化语义及独立用例；未核实前不按模块名称推定允许状态，也不声明支持。GUI 控件禁用不能成为唯一守卫；GUI／MCP 在实际提交前复用业务状态检查，拒绝不得产生部分修改。即时窗口移动／缩放与保存窗口位置／尺寸等设置分开记录；前者复用完整几何、尺寸基线、布局及既有保存行为，不能只设置窗口属性后回显成功。

禁止以“设置控件值后返回成功”完成接入，也不能为 MCP 复制业务逻辑。避免控件信号与业务提交重复执行；信号屏蔽仅用于必要界面回显，不能跳过处理。初始化／加载与用户编辑路径的差异必须明确保留。

如果发现原有 GUI 行为本身存在问题，单独记录并按已授权范围处理，不能通过另一套 MCP 逻辑悄悄改变语义。

### 独立对照验证

1. **GUI 基线**：通过界面事件完成真实用户操作流程。
2. **MCP 路径**：通过正式 MCP 请求完成相同语义操作。

两条路径从等价的隔离配置与状态开始，分别比较：

| 层次 | 必查结果 |
|---|---|
| 主数据 | 实际映射、宏、设置值、目标配置及来源 Tab／行 |
| 派生状态 | 实际被使用的字段、缓存、解析结果、Worker 接收状态 |
| 操作结果 | 冲突联动、启用／禁用、必要处理及通知、异步完成条件 |
| 持久化 | 未保存状态、实际写盘内容、冷启动恢复结果 |
| 最终效果 | 受控输入输出、实际缩放与布局、相关系统行为 |

断言同时检查业务预期，避免两条路径共同出错却因结果相同而通过。只排除有理由的非业务差异，例如动态时间戳或操作来源元数据，并记录理由。

GUI 基线不得调用 MCP 或直接调用共用业务函数替代界面操作。批量用例可用 Qt 界面事件驱动，代表场景再通过真实系统输入复核。缩放回归须检查实际有效比例及显示结果，使“只改索引、遗漏激活处理”的退化会被发现。

状态对照按矩阵 EQ-OPSTATE-01～04 覆盖运行中允许的即时显示操作、要求空闲的编辑拒绝及用户选择／停止清理保护。显示调整比较实际布局、正常窗口尺寸基线和既有持久化结果，不得写入映射内容或引擎配置；最终效果对照固定焦点与匹配条件。另记录显示／激活本身引起的既有窗口匹配变化，保持两条路径的真实 GUI 语义，不将焦点副作用冒充配置修改或承诺所有显示动作绝无运行影响。

**任何声明支持的能力，等价性失败或缺少必要证据时不得标记完成。** 接入功能、修改处理链路或修复问题时维护对应回归用例。

### STOP-56 来源并发边界盘点（历史基线，当前只读增补见 STOP-57）

| 来源 | 已核对的处理边界 | 后续放行要求 |
|---|---|---|
| Source Snapshot | GUI 线程在 SourceAccess 互斥内复制实际源，仍要求首次静止 | 新只读许可绑定已完成清理、当前活动修订、有效加载上下文，不能只看零计数 |
| 发送与序列控制 | Unlock 等源写入在同步入口；包装也可直接执行控制而不构造任务 | 同步／包装／每次任务构造均在相关处理前记录修订，并与复制互斥；不得因历史已经为真跳过 |
| Hook／手柄锁／虚拟按钮 | 普通 Hook 的状态／准入、仅生命周期启动的手柄捕获、按钮排队及派发守卫 | 保留 raw DOWN／UP；运行中／停止中不读可变源，必须先完成旧所有者和发送链路 |
| 生命周期清理／源恢复 | 连发清理与 Worker→源表写回发生在原开始／停止／重启路径 | 槽返回、STOPPED 或计数零不是独立完成证明，沿用前后所有者、池及 GUI 尾部 |
| 晚到自定义标签／派生序列 | 仍可能读取行元数据或依赖当前全局源上下文 | 只读验证不能授予删改源表权限；写接入前须解决对应上下文与真实输入证据 |

活动修订是每次准入观察，不是作业 ID、持久化版本或业务修订；同一逻辑作业可以多次增加。达到上限保持饱和，后续许可拒绝该值。表中记录 STOP-56 当时限制；STOP-57 只增加当前完整清理后的读取，不赋予修改源表的安全性。原同步锁的已测普通键盘性能范围见 ENGINE-61／EXIT-63，未测系统及设备范围仍保留。

## 3. 作业阶段与时间预估

排期假设：**GPT-6.1 Sol、Max、单 Agent 串行执行**，本机已有可用开发环境。两机执行前核对模型及档位；文档不代表已修改应用模型设置。Max 不代表固定开发速度，以下是本项目工程估算，不是官方基准。

有效作业时间含阅读、实现、编译、调查和选定验证；不含人工等待、额度等待、首次工具链安装及等待硬件。阶段结束记录实耗并更新剩余估算。

| 阶段 | 工作与验收切点 | 最近校准的剩余有效作业时间（STOP-53 基线＋DESIGN-58 新范围） | 当前状态 |
|---|---|---:|---|
| M0：契约、等价性盘点和交接基础 | 迁移计划；能力矩阵与完整 GUI 链路；接口、安全和完成语义；两机环境核对 | 4–6 小时 | 进行中；M0.1 完成，M0.2 首轮入口和关键链路已记录，M0.3 部分完成 |
| M1：首个共用业务闭环 | 桥接可靠性、公共引擎接入门槛、安全读取、普通映射增删改、预览提交、显式保存、冷启动、明确启停及等待；多实例会话及整窗提示 | 20–36 小时 | 进行中；Rust 读取、普通编辑／保存／冷读及受限启停已选验。普通 Release 活动态扩展、系统／设备、多实例及整窗提示继续跟踪 |
| M2-A：映射、Tab 与宏 | 完整映射参数、排序／分类、共通、普通宏／通用宏及导入导出，逐项等价性 | 10–18 小时 | 进行中。排序162～167、Tab及行属性168～194、触发195～196、复制197～201、宏202～212、单表传输215／216／218～220、普通组传输222～224／226／228～230选验。扩展 DSL、活动态及其它配置范围另验 |
| M2-B：软件设置与界面条件 | 配置切换、窗口匹配、全部全局／高级设置、悬浮与显示、必要界面会话；核对联动、缓存和生效时机 | 12–22 小时 | 进行中。行插入、编辑触发、窗口显示匹配、窗口尺寸与分割条记忆、通用与映射高级设置、忽略规则、路径恢复、缩放、陀螺仪选项与 8 项参数、面板／双通知／托盘／悬浮窗／准星／页签自定义图片／浮动按钮与同步组／主窗口通用偏好与行为／启停热键与配置说明 213／214／217／221／225／227／232／234～257／261～272（含 `257～266` r1～r3 修复 `d2c742aa`／`5db982a9`／`9560d278`）选验；实际视觉绘制、拖动、悬浮／穿透热键与剩余对话框仍待验 |
| M3-A：深入观测与复现验证 | 诊断事件、真实输入观测、相关设备操作、授权与异常清理；现有生命周期及崩溃／卡死诊断衔接 | 8–14 小时 | 进行中。手柄触发器／摇杆阈值 258、触摸板速度／反转 259 与接受虚拟手柄输入 260 的配置子项已选验；真实物理输入、虚拟手柄反馈、诊断事件、异常清理与设备观测仍待验（保留全阶段剩余门槛） |
| M3-B：双系统集成与交付 | Win10／Win11 验收、相关 Qt5 兼容、性能对照、打包、说明及完整能力覆盖 | 6–10 小时 | x64 新布局夹具通过；Win10 正式构建与三包待验，Antigravity 选验见 AGY-160 |
| DEMO：专用演示与截图会话 | P0可行性→正式窗口／捕获→串行A→B及重置→预置Release预热／免Python→末期Win10 QQ联调 | 后续场景／交付待校准，未计入旧合计 | 进行中；P0／P1／P2及P3预热已选验，扩展／Win10／QQ／完整交付待，PC-W11无gateway环境 |
| 剩余合计 | 原范围未完成部分＋新增DEMO待校准；Rust迁移另计 | **原60–106小时＋DEMO新增待估** | 未完成，不含已投入未可靠分账工时；新增部分先P0后分账，复用不重复计时 |

- 历史估算为初始 48–84 小时、阶段总量校准 56–94 小时（预留后 70–120）、STOP-53 剩余 56–98 小时（预留后 70–125）；均不是实耗。上表现在明确表示**剩余**工作：在 STOP-53 基线上，仅为 DESIGN-58 整窗提示实现及新增针对性验收增加 4–8 小时，纳入 M1 后为 20–36 小时；其他阶段沿用原范围，合计 60–106 小时，约 25% 预留并取整后按 **原范围75–135小时剩余有效作业时间**考虑（不含DEMO新增待估）。已有状态守卫、显示／会话和双系统测试复用，不重复计时；未依据未分账投入扣减工时。首个可用闭环的剩余工作主要在 M1，不承诺某个日历日期。
- 本次校准依据已确认的同用户 UAC／工具兼容、源身份、两所有者排队、在途回调、定时器残留和池／GUI 完成边界复杂度。组件通过尚未替代 GUI／MCP 操作等价、持久化／冷启动及真正输入效果验证；目标和验收范围不缩减。后续按实际切片继续校准，缺少分账的历史投入不倒算成已完成比例。
- PC-W11 首次补齐环境另预留约 1–3 小时；正常换机接续每次约 15–30 分钟。
- 每阶段拆为通常约 1–3 小时的可验证切片。切片结束更新进度，不要求每片新建聊天或换机。
- M0 完成后按实际能力数量与复杂度校准。超出区间时更新原因和剩余预算，不以省略验证压缩工期。
- 本轮文档切片不代表 M0 完整盘点、接口契约或两机环境核对已完成；历史测试不计作新增能力验收。

### 切片与初始任务

| ID | 作业 | 状态／完成标准 |
|---|---|---|
| M0.1 | 文档迁移、目录规则、导航、矩阵与验证记录初始化 | 完成；四份文档、导航、旧正文迁移及 DOC-01～06 检查通过，未提交／推送 |
| M0.2 | 全量用户能力及处理链路盘点 | 进行中；387 个候选入口、38 个动态动作族／入口行；M0.2b 三批补充后共 68 条静态关键链路、63 个已关联 UI 入口。其余模块继续逐项完成，不将入口或静态链路计为已支持能力 |
| M0.2a | 全部根 UI 定义及首轮动态控件／菜单定位，追踪首闭环和代表性高风险链路 | 完成；SRC-20261022-02 及 DOC-20261022-05 通过，只表示本子切片完成 |
| M0.2b | 补齐能力角色、调用者、动态动作、所有参数约束与实际有效／保存链路 | 进行中；Tab／插入／宏 14 条见 SRC-24；通用／映射高级 21 条见 SRC-28；配置／窗口匹配／忽略规则 17 条见 SRC-29。参数、保存／生效及拟定用例分项记录；剩余显示、设备、输入编辑等继续核对，尚无业务运行等价性结果 |
| M0.3 | 首个闭环契约、安全边界及两机环境记录 | 进行中；通信／配对、只读 schema 及选定双机哈希环境／接续已验；完整事务修订、风险、写入／运行及后续设备契约仍随接入补齐 |
| M0.3a | v0.1 设计、本机基础依赖及原生管道／Qt 接管可行性 | 完成当前本机子切片；ENV-20261022-03／IPC-PROBE-20261022-04；不代表完整接口、安全或跨机验收 |
| M0.3b | 双方身份／配对与权限协议、负向用例、首轮共用操作签名及具体实现入口 | 进行中；鉴权状态／证明／存储／撤销草案已补；普通映射共用操作签名及具体业务调用者仍须在 M1.2 前核对 |
| M0.3c | 跨机 Python 版本／开发依赖约束和环境恢复 | ENV-39／CACHE-40：安装文件身份、31项wheel哈希锁及PC-W11独立venv选定接续完成，PC-W10按原规则核对；PKG-123另固定3.11.9嵌入归档／标准库构建及原运行wheel，无第三方打包器。完整M3-B／M0不标完成 |
| M1.1a | 有界帧、真实对端进程主 Token 身份及维护测试 | 完成；IPC-20261022-07，两套 Qt 均 30 项通过；仅内部组件，不对外声明 MCP 能力 |
| M1.1b | Windows listener、Qt 接管、认证前限制及异常关闭 | 完成；IPC-20261022-11，两套 Qt 均 54 项通过；无产品监听或业务授权 |
| M1.1c | 配对、挑战证明、撤销及 GUI 权限 | 完成选定通信／权限切片；IPC-31 两套实际管理员产品及普通 SDK 六分项通过；托盘／异用户／Win11 等未测范围仍保留，不等于业务完成 |
| M1.1c-1 | 隔离线程的请求 Token 检查基础组件 | 完成；IPC-20261022-12，后续 c-2 已集成；不等于完整鉴权 |
| M1.1c-2 | 请求上下文检查集成与异常恢复 | 完成；IPC-20261022-13，普通 59／59、故障 64；真实 OS 故障未制造，配对／业务继续关闭 |
| M1.1c-3 | 双向挑战证明基础组件 | 完成；IPC-20261022-14，独立向量与普通 66／66、故障 71；未接网络证明消息或凭据 |
| M1.1c-4 | 配对凭据、证明收发、GUI 授权及撤销 | 完成选定切片；a／b／c／d／e 组件与实际产品按对应记录验收，业务未开放 |
| M1.1c-4a | DPAPI 配对凭据与私有原子存储 | 完成；IPC-20261022-15，普通 71／71、故障 77；非提权运行，尚未接控制器 |
| M1.1c-4b | 分阶段收发、认证／配对与权限控制器 | 完成；IPC-20261022-16／17，普通 80／80、故障及 ASan 各 86；无产品 GUI 或业务 |
| M1.1c-4c | 产品管理窗口与可移植私有桌面验证 | 完成选定组件切片；Qt6／Qt5 各 11 项、三语言静态图、产品构建及定向分析通过；实际产品入口／高低权限仍未测，见 IPC-18 |
| M1.1c-4d | 私有实例发现与关闭清理 | 完成选定切片；IPC-19，正式 SDK 后续 IPC-20 已通过；高低权限仍须产品验收 |
| M1.1c-4e | 关闭状态的配对查看／撤销及实际权限验收 | 完成选定切片；e-a 组件、e-b 两套管理员产品／普通桥接通过，见 IPC-21／22／31 |
| M1.1c-4e-a | 离线配对管理与同目录独占租约 | 完成选定组件切片；IPC-21，IPC 87／87／93、GUI 16／16、SDK 两套各 12 短场景、定向 ASan 12、三语言新状态及定向分析通过 |
| M1.1c-4e-b | 管理员服务与普通桥接、真正产品入口／权限 | 完成选定功能切片；IPC-22 High 维护各 13、IPC-23 GUI 各 18；IPC-31 两套正式产品各 6 分项含冷启动／撤销持久化通过。普通截图未验、SDK 首次维护退出未重现等限制见该记录 |
| M1.1d | 官方 Python MCP SDK 桥接与跨语言互操作 | 完成选定通信组件切片；IPC-20：SDK initialize／list／正式工具往返、真实 GUI 批准／撤销／60 秒超时、取消及失败通过；无产品业务 |
| TARGET-DESIGN-47 | 普通用户多实例选择、可视确认、会话绑定与主动切换设计 | DESIGN-58 增补整窗黄色闪烁框；候选枚举／导入引导、独立显示权限、提示／窗口观测、绑定／切换均待后续 M1 连接／会话切片实现及 TARGET-01～07 验证，免 Python 交付仍归 M3-B；不替代 2b-3 |
| DESIGN-58 | 逐项 GUI／MCP 状态规则与待绑定整窗提示文档补充 | 本轮仅更新三份文档、保留 STOP-57 接续点；新增能力及 EQ-OPSTATE-01～04／TARGET-07 均待实现／待验证，不改变 STOP-56 已验范围 |
| BRIDGE-59／60 | R1 取消恢复与 R2 MCP 错误语义 | 按 REVIEW-59 优先实施并独立本地提交；未改 C++ 引擎，具体当前证据见顶部卡片及验证记录 |
| ENGINE-61 | 公共引擎原功能／性能门槛 | A／B1 及 B2 三类已验，EXIT-63 已修复并验选定退出场景；设备／系统／唤醒等继续保留，后续按实际接入风险补验 |
| QUERY-62 | 源查询分步优化 | A 纯参数前置、B 一次完整编码复用和助手基准完成选定验收；产品全量采集／锁内编码待后续测量优化，先推进业务闭环，不弱化引用或门禁 |
| HELP-66 | 首闭环只读语法／键名帮助 | 完成选定停止态验收；八工具／help.read，双 Qt 组件／SDK及 Qt6 私有产品 12／源查询 10 场景通过。不授编辑权限，完整语法／设备依赖随对应能力扩展 |
| WRITE-64 | 首个源编辑前的上下文安全 | A 为 7b188d5c；B 三阶段／双 Qt／ASan 与受限真实标签、四原功能回归通过，独立公共修复提交切点。性能／设备／其他写安全门槛保留，不授 MCP 编辑权限 |
| MAP-67 | 普通新增共用操作及首闭环预览／提交 | A为755aee4a、B0为cac101be；B1正式接线及基本内存添加选定验收完成，详见MAP-20261025-67-B1。接续C1保存；完整增改删／保存冷启动／MCP启停闭环未完成 |
| M1.2 及以后 | 查询、普通映射共用操作、预览提交、保存和明确启停／等待；后续全能力阶段 | 进行中；a-1 摘要及 a-2a 目录选定读流已验，完整闭环与全部能力目标不变 |
| M1.2a | 首批业务只读状态／源数据查询及正式 SDK | 进行中；a-1 选定停止态验证完成，a-2 源数据查询接续；不能把 IDLE／STOPPED 当作清理完成 |
| M1.2a-1 | 首批状态摘要及固定正式 MCP 工具 | 完成选定停止态子切片；STATE-34 两套产品各 8 分项。其他配置／运行／加载／超大摘要边界仍须在相应切片补验，不表示完整业务等价性或 M1 完成 |
| M1.2a-2a | 配置／Tab 目录与上下文修订 | 完成选定停止态读流；CAT-36 两套产品各 10 分项，GUI 实际目录变更／大结果等边界后续补验；目录引用不是源行／编辑引用 |
| M1.2a-2b | 实际源身份、映射行快照及稳定引用 | 进行中；2b-1 加载记录与 2b-2 初始静止只读源快照完成选定范围，稳定编辑授权、运行后读取及 Worker 写回／清理协调仍未闭环 |
| M1.2a-2b-1 | 最近映射加载记录、失败／缺失解析及初始目录上下文 | LOAD-37 首批启动六组各 11 项、LOAD-41 两套独立 GUI 七步及正式 MCP 各 18 项通过；温切换／空白重置、旧目录失效和只读不变性完成选定范围，不表示 2b 或 M1 完成 |
| M1.2a-2b-2 | 实际普通／共通源快照及只读引用 | SOURCE-42：PC-W11 两套最终产品各 9 项、源维护各 3 组、协议 15／SDK 各 12 通过；首次活动门禁只保护初始静止读取，引用不是稳定编辑授权 |
| M1.2a-2b-3／STOP-57 | 真实生命周期完成边界及运行后源读取 | 完成选定只读范围：历史 NoOp 边界保留，STOP-57 双 Qt 组件／构建及 Win10 Qt6 正式源／真实启停回切后只读通过。当前完整清理许可可用，不授写权限或 formal operation 语义；其他系统／设备及后续写入安全仍按门槛推进 |

## 4. Git 跟踪的交接目录与管理规则

遵循 [handoff 目录规则](../README.md)：

```text
handoff/
├─ README.md
└─ qkeymapper_mcp_control/
   ├─ qkeymapper_mcp_automation_plan_codex.md
   ├─ capability_matrix.md
   ├─ validation.md
   └─ map67b1_validation.md
```

本文件是唯一进度来源；能力表记录覆盖及等价性，验证记录按源码、配置和机器保存精简结果。默认只放 Markdown，单文件建议不超过 256 KiB，单任务目录建议不超过 2 MiB。原始日志、图像、二进制、转储、缓存及个人数据不进入本目录。

用户明确要求保留未验证接线时，允许在 unverified_patches/<阶段ID>/ 保存纯文本 .patch 及来源／使用说明；这是默认 Markdown 规则的限定例外。补丁不参与构建、不自动应用、不证明功能完成；写明生成基准和依赖检查点，确认不再需要后连同主计划引用一起删除。仍遵守大小和敏感材料限制。

主计划顶部持续记录阶段／切片、统一状态、执行机器、代码提交及相关未提交变更、当前验证、未验证项、授权边界、本机独有材料、下一项明确动作及完成标准、实耗及剩余估算。可验证切片结束、重要失败、决策变化及交接前更新。任务进度不能只留在聊天或本机个人记忆中。

使用任务分支 `codex/qkeymapper-mcp-control`，同一时间只由一台 PC 写入。

### 源 PC

1. 完成或明确暂停切片，释放测试输入并正常关闭自有测试进程。
2. 更新计划、能力表、必要验证记录及可审阅差异。
3. 本任务用户已明确授权 Agent 自主选择合适检查点，形成聚焦的本地 commit；不提交无关文件。后续如授权改变，以用户最新指示为准。
4. 默认由用户手工 push；用户明确授权时，Agent 仅推送获授权的任务分支并核实远端 SHA，提供接续分支与检查点。2026-10-23 用户已授权本次推送 `codex/qkeymapper-mcp-control`；不合并主分支或强制推送。

### 接收 PC

1. 保留已有本地改动，核对分支及检查点，仅允许可快进同步；分叉、冲突或不明变更先核实，不重置覆盖。
2. 读取仓库 `AGENTS.md`、目录规则、本文件顶部卡片及相关验证记录，以当前文件和 Git 状态为准。
3. 探测本机依赖、驱动、设备、桌面及权限，按下节统一 Python 基线重新创建／校验本机 venv，使用当前源码构建所需目标；不复用另一台机器的 venv、构建目录或配对凭据。
4. 运行接续所需最小检查和当前切片相关用例，再执行卡片指定的下一项动作；可复用证据注明原版本与范围。
5. 将执行机器及新结果写回本文件。Win10 通过不代表 Win11 系统行为通过；两机首次接续及最终集成阶段均验证交接流程。

可在另一台 PC 新建本地聊天恢复，无需复制 Codex 数据库或整段聊天。即使使用原生跨主机交接，也执行相同仓库／环境核对。意外中断后先检查已保存工作及操作结果，避免重复副作用；未保存、未提交或未推送的内容不能假定已同步。

### Python 开发环境与跨机一致性

当前统一基线为 **Windows x64 CPython 3.11.9**，包含补丁版本与架构；以 [环境校验入口](../../mcp_bridge/check_environment.py) 为机器可执行约束。现有 [运行依赖锁](../../mcp_bridge/requirements-windows.txt) 固定全部 29 项直接／传递依赖；新增 [开发依赖锁](../../mcp_bridge/requirements-dev-windows.txt) 固定 pip 24.0、setuptools 65.5.0。这组版本来源于本机已验证环境，不是对最新版的推荐，也不永久冻结后续升级。

- 每台 PC 从明确的可信解释器路径，在仓库 out/mcp_bridge/venv 内独立创建环境；使用 venv 内解释器运行 Python SDK 客户端和维护夹具。不能仅以 `python`、`py -3.11` 或“已激活”推断实际版本；不使用 system-site-packages，不复制 venv 或个人凭据。路径允许不同，版本及依赖约束相同。
- 安装命令、版本校验及 pip check 见 [桥接说明](../../mcp_bridge/README.md)。使用完整锁、--no-deps 与匹配 Windows x64 的二进制 wheel；校验缺失／额外／重复包、版本、解释器、架构及隔离状态，任何失败都先修复环境再验收。记录实际版本和选定测试结果，不能只凭安装命令成功宣称一致。
- **首次接收 PC 的环境准备阶段**完成可信 Python 安装文件身份校验、Windows x64 wheel 文件 SHA-256 锁及实际 hash-checking 安装后再恢复业务验收。ENV-39 已在 PC-W11 落实；锁及命令见桥接说明。安装文件／包和原始证据留在 Git 外，不能仅版本相同就宣称包文件一致；已有解释器的版本／签名校验也不追溯证明其全部文件来自锁定安装包。
- **M3-B 打包**：正式 x64 包使用已核验源码、依赖锁及归档摘要的 Rust 单 EXE。Win10 x64 干净离线组件已验；完整 Release 仍按顶部交付项验收。PKG-123 旧组件和原 wheel 锁保留历史记录。
- 升级解释器、SDK、依赖或打包工具时，在同一 Git 检查点更新相关锁、校验入口和必要回归证据，两机随后重建；不同基线结果分开记录。首次换机须至少通过环境检查、pip check、环境／协议用例和当前切片所需正式 SDK／产品用例。
- Python 锁用于测试及打包。Rust 与 Qt 产品按各自构建锁和目标架构核对。环境一致性减少变量，不能承诺跨系统结果必然相同；业务等价性与系统相关最终效果仍分别验收。

### 环境与证据边界

| 机器 | 已知信息 | 待核验／验收 |
|---|---|---|
| PC-W10 | Windows 10 19045 AMD64；产品 Qt6.8.3／Qt5.15.2、MSVC BuildTools 14.40 x64；桥接独立 venv 为 CPython 3.11.9 x64、pip 24.0／setuptools 65.5.0、29 项固定运行依赖，mcp 2.3.0／pywin32 312。已有普通构建、SDK／产品的选定验证，具体以逐项记录为准 | 不修改全局 Python。安装文件／wheel 哈希锁、打包工具锁及正式打包未完成；驱动／设备、实际桌面及输入条件仍按切片分别验收 |
| PC-W11 | Win11 企业版 10.0.26200 x64；b3c6f277 已同步。CPython 3.11.9 x64／31 项固定依赖实际哈希安装、环境检查及两套 Qt／SDK／选定会话／正式产品接续通过，见 ENV-39；PID 缓存对照见 CACHE-40 | 加载迁移、源行身份、写入／保存、真实停止完成、设备与实际输入等按切片分别验收；High 截图、打包工具锁／交付未完成 |

不同盘符和工具路径通过仓库相对路径及参数处理。测试实例绑定绝对 EXE、PID、启动时间、构建及独立端点，使用独立运行目录／INI；当前产品会切换到 EXE 目录，因此只改变工作目录不足以隔离。配置隔离也不代表系统 Hook／驱动隔离，真实输入及驱动测试串行进行并核对其他实例。

连续固定动作沿用用户明确批准的 gsudo PID 缓存边界，见 CACHE-40；日常查看／临时窗口优先使用[工作区缓存技能](../../.agents/skills/qkeymapper-gsudo-cache/SKILL.md)，当前完整验收边界见 CACHE-SKILL-69。CACHE72后，正式产品／源查询／输入整套脚本省略旧`--gsudo-cache`，经受管run及公共Start／Close／固定GUI请求复用缓存所属普通控制器的实际调用链；旧选项明确拒绝，不再另建或嵌套缓存。新同类目标／用例经精确内容、参数、助手、桌面和预算定义及独立已有授权摘要登记；实际进程拥有关系和清理仍逐项核对。默认五分钟、最长24小时及原单调硬截止由技能管理，off先停止新派发并在原截止内清理，最后关闭本PID缓存并核实；不使用 PID 0、无限期缓存、全局 `-k` 或修改 CacheMode。缓存不提升开发、构建、SDK、桥接或截图进程；依然核对产品实际完整性、系统登录关联、EXE 身份及私有桌面。缓存与无缓存的业务验收结果可在已核实范围对照，UAC 提示／拒绝、焦点切换及等待耗时不能当作相同覆盖；此类 UAC 交互场景另行不使用缓存验收。

技能管理模式另外设置单调时钟硬截止：status不创建或续期，时间指令可延长／缩短，restore当前剩余5分钟，上限24小时。复用同一普通父控制器时，子脚本省略 --gsudo-cache，避免嵌套PID缓存；范围默认空，按既有授权另行登记。原直接脚本模式仍保留上段五分钟空闲及finally清理语义；本轮未把两种调度方式或三个固定入口记为业务等价通过。

快照区分内存草稿与已保存配置，恢复仅限本轮拥有的实例和配置；系统及驱动副作用单独处理。测试使用 `finally` 清理，异常保留必要证据，不自动恢复映射运行。

## 5. 验收、证据与本次实施范围

新增能力至少验证：GUI／MCP 业务等价性；运行中限制、取消、停止完成；过期预览、保存失败、重连和重复请求；非当前 Tab、筛选、共通、宏引用、打开编辑窗与 GUI 并发修改；内存／磁盘及冷启动恢复；风险授权、停止入口、清理及隐私保护。

复用现有 [测试说明](../../QKeyMapper/tests/README.md)、[进程管理](../../scripts/test_process.ps1)、[截图脚本](../../scripts/capture_ui_snapshots.ps1)、SendInput 生命周期测试和现有诊断。维护测试位于 `QKeyMapper/tests/<主题>/`，桥接测试与桥接程序相邻，实际运行副本与证据在忽略的 `out/`。回归脚本使用正式 MCP 服务，不另建测试语言；编译、Git 和静态分析继续使用原工具，不通过 MCP 新增通用终端执行。

| 证据 | 可证明的范围 |
|---|---|
| 配置及有效状态读取 | 条件已布置、产品报告状态符合预期；不能仅回显请求值 |
| 引擎事件或执行轨迹 | 相应产品路径与步骤实际执行 |
| 受控接收窗口记录 | 该输入路径在受控环境产生对应效果 |
| GUI 截图及独立交互 | 界面显示或指定交互符合预期 |
| 实际设备及目标程序测试 | 对应硬件、驱动和目标环境下行为 |

内部派发成功不能代替目标程序接收成功；引擎直调不能代替物理输入链路；配置检查不能代替焦点、拖动或 DPI 验收。卡死／崩溃时依赖外部诊断保留证据，不能假设 GUI 内的 MCP 服务仍可回应。

配置和静态界面验证优先使用私有桌面，由截图脚本采集并自主审阅。未来首轮 Default 场景包含代表性 GUI 添加／设置、受控 A→B 输入与释放及实际停止快捷键，每台 PC 预计 10–15 分钟，临时占用焦点和键鼠，结束释放并关闭自有窗口；首轮不安装驱动。真实左键改键、跨屏拖动、托盘／遮挡及硬件测试在对应阶段明确新增范围和授权后执行。

Qt6、受影响 Qt5 兼容、静态分析及 ASan 按具体风险选择；普通 Release 用于性能结论。比较接口关闭、空闲连接及请求负载，不承诺零损耗、100% 确定性或固定 5ms。

阶段记录保留实际源码与相关差异、构建配置、实际 EXE、机器／桌面、场景、结果及必要证据，区分当前、复用及未执行覆盖。不能把文档、语法或编码检查称作构建／运行通过。

### M0.1 文档迁移切片的完成条件（已完成）

- 新目录四份文档和 `AGENTS.md` 导航完整，链接及源文件引用有效。
- 新计划包含用户确认的目标、等价性、时间、双机交接及提交／推送边界。
- 移除旧 Codex 计划正文，只保留新位置作为唯一计划；原 Gemini 方案不变。
- 新目录未被 Git 忽略，差异只涉及本切片；保留既有无关本地配置。
- 文档为 UTF-8 无 BOM、LF，空白和大小检查通过。
- 更新本轮检查及未覆盖范围，形成**本地开发检查点**；不因此宣称 M0 或产品功能完成。

以上为 M0.1 当时的文档完成范围，其后阶段与当前实现以顶部交接卡片为准，不把早期“尚未接入”描述当成当前状态。TARGET-DESIGN-47 初次设计时的必要文档 commit 及任务分支普通推送授权仅按当轮范围保留；本轮 DESIGN-58 仅形成三份文档的本地提交，不推送，其他提交／推送仍按实际授权处理。

### 当前决定与实现前检查点

| ID | 决定／依据 | 后续证明或调整条件 |
|---|---|---|
| DEC-01 | 共用业务提交以明确源 Tab／源行／参数和冲突选择为输入，GUI 负责焦点、修饰键、控件采集与提示 | 每个能力实际接入前追踪所有相关调用者，以独立 GUI／MCP 对照和业务期望验收；不做全局模型重写 |
| DEC-02 | 首轮引用绑定实例及查询修订，不修改 INI 加永久 UUID；加载／重建后重新查询 | 若后续出现确实需要跨启动稳定引用的需求再单独设计；暂不把显示行号承诺为稳定 ID |
| DEC-03 | 监视、实际映射和停止清理完成是不同条件；新增完成通知以现有生命周期为基础 | Worker STOPPED 仍不能单独证明停止发送与控制器任务结束；必须有包含相关任务的测试，超时不写入 |
| DEC-04 | 官方 SDK 处理 MCP，C++ 保持有限业务 IPC；桥接依赖隔离、后续固定版本 | 正式 MCP 客户端回归与实际 stdio、错误、重连测试通过后才声明支持，不以直接 IPC 测试替代 |
| DEC-05 | 不预先锁死 `QLocalServer`：本机 Qt6.8.3 `qlocalserver_win.cpp` 的 `CreateNamedPipe`（141）没有远程拒绝标志，`listen(qintptr)`（230）明确不支持 Windows。已用 Win32 创建管道并交给 `QLocalSocket::setSocketDescriptor`，Qt6.8.3／Qt5.15.2 隔离编译和收发／关闭重建均通过 | 可按此路线实现最小本机 listener，但须补 Token／配对／撤销、负向身份与异常生命周期测试；同进程探针不能证明完整安全边界。不修改现有 SingleApplication 通信、不打补丁重编 Qt |
| DEC-06 | 普通／自定义通知共用 UI 定义但需要明确实例作用域；各设置以实际 getter／运行消费者为准 | `objectName` 或控件类相同不代表同一设置；候选清单只是定位，不对外发布原始 XML 范围 |

所有决定目前是源码与设计结论，不是产品、安全或跨机运行验收。PC-W11 的未核验不应伪装成失败或通过；可先继续本机不依赖第二台机器的设计、隔离测试和实现，相关阶段验收仍保留该缺口。

## 参考

- [产品使用手册](../../readme/QKeyMapper使用手册.html)：风险及功能盘点起点，需要对照当前源码。
- [手册覆盖表](../../readme/manual/coverage.md)：模块导航，不作为当前产品或 MCP 验收证据。
- [OpenAI 模型与推理档位说明](https://learn.chatgpt.com/docs/models?surface=app)：排期假设来源，非本项目工时基准。
- [Codex 跨主机交接说明](https://learn.chatgpt.com/docs/remote-connections#hand-off-a-chat-between-hosts)：原生交接仍需本计划规定的状态核对。

### M1.1c-4c 补充决定

历史v1决定，仅说明当时源码／证据；独立窗口权限现见105，保存启动选择现见136，默认v2目录现见137-A/B，不沿用下列旧状态推断当前支持范围。

- 管理窗口除通用高级设置外另有独立托盘入口，因为原高级设置在映射运行时关闭。MCP 默认本次进程关闭，不写个人 INI；用户打开窗口不等于启用接口。产品退出先关闭端点。窗口关闭／Escape 取消待批准请求，但保留已经启用的接口；撤销是单独明确操作。
- GUI 仅能授予首轮读取／配置／启停三组权限；已存记录中的其他权限位也完整显示，但当前没有对应产品业务能力。客户端名明确为自报，标签使用 PlainText，真实程序路径单行只读可滚动，避免长路径挤出批准按钮；Enter 不默认批准或撤销。
- 产品存储命名空间与应用绑定采用 SHA256：`QKeyMapper-MCP-App/v1`＋NUL＋Qt canonicalFilePath 的原样 UTF-8＋NUL＋当前 SessionId 的十进制 ASCII；不作 Unicode case folding，避免不同实际路径合并。目录为 GenericDataLocation 下 `QKeyMapper/MCP/<binding hex>`。SessionId 分开同时登录的记录，实际身份仍由 logon SID／AuthenticationId／SessionId／PID／创建时间核实；存储路径不是授权依据。该产品默认路径尚待实际产品验收，不能从维护目标的注入路径推断通过。
- 私有桌面测试／截图入口纳入 `QKeyMapper/tests/mcp_control/`，Qt、EXE、输出和 PowerShell 以参数传入，不依赖本机 out 脚本。Win11 需重建并重新运行，不能复制本机 EXE 或沿用 Win10 系统结论。
- 官方 Python SDK 已在隔离 venv 实际安装并核对为 `mcp==2.3.0`；依据 [PyPI](https://pypi.org/project/mcp/2.3.0/) 和 [官方 v2 SDK](https://github.com/modelcontextprotocol/python-sdk)，采用 MCPServer API。该准备不表示桥接已经实现；依赖锁及正式 SDK initialize／tools 调用在下一切片验证。

### M1.1c-4d 补充决定

- 发现文件仅七项公开连接元数据，原子发布、同用户私有 ACL；支持有界长路径。活跃文件只读租约和发布者复核防止同命名空间的第二服务器覆盖后继续服务。关闭按实例／进程核对并按句柄清理，失败保留且报告。
- 发现提示不等于授权：读取核对 OS 身份和明确指定的预期 EXE，连接仍须核对真实 pipe server／证明；不打开提示中尚未经 OS 比较的程序路径。无任意文件／属性／命令入口。
- 已确认默认开发沙箱与获准普通用户运行是不同账号上下文；两者均未提权。未记录用户名／SID，不据此推断 DPAPI 错误 2 的全部原因；当前结果不算实际异用户 IPC 拒绝已测。
- 管理员产品与普通桥接不能只凭 DACL 宣称兼容，仍要查验实际对象完整性级别与往返。依据 [Microsoft MIC](https://learn.microsoft.com/en-us/windows/win32/secauthz/mandatory-integrity-control)，完整性控制独立于 DACL；本轮未改变管道完整性标签，也未触发 UAC。高低权限／存储读取和真正产品进程列为未验证。
- 当前管理窗口在禁用时清除凭据 RAM，既有配对列表／撤销也不可用；需增加关闭状态的本地管理路径，在开放配置／运行业务前消除此缺口，不能要求用户为撤销先启用旧客户端的入口。

### M1.1d 补充决定

- 桥接使用官方 MCPServer／Client 的 stdio API，固定实际通过的全部依赖。当前公开工具只有连接状态、连接、申请 GUI 配对、断开和能力查询，不添加通用 RPC／属性／INI／命令入口。C++ 无业务回调时仅能力查询返回 transport_only 和空业务能力，其余业务仍拒绝。
- 客户端先验私有发现信息和显式可信 EXE，再验真实 pipe peer／登录会话及固定 HMAC transcript；整数创建时间直接取原生 FILETIME，避免 Python datetime 精度截断。身份／密钥不从 Agent 提交的确认值推断。
- 客户端使用当前用户 Windows 凭据管理器的本机持久化，并以 DPAPI 绑定机器、用途、显式发现／EXE 路径和 SessionId。pywin32 CredWrite 的 Unicode 参数只携带 DPAPI 密文十六进制；实际 API 错误已由互操作调查修正。先存客户端 key 再确认，服务端保存后才 ready；失败可能留孤立客户端记录，但不算授权成立。
- 原生 I/O 只有一个工作线程／在途请求；忙时拒绝新作业，断开最多排一项清理。取消在原线程发起并排空 I/O 后释放缓冲区／OVERLAPPED；任务提交与关闭同锁序列化，关闭可重复。没有写请求重试。
- 正式 SDK 用例独立启动 stdio 桥接，通过同源维护窗口的真实 Qt 控件事件批准／撤销；维护固定事件／故障夹具不进入产品。Qt6、Qt5 各通过真实 60 秒等待和保存失败；不能据此写成管理员产品或映射等价性通过。开发入口尚需 Python，普通用户免 Python 打包仍在 M3。

## 跨机接续

跨机由用户明确安排，按[交接导航](../README.md)、顶部卡片及固定 SHA 仅快进接续。保留本地改动，按当前动作恢复本机依赖和登记，原始证据、凭据、构建目录及缓存 PID 留在原机。
