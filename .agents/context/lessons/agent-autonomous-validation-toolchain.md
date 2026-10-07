# Agent Autonomous Validation & Layered Toolchain

## Purpose and ownership

This reference describes reusable QKeyMapper diagnostic and acceptance methods. The repository playbooks own the operational rules; AGENTS.md carries short workspace principles. Preserve autonomous validation and the default milestone gates. Exceptions require explicit user authorization for the current task.

Do not store task-specific dimensions, pixel-mask coordinates, diagnostic counts, private INI contents or session transcripts here. Keep detailed task evidence in ignored local artifacts. This existing reference is already indexed in lessons/MEMORY.md.

## Build targets and runtime preparation

Run from the repository root, non-elevated:

```powershell
pwsh -NoProfile -File .\scripts\build_qt6.ps1
pwsh -NoProfile -File .\scripts\build_qt6.ps1 -Diagnostic
pwsh -NoProfile -File .\scripts\build_qt6.ps1 -AddressSanitizer
pwsh -NoProfile -File .\scripts\build_qt6.ps1 -Diagnostic -AddressSanitizer
```

| Release target | Build directory |
|---|---|
| Standard | out/build_qt6/ |
| Diagnostic | out/build_qt6_diagnostic/ |
| Standard ASan | out/build_qt6_asan/ |
| Diagnostic ASan | out/build_qt6_diagnostic_asan/ |
| Qt 5.15.2 x64 Release | out/build_qt5_5152/ |

Daily iterations build ordinary Qt6 Release and verify affected UI paths. Build diagnostic for an actual diagnostic need or changes to diagnostic-only code. Full milestones retain ordinary and diagnostic Release, the intended ASan target, static analysis and visual gates; two ASan variants are not automatically required. Check Qt 5.15.2 compatibility at stage acceptance, earlier for version-specific APIs.

Use `pwsh -NoProfile -File .\scripts\build_qt5.ps1` for incremental Qt 5.15.2 x64 Release; `-Jobs` and `-Clean` are supported. All validation build trees belong under ignored `out/`. Existing root build trees are legacy artifacts: leave them untouched, regenerate qmake files in the new location and never silently use an old EXE as fallback.

Diagnostic requires Release, enables LOGOUT_TOFILE/DEBUG_LOGOUT_ON and writes the existing rotating log to `log/QKeyMapper.log` under the EXE directory. Ordinary Release does not compile the diagnostic-only sampling. Do not turn a diagnostic fix into unconditional logging.

The script discovers supported MSVC, Qt and JOM locations; use its reported EXE rather than inventing local build commands. JOM handles incremental object compilation. Resolve errors at their exact source location, use build-generated ui headers and check for source-tree shadows when Ui members disagree.

A build-output EXE is not necessarily a complete runnable package. Prepare a runtime under an ignored directory with the intended EXE, matching DLLs/resources and a controlled test INI. Do not use the user's normal runtime as a test fixture. Capture/ASan scripts do not deploy a package automatically. Verify the artifact path/hash after copying.

Before generating artifacts, confirm the destination is ignored with git check-ignore. Record source commit plus relevant worktree state or source hashes, Qt/toolchain, build configuration, EXE path/hash, launch arguments, environment overrides and test scenarios. Compilation, link warnings, static diagnostics and runtime results are separate evidence.

## Permissions and test-process ownership

Keep source/document edits, Git, builds, analysis and screenshot capture non-elevated. If the test needs driver privileges or the original executable's administrator manifest, use the already-authorized -UseGsudo option; only the short-lived launch/close helper and necessary target are elevated, with UAC confirmed by the user.

Reuse scripts/test_process.ps1 rather than process-name searches or new lifecycle wrappers:

- Start-QkmTestProcess returns the process, absolute EXE path, start ticks and elevation mode.
- Close-QkmTestProcess verifies identity and requests graceful close. An identity mismatch or failed close is a reported failure; the helper leaves an unclosed process running.
- Retain the actual exit code. Do not force-kill a user instance or treat a missing/null exit code as successful shutdown.

Preserve the formal executable and source manifest. A temporary asInvoker copy is limited to isolated UI checks without starting mapping or performing driver tests. Record that it differs from the formal runtime; it cannot establish administrator/driver behavior.

## Screenshot capture and comparison

After preparing the example runtime, launch and capture it:

```powershell
pwsh -NoProfile -File .\scripts\capture_ui_snapshots.ps1 -ExecutablePath .\out\ui_validation\runtime\QKeyMapper.exe -PrintWindow -Label ui_check
```

The PNG is written to `test_snapshots/snapshot_<Label>.png`; labels must be filename components. Preserve each comparison image before reusing its label.

Add -UseGsudo only for necessary, already-authorized test elevation. Use -Arguments for a startup scenario; those arguments are not proof of a live UI action.

Set `$testPid` to the verified intended PID before attaching to a running test instance:

```powershell
pwsh -NoProfile -File .\scripts\capture_ui_snapshots.ps1 -ProcessId $testPid -PrintWindow -Label ui_attached
```

Attachment does not close it and cannot be combined with launch options. When selecting a particular window, -WindowHandle must belong to that PID. If the default build EXE is missing, the capture script requires an explicit EXE path or PID; it never selects an arbitrary same-name instance. Prefer explicit targets for reproducible multi-instance tests.

