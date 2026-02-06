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
        src/main.cpp
    )

    target_link_libraries(RogueCityVisualizerHeadless PRIVATE RogueCityCore RogueCityImGui)
    target_compile_features(RogueCityVisualizerHeadless PRIVATE cxx_std_20)
endif()

