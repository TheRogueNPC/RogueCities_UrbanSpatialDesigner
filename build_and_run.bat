@echo off
echo ===================================
echo Building RogueCity Visualizer GUI
echo ===================================

:: Build in Release mode
cmake --build build_vs --target RogueCityVisualizerGui --config Release --parallel 4

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ===================================
echo Build successful! Launching app...
echo ===================================
echo.

:: Run the executable (output is in bin/ directory)
start bin\RogueCityVisualizerGui.exe

echo Application launched!
pause
