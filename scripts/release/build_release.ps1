#requires -Version 7.2
[CmdletBinding()]
param([switch]$CheckOnly, [ValidateRange(1, 64)][int]$Jobs = 4)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:LogDirectory = $null
$script:CommandLogPath = $null
$script:Stage = '启动'
$script:AssetPattern = '^QKeyMapper_(v\d+\.\d+\.\d+\.\d{8})_(Qt[56]_x(?:64|86))\.zip$'
$script:SymbolNames = @('QKeyMapper.exe', 'QKeyMapper.pdb', 'QKeyMapperCrashReporter.exe', 'QKeyMapperCrashReporter.pdb')

function Write-Stage([string]$Message) {
    $script:Stage = $Message
    Write-Host "`n[$([DateTime]::Now.ToString('HH:mm:ss'))] $Message"
    if ($script:LogDirectory) { Add-Content -LiteralPath (Join-Path $script:LogDirectory 'summary.txt') -Value $Message -Encoding utf8 }
}

function Assert-SafeTree([string]$Path, [string]$Root) {
    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\')
    $prefix = [IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    if (-not $full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { throw "操作路径必须位于指定目录内：$full；边界：$Root" }
    for ($cursor = $full; $cursor; $cursor = [IO.Path]::GetDirectoryName($cursor)) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "拒绝目录联接或符号链接：$cursor"
        }
    }
    if (Test-Path -LiteralPath $full -PathType Container) {
        $links = @(Get-ChildItem -LiteralPath $full -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint })
        if ($links.Count) { throw "目录内存在联接或符号链接：$($links[0].FullName)" }
    }
}

function Reset-WorkDirectory([string]$Path, [string]$Root) {
    Assert-SafeTree $Path $Root
    if (Test-Path -LiteralPath $Path) { Remove-Item -LiteralPath $Path -Recurse -Force }
    New-Item -ItemType Directory -Path $Path -Force | Out-Null
}

function Assert-File([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "缺少必需文件：$Path" }
}

function Convert-NativeOutput([IO.Stream]$Stream) {
    $Stream.Flush()
    $Stream.Position = 0
    $buffer = [IO.MemoryStream]::new()
    try {
        $Stream.CopyTo($buffer)
        try { return [Text.UTF8Encoding]::new($false, $true).GetString($buffer.ToArray()) }
        catch [Text.DecoderFallbackException] {
            $codePage = [int][Microsoft.Win32.Registry]::GetValue('HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Nls\CodePage', 'ACP', 936)
            return [Text.Encoding]::GetEncoding($codePage).GetString($buffer.ToArray())
        }
    } finally { $buffer.Dispose() }
}

function Invoke-Native([string]$File, [string[]]$Arguments, [string]$WorkingDirectory = $PSScriptRoot) {
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $File
    if ($File -ieq $env:ComSpec -and $Arguments.Count -eq 4 -and $Arguments[2] -eq '/c') {
        # cmd.exe uses its own quoting rules, not CommandLineToArgvW escaping.
        $info.Arguments = '/d /s /c "' + $Arguments[3] + '"'
    } else {
        foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) }
    }
    $info.WorkingDirectory = $WorkingDirectory
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.Environment['PATH'] = $env:PATH
    $info.Environment['GIT_TERMINAL_PROMPT'] = '0'
    $info.Environment['GCM_INTERACTIVE'] = 'Never'
    $info.Environment['GIT_OPTIONAL_LOCKS'] = '0'
    $info.Environment['VSLANG'] = '1033'
    $stdout = [IO.MemoryStream]::new()
    $stderr = [IO.MemoryStream]::new()
    $log = $null
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $info
    try {
        if ($script:CommandLogPath) {
            $log = [IO.StreamWriter]::new($script:CommandLogPath, $true, [Text.UTF8Encoding]::new($false))
            $log.AutoFlush = $true
            $log.WriteLine("`n> $File $($Arguments -join ' ')")
            if ($File -ieq $env:ComSpec) {
                $stdout.Dispose()
                $stderr.Dispose()
                $stdout = [IO.FileStream]::new("$script:CommandLogPath.stdout.bin", [IO.FileMode]::Create, [IO.FileAccess]::ReadWrite, [IO.FileShare]::ReadWrite)
                $stderr = [IO.FileStream]::new("$script:CommandLogPath.stderr.bin", [IO.FileMode]::Create, [IO.FileAccess]::ReadWrite, [IO.FileShare]::ReadWrite)
            }
        }
        if (-not $process.Start()) { throw "无法启动：$File" }
        # Drain both pipes concurrently and keep the original compiler bytes before decoding.
        $outputTask = $process.StandardOutput.BaseStream.CopyToAsync($stdout)
        $errorTask = $process.StandardError.BaseStream.CopyToAsync($stderr)
        $process.WaitForExit()
        $null = $outputTask.GetAwaiter().GetResult()
        $null = $errorTask.GetAwaiter().GetResult()
        $output = Convert-NativeOutput $stdout
        $errorText = Convert-NativeOutput $stderr
        if ($log) { $log.WriteLine($output); $log.WriteLine($errorText); $log.WriteLine("ExitCode=$($process.ExitCode)") }
        if ($process.ExitCode -ne 0) {
            $tail = ($output -split '\r?\n' | Select-Object -Last 20) -join "`n"
            throw "命令失败，退出码 $($process.ExitCode)：$File $($Arguments -join ' ')`n$tail`n$errorText"
        }
        [pscustomobject]@{ Output = $output.TrimEnd(); Error = $errorText.TrimEnd() }
    } finally {
        if ($log) { $log.Dispose() }
        $stdout.Dispose()
        $stderr.Dispose()
        $process.Dispose()
    }
}

