#include "ui/tools/rc_tool_interaction_metrics.h"

#include <algorithm>

namespace RC_UI::Tools {

ToolInteractionMetrics BuildToolInteractionMetrics(float viewport_zoom, float ui_scale) {
    const double zoom = std::max(0.1, static_cast<double>(viewport_zoom));
    const double scale = std::max(0.75, static_cast<double>(ui_scale));

    ToolInteractionMetrics metrics{};
    metrics.world_pick_radius = std::max(4.0, (12.0 * scale) / zoom);
    metrics.world_vertex_pick_radius = std::max(5.0, (9.0 * scale) / zoom);
    metrics.world_gizmo_pick_radius = std::max(8.0, (14.0 * scale) / zoom);
    metrics.world_lasso_step = std::max(2.0, (3.0 * scale) / zoom);
    metrics.edge_insert_radius_multiplier = 1.5;
    return metrics;
}

} // namespace RC_UI::Tools

