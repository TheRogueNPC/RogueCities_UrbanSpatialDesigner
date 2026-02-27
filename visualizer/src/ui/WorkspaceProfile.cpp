// FILE: WorkspaceProfile.cpp
// PURPOSE: Built-in WorkspaceProfile factory + JSON serialization.
// NOTE:  This file lives in visualizer/ (Layer 2).  It may freely call
//        ThemeManager (app/ Layer 2) but must not touch core/ or generators/.

#include "RogueCity/Visualizer/WorkspaceProfile.hpp"

namespace RC_UI {

// ---------------------------------------------------------------------------
// JSON helpers for ThemeProfile (mirrors ThemeManager::SaveThemeToFile format)
// ---------------------------------------------------------------------------
namespace {

nlohmann::json ThemeProfileToJson(const RogueCity::UI::ThemeProfile& t) {
    using json = nlohmann::json;
    json j;
    j["name"]                    = t.name;
    j["colors"]["primary_accent"]   = t.primary_accent;
    j["colors"]["secondary_accent"] = t.secondary_accent;
    j["colors"]["success_color"]    = t.success_color;
    j["colors"]["warning_color"]    = t.warning_color;
    j["colors"]["error_color"]      = t.error_color;
    j["colors"]["background_dark"]  = t.background_dark;
    j["colors"]["panel_background"] = t.panel_background;
    j["colors"]["grid_overlay"]     = t.grid_overlay;
    j["colors"]["text_primary"]     = t.text_primary;
    j["colors"]["text_secondary"]   = t.text_secondary;
    j["colors"]["text_disabled"]    = t.text_disabled;
    j["colors"]["border_accent"]    = t.border_accent;
    return j;
}

RogueCity::UI::ThemeProfile ThemeProfileFromJson(const nlohmann::json& j) {
    RogueCity::UI::ThemeProfile t;
    t.name             = j.value("name", "Custom");
    const auto& c      = j.at("colors");
    t.primary_accent   = c.at("primary_accent").get<ImU32>();
    t.secondary_accent = c.at("secondary_accent").get<ImU32>();
    t.success_color    = c.at("success_color").get<ImU32>();
    t.warning_color    = c.at("warning_color").get<ImU32>();
    t.error_color      = c.at("error_color").get<ImU32>();
    t.background_dark  = c.at("background_dark").get<ImU32>();
    t.panel_background = c.at("panel_background").get<ImU32>();
    t.grid_overlay     = c.at("grid_overlay").get<ImU32>();
    t.text_primary     = c.at("text_primary").get<ImU32>();
    t.text_secondary   = c.at("text_secondary").get<ImU32>();
    t.text_disabled    = c.at("text_disabled").get<ImU32>();
    t.border_accent    = c.at("border_accent").get<ImU32>();
    return t;
}

constexpr const char* PersonaToString(WorkspacePersona p) noexcept {
    switch (p) {
        case WorkspacePersona::Enterprise: return "enterprise";
        case WorkspacePersona::Hobbyist:   return "hobbyist";
        case WorkspacePersona::Planner:    return "planner";
        case WorkspacePersona::Artist:     return "artist";
        default:                           return "rogue";
    }
}

WorkspacePersona PersonaFromString(const std::string& s) noexcept {
    if (s == "enterprise") return WorkspacePersona::Enterprise;
    if (s == "hobbyist")   return WorkspacePersona::Hobbyist;
    if (s == "planner")    return WorkspacePersona::Planner;
    if (s == "artist")     return WorkspacePersona::Artist;
    return WorkspacePersona::Rogue;
}

} // namespace

// ---------------------------------------------------------------------------
// GetBuiltInWorkspaceProfiles
// ---------------------------------------------------------------------------
std::vector<WorkspaceProfile> GetBuiltInWorkspaceProfiles() {
    using namespace RogueCity::UI;
    std::vector<WorkspaceProfile> profiles;
    profiles.reserve(5);

    // --- Rogue ---
    {
        WorkspaceProfile p;
        p.id           = "rogue";
        p.display_name = "Rogue";
        p.persona      = WorkspacePersona::Rogue;
        p.theme        = Themes::Default();
        p.info_density    = 0.8f;
        p.show_indices    = false;
        p.visual_first    = false;
        p.narrative_mode  = false;
        p.show_diagnostics = true;
        p.default_entry_state = "AXIOM";
        profiles.push_back(std::move(p));
    }

    // --- Enterprise ---
    {
        WorkspaceProfile p;
        p.id           = "enterprise";
        p.display_name = "Enterprise";
        p.persona      = WorkspacePersona::Enterprise;
        p.theme        = Themes::Enterprise();
        p.info_density    = 1.0f;
        p.show_indices    = true;
        p.visual_first    = false;
        p.narrative_mode  = false;
        p.show_diagnostics = true;
        p.default_entry_state = "AXIOM";
        profiles.push_back(std::move(p));
    }

    // --- Hobbyist ---
    {
        WorkspaceProfile p;
        p.id           = "hobbyist";
        p.display_name = "Hobbyist";
        p.persona      = WorkspacePersona::Hobbyist;
        p.theme        = Themes::CyberPunk();
        p.info_density    = 0.3f;
        p.show_indices    = false;
        p.visual_first    = true;
        p.narrative_mode  = false;
        p.show_diagnostics = false;
        p.default_entry_state = "AXIOM";
        profiles.push_back(std::move(p));
    }

    // --- Planner ---
    {
        WorkspaceProfile p;
        p.id           = "planner";
        p.display_name = "Planner";
        p.persona      = WorkspacePersona::Planner;
        p.theme        = Themes::Planner();
        p.info_density    = 1.0f;
        p.show_indices    = true;
        p.visual_first    = false;
        p.narrative_mode  = false;
        p.show_diagnostics = true;
        p.default_entry_state = "AXIOM";
        profiles.push_back(std::move(p));
    }

    // --- Artist ---
    {
        WorkspaceProfile p;
        p.id           = "artist";
        p.display_name = "Artist";
        p.persona      = WorkspacePersona::Artist;
        p.theme        = Themes::DowntownCity();
        p.info_density    = 0.4f;
        p.show_indices    = false;
        p.visual_first    = true;
        p.narrative_mode  = true;
        p.show_diagnostics = false;
        p.default_entry_state = "AXIOM";
        profiles.push_back(std::move(p));
    }

    return profiles;
}

// ---------------------------------------------------------------------------
// JSON serialization
// ---------------------------------------------------------------------------
nlohmann::json WorkspaceProfileToJson(const WorkspaceProfile& profile) {
    using json = nlohmann::json;
    json j;
    j["id"]           = profile.id;
    j["display_name"] = profile.display_name;
    j["persona"]      = PersonaToString(profile.persona);
    j["theme"]        = ThemeProfileToJson(profile.theme);
    j["info_density"]    = profile.info_density;
    j["show_indices"]    = profile.show_indices;
    j["visual_first"]    = profile.visual_first;
    j["narrative_mode"]  = profile.narrative_mode;
    j["show_diagnostics"] = profile.show_diagnostics;
    j["pinned_tools"]    = profile.pinned_tools;
    j["default_entry_state"] = profile.default_entry_state;
    j["active_export_pipelines"] = profile.active_export_pipelines;
    return j;
}

WorkspaceProfile WorkspaceProfileFromJson(const nlohmann::json& j) {
    WorkspaceProfile p;
    p.id           = j.value("id", "rogue");
    p.display_name = j.value("display_name", "Rogue");
    p.persona      = PersonaFromString(j.value("persona", "rogue"));
    if (j.contains("theme")) {
        p.theme = ThemeProfileFromJson(j.at("theme"));
    }
    p.info_density     = j.value("info_density", 0.5f);
    p.show_indices     = j.value("show_indices", false);
    p.visual_first     = j.value("visual_first", false);
    p.narrative_mode   = j.value("narrative_mode", false);
    p.show_diagnostics = j.value("show_diagnostics", true);
    if (j.contains("pinned_tools") && j["pinned_tools"].is_array()) {
        for (const auto& t : j["pinned_tools"]) {
            p.pinned_tools.push_back(t.get<std::string>());
        }
    }
    p.default_entry_state = j.value("default_entry_state", "AXIOM");
    if (j.contains("active_export_pipelines") && j["active_export_pipelines"].is_array()) {
        for (const auto& ep : j["active_export_pipelines"]) {
            p.active_export_pipelines.push_back(ep.get<std::string>());
        }
    }
    return p;
}

} // namespace RC_UI
