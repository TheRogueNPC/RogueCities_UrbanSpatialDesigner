// FILE: ZoningBridge.cpp
// PURPOSE: Implementation of ZoningBridge (UI ? Generator translation)

#include "RogueCity/App/Integration/ZoningBridge.hpp"
#include "RogueCity/Generators/Districts/AESPClassifier.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include "RogueCity/Generators/Pipeline/CitySpecAdapter.hpp"
#include "RogueCity/Generators/Pipeline/PlanValidatorGenerator.hpp"
#include "RogueCity/Generators/Urban/BlockGenerator.hpp"
#include <limits>
#include <algorithm>
#include <unordered_map>

namespace RogueCity::App::Integration {

void ZoningBridge::Generate(const UiConfig& ui_cfg, Core::Editor::GlobalState& gs) {
    // Translate UI config to generator config
    auto gen_config = TranslateConfig(ui_cfg);
    
    // Prepare input from current GlobalState
    auto input = PrepareInput(gs);
    
    // Run generator
    Generators::ZoningGenerator generator;
    auto output = generator.generate(input, gen_config);

    if (gs.blocks.size() == 0 && !input.blocks.empty()) {
        for (const auto& block : input.blocks) {
            gs.blocks.add(block);
        }
    }
    
    // Populate GlobalState with output
    PopulateGlobalState(output, gs);
    RunPlanValidation(gs);

    gs.generation_stats.roads_generated = static_cast<uint32_t>(gs.roads.size());
    gs.generation_stats.districts_generated = static_cast<uint32_t>(gs.districts.size());
    gs.generation_stats.lots_generated = static_cast<uint32_t>(gs.lots.size());
    gs.generation_stats.buildings_generated = static_cast<uint32_t>(gs.buildings.size());
    gs.generation_stats.generation_time_ms = static_cast<float>(output.generationTimeMs);
    gs.generation_stats.used_parallelization = output.usedParallelization;
    
    // Record statistics
    m_last_stats.lots_created = static_cast<int>(output.lots.size());
    m_last_stats.buildings_placed = static_cast<int>(output.buildings.size());
    m_last_stats.total_budget_allocated = output.totalBudgetUsed;
    m_last_stats.projected_population = static_cast<int>(output.totalPopulation);
    m_last_stats.generation_time_ms = static_cast<float>(output.generationTimeMs);
}

bool ZoningBridge::GenerateFromCitySpec(
    const Core::CitySpec& city_spec,
    const UiConfig& ui_cfg,
    Core::Editor::GlobalState& gs,
    std::string* out_error) {
    gs.active_city_spec = city_spec;

    Generators::CitySpecGenerationRequest request;
    std::string adapter_error;
    if (!Generators::CitySpecAdapter::TryBuildRequest(city_spec, request, &adapter_error)) {
        if (out_error) {
            *out_error = adapter_error.empty()
                ? "Failed to convert CitySpec into generator request."
                : adapter_error;
        }
        return false;
    }

    Generators::CityGenerator city_generator;
    auto city_output = city_generator.generate(request.axioms, request.config, &gs);

    gs.params.seed = request.config.seed;
    gs.generation.seed = request.config.seed;
    gs.generation.width = request.config.width;
    gs.generation.height = request.config.height;
    gs.generation.cell_size = request.config.cell_size;

    gs.roads.clear();
    for (const auto& road : city_output.roads) {
        gs.roads.add(road);
    }

    gs.districts.clear();
    for (const auto& district : city_output.districts) {
        gs.districts.add(district);
    }

    gs.blocks.clear();
    for (const auto& block : city_output.blocks) {
        gs.blocks.add(block);
    }

    gs.world_constraints = city_output.world_constraints;
    gs.site_profile = city_output.site_profile;
    gs.plan_violations = city_output.plan_violations;
    gs.plan_approved = city_output.plan_approved;

    gs.generation_stats.roads_generated = static_cast<uint32_t>(gs.roads.size());
    gs.generation_stats.districts_generated = static_cast<uint32_t>(gs.districts.size());

    Generate(ui_cfg, gs);
    return true;
}

void ZoningBridge::ClearAll(Core::Editor::GlobalState& gs) {
    gs.blocks.clear();
    gs.lots.clear();
    gs.buildings.clear();
    gs.world_constraints = Core::WorldConstraintField{};
    gs.site_profile = Core::SiteProfile{};
    gs.plan_violations.clear();
    gs.plan_approved = true;
    gs.generation_stats = Core::GenerationStats{};
    m_last_stats = Stats{};
}

Generators::ZoningGenerator::Config ZoningBridge::TranslateConfig(const UiConfig& ui_cfg) {
    Generators::ZoningGenerator::Config cfg;
    
    // Lot sizing
    cfg.minLotWidth = static_cast<float>(ui_cfg.min_lot_width);
    cfg.maxLotWidth = static_cast<float>(ui_cfg.max_lot_width);
    cfg.minLotDepth = static_cast<float>(ui_cfg.min_lot_depth);
    cfg.maxLotDepth = static_cast<float>(ui_cfg.max_lot_depth);
    
    // Building constraints (map UI percentage to generator ratio)
    // UI config not directly mapped in current ZoningGenerator::Config
    // This would need to be added to ZoningGenerator if required
    
    // Budget/Population
    // Map to ZoningGenerator budget fields
    cfg.totalBuildingBudget = ui_cfg.budget_per_capita * ui_cfg.target_population;
    cfg.seed = ui_cfg.rng_seed;
    
    // Performance
    if (ui_cfg.auto_threading) {
        cfg.parallelizationThreshold = static_cast<uint32_t>(ui_cfg.threading_threshold);
    } else {
        cfg.parallelizationThreshold = std::numeric_limits<uint32_t>::max();  // Disable threading
    }

    cfg.maxLots = static_cast<uint32_t>(std::max(1000, ui_cfg.target_population));
    cfg.maxBuildings = static_cast<uint32_t>(std::max(1000, ui_cfg.target_population * 2));
    
    return cfg;
}

Generators::ZoningGenerator::ZoningInput ZoningBridge::PrepareInput(const Core::Editor::GlobalState& gs) {
    Generators::ZoningGenerator::ZoningInput input;
    
    // Copy axioms from GlobalState
    // Copy roads (fva::Container)
    input.roads = gs.roads;

    // Copy districts (fva::Container -> std::vector)
    input.districts.reserve(gs.districts.size());
    for (const auto& district : gs.districts) {
        input.districts.push_back(district);
    }

    input.blocks.reserve(gs.blocks.size());
    for (const auto& block : gs.blocks) {
        input.blocks.push_back(block);
    }

    if (input.blocks.empty()) {
        input.blocks = Generators::Urban::BlockGenerator::generate(input.districts);
    }

    input.citySpec = gs.active_city_spec;
    
    return input;
}

void ZoningBridge::PopulateGlobalState(const Generators::ZoningGenerator::ZoningOutput& output, Core::Editor::GlobalState& gs) {
    // Clear existing lots/buildings
    gs.lots.clear();
    gs.buildings.clear();
    
    // Add new lots (use FVA for stable handles)
    for (const auto& lot : output.lots) {
        gs.lots.add(lot);
    }

    // Add new buildings (siv::Vector -> siv::Vector)
    for (size_t i = 0; i < output.buildings.size(); ++i) {
        gs.buildings.push_back(output.buildings[i]);
    }

    std::unordered_map<uint32_t, float> district_budgets;
    district_budgets.reserve(gs.lots.size());
    for (const auto& lot : gs.lots) {
        district_budgets[lot.district_id] += lot.budget_allocation;
    }

    std::unordered_map<uint32_t, uint32_t> district_building_counts;
    district_building_counts.reserve(gs.buildings.size());
    for (const auto& building : gs.buildings) {
        district_building_counts[building.district_id] += 1u;
    }

    uint32_t total_buildings = 0;
    for (const auto& [_, count] : district_building_counts) {
        total_buildings += count;
    }

    for (auto& district : gs.districts) {
        district.budget_allocated = district_budgets[district.id];
        if (total_buildings > 0) {
            const uint32_t district_buildings = district_building_counts[district.id];
            const float share = static_cast<float>(district_buildings) / static_cast<float>(total_buildings);
            district.projected_population = static_cast<uint32_t>(share * static_cast<float>(output.totalPopulation));
        } else {
            district.projected_population = 0;
        }
    }
}

void ZoningBridge::RunPlanValidation(Core::Editor::GlobalState& gs) {
    gs.plan_violations.clear();
    gs.plan_approved = true;
    if (!gs.world_constraints.isValid()) {
        return;
    }

    std::vector<Core::LotToken> lots;
    lots.reserve(gs.lots.size());
    for (const auto& lot : gs.lots) {
        lots.push_back(lot);
    }

    Generators::PlanValidatorGenerator validator;
    Generators::PlanValidatorGenerator::Input input;
    input.constraints = &gs.world_constraints;
    input.site_profile = &gs.site_profile;
    input.roads = &gs.roads;
    input.lots = &lots;

    auto output = validator.validate(input);
    gs.plan_violations = std::move(output.violations);
    gs.plan_approved = output.approved;
}

} // namespace RogueCity::App::Integration
