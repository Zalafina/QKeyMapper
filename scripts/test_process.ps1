[CmdletBinding()]
param([string]$LaunchRequest)

function Get-QkmMainWindowHandle {
    param([int]$ProcessId)
    if (-not ([System.Management.Automation.PSTypeName]'QkmTestWindows').Type) {
        Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class QkmTestWindows {
    private delegate bool WindowCallback(IntPtr window, IntPtr data);
    [DllImport("user32.dll")] private static extern bool EnumWindows(WindowCallback callback, IntPtr data);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetWindowText(IntPtr window, StringBuilder text, int length);
    [DllImport("user32.dll")] private static extern int GetWindowLong(IntPtr window, int index);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr wparam, IntPtr lparam);
    public static uint Owner(IntPtr window) {
        uint process; GetWindowThreadProcessId(window, out process); return process;
    }
    public static IntPtr MainWindow(int process) {
        IntPtr result = IntPtr.Zero;
        EnumWindows((window, data) => {
            uint owner; GetWindowThreadProcessId(window, out owner);
            if (owner != process) return true;
            if ((GetWindowLong(window, -16) & 0x00C00000) != 0x00C00000) return true;
            var title = new StringBuilder(512); GetWindowText(window, title, title.Capacity);
            if (!title.ToString().StartsWith("QKeyMapper v", StringComparison.OrdinalIgnoreCase)) return true;
            result = window; return false;
        }, IntPtr.Zero);
        return result;
    }
}
'@
    }
    return [QkmTestWindows]::MainWindow($ProcessId)
}

