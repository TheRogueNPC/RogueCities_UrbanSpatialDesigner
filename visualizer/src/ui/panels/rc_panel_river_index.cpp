// FILE: rc_panel_river_index.cpp
// PURPOSE: Implementation of river index panel (currently a stub).

#include "ui/panels/rc_panel_river_index.h"
#include <imgui.h>

namespace RC_UI::Panels::RiverIndex {

void Draw(float /*dt*/)
{
    ImGui::Begin("River Index", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::TextUnformatted("No rivers to display");
    ImGui::End();
}

} // namespace RC_UI::Panels::RiverIndex