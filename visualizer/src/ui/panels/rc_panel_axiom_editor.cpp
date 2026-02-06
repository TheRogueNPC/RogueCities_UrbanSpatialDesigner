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
static bool s_external_dirty = false;

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

RogueCity::App::AxiomPlacementTool* GetAxiomTool() { return s_axiom_tool.get(); }
RogueCity::App::RealTimePreview* GetPreview() { return s_preview.get(); }
RogueCity::App::AxiomVisual* GetSelectedAxiom() { return s_axiom_tool ? s_axiom_tool->get_selected_axiom() : nullptr; }

bool IsLivePreviewEnabled() { return s_live_preview; }
void SetLivePreviewEnabled(bool enabled) { s_live_preview = enabled; }

float GetDebounceSeconds() { return s_debounce_seconds; }
void SetDebounceSeconds(float seconds) {
    s_debounce_seconds = std::max(0.0f, seconds);
    if (s_preview) {
        s_preview->set_debounce_delay(s_debounce_seconds);
    }
}

uint32_t GetSeed() { return s_seed; }
void SetSeed(uint32_t seed) { s_seed = seed; }

void RandomizeSeed() {
    s_seed = static_cast<uint32_t>(ImGui::GetTime() * 1000.0);
    s_external_dirty = true;
}

void MarkAxiomChanged() {
    s_external_dirty = true;
}

static bool BuildInputs(
    std::vector<RogueCity::Generators::CityGenerator::AxiomInput>& out_inputs,
    RogueCity::Generators::CityGenerator::Config& out_config) {
    if (!s_axiom_tool) return false;

    out_inputs = s_axiom_tool->get_axiom_inputs();

    const auto& axioms = s_axiom_tool->axioms();
    out_config.width = 2000;
    out_config.height = 2000;
    out_config.cell_size = 10.0;
    out_config.seed = s_seed;
    out_config.num_seeds = std::max(10, static_cast<int>(axioms.size() * 6));

    if (!RogueCity::App::GeneratorBridge::validate_axioms(out_inputs, out_config)) {
        s_validation_error = "ERROR: Invalid axioms (bounds/overlap)";
        return false;
    }

    s_validation_error.clear();
    return true;
}

bool CanGenerate() {
    if (!s_preview || !s_axiom_tool) return false;
    if (s_preview->is_generating()) return false;
    return !s_axiom_tool->axioms().empty();
}

void ForceGenerate() {
    if (!s_preview) return;

    std::vector<RogueCity::Generators::CityGenerator::AxiomInput> inputs;
    RogueCity::Generators::CityGenerator::Config cfg;
    if (!BuildInputs(inputs, cfg)) return;

    s_preview->force_regeneration(inputs, cfg);
}

const char* GetValidationError() {
    return s_validation_error.c_str();
}

void Draw(float dt) {
Initialize();
    
ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorBG);
ImGui::Begin("RogueVisualizer", nullptr, 
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

    bool ui_modified_axiom = false;

    // Axiom Library (palette) - toggled by "Axiom Deck"
    if (RC_UI::IsAxiomLibraryOpen()) {
        ImGui::Begin("Axiom Library", nullptr, ImGuiWindowFlags_NoCollapse);

        ImGui::TextColored(ImVec4(1, 1, 1, 0.85f), "Axiom Library (Ctrl: apply to selection)");

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
                ImGui::SetTooltip("%s", info.name);
            }
            ImGui::PopID();
        }

        ImGui::End();
    }

    // ---------------------------------------------------------------------
    // Viewport mouse interaction (placement + manipulation)
    // ---------------------------------------------------------------------
    const ImVec2 mouse_pos = ImGui::GetMousePos();
    const ImVec2 vp_min = viewport_pos;
    const ImVec2 vp_max(viewport_pos.x + viewport_size.x, viewport_pos.y + viewport_size.y);
    const bool in_viewport = ImGui::IsMouseHoveringRect(vp_min, vp_max);
    const bool editor_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

    if (axiom_mode && editor_hovered && in_viewport && !ImGui::GetIO().WantCaptureMouse) {
        ImGuiIO& io = ImGui::GetIO();
        RogueCity::Core::Vec2 world_pos = s_primary_viewport->screen_to_world(mouse_pos);

        // CAD-grade viewport controls (2D/2.5D)
        const bool orbit = io.KeyAlt && io.MouseDown[ImGuiMouseButton_Left];
        const bool pan = io.MouseDown[ImGuiMouseButton_Middle] || (io.KeyShift && io.MouseDown[ImGuiMouseButton_Right]);
        const bool zoom_drag = io.KeyCtrl && io.MouseDown[ImGuiMouseButton_Right];
        const bool nav_active = orbit || pan || zoom_drag;

        // Mouse wheel zoom (only when not captured by UI)
        if (io.MouseWheel != 0.0f) {
            const float z = s_primary_viewport->get_camera_z();
            const float factor = std::pow(1.12f, -io.MouseWheel);
            const float new_z = std::clamp(z * factor, 80.0f, 4000.0f);
            s_primary_viewport->set_camera_position(s_primary_viewport->get_camera_xy(), new_z);
        }

        if (orbit) {
            const float yaw = s_primary_viewport->get_camera_yaw();
            s_primary_viewport->set_camera_yaw(yaw + io.MouseDelta.x * 0.0075f);
        }

        if (pan) {
            const float z = s_primary_viewport->get_camera_z();
            const float zoom = 500.0f / std::max(100.0f, z);
            const float yaw = s_primary_viewport->get_camera_yaw();

            const float dx = io.MouseDelta.x / zoom;
            const float dy = io.MouseDelta.y / zoom;

            const float c = std::cos(yaw);
            const float s = std::sin(yaw);
            RogueCity::Core::Vec2 delta_world(dx * c - dy * s, dx * s + dy * c);

            auto cam = s_primary_viewport->get_camera_xy();
            cam.x -= delta_world.x;
            cam.y -= delta_world.y;
            s_primary_viewport->set_camera_position(cam, z);
        }

        if (zoom_drag) {
            const float z = s_primary_viewport->get_camera_z();
            const float factor = std::exp(io.MouseDelta.y * 0.01f);
            const float new_z = std::clamp(z * factor, 80.0f, 4000.0f);
            s_primary_viewport->set_camera_position(s_primary_viewport->get_camera_xy(), new_z);
        }

        // Debug: Show mouse world position
        ImGui::SetCursorScreenPos(ImVec2(10, 30));
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 0.7f),
            "Mouse World: (%.1f, %.1f)%s", world_pos.x, world_pos.y, nav_active ? " [NAV]" : "");

        // Tool interaction (disabled while navigating)
        if (!nav_active) {
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
    }
    
    // Always update axiom tool (for animations)
    s_axiom_tool->update(dt, *s_primary_viewport);

    // Build generation inputs/config for preview
    std::vector<RogueCity::Generators::CityGenerator::AxiomInput> axiom_inputs;
    RogueCity::Generators::CityGenerator::Config config;
    const bool inputs_ok = BuildInputs(axiom_inputs, config);
    if (inputs_ok) {
        // Debounced live preview: keep updating the request while the user manipulates axioms.
        if (s_preview && s_live_preview &&
            (ui_modified_axiom || s_external_dirty || s_axiom_tool->is_interacting() || s_axiom_tool->consume_dirty())) {
            s_preview->request_regeneration(axiom_inputs, config);
            s_external_dirty = false;
        }
    }
    
    // Render axioms
    const auto& axioms = s_axiom_tool->axioms();
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
    
    // Validation errors are shown in the Tools strip (and can still be overlayed here if desired).
    
    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Panels::AxiomEditor
