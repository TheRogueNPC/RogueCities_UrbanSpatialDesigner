// FILE: AxiomEditorPanel.cpp - Integrated axiom editor with viewport
// Full axiom placement workflow and minimap controls

#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_root.h"  // NEW: Access to shared minimap
#include "ui/rc_ui_input_gate.h"
#include "ui/rc_ui_tokens.h"
#include "ui/commands/rc_context_command_registry.h"
#include "ui/commands/rc_context_menu_smart.h"
#include "ui/commands/rc_context_menu_pie.h"
#include "ui/commands/rc_command_palette.h"
#include "ui/tools/rc_tool_contract.h"
#include "ui/tools/rc_tool_dispatcher.h"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/App/Viewports/MinimapViewport.hpp"
#include "RogueCity/App/Viewports/ViewportSyncManager.hpp"
#include "RogueCity/App/Tools/AxiomPlacementTool.hpp"
#include "RogueCity/App/Integration/CityOutputApplier.hpp"
#include "RogueCity/App/Integration/GenerationCoordinator.hpp"
#include "RogueCity/App/Integration/GeneratorBridge.hpp"
#include "RogueCity/App/Integration/RealTimePreview.hpp"
#include "RogueCity/App/Editor/EditorManipulation.hpp"
#include "RogueCity/App/Tools/AxiomIcon.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Validation/EditorOverlayValidation.hpp"
#include "ui/viewport/rc_scene_frame.h"
#include "ui/viewport/rc_minimap_renderer.h"
#include "ui/viewport/rc_minimap_interaction_math.h"
#include "ui/viewport/rc_viewport_scene_controller.h"
#include "ui/viewport/rc_viewport_interaction.h"
#include "ui/viewport/rc_viewport_overlays.h"
#include "ui/introspection/UiIntrospection.h"
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <deque>
#include <numeric>
#include <sstream>
#include <unordered_map>
#include <string>

namespace RC_UI::Panels::AxiomEditor {

namespace {
    [[nodiscard]] ImU32 TokenColor(ImU32 base, uint8_t alpha = 255u) {
        return WithAlpha(base, alpha);
    }

    [[nodiscard]] ImVec4 TokenColorF(ImU32 base, uint8_t alpha = 255u) {
        return ImGui::ColorConvertU32ToFloat4(TokenColor(base, alpha));
    }

    using Preview = RogueCity::App::RealTimePreview;
    // === ROGUENAV MINIMAP CONFIGURATION ===
    enum class MinimapMode {
        Disabled,
        Soliton,      // Render-to-texture (recommended, high performance)
        Reactive,     // Dual viewport (heavier, real-time updates)
        Satellite     // Future extension point: satellite-style view
    };

    enum class MinimapLOD : uint8_t {
        Strategic = 0,
        Tactical,
        Detail
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
    static bool s_minimap_auto_lod = true;
    static MinimapLOD s_minimap_lod = MinimapLOD::Tactical;
    static MinimapLOD s_minimap_effective_lod = MinimapLOD::Tactical;
    static bool s_minimap_adaptive_quality = true;
    static std::deque<float> s_minimap_fps_history{};
    static int s_minimap_low_fps_streak = 0;
    static int s_minimap_high_fps_streak = 0;
    static int s_minimap_fps_degrade_steps = 0;
    static std::string s_minimap_lod_status_text = "LOD: Auto (1)";

    [[nodiscard]] bool TryAxiomTypeFromActionId(RC_UI::Tools::ToolActionId action_id,
                                                RogueCity::App::AxiomType& out_type) {
        using RC_UI::Tools::ToolActionId;
        switch (action_id) {
            case ToolActionId::Axiom_Organic: out_type = RogueCity::App::AxiomType::Organic; return true;
            case ToolActionId::Axiom_Grid: out_type = RogueCity::App::AxiomType::Grid; return true;
            case ToolActionId::Axiom_Radial: out_type = RogueCity::App::AxiomType::Radial; return true;
            case ToolActionId::Axiom_Hexagonal: out_type = RogueCity::App::AxiomType::Hexagonal; return true;
            case ToolActionId::Axiom_Stem: out_type = RogueCity::App::AxiomType::Stem; return true;
            case ToolActionId::Axiom_LooseGrid: out_type = RogueCity::App::AxiomType::LooseGrid; return true;
            case ToolActionId::Axiom_Suburban: out_type = RogueCity::App::AxiomType::Suburban; return true;
            case ToolActionId::Axiom_Superblock: out_type = RogueCity::App::AxiomType::Superblock; return true;
            case ToolActionId::Axiom_Linear: out_type = RogueCity::App::AxiomType::Linear; return true;
            case ToolActionId::Axiom_GridCorrective: out_type = RogueCity::App::AxiomType::GridCorrective; return true;
            default: break;
        }
        return false;
    }

    void DrawAxiomModeStatus(const Preview& preview, ImVec2 pos) {
        ImGui::SetCursorScreenPos(pos);

        const auto phase = preview.phase();
        const float t = preview.phase_elapsed_seconds();

        if (phase == Preview::GenerationPhase::InitStreetSweeper) {
            const char* text = "_INIT_STREETSWEEPER";
            const int len = static_cast<int>(std::strlen(text));
            const float reveal = std::clamp(t / 0.25f, 0.0f, 1.0f);
            const int visible = std::clamp(static_cast<int>(reveal * static_cast<float>(len)), 0, len);

            ImGui::TextColored(TokenColorF(UITokens::ErrorRed), "%.*s", visible, text);
            return;
        }

        if (phase == Preview::GenerationPhase::Sweeping) {
            const float p = preview.get_progress();
            ImGui::TextColored(TokenColorF(UITokens::AmberGlow), "_SWEEPING  %.0f%%", p * 100.0f);
            return;
        }

        if (phase == Preview::GenerationPhase::Cancelled) {
            const float fade = std::clamp(t / 0.8f, 0.0f, 1.0f);
            const ImVec4 color(1.0f, 0.35f + (fade * 0.55f), 0.0f, 1.0f - fade * 0.5f);
            ImGui::TextColored(color, "_SWEEP_CANCELLED");
            return;
        }

        if (phase == Preview::GenerationPhase::StreetsSwept) {
            const float fade = std::clamp(t / 1.25f, 0.0f, 1.0f);
            const ImVec4 color(1.0f - fade, fade, 0.0f, 1.0f - fade * 0.5f);
            ImGui::TextColored(color, "_STREETS_SWEPT");
            return;
        }

        ImGui::TextColored(TokenColorF(UITokens::TextPrimary, 242u), "AXIOM MODE ACTIVE");
    }

}

// === Clear Layer Command (Undoable) ===
struct ClearLayerCommand : public RogueCity::App::ICommand {
    enum LayerMask : uint32_t {
        None = 0,
        Axioms = 1 << 0,
        Water = 1 << 1,
        Roads = 1 << 2,
        Districts = 1 << 3,
        Lots = 1 << 4,
        Buildings = 1 << 5,
        Blocks = 1 << 6,
        All = Axioms | Water | Roads | Districts | Lots | Buildings | Blocks
    };

    uint32_t layers{ None };
    std::string description{};

    // Snapshots (stored before clear)
    std::vector<RogueCity::App::AxiomPlacementTool::AxiomSnapshot> axioms_snapshot{};
    std::vector<RogueCity::Core::WaterBody> water_snapshot{};
    std::vector<RogueCity::Core::Road> roads_snapshot{};
    std::vector<RogueCity::Core::District> districts_snapshot{};
    std::vector<RogueCity::Core::LotToken> lots_snapshot{};
    std::vector<RogueCity::Core::BuildingSite> buildings_snapshot{};
    std::vector<RogueCity::Core::BlockPolygon> blocks_snapshot{};

    void Execute() override {
        auto& gs = RogueCity::Core::Editor::GetGlobalState();

        // Capture snapshots before clearing
        if (layers & Axioms) {
            axioms_snapshot.clear();
            if (auto* axiom_tool = RC_UI::Panels::AxiomEditor::GetAxiomTool()) {
                const auto& tool_axioms = axiom_tool->axioms();
                axioms_snapshot.reserve(tool_axioms.size());
                for (const auto& axiom : tool_axioms) {
                    if (!axiom) {
                        continue;
                    }

                    RogueCity::App::AxiomPlacementTool::AxiomSnapshot snapshot{};
                    snapshot.id = axiom->id();
                    snapshot.type = axiom->type();
                    snapshot.position = axiom->position();
                    snapshot.radius = axiom->radius();
                    snapshot.rotation = axiom->rotation();
                    snapshot.organic_curviness = axiom->organic_curviness();
                    snapshot.radial_spokes = axiom->radial_spokes();
                    snapshot.loose_grid_jitter = axiom->loose_grid_jitter();
                    snapshot.suburban_loop_strength = axiom->suburban_loop_strength();
                    snapshot.stem_branch_angle = axiom->stem_branch_angle();
                    snapshot.superblock_block_size = axiom->superblock_block_size();
                    axioms_snapshot.push_back(snapshot);
                }
            }
        }
        if (layers & Water) {
            water_snapshot.clear();
            for (const auto& wb : gs.waterbodies) {
                water_snapshot.push_back(wb);
            }
        }
        if (layers & Roads) {
            roads_snapshot.clear();
            for (const auto& rd : gs.roads) {
                roads_snapshot.push_back(rd);
            }
        }
        if (layers & Districts) {
            districts_snapshot.clear();
            for (const auto& dt : gs.districts) {
                districts_snapshot.push_back(dt);
            }
        }
        if (layers & Lots) {
            lots_snapshot.clear();
            for (const auto& lt : gs.lots) {
                lots_snapshot.push_back(lt);
            }
        }
        if (layers & Buildings) {
            buildings_snapshot.clear();
            for (const auto& bd : gs.buildings) {
                buildings_snapshot.push_back(bd);
            }
        }
        if (layers & Blocks) {
            blocks_snapshot.clear();
            for (const auto& bl : gs.blocks) {
                blocks_snapshot.push_back(bl);
            }
        }

        // Perform the clear
        if (layers & Axioms) {
            if (auto* axiom_tool = RC_UI::Panels::AxiomEditor::GetAxiomTool()) {
                axiom_tool->clear_axioms();
            }
            gs.axioms.clear();
        }
        if (layers & Water) gs.waterbodies.clear();
        if (layers & Roads) gs.roads.clear();
        if (layers & Districts) gs.districts.clear();
        if (layers & Lots) gs.lots.clear();
        if (layers & Buildings) gs.buildings.clear();
        if (layers & Blocks) gs.blocks.clear();

        // Mark affected layers as dirty
        if (layers & Axioms) gs.dirty_layers.MarkFromAxiomEdit();
        if (layers & Water) gs.dirty_layers.MarkDirty(RogueCity::Core::Editor::DirtyLayer::Roads);
        if (layers & (Roads | Districts | Lots | Buildings | Blocks)) {
            gs.dirty_layers.MarkDirty(RogueCity::Core::Editor::DirtyLayer::Roads);
            gs.dirty_layers.MarkDirty(RogueCity::Core::Editor::DirtyLayer::Districts);
            gs.dirty_layers.MarkDirty(RogueCity::Core::Editor::DirtyLayer::Lots);
            gs.dirty_layers.MarkDirty(RogueCity::Core::Editor::DirtyLayer::Buildings);
        }
    }

