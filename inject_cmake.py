import os

cmake_path = r"d:\Projects\RogueCities\RogueCities_UrbanSpatialDesigner\CMakeLists.txt"
with open(cmake_path, "r", encoding="utf-8") as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    new_lines.append(line)
    if "message(STATUS \"LInput not found" in line:
        pass
    if line.strip() == "# === UI TOOLCHAIN LIBS (console-safe) ===":
        injection = """
# === Clipper2 ===
set(CLIPPER2_DIR "${CMAKE_SOURCE_DIR}/3rdparty/Clipper2")
set(ROGUECITY_HAS_CLIPPER2 OFF)
if(EXISTS "${CLIPPER2_DIR}/CMakeLists.txt")
    # We only need the core Clipper2 library, turn off tests/utils to keep noise low
    set(CLIPPER2_EXAMPLES OFF CACHE BOOL "Disable Clipper2 examples" FORCE)
    set(CLIPPER2_TESTS OFF CACHE BOOL "Disable Clipper2 tests" FORCE)
    set(CLIPPER2_UTILS OFF CACHE BOOL "Disable Clipper2 utils" FORCE)
    set(CLIPPER2_USINGZ_MACRO OFF CACHE BOOL "Disable Clipper2 Z-coordinate macro" FORCE)
    
    add_subdirectory("${CLIPPER2_DIR}" clipper2_build EXCLUDE_FROM_ALL)
    
    # Silence warnings from 3rdparty
    if(TARGET Clipper2)
        if(MSVC)
            target_compile_options(Clipper2 PRIVATE /W0)
        else()
            target_compile_options(Clipper2 PRIVATE -w)
        endif()
    endif()
    set(ROGUECITY_HAS_CLIPPER2 ON)
    message(STATUS "Found and configured Clipper2 from 3rdparty")
else()
    message(STATUS "Clipper2 not found (expected at: ${CLIPPER2_DIR})")
endif()

"""
        new_lines.insert(-1, injection)

with open(cmake_path, "w", encoding="utf-8") as f:
    f.writelines(new_lines)
print("Injected Clipper2 into CMakeLists.txt")
