// FILE: rc_viewport_overlays.cpp
// PURPOSE: Implementation of viewport overlays

#include "ui/viewport/rc_viewport_overlays.h"
#include "RogueCity/App/UI/ThemeManager.h"
#include "ui/rc_ui_tokens.h"
#include "ui/viewport/rc_viewport_lod_policy.h"
#include <RogueCity/Core/Infomatrix.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <imgui.h>
#include <limits>
#include <magic_enum/magic_enum.hpp>
#include <unordered_map>


namespace RC_UI::Viewport {

namespace {

[[nodiscard]] ImU32 TokenColor(ImU32 base, uint8_t alpha = 255u) {
  return RC_UI::WithAlpha(base, alpha);
}

[[nodiscard]] ImVec4 TokenColorF(ImU32 base, uint8_t alpha = 255u) {
  return ImGui::ColorConvertU32ToFloat4(TokenColor(base, alpha));
}

struct RoadRenderStyle {
  ImU32 color{RC_UI::WithAlpha(RC_UI::UITokens::CyanAccent, 200u)};
  float width{2.0f};
};

[[nodiscard]] RoadRenderStyle
ResolveRoadRenderStyle(RogueCity::Core::RoadType type) {
  using RogueCity::Core::RoadType;
  switch (type) {
  case RoadType::Highway:
    return {RC_UI::WithAlpha(RC_UI::UITokens::AmberGlow, 230u), 4.6f};
  case RoadType::Arterial:
  case RoadType::Avenue:
  case RoadType::Boulevard:
  case RoadType::M_Major:
    return {RC_UI::WithAlpha(RC_UI::UITokens::CyanAccent, 220u), 3.2f};
  case RoadType::Street:
  case RoadType::M_Minor:
    return {RC_UI::WithAlpha(RC_UI::UITokens::InfoBlue, 210u), 2.2f};
  case RoadType::Lane:
  case RoadType::Alleyway:
  case RoadType::CulDeSac:
  case RoadType::Drive:
  case RoadType::Driveway:
  default:
    return {RC_UI::WithAlpha(RC_UI::UITokens::TextSecondary, 180u), 1.6f};
  }
}

const RogueCity::Core::Road *
FindRoadById(const RogueCity::Core::Editor::GlobalState &gs, uint32_t id) {
  const auto it = gs.road_handle_by_id.find(id);
  if (it == gs.road_handle_by_id.end()) {
    return nullptr;
  }
  const uint32_t handle = it->second;
  if (!gs.roads.isValidIndex(handle)) {
    return nullptr;
  }
  return &gs.roads[handle];
}

const RogueCity::Core::District *
FindDistrictById(const RogueCity::Core::Editor::GlobalState &gs, uint32_t id) {
  const auto it = gs.district_handle_by_id.find(id);
  if (it == gs.district_handle_by_id.end()) {
    return nullptr;
  }
  const uint32_t handle = it->second;
  if (!gs.districts.isValidIndex(handle)) {
    return nullptr;
  }
  return &gs.districts[handle];
}

const RogueCity::Core::LotToken *
FindLotById(const RogueCity::Core::Editor::GlobalState &gs, uint32_t id) {
  const auto it = gs.lot_handle_by_id.find(id);
  if (it == gs.lot_handle_by_id.end()) {
    return nullptr;
  }
  const uint32_t handle = it->second;
  if (!gs.lots.isValidIndex(handle)) {
    return nullptr;
  }
  return &gs.lots[handle];
}

const RogueCity::Core::BuildingSite *
FindBuildingById(const RogueCity::Core::Editor::GlobalState &gs, uint32_t id) {
  const auto it = gs.building_handle_by_id.find(id);
  if (it == gs.building_handle_by_id.end()) {
    return nullptr;
  }
  const uint32_t handle = it->second;
  if (handle >= gs.buildings.size()) {
    return nullptr;
  }
  const auto &building = gs.buildings.getData()[handle];
  return building.id == id ? &building : nullptr;
}

const RogueCity::Core::WaterBody *
FindWaterById(const RogueCity::Core::Editor::GlobalState &gs, uint32_t id) {
  const auto it = gs.water_handle_by_id.find(id);
  if (it == gs.water_handle_by_id.end()) {
    return nullptr;
  }
  const uint32_t handle = it->second;
  if (!gs.waterbodies.isValidIndex(handle)) {
    return nullptr;
  }
  return &gs.waterbodies[handle];
}

float LayerOpacity(const RogueCity::Core::Editor::GlobalState &gs,
                   RogueCity::Core::Editor::VpEntityKind kind, uint32_t id) {
  const uint8_t layer_id = gs.GetEntityLayer(kind, id);
  if (const auto *layer = gs.FindLayer(layer_id); layer != nullptr) {
    return std::clamp(layer->opacity, 0.05f, 1.0f);
  }
  return 1.0f;
}

bool ResolveAnchorForItem(const RogueCity::Core::Editor::GlobalState &gs,
                          const RogueCity::Core::Editor::SelectionItem &item,
                          RogueCity::Core::Vec2 &out_anchor) {
  using RogueCity::Core::Editor::VpEntityKind;

  switch (item.kind) {
  case VpEntityKind::Road:
    if (const auto *road = FindRoadById(gs, item.id);
        road && !road->points.empty()) {
      out_anchor = road->points[road->points.size() / 2];
      return true;
    }
    return false;
  case VpEntityKind::District:
    if (const auto *district = FindDistrictById(gs, item.id);
        district && !district->border.empty()) {
      RogueCity::Core::Vec2 centroid{};
      for (const auto &p : district->border) {
        centroid += p;
      }
      centroid /= static_cast<double>(district->border.size());
      out_anchor = centroid;
      return true;
    }
    return false;
  case VpEntityKind::Lot:
    if (const auto *lot = FindLotById(gs, item.id)) {
      out_anchor = lot->centroid;
      return true;
    }
    return false;
  case VpEntityKind::Building:
    if (const auto *building = FindBuildingById(gs, item.id)) {
      out_anchor = building->position;
      return true;
    }
    return false;
  case VpEntityKind::Water:
    if (const auto *water = FindWaterById(gs, item.id);
        water && !water->boundary.empty()) {
      out_anchor = water->boundary[water->boundary.size() / 2];
      return true;
    }
    return false;
  default:
    return false;
  }
}

double SignedArea2D(const std::vector<ImVec2> &points) {
  if (points.size() < 3) {
    return 0.0;
  }
  double area2 = 0.0;
  for (size_t i = 0; i < points.size(); ++i) {
    const size_t j = (i + 1) % points.size();
    area2 +=
        static_cast<double>(points[i].x) * static_cast<double>(points[j].y) -
        static_cast<double>(points[j].x) * static_cast<double>(points[i].y);
  }
  return area2;
}

double Cross2D(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c) {
  return static_cast<double>(b.x - a.x) * static_cast<double>(c.y - a.y) -
         static_cast<double>(b.y - a.y) * static_cast<double>(c.x - a.x);
}

bool PointInTriangle(const ImVec2 &p, const ImVec2 &a, const ImVec2 &b,
                     const ImVec2 &c) {
  const double c0 = Cross2D(a, b, p);
  const double c1 = Cross2D(b, c, p);
  const double c2 = Cross2D(c, a, p);
  const bool has_neg = (c0 < 0.0) || (c1 < 0.0) || (c2 < 0.0);
  const bool has_pos = (c0 > 0.0) || (c1 > 0.0) || (c2 > 0.0);
  return !(has_neg && has_pos);
}

bool TriangulateSimplePolygon(const std::vector<ImVec2> &points,
                              std::vector<uint32_t> &out_indices) {
  out_indices.clear();
  if (points.size() < 3) {
    return false;
  }

  thread_local std::vector<uint32_t> vertices;
  vertices.clear();
  vertices.reserve(points.size());
  for (uint32_t i = 0; i < static_cast<uint32_t>(points.size()); ++i) {
    vertices.push_back(i);
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
      const ImVec2 &a = points[a_i];
      const ImVec2 &b = points[b_i];
      const ImVec2 &c = points[c_i];

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

// Adaptive Bezier Tessellation for smooth spline rendering
// Adds points to 'out_points' (excluding p0, which should be added by caller if
// needed)
void TessellateBezierRecursive(const ImVec2 &p0, const ImVec2 &p1,
                               const ImVec2 &p2, const ImVec2 &p3,
                               std::vector<ImVec2> &out_points,
                               float tolerance_sq) {

  // Flatness test: check if control points p1, p2 are close enough to the line
  // segment p0-p3
  float dx = p3.x - p0.x;
  float dy = p3.y - p0.y;
  float d2 = dx * dx + dy * dy;

  bool flat = false;
  if (d2 < 1e-6f) {
    // Endpoints are coincident, check distance of control points to p0
    float d1_sq = (p1.x - p0.x) * (p1.x - p0.x) + (p1.y - p0.y) * (p1.y - p0.y);
    float d2_sq = (p2.x - p0.x) * (p2.x - p0.x) + (p2.y - p0.y) * (p2.y - p0.y);
    flat = (d1_sq < tolerance_sq && d2_sq < tolerance_sq);
  } else {
    // Distance from p1/p2 to line p0-p3
    // Area = 0.5 * |x1(y2 - y3) + x2(y3 - y1) + x3(y1 - y2)|
    // Height = 2 * Area / Base
    float s1 = (dy * p1.x - dx * p1.y + p3.x * p0.y - p3.y * p0.x);
    float dist1_sq = (s1 * s1) / d2;

    float s2 = (dy * p2.x - dx * p2.y + p3.x * p0.y - p3.y * p0.x);
    float dist2_sq = (s2 * s2) / d2;

    flat = (dist1_sq < tolerance_sq && dist2_sq < tolerance_sq);
  }

  if (flat) {
    out_points.push_back(p3);
  } else {
    // De Casteljau subdivision
    ImVec2 p01 = ImVec2((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
    ImVec2 p12 = ImVec2((p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f);
    ImVec2 p23 = ImVec2((p2.x + p3.x) * 0.5f, (p2.y + p3.y) * 0.5f);
    ImVec2 p012 = ImVec2((p01.x + p12.x) * 0.5f, (p01.y + p12.y) * 0.5f);
    ImVec2 p123 = ImVec2((p12.x + p23.x) * 0.5f, (p12.y + p23.y) * 0.5f);
    ImVec2 p0123 = ImVec2((p012.x + p123.x) * 0.5f, (p012.y + p123.y) * 0.5f);

    TessellateBezierRecursive(p0, p01, p012, p0123, out_points, tolerance_sq);
    TessellateBezierRecursive(p0123, p123, p23, p3, out_points, tolerance_sq);
  }
}

} // namespace

glm::vec4
DistrictColorScheme::GetColorForType(RogueCity::Core::DistrictType type) {
  using RogueCity::Core::DistrictType;
  switch (type) {
  case DistrictType::Residential:
    return Residential();
  case DistrictType::Commercial:
    return Commercial();
  case DistrictType::Industrial:
    return Industrial();
  case DistrictType::Civic:
    return Civic();
  case DistrictType::Mixed:
  default:
    return Mixed();
  }
}

void ViewportOverlays::Render(RogueCity::Core::Editor::GlobalState &gs,
                              const OverlayConfig &config) {
  // If viewport size not set, infer from current ImGui window
  if (view_transform_.viewport_size.x <= 0.0f ||
      view_transform_.viewport_size.y <= 0.0f) {
    view_transform_.viewport_pos = ImGui::GetWindowPos();
    view_transform_.viewport_size = ImGui::GetWindowSize();
  }

  auto &telemetry = gs.tool_runtime.viewport_render_telemetry;
  telemetry.ResetFrame();
  telemetry.last_frame_updated = gs.frame_counter;
  scratch_deduped_handles_by_layer_.fill(0u);
  scratch_active_dedupe_layer_.reset();
  scratch_fallback_full_scan_count_ = 0u;

  int min_cell_x = 0;
  int max_cell_x = 0;
  int min_cell_y = 0;
  int max_cell_y = 0;
  if (ComputeVisibleCellRange(gs, min_cell_x, max_cell_x, min_cell_y,
                              max_cell_y)) {
    telemetry.visible_cell_count = static_cast<uint32_t>(
        (max_cell_x - min_cell_x + 1) * (max_cell_y - min_cell_y + 1));
  }

  // Reset transient hover highlights each frame
  highlights_.has_hovered_lot = false;
  highlights_.has_hovered_building = false;

  if (config.show_roads) {
    RenderRoadNetwork(gs);
  }

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
    RenderCompassGimbalHUD(config.compass_parented, config.compass_center,
                           config.compass_radius);
  }
  RenderFlightDeckHUD(gs);
  RenderSelectionOutlines(gs);
  RenderHighlights();

  telemetry.fallback_full_scan_count = scratch_fallback_full_scan_count_;
  telemetry.deduped_handles = scratch_deduped_handles_by_layer_;
}

void ViewportOverlays::RenderZoneColors(
    const RogueCity::Core::Editor::GlobalState &gs) {
  auto draw_district = [&](const RogueCity::Core::District &district) {
    if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::District,
                            district.id)) {
      return;
    }
    if (district.border.size() < 3) {
      return;
    }
    glm::vec4 color = DistrictColorScheme::GetColorForType(district.type);
    color.a *= LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::District,
                            district.id);
    DrawPolygon(district.border, color);
  };

  int min_cell_x = 0;
  int max_cell_x = 0;
  int min_cell_y = 0;
  int max_cell_y = 0;
  if (ComputeVisibleCellRange(gs, min_cell_x, max_cell_x, min_cell_y,
                              max_cell_y)) {
    const auto &spatial = gs.render_spatial_grid;
    BeginLayerDedupePass(
        RogueCity::Core::Editor::ViewportRenderLayer::ZoneColors,
        gs.districts.indexCount());
    for (int y = min_cell_y; y <= max_cell_y; ++y) {
      for (int x = min_cell_x; x <= max_cell_x; ++x) {
        const uint32_t cell = spatial.ToCellIndex(static_cast<uint32_t>(x),
                                                  static_cast<uint32_t>(y));
        const uint32_t begin = spatial.districts.offsets[cell];
        const uint32_t end = spatial.districts.offsets[cell + 1u];
        for (uint32_t cursor = begin; cursor < end; ++cursor) {
          const uint32_t handle = spatial.districts.handles[cursor];
          if (handle >= gs.districts.indexCount() ||
              !gs.districts.isValidIndex(handle)) {
            continue;
          }
          if (!MarkHandleSeen(handle)) {
            continue;
          }
          draw_district(gs.districts[handle]);
        }
      }
    }
    return;
  }

  RecordFallbackFullScan();
  for (const auto &district : gs.districts) {
    draw_district(district);
  }
}

namespace {
float SnapNiceMeters(float meters) {
  if (meters <= 0.0f)
    return 0.0f;
  const float base = std::pow(10.0f, std::floor(std::log10(meters)));
  const float mant = meters / base;
  float nice = 1.0f;
  if (mant < 1.5f)
    nice = 1.0f;
  else if (mant < 3.5f)
    nice = 2.0f;
  else if (mant < 7.5f)
    nice = 5.0f;
  else
    nice = 10.0f;
  return nice * base;
}

void FormatDistance(char *out, size_t out_sz, float meters) {
  if (meters >= 1000.0f) {
    const float km = meters / 1000.0f;
    std::snprintf(out, out_sz, "%.2f km", km);
  } else {
    if (meters >= 100.0f)
      std::snprintf(out, out_sz, "%.0f m", meters);
    else if (meters >= 10.0f)
      std::snprintf(out, out_sz, "%.1f m", meters);
    else
      std::snprintf(out, out_sz, "%.2f m", meters);
  }
}
} // namespace

void ViewportOverlays::RenderScaleRulerHUD(
    const RogueCity::Core::Editor::GlobalState &gs) {
  ImDrawList *dl = ImGui::GetWindowDrawList();
  const float padding = 16.0f;
  const float minimap_h = 180.0f + 12.0f; // kMinimapSize + kMinimapPadding
  const ImVec2 base_pt =
      ImVec2(view_transform_.viewport_pos.x + view_transform_.viewport_size.x -
                 padding,
             view_transform_.viewport_pos.y + minimap_h + 32.0f);

  const double mpp =
      gs.HasTextureSpace()
          ? gs.TextureSpaceRef().coordinateSystem().metersPerPixel()
          : gs.city_meters_per_pixel;
  const float ppm =
      std::max(1e-4f, view_transform_.zoom / static_cast<float>(mpp));
  const float target_px = 140.0f;
  const float meters = target_px / ppm;
  const float nice_m = SnapNiceMeters(meters);
  const float px_len = std::max(6.0f, nice_m * ppm);

  const ImU32 col = TokenColor(UITokens::TextSecondary, 200u);
  const ImVec2 a = ImVec2(base_pt.x - px_len, base_pt.y - 6.0f);
  const ImVec2 b = ImVec2(base_pt.x, base_pt.y - 6.0f);
  dl->AddLine(a, b, col, 2.5f);
  dl->AddLine(ImVec2(a.x, a.y - 6.0f), ImVec2(a.x, a.y + 6.0f), col, 2.0f);
  dl->AddLine(ImVec2(b.x, b.y - 6.0f), ImVec2(b.x, b.y + 6.0f), col, 2.0f);

  char buf[64];
  FormatDistance(buf, sizeof(buf), nice_m);
  ImVec2 text_sz = ImGui::CalcTextSize(buf);
  dl->AddText(ImVec2(b.x - text_sz.x, a.y - 20.0f), col, buf);
}

void ViewportOverlays::RenderFlightDeckHUD(
    const RogueCity::Core::Editor::GlobalState &gs) {
  ImDrawList *dl = ImGui::GetWindowDrawList();

  // Layout constants
  const float bar_height = 32.0f;
  const float margin_x = 12.0f;
  const float margin_y = 12.0f;

  // Anchor to bottom of viewport
  const ImVec2 vp_pos = view_transform_.viewport_pos;
  const ImVec2 vp_size = view_transform_.viewport_size;

  const ImVec2 bar_pos =
      ImVec2(vp_pos.x + margin_x, vp_pos.y + vp_size.y - bar_height - margin_y);
  const ImVec2 bar_size = ImVec2(vp_size.x - margin_x * 2.0f, bar_height);
  const ImVec2 bar_end = ImVec2(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y);

  // Glass-style background
  dl->AddRectFilled(bar_pos, bar_end, IM_COL32(10, 12, 16, 220), 6.0f);
  dl->AddRect(bar_pos, bar_end, IM_COL32(255, 255, 255, 30), 6.0f);

  static bool s_terminal_mode_active = false;
  static char s_terminal_input_buf[256] = "";

  // 1. Mode Indicator (Capsule) - Terminal Toggle
  const char *mode_str = "[_>|]";
  const ImVec2 mode_pad(12.0f, 4.0f);
  const ImVec2 mode_text_sz = ImGui::CalcTextSize(mode_str);
  const float mode_w = mode_text_sz.x + mode_pad.x * 2.0f;
  const ImVec2 mode_min(bar_pos.x + 4.0f, bar_pos.y + 4.0f);
  const ImVec2 mode_max(mode_min.x + mode_w, bar_end.y - 4.0f);

  const ImVec2 mp = ImGui::GetIO().MousePos;
  const bool hovering_mode = mp.x >= mode_min.x && mp.x <= mode_max.x &&
                             mp.y >= mode_min.y && mp.y <= mode_max.y;

  if (hovering_mode && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    s_terminal_mode_active = !s_terminal_mode_active;
  }

  dl->AddRectFilled(mode_min, mode_max,
                    s_terminal_mode_active ? IM_COL32(180, 0, 0, 200)
                                           : IM_COL32(0, 180, 180, 200),
                    4.0f);
  dl->AddText(ImVec2(mode_min.x + mode_pad.x, mode_min.y + mode_pad.y - 1.0f),
              IM_COL32(10, 10, 10, 255), mode_str);

  // 2. Context Cue / Terminal Input
  const float input_start_x = mode_max.x + 16.0f;
  if (s_terminal_mode_active) {
    ImGui::SetCursorScreenPos(ImVec2(input_start_x, bar_pos.y + 4.0f));
    ImGui::PushItemWidth(bar_end.x - input_start_x - 16.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 100));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
    if (ImGui::InputText("##terminal_input", s_terminal_input_buf,
                         sizeof(s_terminal_input_buf),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
      // Handle enter -> dispatch command, clear, close terminal
      RogueCity::Core::Editor::GlobalState &mutable_gs =
          const_cast<RogueCity::Core::Editor::GlobalState &>(gs);
      mutable_gs.tool_runtime.last_viewport_status =
          std::string("Executed: ") + s_terminal_input_buf;
      mutable_gs.tool_runtime.last_viewport_status_frame =
          mutable_gs.frame_counter;
      s_terminal_input_buf[0] = '\0'; // clear
      s_terminal_mode_active = false;
    }
    ImGui::PopStyleColor(2);
    ImGui::PopItemWidth();
    if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() &&
        !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      ImGui::SetKeyboardFocusHere(-1); // Auto-focus
    }
  } else {
    const char *cue_text = "Ready";
    ImU32 text_col = TokenColor(UITokens::TextPrimary, 220u);

    const auto &events = gs.infomatrix.events().data;
    if (!events.empty()) {
      const auto &ev = events.back();
      cue_text = ev.msg.c_str();
      using RogueCity::Core::Editor::InfomatrixEvent;
      if (ev.cat == InfomatrixEvent::Category::Runtime) {
        text_col = TokenColor(UITokens::CyanAccent, 255u);
      } else if (ev.cat == InfomatrixEvent::Category::Validation) {
        text_col = ev.msg.find("rejected") != std::string::npos
                       ? TokenColor(UITokens::ErrorRed, 255u)
                       : TokenColor(UITokens::GreenHUD, 255u);
      } else if (ev.cat == InfomatrixEvent::Category::Dirty) {
        text_col = TokenColor(UITokens::YellowWarning, 255u);
      }
    } else if (!gs.tool_runtime.last_action_status.empty()) {
      cue_text = gs.tool_runtime.last_action_status.c_str();
    } else if (!gs.tool_runtime.last_viewport_status.empty()) {
      cue_text = gs.tool_runtime.last_viewport_status.c_str();
    }

    if (!gs.tool_runtime.viewport_warning_text.empty() &&
        gs.tool_runtime.viewport_warning_ttl_seconds > 0.0f) {
      cue_text = gs.tool_runtime.viewport_warning_text.c_str();
      text_col = TokenColor(UITokens::AmberGlow, 255u);
    }
    const ImVec2 cue_pos(input_start_x, bar_pos.y + 8.0f);
    dl->AddText(cue_pos, text_col, cue_text);
  }
}

void ViewportOverlays::RenderCompassGimbalHUD(bool parented,
                                              const ImVec2 &center,
                                              float radius) {
  ImDrawList *dl = ImGui::GetWindowDrawList();
  const float padding = 16.0f;
  const float r = std::max(18.0f, radius);
  const ImVec2 hud_center =
      parented ? center
               : ImVec2(view_transform_.viewport_pos.x +
                            view_transform_.viewport_size.x - padding - r,
                        view_transform_.viewport_pos.y + padding + r);
  const ImU32 bg = IM_COL32(24, 24, 28, 220);
  const ImU32 ring = IM_COL32(240, 240, 240, 200);
  dl->AddCircleFilled(hud_center, r, bg);
  dl->AddCircle(hud_center, r, ring, 24, 2.0f);

  const float ang = -view_transform_.yaw;
  const float ca = std::cos(ang);
  const float sa = std::sin(ang);
  auto rotate_dir = [ca, sa](const ImVec2 &v) -> ImVec2 {
    return ImVec2(v.x * ca - v.y * sa, v.x * sa + v.y * ca);
  };

  const ImVec2 dir_n = rotate_dir(ImVec2(0.0f, -1.0f));
  const ImVec2 dir_e = rotate_dir(ImVec2(1.0f, 0.0f));
  const ImVec2 dir_s = rotate_dir(ImVec2(0.0f, 1.0f));
  const ImVec2 dir_w = rotate_dir(ImVec2(-1.0f, 0.0f));

  const float label_radius = std::max(8.0f, r - 10.0f);
  const ImU32 label_col = IM_COL32(220, 220, 220, 220);
  auto draw_label = [&](const char *text, const ImVec2 &dir) {
    const ImVec2 text_size = ImGui::CalcTextSize(text);
    const ImVec2 pos(hud_center.x + dir.x * label_radius - text_size.x * 0.5f,
                     hud_center.y + dir.y * label_radius - text_size.y * 0.5f);
    dl->AddText(pos, label_col, text);
    return pos;
  };
  const ImVec2 north_label_pos = draw_label("N", dir_n);
  draw_label("E", dir_e);
  draw_label("S", dir_s);
  draw_label("W", dir_w);

  // North needle (red).
  const ImVec2 needle_tip = ImVec2(hud_center.x + dir_n.x * (r - 10.0f),
                                   hud_center.y + dir_n.y * (r - 10.0f));
  dl->AddLine(hud_center, needle_tip, IM_COL32(220, 60, 60, 240), 3.0f);

  // Interaction: click + drag on compass ring to steer camera yaw.
  const ImVec2 mp = ImGui::GetIO().MousePos;
  const float dx = mp.x - hud_center.x;
  const float dy = mp.y - hud_center.y;
  const float dist2 = dx * dx + dy * dy;
  const bool inside = dist2 <= r * r;

  if (ImGui::IsWindowHovered() &&
      ImGui::IsMouseClicked(ImGuiMouseButton_Left) && inside) {
    const ImVec2 north_center =
        ImVec2(north_label_pos.x + ImGui::CalcTextSize("N").x * 0.5f,
               north_label_pos.y + ImGui::CalcTextSize("N").y * 0.5f);
    const float ndx = mp.x - north_center.x;
    const float ndy = mp.y - north_center.y;
    const float near_n_dist2 = ndx * ndx + ndy * ndy;
    const bool near_north = near_n_dist2 <= (10.0f * 10.0f);
    if (near_north) {
      requested_yaw_ = 0.0f;
      compass_drag_active_ = false;
    } else {
      requested_yaw_ = std::atan2(dy, dx) + (3.14159265f * 0.5f);
      compass_drag_active_ = true;
    }
  }
  if (compass_drag_active_ && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    compass_drag_active_ = false;
  }
  if (compass_drag_active_) {
    const float ang_click = std::atan2(dy, dx);
    const float desired = ang_click + (3.14159265f * 0.5f);
    requested_yaw_ = desired;
  }
}

void ViewportOverlays::RenderAESPHeatmap(
    const RogueCity::Core::Editor::GlobalState &gs,
    OverlayConfig::AESPComponent component) {
  const ViewportLOD lod = ResolveViewportLOD(view_transform_.zoom);
  if (!ShouldRenderLotsForLOD(gs, lod)) {
    return;
  }

  auto draw_lot = [&](const RogueCity::Core::LotToken &lot) {
    if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Lot,
                            lot.id)) {
      return;
    }

