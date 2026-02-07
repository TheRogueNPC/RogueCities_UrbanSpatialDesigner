# File: Start_Ai_Bridge.ps1
# Purpose:
# - Ensure Ollama is running
# - Start a local FastAPI toolserver (localhost:7077)
# - Expose it via Cloudflare Tunnel (public HTTPS URL)
# - Intelligently handle existing processes

$ErrorActionPreference = "Stop"
trap {
    Write-Host "`nERROR:" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host "`nPress ENTER to exit..." -ForegroundColor Yellow
    Read-Host | Out-Null
    exit 1
}

# --- Config ---
$Port = 7077
$ToolServerHost = "127.0.0.1"
$ToolServerUrl = "http://$ToolServerHost`:$Port"
$Model = "qwen3-coder-optimized:latest"

# --- Paths ---
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$RunDir = Join-Path $Root ".run"
New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
$ToolServerPy = Join-Path $RunDir "toolserver.py"
$ToolServerPidFile = Join-Path $RunDir "toolserver.pid"
$TunnelPidFile = Join-Path $RunDir "tunnel.pid"
$OllamaPidFile = Join-Path $RunDir "ollama.pid"
$TunnelUrlFile = Join-Path $RunDir "tunnel_url.txt"

function Write-Header([string]$msg) {
    Write-Host "`n=== $msg ===" -ForegroundColor Cyan
}

function Test-Command([string]$name) {
    return $null -ne (Get-Command $name -ErrorAction SilentlyContinue)
}

