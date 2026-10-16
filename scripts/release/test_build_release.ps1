#requires -Version 7.2
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'build_release.ps1')
$testRoot = Join-Path $PSScriptRoot '_test_work'
Reset-WorkDirectory $testRoot $PSScriptRoot
$script:CommandLogPath = Join-Path $testRoot 'native.log'
$passed = 0
function Check([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw "FAILED: $Message" }
    $script:passed++
    Write-Host "PASS: $Message"
}
function Expect-Failure([scriptblock]$Action, [string]$Pattern, [string]$Message) {
    $failure = $null
    try { & $Action | Out-Null } catch { $failure = $_.Exception.Message }
    Check ($null -ne $failure -and $failure -match $Pattern) "$Message (diagnostic=$failure)"
}
function Put([string]$Path, [string]$Content) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $Path) -Force | Out-Null
    [IO.File]::WriteAllText($Path, $Content, [Text.UTF8Encoding]::new($false))
}
function Git([string]$Repo, [string[]]$ArgsList) { Invoke-Git $Repo $ArgsList | Out-Null }

$encodingMessage = '未找到 PDB 文件，正在链接对象，如同没有调试信息一样'
$encoded = [IO.MemoryStream]::new([Text.Encoding]::GetEncoding(936).GetBytes($encodingMessage))
try { Check ((Convert-NativeOutput $encoded) -ceq $encodingMessage) 'CP936 compiler diagnostics decode without replacement characters' } finally { $encoded.Dispose() }
$encoded = [IO.MemoryStream]::new([Text.Encoding]::UTF8.GetBytes('UTF-8 中文文件名'))
try { Check ((Convert-NativeOutput $encoded) -ceq 'UTF-8 中文文件名') 'UTF-8 Git output remains intact' } finally { $encoded.Dispose() }

$seed = Join-Path $testRoot 'seed'
New-Item -ItemType Directory -Path $seed | Out-Null
Git $seed @('init', '-b', 'master')
Git $seed @('config', 'user.email', 'release-test@example.invalid')
Git $seed @('config', 'user.name', 'Release self-check')
Git $seed @('config', 'core.autocrlf', 'false')
Put (Join-Path $testRoot 'fixture-ignore') ''
Git $seed @('config', 'core.excludesfile', (Join-Path $testRoot 'fixture-ignore'))
Put "$seed/QKeyMapper/QKeyMapper.rc" 'VALUE "ProductVersion", "v1.5.0.20261016"'
Put "$seed/tracked.txt" 'initial'
Git $seed @('add', '.')
Git $seed @('commit', '-m', 'fixture baseline')
$variants = @()
foreach ($name in @('Qt6_x64', 'Qt5_x64', 'Qt5_x86')) {
    $repo = Join-Path $testRoot $name
    Invoke-Native 'git.exe' @('clone', '--quiet', $seed, $repo) | Out-Null
    Git $repo @('config', 'user.email', 'release-test@example.invalid')
    Git $repo @('config', 'user.name', 'Release self-check')
    Git $repo @('config', 'core.autocrlf', 'false')
    Git $repo @('config', 'core.excludesfile', (Join-Path $testRoot 'fixture-ignore'))
    $variants += [pscustomobject]@{ Name = $name; Repository = $repo }
}
$source = Assert-SynchronizedSources $variants
Check ($source.Version -ceq 'v1.5.0.20261016') 'clean synchronized sources'
$repo = $variants[0].Repository
Put "$repo/untracked.txt" 'untracked'
Expect-Failure { Assert-Repository $variants[0] } '未提交改动.*|untracked.txt' 'untracked file blocks release'
Remove-Item -LiteralPath "$repo/untracked.txt"
Put "$repo/tracked.txt" 'modified'
Expect-Failure { Assert-Repository $variants[0] } '未提交改动' 'unstaged change blocks release'
Git $repo @('add', 'tracked.txt')
Expect-Failure { Assert-Repository $variants[0] } '未提交改动' 'staged change blocks release'
Git $repo @('commit', '-m', 'local commit')
Expect-Failure { Assert-SynchronizedSources $variants } 'HEAD 不等于 origin/master' 'local ahead commit blocks release'

