[CmdletBinding()]
param(
    [string[]]$Files,
    [string]$BuildDirectory,
    [switch]$Diagnostic,
    [string]$ClangTidyPath,
    [string]$ClazyPath
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path $repoRoot "build_test_qt6"
}

$reportDirectory = Join-Path $repoRoot "out\static-analysis"
$clangTidyReport = Join-Path $reportDirectory "clang-tidy.txt"
$clazyReport = Join-Path $reportDirectory "clazy.txt"
$summaryReport = Join-Path $reportDirectory "summary.txt"
$clangTidyConfig = Join-Path $repoRoot ".clang-tidy"
$headerFilter = ".*[\\/]QKeyMapper[\\/].*"
$clazyChecks = "overloaded-signal,connect-by-name,connect-non-signal,qstring-comparison-to-implicit-char,wrong-qevent-cast,lambda-in-connect,lambda-unique-connection,qdatetime-utc,qgetenv,qstring-insensitive-allocation,fully-qualified-moc-types,unused-non-trivial-variable,connect-not-normalized,mutable-container-key,qenums,qmap-with-pointer-key,qstring-ref,strict-iterators,writing-to-temporary,container-anti-pattern,qcolor-from-literal,qfileinfo-exists,qstring-arg,empty-qstringliteral,qt-macros,temporary-iterator,wrong-qglobalstatic,lowercase-qml-type-name,no-module-include,use-static-qregularexpression,auto-unexpected-qstringbuilder,connect-3arg-lambda,const-signal-or-slot,detaching-temporary,foreach,incorrect-emit,install-event-filter,non-pod-global-static,post-event,qdeleteall,qlatin1string-non-ascii,qproperty-without-notify,qstring-left,range-loop-detach,range-loop-reference,returning-data-from-temporary,rule-of-two-soft,child-event-qobject-cast,virtual-signal,overridden-signal,qhash-namespace,skipped-base-method,readlock-detaching"

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

# Generate or load compile_commands.json
$compileDbPath = Join-Path $reportDirectory "compile_commands.json"
New-Item -ItemType Directory -Force -Path $reportDirectory | Out-Null

$sourceFiles = @()
if ($Files -and $Files.Count -gt 0) {
    foreach ($f in $Files) {
        if (Test-Path $f) {
            $sourceFiles += (Resolve-Path $f).Path
        } else {
            throw "Static analysis source file was not found: $f"
        }
    }
} else {
    # Default to main project cpp files
    $sourceFiles = @(Get-ChildItem -Path (Join-Path $repoRoot "QKeyMapper") -Filter "*.cpp" |
        Where-Object { $_.Name -notmatch '^(moc_|qrc_|ui_)' } |
        ForEach-Object { $_.FullName })
}

$vsDir = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC"
if ($sourceFiles.Count -eq 0) { throw 'Static analysis requires at least one source file.' }
if (-not (Test-Path $vsDir)) {
    $vsDir = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"
}
$msvcVer = (Get-ChildItem -Path $vsDir -Directory | Sort-Object Name -Descending | Select-Object -First 1).Name

$sdkIncludeDir = "C:\Program Files (x86)\Windows Kits\10\include"
$sdkVer = (Get-ChildItem -Path $sdkIncludeDir -Directory | Sort-Object Name -Descending | Select-Object -First 1).Name

