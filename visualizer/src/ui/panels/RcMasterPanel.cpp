// FILE: RcMasterPanel.cpp
// PURPOSE: Implementation of master panel container with hybrid tabs+search
// ARCHITECTURE: Single ImGui window, routes to drawers via PanelRegistry

#include "RcMasterPanel.h"
#include "RogueCity/App/Editor/CommandHistory.hpp"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_animation.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <imgui.h>

namespace RC_UI::Panels {

RcMasterPanel::RcMasterPanel() {
  // Initialize popout states (all embedded by default)
  for (int i = 0; i < static_cast<int>(PanelType::COUNT); ++i) {
    m_popout_states[static_cast<PanelType>(i)] = false;
  }
}

void RcMasterPanel::Draw(float dt) {
  using RogueCity::Core::Editor::GetEditorHFSM;
  using RogueCity::Core::Editor::GetGlobalState;

  auto &gs = GetGlobalState();
  auto &hfsm = GetEditorHFSM();
  auto &introspector = RogueCity::UIInt::UiIntrospector::Instance();

  DrawContext ctx{gs,           hfsm,
                  introspector, &RogueCity::App::GetEditorCommandHistory(),
                  dt,           false};

  // Handle search overlay (Ctrl+P)
  if (ImGui::IsKeyPressed(ImGuiKey_P) && ImGui::GetIO().KeyCtrl) {
    OpenSearch();
  }

  // Main master panel window
  if (m_master_window_open) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

    static RC_UI::DockableWindowState s_master_window_state;
    const bool open =
        RC_UI::BeginDockableWindow("Master Panel", s_master_window_state,
                                   "Left", ImGuiWindowFlags_NoCollapse);

    introspector.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Master Panel",
            "Master Panel",
            "container",
            "Left",
            "visualizer/src/ui/panels/RcMasterPanel.cpp",
            {"master", "router", "tabs"}},
        open);

    if (open) {
      // Mockup styling: LCARS / Y2K geometric panel frame
      RC_UI::Components::DrawPanelFrame(UITokens::CyanAccent);

      // Tab bar for category navigation
      DrawTabBar(ctx);
      ImGui::Spacing();

      // Active drawer content
      DrawActiveDrawer(ctx);

      // Y2K pulsing border when focused (Cockpit Doctrine: motion as
      // instruction)
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow)) {
        AnimationHelpers::DrawPulsingBorder(UITokens::CyanAccent, 0.7f);
      }
    }

    introspector.EndPanel();
    if (open) {
      RC_UI::EndDockableWindow();
    }
  }

  // Search overlay (modal)
  if (m_search_open) {
    DrawSearchOverlay(ctx);
  }

  // Popout windows
  HandlePopouts(ctx);
}

