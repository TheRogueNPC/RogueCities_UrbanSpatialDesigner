#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"

#include <vector>

namespace RogueCity::Generators::Roads {

/*
 * A struct representing default flow settings for roads.
 */
struct RoadFlowDefaults {
    float v_base = 13.0f; // Base velocity in km/h
    float cap_base = 1.0f; // Base capacity factor
    float access_control = 0.0f; // Access control level (0-1)
    bool signal_allowed = true; // Whether signals are allowed on this road
    bool roundabout_allowed = true; // Whether roundabouts are allowed on this road
};

/*
 * A struct containing threshold values for determining the type of control needed.
 */
struct ControlThresholds {
    float uncontrolled_d_max = 10.0f; // Maximum distance (in km) before an uncontrolled intersection is used
    float uncontrolled_r_max = 10.0f; // Maximum radius (in m) before an uncontrolled intersection is used
    float yield_d_max = 25.0f; // Maximum distance (in km) before a yield sign is used
    float yield_r_max = 25.0f; // Maximum radius (in m) before a yield sign is used
    float allway_d_max = 45.0f; // Maximum distance (in km) before an all-way stop is used
    float allway_r_max = 45.0f; // Maximum risk bound before an all-way stop is used
    float signal_d_max = 75.0f; // Maximum distance (in km) before a traffic signal is used
    float signal_r_max = 75.0f; // Maximum radius (in m) before a traffic signal is used
    float roundabout_d_min = 60.0f; // Minimum distance (in km) for a roundabout to be considered
    float roundabout_r_min = 60.0f; // Minimum radius (in m) for a roundabout to be considered
    float interchange_d_min = 120.0f; // Minimum distance (in km) for an interchange to be considered
    float interchange_r_min = 120.0f; // Minimum radius (in m) for an interchange to be considered
};

/*
 * A struct containing the configuration for flow and control settings.
 */
struct FlowControlConfig {
    std::vector<RoadFlowDefaults> road_defaults; // List of default flow settings for different types of roads
    ControlThresholds thresholds{}; // Control thresholds
    double turn_penalty = 12.0; // Penalty value for turning movements
    float district_speed_mult = 1.0f; // Speed multiplier based on the district context
    float zone_speed_mult = 1.0f; // Speed multiplier based on the zone context
    float control_delay_mult = 1.0f; // Multiplier for controlling delays in traffic flow
};

/*
 * Function to apply flow and control configurations to a road network graph.
 */
void applyFlowAndControl(Urban::Graph& g, const FlowControlConfig& cfg);

} // namespace RogueCity::Generators::Roads
