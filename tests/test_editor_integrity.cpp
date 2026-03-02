#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Core/Validation/EditorIntegrity.hpp"

#include <iostream>
#include <string_view>

#define RC_EXPECT(expr)          \
    do {                         \
        if (!(expr)) {           \
            std::cerr << "RC_EXPECT failed at line " << __LINE__ << ": " << #expr << '\n'; \
            return 1;            \
        }                        \
    } while (false)

namespace {

using RogueCity::Core::BuildingSite;
using RogueCity::Core::District;
using RogueCity::Core::LotToken;
using RogueCity::Core::PlanViolation;
using RogueCity::Core::Road;
using RogueCity::Core::Vec2;
using RogueCity::Core::Editor::GlobalState;
using RogueCity::Core::Validation::SpatialCheckAll;
using RogueCity::Core::Validation::ValidateAll;

constexpr std::string_view kEntityPrefix = "[Integrity/Entity] ";
constexpr std::string_view kSpatialPrefix = "[Integrity/Spatial] ";

size_t CountTagged(const std::vector<PlanViolation>& violations, std::string_view prefix) {
    size_t count = 0u;
    for (const auto& violation : violations) {
        if (violation.message.rfind(prefix.data(), 0u) == 0u) {
            ++count;
        }
    }
    return count;
}

bool ContainsTaggedMessage(
    const std::vector<PlanViolation>& violations,
    std::string_view prefix,
    std::string_view message_fragment) {
    for (const auto& violation : violations) {
        const bool prefix_match = prefix.empty() || violation.message.rfind(prefix.data(), 0u) == 0u;
        if (prefix_match && violation.message.find(message_fragment) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    GlobalState gs{};

    Road malformed_road{};
    malformed_road.id = 0u;
    malformed_road.points = { Vec2(0.0, 0.0) };
    gs.roads.add(malformed_road);

    District malformed_district{};
    malformed_district.id = 0u;
    malformed_district.border = { Vec2(0.0, 0.0), Vec2(1.0, 1.0) };
    gs.districts.add(malformed_district);

    LotToken malformed_lot{};
    malformed_lot.id = 0u;
    malformed_lot.district_id = 0u;
    malformed_lot.centroid = Vec2(0.0, 0.0);
    malformed_lot.boundary = { Vec2(0.0, 0.0), Vec2(1.0, 0.0) };
    gs.lots.add(malformed_lot);

    BuildingSite malformed_building{};
    malformed_building.id = 0u;
    malformed_building.lot_id = 0u;
    malformed_building.position = Vec2(5.0, 5.0);
    gs.buildings.push_back(malformed_building);

    ValidateAll(gs);
    const size_t entity_violations = CountTagged(gs.plan_violations, kEntityPrefix);
    RC_EXPECT(entity_violations > 0u);
    RC_EXPECT(ContainsTaggedMessage(gs.plan_violations, kEntityPrefix, "Road"));

    ValidateAll(gs);
    RC_EXPECT(CountTagged(gs.plan_violations, kEntityPrefix) == entity_violations);

    Road road_a{};
    road_a.id = 10u;
    road_a.points = { Vec2(0.0, 0.0), Vec2(10.0, 10.0) };
    gs.roads.add(road_a);

    Road road_b{};
    road_b.id = 11u;
    road_b.points = { Vec2(10.0, 0.0), Vec2(0.0, 10.0) };
    gs.roads.add(road_b);

    BuildingSite orphan{};
    orphan.id = 11u;
    orphan.lot_id = 999u;
    orphan.position = Vec2(2.0, 2.0);
    gs.buildings.push_back(orphan);

    PlanViolation preserved{};
    preserved.message = "External violation";
    gs.plan_violations.push_back(preserved);

    SpatialCheckAll(gs);
    const size_t spatial_violations = CountTagged(gs.plan_violations, kSpatialPrefix);
    RC_EXPECT(spatial_violations > 0u);
    RC_EXPECT(ContainsTaggedMessage(gs.plan_violations, kSpatialPrefix, "intersection"));
    RC_EXPECT(ContainsTaggedMessage(gs.plan_violations, kSpatialPrefix, "missing lot"));
    RC_EXPECT(ContainsTaggedMessage(gs.plan_violations, std::string_view{}, "External violation"));

    SpatialCheckAll(gs);
    RC_EXPECT(CountTagged(gs.plan_violations, kSpatialPrefix) == spatial_violations);

    return 0;
}

#undef RC_EXPECT
