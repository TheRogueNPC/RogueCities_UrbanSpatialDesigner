// ThemeManager.cpp - Multi-theme system implementation
#include "RogueCity/App/UI/ThemeManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>

namespace RogueCity::UI {

// === BUILT-IN THEME DEFINITIONS ===

ThemeProfile Themes::Default() {
    return ThemeProfile{
        "Default",
        IM_COL32(0, 255, 255, 255),      // primary_accent (Cyan)
        IM_COL32(0, 255, 128, 255),      // secondary_accent (Green)
        IM_COL32(0, 255, 100, 255),      // success_color
        IM_COL32(255, 200, 0, 255),      // warning_color (Yellow)
        IM_COL32(255, 50, 50, 255),      // error_color
        IM_COL32(15, 20, 30, 255),       // background_dark
        IM_COL32(20, 25, 35, 255),       // panel_background
        IM_COL32(40, 50, 70, 100),       // grid_overlay (with alpha)
        IM_COL32(255, 255, 255, 255),    // text_primary
        IM_COL32(180, 180, 180, 255),    // text_secondary
        IM_COL32(100, 100, 100, 255),    // text_disabled
        IM_COL32(0, 255, 255, 255)       // border_accent (Cyan)
    };
}

ThemeProfile Themes::RedRetro() {
    return ThemeProfile{
        "RedRetro",
        IM_COL32(209, 33, 40, 255),      // primary_accent (#D12128 red)
        IM_COL32(250, 227, 172, 255),    // secondary_accent (#FAE3AC cream)
        IM_COL32(0, 200, 100, 255),      // success_color (green contrast)
        IM_COL32(255, 180, 50, 255),     // warning_color (amber)
        IM_COL32(200, 20, 20, 255),      // error_color (dark red)
        IM_COL32(1, 52, 79, 255),        // background_dark (#01344F deep blue)
        IM_COL32(10, 60, 90, 255),       // panel_background (lighter blue)
        IM_COL32(250, 227, 172, 50),     // grid_overlay (cream with alpha)
        IM_COL32(250, 227, 172, 255),    // text_primary (cream)
        IM_COL32(200, 180, 140, 255),    // text_secondary (dimmed cream)
        IM_COL32(100, 90, 70, 255),      // text_disabled
        IM_COL32(209, 33, 40, 255)       // border_accent (red)
    };
}

ThemeProfile Themes::Soviet() {
    return ThemeProfile{
        "Soviet",
        IM_COL32(192, 48, 30, 255),      // primary_accent (#C0301E red)
        IM_COL32(246, 218, 157, 255),    // secondary_accent (#F6DA9D beige)
        IM_COL32(100, 200, 100, 255),    // success_color (muted green)
        IM_COL32(220, 180, 60, 255),     // warning_color (gold)
        IM_COL32(180, 30, 20, 255),      // error_color (dark red)
        IM_COL32(0, 0, 0, 255),          // background_dark (#000000 black)
        IM_COL32(20, 20, 20, 255),       // panel_background (near-black)
        IM_COL32(246, 218, 157, 40),     // grid_overlay (beige with alpha)
        IM_COL32(246, 218, 157, 255),    // text_primary (beige)
        IM_COL32(200, 170, 120, 255),    // text_secondary (dimmed beige)
        IM_COL32(80, 70, 50, 255),       // text_disabled
        IM_COL32(192, 48, 30, 255)       // border_accent (red)
    };
}

ThemeProfile Themes::CyberPunk() {
    return ThemeProfile{
        "CyberPunk",
        IM_COL32(138, 80, 159, 255),     // primary_accent (#8A509F purple)
        IM_COL32(250, 153, 197, 255),    // secondary_accent (#FA99C5 pink)
        IM_COL32(100, 255, 200, 255),    // success_color (neon cyan)
        IM_COL32(255, 150, 255, 255),    // warning_color (magenta)
        IM_COL32(255, 50, 150, 255),     // error_color (hot pink)
        IM_COL32(10, 38, 112, 255),      // background_dark (#0A2670 deep blue)
        IM_COL32(20, 50, 130, 255),      // panel_background (blue-purple)
        IM_COL32(250, 153, 197, 60),     // grid_overlay (pink with alpha)
        IM_COL32(250, 153, 197, 255),    // text_primary (pink)
        IM_COL32(180, 120, 160, 255),    // text_secondary (dimmed purple)
        IM_COL32(80, 60, 90, 255),       // text_disabled
        IM_COL32(138, 80, 159, 255)      // border_accent (purple)
    };
}

// === THEME MANAGER IMPLEMENTATION ===

ThemeManager& ThemeManager::Instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() 
    : m_activeTheme(Themes::Default())
    , m_activeThemeName("Default")
{
    RegisterBuiltInThemes();
}

void ThemeManager::RegisterBuiltInThemes() {
    m_themes["Default"] = Themes::Default();
    m_themes["RedRetro"] = Themes::RedRetro();
    m_themes["Soviet"] = Themes::Soviet();
    m_themes["CyberPunk"] = Themes::CyberPunk();
    
    // Store built-in theme names for later checking
    m_builtInThemeNames = { "Default", "RedRetro", "Soviet", "CyberPunk" };
}

bool ThemeManager::LoadTheme(const std::string& name) {
    auto it = m_themes.find(name);
    if (it == m_themes.end()) {
        return false;
    }
    
    m_activeTheme = it->second;
    m_activeThemeName = name;
    ApplyToImGui();
    return true;
}

std::vector<std::string> ThemeManager::GetAvailableThemes() const {
    std::vector<std::string> names;
    names.reserve(m_themes.size());
    for (const auto& [name, _] : m_themes) {
        names.push_back(name);
    }
    return names;
}

bool ThemeManager::RegisterTheme(const ThemeProfile& profile) {
    if (m_themes.count(profile.name) > 0) {
        return false; // Already exists
    }
    m_themes[profile.name] = profile;
    return true;
}

void ThemeManager::ApplyToImGui() {
    ImGuiStyle& style = ImGui::GetStyle();
    const ThemeProfile& t = m_activeTheme;
    
    // Convert ImU32 to ImVec4 for ImGui style
    auto toVec4 = [](ImU32 color) {
        return ImGui::ColorConvertU32ToFloat4(color);
    };
    
    // === COLORS ===
    style.Colors[ImGuiCol_Text] = toVec4(t.text_primary);
    style.Colors[ImGuiCol_TextDisabled] = toVec4(t.text_disabled);
    style.Colors[ImGuiCol_WindowBg] = toVec4(t.panel_background);
    style.Colors[ImGuiCol_ChildBg] = toVec4(t.panel_background);
    style.Colors[ImGuiCol_PopupBg] = toVec4(t.panel_background);
    style.Colors[ImGuiCol_Border] = toVec4(t.border_accent);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    
    // Frame backgrounds (inputs, sliders)
    style.Colors[ImGuiCol_FrameBg] = toVec4(t.background_dark);
    style.Colors[ImGuiCol_FrameBgHovered] = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_FrameBgActive] = toVec4(t.secondary_accent);
    
