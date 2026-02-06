# ==================================================
# RogueCity MVP - Core-Only Build
# ==================================================
# This builds ONLY the new core library, ignoring broken src/

cmake_minimum_required(VERSION 3.20)
project(RogueCityMVP VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "===========================================")
message(STATUS "Building RogueCity Core (MVP Mode)")
message(STATUS "Ignoring old src/Generator/ code")
message(STATUS "===========================================")

# === GLM ===
find_package(glm CONFIG QUIET)
if(NOT glm_FOUND)
    message(STATUS "GLM not found via find_package, using header-only fallback")
    add_library(glm INTERFACE)
    
    # Try common locations
    set(GLM_SEARCH_PATHS
        "${CMAKE_SOURCE_DIR}/3rdparty/glm"
        "C:/vcpkg/installed/x64-windows/include"
        "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
    )
    
    foreach(path ${GLM_SEARCH_PATHS})
        if(EXISTS "${path}/glm/glm.hpp")
            message(STATUS "Found GLM at: ${path}")
            target_include_directories(glm INTERFACE "${path}")
            break()
        endif()
    endforeach()
    
    add_library(glm::glm ALIAS glm)
endif()

# === magic_enum ===
set(MAGIC_ENUM_DIR "${CMAKE_SOURCE_DIR}/3rdparty/magic_enum")
if(NOT EXISTS "${MAGIC_ENUM_DIR}/include/magic_enum/magic_enum.hpp")
    message(FATAL_ERROR 
        "magic_enum not found!\n"
        "Run: cd 3rdparty && git clone https://github.com/Neargye/magic_enum.git"
    )
endif()

if(NOT TARGET magic_enum)
    add_library(magic_enum INTERFACE)
    target_include_directories(magic_enum INTERFACE "${MAGIC_ENUM_DIR}/include")
endif()

# === UI TOOLCHAIN LIBS (console-safe) ===
# These are wired for the `test_ui_toolchain` probe and future UI work.

# --- tabulate (header-only) ---
set(TABULATE_DIR "${CMAKE_SOURCE_DIR}/3rdparty/tabulate")
set(ROGUECITY_HAS_TABULATE OFF)
if(EXISTS "${TABULATE_DIR}/include/tabulate/table.hpp")
    if(NOT TARGET tabulate)
        add_library(tabulate INTERFACE)
        target_include_directories(tabulate INTERFACE "${TABULATE_DIR}/include")
    endif()
    if(NOT TARGET tabulate::tabulate)
        add_library(tabulate::tabulate ALIAS tabulate)
    endif()
    set(ROGUECITY_HAS_TABULATE ON)
else()
    message(STATUS "tabulate not found (expected at: ${TABULATE_DIR})")
endif()

# --- sol2 (header-only, requires Lua headers+libs) ---
set(SOL2_DIR "${CMAKE_SOURCE_DIR}/3rdparty/sol2")
set(ROGUECITY_HAS_SOL2 OFF)
if(EXISTS "${SOL2_DIR}/include/sol/sol.hpp")
    # Prefer vendored Lua for better UX.
    # This repo vendors multiple Lua versions; sol2 officially supports 5.1-5.4.
    # Default to Lua 5.4.8 and keep 5.5.0 for future experiments.
    set(ROGUECITY_LUA_VENDORED_VERSION "5.4.8" CACHE STRING "Vendored Lua version to use (e.g. 5.4.8)")
    set(ROGUECITY_LUA_VENDORED_DIR "${CMAKE_SOURCE_DIR}/3rdparty/lua/lua-${ROGUECITY_LUA_VENDORED_VERSION}")
    set(ROGUECITY_LUA_VENDORED_SRC_DIR "${ROGUECITY_LUA_VENDORED_DIR}/src")
    if(EXISTS "${ROGUECITY_LUA_VENDORED_DIR}/CMakeLists.txt" AND EXISTS "${ROGUECITY_LUA_VENDORED_SRC_DIR}/lua.h")
        add_subdirectory(${ROGUECITY_LUA_VENDORED_DIR} EXCLUDE_FROM_ALL)
        set(LUA_FOUND TRUE)
        set(LUA_INCLUDE_DIRS "${ROGUECITY_LUA_VENDORED_SRC_DIR}")
        set(LUA_LIBRARIES lua::lua)
    else()
        find_package(Lua QUIET)
    endif()

    if(LUA_FOUND)
        if(NOT TARGET sol2)
            add_library(sol2 INTERFACE)
            target_include_directories(sol2 INTERFACE
                "${SOL2_DIR}/include"
                ${LUA_INCLUDE_DIR}
                ${LUA_INCLUDE_DIRS}
            )
            target_link_libraries(sol2 INTERFACE
                ${LUA_LIBRARIES}
                ${LUA_LIBRARY}
            )
        endif()
        if(NOT TARGET sol2::sol2)
            add_library(sol2::sol2 ALIAS sol2)
        endif()
        set(ROGUECITY_HAS_SOL2 ON)
    else()
        message(STATUS "Lua not found; sol2 present but disabled (set Lua_DIR / install Lua dev files)")
    endif()
else()
    message(STATUS "sol2 not found (expected at: ${SOL2_DIR})")
endif()

# === CORE LIBRARY ===
add_subdirectory(core)
#=== GENERATORS LIBRARY (Phase 2)===
add_subdirectory(generators)

# === VISUALIZER APP (stub) ===
# ImGui-based Visualizer will live under the visualizer/ folder.
# This keeps UI concerns separate from core/generators.
if(EXISTS "${CMAKE_SOURCE_DIR}/visualizer/CMakeLists.txt")
    add_subdirectory(visualizer)
endif()
# === TESTS ===
add_executable(test_generators tests/test_generators.cpp)
target_link_libraries(test_generators PRIVATE RogueCityCore RogueCityGenerators)
target_compile_features(test_generators PRIVATE cxx_std_20)

add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE RogueCityCore)
target_compile_features(test_core PRIVATE cxx_std_20)

# === UI TOOLCHAIN PROBE (console, pre-ImGui) ===
# This small executable is a console-only harness used to verify that
# the external UI/toolchain dependencies are reachable. It intentionally
# avoids any imgui/glfw/glad usage so that Core/Generators stay UI-free.
add_executable(test_ui_toolchain tests/ui_toolchain_demo.cpp)
target_compile_features(test_ui_toolchain PRIVATE cxx_std_20)
if(ROGUECITY_HAS_TABULATE)
    target_link_libraries(test_ui_toolchain PRIVATE tabulate::tabulate)
    target_compile_definitions(test_ui_toolchain PRIVATE ROGUECITY_HAS_TABULATE=1)
endif()
if(ROGUECITY_HAS_SOL2)
    target_link_libraries(test_ui_toolchain PRIVATE sol2::sol2)
    target_compile_definitions(test_ui_toolchain PRIVATE ROGUECITY_HAS_SOL2=1)
endif()

message(STATUS "===========================================")
message(STATUS "Core configured successfully!")
message(STATUS "Build with: cmake --build build_core --target RogueCityCore")
message(STATUS "===========================================")
