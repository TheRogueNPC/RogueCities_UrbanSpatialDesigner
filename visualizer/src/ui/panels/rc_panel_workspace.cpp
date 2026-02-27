// FILE: rc_panel_workspace.cpp
// PURPOSE: Workspace / persona selector panel.
//
// Draws a small, always-accessible window that shows:
//   - The current persona name and a short description
//   - Five persona buttons / combo to switch persona
//   - "Customize..." link (opens the existing UiSettings panel)
//   - "Save as preset..." to save current layout + theme
//   - "Mockup Link" section — hot-reload CSS tokens from RC_UI_Mockup.html
//
// Viewport is sacred: this panel is docked to the bottom of the master panel
// (or the "Left" dock area) — never fullscreen, never obstructing the 3D view.

#include "ui/panels/rc_panel_workspace.h"
#include "ui/panels/rc_panel_ui_settings.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"
#include "ui/introspection/UiIntrospection.h"
#include "RogueCity/App/UI/ThemeManager.h"
#include "RogueCity/Visualizer/WorkspaceProfile.hpp"
#include "RogueCity/Visualizer/MockupTokenParser.hpp"
#include "RogueCity/Visualizer/MockupWatcher.hpp"

#include <imgui.h>
#include <array>
#include <string>

namespace RC_UI::Panels::Workspace {

namespace {

static bool  s_open              = true;
static int   s_active_persona    = 0;   // index into built-in profiles
static char  s_preset_name[64]   = "my_workspace";
static char  s_last_error[256]   = "";
static char  s_last_status[256]  = "";

// ---- Mockup Live Link state ------------------------------------------------
static bool                                 s_mockup_link_enabled = false;
static RogueCity::Visualizer::MockupWatcher s_mockup_watcher;
static RogueCity::Visualizer::MockupLayoutTokens s_last_layout_tokens;
static char s_mockup_status[256] = "";

static constexpr const char* kDefaultMockupPath = "visualizer/RC_UI_Mockup.html";

// Short description for each persona (shown below the name)
static constexpr std::array<const char*, 5> kPersonaDescriptions = {{
    "Full cockpit  \xe2\x80\x94  every panel, full telemetry, diagnostics on.",
    "High-contrast monochrome, index columns, data-dense readout.",
    "Visual-first, big viewport, minimal chrome. Great for exploring.",
    "Tabular, cyan-on-dark, indices visible. Precision data work.",
    "Narrative mode, warm palette. Story & POI notes front and center.",
}};

static constexpr std::array<const char*, 5> kPersonaLabels = {{
    "Rogue", "Enterprise", "Hobbyist", "Planner", "Artist"
}};

void ApplyProfile(const WorkspaceProfile& profile) {
    auto& tm = RogueCity::UI::ThemeManager::Instance();
    const std::string& tname = profile.theme.name;
    // Register if it does not exist yet (built-in profiles always will)
    if (!tm.GetTheme(tname)) {
        tm.RegisterTheme(profile.theme);
    }
    if (!tm.LoadTheme(tname)) {
        // Fallback: overwrite active and apply directly
        tm.GetActiveThemeMutable() = profile.theme;
        tm.ApplyToImGui();
    }
    // Note: layout flags (show_indices, visual_first, etc.) are stored on the
    // profile for downstream systems to read; we don't change docking here
    // so the user's custom arrangement is preserved across persona switches.
}

void ForceMockupReload() {
    using namespace RogueCity::Visualizer;
    auto result = MockupTokenParser::ParseFromHtmlFile(kDefaultMockupPath);
    if (!result || !result->valid) {
        std::snprintf(s_mockup_status, sizeof(s_mockup_status),
            "Parse failed or not found: %s", kDefaultMockupPath);
        return;
    }

    // Apply complete ImGuiStyle directly — this is the 1-to-1 match path.
    // No ThemeManager routing: we own the style object entirely.
    ImGui::GetStyle() = result->imgui_style;

    // Sync ThemeManager so persona-switch labels stay coherent
    auto& tm = RogueCity::UI::ThemeManager::Instance();
    if (!tm.GetTheme("Mockup")) {
        tm.RegisterTheme(result->theme);
    } else {
        // Overwrite existing Mockup entry
        tm.GetActiveThemeMutable() = result->theme;
    }

    // Apply layout ratios to dockspace
    s_last_layout_tokens = result->layout;
    RC_UI::DockLayoutPreferences prefs;
    prefs.left_panel_ratio  = result->layout.left_panel_ratio;
    prefs.right_panel_ratio = result->layout.right_panel_ratio;
    prefs.tool_deck_ratio   = result->layout.tool_deck_ratio;
    RC_UI::SetDockLayoutPreferences(prefs);

    std::snprintf(s_mockup_status, sizeof(s_mockup_status),
        "OK — L=%.2f R=%.2f T=%.2f | %s",
        result->layout.left_panel_ratio,
        result->layout.right_panel_ratio,
        result->layout.tool_deck_ratio,
        kDefaultMockupPath);
}

void OnMockupChanged() {
    if (!s_mockup_link_enabled) return;
    ForceMockupReload();
}

} // namespace

// ---------------------------------------------------------------------------

bool IsOpen()  { return s_open; }
void Toggle()  { s_open = !s_open; }

RogueCity::Visualizer::MockupLayoutTokens GetMockupLayoutTokens() {
    return s_last_layout_tokens;
}

void DrawContent(float dt) {
    const auto profiles = GetBuiltInWorkspaceProfiles();
    auto& tm = RogueCity::UI::ThemeManager::Instance();
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();

    uiint.BeginPanel({
        "WorkspaceSelector",
        "Workspace",
        "settings",
        "Left",
        "rc_panel_workspace.cpp",
        { "workspace", "persona", "theme" }
    }, s_open);

    // ---- Persona buttons ---------------------------------------------------
    ImGui::TextDisabled("PERSONA");
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", kPersonaDescriptions[s_active_persona]);
    ImGui::Spacing();

    const float btn_w = (ImGui::GetContentRegionAvail().x - 4.0f * 4.0f) / 5.0f;
    for (int i = 0; i < 5; ++i) {
        if (i > 0) ImGui::SameLine(0.0f, 4.0f);
        const bool active = (s_active_persona == i);
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button,
                ImGui::ColorConvertU32ToFloat4(UITokens::CyanAccent));
        }
        if (ImGui::Button(kPersonaLabels[i], ImVec2(btn_w, 0.0f))) {
            s_active_persona = i;
            ApplyProfile(profiles[i]);
            uiint.RegisterWidget({ "button", kPersonaLabels[i],
                "workspace.switch_persona", {"persona"} });
        }
        if (active) ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ---- Active theme name -------------------------------------------------
    ImGui::TextDisabled("THEME");
    ImGui::SameLine();
    ImGui::Text("%s", tm.GetActiveThemeName().c_str());

