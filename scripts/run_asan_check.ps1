[CmdletBinding()]
param(
    [string]$ExecutablePath,
    [string]$Arguments = '',
    [switch]$UseGsudo,
    [ValidateRange(1, 60)]
    [int]$TimeoutSeconds = 15
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot 'test_process.ps1')

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $ExecutablePath) {
    $ExecutablePath = Join-Path $repoRoot "out\build_qt6_asan\release\QKeyMapper.exe"
}

if (-not (Test-Path $ExecutablePath)) {
    throw "Target executable not found: $ExecutablePath"
}
$ExecutablePath = (Resolve-Path -LiteralPath $ExecutablePath).Path
# Do not report a successful ASan check for an uninstrumented Release binary.
$imageText = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($ExecutablePath))
if ($imageText -notmatch 'clang_rt\.asan_dynamic[^\x00]*\.dll') {
    throw 'Target executable does not import the MSVC ASan runtime.'
}

$outDir = Join-Path $repoRoot ('out\asan\' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$asanLogPrefix = Join-Path $outDir "asan_log"

# Locate MSVC ASan DLL
$vsDir = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC"
if (-not (Test-Path $vsDir)) {
    $vsDir = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"
}
$latestMsvc = Get-ChildItem -Path $vsDir -Directory | Sort-Object Name -Descending | Select-Object -First 1
$asanDllDir = Join-Path $latestMsvc.FullName "bin\Hostx64\x64"

$qtBinCandidates = @(
    "C:\Qt\Qt6\6.8.3\msvc2022_64\bin",
    "C:\Qt\6.8.3\msvc2022_64\bin"
)
$qtBinDir = $null
foreach ($b in $qtBinCandidates) {
    if (Test-Path $b) { $qtBinDir = (Resolve-Path $b).Path; break }
}
if (-not $qtBinDir) { throw "Qt bin directory not found" }

$testEnvironment = @{
    PATH = "$asanDllDir;$($env:PATH);$qtBinDir"
    # Quote the Windows drive colon so the sanitizer flag parser keeps it in the value.
    ASAN_OPTIONS = "halt_on_error=1:abort_on_error=1:detect_leaks=0:log_path='$asanLogPrefix'"
}

Write-Host "Running AddressSanitizer check on: $ExecutablePath"
$testProcess = Start-QkmTestProcess -ExecutablePath $ExecutablePath -Arguments $Arguments -UseGsudo:$UseGsudo -EnvironmentOverrides $testEnvironment
$proc = $testProcess.Process
Write-Host "Process started (PID: $($proc.Id)). Monitoring for $TimeoutSeconds seconds..."

$exited = $proc.WaitForExit($TimeoutSeconds * 1000)
if ($exited) { throw "ASan test exited before the monitoring interval completed (exit $($proc.ExitCode))." }
Write-Host "Closing process gracefully..."
$exitCode = Close-QkmTestProcess -TestProcess $testProcess
if ($null -eq $exitCode -or $exitCode -ne 0) { throw "ASan test exited with code '$exitCode'." }

$asanLogs = @(Get-ChildItem -Path $outDir -Filter "asan_log*")
if ($asanLogs.Count -gt 0) {
    Write-Warning "AddressSanitizer reported issues:"
    foreach ($log in $asanLogs) {
        Get-Content $log.FullName
    }
    throw "ASan check failed!"
}

Write-Host "AddressSanitizer smoke check PASSED! No memory violations detected."
