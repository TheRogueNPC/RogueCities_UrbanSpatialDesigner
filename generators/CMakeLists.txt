# ==================================================
# RogueCity Generators Library
# ==================================================
# Procedural city generation algorithms using core data types

project(RogueCityGenerators VERSION 0.1.0 LANGUAGES CXX)

message(STATUS "Configuring Generators Library...")

# Source files
set(GENERATOR_SOURCES
    src/Generators/Tensors/TensorFieldGenerator.cpp
    src/Generators/Tensors/BasisFields.cpp
    src/Generators/Roads/StreamlineTracer.cpp
    src/Generators/Roads/RoadClassifier.cpp
    src/Generators/Districts/AESPClassifier.cpp
    src/Generators/Pipeline/CityGenerator.cpp
)

# Header files (for IDE integration)
set(GENERATOR_HEADERS
    include/RogueCity/Generators/Tensors/TensorFieldGenerator.hpp
    include/RogueCity/Generators/Tensors/BasisFields.hpp
    include/RogueCity/Generators/Roads/StreamlineTracer.hpp
    include/RogueCity/Generators/Roads/RoadClassifier.hpp
    include/RogueCity/Generators/Districts/AESPClassifier.hpp
    include/RogueCity/Generators/Pipeline/CityGenerator.hpp
)

# Create static library
add_library(RogueCityGenerators STATIC
    ${GENERATOR_SOURCES}
    ${GENERATOR_HEADERS}
)

# Public include directories
target_include_directories(RogueCityGenerators
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(RogueCityGenerators
    PUBLIC
        RogueCityCore    # Phase 1 core library
        glm::glm         # Math utilities
)

# C++20 features
target_compile_features(RogueCityGenerators PUBLIC cxx_std_20)

# Optimization flags (configuration-specific)
if(MSVC)
    target_compile_options(RogueCityGenerators PRIVATE 
        /W4
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Od>
    )
else()
    target_compile_options(RogueCityGenerators PRIVATE 
        -Wall -Wextra
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Debug>:-O0>
    )
endif()

message(STATUS "Generators library configured: ${CMAKE_CURRENT_BINARY_DIR}")
