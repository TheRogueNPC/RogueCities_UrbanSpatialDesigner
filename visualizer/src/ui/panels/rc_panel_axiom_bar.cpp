// FILE: visualizer/src/ui/panels/rc_panel_axiom_bar.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Stub LCARS axiom ribbon with reactive chip.
#include "ui/panels/rc_panel_axiom_bar.h"

#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_root.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>

namespace RC_UI::Panels::AxiomBar {

void Draw(float dt)
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorPanel);
    ImGui::Begin("Axiom Bar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 bar_end(start.x + avail.x, start.y + 46.0f);
    draw_list->AddRectFilled(start, bar_end, ImGui::ColorConvertFloat4ToU32(ZoneAxiom.base), 22.0f);

    ImGui::SetCursorScreenPos(ImVec2(start.x + 18.0f, start.y + 8.0f));
    static ReactiveF chip_hover;
    constexpr float kChipBaseWidth = 120.0f;
    constexpr float kChipHoverExpansion = 40.0f;
    const ImVec2 chip_box(160.0f, 28.0f);
    ImGui::InvisibleButton("AxiomChip", chip_box);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        RC_UI::ToggleAxiomLibrary();
    }
    chip_hover.target = ImGui::IsItemHovered() ? 1.0f : 0.0f;
    chip_hover.Update(dt);

    const ImVec2 chip_min = ImGui::GetItemRectMin();
    const float chip_width = kChipBaseWidth + kChipHoverExpansion * chip_hover.v;
    const ImVec2 chip_max(chip_min.x + chip_width, chip_min.y + chip_box.y);
    draw_list->AddRectFilled(chip_min, chip_max, ImGui::ColorConvertFloat4ToU32(ZoneAxiom.accent), 14.0f);
    draw_list->AddText(ImVec2(chip_min.x + 12.0f, chip_min.y + 6.0f),
        ImGui::ColorConvertFloat4ToU32(ColorBG), "Axiom Deck");

    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace RC_UI::Panels::AxiomBar
