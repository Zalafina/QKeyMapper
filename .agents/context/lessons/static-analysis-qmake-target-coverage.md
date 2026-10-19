# Static analysis from actual qmake targets

Verified against Qt 6.8.3/MSVC Release targets and native reporting regression
checks on 2026-10-08. Confidence: high. This is engineering evidence, not a new
rule collection or authorization to change runtime behavior.

- Use each generated qmake target's sources, definitions, include paths and CRT
  configuration. Native helpers must not inherit the Qt application's flags.
  For shared source, select configurations whose effective code or contracts
  are affected; do not mechanically analyze every target that includes the file.
- When a full inventory is selected, include maintained first-party diagnostic
  helpers and tests. Fail on an unmatched first-party source instead of silently
  dropping it. Vendor dependencies still need to parse; exclude vendor targets
  and diagnostics separately.
- Check native exit status independently of parsed diagnostics. Empty output
  and zero project warnings cannot turn a crashed or failed tool into a pass.
  Retain raw output, target configuration and per-tool exit codes.
- Keep any verified vendor-header compatibility workaround in analysis-only
  arguments, constrained to the applicable tool/header combination. Do not
  propagate it to product flags or edit vendor source to make the report clean.
- Use the current user-approved checks; record centralized exclusions and
  audited point waivers separately. Preserve intentional meta-object consumers
  and object lifetimes rather than changing behavior merely to remove a warning.

Select whether static analysis is needed using the playbook first. Diagnostic-only
log collection does not select this gate automatically. For a targeted check,
choose only configurations affected by the actual change; script discovery of
shared test targets does not make those targets mandatory for every task.

Entry points: scripts/run_static_analysis.ps1 and
scripts/test_static_analysis.ps1. The latter uses native stubs only to verify
reporting: vendor-warning exclusion, first-party-warning rejection and silent
nonzero exit rejection. It does not establish C++ correctness; run the actual
tools separately.

Maintained first-party test projects, sources and dedicated runners live in
QKeyMapper/tests/<topic>/; generated builds and reporting fixtures live under
out/. Keep target discovery aligned when moving a test; reporting-stub checks
verify inventory and failure handling, not actual static-analysis diagnostics.
See QKeyMapper/tests/README.md for the current engineering entry points.
