// FILE: rc_viewport_overlays.cpp
// PURPOSE: Implementation of viewport overlays

#include "ui/viewport/rc_viewport_overlays.h"
#include <imgui.h>
#include <cmath>

namespace RC_UI::Viewport {

glm::vec4 DistrictColorScheme::GetColorForType(RogueCity::Core::DistrictType type) {
    using RogueCity::Core::DistrictType;
    switch (type) {
        case DistrictType::Residential: return Residential();
        case DistrictType::Commercial: return Commercial();
        case DistrictType::Industrial: return Industrial();
        case DistrictType::Civic: return Civic();
        case DistrictType::Mixed:
        default: return Mixed();
    }
}

void ViewportOverlays::Render(const RogueCity::Core::Editor::GlobalState& gs, const OverlayConfig& config) {
    // If viewport size not set, infer from current ImGui window
    if (view_transform_.viewport_size.x <= 0.0f || view_transform_.viewport_size.y <= 0.0f) {
        view_transform_.viewport_pos = ImGui::GetWindowPos();
        view_transform_.viewport_size = ImGui::GetWindowSize();
    }

    // Reset transient hover highlights each frame
    highlights_.has_hovered_lot = false;
    highlights_.has_hovered_building = false;

    if (config.show_zone_colors) {
        RenderZoneColors(gs);
    }
    
    if (config.show_aesp_heatmap) {
        RenderAESPHeatmap(gs, config.aesp_component);
    }
    
    if (config.show_road_labels) {
        RenderRoadLabels(gs);
    }
    
    if (config.show_budget_bars) {
        RenderBudgetIndicators(gs);
    }

    RenderHighlights();
}

void ViewportOverlays::RenderZoneColors(const RogueCity::Core::Editor::GlobalState& gs) {
    // Render color-coded district polygons
    for (const auto& district : gs.districts) {
        glm::vec4 color = DistrictColorScheme::GetColorForType(district.type);
        DrawPolygon(district.border, color);
    }
}

void ViewportOverlays::RenderAESPHeatmap(const RogueCity::Core::Editor::GlobalState& gs, OverlayConfig::AESPComponent component) {
    // Render AESP gradient overlays on lots
    for (const auto& lot : gs.lots) {
        
        // Select score component
        float score = 0.0f;
        switch (component) {
            case OverlayConfig::AESPComponent::Access: score = lot.access; break;
            case OverlayConfig::AESPComponent::Exposure: score = lot.exposure; break;
            case OverlayConfig::AESPComponent::Service: score = lot.serviceability; break;
            case OverlayConfig::AESPComponent::Privacy: score = lot.privacy; break;
            case OverlayConfig::AESPComponent::Combined:
            default: score = lot.aespScore(); break;
        }
        
        glm::vec4 color = GetAESPGradientColor(score);
        // TODO: DrawPolygon for lot boundary (need to add lot.boundary field to LotToken)
    }
}

void ViewportOverlays::RenderRoadLabels(const RogueCity::Core::Editor::GlobalState& gs) {
    // Render road classification labels
    for (const auto& road : gs.roads) {
        
        // Calculate midpoint of road
        if (road.points.empty()) continue;
        RogueCity::Core::Vec2 midpoint = road.points[road.points.size() / 2];
        
        // Get road type name
        const char* type_name = "Road";
        using RogueCity::Core::RoadType;
        switch (road.type) {
            case RoadType::Highway: type_name = "Highway"; break;
            case RoadType::Arterial: type_name = "Arterial"; break;
            case RoadType::Avenue: type_name = "Avenue"; break;
            case RoadType::Boulevard: type_name = "Boulevard"; break;
            case RoadType::Street: type_name = "Street"; break;
            case RoadType::Lane: type_name = "Lane"; break;
            case RoadType::Alleyway: type_name = "Alley"; break;
            default: break;
        }
        
        DrawWorldText(midpoint, type_name, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));
    }
}

void ViewportOverlays::RenderBudgetIndicators(const RogueCity::Core::Editor::GlobalState& gs) {
    // Render budget bars per district
    // TODO: Add budget tracking to District struct
    // For now, placeholder implementation
    for (const auto& district : gs.districts) {
        if (district.border.empty()) continue;
        
        // Calculate district centroid
        RogueCity::Core::Vec2 centroid{};
        for (const auto& pt : district.border) {
            centroid.x += pt.x;
            centroid.y += pt.y;
        }
        centroid.x /= static_cast<double>(district.border.size());
        centroid.y /= static_cast<double>(district.border.size());
        
        // Placeholder ratio until budget tracking is wired
        DrawBudgetBar(centroid, 0.5f, glm::vec4(0.9f, 0.8f, 0.2f, 0.9f), glm::vec4(0.1f, 0.1f, 0.1f, 0.8f));
    }
}

void ViewportOverlays::DrawPolygon(const std::vector<RogueCity::Core::Vec2>& points, const glm::vec4& color) {
    if (points.size() < 3) {
        return;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    std::vector<ImVec2> screen_points;
    screen_points.reserve(points.size());
    for (const auto& pt : points) {
        screen_points.push_back(WorldToScreen(pt));
    }

    const ImU32 fill = ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a));
    const ImU32 outline = ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, std::min(color.a + 0.2f, 1.0f)));
    draw_list->AddConvexPolyFilled(screen_points.data(), static_cast<int>(screen_points.size()), fill);
    draw_list->AddPolyline(screen_points.data(), static_cast<int>(screen_points.size()), outline, true, 1.0f);
}

