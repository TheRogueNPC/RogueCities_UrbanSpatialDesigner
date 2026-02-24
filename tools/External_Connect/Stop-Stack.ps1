# File: Stop-Stack.ps1
# Stops all RogueCities Gateway Stack processes
#
# Kills: Gateway (7077), RogueFS Agent (7078), zrok tunnel, Ollama (optional)

[CmdletBinding()]
param(
    [int]$GatewayPort = 7077,
    [int]$RogueFSPort = 7078,
    [switch]$StopOllama,
    [switch]$Force
)

function Write-Info([string]$Text) { Write-Host "[INFO] $Text" -ForegroundColor Gray }
function Write-Ok([string]$Text) { Write-Host "[OK]   $Text" -ForegroundColor Green }
function Write-Warn([string]$Text) { Write-Host "[WARN] $Text" -ForegroundColor Yellow }

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
    param($Proc, [string]$Name = "process")
    if ($null -eq $Proc) { return $false }
    try { $null = $Proc.HasExited } catch { return $false }
    if ($Proc.HasExited) { return $true }
    
    Write-Info "Stopping $Name (PID $($Proc.Id))..."
    try {
        Stop-Process -Id $Proc.Id -Force:$Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 500
        
        $Proc.Refresh()
        if (-not $Proc.HasExited) {
            Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue
        }
        Write-Ok "$Name stopped"
        return $true
    } catch {
        Write-Warn "Failed to stop $Name"
        return $false
    }
}

function Stop-ZrokTunnels {
    $stopped = 0
    try {
        $zrokProcs = Get-Process -Name "zrok" -ErrorAction SilentlyContinue
        if ($zrokProcs) {
            foreach ($proc in @($zrokProcs)) {
                if (Stop-ProcessSafe -Proc $proc -Name "zrok tunnel") {
                    $stopped++
                }
            }
        }
    } catch {}
    return $stopped
}

function Stop-PythonOnPort {
    param([int]$Port, [string]$Name)
    $proc = Get-PortProcess -Port $Port
    if ($proc) {
        return Stop-ProcessSafe -Proc $proc -Name $Name
    }
    Write-Info "$Name not running on port $Port"
    return $false
}

$stoppedAny = $false

Write-Host ""
Write-Host "Stopping RogueCities Gateway Stack..." -ForegroundColor Cyan
Write-Host ""

if (Stop-PythonOnPort -Port $GatewayPort -Name "Gateway") { $stoppedAny = $true }
if (Stop-PythonOnPort -Port $RogueFSPort -Name "RogueFS") { $stoppedAny = $true }

$zrokStopped = Stop-ZrokTunnels
if ($zrokStopped -gt 0) {
    Write-Ok "Stopped $zrokStopped zrok tunnel(s)"
    $stoppedAny = $true
}

if ($StopOllama) {
    try {
        $ollamaProcs = Get-Process -Name "ollama" -ErrorAction SilentlyContinue
        if ($ollamaProcs) {
            foreach ($proc in @($ollamaProcs)) {
                Stop-ProcessSafe -Proc $proc -Name "Ollama"
            }
            $stoppedAny = $true
        }
    } catch {}
}

Write-Host ""
if ($stoppedAny) {
    Write-Ok "Stack shutdown complete"
} else {
    Write-Info "No processes found to stop"
}
