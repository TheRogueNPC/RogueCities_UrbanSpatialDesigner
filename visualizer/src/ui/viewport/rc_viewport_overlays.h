// FILE: rc_viewport_overlays.h
// PURPOSE: Viewport overlay rendering for zones, AESP, roads, budgets
// LAYER: Visualizer (App UI)
// PHASE 2: Step 6 - Visualization

#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"
#include <imgui.h>
#include <glm/glm.hpp>

namespace RC_UI::Viewport {

// Overlay visibility toggles
struct OverlayConfig {
    bool show_zone_colors = true;
    bool show_aesp_heatmap = false;
    bool show_road_labels = true;
    bool show_budget_bars = false;
    bool show_slope_heatmap = false;
    bool show_no_build_mask = true;
    bool show_nature_heatmap = false;
    bool show_tensor_field = false;
    bool show_height_field = false;
    bool show_zone_field = false;
    bool show_validation_errors = true;
    bool show_gizmos = true;
    
    // AI_INTEGRATION_TAG: V1_PASS1_TASK5_OVERLAY_CONFIG
    bool show_axioms = true;
    bool show_roads = true;
    bool show_districts = true;
    bool show_lots = true;
    bool show_water_bodies = true;
    bool show_building_sites = true;
    bool show_lot_boundaries = true;
    bool show_height_indicators = true;
    bool show_city_boundary = true;
    bool show_connector_graph = true;
    
    // AESP component selection (for heatmap)
    enum class AESPComponent {
        Access,
        Exposure,
        Service,
        Privacy,
        Combined
    };
    AESPComponent aesp_component = AESPComponent::Combined;
    bool show_scale_ruler = true;
    bool show_compass_gimbal = true;
};

// Color scheme for districts (Y2K palette)
struct DistrictColorScheme {
    static glm::vec4 GetColorForType(RogueCity::Core::DistrictType type);
    static glm::vec4 Residential() { return glm::vec4(0.3f, 0.5f, 0.9f, 0.6f); }  // Blue
    static glm::vec4 Commercial() { return glm::vec4(0.3f, 0.9f, 0.5f, 0.6f); }   // Green
    static glm::vec4 Industrial() { return glm::vec4(0.9f, 0.3f, 0.3f, 0.6f); }   // Red
    static glm::vec4 Civic() { return glm::vec4(0.9f, 0.7f, 0.3f, 0.6f); }        // Orange
    static glm::vec4 Mixed() { return glm::vec4(0.7f, 0.7f, 0.7f, 0.6f); }        // Gray
};

// Viewport overlay renderer
class ViewportOverlays {
public:
    ViewportOverlays() = default;

    struct ViewTransform {
        RogueCity::Core::Vec2 camera_xy{ 0.0, 0.0 };
        float zoom{ 1.0f };
        float yaw{ 0.0f };
        ImVec2 viewport_pos{ 0.0f, 0.0f };
        ImVec2 viewport_size{ 0.0f, 0.0f };
    };

    struct HighlightState {
        bool has_selected_lot{ false };
        bool has_hovered_lot{ false };
        bool has_selected_building{ false };
        bool has_hovered_building{ false };
        RogueCity::Core::Vec2 selected_lot_pos{};
        RogueCity::Core::Vec2 hovered_lot_pos{};
        RogueCity::Core::Vec2 selected_building_pos{};
        RogueCity::Core::Vec2 hovered_building_pos{};
    };
    
    // Main render call (called from main viewport loop)
    void Render(const RogueCity::Core::Editor::GlobalState& gs, const OverlayConfig& config);
    
    // Individual overlay renders
    void RenderZoneColors(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderAESPHeatmap(const RogueCity::Core::Editor::GlobalState& gs, OverlayConfig::AESPComponent component);
    void RenderRoadLabels(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderBudgetIndicators(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderSlopeHeatmap(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderNoBuildMask(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderNatureHeatmap(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderTensorField(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderHeightField(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderZoneField(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderValidationErrors(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderGizmos(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderGridOverlay(const RogueCity::Core::Editor::GlobalState& gs);
    
    // AI_INTEGRATION_TAG: V1_PASS1_TASK5_VIEWPORT_OVERLAYS
    void RenderWaterBodies(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderBuildingSites(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderLotBoundaries(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderCityBoundary(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderConnectorGraph(const RogueCity::Core::Editor::GlobalState& gs);

    void RenderScaleRulerHUD(const RogueCity::Core::Editor::GlobalState& gs);
    void RenderCompassGimbalHUD();
    std::optional<float> requested_yaw_{};

    void SetViewTransform(const ViewTransform& transform) { view_transform_ = transform; }
    const ViewTransform& GetViewTransform() const { return view_transform_; }

    void SetSelectedLot(const RogueCity::Core::Vec2& pos) { highlights_.has_selected_lot = true; highlights_.selected_lot_pos = pos; }
    void SetHoveredLot(const RogueCity::Core::Vec2& pos) { highlights_.has_hovered_lot = true; highlights_.hovered_lot_pos = pos; }
    void ClearLotHighlights() { highlights_.has_selected_lot = highlights_.has_hovered_lot = false; }

    void SetSelectedBuilding(const RogueCity::Core::Vec2& pos) { highlights_.has_selected_building = true; highlights_.selected_building_pos = pos; }
    void SetHoveredBuilding(const RogueCity::Core::Vec2& pos) { highlights_.has_hovered_building = true; highlights_.hovered_building_pos = pos; }
    void ClearBuildingHighlights() { highlights_.has_selected_building = highlights_.has_hovered_building = false; }
    
    // Helper: Draw polygon with color
    void DrawPolygon(const std::vector<RogueCity::Core::Vec2>& points, const glm::vec4& color);
    
    // Helper: Draw text at world position
    void DrawWorldText(const RogueCity::Core::Vec2& pos, const char* text, const glm::vec4& color);

    // Drawing primitives
    void DrawLabel(const RogueCity::Core::Vec2& pos, const char* text, const glm::vec4& color);
    void DrawBudgetBar(const RogueCity::Core::Vec2& pos, float ratio, const glm::vec4& fill, const glm::vec4& outline);
    
    // Helper: Calculate AESP gradient color (0.0 = cold, 1.0 = hot)
    glm::vec4 GetAESPGradientColor(float score);

    // Projection helpers
    ImVec2 WorldToScreen(const RogueCity::Core::Vec2& world_pos) const;
    float WorldToScreenScale(float world_distance) const;

    void RenderHighlights();
    void RenderSelectionOutlines(const RogueCity::Core::Editor::GlobalState& gs);

private:
    ViewTransform view_transform_{};
    HighlightState highlights_{};
};

// Singleton accessor
ViewportOverlays& GetViewportOverlays();

} // namespace RC_UI::Viewport
