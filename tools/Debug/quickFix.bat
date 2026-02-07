@echo off
echo ========================================
echo  Quick Fix - Clean AI Bridge State
echo ========================================
echo.

echo [1/4] Killing all Python processes...
taskkill /F /IM python.exe 2>nul
if %ERRORLEVEL% EQU 0 (
    echo   ? Python processes terminated
) else (
    echo   ? No Python processes found
)

echo [2/4] Killing visualizer if running...
taskkill /F /IM RogueCityVisualizerGui.exe 2>nul
if %ERRORLEVEL% EQU 0 (
    echo   ? Visualizer terminated
) else (
    echo   ? Visualizer not running
)

echo [3/4] Waiting for cleanup...
timeout /t 2 >nul

echo [4/4] Checking port 7077...
netstat -an | findstr :7077 >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ? WARNING: Port 7077 still in use!
    echo.
    echo   Finding process...
    for /f "tokens=5" %%p in ('netstat -ano ^| findstr :7077 ^| findstr LISTENING') do (
        echo   Process ID: %%p
        tasklist /FI "PID eq %%p" /FO TABLE
    )
    echo.
    echo   Please close this process manually from Task Manager
) else (
    echo   ? Port 7077 is FREE
)

echo.
echo ========================================
echo  Ready to Test!
echo ========================================
echo.
echo Option 1 (Recommended): Start toolserver manually
echo   Command: tools\Debug\runMockFixed.bat
echo   Then: bin\RogueCityVisualizerGui.exe
echo.
echo Option 2: Use app's Start AI Bridge button
echo   Command: bin\RogueCityVisualizerGui.exe
echo   Then: Click "Start AI Bridge" in AI Console
echo.
pause