@echo off
REM Simple fix for runMock.bat to work from Debug subdirectory
echo Fixing runMock.bat to work from tools/Debug directory...

REM Change to repository root (two levels up from tools/Debug)
cd /d "%~dp0\..\.."

echo Working directory changed to: %CD%
echo.

REM Set mock mode
set ROGUECITY_TOOLSERVER_MOCK=1

echo Starting toolserver in MOCK mode...
echo.

REM Check if we can find the toolserver
if not exist "tools\toolserver.py" (
    echo ERROR: Cannot find tools\toolserver.py
    echo Current directory: %CD%
    pause
    exit /b 1
)

echo Found toolserver at: %CD%\tools\toolserver.py
echo.
echo Starting server...
python -m uvicorn tools.toolserver:app --host 127.0.0.1 --port 7077 --reload

pause