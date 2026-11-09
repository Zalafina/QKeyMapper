#requires -Version 5.1
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../../RazerDriver/manage_driver.ps1')
$script:Checks=0
function Check([bool]$Value,[string]$Message) { if (-not $Value) { throw "FAIL: $Message" }; $script:Checks++ }
function Node([string]$Id,[string[]]$Hardware,[string]$Class,[int]$Problem=0,[string]$Guid='') {
    [pscustomobject]@{ PNPDeviceID=$Id; HardwareID=$Hardware; PNPClass=$Class; ClassGuid=$Guid; ConfigManagerErrorCode=$Problem; Status='OK'; Name=$Id }
}
$virtual=@(
    (Node 'ROOT\RAZER_DEVICE\0000' @($script:BusId) 'Razer Device' 0 $script:ClassId),
    (Node 'HID\virtual-kbd' @('HID\VID_1532&PID_0306&MI_00&Col01') 'Keyboard'),
    (Node 'HID\virtual-mou' @('HID\VID_1532&PID_0306&MI_00&Col00') 'Mouse')
)
$physical=Node 'HID\physical-razer' @('HID\VID_1532&PID_9999') 'Keyboard'
$script:Nodes=@($virtual)+@($physical)
$script:Calls=[Collections.Generic.List[object]]::new()
$script:FailAt=0; $script:ReturnCode=0; $script:KeepBus=$false; $script:Confirmation='INSTALL'
$script:Admin=$false; $script:Mapper=$false
function Get-RazerDevices { $script:Nodes }
function Test-Administrator { $script:Admin }
function Get-Process { if ($script:Mapper) { [pscustomobject]@{ Name='QKeyMapper'; Id=123 } } }
function Read-Host { $script:Confirmation }
function Start-Sleep {}
function Invoke-DriverTool([string]$FilePath,[string[]]$Arguments) {
    $script:Calls.Add([pscustomobject]@{ File=$FilePath; Arguments=$Arguments })
    $code=if ($script:FailAt -eq $script:Calls.Count) { $script:ReturnCode } else { 0 }
    if ($code -notin @(0,3010)) { return $code }
    if ($Arguments[0] -eq 'install') { $script:Nodes=@($virtual)+@($physical) }
    if ($Arguments[0] -eq '/remove-device' -and -not $script:KeepBus) {
        $script:Nodes=@($script:Nodes | Where-Object { $_.PNPDeviceID -cne $Arguments[1] })
    }
    return $code
}
function Reset-Case {
    $script:Calls.Clear(); $script:FailAt=0; $script:ReturnCode=0; $script:KeepBus=$false
    $script:Nodes=@($virtual)+@($physical); $script:Confirmation='INSTALL'
}
$state=Get-RazerState
Check ($state.BusReady -and $state.KeyboardReady -and $state.MouseReady) 'healthy bus and child nodes'
Check ($state.Buses.Count -eq 1 -and $state.Nodes.Count -eq 3) 'physical Razer device excluded'
$script:Nodes=@((Node 'ROOT\wrong-class' @($script:BusId) 'System' 0 '{00000000-0000-0000-0000-000000000000}'),$physical)
Check ((Get-RazerState).Buses.Count -eq 0) 'hardware ID alone cannot select another class'
$script:Nodes=@((Node 'ROOT\bad-bus' @($script:BusId) 'Razer Device' 10 $script:ClassId))
Check (-not (Get-RazerState).BusReady) 'problem node is not ready'
$failure=$false; try { Assert-DriverChangesAllowed } catch { $failure=$true }
Check ($failure -and $script:Calls.Count -eq 0) 'ordinary caller cannot change devices'
$script:Admin=$true; $script:Mapper=$true
$failure=$false; try { Assert-DriverChangesAllowed } catch { $failure=$true }
Check ($failure) 'running mapper blocks device changes'
$script:Mapper=$false

Reset-Case
Check ((Install-RazerDriver) -eq 0) 'existing healthy installation succeeds'
Check ($script:Calls.Count -eq 6) 'existing bus is retained without remove or duplicate create'
Reset-Case; $script:Nodes=@($physical)
Check ((Install-RazerDriver) -eq 0) 'missing virtual bus creates after registration'
Check ($script:Calls.Count -eq 7 -and $script:Calls[6].Arguments[0] -ceq 'install') 'all INF registrations precede root creation'
Reset-Case; $script:FailAt=3; $script:ReturnCode=5
Check ((Install-RazerDriver) -eq 5 -and $script:Calls.Count -eq 3) 'INF failure is propagated and stops installation'
Reset-Case; $script:FailAt=2; $script:ReturnCode=3010
Check ((Install-RazerDriver) -eq 3010) 'reboot requirement survives later successes'
Reset-Case; $script:Confirmation='cancel'
Check ((Install-RazerDriver) -eq 1223 -and $script:Calls.Count -eq 0) 'cancelled confirmation executes no native tool'

Reset-Case
Check ((Remove-RazerDevice) -eq 0 -and $script:Calls.Count -eq 1) 'one exact virtual bus is removed'
Check ($script:Calls[0].File -like '*\pnputil.exe' -and $script:Calls[0].Arguments.Count -eq 2 -and
    $script:Calls[0].Arguments[0] -ceq '/remove-device' -and $script:Calls[0].Arguments[1] -ceq 'ROOT\RAZER_DEVICE\0000') 'instance-targeted PnPUtil command only'
Check (@($script:Nodes | Where-Object PNPDeviceID -eq $physical.PNPDeviceID).Count -eq 1) 'physical device remains outside removal'
Reset-Case; $script:Nodes=@($physical)
Check ((Remove-RazerDevice) -eq 0 -and $script:Calls.Count -eq 0) 'absent virtual bus performs no native action'
Reset-Case; $script:FailAt=1; $script:ReturnCode=5
Check ((Remove-RazerDevice) -eq 5) 'remove native failure propagated'
Reset-Case; $script:FailAt=1; $script:ReturnCode=3010; $script:KeepBus=$true
Check ((Remove-RazerDevice) -eq 3010) 'pending-reboot removal not claimed complete'
Reset-Case; $script:KeepBus=$true
Check ((Remove-RazerDevice) -eq 1) 'still-present node is not reported removed'
$source=Get-Content -LiteralPath (Join-Path $PSScriptRoot '../../RazerDriver/manage_driver.ps1') -Raw -Encoding UTF8
Check ($source -notmatch '--remove-device-node|--uninstall-driver|/delete-driver|--remove-class-filter') 'no package-deletion fallback or class-filter edits'
Write-Host "PASS $script:Checks checks; CIM, privilege, prompts and native driver tools substituted. No real device operation."