$qtIncCandidates = @(
    "C:\Qt\Qt6\6.8.3\msvc2022_64\include",
    "C:\Qt\6.8.3\msvc2022_64\include"
)
$qtIncludeDir = $null
foreach ($d in $qtIncCandidates) {
    if (Test-Path $d) { $qtIncludeDir = (Resolve-Path $d).Path.Replace('\', '/'); break }
}
if (-not $qtIncludeDir) { throw "Qt include directory not found among candidates: $($qtIncCandidates -join '; ')" }

$compilerArgs = [System.Collections.Generic.List[string]]::new()
$compilerArgs.Add("-std=c++17")
$compilerArgs.Add("-fms-extensions")
$compilerArgs.Add("-fms-compatibility")
$compilerArgs.Add("-fdelayed-template-parsing")

foreach ($d in @("-DUNICODE", "-D_UNICODE", "-DWIN32", "-DWIN64", "-DQT_CORE_LIB", "-DQT_GUI_LIB", "-DQT_WIDGETS_LIB", "-DQT_NETWORK_LIB", "-DSINGLE_APPLICATION", "-DVIGEM_CLIENT_SUPPORT", "-DFAKERINPUT_SUPPORT")) {
    $compilerArgs.Add($d)
}
if ($Diagnostic) {
    $compilerArgs.Add('-DLOGOUT_TOFILE')
    $compilerArgs.Add('-DDEBUG_LOGOUT_ON')
}

$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper")).Replace('\', '/'))
$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper/QJoysticks/include")).Replace('\', '/'))
$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper/QJoysticks/src")).Replace('\', '/'))
$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper/QJoysticks/lib/SDL/include")).Replace('\', '/'))
$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper/QSimpleUpdater/include")).Replace('\', '/'))
$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper/ViGEm/include")).Replace('\', '/'))
$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper/Interception/include")).Replace('\', '/'))
$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper/libusb/include")).Replace('\', '/'))
$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper/FakerInput/include")).Replace('\', '/'))
$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper/orderedmap")).Replace('\', '/'))
$compilerArgs.Add(("-I" + (Join-Path $repoRoot "QKeyMapper/GamepadMotion")).Replace('\', '/'))
$compilerArgs.Add(("-I" + $BuildDirectory).Replace('\', '/'))
$compilerArgs.Add("-I$qtIncludeDir")
$compilerArgs.Add("-I$qtIncludeDir/QtCore")
$compilerArgs.Add("-I$qtIncludeDir/QtGui")
$compilerArgs.Add("-I$qtIncludeDir/QtWidgets")
$compilerArgs.Add("-I$qtIncludeDir/QtNetwork")
$compilerArgs.Add("-I$qtIncludeDir/QtSvg")
foreach ($systemInclude in @(
    "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/$msvcVer/include",
    "C:/Program Files (x86)/Windows Kits/10/include/$sdkVer/ucrt",
    "C:/Program Files (x86)/Windows Kits/10/include/$sdkVer/um",
    "C:/Program Files (x86)/Windows Kits/10/include/$sdkVer/shared"
)) {
    $compilerArgs.Add('-isystem')
    $compilerArgs.Add($systemInclude)
}

$dbEntries = foreach ($src in $sourceFiles) {
    [PSCustomObject]@{
        directory = $repoRoot
        file = $src
        arguments = @("clang++", $src) + $compilerArgs.ToArray()
    }
}
$dbArray = @($dbEntries)
"[" + (($dbArray | ForEach-Object { $_ | ConvertTo-Json -Depth 5 -Compress }) -join ",`n") + "]" | Set-Content -LiteralPath $compileDbPath -Encoding UTF8

Write-Host "Running static analysis on $($sourceFiles.Count) files..."

$tidyLines = [System.Collections.Generic.List[string]]::new()
$clazyLines = [System.Collections.Generic.List[string]]::new()

foreach ($src in $sourceFiles) {
    $relName = [System.IO.Path]::GetFileName($src)
    Write-Host "[Clang-Tidy] $relName"
    $prev = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $tOut = @(& $clangTidy "--config-file=$clangTidyConfig" "--header-filter=$headerFilter" "-p=$reportDirectory" $src 2>&1 | ForEach-Object { $_.ToString() })
    } finally {
        $ErrorActionPreference = $prev
    }
    foreach ($l in $tOut) { $tidyLines.Add($l) }

    Write-Host "[Clazy] $relName"
    $prev = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $cOut = @(& $clazy "-checks=$clazyChecks" "--header-filter=$headerFilter" "-p=$reportDirectory" $src 2>&1 | ForEach-Object { $_.ToString() })
    } finally {
        $ErrorActionPreference = $prev
    }
    foreach ($l in $cOut) { $clazyLines.Add($l) }
}

$tidyLines.ToArray() | Set-Content -LiteralPath $clangTidyReport -Encoding UTF8
$clazyLines.ToArray() | Set-Content -LiteralPath $clazyReport -Encoding UTF8

$projectSourcePrefix = [regex]::Escape(([IO.Path]::GetFullPath((Join-Path $repoRoot 'QKeyMapper'))).Replace('\', '/') + '/')
$diagPattern = '(?i)' + $projectSourcePrefix + '(?!(GamepadMotion|Interception|ViGEm|FakerInput|libusb|QJoysticks|QSimpleUpdater)/).*:\d+:\d+:\s+(warning|error):'
$diagnostics = foreach ($line in @($tidyLines + $clazyLines)) {
    $norm = ($line -replace '\((\d+),(\d+)\):\s+', ':$1:$2: ').Replace('\', '/')
    if ($norm -match $diagPattern) { $norm }
}
$diagnostics = @($diagnostics | Sort-Object -Unique)

$summary = @(
    "Clang-Tidy: $clangTidy"
    "Clazy: $clazy"
    "Target Files: $($sourceFiles.Count)"
    "Unique Diagnostics in Project: $($diagnostics.Count)"
    ""
) + $diagnostics

$summary | Set-Content -LiteralPath $summaryReport -Encoding UTF8
Write-Host "Static analysis report: $summaryReport"
Write-Host "Diagnostics found: $($diagnostics.Count)"
if (@($tidyLines + $clazyLines | Where-Object { $_ -match '(^error:|fatal error:|:\s+error:)' }).Count -gt 0) {
    throw 'Static analysis encountered compiler/tool errors. See the full reports.'
}
if ($diagnostics.Count -gt 0) { throw 'Static analysis found diagnostics in project code. See summary.txt.' }
