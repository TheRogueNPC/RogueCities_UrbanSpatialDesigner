// FILE: visualizer/src/ui/panels/rc_panel_system_map.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Stub system map visualization with reactive focus ring.
#include "ui/panels/rc_panel_system_map.h"

#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>

namespace RC_UI::Panels::SystemMap {

void Draw(float dt)
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorBG);
    ImGui::Begin("System Map", nullptr, ImGuiWindowFlags_NoCollapse);

    // TODO (visualizer/src/ui/panels/rc_panel_system_map.cpp): Replace placeholder map with live topology render.
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 panel_pos = ImGui::GetCursorScreenPos();
    const ImVec2 panel_size = ImGui::GetContentRegionAvail();
    const ImVec2 center(panel_pos.x + panel_size.x * 0.5f, panel_pos.y + panel_size.y * 0.5f);
    const float radius = (panel_size.x < panel_size.y ? panel_size.x : panel_size.y) * 0.22f;

    static ReactiveF focus;
    focus.target = ImGui::IsWindowHovered() ? 1.0f : 0.0f;
    focus.Update(dt);

    draw_list->AddRectFilled(panel_pos,
        ImVec2(panel_pos.x + panel_size.x, panel_pos.y + panel_size.y),
        ImGui::ColorConvertFloat4ToU32(ColorPanel), 18.0f);
    draw_list->AddCircleFilled(center, radius, ImGui::ColorConvertFloat4ToU32(ZoneTransit.base), 64);
    draw_list->AddCircle(center, radius + 18.0f * focus.v,
        ImGui::ColorConvertFloat4ToU32(ZoneTransit.accent), 64, 3.0f);
    draw_list->AddCircle(center, radius + 36.0f * focus.v,
        ImGui::ColorConvertFloat4ToU32(ZoneTransit.highlight), 64, 2.0f);

    ImGui::SetCursorScreenPos(ImVec2(panel_pos.x + 24.0f, panel_pos.y + 20.0f));
    ImGui::Text("System Map (stub)");

    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Panels::SystemMap
