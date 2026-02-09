# AI_INTEGRATION_TAG: V1_PASS1_TASK1_LUA_LIBRARY_BUILD
# AGENT: Debug_Manager
# CATEGORY: Build_System_Fix
# LAST_MODIFIED: 2026-02-08

cmake_minimum_required(VERSION 3.15)
project(lua548 VERSION 5.4.8 LANGUAGES C)

# Lua 5.4.8 Library Build
# This CMakeLists.txt creates the lua::lua target for proper linkage with sol2

# Explicitly list all Lua core sources (glob doesn't always work reliably in CMake)
set(LUA_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lapi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lcode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lctype.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ldebug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ldo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ldump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lfunc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lgc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/llex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lobject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lopcodes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lparser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lstate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lstring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ltable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ltm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lundump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lzio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lauxlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lbaselib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lcorolib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ldblib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/liolib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lmathlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loadlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loslib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lstrlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ltablib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lutf8lib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/linit.c
)

# Create Lua library target
add_library(lua_548 STATIC ${LUA_CORE_SOURCES})

# Set include directories
target_include_directories(lua_548
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# Platform-specific definitions
if(WIN32)
    target_compile_definitions(lua_548 PRIVATE LUA_BUILD_AS_DLL)
endif()

if(UNIX)
    target_compile_definitions(lua_548 PRIVATE LUA_USE_POSIX)
    target_link_libraries(lua_548 PUBLIC m dl)
endif()

# AI_INTEGRATION_TAG: V1_PASS1_TASK1_LUA_COMPAT_MACROS
# Expose macros as functions for sol2 compatibility
# Lua 5.4 changed lua_remove, lua_insert, lua_replace from functions to macros
# sol2's compat layer expects them as functions, so we need to define them
target_compile_definitions(lua_548 PUBLIC
    LUA_COMPAT_5_3
)

# Create lua::lua alias for sol2 compatibility
add_library(lua::lua ALIAS lua_548)

# DEBUG_TAG: LUA_LIBRARY_CREATED
message(STATUS "[V1 PASS1 Task 1.1] Lua 5.4.8 library target 'lua::lua' created with Lua 5.3 compat")