-PrintWindow captures the HWND without depending on screen occlusion; the default screen-copy capture has different behavior. Use the same method for before/after comparisons, inspect the resulting image and report incomplete/invalid captures rather than assuming the API call proves visual correctness.

Use the image-viewing tool available on the current host (for example view_image or view_file). Do not block routine review on one tool name or request user screenshots when static capture can establish the result.

Match the baseline's language, theme, INI, client dimensions, DPI, populated data and page-visit history. Compare both fresh startup and relevant interaction history when hidden-page caches may matter. Check default presentation first when the task preserves it, then changed states and return paths.

Separate Qt logical client geometry from physical PNG dimensions, which may include native title bars and borders. QT_SCALE_FACTOR or QT_SCREEN_SCALE_FACTORS simulations are not genuine OS-DPI coverage. A localized baseline may have different content minima; do not force one language's nominal width onto another.

Keep raw images and original diffs. Define compared regions and justify every dynamic mask (for example a live process row or an expected dirty indicator). Do not mask static text, spacing or geometry regressions to obtain equality. Pixel equality of one region does not prove whole-window or interaction equivalence.

## State transitions and layout diagnostics

Drive the intended application action and confirm its observed state/log effect. A successful UIAutomation selection or a posted event alone does not prove the relevant Qt signal ran. Keep startup-argument scenarios separate from runtime-interaction scenarios.

Observe the latest corresponding transition after layout settles. If instrumentation exists, correlate a request/sequence with before, metric-update, after and settled samples. Fixed waits only allow time to pass; they do not establish a stable final state. Use bounded observation and report timeouts or incomplete transitions.

Choose focused diagnostic fields:

- Requested and actual client sizes, window state and applicable scale/environment values.
- minimumSize, maximumSize, sizeHint/minimumSizeHint, QSizePolicy, fonts and actual control geometry.
- Layout margins, spacing, spacers and hidden-page constraints/visit state.
- Child visibility/intersection against every ancestor, rather than only its immediate parent.
- Actual vertical row heights, header/column dimensions, viewport and scrollbars for tables.

Take correlated measurements before choosing a fix. Diagnostic trees need not collect mapping contents, device identities or window titles. Keep original logs local and redact existing sensitive output before sharing or creating durable examples.

## Static and memory-safety evidence

For an affected diagnostic translation unit:

```powershell
pwsh -NoProfile -File .\scripts\run_static_analysis.ps1 -Files QKeyMapper\qkeymapper.cpp -Diagnostic -BuildDirectory .\out\build_qt6_diagnostic
```

Match compile flags/generated headers to the tested build. Default gate: zero project diagnostics. Preserve `out/static-analysis/summary.txt`, `clang-tidy.txt` and `clazy.txt`; a subsequent run overwrites these paths, so copy the baseline/final evidence before rerunning. Check tool/compiler errors separately.

If the user explicitly authorizes an incremental gate, preserve the baseline version, full inventory and final diff. Compare file/rule/message, code context and multiplicity; account for shifted lines and inspect diagnostics on changed code. An unchanged total can hide a removed warning replaced by a new one. Do not suppress remaining diagnostics, reinterpret the strict script's failure as all-project success, or carry the exception to another task.

ASan requires the intended instrumented EXE. scripts/run_asan_check.ps1 checks the runtime marker and refuses ordinary Release fallback; confirm the build configuration and actual instrumented execution as well. After preparing the isolated ASan runtime:

```powershell
pwsh -NoProfile -File .\scripts\run_asan_check.ps1 -ExecutablePath .\out\ui_validation\runtime\QKeyMapper.exe
```

This command is for an ASan-instrumented runtime, not the ordinary diagnostic example. Add -UseGsudo only when necessary and authorized. Each run uses a separate `out/asan/<GUID>/` directory; use its actual path. Require meaningful execution, graceful exit 0 and no violation reports, and name exercised scenarios. Startup/close smoke does not validate scale transitions, dynamic controls or active mapping.

## Layered gates and handover

1. Code iteration: ordinary Qt6 Release and affected paths, zero errors and warnings; diagnostic is targeted when needed.
2. UI change: snapshots plus autonomous visual review.
3. Feature/stage milestone: ordinary and diagnostic Release, Qt 5.15.2 compatibility, Clang-Tidy/Clazy with zero project diagnostics and actual ASan instrumentation with zero memory bugs, unless an explicit task-specific authorization changes the required coverage.

After passing checks, repeat or widen them for new changes, failures or unresolved concerns, not merely because earlier checks finished. Identify affected paths after the last edit. Reuse results for unchanged inputs only with their original version and coverage; never label old ASan or UI evidence as a rerun of later source.

For documentation-only edits, verify command parameters, output paths, links, mirrored playbooks and whitespace. This does not establish compilation or runtime results and does not weaken code-change milestone gates.

Handover names current-build checks, historical checks, authorized omissions, unperformed scenarios and disclosed diagnostics. Keep compiler/linker/static results distinct. State local checkpoint versus independent commit slice, and do not commit or push without explicit user instruction.
