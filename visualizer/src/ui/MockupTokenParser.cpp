// FILE: MockupTokenParser.cpp
// PURPOSE: Parse CSS :root {} design tokens from RC_UI_Mockup.html into a
//          complete ImGuiStyle + ThemeProfile + layout ratios.
//
// Full color slot mapping — every ImGuiCol_ slot is assigned:
//
//   WindowBg              ← --bg-deep   (deepest layer — window chrome)
//   ChildBg               ← --bg-panel  (content areas / child windows)
//   PopupBg               ← --bg-panel
//   Border                ← --border    (dim, has embedded alpha in hex)
//   FrameBg               ← --bg-input  (input fields, sliders)
//   FrameBgHovered        ← --secondary-dim (teal glow, embedded alpha)
//   FrameBgActive         ← --primary-dim   (orange tint, embedded alpha)
//   TitleBg               ← --bg-elevated
//   TitleBgActive         ← --secondary @ 25% alpha
//   TitleBgCollapsed      ← --bg-deep @ 80%
//   MenuBarBg             ← --bg-panel
//   ScrollbarBg           ← --bg-deep
//   ScrollbarGrab         ← --text-dim @ 60%
//   ScrollbarGrabHovered  ← --secondary
//   ScrollbarGrabActive   ← --primary
//   CheckMark             ← --primary
//   SliderGrab            ← --primary
//   SliderGrabActive      ← --secondary
//   Button                ← --primary-dim (semi-transparent)
//   ButtonHovered         ← --primary @ 47%
//   ButtonActive          ← --primary (full)
//   Header                ← --secondary-dim
//   HeaderHovered         ← --secondary @ 30%
//   HeaderActive          ← --primary @ 40%
//   Separator             ← --border
//   SeparatorHovered      ← --secondary
//   SeparatorActive       ← --primary
//   Tab                   ← --bg-surface @ 85%
//   TabHovered            ← --primary @ 30%
//   TabActive             ← --primary @ 18%
//   TabUnfocused          ← --bg-deep
//   TabUnfocusedActive    ← --bg-panel
//   DockingPreview        ← --secondary @ 70%
//   DockingEmptyBg        ← --bg-deep
//   PlotLines             ← --secondary
//   PlotLinesHovered      ← --error
//   PlotHistogram         ← --primary
//   PlotHistogramHovered  ← --warning
//   TableHeaderBg         ← --bg-panel
//   TableBorderStrong     ← --border @ 30%
//   TableBorderLight      ← --border @ 15%
//   TextSelectedBg        ← --primary @ 32%
//   NavHighlight          ← --secondary
//   ModalWindowDimBg      ← --bg-deep @ 70%

#include "RogueCity/Visualizer/MockupTokenParser.hpp"
#include "RogueCity/App/UI/ThemeManager.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace RogueCity::Visualizer {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static std::string Trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last  = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

// ---------------------------------------------------------------------------
// HexToImU32 — #RGB / #RRGGBB / #RRGGBBAA → ImU32
// ---------------------------------------------------------------------------

std::optional<ImU32> MockupTokenParser::HexToImU32(const std::string& hex) {
    if (hex.empty() || hex[0] != '#') return std::nullopt;

    std::string s = hex.substr(1);

    // Expand 3-digit shorthand: #ABC → #AABBCC
    if (s.size() == 3) {
        std::string expanded;
        expanded.reserve(6);
        for (char c : s) { expanded += c; expanded += c; }
        s = std::move(expanded);
    }

    if (s.size() != 6 && s.size() != 8) return std::nullopt;

    try {
        const unsigned long val = std::stoul(s, nullptr, 16);
        uint8_t r, g, b, a = 255;
        if (s.size() == 8) {
            r = static_cast<uint8_t>((val >> 24) & 0xFF);
            g = static_cast<uint8_t>((val >> 16) & 0xFF);
            b = static_cast<uint8_t>((val >>  8) & 0xFF);
            a = static_cast<uint8_t>((val      ) & 0xFF);
        } else {
            r = static_cast<uint8_t>((val >> 16) & 0xFF);
            g = static_cast<uint8_t>((val >>  8) & 0xFF);
            b = static_cast<uint8_t>((val      ) & 0xFF);
        }
        return IM_COL32(r, g, b, a);
    } catch (...) {
        return std::nullopt;
    }
}

