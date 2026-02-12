# CMake Build System Maintenance

This document describes how to maintain and troubleshoot the CMake build system for RogueCities_UrbanSpatialDesigner.

## Quick Health Check

Run the compatibility checker to verify your build system is up-to-date:

```powershell
# Check for issues (read-only)
.\tools\Check_CMake_Compatibility.ps1

# Auto-fix common issues
.\tools\Check_CMake_Compatibility.ps1 -Fix

# Verbose output
.\tools\Check_CMake_Compatibility.ps1 -Verbose
```

## Current Dependency Structure

### Git Submodules (Required)

All dependencies are vendored in `3rdparty/` as git submodules:

```
3rdparty/
├── imgui/           # Dear ImGui v1.92+ (UI framework)
├── glfw/            # GLFW 3.x (windowing/input)
├── glm/             # GLM (math library)
├── magic_enum/      # Enum reflection
├── sol2/            # Lua binding
├── ImDesignManager/ # Design system integration
├── tabulate/        # Table formatting
└── ymery-cpp/       # YAML parsing
```

**Initialize all submodules:**

```bash
git submodule update --init --recursive
```

### CMakeLists.txt Architecture

**Layer Separation:**
```
Root CMakeLists.txt
├── core/           # Pure data/math (no UI deps)
├── generators/     # Pipeline algorithms
├── app/            # Integration layer (creates RogueCityImGui)
├── visualizer/     # UI executables (builds GLFW from source)
└── AI/             # AI protocol layer (creates RogueCityImGui if needed)
```

## Common Patterns

### ImGui Integration

**Standard pattern** (used in `app/CMakeLists.txt`, `AI/CMakeLists.txt`):

```cmake
if(NOT TARGET RogueCityImGui)
    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/3rdparty/imgui")
    
    if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
        message(FATAL_ERROR 
            "ImGui not found at ${IMGUI_DIR}.\n"
            "Run: git submodule update --init --recursive"
        )
    endif()
    
    add_library(RogueCityImGui STATIC
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"  # Required for v1.80+
        "${IMGUI_DIR}/imgui_demo.cpp"
    )
    
    target_include_directories(RogueCityImGui PUBLIC "${IMGUI_DIR}")
    target_compile_features(RogueCityImGui PUBLIC cxx_std_20)
endif()
```

**Critical:** Always include `imgui_tables.cpp` for ImGui 1.80+.

### GLFW Integration

**Build from source** (used in `visualizer/CMakeLists.txt`):

```cmake
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/3rdparty/glfw")

if(EXISTS "${GLFW_DIR}/CMakeLists.txt")
    # Configure GLFW options
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    
    add_subdirectory("${GLFW_DIR}" "${CMAKE_BINARY_DIR}/glfw" EXCLUDE_FROM_ALL)
    
    # Link target: 'glfw' (NOT 'glfw_bundled')
    target_link_libraries(MyTarget PRIVATE glfw)
endif()
```

**Never use prebuilt GLFW binaries** - they're not cross-platform.

## Migration Guide

### Updating from Old Patterns

| Old Pattern | New Pattern | Fix Command |
|-------------|-------------|-------------|
| `3rdparty/imvue/imgui` | `3rdparty/imgui` | Run compatibility script with `-Fix` |
| `glfw_bundled` | `glfw` | Update `target_link_libraries` |
| `lib-vc2010-64` detection | `add_subdirectory(glfw)` | Use source build pattern above |
| Missing `imgui_tables.cpp` | Add to ImGui sources | Update all `add_library(RogueCityImGui)` calls |

### Automated Fix Process

```powershell
# 1. Check current state
.\tools\Check_CMake_Compatibility.ps1 -Verbose

# 2. Apply fixes
.\tools\Check_CMake_Compatibility.ps1 -Fix

# 3. Review changes
git diff

# 4. Test build
cmake -B build -S .
cmake --build build --config Release

# 5. Commit if successful
git add -A
git commit -m "Apply CMake compatibility fixes"
```

## Troubleshooting

### Issue: "ImGui not found"

**Cause:** Submodules not initialized.

**Fix:**
```bash
git submodule update --init --recursive
```

### Issue: "Cannot find source file: imgui_tables.cpp"

**Cause:** ImGui version < 1.80 or missing from sources list.

**Fix:**
1. Update imgui submodule: `cd 3rdparty/imgui && git pull origin master`
2. Add to CMakeLists.txt: `"${IMGUI_DIR}/imgui_tables.cpp"`

### Issue: "GLFW library not found"

**Cause:** Using old prebuilt binary detection instead of source build.

**Fix:**
Replace prebuilt detection with `add_subdirectory` pattern (see above).

### Issue: "vcpkg not found" warnings

**Cause:** Old CMakeLists.txt has hardcoded vcpkg paths as fallbacks.

**Fix:**
These are harmless fallback paths. The build will succeed using vendored dependencies. To remove warnings, delete vcpkg search paths from root `CMakeLists.txt`.

## CI/CD Integration

### Azure Pipelines

Ensure `azure-pipelines.yml` includes recursive submodule checkout:

```yaml
steps:
- checkout: self
  submodules: recursive
```

### GitHub Actions

Ensure workflow includes recursive submodule checkout:

```yaml
steps:
- uses: actions/checkout@v4
  with:
    submodules: recursive
```

## Maintenance Checklist

**Before major commits:**
- [ ] Run `.\tools\Check_CMake_Compatibility.ps1`
- [ ] Verify all submodules initialized: `git submodule status`
- [ ] Test clean build: `cmake -B build_test -S . && cmake --build build_test`
- [ ] Check for warnings in CMake configure step

**After dependency updates:**
- [ ] Update submodule: `cd 3rdparty/<name> && git pull origin master`
- [ ] Update root: `git add 3rdparty/<name> && git commit -m "Update <name> submodule"`
- [ ] Run compatibility script
- [ ] Test build on Windows and Linux (if applicable)

**When adding new dependencies:**
1. Add as git submodule: `git submodule add <url> 3rdparty/<name>`
2. Add detection logic to root `CMakeLists.txt`
3. Update `Check_CMake_Compatibility.ps1` with new patterns
4. Update this document with integration pattern

## Reference

- **ImGui Documentation:** https://github.com/ocornut/imgui
- **GLFW Documentation:** https://www.glfw.org/docs/latest/
- **CMake Best Practices:** https://cliutils.gitlab.io/modern-cmake/

---

**Last Updated:** February 11, 2026  
**Maintainer:** Build System Team  
**Related Files:**
- `tools/Check_CMake_Compatibility.ps1`
- `.github/copilot-instructions.md`
- `docs/temp_Documentation/TechStack.md`
