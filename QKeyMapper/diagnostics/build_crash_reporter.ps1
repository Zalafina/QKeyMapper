[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$QMakePath,
    [Parameter(Mandatory)][string]$MakePath,
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$DeployDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'diagnostic_artifacts.ps1')
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$DeployDirectory = [IO.Path]::GetFullPath($DeployDirectory)
New-Item -ItemType Directory -Path $BuildDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $DeployDirectory -Force | Out-Null
Push-Location -LiteralPath $BuildDirectory
try {
    & $QMakePath '-o' 'Makefile' (Join-Path $PSScriptRoot 'crash_reporter.pro') '-spec' 'win32-msvc'
    if ($LASTEXITCODE -ne 0) { throw "Crash reporter qmake failed: $LASTEXITCODE" }
    & $MakePath
    if ($LASTEXITCODE -ne 0) { throw "Crash reporter build failed: $LASTEXITCODE" }
    Assert-ReleaseOptions (Join-Path $BuildDirectory 'Makefile')
    $null = Assert-SymbolPair (Join-Path $BuildDirectory 'release/QKeyMapperCrashReporter.exe') (Join-Path $BuildDirectory 'release/QKeyMapperCrashReporter.pdb')
    Assert-DbgHelpImport (Join-Path $BuildDirectory 'release/QKeyMapperCrashReporter.exe') $true
    foreach ($name in @('QKeyMapperCrashReporter.exe', 'QKeyMapperCrashReporter.pdb')) {
        $source = Join-Path $BuildDirectory "release/$name"
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Missing reporter artifact: $source" }
        Copy-Item -LiteralPath $source -Destination (Join-Path $DeployDirectory $name) -Force
    }
}
finally { Pop-Location }
