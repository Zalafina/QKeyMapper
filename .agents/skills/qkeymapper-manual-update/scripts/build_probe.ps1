[CmdletBinding()]
param(
    [string]$QtRoot,
    [string]$VcVars,
    [string]$Jom,
    [string]$Python = 'python',
    [ValidateRange(1,64)][int]$Jobs = 4
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..'))
$observer = Join-Path $repo 'out/user_manual/observer'
$build = Join-Path $observer 'build'
$runtime = Join-Path $observer 'runtime'
$assets = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../assets'))

function Find-Tool([string]$Explicit, [string[]]$Candidates, [string]$Name) {
    if ($Explicit) { return (Resolve-Path -LiteralPath $Explicit -ErrorAction Stop).Path }
    foreach ($candidate in $Candidates) { if (Test-Path -LiteralPath $candidate) { return (Resolve-Path -LiteralPath $candidate).Path } }
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    throw "Missing $Name. Pass its path explicitly."
}
$qmakeCandidates = @()
if ($QtRoot) { $qmakeCandidates += Join-Path $QtRoot 'bin/qmake.exe' }
elseif ($env:QTDIR) { $qmakeCandidates += Join-Path $env:QTDIR 'bin/qmake.exe' }
if ($QtRoot) { $qmake = Find-Tool (Join-Path $QtRoot 'bin/qmake.exe') @() 'qmake.exe' }
else { $qmake = Find-Tool '' $qmakeCandidates 'qmake.exe' }
$qtVersion = & $qmake -query QT_VERSION
if ($LASTEXITCODE -ne 0 -or $qtVersion -ne '6.8.3') { throw "Requires Qt6.8.3 x64 MSVC kit, got $qtVersion" }
$qtRootPath = Split-Path -Parent (Split-Path -Parent $qmake)
if (-not $VcVars) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($LASTEXITCODE -ne 0) { throw 'vswhere failed' }
        if ($vs) { $VcVars = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat' }
    }
}
$VcVars = Find-Tool $VcVars @() 'vcvars64.bat'
$Jom = Find-Tool $Jom @((Join-Path $qtRootPath '../../Tools/QtCreator/bin/jom/jom.exe'), (Join-Path $qtRootPath '../../../Tools/QtCreator/bin/jom/jom.exe')) 'jom.exe'
New-Item -ItemType Directory -Force -Path $build,$runtime | Out-Null
& $Python (Join-Path $PSScriptRoot 'prepare_probe.py')
if ($LASTEXITCODE -ne 0) { throw 'Observer entry preparation failed' }

# Import only this process's MSVC environment; qmake arguments remain separate strings.
$environment = & $env:ComSpec /d /s /c "call `"$VcVars`" >nul && set"
if ($LASTEXITCODE -ne 0) { throw 'MSVC environment initialization failed' }
foreach ($line in $environment) { if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1],$Matches[2],'Process') } }
$source = (Join-Path $observer 'main_manual_probe.cpp').Replace('\','/')
$product = (Join-Path $repo 'QKeyMapper').Replace('\','/')
Push-Location $build
try {
    & $qmake (Join-Path $repo 'QKeyMapper/QKeyMapper.pro') -spec win32-msvc 'CONFIG+=release' 'CONFIG-=debug' -after 'TARGET=QkmManualProbe' 'SOURCES-=main.cpp' ('SOURCES+=$$quote(' + $source + ')') ('INCLUDEPATH+=$$quote(' + $assets.Replace('\','/') + ')') ('INCLUDEPATH+=$$quote(' + $product + ')') 'DEFINES-=QKM_REQUIRE_ADMIN' 'DEFINES-=LOGOUT_TOFILE' 'DEFINES-=HOOKSTART_ONSTARTUP'
    if ($LASTEXITCODE -ne 0) { throw 'qmake failed' }
    & $Jom "-j$Jobs"
    if ($LASTEXITCODE -ne 0) { throw 'Observer build failed' }
} finally { Pop-Location }
Copy-Item -LiteralPath (Join-Path $build 'release/QkmManualProbe.exe') -Destination $runtime
& (Join-Path $qtRootPath 'bin/windeployqt.exe') --release --translations zh_CN --no-compiler-runtime (Join-Path $runtime 'QkmManualProbe.exe')
if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed' }
foreach ($relative in @('QJoysticks/lib/SDL/lib/x64/SDL2.dll','ViGEm/lib/x64/ViGEmClient.dll','Interception/lib/x64/interception.dll','libusb/lib/x64/libusb-1.0.dll','FakerInput/lib/x64/FakerInputDll.dll')) {
    $dependency = Join-Path $repo ('QKeyMapper/' + $relative)
    if (-not (Test-Path -LiteralPath $dependency)) { throw "Missing repository runtime: $dependency" }
    Copy-Item -LiteralPath $dependency -Destination $runtime
}
Write-Output "Ordinary Qt6 UI-only observer: $runtime"
