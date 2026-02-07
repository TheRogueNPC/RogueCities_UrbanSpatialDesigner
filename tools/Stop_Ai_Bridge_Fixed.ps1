# Stop_Ai_Bridge_Fixed.ps1
# Reliably stops the RogueCity AI Bridge toolserver

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  RogueCity AI Bridge - Stop" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$Port = 7077
$PidFile = Join-Path $PSScriptRoot ".run\toolserver.pid"

# Method 1: Stop using saved PID
if (Test-Path $PidFile) {
    $pid = Get-Content $PidFile -ErrorAction SilentlyContinue
    if ($pid) {
        Write-Host "Found saved PID: $pid" -ForegroundColor Yellow
        try {
            $process = Get-Process -Id $pid -ErrorAction Stop
            Stop-Process -Id $pid -Force
            Write-Host "? Stopped process $pid ($($process.Name))" -ForegroundColor Green
        } catch {
            Write-Host "? Process $pid not found (may have already stopped)" -ForegroundColor Gray
        }
        Remove-Item $PidFile -ErrorAction SilentlyContinue
    }
}

# Method 2: Find and stop by port
Write-Host "Checking port $Port..." -ForegroundColor Yellow

$connections = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
if ($connections) {
    foreach ($conn in $connections) {
        $pid = $conn.OwningProcess
        try {
            $process = Get-Process -Id $pid -ErrorAction Stop
            Write-Host "Found process on port $Port`: $pid ($($process.Name))" -ForegroundColor Yellow
            
            # Try graceful stop first
            Stop-Process -Id $pid -ErrorAction SilentlyContinue
            Start-Sleep -Milliseconds 500
            
            # Force if still running
            if (Get-Process -Id $pid -ErrorAction SilentlyContinue) {
                Stop-Process -Id $pid -Force
            }
            
            Write-Host "? Stopped process $pid" -ForegroundColor Green
        } catch {
            Write-Host "? Failed to stop process $pid" -ForegroundColor Red
        }
    }
} else {
    Write-Host "No process listening on port $Port" -ForegroundColor Gray
}

# Method 3: Kill all Python processes running toolserver
Write-Host "Cleaning up Python toolserver processes..." -ForegroundColor Yellow

$pythonProcesses = Get-Process python -ErrorAction SilentlyContinue | Where-Object {
    $_.CommandLine -like "*toolserver*" -or $_.CommandLine -like "*uvicorn*"
}

if ($pythonProcesses) {
    foreach ($proc in $pythonProcesses) {
        Write-Host "Stopping Python process: $($proc.Id)" -ForegroundColor Yellow
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    }
    Write-Host "? Cleaned up Python processes" -ForegroundColor Green
} else {
    Write-Host "No Python toolserver processes found" -ForegroundColor Gray
}

# Verify port is free
Start-Sleep -Milliseconds 1000
$stillInUse = netstat -ano | Select-String ":$Port" | Select-String "LISTENING"
if ($stillInUse) {
    Write-Host ""
    Write-Host "? WARNING: Port $Port may still be in use" -ForegroundColor Yellow
    Write-Host "You may need to manually close the process from Task Manager" -ForegroundColor Yellow
    
    # Show what's still using it
    $connections = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
    if ($connections) {
        foreach ($conn in $connections) {
            $pid = $conn.OwningProcess
            $process = Get-Process -Id $pid -ErrorAction SilentlyContinue
            if ($process) {
                Write-Host "  Still running: PID $pid - $($process.Name)" -ForegroundColor Red
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

Write-Host "Press any key to exit..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")