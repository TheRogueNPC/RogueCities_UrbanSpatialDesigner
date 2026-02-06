
cmake_minimum_required(VERSION 3.20)

project(RogueCityVisualizer LANGUAGES CXX)

option(ROGUECITY_BUILD_VISUALIZER "Build the (headless) ImGui visualizer example" OFF)

if(NOT ROGUECITY_BUILD_VISUALIZER)
    message(STATUS "Visualizer disabled (set -DROGUECITY_BUILD_VISUALIZER=ON to build examples)")
endif()

if(ROGUECITY_BUILD_VISUALIZER)
    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/3rdparty/imvue/imgui")

    if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
        message(FATAL_ERROR "ImGui not found at ${IMGUI_DIR}. Ensure 3rdparty/imvue submodules are initialized.")
    endif()

    add_library(RogueCityImGui STATIC
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
    )

    target_include_directories(RogueCityImGui PUBLIC "${IMGUI_DIR}")
    target_compile_features(RogueCityImGui PUBLIC cxx_std_20)

    add_executable(RogueCityVisualizerHeadless
        # Hyper-reactive UI module sources.
        src/main.cpp
        src/ui/rc_ui_anim.cpp
        src/ui/rc_ui_root.cpp
        src/ui/rc_ui_theme.cpp
        # Panels
        src/ui/panels/rc_panel_axiom_bar.cpp
        src/ui/panels/rc_panel_system_map.cpp
        src/ui/panels/rc_panel_log.cpp
        src/ui/panels/rc_panel_telemetry.cpp
        src/ui/panels/rc_panel_tools.cpp
        src/ui/panels/rc_panel_district_index.cpp
        src/ui/panels/rc_panel_road_index.cpp
        src/ui/panels/rc_panel_lot_index.cpp
        src/ui/panels/rc_panel_river_index.cpp
    )

    target_link_libraries(RogueCityVisualizerHeadless PRIVATE RogueCityCore RogueCityImGui)
    # Expose UI module headers.
    target_include_directories(RogueCityVisualizerHeadless PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_features(RogueCityVisualizerHeadless PRIVATE cxx_std_20)
endif()