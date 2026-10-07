[CmdletBinding()]
param(
    [ValidateSet("Release")]
    [string]$Configuration = "Release",
    [switch]$Clean,
    [ValidateRange(1, 256)]
    [int]$Jobs = [System.Environment]::ProcessorCount
)

$ErrorActionPreference = "Stop"

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
    "C:\Qt\Qt6\5.15.2\msvc2019_64\bin\qmake.exe",
    "C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe"
)
$jomCandidates = @(
    "C:\Qt\Qt6\Tools\QtCreator\bin\jom\jom.exe",
    "C:\Qt\Tools\QtCreator\bin\jom\jom.exe"
)

$vcvars = Resolve-Tool $vcvarsCandidates "vcvars64.bat"
$qmake = Resolve-Tool $qmakeCandidates "qmake.exe"
$jom = Resolve-Tool $jomCandidates "jom.exe"
$qtVersion = (& $qmake -query QT_VERSION | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $qtVersion -ne "5.15.2") {
    throw "Qt 5.15.2 is required; selected qmake reports '$qtVersion': $qmake"
}
$qtArch = Get-Content -LiteralPath (Join-Path (Split-Path -Parent (Split-Path -Parent $qmake)) 'mkspecs\qconfig.pri') | Select-String '^QT_ARCH\s*=\s*x86_64\s*$'
if (-not $qtArch) { throw "The Qt 5.15.2 kit must target x86_64: $qmake" }
Write-Output "QtVersion=$qtVersion QMake=$qmake"

if (-not (Test-Path -LiteralPath $proFile)) {
    throw "Project file not found: $proFile"
}

$buildDirName = "out\build_qt5_5152"
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

$qmakeConfig = @("CONFIG-=debug", "CONFIG+=release")

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