    float score = 0.0f;
    switch (component) {
    case OverlayConfig::AESPComponent::Access:
      score = lot.access;
      break;
    case OverlayConfig::AESPComponent::Exposure:
      score = lot.exposure;
      break;
    case OverlayConfig::AESPComponent::Service:
      score = lot.serviceability;
      break;
    case OverlayConfig::AESPComponent::Privacy:
      score = lot.privacy;
      break;
    case OverlayConfig::AESPComponent::Combined:
    default:
      score = lot.aespScore();
      break;
    }

    glm::vec4 color = GetAESPGradientColor(score);
    color.a *=
        LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::Lot, lot.id);
    if (!lot.boundary.empty()) {
      DrawPolygon(lot.boundary, color);
      return;
    }

    const ImVec2 pos = WorldToScreen(lot.centroid);
    ImGui::GetWindowDrawList()->AddCircleFilled(
        pos, std::max(2.0f, 3.0f * view_transform_.zoom),
        ImGui::ColorConvertFloat4ToU32(
            ImVec4(color.r, color.g, color.b, color.a)));
  };

  int min_cell_x = 0;
  int max_cell_x = 0;
  int min_cell_y = 0;
  int max_cell_y = 0;
  if (ComputeVisibleCellRange(gs, min_cell_x, max_cell_x, min_cell_y,
                              max_cell_y)) {
    const auto &spatial = gs.render_spatial_grid;
    BeginLayerDedupePass(
        RogueCity::Core::Editor::ViewportRenderLayer::AESPHeatmap,
        gs.lots.indexCount());
    for (int y = min_cell_y; y <= max_cell_y; ++y) {
      for (int x = min_cell_x; x <= max_cell_x; ++x) {
        const uint32_t cell = spatial.ToCellIndex(static_cast<uint32_t>(x),
                                                  static_cast<uint32_t>(y));
        const uint32_t begin = spatial.lots.offsets[cell];
        const uint32_t end = spatial.lots.offsets[cell + 1u];
        for (uint32_t cursor = begin; cursor < end; ++cursor) {
          const uint32_t handle = spatial.lots.handles[cursor];
          if (handle >= gs.lots.indexCount() || !gs.lots.isValidIndex(handle)) {
            continue;
          }
          if (!MarkHandleSeen(handle)) {
            continue;
          }
          draw_lot(gs.lots[handle]);
        }
      }
    }
    return;
  }

  RecordFallbackFullScan();
  for (const auto &lot : gs.lots) {
    draw_lot(lot);
  }
}

