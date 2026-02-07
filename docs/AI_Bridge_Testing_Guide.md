# AI Bridge Testing & Troubleshooting Guide

**Date**: February 7, 2026  
**Status**: Debugging connection issues

---

## Current Issue

**Symptom**: App's "Start AI Bridge" button fails health check  
**Cause**: Unknown process on port 7077 returning `{"detail":"Unauthorized"}`

---

## How to Test Properly

### Step 1: Clean State

```cmd
# Stop any existing toolserver
tools\stopServer.bat

# Verify port is free
netstat -an | findstr :7077
# Should return NOTHING
```

### Step 2: Start Toolserver Manually (Mock Mode)

```cmd
# From repo root:
tools\Debug\runMockFixed.bat

# You should see:
# INFO:     Uvicorn running on http://127.0.0.1:7077
# INFO:     Application startup complete.
```

### Step 3: Test Health Endpoint

```powershell
# In another terminal:
curl http://127.0.0.1:7077/health

# Expected response:
# {"status":"ok","service":"RogueCity AI Bridge"}
```

### Step 4: Launch Visualizer

```cmd
bin\RogueCityVisualizerGui.exe
```

### Step 5: Test AI Console

In the visualizer:
1. Open **"AI Console"** panel
2. Should show:
   - Bridge Status: **Online** (green)
   - No errors
3. If it shows "Failed" - check Step 3 again

---

## Two Different Workflows

### Workflow A: Manual Start (What We've Been Testing)

```
You run runMockFixed.bat ? Toolserver starts ? App detects it ? ? Works
```

**Pros**: Direct control, easy to debug  
**Cons**: Extra step before launching app

### Workflow B: App-Managed Start (What's Broken)

```
App button ? Runs Start_Ai_Bridge.ps1 ? Should start toolserver ? ? Failing
```

**Pros**: One-click solution  
**Cons**: More complex, harder to debug

---

## Why Workflow B Might Be Failing

### 1. **Port Conflict**
- Another process already using port 7077
- The "Unauthorized" response suggests a different server

**Fix**:
```cmd
# Kill everything on port 7077
netstat -ano | findstr :7077
# Find PID, then:
taskkill /PID <PID> /F
```

### 2. **PowerShell Script Path Issues**
- Script might not be finding the repo toolserver
- Falls back to generating own toolserver in `.run/`

**Check**:
```powershell
# Run this to see which toolserver is being used:
$Root = "D:\Projects\RogueCities\RogueCities_UrbanSpatialDesigner"
$RepoToolServer = Join-Path $Root "tools\toolserver.py"
Test-Path $RepoToolServer
# Should return: True
```

### 3. **Mock Mode Not Set**
- App's PowerShell script doesn't set `ROGUECITY_TOOLSERVER_MOCK=1`
- Tries to connect to Ollama (which may not be running)

**Fix**: Edit `tools\Start_Ai_Bridge.ps1` line ~140 to add:
```powershell
$env:ROGUECITY_TOOLSERVER_MOCK = "1"
```

---

## Recommended Test Sequence

### Test 1: Verify Clean Slate
```cmd
# 1. Stop everything
taskkill /F /IM python.exe
taskkill /F /IM RogueCityVisualizerGui.exe

# 2. Verify port free
netstat -an | findstr :7077
# Should be empty
```

### Test 2: Manual Mock Mode
```cmd
# 1. Start toolserver
tools\Debug\runMockFixed.bat

# 2. Test health (in another terminal)
curl http://127.0.0.1:7077/health
# Should return: {"status":"ok","service":"RogueCity AI Bridge"}

# 3. Start visualizer
bin\RogueCityVisualizerGui.exe

# 4. Check AI Console
# Should show: Bridge Status: Online
```

### Test 3: App-Managed Start
```cmd
# 1. Ensure nothing running
# (same as Test 1)

# 2. Start visualizer FIRST
bin\RogueCityVisualizerGui.exe

# 3. Click "Start AI Bridge" in AI Console
# Should start toolserver automatically

# 4. Check status
# Should show: Bridge Status: Starting... then Online
```

---

## Current Status

**What's Working**:
- ? Toolserver code (fixed duplicate code)
- ? Manual start with `runMockFixed.bat`
- ? Health endpoint responds correctly
- ? Mock mode returns proper responses

**What's NOT Working**:
- ? App's "Start AI Bridge" button
- ? Unknown process on port 7077 blocking
- ? Health check failing when started from app

**Next Steps**:
1. Kill the unknown process on port 7077
2. Test Workflow A (manual) thoroughly
3. Debug Workflow B (app-managed) once A works
4. Update Start_Ai_Bridge.ps1 if needed

---

## Debug Commands

```powershell
# Check what's on port 7077
netstat -ano | findstr :7077

# Test health manually
Invoke-WebRequest -Uri "http://127.0.0.1:7077/health" -UseBasicParsing

# Check if mock mode is enabled
$env:ROGUECITY_TOOLSERVER_MOCK

# View running Python processes
Get-Process python

# Check toolserver.py for syntax errors
python -m py_compile tools\toolserver.py
```

---

## Quick Fix Script

Save as `tools\Debug\quickFix.bat`:

```cmd
@echo off
echo Killing all Python and Visualizer processes...
taskkill /F /IM python.exe 2>nul
taskkill /F /IM RogueCityVisualizerGui.exe 2>nul

echo Waiting 2 seconds...
timeout /t 2 >nul

echo Checking port 7077...
netstat -an | findstr :7077
if %ERRORLEVEL% EQU 0 (
    echo WARNING: Port 7077 still in use!
    echo Please close the blocking process manually
    pause
) else (
    echo Port 7077 is free
)

echo.
echo Ready to test! Run one of:
echo   1. tools\Debug\runMockFixed.bat
echo   2. bin\RogueCityVisualizerGui.exe
pause
```

---

**Summary**: The toolserver code is fixed and works when started manually. The issue is with how the app's button starts it, likely due to port conflicts or the PowerShell script not finding the right toolserver.py file.
