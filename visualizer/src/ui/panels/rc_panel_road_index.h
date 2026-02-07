// FILE: rc_panel_road_index.h
// PURPOSE: Index panel listing all roads with classification and status.
// REFACTORED: Now uses RcDataIndexPanel<T> template (Phase 2 - R0.2)

#pragma once

#include "ui/patterns/rc_ui_data_index_panel.h"
#include "ui/panels/rc_panel_data_index_traits.h"

namespace RC_UI::Panels::RoadIndex {

// Singleton instance of the templated panel
inline RC_UI::Patterns::RcDataIndexPanel<RogueCity::Core::Road, RoadIndexTraits>& GetPanel() {
    using PanelType = RC_UI::Patterns::RcDataIndexPanel<RogueCity::Core::Road, RC_UI::Panels::RoadIndexTraits>;
    static PanelType panel("Road Index", "visualizer/src/ui/panels/rc_panel_road_index.cpp");
    return panel;
}

// Draw the road index panel. Shows count and each road ID/type.
inline void Draw(float dt) {
    GetPanel().Draw(dt);
}

} // namespace RC_UI::Panels::RoadIndex
