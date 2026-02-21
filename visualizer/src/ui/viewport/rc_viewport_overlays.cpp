// FILE: rc_viewport_overlays.cpp
// PURPOSE: Implementation of viewport overlays

#include "ui/viewport/rc_viewport_overlays.h"
#include "RogueCity/App/UI/ThemeManager.h"
#include <imgui.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
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

float LayerOpacity(
    const RogueCity::Core::Editor::GlobalState& gs,
    RogueCity::Core::Editor::VpEntityKind kind,
    uint32_t id) {
    const uint8_t layer_id = gs.GetEntityLayer(kind, id);
    if (const auto* layer = gs.FindLayer(layer_id); layer != nullptr) {
        return std::clamp(layer->opacity, 0.05f, 1.0f);
    }
    return 1.0f;
}

bool ResolveAnchorForItem(
    const RogueCity::Core::Editor::GlobalState& gs,
    const RogueCity::Core::Editor::SelectionItem& item,
    RogueCity::Core::Vec2& out_anchor) {
    using RogueCity::Core::Editor::VpEntityKind;

    switch (item.kind) {
    case VpEntityKind::Road:
        if (const auto* road = FindRoadById(gs, item.id); road && !road->points.empty()) {
            out_anchor = road->points[road->points.size() / 2];
            return true;
        }
        return false;
    case VpEntityKind::District:
        if (const auto* district = FindDistrictById(gs, item.id); district && !district->border.empty()) {
            RogueCity::Core::Vec2 centroid{};
            for (const auto& p : district->border) {
                centroid += p;
            }
            centroid /= static_cast<double>(district->border.size());
            out_anchor = centroid;
            return true;
        }
        return false;
    case VpEntityKind::Lot:
        if (const auto* lot = FindLotById(gs, item.id)) {
            out_anchor = lot->centroid;
            return true;
        }
        return false;
    case VpEntityKind::Building:
        if (const auto* building = FindBuildingById(gs, item.id)) {
            out_anchor = building->position;
            return true;
        }
        return false;
    default:
        return false;
    }
}

double SignedArea2D(const std::vector<ImVec2>& points) {
    if (points.size() < 3) {
        return 0.0;
    }
    double area2 = 0.0;
    for (size_t i = 0; i < points.size(); ++i) {
        const size_t j = (i + 1) % points.size();
        area2 += static_cast<double>(points[i].x) * static_cast<double>(points[j].y) -
            static_cast<double>(points[j].x) * static_cast<double>(points[i].y);
    }
    return area2;
}

double Cross2D(const ImVec2& a, const ImVec2& b, const ImVec2& c) {
    return static_cast<double>(b.x - a.x) * static_cast<double>(c.y - a.y) -
        static_cast<double>(b.y - a.y) * static_cast<double>(c.x - a.x);
}

bool PointInTriangle(const ImVec2& p, const ImVec2& a, const ImVec2& b, const ImVec2& c) {
    const double c0 = Cross2D(a, b, p);
    const double c1 = Cross2D(b, c, p);
    const double c2 = Cross2D(c, a, p);
    const bool has_neg = (c0 < 0.0) || (c1 < 0.0) || (c2 < 0.0);
    const bool has_pos = (c0 > 0.0) || (c1 > 0.0) || (c2 > 0.0);
    return !(has_neg && has_pos);
}

