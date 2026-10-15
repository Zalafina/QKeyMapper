# Agent Autonomous Validation & Layered Toolchain

## Purpose and ownership

This reference describes reusable QKeyMapper diagnostic and acceptance methods, not a checklist to execute in full. AGENTS.md and the repository playbooks own risk-based check selection. Read only the methods needed for the selected scope; the examples below do not automatically require diagnostic, Qt5, full static analysis or ASan. Complete agreed coverage and obtain approval before reducing it.

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

Product C++/.ui/build-config changes default to one incremental ordinary Qt6 Release build (zero errors/warnings) and affected behavior; test-only edits build the relevant test target, and documentation/comments need no product build. Do not clean/rebuild by default. Select diagnostic for diagnosis or affected diagnostic-only code, Qt5 for affected version-specific APIs/styles/layout/serialization or planned compatibility acceptance, and targeted ASan for memory/lifetime risks. Stage completion alone does not require all configurations; use the playbook's selection table.

Use `pwsh -NoProfile -File .\scripts\build_qt5.ps1` for incremental Qt 5.15.2 x64 Release; `-Jobs` and `-Clean` are supported. All validation build trees belong under ignored `out/`. Existing root build trees are legacy artifacts: leave them untouched, regenerate qmake files in the new location and never silently use an old EXE as fallback.

Diagnostic requires Release, enables LOGOUT_TOFILE/DEBUG_LOGOUT_ON and writes the existing rotating log to `log/QKeyMapper.log` under the EXE directory. Ordinary Release does not compile the diagnostic-only sampling and is the performance baseline; diagnostic/ASan results establish diagnostic or safety behavior, not ordinary-user performance. Do not turn a diagnostic fix into unconditional logging.

The script discovers supported MSVC, Qt and JOM locations; use its reported EXE rather than inventing local build commands. JOM handles incremental object compilation. Resolve errors at their exact source location, use build-generated ui headers and check for source-tree shadows when Ui members disagree.

A build-output EXE is not necessarily a complete runnable package. Prepare a runtime under an ignored directory with the intended EXE, matching DLLs/resources and a controlled test INI. Do not use the user's normal runtime as a test fixture. Capture/ASan scripts do not deploy a package automatically. Confirm the actual copied/started EXE path and build source; hash relevant artifacts only when identity or provenance needs it.

Keep artifacts under ignored out/ paths (use git check-ignore for a new destination). Routine evidence is source commit/relevant worktree changes, configuration, actual EXE path, results and necessary logs/PNGs, with arguments/environment when relevant. Hash only for confusing binaries, cross-machine handover, deployment/provenance or reused-artifact identity; reuse hashes for unchanged inputs. No default whole-source fingerprints at any stage without an explicit byte-identity requirement, JSON receipts or duplicate manifests. Compilation, link warnings, static diagnostics and runtime results remain separate evidence.

### Standing exception: Qt5 qtmain missing-PDB LNK4099

The user approved a continuing exception for the already investigated vendor warning. Accept it only when all three conditions hold:

- The linked library comes from the official Qt 5.15.2 x64 `msvc2019_64` kit.
- The diagnostic identifies `qtmain.lib(qtmain_win.obj) : warning LNK4099`.
- The reason is a missing vendor PDB, with the object linked without debug information.

Provenance evidence: the local Qt5 logs `out/backup_popup_fix/build_qt5.log`, `out/font_clarity/build_qt5_direct.log` and `out/spin_editor_fix/build_fixed_qt5.log` show the same library/object warning in the QKeyMapper link step and the Qt 5.15.2 `msvc2019_64` build context. These are historical ignored local artifacts, not guaranteed to exist in another checkout or evidence of a later build. Keep the original logs; localized wording or console encoding may differ. Verify the current kit/library origin and missing-PDB cause rather than accepting the code or basename alone.

Matching occurrences remain in raw logs but do not fail Qt5 compatibility acceptance or require repetitive routine reminders. When the build otherwise passes, use "Qt5.15.2 兼容构建通过" in the routine summary. If the user asks for warning counts or detailed diagnostics, disclose the actual occurrences and their accepted status; never claim literal zero warnings for such a build.

