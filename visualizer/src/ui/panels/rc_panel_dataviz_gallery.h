// FILE: rc_panel_dataviz_gallery.h
// PURPOSE: RCDV Gallery — floating reference window for all DataViz components.
// Open via Help > RCDV Gallery...
#pragma once

namespace RC_UI::DataVizGallery {

bool IsOpen();
void Toggle();

/// Draw the gallery window (call every frame from DrawRuntimeTitlebar).
/// Internally uses ImGui::GetTime() for time-based effects — no dt required.
void Draw();

} // namespace RC_UI::DataVizGallery
