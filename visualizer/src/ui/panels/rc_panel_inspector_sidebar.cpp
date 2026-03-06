// FILE: visualizer/src/ui/panels/rc_panel_inspector_sidebar.cpp
// PURPOSE: Unified right-column container with Inspector + System Map tabs.
//          Keeps the ImGui window name "Inspector" so saved dock layouts remain
//          valid, and adds a tab bar to switch between the two sub-panels.
//
// B2 WIRING: s_requested_tab is set by ActivityBarRight and consumed here
//            each frame to focus the correct tab.

#include "ui/panels/rc_panel_inspector_sidebar.h"

#include "ui/introspection/UiIntrospection.h"
#include "ui/panels/rc_panel_city_health.h"
#include "ui/panels/rc_panel_inspector.h"
#include "ui/panels/rc_panel_system_map.h"
#include "ui/rc_ui_components.h"

#include <imgui.h>

namespace RC_UI::Panels {

// Static definition
int RcInspectorSidebar::s_requested_tab = -1;

void RcInspectorSidebar::RequestTab(int tab_index) {
    s_requested_tab = tab_index;
}

int RcInspectorSidebar::GetRequestedTab() {
    return s_requested_tab;
}

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

    // Honour a pending B2 tab request (consume it this frame).
    if (s_requested_tab >= 0) {
        m_active_tab   = s_requested_tab;
        s_requested_tab = -1;
    }

    if (ImGui::BeginTabBar("##InspectorSidebarTabs")) {
        // Pass ImGuiTabItemFlags_SetSelected to auto-focus when B2 drove it.
        auto tab_flags = [&](int idx) -> ImGuiTabItemFlags {
            return (m_active_tab == idx)
                ? ImGuiTabItemFlags_SetSelected
                : ImGuiTabItemFlags_None;
        };

        if (ImGui::BeginTabItem("Inspector", nullptr, tab_flags(0))) {
            if (ImGui::IsItemActivated()) m_active_tab = 0;
            RC_UI::Panels::Inspector::DrawContent(dt);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("System Map", nullptr, tab_flags(1))) {
            if (ImGui::IsItemActivated()) m_active_tab = 1;
            RC_UI::Panels::SystemMap::DrawContent(dt);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Health", nullptr, tab_flags(2))) {
            if (ImGui::IsItemActivated()) m_active_tab = 2;
            RC_UI::Panels::CityHealth::DrawContent(dt);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    uiint.EndPanel();
    Components::EndTokenPanel();
}

} // namespace RC_UI::Panels
