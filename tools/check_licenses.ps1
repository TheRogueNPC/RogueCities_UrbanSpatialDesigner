$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$scanFiles = New-Object System.Collections.Generic.List[string]

$thirdParty = Join-Path $root "3rdparty"
if (Test-Path $thirdParty) {
    $patterns = @("license*", "copying*", "copyright*")
    foreach ($pattern in $patterns) {
        Get-ChildItem -Path $thirdParty -Recurse -Depth 3 -File -Filter $pattern |
            Where-Object { $_.FullName -notmatch "[\\/]+3rdparty[\\/]+vcpkg([\\/]|$)" } |
            ForEach-Object { [void]$scanFiles.Add($_.FullName) }
    }
}

$vcpkgManifest = Join-Path $root "vcpkg.json"
if (Test-Path $vcpkgManifest) {
    [void]$scanFiles.Add($vcpkgManifest)
}

$vcpkgConfig = Join-Path $root "vcpkg-configuration.json"
if (Test-Path $vcpkgConfig) {
    [void]$scanFiles.Add($vcpkgConfig)
}

if ($scanFiles.Count -eq 0) {
    Write-Host "No dependency license metadata files were found to scan."
    exit 0
}

$regex = "(GNU\s+GENERAL\s+PUBLIC\s+LICENSE|GNU\s+AFFERO\s+GENERAL\s+PUBLIC\s+LICENSE|\bAGPL\b|\bLGPL\b|non-commercial|academic\s+use\s+only|research\s+use\s+only)"
$violations = @()

foreach ($file in $scanFiles | Sort-Object -Unique) {
    $matches = Select-String -Path $file -Pattern $regex -AllMatches -CaseSensitive:$false
    if ($matches) {
        $violations += $file
        Write-Host "ERROR: Disallowed license terms detected in $file"
        $matches | Select-Object -First 5 | ForEach-Object {
            Write-Host ("  L{0}: {1}" -f $_.LineNumber, $_.Line.Trim())
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Error "License policy check failed."
}

Write-Host "License policy check passed."
