// FILE: AxiomEditorPanel.cpp - Integrated axiom editor with viewport
// Replaces SystemMap stub with full axiom placement functionality

#include "ui/panels/rc_panel_axiom_editor.h"
#include "ui/rc_ui_root.h"  // NEW: Access to shared minimap
#include "ui/rc_ui_viewport_config.h"
#include "RogueCity/App/UI/DesignSystem.h"  // Cockpit Doctrine enforcement
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/App/Viewports/MinimapViewport.hpp"
#include "RogueCity/App/Viewports/ViewportSyncManager.hpp"
#include "RogueCity/App/Tools/AxiomPlacementTool.hpp"
#include "RogueCity/App/Integration/GeneratorBridge.hpp"
#include "RogueCity/App/Integration/RealTimePreview.hpp"
#include "RogueCity/App/Editor/EditorManipulation.hpp"
#include "RogueCity/App/Editor/ViewportIndexBuilder.hpp"
#include "RogueCity/App/Tools/AxiomIcon.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Core/Data/MaterialEncoding.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/SelectionSync.hpp"
#include "RogueCity/Core/Validation/EditorOverlayValidation.hpp"
#include "ui/viewport/rc_viewport_overlays.h"
#include "ui/introspection/UiIntrospection.h"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <limits>
#include <numbers>
#include <unordered_set>
#include <string>

namespace RC_UI::Panels::AxiomEditor {

using namespace RogueCity::UI;  // For DesignSystem helpers

namespace {
    using Preview = RogueCity::App::RealTimePreview;
    static bool s_viewport_open = true;
    
    // === ROGUENAV MINIMAP CONFIGURATION ===
    enum class MinimapMode {
        Disabled,
        Soliton,      // Render-to-texture (recommended, high performance)
        Reactive,     // Dual viewport (heavier, real-time updates)
        Satellite     // Future: Satellite-style view (stub)
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
    static bool s_minimap_adaptive_quality = true;

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

    void SyncGlobalStateFromPreview(const RogueCity::Generators::CityGenerator::CityOutput& output) {
        auto& gs = RogueCity::Core::Editor::GetGlobalState();
        auto point_in_polygon = [](const RogueCity::Core::Vec2& point, const std::vector<RogueCity::Core::Vec2>& polygon) {
            if (polygon.size() < 3) {
                return false;
            }

            bool inside = false;
            for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
                const auto& a = polygon[i];
                const auto& b = polygon[j];
                const bool intersects = ((a.y > point.y) != (b.y > point.y)) &&
                    (point.x < (b.x - a.x) * (point.y - a.y) / ((b.y - a.y) + 1e-12) + a.x);
                if (intersects) {
                    inside = !inside;
                }
            }
            return inside;
        };

        gs.roads.clear();
        for (const auto& road : output.roads) {
            gs.roads.add(road);
        }

        gs.districts.clear();
        for (const auto& district : output.districts) {
            gs.districts.add(district);
        }

        gs.blocks.clear();
        for (const auto& block : output.blocks) {
            gs.blocks.add(block);
        }

        gs.lots.clear();
        for (const auto& lot : output.lots) {
            gs.lots.add(lot);
        }

        gs.buildings.clear();
        for (size_t i = 0; i < output.buildings.size(); ++i) {
            gs.buildings.push_back(output.buildings[i]);
        }

