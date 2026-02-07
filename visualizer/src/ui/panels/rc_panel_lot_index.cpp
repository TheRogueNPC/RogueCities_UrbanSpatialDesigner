// FILE: rc_panel_lot_index.cpp
// PURPOSE: Implementation of lot index panel.

#include "ui/panels/rc_panel_lot_index.h"
#include "ui/introspection/UiIntrospection.h"
#include <imgui.h>

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"

namespace RC_UI::Panels::LotIndex {

void Draw(float /*dt*/)
{
    using RogueCity::Core::Editor::GetGlobalState;
    auto& gs = GetGlobalState();
    const bool open = ImGui::Begin("Lot Index", nullptr, ImGuiWindowFlags_NoCollapse);

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Lot Index",
            "Lot Index",
            "index",
            "Bottom",
            "visualizer/src/ui/panels/rc_panel_lot_index.cpp",
            {"lots", "index"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        ImGui::End();
        return;
    }

    ImGui::Text("Lots: %llu", static_cast<unsigned long long>(gs.lots.size()));
    ImGui::Separator();
    for (size_t i = 0; i < gs.lots.size(); ++i) {
        const auto& lot = gs.lots[i];
        char label[128];
        snprintf(label, sizeof(label), "Lot %u (District %u)", lot.id, lot.district_id);
        ImGui::Selectable(label);
    }
    uiint.RegisterWidget({"table", "Lots", "lots[]", {"index"}});
    uiint.EndPanel();
    ImGui::End();
}

} // namespace RC_UI::Panels::LotIndex
