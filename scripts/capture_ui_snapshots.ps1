[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Label,
    [string]$ExecutablePath
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$sig = @"
using System;
using System.Runtime.InteropServices;

public class WinAPI {
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
}
"@
if (-not ([System.Management.Automation.PSTypeName]'WinAPI').Type) {
    Add-Type -TypeDefinition $sig
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$snapshotDir = Join-Path $repoRoot "test_snapshots"
New-Item -ItemType Directory -Force -Path $snapshotDir | Out-Null

$proc = $null
$startedProcess = $false

if ($ExecutablePath) {
    if (-not (Test-Path $ExecutablePath)) {
        throw "Executable not found: $ExecutablePath"
    }
    $absExe = (Resolve-Path $ExecutablePath).Path
    Write-Host "Starting: $absExe"
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $absExe
    $psi.WorkingDirectory = Split-Path -Parent $absExe
    $proc = [System.Diagnostics.Process]::Start($psi)
    $startedProcess = $true
    Start-Sleep -Seconds 3
} else {
    $proc = Get-Process -Name "QKeyMapper" -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $proc) {
        throw "No running QKeyMapper process found and no ExecutablePath provided."
    }
}

try {
    $hwnd = $proc.MainWindowHandle
    if ($hwnd -eq [IntPtr]::Zero) {
        Start-Sleep -Seconds 2
        $hwnd = (Get-Process -Id $proc.Id).MainWindowHandle
    }
    if ($hwnd -eq [IntPtr]::Zero) {
        throw "Could not obtain MainWindowHandle for process $($proc.Id)."
    }

    [WinAPI]::SetForegroundWindow($hwnd) | Out-Null
    Start-Sleep -Milliseconds 500

    $rect = New-Object WinAPI+RECT
    [WinAPI]::GetWindowRect($hwnd, [ref]$rect) | Out-Null

    $w = $rect.Right - $rect.Left
    $h = $rect.Bottom - $rect.Top

    if ($w -le 0 -or $h -le 0) {
        throw "Invalid window geometry: width=$w, height=$h"
    }

    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $gfx.CopyFromScreen($rect.Left, $rect.Top, 0, 0, (New-Object System.Drawing.Size $w, $h))
    $gfx.Dispose()

    $targetFile = Join-Path $snapshotDir ("snapshot_" + $Label + ".png")
    $bmp.Save($targetFile, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()

    Write-Host "Snapshot saved: $targetFile ($w x $h)"
} finally {
    if ($startedProcess -and $proc -and -not $proc.HasExited) {
        $proc.CloseMainWindow() | Out-Null
        Start-Sleep -Milliseconds 800
        if (-not $proc.HasExited) {
            $proc.Kill()
        }
    }
}