Put "$seed/tracked.txt" 'remote modification'
Git $seed @('add', '.')
Git $seed @('commit', '-m', 'remote commit')
Expect-Failure { Invoke-Git $repo @('pull', '--ff-only', 'origin', 'master') } '命令失败' 'diverged history rejects fast-forward pull'
Git $repo @('reset', '--hard', 'origin/master')
foreach ($variant in $variants | Select-Object -Skip 1) { Git $variant.Repository @('pull', '--ff-only', 'origin', 'master') }
$null = Assert-SynchronizedSources $variants
Git $repo @('switch', '-c', 'fixture-other-branch')
Expect-Failure { Assert-Repository $variants[0] } '必须位于 master' 'wrong branch blocks release'
Git $repo @('switch', 'master')
Put "$repo/QKeyMapper/QKeyMapper.rc" 'VALUE "ProductVersion", "v1.5.0.20260230"'
Expect-Failure { Get-SourceVersion $repo } '日期无效' 'invalid date rejected'
Put "$repo/QKeyMapper/QKeyMapper.rc" 'VALUE "ProductVersion", "v1.5.0.20261017"'
Check ((Get-SourceVersion $repo) -cne (Get-SourceVersion $seed)) 'version discrepancy detectable'
Git $repo @('restore', 'QKeyMapper/QKeyMapper.rc')
Expect-Failure { Invoke-Native $env:ComSpec @('/d', '/s', '/c', 'exit /b 7') } '退出码 7' 'native failure exit code preserved'
$warningVariant = [pscustomobject]@{ Name = 'Qt5_x64'; QtVersion = '5.15.2'; QtRoot = 'C:/Qt/Qt6/5.15.2/msvc2019_64'; BuildDirectory = Join-Path $testRoot 'warning-fixture' }
Put "$($warningVariant.BuildDirectory)/Makefile.Release" 'LIBS = C:\Qt\Qt6\5.15.2\msvc2019_64\lib\qtmain.lib'
$warningLog = Join-Path $testRoot 'warning.log'
Put $warningLog "qtmain.lib(qtmain_win.obj) : warning LNK4099: PDB 'qtmain.pdb' was not found; linking object as if no debug info"
Assert-BuildWarnings $warningLog $warningVariant
Check $true 'exact official Qt5 x64 vendor PDB exception accepted'
Put $warningLog "qtmain.lib(qtmain_win.obj) : warning LNK4099: $encodingMessage"
Assert-BuildWarnings $warningLog $warningVariant
Check $true 'localized official vendor PDB warning accepted'
$warningVariant.Name = 'Qt5_x86'
Expect-Failure { Assert-BuildWarnings $warningLog $warningVariant } '未获认可' 'variant and approved kit must match'
$warningVariant.QtRoot = 'C:/Qt/Qt6/5.15.2/msvc2019'
Put "$($warningVariant.BuildDirectory)/Makefile.Release" 'LIBS = C:\Qt\Qt6\5.15.2\msvc2019\lib\qtmain.lib'
Assert-BuildWarnings $warningLog $warningVariant
Check $true 'user-approved exact official Qt5 x86 vendor PDB exception accepted'
Put $warningLog "other.lib(other.obj) : warning LNK4099: $encodingMessage"
Expect-Failure { Assert-BuildWarnings $warningLog $warningVariant } '未获认可' 'unrelated library PDB warning still blocks release'
Put $warningLog 'source.cpp(12): warning C4100: unused parameter'
Expect-Failure { Assert-BuildWarnings $warningLog $warningVariant } '未获认可' 'project compiler warning blocks release'
Expect-Failure { Reset-WorkDirectory $testRoot $testRoot } '操作路径必须' 'cleanup refuses boundary root'
$linkTarget = Join-Path $testRoot 'junction-target'
New-Item -ItemType Directory -Path $linkTarget | Out-Null
$junction = Join-Path $testRoot 'junction'
New-Item -ItemType Junction -Path $junction -Target $linkTarget | Out-Null
Expect-Failure { Reset-WorkDirectory $junction $testRoot } '联接|链接' 'cleanup refuses junction'
Remove-Item -LiteralPath $junction

