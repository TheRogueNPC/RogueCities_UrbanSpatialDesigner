#include "RogueCity/App/Tools/GeometryPolicy.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "ui/tools/rc_tool_interaction_metrics.h"

#include <cassert>

int main() {
  using RogueCity::Core::Editor::EditorState;
  using RogueCity::Core::Editor::GlobalState;
  using namespace RogueCity::App::Tools;

  const auto metrics_zoomed_out =
      RC_UI::Tools::BuildToolInteractionMetrics(0.5f, 1.0f);
  const auto metrics_zoomed_in =
      RC_UI::Tools::BuildToolInteractionMetrics(4.0f, 1.0f);

  assert(metrics_zoomed_out.world_pick_radius >
         metrics_zoomed_in.world_pick_radius);
  assert(metrics_zoomed_out.world_vertex_pick_radius >
         metrics_zoomed_in.world_vertex_pick_radius);
  assert(metrics_zoomed_out.world_gizmo_pick_radius >
         metrics_zoomed_in.world_gizmo_pick_radius);
  assert(metrics_zoomed_in.world_pick_radius >= 4.0);

  GlobalState gs{};
  gs.config.ui_scale = 1.0f;
  const auto district_default =
      ResolveGeometryPolicy(gs, EditorState::Editing_Districts);

  gs.config.ui_scale = 2.0f;
  const auto district_scaled =
      ResolveGeometryPolicy(gs, EditorState::Editing_Districts);
  assert(district_scaled.district_placement_half_extent >
         district_default.district_placement_half_extent);
  assert(district_scaled.merge_radius_world >
         district_default.merge_radius_world);

  gs.config.ui_scale = 1.0f;
  gs.tool_runtime.water_subtool = RogueCity::Core::Editor::WaterSubtool::Erode;
  const auto water_erode =
      ResolveGeometryPolicy(gs, EditorState::Editing_Water);
  gs.tool_runtime.water_subtool = RogueCity::Core::Editor::WaterSubtool::Flow;
  const auto water_flow = ResolveGeometryPolicy(gs, EditorState::Editing_Water);
  assert(water_erode.water_falloff_radius_world >
         water_flow.water_falloff_radius_world);

  const double w0 = ComputeFalloffWeight(0.0, 50.0);
  const double wmid = ComputeFalloffWeight(25.0, 50.0);
  const double wend = ComputeFalloffWeight(50.0, 50.0);
  assert(w0 > wmid);
  assert(wmid > wend);
  assert(wend == 0.0);

  return 0;
}
