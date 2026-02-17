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
    src/Generators/Roads/StreamlineTracer.cpp
    src/Generators/Roads/RoadClassifier.cpp
    src/Generators/Roads/SegmentGridStorage.cpp
    src/Generators/Roads/RoadNoder.cpp
    src/Generators/Roads/GraphSimplify.cpp
    src/Generators/Roads/FlowAndControl.cpp
    src/Generators/Roads/Verticality.cpp
    src/Generators/Roads/IntersectionTemplates.cpp
    src/Generators/Scoring/RogueProfiler.cpp
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
    include/RogueCity/Generators/Geometry/GeometryAdapter.hpp
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
    include/RogueCity/Generators/Scoring/RogueProfiler.hpp
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

# Geometry backend selection for incremental GEOS -> Boost migration.
if(USE_LEGACY_GEOS)
    find_package(GEOS CONFIG QUIET)
    if(GEOS_FOUND)
        target_link_libraries(RogueCityGenerators PUBLIC GEOS::geos_c)
        target_compile_definitions(RogueCityGenerators PUBLIC
            ROGUECITY_USE_LEGACY_GEOS=1
            ROGUECITY_HAS_GEOS=1
        )
        message(STATUS "Generators geometry backend: GEOS")
    else()
        target_compile_definitions(RogueCityGenerators PUBLIC
            ROGUECITY_USE_LEGACY_GEOS=1
            ROGUECITY_GEOMETRY_LEGACY_SHIM=1
        )
        message(STATUS "USE_LEGACY_GEOS=ON but GEOS was not found. Using legacy shim backend.")
    endif()
else()
    find_package(Boost QUIET)
    if(Boost_FOUND)
        target_include_directories(RogueCityGenerators PRIVATE ${Boost_INCLUDE_DIRS})
        target_compile_definitions(RogueCityGenerators PUBLIC ROGUECITY_USE_BOOST_GEOMETRY=1)
        message(STATUS "Generators geometry backend: Boost.Geometry")
    else()
        target_compile_definitions(RogueCityGenerators PUBLIC
            ROGUECITY_USE_LEGACY_GEOS=1
            ROGUECITY_GEOMETRY_LEGACY_SHIM=1
        )
        message(WARNING "USE_LEGACY_GEOS=OFF but Boost was not found. Falling back to legacy shim backend.")
    endif()
endif()

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
