// FILE: rc_panel_tools.cpp
// PURPOSE: Left-hand tool shelf panel implementation.

#include "ui/panels/rc_panel_tools.h"
#include <imgui.h>

namespace RC_UI::Panels::Tools {

void Draw(float /*dt*/)
{
    // Tools panel is always available when docking is enabled.
    ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::TextUnformatted("Tool Shelf");
    ImGui::Separator();
    ImGui::TextUnformatted("View");
    ImGui::TextUnformatted("Edit Roads");
    ImGui::TextUnformatted("Edit Districts");
    ImGui::TextUnformatted("Edit Lots");
    ImGui::TextUnformatted("Placeholder Tools");
    ImGui::End();
}

} // namespace RC_UI::Panels::Tools