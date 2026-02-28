# Stop_Ai_Bridge_Fixed.ps1
# Reliably stops the RogueCity AI Bridge toolserver

param(
    [switch]$NonInteractive,
    [int]$Port = 7077
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  RogueCity AI Bridge - Stop" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$PidFile = Join-Path $PSScriptRoot ".run\toolserver.pid"
$TaskkillExe = Join-Path $env:WINDIR "System32\taskkill.exe"

function Invoke-TaskKill {
    param([int]$PidToKill)
    try {
        if ($TaskkillExe -and (Test-Path -LiteralPath $TaskkillExe)) {
            & $TaskkillExe /PID $PidToKill /T /F | Out-Null
            return $true
        }
        & cmd /c "taskkill /PID $PidToKill /T /F" | Out-Null
        return $true
    } catch {
        return $false
    }
}

function Invoke-CimTerminate {
    param([int]$PidToKill)
    try {
        $proc = Get-CimInstance Win32_Process -Filter "ProcessId = $PidToKill" -ErrorAction SilentlyContinue
        if ($proc) {
            $result = Invoke-CimMethod -InputObject $proc -MethodName Terminate -ErrorAction SilentlyContinue
            return ($result -and $result.ReturnValue -eq 0)
        }
    } catch {}
    return $false
}

function Invoke-BridgeShutdownEndpoint {
    param([int]$ShutdownPort)
    try {
        $resp = Invoke-WebRequest -Uri "http://127.0.0.1:$ShutdownPort/admin/shutdown" -Method Post -UseBasicParsing -TimeoutSec 2 -ErrorAction Stop
        return ($resp.StatusCode -ge 200 -and $resp.StatusCode -lt 300)
    } catch {
        return $false
    }
}

# Method 1: Stop using saved PID
if (Test-Path $PidFile) {
    $targetPid = Get-Content $PidFile -ErrorAction SilentlyContinue
    if ($targetPid) {
        Write-Host "Found saved PID: $targetPid" -ForegroundColor Yellow
        try {
            $process = Get-Process -Id $targetPid -ErrorAction Stop
            Stop-Process -Id $targetPid -Force
            Start-Sleep -Milliseconds 300
            if (Get-Process -Id $targetPid -ErrorAction SilentlyContinue) {
                $null = Invoke-TaskKill -PidToKill $targetPid
            }
            Write-Host "? Stopped process $targetPid ($($process.Name))" -ForegroundColor Green
        } catch {
            Write-Host "? Process $targetPid not found (may have already stopped)" -ForegroundColor Gray
        }
        Remove-Item $PidFile -ErrorAction SilentlyContinue
    }
}

# Method 2: Find and stop by port
Write-Host "Checking port $Port..." -ForegroundColor Yellow

$preConnections = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
if ($preConnections) {
    if (Invoke-BridgeShutdownEndpoint -ShutdownPort $Port) {
        Write-Host "? Requested graceful shutdown via /admin/shutdown." -ForegroundColor Green
        Start-Sleep -Milliseconds 800
    }
}

$connections = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
if ($connections) {
    foreach ($conn in $connections) {
        $targetPid = $conn.OwningProcess
        $process = Get-Process -Id $targetPid -ErrorAction SilentlyContinue
        if (-not $process) {
            Write-Host "Port $Port reports PID $targetPid, but no process metadata is accessible." -ForegroundColor Yellow
            if (Invoke-CimTerminate -PidToKill $targetPid) {
                Write-Host "? Terminated process $targetPid via CIM." -ForegroundColor Green
            } elseif (Invoke-TaskKill -PidToKill $targetPid) {
                Write-Host "? Terminated process $targetPid via taskkill." -ForegroundColor Green
            } else {
                Write-Host "? Could not terminate process $targetPid from this shell context." -ForegroundColor Red
            }
            continue
        }
        try {
            Write-Host "Found process on port $Port`: $targetPid ($($process.Name))" -ForegroundColor Yellow
            
            # Try graceful stop first
            Stop-Process -Id $targetPid -ErrorAction SilentlyContinue
            Start-Sleep -Milliseconds 500
            
            # Force if still running
            if (Get-Process -Id $targetPid -ErrorAction SilentlyContinue) {
                Stop-Process -Id $targetPid -Force
            }
            
            Write-Host "? Stopped process $targetPid" -ForegroundColor Green
        } catch {
            Write-Host "? Failed to stop process $targetPid with Stop-Process; trying taskkill..." -ForegroundColor Yellow
            if (Invoke-TaskKill -PidToKill $targetPid) {
                Write-Host "? Stopped process $targetPid via taskkill" -ForegroundColor Green
            } else {
                Write-Host "? Failed to stop process $targetPid" -ForegroundColor Red
            }
        }
    }
} else {
    Write-Host "No process listening on port $Port" -ForegroundColor Gray
}

# Method 3: Kill all Python processes running toolserver
Write-Host "Cleaning up Python toolserver processes..." -ForegroundColor Yellow

$pythonProcesses = Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | Where-Object {
    $_.Name -match '^python(\.exe)?$' -and ($_.CommandLine -match 'toolserver|uvicorn')
}

if ($pythonProcesses) {
    foreach ($proc in $pythonProcesses) {
        $targetPid = [int]$proc.ProcessId
        Write-Host "Stopping Python process: $targetPid" -ForegroundColor Yellow
        Stop-Process -Id $targetPid -Force -ErrorAction SilentlyContinue
        if (Get-Process -Id $targetPid -ErrorAction SilentlyContinue) {
            $null = Invoke-TaskKill -PidToKill $targetPid
        }
    }
    Write-Host "? Cleaned up Python processes" -ForegroundColor Green
} else {
    Write-Host "No Python toolserver processes found" -ForegroundColor Gray
}

# Verify port is free
Start-Sleep -Milliseconds 1000
$stillInUse = $null
$netstatCmd = Get-Command netstat -ErrorAction SilentlyContinue
if ($netstatCmd) {
    $stillInUse = netstat -ano | Select-String ":$Port" | Select-String "LISTENING"
} else {
    $stillInUse = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
}
if ($stillInUse) {
    Write-Host ""
    Write-Host "? WARNING: Port $Port may still be in use" -ForegroundColor Yellow
    Write-Host "You may need to manually close the process from Task Manager" -ForegroundColor Yellow
    
    # Show what's still using it
    $connections = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
    if ($connections) {
        foreach ($conn in $connections) {
            $targetPid = $conn.OwningProcess
            $process = Get-Process -Id $targetPid -ErrorAction SilentlyContinue
            if ($process) {
                Write-Host "  Still running: PID $targetPid - $($process.Name)" -ForegroundColor Red
            } else {
                Write-Host "  Listener reports PID $targetPid, but process details are unavailable." -ForegroundColor Yellow
            }
        }
    }
} else {
    Write-Host ""
    Write-Host "? Port $Port is now free" -ForegroundColor Green
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  Cleanup Complete" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

if (-not $NonInteractive) {
    Write-Host "Press any key to exit..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
}
