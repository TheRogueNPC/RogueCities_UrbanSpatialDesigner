// FILE: rc_panel_guide_index.cpp
// PURPOSE: Floating Guide Index window — tabulated view of all DivLine guides.
//          Supports filter, sort, inline label editing, enable/disable toggle,
//          per-row delete, clear-all, and viewport selection sync.

#include "ui/panels/rc_panel_guide_index.h"
#include "ui/api/rc_imgui_api.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/viewport/rc_viewport_overlays.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <imgui.h>

#ifdef RC_FEATURE_GUIDE_DB_SYNC
#include "RogueCity/Core/Database/SpacetimeClient.hpp"
#endif

namespace RC_UI::Panels::GuideIndex {

namespace {

static bool s_open          = false;
static bool s_sort_asc      = true;
static char s_filter[128]   = {};

// Buffers for per-row label editing — re-used each frame; PushID ensures
// uniqueness. Label is at most 63 chars.
static char s_label_buf[64] = {};

} // namespace

// ---------------------------------------------------------------------------

bool IsOpen() { return s_open; }
void Open()   { s_open = true; ImGui::SetNextWindowFocus(); }

// ---------------------------------------------------------------------------

void Draw(float /*dt*/) {
  if (!s_open) return;

  ImGui::SetNextWindowSize(ImVec2(480.0f, 320.0f), ImGuiCond_FirstUseEver);

  static RC_UI::DockableWindowState s_win;
  if (!RC_UI::BeginDockableWindow("Guide Index##GuideIndexPanel", s_win,
                                  "Right",
                                  ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoScrollWithMouse)) {
    return;
  }

  auto &tracker = RC_UI::Viewport::GetViewportOverlays().div_lines;

  // ── Header row: filter + sort + count ────────────────────────────────────
  ImGui::Text("Guides: %d", static_cast<int>(tracker.lines.size()));
  API::SameLine(0.0f, 12.0f);
  API::SetNextItemWidth(160.0f);
  API::InputText("##gfilter", s_filter, sizeof(s_filter));
  API::SameLine();
  if (API::SmallButton(s_sort_asc ? "Sort: ID ^" : "Sort: ID v"))
    s_sort_asc = !s_sort_asc;
  API::SameLine();
  API::BeginDisabled(tracker.lines.empty());
  if (API::Button("Clear All")) {
    tracker.Clear();
    tracker.selected_div_id = -1;
#ifdef RC_FEATURE_GUIDE_DB_SYNC
    RogueCity::Core::Database::SpacetimeClient::ClearDivGuides();
#endif
  }
  API::EndDisabled();

  API::Separator();

  // ── Table ────────────────────────────────────────────────────────────────
  const ImGuiTableFlags tflags =
      ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
      ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable |
      ImGuiTableFlags_SizingStretchProp;

  ImGui::PushStyleColor(ImGuiCol_TableRowBg,
                        ImGui::ColorConvertU32ToFloat4(UITokens::Transparent));
  ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt,
                        ImGui::ColorConvertU32ToFloat4(
                            WithAlpha(UITokens::PanelBackground, 100)));

