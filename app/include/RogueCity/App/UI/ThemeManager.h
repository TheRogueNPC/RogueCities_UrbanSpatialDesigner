/**
 * @file ThemeManager.h
 * @brief Multi-theme system for Y2K Cockpit UI in RogueCities Urban Spatial Designer.
 *
 * Provides management and application of UI themes for ImGui-based interfaces.
 * Supports built-in and custom themes, loading/saving from JSON, and runtime editing.
 *
 * @namespace RogueCity::UI
 * 
 * @struct ThemeProfile
 *   Represents a theme color profile with 12 key tokens for accents, backgrounds, text, and borders.
 *   - name: Theme name.
 *   - primary_accent: Main interactive elements color.
 *   - secondary_accent: Secondary highlights color.
 *   - success_color: Success state color.
 *   - warning_color: Warning/caution color.
 *   - error_color: Error/danger color.
 *   - background_dark: Main viewport/window background color.
 *   - panel_background: Panel background color.
 *   - grid_overlay: Subtle grid/guides color (with alpha).
 *   - text_primary: Main text color.
 *   - text_secondary: Secondary text color.
 *   - text_disabled: Disabled text color.
 *   - border_accent: Window borders/active elements color.
 *
 * @class ThemeManager
 *   Singleton class for managing UI themes.
 *   - Load, register, and save themes.
 *   - Apply themes to ImGui.
 *   - Access built-in and custom themes.
 *   - Edit active theme at runtime.
 *
 * @namespace Themes
 *   Provides built-in theme presets for UI preview and selection:
 *   - Default
 *   - Soviet (alias for DowntownCity)
 *   - RedRetro
 *   - DowntownCity
 *   - RedlightDistrict
 *   - CyberPunk
 *   - Tron
 */
// ThemeManager.h - Multi-theme system for Y2K Cockpit UI
#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace RogueCity::UI {

/// Theme color profile with 12 key tokens
struct ThemeProfile {
    std::string name;
    
    // Primary accent colors
    ImU32 primary_accent;      // Main interactive elements
    ImU32 secondary_accent;    // Secondary highlights
    ImU32 success_color;       // Success states
    ImU32 warning_color;       // Warnings, caution
    ImU32 error_color;         // Errors, danger
    
    // Background colors
    ImU32 background_dark;     // Main viewport/window background
    ImU32 panel_background;    // Panel backgrounds
    ImU32 grid_overlay;        // Subtle grid/guides (with alpha)
    
    // Text colors
    ImU32 text_primary;        // Main text
    ImU32 text_secondary;      // Secondary text
    ImU32 text_disabled;       // Disabled text
    
    // Border accent
    ImU32 border_accent;       // Window borders, active elements
};

/// Manages UI themes and applies them to ImGui
class ThemeManager {
public:
    static ThemeManager& Instance();
    
    /// Load a theme by name (returns false if not found)
    bool LoadTheme(const std::string& name);
    
    /// Get the currently active theme
    const ThemeProfile& GetActiveTheme() const { return m_activeTheme; }
    std::string GetActiveThemeName() const { return m_activeThemeName; }
    
    /// Get list of available theme names
    std::vector<std::string> GetAvailableThemes() const;
    
    /// Register a custom theme (returns false if name already exists)
    bool RegisterTheme(const ThemeProfile& profile);
    
    /// Save custom theme to JSON config (in AI/config/themes/)
    bool SaveThemeToFile(const ThemeProfile& profile, std::string* outError = nullptr);
    
    /// Load custom themes from JSON config directory
    void LoadCustomThemes();
    
    /// Apply the current theme to ImGui style
    void ApplyToImGui();
    
    /// Get built-in theme by name (for UI preview)
    const ThemeProfile* GetTheme(const std::string& name) const;
    
    /// Check if a theme is built-in (non-editable)
    bool IsBuiltInTheme(const std::string& name) const;
    
    /// Create a custom theme from the currently active theme
    /// Returns the new theme name, or empty string on failure
    std::string CreateCustomFromActive(const std::string& basename = "Custom");
    
    /// Get non-const reference to active theme (for UI editing)
    ThemeProfile& GetActiveThemeMutable() { return m_activeTheme; }
    
private:
    ThemeManager();
    void RegisterBuiltInThemes();
    
    std::vector<std::string> m_builtInThemeNames;
    
    std::unordered_map<std::string, ThemeProfile> m_themes;
    ThemeProfile m_activeTheme;
    std::string m_activeThemeName;
};

// === BUILT-IN THEME PRESETS ===
namespace Themes {
    ThemeProfile Default();          // Rogue baseline preset (formerly Default)
    ThemeProfile Soviet();           // Backward-compat alias -> DowntownCity
    ThemeProfile RedRetro();         // #D12128, #FAE3AC, #01344F
    ThemeProfile DowntownCity();     // #1B1B2F, #E94560, #0F3460
    ThemeProfile RedlightDistrict(); // #C70039, #FF5733, #900C3F
    ThemeProfile CyberPunk();       // #8A509F, #FA99C5, #0A2670
    ThemeProfile Tron();            // #0FF, #F0F, #FF0 on black
}

} // namespace RogueCity::UI
