// FILE: rc_panel_telemetry.cpp
// PURPOSE: Live analytics panel showing procedural generation metrics.

#include "ui/panels/rc_panel_telemetry.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_components.h"

#include <RogueCity/Core/Editor/GlobalState.hpp>

#include <algorithm>
#include <imgui.h>
#include <string>

namespace RC_UI::Panels::Telemetry {

void DrawContent(float dt) {
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  auto &gs = RogueCity::Core::Editor::GetGlobalState();
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

  static ReactiveF fill;
  const float target_load =
      0.35f * std::clamp(static_cast<float>(dirty_count) / 7.0f, 0.0f, 1.0f) +
      0.45f * std::clamp(frame_ms / 22.0f, 0.0f, 1.0f) +
      0.20f *
          std::clamp(static_cast<float>(gs.selection_manager.Count()) / 12.0f,
                     0.0f, 1.0f);
  fill.target = target_load;
  fill.Update(dt);

  // Runtime Pressure Section
  if (Components::DrawSectionHeader("Runtime", UITokens::CyanAccent)) {
      Components::DrawMeter("Pressure", fill.v, fill.v > 0.8f ? UITokens::ErrorRed : (fill.v > 0.5f ? UITokens::YellowWarning : UITokens::SuccessGreen));
      
      char fps_str[32];
      snprintf(fps_str, sizeof(fps_str), "%.1f (%.2f ms)", fps, frame_ms);
      Components::DrawDiagRow("FPS", fps_str, fps > 45.0f ? UITokens::SuccessGreen : UITokens::YellowWarning);
      
      char dirty_str[32];
      snprintf(dirty_str, sizeof(dirty_str), "%d / 7", dirty_count);
      Components::DrawDiagRow("Dirty Layers", dirty_str);
      
      char ent_str[64];
      snprintf(ent_str, sizeof(ent_str), "R:%zu D:%zu L:%zu B:%zu", gs.roads.size(), gs.districts.size(), gs.lots.size(), gs.buildings.size());
      Components::DrawDiagRow("Entities", ent_str);
      
      ImGui::Spacing();
      Components::StatusChip(gs.plan_approved ? "PLAN OK" : "PLAN BLOCKED", gs.plan_approved ? UITokens::SuccessGreen : UITokens::ErrorRed, true);
      ImGui::Spacing();
  }

  // Grid Quality Index Section
  if (Components::DrawSectionHeader("Grid Quality Index", UITokens::CyanAccent)) {
      auto &gq = gs.grid_quality;
      
      Components::DrawMeter("Composite", gq.composite_index, UITokens::SuccessGreen);
      Components::DrawMeter("Straightness", gq.straightness, UITokens::SuccessGreen);
      Components::DrawMeter("Orientation", gq.orientation_order, UITokens::InfoBlue);
      Components::DrawMeter("4-Way Prop", gq.four_way_proportion, UITokens::YellowWarning);
      
      ImGui::Spacing();
  }

  // Urban Hell Diagnostics Section
  if (Components::DrawSectionHeader("Urban Hell Diagnostics", UITokens::YellowWarning)) {
      auto &gq = gs.grid_quality;

      if (gq.island_count > 1) {
          Components::DrawDiagRow("Connectivity", (std::string("Disconnected (") + std::to_string(gq.island_count) + ")").c_str(), UITokens::ErrorRed);
      } else {
          Components::DrawDiagRow("Connectivity", "Unified", UITokens::SuccessGreen);
      }

      char dead_ends_str[32];
      snprintf(dead_ends_str, sizeof(dead_ends_str), "%d%%", static_cast<int>(gq.dead_end_proportion * 100));
      Components::DrawDiagRow("Dead Ends", dead_ends_str, gq.dead_end_proportion > 0.4f ? UITokens::ErrorRed : UITokens::TextPrimary);

      if (gq.micro_segment_count > 0) {
          Components::DrawDiagRow("Micro-Segments", std::to_string(gq.micro_segment_count).c_str(), UITokens::YellowWarning);
      }

      char gamma_str[32];
      snprintf(gamma_str, sizeof(gamma_str), "%.2f", gq.connectivity_index);
      Components::DrawDiagRow("Gamma Index", gamma_str);
      
      Components::DrawDiagRow("Strokes", std::to_string(gq.total_strokes).c_str());
      Components::DrawDiagRow("Intersections", std::to_string(gq.total_intersections).c_str());
  }

  uiint.RegisterWidget(
      {"property_editor", "Flow Rate", "metrics.flow_rate", {"metrics"}});
}

void Draw(float dt) {
  const bool open =
      Components::BeginTokenPanel("Analytics", UITokens::CyanAccent);

  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{
          "Analytics",
          "Analytics",
          "analytics",
          "ToolDeck",
          "visualizer/src/ui/panels/rc_panel_telemetry.cpp",
          {"analytics", "metrics"}},
      open);

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
