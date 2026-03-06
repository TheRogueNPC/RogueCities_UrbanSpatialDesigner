// FILE: visualizer/src/ui/panels/rc_panel_activity_bar_left.h
// PURPOSE: B1 — Far-left activity bar.  A narrow, fixed-width vertical strip
//          of 28-px icon buttons that map 1:1 to HFSM EditorState domains and
//          Data-Index/Explorer mode for P3.
//
// DESIGN CONTRACT:
//  • Window name "ActivityBarLeft" — must match BuildDefaultWorkspace() routing.
//  • No tabs, no resize (node flags set by BuildDefaultWorkspace).
//  • Top section: one button per EditorState domain (Axiom/Road/Zoning/Lot/
//    Building/Water).  Click fires hfsm.handle_event(Tool_X, gs).
//  • Separator.
//  • Bottom section: Index-mode toggles (switch P3 to Explorer mode).
//    Uses RcMasterPanel::RequestCategory() to change the P3 tab externally.
//  • Active domain icon has a CyanAccent tinted background + border.

#pragma once

namespace RC_UI::Panels::ActivityBarLeft {

void Draw(float dt);

} // namespace RC_UI::Panels::ActivityBarLeft
