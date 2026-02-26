#include "RogueCity/App/Tools/RoadTool.hpp"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/Core/Editor/EditorUtils.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include <imgui.h>

#include "RogueCity/App/Tools/GeometryPolicy.hpp"

namespace RogueCity::App {

namespace {
Tools::SplineManipulatorParams
GetRoadParams(const Core::Editor::GlobalState &gs,
              const PrimaryViewport *viewport) {
  Tools::SplineManipulatorParams params{};
  params.closed = false;
  params.snap_size = gs.params.snap_to_grid ? gs.params.snap_size : 0.0f;

  const auto policy = Tools::ResolveGeometryPolicy(
      gs, Core::Editor::EditorState::Editing_Roads);
  const float zoom = viewport ? viewport->world_to_screen_scale(1.0f) : 1.0f;
  params.pick_radius = policy.base_pick_radius / static_cast<double>(zoom);

  params.tension = gs.spline_editor.tension;
  params.samples_per_segment = gs.spline_editor.samples_per_segment;
  return params;
}
} // namespace

const char *RoadTool::tool_name() const { return "RoadTool"; }

void RoadTool::update(float delta_time, PrimaryViewport &viewport) {
  (void)delta_time;
  viewport_ = &viewport;
}

void RoadTool::render(ImDrawList *draw_list, const PrimaryViewport &viewport) {
  (void)draw_list;
  (void)viewport;
}

void RoadTool::on_mouse_down(const Core::Vec2 &world_pos) {
  auto &gs = Core::Editor::GetGlobalState();
  auto *road =
      gs.selection.selected_road
          ? Core::Editor::FindRoadMutable(gs, gs.selection.selected_road->id)
          : nullptr;

  if (road) {
    const auto params = GetRoadParams(gs, viewport_);
    if (Tools::SplineManipulator::TryBeginDrag(road->points, world_pos, params,
                                               spline_state_)) {
      spline_state_.entity_id = road->id;
    } else if (gs.tool_runtime.road_spline_subtool ==
               Core::Editor::RoadSplineSubtool::Pen) {
      // Add new point at end if pen tool is active and no vertex was picked
      road->points.push_back(world_pos);
      gs.dirty_layers.MarkDirty(Core::Editor::DirtyLayer::Roads);
    }
  }
}

void RoadTool::on_mouse_up(const Core::Vec2 &world_pos) {
  (void)world_pos;
  spline_state_.active = false;
}

void RoadTool::on_mouse_move(const Core::Vec2 &world_pos) {
  if (!spline_state_.active)
    return;

  auto &gs = Core::Editor::GetGlobalState();
  auto *road = Core::Editor::FindRoadMutable(gs, spline_state_.entity_id);

  if (road) {
    const auto params = GetRoadParams(gs, viewport_);
    if (Tools::SplineManipulator::UpdateDrag(road->points, world_pos, params,
                                             spline_state_)) {
      gs.dirty_layers.MarkDirty(Core::Editor::DirtyLayer::Roads);
    }
  }
}

void RoadTool::on_right_click(const Core::Vec2 &world_pos) {
  auto &gs = Core::Editor::GetGlobalState();
  auto *road =
      gs.selection.selected_road
          ? Core::Editor::FindRoadMutable(gs, gs.selection.selected_road->id)
          : nullptr;

  if (road) {
    const auto params = GetRoadParams(gs, viewport_);
    if (Tools::SplineManipulator::RemoveVertex(road->points, world_pos,
                                               params)) {
      gs.dirty_layers.MarkDirty(Core::Editor::DirtyLayer::Roads);
    }
  }
}

} // namespace RogueCity::App