$baseline = Join-Path $testRoot 'baseline'
$work = Join-Path $testRoot 'work'
$release = Join-Path $testRoot 'release'
$stage = Join-Path $work 'staging/QKeyMapper_Qt6_x64'
Put "$baseline/映射表模版/keep.ini" 'template'
Put "$baseline/keymapdata.ini" 'private runtime data'
Put "$baseline/sound.wav" 'runtime wav'
Put "$baseline/log/old.dmp" 'runtime crash'
Put "$baseline/utils/script/deleted.ahk" 'removed script'
Put "$baseline/utils/ahk.exe" 'maintained dependency'
Put "$baseline/old.pdb" 'symbols'
Put "$release/QKeyMapper.exe" 'new main'
Put "$release/QKeyMapperCrashReporter.exe" 'new helper'
Put "$repo/QKeyMapper/ahk_utils/script/current.ahk" 'new script'
Put "$repo/QKeyMapper/ahk_utils/readme.txt" 'readme'
Put "$repo/QKeyMapper_Keyname.pdf" 'keys'
Put "$repo/QKeyMapper_Parameter_Format_Quick_Reference.pdf" 'parameters'
Prepare-Package $baseline $stage $repo $release $work
Check (Test-Path -LiteralPath "$stage/映射表模版/keep.ini") 'mapping template preserved'
Check (-not (Test-Path -LiteralPath "$stage/keymapdata.ini")) 'runtime INI removed'
Check (-not (Test-Path -LiteralPath "$stage/sound.wav")) 'runtime WAV removed'
Check (-not (Test-Path -LiteralPath "$stage/log")) 'crash logs removed'
Check (-not (Test-Path -LiteralPath "$stage/utils/script/deleted.ahk")) 'deleted script not retained'
Check ((Get-Content -LiteralPath "$stage/utils/script/current.ahk") -ceq 'new script') 'source scripts deployed'
Check (Test-Path -LiteralPath "$baseline/keymapdata.ini") 'maintained baseline untouched'
Check (-not (Test-Path -LiteralPath "$stage/old.pdb")) 'PDB excluded'
$zip = Join-Path $work 'QKeyMapper_v1.5.0.20261016_Qt6_x64.zip'
[IO.Compression.ZipFile]::CreateFromDirectory($stage, $zip, [IO.Compression.CompressionLevel]::Optimal, $true)
Assert-PackageZip $zip $stage
Check $true 'ZIP root, contents, EXE hashes and updater name checked'
Expect-Failure { Assert-PackageZip "$work/invalid.zip" $stage } '文件名不符合' 'invalid updater filename rejected'
Put "$stage/QKeyMapper.exe" 'bad main'
Expect-Failure { Assert-PackageZip $zip $stage } '不一致' 'same-length stale ZIP EXE rejected by hash'