void RcMasterPanel::DrawTabBar(DrawContext &ctx) {
  auto &registry = PanelRegistry::Instance();

  const std::array<PanelCategory, 5> categories = {
      PanelCategory::Indices, PanelCategory::Controls, PanelCategory::Tools,
      PanelCategory::System, PanelCategory::AI};

  std::vector<PanelType> active_panels;

  ImGui::PushStyleColor(ImGuiCol_TabHovered, ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::PanelBackground, 255)));
  ImGui::PushStyleColor(ImGuiCol_Tab, ImGui::ColorConvertU32ToFloat4(UITokens::BackgroundDark));
  ImGui::PushStyleColor(ImGuiCol_TabSelected, ImGui::ColorConvertU32ToFloat4(UITokens::PanelBackground));
  ImGui::PushStyleColor(ImGuiCol_TabSelectedOverline, ImGui::ColorConvertU32ToFloat4(UITokens::AmberGlow));
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary)); // default text dim
  ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0.0f);

  if (ImGui::BeginTabBar("##MasterPanelTabs",
                         ImGuiTabBarFlags_FittingPolicyScroll)) {
    for (PanelCategory cat : categories) {
      auto panels = registry.GetPanelsInCategory(cat);
      if (panels.empty()) {
        continue;
      }
      if (cat == PanelCategory::AI) {
#if !defined(ROGUE_AI_DLC_ENABLED)
        continue;
#endif
        if (!ctx.global_state.config.dev_mode_enabled) {
          continue;
        }
      }

      // If active, push primary text color
      bool is_active_cat = (m_active_category == cat);
      if (is_active_cat) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::AmberGlow));
      
      if (ImGui::BeginTabItem(PanelCategoryName(cat))) {
        m_active_category = cat;
        active_panels = panels;
        ImGui::EndTabItem();
      }
      
      if (is_active_cat) ImGui::PopStyleColor();
    }
    ImGui::EndTabBar();
  }

  ImGui::PopStyleVar();
  ImGui::PopStyleColor(5);

  if (active_panels.empty()) {
    for (PanelCategory cat : categories) {
      auto panels = registry.GetPanelsInCategory(cat);
      if (panels.empty()) {
        continue;
      }
      if (cat == PanelCategory::AI) {
#if !defined(ROGUE_AI_DLC_ENABLED)
        continue;
#endif
        if (!ctx.global_state.config.dev_mode_enabled) {
          continue;
        }
      }
      m_active_category = cat;
      active_panels = panels;
      break;
    }
  }

  DrawCategoryTab(m_active_category, active_panels);
  ImGui::Spacing();

  // Mockup style search bar
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(UITokens::BackgroundDark));
  ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::CyanAccent, 100)));
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::TextPrimary));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
  
  ImGui::SetNextItemWidth(-1.0f);
  if (ImGui::InputTextWithHint("##InlineMasterSearch", "Search... (Ctrl+P)", m_search_filter, sizeof(m_search_filter))) {
      // Typing here directly updates m_search_filter, same as the popup
      m_search_results = FilterPanelsBySearch(m_search_filter);
      // If we wanted to, we could show a popover below this. 
      // For now, it updates the filter string which is used by the popup if it opens.
  }
  
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(3);

  ImGui::Spacing();
  ImGui::Separator();
}

void RcMasterPanel::DrawCategoryTab(PanelCategory cat,
                                    const std::vector<PanelType> &panels) {
  if (panels.empty()) {
    return;
  }

  if (std::find(panels.begin(), panels.end(), m_active_panel) == panels.end()) {
    m_active_panel = panels.front();
  }

  auto &registry = PanelRegistry::Instance();
  if (panels.size() == 1) {
    m_active_panel = panels[0];
    return;
  }

  const char *sub_tab_id = "##CategorySubTabs";
  switch (cat) {
  case PanelCategory::Indices:
    sub_tab_id = "##CategorySubTabs_Indices";
    break;
  case PanelCategory::Controls:
    sub_tab_id = "##CategorySubTabs_Controls";
    break;
  case PanelCategory::Tools:
    sub_tab_id = "##CategorySubTabs_Tools";
    break;
  case PanelCategory::System:
    sub_tab_id = "##CategorySubTabs_System";
    break;
  case PanelCategory::AI:
    sub_tab_id = "##CategorySubTabs_AI";
    break;
  default:
    break;
  }

  ImGui::PushStyleColor(ImGuiCol_TabHovered, ImGui::ColorConvertU32ToFloat4(UITokens::TextPrimary));
  ImGui::PushStyleColor(ImGuiCol_Tab, ImGui::ColorConvertU32ToFloat4(RC_UI::WithAlpha(UITokens::PanelBackground, 0)));
  ImGui::PushStyleColor(ImGuiCol_TabSelected, ImGui::ColorConvertU32ToFloat4(RC_UI::WithAlpha(UITokens::CyanAccent, 40)));
  ImGui::PushStyleColor(ImGuiCol_TabSelectedOverline, ImGui::ColorConvertU32ToFloat4(UITokens::CyanAccent));
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::TextSecondary));
  ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0.0f);

  if (ImGui::BeginTabBar(sub_tab_id, ImGuiTabBarFlags_FittingPolicyScroll)) {
    for (PanelType type : panels) {
      IPanelDrawer *drawer = registry.GetDrawer(type);
      if (!drawer) {
        continue;
      }

      bool is_active_panel = (m_active_panel == type);
      if (is_active_panel) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::CyanAccent));

      if (ImGui::BeginTabItem(drawer->display_name())) {
        m_active_panel = type;
        ImGui::EndTabItem();
      }

      if (is_active_panel) ImGui::PopStyleColor();

      if (ImGui::IsItemHovered() &&
          ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("##MasterPanelContextMenu");
        m_context_menu_target = type;
      }
    }

    if (ImGui::BeginPopup("##MasterPanelContextMenu")) {
      IPanelDrawer *target_drawer = registry.GetDrawer(m_context_menu_target);
      if (target_drawer) {
        ImGui::TextDisabled("%s Actions", target_drawer->display_name());
        ImGui::Separator();

        if (ImGui::MenuItem("Pop Out (Float)")) {
          ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Duplicate View")) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Pin to Edge")) {
          ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Minimize to Shelf")) {
          ImGui::CloseCurrentPopup();
        }
      }
      ImGui::EndPopup();
    }

    ImGui::EndTabBar();
  }
  
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(5);
}

