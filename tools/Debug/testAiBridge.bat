@echo off
REM Comprehensive test of AI Bridge integration
REM Tests both manual toolserver and app's Start AI Bridge

echo ========================================
echo  AI Bridge Integration Test
echo ========================================
echo.

REM Test 1: Check files exist
echo [TEST 1] Checking files...
if exist "tools\toolserver.py" (
    echo   ? tools\toolserver.py exists
) else (
    echo   ? tools\toolserver.py NOT FOUND
    goto :error
)

if exist "tools\Start_Ai_Bridge.ps1" (
    echo   ? tools\Start_Ai_Bridge.ps1 exists
) else (
    echo   ? tools\Start_Ai_Bridge.ps1 NOT FOUND
    goto :error
)

if exist "bin\RogueCityVisualizerGui.exe" (
    echo   ? RogueCityVisualizerGui.exe exists
) else (
    echo   ? RogueCityVisualizerGui.exe NOT FOUND
    goto :error
)

echo.

REM Test 2: Check if toolserver is already running
echo [TEST 2] Checking if toolserver is running...
netstat -an | findstr :7077 >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ? Toolserver is running on port 7077
    set TOOLSERVER_RUNNING=1
) else (
    echo   ? Toolserver is NOT running
    set TOOLSERVER_RUNNING=0
)
echo.

REM Test 3: Test health endpoint if running
if %TOOLSERVER_RUNNING%==1 (
    echo [TEST 3] Testing health endpoint...
    curl -s http://127.0.0.1:7077/health >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo   ? Health endpoint responding
        curl -s http://127.0.0.1:7077/health
    ) else (
        echo   ? Health endpoint not responding
    )
    echo.
) else (
    echo [TEST 3] Skipped - toolserver not running
    echo.
)

REM Test 4: Check Python dependencies
echo [TEST 4] Checking Python dependencies...
python --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ? Python found
) else (
    echo   ? Python NOT found
    goto :error
)

python -c "import fastapi, uvicorn, httpx, pydantic" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ? Python packages installed
) else (
    echo   ? Python packages missing
    echo   Run: pip install fastapi uvicorn httpx pydantic
    goto :error
)
echo.

REM Test 5: Check PowerShell
echo [TEST 5] Checking PowerShell...
pwsh --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ? PowerShell 7 (pwsh) found
) else (
    powershell -Command "Write-Output 'test'" >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo   ? PowerShell 5.1 found
    ) else (
        echo   ? No PowerShell found
        goto :error
    )
)
echo.

REM Summary
echo ========================================
echo  Test Summary
echo ========================================
echo.

if %TOOLSERVER_RUNNING%==1 (
    echo Status: Toolserver is running and ready
    echo.
    echo Next steps:
    echo   1. Launch visualizer: bin\RogueCityVisualizerGui.exe
    echo   2. Open AI Console panel
    echo   3. Click "Start AI Bridge" - should show "Online"
) else (
    echo Status: Toolserver is NOT running
    echo.
    echo To start toolserver manually:
    echo   Option 1: tools\Debug\runMockFixed.bat  (mock mode)
    echo   Option 2: tools\runMock.bat             (mock mode, needs correct directory)
    echo   Option 3: From app - click "Start AI Bridge" button
)
echo.

goto :end

:error
echo.
echo ========================================
echo  TESTS FAILED - See errors above
echo ========================================
echo.

:end
echo.
pause