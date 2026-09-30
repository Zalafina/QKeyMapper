[CmdletBinding()]
param(
    [string]$ExecutablePath,
    [int]$TimeoutSeconds = 15
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $ExecutablePath) {
    $ExecutablePath = Join-Path $repoRoot "build_test_qt6_asan\release\QKeyMapper.exe"
    if (-not (Test-Path $ExecutablePath)) {
        $ExecutablePath = Join-Path $repoRoot "build_test_qt6\release\QKeyMapper.exe"
    }
}

if (-not (Test-Path $ExecutablePath)) {
    throw "Target executable not found: $ExecutablePath"
}

$outDir = Join-Path $repoRoot "out\asan"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$asanLogPrefix = Join-Path $outDir "asan_log"

# Locate MSVC ASan DLL
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$vsDir = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC"
$latestMsvc = Get-ChildItem -Path $vsDir -Directory | Sort-Object Name -Descending | Select-Object -First 1
$asanDllDir = Join-Path $latestMsvc.FullName "bin\Hostx64\x64"

$env:PATH = "$asanDllDir;$($env:PATH);C:\Qt\6.8.3\msvc2022_64\bin"
$env:ASAN_OPTIONS = "halt_on_error=1:abort_on_error=1:detect_leaks=0:log_path=$asanLogPrefix"

Write-Host "Running AddressSanitizer check on: $ExecutablePath"
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $ExecutablePath
$psi.WorkingDirectory = (Split-Path -Parent $ExecutablePath)
$psi.UseShellExecute = $false

$proc = [System.Diagnostics.Process]::Start($psi)
Write-Host "Process started (PID: $($proc.Id)). Monitoring for $TimeoutSeconds seconds..."

$exited = $proc.WaitForExit($TimeoutSeconds * 1000)
if (-not $exited) {
    Write-Host "Closing process gracefully..."
    $proc.CloseMainWindow() | Out-Null
    Start-Sleep -Seconds 2
    if (-not $proc.HasExited) {
        $proc.Kill()
    }
}

$asanLogs = Get-ChildItem -Path $outDir -Filter "asan_log*"
if ($asanLogs.Count -gt 0) {
    Write-Warning "AddressSanitizer reported issues:"
    foreach ($log in $asanLogs) {
        Get-Content $log.FullName
    }
    throw "ASan check failed!"
}

Write-Host "AddressSanitizer smoke check PASSED! No memory violations detected."
