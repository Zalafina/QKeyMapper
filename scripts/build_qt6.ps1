[CmdletBinding()]
param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [switch]$Diagnostic,
    [switch]$AddressSanitizer,
    [switch]$Clean,
    [int]$Jobs = [System.Environment]::ProcessorCount
)

$ErrorActionPreference = "Stop"

if ($Diagnostic -and $Configuration -ne "Release") {
    throw "-Diagnostic requires -Configuration Release."
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$proFile = Join-Path $repoRoot "QKeyMapper\QKeyMapper.pro"
function Resolve-Tool {
    param([string[]]$CandidatePaths, [string]$CommandName)
    foreach ($p in $CandidatePaths) {
        if ($p -and (Test-Path -LiteralPath $p)) {
            return (Resolve-Path -LiteralPath $p).Path
        }
    }
    if ($CommandName) {
        $cmd = Get-Command $CommandName -ErrorAction SilentlyContinue
        if ($cmd) { return $cmd.Source }
    }
    throw "Required tool was not found. Searched: $($CandidatePaths -join '; ') or command '$CommandName'"
}

$vcvarsCandidates = @(
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)
$qmakeCandidates = @(
    "C:\Qt\Qt6\6.8.3\msvc2022_64\bin\qmake.exe",
    "C:\Qt\6.8.3\msvc2022_64\bin\qmake.exe"
)
$jomCandidates = @(
    "C:\Qt\Qt6\Tools\QtCreator\bin\jom\jom.exe",
    "C:\Qt\Tools\QtCreator\bin\jom\jom.exe"
)

$vcvars = Resolve-Tool $vcvarsCandidates "vcvars64.bat"
$qmake = Resolve-Tool $qmakeCandidates "qmake.exe"
$jom = Resolve-Tool $jomCandidates "jom.exe"

if (-not (Test-Path -LiteralPath $proFile)) {
    throw "Project file not found: $proFile"
}

$buildDirName = if ($Diagnostic) {
    if ($AddressSanitizer) { "out\build_qt6_diagnostic_asan" } else { "out\build_qt6_diagnostic" }
} else {
    if ($AddressSanitizer) { "build_test_qt6_asan" } else { "build_test_qt6" }
}
$buildDir = [IO.Path]::GetFullPath((Join-Path $repoRoot $buildDirName))
$workspacePrefix = [IO.Path]::GetFullPath($repoRoot).TrimEnd('\') + '\'
if (-not $buildDir.StartsWith($workspacePrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Build directory must be inside the repository: $buildDir"
}

if ($Clean -and (Test-Path -LiteralPath $buildDir)) {
    Write-Host "Cleaning build directory: $buildDir"
    Remove-Item -Recurse -Force -LiteralPath $buildDir
}

if (-not (Test-Path -LiteralPath $buildDir)) {
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
}

$qmakeConfig = @(
    "CONFIG-=$(@{ Release = 'debug'; Debug = 'release' }[$Configuration])",
    "CONFIG+=$($Configuration.ToLowerInvariant())"
)
if ($Diagnostic) {
    $qmakeConfig += "DEFINES+=LOGOUT_TOFILE"
}
if ($AddressSanitizer) {
    $qmakeConfig += "CONFIG+=asan"
}

$qmakeArgs = @(
    ('"{0}"' -f $proFile),
    "-spec", "win32-msvc",
    ($qmakeConfig -join " ")
)

Write-Host "Configuring with qmake in $buildDir ..."
$cmdList = @(
    ('call "{0}" >nul' -f $vcvars),
    ('cd /d "{0}"' -f $buildDir),
    ('"{0}" {1}' -f $qmake, ($qmakeArgs -join " ")),
    ('"{0}" -j{1}' -f $jom, $Jobs)
)

$fullCmd = $cmdList -join " && "
Write-Verbose $fullCmd
$buildInfo = New-Object System.Diagnostics.ProcessStartInfo
$buildInfo.FileName = $env:ComSpec
$buildInfo.Arguments = '/d /s /c "' + $fullCmd + '"'
$buildInfo.UseShellExecute = $false
$buildInfo.CreateNoWindow = $true
$buildInfo.RedirectStandardOutput = $true
$buildInfo.RedirectStandardError = $true
# Normalize duplicate PATH/Path entries before vcvars and JOM inherit the environment.
$buildInfo.EnvironmentVariables['PATH'] = $env:PATH
$buildProcess = [System.Diagnostics.Process]::Start($buildInfo)
try {
    $buildErrors = $buildProcess.StandardError.ReadToEndAsync()
    while (-not $buildProcess.StandardOutput.EndOfStream) {
        Write-Output $buildProcess.StandardOutput.ReadLine()
    }
    $buildProcess.WaitForExit()
    Write-Output $buildErrors.GetAwaiter().GetResult()
    if ($buildProcess.ExitCode -ne 0) {
        throw "Build failed with exit code $($buildProcess.ExitCode)."
    }
} finally {
    $buildProcess.Dispose()
}

Write-Host "Build succeeded: $buildDir"
$executable = Join-Path $buildDir "$($Configuration.ToLowerInvariant())\QKeyMapper.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Expected executable was not generated: $executable"
}
Write-Output "Executable=$executable"
if ($Diagnostic) {
    Write-Output "DiagnosticLog=$(Join-Path (Split-Path -Parent $executable) 'log\QKeyMapper.log')"
}
