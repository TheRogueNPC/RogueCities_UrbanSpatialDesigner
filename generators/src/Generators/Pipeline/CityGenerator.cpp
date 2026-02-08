#define _USE_MATH_DEFINES
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <algorithm>
#include <cmath>

namespace RogueCity::Generators {

    CityGenerator::CityOutput CityGenerator::generate(
        const std::vector<AxiomInput>& axioms,
        const Config& config
    ) {
        config_ = config;
        rng_ = RNG(config_.seed);

        CityOutput output;

        // Stage 1: Generate tensor field from axioms
        output.tensor_field = generateTensorField(axioms);

        // Stage 2: Generate seed points for streamline tracing
        std::vector<Vec2> seeds = generateSeeds();

        // Stage 3: Trace road network
        output.roads = traceRoads(output.tensor_field, seeds);

        // Stage 4: Classify districts
        output.districts = classifyDistricts(output.roads);

        // Stage 5: Extract block polygons
        output.blocks = generateBlocks(output.districts);

        // Stage 6: Generate lots
        output.lots = generateLots(output.roads, output.districts, output.blocks);

        // Stage 7: Place buildings
        output.buildings = generateBuildings(output.lots);

        return output;
    }

    TensorFieldGenerator CityGenerator::generateTensorField(
        const std::vector<AxiomInput>& axioms
    ) {
        TensorFieldGenerator::Config field_config;
        field_config.width = static_cast<int>(config_.width / config_.cell_size);
        field_config.height = static_cast<int>(config_.height / config_.cell_size);
        field_config.cell_size = config_.cell_size;

        TensorFieldGenerator field(field_config);

        // Add basis fields from axioms
        for (const auto& axiom : axioms) {
            switch (axiom.type) {
                case AxiomInput::Type::Organic:
                    field.addOrganicField(axiom.position, axiom.radius, axiom.theta, axiom.organic_curviness, axiom.decay);
                    break;

                case AxiomInput::Type::Grid:
                    field.addGridField(axiom.position, axiom.radius, axiom.theta, axiom.decay);
                    break;

                case AxiomInput::Type::Radial:
                    field.addRadialField(axiom.position, axiom.radius, axiom.radial_spokes, axiom.decay);
                    break;

                case AxiomInput::Type::Hexagonal:
                    field.addHexagonalField(axiom.position, axiom.radius, axiom.theta, axiom.decay);
                    break;

                case AxiomInput::Type::Stem:
                    field.addStemField(axiom.position, axiom.radius, axiom.theta, axiom.stem_branch_angle, axiom.decay);
                    break;

                case AxiomInput::Type::LooseGrid:
                    field.addLooseGridField(axiom.position, axiom.radius, axiom.theta, axiom.loose_grid_jitter, axiom.decay);
                    break;

                case AxiomInput::Type::Suburban:
                    field.addSuburbanField(axiom.position, axiom.radius, axiom.suburban_loop_strength, axiom.decay);
                    break;

                case AxiomInput::Type::Superblock:
                    field.addSuperblockField(axiom.position, axiom.radius, axiom.theta, axiom.superblock_block_size, axiom.decay);
                    break;

                case AxiomInput::Type::Linear:
                    field.addLinearField(axiom.position, axiom.radius, axiom.theta, axiom.decay);
                    break;

                case AxiomInput::Type::GridCorrective:
                    field.addGridCorrective(axiom.position, axiom.radius, axiom.theta, axiom.decay);
                    break;

                case AxiomInput::Type::COUNT:
                    break;
            }
        }

        // Generate field
        field.generateField();

        return field;
    }

    std::vector<Vec2> CityGenerator::generateSeeds() {
        std::vector<Vec2> seeds;
        seeds.reserve(config_.num_seeds);

        // Generate random seed points within city bounds
        for (int i = 0; i < config_.num_seeds; ++i) {
            Vec2 seed(
                rng_.uniform(0.0, static_cast<double>(config_.width)),
                rng_.uniform(0.0, static_cast<double>(config_.height))
            );
            seeds.push_back(seed);
        }

        return seeds;
    }

    fva::Container<Road> CityGenerator::traceRoads(
        const TensorFieldGenerator& field,
        const std::vector<Vec2>& seeds
    ) {
        Urban::RoadGenerator::Config road_cfg;
        road_cfg.tracing.step_size = 5.0;
        road_cfg.tracing.max_length = 800.0;
        road_cfg.tracing.bidirectional = true;
        road_cfg.tracing.max_iterations = 500;
        return Urban::RoadGenerator::generate(seeds, field, road_cfg);
    }

    std::vector<District> CityGenerator::classifyDistricts(
        const fva::Container<Road>& roads
    ) {
        Urban::DistrictGenerator::Config district_cfg;
        district_cfg.grid_resolution = std::clamp(
            static_cast<int>(std::sqrt(std::max<size_t>(4, roads.size() / 4))),
            4, 14);
        district_cfg.max_districts = config_.max_districts;

        Bounds bounds;
        bounds.min = Vec2(0.0, 0.0);
        bounds.max = Vec2(static_cast<double>(config_.width), static_cast<double>(config_.height));
        return Urban::DistrictGenerator::generate(roads, bounds, district_cfg);
    }

    std::vector<BlockPolygon> CityGenerator::generateBlocks(
        const std::vector<District>& districts
    ) {
        Urban::BlockGenerator::Config block_cfg;
        block_cfg.prefer_road_cycles = false;
        return Urban::BlockGenerator::generate(districts, block_cfg);
    }

    std::vector<LotToken> CityGenerator::generateLots(
        const fva::Container<Road>& roads,
        const std::vector<District>& districts,
        const std::vector<BlockPolygon>& blocks
    ) {
        Urban::LotGenerator::Config lot_cfg;
        lot_cfg.min_lot_width = 10.0f;
        lot_cfg.max_lot_width = 45.0f;
        lot_cfg.min_lot_depth = 12.0f;
        lot_cfg.max_lot_depth = 55.0f;
        lot_cfg.max_lots = config_.max_lots;
        return Urban::LotGenerator::generate(roads, districts, blocks, lot_cfg, config_.seed);
    }

    siv::Vector<BuildingSite> CityGenerator::generateBuildings(const std::vector<LotToken>& lots) {
        Urban::SiteGenerator::Config site_cfg;
        site_cfg.max_buildings = config_.max_buildings;
        return Urban::SiteGenerator::generate(lots, site_cfg, config_.seed);
    }

} // namespace RogueCity::Generators
