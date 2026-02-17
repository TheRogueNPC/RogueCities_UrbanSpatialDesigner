# RogueCity Solution Build Quick Start

## Overview

This project builds from the CMake-generated Visual Studio solution at [build_vs/RogueCities.sln](build_vs/RogueCities.sln).

Key targets:

1. `RogueCityCore`
2. `RogueCityGenerators`
3. `RogueCityApp`
4. `RogueCityAI`
5. `RogueCityVisualizerGui`

Primary executable output:

- [bin/RogueCityVisualizerGui.exe](bin/RogueCityVisualizerGui.exe)

## Build and Run

### Visual Studio

```powershell
start build_vs\RogueCities.sln
```

Then:

1. Set `RogueCityVisualizerGui` as startup project.
2. Select `Release` or `Debug`.
3. Press `F5`.

### Command Line

```powershell
cmake --build build_vs --target RogueCityVisualizerGui --config Release --parallel 4
.\bin\RogueCityVisualizerGui.exe
```

### One-Step Script

```powershell
.\build_and_run.bat
```

## Configure

```powershell
cmake -B build_vs -S . -DROGUECITY_BUILD_VISUALIZER=ON
```

## Geometry Migration Flags

```powershell
cmake -B build_vs -S . -DUSE_LEGACY_GEOS=ON
cmake -B build_vs -S . -DUSE_LEGACY_GEOS=OFF
```

Recommended dependency install:

```powershell
vcpkg install boost-geometry fastnoise2 nanoflann delaunator-cpp
```

Optional terrain backend:

```powershell
-DROGUECITY_ENABLE_SLTERRAIN=ON
```

## Troubleshooting

### Build path errors

```powershell
Remove-Item build_vs/CMakeCache.txt -Force
cmake -B build_vs -S . -DROGUECITY_BUILD_VISUALIZER=ON
```

### Visualizer disabled

```powershell
cmake -B build_vs -S . -DROGUECITY_BUILD_VISUALIZER=ON
```

### Verify binary

Expected file:

- [bin/RogueCityVisualizerGui.exe](bin/RogueCityVisualizerGui.exe)

## Quality Checks

```powershell
python3 tools/check_ui_compliance.py
python3 tools/check_generator_viewport_contract.py
python3 tools/check_tool_wiring_contract.py
```