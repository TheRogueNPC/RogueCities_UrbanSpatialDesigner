// FILE: visualizer/src/ui/panels/rc_panel_activity_bar_right.cpp
// PURPOSE: B2 Activity Bar — vertical icon strip (far right).

#include "ui/panels/rc_panel_activity_bar_right.h"

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

namespace RC_UI::Panels::ActivityBarRight {

namespace {

// ── P4 tab entries ────────────────────────────────────────────────────────────
struct P4Entry {
    const char* label;
    const char* tooltip;
    const char* action_id;
    const char* icon_path;
    int tab_index; // matches RcInspectorSidebar tab order: 0=Inspector 1=SystemMap 2=Health
};

static constexpr P4Entry kP4Entries[] = {
    { "IN", "Inspector Runtime",   "b2.inspector",   LC::Activity,   0 },
    { "SM", "System Map",          "b2.system_map",  LC::Map,        1 },
    { "HT", "City Health",         "b2.health",      LC::MonitorCog, 2 },
};

// ── Global config entries ─────────────────────────────────────────────────────
// These route to panels inside RcMasterPanel (System category).
struct ConfigEntry {
    const char*  label;
    const char*  tooltip;
    const char*  action_id;
    const char*  icon_path;
    PanelType    panel;   // RequestPanel() target
};

static constexpr ConfigEntry kConfigEntries[] = {
    // UISettings drawer = Workspace + Theme controls
    { "UI", "UI Settings / Workspace", "b2.workspace", LC::Sliders,    PanelType::UISettings },
    { "SY", "System / Dev Shell",      "b2.devshell",  LC::MonitorCog, PanelType::Log        },
};

bool IconButton(const char* label, const char* tooltip,
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
            RC_UI::WithAlpha(UITokens::AmberGlow, 40), // AmberGlow tint
            3.0f);
    }

    ImGui::PushStyleColor(ImGuiCol_Button,        UITokens::Transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RC_UI::WithAlpha(UITokens::AmberGlow, 30));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  RC_UI::WithAlpha(UITokens::AmberGlow, 70));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

    const bool clicked = ImGui::Button(label, ImVec2(size, size));

    if (icon_path != nullptr) {
        if (ImTextureID icon = RC::SvgTextureCache::Get().Load(icon_path, size * 0.56f)) {
            const float icon_size = size * 0.56f;
            const ImVec2 icon_pos(cursor.x + (size - icon_size) * 0.5f,
                                  cursor.y + (size - icon_size) * 0.5f);
            ImGui::GetWindowDrawList()->AddImage(icon,
                                                 icon_pos,
                                                 ImVec2(icon_pos.x + icon_size,
                                                        icon_pos.y + icon_size),
                                                 ImVec2(0, 0), ImVec2(1, 1),
                                                 UITokens::TextPrimary);
        }
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

    ImGui::Spacing();

    // ── P4 tab buttons ────────────────────────────────────────────────────────
    for (const auto& e : kP4Entries) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + margin_x);
        if (IconButton(e.label, e.tooltip, e.icon_path,
                   active_tab == e.tab_index, btn_size, UITokens::AmberGlow)) {
            RcInspectorSidebar::RequestTab(e.tab_index);
        }
        uiint.RegisterWidget({"button", e.tooltip, e.action_id,
                               {"activity","p4","inspector"}});
        ImGui::Spacing();
    }

    // ── Separator ─────────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::Spacing();

    // ── Global config buttons — route to P3 System/Config panels ─────────────
    const PanelCategory cur_cat = RcMasterPanel::GetRequestedCategory();
    for (const auto& c : kConfigEntries) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + margin_x);
        const bool active = (cur_cat == PanelCategory::System);
        if (IconButton(c.label, c.tooltip, c.icon_path,
                   active, btn_size, UITokens::GreenHUD)) {
            RcMasterPanel::RequestPanel(c.panel);
        }
        uiint.RegisterWidget({"button", c.tooltip, c.action_id,
                               {"activity","config","global"}});
        ImGui::Spacing();
    }

    uiint.EndPanel();
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::ActivityBarRight
