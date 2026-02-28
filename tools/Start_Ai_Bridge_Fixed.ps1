# Start_Ai_Bridge_Fixed.ps1
# Fixed version that properly starts the repository toolserver in mock mode

param(
    [switch]$MockMode = $true,
    [switch]$NoTunnel = $true,
    [switch]$NonInteractive,
    [switch]$ForceRestart,
    [switch]$EnableReload,
    [int]$Port = 7077
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  RogueCity AI Bridge - Start" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Configuration
$RepoRoot = Split-Path -Parent $PSScriptRoot
$ToolServerPath = Join-Path $RepoRoot "tools\toolserver.py"
$PidFile = Join-Path $PSScriptRoot ".run\toolserver.pid"
$PythonExe = $null
$PythonArgsPrefix = @()

function Test-PythonCommand {
    param([string[]]$Args)
    try {
        $process = Start-Process -FilePath $PythonExe -ArgumentList @($PythonArgsPrefix + $Args) -PassThru -WindowStyle Hidden
        $deadline = (Get-Date).AddSeconds(20)
        while (-not $process.HasExited -and (Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 200
            $process.Refresh()
        }
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            return $false
        }
        return ($process.ExitCode -eq 0)
    } catch {
        return $false
    }
}

function Test-BridgeHealth {
    param([int]$HealthPort)
    try {
        $resp = Invoke-WebRequest -Uri "http://127.0.0.1:$HealthPort/health" -UseBasicParsing -TimeoutSec 2 -ErrorAction Stop
        return ($resp.StatusCode -eq 200)
    } catch {
        return $false
    }
}

function Get-BridgeHealth {
    param([int]$HealthPort)
    try {
        $resp = Invoke-WebRequest -Uri "http://127.0.0.1:$HealthPort/health" -UseBasicParsing -TimeoutSec 2 -ErrorAction Stop
        if ($resp.StatusCode -ne 200 -or -not $resp.Content) {
            return $null
        }
        return ($resp.Content | ConvertFrom-Json)
    } catch {
        return $null
    }
}

function Get-BridgeMode {
    param([int]$ProbePort)

    $health = Get-BridgeHealth -HealthPort $ProbePort
    if ($health -ne $null -and $health.PSObject.Properties.Name -contains "mock") {
        if ($health.mock -eq $true) { return "mock" }
        if ($health.mock -eq $false) { return "live" }
    }

    # Back-compat probe for older bridge builds without health.mock:
    # /ui_agent mock path emits "mock: true"; live path omits/clears it.
    try {
        $probeReq = @{
            snapshot = @{ panels = @() }
            goal = "mode-probe"
            model = "deepseek-coder-v2:16b"
        } | ConvertTo-Json -Depth 6
        $probeResp = Invoke-RestMethod -Uri "http://127.0.0.1:$ProbePort/ui_agent" -Method Post -ContentType "application/json" -Body $probeReq -TimeoutSec 4
        if ($probeResp -and ($probeResp.PSObject.Properties.Name -contains "mock") -and $probeResp.mock -eq $true) {
            return "mock"
        }
        return "live"
    } catch {
        return "unknown"
    }
}

function Write-ManualRecoveryCommands {
    param([int]$RecoveryPort)
    $stopCmd = "powershell -NoProfile -ExecutionPolicy Bypass -File `"$PSScriptRoot\Stop_Ai_Bridge_Fixed.ps1`" -NonInteractive -Port $RecoveryPort"
    $startCmd = "powershell -NoProfile -ExecutionPolicy Bypass -File `"$PSScriptRoot\Start_Ai_Bridge_Fixed.ps1`" -MockMode:`$false -NonInteractive -ForceRestart -Port $RecoveryPort"
    Write-Host ""
    Write-Host "PORT OCCUPIED - manual recovery commands:" -ForegroundColor Yellow
    Write-Host "Stop listener (run in cmd or pwsh):" -ForegroundColor Yellow
    Write-Host "  $stopCmd" -ForegroundColor White
    Write-Host "Restart server (run in cmd or pwsh):" -ForegroundColor Yellow
    Write-Host "  $startCmd" -ForegroundColor White
}

# Create .run directory
$RunDir = Join-Path $PSScriptRoot ".run"
if (-not (Test-Path $RunDir)) {
    New-Item -ItemType Directory -Path $RunDir | Out-Null
}

# Check if toolserver exists
if (-not (Test-Path $ToolServerPath)) {
    Write-Host "? ERROR: toolserver.py not found at: $ToolServerPath" -ForegroundColor Red
    Write-Host "Please ensure you're running from the correct directory" -ForegroundColor Yellow
    if (-not $NonInteractive) { pause }
    exit 1
}
Write-Host "? Found toolserver.py" -ForegroundColor Green

# Resolve Python launcher (py/python/python3 plus common install paths)
$launcherCandidates = @()
$venvPython = Join-Path $RepoRoot ".venv\Scripts\python.exe"
if (Test-Path -LiteralPath $venvPython) {
    $launcherCandidates += @{ exe = $venvPython; argsPrefix = @() }
}
$cmdPy = Get-Command py -ErrorAction SilentlyContinue
if ($cmdPy) {
    $launcherCandidates += @{ exe = $cmdPy.Source; argsPrefix = @("-3") }
}
$cmdPython = Get-Command python -ErrorAction SilentlyContinue
if ($cmdPython) {
    $launcherCandidates += @{ exe = $cmdPython.Source; argsPrefix = @() }
}
$cmdPython3 = Get-Command python3 -ErrorAction SilentlyContinue
if ($cmdPython3) {
    $launcherCandidates += @{ exe = $cmdPython3.Source; argsPrefix = @() }
}

$winDir = [Environment]::GetEnvironmentVariable("WINDIR")
$localAppData = [Environment]::GetEnvironmentVariable("LOCALAPPDATA")
$pathCandidates = @(
    (Join-Path $localAppData "Programs\Python\Python313\python.exe"),
    (Join-Path $localAppData "Programs\Python\Python312\python.exe"),
    (Join-Path $localAppData "Programs\Python\Python311\python.exe"),
    (Join-Path $localAppData "Programs\Python\Python310\python.exe"),
    (Join-Path $winDir "py.exe")
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

foreach ($candidate in $pathCandidates) {
    if ($candidate.ToLowerInvariant().EndsWith("py.exe")) {
        $launcherCandidates += @{ exe = $candidate; argsPrefix = @("-3") }
    } else {
        $launcherCandidates += @{ exe = $candidate; argsPrefix = @() }
    }
}

foreach ($launcher in $launcherCandidates) {
    if (Test-Path -LiteralPath $launcher.exe) {
        $PythonExe = $launcher.exe
        $PythonArgsPrefix = @($launcher.argsPrefix)
        Write-Host "? Python found: $PythonExe" -ForegroundColor Green
        break
    }
}

if (-not $PythonExe) {
    Write-Host "? ERROR: Python not found" -ForegroundColor Red
    Write-Host "Please install Python 3.10+ and ensure py/python is available." -ForegroundColor Yellow
    if (-not $NonInteractive) { pause }
    exit 1
}

# Check dependencies
Write-Host "Checking Python packages..." -ForegroundColor Yellow
$packages = @("fastapi", "uvicorn", "httpx", "pydantic")
$missing = @()

foreach ($pkg in $packages) {
    if (-not (Test-PythonCommand -Args @("-c", "import $pkg"))) {
        $missing += $pkg
    }
}

if ($missing.Count -gt 0) {
    Write-Host "? Package probe reported missing modules: $($missing -join ', ')" -ForegroundColor Yellow
    Write-Host "  Continuing; startup health check will validate runtime dependencies." -ForegroundColor Yellow
    Write-Host "  If startup fails, install with: pip install $($missing -join ' ')" -ForegroundColor Gray
} else {
    Write-Host "? All packages installed" -ForegroundColor Green
}

# Check if port is in use
$portInUse = $null
$netstatCmd = Get-Command netstat -ErrorAction SilentlyContinue
if ($netstatCmd) {
    $portInUse = netstat -ano | Select-String ":$Port" | Select-String "LISTENING"
} else {
    $portInUse = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
}
if ($portInUse) {
    Write-Host "? Port $Port is already in use." -ForegroundColor Yellow
    if ($ForceRestart) {
        Write-Host "ForceRestart enabled: stopping existing listener before relaunch..." -ForegroundColor Yellow
        $stopScript = Join-Path $PSScriptRoot "Stop_Ai_Bridge_Fixed.ps1"
        if (Test-Path -LiteralPath $stopScript) {
            & $stopScript -NonInteractive
            Start-Sleep -Milliseconds 500
        }

        $remaining = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
        if ($remaining) {
            $health = Get-BridgeHealth -HealthPort $Port
            if ($health -ne $null -and $health.status -eq "ok") {
                $existingMode = Get-BridgeMode -ProbePort $Port
                if ($MockMode) {
                    if ($existingMode -eq "mock") {
                        Write-Host "? Existing bridge on port $Port is healthy and already in mock mode; reusing instance." -ForegroundColor Green
                        exit 0
                    }
                    Write-Host "? ERROR: Existing bridge is healthy but not in mock mode, and termination was blocked." -ForegroundColor Red
                    exit 1
                }
                if ($existingMode -eq "live") {
                    Write-Host "? Existing bridge is healthy and already in live mode; reusing current instance." -ForegroundColor Green
                    exit 0
                }
                Write-Host "? ERROR: Existing bridge on port $Port is healthy but could not be terminated." -ForegroundColor Red
                Write-Host "Refusing to reuse unknown-mode process for live workflow (could still be mock)." -ForegroundColor Yellow
                Write-Host "Run from the owning/elevated shell and retry: rc-ai-stop ; rc-ai-start" -ForegroundColor Yellow
                Write-ManualRecoveryCommands -RecoveryPort $Port
                exit 1
            }
            Write-Host "? ERROR: Port $Port is still blocked after stop attempt." -ForegroundColor Red
            Write-Host "Try running stop/start in the same elevated shell or close the owning terminal session." -ForegroundColor Yellow
            Write-ManualRecoveryCommands -RecoveryPort $Port
            exit 1
        }
    } else {
        if (Test-BridgeHealth -HealthPort $Port) {
            Write-Host "? Bridge already healthy on port $Port; reusing existing instance." -ForegroundColor Green
            exit 0
        }
        Write-Host "Existing listener is not healthy. Re-run with -ForceRestart to recycle it." -ForegroundColor Yellow
        Write-ManualRecoveryCommands -RecoveryPort $Port
        exit 1
    }
}

# Set mock mode if requested
if ($MockMode) {
    $env:ROGUECITY_TOOLSERVER_MOCK = "1"
    Write-Host "? Mock mode enabled (no Ollama required)" -ForegroundColor Green
} else {
    $env:ROGUECITY_TOOLSERVER_MOCK = $null
    Write-Host "? Live mode enabled (requires Ollama)" -ForegroundColor Green
    
    # Check Ollama if live mode
    try {
        $ollamaTest = Invoke-WebRequest -Uri "http://127.0.0.1:11434/api/tags" -UseBasicParsing -TimeoutSec 2 -ErrorAction Stop
        Write-Host "? Ollama detected" -ForegroundColor Green
    } catch {
        Write-Host "? ERROR: Ollama not running" -ForegroundColor Red
        Write-Host "Start Ollama first with: ollama serve" -ForegroundColor Yellow
        if (-not $NonInteractive) { pause }
        exit 1
    }
}

Write-Host ""
Write-Host "Starting toolserver..." -ForegroundColor Yellow
Write-Host "  Path: $ToolServerPath" -ForegroundColor Gray
Write-Host "  Port: $Port" -ForegroundColor Gray
Write-Host "  Mode: $(if ($MockMode) { 'Mock' } else { 'Live' })" -ForegroundColor Gray
Write-Host "  Reload: $(if ($EnableReload) { 'On' } else { 'Off' })" -ForegroundColor Gray
Write-Host ""

# Change to repo root for proper module imports
Push-Location $RepoRoot

try {
    # Start uvicorn in background
    $uvicornArgs = @($PythonArgsPrefix + @(
        "-m", "uvicorn",
        "tools.toolserver:app",
        "--host", "127.0.0.1",
        "--port", "$Port"
    ))
    if ($EnableReload) {
        $uvicornArgs += "--reload"
    }
    $process = Start-Process $PythonExe -ArgumentList $uvicornArgs -PassThru -WindowStyle Hidden
    
    # Save PID
    $process.Id | Out-File -FilePath $PidFile -Encoding ASCII
    
    Write-Host "? Toolserver started (PID: $($process.Id))" -ForegroundColor Green
    
    # Wait for health check
    Write-Host "Waiting for health check..." -ForegroundColor Yellow
    
    $maxAttempts = 30
    $attempt = 0
    $healthy = $false
    
    while ($attempt -lt $maxAttempts -and -not $healthy) {
        Start-Sleep -Milliseconds 500
        try {
            $response = Invoke-WebRequest -Uri "http://127.0.0.1:$Port/health" -UseBasicParsing -TimeoutSec 2 -ErrorAction Stop
            if ($response.StatusCode -eq 200) {
                $healthy = $true
                Write-Host "? Health check passed!" -ForegroundColor Green
            }
        } catch {
            $attempt++
            Write-Host "." -NoNewline -ForegroundColor Gray
        }
    }
    
    if (-not $healthy) {
        Write-Host ""
        Write-Host "? Health check failed after $maxAttempts attempts" -ForegroundColor Red
        Write-Host "Check toolserver logs for errors" -ForegroundColor Yellow
        
        # Kill the process
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        exit 1
    }
    
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "  AI Bridge Running Successfully!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Endpoints available:" -ForegroundColor Yellow
    Write-Host "  GET  http://127.0.0.1:$Port/health" -ForegroundColor White
    Write-Host "  POST http://127.0.0.1:$Port/ui_agent" -ForegroundColor White
    Write-Host "  POST http://127.0.0.1:$Port/city_spec" -ForegroundColor White
    Write-Host "  POST http://127.0.0.1:$Port/ui_design_assistant" -ForegroundColor White
    Write-Host ""
    Write-Host "The toolserver is running in the background." -ForegroundColor Cyan
    Write-Host "Run Stop_Ai_Bridge.ps1 to stop it." -ForegroundColor Cyan
    Write-Host ""
    
} finally {
    Pop-Location
}

if (-not $NonInteractive) {
    Write-Host "Press any key to exit..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
}

# For automation/scripting: Allow Ctrl+C or closing console to exit gracefully
exit 0
