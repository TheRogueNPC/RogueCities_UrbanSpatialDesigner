// FILE: AxiomEditorPanel.cpp - Integrated axiom editor with viewport
// Replaces SystemMap stub with full axiom placement functionality

#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_root.h"  // NEW: Access to shared minimap
#include "RogueCity/App/UI/DesignSystem.h"  // Cockpit Doctrine enforcement
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
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <numbers>
#include <string>

namespace RC_UI::Panels::AxiomEditor {

using namespace RogueCity::UI;  // For DesignSystem helpers

namespace {
    using Preview = RogueCity::App::RealTimePreview;
    
    // === ROGUENAV MINIMAP CONFIGURATION ===
    enum class MinimapMode {
        Disabled,
        Soliton,      // Render-to-texture (recommended, high performance)
        Reactive,     // Dual viewport (heavier, real-time updates)
        Satellite     // Future: Satellite-style view (stub)
    };
    
    enum class RogueNavAlert {
        Normal,       // Blue - All clear
        Caution,      // Yellow - Suspicious
        Evasion,      // Orange - Being tracked
        Alert         // Red - Detected/Combat
    };
    
    static bool s_minimap_visible = true;
    static MinimapMode s_minimap_mode = MinimapMode::Soliton;
    static RogueNavAlert s_nav_alert_level = RogueNavAlert::Normal;
    static constexpr float kMinimapSize = 250.0f;
    static constexpr float kMinimapPadding = 10.0f;
    static constexpr float kMinimapWorldSize = 2000.0f;  // World bounds for minimap (2km)
    static float s_minimap_zoom = 1.0f;  // Zoom level (0.5 = zoomed out, 2.0 = zoomed in)

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

// === ROGUENAV MINIMAP RENDERING ===
static ImU32 GetNavAlertColor() {
    switch (s_nav_alert_level) {
        case RogueNavAlert::Alert:   return DesignTokens::ErrorRed;      // Red - Detected
        case RogueNavAlert::Evasion: return IM_COL32(255, 128, 0, 255);  // Orange - Tracked
        case RogueNavAlert::Caution: return DesignTokens::YellowWarning; // Yellow - Suspicious
        case RogueNavAlert::Normal:
        default:                     return DesignTokens::InfoBlue;      // Blue - All clear
    }
}

// Convert world coordinates to minimap UV space [0,1]
static ImVec2 WorldToMinimapUV(const RogueCity::Core::Vec2& world_pos, const RogueCity::Core::Vec2& camera_pos) {
    // World space relative to camera
    float rel_x = (world_pos.x - camera_pos.x) / (kMinimapWorldSize * s_minimap_zoom);
    float rel_y = (world_pos.y - camera_pos.y) / (kMinimapWorldSize * s_minimap_zoom);
    
    // Center on minimap (0.5, 0.5) + relative offset
    return ImVec2(0.5f + rel_x, 0.5f + rel_y);
}

// Convert minimap pixel coords to world coordinates
static RogueCity::Core::Vec2 MinimapPixelToWorld(const ImVec2& pixel_pos, const ImVec2& minimap_pos, const RogueCity::Core::Vec2& camera_pos) {
    // Normalize to UV [0,1]
    float u = (pixel_pos.x - minimap_pos.x) / kMinimapSize;
    float v = (pixel_pos.y - minimap_pos.y) / kMinimapSize;
    
    // UV to world (centered on camera)
    float world_x = camera_pos.x + (u - 0.5f) * kMinimapWorldSize * s_minimap_zoom;
    float world_y = camera_pos.y + (v - 0.5f) * kMinimapWorldSize * s_minimap_zoom;
    
    return RogueCity::Core::Vec2(world_x, world_y);
}

static void RenderMinimapOverlay(ImDrawList* draw_list, const ImVec2& viewport_pos, const ImVec2& viewport_size) {
    if (!s_minimap_visible) return;
    
    // Position minimap in top-right corner
    const ImVec2 minimap_pos = ImVec2(
        viewport_pos.x + viewport_size.x - kMinimapSize - kMinimapPadding,
        viewport_pos.y + kMinimapPadding
    );
    
    // RogueNav border (alert-colored warning stripe)
    const ImU32 alert_color = GetNavAlertColor();
    draw_list->AddRect(
        minimap_pos,
        ImVec2(minimap_pos.x + kMinimapSize, minimap_pos.y + kMinimapSize),
        alert_color,
        0.0f,  // No rounding (Y2K hard edges)
        0,
        3.0f   // 3px warning stripe border
    );
    
    // Semi-transparent background
    draw_list->AddRectFilled(
        ImVec2(minimap_pos.x + 3, minimap_pos.y + 3),
        ImVec2(minimap_pos.x + kMinimapSize - 3, minimap_pos.y + kMinimapSize - 3),
        IM_COL32(10, 15, 25, 200)  // Dark semi-transparent
    );
    
    // RogueNav label (top-left corner, inside border)
    ImGui::SetCursorScreenPos(ImVec2(minimap_pos.x + 8, minimap_pos.y + 8));
    ImGui::PushStyleColor(ImGuiCol_Text, alert_color);
    ImGui::Text("ROGUENAV");
    ImGui::PopStyleColor();
    
    // Mode indicator (top-right corner, inside border)
    const char* mode_text = "";
    switch (s_minimap_mode) {
        case MinimapMode::Soliton:    mode_text = "SOLITON"; break;
        case MinimapMode::Reactive:   mode_text = "REACTIVE"; break;
        case MinimapMode::Satellite:  mode_text = "SATELLITE"; break;
        default: mode_text = "OFF"; break;
    }
    ImVec2 mode_text_size = ImGui::CalcTextSize(mode_text);
    ImGui::SetCursorScreenPos(ImVec2(
        minimap_pos.x + kMinimapSize - mode_text_size.x - 8,
        minimap_pos.y + 8
    ));
    ImGui::PushStyleColor(ImGuiCol_Text, DesignTokens::TextSecondary);
    ImGui::Text("%s", mode_text);
    ImGui::PopStyleColor();
    
    // Alert level indicator (bottom-right corner)
    const char* alert_text = "";
    switch (s_nav_alert_level) {
        case RogueNavAlert::Alert:   alert_text = "ALERT"; break;
        case RogueNavAlert::Evasion: alert_text = "EVASION"; break;
        case RogueNavAlert::Caution: alert_text = "CAUTION"; break;
        case RogueNavAlert::Normal:  alert_text = "NORMAL"; break;
    }
    ImVec2 alert_text_size = ImGui::CalcTextSize(alert_text);
    ImGui::SetCursorScreenPos(ImVec2(
        minimap_pos.x + kMinimapSize - alert_text_size.x - 8,
        minimap_pos.y + kMinimapSize - 20
    ));
    ImGui::PushStyleColor(ImGuiCol_Text, alert_color);
    ImGui::Text("%s", alert_text);
    ImGui::PopStyleColor();
    
    // TODO: Render actual minimap content (orthographic top-down view)
    // Get camera position for world-to-minimap conversion
    const auto camera_pos = s_primary_viewport ? s_primary_viewport->get_camera_xy() : RogueCity::Core::Vec2(1000.0f, 1000.0f);
    
    // === PHASE 2: RENDER AXIOMS AS COLORED DOTS ===
    if (s_axiom_tool) {
        const auto& axioms = s_axiom_tool->axioms();
        for (const auto& axiom : axioms) {
            ImVec2 uv = WorldToMinimapUV(axiom->position(), camera_pos);
            
            // Only draw if within minimap bounds
            if (uv.x >= 0.0f && uv.x <= 1.0f && uv.y >= 0.0f && uv.y <= 1.0f) {
                ImVec2 screen_pos = ImVec2(
                    minimap_pos.x + uv.x * kMinimapSize,
                    minimap_pos.y + uv.y * kMinimapSize
                );
                
                // Draw axiom as colored circle
                ImU32 axiom_color = DesignTokens::MagentaHighlight;
                draw_list->AddCircleFilled(screen_pos, 3.0f, axiom_color);
                draw_list->AddCircle(screen_pos, 5.0f, axiom_color, 8, 1.0f);
            }
        }
    }
    
    // === PHASE 2: RENDER ROADS AS SIMPLIFIED LINES ===
    if (s_preview && s_preview->get_output()) {
        const auto& roads = s_preview->get_output()->roads;
        for (auto it = roads.begin(); it != roads.end(); ++it) {
            const auto& road = *it;
            if (road.points.empty()) continue;
            
            // Draw road as line segments (every 5th point for performance)
            for (size_t i = 0; i < road.points.size() - 1; i += 5) {
                ImVec2 uv1 = WorldToMinimapUV(road.points[i], camera_pos);
                ImVec2 uv2 = WorldToMinimapUV(road.points[i + 1], camera_pos);
                
                // Clip to minimap bounds
                if ((uv1.x >= 0.0f && uv1.x <= 1.0f && uv1.y >= 0.0f && uv1.y <= 1.0f) ||
                    (uv2.x >= 0.0f && uv2.x <= 1.0f && uv2.y >= 0.0f && uv2.y <= 1.0f)) {
                    
                    ImVec2 screen1 = ImVec2(
                        minimap_pos.x + uv1.x * kMinimapSize,
                        minimap_pos.y + uv1.y * kMinimapSize
                    );
                    ImVec2 screen2 = ImVec2(
                        minimap_pos.x + uv2.x * kMinimapSize,
                        minimap_pos.y + uv2.y * kMinimapSize
                    );
                    
                    draw_list->AddLine(screen1, screen2, DesignTokens::CyanAccent, 1.0f);
                }
            }
        }
    }
    
    // === PHASE 1: DRAW FRUSTUM RECTANGLE (MAIN CAMERA VIEW) ===
    // Draw a rectangle showing the main viewport's visible area
    // For now, draw a simple rectangle around the center (placeholder)
    const float frustum_size = 50.0f;  // Placeholder size in pixels
    const ImVec2 frustum_center = ImVec2(
        minimap_pos.x + kMinimapSize / 2.0f,
        minimap_pos.y + kMinimapSize / 2.0f
    );
    draw_list->AddRect(
        ImVec2(frustum_center.x - frustum_size, frustum_center.y - frustum_size),
        ImVec2(frustum_center.x + frustum_size, frustum_center.y + frustum_size),
        DesignTokens::YellowWarning,
        0.0f, 0, 2.0f
    );
    
    // Center crosshair (current camera position)
    const ImVec2 center = ImVec2(
        minimap_pos.x + kMinimapSize / 2.0f,
        minimap_pos.y + kMinimapSize / 2.0f
    );
    draw_list->AddCircleFilled(center, 4.0f, alert_color);
    draw_list->AddCircle(center, 8.0f, alert_color, 16, 1.5f);
}

void Draw(float dt) {
Initialize();
    
// Use DesignSystem panel helper (enforces Cockpit Doctrine styling)
DesignSystem::BeginPanel("RogueVisualizer", 
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
    
    // === ROGUENAV MINIMAP OVERLAY ===
    // Render minimap as overlay in top-right corner
    RenderMinimapOverlay(draw_list, viewport_pos, viewport_size);
    
    // === PHASE 1: MINIMAP INTERACTION ===
    const ImVec2 minimap_pos_bounds = ImVec2(
        viewport_pos.x + viewport_size.x - kMinimapSize - kMinimapPadding,
        viewport_pos.y + kMinimapPadding
    );
    const ImVec2 minimap_max_bounds = ImVec2(minimap_pos_bounds.x + kMinimapSize, minimap_pos_bounds.y + kMinimapSize);
    
    // Check if mouse is over minimap
    const ImVec2 mouse_screen = ImGui::GetMousePos();
    const bool minimap_hovered = (mouse_screen.x >= minimap_pos_bounds.x && mouse_screen.x <= minimap_max_bounds.x &&
                                   mouse_screen.y >= minimap_pos_bounds.y && mouse_screen.y <= minimap_max_bounds.y);
    
    if (minimap_hovered && s_minimap_visible) {
        // Phase 1: Scroll to zoom
        float scroll = ImGui::GetIO().MouseWheel;
        if (scroll != 0.0f) {
            s_minimap_zoom *= (1.0f + scroll * 0.1f);
            s_minimap_zoom = std::clamp(s_minimap_zoom, 0.5f, 3.0f);  // Min 0.5x, max 3x
        }
        
        // Phase 1: Click to teleport camera
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const auto camera_pos = s_primary_viewport->get_camera_xy();
            const auto camera_z = s_primary_viewport->get_camera_z();
            const auto world_pos = MinimapPixelToWorld(mouse_screen, minimap_pos_bounds, camera_pos);
            
            // Set camera to clicked position
            s_primary_viewport->set_camera_position(world_pos, camera_z);
        }
    }
    
    // Minimap toggle hotkey (M key)
    if (ImGui::IsKeyPressed(ImGuiKey_M) && !ImGui::GetIO().WantTextInput) {
        s_minimap_visible = !s_minimap_visible;
    }
    
    // Validation errors are shown in the Tools strip (and can still be overlayed here if desired).
    
    DesignSystem::EndPanel();  // Use DesignSystem helper (matches BeginPanel)
}

} // namespace RC_UI::Panels::AxiomEditor
