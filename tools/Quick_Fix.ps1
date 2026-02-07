# Quick_Fix.ps1
# Emergency cleanup and diagnostic tool for AI Bridge

param(
    [switch]$Force,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  AI Bridge Quick Fix" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$Port = 7077
$fixes = 0
$warnings = 0

# Fix 1: Kill all Python processes
Write-Host "[1/5] Checking Python processes..." -ForegroundColor Yellow
$pythonProcs = Get-Process python -ErrorAction SilentlyContinue
if ($pythonProcs) {
    Write-Host "  Found $($pythonProcs.Count) Python process(es)" -ForegroundColor Yellow
    
    if ($Force) {
        foreach ($proc in $pythonProcs) {
            if ($Verbose) {
                Write-Host "    Killing PID $($proc.Id): $($proc.Path)" -ForegroundColor Gray
            }
            Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        }
        Write-Host "  ? Terminated all Python processes" -ForegroundColor Green
        $fixes++
    } else {
        Write-Host "  ? Use -Force to kill Python processes" -ForegroundColor Cyan
    }
} else {
    Write-Host "  ? No Python processes running" -ForegroundColor Green
}

# Fix 2: Kill visualizer
Write-Host "[2/5] Checking visualizer..." -ForegroundColor Yellow
$vizProc = Get-Process RogueCityVisualizerGui -ErrorAction SilentlyContinue
if ($vizProc) {
    Write-Host "  Found visualizer (PID: $($vizProc.Id))" -ForegroundColor Yellow
    
    if ($Force) {
        Stop-Process -Id $vizProc.Id -Force -ErrorAction SilentlyContinue
        Write-Host "  ? Terminated visualizer" -ForegroundColor Green
        $fixes++
    } else {
        Write-Host "  ? Use -Force to kill visualizer" -ForegroundColor Cyan
    }
} else {
    Write-Host "  ? Visualizer not running" -ForegroundColor Green
}

# Fix 3: Check port 7077
Write-Host "[3/5] Checking port $Port..." -ForegroundColor Yellow
$connections = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
if ($connections) {
    foreach ($conn in $connections) {
        $pid = $conn.OwningProcess
        $process = Get-Process -Id $pid -ErrorAction SilentlyContinue
        Write-Host "  ? Port in use by PID $pid ($($process.Name))" -ForegroundColor Yellow
        $warnings++
        
        if ($Force) {
            Stop-Process -Id $pid -Force -ErrorAction SilentlyContinue
            Write-Host "    ? Killed process $pid" -ForegroundColor Green
            $fixes++
        }
    }
    
    if (-not $Force) {
        Write-Host "  ? Use -Force to kill processes on port $Port" -ForegroundColor Cyan
    }
} else {
    Write-Host "  ? Port $Port is free" -ForegroundColor Green
}

# Fix 4: Clean .run directory
Write-Host "[4/5] Checking .run directory..." -ForegroundColor Yellow
$runDir = Join-Path $PSScriptRoot ".run"
if (Test-Path $runDir) {
    $pidFiles = Get-ChildItem $runDir -Filter "*.pid"
    if ($pidFiles) {
        Write-Host "  Found $($pidFiles.Count) stale PID file(s)" -ForegroundColor Yellow
        
        if ($Force) {
            Remove-Item "$runDir\*.pid" -Force -ErrorAction SilentlyContinue
            Write-Host "  ? Cleaned PID files" -ForegroundColor Green
            $fixes++
        } else {
            Write-Host "  ? Use -Force to clean PID files" -ForegroundColor Cyan
        }
    } else {
        Write-Host "  ? No stale PID files" -ForegroundColor Green
    }
} else {
    Write-Host "  ? .run directory doesn't exist" -ForegroundColor Gray
}

# Fix 5: Verify toolserver.py exists
Write-Host "[5/5] Checking toolserver.py..." -ForegroundColor Yellow
$repoRoot = Split-Path -Parent $PSScriptRoot
$toolserverPath = Join-Path $repoRoot "tools\toolserver.py"
if (Test-Path $toolserverPath) {
    Write-Host "  ? toolserver.py found" -ForegroundColor Green
    
    # Check for syntax errors
    if ($Verbose) {
        Write-Host "  Checking Python syntax..." -ForegroundColor Gray
        python -m py_compile $toolserverPath 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  ? No syntax errors" -ForegroundColor Green
        } else {
            Write-Host "  ? Syntax errors detected!" -ForegroundColor Red
            $warnings++
        }
    }
} else {
    Write-Host "  ? toolserver.py NOT FOUND at: $toolserverPath" -ForegroundColor Red
    $warnings++
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if ($Force) {
    Write-Host "Fixes applied: $fixes" -ForegroundColor Green
} else {
    Write-Host "Fixes available: Run with -Force to apply" -ForegroundColor Yellow
}

if ($warnings -gt 0) {
    Write-Host "Warnings: $warnings" -ForegroundColor Yellow
} else {
    Write-Host "No warnings" -ForegroundColor Green
}

Write-Host ""

# Wait a moment for things to settle
if ($Force) {
    Write-Host "Waiting for cleanup to complete..." -ForegroundColor Gray
    Start-Sleep -Seconds 2
    
    # Final verification
    $finalCheck = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
    if ($finalCheck) {
        Write-Host "? Port $Port is still in use" -ForegroundColor Yellow
        Write-Host "Manual intervention may be required" -ForegroundColor Yellow
    } else {
        Write-Host "? All clean! Ready to start toolserver" -ForegroundColor Green
        Write-Host ""
        Write-Host "Next steps:" -ForegroundColor Cyan
        Write-Host "  1. Run: .\tools\Start_Ai_Bridge_Fixed.ps1" -ForegroundColor White
        Write-Host "  2. Launch: .\bin\RogueCityVisualizerGui.exe" -ForegroundColor White
    }
}

Write-Host ""
Write-Host "Press any key to exit..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")