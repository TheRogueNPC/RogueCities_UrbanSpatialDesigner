#include "RogueCity/Generators/Urban/PolygonFinder.hpp"
#include "RogueCity/Generators/Urban/PolygonUtil.hpp"

namespace RogueCity::Generators::Urban {

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

    std::vector<Core::BlockPolygon> PolygonFinder::fromGraph(
        const Graph&,
        const std::vector<Core::District>& districts) {
        // Road-cycle extraction can be layered in later. For Phase 2, district
        // polygons are the stable fallback block source.
        return fromDistricts(districts);
    }

} // namespace RogueCity::Generators::Urban

