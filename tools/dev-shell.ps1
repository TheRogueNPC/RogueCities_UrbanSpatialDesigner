# Manually load VS Code shell integration
if ($env:TERM_PROGRAM -eq "vscode") {
    # Support all VS Code flavors (VS Code, Antigravity, Cursor, Claude, Copilot) dynamically without breaking existing integrations
    $cliCandidates = @("code", "antigravity", "cursor", "claude", "copilot", "chatgpt", "code-insiders")
    $integrationPath = $null

    foreach ($cli in $cliCandidates) {
        if (Get-Command $cli -ErrorAction SilentlyContinue) {
            $path = (& $cli --locate-shell-integration-path pwsh 2>$null) | Select-Object -First 1
            if ($path -and (Test-Path $path)) {
                $integrationPath = $path
                break
            }
        }
    }
    
    if ($integrationPath) {
        . "$integrationPath"
    }
}

$ErrorActionPreference = "Stop"
$script:RCVsDevImportAttempted = $false

function Test-RCCmdInterop {
    if ($env:OS -ne "Windows_NT") {
        return $false
    }
    $cmd = Get-Command cmd.exe -ErrorAction SilentlyContinue
    if (-not $cmd) {
        return $false
    }
    $probe = & $cmd.Source /d /c "echo RC_CMD_OK" 2>$null
    if ($LASTEXITCODE -ne 0) {
        return $false
    }
    $probeText = ($probe -join "`n")
    return $probeText -match "RC_CMD_OK"
}

function Add-RCPathEntries {
    param([string[]]$Candidates)

    if (-not $env:PATH) {
        $env:PATH = ""
    }
    $current = @($env:PATH -split ';' | Where-Object { $_ -and $_.Trim().Length -gt 0 })
    $currentSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $current) {
        [void]$currentSet.Add($entry)
    }

    $prepend = [System.Collections.Generic.List[string]]::new()
    foreach ($candidate in $Candidates) {
        if (-not $candidate) { continue }
        if (-not (Test-Path -LiteralPath $candidate)) { continue }
        if ($currentSet.Contains($candidate)) { continue }
        [void]$prepend.Add($candidate)
        [void]$currentSet.Add($candidate)
    }

    if ($prepend.Count -gt 0) {
        $env:PATH = (($prepend -join ';') + ';' + $env:PATH)
    }
}

function Import-RCVsDevEnvironment {
    param([Parameter(Mandatory = $true)][string]$InitScriptPath)

    if ($env:OS -ne "Windows_NT") {
        return $false
    }
    if ($env:VSCMD_VER) {
        return $true
    }
    if (-not (Test-RCCmdInterop)) {
        return $false
    }
    if (-not (Test-Path -LiteralPath $InitScriptPath)) {
        $script:RCVsDevImportAttempted = $true
        return $false
    }

    $script:RCVsDevImportAttempted = $true
    $tempBatchPath = [IO.Path]::ChangeExtension([IO.Path]::GetTempFileName(), ".cmd")
    try {
        @"
@echo off
call "$InitScriptPath" >nul
if errorlevel 1 exit /b 1
set
"@ | Set-Content -LiteralPath $tempBatchPath -Encoding ASCII

        $envDump = & cmd.exe /d /c """$tempBatchPath""" 2>$null
        if ($LASTEXITCODE -ne 0 -or -not $envDump) {
            return $false
        }
    }
    finally {
        if (Test-Path -LiteralPath $tempBatchPath) {
            Remove-Item -LiteralPath $tempBatchPath -Force -ErrorAction SilentlyContinue
        }
    }

    foreach ($line in $envDump) {
        $idx = $line.IndexOf('=')
        if ($idx -le 0) { continue }
        $name = $line.Substring(0, $idx)
        $value = $line.Substring($idx + 1)
        [Environment]::SetEnvironmentVariable($name, $value, "Process")
    }
    return $true
}

