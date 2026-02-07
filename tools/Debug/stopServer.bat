@echo off
REM Stop RogueCity toolserver (covers both Mock and Live modes)
REM Finds and terminates any Python process running on port 7077

setlocal enabledelayedexpansion

echo ========================================
echo  RogueCity AI Toolserver - STOP
echo ========================================
echo.

echo Stopping toolserver on port 7077...

REM Find process using port 7077
set PID=
for /f "tokens=5" %%p in ('netstat -ano 2^>nul ^| findstr :7077 ^| findstr LISTENING') do (
    set PID=%%p
    goto :found_process
)

:found_process
if "!PID!"=="" (
    echo No process found listening on port 7077
    echo Toolserver may already be stopped
    goto :cleanup
)

echo Found process ID: !PID!

REM Get process name for verification
for /f "tokens=1" %%n in ('tasklist /FI "PID eq !PID!" /FO CSV /NH 2^>nul') do (
    set PROCESS_NAME=%%n
    set PROCESS_NAME=!PROCESS_NAME:"=!
)

echo Process name: !PROCESS_NAME!

REM Try multiple termination methods
echo Attempting graceful termination...
taskkill /PID !PID! /T >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ? Process terminated gracefully
    goto :cleanup
)

echo Graceful termination failed, trying force kill...
taskkill /PID !PID! /F /T >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ? Process force-killed successfully
    goto :cleanup
)

echo Force kill failed, trying alternative method...
wmic process where ProcessId=!PID! delete >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ? Process terminated via WMI
    goto :cleanup
)

echo ? All termination methods failed
echo Please manually close the process from Task Manager:
echo   Process ID: !PID!
echo   Process Name: !PROCESS_NAME!
echo.
echo Or try running this script as Administrator

:cleanup
echo.
echo Cleaning up any remaining Python/uvicorn processes...

REM Kill by process name (more reliable)
taskkill /F /IM python.exe /FI "COMMANDLINE eq *uvicorn*" >nul 2>&1
taskkill /F /IM python.exe /FI "COMMANDLINE eq *toolserver*" >nul 2>&1

REM Kill any process using our specific port (nuclear option)
for /f "tokens=5" %%p in ('netstat -ano 2^>nul ^| findstr :7077') do (
    echo Cleaning up remaining process: %%p
    taskkill /PID %%p /F >nul 2>&1
)

echo Cleanup complete.

REM Verify port is free
timeout /t 2 >nul
netstat -an | findstr :7077 >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ??  Port 7077 may still be in use
    echo Wait a few seconds and try again
) else (
    echo ? Port 7077 is now free
)

:end
echo.
echo Press any key to exit...
pause >nul

endlocal