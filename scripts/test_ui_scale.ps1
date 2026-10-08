[CmdletBinding()]
param([string]$QtRoot = 'C:\Qt\6.8.3\msvc2022_64', [switch]$AddressSanitizer)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $repoRoot ('out\ui-scale-test-' + (Split-Path -Leaf (Split-Path -Parent $QtRoot)) + $(if ($AddressSanitizer) { '-asan' }))
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$vcvars = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
$qmake = Join-Path $QtRoot 'bin\qmake.exe'
$project = Join-Path $repoRoot 'QKeyMapper\tests\ui_scale_test.pro'
$jom = 'C:\Qt\Tools\QtCreator\bin\jom\jom.exe'
$flags = if ($AddressSanitizer) { ' "QMAKE_CXXFLAGS+=/fsanitize=address" "QMAKE_LFLAGS+=/INCREMENTAL:NO"' } else { '' }
$command = 'call "{0}" >nul && cd /d "{1}" && "{2}" "{3}"{4} && "{5}" -j4 && set "PATH={6}\bin;!PATH!" && release\QKeyMapperUiScaleTest.exe' -f $vcvars, $buildDirectory, $qmake, $project, $flags, $jom, $QtRoot
& $env:ComSpec /d /v:on /s /c $command
if ($LASTEXITCODE -ne 0) { throw "UI scale checks failed: exit $LASTEXITCODE" }
