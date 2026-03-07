// FILE: visualizer/src/ui/panels/rc_panel_activity_bar_left.cpp
// PURPOSE: B1 Activity Bar — vertical domain-icon strip (far left).

#include "ui/panels/rc_panel_activity_bar_left.h"

#include "ui/api/rc_imgui_api.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/panels/RcMasterPanel.h"
#include "ui/panels/IPanelDrawer.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_tokens.h"

#include <RogueCity/Visualizer/LucideIcons.hpp>
#include <RogueCity/Visualizer/SvgTextureCache.hpp>

#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <string>

namespace RC_UI::Panels::ActivityBarLeft {

namespace {

using RogueCity::Core::Editor::EditorState;
using RogueCity::Core::Editor::EditorEvent;
using RogueCity::Core::Editor::GetEditorHFSM;
using RogueCity::Core::Editor::GetGlobalState;

// ── Domain entry ──────────────────────────────────────────────────────────────
struct DomainEntry {
    const char* label;      // Short glyph / abbreviation shown on the button
    const char* tooltip;    // Shown on hover
    const char* action_id;  // For introspector
    const char* icon_path;
    ToolLibrary tool;
    int color_variant;
    EditorEvent event;      // HFSM event to fire on click
    EditorState state;      // State that represents "active" for highlight
};

static constexpr DomainEntry kDomains[] = {
    { "AX", "Axiom = radius",                "activity.axiom",    LC::Radius,       ToolLibrary::Axiom,    0, EditorEvent::Tool_Axioms,    EditorState::Editing_Axioms    },
    { "WA", "Water = waves",                 "activity.water",    LC::Waves,        ToolLibrary::Water,    0, EditorEvent::Tool_Water,     EditorState::Editing_Water     },
    { "RD", "Road = waypoints",              "activity.road",     LC::Waypoints,    ToolLibrary::Road,     0, EditorEvent::Tool_Roads,     EditorState::Editing_Roads     },
    { "ZN", "Zone/Districts = chart-network","activity.zone",     LC::ChartNetwork, ToolLibrary::District, 0, EditorEvent::Tool_Districts, EditorState::Editing_Districts },
    { "LT", "Lot = map-pin-house",           "activity.lot",      LC::MapPinHouse,  ToolLibrary::Lot,      1, EditorEvent::Tool_Lots,      EditorState::Editing_Lots      },
    { "BL", "Building = building-2",         "activity.building", LC::Building2,    ToolLibrary::Building, 0, EditorEvent::Tool_Buildings, EditorState::Editing_Buildings },
};

// ── Index / Explorer mode entries ────────────────────────────────────────────
struct IndexEntry {
    const char* label;
    const char* tooltip;
    const char* action_id;
    const char* icon_path;
    PanelCategory category;
    ToolLibrary tool;
    int color_variant;
};

static constexpr IndexEntry kIndexModes[] = {
    { "ID", "Indices = book-marked (ID)", "activity.idx.road",      LC::BookMarked, PanelCategory::Indices, ToolLibrary::Road,     0 },
    { "Tu", "Indices = book-marked (Tu)", "activity.idx.district",  LC::BookMarked, PanelCategory::Indices, ToolLibrary::District, 0 },
    { "Ix", "Indices = book-marked",      "activity.idx.lot",       LC::BookMarked, PanelCategory::Indices, ToolLibrary::Lot,      1 },
};

// Draw a square icon button, returns true if clicked.
// Colours the background when `active` is true.
bool IconButton(const char* id, const char* fallback_label,
                const char* tooltip,
                const char* icon_path,
                bool active, float size, ImU32 accent_color) {
    ImVec2 cursor = ImGui::GetCursorScreenPos();

    // Active background fill
    if (active) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            cursor,
            ImVec2(cursor.x + size, cursor.y + size),
            RC_UI::WithAlpha(accent_color, 55),
            3.0f);
    }

