[CmdletBinding()]
param(
    [string[]]$Files,
    [string]$BuildDirectory,
    [switch]$Diagnostic,
    [string]$ClangTidyPath,
    [string]$ClazyPath,
    [string]$ReportDirectory
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path $repoRoot $(if ($Diagnostic) { 'out\build_qt6_diagnostic' } else { 'out\build_qt6' })
}
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)

if (-not $ReportDirectory) { $ReportDirectory = Join-Path $repoRoot 'out\static-analysis' }
$reportDirectory = [IO.Path]::GetFullPath($ReportDirectory)
$clangTidyReport = Join-Path $reportDirectory "clang-tidy.txt"
$clazyReport = Join-Path $reportDirectory "clazy.txt"
$summaryReport = Join-Path $reportDirectory "summary.txt"
$clangTidyConfig = Join-Path $repoRoot ".clang-tidy"
# Designer auto-connect is intentional; keep the other Qt Creator checks.
$clazyChecks = "overloaded-signal,connect-non-signal,qstring-comparison-to-implicit-char,wrong-qevent-cast,lambda-in-connect,lambda-unique-connection,qdatetime-utc,qgetenv,qstring-insensitive-allocation,fully-qualified-moc-types,unused-non-trivial-variable,connect-not-normalized,mutable-container-key,qenums,qmap-with-pointer-key,qstring-ref,strict-iterators,writing-to-temporary,container-anti-pattern,qcolor-from-literal,qfileinfo-exists,qstring-arg,empty-qstringliteral,qt-macros,temporary-iterator,wrong-qglobalstatic,lowercase-qml-type-name,no-module-include,use-static-qregularexpression,auto-unexpected-qstringbuilder,connect-3arg-lambda,const-signal-or-slot,detaching-temporary,foreach,incorrect-emit,install-event-filter,non-pod-global-static,post-event,qdeleteall,qlatin1string-non-ascii,qproperty-without-notify,qstring-left,range-loop-detach,range-loop-reference,returning-data-from-temporary,rule-of-two-soft,child-event-qobject-cast,virtual-signal,overridden-signal,qhash-namespace,skipped-base-method,readlock-detaching"

function Resolve-Tool {
    param([string]$Explicit, [string[]]$CandidatePaths, [string]$CmdName)
    if ($Explicit -and (Test-Path $Explicit)) { return (Resolve-Path $Explicit).Path }
    foreach ($p in $CandidatePaths) {
        if ($p -and (Test-Path $p)) { return (Resolve-Path $p).Path }
    }
    $cmd = Get-Command $CmdName -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw "Tool not found: $CmdName (searched: $($CandidatePaths -join '; '))"
}

$clangTidyCandidates = @(
    "C:\Qt\Qt6\Tools\QtCreator\bin\clang\bin\clang-tidy.exe",
    "C:\Qt\Tools\QtCreator\bin\clang\bin\clang-tidy.exe"
)
$clazyCandidates = @(
    "C:\Qt\Qt6\Tools\QtCreator\bin\clang\bin\clazy-standalone.exe",
    "C:\Qt\Tools\QtCreator\bin\clang\bin\clazy-standalone.exe"
)

$clangTidy = Resolve-Tool $ClangTidyPath $clangTidyCandidates "clang-tidy.exe"
$clazy = Resolve-Tool $ClazyPath $clazyCandidates "clazy-standalone.exe"

$sourceRoot = Join-Path $repoRoot 'QKeyMapper'
$vendorDirectories = @('GamepadMotion', 'Interception', 'ViGEm', 'FakerInput', 'libusb',
    'QJoysticks', 'QSimpleUpdater', 'QHotkey', 'singleapp', 'orderedmap', 'win_lib', 'ahk_utils')
