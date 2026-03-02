#include "RogueCity/Generators/Urban/BlockGenerator.hpp"
#include "RogueCity/Generators/Urban/PolygonFinder.hpp"

namespace RogueCity::Generators::Urban {

    // Convenience overload with default block-generation config.
    std::vector<Core::BlockPolygon> BlockGenerator::generate(
        const std::vector<Core::District>& districts) {
        return generate(districts, Config{});
    }

    // Config-aware overload without road graph: always uses district-derived blocks.
    // When prefer_road_cycles is requested but no graph is available, the caller should
    // use the three-argument overload that accepts a pre-built Graph.
    std::vector<Core::BlockPolygon> BlockGenerator::generate(
        const std::vector<Core::District>& districts,
        const Config& config) {
        // No road graph provided — cannot do road-cycle extraction regardless of preference.
        // Documented fallback: district polygons are the stable fallback block source.
        (void)config; // prefer_road_cycles has no effect without graph data
        return PolygonFinder::fromDistricts(districts);
    }

    // Road-cycle aware overload: when prefer_road_cycles is true and the graph has edges,
    // calls PolygonFinder::fromGraph to filter/validate blocks against the road network.
    // This eliminates the silent no-op where both branches returned the same result.
    std::vector<Core::BlockPolygon> BlockGenerator::generate(
        const std::vector<Core::District>& districts,
        const Graph& road_graph,
        const Config& config) {
        if (config.prefer_road_cycles && !road_graph.edges().empty()) {
            // Road-cycle path: validate district blocks against graph edge coverage.
            // PolygonFinder::fromGraph returns blocks filtered to those with road backing,
            // which can differ from fromDistricts when some districts lack road coverage.
            return PolygonFinder::fromGraph(road_graph, districts);
        }
        // Fallback: road-cycle preference is off, or graph is empty (e.g. early pipeline stage).
        return PolygonFinder::fromDistricts(districts);
    }

} // namespace RogueCity::Generators::Urban
