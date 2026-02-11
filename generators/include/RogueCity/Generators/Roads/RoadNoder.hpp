#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Roads/PolylineRoadCandidate.hpp"
#include "RogueCity/Generators/Roads/SegmentGridStorage.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"

#include <vector>

namespace RogueCity::Generators::Roads {

    struct RoadTypeParams {
        float snap_radius = 8.0f;
        float weld_radius = 5.0f;
        float min_separation = 60.0f;
        float min_edge_length = 1.0f;

        float min_node_spacing = 50.0f;
        float v_base = 12.0f;
        float cap_base = 1.0f;
        float access_control = 0.0f;
        bool signal_allowed = true;
        bool roundabout_allowed = true;
        float grade_sep_preferred = 0.0f;
    };

    struct NoderConfig {
        std::vector<RoadTypeParams> type_params;
        float global_weld_radius = 5.0f;
        float global_min_edge_length = 1.0f;
        int max_layers = 2;
    };

    class RoadNoder {
    public:
        explicit RoadNoder(NoderConfig cfg);

        void buildGraph(
            const std::vector<PolylineRoadCandidate>& candidates,
            Urban::Graph& out_graph);

    private:
        struct RawSegment {
            uint32_t id = 0;
            uint32_t candidate_id = 0;
            uint32_t local_index = 0;
            Core::Vec2 a{};
            Core::Vec2 b{};
            int layer_id = 0;
            Core::RoadType type = Core::RoadType::Street;
        };

        NoderConfig cfg_;
        SegmentGridStorage seg_index_;

        [[nodiscard]] const RoadTypeParams& paramsForType(Core::RoadType type) const;
        [[nodiscard]] Urban::VertexID getOrCreateVertex(
            Urban::Graph& g,
            const Core::Vec2& p,
            int layer_id,
            float weld_radius) const;
        [[nodiscard]] bool segmentIntersect(
            const Core::Vec2& a0,
            const Core::Vec2& a1,
            const Core::Vec2& b0,
            const Core::Vec2& b1,
            Core::Vec2& out_p,
            double& out_ta,
            double& out_tb,
            float tol) const;
        [[nodiscard]] float polylineLength(const std::vector<Core::Vec2>& pts) const;
    };

} // namespace RogueCity::Generators::Roads
