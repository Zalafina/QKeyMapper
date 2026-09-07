[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$QMakePath,
    [Parameter(Mandatory)][string]$MakePath,
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '../../build/diagnostics-tests'),
    [switch]$Runtime,
    [switch]$LongLifecycleTests
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../diagnostic_artifacts.ps1')
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
New-Item -ItemType Directory -Path $BuildDirectory -Force | Out-Null
Push-Location -LiteralPath $BuildDirectory
try {
    & $QMakePath '-o' 'Makefile' (Join-Path $PSScriptRoot 'diagnostics_test.pro') '-spec' 'win32-msvc'
    if ($LASTEXITCODE -ne 0) { throw 'Test qmake failed.' }
    & $MakePath
    if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
}
finally { Pop-Location }
$outputDirectory = Join-Path $BuildDirectory 'release'
& (Join-Path $PSScriptRoot '../build_crash_reporter.ps1') -QMakePath $QMakePath -MakePath $MakePath `
    -BuildDirectory (Join-Path $BuildDirectory 'reporter') -DeployDirectory $outputDirectory
$testExe = Join-Path $outputDirectory 'QKeyMapperDiagnosticsTest.exe'
& $testExe '--self-test'
if ($LASTEXITCODE -ne 0) { throw 'Native policy self-check failed.' }
$testIdentity = Assert-SymbolPair $testExe (Join-Path $outputDirectory 'QKeyMapperDiagnosticsTest.pdb')
$helperIdentity = Assert-SymbolPair (Join-Path $outputDirectory 'QKeyMapperCrashReporter.exe') (Join-Path $outputDirectory 'QKeyMapperCrashReporter.pdb')
if ($testIdentity.Machine -ne $helperIdentity.Machine) { throw 'Test/helper architecture mismatch.' }
Assert-DbgHelpImport $testExe $false
Assert-DbgHelpImport (Join-Path $outputDirectory 'QKeyMapperCrashReporter.exe') $true
$rejected = $false
try { $null = Assert-SymbolPair $testExe (Join-Path $outputDirectory 'QKeyMapperCrashReporter.pdb') }
catch { $rejected = $true }
if (-not $rejected) { throw 'Mismatched symbols were accepted.' }
Write-Output 'Policy and symbol checks passed.'
if (-not $Runtime) { return }

$caseRoot = Join-Path $BuildDirectory ('cases/' + [Guid]::NewGuid().ToString('N'))
function New-CaseDirectory([string]$Name) {
    $directory = Join-Path $caseRoot $Name
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    foreach ($file in @('QKeyMapperDiagnosticsTest.exe','QKeyMapperDiagnosticsTest.pdb','QKeyMapperCrashReporter.exe','QKeyMapperCrashReporter.pdb')) {
        Copy-Item -LiteralPath (Join-Path $outputDirectory $file) -Destination (Join-Path $directory $file)
    }
    return $directory
}
function Invoke-Case([string]$Mode, [bool]$ExpectDump, [bool]$ExpectCrash, [int]$TimeoutSeconds = 65) {
    $directory = New-CaseDirectory $Mode
    if ($Mode -eq 'missing-import') {
        # Edit only the fresh helper copy in this unique case directory, never the build output or a system DLL.
        $helperCopy = Join-Path $directory 'QKeyMapperCrashReporter.exe'
        $imports = @((Get-PeIdentity -Path $helperCopy -IncludeImports).Imports | Where-Object { $_.Name -ieq 'dbghelp.dll' })
        if ($imports.Count -ne 1) { throw 'Expected one DbgHelp import in the isolated helper.' }
        $missingName = [Guid]::NewGuid().ToString('N').Substring(0, 7) + '.dll'
        $bytes = [IO.File]::ReadAllBytes($helperCopy)
        $replacement = [Text.Encoding]::ASCII.GetBytes($missingName)
        if ($replacement.Length -ne $imports[0].Name.Length) { throw 'Import replacement must keep the image layout.' }
        [Array]::Copy($replacement, 0, $bytes, $imports[0].NameOffset, $replacement.Length)
        [IO.File]::WriteAllBytes($helperCopy, $bytes)
        $modified = (Get-PeIdentity -Path $helperCopy -IncludeImports).Imports
        if (@($modified | Where-Object { $_.Name -eq $missingName }).Count -ne 1 -or
            @($modified | Where-Object { $_.Name -ieq 'dbghelp.dll' }).Count) { throw 'Isolated import replacement failed.' }
        Write-Output "Loader failure fixture: $helperCopy -> $missingName"
    }
    $process = Start-Process -FilePath (Join-Path $directory 'QKeyMapperDiagnosticsTest.exe') -ArgumentList "--$Mode" -WindowStyle Hidden -PassThru
    try {
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            $process.Kill()
            $process.WaitForExit()
            throw "Owned test instance timed out: $Mode"
        }
        if (-not $ExpectCrash -and $process.ExitCode -ne 0) { throw "Test failed: $Mode, exit $($process.ExitCode)" }
    }
    finally { $process.Dispose() }
    Start-Sleep -Seconds 3
    $remaining = @(Get-CimInstance Win32_Process -Filter "Name='QKeyMapperCrashReporter.exe'" | Where-Object {
        $_.ExecutablePath -eq (Join-Path $directory 'QKeyMapperCrashReporter.exe')
    })
    if ($remaining.Count) {
        # This unique case directory contains only helpers owned by the finished test.
        foreach ($helper in $remaining) { Stop-Process -Id $helper.ProcessId -Force -ErrorAction SilentlyContinue }
        throw "Reporter did not exit for $Mode"
    }
    $dumps = @(Get-ChildItem -LiteralPath $directory -Filter 'QKM_Diag_v1_*.dmp' -File -Recurse)
    if (($ExpectDump -and $dumps.Count -ne 1) -or (-not $ExpectDump -and $dumps.Count)) { throw "Unexpected dump count for $Mode : $($dumps.Count)" }
    foreach ($dump in $dumps) {
        $bytes = [IO.File]::ReadAllBytes($dump.FullName)
        if ($bytes.Length -lt 32 -or [Text.Encoding]::ASCII.GetString($bytes, 0, 4) -ne 'MDMP') { throw "Invalid minidump: $($dump.FullName)" }
        $streams = [BitConverter]::ToUInt32($bytes, 8)
        $directoryRva = [BitConverter]::ToUInt32($bytes, 12)
        $hasException = $false
        $hasThreads = $false
        for ($i = 0; $i -lt $streams; $i++) {
            $type = [BitConverter]::ToUInt32($bytes, $directoryRva + 12 * $i)
            if ($type -eq 3) { $hasThreads = $true }
            if ($type -eq 6) { $hasException = $true }
        }
        if (-not $hasThreads -or $hasException -ne $ExpectCrash) { throw "Wrong dump streams: $Mode" }
        $record = Get-Content -LiteralPath ([IO.Path]::ChangeExtension($dump.FullName, '.txt')) -Raw
        if ($record -notmatch '(?m)^status=complete\r?$' -or $record -notmatch '(?m)^source=') { throw "Incomplete metadata: $Mode" }
    }
    Write-Output "Passed: $Mode ($directory)"
}
foreach ($mode in @('normal','kill-helper','exit-without-cleanup')) { Invoke-Case $mode $false $false }
Invoke-Case 'missing-import' $false $false 10
foreach ($mode in @('crash-startup','crash-ui','crash-worker','race-crash')) { Invoke-Case $mode $true $true }
foreach ($mode in @('hang-ui','hang-worker','hang-hook','hang-interception')) { Invoke-Case $mode $true $false }
if ($LongLifecycleTests) {
    Invoke-Case 'hang-startup' $true $false 155
    Invoke-Case 'hang-shutdown' $true $false 95
}

function Invoke-WithDeniedRights([string]$Directory, [Security.AccessControl.FileSystemRights]$Rights, [scriptblock]$Action) {
    $acl = Get-Acl -LiteralPath $Directory
    $saved = $acl.GetSecurityDescriptorSddlForm([Security.AccessControl.AccessControlSections]::Access)
    $sid = [Security.Principal.WindowsIdentity]::GetCurrent().User
    $rule = [Security.AccessControl.FileSystemAccessRule]::new($sid, $Rights, [Security.AccessControl.AccessControlType]::Deny)
    try {
        $acl.AddAccessRule($rule)
        Set-Acl -LiteralPath $Directory -AclObject $acl
        & $Action
    }
    finally {
        $acl.SetSecurityDescriptorSddlForm($saved, [Security.AccessControl.AccessControlSections]::Access)
        Set-Acl -LiteralPath $Directory -AclObject $acl
    }
}
$directory = New-CaseDirectory 'directory-fallback'
$probe = Join-Path $directory 'QKeyMapperDiagnosticsTest.exe'
Invoke-WithDeniedRights $directory ([Security.AccessControl.FileSystemRights]::CreateDirectories) {
    $result = (& $probe '--probe-directory') -join "`n"
    if ($LASTEXITCODE -ne 0 -or $result -notmatch 'fallback=1;directory_error=5;file_error=0;') { throw "Root fallback failed: $result" }
}
$result = (& $probe '--probe-directory') -join "`n"
if ($LASTEXITCODE -ne 0 -or $result -notmatch 'fallback=0;') { throw 'Did not return to log after permissions recovered.' }
Invoke-WithDeniedRights (Join-Path $directory 'log') ([Security.AccessControl.FileSystemRights]::Write) {
    $result = (& $probe '--probe-directory') -join "`n"
    if ($LASTEXITCODE -eq 0 -or $result -notmatch 'fallback=0;') { throw 'An existing unwritable log incorrectly used root fallback.' }
}
$directory = New-CaseDirectory 'directory-both-fail'
$probe = Join-Path $directory 'QKeyMapperDiagnosticsTest.exe'
Invoke-WithDeniedRights $directory ([Security.AccessControl.FileSystemRights]::Write) {
    $result = (& $probe '--probe-directory') -join "`n"
    if ($LASTEXITCODE -eq 0 -or $result -notmatch 'fallback=1;directory_error=5;file_error=5;') { throw "Both-stage errors were not preserved: $result" }
}
$directory = New-CaseDirectory 'combined-retention'
$log = Join-Path $directory 'log'
New-Item -ItemType Directory -Path $log | Out-Null
for ($i = 0; $i -lt 12; $i++) {
    $folder = if ($i % 2) { $directory } else { $log }
    $stem = 'QKM_Diag_v1_20260907_120000_{0:D3}_42_0000000000000001_{0}_Hang' -f $i
    [IO.File]::WriteAllText((Join-Path $folder "$stem.txt"), "QKM-DIAGNOSTICS-V1`ngroup=$stem`n", [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $folder "$stem.partial"), 'isolated retention fixture')
}
[IO.File]::WriteAllText((Join-Path $directory 'keymapdata.ini'), 'leave unchanged')
& (Join-Path $directory 'QKeyMapperDiagnosticsTest.exe') '--cleanup'
if ($LASTEXITCODE -ne 0) { throw 'Combined retention failed.' }
$groups = @(Get-ChildItem -LiteralPath $directory -Filter 'QKM_Diag_v1_*.txt' -File -Recurse)
if ($groups.Count -ne 10 -or [IO.File]::ReadAllText((Join-Path $directory 'keymapdata.ini')) -ne 'leave unchanged' -or -not (Test-Path -LiteralPath (Join-Path $directory 'QKeyMapperDiagnosticsTest.exe'))) { throw 'Retention count or file boundary failed.' }
Write-Output "Directory and retention checks passed. Test artifacts retained at $caseRoot"
Write-Output 'WinDbg source/stack validation, real Qt event-loop behavior and input performance still require separate validation.'
