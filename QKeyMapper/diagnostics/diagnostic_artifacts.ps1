Set-StrictMode -Version Latest

function Get-PeIdentity {
    param([Parameter(Mandatory)][string]$Path, [switch]$IncludeImports)
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 64 -or [BitConverter]::ToUInt16($bytes, 0) -ne 0x5a4d) { throw "Invalid PE: $Path" }
    $pe = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($pe -lt 0 -or $pe + 24 -gt $bytes.Length -or [BitConverter]::ToUInt32($bytes, $pe) -ne 0x4550) { throw "Invalid PE header: $Path" }
    $machine = [BitConverter]::ToUInt16($bytes, $pe + 4)
    $count = [BitConverter]::ToUInt16($bytes, $pe + 6)
    $optional = $pe + 24
    $optionalSize = [BitConverter]::ToUInt16($bytes, $pe + 20)
    $magic = [BitConverter]::ToUInt16($bytes, $optional)
    $directory = switch ($magic) { 0x10b { $optional + 96 }; 0x20b { $optional + 112 }; default { throw "Unsupported PE: $Path" } }
    $sections = @()
    for ($i = 0; $i -lt $count; $i++) {
        $offset = $optional + $optionalSize + 40 * $i
        $sections += [pscustomobject]@{
            Rva = [BitConverter]::ToUInt32($bytes, $offset + 12)
            VirtualSize = [BitConverter]::ToUInt32($bytes, $offset + 8)
            RawSize = [BitConverter]::ToUInt32($bytes, $offset + 16)
            RawOffset = [BitConverter]::ToUInt32($bytes, $offset + 20)
        }
    }
    $debugRva = [BitConverter]::ToUInt32($bytes, $directory + 48)
    $debugSize = [BitConverter]::ToUInt32($bytes, $directory + 52)
    $guid = $null
    $age = $null
    foreach ($section in $sections) {
        if ($debugRva -gt 0 -and $debugRva -ge $section.Rva -and $debugRva -lt $section.Rva + $section.RawSize) {
            $offset = $section.RawOffset + $debugRva - $section.Rva
            for ($end = $offset + $debugSize; $offset + 28 -le $end; $offset += 28) {
                if ([BitConverter]::ToUInt32($bytes, $offset + 12) -ne 2) { continue }
                $data = [BitConverter]::ToUInt32($bytes, $offset + 24)
                if ([Text.Encoding]::ASCII.GetString($bytes, $data, 4) -ne 'RSDS') { continue }
                $guidBytes = New-Object byte[] 16
                [Array]::Copy($bytes, $data + 4, $guidBytes, 0, 16)
                $guid = [Guid]::new($guidBytes).ToString()
                $age = [BitConverter]::ToUInt32($bytes, $data + 20)
            }
        }
    }
    $identity = [pscustomobject]@{ Machine = $machine; PdbGuid = $guid; PdbAge = $age }
    if ($IncludeImports) {
        $resolveRva = {
            param([uint32]$Rva, [int]$Length = 1)
            foreach ($section in $sections) {
                $relative = [long]$Rva - $section.Rva
                if ($relative -ge 0 -and $relative + $Length -le $section.RawSize -and
                    $section.RawOffset + $relative + $Length -le $bytes.Length) {
                    return [int]($section.RawOffset + $relative)
                }
            }
            throw "Invalid PE import address: $Path"
        }
        $imports = @()
        $importRva = [BitConverter]::ToUInt32($bytes, $directory + 8)
        $importSize = [BitConverter]::ToUInt32($bytes, $directory + 12)
        if ($importRva) {
            $terminated = $false
            for ($i = 0; $i + 20 -le $importSize; $i += 20) {
                $descriptor = & $resolveRva ($importRva + $i) 20
                $nameRva = [BitConverter]::ToUInt32($bytes, $descriptor + 12)
                if (-not $nameRva) { $terminated = $true; break }
                $nameOffset = & $resolveRva $nameRva
                $end = $nameOffset
                while ($end -lt $bytes.Length -and $bytes[$end]) { $end++ }
                if ($end -eq $bytes.Length) { throw "Unterminated PE import name: $Path" }
                $imports += [pscustomobject]@{ Name = [Text.Encoding]::ASCII.GetString($bytes, $nameOffset, $end - $nameOffset); NameOffset = $nameOffset }
            }
            if (-not $terminated) { throw "Unterminated PE import directory: $Path" }
        }
        $identity | Add-Member -NotePropertyName Imports -NotePropertyValue $imports
    }
    $identity
}

