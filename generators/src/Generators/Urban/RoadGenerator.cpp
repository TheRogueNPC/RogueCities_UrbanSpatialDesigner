#include "RogueCity/Generators/Urban/RoadGenerator.hpp"
#include "RogueCity/Generators/Roads/IntersectionTemplates.hpp"
#include "RogueCity/Generators/Roads/PolylineRoadCandidate.hpp"
#include "RogueCity/Generators/Roads/RoadClassifier.hpp"
#include <cmath>

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

            // If this is a major-type candidate, enforce tensor-alignment
            // tolerance: compare the initial polyline heading with the sampled
            // tensor major-eigenvector at the seed point and drop the
            // candidate if it deviates beyond tolerance.
            if (!candidate.pts.empty()) {
                const double tol_deg = config.tracing.major_tensor_tolerance_degrees;
                if (tol_deg >= 0.0 && candidate.pts.size() > 1) {
                    const auto p0 = candidate.pts.front();
                    const auto p1 = candidate.pts[1];
                    Core::Vec2 dir = p1 - p0;
                    const double seg_len = dir.length();
                    if (seg_len > 1e-6) {
                        dir.normalize();
                        const Core::Tensor2D t = field.sampleTensor(p0);
                        Core::Vec2 major = t.majorEigenvector();
                        major.normalize();
                        // signed angle from dir -> major
                        const double ang_rad = Core::angleBetween(dir, major);
                        const double ang_deg = std::abs(ang_rad) * (180.0 / 3.14159265358979323846);

                        // If within tolerance, do nothing. If outside, softly blend
                        // the initial heading toward the tensor major direction.
                        if (ang_deg > tol_deg) {
                            // Normalize excess into [0,1] over remaining 90deg span
                            const double max_excess_deg = std::max(1.0, 90.0 - tol_deg);
                            const double excess_deg = std::min(ang_deg - tol_deg, max_excess_deg);
                            const double norm = std::clamp(excess_deg / max_excess_deg, 0.0, 1.0);

                            // Blend strength stronger for major hints, weaker for others
                            const double base_strength = candidate.is_major_hint ? 1.0 : 0.4;
                            const double blend = std::clamp(norm * base_strength, 0.0, 1.0);

                            // Rotate initial direction a fraction of the signed angle toward major
                            const double rotate_rad = ang_rad * blend;
                            const double c = std::cos(rotate_rad);
                            const double s = std::sin(rotate_rad);
                            Core::Vec2 newdir{ dir.x * c - dir.y * s, dir.x * s + dir.y * c };
                            newdir.normalize();

                            // Replace the second point to nudge the polyline heading
                            candidate.pts[1] = Core::Vec2(p0.x + newdir.x * seg_len, p0.y + newdir.y * seg_len);
                        }
                    }
                }
            }
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
