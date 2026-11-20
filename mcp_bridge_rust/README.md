# QKeyMapper MCP Sidecar in Rust

Rust is the supported QKeyMapper MCP Sidecar. The Python Sidecar source
and its earlier evidence are archived. Python remains useful for test
clients, Windows fixture utilities and packaging.

The Rust Sidecar provides stdio MCP, authenticated IPC, configuration
transactions and restricted windows with verified PNG results. It also includes
a standalone protocol fixture.
See the [MCP development plan](../handoff/qkeymapper_mcp_control/qkeymapper_mcp_automation_plan_codex.md)
for current delivery work and the [Rust migration record](../handoff/qkeymapper_mcp_control/qkeymapper_mcp_rust_migration_plan.md)
for earlier validation.

The main `QKeyMapper.pro` build enables `QKM_BUILD_MCP_BRIDGE=1` for Qt5 and Qt6
Windows x64 MSVC. Pass `QKM_BUILD_MCP_BRIDGE=0` on the qmake command line or in
Qt Creator's additional qmake arguments to build only the Qt application.
Qt5 x86 skips the Sidecar. Qt Debug and Release share this Rust Release output
under the repository root, including across Qt shadow builds.

```text
QKeyMapper/build/mcp_bridge/x86_64-pc-windows-msvc/release/qkeymapper-mcp-rust.exe
```

For a separate build or a fixture, use the same build script from the repository
root in a normal Windows x64 MSVC development shell. It uses an installed Rust
`1.100.0-x86_64-pc-windows-msvc` toolchain or an installed x64 `stable` at
exactly `1.100.0`. Python is used for packaging and fixture tests.

```powershell
$rustTarget = Join-Path (Get-Location) 'out/rust_sidecar/release-cargo'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\mcp_bridge_rust\tools\build_windows.ps1 -TargetDirectory $rustTarget
if ($LASTEXITCODE -ne 0) { throw 'Sidecar build failed.' }
$exe = Join-Path $rustTarget 'x86_64-pc-windows-msvc/release/qkeymapper-mcp-rust.exe'
```

Run the stdio fixture with the prepared test environment.

```powershell
& 'out/mcp_bridge/venv/Scripts/python.exe' -B -X utf8 mcp_bridge_rust/tests/test_stdio.py --exe $exe -v
```

Add `--sample-startup` to measure process launch through the first modern
`tools/list` response. The first measurement is the first launch in the
sample. Subsequent launches reuse the same Windows environment.

The fixture accepts newline-delimited UTF-8 JSON-RPC on stdout/stdin. Connect
Inspector 2.15.0 in CLI mode to the absolute EXE path with
`args: ["--protocol-fixture"]`. Use `--protocol-era legacy` or
`--protocol-era modern` and call `qkm_protocol_fixture` with `mode=text`,
`mode=error`, or `mode=image`. The 2×2 PNG is fixed protocol test data.

The fixture drops input lines above 64 KiB under the SDK's bounded line
reader. Supported legacy versions use initialization. Modern requests use
their own metadata. On Windows the SDK can keep a blocking stdin read after
a failed write to stdout. The adapter stops the MCP service and bounds
runtime shutdown in that case. This shutdown path is for stdio. Product
work and native named-pipe cancellation need their own completion checks.

The IPC candidate uses `--executable PATH --discovery-file PATH/instance.json`
or `--executable PATH --persistent-target`. Starting it leaves the connection
disconnected. Use `qkm_connect` with an existing pairing or
`qkm_request_pairing` followed by approval on the verified target.
Add either `--demo-record PATH` or `--product-demo-record` to expose the
read-only demo readiness check for a dedicated private product. The
record is verified after authentication against the selected target.
Add `--enable-demo-mapping` only for the dedicated configuration-only
product. The bridge runs each scene as one native job, returns a verified
image after cleanup, and keeps the cleanup receipt in its own process.

On Windows, `cargo test --locked` also covers argument handling, framing,
proof vectors, identity and pipe cancellation. Edit the reviewed MCP contract
in `src/tool_catalog.json` directly when changing tools, schemas or reviewed
errors. Run `tests/test_tool_contract.py --exe $exe` in the locked test
environment. It checks published tools, independent invalid arguments and
structured error expectations in both protocol modes. The opt-in
`dedicated_credential_roundtrip` test verifies and removes its own v1 and v2
DPAPI-protected Windows entries. The private Qt fixture uses
`tests/test_ipc_interop.py` and the repository's asInvoker
`QKeyMapperMcpControlTest.exe`. Run the Windows credential and Qt cases in
ordinary Medium integrity.

`tests/test_product_transactions.py` is the private stock-product check
for configuration changes. It needs a separately approved and registered
product copy, the untouched synthetic persistence fixture, and an exact Rust
client executable for the Read and Configure approval. The test controller
and each Rust session stay at ordinary Medium integrity.