function Assert-DbgHelpImport {
    param([string]$Executable, [bool]$Expected)
    # Use the same MSVC linker that the qmake build requires; /dump does not relink the image.
    $imports = (& link.exe /dump /nologo /imports $Executable) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw "Import inspection failed: $Executable" }
    $hasLibrary = $imports -match '(?im)^\s*dbghelp\.dll\s*$'
    if ($hasLibrary -ne $Expected -or ($Expected -and $imports -notmatch '\bMiniDumpWriteDump\b')) {
        throw "Unexpected DbgHelp imports: $Executable"
    }
}

function Get-PdbIdentity {
    param([Parameter(Mandatory)][string]$Path)
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 56 -or -not [Text.Encoding]::ASCII.GetString($bytes, 0, 32).StartsWith('Microsoft C/C++ MSF 7.00')) { throw "Unsupported PDB: $Path" }
    $blockSize = [BitConverter]::ToInt32($bytes, 32)
    $directorySize = [BitConverter]::ToInt32($bytes, 44)
    $blockMap = [BitConverter]::ToInt32($bytes, 52)
    if ($blockSize -lt 512 -or $blockSize -gt 65536 -or ($blockSize -band ($blockSize - 1)) -ne 0 -or $directorySize -lt 12 -or $directorySize -gt $bytes.Length) { throw "Invalid PDB directory: $Path" }
    $directory = New-Object byte[] $directorySize
    $blockCount = [int][Math]::Ceiling($directorySize / [double]$blockSize)
    if ($blockCount * 4 -gt $blockSize) { throw "PDB directory block map is too large: $Path" }
    for ($i = 0; $i -lt $blockCount; $i++) {
        $block = [BitConverter]::ToInt32($bytes, $blockMap * $blockSize + 4 * $i)
        [Array]::Copy($bytes, $block * $blockSize, $directory, $i * $blockSize, [Math]::Min($blockSize, $directorySize - $i * $blockSize))
    }
    $streams = [BitConverter]::ToInt32($directory, 0)
    if ($streams -lt 2 -or $streams -gt ($directorySize - 4) / 4) { throw "Invalid PDB streams: $Path" }
    $streamZeroSize = [BitConverter]::ToInt32($directory, 4)
    if ([BitConverter]::ToInt32($directory, 8) -lt 28) { throw "Missing PDB information stream: $Path" }
    $skip = 0
    if ($streamZeroSize -gt 0) { $skip = [int][Math]::Ceiling($streamZeroSize / [double]$blockSize) }
    $informationBlock = [BitConverter]::ToInt32($directory, 4 + 4 * $streams + 4 * $skip)
    $informationOffset = $informationBlock * $blockSize
    $guidBytes = New-Object byte[] 16
    [Array]::Copy($bytes, $informationOffset + 12, $guidBytes, 0, 16)
    [pscustomobject]@{ PdbGuid = [Guid]::new($guidBytes).ToString(); PdbAge = [BitConverter]::ToUInt32($bytes, $informationOffset + 8) }
}

function Assert-SymbolPair {
    param([string]$Executable, [string]$Pdb)
    $image = Get-PeIdentity -Path $Executable
    $symbols = Get-PdbIdentity -Path $Pdb
    if (-not $image.PdbGuid -or $image.PdbGuid -ne $symbols.PdbGuid -or $image.PdbAge -ne $symbols.PdbAge) {
        throw "EXE/PDB identity mismatch: $Executable / $Pdb"
    }
    $image
}

