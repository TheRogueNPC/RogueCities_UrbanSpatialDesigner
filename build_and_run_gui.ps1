# Build and Run RogueCity Visualizer GUI
# This script configures, builds, and launches the GUI visualizer

param(
    [switch]$Clean = $false,
    [switch]$RunOnly = $false,
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

$BuildDir = "build_gui"
$ExePath = "bin\RogueCityVisualizerGui.exe"

Write-Host "RogueCity Visualizer GUI Build Script" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan

if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue
}

if (-not $RunOnly) {
    Write-Host "Configuring with CMake..." -ForegroundColor Green
    cmake -B $BuildDir -S . -G Ninja -DROGUECITY_BUILD_VISUALIZER=ON "-DCMAKE_BUILD_TYPE=$BuildType"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Configuration failed!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Building RogueCityVisualizerGui..." -ForegroundColor Green
    cmake --build $BuildDir --target RogueCityVisualizerGui -j 8
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Build successful!" -ForegroundColor Green
}

if (Test-Path $ExePath) {
    Write-Host "Launching GUI..." -ForegroundColor Cyan
    & $ExePath
} else {
    Write-Host "Error: Executable not found at $ExePath" -ForegroundColor Red
    Write-Host "Try running without -RunOnly flag to build first" -ForegroundColor Yellow
    exit 1
}
