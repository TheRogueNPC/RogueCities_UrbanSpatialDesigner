// FILE: visualizer/src/ui/panels/rc_panel_axiom_bar.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Tool Deck dock with reactive deck chips.
#include "ui/panels/rc_panel_axiom_bar.h"

#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_theme.h"
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"

#include <imgui.h>

#include <algorithm>

namespace RC_UI::Panels::AxiomBar {

namespace {
    const char* ToolLabel(RC_UI::ToolLibrary tool) {
        switch (tool) {
            case RC_UI::ToolLibrary::Axiom: return "Axiom Deck";
            case RC_UI::ToolLibrary::Water: return "Water Deck";
            case RC_UI::ToolLibrary::Road: return "Road Deck";
            case RC_UI::ToolLibrary::District: return "District Deck";
            case RC_UI::ToolLibrary::Lot: return "Lot Deck";
            case RC_UI::ToolLibrary::Building: return "Building Deck";
        }
        return "Deck";
    }

    const char* ToolActionId(RC_UI::ToolLibrary tool) {
        switch (tool) {
            case RC_UI::ToolLibrary::Axiom: return "action:axiom_library.toggle";
            case RC_UI::ToolLibrary::Water: return "action:water_library.toggle";
            case RC_UI::ToolLibrary::Road: return "action:road_library.toggle";
            case RC_UI::ToolLibrary::District: return "action:district_library.toggle";
            case RC_UI::ToolLibrary::Lot: return "action:lot_library.toggle";
            case RC_UI::ToolLibrary::Building: return "action:building_library.toggle";
        }
        return "action:tool_library.toggle";
    }

    const char* ToolActionKey(RC_UI::ToolLibrary tool) {
        switch (tool) {
            case RC_UI::ToolLibrary::Axiom: return "axiom_library.toggle";
            case RC_UI::ToolLibrary::Water: return "water_library.toggle";
            case RC_UI::ToolLibrary::Road: return "road_library.toggle";
            case RC_UI::ToolLibrary::District: return "district_library.toggle";
            case RC_UI::ToolLibrary::Lot: return "lot_library.toggle";
            case RC_UI::ToolLibrary::Building: return "building_library.toggle";
        }
        return "tool_library.toggle";
    }

    const char* ToolActionLabel(RC_UI::ToolLibrary tool) {
        switch (tool) {
            case RC_UI::ToolLibrary::Axiom: return "Toggle Axiom Library";
            case RC_UI::ToolLibrary::Water: return "Toggle Water Library";
            case RC_UI::ToolLibrary::Road: return "Toggle Road Library";
            case RC_UI::ToolLibrary::District: return "Toggle District Library";
            case RC_UI::ToolLibrary::Lot: return "Toggle Lot Library";
            case RC_UI::ToolLibrary::Building: return "Toggle Building Library";
        }
        return "Toggle Tool Library";
    }

    void DrawToolIcon(ImDrawList* draw_list, RC_UI::ToolLibrary tool, const ImVec2& center, float size) {
        const ImU32 color = ImGui::ColorConvertFloat4ToU32(ColorBG);
        const float half = size * 0.5f;
        switch (tool) {
            case RC_UI::ToolLibrary::Axiom:
                draw_list->AddCircle(center, half, color, 12, 2.0f);
                draw_list->AddCircleFilled(center, half * 0.35f, color, 12);
                break;
            case RC_UI::ToolLibrary::Water:
                draw_list->AddTriangleFilled(
                    ImVec2(center.x, center.y - half),
                    ImVec2(center.x - half, center.y + half),
                    ImVec2(center.x + half, center.y + half),
                    color);
                break;
            case RC_UI::ToolLibrary::Road:
                draw_list->AddLine(
                    ImVec2(center.x - half, center.y + half),
                    ImVec2(center.x + half, center.y - half),
                    color, 2.5f);
                break;
            case RC_UI::ToolLibrary::District:
                draw_list->AddRect(
                    ImVec2(center.x - half, center.y - half),
                    ImVec2(center.x + half, center.y + half),
                    color, 3.0f, 0, 2.0f);
                break;
            case RC_UI::ToolLibrary::Lot:
                draw_list->AddRect(
                    ImVec2(center.x - half, center.y - half),
                    ImVec2(center.x + half, center.y + half),
                    color, 0.0f, 0, 2.0f);
                draw_list->AddLine(
                    ImVec2(center.x, center.y - half),
                    ImVec2(center.x, center.y + half),
                    color, 1.5f);
                draw_list->AddLine(
                    ImVec2(center.x - half, center.y),
                    ImVec2(center.x + half, center.y),
                    color, 1.5f);
                break;
            case RC_UI::ToolLibrary::Building:
                draw_list->AddRectFilled(
                    ImVec2(center.x - half, center.y - half),
                    ImVec2(center.x + half, center.y + half),
                    color, 2.0f);
                draw_list->AddRect(
                    ImVec2(center.x - half, center.y - half),
                    ImVec2(center.x + half, center.y + half),
                    color, 2.0f, 0, 2.0f);
                break;
        }
    }
}

void DrawContent(float dt)
{
    (void)dt;
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 180.0f || avail.y < 40.0f) {
        ImGui::TextDisabled("Expand Tool Deck to access library controls.");
        if (avail.x > 110.0f && ImGui::Button("Reset Dock Layout")) {
            RC_UI::ResetDockLayout();
        }
        return;
    }

