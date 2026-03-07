#pragma once
namespace RC_UI::Panels::RcdtdGenerator {

// Draws the standalone RC_DTD generator window (DockableWindow).
// Must be called each frame from rc_ui_root. All RC_DTD rendering
// is self-contained here; there is no viewport overlay companion.
void DrawWindow(float dt);

} // namespace RC_UI::Panels::RcdtdGenerator
