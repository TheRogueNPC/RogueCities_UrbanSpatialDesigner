# Manually load VS Code shell integration
if ($env:TERM_PROGRAM -eq "vscode") {
    if (Get-Command "code" -ErrorAction SilentlyContinue) {
        . "$(code --locate-shell-integration-path pwsh)"
    }
}

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

# Ensure CMake tools are available in shells that do not inherit system PATH
$programFilesX86 = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
$cmakeBinCandidates = @(
    (Join-Path $env:ProgramFiles "CMake\bin"),
    ($(if ($programFilesX86) { Join-Path $programFilesX86 "CMake\bin" } else { $null }))
) | Where-Object { $_ }

$cmakeResolved = $false
foreach ($cmakeBin in $cmakeBinCandidates) {
    $cmakeExe = Join-Path $cmakeBin "cmake.exe"
    if (Test-Path $cmakeExe) {
        if (-not ($env:PATH -split ';' | Where-Object { $_ -ieq $cmakeBin })) {
            $env:PATH = "$cmakeBin;$env:PATH"
        }
        if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
            Set-Alias -Name cmake -Value $cmakeExe -Scope Global
        }
        $ctestExe = Join-Path $cmakeBin "ctest.exe"
        if ((Test-Path $ctestExe) -and -not (Get-Command ctest -ErrorAction SilentlyContinue)) {
            Set-Alias -Name ctest -Value $ctestExe -Scope Global
        }
        $cmakeResolved = $true
        break
    }
}
if (-not $cmakeResolved -and -not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Warning "cmake.exe not found in standard locations; rc-cfg/rc-bld may fail."
}

# Force MSVC to use absolute paths in diagnostics (improves VS Code click-to-open)
$env:FC = "Absolute" 
$env:CC = "Absolute"

# AI Enhancement: Auto-activate Python venv if it exists
$venvPath = Join-Path $env:RC_ROOT ".venv\Scripts\Activate.ps1"
if (Test-Path $venvPath) {
    . $venvPath
    Write-Host "Python virtual environment activated." -ForegroundColor Gray
}

function rc-help {
    <#
    .SYNOPSIS
        Displays the list of available RogueCities development commands.
    #>
    Write-Host "RogueCities dev commands:" -ForegroundColor Cyan
    Write-Host "  rc-cfg     [-Clean] [-Preset dev]" -ForegroundColor Yellow
    Write-Host "  rc-bld     [-Preset gui-release]" -ForegroundColor Yellow
    Write-Host "  rc-run" -ForegroundColor Yellow
    Write-Host "  rc-tst     [-Preset ctest-release]" -ForegroundColor Yellow
    Write-Host "  rc-fmt" -ForegroundColor Yellow
    Write-Host "  rc-deps    [-ExeName RogueCityVisualizerGui.exe]" -ForegroundColor Yellow
    Write-Host "  rc-rdoc" -ForegroundColor Yellow
    Write-Host "  rc-doctor" -ForegroundColor Yellow
    Write-Host "  rc-contract" -ForegroundColor Yellow
    Write-Host "  rc-problems [-MaxItems 10]" -ForegroundColor Yellow
    Write-Host "  rc-pdiff" -ForegroundColor Yellow
    Write-Host "  rc-refresh [-ConfigurePreset dev] [-BuildPreset gui-release]" -ForegroundColor Yellow
    Write-Host "  rc-env" -ForegroundColor Yellow
    Write-Host "  rc-context" -ForegroundColor Yellow
}

function rc-env {
    <#
    .SYNOPSIS
        Prints current environment variables relevant to the build system.
    #>
    Write-Host "RC_ROOT=$env:RC_ROOT"
    Write-Host "RC_BUILD_DIR=$env:RC_BUILD_DIR"
    Write-Host "CMAKE_GENERATOR=$env:CMAKE_GENERATOR"
    Write-Host "CMAKE_BUILD_PARALLEL_LEVEL=$env:CMAKE_BUILD_PARALLEL_LEVEL"
}

function rc-context {
    <#
    .SYNOPSIS
        Outputs the current shell environment configuration as JSON for AI analysis.
    #>
    $context = @{
        Root            = $env:RC_ROOT
        BuildDir        = $env:RC_BUILD_DIR
        Generator       = $env:CMAKE_GENERATOR
        ParallelLevel   = $env:CMAKE_BUILD_PARALLEL_LEVEL
        AvailableTools  = (Get-Command rc-* -CommandType Function).Name
        Python          = (Get-Command py -ErrorAction SilentlyContinue).Source
    }
    return ($context | ConvertTo-Json -Depth 2)
}