// ---------------------------------------------------------------------------
// ExtractCssVars — scan :root {} for --var: value; pairs
// ---------------------------------------------------------------------------

MockupTokenParser::VarMap
MockupTokenParser::ExtractCssVars(const std::string& css) {
    VarMap result;

    const auto rootPos = css.find(":root");
    if (rootPos == std::string::npos) return result;

    const auto braceOpen = css.find('{', rootPos);
    if (braceOpen == std::string::npos) return result;

    const auto braceClose = css.find('}', braceOpen);
    if (braceClose == std::string::npos) return result;

    const std::string block = css.substr(braceOpen + 1, braceClose - braceOpen - 1);

    std::istringstream ss(block);
    std::string line;
    while (std::getline(ss, line)) {
        line = Trim(line);
        if (line.empty() || line.rfind("--", 0) != 0) continue;

        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string name  = Trim(line.substr(2, colon - 2));
        std::string value = Trim(line.substr(colon + 1));

        const auto semi = value.find(';');
        if (semi != std::string::npos) value = Trim(value.substr(0, semi));

        const auto commentStart = value.find("/*");
        if (commentStart != std::string::npos) value = Trim(value.substr(0, commentStart));

        if (!name.empty() && !value.empty()) {
            result[name] = value;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// BuildImGuiStyle — full ImGuiStyle from the CSS var map
// ---------------------------------------------------------------------------

ImGuiStyle MockupTokenParser::BuildImGuiStyle(const VarMap& vars) {
    ImGuiStyle s;
    ImGui::StyleColorsDark(&s); // start from a clean dark baseline

    // ---- Helpers --------------------------------------------------------
    // Get a CSS var as ImVec4 (preserves embedded alpha from 8-digit hex)
    auto getColor = [&](const std::string& var, ImVec4 def) -> ImVec4 {
        const auto it = vars.find(var);
        if (it == vars.end()) return def;
        const auto col = HexToImU32(it->second);
        return col ? ImGui::ColorConvertU32ToFloat4(*col) : def;
    };

    // Get a CSS var as ImVec4, then override its alpha
    auto getColorA = [&](const std::string& var, float alpha, ImVec4 def) -> ImVec4 {
        ImVec4 v = getColor(var, def);
        v.w = alpha;
        return v;
    };

    // Get a CSS var as float (strips "px" suffix)
    auto getFloat = [&](const std::string& var, float def) -> float {
        const auto it = vars.find(var);
        if (it == vars.end()) return def;
        std::string val = it->second;
        const auto pxPos = val.find("px");
        if (pxPos != std::string::npos) val = val.substr(0, pxPos);
        try { return std::stof(val); } catch (...) { return def; }
    };

    // ---- Source values from CSS vars ------------------------------------
    const ImVec4 def_zero(0, 0, 0, 0);

    // Backgrounds (layered dark → elevated)
    const ImVec4 bg_deep     = getColor("bg-deep",     ImVec4(0.024f, 0.043f, 0.063f, 1.0f));
    const ImVec4 bg_panel    = getColor("bg-panel",    ImVec4(0.039f, 0.067f, 0.094f, 1.0f));
    const ImVec4 bg_surface  = getColor("bg-surface",  ImVec4(0.055f, 0.094f, 0.125f, 1.0f));
    const ImVec4 bg_elevated = getColor("bg-elevated", ImVec4(0.071f, 0.122f, 0.165f, 1.0f));
    const ImVec4 bg_input    = getColor("bg-input",    bg_deep);

    // Accent colors
    const ImVec4 primary     = getColor("primary",     ImVec4(0.839f, 0.471f, 0.282f, 1.0f));
    const ImVec4 secondary   = getColor("secondary",   ImVec4(0.408f, 0.839f, 0.847f, 1.0f));

    // Semi-transparent variants — read embedded alpha from CSS (e.g. #D6784833)
    const ImVec4 primary_dim   = getColor("primary-dim",   getColorA("primary",   0.20f, primary));
    const ImVec4 secondary_dim = getColor("secondary-dim", getColorA("secondary", 0.13f, secondary));
    const ImVec4 border_dim    = getColor("border",        getColorA("secondary", 0.20f, secondary));

    // Text
    const ImVec4 text       = getColor("text",       ImVec4(0.784f, 0.847f, 0.878f, 1.0f));
    const ImVec4 text_dim   = getColor("text-dim",   ImVec4(0.376f, 0.471f, 0.533f, 1.0f));
    const ImVec4 text_label = getColor("text-label", ImVec4(0.541f, 0.612f, 0.659f, 1.0f));

    // State colors
    const ImVec4 success = getColor("success", ImVec4(0.463f, 0.769f, 0.498f, 1.0f));
    const ImVec4 warning = getColor("warning", ImVec4(0.910f, 0.635f, 0.306f, 1.0f));
    const ImVec4 error   = getColor("error",   ImVec4(0.769f, 0.290f, 0.235f, 1.0f));

    // ---- Color slot assignments -----------------------------------------
    s.Colors[ImGuiCol_Text]         = text;
    s.Colors[ImGuiCol_TextDisabled] = text_dim;

    s.Colors[ImGuiCol_WindowBg] = bg_deep;
    s.Colors[ImGuiCol_ChildBg]  = bg_panel;
    s.Colors[ImGuiCol_PopupBg]  = bg_panel;

    s.Colors[ImGuiCol_Border]       = border_dim;
    s.Colors[ImGuiCol_BorderShadow] = def_zero;

    s.Colors[ImGuiCol_FrameBg]        = bg_input;
    s.Colors[ImGuiCol_FrameBgHovered] = secondary_dim;
    s.Colors[ImGuiCol_FrameBgActive]  = primary_dim;

    s.Colors[ImGuiCol_TitleBg]          = bg_elevated;
    s.Colors[ImGuiCol_TitleBgActive]    = getColorA("secondary", 0.25f, secondary);
    s.Colors[ImGuiCol_TitleBgCollapsed] = getColorA("bg-deep",   0.80f, bg_deep);

    s.Colors[ImGuiCol_MenuBarBg] = bg_panel;

    s.Colors[ImGuiCol_ScrollbarBg]          = bg_deep;
    s.Colors[ImGuiCol_ScrollbarGrab]         = getColorA("text-dim", 0.60f, text_dim);
    s.Colors[ImGuiCol_ScrollbarGrabHovered]  = secondary;
    s.Colors[ImGuiCol_ScrollbarGrabActive]   = primary;

    s.Colors[ImGuiCol_CheckMark]        = primary;
    s.Colors[ImGuiCol_SliderGrab]       = primary;
    s.Colors[ImGuiCol_SliderGrabActive] = secondary;

    s.Colors[ImGuiCol_Button]        = primary_dim;
    s.Colors[ImGuiCol_ButtonHovered] = getColorA("primary", 0.47f, primary);
    s.Colors[ImGuiCol_ButtonActive]  = primary;

    s.Colors[ImGuiCol_Header]        = secondary_dim;
    s.Colors[ImGuiCol_HeaderHovered] = getColorA("secondary", 0.30f, secondary);
    s.Colors[ImGuiCol_HeaderActive]  = getColorA("primary",   0.40f, primary);

    s.Colors[ImGuiCol_Separator]        = border_dim;
    s.Colors[ImGuiCol_SeparatorHovered] = secondary;
    s.Colors[ImGuiCol_SeparatorActive]  = primary;

    s.Colors[ImGuiCol_ResizeGrip]        = secondary_dim;
    s.Colors[ImGuiCol_ResizeGripHovered] = secondary;
    s.Colors[ImGuiCol_ResizeGripActive]  = primary;

    // Tabs: match mockup .tab CSS
    s.Colors[ImGuiCol_Tab]                = getColorA("bg-surface",  0.85f, bg_surface);
    s.Colors[ImGuiCol_TabHovered]         = getColorA("primary",     0.30f, primary);
    s.Colors[ImGuiCol_TabActive]          = getColorA("primary",     0.18f, primary);
    s.Colors[ImGuiCol_TabUnfocused]       = bg_deep;
    s.Colors[ImGuiCol_TabUnfocusedActive] = bg_panel;

#if defined(IMGUI_HAS_DOCK)
    s.Colors[ImGuiCol_DockingPreview]  = getColorA("secondary", 0.70f, secondary);
    s.Colors[ImGuiCol_DockingEmptyBg]  = bg_deep;
#endif

    s.Colors[ImGuiCol_PlotLines]            = secondary;
    s.Colors[ImGuiCol_PlotLinesHovered]     = error;
    s.Colors[ImGuiCol_PlotHistogram]        = primary;
    s.Colors[ImGuiCol_PlotHistogramHovered] = warning;

    s.Colors[ImGuiCol_TableHeaderBg]     = bg_panel;
    s.Colors[ImGuiCol_TableBorderStrong] = getColorA("secondary", 0.30f, secondary);
    s.Colors[ImGuiCol_TableBorderLight]  = getColorA("secondary", 0.15f, secondary);
    s.Colors[ImGuiCol_TableRowBg]        = def_zero;
    s.Colors[ImGuiCol_TableRowBgAlt]     = getColorA("bg-panel",  0.25f, bg_panel);

    s.Colors[ImGuiCol_TextSelectedBg]  = getColorA("primary",    0.32f, primary);
    s.Colors[ImGuiCol_DragDropTarget]  = warning;
    s.Colors[ImGuiCol_NavHighlight]    = secondary;
    s.Colors[ImGuiCol_NavWindowingHighlight] = getColorA("secondary", 0.70f, secondary);
    s.Colors[ImGuiCol_NavWindowingDimBg]     = getColorA("bg-deep",   0.40f, bg_deep);
    s.Colors[ImGuiCol_ModalWindowDimBg]      = getColorA("bg-deep",   0.70f, bg_deep);

    // ---- Geometry — all driven by CSS vars ------------------------------
    s.WindowRounding    = getFloat("window-rounding",  0.0f);
    s.ChildRounding     = getFloat("frame-rounding",   0.0f);
    s.FrameRounding     = getFloat("frame-rounding",   0.0f);
    s.PopupRounding     = getFloat("window-rounding",  0.0f);
    s.ScrollbarRounding = 0.0f;
    s.GrabRounding      = getFloat("frame-rounding",   0.0f);
    s.TabRounding       = getFloat("tab-rounding",     0.0f);

    s.WindowBorderSize = getFloat("border-size",       1.0f);
    s.ChildBorderSize  = getFloat("border-size",       1.0f);
    s.PopupBorderSize  = getFloat("border-size",       1.0f);
    s.FrameBorderSize  = getFloat("frame-border-size", 1.0f);

    const float wpx = getFloat("window-padding",   8.0f);
    const float wpy = getFloat("window-padding-y", wpx * 0.75f);
    s.WindowPadding    = ImVec2(wpx, wpy);

    const float fpx = getFloat("frame-padding",   6.0f);
    const float fpy = getFloat("frame-padding-y", fpx * 0.66f);
    s.FramePadding     = ImVec2(fpx, fpy);

    const float isx = getFloat("item-spacing",   8.0f);
    const float isy = getFloat("item-spacing-y", 4.0f);
    s.ItemSpacing      = ImVec2(isx, isy);
    s.ItemInnerSpacing = ImVec2(isx * 0.5f, isy);
    s.CellPadding      = ImVec2(fpx * 0.66f, fpy * 0.5f);

    s.ScrollbarSize  = getFloat("scrollbar-size",  10.0f);
    s.GrabMinSize    = 8.0f;
    s.IndentSpacing  = getFloat("indent-spacing",  16.0f);

    return s;
}

// ---------------------------------------------------------------------------
// VarsToTheme — for ThemeManager registration (persona switching etc.)
// ---------------------------------------------------------------------------

UI::ThemeProfile MockupTokenParser::VarsToTheme(const VarMap& vars,
                                                 const UI::ThemeProfile& fallback) {
    UI::ThemeProfile t = fallback;

    auto getColor = [&](const std::string& var, ImU32 def) -> ImU32 {
        const auto it = vars.find(var);
        if (it == vars.end()) return def;
        const auto col = HexToImU32(it->second);
        return col.value_or(def);
    };

    t.background_dark  = getColor("bg-deep",       t.background_dark);
    t.panel_background = getColor("bg-panel",       t.panel_background);
    t.primary_accent   = getColor("primary",        t.primary_accent);
    t.secondary_accent = getColor("secondary",      t.secondary_accent);
    t.text_primary     = getColor("text",           t.text_primary);
    t.text_secondary   = getColor("text-dim",       t.text_secondary);
    t.text_disabled    = getColor("text-label",     t.text_disabled);
    t.border_accent    = getColor("border-active",  t.border_accent);
    t.success_color    = getColor("success",        t.success_color);
    t.warning_color    = getColor("warning",        t.warning_color);
    t.error_color      = getColor("error",          t.error_color);

    t.name = "Mockup";
    return t;
}

// ---------------------------------------------------------------------------
// VarsToLayout
// ---------------------------------------------------------------------------

MockupLayoutTokens MockupTokenParser::VarsToLayout(const VarMap& vars) {
    MockupLayoutTokens layout;

    auto getFloat = [&](const std::string& var, float def) -> float {
        const auto it = vars.find(var);
        if (it == vars.end()) return def;
        try {
            float v = std::stof(it->second);
            if (v < 0.1f) v = 0.1f;
            if (v > 0.9f) v = 0.9f;
            return v;
        } catch (...) { return def; }
    };

    layout.left_panel_ratio  = getFloat("master-ratio",    0.32f);
    layout.right_panel_ratio = getFloat("right-ratio",     0.22f);
    layout.tool_deck_ratio   = getFloat("tool-deck-ratio", 0.24f);
    return layout;
}

// ---------------------------------------------------------------------------
// ParseFromCssString
// ---------------------------------------------------------------------------

std::optional<MockupParseResult> MockupTokenParser::ParseFromCssString(
    const std::string& css)
{
    auto vars = ExtractCssVars(css);
    if (vars.empty()) return std::nullopt;

    MockupParseResult result;
    result.imgui_style = BuildImGuiStyle(vars);
    result.theme  = VarsToTheme(vars,
        RogueCity::UI::ThemeManager::Instance().GetActiveTheme());
    result.layout = VarsToLayout(vars);
    result.valid  = true;
    return result;
}

// ---------------------------------------------------------------------------
// ParseFromHtmlFile
// ---------------------------------------------------------------------------

std::optional<MockupParseResult> MockupTokenParser::ParseFromHtmlFile(
    const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt; // silent fail

    const std::string content(
        (std::istreambuf_iterator<char>(file)),
         std::istreambuf_iterator<char>());

    const auto styleOpen  = content.find("<style");
    const auto styleClose = content.find("</style>");
    if (styleOpen == std::string::npos || styleClose == std::string::npos) {
        std::cerr << "[MockupTokenParser] No <style> block in: " << path << "\n";
        return std::nullopt;
    }

    const auto styleTagEnd = content.find('>', styleOpen);
    if (styleTagEnd == std::string::npos) return std::nullopt;

    const std::string css = content.substr(
        styleTagEnd + 1, styleClose - styleTagEnd - 1);

    return ParseFromCssString(css);
}

} // namespace RogueCity::Visualizer
