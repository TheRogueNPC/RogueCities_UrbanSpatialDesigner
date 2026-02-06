// FILE: AxiomEditorPanel.cpp - Integrated axiom editor with viewport
// Replaces SystemMap stub with full axiom placement functionality

#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_root.h"  // NEW: Access to shared minimap
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/App/Viewports/MinimapViewport.hpp"
#include "RogueCity/App/Viewports/ViewportSyncManager.hpp"
#include "RogueCity/App/Tools/AxiomPlacementTool.hpp"
#include "RogueCity/App/Integration/GeneratorBridge.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "ui/rc_ui_theme.h"
#include <imgui.h>
#include <string>

namespace RC_UI::Panels::AxiomEditor {

// Singleton instances (owned by this panel)
static std::unique_ptr<RogueCity::App::PrimaryViewport> s_primary_viewport;
static std::unique_ptr<RogueCity::App::ViewportSyncManager> s_sync_manager;
static std::unique_ptr<RogueCity::App::AxiomPlacementTool> s_axiom_tool;
static std::unique_ptr<RogueCity::Generators::CityGenerator> s_city_generator;
static std::unique_ptr<RogueCity::Generators::CityGenerator::CityOutput> s_city_output;
static bool s_initialized = false;
static bool s_generating = false;
static float s_generation_time = 0.0f;
static std::string s_status_message = "Ready";

void Initialize() {
    if (s_initialized) return;
    
    s_primary_viewport = std::make_unique<RogueCity::App::PrimaryViewport>();
    s_axiom_tool = std::make_unique<RogueCity::App::AxiomPlacementTool>();
    s_city_generator = std::make_unique<RogueCity::Generators::CityGenerator>();
    
    // Use shared minimap from RC_UI root
    auto* minimap = RC_UI::GetMinimapViewport();
    
    s_sync_manager = std::make_unique<RogueCity::App::ViewportSyncManager>(
        s_primary_viewport.get(),
        minimap
    );
    
    // Set initial camera position (centered on 1000,1000 city)
    s_primary_viewport->set_camera_position(RogueCity::Core::Vec2(1000.0, 1000.0), 500.0f);
    
    // Enable smooth sync by default
    s_sync_manager->set_sync_enabled(true);
    s_sync_manager->set_smooth_factor(0.2f);
    
    s_initialized = true;
}

void Shutdown() {
    s_city_output.reset();
    s_city_generator.reset();
    s_axiom_tool.reset();
    s_sync_manager.reset();
    s_primary_viewport.reset();
    s_initialized = false;
}

void Draw(float dt) {
Initialize();
    
ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorBG);
ImGui::Begin("Axiom Editor", nullptr, 
    ImGuiWindowFlags_NoCollapse | 
    ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoScrollWithMouse
);
    
// Get editor state to determine if axiom tool should be active
auto& gs = RogueCity::Core::Editor::GetGlobalState();
auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
auto current_state = hfsm.state();
    
// Enable axiom mode by default for now (always allow placement)
// TODO: Properly trigger HFSM transition to Editing_Axioms state
bool axiom_mode = true;  // Always allow axiom placement for MVP
    
// Debug: Show current state
ImGui::SetCursorScreenPos(ImVec2(10, 10));
ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 0.7f), 
    "Editor State: %d (Axiom Mode: %s)", 
    static_cast<int>(current_state),
    axiom_mode ? "ON" : "OFF"
);
    
    // Update viewport sync
    s_sync_manager->update(dt);
    
    // Render primary viewport with axiom tool integration
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 viewport_pos = ImGui::GetCursorScreenPos();
    const ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    
    // Background (Y2K grid pattern)
    draw_list->AddRectFilled(
        viewport_pos,
        ImVec2(viewport_pos.x + viewport_size.x, viewport_pos.y + viewport_size.y),
        IM_COL32(15, 20, 30, 255)
    );
    
    // Grid overlay (subtle Y2K aesthetic)
    const float grid_spacing = 50.0f;  // 50 meter grid
    const ImU32 grid_color = IM_COL32(40, 50, 70, 100);
    
    for (float x = 0; x < viewport_size.x; x += grid_spacing) {
        draw_list->AddLine(
            ImVec2(viewport_pos.x + x, viewport_pos.y),
            ImVec2(viewport_pos.x + x, viewport_pos.y + viewport_size.y),
            grid_color
        );
    }
    
    for (float y = 0; y < viewport_size.y; y += grid_spacing) {
        draw_list->AddLine(
            ImVec2(viewport_pos.x, viewport_pos.y + y),
            ImVec2(viewport_pos.x + viewport_size.x, viewport_pos.y + y),
            grid_color
        );
    }
    
    // Mouse interaction (always active for placing axioms and clicking buttons)
    const bool is_viewport_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    
    if (axiom_mode && is_viewport_hovered) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        RogueCity::Core::Vec2 world_pos = s_primary_viewport->screen_to_world(mouse_pos);
        
        // Debug: Show mouse world position
        ImGui::SetCursorScreenPos(ImVec2(10, 30));
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 0.7f), 
            "Mouse World: (%.1f, %.1f)", world_pos.x, world_pos.y);
        
        // Handle mouse events
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            s_axiom_tool->on_mouse_down(world_pos);
            s_status_message = "Axiom placed!";
        }
        
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            s_axiom_tool->on_mouse_up(world_pos);
        }
        
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            s_axiom_tool->on_mouse_move(world_pos);
        }
        
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            s_axiom_tool->on_right_click(world_pos);
            s_status_message = "Right-click detected";
        }
    }
    
    // Always update axiom tool (for animations)
    s_axiom_tool->update(dt, *s_primary_viewport);
    
    // Render axioms
    const auto& axioms = s_axiom_tool->axioms();
    for (const auto& axiom : axioms) {
        axiom->render(draw_list, *s_primary_viewport);
    }
    
    // Status text overlay (cockpit style) - moved down to avoid overlap with debug
    ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 20, viewport_pos.y + 60));
    if (axiom_mode) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "AXIOM MODE ACTIVE");
        ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 20, viewport_pos.y + 80));
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
            "Click to place axiom | Drag knobs to adjust radius");
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 0.7f), "Viewport Ready");
    }
    
    // Axiom count display
    ImGui::SetCursorScreenPos(ImVec2(
        viewport_pos.x + viewport_size.x - 120,
        viewport_pos.y + 20
    ));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.7f), 
        "Axioms: %zu", axioms.size());
    
    // Render generated city output (roads)
    if (s_city_output) {
        const auto& roads = s_city_output->roads;
        
        // Iterate using const iterators
        for (auto it = roads.begin(); it != roads.end(); ++it) {
            const auto& road = *it;
            
            if (road.points.empty()) continue;
            
            // Convert road points to screen space and draw polyline
            ImVec2 prev_screen = s_primary_viewport->world_to_screen(road.points[0]);
            
            for (size_t j = 1; j < road.points.size(); ++j) {
                ImVec2 curr_screen = s_primary_viewport->world_to_screen(road.points[j]);
                
                // Y2K road styling (bright cyan for visibility)
                ImU32 road_color = IM_COL32(0, 255, 255, 200);
                float road_width = 2.0f;
                
                draw_list->AddLine(prev_screen, curr_screen, road_color, road_width);
                prev_screen = curr_screen;
            }
        }
        
        // Road count display
        ImGui::SetCursorScreenPos(ImVec2(
            viewport_pos.x + viewport_size.x - 120,
            viewport_pos.y + 45
        ));
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 0.7f), 
            "Roads: %llu", roads.size());
    }
    
    // Generate City button (Y2K style)
    ImGui::SetCursorScreenPos(ImVec2(
        viewport_pos.x + 20,
        viewport_pos.y + viewport_size.y - 80
    ));
    
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 150, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 180, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 120, 200, 255));
    
    const bool can_generate = axioms.size() > 0 && !s_generating;
    
    if (!can_generate) {
        ImGui::BeginDisabled();
    }
    
    if (ImGui::Button("GENERATE CITY", ImVec2(180, 40))) {
        // Convert axioms to generator inputs
        auto axiom_inputs = RogueCity::App::GeneratorBridge::convert_axioms(axioms);
        
        // Configure generator
        RogueCity::Generators::CityGenerator::Config config;
        config.width = 2000;
        config.height = 2000;
        config.cell_size = 10.0;
        config.seed = static_cast<uint32_t>(ImGui::GetTime() * 1000);
        config.num_seeds = static_cast<int>(axioms.size() * 5);  // 5 seeds per axiom
        
        // Validate axioms
        if (RogueCity::App::GeneratorBridge::validate_axioms(axiom_inputs, config)) {
            s_generating = true;
            s_status_message = "Generating...";
            
            // Generate city
            auto start_time = ImGui::GetTime();
            s_city_output = std::make_unique<RogueCity::Generators::CityGenerator::CityOutput>(
                s_city_generator->generate(axiom_inputs, config)
            );
            s_generation_time = ImGui::GetTime() - start_time;
            
            // Set output to viewport for rendering
            s_primary_viewport->set_city_output(s_city_output.get());
            
            s_generating = false;
            s_status_message = "Generation complete!";
        } else {
            s_status_message = "ERROR: Invalid axioms (bounds/overlap)";
        }
    }
    
    if (!can_generate) {
        ImGui::EndDisabled();
    }
    
    ImGui::PopStyleColor(3);
    
    // Status message display
    ImGui::SetCursorScreenPos(ImVec2(
        viewport_pos.x + 210,
        viewport_pos.y + viewport_size.y - 70
    ));
    
    if (s_status_message.find("ERROR") != std::string::npos) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", s_status_message.c_str());
    } else if (s_status_message.find("complete") != std::string::npos) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%s", s_status_message.c_str());
        ImGui::SetCursorScreenPos(ImVec2(
            viewport_pos.x + 210,
            viewport_pos.y + viewport_size.y - 50
        ));
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
            "Generation time: %.2f ms", s_generation_time * 1000.0f);
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", s_status_message.c_str());
    }
    
    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Panels::AxiomEditor
