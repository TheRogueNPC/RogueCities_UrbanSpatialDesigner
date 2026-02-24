# ==================================================
# RogueCity Generators Library
# ==================================================
# Procedural city generation algorithms using core data types

project(RogueCityGenerators VERSION 0.1.0 LANGUAGES CXX)

message(STATUS "Configuring Generators Library...")

# Source files
set(GENERATOR_SOURCES
    src/Generators/Geometry/GeometryAdapter.cpp
    src/Generators/Tensors/TensorFieldGenerator.cpp
    src/Generators/Tensors/BasisFields.cpp
    src/Generators/Tensors/TerminalFeatureApplier.cpp
    src/Generators/Roads/StreamlineTracer.cpp
    src/Generators/Roads/RoadClassifier.cpp
    src/Generators/Roads/SegmentGridStorage.cpp
    src/Generators/Roads/RoadNoder.cpp
    src/Generators/Roads/GraphSimplify.cpp
    src/Generators/Roads/FlowAndControl.cpp
    src/Generators/Roads/Verticality.cpp
    src/Generators/Roads/IntersectionTemplates.cpp
    src/Generators/Scoring/RogueProfiler.cpp
    src/Generators/Scoring/GridAnalytics.cpp
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
    src/Generators/Pipeline/AxiomInteractionResolver.cpp
    src/Generators/Pipeline/MajorConnectorGraph.cpp
    src/Generators/Pipeline/CitySpecAdapter.cpp
    src/Generators/Pipeline/ZoningGenerator.cpp
    src/Generators/Pipeline/TerrainConstraintGenerator.cpp
    src/Generators/Pipeline/PlanValidatorGenerator.cpp
)

# Header files (for IDE integration)
set(GENERATOR_HEADERS
    include/RogueCity/Generators/Geometry/GeometryAdapter.hpp
    include/RogueCity/Generators/Tensors/TensorFieldGenerator.hpp
    include/RogueCity/Generators/Tensors/BasisFields.hpp
    include/RogueCity/Generators/Tensors/AxiomTerminalFeatures.hpp
    include/RogueCity/Generators/Tensors/TerminalFeatureApplier.hpp
    include/RogueCity/Generators/Roads/StreamlineTracer.hpp
    include/RogueCity/Generators/Roads/RoadClassifier.hpp
    include/RogueCity/Generators/Roads/PolylineRoadCandidate.hpp
    include/RogueCity/Generators/Roads/SegmentGridStorage.hpp
    include/RogueCity/Generators/Roads/RoadNoder.hpp
    include/RogueCity/Generators/Roads/GraphSimplify.hpp
    include/RogueCity/Generators/Roads/FlowAndControl.hpp
    include/RogueCity/Generators/Roads/Verticality.hpp
    include/RogueCity/Generators/Roads/IntersectionTemplates.hpp
    include/RogueCity/Generators/Scoring/RogueProfiler.hpp
    include/RogueCity/Generators/Scoring/ScoringProfile.hpp
    include/RogueCity/Generators/Scoring/GridAnalytics.hpp
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
    include/RogueCity/Generators/Pipeline/AxiomInteractionResolver.hpp
    include/RogueCity/Generators/Pipeline/MajorConnectorGraph.hpp
    include/RogueCity/Generators/Pipeline/GenerationContext.hpp
    include/RogueCity/Generators/Pipeline/GenerationStage.hpp
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

# Geometry backend selection.
# This repo uses Boost.Geometry only (header-only).
set(_roguecity_boost_hints
    "${CMAKE_SOURCE_DIR}/3rdparty/boost_1_87_0"
    "${CMAKE_SOURCE_DIR}/3rdparty/boost"
    "${CMAKE_SOURCE_DIR}/3rdparty/boost/include"
    "$ENV{BOOST_ROOT}"
    "$ENV{BOOST_INCLUDEDIR}"
    "C:/vcpkg/installed/x64-windows/include"
)

find_path(ROGUECITY_BOOST_INCLUDE_DIR
    NAMES boost/geometry.hpp
    HINTS ${_roguecity_boost_hints}
)

if(NOT ROGUECITY_BOOST_INCLUDE_DIR)
    message(FATAL_ERROR
        "Boost headers not found (boost/geometry.hpp).\n"
        "Checked hints:\n"
        "  - ${_roguecity_boost_hints}\n"
        "Set BOOST_ROOT or BOOST_INCLUDEDIR, or provide 3rdparty/boost."
    )
endif()

target_include_directories(RogueCityGenerators
    PUBLIC
        $<BUILD_INTERFACE:${ROGUECITY_BOOST_INCLUDE_DIR}>
)
target_compile_definitions(RogueCityGenerators PUBLIC ROGUECITY_USE_BOOST_GEOMETRY=1)
message(STATUS "Generators geometry backend: Boost.Geometry (${ROGUECITY_BOOST_INCLUDE_DIR})")

# Optional SLTerrainGeneration integration (header-first fallback).
if(ROGUECITY_ENABLE_SLTERRAIN)
    set(SLTERRAIN_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SLTerrainGeneration")
    set(ROGUECITY_HAS_SLTERRAIN OFF)
    if(EXISTS "${SLTERRAIN_DIR}")
        if(EXISTS "${SLTERRAIN_DIR}/CMakeLists.txt")
            add_subdirectory(
                "${SLTERRAIN_DIR}"
                "${CMAKE_BINARY_DIR}/_deps/SLTerrainGeneration"
                EXCLUDE_FROM_ALL
            )
        endif()

        if(TARGET SLTerrainGeneration)
            target_link_libraries(RogueCityGenerators PUBLIC SLTerrainGeneration)
            set(ROGUECITY_HAS_SLTERRAIN ON)
        else()
            if(NOT TARGET SLTerrainGeneration)
                add_library(SLTerrainGeneration INTERFACE)
                target_include_directories(SLTerrainGeneration INTERFACE
                    "${SLTERRAIN_DIR}"
                    "${SLTERRAIN_DIR}/include"
                )
            endif()
            target_link_libraries(RogueCityGenerators PUBLIC SLTerrainGeneration)
            set(ROGUECITY_HAS_SLTERRAIN ON)
        endif()
    endif()

    if(ROGUECITY_HAS_SLTERRAIN)
        target_compile_definitions(RogueCityGenerators PUBLIC ROGUECITY_HAS_SLTERRAIN=1)
        message(STATUS "Generators terrain backend: SLTerrainGeneration")
    else()
        message(STATUS "SLTerrainGeneration not found (expected at: ${SLTERRAIN_DIR})")
    endif()
endif()

# Link dependencies
target_link_libraries(RogueCityGenerators
    PUBLIC
        RogueCityCore    # Phase 1 core library
)

# C++20 features
target_compile_features(RogueCityGenerators PUBLIC cxx_std_20)
target_compile_definitions(RogueCityGenerators PUBLIC ROGUECITY_GENERATORS_CITYSPEC_ADAPTER=1)
add_library(RogueCity::Generators ALIAS RogueCityGenerators)

# === INSTALL RULES ===
install(TARGETS RogueCityGenerators
    EXPORT RogueCitiesTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/RogueCity/Generators
    DESTINATION include/RogueCity
    FILES_MATCHING PATTERN "*.hpp"
)

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