    void Undo() override {
        auto& gs = RogueCity::Core::Editor::GetGlobalState();

        // Restore from snapshots
        if (layers & Axioms) {
            if (auto* axiom_tool = RC_UI::Panels::AxiomEditor::GetAxiomTool()) {
                axiom_tool->clear_axioms();
                for (const auto& snapshot : axioms_snapshot) {
                    axiom_tool->add_axiom_from_snapshot(snapshot);
                }
            }
            gs.axioms.clear();
        }
        if (layers & Water) {
            gs.waterbodies.clear();
            for (const auto& wb : water_snapshot) {
                gs.waterbodies.add(wb);
            }
        }
        if (layers & Roads) {
            gs.roads.clear();
            for (const auto& rd : roads_snapshot) {
                gs.roads.add(rd);
            }
        }
        if (layers & Districts) {
            gs.districts.clear();
            for (const auto& dt : districts_snapshot) {
                gs.districts.add(dt);
            }
        }
        if (layers & Lots) {
            gs.lots.clear();
            for (const auto& lt : lots_snapshot) {
                gs.lots.add(lt);
            }
        }
        if (layers & Buildings) {
            gs.buildings.clear();
            for (const auto& bd : buildings_snapshot) {
                    gs.buildings.push_back(bd);
            }
        }
        if (layers & Blocks) {
            gs.blocks.clear();
            for (const auto& bl : blocks_snapshot) {
                gs.blocks.add(bl);
            }
        }

        // Mark affected layers as dirty
        if (layers & Axioms) gs.dirty_layers.MarkFromAxiomEdit();
        if (layers & Water) gs.dirty_layers.MarkDirty(RogueCity::Core::Editor::DirtyLayer::Roads);
        if (layers & (Roads | Districts | Lots | Buildings | Blocks)) {
            gs.dirty_layers.MarkDirty(RogueCity::Core::Editor::DirtyLayer::Roads);
            gs.dirty_layers.MarkDirty(RogueCity::Core::Editor::DirtyLayer::Districts);
            gs.dirty_layers.MarkDirty(RogueCity::Core::Editor::DirtyLayer::Lots);
            gs.dirty_layers.MarkDirty(RogueCity::Core::Editor::DirtyLayer::Buildings);
        }
    }

