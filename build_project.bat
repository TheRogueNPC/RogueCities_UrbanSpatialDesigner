@echo off
setlocal

echo ==================================================
echo RogueCities Build Script (Robust vcpkg integration)
echo ==================================================

:: 1. Check if vcpkg needs bootstrapping
if not exist "3rdparty\vcpkg\vcpkg.exe" (
    echo [Build] Bootstrapping vcpkg...
    cd 3rdparty\vcpkg
    call bootstrap-vcpkg.bat
    cd ..\..
) else (
    echo [Build] vcpkg executable found.
)

:: 2. Install dependencies via vcpkg
echo [Build] Installing dependencies via vcpkg...
3rdparty\vcpkg\vcpkg.exe install clipper2 fastnoise2 manifold recastnavigation tinygltf gdal
if %ERRORLEVEL% neq 0 (
    echo [Error] vcpkg installation failed!
    exit /b %ERRORLEVEL%
)

:: 3. Configure CMake with the preset (which now forces the toolchain file)
echo [Build] Configuring CMake...
cmake --preset=default -S . -B build_vs
if %ERRORLEVEL% neq 0 (
    echo [Error] CMake configuration failed!
    exit /b %ERRORLEVEL%
)

:: 4. Build the project
echo [Build] Compiling project...
cmake --build build_vs --config Debug
if %ERRORLEVEL% neq 0 (
    echo [Error] Compilation failed!
    exit /b %ERRORLEVEL%
)

echo ==================================================
echo [Build] Success!
echo ==================================================
endlocal
exit /b 0
