[CmdletBinding()]
param(
    [string]$QMakePath,
    [string]$MakePath,
    [string]$BuildRoot = "build/release-dual",
    [ValidateSet("All", "QKeyMapper", "QKeyMapper_diagnostic")]
    [string]$Target = "All"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-BuildTool {
    param(
        [string]$RequestedPath,
        [string[]]$FallbackNames,
        [string]$DisplayName
    )

    $candidates = if ([string]::IsNullOrWhiteSpace($RequestedPath)) {
        $FallbackNames
    }
    else {
        @($RequestedPath)
    }

    foreach ($candidate in $candidates) {
        $command = Get-Command $candidate -CommandType Application -ErrorAction SilentlyContinue
        if ($null -ne $command) {
            return $command.Source
        }
    }

    throw "$DisplayName was not found. Pass its full path explicitly."
}

function Assert-ChildBuildPath {
    param(
        [string]$Path,
        [string]$RootPrefix
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if (-not $fullPath.StartsWith($RootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a path outside BuildRoot: $fullPath"
    }

    return $fullPath
}

function Invoke-ReleaseBuild {
    param(
        [string]$BuildDirectory,
        [bool]$Diagnostic
    )

    New-Item -ItemType Directory -Path $BuildDirectory -Force | Out-Null
    Push-Location $BuildDirectory
    try {
        $qmakeArguments = @(
            "-o",
            "Makefile",
            $script:ProjectFile,
            "-spec",
            "win32-msvc",
            "CONFIG+=release"
        )
        if ($Diagnostic) {
            $qmakeArguments += "DEFINES+=LOGOUT_TOFILE"
        }

        & $script:ResolvedQMake @qmakeArguments
        if ($LASTEXITCODE -ne 0) {
            throw "qmake failed for $BuildDirectory with exit code $LASTEXITCODE."
        }

        & $script:ResolvedMake release
        if ($LASTEXITCODE -ne 0) {
            throw "Release build failed for $BuildDirectory with exit code $LASTEXITCODE."
        }
    }
    finally {
        Pop-Location
    }
}

$ProjectFile = Join-Path $PSScriptRoot "QKeyMapper.pro"
if (-not (Test-Path -LiteralPath $ProjectFile -PathType Leaf)) {
    throw "Project file was not found: $ProjectFile"
}
$ProjectFile = [System.IO.Path]::GetFullPath($ProjectFile)

$ResolvedQMake = Resolve-BuildTool -RequestedPath $QMakePath -FallbackNames @("qmake.exe") -DisplayName "qmake.exe"
$ResolvedMake = Resolve-BuildTool -RequestedPath $MakePath -FallbackNames @("jom.exe", "nmake.exe") -DisplayName "jom.exe or nmake.exe"
$null = Resolve-BuildTool -FallbackNames @("cl.exe") -DisplayName "cl.exe"

if (-not [System.IO.Path]::IsPathRooted($BuildRoot)) {
    $BuildRoot = Join-Path $PSScriptRoot $BuildRoot
}
$BuildRoot = [System.IO.Path]::GetFullPath($BuildRoot)
if ($BuildRoot -eq [System.IO.Path]::GetPathRoot($BuildRoot)) {
    throw "BuildRoot cannot be a filesystem root."
}

$rootPrefix = $BuildRoot
if (-not $rootPrefix.EndsWith([System.IO.Path]::DirectorySeparatorChar.ToString())) {
    $rootPrefix += [System.IO.Path]::DirectorySeparatorChar
}
$projectRoot = [System.IO.Path]::GetFullPath($PSScriptRoot)
if ($projectRoot.Equals($BuildRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
    $projectRoot.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "BuildRoot cannot be the project directory or one of its ancestors."
}

$standardBuildDirectory = Assert-ChildBuildPath -Path (Join-Path $BuildRoot "QKeyMapper") -RootPrefix $rootPrefix
$diagnosticBuildDirectory = Assert-ChildBuildPath -Path (Join-Path $BuildRoot "QKeyMapper_diagnostic") -RootPrefix $rootPrefix

$selectedBuilds = @()
if ($Target -eq "All" -or $Target -eq "QKeyMapper") {
    $selectedBuilds += [PSCustomObject]@{
        Name = "QKeyMapper"
        Directory = $standardBuildDirectory
        Diagnostic = $false
    }
}
if ($Target -eq "All" -or $Target -eq "QKeyMapper_diagnostic") {
    $selectedBuilds += [PSCustomObject]@{
        Name = "QKeyMapper_diagnostic"
        Directory = $diagnosticBuildDirectory
        Diagnostic = $true
    }
}

foreach ($build in $selectedBuilds) {
    if (Test-Path -LiteralPath $build.Directory) {
        Remove-Item -LiteralPath $build.Directory -Recurse -Force
    }
}

foreach ($build in $selectedBuilds) {
    Invoke-ReleaseBuild -BuildDirectory $build.Directory -Diagnostic $build.Diagnostic
}

""
foreach ($build in $selectedBuilds) {
    $executable = Join-Path $build.Directory "release/QKeyMapper.exe"
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "Expected release executable was not generated: $executable"
    }
    Write-Output "$($build.Name)=$([System.IO.Path]::GetFullPath($executable))"
}