function Get-RCVisualStudioGenerator {
    if ($env:OS -ne "Windows_NT") {
        return "Ninja Multi-Config"
    }

    $programFilesX86 = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
    $vswhereCandidates = @(
        (Join-Path $programFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"),
        "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    ) | Where-Object { $_ }

    foreach ($vswhere in $vswhereCandidates) {
        if (-not (Test-Path -LiteralPath $vswhere)) { continue }
        $versionLines = & $vswhere -latest -products "*" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion 2>$null
        $version = $versionLines | Select-Object -First 1
        if (-not $version) { continue }
        $major = ($version -split '\.')[0]
        if ($major -eq "18") { return "Visual Studio 18 2026" }
        if ($major -eq "17") { return "Visual Studio 17 2022" }
    }

    return "Visual Studio 17 2022"
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$env:RC_ROOT = $repoRoot
$env:RC_BUILD_DIR = Join-Path $repoRoot "build_vs"
$vsDevInitScript = Join-Path $repoRoot ".vscode\vsdev-init.cmd"
$vsDevLoaded = Import-RCVsDevEnvironment -InitScriptPath $vsDevInitScript
if (-not $env:CMAKE_GENERATOR) {
    $env:CMAKE_GENERATOR = Get-RCVisualStudioGenerator
}
if (-not $env:CMAKE_BUILD_PARALLEL_LEVEL) {
    $env:CMAKE_BUILD_PARALLEL_LEVEL = "8"
}
$env:CTEST_OUTPUT_ON_FAILURE = "1"
$env:RC_DEVSHELL_MSBUILD_READY = "0"
$env:RC_DEFAULT_CONFIGURE_PRESET = "dev"
$env:RC_DEFAULT_BUILD_PRESET = "gui-release"
$env:RC_DEFAULT_TEST_PRESET = "ctest-release"

$pathCandidates = @(
    "C:\tools\Miniconda3\condabin",
    (Join-Path $repoRoot "tools"),
    "C:\Program Files\CMake\bin",
    "C:\Program Files\Ninja",
    "C:\tools\ninja",
    "C:\Program Files\LLVM\bin",
    (Join-Path $env:USERPROFILE ".cargo\bin"),
    (Join-Path $env:LOCALAPPDATA "SpacetimeDB")
)
if ($env:VSINSTALLDIR) {
    $pathCandidates += @(
        (Join-Path $env:VSINSTALLDIR "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"),
        (Join-Path $env:VSINSTALLDIR "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"),
        (Join-Path $env:VSINSTALLDIR "VC\Tools\Llvm\x64\bin")
    )
}
Add-RCPathEntries -Candidates $pathCandidates

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
if ((Get-Command cl.exe -ErrorAction SilentlyContinue) -and (Get-Command msbuild.exe -ErrorAction SilentlyContinue)) {
    $env:RC_DEVSHELL_MSBUILD_READY = "1"
} elseif (-not $vsDevLoaded -and $script:RCVsDevImportAttempted) {
    Write-Warning "MSBuild toolchain was not imported. Run .vscode\\vsdev-init.cmd manually if rc-cfg fails."
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

# AI compatibility defaults (set once per shell; explicit user env vars still win).
$aiDefaultEnv = @{
    "RC_AI_PIPELINE_V2" = "on"
    "RC_AI_AUDIT_STRICT" = "off"
    "RC_AI_CONTROLLER_MODEL" = "functiongemma"
    "RC_AI_TRIAGE_MODEL" = "codegemma:2b"
    "RC_AI_SYNTH_FAST_MODEL" = "gemma3:4b"
    "RC_AI_SYNTH_ESCALATION_MODEL" = "gemma3:12b"
    "RC_AI_EMBEDDING_MODEL" = "embeddinggemma"
    "RC_AI_VISION_MODEL" = "granite3.2-vision"
    "RC_AI_OCR_MODEL" = "glm-ocr"
    "OLLAMA_FLASH_ATTENTION" = "1"
    "OLLAMA_KV_CACHE_TYPE" = "f16"
}
foreach ($name in $aiDefaultEnv.Keys) {
    $existing = [string](Get-Item -Path "Env:$name" -ErrorAction SilentlyContinue).Value
    if (-not $existing) {
        Set-Item -Path "Env:$name" -Value $aiDefaultEnv[$name]
    }
}
if (-not $env:RC_AI_BRIDGE_BASE_URL) {
    $env:RC_AI_BRIDGE_BASE_URL = "http://127.0.0.1:7077"
}

# ----------------------------------------------------------------------------------------------------------
# VS Code / IDE Companion Environment Overrides (Matches settings.json terminal.integrated.env.windows)
# ----------------------------------------------------------------------------------------------------------
$ideEnvMap = @{
    # vadimcn.vscode-lldb
    CODELLDB_LAUNCH_CONNECT_FILE  = "c:\Users\teamc\AppData\Roaming\Antigravity\User\workspaceStorage\0eeab2f8f9b1993d251d2f79a7a77f75\vadimcn.vscode-lldb\rpcaddress.txt"

    # google.gemini-cli-vscode-ide-companion
    GEMINI_CLI_IDE_SERVER_PORT    = "61181"
    GEMINI_CLI_IDE_WORKSPACE_PATH = "c:\Users\teamc\Documents\Rogue Cities\RogueCities_UrbanSpatialDesigner"
    GEMINI_CLI_IDE_AUTH_TOKEN     = if ($env:GEMINI_CLI_IDE_AUTH_TOKEN) { $env:GEMINI_CLI_IDE_AUTH_TOKEN } else { "" }

    # Claude Code
    RC_ACTIVE_AGENT               = "claude-code"

    # vscode.git
    GIT_ASKPASS                   = "c:\Users\teamc\AppData\Local\Programs\Antigravity\resources\app\extensions\git\dist\askpass.sh"
    VSCODE_GIT_ASKPASS_NODE       = "C:\Users\teamc\AppData\Local\Programs\Antigravity\Antigravity.exe"
    VSCODE_GIT_ASKPASS_EXTRA_ARGS = ""
    VSCODE_GIT_ASKPASS_MAIN       = "c:\Users\teamc\AppData\Local\Programs\Antigravity\resources\app\extensions\git\dist\askpass-main.js"
    VSCODE_GIT_IPC_HANDLE         = "\\.\pipe\vscode-git-de8377d597-sock"
    
    # eamodio.gitlens
    GK_GL_ADDR                    = "http://127.0.0.1:61180"
    GK_GL_PATH                    = "C:\Users\teamc\AppData\Local\Temp\gitkraken\gitlens\gitlens-ipc-server-202872-61180.json"
    
    # ms-python.python
    PYTHONSTARTUP                 = "c:\Users\teamc\AppData\Roaming\Antigravity\User\workspaceStorage\0eeab2f8f9b1993d251d2f79a7a77f75\ms-python.python\pythonrc.py"
    PYTHON_BASIC_REPL             = "1"
}
foreach ($key in $ideEnvMap.Keys) {
    Set-Item -Path "Env:$key" -Value $ideEnvMap[$key]
}

$env:PATH = "c:\Users\teamc\.antigravity\extensions\vadimcn.vscode-lldb-1.12.1\bin;$env:PATH"

function rc-help {
    <#
    .SYNOPSIS
        Displays the list of available RogueCities development commands.
    #>
    Write-Host "RogueCities dev commands:" -ForegroundColor Cyan
    Write-Host "  rc-cfg     [-Clean] [-Preset $env:RC_DEFAULT_CONFIGURE_PRESET]" -ForegroundColor Yellow
    Write-Host "  rc-bld     [-Preset $env:RC_DEFAULT_BUILD_PRESET]" -ForegroundColor Yellow
    Write-Host "  rc-run" -ForegroundColor Yellow
    Write-Host "  rc-tst     [-Preset $env:RC_DEFAULT_TEST_PRESET]" -ForegroundColor Yellow
    Write-Host "  rc-fmt" -ForegroundColor Yellow
    Write-Host "  rc-deps    [-ExeName RogueCityVisualizerGui.exe]" -ForegroundColor Yellow
    Write-Host "  rc-rdoc" -ForegroundColor Yellow
    Write-Host "  rc-doctor" -ForegroundColor Yellow
    Write-Host "  rc-contract" -ForegroundColor Yellow
    Write-Host "  rc-problems [-MaxItems 10]" -ForegroundColor Yellow
    Write-Host "  rc-pdiff" -ForegroundColor Yellow
    Write-Host "  rc-refresh [-ConfigurePreset dev] [-BuildPreset gui-release]" -ForegroundColor Yellow
    Write-Host "  rc-cfg-vs2022 | rc-cfg-vs2026 | rc-cfg-ninja-msvc | rc-cfg-ninja-clang" -ForegroundColor Yellow
    Write-Host "  rc-bld-ninja-msvc | rc-bld-ninja-clang" -ForegroundColor Yellow
    Write-Host "  rc-env" -ForegroundColor Yellow
    Write-Host "  rc-context" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Layer-Specific Commands:" -ForegroundColor Cyan
    Write-Host "  rc-bld-core | rc-bld-gen | rc-bld-app | rc-bld-vis" -ForegroundColor DarkCyan
    Write-Host "  rc-tst-core | rc-tst-gen | rc-tst-app" -ForegroundColor DarkCyan
    Write-Host "  rc-bld-headless | rc-run-headless | rc-smoke-headless" -ForegroundColor DarkCyan
    Write-Host "  rc-console   (Starts the interactive cyberpunk UI TUI/REPL)" -ForegroundColor DarkCyan
    Write-Host ""
    Write-Host "Advanced Tooling:" -ForegroundColor Cyan
    Write-Host "  rc-watch          [-Target RogueCityVisualizerGui]" -ForegroundColor Magenta
    Write-Host "  rc-index" -ForegroundColor Magenta
    Write-Host "  rc-install-natvis [-Force]  (copies imgui.natvis to VS per-user Visualizers dirs)" -ForegroundColor Magenta
    Write-Host "  rc-imgui-debug-smoke [-ForceNatvis] [-SkipTests] [-ProjectName RogueCityVisualizer] [-PvsDir 'C:\...\PVS-Studio']" -ForegroundColor Magenta
    Write-Host "  rc-pvs-studio     [-PvsDir 'C:\...\PVS-Studio'] [-ProjectName RogueCityVisualizer] [-OutputDir tools/.run/pvs_studio] [-Severity GA:1,2;OP:1] [-OpenReport]" -ForegroundColor Magenta
    Write-Host ""
    Write-Host "AI Integration (Ollama / Local):" -ForegroundColor Cyan
    Write-Host "  rc-ai-setup      (Pulls Gemma-first local AI stack)" -ForegroundColor DarkCyan
    Write-Host "  rc-ai-harden     [-Port 7077] [-StartBridge] [-InstallDeps] (compat defaults + dependency checks)" -ForegroundColor DarkCyan
    Write-Host "  rc-ai-start      [-Port 7077] [-BindAll] [-BindHost 127.0.0.1] (Recycle then start AI Bridge in live mode)" -ForegroundColor DarkCyan
    Write-Host "  rc-ai-restart    [-Port 7077] [-BindAll] [-BindHost 127.0.0.1] (Force stop stale listener then relaunch)" -ForegroundColor DarkCyan
    Write-Host "  rc-ai-start-wsl  [-Port 7077] [-Mock] [-WslHost 127.0.0.1] [-OllamaBaseUrl http://host.docker.internal:11434] (Run bridge in WSL/local runtime)" -ForegroundColor DarkCyan
    Write-Host "  rc-ai-stop-wsl   [-Port 7077] (Stop WSL/local bridge runtime)" -ForegroundColor DarkCyan
    Write-Host "  rc-ai-stop       [-Port 7077] (Stops Python AI Bridge)" -ForegroundColor DarkCyan
    Write-Host "  rc-ai-stop-admin [-Port 7077] (Elevated stop for blocked listeners)" -ForegroundColor DarkCyan
    Write-Host "  rc-ai-smoke      [-Live] [-Model gemma3:4b] [-Port 7077]" -ForegroundColor DarkCyan
    Write-Host "  rc-ai-query      -Prompt \"...\" [-Model gemma3:4b] [-File path1,path2] [-NoStrict] [-Audit] [-Temperature 0.10] [-TopP 0.85] [-NumPredict 256] [-MaxRetries 1] [-IncludeRepoIndex] [-SearchPattern p1,p2]" -ForegroundColor DarkCyan
    Write-Host "  rc-ai-eval       [-CaseFile tools/ai_eval_cases.json] [-Model gemma3:4b] [-MaxCases 10] [-Audit] [-Temperature 0.10] [-TopP 0.85] [-NumPredict 256]" -ForegroundColor DarkCyan
    Write-Host "  rc-mcp-setup     (Installs/updates RogueCity MCP Python package and deps)" -ForegroundColor DarkCyan
    Write-Host "  rc-mcp-smoke     [-Port 7077] (Runs MCP self-test and returns ok_core/ok_full/runtime_recommendation)" -ForegroundColor DarkCyan
    Write-Host "  rc-perceive-ui   [-Mode quick|full] [-Frames 5] [-Screenshot] [-IncludeVision \$true] [-IncludeOcr \$true]" -ForegroundColor DarkCyan
    Write-Host "  rc-perception-audit [-Observations 3] [-Mode quick|full] [-IncludeReports] [-IncludeVision \$true] [-IncludeOcr \$true] [-LatencyGateP95Ms 8000]" -ForegroundColor DarkCyan
    Write-Host "  rc-full-smoke    [-Port 7222] [-Runs 2] (Commit-gate smoke/audit with strict checks)" -ForegroundColor DarkCyan
    Write-Host "  rc-cfg-ai        (Configures CMake with AI Bridge ON)" -ForegroundColor DarkCyan
    Write-Host ""
    Write-Host "Claude Code Agent:" -ForegroundColor Cyan
    Write-Host "  rc-claude        [-Prompt '...'] (Open Claude Code; optional one-shot prompt)" -ForegroundColor Blue
    Write-Host "  rc-claude-status (Show active agent config + memory snapshot)" -ForegroundColor Blue
    Write-Host "  rc-claude-handoff [-Summary '...'] [-NextAgent gemini] (Write session brief to AI/collaboration/)" -ForegroundColor Blue
    Write-Host ""
    Write-Host "SpacetimeDB (Live Database):" -ForegroundColor Cyan
    Write-Host "  rc-db-publish    [-Server http://127.0.0.1:3000] [-Clear] (Build + publish server module)" -ForegroundColor Green
    Write-Host "  rc-db-generate   (Regenerate Rust client bindings from server schema)" -ForegroundColor Green
    Write-Host "  rc-db-status     [-Server http://127.0.0.1:3000] (Show database connection + table row counts)" -ForegroundColor Green
    Write-Host "  rc-db-logs       [-Server http://127.0.0.1:3000] [-Lines 50] (Tail server reducer logs)" -ForegroundColor Green
    Write-Host "  rc-db-clear      [-Server http://127.0.0.1:3000] (Call clear_generated + clear_axioms reducers)" -ForegroundColor Green
    Write-Host "  rc-db-sql        [-Server http://127.0.0.1:3000] -Query '...' (Run SQL query against DB)" -ForegroundColor Green
    Write-Host "  rc-db-build-ffi  (Cargo build rc_db_client staticlib)" -ForegroundColor Green
}

# ----------------------------------------------------------------------------------------------------------
# SpacetimeDB Dev Commands
# ----------------------------------------------------------------------------------------------------------

function rc-db-publish {
    param(
        [string]$Server = "http://127.0.0.1:3000",
        [switch]$Clear
    )
    $dbName = "rogue_cities"
    Push-Location "$env:RC_ROOT"
    try {
        Write-Host "[rc-db-publish] Building + publishing server module to $Server..." -ForegroundColor Green
        if ($Clear) {
            Write-Host "[rc-db-publish] --clear flag: database will be wiped on publish" -ForegroundColor Yellow
            spacetime publish $dbName --project-path server --server $Server --clear-database -y
        } else {
            spacetime publish $dbName --project-path server --server $Server -y
        }
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[rc-db-publish] Published successfully to $Server/$dbName" -ForegroundColor Green
        } else {
            # spacetime publish may use a different arg syntax, try alternative
            Write-Host "[rc-db-publish] Retrying with positional args..." -ForegroundColor Yellow
            spacetime publish $dbName server --server $Server -y
        }
    } finally {
        Pop-Location
    }
}

function rc-db-generate {
    Push-Location "$env:RC_ROOT"
    try {
        Write-Host "[rc-db-generate] Regenerating Rust client bindings from server schema..." -ForegroundColor Green
        spacetime generate --lang rust --module-path server --out-dir rc_db_client/src/bindings -y
        Write-Host "[rc-db-generate] Bindings written to rc_db_client/src/bindings/" -ForegroundColor Green
        Write-Host "[rc-db-generate] Run 'rc-db-build-ffi' to recompile the FFI staticlib." -ForegroundColor DarkGreen
    } finally {
        Pop-Location
    }
}

function rc-db-status {
    param(
        [string]$Server = "http://127.0.0.1:3000"
    )
    $dbName = "rogue_cities"
    Write-Host "[rc-db-status] Querying $Server/$dbName..." -ForegroundColor Green
    $tables = @("axiom", "road", "district", "lot", "building", "generation_stats", "generation_intent")
    foreach ($tbl in $tables) {
        try {
            $result = spacetime sql $dbName "SELECT COUNT(*) as cnt FROM $tbl" --server $Server 2>&1
            Write-Host "  $($tbl.PadRight(20)) $result" -ForegroundColor DarkGreen
        } catch {
            Write-Host "  $($tbl.PadRight(20)) (query failed)" -ForegroundColor DarkYellow
        }
    }
}

function rc-db-logs {
    param(
        [string]$Server = "http://127.0.0.1:3000",
        [int]$Lines = 50
    )
    $dbName = "rogue_cities"
    Write-Host "[rc-db-logs] Tailing last $Lines log lines from $dbName..." -ForegroundColor Green
    spacetime logs $dbName --server $Server -n $Lines
}

function rc-db-clear {
    param(
        [string]$Server = "http://127.0.0.1:3000"
    )
    $dbName = "rogue_cities"
    Write-Host "[rc-db-clear] Calling clear_generated + clear_axioms reducers..." -ForegroundColor Yellow
    spacetime call $dbName clear_generated --server $Server 2>&1
    spacetime call $dbName clear_axioms --server $Server 2>&1
    Write-Host "[rc-db-clear] Done." -ForegroundColor Green
}

function rc-db-sql {
    param(
        [string]$Server = "http://127.0.0.1:3000",
        [Parameter(Mandatory=$true)]
        [string]$Query
    )
    $dbName = "rogue_cities"
    Write-Host "[rc-db-sql] $Query" -ForegroundColor DarkGreen
    spacetime sql $dbName $Query --server $Server
}

function rc-db-build-ffi {
    Push-Location "$env:RC_ROOT/rc_db_client"
    try {
        Write-Host "[rc-db-build-ffi] Building rc_db_client staticlib..." -ForegroundColor Green
        cargo build --release
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[rc-db-build-ffi] rc_db_client.lib built successfully." -ForegroundColor Green
        } else {
            Write-Host "[rc-db-build-ffi] Build failed." -ForegroundColor Red
        }
    } finally {
        Pop-Location
    }
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
    Write-Host "RC_DEVSHELL_MSBUILD_READY=$env:RC_DEVSHELL_MSBUILD_READY"
    Write-Host "VSCMD_VER=$env:VSCMD_VER"
}

function rc-context {
    <#
    .SYNOPSIS
        Outputs the current shell environment configuration as JSON for AI analysis.
    #>
    $context = @{
        Root           = $env:RC_ROOT
        BuildDir       = $env:RC_BUILD_DIR
        Generator      = $env:CMAKE_GENERATOR
        ParallelLevel  = $env:CMAKE_BUILD_PARALLEL_LEVEL
        AvailableTools = (Get-Command rc-* -CommandType Function).Name
        Python         = (Get-Command py -ErrorAction SilentlyContinue).Source
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
        [string]$Preset = $env:RC_DEFAULT_CONFIGURE_PRESET
    )

    if ($Clean -and (Test-Path -LiteralPath $env:RC_BUILD_DIR)) {
        Remove-Item -LiteralPath $env:RC_BUILD_DIR -Recurse -Force
    }

    cmake --preset $Preset
}

function rc-cfg-vs2022 {
    <# .SYNOPSIS Configures with Visual Studio 2022/MSBuild preset. #>
    rc-cfg -Preset "dev-vs2022"
}

function rc-cfg-vs2026 {
    <# .SYNOPSIS Configures with Visual Studio 2026/MSBuild preset. #>
    rc-cfg -Preset "dev-vs2026"
}

function rc-cfg-ninja-msvc {
    <# .SYNOPSIS Configures with Ninja Multi-Config + MSVC toolchain preset. #>
    rc-cfg -Preset "dev-ninja-msvc"
}

function rc-cfg-ninja-clang {
    <# .SYNOPSIS Configures with Ninja Multi-Config + clang-cl preset. #>
    rc-cfg -Preset "dev-ninja-clang"
}

function rc-bld {
    <#
    .SYNOPSIS
        Builds the project using CMake.
    .PARAMETER Preset
        The CMake build preset to use (default: "gui-release").
    #>
    param(
        [string]$Preset = $env:RC_DEFAULT_BUILD_PRESET
    )

    cmake --build --preset $Preset
}

function rc-bld-ninja-msvc {
    <# .SYNOPSIS Builds GUI Release from Ninja+MSVC preset. #>
    cmake --build --preset gui-release-ninja-msvc
}

function rc-bld-ninja-clang {
    <# .SYNOPSIS Builds GUI Release from Ninja+clang-cl preset. #>
    cmake --build --preset gui-release-ninja-clang
}

function rc-bld-core {
    <# .SYNOPSIS Builds just the Core layer. #>
    cmake --build --preset gui-release --target RogueCityCore
}
function rc-bld-gen {
    <# .SYNOPSIS Builds just the Generators layer. #>
    cmake --build --preset gui-release --target RogueCityGenerators
}
function rc-bld-app {
    <# .SYNOPSIS Builds just the App layer. #>
    cmake --build --preset gui-release --target RogueCityApp
}
function rc-bld-vis {
    <# .SYNOPSIS Builds just the Visualizer layer. #>
    cmake --build --preset gui-release --target RogueCityVisualizerGui
}

function rc-bld-headless {
    <# .SYNOPSIS Builds the headless visualizer executable. #>
    cmake --build --preset gui-release --target RogueCityVisualizerHeadless
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

function rc-run-headless {
    <#
    .SYNOPSIS
        Runs the headless visualizer executable.
    #>
    $exe = Join-Path $env:RC_ROOT "bin\RogueCityVisualizerHeadless.exe"
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "Headless executable not found: $exe"
    }
    & $exe
}

function rc-console {
    <#
    .SYNOPSIS
        Starts the cyberpunk interactive dev console (Headless UI).
    #>
    $exe = Join-Path $env:RC_ROOT "bin\RogueCityVisualizerHeadless.exe"
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "Headless executable not found: $exe"
    }
    & $exe --interactive
}

function rc-smoke-headless {
    <#
    .SYNOPSIS
        Quick smoke test for headless visualizer process startup/shutdown.
    .PARAMETER TimeoutSec
        Kill the process if still alive after timeout.
    #>
    param([int]$TimeoutSec = 10)

    $exe = Join-Path $env:RC_ROOT "bin\RogueCityVisualizerHeadless.exe"
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "Headless executable not found: $exe"
    }

    $p = Start-Process -FilePath $exe -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds $TimeoutSec
    if (Get-Process -Id $p.Id -ErrorAction SilentlyContinue) {
        Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        Write-Host "Headless smoke: process exceeded timeout and was terminated." -ForegroundColor Yellow
    } else {
        Write-Host "Headless smoke: process exited within timeout." -ForegroundColor Green
    }
}

