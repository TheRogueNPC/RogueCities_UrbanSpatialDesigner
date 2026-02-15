@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem RogueCities_UrbanSatialDesigner - one-click build helper
rem Outputs built .exe files into: <repo>\Executibles

set "ROOT=%~dp0"
pushd "%ROOT%" >nul || (echo ERROR: Failed to cd to "%ROOT%". & exit /b 1)

set "CFG=Release"
set "CLEAN=0"
set "VISUALIZER=auto"
set "WITH_UI_TOOLCHAIN=0"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--debug"        (set "CFG=Debug"   & shift & goto parse_args)
if /I "%~1"=="--release"      (set "CFG=Release" & shift & goto parse_args)
if /I "%~1"=="--clean"        (set "CLEAN=1"     & shift & goto parse_args)
if /I "%~1"=="--visualizer"   (set "VISUALIZER=on"  & shift & goto parse_args)
if /I "%~1"=="--no-visualizer" (set "VISUALIZER=off" & shift & goto parse_args)
if /I "%~1"=="--with-ui-toolchain" (set "WITH_UI_TOOLCHAIN=1" & shift & goto parse_args)
if /I "%~1"=="-h"  goto help
if /I "%~1"=="--help" goto help

echo ERROR: Unknown argument: %~1
goto help

:help
echo.
echo Usage: StartupBuild.bat [--release ^| --debug] [--visualizer ^| --no-visualizer] [--clean]
echo.
echo   --release         Build Release (default)
echo   --debug           Build Debug
echo   --visualizer      Force building visualizer target(s)
echo   --no-visualizer   Force skipping visualizer target(s)
echo   --clean           Delete "build" before configuring
echo   --with-ui-toolchain  Also build test_ui_toolchain.exe (may require Lua/sol2 setup)
echo.
popd >nul
exit /b 2

:args_done

set "IMGUI_HDR=%ROOT%3rdparty\imgui\imgui.h"
if /I "%VISUALIZER%"=="auto" (
  if exist "%IMGUI_HDR%" (set "VISUALIZER=on") else (set "VISUALIZER=off")
)

set "VISUALIZER_CMAKE=OFF"
if /I "%VISUALIZER%"=="on" set "VISUALIZER_CMAKE=ON"

call :SetupVsEnv

call :FindCMake
call :FindMSBuild

if "%CLEAN%"=="1" (
  if exist "%ROOT%build\" (
    echo Cleaning "%ROOT%build"...
    rmdir /s /q "%ROOT%build" >nul 2>nul
  )
)

set "BUILD_DIR=%ROOT%build"
if not exist "%BUILD_DIR%\" mkdir "%BUILD_DIR%" >nul 2>nul

set "TARGETS=test_core test_generators test_editor_hfsm"
if /I "%VISUALIZER%"=="on" (
  set "TARGETS=%TARGETS% RogueCityVisualizerHeadless"
)
if "%WITH_UI_TOOLCHAIN%"=="1" (
  set "TARGETS=%TARGETS% test_ui_toolchain"
)

set "BUILT_OK=0"

if defined CMAKE_EXE (
  echo.
  echo === CMake Configure (CFG=%CFG%, VISUALIZER=%VISUALIZER%^) ===
  "%CMAKE_EXE%" -S "%ROOT%" -B "%BUILD_DIR%" -DROGUECITY_BUILD_VISUALIZER=%VISUALIZER_CMAKE% 1> "%BUILD_DIR%\\configure.log" 2>&1
  if errorlevel 1 (
    echo CMake configure failed. See "%BUILD_DIR%\\configure.log"
    goto fallback
  )

  echo.
  echo === CMake Build (%TARGETS%^) ===
  "%CMAKE_EXE%" --build "%BUILD_DIR%" --config %CFG% --target %TARGETS% 1> "%BUILD_DIR%\\build.log" 2>&1
  if errorlevel 1 (
    echo CMake build failed. See "%BUILD_DIR%\\build.log"
    goto fallback
  )

  set "BUILT_OK=1"
  goto collect
)

:fallback
echo.
echo === Fallback Build (MSBuild) ===
if not defined MSBUILD_EXE (
  echo ERROR: No CMake found and MSBuild not detected.
  goto collect
)

call :PickSolution
if not defined FALLBACK_SLN (
  echo ERROR: No fallback .sln found under build folders.
  goto collect
)

echo Using solution: "%FALLBACK_SLN%"
echo.
for %%T in (%TARGETS%) do (
  echo === MSBuild Target: %%T ===
  "%MSBUILD_EXE%" "%FALLBACK_SLN%" /m /t:%%T /p:Configuration=%CFG% /p:Platform=x64
  if errorlevel 1 goto msbuild_failed
)

set "BUILT_OK=1"
goto collect

:msbuild_failed
echo MSBuild failed.
goto collect

:collect
call :CollectExecutibles
call :UpdateVisualizerShortcut

echo.
if "%BUILT_OK%"=="1" (
  echo Build completed.
  popd >nul
  exit /b 0
)

echo Build did not complete successfully; executables were still collected if any existed.
popd >nul
exit /b 1

rem ----------------------------
rem Helpers
rem ----------------------------

:SetupVsEnv
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 0

for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
if not defined VSINSTALL exit /b 0

set "VSDEVCMD=%VSINSTALL%\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" exit /b 0

call "%VSDEVCMD%" -no_logo -arch=x64 >nul 2>nul
exit /b 0

:FindCMake
set "CMAKE_EXE="

