# File: Start_Ai_Bridge.ps1
# Purpose:
#   - Ensure Ollama is running
#   - Start a local FastAPI toolserver (localhost:7077)
#   - Expose it via Cloudflare Tunnel (public HTTPS URL)
#   - Require an API key header for all requests (so randoms can't hit your tunnel)

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

# Shared secret expected from n8n Cloud in header: X-API-KEY
# You can hardcode your own or let the script generate one each run.
$ApiKey = [guid]::NewGuid().ToString("N")  # random per run

# --- Paths ---
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$RunDir = Join-Path $Root ".run"
New-Item -ItemType Directory -Force -Path $RunDir | Out-Null

$ToolServerPy        = Join-Path $RunDir "toolserver.py"
$ToolServerPidFile   = Join-Path $RunDir "toolserver.pid"
$TunnelPidFile       = Join-Path $RunDir "tunnel.pid"
$OllamaPidFile       = Join-Path $RunDir "ollama.pid"
$TunnelUrlFile       = Join-Path $RunDir "tunnel_url.txt"
$TunnelLogFile       = Join-Path $RunDir "tunnel.log"

# Track whether THIS script started Ollama (so stop script doesn't kill a pre-existing Ollama)
$OllamaStartedFlag   = Join-Path $RunDir "ollama_started_by_script.flag"

function Write-Header([string]$msg) {
    Write-Host "`n=== $msg ===" -ForegroundColor Cyan
}

function Test-Command([string]$name) {
    return $null -ne (Get-Command $name -ErrorAction SilentlyContinue)
}

function Ensure-OllamaRunning {
    Write-Header "Checking Ollama"

    try {
        $null = Invoke-RestMethod -Uri "http://127.0.0.1:11434/api/tags" -Method GET -TimeoutSec 2
        Write-Host "Ollama server already running." -ForegroundColor Green

        # If Ollama was already running, make sure we don't claim we started it
        if (Test-Path $OllamaStartedFlag) { Remove-Item -Force $OllamaStartedFlag | Out-Null }
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

    # Mark that THIS script started Ollama
    New-Item -ItemType File -Force -Path $OllamaStartedFlag | Out-Null

    $deadline = (Get-Date).AddSeconds(20)
    while ((Get-Date) -lt $deadline) {
        try {
            $null = Invoke-RestMethod -Uri "http://127.0.0.1:11434/api/tags" -Method GET -TimeoutSec 2
            Write-Host "Ollama is up." -ForegroundColor Green
            return
        } catch {
            Start-Sleep -Milliseconds 400
        }
    }

    throw "Ollama did not start within 20 seconds."
}

function Write-ToolServerFile {
@"
# File: .run/toolserver.py
# Local API for n8n Cloud -> your machine -> Ollama
# Runs on localhost only: http://127.0.0.1:$Port
# Endpoints:
#   GET  /health
#   POST /chat  {prompt, model?, system?}
#
# Security:
#   Requires header: X-API-KEY: <key>

from fastapi import FastAPI, Header, HTTPException
from pydantic import BaseModel
import httpx
import uvicorn

OLLAMA_URL = "http://127.0.0.1:11434"
DEFAULT_MODEL = "$Model"
API_KEY = "$ApiKey"

app = FastAPI()

class ChatIn(BaseModel):
    prompt: str
    model: str | None = None
    system: str | None = None

def require_key(x_api_key: str | None):
    if x_api_key != API_KEY:
        raise HTTPException(status_code=401, detail="Unauthorized")

@app.get("/health")
def health(x_api_key: str | None = Header(default=None)):
    require_key(x_api_key)
    return {"ok": True}

@app.post("/chat")
async def chat(payload: ChatIn, x_api_key: str | None = Header(default=None)):
    require_key(x_api_key)

    model = payload.model or DEFAULT_MODEL
    prompt = payload.prompt if not payload.system else f"{payload.system}\n\n{payload.prompt}"
    req = {"model": model, "prompt": prompt, "stream": False}

    async with httpx.AsyncClient(timeout=300.0) as client:
        r = await client.post(f"{OLLAMA_URL}/api/generate", json=req)
        r.raise_for_status()
        data = r.json()

    return {"model": model, "response": data.get("response", "")}

if __name__ == "__main__":
    uvicorn.run(app, host="$ToolServerHost", port=$Port)
"@ | Out-File -FilePath $ToolServerPy -Encoding utf8
}

function Ensure-PythonDeps {
    Write-Header "Ensuring Python deps"

    if (-not (Test-Command "python")) {
        throw "Python not found on PATH."
    }

    python -m pip install --upgrade pip | Out-Null
    python -m pip install fastapi uvicorn httpx pydantic | Out-Null

    Write-Host "Python deps OK." -ForegroundColor Green
}

function Start-ToolServer {
    Write-Header "Starting toolserver on $ToolServerUrl"

    Write-ToolServerFile
    Ensure-PythonDeps

    $p = Start-Process -FilePath "python" -ArgumentList $ToolServerPy -PassThru -WindowStyle Hidden
    $p.Id | Out-File -FilePath $ToolServerPidFile -Encoding ascii

    $deadline = (Get-Date).AddSeconds(20)
    while ((Get-Date) -lt $deadline) {
        try {
            $headers = @{ "X-API-KEY" = $ApiKey }
            $resp = Invoke-RestMethod -Uri "$ToolServerUrl/health" -Headers $headers -Method GET -TimeoutSec 2
            if ($resp.ok -eq $true) {
                Write-Host "Toolserver is up." -ForegroundColor Green
                return
            }
        } catch {
            Start-Sleep -Milliseconds 300
        }
    }

    throw "Toolserver did not start within 20 seconds."
}

function Start-Tunnel {
    Write-Header "Starting Cloudflare tunnel"

    if (-not (Test-Command "cloudflared")) {
        throw "cloudflared not found. Install: winget install Cloudflare.cloudflared"
    }

    Remove-Item -Force -ErrorAction SilentlyContinue $TunnelUrlFile, $TunnelLogFile | Out-Null

    $args = @("tunnel", "--url", $ToolServerUrl)

    $p = Start-Process -FilePath "cloudflared" -ArgumentList $args -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $TunnelLogFile -RedirectStandardError $TunnelLogFile

    $p.Id | Out-File -FilePath $TunnelPidFile -Encoding ascii

    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path $TunnelLogFile) {
            $log = Get-Content $TunnelLogFile -Raw -ErrorAction SilentlyContinue
            $m = [regex]::Match($log, "https://[a-z0-9\-]+\.trycloudflare\.com", "IgnoreCase")
            if ($m.Success) {
                $m.Value | Out-File -FilePath $TunnelUrlFile -Encoding ascii
                Write-Host "Tunnel URL: $($m.Value)" -ForegroundColor Green
                return $m.Value
            }
        }
        Start-Sleep -Milliseconds 400
    }

    throw "Could not find tunnel URL in log within 30 seconds. Check $TunnelLogFile"
}

# --- Main ---
Ensure-OllamaRunning
Start-ToolServer
$url = Start-Tunnel

Write-Header "NEXT STEP (n8n Cloud)"
Write-Host "Base URL:" -ForegroundColor Yellow
Write-Host "  $url" -ForegroundColor White
Write-Host "API Key (send as header X-API-KEY):" -ForegroundColor Yellow
Write-Host "  $ApiKey" -ForegroundColor White
Write-Host ""
Write-Host "Test endpoint (with header):" -ForegroundColor Yellow
Write-Host "  $url/health" -ForegroundColor White

Write-Host ""
Write-Host "Press ENTER to exit..." -ForegroundColor Yellow
Read-Host | Out-Null
