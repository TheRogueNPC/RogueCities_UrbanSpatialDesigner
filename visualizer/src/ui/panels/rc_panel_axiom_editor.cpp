// FILE: AxiomEditorPanel.cpp - Integrated axiom editor with viewport
// Replaces SystemMap stub with full axiom placement functionality

#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_root.h"  // NEW: Access to shared minimap
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/App/Viewports/MinimapViewport.hpp"
#include "RogueCity/App/Viewports/ViewportSyncManager.hpp"
#include "RogueCity/App/Tools/AxiomPlacementTool.hpp"
#include "RogueCity/App/Integration/GeneratorBridge.hpp"
#include "RogueCity/App/Integration/RealTimePreview.hpp"
#include "RogueCity/App/Tools/AxiomIcon.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "ui/rc_ui_theme.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <numbers>
#include <string>

namespace RC_UI::Panels::AxiomEditor {

namespace {
    using Preview = RogueCity::App::RealTimePreview;

    void DrawAxiomModeStatus(const Preview& preview, ImVec2 pos) {
        ImGui::SetCursorScreenPos(pos);

        const auto phase = preview.phase();
        const float t = preview.phase_elapsed_seconds();

        if (phase == Preview::GenerationPhase::InitStreetSweeper) {
            const char* text = "_INIT_STREETSWEEPER";
            const int len = static_cast<int>(std::strlen(text));
            const float reveal = std::clamp(t / 0.25f, 0.0f, 1.0f);
            const int visible = std::clamp(static_cast<int>(reveal * static_cast<float>(len)), 0, len);

            ImGui::TextColored(ImVec4(1.0f, 0.15f, 0.15f, 1.0f), "%.*s", visible, text);
            return;
        }

        if (phase == Preview::GenerationPhase::Sweeping) {
            const float p = preview.get_progress();
            ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.0f, 1.0f), "_SWEEPING  %.0f%%", p * 100.0f);
            return;
        }

        if (phase == Preview::GenerationPhase::StreetsSwept) {
            const float fade = std::clamp(t / 1.25f, 0.0f, 1.0f);
            const ImVec4 color(1.0f - fade, fade, 0.0f, 1.0f - fade * 0.5f);
            ImGui::TextColored(color, "_STREETS_SWEPT");
            return;
        }

        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.95f), "AXIOM MODE ACTIVE");
    }
}

// Singleton instances (owned by this panel)
static std::unique_ptr<RogueCity::App::PrimaryViewport> s_primary_viewport;
static std::unique_ptr<RogueCity::App::ViewportSyncManager> s_sync_manager;
static std::unique_ptr<RogueCity::App::AxiomPlacementTool> s_axiom_tool;
static std::unique_ptr<RogueCity::App::RealTimePreview> s_preview;
static bool s_initialized = false;
static bool s_live_preview = true;
static float s_debounce_seconds = 0.30f;
static uint32_t s_seed = 42u;
static std::string s_validation_error;

void Initialize() {
    if (s_initialized) return;
    
    s_primary_viewport = std::make_unique<RogueCity::App::PrimaryViewport>();
    s_axiom_tool = std::make_unique<RogueCity::App::AxiomPlacementTool>();
    s_preview = std::make_unique<RogueCity::App::RealTimePreview>();
    s_preview->set_debounce_delay(s_debounce_seconds);
    
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

    s_preview->set_on_complete([minimap](const RogueCity::Generators::CityGenerator::CityOutput& output) {
        if (s_primary_viewport) {
            s_primary_viewport->set_city_output(&output);
        }
        if (minimap) {
            minimap->set_city_output(&output);
        }
    });
    
    s_initialized = true;
}

