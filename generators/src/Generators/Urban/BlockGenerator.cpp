#include "RogueCity/Generators/Urban/BlockGenerator.hpp"
#include "RogueCity/Generators/Urban/PolygonFinder.hpp"

namespace RogueCity::Generators::Urban {

    // Convenience overload with default block-generation config.
    std::vector<Core::BlockPolygon> BlockGenerator::generate(
        const std::vector<Core::District>& districts) {
        return generate(districts, Config{});
    }

    // Current block strategy uses district polygons as canonical block shells.
    // Future implementations may layer road-cycle extraction behind this interface.
    std::vector<Core::BlockPolygon> BlockGenerator::generate(
        const std::vector<Core::District>& districts,
        const Config&) {
        return PolygonFinder::fromDistricts(districts);
    }

} // namespace RogueCity::Generators::Urban