`tests/test_product_demo.py` uses an approved configuration-only product
and the fixed private GUI helper for the two window permissions. It
checks real product images and recovers both demonstration scenes.

## Reordering a mapping row

Query the current mapping source and pass its row's `source_ref` with
one of `mapping.move_up`, `mapping.move_down`, `mapping.move_to_top` or
`mapping.move_to_bottom` as the `action` to `qkm_preview_changes`. Read
and Configure are required. The preview lists every affected row and
its zero-based old and new positions. Review it and commit with
`qkm_apply_changes` using its `preview_id`. The four actions update the
memory draft of the current unfiltered normal table. For any of the four
moves, `source_refs` may replace `source_ref` with 2–100 consecutive references
from the same source and revision in ascending order. To persist the
draft, preview and confirm a separate `configuration.save`.
Query the source again before using row references.

## Configuring burst and lock on a mapping

Use `mapping.update` with the current visible `source_ref` and a standalone
`row_modes` object. Set the boolean `burst` or `lock`, or the integer timings
`burst_press_ms` and `burst_release_ms` from 1 to 99999. Supply at least one
supported field. Timings can be set while burst is off on an eligible row.
Save the configuration separately to persist the selected values.

## Editing a mapping note

Use `mapping.update` with the current visible row's `source_ref` and
single-line `note`. Submit the note on its own. An empty string clears it.
Spaces and Unicode characters are preserved. Save the configuration
separately to persist the change.

## Editing a mapping category

Use `mapping.update` with the current visible row's `source_ref` and
standalone single-line `category`. The product trims surrounding whitespace
and updates the category filter options, as in its context menu. An empty
value clears the category. Save the configuration separately to persist it.

For a batch, replace `source_ref` with 2–100 distinct `source_refs` from
the same current source and revision. The preview lists each row in source
order and rejects the entire request if any row is out of scope or hidden.

## Filtering categories in a tab

Use `tab.set_category_filter` with the current `source_tab_ref` while the
category column is visible. Set `category_filter` to `"all"`, `[]` to select
none, or a list of category names. An empty string in that list selects
the Blank option when it is available. The filter affects the current view.
Source references and the saved configuration stay stable.

## Showing disabled mapping rows

Use `configuration.set_show_disabled_rows` with the current
`source_tab_ref` and the boolean `show_disabled_rows`. The setting applies
to all mapping tables. The category filter continues to affect visibility.
Save the configuration separately to persist the preference.

## Changing mapping table display options

Use `configuration.set_mapping_display` with the current `source_tab_ref`
and a `mapping_display` object containing one or more boolean options:
`category_column`, `notes`, and `floating_column`. Hiding the category column
clears the current tab's category filter. Save the configuration separately
to persist these display preferences.

## Renaming a mapping tab

Use `tab.rename` with the current normal tab's `source_tab_ref` and the
new `tab_name` in `qkm_preview_changes`. Read and Configure are required.
The existing product rules reject empty, reserved and duplicate names.
Submitting the current name leaves the source revision and draft intact.
A successful change gives a new source reference, including after a
rename back to the old name. Persist the draft with `configuration.save`.

## Adding a blank mapping tab

Pass `action: "tab.add_blank"` and the current normal tab's `source_tab_ref`
to `qkm_preview_changes` with Read and Configure. The preview shows the
default name, insertion index, empty local source and Common inheritance
setting. Applying it selects the new tab and advances the source revision.
Query the sources again. Use a separate `configuration.save` to persist it.

Use `action: "tab.copy"` with the same reference and permissions to copy
the ordered local rows, macro references and tab properties. The copy
appears immediately after its source and the source stays selected.
The preview uses the GUI copy name and reports if the hotkey is copied.
A new snapshot is needed for source references after the commit.

Use `action: "tab.delete"` with the selected normal tab's `source_tab_ref`.
Review the whole-tab scope and the tab that will become selected. Apply
with `confirm_delete: true`. Common and the last normal tab are protected.
Persistence still requires the separate `configuration.save` action.

Use `action: "tab.clear"` and the selected tab's `source_tab_ref` to clear
all of its local rows, including hidden rows. A clear on a normal tab
keeps inherited Common rows. Selecting and clearing the Common source
refreshes the tabs that inherit it. Apply with `confirm_delete: true`
and save separately to persist the draft.

Use `action: "tab.move"` with `source_tab_ref` for the selected normal tab
and `target_tab_ref` for another normal tab from the same source snapshot.
The source moves into the target's former position and stays selected.
Review the affected order, apply the preview, then query fresh references.

Use `tab.set_common_inheritance` with the current normal tab's
`source_tab_ref` and boolean `include_common`. Common mapping must be
enabled. Applying the preview updates the inherited display and keeps
the source rows. Persist the draft with a separate save.

Use `tab.set_common_collapsed` with the current normal tab's
`source_tab_ref` and boolean `common_collapsed` when the inherited Common
rows have a separator. Review and apply the preview. Local and Common
source rows stay intact, and expanding reapplies the existing filters.
Persist the display choice with a separate save.