void ViewportOverlays::RenderRoadNetwork(
    const RogueCity::Core::Editor::GlobalState &gs) {
  const ViewportLOD lod = ResolveViewportLOD(view_transform_.zoom);
  const bool force_roads_visible =
      IsRoadDomainActive(gs.tool_runtime.active_domain);
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  auto draw_road = [&](const RogueCity::Core::Road &road) {
    if (road.points.size() < 2) {
      return;
    }
    const RoadRenderStyle style = ResolveRoadRenderStyle(road.type);
    scratch_screen_points_.clear();
    scratch_screen_points_.reserve(road.points.size());
    for (const auto &point : road.points) {
      scratch_screen_points_.push_back(WorldToScreen(point));
    }
    draw_list->AddPolyline(scratch_screen_points_.data(),
                           static_cast<int>(scratch_screen_points_.size()),
                           style.color, false, style.width);
  };

  int min_cell_x = 0;
  int max_cell_x = 0;
  int min_cell_y = 0;
  int max_cell_y = 0;
  if (ComputeVisibleCellRange(gs, min_cell_x, max_cell_x, min_cell_y,
                              max_cell_y)) {
    const auto &spatial = gs.render_spatial_grid;
    BeginLayerDedupePass(
        RogueCity::Core::Editor::ViewportRenderLayer::RoadNetwork,
        gs.roads.indexCount());
    for (int y = min_cell_y; y <= max_cell_y; ++y) {
      for (int x = min_cell_x; x <= max_cell_x; ++x) {
        const uint32_t cell = spatial.ToCellIndex(static_cast<uint32_t>(x),
                                                  static_cast<uint32_t>(y));
        const uint32_t begin = spatial.roads.offsets[cell];
        const uint32_t end = spatial.roads.offsets[cell + 1u];
        for (uint32_t cursor = begin; cursor < end; ++cursor) {
          const uint32_t handle = spatial.roads.handles[cursor];
          if (handle >= gs.roads.indexCount() ||
              !gs.roads.isValidIndex(handle)) {
            continue;
          }
          if (!MarkHandleSeen(handle)) {
            continue;
          }
          const auto &road = gs.roads[handle];
          if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Road,
                                  road.id)) {
            continue;
          }
          if (!force_roads_visible && !ShouldRenderRoadForLOD(road.type, lod)) {
            continue;
          }
          draw_road(road);
        }
      }
    }
    return;
  }

  RecordFallbackFullScan();
  for (const auto &road : gs.roads) {
    if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Road,
                            road.id)) {
      continue;
    }
    if (!force_roads_visible && !ShouldRenderRoadForLOD(road.type, lod)) {
      continue;
    }
    draw_road(road);
  }
}

