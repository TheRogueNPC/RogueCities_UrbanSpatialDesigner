# Start_Ai_Bridge_Fixed.ps1
# Fixed version that properly starts the repository toolserver in mock mode

param(
    [switch]$MockMode = $true,
    [switch]$NoTunnel = $true
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  RogueCity AI Bridge - Start" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Configuration
$Port = 7077
$RepoRoot = Split-Path -Parent $PSScriptRoot
$ToolServerPath = Join-Path $RepoRoot "tools\toolserver.py"
$PidFile = Join-Path $PSScriptRoot ".run\toolserver.pid"

# Create .run directory
$RunDir = Join-Path $PSScriptRoot ".run"
if (-not (Test-Path $RunDir)) {
    New-Item -ItemType Directory -Path $RunDir | Out-Null
}

# Check if toolserver exists
if (-not (Test-Path $ToolServerPath)) {
    Write-Host "? ERROR: toolserver.py not found at: $ToolServerPath" -ForegroundColor Red
    Write-Host "Please ensure you're running from the correct directory" -ForegroundColor Yellow
    pause
    exit 1
}
Write-Host "? Found toolserver.py" -ForegroundColor Green

# Check Python
try {
    $pythonVersion = python --version 2>&1
    Write-Host "? Python found: $pythonVersion" -ForegroundColor Green
} catch {
    Write-Host "? ERROR: Python not found" -ForegroundColor Red
    Write-Host "Please install Python 3.10+ and add to PATH" -ForegroundColor Yellow
    pause
    exit 1
}

# Check dependencies
Write-Host "Checking Python packages..." -ForegroundColor Yellow
$packages = @("fastapi", "uvicorn", "httpx", "pydantic")
$missing = @()

foreach ($pkg in $packages) {
    python -c "import $pkg" 2>$null
    if ($LASTEXITCODE -ne 0) {
        $missing += $pkg
    }
}

if ($missing.Count -gt 0) {
    Write-Host "? Missing packages: $($missing -join ', ')" -ForegroundColor Red
    Write-Host ""
    Write-Host "Install with:" -ForegroundColor Yellow
    Write-Host "  pip install $($missing -join ' ')" -ForegroundColor White
    pause
    exit 1
}
Write-Host "? All packages installed" -ForegroundColor Green

# Check if port is in use
$portInUse = netstat -ano | Select-String ":$Port" | Select-String "LISTENING"
if ($portInUse) {
    Write-Host "? WARNING: Port $Port appears to be in use" -ForegroundColor Yellow
    Write-Host "Run Stop_Ai_Bridge.ps1 first or use tools\Debug\quickFix.bat" -ForegroundColor Yellow
    Write-Host ""
    $continue = Read-Host "Continue anyway? (y/n)"
    if ($continue -ne "y") {
        exit 0
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
        pause
        exit 1
    }
}

Write-Host ""
Write-Host "Starting toolserver..." -ForegroundColor Yellow
Write-Host "  Path: $ToolServerPath" -ForegroundColor Gray
Write-Host "  Port: $Port" -ForegroundColor Gray
Write-Host "  Mode: $(if ($MockMode) { 'Mock' } else { 'Live' })" -ForegroundColor Gray
Write-Host ""

# Change to repo root for proper module imports
Push-Location $RepoRoot

try {
    # Start uvicorn in background
    $process = Start-Process python -ArgumentList @(
        "-m", "uvicorn",
        "tools.toolserver:app",
        "--host", "127.0.0.1",
        "--port", $Port,
        "--reload"
    ) -PassThru -WindowStyle Hidden
    
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

Write-Host "Press any key to exit..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

# For automation/scripting: Allow Ctrl+C or closing console to exit gracefully
exit 0