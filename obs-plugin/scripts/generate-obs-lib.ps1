# Generate OBS import library from obs.dll
# Usage: .\generate-obs-lib.ps1 -ObsDllDir <path-to-dir-with-obs.dll>
param(
    [Parameter(Mandatory=$true)]
    [string]$ObsDllDir
)

$ErrorActionPreference = "Stop"

$obsDll = Join-Path $ObsDllDir "obs.dll"
if (-not (Test-Path $obsDll)) {
    Write-Error "obs.dll not found at $obsDll"
    exit 1
}

Push-Location $ObsDllDir

# Dump exports
Write-Host "Dumping exports from obs.dll..."
& dumpbin /exports obs.dll | Out-File -FilePath obs_exports.txt -Encoding ASCII

# Parse exports into .def file
$lines = Get-Content obs_exports.txt
$exports = [System.Collections.ArrayList]@()
[void]$exports.Add("LIBRARY obs")
[void]$exports.Add("EXPORTS")

$inSection = $false
foreach ($line in $lines) {
    if ($line -match '^\s+ordinal\s+hint') {
        $inSection = $true
        continue
    }
    if ($inSection -and $line -match '^\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\S+)') {
        [void]$exports.Add("  " + $Matches[1])
    }
    if ($inSection -and $line.Trim() -eq '') {
        $inSection = $false
    }
}

$exports | Out-File -FilePath obs.def -Encoding ASCII
Write-Host "Generated obs.def with $($exports.Count - 2) exports"

# Generate import library
Write-Host "Generating obs.lib..."
& lib /def:obs.def /out:obs.lib /machine:x64

if (Test-Path obs.lib) {
    Write-Host "Successfully generated obs.lib"
    Get-Item obs.lib | Format-Table Name, Length
} else {
    Write-Error "Failed to generate obs.lib"
    exit 1
}

Pop-Location
