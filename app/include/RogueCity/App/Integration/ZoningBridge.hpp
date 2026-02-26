 /**
 * @brief Bridge between UI and ZoningGenerator for urban spatial design.
 *
 * The ZoningBridge class provides an interface for generating, clearing, and managing zoning,
 * lot, and building data within the application's GlobalState. It exposes configuration options
 * for UI integration, manages deterministic generation pipelines, and tracks generation statistics.
 *
 * Features:
 * - UI-exposed configuration for zoning and building generation.
 * - Generation of zones, lots, and buildings, populating the editor's GlobalState.
 * - Full city specification pipeline for deterministic city generation.
 * - Clearing of all generated zoning, lot, and building data.
 * - Retrieval of generation statistics for UI display.
 * - Internal helpers for configuration translation, input preparation, output population,
 *   and plan validation.
 */
 
#pragma once

#include "RogueCity/Generators/Pipeline/ZoningGenerator.hpp"
#include "RogueCity/Core/Editor/GlobalState.hpp"
#include <string>

namespace RogueCity::App::Integration {

/// Bridge between UI and ZoningGenerator
class ZoningBridge {
public:
    /// Configuration exposed to UI
    struct UiConfig {
        // Lot sizing (sliders in control panel)
        int min_lot_width = 10;
        int max_lot_width = 50;
        int min_lot_depth = 15;
        int max_lot_depth = 60;
        
        // Building constraints
        float min_building_coverage = 0.3f;  // 30%
        float max_building_coverage = 0.8f;  // 80%
        
        // Budget/Population
        float budget_per_capita = 100000.0f;
        int target_population = 50000;
        
        // Performance
        bool auto_threading = true;  // Automatic threshold detection
        int threading_threshold = 100;
        
        // Generation seed
        uint32_t rng_seed = 42;
    };
    
    /// Generate zones/lots/buildings and populate GlobalState
    void Generate(const UiConfig& ui_cfg, Core::Editor::GlobalState& gs);

    /// Full CitySpec pipeline: roads -> districts -> blocks -> lots -> buildings
    /// Uses the same zoning stage as `Generate` for deterministic parity.
    bool GenerateFromCitySpec(
        const Core::CitySpec& city_spec,
        const UiConfig& ui_cfg,
        Core::Editor::GlobalState& gs,
        std::string* out_error = nullptr);
    
    /// Clear all zones/lots/buildings from GlobalState
    void ClearAll(Core::Editor::GlobalState& gs);
    
    /// Get last generation statistics (for UI display)
    struct Stats {
        int lots_created = 0;
        int buildings_placed = 0;
        float total_budget_allocated = 0.0f;
        int projected_population = 0;
        float generation_time_ms = 0.0f;
    };
    Stats GetLastStats() const { return m_last_stats; }
    
private:
    Stats m_last_stats;
    
    // Helper: Convert UiConfig ? Generators::ZoningGenerator::Config
    Generators::ZoningGenerator::Config TranslateConfig(const UiConfig& ui_cfg);
    
    // Helper: Prepare input from GlobalState
    Generators::ZoningGenerator::ZoningInput PrepareInput(const Core::Editor::GlobalState& gs);
    
    // Helper: Populate GlobalState from output
    void PopulateGlobalState(const Generators::ZoningGenerator::ZoningOutput& output, Core::Editor::GlobalState& gs);

    // Helper: Validate generated plan against active world constraints.
    void RunPlanValidation(Core::Editor::GlobalState& gs);
};

} // namespace RogueCity::App::Integration
