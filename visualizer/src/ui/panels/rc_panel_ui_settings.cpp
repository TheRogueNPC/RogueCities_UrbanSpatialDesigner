// FILE: rc_panel_ui_settings.cpp
// PURPOSE: UI Settings panel for theme selection and customization

#include "ui/panels/rc_panel_ui_settings.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/rc_ui_animation.h"
#include "ui/introspection/UiIntrospection.h"
#include "RogueCity/App/UI/ThemeManager.h"
#include "RogueCity/App/UI/DesignSystem.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <imgui.h>
#include <algorithm>
#include <array>
#include <vector>
#include <string>

namespace RC_UI::Panels::UiSettings {

namespace {
    static bool s_open = false;
    static int s_selected_theme_index = 0;
    static char s_custom_theme_name[64] = "MyTheme";
    static bool s_layout_prefs_initialized = false;
    static RC_UI::DockLayoutPreferences s_layout_prefs_edit{};
    
    // Theme editing state
    static bool s_auto_save_custom_themes = true;
    static bool s_show_hex_by_default = true;
    static bool s_theme_was_edited = false;
    static float s_theme_save_timer = 0.0f;
    static constexpr float kSaveDebounceSeconds = 2.0f;
}

bool IsOpen() {
    return s_open;
}

void Toggle() {
    s_open = !s_open;
}

void DrawContent(float dt)
{
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    auto& theme_mgr = RogueCity::UI::ThemeManager::Instance();

    if (!s_layout_prefs_initialized) {
        s_layout_prefs_edit = RC_UI::GetDockLayoutPreferences();
        s_layout_prefs_initialized = true;
    }
    
    // Handle debounced theme auto-save
    if (s_theme_was_edited && s_auto_save_custom_themes) {
        s_theme_save_timer += dt;
        if (s_theme_save_timer >= kSaveDebounceSeconds) {
            std::string error;
            const auto& active = theme_mgr.GetActiveTheme();
            if (!theme_mgr.IsBuiltInTheme(active.name)) {
                theme_mgr.SaveThemeToFile(active, &error);
            }
            s_theme_was_edited = false;
            s_theme_save_timer = 0.0f;
        }
    }
    
    bool panel_focused = ImGui::IsWindowFocused();
    
    // ======================================================================
    // THEME SELECTION (with breathing animation)
    // ======================================================================
    RC_UI::AnimationHelpers::BeginBreathingHeader("Theme Selection", UITokens::InfoBlue, panel_focused);
    {
        
        std::vector<std::string> theme_names = theme_mgr.GetAvailableThemes();
        std::vector<const char*> theme_names_cstr;
        for (const auto& name : theme_names) {
            theme_names_cstr.push_back(name.c_str());
        }
        
        // Find current theme index
        std::string current_theme = theme_mgr.GetActiveThemeName();
        for (size_t i = 0; i < theme_names.size(); ++i) {
            if (theme_names[i] == current_theme) {
                s_selected_theme_index = static_cast<int>(i);
                break;
            }
        }
        
        ImGui::Text("Active Theme: %s", current_theme.c_str());
        ImGui::Spacing();
        
        if (ImGui::Combo("Available Themes", &s_selected_theme_index, theme_names_cstr.data(), static_cast<int>(theme_names_cstr.size()))) {
            // Theme changed - apply it
            theme_mgr.LoadTheme(theme_names[s_selected_theme_index]);
            gs.config.active_theme = theme_names[s_selected_theme_index];
        }
        uiint.RegisterWidget({"combo", "Available Themes", "ui.theme_selector", {"ui", "settings"}});
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Theme color editor (interactive)
        RogueCity::UI::ThemeProfile& active_theme_mut = theme_mgr.GetActiveThemeMutable();
        ImGui::TextUnformatted("Theme Colors (Click to Edit):");
        ImGui::Spacing();
        
        const ImGuiColorEditFlags color_flags = 
            ImGuiColorEditFlags_DisplayHex | 
            ImGuiColorEditFlags_InputRGB | 
            ImGuiColorEditFlags_AlphaBar;
        
        bool any_color_edited = false;
        
        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Primary");
        ImGui::SameLine(120.0f);
        if (ImGui::ColorEdit4("##Primary", reinterpret_cast<float*>(&active_theme_mut.primary_accent), color_flags)) {
            any_color_edited = true;
        }
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Secondary");
        ImGui::SameLine(120.0f);
        if (ImGui::ColorEdit4("##Secondary", reinterpret_cast<float*>(&active_theme_mut.secondary_accent), color_flags)) {
            any_color_edited = true;
        }
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Success");
        ImGui::SameLine(120.0f);
        if (ImGui::ColorEdit4("##Success", reinterpret_cast<float*>(&active_theme_mut.success_color), color_flags)) {
            any_color_edited = true;
        }
        ImGui::EndGroup();
        
        ImGui::SameLine(420.0f);
        
        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Warning");
        ImGui::SameLine(120.0f);
        if (ImGui::ColorEdit4("##Warning", reinterpret_cast<float*>(&active_theme_mut.warning_color), color_flags)) {
            any_color_edited = true;
        }
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Error");
        ImGui::SameLine(120.0f);
        if (ImGui::ColorEdit4("##Error", reinterpret_cast<float*>(&active_theme_mut.error_color), color_flags)) {
            any_color_edited = true;
        }
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Border");
        ImGui::SameLine(120.0f);
        if (ImGui::ColorEdit4("##Border", reinterpret_cast<float*>(&active_theme_mut.border_accent), color_flags)) {
            any_color_edited = true;
        }
        ImGui::EndGroup();
        
        // Handle color edits: auto-create custom theme if editing a built-in
        if (any_color_edited) {
            if (theme_mgr.IsBuiltInTheme(current_theme)) {
                // Auto-create custom theme from edited built-in
                std::string new_theme_name = theme_mgr.CreateCustomFromActive("Custom");
                if (!new_theme_name.empty()) {
                    gs.config.active_theme = new_theme_name;
                    // Update theme list to show new custom theme
                }
            } else {
                // Editing existing custom theme - mark for auto-save
                s_theme_was_edited = true;
                s_theme_save_timer = 0.0f;
            }
            // Apply changes immediately to UI
            theme_mgr.ApplyToImGui();
        }
        
        ImGui::Spacing();
        
    }
    
    ImGui::Separator();
    
    // ======================================================================
    // THEME EDITOR OPTIONS (with breathing animation)
    // ======================================================================
    RC_UI::AnimationHelpers::BeginBreathingHeader("Theme Editor Options", UITokens::InfoBlue, panel_focused);
    {
        ImGui::Checkbox("Auto-save Custom Themes", &s_auto_save_custom_themes);
        uiint.RegisterWidget({"checkbox", "Auto-save Custom Themes", "ui.theme_autosave", {"ui", "settings"}});
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Automatically save custom theme changes after 2 seconds");
        }
        
        ImGui::Checkbox("Show Hex by Default", &s_show_hex_by_default);
        uiint.RegisterWidget({"checkbox", "Show Hex by Default", "ui.theme_show_hex", {"ui", "settings"}});
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Display color values in hexadecimal format");
        }
        