function rc-tst {
    <#
    .SYNOPSIS
        Runs project tests using CTest.
    .PARAMETER Preset
        The CTest preset to use (default: "ctest-release").
    #>
    param(
        [string]$Preset = $env:RC_DEFAULT_TEST_PRESET
    )

    ctest --preset $Preset
}

function rc-tst-core {
    <# .SYNOPSIS Runs Core layer tests. #>
    ctest --preset ctest-release -R "core"
}

function rc-tst-gen {
    <# .SYNOPSIS Runs Generators layer tests. #>
    ctest --preset ctest-release -R "generator"
}

function rc-tst-app {
    <# .SYNOPSIS Runs App layer tests. #>
    ctest --preset ctest-release -R "app"
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
    }
    else {
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
    }
    else {
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
    }
    else {
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

    $scriptArgs = @($script) + $Args
    $attempts = @()

    $launcherCandidates = @()
    $cmdPy = Get-Command py -ErrorAction SilentlyContinue
    if ($cmdPy) { $launcherCandidates += @{ exe = $cmdPy.Source; args = @("-3") + $scriptArgs } }
    $cmdPython = Get-Command python -ErrorAction SilentlyContinue
    if ($cmdPython) { $launcherCandidates += @{ exe = $cmdPython.Source; args = $scriptArgs } }
    $cmdPython3 = Get-Command python3 -ErrorAction SilentlyContinue
    if ($cmdPython3) { $launcherCandidates += @{ exe = $cmdPython3.Source; args = $scriptArgs } }

    $localAppData = [Environment]::GetEnvironmentVariable("LOCALAPPDATA")
    $winDir = [Environment]::GetEnvironmentVariable("WINDIR")
    $pathCandidates = @(
        (Join-Path $winDir "py.exe"),
        (Join-Path $localAppData "Programs\Python\Python313\python.exe"),
        (Join-Path $localAppData "Programs\Python\Python312\python.exe"),
        (Join-Path $localAppData "Programs\Python\Python311\python.exe")
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }
    foreach ($exePath in $pathCandidates) {
        if ([IO.Path]::GetFileName($exePath).ToLowerInvariant() -eq "py.exe") {
            $launcherCandidates += @{ exe = $exePath; args = @("-3") + $scriptArgs }
        }
        else {
            $launcherCandidates += @{ exe = $exePath; args = $scriptArgs }
        }
    }

    # De-duplicate equivalent launch attempts.
    $seen = @{}
    $uniqueCandidates = @()
    foreach ($candidate in $launcherCandidates) {
        $key = ($candidate.exe + "||" + (($candidate.args -join "`u{1F}")))
        if (-not $seen.ContainsKey($key)) {
            $seen[$key] = $true
            $uniqueCandidates += $candidate
        }
    }

    Push-Location $env:RC_ROOT
    try {
        foreach ($candidate in $uniqueCandidates) {
            & $candidate.exe @($candidate.args)
            $ok = $? -and (($null -eq $LASTEXITCODE) -or ($LASTEXITCODE -eq 0))
            if ($ok) {
                return
            }
            $attempts += "$($candidate.exe) (exit=$LASTEXITCODE)"
        }

        # Last-resort fallback: call through cmd.exe which may resolve app aliases
        # unavailable to direct PowerShell command discovery.
        $cmdExe = Get-Command cmd.exe -ErrorAction SilentlyContinue
        if ($cmdExe) {
            $argString = ($scriptArgs | ForEach-Object { '"' + ($_ -replace '"', '\"') + '"' }) -join " "
            & $cmdExe.Source /c "py -3 $argString"
            $ok = $? -and (($null -eq $LASTEXITCODE) -or ($LASTEXITCODE -eq 0))
            if ($ok) {
                return
            }
            $attempts += "cmd.exe /c py -3 (exit=$LASTEXITCODE)"

            & $cmdExe.Source /c "python $argString"
            $ok = $? -and (($null -eq $LASTEXITCODE) -or ($LASTEXITCODE -eq 0))
            if ($ok) {
                return
            }
            $attempts += "cmd.exe /c python (exit=$LASTEXITCODE)"
        }
    }
    finally {
        Pop-Location
    }

    $attemptSummary = if ($attempts.Count -gt 0) { ($attempts -join "; ") } else { "no launchers discovered" }
    throw "Python tool execution failed for: $ScriptPath. Attempts: $attemptSummary"
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

function rc-watch {
    <#
    .SYNOPSIS
        Watches source files and rebuilds the specified target on change.
    .PARAMETER Target
        The CMake target to rebuild (default: RogueCityVisualizerGui).
    #>
    param([string]$Target = "RogueCityVisualizerGui")
    Invoke-RCPythonTool -ScriptPath "tools\dev_watch.py" -Args @($Target)
}

function rc-index {
    <#
    .SYNOPSIS
        Generates a symbolic index of the codebase for AI agents.
    #>
    Invoke-RCPythonTool -ScriptPath "tools\dev_index.py"
}

# ==============================================================================
# Claude Code Agent Commands
# ==============================================================================

function rc-claude {
    <#
    .SYNOPSIS
        Opens Claude Code in the project root, optionally with a one-shot prompt.
    .PARAMETER Prompt
        If provided, runs Claude Code in print mode with the given prompt.
    #>
    param([string]$Prompt = "")
    Push-Location $env:RC_ROOT
    try {
        if ($Prompt) {
            & claude --print $Prompt
        } else {
            & claude
        }
    } finally {
        Pop-Location
    }
}

function rc-claude-status {
    <#
    .SYNOPSIS
        Prints Claude Code active agent config and first 40 lines of its memory file.
    #>
    Write-Host "RC_ACTIVE_AGENT    = $env:RC_ACTIVE_AGENT" -ForegroundColor Cyan
    Write-Host "RC_AI_BRIDGE_URL   = $env:RC_AI_BRIDGE_BASE_URL" -ForegroundColor Cyan
    Write-Host "RC_AI_PIPELINE_V2  = $env:RC_AI_PIPELINE_V2" -ForegroundColor Cyan

    $memPath = Join-Path $env:USERPROFILE ".claude\projects\C--Users-teamc-Documents-Rogue-Cities-RogueCities-UrbanSpatialDesigner\memory\MEMORY.md"
    if (Test-Path -LiteralPath $memPath) {
        Write-Host "`nClaude Memory (MEMORY.md - first 40 lines):" -ForegroundColor Yellow
        Get-Content -LiteralPath $memPath | Select-Object -First 40 | ForEach-Object {
            Write-Host "  $_" -ForegroundColor Gray
        }
    } else {
        Write-Warning "Claude memory file not found: $memPath"
    }
}

function rc-claude-handoff {
    <#
    .SYNOPSIS
        Writes a Claude Code session handoff brief to AI/collaboration/ for the next agent.
    .PARAMETER Summary
        Short summary of what was accomplished this session.
    .PARAMETER NextAgent
        The agent expected to read this brief (default: gemini).
    #>
    param(
        [string]$Summary = "(no summary provided)",
        [string]$NextAgent = "gemini"
    )
    $date = Get-Date -Format "yyyy-MM-dd"
    $filename = "claude_${date}_handoff.md"
    $outPath = Join-Path $env:RC_ROOT "AI\collaboration\$filename"

    $commands = (Get-Command rc-* -CommandType Function).Name -join ", "
    $content = @"
# Claude Code Handoff Brief - $date

## Summary
$Summary

## Environment Snapshot
- Active Agent : $env:RC_ACTIVE_AGENT
- AI Bridge URL: $env:RC_AI_BRIDGE_BASE_URL
- RC Root      : $env:RC_ROOT
- Pipeline V2  : $env:RC_AI_PIPELINE_V2

## Available RC Commands
$commands

## Recommended Next Steps for $NextAgent
- Read ``AI/collaboration/`` for prior session briefs
- Run ``rc-ai-harden`` to confirm AI stack health
- Run ``rc-mcp-smoke`` to validate bridge + MCP
- Run ``rc-perceive-ui`` to capture current UI snapshot

## Memory Reference
Claude Code project memory: ``.claude\projects\C--Users-teamc-Documents-Rogue-Cities-RogueCities-UrbanSpatialDesigner\memory\MEMORY.md``
"@
    Set-Content -LiteralPath $outPath -Value $content -Encoding UTF8
    Write-Host "Handoff brief written: $outPath" -ForegroundColor Green
    Write-Host "Share with ${NextAgent}: AI/collaboration/$filename" -ForegroundColor Cyan
}

# ==============================================================================
# Debugger Support — Natvis
# ==============================================================================

function rc-install-natvis {
    <#
    .SYNOPSIS
        Copies imgui.natvis to all discovered Visual Studio per-user Visualizers
        directories so the VS debugger picks it up automatically for every solution.
    .DESCRIPTION
        Option 1 of the Dear ImGui natvis setup: per-user auto-load path.
        Option 2 (CMake target_sources) wires it into the generated .vcxproj
        for project-scoped loading. Running both ensures coverage regardless of
        how VS / VS Code loads the project.
    .PARAMETER Force
        Overwrite an existing imgui.natvis in each Visualizers directory.
    .EXAMPLE
        rc-install-natvis
        rc-install-natvis -Force
    #>
    param([switch]$Force)

    $natvis_src = Join-Path $env:RC_ROOT "3rdparty\imgui\misc\debuggers\imgui.natvis"
    if (-not (Test-Path -LiteralPath $natvis_src)) {
        Write-Warning "imgui.natvis not found at '$natvis_src'. Ensure the imgui submodule is initialised."
        return
    }

    $docs = [Environment]::GetFolderPath("MyDocuments")

    # Seed with known VS versions, then include any discovered Visual Studio folders.
    $vs_dirs = @(
        "Visual Studio 2026\Visualizers",
        "Visual Studio 2022\Visualizers",
        "Visual Studio 2019\Visualizers",
        "Visual Studio 2017\Visualizers"
    ) | ForEach-Object { Join-Path $docs $_ }

    $discovered = Get-ChildItem -Path $docs -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like "Visual Studio *" } |
        ForEach-Object { Join-Path $_.FullName "Visualizers" }
    $vs_dirs = @($vs_dirs + $discovered | Select-Object -Unique)

    $installed = 0
    foreach ($dir in $vs_dirs) {
        $dest = Join-Path $dir "imgui.natvis"
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        if ((Test-Path -LiteralPath $dest) -and -not $Force) {
            Write-Host "  [skip] already exists: $dest  (use -Force to overwrite)" -ForegroundColor DarkGray
            $installed++
            continue
        }
        Copy-Item -LiteralPath $natvis_src -Destination $dest -Force
        Write-Host "  [ok]   $dest" -ForegroundColor Green
        $installed++
    }

    Write-Host "`n[natvis] imgui.natvis installed for $installed VS version(s)." -ForegroundColor Cyan
    Write-Host "  Restart Visual Studio / VS Code debugger session to apply." -ForegroundColor DarkCyan
    Write-Host "  CMake target_sources also wires it into .vcxproj on next rc-cfg." -ForegroundColor DarkCyan
}

