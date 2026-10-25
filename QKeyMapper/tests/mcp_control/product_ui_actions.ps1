[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('OpenAdvanced','OpenMcp','Enable','ApproveRead','ApproveConfigure','Revoke','Disable','CloseMcp',
        'ReadMapping','ReadTab1','ReadTab2','ReadCommon','MoveSecondUp','RestoreFirstRows',
        'SelectSaved','SelectSecond','SelectInvalid','SelectBlank','ReloadCurrent',
        'AddBasicMapping','ClearBasicMapping','ShowForCapture','OpenFloatingSetup','CloseFloatingSetup',
        'SyncGroupOne','SyncGroupZero','StartMapping','StopMapping')][string]$Action,
    [Parameter(Mandatory)][int]$TargetProcessId,
    [Parameter(Mandatory)][string]$ExecutablePath,
    [Parameter(Mandatory)][long]$StartTicks,
    [Parameter(Mandatory)][string]$DesktopName,
    [Parameter(Mandatory)][string]$ResultFile,
    [string]$ClientExecutable,
    [switch]$OnDesktop,
    [switch]$AllowDefaultDesktop
)

# Maintenance-only, explicitly authorized UI actions; never an MCP or product entry point.
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$runtimeRoot = [IO.Path]::GetFullPath((Join-Path $repo 'out/mcp_product')) + [IO.Path]::DirectorySeparatorChar
$expected = [IO.Path]::GetFullPath($ExecutablePath)
$resultPath = [IO.Path]::GetFullPath($ResultFile)
$engineRoot = [IO.Path]::GetFullPath((Join-Path $repo 'out/mcp_product/engine61')) + [IO.Path]::DirectorySeparatorChar
$engineDefault = $AllowDefaultDesktop -and $DesktopName -eq 'Default' -and
    $expected.StartsWith($engineRoot, [StringComparison]::OrdinalIgnoreCase) -and
    $resultPath.StartsWith($engineRoot, [StringComparison]::OrdinalIgnoreCase) -and
    [IO.Path]::GetFileName($expected) -match '^QKeyMapperMcpAcceptance_engine61_(baseline|current)\.exe$'
