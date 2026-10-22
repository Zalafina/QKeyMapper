[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('OpenAdvanced','OpenMcp','Enable','ApproveRead','Revoke','Disable','CloseMcp')][string]$Action,
    [Parameter(Mandatory)][int]$TargetProcessId,
    [Parameter(Mandatory)][string]$ExecutablePath,
    [Parameter(Mandatory)][long]$StartTicks,
    [Parameter(Mandatory)][string]$DesktopName,
    [Parameter(Mandatory)][string]$ResultFile,
    [string]$ClientExecutable,
    [switch]$OnDesktop
)

# Maintenance-only, explicitly authorized UI actions; never an MCP or product entry point.
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$runtimeRoot = [IO.Path]::GetFullPath((Join-Path $repo 'out/mcp_product')) + [IO.Path]::DirectorySeparatorChar
$expected = [IO.Path]::GetFullPath($ExecutablePath)
$resultPath = [IO.Path]::GetFullPath($ResultFile)
if (-not $expected.StartsWith($runtimeRoot, [StringComparison]::OrdinalIgnoreCase) -or
    -not $resultPath.StartsWith($runtimeRoot, [StringComparison]::OrdinalIgnoreCase) -or
    [IO.Path]::GetExtension($resultPath) -ine '.json' -or
    [IO.Path]::GetFileName($expected) -notmatch '^QKeyMapperMcpAcceptance_[a-zA-Z0-9_]+\.exe$' -or
    $DesktopName -notmatch '^QkmProduct_[a-f0-9]{32}$') { throw 'Select an owned, isolated product fixture.' }
