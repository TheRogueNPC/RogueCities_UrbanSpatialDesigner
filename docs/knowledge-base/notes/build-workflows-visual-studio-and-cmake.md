---
tags: [roguecity, build, cmake, visual-studio, workflow]
type: how-to
created: 2026-02-15
---

# Build Workflows Visual Studio and CMake (Windows)

The preferred developer flow is opening `build_vs/RogueCities.sln` in Visual Studio, but command-line CMake builds are first-class and documented for automated or script-driven usage.

## Common Commands
- Configure: `cmake -B build_vs -S . -DROGUECITY_BUILD_VISUALIZER=ON`
- Build GUI target: `cmake --build build_vs --target RogueCityVisualizerGui --config Release --parallel 4`
- Run binary: `bin/RogueCityVisualizerGui.exe`

## Helpful Scripts
- `build_and_run.bat`
- `build_and_run_gui.ps1`

## Source Files
- `BUILD_INSTRUCTIONS.md`

## Related
- [[topics/build-and-developer-workflows]]
- [[notes/cmake-dependency-management-with-submodules]]
- [[notes/geometry-backend-selection-geos-vs-boost]]
