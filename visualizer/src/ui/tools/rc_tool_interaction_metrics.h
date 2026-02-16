#pragma once

namespace RC_UI::Tools {

struct ToolInteractionMetrics {
    double world_pick_radius{ 8.0 };
    double world_vertex_pick_radius{ 8.0 };
    double world_gizmo_pick_radius{ 12.0 };
    double world_lasso_step{ 3.0 };
    double edge_insert_radius_multiplier{ 1.5 };
};

[[nodiscard]] ToolInteractionMetrics BuildToolInteractionMetrics(float viewport_zoom, float ui_scale);

} // namespace RC_UI::Tools

