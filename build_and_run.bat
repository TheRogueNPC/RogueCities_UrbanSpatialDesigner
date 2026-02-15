@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo ===================================
echo Building RogueCity Visualizer GUI
echo ===================================

set "ROOT=%~dp0"
pushd "%ROOT%" >nul || (
    echo Failed to enter repo root.
    exit /b 1
)

set "SLN=%ROOT%build_vs\RogueCities.sln"
set "MSBUILD_EXE="

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do set "VSINSTALL=%%I"
)

if defined VSINSTALL (
    if exist "%VSINSTALL%\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD_EXE=%VSINSTALL%\MSBuild\Current\Bin\MSBuild.exe"
    if not defined MSBUILD_EXE if exist "%VSINSTALL%\MSBuild\15.0\Bin\MSBuild.exe" set "MSBUILD_EXE=%VSINSTALL%\MSBuild\15.0\Bin\MSBuild.exe"
)

if not defined MSBUILD_EXE (
    where msbuild >nul 2>nul && set "MSBUILD_EXE=msbuild"
)

if defined MSBUILD_EXE if exist "%SLN%" (
    echo Using solution integration: "%SLN%"
    "%MSBUILD_EXE%" "%SLN%" /m /t:RogueCityVisualizerGui /p:Configuration=Release /p:Platform=x64
) else (
    echo Solution build unavailable, falling back to CMake.
    cmake --build build_vs --target RogueCityVisualizerGui --config Release --parallel 4
)

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    popd >nul
    exit /b 1
)

set "VIS_EXE=%ROOT%bin\RogueCityVisualizerGui.exe"
set "VIS_SHORTCUT=%ROOT%RogueCityVisualizerGui.lnk"

if exist "%VIS_EXE%" (
    where powershell >nul 2>nul
    if %ERRORLEVEL% EQU 0 (
        powershell -NoProfile -ExecutionPolicy Bypass -Command "$W=New-Object -ComObject WScript.Shell; $S=$W.CreateShortcut('%VIS_SHORTCUT%'); $S.TargetPath='%VIS_EXE%'; $S.WorkingDirectory='%ROOT%'; $S.IconLocation='%VIS_EXE%,0'; $S.Description='RogueCity Visualizer GUI'; $S.Save()"
        if %ERRORLEVEL% EQU 0 (
            echo Shortcut updated: "%VIS_SHORTCUT%"
        ) else (
            echo WARNING: Failed to update shortcut "%VIS_SHORTCUT%".
        )
    ) else (
        echo WARNING: PowerShell not found; could not create shortcut "%VIS_SHORTCUT%".
    )
) else (
    echo WARNING: Expected executable not found: "%VIS_EXE%"
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
popd >nul
