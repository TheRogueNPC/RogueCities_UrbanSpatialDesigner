// FILE: visualizer/src/ui/panels/rc_panel_log.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Stub log panel with reactive highlight.
#include "ui/panels/rc_panel_log.h"

#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>

namespace RC_UI::Panels::Log {

void Draw(float dt)
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorPanel);
    ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoCollapse);

    // TODO (visualizer/src/ui/panels/rc_panel_log.cpp): Wire log stream to generator events.
    static ReactiveF flash;
    constexpr float kGlowBase = 0.2f;
    constexpr float kGlowRange = 0.5f;
    flash.target = ImGui::IsWindowHovered() ? 1.0f : 0.0f;
    flash.Update(dt);

    const ImVec4 glow = ImVec4(ColorAccentB.x, ColorAccentB.y, ColorAccentB.z, kGlowBase + kGlowRange * flash.v);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, glow);
    ImGui::BeginChild("LogStream", ImVec2(0.0f, 0.0f), true);
    ImGui::Text("↳ Generator: Ready");
    ImGui::Text("↳ Axiom field locked");
    ImGui::Text("↳ District solver idle");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Panels::Log
