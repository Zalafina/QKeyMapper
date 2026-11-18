# QKeyMapper MCP Sidecar in Rust

This project currently provides the RUST-A protocol fixture. It exposes one
synthetic tool and requires the explicit `--protocol-fixture` argument. Follow the
[migration plan](../handoff/qkeymapper_mcp_control/qkeymapper_mcp_rust_migration_plan.md)
for the product connection and delivery status.

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
