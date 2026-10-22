[CmdletBinding()]
param([string]$LaunchRequest)

function Initialize-QkmTestWindows {
    if (-not ([System.Management.Automation.PSTypeName]'QkmTestWindows').Type) {
        Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.ComponentModel;
using System.Threading;
using System.Runtime.InteropServices;
public static class QkmTestWindows {
    private delegate bool WindowCallback(IntPtr window, IntPtr data);
    [DllImport("user32.dll")] private static extern bool EnumWindows(WindowCallback callback, IntPtr data);
    [DllImport("user32.dll", SetLastError=true)] private static extern bool EnumDesktopWindows(IntPtr desktop, WindowCallback callback, IntPtr data);
    [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)] private static extern IntPtr OpenDesktop(string name, uint flags, bool inherit, uint access);
    [DllImport("user32.dll")] private static extern bool CloseDesktop(IntPtr desktop);
    [DllImport("user32.dll", SetLastError=true)] private static extern bool SetThreadDesktop(IntPtr desktop);
    [DllImport("user32.dll")] private static extern IntPtr GetThreadDesktop(uint thread);
    [DllImport("kernel32.dll")] private static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetWindowText(IntPtr window, StringBuilder text, int length);
    [DllImport("user32.dll")] private static extern int GetWindowLong(IntPtr window, int index);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr wparam, IntPtr lparam);
    public static uint Owner(IntPtr window) {
        uint process; GetWindowThreadProcessId(window, out process); return process;
    }
    public static IntPtr MainWindow(int process, string desktop) {
        IntPtr result = IntPtr.Zero;
        WindowCallback callback = (window, data) => {
            uint owner; GetWindowThreadProcessId(window, out owner);
            if (owner != process) return true;
            if ((GetWindowLong(window, -16) & 0x00C00000) != 0x00C00000) return true;
            var title = new StringBuilder(512); GetWindowText(window, title, title.Capacity);
            if (!title.ToString().StartsWith("QKeyMapper v", StringComparison.OrdinalIgnoreCase)) return true;
            result = window; return false;
        };
        if (String.IsNullOrEmpty(desktop)) EnumWindows(callback, IntPtr.Zero);
        else {
            IntPtr handle = OpenDesktop(desktop, 0, false, 1);
            if (handle == IntPtr.Zero) throw new Win32Exception(Marshal.GetLastWin32Error());
            try {
                if (!EnumDesktopWindows(handle, callback, IntPtr.Zero) && result == IntPtr.Zero)
                    throw new Win32Exception(Marshal.GetLastWin32Error());
            } finally { CloseDesktop(handle); }
        }
        return result;
    }
    public static bool ClosePrivate(int process, string desktop) {
        bool result = false; Exception failure = null;
        var thread = new Thread(() => {
            IntPtr original = GetThreadDesktop(GetCurrentThreadId());
            IntPtr handle = OpenDesktop(desktop, 0, false, 0x83);
            if (handle == IntPtr.Zero) { failure = new Win32Exception(Marshal.GetLastWin32Error()); return; }
            try {
                if (!SetThreadDesktop(handle)) throw new Win32Exception(Marshal.GetLastWin32Error());
                IntPtr window = MainWindow(process, desktop);
                result = window != IntPtr.Zero && PostMessage(window, 0x10, IntPtr.Zero, IntPtr.Zero);
            } catch (Exception error) { failure = error; }
            finally {
                if (!SetThreadDesktop(original) && failure == null) failure = new Win32Exception(Marshal.GetLastWin32Error());
                CloseDesktop(handle);
            }
        });
        thread.Start(); thread.Join();
        if (failure != null) throw failure;
        return result;
    }
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    private struct StartupInfo {
        public uint cb;
        public string reserved, desktop, title;
        public uint x, y, width, height, charsX, charsY, fill, flags;
        public ushort show, reservedSize;
        public IntPtr reservedData, input, output, error;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct ProcessInfo {
        public IntPtr process, thread;
        public uint processId, threadId;
    }
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern bool CreateProcess(string executable, StringBuilder command, IntPtr processAttributes,
        IntPtr threadAttributes, bool inherit, uint flags, IntPtr environment, string directory,
        ref StartupInfo startup, out ProcessInfo process);
    [DllImport("kernel32.dll")] private static extern bool CloseHandle(IntPtr handle);
    public static System.Diagnostics.Process StartPrivate(string executable, string arguments, string directory, string desktop, string[] environment) {
        Array.Sort(environment, StringComparer.OrdinalIgnoreCase);
        foreach (string entry in environment)
            if (entry.IndexOf('\0') >= 0) throw new ArgumentException("Environment entry contains NUL.");
        IntPtr handle = OpenDesktop(desktop, 0, false, 0x83);
        if (handle == IntPtr.Zero) throw new Win32Exception(Marshal.GetLastWin32Error());
        IntPtr block = IntPtr.Zero;
        try {
            block = Marshal.StringToHGlobalUni(String.Join("\0", environment) + "\0\0");
            var startup = new StartupInfo { cb = (uint)Marshal.SizeOf(typeof(StartupInfo)), desktop = "winsta0\\" + desktop };
            var command = new StringBuilder("\"" + executable + "\"" + (String.IsNullOrEmpty(arguments) ? "" : " " + arguments));
            ProcessInfo process;
            if (!CreateProcess(executable, command, IntPtr.Zero, IntPtr.Zero, false, 0x08000400, block,
                               directory, ref startup, out process))
                throw new Win32Exception(Marshal.GetLastWin32Error());
            try {
                // Capture the managed handle before releasing creation handles, so the PID cannot be reused.
                var child = System.Diagnostics.Process.GetProcessById(checked((int)process.processId));
                try {
                    if (child.Handle == IntPtr.Zero) throw new Win32Exception("Could not retain the test process handle.");
                    return child;
                } catch { child.Dispose(); throw; }
            } finally { CloseHandle(process.thread); CloseHandle(process.process); }
        } finally {
            if (block != IntPtr.Zero) Marshal.FreeHGlobal(block);
            CloseDesktop(handle);
        }
    }
}
'@
    }
}

