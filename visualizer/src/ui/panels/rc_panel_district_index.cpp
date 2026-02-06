// FILE: rc_panel_district_index.cpp
// PURPOSE: Implementation of the district index panel.

#include "ui/panels/rc_panel_district_index.h"
#include <imgui.h>

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"

namespace RC_UI::Panels::DistrictIndex {

void Draw(float /*dt*/)
{
    using RogueCity::Core::Editor::GetGlobalState;
    auto& gs = GetGlobalState();
    ImGui::Begin("District Index", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Districts: %llu", static_cast<unsigned long long>(gs.districts.size()));
    ImGui::Separator();
    for (size_t i = 0; i < gs.districts.size(); ++i) {
        const auto& district = gs.districts[i];
        // Display ID and axiom information
        char label[128];
        snprintf(label, sizeof(label), "District %u (Primary Axiom: %d)", district.id, district.primary_axiom_id);
        ImGui::Selectable(label);
    }
    ImGui::End();
}

} // namespace RC_UI::Panels::DistrictIndex