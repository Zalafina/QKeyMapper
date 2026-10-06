# Agent Autonomous Validation & Layered Toolchain

## Context
When performing Qt6 / MSVC C++ development in QKeyMapper, the Agent must autonomously validate changes using local toolchains rather than asking the user to manually compile and verify. Early workflows constrained the Agent to passive user-delegated builds and single-retry bailouts. This lesson formalizes the autonomous validation toolchain and layered quality gates.

## Autonomous Toolchain Structure

### 1. Incremental Build (`scripts/build_qt6.ps1`)
- **Execution**: `powershell -File .\scripts\build_qt6.ps1 [-Configuration Release|Debug] [-AddressSanitizer] [-Clean]`
- **Tool resolution**: The script dynamically discovers MSVC vcvars64 (`BuildTools`, `Community`, etc.), Qt 6.8.3 (`C:\Qt\Qt6\6.8.3` or `C:\Qt\6.8.3`), and JOM parallel build tool (`Tools\QtCreator\bin\jom\jom.exe`).
- **Parallel incremental build**: Driven by `jom.exe -j%NUMBER_OF_PROCESSORS%`, compiling modified translation units in seconds.
- **Diagnostic pattern**: When MSVC reports compiler errors (e.g. `C2065` undeclared identifier), read the exact file and line number, verify class members or constants in relevant headers, fix in-place, and re-run.

### 2. Visual Snapshot Review (`scripts/capture_ui_snapshots.ps1`)
- **Execution**: `powershell -File .\scripts\capture_ui_snapshots.ps1 -Label <name> [-Arguments <args>]`
- **Output**: `test_snapshots/snapshot_<name>.png`
- **Multimodal inspection**: Agents must use the `view_file` tool to inspect the captured PNG directly. Verify:
  - Form layout vertical and horizontal alignment
  - GroupBox borders (`windowsStyle` 3D style) and center alignment
  - Child widget padding, margins, and absence of text clipping
  - Scaling across 50%, 100%, 125%, 150%, 200% factors

### 3. Static Analysis (`scripts/run_static_analysis.ps1`)
- **Execution**: `powershell -File .\scripts\run_static_analysis.ps1 [-Files @("path/file.cpp")]`
- **Engine**: Clang-Tidy (with root `.clang-tidy` rules) and Clazy standalone (53 Qt-specific checks).
- **Report**: `out/static-analysis/summary.txt` (filtered to project files, excluding third-party libraries).
- **Gate**: Milestone code must achieve 0 diagnostics in touched project files.

### 4. Memory Safety (`scripts/run_asan_check.ps1`)
- **Build**: `powershell -File .\scripts\build_qt6.ps1 -AddressSanitizer` (builds to `build_test_qt6_asan/`)
- **Run**: `powershell -File .\scripts\run_asan_check.ps1`
- **Mechanism**: Configures `ASAN_OPTIONS=halt_on_error=1:abort_on_error=1:detect_leaks=0:log_path=out/asan/asan_log`. Runs the executable, monitors for memory access violations or use-after-free, and closes cleanly.
- **Verification**: Verifies 0 ASan violation logs in `out/asan/`.

## Layered Gate Protocol
1. **Iteration Check**: Every code edit -> run `build_qt6.ps1` -> ensure 0 errors and 0 warnings.
2. **UI Visual Gate**: UI changes -> run `capture_ui_snapshots.ps1` -> use `view_file` to review rendered layout.
3. **Milestone Gate**: Feature/stage completion -> run `run_static_analysis.ps1` (0 warnings) + ASan check (0 memory bugs) before declaring milestone ready for review.
