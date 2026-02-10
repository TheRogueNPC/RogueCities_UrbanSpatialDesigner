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

        // Stage 0: Analyze world constraints before tensor + roads.
        if (config_.enable_world_constraints) {
            TerrainConstraintGenerator terrain;
            TerrainConstraintGenerator::Input terrain_input;
            terrain_input.world_width = config_.width;
            terrain_input.world_height = config_.height;
            terrain_input.cell_size = config_.cell_size;
            terrain_input.seed = config_.seed;
            auto terrain_output = terrain.generate(terrain_input, config_.terrain);
            output.world_constraints = std::move(terrain_output.constraints);
            output.site_profile = terrain_output.profile;
        }

        // Stage 1: Generate tensor field from axioms
        output.tensor_field = generateTensorField(axioms);

        // Stage 2: Generate seed points for streamline tracing
        const WorldConstraintField* constraints = output.world_constraints.isValid() ? &output.world_constraints : nullptr;
        const SiteProfile* site_profile = constraints != nullptr ? &output.site_profile : nullptr;
        std::vector<Vec2> seeds = generateSeeds(constraints);

        // Stage 3: Trace road network
        output.roads = traceRoads(output.tensor_field, seeds, constraints, site_profile);

        // Stage 4: Classify districts
        output.districts = classifyDistricts(output.roads, constraints);

        // Stage 5: Extract block polygons
        output.blocks = generateBlocks(output.districts);

        // Stage 6: Generate lots
        output.lots = generateLots(output.roads, output.districts, output.blocks, site_profile);

        // Stage 7: Place buildings
        output.buildings = generateBuildings(output.lots, site_profile);

        // Stage 8: Plan validation against world constraints.
        if (constraints != nullptr) {
            PlanValidatorGenerator validator;
            PlanValidatorGenerator::Input validator_input;
            validator_input.constraints = constraints;
            validator_input.site_profile = site_profile;
            validator_input.roads = &output.roads;
            validator_input.lots = &output.lots;
            auto validation = validator.validate(validator_input, config_.validator);
            output.plan_violations = std::move(validation.violations);
            output.plan_approved = validation.approved;
        }

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

    std::vector<Vec2> CityGenerator::generateSeeds(const WorldConstraintField* constraints) {
        std::vector<Vec2> seeds;
        seeds.reserve(config_.num_seeds);

        const int max_attempts = std::max(8, config_.num_seeds * 25);
        int attempts = 0;
        while (static_cast<int>(seeds.size()) < config_.num_seeds && attempts < max_attempts) {
            ++attempts;
            Vec2 seed(
                rng_.uniform(0.0, static_cast<double>(config_.width)),
                rng_.uniform(0.0, static_cast<double>(config_.height))
            );

            if (constraints != nullptr && constraints->isValid()) {
                if (constraints->sampleNoBuild(seed)) {
                    continue;
                }
                if (constraints->sampleFloodMask(seed) > 1u) {
                    continue;
                }
            }
            seeds.push_back(seed);
        }

        if (seeds.empty()) {
            seeds.push_back(Vec2(
                static_cast<double>(config_.width) * 0.5,
                static_cast<double>(config_.height) * 0.5));
        }

        return seeds;
    }

    fva::Container<Road> CityGenerator::traceRoads(
        const TensorFieldGenerator& field,
        const std::vector<Vec2>& seeds,
        const WorldConstraintField* constraints,
        const SiteProfile* profile
    ) {
        Urban::RoadGenerator::Config road_cfg;
        road_cfg.tracing.step_size = 5.0;
        road_cfg.tracing.max_length = 800.0;
        road_cfg.tracing.bidirectional = true;
        road_cfg.tracing.max_iterations = 500;

        if (constraints != nullptr && constraints->isValid()) {
            road_cfg.tracing.constraints = constraints;
            road_cfg.tracing.max_slope_degrees = 30.0;
            road_cfg.tracing.max_flood_level = 1u;
            road_cfg.tracing.min_soil_strength = 0.14f;
            if (profile != nullptr && profile->mode == GenerationMode::ConservationOnly) {
                road_cfg.tracing.max_length = 520.0;
                road_cfg.tracing.max_flood_level = 0u;
            } else if (profile != nullptr && profile->mode == GenerationMode::HillTown) {
                road_cfg.tracing.max_slope_degrees = 34.0;
            }
        }

        return Urban::RoadGenerator::generate(seeds, field, road_cfg);
    }

    std::vector<District> CityGenerator::classifyDistricts(
        const fva::Container<Road>& roads,
        const WorldConstraintField* constraints
    ) {
        Urban::DistrictGenerator::Config district_cfg;
        district_cfg.grid_resolution = std::clamp(
            static_cast<int>(std::sqrt(std::max<size_t>(4, roads.size() / 4))),
            4, 14);
        district_cfg.max_districts = config_.max_districts;

        Bounds bounds;
        bounds.min = Vec2(0.0, 0.0);
        bounds.max = Vec2(static_cast<double>(config_.width), static_cast<double>(config_.height));
        auto districts = Urban::DistrictGenerator::generate(roads, bounds, district_cfg);

        if (constraints != nullptr && constraints->isValid()) {
            for (auto& district : districts) {
                if (district.border.empty()) {
                    continue;
                }
                Vec2 centroid{};
                for (const auto& p : district.border) {
                    centroid += p;
                }
                centroid /= static_cast<double>(district.border.size());

                const uint8_t tags = constraints->sampleHistoryTags(centroid);
                if (HasHistoryTag(tags, HistoryTag::SacredSite)) {
                    district.type = DistrictType::Civic;
                } else if (HasHistoryTag(tags, HistoryTag::Brownfield) ||
                    HasHistoryTag(tags, HistoryTag::Contaminated)) {
                    district.type = DistrictType::Industrial;
                }

                const float nature = constraints->sampleNatureScore(centroid);
                const float slope = constraints->sampleSlopeDegrees(centroid);
                const float flood_penalty = constraints->sampleFloodMask(centroid) > 0u ? 0.18f : 0.0f;
                district.desirability = std::clamp(1.0f - (slope / 45.0f) + nature * 0.25f - flood_penalty, 0.0f, 1.0f);
            }
        }

        return districts;
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
        const std::vector<BlockPolygon>& blocks,
        const SiteProfile* profile
    ) {
        Urban::LotGenerator::Config lot_cfg;
        lot_cfg.min_lot_width = 10.0f;
        lot_cfg.max_lot_width = 45.0f;
        lot_cfg.min_lot_depth = 12.0f;
        lot_cfg.max_lot_depth = 55.0f;
        lot_cfg.max_lots = config_.max_lots;

        if (profile != nullptr) {
            const float buildable_scale = std::clamp(profile->buildable_fraction, 0.15f, 1.0f);
            lot_cfg.max_lots = std::max<uint32_t>(
                600u,
                static_cast<uint32_t>(static_cast<float>(lot_cfg.max_lots) * buildable_scale));

            if (profile->mode == GenerationMode::Patchwork) {
                lot_cfg.min_lot_width = 8.0f;
                lot_cfg.min_lot_depth = 10.0f;
            } else if (profile->mode == GenerationMode::ConservationOnly) {
                lot_cfg.min_lot_width = 14.0f;
                lot_cfg.min_lot_depth = 18.0f;
            }
        }

        return Urban::LotGenerator::generate(roads, districts, blocks, lot_cfg, config_.seed);
    }

    siv::Vector<BuildingSite> CityGenerator::generateBuildings(
        const std::vector<LotToken>& lots,
        const SiteProfile* profile) {
        Urban::SiteGenerator::Config site_cfg;
        site_cfg.max_buildings = config_.max_buildings;
        if (profile != nullptr && profile->mode == GenerationMode::ConservationOnly) {
            site_cfg.max_buildings = std::max<uint32_t>(1000u, config_.max_buildings / 2u);
        }
        return Urban::SiteGenerator::generate(lots, site_cfg, config_.seed);
    }

} // namespace RogueCity::Generators
