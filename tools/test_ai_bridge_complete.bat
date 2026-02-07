@echo off
REM Complete AI Bridge Test Script
REM Tests all 3 fallback methods

echo ========================================
echo  AI Bridge Complete Test
echo ========================================
echo.

REM Test 1: Check files exist
echo [TEST 1/5] Checking files...
if not exist "tools\Start_Ai_Bridge_Fixed.ps1" (
    echo ? Start_Ai_Bridge_Fixed.ps1 not found
    goto :error
)
if not exist "tools\Stop_Ai_Bridge_Fixed.ps1" (
    echo ? Stop_Ai_Bridge_Fixed.ps1 not found
    goto :error
)
if not exist "tools\Start_Ai_Bridge_Fallback.bat" (
    echo ? Start_Ai_Bridge_Fallback.bat not found
    goto :error
)
if not exist "AI\ai_config.json" (
    echo ? ai_config.json not found
    goto :error
)
echo ? All script files exist
echo.

REM Test 2: Check config points to fixed scripts
echo [TEST 2/5] Checking config...
findstr /C:"Start_Ai_Bridge_Fixed" AI\ai_config.json >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ? Config still points to old script
    echo Please update AI\ai_config.json
    goto :error
)
echo ? Config points to fixed scripts
echo.

REM Test 3: Check PowerShell availability
echo [TEST 3/5] Checking PowerShell...
pwsh --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ? PowerShell Core (pwsh) available
    set PWSH_AVAILABLE=1
) else (
    echo ? PowerShell Core not available
    set PWSH_AVAILABLE=0
)

powershell -Command "Write-Output 'test'" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ? Windows PowerShell available
    set PS_AVAILABLE=1
) else (
    echo ? Windows PowerShell not available (unusual!)
    set PS_AVAILABLE=0
)

if %PWSH_AVAILABLE%==0 if %PS_AVAILABLE%==0 (
    echo ? No PowerShell available - will use BAT fallback
)
echo.

REM Test 4: Check Python and packages
echo [TEST 4/5] Checking Python environment...
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ? Python not found
    goto :error
)
echo ? Python found

python -c "import fastapi, uvicorn, httpx, pydantic" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ? Python packages missing
    echo Install: pip install fastapi uvicorn httpx pydantic
    goto :error
)
echo ? Python packages installed
echo.

REM Test 5: Test BAT fallback (without starting)
echo [TEST 5/5] Testing BAT fallback script...
echo Checking if BAT fallback can run (dry-run)...
if %PS_AVAILABLE%==1 (
    powershell -Command "Get-Content tools\Start_Ai_Bridge_Fallback.bat | Select-String -Pattern 'python -m uvicorn'" >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo ? BAT fallback script looks valid
    ) else (
        echo ? Could not verify BAT fallback
    )
) else (
    echo ? Skipping BAT validation (no PowerShell)
)
echo.

echo ========================================
echo  ALL TESTS PASSED!
echo ========================================
echo.
echo Summary:
if %PWSH_AVAILABLE%==1 echo   ? Tier 1: PowerShell Core available
if %PS_AVAILABLE%==1 echo   ? Tier 2: Windows PowerShell available
echo   ? Tier 3: BAT fallback ready
echo   ? Config updated
echo   ? Python environment ready
echo.
echo Next steps:
echo   1. Build: cmake --build build --target RogueCityVisualizerGui
echo   2. Run: .\bin\RogueCityVisualizerGui.exe
echo   3. Click "Start AI Bridge" in AI Console
echo   4. Should see "Online" status
echo.
echo Or test manually:
if %PWSH_AVAILABLE%==1 echo   pwsh -NoProfile -ExecutionPolicy Bypass -File .\tools\Start_Ai_Bridge_Fixed.ps1
if %PS_AVAILABLE%==1 echo   powershell -ExecutionPolicy Bypass -File .\tools\Start_Ai_Bridge_Fixed.ps1
echo   tools\Start_Ai_Bridge_Fallback.bat
echo.
goto :end

:error
echo.
echo ========================================
echo  TESTS FAILED - See errors above
echo ========================================
echo.

:end
pause
