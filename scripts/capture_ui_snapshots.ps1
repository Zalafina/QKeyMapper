[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Label,
    [string]$ExecutablePath,
    [string]$Arguments = "",
    [int]$ProcessId,
    [long]$WindowHandle,
    [switch]$PrintWindow,
    [switch]$UseGsudo
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot 'test_process.ps1')
if ($ProcessId -and ($ExecutablePath -or $Arguments -or $UseGsudo)) {
    throw '-ProcessId attaches without launching; do not combine it with launch options.'
}
if ([IO.Path]::GetFileName($Label) -ne $Label -or $Label -match '[\\/:*?"<>|]') {
    throw 'Label must be a filename component.'
}

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$sig = @"
using System;
using System.Runtime.InteropServices;

public class WinAPI {
    [DllImport("user32.dll")]
    public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);

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
[WinAPI]::SetProcessDPIAware() | Out-Null

$repoRoot = Split-Path -Parent $PSScriptRoot
$snapshotDir = Join-Path $repoRoot "test_snapshots"
New-Item -ItemType Directory -Force -Path $snapshotDir | Out-Null

$proc = $null
$testProcess = $null

if (-not $ProcessId -and -not $ExecutablePath) {
    $candidateExe = Join-Path $repoRoot "out\build_qt6\release\QKeyMapper.exe"
    if (Test-Path $candidateExe) {
        $ExecutablePath = $candidateExe
    }
}

if ($ProcessId) {
    $proc = Get-Process -Id $ProcessId -ErrorAction Stop
    Write-Host "Attaching to PID $ProcessId; this process will be left running."
} elseif ($ExecutablePath) {
    $testProcess = Start-QkmTestProcess -ExecutablePath $ExecutablePath -Arguments $Arguments -UseGsudo:$UseGsudo
    $proc = $testProcess.Process
    Start-Sleep -Seconds 3
} else {
    throw "No default executable at $candidateExe. Build it or provide -ExecutablePath / -ProcessId explicitly."
}

try {
    $hwnd = if ($WindowHandle) { [IntPtr]$WindowHandle } else { Get-QkmMainWindowHandle -ProcessId $proc.Id }
    if ($WindowHandle) {
        $null = Get-QkmMainWindowHandle -ProcessId $proc.Id
        if ([QkmTestWindows]::Owner($hwnd) -ne $proc.Id) {
            throw 'Requested window does not belong to the attached test process.'
        }
        if (-not [WinAPI]::GetWindowRect($hwnd, [ref](New-Object WinAPI+RECT))) {
            throw 'Requested window handle is no longer valid.'
        }
    }
    if ($hwnd -eq [IntPtr]::Zero) { $hwnd = $proc.MainWindowHandle }
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
    if (-not [WinAPI]::GetWindowRect($hwnd, [ref]$rect)) {
        throw "GetWindowRect failed for PID $($proc.Id)."
    }

    $w = $rect.Right - $rect.Left
    $h = $rect.Bottom - $rect.Top

    if ($w -le 0 -or $h -le 0) {
        throw "Invalid window geometry: width=$w, height=$h"
    }

    $targetFile = Join-Path $snapshotDir ("snapshot_" + $Label + ".png")
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    try {
        $gfx = [System.Drawing.Graphics]::FromImage($bmp)
        try {
            if ($PrintWindow) {
                $dc = $gfx.GetHdc()
                try {
                    if (-not [WinAPI]::PrintWindow($hwnd, $dc, 2)) {
                        throw 'PrintWindow failed; no screenshot was saved.'
                    }
                } finally { $gfx.ReleaseHdc($dc) }
            } else {
                $gfx.CopyFromScreen($rect.Left, $rect.Top, 0, 0, (New-Object System.Drawing.Size $w, $h))
            }
        } finally {
            $gfx.Dispose()
        }
        $bmp.Save($targetFile, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $bmp.Dispose()
    }

    Write-Host "Snapshot saved: $targetFile ($w x $h)"
} finally {
    if ($testProcess) { Close-QkmTestProcess -TestProcess $testProcess | Out-Null }
}