void Shutdown() {
    s_preview.reset();
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
    
    // Update viewport sync + preview scheduler
    s_sync_manager->update(dt);
    if (s_preview) {
        s_preview->update(dt);
    }
    
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
    
    // Mouse interaction is handled after overlay windows are submitted so hover tests are correct.

    // ---------------------------------------------------------------------
    // Overlay windows: Palette + Properties
    // ---------------------------------------------------------------------
    bool ui_modified_axiom = false;

    // Axiom Library (palette)
    {
        const ImVec2 palette_pos(viewport_pos.x + 12.0f, viewport_pos.y + 110.0f);
        ImGui::SetNextWindowPos(palette_pos);
        ImGui::SetNextWindowSize(ImVec2(310.0f, 158.0f));
        ImGui::Begin("Axiom Library", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings
        );

        ImGui::TextColored(ImVec4(1, 1, 1, 0.85f), "Axiom Library");

        const float icon_size = 46.0f;
        const int columns = 5;
        const auto infos = RogueCity::App::GetAxiomTypeInfos();

        const auto default_type = s_axiom_tool->default_axiom_type();
        const bool apply_to_selected = ImGui::GetIO().KeyCtrl;

        for (int i = 0; i < static_cast<int>(infos.size()); ++i) {
            const auto& info = infos[static_cast<size_t>(i)];
            if (i > 0 && (i % columns) != 0) {
                ImGui::SameLine();
            }

            ImGui::PushID(i);
            if (ImGui::InvisibleButton("AxiomType", ImVec2(icon_size, icon_size))) {
                if (apply_to_selected) {
                    if (auto* selected = s_axiom_tool->get_selected_axiom()) {
                        selected->set_type(info.type);
                        ui_modified_axiom = true;
                    }
                } else {
                    s_axiom_tool->set_default_axiom_type(info.type);
                }
            }

            const ImVec2 bmin = ImGui::GetItemRectMin();
            const ImVec2 bmax = ImGui::GetItemRectMax();
            const ImVec2 c((bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f);

            ImDrawList* dl = ImGui::GetWindowDrawList();
            const bool is_default = (default_type == info.type);
            const ImU32 bg = IM_COL32(0, 0, 0, ImGui::IsItemHovered() ? 200 : 140);
            const ImU32 border = is_default ? IM_COL32(255, 255, 255, 220) : IM_COL32(80, 90, 110, 160);

            dl->AddRectFilled(bmin, bmax, bg, 8.0f);
            dl->AddRect(bmin, bmax, border, 8.0f, 0, is_default ? 2.5f : 1.5f);
            RogueCity::App::DrawAxiomIcon(dl, c, icon_size * 0.32f, info.type, info.primary_color);

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s%s", info.name, apply_to_selected ? " (Ctrl: apply to selection)" : "");
            }
            ImGui::PopID();
        }

        ImGui::End();
    }

    // Axiom Properties (selection)
    {
        const ImVec2 props_pos(viewport_pos.x + viewport_size.x - 330.0f, viewport_pos.y + 110.0f);
        ImGui::SetNextWindowPos(props_pos);
        ImGui::SetNextWindowSize(ImVec2(318.0f, 240.0f));
        ImGui::Begin("Axiom Properties", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings
        );

        ImGui::TextColored(ImVec4(1, 1, 1, 0.85f), "Axiom Properties");

        auto* selected = s_axiom_tool->get_selected_axiom();
        if (!selected) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.9f), "No axiom selected.");
        } else {
            // Type dropdown
            const auto infos = RogueCity::App::GetAxiomTypeInfos();
            int current_index = 0;
            for (int i = 0; i < static_cast<int>(infos.size()); ++i) {
                if (infos[static_cast<size_t>(i)].type == selected->type()) {
                    current_index = i;
                    break;
                }
            }

            if (ImGui::BeginCombo("Type", infos[static_cast<size_t>(current_index)].name)) {
                for (int i = 0; i < static_cast<int>(infos.size()); ++i) {
                    const bool is_selected = (i == current_index);
                    if (ImGui::Selectable(infos[static_cast<size_t>(i)].name, is_selected)) {
                        selected->set_type(infos[static_cast<size_t>(i)].type);
                        ui_modified_axiom = true;
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // Common properties
            float radius = selected->radius();
            if (ImGui::DragFloat("Radius", &radius, 1.0f, 50.0f, 1000.0f, "%.0fm")) {
                selected->set_radius(radius);
                ui_modified_axiom = true;
            }

            float pos[2] = { static_cast<float>(selected->position().x), static_cast<float>(selected->position().y) };
            if (ImGui::DragFloat2("Position", pos, 1.0f, 0.0f, 2000.0f, "%.0f")) {
                selected->set_position(RogueCity::Core::Vec2(pos[0], pos[1]));
                ui_modified_axiom = true;
            }

            float theta = selected->rotation();
            if (ImGui::DragFloat("Theta", &theta, 0.01f, -std::numbers::pi_v<float>, std::numbers::pi_v<float>, "%.2f")) {
                selected->set_rotation(theta);
                ui_modified_axiom = true;
            }

            // Type-specific properties
            switch (selected->type()) {
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::Organic: {
                float curv = selected->organic_curviness();
                if (ImGui::SliderFloat("Curviness", &curv, 0.0f, 1.0f)) {
                    selected->set_organic_curviness(curv);
                    ui_modified_axiom = true;
                }
                break;
            }
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::Radial: {
                int spokes = selected->radial_spokes();
                if (ImGui::SliderInt("Spokes", &spokes, 3, 24)) {
                    selected->set_radial_spokes(spokes);
                    ui_modified_axiom = true;
                }
                break;
            }
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::LooseGrid: {
                float jitter = selected->loose_grid_jitter();
                if (ImGui::SliderFloat("Jitter", &jitter, 0.0f, 1.0f)) {
                    selected->set_loose_grid_jitter(jitter);
                    ui_modified_axiom = true;
                }
                break;
            }
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::Suburban: {
                float loop = selected->suburban_loop_strength();
                if (ImGui::SliderFloat("Loop Strength", &loop, 0.0f, 1.0f)) {
                    selected->set_suburban_loop_strength(loop);
                    ui_modified_axiom = true;
                }
                break;
            }
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::Stem: {
                float branch = selected->stem_branch_angle();
                if (ImGui::SliderAngle("Branch Angle", &branch, 0.0f, 90.0f)) {
                    selected->set_stem_branch_angle(branch);
                    ui_modified_axiom = true;
                }
                break;
            }
            case RogueCity::Generators::CityGenerator::AxiomInput::Type::Superblock: {
                float block = selected->superblock_block_size();
                if (ImGui::DragFloat("Block Size", &block, 1.0f, 50.0f, 600.0f, "%.0fm")) {
                    selected->set_superblock_block_size(block);
                    ui_modified_axiom = true;
                }
                break;
            }
            default:
                break;
            }
        }

        ImGui::End();
    }

    // ---------------------------------------------------------------------
    // Viewport mouse interaction (placement + manipulation)
    // ---------------------------------------------------------------------
    const ImVec2 mouse_pos = ImGui::GetMousePos();
    const ImVec2 vp_min = viewport_pos;
    const ImVec2 vp_max(viewport_pos.x + viewport_size.x, viewport_pos.y + viewport_size.y);

    const ImVec2 gen_min(viewport_pos.x + 20.0f, vp_max.y - 82.0f);
    const ImVec2 gen_max(viewport_pos.x + 20.0f + 180.0f, vp_max.y - 40.0f);
    const ImVec2 bottom_ui_min(viewport_pos.x + 20.0f, vp_max.y - 40.0f);
    const ImVec2 bottom_ui_max(viewport_pos.x + 520.0f, vp_max.y - 10.0f);

    const bool over_generate = ImGui::IsMouseHoveringRect(gen_min, gen_max);
    const bool over_bottom_ui = ImGui::IsMouseHoveringRect(bottom_ui_min, bottom_ui_max);
    const bool in_viewport = ImGui::IsMouseHoveringRect(vp_min, vp_max);
    const bool editor_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

    if (axiom_mode && editor_hovered && in_viewport && !over_generate && !over_bottom_ui) {
        RogueCity::Core::Vec2 world_pos = s_primary_viewport->screen_to_world(mouse_pos);

        // Debug: Show mouse world position
        ImGui::SetCursorScreenPos(ImVec2(10, 30));
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 0.7f),
            "Mouse World: (%.1f, %.1f)", world_pos.x, world_pos.y);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            s_axiom_tool->on_mouse_down(world_pos);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            s_axiom_tool->on_mouse_up(world_pos);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            s_axiom_tool->on_mouse_move(world_pos);
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            s_axiom_tool->on_right_click(world_pos);
        }
    }
    
    // Always update axiom tool (for animations)
    s_axiom_tool->update(dt, *s_primary_viewport);

    // Build generation inputs/config for preview (shared by button + live preview)
    const auto& axioms = s_axiom_tool->axioms();
    auto axiom_inputs = s_axiom_tool->get_axiom_inputs();

    RogueCity::Generators::CityGenerator::Config config;
    config.width = 2000;
    config.height = 2000;
    config.cell_size = 10.0;
    config.seed = s_seed;
    config.num_seeds = std::max(10, static_cast<int>(axioms.size() * 6));

    if (!RogueCity::App::GeneratorBridge::validate_axioms(axiom_inputs, config)) {
        s_validation_error = "ERROR: Invalid axioms (bounds/overlap)";
    } else {
        s_validation_error.clear();

        // Debounced live preview: keep updating the request while the user manipulates axioms.
        if (s_preview && s_live_preview && (ui_modified_axiom || s_axiom_tool->is_interacting() || s_axiom_tool->consume_dirty())) {
            s_preview->request_regeneration(axiom_inputs, config);
        }
    }
    
    // Render axioms
    for (const auto& axiom : axioms) {
        axiom->render(draw_list, *s_primary_viewport);
    }
    
    // Status text overlay (cockpit style) - moved down to avoid overlap with debug
    if (axiom_mode && s_preview) {
        DrawAxiomModeStatus(*s_preview, ImVec2(viewport_pos.x + 20, viewport_pos.y + 60));
        ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 20, viewport_pos.y + 80));
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
            "Click-drag to set radius | Edit type via palette");
    } else {
        ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 20, viewport_pos.y + 60));
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
    if (s_preview && s_preview->get_output()) {
        const auto& roads = s_preview->get_output()->roads;
        
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
    
    // Generate City button (Y2K style) + Live preview toggle
    ImGui::SetCursorScreenPos(ImVec2(
        viewport_pos.x + 20,
        viewport_pos.y + viewport_size.y - 80
    ));
    
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 150, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 180, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 120, 200, 255));
    
    const bool can_generate = axioms.size() > 0 && s_preview && !s_preview->is_generating();
    
    if (!can_generate) {
        ImGui::BeginDisabled();
    }
    
    if (ImGui::Button("GENERATE CITY", ImVec2(180, 40))) {
        if (!s_validation_error.empty()) {
            // Keep the current output, just surface the error below.
        } else if (s_preview) {
            s_preview->force_regeneration(axiom_inputs, config);
        }
    }
    
    if (!can_generate) {
        ImGui::EndDisabled();
    }
    
    ImGui::PopStyleColor(3);
    
    // Live preview toggle
    ImGui::SetCursorScreenPos(ImVec2(
        viewport_pos.x + 20,
        viewport_pos.y + viewport_size.y - 34
    ));
    ImGui::Checkbox("Live Preview", &s_live_preview);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::DragFloat("Debounce", &s_debounce_seconds, 0.01f, 0.0f, 1.5f, "%.2fs")) {
        if (s_preview) {
            s_preview->set_debounce_delay(s_debounce_seconds);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Random Seed")) {
        s_seed = static_cast<uint32_t>(ImGui::GetTime() * 1000.0);
        if (s_preview && s_live_preview && s_validation_error.empty()) {
            s_preview->force_regeneration(axiom_inputs, config);
        }
    }

    // Status message display (validation errors)
    ImGui::SetCursorScreenPos(ImVec2(
        viewport_pos.x + 210,
        viewport_pos.y + viewport_size.y - 70
    ));
    
    if (!s_validation_error.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", s_validation_error.c_str());
    }
    
    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Panels::AxiomEditor
