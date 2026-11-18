# QKeyMapper MCP Sidecar in Rust

This project provides the RUST-A protocol fixture and a candidate for pairing
and authenticated IPC, including queries, configuration transactions and
restricted windows with verified PNG results.
Follow the [migration plan](../handoff/qkeymapper_mcp_control/qkeymapper_mcp_rust_migration_plan.md)
for validation status and the remaining product and delivery work.

Build from the repository root in an ordinary Windows x64 development shell
with the MSVC C++ build tools. The existing Python environment is used for the
test client only.

```powershell
$env:RUSTUP_HOME = Join-Path (Get-Location) 'out/rust_sidecar/toolchain'
$env:CARGO_HOME = Join-Path (Get-Location) 'out/rust_sidecar/cargo'
$env:CARGO_TARGET_DIR = Join-Path (Get-Location) 'out/rust_sidecar/target'
$rust = '1.100.0-x86_64-pc-windows-msvc'
rustup toolchain install $rust --profile minimal --component rustfmt,clippy --no-self-update
cargo "+$rust" build --locked --release --manifest-path mcp_bridge_rust/Cargo.toml
$exe = 'out/rust_sidecar/target/release/qkeymapper-mcp-rust.exe'
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

On Windows, `cargo test --locked` also covers argument handling, framing,
proof vectors, identity and pipe cancellation. Run `tests/test_tool_contract.py --exe $exe`
with the locked Python environment to compare the tool catalog and result
format on both protocol versions. Use its `--export-catalog` option when
synchronizing reviewed Python tool descriptions, schemas or error messages.
The ignored `dedicated_credential_roundtrip`
test uses the locked Python environment to cross-check two newly generated
credential contexts and removes both entries. The private Qt fixture uses
`tests/test_ipc_interop.py` and the repository's asInvoker
`QKeyMapperMcpControlTest.exe`. Run the Windows credential and Qt cases in
ordinary Medium integrity.

`tests/test_product_transactions.py` is the private stock-product check
for configuration changes. It needs a separately approved and registered
product copy, the untouched synthetic persistence fixture, and an exact Rust
client executable for the Read and Configure approval. The test controller
and both sidecars stay at ordinary Medium integrity.
