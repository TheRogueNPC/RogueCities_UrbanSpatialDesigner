// FILE: visualizer/src/ui/panels/rc_panel_axiom_bar.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Tool Deck dock with reactive deck chips.
#include "ui/panels/rc_panel_axiom_bar.h"

#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_theme.h"
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/tools/rc_tool_dispatcher.h"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

#include <imgui.h>

#include <algorithm>
#include <optional>
#include <string>

namespace RC_UI::Panels::AxiomBar {

namespace {
    std::optional<RC_UI::Tools::ToolActionId> DefaultLibraryAction(RC_UI::ToolLibrary tool) {
        using RC_UI::Tools::ToolActionId;
        switch (tool) {
            case RC_UI::ToolLibrary::Axiom: return ToolActionId::Axiom_Organic;
            case RC_UI::ToolLibrary::Water: return ToolActionId::Water_Flow;
            case RC_UI::ToolLibrary::Road: return ToolActionId::Road_Spline;
            case RC_UI::ToolLibrary::District: return ToolActionId::District_Zone;
            case RC_UI::ToolLibrary::Lot: return ToolActionId::Lot_Plot;
            case RC_UI::ToolLibrary::Building: return ToolActionId::Building_Place;
        }
        return std::nullopt;
    }

    std::optional<RogueCity::Core::Editor::EditorEvent> ToolEvent(RC_UI::ToolLibrary tool) {
        using RogueCity::Core::Editor::EditorEvent;
        switch (tool) {
            case RC_UI::ToolLibrary::Axiom: return EditorEvent::Tool_Axioms;
            case RC_UI::ToolLibrary::Water: return EditorEvent::Tool_Water;
            case RC_UI::ToolLibrary::Road: return EditorEvent::Tool_Roads;
            case RC_UI::ToolLibrary::District: return EditorEvent::Tool_Districts;
            case RC_UI::ToolLibrary::Lot: return EditorEvent::Tool_Lots;
            case RC_UI::ToolLibrary::Building: return EditorEvent::Tool_Buildings;
        }
        return std::nullopt;
    }

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
    auto& hfsm = RogueCity::Core::Editor::GetEditorHFSM();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    
    const ImVec2 avail = ImGui::GetContentRegionAvail();

    const int tool_count = static_cast<int>(RC_UI::kToolLibraryOrder.size());
    const float min_icon_size = 26.0f;
    const float base_icon_size = 46.0f;
    const float min_spacing = 4.0f;

    const int columns = std::clamp(
        static_cast<int>((avail.x + min_spacing) / (min_icon_size + min_spacing)),
        1,
        tool_count);
    const float icon_size = std::clamp(
        (avail.x - min_spacing * static_cast<float>(columns - 1)) / static_cast<float>(columns),
        min_icon_size,
        base_icon_size);
    const float spacing = columns > 1
        ? std::max(min_spacing, (avail.x - icon_size * static_cast<float>(columns)) / static_cast<float>(columns - 1))
        : 0.0f;

    for (size_t i = 0; i < RC_UI::kToolLibraryOrder.size(); ++i) {
        const auto tool = RC_UI::kToolLibraryOrder[i];
        if (i > 0 && (static_cast<int>(i) % columns) != 0) {
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
        if (clicked || double_clicked) {
            bool dispatched = false;
            if (const auto default_action = DefaultLibraryAction(tool); default_action.has_value()) {
                std::string dispatch_status;
                const auto result = RC_UI::Tools::DispatchToolAction(
                    *default_action,
                    RC_UI::Tools::DispatchContext{
                        &hfsm,
                        &gs,
                        &uiint,
                        "Tool Deck"
                    },
                    &dispatch_status);
                dispatched = result == RC_UI::Tools::DispatchResult::Handled;
            }
            if (!dispatched) {
                if (const auto event = ToolEvent(tool); event.has_value()) {
                    hfsm.handle_event(*event, gs);
                }
            }
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
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("Click opens in library frame. Double-click pops out a duplicate.");
    ImGui::PopTextWrapPos();
    if (ImGui::Button("Reset Dock Layout")) {
        RC_UI::ResetDockLayout();
    }
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

