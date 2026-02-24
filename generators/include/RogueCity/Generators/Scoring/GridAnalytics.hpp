#pragma once

#include "RogueCity/Core/Analytics/GridMetrics.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"
#include <vector>

namespace RogueCity::Generators::Scoring {

    /// Analyzer for evaluating road network quality metrics.
    class GridAnalytics {
    public:
        /// Evaluates a full road graph and produces a composite quality report.
        [[nodiscard]] static Core::GridQualityReport Evaluate(const Urban::Graph& graph);

        /// Extracts semantic "strokes" from a graph by merging edges at junctions
        /// where the path remains nearly straight and maintains road type.
        [[nodiscard]] static std::vector<Core::RoadStroke> ExtractStrokes(
            const Urban::Graph& graph, 
            float turn_tolerance_deg = 25.0f);

    private:
        /// Calculates orientation order (\Phi) using Shannon Entropy of bearings.
        [[nodiscard]] static float CalculateOrientationOrder(const std::vector<Core::RoadStroke>& strokes);

        /// Calculates intersection proportion (I) from graph vertex degrees.
        [[nodiscard]] static void CalculateIntersectionStats(
            const Urban::Graph& graph, 
            uint32_t& total, 
            uint32_t& four_way);

        /// Calculates average straightness (\varsigma) of all strokes.
        [[nodiscard]] static float CalculateAverageStraightness(const std::vector<Core::RoadStroke>& strokes);
    };

} // namespace RogueCity::Generators::Scoring
