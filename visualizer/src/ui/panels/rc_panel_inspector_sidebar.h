// FILE: visualizer/src/ui/panels/rc_panel_inspector_sidebar.h
// PURPOSE: Unified right-column container with Inspector + System Map tabs,
//          matching the RC_UI_Mockup.html 3-column layout.

#pragma once

namespace RC_UI::Panels {

class RcInspectorSidebar {
public:
    void Draw(float dt);

private:
    bool m_window_open = true;
    int  m_active_tab  = 0; // 0 = Inspector, 1 = System Map
};

} // namespace RC_UI::Panels
