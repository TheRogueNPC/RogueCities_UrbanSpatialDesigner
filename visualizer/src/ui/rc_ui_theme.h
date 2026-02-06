// FILE: visualizer/src/ui/rc_ui_theme.h (RogueCities_UrbanSpatialDesigner)
// PURPOSE: LCARS-inspired palette + semantic color tokens for the UI layer.
#pragma once

#include <imgui.h>

namespace RC_UI {

struct SystemZone {
    ImVec4 base;
    ImVec4 accent;
    ImVec4 highlight;
};

namespace Palette {
extern const ImVec4 DeepSpace;
extern const ImVec4 Slate;
extern const ImVec4 Nebula;
extern const ImVec4 Amber;
extern const ImVec4 Cyan;
extern const ImVec4 Magenta;
extern const ImVec4 Green;
} // namespace Palette

extern const SystemZone ZoneAxiom;
extern const SystemZone ZoneTransit;
extern const SystemZone ZoneTelemetry;

extern const ImVec4 ColorBG;
extern const ImVec4 ColorPanel;
extern const ImVec4 ColorText;
extern const ImVec4 ColorAccentA;
extern const ImVec4 ColorAccentB;
extern const ImVec4 ColorWarn;
extern const ImVec4 ColorGood;

void ApplyTheme();

} // namespace RC_UI
