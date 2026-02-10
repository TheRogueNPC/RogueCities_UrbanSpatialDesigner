// FILE: rc_viewport_overlays.cpp
// PURPOSE: Implementation of viewport overlays

#include "ui/viewport/rc_viewport_overlays.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>

namespace RC_UI::Viewport {

namespace {

const RogueCity::Core::Road* FindRoadById(const RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (const auto& road : gs.roads) {
        if (road.id == id) {
            return &road;
        }
    }
    return nullptr;
}

const RogueCity::Core::District* FindDistrictById(const RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (const auto& district : gs.districts) {
        if (district.id == id) {
            return &district;
        }
    }
    return nullptr;
}

const RogueCity::Core::LotToken* FindLotById(const RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (const auto& lot : gs.lots) {
        if (lot.id == id) {
            return &lot;
        }
    }
    return nullptr;
}

const RogueCity::Core::BuildingSite* FindBuildingById(const RogueCity::Core::Editor::GlobalState& gs, uint32_t id) {
    for (const auto& building : gs.buildings) {
        if (building.id == id) {
            return &building;
        }
    }
    return nullptr;
}

} // namespace

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

    if (config.show_slope_heatmap) {
        RenderSlopeHeatmap(gs);
    }

    if (config.show_no_build_mask) {
        RenderNoBuildMask(gs);
    }

    if (config.show_nature_heatmap) {
        RenderNatureHeatmap(gs);
    }
    
    // AI_INTEGRATION_TAG: V1_PASS1_TASK5_RENDER_NEW_OVERLAYS
    if (config.show_lot_boundaries) {
        RenderLotBoundaries(gs);
    }
    
    if (config.show_water_bodies) {
        RenderWaterBodies(gs);
    }
    
    if (config.show_building_sites) {
        RenderBuildingSites(gs);
    }

    RenderSelectionOutlines(gs);
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
        if (!lot.boundary.empty()) {
            DrawPolygon(lot.boundary, color);
            continue;
        }

        const ImVec2 pos = WorldToScreen(lot.centroid);
        ImGui::GetWindowDrawList()->AddCircleFilled(
            pos,
            std::max(2.0f, 3.0f * view_transform_.zoom),
            ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a)));
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
    std::unordered_map<uint32_t, float> budget_by_district;
    budget_by_district.reserve(gs.lots.size());
    for (const auto& lot : gs.lots) {
        budget_by_district[lot.district_id] += lot.budget_allocation;
    }

    float max_budget = 0.0f;
    for (const auto& district : gs.districts) {
        const float budget = district.budget_allocated > 0.0f
            ? district.budget_allocated
            : budget_by_district[district.id];
        max_budget = std::max(max_budget, budget);
    }
    max_budget = std::max(1.0f, max_budget);

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
        
        const float budget = district.budget_allocated > 0.0f
            ? district.budget_allocated
            : budget_by_district[district.id];
        const float ratio = budget / max_budget;

        DrawBudgetBar(
            centroid,
            ratio,
            glm::vec4(0.9f, 0.8f, 0.2f, 0.9f),
            glm::vec4(0.1f, 0.1f, 0.1f, 0.8f));

        char budget_label[64];
        std::snprintf(budget_label, sizeof(budget_label), "$%.0f", budget);
        DrawLabel(centroid, budget_label, glm::vec4(1.0f, 0.95f, 0.6f, 0.9f));
    }
}

void ViewportOverlays::RenderSlopeHeatmap(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.world_constraints.isValid()) {
        return;
    }

    const auto& constraints = gs.world_constraints;
    const int stride = std::max(1, std::max(constraints.width, constraints.height) / 90);
    const float radius = std::max(1.0f, WorldToScreenScale(constraints.cell_size * static_cast<double>(stride) * 0.22));
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int y = 0; y < constraints.height; y += stride) {
        for (int x = 0; x < constraints.width; x += stride) {
            const size_t idx = static_cast<size_t>(constraints.toIndex(x, y));
            const float slope = constraints.slope_degrees[idx];
            const float t = std::clamp(slope / 45.0f, 0.0f, 1.0f);
            if (t < 0.12f) {
                continue;
            }

            const ImVec4 color(
                0.15f + 0.85f * t,
                0.85f - 0.65f * t,
                0.20f + 0.10f * (1.0f - t),
                0.12f + 0.35f * t);

            const RogueCity::Core::Vec2 center(
                (static_cast<double>(x) + 0.5) * constraints.cell_size,
                (static_cast<double>(y) + 0.5) * constraints.cell_size);
            draw_list->AddCircleFilled(
                WorldToScreen(center),
                radius,
                ImGui::ColorConvertFloat4ToU32(color));
        }
    }
}

