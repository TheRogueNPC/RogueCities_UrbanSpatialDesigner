# File: Start-Stack.ps1
# Combined launcher for RogueCities Gateway Stack
#
# Starts: Gateway (7077) + RogueFS Agent (7078) + zrok tunnel
# Features: Health monitoring, auto-restart, unified logging, hotkeys

[CmdletBinding()]
param(
    [string]$ProjectDir = "",
    [string]$GatewayDir = "",
    [string]$RogueFSDir = "",
    
    # Ports
    [int]$GatewayPort = 7077,
    [int]$RogueFSPort = 7078,
    
    # Features
    [switch]$SkipZrok,
    [switch]$SkipOllama,
    [switch]$ForceKillExisting,
    [switch]$KeepLauncherOpen = $true,
    
    # Timeouts
    [int]$HealthTimeoutSeconds = 60,
    [int]$StartupDelayMs = 1500
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Refresh PATH from registry (catches tools installed via winget)
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

# Resolve project directory
if (-not $ProjectDir) {
    if ($PSScriptRoot) {
        $ProjectDir = $PSScriptRoot
    } else {
        $ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
        if (-not $ProjectDir) {
            $ProjectDir = Get-Location
        }
    }
}

# Resolve directories
if (-not $GatewayDir) {
    $GatewayDir = Join-Path $ProjectDir "ollama-gateway"
}
if (-not $RogueFSDir) {
    $RogueFSDir = Join-Path $ProjectDir "roguefs-agent"
}

# Verify directories exist before resolving
if (-not (Test-Path $GatewayDir)) {
    throw "Gateway directory not found: $GatewayDir"
}
if (-not (Test-Path $RogueFSDir)) {
    throw "RogueFS directory not found: $RogueFSDir"
}

$GatewayDir = (Resolve-Path $GatewayDir -ErrorAction Stop).Path
$RogueFSDir = (Resolve-Path $RogueFSDir -ErrorAction Stop).Path
$ProjectDir = (Resolve-Path $ProjectDir -ErrorAction Stop).Path

# Log directory
$LogDir = Join-Path $env:TEMP "rogue-stack-logs"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

# Log files
$GatewayOutLog = Join-Path $LogDir "gateway.out.log"
$GatewayErrLog = Join-Path $LogDir "gateway.err.log"
$RogueFSOutLog = Join-Path $LogDir "roguefs.out.log"
$RogueFSErrLog = Join-Path $LogDir "roguefs.err.log"
$ZrokOutLog = Join-Path $LogDir "zrok.out.log"
$ZrokErrLog = Join-Path $LogDir "zrok.err.log"

# Clear old logs
foreach ($f in @($GatewayOutLog, $GatewayErrLog, $RogueFSOutLog, $RogueFSErrLog, $ZrokOutLog, $ZrokErrLog)) {
    try { if (Test-Path $f) { Remove-Item $f -Force -ErrorAction SilentlyContinue } } catch {}
}

# UI helpers
function Write-Section([string]$Text) {
    Write-Host ""
    Write-Host ("=" * 70) -ForegroundColor DarkGray
    Write-Host "  $Text" -ForegroundColor Cyan
    Write-Host ("=" * 70) -ForegroundColor DarkGray
}

function Write-Info([string]$Text) { Write-Host "[INFO] $Text" -ForegroundColor Gray }
function Write-Ok([string]$Text) { Write-Host "[OK]   $Text" -ForegroundColor Green }
function Write-Warn([string]$Text) { Write-Host "[WARN] $Text" -ForegroundColor Yellow }
function Write-Err([string]$Text) { Write-Host "[ERR]  $Text" -ForegroundColor Red }

function Test-Port {
    param([int]$Port)
    try {
        $conn = Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue
        return $null -ne $conn
    } catch {
        return $false
    }
}

function Get-PortProcess {
    param([int]$Port)
    try {
        $conn = Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction Stop | Select-Object -First 1
        if ($conn) {
            return Get-Process -Id $conn.OwningProcess -ErrorAction Stop
        }
    } catch {}
    return $null
}

function Stop-ProcessSafe {
    param($Proc, [int]$GraceMs = 1500)
    if ($null -eq $Proc) { return }
    try { $null = $Proc.HasExited } catch { return }
    if ($Proc.HasExited) { return }
    
    Write-Info "Stopping PID $($Proc.Id) ($($Proc.ProcessName))..."
    try { Stop-Process -Id $Proc.Id -ErrorAction SilentlyContinue } catch {}
    Start-Sleep -Milliseconds $GraceMs
    
    try { $Proc.Refresh() } catch {}
    try {
        if (-not $Proc.HasExited) {
            Write-Warn "Force killing PID $($Proc.Id)..."
            Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue
        }
    } catch {}
}

function Start-LoggedProcess {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList,
        [string]$WorkingDirectory,
        [string]$StdOutPath,
        [string]$StdErrPath,
        [hashtable]$Env = @{},
        [string]$Title = ""
    )
    
    if ($Title) { Write-Info "Starting $Title..." }
    
    $savedEnv = @{}
    foreach ($key in $Env.Keys) {
        $savedEnv[$key] = [Environment]::GetEnvironmentVariable($key)
        [Environment]::SetEnvironmentVariable($key, $Env[$key], "Process")
    }
    
    try {
        $proc = Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -WorkingDirectory $WorkingDirectory -RedirectStandardOutput $StdOutPath -RedirectStandardError $StdErrPath -NoNewWindow -PassThru
        
        foreach ($key in $savedEnv.Keys) {
            if ($null -eq $savedEnv[$key]) {
                [Environment]::SetEnvironmentVariable($key, $null, "Process")
            } else {
                [Environment]::SetEnvironmentVariable($key, $savedEnv[$key], "Process")
            }
        }
        
        Start-Sleep -Milliseconds $StartupDelayMs
        
        $proc.Refresh()
        if ($proc.HasExited) {
            $errPreview = ""
            if (Test-Path $StdErrPath) {
                $errPreview = (Get-Content $StdErrPath -Tail 40 -ErrorAction SilentlyContinue) -join "`n"
            }
            throw "$Title exited immediately.`n$errPreview"
        }
        return $proc
    } catch {
        foreach ($key in $savedEnv.Keys) {
            if ($null -eq $savedEnv[$key]) {
                [Environment]::SetEnvironmentVariable($key, $null, "Process")
            } else {
                [Environment]::SetEnvironmentVariable($key, $savedEnv[$key], "Process")
            }
        }
        throw
    }
}

