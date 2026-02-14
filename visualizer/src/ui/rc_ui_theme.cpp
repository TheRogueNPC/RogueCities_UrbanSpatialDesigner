// FILE: visualizer/src/ui/rc_ui_theme.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Centralized palette and theme application.
//todo add utility functions for User color manipulation (e.g. brightness adjustment, alpha modulation) to support dynamic theming and state-based color changes in the future.
//todo track and store use config for theme customization in user settings, allowing users to create and save their own themes - plug in Ai for "Smart Assignment"
#include "ui/rc_ui_theme.h"

namespace RC_UI {
// Centralized color palette for the UI, defined as a set of named ImVec4 constants for easy reference and consistency across the application.
namespace Palette {
const ImVec4 DeepSpace = ImVec4(0.06f, 0.07f, 0.09f, 1.0f);
const ImVec4 Slate = ImVec4(0.16f, 0.18f, 0.22f, 1.0f);
const ImVec4 Nebula = ImVec4(0.24f, 0.26f, 0.32f, 1.0f);
const ImVec4 Amber = ImVec4(0.95f, 0.62f, 0.18f, 1.0f);
const ImVec4 Cyan = ImVec4(0.30f, 0.76f, 0.86f, 1.0f);
const ImVec4 Magenta = ImVec4(0.78f, 0.38f, 0.74f, 1.0f);
const ImVec4 Green = ImVec4(0.36f, 0.80f, 0.52f, 1.0f);
} // namespace Palette

const SystemZone ZoneAxiom = {Palette::Slate, Palette::Amber, Palette::Magenta};
const SystemZone ZoneTransit = {Palette::Nebula, Palette::Cyan, Palette::Amber};
const SystemZone ZoneTelemetry = {Palette::Slate, Palette::Green, Palette::Cyan};

const ImVec4 ColorBG = Palette::DeepSpace;
const ImVec4 ColorPanel = Palette::Slate;
const ImVec4 ColorText = ImVec4(0.90f, 0.92f, 0.95f, 1.0f);
const ImVec4 ColorAccentA = Palette::Amber;
const ImVec4 ColorAccentB = Palette::Cyan;
const ImVec4 ColorWarn = ImVec4(0.95f, 0.35f, 0.24f, 1.0f);
const ImVec4 ColorGood = Palette::Green;
// Additional semantic colors can be defined here as needed, e.g. for specific UI states, categories, or data types.
void ApplyTheme()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 16.0f;
    style.FrameRounding = 12.0f;
    style.ChildRounding = 12.0f;
    style.PopupRounding = 12.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding = 10.0f;
    style.WindowPadding = ImVec2(18.0f, 16.0f);
    style.FramePadding = ImVec2(14.0f, 8.0f);
    style.ItemSpacing = ImVec2(12.0f, 10.0f);
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;

    // ADDED (visualizer/src/ui/rc_ui_theme.cpp): Set base semantic colors.
    style.Colors[ImGuiCol_WindowBg] = ColorBG;
    style.Colors[ImGuiCol_ChildBg] = ColorBG;
    style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
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
}

} // namespace RC_UI