if (-not $expected.StartsWith($runtimeRoot, [StringComparison]::OrdinalIgnoreCase) -or
    -not $resultPath.StartsWith($runtimeRoot, [StringComparison]::OrdinalIgnoreCase) -or
    [IO.Path]::GetExtension($resultPath) -ine '.json' -or
    [IO.Path]::GetFileName($expected) -notmatch '^QKeyMapperMcpAcceptance_[a-zA-Z0-9_]+\.exe$' -or
    ($DesktopName -notmatch '^QkmProduct_[a-f0-9]{32}$' -and -not $engineDefault) -or
    ($AllowDefaultDesktop -and -not $engineDefault) -or
    ($Action -in 'StartMapping','StopMapping','ClearBasicMapping','ShowForCapture','OpenFloatingSetup',
        'CloseFloatingSetup','SyncGroupOne','SyncGroupZero' -and -not $engineDefault) -or
    ($Action -eq 'AddBasicMapping' -and $engineDefault)) { throw 'Select an owned, isolated product fixture.' }
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
    if ($AllowDefaultDesktop) { $arguments += '-AllowDefaultDesktop' }
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
    [DllImport("user32.dll")] private static extern bool IsWindow(IntPtr window);
    [DllImport("user32.dll", SetLastError=true)] private static extern bool SetWindowPos(IntPtr window,
        IntPtr insertAfter, int x, int y, int width, int height, uint flags);
    [DllImport("user32.dll")] private static extern uint MapVirtualKey(uint key, uint type);
    [DllImport("user32.dll", SetLastError=true)]
    private static extern IntPtr SendMessageTimeout(IntPtr window, uint message, UIntPtr wparam, IntPtr lparam,
        uint flags, uint timeout, out UIntPtr result);
    [DllImport("user32.dll", SetLastError=true)]
    private static extern bool PostMessage(IntPtr window, uint message, UIntPtr wparam, IntPtr lparam);
    [StructLayout(LayoutKind.Sequential)] private struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll", SetLastError=true)] private static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [StructLayout(LayoutKind.Sequential)] private struct Point { public int X, Y; }
    [DllImport("user32.dll", SetLastError=true)] private static extern bool ScreenToClient(IntPtr window, ref Point point);
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
            if (names == null || Array.IndexOf(names, text.ToString()) >= 0) result.Add(window);
            return true;
        };
        if (!EnumDesktopWindows(GetThreadDesktop(GetCurrentThreadId()), callback, IntPtr.Zero))
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        return result.ToArray();
    }
    private static void ValidateWindow(IntPtr window, uint expectedProcess, string desktop) {
        uint process;
        if (window == IntPtr.Zero || GetWindowThreadProcessId(window, out process) == 0 || process != expectedProcess
            || Name(window) != desktop || Name(IntPtr.Zero) != desktop)
            throw new InvalidOperationException("Owned private-desktop window identity changed.");
    }
    private static void Message(IntPtr window, uint message, uint key, long data) {
        UIntPtr result;
        if (SendMessageTimeout(window, message, new UIntPtr(key), new IntPtr(data), 3, 5000, out result) == IntPtr.Zero)
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
    }
    public static void Key(IntPtr window, uint process, string desktop, uint key) {
        if (key != 0x26 && key != 0x28 && key != 0x0D && key != 0x1B)
            throw new InvalidOperationException("Only fixed combo-box navigation keys are allowed.");
        ValidateWindow(window, process, desktop);
        long data = 1 | ((long)MapVirtualKey(key, 0) << 16);
        if (key == 0x26 || key == 0x28) data |= 1L << 24;
        try { Message(window, 0x0100, key, data); }
        finally {
            if (IsWindow(window)) {
                ValidateWindow(window, process, desktop);
                Message(window, 0x0101, key, data | 0xC0000000L);
            }
        }
    }
    private static void ClickControl(IntPtr window, uint process, string desktop, int x, int y, bool right) {
        ValidateWindow(window, process, desktop);
        Rect bounds;
        if (!GetWindowRect(window, out bounds) || x < bounds.Left || x >= bounds.Right || y < bounds.Top || y >= bounds.Bottom
            || x < short.MinValue || x > short.MaxValue || y < short.MinValue || y > short.MaxValue)
            throw new InvalidOperationException("The fixed GUI control lies outside the owned window.");
        var cell = new Point { X = x, Y = y };
        if (!ScreenToClient(window, ref cell)) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        uint point = (uint)(ushort)cell.X | ((uint)(ushort)cell.Y << 16);
        try {
            if (!PostMessage(window, right ? 0x0204u : 0x0201u, new UIntPtr(right ? 2u : 1u), new IntPtr(point)))
                throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        } finally {
            ValidateWindow(window, process, desktop);
            if (!PostMessage(window, right ? 0x0205u : 0x0202u, UIntPtr.Zero, new IntPtr(point)))
                throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        }
    }
    public static void OpenMappingMenu(IntPtr window, uint process, string desktop, int x, int y) {
        ClickControl(window, process, desktop, x, y, true);
    }
    public static void OpenConfigurationPopup(IntPtr window, uint process, string desktop, int x, int y) {
        ClickControl(window, process, desktop, x, y, false);
    }
    public static void RaiseForSnapshot(IntPtr window, uint process, string desktop) {
        ValidateWindow(window, process, desktop);
        if (desktop != "Default" || !SetWindowPos(window, IntPtr.Zero, 0, 0, 0, 0, 0x53))
            throw new InvalidOperationException("Cannot show the owned Default window for capture.");
    }
}
'@
Add-Type -AssemblyName UIAutomationClient
$main = Get-QkmMainWindowHandle -ProcessId $TargetProcessId -DesktopName $DesktopName
if ($main -eq [IntPtr]::Zero -or [QkmProductUiDesktop]::Name($main) -cne $DesktopName -or
    [QkmProductUiDesktop]::Name([IntPtr]::Zero) -cne $DesktopName) { throw 'Actual target/helper desktop mismatch.' }
$mainRoot = [Windows.Automation.AutomationElement]::FromHandle($main)
if ($engineDefault) {
    $windowPattern = $mainRoot.GetCurrentPattern([Windows.Automation.WindowPattern]::Pattern)
    if ($windowPattern.Current.WindowVisualState -eq [Windows.Automation.WindowVisualState]::Minimized) {
        $windowPattern.SetWindowVisualState([Windows.Automation.WindowVisualState]::Normal)
    }
}
$mcpNames = @('Agent / MCP 控制','Agent / MCP Control','Agent / MCP 操作')
$floatingNames = @('悬浮按钮设定','悬浮按钮设置','Floating Button Setup','フローティングボタン設定')

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