function Wait-ForHealth {
    param([string]$Url, [int]$TimeoutSeconds, [string]$ApiKey = "")
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $lastError = $null
    
    while ((Get-Date) -lt $deadline) {
        try {
            $headers = @{}
            if ($ApiKey) { $headers["Authorization"] = "Bearer $ApiKey" }
            $null = Invoke-RestMethod -Uri $Url -Method GET -TimeoutSec 3 -Headers $headers
            return $true
        } catch {
            $lastError = $_.Exception.Message
            Start-Sleep -Milliseconds 500
        }
    }
    throw "Health check timed out: $Url`nLast error: $lastError"
}

function Get-ZrokUrl {
    param([string]$OutLog, [string]$ErrLog)
    foreach ($path in @($OutLog, $ErrLog)) {
        if (-not (Test-Path $path)) { continue }
        try {
            $lines = Get-Content $path -Tail 200 -ErrorAction SilentlyContinue
            foreach ($line in $lines) {
                if ($line -match 'https://[a-zA-Z0-9\.-]+\.share\.zrok\.io') {
                    return $Matches[0]
                }
            }
        } catch {}
    }
    return $null
}

function Dump-LogTail {
    param([string]$Path, [int]$Tail = 40, [string]$Label = "")
    if (-not (Test-Path $Path)) {
        Write-Warn "$Label (not found)"
        return
    }
    if ($Label) { Write-Host "--- $Label ---" -ForegroundColor DarkYellow }
    Get-Content $Path -Tail $Tail -ErrorAction SilentlyContinue
}

# Load environment
$envFile = Join-Path $GatewayDir ".env"
if (Test-Path $envFile) {
    Write-Info "Loading environment from: $envFile"
    Get-Content $envFile | ForEach-Object {
        if ($_ -match '^([^#][^=]+)=(.*)$') {
            $name = $Matches[1].Trim()
            $value = $Matches[2].Trim()
            if ($value.StartsWith('"') -and $value.EndsWith('"')) {
                $value = $value.Substring(1, $value.Length - 2)
            }
            [Environment]::SetEnvironmentVariable($name, $value, "Process")
        }
    }
}

