// FILE: rc_panel_building_index.h
// PURPOSE: Index panel listing all buildings with district/lot association
// REFACTORED: Uses RcDataIndexPanel<T> template (Phase 2 - RC-0.10)

#pragma once

#include "ui/patterns/rc_ui_data_index_panel.h"
#include "ui/panels/rc_panel_data_index_traits.h"

namespace RC_UI::Panels::BuildingIndex {

// Singleton instance of the templated panel
inline RC_UI::Patterns::RcDataIndexPanel<RogueCity::Core::BuildingSite, BuildingIndexTraits>& GetPanel() {
    using PanelType = RC_UI::Patterns::RcDataIndexPanel<RogueCity::Core::BuildingSite, RC_UI::Panels::BuildingIndexTraits>;
    static PanelType panel("Building Index", "visualizer/src/ui/panels/rc_panel_building_index.cpp");
    return panel;
}

} // namespace RC_UI::Panels::BuildingIndex