    // Title bar
    style.Colors[ImGuiCol_TitleBg] = toVec4(t.background_dark);
    style.Colors[ImGuiCol_TitleBgActive] = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_TitleBgCollapsed] = toVec4(t.background_dark);
    
    // Menu bar
    style.Colors[ImGuiCol_MenuBarBg] = toVec4(t.panel_background);
    
    // Scrollbar
    style.Colors[ImGuiCol_ScrollbarBg] = toVec4(t.background_dark);
    style.Colors[ImGuiCol_ScrollbarGrab] = toVec4(t.text_secondary);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = toVec4(t.secondary_accent);
    
    // Checkbox/radio
    style.Colors[ImGuiCol_CheckMark] = toVec4(t.success_color);
    
    // Slider grab
    style.Colors[ImGuiCol_SliderGrab] = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_SliderGrabActive] = toVec4(t.secondary_accent);
    
    // Buttons
    style.Colors[ImGuiCol_Button] = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_ButtonHovered] = toVec4(t.secondary_accent);
    style.Colors[ImGuiCol_ButtonActive] = toVec4(t.border_accent);
    
    // Headers (collapsing, tree nodes)
    style.Colors[ImGuiCol_Header] = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_HeaderHovered] = toVec4(t.secondary_accent);
    style.Colors[ImGuiCol_HeaderActive] = toVec4(t.border_accent);
    
    // Separator
    style.Colors[ImGuiCol_Separator] = toVec4(t.text_disabled);
    style.Colors[ImGuiCol_SeparatorHovered] = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_SeparatorActive] = toVec4(t.secondary_accent);
    
    // Tabs
    style.Colors[ImGuiCol_Tab] = toVec4(t.background_dark);
    style.Colors[ImGuiCol_TabHovered] = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_TabActive] = toVec4(t.border_accent);
    style.Colors[ImGuiCol_TabUnfocused] = toVec4(t.background_dark);
    style.Colors[ImGuiCol_TabUnfocusedActive] = toVec4(t.panel_background);
    
    // Dock preview (only when docking enum is available)
