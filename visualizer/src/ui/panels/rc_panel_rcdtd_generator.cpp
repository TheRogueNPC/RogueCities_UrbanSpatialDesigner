#include "ui/panels/rc_panel_rcdtd_generator.h"
#include "RogueCity/Generators/Urban/RCDTDGenerator.hpp"
#include "ui/api/rc_imgui_api.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include <algorithm>
#include <cmath>

namespace RC_UI::Panels::RcdtdGenerator {

using namespace RogueCity::Generators::Urban;

static ImU32 GetZoneColor(ZoneType type) {
  // Y2K palette — matches canonical district color scheme where applicable.
  switch (type) {
  case ZoneType::CityCenter:   return WithAlpha(UITokens::YellowWarning,  200);
  case ZoneType::MixedResCom:  return WithAlpha(UITokens::InfoBlue,       180);
  case ZoneType::HighRes:      return WithAlpha(UITokens::CyanAccent,     180);
  case ZoneType::MedRes:       return WithAlpha(UITokens::MagentaHighlight, 150);
  case ZoneType::LowRes:       return WithAlpha(UITokens::TextSecondary,  160);
  case ZoneType::Commercial:   return WithAlpha(UITokens::SuccessGreen,   180);
  case ZoneType::Industrial:   return WithAlpha(UITokens::ErrorRed,       180);
  case ZoneType::Business:     return WithAlpha(UITokens::AmberGlow,      180);
  case ZoneType::Park:         return IM_COL32(60, 200, 80, 160);
  default:                     return WithAlpha(UITokens::TextDisabled,   140);
  }
}

// Persistent state for the RCDTD generator
static RCDTDGenerator s_generator;
static RCDTDConfig s_config;
static bool s_initialized = false;

static void EnsureInitialized() {
  if (s_initialized)
    return;

  s_config.seed            = 42;
  s_config.minAngleDeg     = 55.0f; // Thesis recommended range 55-65
  s_config.recursionLevels = 2;
  s_config.growthBudget    = 1000.0f;
  s_config.straightnessWeight = 1.0f;
  s_config.distanceWeight  = 0.2f;
  s_config.capIntersectionsAt4 = true;

  // Pre-size to cover max recursionLevels (3) so UI can edit all safely.
  s_config.levels.resize(3);
  s_config.levels[0] = {25.0f, {30.0f, 70.0f}, 60.0f, 100.0f};
  s_config.levels[1] = {12.0f, {15.0f, 35.0f}, 30.0f, 60.0f};
  s_config.levels[2] = { 6.0f, {10.0f, 20.0f}, 20.0f, 40.0f};

  s_config.zoneTypes = {
      ZoneType::CityCenter,  ZoneType::MixedResCom, ZoneType::HighRes,
      ZoneType::MedRes,      ZoneType::LowRes,      ZoneType::Commercial,
      ZoneType::Industrial,  ZoneType::Business,    ZoneType::Park};
  const size_t n = s_config.zoneTypes.size();
  s_config.targetAreaProportions.assign(n, 1.0f / static_cast<float>(n));
  s_config.adjacencyWeights.assign(n, std::vector<float>(n, 1.0f));
  // MixedResCom <-> Industrial penalty
  s_config.adjacencyWeights[1][6] = s_config.adjacencyWeights[6][1] = 0.1f;
  // Commercial <-> CityCenter affinity
  s_config.adjacencyWeights[5][0] = s_config.adjacencyWeights[0][5] = 2.0f;

  s_generator.SetConfig(s_config);
  s_initialized = true;
}

// Canvas pan/zoom state — persist across frames.
// Offset is in pixels relative to auto-fit center; zoom is a multiplier on top.
static ImVec2 s_canvas_offset{0.0f, 0.0f};
static float  s_canvas_zoom = 1.0f;

void DrawWindow(float dt) {
  EnsureInitialized();

  static RC_UI::DockableWindowState s_rcdtd_window;
  if (!RC_UI::BeginDockableWindow("RC_DTD", s_rcdtd_window, "Right",
                                  ImGuiWindowFlags_NoCollapse)) {
    return;
  }

  // ── Generator Settings ───────────────────────────────────────────────────
  if (Components::DrawSectionHeader("Generator Settings", UITokens::CyanAccent)) {
    API::Indent();
    bool changed = false;
    changed |= API::SliderInt  ("Seed",             (int *)&s_config.seed,            0,    9999);
    changed |= API::SliderFloat("Growth Budget",    &s_config.growthBudget,          100.0f, 10000.0f);
    changed |= API::SliderFloat("Min Angle (Deg)",  &s_config.minAngleDeg,            10.0f,    90.0f);
    changed |= API::SliderFloat("Straightness Wt",  &s_config.straightnessWeight,      0.0f,     5.0f);
    changed |= API::SliderFloat("Distance Wt",      &s_config.distanceWeight,          0.0f,     5.0f);
    changed |= API::Checkbox   ("Cap 4-Way Intersections", &s_config.capIntersectionsAt4);

    // Recursion levels — clamp levels array size to match
    int rec = s_config.recursionLevels;
    if (API::SliderInt("Recursion Levels", &rec, 1, 3)) {
      s_config.recursionLevels = rec;
      changed = true;
    }
    if (changed)
      s_generator.SetConfig(s_config);
    API::Unindent();
    API::Spacing();
  }

  // ── Per-level configs (dynamic, driven by recursionLevels) ───────────────
  // Max 3 entries are always allocated in EnsureInitialized; only show active ones.
  static constexpr const char* kLevelLabels[] = {"Growth Level 0", "Growth Level 1", "Growth Level 2"};
  static constexpr ImU32 kLevelColors[] = {UITokens::InfoBlue, UITokens::AmberGlow, UITokens::MagentaHighlight};
  const int numLevels = std::clamp(s_config.recursionLevels, 1, 3);
  for (int li = 0; li < numLevels; ++li) {
    ImGui::PushID(li);
    if (Components::DrawSectionHeader(kLevelLabels[li], kLevelColors[li], false)) {
      API::Indent();
      auto &l = s_config.levels[static_cast<size_t>(li)];
      bool lc = false;
      lc |= API::SliderFloat("Clearance",   &l.clearance,              1.0f, 100.0f);
      lc |= API::SliderFloat("Conn Radius", &l.connectionRadius,      10.0f, 300.0f);
      lc |= API::SliderFloat("Min Ext",     &l.extensionRange.min,     5.0f, 150.0f);
      lc |= API::SliderFloat("Max Ext",     &l.extensionRange.max,    10.0f, 300.0f);
      lc |= API::SliderFloat("Split Dist",  &l.edgeSplitDistance,     20.0f, 300.0f);
      if (lc) s_generator.SetConfig(s_config);
      API::Unindent();
      API::Spacing();
    }
    ImGui::PopID();
  }

  // ── Actions ───────────────────────────────────────────────────────────────
  API::Spacing();
  if (API::Button("GENERATE ALL", ImVec2(-1, 40))) {
    s_generator.Generate();
    // Reset canvas view to fit new data
    s_canvas_zoom   = 1.0f;
    s_canvas_offset = {0.0f, 0.0f};
  }
  if (API::Button("Step Generation", ImVec2(-1, 30))) {
    s_generator.GenerateStep();
  }

  // ── Stats ─────────────────────────────────────────────────────────────────
  API::Spacing();
  if (Components::DrawSectionHeader("Stats", UITokens::TextSecondary, false)) {
    API::Indent();
    API::Text(UITokens::TextPrimary, "Nodes: %zu",       s_generator.GetRoadGraph().vertices().size());
    API::Text(UITokens::TextPrimary, "Edges: %zu",       s_generator.GetRoadGraph().edges().size());
    API::Text(UITokens::TextPrimary, "Faces/Blocks: %zu",s_generator.GetBlockGraph().faces.size());
    API::Text(UITokens::TextPrimary, "Zones assigned: %zu", s_generator.GetZoning().assignments.size());
    API::Unindent();
  }

  API::Spacing();

  // ── Canvas ────────────────────────────────────────────────────────────────
  // Contract: BeginChild/EndChild must be unconditional — content is gated
  // inside. Pan: right-click drag. Zoom: mouse wheel centered on cursor.
  const ImVec2 canvas_size = ImGui::GetContentRegionAvail();
  const ImVec2 child_size(canvas_size.x, std::max(canvas_size.y, 1.0f));
  ImGui::BeginChild("RcdtdCanvas", child_size, true,
                    ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse);

  if (canvas_size.y > 60.0f) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

    // Background
    draw_list->AddRectFilled(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        UITokens::BackgroundDark);


    // Y2K Warning stripe border
    draw_list->AddRect(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        UITokens::YellowWarning, 0.0f, 0, 2.0f);

    // ── Pan / Zoom interaction ────────────────────────────────────────────
    // Mouse wheel zooms centered on cursor; right-click drags to pan.
    if (ImGui::IsWindowHovered()) {
      const float scroll = ImGui::GetIO().MouseWheel;
      if (std::abs(scroll) > 0.0f) {
        const float k = (scroll > 0.0f) ? 1.15f : (1.0f / 1.15f);
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const float cx = canvas_pos.x + canvas_size.x * 0.5f;
        const float cy = canvas_pos.y + canvas_size.y * 0.5f;
        // Zoom centered on mouse: adjust offset so point under cursor stays fixed.
        s_canvas_offset.x = mouse.x - cx + (s_canvas_offset.x - (mouse.x - cx)) * k;
        s_canvas_offset.y = mouse.y - cy + (s_canvas_offset.y - (mouse.y - cy)) * k;
        s_canvas_zoom = std::clamp(s_canvas_zoom * k, 0.05f, 40.0f);
      }
      if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
        const ImVec2 d = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right, 0.0f);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
        s_canvas_offset.x += d.x;
        s_canvas_offset.y += d.y;
      }
    }
    // Fit button — top-right corner of canvas
    {
      const float btn_w = 36.0f;
      const float btn_h = 18.0f;
      ImGui::SetCursorScreenPos(ImVec2(
          canvas_pos.x + canvas_size.x - btn_w - 6.0f,
          canvas_pos.y + 6.0f));
      if (ImGui::Button("Fit", ImVec2(btn_w, btn_h))) {
        s_canvas_zoom   = 1.0f;
        s_canvas_offset = {0.0f, 0.0f};
      }
    }

    // ── Auto-fit bounds ───────────────────────────────────────────────────
    auto &roadGraph = s_generator.GetRoadGraph();
    bool has_bounds = false;
    RogueCity::Core::Vec2 min_b{0.0, 0.0};
    RogueCity::Core::Vec2 max_b{0.0, 0.0};
    for (const auto &v : roadGraph.vertices()) {
      if (!has_bounds) { min_b = max_b = v.pos; has_bounds = true; }
      else {
        min_b.x = std::min(min_b.x, v.pos.x); min_b.y = std::min(min_b.y, v.pos.y);
        max_b.x = std::max(max_b.x, v.pos.x); max_b.y = std::max(max_b.y, v.pos.y);
      }
    }
    double w = max_b.x - min_b.x;
    double h = max_b.y - min_b.y;
    const double pad = std::max(std::max(w, h) * 0.1, 10.0);
    min_b.x -= pad; min_b.y -= pad;
    max_b.x += pad; max_b.y += pad;
    w = max_b.x - min_b.x;
    h = max_b.y - min_b.y;

    // World-to-screen: auto-fit is the neutral frame, zoom+pan layered on top.
    const float cx = canvas_pos.x + canvas_size.x * 0.5f;
    const float cy = canvas_pos.y + canvas_size.y * 0.5f;
    const auto world_to_screen = [&](const RogueCity::Core::Vec2 &p) -> ImVec2 {
      const float bx = canvas_pos.x + static_cast<float>((p.x - min_b.x) / w) * canvas_size.x;
      const float by = canvas_pos.y + static_cast<float>((p.y - min_b.y) / h) * canvas_size.y;
      return ImVec2(
          cx + (bx - cx) * s_canvas_zoom + s_canvas_offset.x,
          cy + (by - cy) * s_canvas_zoom + s_canvas_offset.y);
    };

    if (has_bounds && w > 1e-6 && h > 1e-6) {
      // Draw block faces (zone wireframe)
      auto &blockGraph = s_generator.GetBlockGraph();
      for (const auto &[id, face] : blockGraph.faces) {
        if (face.boundary.empty()) continue;
        const ImU32 zone_col = GetZoneColor(face.assignedZone);
        std::vector<ImVec2> pts;
        pts.reserve(face.boundary.size());
        for (const auto &bp : face.boundary)
          pts.push_back(world_to_screen(bp));
        draw_list->AddPolyline(pts.data(), static_cast<int>(pts.size()),
                               zone_col, ImDrawFlags_Closed, 1.5f);
      }
      // Draw road edges
      for (const auto &e : roadGraph.edges()) {
        const auto *vA = roadGraph.getVertex(e.a);
        const auto *vB = roadGraph.getVertex(e.b);
        draw_list->AddLine(world_to_screen(vA->pos), world_to_screen(vB->pos),
                           UITokens::CyanAccent, 1.5f);
      }
      // Draw nodes
      for (const auto &v : roadGraph.vertices())
        draw_list->AddCircleFilled(world_to_screen(v.pos), 2.0f, UITokens::TextPrimary);

      // ── Zone legend (bottom-left) ─────────────────────────────────────
      struct LegendEntry { const char *name; ImU32 col; };
      static constexpr LegendEntry kLegend[] = {
          {"City Center",  IM_COL32(255,200,  0,200)},
          {"Mixed Res/Com",IM_COL32(100,150,255,180)},
          {"High Res",     IM_COL32(  0,255,255,180)},
          {"Med Res",      IM_COL32(255,  0,255,150)},
          {"Low Res",      IM_COL32(180,180,180,160)},
          {"Commercial",   IM_COL32(  0,255,100,180)},
          {"Industrial",   IM_COL32(255, 50, 50,180)},
          {"Business",     IM_COL32(255,180,  0,180)},
          {"Park",         IM_COL32( 60,200, 80,160)},
      };
      const float lg_x = canvas_pos.x + 8.0f;
      float lg_y = canvas_pos.y + canvas_size.y - 8.0f
                   - static_cast<float>(IM_ARRAYSIZE(kLegend)) * 14.0f;
      for (const auto &entry : kLegend) {
        draw_list->AddRectFilled({lg_x, lg_y + 2.0f}, {lg_x + 10.0f, lg_y + 12.0f}, entry.col);
        draw_list->AddText({lg_x + 14.0f, lg_y}, UITokens::TextSecondary, entry.name);
        lg_y += 14.0f;
      }
    } else {
      // No graph data — draw help text
      const char *hint = "Press GENERATE ALL";
      const ImVec2 ts = ImGui::CalcTextSize(hint);
      draw_list->AddText(
          ImVec2(canvas_pos.x + (canvas_size.x - ts.x) * 0.5f,
                 canvas_pos.y + (canvas_size.y - ts.y) * 0.5f),
          UITokens::TextDisabled, hint);
    }
  } // end content block (canvas_size.y > 60.0f)

  // EndChild always paired with the unconditional BeginChild above.
  ImGui::EndChild();

  RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::RcdtdGenerator
