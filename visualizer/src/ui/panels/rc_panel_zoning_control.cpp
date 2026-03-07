// FILE: rc_panel_zoning_control.cpp
// PURPOSE: Implementation of zoning control panel
// Y2K GEOMETRY: Pulse animations, glow affordances, state-reactive colors

#include "ui/panels/rc_panel_zoning_control.h"
#include "ui/api/rc_imgui_api.h"
#include "RogueCity/Core/Editor/EditorState.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_tokens.h"
#include <cmath>
#include <imgui.h>

#include "RogueCity/App/Editor/CommandHistory.hpp"
#include "ui/panels/IPanelDrawer.h"
#include "ui/tools/rc_tool_dispatcher.h"
#include <RogueCity/Visualizer/LucideIcons.hpp>

namespace RC_UI::Panels::ZoningControl {

namespace {

const char *GenerationModeLabel(RogueCity::Core::GenerationMode mode) {
  using RogueCity::Core::GenerationMode;
  switch (mode) {
  case GenerationMode::Standard:
    return "Standard";
  case GenerationMode::HillTown:
    return "HillTown";
  case GenerationMode::ConservationOnly:
    return "ConservationOnly";
  case GenerationMode::BrownfieldCore:
    return "BrownfieldCore";
  case GenerationMode::CompromisePlan:
    return "CompromisePlan";
  case GenerationMode::Patchwork:
    return "Patchwork";
  default:
    return "Unknown";
  }
}

} // namespace

PanelState &GetPanelState() {
  static PanelState state;
  return state;
}

// Content-only draw (for Master Panel drawer)
void DrawContent(float dt) {
  using RogueCity::Core::Editor::GetGlobalState;

  auto &gs = GetGlobalState();
  auto &state = GetPanelState();
  auto &bridge = state.bridge;

  auto &hfsm = RogueCity::Core::Editor::GetEditorHFSM();
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  RC_UI::Panels::DrawContext ctx{
      gs, hfsm, uiint, &RogueCity::App::GetEditorCommandHistory(), dt, false};

  if (Components::DrawSectionHeader(
          "Actions", UITokens::CyanAccent, true,
          RC::SvgTextureCache::Get().Load(LC::Plus, 14.f))) {
    API::Indent();
    API::Spacing();
    RC_UI::Components::DrawToolActionGrid(RC_UI::ToolLibrary::District, ctx);
    API::Unindent();
    API::Spacing();
  }

  // === PARAMETER SLIDERS ===
  bool changed = false;

  if (Components::DrawSectionHeader("Lot Sizing", UITokens::CyanAccent)) {
    API::Indent();
    changed |=
        API::SliderInt("Min Lot Width", &state.config.min_lot_width, 5, 30);
    changed |=
        API::SliderInt("Max Lot Width", &state.config.max_lot_width, 30, 80);
    changed |=
        API::SliderInt("Min Lot Depth", &state.config.min_lot_depth, 10, 40);
    changed |=
        API::SliderInt("Max Lot Depth", &state.config.max_lot_depth, 40, 100);
    API::Unindent();
    API::Spacing();
  }

  if (Components::DrawSectionHeader("Building Constraints",
                                    UITokens::CyanAccent)) {
    API::Indent();
    changed |=
        API::SliderFloat("Min Coverage", &state.config.min_building_coverage,
                           0.2f, 0.6f, "%.1f%%");
    changed |=
        API::SliderFloat("Max Coverage", &state.config.max_building_coverage,
                           0.6f, 0.95f, "%.1f%%");
    API::Unindent();
    API::Spacing();
  }

  if (Components::DrawSectionHeader("Budget & Population",
                                    UITokens::AmberGlow)) {
    API::Indent();
    changed |=
        API::SliderFloat("Budget per Capita", &state.config.budget_per_capita,
                           50000.0f, 200000.0f, "$%.0f");
    changed |= API::SliderInt("Target Population",
                                &state.config.target_population, 10000, 100000);
    API::Unindent();
    API::Spacing();
  }

  if (Components::DrawSectionHeader("Performance", UITokens::GreenHUD)) {
    API::Indent();
    changed |= API::Checkbox("Auto Threading", &state.config.auto_threading);
    if (state.config.auto_threading) {
      changed |= API::SliderInt("Thread Threshold",
                                  &state.config.threading_threshold, 50, 500);
    }
    API::Unindent();
    API::Spacing();
  }

  state.parameters_changed = changed;

  // Update glow intensity (Y2K affordance)
  if (state.parameters_changed) {
    state.glow_intensity = std::min(state.glow_intensity + dt * 2.0f, 1.0f);
  } else {
    state.glow_intensity = std::max(state.glow_intensity - dt * 0.5f, 0.0f);
  }

  // === GENERATE BUTTON (with pulse animation) ===
  API::Separator();

  if (state.parameters_changed) {
    state.pulse_phase += dt * 3.0f;
    const float pulse = 0.5f + 0.5f * std::sin(state.pulse_phase);
    const ImU32 pulse_color =
        LerpColor(UITokens::InfoBlue, UITokens::CyanAccent, pulse);
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImGui::ColorConvertU32ToFloat4(pulse_color));
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImGui::ColorConvertU32ToFloat4(UITokens::InfoBlue));
  }

  if (API::Button("Generate Zones & Buildings", ImVec2(-1, 40))) {
    state.is_generating = true;

    std::string pipeline_error;
    bool pipeline_ok = true;
    if (gs.active_city_spec.has_value()) {
      pipeline_ok = bridge.GenerateFromCitySpec(
          *gs.active_city_spec, state.config, gs, &pipeline_error);
    } else {
      bridge.Generate(state.config, gs);
    }

    state.is_generating = false;
    state.parameters_changed = !pipeline_ok;
    state.pulse_phase = 0.0f;
    state.last_error = pipeline_error;
  }

  ImGui::PopStyleColor();

  // === STATISTICS DISPLAY ===
  API::Separator();
  ImGui::Text("Last Generation:");

  auto stats = bridge.GetLastStats();

  ImGui::BulletText("Lots Created: %d", stats.lots_created);
  ImGui::BulletText("Buildings Placed: %d", stats.buildings_placed);
  ImGui::BulletText("Budget Allocated: $%.0f", stats.total_budget_allocated);
  ImGui::BulletText("Projected Population: %d", stats.projected_population);
  ImGui::BulletText("Generation Time: %.2f ms", stats.generation_time_ms);
  if (gs.active_city_spec.has_value()) {
    ImGui::BulletText("Pipeline: CitySpec -> Zoning");
  } else {
    ImGui::BulletText("Pipeline: Zoning only");
  }
  if (!state.last_error.empty()) {
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::ErrorRed), "%s",
                       state.last_error.c_str());
  }

  if (gs.world_constraints.isValid() &&
      Components::DrawSectionHeader("Site Diagnostics",
                                    UITokens::YellowWarning)) {
    API::Indent();
    ImGui::BulletText("Mode: %s", GenerationModeLabel(gs.site_profile.mode));
    ImGui::BulletText("Buildable Area: %.1f%%",
                      gs.site_profile.buildable_fraction * 100.0f);
    ImGui::BulletText("Avg Buildable Slope: %.1f deg",
                      gs.site_profile.average_buildable_slope);
    ImGui::BulletText("Fragmentation: %.2f",
                      gs.site_profile.buildable_fragmentation);
    ImGui::BulletText("Policy Friction: %.2f", gs.site_profile.policy_friction);
    const ImVec4 status_color = ImGui::ColorConvertU32ToFloat4(
        gs.plan_approved ? UITokens::SuccessGreen : UITokens::ErrorRed);
    ImGui::TextColored(status_color, "Plan %s (%zu violations)",
                       gs.plan_approved ? "APPROVED" : "NEEDS REVIEW",
                       gs.plan_violations.size());
    const int max_preview = 5;
    for (int i = 0;
         i < static_cast<int>(gs.plan_violations.size()) && i < max_preview;
         ++i) {
      const auto &violation = gs.plan_violations[static_cast<size_t>(i)];
      ImGui::BulletText("[%.2f] %s", violation.severity,
                        violation.message.empty() ? "Validation issue"
                                                  : violation.message.c_str());
    }
    API::Unindent();
    API::Spacing();
  }
}

