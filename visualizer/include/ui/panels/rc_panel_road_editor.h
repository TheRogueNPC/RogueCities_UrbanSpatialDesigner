// FILE: visualizer/include/ui/panels/rc_panel_road_editor.h
// PURPOSE: Panel for editing road networks.

#pragma once

namespace RC_UI::Panels::RoadEditor {

// Sub-tools available in the road editor.
enum class RoadEditorSubtool {
    Select,
    AddVertex,
    Split,
    Merge,
    Convert
};

// Draw the road editor panel.
void Draw(float dt);
void DrawContent(float dt);

} // namespace RC_UI::Panels::RoadEditor
