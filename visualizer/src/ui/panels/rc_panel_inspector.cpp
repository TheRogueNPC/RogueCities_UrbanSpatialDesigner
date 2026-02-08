// FILE: visualizer/src/ui/panels/rc_panel_inspector.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Stub inspector with reactive selection focus.
#include "ui/panels/rc_panel_inspector.h"

#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_theme.h"
#include "ui/introspection/UiIntrospection.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"

#include <imgui.h>

namespace RC_UI::Panels::Inspector {

void Draw(float dt)
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorPanel);
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse);

    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();

    static ReactiveF focus;
    constexpr float kBaseBorderThickness = 2.0f;
    const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    focus.target = hovered ? 1.0f : 0.0f;
    focus.Update(dt);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 40.0f);
    const ImVec2 end(pos.x + size.x, pos.y + size.y);
    draw_list->AddRectFilled(pos, end, ImGui::ColorConvertFloat4ToU32(Palette::Nebula), 12.0f);
    draw_list->AddRect(pos, end, ImGui::ColorConvertFloat4ToU32(ZoneAxiom.accent), 12.0f, 0,
        kBaseBorderThickness + kBaseBorderThickness * focus.v);
    ImGui::Dummy(size);

    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Inspector",
            "Inspector",
            "inspector",
            "Right",
            "visualizer/src/ui/panels/rc_panel_inspector.cpp",
            {"detail", "selection"}
        },
        true
    );

    if (gs.selection.selected_building) {
        auto& building = *gs.selection.selected_building;
        ImGui::Text("Selection: Building");
        ImGui::Separator();
        ImGui::InputScalar("Building ID", ImGuiDataType_U32, &building.id);
        ImGui::InputScalar("Lot ID", ImGuiDataType_U32, &building.lot_id);
        ImGui::InputScalar("District ID", ImGuiDataType_U32, &building.district_id);
        ImGui::InputFloat2("Position", &building.position.x);
        ImGui::Checkbox("User Placed", &building.is_user_placed);
        ImGui::Checkbox("Locked Type", &building.locked_type);
        uiint.RegisterWidget({"property_editor", "Building", "buildings[]", {"detail"}});
    } else if (gs.selection.selected_lot) {
        auto& lot = *gs.selection.selected_lot;
        ImGui::Text("Selection: Lot");
        ImGui::Separator();
        ImGui::InputScalar("Lot ID", ImGuiDataType_U32, &lot.id);
        ImGui::InputScalar("District ID", ImGuiDataType_U32, &lot.district_id);
        ImGui::InputFloat2("Centroid", &lot.centroid.x);
        ImGui::Checkbox("User Placed", &lot.is_user_placed);
        ImGui::Checkbox("Locked Type", &lot.locked_type);
        ImGui::SeparatorText("AESP Scores");
        ImGui::Text("Access: %.2f", lot.access);
        ImGui::Text("Exposure: %.2f", lot.exposure);
        ImGui::Text("Service: %.2f", lot.serviceability);
        ImGui::Text("Privacy: %.2f", lot.privacy);
        uiint.RegisterWidget({"property_editor", "Lot", "lots[]", {"detail"}});
    } else if (gs.selection.selected_district) {
        auto& district = *gs.selection.selected_district;
        ImGui::Text("Selection: District");
        ImGui::Separator();
        ImGui::InputScalar("District ID", ImGuiDataType_U32, &district.id);
        ImGui::InputInt("Primary Axiom", &district.primary_axiom_id);
        ImGui::InputInt("Secondary Axiom", &district.secondary_axiom_id);
        ImGui::Text("Border Points: %zu", district.border.size());
        uiint.RegisterWidget({"property_editor", "District", "districts[]", {"detail"}});
    } else if (gs.selection.selected_road) {
        auto& road = *gs.selection.selected_road;
        ImGui::Text("Selection: Road");
        ImGui::Separator();
        ImGui::InputScalar("Road ID", ImGuiDataType_U32, &road.id);
        ImGui::Checkbox("User Created", &road.is_user_created);
        ImGui::Text("Point Count: %zu", road.points.size());
        uiint.RegisterWidget({"property_editor", "Road", "roads[]", {"detail"}});
    } else {
        ImGui::TextColored(ColorAccentA, "No selection");
    }

    uiint.EndPanel();

    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Panels::Inspector
