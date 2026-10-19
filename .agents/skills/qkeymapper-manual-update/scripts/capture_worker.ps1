param([Parameter(Mandatory)][string]$Request)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..'))
$job = Get-Content -LiteralPath $Request -Raw | ConvertFrom-Json
Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class ManualCaptureDpi {
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
}
'@
$prior = [ManualCaptureDpi]::SetThreadDpiAwarenessContext([IntPtr](-4))
try {
    & (Join-Path $repo 'scripts/capture_ui_snapshots.ps1') -Label $job.label -ProcessId $job.pid -WindowHandle ([long]$job.hwnd) -PrintWindow
    Copy-Item -LiteralPath (Join-Path $repo ("test_snapshots/snapshot_$($job.label).png")) -Destination $job.destination
} finally { [void][ManualCaptureDpi]::SetThreadDpiAwarenessContext($prior) }
