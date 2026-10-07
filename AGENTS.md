# QKeyMapper Workspace Instructions

- This is the shared repository instruction file for Claude Code and Codex.
- Windows-only Qt Widgets + WinAPI C++.
- Keep changes small and local; reuse existing QKeyMapper architecture; avoid new frameworks and large refactors.
- For QKeyMapper work, use `.agents/skills/qkeymapper-workflow/SKILL.md` as the repo playbook.
- Qt/C++ source files use UTF-8 without BOM; new code comments in English; do not edit .ts files unless asked.
- Qt compatibility: The shared source targets Qt 6.8.3 and Qt 5.15.2. Check Qt5 at stage acceptance, earlier for version-dependent APIs.
- Plans, confirmations, and summaries are in Chinese. When a user decision is needed, ask a clear question and allow a freeform response.
- UI language priority: Chinese is the primary acceptance language; preserve its functionality, display and interaction experience first. Continue maintaining and validating English and Japanese. If a minor visual defect in either language would require disproportionate cost or risk to stability, performance or architecture, present evidence and tradeoffs for the user to decide whether to accept it; this is not advance approval to ignore defects.
- If a change touches stability, driver behavior, or system API side effects, call it out and ask the user to decide.
- Autonomous validation & quality gates: After code or build-config edits, autonomously build ordinary Qt6 Release using `scripts\build_qt6.ps1` and require 0 errors and 0 warnings. Verify affected UI paths; build diagnostic when needed or when diagnostic-only paths change. Full milestones retain ordinary and diagnostic Release, static/visual gates and actual ASan instrumentation; task-specific exceptions need explicit user authorization.
- Visual debugging & acceptance: Use `scripts\capture_ui_snapshots.ps1` to capture UI screenshots and inspect them autonomously with the available image-viewing tool (for example `view_image` or `view_file`). Do not request interactive Computer Use or ask the user to do routine visual inspections that can be verified through static snapshots.
- Non-elevated execution boundary: Keep source edits, Git, builds, static analysis, and UI snapshots non-elevated. Only necessary test startup/close (driver privileges or the original executable manifest) may use already-authorized gsudo with user-confirmed UAC; do not elevate the whole development workflow or alter the formal manifest.
- Review and handover protocol: At completion, state whether the work forms a local development checkpoint ("本地开发检查点") or an independent commit slice ("独立提交切点"). Without explicit user commit instruction, do not automatically commit or push.
- Validation evidence: Tie results to the actual source/worktree and tested binary. Distinguish current checks, historical evidence and unperformed coverage; disclose diagnostics and explicitly authorized task-specific gate exceptions without weakening default milestone gates.
- Shared project experience memory is in `.agents/context/`; start with `.agents/context/project-memory.md` and consult its linked index before relevant work. Update it only with durable, verified knowledge.
- For QKeyMapper work, self-improving-agent artifacts must be stored in `.agents/context/lessons/` and indexed in `.agents/context/lessons/MEMORY.md`; never write QKeyMapper-specific experience to a global skill `memory/` directory.
- Knowledge graph at `.understand-anything/knowledge-graph.json`; use an available knowledge-graph tool when present, otherwise inspect the file directly.

## Architecture quick reference (按需查阅)

| Layer | Key files | What it does |
|-------|-----------|--------------|
| **Entry** | `QKeyMapper/main.cpp` | Qt 初始化、缩放适配、启动主窗口 |
| **Main UI** | `QKeyMapper/qkeymapper.cpp` (47k), `QKeyMapper/QKeyMapper.h` (2.5k), `QKeyMapper/qkeymapper.ui` | 主窗口、系统托盘、浮动按钮面板、准星叠加、窗口匹配、设置序列化、INI 读写 |
| **Mapping Engine** | `QKeyMapper/qkeymapper_worker.cpp` (21k), `QKeyMapper/qkeymapper_worker.h` (1.8k) | 核心映射线程：键盘/鼠标 Hook、手柄事件处理、连发/锁定/长按/双击、按键序列发送、Forza 自动映射 |
| **Constants** | `QKeyMapper/qkeymapper_constants.h` (2k) | 所有按键码、默认值、枚举、宏定义 |
| **Item Setup** | `QKeyMapper/qitemsetupdialog.cpp` (4.3k), `.h`, `.ui` | 单条映射项的完整编辑 UI（原始键/映射键/抬起键/连发/锁定/时机） |
| **Macro** | `QKeyMapper/qmacrolistdialog.cpp` (3.9k), `.h`, `.ui` | 宏列表管理、类别筛选、拖拽排序 |
| **Sequence Edit** | `QKeyMapper/qmappingsequenceedit.cpp` (1.9k), `.h`, `.ui` | 按键序列编辑器（undo/redo、拖拽排序） |
| **Floating Button** | `QKeyMapper/qfloatingbuttonsetupdialog.cpp` (1.5k), `QKeyMapper/qvbuttonpanel.cpp`, `.h` | 浮动按钮样式编辑、虚拟按钮面板 |
| **Input Drivers** | `QKeyMapper/interception_worker.cpp`, `QKeyMapper/Interception/include/interception.h` | Interception 键盘/鼠标拦截驱动封装 |
| **Joystick** | `QKeyMapper/QJoysticks/src/QJoysticks.cpp`, `QKeyMapper/QJoysticks/src/SDL_Joysticks.cpp` | 物理/虚拟手柄管理、SDL2 后端 |
| **Virtual Gamepad** | `QKeyMapper/ViGEm/include/ViGEm/Client.h`, `Common.h` | ViGEm 虚拟 Xbox/DS4 手柄 |
| **Gyro/Touchpad** | `QKeyMapper/GamepadMotion/GamepadMotion.hpp` | 陀螺仪→鼠标转换、触摸板手势 |
| **FakerInput** | `QKeyMapper/FakerInput/include/fakerinputclient.h`, `fakerinputcommon.h` | 虚拟键盘/鼠标输入设备 |
| **Updater** | `QKeyMapper/QSimpleUpdater/src/QSimpleUpdater.cpp`, `Updater.cpp`, `Downloader.cpp` | 在线版本检查与更新下载 |
| **Volume** | `QKeyMapper/volumecontroller.cpp`, `.h` | Windows 音频端点音量控制 |
| **Single Instance** | `QKeyMapper/singleapp/singleapplication.cpp`, `.h` | 单实例应用、主/副实例通信 |
| **OrderedMap** | `QKeyMapper/orderedmap/orderedmap.h` | 插入顺序保持的哈希映射 |
| **AHK Scripts** | `QKeyMapper/ahk_utils/script/*.ahk`, `*.vbs` | 窗口管理、输入法切换辅助脚本 |
| **Build Config** | `QKeyMapper/QKeyMapper.pro`, `*.pri`, `*.qrc` | QMake 项目文件、第三方库引用、资源文件 |
| **Docs** | `README.md`, `README_en.md`, `KeyNameChanged.md`, `CombinationSupportKeys.md` | 项目文档 |
