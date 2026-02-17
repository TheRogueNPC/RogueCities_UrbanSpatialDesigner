// FILE: rc_panel_district_index.h
// PURPOSE: Index panel listing all districts for editing and selection.
// REFACTORED: Now uses RcDataIndexPanel<T> template (Phase 2 - R0.2)

#pragma once

#include "ui/patterns/rc_ui_data_index_panel.h"
#include "ui/panels/rc_panel_data_index_traits.h"

namespace RC_UI::Panels::DistrictIndex {

// Singleton instance of the templated panel
inline RC_UI::Patterns::RcDataIndexPanel<RogueCity::Core::District, DistrictIndexTraits>& GetPanel() {
    using PanelType = RC_UI::Patterns::RcDataIndexPanel<RogueCity::Core::District, RC_UI::Panels::DistrictIndexTraits>;
    static PanelType panel("District Index", "visualizer/src/ui/panels/rc_panel_district_index.cpp");
    return panel;
}

} // namespace RC_UI::Panels::DistrictIndex