Other libraries, objects, causes or Qt versions are outside this exception. Re-evaluate a changed Qt library source or diagnostic identity/cause. Do not add `/IGNORE:4099`, filter output, modify build scripts or rewrite historical results. Qt6 still requires zero errors and warnings, and other selected checks retain their pass criteria. This named standing exception does not carry a task-specific static-analysis or coverage exception into future tasks.

## Permissions and test-process ownership

Keep source/document edits, Git, builds, analysis and screenshot capture non-elevated. If the test needs driver privileges or the original executable's administrator manifest, use the already-authorized -UseGsudo option; only the short-lived launch/close helper and necessary target are elevated, with UAC confirmed by the user.

Reuse scripts/test_process.ps1 rather than process-name searches or new lifecycle wrappers:

- Start-QkmTestProcess returns the process, absolute EXE path, start ticks and elevation mode.
- Close-QkmTestProcess verifies identity and requests graceful close. An identity mismatch or failed close is a reported failure; the helper leaves an unclosed process running.
- Retain the actual exit code. Do not force-kill a user instance or treat a missing/null exit code as successful shutdown.

Preserve the formal executable and source manifest. A temporary asInvoker copy is limited to isolated UI checks without starting mapping or performing driver tests. Record that it differs from the formal runtime; it cannot establish administrator/driver behavior.

## Desktop selection and user disruption

Prefer the least disruptive execution environment that can fully establish the intended result. A private desktop here is a Windows desktop object: a real Qt window can have an HWND and render there without appearing on the user's active `Default` desktop or taskbar. It is not Qt offscreen rendering, a hidden window or Windows Task View. Never change the tested style, DPI or business behavior merely to obtain background execution.

| Intended evidence | Recommended execution |
|---|---|
| Fonts, layout, sizes, clipping, programmatic scale changes, window creation/destruction and return values | Real windows on an available private desktop, `PrintWindow`, actual state checks and image inspection, when sufficient for the scenario |
| Real mouse/keyboard, foreground focus, user-desktop taskbar/tray, occlusion or z-order | Necessary scenarios on the user's `Default` desktop |
| Physical cross-monitor dragging and associated focus/DPI recovery | `Default`; programmatic positioning can be checked separately in isolation |
| Private desktop unavailable, or captured content cannot prove the behavior | Explain the limitation and why `Default` is needed; preserve quality gates and global sandbox configuration |

Modal dialogs, menus, ComboBox popups and native file dialogs do not require `Default` solely because of their type. Choose from the required behavior and input method. A programmatic action or UIAutomation invocation may establish its observed state transition, but must not be reported as proof of a physical mouse/keyboard action.

### Planning and execution

Plans must list necessary `Default` scenarios, reasons, operation/capture methods, visible-window/focus/mouse/keyboard impact and estimated duration; explicitly state none when no such tests are required. For example, distinguish a private-desktop layout snapshot from a short `Default` mouse-drag test rather than calling both "UI verification".

After plan approval, notify the user before starting and execute the planned tests, including approved short real-input/foreground use, without reconfirming a time slot. Batch visible tests to reduce disruption. Disclose added `Default` scenarios before execution; new input/foreground scope outside the approved plan requires user authorization. Release any keys or mouse buttons held by the test, gracefully close self-started instances, and leave attached user instances and user configuration intact.

The current `scripts/capture_ui_snapshots.ps1` attempts `SetForegroundWindow` before either capture path. Consequently, `-PrintWindow` on `Default` can affect focus; the flag selects the capture method, not a silent launch or a desktop. `CreateNoWindow` does not prevent the GUI application from creating its own windows. Do not use elevation merely to bypass desktop isolation or change global sandbox settings as a test convenience. The existing privilege and process-ownership rules still apply.

### Desktop identity and evidence boundaries

Record the tested PID/absolute EXE, target window desktop, operation/capture methods and actual coverage. When available, use the target HWND's thread ID from `GetWindowThreadProcessId`, then `GetThreadDesktop` and `GetUserObjectInformation` (`UOI_NAME`) to identify its desktop. The runner's thread desktop and the current input desktop (`OpenInputDesktop`) provide comparison context, not a substitute for target identity. Check query success and access limitations; an unavailable identity is "unverified", not an assumed private or `Default` desktop.

Do not infer the target's desktop from `config.toml`, launcher options, a console window's absence, a successful `SetForegroundWindow` attempt or a saved PNG. Launch routes and effective sandbox behavior may vary between machines or helpers. Inspect the actual target after launch or relaunch. A private-desktop capture must contain the intended, settled content; API success alone does not establish validity.

