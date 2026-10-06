---
name: qkeymapper-workflow
description: QKeyMapper playbook for planning, implementation, debugging, and validation in this repo.
when_to_use: When planning a change, implementing a feature, debugging an issue, or validating a fix in this project. Also when the user asks about floating button behavior, key mapping logic, Forza mode, dialog UI changes, serialization, or mentions QKeyMapper.
---

# QKeyMapper Workflow

Use this skill for repo-specific work in QKeyMapper. Keep scope narrow, reuse existing architecture, and validate the smallest slice that proves the change.

## Start here
1. Anchor on the user-visible behavior, failing command, or touched file.
2. Step to the nearest owning function, class, or dialog.
3. Check same-module patterns before broad search.
4. Keep the first change small and reversible.
5. Before editing, identify a unique anchor for the exact snippet; if the file has repeated similar blocks, do not patch until the target is uniquely disambiguated by nearby literal text or line context.

## Core files
- qkeymapper.cpp / qkeymapper.h
- qkeymapper_worker.cpp / qkeymapper_worker.h
- qkeymapper_constants.h
- qitemsetupdialog.cpp / qitemsetupdialog.h
- qfloatingbuttonsetupdialog.cpp / qfloatingbuttonsetupdialog.h
- qkeymapper.ui
- qkeymapper_qt_compat.h
- QKeyMapper.pro

## Module guidelines
- Dialog/UI: update load, apply, save, and restore together; if generated UI members are involved, check for a source-tree ui_*.h shadow file first.
- Floating Button: treat constants, worker state, dialog load/apply, runtime visuals, and save/restore as one pipeline; watch resize loops; validate the smallest slice.
- Forza/runtime mapping: keep legacy and new syntax on the same runtime path; verify press/release ordering and per-player state transitions; add DEBUG_LOGOUT_ON only at the exact transition point.
- Serialization/settings: keep keymapdata.ini and Save Setting paths in sync; check load/apply/save/recover paths together.

## Qt version compatibility
- Code must compile on both Qt 6.8.3 and Qt 5.12.10 from a single source tree. When an API differs between versions, add a compatibility wrapper in `qkeymapper_qt_compat.h` — avoid scattering `#if QT_VERSION` checks throughout the codebase.

## Autonomous quality gates & validation

The Agent must autonomously execute build and verification gates instead of delegating ordinary compilation to the user. The default milestone gates remain unchanged; task-specific exceptions require explicit user authorization and must be reported with their scope.

### 1. Build validation (`scripts/build_qt6.ps1`)

- Standard build: `pwsh -NoProfile -File .\scripts\build_qt6.ps1` (Release) or with `-Configuration Debug`.
- Diagnostic Release: `pwsh -NoProfile -File .\scripts\build_qt6.ps1 -Diagnostic` (outputs to `out/build_qt6_diagnostic/`). Diagnostic plus ASan uses `-Diagnostic -AddressSanitizer` and a separate output directory.
- Standard ASan build: `pwsh -NoProfile -File .\scripts\build_qt6.ps1 -AddressSanitizer` (outputs to `build_test_qt6_asan/`).
- JOM incrementally builds changed objects. Read compiler failures, fix the exact source, and rebuild; do not treat whitespace or encoding checks as compilation.
- Use build-directory generated `ui_*.h`; if Ui members mismatch, check for source-tree shadow headers.
- Record source/worktree version, configuration, Qt/toolchain, build output and the tested EXE path/hash. Keep compiler, linker and static-analysis diagnostics distinct; disclose warnings rather than silently filtering them.

### 2. Visual inspection (`scripts/capture_ui_snapshots.ps1`)

- Launch a prepared isolated runtime: `pwsh -NoProfile -File .\scripts\capture_ui_snapshots.ps1 -ExecutablePath .\out\ui_validation\runtime\QKeyMapper.exe -PrintWindow -Label ui_check`. The example runtime must first be populated with the intended EXE, dependencies and test INI; the capture script does not deploy it.
- Set `$testPid` to the verified intended PID, then attach: `pwsh -NoProfile -File .\scripts\capture_ui_snapshots.ps1 -ProcessId $testPid -PrintWindow -Label ui_attached`. Attachment leaves the process running; do not combine it with launch options.
- Inspect PNGs with the available image-viewing tool, such as `view_image` or `view_file`. Check alignment, padding, text clipping and control visibility.
- Match language, theme, INI, client size, DPI, capture method and page-visit history before comparing. Distinguish logical client sizes from screenshot pixels and genuine OS DPI from simulated Qt factors.

### 3. Static analysis (`scripts/run_static_analysis.ps1`)

- Run Clang-Tidy and Clazy on the affected sources. For a diagnostic build, use `pwsh -NoProfile -File .\scripts\run_static_analysis.ps1 -Files QKeyMapper\qkeymapper.cpp -Diagnostic -BuildDirectory .\out\build_qt6_diagnostic`.
- Default gate: 0 project diagnostics in `out/static-analysis/summary.txt`; retain full reports and inspect tool/compiler failures.
- An explicitly authorized incremental gate must preserve a versioned baseline and compare diagnostic identity, code context and multiplicity, accounting for line shifts. Equal totals alone do not prove zero new diagnostics. Report the strict gate's actual exit status and remaining diagnostics.

