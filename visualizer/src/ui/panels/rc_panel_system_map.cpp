// FILE: visualizer/src/ui/panels/rc_panel_system_map.cpp (RogueCities_UrbanSpatialDesigner)
// PURPOSE: Live system map visualization with reactive Y2K overlays and query/toggle plumbing.
#include "ui/panels/rc_panel_system_map.h"
#include "ui/panels/rc_system_map_query.h"

#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>
#include <RogueCity/Core/Editor/SelectionSync.hpp>

#include <imgui.h>
#include <algorithm>
#include <cstdio>

namespace RC_UI::Panels::SystemMap {
namespace {

void ResetHoverState(RogueCity::Core::Editor::GlobalState& gs) {
    auto& systems_map = gs.systems_map;
    systems_map.hovered_kind = RogueCity::Core::Editor::VpEntityKind::Unknown;
    systems_map.hovered_id = 0;
    systems_map.hovered_distance = 0.0f;
    systems_map.hovered_frame = gs.frame_counter;
    systems_map.hovered_label.clear();
}

} // namespace

void DrawContent(float dt)
{
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    auto& systems_map = gs.systems_map;

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
    const bool has_bounds = BuildSystemsMapBounds(gs, bounds);

    SystemsMapQueryHit query_hit{};
    RogueCity::Core::Vec2 cursor_world{};
    bool cursor_in_bounds = false;

    if (has_bounds) {
        const auto world_to_map = [&](const RogueCity::Core::Vec2& p) {
            const float u = static_cast<float>((p.x - bounds.min.x) / bounds.width());
            const float v = static_cast<float>((p.y - bounds.min.y) / bounds.height());
            return ImVec2(
                panel_pos.x + u * panel_size.x,
                panel_pos.y + v * panel_size.y);
        };

        const auto map_to_world = [&](const ImVec2& map_pos) {
            const double u = std::clamp(static_cast<double>(map_pos.x - panel_pos.x) / static_cast<double>(panel_size.x), 0.0, 1.0);
            const double v = std::clamp(static_cast<double>(map_pos.y - panel_pos.y) / static_cast<double>(panel_size.y), 0.0, 1.0);
            return RogueCity::Core::Vec2(
                bounds.min.x + u * bounds.width(),
                bounds.min.y + v * bounds.height());
        };

        if (systems_map.show_districts) {
            for (const auto& district : gs.districts) {
                if (district.border.size() < 3) {
                    continue;
                }
                std::vector<ImVec2> screen_points;
                screen_points.reserve(district.border.size());
                for (const auto& p : district.border) {
                    screen_points.push_back(world_to_map(p));
                }
                draw_list->AddPolyline(
                    screen_points.data(),
                    static_cast<int>(screen_points.size()),
                    WithAlpha(UITokens::GreenHUD, 150),
                    true,
                    1.0f);
            }
        }

        if (systems_map.show_lots) {
            for (const auto& lot : gs.lots) {
                if (lot.boundary.size() < 3) {
                    continue;
                }
                std::vector<ImVec2> screen_points;
                screen_points.reserve(lot.boundary.size());
                for (const auto& p : lot.boundary) {
                    screen_points.push_back(world_to_map(p));
                }
                draw_list->AddPolyline(
                    screen_points.data(),
                    static_cast<int>(screen_points.size()),
                    WithAlpha(UITokens::AmberGlow, 120),
                    true,
                    0.9f);
            }
        }

        if (systems_map.show_roads) {
            for (const auto& road : gs.roads) {
                if (road.points.size() < 2) {
                    continue;
                }
                for (size_t i = 1; i < road.points.size(); ++i) {
                    draw_list->AddLine(
                        world_to_map(road.points[i - 1]),
                        world_to_map(road.points[i]),
                        WithAlpha(UITokens::CyanAccent, 180),
                        1.2f);
                }
            }
        }

        if (systems_map.show_water) {
            for (const auto& water : gs.waterbodies) {
                if (water.boundary.size() < 2) {
                    continue;
                }
                for (size_t i = 1; i < water.boundary.size(); ++i) {
                    draw_list->AddLine(
                        world_to_map(water.boundary[i - 1]),
                        world_to_map(water.boundary[i]),
                        WithAlpha(UITokens::InfoBlue, 170),
                        1.1f);
                }
            }
        }

        if (systems_map.show_buildings) {
            for (const auto& building : gs.buildings) {
                const ImVec2 p = world_to_map(building.position);
                draw_list->AddCircleFilled(p, 2.0f, WithAlpha(UITokens::TextPrimary, 180), 8);
            }
        }

        if (systems_map.show_world_constraints && gs.world_constraints.isValid()) {
            draw_list->AddRect(
                panel_pos,
                ImVec2(panel_pos.x + panel_size.x, panel_pos.y + panel_size.y),
                WithAlpha(UITokens::MagentaHighlight, 120),
                0.0f,
                0,
                1.0f);
        }

        const ImVec2 mouse = ImGui::GetMousePos();
        const bool mouse_in_map =
            mouse.x >= panel_pos.x && mouse.x <= panel_pos.x + panel_size.x &&
            mouse.y >= panel_pos.y && mouse.y <= panel_pos.y + panel_size.y;

        if (mouse_in_map) {
            cursor_world = map_to_world(mouse);
            cursor_in_bounds = true;

            if (systems_map.enable_hover_query) {
                const float pick_radius = static_cast<float>(std::max(bounds.width(), bounds.height()) * 0.018);
                query_hit = QuerySystemsMapEntity(gs, systems_map, cursor_world, pick_radius);
                if (query_hit.valid) {
                    systems_map.hovered_kind = query_hit.kind;
                    systems_map.hovered_id = query_hit.id;
                    systems_map.hovered_distance = query_hit.distance;
                    systems_map.hovered_frame = gs.frame_counter;
                    char label_buf[64]{};
                    std::snprintf(label_buf, sizeof(label_buf), "%s #%u", query_hit.label, query_hit.id);
                    systems_map.hovered_label = label_buf;

                    if (systems_map.enable_click_select && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        gs.selection_manager.Select(query_hit.kind, query_hit.id);
                        RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
                    }
                } else {
                    ResetHoverState(gs);
                }
            } else {
                ResetHoverState(gs);
            }
        } else {
            ResetHoverState(gs);
        }
    } else {
        ResetHoverState(gs);
    }

    ImGui::SetCursorScreenPos(ImVec2(panel_pos.x + 12.0f, panel_pos.y + 10.0f));
    Components::StatusChip("SYSTEM MAP", UITokens::CyanAccent, true);
    ImGui::Text("Roads: %zu  Districts: %zu  Lots: %zu", gs.roads.size(), gs.districts.size(), gs.lots.size());

    ImGui::Checkbox("Roads", &systems_map.show_roads);
    ImGui::SameLine();
    ImGui::Checkbox("Districts", &systems_map.show_districts);
    ImGui::SameLine();
    ImGui::Checkbox("Lots", &systems_map.show_lots);
    ImGui::Checkbox("Buildings", &systems_map.show_buildings);
    ImGui::SameLine();
    ImGui::Checkbox("Water", &systems_map.show_water);
    ImGui::SameLine();
    ImGui::Checkbox("Labels", &systems_map.show_labels);
    ImGui::Checkbox("World Constraints", &systems_map.show_world_constraints);

    ImGui::Checkbox("Hover Query", &systems_map.enable_hover_query);
    ImGui::SameLine();
    ImGui::Checkbox("Click Select", &systems_map.enable_click_select);

    if (query_hit.valid) {
        ImGui::Text("Query: %s #%u (d=%.2f)", SystemsMapKindLabel(query_hit.kind), query_hit.id, query_hit.distance);
    } else {
        ImGui::Text("Query: none");
    }

    if (cursor_in_bounds) {
        ImGui::Text("Cursor World: %.1f, %.1f", cursor_world.x, cursor_world.y);
    }

    uiint.RegisterWidget({"viewport", "System Map", "map.system", {"map"}});
}

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

    DrawContent(dt);

    uiint.EndPanel();
    Components::EndTokenPanel();
}

} // namespace RC_UI::Panels::SystemMap
