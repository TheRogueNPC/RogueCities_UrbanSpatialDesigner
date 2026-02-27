// FILE: MockupTokenParser.hpp
// PURPOSE: Parse CSS :root {} design tokens from RC_UI_Mockup.html
//          into a complete ImGuiStyle + layout ratios for live hot-reload.
//
// Layer constraint: visualizer/ — allowed to call app/ ThemeManager.
// No regex, no new vcpkg deps. Plain std::string operations only.
//
// CSS → ImGui color slot mapping (see MockupTokenParser.cpp for full table):
//   --bg-deep        → WindowBg, DockingEmptyBg
//   --bg-panel       → ChildBg, PopupBg, MenuBarBg
//   --bg-surface     → Tab (unfocused)
//   --bg-elevated    → TitleBg, TableHeaderBg
//   --bg-input       → FrameBg
//   --primary        → Button, SliderGrab, CheckMark, TabActive
//   --primary-dim    → ButtonHovered, FrameBgActive   (uses embedded alpha)
//   --secondary      → Border (active), ScrollbarGrabHovered
//   --secondary-dim  → Header, FrameBgHovered         (uses embedded alpha)
//   --border         → Border                          (uses embedded alpha)
//   --text           → Text
//   --text-dim       → TextDisabled
//   --success / --warning / --error → plot/state colors
//
// Geometry CSS vars (add to :root to control live):
//   --window-rounding, --frame-rounding, --tab-rounding
//   --border-size, --frame-border-size
//   --window-padding, --frame-padding, --item-spacing, --item-spacing-y
//   --scrollbar-size, --indent-spacing

#pragma once

#include "RogueCity/App/UI/ThemeManager.h"

#include <imgui.h>
#include <optional>
#include <string>
#include <unordered_map>

namespace RogueCity::Visualizer {

/// Layout ratios extracted from CSS vars (--master-ratio etc.).
struct MockupLayoutTokens {
    float left_panel_ratio   = 0.32f;
    float right_panel_ratio  = 0.22f;
    float tool_deck_ratio    = 0.24f;
};

/// Full result of a successful mockup parse.
/// Apply imgui_style directly to ImGui::GetStyle() for 1-to-1 visual match.
struct MockupParseResult {
    UI::ThemeProfile   theme;        ///< For ThemeManager sync (persona switches etc.)
    MockupLayoutTokens layout;       ///< Dock ratio preferences
    ImGuiStyle         imgui_style;  ///< Complete style — apply to ImGui::GetStyle()
    bool               valid = false;
};

class MockupTokenParser {
public:
    /// Parse from an HTML file on disk.
    /// Returns nullopt silently if the file is missing or has no :root block.
    static std::optional<MockupParseResult> ParseFromHtmlFile(const std::string& path);

    /// Parse from raw CSS text (e.g. the inner content of a <style> tag).
    static std::optional<MockupParseResult> ParseFromCssString(const std::string& css);

private:
    using VarMap = std::unordered_map<std::string, std::string>;

    /// Extract --varname: value; pairs from the CSS :root {} block.
    static VarMap ExtractCssVars(const std::string& css);

    /// Convert a CSS hex colour string (#RGB / #RRGGBB / #RRGGBBAA) to ImU32.
    static std::optional<ImU32> HexToImU32(const std::string& hex);

    /// Build a complete ImGuiStyle from the var map.
    /// Geometry vars (--window-padding, etc.) drive padding/rounding.
    static ImGuiStyle BuildImGuiStyle(const VarMap& vars);

    /// Map the var table → ThemeProfile (for ThemeManager registration).
    static UI::ThemeProfile VarsToTheme(const VarMap& vars,
                                        const UI::ThemeProfile& fallback);

    /// Map the var table → MockupLayoutTokens.
    static MockupLayoutTokens VarsToLayout(const VarMap& vars);
};

} // namespace RogueCity::Visualizer
