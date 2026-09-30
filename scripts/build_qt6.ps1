[CmdletBinding()]
param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [switch]$AddressSanitizer,
    [switch]$Clean,
    [int]$Jobs = [System.Environment]::ProcessorCount
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$proFile = Join-Path $repoRoot "QKeyMapper\QKeyMapper.pro"
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$qmake = "C:\Qt\6.8.3\msvc2022_64\bin\qmake.exe"
$jom = "C:\Qt\Tools\QtCreator\bin\jom\jom.exe"

foreach ($tool in @($vcvars, $qmake, $jom, $proFile)) {
    if (-not (Test-Path -LiteralPath $tool)) {
        throw "Required tool/file was not found: $tool"
    }
}

$buildDirName = if ($AddressSanitizer) { "build_test_qt6_asan" } else { "build_test_qt6" }
$buildDir = Join-Path $repoRoot $buildDirName

if ($Clean -and (Test-Path -LiteralPath $buildDir)) {
    Write-Host "Cleaning build directory: $buildDir"
    Remove-Item -Recurse -Force -LiteralPath $buildDir
}

if (-not (Test-Path -LiteralPath $buildDir)) {
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
}

$qmakeConfig = @(
    "CONFIG+=$($Configuration.ToLowerInvariant())"
)
if ($AddressSanitizer) {
    $qmakeConfig += "CONFIG+=asan"
}

$qmakeArgs = @(
    $proFile,
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
cmd.exe /d /s /c $fullCmd
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

Write-Host "Build succeeded: $buildDir"
