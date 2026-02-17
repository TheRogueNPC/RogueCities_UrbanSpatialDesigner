$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$env:RC_ROOT = $repoRoot
$env:RC_BUILD_DIR = Join-Path $repoRoot "build_vs"
$env:CMAKE_GENERATOR = "Visual Studio 18 2026"
if (-not $env:CMAKE_BUILD_PARALLEL_LEVEL) {
    $env:CMAKE_BUILD_PARALLEL_LEVEL = "8"
}
$env:CTEST_OUTPUT_ON_FAILURE = "1"
$env:PATH = "$repoRoot\tools;$env:PATH"

function rc-help {
    Write-Host "RogueCities dev commands:" -ForegroundColor Cyan
    Write-Host "  rc-cfg [-Clean] [-Preset dev]" -ForegroundColor Yellow
    Write-Host "  rc-bld [-Preset gui-release]" -ForegroundColor Yellow
    Write-Host "  rc-run" -ForegroundColor Yellow
    Write-Host "  rc-tst [-Preset ctest-release]" -ForegroundColor Yellow
    Write-Host "  rc-doctor" -ForegroundColor Yellow
    Write-Host "  rc-problems [-MaxItems 10]" -ForegroundColor Yellow
    Write-Host "  rc-pdiff" -ForegroundColor Yellow
    Write-Host "  rc-refresh [-ConfigurePreset dev] [-BuildPreset gui-release]" -ForegroundColor Yellow
    Write-Host "  rc-env" -ForegroundColor Yellow
}

function rc-env {
    Write-Host "RC_ROOT=$env:RC_ROOT"
    Write-Host "RC_BUILD_DIR=$env:RC_BUILD_DIR"
    Write-Host "CMAKE_GENERATOR=$env:CMAKE_GENERATOR"
    Write-Host "CMAKE_BUILD_PARALLEL_LEVEL=$env:CMAKE_BUILD_PARALLEL_LEVEL"
}

function rc-cfg {
    param(
        [switch]$Clean,
        [string]$Preset = "dev"
    )

    if ($Clean -and (Test-Path -LiteralPath $env:RC_BUILD_DIR)) {
        Remove-Item -LiteralPath $env:RC_BUILD_DIR -Recurse -Force
    }

    cmake --preset $Preset
}

function rc-bld {
    param(
        [string]$Preset = "gui-release"
    )

    cmake --build --preset $Preset
}

function rc-run {
    $exe = Join-Path $env:RC_ROOT "bin\RogueCityVisualizerGui.exe"
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "Executable not found: $exe"
    }

    & $exe
}

function rc-tst {
    param(
        [string]$Preset = "ctest-release"
    )

    ctest --preset $Preset
}

function Invoke-RCPythonTool {
    param(
        [Parameter(Mandatory = $true)][string]$ScriptPath,
        [string[]]$Args = @()
    )

    $script = Join-Path $env:RC_ROOT $ScriptPath
    if (-not (Test-Path -LiteralPath $script)) {
        throw "Tool script not found: $script"
    }

    $argString = ($Args -join " ")
    $cmd = "py -3 `"$script`" $argString"
    cmd /c $cmd
    if ($LASTEXITCODE -ne 0) {
        $cmd = "python `"$script`" $argString"
        cmd /c $cmd
    }
}

function rc-doctor {
    Invoke-RCPythonTool -ScriptPath "tools\env_doctor.py"
}

function rc-problems {
    param([int]$MaxItems = 10)
    Invoke-RCPythonTool -ScriptPath "tools\problems_triage.py" -Args @("--input", ".vscode/problems.export.json", "--max-items", "$MaxItems")
}

function rc-pdiff {
    Invoke-RCPythonTool -ScriptPath "tools\problems_diff.py" -Args @("--current", ".vscode/problems.export.json", "--snapshot-current")
}

function rc-refresh {
    param(
        [string]$ConfigurePreset = "dev",
        [string]$BuildPreset = "gui-release"
    )
    Invoke-RCPythonTool -ScriptPath "tools\dev_refresh.py" -Args @("--configure-preset", $ConfigurePreset, "--build-preset", $BuildPreset)
}

Write-Host "RogueCities dev shell loaded. Run rc-help for commands." -ForegroundColor Green