function Show-Progress {
    param(
        [ScriptBlock]$Action,
        [string]$Message,
        [int]$TimeoutSeconds = 30,
        [int]$CheckIntervalMs = 300
    )
    
    $spinChars = @('|', '/', '-', '\')
    $spinIndex = 0
    $elapsed = 0
    $startTime = Get-Date
    
    $completed = $false
    $result = $null
    
    Write-Host "$Message " -NoNewline -ForegroundColor Yellow
    
    while (-not $completed -and $elapsed -lt $TimeoutSeconds) {
        Write-Host "`r$Message $($spinChars[$spinIndex]) " -NoNewline -ForegroundColor Yellow
        $spinIndex = ($spinIndex + 1) % $spinChars.Length
        
        try {
            $result = & $Action
            $completed = $true
        } catch {
            # Keep spinning
        }
        
        if (-not $completed) {
            Start-Sleep -Milliseconds $CheckIntervalMs
            $elapsed = ((Get-Date) - $startTime).TotalSeconds
            
            if ([math]::Floor($elapsed) -ne [math]::Floor($elapsed - ($CheckIntervalMs / 1000))) {
                Write-Host "`r$Message $($spinChars[$spinIndex]) ($([math]::Floor($elapsed))s/$($TimeoutSeconds)s) " -NoNewline -ForegroundColor Yellow
            }
        }
    }
    
    if ($completed) {
        Write-Host "`r$Message ✓ ($([math]::Round($elapsed, 1))s)" -ForegroundColor Green
        return $result
    } else {
        Write-Host "`r$Message ✗ (timeout after $($TimeoutSeconds)s)" -ForegroundColor Red
        throw "Operation timed out after $TimeoutSeconds seconds"
    }
}

function Ensure-OllamaRunning {
    Write-Header "Checking Ollama"
    
    $checkOllama = {
        $null = Invoke-RestMethod -Uri "http://127.0.0.1:11434/api/tags" -Method GET -TimeoutSec 2 -ErrorAction Stop
        return $true
    }
    
    try {
        Show-Progress -Action $checkOllama -Message "Testing Ollama connection" -TimeoutSeconds 5 -CheckIntervalMs 500
        Write-Host "Ollama server already running." -ForegroundColor Green
        return
    } catch {
        # Not running; start it.
    }

    if (-not (Test-Command "ollama")) {
        throw "Ollama not found on PATH. Install from https://ollama.com and restart PowerShell."
    }

    Write-Host "Starting Ollama server..." -ForegroundColor Yellow
    $p = Start-Process -FilePath "ollama" -ArgumentList "serve" -PassThru -WindowStyle Hidden
    $p.Id | Out-File -FilePath $OllamaPidFile -Encoding ascii
    
    Show-Progress -Action $checkOllama -Message "Waiting for Ollama to start" -TimeoutSeconds 30 -CheckIntervalMs 500
    Write-Host "Ollama is up." -ForegroundColor Green
}

function Write-ToolServerFile {
@"
# File: .run/toolserver.py
from fastapi import FastAPI
from pydantic import BaseModel
import httpx
import uvicorn

OLLAMA_URL = "http://127.0.0.1:11434"
DEFAULT_MODEL = "$Model"

app = FastAPI()

class ChatIn(BaseModel):
    prompt: str
    model: str | None = None
    system: str | None = None

@app.get("/health")
async def health():
    try:
        async with httpx.AsyncClient(timeout=5.0) as client:
            r = await client.get(f"{OLLAMA_URL}/api/tags")
            r.raise_for_status()
            data = r.json()
            models = data.get("models", [])
            model_names = [m.get("name", "unknown") for m in models]
            return {
                "ok": True,
                "ollama": "connected",
                "models_available": len(models),
                "models": model_names,
                "default_model": DEFAULT_MODEL
            }
    except httpx.ConnectError:
        return {"ok": False, "ollama": "disconnected", "error": f"Cannot connect to Ollama at {OLLAMA_URL}"}
    except Exception as e:
        return {"ok": False, "ollama": "error", "error": str(e)}

@app.get("/models")
async def list_models():
    async with httpx.AsyncClient(timeout=5.0) as client:
        r = await client.get(f"{OLLAMA_URL}/api/tags")
        r.raise_for_status()
        return r.json()

@app.post("/chat")
async def chat(payload: ChatIn):
    model = payload.model or DEFAULT_MODEL
    prompt = payload.prompt if not payload.system else f"{payload.system}\n\n{payload.prompt}"
    req = {"model": model, "prompt": prompt, "stream": False}
    async with httpx.AsyncClient(timeout=300.0) as client:
        r = await client.post(f"{OLLAMA_URL}/api/generate", json=req)
        r.raise_for_status()
        data = r.json()
        return {"model": model, "response": data.get("response", "")}

if __name__ == "__main__":
    uvicorn.run(app, host="$ToolServerHost", port=$Port, log_level="warning")
"@ | Out-File -FilePath $ToolServerPy -Encoding utf8
}

function Ensure-PythonDeps {
    Write-Header "Ensuring Python deps"
    if (-not (Test-Command "python")) {
        throw "Python not found on PATH."
    }

    Write-Host "Checking Python modules... " -NoNewline -ForegroundColor Yellow
    $hasModules = python -c "import fastapi, uvicorn, httpx, pydantic" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "✗" -ForegroundColor Red
        Write-Host "Installing Python dependencies..." -ForegroundColor Yellow
        
        $installAction = {
            python -m pip install --upgrade pip --quiet 2>&1 | Out-Null
            python -m pip install fastapi uvicorn httpx pydantic --quiet 2>&1 | Out-Null
            if ($LASTEXITCODE -eq 0) { return $true }
            throw "Install failed"
        }
        
        Show-Progress -Action $installAction -Message "Installing packages" -TimeoutSeconds 90 -CheckIntervalMs 1000
    } else {
        Write-Host "✓" -ForegroundColor Green
    }
    Write-Host "Python deps OK." -ForegroundColor Green
}

