// FILE: visualizer/src/ui/panels/rc_panel_activity_bar_right.cpp
// PURPOSE: B2 Activity Bar — vertical icon strip (far right).

#include "ui/panels/rc_panel_activity_bar_right.h"

#include "ui/api/rc_imgui_api.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/panels/IPanelDrawer.h"
#include "ui/panels/RcMasterPanel.h"
#include "ui/panels/rc_panel_inspector_sidebar.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"

#include <RogueCity/Visualizer/LucideIcons.hpp>
#include <RogueCity/Visualizer/SvgTextureCache.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <string>

namespace RC_UI::Panels::ActivityBarRight {

namespace {

// ── P4 tab entries ────────────────────────────────────────────────────────────
struct P4Entry {
    const char* label;
    const char* tooltip;
    const char* action_id;
    const char* icon_path;
    int tab_index; // matches RcInspectorSidebar tab order: 0=Inspector 1=SystemMap 2=Health
    ToolLibrary color_tool;
    int color_variant;
};

static constexpr P4Entry kP4Entries[] = {
    { "IN", "Inspector Runtime",   "b2.inspector",   LC::Activity,   0, ToolLibrary::Axiom,    0 },
    { "SM", "System Map",          "b2.system_map",  LC::Map,        1, ToolLibrary::Road,     0 },
    { "HT", "City Health",         "b2.health",      LC::MonitorCog, 2, ToolLibrary::District, 0 },
};

// ── Global config entries ─────────────────────────────────────────────────────
// These route to panels inside RcMasterPanel (System category).
struct ConfigEntry {
    const char*  label;
    const char*  tooltip;
    const char*  action_id;
    const char*  icon_path;
    PanelType    panel;   // RequestPanel() target
    ToolLibrary  color_tool;
    int          color_variant;
};

static constexpr ConfigEntry kConfigEntries[] = {
    // UISettings drawer = Workspace + Theme controls
    { "UI", "UI Settings / Workspace", "b2.workspace", LC::Sliders,    PanelType::UISettings, ToolLibrary::Water,    0 },
    { "SY", "System / Dev Shell",      "b2.devshell",  LC::MonitorCog, PanelType::Log,        ToolLibrary::Building, 0 },
};

bool IconButton(const char* id, const char* fallback_label,
                const char* tooltip,
                const char* icon_path,
                bool active, float size, ImU32 accent) {
    ImVec2 cursor = ImGui::GetCursorScreenPos();

    if (active) {
        // Right-side accent bar (B2 mirrors B1's left-side bar)
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(cursor.x + size - 3.0f, cursor.y),
            ImVec2(cursor.x + size,        cursor.y + size),
            accent, 0.0f);

        ImGui::GetWindowDrawList()->AddRectFilled(
            cursor,
            ImVec2(cursor.x + size, cursor.y + size),
            RC_UI::WithAlpha(accent, 40),
            3.0f);
    }

    ImGui::PushStyleColor(ImGuiCol_Button,        UITokens::Transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RC_UI::WithAlpha(accent, 30));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  RC_UI::WithAlpha(accent, 70));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

    const std::string button_id = std::string("##") + (id ? id : "activity.right.icon");
    const bool clicked = API::Button(button_id.c_str(), ImVec2(size, size));

    bool drew_icon = false;
    if (icon_path != nullptr) {
        const float icon_size = std::min(size * 0.56f, std::max(12.0f, size - 10.0f));
        if (ImTextureID icon = RC::SvgTextureCache::Get().Load(icon_path, icon_size)) {
            const ImVec2 icon_pos(cursor.x + (size - icon_size) * 0.5f,
                                  cursor.y + (size - icon_size) * 0.5f);
            ImGui::GetWindowDrawList()->AddImage(icon,
                                                 icon_pos,
                                                 ImVec2(icon_pos.x + icon_size,
                                                        icon_pos.y + icon_size),
                                                 ImVec2(0, 0), ImVec2(1, 1),
                                                 active ? LerpColor(accent, UITokens::TextPrimary, 0.20f)
                                                        : accent);
            drew_icon = true;
        }
    }
    if (!drew_icon && fallback_label && fallback_label[0] != '\0') {
        const ImVec2 text_size = ImGui::CalcTextSize(fallback_label);
        const ImVec2 text_pos(cursor.x + (size - text_size.x) * 0.5f,
                              cursor.y + (size - text_size.y) * 0.5f);
        ImGui::GetWindowDrawList()->AddText(text_pos, accent, fallback_label);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && tooltip) {
        ImGui::SetTooltip("%s", tooltip);
    }

    return clicked;
}

} // anonymous namespace

void Draw(float /*dt*/) {
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse   | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav        | ImGuiWindowFlags_NoMove;

    static RC_UI::DockableWindowState s_win_state;
    ImGui::SetNextWindowBgAlpha(0.92f);
    const bool open = RC_UI::BeginDockableWindow("ActivityBarRight", s_win_state, "B2", flags);

    uiint.BeginPanel({"ActivityBarRight","Activity Bar Right","activity_bar_right",
                      "B2","visualizer/src/ui/panels/rc_panel_activity_bar_right.cpp",
                      {"activity","bar","navigation","inspector"}}, open);

    if (!open) {
        uiint.EndPanel();
        return;
    }

    const float avail_w  = ImGui::GetContentRegionAvail().x;
    const float btn_size = std::max(24.0f, avail_w - 4.0f);
    const float margin_x = (avail_w - btn_size) * 0.5f;

    const int active_tab = RcInspectorSidebar::GetRequestedTab();

    API::Spacing();

    // ── P4 tab buttons ────────────────────────────────────────────────────────
    for (const auto& e : kP4Entries) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + margin_x);
        const ImU32 accent = RC_UI::ToolColor(e.color_tool, e.color_variant);
        if (IconButton(e.action_id, e.label, e.tooltip, e.icon_path,
                   active_tab == e.tab_index, btn_size, accent)) {
            RcInspectorSidebar::RequestTab(e.tab_index);
        }
        uiint.RegisterWidget({"button", e.tooltip, e.action_id,
                               {"activity","p4","inspector"}});
        API::Spacing();
    }

    // ── Separator ─────────────────────────────────────────────────────────────
    API::Separator();
    API::Spacing();

    // ── Global config buttons — route to P3 System/Config panels ─────────────
    const PanelCategory cur_cat = RcMasterPanel::GetRequestedCategory();
    for (const auto& c : kConfigEntries) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + margin_x);
        const bool active = (cur_cat == PanelCategory::System);
        const ImU32 accent = RC_UI::ToolColor(c.color_tool, c.color_variant);
        if (IconButton(c.action_id, c.label, c.tooltip, c.icon_path,
                   active, btn_size, accent)) {
            RcMasterPanel::RequestPanel(c.panel);
        }
        uiint.RegisterWidget({"button", c.tooltip, c.action_id,
                               {"activity","config","global"}});
        API::Spacing();
    }

    uiint.EndPanel();
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::ActivityBarRight
