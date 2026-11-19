[CmdletBinding()]
param([Parameter(Mandatory)][string]$TargetDirectory)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$target = 'x86_64-pc-windows-msvc'
$version = '1.100.0'
$pinned = "$version-$target"
$optOut = 'Use QKM_BUILD_MCP_BRIDGE=0 to build only the Qt application.'
$rustup = Get-Command rustup.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $rustup) { throw "MCP Sidecar requires rustup and Rust $pinned on PATH. $optOut" }

$installed = & $rustup.Source toolchain list
if ($LASTEXITCODE -ne 0) { throw "Cannot list installed Rust toolchains (exit $LASTEXITCODE). $optOut" }
$installed = $installed | ForEach-Object { ($_ -split '\s+', 2)[0] }
$toolchain = if ($pinned -in $installed) { $pinned } else { "stable-$target" }
if ($toolchain -notin $installed) { throw "MCP Sidecar requires an installed Rust $pinned. $optOut" }

$rustc = & $rustup.Source which --toolchain $toolchain rustc
if ($LASTEXITCODE -ne 0) { throw "Cannot locate rustc for $toolchain (exit $LASTEXITCODE). $optOut" }
$info = & $rustc -vV
if ($LASTEXITCODE -ne 0 -or "release: $version" -cnotin $info -or "host: $target" -cnotin $info) {
    throw "MCP Sidecar requires Rust $pinned. Selected toolchain: $toolchain. $optOut"
}

$TargetDirectory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($TargetDirectory)
$crate = Split-Path -Parent $PSScriptRoot
Write-Host "Building MCP Sidecar with $toolchain (Rust $version): $TargetDirectory"
Push-Location -LiteralPath $crate
try {
    $env:RUSTC = $rustc
    $env:CARGO_ENCODED_RUSTFLAGS = '-Ctarget-feature=+crt-static'
    $cargoArgs = @('build', '--locked', '--release', '--target', $target, '--bin', 'qkeymapper-mcp-rust',
        '--manifest-path', (Join-Path $crate 'Cargo.toml'), '--target-dir', $TargetDirectory)
    & $rustup.Source run $toolchain cargo @cargoArgs
    $cargoExit = $LASTEXITCODE
    if ($cargoExit -ne 0) {
        [Console]::Error.WriteLine("MCP Sidecar build failed (Cargo exit $cargoExit).")
        exit $cargoExit
    }
    $artifact = Join-Path $TargetDirectory "$target/release/qkeymapper-mcp-rust.exe"
    if (-not (Test-Path -LiteralPath $artifact -PathType Leaf)) { throw "Missing MCP Sidecar artifact: $artifact" }
    Write-Host "MCP Sidecar built: $artifact"
} finally {
    Pop-Location
}
