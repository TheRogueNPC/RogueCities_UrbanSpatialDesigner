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
    
private:
    ThemeManager();
    void RegisterBuiltInThemes();
    
    std::unordered_map<std::string, ThemeProfile> m_themes;
    ThemeProfile m_activeTheme;
    std::string m_activeThemeName;
};

// === BUILT-IN THEME PRESETS ===
namespace Themes {
    ThemeProfile Default();      // Current cyan/green/yellow tokens
    ThemeProfile RedRetro();     // #D12128, #FAE3AC, #01344F
    ThemeProfile Soviet();       // #C0301E, #F6DA9D, #000000
    ThemeProfile CyberPunk();    // #8A509F, #FA99C5, #0A2670
}

} // namespace RogueCity::UI