void ViewportOverlays::RenderNoBuildMask(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.world_constraints.isValid()) {
        return;
    }

    const auto& constraints = gs.world_constraints;
    const int stride = std::max(1, std::max(constraints.width, constraints.height) / 120);
    const float radius = std::max(1.0f, WorldToScreenScale(constraints.cell_size * static_cast<double>(stride) * 0.20));
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int y = 0; y < constraints.height; y += stride) {
        for (int x = 0; x < constraints.width; x += stride) {
            const size_t idx = static_cast<size_t>(constraints.toIndex(x, y));
            if (constraints.no_build_mask[idx] == 0u) {
                continue;
            }

            const RogueCity::Core::Vec2 center(
                (static_cast<double>(x) + 0.5) * constraints.cell_size,
                (static_cast<double>(y) + 0.5) * constraints.cell_size);
            draw_list->AddCircleFilled(
                WorldToScreen(center),
                radius,
                IM_COL32(220, 60, 60, 135));
        }
    }
}

void ViewportOverlays::RenderNatureHeatmap(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.world_constraints.isValid()) {
        return;
    }

    const auto& constraints = gs.world_constraints;
    const int stride = std::max(1, std::max(constraints.width, constraints.height) / 95);
    const float radius = std::max(1.0f, WorldToScreenScale(constraints.cell_size * static_cast<double>(stride) * 0.18));
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int y = 0; y < constraints.height; y += stride) {
        for (int x = 0; x < constraints.width; x += stride) {
            const size_t idx = static_cast<size_t>(constraints.toIndex(x, y));
            const float nature = constraints.nature_score[idx];
            if (nature < 0.15f) {
                continue;
            }

            const ImVec4 color(
                0.10f + 0.18f * nature,
                0.55f + 0.40f * nature,
                0.18f + 0.12f * nature,
                0.10f + 0.25f * nature);

            const RogueCity::Core::Vec2 center(
                (static_cast<double>(x) + 0.5) * constraints.cell_size,
                (static_cast<double>(y) + 0.5) * constraints.cell_size);
            draw_list->AddCircleFilled(
                WorldToScreen(center),
                radius,
                ImGui::ColorConvertFloat4ToU32(color));
        }
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

void ViewportOverlays::RenderSelectionOutlines(const RogueCity::Core::Editor::GlobalState& gs) {
    const auto pulse = 0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 2.0f * 3.1415926f * 2.0f);
    const float thickness = 1.5f + pulse * 1.75f;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    auto draw_road = [&](const RogueCity::Core::Road& road, const ImU32 color) {
        if (road.points.size() < 2) {
            return;
        }
        std::vector<ImVec2> screen_points;
        screen_points.reserve(road.points.size());
        for (const auto& point : road.points) {
            screen_points.push_back(WorldToScreen(point));
        }
        draw_list->AddPolyline(screen_points.data(), static_cast<int>(screen_points.size()), color, false, thickness);
    };

    auto draw_district = [&](const RogueCity::Core::District& district, const ImU32 color) {
        if (district.border.size() < 3) {
            return;
        }
        std::vector<ImVec2> screen_points;
        screen_points.reserve(district.border.size());
        for (const auto& point : district.border) {
            screen_points.push_back(WorldToScreen(point));
        }
        draw_list->AddPolyline(screen_points.data(), static_cast<int>(screen_points.size()), color, true, thickness);
    };

    auto draw_lot = [&](const RogueCity::Core::LotToken& lot, const ImU32 color) {
        if (lot.boundary.size() >= 3) {
            std::vector<ImVec2> screen_points;
            screen_points.reserve(lot.boundary.size());
            for (const auto& point : lot.boundary) {
                screen_points.push_back(WorldToScreen(point));
            }
            draw_list->AddPolyline(screen_points.data(), static_cast<int>(screen_points.size()), color, true, thickness);
            return;
        }
        draw_list->AddCircle(WorldToScreen(lot.centroid), 10.0f, color, 24, thickness);
    };

    auto draw_building = [&](const RogueCity::Core::BuildingSite& building, const ImU32 color) {
        const ImVec2 pos = WorldToScreen(building.position);
        const float radius = 7.0f + pulse * 2.0f;
        draw_list->AddRect(ImVec2(pos.x - radius, pos.y - radius), ImVec2(pos.x + radius, pos.y + radius), color, 2.0f, 0, thickness);
    };

    const ImU32 selected_color = IM_COL32(255, 220, 60, 230);
    for (const auto& item : gs.selection_manager.Items()) {
        switch (item.kind) {
        case RogueCity::Core::Editor::VpEntityKind::Road:
            if (const auto* road = FindRoadById(gs, item.id)) {
                draw_road(*road, selected_color);
            }
            break;
        case RogueCity::Core::Editor::VpEntityKind::District:
            if (const auto* district = FindDistrictById(gs, item.id)) {
                draw_district(*district, selected_color);
            }
            break;
        case RogueCity::Core::Editor::VpEntityKind::Lot:
            if (const auto* lot = FindLotById(gs, item.id)) {
                draw_lot(*lot, selected_color);
            }
            break;
        case RogueCity::Core::Editor::VpEntityKind::Building:
            if (const auto* building = FindBuildingById(gs, item.id)) {
                draw_building(*building, selected_color);
            }
            break;
        default:
            break;
        }
    }

    if (!gs.hovered_entity.has_value()) {
        return;
    }

    const auto& hover = *gs.hovered_entity;
    const ImU32 hover_color = IM_COL32(120, 220, 255, 220);
    switch (hover.kind) {
    case RogueCity::Core::Editor::VpEntityKind::Road:
        if (const auto* road = FindRoadById(gs, hover.id)) {
            draw_road(*road, hover_color);
        }
        break;
    case RogueCity::Core::Editor::VpEntityKind::District:
        if (const auto* district = FindDistrictById(gs, hover.id)) {
            draw_district(*district, hover_color);
        }
        break;
    case RogueCity::Core::Editor::VpEntityKind::Lot:
        if (const auto* lot = FindLotById(gs, hover.id)) {
            draw_lot(*lot, hover_color);
        }
        break;
    case RogueCity::Core::Editor::VpEntityKind::Building:
        if (const auto* building = FindBuildingById(gs, hover.id)) {
            draw_building(*building, hover_color);
        }
        break;
    default:
        break;
    }
}