#if defined(IMGUI_HAS_DOCK)
    style.Colors[ImGuiCol_DockingPreview] = toVec4(t.primary_accent);
#endif
    
    // === Y2K GEOMETRY (hard edges) ===
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    
    // Border widths (prominent for Y2K aesthetic)
    style.WindowBorderSize = 2.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 2.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;
}

bool ThemeManager::SaveThemeToFile(const ThemeProfile& profile, std::string* outError) {
    using json = nlohmann::json;
    
    try {
        json j;
        j["name"] = profile.name;
        j["colors"]["primary_accent"] = profile.primary_accent;
        j["colors"]["secondary_accent"] = profile.secondary_accent;
        j["colors"]["success_color"] = profile.success_color;
        j["colors"]["warning_color"] = profile.warning_color;
        j["colors"]["error_color"] = profile.error_color;
        j["colors"]["background_dark"] = profile.background_dark;
        j["colors"]["panel_background"] = profile.panel_background;
        j["colors"]["grid_overlay"] = profile.grid_overlay;
        j["colors"]["text_primary"] = profile.text_primary;
        j["colors"]["text_secondary"] = profile.text_secondary;
        j["colors"]["text_disabled"] = profile.text_disabled;
        j["colors"]["border_accent"] = profile.border_accent;
        
        std::filesystem::create_directories("AI/config/themes");
        std::string filepath = "AI/config/themes/" + profile.name + ".json";
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            if (outError) *outError = "Failed to open file for writing: " + filepath;
            return false;
        }
        
        file << j.dump(2);
        return true;
    }
    catch (const std::exception& e) {
        if (outError) *outError = std::string("JSON serialization error: ") + e.what();
        return false;
    }
}

void ThemeManager::LoadCustomThemes() {
    namespace fs = std::filesystem;
    
    try {
        std::string themes_dir = "AI/config/themes";
        if (!fs::exists(themes_dir)) {
            return; // No custom themes yet
        }
        
        for (const auto& entry : fs::directory_iterator(themes_dir)) {
            if (entry.path().extension() != ".json") continue;
            
            try {
                std::ifstream file(entry.path());
                if (!file.is_open()) continue;
                
                nlohmann::json j;
                file >> j;
                
                ThemeProfile profile;
                profile.name = j["name"].get<std::string>();
                profile.primary_accent = j["colors"]["primary_accent"].get<ImU32>();
                profile.secondary_accent = j["colors"]["secondary_accent"].get<ImU32>();
                profile.success_color = j["colors"]["success_color"].get<ImU32>();
                profile.warning_color = j["colors"]["warning_color"].get<ImU32>();
                profile.error_color = j["colors"]["error_color"].get<ImU32>();
                profile.background_dark = j["colors"]["background_dark"].get<ImU32>();
                profile.panel_background = j["colors"]["panel_background"].get<ImU32>();
                profile.grid_overlay = j["colors"]["grid_overlay"].get<ImU32>();
                profile.text_primary = j["colors"]["text_primary"].get<ImU32>();
                profile.text_secondary = j["colors"]["text_secondary"].get<ImU32>();
                profile.text_disabled = j["colors"]["text_disabled"].get<ImU32>();
                profile.border_accent = j["colors"]["border_accent"].get<ImU32>();
                
                RegisterTheme(profile);
            }
            catch (const std::exception&) {
                // Skip malformed theme files
                continue;
            }
        }
    }
    catch (const std::exception&) {
        // Directory iteration failed, ignore
    }
}

const ThemeProfile* ThemeManager::GetTheme(const std::string& name) const {
    auto it = m_themes.find(name);
    return (it != m_themes.end()) ? &it->second : nullptr;
}

bool ThemeManager::IsBuiltInTheme(const std::string& name) const {
    return std::find(m_builtInThemeNames.begin(), m_builtInThemeNames.end(), name) 
           != m_builtInThemeNames.end();
}

std::string ThemeManager::CreateCustomFromActive(const std::string& basename) {
    // Generate unique name with timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    
    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm_buf);
    
    std::string new_name = basename + "_" + timestamp;
    
    // Copy active theme with new name
    ThemeProfile custom_theme = m_activeTheme;
    custom_theme.name = new_name;
    
    // Register and save
    if (RegisterTheme(custom_theme)) {
        std::string error;
        if (SaveThemeToFile(custom_theme, &error)) {
            // Switch to the new custom theme
            LoadTheme(new_name);
            return new_name;
        }
    }
    
    return ""; // Failed
}

} // namespace RogueCity::UI
