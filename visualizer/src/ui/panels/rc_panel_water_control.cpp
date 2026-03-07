// FILE: rc_panel_water_control.cpp
// PURPOSE: Control panel for water/river editing.
// AI_INTEGRATION_TAG: V1_PASS1_TASK4_WATER_CONTROL_PANEL
// AGENT: UI/UX_Master
// CATEGORY: Control_Panel_Authoring
// NOTE: This panel now provides functional authoring seeds and live
// diagnostics.

#include "ui/panels/rc_panel_water_control.h"
#include "ui/api/rc_imgui_api.h"
#include "ui/introspection/UiIntrospection.h"
#include "ui/rc_ui_components.h"

#include <RogueCity/App/Editor/CommandHistory.hpp>
#include <RogueCity/Core/Editor/EditorState.hpp>
#include <RogueCity/Core/Editor/GlobalState.hpp>

#include "ui/panels/IPanelDrawer.h"
#include "ui/tools/rc_tool_dispatcher.h"
#include <RogueCity/Visualizer/LucideIcons.hpp>

#include <algorithm>
#include <array>
#include <imgui.h>

namespace RC_UI::Panels::WaterControl {

void DrawContent(float dt) {
  using namespace RogueCity::Core::Editor;

  GlobalState &gs = GetGlobalState();
  auto &hfsm = GetEditorHFSM();
  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  RC_UI::Panels::DrawContext ctx{
      gs, hfsm, uiint, &RogueCity::App::GetEditorCommandHistory(), dt, false};

  if (Components::DrawSectionHeader(
          "Actions", UITokens::CyanAccent, true,
          RC::SvgTextureCache::Get().Load(LC::Plus, 14.f))) {
    API::Indent();
    API::Spacing();
    RC_UI::Components::DrawToolActionGrid(RC_UI::ToolLibrary::Water, ctx);
    API::Unindent();
    API::Spacing();
  }

  static int selected_type = 0;
  static float depth_m = 6.0f;
  static bool generate_shore = true;
  static ButtonFeedback add_feedback{};

  const char *types[] = {"Lake", "River", "Ocean", "Pond"};
  API::SetNextItemWidth(170.0f);
  API::Combo("Water Type", &selected_type, types, IM_ARRAYSIZE(types));
  API::SetNextItemWidth(140.0f);
  API::SliderFloat("Depth (m)", &depth_m, 0.5f, 40.0f, "%.1f");
  API::Checkbox("Generate Shore", &generate_shore);

  if (Components::AnimatedActionButton("spawn_water", "Add Sample Body",
                                       add_feedback, dt, false,
                                       ImVec2(170.0f, 30.0f))) {
    RogueCity::Core::WaterBody body{};
    body.id = static_cast<uint32_t>(gs.waterbodies.size() + 1u);
    body.type = static_cast<RogueCity::Core::WaterType>(
        std::clamp(selected_type, 0, 3));
    body.depth = depth_m;
    body.generate_shore = generate_shore;
    body.is_user_placed = true;
    body.generation_tag = RogueCity::Core::GenerationTag::M_user;
    body.generation_locked = true;

    const RogueCity::Core::Bounds bounds =
        gs.world_constraints.isValid()
            ? RogueCity::Core::Bounds{{0.0, 0.0},
                                      {static_cast<double>(
                                           gs.world_constraints.width) *
                                           gs.world_constraints.cell_size,
                                       static_cast<double>(
                                           gs.world_constraints.height) *
                                           gs.world_constraints.cell_size}}
            : gs.texture_space_bounds;
    const RogueCity::Core::Vec2 c = bounds.center();
    const double r =
        std::max(35.0, std::min(bounds.width(), bounds.height()) * 0.04);
    body.boundary = {{c.x - r, c.y - r},
                     {c.x + r, c.y - r * 0.5},
                     {c.x + r * 0.8, c.y + r},
                     {c.x - r * 0.7, c.y + r * 0.7}};

    gs.waterbodies.add(std::move(body));
  }
  API::SameLine();
  if (API::Button("Remove Last")) {
    const auto size = gs.waterbodies.size();
    if (size > 0) {
      const auto handle = gs.waterbodies.createHandleFromData(size - 1u);
      // Validate handle and size hasn't changed during creation
      if (handle && gs.waterbodies.size() == size) {
        gs.waterbodies.remove(handle);
      }
    }
  }

  if (Components::DrawSectionHeader("Status", UITokens::InfoBlue)) {
    API::Indent();
    ImGui::Text("Total Water Bodies: %zu", gs.waterbodies.size());
    Components::StatusChip(gs.waterbodies.size() > 0 ? "ACTIVE" : "EMPTY",
                           gs.waterbodies.size() > 0 ? UITokens::SuccessGreen
                                                     : UITokens::YellowWarning,
                           true);
    API::TextDisabled(
        "Sample authoring is now live; spline/paint tools remain next.");
    API::Unindent();
    API::Spacing();
  }
}

void Draw(float dt) {
  using namespace RogueCity::Core::Editor;

  // State-reactive: Only show if in Water mode
  EditorHFSM &hfsm = GetEditorHFSM();
  if (hfsm.state() != EditorState::Editing_Water) {
    return;
  }

  const bool open =
      Components::BeginTokenPanel("Water Body Authoring", UITokens::InfoBlue,
                                  nullptr, ImGuiWindowFlags_AlwaysAutoResize);

  auto &uiint = RogueCity::UIInt::UiIntrospector::Instance();
  uiint.BeginPanel(
      RogueCity::UIInt::PanelMeta{
          "Water Body Authoring",
          "WaterControl",
          "water_authoring",
          "Right",
          "visualizer/src/ui/panels/rc_panel_water_control.cpp",
          {"authoring", "water", "controls"}},
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

} // namespace RC_UI::Panels::WaterControl
