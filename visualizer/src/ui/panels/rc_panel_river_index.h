#include "RogueCity/Core/Data/CityTypes.hpp"
#include "ui/panels/rc_panel_data_index_traits.h"
#include "ui/patterns/rc_ui_data_index_panel.h"

namespace RC_UI::Panels::RiverIndex {

// Singleton instance of the templated panel
inline RC_UI::Patterns::RcDataIndexPanel<RogueCity::Core::WaterBody,
                                         RC_UI::Panels::RiverIndexTraits> &
GetPanel() {
  using PanelType =
      RC_UI::Patterns::RcDataIndexPanel<RogueCity::Core::WaterBody,
                                        RC_UI::Panels::RiverIndexTraits>;
  static PanelType panel("Water Index",
                         "visualizer/src/ui/panels/rc_panel_river_index.cpp");
  return panel;
}

// Draw the river index panel.
void Draw(float dt);
// Draw only the panel body (for Master Panel drawer embedding).
void DrawContent(float dt);

} // namespace RC_UI::Panels::RiverIndex
