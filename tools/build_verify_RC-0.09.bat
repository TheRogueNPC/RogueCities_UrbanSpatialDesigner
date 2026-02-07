@echo off
REM Build verification script for RC-0.09-Test
REM Tests all major components

echo ========================================
echo  RC-0.09-Test Build Verification
echo ========================================
echo.

echo [1/5] Configuring CMake...
cmake -B build -S . -G Ninja -DBUILD_SHARED_LIBS=OFF -DROGUECITY_BUILD_VISUALIZER=ON
if %ERRORLEVEL% NEQ 0 (
    echo ? CMake configuration failed!
    goto :error
)
echo ? CMake configured
echo.

echo [2/5] Building Core...
cmake --build build --target RogueCityCore --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ? Core build failed!
    goto :error
)
echo ? Core built
echo.

echo [3/5] Building Generators...
cmake --build build --target RogueCityGenerators --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ? Generators build failed!
    goto :error
)
echo ? Generators built
echo.

echo [4/5] Building AI...
cmake --build build --target RogueCityAI --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ? AI build failed!
    goto :error
)
echo ? AI built
echo.

echo [5/5] Building Visualizer (with new templates)...
cmake --build build --target RogueCityVisualizerGui --config Release 2>&1 | findstr /C:"error" /C:"warning"
if %ERRORLEVEL% EQU 0 (
    echo ? Build has warnings/errors - check output above
) else (
    echo ? Visualizer built successfully
)
echo.

echo ========================================
echo  Build Verification Complete!
echo ========================================
echo.
echo Executable: bin\RogueCityVisualizerGui.exe
echo.
echo Next steps:
echo   1. Run: bin\RogueCityVisualizerGui.exe
echo   2. Test index panels (filtering, sorting, context menus)
echo   3. Verify viewport margins (no clipping)
echo   4. Tag: git tag -a RC-0.09-Test -m "Phase 2 Complete"
echo.
goto :end

:error
echo.
echo ========================================
echo  BUILD FAILED - See errors above
echo ========================================
echo.
echo Common fixes:
echo   - Lua linking: Check Visualizer/CMakeLists.txt has sol2::sol2
echo   - Missing headers: Check all new files are included
echo   - Template errors: Check trait implementations
echo.

:end
pause
