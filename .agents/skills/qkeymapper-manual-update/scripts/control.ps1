# Dot-source this file from PowerShell 7.
$ErrorActionPreference = 'Stop'
function Send-ManualCommand {
    param([Parameter(Mandatory)][string]$Session,[Parameter(Mandatory)][hashtable]$Command,[int]$TimeoutSeconds = 25)
    if (-not $Command.id) { $Command.id = [guid]::NewGuid().ToString('N') }
    if ($Command.id -notmatch '^[a-zA-Z0-9_-]+$') { throw 'Request id must be a filename component' }
    $response = Join-Path $Session ('response_' + $Command.id + '.json')
    if (Test-Path -LiteralPath $response) { throw 'Request id already used' }
    if (Test-Path -LiteralPath (Join-Path $Session 'request.json')) { throw 'Wait for the previous request to finish' }
    $temp = Join-Path $Session 'request.tmp'
    [IO.File]::WriteAllText($temp,($Command | ConvertTo-Json -Depth 12),[Text.UTF8Encoding]::new($false))
    Move-Item -LiteralPath $temp -Destination (Join-Path $Session 'request.json')
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    do {
        $result = $null
        if (Test-Path -LiteralPath $response) {
            try { $result = Get-Content -LiteralPath $response -Raw | ConvertFrom-Json -AsHashtable } catch { }
        }
        if ($result -and $result.id -eq $Command.id -and $result.roots) {
            if ($result.extra.error) { throw $result.extra.error }
            return $result
        }
        if ((Get-Date) -gt $deadline) { throw "Observer response timed out: $($Command.id)" }
        Start-Sleep -Milliseconds 40
    } while ($true)
}
function Save-ManualCapture {
    param([Parameter(Mandatory)][hashtable]$State,[string]$Root = 'main',[Parameter(Mandatory)][string]$Destination,[string]$Label = 'manual')
    $target = $State.roots[$Root]
    if (-not $target.nativeVisible -or -not $target.visible -or $target.desktop -notmatch '^QkmManual_[0-9a-f]{32}$') { throw 'Capture needs a visible owned private-desktop window' }
    if ($Label -notmatch '^[a-zA-Z0-9_-]+$') { throw 'Use a simple capture label' }
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    $job = @{label=$Label;pid=$target.pid;hwnd=$target.hwnd;desktop=$target.desktop;destination=[IO.Path]::GetFullPath($Destination)}
    $request = Join-Path $Destination ("capture_$Label.json")
    [IO.File]::WriteAllText($request,($job | ConvertTo-Json),[Text.UTF8Encoding]::new($false))
    $worker = Join-Path $PSScriptRoot 'capture_worker.ps1'
    $shell = (Get-Process -Id $PID).Path
    $arguments = "-NoProfile -File `"$worker`" -Request `"$request`""
    & $shell -NoProfile -File (Join-Path $PSScriptRoot 'run_on_desktop.ps1') -ExecutablePath $shell -Desktop $target.desktop ("-Arguments:$arguments") -LogPath (Join-Path $Destination ("capture_$Label.log")) -TimeoutSeconds 30
    if ($LASTEXITCODE -ne 0) { throw 'Private capture failed; inspect capture log' }
    $png = Join-Path $Destination ("snapshot_$Label.png")
    if (-not (Test-Path -LiteralPath $png)) { throw 'PNG missing' }
    return $png
}
