// FILE: rc_panel_guide_index.h
// PURPOSE: Standalone floating index window for viewport divider guides.
//          Lists all DivLines with filter, inline label editing, per-row
//          enable/disable + delete, and a Clear All footer button.
//          Selecting a row syncs the viewport amber highlight via selected_div_id.

#pragma once

namespace RC_UI::Panels::GuideIndex {

/// Open (or bring to front) the floating Guide Index window.
void Open();

bool IsOpen();

/// Call once per frame from the root draw loop.
void Draw(float dt);

} // namespace RC_UI::Panels::GuideIndex