// AI_INTEGRATION_TAG: V1_PASS1_TASK5_WATER_OVERLAY
void ViewportOverlays::RenderWaterBodies(const RogueCity::Core::Editor::GlobalState& gs) {
    using RogueCity::Core::WaterType;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    for (const auto& water : gs.waterbodies) {
        if (water.boundary.empty()) continue;
        
        // Determine color based on water type
        glm::vec4 water_color;
        switch (water.type) {
            case WaterType::River:
                water_color = glm::vec4(0.31f, 0.59f, 0.86f, 0.7f);  // Light blue
                break;
            case WaterType::Lake:
                water_color = glm::vec4(0.20f, 0.47f, 0.78f, 0.6f);  // Medium blue
                break;
            case WaterType::Ocean:
                water_color = glm::vec4(0.12f, 0.35f, 0.71f, 0.65f); // Dark blue
                break;
            case WaterType::Pond:
            default:
                water_color = glm::vec4(0.25f, 0.50f, 0.75f, 0.6f);  // Default blue
                break;
        }
        
        // Render boundary polygon/polyline
        if (water.type == WaterType::River && water.boundary.size() >= 2) {
            // Rivers are rendered as polylines (flowing paths)
            std::vector<ImVec2> screen_points;
            screen_points.reserve(water.boundary.size());
            for (const auto& pt : water.boundary) {
                screen_points.push_back(WorldToScreen(pt));
            }
            
            ImU32 river_color = ImGui::ColorConvertFloat4ToU32(ImVec4(water_color.r, water_color.g, water_color.b, water_color.a));
            float line_width = 3.0f + water.depth * 0.5f; // Width based on depth
            draw_list->AddPolyline(screen_points.data(), static_cast<int>(screen_points.size()), river_color, false, line_width);
            
            // Add direction arrows (optional, for visual clarity)
            for (size_t i = 0; i < water.boundary.size() - 1; ++i) {
                if (i % 10 == 0) { // Every 10th segment
                    RogueCity::Core::Vec2 dir = water.boundary[i + 1] - water.boundary[i];
                    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                    if (len > 1.0) {
                        dir.x /= len;
                        dir.y /= len;
                        RogueCity::Core::Vec2 mid = (water.boundary[i] + water.boundary[i + 1]) * 0.5;
                        RogueCity::Core::Vec2 arrow_tip = mid + dir * 5.0;
                        ImVec2 screen_tip = WorldToScreen(arrow_tip);
                        draw_list->AddCircleFilled(screen_tip, 2.0f, IM_COL32(200, 220, 255, 180));
                    }
                }
            }
        } else {
            // Lakes, ponds, oceans are rendered as filled polygons
            DrawPolygon(water.boundary, water_color);
            
            // Shore detail rendering (if enabled)
            if (water.generate_shore) {
                std::vector<ImVec2> screen_shore;
                screen_shore.reserve(water.boundary.size());
                for (const auto& pt : water.boundary) {
                    screen_shore.push_back(WorldToScreen(pt));
                }
                
                // Draw shore outline with lighter color
                glm::vec4 shore_color(0.78f, 0.71f, 0.55f, 0.8f); // Sandy color
                ImU32 shore = ImGui::ColorConvertFloat4ToU32(ImVec4(shore_color.r, shore_color.g, shore_color.b, shore_color.a));
                draw_list->AddPolyline(screen_shore.data(), static_cast<int>(screen_shore.size()), shore, true, 1.5f);
            }
        }
        
        // Label water body type (only if large enough)
        if (water.boundary.size() >= 3) {
            // Calculate centroid
            RogueCity::Core::Vec2 centroid{};
            for (const auto& pt : water.boundary) {
                centroid.x += pt.x;
                centroid.y += pt.y;
            }
            centroid.x /= static_cast<double>(water.boundary.size());
            centroid.y /= static_cast<double>(water.boundary.size());
            
            const char* type_name = "Water";
            switch (water.type) {
                case WaterType::River: type_name = "River"; break;
                case WaterType::Lake: type_name = "Lake"; break;
                case WaterType::Ocean: type_name = "Ocean"; break;
                case WaterType::Pond: type_name = "Pond"; break;
            }
            
            DrawWorldText(centroid, type_name, glm::vec4(1.0f, 1.0f, 1.0f, 0.9f));
        }
    }
}

