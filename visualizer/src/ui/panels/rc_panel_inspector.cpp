// FILE: visualizer/src/ui/panels/rc_panel_inspector.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Stub inspector with reactive selection focus.
#include "ui/panels/rc_panel_inspector.h"

#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>

namespace RC_UI::Panels::Inspector {

void Draw(float dt)
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorPanel);
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse);

    // TODO (visualizer/src/ui/panels/rc_panel_inspector.cpp): Bind inspector fields to selected entities.
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

    ImGui::Text("Selection: District Node");
    ImGui::TextColored(ColorAccentA, "Status: PENDING");

    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Panels::Inspector