void RcMasterPanel::DrawActiveDrawer(DrawContext &ctx) {
  auto &registry = PanelRegistry::Instance();

  // Check if active panel switched (trigger lifecycle hooks)
  if (m_active_panel != m_last_active_panel) {
    if (IPanelDrawer *old_drawer = registry.GetDrawer(m_last_active_panel)) {
      old_drawer->on_deactivated();
    }
    if (IPanelDrawer *new_drawer = registry.GetDrawer(m_active_panel)) {
      new_drawer->on_activated();
    }
    m_last_active_panel = m_active_panel;
  }

  ImGuiWindow *window = ImGui::GetCurrentWindow();
  bool is_docked_left = false;
  if (window->DockNode && ImGui::GetMainViewport()) {
    is_docked_left =
        (window->DockNode->Pos.x < ImGui::GetMainViewport()->Size.x * 0.5f);
  }

  // Draw active panel content
  ImGui::BeginChild("##DrawerContent", ImVec2(0, 0), false,
                    ImGuiWindowFlags_NoScrollbar);

  IPanelDrawer *drawer = registry.GetDrawer(m_active_panel);
  if (drawer && drawer->is_visible(ctx)) {
    // Popout button
    if (drawer->can_popout()) {
      bool is_popout = IsPopout(m_active_panel);
      if (ImGui::Button(is_popout ? "Dock" : "Popout")) {
        SetPopout(m_active_panel, !is_popout);
      }
      ImGui::Spacing();
    }

    // Drawer content
    drawer->draw(ctx);
  } else if (!drawer) {
    ImGui::TextDisabled("Panel not available");
  } else {
    ImGui::TextDisabled("Panel not visible in current state");
  }

  float scroll_y = ImGui::GetScrollY();
  float scroll_max = ImGui::GetScrollMaxY();
  ImGui::EndChild();

  if (scroll_max > 0.0f) {
    // Render custom dock-aware stylized scrollbar
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    float scroll_ratio = scroll_y / scroll_max;
    float bar_h = std::max(20.0f, size.y * 0.1f);
    float bar_y = pos.y + 40.0f + (size.y - 80.0f - bar_h) * scroll_ratio;
    float bar_x = is_docked_left ? pos.x + 2.0f : pos.x + size.x - 6.0f;

    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(bar_x, bar_y), ImVec2(bar_x + 4.0f, bar_y + bar_h),
        RC_UI::UITokens::CyanAccent, 2.0f);
  }
}

void RcMasterPanel::DrawSearchOverlay(DrawContext &ctx) {
  ImGui::OpenPopup("##PanelSearch");

  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Always);
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

  if (ImGui::BeginPopupModal("##PanelSearch", &m_search_open,
                             ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize)) {

    ImGui::Text("Search Panels");
    ImGui::Separator();

    // Search input (auto-focus)
    ImGui::SetKeyboardFocusHere();
    bool enter_pressed =
        ImGui::InputText("##search", m_search_filter, sizeof(m_search_filter),
                         ImGuiInputTextFlags_EnterReturnsTrue);

    // Filter results
    m_search_results = FilterPanelsBySearch(m_search_filter);

    ImGui::Separator();

    // Results list
    if (m_search_results.empty()) {
      ImGui::TextDisabled("No results");
    } else {
      auto &registry = PanelRegistry::Instance();

      for (size_t i = 0; i < m_search_results.size(); ++i) {
        PanelType type = m_search_results[i];
        IPanelDrawer *drawer = registry.GetDrawer(type);
        if (!drawer)
          continue;

        bool is_selected = (static_cast<int>(i) == m_search_selected_index);
        if (ImGui::Selectable(drawer->display_name(), is_selected)) {
          SelectSearchResult(static_cast<int>(i));
        }

        // Keyboard navigation
        if (is_selected) {
          if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            m_search_selected_index =
                std::min(m_search_selected_index + 1,
                         static_cast<int>(m_search_results.size()) - 1);
          }
          if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            m_search_selected_index = std::max(m_search_selected_index - 1, 0);
          }
        }
      }

      // Enter activates selection
      if (enter_pressed && !m_search_results.empty()) {
        SelectSearchResult(m_search_selected_index);
      }
    }

    // Close on Escape
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      CloseSearch();
    }

    ImGui::EndPopup();
  }
}

