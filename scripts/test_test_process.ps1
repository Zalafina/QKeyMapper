[CmdletBinding()]
param([string]$OutputDirectory, [switch]$Fixture, [string]$StatePath)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$outRoot = [IO.Path]::GetFullPath((Join-Path $repo 'out')) + [IO.Path]::DirectorySeparatorChar
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class QkmProcessFixture {
    [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern IntPtr CreateDesktop(string name, IntPtr device, IntPtr mode, uint flags, uint access, IntPtr security);
    [DllImport("user32.dll")] public static extern bool CloseDesktop(IntPtr desktop);
    [DllImport("user32.dll")] private static extern IntPtr GetThreadDesktop(uint thread);
    [DllImport("kernel32.dll")] private static extern uint GetCurrentThreadId();
    [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern bool GetUserObjectInformation(IntPtr handle, int index, StringBuilder value, uint size, out uint needed);
    public static string DesktopName() {
        var name = new StringBuilder(256); uint needed;
        if (!GetUserObjectInformation(GetThreadDesktop(GetCurrentThreadId()), 2, name, 512, out needed))
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        return name.ToString();
    }
}
'@

if ($Fixture) {
    $state = [IO.Path]::GetFullPath($StatePath)
    if (-not $state.StartsWith($outRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture state must be under out/.' }
    Add-Type -AssemblyName System.Windows.Forms
    $form = New-Object Windows.Forms.Form
    $form.Text = 'QKeyMapper vProcessFixture'
    $form.Width = 320
    $form.Height = 160
    $form.Add_Shown({
        $data = @{ pid = $PID; desktop = [QkmProcessFixture]::DesktopName(); value = $env:QKM_PROCESS_FIXTURE_VALUE }
        [IO.File]::WriteAllText($state, ($data | ConvertTo-Json), (New-Object Text.UTF8Encoding($false)))
    })
    $timer = New-Object Windows.Forms.Timer
    $timer.Interval = 30000
    $timer.Add_Tick({ $form.Close() })
    $timer.Start()
    try { [Windows.Forms.Application]::Run($form) }
    finally { $timer.Dispose(); $form.Dispose() }
    exit 0
}

. (Join-Path $PSScriptRoot 'test_process.ps1')
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (-not $output.StartsWith($outRoot, [StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $output)) {
    throw 'Choose a new output directory under this checkout out/.'
}
New-Item -ItemType Directory -Path $output | Out-Null
$desktopName = 'QkmProcess_' + [Guid]::NewGuid().ToString('N')
$desktop = [QkmProcessFixture]::CreateDesktop($desktopName, [IntPtr]::Zero, [IntPtr]::Zero, 0, 511, [IntPtr]::Zero)
if ($desktop -eq [IntPtr]::Zero) { throw 'CreateDesktop failed.' }
$testProcess = $null
try {
    if ((Get-QkmMainWindowHandle -ProcessId $PID -DesktopName $desktopName) -ne [IntPtr]::Zero) {
        throw 'An empty private desktop unexpectedly contained a matching main window.'
    }
    $shell = (Get-Process -Id $PID).Path
    $state = Join-Path $output 'state with spaces.json'
    $arguments = '-NoProfile -STA -File "' + $PSCommandPath + '" -Fixture -StatePath "' + $state + '"'
    $probe = 'Unicode 中文 日本語 & $() ` literal'
    $testProcess = Start-QkmTestProcess -ExecutablePath $shell -Arguments $arguments -DesktopName $desktopName -EnvironmentOverrides @{ QKM_PROCESS_FIXTURE_VALUE = $probe }
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    while (-not (Test-Path -LiteralPath $state) -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 50 }
    $record = Get-Content -LiteralPath $state -Raw | ConvertFrom-Json
    if ($record.pid -ne $testProcess.Process.Id -or $record.desktop -ne $desktopName -or $record.value -cne $probe) {
        throw 'Process, actual desktop, argument or environment did not match.'
    }
    $startTicks = $testProcess.StartTicks
    $testProcess.StartTicks++
    try {
        $rejected = $false
        try { Close-QkmTestProcess $testProcess | Out-Null }
        catch { $rejected = $_.Exception.Message -like '*identity changed*' }
        if (-not $rejected -or $testProcess.Process.HasExited) { throw 'Mismatched identity was not safely rejected.' }
    } finally { $testProcess.StartTicks = $startTicks }
    $exitCode = Close-QkmTestProcess $testProcess
    if ($null -eq $exitCode -or $exitCode -ne 0) { throw "Close exit code was not 0: $exitCode" }
    $testProcess = $null
    $rejected = $false
    try { Start-QkmTestProcess -ExecutablePath $shell -DesktopName ($desktopName + '_missing') | Out-Null }
    catch { $rejected = $true }
    if (-not $rejected) { throw 'Missing desktop unexpectedly started a process.' }
    Write-Output 'PASS: empty desktop, private startup, actual desktop, literal arguments/environment, identity guard, graceful exit 0, missing desktop rejection.'
} finally {
    if ($null -ne $testProcess) { Close-QkmTestProcess $testProcess | Out-Null }
    [QkmProcessFixture]::CloseDesktop($desktop) | Out-Null
}
