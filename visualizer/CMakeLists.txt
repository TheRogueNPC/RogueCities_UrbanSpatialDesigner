cmake_minimum_required(VERSION 3.20)

project(RogueCityVisualizer LANGUAGES CXX C)

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
        # ADDED (visualizer/CMakeLists.txt): Hyper-reactive UI module sources.
        src/main.cpp
        src/ui/rc_ui_anim.cpp
        src/ui/rc_ui_root.cpp
        src/ui/rc_ui_theme.cpp
        src/ui/panels/rc_panel_axiom_bar.cpp
        src/ui/panels/rc_panel_inspector.cpp
        src/ui/panels/rc_panel_log.cpp
        src/ui/panels/rc_panel_system_map.cpp
        src/ui/panels/rc_panel_telemetry.cpp
    )

    target_link_libraries(RogueCityVisualizerHeadless PRIVATE RogueCityCore RogueCityImGui)
    # ADDED (visualizer/CMakeLists.txt): Expose UI module headers.
    target_include_directories(RogueCityVisualizerHeadless PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_features(RogueCityVisualizerHeadless PRIVATE cxx_std_20)
    
    # GUI executable (GLFW + OpenGL3) using ImGui backends and bundled GLFW
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
                "${IMGUI_DIR}/examples/imgui_impl_glfw.cpp"
                "${IMGUI_DIR}/examples/imgui_impl_opengl3.cpp"
            )
            
            # Add gl3w.c as a separate C source (not C++)
            target_sources(RogueCityVisualizerGui PRIVATE "${IMGUI_DIR}/examples/libs/gl3w/GL/gl3w.c")
            set_source_files_properties("${IMGUI_DIR}/examples/libs/gl3w/GL/gl3w.c" PROPERTIES LANGUAGE C)

            target_include_directories(RogueCityVisualizerGui PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/src"
                "${IMGUI_DIR}"
                "${IMGUI_DIR}/examples"
                "${IMGUI_DIR}/examples/libs/gl3w"
                "${GLFW_DIR}/include"
            )

            target_compile_definitions(RogueCityVisualizerGui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GL3W=1)
            target_link_libraries(RogueCityVisualizerGui PRIVATE RogueCityCore RogueCityImGui OpenGL::GL glfw_bundled)
            
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