function rc-imgui-debug-smoke {
    <#
    .SYNOPSIS
        One-shot ImGui debug tooling smoke check: natvis install, PVS presence, and core tests.
    #>
    param(
        [switch]$ForceNatvis,
        [switch]$SkipTests,
        [string]$ProjectName = "RogueCityVisualizer",
        [string]$PvsDir = "C:\Program Files (x86)\PVS-Studio"
    )

    Write-Host "[imgui-debug-smoke] Step 1/3: Natvis install check" -ForegroundColor Cyan
    rc-install-natvis -Force:$ForceNatvis

    Write-Host "[imgui-debug-smoke] Step 2/3: PVS-Studio readiness" -ForegroundColor Cyan
    $pvsCmd = Join-Path $PvsDir "PVS-Studio_Cmd.exe"
    if (Test-Path -LiteralPath $pvsCmd) {
        Write-Host "  [ok] PVS-Studio found at: $pvsCmd" -ForegroundColor Green
    } else {
        Write-Warning "  [warn] PVS-Studio not found at '$pvsCmd'."
        Write-Warning "        Install it or pass -PvsDir, then run rc-pvs-studio."
    }

    Write-Host "[imgui-debug-smoke] Step 3/3: CTest core smoke" -ForegroundColor Cyan
    if ($SkipTests) {
        Write-Host "  [skip] core tests skipped by -SkipTests." -ForegroundColor DarkGray
    } else {
        rc-tst-core
    }

    $summary = [ordered]@{
        natvis_source = Join-Path $env:RC_ROOT "3rdparty\imgui\misc\debuggers\imgui.natvis"
        pvs_cmd = $pvsCmd
        pvs_present = (Test-Path -LiteralPath $pvsCmd)
        tests_ran = (-not $SkipTests)
    }
    Write-Host "[imgui-debug-smoke] Summary:" -ForegroundColor Cyan
    Write-Output ($summary | ConvertTo-Json -Depth 4)
}

# ==============================================================================
# Static Analysis — PVS-Studio
# ==============================================================================

function rc-pvs-studio {
    <#
    .SYNOPSIS
        Run PVS-Studio static analysis on a RogueCities MSVC project and emit
        HTML, FullHTML, TXT, and Totals reports.
    .DESCRIPTION
        PowerShell adaptation of the run_pvs_studio.bat recipe from the Dear ImGui
        developer tips. Locates the .vcxproj under RC_BUILD_DIR, runs analysis,
        converts the .plog, and optionally opens the HTML report.
    .PARAMETER PvsDir
        PVS-Studio installation directory.
        Default: "C:\Program Files (x86)\PVS-Studio"
    .PARAMETER ProjectName
        MSVC project name to analyze (searched recursively under RC_BUILD_DIR).
        Default: "RogueCityVisualizer"
    .PARAMETER OutputDir
        Directory for analysis reports. Created if absent.
        Default: tools/.run/pvs_studio
    .PARAMETER Severity
        PVS-Studio severity filter passed to PlogConverter.
        Default: "GA:1,2;OP:1"  (general analysis levels 1-2 + optimization level 1)
    .PARAMETER OpenReport
        When set, open fullhtml/index.html in the default browser after analysis.
    .EXAMPLE
        rc-pvs-studio
        rc-pvs-studio -ProjectName RogueCityCore -OpenReport
        rc-pvs-studio -PvsDir "D:\PVS-Studio" -OutputDir "out/pvs"
    #>
    param(
        [string]$PvsDir      = "C:\Program Files (x86)\PVS-Studio",
        [string]$ProjectName = "RogueCityVisualizer",
        [string]$OutputDir   = (Join-Path $env:RC_ROOT "tools\.run\pvs_studio"),
        [string]$Severity    = "GA:1,2;OP:1",
        [switch]$OpenReport
    )

    $pvs_cmd       = Join-Path $PvsDir "PVS-Studio_Cmd.exe"
    $plog_conv     = Join-Path $PvsDir "PlogConverter.exe"

    if (-not (Test-Path -LiteralPath $pvs_cmd)) {
        Write-Warning "[PVS-Studio] PVS-Studio_Cmd.exe not found at '$pvs_cmd'."
        Write-Warning "  Install PVS-Studio from https://pvs-studio.com (free licence available for OSS)."
        Write-Warning "  Or pass -PvsDir '<install-path>'."
        return
    }

    if (-not $env:RC_BUILD_DIR) {
        Write-Warning "[PVS-Studio] RC_BUILD_DIR is not set. Run rc-cfg first."
        return
    }

    $proj_file = Get-ChildItem -Path $env:RC_BUILD_DIR `
                               -Filter "$ProjectName.vcxproj" `
                               -Recurse `
                               -ErrorAction SilentlyContinue |
                 Select-Object -First 1

    if (-not $proj_file) {
        Write-Warning "[PVS-Studio] Could not find '$ProjectName.vcxproj' under '$env:RC_BUILD_DIR'."
        Write-Warning "  Ensure the project is configured (rc-cfg) and check -ProjectName."
        return
    }

    $plog_path = Join-Path $proj_file.DirectoryName "$ProjectName.plog"

    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

    Write-Host "[PVS-Studio] Analysing: $($proj_file.FullName)" -ForegroundColor Cyan
    & "$pvs_cmd" -r -t "$($proj_file.FullName)" -o "$plog_path"

    if (-not (Test-Path -LiteralPath $plog_path)) {
        Write-Warning "[PVS-Studio] No .plog produced at '$plog_path'. Analysis may have failed."
        return
    }

    Write-Host "[PVS-Studio] Converting report (severity: $Severity)..." -ForegroundColor Cyan
    & "$plog_conv" -a $Severity -t "Html,FullHtml,Txt,Totals" "$plog_path" -o "$OutputDir"
    Remove-Item -LiteralPath $plog_path -ErrorAction SilentlyContinue

    $totals = Join-Path $OutputDir "$ProjectName.plog_totals.txt"
    if (Test-Path -LiteralPath $totals) {
        Write-Host "`n---- PVS-Studio Totals:" -ForegroundColor Yellow
        Get-Content -LiteralPath $totals
    }

    Write-Host "`n[PVS-Studio] Reports written to: $OutputDir" -ForegroundColor Green

    if ($OpenReport) {
        $index = Join-Path $OutputDir "fullhtml\index.html"
        if (Test-Path -LiteralPath $index) {
            Start-Process $index
        } else {
            Write-Warning "[PVS-Studio] Full HTML report not found at '$index'."
        }
    }
}

# ==============================================================================
# Terminal Auto-Completion Handlers
# ==============================================================================

# Provide tab-completion for CMake setup presets
Register-ArgumentCompleter -CommandName rc-cfg -ParameterName Preset -ScriptBlock {
    param($commandName, $parameterName, $wordToComplete, $commandAst, $fakeBoundParameters)
    @("dev", "dev-vs2022", "dev-vs2026", "dev-ninja-msvc", "dev-ninja-clang") | Where-Object { $_ -like "$wordToComplete*" }
}

# Provide tab-completion for CMake build presets
Register-ArgumentCompleter -CommandName rc-bld -ParameterName Preset -ScriptBlock {
    param($commandName, $parameterName, $wordToComplete, $commandAst, $fakeBoundParameters)
    @("gui-release", "gui-debug", "gui-release-vs2026", "gui-debug-vs2026", "gui-release-ninja-msvc", "gui-debug-ninja-msvc", "gui-release-ninja-clang", "gui-debug-ninja-clang") | Where-Object { $_ -like "$wordToComplete*" }
}

# Provide tab-completion for CTest presets
Register-ArgumentCompleter -CommandName rc-tst -ParameterName Preset -ScriptBlock {
    param($commandName, $parameterName, $wordToComplete, $commandAst, $fakeBoundParameters)
    @("ctest-release", "ctest-release-vs2026", "ctest-release-ninja-msvc", "ctest-release-ninja-clang") | Where-Object { $_ -like "$wordToComplete*" }
}

# Provide tab-completion for specific known CMake targets in rc-watch
Register-ArgumentCompleter -CommandName rc-watch -ParameterName Target -ScriptBlock {
    param($commandName, $parameterName, $wordToComplete, $commandAst, $fakeBoundParameters)
    @("RogueCityVisualizerGui", "RogueCityVisualizerHeadless", "RogueCityCore", "RogueCityGenerators", "RogueCityApp") | Where-Object { $_ -like "$wordToComplete*" }
}

# ==============================================================================
# AI Integration Commands
# ==============================================================================