$viewActions = @('ReadMapping','ReadTab1','ReadTab2','ReadCommon','MoveSecondUp','RestoreFirstRows','ClearBasicMapping','OpenFloatingSetup')
$mappingActions = $viewActions + @('SelectSaved','SelectSecond','SelectInvalid','SelectBlank','ReloadCurrent')
function Get-MappingGrid {
    $condition = [Windows.Automation.OrCondition]::new(
        [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::DataGrid),
        [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::Table))
    $tables = @($mainRoot.FindAll([Windows.Automation.TreeScope]::Descendants, $condition) | Where-Object {
        $grid = $null
        $bounds = $_.Current.BoundingRectangle
        $_.Current.ProcessId -eq $TargetProcessId -and -not $_.Current.IsOffscreen -and
            $bounds.Width -gt 0 -and $bounds.Height -gt 0 -and
            $_.TryGetCurrentPattern([Windows.Automation.GridPattern]::Pattern, [ref]$grid) -and $grid.Current.ColumnCount -eq 7
    })
    if ($tables.Count -ne 1) {
        $candidates = @($mainRoot.FindAll([Windows.Automation.TreeScope]::Descendants, $condition) | ForEach-Object {
            "$($_.Current.AutomationId)/$($_.Current.Name)/offscreen=$($_.Current.IsOffscreen)/$($_.Current.BoundingRectangle)"
        }) -join '; '
        throw "Expected one visible mapping grid, found $($tables.Count); candidates=$candidates."
    }
    return $tables[0]
}

function Read-MappingView {
    $configuration = Find-Control $mainRoot ([Windows.Automation.ControlType]::ComboBox) 'settingselectComboBox' @()
    $grid = (Get-MappingGrid).GetCurrentPattern([Windows.Automation.GridPattern]::Pattern)
    if ($grid.Current.RowCount -gt 8) { throw 'The fixed synthetic fixture has too many rows.' }
    $rows = @()
    $separators = @()
    for ($row = 0; $row -lt $grid.Current.RowCount; ++$row) {
        $cell = $grid.GetItem($row, 0)
        $span = $cell.GetCurrentPattern([Windows.Automation.GridItemPattern]::Pattern).Current.ColumnSpan
        # Qt's UIA grid can report a merged separator as one column.
        $separator = $span -gt 1 -or @('以下是共通映射项 — 点击左侧「－」可折叠隐藏',
            'Common mapping items — click left 「－」 to collapse',
            '以下は共通マッピング項目です — 左側の「－」をクリックで折りたたみ') -ccontains $cell.Current.Name
        if ($separator) { $separators += $row }
        $rows += @{ original = $cell.Current.Name; mapping = if ($separator) { '' } else { $grid.GetItem($row, 1).Current.Name } }
    }
    $stopKey = Find-Control $mainRoot ([Windows.Automation.ControlType]::Edit) 'mappingStopKeyLineEdit' @()
    $startButton = Find-Control $mainRoot ([Windows.Automation.ControlType]::Button) 'keymapButton' @()
    return @{ configuration_display = $configuration.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value;
        configuration_read_method = 'ValuePattern'; mapping_rows = $rows; mapping_row_count = $rows.Count;
        separator_rows = $separators;
        mapping_table_read_method = 'GridPattern.Item.Name'; mapping_button_text = $startButton.Current.Name;
        stop_shortcut_edit = $stopKey.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value;
        add_button_enabled = (Find-Control $mainRoot ([Windows.Automation.ControlType]::Button) 'addmapdataButton' @()).Current.IsEnabled }
}

