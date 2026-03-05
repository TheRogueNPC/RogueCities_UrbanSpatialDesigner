# Build and Run RogueCity Visualizer GUI
# Defaults to MSBuild-backed CMake presets from RogueCities devshell.

param(
    [switch]$Clean = $false,
    [switch]$RunOnly = $false,
    [ValidateSet("Debug", "Release")][string]$BuildType = "Release",
    [ValidateSet("msbuild", "msbuild-vs2026", "ninja-msvc", "ninja-clang")][string]$Toolchain = "msbuild"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$exePath = Join-Path $repoRoot "bin\RogueCityVisualizerGui.exe"

$configurePreset = switch ($Toolchain) {
    "msbuild" { "dev" }
    "msbuild-vs2026" { "dev-vs2026" }
    "ninja-msvc" { "dev-ninja-msvc" }
    "ninja-clang" { "dev-ninja-clang" }
    default { "dev" }
}

$buildPreset = switch ("$Toolchain-$BuildType") {
    "msbuild-Debug" { "gui-debug" }
    "msbuild-Release" { "gui-release" }
    "msbuild-vs2026-Debug" { "gui-debug-vs2026" }
    "msbuild-vs2026-Release" { "gui-release-vs2026" }
    "ninja-msvc-Debug" { "gui-debug-ninja-msvc" }
    "ninja-msvc-Release" { "gui-release-ninja-msvc" }
    "ninja-clang-Debug" { "gui-debug-ninja-clang" }
    "ninja-clang-Release" { "gui-release-ninja-clang" }
    default { "gui-release" }
}

$buildDir = switch ($Toolchain) {
    "msbuild" { "build_vs" }
    "msbuild-vs2026" { "build_vs" }
    "ninja-msvc" { "build_ninja_msvc" }
    "ninja-clang" { "build_ninja_clang" }
    default { "build_vs" }
}

Write-Host "RogueCity Visualizer GUI Build Script" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host "Toolchain: $Toolchain | BuildType: $BuildType" -ForegroundColor DarkCyan
Write-Host "Configure Preset: $configurePreset | Build Preset: $buildPreset" -ForegroundColor DarkCyan

if ($Clean) {
    Write-Host "Cleaning build directory '$buildDir'..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force (Join-Path $repoRoot $buildDir) -ErrorAction SilentlyContinue
}

if (-not $RunOnly) {
    Push-Location $repoRoot
    try {
        Write-Host "Configuring with CMake preset '$configurePreset'..." -ForegroundColor Green
        cmake --preset $configurePreset
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Configuration failed." -ForegroundColor Red
            exit 1
        }

        Write-Host "Building with preset '$buildPreset'..." -ForegroundColor Green
        cmake --build --preset $buildPreset
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Build failed." -ForegroundColor Red
            exit 1
        }
    } finally {
        Pop-Location
    }

    Write-Host "Build successful." -ForegroundColor Green
}

if (Test-Path -LiteralPath $exePath) {
    Write-Host "Launching GUI..." -ForegroundColor Cyan
    & $exePath
} else {
    Write-Host "Error: executable not found at $exePath" -ForegroundColor Red
    Write-Host "Run without -RunOnly to configure/build first." -ForegroundColor Yellow
    exit 1
}
