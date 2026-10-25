[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ExecutablePath,
    [Parameter(Mandatory)][string]$DesktopName,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateRange(60,600)][int]$MaximumSeconds = 240,
    [switch]$AllowDefaultDesktop
)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$root = [IO.Path]::GetFullPath((Join-Path $repo 'out/mcp_product')) + [IO.Path]::DirectorySeparatorChar
$executable = (Resolve-Path -LiteralPath $ExecutablePath).Path
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
$engineRoot = [IO.Path]::GetFullPath((Join-Path $repo 'out/mcp_product/engine61')) + [IO.Path]::DirectorySeparatorChar
$engineDefault = $AllowDefaultDesktop -and $DesktopName -eq 'Default' -and
    $executable.StartsWith($engineRoot, [StringComparison]::OrdinalIgnoreCase) -and
    ($output + [IO.Path]::DirectorySeparatorChar).StartsWith($engineRoot, [StringComparison]::OrdinalIgnoreCase) -and
    [IO.Path]::GetFileName($executable) -match '^QKeyMapperMcpAcceptance_engine61_(baseline|current)\.exe$'
if (-not $executable.StartsWith($root, [StringComparison]::OrdinalIgnoreCase) -or
    -not ($output + [IO.Path]::DirectorySeparatorChar).StartsWith($root, [StringComparison]::OrdinalIgnoreCase) -or
    [IO.Path]::GetFileName($executable) -notmatch '^QKeyMapperMcpAcceptance_[a-zA-Z0-9_]+\.exe$' -or
    ($DesktopName -notmatch '^QkmProduct_[a-f0-9]{32}$' -and -not $engineDefault) -or
    ($AllowDefaultDesktop -and -not $engineDefault)) { throw 'Select an owned product runtime and authorized desktop.' }
. (Join-Path $repo 'scripts/test_process.ps1')

function Write-State {
    param([string]$Name, [hashtable]$Value)
    $file = Join-Path $output $Name
    [IO.File]::WriteAllText($file + '.new', ($Value | ConvertTo-Json), [Text.UTF8Encoding]::new($false))
    Move-Item -LiteralPath ($file + '.new') -Destination $file
}

$testProcess = $null
try {
    $testProcess = Start-QkmTestProcess -ExecutablePath $executable -UseGsudo -DesktopName $DesktopName
    $deadline = [DateTime]::UtcNow.AddSeconds(25)
    do {
        $window = Get-QkmMainWindowHandle -ProcessId $testProcess.Process.Id -DesktopName $DesktopName
        if ($window -ne [IntPtr]::Zero) { break }
        if ($testProcess.Process.HasExited) { throw 'Product exited before its main window appeared.' }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($window -eq [IntPtr]::Zero) { throw 'Product main window did not appear.' }
    Write-State 'started.json' @{ pid = $testProcess.Process.Id; executable = $executable; desktop = $DesktopName;
        start_ticks = [string]$testProcess.StartTicks;
        created = [string]$testProcess.Process.StartTime.ToUniversalTime().ToFileTimeUtc(); window = $window.ToInt64() }
    # A lost controller must not leave this owned administrator product running indefinitely.
    $deadline = [DateTime]::UtcNow.AddSeconds($MaximumSeconds)
    while (-not (Test-Path -LiteralPath (Join-Path $output 'stop')) -and [DateTime]::UtcNow -lt $deadline) {
        if ($testProcess.Process.HasExited) { throw 'Product exited unexpectedly.' }
        Start-Sleep -Milliseconds 100
    }
} finally {
    if ($null -ne $testProcess) {
        $exitCode = Close-QkmTestProcess $testProcess
        Write-State 'closed.json' @{ pid = $testProcess.Process.Id; exit_code = $exitCode }
        if ($null -eq $exitCode -or $exitCode -ne 0) { throw 'Product did not return exit code 0.' }
    }
}