$sourcePrefix = [IO.Path]::GetFullPath($sourceRoot).Replace('\', '/') + '/'
$vendorPattern = '(?:' + (($vendorDirectories | ForEach-Object { [regex]::Escape($_) }) -join '|') + ')'
$headerFilter = [regex]::Escape($sourcePrefix).Replace('/', '[\\/]') + "(?!$vendorPattern[\\/]).*"

function Test-FirstPartyPath {
    param([string]$Path)
    $normalized = [IO.Path]::GetFullPath($Path).Replace('\', '/')
    return $normalized.StartsWith($sourcePrefix, [StringComparison]::OrdinalIgnoreCase) -and
        $normalized.Substring($sourcePrefix.Length) -notmatch "^$vendorPattern/" -and
        [IO.Path]::GetFileName($normalized) -notmatch '^(moc_|qrc_|ui_)'
}

function Get-MakefileValue {
    param([string]$Text, [string]$Name)
    $joined = $Text -replace '\\\r?\n\s*', ' '
    $match = [regex]::Match($joined, '(?m)^' + [regex]::Escape($Name) + '\s*=\s*([^\r\n]*)')
    if (-not $match.Success) { throw "Missing qmake variable: $Name" }
    return $match.Groups[1].Value.Trim()
}

function Split-MakefileArguments {
    param([string]$Value)
    return @([regex]::Matches($Value, '(?:"[^"]*"|[^\s"]+)+') | ForEach-Object { $_.Value })
}

$outPrefix = [IO.Path]::GetFullPath((Join-Path $repoRoot 'out')).TrimEnd('\') + '\'
if (-not $reportDirectory.StartsWith($outPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Static-analysis reports must be inside the repository out directory.'
}
New-Item -ItemType Directory -Force -Path $reportDirectory | Out-Null
$compileDbPath = Join-Path $reportDirectory 'compile_commands.json'
$mainMakefilePath = Join-Path $BuildDirectory 'Makefile.Release'
if (-not (Test-Path -LiteralPath $mainMakefilePath)) {
    throw "Build ordinary/diagnostic Release first; qmake Makefile missing: $mainMakefilePath"
}
$mainMakefile = Get-Content -Raw -LiteralPath $mainMakefilePath
$mainDefines = Get-MakefileValue $mainMakefile 'DEFINES'
if (($mainDefines -match '(?:^|\s)-DLOGOUT_TOFILE(?:\s|$)') -ne [bool]$Diagnostic) {
    throw '-Diagnostic must match the selected build directory configuration.'
}
$qmake = (Get-MakefileValue $mainMakefile 'QMAKE').Trim('"')

# Use the same x64 compiler environment as the build, instead of guessing SDK versions.
$vcvars = Resolve-Tool '' @(
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat'
) 'vcvars64.bat'
$environmentCommand = 'call "{0}" >nul && set INCLUDE && set VCToolsVersion' -f $vcvars
$environmentLines = @(& $env:ComSpec /d /s /c $environmentCommand 2>&1)
if ($LASTEXITCODE -ne 0) { throw 'Cannot load the x64 MSVC analysis environment.' }
$includeLine = $environmentLines | Where-Object { $_ -match '^INCLUDE=' } | Select-Object -First 1
$versionLine = $environmentLines | Where-Object { $_ -match '^VCToolsVersion=' } | Select-Object -First 1
if (-not $includeLine -or -not $versionLine) { throw 'Missing MSVC environment values.' }
$systemIncludes = $includeLine.Substring(8).Split(';', [StringSplitOptions]::RemoveEmptyEntries)
$msvcVersion = $versionLine.Substring(15).Trim('\')
$compatibilityVersion = $msvcVersion -replace '^14\.', '19.'

$clangVersion = (& $clangTidy --version 2>&1) -join [Environment]::NewLine
$clazyVersion = (& $clazy --version 2>&1) -join [Environment]::NewLine
$sdlEndianPath = Join-Path $sourceRoot 'QJoysticks\lib\SDL\include\SDL_endian.h'
# Analysis only: Clang 22 supplies the builtin duplicated by this legacy SDL shim.
$useSdlGuard = $clangVersion -match 'LLVM version 22\.' -and
    $clazyVersion -match 'LLVM version 22\.' -and
    (Get-Content -Raw -LiteralPath $sdlEndianPath) -match '(?s)#ifndef __PRFCHWINTRIN_H.*?static __inline__.*?_m_prefetch\('

$testBuildDirectory = Join-Path $reportDirectory 'diagnostics-test-qmake'
New-Item -ItemType Directory -Force -Path $testBuildDirectory | Out-Null
$testMakefilePath = Join-Path $testBuildDirectory 'Makefile'
$testProjectPath = Join-Path $sourceRoot 'diagnostics\tests\diagnostics_test.pro'
$qmakeCommand = 'call "{0}" >nul && "{1}" -o "{2}" "{3}" -spec win32-msvc' -f $vcvars, $qmake, $testMakefilePath, $testProjectPath
$qmakeOutput = @(& $env:ComSpec /d /s /c $qmakeCommand 2>&1)
$qmakeExitCode = $LASTEXITCODE
$qmakeOutput | Set-Content -LiteralPath (Join-Path $reportDirectory 'qmake.txt') -Encoding utf8
if ($qmakeExitCode -ne 0) { throw 'Cannot generate diagnostics-test compilation parameters.' }

$targets = @(
    @{ Name = 'main'; Makefile = $mainMakefilePath },
    @{ Name = 'crash-reporter'; Makefile = (Join-Path $BuildDirectory 'crash-reporter\Makefile') },
    @{ Name = 'diagnostics-test'; Makefile = $testMakefilePath }
)
$sourceFiles = if ($Files -and $Files.Count -gt 0) {
    @($Files | ForEach-Object {
        $resolved = (Resolve-Path -LiteralPath $_ -ErrorAction Stop).Path
        if (-not (Test-FirstPartyPath $resolved)) { throw "Not a first-party analysis target: $_" }
        $resolved
    } | Sort-Object -Unique)
} else {
    @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -Filter '*.cpp' -File |
        Where-Object { Test-FirstPartyPath $_.FullName } | ForEach-Object { $_.FullName } | Sort-Object -Unique)
}
if ($sourceFiles.Count -eq 0) { throw 'Static analysis requires at least one source file.' }

$dbEntries = @()
$jobs = @()
foreach ($target in $targets) {
    $makefile = Get-Content -Raw -LiteralPath $target.Makefile
    $targetDirectory = Split-Path -Parent $target.Makefile
    $targetSources = @(Split-MakefileArguments (Get-MakefileValue $makefile 'SOURCES') |
        ForEach-Object { [IO.Path]::GetFullPath($_.Trim('"'), $targetDirectory) } |
        Where-Object { (Test-FirstPartyPath $_) -and $_ -in $sourceFiles })
    if ($targetSources.Count -eq 0) { continue }
    $compilerFlags = Get-MakefileValue $makefile 'CXXFLAGS'
    if ($compilerFlags -notmatch '-std:c\+\+17') { throw 'Expected the C++17 MSVC target.' }
    $compilerArgs = @('--target=x86_64-pc-windows-msvc', '-std=c++17', '-fms-extensions',
        '-fdelayed-template-parsing', '-fexceptions', "-fms-compatibility-version=$compatibilityVersion", '-D_MT')
    if ($compilerFlags -match '(?:^|\s)[/-]MD(?:d)?(?:\s|$)') { $compilerArgs += '-D_DLL' }
    $compilerArgs += Split-MakefileArguments (Get-MakefileValue $makefile 'DEFINES')
    foreach ($include in (Split-MakefileArguments (Get-MakefileValue $makefile 'INCPATH'))) {
        if (-not $include.StartsWith('-I')) { throw "Unexpected qmake include argument: $include" }
        $includePath = [IO.Path]::GetFullPath($include.Substring(2).Trim('"'), $targetDirectory)
        $compilerArgs += '-I' + $includePath.Replace('\', '/')
    }
    foreach ($include in $systemIncludes) {
        $compilerArgs += @('-isystem', $include.Replace('\', '/'))
    }
    if ($target.Name -eq 'main' -and $useSdlGuard) { $compilerArgs += '-D__PRFCHWINTRIN_H' }
    $targetDbDirectory = Join-Path $reportDirectory $target.Name
    New-Item -ItemType Directory -Force -Path $targetDbDirectory | Out-Null
    $entries = @($targetSources | ForEach-Object {
        [PSCustomObject]@{
            directory = $targetDirectory
            file = $_
            arguments = @('clang++', $_) + $compilerArgs
        }
    })
    ConvertTo-Json -InputObject $entries -Depth 5 |
        Set-Content -LiteralPath (Join-Path $targetDbDirectory 'compile_commands.json') -Encoding utf8
    $dbEntries += $entries
    foreach ($entry in $entries) {
        $jobs += [PSCustomObject]@{ Target = $target.Name; Source = $entry.file; Database = $targetDbDirectory }
    }
}
$unmatched = @($sourceFiles | Where-Object { $_ -notin $dbEntries.file })
if ($unmatched.Count) { throw "First-party sources missing from qmake targets: $($unmatched -join ', ')" }
ConvertTo-Json -InputObject $dbEntries -Depth 5 | Set-Content -LiteralPath $compileDbPath -Encoding utf8
Write-Host "Running static analysis on $($sourceFiles.Count) files / $($jobs.Count) target compilations..."

$tidyLines = [System.Collections.Generic.List[string]]::new()
$clazyLines = [System.Collections.Generic.List[string]]::new()

$toolRuns = @()
foreach ($job in $jobs) {
    $src = $job.Source
    $relName = $job.Target + '/' + [System.IO.Path]::GetFileName($src)
    Write-Host "[Clang-Tidy] $relName"
    $prev = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $tOut = @(& $clangTidy "--config-file=$clangTidyConfig" "--header-filter=$headerFilter" "-p=$($job.Database)" $src 2>&1 | ForEach-Object { $_.ToString() })
        $tidyExitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $prev
    }
    foreach ($l in $tOut) { $tidyLines.Add($l) }

    Write-Host "[Clazy] $relName"
    $prev = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $cOut = @(& $clazy "-checks=$clazyChecks" "--header-filter=$headerFilter" "-p=$($job.Database)" $src 2>&1 | ForEach-Object { $_.ToString() })
        $clazyExitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $prev
    }
    foreach ($l in $cOut) { $clazyLines.Add($l) }
    $toolRuns += [PSCustomObject]@{
        Target = $job.Target; Source = $src; ClangTidyExitCode = $tidyExitCode; ClazyExitCode = $clazyExitCode
    }
}

$tidyLines.ToArray() | Set-Content -LiteralPath $clangTidyReport -Encoding UTF8
$clazyLines.ToArray() | Set-Content -LiteralPath $clazyReport -Encoding UTF8

$diagnostics = foreach ($line in @($tidyLines + $clazyLines)) {
    $norm = ($line -replace '\((\d+),(\d+)\):\s+', ':$1:$2: ').Replace('\', '/')
    if ($norm -match '^(.*?):\d+:\d+:\s+(warning|error):' -and (Test-FirstPartyPath $Matches[1])) { $norm }
}
$diagnostics = @($diagnostics | Sort-Object -Unique)

$summary = @(
    "Clang-Tidy: $clangTidy"
    "Clazy: $clazy"
    "Target Files: $($sourceFiles.Count)"
    "Target Compilations: $($jobs.Count)"
    "Clang-Tidy Checks: -*,clang-*"
    "Clazy Checks: $clazyChecks"
    "Analysis-only SDL compatibility guard: $useSdlGuard"
    "Nonzero Tool Exits: $(@($toolRuns | Where-Object { $_.ClangTidyExitCode -ne 0 -or $_.ClazyExitCode -ne 0 }).Count)"
    "Unique Diagnostics in Project: $($diagnostics.Count)"
    ""
) + $diagnostics

[PSCustomObject]@{
    SourceRevision = (& git -C $repoRoot rev-parse HEAD)
    WorktreeStatus = @(& git -C $repoRoot status --porcelain)
    BuildDirectory = $BuildDirectory
    Diagnostic = [bool]$Diagnostic
    ClangTidyVersion = $clangVersion
    ClazyVersion = $clazyVersion
    MsvcVersion = $msvcVersion
    ClangTidyChecks = '-*,clang-*'
    ClazyChecks = $clazyChecks
    AnalysisOnlySdlGuard = $useSdlGuard
    Runs = $toolRuns
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $reportDirectory 'runs.json') -Encoding utf8
$summary | Set-Content -LiteralPath $summaryReport -Encoding UTF8
Write-Host "Static analysis report: $summaryReport"
Write-Host "Diagnostics found: $($diagnostics.Count)"
if (@($toolRuns | Where-Object { $_.ClangTidyExitCode -ne 0 -or $_.ClazyExitCode -ne 0 }).Count -gt 0) {
    throw 'Static analysis tools returned nonzero exit codes. See runs.json and the full reports.'
}
if (@($tidyLines + $clazyLines | Where-Object { $_ -match '(^error:|fatal error:|:\s+error:)' }).Count -gt 0) {
    throw 'Static analysis encountered compiler/tool errors. See the full reports.'
}
if ($diagnostics.Count -gt 0) { throw 'Static analysis found diagnostics in project code. See summary.txt.' }