void ViewportOverlays::DrawWorldText(const RogueCity::Core::Vec2& pos, const char* text, const glm::vec4& color) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 screen = WorldToScreen(pos);
    draw_list->AddText(screen, ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a)), text);
}

void ViewportOverlays::DrawLabel(const RogueCity::Core::Vec2& pos, const char* text, const glm::vec4& color) {
    DrawWorldText(pos, text, color);
}

void ViewportOverlays::DrawBudgetBar(const RogueCity::Core::Vec2& pos, float ratio, const glm::vec4& fill, const glm::vec4& outline) {
    const ImVec2 screen = WorldToScreen(pos);
    const float width = 60.0f;
    const float height = 6.0f;
    const ImVec2 min(screen.x - width * 0.5f, screen.y + 10.0f);
    const ImVec2 max(screen.x + width * 0.5f, screen.y + 10.0f + height);
    const ImVec2 fill_max(min.x + width * std::clamp(ratio, 0.0f, 1.0f), max.y);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(ImVec4(outline.r, outline.g, outline.b, outline.a)));
    draw_list->AddRectFilled(min, fill_max, ImGui::ColorConvertFloat4ToU32(ImVec4(fill.r, fill.g, fill.b, fill.a)));
}

glm::vec4 ViewportOverlays::GetAESPGradientColor(float score) {
    // Gradient: Blue (low) ? Green ? Yellow ? Red (high)
    if (score < 0.25f) {
        float t = score / 0.25f;
        return glm::mix(glm::vec4(0.0f, 0.0f, 1.0f, 0.6f), glm::vec4(0.0f, 1.0f, 0.0f, 0.6f), t);
    } else if (score < 0.5f) {
        float t = (score - 0.25f) / 0.25f;
        return glm::mix(glm::vec4(0.0f, 1.0f, 0.0f, 0.6f), glm::vec4(1.0f, 1.0f, 0.0f, 0.6f), t);
    } else if (score < 0.75f) {
        float t = (score - 0.5f) / 0.25f;
        return glm::mix(glm::vec4(1.0f, 1.0f, 0.0f, 0.6f), glm::vec4(1.0f, 0.5f, 0.0f, 0.6f), t);
    } else {
        float t = (score - 0.75f) / 0.25f;
        return glm::mix(glm::vec4(1.0f, 0.5f, 0.0f, 0.6f), glm::vec4(1.0f, 0.0f, 0.0f, 0.6f), t);
    }
}

ImVec2 ViewportOverlays::WorldToScreen(const RogueCity::Core::Vec2& world_pos) const {
    const ImVec2 vp_pos = view_transform_.viewport_pos;
    const ImVec2 vp_size = view_transform_.viewport_size;

    const RogueCity::Core::Vec2 rel(world_pos.x - view_transform_.camera_xy.x, world_pos.y - view_transform_.camera_xy.y);
    const float c = std::cos(view_transform_.yaw);
    const float s = std::sin(view_transform_.yaw);
    const RogueCity::Core::Vec2 rotated(rel.x * c - rel.y * s, rel.x * s + rel.y * c);

    return ImVec2(
        vp_pos.x + vp_size.x * 0.5f + static_cast<float>(rotated.x) * view_transform_.zoom,
        vp_pos.y + vp_size.y * 0.5f + static_cast<float>(rotated.y) * view_transform_.zoom
    );
}

float ViewportOverlays::WorldToScreenScale(float world_distance) const {
    return world_distance * view_transform_.zoom;
}

void ViewportOverlays::RenderHighlights() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (highlights_.has_selected_lot) {
        const ImVec2 pos = WorldToScreen(highlights_.selected_lot_pos);
        draw_list->AddCircle(pos, 12.0f, IM_COL32(255, 220, 120, 200), 24, 2.0f);
    }
    if (highlights_.has_hovered_lot) {
        const ImVec2 pos = WorldToScreen(highlights_.hovered_lot_pos);
        draw_list->AddCircle(pos, 10.0f, IM_COL32(120, 200, 255, 180), 24, 2.0f);
    }
    if (highlights_.has_selected_building) {
        const ImVec2 pos = WorldToScreen(highlights_.selected_building_pos);
        draw_list->AddRect(ImVec2(pos.x - 6, pos.y - 6), ImVec2(pos.x + 6, pos.y + 6), IM_COL32(255, 180, 80, 220), 2.0f, 0, 2.0f);
    }
    if (highlights_.has_hovered_building) {
        const ImVec2 pos = WorldToScreen(highlights_.hovered_building_pos);
        draw_list->AddRect(ImVec2(pos.x - 5, pos.y - 5), ImVec2(pos.x + 5, pos.y + 5), IM_COL32(120, 255, 180, 200), 2.0f, 0, 2.0f);
    }
}

ViewportOverlays& GetViewportOverlays() {
    static ViewportOverlays instance;
    return instance;
}

} // namespace RC_UI::Viewport