where cmake >nul 2>nul && (set "CMAKE_EXE=cmake" & exit /b 0)

if defined VSINSTALL (
  set "VS_CMAKE=%VSINSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  if exist "!VS_CMAKE!" (set "CMAKE_EXE=!VS_CMAKE!" & exit /b 0)
)

if exist "%ProgramFiles%\CMake\bin\cmake.exe" (set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe" & exit /b 0)
if exist "%ProgramFiles(x86)%\CMake\bin\cmake.exe" (set "CMAKE_EXE=%ProgramFiles(x86)%\CMake\bin\cmake.exe" & exit /b 0)

exit /b 0

:FindMSBuild
set "MSBUILD_EXE="

where msbuild >nul 2>nul && (set "MSBUILD_EXE=msbuild" & exit /b 0)

if defined VSINSTALL (
  if exist "%VSINSTALL%\MSBuild\Current\Bin\MSBuild.exe" (set "MSBUILD_EXE=%VSINSTALL%\MSBuild\Current\Bin\MSBuild.exe" & exit /b 0)
  if exist "%VSINSTALL%\MSBuild\15.0\Bin\MSBuild.exe" (set "MSBUILD_EXE=%VSINSTALL%\MSBuild\15.0\Bin\MSBuild.exe" & exit /b 0)
)

exit /b 0

:PickSolution
set "FALLBACK_SLN="

if exist "%ROOT%build_vs\RogueCities.sln" (set "FALLBACK_SLN=%ROOT%build_vs\RogueCities.sln" & exit /b 0)
if exist "%ROOT%build\RogueCityMVP.sln" (set "FALLBACK_SLN=%ROOT%build\RogueCityMVP.sln" & exit /b 0)
if exist "%ROOT%build_core\RogueCityMVP.sln" (set "FALLBACK_SLN=%ROOT%build_core\RogueCityMVP.sln" & exit /b 0)
if exist "%ROOT%build\core\RogueCityCore.sln" (set "FALLBACK_SLN=%ROOT%build\core\RogueCityCore.sln" & exit /b 0)
if exist "%ROOT%build\generators\RogueCityGenerators.sln" (set "FALLBACK_SLN=%ROOT%build\generators\RogueCityGenerators.sln" & exit /b 0)
if exist "%ROOT%build\visualizer\RogueCityVisualizer.sln" (set "FALLBACK_SLN=%ROOT%build\visualizer\RogueCityVisualizer.sln" & exit /b 0)

exit /b 0

:CollectExecutibles
set "DEST=%ROOT%Executibles"
if not exist "%DEST%\" mkdir "%DEST%" >nul 2>nul
del /q "%DEST%\*.exe" >nul 2>nul

set /a COPIED=0

call :CopyFromDir "%ROOT%build" "%DEST%"
call :CopyFromDir "%ROOT%build_core" "%DEST%"
call :CopyFromDir "%ROOT%build_gui" "%DEST%"
call :CopyFromDir "%ROOT%build_ninja" "%DEST%"
call :CopyFromDir "%ROOT%out" "%DEST%"

echo.
echo Copied %COPIED% exe(s) to: "%DEST%"
exit /b 0

:CopyFromDir
set "SRC=%~1"
set "DESTDIR=%~2"
if not exist "%SRC%\" exit /b 0

for /r "%SRC%" %%F in (*.exe) do (
  set "P=%%~fF"

  echo !P! | findstr /I /C:"\CMakeFiles\" /C:"\CompilerId" /C:"\CMakeCCompilerId" /C:"\CMakeCXXCompilerId" /C:"\Testing\" /C:"\_deps\" /C:"\vcpkg_installed\" >nul
  if not errorlevel 1 (
    rem Skip CMake/tooling outputs
  ) else (
    call :CopyOne "%%~fF" "%DESTDIR%"
  )
)

exit /b 0

:CopyOne
set "EXE=%~1"
set "DESTDIR=%~2"
set "NAME=%~nx1"
set "OUT=%DESTDIR%\%NAME%"

if exist "%OUT%" (
  set "BASE=%~n1"
  set "EXT=%~x1"
  set /a N=2
  :dup_loop
  set "OUT=%DESTDIR%\%BASE%__!N!%EXT%"
  if exist "!OUT!" (set /a N+=1 & goto dup_loop)
)

copy /y "%EXE%" "!OUT!" >nul
if not errorlevel 1 set /a COPIED+=1
exit /b 0

:UpdateVisualizerShortcut
set "VIS_EXE=%ROOT%bin\RogueCityVisualizerGui.exe"
set "VIS_SHORTCUT=%ROOT%RogueCityVisualizerGui.lnk"

if not exist "%VIS_EXE%" exit /b 0

where powershell >nul 2>nul
if errorlevel 1 (
  echo WARNING: PowerShell not found; could not create shortcut "%VIS_SHORTCUT%".
  exit /b 0
)

powershell -NoProfile -ExecutionPolicy Bypass -Command "$W=New-Object -ComObject WScript.Shell; $S=$W.CreateShortcut('%VIS_SHORTCUT%'); $S.TargetPath='%VIS_EXE%'; $S.WorkingDirectory='%ROOT%'; $S.IconLocation='%VIS_EXE%,0'; $S.Description='RogueCity Visualizer GUI'; $S.Save()"
if errorlevel 1 (
  echo WARNING: Failed to update shortcut "%VIS_SHORTCUT%".
) else (
  echo Shortcut updated: "%VIS_SHORTCUT%"
)
exit /b 0
