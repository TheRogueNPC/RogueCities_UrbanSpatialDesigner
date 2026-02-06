# Build Fixes - CMake & Executable Output

## Issues Fixed

### 1. CMake Generator Expression Error
**Problem:** CMake was showing errors like:
```
Error evaluating generator expression: $<CONFIG:$BuildType>
Expression syntax not recognized.
```

**Root Cause:** PowerShell backtick line continuation was interfering with variable expansion when passing CMAKE_BUILD_TYPE.

**Solution:** Rewrote the build script to avoid backticks and properly quote the CMAKE_BUILD_TYPE argument:
```powershell
cmake -B $BuildDir -S . -G Ninja -DROGUECITY_BUILD_VISUALIZER=ON "-DCMAKE_BUILD_TYPE=$BuildType"
```

### 2. Executable Output Directory
**Problem:** Executables were scattered in different build subdirectories making them hard to find.

**Solution:** Centralized all executable output to `bin/` directory by adding to root `CMakeLists.txt`:
```cmake
# === OUTPUT DIRECTORIES ===
# Set all executables to go to the bin/ folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/../bin)
```

## Files Modified

1. **`CMakeLists.txt`**  
   - Added output directory configuration
   - Updated status message to show where executables will be output

2. **`build_and_run_gui.ps1`**  
   - Fixed CMake invocation to properly expand `$BuildType` variable
   - Removed backtick line continuations
   - Updated executable path from `build_gui\visualizer\` to `bin\`
   - Removed `--config` flag from build command (not needed for Ninja)

3. **`visualizer/CMakeLists.txt`**  
   - Removed redundant output directory settings (now handled globally)

## New Executable Locations

All executables now output to: `bin/`

- `bin/RogueCityVisualizerGui.exe` - GUI visualizer
- `bin/test_generators.exe` - Generator tests
- `bin/test_core.exe` - Core tests
- `bin/test_editor_hfsm.exe` - HFSM tests
- `bin/test_ui_toolchain.exe` - UI toolchain probe

## How to Build & Run

### Clean Build
```powershell
.\build_and_run_gui.ps1 -Clean
```

### Incremental Build
```powershell
.\build_and_run_gui.ps1
```

### Run Only (No Build)
```powershell
.\build_and_run_gui.ps1 -RunOnly
```

### Manual Build
```powershell
cmake -B build_gui -S . -G Ninja -DROGUECITY_BUILD_VISUALIZER=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build_gui --target RogueCityVisualizerGui -j 8
.\bin\RogueCityVisualizerGui.exe
```

## Testing the Fix

To verify the fix works:

1. Clean all build directories:
   ```powershell
   Remove-Item -Recurse -Force build_gui,bin -ErrorAction SilentlyContinue
   ```

2. Run the build script:
   ```powershell
   .\build_and_run_gui.ps1
   ```

3. Verify executable exists:
   ```powershell
   Test-Path bin\RogueCityVisualizerGui.exe
   # Should return: True
   ```

4. Run the GUI:
   ```powershell
   .\bin\RogueCityVisualizerGui.exe
   ```

## Note on Ninja vs Visual Studio Generator

- **Ninja:** Uses `-DCMAKE_BUILD_TYPE=<config>` at configure time
- **Visual Studio:** Uses `--config <config>` at build time

The script now correctly uses Ninja's approach (build type at configure time) and doesn't pass `--config` to the build command.