    const char* GetDescription() const override {
        return description.c_str();
    }
};

// Singleton instances (owned by this panel)
static std::unique_ptr<RogueCity::App::PrimaryViewport> s_primary_viewport;
static std::unique_ptr<RogueCity::App::ViewportSyncManager> s_sync_manager;
static std::unique_ptr<RogueCity::App::AxiomPlacementTool> s_axiom_tool;
static std::unique_ptr<RogueCity::App::GenerationCoordinator> s_generation_coordinator;
static bool s_initialized = false;
static bool s_live_preview = true;
static float s_debounce_seconds = 0.30f;
static float s_flow_rate = 1.0f;
static uint32_t s_seed = 42u;
static std::string s_validation_error;
static bool s_external_dirty = false;
static bool s_library_modified = false;
static RC_UI::Viewport::SceneFrame s_scene_frame{};
static RC_UI::Commands::SmartMenuState s_smart_command_menu{};
static RC_UI::Commands::PieMenuState s_pie_command_menu{};
static RC_UI::Commands::CommandPaletteState s_command_palette{};

static RC_UI::Viewport::NonAxiomInteractionState s_non_axiom_interaction{};

RogueCity::Core::Editor::EditorAxiom::Type ToEditorAxiomType(RogueCity::App::AxiomVisual::AxiomType type) {
    using RogueCity::App::AxiomVisual;
    using RogueCity::Core::Editor::EditorAxiom;

    switch (type) {
    case AxiomVisual::AxiomType::Organic: return EditorAxiom::Type::Organic;
    case AxiomVisual::AxiomType::Grid: return EditorAxiom::Type::Grid;
    case AxiomVisual::AxiomType::Radial: return EditorAxiom::Type::Radial;
    case AxiomVisual::AxiomType::Hexagonal: return EditorAxiom::Type::Hexagonal;
    case AxiomVisual::AxiomType::Stem: return EditorAxiom::Type::Stem;
    case AxiomVisual::AxiomType::LooseGrid: return EditorAxiom::Type::LooseGrid;
    case AxiomVisual::AxiomType::Suburban: return EditorAxiom::Type::Suburban;
    case AxiomVisual::AxiomType::Superblock: return EditorAxiom::Type::Superblock;
    case AxiomVisual::AxiomType::Linear: return EditorAxiom::Type::Linear;
    case AxiomVisual::AxiomType::GridCorrective: return EditorAxiom::Type::GridCorrective;
    case AxiomVisual::AxiomType::COUNT: break;
    }

    return EditorAxiom::Type::Grid;
}

void SyncToolAxiomsToGlobalState() {
    if (!s_axiom_tool) {
        return;
    }

    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    gs.axioms.clear();

    const auto& tool_axioms = s_axiom_tool->axioms();
    for (const auto& axiom : tool_axioms) {
        if (!axiom) {
            continue;
        }

        RogueCity::Core::Editor::EditorAxiom editor_axiom{};
        editor_axiom.id = static_cast<uint32_t>(std::max(0, axiom->id()));
        editor_axiom.type = ToEditorAxiomType(axiom->type());
        editor_axiom.position = axiom->position();
        editor_axiom.radius = static_cast<double>(axiom->radius());
        editor_axiom.theta = static_cast<double>(axiom->rotation());
        editor_axiom.decay = 2.0;
        editor_axiom.is_user_placed = true;
        gs.axioms.add(editor_axiom);
    }
}

RogueCity::Core::Vec2 PolygonCentroid(const std::vector<RogueCity::Core::Vec2>& points) {
    RogueCity::Core::Vec2 centroid{};
    if (points.empty()) {
        return centroid;
    }
    for (const auto& p : points) {
        centroid += p;
    }
    centroid /= static_cast<double>(points.size());
    return centroid;
}

bool ResolveSelectionAnchor(
    const RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::VpEntityKind kind,
    uint32_t id,
    RogueCity::Core::Vec2& out_anchor) {
    using RogueCity::Core::Editor::VpEntityKind;

    switch (kind) {
    case VpEntityKind::Road:
        for (const auto& road : gs.roads) {
            if (road.id == id && !road.points.empty()) {
                out_anchor = road.points[road.points.size() / 2];
                return true;
            }
        }
        return false;
    case VpEntityKind::District:
        for (const auto& district : gs.districts) {
            if (district.id == id) {
                out_anchor = PolygonCentroid(district.border);
                return !district.border.empty();
            }
        }
        return false;
    case VpEntityKind::Lot:
        for (const auto& lot : gs.lots) {
            if (lot.id == id) {
                out_anchor = lot.boundary.empty() ? lot.centroid : PolygonCentroid(lot.boundary);
                return true;
            }
        }
        return false;
    case VpEntityKind::Building:
        for (const auto& building : gs.buildings) {
            if (building.id == id) {
                out_anchor = building.position;
                return true;
            }
        }
        return false;
    default:
        return false;
    }
}

RogueCity::Core::Road* FindRoadMutable(RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (auto& road : gs.roads) {
        if (road.id == id) {
            return &road;
        }
    }
    return nullptr;
}

RogueCity::Core::District* FindDistrictMutable(RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (auto& district : gs.districts) {
        if (district.id == id) {
            return &district;
        }
    }
    return nullptr;
}

RogueCity::Core::WaterBody* FindWaterMutable(RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (auto& water : gs.waterbodies) {
        if (water.id == id) {
            return &water;
        }
    }
    return nullptr;
}

void Initialize() {
    if (s_initialized) return;
    
    s_primary_viewport = std::make_unique<RogueCity::App::PrimaryViewport>();
    s_axiom_tool = std::make_unique<RogueCity::App::AxiomPlacementTool>();
    s_generation_coordinator = std::make_unique<RogueCity::App::GenerationCoordinator>();
    s_generation_coordinator->SetDebounceDelay(s_debounce_seconds);
    
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

    s_generation_coordinator->SetOnComplete([minimap](const RogueCity::Generators::CityGenerator::CityOutput& output) {
        auto& gs = RogueCity::Core::Editor::GetGlobalState();
        RogueCity::App::ApplyCityOutputToGlobalState(output, gs);
        gs.tool_runtime.explicit_generation_pending = false;
        gs.tool_runtime.last_viewport_status = "generation-applied";
        gs.tool_runtime.last_viewport_status_frame = gs.frame_counter;
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
    s_generation_coordinator.reset();
    s_axiom_tool.reset();
    s_sync_manager.reset();
    s_primary_viewport.reset();
    s_initialized = false;
}

RogueCity::App::AxiomPlacementTool* GetAxiomTool() { return s_axiom_tool.get(); }
RogueCity::App::RealTimePreview* GetPreview() {
    return s_generation_coordinator ? s_generation_coordinator->Preview() : nullptr;
}
RogueCity::App::AxiomVisual* GetSelectedAxiom() { return s_axiom_tool ? s_axiom_tool->get_selected_axiom() : nullptr; }

bool IsLivePreviewEnabled() { return s_live_preview; }
void SetLivePreviewEnabled(bool enabled) { s_live_preview = enabled; }

float GetDebounceSeconds() { return s_debounce_seconds; }
void SetDebounceSeconds(float seconds) {
    s_debounce_seconds = std::max(0.0f, seconds);
    if (s_generation_coordinator) {
        s_generation_coordinator->SetDebounceDelay(s_debounce_seconds);
    }
}

bool CanUndo() {
    return s_axiom_tool && s_axiom_tool->can_undo();
}

bool CanRedo() {
    return s_axiom_tool && s_axiom_tool->can_redo();
}

void Undo() {
    if (s_axiom_tool) {
        s_axiom_tool->undo();
        RogueCity::Core::Editor::GetGlobalState().dirty_layers.MarkFromAxiomEdit();
    }
}

void Redo() {
    if (s_axiom_tool) {
        s_axiom_tool->redo();
        RogueCity::Core::Editor::GetGlobalState().dirty_layers.MarkFromAxiomEdit();
    }
}

const char* GetUndoLabel() {
    return s_axiom_tool ? s_axiom_tool->undo_label() : "Undo";
}

const char* GetRedoLabel() {
    return s_axiom_tool ? s_axiom_tool->redo_label() : "Redo";
}

uint32_t GetSeed() { return s_seed; }
void SetSeed(uint32_t seed) { s_seed = seed; }
float GetFlowRate() { return s_flow_rate; }
void SetFlowRate(float flowRate) { s_flow_rate = std::clamp(flowRate, 0.1f, 4.0f); }

void RandomizeSeed() {
    s_seed = static_cast<uint32_t>(ImGui::GetTime() * 1000.0);
    s_external_dirty = true;
}

void MarkAxiomChanged() {
    s_external_dirty = true;
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    gs.dirty_layers.MarkFromAxiomEdit();
}

// Clear layer operations (undoable)
void ClearLayer(uint32_t layer_mask, const char* description) {
    if (!s_axiom_tool) return;
    
    auto cmd = std::make_unique<ClearLayerCommand>();
    cmd->layers = layer_mask;
    cmd->description = description;
    s_axiom_tool->push_command(std::move(cmd));
}

void ClearAxioms() {
    ClearLayer(ClearLayerCommand::Axioms, "Clear Axioms");
}

void ClearWater() {
    ClearLayer(ClearLayerCommand::Water, "Clear Water");
}

void ClearRoads() {
    ClearLayer(ClearLayerCommand::Roads, "Clear Roads");
}

void ClearDistricts() {
    ClearLayer(ClearLayerCommand::Districts, "Clear Districts");
}

void ClearLots() {
    ClearLayer(ClearLayerCommand::Lots, "Clear Lots");
}

void ClearBuildings() {
    ClearLayer(ClearLayerCommand::Buildings, "Clear Buildings");
}

void ClearAllData() {
    ClearLayer(ClearLayerCommand::All, "Clear All Data");
}

//this function is called by the AxiomPlacementTool when axioms are modified, to trigger validation and preview updates
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
    out_config.num_seeds = std::max(10, static_cast<int>(axioms.size() * 6 * s_flow_rate));

    if (!RogueCity::App::GeneratorBridge::validate_axioms(out_inputs, out_config)) {
        s_validation_error = "ERROR: Invalid axioms (bounds/overlap)";
        return false;
    }

    s_validation_error.clear();
    return true;
}

bool CanGenerate() {
    if (!s_generation_coordinator || !s_axiom_tool) return false;
    if (s_generation_coordinator->IsGenerating()) return false;
    return !s_axiom_tool->axioms().empty();
}

void ForceGenerate() {
    if (!s_generation_coordinator) return;

    std::vector<RogueCity::Generators::CityGenerator::AxiomInput> inputs;
    RogueCity::Generators::CityGenerator::Config cfg;
    if (!BuildInputs(inputs, cfg)) return;

    s_generation_coordinator->ForceRegeneration(
        inputs,
        cfg,
        RogueCity::App::GenerationRequestReason::ForceGenerate);
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    gs.dirty_layers.MarkAllClean();
    gs.tool_runtime.explicit_generation_pending = false;
    gs.tool_runtime.last_viewport_status = "explicit-generation-triggered";
    gs.tool_runtime.last_viewport_status_frame = gs.frame_counter;
}

const char* GetRogueNavModeName() {
    switch (s_minimap_mode) {
        case MinimapMode::Soliton: return "SOLITON";
        case MinimapMode::Reactive: return "REACTIVE";
        case MinimapMode::Satellite: return "SATELLITE";
        case MinimapMode::Disabled: return "OFF";
        default: return "OFF";
    }
}

const char* GetRogueNavFilterName() {
    switch (s_nav_alert_level) {
        case RogueNavAlert::Normal: return "NORMAL";
        case RogueNavAlert::Caution: return "CAUTION";
        case RogueNavAlert::Evasion: return "EVASION";
        case RogueNavAlert::Alert: return "ALERT";
        default: return "NORMAL";
    }
}

static std::string ToUpperCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

bool SetRogueNavModeByName(const std::string& mode) {
    const std::string value = ToUpperCopy(mode);
    if (value == "SOLITON") {
        s_minimap_mode = MinimapMode::Soliton;
        return true;
    }
    if (value == "REACTIVE") {
        s_minimap_mode = MinimapMode::Reactive;
        return true;
    }
    if (value == "SATELLITE") {
        s_minimap_mode = MinimapMode::Satellite;
        return true;
    }
    if (value == "OFF" || value == "DISABLED") {
        s_minimap_mode = MinimapMode::Disabled;
        return true;
    }
    return false;
}

bool SetRogueNavFilterByName(const std::string& filter) {
    const std::string value = ToUpperCopy(filter);
    if (value == "NORMAL") {
        s_nav_alert_level = RogueNavAlert::Normal;
        return true;
    }
    if (value == "CAUTION") {
        s_nav_alert_level = RogueNavAlert::Caution;
        return true;
    }
    if (value == "EVASION") {
        s_nav_alert_level = RogueNavAlert::Evasion;
        return true;
    }
    if (value == "ALERT") {
        s_nav_alert_level = RogueNavAlert::Alert;
        return true;
    }
    return false;
}

static int MinimapLODLevel(MinimapLOD lod);
static MinimapLOD MinimapLODFromLevel(int level);
static const char* MinimapLODStatusText();

bool IsMinimapManualLODOverride() {
    return !s_minimap_auto_lod;
}

void SetMinimapManualLODOverride(bool enabled) {
    s_minimap_auto_lod = !enabled;
    if (enabled) {
        s_minimap_lod = s_minimap_effective_lod;
    }
}

int GetMinimapManualLODLevel() {
    return MinimapLODLevel(s_minimap_lod);
}

void SetMinimapManualLODLevel(int level) {
    s_minimap_lod = MinimapLODFromLevel(level);
    s_minimap_auto_lod = false;
}

bool IsMinimapAdaptiveQualityEnabled() {
    return s_minimap_adaptive_quality;
}

void SetMinimapAdaptiveQualityEnabled(bool enabled) {
    s_minimap_adaptive_quality = enabled;
}

int GetMinimapEffectiveLODLevel() {
    return MinimapLODLevel(s_minimap_effective_lod);
}

const char* GetMinimapLODStatusText() {
    return MinimapLODStatusText();
}

bool IsMinimapVisible() {
    return s_minimap_visible;
}

void SetMinimapVisible(bool visible) {
    s_minimap_visible = visible;
}

void ToggleMinimapVisible() {
    s_minimap_visible = !s_minimap_visible;
}

bool ApplyGeneratorRequest(
    const std::vector<RogueCity::Generators::CityGenerator::AxiomInput>& axioms,
    const RogueCity::Generators::CityGenerator::Config& config,
    std::string* outError) {
    Initialize();
    if (!s_generation_coordinator) {
        if (outError) {
            *outError = "Realtime preview system is not initialized.";
        }
        return false;
    }
    if (axioms.empty()) {
        if (outError) {
            *outError = "No generation axioms were provided.";
        }
        return false;
    }

    if (!RogueCity::App::GeneratorBridge::validate_axioms(axioms, config)) {
        if (outError) {
            *outError = "Generator request failed axiom validation.";
        }
        s_validation_error = "ERROR: Invalid axioms (bounds/overlap)";
        return false;
    }

    s_seed = config.seed;
    RogueCity::Core::Editor::GetGlobalState().params.seed = config.seed;
    s_validation_error.clear();
    s_generation_coordinator->ForceRegeneration(
        axioms,
        config,
        RogueCity::App::GenerationRequestReason::ExternalRequest);
    s_external_dirty = false;
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    gs.dirty_layers.MarkAllClean();
    gs.tool_runtime.explicit_generation_pending = false;
    gs.tool_runtime.last_viewport_status = "external-generation-triggered";
    gs.tool_runtime.last_viewport_status_frame = gs.frame_counter;

    return true;
}

const char* GetValidationError() {
    return s_validation_error.c_str();
}

// === ROGUENAV MINIMAP RENDERING ===
static ImU32 GetNavAlertColor() {
    switch (s_nav_alert_level) {
        case RogueNavAlert::Alert:   return UITokens::ErrorRed;      // Red - Detected
        case RogueNavAlert::Evasion: return UITokens::AmberGlow;  // Orange - Tracked
        case RogueNavAlert::Caution: return UITokens::YellowWarning; // Yellow - Suspicious
        case RogueNavAlert::Normal:
        default:                     return UITokens::InfoBlue;      // Blue - All clear
    }
}

// Convert world coordinates to minimap UV space [0,1]
static ImVec2 WorldToMinimapUV(const RogueCity::Core::Vec2& world_pos, const RogueCity::Core::Vec2& camera_pos) {
    // World space relative to camera
    const double world_span = static_cast<double>(kMinimapWorldSize) * static_cast<double>(s_minimap_zoom);
    const double rel_x = (world_pos.x - camera_pos.x) / world_span;
    const double rel_y = (world_pos.y - camera_pos.y) / world_span;
    
    // Center on minimap (0.5, 0.5) + relative offset
    return ImVec2(
        static_cast<float>(0.5 + rel_x),
        static_cast<float>(0.5 + rel_y));
}

// Convert minimap pixel coords to world coordinates
static RogueCity::Core::Vec2 MinimapPixelToWorld(const ImVec2& pixel_pos, const ImVec2& minimap_pos, const RogueCity::Core::Vec2& camera_pos) {
    return RC_UI::Viewport::MinimapPixelToWorld(
        RogueCity::Core::Vec2(pixel_pos.x, pixel_pos.y),
        RogueCity::Core::Vec2(minimap_pos.x, minimap_pos.y),
        kMinimapSize,
        camera_pos,
        kMinimapWorldSize,
        s_minimap_zoom);
}

static RogueCity::Core::Vec2 ClampMinimapCameraToConstraints(
    const RogueCity::Core::Vec2& camera_pos,
    const RogueCity::Core::Editor::GlobalState& gs) {
    return RC_UI::Viewport::ClampToWorldConstraints(camera_pos, gs.world_constraints);
}

static int MinimapLODLevel(MinimapLOD lod) {
    switch (lod) {
        case MinimapLOD::Detail: return 0;    // Full detail
        case MinimapLOD::Tactical: return 1;  // Medium
        case MinimapLOD::Strategic:
        default: return 2;                    // Coarse
    }
}

static MinimapLOD MinimapLODFromLevel(int level) {
    switch (std::clamp(level, 0, 2)) {
        case 0: return MinimapLOD::Detail;
        case 1: return MinimapLOD::Tactical;
        case 2:
        default: return MinimapLOD::Strategic;
    }
}

static MinimapLOD ComputeAutoMinimapLOD(float viewport_zoom, float minimap_zoom) {
    MinimapLOD lod = MinimapLOD::Detail;
    if (viewport_zoom <= 0.85f) {
        lod = MinimapLOD::Strategic;
    } else if (viewport_zoom <= 1.6f) {
        lod = MinimapLOD::Tactical;
    } else {
        lod = MinimapLOD::Detail;
    }

    // Secondary modifier: minimap zoom nudges one level, but never dominates viewport zoom.
    if (minimap_zoom < 0.75f) {
        const int coarser = std::min(2, MinimapLODLevel(lod) + 1);
        lod = MinimapLODFromLevel(coarser);
    } else if (minimap_zoom > 1.85f) {
        const int finer = std::max(0, MinimapLODLevel(lod) - 1);
        lod = MinimapLODFromLevel(finer);
    }

    return lod;
}

static MinimapLOD ActiveBaseMinimapLOD(float viewport_zoom) {
    return s_minimap_auto_lod
        ? ComputeAutoMinimapLOD(viewport_zoom, s_minimap_zoom)
        : s_minimap_lod;
}

static MinimapLOD CoarsenMinimapLOD(MinimapLOD lod) {
    return MinimapLODFromLevel(std::min(2, MinimapLODLevel(lod) + 1));
}

static void UpdateMinimapFPSPressure(float current_fps) {
    s_minimap_fps_history.push_back(current_fps);
    if (s_minimap_fps_history.size() > 60u) {
        s_minimap_fps_history.pop_front();
    }

    if (s_minimap_fps_history.empty()) {
        return;
    }

    const float fps_sum = std::accumulate(s_minimap_fps_history.begin(), s_minimap_fps_history.end(), 0.0f);
    const float avg_fps = fps_sum / static_cast<float>(s_minimap_fps_history.size());

    if (avg_fps < 45.0f) {
        ++s_minimap_low_fps_streak;
        s_minimap_high_fps_streak = 0;
    } else if (avg_fps > 55.0f) {
        ++s_minimap_high_fps_streak;
        s_minimap_low_fps_streak = 0;
    } else {
        s_minimap_low_fps_streak = 0;
        s_minimap_high_fps_streak = 0;
    }

    if (s_minimap_low_fps_streak >= 3) {
        s_minimap_fps_degrade_steps = std::min(2, s_minimap_fps_degrade_steps + 1);
        s_minimap_low_fps_streak = 0;
    } else if (s_minimap_high_fps_streak >= 3) {
        s_minimap_fps_degrade_steps = std::max(0, s_minimap_fps_degrade_steps - 1);
        s_minimap_high_fps_streak = 0;
    }
}

static int CountDirtyTextureLayers(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.HasTextureSpace()) {
        return 0;
    }

    int dirty = 0;
    for (int i = 0; i < static_cast<int>(RogueCity::Core::Data::TextureLayer::Count); ++i) {
        const auto layer = static_cast<RogueCity::Core::Data::TextureLayer>(i);
        if (gs.TextureSpaceRef().isDirty(layer)) {
            ++dirty;
        }
    }
    return dirty;
}

static MinimapLOD ComputeAdaptiveMinimapLOD(
    MinimapLOD base_lod,
    const RogueCity::Core::Editor::GlobalState& gs) {
    if (!s_minimap_adaptive_quality || !s_minimap_auto_lod) {
        return base_lod;
    }

    UpdateMinimapFPSPressure(ImGui::GetIO().Framerate);

    MinimapLOD effective = base_lod;
    for (int i = 0; i < s_minimap_fps_degrade_steps; ++i) {
        effective = CoarsenMinimapLOD(effective);
    }

    // Dirty-pressure trigger: >3 dirty texture layers while active painting/terrain edits.
    const int dirty_texture_layers = CountDirtyTextureLayers(gs);
    const bool painting_in_progress =
        gs.IsTextureEditingInProgress(3) ||
        (ImGui::GetIO().MouseDown[0] && dirty_texture_layers > 0);
    if (dirty_texture_layers > 3 && painting_in_progress) {
        effective = CoarsenMinimapLOD(effective);
    }

    return effective;
}

static void CycleManualMinimapLOD() {
    s_minimap_lod = MinimapLODFromLevel((MinimapLODLevel(s_minimap_lod) + 1) % 3);
}

static float ComputeViewportZoomForLOD() {
    if (s_primary_viewport == nullptr) {
        return 1.0f;
    }
    const float camera_z = s_primary_viewport->get_camera_z();
    return 500.0f / std::max(100.0f, camera_z);
}

static int OverlayStrideForLOD(MinimapLOD lod) {
    switch (lod) {
        case MinimapLOD::Detail: return 1;
        case MinimapLOD::Tactical: return 2;
        case MinimapLOD::Strategic:
        default: return 4;
    }
}

enum class RoadDetailClass : uint8_t {
    Arterial = 0,
    Collector = 1,
    Local = 2
};

static RoadDetailClass ClassifyRoadDetail(RogueCity::Core::RoadType type) {
    using RogueCity::Core::RoadType;
    switch (type) {
        case RoadType::Highway:
        case RoadType::Arterial:
        case RoadType::Avenue:
        case RoadType::Boulevard:
        case RoadType::M_Major:
            return RoadDetailClass::Arterial;
        case RoadType::Street:
        case RoadType::M_Minor:
            return RoadDetailClass::Collector;
        case RoadType::Lane:
        case RoadType::Alleyway:
        case RoadType::CulDeSac:
        case RoadType::Drive:
        case RoadType::Driveway:
        default:
            return RoadDetailClass::Local;
    }
}

static bool ShouldRenderRoadForLOD(RogueCity::Core::RoadType type, MinimapLOD lod) {
    const RoadDetailClass detail = ClassifyRoadDetail(type);
    if (lod == MinimapLOD::Detail) {
        return true;
    }
    if (lod == MinimapLOD::Tactical) {
        return detail != RoadDetailClass::Local; // Drop local at LOD1.
    }
    return detail == RoadDetailClass::Arterial; // Drop collector/local at LOD2.
}

struct ViewportRoadStyle {
    ImU32 color{ TokenColor(UITokens::CyanAccent, 200u) };
    float width{ 2.0f };
};

static ViewportRoadStyle ResolveViewportRoadStyle(RogueCity::Core::RoadType type) {
    using RogueCity::Core::RoadType;
    switch (type) {
        case RoadType::Highway:
            return {TokenColor(UITokens::AmberGlow, 230u), 4.6f};
        case RoadType::Arterial:
        case RoadType::Avenue:
        case RoadType::Boulevard:
        case RoadType::M_Major:
            return {TokenColor(UITokens::CyanAccent, 220u), 3.2f};
        case RoadType::Street:
        case RoadType::M_Minor:
            return {TokenColor(UITokens::InfoBlue, 210u), 2.2f};
        case RoadType::Lane:
        case RoadType::Alleyway:
        case RoadType::CulDeSac:
        case RoadType::Drive:
        case RoadType::Driveway:
        default:
            return {TokenColor(UITokens::TextSecondary, 180u), 1.6f};
    }
}

static const char* MinimapLODStatusText() {
    return s_minimap_lod_status_text.c_str();
}

static void RenderMinimapSelectionHighlights(
    ImDrawList* draw_list,
    const RogueCity::Core::Editor::GlobalState& gs,
    const RogueCity::Core::Vec2& camera_pos,
    const ImVec2& minimap_pos) {
    if (gs.selection_manager.Count() == 0) {
        return;
    }

    const auto* primary = gs.selection_manager.Primary();
    for (const auto& item : gs.selection_manager.Items()) {
        RogueCity::Core::Vec2 anchor{};
        if (!ResolveSelectionAnchor(gs, item.kind, item.id, anchor)) {
            continue;
        }

        const ImVec2 uv = WorldToMinimapUV(anchor, camera_pos);
        if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
            continue;
        }

        const bool is_primary = primary != nullptr && primary->kind == item.kind && primary->id == item.id;
        const ImU32 ring_color = is_primary
            ? TokenColor(UITokens::YellowWarning, 235u)
            : TokenColor(UITokens::TextPrimary, 200u);
        const float radius = is_primary ? 5.5f : 4.0f;
        const float thickness = is_primary ? 2.0f : 1.4f;

        const ImVec2 screen_pos(minimap_pos.x + uv.x * kMinimapSize, minimap_pos.y + uv.y * kMinimapSize);
        draw_list->AddCircle(screen_pos, radius, ring_color, 20, thickness);
    }
}

static ImU32 DistrictColor(RogueCity::Core::DistrictType type, MinimapLOD lod) {
    const int alpha = (lod == MinimapLOD::Strategic) ? 120 : 165;
    const auto a = static_cast<uint8_t>(alpha);
    switch (type) {
        case RogueCity::Core::DistrictType::Residential: return TokenColor(UITokens::GreenHUD, a);
        case RogueCity::Core::DistrictType::Commercial: return TokenColor(UITokens::InfoBlue, a);
        case RogueCity::Core::DistrictType::Industrial: return TokenColor(UITokens::AmberGlow, a);
        case RogueCity::Core::DistrictType::Civic: return TokenColor(UITokens::MagentaHighlight, a);
        case RogueCity::Core::DistrictType::Mixed:
        default: return TokenColor(UITokens::TextSecondary, a);
    }
}

static void RenderHeightOverlay(
    ImDrawList* draw_list,
    const RogueCity::Core::Data::TextureSpace& texture_space,
    const RogueCity::Core::Vec2& camera_pos,
    const ImVec2& minimap_pos,
    MinimapLOD lod) {
    const int lod_stride = OverlayStrideForLOD(lod);
    const int sample_step = 4 * lod_stride;
    for (int py = 0; py < static_cast<int>(kMinimapSize); py += sample_step) {
        for (int px = 0; px < static_cast<int>(kMinimapSize); px += sample_step) {
            const ImVec2 pixel(minimap_pos.x + static_cast<float>(px), minimap_pos.y + static_cast<float>(py));
            const RogueCity::Core::Vec2 world = MinimapPixelToWorld(pixel, minimap_pos, camera_pos);
            if (!texture_space.coordinateSystem().isInBounds(world)) {
                continue;
            }

            const float h = texture_space.heightLayer().sampleBilinear(
                texture_space.coordinateSystem().worldToUV(world));
            const float normalized = std::clamp(h / 120.0f, 0.0f, 1.0f);
            const ImU32 color = TokenColor(LerpColor(UITokens::InfoBlue, UITokens::AmberGlow, normalized), 48u);
            draw_list->AddRectFilled(
                pixel,
                ImVec2(pixel.x + static_cast<float>(sample_step), pixel.y + static_cast<float>(sample_step)),
                color);
        }
    }
}

static void RenderTensorOverlay(
    ImDrawList* draw_list,
    const RogueCity::Core::Data::TextureSpace& texture_space,
    const RogueCity::Core::Vec2& camera_pos,
    const ImVec2& minimap_pos,
    MinimapLOD lod) {
    const int lod_stride = OverlayStrideForLOD(lod);
    const int sample_step = 24 * lod_stride;
    for (int py = sample_step / 2; py < static_cast<int>(kMinimapSize); py += sample_step) {
        for (int px = sample_step / 2; px < static_cast<int>(kMinimapSize); px += sample_step) {
            const ImVec2 pixel(minimap_pos.x + static_cast<float>(px), minimap_pos.y + static_cast<float>(py));
            const RogueCity::Core::Vec2 world = MinimapPixelToWorld(pixel, minimap_pos, camera_pos);
            if (!texture_space.coordinateSystem().isInBounds(world)) {
                continue;
            }

            const RogueCity::Core::Vec2 dir = texture_space.tensorLayer().sampleBilinearTyped<RogueCity::Core::Vec2>(
                texture_space.coordinateSystem().worldToUV(world));
            if (dir.lengthSquared() <= 1e-6) {
                continue;
            }
            RogueCity::Core::Vec2 unit = dir;
            unit.normalize();
            const float half_len = 5.0f;
            const ImVec2 a(pixel.x - static_cast<float>(unit.x) * half_len, pixel.y - static_cast<float>(unit.y) * half_len);
            const ImVec2 b(pixel.x + static_cast<float>(unit.x) * half_len, pixel.y + static_cast<float>(unit.y) * half_len);
            draw_list->AddLine(a, b, TokenColor(UITokens::CyanAccent, 130u), 1.0f);
        }
    }
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
        TokenColor(UITokens::BackgroundDark, 200u)
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
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary));
    ImGui::Text("%s", mode_text);
    ImGui::PopStyleColor();

    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    const float viewport_zoom = ComputeViewportZoomForLOD();
    const MinimapLOD base_lod = ActiveBaseMinimapLOD(viewport_zoom);
    const MinimapLOD active_lod = ComputeAdaptiveMinimapLOD(base_lod, gs);
    s_minimap_effective_lod = active_lod;
    gs.minimap_manual_lod = !s_minimap_auto_lod;
    gs.minimap_lod_level = static_cast<uint8_t>(MinimapLODLevel(active_lod));

    {
        std::ostringstream status;
        if (s_minimap_auto_lod) {
            status << "LOD: Auto (" << MinimapLODLevel(active_lod) << ")";
        } else {
            status << "LOD: Manual (" << MinimapLODLevel(s_minimap_lod) << ")";
        }
        if (active_lod != base_lod) {
            status << " -> " << MinimapLODLevel(active_lod);
        }
        if (s_minimap_adaptive_quality) {
            status << " AQ";
        }
        s_minimap_lod_status_text = status.str();
    }

    ImGui::SetCursorScreenPos(ImVec2(minimap_pos.x + 8.0f, minimap_pos.y + 24.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary));
    ImGui::Text("%s", MinimapLODStatusText());
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
    
    // Shared scene frame (main viewport + minimap) is the authoritative render payload.
    const auto camera_pos = s_scene_frame.camera_xy;
    const auto* output = s_scene_frame.output;
    
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
                ImU32 axiom_color = UITokens::MagentaHighlight;
                draw_list->AddCircleFilled(screen_pos, 3.0f, axiom_color);
                draw_list->AddCircle(screen_pos, 5.0f, axiom_color, 8, 1.0f);
            }
        }
    }
    
    if (output != nullptr) {
        const size_t district_step = (active_lod == MinimapLOD::Detail) ? 1u : ((active_lod == MinimapLOD::Tactical) ? 2u : 6u);
        for (const auto& district : output->districts) {
            if (district.border.empty()) {
                continue;
            }

            if (active_lod != MinimapLOD::Strategic) {
                for (size_t i = 0; i < district.border.size(); i += district_step) {
                    const size_t ni = (i + district_step) % district.border.size();
                    const ImVec2 uv1 = WorldToMinimapUV(district.border[i], camera_pos);
                    const ImVec2 uv2 = WorldToMinimapUV(district.border[ni], camera_pos);
                    if ((uv1.x < 0.0f || uv1.x > 1.0f || uv1.y < 0.0f || uv1.y > 1.0f) &&
                        (uv2.x < 0.0f || uv2.x > 1.0f || uv2.y < 0.0f || uv2.y > 1.0f)) {
                        continue;
                    }
                    draw_list->AddLine(
                        ImVec2(minimap_pos.x + uv1.x * kMinimapSize, minimap_pos.y + uv1.y * kMinimapSize),
                        ImVec2(minimap_pos.x + uv2.x * kMinimapSize, minimap_pos.y + uv2.y * kMinimapSize),
                        DistrictColor(district.type, active_lod),
                        1.0f);
                }
            }

            RogueCity::Core::Vec2 centroid{};
            for (const auto& p : district.border) {
                centroid += p;
            }
            centroid /= static_cast<double>(district.border.size());
            const ImVec2 uv = WorldToMinimapUV(centroid, camera_pos);
            if (uv.x >= 0.0f && uv.x <= 1.0f && uv.y >= 0.0f && uv.y <= 1.0f) {
                const ImVec2 screen_pos(minimap_pos.x + uv.x * kMinimapSize, minimap_pos.y + uv.y * kMinimapSize);
                draw_list->AddCircleFilled(screen_pos, (active_lod == MinimapLOD::Strategic) ? 3.5f : 2.5f, DistrictColor(district.type, active_lod));
            }
        }

        const auto& roads = output->roads;
        const size_t sample_step = (active_lod == MinimapLOD::Detail) ? 1u : ((active_lod == MinimapLOD::Tactical) ? 3u : 8u);
        const float road_width = (active_lod == MinimapLOD::Detail) ? 1.25f : 1.0f;
        const ImU32 road_color = (active_lod == MinimapLOD::Strategic)
            ? TokenColor(UITokens::InfoBlue, 150u)
            : UITokens::CyanAccent;
        for (auto it = roads.begin(); it != roads.end(); ++it) {
            const auto& road = *it;
            if (road.points.size() < 2 || !ShouldRenderRoadForLOD(road.type, active_lod)) {
                continue;
            }

            for (size_t i = 0; i + 1 < road.points.size(); i += sample_step) {
                const size_t next_i = std::min(i + sample_step, road.points.size() - 1);
                const ImVec2 uv1 = WorldToMinimapUV(road.points[i], camera_pos);
                const ImVec2 uv2 = WorldToMinimapUV(road.points[next_i], camera_pos);
                if ((uv1.x < 0.0f || uv1.x > 1.0f || uv1.y < 0.0f || uv1.y > 1.0f) &&
                    (uv2.x < 0.0f || uv2.x > 1.0f || uv2.y < 0.0f || uv2.y > 1.0f)) {
                    continue;
                }
                draw_list->AddLine(
                    ImVec2(minimap_pos.x + uv1.x * kMinimapSize, minimap_pos.y + uv1.y * kMinimapSize),
                    ImVec2(minimap_pos.x + uv2.x * kMinimapSize, minimap_pos.y + uv2.y * kMinimapSize),
                    road_color,
                    road_width);
            }
        }

        if (active_lod == MinimapLOD::Detail) {
            std::unordered_map<uint32_t, const RogueCity::Core::LotToken*> lots_by_id;
            lots_by_id.reserve(output->lots.size());
            for (const auto& lot : output->lots) {
                lots_by_id[lot.id] = &lot;
            }

            for (const auto& building : output->buildings) {
                const auto lot_it = lots_by_id.find(building.lot_id);
                if (lot_it == lots_by_id.end() || lot_it->second == nullptr || lot_it->second->boundary.size() < 3) {
                    const ImVec2 uv = WorldToMinimapUV(building.position, camera_pos);
                    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
                        continue;
                    }
                    const ImVec2 screen_pos(minimap_pos.x + uv.x * kMinimapSize, minimap_pos.y + uv.y * kMinimapSize);
                    draw_list->AddRectFilled(
                        ImVec2(screen_pos.x - 1.5f, screen_pos.y - 1.5f),
                        ImVec2(screen_pos.x + 1.5f, screen_pos.y + 1.5f),
                        TokenColor(UITokens::TextPrimary, 220u));
                    continue;
                }

                const auto& footprint = lot_it->second->boundary;
                for (size_t i = 0; i < footprint.size(); ++i) {
                    const size_t ni = (i + 1) % footprint.size();
                    const ImVec2 uv1 = WorldToMinimapUV(footprint[i], camera_pos);
                    const ImVec2 uv2 = WorldToMinimapUV(footprint[ni], camera_pos);
                    if ((uv1.x < 0.0f || uv1.x > 1.0f || uv1.y < 0.0f || uv1.y > 1.0f) &&
                        (uv2.x < 0.0f || uv2.x > 1.0f || uv2.y < 0.0f || uv2.y > 1.0f)) {
                        continue;
                    }
                    draw_list->AddLine(
                        ImVec2(minimap_pos.x + uv1.x * kMinimapSize, minimap_pos.y + uv1.y * kMinimapSize),
                        ImVec2(minimap_pos.x + uv2.x * kMinimapSize, minimap_pos.y + uv2.y * kMinimapSize),
                        TokenColor(UITokens::TextPrimary, 210u),
                        1.0f);
                }
            }
        } else if (active_lod == MinimapLOD::Tactical) {
            for (const auto& building : output->buildings) {
                const ImVec2 uv = WorldToMinimapUV(building.position, camera_pos);
                if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
                    continue;
                }
                const ImVec2 screen_pos(minimap_pos.x + uv.x * kMinimapSize, minimap_pos.y + uv.y * kMinimapSize);
                draw_list->AddCircleFilled(screen_pos, 1.4f, TokenColor(UITokens::TextPrimary, 185u));
            }
        }
    }

    // Selection highlights are always visible regardless of LOD policy.
    RenderMinimapSelectionHighlights(draw_list, gs, camera_pos, minimap_pos);

    if (gs.HasTextureSpace()) {
        const auto& texture_space = gs.TextureSpaceRef();
        if (gs.debug_show_height_overlay) {
            RenderHeightOverlay(draw_list, texture_space, camera_pos, minimap_pos, active_lod);
        }
        if (gs.debug_show_tensor_overlay) {
            RenderTensorOverlay(draw_list, texture_space, camera_pos, minimap_pos, active_lod);
        }
    }

    // Frustum rectangle from current primary viewport zoom.
    // RC-0.09-Test P1 fix: Clamp frustum to minimap bounds
    const double half_w_world = (static_cast<double>(viewport_size.x) * 0.5) / std::max(0.05f, viewport_zoom);
    const double half_h_world = (static_cast<double>(viewport_size.y) * 0.5) / std::max(0.05f, viewport_zoom);
    const std::array<RogueCity::Core::Vec2, 4> frustum_world = {
        RogueCity::Core::Vec2(camera_pos.x - half_w_world, camera_pos.y - half_h_world),
        RogueCity::Core::Vec2(camera_pos.x + half_w_world, camera_pos.y - half_h_world),
        RogueCity::Core::Vec2(camera_pos.x + half_w_world, camera_pos.y + half_h_world),
        RogueCity::Core::Vec2(camera_pos.x - half_w_world, camera_pos.y + half_h_world)
    };
    const ImVec2 minimap_min = minimap_pos;
    const ImVec2 minimap_max = ImVec2(minimap_pos.x + kMinimapSize, minimap_pos.y + kMinimapSize);
    
    for (size_t i = 0; i < frustum_world.size(); ++i) {
        const size_t ni = (i + 1) % frustum_world.size();
        const ImVec2 uv1 = WorldToMinimapUV(frustum_world[i], camera_pos);
        const ImVec2 uv2 = WorldToMinimapUV(frustum_world[ni], camera_pos);
        ImVec2 p1 = ImVec2(minimap_pos.x + uv1.x * kMinimapSize, minimap_pos.y + uv1.y * kMinimapSize);
        ImVec2 p2 = ImVec2(minimap_pos.x + uv2.x * kMinimapSize, minimap_pos.y + uv2.y * kMinimapSize);
        
        // Clamp frustum line endpoints to minimap bounds
        p1.x = std::clamp(p1.x, minimap_min.x, minimap_max.x);
        p1.y = std::clamp(p1.y, minimap_min.y, minimap_max.y);
        p2.x = std::clamp(p2.x, minimap_min.x, minimap_max.x);
        p2.y = std::clamp(p2.y, minimap_min.y, minimap_max.y);
        
        draw_list->AddLine(p1, p2, UITokens::YellowWarning, 1.5f);
    }
    
    // Center crosshair (current camera position)
    const ImVec2 center = ImVec2(
        minimap_pos.x + kMinimapSize / 2.0f,
        minimap_pos.y + kMinimapSize / 2.0f
    );
    draw_list->AddCircleFilled(center, 4.0f, alert_color);
    draw_list->AddCircle(center, 8.0f, alert_color, 16, 1.5f);

    // Never-blank anchors: world frame + cursor/world readout.
    draw_list->AddRect(
        ImVec2(minimap_pos.x + 6.0f, minimap_pos.y + 6.0f),
        ImVec2(minimap_pos.x + kMinimapSize - 6.0f, minimap_pos.y + kMinimapSize - 6.0f),
        TokenColor(UITokens::TextPrimary, 40u),
        0.0f,
        0,
        1.0f);

    const ImVec2 mouse = ImGui::GetMousePos();
    const bool mouse_in_minimap =
        mouse.x >= minimap_pos.x && mouse.x <= (minimap_pos.x + kMinimapSize) &&
        mouse.y >= minimap_pos.y && mouse.y <= (minimap_pos.y + kMinimapSize);
    const RogueCity::Core::Vec2 cursor_world = mouse_in_minimap
        ? MinimapPixelToWorld(mouse, minimap_pos, camera_pos)
        : camera_pos;

    ImGui::SetCursorScreenPos(ImVec2(minimap_pos.x + 8.0f, minimap_pos.y + kMinimapSize - 36.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary));
    ImGui::Text(
        "CUR %.0f, %.0f",
        static_cast<float>(cursor_world.x),
        static_cast<float>(cursor_world.y));
    ImGui::PopStyleColor();
}

