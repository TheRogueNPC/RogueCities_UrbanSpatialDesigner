cmake_minimum_required(VERSION 3.20)

# Lua (vendored) - builds a static library target `lua::lua`.
# Expects Lua sources under `3rdparty/lua/src/`.

project(lua C)

set(LUA_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")

set(LUA_CORE_SOURCES
    "${LUA_SRC_DIR}/lapi.c"
    "${LUA_SRC_DIR}/lcode.c"
    "${LUA_SRC_DIR}/lctype.c"
    "${LUA_SRC_DIR}/ldebug.c"
    "${LUA_SRC_DIR}/ldo.c"
    "${LUA_SRC_DIR}/ldump.c"
    "${LUA_SRC_DIR}/lfunc.c"
    "${LUA_SRC_DIR}/lgc.c"
    "${LUA_SRC_DIR}/llex.c"
    "${LUA_SRC_DIR}/lmem.c"
    "${LUA_SRC_DIR}/lobject.c"
    "${LUA_SRC_DIR}/lopcodes.c"
    "${LUA_SRC_DIR}/lparser.c"
    "${LUA_SRC_DIR}/lstate.c"
    "${LUA_SRC_DIR}/lstring.c"
    "${LUA_SRC_DIR}/ltable.c"
    "${LUA_SRC_DIR}/ltm.c"
    "${LUA_SRC_DIR}/lundump.c"
    "${LUA_SRC_DIR}/lvm.c"
    "${LUA_SRC_DIR}/lzio.c"
    "${LUA_SRC_DIR}/lauxlib.c"
    "${LUA_SRC_DIR}/lbaselib.c"
    "${LUA_SRC_DIR}/lcorolib.c"
    "${LUA_SRC_DIR}/ldblib.c"
    "${LUA_SRC_DIR}/liolib.c"
    "${LUA_SRC_DIR}/lmathlib.c"
    "${LUA_SRC_DIR}/loadlib.c"
    "${LUA_SRC_DIR}/loslib.c"
    "${LUA_SRC_DIR}/lstrlib.c"
    "${LUA_SRC_DIR}/ltablib.c"
    "${LUA_SRC_DIR}/lutf8lib.c"
    "${LUA_SRC_DIR}/linit.c"
)

add_library(lua STATIC ${LUA_CORE_SOURCES})
add_library(lua::lua ALIAS lua)

target_include_directories(lua PUBLIC "${LUA_SRC_DIR}")

target_compile_definitions(lua PRIVATE LUA_COMPAT_5_3)

if(MSVC)
    target_compile_definitions(lua PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
