param(
    [switch]$Gui,
    [switch]$NoRun,
    [switch]$VerboseChecks,
    [switch]$ForceStartupBuild
)

$ErrorActionPreference = "Stop"

function Test-CommandExists {
    param([Parameter(Mandatory = $true)][string]$Name)
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Write-Section {
    param([string]$Text)
    Write-Host "`n== $Text ==" -ForegroundColor Cyan
}

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptPath

$startupBuild = Join-Path $repoRoot "StartupBuild.bat"
$startupCli = Join-Path $repoRoot "build_and_run.bat"
$startupGui = Join-Path $repoRoot "build_and_run_gui.ps1"

$checks = [ordered]@{
    is_windows       = $IsWindows
    has_startupbuild = Test-Path $startupBuild
    has_startup_cli  = Test-Path $startupCli
    has_startup_gui  = Test-Path $startupGui
    has_cmake        = Test-CommandExists "cmake"
    has_msbuild      = Test-CommandExists "msbuild"
    has_ninja        = Test-CommandExists "ninja"
    has_pwsh         = Test-CommandExists "pwsh"
}

$cachePath = Join-Path $repoRoot "build/CMakeCache.txt"
$cacheExists = Test-Path $cachePath
$cacheRootMismatch = $false

if ($cacheExists) {
    try {
        $cacheLine = Select-String -Path $cachePath -Pattern "^CMAKE_HOME_DIRECTORY:INTERNAL=" -SimpleMatch:$false | Select-Object -First 1
        if ($null -ne $cacheLine) {
            $configuredRoot = ($cacheLine.Line -replace "^CMAKE_HOME_DIRECTORY:INTERNAL=", "").Trim()
            $normalizedRepo = $repoRoot.TrimEnd('\\')
            $normalizedConfigured = $configuredRoot.TrimEnd('\\')
            if ($normalizedConfigured -and ($normalizedConfigured -ne $normalizedRepo)) {
                $cacheRootMismatch = $true
            }
        }
    } catch {
        $cacheRootMismatch = $true
    }
}

$checks.cache_exists = $cacheExists
$checks.cache_root_mismatch = $cacheRootMismatch

$criticalIssues = @()

if (-not $checks.is_windows) {
    $criticalIssues += "Non-Windows host detected; startup BAT workflow is Windows-first."
}
if (-not $checks.has_startupbuild) {
    $criticalIssues += "Missing StartupBuild.bat in repo root."
}
if ((-not $checks.has_cmake) -and (-not $checks.has_msbuild)) {
    $criticalIssues += "Neither cmake nor msbuild found in PATH."
}

$unstableEnvironment = $ForceStartupBuild -or $cacheRootMismatch -or ($criticalIssues.Count -gt 0)

Write-Section "Preflight Summary"
Write-Host "Repo Root: $repoRoot"
Write-Host "Environment unstable: $unstableEnvironment"

if ($VerboseChecks) {
    $checks.GetEnumerator() | ForEach-Object {
        Write-Host (" - {0}: {1}" -f $_.Key, $_.Value)
    }
}

if ($criticalIssues.Count -gt 0) {
    Write-Host "`nCritical issues detected:" -ForegroundColor Yellow
    $criticalIssues | ForEach-Object { Write-Host " - $_" -ForegroundColor Yellow }
}

$selectedPath = $null
$selectedKind = $null

if ($unstableEnvironment) {
    $selectedPath = $startupBuild
    $selectedKind = "bat"
} elseif ($Gui) {
    if ($checks.has_startup_gui) {
        $selectedPath = $startupGui
        $selectedKind = "ps1"
    } elseif ($checks.has_startup_cli) {
        $selectedPath = $startupCli
        $selectedKind = "bat"
    } else {
        $selectedPath = $startupBuild
        $selectedKind = "bat"
    }
} else {
    if ($checks.has_startup_cli) {
        $selectedPath = $startupCli
        $selectedKind = "bat"
    } else {
        $selectedPath = $startupBuild
        $selectedKind = "bat"
    }
}

Write-Section "Selected Startup Path"
Write-Host "Script: $selectedPath"

if ($NoRun) {
    Write-Host "NoRun enabled: preflight finished without execution." -ForegroundColor Green
    exit 0
}

if (-not (Test-Path $selectedPath)) {
    Write-Host "Selected startup script does not exist: $selectedPath" -ForegroundColor Red
    exit 2
}

Push-Location $repoRoot
try {
    if ($selectedKind -eq "bat") {
        & cmd /c "`"$selectedPath`""
        $exitCode = $LASTEXITCODE
    } else {
        & pwsh -NoProfile -ExecutionPolicy Bypass -File $selectedPath
        $exitCode = $LASTEXITCODE
    }
} finally {
    Pop-Location
}

if ($exitCode -ne 0) {
    Write-Host "Startup script failed with exit code $exitCode." -ForegroundColor Red
    exit $exitCode
}

Write-Host "Startup completed successfully." -ForegroundColor Green
exit 0
