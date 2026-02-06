# ==================================================
# RogueCity App Layer
# ==================================================
# ImGui-based UI components for axiom placement, viewports, and docking
# Implements Cockpit Doctrine: Vignelli structure + Y2K geometry + LCARS segmentation

project(RogueCityApp VERSION 0.1.0 LANGUAGES CXX)

message(STATUS "Configuring App Layer...")

# === SOURCE FILES ===
set(APP_SOURCES
    # Tools (Axiom placement and visualization)
    src/Tools/AxiomVisual.cpp
    src/Tools/AxiomAnimationController.cpp
    src/Tools/AxiomPlacementTool.cpp
    src/Tools/AxiomIcon.cpp
    src/Tools/ContextWindowPopup.cpp
    
    # Viewports (3D/2D hybrid rendering)
    src/Viewports/PrimaryViewport.cpp
    src/Viewports/MinimapViewport.cpp
    src/Viewports/ViewportSyncManager.cpp
    
    # Docking (State-reactive panel management)
    src/Docking/DockLayoutManager.cpp
    
    # Integration (Generator bridge)
    src/Integration/GeneratorBridge.cpp
    src/Integration/RealTimePreview.cpp
)

set(APP_HEADERS
    # Tools
    include/RogueCity/App/Tools/AxiomVisual.hpp
    include/RogueCity/App/Tools/AxiomAnimationController.hpp
    include/RogueCity/App/Tools/AxiomPlacementTool.hpp
    include/RogueCity/App/Tools/AxiomIcon.hpp
    include/RogueCity/App/Tools/ContextWindowPopup.hpp
    
    # Viewports
    include/RogueCity/App/Viewports/PrimaryViewport.hpp
    include/RogueCity/App/Viewports/MinimapViewport.hpp
    include/RogueCity/App/Viewports/ViewportSyncManager.hpp
    include/RogueCity/App/Viewports/ViewportManager.hpp
    
    # Docking
    include/RogueCity/App/Docking/DockLayoutManager.hpp
    include/RogueCity/App/Docking/PanelRegistry.hpp
    
    # Integration
    include/RogueCity/App/Integration/GeneratorBridge.hpp
    include/RogueCity/App/Integration/RealTimePreview.hpp
)

# === IMGUI LIBRARY TARGET ===
# Check if ImGui target already exists (from visualizer)
if(NOT TARGET RogueCityImGui)
    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/3rdparty/imvue/imgui")
    
    if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
        message(FATAL_ERROR 
            "ImGui not found at ${IMGUI_DIR}.\n"
            "Run: cd 3rdparty && git submodule update --init --recursive"
        )
    endif()
    
    add_library(RogueCityImGui STATIC
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
    )
    
    target_include_directories(RogueCityImGui PUBLIC "${IMGUI_DIR}")
    target_compile_features(RogueCityImGui PUBLIC cxx_std_20)
endif()

# === LIBRARY TARGET ===
add_library(RogueCityApp STATIC ${APP_SOURCES} ${APP_HEADERS})

# === PUBLIC INTERFACE ===
target_include_directories(RogueCityApp 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# === DEPENDENCIES ===
target_link_libraries(RogueCityApp
    PUBLIC
        RogueCityCore          # Core data types and math
        RogueCityGenerators    # CityGenerator pipeline
        RogueCityImGui         # ImGui interface
        glm::glm               # Vector math
)

# === C++ STANDARD ===
target_compile_features(RogueCityApp PUBLIC cxx_std_20)

# === PRECOMPILED HEADERS (FAST BUILDS) ===
target_precompile_headers(RogueCityApp PUBLIC
    <vector>
    <array>
    <memory>
    <cstdint>
    <cmath>
    <string>
    <algorithm>
    <imgui.h>
)

# === COMPILER WARNINGS ===
if(MSVC)
    target_compile_options(RogueCityApp PRIVATE 
        /W4           # High warning level
        # /WX removed temporarily for Phase 1 integration
        /permissive-  # Strict standard conformance
    )
else()
    target_compile_options(RogueCityApp PRIVATE 
        -Wall -Wextra -pedantic
    )
endif()

# === COCKPIT DOCTRINE COMPLIANCE ===
# Ensure this layer follows the design principles:
# - Y2K Geometry: Hard-edged shapes, capsule knobs, warning stripes
# - Affordance-Rich: Visual feedback on hover/drag
# - State-Reactive: Docking layouts change per HFSM state
# - Motion as Instruction: Animations teach spatial relationships

message(STATUS "App layer configured: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "  - Axiom tools with Y2K visual feedback")
message(STATUS "  - State-reactive viewport sync")
message(STATUS "  - HFSM-driven docking layouts")