    const int tool_count = static_cast<int>(RC_UI::kToolLibraryOrder.size());
    const float min_icon_size = 34.0f;
    const float base_icon_size = 46.0f;
    const float min_spacing = 6.0f;

    float icon_size = base_icon_size;
    float spacing = 10.0f;
    const float preferred_width = tool_count * icon_size + (tool_count - 1) * spacing;
    if (preferred_width > avail.x) {
        icon_size = std::max(min_icon_size, (avail.x - (tool_count - 1) * min_spacing) / tool_count);
        spacing = std::max(min_spacing, (avail.x - icon_size * tool_count) / std::max(1, tool_count - 1));
    } else {
        spacing = std::max(8.0f, (avail.x - icon_size * tool_count) / std::max(1, tool_count - 1));
    }

    for (size_t i = 0; i < RC_UI::kToolLibraryOrder.size(); ++i) {
        const auto tool = RC_UI::kToolLibraryOrder[i];
        if (i > 0) {
            ImGui::SameLine(0.0f, spacing);
        }
        ImGui::PushID(static_cast<int>(i));

        ImGui::InvisibleButton("ToolChip", ImVec2(icon_size, icon_size));
        const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        const bool double_clicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        if (double_clicked && tool != RC_UI::ToolLibrary::Axiom) {
            RC_UI::PopoutToolLibrary(tool);
        } else if (clicked) {
            RC_UI::ActivateToolLibrary(tool);
        }

        const char* label = ToolLabel(tool);
        uiint.RegisterWidget({"button", label, ToolActionId(tool), {"action", "nav"}});
        uiint.RegisterAction({ToolActionKey(tool), ToolActionLabel(tool), "Tool Deck", {}, "RC_UI::ToggleToolLibrary"});

        const bool is_open = RC_UI::IsToolLibraryOpen(tool);
        const bool is_popout = RC_UI::IsToolLibraryPopoutOpen(tool);
        const bool hovered = ImGui::IsItemHovered();
        const ImVec2 bmin = ImGui::GetItemRectMin();
        const ImVec2 bmax = ImGui::GetItemRectMax();
        const ImVec2 center((bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        const ImU32 fill = hovered
            ? WithAlpha(UITokens::PanelBackground, 235u)
            : WithAlpha(UITokens::PanelBackground, 210u);
        const ImU32 border = is_popout
            ? WithAlpha(UITokens::MagentaHighlight, 240u)
            : (is_open ? WithAlpha(UITokens::CyanAccent, 230u) : WithAlpha(UITokens::TextSecondary, 170u));

        draw_list->AddRectFilled(bmin, bmax, fill, 8.0f);
        draw_list->AddRect(bmin, bmax, border, 8.0f, 0, is_open || is_popout ? 2.2f : 1.4f);
        DrawToolIcon(draw_list, tool, center, icon_size * 0.44f);

        if (hovered) {
            if (tool == RC_UI::ToolLibrary::Axiom) {
                ImGui::SetTooltip("%s\nClick: open shared library frame", label);
            } else {
                ImGui::SetTooltip("%s\nClick: open shared library frame\nDouble-click: popout clone", label);
            }
        }
        ImGui::PopID();
    }
    ImGui::Spacing();
    ImGui::TextDisabled("Click opens in library frame. Double-click pops out a duplicate.");
}

void Draw(float dt)
{
    static RC_UI::DockableWindowState s_tool_deck_window;
    const bool open = RC_UI::BeginDockableWindow("Tool Deck", s_tool_deck_window, "ToolDeck", ImGuiWindowFlags_NoCollapse);

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Tool Deck",
            "Tool Deck",
            "tool_deck",
            "ToolDeck",
            "visualizer/src/ui/panels/rc_panel_axiom_bar.cpp",
            {"deck", "nav"}
        },
        open
    );

    if (open) {
        DrawContent(dt);
        RC_UI::EndDockableWindow();
    }
    uiint.EndPanel();
}

} // namespace RC_UI::Panels::AxiomBar

