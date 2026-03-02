// FILE: visualizer/src/ui/panels/rc_panel_inspector_sidebar.cpp
// PURPOSE: Unified right-column container with Inspector + System Map tabs.
//          Keeps the ImGui window name "Inspector" so saved dock layouts remain
//          valid, and adds a tab bar to switch between the two sub-panels.

#include "ui/panels/rc_panel_inspector_sidebar.h"

#include "ui/introspection/UiIntrospection.h"
#include "ui/panels/rc_panel_inspector.h"
#include "ui/panels/rc_panel_system_map.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>

namespace RC_UI::Panels {

void RcInspectorSidebar::Draw(float dt) {
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();

    const bool open =
        Components::BeginTokenPanel("Inspector", UITokens::MagentaHighlight,
                                    &m_window_open);
    if (!open) {
        Components::EndTokenPanel();
        return;
    }

    Components::DrawPanelFrame(UITokens::MagentaHighlight);

    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Inspector",
            "Inspector Sidebar",
            "inspector_sidebar",
            "Right",
            "visualizer/src/ui/panels/rc_panel_inspector_sidebar.cpp",
            {"inspector", "system_map", "tabs", "right_column"}},
        true);

    if (ImGui::BeginTabBar("##InspectorSidebarTabs")) {
        if (ImGui::BeginTabItem("Inspector")) {
            m_active_tab = 0;
            RC_UI::Panels::Inspector::DrawContent(dt);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("System Map")) {
            m_active_tab = 1;
            RC_UI::Panels::SystemMap::DrawContent(dt);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    uiint.EndPanel();
    Components::EndTokenPanel();
}

} // namespace RC_UI::Panels
