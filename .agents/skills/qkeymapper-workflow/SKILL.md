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

The Agent must autonomously execute build and verification gates instead of delegating ordinary compilation to the user:

### 1. Build validation (`scripts/build_qt6.ps1`)
- Standard build: `powershell -File .\scripts\build_qt6.ps1` (Release) or with `-Configuration Debug`.
- ASan build: `powershell -File .\scripts\build_qt6.ps1 -AddressSanitizer` (builds to `build_test_qt6_asan/`).
- Incremental compilation: JOM automatically parallelizes object compilation in seconds.
- Compiler error diagnostics: Read exact compiler output (file, line, symbol, C-error code); fix syntax or unresolved identifiers directly in the source file and re-run build. Only pause if external toolchains or system dependencies are unrecoverable.
- Generated UI headers: UI compiler outputs `ui_*.h` to the build directory. If symbols mismatch, check for stray source-tree `QKeyMapper/ui_*.h` shadows.

### 2. Visual inspection (`scripts/capture_ui_snapshots.ps1`)
- Snapshot capture: `powershell -File .\scripts\capture_ui_snapshots.ps1 -Label <name> [-Arguments <cli-args>]`
- Multimodal review: Use the `view_file` tool to inspect the captured PNG in `test_snapshots/` directly. Check layout alignment, padding, margins, font clipping, and high DPI scaling.

### 3. Static analysis (`scripts/run_static_analysis.ps1`)
- Run Clang-Tidy & Clazy (53 checks): `powershell -File .\scripts\run_static_analysis.ps1 [-Files @("QKeyMapper\file.cpp")]`
- Review report: Check `out/static-analysis/summary.txt` to confirm 0 diagnostics in project code.

### 4. Memory safety validation (`scripts/run_asan_check.ps1`)
- After building with `-AddressSanitizer`, run: `powershell -File .\scripts\run_asan_check.ps1`
- Smoke checks monitor runtime execution for 15s under `ASAN_OPTIONS`. Check `out/asan/` for violations.

### 5. Layered gate protocol
- **Iteration Check**: Every code edit -> run `build_qt6.ps1` -> ensure 0 errors and 0 warnings.
- **UI Visual Gate**: UI changes -> run `capture_ui_snapshots.ps1` -> use `view_file` to review rendered layout.
- **Milestone Gate**: Feature/stage completion -> run `run_static_analysis.ps1` (0 warnings) + ASan check (0 memory bugs) before declaring milestone ready for review.

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
