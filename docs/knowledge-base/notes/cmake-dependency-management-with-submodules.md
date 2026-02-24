---
tags: [roguecity, cmake, dependencies, git-submodules]
type: reference
created: 2026-02-15
---

# CMake Dependency Management with Git Submodules

Dependencies are vendored in `3rdparty/` and CMake target creation patterns are standardized so app, AI, and visualizer can share `RogueCityImGui`, while GLFW is built from source via subdirectory integration.

## Dependency Practices
- Initialize once: `git submodule update --init --recursive`
- Build GLFW from source, avoid prebuilt binaries
- Include `imgui_tables.cpp` in ImGui target definitions
- Use maintenance checker for migration/fixes

## Source Files
- `docs/CMake_Maintenance.md`
- `app/CMakeLists.txt`
- `AI/CMakeLists.txt`
- `visualizer/CMakeLists.txt`

## Related
- [[topics/build-and-developer-workflows]]
- [[notes/build-workflows-visual-studio-and-cmake]]
- [[notes/module-visualizer-executables]]
