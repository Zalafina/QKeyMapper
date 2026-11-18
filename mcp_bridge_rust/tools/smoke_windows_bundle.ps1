[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Archive,
    [Parameter(Mandatory)][ValidatePattern('^[0-9a-fA-F]{64}$')][string]$Sha256,
    [Parameter(Mandatory)][string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
if (-not [Environment]::Is64BitProcess) { throw 'Use Windows x64 PowerShell.' }
$zip = (Resolve-Path -LiteralPath $Archive).Path
if ((Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash -ine $Sha256) { throw 'Archive digest mismatch.' }
$destination = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $destination) { throw 'Use a new extraction directory.' }
Expand-Archive -LiteralPath $zip -DestinationPath $destination
$roots = @(Get-ChildItem -LiteralPath $destination -Directory)
if ($roots.Count -ne 1) { throw 'Expected one component directory.' }
$root = $roots[0].FullName
$manifest = Get-Content -LiteralPath (Join-Path $root 'bundle-manifest.json') -Raw -Encoding UTF8 | ConvertFrom-Json
if ($manifest.schema_version -ne 1 -or $manifest.architecture -ne 'x86_64-pc-windows-msvc' -or
    $manifest.binary -cnotin @('qkeymapper-mcp-rust.exe', 'qkeymapper_mcp.exe') -or $null -eq $manifest.app_local) {
    throw 'Unexpected bundle format.'
}
$needsCrt = $manifest.binary -ceq 'qkeymapper-mcp-rust.exe'
if ($needsCrt -ne [bool]$manifest.app_local.PSObject.Properties['VCRUNTIME140.dll']) {
    throw 'The bundled CRT does not match the executable type.'
}
foreach ($file in $manifest.files) {
    $path = [IO.Path]::GetFullPath((Join-Path $root $file.path))
    if (-not $path.StartsWith(($root + [IO.Path]::DirectorySeparatorChar), [StringComparison]::OrdinalIgnoreCase) -or
        (Get-Item -LiteralPath $path).Length -ne $file.bytes -or
        (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ine $file.sha256) {
        throw "Invalid package member $($file.path)"
    }
}
if (@(Get-ChildItem -LiteralPath $root -Recurse -File).Count -ne $manifest.files.Count + 1) {
    throw 'Package contains unlisted files.'
}

function Send-Request {
    param($Child, [string]$Method, [int]$Id, [hashtable]$Parameters)
    $request = @{ jsonrpc = '2.0'; method = $Method; params = $Parameters }
    if ($Id -gt 0) { $request.id = $Id }
    $Child.StandardInput.WriteLine(($request | ConvertTo-Json -Depth 12 -Compress))
    $Child.StandardInput.Flush()
    if ($Id -le 0) { return }
    $read = $Child.StandardOutput.ReadLineAsync()
    if (-not $read.Wait(6000) -or -not $read.Result) { throw "No response to $Method" }
    $reply = $read.Result | ConvertFrom-Json
    if ($reply.id -ne $Id -or $reply.jsonrpc -ne '2.0' -or $reply.PSObject.Properties['error']) { throw "Unexpected response to $Method" }
    return $reply.result
}

$executable = Join-Path $root $manifest.binary
$info = [Diagnostics.ProcessStartInfo]::new($executable)
$info.WorkingDirectory = $root
$info.Arguments = '--protocol-fixture'
$info.UseShellExecute = $false
$info.CreateNoWindow = $true
$info.RedirectStandardInput = $true
$info.RedirectStandardOutput = $true
$info.RedirectStandardError = $true
$info.EnvironmentVariables['PATH'] = (Join-Path $env:SystemRoot 'System32') + ';' + $env:SystemRoot
$child = [Diagnostics.Process]::Start($info)
try {
    $initial = Send-Request $child 'initialize' 1 @{
        protocolVersion = '2025-11-25'; capabilities = @{}; clientInfo = @{ name = 'qkm-package-smoke'; version = '1' }
    }
    if ($initial.protocolVersion -ne '2025-11-25') { throw 'Legacy negotiation differed.' }
    Send-Request $child 'notifications/initialized' 0 @{}
    $catalog = Send-Request $child 'tools/list' 2 @{}
    if (@($catalog.tools).Count -ne 1 -or $catalog.tools[0].name -ne 'qkm_protocol_fixture') {
        throw 'Fixture tool was not discovered.'
    }
    $result = Send-Request $child 'tools/call' 3 @{ name = 'qkm_protocol_fixture'; arguments = @{ mode = 'image' } }
    $image = @($result.content | Where-Object type -eq 'image')
    if ($result.isError -or $result.structuredContent.width -ne 2 -or $image.Count -ne 1 -or
        $image[0].mimeType -ne 'image/png' -or [Convert]::FromBase64String($image[0].data).Length -ne 75) {
        throw 'Structured PNG smoke failed.'
    }
    $modules = @($child.Modules | Where-Object ModuleName -Match '^(vcruntime[0-9]|msvcp[0-9]|python[0-9]|rust_std)')
    $runtime = $null
    if ($needsCrt) {
        $runtime = Join-Path $root 'VCRUNTIME140.dll'
        if ($modules.Count -ne 1 -or $modules[0].FileName -ine $runtime) {
            throw 'App-local Visual C++ runtime was not observed.'
        }
    } elseif ($modules.Count -ne 0) {
        throw 'The standalone executable unexpectedly loaded a redistributable runtime.'
    }
    $child.StandardInput.Close()
    if (-not $child.WaitForExit(7000) -or $child.ExitCode -ne 0 -or
        $child.StandardOutput.ReadToEnd() -or $child.StandardError.ReadToEnd()) {
        throw 'Unexpected stdio output or exit after EOF.'
    }
    [pscustomobject]@{ bundle_sha256 = $Sha256; protocol = $initial.protocolVersion; image_bytes = 75
        runtime = $runtime; native_exit = $child.ExitCode; extracted = $root }
} finally {
    if (-not $child.HasExited) {
        $child.StandardInput.Close()
        if (-not $child.WaitForExit(2000)) {
            $child.Kill()
            $child.WaitForExit()
        }
    }
    $child.Dispose()
}
