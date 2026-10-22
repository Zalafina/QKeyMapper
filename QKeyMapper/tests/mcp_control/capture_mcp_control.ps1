param([int]$TestPid, [long]$WindowHandle, [string]$Label)
$ErrorActionPreference='Stop'
$repoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
& (Join-Path $repoRoot 'scripts/capture_ui_snapshots.ps1') -ProcessId $TestPid -WindowHandle $WindowHandle -PrintWindow -Label $Label
if ([QkmTestWindows]::Owner([IntPtr]$WindowHandle) -ne $TestPid) { throw 'Window identity changed' }
if (-not [QkmTestWindows]::PostMessage([IntPtr]$WindowHandle,0x10,[IntPtr]::Zero,[IntPtr]::Zero)) { throw 'Owned preview did not accept close' }
