// FILE: rc_panel_telemetry.cpp
// PURPOSE: Live analytics panel showing procedural generation metrics.

#include "ui/panels/rc_panel_telemetry.h"
#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_components.h"
#include "ui/introspection/UiIntrospection.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <imgui.h>
#include <algorithm>
#include <string>

namespace RC_UI::Panels::Telemetry {

void DrawContent(float dt)
{
    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    auto& gs = RogueCity::Core::Editor::GetGlobalState();
    const float fps = ImGui::GetIO().Framerate;
    const float frame_ms = (fps > 0.0f) ? (1000.0f / fps) : 0.0f;
    const int dirty_count = [&gs]() {
        using RogueCity::Core::Editor::DirtyLayer;
        int count = 0;
        for (int i = 0; i < static_cast<int>(DirtyLayer::Count); ++i) {
            if (gs.dirty_layers.IsDirty(static_cast<DirtyLayer>(i))) {
                ++count;
            }
        }
        return count;
    }();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 120.0f);

    static ReactiveF fill;
    const float target_load =
        0.35f * std::clamp(static_cast<float>(dirty_count) / 7.0f, 0.0f, 1.0f) +
        0.45f * std::clamp(frame_ms / 22.0f, 0.0f, 1.0f) +
        0.20f * std::clamp(static_cast<float>(gs.selection_manager.Count()) / 12.0f, 0.0f, 1.0f);
    fill.target = target_load;
    fill.Update(dt);

    Components::DrawScanlineBackdrop(start, ImVec2(start.x + size.x, start.y + size.y), static_cast<float>(ImGui::GetTime()), UITokens::GreenHUD);
    const ImVec2 bar_end(start.x + size.x, start.y + size.y);
    draw_list->AddRectFilled(start, bar_end, WithAlpha(UITokens::BackgroundDark, 235), UITokens::RoundingPanel);
    draw_list->AddRect(start, bar_end, WithAlpha(UITokens::YellowWarning, 180), UITokens::RoundingPanel, 0, UITokens::BorderNormal);

    const float fill_height = size.y * (0.2f + 0.7f * fill.v);
    const ImVec2 fill_start(start.x + 8.0f, bar_end.y - fill_height - 8.0f);
    const ImVec2 fill_end(bar_end.x - 8.0f, bar_end.y - 8.0f);
    draw_list->AddRectFilled(fill_start, fill_end, WithAlpha(UITokens::CyanAccent, 180), UITokens::RoundingButton);

    ImGui::Dummy(size);
    ImGui::Text("Runtime Pressure");
    ImGui::ProgressBar(fill.v, ImVec2(-1.0f, 0.0f), (std::to_string(static_cast<int>(fill.v * 100.0f)) + "%").c_str());
    ImGui::Text("FPS: %.1f  (%.2f ms)", fps, frame_ms);
    ImGui::Text("Dirty Layers: %d / 7", dirty_count);
    ImGui::Text("Entities R:%zu D:%zu L:%zu B:%zu", gs.roads.size(), gs.districts.size(), gs.lots.size(), gs.buildings.size());
    Components::StatusChip(gs.plan_approved ? "PLAN OK" : "PLAN BLOCKED",
        gs.plan_approved ? UITokens::SuccessGreen : UITokens::ErrorRed,
        true);

    ImGui::Separator();
    ImGui::TextColored(UITokens::CyanAccent, "Grid Quality Index");
    
    auto& gq = gs.grid_quality;
    const float progress_w = ImGui::GetContentRegionAvail().x * 0.4f;
    
    ImGui::Text("Composite:"); ImGui::SameLine(progress_w);
    ImGui::ProgressBar(gq.composite_index, ImVec2(-1, 0), (std::to_string(static_cast<int>(gq.composite_index * 100)) + "%").c_str());
    
    ImGui::Text("Straightness:"); ImGui::SameLine(progress_w);
    ImGui::ProgressBar(gq.straightness, ImVec2(-1, 0));
    
    ImGui::Text("Orientation:"); ImGui::SameLine(progress_w);
    ImGui::ProgressBar(gq.orientation_order, ImVec2(-1, 0));
    
    ImGui::Text("4-Way Prop:"); ImGui::SameLine(progress_w);
    ImGui::ProgressBar(gq.four_way_proportion, ImVec2(-1, 0));

    ImGui::Text("Strokes: %u", gq.total_strokes);
    ImGui::SameLine();
    ImGui::Text("Inters: %u", gq.total_intersections);

    ImGui::Separator();
    ImGui::TextColored(UITokens::YellowWarning, "Urban Hell Diagnostics");
    
    if (gq.island_count > 1) {
        ImGui::TextColored(UITokens::ErrorRed, "Disconnected Islands: %u", gq.island_count);
    } else {
        ImGui::Text("Network Connectivity: Unified");
    }

    ImGui::Text("Dead Ends: %d%%", static_cast<int>(gq.dead_end_proportion * 100));
    if (gq.dead_end_proportion > 0.4f) {
        ImGui::SameLine(); ImGui::TextColored(UITokens::ErrorRed, "(HIGH SPRAWL)");
    }

    if (gq.micro_segment_count > 0) {
        ImGui::TextColored(UITokens::YellowWarning, "Micro-Segments (<1m): %u", gq.micro_segment_count);
    }
    
    ImGui::Text("Gamma Index: %.2f", gq.connectivity_index);

    uiint.RegisterWidget({"property_editor", "Flow Rate", "metrics.flow_rate", {"metrics"}});

}

void Draw(float dt)
{
    const bool open = Components::BeginTokenPanel("Analytics", UITokens::CyanAccent);

    auto& uiint = RogueCity::UIInt::UiIntrospector::Instance();
    uiint.BeginPanel(
        RogueCity::UIInt::PanelMeta{
            "Analytics",
            "Analytics",
            "analytics",
            "ToolDeck",
            "visualizer/src/ui/panels/rc_panel_telemetry.cpp",
            {"analytics", "metrics"}
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

} // namespace RC_UI::Panels::Telemetry
