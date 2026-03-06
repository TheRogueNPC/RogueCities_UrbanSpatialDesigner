// FILE: visualizer/src/ui/panels/rc_panel_activity_bar_right.h
// PURPOSE: B2 — Far-right activity bar.  A narrow, fixed-width vertical strip
//          of icon buttons that route visibility to P4 Inspector tabs and open
//          global config panels.
//
// DESIGN CONTRACT:
//  • Window name "ActivityBarRight" — matches BuildDefaultWorkspace() routing.
//  • No tabs, no resize (node flags set by BuildDefaultWorkspace).
//  • Clicking an icon sets the P4 requested tab via RcInspectorSidebar::RequestTab().
//  • Bottom section: global config icons (Workspace, Theme).
//  • Active P4 tab icon has an AmberGlow right-side accent bar.

#pragma once

namespace RC_UI::Panels::ActivityBarRight {

void Draw(float dt);

} // namespace RC_UI::Panels::ActivityBarRight
