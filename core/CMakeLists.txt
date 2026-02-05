
# ==================================================
# RogueCity Core Data Layer
# ==================================================
# Pure data structures and math utilities
# Zero dependencies on UI or generation logic

project(RogueCityCore VERSION 0.1.0 LANGUAGES CXX)

# === SOURCE FILES ===
set(CORE_HEADERS
    include/RogueCity/Core/Types.hpp
    include/RogueCity/Core/Math/Vec2.hpp
    include/RogueCity/Core/Data/CityTypes.hpp
    include/RogueCity/Core/Data/TensorTypes.hpp
)

set(CORE_SOURCES
    src/Core/Math/Vec2.cpp
    src/Core/Data/CityTypes.cpp
    src/Core/Data/TensorTypes.cpp
)

# === LIBRARY TARGET ===
add_library(RogueCityCore STATIC ${CORE_SOURCES} ${CORE_HEADERS} "src/Generators/Tensors/BasisFields.cpp" "src/Generators/Tensors/TensorFieldGenerator.cpp" "src/Generators/Roads/StreamlineTracer.cpp" "src/Generators/Roads/RoadClassifier.cpp" "src/Generators/Districts/AESPClassifier.cpp" "src/Generators/Pipeline/CityGenerator.cpp")

# === PUBLIC INTERFACE ===
target_include_directories(RogueCityCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# === DEPENDENCIES ===
target_link_libraries(RogueCityCore PUBLIC
    glm::glm
    magic_enum
)

# === C++ STANDARD ===
target_compile_features(RogueCityCore PUBLIC cxx_std_20)

# === PRECOMPILED HEADERS (FAST BUILDS) ===
target_precompile_headers(RogueCityCore PUBLIC
    <vector>
    <array>
    <unordered_map>
    <memory>
    <cstdint>
    <cmath>
    <string>
    <algorithm>
)

# === COMPILER WARNINGS ===
if(MSVC)
    target_compile_options(RogueCityCore PRIVATE 
        /W4           # High warning level
        /WX           # Treat warnings as errors
        /permissive-  # Strict standard conformance
    )
else()
    target_compile_options(RogueCityCore PRIVATE 
        -Wall -Wextra -Werror -pedantic
    )
endif()

# === VERIFICATION ===
# Ensure no accidental UI dependencies
get_target_property(CORE_INCLUDES RogueCityCore INCLUDE_DIRECTORIES)
foreach(inc ${CORE_INCLUDES})
    if("${inc}" MATCHES "imgui|glfw|glad")
        message(FATAL_ERROR "Core cannot depend on UI library! Found: ${inc}")
    endif()
endforeach()

# === INSTALL RULES ===
install(TARGETS RogueCityCore
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/RogueCity
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

message(STATUS "RogueCityCore configured:")
message(STATUS "  - Source files: ${CORE_SOURCES}")
message(STATUS "  - Include dir: ${CMAKE_CURRENT_SOURCE_DIR}/include")
