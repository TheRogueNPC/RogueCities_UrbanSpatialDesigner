/**
 * @file CityOutputApplier.hpp
 * @brief Functions and types for applying city generation output to the editor's global state.
 *
 * This header defines mechanisms for integrating generated city data (such as roads, districts, and other entities)
 * into the editor's global state. It provides options to control the scope and behavior of the application process.
 */

/**
 * @enum GenerationScope
 * @brief Specifies the scope of city generation output to apply.
 *
 * - RoadsOnly: Only apply generated roads.
 * - RoadsAndBounds: Apply roads and city boundary information.
 * - FullCity: Apply all generated city entities (roads, bounds, districts, etc.).
 */

/**
 * @struct CityOutputApplyOptions
 * @brief Options for applying city generation output to the editor's global state.
 *
 * @var scope
 *      The scope of entities to apply from the city generation output.
 * @var rebuild_viewport_index
 *      Whether to rebuild the viewport index after applying changes.
 * @var mark_dirty_layers_clean
 *      Whether to mark previously dirty layers as clean after applying changes.
 * @var preserve_locked_user_entities
 *      Whether to preserve user-locked entities during the application process.
 */

/**
 * @brief Applies the given city generation output to the editor's global state.
 *
 * Modifies roads, districts, and other entities in the global state based on the specified options.
 * The function respects the scope and behavioral flags provided in CityOutputApplyOptions.
 *
 * @param output
 *      The city generation output to apply.
 * @param global_state
 *      The editor's global state to modify.
 * @param options
 *      Options controlling how the output is applied (defaults to full city application).
 */
 
#pragma once

#include "RogueCity/Core/Editor/GlobalState.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"

#include <cstdint>

namespace RogueCity::App {
// Applies the given city generation output to the editor's global state, modifying roads, districts, and other entities as needed based on the specified options.
enum class GenerationScope : uint8_t {
    RoadsOnly = 0,
    RoadsAndBounds,
    FullCity
};

// Options for how to apply city generation output to the editor's global state.
struct CityOutputApplyOptions { 
    GenerationScope scope{ GenerationScope::FullCity };
    bool rebuild_viewport_index{ true };
    bool mark_dirty_layers_clean{ true };
    bool preserve_locked_user_entities{ true };
};

 
void ApplyCityOutputToGlobalState(
    const Generators::CityGenerator::CityOutput& output,
    Core::Editor::GlobalState& global_state,
    const CityOutputApplyOptions& options = {});

} // namespace RogueCity::App
