// FILE: rc_panel_road_index.cpp
// PURPOSE: Implementation of road index panel.

#include "ui/panels/rc_panel_road_index.h"
#include "ui/introspection/UiIntrospection.h"
#include <imgui.h>

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"

namespace RC_UI::Panels::RoadIndex {

void Draw(float /*dt*/)
{
    using RogueCity::Core::Editor::GetGlobalState;
    auto& gs = GetGlobalState();
    const bool open = ImGui::Begin("Road Index", nullptr, ImGuiWindowFlags_NoCollapse);

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Road Index",
            "Road Index",
            "index",
            "Bottom",
            "visualizer/src/ui/panels/rc_panel_road_index.cpp",
            {"roads", "index"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        ImGui::End();
        return;
    }

    ImGui::Text("Roads: %llu", static_cast<unsigned long long>(gs.roads.size()));
    ImGui::Separator();
    for (size_t i = 0; i < gs.roads.size(); ++i) {
        const auto& road = gs.roads[i];
        // Determine classification string (major/minor vs generated)
        const char* user_flag = road.is_user_created ? "User" : "Generated";
        char label[128];
        snprintf(label, sizeof(label), "Road %u (%s)", road.id, user_flag);
        ImGui::Selectable(label);
    }
    uiint.RegisterWidget({"table", "Roads", "roads[]", {"index"}});
    uiint.EndPanel();
    ImGui::End();
}

} // namespace RC_UI::Panels::RoadIndex