void RcMasterPanel::HandlePopouts(DrawContext &ctx) {
  auto &registry = PanelRegistry::Instance();

  // Track which popouts are still active this frame
  std::unordered_set<PanelType> new_active_popouts;

  for (const auto &[type, is_popout] : m_popout_states) {
    if (!is_popout)
      continue;

    IPanelDrawer *drawer = registry.GetDrawer(type);
    if (!drawer || !drawer->is_visible(ctx))
      continue;

    bool window_open = true;
    if (Components::BeginTokenPanel(drawer->display_name(), UITokens::InfoBlue,
                                    &window_open, ImGuiWindowFlags_None)) {
      // Mark context as floating window
      DrawContext float_ctx = ctx;
      float_ctx.is_floating_window = true;

      drawer->draw(float_ctx);
      new_active_popouts.insert(type);
    }
    Components::EndTokenPanel();

    // If window closed, dock it back
    if (!window_open) {
      SetPopout(type, false);
    }
  }

  // Trigger deactivation for closed popouts
  for (PanelType type : m_active_popouts) {
    if (new_active_popouts.find(type) == new_active_popouts.end()) {
      if (IPanelDrawer *drawer = registry.GetDrawer(type)) {
        drawer->on_deactivated();
      }
    }
  }

  // Trigger activation for new popouts
  for (PanelType type : new_active_popouts) {
    if (m_active_popouts.find(type) == m_active_popouts.end()) {
      if (IPanelDrawer *drawer = registry.GetDrawer(type)) {
        drawer->on_activated();
      }
    }
  }

  m_active_popouts = new_active_popouts;
}

std::vector<PanelType>
RcMasterPanel::FilterPanelsBySearch(const std::string &query) {
  auto &registry = PanelRegistry::Instance();
  auto all_panels = registry.GetAllPanelTypes();

  if (query.empty()) {
    return all_panels;
  }

  // Simple case-insensitive substring match
  std::string lower_query = query;
  std::transform(
      lower_query.begin(), lower_query.end(), lower_query.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  std::vector<PanelType> results;
  for (PanelType type : all_panels) {
    IPanelDrawer *drawer = registry.GetDrawer(type);
    if (!drawer)
      continue;

    std::string lower_name = drawer->display_name();
    std::transform(
        lower_name.begin(), lower_name.end(), lower_name.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower_name.find(lower_query) != std::string::npos) {
      results.push_back(type);
    }
  }

  return results;
}

void RcMasterPanel::SelectSearchResult(int index) {
  if (index < 0 || index >= static_cast<int>(m_search_results.size())) {
    return;
  }

  SetActivePanel(m_search_results[index]);
  CloseSearch();
}

void RcMasterPanel::SetActivePanel(PanelType type) {
  m_active_panel = type;

  // Switch to appropriate category
  auto &registry = PanelRegistry::Instance();
  if (IPanelDrawer *drawer = registry.GetDrawer(type)) {
    m_active_category = drawer->category();
  }
}

void RcMasterPanel::SetPopout(PanelType type, bool popout) {
  m_popout_states[type] = popout;
}

bool RcMasterPanel::IsPopout(PanelType type) const {
  auto it = m_popout_states.find(type);
  return (it != m_popout_states.end()) ? it->second : false;
}

void RcMasterPanel::OpenSearch() {
  m_search_open = true;
  m_search_selected_index = 0;
  m_search_filter[0] = '\0';
}

void RcMasterPanel::CloseSearch() { m_search_open = false; }

} // namespace RC_UI::Panels