### 4. Memory safety validation (`scripts/run_asan_check.ps1`)

- After an ASan build and isolated runtime preparation, pass the intended instrumented EXE with `-ExecutablePath`; do not substitute an ordinary Release binary.
- Verify the instrumentation/build target, observed execution, exit code, violation reports and exercised scenarios. Startup/close smoke coverage does not establish mapping or interactive UI coverage.
- Record explicitly authorized omissions. Earlier ASan results do not cover subsequent code changes; this does not waive the default milestone gate.

### 5. Layered gate protocol

- **Iteration Check**: Code changes -> affected build -> 0 errors and 0 warnings.
- **UI Visual Gate**: UI changes -> screenshots -> autonomous image review.
- **Milestone Gate**: Feature/stage completion -> Clang-Tidy/Clazy (0 project diagnostics) + ASan (0 memory bugs), unless the user explicitly authorizes a task-specific exception.
- After successful checks, repeat or broaden them only for new changes, failures or unresolved concerns. For a later edit, identify and recheck affected paths while retaining the version and coverage of reused evidence.
- Documentation-only changes need command/interface, link, mirror and whitespace checks; do not claim C++ build or runtime validation from these checks.

### 6. Diagnostic operation and evidence

1. Prepare test EXE/INI/logs under existing ignored paths and preserve the user's runtime/configuration. Use explicit EXE paths or PIDs; avoid choosing an arbitrary same-name process.
2. Reuse `scripts/test_process.ps1` for owned-process startup/close. Check PID, absolute EXE and start time; close normally, report an unclosed process, and do not force-kill another instance.
3. Keep editing, Git, builds, analysis and capture non-elevated. Only necessary test startup/close may use already-authorized `-UseGsudo` (driver requirements or the original executable manifest); the user handles UAC. Do not run the whole development workflow under gsudo.
4. Preserve the formal/source manifest. A temporary asInvoker copy may be used only for isolated UI checks without mapping/driver tests; report that boundary and do not claim privileged behavior was tested.
5. Separate startup arguments from live actions. Confirm the intended event actually changes application state; sample the latest matching request after layout settles, not merely after a fixed sleep or a successful automation call.
6. For layout faults, compare requested/actual sizes and constraint/hint chains, including hidden pages. Check clipping against every ancestor, and table row heights against the actual vertical header/viewport.
7. Keep original snapshots, raw logs and diffs; explain comparison crops and masks. Exclude only justified dynamic content, not unexpected static regressions. Redact private configuration, mapping text, device names and window titles from durable/shared examples.
8. Handover separates current-binary checks, historical evidence and unperformed coverage; include authorization scope, remaining diagnostics and checkpoint/commit status.

Detailed preparation, commands and diagnostic pitfalls: [Autonomous validation toolchain](../../../.agents/context/lessons/agent-autonomous-validation-toolchain.md).

## Advanced diagnostic sandbox (Opt-in Heavy Diagnosis)
- Default to Level 1 lightweight analysis: keep changes small and reversible; do not launch heavy custom build sandboxes for ordinary bugs.
- When an issue touches deep internal mechanisms (Qt-internal shared cache, hidden state, Win32 hook/message races, driver I/O) or resists standard debugging, propose Level 2 escalation to the user first:
  Ask explicitly: "是否需要让 Agent 自主搭建完整诊断工作流来解析定位复杂问题根因？"
- Only after explicit user approval, refer to `.agents/context/lessons/qt-headless-sandbox-diagnostic-workflow.md` to spin up the standalone headless console sandbox using the fast-path toolchain and batch templates.

## Style
- Keep scope narrow.
- Reuse existing architecture.
- New comments in English.
- No .ts edits unless asked.
- Keep Qt/C++ source UTF-8 without BOM.

## Translation
- When adding or modifying `tr(...)` strings, provide Chinese and Japanese translation recommendations alongside the English original. Present each string as a three-row table for readability: `| English | <original> |` / `| 中文 | <translation> |` / `| 日本語 | <translation> |`.

## Continuous improvement
### QKeyMapper workspace memory routing
When self-improving-agent is invoked for QKeyMapper work, the repository is the
only persistence location for QKeyMapper-specific experience. Do not create or
modify the global self-improving-agent installation's `memory/` directory.

1. Read `.agents/context/project-memory.md` before recording a lesson.
2. Record durable, reusable QKeyMapper decisions, pitfalls, and verified
   patterns in `.agents/context/lessons/`. Keep task transcripts, temporary
   status, and sensitive data out of the repository memory.
3. Add every new lesson to `.agents/context/lessons/MEMORY.md` so future
   QKeyMapper sessions can discover it.
4. Update `.agents/context/project-memory.md` only when the shared memory
   routing or other high-level project guidance changes.

The `.agents/context/` directory is version-controlled workspace knowledge and
must be the source used for remote synchronization.

After completing a task in this skill — especially when the user corrects you, a command fails, or you discover a better approach — invoke the self-improving-agent skill to extract patterns and update memory. This closes the loop so future sessions benefit from what was learned.