function Resolve-RCPythonLauncher {
    $launcherCandidates = @()

    $venvPython = Join-Path $env:RC_ROOT ".venv\Scripts\python.exe"
    if (Test-Path -LiteralPath $venvPython) {
        $launcherCandidates += @{ exe = $venvPython; argsPrefix = @() }
    }

    $cmdPy = Get-Command py -ErrorAction SilentlyContinue
    if ($cmdPy) { $launcherCandidates += @{ exe = $cmdPy.Source; argsPrefix = @("-3") } }
    $cmdPython = Get-Command python -ErrorAction SilentlyContinue
    if ($cmdPython) { $launcherCandidates += @{ exe = $cmdPython.Source; argsPrefix = @() } }
    $cmdPython3 = Get-Command python3 -ErrorAction SilentlyContinue
    if ($cmdPython3) { $launcherCandidates += @{ exe = $cmdPython3.Source; argsPrefix = @() } }

    $localAppData = [Environment]::GetEnvironmentVariable("LOCALAPPDATA")
    $winDir = [Environment]::GetEnvironmentVariable("WINDIR")
    $pathCandidates = @(
        (Join-Path $winDir "py.exe"),
        (Join-Path $localAppData "Programs\Python\Python313\python.exe"),
        (Join-Path $localAppData "Programs\Python\Python312\python.exe"),
        (Join-Path $localAppData "Programs\Python\Python311\python.exe"),
        (Join-Path $localAppData "Programs\Python\Python310\python.exe")
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    foreach ($p in $pathCandidates) {
        if ([IO.Path]::GetFileName($p).ToLowerInvariant() -eq "py.exe") {
            $launcherCandidates += @{ exe = $p; argsPrefix = @("-3") }
        }
        else {
            $launcherCandidates += @{ exe = $p; argsPrefix = @() }
        }
    }

    foreach ($candidate in $launcherCandidates) {
        if ($candidate.exe -and (Test-Path -LiteralPath $candidate.exe)) {
            return $candidate
        }
    }
    throw "No Python launcher found (expected py/python/python3 or .venv\\Scripts\\python.exe)."
}

function Invoke-RCPythonModule {
    param([Parameter(Mandatory = $true)][string[]]$Args)
    $launcher = Resolve-RCPythonLauncher
    $fullArgs = @($launcher.argsPrefix + $Args)
    Push-Location $env:RC_ROOT
    try {
        & $launcher.exe @fullArgs
        $ok = $? -and (($null -eq $LASTEXITCODE) -or ($LASTEXITCODE -eq 0))
        if (-not $ok) {
            throw "Python command failed: $($launcher.exe) $($fullArgs -join ' ') (exit=$LASTEXITCODE)"
        }
    }
    finally {
        Pop-Location
    }
}

function Set-RCAiCompatibilityEnv {
    param([int]$Port = 7077)
    $defaults = @{
        "RC_AI_PIPELINE_V2" = "on"
        "RC_AI_AUDIT_STRICT" = "off"
        "RC_AI_CONTROLLER_MODEL" = "functiongemma"
        "RC_AI_TRIAGE_MODEL" = "codegemma:2b"
        "RC_AI_SYNTH_FAST_MODEL" = "gemma3:4b"
        "RC_AI_SYNTH_ESCALATION_MODEL" = "gemma3:12b"
        "RC_AI_EMBEDDING_MODEL" = "embeddinggemma"
        "RC_AI_VISION_MODEL" = "granite3.2-vision"
        "RC_AI_OCR_MODEL" = "glm-ocr"
        "OLLAMA_FLASH_ATTENTION" = "1"
        "OLLAMA_KV_CACHE_TYPE" = "f16"
    }
    foreach ($name in $defaults.Keys) {
        $existing = [string](Get-Item -Path "Env:$name" -ErrorAction SilentlyContinue).Value
        if (-not $existing) {
            Set-Item -Path "Env:$name" -Value $defaults[$name]
        }
    }
    $env:RC_AI_BRIDGE_BASE_URL = "http://127.0.0.1:$Port"
}

function Get-RCOllamaBaseUrl {
    <#
    .SYNOPSIS
        Resolves the best Ollama base URL for this shell context.
    .PARAMETER ProbeReachable
        When set, probes candidate endpoints and returns the first reachable URL.
    #>
    param([switch]$ProbeReachable)

    $explicit = [string](Get-Item -Path "Env:OLLAMA_BASE_URL" -ErrorAction SilentlyContinue).Value
    if ($explicit) {
        return $explicit.Trim().TrimEnd('/')
    }

    $candidates = @("http://127.0.0.1:11434", "http://host.docker.internal:11434")
    if ($IsLinux -and (Test-Path -LiteralPath "/etc/resolv.conf")) {
        try {
            $nameserver = (Get-Content -LiteralPath "/etc/resolv.conf" -ErrorAction SilentlyContinue |
                Where-Object { $_ -match '^nameserver\s+' } |
                Select-Object -First 1)
            if ($nameserver) {
                $parts = $nameserver -split '\s+'
                if ($parts.Count -ge 2 -and $parts[1]) {
                    $candidates += "http://$($parts[1]):11434"
                }
            }
        }
        catch {
            # no-op: keep defaults
        }
    }
    $candidates = @($candidates | Select-Object -Unique)

    if (-not $ProbeReachable) {
        return $candidates[0]
    }

    foreach ($base in $candidates) {
        try {
            Invoke-RestMethod -Uri "$($base.TrimEnd('/'))/api/tags" -Method Get -TimeoutSec 3 | Out-Null
            return $base.TrimEnd('/')
        }
        catch {
            continue
        }
    }
    return $candidates[0]
}

function Get-RCOllamaApiUrl {
    param([string]$Path = "/api/tags", [switch]$ProbeReachable)
    $base = Get-RCOllamaBaseUrl -ProbeReachable:$ProbeReachable
    if (-not $Path.StartsWith("/")) {
        $Path = "/$Path"
    }
    return "$($base.TrimEnd('/'))$Path"
}

function rc-ai-harden {
    <#
    .SYNOPSIS
        Applies hardened AI env defaults, validates model/runtime prerequisites, and optionally installs dependencies.
    #>
    param(
        [int]$Port = 7077,
        [switch]$InstallDeps,
        [switch]$StartBridge
    )

    Set-RCAiCompatibilityEnv -Port $Port
    $requiredModels = @(
        "functiongemma:latest",
        "embeddinggemma:latest",
        "codegemma:2b",
        "gemma3:4b",
        "gemma3:12b",
        "granite3.2-vision:latest",
        "glm-ocr:latest"
    )

    $missingModels = @()
    $ollamaReachable = $false
    $ollamaTagsUrl = Get-RCOllamaApiUrl -Path "/api/tags" -ProbeReachable
    try {
        $tags = Invoke-RestMethod -Uri $ollamaTagsUrl -Method Get -TimeoutSec 6
        $ollamaReachable = $true
        $present = @($tags.models | ForEach-Object { $_.name })
        foreach ($m in $requiredModels) {
            if (-not ($present -contains $m)) {
                $missingModels += $m
            }
        }
    }
    catch {
        $ollamaReachable = $false
    }

    if ($InstallDeps) {
        Invoke-RCPythonModule -Args @("-m", "pip", "install", "-U", "fastapi", "uvicorn", "httpx", "pydantic")
        rc-mcp-setup -Upgrade
    }

    if ($StartBridge) {
        rc-ai-restart -Port $Port
    }

    $summary = [ordered]@{
        bridge_url = $env:RC_AI_BRIDGE_BASE_URL
        pipeline_v2 = $env:RC_AI_PIPELINE_V2
        audit_strict = $env:RC_AI_AUDIT_STRICT
        models = [ordered]@{
            controller = $env:RC_AI_CONTROLLER_MODEL
            triage = $env:RC_AI_TRIAGE_MODEL
            synth_fast = $env:RC_AI_SYNTH_FAST_MODEL
            synth_escalation = $env:RC_AI_SYNTH_ESCALATION_MODEL
            embedding = $env:RC_AI_EMBEDDING_MODEL
            vision = $env:RC_AI_VISION_MODEL
            ocr = $env:RC_AI_OCR_MODEL
        }
        ollama_tuning = [ordered]@{
            flash_attention = $env:OLLAMA_FLASH_ATTENTION
            kv_cache_type = $env:OLLAMA_KV_CACHE_TYPE
        }
        ollama_base_url = (Get-RCOllamaBaseUrl)
        ollama_reachable = $ollamaReachable
        missing_models = $missingModels
        hints = @(
            "Run rc-ai-setup to pull missing models.",
            "Run rc-mcp-smoke -Port $Port after bridge start."
        )
    }
    Write-Output ($summary | ConvertTo-Json -Depth 8)
}

function rc-ai-setup {
    <# .SYNOPSIS Pulls required models via Ollama. #>
    $models = @(
        "functiongemma:latest",
        "embeddinggemma:latest",
        "codegemma:2b",
        "gemma3:4b",
        "gemma3:12b",
        "granite3.2-vision:latest",
        "glm-ocr:latest"
    )
    Write-Host "Pulling Gemma-first AI stack via Ollama..." -ForegroundColor Cyan
    foreach ($m in $models) {
        Write-Host "  ollama pull $m" -ForegroundColor Gray
        ollama pull $m
    }
    Write-Host "Core Gemma-first stack is ready." -ForegroundColor Green
}

function rc-mcp-setup {
    <# .SYNOPSIS Installs or updates the RogueCity MCP package and dependencies. #>
    param(
        [switch]$Upgrade,
        [switch]$UserInstall
    )

    $mcpRoot = Join-Path $env:RC_ROOT "tools\mcp-server\roguecity-mcp"
    if (-not (Test-Path -LiteralPath $mcpRoot)) {
        throw "RogueCity MCP path not found: $mcpRoot"
    }

    $pipArgs = @("-m", "pip", "install")
    if ($Upgrade) { $pipArgs += "-U" }
    if ($UserInstall) { $pipArgs += "--user" }
    $pipArgs += @("-e", $mcpRoot)
    Invoke-RCPythonModule -Args $pipArgs
    Invoke-RCPythonModule -Args @("-c", "import mcp, httpx, pydantic; print('roguecity_mcp_ready')")
}

function rc-mcp-smoke {
    <# .SYNOPSIS Runs deterministic MCP/bridge readiness checks and returns JSON. #>
    param([int]$Port = 7077)
    Set-RCAiCompatibilityEnv -Port $Port
    $baseUrl = "http://127.0.0.1:$Port"

    $pythonOk = $false
    try {
        Invoke-RCPythonModule -Args @("-c", "import mcp, httpx, pydantic")
        $pythonOk = $true
    }
    catch {
        $pythonOk = $false
    }

    $health = $null
    $pipelineStatus = 0
    $perceptionStatus = 0
    $bridgeOk = $false
    try {
        $health = Invoke-RestMethod -Uri "$baseUrl/health" -Method Get -TimeoutSec 8
        $bridgeOk = $true
    }
    catch {
        $bridgeOk = $false
    }

    try {
        Invoke-WebRequest -Uri "$baseUrl/pipeline/query" -Method Get -UseBasicParsing -TimeoutSec 8 -ErrorAction Stop | Out-Null
        $pipelineStatus = 200
    }
    catch {
        if ($_.Exception.Response -and $_.Exception.Response.StatusCode) {
            $pipelineStatus = [int]$_.Exception.Response.StatusCode
        }
    }

    try {
        Invoke-WebRequest -Uri "$baseUrl/perception/observe" -Method Get -UseBasicParsing -TimeoutSec 8 -ErrorAction Stop | Out-Null
        $perceptionStatus = 200
    }
    catch {
        if ($_.Exception.Response -and $_.Exception.Response.StatusCode) {
            $perceptionStatus = [int]$_.Exception.Response.StatusCode
        }
    }

    $summary = [ordered]@{
        bridge_url = $baseUrl
        python_mcp_deps_ok = $pythonOk
        bridge_health_ok = $bridgeOk
        health = $health
        pipeline_endpoint_present = ($pipelineStatus -ne 404 -and $pipelineStatus -ne 0)
        pipeline_probe_status = $pipelineStatus
        perception_endpoint_present = ($perceptionStatus -ne 404 -and $perceptionStatus -ne 0)
        perception_probe_status = $perceptionStatus
        ok_core = $false
        ok_full = $false
        runtime_recommendation = "powershell_primary"
        mcp_self_test = $null
    }

    try {
        $mcpMain = Join-Path $env:RC_ROOT "tools\mcp-server\roguecity-mcp\main.py"
        if (Test-Path -LiteralPath $mcpMain) {
            $py = Resolve-RCPythonLauncher
            $args = @($py.argsPrefix + @($mcpMain, "--self-test", "--toolserver-url", $baseUrl))
            $raw = & $py.exe @args
            if ($LASTEXITCODE -eq 0 -and $raw) {
                $selfTest = $raw | ConvertFrom-Json -ErrorAction Stop
                $summary.mcp_self_test = $selfTest
                $summary.ok_core = [bool]$selfTest.ok_core
                $summary.ok_full = [bool]$selfTest.ok_full
                $summary.runtime_recommendation = [string]$selfTest.runtime_recommendation
            }
        }
    } catch {
        # Fall back to local readiness checks below when MCP self-test cannot run.
    }

    if (-not $summary.ok_core) {
        $summary.ok_core = [bool]($pythonOk -and $bridgeOk -and $summary.pipeline_endpoint_present -and $summary.perception_endpoint_present)
    }
    if (-not $summary.ok_full) {
        $summary.ok_full = $summary.ok_core
    }
    if (-not $summary.mcp_self_test) {
        $summary.runtime_recommendation = if ($summary.ok_full) { "powershell_or_wsl" } else { "powershell_primary" }
    }

    if ($summary.runtime_recommendation -eq "powershell_primary") {
        $stopCmd = "powershell -NoProfile -ExecutionPolicy Bypass -File `"$env:RC_ROOT\tools\Stop_Ai_Bridge_Fixed.ps1`" -NonInteractive -Port $Port"
        $restartCmd = "powershell -NoProfile -ExecutionPolicy Bypass -File `"$env:RC_ROOT\tools\Start_Ai_Bridge_Fixed.ps1`" -MockMode:`$false -NonInteractive -ForceRestart -Port $Port -BindAll"
        $summary.fallback_commands = [ordered]@{
            stop_command = $stopCmd
            restart_command = $restartCmd
            set_bridge_url = "`$env:RC_AI_BRIDGE_BASE_URL = `"http://127.0.0.1:$Port`""
        }
    }
    Write-Output ($summary | ConvertTo-Json -Depth 8)
}

function rc-ai-start {
    <# .SYNOPSIS Starts the Python AI Bridge and connects to Ollama. #>
    param(
        [int]$Port = 7077,
        [switch]$BindAll,
        [string]$BindHost = "127.0.0.1"
    )
    Write-Host "Starting AI Bridge (Live Mode, standardized restart flow)..." -ForegroundColor Cyan
    if ($BindAll) {
        & "$repoRoot\tools\Start_Ai_Bridge_Fixed.ps1" -MockMode:$false -NonInteractive -ForceRestart -Port:$Port -BindAll
    } else {
        & "$repoRoot\tools\Start_Ai_Bridge_Fixed.ps1" -MockMode:$false -NonInteractive -ForceRestart -Port:$Port -BindHost:$BindHost
    }
}

function rc-ai-restart {
    <# .SYNOPSIS Force-restarts the Python AI Bridge. #>
    param(
        [int]$Port = 7077,
        [switch]$BindAll,
        [string]$BindHost = "127.0.0.1"
    )
    Write-Host "Force-restarting AI Bridge..." -ForegroundColor Cyan
    & "$repoRoot\tools\Stop_Ai_Bridge_Fixed.ps1" -NonInteractive -Port:$Port
    Start-Sleep -Milliseconds 500
    if ($BindAll) {
        & "$repoRoot\tools\Start_Ai_Bridge_Fixed.ps1" -MockMode:$false -NonInteractive -ForceRestart -Port:$Port -BindAll
    } else {
        & "$repoRoot\tools\Start_Ai_Bridge_Fixed.ps1" -MockMode:$false -NonInteractive -ForceRestart -Port:$Port -BindHost:$BindHost
    }
}

