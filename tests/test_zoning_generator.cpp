#include "RogueCity/Core/Data/CitySpec.hpp"
#include "RogueCity/Core/Geometry/PolygonOps.hpp"
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Pipeline/ZoningGenerator.hpp"
#include "RogueCity/Generators/Urban/PolygonUtil.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_map>

namespace {

using RogueCity::Core::BlockPolygon;
using RogueCity::Core::CitySpec;
using RogueCity::Core::District;
using RogueCity::Core::DistrictType;
using RogueCity::Core::LotToken;
using RogueCity::Core::Road;
using RogueCity::Core::RoadType;
using RogueCity::Core::Vec2;
using RogueCity::Core::Geometry::Polygon;
using RogueCity::Core::Geometry::PolygonOps;
using RogueCity::Generators::Urban::PolygonUtil::area;
using RogueCity::Generators::ZoningGenerator;

ZoningGenerator::ZoningInput BuildBasicInput() {
    ZoningGenerator::ZoningInput input{};

    Road road{};
    road.id = 1u;
    road.type = RoadType::Street;
    road.points = {Vec2(0.0, 50.0), Vec2(100.0, 50.0)};
    input.roads.add(road);

    District district{};
    district.id = 1u;
    district.type = DistrictType::Residential;
    district.border = {
        Vec2(0.0, 0.0),
        Vec2(100.0, 0.0),
        Vec2(100.0, 100.0),
        Vec2(0.0, 100.0),
    };
    input.districts.push_back(district);

    BlockPolygon block{};
    block.district_id = district.id;
    block.outer = district.border;
    input.blocks.push_back(block);

    return input;
}

float SumLotBudgets(const std::vector<LotToken>& lots) {
    float sum = 0.0f;
    for (const auto& lot : lots) {
        sum += lot.budget_allocation;
    }
    return sum;
}

bool NearlyEqual(float a, float b, float eps = 1e-3f) {
    return std::fabs(a - b) <= eps;
}

Polygon ToGeometryPolygon(const std::vector<Vec2>& ring) {
    Polygon poly{};
    poly.vertices = ring;
    return poly;
}

double TotalPolygonArea(const std::vector<Polygon>& polygons) {
    double sum = 0.0;
    for (const auto& polygon : polygons) {
        sum += std::abs(area(polygon.vertices));
    }
    return sum;
}

struct Bounds {
    double min_x = 0.0;
    double max_x = 0.0;
    double min_y = 0.0;
    double max_y = 0.0;
};

Bounds ComputeLotBounds(const LotToken& lot) {
    Bounds bounds{};
    if (lot.boundary.empty()) {
        bounds.min_x = lot.centroid.x;
        bounds.max_x = lot.centroid.x;
        bounds.min_y = lot.centroid.y;
        bounds.max_y = lot.centroid.y;
        return bounds;
    }

    bounds.min_x = lot.boundary.front().x;
    bounds.max_x = lot.boundary.front().x;
    bounds.min_y = lot.boundary.front().y;
    bounds.max_y = lot.boundary.front().y;
    for (const auto& point : lot.boundary) {
        bounds.min_x = std::min(bounds.min_x, point.x);
        bounds.max_x = std::max(bounds.max_x, point.x);
        bounds.min_y = std::min(bounds.min_y, point.y);
        bounds.max_y = std::max(bounds.max_y, point.y);
    }
    return bounds;
}

Bounds ComputeInsetBounds(
    const LotToken& lot,
    float side_setback,
    float front_setback,
    float rear_setback) {
    Bounds bounds = ComputeLotBounds(lot);

    const double side = std::max(0.0, static_cast<double>(side_setback));
    const double front = std::max(0.0, static_cast<double>(front_setback));
    const double rear = std::max(0.0, static_cast<double>(rear_setback));

    bounds.min_x += side;
    bounds.max_x -= side;
    if (bounds.min_x > bounds.max_x) {
        const double center_x = (bounds.min_x + bounds.max_x) * 0.5;
        bounds.min_x = center_x;
        bounds.max_x = center_x;
    }

    bounds.min_y += front;
    bounds.max_y -= rear;
    if (bounds.min_y > bounds.max_y) {
        const double center_y = (bounds.min_y + bounds.max_y) * 0.5;
        bounds.min_y = center_y;
        bounds.max_y = center_y;
    }

    return bounds;
}

} // namespace

#define RC_EXPECT(expr)                                                       \
    do {                                                                      \
        if (!(expr)) {                                                        \
            std::cerr << "RC_EXPECT failed: " << #expr                        \
                      << " (line " << __LINE__ << ")" << std::endl;          \
            return 1;                                                         \
        }                                                                     \
    } while (false)