// AI_INTEGRATION_TAG: V1_PASS1_TASK5_BUILDING_OVERLAY
void ViewportOverlays::RenderBuildingSites(const RogueCity::Core::Editor::GlobalState& gs) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    for (const auto& building : gs.buildings) {
        // Render building as a simple marker/circle for now
        // In future, this could be footprint polygons with height
        ImVec2 screen_pos = WorldToScreen(building.position);
        
        // Building color based on type
        glm::vec4 building_color;
        using RogueCity::Core::BuildingType;
        switch (building.type) {
            case BuildingType::Residential:
                building_color = glm::vec4(0.4f, 0.6f, 0.95f, 0.85f); // Blue
                break;
            case BuildingType::Retail:
            case BuildingType::MixedUse:
                building_color = glm::vec4(0.4f, 0.95f, 0.6f, 0.85f); // Green
                break;
            case BuildingType::Industrial:
                building_color = glm::vec4(0.95f, 0.4f, 0.4f, 0.85f); // Red
                break;
            case BuildingType::Civic:
                building_color = glm::vec4(0.95f, 0.8f, 0.4f, 0.85f); // Yellow
                break;
            case BuildingType::Luxury:
                building_color = glm::vec4(0.8f, 0.4f, 0.95f, 0.85f); // Purple
                break;
            default:
                building_color = glm::vec4(0.7f, 0.7f, 0.7f, 0.85f); // Gray
                break;
        }
        
        ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(building_color.r, building_color.g, building_color.b, building_color.a));
        
        // Draw building marker (square for now)
        float size = 4.0f * std::max(1.0f, view_transform_.zoom);
        draw_list->AddRectFilled(
            ImVec2(screen_pos.x - size, screen_pos.y - size),
            ImVec2(screen_pos.x + size, screen_pos.y + size),
            color
        );
        
        // Outline for visibility
        ImU32 outline = IM_COL32(255, 255, 255, 100);
        draw_list->AddRect(
            ImVec2(screen_pos.x - size, screen_pos.y - size),
            ImVec2(screen_pos.x + size, screen_pos.y + size),
            outline,
            0.0f,
            0,
            1.0f
        );
        
        // Height indicator (vertical line) - only if enabled in config
        // For now, we'll skip this as it requires config check in Render()
    }
}

// AI_INTEGRATION_TAG: V1_PASS1_TASK5_LOT_OVERLAY
void ViewportOverlays::RenderLotBoundaries(const RogueCity::Core::Editor::GlobalState& gs) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    for (const auto& lot : gs.lots) {
        if (lot.boundary.empty()) continue;
        
        // Lot boundary color (subtle, so it doesn't overwhelm)
        glm::vec4 lot_color(0.8f, 0.8f, 0.8f, 0.3f); // Light gray, semi-transparent
        
        // Convert to screen space
        std::vector<ImVec2> screen_points;
        screen_points.reserve(lot.boundary.size());
        for (const auto& pt : lot.boundary) {
            screen_points.push_back(WorldToScreen(pt));
        }
        
        // Draw boundary lines
        ImU32 line_color = ImGui::ColorConvertFloat4ToU32(ImVec4(lot_color.r, lot_color.g, lot_color.b, lot_color.a));
        draw_list->AddPolyline(
            screen_points.data(),
            static_cast<int>(screen_points.size()),
            line_color,
            true, // Closed loop
            1.0f
        );
    }
}

ViewportOverlays& GetViewportOverlays() {
    static ViewportOverlays instance;
    return instance;
}

} // namespace RC_UI::Viewport