// Legacy window-owning draw (for backward compatibility)
void Draw(float dt) {
  using RogueCity::Core::Editor::GetEditorHFSM;

  auto &hfsm = GetEditorHFSM();

  // Panel visibility based on HFSM state (Cockpit Doctrine)
  auto editor_state = hfsm.state();
  bool is_visible =
      (editor_state ==
           RogueCity::Core::Editor::EditorState::Editing_Districts ||
       editor_state == RogueCity::Core::Editor::EditorState::Editing_Lots ||
       editor_state == RogueCity::Core::Editor::EditorState::Editing_Buildings);

  if (!is_visible)
    return;

  // State-reactive color (Y2K geometry)
  ImU32 panel_tint = UITokens::TextPrimary;
  if (editor_state == RogueCity::Core::Editor::EditorState::Editing_Districts) {
    panel_tint = UITokens::InfoBlue;
  } else if (editor_state ==
             RogueCity::Core::Editor::EditorState::Editing_Lots) {
    panel_tint = UITokens::SuccessGreen;
  } else if (editor_state ==
             RogueCity::Core::Editor::EditorState::Editing_Buildings) {
    panel_tint = UITokens::AmberGlow;
  }

  const ImU32 window_bg =
      LerpColor(UITokens::BackgroundDark, panel_tint, 0.15f);
  const bool open = Components::BeginTokenPanel(
      "Zoning Control", panel_tint, nullptr, ImGuiWindowFlags_NoCollapse,
      WithAlpha(window_bg, 242u));

  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{
          "Zoning Control",
          "Zoning Control",
          "toolbox",
          "Right",
          "visualizer/src/ui/panels/rc_panel_zoning_control.cpp",
          {"zoning", "generator", "control"}},
      open);

  if (!open) {
    uiint.EndPanel();
    Components::EndTokenPanel();
    return;
  }

  // Delegate to content-only method
  DrawContent(dt);

  uiint.RegisterWidget({"button", "Generate", "generate_button", {"action"}});
  uiint.EndPanel();
  Components::EndTokenPanel();
}

} // namespace RC_UI::Panels::ZoningControl
