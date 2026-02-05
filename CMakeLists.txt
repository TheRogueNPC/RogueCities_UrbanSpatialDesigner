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

# === CORE LIBRARY ===
add_subdirectory(core)
#=== GENERATORS LIBRARY (Phase 2)===
add_subdirectory(generators)
# === TESTS ===
add_executable(test_generators test_generators.cpp)
target_link_libraries(test_generators PRIVATE RogueCityCore RogueCityGenerators)

message(STATUS "===========================================")
message(STATUS "Core configured successfully!")
message(STATUS "Build with: cmake --build build_core --target RogueCityCore")
message(STATUS "===========================================")
