// FILE: visualizer/src/ui/panels/rc_panel_inspector.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Inspector with reactive selection focus.
#include "ui/panels/rc_panel_inspector.h"

#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_theme.h"
#include "ui/rc_ui_components.h"
#include "ui/panels/rc_property_editor.h"
#include "ui/introspection/UiIntrospection.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"

#include <imgui.h>

namespace RC_UI::Panels::Inspector {

void DrawContent(float dt)
{
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    
    static ReactiveF focus;
    constexpr float kBaseBorderThickness = 2.0f;
    const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    focus.target = hovered ? 1.0f : 0.0f;
    focus.Update(dt);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 40.0f);
    const ImVec2 end(pos.x + size.x, pos.y + size.y);
    draw_list->AddRectFilled(pos, end, ImGui::ColorConvertFloat4ToU32(Palette::Nebula), 12.0f);
    draw_list->AddRect(pos, end, ImGui::ColorConvertFloat4ToU32(ZoneAxiom.accent), 12.0f, 0,
        kBaseBorderThickness + kBaseBorderThickness * focus.v);
    ImGui::Dummy(size);
    ImGui::Spacing();

    // Push Mockup Y2K Styling for the Inspector
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(UITokens::BackgroundDark));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::PanelBackground, 255)));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::AmberGlow, 100)));
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::GridOverlay, 180)));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::AmberGlow, 200)));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImGui::ColorConvertU32ToFloat4(UITokens::AmberGlow));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0.0f);

    static PropertyEditor editor;
    editor.Draw(gs);
    
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(7);

    uiint.RegisterWidget({"property_editor", "Selection", "selection_manager", {"detail"}});

}

void Draw(float dt)
{
    const bool open = Components::BeginTokenPanel("Inspector", UITokens::MagentaHighlight);
    if (!open) {
        Components::EndTokenPanel();
        return;
    }

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Inspector",
            "Inspector",
            "inspector",
            "ToolDeck",
            "visualizer/src/ui/panels/rc_panel_inspector.cpp",
            {"detail", "selection"}
        },
        true
    );
    
    DrawContent(dt);
    
    uiint.EndPanel();
    Components::EndTokenPanel();
}

} // namespace RC_UI::Panels::Inspector