Use `configuration.set_common_enabled` with the current `source_tab_ref`
and boolean `common_feature_enabled`. The preview lists any local mappings
that Common priority will disable. Review the list and pass
`confirm_conflicts=true` when applying such a preview. Turning Common off
keeps its rows in the same configuration and preserves each row's disabled
state. From the Common tab, turning it off selects the first normal tab.
Save the configuration separately to persist the change.

Use `tab.select` with the current `source_tab_ref` and a `target_tab_ref`
from the same source revision. The destination may be a normal tab or an
attached Common tab. Applying selects the tab and preserves the draft and
source rows. When the current tab changes after preview, review a new
preview. The result reports `selection_changed`.

Use `tab.set_hotkey` with the current normal tab's `source_tab_ref` and a
single-line `tab_hotkey`. Preview shows the key after the same normalization
used by Tab settings. An empty string clears it. Tabs can share a shortcut
and retain their order as destinations. Use a separate save to persist it.

Use `tab.set_notification_mode` with the current normal tab's
`source_tab_ref` and integer `notification_mode`. Mode 0 lets the configured
notification policy handle mapping starts and tab switches. Mode 1
suppresses start notifications. Mode 2 suppresses both. Stop notifications
follow the existing stop policy. Save separately to persist the mode.

Use `tab.set_notification_colors` with the current normal `source_tab_ref`
and either or both fields. `notification_font_color` accepts `#RRGGBB` and
`notification_background_color` accepts `#AARRGGBB`. Use `default` for either
field to inherit the usual color for its configuration. Save separately.

## Monitoring and stop results

`qkm_wait_state` samples the authenticated target for `monitoring_enabled`,
`mapping_active` or `stopped_and_quiescent`. The caller sets a timeout of
100–5000 ms (default 1000). A condition timeout includes the last sample.

With Read and RunMapping permission, `qkm_start_mapping` accepts the
current `instance`, `request_id`, `source_revision`, `source_tab_ref` and
`save_policy="start_only"`. Select the loaded normal profile and query its
current source before starting. The supported scope is simple keyboard rows
with automatic profile switching and FilterKeys changes disabled. The
operation starts monitoring using the current memory configuration.
Use `qkm_wait_state` to observe actual mapping activation.

With the same permissions, `qkm_stop_mapping` accepts the current
`instance` from `qkm_get_state` and a new 32-character lowercase hex
`request_id`. It can stop unmatched monitoring with stable sources and no
configured stop actions. It also accepts an active match started through
`qkm_start_mapping` if the source identity still matches. The tool observes
an existing GUI stop and returns `already_idle` for an idle target. Use the
GUI for other active configurations and configured stop actions.
The configuration-only demo omits the run controls.

The request ID is also the operation ID. Poll `qkm_get_operation` after
`waiting_cleanup` or a disconnect. `completed` means the product observed
cleanup for that operation. A cleanup timeout leaves the operation pending.
A repeated ID returns the retained result for the same instance, pairing
and permission epoch. Verify current state if the history is unavailable.
`tests/test_product_stop.py` covers the selected private GUI and Rust flow.

## Windows x64 component

Package the static build above using the same installed Rust toolchain and a
clean Rust source tree. Use `stable-x86_64-pc-windows-msvc` for `$rust` if that is
the exact-version toolchain reported by the build script. The packager uses
the selected toolchain's installed Rust standard library notices.

```powershell
$rust = '1.100.0-x86_64-pc-windows-msvc'
$component = Join-Path (Get-Location) 'out/rust_sidecar/release-input/QKeyMapperMcp-x64'
$dumpbin = (Get-Command dumpbin.exe).Source
rustup.exe run $rust python.exe -B -X utf8 mcp_bridge_rust/tools/package_windows.py --executable $exe --dumpbin $dumpbin --output $component
```

The packager checks x64 PE imports, retains the locked dependency notices and
source identities, and generates `QKeyMapperMcp-x64.zip` with its SHA-256 printed
in the result. The two x64 Release variants share this input. Their product
ZIPs copy `qkeymapper_mcp.exe` next to `QKeyMapper.exe` and the combined notice
to `licenses/QKeyMapper-MCP-Licenses.txt`. The guide, file manifest and original
dependency notices stay in the build component. See the
[client configuration examples](BUNDLE_README.md) and
[Win10 Release instructions](../scripts/release/Release使用说明.md).

For a separate dynamic test component, build with the default MSVC runtime
linkage and pass the installed x64 `VCRUNTIME140.dll` with `--crt`.

`tools/smoke_windows_bundle.ps1 -Archive PATH -Sha256 DIGEST -OutputDirectory NEW_PATH`
verifies package hashes, the runtime modules actually loaded, an MCP image
with structured content, and normal exit. The synthetic smoke uses Windows
PowerShell and runs independently of QKeyMapper and Python.