int main() {
    ZoningGenerator generator{};
    ZoningGenerator::Config config{};
    config.minLotWidth = 10.0f;
    config.maxLotWidth = 10.0f;
    config.minLotDepth = 10.0f;
    config.maxLotDepth = 10.0f;
    config.maxLots = 300u;
    config.maxBuildings = 600u;
    config.totalBuildingBudget = 1000.0f;
    config.seed = 1337u;

    {
        auto input = BuildBasicInput();
        const auto output = generator.generate(input, config);
        RC_EXPECT(!output.lots.empty());

        const float lot_budget_sum = SumLotBudgets(output.lots);
        RC_EXPECT(lot_budget_sum <= config.totalBuildingBudget + 1e-3f);
        RC_EXPECT(output.totalBudgetUsed <= config.totalBuildingBudget + 1e-3f);
        RC_EXPECT(NearlyEqual(lot_budget_sum, output.totalBudgetUsed, 1e-2f));

        for (const auto& lot : output.lots) {
            RC_EXPECT(lot.budget_allocation >= 0.0f);
        }
    }

    {
        auto input = BuildBasicInput();
        auto zero_budget_config = config;
        zero_budget_config.totalBuildingBudget = 0.0f;

        const auto output = generator.generate(input, zero_budget_config);
        const float lot_budget_sum = SumLotBudgets(output.lots);
        RC_EXPECT(NearlyEqual(lot_budget_sum, 0.0f));
        RC_EXPECT(NearlyEqual(output.totalBudgetUsed, 0.0f));
    }

    {
        auto input = BuildBasicInput();
        CitySpec spec{};
        spec.buildingBudget.totalBudget = 37.0f;
        input.citySpec = spec;

        auto ui_config_budget = config;
        ui_config_budget.totalBuildingBudget = 1000.0f;

        const auto output = generator.generate(input, ui_config_budget);
        RC_EXPECT(!output.lots.empty());

        const float lot_budget_sum = SumLotBudgets(output.lots);
        RC_EXPECT(lot_budget_sum <= spec.buildingBudget.totalBudget + 1e-3f);
        RC_EXPECT(output.totalBudgetUsed <= spec.buildingBudget.totalBudget + 1e-3f);
        RC_EXPECT(NearlyEqual(lot_budget_sum, output.totalBudgetUsed, 1e-2f));
    }

    {
        auto input = BuildBasicInput();

        auto low_density = config;
        low_density.residentialDensity = 0.15f;
        low_density.commercialDensity = 0.15f;
        low_density.industrialDensity = 0.15f;
        low_density.civicDensity = 0.15f;

        auto high_density = config;
        high_density.residentialDensity = 0.95f;
        high_density.commercialDensity = 0.95f;
        high_density.industrialDensity = 0.95f;
        high_density.civicDensity = 0.95f;

        const auto low_output = generator.generate(input, low_density);
        const auto high_output = generator.generate(input, high_density);

        RC_EXPECT(!low_output.lots.empty());
        RC_EXPECT(!high_output.lots.empty());
        RC_EXPECT(high_output.buildings.size() > low_output.buildings.size());
    }

    {
        auto input = BuildBasicInput();

        auto setback_cfg = config;
        setback_cfg.residentialDensity = 1.0f;
        setback_cfg.commercialDensity = 1.0f;
        setback_cfg.industrialDensity = 1.0f;
        setback_cfg.civicDensity = 1.0f;
        setback_cfg.sideSetback = 2.0f;
        setback_cfg.frontSetback = 1.5f;
        setback_cfg.rearSetback = 3.0f;

        const auto output = generator.generate(input, setback_cfg);
        RC_EXPECT(!output.lots.empty());
        RC_EXPECT(!output.buildings.empty());

        std::unordered_map<uint32_t, const LotToken*> lots_by_id;
        lots_by_id.reserve(output.lots.size());
        for (const auto& lot : output.lots) {
            lots_by_id[lot.id] = &lot;
        }

        constexpr double kEps = 1e-6;
        for (const auto& building : output.buildings) {
            const auto lot_it = lots_by_id.find(building.lot_id);
            RC_EXPECT(lot_it != lots_by_id.end());

            const Bounds inset = ComputeInsetBounds(
                *lot_it->second,
                setback_cfg.sideSetback,
                setback_cfg.frontSetback,
                setback_cfg.rearSetback);
            RC_EXPECT(building.position.x >= inset.min_x - kEps);
            RC_EXPECT(building.position.x <= inset.max_x + kEps);
            RC_EXPECT(building.position.y >= inset.min_y - kEps);
            RC_EXPECT(building.position.y <= inset.max_y + kEps);
        }
    }

    {
        auto input = BuildBasicInput();
        CitySpec spec{};
        spec.zoningConstraints.residentialDensity = 0.05f;
        spec.zoningConstraints.commercialDensity = 0.05f;
        spec.zoningConstraints.industrialDensity = 0.05f;
        spec.zoningConstraints.civicDensity = 0.05f;
        input.citySpec = spec;

        auto high_density = config;
        high_density.residentialDensity = 1.0f;
        high_density.commercialDensity = 1.0f;
        high_density.industrialDensity = 1.0f;
        high_density.civicDensity = 1.0f;

        const auto overridden_output = generator.generate(input, high_density);

        auto baseline_input = BuildBasicInput();
        const auto baseline_output = generator.generate(baseline_input, high_density);

        RC_EXPECT(!baseline_output.buildings.empty());
        RC_EXPECT(overridden_output.buildings.size() < baseline_output.buildings.size());
    }

    {
        auto input = BuildBasicInput();
        CitySpec spec{};
        spec.zoningConstraints.sideSetback = 3.0f;
        spec.zoningConstraints.frontSetback = 2.0f;
        spec.zoningConstraints.rearSetback = 4.0f;
        spec.zoningConstraints.residentialDensity = 1.0f;
        spec.zoningConstraints.commercialDensity = 1.0f;
        spec.zoningConstraints.industrialDensity = 1.0f;
        spec.zoningConstraints.civicDensity = 1.0f;
        input.citySpec = spec;

        auto ui_setback_cfg = config;
        ui_setback_cfg.sideSetback = 0.0f;
        ui_setback_cfg.frontSetback = 0.0f;
        ui_setback_cfg.rearSetback = 0.0f;
        ui_setback_cfg.residentialDensity = 1.0f;
        ui_setback_cfg.commercialDensity = 1.0f;
        ui_setback_cfg.industrialDensity = 1.0f;
        ui_setback_cfg.civicDensity = 1.0f;

        const auto output = generator.generate(input, ui_setback_cfg);
        RC_EXPECT(!output.buildings.empty());

        std::unordered_map<uint32_t, const LotToken*> lots_by_id;
        lots_by_id.reserve(output.lots.size());
        for (const auto& lot : output.lots) {
            lots_by_id[lot.id] = &lot;
        }

        constexpr double kEps = 1e-6;
        for (const auto& building : output.buildings) {
            const auto lot_it = lots_by_id.find(building.lot_id);
            RC_EXPECT(lot_it != lots_by_id.end());

            const Bounds inset = ComputeInsetBounds(
                *lot_it->second,
                spec.zoningConstraints.sideSetback,
                spec.zoningConstraints.frontSetback,
                spec.zoningConstraints.rearSetback);
            RC_EXPECT(building.position.x >= inset.min_x - kEps);
            RC_EXPECT(building.position.x <= inset.max_x + kEps);
            RC_EXPECT(building.position.y >= inset.min_y - kEps);
            RC_EXPECT(building.position.y <= inset.max_y + kEps);
        }
    }

    {
        auto input = BuildBasicInput();
        input.blocks.clear();

        BlockPolygon holed_block{};
        holed_block.district_id = input.districts.front().id;
        holed_block.outer = {
            Vec2(0.0, 0.0),
            Vec2(120.0, 0.0),
            Vec2(120.0, 120.0),
            Vec2(0.0, 120.0),
        };
        holed_block.holes.push_back({
            Vec2(45.0, 45.0),
            Vec2(75.0, 45.0),
            Vec2(75.0, 75.0),
            Vec2(45.0, 75.0),
        });
        input.blocks.push_back(holed_block);

        auto hole_cfg = config;
        hole_cfg.minLotWidth = 12.0f;
        hole_cfg.maxLotWidth = 12.0f;
        hole_cfg.minLotDepth = 12.0f;
        hole_cfg.maxLotDepth = 12.0f;
        hole_cfg.minLotArea = 20.0f;
        hole_cfg.maxLotArea = 500.0f;
        hole_cfg.maxLots = 600u;
        hole_cfg.maxBuildings = 900u;
        hole_cfg.residentialDensity = 1.0f;
        hole_cfg.commercialDensity = 1.0f;
        hole_cfg.industrialDensity = 1.0f;
        hole_cfg.civicDensity = 1.0f;

        const auto output = generator.generate(input, hole_cfg);
        RC_EXPECT(!output.lots.empty());

        const Polygon outer_poly = ToGeometryPolygon(holed_block.outer);
        const Polygon hole_poly = ToGeometryPolygon(holed_block.holes.front());
        constexpr double kAreaEps = 0.5;
        for (const auto& lot : output.lots) {
            const Polygon lot_poly = ToGeometryPolygon(lot.boundary);
            RC_EXPECT(PolygonOps::IsValidPolygon(lot_poly));

            const double lot_area = std::abs(area(lot.boundary));
            RC_EXPECT(lot_area >= static_cast<double>(hole_cfg.minLotArea) - kAreaEps);
            RC_EXPECT(lot_area <= static_cast<double>(hole_cfg.maxLotArea) + kAreaEps);

            const auto overlap_hole = PolygonOps::ClipPolygons(lot_poly, hole_poly);
            RC_EXPECT(TotalPolygonArea(overlap_hole) <= kAreaEps);

            const auto overlap_outer = PolygonOps::ClipPolygons(lot_poly, outer_poly);
            RC_EXPECT(std::abs(TotalPolygonArea(overlap_outer) - lot_area) <= kAreaEps);
        }
    }

    return 0;
}

#undef RC_EXPECT
