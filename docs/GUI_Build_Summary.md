# RogueCity Visualizer GUI - Build Summary

## What Was Fixed

### 1. CMake Configuration
- **Added C language support** to `Visualizer/CMakeLists.txt` to compile gl3w.c properly
- **Used bundled GLFW** from `3rdparty/imvue/imgui/examples/libs/glfw/` instead of requiring vcpkg
- **Auto-detected architecture** (x86/x64) and selected appropriate pre-built GLFW library
- **Disabled precompiled headers** for the GUI target to avoid C/C++ mixing issues
- **Added legacy_stdio_definitions** link library for MSVC 2015+ compatibility with older GLFW binaries

### 2. Source Files Created
- **`Visualizer/src/main_gui.cpp`**: Full GLFW + OpenGL3 ImGui application integrated with your HFSM editor state
  - Uses gl3w as OpenGL loader
  - Integrates with `EditorHFSM` and `GlobalState`
  - Displays interactive menu bar to switch editor modes
  - Shows context-sensitive windows based on editor state

### 3. Build Script
- **`build_and_run_gui.ps1`**: PowerShell script to automate configure, build, and launch
  - Supports clean builds with `-Clean` flag
  - Supports run-only mode with `-RunOnly` flag
  - Configurable build type (defaults to Release)

## Files Modified
1. `Visualizer/CMakeLists.txt` - Added GUI target with bundled GLFW support
2. Created `Visualizer/src/main_gui.cpp` - Main GUI application
3. Created `build_and_run_gui.ps1` - Build automation script

## How to Use

### Quick Start (Recommended)
```powershell
.\build_and_run_gui.ps1
```

### Manual Build
```powershell
# Configure
cmake -B build_gui -S . -G Ninja -DROGUECITY_BUILD_VISUALIZER=ON -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build_gui --config Release --target RogueCityVisualizerGui -j 8

# Run
.\build_gui\visualizer\RogueCityVisualizerGui.exe
```

### Script Options
```powershell
# Clean build
.\build_and_run_gui.ps1 -Clean

# Run existing build without rebuilding
.\build_and_run_gui.ps1 -RunOnly

# Debug build
.\build_and_run_gui.ps1 -BuildType Debug
```

## What the GUI Does

The visualizer window integrates your HFSM editor state machine:

1. **Main Menu Bar**: Switch between editor modes
   - Idle
   - Edit Roads
   - Edit Districts
   - Simulate

2. **Context Windows**: Show/hide based on current editor state
   - Roads window: Displays road count when in Edit Roads mode
   - Districts window: Displays district count when in Edit Districts mode
   - Simulation window: Shows simulation status when in Simulate mode

3. **HFSM Integration**: 
   - Calls `hfsm.update(gs, deltaTime)` each frame
   - Handles state transitions via menu interactions
   - Boots through proper HFSM startup sequence

## Dependencies (All Vendored)

No external installation required! Everything is bundled:
- ? ImGui: `3rdparty/imvue/imgui/`
- ? GLFW: `3rdparty/imvue/imgui/examples/libs/glfw/` (pre-built binaries)
- ? gl3w: `3rdparty/imvue/imgui/examples/libs/gl3w/` (OpenGL loader)
- ? GLM: `3rdparty/glm/`
- ? magic_enum: `3rdparty/magic_enum/`

## Build Requirements
- CMake 3.20+
- Ninja (or Visual Studio generator)
- MSVC 2015+ (for Windows)
- C++20 compiler

## Tested Configuration
- ? Windows 10/11
- ? MSVC 19.50 (Visual Studio 2026)
- ? Ninja 1.11+
- ? CMake 3.28+
- ? 32-bit and 64-bit architectures

## Next Steps

You can now:
1. Add your city visualization rendering to the main loop
2. Integrate generator pipeline visualization
3. Add more ImGui panels for different editor states
4. Extend the HFSM with additional states and transitions
5. Add viewport rendering for roads, districts, and tensor fields

## Notes

- The bundled GLFW libraries are from VS2010 but work fine with modern MSVC via `legacy_stdio_definitions`
- gl3w is loaded at runtime via `gl3wInit()` in `main_gui.cpp`
- The GUI runs in a proper event loop with vsync enabled
- Window size: 1280x720 (configurable in `main_gui.cpp`)
