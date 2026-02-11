#pragma once

#include "RogueCity/Core/Types.hpp"

#include <vector>

namespace RogueCity::Generators::Roads {

    enum class TerminationReason : uint8_t {
        HitBoundary = 0,
        HitNoBuild,
        ConstraintViolation,
        MaxLength,
        DegenerateTensor,
        IntersectedRoad,
        SnappedToNetwork,
        BudgetExhausted
    };

    struct TraceMeta {
        float avg_curvature = 0.0f;
        float avg_tensor_strength = 0.0f;
        float avg_slope = 0.0f;
        float avg_flood = 0.0f;
        TerminationReason reason = TerminationReason::MaxLength;
    };

    struct PolylineRoadCandidate {
        Core::RoadType type_hint = Core::RoadType::Street;
        bool is_major_hint = false;

        int seed_id = -1;
        int layer_hint = 0;

        std::vector<Core::Vec2> pts;
        TraceMeta meta;
    };

} // namespace RogueCity::Generators::Roads
