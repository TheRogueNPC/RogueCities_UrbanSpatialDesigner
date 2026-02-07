@echo off
REM Fallback start script - works without PowerShell
REM Uses Python directly to start toolserver

setlocal

echo ========================================
echo  AI Bridge Fallback Start (No PS)
echo ========================================
echo.

REM Set mock mode
set ROGUECITY_TOOLSERVER_MOCK=1

echo [1/3] Checking Python...
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ? Python not found in PATH
    goto :error
)
echo ? Python found

echo [2/3] Checking packages...
python -c "import fastapi, uvicorn, httpx, pydantic" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ? Missing packages
    echo Install with: pip install fastapi uvicorn httpx pydantic
    goto :error
)
echo ? Packages installed

echo [3/3] Starting toolserver...
echo.
echo Toolserver starting in background (Mock mode)
echo Port: 7077
echo.

REM Start toolserver in background
start /B python -m uvicorn tools.toolserver:app --host 127.0.0.1 --port 7077 --reload >nul 2>&1

REM Wait a moment for startup
timeout /t 3 /nobreak >nul

echo ? Toolserver started
echo.
echo Endpoints:
echo   http://127.0.0.1:7077/health
echo   http://127.0.0.1:7077/ui_agent
echo   http://127.0.0.1:7077/city_spec
echo.
echo To stop: tools\stopServer.bat
echo.
goto :end

:error
echo.
echo ========================================
echo  START FAILED
echo ========================================
echo.
pause
exit /b 1

:end
REM Don't pause - let it return to GUI
exit /b 0
