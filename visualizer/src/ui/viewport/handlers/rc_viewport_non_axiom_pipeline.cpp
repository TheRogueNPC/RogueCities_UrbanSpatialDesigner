#include "ui/viewport/handlers/rc_viewport_non_axiom_pipeline.h"

#include "RogueCity/App/Editor/CommandHistory.hpp"
#include "RogueCity/App/Editor/EditorManipulation.hpp"
#include "RogueCity/App/Tools/GeometryPolicy.hpp"
#include "RogueCity/App/Tools/RoadTool.hpp"
#include "RogueCity/App/Tools/WaterTool.hpp"
#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/Core/Editor/SelectionSync.hpp"
#include "ui/tools/rc_tool_interaction_metrics.h"
#include "ui/viewport/handlers/rc_viewport_domain_handlers.h"
#include "ui/viewport/handlers/rc_viewport_handler_common.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <unordered_set>

namespace RC_UI::Viewport {

NonAxiomInteractionResult ProcessNonAxiomViewportInteractionPipeline(
    const NonAxiomInteractionParams &params,
    NonAxiomInteractionState *interaction_state) {
  NonAxiomInteractionResult result{};
  if (params.axiom_mode || params.primary_viewport == nullptr ||
      params.global_state == nullptr || interaction_state == nullptr) {
    return result;
  }

  auto &gs = *params.global_state;
  ImGuiIO &io = ImGui::GetIO();
  const bool left_click_attempt = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
  result.active = true;
  result.has_world_pos = true;
  result.world_pos = params.primary_viewport->screen_to_world(params.mouse_pos);
  result.status_code = "ready";

  if (params.editor_state !=
      RogueCity::Core::Editor::EditorState::Editing_Water) {
    interaction_state->water_vertex_drag.active = false;
    interaction_state->water_pen_drag.active = false;
  }
  if (params.editor_state !=
      RogueCity::Core::Editor::EditorState::Editing_Roads) {
    interaction_state->road_vertex_drag.active = false;
    interaction_state->road_pen_drag.active = false;
  }
  if (params.editor_state !=
      RogueCity::Core::Editor::EditorState::Editing_Districts) {
    interaction_state->district_boundary_drag.active = false;
  }

  if (!params.allow_viewport_mouse_actions || !params.in_viewport ||
      params.minimap_hovered) {
    gs.hovered_entity.reset();
    if (left_click_attempt) {
      result.status_code = "blocked-input-gate";
      result.outcome = InteractionOutcome::BlockedByInputGate;
      gs.tool_runtime.last_viewport_status = result.status_code;
      gs.tool_runtime.last_viewport_status_frame = gs.frame_counter;
    }
    return result;
  }

  const bool domain_requires_explicit_generation =
      !gs.generation_policy.IsLive(gs.tool_runtime.active_domain);
  const auto dirty_before = gs.dirty_layers.flags;
  bool selection_click_applied = false;
  bool command_history_applied = false;

  if (params.allow_viewport_key_actions) {
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
      auto &history = RogueCity::App::GetEditorCommandHistory();
      if (io.KeyShift) {
        if (history.CanRedo()) {
          history.Redo();
          command_history_applied = true;
        }
      } else if (history.CanUndo()) {
        history.Undo();
        command_history_applied = true;
      }
    } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
      auto &history = RogueCity::App::GetEditorCommandHistory();
      if (history.CanRedo()) {
        history.Redo();
        command_history_applied = true;
      }
    } else if (ImGui::IsKeyPressed(ImGuiKey_G)) {
      gs.gizmo.enabled = true;
      gs.gizmo.operation = RogueCity::Core::Editor::GizmoOperation::Translate;
    } else if (ImGui::IsKeyPressed(ImGuiKey_R)) {
      gs.gizmo.enabled = true;
      gs.gizmo.operation = RogueCity::Core::Editor::GizmoOperation::Rotate;
    } else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
      gs.gizmo.enabled = true;
      gs.gizmo.operation = RogueCity::Core::Editor::GizmoOperation::Scale;
    } else if (ImGui::IsKeyPressed(ImGuiKey_X)) {
      gs.gizmo.snapping = !gs.gizmo.snapping;
    } else if (ImGui::IsKeyPressed(ImGuiKey_1)) {
      gs.layer_manager.active_layer = 0u;
    } else if (ImGui::IsKeyPressed(ImGuiKey_2)) {
      gs.layer_manager.active_layer = 1u;
    } else if (ImGui::IsKeyPressed(ImGuiKey_3)) {
      gs.layer_manager.active_layer = 2u;
    }
  }

  Handlers::ApplyToolPreferredGizmoOperation(gs, params.editor_state);

  bool navigation_pan_applied = false;
  bool navigation_zoom_applied = false;

  if (io.MouseWheel != 0.0f) {
    const float z = params.primary_viewport->get_camera_z();
    const float factor = std::pow(1.12f, -io.MouseWheel);
    const float new_z = std::clamp(z * factor, 80.0f, 4000.0f);
    params.primary_viewport->set_camera_position(
        params.primary_viewport->get_camera_xy(), new_z);
    result.world_pos =
        params.primary_viewport->screen_to_world(params.mouse_pos);
    navigation_zoom_applied = true;
  }

  const bool pan = io.MouseDown[ImGuiMouseButton_Middle] ||
                   (io.KeyShift && io.MouseDown[ImGuiMouseButton_Right]);
  if (pan) {
    const float z = params.primary_viewport->get_camera_z();
    const float pan_zoom = 500.0f / std::max(100.0f, z);
    const float yaw = params.primary_viewport->get_camera_yaw();
    const float dx = io.MouseDelta.x / pan_zoom;
    const float dy = io.MouseDelta.y / pan_zoom;
    const float c = std::cos(yaw);
    const float s = std::sin(yaw);
    RogueCity::Core::Vec2 delta_world(dx * c - dy * s, dx * s + dy * c);

    auto cam = params.primary_viewport->get_camera_xy();
    cam.x -= delta_world.x;
    cam.y -= delta_world.y;
    params.primary_viewport->set_camera_position(cam, z);
    result.world_pos =
        params.primary_viewport->screen_to_world(params.mouse_pos);
    gs.hovered_entity.reset();
    navigation_pan_applied = true;
  }

  const float zoom = params.primary_viewport->world_to_screen_scale(1.0f);
  const auto interaction_metrics =
      RC_UI::Tools::BuildToolInteractionMetrics(zoom, gs.config.ui_scale);
  const auto geometry_policy =
      RogueCity::App::Tools::ResolveGeometryPolicy(gs, params.editor_state);
  Handlers::ViewportInteractionContext interaction_context{
      gs,
      params.editor_state,
      result.world_pos,
      interaction_metrics,
      geometry_policy,
      io,
  };

  bool consumed_interaction = navigation_pan_applied;
  if (!consumed_interaction) {
    if (params.editor_state ==
            RogueCity::Core::Editor::EditorState::Editing_Roads &&
        params.road_tool) {
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        params.road_tool->on_mouse_down(result.world_pos);
      if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        params.road_tool->on_mouse_up(result.world_pos);
      if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        params.road_tool->on_mouse_move(result.world_pos);
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        params.road_tool->on_right_click(result.world_pos);
      consumed_interaction =
          true; // Tools now handle internal consumption check
    } else if (params.editor_state ==
                   RogueCity::Core::Editor::EditorState::Editing_Water &&
               params.water_tool) {
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        params.water_tool->on_mouse_down(result.world_pos);
      if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        params.water_tool->on_mouse_up(result.world_pos);
      if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        params.water_tool->on_mouse_move(result.world_pos);
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        params.water_tool->on_right_click(result.world_pos);
      consumed_interaction = true;
    } else {
      consumed_interaction = Handlers::HandleDomainPlacementStage(
          interaction_context, *interaction_state);
    }
  }

  const auto &selected_items = gs.selection_manager.Items();
  if (!consumed_interaction && gs.gizmo.enabled && !selected_items.empty()) {
    const RogueCity::Core::Vec2 pivot = Handlers::ComputeSelectionPivot(gs);
    const double pick_radius = interaction_metrics.world_gizmo_pick_radius;
    const double dist_to_pivot = result.world_pos.distanceTo(pivot);

    auto can_start_drag = [&]() {
      using RogueCity::Core::Editor::GizmoOperation;
      switch (gs.gizmo.operation) {
      case GizmoOperation::Translate:
        return dist_to_pivot <= pick_radius * 1.5;
      case GizmoOperation::Rotate:
        return dist_to_pivot >= pick_radius * 1.2 &&
               dist_to_pivot <= pick_radius * 3.0;
      case GizmoOperation::Scale:
        return dist_to_pivot >= pick_radius * 1.0 &&
               dist_to_pivot <= pick_radius * 2.2;
      default:
        return false;
      }
    };

    if (!interaction_state->gizmo_drag.active &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt &&
        !io.KeyShift && !io.KeyCtrl && can_start_drag()) {
      interaction_state->gizmo_drag.active = true;
      interaction_state->gizmo_drag.pivot = pivot;
      interaction_state->gizmo_drag.previous_world = result.world_pos;
      consumed_interaction = true;
    }

    if (interaction_state->gizmo_drag.active) {
      if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        using RogueCity::App::EditorManipulation::ApplyRotate;
        using RogueCity::App::EditorManipulation::ApplyScale;
        using RogueCity::App::EditorManipulation::ApplyTranslate;
        using RogueCity::Core::Editor::GizmoOperation;

        bool changed = false;
        if (gs.gizmo.operation == GizmoOperation::Translate) {
          RogueCity::Core::Vec2 delta =
              result.world_pos - interaction_state->gizmo_drag.previous_world;
          if (gs.gizmo.snapping && gs.gizmo.translate_snap > 0.0f) {
            delta.x = std::round(delta.x / gs.gizmo.translate_snap) *
                      gs.gizmo.translate_snap;
            delta.y = std::round(delta.y / gs.gizmo.translate_snap) *
                      gs.gizmo.translate_snap;
          }
          changed = ApplyTranslate(gs, selected_items, delta);
        } else if (gs.gizmo.operation == GizmoOperation::Rotate) {
          const RogueCity::Core::Vec2 prev =
              interaction_state->gizmo_drag.previous_world -
              interaction_state->gizmo_drag.pivot;
          const RogueCity::Core::Vec2 curr =
              result.world_pos - interaction_state->gizmo_drag.pivot;
          const double prev_angle = std::atan2(prev.y, prev.x);
          const double curr_angle = std::atan2(curr.y, curr.x);
          double delta_angle = curr_angle - prev_angle;
          if (gs.gizmo.snapping && gs.gizmo.rotate_snap_degrees > 0.0f) {
            const double step =
                gs.gizmo.rotate_snap_degrees * std::numbers::pi / 180.0;
            delta_angle = std::round(delta_angle / step) * step;
          }
          changed =
              ApplyRotate(gs, selected_items,
                          interaction_state->gizmo_drag.pivot, delta_angle);
        } else if (gs.gizmo.operation == GizmoOperation::Scale) {
          const double prev_dist = std::max(
              1e-5, interaction_state->gizmo_drag.previous_world.distanceTo(
                        interaction_state->gizmo_drag.pivot));
          const double curr_dist = std::max(
              1e-5,
              result.world_pos.distanceTo(interaction_state->gizmo_drag.pivot));
          double factor = curr_dist / prev_dist;
          if (gs.gizmo.snapping && gs.gizmo.scale_snap > 0.0f) {
            factor = 1.0 + std::round((factor - 1.0) / gs.gizmo.scale_snap) *
                               gs.gizmo.scale_snap;
          }
          changed = ApplyScale(gs, selected_items,
                               interaction_state->gizmo_drag.pivot, factor);
        }

        if (changed) {
          std::unordered_set<uint64_t> seen;
          seen.reserve(selected_items.size());
          for (const auto &item : selected_items) {
            if (!seen.insert(Handlers::SelectionKey(item)).second) {
              continue;
            }
            Handlers::PromoteEntityToUserLocked(gs, item.kind, item.id);
            Handlers::MarkDirtyForSelectionKind(gs, item.kind);
          }
        }
        interaction_state->gizmo_drag.previous_world = result.world_pos;
        consumed_interaction = true;
      }
      if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        interaction_state->gizmo_drag.active = false;
        consumed_interaction = true;
      }
    }
  }

  if (!consumed_interaction) {
    consumed_interaction = Handlers::HandleRoadVertexEdits(interaction_context,
                                                           *interaction_state);
  }

  if (!consumed_interaction) {
    consumed_interaction = Handlers::HandleDistrictVertexEdits(
        interaction_context, *interaction_state);
  }

  const bool start_box =
      io.KeyAlt && io.KeyShift && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
  const bool start_lasso =
      io.KeyAlt && !io.KeyShift && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

  if (!consumed_interaction && start_box) {
    interaction_state->selection_drag.box_active = true;
    interaction_state->selection_drag.lasso_active = false;
    interaction_state->selection_drag.box_start = result.world_pos;
    interaction_state->selection_drag.box_end = result.world_pos;
    gs.hovered_entity.reset();
    consumed_interaction = true;
  } else if (!consumed_interaction && start_lasso) {
    interaction_state->selection_drag.lasso_active = true;
    interaction_state->selection_drag.box_active = false;
    interaction_state->selection_drag.lasso_points.clear();
    interaction_state->selection_drag.lasso_points.push_back(result.world_pos);
    gs.hovered_entity.reset();
    consumed_interaction = true;
  }

  if (!consumed_interaction && interaction_state->selection_drag.box_active) {
    interaction_state->selection_drag.box_end = result.world_pos;
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      const double min_x =
          std::min(interaction_state->selection_drag.box_start.x,
                   interaction_state->selection_drag.box_end.x);
      const double max_x =
          std::max(interaction_state->selection_drag.box_start.x,
                   interaction_state->selection_drag.box_end.x);
      const double min_y =
          std::min(interaction_state->selection_drag.box_start.y,
                   interaction_state->selection_drag.box_end.y);
      const double max_y =
          std::max(interaction_state->selection_drag.box_start.y,
                   interaction_state->selection_drag.box_end.y);

      auto region_items = Handlers::QueryRegionFromViewportIndex(
          gs,
          [=](const RogueCity::Core::Vec2 &p) {
            return p.x >= min_x && p.x <= max_x && p.y >= min_y && p.y <= max_y;
          },
          true);
      gs.selection_manager.SetItems(std::move(region_items));
      RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
      gs.hovered_entity.reset();
      interaction_state->selection_drag.box_active = false;
    }
  } else if (!consumed_interaction &&
             interaction_state->selection_drag.lasso_active) {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      if (interaction_state->selection_drag.lasso_points.empty() ||
          interaction_state->selection_drag.lasso_points.back().distanceTo(
              result.world_pos) > interaction_metrics.world_lasso_step) {
        interaction_state->selection_drag.lasso_points.push_back(
            result.world_pos);
      }
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      auto region_items = Handlers::QueryRegionFromViewportIndex(
          gs, [&](const RogueCity::Core::Vec2 &p) {
            return Handlers::PointInPolygon(
                p, interaction_state->selection_drag.lasso_points);
          });
      gs.selection_manager.SetItems(std::move(region_items));
      RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
      gs.hovered_entity.reset();
      interaction_state->selection_drag.lasso_active = false;
      interaction_state->selection_drag.lasso_points.clear();
    }
  } else if (!consumed_interaction) {
    const bool precision_pick_mode = io.KeyShift && io.KeyCtrl;
    const auto hovered = Handlers::PickFromViewportIndex(
        gs, result.world_pos, interaction_metrics,
        precision_pick_mode ? 0.6 : 1.8, !precision_pick_mode);
    gs.hovered_entity = hovered;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      if (hovered.has_value()) {
        if (io.KeyShift) {
          gs.selection_manager.Add(hovered->kind, hovered->id);
        } else if (io.KeyCtrl) {
          gs.selection_manager.Toggle(hovered->kind, hovered->id);
        } else {
          gs.selection_manager.Select(hovered->kind, hovered->id);
        }
        RogueCity::Core::Editor::SyncPrimarySelectionFromManager(gs);
        selection_click_applied = true;
      } else if (!io.KeyShift && !io.KeyCtrl) {
        gs.selection_manager.Clear();
        RogueCity::Core::Editor::ClearPrimarySelection(gs.selection);
        gs.hovered_entity.reset();
        selection_click_applied = true;
      }
    }
  }

  bool dirty_changed = false;
  for (size_t i = 0; i < dirty_before.size(); ++i) {
    if (dirty_before[i] != gs.dirty_layers.flags[i]) {
      dirty_changed = true;
      break;
    }
  }

  const bool left_click_unmodified =
      left_click_attempt && !io.KeyAlt && !io.KeyShift && !io.KeyCtrl;

  if (dirty_changed) {
    result.handled = true;
    result.outcome = InteractionOutcome::Mutation;
    result.requires_explicit_generation = domain_requires_explicit_generation;
    result.status_code = domain_requires_explicit_generation
                             ? "mutated-explicit-generate-required"
                             : "mutated-live-preview";
  } else if (selection_click_applied) {
    result.handled = true;
    result.outcome = InteractionOutcome::Selection;
    result.status_code = "selection-updated";
  } else if (navigation_pan_applied || navigation_zoom_applied) {
    result.handled = true;
    result.outcome = InteractionOutcome::ActivateOnly;
    result.status_code =
        navigation_pan_applied ? "viewport-pan" : "viewport-zoom";
  } else if (command_history_applied) {
    result.handled = true;
    result.outcome = InteractionOutcome::ActivateOnly;
    result.status_code = "undo-redo-applied";
  } else if (consumed_interaction) {
    result.handled = true;
    result.outcome = InteractionOutcome::GizmoInteraction;
    result.status_code = "viewport-interaction-handled";
  } else if (left_click_unmodified) {
    result.outcome = InteractionOutcome::NoEligibleTarget;
    result.status_code = "click-no-target";
  }

  if (result.handled || result.outcome != InteractionOutcome::None) {
    gs.tool_runtime.last_viewport_status = result.status_code;
    gs.tool_runtime.last_viewport_status_frame = gs.frame_counter;
  }

  return result;
}

std::optional<RogueCity::Core::Editor::SelectionItem>
PickFromViewportIndexTestHook(
    const RogueCity::Core::Editor::GlobalState &gs,
    const RogueCity::Core::Vec2 &world_pos,
    const RC_UI::Tools::ToolInteractionMetrics &interaction_metrics) {
  return Handlers::PickFromViewportIndex(gs, world_pos, interaction_metrics);
}

std::vector<RogueCity::Core::Editor::SelectionItem>
QueryRegionFromViewportIndexTestHook(
    const RogueCity::Core::Editor::GlobalState &gs,
    const RogueCity::Core::Vec2 &world_min,
    const RogueCity::Core::Vec2 &world_max, bool include_hidden) {
  const double min_x = std::min(world_min.x, world_max.x);
  const double max_x = std::max(world_min.x, world_max.x);
  const double min_y = std::min(world_min.y, world_max.y);
  const double max_y = std::max(world_min.y, world_max.y);
  return Handlers::QueryRegionFromViewportIndex(
      gs,
      [=](const RogueCity::Core::Vec2 &p) {
        return p.x >= min_x && p.x <= max_x && p.y >= min_y && p.y <= max_y;
      },
      include_hidden);
}

} // namespace RC_UI::Viewport
