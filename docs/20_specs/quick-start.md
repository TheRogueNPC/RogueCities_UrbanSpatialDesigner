# Quick Start Guide - Fixed AI Bridge

**Updated**: February 7, 2026  
**For**: RogueCity Visualizer R0.2

---

## ?? Quick Start (3 Steps)

### Step 1: Clean Everything
```powershell
.\tools\Quick_Fix.ps1 -Force
```

### Step 2: Start Toolserver
```powershell
.\tools\Start_Ai_Bridge_Fixed.ps1
```

### Step 3: Launch Visualizer
```powershell
.\bin\RogueCityVisualizerGui.exe
```

**Expected Result**: AI Console shows "Bridge Status: Online" ?

---

## ?? Troubleshooting

### Problem: "Port 7077 in use"
```powershell
.\tools\Stop_Ai_Bridge_Fixed.ps1
.\tools\Quick_Fix.ps1 -Force
# Then restart
```

### Problem: "Health check failed"
```powershell
# Check if toolserver is actually running
curl http://127.0.0.1:7077/health

# Should return: {"status":"ok","service":"RogueCity AI Bridge"}
```

### Problem: "Python not found"
```powershell
# Install Python 3.10+, then:
pip install fastapi uvicorn httpx pydantic
```

---

## ?? File Structure

```
tools/
??? Start_Ai_Bridge_Fixed.ps1  ? Use this (not old Start_Ai_Bridge.ps1)
??? Stop_Ai_Bridge_Fixed.ps1   ? Use this (not old Stop_Ai_Bridge.ps1)
??? Quick_Fix.ps1               ? Emergency cleanup
??? toolserver.py               ? The actual server
??? Debug/
    ??? runMockFixed.bat        ? Batch alternative
    ??? quickFix.bat            ? Batch cleanup
    ??? testAiBridge.bat        ? Diagnostic tool
```

---

## ?? What's Working

- ? Toolserver starts reliably
- ? Mock mode (no AI needed)
- ? Live mode (with Ollama)
- ? Health checks pass
- ? UI Agent generates commands
- ? CitySpec generates city designs
- ? Design Assistant finds refactoring opportunities

---

## ?? Next Phase

**Phase 2**: Implement AI's refactoring suggestions
- Extract `RcDataIndexPanel<T>` template
- Reduce code duplication by 80%
- Apply to 3 panels (roads, districts, lots)

See: `docs/R0.2_Phase1_Complete_Phase2_Plan.md`

---

## ?? Emergency Commands

### Nuclear Option (kills everything)
```powershell
taskkill /F /IM python.exe
taskkill /F /IM RogueCityVisualizerGui.exe
```

### Check What's Running
```powershell
# Check port
netstat -an | findstr :7077

# Check Python
Get-Process python

# Check visualizer
Get-Process RogueCityVisualizerGui
```

---

**All systems ready!** Ready for Phase 2 implementation. ??
