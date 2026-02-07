# Create shortcut to RogueCityVisualizerGui.exe in repo root
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$targetExe = Join-Path $repoRoot 'bin\RogueCityVisualizerGui.exe'
$shortcutPath = Join-Path $repoRoot 'RogueCityVisualizerGui.lnk'

if (-not (Test-Path $targetExe)) {
    Write-Host "ERROR: Target executable not found: $targetExe" -ForegroundColor Red
    Write-Host "Please build the project first or place the exe at the expected location." -ForegroundColor Yellow
    Write-Host "Press any key to exit..."
    $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
    exit 1
}

try {
    $wsh = New-Object -ComObject WScript.Shell
    $sc = $wsh.CreateShortcut($shortcutPath)
    $sc.TargetPath = $targetExe
    $sc.WorkingDirectory = Split-Path $targetExe -Parent
    $sc.IconLocation = $targetExe
    $sc.Save()
    Write-Host "Created shortcut at: $shortcutPath" -ForegroundColor Green
} catch {
    Write-Host "Failed to create shortcut: $_" -ForegroundColor Red
    Write-Host "Press any key to exit..."
    $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
    exit 1
}

Write-Host "Press any key to exit..."
$null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