    // Coloured left accent bar on active
    if (active) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            cursor,
            ImVec2(cursor.x + 3.0f, cursor.y + size),
            accent_color,
            0.0f);
    }

    ImGui::PushStyleColor(ImGuiCol_Button,        UITokens::Transparent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RC_UI::WithAlpha(accent_color, 35));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  RC_UI::WithAlpha(accent_color, 80));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

    const std::string button_id = std::string("##") + (id ? id : "activity.left.icon");
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
                                                 active ? LerpColor(accent_color, UITokens::TextPrimary, 0.20f)
                                                        : accent_color);
            drew_icon = true;
        }
    }
    if (!drew_icon && fallback_label && fallback_label[0] != '\0') {
        const ImVec2 text_size = ImGui::CalcTextSize(fallback_label);
        const ImVec2 text_pos(cursor.x + (size - text_size.x) * 0.5f,
                              cursor.y + (size - text_size.y) * 0.5f);
        ImGui::GetWindowDrawList()->AddText(text_pos, accent_color, fallback_label);
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
    auto& uiint   = RogueCity::UIInt::UiIntrospector::Instance();
    auto& hfsm    = GetEditorHFSM();
    auto& gs      = GetGlobalState();

    // Window is docked into B1 node by BuildDefaultWorkspace. The node has
    // NoTabBar | NoResize flags so we just fill whatever space we get.
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse   | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav        | ImGuiWindowFlags_NoMove;

    static RC_UI::DockableWindowState s_win_state;
    ImGui::SetNextWindowBgAlpha(0.92f);
    const bool open = RC_UI::BeginDockableWindow("ActivityBarLeft", s_win_state, "B1", flags);

    uiint.BeginPanel({"ActivityBarLeft","Activity Bar Left","activity_bar_left",
                      "B1","visualizer/src/ui/panels/rc_panel_activity_bar_left.cpp",
                      {"activity","bar","navigation","hfsm"}}, open);

    if (!open) {
        uiint.EndPanel();
        return;
    }

    const float avail_w  = ImGui::GetContentRegionAvail().x;
    // Use almost the full width for the button, leave 2px margin each side.
    const float btn_size = std::max(24.0f, avail_w - 4.0f);
    const float margin_x = (avail_w - btn_size) * 0.5f;

    const EditorState current_state = hfsm.state();

    // ── Domain buttons ────────────────────────────────────────────────────────
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + margin_x);
    API::Spacing();

    for (const auto& d : kDomains) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + margin_x);
        const bool active = (current_state == d.state);
        const ImU32 color = RC_UI::ToolColor(d.tool, d.color_variant);
        if (IconButton(d.action_id, d.label, d.tooltip, d.icon_path, active, btn_size,
                       color)) {
            hfsm.handle_event(d.event, gs);
            // Switch P3 to Tool mode (not Index mode) when a domain is selected
            RcMasterPanel::RequestCategory(PanelCategory::Tools);
        }
        uiint.RegisterWidget({"button", d.tooltip, d.action_id, {"activity","domain"}});
        API::Spacing();
    }

    // ── Separator ─────────────────────────────────────────────────────────────
    API::Separator();
    API::Spacing();

    // ── Index / Explorer mode buttons ─────────────────────────────────────────
    // These switch P3 to its Index/Explorer view without changing HFSM domain.
    const PanelCategory requested = RcMasterPanel::GetRequestedCategory();

    for (const auto& idx : kIndexModes) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + margin_x);
        const bool active = (requested == PanelCategory::Indices);
        const ImU32 color = RC_UI::ToolColor(idx.tool, idx.color_variant);
        if (IconButton(idx.action_id, idx.label, idx.tooltip, idx.icon_path, active,
                       btn_size, color)) {
            RcMasterPanel::RequestCategory(PanelCategory::Indices);
        }
        uiint.RegisterWidget({"button", idx.tooltip, idx.action_id,
                               {"activity","index","explorer"}});
        API::Spacing();
    }

    uiint.EndPanel();
    RC_UI::EndDockableWindow();
}

} // namespace RC_UI::Panels::ActivityBarLeft
