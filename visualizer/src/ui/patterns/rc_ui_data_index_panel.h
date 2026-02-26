// FILE: rc_ui_data_index_panel.h
// PURPOSE: Generic template for data index panels with sortable/filterable
// table REDUCES: 80% code duplication across road_index, district_index,
// lot_index panels

#pragma once

#include <algorithm>
#include <functional>
#include <imgui.h>
#include <string>
#include <vector>


#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_root.h"


namespace RC_UI::Patterns {

// Trait interface for customizing RcDataIndexPanel behavior
template <typename T> struct DataIndexPanelTraits {
  using ContainerType = std::vector<T>;
  // Required: Get the data collection from GlobalState
  static ContainerType &GetData(RogueCity::Core::Editor::GlobalState &gs);

  // Required: Get display name for an entity (legacy/default if only 1 column)
  static std::string GetEntityLabel(const T &entity, size_t index) {
    return "Entity";
  }

  // Multi-column support
  static int GetColumnCount() { return 1; }
  static const char *GetColumnHeader(int column) { return "Entity"; }
  static void RenderCell(T &entity, size_t index, int column) {
    ImGui::Text("%s", GetEntityLabel(entity, index).c_str());
  }

  // Optional: Custom panel title
  static const char *GetPanelTitle() { return "Data Index"; }

  // Optional: Custom data name (for introspection)
  static const char *GetDataName() { return "entities"; }

  // Optional: Custom tags for introspection
  static std::vector<std::string> GetTags() { return {"index"}; }

  // Optional: Custom filter predicate
  static bool FilterEntity(const T &entity, const std::string &filter) {
    return true;
  }

  // Optional: Custom sort comparator (default: by ID)
  static bool CompareEntities(const T &a, const T &b) { return a.id < b.id; }

  // Optional: Custom context menu for entity
  static void ShowContextMenu(T &entity, size_t index) { /* no-op by default */
  }

  // Optional: Custom selection callback
  static void OnEntitySelected(T &entity, size_t index) { /* no-op by default */
  }

  // Optional: Custom hover callback
  static void OnEntityHovered(T &entity, size_t index) { /* no-op by default */
  }
};

// Generic data index panel template
template <typename T, typename Traits = DataIndexPanelTraits<T>>
class RcDataIndexPanel {
public:
  RcDataIndexPanel(const char *panel_title = nullptr,
                   const char *source_file = "unknown")
      : m_panel_title(panel_title ? panel_title : Traits::GetPanelTitle()),
        m_source_file(source_file), m_filter_text{}, m_selected_index(-1),
        m_sort_ascending(true) {}

  // Content-only draw (for drawer mode - no window creation)
  void DrawContent(RogueCity::Core::Editor::GlobalState &gs,
                   RogueCity::UIInt::UiIntrospector &uiint) {
    // Get data
    auto &data = Traits::GetData(gs);

    // Header with count
    ImGui::Text("%s: %llu", Traits::GetDataName(),
                static_cast<unsigned long long>(data.size()));

    const char *sort_label = m_sort_ascending ? "Sort: A-Z" : "Sort: Z-A";
    const ImGuiStyle &style = ImGui::GetStyle();
    const float controls_width = ImGui::GetContentRegionAvail().x;
    const float sort_width =
        std::max(100.0f, ImGui::CalcTextSize(sort_label).x +
                             style.FramePadding.x * 2.0f + 12.0f);

    if (controls_width < 320.0f) {
      ImGui::SetNextItemWidth(-1.0f);
      ImGui::InputTextWithHint("##filter", "Filter...", m_filter_text,
                               sizeof(m_filter_text));
      if (ImGui::Button(sort_label, ImVec2(-1.0f, 0.0f))) {
        m_sort_ascending = !m_sort_ascending;
      }
    } else {
      const float filter_width =
          std::max(140.0f, controls_width - sort_width - style.ItemSpacing.x);
      ImGui::SetNextItemWidth(filter_width);
      ImGui::InputTextWithHint("##filter", "Filter...", m_filter_text,
                               sizeof(m_filter_text));
      ImGui::SameLine();
      if (ImGui::Button(sort_label, ImVec2(sort_width, 0.0f))) {
        m_sort_ascending = !m_sort_ascending;
      }
    }

    ImGui::Separator();

    // Table
    const ImGuiTableFlags table_flags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable;

    const int column_count = Traits::GetColumnCount();
    if (ImGui::BeginTable("##data_table", column_count, table_flags,
                          ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) {
      ImGui::TableSetupScrollFreeze(0, 1);
      for (int i = 0; i < column_count; ++i) {
        ImGui::TableSetupColumn(Traits::GetColumnHeader(i),
                                (i == 0) ? ImGuiTableColumnFlags_NoHide : 0);
      }
      ImGui::TableHeadersRow();

      // Filter and sort
      std::vector<size_t> visible_indices;
      std::string filter_str(m_filter_text);

      for (size_t i = 0; i < data.size(); ++i) {
        if (filter_str.empty() || Traits::FilterEntity(data[i], filter_str)) {
          visible_indices.push_back(i);
        }
      }

      // Sort if requested
      if (!m_sort_ascending) {
        std::reverse(visible_indices.begin(), visible_indices.end());
      }

      // Render rows
      for (size_t idx : visible_indices) {
        auto &entity = data[idx];
        ImGui::TableNextRow();

        // Primary column serves as the Selectable
        ImGui::TableSetColumnIndex(0);
        ImGui::PushID(static_cast<int>(idx));

        const bool is_selected = (m_selected_index == static_cast<int>(idx));
        const std::string hidden_label = "##row_" + std::to_string(idx);

        // Span selectable over the first column's cell
        if (ImGui::Selectable(hidden_label.c_str(), is_selected,
                              ImGuiSelectableFlags_SpanAllColumns |
                                  ImGuiSelectableFlags_AllowOverlap)) {
          m_selected_index = static_cast<int>(idx);
          Traits::OnEntitySelected(entity, idx);
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
          Traits::OnEntityHovered(entity, idx);
        }

        if (ImGui::BeginPopupContextItem()) {
          Traits::ShowContextMenu(entity, idx);
          ImGui::EndPopup();
        }

        // Now render actual content into the columns
        for (int col = 0; col < column_count; ++col) {
          ImGui::TableSetColumnIndex(col);
          Traits::RenderCell(entity, idx, col);
        }

        ImGui::PopID();
      }

      ImGui::EndTable();
    }

    // Footer with selection info
    if (m_selected_index >= 0 &&
        m_selected_index < static_cast<int>(data.size())) {
      ImGui::Separator();
      ImGui::Text("Selection: #%d", m_selected_index);
    }

    uiint.RegisterWidget({"table",
                          Traits::GetDataName(),
                          std::string(Traits::GetDataName()) + "[]",
                          {"index"}});
  }

  // Get currently selected index
  int GetSelectedIndex() const { return m_selected_index; }

  // Programmatically set selection
  void SetSelectedIndex(int index) { m_selected_index = index; }

  // Clear selection
  void ClearSelection() { m_selected_index = -1; }

private:
  const char *m_panel_title;
  const char *m_source_file;
  char m_filter_text[256];
  int m_selected_index;
  bool m_sort_ascending;
};

} // namespace RC_UI::Patterns
