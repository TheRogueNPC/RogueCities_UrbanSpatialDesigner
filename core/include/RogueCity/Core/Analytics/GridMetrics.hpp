#pragma once

#include "RogueCity/Core/Math/Vec2.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"
#include <vector>
#include <cstdint>

namespace RogueCity::Core {

    /// Comprehensive report for urban grid quality metrics.
    struct GridQualityReport {
        float straightness{ 0.0f };             // \varsigma: great-circle / path_length
        float orientation_order{ 0.0f };       // \Phi: 1 - normalized entropy
        float four_way_proportion{ 0.0f };      // I: 4-way / total intersections
        float composite_index{ 0.0f };          // (\varsigma * \Phi * I)^(1/3)
        
        float connectivity_index{ 0.0f };       // Gamma index: E / 3(V-2)
        float dead_end_proportion{ 0.0f };      // % of nodes with degree 1
        uint32_t island_count{ 0 };             // Number of disconnected networks
        uint32_t micro_segment_count{ 0 };      // Edges shorter than 1.0m

        float terrain_adaptation{ 0.0f };       // Variance in slope adaptation
        uint32_t total_intersections{ 0 };
        uint32_t four_way_intersections{ 0 };
        uint32_t total_strokes{ 0 };
    };

    /// A semantic "Road Stroke" representing a continuous path between major junctions.
    /// This is used to calculate straightness and orientation consistently across
    /// fragmented road segments.
    struct RoadStroke {
        std::vector<Vec2> points;
        RoadType type{ RoadType::Street };
        float total_length{ 0.0f };
        float displacement{ 0.0f }; // Straight-line distance between endpoints
        
        [[nodiscard]] float straightness() const {
            if (total_length <= 1e-4f) return 1.0f;
            return std::clamp(displacement / total_length, 0.0f, 1.0f);
        }

        [[nodiscard]] float bearing_degrees() const {
            if (points.size() < 2) return 0.0f;
            const Vec2 dir = points.back() - points.front();
            float angle = static_cast<float>(std::atan2(dir.y, dir.x) * (180.0 / 3.14159265358979323846));
            if (angle < 0.0f) angle += 180.0f; // 0-180 range for grid orientation
            if (angle >= 180.0f) angle -= 180.0f;
            return angle;
        }
    };

} // namespace RogueCity::Core