        gs.generation_stats.roads_generated = static_cast<uint32_t>(gs.roads.size());
        gs.generation_stats.districts_generated = static_cast<uint32_t>(gs.districts.size());
        gs.generation_stats.lots_generated = static_cast<uint32_t>(gs.lots.size());
        gs.generation_stats.buildings_generated = static_cast<uint32_t>(gs.buildings.size());
        gs.world_constraints = output.world_constraints;
        gs.site_profile = output.site_profile;
        gs.plan_violations = output.plan_violations;
        gs.plan_approved = output.plan_approved;
        RogueCity::Core::Bounds world_bounds{};
        world_bounds.min = RogueCity::Core::Vec2(0.0, 0.0);
        if (output.world_constraints.isValid()) {
            world_bounds.max = RogueCity::Core::Vec2(
                static_cast<double>(output.world_constraints.width) * output.world_constraints.cell_size,
                static_cast<double>(output.world_constraints.height) * output.world_constraints.cell_size);
        } else {
            world_bounds.max = RogueCity::Core::Vec2(
                static_cast<double>(output.tensor_field.getWidth()) * output.tensor_field.getCellSize(),
                static_cast<double>(output.tensor_field.getHeight()) * output.tensor_field.getCellSize());
        }
        const int resolution = output.world_constraints.isValid()
            ? std::max(output.world_constraints.width, output.world_constraints.height)
            : std::max(output.tensor_field.getWidth(), output.tensor_field.getHeight());
        gs.InitializeTextureSpace(world_bounds, std::max(1, resolution));
        if (gs.HasTextureSpace()) {
            auto& texture_space = gs.TextureSpaceRef();
            if (output.world_constraints.isValid()) {
                auto& height_layer = texture_space.heightLayer();
                auto& material_layer = texture_space.materialLayer();
                auto& distance_layer = texture_space.distanceLayer();
                const auto& coords = texture_space.coordinateSystem();
                for (int y = 0; y < height_layer.height(); ++y) {
                    for (int x = 0; x < height_layer.width(); ++x) {
                        const RogueCity::Core::Vec2 world = coords.pixelToWorld({ x, y });
                        height_layer.at(x, y) = output.world_constraints.sampleHeightMeters(world);
                        material_layer.at(x, y) = RogueCity::Core::Data::EncodeMaterialSample(
                            output.world_constraints.sampleFloodMask(world),
                            output.world_constraints.sampleNoBuild(world));
                        distance_layer.at(x, y) = output.world_constraints.sampleSlopeDegrees(world);
                    }
                }
                gs.ClearTextureLayerDirty(RogueCity::Core::Data::TextureLayer::Height);
                gs.ClearTextureLayerDirty(RogueCity::Core::Data::TextureLayer::Material);
                gs.ClearTextureLayerDirty(RogueCity::Core::Data::TextureLayer::Distance);
            }
            output.tensor_field.writeToTextureSpace(texture_space);
            gs.ClearTextureLayerDirty(RogueCity::Core::Data::TextureLayer::Tensor);
            auto& zone_layer = texture_space.zoneLayer();
            zone_layer.fill(0u);
            const auto& coords = texture_space.coordinateSystem();
            for (const auto& district : output.districts) {
                if (district.border.size() < 3) {
                    continue;
                }

                RogueCity::Core::Bounds district_bounds{};
                district_bounds.min = district.border.front();
                district_bounds.max = district.border.front();
                for (const auto& p : district.border) {
                    district_bounds.min.x = std::min(district_bounds.min.x, p.x);
                    district_bounds.min.y = std::min(district_bounds.min.y, p.y);
                    district_bounds.max.x = std::max(district_bounds.max.x, p.x);
                    district_bounds.max.y = std::max(district_bounds.max.y, p.y);
                }

                const auto pmin = coords.worldToPixel(district_bounds.min);
                const auto pmax = coords.worldToPixel(district_bounds.max);
                const int x0 = std::clamp(std::min(pmin.x, pmax.x), 0, zone_layer.width() - 1);
                const int x1 = std::clamp(std::max(pmin.x, pmax.x), 0, zone_layer.width() - 1);
                const int y0 = std::clamp(std::min(pmin.y, pmax.y), 0, zone_layer.height() - 1);
                const int y1 = std::clamp(std::max(pmin.y, pmax.y), 0, zone_layer.height() - 1);
                const uint8_t zone_value = static_cast<uint8_t>(static_cast<uint8_t>(district.type) + 1u);

                for (int y = y0; y <= y1; ++y) {
                    for (int x = x0; x <= x1; ++x) {
                        const RogueCity::Core::Vec2 world = coords.pixelToWorld({ x, y });
                        if (point_in_polygon(world, district.border)) {
                            zone_layer.at(x, y) = zone_value;
                        }
                    }
                }
            }
            gs.ClearTextureLayerDirty(RogueCity::Core::Data::TextureLayer::Zone);
        }
        gs.entity_layers.clear();
        RogueCity::App::ViewportIndexBuilder::Build(gs);
        gs.dirty_layers.MarkAllClean();
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
static float s_flow_rate = 1.0f;
static uint32_t s_seed = 42u;
static std::string s_validation_error;
static bool s_external_dirty = false;

struct SelectionDragState {
    bool lasso_active{ false };
    bool box_active{ false };
    RogueCity::Core::Vec2 box_start{};
    RogueCity::Core::Vec2 box_end{};
    std::vector<RogueCity::Core::Vec2> lasso_points{};
};

static SelectionDragState s_selection_drag{};

struct GizmoDragState {
    bool active{ false };
    RogueCity::Core::Vec2 pivot{};
    RogueCity::Core::Vec2 previous_world{};
};

struct RoadVertexDragState {
    bool active{ false };
    uint32_t road_id{ 0 };
    size_t vertex_index{ 0 };
};

struct DistrictBoundaryDragState {
    bool active{ false };
    uint32_t district_id{ 0 };
    size_t vertex_index{ 0 };
};

static GizmoDragState s_gizmo_drag{};
static RoadVertexDragState s_road_vertex_drag{};
static DistrictBoundaryDragState s_district_boundary_drag{};

double DistanceToSegment(const RogueCity::Core::Vec2& p, const RogueCity::Core::Vec2& a, const RogueCity::Core::Vec2& b) {
    const RogueCity::Core::Vec2 ab = b - a;
    const double ab_len_sq = ab.lengthSquared();
    if (ab_len_sq <= 1e-9) {
        return p.distanceTo(a);
    }
    const double t = std::clamp((p - a).dot(ab) / ab_len_sq, 0.0, 1.0);
    const RogueCity::Core::Vec2 proj = a + ab * t;
    return p.distanceTo(proj);
}

bool PointInPolygon(const RogueCity::Core::Vec2& point, const std::vector<RogueCity::Core::Vec2>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }

    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const auto& pi = polygon[i];
        const auto& pj = polygon[j];
        const bool intersect = ((pi.y > point.y) != (pj.y > point.y)) &&
            (point.x < (pj.x - pi.x) * (point.y - pi.y) / ((pj.y - pi.y) + 1e-12) + pi.x);
        if (intersect) {
            inside = !inside;
        }
    }
    return inside;
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

bool IsSelectableKind(RogueCity::Core::Editor::VpEntityKind kind) {
    using RogueCity::Core::Editor::VpEntityKind;
    return kind == VpEntityKind::Road ||
        kind == VpEntityKind::District ||
        kind == VpEntityKind::Lot ||
        kind == VpEntityKind::Building;
}

bool ProbeContainsPoint(
    const RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::VpEntityKind kind,
    uint32_t id,
    const RogueCity::Core::Vec2& world_pos,
    double world_radius,
    int& out_priority,
    double& out_distance) {
    using RogueCity::Core::Editor::VpEntityKind;

    out_priority = 0;
    out_distance = std::numeric_limits<double>::max();

    if (!IsSelectableKind(kind)) {
        return false;
    }

    if (kind == VpEntityKind::Road) {
        for (const auto& road : gs.roads) {
            if (road.id != id || road.points.size() < 2) {
                continue;
            }
            double min_distance = std::numeric_limits<double>::max();
            for (size_t i = 1; i < road.points.size(); ++i) {
                min_distance = std::min(min_distance, DistanceToSegment(world_pos, road.points[i - 1], road.points[i]));
            }
            if (min_distance <= world_radius * 1.25) {
                out_priority = 3;
                out_distance = min_distance;
                return true;
            }
            return false;
        }
        return false;
    }

    if (kind == VpEntityKind::District) {
        for (const auto& district : gs.districts) {
            if (district.id != id || district.border.empty()) {
                continue;
            }
            const bool inside = PointInPolygon(world_pos, district.border);
            if (!inside) {
                return false;
            }
            out_priority = 2;
            out_distance = world_pos.distanceTo(PolygonCentroid(district.border));
            return true;
        }
        return false;
    }

    if (kind == VpEntityKind::Lot) {
        for (const auto& lot : gs.lots) {
            if (lot.id != id) {
                continue;
            }
            if (!lot.boundary.empty() && PointInPolygon(world_pos, lot.boundary)) {
                out_priority = 4;
                out_distance = world_pos.distanceTo(lot.centroid);
                return true;
            }
            const double d = world_pos.distanceTo(lot.centroid);
            if (d <= world_radius * 2.0) {
                out_priority = 2;
                out_distance = d;
                return true;
            }
            return false;
        }
        return false;
    }

    for (const auto& building : gs.buildings) {
        if (building.id != id) {
            continue;
        }
        const double d = world_pos.distanceTo(building.position);
        if (d <= world_radius * 1.75) {
            out_priority = 5;
            out_distance = d;
            return true;
        }
        return false;
    }
    return false;
}

std::optional<RogueCity::Core::Editor::SelectionItem> PickFromViewportIndex(
    const RogueCity::Core::Editor::GlobalState& gs,
    const RogueCity::Core::Vec2& world_pos,
    float zoom) {
    const double world_radius = std::max(4.0, 12.0 / std::max(0.1f, zoom));
    int best_priority = -1;
    double best_distance = std::numeric_limits<double>::max();
    std::optional<RogueCity::Core::Editor::SelectionItem> best{};

    for (const auto& probe : gs.viewport_index) {
        if (!IsSelectableKind(probe.kind)) {
            continue;
        }
        if (!gs.IsEntityVisible(probe.kind, probe.id)) {
            continue;
        }
        int priority = 0;
        double distance = 0.0;
        if (!ProbeContainsPoint(gs, probe.kind, probe.id, world_pos, world_radius, priority, distance)) {
            continue;
        }
        if (priority > best_priority || (priority == best_priority && distance < best_distance)) {
            best_priority = priority;
            best_distance = distance;
            best = RogueCity::Core::Editor::SelectionItem{ probe.kind, probe.id };
        }
    }

    return best;
}

std::vector<RogueCity::Core::Editor::SelectionItem> QueryRegionFromViewportIndex(
    const RogueCity::Core::Editor::GlobalState& gs,
    const std::function<bool(const RogueCity::Core::Vec2&)>& include_point,
    bool include_hidden = false) {
    std::vector<RogueCity::Core::Editor::SelectionItem> results;
    std::unordered_set<uint64_t> dedupe;
    results.reserve(gs.viewport_index.size());

    for (const auto& probe : gs.viewport_index) {
        if (!IsSelectableKind(probe.kind)) {
            continue;
        }
        if (!include_hidden && !gs.IsEntityVisible(probe.kind, probe.id)) {
            continue;
        }

        RogueCity::Core::Vec2 anchor{};
        if (!ResolveSelectionAnchor(gs, probe.kind, probe.id, anchor)) {
            continue;
        }
        if (!include_point(anchor)) {
            continue;
        }

        const uint64_t key = (static_cast<uint64_t>(static_cast<uint8_t>(probe.kind)) << 32) | probe.id;
        if (!dedupe.insert(key).second) {
            continue;
        }
        results.push_back({ probe.kind, probe.id });
    }

    return results;
}

RogueCity::Core::Vec2 ComputeSelectionPivot(const RogueCity::Core::Editor::GlobalState& gs) {
    RogueCity::Core::Vec2 pivot{};
    size_t count = 0;
    for (const auto& item : gs.selection_manager.Items()) {
        RogueCity::Core::Vec2 anchor{};
        if (!ResolveSelectionAnchor(gs, item.kind, item.id, anchor)) {
            continue;
        }
        pivot += anchor;
        ++count;
    }
    if (count == 0) {
        return pivot;
    }
    pivot /= static_cast<double>(count);
    return pivot;
}

void MarkDirtyForSelectionKind(
    RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::VpEntityKind kind) {
    using RogueCity::Core::Editor::DirtyLayer;
    using RogueCity::Core::Editor::VpEntityKind;
    switch (kind) {
    case VpEntityKind::Road:
        gs.dirty_layers.MarkDirty(DirtyLayer::Roads);
        gs.dirty_layers.MarkDirty(DirtyLayer::Districts);
        gs.dirty_layers.MarkDirty(DirtyLayer::Lots);
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        break;
    case VpEntityKind::District:
        gs.dirty_layers.MarkDirty(DirtyLayer::Districts);
        gs.dirty_layers.MarkDirty(DirtyLayer::Lots);
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        break;
    case VpEntityKind::Lot:
        gs.dirty_layers.MarkDirty(DirtyLayer::Lots);
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        break;
    case VpEntityKind::Building:
        gs.dirty_layers.MarkDirty(DirtyLayer::Buildings);
        break;
    default:
        break;
    }
    gs.dirty_layers.MarkDirty(DirtyLayer::ViewportIndex);
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
        SyncGlobalStateFromPreview(output);
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
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    gs.dirty_layers.MarkAllClean();
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

bool ApplyGeneratorRequest(
    const std::vector<RogueCity::Generators::CityGenerator::AxiomInput>& axioms,
    const RogueCity::Generators::CityGenerator::Config& config,
    std::string* outError) {
    Initialize();
    if (!s_preview) {
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
    s_preview->force_regeneration(axioms, config);
    s_external_dirty = false;
    RogueCity::Core::Editor::GetGlobalState().dirty_layers.MarkAllClean();

    return true;
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

static MinimapLOD ComputeAutoMinimapLOD(float zoom) {
    if (zoom <= 0.85f) {
        return MinimapLOD::Strategic;
    }
    if (zoom <= 1.6f) {
        return MinimapLOD::Tactical;
    }
    return MinimapLOD::Detail;
}

static MinimapLOD ActiveMinimapLOD() {
    return s_minimap_auto_lod ? ComputeAutoMinimapLOD(s_minimap_zoom) : s_minimap_lod;
}

static MinimapLOD CoarsenMinimapLOD(MinimapLOD lod) {
    switch (lod) {
        case MinimapLOD::Detail: return MinimapLOD::Tactical;
        case MinimapLOD::Tactical: return MinimapLOD::Strategic;
        case MinimapLOD::Strategic:
        default: return MinimapLOD::Strategic;
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
    if (!s_minimap_adaptive_quality) {
        return base_lod;
    }

    MinimapLOD effective = base_lod;
    const float fps = ImGui::GetIO().Framerate;

    // Phase 7 degradation policy: FPS pressure first, then dirty pressure.
    if (fps < 50.0f) {
        effective = CoarsenMinimapLOD(effective);
    }
    if (fps < 40.0f) {
        effective = MinimapLOD::Strategic;
    }

    const int dirty_texture_layers = CountDirtyTextureLayers(gs);
    if (dirty_texture_layers >= 3 || gs.dirty_layers.AnyDirty()) {
        effective = CoarsenMinimapLOD(effective);
    }

    return effective;
}

static const char* MinimapLODName(MinimapLOD lod) {
    switch (lod) {
        case MinimapLOD::Strategic: return "LOD2";
        case MinimapLOD::Tactical: return "LOD1";
        case MinimapLOD::Detail: return "LOD0";
        default: return "LOD1";
    }
}

static void CycleManualMinimapLOD() {
    switch (s_minimap_lod) {
        case MinimapLOD::Strategic:
            s_minimap_lod = MinimapLOD::Tactical;
            break;
        case MinimapLOD::Tactical:
            s_minimap_lod = MinimapLOD::Detail;
            break;
        case MinimapLOD::Detail:
            s_minimap_lod = MinimapLOD::Strategic;
            break;
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

    const auto& gs = RogueCity::Core::Editor::GetGlobalState();
    const MinimapLOD base_lod = ActiveMinimapLOD();
    const MinimapLOD active_lod = ComputeAdaptiveMinimapLOD(base_lod, gs);
    ImGui::SetCursorScreenPos(ImVec2(minimap_pos.x + 8.0f, minimap_pos.y + 24.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, DesignTokens::TextSecondary);
    ImGui::Text(
        "%s %s%s",
        MinimapLODName(active_lod),
        s_minimap_auto_lod ? "AUTO" : "MAN",
        (active_lod != base_lod) ? " ADAPT" : "");
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
    
    // Get camera position for world-to-minimap conversion
    const auto camera_pos = s_primary_viewport ? s_primary_viewport->get_camera_xy() : RogueCity::Core::Vec2(1000.0f, 1000.0f);
    const auto* output = (s_preview && s_preview->get_output()) ? s_preview->get_output() : nullptr;
    
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
    
    if (output != nullptr) {
        if (active_lod == MinimapLOD::Strategic) {
            auto districtColor = [](RogueCity::Core::DistrictType type) {
                switch (type) {
                    case RogueCity::Core::DistrictType::Residential: return IM_COL32(80, 210, 120, 170);
                    case RogueCity::Core::DistrictType::Commercial: return IM_COL32(120, 170, 255, 170);
                    case RogueCity::Core::DistrictType::Industrial: return IM_COL32(225, 170, 80, 170);
                    case RogueCity::Core::DistrictType::Civic: return IM_COL32(210, 120, 210, 170);
                    case RogueCity::Core::DistrictType::Mixed:
                    default: return IM_COL32(190, 190, 190, 170);
                }
            };
            for (const auto& district : output->districts) {
                if (district.border.empty()) {
                    continue;
                }
                RogueCity::Core::Vec2 centroid{};
                for (const auto& p : district.border) {
                    centroid += p;
                }
                centroid /= static_cast<double>(district.border.size());
                const ImVec2 uv = WorldToMinimapUV(centroid, camera_pos);
                if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
                    continue;
                }
                const ImVec2 screen_pos(
                    minimap_pos.x + uv.x * kMinimapSize,
                    minimap_pos.y + uv.y * kMinimapSize);
                draw_list->AddCircleFilled(screen_pos, 3.5f, districtColor(district.type));
            }
        }

        const auto& roads = output->roads;
        const size_t sample_step = (active_lod == MinimapLOD::Detail) ? 1u : ((active_lod == MinimapLOD::Tactical) ? 4u : 10u);
        const float road_width = (active_lod == MinimapLOD::Detail) ? 1.25f : 1.0f;
        const ImU32 road_color = (active_lod == MinimapLOD::Strategic)
            ? IM_COL32(90, 180, 200, 150)
            : DesignTokens::CyanAccent;

        for (auto it = roads.begin(); it != roads.end(); ++it) {
            const auto& road = *it;
            if (road.points.size() < 2) {
                continue;
            }

            for (size_t i = 0; i + 1 < road.points.size(); i += sample_step) {
                const size_t next_i = std::min(i + sample_step, road.points.size() - 1);
                ImVec2 uv1 = WorldToMinimapUV(road.points[i], camera_pos);
                ImVec2 uv2 = WorldToMinimapUV(road.points[next_i], camera_pos);

                if ((uv1.x >= 0.0f && uv1.x <= 1.0f && uv1.y >= 0.0f && uv1.y <= 1.0f) ||
                    (uv2.x >= 0.0f && uv2.x <= 1.0f && uv2.y >= 0.0f && uv2.y <= 1.0f)) {
                    ImVec2 screen1(
                        minimap_pos.x + uv1.x * kMinimapSize,
                        minimap_pos.y + uv1.y * kMinimapSize);
                    ImVec2 screen2(
                        minimap_pos.x + uv2.x * kMinimapSize,
                        minimap_pos.y + uv2.y * kMinimapSize);

                    draw_list->AddLine(screen1, screen2, road_color, road_width);
                }
            }
        }

        if (active_lod == MinimapLOD::Detail) {
            for (const auto& building : output->buildings) {
                const ImVec2 uv = WorldToMinimapUV(building.position, camera_pos);
                if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
                    continue;
                }
                const ImVec2 screen_pos(
                    minimap_pos.x + uv.x * kMinimapSize,
                    minimap_pos.y + uv.y * kMinimapSize);
                draw_list->AddRectFilled(
                    ImVec2(screen_pos.x - 1.5f, screen_pos.y - 1.5f),
                    ImVec2(screen_pos.x + 1.5f, screen_pos.y + 1.5f),
                    IM_COL32(245, 245, 245, 220));
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

    // Never-blank anchors: world frame + cursor/world readout.
    draw_list->AddRect(
        ImVec2(minimap_pos.x + 6.0f, minimap_pos.y + 6.0f),
        ImVec2(minimap_pos.x + kMinimapSize - 6.0f, minimap_pos.y + kMinimapSize - 6.0f),
        IM_COL32(255, 255, 255, 40),
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
    ImGui::PushStyleColor(ImGuiCol_Text, DesignTokens::TextSecondary);
    ImGui::Text(
        "CUR %.0f, %.0f",
        static_cast<float>(cursor_world.x),
        static_cast<float>(cursor_world.y));
    ImGui::PopStyleColor();
}

void Draw(float dt) {
 Initialize();
    
    RC_UI::ScopedViewportPadding viewport_padding;
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

    // Get editor state to determine if axiom tool should be active
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
    auto current_state = hfsm.state();

    const bool axiom_mode =
        (current_state == RogueCity::Core::Editor::EditorState::Editing_Axioms) ||
        (current_state == RogueCity::Core::Editor::EditorState::Viewport_PlaceAxiom);
    
    // Update viewport sync + preview scheduler
    s_sync_manager->update(dt);
    if (s_preview) {
        s_preview->update(dt);
    }
    
    // Render primary viewport with axiom tool integration
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 viewport_pos = ImGui::GetCursorScreenPos();
    const ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    const ImVec2 viewport_max(viewport_pos.x + viewport_size.x, viewport_pos.y + viewport_size.y);
    draw_list->PushClipRect(viewport_pos, viewport_max, true);
    
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
        const bool open = ImGui::Begin("Axiom Library", nullptr, ImGuiWindowFlags_NoCollapse);

        uiint.BeginPanel(
            RogueCity::UIInt::PanelMeta{
                "Axiom Library",
                "Axiom Library",
                "toolbox",
                "Library",
                "visualizer/src/ui/panels/rc_panel_axiom_editor.cpp",
                {"axiom", "library"}
            },
            open
        );

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

        uiint.RegisterWidget({"table", "Axiom Types", "axiom.types[]", {"axiom", "library"}});
        uiint.EndPanel();
        ImGui::End();
    }

    // ---------------------------------------------------------------------
    // Viewport mouse interaction (placement + manipulation)
    // ---------------------------------------------------------------------
    const ImVec2 mouse_pos = ImGui::GetMousePos();
    const ImVec2 vp_min = viewport_pos;
    const ImVec2 vp_max(viewport_pos.x + viewport_size.x, viewport_pos.y + viewport_size.y);
    const bool in_viewport = ImGui::IsMouseHoveringRect(vp_min, vp_max);
    const bool editor_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    const ImVec2 minimap_pos_bounds = ImVec2(
        viewport_pos.x + viewport_size.x - kMinimapSize - kMinimapPadding,
        viewport_pos.y + kMinimapPadding
    );
    const ImVec2 minimap_max_bounds = ImVec2(minimap_pos_bounds.x + kMinimapSize, minimap_pos_bounds.y + kMinimapSize);
    const bool minimap_hovered = (mouse_pos.x >= minimap_pos_bounds.x && mouse_pos.x <= minimap_max_bounds.x &&
                                   mouse_pos.y >= minimap_pos_bounds.y && mouse_pos.y <= minimap_max_bounds.y);

    if (axiom_mode && editor_hovered && in_viewport && !ImGui::IsAnyItemActive() && !minimap_hovered) {
        ImGuiIO& io = ImGui::GetIO();
        RogueCity::Core::Vec2 world_pos = s_primary_viewport->screen_to_world(mouse_pos);

        // CAD-grade viewport controls (2D/2.5D)
        const bool orbit = io.KeyAlt && io.MouseDown[ImGuiMouseButton_Left];
        const bool pan = io.MouseDown[ImGuiMouseButton_Middle] || (io.KeyShift && io.MouseDown[ImGuiMouseButton_Right]);
        const bool zoom_drag = io.KeyCtrl && io.MouseDown[ImGuiMouseButton_Right];
        const bool nav_active = orbit || pan || zoom_drag;

        if (!io.WantTextInput) {
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
                if (io.KeyShift) {
                    Redo();
                } else {
                    Undo();
                }
            } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
                Redo();
            }
        }

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

    // ---------------------------------------------------------------------
    // Viewport selection interaction (index-backed picking + region selects)
    // Alt+Drag: lasso select
    // Shift+Alt+Drag: box select through layers
    // ---------------------------------------------------------------------
    if (!axiom_mode && editor_hovered && in_viewport && !ImGui::IsAnyItemActive() && !minimap_hovered) {
        ImGuiIO& io = ImGui::GetIO();
        const RogueCity::Core::Vec2 world_pos = s_primary_viewport->screen_to_world(mouse_pos);
        const float zoom = s_primary_viewport->world_to_screen_scale(1.0f);

        if (!io.WantTextInput) {
            if (ImGui::IsKeyPressed(ImGuiKey_G)) {
                gs.gizmo.enabled = true;
                gs.gizmo.operation = RogueCity::Core::Editor::GizmoOperation::Translate;
            } else if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                gs.gizmo.enabled = true;
                gs.gizmo.operation = RogueCity::Core::Editor::GizmoOperation::Rotate;
            } else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                gs.gizmo.enabled = true;
                gs.gizmo.operation = RogueCity::Core::Editor::GizmoOperation::Scale;
            } else if (ImGui::IsKeyPressed(ImGuiKey_X)) {
                gs.gizmo.snapping = !gs.gizmo.snapping;
            } else if (ImGui::IsKeyPressed(ImGuiKey_1)) {
                gs.layer_manager.active_layer = 0u;
            } else if (ImGui::IsKeyPressed(ImGuiKey_2)) {
                gs.layer_manager.active_layer = 1u;
            } else if (ImGui::IsKeyPressed(ImGuiKey_3)) {
                gs.layer_manager.active_layer = 2u;
            }
        }

        bool consumed_interaction = false;

        // Gizmo interaction for selected entities.
        const auto& selected_items = gs.selection_manager.Items();
        if (gs.gizmo.enabled && !selected_items.empty()) {
            const RogueCity::Core::Vec2 pivot = ComputeSelectionPivot(gs);
            const double pick_radius = std::max(8.0, 14.0 / std::max(0.1f, zoom));
            const double dist_to_pivot = world_pos.distanceTo(pivot);

            auto can_start_drag = [&]() {
                using RogueCity::Core::Editor::GizmoOperation;
                switch (gs.gizmo.operation) {
                case GizmoOperation::Translate:
                    return dist_to_pivot <= pick_radius * 1.5;
                case GizmoOperation::Rotate:
                    return dist_to_pivot >= pick_radius * 1.2 && dist_to_pivot <= pick_radius * 3.0;
                case GizmoOperation::Scale:
                    return dist_to_pivot >= pick_radius * 1.0 && dist_to_pivot <= pick_radius * 2.2;
                default:
                    return false;
                }
            };

            if (!s_gizmo_drag.active &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !io.KeyAlt && !io.KeyShift && !io.KeyCtrl &&
                can_start_drag()) {
                s_gizmo_drag.active = true;
                s_gizmo_drag.pivot = pivot;
                s_gizmo_drag.previous_world = world_pos;
                consumed_interaction = true;
            }

            if (s_gizmo_drag.active) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    using RogueCity::App::EditorManipulation::ApplyRotate;
                    using RogueCity::App::EditorManipulation::ApplyScale;
                    using RogueCity::App::EditorManipulation::ApplyTranslate;
                    using RogueCity::Core::Editor::GizmoOperation;

                    bool changed = false;
                    if (gs.gizmo.operation == GizmoOperation::Translate) {
                        RogueCity::Core::Vec2 delta = world_pos - s_gizmo_drag.previous_world;
                        if (gs.gizmo.snapping && gs.gizmo.translate_snap > 0.0f) {
                            delta.x = std::round(delta.x / gs.gizmo.translate_snap) * gs.gizmo.translate_snap;
                            delta.y = std::round(delta.y / gs.gizmo.translate_snap) * gs.gizmo.translate_snap;
                        }
                        changed = ApplyTranslate(gs, selected_items, delta);
                    } else if (gs.gizmo.operation == GizmoOperation::Rotate) {
                        const RogueCity::Core::Vec2 prev = s_gizmo_drag.previous_world - s_gizmo_drag.pivot;
                        const RogueCity::Core::Vec2 curr = world_pos - s_gizmo_drag.pivot;
                        const double prev_angle = std::atan2(prev.y, prev.x);
                        const double curr_angle = std::atan2(curr.y, curr.x);
                        double delta_angle = curr_angle - prev_angle;
                        if (gs.gizmo.snapping && gs.gizmo.rotate_snap_degrees > 0.0f) {
                            const double step = gs.gizmo.rotate_snap_degrees * std::numbers::pi / 180.0;
                            delta_angle = std::round(delta_angle / step) * step;
                        }
                        changed = ApplyRotate(gs, selected_items, s_gizmo_drag.pivot, delta_angle);
                    } else if (gs.gizmo.operation == GizmoOperation::Scale) {
                        const double prev_dist = std::max(1e-5, s_gizmo_drag.previous_world.distanceTo(s_gizmo_drag.pivot));
                        const double curr_dist = std::max(1e-5, world_pos.distanceTo(s_gizmo_drag.pivot));
                        double factor = curr_dist / prev_dist;
                        if (gs.gizmo.snapping && gs.gizmo.scale_snap > 0.0f) {
                            factor = 1.0 + std::round((factor - 1.0) / gs.gizmo.scale_snap) * gs.gizmo.scale_snap;
                        }
                        changed = ApplyScale(gs, selected_items, s_gizmo_drag.pivot, factor);
                    }

                    if (changed) {
                        for (const auto& item : selected_items) {
                            MarkDirtyForSelectionKind(gs, item.kind);
                        }
                    }
                    s_gizmo_drag.previous_world = world_pos;
                    consumed_interaction = true;
                }
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    s_gizmo_drag.active = false;
                    consumed_interaction = true;
                }
            }
        }

        // Road vertex editing while in road mode.
        if (!consumed_interaction &&
            current_state == RogueCity::Core::Editor::EditorState::Editing_Roads &&
            gs.selection.selected_road) {
            const uint32_t road_id = gs.selection.selected_road->id;
            RogueCity::Core::Road* road = FindRoadMutable(gs, road_id);
            if (road && road->points.size() >= 2) {
                const double pick_radius = std::max(5.0, 9.0 / std::max(0.1f, zoom));
                if (!s_road_vertex_drag.active && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt) {
                    double best = std::numeric_limits<double>::max();
                    size_t best_idx = 0;
                    for (size_t i = 0; i < road->points.size(); ++i) {
                        const double d = road->points[i].distanceTo(world_pos);
                        if (d < best) {
                            best = d;
                            best_idx = i;
                        }
                    }
                    if (best <= pick_radius) {
                        s_road_vertex_drag.active = true;
                        s_road_vertex_drag.road_id = road_id;
                        s_road_vertex_drag.vertex_index = best_idx;
                        consumed_interaction = true;
                    }
                }
                if (s_road_vertex_drag.active) {
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                        RogueCity::Core::Vec2 snapped = world_pos;
                        if (gs.district_boundary_editor.snap_to_grid && gs.district_boundary_editor.snap_size > 0.0f) {
                            const double snap = gs.district_boundary_editor.snap_size;
                            snapped.x = std::round(snapped.x / snap) * snap;
                            snapped.y = std::round(snapped.y / snap) * snap;
                        }
                        RogueCity::App::EditorManipulation::MoveRoadVertex(*road, s_road_vertex_drag.vertex_index, snapped);
                        MarkDirtyForSelectionKind(gs, RogueCity::Core::Editor::VpEntityKind::Road);
                        consumed_interaction = true;
                    }
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                        s_road_vertex_drag.active = false;
                        consumed_interaction = true;
                    }
                }
            }
        }

        // District boundary direct manipulation.
        if (!consumed_interaction &&
            current_state == RogueCity::Core::Editor::EditorState::Editing_Districts &&
            gs.district_boundary_editor.enabled &&
            gs.selection.selected_district) {
            const uint32_t district_id = gs.selection.selected_district->id;
            RogueCity::Core::District* district = FindDistrictMutable(gs, district_id);
            if (district && district->border.size() >= 3) {
                const double pick_radius = std::max(5.0, 9.0 / std::max(0.1f, zoom));
                if (!s_district_boundary_drag.active && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt) {
                    double best = std::numeric_limits<double>::max();
                    size_t best_idx = 0;
                    for (size_t i = 0; i < district->border.size(); ++i) {
                        const double d = district->border[i].distanceTo(world_pos);
                        if (d < best) {
                            best = d;
                            best_idx = i;
                        }
                    }

                    if (gs.district_boundary_editor.delete_mode && best <= pick_radius) {
                        RogueCity::App::EditorManipulation::RemoveDistrictVertex(*district, best_idx);
                        MarkDirtyForSelectionKind(gs, RogueCity::Core::Editor::VpEntityKind::District);
                        consumed_interaction = true;
                    } else if (gs.district_boundary_editor.insert_mode) {
                        double best_edge = std::numeric_limits<double>::max();
                        size_t edge_idx = 0;
                        for (size_t i = 0; i < district->border.size(); ++i) {
                            const auto& a = district->border[i];
                            const auto& b = district->border[(i + 1) % district->border.size()];
                            const double d = DistanceToSegment(world_pos, a, b);
                            if (d < best_edge) {
                                best_edge = d;
                                edge_idx = i;
                            }
                        }
                        if (best_edge <= pick_radius * 1.5) {
                            RogueCity::App::EditorManipulation::InsertDistrictVertex(*district, edge_idx, world_pos);
                            MarkDirtyForSelectionKind(gs, RogueCity::Core::Editor::VpEntityKind::District);
                            consumed_interaction = true;
                        }
                    } else if (best <= pick_radius) {
                        s_district_boundary_drag.active = true;
                        s_district_boundary_drag.district_id = district_id;
                        s_district_boundary_drag.vertex_index = best_idx;
                        consumed_interaction = true;
                    }
                }

                if (s_district_boundary_drag.active) {
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                        RogueCity::Core::Vec2 snapped = world_pos;
                        if (gs.district_boundary_editor.snap_to_grid && gs.district_boundary_editor.snap_size > 0.0f) {
                            const double snap = gs.district_boundary_editor.snap_size;
                            snapped.x = std::round(snapped.x / snap) * snap;
                            snapped.y = std::round(snapped.y / snap) * snap;
                        }
                        if (s_district_boundary_drag.vertex_index < district->border.size()) {
                            district->border[s_district_boundary_drag.vertex_index] = snapped;
                            MarkDirtyForSelectionKind(gs, RogueCity::Core::Editor::VpEntityKind::District);
                        }
                        consumed_interaction = true;
                    }
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                        s_district_boundary_drag.active = false;
                        consumed_interaction = true;
                    }
                }
            }
        }

