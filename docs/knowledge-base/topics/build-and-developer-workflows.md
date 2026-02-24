---
tags: [roguecity, build, cmake, visual-studio]
type: how-to
created: 2026-02-15
---

# Build and Developer Workflows (CMake + Visual Studio)

The project supports both Visual Studio solution workflows and direct CMake CLI workflows, with vendored dependencies in `3rdparty/` and maintenance scripts for compatibility checks.

## Main Build Paths
- Visual Studio solution flow via `build_vs/RogueCities.sln`
- CMake configure + build flow via `cmake -B build_vs -S .`
- Quick launch script via `build_and_run.bat`

## Related Notes
- [[notes/build-workflows-visual-studio-and-cmake]]
- [[notes/cmake-dependency-management-with-submodules]]
- [[notes/geometry-backend-selection-geos-vs-boost]]
- [[topics/testing-and-quality-assurance]]
