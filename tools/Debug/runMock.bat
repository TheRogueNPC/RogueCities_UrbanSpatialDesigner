@echo off
REM Run RogueCity toolserver in MOCK mode (no AI required)
REM Fast testing without Ollama dependency

setlocal

echo ========================================
echo  RogueCity AI Toolserver - MOCK MODE
echo ========================================
echo.

REM Enable mock mode
set ROGUECITY_TOOLSERVER_MOCK=1

echo Starting toolserver in MOCK mode...
echo No AI/Ollama required - returns sample responses
echo.
echo Endpoints available:
echo   GET  http://127.0.0.1:7077/health
echo   POST http://127.0.0.1:7077/ui_agent          (mock)
echo   POST http://127.0.0.1:7077/city_spec         (mock) 
echo   POST http://127.0.0.1:7077/ui_design_assistant (mock)
echo.

REM Check dependencies first
echo Checking dependencies...

REM Check if Python is available
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ✗ ERROR: Python not found in PATH
    echo Please install Python or add it to PATH
    echo.
    goto :error_exit
)
echo ✓ Python found

REM Check if uvicorn is installed
python -c "import uvicorn" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ✗ ERROR: uvicorn not installed
    echo Please install: pip install uvicorn
    echo.
    goto :error_exit
)
echo ✓ uvicorn found

REM Check if FastAPI is installed
python -c "import fastapi" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ✗ ERROR: FastAPI not installed
    echo Please install: pip install fastapi
    echo.
    goto :error_exit
)
echo ✓ FastAPI found

REM Check if port 7077 is already in use
netstat -an | findstr :7077 >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✗ WARNING: Port 7077 appears to be in use
    echo You may need to run stopServer.bat first
    echo.
    echo Press any key to continue anyway...
    pause >nul
)

echo Press any key to start the server...
pause >nul
echo.

echo Starting FastAPI server...
echo Command: python -m uvicorn tools.toolserver:app --host 127.0.0.1 --port 7077 --reload
echo.

python -m uvicorn tools.toolserver:app --host 127.0.0.1 --port 7077 --reload

REM If we get here, the server stopped
echo.
echo Server stopped.
goto :normal_exit

:error_exit
echo.
echo ========================================
echo  SETUP FAILED - See errors above
echo ========================================
echo.
echo Quick fix commands:
echo   pip install fastapi uvicorn httpx pydantic
echo.
pause
exit /b 1

:normal_exit
echo.
echo Press any key to exit...
pause >nul

endlocal