    // ---- Customize ---------------------------------------------------------
    if (ImGui::SmallButton("Customize...")) {
        if (!Panels::UiSettings::IsOpen()) {
            Panels::UiSettings::Toggle();
        }
        uiint.RegisterWidget({ "button", "Customize...",
            "workspace.open_customize", {"settings"} });
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ---- Save current layout + theme as named preset -----------------------
    ImGui::TextDisabled("SAVE PRESET");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 52.0f);
    ImGui::InputText("##preset_name", s_preset_name, sizeof(s_preset_name));
    ImGui::SameLine();
    if (ImGui::Button("Save", ImVec2(48.0f, 0.0f))) {
        std::string err;
        if (SaveWorkspacePreset(s_preset_name, &err)) {
            std::snprintf(s_last_status, sizeof(s_last_status),
                "Saved \"%s\"", s_preset_name);
            s_last_error[0] = '\0';
        } else {
            std::snprintf(s_last_error, sizeof(s_last_error),
                "Save failed: %s", err.c_str());
            s_last_status[0] = '\0';
        }
        uiint.RegisterWidget({ "button", "Save preset",
            "workspace.save_preset", {"preset"} });
    }

    if (s_last_status[0] != '\0') {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen),
            "%s", s_last_status);
    }
    if (s_last_error[0] != '\0') {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed),
            "%s", s_last_error);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ---- Mockup Live Link --------------------------------------------------
    ImGui::TextDisabled("MOCKUP LINK");
    uiint.RegisterWidget({ "label", "MOCKUP LINK", "workspace.mockup_link", {"mockup"} });

    bool link_changed = ImGui::Checkbox("Live Link##mockup", &s_mockup_link_enabled);
    uiint.RegisterWidget({ "checkbox", "Mockup Live Link",
        "workspace.mockup_link_enabled", {"mockup"} });

    if (link_changed) {
        if (s_mockup_link_enabled) {
            s_mockup_watcher.SetFile(kDefaultMockupPath);
            s_mockup_watcher.SetCallback(OnMockupChanged);
            s_mockup_watcher.SetPollInterval(1.0f);
        } else {
            s_mockup_status[0] = '\0';
        }
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!s_mockup_link_enabled);
    if (ImGui::SmallButton("Load Now")) {
        ForceMockupReload();
        uiint.RegisterWidget({ "button", "Load Now",
            "workspace.mockup_force_reload", {"mockup"} });
    }
    ImGui::EndDisabled();

    ImGui::TextDisabled("%s", kDefaultMockupPath);

    if (s_mockup_status[0] != '\0') {
        ImGui::TextWrapped("%s", s_mockup_status);
    }

    // Tick watcher (poll for file changes)
    if (s_mockup_link_enabled) {
        s_mockup_watcher.Update(dt);
    }

    uiint.EndPanel();
}

void Draw(float dt) {
    if (!s_open) {
        return;
    }

    static RC_UI::DockableWindowState s_workspace_window;
    ImGui::SetNextWindowSize(ImVec2(320.0f, 200.0f), ImGuiCond_FirstUseEver);
    if (!RC_UI::BeginDockableWindow("Workspace##WorkspacePanel", s_workspace_window, "Left",
                                    ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoScrollWithMouse)) {
        return;
    }
    DrawContent(dt);
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::Workspace
