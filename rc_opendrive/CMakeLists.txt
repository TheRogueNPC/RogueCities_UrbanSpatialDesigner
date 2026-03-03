# ==============================================================================
# rc_opendrive - RogueCities OpenDRIVE ASAM Specification Parser
# Target: Static library when sources exist, INTERFACE stub otherwise.
# ==============================================================================

cmake_minimum_required(VERSION 3.20)
project(RogueOpenDRIVE VERSION 1.0.0 LANGUAGES CXX)

# Require C++20 for compatibility with RogueCityCore.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Discover library sources/headers. If no sources are present yet, we still
# provide an include-only target so workspace configure/build does not fail.
file(GLOB_RECURSE RC_OPENDRIVE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE RC_OPENDRIVE_HEADERS CONFIGURE_DEPENDS
    "include/RogueOpenDRIVE/*.h"
    "include/RogueOpenDRIVE/*.hpp")

set(RC_OPENDRIVE_PUBLIC_INCLUDE_DIRS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/RogueOpenDRIVE>
    $<INSTALL_INTERFACE:include>)
set(RC_OPENDRIVE_PRIVATE_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include/RogueOpenDRIVE)

if(RC_OPENDRIVE_SOURCES)
    add_library(rc_opendrive STATIC ${RC_OPENDRIVE_SOURCES} ${RC_OPENDRIVE_HEADERS})
    target_include_directories(rc_opendrive
        PUBLIC ${RC_OPENDRIVE_PUBLIC_INCLUDE_DIRS}
        PRIVATE ${RC_OPENDRIVE_PRIVATE_INCLUDE_DIRS})

    # Link against nlohmann_json if target exists; otherwise use local headers.
    if(TARGET nlohmann_json::nlohmann_json)
        target_link_libraries(rc_opendrive PUBLIC nlohmann_json::nlohmann_json)
    else()
        target_include_directories(
            rc_opendrive PUBLIC "${CMAKE_SOURCE_DIR}/3rdparty/nlohmann_json/include")
    endif()

    if(MSVC)
        target_compile_options(rc_opendrive PRIVATE /W4 /WX-)
        target_compile_definitions(rc_opendrive PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    message(STATUS "[rc_opendrive] Configured as static library (RogueCity::OpenDRIVE)")
else()
    add_library(rc_opendrive INTERFACE)
    target_include_directories(rc_opendrive INTERFACE ${RC_OPENDRIVE_PUBLIC_INCLUDE_DIRS})
    message(WARNING "[rc_opendrive] No src/*.cpp found; configuring include-only stub target.")
endif()

# Provide a stable alias used by the rest of the workspace.
add_library(RogueCity::OpenDRIVE ALIAS rc_opendrive)