Report private-desktop presentation/state checks separately from real user-desktop interaction. They do not by themselves prove foreground activation, physical input, user-desktop shell behavior or physical dragging. Preserve historical evidence as recorded; do not retroactively assign a desktop to old runs that did not record it. These methods do not expand the selected verification scope or require a new diagnostic framework.

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

For the chosen comparison, match language, theme, INI, client dimensions, DPI, populated data and page-visit history. Start with Chinese and affected states; expand languages/themes when changed text, fonts, layout or theme mechanisms affect them, not into a default complete matrix. Compare fresh startup and relevant interaction history when hidden-page caches matter, then changed states and return paths.

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

Simple, direct low-risk fixes need no static analysis. For C++ changes to control flow, state, lifetime or interfaces, select affected translation units and necessary target configurations; full first-party inventory is not a routine default. For example, an ordinary Release translation unit:

```powershell
pwsh -NoProfile -File .\scripts\run_static_analysis.ps1 -Files QKeyMapper\qkeymapper.cpp -BuildDirectory .\out\build_qt6
```

Match compile flags/generated headers to the tested build; for a selected diagnostic configuration add -Diagnostic and its build directory. Require native tool exits 0 and zero project diagnostics. Keep raw `out/static-analysis/summary.txt`, `clang-tidy.txt` and `clazy.txt`; preserve relevant baseline/final reports before a rerun overwrites them. Tool/compiler failures are not a zero-diagnostic pass.

If the user explicitly authorizes an incremental gate, preserve the baseline version, full inventory and final diff. Compare file/rule/message, code context and multiplicity; account for shifted lines and inspect diagnostics on changed code. An unchanged total can hide a removed warning replaced by a new one. Do not suppress remaining diagnostics, reinterpret the strict script's failure as all-project success, or carry the exception to another task.

When memory/lifetime risk or the acceptance plan selects ASan, use the intended instrumented EXE. scripts/run_asan_check.ps1 checks the runtime marker and refuses ordinary Release fallback; confirm the build configuration and actual instrumented execution as well. After preparing the isolated ASan runtime:

```powershell
pwsh -NoProfile -File .\scripts\run_asan_check.ps1 -ExecutablePath .\out\ui_validation\runtime\QKeyMapper.exe
```

This command is for an ASan-instrumented runtime, not the ordinary diagnostic example. Add -UseGsudo only when necessary and authorized. Each run uses a separate `out/asan/<GUID>/` directory; use its actual path. Require meaningful execution, graceful exit 0 and no violation reports, and name exercised scenarios. Startup/close smoke does not validate scale transitions, dynamic controls or active mapping.

## Layered gates and handover

Choose checks using the playbook's risk-based table. A plan briefly names risk, recommended checks and important omissions; offer 精简／推荐／扩展 only for meaningful coverage/cost tradeoffs. A small feature or stage completion does not automatically require full gates. If a full gate is selected, complete ordinary and diagnostic Release, Qt5 compatibility, full first-party Clang-Tidy/Clazy, actual ASan and relevant UI acceptance; duplicate ASan variants need a reason. Obtain approval before reducing agreed coverage.

After passing checks, repeat or widen them for new changes, failures or unresolved concerns, not merely because earlier checks finished. Identify affected paths after the last edit. Reuse results for unchanged inputs only with their original version and coverage; never label old ASan or UI evidence as a rerun of later source.

For documentation-only edits, verify relevant command parameters, output paths, links, mirrored playbooks and whitespace; do not build/run the product. This establishes document consistency, not compilation or runtime results. Missing targeted tests call for the smallest reproduction/assertion that proves the behavior, not automatic static-only acceptance. Do not create elaborate mocks, permanent test projects or heavy sandboxes for simple fixes, and do not use line count or a fixed duration cutoff to skip necessary checks.

Small tasks need a short handover naming current checks, reused evidence, important unperformed coverage and diagnostics; update progress documents only when relevant, without rewriting historical reports. Keep compiler/linker/static results distinct. State local checkpoint versus independent commit slice, and do not commit or push without explicit user instruction. Update experience only for new, verified, reusable knowledge, not every task completion, correction or failed command; do not duplicate playbook policy as a new lesson.
