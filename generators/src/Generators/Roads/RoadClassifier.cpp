#include "RogueCity/Generators/Roads/RoadClassifier.hpp"

#include "RogueCity/Generators/Urban/GraphAlgorithms.hpp"
#include "RogueCity/Generators/Scoring/RogueProfiler.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>

namespace RogueCity::Generators {

    namespace {

        [[nodiscard]] float roadLength(const Road& road) {
            return static_cast<float>(road.length());
        }

        [[nodiscard]] int roadRank(Core::RoadType type) {
            switch (type) {
                case Core::RoadType::Highway: return 9;
                case Core::RoadType::Arterial: return 8;
                case Core::RoadType::Avenue: return 7;
                case Core::RoadType::Boulevard: return 6;
                case Core::RoadType::Street: return 5;
                case Core::RoadType::Lane: return 4;
                case Core::RoadType::Alleyway: return 3;
                case Core::RoadType::CulDeSac: return 2;
                case Core::RoadType::Drive: return 1;
                case Core::RoadType::Driveway: return 0;
                case Core::RoadType::M_Major: return 8;
                case Core::RoadType::M_Minor: return 5;
                default: return 0;
            }
        }

        [[nodiscard]] Core::RoadType bestAespType(float access_target, float exposure_target) {
            static const std::array<Core::RoadType, 10> kTypes = {
                Core::RoadType::Highway,
                Core::RoadType::Arterial,
                Core::RoadType::Avenue,
                Core::RoadType::Boulevard,
                Core::RoadType::Street,
                Core::RoadType::Lane,
                Core::RoadType::Alleyway,
                Core::RoadType::CulDeSac,
                Core::RoadType::Drive,
                Core::RoadType::Driveway
            };

            Core::RoadType best = Core::RoadType::Street;
            float best_score = std::numeric_limits<float>::max();
            for (const auto type : kTypes) {
                const float access = RogueProfiler::roadTypeToAccess(type);
                const float exposure = RogueProfiler::roadTypeToExposure(type);
                const float diff = std::abs(access - access_target) + std::abs(exposure - exposure_target);
                if (diff < best_score) {
                    best_score = diff;
                    best = type;
                }
            }
            return best;
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
            const auto base_type = classifyScore(score, length_norm, cent, endpoint_degree);

            const float access_target = std::clamp(0.45f * cent + 0.35f * degree_norm + 0.20f * length_norm, 0.0f, 1.0f);
            const float exposure_target = std::clamp(0.60f * cent + 0.40f * length_norm, 0.0f, 1.0f);
            const auto aesp_type = bestAespType(access_target, exposure_target);

            const float base_access = RogueProfiler::roadTypeToAccess(base_type);
            const float base_exposure = RogueProfiler::roadTypeToExposure(base_type);
            const float aesp_access = RogueProfiler::roadTypeToAccess(aesp_type);
            const float aesp_exposure = RogueProfiler::roadTypeToExposure(aesp_type);
            const float base_diff = std::abs(base_access - access_target) + std::abs(base_exposure - exposure_target);
            const float aesp_diff = std::abs(aesp_access - access_target) + std::abs(aesp_exposure - exposure_target);

            if (aesp_diff + 0.08f < base_diff && roadRank(aesp_type) >= roadRank(base_type) - 1) {
                edge->type = aesp_type;
            } else {
                edge->type = base_type;
            }
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
