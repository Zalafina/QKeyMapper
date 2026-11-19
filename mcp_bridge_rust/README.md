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
memory draft of the current unfiltered normal table. For top and bottom,
`source_refs` may replace `source_ref` with 2–100 consecutive references
from the same source and revision in ascending order. To persist the
draft, preview and confirm a separate `configuration.save`.
Query the source again before using row references.

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