void DrawAxiomLibraryContent() {
    if (s_axiom_tool == nullptr) {
        return;
    }

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();

    ImGui::TextColored(TokenColorF(UITokens::TextPrimary, 217u), "Axiom Library (Ctrl: apply to selection)");

    const float icon_size = std::max(38.0f, ImGui::GetFrameHeight() * 1.9f);
    const float spacing = std::max(8.0f, ImGui::GetStyle().ItemSpacing.x);
    const int columns = std::max(1, static_cast<int>(
        std::floor((ImGui::GetContentRegionAvail().x + spacing) / (icon_size + spacing))));
    const auto actions = RC_UI::Tools::GetToolActionsForLibrary(ToolLibrary::Axiom);

    const auto default_type = s_axiom_tool->default_axiom_type();
    const bool apply_to_selected = ImGui::GetIO().KeyCtrl;

    for (int i = 0; i < static_cast<int>(actions.size()); ++i) {
        const auto& action = actions[static_cast<size_t>(i)];
        RogueCity::App::AxiomType axiom_type = RogueCity::App::AxiomType::Grid;
        if (!TryAxiomTypeFromActionId(action.id, axiom_type)) {
            continue;
        }
        const auto& info = RogueCity::App::GetAxiomTypeInfo(axiom_type);

        if (i > 0 && (i % columns) != 0) {
            ImGui::SameLine(0.0f, spacing);
        }

        ImGui::PushID(i);
        if (ImGui::InvisibleButton("AxiomType", ImVec2(icon_size, icon_size))) {
            if (apply_to_selected) {
                if (auto* selected = s_axiom_tool->get_selected_axiom()) {
                    selected->set_type(axiom_type);
                    s_library_modified = true;
                }
            } else {
                s_axiom_tool->set_default_axiom_type(axiom_type);
            }

            std::string dispatch_status;
            RC_UI::Tools::DispatchContext dispatch_context{
                &hfsm,
                &gs,
                &uiint,
                "Axiom Library"
            };
            const auto dispatch_result = RC_UI::Tools::DispatchToolAction(action.id, dispatch_context, &dispatch_status);
            (void)dispatch_result;
        }

        const ImVec2 bmin = ImGui::GetItemRectMin();
        const ImVec2 bmax = ImGui::GetItemRectMax();
        const ImVec2 c((bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const bool is_default = (default_type == axiom_type);
        const ImU32 bg = TokenColor(UITokens::BackgroundDark, ImGui::IsItemHovered() ? 200u : 140u);
        const ImU32 border = is_default ? TokenColor(UITokens::TextPrimary, 220u) : TokenColor(UITokens::TextSecondary, 160u);

        dl->AddRectFilled(bmin, bmax, bg, 8.0f);
        dl->AddRect(bmin, bmax, border, 8.0f, 0, is_default ? 2.5f : 1.5f);
        RogueCity::App::DrawAxiomIcon(dl, c, icon_size * 0.32f, axiom_type, info.primary_color);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s\n%s", info.name, action.tooltip);
        }
        uiint.RegisterWidget({"button", action.label, std::string("action:") + RC_UI::Tools::ToolActionName(action.id), {"tool", "library", "axiom"}});
        ImGui::PopID();
    }

    uiint.RegisterWidget({"table", "Axiom Types", "axiom.types[]", {"axiom", "library"}});
}

void DrawContent(float dt) {
    Initialize();

    // Get editor state to determine if axiom tool should be active
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    auto current_state = hfsm.state();

    const bool axiom_mode =
        (current_state == RogueCity::Core::Editor::EditorState::Editing_Axioms) ||
        (current_state == RogueCity::Core::Editor::EditorState::Viewport_PlaceAxiom);
    
    // Update viewport sync + generation scheduler through a shared scene controller path.
    RC_UI::Viewport::UpdateSceneController(
        s_scene_frame,
        RC_UI::Viewport::SceneControllerUpdateInput{
            dt,
            s_primary_viewport.get(),
            s_sync_manager.get(),
            s_generation_coordinator.get(),
            RC_UI::GetMinimapViewport(),
        });
    
    // Render primary viewport with axiom tool integration
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 viewport_pos = ImGui::GetCursorScreenPos();
    const ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    if (viewport_size.x <= 0.0f || viewport_size.y <= 0.0f) {
        return;
    }
    ImGui::InvisibleButton("##ViewportCanvas", viewport_size);
    const bool viewport_canvas_hovered = ImGui::IsItemHovered();
    const bool viewport_canvas_active = ImGui::IsItemActive();
    const ImVec2 viewport_min = ImGui::GetItemRectMin();
    const ImVec2 viewport_max = ImGui::GetItemRectMax();
    draw_list->PushClipRect(viewport_min, viewport_max, true);
    
    // Background (Y2K grid pattern)
    draw_list->AddRectFilled(
        viewport_pos,
        ImVec2(viewport_pos.x + viewport_size.x, viewport_pos.y + viewport_size.y),
        TokenColor(UITokens::BackgroundDark)
    );
    
    // Grid overlay (subtle Y2K aesthetic)
    const float grid_spacing = 50.0f;  // 50 meter grid
    const ImU32 grid_color = TokenColor(UITokens::GridOverlay, 100u);
    
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
    const bool ui_modified_axiom = s_library_modified;
    s_library_modified = false;

    // ---------------------------------------------------------------------
    // Viewport mouse interaction (placement + manipulation)
    // ---------------------------------------------------------------------
    // TODO(hfsm-context): Route right-click context actions through HFSM command maps,
    // then expose user-customizable context->ribbon transforms for workspace tailoring.
    const ImVec2 mouse_pos = ImGui::GetMousePos();
    const bool in_viewport = viewport_canvas_hovered;
    const bool editor_hovered =
        viewport_canvas_hovered || ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    const bool any_item_active = ImGui::IsAnyItemActive();

    const ImVec2 minimap_pos_bounds = ImVec2(
        viewport_pos.x + viewport_size.x - kMinimapSize - kMinimapPadding,
        viewport_pos.y + kMinimapPadding
    );
    const ImVec2 minimap_max_bounds = ImVec2(minimap_pos_bounds.x + kMinimapSize, minimap_pos_bounds.y + kMinimapSize);
    const bool minimap_hovered = s_minimap_visible &&
        (mouse_pos.x >= minimap_pos_bounds.x && mouse_pos.x <= minimap_max_bounds.x &&
         mouse_pos.y >= minimap_pos_bounds.y && mouse_pos.y <= minimap_max_bounds.y);
    const UiInputGateState input_gate = RC_UI::BuildUiInputGateState(
        editor_hovered,
        viewport_canvas_hovered,
        viewport_canvas_active,
        any_item_active,
        minimap_hovered);
    RC_UI::PublishUiInputGateState(input_gate);
    const bool allow_viewport_mouse_actions = RC_UI::AllowViewportMouseActions(input_gate);
    const bool allow_viewport_key_actions = RC_UI::AllowViewportKeyActions(input_gate);
    RC_UI::Tools::DispatchContext command_dispatch{
        &hfsm,
        &gs,
        &uiint,
        "Viewport Commands"
    };
    RC_UI::Viewport::ProcessViewportCommandTriggers(
        RC_UI::Viewport::CommandInteractionParams{
            input_gate,
            in_viewport,
            minimap_hovered,
            mouse_pos,
            &gs.config,
        },
        RC_UI::Viewport::CommandMenuStateBundle{
            &s_smart_command_menu,
            &s_pie_command_menu,
            &s_command_palette,
        });

    const RC_UI::Viewport::AxiomInteractionResult axiom_interaction =
        RC_UI::Viewport::ProcessAxiomViewportInteraction(
            RC_UI::Viewport::AxiomInteractionParams{
                axiom_mode,
                allow_viewport_mouse_actions,
                allow_viewport_key_actions,
                mouse_pos,
                s_primary_viewport.get(),
                s_axiom_tool.get(),
                &gs,
            });
    if (axiom_interaction.active && axiom_interaction.has_world_pos) {
        ImGui::SetCursorScreenPos(ImVec2(10, 30));
        ImGui::TextColored(
            TokenColorF(UITokens::CyanAccent, 178u),
            "Mouse World: (%.1f, %.1f)%s",
            axiom_interaction.world_pos.x,
            axiom_interaction.world_pos.y,
            axiom_interaction.nav_active ? " [NAV]" : "");
    }

    const RC_UI::Viewport::NonAxiomInteractionResult non_axiom_interaction =
        RC_UI::Viewport::ProcessNonAxiomViewportInteraction(
        RC_UI::Viewport::NonAxiomInteractionParams{
            axiom_mode,
            in_viewport,
            minimap_hovered,
            allow_viewport_mouse_actions,
            allow_viewport_key_actions,
            mouse_pos,
            s_primary_viewport.get(),
            current_state,
            &gs,
        },
        &s_non_axiom_interaction);
    if (non_axiom_interaction.requires_explicit_generation) {
        gs.tool_runtime.explicit_generation_pending = true;
    }

    if (!editor_hovered || !in_viewport || minimap_hovered || !allow_viewport_mouse_actions) {
        gs.hovered_entity.reset();
    }
    
    // Always update axiom tool (for animations)
    s_axiom_tool->update(dt, *s_primary_viewport);
    SyncToolAxiomsToGlobalState();

    // Build generation inputs/config for preview
    std::vector<RogueCity::Generators::CityGenerator::AxiomInput> axiom_inputs;
    RogueCity::Generators::CityGenerator::Config config;
    const bool inputs_ok = BuildInputs(axiom_inputs, config);
    if (inputs_ok) {
        // Debounced live preview: keep updating the request while the user manipulates axioms.
        const bool domain_live_preview_enabled = gs.generation_policy.IsLive(gs.tool_runtime.active_domain);
        if (s_generation_coordinator && s_live_preview && domain_live_preview_enabled &&
            (ui_modified_axiom || s_external_dirty || s_axiom_tool->is_interacting() || s_axiom_tool->consume_dirty())) {
            s_generation_coordinator->RequestRegeneration(
                axiom_inputs,
                config,
                RogueCity::App::GenerationRequestReason::LivePreview);
            s_external_dirty = false;
            gs.tool_runtime.explicit_generation_pending = false;
            gs.dirty_layers.MarkFromAxiomEdit();
        }
    }
    
    // Render axioms
    if (gs.show_layer_axioms) {
        const auto& axioms = s_axiom_tool->axioms();
        for (const auto& axiom : axioms) {
            axiom->render(draw_list, *s_primary_viewport);
        }
    }
    
    const char* viewport_status = gs.tool_runtime.last_viewport_status.empty()
        ? "idle"
        : gs.tool_runtime.last_viewport_status.c_str();
    const char* active_domain = RC_UI::Tools::ToolDomainName(gs.tool_runtime.active_domain);

    // Status text overlay (cockpit style) - moved down to avoid overlap with debug
    if (axiom_mode && s_generation_coordinator) {
        ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 20, viewport_pos.y + 22));
        ImGui::TextColored(TokenColorF(UITokens::TextPrimary, 235u), "MODE: AXIOM-EDIT");
        DrawAxiomModeStatus(*s_generation_coordinator->Preview(), ImVec2(viewport_pos.x + 20, viewport_pos.y + 60));
        if (s_generation_coordinator->IsGenerating()) {
            ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 20, viewport_pos.y + 98));
            if (ImGui::Button("_STOP_SWEEP")) {
                s_generation_coordinator->CancelGeneration();
                gs.tool_runtime.last_viewport_status = "preview-cancelled";
                gs.tool_runtime.last_viewport_status_frame = gs.frame_counter;
            }
        }
        ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 20, viewport_pos.y + 80));
        ImGui::TextColored(TokenColorF(UITokens::TextSecondary),
            "Click-drag to set radius | Edit type via palette");
    } else {
        ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 20, viewport_pos.y + 60));
        ImGui::TextColored(TokenColorF(UITokens::InfoBlue, 178u), "Mode: %s", active_domain);
        ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 20, viewport_pos.y + 78));
        ImGui::TextColored(TokenColorF(UITokens::TextSecondary, 230u), "Viewport: %s", viewport_status);
    }
    if (!axiom_mode && gs.tool_runtime.explicit_generation_pending) {
        ImGui::SetCursorScreenPos(ImVec2(viewport_pos.x + 20, viewport_pos.y + 96));
        ImGui::TextColored(
            TokenColorF(UITokens::YellowWarning, 240u),
            "Generation pending (explicit trigger required)");
    }
    
    // Axiom count display
    ImGui::SetCursorScreenPos(ImVec2(
        viewport_pos.x + viewport_size.x - 120,
        viewport_pos.y + 20
    ));
    ImGui::TextColored(TokenColorF(UITokens::TextPrimary, 178u), 
        "Axioms: %zu", s_axiom_tool ? s_axiom_tool->axioms().size() : size_t{0});
    
    // Render generated city output (roads)
    if (gs.show_layer_roads && s_generation_coordinator && s_generation_coordinator->GetOutput()) {
        const auto& roads = s_generation_coordinator->GetOutput()->roads;
        
        // Iterate using const iterators
        for (auto it = roads.begin(); it != roads.end(); ++it) {
            const auto& road = *it;
            
            if (road.points.empty()) continue;
            
            // Convert road points to screen space and draw polyline
            ImVec2 prev_screen = s_primary_viewport->world_to_screen(road.points[0]);
            
            for (size_t j = 1; j < road.points.size(); ++j) {
                ImVec2 curr_screen = s_primary_viewport->world_to_screen(road.points[j]);

                // Hierarchy-aware road styling (additive visual change only).
                const ViewportRoadStyle road_style = ResolveViewportRoadStyle(road.type);
                draw_list->AddLine(prev_screen, curr_screen, road_style.color, road_style.width);
                prev_screen = curr_screen;
            }
        }
        
        // Road count display
        ImGui::SetCursorScreenPos(ImVec2(
            viewport_pos.x + viewport_size.x - 120,
            viewport_pos.y + 45
        ));
        ImGui::TextColored(TokenColorF(UITokens::CyanAccent, 178u), 
            "Roads: %llu", roads.size());
    }

    // Editor-local validation overlay payload.
    gs.validation_overlay.errors = RogueCity::Core::Validation::CollectOverlayValidationErrors(gs);

    // Road vertex handles + spline preview in road mode.
    if (current_state == RogueCity::Core::Editor::EditorState::Editing_Roads && gs.selection.selected_road) {
        const uint32_t road_id = gs.selection.selected_road->id;
        if (RogueCity::Core::Road* road = FindRoadMutable(gs, road_id); road != nullptr) {
            for (size_t i = 0; i < road->points.size(); ++i) {
                const ImVec2 p = s_primary_viewport->world_to_screen(road->points[i]);
                const ImU32 color = (s_non_axiom_interaction.road_vertex_drag.active &&
                                     s_non_axiom_interaction.road_vertex_drag.vertex_index == i)
                    ? TokenColor(UITokens::YellowWarning)
                    : TokenColor(UITokens::CyanAccent, 225u);
                draw_list->AddCircleFilled(p, 4.0f, color, 12);
                draw_list->AddCircle(p, 6.0f, TokenColor(UITokens::BackgroundDark, 200u), 12, 1.0f);
            }

            if (gs.spline_editor.enabled && road->points.size() >= 3) {
                RogueCity::App::EditorManipulation::SplineOptions options{};
                options.closed = gs.spline_editor.closed;
                options.samples_per_segment = gs.spline_editor.samples_per_segment;
                options.tension = gs.spline_editor.tension;
                const auto smooth_points = RogueCity::App::EditorManipulation::BuildCatmullRomSpline(road->points, options);
                if (smooth_points.size() >= 2) {
                    std::vector<ImVec2> screen;
                    screen.reserve(smooth_points.size());
                    for (const auto& point : smooth_points) {
                        screen.push_back(s_primary_viewport->world_to_screen(point));
                    }
                    draw_list->AddPolyline(
                        screen.data(),
                        static_cast<int>(screen.size()),
                        TokenColor(UITokens::AmberGlow, 185u),
                        gs.spline_editor.closed,
                        2.0f);
                }
            }
        }
    }

    // District boundary handles in district mode.
    if (current_state == RogueCity::Core::Editor::EditorState::Editing_Districts &&
        gs.selection.selected_district &&
        (gs.district_boundary_editor.enabled ||
         gs.tool_runtime.district_subtool == RogueCity::Core::Editor::DistrictSubtool::Select ||
         gs.tool_runtime.district_subtool == RogueCity::Core::Editor::DistrictSubtool::Paint ||
         gs.tool_runtime.district_subtool == RogueCity::Core::Editor::DistrictSubtool::Zone ||
         gs.tool_runtime.district_subtool == RogueCity::Core::Editor::DistrictSubtool::Split ||
         gs.tool_runtime.district_subtool == RogueCity::Core::Editor::DistrictSubtool::Merge)) {
        const uint32_t district_id = gs.selection.selected_district->id;
        if (RogueCity::Core::District* district = FindDistrictMutable(gs, district_id); district != nullptr) {
            for (size_t i = 0; i < district->border.size(); ++i) {
                const ImVec2 p = s_primary_viewport->world_to_screen(district->border[i]);
                const ImU32 color = (s_non_axiom_interaction.district_boundary_drag.active &&
                                     s_non_axiom_interaction.district_boundary_drag.vertex_index == i)
                    ? TokenColor(UITokens::ErrorRed)
                    : TokenColor(UITokens::GreenHUD, 235u);
                draw_list->AddRectFilled(ImVec2(p.x - 4.0f, p.y - 4.0f), ImVec2(p.x + 4.0f, p.y + 4.0f), color);
                draw_list->AddRect(
                    ImVec2(p.x - 6.0f, p.y - 6.0f),
                    ImVec2(p.x + 6.0f, p.y + 6.0f),
                    TokenColor(UITokens::BackgroundDark, 190u),
                    0.0f, 0, 1.0f);
            }
        }
    }

    // Water vertex handles in water mode.
    if (current_state == RogueCity::Core::Editor::EditorState::Editing_Water) {
        const auto* primary = gs.selection_manager.Primary();
        const bool has_selected_water =
            primary != nullptr && primary->kind == RogueCity::Core::Editor::VpEntityKind::Water;
        if (has_selected_water) {
            if (RogueCity::Core::WaterBody* water = FindWaterMutable(gs, primary->id);
                water != nullptr && !water->boundary.empty()) {
                for (size_t i = 0; i < water->boundary.size(); ++i) {
                    const ImVec2 p = s_primary_viewport->world_to_screen(water->boundary[i]);
                    const ImU32 color = (s_non_axiom_interaction.water_vertex_drag.active &&
                                         s_non_axiom_interaction.water_vertex_drag.vertex_index == i)
                        ? TokenColor(UITokens::YellowWarning)
                        : TokenColor(UITokens::InfoBlue, 220u);
                    draw_list->AddCircleFilled(p, 4.0f, color, 12);
                    draw_list->AddCircle(p, 6.0f, TokenColor(UITokens::BackgroundDark, 180u), 12, 1.0f);
                }
            }
        }
    }

    auto& overlays = RC_UI::Viewport::GetViewportOverlays();
    RC_UI::Viewport::ViewportOverlays::ViewTransform transform{};
    transform.camera_xy = s_primary_viewport->get_camera_xy();
    transform.zoom = s_primary_viewport->world_to_screen_scale(1.0f);
    transform.yaw = s_primary_viewport->get_camera_yaw();
    transform.viewport_pos = viewport_pos;
    transform.viewport_size = viewport_size;
    overlays.SetViewTransform(transform);

    RC_UI::Viewport::OverlayConfig overlay_config{};
    overlay_config.show_zone_colors = gs.districts.size() != 0 && gs.show_layer_districts;
    overlay_config.show_road_labels = gs.roads.size() != 0 && gs.show_layer_roads;
    overlay_config.show_lot_boundaries = gs.lots.size() != 0 && gs.show_layer_lots;  // Enable lot boundaries
    overlay_config.show_no_build_mask = gs.world_constraints.isValid();
    overlay_config.show_slope_heatmap =
        gs.world_constraints.isValid() &&
        (current_state == RogueCity::Core::Editor::EditorState::Editing_Axioms ||
         current_state == RogueCity::Core::Editor::EditorState::Editing_Roads);
    overlay_config.show_nature_heatmap =
        gs.world_constraints.isValid() &&
        (current_state == RogueCity::Core::Editor::EditorState::Editing_Districts ||
         current_state == RogueCity::Core::Editor::EditorState::Editing_Lots);
    overlay_config.show_height_field = gs.debug_show_height_overlay;
    overlay_config.show_tensor_field = gs.debug_show_tensor_overlay;
    overlay_config.show_zone_field = gs.debug_show_zone_overlay;
    
    // Layer visibility from GlobalState
    overlay_config.show_axioms = gs.show_layer_axioms;
    overlay_config.show_water_bodies = gs.show_layer_water;
    overlay_config.show_roads = gs.show_layer_roads;
    overlay_config.show_districts = gs.show_layer_districts;
    overlay_config.show_lots = gs.show_layer_lots;
    overlay_config.show_building_sites = gs.show_layer_buildings;
    
    overlay_config.show_aesp_heatmap =
        (current_state == RogueCity::Core::Editor::EditorState::Editing_Lots ||
         current_state == RogueCity::Core::Editor::EditorState::Editing_Buildings) &&
        gs.lots.size() != 0;
    overlay_config.show_budget_bars =
        current_state == RogueCity::Core::Editor::EditorState::Editing_Buildings &&
        gs.districts.size() != 0;
    overlay_config.show_validation_errors = gs.validation_overlay.enabled;
    overlay_config.show_gizmos = gs.gizmo.visible && gs.selection_manager.Count() > 0;
    overlays.Render(gs, overlay_config);

    if (s_non_axiom_interaction.selection_drag.box_active) {
        const ImVec2 a = s_primary_viewport->world_to_screen(s_non_axiom_interaction.selection_drag.box_start);
        const ImVec2 b = s_primary_viewport->world_to_screen(s_non_axiom_interaction.selection_drag.box_end);
        draw_list->AddRect(
            ImVec2(std::min(a.x, b.x), std::min(a.y, b.y)),
            ImVec2(std::max(a.x, b.x), std::max(a.y, b.y)),
            TokenColor(UITokens::CyanAccent, 220u),
            0.0f,
            0,
            2.0f);
    }

    if (s_non_axiom_interaction.selection_drag.lasso_active &&
        s_non_axiom_interaction.selection_drag.lasso_points.size() >= 2) {
        std::vector<ImVec2> lasso_screen;
        lasso_screen.reserve(s_non_axiom_interaction.selection_drag.lasso_points.size());
        for (const auto& p : s_non_axiom_interaction.selection_drag.lasso_points) {
            lasso_screen.push_back(s_primary_viewport->world_to_screen(p));
        }
        draw_list->AddPolyline(
            lasso_screen.data(),
            static_cast<int>(lasso_screen.size()),
            TokenColor(UITokens::CyanAccent, 220u),
            false,
            2.0f);
    }
    
    // === ROGUENAV MINIMAP OVERLAY ===
    // Render minimap as overlay in top-right corner
    RenderMinimapOverlay(draw_list, viewport_pos, viewport_size);
    
    // === MINIMAP INTERACTION (only if viewport window is hovered) ===
    const bool viewport_window_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    
    if (viewport_window_hovered && s_minimap_visible && !input_gate.imgui_wants_mouse) {
        const ImVec2 minimap_interact_pos = ImVec2(
            viewport_pos.x + viewport_size.x - kMinimapSize - kMinimapPadding,
            viewport_pos.y + kMinimapPadding
        );
        const ImVec2 minimap_interact_max = ImVec2(minimap_interact_pos.x + kMinimapSize, minimap_interact_pos.y + kMinimapSize);
        
        // Check if mouse is over minimap
        const ImVec2 mouse_screen = ImGui::GetMousePos();
        const bool minimap_overlay_hovered = (mouse_screen.x >= minimap_interact_pos.x && mouse_screen.x <= minimap_interact_max.x &&
                                              mouse_screen.y >= minimap_interact_pos.y && mouse_screen.y <= minimap_interact_max.y);
        
        if (minimap_overlay_hovered) {
            // Scroll to zoom
            float scroll = ImGui::GetIO().MouseWheel;
            if (scroll != 0.0f) {
                s_minimap_zoom *= (1.0f + scroll * 0.1f);
                s_minimap_zoom = RC_UI::Viewport::ClampMinimapZoom(s_minimap_zoom);
            }

            const ImGuiIO& io = ImGui::GetIO();
            if (allow_viewport_key_actions && ImGui::IsKeyPressed(ImGuiKey_L)) {
                if (io.KeyShift) {
                    // Release manual pin and return to auto LOD switching.
                    s_minimap_auto_lod = true;
                } else if (s_minimap_auto_lod) {
                    // First press enters manual mode and pins the current effective level.
                    s_minimap_auto_lod = false;
                    s_minimap_lod = s_minimap_effective_lod;
                } else {
                    CycleManualMinimapLOD();
                }
            }
            if (allow_viewport_key_actions && ImGui::IsKeyPressed(ImGuiKey_1)) {
                s_minimap_auto_lod = false;
                s_minimap_lod = MinimapLODFromLevel(0);
            }
            if (allow_viewport_key_actions && ImGui::IsKeyPressed(ImGuiKey_2)) {
                s_minimap_auto_lod = false;
                s_minimap_lod = MinimapLODFromLevel(1);
            }
            if (allow_viewport_key_actions && ImGui::IsKeyPressed(ImGuiKey_3)) {
                s_minimap_auto_lod = false;
                s_minimap_lod = MinimapLODFromLevel(2);
            }
            if (allow_viewport_key_actions && ImGui::IsKeyPressed(ImGuiKey_K)) {
                s_minimap_adaptive_quality = !s_minimap_adaptive_quality;
            }
             
            const bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f);
            if (dragging) {
                const auto camera_pos = s_primary_viewport->get_camera_xy();
                const auto camera_z = s_primary_viewport->get_camera_z();
                const ImVec2 delta = io.MouseDelta;
                const double world_per_pixel = RC_UI::Viewport::ComputeMinimapWorldPerPixel(
                    kMinimapWorldSize,
                    s_minimap_zoom,
                    kMinimapSize);
                RogueCity::Core::Vec2 next_camera = camera_pos;
                next_camera.x -= static_cast<double>(delta.x) * world_per_pixel;
                next_camera.y -= static_cast<double>(delta.y) * world_per_pixel;
                next_camera = ClampMinimapCameraToConstraints(next_camera, gs);
                s_primary_viewport->set_camera_position(next_camera, camera_z);
            } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // Click to jump camera
                const auto camera_pos = s_primary_viewport->get_camera_xy();
                const auto camera_z = s_primary_viewport->get_camera_z();
                auto world_pos = MinimapPixelToWorld(mouse_screen, minimap_interact_pos, camera_pos);
                world_pos = ClampMinimapCameraToConstraints(world_pos, gs);
                s_primary_viewport->set_camera_position(world_pos, camera_z);
            }
        }
    }
    
    // Minimap toggle hotkey (M key) - only when viewport has focus
    if (viewport_window_hovered && allow_viewport_key_actions && ImGui::IsKeyPressed(ImGuiKey_M)) {
        ToggleMinimapVisible();
    }
    
    // Validation errors are shown in the Tools strip (and can still be overlayed here if desired).
    draw_list->PopClipRect();

    const ToolLibrary preferred_library = RC_UI::Commands::CommandLibraryForDomain(gs.tool_runtime.active_domain);
    RC_UI::Commands::DrawSmartMenu(s_smart_command_menu, command_dispatch);
    RC_UI::Commands::DrawPieMenu(s_pie_command_menu, preferred_library, command_dispatch);
    RC_UI::Commands::DrawCommandPalette(s_command_palette, command_dispatch);

}

void Draw(float dt) {
    static RC_UI::DockableWindowState s_viewport_window;
    if (!RC_UI::BeginDockableWindow("RogueVisualizer", s_viewport_window, "Center",
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse)) {
        return;
    }

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "RogueVisualizer",
            "RogueVisualizer",
            "viewport",
            "Center",
            "visualizer/src/ui/panels/rc_panel_axiom_editor.cpp",
            {"viewport", "axiom", "minimap"}
        },
        true
    );
    uiint.RegisterWidget({"viewport", "Primary Viewport", "viewport.primary", {"viewport"}});
    
    DrawContent(dt);
    
    uiint.EndPanel();
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::AxiomEditor