bool TriangulateSimplePolygon(const std::vector<ImVec2>& points, std::vector<uint32_t>& out_indices) {
    out_indices.clear();
    if (points.size() < 3) {
        return false;
    }

    std::vector<uint32_t> vertices(points.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(points.size()); ++i) {
        vertices[i] = i;
    }

    if (SignedArea2D(points) < 0.0) {
        std::reverse(vertices.begin(), vertices.end());
    }

    const double epsilon = 1e-8;
    size_t fail_safe = 0;
    while (vertices.size() > 2) {
        bool ear_found = false;
        for (size_t i = 0; i < vertices.size(); ++i) {
            const size_t prev_i = (i + vertices.size() - 1) % vertices.size();
            const size_t next_i = (i + 1) % vertices.size();
            const uint32_t a_i = vertices[prev_i];
            const uint32_t b_i = vertices[i];
            const uint32_t c_i = vertices[next_i];
            const ImVec2& a = points[a_i];
            const ImVec2& b = points[b_i];
            const ImVec2& c = points[c_i];

            if (Cross2D(a, b, c) <= epsilon) {
                continue;
            }

            bool contains_vertex = false;
            for (size_t j = 0; j < vertices.size(); ++j) {
                const uint32_t p_i = vertices[j];
                if (p_i == a_i || p_i == b_i || p_i == c_i) {
                    continue;
                }
                if (PointInTriangle(points[p_i], a, b, c)) {
                    contains_vertex = true;
                    break;
                }
            }
            if (contains_vertex) {
                continue;
            }

            out_indices.push_back(a_i);
            out_indices.push_back(b_i);
            out_indices.push_back(c_i);
            vertices.erase(vertices.begin() + static_cast<std::ptrdiff_t>(i));
            ear_found = true;
            break;
        }

        if (!ear_found) {
            break;
        }
        ++fail_safe;
        if (fail_safe > points.size() * points.size()) {
            break;
        }
    }

    return !out_indices.empty() && out_indices.size() % 3 == 0;
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

    if (config.show_height_field) {
        RenderHeightField(gs);
    }

    if (config.show_tensor_field) {
        RenderTensorField(gs);
    }

    if (config.show_zone_field) {
        RenderZoneField(gs);
    }

    if (config.show_validation_errors) {
        RenderValidationErrors(gs);
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
    if (config.show_connector_graph) {
        RenderConnectorGraph(gs);
    }
    if (config.show_city_boundary) {
        RenderCityBoundary(gs);
    }

    if (config.show_gizmos) {
        RenderGizmos(gs);
    }

    // Grid overlay (draw behind everything)
    RenderGridOverlay(gs);

    if (config.show_scale_ruler) {
        RenderScaleRulerHUD(gs);
    }
    if (config.show_compass_gimbal) {
        RenderCompassGimbalHUD();
    }
    RenderSelectionOutlines(gs);
    RenderHighlights();
}

void ViewportOverlays::RenderZoneColors(const RogueCity::Core::Editor::GlobalState& gs) {
    // Render color-coded district polygons
    for (const auto& district : gs.districts) {
        if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::District, district.id)) {
            continue;
        }
        glm::vec4 color = DistrictColorScheme::GetColorForType(district.type);
        color.a *= LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::District, district.id);
        DrawPolygon(district.border, color);
    }
}

namespace {
float SnapNiceMeters(float meters) {
    if (meters <= 0.0f) return 0.0f;
    const float base = std::pow(10.0f, std::floor(std::log10(meters)));
    const float mant = meters / base;
    float nice = 1.0f;
    if (mant < 1.5f) nice = 1.0f;
    else if (mant < 3.5f) nice = 2.0f;
    else if (mant < 7.5f) nice = 5.0f;
    else nice = 10.0f;
    return nice * base;
}

void FormatDistance(char* out, size_t out_sz, float meters) {
    if (meters >= 1000.0f) {
        const float km = meters / 1000.0f;
        std::snprintf(out, out_sz, "%.2f km", km);
    } else {
        if (meters >= 100.0f) std::snprintf(out, out_sz, "%.0f m", meters);
        else if (meters >= 10.0f) std::snprintf(out, out_sz, "%.1f m", meters);
        else std::snprintf(out, out_sz, "%.2f m", meters);
    }
}
} // namespace

void ViewportOverlays::RenderScaleRulerHUD(const RogueCity::Core::Editor::GlobalState& gs) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float padding = 16.0f;
    const ImVec2 base_pt = ImVec2(view_transform_.viewport_pos.x + padding, view_transform_.viewport_pos.y + view_transform_.viewport_size.y - padding);

    const double mpp = gs.HasTextureSpace() ? gs.TextureSpaceRef().coordinateSystem().metersPerPixel() : gs.city_meters_per_pixel;
    const float ppm = std::max(1e-4f, view_transform_.zoom / static_cast<float>(mpp));
    const float target_px = 140.0f;
    const float meters = target_px / ppm;
    const float nice_m = SnapNiceMeters(meters);
    const float px_len = std::max(6.0f, nice_m * ppm);

    const ImU32 col = IM_COL32(220,220,220,220);
    const ImVec2 a = ImVec2(base_pt.x, base_pt.y - 6.0f);
    const ImVec2 b = ImVec2(base_pt.x + px_len, base_pt.y - 6.0f);
    dl->AddLine(a, b, col, 2.5f);
    dl->AddLine(ImVec2(a.x, a.y - 6.0f), ImVec2(a.x, a.y + 6.0f), col, 2.0f);
    dl->AddLine(ImVec2(b.x, b.y - 6.0f), ImVec2(b.x, b.y + 6.0f), col, 2.0f);

    char buf[64];
    FormatDistance(buf, sizeof(buf), nice_m);
    dl->AddText(ImVec2(a.x, a.y - 20.0f), col, buf);
}

