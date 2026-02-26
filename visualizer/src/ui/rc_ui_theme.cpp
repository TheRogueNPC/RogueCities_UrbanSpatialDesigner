// FILE: visualizer/src/ui/rc_ui_theme.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Centralized palette and theme application.
// todo add utility functions for User color manipulation (e.g. brightness
// adjustment, alpha modulation) to support dynamic theming and state-based
// color changes in the future. todo track and store use config for theme
// customization in user settings, allowing users to create and save their own
// themes - plug in Ai for "Smart Assignment"
#include "ui/rc_ui_theme.h"
#include "ui/rc_ui_tokens.h"

namespace RC_UI {

// Helper to convert ImU32 to ImVec4
inline ImVec4 ToVec4(ImU32 col) { return ImGui::ColorConvertU32ToFloat4(col); }

// Centralized color palette for the UI, defined as a set of named ImVec4
// constants for easy reference and consistency across the application.
namespace Palette {
const ImVec4 DeepSpace = ToVec4(UITokens::BackgroundDark);
const ImVec4 Slate = ToVec4(UITokens::PanelBackground);
const ImVec4 Nebula =
    ToVec4(IM_COL32(40, 50, 70, 255)); // Derived from GridOverlay but opaque
const ImVec4 Amber = ToVec4(UITokens::AmberGlow);
const ImVec4 Cyan = ToVec4(UITokens::CyanAccent);
const ImVec4 Magenta = ToVec4(UITokens::MagentaHighlight);
const ImVec4 Green = ToVec4(UITokens::SuccessGreen);
} // namespace Palette

const SystemZone ZoneAxiom = {Palette::Slate, Palette::Amber, Palette::Magenta};
const SystemZone ZoneTransit = {Palette::Nebula, Palette::Cyan, Palette::Amber};
const SystemZone ZoneTelemetry = {Palette::Slate, Palette::Green,
                                  Palette::Cyan};

const ImVec4 ColorBG = Palette::DeepSpace;
const ImVec4 ColorPanel = Palette::Slate;
const ImVec4 ColorText = ToVec4(UITokens::TextPrimary);
const ImVec4 ColorAccentA = Palette::Amber;
const ImVec4 ColorAccentB = Palette::Cyan;
const ImVec4 ColorWarn = ToVec4(UITokens::ErrorRed);
const ImVec4 ColorGood = Palette::Green;

void ApplyTheme() {
  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();

  style.WindowRounding = UITokens::RoundingPanel;
  style.FrameRounding = UITokens::RoundingButton;
  style.ChildRounding = UITokens::RoundingButton;
  style.PopupRounding = UITokens::RoundingButton;
  style.ScrollbarRounding = UITokens::RoundingButton;
  style.GrabRounding = UITokens::RoundingButton;

  style.WindowPadding = ImVec2(UITokens::SpaceM, UITokens::SpaceM);
  style.FramePadding = ImVec2(UITokens::SpaceS + 6.0f, UITokens::SpaceS);
  style.ItemSpacing = ImVec2(UITokens::SpaceS + 4.0f, UITokens::SpaceS + 2.0f);

  style.WindowBorderSize = UITokens::BorderThin;
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = UITokens::BorderThin;

  style.Colors[ImGuiCol_WindowBg] = ColorBG;
  style.Colors[ImGuiCol_ChildBg] = ColorBG;
  style.Colors[ImGuiCol_Border] = ToVec4(UITokens::GridOverlay);
  style.Colors[ImGuiCol_Text] = ColorText;
  style.Colors[ImGuiCol_TitleBg] = ColorPanel;
  style.Colors[ImGuiCol_TitleBgActive] = ColorPanel;
  style.Colors[ImGuiCol_FrameBg] = Palette::Nebula;
  style.Colors[ImGuiCol_FrameBgHovered] = Palette::Slate;
  style.Colors[ImGuiCol_FrameBgActive] = Palette::Slate;
  style.Colors[ImGuiCol_Button] = Palette::Nebula;
  style.Colors[ImGuiCol_ButtonHovered] = Palette::Slate;
  style.Colors[ImGuiCol_ButtonActive] = Palette::Slate;
  style.Colors[ImGuiCol_CheckMark] = ColorAccentA;
  style.Colors[ImGuiCol_SliderGrab] = ColorAccentB;
  style.Colors[ImGuiCol_SliderGrabActive] = ColorAccentA;
  style.Colors[ImGuiCol_Header] = Palette::Slate;
  style.Colors[ImGuiCol_HeaderHovered] = ToVec4(IM_COL32(60, 70, 90, 255));
  style.Colors[ImGuiCol_HeaderActive] = ColorAccentB;
}

} // namespace RC_UI
