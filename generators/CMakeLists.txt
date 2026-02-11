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
    src/Generators/Roads/SegmentGridStorage.cpp
    src/Generators/Roads/RoadNoder.cpp
    src/Generators/Roads/GraphSimplify.cpp
    src/Generators/Roads/FlowAndControl.cpp
    src/Generators/Roads/Verticality.cpp
    src/Generators/Roads/IntersectionTemplates.cpp
    src/Generators/Districts/AESPClassifier.cpp
    src/Generators/Urban/FrontageProfiles.cpp
    src/Generators/Urban/Integrator.cpp
    src/Generators/Urban/GridStorage.cpp
    src/Generators/Urban/PolygonUtil.cpp
    src/Generators/Urban/Graph.cpp
    src/Generators/Urban/GraphAlgorithms.cpp
    src/Generators/Urban/PolygonFinder.cpp
    src/Generators/Urban/RoadGenerator.cpp
    src/Generators/Urban/DistrictGenerator.cpp
    src/Generators/Urban/BlockGenerator.cpp
    src/Generators/Urban/LotGenerator.cpp
    src/Generators/Urban/SiteGenerator.cpp
    src/Generators/Pipeline/CityGenerator.cpp
    src/Generators/Pipeline/CitySpecAdapter.cpp
    src/Generators/Pipeline/ZoningGenerator.cpp
    src/Generators/Pipeline/TerrainConstraintGenerator.cpp
    src/Generators/Pipeline/PlanValidatorGenerator.cpp
)

# Header files (for IDE integration)
set(GENERATOR_HEADERS
    include/RogueCity/Generators/Tensors/TensorFieldGenerator.hpp
    include/RogueCity/Generators/Tensors/BasisFields.hpp
    include/RogueCity/Generators/Roads/StreamlineTracer.hpp
    include/RogueCity/Generators/Roads/RoadClassifier.hpp
    include/RogueCity/Generators/Roads/PolylineRoadCandidate.hpp
    include/RogueCity/Generators/Roads/SegmentGridStorage.hpp
    include/RogueCity/Generators/Roads/RoadNoder.hpp
    include/RogueCity/Generators/Roads/GraphSimplify.hpp
    include/RogueCity/Generators/Roads/FlowAndControl.hpp
    include/RogueCity/Generators/Roads/Verticality.hpp
    include/RogueCity/Generators/Roads/IntersectionTemplates.hpp
    include/RogueCity/Generators/Districts/AESPClassifier.hpp
    include/RogueCity/Generators/Urban/FrontageProfiles.hpp
    include/RogueCity/Generators/Urban/Integrator.hpp
    include/RogueCity/Generators/Urban/GridStorage.hpp
    include/RogueCity/Generators/Urban/PolygonUtil.hpp
    include/RogueCity/Generators/Urban/Graph.hpp
    include/RogueCity/Generators/Urban/GraphAlgorithms.hpp
    include/RogueCity/Generators/Urban/PolygonFinder.hpp
    include/RogueCity/Generators/Urban/RoadGenerator.hpp
    include/RogueCity/Generators/Urban/DistrictGenerator.hpp
    include/RogueCity/Generators/Urban/BlockGenerator.hpp
    include/RogueCity/Generators/Urban/LotGenerator.hpp
    include/RogueCity/Generators/Urban/SiteGenerator.hpp
    include/RogueCity/Generators/Pipeline/CityGenerator.hpp
    include/RogueCity/Generators/Pipeline/CitySpecAdapter.hpp
    include/RogueCity/Generators/Pipeline/ZoningGenerator.hpp
    include/RogueCity/Generators/Pipeline/TerrainConstraintGenerator.hpp
    include/RogueCity/Generators/Pipeline/PlanValidatorGenerator.hpp
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
target_compile_definitions(RogueCityGenerators PUBLIC ROGUECITY_GENERATORS_CITYSPEC_ADAPTER=1)
add_library(RogueCity::Generators ALIAS RogueCityGenerators)

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