function Invoke-Git([string]$Repository, [string[]]$Arguments) {
    Invoke-Native 'git.exe' (@('-c', 'core.quotepath=false', '-C', $Repository) + $Arguments)
}

function Assert-Repository($Variant) {
    $repo = $Variant.Repository
    $root = (Invoke-Git $repo @('rev-parse', '--show-toplevel')).Output
    if ([IO.Path]::GetFullPath($root).TrimEnd('\') -ine [IO.Path]::GetFullPath($repo).TrimEnd('\')) { throw "仓库根目录不匹配：$repo；实际：$root" }
    $status = Invoke-Git $repo @('status', '--porcelain=v1', '--untracked-files=all', '--ignore-submodules=none')
    if ($status.Error) { throw "$($Variant.Name) Git 状态检查出现诊断，不能确认工作区干净：$repo`n$($status.Error)" }
    if ($status.Output) { throw "$($Variant.Name) 仓库存在未提交改动：$repo`n$($status.Output)" }
    $branch = (Invoke-Git $repo @('symbolic-ref', '--short', 'HEAD')).Output
    if ($branch -cne 'master') { throw "$($Variant.Name) 必须位于 master，当前为 $branch：$repo" }
    [pscustomobject]@{
        Origin = (Invoke-Git $repo @('remote', 'get-url', 'origin')).Output
        Commit = (Invoke-Git $repo @('rev-parse', 'HEAD')).Output
    }
}

function Get-SourceVersion([string]$Repository) {
    $resource = Join-Path $Repository 'QKeyMapper/QKeyMapper.rc'
    $matchesFound = [regex]::Matches((Get-Content -LiteralPath $resource -Raw), 'VALUE\s+"ProductVersion"\s*,\s*"(v\d+\.\d+\.\d+\.(\d{8}))"')
    if ($matchesFound.Count -ne 1) { throw "ProductVersion 格式不正确或不唯一：$resource" }
    $date = [DateTime]::MinValue
    if (-not [DateTime]::TryParseExact($matchesFound[0].Groups[2].Value, 'yyyyMMdd', [Globalization.CultureInfo]::InvariantCulture, [Globalization.DateTimeStyles]::None, [ref]$date)) {
        throw "ProductVersion 中的日期无效：$resource"
    }
    $matchesFound[0].Groups[1].Value
}

function Assert-SynchronizedSources($Variants, [string]$ExpectedCommit = '') {
    $commits = @()
    $versions = @()
    foreach ($variant in $Variants) {
        $state = Assert-Repository $variant
        $remoteCommit = (Invoke-Git $variant.Repository @('rev-parse', 'refs/remotes/origin/master')).Output
        if ($state.Commit -cne $remoteCommit) { throw "$($variant.Name) HEAD 不等于 origin/master；可能有本地领先提交：$($variant.Repository)" }
        $commits += $state.Commit
        $versions += Get-SourceVersion $variant.Repository
    }
    if (@($commits | Select-Object -Unique).Count -ne 1) { throw '三个仓库提交不一致，请检查远端变化后重新运行。' }
    if (@($versions | Select-Object -Unique).Count -ne 1) { throw "三个仓库版本不一致：$($versions -join ', ')" }
    if ($ExpectedCommit -and $commits[0] -cne $ExpectedCommit) { throw '构建期间源码提交发生变化，停止发布。' }
    foreach ($variant in $Variants) { Assert-File (Join-Path $variant.Repository 'readme/QKeyMapper使用手册.html') }
    [pscustomobject]@{ Commit = $commits[0]; Version = $versions[0] }
}

function Get-ReleaseVariants {
    $vsRoot = 'C:/Program Files (x86)/Microsoft Visual Studio'
    @(
        @{ Name = 'Qt6_x64'; Repository = 'F:/work/code/mygit_hub/QKeyMapper'; Kit = '6.8.3/msvc2022_64'; QtVersion = '6.8.3'; Arch = 'x86_64'; Machine = 0x8664; Vs = '2022'; CompilerArgument = 'amd64' },
        @{ Name = 'Qt5_x64'; Repository = 'F:/work/code/mygit_hub/QKeyMapper_Qt5'; Kit = '5.15.2/msvc2019_64'; QtVersion = '5.15.2'; Arch = 'x86_64'; Machine = 0x8664; Vs = '2019'; CompilerArgument = 'x86_amd64' },
        @{ Name = 'Qt5_x86'; Repository = 'F:/work/code/mygit_hub/QKeyMapper_Qt5_32bit/QKeyMapper_Qt5_x86'; Kit = '5.15.2/msvc2019'; QtVersion = '5.15.2'; Arch = 'i386'; Machine = 0x14c; Vs = '2019'; CompilerArgument = 'x86' }
    ) | ForEach-Object {
        $_.QtRoot = "C:/Qt/Qt6/$($_.Kit)"
        $_.QMake = "$($_.QtRoot)/bin/qmake.exe"
        $_.CompilerEnvironment = "$vsRoot/$($_.Vs)/BuildTools/VC/Auxiliary/Build/vcvarsall.bat"
        $_.BuildDirectory = "$($_.Repository)/out/release_package_build"
        [pscustomobject]$_
    }
}

function Assert-Environment($Variants, [string]$PackageRoot, [string]$Jom) {
    $origin = $null
    foreach ($variant in $Variants) {
        Write-Stage "预检 $($variant.Name)：$($variant.Repository)"
        $state = Assert-Repository $variant
        if ($null -eq $origin) { $origin = $state.Origin }
        if ($state.Origin -cne $origin) { throw "origin 不一致：$($variant.Repository)；$($state.Origin)" }
        foreach ($relative in @('QKeyMapper/QKeyMapper.pro', 'QKeyMapper/diagnostics/diagnostic_artifacts.ps1', 'QKeyMapper/ahk_utils/readme.txt', 'QKeyMapper_Keyname.pdf')) {
            Assert-File (Join-Path $variant.Repository $relative)
        }
        . (Join-Path $variant.Repository 'QKeyMapper/diagnostics/diagnostic_artifacts.ps1')
        if (-not (Test-Path -LiteralPath (Join-Path $variant.Repository 'QKeyMapper/ahk_utils/script') -PathType Container)) { throw "缺少 AHK 脚本目录：$($variant.Repository)" }
        Assert-File $variant.QMake
        Assert-File $variant.CompilerEnvironment
        $qtVersion = (Invoke-Native $variant.QMake @('-query', 'QT_VERSION')).Output
        if ($qtVersion -cne $variant.QtVersion) { throw "Qt 版本不符：$($variant.QMake)；实际：$qtVersion" }
        $config = Get-Content -LiteralPath (Join-Path $variant.QtRoot 'mkspecs/qconfig.pri') -Raw
        if ($config -notmatch "(?m)^QT_ARCH\s*=\s*$([regex]::Escape($variant.Arch))\s*$") { throw "Qt 架构不符：$($variant.QtRoot)" }
        $baseline = Join-Path $PackageRoot "QKeyMapper_$($variant.Name)"
        Assert-SafeTree $baseline $PackageRoot
        foreach ($name in @("Qt$($variant.QtVersion[0])Core.dll", 'platforms/qwindows.dll', 'SDL2.dll', 'FakerInputDll.dll', 'interception.dll', 'libusb-1.0.dll', 'ViGEmClient.dll', 'utils/ahk.exe', 'zipupdater/zipupdater.exe')) { Assert-File (Join-Path $baseline $name) }
        foreach ($name in @("Qt$($variant.QtVersion[0])Core.dll", 'platforms/qwindows.dll', 'SDL2.dll', 'FakerInputDll.dll', 'interception.dll', 'libusb-1.0.dll', 'ViGEmClient.dll')) {
            if ((Get-PeIdentity (Join-Path $baseline $name)).Machine -ne $variant.Machine) { throw "配套 DLL 架构不符：$baseline/$name" }
        }
        $core = (Get-Item -LiteralPath (Join-Path $baseline "Qt$($variant.QtVersion[0])Core.dll")).VersionInfo
        if ($core.ProductVersion -ne "$($variant.QtVersion).0") { throw "配套 Qt DLL 版本不符：$baseline" }
        Assert-SafeTree $variant.BuildDirectory $variant.Repository
    }
    Assert-File $Jom
}

function Assert-BuildWarnings([string]$LogPath, $Variant) {
    $approvedKit = switch ($Variant.Name) {
        'Qt5_x64' { 'C:/Qt/Qt6/5.15.2/msvc2019_64' }
        'Qt5_x86' { 'C:/Qt/Qt6/5.15.2/msvc2019' }
        default { '' }
    }
    $warnings = @(Get-Content -LiteralPath $LogPath | Where-Object { $_ -match '(?i)\bwarning\b|警告' })
    foreach ($warning in $warnings) {
        $vendorPdb = $approvedKit -and $Variant.QtVersion -eq '5.15.2' -and $Variant.QtRoot -eq $approvedKit -and
            $warning -match '(?i)qtmain\.lib\(qtmain_win\.obj\)\s*:\s*warning LNK4099\b' -and
            $warning -match '(?i)PDB' -and $warning -match '(?i)not found|找不到|未找到' -and $warning -match '(?i)without debug|as if no debug|没有调试|无调试'
        if ($vendorPdb) {
            $makefile = Get-Content -LiteralPath (Join-Path $Variant.BuildDirectory 'Makefile.Release') -Raw
            $library = (Join-Path $Variant.QtRoot 'lib/qtmain.lib').Replace('/', '\')
            Assert-File $library
            $vendorPdb = $makefile.Replace('/', '\').Contains($library, [StringComparison]::OrdinalIgnoreCase)
        }
        if (-not $vendorPdb) { throw "存在未获认可的构建警告：$warning`n完整日志：$LogPath" }
    }
}

function Build-Variant($Variant, [string]$Jom, [int]$JobCount) {
    Write-Stage "完整重建 $($Variant.Name)：$($Variant.BuildDirectory)"
    Reset-WorkDirectory $Variant.BuildDirectory $Variant.Repository
    $script:CommandLogPath = Join-Path $script:LogDirectory "$($Variant.Name)-build.log"
    $command = ('call "{0}" {1} && "{2}" "{3}" -spec win32-msvc "CONFIG-=debug" "CONFIG+=release" && "{4}" -j{5} release' -f
        $Variant.CompilerEnvironment, $Variant.CompilerArgument, $Variant.QMake, (Join-Path $Variant.Repository 'QKeyMapper/QKeyMapper.pro'), $Jom, $JobCount)
    Invoke-Native $env:ComSpec @('/d', '/s', '/c', $command) $Variant.BuildDirectory | Out-Null
    Assert-BuildWarnings $script:CommandLogPath $Variant
    foreach ($name in $script:SymbolNames) { Assert-File (Join-Path $Variant.BuildDirectory "release/$name") }
    foreach ($name in @('QKeyMapper', 'QKeyMapperCrashReporter')) {
        $identity = Assert-SymbolPair (Join-Path $Variant.BuildDirectory "release/$name.exe") (Join-Path $Variant.BuildDirectory "release/$name.pdb")
        if ($identity.Machine -ne $Variant.Machine) { throw "EXE 架构不符：$($Variant.Name) $name" }
    }
    $script:CommandLogPath = Join-Path $script:LogDirectory 'commands.log'
}

function Prepare-Package([string]$Baseline, [string]$StagePath, [string]$Repository, [string]$ReleasePath, [string]$WorkRoot) {
    $manual = Join-Path $Repository 'readme/QKeyMapper使用手册.html'
    Assert-File $manual
    Assert-SafeTree $Baseline (Split-Path -Parent $Baseline)
    Reset-WorkDirectory $StagePath $WorkRoot
    Get-ChildItem -LiteralPath $Baseline -Force | Copy-Item -Destination $StagePath -Recurse -Force
    Get-ChildItem -LiteralPath $StagePath -File -Force | Where-Object { $_.Extension -in @('.ini', '.wav', '.log', '.dmp', '.pdb') } | Remove-Item -Force
    Get-ChildItem -LiteralPath $StagePath -File -Force | Where-Object { $_.Name -in @(
        'QKeyMapper使用说明.pdf', 'QKeyMapper映射键参数速查表.pdf', 'QKeyMapper正则匹配规则说明.pdf', '组合键支持按键列表.pdf'
    ) } | Remove-Item -Force
    foreach ($name in @('log', 'crash_dumps')) {
        $runtime = Join-Path $StagePath $name
        if (Test-Path -LiteralPath $runtime) { Assert-SafeTree $runtime $WorkRoot; Remove-Item -LiteralPath $runtime -Recurse -Force }
    }
    foreach ($name in @('QKeyMapper.exe', 'QKeyMapperCrashReporter.exe')) { Copy-Item -LiteralPath (Join-Path $ReleasePath $name) -Destination (Join-Path $StagePath $name) -Force }
    $scripts = Join-Path $StagePath 'utils/script'
    Reset-WorkDirectory $scripts $WorkRoot
    Get-ChildItem -LiteralPath (Join-Path $Repository 'QKeyMapper/ahk_utils/script') -Force | Copy-Item -Destination $scripts -Recurse -Force
    Copy-Item -LiteralPath (Join-Path $Repository 'QKeyMapper/ahk_utils/readme.txt') -Destination (Join-Path $StagePath 'utils/readme.txt') -Force
    Copy-Item -LiteralPath (Join-Path $Repository 'QKeyMapper_Keyname.pdf') -Destination (Join-Path $StagePath 'QKeyMapper按键名称说明.pdf') -Force
    Copy-Item -LiteralPath $manual -Destination (Join-Path $StagePath 'QKeyMapper使用手册.html') -Force
    Get-ChildItem -LiteralPath $StagePath -Recurse -Force -File -Filter '*.pdb' | Remove-Item -Force
}

function Assert-PackageZip([string]$ZipPath, [string]$StagePath) {
    if ([IO.Path]::GetFileName($ZipPath) -cnotmatch $script:AssetPattern) { throw "文件名不符合更新器规则：$ZipPath" }
    Assert-File (Join-Path $StagePath 'QKeyMapper使用手册.html')
    $rootName = Split-Path -Leaf $StagePath
    $zip = [IO.Compression.ZipFile]::OpenRead($ZipPath)
    try {
        $expectedFiles = @(Get-ChildItem -LiteralPath $StagePath -Recurse -File -Force)
        $actualFiles = @($zip.Entries | Where-Object { $_.Name })
        if ($actualFiles.Count -ne $expectedFiles.Count) { throw "ZIP 文件数量不匹配：$ZipPath" }
        foreach ($file in $expectedFiles) {
            $relative = [IO.Path]::GetRelativePath($StagePath, $file.FullName).Replace('\', '/')
            $entry = $zip.GetEntry("$rootName/$relative")
            if ($null -eq $entry -or $entry.Length -ne $file.Length) { throw "ZIP 文件缺失或长度不符：$relative" }
            if ($relative -in @('QKeyMapper.exe', 'QKeyMapperCrashReporter.exe', 'QKeyMapper使用手册.html')) {
                $stream = $entry.Open()
                try { $hash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) } finally { $stream.Dispose() }
                if ($hash -ne (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash) { throw "ZIP 内文件与本次产物不一致：$relative" }
            }
        }
    } finally { $zip.Dispose() }
}

function Publish-Release([string]$PackageRoot, [string]$PendingRoot, [string]$WorkRoot, $Variants, [string]$Version) {
    $latest = Join-Path $PackageRoot 'latest_release'
    $backup = Join-Path $WorkRoot 'previous_release'
    Assert-SafeTree $latest $PackageRoot
    New-Item -ItemType Directory -Path $latest -Force | Out-Null
    $newFiles = @()
    foreach ($variant in $Variants) {
        $zipName = "QKeyMapper_${Version}_$($variant.Name).zip"
        $newFiles += [pscustomobject]@{ Source = Join-Path $PendingRoot $zipName; Destination = Join-Path $latest $zipName }
        $dumpName = "$($variant.Name)_dump_analyze"
        Assert-SafeTree (Join-Path $PackageRoot $dumpName) $PackageRoot
        foreach ($name in $script:SymbolNames) { $newFiles += [pscustomobject]@{ Source = Join-Path $PendingRoot "$dumpName/$name"; Destination = Join-Path $PackageRoot "$dumpName/$name" } }
    }
    foreach ($file in $newFiles) { Assert-File $file.Source }
    $oldFiles = @(Get-ChildItem -LiteralPath $latest -File | Where-Object { $_.Name -cmatch $script:AssetPattern } | ForEach-Object { $_.FullName })
    $oldFiles += @($newFiles | Where-Object { (Test-Path -LiteralPath $_.Destination -PathType Leaf) -and $_.Destination -notin $oldFiles } | ForEach-Object { $_.Destination })
    Reset-WorkDirectory $backup $WorkRoot
    foreach ($old in $oldFiles) {
        $saved = Join-Path $backup ([IO.Path]::GetRelativePath($PackageRoot, $old))
        New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force | Out-Null
        Copy-Item -LiteralPath $old -Destination $saved -Force
    }
    try {
        foreach ($old in @($oldFiles | Where-Object { (Split-Path -Parent $_) -eq $latest })) { Remove-Item -LiteralPath $old -Force }
        foreach ($file in $newFiles) {
            New-Item -ItemType Directory -Path (Split-Path -Parent $file.Destination) -Force | Out-Null
            Copy-Item -LiteralPath $file.Source -Destination $file.Destination -Force
        }
    } catch {
        $publishError = $_.Exception.Message
        $rollbackErrors = @()
        foreach ($file in $newFiles) {
            if ($file.Destination -notin $oldFiles -and (Test-Path -LiteralPath $file.Destination -PathType Leaf)) {
                try { Remove-Item -LiteralPath $file.Destination -Force } catch { $rollbackErrors += $_.Exception.Message }
            }
        }
        foreach ($old in $oldFiles) {
            try { Copy-Item -LiteralPath (Join-Path $backup ([IO.Path]::GetRelativePath($PackageRoot, $old))) -Destination $old -Force } catch { $rollbackErrors += $_.Exception.Message }
        }
        if ($rollbackErrors.Count) { throw "发布失败：$publishError`n恢复未全部完成，请保留备份 $backup`n$($rollbackErrors -join "`n")" }
        throw "发布失败，已恢复上一套文件：$publishError`n备份：$backup"
    }
}

if ($MyInvocation.InvocationName -eq '.') { return }

$releaseLock = $null
$transcribing = $false
try {
    $packageRoot = [IO.Path]::GetFullPath($PSScriptRoot)
    $workRoot = Join-Path $packageRoot '_work'
    $variants = @(Get-ReleaseVariants)
    $jom = 'C:/Qt/Qt6/Tools/QtCreator/bin/jom/jom.exe'
    if (-not $CheckOnly) {
        Assert-SafeTree $workRoot $packageRoot
        New-Item -ItemType Directory -Path $workRoot -Force | Out-Null
        try { $releaseLock = [IO.File]::Open((Join-Path $workRoot 'release.lock'), [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None) }
        catch { throw "无法取得 Release 独占锁，可能已有任务运行：$workRoot；$($_.Exception.Message)" }
        $logs = Join-Path $workRoot 'logs'
        $previous = Join-Path $logs 'previous'
        Reset-WorkDirectory $previous $workRoot
        $current = Join-Path $logs 'latest'
        Assert-SafeTree $current $workRoot
        if (Test-Path -LiteralPath $current) { Get-ChildItem -LiteralPath $current -Force | Move-Item -Destination $previous -Force; Remove-Item -LiteralPath $current -Force }
        New-Item -ItemType Directory -Path $current -Force | Out-Null
        $script:LogDirectory = $current
        $script:CommandLogPath = Join-Path $current 'commands.log'
        Start-Transcript -LiteralPath (Join-Path $current 'console.log') | Out-Null
        $transcribing = $true
    }
    Assert-Environment $variants $packageRoot $jom
    if ($CheckOnly) {
        $localState = Assert-SynchronizedSources $variants
        Write-Host "`n本地只读预检通过：$($localState.Version)，提交 $($localState.Commit)"
        Write-Host '未联网、未拉取、未创建日志、未构建或更新发布文件；远端状态尚未刷新。'
        exit 0
    }
    Write-Stage '拉取三个仓库：仅允许 fast-forward'
    foreach ($variant in $variants) { Invoke-Git $variant.Repository @('pull', '--ff-only', 'origin', 'master') | Out-Null }
    Assert-Environment $variants $packageRoot $jom
    $source = Assert-SynchronizedSources $variants
    Write-Stage "源码版本 $($source.Version)，提交 $($source.Commit)"
    . (Join-Path $variants[0].Repository 'QKeyMapper/diagnostics/diagnostic_artifacts.ps1')
    foreach ($variant in $variants) {
        Build-Variant $variant $jom $Jobs
        $exe = Join-Path $variant.BuildDirectory 'release/QKeyMapper.exe'
        if ([Diagnostics.FileVersionInfo]::GetVersionInfo($exe).ProductVersion -cne $source.Version) { throw "EXE 版本与源码不一致：$exe" }
        Write-Stage "构建通过 $($variant.Name)：$exe"
    }
    Assert-SynchronizedSources $variants $source.Commit | Out-Null
    Write-Stage '隔离部署与压缩包检查'
    $staging = Join-Path $workRoot 'staging'
    $pending = Join-Path $workRoot 'pending'
    Reset-WorkDirectory $staging $workRoot
    Reset-WorkDirectory $pending $workRoot
    foreach ($variant in $variants) {
        $packageName = "QKeyMapper_$($variant.Name)"
        $stagePath = Join-Path $staging $packageName
        $release = Join-Path $variant.BuildDirectory 'release'
        Prepare-Package (Join-Path $packageRoot $packageName) $stagePath $variant.Repository $release $workRoot
        $zipPath = Join-Path $pending "QKeyMapper_$($source.Version)_$($variant.Name).zip"
        [IO.Compression.ZipFile]::CreateFromDirectory($stagePath, $zipPath, [IO.Compression.CompressionLevel]::Optimal, $true)
        Assert-PackageZip $zipPath $stagePath
        $symbols = Join-Path $pending "$($variant.Name)_dump_analyze"
        New-Item -ItemType Directory -Path $symbols -Force | Out-Null
        foreach ($name in $script:SymbolNames) { Copy-Item -LiteralPath (Join-Path $release $name) -Destination (Join-Path $symbols $name) -Force }
    }
    Assert-SynchronizedSources $variants $source.Commit | Out-Null
    Write-Stage '发布三个 ZIP 与配套调试文件（失败则恢复）'
    Publish-Release $packageRoot $pending $workRoot $variants $source.Version
    Write-Stage "Release 成功：$($source.Version)；源码提交：$($source.Commit)"
    foreach ($variant in $variants) { Write-Host (Join-Path $packageRoot "latest_release/QKeyMapper_$($source.Version)_$($variant.Name).zip") }
    Write-Host "日志：$script:LogDirectory"
    exit 0
} catch {
    Write-Host "`nRelease 已中止" -ForegroundColor Red
    Write-Host "阶段：$script:Stage" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    if ($script:LogDirectory) {
        Write-Host "日志：$script:LogDirectory"
        Add-Content -LiteralPath (Join-Path $script:LogDirectory 'summary.txt') -Value "已中止：$($_.Exception.Message)" -Encoding utf8 -ErrorAction Continue
    } else { Write-Host '日志：尚未创建（只读预检或启动失败）；请保留本窗口中的原因。' }
    exit 1
} finally {
    if ($transcribing) { Stop-Transcript | Out-Null }
    if ($releaseLock) { $releaseLock.Dispose() }
}