function Assert-ReleaseOptions {
    param([string]$Makefile)
    $lines = Get-Content -LiteralPath $Makefile
    $compiler = ($lines | Where-Object { $_ -match '^CXXFLAGS\s*=' }) -join ' '
    $linker = ($lines | Where-Object { $_ -match '^LFLAGS\s*=' }) -join ' '
    foreach ($option in @('O2', 'Zi')) {
        if ($compiler -notmatch "(?:^|\s)[/-]$option(?:\s|$)") { throw "Missing Release option $option in $Makefile" }
    }
    foreach ($option in @('/OPT:REF', '/OPT:ICF', '/INCREMENTAL:NO', '/DEBUG')) {
        if ($linker -notmatch [regex]::Escape($option)) { throw "Missing Release option $option in $Makefile" }
    }
    if ($linker -match '/DEBUG:FASTLINK|/OPT:NOREF|/OPT:NOICF' -or $compiler -match '(?:^|\s)[/-]Od(?:\s|$)') {
        throw "Unexpected non-optimized or non-portable symbol options in $Makefile"
    }
}

function Save-DiagnosticSymbols {
    param([string]$BuildDirectory, [string]$ArchiveRoot, [string]$Variant, [string]$QMakePath)
    $outputDirectory = Join-Path $BuildDirectory 'release'
    $main = Assert-SymbolPair (Join-Path $outputDirectory 'QKeyMapper.exe') (Join-Path $outputDirectory 'QKeyMapper.pdb')
    $helper = Assert-SymbolPair (Join-Path $outputDirectory 'QKeyMapperCrashReporter.exe') (Join-Path $outputDirectory 'QKeyMapperCrashReporter.pdb')
    if ($main.Machine -ne $helper.Machine -or $main.Machine -notin @(0x14c, 0x8664)) { throw 'Main/helper architecture mismatch.' }
    Assert-DbgHelpImport (Join-Path $outputDirectory 'QKeyMapper.exe') $false
    Assert-DbgHelpImport (Join-Path $outputDirectory 'QKeyMapperCrashReporter.exe') $true
    Assert-ReleaseOptions (Join-Path $BuildDirectory 'Makefile.Release')
    $helperMakefile = Join-Path $BuildDirectory 'crash-reporter/Makefile'
    Assert-ReleaseOptions $helperMakefile
    $destination = Join-Path $ArchiveRoot ($main.PdbGuid + '-' + $main.PdbAge + '/' + $Variant)
    if (Test-Path -LiteralPath $destination) { throw "Symbol archive already exists: $destination" }
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    $files = @()
    foreach ($name in @('QKeyMapper.exe', 'QKeyMapper.pdb', 'QKeyMapperCrashReporter.exe', 'QKeyMapperCrashReporter.pdb')) {
        $source = Join-Path $outputDirectory $name
        Copy-Item -LiteralPath $source -Destination (Join-Path $destination $name)
        $files += [pscustomobject]@{ Name = $name; Sha256 = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash }
    }
    $modules = @(Get-ChildItem -LiteralPath $outputDirectory -Filter '*.dll' -File | ForEach-Object {
        [pscustomobject]@{ Name = $_.Name; Version = $_.VersionInfo.FileVersion; Sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
    $identity = Get-Content -LiteralPath (Join-Path $BuildDirectory 'qkm_build_identity.h') -Raw
    $makefile = Get-Content -LiteralPath (Join-Path $BuildDirectory 'Makefile.Release')
    $compilerProbe = @()
    $stashPath = Join-Path $BuildDirectory '.qmake.stash'
    if (Test-Path -LiteralPath $stashPath) { $compilerProbe = @(Get-Content -LiteralPath $stashPath | Where-Object { $_ -match 'QMAKE_MSC_(FULL_)?VER\s*=' }) }
    $manifest = [ordered]@{
        Protocol = 1; Variant = $Variant; Main = $main; Helper = $helper
        SourceIdentity = $identity; QMake = $QMakePath; QtVersion = (& $QMakePath '-query' 'QT_VERSION')
        ArchivedUtc = [DateTime]::UtcNow.ToString('o'); Files = $files; DeployedModules = $modules
        CompilerAndLinker = @($makefile | Where-Object { $_ -match '^(CC|CXX|CXXFLAGS|LINK|LFLAGS)\s*=' })
        CompilerVersion = $compilerProbe
        HelperCompilerAndLinker = @(Get-Content -LiteralPath $helperMakefile | Where-Object { $_ -match '^(CC|CXX|CXXFLAGS|LINK|LFLAGS)\s*=' })
    }
    [IO.File]::WriteAllText((Join-Path $destination 'manifest.json'), ($manifest | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
    Write-Output "Symbols=$destination"
}