  if (ImGui::BeginTable("##guide_table", 6, tflags,
                        ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()))) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("ID",       ImGuiTableColumnFlags_WidthFixed,  32.0f);
    ImGui::TableSetupColumn("Type",     ImGuiTableColumnFlags_WidthFixed,  40.0f);
    ImGui::TableSetupColumn("Pos (m)",  ImGuiTableColumnFlags_WidthFixed,  72.0f);
    ImGui::TableSetupColumn("Label",    ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("En",       ImGuiTableColumnFlags_WidthFixed,  28.0f);
    ImGui::TableSetupColumn("Del",      ImGuiTableColumnFlags_WidthFixed,  28.0f);
    ImGui::TableHeadersRow();

    // Build filtered + sorted index list
    std::vector<int> indices;
    indices.reserve(tracker.lines.size());
    for (int i = 0; i < static_cast<int>(tracker.lines.size()); ++i) {
      const auto &d = tracker.lines[static_cast<std::size_t>(i)];
      if (s_filter[0] != '\0') {
        // Simple substring match against label (case-insensitive chars)
        const char *lbl = d.label.c_str();
        bool found = false;
        for (const char *p = lbl; *p; ++p) {
          if (std::tolower(static_cast<unsigned char>(*p)) ==
              std::tolower(static_cast<unsigned char>(s_filter[0]))) {
            // Check rest of filter
            bool match = true;
            for (int fi = 0; s_filter[fi] && *(p + fi); ++fi) {
              if (std::tolower(static_cast<unsigned char>(*(p + fi))) !=
                  std::tolower(static_cast<unsigned char>(s_filter[fi]))) {
                match = false; break;
              }
            }
            if (match) { found = true; break; }
          }
        }
        if (!found) continue;
      }
      indices.push_back(i);
    }

    if (!s_sort_asc) std::reverse(indices.begin(), indices.end());

    int to_delete = -1;  // Defer deletion to avoid iterator invalidation

    for (const int ri : indices) {
      auto &d = tracker.lines[static_cast<std::size_t>(ri)];
      const bool is_sel = (d.id == tracker.selected_div_id);

      ImGui::TableNextRow();
      if (is_sel) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                               WithAlpha(UITokens::AmberGlow, 35));
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1,
                               WithAlpha(UITokens::AmberGlow, 35));
      }

      ImGui::PushID(d.id);

      // Col 0 — ID (selectable spanning all columns)
      ImGui::TableSetColumnIndex(0);
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                            ImGui::ColorConvertU32ToFloat4(
                                WithAlpha(UITokens::CyanAccent, 40)));
      ImGui::PushStyleColor(ImGuiCol_HeaderActive,
                            ImGui::ColorConvertU32ToFloat4(
                                WithAlpha(UITokens::AmberGlow, 60)));
      if (API::Selectable("##row", is_sel,
                          ImGuiSelectableFlags_SpanAllColumns |
                          ImGuiSelectableFlags_AllowOverlap)) {
        tracker.selected_div_id = is_sel ? -1 : d.id;  // toggle on re-click
      }
      ImGui::PopStyleColor(2);

      // Amber selection bar on leading edge
      if (is_sel) {
        const ImVec2 pmin = ImGui::GetItemRectMin();
        const ImVec2 pmax = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRectFilled(
            pmin, ImVec2(pmin.x + 3.0f, pmax.y), UITokens::AmberGlow);
      }

      API::SameLine();
      char id_buf[8];
      std::snprintf(id_buf, sizeof(id_buf), "%d", d.id);
      ImGui::TextUnformatted(id_buf);

      // Col 1 — Type
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted(d.horizontal ? "H" : "V");

      // Col 2 — Position
      ImGui::TableSetColumnIndex(2);
      char pos_buf[24];
      std::snprintf(pos_buf, sizeof(pos_buf), "%.2f", d.world_pos);
      ImGui::TextUnformatted(pos_buf);

      // Col 3 — Label (inline edit)
      ImGui::TableSetColumnIndex(3);
      std::strncpy(s_label_buf, d.label.c_str(), sizeof(s_label_buf) - 1);
      s_label_buf[sizeof(s_label_buf) - 1] = '\0';
      API::SetNextItemWidth(-1.0f);
      if (API::InputText("##lbl", s_label_buf, sizeof(s_label_buf))) {
        d.label = s_label_buf;
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        d.label = s_label_buf;
#ifdef RC_FEATURE_GUIDE_DB_SYNC
        RogueCity::Core::Database::SpacetimeClient::PushDivGuide(
            d.id, d.horizontal, d.world_pos, d.label.c_str(), d.enabled);
#endif
      }

      // Col 4 — Enabled checkbox
      ImGui::TableSetColumnIndex(4);
      if (API::Checkbox("##en", &d.enabled)) {
#ifdef RC_FEATURE_GUIDE_DB_SYNC
        RogueCity::Core::Database::SpacetimeClient::PushDivGuide(
            d.id, d.horizontal, d.world_pos, d.label.c_str(), d.enabled);
#endif
      }

      // Col 5 — Delete button
      ImGui::TableSetColumnIndex(5);
      if (API::SmallButton("X")) {
        to_delete = d.id;
      }

      ImGui::PopID();
    }

    // Deferred deletion (safe — outside the row iteration)
    if (to_delete >= 0) {
      tracker.Remove(to_delete);
      if (tracker.selected_div_id == to_delete)
        tracker.selected_div_id = -1;
#ifdef RC_FEATURE_GUIDE_DB_SYNC
      RogueCity::Core::Database::SpacetimeClient::RemoveDivGuide(to_delete);
#endif
    }

    ImGui::EndTable();
  }

  ImGui::PopStyleColor(2);

  // ── Footer hint ───────────────────────────────────────────────────────────
  API::TextDisabled("Alt+click a guide in the viewport to select it.");

  RC_UI::EndDockableWindow();
  // Note: s_open is managed by the dockable window's close button via &s_open
  // passed through DockableWindowState if supported; otherwise stays true
  // until the user explicitly closes via the X on the window title bar.
}

} // namespace RC_UI::Panels::GuideIndex
