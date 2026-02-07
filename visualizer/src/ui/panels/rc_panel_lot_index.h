// FILE: rc_panel_lot_index.h
// PURPOSE: Index panel listing all lots with district association.
// REFACTORED: Now uses RcDataIndexPanel<T> template (Phase 2 - R0.2)

#pragma once

#include "ui/patterns/rc_ui_data_index_panel.h"
#include "ui/panels/rc_panel_data_index_traits.h"

namespace RC_UI::Panels::LotIndex {

// Singleton instance of the templated panel
inline RC_UI::Patterns::RcDataIndexPanel<RogueCity::Core::LotToken, LotIndexTraits>& GetPanel() {
    using PanelType = RC_UI::Patterns::RcDataIndexPanel<RogueCity::Core::LotToken, RC_UI::Panels::LotIndexTraits>;
    static PanelType panel("Lot Index", "visualizer/src/ui/panels/rc_panel_lot_index.cpp");
    return panel;
}

// Draw the lot index panel.
inline void Draw(float dt) {
    GetPanel().Draw(dt);
}

} // namespace RC_UI::Panels::LotIndex