if (-not (New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'This authorized UI helper requires elevation.' }
$target = Get-Process -Id $TargetProcessId -ErrorAction Stop
if ($target.Path -ine $expected -or $target.StartTime.ToUniversalTime().Ticks -ne $StartTicks) {
    throw 'Test process identity changed; refusing UI actions.'
}

. (Join-Path $repo 'scripts/test_process.ps1')
trap {
    if ($OnDesktop) {
        [IO.File]::WriteAllText($resultPath, (@{ success = $false; error = $_.Exception.Message; stack = $_.ScriptStackTrace } | ConvertTo-Json -Compress), [Text.UTF8Encoding]::new($false))
    }
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
if (-not $OnDesktop) {
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = (Get-Process -Id $PID).Path
    $arguments = @('-NoProfile','-STA','-File',$PSCommandPath,'-OnDesktop','-Action',$Action,
        '-TargetProcessId',[string]$TargetProcessId,'-ExecutablePath',$expected,'-StartTicks',[string]$StartTicks,
        '-DesktopName',$DesktopName,'-ResultFile',$resultPath)
    if ($ClientExecutable) { $arguments += @('-ClientExecutable',[IO.Path]::GetFullPath($ClientExecutable)) }
    $info.Arguments = ($arguments | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $info.WorkingDirectory = $repo
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $child = Start-QkmProcessInfo $info $DesktopName
    try {
        $null = $child.Handle
        if (-not $child.WaitForExit(45000)) { throw "Owned GUI helper did not finish; PID $($child.Id) was left running." }
        if (-not (Test-Path -LiteralPath $resultPath)) { throw "GUI helper produced no result; exit $($child.ExitCode)." }
        $value = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
        if ($child.ExitCode -ne 0 -or -not $value.success) { throw "GUI helper failed: $($value.error)" }
        $value | ConvertTo-Json -Compress
        exit 0
    } finally { $child.Dispose() }
}
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class QkmProductUiDesktop {
    private delegate bool WindowCallback(IntPtr window, IntPtr data);
    [DllImport("kernel32.dll")] private static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
    [DllImport("user32.dll")] private static extern IntPtr GetThreadDesktop(uint thread);
    [DllImport("user32.dll", SetLastError=true)] private static extern bool EnumDesktopWindows(IntPtr desktop, WindowCallback callback, IntPtr data);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetWindowText(IntPtr window, StringBuilder text, int length);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern bool GetUserObjectInformation(IntPtr handle, int index, StringBuilder text, uint size, out uint needed);
    public static string Name(IntPtr window) {
        uint process;
        uint thread = window == IntPtr.Zero ? GetCurrentThreadId() : GetWindowThreadProcessId(window, out process);
        var name = new StringBuilder(256); uint needed;
        if (!GetUserObjectInformation(GetThreadDesktop(thread), 2, name, 512, out needed))
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        return name.ToString();
    }
    public static IntPtr[] NamedWindows(int process, string[] names) {
        var result = new List<IntPtr>();
        WindowCallback callback = (window, data) => {
            uint owner; GetWindowThreadProcessId(window, out owner);
            if (owner != process || !IsWindowVisible(window)) return true;
            var text = new StringBuilder(512); GetWindowText(window, text, text.Capacity);
            if (Array.IndexOf(names, text.ToString()) >= 0) result.Add(window);
            return true;
        };
        if (!EnumDesktopWindows(GetThreadDesktop(GetCurrentThreadId()), callback, IntPtr.Zero))
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        return result.ToArray();
    }
}
'@
Add-Type -AssemblyName UIAutomationClient
$main = Get-QkmMainWindowHandle -ProcessId $TargetProcessId -DesktopName $DesktopName
if ($main -eq [IntPtr]::Zero -or [QkmProductUiDesktop]::Name($main) -cne $DesktopName -or
    [QkmProductUiDesktop]::Name([IntPtr]::Zero) -cne $DesktopName) { throw 'Actual target/helper desktop mismatch.' }
$mainRoot = [Windows.Automation.AutomationElement]::FromHandle($main)
$mcpNames = @('Agent / MCP 控制','Agent / MCP Control','Agent / MCP 操作')

function Find-OwnedWindow {
    param([string[]]$Names)
    $deadline = [DateTime]::UtcNow.AddSeconds(3)
    do {
        $matches = @([QkmProductUiDesktop]::NamedWindows($TargetProcessId, $Names))
        if ($matches.Count) { break }
        Start-Sleep -Milliseconds 50
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($matches.Count -ne 1) { throw 'Expected one owned dialog; no action was performed.' }
    return [Windows.Automation.AutomationElement]::FromHandle($matches[0])
}

function Find-Control {
    param($Root, [Windows.Automation.ControlType]$Type, [string]$Id, [string[]]$Names)
    $condition = [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::ControlTypeProperty, $Type)
    $deadline = [DateTime]::UtcNow.AddSeconds(3)
    do {
        $items = $Root.FindAll([Windows.Automation.TreeScope]::Descendants, $condition)
        $matches = @($items | Where-Object { ($Id -and ($_.Current.AutomationId -ceq $Id -or
            $_.Current.AutomationId.EndsWith('.' + $Id, [StringComparison]::Ordinal))) -or $Names -ccontains $_.Current.Name })
        if ($matches.Count) { break }
        Start-Sleep -Milliseconds 50
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($matches.Count -ne 1 -or $matches[0].Current.ProcessId -ne $TargetProcessId) {
        $candidates = @($items | Select-Object -First 60 | ForEach-Object { $_.Current.AutomationId + ':' + $_.Current.Name }) -join '; '
        throw "Expected one owned $($Type.ProgrammaticName) control '$Id'; count $($items.Count); root '$($Root.Current.Name)'; candidates $candidates. No action was performed."
    }
    return $matches[0]
}

function Invoke-Button {
    param($Root, [string]$Id, [string[]]$Names)
    $button = Find-Control $Root ([Windows.Automation.ControlType]::Button) $Id $Names
    $deadline = [DateTime]::UtcNow.AddSeconds(3)
    while (-not $button.Current.IsEnabled -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 50 }
    if (-not $button.Current.IsEnabled) { throw 'Expected enabled button.' }
    $button.GetCurrentPattern([Windows.Automation.InvokePattern]::Pattern).Invoke()
}

function Check-State {
    param($Root, [string]$Id, [string[]]$Names)
    $check = Find-Control $Root ([Windows.Automation.ControlType]::CheckBox) $Id $Names
    return $check.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern).Current.ToggleState
}

function Select-Control {
    param($Control)
    $pattern = $null
    if ($Control.TryGetCurrentPattern([Windows.Automation.SelectionItemPattern]::Pattern, [ref]$pattern)) {
        $pattern.Select()
    } elseif ($Control.TryGetCurrentPattern([Windows.Automation.InvokePattern]::Pattern, [ref]$pattern)) {
        $pattern.Invoke()
    } else {
        $supported = @($Control.GetSupportedPatterns() | ForEach-Object { $_.ProgrammaticName }) -join ', '
        throw "Selection unavailable for '$($Control.Current.Name)'; supported patterns: $supported."
    }
}

if ($Action -eq 'OpenAdvanced') {
    $mappingTab = Find-Control $mainRoot ([Windows.Automation.ControlType]::TabItem) '' @('映射设定','Mapping','マッピング設定')
    Select-Control $mappingTab
    $stopKey = Find-Control $mainRoot ([Windows.Automation.ControlType]::Edit) 'mappingStopKeyLineEdit' @()
    $stopShortcut = $stopKey.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value
    $startButton = Find-Control $mainRoot ([Windows.Automation.ControlType]::Button) 'keymapButton' @()
    $mappingButtonText = $startButton.Current.Name
    $sourceTab = Find-Control $mainRoot ([Windows.Automation.ControlType]::TabItem) '' @('Tab1')
    $sourceTabName = $sourceTab.Current.Name
    $configuration = Find-Control $mainRoot ([Windows.Automation.ControlType]::ComboBox) 'settingselectComboBox' @()
    $configurationDisplay = $configuration.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value
    $tab = Find-Control $mainRoot ([Windows.Automation.ControlType]::TabItem) '' @('通用设定','General','一般設定')
    Select-Control $tab
    Invoke-Button $mainRoot 'generalAdvancedButton' @('通用高级设定','General Advanced','一般高度設定')
    $null = Find-OwnedWindow @('通用高级设定','General Advanced Setting','一般高度設定')
} elseif ($Action -eq 'OpenMcp') {
    $dialog = Find-OwnedWindow @('通用高级设定','General Advanced Setting','一般高度設定')
    Invoke-Button $dialog 'mcpControlButton' $mcpNames
} else {
    $dialog = Find-OwnedWindow $mcpNames
    switch ($Action) {
        { $_ -in 'Enable','Disable' } {
            $check = Find-Control $dialog ([Windows.Automation.ControlType]::CheckBox) 'mcpEnabled' @('启用本次运行的 MCP 接口','Enable MCP for this run','今回の実行で MCP を有効にする')
            $pattern = $check.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern)
            $wanted = if ($Action -eq 'Enable') { [Windows.Automation.ToggleState]::On } else { [Windows.Automation.ToggleState]::Off }
            if ($pattern.Current.ToggleState -ne $wanted) { $pattern.Toggle() }
        }
        'ApproveRead' {
            if (-not $ClientExecutable) { throw 'A trusted client executable is required.' }
            $field = Find-Control $dialog ([Windows.Automation.ControlType]::Edit) 'mcpClientExecutable' @('系统核实的客户端程序路径','Verified client executable path','確認済みクライアントのパス')
            $actual = $field.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value
            if ($actual -ine [IO.Path]::GetFullPath($ClientExecutable)) { throw 'Verified client path differs; refusing approval.' }
            $read = Check-State $dialog '' @('读取状态与设定（必需）','Read state and configuration (required)','状態と設定の読み取り（必須）')
            $configure = Check-State $dialog 'mcpConfigure' @('更改映射与软件设定','Change mappings and settings','マッピングと設定の変更')
            $run = Check-State $dialog 'mcpRunMapping' @('开始与停止映射（可能发送输入）','Start and stop mapping (may send input)','マッピングの開始と停止（入力を送信する場合あり）')
            if ($read -ne [Windows.Automation.ToggleState]::On -or $configure -ne [Windows.Automation.ToggleState]::Off -or
                $run -ne [Windows.Automation.ToggleState]::Off) { throw 'Only read permission may be approved by this test helper.' }
            Invoke-Button $dialog 'mcpApprove' @('批准选定权限','Approve selected permissions','選択した権限で承認')
        }
        'Revoke' {
            $list = Find-Control $dialog ([Windows.Automation.ControlType]::List) 'mcpPairings' @('已配对客户端及权限','Paired clients and permissions','ペアリング済みクライアントと権限')
            $items = $list.FindAll([Windows.Automation.TreeScope]::Children,
                [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::ListItem))
            if ($items.Count -ne 1) { throw 'Expected exactly one test pairing; refusing revocation.' }
            Select-Control $items[0]
            Invoke-Button $dialog 'mcpRevoke' @('撤销选中客户端的配对','Revoke selected pairing','選択したペアリングを解除')
            $deadline = [DateTime]::UtcNow.AddSeconds(5)
            do {
                $remaining = $list.FindAll([Windows.Automation.TreeScope]::Children,
                    [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::ListItem)).Count
                if (-not $remaining) { break }
                Start-Sleep -Milliseconds 50
            } while ([DateTime]::UtcNow -lt $deadline)
            if ($remaining) { throw 'Revocation completion was not observed; inspect state before retrying.' }
        }
        'CloseMcp' { Invoke-Button $dialog '' @('关闭窗口','Close window','画面を閉じる') }
    }
}
$result = @{ success = $true; action = $Action; desktop = [QkmProductUiDesktop]::Name($main); window = $main.ToInt64() }
if ($Action -eq 'OpenAdvanced') {
    $result.stop_shortcut_edit = $stopShortcut
    $result.mapping_button_text = $mappingButtonText
    $result.mapping_tab_name = $sourceTabName
    $result.configuration_display = $configurationDisplay
    $result.configuration_read_method = 'ValuePattern'
}
if ($Action -notin 'OpenAdvanced','CloseMcp') {
    $dialog = Find-OwnedWindow $mcpNames
    $result.dialog_window = $dialog.Current.NativeWindowHandle
    $field = Find-Control $dialog ([Windows.Automation.ControlType]::Edit) 'mcpConnection' @('连接信息文件路径','Connection information file','接続情報ファイルのパス')
    $result.discovery = $field.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value
    $result.enabled = (Check-State $dialog 'mcpEnabled' @('启用本次运行的 MCP 接口','Enable MCP for this run','今回の実行で MCP を有効にする')) -eq [Windows.Automation.ToggleState]::On
    $list = Find-Control $dialog ([Windows.Automation.ControlType]::List) 'mcpPairings' @('已配对客户端及权限','Paired clients and permissions','ペアリング済みクライアントと権限')
    $result.pair_count = $list.FindAll([Windows.Automation.TreeScope]::Children,
        [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::ListItem)).Count
}
# A successful UI call is only an observation; the controller checks IPC and persistence separately.
[IO.File]::WriteAllText($resultPath, ($result | ConvertTo-Json -Compress), [Text.UTF8Encoding]::new($false))