function Request-QkmTestProcessClose {
    param($Process)
    $window = Get-QkmMainWindowHandle -ProcessId $Process.Id
    if ($window -ne [IntPtr]::Zero) {
        return [QkmTestWindows]::PostMessage($window, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
    }
    return $Process.CloseMainWindow()
}

# Elevate only this short-lived launch/close helper, not the caller's test script.
if ($LaunchRequest) {
    $ErrorActionPreference = 'Stop'
    $request = Get-Content -LiteralPath $LaunchRequest -Raw | ConvertFrom-Json
    try {
        if ($request.Action -eq 'Start') {
            $info = New-Object Diagnostics.ProcessStartInfo
            $info.FileName = $request.Executable
            $info.Arguments = $request.Arguments
            $info.WorkingDirectory = Split-Path -Parent $request.Executable
            $info.UseShellExecute = $false
            $info.CreateNoWindow = $true
            $info.EnvironmentVariables['PATH'] = $env:PATH
            foreach ($entry in $request.Environment.PSObject.Properties) {
                $info.EnvironmentVariables[$entry.Name] = [string]$entry.Value
            }
            $child = [Diagnostics.Process]::Start($info)
        } elseif ($request.Action -eq 'Close') {
            $child = Get-Process -Id $request.ProcessId -ErrorAction Stop
            if ($child.Path -ne $request.Executable -or $child.StartTime.ToUniversalTime().Ticks -ne $request.StartTicks) {
                throw 'Test process identity changed; refusing to close it.'
            }
            # Retain a process handle before exit; a Get-Process wrapper alone can lose ExitCode.
            $null = $child.Handle
            if (-not (Request-QkmTestProcessClose $child) -or -not $child.WaitForExit(10000)) {
                throw 'Test process did not close gracefully; it has been left running.'
            }
        } else {
            throw 'Unknown test process action.'
        }
        $result = @{ Success = $true; ProcessId = $child.Id; Executable = $request.Executable }
        if ($request.Action -eq 'Start') { $result.StartTicks = $child.StartTime.ToUniversalTime().Ticks }
        if ($request.Action -eq 'Close') { $result.ExitCode = $child.ExitCode }
    } catch {
        $result = @{ Success = $false; Error = $_.Exception.Message }
    }
    $result | ConvertTo-Json | Set-Content -LiteralPath $request.Response -Encoding UTF8
    if (-not $result.Success) { exit 1 }
    exit 0
}

$script:qkmTestProcessHelperPath = $PSCommandPath

function Invoke-QkmGsudoRequest {
    param([hashtable]$Request)
    $gsudoCommand = Get-Command gsudo.exe -CommandType Application -ErrorAction SilentlyContinue
    $gsudoPath = if ($gsudoCommand) { $gsudoCommand.Source } else { $null }
    if (-not $gsudoPath) {
        $gsudoPath = Get-ChildItem -LiteralPath 'C:\Program Files\gsudo' -Filter gsudo.exe -File -Recurse -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
    }
    if (-not $gsudoPath) { throw 'gsudo.exe was not found. Use -ProcessId to attach an already running test.' }
    $runDirectory = Join-Path (Split-Path -Parent $PSScriptRoot) ('out\test_process\' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Force -Path $runDirectory | Out-Null
    $requestFile = Join-Path $runDirectory 'request.json'
    $Request.Response = Join-Path $runDirectory 'response.json'
    $Request | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $requestFile -Encoding UTF8
    $shellPath = (Get-Process -Id $PID).Path
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $gsudoPath
    $info.Arguments = '--direct "' + $shellPath + '" -NoProfile -ExecutionPolicy Bypass -File "' + $script:qkmTestProcessHelperPath + '" -LaunchRequest "' + $requestFile + '"'
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.EnvironmentVariables['PATH'] = $env:PATH
    Write-Host "gsudo: $($Request.Action) $($Request.Executable) (confirm the elevation prompt if shown)"
    $launcher = [Diagnostics.Process]::Start($info)
    try {
        if (-not $launcher.WaitForExit(60000)) {
            throw "gsudo has not completed. Check the elevation prompt and $runDirectory."
        }
        if (-not (Test-Path -LiteralPath $Request.Response)) {
            throw "gsudo did not return a test process result (exit $($launcher.ExitCode))."
        }
        $response = Get-Content -LiteralPath $Request.Response -Raw | ConvertFrom-Json
        if (-not $response.Success) { throw $response.Error }
        if ($launcher.ExitCode -ne 0) { throw "gsudo failed with exit code $($launcher.ExitCode)." }
        return $response
    } finally {
        $launcher.Dispose()
    }
}

function Start-QkmTestProcess {
    param([string]$ExecutablePath, [string]$Arguments = '', [switch]$UseGsudo, [hashtable]$EnvironmentOverrides = @{})
    $executable = (Resolve-Path -LiteralPath $ExecutablePath -ErrorAction Stop).Path
    if ($UseGsudo) {
        $response = Invoke-QkmGsudoRequest @{ Action = 'Start'; Executable = $executable; Arguments = $Arguments; Environment = $EnvironmentOverrides }
        $process = Get-Process -Id $response.ProcessId -ErrorAction Stop
        $startTicks = $response.StartTicks
    } else {
        $info = New-Object Diagnostics.ProcessStartInfo
        $info.FileName = $executable
        $info.Arguments = $Arguments
        $info.WorkingDirectory = Split-Path -Parent $executable
        $info.UseShellExecute = $false
        $info.CreateNoWindow = $true
        $info.EnvironmentVariables['PATH'] = $env:PATH
        foreach ($key in $EnvironmentOverrides.Keys) { $info.EnvironmentVariables[$key] = $EnvironmentOverrides[$key] }
        try {
            $process = [Diagnostics.Process]::Start($info)
        } catch {
            throw "Could not start test EXE. For an administrator manifest, retry with -UseGsudo. $($_.Exception.Message)"
        }
        $startTicks = $process.StartTime.ToUniversalTime().Ticks
    }
    return [pscustomobject]@{ Process = $process; Executable = $executable; StartTicks = $startTicks; UseGsudo = [bool]$UseGsudo }
}

function Close-QkmTestProcess {
    param($TestProcess)
    $process = $TestProcess.Process
    $process.Refresh()
    if ($process.HasExited) { return $process.ExitCode }
    if ($TestProcess.UseGsudo) {
        $response = Invoke-QkmGsudoRequest @{ Action = 'Close'; Executable = $TestProcess.Executable; ProcessId = $process.Id; StartTicks = $TestProcess.StartTicks }
        return $response.ExitCode
    } else {
        if ($process.StartTime.ToUniversalTime().Ticks -ne $TestProcess.StartTicks -or $process.Path -ne $TestProcess.Executable) {
            throw 'Test process identity changed; refusing to close it.'
        }
        # Keep the exit status available for reconstructed Get-Process wrappers.
        $null = $process.Handle
        if (-not (Request-QkmTestProcessClose $process) -or -not $process.WaitForExit(10000)) {
            throw 'Test process did not close gracefully; it has been left running.'
        }
        return $process.ExitCode
    }
}
