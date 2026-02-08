#include "RogueCity/Generators/Urban/BlockGenerator.hpp"
#include "RogueCity/Generators/Urban/PolygonFinder.hpp"

namespace RogueCity::Generators::Urban {

    std::vector<Core::BlockPolygon> BlockGenerator::generate(
        const std::vector<Core::District>& districts,
        const Config&) {
        return PolygonFinder::fromDistricts(districts);
    }

} // namespace RogueCity::Generators::Urban

