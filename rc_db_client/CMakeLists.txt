cmake_minimum_required(VERSION 3.20)
project(rc_db_client LANGUAGES CXX C)

# Use the default cargo target dir inside the crate itself to avoid
# path-with-spaces issues on Windows when CARGO_TARGET_DIR is set.
set(RUST_TARGET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/target")

# MSVC multi-config generators don't set CMAKE_BUILD_TYPE, so check generator
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_is_multi_config)
    # For multi-config (Visual Studio), always build release for the Rust side
    set(CARGO_PROFILE "release")
    set(CARGO_BUILD_ARGS "build" "--release")
    set(RUST_OUTPUT_DIR "${RUST_TARGET_DIR}/release")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_PROFILE "dev")
    set(CARGO_BUILD_ARGS "build")
    set(RUST_OUTPUT_DIR "${RUST_TARGET_DIR}/debug")
else()
    set(CARGO_PROFILE "release")
    set(CARGO_BUILD_ARGS "build" "--release")
    set(RUST_OUTPUT_DIR "${RUST_TARGET_DIR}/release")
endif()

if(MSVC)
    set(RUST_LIB_NAME "rc_db_client.lib")
else()
    set(RUST_LIB_NAME "librc_db_client.a")
endif()

set(RUST_OUTPUT_LIB "${RUST_OUTPUT_DIR}/${RUST_LIB_NAME}")

# Invoke Cargo — no CARGO_TARGET_DIR override needed since we use the default.
add_custom_command(
    OUTPUT "${RUST_OUTPUT_LIB}"
    COMMAND cargo ${CARGO_BUILD_ARGS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Rust FFI library (profile: ${CARGO_PROFILE})"
    VERBATIM
)

# Custom target to force the custom command
add_custom_target(rc_db_client_cargo ALL DEPENDS "${RUST_OUTPUT_LIB}")

# Create the CMake imported static library
add_library(rc_db_client STATIC IMPORTED GLOBAL)
set_target_properties(rc_db_client PROPERTIES IMPORTED_LOCATION "${RUST_OUTPUT_LIB}")
add_dependencies(rc_db_client rc_db_client_cargo)

# On Windows/MSVC, Rust's tokio + networking requires system libraries
if(MSVC)
    set_target_properties(rc_db_client PROPERTIES
        INTERFACE_LINK_LIBRARIES "ws2_32;userenv;bcrypt;advapi32;ntdll;crypt32;secur32;ncrypt"
    )
endif()
