#include "RogueCity/Generators/Roads/RoadClassifier.hpp"

#include "RogueCity/Generators/Urban/GraphAlgorithms.hpp"

#include <algorithm>
#include <numeric>

namespace RogueCity::Generators {

    namespace {

        [[nodiscard]] float roadLength(const Road& road) {
            return static_cast<float>(road.length());
        }

    } // namespace

    void RoadClassifier::classifyNetwork(fva::Container<Road>& roads) {
        if (roads.size() == 0) {
            return;
        }

        double total_length = 0.0;
        for (const auto& road : roads) {
            total_length += road.length();
        }
        const double avg_len = total_length / static_cast<double>(roads.size());
        for (auto& road : roads) {
            if (road.type == RoadType::M_Major || road.type == RoadType::M_Minor) {
                continue;
            }
            road.type = classifyRoad(road, avg_len);
        }
    }

    void RoadClassifier::classifyGraph(Urban::Graph& graph, uint32_t centrality_samples) {
        if (graph.edges().empty()) {
            return;
        }

        const auto centrality = Urban::GraphAlgorithms::sampledEdgeCentrality(graph, centrality_samples, 2026u);
        float max_len = 0.0f;
        for (const auto& edge : graph.edges()) {
            max_len = std::max(max_len, edge.length);
        }
        max_len = std::max(1.0f, max_len);

        for (Urban::EdgeID eid = 0; eid < graph.edges().size(); ++eid) {
            auto* edge = graph.getEdgeMutable(eid);
            if (edge == nullptr) {
                continue;
            }
            const auto* a = graph.getVertex(edge->a);
            const auto* b = graph.getVertex(edge->b);
            const float endpoint_degree = (a != nullptr && b != nullptr)
                ? static_cast<float>(a->edges.size() + b->edges.size()) * 0.5f
                : 2.0f;
            const float length_norm = std::clamp(edge->length / max_len, 0.0f, 1.0f);
            const float cent = (eid < centrality.size()) ? centrality[eid] : 0.0f;
            const float degree_norm = std::clamp((endpoint_degree - 1.0f) / 4.0f, 0.0f, 1.0f);

            const float score = (0.45f * cent) + (0.35f * length_norm) + (0.20f * degree_norm);
            edge->type = classifyScore(score, length_norm, cent, endpoint_degree);
        }
    }

    RoadType RoadClassifier::classifyRoad(const Road& road, double avg_length) {
        const double len = road.length();
        if (len >= avg_length * 2.2) {
            return RoadType::Arterial;
        }
        if (len >= avg_length * 1.5) {
            return RoadType::Avenue;
        }
        if (len >= avg_length * 0.8) {
            return RoadType::Street;
        }
        return RoadType::Lane;
    }

    RoadType RoadClassifier::classifyScore(
        float score,
        float length_norm,
        float centrality,
        float endpoint_degree) {
        if (score >= 0.86f || (centrality > 0.90f && endpoint_degree >= 3.8f)) {
            return RoadType::Highway;
        }
        if (score >= 0.70f || (length_norm > 0.75f && centrality > 0.55f)) {
            return RoadType::Arterial;
        }
        if (score >= 0.56f) {
            return RoadType::Avenue;
        }
        if (score >= 0.40f) {
            return RoadType::Street;
        }
        if (score >= 0.25f) {
            return RoadType::Lane;
        }
        return RoadType::Drive;
    }

} // namespace RogueCity::Generators
