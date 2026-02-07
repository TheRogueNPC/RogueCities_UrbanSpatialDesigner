
cmake_minimum_required(VERSION 3.20)

project(RogueCityVisualizer LANGUAGES CXX C)

option(ROGUECITY_BUILD_VISUALIZER "Build the (headless) ImGui visualizer example" OFF)

if(NOT ROGUECITY_BUILD_VISUALIZER)
    message(STATUS "Visualizer disabled (set -DROGUECITY_BUILD_VISUALIZER=ON to build examples)")
endif()

if(ROGUECITY_BUILD_VISUALIZER)
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/3rdparty/imvue/imgui")
set(ROGUECITY_GL3W_DIR "${CMAKE_SOURCE_DIR}/3rdparty/gl3w")

if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "ImGui not found at ${IMGUI_DIR}. Ensure 3rdparty/imvue submodules are initialized.")
endif()

# Only create RogueCityImGui if it doesn't already exist (from app layer)
if(NOT TARGET RogueCityImGui)
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
    
    # ==================================================
    # GUI executable (GLFW + OpenGL3 + ImGui)
    # ==================================================
    find_package(OpenGL REQUIRED)
    
    # Use bundled GLFW from imgui examples
    set(GLFW_DIR "${CMAKE_SOURCE_DIR}/3rdparty/imvue/imgui/examples/libs/glfw")
    
    if(EXISTS "${GLFW_DIR}/include/GLFW/glfw3.h")
        # Detect architecture and select appropriate GLFW library
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(GLFW_LIB_DIR "${GLFW_DIR}/lib-vc2010-64")
        else()
            set(GLFW_LIB_DIR "${GLFW_DIR}/lib-vc2010-32")
        endif()
        
        if(EXISTS "${GLFW_LIB_DIR}/glfw3.lib")
            add_library(glfw_bundled STATIC IMPORTED)
            set_target_properties(glfw_bundled PROPERTIES
                IMPORTED_LOCATION "${GLFW_LIB_DIR}/glfw3.lib"
                INTERFACE_INCLUDE_DIRECTORIES "${GLFW_DIR}/include"
            )
            
            add_executable(RogueCityVisualizerGui
                src/main_gui.cpp
                # UI system sources
                src/ui/rc_ui_anim.cpp
                src/ui/rc_ui_root.cpp
                src/ui/rc_ui_theme.cpp
                # Panel sources
                src/ui/panels/rc_panel_axiom_bar.cpp
                src/ui/panels/rc_panel_axiom_editor.cpp  # NEW: Integrated axiom editor
                src/ui/panels/rc_panel_system_map.cpp
                src/ui/panels/rc_panel_log.cpp
                src/ui/panels/rc_panel_telemetry.cpp
                src/ui/panels/rc_panel_tools.cpp
                src/ui/panels/rc_panel_district_index.cpp
                src/ui/panels/rc_panel_road_index.cpp
                src/ui/panels/rc_panel_lot_index.cpp
                src/ui/panels/rc_panel_river_index.cpp
                src/ui/panels/rc_panel_ai_console.cpp  # NEW: AI bridge control
                src/ui/panels/rc_panel_ui_agent.cpp    # NEW: UI Agent assistant (Phase 2)
                src/ui/panels/rc_panel_city_spec.cpp   # NEW: CitySpec generator (Phase 3)
                # ImGui backend implementations
                "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
                "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
            )
            
            # Add gl3w.c as a separate C source (not C++).
            # Newer Dear ImGui no longer ships the gl3w loader; we keep it vendored at 3rdparty/gl3w.
            target_sources(RogueCityVisualizerGui PRIVATE "${ROGUECITY_GL3W_DIR}/GL/gl3w.c")
            set_source_files_properties("${ROGUECITY_GL3W_DIR}/GL/gl3w.c" PROPERTIES LANGUAGE C)

            target_include_directories(RogueCityVisualizerGui PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/src"
                "${IMGUI_DIR}"
                "${IMGUI_DIR}/backends"
                "${ROGUECITY_GL3W_DIR}"
                "${GLFW_DIR}/include"
            )

            target_compile_definitions(RogueCityVisualizerGui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GL3W=1)
            target_link_libraries(RogueCityVisualizerGui PRIVATE 
                RogueCityCore 
                RogueCityGenerators  # NEW: For CityGenerator integration
                RogueCityApp         # NEW: For viewport and axiom tools
                RogueCityAI          # NEW: For AI assist controls
                RogueCityImGui 
                OpenGL::GL 
                glfw_bundled
            )

            if(ROGUECITY_HAS_LINPUT)
                target_link_libraries(RogueCityVisualizerGui PRIVATE LInput::LInput)
                target_compile_definitions(RogueCityVisualizerGui PRIVATE ROGUECITY_HAS_LINPUT=1)
            endif()
            
            # Link legacy_stdio_definitions for MSVC compatibility with older GLFW binaries
            if(MSVC AND MSVC_VERSION GREATER_EQUAL 1900)
                target_link_libraries(RogueCityVisualizerGui PRIVATE legacy_stdio_definitions)
            endif()
            
            target_compile_features(RogueCityVisualizerGui PRIVATE cxx_std_20)
            
            # Disable precompiled headers for this target (mixed C/C++)
            set_target_properties(RogueCityVisualizerGui PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)
            
            message(STATUS "GUI Visualizer enabled (using bundled GLFW from ${GLFW_LIB_DIR})")
        else()
            message(STATUS "Bundled GLFW library not found at ${GLFW_LIB_DIR}")
        endif()
    else()
        message(STATUS "Bundled GLFW not found; GUI visualizer disabled.")
    endif()
endif()
