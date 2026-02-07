// FILE: rc_panel_river_index.cpp
// PURPOSE: Implementation of river index panel (currently a stub).

#include "ui/panels/rc_panel_river_index.h"
#include "ui/introspection/UiIntrospection.h"
#include <imgui.h>

namespace RC_UI::Panels::RiverIndex {

void Draw(float /*dt*/)
{
    const bool open = ImGui::Begin("River Index", nullptr, ImGuiWindowFlags_NoCollapse);

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "River Index",
            "River Index",
            "index",
            "Bottom",
            "visualizer/src/ui/panels/rc_panel_river_index.cpp",
            {"rivers", "index"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("No rivers to display");
    uiint.RegisterWidget({"table", "Rivers", "rivers[]", {"index"}});
    uiint.EndPanel();
    ImGui::End();
}

} // namespace RC_UI::Panels::RiverIndex
