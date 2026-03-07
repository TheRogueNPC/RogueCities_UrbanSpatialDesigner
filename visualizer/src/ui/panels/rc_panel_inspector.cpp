// FILE: visualizer/src/ui/panels/rc_panel_inspector.cpp
// (RogueCities_UrbanSpatialDesigner) PURPOSE: Inspector with reactive selection
// focus.
#include "ui/panels/rc_panel_inspector.h"

#include "ui/api/rc_imgui_api.h"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/introspection/UiIntrospection.h"
#include "ui/panels/rc_property_editor.h"
#include "ui/rc_ui_components.h"
#include <RogueCity/Visualizer/LucideIcons.hpp>

#include <imgui.h>
#include <algorithm>
#include <array>
#include <magic_enum/magic_enum.hpp>

namespace RC_UI::Panels::Inspector {

namespace {

void DrawActiveToolControls(RogueCity::Core::Editor::GlobalState &gs,
                            float dt) {
  using namespace RogueCity::Core::Editor;
  switch (gs.tool_runtime.active_domain) {
  case ToolDomain::Road:
  case ToolDomain::Paths:
    API::Checkbox("Spline Editing", &gs.spline_editor.enabled);
    API::SameLine();
    API::SetNextItemWidth(120.0f);
    API::SliderInt("Samples##SplineRoad",
                     &gs.spline_editor.samples_per_segment, 2, 24);
    break;
  case ToolDomain::Water:
    API::Checkbox("Spline Editing", &gs.spline_editor.enabled);
    API::SameLine();
    API::SetNextItemWidth(120.0f);
    API::SliderInt("Samples##SplineWater",
                     &gs.spline_editor.samples_per_segment, 2, 24);
    break;
  case ToolDomain::Flow:
    ImGui::SeparatorText("Simulation Setup");
    API::Checkbox("Enable Pedestrians",
                    &gs.flow_simulation.enable_pedestrians);
    if (gs.flow_simulation.enable_pedestrians) {
      API::SetNextItemWidth(150.0f);
      API::SliderFloat("Pedestrian Density",
                         &gs.flow_simulation.pedestrian_density, 0.0f, 1.0f);
    }
    API::Checkbox("Enable Traffic", &gs.flow_simulation.enable_traffic);
    if (gs.flow_simulation.enable_traffic) {
      API::SetNextItemWidth(150.0f);
      API::SliderFloat("Traffic Density", &gs.flow_simulation.traffic_density,
                         0.0f, 1.0f);
    }
    ImGui::SeparatorText("Environment");
    API::SetNextItemWidth(150.0f);
    API::SliderFloat("Time of Day", &gs.flow_simulation.time_of_day, 0.0f,
                       24.0f, "%.1f h");
    API::SetNextItemWidth(150.0f);
    API::SliderInt("Sim Speed", &gs.flow_simulation.simulation_speed, 0, 10,
                     "%d x");
    break;
  case ToolDomain::District:
  case ToolDomain::Zone:
    API::Checkbox("Boundary Editor", &gs.district_boundary_editor.enabled);
    API::SameLine();
    API::Checkbox("Snap", &gs.district_boundary_editor.snap_to_grid);
    if (gs.district_boundary_editor.snap_to_grid) {
      API::SetNextItemWidth(120.0f);
      API::DragFloat("Snap Size", &gs.district_boundary_editor.snap_size,
                       0.5f, 0.5f, 200.0f, "%.1f");
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
  if (Components::DrawSectionHeader("RUNTIME PANEL",
                                    UITokens::AmberGlow, true,
                                    RC::SvgTextureCache::Get().Load(LC::Activity, 14.f))) {
    API::Indent();
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
    Components::DrawDiagRow("Viewport Status",
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
      const auto road_sub = magic_enum::enum_name(gs.tool_runtime.road_subtool);
      Components::DrawDiagRow(
          "Subtool", road_sub.empty() ? "?" : std::string(road_sub).c_str(),
          UITokens::AmberGlow);
      const auto spline_sub =
          magic_enum::enum_name(gs.tool_runtime.road_spline_subtool);
      Components::DrawDiagRow(
          "Spline Sub",
          spline_sub.empty() ? "?" : std::string(spline_sub).c_str(),
          UITokens::TextPrimary);
      break;
    }
    case RogueCity::Core::Editor::ToolDomain::Water: {
      const auto sub = magic_enum::enum_name(gs.tool_runtime.water_subtool);
      Components::DrawDiagRow("Subtool",
                              sub.empty() ? "?" : std::string(sub).c_str(),
                              UITokens::InfoBlue);
      break;
    }
    case RogueCity::Core::Editor::ToolDomain::Flow: {
      Components::DrawDiagRow("Subtool", "Traffic/Pedestrians",
                              UITokens::MagentaHighlight);
      break;
    }
    case RogueCity::Core::Editor::ToolDomain::District: {
      const auto sub = magic_enum::enum_name(gs.tool_runtime.district_subtool);
      Components::DrawDiagRow("Subtool",
                              sub.empty() ? "?" : std::string(sub).c_str(),
                              UITokens::GreenHUD);
      break;
    }
    default:
      break;
    }
    DrawActiveToolControls(gs, dt);
    API::Unindent();
    API::Spacing();
  }

  // GIZMO
  if (Components::DrawSectionHeader("GIZMO", UITokens::CyanAccent, true,
                                    RC::SvgTextureCache::Get().Load(LC::Move, 14.f))) {
    API::Indent();
    API::Checkbox("Enabled##Gizmo", &gs.gizmo.enabled);
    API::SameLine();
    API::Checkbox("Visible##Gizmo", &gs.gizmo.visible);
    API::SameLine();
    API::Checkbox("Snapping##Gizmo", &gs.gizmo.snapping);
    // Operation combo
    {
      static const char *k_ops[] = {"Translate", "Rotate", "Scale"};
      int op_idx = static_cast<int>(gs.gizmo.operation);
      op_idx = std::clamp(op_idx, 0, 2);
      API::SetNextItemWidth(120.0f);
      if (API::Combo("Operation##Gizmo", &op_idx, k_ops, 3)) {
        gs.gizmo.operation =
            static_cast<RogueCity::Core::Editor::GizmoOperation>(op_idx);
      }
    }

    API::SetNextItemWidth(100.0f);
    API::SliderFloat("Translate Snap", &gs.gizmo.translate_snap, 0.1f, 100.0f,
                       "%.1f");
    API::SetNextItemWidth(100.0f);
    API::SliderFloat("Rotate Snap (deg)", &gs.gizmo.rotate_snap_degrees, 1.0f,
                       90.0f, "%.1f");
    API::SetNextItemWidth(100.0f);
    API::SliderFloat("Scale Snap", &gs.gizmo.scale_snap, 0.1f, 10.0f, "%.2f");

    ImGui::SeparatorText("Debug Overlays");
    API::Checkbox("Tensor Field Overlay", &gs.debug_show_tensor_overlay);
    API::SameLine();
    API::Checkbox("Height Field Overlay", &gs.debug_show_height_overlay);
    API::Checkbox("Zone Field Overlay", &gs.debug_show_zone_overlay);

    ImGui::SeparatorText("Validation Overlay");
    API::Checkbox("Show Validation Overlay", &gs.validation_overlay.enabled);
    API::SameLine();
    API::Checkbox("Show Warnings", &gs.validation_overlay.show_warnings);
    API::Checkbox("Show Labels", &gs.validation_overlay.show_labels);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(UITokens::SuccessGreen),
                       "Errors: %d",
                       static_cast<int>(gs.validation_overlay.errors.size()));

    API::Unindent();
    API::Spacing();
  }

  // LAYER MANAGER
  if (Components::DrawSectionHeader("LAYER MANAGER", UITokens::GreenHUD, true,
                                    RC::SvgTextureCache::Get().Load(LC::Layers, 14.f))) {
    API::Indent();
    for (auto &layer : gs.layer_manager.layers) {
      API::Checkbox(layer.name.c_str(), &layer.visible);
      API::SameLine(120.0f);
      API::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      API::SliderFloat((std::string("##op_") + layer.name).c_str(),
                         &layer.opacity, 0.0f, 1.0f, "");
    }
    API::Checkbox("Dim Inactive Layers", &gs.layer_manager.dim_inactive);
    API::Checkbox("See Through Hidden Layers",
                    &gs.layer_manager.allow_through_hidden);

    ImGui::SeparatorText("Layer Visibility");
    API::Checkbox("Axioms", &gs.show_layer_axioms);
    API::SameLine();
    API::Checkbox("Water", &gs.show_layer_water);
    API::SameLine();
    API::Checkbox("Roads", &gs.show_layer_roads);
    API::Checkbox("Districts", &gs.show_layer_districts);
    API::SameLine();
    API::Checkbox("Lots", &gs.show_layer_lots);
    API::SameLine();
    API::Checkbox("Buildings", &gs.show_layer_buildings);

    ImGui::SeparatorText("Dirty Layers");
    const bool dirty_any = gs.dirty_layers.AnyDirty();
    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(dirty_any ? UITokens::YellowWarning
                                                 : UITokens::SuccessGreen),
        dirty_any ? "Dirty Layers: pending" : "Dirty Layers: clean");

    using RogueCity::Core::Editor::DirtyLayer;
    int dirty_count = 0;
    for (int i = 0; i < static_cast<int>(DirtyLayer::Count); ++i) {
      if (gs.dirty_layers.IsDirty(static_cast<DirtyLayer>(i))) {
        ++dirty_count;
      }
    }
    const float dirty_ratio =
        static_cast<float>(dirty_count) /
        static_cast<float>(static_cast<int>(DirtyLayer::Count));
    API::SetNextItemWidth(150.0f);
    ImGui::ProgressBar(dirty_ratio, ImVec2(150.0f, 0.0f),
                       dirty_any ? "Rebuild pending" : "Clean");

    struct DirtyChip {
      const char *label;
      DirtyLayer layer;
    };
    constexpr std::array<DirtyChip, 7> kDirtyChips = {{
        {"Axioms", DirtyLayer::Axioms},
        {"Tensor", DirtyLayer::Tensor},
        {"Roads", DirtyLayer::Roads},
        {"Districts", DirtyLayer::Districts},
        {"Lots", DirtyLayer::Lots},
        {"Buildings", DirtyLayer::Buildings},
        {"Viewport", DirtyLayer::ViewportIndex},
    }};

    auto draw_chip = [](const char *label, bool dirty) {
      const ImU32 chip_color = dirty ? WithAlpha(UITokens::AmberGlow, 230u)
                                     : WithAlpha(UITokens::SuccessGreen, 190u);
      const ImVec4 color = ImGui::ColorConvertU32ToFloat4(chip_color);
      ImGui::PushStyleColor(ImGuiCol_Button, color);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
      API::SmallButton(label);
      ImGui::PopStyleColor(3);
    };

    ImGui::PushID("LayerManagerDirtyChips");
    for (size_t chip_index = 0; chip_index < kDirtyChips.size(); ++chip_index) {
      const DirtyChip &chip = kDirtyChips[chip_index];
      ImGui::PushID(static_cast<int>(chip_index));
      draw_chip(chip.label, gs.dirty_layers.IsDirty(chip.layer));
      ImGui::PopID();
      if (chip_index + 1 < kDirtyChips.size()) {
        API::SameLine();
      }
    }
    ImGui::PopID();

    if (dirty_any) {
      if (API::Button("Clear Dirty Flags")) {
        API::OpenPopup("ClearDirtyFlagsHelp");
      }
      if (API::BeginPopupModal("ClearDirtyFlagsHelp", nullptr,
                                 ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Clearing dirty flags marks downstream layers as clean without rebuilding.");
        ImGui::TextWrapped("Use this only if you intentionally want to suppress regeneration prompts.");
        if (API::Button("Clear Flags", ImVec2(120, 0))) {
          gs.dirty_layers.MarkAllClean();
          API::CloseCurrentPopup();
        }
        API::SameLine();
        if (API::Button("Cancel", ImVec2(120, 0))) {
          API::CloseCurrentPopup();
        }
        API::EndPopup();
      }
    }

    API::Spacing();
    if (auto ico = RC::SvgTextureCache::Get().Load(LC::Layers, 14.f)) {
      ImGui::Image(ico, ImVec2(14, 14)); API::SameLine(0, 4);
    }
    if (API::Button("Assign Selection \xe2\x86\x92 Active Layer")) {
      for (const auto &item : gs.selection_manager.Items()) {
        const uint64_t key =
            (static_cast<uint64_t>(static_cast<uint8_t>(item.kind)) << 32ull) |
            static_cast<uint64_t>(item.id);
        gs.entity_layers[key] = gs.layer_manager.active_layer;
      }
    }
    API::Unindent();
    API::Spacing();
  }

  // PROPERTY EDITOR / QUERY
  if (Components::DrawSectionHeader("QUERY HIERARCHY",
                                    UITokens::MagentaHighlight, true,
                                    RC::SvgTextureCache::Get().Load(LC::Search, 14.f))) {
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
