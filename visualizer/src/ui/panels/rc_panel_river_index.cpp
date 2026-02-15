// FILE: rc_panel_river_index.cpp
// PURPOSE: Live river index panel backed by GlobalState water data.

#include "ui/panels/rc_panel_river_index.h"
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"

#include <RogueCity/Core/Data/CityTypes.hpp>
#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <imgui.h>

namespace RC_UI::Panels::RiverIndex {

namespace {
const char* WaterTypeLabel(RogueCity::Core::WaterType type) {
    using RogueCity::Core::WaterType;
    switch (type) {
        case WaterType::Lake: return "Lake";
        case WaterType::River: return "River";
        case WaterType::Ocean: return "Ocean";
        case WaterType::Pond: return "Pond";
    }
    return "Unknown";
}

void DrawRiverIndexBody(RogueCity::UIInt::UiIntrospector& uiint) {
    const auto& gs = RogueCity::Core::Editor::GetGlobalState();

    size_t river_count = 0;
    for (const auto& water : gs.waterbodies) {
        if (water.type == RogueCity::Core::WaterType::River) {
            ++river_count;
        }
    }

    Components::StatusChip(river_count > 0 ? "ACTIVE" : "EMPTY",
        river_count > 0 ? UITokens::SuccessGreen : UITokens::YellowWarning,
        true);
    ImGui::SameLine();
    ImGui::Text("Rivers: %zu / Bodies: %zu", river_count, gs.waterbodies.size());
    ImGui::Separator();

    if (river_count == 0) {
        ImGui::TextDisabled("No rivers detected in the current scene.");
        ImGui::TextDisabled("Tip: Water Editing Control -> set type to River -> Add Sample Body.");
    } else if (ImGui::BeginTable("RiverTable", 6,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Points");
        ImGui::TableSetupColumn("Depth");
        ImGui::TableSetupColumn("Shore");
        ImGui::TableSetupColumn("Origin");
        ImGui::TableHeadersRow();

        for (const auto& water : gs.waterbodies) {
            if (water.type != RogueCity::Core::WaterType::River) {
                continue;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%u", water.id);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(WaterTypeLabel(water.type));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", water.boundary.size());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.1fm", water.depth);
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(water.generate_shore ? "Yes" : "No");
            ImGui::TableSetColumnIndex(5);
            ImGui::TextUnformatted(water.is_user_placed ? "User" : "Generated");
        }
        ImGui::EndTable();
    }

    uiint.RegisterWidget({"table", "Rivers", "rivers[]", {"index", "water"}});
}
} // namespace

void Draw(float dt)
{
    (void)dt;
    const bool open = Components::BeginTokenPanel("River Index", UITokens::InfoBlue);

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
        Components::EndTokenPanel();
        return;
    }

    DrawRiverIndexBody(uiint);
    uiint.EndPanel();
    Components::EndTokenPanel();
}

void DrawContent(float dt) {
    (void)dt;
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    DrawRiverIndexBody(uiint);
}

} // namespace RC_UI::Panels::RiverIndex
