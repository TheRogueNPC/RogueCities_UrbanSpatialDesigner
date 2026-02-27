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
        "Rogue",
        IM_COL32(214, 120, 72, 255),     // primary_accent (sunset amber)
        IM_COL32(96, 214, 208, 255),     // secondary_accent (neon teal)
        IM_COL32(118, 196, 127, 255),    // success_color (urban green)
        IM_COL32(232, 162, 78, 255),     // warning_color (streetlamp amber)
        IM_COL32(196, 74, 60, 255),      // error_color (brick red)
        IM_COL32(20, 16, 14, 255),       // background_dark (deep asphalt)
        IM_COL32(32, 26, 22, 255),       // panel_background (warm charcoal)
        IM_COL32(236, 172, 110, 42),     // grid_overlay (sunset haze)
        IM_COL32(242, 222, 196, 255),    // text_primary (warm ivory)
        IM_COL32(199, 170, 142, 255),    // text_secondary (soft tan)
        IM_COL32(113, 98, 84, 255),      // text_disabled
        IM_COL32(96, 214, 208, 255)      // border_accent (neon teal)
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

ThemeProfile Themes::DowntownCity() {
    return ThemeProfile{
        "Downtown City",
        IM_COL32(214, 120, 72, 255),     // primary_accent (sunset amber)
        IM_COL32(96, 214, 208, 255),     // secondary_accent (neon teal)
        IM_COL32(118, 196, 127, 255),    // success_color (urban green)
        IM_COL32(232, 162, 78, 255),     // warning_color (streetlamp amber)
        IM_COL32(196, 74, 60, 255),      // error_color (brick red)
        IM_COL32(20, 16, 14, 255),       // background_dark (deep asphalt)
        IM_COL32(32, 26, 22, 255),       // panel_background (warm charcoal)
        IM_COL32(236, 172, 110, 42),     // grid_overlay (sunset haze)
        IM_COL32(242, 222, 196, 255),    // text_primary (warm ivory)
        IM_COL32(199, 170, 142, 255),    // text_secondary (soft tan)
        IM_COL32(113, 98, 84, 255),      // text_disabled
        IM_COL32(96, 214, 208, 255)      // border_accent (neon teal)
    };
}