void ViewportOverlays::RenderCompassGimbalHUD() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float padding = 16.0f;
    const float r = 36.0f;
    const ImVec2 center = ImVec2(view_transform_.viewport_pos.x + view_transform_.viewport_size.x - padding - r, view_transform_.viewport_pos.y + padding + r);
    const ImU32 bg = IM_COL32(24,24,28,220);
    const ImU32 ring = IM_COL32(240,240,240,200);
    dl->AddCircleFilled(center, r, bg);
    dl->AddCircle(center, r, ring, 24, 2.0f);

    const float ang = -view_transform_.yaw;
    const float ca = std::cos(ang);
    const float sa = std::sin(ang);
    const ImU32 label_col = IM_COL32(220,220,220,220);
    const ImVec2 n = ImVec2(0.0f * ca - (-1.0f) * sa, 0.0f * sa + (-1.0f) * ca);
    // N/E/S/W positions
    const ImVec2 offsN = ImVec2(center.x + 0.0f, center.y - r + 8.0f);
    const ImVec2 offsE = ImVec2(center.x + r - 8.0f, center.y + 0.0f);
    const ImVec2 offsS = ImVec2(center.x + 0.0f, center.y + r - 8.0f);
    const ImVec2 offsW = ImVec2(center.x - r + 8.0f, center.y + 0.0f);
    dl->AddText(offsN, label_col, "N");
    dl->AddText(offsE, label_col, "E");
    dl->AddText(offsS, label_col, "S");
    dl->AddText(offsW, label_col, "W");

    // North needle (red)
    const ImVec2 needle_tip = ImVec2(center.x + (-sa) * (r - 10.0f), center.y + (-ca) * (r - 10.0f));
    dl->AddLine(center, needle_tip, IM_COL32(220,60,60,240), 3.0f);

    // Interaction: detect clicks inside circle and set requested_yaw_
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mp = ImGui::GetIO().MousePos;
        const float dx = mp.x - center.x;
        const float dy = mp.y - center.y;
        const float dist2 = dx * dx + dy * dy;
        if (dist2 <= r * r) {
            const float ang_click = std::atan2(dy, dx);
            const float desired = ang_click + (3.14159265f * 0.5f);
            requested_yaw_ = desired;
        }
    }
}

void ViewportOverlays::RenderAESPHeatmap(const RogueCity::Core::Editor::GlobalState& gs, OverlayConfig::AESPComponent component) {
    // Render AESP gradient overlays on lots
    for (const auto& lot : gs.lots) {
        if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Lot, lot.id)) {
            continue;
        }
        
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
        color.a *= LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::Lot, lot.id);
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
        if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Road, road.id)) {
            continue;
        }
        
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
        
        const float opacity = LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::Road, road.id);
        DrawWorldText(midpoint, type_name, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f * opacity));
    }
}

void ViewportOverlays::RenderBudgetIndicators(const RogueCity::Core::Editor::GlobalState& gs) {
    std::unordered_map<uint32_t, float> budget_by_district;
    const uint64_t lot_count = gs.lots.size();
    const uint64_t reserve_cap = std::min<uint64_t>(
        lot_count,
        static_cast<uint64_t>(std::numeric_limits<size_t>::max()));
    budget_by_district.reserve(static_cast<size_t>(reserve_cap));
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
        if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::District, district.id)) {
            continue;
        }
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

        const float opacity = LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::District, district.id);
        DrawBudgetBar(
            centroid,
            ratio,
            glm::vec4(0.9f, 0.8f, 0.2f, 0.9f * opacity),
            glm::vec4(0.1f, 0.1f, 0.1f, 0.8f * opacity));

        char budget_label[64];
        std::snprintf(budget_label, sizeof(budget_label), "$%.0f", budget);
        DrawLabel(centroid, budget_label, glm::vec4(1.0f, 0.95f, 0.6f, 0.9f * opacity));
    }
}

