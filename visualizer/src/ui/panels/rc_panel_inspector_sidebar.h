// FILE: visualizer/src/ui/panels/rc_panel_inspector_sidebar.h
// PURPOSE: Unified right-column container with Inspector + System Map tabs,
//          matching the RC_UI_Mockup.html 3-column layout.
//
// B2 WIRING:
//  ActivityBarRight calls RequestTab(n) to focus a specific P4 tab.
//  Draw() reads s_requested_tab each frame and honours it once.

#pragma once

namespace RC_UI::Panels {

class RcInspectorSidebar {
public:
    void Draw(float dt);

    void SetWindowOpen(bool open) { m_window_open = open; }
    [[nodiscard]] bool IsWindowOpen() const { return m_window_open; }

    // B2 Activity Bar interface — thread-safe within single-threaded ImGui loop.
    static void RequestTab(int tab_index);   // 0=Inspector 1=SystemMap 2=Health
    static int  GetRequestedTab();           // -1 = no pending request

private:
    bool m_window_open = true;
    int  m_active_tab  = 0; // 0 = Inspector, 1 = System Map, 2 = Health

    // One-frame pending request from B2 bar; -1 = none
    static int s_requested_tab;
};

} // namespace RC_UI::Panels