void ViewportOverlays::RenderRoadLabels(
    const RogueCity::Core::Editor::GlobalState &gs) {
  const ViewportLOD lod = ResolveViewportLOD(view_transform_.zoom);
  const bool force_roads_visible =
      IsRoadDomainActive(gs.tool_runtime.active_domain);

  auto draw_label = [&](const RogueCity::Core::Road &road) {
    if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Road,
                            road.id)) {
      return;
    }
    if (road.points.empty()) {
      return;
    }
    if (!force_roads_visible && !ShouldRenderRoadForLOD(road.type, lod)) {
      return;
    }

    const RogueCity::Core::Vec2 midpoint = road.points[road.points.size() / 2];
    const char *type_name = "Road";
    using RogueCity::Core::RoadType;
    switch (road.type) {
    case RoadType::Highway:
      type_name = "Highway";
      break;
    case RoadType::Arterial:
      type_name = "Arterial";
      break;
    case RoadType::Avenue:
      type_name = "Avenue";
      break;
    case RoadType::Boulevard:
      type_name = "Boulevard";
      break;
    case RoadType::Street:
      type_name = "Street";
      break;
    case RoadType::Lane:
      type_name = "Lane";
      break;
    case RoadType::Alleyway:
      type_name = "Alley";
      break;
    default:
      break;
    }

    const float opacity =
        LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::Road, road.id);
    DrawWorldText(midpoint, type_name,
                  glm::vec4(1.0f, 1.0f, 1.0f, 0.8f * opacity));
  };

  int min_cell_x = 0;
  int max_cell_x = 0;
  int min_cell_y = 0;
  int max_cell_y = 0;
  if (ComputeVisibleCellRange(gs, min_cell_x, max_cell_x, min_cell_y,
                              max_cell_y)) {
    const auto &spatial = gs.render_spatial_grid;
    BeginLayerDedupePass(
        RogueCity::Core::Editor::ViewportRenderLayer::RoadLabels,
        gs.roads.indexCount());
    for (int y = min_cell_y; y <= max_cell_y; ++y) {
      for (int x = min_cell_x; x <= max_cell_x; ++x) {
        const uint32_t cell = spatial.ToCellIndex(static_cast<uint32_t>(x),
                                                  static_cast<uint32_t>(y));
        const uint32_t begin = spatial.roads.offsets[cell];
        const uint32_t end = spatial.roads.offsets[cell + 1u];
        for (uint32_t cursor = begin; cursor < end; ++cursor) {
          const uint32_t handle = spatial.roads.handles[cursor];
          if (handle >= gs.roads.indexCount() ||
              !gs.roads.isValidIndex(handle)) {
            continue;
          }
          if (!MarkHandleSeen(handle)) {
            continue;
          }
          draw_label(gs.roads[handle]);
        }
      }
    }
    return;
  }

  RecordFallbackFullScan();
  for (const auto &road : gs.roads) {
    draw_label(road);
  }
}

void ViewportOverlays::RenderBudgetIndicators(
    const RogueCity::Core::Editor::GlobalState &gs) {
  std::unordered_map<uint32_t, float> budget_by_district;
  const uint64_t lot_count = gs.lots.size();
  const uint64_t reserve_cap = std::min<uint64_t>(
      lot_count, static_cast<uint64_t>(std::numeric_limits<size_t>::max()));
  budget_by_district.reserve(static_cast<size_t>(reserve_cap));
  for (const auto &lot : gs.lots) {
    budget_by_district[lot.district_id] += lot.budget_allocation;
  }

  float max_budget = 0.0f;
  for (const auto &district : gs.districts) {
    const float budget = district.budget_allocated > 0.0f
                             ? district.budget_allocated
                             : budget_by_district[district.id];
    max_budget = std::max(max_budget, budget);
  }
  max_budget = std::max(1.0f, max_budget);

  for (const auto &district : gs.districts) {
    if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::District,
                            district.id)) {
      continue;
    }
    if (district.border.empty())
      continue;

    // Calculate district centroid
    RogueCity::Core::Vec2 centroid{};
    for (const auto &pt : district.border) {
      centroid.x += pt.x;
      centroid.y += pt.y;
    }
    centroid.x /= static_cast<double>(district.border.size());
    centroid.y /= static_cast<double>(district.border.size());

    const float budget = district.budget_allocated > 0.0f
                             ? district.budget_allocated
                             : budget_by_district[district.id];
    const float ratio = budget / max_budget;

    const float opacity = LayerOpacity(
        gs, RogueCity::Core::Editor::VpEntityKind::District, district.id);
    DrawBudgetBar(centroid, ratio, glm::vec4(0.9f, 0.8f, 0.2f, 0.9f * opacity),
                  glm::vec4(0.1f, 0.1f, 0.1f, 0.8f * opacity));

    char budget_label[64];
    std::snprintf(budget_label, sizeof(budget_label), "$%.0f", budget);
    DrawLabel(centroid, budget_label,
              glm::vec4(1.0f, 0.95f, 0.6f, 0.9f * opacity));
  }
}

void ViewportOverlays::RenderSlopeHeatmap(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (!gs.world_constraints.isValid()) {
    return;
  }

  const auto &constraints = gs.world_constraints;
  const int stride =
      std::max(1, std::max(constraints.width, constraints.height) / 90);
  const float radius = std::max(
      1.0f, WorldToScreenScale(static_cast<float>(
                constraints.cell_size * static_cast<double>(stride) * 0.22)));
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  for (int y = 0; y < constraints.height; y += stride) {
    for (int x = 0; x < constraints.width; x += stride) {
      const size_t idx = static_cast<size_t>(constraints.toIndex(x, y));
      const float slope = constraints.slope_degrees[idx];
      const float t = std::clamp(slope / 45.0f, 0.0f, 1.0f);
      if (t < 0.12f) {
        continue;
      }

      const ImVec4 color(0.15f + 0.85f * t, 0.85f - 0.65f * t,
                         0.20f + 0.10f * (1.0f - t), 0.12f + 0.35f * t);

      const RogueCity::Core::Vec2 center(
          (static_cast<double>(x) + 0.5) * constraints.cell_size,
          (static_cast<double>(y) + 0.5) * constraints.cell_size);
      draw_list->AddCircleFilled(WorldToScreen(center), radius,
                                 ImGui::ColorConvertFloat4ToU32(color));
    }
  }
}

void ViewportOverlays::RenderNoBuildMask(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (!gs.world_constraints.isValid()) {
    return;
  }

  const auto &constraints = gs.world_constraints;
  const int stride =
      std::max(1, std::max(constraints.width, constraints.height) / 120);
  const float radius = std::max(
      1.0f, WorldToScreenScale(static_cast<float>(
                constraints.cell_size * static_cast<double>(stride) * 0.20)));
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  for (int y = 0; y < constraints.height; y += stride) {
    for (int x = 0; x < constraints.width; x += stride) {
      const size_t idx = static_cast<size_t>(constraints.toIndex(x, y));
      if (constraints.no_build_mask[idx] == 0u) {
        continue;
      }

      const RogueCity::Core::Vec2 center(
          (static_cast<double>(x) + 0.5) * constraints.cell_size,
          (static_cast<double>(y) + 0.5) * constraints.cell_size);
      draw_list->AddCircleFilled(WorldToScreen(center), radius,
                                 IM_COL32(220, 60, 60, 135));
    }
  }
}

void ViewportOverlays::RenderNatureHeatmap(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (!gs.world_constraints.isValid()) {
    return;
  }

  const auto &constraints = gs.world_constraints;
  const int stride =
      std::max(1, std::max(constraints.width, constraints.height) / 95);
  const float radius = std::max(
      1.0f, WorldToScreenScale(static_cast<float>(
                constraints.cell_size * static_cast<double>(stride) * 0.18)));
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  for (int y = 0; y < constraints.height; y += stride) {
    for (int x = 0; x < constraints.width; x += stride) {
      const size_t idx = static_cast<size_t>(constraints.toIndex(x, y));
      const float nature = constraints.nature_score[idx];
      if (nature < 0.15f) {
        continue;
      }

      const ImVec4 color(0.10f + 0.18f * nature, 0.55f + 0.40f * nature,
                         0.18f + 0.12f * nature, 0.10f + 0.25f * nature);

      const RogueCity::Core::Vec2 center(
          (static_cast<double>(x) + 0.5) * constraints.cell_size,
          (static_cast<double>(y) + 0.5) * constraints.cell_size);
      draw_list->AddCircleFilled(WorldToScreen(center), radius,
                                 ImGui::ColorConvertFloat4ToU32(color));
    }
  }
}

void ViewportOverlays::RenderHeightField(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (!gs.HasTextureSpace()) {
    return;
  }

  const auto &texture_space = gs.TextureSpaceRef();
  const auto &height = texture_space.heightLayer();
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
  const float radius =
      std::max(1.0f, WorldToScreenScale(static_cast<float>(
                         texture_space.coordinateSystem().metersPerPixel() *
                         static_cast<double>(stride) * 0.32)));
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const auto &coords = texture_space.coordinateSystem();

  for (int y = 0; y < height.height(); y += stride) {
    for (int x = 0; x < height.width(); x += stride) {
      const float h = height.at(x, y);
      const float t = std::clamp((h - min_h) / range, 0.0f, 1.0f);
      const ImVec4 color(0.15f + 0.75f * t,
                         0.20f + 0.55f * (1.0f - std::abs(t - 0.5f) * 1.7f),
                         0.85f - 0.65f * t, 0.10f + 0.28f * t);
      const RogueCity::Core::Vec2 world = coords.pixelToWorld({x, y});
      draw_list->AddCircleFilled(WorldToScreen(world), radius,
                                 ImGui::ColorConvertFloat4ToU32(color));
    }
  }
}