void ViewportOverlays::RenderSlopeHeatmap(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.world_constraints.isValid()) {
        return;
    }

    const auto& constraints = gs.world_constraints;
    const int stride = std::max(1, std::max(constraints.width, constraints.height) / 90);
    const float radius = std::max(
        1.0f,
        WorldToScreenScale(static_cast<float>(constraints.cell_size * static_cast<double>(stride) * 0.22)));
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
    const float radius = std::max(
        1.0f,
        WorldToScreenScale(static_cast<float>(constraints.cell_size * static_cast<double>(stride) * 0.20)));
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
    const float radius = std::max(
        1.0f,
        WorldToScreenScale(static_cast<float>(constraints.cell_size * static_cast<double>(stride) * 0.18)));
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

void ViewportOverlays::RenderHeightField(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.HasTextureSpace()) {
        return;
    }

    const auto& texture_space = gs.TextureSpaceRef();
    const auto& height = texture_space.heightLayer();
    if (height.empty()) {
        return;
    }

    float min_h = height.data().front();
    float max_h = min_h;
    for (const float value : height.data()) {
        min_h = std::min(min_h, value);
        max_h = std::max(max_h, value);
    }
    const float range = std::max(1e-4f, max_h - min_h);

    const int stride = std::max(1, height.width() / 96);
    const float radius = std::max(
        1.0f,
        WorldToScreenScale(static_cast<float>(texture_space.coordinateSystem().metersPerPixel() * static_cast<double>(stride) * 0.32)));
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const auto& coords = texture_space.coordinateSystem();

    for (int y = 0; y < height.height(); y += stride) {
        for (int x = 0; x < height.width(); x += stride) {
            const float h = height.at(x, y);
            const float t = std::clamp((h - min_h) / range, 0.0f, 1.0f);
            const ImVec4 color(
                0.15f + 0.75f * t,
                0.20f + 0.55f * (1.0f - std::abs(t - 0.5f) * 1.7f),
                0.85f - 0.65f * t,
                0.10f + 0.28f * t);
            const RogueCity::Core::Vec2 world = coords.pixelToWorld({ x, y });
            draw_list->AddCircleFilled(WorldToScreen(world), radius, ImGui::ColorConvertFloat4ToU32(color));
        }
    }
}

void ViewportOverlays::RenderTensorField(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.HasTextureSpace()) {
        return;
    }

    const auto& texture_space = gs.TextureSpaceRef();
    const auto& tensor = texture_space.tensorLayer();
    if (tensor.empty()) {
        return;
    }

    const int stride = std::max(1, tensor.width() / 60);
    const auto& coords = texture_space.coordinateSystem();
    const double world_step = coords.metersPerPixel() * static_cast<double>(stride);
    const float half_len = std::max(2.0f, WorldToScreenScale(static_cast<float>(world_step * 0.35)));
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (int y = 0; y < tensor.height(); y += stride) {
        for (int x = 0; x < tensor.width(); x += stride) {
            const RogueCity::Core::Vec2 dir = tensor.at(x, y);
            const double mag_sq = dir.lengthSquared();
            if (mag_sq <= 1e-8) {
                continue;
            }

            const RogueCity::Core::Vec2 n = dir / std::sqrt(mag_sq);
            const RogueCity::Core::Vec2 world = coords.pixelToWorld({ x, y });
            const ImVec2 center = WorldToScreen(world);
            const ImVec2 a(center.x - static_cast<float>(n.x) * half_len, center.y - static_cast<float>(n.y) * half_len);
            const ImVec2 b(center.x + static_cast<float>(n.x) * half_len, center.y + static_cast<float>(n.y) * half_len);

            const float angle = static_cast<float>((std::atan2(n.y, n.x) + 3.14159265) / (2.0 * 3.14159265));
            const ImVec4 color(
                0.2f + 0.7f * angle,
                0.8f - 0.5f * angle,
                0.9f - 0.7f * std::abs(angle - 0.5f),
                0.8f);
            draw_list->AddLine(a, b, ImGui::ColorConvertFloat4ToU32(color), 1.4f);
        }
    }
}

