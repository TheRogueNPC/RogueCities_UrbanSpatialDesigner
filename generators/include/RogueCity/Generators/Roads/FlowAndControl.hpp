#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"

#include <vector>

namespace RogueCity::Generators::Roads {

    struct RoadFlowDefaults {
        float v_base = 13.0f;
        float cap_base = 1.0f;
        float access_control = 0.0f;
        bool signal_allowed = true;
        bool roundabout_allowed = true;
    };

    struct ControlThresholds {
        float uncontrolled_d_max = 10.0f;
        float uncontrolled_r_max = 10.0f;
        float yield_d_max = 25.0f;
        float yield_r_max = 25.0f;
        float allway_d_max = 45.0f;
        float allway_r_max = 45.0f;
        float signal_d_max = 75.0f;
        float signal_r_max = 75.0f;
        float roundabout_d_min = 60.0f;
        float roundabout_r_min = 60.0f;
        float interchange_d_min = 120.0f;
        float interchange_r_min = 120.0f;
    };

    struct FlowControlConfig {
        std::vector<RoadFlowDefaults> road_defaults;
        ControlThresholds thresholds{};
        double turn_penalty = 12.0;
        float district_speed_mult = 1.0f;
        float zone_speed_mult = 1.0f;
    };

    void applyFlowAndControl(Urban::Graph& g, const FlowControlConfig& cfg);

} // namespace RogueCity::Generators::Roads
