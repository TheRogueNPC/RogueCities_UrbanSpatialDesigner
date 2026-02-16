
cmake_minimum_required(VERSION 3.20)

project(RogueCityVisualizer LANGUAGES CXX C)

option(ROGUECITY_BUILD_VISUALIZER "Build the (headless) ImGui visualizer example" OFF)
option(ROGUE_AI_DLC_ENABLED "Enable feature-gated AI panels (AI Console, UI Agent, City Spec)" ON)
option(ROGUE_SHIP_DEMO_MODE "Ship demo mode - completely hides AI tab and DLC features" OFF)

if(NOT ROGUECITY_BUILD_VISUALIZER)
    message(STATUS "Visualizer disabled (set -DROGUECITY_BUILD_VISUALIZER=ON to build examples)")
endif()

if(ROGUECITY_BUILD_VISUALIZER)
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/3rdparty/imgui")
set(ROGUECITY_GL3W_DIR "${CMAKE_SOURCE_DIR}/3rdparty/gl3w")

function(rc_configure_msvc_pdb target base_name)
    if(NOT MSVC)
        return()
    endif()

    target_compile_options(${target} PRIVATE /FS)
    set_target_properties(${target} PROPERTIES
        COMPILE_PDB_NAME "${base_name}_$<CONFIG>"
        PDB_NAME "${base_name}_$<CONFIG>"
    )

    foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${cfg}" cfg_upper)
        set_target_properties(${target} PROPERTIES
            "COMPILE_PDB_OUTPUT_DIRECTORY_${cfg_upper}" "${CMAKE_BINARY_DIR}/pdb/compile/${cfg}"
            "PDB_OUTPUT_DIRECTORY_${cfg_upper}" "${CMAKE_BINARY_DIR}/pdb/link/${cfg}"
        )
    endforeach()
endfunction()