function Select-BasicKey {
    param([string]$Id, [ValidateSet('A','B')][string]$Key)
    $combo = Find-Control $mainRoot ([Windows.Automation.ControlType]::ComboBox) $Id @()
    if (-not $combo.Current.IsEnabled) { throw 'The basic key selector is not enabled.' }
    $before = @([QkmProductUiDesktop]::NamedWindows($TargetProcessId, $null))
    $bounds = $combo.Current.BoundingRectangle
    [QkmProductUiDesktop]::OpenConfigurationPopup($main, $TargetProcessId, $DesktopName,
        [int]($bounds.Left + $bounds.Width / 2), [int]($bounds.Top + $bounds.Height / 2))
    $popup = [IntPtr]::Zero
    try {
        $deadline = [DateTime]::UtcNow.AddSeconds(3)
        do {
            $candidates = @()
            foreach ($handle in @([QkmProductUiDesktop]::NamedWindows($TargetProcessId, $null) | Where-Object { $before -notcontains $_ })) {
                $candidate = [Windows.Automation.AutomationElement]::FromHandle($handle)
                $editsCondition = [Windows.Automation.PropertyCondition]::new(
                    [Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::Edit)
                $listCondition = [Windows.Automation.PropertyCondition]::new(
                    [Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::List)
                $edits = @($candidate.FindAll([Windows.Automation.TreeScope]::Descendants, $editsCondition) | Where-Object { -not $_.Current.IsOffscreen })
                $lists = @($candidate.FindAll([Windows.Automation.TreeScope]::Descendants, $listCondition) | Where-Object { -not $_.Current.IsOffscreen })
                if ($candidate.Current.ProcessId -eq $TargetProcessId -and $edits.Count -eq 1 -and $lists.Count -eq 1) {
                    $candidates += $handle
                }
            }
            if ($candidates.Count -eq 1) { break }
            Start-Sleep -Milliseconds 30
        } while ([DateTime]::UtcNow -lt $deadline)
        if ($candidates.Count -ne 1) { throw 'Expected one owned key-selector popup.' }
        $popup = $candidates[0]
        $root = [Windows.Automation.AutomationElement]::FromHandle($popup)
        $condition = [Windows.Automation.PropertyCondition]::new(
            [Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::Edit)
        $edits = @($root.FindAll([Windows.Automation.TreeScope]::Descendants, $condition) | Where-Object { -not $_.Current.IsOffscreen })
        if ($edits.Count -ne 1 -or $edits[0].Current.ProcessId -ne $TargetProcessId) { throw 'Expected one owned key search edit.' }
        $edits[0].GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).SetValue($Key)
        $item = Find-Control $root ([Windows.Automation.ControlType]::ListItem) '' @($Key)
        $bounds = $item.Current.BoundingRectangle
        [QkmProductUiDesktop]::OpenConfigurationPopup($popup, $TargetProcessId, $DesktopName,
            [int]($bounds.Left + $bounds.Width / 2), [int]($bounds.Top + $bounds.Height / 2))
        $deadline = [DateTime]::UtcNow.AddSeconds(3)
        do {
            $value = $combo.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value
            if ($value -ceq $Key) { break }
            Start-Sleep -Milliseconds 30
        } while ([DateTime]::UtcNow -lt $deadline)
        if ($value -cne $Key) { throw 'The key-selector GUI event did not select the requested key.' }
    } finally {
        if ($popup -ne [IntPtr]::Zero -and @([QkmProductUiDesktop]::NamedWindows($TargetProcessId, $null)) -contains $popup) {
            [QkmProductUiDesktop]::Key($popup, $TargetProcessId, $DesktopName, 0x1B)
        }
    }
}

$mappingObservation = $null
if ($Action -eq 'AddBasicMapping') {
    Select-Control (Find-Control $mainRoot ([Windows.Automation.ControlType]::TabItem) '' @('映射设定','Mapping','マッピング設定'))
    $before = Read-MappingView
    if ($before.configuration_display -cne '' -or $before.mapping_button_text -cne '映射开始' -or
        $before.mapping_row_count -gt 1) { throw 'Basic addition requires the bounded blank, idle fixture.' }
    Select-BasicKey 'orikeyComboBox' 'A'
    Select-BasicKey 'mapkeyComboBox' 'B'
    Invoke-Button $mainRoot 'addmapdataButton' @()
    $deadline = [DateTime]::UtcNow.AddSeconds(3)
    do {
        $mappingObservation = Read-MappingView
        if ($mappingObservation.mapping_row_count -eq $before.mapping_row_count + 1) { break }
        Start-Sleep -Milliseconds 30
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($mappingObservation.mapping_row_count -ne $before.mapping_row_count + 1) { throw 'GUI addition was not observed.' }
    $mappingObservation.gui_event_method = 'owned_key_popup_search_and_mouse_selection_UIA_add_button'
} elseif ($Action -in $mappingActions) {
    $mappingTab = Find-Control $mainRoot ([Windows.Automation.ControlType]::TabItem) '' @('映射设定','Mapping','マッピング設定')
    Select-Control $mappingTab
    if ($Action -in 'ReadTab1','ReadTab2','ReadCommon','MoveSecondUp','RestoreFirstRows','ClearBasicMapping','OpenFloatingSetup') {
        $names = switch ($Action) {
            'ReadTab2' { @('Tab2') }
            'ReadCommon' { @('共通','Common') }
            default { @('Tab1') }
        }
        Select-Control (Find-Control $mainRoot ([Windows.Automation.ControlType]::TabItem) '' $names)
    }
    if ($Action -in 'MoveSecondUp','RestoreFirstRows','ClearBasicMapping','OpenFloatingSetup') {
        $configuration = Find-Control $mainRoot ([Windows.Automation.ControlType]::ComboBox) 'settingselectComboBox' @()
        $fixedBasic = $Action -in 'ClearBasicMapping','OpenFloatingSetup'
        $expectedGroup = if ($fixedBasic) { 'Engine61' } else { 'McpSaved' }
        if ($configuration.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value -cne $expectedGroup) {
            throw 'The fixed source fixture is not selected.'
        }
        $table = Get-MappingGrid
        $grid = $table.GetCurrentPattern([Windows.Automation.GridPattern]::Pattern)
        $first = if ($Action -eq 'MoveSecondUp') { 'A' } else { 'G' }
        $second = if ($Action -eq 'MoveSecondUp') { 'G' } else { 'A' }
        if ($fixedBasic) {
            if ($grid.Current.RowCount -ne 1 -or $grid.GetItem(0,0).Current.Name -cne 'A' -or
                $grid.GetItem(0,1).Current.Name -cne 'B' -or (Read-MappingView).mapping_button_text -cne '映射开始') {
                throw 'The fixed GUI action requires exactly the owned idle A to B row.'
            }
        } elseif ($grid.Current.RowCount -gt 8 -or $grid.Current.RowCount -lt 2 -or
            $grid.GetItem(0,0).Current.Name -cne $first -or $grid.GetItem(1,0).Current.Name -cne $second) {
            throw 'The fixed source rows changed; refusing reordering.'
        }
        $cell = $grid.GetItem($(if ($fixedBasic) { 0 } else { 1 }),0)
        $selection = $null
        if ($cell.TryGetCurrentPattern([Windows.Automation.SelectionItemPattern]::Pattern, [ref]$selection)) { $selection.Select() }
        $bounds = $cell.Current.BoundingRectangle
        $before = @([QkmProductUiDesktop]::NamedWindows($TargetProcessId, $null))
        $menuWindow = [IntPtr]::Zero
        [QkmProductUiDesktop]::OpenMappingMenu($main, $TargetProcessId, $DesktopName,
            [int]($bounds.Left + $bounds.Width / 2), [int]($bounds.Top + $bounds.Height / 2))
        try {
            $deadline = [DateTime]::UtcNow.AddSeconds(3)
            do {
                $actions = @()
                $menuCandidates = @()
                foreach ($handle in @([QkmProductUiDesktop]::NamedWindows($TargetProcessId, $null) | Where-Object { $before -notcontains $_ })) {
                    $root = [Windows.Automation.AutomationElement]::FromHandle($handle)
                    $menuCandidates += "$handle/$($root.Current.NativeWindowHandle)/$($root.Current.Name)"
                    if ($root.Current.NativeWindowHandle -ne $handle.ToInt64() -or $root.Current.ProcessId -ne $TargetProcessId) { continue }
                    $items = $root.FindAll([Windows.Automation.TreeScope]::Descendants,
                        [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::MenuItem))
                    foreach ($item in $items) {
                        $wantedNames = switch ($Action) {
                            'ClearBasicMapping' { @('删除','Delete','削除') }
                            'OpenFloatingSetup' { $floatingNames }
                            default { @('上移','Move Up','上へ移動') }
                        }
                        if ($item.Current.ProcessId -eq $TargetProcessId -and $wantedNames -ccontains $item.Current.Name) {
                            $actions += @{ window = $handle; item = $item }
                        }
                    }
                }
                if ($actions.Count) { break }
                Start-Sleep -Milliseconds 30
            } while ([DateTime]::UtcNow -lt $deadline)
            if ($actions.Count -ne 1) { throw "Expected one fixed mapping GUI menu action; matches=$($actions.Count); windows=$($menuCandidates -join '; ')." }
            $menuWindow = $actions[0].window
            $actions[0].item.GetCurrentPattern([Windows.Automation.InvokePattern]::Pattern).Invoke()
        } finally {
            if ($menuWindow -ne [IntPtr]::Zero -and @([QkmProductUiDesktop]::NamedWindows($TargetProcessId, $null)) -contains $menuWindow) {
                [QkmProductUiDesktop]::Key($menuWindow, $TargetProcessId, $DesktopName, 0x1B)
            }
        }
    } elseif ($Action -notin $viewActions) {
        $configuration = Find-Control $mainRoot ([Windows.Automation.ControlType]::ComboBox) 'settingselectComboBox' @()
        if (-not $configuration.Current.IsEnabled) { throw 'Configuration selection is not enabled.' }
        $wanted = switch ($Action) {
            'SelectSaved' { 'McpSaved' }
            'SelectSecond' { 'McpSecond' }
            'SelectInvalid' { 'McpInvalid' }
            'SelectBlank' { '' }
            'ReloadCurrent' {
                $value = $configuration.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value
                if ($value -cne 'McpSaved') { throw 'ReloadCurrent requires the fixed McpSaved fixture.' }
                $value
            }
        }
        $before = @([QkmProductUiDesktop]::NamedWindows($TargetProcessId, $null))
        # Qt's custom popup is not the standard combo view described by UIA ExpandCollapse.
        $openMethod = 'owned_window_combo_left_click_Up_Down_Return'
        $bounds = $configuration.Current.BoundingRectangle
        [QkmProductUiDesktop]::OpenConfigurationPopup($main, $TargetProcessId, $DesktopName,
            [int]($bounds.Left + $bounds.Width / 2), [int]($bounds.Top + $bounds.Height / 2))
        $popup = [IntPtr]::Zero
        $popupObservationErrors = @()
        try {
            $deadline = [DateTime]::UtcNow.AddSeconds(3)
            do {
                $popups = @([QkmProductUiDesktop]::NamedWindows($TargetProcessId, $null) | Where-Object { $before -notcontains $_ })
                $candidates = @()
                $popupDetails = @()
                foreach ($handle in $popups) {
                    try {
                        $root = [Windows.Automation.AutomationElement]::FromHandle($handle)
                        $listItems = $root.FindAll([Windows.Automation.TreeScope]::Descendants,
                            [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::ListItem))
                        $popupDetails += @{ window = $handle.ToInt64(); item_count = $listItems.Count;
                            native_window = $root.Current.NativeWindowHandle; control_type = $root.Current.ControlType.ProgrammaticName }
                        if ($root.Current.ProcessId -eq $TargetProcessId -and $root.Current.NativeWindowHandle -eq $handle.ToInt64() -and $listItems.Count -eq 5) {
                            $candidates += @{ window = $handle; items = $listItems }
                        }
                    } catch {
                        # Observation can race popup/helper-window creation; no input is sent to a failed candidate.
                        if ($popupObservationErrors.Count -lt 16) {
                            $failure = $_.Exception.GetBaseException()
                            $popupObservationErrors += @{ window = $handle.ToInt64(); error = $failure.Message;
                                exception_type = $failure.GetType().FullName; hresult = $failure.HResult.ToString('X8') }
                        }
                    }
                }
                if ($candidates.Count) { break }
                Start-Sleep -Milliseconds 30
            } while ([DateTime]::UtcNow -lt $deadline)
            if ($candidates.Count -ne 1) { throw "Expected one owned five-item configuration popup: $($popupDetails | ConvertTo-Json -Compress); observation errors: $($popupObservationErrors | ConvertTo-Json -Compress)." }
            $popup = $candidates[0].window
            $items = $candidates[0].items
            $wantedNames = if ($wanted -eq '') { @('(空白设定)','(Empty Setting)','(空白設定)') } else { @($wanted) }
            $indices = @(for ($index = 0; $index -lt $items.Count; ++$index) {
                if ($items[$index].Current.ProcessId -eq $TargetProcessId -and $wantedNames -ccontains $items[$index].Current.Name) { $index }
            })
            if ($indices.Count -ne 1) { throw "Expected one fixed configuration popup item '$wanted'." }
            # The custom popup focuses its search edit; Up clamps the highlighted list row, Home only moves the caret.
            for ($index = 0; $index -lt $items.Count; ++$index) {
                [QkmProductUiDesktop]::Key($popup, $TargetProcessId, $DesktopName, 0x26)
            }
            for ($index = 0; $index -lt $indices[0]; ++$index) {
                [QkmProductUiDesktop]::Key($popup, $TargetProcessId, $DesktopName, 0x28)
            }
            [QkmProductUiDesktop]::Key($popup, $TargetProcessId, $DesktopName, 0x0D)
        } finally {
            if ($popup -ne [IntPtr]::Zero -and @([QkmProductUiDesktop]::NamedWindows($TargetProcessId, $null)) -contains $popup) {
                [QkmProductUiDesktop]::Key($popup, $TargetProcessId, $DesktopName, 0x1B)
            }
        }
    }
    if ($Action -eq 'ClearBasicMapping') {
        $deadline = [DateTime]::UtcNow.AddSeconds(3)
        do {
            $observation = Read-MappingView
            if ($observation.mapping_row_count -eq 0) { break }
            Start-Sleep -Milliseconds 30
        } while ([DateTime]::UtcNow -lt $deadline)
        if ($observation.mapping_row_count -ne 0) { throw 'The fixed Delete GUI action did not clear the row.' }
    }
    if ($Action -eq 'OpenFloatingSetup') { $null = Find-OwnedWindow $floatingNames }
    $mappingObservation = Read-MappingView
    $mappingObservation.gui_event_method = if ($Action -in 'MoveSecondUp','RestoreFirstRows') {
        'owned_cell_right_click_UIA_MoveUp'
    } elseif ($Action -eq 'ClearBasicMapping') { 'owned_cell_right_click_UIA_Delete' }
    elseif ($Action -in 'ReadTab1','ReadTab2','ReadCommon') { 'owned_UIA_tab_select' }
    elseif ($Action -eq 'ReadMapping') { 'read_only' } else { $openMethod }
    if ($Action -notin $viewActions) {
        $mappingObservation.requested_configuration = $wanted
        $mappingObservation.popup_candidates = $popupDetails
        $mappingObservation.popup_observation_errors = $popupObservationErrors
    }
} elseif ($Action -in 'SyncGroupOne','SyncGroupZero','CloseFloatingSetup') {
    $dialog = Find-OwnedWindow $floatingNames
    if ($Action -eq 'CloseFloatingSetup') {
        $dialog.GetCurrentPattern([Windows.Automation.WindowPattern]::Pattern).Close()
    } else {
        $condition = [Windows.Automation.PropertyCondition]::new(
            [Windows.Automation.AutomationElement]::ControlTypeProperty, [Windows.Automation.ControlType]::Spinner)
        $groups = @()
        foreach ($box in $dialog.FindAll([Windows.Automation.TreeScope]::Descendants, $condition)) {
            $pattern = $null
            if ($box.Current.ProcessId -eq $TargetProcessId -and $box.Current.IsEnabled -and
                $box.TryGetCurrentPattern([Windows.Automation.RangeValuePattern]::Pattern, [ref]$pattern) -and
                $pattern.Current.Minimum -eq 0 -and $pattern.Current.Maximum -eq 999 -and -not $pattern.Current.IsReadOnly) {
                $groups += $pattern
            }
        }
        if ($groups.Count -ne 1) { throw 'Expected the unique writable Group ID range 0 to 999.' }
        $wanted = if ($Action -eq 'SyncGroupOne') { 1 } else { 0 }
        if ($groups[0].Current.Value -ne (1 - $wanted)) { throw 'The fixed Group ID source value changed.' }
        $groups[0].SetValue($wanted)
        Invoke-Button $dialog '' @('应用','Apply','適用')
        if ($groups[0].Current.Value -ne $wanted) { throw 'The fixed Group ID GUI value was not observed.' }
    }
} elseif ($Action -eq 'ShowForCapture') {
    foreach ($handle in @([QkmProductUiDesktop]::NamedWindows($TargetProcessId,
        @('通用高级设定','General Advanced Setting','一般高度設定') + $mcpNames))) {
        [Windows.Automation.AutomationElement]::FromHandle($handle).GetCurrentPattern([Windows.Automation.WindowPattern]::Pattern).Close()
    }
    Select-Control (Find-Control $mainRoot ([Windows.Automation.ControlType]::TabItem) '' @('映射设定','Mapping','マッピング設定'))
    [QkmProductUiDesktop]::RaiseForSnapshot($main, $TargetProcessId, $DesktopName)
    $mappingObservation = Read-MappingView
} elseif ($Action -in 'StartMapping','StopMapping') {
    $button = Find-Control $mainRoot ([Windows.Automation.ControlType]::Button) 'keymapButton' @()
    $before = $button.Current.Name
    if ($before -notin '映射开始','映射停止') { throw 'Unexpected mapping button state; refusing to toggle.' }
    $wanted = if ($Action -eq 'StartMapping') { '映射停止' } else { '映射开始' }
    if ($before -ne $wanted -and $button.Current.IsEnabled) {
        $button.GetCurrentPattern([Windows.Automation.InvokePattern]::Pattern).Invoke()
    }
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    do {
        $button = Find-Control $mainRoot ([Windows.Automation.ControlType]::Button) 'keymapButton' @()
        if ($button.Current.Name -eq $wanted -and $button.Current.IsEnabled) { break }
        Start-Sleep -Milliseconds 50
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($button.Current.Name -ne $wanted -or -not $button.Current.IsEnabled) { throw 'Mapping button did not reach the requested state.' }
    # A button observation is not proof that all input or worker cleanup is complete.
    $mappingActionObservation = @{ before = $before; after = $button.Current.Name; button_enabled = $button.Current.IsEnabled;
        add_button_enabled = (Find-Control $mainRoot ([Windows.Automation.ControlType]::Button) 'addmapdataButton' @()).Current.IsEnabled }
    $windowPattern.SetWindowVisualState([Windows.Automation.WindowVisualState]::Minimized)
    $mappingActionObservation.minimized_for_receiver = $windowPattern.Current.WindowVisualState -eq [Windows.Automation.WindowVisualState]::Minimized
    if (-not $mappingActionObservation.minimized_for_receiver) { throw 'Owned product did not minimize for ordinary receiver focus.' }
} elseif ($Action -eq 'OpenAdvanced') {
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
        { $_ -in 'ApproveRead','ApproveConfigure' } {
            if (-not $ClientExecutable) { throw 'A trusted client executable is required.' }
            $field = Find-Control $dialog ([Windows.Automation.ControlType]::Edit) 'mcpClientExecutable' @('系统核实的客户端程序路径','Verified client executable path','確認済みクライアントのパス')
            $actual = $field.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern).Current.Value
            if ($actual -ine [IO.Path]::GetFullPath($ClientExecutable)) { throw 'Verified client path differs; refusing approval.' }
            $read = Check-State $dialog '' @('读取状态与设定（必需）','Read state and configuration (required)','状態と設定の読み取り（必須）')
            $configure = Check-State $dialog 'mcpConfigure' @('更改映射与软件设定','Change mappings and settings','マッピングと設定の変更')
            $run = Check-State $dialog 'mcpRunMapping' @('开始与停止映射（可能发送输入）','Start and stop mapping (may send input)','マッピングの開始と停止（入力を送信する場合あり）')
            if ($read -ne [Windows.Automation.ToggleState]::On -or $configure -ne [Windows.Automation.ToggleState]::Off -or
                $run -ne [Windows.Automation.ToggleState]::Off) { throw 'Only read permission may be approved by this test helper.' }
            if ($Action -eq 'ApproveConfigure') {
                $box = Find-Control $dialog ([Windows.Automation.ControlType]::CheckBox) 'mcpConfigure' @('更改映射与软件设定','Change mappings and settings','マッピングと設定の変更')
                $box.GetCurrentPattern([Windows.Automation.TogglePattern]::Pattern).Toggle()
                if ((Check-State $dialog 'mcpConfigure' @('更改映射与软件设定','Change mappings and settings','マッピングと設定の変更')) -ne [Windows.Automation.ToggleState]::On -or
                    (Check-State $dialog 'mcpRunMapping' @('开始与停止映射（可能发送输入）','Start and stop mapping (may send input)','マッピングの開始と停止（入力を送信する場合あり）')) -ne [Windows.Automation.ToggleState]::Off) {
                    throw 'Expected Configure only, without RunMapping.'
                }
            }
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
        'CloseMcp' { Invoke-Button $dialog 'mcpClose' @('关闭','Close','閉じる') }
    }
}
$result = @{ success = $true; action = $Action; desktop = [QkmProductUiDesktop]::Name($main); window = $main.ToInt64() }
if ($null -ne $mappingActionObservation) { $result.mapping_action = $mappingActionObservation }
if ($null -ne $mappingObservation) {
    foreach ($entry in $mappingObservation.GetEnumerator()) { $result[$entry.Key] = $entry.Value }
}
if ($Action -eq 'OpenAdvanced') {
    $result.stop_shortcut_edit = $stopShortcut
    $result.mapping_button_text = $mappingButtonText
    $result.mapping_tab_name = $sourceTabName
    $result.configuration_display = $configurationDisplay
    $result.configuration_read_method = 'ValuePattern'
}
if ($Action -in 'OpenMcp','Enable','ApproveRead','ApproveConfigure','Revoke','Disable') {
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
[IO.File]::WriteAllText($resultPath, ($result | ConvertTo-Json -Depth 6 -Compress), [Text.UTF8Encoding]::new($false))