        const bool start_box = io.KeyAlt && io.KeyShift && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const bool start_lasso = io.KeyAlt && !io.KeyShift && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (!consumed_interaction && start_box) {
            s_selection_drag.box_active = true;
            s_selection_drag.lasso_active = false;
            s_selection_drag.box_start = world_pos;
            s_selection_drag.box_end = world_pos;
            gs.hovered_entity.reset();
            consumed_interaction = true;
        } else if (!consumed_interaction && start_lasso) {
            s_selection_drag.lasso_active = true;
            s_selection_drag.box_active = false;
            s_selection_drag.lasso_points.clear();
            s_selection_drag.lasso_points.push_back(world_pos);
            gs.hovered_entity.reset();
            consumed_interaction = true;
        }

        if (!consumed_interaction && s_selection_drag.box_active) {
            s_selection_drag.box_end = world_pos;
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                const double min_x = std::min(s_selection_drag.box_start.x, s_selection_drag.box_end.x);
                const double max_x = std::max(s_selection_drag.box_start.x, s_selection_drag.box_end.x);
                const double min_y = std::min(s_selection_drag.box_start.y, s_selection_drag.box_end.y);
                const double max_y = std::max(s_selection_drag.box_start.y, s_selection_drag.box_end.y);

                auto region_items = QueryRegionFromViewportIndex(
                    gs,
                    [=](const RogueCity::Core::Vec2& p) {
                        return p.x >= min_x && p.x <= max_x && p.y >= min_y && p.y <= max_y;
                    },
                    true);
                gs.selection_manager.SetItems(std::move(region_items));
                RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
                gs.hovered_entity.reset();
                s_selection_drag.box_active = false;
            }
        } else if (!consumed_interaction && s_selection_drag.lasso_active) {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                if (s_selection_drag.lasso_points.empty() ||
                    s_selection_drag.lasso_points.back().distanceTo(world_pos) > 3.0) {
                    s_selection_drag.lasso_points.push_back(world_pos);
                }
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                auto region_items = QueryRegionFromViewportIndex(
                    gs,
                    [&](const RogueCity::Core::Vec2& p) {
                        return PointInPolygon(p, s_selection_drag.lasso_points);
                    });
                gs.selection_manager.SetItems(std::move(region_items));
                RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
                gs.hovered_entity.reset();
                s_selection_drag.lasso_active = false;
                s_selection_drag.lasso_points.clear();
            }
        } else if (!consumed_interaction) {
            const auto hovered = PickFromViewportIndex(gs, world_pos, zoom);
            gs.hovered_entity = hovered;

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (hovered.has_value()) {
                    if (io.KeyShift) {
                        gs.selection_manager.Add(hovered->kind, hovered->id);
                    } else if (io.KeyCtrl) {
                        gs.selection_manager.Toggle(hovered->kind, hovered->id);
                    } else {
                        gs.selection_manager.Select(hovered->kind, hovered->id);
                    }
                    RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
                } else if (!io.KeyShift && !io.KeyCtrl) {
                    gs.selection_manager.Clear();
                    RogueCity::Core::Editor::ClearPrimarySelection(gs.selection);
                    gs.hovered_entity.reset();
                }
            }
        }
    }

    if (!editor_hovered || !in_viewport || minimap_hovered) {
        gs.hovered_entity.reset();
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
            gs.dirty_layers.MarkFromAxiomEdit();
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

    // Editor-local validation overlay payload.
    gs.validation_overlay.errors = RogueCity::Core::Validation::CollectOverlayValidationErrors(gs);

    // Road vertex handles + spline preview in road mode.
    if (current_state == RogueCity::Core::Editor::EditorState::Editing_Roads && gs.selection.selected_road) {
        const uint32_t road_id = gs.selection.selected_road->id;
        if (RogueCity::Core::Road* road = FindRoadMutable(gs, road_id); road != nullptr) {
            for (size_t i = 0; i < road->points.size(); ++i) {
                const ImVec2 p = s_primary_viewport->world_to_screen(road->points[i]);
                const ImU32 color = (s_road_vertex_drag.active && s_road_vertex_drag.vertex_index == i)
                    ? IM_COL32(255, 215, 80, 255)
                    : IM_COL32(120, 220, 255, 225);
                draw_list->AddCircleFilled(p, 4.0f, color, 12);
                draw_list->AddCircle(p, 6.0f, IM_COL32(20, 20, 20, 200), 12, 1.0f);
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
                        IM_COL32(255, 190, 80, 185),
                        gs.spline_editor.closed,
                        2.0f);
                }
            }
        }
    }

    // District boundary handles in district mode.
    if (current_state == RogueCity::Core::Editor::EditorState::Editing_Districts &&
        gs.district_boundary_editor.enabled &&
        gs.selection.selected_district) {
        const uint32_t district_id = gs.selection.selected_district->id;
        if (RogueCity::Core::District* district = FindDistrictMutable(gs, district_id); district != nullptr) {
            for (size_t i = 0; i < district->border.size(); ++i) {
                const ImVec2 p = s_primary_viewport->world_to_screen(district->border[i]);
                const ImU32 color = (s_district_boundary_drag.active && s_district_boundary_drag.vertex_index == i)
                    ? IM_COL32(255, 120, 80, 255)
                    : IM_COL32(80, 255, 170, 235);
                draw_list->AddRectFilled(ImVec2(p.x - 4.0f, p.y - 4.0f), ImVec2(p.x + 4.0f, p.y + 4.0f), color);
                draw_list->AddRect(ImVec2(p.x - 6.0f, p.y - 6.0f), ImVec2(p.x + 6.0f, p.y + 6.0f), IM_COL32(20, 20, 20, 190), 0.0f, 0, 1.0f);
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
    overlay_config.show_zone_colors = gs.districts.size() != 0;
    overlay_config.show_road_labels = gs.roads.size() != 0;
    overlay_config.show_lot_boundaries = gs.lots.size() != 0;  // Enable lot boundaries
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

    if (s_selection_drag.box_active) {
        const ImVec2 a = s_primary_viewport->world_to_screen(s_selection_drag.box_start);
        const ImVec2 b = s_primary_viewport->world_to_screen(s_selection_drag.box_end);
        draw_list->AddRect(
            ImVec2(std::min(a.x, b.x), std::min(a.y, b.y)),
            ImVec2(std::max(a.x, b.x), std::max(a.y, b.y)),
            IM_COL32(120, 220, 255, 220),
            0.0f,
            0,
            2.0f);
    }

    if (s_selection_drag.lasso_active && s_selection_drag.lasso_points.size() >= 2) {
        std::vector<ImVec2> lasso_screen;
        lasso_screen.reserve(s_selection_drag.lasso_points.size());
        for (const auto& p : s_selection_drag.lasso_points) {
            lasso_screen.push_back(s_primary_viewport->world_to_screen(p));
        }
        draw_list->AddPolyline(
            lasso_screen.data(),
            static_cast<int>(lasso_screen.size()),
            IM_COL32(120, 220, 255, 220),
            false,
            2.0f);
    }
    
    // === ROGUENAV MINIMAP OVERLAY ===
    // Render minimap as overlay in top-right corner
    RenderMinimapOverlay(draw_list, viewport_pos, viewport_size);
    
    // === MINIMAP INTERACTION (only if viewport window is hovered) ===
    const bool viewport_window_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    
    if (viewport_window_hovered && s_minimap_visible) {
        const ImVec2 minimap_interact_pos = ImVec2(
            viewport_pos.x + viewport_size.x - kMinimapSize - kMinimapPadding,
            viewport_pos.y + kMinimapPadding
        );
        const ImVec2 minimap_interact_max = ImVec2(minimap_interact_pos.x + kMinimapSize, minimap_interact_pos.y + kMinimapSize);
        
        // Check if mouse is over minimap
        const ImVec2 mouse_screen = ImGui::GetMousePos();
        const bool minimap_hovered = (mouse_screen.x >= minimap_interact_pos.x && mouse_screen.x <= minimap_interact_max.x &&
                                       mouse_screen.y >= minimap_interact_pos.y && mouse_screen.y <= minimap_interact_max.y);
        
        if (minimap_hovered) {
            // Scroll to zoom
            float scroll = ImGui::GetIO().MouseWheel;
            if (scroll != 0.0f) {
                s_minimap_zoom *= (1.0f + scroll * 0.1f);
                s_minimap_zoom = std::clamp(s_minimap_zoom, 0.5f, 3.0f);
            }

            const ImGuiIO& io = ImGui::GetIO();
            if (ImGui::IsKeyPressed(ImGuiKey_L) && !io.WantTextInput) {
                if (io.KeyShift) {
                    s_minimap_auto_lod = !s_minimap_auto_lod;
                } else if (!s_minimap_auto_lod) {
                    CycleManualMinimapLOD();
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_K) && !io.WantTextInput) {
                s_minimap_adaptive_quality = !s_minimap_adaptive_quality;
            }
             
            const bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f);
            if (dragging) {
                const auto camera_pos = s_primary_viewport->get_camera_xy();
                const auto camera_z = s_primary_viewport->get_camera_z();
                const ImVec2 delta = io.MouseDelta;
                const double world_per_pixel = (kMinimapWorldSize * s_minimap_zoom) / kMinimapSize;
                RogueCity::Core::Vec2 next_camera = camera_pos;
                next_camera.x -= static_cast<double>(delta.x) * world_per_pixel;
                next_camera.y -= static_cast<double>(delta.y) * world_per_pixel;
                s_primary_viewport->set_camera_position(next_camera, camera_z);
            } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // Click to jump camera
                const auto camera_pos = s_primary_viewport->get_camera_xy();
                const auto camera_z = s_primary_viewport->get_camera_z();
                const auto world_pos = MinimapPixelToWorld(mouse_screen, minimap_interact_pos, camera_pos);
                s_primary_viewport->set_camera_position(world_pos, camera_z);
            }
        }
    }
    
    // Minimap toggle hotkey (M key) - only when viewport has focus
    if (viewport_window_hovered && ImGui::IsKeyPressed(ImGuiKey_M) && !ImGui::GetIO().WantTextInput) {
        s_minimap_visible = !s_minimap_visible;
    }
    
    // Validation errors are shown in the Tools strip (and can still be overlayed here if desired).
    draw_list->PopClipRect();

    uiint.EndPanel();
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::AxiomEditor