function rc-ai-start-wsl {
    <# .SYNOPSIS Starts a local bridge process in WSL/Linux runtime for same-environment MCP usage. #>
    param(
        [int]$Port = 7077,
        [switch]$Mock,
        [string]$WslHost = "127.0.0.1",
        [string]$OllamaBaseUrl = ""
    )
    $script = Join-Path $repoRoot "tools\start_ai_bridge_wsl.sh"
    if (-not (Test-Path -LiteralPath $script)) {
        throw "WSL bridge starter not found: $script"
    }
    if (-not $IsLinux -and -not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) {
        throw "wsl.exe not found; cannot launch WSL bridge runtime from this shell."
    }
    function Convert-ToWslPath {
        param([string]$PathValue)
        if (-not $PathValue) { return "" }
        $raw = & wsl.exe wslpath -a -- "$PathValue" 2>$null
        if ($raw) {
            $candidate = [string]($raw | Select-Object -First 1)
            if ($candidate) { return $candidate.Trim() }
        }
        if ($PathValue -match '^[A-Za-z]:\\') {
            $drive = $PathValue.Substring(0, 1).ToLowerInvariant()
            $rest = $PathValue.Substring(2).Replace('\', '/')
            return "/mnt/$drive$rest"
        }
        return ""
    }
    $modeArg = if ($Mock) { "--mock" } else { "--live" }
    if (-not $OllamaBaseUrl) {
        $OllamaBaseUrl = Get-RCOllamaBaseUrl -ProbeReachable
    }
    $wslScript = Convert-ToWslPath -PathValue $script
    if (-not $wslScript) {
        throw "Failed to translate WSL script path for: $script"
    }
    $quotedOllama = $OllamaBaseUrl.Replace("'", "''")
    $ollamaArg = if ($quotedOllama) { "--ollama-base-url '$quotedOllama'" } else { "" }
    if ($IsLinux) {
        & bash -lc "'$wslScript' --port $Port --host $WslHost $modeArg $ollamaArg"
    } else {
        & wsl.exe bash -lc "'$wslScript' --port $Port --host $WslHost $modeArg $ollamaArg"
    }

    try {
        $smoke = rc-mcp-smoke -Port $Port | ConvertFrom-Json -ErrorAction Stop
        if ($smoke -and $smoke.runtime_recommendation -eq "powershell_primary") {
            Write-Warning "WSL bridge is running in degraded mode (Ollama routing mismatch). Defaulting recommendation to PowerShell runtime."
            if ($smoke.PSObject.Properties.Name -contains "fallback_commands" -and $smoke.fallback_commands) {
                Write-Host "Run these commands in PowerShell:" -ForegroundColor Yellow
                Write-Host "  $($smoke.fallback_commands.stop_command)" -ForegroundColor Gray
                Write-Host "  $($smoke.fallback_commands.restart_command)" -ForegroundColor Gray
                Write-Host "  $($smoke.fallback_commands.set_bridge_url)" -ForegroundColor Gray
            }
        }
    } catch {
        Write-Warning "WSL post-start smoke check failed: $($_.Exception.Message)"
    }
}

function rc-ai-stop-wsl {
    <# .SYNOPSIS Stops local WSL/Linux bridge process. #>
    param([int]$Port = 7077)
    $script = Join-Path $repoRoot "tools\stop_ai_bridge_wsl.sh"
    if (-not (Test-Path -LiteralPath $script)) {
        throw "WSL bridge stopper not found: $script"
    }
    if (-not $IsLinux -and -not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) {
        throw "wsl.exe not found; cannot stop WSL bridge runtime from this shell."
    }
    function Convert-ToWslPath {
        param([string]$PathValue)
        if (-not $PathValue) { return "" }
        $raw = & wsl.exe wslpath -a -- "$PathValue" 2>$null
        if ($raw) {
            $candidate = [string]($raw | Select-Object -First 1)
            if ($candidate) { return $candidate.Trim() }
        }
        if ($PathValue -match '^[A-Za-z]:\\') {
            $drive = $PathValue.Substring(0, 1).ToLowerInvariant()
            $rest = $PathValue.Substring(2).Replace('\', '/')
            return "/mnt/$drive$rest"
        }
        return ""
    }
    $wslScript = Convert-ToWslPath -PathValue $script
    if (-not $wslScript) {
        throw "Failed to translate WSL script path for: $script"
    }
    if ($IsLinux) {
        & bash -lc "'$wslScript' --port $Port"
    } else {
        & wsl.exe bash -lc "'$wslScript' --port $Port"
    }
}

function rc-ai-stop {
    <# .SYNOPSIS Stops the Python AI Bridge. #>
    param([int]$Port = 7077)
    Write-Host "Stopping AI Bridge..." -ForegroundColor Cyan
    & "$repoRoot\tools\Stop_Ai_Bridge_Fixed.ps1" -NonInteractive -Port:$Port
}

function rc-ai-stop-admin {
    <# .SYNOPSIS Stops the Python AI Bridge using an elevated PowerShell process. #>
    param([int]$Port = 7077)

    $stopScript = Join-Path $repoRoot "tools\Stop_Ai_Bridge_Fixed.ps1"
    $startScript = Join-Path $repoRoot "tools\Start_Ai_Bridge_Fixed.ps1"
    if (-not (Test-Path -LiteralPath $stopScript)) {
        throw "Stop script not found: $stopScript"
    }
    if (-not (Test-Path -LiteralPath $startScript)) {
        throw "Start script not found: $startScript"
    }

    $manualStop = "powershell -NoProfile -ExecutionPolicy Bypass -File `"$stopScript`" -NonInteractive -Port $Port"
    $manualStart = "powershell -NoProfile -ExecutionPolicy Bypass -File `"$startScript`" -MockMode:`$false -NonInteractive -ForceRestart -Port $Port"

    Write-Host "Stopping AI Bridge with Administrator privileges..." -ForegroundColor Cyan
    $argList = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", "`"$stopScript`"",
        "-NonInteractive",
        "-Port", "$Port"
    )

    $p = Start-Process -FilePath "powershell.exe" -ArgumentList $argList -Verb RunAs -PassThru
    if ($p) {
        Wait-Process -Id $p.Id -ErrorAction SilentlyContinue
    } else {
        throw "Failed to launch elevated PowerShell for rc-ai-stop-admin."
    }

    Start-Sleep -Milliseconds 600
    $stillListening = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
    if ($stillListening) {
        Write-Host "PORT OCCUPIED - manual recovery commands:" -ForegroundColor Yellow
        Write-Host "Stop listener (run in cmd or pwsh):" -ForegroundColor Yellow
        Write-Host "  $manualStop" -ForegroundColor White
        Write-Host "Restart server (run in cmd or pwsh):" -ForegroundColor Yellow
        Write-Host "  $manualStart" -ForegroundColor White
        throw "Elevated stop did not clear port $Port. Ensure UAC prompt is accepted and rerun."
    }

    Write-Host "Elevated stop completed; port $Port is free." -ForegroundColor Green
}

function rc-cfg-ai {
    <# .SYNOPSIS Configures CMake with the AI Bridge enabled. #>
    param([string]$Preset = "dev")
    cmake --preset $Preset -DRC_FEATURE_AI_BRIDGE=ON
}

function Test-RCPipelineV2Enabled {
    $raw = [string]$env:RC_AI_PIPELINE_V2
    if (-not $raw) { return $false }
    return @("1", "true", "yes", "on") -contains $raw.Trim().ToLowerInvariant()
}

function Get-RCBridgeBaseUrl {
    if ($env:RC_AI_BRIDGE_BASE_URL) {
        return [string]$env:RC_AI_BRIDGE_BASE_URL
    }
    $cfgPath = Join-Path $env:RC_ROOT "AI/ai_config.json"
    if (Test-Path -LiteralPath $cfgPath) {
        try {
            $cfg = Get-Content -LiteralPath $cfgPath -Raw | ConvertFrom-Json -ErrorAction Stop
            if ($cfg -and $cfg.bridge_base_url) {
                return [string]$cfg.bridge_base_url
            }
        } catch {
            # fall through to default
        }
    }
    return "http://127.0.0.1:7077"
}

function rc-ai-smoke {
    <#
    .SYNOPSIS
        Runs an AI bridge endpoint smoke test.
    .PARAMETER Live
        Use non-mock mode (requires local Ollama and model availability).
    .PARAMETER Model
        Model passed to /ui_agent and /city_spec requests.
    .PARAMETER Port
        Local toolserver port.
    #>
    param(
        [switch]$Live,
        [string]$Model = "gemma3:4b",
        [int]$Port = 7077
    )

    $toolserverModule = "tools.toolserver:app"
    $bindHost = "127.0.0.1"
    $baseUrl = "http://$bindHost`:$Port"

    if ($Live) {
        Write-Host "AI smoke: LIVE mode (non-mock)." -ForegroundColor Cyan
        $env:ROGUECITY_TOOLSERVER_MOCK = $null
        $ollamaModels = @()
        $ollamaTagsUrl = Get-RCOllamaApiUrl -Path "/api/tags" -ProbeReachable
        try {
            $ollama = Invoke-RestMethod -Uri $ollamaTagsUrl -Method Get -TimeoutSec 5
            $ollamaModels = @($ollama.models | ForEach-Object { $_.name })
            if ($ollamaModels.Count -eq 0) {
                Write-Warning "Ollama is running but no models are listed."
            } elseif (-not ($ollamaModels -contains $Model)) {
                Write-Warning "Model '$Model' not present in Ollama tags."
                $Model = $ollamaModels[0]
                Write-Warning "Falling back to first available model: $Model"
            }
        }
        catch {
            throw "Ollama is not reachable at $ollamaTagsUrl (required for -Live)."
        }
    }
    else {
        Write-Host "AI smoke: MOCK mode." -ForegroundColor Cyan
        $env:ROGUECITY_TOOLSERVER_MOCK = "1"
    }

    $pyLauncher = Get-Command py -ErrorAction SilentlyContinue
    $pythonExe = if ($pyLauncher) { $pyLauncher.Source } else { (Get-Command python -ErrorAction SilentlyContinue).Source }
    if (-not $pythonExe) {
        $pyPath = Join-Path $env:WINDIR "py.exe"
        if (Test-Path -LiteralPath $pyPath) {
            $pythonExe = $pyPath
        }
    }
    if (-not $pythonExe) {
        $localAppData = [Environment]::GetEnvironmentVariable("LOCALAPPDATA")
        foreach ($candidate in @(
            (Join-Path $localAppData "Programs\Python\Python313\python.exe"),
            (Join-Path $localAppData "Programs\Python\Python312\python.exe"),
            (Join-Path $localAppData "Programs\Python\Python311\python.exe")
        )) {
            if ($candidate -and (Test-Path -LiteralPath $candidate)) {
                $pythonExe = $candidate
                break
            }
        }
    }
    if (-not $pythonExe) {
        throw "Python launcher not found for rc-ai-smoke."
    }

    $argList = if ($pythonExe.ToLowerInvariant().EndsWith("py.exe")) {
        @("-3", "-m", "uvicorn", $toolserverModule, "--host", $bindHost, "--port", "$Port")
    } else {
        @("-m", "uvicorn", $toolserverModule, "--host", $bindHost, "--port", "$Port")
    }

    $proc = Start-Process -FilePath $pythonExe -ArgumentList $argList -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 2

    try {
        $health = Invoke-RestMethod -Uri "$baseUrl/health" -Method Get -TimeoutSec 10
        $uiReq = @{
            snapshot = @{ panels = @() }
            goal = "dock tools left"
            model = $Model
        } | ConvertTo-Json -Depth 8
        $uiResp = $null
        $uiError = $null
        try {
            $uiResp = Invoke-RestMethod -Uri "$baseUrl/ui_agent" -Method Post -ContentType "application/json" -Body $uiReq -TimeoutSec 30
        }
        catch {
            $uiError = $_.Exception.Message
        }

        $specReq = @{
            description = "compact walkable town"
            constraints = @{ scale = "town" }
            model = $Model
        } | ConvertTo-Json -Depth 8
        $specResp = $null
        $specError = $null
        try {
            $specResp = Invoke-RestMethod -Uri "$baseUrl/city_spec" -Method Post -ContentType "application/json" -Body $specReq -TimeoutSec 45
        }
        catch {
            $specError = $_.Exception.Message
        }

        $summary = [ordered]@{
            mode = if ($Live) { "live" } else { "mock" }
            model = $Model
            health = $health
            ui = @{
                has_error = [bool]$uiResp.error -or [bool]$uiError
                error = if ($uiResp.error) { $uiResp.error } else { $uiError }
                command_count = if ($uiResp -and $uiResp.commands) { @($uiResp.commands).Count } else { 0 }
                mock = if ($uiResp) { $uiResp.mock } else { $null }
            }
            city_spec = @{
                has_error = [bool]$specResp.error -or [bool]$specError
                error = if ($specResp.error) { $specResp.error } else { $specError }
                has_spec = [bool]$specResp.spec
                district_count = if ($specResp -and $specResp.spec -and $specResp.spec.districts) { @($specResp.spec.districts).Count } else { 0 }
                mock = if ($specResp) { $specResp.mock } else { $null }
            }
        }

        $json = $summary | ConvertTo-Json -Depth 8
        Write-Host "AI smoke result:" -ForegroundColor Green
        Write-Output $json

        if ($summary.ui.has_error -or $summary.city_spec.has_error) {
            throw "AI smoke completed with endpoint errors. See JSON output."
        }
    }
    finally {
        if ($proc -and (Get-Process -Id $proc.Id -ErrorAction SilentlyContinue)) {
            Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        }
    }
}

function rc-ai-query {
    <#
    .SYNOPSIS
        Query local Ollama with RogueCities workspace directives/persona context.
    .PARAMETER Prompt
        User task prompt for the local model.
    .PARAMETER Model
        Ollama model name.
    .PARAMETER File
        Optional extra files to embed into context.
    #>
    param(
        [Parameter(Mandatory = $true)][string]$Prompt,
        [string]$Model = "gemma3:4b",
        [string[]]$File = @(),
        [switch]$NoStrict,
        [switch]$Audit,
        [string[]]$RequiredPaths = @(),
        [switch]$IncludeRepoIndex,
        [string[]]$SearchPattern = @(),
        [int]$MaxSearchResults = 120,
        [int]$MaxRetries = 1,
        [double]$Temperature = 0.10,
        [double]$TopP = 0.85,
        [int]$NumPredict = 256
    )

    function Get-RCAiContextText {
        param([string[]]$ExtraFiles)
        $defaultContextFiles = @(
            ".gemini/GEMINI.md",
            ".agents/Agents.md"
        )
        $contextFiles = @($defaultContextFiles + $ExtraFiles | Select-Object -Unique)
        $contextBlocks = @()
        foreach ($rel in $contextFiles) {
            $abs = Join-Path $env:RC_ROOT $rel
            if (Test-Path -LiteralPath $abs) {
                $text = Get-Content -LiteralPath $abs -Raw -ErrorAction SilentlyContinue
                $text = [regex]::Replace($text, "[\x00-\x08\x0B\x0C\x0E-\x1F]", " ")
                if ($text.Length -gt 8000) { $text = $text.Substring(0, 8000) }
                $contextBlocks += "FILE: $rel`n$text"
            }
        }
        return ($contextBlocks -join "`n`n---`n`n")
    }

    function Invoke-RCOllamaGenerate {
        param(
            [string]$PromptText,
            [string]$ModelName,
            [double]$Temp,
            [double]$TopPValue
        )
        $payloadObj = @{
            model = $ModelName
            prompt = $PromptText
            stream = $false
            options = @{
                temperature = $Temp
                top_p = $TopPValue
            }
        }
        $payload = $payloadObj | ConvertTo-Json -Depth 10 -Compress
        $payloadBytes = [System.Text.Encoding]::UTF8.GetBytes($payload)
        $generateUrl = Get-RCOllamaApiUrl -Path "/api/generate" -ProbeReachable
        $resp = Invoke-RestMethod -Uri $generateUrl -Method Post -ContentType "application/json; charset=utf-8" -Body $payloadBytes -TimeoutSec 120
        if (-not $resp.response) {
            throw "No response from Ollama generate endpoint."
        }
        return [string]$resp.response
    }

    function Get-RCInvalidPaths {
        param([string[]]$Paths)
        $invalid = @()
        foreach ($p in @($Paths)) {
            if (-not $p) { continue }
            $candidate = [string]$p
            if (-not [System.IO.Path]::IsPathRooted($candidate)) {
                $candidate = Join-Path $env:RC_ROOT $candidate
            }
            if (-not (Test-Path -LiteralPath $candidate)) {
                $invalid += $p
            }
        }
        return @($invalid | Select-Object -Unique)
    }

    function Get-RCRepoIndexSnippet {
        try {
            Push-Location $env:RC_ROOT
            $files = @()
            $rg = Get-Command rg -ErrorAction SilentlyContinue
            if ($rg) {
                $files = @(rg --files 2>$null | Select-Object -First 1500)
            } else {
                $files = @(Get-ChildItem -Recurse -File | ForEach-Object { $_.FullName.Substring($env:RC_ROOT.Length + 1) } | Select-Object -First 1500)
            }
            $text = ($files -join "`n")
            if ($text.Length -gt 12000) { $text = $text.Substring(0, 12000) }
            return $text
        } finally {
            Pop-Location
        }
    }

    function Get-RCSearchSnippet {
        param([string[]]$Patterns, [int]$MaxHits)
        if (-not $Patterns -or $Patterns.Count -eq 0) { return "" }
        try {
            Push-Location $env:RC_ROOT
            $hits = @()
            $rg = Get-Command rg -ErrorAction SilentlyContinue
            foreach ($pat in $Patterns) {
                if (-not $pat) { continue }
                if ($rg) {
                    $out = @(rg -n --no-heading --fixed-strings -- "$pat" . 2>$null | Select-Object -First $MaxHits)
                    if ($out.Count -gt 0) {
                        $hits += "PATTERN: $pat"
                        $hits += $out
                    }
                } else {
                    $out = @(
                        Get-ChildItem -Recurse -File |
                            Where-Object { $_.Name -notin @("nul", "con", "prn", "aux", "clock$") } |
                            Select-Object -First 600 |
                            Select-String -SimpleMatch $pat -ErrorAction SilentlyContinue |
                            Select-Object -First $MaxHits |
                            ForEach-Object { "$($_.Path):$($_.LineNumber):$($_.Line)" }
                    )
                    if ($out.Count -gt 0) {
                        $hits += "PATTERN: $pat"
                        $hits += $out
                    }
                }
            }
            $text = ($hits -join "`n")
            if ($text.Length -gt 12000) { $text = $text.Substring(0, 12000) }
            return $text
        } finally {
            Pop-Location
        }
    }

    function Get-RCFirstJsonObjectString {
        param([string]$Text)
        if (-not $Text) { return $null }
        $start = $Text.IndexOf("{")
        if ($start -lt 0) { return $null }
        $depth = 0
        $inString = $false
        $escape = $false
        for ($i = $start; $i -lt $Text.Length; $i++) {
            $ch = $Text[$i]
            if ($escape) {
                $escape = $false
                continue
            }
            if ($ch -eq "\") {
                if ($inString) { $escape = $true }
                continue
            }
            if ($ch -eq '"') {
                $inString = -not $inString
                continue
            }
            if (-not $inString) {
                if ($ch -eq "{") { $depth++ }
                elseif ($ch -eq "}") {
                    $depth--
                    if ($depth -eq 0) {
                        return $Text.Substring($start, $i - $start + 1)
                    }
                }
            }
        }
        return $null
    }

    function Convert-RCResponseToJsonObject {
        param([string]$Text)
        $candidates = @()
        $raw = [string]$Text
        if ($raw) { $candidates += $raw.Trim() }
        if ($raw -match '```(?:json)?\s*([\s\S]*?)```') {
            $candidates += $matches[1].Trim()
        }
        $firstObj = Get-RCFirstJsonObjectString -Text $raw
        if ($firstObj) { $candidates += $firstObj.Trim() }

        foreach ($candidate in @($candidates | Select-Object -Unique)) {
            if (-not $candidate) { continue }
            try {
                return ($candidate | ConvertFrom-Json -ErrorAction Stop)
            } catch {
                continue
            }
        }
        return $null
    }

    $Strict = -not $NoStrict

    if (Test-RCPipelineV2Enabled) {
        $baseUrl = Get-RCBridgeBaseUrl
        $mode = if ($Audit) { "audit" } else { "normal" }
        $requestBody = [ordered]@{
            prompt = $Prompt
            mode = $mode
            context_files = @($File | Where-Object { $_ } | Select-Object -Unique)
            required_paths = @($RequiredPaths | Where-Object { $_ } | Select-Object -Unique)
            search_hints = @($SearchPattern | Where-Object { $_ } | Select-Object -Unique)
            include_repo_index = [bool]$IncludeRepoIndex
            include_semantic = $false
            embedding_dimensions = 512
            max_search_results = [Math]::Min([Math]::Max($MaxSearchResults, 1), 120)
            max_context_chars = 24000
            debug_trace = $false
            temperature = $Temperature
            top_p = $TopP
            num_predict = [Math]::Max($NumPredict, 64)
        }
        if ($PSBoundParameters.ContainsKey("Model") -and $Model) {
            $requestBody.model_overrides = @{
                triage_model = $Model
                synth_fast_model = $Model
                synth_escalation_model = $Model
            }
        }

        $payload = $requestBody | ConvertTo-Json -Depth 12
        try {
            $resp = Invoke-RestMethod -Uri "$baseUrl/pipeline/query" -Method Post -ContentType "application/json" -Body $payload -TimeoutSec 120
            if ($Strict -and $resp -and $resp.verifier) {
                $invalid = @($resp.verifier.invalid_paths)
                $missingRequired = @($resp.verifier.missing_required_paths)
                $answerQualityOk = $true
                $answerQualityReason = "ok"
                if ($resp.verifier.PSObject.Properties.Name -contains "answer_quality_ok") {
                    $answerQualityOk = [bool]$resp.verifier.answer_quality_ok
                }
                if ($resp.verifier.PSObject.Properties.Name -contains "answer_quality_reason") {
                    $answerQualityReason = [string]$resp.verifier.answer_quality_reason
                }
                if ($invalid.Count -gt 0) {
                    Write-Warning "rc-ai-query strict mode: response contains invalid paths: $($invalid -join ', ')"
                }
                if ($missingRequired.Count -gt 0) {
                    Write-Warning "rc-ai-query strict mode: response missing required paths: $($missingRequired -join ', ')"
                }
                if (-not $answerQualityOk) {
                    $msg = "rc-ai-query strict mode: response answer quality failed: $answerQualityReason"
                    Write-Warning $msg
                    throw $msg
                }
                if ([string]::IsNullOrWhiteSpace([string]$resp.answer)) {
                    $msg = "rc-ai-query strict mode: empty answer received."
                    Write-Warning $msg
                    throw $msg
                }
            }
            Write-Output ($resp | ConvertTo-Json -Depth 12)
            return
        } catch {
            $detail = $_.Exception.Message
            if ($_.ErrorDetails -and $_.ErrorDetails.Message) {
                $detail = $_.ErrorDetails.Message
            }
            if ($detail -match '"detail"\s*:\s*"Not Found"' -or $detail -match '404') {
                $manualStart = "powershell -NoProfile -ExecutionPolicy Bypass -File `"$env:RC_ROOT\tools\Start_Ai_Bridge_Fixed.ps1`" -MockMode:`$false -NonInteractive -ForceRestart -Port 7077"
                throw "pipeline/query not found at $baseUrl. A legacy bridge process is likely running. Restart bridge in the owning/elevated shell.`n$manualStart"
            }
            throw "pipeline/query request failed at $baseUrl. $detail"
        }
    }

    $contextText = Get-RCAiContextText -ExtraFiles $File
    $fileManifest = @($File | Where-Object { $_ } | Select-Object -Unique)
    $fileManifestText = if ($fileManifest.Count -gt 0) { ($fileManifest -join "`n") } else { "(none provided)" }
    $requiredManifest = @($RequiredPaths | Where-Object { $_ } | Select-Object -Unique)
    $requiredManifestText = if ($requiredManifest.Count -gt 0) { ($requiredManifest -join "`n") } else { "(none required)" }
    $repoIndexText = if ($IncludeRepoIndex) { Get-RCRepoIndexSnippet } else { "" }
    $searchText = Get-RCSearchSnippet -Patterns $SearchPattern -MaxHits $MaxSearchResults
    $system = @"
You are the local RogueCities assistant. Follow workspace mandates and prioritize:
- architectural boundary compliance
- deterministic behavior
- pragmatic, testable code changes
- concise actionable output
Use provided context files as authoritative instructions for this workspace.
"@

    $strictContract = @"
STRICT OUTPUT CONTRACT:
- Return ONLY valid JSON object.
- Schema:
  {
    "answer": "string",
    "paths_used": ["repo/relative/path.ext"],
    "insufficient_context": false,
    "confidence": "high|medium|low"
  }
- Include only paths that exist in provided context.
- If unsure, set insufficient_context=true and keep paths_used empty.
- If REQUIRED PATHS are listed, include all of them in paths_used unless truly impossible.
"@

    $basePrompt = @"
SYSTEM:
$system

WORKSPACE CONTEXT:
$contextText

CONTEXT FILE MANIFEST (prefer these exact repo-relative paths):
$fileManifestText

REQUIRED PATHS (must include in paths_used when relevant):
$requiredManifestText

REPO INDEX (from rg --files):
$repoIndexText

SEARCH HITS (from rg -n):
$searchText

USER REQUEST:
$Prompt
"@

    $fullPrompt = if ($Strict) { "$basePrompt`n`n$strictContract" } else { $basePrompt }

    $attempt = 0
    $rawResponse = ""
    $lastInvalid = @()
    $lastMissingRequired = @()
    $parsed = $null

    while ($attempt -le $MaxRetries) {
        $attempt++
        $rawResponse = Invoke-RCOllamaGenerate -PromptText $fullPrompt -ModelName $Model -Temp $Temperature -TopPValue $TopP

        if (-not $Strict) {
            break
        }

        $parsed = Convert-RCResponseToJsonObject -Text $rawResponse
        if (-not $parsed) {
            $lastInvalid = @("__INVALID_JSON__")
        }

        if ($parsed) {
            $pathsUsed = @()
            if ($parsed.PSObject.Properties.Name -contains "paths_used") {
                $pathsUsed = @($parsed.paths_used | ForEach-Object { [string]$_ })
            }
            $lastInvalid = Get-RCInvalidPaths -Paths $pathsUsed
            $lastMissingRequired = @()
            foreach ($req in $requiredManifest) {
                if ($pathsUsed -notcontains [string]$req) {
                    $lastMissingRequired += [string]$req
                }
            }
            if ($lastInvalid.Count -eq 0 -and $lastMissingRequired.Count -eq 0) {
                break
            }
        }

        if ($attempt -le $MaxRetries) {
            $retryReason = if ($lastInvalid -contains "__INVALID_JSON__") {
                "Your previous output was not valid JSON."
            } elseif ($lastInvalid.Count -gt 0) {
                "Your previous output referenced non-existent paths: $($lastInvalid -join ', ')."
            } elseif ($lastMissingRequired.Count -gt 0) {
                "Your previous output did not include required paths: $($lastMissingRequired -join ', ')."
            } else {
                "Your previous output did not satisfy strict contract."
            }
            $fullPrompt = @"
$basePrompt

$strictContract

CORRECTION:
$retryReason
Return corrected JSON now.
"@
        }
    }

    if ($Strict) {
        if (-not $parsed) {
            Write-Warning "rc-ai-query strict mode: model did not return valid JSON."
            Write-Output $rawResponse
            return
        }
        if ($lastInvalid.Count -gt 0) {
            Write-Warning "rc-ai-query strict mode: response still contains invalid paths: $($lastInvalid -join ', ')"
        }
        if ($lastMissingRequired.Count -gt 0) {
            Write-Warning "rc-ai-query strict mode: response missing required paths: $($lastMissingRequired -join ', ')"
        }
        Write-Output ($parsed | ConvertTo-Json -Depth 10)
        return
    }

    Write-Output $rawResponse
}

function rc-ai-eval {
    <#
    .SYNOPSIS
        Evaluates local AI grounding/compliance using benchmark prompts.
    #>
    param(
        [string]$CaseFile = "tools/ai_eval_cases.json",
        [string]$Model = "gemma3:4b",
        [int]$MaxCases = 10,
        [switch]$IncludeRepoIndex,
        [switch]$Audit,
        [double]$Temperature = 0.10,
        [double]$TopP = 0.85,
        [int]$NumPredict = 256
    )

    $casePath = if ([System.IO.Path]::IsPathRooted($CaseFile)) { $CaseFile } else { Join-Path $env:RC_ROOT $CaseFile }
    if (-not (Test-Path -LiteralPath $casePath)) {
        throw "Case file not found: $casePath"
    }

    if (Test-RCPipelineV2Enabled) {
        $baseUrl = Get-RCBridgeBaseUrl
        $mode = if ($Audit) { "audit" } else { "normal" }
        $requestBody = [ordered]@{
            case_file = $CaseFile
            max_cases = [Math]::Max($MaxCases, 1)
            mode = $mode
            include_repo_index = [bool]$IncludeRepoIndex
            include_semantic = $false
            embedding_dimensions = 512
            temperature = $Temperature
            top_p = $TopP
            num_predict = [Math]::Max($NumPredict, 64)
        }
        if ($PSBoundParameters.ContainsKey("Model") -and $Model) {
            $requestBody.model_overrides = @{
                triage_model = $Model
                synth_fast_model = $Model
                synth_escalation_model = $Model
            }
        }
        $payload = $requestBody | ConvertTo-Json -Depth 12
        try {
            $resp = Invoke-RestMethod -Uri "$baseUrl/pipeline/eval" -Method Post -ContentType "application/json" -Body $payload -TimeoutSec 300
            Write-Output ($resp | ConvertTo-Json -Depth 12)
            return
        } catch {
            $detail = $_.Exception.Message
            if ($_.ErrorDetails -and $_.ErrorDetails.Message) {
                $detail = $_.ErrorDetails.Message
            }
            if ($detail -match '"detail"\s*:\s*"Not Found"' -or $detail -match '404') {
                $manualStart = "powershell -NoProfile -ExecutionPolicy Bypass -File `"$env:RC_ROOT\tools\Start_Ai_Bridge_Fixed.ps1`" -MockMode:`$false -NonInteractive -ForceRestart -Port 7077"
                throw "pipeline/eval not found at $baseUrl. A legacy bridge process is likely running. Restart bridge in the owning/elevated shell.`n$manualStart"
            }
            throw "pipeline/eval request failed at $baseUrl. $detail"
        }
    }

    $cases = Get-Content -LiteralPath $casePath -Raw | ConvertFrom-Json
    $subset = @($cases | Select-Object -First $MaxCases)
    $results = @()

    foreach ($case in $subset) {
        $respJsonText = rc-ai-query -Prompt $case.prompt -Model $Model -File @($case.files) -RequiredPaths @($case.required_paths) -IncludeRepoIndex:$IncludeRepoIndex -SearchPattern @($case.search_patterns) -MaxSearchResults 20 -MaxRetries 2 -Temperature 0.10 -TopP 0.85 -NumPredict 256
        $parsed = $null
        $jsonOk = $false
        try {
            $parsed = $respJsonText | ConvertFrom-Json -ErrorAction Stop
            $jsonOk = $true
        } catch {
            $jsonOk = $false
        }

        $paths = @()
        $invalid = @()
        $requiredMissing = @()
        $insufficient = $null
        $answerQualityOk = $null
        $answerQualityReason = ""
        if ($jsonOk -and $parsed) {
            if ($parsed.PSObject.Properties.Name -contains "paths_used") {
                $paths = @($parsed.paths_used | ForEach-Object { [string]$_ })
            }
            foreach ($p in $paths) {
                $abs = if ([System.IO.Path]::IsPathRooted($p)) { $p } else { Join-Path $env:RC_ROOT $p }
                if (-not (Test-Path -LiteralPath $abs)) {
                    $invalid += $p
                }
            }
            if ($parsed.PSObject.Properties.Name -contains "insufficient_context") {
                $insufficient = [bool]$parsed.insufficient_context
            }
            if ($parsed.PSObject.Properties.Name -contains "verifier" -and $parsed.verifier) {
                if ($parsed.verifier.PSObject.Properties.Name -contains "answer_quality_ok") {
                    $answerQualityOk = [bool]$parsed.verifier.answer_quality_ok
                }
                if ($parsed.verifier.PSObject.Properties.Name -contains "answer_quality_reason") {
                    $answerQualityReason = [string]$parsed.verifier.answer_quality_reason
                }
            }
        }

        foreach ($req in @($case.required_paths)) {
            if ($paths -notcontains [string]$req) {
                $requiredMissing += [string]$req
            }
        }

        $results += [ordered]@{
            id = [string]$case.id
            json_ok = $jsonOk
            insufficient_context = $insufficient
            path_count = @($paths).Count
            invalid_path_count = @($invalid).Count
            required_missing_count = @($requiredMissing).Count
            answer_quality_ok = $answerQualityOk
            answer_quality_reason = $answerQualityReason
            invalid_paths = @($invalid)
            required_missing = @($requiredMissing)
        }
    }

    $summary = [ordered]@{
        case_file = $CaseFile
        model = $Model
        total_cases = @($results).Count
        json_ok_cases = @($results | Where-Object { $_.json_ok }).Count
        cases_with_invalid_paths = @($results | Where-Object { $_.invalid_path_count -gt 0 }).Count
        cases_missing_required_paths = @($results | Where-Object { $_.required_missing_count -gt 0 }).Count
        cases_answer_quality_failed = @($results | Where-Object { $_.answer_quality_ok -eq $false }).Count
        results = $results
    }

    Write-Output ($summary | ConvertTo-Json -Depth 8)
}

function rc-perceive-ui {
    <#
    .SYNOPSIS
        Observes runtime UI state via perception pipeline (snapshot + optional vision/OCR).
    #>
    param(
        [ValidateSet("quick", "full")][string]$Mode = "quick",
        [int]$Frames = 5,
        [switch]$Screenshot,
        [bool]$IncludeVision = $true,
        [bool]$IncludeOcr = $true,
        [bool]$StrictContract = $true,
        [string]$MockupPath = "visualizer/RC_UI_Mockup.html",
        [string[]]$ScreenshotPath = @(),
        [int]$VisionTimeoutSec = 45,
        [int]$OcrTimeoutSec = 45
    )

    $baseUrl = Get-RCBridgeBaseUrl
    $requestBody = [ordered]@{
        mode = $Mode
        frames = [Math]::Max($Frames, 1)
        screenshot = [bool]$Screenshot
        screenshot_paths = @($ScreenshotPath | Where-Object { $_ } | Select-Object -Unique)
        include_vision = $IncludeVision
        include_ocr = $IncludeOcr
        strict_contract = $StrictContract
        mockup_path = $MockupPath
        vision_timeout_sec = [Math]::Max($VisionTimeoutSec, 5)
        ocr_timeout_sec = [Math]::Max($OcrTimeoutSec, 5)
    }
    $payload = $requestBody | ConvertTo-Json -Depth 12
    try {
        $resp = Invoke-RestMethod -Uri "$baseUrl/perception/observe" -Method Post -ContentType "application/json" -Body $payload -TimeoutSec 300
        Write-Output ($resp | ConvertTo-Json -Depth 14)
    } catch {
        $detail = $_.Exception.Message
        if ($_.ErrorDetails -and $_.ErrorDetails.Message) {
            $detail = $_.ErrorDetails.Message
        }
        throw "perception/observe request failed at $baseUrl. $detail"
    }
}

function rc-perception-audit {
    <#
    .SYNOPSIS
        Runs repeated perception observations and reports section/latency metrics.
    #>
    param(
        [int]$Observations = 3,
        [switch]$IncludeReports,
        [ValidateSet("quick", "full")][string]$Mode = "quick",
        [int]$Frames = 5,
        [switch]$Screenshot,
        [bool]$IncludeVision = $true,
        [bool]$IncludeOcr = $true,
        [bool]$StrictContract = $true,
        [string]$MockupPath = "visualizer/RC_UI_Mockup.html",
        [int]$LatencyGateP95Ms = 8000
    )

    $baseUrl = Get-RCBridgeBaseUrl
    $requestBody = [ordered]@{
        observations = [Math]::Min([Math]::Max($Observations, 1), 20)
        include_reports = [bool]$IncludeReports
        mode = $Mode
        frames = [Math]::Max($Frames, 1)
        screenshot = [bool]$Screenshot
        include_vision = $IncludeVision
        include_ocr = $IncludeOcr
        strict_contract = $StrictContract
        mockup_path = $MockupPath
        latency_gate_p95_ms = [Math]::Max($LatencyGateP95Ms, 1000)
    }
    $payload = $requestBody | ConvertTo-Json -Depth 12
    try {
        $resp = Invoke-RestMethod -Uri "$baseUrl/perception/audit" -Method Post -ContentType "application/json" -Body $payload -TimeoutSec 300
        Write-Output ($resp | ConvertTo-Json -Depth 14)
    } catch {
        $detail = $_.Exception.Message
        if ($_.ErrorDetails -and $_.ErrorDetails.Message) {
            $detail = $_.ErrorDetails.Message
        }
        throw "perception/audit request failed at $baseUrl. $detail"
    }
}

function rc-full-smoke {
    <#
    .SYNOPSIS
        Runs commit-gate full smoke/audit script (PowerShell-primary) one or more times.
    #>
    param(
        [int]$Port = 7222,
        [int]$Runs = 2
    )

    $scriptPath = Join-Path $env:RC_ROOT "tools\.run\rc_full_smoke.ps1"
    if (-not (Test-Path -LiteralPath $scriptPath)) {
        throw "Full smoke script not found: $scriptPath"
    }
    $runCount = [Math]::Max($Runs, 1)
    for ($i = 1; $i -le $runCount; $i++) {
        Write-Host "Running full smoke gate ($i/$runCount) on port $Port..." -ForegroundColor Cyan
        & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath -Port $Port
        if ($LASTEXITCODE -ne 0) {
            throw "Full smoke gate failed on run $i (exit $LASTEXITCODE)."
        }
    }
}

Write-Host ""
Write-Host "    ____  ____  ____  __  __  ____    ___  ____  ____  _  _    ____  ____  ___  ____  ____  __ _  ____  ____ " -ForegroundColor Red
Write-Host "   (  _ \(  _ \(  __)(  )(  )(  __)  / __)(_  _)(_  _)( \/ )  (  _ \(  __)/ __)(_  _)(  _ \(  ( \(  __)(  _ \" -ForegroundColor Red
Write-Host "    )   / )(_) ))_)   )(__)(  ) _)  ( (__  _)(_   )(   \  /    )(_) ))_) \__ \  _)(_  ) _ (/    / ) _)  )   /" -ForegroundColor Red
Write-Host "   (_)\_)(____/(____)(______)(____)  \___)(____) (__)  (__)   (____/(____)(___/(____)(____/\_)__)(____)(_)\_)" -ForegroundColor Red
Write-Host ""
Write-Host "   [:: HYPER-REACTIVE DEV CONSOLE ONLINE ::]" -ForegroundColor Yellow
Write-Host "   [:: Dev Shell Loaded. Type 'rc-help' for commands or 'rc-console' to start TUI. ::]" -ForegroundColor DarkYellow
Write-Host ""
