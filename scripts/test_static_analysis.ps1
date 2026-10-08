[CmdletBinding()]
param([string]$BuildDirectory)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $repoRoot 'out\build_qt6' }
$testDirectory = Join-Path $repoRoot ('out\static-analysis-tests\' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $testDirectory | Out-Null
$analysisScript = Join-Path $PSScriptRoot 'run_static_analysis.ps1'

function Assert-Analysis {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

# Native stubs test the reporting gate, not C++ correctness. Real tools run separately.
$passTool = Join-Path $testDirectory 'pass.exe'
$failTool = Join-Path $testDirectory 'fail.exe'
$stubSource = Join-Path $testDirectory 'stub.c'
$vendorPath = (Join-Path $repoRoot 'QKeyMapper\QJoysticks\vendor.h').Replace('\', '/')
$ownedPath = (Join-Path $repoRoot 'QKeyMapper\diagnostics\tests\diagnostics_test.cpp').Replace('\', '/')
@'
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--version") == 0) { puts("LLVM version 22.1.8"); return 0; }
    if (strstr(argv[0], "fail.exe")) { return 42; }
    if (strstr(argv[0], "warning.exe")) { puts("OWNED_PATH:1:1: warning: owned diagnostic"); return 0; }
    puts("VENDOR_PATH:1:1: warning: vendor diagnostic");
    return 0;
}
'@.Replace('VENDOR_PATH', $vendorPath).Replace('OWNED_PATH', $ownedPath) | Set-Content -LiteralPath $stubSource -Encoding utf8
$vcvars = @(
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat'
) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $vcvars) { throw 'MSVC is required to build the native test stub.' }
$compileCommand = 'call "{0}" >nul && cl /nologo /W3 /MT /Fe:"{1}" /Fo:"{2}" "{3}"' -f $vcvars, $passTool, (Join-Path $testDirectory 'stub.obj'), $stubSource
& $env:ComSpec /d /s /c $compileCommand
if ($LASTEXITCODE -ne 0) { throw 'Native reporting stub compilation failed.' }
Copy-Item -LiteralPath $passTool -Destination $failTool
$report = Join-Path $testDirectory 'pass'
& $analysisScript -BuildDirectory $BuildDirectory -ReportDirectory $report -ClangTidyPath $passTool -ClazyPath $passTool
$runs = Get-Content -Raw -LiteralPath (Join-Path $report 'runs.json') | ConvertFrom-Json
Assert-Analysis ($runs.ClangTidyChecks -eq '-*,clang-*') 'Unexpected Clang-Tidy checks.'
Assert-Analysis ($runs.ClazyChecks.Split(',').Count -eq 52 -and $runs.ClazyChecks.Split(',') -notcontains 'connect-by-name') 'Unexpected Clazy checks.'
$db = Get-Content -Raw -LiteralPath (Join-Path $report 'compile_commands.json') | ConvertFrom-Json
$worker = $db | Where-Object { $_.file -like '*qkeymapper_worker.cpp' }
Assert-Analysis ($worker.arguments -contains '-DSDL_SUPPORTED' -and $worker.arguments -contains '-DQT_WIDGETS_LIB') 'Missing main-target definitions.'
Assert-Analysis (@($worker.arguments | Where-Object { $_ -match '^-I[A-Z]:/.*/QtCore$' }).Count -gt 0) 'Absolute Qt include path was corrupted.'
$reporter = $db | Where-Object { $_.file -like '*crash_reporter.cpp' }
Assert-Analysis ($reporter.arguments -contains '-D_MT' -and $reporter.arguments -notcontains '-D_DLL' -and $reporter.arguments -notcontains '-DQT_CORE_LIB') 'Native helper inherited main-target flags.'
Assert-Analysis (@($db | Where-Object { $_.file -like '*crash_client.cpp' }).Count -eq 2) 'Shared diagnostic source needs both target configurations.'
Assert-Analysis (@($runs.Runs | Where-Object { $_.Target -eq 'diagnostics-test' -and $_.Source -like '*diagnostics_test.cpp' }).Count -eq 1) 'Diagnostics test was omitted.'

$failureReport = Join-Path $testDirectory 'fail'
$failed = $false
try {
    & $analysisScript -Files (Join-Path $repoRoot 'QKeyMapper\diagnostics\tests\diagnostics_test.cpp') -BuildDirectory $BuildDirectory -ReportDirectory $failureReport -ClangTidyPath $failTool -ClazyPath $passTool
} catch {
    $failed = $_.Exception.Message -like '*nonzero exit codes*'
}
Assert-Analysis $failed 'Silent native-tool failure passed the gate.'
$failureRuns = Get-Content -Raw -LiteralPath (Join-Path $failureReport 'runs.json') | ConvertFrom-Json
Assert-Analysis ($failureRuns.Runs[0].ClangTidyExitCode -eq 42) 'Native exit code was not retained.'
$warningTool = Join-Path $testDirectory 'warning.exe'
Copy-Item -LiteralPath $passTool -Destination $warningTool
$warningRejected = $false
try {
    & $analysisScript -Files $ownedPath -BuildDirectory $BuildDirectory -ReportDirectory (Join-Path $testDirectory 'warning') -ClangTidyPath $warningTool -ClazyPath $passTool
} catch {
    $warningRejected = $_.Exception.Message -like '*diagnostics in project code*'
}
Assert-Analysis $warningRejected 'First-party warning passed the gate.'
Write-Host "Static-analysis reporting tests passed: $testDirectory"
