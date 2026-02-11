#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"

#include <vector>

namespace RogueCity::Generators::Roads {

    enum class JunctionArchetype : uint8_t {
        None = 0,
        Diamond,
        FoldedDiamond,
        Cloverleaf,
        DirectionalT,
        Stack,
        CompactRoundabout
    };

    struct GreenspaceCandidate {
        Core::Polygon polygon;
        float score = 0.0f;
    };

    struct InterchangeTemplate {
        JunctionArchetype archetype = JunctionArchetype::None;
        Core::Vec2 center{};
        float radius = 0.0f;
        float rotation = 0.0f;
    };

    struct TemplateOutput {
        std::vector<Core::Polygon> paved_areas;
        std::vector<Core::Polygon> keep_out_islands;
        std::vector<Core::Polygon> support_footprints;
        std::vector<GreenspaceCandidate> greenspace_candidates;
        std::vector<InterchangeTemplate> interchanges;
    };

    struct TemplateConfig {
        int circle_segments = 12;
        float paved_radius_base = 12.0f;
        float keep_out_radius_base = 4.5f;
        float support_radius_base = 2.0f;
        float buffer_radius_base = 20.0f;
    };

    TemplateOutput emitIntersectionTemplates(const Urban::Graph& g, const TemplateConfig& cfg);

} // namespace RogueCity::Generators::Roads