function rc-cfg {
    <#
    .SYNOPSIS
        Configures the project using CMake.
    .PARAMETER Clean
        Removes the build directory before configuring.
    .PARAMETER Preset
        The CMake preset to use (default: "dev").
    #>
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
    <#
    .SYNOPSIS
        Builds the project using CMake.
    .PARAMETER Preset
        The CMake build preset to use (default: "gui-release").
    #>
    param(
        [string]$Preset = "gui-release"
    )

    cmake --build --preset $Preset
}

function rc-run {
    <#
    .SYNOPSIS
        Runs the main RogueCityVisualizerGui executable.
    #>
    $exe = Join-Path $env:RC_ROOT "bin\RogueCityVisualizerGui.exe"
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "Executable not found: $exe"
    }

    & $exe
}

function rc-tst {
    <#
    .SYNOPSIS
        Runs project tests using CTest.
    .PARAMETER Preset
        The CTest preset to use (default: "ctest-release").
    #>
    param(
        [string]$Preset = "ctest-release"
    )

    ctest --preset $Preset
}

function rc-fmt {
    <#
    .SYNOPSIS
        Runs clang-format only on modified or staged files (Git-aware).
    #>
    $files = git diff --name-only --cached -- '*.cpp' '*.h' '*.hpp'
    $files += git diff --name-only -- '*.cpp' '*.h' '*.hpp'
    
    if ($files) {
        $uniqueFiles = $files | Select-Object -Unique
        foreach ($file in $uniqueFiles) {
            if (Test-Path $file) {
                Write-Host "Formatting: $file" -ForegroundColor Cyan
                clang-format -i -style=file $file
            }
        }
    } else {
        Write-Host "No modified C++ files found to format." -ForegroundColor Gray
    }
}

function rc-deps {
    <#
    .SYNOPSIS
        Checks which DLLs your executable is trying to load.
    .PARAMETER ExeName
        Name of the executable to inspect (default: RogueCityVisualizerGui.exe).
    #>
    param([string]$ExeName = "RogueCityVisualizerGui.exe")
    
    $exePath = Join-Path $env:RC_ROOT "bin\$ExeName"
    if (-not (Test-Path $exePath)) { Write-Error "Exe not found: $exePath"; return }

    $dumpbin = Get-Command "dumpbin" -ErrorAction SilentlyContinue
    
    if ($dumpbin) {
        dumpbin /DEPENDENTS $exePath
    } else {
        Write-Warning "dumpbin.exe not found. Ensure you have VS C++ Tools in your path."
    }
}

function rc-rdoc {
    <#
    .SYNOPSIS
        Launches the application via RenderDoc for frame capturing.
    #>
    $renderDocPath = "C:\Program Files\RenderDoc\qrenderdoc.exe"
    $exe = Join-Path $env:RC_ROOT "bin\RogueCityVisualizerGui.exe"
    
    if (Test-Path $renderDocPath) {
        if (-not (Test-Path $exe)) { throw "Executable not found: $exe" }
        Start-Process $renderDocPath -ArgumentList "--capture", $exe
    } else {
        Write-Warning "RenderDoc not found at default location ($renderDocPath)."
    }
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
    <#
    .SYNOPSIS
        Runs the environment doctor script to check for issues.
    #>
    Invoke-RCPythonTool -ScriptPath "tools\env_doctor.py"
}

function rc-contract {
    <#
    .SYNOPSIS
        Checks the Clang builder contract.
    #>
    Invoke-RCPythonTool -ScriptPath "tools\check_clang_builder_contract.py"
}

function rc-problems {
    <#
    .SYNOPSIS
        Triages build problems from the VS Code export.
    .PARAMETER MaxItems
        Maximum number of problems to display (default: 10).
    #>
    param([int]$MaxItems = 10)
    Invoke-RCPythonTool -ScriptPath "tools\problems_triage.py" -Args @("--input", ".vscode/problems.export.json", "--max-items", "$MaxItems")
}

function rc-pdiff {
    <#
    .SYNOPSIS
        Compares current build problems against a snapshot.
    #>
    Invoke-RCPythonTool -ScriptPath "tools\problems_diff.py" -Args @("--current", ".vscode/problems.export.json", "--snapshot-current")
}

function rc-refresh {
    <#
    .SYNOPSIS
        Refreshes the development environment by re-configuring and re-building.
    .PARAMETER ConfigurePreset
        The CMake configure preset (default: "dev").
    .PARAMETER BuildPreset
        The CMake build preset (default: "gui-release").
    #>
    param(
        [string]$ConfigurePreset = "dev",
        [string]$BuildPreset = "gui-release"
    )
    Invoke-RCPythonTool -ScriptPath "tools\dev_refresh.py" -Args @("--configure-preset", $ConfigurePreset, "--build-preset", $BuildPreset)
}

Write-Host "RogueCities dev shell loaded. Run rc-help for commands." -ForegroundColor Green