ThemeProfile Themes::Soviet() {
    // Backward-compat shim for configs that still request "Soviet".
    return Themes::DowntownCity();
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

ThemeProfile Themes::Tron() {
    return ThemeProfile{
        "Tron",
        IM_COL32(  0, 255, 255, 255),    // primary_accent (cyan #0FF)
        IM_COL32(255,   0, 255, 255),    // secondary_accent (magenta #F0F)
        IM_COL32(  0, 255,   0, 255),    // success_color (green)
        IM_COL32(255, 255,   0, 255),    // warning_color (yellow #FF0)
        IM_COL32(255,  50,  50, 255),    // error_color
        IM_COL32(  0,   0,   0, 255),    // background_dark (pure black)
        IM_COL32(  5,  10,  20, 255),    // panel_background (near-black blue)
        IM_COL32(  0, 255, 255,  30),    // grid_overlay (cyan with alpha)
        IM_COL32(  0, 255, 255, 255),    // text_primary (cyan)
        IM_COL32(  0, 180, 180, 255),    // text_secondary (dim cyan)
        IM_COL32(  0,  80,  80, 255),    // text_disabled
        IM_COL32(255,   0, 255, 255)     // border_accent (magenta)
    };
}

ThemeProfile Themes::RedlightDistrict() {
    return ThemeProfile{
        "Redlight District",
        IM_COL32(199,   0,  57, 255),    // primary_accent (#C70039 deep red)
        IM_COL32(255,  87,  51, 255),    // secondary_accent (#FF5733 orange-red)
        IM_COL32(100, 200, 100, 255),    // success_color
        IM_COL32(255, 160,   0, 255),    // warning_color
        IM_COL32(144,  12,  63, 255),    // error_color (#900C3F dark crimson)
        IM_COL32( 15,   5,   8, 255),    // background_dark
        IM_COL32( 30,  10,  15, 255),    // panel_background
        IM_COL32(199,   0,  57,  40),    // grid_overlay
        IM_COL32(255, 200, 210, 255),    // text_primary
        IM_COL32(190, 140, 150, 255),    // text_secondary
        IM_COL32( 90,  60,  65, 255),    // text_disabled
        IM_COL32(199,   0,  57, 255)     // border_accent
    };
}

ThemeProfile Themes::Enterprise() {
    return ThemeProfile{
        "Enterprise",
        IM_COL32( 80, 144, 168, 255),    // primary_accent (#5090A8 steel blue)
        IM_COL32(200, 216, 224, 255),    // secondary_accent (#C8D8E0 pale blue-grey)
        IM_COL32( 80, 180, 100, 255),    // success_color
        IM_COL32(200, 160,  60, 255),    // warning_color
        IM_COL32(180,  60,  60, 255),    // error_color
        IM_COL32( 10,  10,  15, 255),    // background_dark (#0A0A0F)
        IM_COL32( 17,  17,  24, 255),    // panel_background (#111118)
        IM_COL32(200, 216, 224,  20),    // grid_overlay
        IM_COL32(200, 216, 224, 255),    // text_primary (#C8D8E0)
        IM_COL32(140, 160, 170, 255),    // text_secondary
        IM_COL32( 70,  80,  90, 255),    // text_disabled
        IM_COL32(220, 235, 245, 255)     // border_accent (sharp white)
    };
}

ThemeProfile Themes::Planner() {
    return ThemeProfile{
        "Planner",
        IM_COL32(104, 214, 216, 255),    // primary_accent (#68D6D8 teal)
        IM_COL32( 80, 200, 200, 255),    // secondary_accent
        IM_COL32( 80, 200, 120, 255),    // success_color
        IM_COL32(200, 200,  60, 255),    // warning_color
        IM_COL32(200,  80,  80, 255),    // error_color
        IM_COL32(  6,  11,  16, 255),    // background_dark (#060B10)
        IM_COL32( 10,  18,  26, 255),    // panel_background
        IM_COL32(104, 214, 216,  30),    // grid_overlay (teal with alpha)
        IM_COL32(240, 248, 255, 255),    // text_primary (bright white)
        IM_COL32(160, 210, 220, 255),    // text_secondary (dim teal-white)
        IM_COL32( 60, 100, 110, 255),    // text_disabled
        IM_COL32(104, 214, 216, 255)     // border_accent (teal)
    };
}

// === THEME MANAGER IMPLEMENTATION ===

ThemeManager& ThemeManager::Instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() 
    : m_activeTheme(Themes::Default())
    , m_activeThemeName("Rogue")
{
    RegisterBuiltInThemes();
}

void ThemeManager::RegisterBuiltInThemes() {
    m_themes["Rogue"]            = Themes::Default();
    m_themes["RedRetro"]         = Themes::RedRetro();
    m_themes["Downtown City"]    = Themes::DowntownCity();
    m_themes["CyberPunk"]        = Themes::CyberPunk();
    m_themes["Tron"]             = Themes::Tron();
    m_themes["Redlight District"]= Themes::RedlightDistrict();
    m_themes["Enterprise"]       = Themes::Enterprise();
    m_themes["Planner"]          = Themes::Planner();

    m_builtInThemeNames = {
        "Rogue", "RedRetro", "Downtown City", "CyberPunk",
        "Tron", "Redlight District", "Enterprise", "Planner"
    };
}

bool ThemeManager::LoadTheme(const std::string& name) {
    std::string resolved_name = (name == "Default") ? "Rogue" : name;
    if (resolved_name == "Soviet") {
        resolved_name = "Rogue";
    }
    auto it = m_themes.find(resolved_name);
    if (it == m_themes.end()) {
        return false;
    }
    
    m_activeTheme = it->second;
    m_activeThemeName = resolved_name;
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

    auto toVec4 = [](ImU32 color) {
        return ImGui::ColorConvertU32ToFloat4(color);
    };
    // Build a dim (alpha-blended) variant of a color
    auto dim = [&](ImU32 color, float alpha) -> ImVec4 {
        ImVec4 v = ImGui::ColorConvertU32ToFloat4(color);
        v.w = alpha;
        return v;
    };

    // === COLORS — correct semantic mapping =================================
    // Text
    style.Colors[ImGuiCol_Text]         = toVec4(t.text_primary);
    style.Colors[ImGuiCol_TextDisabled] = toVec4(t.text_secondary);

    // Window backgrounds — layered dark → panel (not reversed)
    style.Colors[ImGuiCol_WindowBg]   = toVec4(t.background_dark);   // --bg-deep: deepest layer
    style.Colors[ImGuiCol_ChildBg]    = toVec4(t.panel_background);  // --bg-panel: content areas
    style.Colors[ImGuiCol_PopupBg]    = toVec4(t.panel_background);

    // Borders — dim accent, not full-brightness
    style.Colors[ImGuiCol_Border]       = dim(t.border_accent, 0.30f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Input/frame backgrounds
    style.Colors[ImGuiCol_FrameBg]        = dim(t.background_dark, 0.90f);
    style.Colors[ImGuiCol_FrameBgHovered] = dim(t.secondary_accent, 0.13f);
    style.Colors[ImGuiCol_FrameBgActive]  = dim(t.primary_accent,   0.20f);

    // Title bar — dark base, secondary tint when focused
    style.Colors[ImGuiCol_TitleBg]          = toVec4(t.background_dark);
    style.Colors[ImGuiCol_TitleBgActive]    = dim(t.secondary_accent, 0.25f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = dim(t.background_dark, 0.80f);

    style.Colors[ImGuiCol_MenuBarBg] = toVec4(t.panel_background);

    // Scrollbar
    style.Colors[ImGuiCol_ScrollbarBg]          = toVec4(t.background_dark);
    style.Colors[ImGuiCol_ScrollbarGrab]         = dim(t.text_secondary, 0.60f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = toVec4(t.secondary_accent);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = toVec4(t.primary_accent);

    // Controls
    style.Colors[ImGuiCol_CheckMark]       = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_SliderGrab]      = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_SliderGrabActive]= toVec4(t.secondary_accent);

    // Buttons — semi-transparent primary, lit on hover/active
    style.Colors[ImGuiCol_Button]        = dim(t.primary_accent, 0.20f);
    style.Colors[ImGuiCol_ButtonHovered] = dim(t.primary_accent, 0.47f);
    style.Colors[ImGuiCol_ButtonActive]  = toVec4(t.primary_accent);

    // Headers (Selectable, TreeNode, CollapsingHeader)
    style.Colors[ImGuiCol_Header]        = dim(t.secondary_accent, 0.15f);
    style.Colors[ImGuiCol_HeaderHovered] = dim(t.secondary_accent, 0.30f);
    style.Colors[ImGuiCol_HeaderActive]  = dim(t.primary_accent,   0.40f);

    // Separator
    style.Colors[ImGuiCol_Separator]        = dim(t.border_accent, 0.30f);
    style.Colors[ImGuiCol_SeparatorHovered] = toVec4(t.secondary_accent);
    style.Colors[ImGuiCol_SeparatorActive]  = toVec4(t.primary_accent);

    // Resize grip
    style.Colors[ImGuiCol_ResizeGrip]        = dim(t.secondary_accent, 0.15f);
    style.Colors[ImGuiCol_ResizeGripHovered] = toVec4(t.secondary_accent);
    style.Colors[ImGuiCol_ResizeGripActive]  = toVec4(t.primary_accent);

    // Tabs — matches mockup: surface bg → elevated on hover → panel when active
    style.Colors[ImGuiCol_Tab]                = dim(t.background_dark, 0.85f);
    style.Colors[ImGuiCol_TabHovered]         = dim(t.primary_accent,  0.30f);
    style.Colors[ImGuiCol_TabActive]          = dim(t.primary_accent,  0.18f);
    style.Colors[ImGuiCol_TabUnfocused]       = toVec4(t.background_dark);
    style.Colors[ImGuiCol_TabUnfocusedActive] = toVec4(t.panel_background);

#if defined(IMGUI_HAS_DOCK)
    style.Colors[ImGuiCol_DockingPreview]  = dim(t.secondary_accent, 0.70f);
    style.Colors[ImGuiCol_DockingEmptyBg]  = toVec4(t.background_dark);
#endif

    // Plots
    style.Colors[ImGuiCol_PlotLines]            = toVec4(t.secondary_accent);
    style.Colors[ImGuiCol_PlotLinesHovered]     = toVec4(t.error_color);
    style.Colors[ImGuiCol_PlotHistogram]        = toVec4(t.primary_accent);
    style.Colors[ImGuiCol_PlotHistogramHovered] = toVec4(t.warning_color);

    // Tables
    style.Colors[ImGuiCol_TableHeaderBg]     = toVec4(t.panel_background);
    style.Colors[ImGuiCol_TableBorderStrong] = dim(t.border_accent, 0.30f);
    style.Colors[ImGuiCol_TableBorderLight]  = dim(t.border_accent, 0.15f);
    style.Colors[ImGuiCol_TableRowBg]        = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]     = dim(t.panel_background, 0.25f);

    // Selection / interaction
    style.Colors[ImGuiCol_TextSelectedBg]  = dim(t.primary_accent,   0.32f);
    style.Colors[ImGuiCol_DragDropTarget]  = toVec4(t.warning_color);
    style.Colors[ImGuiCol_NavHighlight]    = toVec4(t.secondary_accent);
    style.Colors[ImGuiCol_NavWindowingHighlight] = dim(t.secondary_accent, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = dim(t.background_dark,  0.40f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = dim(t.background_dark,  0.70f);

    // === GEOMETRY — Y2K: hard edges, 1 px borders, compact padding =========
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding      = 0.0f;
    style.TabRounding       = 0.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize  = 1.0f;
    style.PopupBorderSize  = 1.0f;
    style.FrameBorderSize  = 1.0f;
    style.TabBorderSize    = 1.0f;

    style.WindowPadding     = ImVec2(8.0f, 6.0f);
    style.FramePadding      = ImVec2(6.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
    style.CellPadding       = ImVec2(4.0f, 2.0f);
    style.ScrollbarSize     = 10.0f;
    style.GrabMinSize       = 8.0f;
    style.IndentSpacing     = 16.0f;
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