        ImGui::Spacing();
        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(UITokens::InfoBlue), 
            "Note: Editing a built-in theme auto-creates a custom copy");
        
    }
    
    ImGui::Separator();
    
    // ======================================================================
    // UI SCALE & DISPLAY OPTIONS (with breathing animation)
    // ======================================================================
    RC_UI::AnimationHelpers::BeginBreathingHeader("Display Options", UITokens::InfoBlue, panel_focused);
    {
        
        ImGui::SliderFloat("UI Scale", &gs.config.ui_scale, 0.8f, 1.5f, "%.2f");
        uiint.RegisterWidget({"slider", "UI Scale", "ui.scale", {"ui", "settings"}});
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Restart required for full effect");
        }

        bool multi_viewport_changed = ImGui::Checkbox("Enable Multi-Viewport (Opt-in)", &gs.config.ui_multi_viewport_enabled);
        uiint.RegisterWidget({"checkbox", "Enable Multi-Viewport (Opt-in)", "ui.multi_viewport_enabled", {"ui", "settings", "dpi"}});
        if (multi_viewport_changed) {
            ImGui::SetTooltip("Restart required to fully apply viewport backend state.");
        }

        ImGui::Checkbox("Scale Fonts With DPI", &gs.config.ui_dpi_scale_fonts_enabled);
        uiint.RegisterWidget({"checkbox", "Scale Fonts With DPI", "ui.dpi_scale_fonts_enabled", {"ui", "settings", "dpi"}});

        ImGui::Checkbox("Scale Viewports With DPI", &gs.config.ui_dpi_scale_viewports_enabled);
        uiint.RegisterWidget({"checkbox", "Scale Viewports With DPI", "ui.dpi_scale_viewports_enabled", {"ui", "settings", "dpi"}});

        static constexpr std::array<const char*, 3> kCommandModeLabels{
            "Smart List",
            "Pie",
            "Global Palette"
        };
        int context_mode = static_cast<int>(gs.config.viewport_context_default_mode);
        if (ImGui::Combo("Viewport Right-Click Mode", &context_mode, kCommandModeLabels.data(), static_cast<int>(kCommandModeLabels.size()))) {
            context_mode = std::clamp(context_mode, 0, static_cast<int>(kCommandModeLabels.size()) - 1);
            gs.config.viewport_context_default_mode = static_cast<RogueCity::Core::Editor::ViewportCommandMode>(context_mode);
        }
        uiint.RegisterWidget({"combo", "Viewport Right-Click Mode", "ui.viewport_context_mode", {"ui", "settings", "commands"}});

        ImGui::Checkbox("Hotkey: Space -> Smart List", &gs.config.viewport_hotkey_space_enabled);
        ImGui::Checkbox("Hotkey: / -> Pie Menu", &gs.config.viewport_hotkey_slash_enabled);
        ImGui::Checkbox("Hotkey: `~ -> Pie Menu", &gs.config.viewport_hotkey_grave_enabled);
        ImGui::Checkbox("Hotkey: P -> Global Palette", &gs.config.viewport_hotkey_p_enabled);
        uiint.RegisterWidget({"checkbox", "Hotkey Space", "ui.hotkeys.space", {"ui", "settings", "commands"}});
        uiint.RegisterWidget({"checkbox", "Hotkey Slash", "ui.hotkeys.slash", {"ui", "settings", "commands"}});
        uiint.RegisterWidget({"checkbox", "Hotkey Grave", "ui.hotkeys.grave", {"ui", "settings", "commands"}});
        uiint.RegisterWidget({"checkbox", "Hotkey P", "ui.hotkeys.p", {"ui", "settings", "commands"}});
        
        ImGui::Spacing();
        
        ImGui::Checkbox("Show Viewport Grid Overlay", &gs.config.show_grid_overlay);
        uiint.RegisterWidget({"checkbox", "Show Viewport Grid Overlay", "ui.grid_overlay", {"ui", "settings"}});
        
        ImGui::Spacing();
        
        ImGui::TextUnformatted("Screen Resolution:");
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (viewport) {
            ImGui::Text("  %d x %d", static_cast<int>(viewport->Size.x), static_cast<int>(viewport->Size.y));
            
            if (viewport->Size.x < 1280 || viewport->Size.y < 720) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(UITokens::YellowWarning));
                ImGui::TextWrapped("UI optimized for 1920x1080+. Current resolution may cause panel clipping.");
                ImGui::PopStyleColor();
            }
        }
        
    }
    
    ImGui::Separator();

    // ======================================================================
    // LAYOUT PREFERENCES (dock ratio controls with safe center constraints)
    // ======================================================================
    RC_UI::AnimationHelpers::BeginBreathingHeader("Layout Preferences", UITokens::InfoBlue, panel_focused);
    {
        ImGui::TextWrapped("Baseline preset is tuned for 1280x1024 and scales to other resolutions.");
        ImGui::Spacing();

        ImGui::SliderFloat("Master Panel Ratio", &s_layout_prefs_edit.left_panel_ratio, 0.20f, 0.45f, "%.2f");
        uiint.RegisterWidget({"slider", "Master Panel Ratio", "ui.layout.left_ratio", {"ui", "layout", "dock"}});

        ImGui::SliderFloat("Right Column Ratio", &s_layout_prefs_edit.right_panel_ratio, 0.15f, 0.35f, "%.2f");
        uiint.RegisterWidget({"slider", "Right Column Ratio", "ui.layout.right_ratio", {"ui", "layout", "dock"}});

        ImGui::SliderFloat("Tool Deck Height Ratio", &s_layout_prefs_edit.tool_deck_ratio, 0.18f, 0.45f, "%.2f");
        uiint.RegisterWidget({"slider", "Tool Deck Height Ratio", "ui.layout.tool_deck_ratio", {"ui", "layout", "dock"}});

        const float center_ratio = 1.0f - s_layout_prefs_edit.left_panel_ratio - s_layout_prefs_edit.right_panel_ratio;
        ImGui::Text("Center Workspace Ratio: %.2f", center_ratio);
        if (center_ratio < 0.40f) {
            ImGui::TextColored(
                ImGui::ColorConvertU32ToFloat4(UITokens::YellowWarning),
                "Center workspace is below recommended minimum (0.40).");
        }

        if (RogueCity::UI::DesignSystem::ButtonPrimary("Apply Layout Preferences", ImVec2(220.0f, 30.0f))) {
            RC_UI::SetDockLayoutPreferences(s_layout_prefs_edit);
            RC_UI::ResetDockLayout();
        }
        uiint.RegisterWidget({"button", "Apply Layout Preferences", "action:ui.layout.apply", {"action", "ui", "layout"}});

        ImGui::SameLine();
        if (RogueCity::UI::DesignSystem::ButtonSecondary("Reset 1280x1024 Preset", ImVec2(220.0f, 30.0f))) {
            s_layout_prefs_edit = RC_UI::GetDefaultDockLayoutPreferences();
            RC_UI::SetDockLayoutPreferences(s_layout_prefs_edit);
            RC_UI::ResetDockLayout();
        }
        uiint.RegisterWidget({"button", "Reset 1280x1024 Preset", "action:ui.layout.reset", {"action", "ui", "layout"}});
    }

    ImGui::Separator();
    
    // ======================================================================
    // CUSTOM THEME CREATION (Advanced, with breathing animation)
    // ======================================================================
    RC_UI::AnimationHelpers::BeginBreathingHeader("Custom Theme (Advanced)", UITokens::InfoBlue, panel_focused);
    {
        
        ImGui::TextWrapped("Create custom theme profiles by editing JSON files in AI/config/themes/");
        ImGui::Spacing();
        
        ImGui::InputText("Theme Name", s_custom_theme_name, sizeof(s_custom_theme_name));
        uiint.RegisterWidget({"text", "Theme Name", "ui.custom_theme_name", {"ui", "settings"}});
        
        ImGui::Spacing();
        
        if (RogueCity::UI::DesignSystem::ButtonPrimary("Export Current Theme", ImVec2(200, 30))) {
            std::string error;
            if (theme_mgr.SaveThemeToFile(theme_mgr.GetActiveTheme(), &error)) {
                ImGui::OpenPopup("Export Success");
            } else {
                ImGui::OpenPopup("Export Failed");
            }
        }
        uiint.RegisterWidget({"button", "Export Current Theme", "action:ui.export_theme", {"action", "ui", "settings"}});
        
        // Export success/fail popups
        if (ImGui::BeginPopupModal("Export Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Theme exported successfully!");
            ImGui::Separator();
            if (RogueCity::UI::DesignSystem::ButtonPrimary("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        if (ImGui::BeginPopupModal("Export Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed), "Theme export failed!");
            ImGui::Separator();
            if (RogueCity::UI::DesignSystem::ButtonPrimary("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        ImGui::Spacing();
        
        if (RogueCity::UI::DesignSystem::ButtonSecondary("Reload Custom Themes", ImVec2(200, 30))) {
            theme_mgr.LoadCustomThemes();
        }
        uiint.RegisterWidget({"button", "Reload Custom Themes", "action:ui.reload_themes", {"action", "ui", "settings"}});
        
    }
}

} // namespace RC_UI::Panels::UiSettings
