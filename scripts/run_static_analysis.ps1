[CmdletBinding()]
param(
    [string[]]$Files,
    [string]$BuildDirectory,
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
    param([string]$Explicit, [string]$DefaultPath, [string]$CmdName)
    if ($Explicit -and (Test-Path $Explicit)) { return (Resolve-Path $Explicit).Path }
    if (Test-Path $DefaultPath) { return (Resolve-Path $DefaultPath).Path }
    $cmd = Get-Command $CmdName -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw "Tool not found: $CmdName (expected at $DefaultPath)"
}

$clangTidy = Resolve-Tool $ClangTidyPath "C:\Qt\Tools\QtCreator\bin\clang\bin\clang-tidy.exe" "clang-tidy.exe"
$clazy = Resolve-Tool $ClazyPath "C:\Qt\Tools\QtCreator\bin\clang\bin\clazy-standalone.exe" "clazy-standalone.exe"

# Generate or load compile_commands.json
$compileDbPath = Join-Path $reportDirectory "compile_commands.json"
New-Item -ItemType Directory -Force -Path $reportDirectory | Out-Null

$sourceFiles = @()
if ($Files -and $Files.Count -gt 0) {
    foreach ($f in $Files) {
        if (Test-Path $f) {
            $sourceFiles += (Resolve-Path $f).Path
        }
    }
} else {
    # Default to main project cpp files
    $sourceFiles = @(Get-ChildItem -Path (Join-Path $repoRoot "QKeyMapper") -Filter "*.cpp" |
        Where-Object { $_.Name -notmatch '^(moc_|qrc_|ui_)' } |
        ForEach-Object { $_.FullName })
}

$vsDir = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC"
$msvcVer = (Get-ChildItem -Path $vsDir -Directory | Sort-Object Name -Descending | Select-Object -First 1).Name
$sdkVer = "10.0.19041.0"

$compilerArgs = [System.Collections.Generic.List[string]]::new()
$compilerArgs.Add("-std=c++17")
$compilerArgs.Add("-fms-extensions")
$compilerArgs.Add("-fms-compatibility")
$compilerArgs.Add("-fdelayed-template-parsing")

foreach ($d in @("-DUNICODE", "-D_UNICODE", "-DWIN32", "-DWIN64", "-DQT_CORE_LIB", "-DQT_GUI_LIB", "-DQT_WIDGETS_LIB", "-DQT_NETWORK_LIB", "-DSINGLE_APPLICATION", "-DVIGEM_CLIENT_SUPPORT", "-DFAKERINPUT_SUPPORT")) {
    $compilerArgs.Add($d)
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
$compilerArgs.Add("-IC:/Qt/6.8.3/msvc2022_64/include")
$compilerArgs.Add("-IC:/Qt/6.8.3/msvc2022_64/include/QtCore")
$compilerArgs.Add("-IC:/Qt/6.8.3/msvc2022_64/include/QtGui")
$compilerArgs.Add("-IC:/Qt/6.8.3/msvc2022_64/include/QtWidgets")
$compilerArgs.Add("-IC:/Qt/6.8.3/msvc2022_64/include/QtNetwork")
$compilerArgs.Add("-IC:/Qt/6.8.3/msvc2022_64/include/QtSvg")
$compilerArgs.Add("-imsvcC:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/$msvcVer/include")
$compilerArgs.Add("-imsvcC:/Program Files (x86)/Windows Kits/10/include/$sdkVer/ucrt")
$compilerArgs.Add("-imsvcC:/Program Files (x86)/Windows Kits/10/include/$sdkVer/um")
$compilerArgs.Add("-imsvcC:/Program Files (x86)/Windows Kits/10/include/$sdkVer/shared")

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

$diagPattern = '[\\/]QKeyMapper[\\/](?!(GamepadMotion|Interception|ViGEm|FakerInput|libusb|QJoysticks|QSimpleUpdater)[\\/]).*:\d+:\d+:\s+(warning|error):'
$diagnostics = foreach ($line in @($tidyLines + $clazyLines)) {
    $norm = $line -replace '\((\d+),(\d+)\):\s+', ':$1:$2: '
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