if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "ImGui not found at ${IMGUI_DIR}. Run: git submodule update --init --recursive")
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

    add_library(RogueCityVisualizerFeaturePanels STATIC
        src/ui/panels/rc_panel_tools.cpp
        src/ui/panels/rc_panel_ai_console.cpp
        src/ui/panels/rc_panel_ui_agent.cpp
        src/ui/panels/rc_panel_city_spec.cpp
    )

    target_include_directories(RogueCityVisualizerFeaturePanels PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/3rdparty/nlohmann_json/include"
        "${IMGUI_DIR}"
    )
    target_link_libraries(RogueCityVisualizerFeaturePanels PRIVATE
        RogueCityCore
        RogueCityGenerators
        RogueCityApp
        RogueCityAI
        RogueCityImGui
    )
    target_compile_features(RogueCityVisualizerFeaturePanels PRIVATE cxx_std_20)

    # Define AI DLC flag for conditional compilation of feature panels.
    if(ROGUE_SHIP_DEMO_MODE)
        target_compile_definitions(RogueCityVisualizerFeaturePanels PUBLIC ROGUE_AI_DLC_ENABLED=0)
    elseif(ROGUE_AI_DLC_ENABLED)
        target_compile_definitions(RogueCityVisualizerFeaturePanels PUBLIC ROGUE_AI_DLC_ENABLED=1)
    endif()

    rc_configure_msvc_pdb(RogueCityVisualizerFeaturePanels "RogueCityVisualizerFeaturePanels")

    add_executable(RogueCityVisualizerHeadless
        # Hyper-reactive UI module sources.
        src/main.cpp
        src/ui/rc_ui_anim.cpp
        src/ui/rc_ui_input_gate.cpp
        src/ui/rc_ui_root.cpp
        src/ui/rc_ui_theme.cpp
        src/ui/tools/rc_tool_contract.cpp
        src/ui/tools/rc_tool_dispatcher.cpp
        src/ui/tools/rc_tool_interaction_metrics.cpp
        src/ui/tools/rc_tool_geometry_policy.cpp
        src/ui/commands/rc_context_command_registry.cpp
        src/ui/commands/rc_context_menu_smart.cpp
        src/ui/commands/rc_context_menu_pie.cpp
        src/ui/commands/rc_command_palette.cpp
        src/ui/introspection/UiIntrospection.cpp
        # Panels
        src/ui/panels/rc_panel_axiom_bar.cpp
        src/ui/panels/rc_panel_axiom_editor.cpp
        src/ui/panels/rc_panel_system_map.cpp
        src/ui/panels/rc_panel_log.cpp
        src/ui/panels/rc_panel_telemetry.cpp
        src/ui/panels/rc_panel_inspector.cpp
        src/ui/panels/rc_panel_ui_settings.cpp
        src/ui/panels/rc_property_editor.cpp
        src/ui/panels/rc_panel_district_index.cpp
        src/ui/panels/rc_panel_road_index.cpp
        src/ui/panels/rc_panel_lot_index.cpp
        src/ui/panels/rc_panel_river_index.cpp
        src/ui/panels/rc_panel_dev_shell.cpp
        # AI_INTEGRATION_TAG: V1_PASS1_TASK4_NEW_CONTROL_PANELS
        src/ui/panels/rc_panel_lot_control.cpp
        src/ui/panels/rc_panel_building_control.cpp
        src/ui/panels/rc_panel_water_control.cpp
        # RC-0.10: Zoning control and viewport overlays
        src/ui/panels/rc_panel_zoning_control.cpp
        src/ui/viewport/rc_viewport_overlays.cpp
        src/ui/viewport/rc_scene_frame.cpp
        src/ui/viewport/rc_minimap_renderer.cpp
        src/ui/viewport/rc_viewport_scene_controller.cpp
        src/ui/viewport/rc_viewport_interaction.cpp
        # Master Panel Architecture (RC-0.10)
        src/ui/panels/IPanelDrawer.cpp
        src/ui/panels/PanelRegistry.cpp
        src/ui/panels/RcMasterPanel.cpp
        src/ui/panels/RcPanelDrawers.cpp
    )

    target_link_libraries(RogueCityVisualizerHeadless PRIVATE
        RogueCityCore
        RogueCityGenerators
        RogueCityApp
        RogueCityAI
        RogueCityImGui
        RogueCityVisualizerFeaturePanels
    )
    # Expose UI module headers.
    target_include_directories(RogueCityVisualizerHeadless PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/3rdparty/nlohmann_json/include"
    )
    target_compile_features(RogueCityVisualizerHeadless PRIVATE cxx_std_20)
    
    # Define AI DLC flag for conditional compilation
    # SHIP_DEMO_MODE overrides and disables AI features completely
    if(ROGUE_SHIP_DEMO_MODE)
        message(STATUS "Demo Mode: AI features disabled (ROGUE_SHIP_DEMO_MODE=ON)")
        target_compile_definitions(RogueCityVisualizerHeadless PRIVATE ROGUE_AI_DLC_ENABLED=0)
    elseif(ROGUE_AI_DLC_ENABLED)
        target_compile_definitions(RogueCityVisualizerHeadless PRIVATE ROGUE_AI_DLC_ENABLED=1)
    endif()
    
    rc_configure_msvc_pdb(RogueCityVisualizerHeadless "RogueCityVisualizerHeadless")
    
    # ==================================================
    # GUI executable (GLFW + OpenGL3 + ImGui)
    # ==================================================
    find_package(OpenGL REQUIRED)
    
    # Build GLFW from source (official glfw/glfw submodule)
    set(GLFW_DIR "${CMAKE_SOURCE_DIR}/3rdparty/glfw")
    
    if(EXISTS "${GLFW_DIR}/CMakeLists.txt")
        # Configure GLFW to build as static library without examples/tests/docs
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
        
        add_subdirectory("${GLFW_DIR}" "${CMAKE_BINARY_DIR}/glfw" EXCLUDE_FROM_ALL)
        
        # GLFW target is now 'glfw' from add_subdirectory
        if(TARGET glfw)
            
            add_executable(RogueCityVisualizerGui
                src/main_gui.cpp
                # UI system sources
                src/ui/rc_ui_anim.cpp
                src/ui/rc_ui_input_gate.cpp
                src/ui/rc_ui_root.cpp
                src/ui/rc_ui_theme.cpp
                src/ui/tools/rc_tool_contract.cpp
                src/ui/tools/rc_tool_dispatcher.cpp
                src/ui/tools/rc_tool_interaction_metrics.cpp
                src/ui/tools/rc_tool_geometry_policy.cpp
                src/ui/commands/rc_context_command_registry.cpp
                src/ui/commands/rc_context_menu_smart.cpp
                src/ui/commands/rc_context_menu_pie.cpp
                src/ui/commands/rc_command_palette.cpp
                src/ui/introspection/UiIntrospection.cpp
                # Panel sources
                src/ui/panels/rc_panel_axiom_bar.cpp
                src/ui/panels/rc_panel_axiom_editor.cpp  # NEW: Integrated axiom editor
                src/ui/panels/rc_panel_system_map.cpp
                src/ui/panels/rc_panel_log.cpp
                src/ui/panels/rc_panel_telemetry.cpp
                src/ui/panels/rc_panel_inspector.cpp
                src/ui/panels/rc_panel_ui_settings.cpp
                src/ui/panels/rc_property_editor.cpp
                src/ui/panels/rc_panel_district_index.cpp
                src/ui/panels/rc_panel_road_index.cpp
                src/ui/panels/rc_panel_lot_index.cpp
                src/ui/panels/rc_panel_river_index.cpp
                src/ui/panels/rc_panel_building_index.cpp  # NEW: Building index panel (RC-0.10)
                src/ui/panels/rc_panel_zoning_control.cpp  # NEW: Zoning control panel (RC-0.10)
                # AI_INTEGRATION_TAG: V1_PASS1_TASK4_NEW_CONTROL_PANELS
                src/ui/panels/rc_panel_lot_control.cpp
                src/ui/panels/rc_panel_building_control.cpp
                src/ui/panels/rc_panel_water_control.cpp
                src/ui/panels/rc_panel_dev_shell.cpp
                # Master Panel Architecture (RC-0.10)
                src/ui/panels/IPanelDrawer.cpp
                src/ui/panels/PanelRegistry.cpp
                src/ui/panels/RcMasterPanel.cpp
                src/ui/panels/RcPanelDrawers.cpp
                # Viewport overlays (RC-0.10)
                src/ui/viewport/rc_viewport_overlays.cpp
                src/ui/viewport/rc_scene_frame.cpp
                src/ui/viewport/rc_minimap_renderer.cpp
                src/ui/viewport/rc_viewport_scene_controller.cpp
                src/ui/viewport/rc_viewport_interaction.cpp
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
                "${CMAKE_SOURCE_DIR}/3rdparty/nlohmann_json/include"
                "${IMGUI_DIR}"
                "${IMGUI_DIR}/backends"
                "${ROGUECITY_GL3W_DIR}"
            )

            target_compile_definitions(RogueCityVisualizerGui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GL3W=1)
            target_link_libraries(RogueCityVisualizerGui PRIVATE 
                RogueCityCore 
                RogueCityGenerators  # NEW: For CityGenerator integration
                RogueCityApp         # NEW: For viewport and axiom tools
                RogueCityAI          # NEW: For AI assist controls
                RogueCityImGui 
                RogueCityVisualizerFeaturePanels
                OpenGL::GL 
                glfw
            )
            
            # Link sol2/lua if available (for UI scripting/toolchain)
            if(TARGET sol2::sol2)
                target_link_libraries(RogueCityVisualizerGui PRIVATE sol2::sol2)
                target_compile_definitions(RogueCityVisualizerGui PRIVATE ROGUECITY_HAS_SOL2=1)
            endif()

            if(ROGUECITY_HAS_LINPUT)
                target_link_libraries(RogueCityVisualizerGui PRIVATE LInput::LInput)
                target_compile_definitions(RogueCityVisualizerGui PRIVATE ROGUECITY_HAS_LINPUT=1)
            endif()

            
            # Link legacy_stdio_definitions for MSVC compatibility with older GLFW binaries
            if(MSVC AND MSVC_VERSION GREATER_EQUAL 1900)
                target_link_libraries(RogueCityVisualizerGui PRIVATE legacy_stdio_definitions)
            endif()
            
            target_compile_features(RogueCityVisualizerGui PRIVATE cxx_std_20)
            
            # Define AI DLC flag for conditional compilation (same as headless)
            # SHIP_DEMO_MODE overrides and disables AI features completely
            if(ROGUE_SHIP_DEMO_MODE)
                message(STATUS "Demo Mode: AI features disabled (ROGUE_SHIP_DEMO_MODE=ON)")
                target_compile_definitions(RogueCityVisualizerGui PRIVATE ROGUE_AI_DLC_ENABLED=0)
            elseif(ROGUE_AI_DLC_ENABLED)
                target_compile_definitions(RogueCityVisualizerGui PRIVATE ROGUE_AI_DLC_ENABLED=1)
            endif()
            
            rc_configure_msvc_pdb(RogueCityVisualizerGui "RogueCityVisualizerGui")
            
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