function Get-QkmMainWindowHandle {
    param([int]$ProcessId, [string]$DesktopName = '')
    Initialize-QkmTestWindows
    return [QkmTestWindows]::MainWindow($ProcessId, $DesktopName)
}

function Start-QkmProcessInfo {
    param($StartInfo, [string]$DesktopName)
    if (-not $DesktopName) { return [Diagnostics.Process]::Start($StartInfo) }
    if ($DesktopName -notmatch '^[a-zA-Z0-9_-]{1,128}$') { throw 'Invalid private desktop name.' }
    Initialize-QkmTestWindows
    $entries = @(foreach ($key in $StartInfo.EnvironmentVariables.Keys) { $key + '=' + $StartInfo.EnvironmentVariables[$key] })
    return [QkmTestWindows]::StartPrivate($StartInfo.FileName, $StartInfo.Arguments,
        $StartInfo.WorkingDirectory, $DesktopName, [string[]]$entries)
}

function Request-QkmTestProcessClose {
    param($Process, [string]$DesktopName = '')
    if ($DesktopName) {
        Initialize-QkmTestWindows
        return [QkmTestWindows]::ClosePrivate($Process.Id, $DesktopName)
    }
    $window = Get-QkmMainWindowHandle -ProcessId $Process.Id -DesktopName $DesktopName
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
            $child = Start-QkmProcessInfo $info $request.DesktopName
        } elseif ($request.Action -eq 'Close') {
            $child = Get-Process -Id $request.ProcessId -ErrorAction Stop
            if ($child.Path -ne $request.Executable -or $child.StartTime.ToUniversalTime().Ticks -ne $request.StartTicks) {
                throw 'Test process identity changed; refusing to close it.'
            }
            # Retain a process handle before exit; a Get-Process wrapper alone can lose ExitCode.
            $null = $child.Handle
            if (-not (Request-QkmTestProcessClose $child -DesktopName $request.DesktopName) -or -not $child.WaitForExit(10000)) {
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
    param([string]$ExecutablePath, [string]$Arguments = '', [switch]$UseGsudo, [hashtable]$EnvironmentOverrides = @{}, [string]$DesktopName = '')
    $executable = (Resolve-Path -LiteralPath $ExecutablePath -ErrorAction Stop).Path
    if ($UseGsudo) {
        $response = Invoke-QkmGsudoRequest @{ Action = 'Start'; Executable = $executable; Arguments = $Arguments; Environment = $EnvironmentOverrides; DesktopName = $DesktopName }
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
            $process = Start-QkmProcessInfo $info $DesktopName
        } catch {
            throw "Could not start test EXE. For an administrator manifest, retry with -UseGsudo. $($_.Exception.Message)"
        }
        $startTicks = $process.StartTime.ToUniversalTime().Ticks
    }
    return [pscustomobject]@{ Process = $process; Executable = $executable; StartTicks = $startTicks; UseGsudo = [bool]$UseGsudo; DesktopName = $DesktopName }
}

function Close-QkmTestProcess {
    param($TestProcess)
    $process = $TestProcess.Process
    $process.Refresh()
    if ($process.HasExited) { return $process.ExitCode }
    if ($TestProcess.UseGsudo) {
        $response = Invoke-QkmGsudoRequest @{ Action = 'Close'; Executable = $TestProcess.Executable; ProcessId = $process.Id; StartTicks = $TestProcess.StartTicks; DesktopName = $TestProcess.DesktopName }
        return $response.ExitCode
    } else {
        if ($process.StartTime.ToUniversalTime().Ticks -ne $TestProcess.StartTicks -or $process.Path -ne $TestProcess.Executable) {
            throw 'Test process identity changed; refusing to close it.'
        }
        # Keep the exit status available for reconstructed Get-Process wrappers.
        $null = $process.Handle
        if (-not (Request-QkmTestProcessClose $process -DesktopName $TestProcess.DesktopName) -or -not $process.WaitForExit(10000)) {
            throw 'Test process did not close gracefully; it has been left running.'
        }
        return $process.ExitCode
    }
}