void ViewportOverlays::RenderTensorField(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (!gs.HasTextureSpace()) {
    return;
  }

  const auto &texture_space = gs.TextureSpaceRef();
  const auto &tensor = texture_space.tensorLayer();
  if (tensor.empty()) {
    return;
  }

  const int stride = std::max(1, tensor.width() / 60);
  const auto &coords = texture_space.coordinateSystem();
  const double world_step =
      coords.metersPerPixel() * static_cast<double>(stride);
  const float half_len =
      std::max(2.0f, WorldToScreenScale(static_cast<float>(world_step * 0.35)));
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  for (int y = 0; y < tensor.height(); y += stride) {
    for (int x = 0; x < tensor.width(); x += stride) {
      const RogueCity::Core::Vec2 dir = tensor.at(x, y);
      const double mag_sq = dir.lengthSquared();
      if (mag_sq <= 1e-8) {
        continue;
      }

      const RogueCity::Core::Vec2 n = dir / std::sqrt(mag_sq);
      const RogueCity::Core::Vec2 world = coords.pixelToWorld({x, y});
      const ImVec2 center = WorldToScreen(world);
      const ImVec2 a(center.x - static_cast<float>(n.x) * half_len,
                     center.y - static_cast<float>(n.y) * half_len);
      const ImVec2 b(center.x + static_cast<float>(n.x) * half_len,
                     center.y + static_cast<float>(n.y) * half_len);

      const float angle = static_cast<float>(
          (std::atan2(n.y, n.x) + 3.14159265) / (2.0 * 3.14159265));
      const ImVec4 color(0.2f + 0.7f * angle, 0.8f - 0.5f * angle,
                         0.9f - 0.7f * std::abs(angle - 0.5f), 0.8f);
      draw_list->AddLine(a, b, ImGui::ColorConvertFloat4ToU32(color), 1.4f);
    }
  }
}

void ViewportOverlays::RenderZoneField(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (!gs.HasTextureSpace()) {
    return;
  }

  const auto &texture_space = gs.TextureSpaceRef();
  const auto &zone = texture_space.zoneLayer();
  if (zone.empty()) {
    return;
  }

  const int stride = std::max(1, zone.width() / 110);
  const float radius =
      std::max(1.0f, WorldToScreenScale(static_cast<float>(
                         texture_space.coordinateSystem().metersPerPixel() *
                         static_cast<double>(stride) * 0.30)));
  const auto &coords = texture_space.coordinateSystem();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  auto zoneColor = [](uint8_t zone_value) {
    if (zone_value == 0u) {
      return ImVec4(0.45f, 0.45f, 0.45f, 0.18f);
    }
    const uint8_t district_value = static_cast<uint8_t>(zone_value - 1u);
    switch (static_cast<RogueCity::Core::DistrictType>(district_value)) {
    case RogueCity::Core::DistrictType::Residential:
      return ImVec4(0.28f, 0.52f, 0.95f, 0.24f);
    case RogueCity::Core::DistrictType::Commercial:
      return ImVec4(0.20f, 0.82f, 0.46f, 0.24f);
    case RogueCity::Core::DistrictType::Industrial:
      return ImVec4(0.93f, 0.34f, 0.30f, 0.26f);
    case RogueCity::Core::DistrictType::Civic:
      return ImVec4(0.92f, 0.76f, 0.30f, 0.26f);
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

      const RogueCity::Core::Vec2 world = coords.pixelToWorld({x, y});
      const ImVec4 color = zoneColor(zone_value);
      draw_list->AddCircleFilled(WorldToScreen(world), radius,
                                 ImGui::ColorConvertFloat4ToU32(color));
    }
  }
}

void ViewportOverlays::DrawPolygon(
    const std::vector<RogueCity::Core::Vec2> &points, const glm::vec4 &color) {
  if (points.size() < 3) {
    return;
  }

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  scratch_screen_points_.clear();
  scratch_screen_points_.reserve(points.size());
  for (const auto &pt : points) {
    scratch_screen_points_.push_back(WorldToScreen(pt));
  }

  const ImU32 fill = ImGui::ColorConvertFloat4ToU32(
      ImVec4(color.r, color.g, color.b, color.a));
  const ImU32 outline = ImGui::ColorConvertFloat4ToU32(
      ImVec4(color.r, color.g, color.b, std::min(color.a + 0.2f, 1.0f)));
  scratch_triangle_indices_.clear();
  if (TriangulateSimplePolygon(scratch_screen_points_,
                               scratch_triangle_indices_)) {
    for (size_t i = 0; i + 2 < scratch_triangle_indices_.size(); i += 3) {
      const ImVec2 &a = scratch_screen_points_[scratch_triangle_indices_[i]];
      const ImVec2 &b =
          scratch_screen_points_[scratch_triangle_indices_[i + 1]];
      const ImVec2 &c =
          scratch_screen_points_[scratch_triangle_indices_[i + 2]];
      draw_list->AddTriangleFilled(a, b, c, fill);
    }
  } else {
    draw_list->AddConvexPolyFilled(
        scratch_screen_points_.data(),
        static_cast<int>(scratch_screen_points_.size()), fill);
  }
  draw_list->AddPolyline(scratch_screen_points_.data(),
                         static_cast<int>(scratch_screen_points_.size()),
                         outline, true, 1.0f);
}

void ViewportOverlays::DrawWorldText(const RogueCity::Core::Vec2 &pos,
                                     const char *text, const glm::vec4 &color) {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2 screen = WorldToScreen(pos);
  draw_list->AddText(screen,
                     ImGui::ColorConvertFloat4ToU32(
                         ImVec4(color.r, color.g, color.b, color.a)),
                     text);
}

void ViewportOverlays::DrawLabel(const RogueCity::Core::Vec2 &pos,
                                 const char *text, const glm::vec4 &color) {
  DrawWorldText(pos, text, color);
}

void ViewportOverlays::DrawBudgetBar(const RogueCity::Core::Vec2 &pos,
                                     float ratio, const glm::vec4 &fill,
                                     const glm::vec4 &outline) {
  const ImVec2 screen = WorldToScreen(pos);
  const float width = 60.0f;
  const float height = 6.0f;
  const ImVec2 min(screen.x - width * 0.5f, screen.y + 10.0f);
  const ImVec2 max(screen.x + width * 0.5f, screen.y + 10.0f + height);
  const ImVec2 fill_max(min.x + width * std::clamp(ratio, 0.0f, 1.0f), max.y);

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(min, max,
                           ImGui::ColorConvertFloat4ToU32(ImVec4(
                               outline.r, outline.g, outline.b, outline.a)));
  draw_list->AddRectFilled(
      min, fill_max,
      ImGui::ColorConvertFloat4ToU32(ImVec4(fill.r, fill.g, fill.b, fill.a)));
}

glm::vec4 ViewportOverlays::GetAESPGradientColor(float score) {
  // Gradient: Blue (low) ? Green ? Yellow ? Red (high)
  if (score < 0.25f) {
    float t = score / 0.25f;
    return glm::mix(glm::vec4(0.0f, 0.0f, 1.0f, 0.6f),
                    glm::vec4(0.0f, 1.0f, 0.0f, 0.6f), t);
  } else if (score < 0.5f) {
    float t = (score - 0.25f) / 0.25f;
    return glm::mix(glm::vec4(0.0f, 1.0f, 0.0f, 0.6f),
                    glm::vec4(1.0f, 1.0f, 0.0f, 0.6f), t);
  } else if (score < 0.75f) {
    float t = (score - 0.5f) / 0.25f;
    return glm::mix(glm::vec4(1.0f, 1.0f, 0.0f, 0.6f),
                    glm::vec4(1.0f, 0.5f, 0.0f, 0.6f), t);
  } else {
    float t = (score - 0.75f) / 0.25f;
    return glm::mix(glm::vec4(1.0f, 0.5f, 0.0f, 0.6f),
                    glm::vec4(1.0f, 0.0f, 0.0f, 0.6f), t);
  }
}

ImVec2
ViewportOverlays::WorldToScreen(const RogueCity::Core::Vec2 &world_pos) const {
  const ImVec2 vp_pos = view_transform_.viewport_pos;
  const ImVec2 vp_size = view_transform_.viewport_size;

  const RogueCity::Core::Vec2 rel(world_pos.x - view_transform_.camera_xy.x,
                                  world_pos.y - view_transform_.camera_xy.y);
  const float c = std::cos(view_transform_.yaw);
  const float s = std::sin(view_transform_.yaw);
  // Keep overlay projection consistent with PrimaryViewport::world_to_screen
  // (R(-yaw)).
  const RogueCity::Core::Vec2 rotated(rel.x * c + rel.y * s,
                                      -rel.x * s + rel.y * c);

  return ImVec2(vp_pos.x + vp_size.x * 0.5f +
                    static_cast<float>(rotated.x) * view_transform_.zoom,
                vp_pos.y + vp_size.y * 0.5f +
                    static_cast<float>(rotated.y) * view_transform_.zoom);
}

RogueCity::Core::Vec2
ViewportOverlays::ScreenToWorld(const ImVec2 &screen_pos) const {
  if (view_transform_.viewport_size.x <= 0.0f ||
      view_transform_.viewport_size.y <= 0.0f ||
      view_transform_.zoom <= 1e-6f) {
    return view_transform_.camera_xy;
  }

  const ImVec2 viewport_min = view_transform_.viewport_pos;
  const float nx =
      (screen_pos.x - viewport_min.x) / view_transform_.viewport_size.x - 0.5f;
  const float ny =
      (screen_pos.y - viewport_min.y) / view_transform_.viewport_size.y - 0.5f;
  const RogueCity::Core::Vec2 view(
      static_cast<double>(nx * view_transform_.viewport_size.x /
                          view_transform_.zoom),
      static_cast<double>(ny * view_transform_.viewport_size.y /
                          view_transform_.zoom));

  const float c = std::cos(view_transform_.yaw);
  const float s = std::sin(view_transform_.yaw);
  const RogueCity::Core::Vec2 rel(view.x * c - view.y * s,
                                  view.x * s + view.y * c);
  return RogueCity::Core::Vec2(view_transform_.camera_xy.x + rel.x,
                               view_transform_.camera_xy.y + rel.y);
}

