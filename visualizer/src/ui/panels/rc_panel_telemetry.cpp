// FILE: rc_panel_telemetry.cpp
// PURPOSE: Live analytics panel showing procedural generation metrics.

#include "ui/panels/rc_panel_telemetry.h"
#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>

namespace RC_UI::Panels::Telemetry {

void Draw(float dt)
{
    // Color the panel background using the theme palette
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorPanel);
    // The window title is "Analytics" to reflect its purpose
    ImGui::Begin("Analytics", nullptr, ImGuiWindowFlags_NoCollapse);

    // TODO: Bind real metrics here. The following reactive bar animates using Pulse().
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 120.0f);

    static ReactiveF fill;
    fill.target = Pulse(static_cast<float>(ImGui::GetTime()), 1.2f);
    fill.Update(dt);

    const ImVec2 bar_end(start.x + size.x, start.y + size.y);
    draw_list->AddRectFilled(start, bar_end, ImGui::ColorConvertFloat4ToU32(Palette::Nebula), 18.0f);

    const float fill_height = size.y * (0.2f + 0.7f * fill.v);
    const ImVec2 fill_start(start.x + 8.0f, bar_end.y - fill_height - 8.0f);
    const ImVec2 fill_end(bar_end.x - 8.0f, bar_end.y - 8.0f);
    draw_list->AddRectFilled(fill_start, fill_end, ImGui::ColorConvertFloat4ToU32(ZoneTelemetry.accent), 14.0f);

    ImGui::Dummy(size);
    ImGui::Text("Flow Rate");
    ImGui::TextColored(ColorAccentB, "%.2f", fill.v);

    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Panels::Telemetry