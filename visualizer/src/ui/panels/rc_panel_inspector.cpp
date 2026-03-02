// FILE: visualizer/src/ui/panels/rc_panel_inspector.cpp
// (RogueCities_UrbanSpatialDesigner) PURPOSE: Inspector with reactive selection
// focus.
#include "ui/panels/rc_panel_inspector.h"
#include "ui/panels/rc_panel_road_editor.h"

#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/panels/rc_property_editor.h"
#include "ui/rc_ui_anim.h"
#include "ui/rc_ui_components.h"
#include "ui/rc_ui_theme.h"

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

namespace RC_UI::Panels::Inspector {

namespace {

void DrawActiveToolControls(RogueCity::Core::Editor::GlobalState& gs, float dt) {
    using namespace RogueCity::Core::Editor;
    switch (gs.tool_runtime.active_domain) {
    case ToolDomain::Road:
    case ToolDomain::Paths:
        ImGui::SeparatorText("Road Editor");
        RC_UI::Panels::RoadEditor::DrawContent(dt);
        ImGui::Checkbox("Spline Editing", &gs.spline_editor.enabled);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderInt("Samples##SplineRoad", &gs.spline_editor.samples_per_segment, 2, 24);
        break;
    case ToolDomain::Water:
    case ToolDomain::Flow:
        ImGui::Checkbox("Spline Editing", &gs.spline_editor.enabled);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderInt("Samples##SplineWater", &gs.spline_editor.samples_per_segment, 2, 24);
        break;
    case ToolDomain::District:
    case ToolDomain::Zone:
        ImGui::Checkbox("Boundary Editor", &gs.district_boundary_editor.enabled);
        ImGui::SameLine();
        ImGui::Checkbox("Snap", &gs.district_boundary_editor.snap_to_grid);
        if (gs.district_boundary_editor.snap_to_grid) {
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat("Snap Size", &gs.district_boundary_editor.snap_size, 0.5f, 0.5f, 200.0f, "%.1f");
        }
        break;
    default:
        break;
    }
}

} // namespace

void DrawContent(float dt) {
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  auto &gs = RogueCity::Core::Editor::GetGlobalState();

  // Push Mockup Y2K Styling for the Inspector inner elements
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(
                                              UITokens::BackgroundDark));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                        ImGui::ColorConvertU32ToFloat4(
                            WithAlpha(UITokens::PanelBackground, 255)));
  ImGui::PushStyleColor(
      ImGuiCol_FrameBgActive,
      ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::AmberGlow, 100)));
  ImGui::PushStyleColor(
      ImGuiCol_Border,
      ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::GridOverlay, 180)));
  ImGui::PushStyleColor(ImGuiCol_CheckMark,
                        ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen));
  ImGui::PushStyleColor(
      ImGuiCol_SliderGrab,
      ImGui::ColorConvertU32ToFloat4(WithAlpha(UITokens::AmberGlow, 200)));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,
                        ImGui::ColorConvertU32ToFloat4(UITokens::AmberGlow));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0.0f);

  // ACTIVE TOOL RUNTIME
  if (Components::DrawSectionHeader("ACTIVE TOOL RUNTIME",
                                    UITokens::AmberGlow)) {
    ImGui::Indent();
    const auto active_domain_name =
        magic_enum::enum_name(gs.tool_runtime.active_domain);
    Components::DrawDiagRow("Domain",
                            active_domain_name.empty()
                                ? "Unknown"
                                : std::string(active_domain_name).c_str(),
                            UITokens::CyanAccent);
    Components::DrawDiagRow("Last Action",
                            gs.tool_runtime.last_action_status.empty()
                                ? "none (idle)"
                                : gs.tool_runtime.last_action_status.c_str());
    // Viewport Status
    Components::DrawDiagRow(
        "Viewport Status",
        gs.tool_runtime.last_viewport_status.empty()
            ? "idle"
            : gs.tool_runtime.last_viewport_status.c_str(),
        UITokens::CyanAccent);
    // Gen Policy for active domain
    {
      const auto policy =
          gs.generation_policy.ForDomain(gs.tool_runtime.active_domain);
      Components::DrawDiagRow(
          "Gen Policy",
          policy == RogueCity::Core::Editor::GenerationMutationPolicy::
                        LiveDebounced
              ? "LiveDebounced"
              : "ExplicitOnly",
          UITokens::TextPrimary);
    }
    Components::DrawDiagRow(
        "Dispatch Serial",
        std::to_string(gs.tool_runtime.action_serial).c_str(),
        UITokens::TextPrimary);
    // Domain-specific subtool rows
    switch (gs.tool_runtime.active_domain) {
    case RogueCity::Core::Editor::ToolDomain::Road: {
      const auto road_sub =
          magic_enum::enum_name(gs.tool_runtime.road_subtool);
      Components::DrawDiagRow("Subtool",
                              road_sub.empty() ? "?"
                                               : std::string(road_sub).c_str(),
                              UITokens::AmberGlow);
      const auto spline_sub =
          magic_enum::enum_name(gs.tool_runtime.road_spline_subtool);
      Components::DrawDiagRow(
          "Spline Sub",
          spline_sub.empty() ? "?" : std::string(spline_sub).c_str(),
          UITokens::TextPrimary);
      break;
    }
    case RogueCity::Core::Editor::ToolDomain::Water:
    case RogueCity::Core::Editor::ToolDomain::Flow: {
      const auto sub =
          magic_enum::enum_name(gs.tool_runtime.water_subtool);
      Components::DrawDiagRow("Subtool",
                              sub.empty() ? "?" : std::string(sub).c_str(),
                              UITokens::InfoBlue);
      break;
    }
    case RogueCity::Core::Editor::ToolDomain::District: {
      const auto sub =
          magic_enum::enum_name(gs.tool_runtime.district_subtool);
      Components::DrawDiagRow("Subtool",
                              sub.empty() ? "?" : std::string(sub).c_str(),
                              UITokens::GreenHUD);
      break;
    }
    default:
      break;
    }
    DrawActiveToolControls(gs, dt);
    ImGui::Unindent();
    ImGui::Spacing();
  }

  // GIZMO
  if (Components::DrawSectionHeader("GIZMO", UITokens::CyanAccent)) {
    ImGui::Indent();
    ImGui::Checkbox("Enabled##Gizmo", &gs.gizmo.enabled);
    ImGui::SameLine();
    ImGui::Checkbox("Visible##Gizmo", &gs.gizmo.visible);
    ImGui::SameLine();
    ImGui::Checkbox("Snapping##Gizmo", &gs.gizmo.snapping);
    // Operation combo
    {
      static const char* k_ops[] = {"Translate", "Rotate", "Scale"};
      int op_idx = static_cast<int>(gs.gizmo.operation);
      ImGui::SetNextItemWidth(120.0f);
      if (ImGui::Combo("Operation##Gizmo", &op_idx, k_ops, 3)) {
        gs.gizmo.operation =
            static_cast<RogueCity::Core::Editor::GizmoOperation>(op_idx);
      }
    }

    ImGui::SetNextItemWidth(100.0f);
    ImGui::SliderFloat("Translate Snap", &gs.gizmo.translate_snap, 0.1f, 100.0f,
                       "%.1f");
    ImGui::SetNextItemWidth(100.0f);
    ImGui::SliderFloat("Rotate Snap (deg)", &gs.gizmo.rotate_snap_degrees, 1.0f,
                       90.0f, "%.1f");
    ImGui::SetNextItemWidth(100.0f);
    ImGui::SliderFloat("Scale Snap", &gs.gizmo.scale_snap, 0.1f, 10.0f, "%.2f");
    ImGui::Unindent();
    ImGui::Spacing();
  }

  // LAYER MANAGER
  if (Components::DrawSectionHeader("LAYER MANAGER", UITokens::GreenHUD)) {
    ImGui::Indent();
    for (auto &layer : gs.layer_manager.layers) {
      ImGui::Checkbox(layer.name.c_str(), &layer.visible);
      ImGui::SameLine(120.0f);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      ImGui::SliderFloat((std::string("##op_") + layer.name).c_str(),
                         &layer.opacity, 0.0f, 1.0f, "");
    }
    ImGui::Checkbox("Dim Inactive Layers", &gs.layer_manager.dim_inactive);
    ImGui::Checkbox("See Through Hidden Layers",
                    &gs.layer_manager.allow_through_hidden);
    ImGui::Spacing();
    if (ImGui::Button("Assign Selection \xe2\x86\x92 Active Layer")) {
      for (const auto& item : gs.selection_manager.Items()) {
        const uint64_t key =
            (static_cast<uint64_t>(static_cast<uint8_t>(item.kind)) << 32ull) |
            static_cast<uint64_t>(item.id);
        gs.entity_layers[key] = gs.layer_manager.active_layer;
      }
    }
    ImGui::Unindent();
    ImGui::Spacing();
  }

  // VALIDATION OVERLAY
  if (Components::DrawSectionHeader("VALIDATION OVERLAY",
                                    UITokens::AmberGlow)) {
    ImGui::Indent();
    ImGui::Checkbox("Show Overlay##ValOv", &gs.validation_overlay.enabled);
    ImGui::SameLine();
    ImGui::Checkbox("Show Warnings##ValOv",
                    &gs.validation_overlay.show_warnings);
    ImGui::Checkbox("Show Labels##ValOv", &gs.validation_overlay.show_labels);
    ImGui::Spacing();
    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen),
        "Errors: %d", static_cast<int>(gs.validation_overlay.errors.size()));
    ImGui::Unindent();
    ImGui::Spacing();
  }

  // PROPERTY EDITOR / QUERY
  if (Components::DrawSectionHeader("QUERY SELECTION",
                                    UITokens::MagentaHighlight)) {
    static PropertyEditor editor;
    editor.Draw(gs);
  }

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(7);

  uiint.RegisterWidget(
      {"property_editor", "Selection", "selection_manager", {"detail"}});
}

void Draw(float dt) {
  const bool open =
      Components::BeginTokenPanel("Inspector", UITokens::MagentaHighlight);
  if (!open) {
    Components::EndTokenPanel();
    return;
  }

  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{
          "Inspector",
          "Inspector",
          "inspector",
          "ToolDeck",
          "visualizer/src/ui/panels/rc_panel_inspector.cpp",
          {"detail", "selection"}},
      true);

  DrawContent(dt);

  uiint.EndPanel();
  Components::EndTokenPanel();
}

} // namespace RC_UI::Panels::Inspector
