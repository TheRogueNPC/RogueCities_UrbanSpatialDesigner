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
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float bar_height = std::clamp(avail.y, 32.0f, 46.0f);
    if (bar_height < 32.0f || avail.x < 120.0f) {
        EndWindowContainer();
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }

    const ImVec2 bar_end(start.x + avail.x, start.y + bar_height);
    draw_list->AddRectFilled(start, bar_end, ImGui::ColorConvertFloat4ToU32(ZoneAxiom.base), 22.0f);

    ImGui::SetCursorScreenPos(ImVec2(start.x + 18.0f, start.y + (bar_height - 28.0f) * 0.5f));
    static std::array<ReactiveF, RC_UI::kToolLibraryOrder.size()> chip_hover;
    constexpr float kChipBaseWidth = 110.0f;
    constexpr float kChipHoverExpansion = 30.0f;
    const float chip_height = std::clamp(bar_height - 10.0f, 20.0f, 28.0f);
    const ImVec2 chip_box(140.0f, chip_height);

    for (size_t i = 0; i < RC_UI::kToolLibraryOrder.size(); ++i) {
        const auto tool = RC_UI::kToolLibraryOrder[i];
        if (i > 0) {
            ImGui::SameLine();
        }
        ImGui::PushID(static_cast<int>(i));
        ImGui::InvisibleButton("ToolChip", chip_box);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            RC_UI::ToggleToolLibrary(tool);
        }
        const char* label = ToolLabel(tool);
        uiint.RegisterWidget({"button", label, ToolActionId(tool), {"action", "nav"}});
        uiint.RegisterAction({ToolActionKey(tool), ToolActionLabel(tool), "Tool Deck", {}, "RC_UI::ToggleToolLibrary"});
        chip_hover[i].target = ImGui::IsItemHovered() ? 1.0f : 0.0f;
        chip_hover[i].Update(dt);

        const ImVec2 chip_min = ImGui::GetItemRectMin();
        const float chip_width = kChipBaseWidth + kChipHoverExpansion * chip_hover[i].v;
        const ImVec2 chip_max(chip_min.x + chip_width, chip_min.y + chip_box.y);
        draw_list->AddRectFilled(chip_min, chip_max, ImGui::ColorConvertFloat4ToU32(ZoneAxiom.accent), 14.0f);
        draw_list->AddText(ImVec2(chip_min.x + 30.0f, chip_min.y + (chip_box.y - 16.0f) * 0.5f),
            ImGui::ColorConvertFloat4ToU32(ColorBG), label);
        DrawToolIcon(draw_list, tool, ImVec2(chip_min.x + 14.0f, chip_min.y + chip_box.y * 0.5f), 12.0f);
        ImGui::PopID();
    }

}

void Draw(float dt)
{
    const bool open = Components::BeginTokenPanel("Tool Deck", UITokens::CyanAccent);

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

    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }

    BeginWindowContainer("##tool_deck_container");
    
    DrawContent(dt);
    
    EndWindowContainer();
    uiint.EndPanel();
    Components::EndTokenPanel();
}

} // namespace RC_UI::Panels::AxiomBar

