#include "RogueCity/Generators/Urban/RoadGenerator.hpp"
#include "RogueCity/Generators/Roads/IntersectionTemplates.hpp"
#include "RogueCity/Generators/Roads/PolylineRoadCandidate.hpp"
#include "RogueCity/Generators/Roads/RoadClassifier.hpp"

namespace RogueCity::Generators::Urban {

    // Convenience overload with default road generation config.
    fva::Container<Core::Road> RoadGenerator::generate(
        const std::vector<Core::Vec2>& seeds,
        const TensorFieldGenerator& field) {
        return generate(seeds, field, Config{});
    }

    // Road pipeline:
    // 1) trace streamlines
    // 2) build graph via noding
    // 3) simplify/classify/enrich graph
    // 4) emit final road polylines
    fva::Container<Core::Road> RoadGenerator::generate(
        const std::vector<Core::Vec2>& seeds,
        const TensorFieldGenerator& field,
        const Config& config) {
        StreamlineTracer tracer;
        const auto traced = tracer.traceNetwork(seeds, field, config.tracing);
        fva::Container<Core::Road> output;
        if (traced.size() == 0) {
            return output;
        }

        // Convert traced polylines into noder candidates.
        std::vector<Roads::PolylineRoadCandidate> candidates;
        candidates.reserve(traced.size());
        int seed_id = 0;
        for (const auto& road : traced) {
            if (road.points.size() < 2) {
                continue;
            }
            Roads::PolylineRoadCandidate candidate{};
            candidate.type_hint = road.type;
            candidate.is_major_hint =
                road.type == Core::RoadType::M_Major ||
                road.type == Core::RoadType::Highway ||
                road.type == Core::RoadType::Arterial;
            candidate.seed_id = seed_id++;
            candidate.layer_hint = 0;
            candidate.pts = road.points;
            candidates.push_back(std::move(candidate));
        }
        if (candidates.empty()) {
            return output;
        }

        // Build and post-process topology graph.
        Roads::RoadNoder noder(config.noder);
        Graph graph;
        noder.buildGraph(candidates, graph);

        Roads::simplifyGraph(graph, config.simplify);
        RoadClassifier::classifyGraph(graph, config.centrality_samples);
        Roads::applyFlowAndControl(graph, config.flow_control);
        if (config.enable_verticality) {
            Roads::applyVerticality(graph, config.verticality);
        }
        const auto template_output = Roads::emitIntersectionTemplates(graph, Roads::TemplateConfig{});
        (void)template_output;

        // Export graph edges back into road records expected by downstream stages.
        uint32_t next_id = 0;
        for (const auto& edge : graph.edges()) {
            if (edge.shape.size() < 2) {
                continue;
            }
            Core::Road road{};
            road.id = next_id++;
            road.type = edge.type;
            road.points = edge.shape;
            road.is_user_created = (road.type == Core::RoadType::M_Major || road.type == Core::RoadType::M_Minor);
            output.add(std::move(road));
        }

        return output;
    }

} // namespace RogueCity::Generators::Urban
