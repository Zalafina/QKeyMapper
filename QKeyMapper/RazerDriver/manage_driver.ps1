#requires -Version 5.1
[CmdletBinding()]
param([ValidateSet('menu','1','2','3','install','uninstall','check','status')][string]$Action='menu')
$ErrorActionPreference='Stop'
$script:BusId='RAZER\VirtualBus\VID_1532&PID_0306'
$script:ClassId='{1750F915-5639-497C-966C-3A65ACECFCB6}'
$script:LogPath=Join-Path $env:TEMP ('QKeyMapper-RazerDriver-'+[guid]::NewGuid().ToString('N')+'.log')
function Test-Administrator {
    $identity=[Security.Principal.WindowsIdentity]::GetCurrent()
    return ([Security.Principal.WindowsPrincipal]$identity).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}
function Get-RazerDevices { @(Get-CimInstance Win32_PnPEntity -ErrorAction Stop) }
function Get-RazerState {
    $nodes=@(Get-RazerDevices)
    $buses=@($nodes | Where-Object {
        $_.ClassGuid -ieq $script:ClassId -and $_.HardwareID -icontains $script:BusId -and $_.PNPDeviceID -ilike 'ROOT\*'
    })
    $keyboard=@($nodes | Where-Object { $_.PNPClass -ieq 'Keyboard' -and $_.HardwareID -icontains 'HID\VID_1532&PID_0306&MI_00&Col01' })
    $mouse=@($nodes | Where-Object { $_.PNPClass -ieq 'Mouse' -and $_.HardwareID -icontains 'HID\VID_1532&PID_0306&MI_00&Col00' })
    [pscustomobject]@{
        Buses=$buses
        BusReady=@($buses | Where-Object { $_.ConfigManagerErrorCode -eq 0 -and $_.Status -eq 'OK' }).Count -gt 0
        KeyboardReady=@($keyboard | Where-Object { $_.ConfigManagerErrorCode -eq 0 -and $_.Status -eq 'OK' }).Count -gt 0
        MouseReady=@($mouse | Where-Object { $_.ConfigManagerErrorCode -eq 0 -and $_.Status -eq 'OK' }).Count -gt 0
        Nodes=@($buses)+@($keyboard)+@($mouse)
    }
}
function Show-RazerState {
    $state=Get-RazerState
    foreach ($item in @(@('虚拟总线',$state.BusReady),@('虚拟键盘',$state.KeyboardReady),@('虚拟鼠标',$state.MouseReady))) {
        $text=if ($item[1]) { '已检测到' } else { '未就绪' }
        Write-Host ('{0}：{1}' -f $item[0],$text)
    }
    Write-Host '以上是设备状态；请在 QKeyMapper 选择 Razer 后点击“连接检查”确认通道。'
    $state.Nodes | Select-Object Name,Status,ConfigManagerErrorCode,PNPDeviceID | Format-Table -AutoSize | Out-Host
}
function Assert-DriverChangesAllowed {
    if (-not [Environment]::Is64BitOperatingSystem -or
        ($env:PROCESSOR_ARCHITECTURE -ine 'AMD64' -and $env:PROCESSOR_ARCHITEW6432 -ine 'AMD64')) {
        throw '本驱动包仅适用于 AMD64 Windows，不支持 x86 或 ARM64 系统。'
    }
    if ([Environment]::OSVersion.Version.Major -lt 10) { throw '本管理工具面向 Windows 10/11；更早系统未验证。' }
    if (-not (Test-Administrator)) { throw '安装或移除虚拟设备需要管理员权限。' }
    if (@(Get-Process -Name 'QKeyMapper*' -ErrorAction SilentlyContinue).Count) {
        throw '请先退出所有 QKeyMapper 实例，再修改虚拟设备。'
    }
}
function Invoke-DriverTool([string]$FilePath,[string[]]$Arguments) {
    Add-Content -LiteralPath $script:LogPath -Value ($FilePath+' '+($Arguments -join ' ')) -Encoding UTF8
    $ErrorActionPreference='Continue'
    & $FilePath @Arguments 2>&1 | Out-File -LiteralPath $script:LogPath -Append -Encoding UTF8
    return [int]$LASTEXITCODE
}
function Install-RazerDriver {
    Assert-DriverChangesAllowed
    Write-Host '这是可选的旧版 Razer 驱动包。已有 Razer 设备或 Synapse 的用户，请先阅读 README 并尝试连接检查。' -ForegroundColor Yellow
    if ((Read-Host '确认需要安装时请输入 INSTALL，其他输入取消') -cne 'INSTALL') { return 1223 }
    $driver=Join-Path $PSScriptRoot 'x64'
    $tool=Join-Path $driver 'nefconw.exe'
    $infs=@('RzCommonU.inf','RzDevU_0306_Kbd.inf','RzDevU_0306_Mou.inf','RzDevU_0306_VCon.inf','RzDevU_0306_VKbd.inf','RzDevU_0306_VMou.inf')
    foreach ($name in @('nefconw.exe','RzCommon.sys','RzDev_0306.sys','RzDevU_0306_VBus.inf')+$infs) {
        if (-not (Test-Path -LiteralPath (Join-Path $driver $name) -PathType Leaf)) { throw "缺少文件：$name" }
    }
    $reboot=$false
    foreach ($inf in $infs) {
        Write-Host "注册 $inf"
        $code=Invoke-DriverTool $tool @('--install-driver','--inf-path',(Join-Path $driver $inf))
        if ($code -eq 3010) { $reboot=$true } elseif ($code -ne 0) { Write-Host "注册失败：$inf，退出码 $code" -ForegroundColor Red; return $code }
    }
    $before=Get-RazerState
    if ($before.Buses.Count -eq 0) {
        $code=Invoke-DriverTool $tool @('install',(Join-Path $driver 'RzDevU_0306_VBus.inf'),$script:BusId)
        if ($code -eq 3010) { $reboot=$true } elseif ($code -ne 0) { Write-Host "创建总线失败，退出码 $code" -ForegroundColor Red; return $code }
        Start-Sleep -Seconds 2
    }
    # Existing nodes are never removed as an installation pre-step.
    if ($reboot) { Write-Host 'Windows 要求重启。请方便时自行重启，再检查设备与连接。' -ForegroundColor Yellow; return 3010 }
    $after=Get-RazerState
    if (-not ($after.BusReady -and $after.KeyboardReady -and $after.MouseReady)) {
        Write-Host '驱动命令已结束，但虚拟总线、键盘或鼠标尚未全部就绪。请查看状态及日志，不要反复安装。' -ForegroundColor Yellow
        return 1
    }
    Write-Host '虚拟键鼠已检测到。请在 QKeyMapper 选择 Razer 并执行连接检查。' -ForegroundColor Green
    return 0
}
function Remove-RazerDevice {
    Assert-DriverChangesAllowed
    if ([Environment]::OSVersion.Version.Build -lt 19041) {
        throw '当前系统不支持 PnPUtil /remove-device。请通过设备管理器移除本包的虚拟总线，并保留驱动包；本工具不会改用驱动包删除。'
    }
    $state=Get-RazerState
    if ($state.Buses.Count -eq 0) { Write-Host '未检测到本包的虚拟总线，无需移除。'; return 0 }
    $system=if ([Environment]::Is64BitOperatingSystem -and -not [Environment]::Is64BitProcess) { 'Sysnative' } else { 'System32' }
    $tool=Join-Path $env:WINDIR "$system\pnputil.exe"
    $reboot=$false
    foreach ($bus in $state.Buses) {
        # Match the exact virtual hardware ID and class before using its current instance ID.
        $code=Invoke-DriverTool $tool @('/remove-device',[string]$bus.PNPDeviceID)
        if ($code -eq 3010) { $reboot=$true } elseif ($code -ne 0) { Write-Host "移除失败，退出码 $code" -ForegroundColor Red; return $code }
    }
    if ($reboot) { Write-Host '设备移除需要重启。请方便时自行重启后确认。' -ForegroundColor Yellow; return 3010 }
    if ((Get-RazerState).Buses.Count) { Write-Host '虚拟总线仍存在，请查看状态及日志。' -ForegroundColor Yellow; return 1 }
    Write-Host '本包虚拟设备已移除，驱动包仍保留在 Windows 驱动存储区。' -ForegroundColor Green
    return 0
}
function Invoke-SelectedAction([string]$Selected) {
    if ($Selected -eq 'install') { return Install-RazerDriver }
    if ($Selected -eq 'uninstall') { return Remove-RazerDevice }
    Show-RazerState
    return 0
}
function Start-AdministratorAction([string]$Selected) {
    $exe=(Get-Process -Id $PID).Path
    $arguments='-NoProfile -ExecutionPolicy Bypass -File "'+$PSCommandPath+'" -Action '+$Selected
    # This is a user-operated interactive tool; keep its elevated result visible until acknowledged.
    $child=Start-Process -FilePath $exe -ArgumentList $arguments -Verb RunAs -WindowStyle Normal -Wait -PassThru
    return [int]$child.ExitCode
}
if ($MyInvocation.InvocationName -eq '.') { return }
$selected=switch ($Action) { '1' {'install'} '2' {'uninstall'} '3' {'status'} 'check' {'status'} default {$Action} }
try {
    if ($selected -eq 'status') { Show-RazerState; exit 0 }
    if ($selected -ne 'menu') {
        if (-not (Test-Administrator)) { exit (Start-AdministratorAction $selected) }
        $code=Invoke-SelectedAction $selected
        Write-Host "结果码：$code；日志：$script:LogPath"
        [void](Read-Host '按 Enter 关闭')
        exit $code
    }
    do {
        Clear-Host
        Write-Host 'Razer 虚拟设备管理工具（PID 0306）' -ForegroundColor Cyan
        try { Show-RazerState } catch { Write-Host ('无法查询设备状态：'+$_.Exception.Message) -ForegroundColor Yellow }
        Write-Host '[1] 安装／修复驱动  [2] 移除虚拟设备（保留驱动包）  [3] 刷新状态  [0] 退出'
        $choice=Read-Host '请选择'
        if ($choice -in @('1','2')) {
            $next=if ($choice -eq '1') { 'install' } else { 'uninstall' }
            if (-not (Test-Administrator)) { $code=Start-AdministratorAction $next }
            else { $code=Invoke-SelectedAction $next; Write-Host "结果码：$code；日志：$script:LogPath"; [void](Read-Host '按 Enter 返回') }
        }
    } while ($choice -ne '0')
    exit 0
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host "如已执行驱动命令，请保留日志：$script:LogPath"
    [void](Read-Host '按 Enter 关闭')
    exit 1
}
