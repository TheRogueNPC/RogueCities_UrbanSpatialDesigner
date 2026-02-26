#pragma once

#include "RogueCity/Core/Editor/EditorState.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"

namespace RogueCity::App::Tools {

struct GeometryPolicy {
  double district_placement_half_extent{45.0};
  double lot_placement_half_extent{16.0};
  double building_default_scale{1.0};
  double water_default_depth{6.0};
  double water_falloff_radius_world{42.0};
  double merge_radius_world{40.0};
  double edge_insert_multiplier{1.5};
  double base_pick_radius{8.0};
};

[[nodiscard]] GeometryPolicy
ResolveGeometryPolicy(const RogueCity::Core::Editor::GlobalState &gs,
                      RogueCity::Core::Editor::EditorState editor_state);

[[nodiscard]] double ComputeFalloffWeight(double distance, double radius);

} // namespace RogueCity::App::Tools