function Test-ExistingToolServer {
    Write-Header "Checking for existing toolserver"
    
    $portCheck = netstat -ano | Select-String ":$Port " | Select-String "LISTENING"
    
    if (-not $portCheck) {
        Write-Host "No process on port $Port." -ForegroundColor Gray
        return $false
    }
    
    $existingPid = ($portCheck -split '\s+')[-1]
    Write-Host "Found process on port $Port (PID: $existingPid)" -ForegroundColor Yellow
    
    $healthCheck = {
        $resp = Invoke-RestMethod -Uri "$ToolServerUrl/health" -Method GET -TimeoutSec 3 -ErrorAction Stop
        if ($resp.ok -eq $true) {
            return $resp
        }
        throw "Unhealthy"
    }
    
    try {
        $resp = Show-Progress -Action $healthCheck -Message "Testing existing server health" -TimeoutSeconds 5 -CheckIntervalMs 500
        Write-Host "✓ Existing toolserver is healthy!" -ForegroundColor Green
        Write-Host "  Ollama: $($resp.ollama)" -ForegroundColor Green
        Write-Host "  Models: $($resp.models_available)" -ForegroundColor Green
        
        $existingPid | Out-File -FilePath $ToolServerPidFile -Encoding ascii
        return $true
    } catch {
        Write-Host "✗ Existing server unhealthy. Restarting..." -ForegroundColor Yellow
        taskkill /PID $existingPid /F 2>&1 | Out-Null
        Start-Sleep -Seconds 2
        return $false
    }
}

function Start-ToolServer {
    Write-Header "Starting toolserver on $ToolServerUrl"
    Write-ToolServerFile
    Ensure-PythonDeps
    
    Write-Host "Launching Python process..." -ForegroundColor Yellow
    $p = Start-Process -FilePath "python" -ArgumentList "`"$ToolServerPy`"" -PassThru -WindowStyle Hidden
    $p.Id | Out-File -FilePath $ToolServerPidFile -Encoding ascii
    Write-Host "  PID: $($p.Id)" -ForegroundColor Gray
    
    $healthCheck = {
        $resp = Invoke-RestMethod -Uri "$ToolServerUrl/health" -Method GET -TimeoutSec 2 -ErrorAction Stop
        if ($resp.ok -eq $true) {
            return $resp
        }
        throw "Not ready"
    }
    
    $resp = Show-Progress -Action $healthCheck -Message "Waiting for toolserver startup" -TimeoutSeconds 30 -CheckIntervalMs 400
    Write-Host "Toolserver is up." -ForegroundColor Green
    Write-Host "  Models available: $($resp.models_available)" -ForegroundColor Green
}

function Test-ExistingTunnel {
    Write-Header "Checking for existing tunnel"
    
    if (Test-Path $TunnelUrlFile) {
        $fileAge = (Get-Date) - (Get-Item $TunnelUrlFile).LastWriteTime
        if ($fileAge.TotalHours -lt 2) {
            $savedUrl = Get-Content $TunnelUrlFile -Raw
            $savedUrl = $savedUrl.Trim()
            
            Write-Host "Found recent tunnel URL from $([math]::Round($fileAge.TotalMinutes, 0)) minutes ago" -ForegroundColor Yellow
            
            $tunnelTest = {
                $null = Invoke-RestMethod -Uri "$savedUrl/health" -Method GET -TimeoutSec 5 -ErrorAction Stop
                return $savedUrl
            }
            
            try {
                $url = Show-Progress -Action $tunnelTest -Message "Testing saved tunnel" -TimeoutSeconds 8 -CheckIntervalMs 500
                Write-Host "✓ Existing tunnel is still active!" -ForegroundColor Green
                return $url
            } catch {
                Write-Host "✗ Saved tunnel no longer active." -ForegroundColor Gray
            }
        }
    }
    
    return $null
}

function Start-Tunnel {
    Write-Header "Starting Cloudflare tunnel"
    if (-not (Test-Command "cloudflared")) {
        throw "cloudflared not found. Install: winget install Cloudflare.cloudflared"
    }

    # Kill any existing cloudflared processes first
    $existing = Get-Process cloudflared -ErrorAction SilentlyContinue
    if ($existing) {
        Write-Host "Killing existing cloudflared processes..." -ForegroundColor Yellow
        $existing | Stop-Process -Force
        Start-Sleep -Seconds 1
    }

    Write-Host "Launching cloudflared (this takes 30-45 seconds)..." -ForegroundColor Yellow
    
    # Use a temporary batch file to capture output reliably in PowerShell 7
    $batchFile = Join-Path $RunDir "start_tunnel.bat"
    $tunnelLog = Join-Path $RunDir "tunnel_output.txt"
    
    @"
@echo off
cloudflared tunnel --url $ToolServerUrl > "$tunnelLog" 2>&1
"@ | Out-File -FilePath $batchFile -Encoding ascii
    
    # Start the batch file
    $proc = Start-Process -FilePath "cmd.exe" -ArgumentList "/c `"$batchFile`"" -PassThru -WindowStyle Hidden
    $proc.Id | Out-File -FilePath $TunnelPidFile -Encoding ascii
    Write-Host "  PID: $($proc.Id)" -ForegroundColor Gray
    
    # Wait for URL to appear in log file
    $getTunnelUrl = {
        if (Test-Path $tunnelLog) {
            $content = Get-Content $tunnelLog -Raw -ErrorAction SilentlyContinue
            if ($content -match "https://[a-z0-9\-]+\.trycloudflare\.com") {
                return $Matches[0]
            }
        }
        throw "Not ready"
    }
    
    try {
        $url = Show-Progress -Action $getTunnelUrl -Message "Waiting for tunnel URL" -TimeoutSeconds 60 -CheckIntervalMs 1000
        $url | Out-File -FilePath $TunnelUrlFile -Encoding ascii
        Write-Host "Tunnel URL: $url" -ForegroundColor Green
        return $url
    } catch {
        Write-Host "`nFailed to detect tunnel URL." -ForegroundColor Red
        if (Test-Path $tunnelLog) {
            Write-Host "Cloudflared output:" -ForegroundColor Yellow
            Get-Content $tunnelLog | Select-Object -First 20 | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
        }
        throw "Could not detect tunnel URL. Check if firewall is blocking cloudflared."
    }
}