bool ViewportOverlays::ComputeVisibleCellRange(
    const RogueCity::Core::Editor::GlobalState &gs, int &out_min_cell_x,
    int &out_max_cell_x, int &out_min_cell_y, int &out_max_cell_y) const {
  const auto &spatial = gs.render_spatial_grid;
  if (!spatial.IsValid()) {
    return false;
  }

  const ImVec2 viewport_min = view_transform_.viewport_pos;
  const ImVec2 viewport_max =
      ImVec2(viewport_min.x + view_transform_.viewport_size.x,
             viewport_min.y + view_transform_.viewport_size.y);

  const std::array<RogueCity::Core::Vec2, 4> corners{
      ScreenToWorld(viewport_min),
      ScreenToWorld(ImVec2(viewport_max.x, viewport_min.y)),
      ScreenToWorld(viewport_max),
      ScreenToWorld(ImVec2(viewport_min.x, viewport_max.y))};

  double world_min_x = corners[0].x;
  double world_max_x = corners[0].x;
  double world_min_y = corners[0].y;
  double world_max_y = corners[0].y;
  for (const auto &corner : corners) {
    world_min_x = std::min(world_min_x, corner.x);
    world_max_x = std::max(world_max_x, corner.x);
    world_min_y = std::min(world_min_y, corner.y);
    world_max_y = std::max(world_max_y, corner.y);
  }

  const bool overlaps_world = !(world_max_x < spatial.world_bounds.min.x ||
                                world_min_x > spatial.world_bounds.max.x ||
                                world_max_y < spatial.world_bounds.min.y ||
                                world_min_y > spatial.world_bounds.max.y);
  if (!overlaps_world) {
    return false;
  }

  out_min_cell_x = std::clamp(
      static_cast<int>(std::floor((world_min_x - spatial.world_bounds.min.x) /
                                  spatial.cell_size_x)),
      0, static_cast<int>(spatial.width) - 1);
  out_max_cell_x = std::clamp(
      static_cast<int>(std::floor((world_max_x - spatial.world_bounds.min.x) /
                                  spatial.cell_size_x)),
      0, static_cast<int>(spatial.width) - 1);
  out_min_cell_y = std::clamp(
      static_cast<int>(std::floor((world_min_y - spatial.world_bounds.min.y) /
                                  spatial.cell_size_y)),
      0, static_cast<int>(spatial.height) - 1);
  out_max_cell_y = std::clamp(
      static_cast<int>(std::floor((world_max_y - spatial.world_bounds.min.y) /
                                  spatial.cell_size_y)),
      0, static_cast<int>(spatial.height) - 1);
  return true;
}

void ViewportOverlays::BeginLayerDedupePass(
    RogueCity::Core::Editor::ViewportRenderLayer layer, size_t required_size) {
  scratch_active_dedupe_layer_ = layer;
  if (scratch_dedupe_stamps_.size() < required_size) {
    scratch_dedupe_stamps_.resize(required_size, 0u);
  }

  scratch_dedupe_epoch_ += 1u;
  if (scratch_dedupe_epoch_ == 0u) {
    std::fill(scratch_dedupe_stamps_.begin(), scratch_dedupe_stamps_.end(), 0u);
    scratch_dedupe_epoch_ = 1u;
  }
}

bool ViewportOverlays::MarkHandleSeen(uint32_t handle) {
  if (handle >= scratch_dedupe_stamps_.size()) {
    return false;
  }
  if (scratch_dedupe_stamps_[handle] == scratch_dedupe_epoch_) {
    return false;
  }
  scratch_dedupe_stamps_[handle] = scratch_dedupe_epoch_;
  if (scratch_active_dedupe_layer_.has_value()) {
    const size_t index = static_cast<size_t>(*scratch_active_dedupe_layer_);
    if (index < scratch_deduped_handles_by_layer_.size()) {
      scratch_deduped_handles_by_layer_[index] += 1u;
    }
  }
  return true;
}

void ViewportOverlays::RecordFallbackFullScan() {
  scratch_active_dedupe_layer_.reset();
  scratch_fallback_full_scan_count_ += 1u;
}

float ViewportOverlays::WorldToScreenScale(float world_distance) const {
  return world_distance * view_transform_.zoom;
}

void ViewportOverlays::RenderHighlights() {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
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
    draw_list->AddRect(ImVec2(pos.x - 6, pos.y - 6),
                       ImVec2(pos.x + 6, pos.y + 6),
                       IM_COL32(255, 180, 80, 220), 2.0f, 0, 2.0f);
  }
  if (highlights_.has_hovered_building) {
    const ImVec2 pos = WorldToScreen(highlights_.hovered_building_pos);
    draw_list->AddRect(ImVec2(pos.x - 5, pos.y - 5),
                       ImVec2(pos.x + 5, pos.y + 5),
                       IM_COL32(120, 255, 180, 200), 2.0f, 0, 2.0f);
  }
}