$packageRoot = Join-Path $testRoot 'publish-fixture'
$publishWork = Join-Path $packageRoot '_work'
$pending = Join-Path $publishWork 'pending'
$oldVersion = 'v1.3.8.20260926'
$newVersion = 'v1.5.0.20261016'
foreach ($variant in $variants) {
    Put "$packageRoot/latest_release/QKeyMapper_${oldVersion}_$($variant.Name).zip" "old-$($variant.Name)"
    Put "$pending/QKeyMapper_${newVersion}_$($variant.Name).zip" "new-$($variant.Name)"
    foreach ($name in $script:SymbolNames) {
        Put "$packageRoot/$($variant.Name)_dump_analyze/$name" 'old symbol'
        Put "$pending/$($variant.Name)_dump_analyze/$name" 'new symbol'
    }
}
Put "$packageRoot/latest_release/diagnostic/keep.txt" 'preserve diagnostic'
Put "$packageRoot/latest_release/other.zip" 'unrelated zip'
Put "$packageRoot/Qt6_x64_dump_analyze/windbg.txt" 'preserve tool'
$lockedSource = "$pending/QKeyMapper_${newVersion}_Qt5_x64.zip"
$lock = [IO.File]::Open($lockedSource, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
try { Expect-Failure { Publish-Release $packageRoot $pending $publishWork $variants $newVersion } '已恢复上一套' 'publish failure triggers rollback after partial replacement' }
finally { $lock.Dispose() }
foreach ($variant in $variants) {
    Check ((Get-Content -LiteralPath "$packageRoot/latest_release/QKeyMapper_${oldVersion}_$($variant.Name).zip") -ceq "old-$($variant.Name)") "rollback restores $($variant.Name) ZIP"
    Check (-not (Test-Path -LiteralPath "$packageRoot/latest_release/QKeyMapper_${newVersion}_$($variant.Name).zip")) "rollback removes $($variant.Name) new ZIP"
    Check ((Get-Content -LiteralPath "$packageRoot/$($variant.Name)_dump_analyze/QKeyMapper.exe") -ceq 'old symbol') "rollback restores $($variant.Name) symbols"
}
Publish-Release $packageRoot $pending $publishWork $variants $newVersion
Check (@(Get-ChildItem -LiteralPath "$packageRoot/latest_release" -File | Where-Object { $_.Name -cmatch $script:AssetPattern }).Count -eq 3) 'exactly three formal ZIPs published'
Check (Test-Path -LiteralPath "$packageRoot/latest_release/diagnostic/keep.txt") 'diagnostic folder preserved'
Check (Test-Path -LiteralPath "$packageRoot/latest_release/other.zip") 'unrelated ZIP preserved'
Check ((Get-Content -LiteralPath "$packageRoot/Qt6_x64_dump_analyze/QKeyMapperCrashReporter.pdb") -ceq 'new symbol') 'helper symbols published'
Check (Test-Path -LiteralPath "$packageRoot/Qt6_x64_dump_analyze/windbg.txt") 'analysis tool preserved'
Check (Test-Path -LiteralPath "$publishWork/previous_release/latest_release/QKeyMapper_${oldVersion}_Qt6_x64.zip") 'previous release backup retained'
Publish-Release $packageRoot $pending $publishWork $variants $newVersion
Check (@(Get-ChildItem -LiteralPath "$publishWork/previous_release/latest_release" -File).Count -eq 3) 'repeat publication keeps exactly one backup generation'
Check (-not (Test-Path -LiteralPath "$publishWork/previous_release/latest_release/QKeyMapper_${oldVersion}_Qt6_x64.zip")) 'older backup generation does not accumulate'

foreach ($case in @('missing-script', 'parse-error', 'native-failure', 'missing-powershell')) {
    $folder = Join-Path $testRoot "launcher-$case"
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'build_release.bat') -Destination $folder
    switch ($case) {
        'parse-error' { Put "$folder/build_release.ps1" 'if (' }
        'native-failure' { Put "$folder/build_release.ps1" 'Write-Host "fixture native failure"; exit 7' }
        'missing-powershell' { Put "$folder/build_release.ps1" 'exit 0' }
    }
}
$dirtyLauncher = Join-Path $testRoot 'launcher-dirty-repository'
New-Item -ItemType Directory -Path $dirtyLauncher -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'build_release.bat') -Destination $dirtyLauncher
$fixtureScript = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'build_release.ps1'))
$realRepositories = @('F:/work/code/mygit_hub/QKeyMapper', 'F:/work/code/mygit_hub/QKeyMapper_Qt5', 'F:/work/code/mygit_hub/QKeyMapper_Qt5_32bit/QKeyMapper_Qt5_x86')
for ($index = 0; $index -lt 3; $index++) {
    $fixtureScript = $fixtureScript.Replace("Repository = '$($realRepositories[$index])'", "Repository = '$($variants[$index].Repository)'")
}
Put (Join-Path $dirtyLauncher 'build_release.ps1') $fixtureScript
Write-Host "`n$passed checks passed. Launcher fixtures: $testRoot"