# --- Main ---
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "       AI Bridge Startup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

Ensure-OllamaRunning

$serverReady = Test-ExistingToolServer

if (-not $serverReady) {
    Start-ToolServer
}

$existingTunnel = Test-ExistingTunnel

if ($existingTunnel) {
    $url = $existingTunnel
    Write-Host "`nReusing existing tunnel." -ForegroundColor Green
} else {
    $url = Start-Tunnel
}

Write-Header "AI BRIDGE READY"
Write-Host "Use this base URL in n8n Cloud:" -ForegroundColor Yellow
Write-Host "  $url" -ForegroundColor White
Write-Host ""
Write-Host "Endpoints:" -ForegroundColor Yellow
Write-Host "  GET  $url/health" -ForegroundColor White
Write-Host "  GET  $url/models" -ForegroundColor White
Write-Host "  POST $url/chat" -ForegroundColor White
Write-Host ""
Write-Host "Press ENTER to stop and exit..." -ForegroundColor Yellow
Read-Host | Out-Null

# Cleanup
Write-Host "`nStopping services..." -ForegroundColor Yellow
if (Test-Path $TunnelPidFile) {
    $tunnelPid = Get-Content $TunnelPidFile
    taskkill /PID $tunnelPid /F /T 2>&1 | Out-Null
}
if (Test-Path $ToolServerPidFile) {
    $serverPid = Get-Content $ToolServerPidFile
    taskkill /PID $serverPid /F 2>&1 | Out-Null
}
Write-Host "Done." -ForegroundColor Green
Write-Host "`nPress ENTER to exit..." -ForegroundColor Yellow
Read-Host | Out-Null
