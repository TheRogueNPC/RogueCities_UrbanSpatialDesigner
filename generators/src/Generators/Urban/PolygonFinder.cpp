#include "RogueCity/Generators/Urban/PolygonFinder.hpp"
#include "RogueCity/Generators/Urban/PolygonUtil.hpp"

namespace RogueCity::Generators::Urban {

    // Converts district borders into block polygons.
    // Borders are normalized to closed rings for downstream polygon operations.
    std::vector<Core::BlockPolygon> PolygonFinder::fromDistricts(
        const std::vector<Core::District>& districts) {
        std::vector<Core::BlockPolygon> blocks;
        blocks.reserve(districts.size());
        for (const auto& district : districts) {
            if (district.border.size() < 3) {
                continue;
            }
            Core::BlockPolygon block;
            block.district_id = district.id;
            block.outer = PolygonUtil::closed(district.border);
            blocks.push_back(std::move(block));
        }
        return blocks;
    }

    // Graph-based cycle extraction is intentionally deferred.
    // For now we rely on district polygons as a deterministic fallback.
    std::vector<Core::BlockPolygon> PolygonFinder::fromGraph(
        const Graph&,
        const std::vector<Core::District>& districts) {
        // Road-cycle extraction can be layered in later. For Phase 2, district
        // polygons are the stable fallback block source.
        return fromDistricts(districts);
    }

} // namespace RogueCity::Generators::Urban
