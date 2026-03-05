// FILE: visualizer/src/ui/panels/rc_panel_city_health.h
// PURPOSE: "Health" inspector tab — runs CityInvariantsChecker on demand and
//          displays a colour-coded summary + scrollable violation list.
#pragma once

namespace RC_UI::Panels::CityHealth {

/// Call once per frame when the Health tab is active.
void DrawContent(float dt);

/// Mark the cached result stale so the next RunCheck call re-evaluates.
/// Call this whenever a generation pass completes.
void MarkDirty();

} // namespace RC_UI::Panels::CityHealth