void ViewportOverlays::RenderSelectionOutlines(
    const RogueCity::Core::Editor::GlobalState &gs) {
  const auto pulse =
      0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 2.0f *
                             3.1415926f * 2.0f);
  const float thickness = 1.5f + pulse * 1.75f;
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  auto draw_road = [&](const RogueCity::Core::Road &road, const ImU32 color) {
    if (road.points.size() < 2) {
      return;
    }
    scratch_screen_points_.clear();
    scratch_screen_points_.reserve(road.points.size());
    for (const auto &point : road.points) {
      scratch_screen_points_.push_back(WorldToScreen(point));
    }
    draw_list->AddPolyline(scratch_screen_points_.data(),
                           static_cast<int>(scratch_screen_points_.size()),
                           color, false, thickness);
  };

  auto draw_district = [&](const RogueCity::Core::District &district,
                           const ImU32 color) {
    if (district.border.size() < 3) {
      return;
    }
    scratch_screen_points_.clear();
    scratch_screen_points_.reserve(district.border.size());
    for (const auto &point : district.border) {
      scratch_screen_points_.push_back(WorldToScreen(point));
    }
    draw_list->AddPolyline(scratch_screen_points_.data(),
                           static_cast<int>(scratch_screen_points_.size()),
                           color, true, thickness);
  };

  auto draw_lot = [&](const RogueCity::Core::LotToken &lot, const ImU32 color) {
    if (lot.boundary.size() >= 3) {
      scratch_screen_points_.clear();
      scratch_screen_points_.reserve(lot.boundary.size());
      for (const auto &point : lot.boundary) {
        scratch_screen_points_.push_back(WorldToScreen(point));
      }
      draw_list->AddPolyline(scratch_screen_points_.data(),
                             static_cast<int>(scratch_screen_points_.size()),
                             color, true, thickness);
      return;
    }
    draw_list->AddCircle(WorldToScreen(lot.centroid), 10.0f, color, 24,
                         thickness);
  };

  auto draw_building = [&](const RogueCity::Core::BuildingSite &building,
                           const ImU32 color) {
    const ImVec2 pos = WorldToScreen(building.position);
    const float radius = 7.0f + pulse * 2.0f;
    draw_list->AddRect(ImVec2(pos.x - radius, pos.y - radius),
                       ImVec2(pos.x + radius, pos.y + radius), color, 2.0f, 0,
                       thickness);
  };

  const ImU32 selected_color = IM_COL32(255, 220, 60, 230);
  for (const auto &item : gs.selection_manager.Items()) {
    if (!gs.IsEntityVisible(item.kind, item.id)) {
      continue;
    }
    switch (item.kind) {
    case RogueCity::Core::Editor::VpEntityKind::Road:
      if (const auto *road = FindRoadById(gs, item.id)) {
        draw_road(*road, selected_color);
      }
      break;
    case RogueCity::Core::Editor::VpEntityKind::District:
      if (const auto *district = FindDistrictById(gs, item.id)) {
        draw_district(*district, selected_color);
      }
      break;
    case RogueCity::Core::Editor::VpEntityKind::Lot:
      if (const auto *lot = FindLotById(gs, item.id)) {
        draw_lot(*lot, selected_color);
      }
      break;
    case RogueCity::Core::Editor::VpEntityKind::Building:
      if (const auto *building = FindBuildingById(gs, item.id)) {
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

  const auto &hover = *gs.hovered_entity;
  if (!gs.IsEntityVisible(hover.kind, hover.id)) {
    return;
  }
  const ImU32 hover_color = IM_COL32(120, 220, 255, 220);
  switch (hover.kind) {
  case RogueCity::Core::Editor::VpEntityKind::Road:
    if (const auto *road = FindRoadById(gs, hover.id)) {
      draw_road(*road, hover_color);
    }
    break;
  case RogueCity::Core::Editor::VpEntityKind::District:
    if (const auto *district = FindDistrictById(gs, hover.id)) {
      draw_district(*district, hover_color);
    }
    break;
  case RogueCity::Core::Editor::VpEntityKind::Lot:
    if (const auto *lot = FindLotById(gs, hover.id)) {
      draw_lot(*lot, hover_color);
    }
    break;
  case RogueCity::Core::Editor::VpEntityKind::Building:
    if (const auto *building = FindBuildingById(gs, hover.id)) {
      draw_building(*building, hover_color);
    }
    break;
  default:
    break;
  }
}

void ViewportOverlays::RenderValidationErrors(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (!gs.validation_overlay.enabled) {
    return;
  }

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  for (const auto &error : gs.validation_overlay.errors) {
    if (error.severity ==
            RogueCity::Core::Editor::ValidationSeverity::Warning &&
        !gs.validation_overlay.show_warnings) {
      continue;
    }

    const ImVec2 p = WorldToScreen(error.world_position);
    ImU32 color = IM_COL32(255, 205, 80, 220);
    float radius = 7.0f;
    if (error.severity == RogueCity::Core::Editor::ValidationSeverity::Error) {
      color = IM_COL32(255, 120, 90, 230);
      radius = 8.0f;
    } else if (error.severity ==
               RogueCity::Core::Editor::ValidationSeverity::Critical) {
      color = IM_COL32(255, 65, 65, 240);
      radius = 9.0f;
    }

    draw_list->AddCircleFilled(p, radius, color, 20);
    draw_list->AddCircle(p, radius + 2.0f, IM_COL32(20, 20, 20, 180), 20, 1.0f);

    if (gs.validation_overlay.show_labels && !error.message.empty()) {
      draw_list->AddText(ImVec2(p.x + 8.0f, p.y - 7.0f), color,
                         error.message.c_str());
    }
  }
}

void ViewportOverlays::RenderGizmos(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (!gs.gizmo.enabled || !gs.gizmo.visible ||
      gs.selection_manager.Count() == 0) {
    return;
  }

  RogueCity::Core::Vec2 pivot{};
  size_t count = 0;
  for (const auto &item : gs.selection_manager.Items()) {
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

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const ImVec2 center = WorldToScreen(pivot);
  const float axis_len = 34.0f;
  const float ring_r = 24.0f;

  switch (gs.gizmo.operation) {
  case RogueCity::Core::Editor::GizmoOperation::Translate:
    draw_list->AddLine(ImVec2(center.x - axis_len, center.y),
                       ImVec2(center.x + axis_len, center.y),
                       IM_COL32(255, 100, 100, 240), 2.5f);
    draw_list->AddLine(ImVec2(center.x, center.y - axis_len),
                       ImVec2(center.x, center.y + axis_len),
                       IM_COL32(100, 220, 255, 240), 2.5f);
    break;
  case RogueCity::Core::Editor::GizmoOperation::Rotate:
    draw_list->AddCircle(center, ring_r, IM_COL32(120, 220, 255, 240), 36,
                         2.5f);
    draw_list->AddCircle(center, ring_r + 7.0f, IM_COL32(255, 180, 80, 180), 36,
                         1.5f);
    break;
  case RogueCity::Core::Editor::GizmoOperation::Scale:
    draw_list->AddRect(ImVec2(center.x - ring_r, center.y - ring_r),
                       ImVec2(center.x + ring_r, center.y + ring_r),
                       IM_COL32(120, 255, 150, 240), 0.0f, 0, 2.5f);
    draw_list->AddRectFilled(
        ImVec2(center.x + ring_r - 5.0f, center.y + ring_r - 5.0f),
        ImVec2(center.x + ring_r + 5.0f, center.y + ring_r + 5.0f),
        IM_COL32(120, 255, 150, 220));
    break;
  default:
    break;
  }
}

// AI_INTEGRATION_TAG: V1_PASS1_TASK5_WATER_OVERLAY
void ViewportOverlays::RenderWaterBodies(
    const RogueCity::Core::Editor::GlobalState &gs) {
  using RogueCity::Core::WaterType;
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  auto draw_water = [&](const RogueCity::Core::WaterBody &water) {
    if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Water,
                            water.id)) {
      return;
    }
    if (water.boundary.empty()) {
      return;
    }
    const float layer_opacity = LayerOpacity(
        gs, RogueCity::Core::Editor::VpEntityKind::Water, water.id);

    // Determine color based on water type
    glm::vec4 water_color;
    switch (water.type) {
    case WaterType::River:
      water_color =
          glm::vec4(0.31f, 0.59f, 0.86f, 0.7f * layer_opacity); // Light blue
      break;
    case WaterType::Lake:
      water_color =
          glm::vec4(0.20f, 0.47f, 0.78f, 0.6f * layer_opacity); // Medium blue
      break;
    case WaterType::Ocean:
      water_color =
          glm::vec4(0.12f, 0.35f, 0.71f, 0.65f * layer_opacity); // Dark blue
      break;
    case WaterType::Pond:
    default:
      water_color =
          glm::vec4(0.25f, 0.50f, 0.75f, 0.6f * layer_opacity); // Default blue
      break;
    }

    // Render boundary polygon/polyline
    if (water.type == WaterType::River && water.boundary.size() >= 2) {
      // Rivers are rendered as polylines (flowing paths)
      scratch_screen_points_.clear();
      scratch_screen_points_.reserve(water.boundary.size());
      for (const auto &pt : water.boundary) {
        scratch_screen_points_.push_back(WorldToScreen(pt));
      }

      ImU32 river_color = ImGui::ColorConvertFloat4ToU32(
          ImVec4(water_color.r, water_color.g, water_color.b, water_color.a));
      float line_width = 3.0f + water.depth * 0.5f; // Width based on depth
      draw_list->AddPolyline(scratch_screen_points_.data(),
                             static_cast<int>(scratch_screen_points_.size()),
                             river_color, false, line_width);

      // Add direction arrows (optional, for visual clarity)
      for (size_t i = 0; i < water.boundary.size() - 1; ++i) {
        if (i % 10 == 0) { // Every 10th segment
          RogueCity::Core::Vec2 dir = water.boundary[i + 1] - water.boundary[i];
          const double len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
          if (len > 1.0) {
            dir.x /= len;
            dir.y /= len;
            RogueCity::Core::Vec2 mid =
                (water.boundary[i] + water.boundary[i + 1]) * 0.5;
            RogueCity::Core::Vec2 arrow_tip = mid + dir * 5.0;
            ImVec2 screen_tip = WorldToScreen(arrow_tip);
            draw_list->AddCircleFilled(screen_tip, 2.0f,
                                       IM_COL32(200, 220, 255, 180));
          }
        }
      }
    } else {
      // Lakes, ponds, oceans are rendered as filled polygons
      DrawPolygon(water.boundary, water_color);

      // Shore detail rendering (if enabled)
      if (water.generate_shore) {
        scratch_screen_points_.clear();
        scratch_screen_points_.reserve(water.boundary.size());
        for (const auto &pt : water.boundary) {
          scratch_screen_points_.push_back(WorldToScreen(pt));
        }

        // Draw shore outline with lighter color
        glm::vec4 shore_color(0.78f, 0.71f, 0.55f, 0.8f); // Sandy color
        ImU32 shore = ImGui::ColorConvertFloat4ToU32(
            ImVec4(shore_color.r, shore_color.g, shore_color.b, shore_color.a));
        draw_list->AddPolyline(scratch_screen_points_.data(),
                               static_cast<int>(scratch_screen_points_.size()),
                               shore, true, 1.5f);
      }
    }

    // Label water body type (only if large enough)
    if (water.boundary.size() >= 3) {
      // Calculate centroid
      RogueCity::Core::Vec2 centroid{};
      for (const auto &pt : water.boundary) {
        centroid.x += pt.x;
        centroid.y += pt.y;
      }
      centroid.x /= static_cast<double>(water.boundary.size());
      centroid.y /= static_cast<double>(water.boundary.size());

      const char *type_name = "Water";
      switch (water.type) {
      case WaterType::River:
        type_name = "River";
        break;
      case WaterType::Lake:
        type_name = "Lake";
        break;
      case WaterType::Ocean:
        type_name = "Ocean";
        break;
      case WaterType::Pond:
        type_name = "Pond";
        break;
      }

      DrawWorldText(centroid, type_name, glm::vec4(1.0f, 1.0f, 1.0f, 0.9f));
    }
  };

  int min_cell_x = 0;
  int max_cell_x = 0;
  int min_cell_y = 0;
  int max_cell_y = 0;
  if (ComputeVisibleCellRange(gs, min_cell_x, max_cell_x, min_cell_y,
                              max_cell_y)) {
    const auto &spatial = gs.render_spatial_grid;
    BeginLayerDedupePass(
        RogueCity::Core::Editor::ViewportRenderLayer::WaterBodies,
        gs.waterbodies.indexCount());
    for (int y = min_cell_y; y <= max_cell_y; ++y) {
      for (int x = min_cell_x; x <= max_cell_x; ++x) {
        const uint32_t cell = spatial.ToCellIndex(static_cast<uint32_t>(x),
                                                  static_cast<uint32_t>(y));
        const uint32_t begin = spatial.water.offsets[cell];
        const uint32_t end = spatial.water.offsets[cell + 1u];
        for (uint32_t cursor = begin; cursor < end; ++cursor) {
          const uint32_t handle = spatial.water.handles[cursor];
          if (handle >= gs.waterbodies.indexCount() ||
              !gs.waterbodies.isValidIndex(handle)) {
            continue;
          }
          if (!MarkHandleSeen(handle)) {
            continue;
          }
          draw_water(gs.waterbodies[handle]);
        }
      }
    }
    return;
  }

  RecordFallbackFullScan();
  for (const auto &water : gs.waterbodies) {
    draw_water(water);
  }
}

// AI_INTEGRATION_TAG: V1_PASS1_TASK5_BUILDING_OVERLAY
void ViewportOverlays::RenderBuildingSites(
    const RogueCity::Core::Editor::GlobalState &gs) {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const ViewportLOD lod = ResolveViewportLOD(view_transform_.zoom);
  if (!ShouldRenderBuildingsForLOD(gs, lod)) {
    return;
  }

  auto draw_building = [&](const RogueCity::Core::BuildingSite &building) {
    if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Building,
                            building.id)) {
      return;
    }
    const float layer_opacity = LayerOpacity(
        gs, RogueCity::Core::Editor::VpEntityKind::Building, building.id);
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
    ImU32 color = ImGui::ColorConvertFloat4ToU32(
        ImVec4(building_color.r, building_color.g, building_color.b,
               building_color.a));

    // Draw building marker (square for now)
    float size = 4.0f * std::max(1.0f, view_transform_.zoom);
    draw_list->AddRectFilled(ImVec2(screen_pos.x - size, screen_pos.y - size),
                             ImVec2(screen_pos.x + size, screen_pos.y + size),
                             color);

    // Outline for visibility
    ImU32 outline = IM_COL32(255, 255, 255, 100);
    draw_list->AddRect(ImVec2(screen_pos.x - size, screen_pos.y - size),
                       ImVec2(screen_pos.x + size, screen_pos.y + size),
                       outline, 0.0f, 0, 1.0f);

    // Height indicator (vertical line) - only if enabled in config
    // For now, we'll skip this as it requires config check in Render()
  };

  int min_cell_x = 0;
  int max_cell_x = 0;
  int min_cell_y = 0;
  int max_cell_y = 0;
  if (ComputeVisibleCellRange(gs, min_cell_x, max_cell_x, min_cell_y,
                              max_cell_y)) {
    const auto &spatial = gs.render_spatial_grid;
    BeginLayerDedupePass(
        RogueCity::Core::Editor::ViewportRenderLayer::BuildingSites,
        gs.buildings.size());
    for (int y = min_cell_y; y <= max_cell_y; ++y) {
      for (int x = min_cell_x; x <= max_cell_x; ++x) {
        const uint32_t cell = spatial.ToCellIndex(static_cast<uint32_t>(x),
                                                  static_cast<uint32_t>(y));
        const uint32_t begin = spatial.buildings.offsets[cell];
        const uint32_t end = spatial.buildings.offsets[cell + 1u];
        for (uint32_t cursor = begin; cursor < end; ++cursor) {
          const uint32_t handle = spatial.buildings.handles[cursor];
          if (handle >= gs.buildings.size()) {
            continue;
          }
          if (!MarkHandleSeen(handle)) {
            continue;
          }
          draw_building(gs.buildings.getData()[handle]);
        }
      }
    }
    return;
  }

  RecordFallbackFullScan();
  for (const auto &building : gs.buildings) {
    draw_building(building);
  }
}