void ViewportOverlays::RenderZoneField(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.HasTextureSpace()) {
        return;
    }

    const auto& texture_space = gs.TextureSpaceRef();
    const auto& zone = texture_space.zoneLayer();
    if (zone.empty()) {
        return;
    }

    const int stride = std::max(1, zone.width() / 110);
    const float radius = std::max(
        1.0f,
        WorldToScreenScale(static_cast<float>(texture_space.coordinateSystem().metersPerPixel() * static_cast<double>(stride) * 0.30)));
    const auto& coords = texture_space.coordinateSystem();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    auto zoneColor = [](uint8_t zone_value) {
        if (zone_value == 0u) {
            return ImVec4(0.45f, 0.45f, 0.45f, 0.18f);
        }
        const uint8_t district_value = static_cast<uint8_t>(zone_value - 1u);
        switch (static_cast<RogueCity::Core::DistrictType>(district_value)) {
            case RogueCity::Core::DistrictType::Residential: return ImVec4(0.28f, 0.52f, 0.95f, 0.24f);
            case RogueCity::Core::DistrictType::Commercial: return ImVec4(0.20f, 0.82f, 0.46f, 0.24f);
            case RogueCity::Core::DistrictType::Industrial: return ImVec4(0.93f, 0.34f, 0.30f, 0.26f);
            case RogueCity::Core::DistrictType::Civic: return ImVec4(0.92f, 0.76f, 0.30f, 0.26f);
            case RogueCity::Core::DistrictType::Mixed:
            default:
                return ImVec4(0.72f, 0.72f, 0.72f, 0.22f);
        }
    };

    for (int y = 0; y < zone.height(); y += stride) {
        for (int x = 0; x < zone.width(); x += stride) {
            const uint8_t zone_value = zone.at(x, y);
            if (zone_value == 0u) {
                continue;
            }

            const RogueCity::Core::Vec2 world = coords.pixelToWorld({ x, y });
            const ImVec4 color = zoneColor(zone_value);
            draw_list->AddCircleFilled(
                WorldToScreen(world),
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
    std::vector<uint32_t> triangle_indices;
    if (TriangulateSimplePolygon(screen_points, triangle_indices)) {
        for (size_t i = 0; i + 2 < triangle_indices.size(); i += 3) {
            const ImVec2& a = screen_points[triangle_indices[i]];
            const ImVec2& b = screen_points[triangle_indices[i + 1]];
            const ImVec2& c = screen_points[triangle_indices[i + 2]];
            draw_list->AddTriangleFilled(a, b, c, fill);
        }
    } else {
        draw_list->AddConvexPolyFilled(screen_points.data(), static_cast<int>(screen_points.size()), fill);
    }
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
    // Keep overlay projection consistent with PrimaryViewport::world_to_screen (R(-yaw)).
    const RogueCity::Core::Vec2 rotated(rel.x * c + rel.y * s, -rel.x * s + rel.y * c);

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
        if (!gs.IsEntityVisible(item.kind, item.id)) {
            continue;
        }
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
    if (!gs.IsEntityVisible(hover.kind, hover.id)) {
        return;
    }
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

void ViewportOverlays::RenderValidationErrors(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.validation_overlay.enabled) {
        return;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    for (const auto& error : gs.validation_overlay.errors) {
        if (error.severity == RogueCity::Core::Editor::ValidationSeverity::Warning &&
            !gs.validation_overlay.show_warnings) {
            continue;
        }

        const ImVec2 p = WorldToScreen(error.world_position);
        ImU32 color = IM_COL32(255, 205, 80, 220);
        float radius = 7.0f;
        if (error.severity == RogueCity::Core::Editor::ValidationSeverity::Error) {
            color = IM_COL32(255, 120, 90, 230);
            radius = 8.0f;
        } else if (error.severity == RogueCity::Core::Editor::ValidationSeverity::Critical) {
            color = IM_COL32(255, 65, 65, 240);
            radius = 9.0f;
        }

        draw_list->AddCircleFilled(p, radius, color, 20);
        draw_list->AddCircle(p, radius + 2.0f, IM_COL32(20, 20, 20, 180), 20, 1.0f);

        if (gs.validation_overlay.show_labels && !error.message.empty()) {
            draw_list->AddText(ImVec2(p.x + 8.0f, p.y - 7.0f), color, error.message.c_str());
        }
    }
}

void ViewportOverlays::RenderGizmos(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.gizmo.enabled || !gs.gizmo.visible || gs.selection_manager.Count() == 0) {
        return;
    }

    RogueCity::Core::Vec2 pivot{};
    size_t count = 0;
    for (const auto& item : gs.selection_manager.Items()) {
        if (!gs.IsEntityVisible(item.kind, item.id)) {
            continue;
        }
        RogueCity::Core::Vec2 anchor{};
        if (!ResolveAnchorForItem(gs, item, anchor)) {
            continue;
        }
        pivot += anchor;
        ++count;
    }
    if (count == 0) {
        return;
    }
    pivot /= static_cast<double>(count);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 center = WorldToScreen(pivot);
    const float axis_len = 34.0f;
    const float ring_r = 24.0f;

    switch (gs.gizmo.operation) {
    case RogueCity::Core::Editor::GizmoOperation::Translate:
        draw_list->AddLine(ImVec2(center.x - axis_len, center.y), ImVec2(center.x + axis_len, center.y), IM_COL32(255, 100, 100, 240), 2.5f);
        draw_list->AddLine(ImVec2(center.x, center.y - axis_len), ImVec2(center.x, center.y + axis_len), IM_COL32(100, 220, 255, 240), 2.5f);
        break;
    case RogueCity::Core::Editor::GizmoOperation::Rotate:
        draw_list->AddCircle(center, ring_r, IM_COL32(120, 220, 255, 240), 36, 2.5f);
        draw_list->AddCircle(center, ring_r + 7.0f, IM_COL32(255, 180, 80, 180), 36, 1.5f);
        break;
    case RogueCity::Core::Editor::GizmoOperation::Scale:
        draw_list->AddRect(ImVec2(center.x - ring_r, center.y - ring_r), ImVec2(center.x + ring_r, center.y + ring_r), IM_COL32(120, 255, 150, 240), 0.0f, 0, 2.5f);
        draw_list->AddRectFilled(ImVec2(center.x + ring_r - 5.0f, center.y + ring_r - 5.0f), ImVec2(center.x + ring_r + 5.0f, center.y + ring_r + 5.0f), IM_COL32(120, 255, 150, 220));
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
        if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Water, water.id)) {
            continue;
        }
        if (water.boundary.empty()) continue;
        const float layer_opacity = LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::Water, water.id);
        
        // Determine color based on water type
        glm::vec4 water_color;
        switch (water.type) {
            case WaterType::River:
                water_color = glm::vec4(0.31f, 0.59f, 0.86f, 0.7f * layer_opacity);  // Light blue
                break;
            case WaterType::Lake:
                water_color = glm::vec4(0.20f, 0.47f, 0.78f, 0.6f * layer_opacity);  // Medium blue
                break;
            case WaterType::Ocean:
                water_color = glm::vec4(0.12f, 0.35f, 0.71f, 0.65f * layer_opacity); // Dark blue
                break;
            case WaterType::Pond:
            default:
                water_color = glm::vec4(0.25f, 0.50f, 0.75f, 0.6f * layer_opacity);  // Default blue
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
                    const double len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
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
        if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Building, building.id)) {
            continue;
        }
        const float layer_opacity = LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::Building, building.id);
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
        
        building_color.a *= layer_opacity;
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
        if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Lot, lot.id)) {
            continue;
        }
        if (lot.boundary.empty()) continue;
        const float layer_opacity = LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::Lot, lot.id);
        
        // Lot boundary color (subtle, so it doesn't overwhelm)
        glm::vec4 lot_color(0.8f, 0.8f, 0.8f, 0.3f * layer_opacity); // Light gray, semi-transparent
        
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