# Set derived env vars
if (-not $env:CODE_ROOT) {
    $env:CODE_ROOT = (Resolve-Path (Join-Path $ProjectDir "..\..")).Path
}
$env:GATEWAY_PORT = $GatewayPort
$env:ROGUEFS_PORT = $RogueFSPort

# Track processes
$script:started = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
$script:zrokUrl = $null
$script:cleanedUp = $false

function Cleanup-All {
    if ($script:cleanedUp) { return }
    $script:cleanedUp = $true
    
    Write-Section "Stopping stack"
    for ($i = $script:started.Count - 1; $i -ge 0; $i--) {
        Stop-ProcessSafe -Proc $script:started[$i]
    }
    
    Write-Info "Log directory: $LogDir"
    Write-Ok "Shutdown complete"
}

# Main execution
try {
    Write-Section "RogueCities Gateway Stack v2.0"
    Write-Info "Gateway dir: $GatewayDir"
    Write-Info "RogueFS dir: $RogueFSDir"
    Write-Info "Gateway port: $GatewayPort"
    Write-Info "RogueFS port: $RogueFSPort"
    
    # Check Python
    $VenvPython = Join-Path $GatewayDir ".venv\Scripts\python.exe"
    if (Test-Path $VenvPython) {
        $PythonExe = $VenvPython
        Write-Info "Using venv Python: $VenvPython"
    } elseif (Get-Command "python" -ErrorAction SilentlyContinue) {
        $PythonExe = (Get-Command "python").Source
        Write-Info "Using system Python: $PythonExe"
    } else {
        throw "Python not found"
    }
    
    # Check ripgrep
    if (-not (Get-Command "rg" -ErrorAction SilentlyContinue)) {
        Write-Warn "ripgrep (rg) not found. Search will be limited."
        Write-Warn "Install: winget install BurntSushi.ripgrep.MSVC"
    }
    
    # Check zrok
    if (-not $SkipZrok -and -not (Get-Command "zrok" -ErrorAction SilentlyContinue)) {
        Write-Warn "zrok not found. Tunnel will be skipped."
        $SkipZrok = $true
    }
    
    # Always kill existing processes on ports (prevents bind errors)
    foreach ($port in @($GatewayPort, $RogueFSPort)) {
        $existing = Get-PortProcess -Port $port
        if ($existing) {
            Write-Warn "Killing existing process on port $port (PID $($existing.Id))"
            Stop-Process -Id $existing.Id -Force -ErrorAction SilentlyContinue
        }
    }
    
    # Also kill any stale zrok processes
    Get-Process -Name zrok -ErrorAction SilentlyContinue | ForEach-Object {
        Write-Warn "Killing stale zrok process (PID $($_.Id))"
        Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
    }
    
    # Wait for ports to be fully released
    Start-Sleep -Milliseconds 1500
    
    # Verify ports are free
    foreach ($port in @($GatewayPort, $RogueFSPort)) {
        $stillThere = Get-NetTCPConnection -State Listen -LocalPort $port -ErrorAction SilentlyContinue
        if ($stillThere) {
            Write-Warn "Port $port still in use, force killing..."
            Stop-Process -Id $stillThere.OwningProcess -Force -ErrorAction SilentlyContinue
            Start-Sleep -Milliseconds 1000
        }
    }
    
    # Check Ollama
    if (-not $SkipOllama) {
        try {
            $null = Invoke-RestMethod -Uri "http://127.0.0.1:11434/api/tags" -TimeoutSec 3
            Write-Ok "Ollama reachable"
        } catch {
            if (Get-Command "ollama" -ErrorAction SilentlyContinue) {
                Write-Info "Starting Ollama..."
                $ollamaProc = Start-Process -FilePath "ollama" -ArgumentList "serve" -PassThru -WindowStyle Hidden
                $script:started.Add($ollamaProc)
                Start-Sleep -Milliseconds 2000
            } else {
                Write-Warn "Ollama not found. LLM features unavailable."
            }
        }
    }
    
    # Start RogueFS Agent
    Write-Section "Starting RogueFS Agent"
    $roguefsScript = Join-Path $RogueFSDir "roguefs-agent.py"
    if (-not (Test-Path $roguefsScript)) {
        throw "roguefs-agent.py not found at: $roguefsScript"
    }
    
    $roguefsProc = Start-LoggedProcess `
        -FilePath $PythonExe `
        -ArgumentList @($roguefsScript) `
        -WorkingDirectory $RogueFSDir `
        -StdOutPath $RogueFSOutLog `
        -StdErrPath $RogueFSErrLog `
        -Title "RogueFS Agent"
    $script:started.Add($roguefsProc)
    Write-Ok "RogueFS started (PID $($roguefsProc.Id)) on port $RogueFSPort"
    
    # Wait for RogueFS health
    Write-Info "Waiting for RogueFS health..."
    Wait-ForHealth -Url "http://127.0.0.1:$RogueFSPort/v1/health" -TimeoutSeconds 30 | Out-Null
    Write-Ok "RogueFS healthy"
    
    # Start Gateway
    Write-Section "Starting Gateway"
    $gatewayScript = Join-Path $GatewayDir "gateway.py"
    if (-not (Test-Path $gatewayScript)) {
        throw "gateway.py not found at: $gatewayScript"
    }
    
    $gatewayEnv = @{
        "CODE_ROOT" = $env:CODE_ROOT
        "OLLAMA_BASE_URL" = $env:OLLAMA_BASE_URL
        "ROGUEFS_PORT" = $RogueFSPort
    }
    if ($env:API_KEY) { $gatewayEnv["API_KEY"] = $env:API_KEY }
    
    $gatewayProc = Start-LoggedProcess `
        -FilePath $PythonExe `
        -ArgumentList @("-m", "uvicorn", "gateway:app", "--host", "127.0.0.1", "--port", $GatewayPort) `
        -WorkingDirectory $GatewayDir `
        -StdOutPath $GatewayOutLog `
        -StdErrPath $GatewayErrLog `
        -Env $gatewayEnv `
        -Title "Gateway"
    $script:started.Add($gatewayProc)
    Write-Ok "Gateway started (PID $($gatewayProc.Id)) on port $GatewayPort"
    
    # Wait for Gateway health
    Write-Info "Waiting for Gateway health..."
    $apiKey = $env:API_KEY
    Wait-ForHealth -Url "http://127.0.0.1:$GatewayPort/health" -TimeoutSeconds $HealthTimeoutSeconds -ApiKey $apiKey | Out-Null
    Write-Ok "Gateway healthy"
    
    # Start zrok tunnel (reserved share for persistent URL)
    $ZrokShareName = "roguecities"
    $script:zrokUrl = "https://roguecities.share.zrok.io"
    
    if (-not $SkipZrok) {
        Write-Section "Starting zrok tunnel"
        Write-Info "Using reserved share: $ZrokShareName"
        $zrokProc = Start-LoggedProcess `
            -FilePath "zrok" `
            -ArgumentList @("share", "reserved", $ZrokShareName, "--headless") `
            -WorkingDirectory $ProjectDir `
            -StdOutPath $ZrokOutLog `
            -StdErrPath $ZrokErrLog `
            -Title "zrok tunnel"
        $script:started.Add($zrokProc)
        Write-Ok "zrok started (PID $($zrokProc.Id))"
        
        # Wait for zrok to connect
        Write-Info "Waiting for zrok tunnel to connect..."
        $zrokReady = $false
        for ($i = 0; $i -lt 15; $i++) {
            try {
                $null = Invoke-RestMethod -Uri $script:zrokUrl -TimeoutSec 2
                $zrokReady = $true
                break
            } catch {
                Start-Sleep -Milliseconds 1000
            }
        }
        if ($zrokReady) {
            Write-Ok "zrok URL: $($script:zrokUrl)"
        } else {
            Write-Warn "zrok tunnel may not be ready yet"
        }
    }
    
    # Stack running
    Write-Section "Stack Running"
    Write-Host "Endpoints:" -ForegroundColor DarkGray
    Write-Host "  Gateway:  http://127.0.0.1:$GatewayPort" -ForegroundColor White
    Write-Host "  RogueFS:  http://127.0.0.1:$RogueFSPort/v1/health" -ForegroundColor White
    if ($script:zrokUrl) {
        Write-Host "  Public:   $($script:zrokUrl)" -ForegroundColor Cyan
    }
    Write-Host ""
    Write-Host "Logs: $LogDir" -ForegroundColor DarkGray
    Write-Host ""
    
    $interactive = $true
    try {
        $null = [Console]::KeyAvailable
    } catch {
        $interactive = $false
    }
    
    if ($interactive) {
        Write-Host "Hotkeys: L=logs  U=URL  Q=quit" -ForegroundColor Yellow
        Write-Host ""
        
        # Main loop with hotkey handling
        $originalTreat = [Console]::TreatControlCAsInput
        [Console]::TreatControlCAsInput = $true
        
        try {
            while ($true) {
                # Check process health
                $alive = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
                foreach ($p in $script:started.ToArray()) {
                    try {
                        $p.Refresh()
                        if ($p.HasExited) {
                            Write-Warn "Process exited: PID $($p.Id) ($($p.ProcessName))"
                        } else {
                            $alive.Add($p)
                        }
                    } catch {}
                }
                $script:started = $alive
                
                # Handle hotkeys
                if ([Console]::KeyAvailable) {
                    $key = [Console]::ReadKey($true)
                    $isCtrl = ($key.Modifiers -band [ConsoleModifiers]::Control) -ne 0
                    
                    if ($isCtrl -and ($key.Key -eq [ConsoleKey]::C -or $key.Key -eq [ConsoleKey]::E)) {
                        Write-Host ""
                        Write-Warn "Ctrl+C detected. Exiting..."
                        break
                    }
                    
                    switch ($key.Key) {
                        { $_ -in @([ConsoleKey]::L) } {
                            Write-Host ""
                            Dump-LogTail -Path $GatewayErrLog -Tail 30 -Label "gateway.err"
                            Dump-LogTail -Path $RogueFSErrLog -Tail 30 -Label "roguefs.err"
                            Write-Host ""
                        }
                        { $_ -in @([ConsoleKey]::U) } {
                            if ($script:zrokUrl) {
                                Write-Host "zrok URL: $($script:zrokUrl)" -ForegroundColor Cyan
                            } else {
                                Write-Warn "No zrok URL available"
                            }
                        }
                        { $_ -in @([ConsoleKey]::Q) } {
                            Write-Host ""
                            Write-Warn "Quit requested. Exiting..."
                            break
                        }
                    }
                }
                
                Start-Sleep -Milliseconds 200
            }
        }
        finally {
            [Console]::TreatControlCAsInput = $originalTreat
        }
    } else {
        Write-Host "Running in non-interactive mode. Ctrl+C to stop." -ForegroundColor Yellow
        Write-Host ""
        
        # Simple wait loop for non-interactive mode
        while ($true) {
            $alive = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
            foreach ($p in $script:started.ToArray()) {
                try {
                    $p.Refresh()
                    if ($p.HasExited) {
                        Write-Warn "Process exited: PID $($p.Id) ($($p.ProcessName))"
                    } else {
                        $alive.Add($p)
                    }
                } catch {}
            }
            $script:started = $alive
            
            if ($script:started.Count -eq 0) {
                Write-Err "All processes exited. Stopping."
                break
            }
            
            Start-Sleep -Seconds 5
        }
    }
    
    Cleanup-All
}
catch {
    Write-Host ""
    Write-Err $_.Exception.Message
    
    Write-Host ""
    Dump-LogTail -Path $GatewayErrLog -Tail 50 -Label "gateway.err"
    Dump-LogTail -Path $RogueFSErrLog -Tail 50 -Label "roguefs.err"
    
    Cleanup-All
    
    if ($KeepLauncherOpen) {
        Write-Host ""
        Write-Host "Press any key to close..." -ForegroundColor Yellow
        $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    }
    
    exit 1
}
finally {
    if ($KeepLauncherOpen -and -not $script:cleanedUp) {
        Write-Host ""
        Write-Host "Press any key to close..." -ForegroundColor Yellow
        $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    }
}
