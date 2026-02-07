<#
.SYNOPSIS
    Move all *.o and *.obj files from the repository into a dedicated folder under build/objects
.DESCRIPTION
    Searches the repository (excluding .git and build output) for object files (*.o, *.obj) and moves
    them into a mirrored directory structure under build\objects to keep the workspace clean.
.PARAMETER Force
    Overwrite existing files in the destination if present.
.EXAMPLE
    .\tools\move_object_files.ps1 -Force
#>

param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$searchRoot = $repoRoot
$destRoot = Join-Path $repoRoot 'build\objects'

Write-Host "Repo root: $repoRoot"
Write-Host "Destination root: $destRoot"

# Exclude common directories
$excludeDirs = @('.git', 'build', 'bin', 'out')

# Gather object files
$objectFiles = Get-ChildItem -Path $searchRoot -Include *.o, *.obj -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
    $exclude = $false
    foreach ($ex in $excludeDirs) {
        if ($_.FullName -like "*\\$ex\\*") { $exclude = $true; break }
    }
    -not $exclude
}

if (-not $objectFiles -or $objectFiles.Count -eq 0) {
    Write-Host "No .o or .obj files found (outside excluded directories)." -ForegroundColor Yellow
    Write-Host "Press any key to exit..."
    $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
    exit 0
}

Write-Host "Found $($objectFiles.Count) object file(s)."

foreach ($file in $objectFiles) {
    # Compute relative path from repo root
    $relPath = $file.FullName.Substring($repoRoot.Length).TrimStart('\','/')
    $relDir = Split-Path $relPath -Parent
    if ([string]::IsNullOrEmpty($relDir)) { $relDir = '.' }
    $targetDir = Join-Path $destRoot $relDir
    if (-not (Test-Path $targetDir)) { New-Item -ItemType Directory -Path $targetDir -Force | Out-Null }

    $destPath = Join-Path $targetDir $file.Name
    if (Test-Path $destPath) {
        if ($Force) {
            Remove-Item -Path $destPath -Force -ErrorAction SilentlyContinue
        } else {
            Write-Host "Skipping existing file: $destPath (use -Force to overwrite)" -ForegroundColor Yellow
            continue
        }
    }

    try {
        Move-Item -Path $file.FullName -Destination $destPath -Force:$Force
        Write-Host "Moved: $($file.FullName) -> $destPath"
    } catch {
        Write-Host "Failed to move $($file.FullName): $_" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "All done. Press any key to exit..."
$null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