void ViewportOverlays::RenderConnectorGraph(const RogueCity::Core::Editor::GlobalState& gs) {
    if (gs.connector_debug_edges.empty()) {
        return;
    }
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImU32 connector_color = IM_COL32(255, 90, 90, 195);
    for (const auto& edge : gs.connector_debug_edges) {
        if (edge.points.size() < 2) {
            continue;
        }
        const ImVec2 a = WorldToScreen(edge.points.front());
        const ImVec2 b = WorldToScreen(edge.points.back());
        draw_list->AddLine(a, b, connector_color, 2.6f);
    }
}

void ViewportOverlays::RenderCityBoundary(const RogueCity::Core::Editor::GlobalState& gs) {
    if (gs.city_boundary.size() < 3) {
        return;
    }
    std::vector<ImVec2> screen_points;
    screen_points.reserve(gs.city_boundary.size());
    for (const auto& p : gs.city_boundary) {
        screen_points.push_back(WorldToScreen(p));
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddPolyline(
        screen_points.data(),
        static_cast<int>(screen_points.size()),
        IM_COL32(35, 90, 255, 240),
        true,
        3.0f);
}

// Y2K Grid Overlay - 50px grid with themed color
void ViewportOverlays::RenderGridOverlay(const RogueCity::Core::Editor::GlobalState& gs) {
    if (!gs.config.show_grid_overlay) {
        return;
    }
    if (view_transform_.viewport_size.x <= 0.0f ||
        view_transform_.viewport_size.y <= 0.0f ||
        view_transform_.zoom <= 1e-6f) {
        return;
    }
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    auto& theme_mgr = ::RogueCity::UI::ThemeManager::Instance();
    const auto& theme = theme_mgr.GetActiveTheme();
    
    // Grid in subtle primary accent (low alpha)
    ImVec4 primary_accent = ImGui::ColorConvertU32ToFloat4(theme.primary_accent);
    primary_accent.w = 0.15f;
    ImU32 grid_color = ImGui::ColorConvertFloat4ToU32(primary_accent);
    
    // Grid spacing: 50px in world space (adjust per zoom if needed)
    const double grid_spacing = 50.0;
    
    // Calculate visible bounds in world space from the four viewport corners.
    const ImVec2 viewport_min = view_transform_.viewport_pos;
    const ImVec2 viewport_max = ImVec2(
        viewport_min.x + view_transform_.viewport_size.x,
        viewport_min.y + view_transform_.viewport_size.y
    );

    const float c = std::cos(view_transform_.yaw);
    const float s = std::sin(view_transform_.yaw);
    const auto screen_to_world = [&](const ImVec2& screen_pos) -> RogueCity::Core::Vec2 {
        const float nx = (screen_pos.x - viewport_min.x) / view_transform_.viewport_size.x - 0.5f;
        const float ny = (screen_pos.y - viewport_min.y) / view_transform_.viewport_size.y - 0.5f;
        const RogueCity::Core::Vec2 view(
            static_cast<double>(nx * view_transform_.viewport_size.x / view_transform_.zoom),
            static_cast<double>(ny * view_transform_.viewport_size.y / view_transform_.zoom));
        const RogueCity::Core::Vec2 rel(
            view.x * c - view.y * s,
            view.x * s + view.y * c);
        return RogueCity::Core::Vec2(
            view_transform_.camera_xy.x + rel.x,
            view_transform_.camera_xy.y + rel.y);
    };

    const std::array<RogueCity::Core::Vec2, 4> corners{
        screen_to_world(viewport_min),
        screen_to_world(ImVec2(viewport_max.x, viewport_min.y)),
        screen_to_world(viewport_max),
        screen_to_world(ImVec2(viewport_min.x, viewport_max.y))
    };

    double world_min_x = corners[0].x;
    double world_max_x = corners[0].x;
    double world_min_y = corners[0].y;
    double world_max_y = corners[0].y;
    for (const auto& corner : corners) {
        world_min_x = std::min(world_min_x, corner.x);
        world_max_x = std::max(world_max_x, corner.x);
        world_min_y = std::min(world_min_y, corner.y);
        world_max_y = std::max(world_max_y, corner.y);
    }
    
    // Snap to grid multiples
    const int grid_start_x = static_cast<int>(std::floor(world_min_x / grid_spacing)) - 1;
    const int grid_end_x = static_cast<int>(std::ceil(world_max_x / grid_spacing)) + 1;
    const int grid_start_y = static_cast<int>(std::floor(world_min_y / grid_spacing)) - 1;
    const int grid_end_y = static_cast<int>(std::ceil(world_max_y / grid_spacing)) + 1;
    
    // Draw vertical grid lines
    for (int i = grid_start_x; i <= grid_end_x; ++i) {
        const double world_x = static_cast<double>(i) * grid_spacing;
        const ImVec2 p1 = WorldToScreen(RogueCity::Core::Vec2{world_x, world_min_y});
        const ImVec2 p2 = WorldToScreen(RogueCity::Core::Vec2{world_x, world_max_y});
        draw_list->AddLine(p1, p2, grid_color, 1.0f);
    }
    
    // Draw horizontal grid lines
    for (int i = grid_start_y; i <= grid_end_y; ++i) {
        const double world_y = static_cast<double>(i) * grid_spacing;
        const ImVec2 p1 = WorldToScreen(RogueCity::Core::Vec2{world_min_x, world_y});
        const ImVec2 p2 = WorldToScreen(RogueCity::Core::Vec2{world_max_x, world_y});
        draw_list->AddLine(p1, p2, grid_color, 1.0f);
    }
}

ViewportOverlays& GetViewportOverlays() {
    static ViewportOverlays instance;
    return instance;
}

} // namespace RC_UI::Viewport
