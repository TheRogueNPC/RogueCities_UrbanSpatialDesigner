// FILE: ZoningBridge.cpp
// PURPOSE: Implementation of ZoningBridge (UI ? Generator translation)

#include "RogueCity/App/Integration/ZoningBridge.hpp"
#include "RogueCity/Generators/Districts/AESPClassifier.hpp"
#include <limits>
#include <chrono>

namespace RogueCity::App::Integration {

void ZoningBridge::Generate(const UiConfig& ui_cfg, Core::Editor::GlobalState& gs) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Translate UI config to generator config
    auto gen_config = TranslateConfig(ui_cfg);
    
    // Prepare input from current GlobalState
    auto input = PrepareInput(gs);
    
    // Run generator
    Generators::ZoningGenerator generator;
    auto output = generator.generate(input, gen_config);
    
    // Populate GlobalState with output
    PopulateGlobalState(output, gs);
    
    // Record statistics
    auto end_time = std::chrono::high_resolution_clock::now();
    m_last_stats.lots_created = static_cast<int>(output.lots.size());
    m_last_stats.buildings_placed = static_cast<int>(output.buildings.size());
    m_last_stats.total_budget_allocated = output.totalBudgetUsed;
    m_last_stats.projected_population = static_cast<int>(output.totalPopulation);
    m_last_stats.generation_time_ms = static_cast<float>(output.generationTimeMs);
}

void ZoningBridge::ClearAll(Core::Editor::GlobalState& gs) {
    gs.lots.clear();
    gs.buildings.clear();
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
    
    // Performance
    if (ui_cfg.auto_threading) {
        cfg.parallelizationThreshold = static_cast<uint32_t>(ui_cfg.threading_threshold);
    } else {
        cfg.parallelizationThreshold = std::numeric_limits<uint32_t>::max();  // Disable threading
    }
    
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

    // Blocks not yet available in GlobalState (leave empty for now)
    input.blocks.clear();

    // CitySpec not wired yet (leave empty)
    input.citySpec = std::nullopt;
    
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
}

} // namespace RogueCity::App::Integration
