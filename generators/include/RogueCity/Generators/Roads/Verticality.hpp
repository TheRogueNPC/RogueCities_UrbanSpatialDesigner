#pragma once

#include "RogueCity/Generators/Urban/Graph.hpp"

namespace RogueCity::Generators::Roads {

    struct VerticalityConfig {
        int max_layers = 2;
        float grade_sep_bias = 1.0f;
        float ramp_cost_mult = 1.0f;
        float visual_intrusion_weight = 1.0f;
        bool allow_grade_sep_without_ramps = true;
        bool emit_portals = true;
        float portal_offset = 6.0f;
    };

    void applyVerticality(Urban::Graph& g, const VerticalityConfig& cfg);

} // namespace RogueCity::Generators::Roads
