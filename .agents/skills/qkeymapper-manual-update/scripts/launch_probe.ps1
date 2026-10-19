[CmdletBinding()]
param([string]$Mt, [ValidateRange(30,7200)][int]$TimeoutSeconds = 1800)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..'))
$observer = Join-Path $repo 'out/user_manual/observer'
$runtime = Join-Path $observer 'runtime'
$source = Join-Path $runtime 'QkmManualProbe.exe'
$ui = Join-Path $runtime 'QkmManualUiProbe.exe'
if (-not (Test-Path -LiteralPath $source)) { throw 'Build the UI observer first' }
if (-not $Mt) {
    $tool = Get-Command mt.exe -ErrorAction SilentlyContinue
    if ($tool) { $Mt = $tool.Source }
    else {
        $kits = @('HKLM:/SOFTWARE/Microsoft/Windows Kits/Installed Roots','HKLM:/SOFTWARE/WOW6432Node/Microsoft/Windows Kits/Installed Roots') | ForEach-Object { Get-ItemProperty $_ -ErrorAction SilentlyContinue } | Where-Object KitsRoot10 | Select-Object -First 1
        $sdkRoots = @($kits.KitsRoot10, $env:WindowsSdkDir, (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10'))
        foreach ($sdkRoot in $sdkRoots) {
            if ($sdkRoot -and (Test-Path -LiteralPath (Join-Path $sdkRoot 'bin'))) {
                $Mt = Get-ChildItem (Join-Path $sdkRoot 'bin') -Filter mt.exe -Recurse | Where-Object FullName -Match '[\\/]x64[\\/]' | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
                if ($Mt) { break }
            }
        }
    }
}
if (-not $Mt -or -not (Test-Path -LiteralPath $Mt)) { throw 'Missing Windows SDK mt.exe; pass -Mt' }
$session = Join-Path $observer ('sessions/' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $session | Out-Null
# Each session owns its EXE/INI: never overwrite a running observer's configuration.
Get-ChildItem -LiteralPath $runtime | Copy-Item -Destination $session -Recurse
$source = Join-Path $session 'QkmManualProbe.exe'
$ui = Join-Path $session 'QkmManualUiProbe.exe'
Copy-Item -LiteralPath $source -Destination $ui
$manifest = Join-Path $session 'ui_copy.manifest'
& $Mt -nologo ("-inputresource:$source;#1") ("-out:$manifest")
if ($LASTEXITCODE -ne 0) { throw 'Manifest extraction failed' }
$text = [IO.File]::ReadAllText($manifest)
if ($text -notmatch 'requireAdministrator') { throw 'Review changed source manifest before preparing an observer' }
[IO.File]::WriteAllText($manifest,$text.Replace('requireAdministrator','asInvoker'),[Text.UTF8Encoding]::new($false))
& $Mt -nologo -manifest $manifest ("-outputresource:$ui;#1")
if ($LASTEXITCODE -ne 0) { throw 'UI-only manifest failed' }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../assets/keymapdata.ini') -Destination $session
$env:QKM_MANUAL_SESSION = $session
Remove-Item Env:QTDIR,Env:QT_SCALE_FACTOR,Env:QT_STYLE_OVERRIDE -ErrorAction SilentlyContinue
Write-Output "Session=$session"
& (Join-Path $PSScriptRoot 'run_private.ps1') -ExecutablePath $ui -WorkingDirectory $session -LogPath (Join-Path $session 'program.log') -TimeoutSeconds $TimeoutSeconds
$code = $LASTEXITCODE
[IO.File]::WriteAllText((Join-Path $session 'exit.txt'),[string]$code)
exit $code
