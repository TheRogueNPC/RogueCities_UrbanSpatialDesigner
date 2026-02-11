// FILE: visualizer/src/ui/panels/rc_panel_system_map.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Live system map visualization with reactive Y2K overlays.
#include "ui/panels/rc_panel_system_map.h"

#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <imgui.h>
#include <algorithm>

namespace RC_UI::Panels::SystemMap {

void Draw(float dt)
{
    const bool open = Components::BeginTokenPanel("System Map", UITokens::MagentaHighlight);

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "System Map",
            "System Map",
            "nav",
            "Left",
            "visualizer/src/ui/panels/rc_panel_system_map.cpp",
            {"map", "nav"}
        },
        open
    );

    if (!open) {
        uiint.EndPanel();
        Components::EndTokenPanel();
        return;
    }

    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 panel_pos = ImGui::GetCursorScreenPos();
    const ImVec2 panel_size = ImGui::GetContentRegionAvail();
    const ImVec2 center(panel_pos.x + panel_size.x * 0.5f, panel_pos.y + panel_size.y * 0.5f);
    const float radius = (panel_size.x < panel_size.y ? panel_size.x : panel_size.y) * 0.20f;

    static ReactiveF focus;
    focus.target = ImGui::IsWindowHovered() ? 1.0f : 0.0f;
    focus.Update(dt);

    draw_list->AddRectFilled(panel_pos,
        ImVec2(panel_pos.x + panel_size.x, panel_pos.y + panel_size.y),
        WithAlpha(UITokens::BackgroundDark, 245), UITokens::RoundingPanel);
    draw_list->AddCircleFilled(center, radius, WithAlpha(UITokens::PanelBackground, 210), 64);
    draw_list->AddCircle(center, radius + 18.0f * focus.v,
        WithAlpha(UITokens::CyanAccent, 180), 64, UITokens::BorderNormal);
    draw_list->AddCircle(center, radius + 36.0f * focus.v,
        WithAlpha(UITokens::MagentaHighlight, 145), 64, UITokens::BorderThin);
    Components::DrawScanlineBackdrop(panel_pos, ImVec2(panel_pos.x + panel_size.x, panel_pos.y + panel_size.y), static_cast<float>(ImGui::GetTime()), UITokens::CyanAccent);

    RogueCity::Core::Bounds bounds{};
    bool has_bounds = false;
    if (gs.world_constraints.isValid()) {
        bounds.min = {0.0, 0.0};
        bounds.max = {
            static_cast<double>(gs.world_constraints.width) * gs.world_constraints.cell_size,
            static_cast<double>(gs.world_constraints.height) * gs.world_constraints.cell_size
        };
        has_bounds = true;
    }

    if (!has_bounds) {
        for (const auto& road : gs.roads) {
            for (const auto& p : road.points) {
                if (!has_bounds) {
                    bounds.min = p;
                    bounds.max = p;
                    has_bounds = true;
                } else {
                    bounds.min.x = std::min(bounds.min.x, p.x);
                    bounds.min.y = std::min(bounds.min.y, p.y);
                    bounds.max.x = std::max(bounds.max.x, p.x);
                    bounds.max.y = std::max(bounds.max.y, p.y);
                }
            }
        }
    }

    if (has_bounds && bounds.width() > 1e-6 && bounds.height() > 1e-6) {
        const auto world_to_map = [&](const RogueCity::Core::Vec2& p) {
            const float u = static_cast<float>((p.x - bounds.min.x) / bounds.width());
            const float v = static_cast<float>((p.y - bounds.min.y) / bounds.height());
            return ImVec2(
                panel_pos.x + u * panel_size.x,
                panel_pos.y + v * panel_size.y);
        };

        for (const auto& district : gs.districts) {
            if (district.border.size() < 3) {
                continue;
            }
            RogueCity::Core::Vec2 centroid{};
            for (const auto& p : district.border) {
                centroid += p;
            }
            centroid /= static_cast<double>(district.border.size());
            const ImVec2 c = world_to_map(centroid);
            draw_list->AddCircleFilled(c, 2.8f, WithAlpha(UITokens::GreenHUD, 185), 12);
        }

        for (const auto& road : gs.roads) {
            if (road.points.size() < 2) {
                continue;
            }
            for (size_t i = 1; i < road.points.size(); ++i) {
                draw_list->AddLine(
                    world_to_map(road.points[i - 1]),
                    world_to_map(road.points[i]),
                    WithAlpha(UITokens::CyanAccent, 170),
                    1.1f);
            }
        }
    }

    ImGui::SetCursorScreenPos(ImVec2(panel_pos.x + 12.0f, panel_pos.y + 10.0f));
    Components::StatusChip("SYSTEM MAP", UITokens::CyanAccent, true);
    ImGui::Text("Roads: %zu  Districts: %zu  Lots: %zu", gs.roads.size(), gs.districts.size(), gs.lots.size());
    ImGui::Text("Hover for enhanced scan");

    uiint.RegisterWidget({"viewport", "System Map", "map.system", {"map"}});
    uiint.EndPanel();

    Components::EndTokenPanel();
}

} // namespace RC_UI::Panels::SystemMap
