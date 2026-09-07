[CmdletBinding()]
param(
    [Parameter(Mandatory)][int]$MainProcessId,
    [ValidateRange(10, 3600)][int]$Seconds = 120,
    [string]$Label = 'unspecified'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$main = Get-Process -Id $MainProcessId
$helperPath = Join-Path ([IO.Path]::GetDirectoryName($main.Path)) 'QKeyMapperCrashReporter.exe'
$children = @(Get-CimInstance Win32_Process -Filter "ParentProcessId=$MainProcessId" | Where-Object { $_.ExecutablePath -eq $helperPath })
if ($children.Count -gt 1) { throw 'More than one matching helper; select a stable test instance.' }
$processes = @($main)
foreach ($child in $children) { $processes += Get-Process -Id $child.ProcessId }
$cpuBefore = 0.0
foreach ($process in $processes) { $cpuBefore += $process.TotalProcessorTime.TotalSeconds }
$privateSamples = @()
$watch = [Diagnostics.Stopwatch]::StartNew()
try {
    do {
        $privateBytes = 0L
        foreach ($process in $processes) {
            $process.Refresh()
            if ($process.HasExited) { throw 'A measured process exited; discard this round.' }
            $privateBytes += $process.PrivateMemorySize64
        }
        $privateSamples += $privateBytes
        Start-Sleep -Seconds 2
    } while ($watch.Elapsed.TotalSeconds -lt $Seconds)
    $cpuAfter = 0.0
    foreach ($process in $processes) {
        $process.Refresh()
        if ($process.HasExited) { throw 'A measured process exited; discard this round.' }
        $cpuAfter += $process.TotalProcessorTime.TotalSeconds
    }
    [pscustomobject]@{
        Label = $Label
        MainProcessId = $MainProcessId
        HelperCount = $children.Count
        ElapsedSeconds = $watch.Elapsed.TotalSeconds
        SingleCoreCpuPercent = 100.0 * ($cpuAfter - $cpuBefore) / $watch.Elapsed.TotalSeconds
        MeanPrivateMiB = ($privateSamples | Measure-Object -Average).Average / 1MB
        PeakPrivateMiB = ($privateSamples | Measure-Object -Maximum).Maximum / 1MB
    }
}
finally { foreach ($process in $processes) { $process.Dispose() } }
