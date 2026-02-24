[CmdletBinding()]
param(
    [int]$GatewayPort = 7077
)

$ErrorActionPreference = "Stop"

function Write-Info([string]$Text) {
    Write-Host "[INFO] $Text" -ForegroundColor Gray
}

function Write-Ok([string]$Text) {
    Write-Host "[OK]   $Text" -ForegroundColor Green
}

function Write-WarnMsg([string]$Text) {
    Write-Host "[WARN] $Text" -ForegroundColor Yellow
}

function Get-TcpListenerProcess {
    param([int]$Port)
    try {
        $conn = Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction Stop | Select-Object -First 1
        if ($conn) {
            return Get-Process -Id $conn.OwningProcess -ErrorAction Stop
        }
    } catch {
        try {
            $line = netstat -ano -p tcp | Select-String -Pattern (":$Port\s+.*LISTENING\s+") | Select-Object -First 1
            if ($line) {
                $parts = ($line.ToString() -replace '^\s+','') -split '\s+'
                $pid = [int]$parts[-1]
                return Get-Process -Id $pid -ErrorAction Stop
            }
        } catch {
            return $null
        }
    }
    return $null
}

Write-Host "Stopping Rogue Gateway stack..." -ForegroundColor Cyan

$gatewayProc = Get-TcpListenerProcess -Port $GatewayPort
if ($gatewayProc) {
    Write-Info "Killing gateway process PID $($gatewayProc.Id) ($($gatewayProc.ProcessName)) on port $GatewayPort"
    Stop-Process -Id $gatewayProc.Id -Force -ErrorAction SilentlyContinue
    Write-Ok "Gateway stopped"
} else {
    Write-Info "No process found on port $GatewayPort"
}

$zrokProcs = Get-Process -Name "zrok" -ErrorAction SilentlyContinue
if ($zrokProcs) {
    foreach ($p in $zrokProcs) {
        Write-Info "Killing zrok process PID $($p.Id)"
        Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    }
    Write-Ok "zrok stopped"
} else {
    Write-Info "No zrok processes found"
}

$ollamaProcs = Get-Process -Name "ollama*" -ErrorAction SilentlyContinue
if ($ollamaProcs) {
    Write-WarnMsg "Ollama processes are still running (use 'ollama stop' or close manually if needed)"
    $ollamaProcs | ForEach-Object { Write-Host "  PID $($_.Id): $($_.ProcessName)" -ForegroundColor DarkGray }
}

Write-Ok "Cleanup complete"
