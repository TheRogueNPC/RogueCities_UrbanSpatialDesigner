// FILE: visualizer/src/ui/rc_ui_panel_macros.h
// PURPOSE: Collapse the 17-line Draw(float dt) boilerplate for Variant A panels.
//
// Variant A: BeginTokenPanel + uiint.BeginPanel-always + DrawContent(dt)
//   — no HFSM state gate, no custom window flags, no BeginDockableWindow.
//
// USAGE — place after DrawContent() in a panel .cpp namespace body:
//
//   RC_PANEL_DRAW_IMPL(
//       "Validation",                                          // display name (BeginTokenPanel title + PanelMeta[0])
//       "Validation",                                          // label (PanelMeta[1])
//       "validation",                                          // panel_id (PanelMeta[2])
//       "Bottom",                                              // dock position (PanelMeta[3])
//       UITokens::AmberGlow,                                   // border color
//       "visualizer/src/ui/panels/rc_panel_validation.cpp",   // source file
//       "validation", "runtime"                                // tags (variadic)
//   )
//
// PREREQUISITES (caller's .cpp must already include):
//   "ui/rc_ui_components.h"           — Components::BeginTokenPanel / EndTokenPanel
//   "ui/introspection/UiIntrospection.h" — RogueCity::UIInt::UiIntrospector, PanelMeta
//
// EXCLUSIONS — do NOT use for:
//   - Panels with an HFSM state gate before BeginTokenPanel (Variant C)
//   - Panels using BeginDockableWindow (Variant D): axiom_editor, axiom_bar, dev_shell, tools
//   - Panels with non-default ImGuiWindowFlags (e.g. AlwaysAutoResize)
//   - rc_panel_log.cpp (custom viewport overlay, no BeginTokenPanel)
//   - rc_panel_inspector.cpp (early-return before BeginPanel, Variant B)
//   - rc_panel_road_editor.cpp (BeginTokenPanel but no uiint.BeginPanel, Variant B)

#pragma once

#define RC_PANEL_DRAW_IMPL(DISPLAY, LABEL, PANEL_ID, DOCK, COLOR, SRC_FILE, ...) \
void Draw(float dt) {                                                              \
    const bool _open = Components::BeginTokenPanel(DISPLAY, COLOR);               \
    auto& _uiint = RogueCity::UIInt::UiIntrospector::Instance();                  \
    _uiint.BeginPanel(                                                             \
        RogueCity::UIInt::PanelMeta{DISPLAY, LABEL, PANEL_ID, DOCK,               \
                                     SRC_FILE, {__VA_ARGS__}},                    \
        _open);                                                                    \
    if (!_open) {                                                                  \
        _uiint.EndPanel();                                                         \
        Components::EndTokenPanel();                                               \
        return;                                                                    \
    }                                                                              \
    DrawContent(dt);                                                               \
    _uiint.EndPanel();                                                             \
    Components::EndTokenPanel();                                                   \
}