// AI_INTEGRATION_TAG: V1_PASS1_TASK5_LOT_OVERLAY
void ViewportOverlays::RenderLotBoundaries(
    const RogueCity::Core::Editor::GlobalState &gs) {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const ViewportLOD lod = ResolveViewportLOD(view_transform_.zoom);
  if (!ShouldRenderLotsForLOD(gs, lod)) {
    return;
  }

  auto draw_lot = [&](const RogueCity::Core::LotToken &lot) {
    if (!gs.IsEntityVisible(RogueCity::Core::Editor::VpEntityKind::Lot,
                            lot.id)) {
      return;
    }
    if (lot.boundary.empty()) {
      return;
    }
    const float layer_opacity =
        LayerOpacity(gs, RogueCity::Core::Editor::VpEntityKind::Lot, lot.id);

    // Lot boundary color (subtle, so it doesn't overwhelm)
    glm::vec4 lot_color(0.8f, 0.8f, 0.8f,
                        0.3f * layer_opacity); // Light gray, semi-transparent

    // Convert to screen space
    scratch_screen_points_.clear();
    scratch_screen_points_.reserve(lot.boundary.size());
    for (const auto &pt : lot.boundary) {
      scratch_screen_points_.push_back(WorldToScreen(pt));
    }

    // Draw boundary lines
    ImU32 line_color = ImGui::ColorConvertFloat4ToU32(
        ImVec4(lot_color.r, lot_color.g, lot_color.b, lot_color.a));
    draw_list->AddPolyline(scratch_screen_points_.data(),
                           static_cast<int>(scratch_screen_points_.size()),
                           line_color,
                           true, // Closed loop
                           1.0f);
  };

  int min_cell_x = 0;
  int max_cell_x = 0;
  int min_cell_y = 0;
  int max_cell_y = 0;
  if (ComputeVisibleCellRange(gs, min_cell_x, max_cell_x, min_cell_y,
                              max_cell_y)) {
    const auto &spatial = gs.render_spatial_grid;
    BeginLayerDedupePass(
        RogueCity::Core::Editor::ViewportRenderLayer::LotBoundaries,
        gs.lots.indexCount());
    for (int y = min_cell_y; y <= max_cell_y; ++y) {
      for (int x = min_cell_x; x <= max_cell_x; ++x) {
        const uint32_t cell = spatial.ToCellIndex(static_cast<uint32_t>(x),
                                                  static_cast<uint32_t>(y));
        const uint32_t begin = spatial.lots.offsets[cell];
        const uint32_t end = spatial.lots.offsets[cell + 1u];
        for (uint32_t cursor = begin; cursor < end; ++cursor) {
          const uint32_t handle = spatial.lots.handles[cursor];
          if (handle >= gs.lots.indexCount() || !gs.lots.isValidIndex(handle)) {
            continue;
          }
          if (!MarkHandleSeen(handle)) {
            continue;
          }
          draw_lot(gs.lots[handle]);
        }
      }
    }
    return;
  }

  RecordFallbackFullScan();
  for (const auto &lot : gs.lots) {
    draw_lot(lot);
  }
}

void ViewportOverlays::RenderConnectorGraph(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (gs.connector_debug_edges.empty()) {
    return;
  }
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const ImU32 connector_color = IM_COL32(255, 90, 90, 195);
  for (const auto &edge : gs.connector_debug_edges) {
    if (edge.points.size() < 2) {
      continue;
    }
    const ImVec2 a = WorldToScreen(edge.points.front());
    const ImVec2 b = WorldToScreen(edge.points.back());
    draw_list->AddLine(a, b, connector_color, 2.6f);
  }
}

void ViewportOverlays::RenderCityBoundary(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (gs.city_boundary.size() < 3) {
    return;
  }
  scratch_screen_points_.clear();
  scratch_screen_points_.reserve(gs.city_boundary.size());
  for (const auto &p : gs.city_boundary) {
    scratch_screen_points_.push_back(WorldToScreen(p));
  }

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddPolyline(scratch_screen_points_.data(),
                         static_cast<int>(scratch_screen_points_.size()),
                         IM_COL32(35, 90, 255, 240), true, 3.0f);
}

// Y2K Grid Overlay - 50px grid with themed color
void ViewportOverlays::RenderGridOverlay(
    const RogueCity::Core::Editor::GlobalState &gs) {
  if (!gs.config.show_grid_overlay) {
    return;
  }
  if (view_transform_.viewport_size.x <= 0.0f ||
      view_transform_.viewport_size.y <= 0.0f ||
      view_transform_.zoom <= 1e-6f) {
    return;
  }

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  auto &theme_mgr = ::RogueCity::UI::ThemeManager::Instance();
  const auto &theme = theme_mgr.GetActiveTheme();

  // Grid in subtle primary accent (low alpha)
  ImVec4 primary_accent = ImGui::ColorConvertU32ToFloat4(theme.primary_accent);
  primary_accent.w = 0.15f;
  ImU32 grid_color = ImGui::ColorConvertFloat4ToU32(primary_accent);

  // Grid spacing: 50px in world space (adjust per zoom if needed)
  const double grid_spacing = 50.0;

  // Calculate visible bounds in world space from the four viewport corners.
  const ImVec2 viewport_min = view_transform_.viewport_pos;
  const ImVec2 viewport_max =
      ImVec2(viewport_min.x + view_transform_.viewport_size.x,
             viewport_min.y + view_transform_.viewport_size.y);

  const std::array<RogueCity::Core::Vec2, 4> corners{
      ScreenToWorld(viewport_min),
      ScreenToWorld(ImVec2(viewport_max.x, viewport_min.y)),
      ScreenToWorld(viewport_max),
      ScreenToWorld(ImVec2(viewport_min.x, viewport_max.y))};

  double world_min_x = corners[0].x;
  double world_max_x = corners[0].x;
  double world_min_y = corners[0].y;
  double world_max_y = corners[0].y;
  for (const auto &corner : corners) {
    world_min_x = std::min(world_min_x, corner.x);
    world_max_x = std::max(world_max_x, corner.x);
    world_min_y = std::min(world_min_y, corner.y);
    world_max_y = std::max(world_max_y, corner.y);
  }

  // Snap to grid multiples
  const int grid_start_x =
      static_cast<int>(std::floor(world_min_x / grid_spacing)) - 1;
  const int grid_end_x =
      static_cast<int>(std::ceil(world_max_x / grid_spacing)) + 1;
  const int grid_start_y =
      static_cast<int>(std::floor(world_min_y / grid_spacing)) - 1;
  const int grid_end_y =
      static_cast<int>(std::ceil(world_max_y / grid_spacing)) + 1;

  // Draw vertical grid lines
  for (int i = grid_start_x; i <= grid_end_x; ++i) {
    const double world_x = static_cast<double>(i) * grid_spacing;
    const ImVec2 p1 =
        WorldToScreen(RogueCity::Core::Vec2{world_x, world_min_y});
    const ImVec2 p2 =
        WorldToScreen(RogueCity::Core::Vec2{world_x, world_max_y});
    draw_list->AddLine(p1, p2, grid_color, 1.0f);
  }

  // Draw horizontal grid lines
  for (int i = grid_start_y; i <= grid_end_y; ++i) {
    const double world_y = static_cast<double>(i) * grid_spacing;
    const ImVec2 p1 =
        WorldToScreen(RogueCity::Core::Vec2{world_min_x, world_y});
    const ImVec2 p2 =
        WorldToScreen(RogueCity::Core::Vec2{world_max_x, world_y});
    draw_list->AddLine(p1, p2, grid_color, 1.0f);
  }
}

ViewportOverlays &GetViewportOverlays() {
  static ViewportOverlays instance;
  return instance;
}

void BuildingSearchOverlay::SetActive(bool active) {
  if (active_ == active)
    return;
  active_ = active;
  if (active_) {
    needs_filter_update_ = true;
    selected_index_ = -1;
  }
}

void BuildingSearchOverlay::UpdateFilter(
    const RogueCity::Core::Editor::GlobalState &gs) {
  filtered_building_ids_.clear();
  std::string search_str = search_buffer_;
  std::transform(search_str.begin(), search_str.end(), search_str.begin(),
                 ::tolower);

  for (const auto &building : gs.buildings) {
    bool match = false;
    if (search_str.empty()) {
      match = true;
    } else {
      if (std::to_string(building.id).find(search_str) != std::string::npos) {
        match = true;
      } else {
        std::string type_name =
            std::string(magic_enum::enum_name(building.type));
        std::transform(type_name.begin(), type_name.end(), type_name.begin(),
                       ::tolower);
        if (type_name.find(search_str) != std::string::npos) {
          match = true;
        }
      }
    }

    if (match) {
      filtered_building_ids_.push_back(building.id);
    }
  }
  needs_filter_update_ = false;
  if (selected_index_ >= static_cast<int>(filtered_building_ids_.size())) {
    selected_index_ = filtered_building_ids_.empty() ? -1 : 0;
  }
}

void BuildingSearchOverlay::Render(
    RogueCity::Core::Editor::GlobalState &gs,
    const ViewportOverlays::ViewTransform &view_transform) {
  if (!active_)
    return;

  if (needs_filter_update_) {
    UpdateFilter(gs);
  }

  const float overlay_width = 400.0f;
  const ImVec2 pos(view_transform.viewport_pos.x +
                       (view_transform.viewport_size.x - overlay_width) * 0.5f,
                   view_transform.viewport_pos.y + 60.0f);

  ImGui::SetNextWindowPos(pos);
  ImGui::SetNextWindowSize(ImVec2(overlay_width, 0.0f));

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
      ImGuiWindowFlags_NoNav;

  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        TokenColorF(UITokens::BackgroundDark, 230u));
  ImGui::PushStyleColor(ImGuiCol_Border,
                        TokenColorF(UITokens::CyanAccent, 180u));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);

  if (ImGui::Begin("##BuildingSearchOverlay", nullptr, flags)) {
    ImGui::TextColored(TokenColorF(UITokens::CyanAccent), "BUILDING SEARCH");
    ImGui::Separator();

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputTextWithHint("##SearchInput", "Enter ID or Type...",
                                 search_buffer_,
                                 IM_ARRAYSIZE(search_buffer_))) {
      needs_filter_update_ = true;
    }

    if (ImGui::IsWindowAppearing()) {
      ImGui::SetKeyboardFocusHere(-1);
    }

    if (filtered_building_ids_.empty()) {
      ImGui::TextColored(TokenColorF(UITokens::ErrorRed),
                         "No buildings found.");
    } else {
      ImGui::BeginChild("##ResultsList", ImVec2(0, 200.0f), false,
                        ImGuiWindowFlags_NoScrollbar);
      for (int i = 0; i < static_cast<int>(filtered_building_ids_.size());
           ++i) {
        const uint32_t id = filtered_building_ids_[i];
        const RogueCity::Core::BuildingSite *building =
            FindBuildingById(gs, id);
        if (!building)
          continue;

        char label[128];
        const std::string type_name =
            std::string(magic_enum::enum_name(building->type));
        snprintf(label, sizeof(label), "ID: %-6u | %s", id, type_name.c_str());

        bool selected = (selected_index_ == i);
        if (ImGui::Selectable(label, selected)) {
          selected_index_ = i;
          gs.selection_manager.Clear();
          gs.selection_manager.Add(
              RogueCity::Core::Editor::VpEntityKind::Building, id);
          gs.dirty_layers.MarkDirty(
              RogueCity::Core::Editor::DirtyLayer::ViewportIndex);
        }

        if (selected && ImGui::IsWindowFocused()) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndChild();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      SetActive(false);
    }
  }
  ImGui::End();

  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(2);
}

BuildingSearchOverlay &GetBuildingSearchOverlay() {
  static BuildingSearchOverlay instance;
  return instance;
}

} // namespace RC_UI::Viewport
